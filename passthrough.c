// play USB input on ALSA output

#include "settings.h"
#include "libusb.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include "alsa/asoundlib.h"

// Must be smaller than settings.h FRAGMENT_SIZE
#undef FRAGMENT_SIZE
#define FRAGMENT_SIZE 64

#define TOTAL_URBS 16
#define EP1_SIZE 0x40
#define SAMPLERATE 44100
#define CHANNELS 2
#define BITS 16
#define OUTPUT_FRAGMENT 64
#define FIFO_BYTES 512
#define SOUNDCARD "hw:0"
#define CLAMP(x, y, z) ((x) = ((x) < (y) ? (y) : ((x) > (z) ? (z) : (x))))

typedef struct
{
	unsigned char *data;
	int size;
	int allocated;
// Next position to write
	int in_position;
// Next position to read
	int out_position;
	pthread_mutex_t lock;
	sem_t data_ready;
} fifo_t;

struct libusb_device_handle *devh;
struct libusb_transfer* in_urb[TOTAL_URBS];
struct libusb_transfer* out_urb;
int total_bytes = 0;
int total_errors = 0;
int usb_write_ready = 0;
int device_buffer = 0;
snd_pcm_t *dsp_out = 0;
// number / 256
int gain = 1 * 256;
fifo_t fifo;


void init_fifo(fifo_t *fifo, int size)
{
	bzero(fifo, sizeof(fifo_t));

	pthread_mutexattr_t mutex_attr;
	pthread_mutexattr_init(&mutex_attr);
	pthread_mutex_init(&fifo->lock, &mutex_attr);
	sem_init(&fifo->data_ready, 0, 0);

	fifo->data = malloc(size);
	fifo->allocated = size;
}

void delete_fifo(fifo_t *fifo)
{
	if(fifo->data) free(fifo->data);
	sem_destroy(&fifo->data_ready);
	pthread_mutex_destroy(&fifo->lock);
	bzero(fifo, sizeof(fifo_t));
}

void wait_fifo(fifo_t *fifo)
{
	sem_wait(&fifo->data_ready);
}

int fifo_size(fifo_t *fifo)
{
	pthread_mutex_lock(&fifo->lock);
	int result = fifo->size;
	pthread_mutex_unlock(&fifo->lock);
	return result;
}

void write_fifo(fifo_t *fifo, unsigned char *data, int size)
{
	int data_offset = 0;

	pthread_mutex_lock(&fifo->lock);
	
	
	while(size > 0)
	{
		int fragment = size;
// Wrap write pointer
		if(fifo->in_position + fragment > fifo->allocated)
		{
			fragment = fifo->allocated - fifo->in_position;
		}


		if(fifo->size + fragment > fifo->allocated)
		{
// Advance the read pointer until enough room is available.
// Must do this for audio.
			int difference = fifo->size + fragment - fifo->allocated;
			fifo->out_position += difference;
// Wrap it
			fifo->out_position %= fifo->allocated;
			fifo->size -= difference;
		}

		if(fragment <= 0) break;

		memcpy(fifo->data + fifo->in_position, data + data_offset, fragment);

		fifo->in_position += fragment;
		fifo->size += fragment;
		data_offset += fragment;
		size -= fragment;
		if(fifo->in_position >= fifo->allocated)
			fifo->in_position = 0;
	}
	
//printf("write_fifo %d %d\n", __LINE__, fifo->size);

	pthread_mutex_unlock(&fifo->lock);
	
	sem_post(&fifo->data_ready);
}

unsigned char read_fifo(fifo_t *fifo)
{
	unsigned char result = 0xff;
	pthread_mutex_lock(&fifo->lock);

	if(fifo->size > 0)
	{
		result = fifo->data[fifo->out_position];
		fifo->out_position++;
		fifo->size--;
		if(fifo->out_position >= fifo->allocated)
			fifo->out_position = 0;
	}
	pthread_mutex_unlock(&fifo->lock);

	return result;
}

void skip_fifo_bytes(fifo_t *fifo, int bytes)
{
	pthread_mutex_lock(&fifo->lock);
	fifo->out_position += bytes;
	fifo->out_position %= fifo->allocated;
	fifo->size -= bytes;
	if(fifo->size < 0) fifo->size = 0;
	pthread_mutex_unlock(&fifo->lock);
}

void copy_fifo_buffer(fifo_t *fifo, unsigned char *dst, int offset, int size)
{
	pthread_mutex_lock(&fifo->lock);

/*
 * printf("copy_fifo_buffer %d %p %d %d\n", 
 * __LINE__,
 * dst,
 * offset,
 * size);
 */
// Convert offset to physical offset in FIFO buffer
	offset += fifo->out_position;
// Wrap it
	offset %= fifo->allocated;

	while(size > 0)
	{
		int fragment = size;
		if(fragment + offset > fifo->allocated)
			fragment = fifo->allocated - offset;
		
		memcpy(dst, fifo->data + offset, fragment);

		dst += fragment;
		size -= fragment;
		offset += fragment;
		if(offset >= fifo->allocated)
			offset = 0;
	}
	
	pthread_mutex_unlock(&fifo->lock);
}


static void in_callback(struct libusb_transfer *transfer)
{
	if(transfer->status == LIBUSB_TRANSFER_COMPLETED)
	{
		int size = transfer->actual_length;
		total_bytes += size;

// pack into 2 bytes per sample
		int i;
		unsigned char *in_ptr = transfer->buffer;
		unsigned char *out_ptr = transfer->buffer;
		for(i = 0; i < size; i += 4)
		{
			int sample = in_ptr[0] |
				(in_ptr[1] << 8) |
				(in_ptr[2] << 16);
			sample = sample * gain / 256;
			CLAMP(sample, -0x7fffff, 0x7fffff);
			out_ptr[0] = (sample >> 8) & 0xff;
			out_ptr[1] = (sample >> 16) & 0xff;
			out_ptr += 2;
			in_ptr += 4;
		}
		int new_size = out_ptr - transfer->buffer;

		write_fifo(&fifo, transfer->buffer, new_size);
//printf("in_callback %d: total_bytes=%d\n", __LINE__, total_bytes);
	}
	else
	{
printf("in_callback %d\n", __LINE__);
		total_errors++;
	}

	libusb_submit_transfer(transfer);
}


static void out_callback(struct libusb_transfer *transfer)
{
	if(transfer->status != 0) printf("out_callback %d status=%d\n", __LINE__, transfer->status);
// Release write routine
	usb_write_ready = 1;
}


snd_pcm_format_t translate_format(int format)
{
	switch(format)
	{
		case 8:
			return SND_PCM_FORMAT_S8;
			break;
		case 16:
			return SND_PCM_FORMAT_S16_LE;
			break;
		case 24:
			return SND_PCM_FORMAT_S24_LE;
			break;
		case 32:
			return SND_PCM_FORMAT_S32_LE;
			break;
	}
}



void set_params(snd_pcm_t *dsp, 
	int channels, 
	int bits,
	int samplerate,
	int samples)
{
	snd_pcm_hw_params_t *params;
	snd_pcm_sw_params_t *swparams;
	int err;

	snd_pcm_hw_params_alloca(&params);
	snd_pcm_sw_params_alloca(&swparams);
	err = snd_pcm_hw_params_any(dsp, params);

	if (err < 0) 
	{
		printf("AudioALSA::set_params: no PCM configurations available\n");
		return;
	}

	snd_pcm_hw_params_set_access(dsp, 
		params,
		SND_PCM_ACCESS_RW_INTERLEAVED);
	snd_pcm_hw_params_set_format(dsp, 
		params, 
		translate_format(bits));
	snd_pcm_hw_params_set_channels(dsp, 
		params, 
		channels);
	snd_pcm_hw_params_set_rate_near(dsp, 
		params, 
		(unsigned int*)&samplerate, 
		(int*)0);

// Buffers written must be equal to period_time
	int buffer_time;
	int period_time;
	buffer_time = (int)((int64_t)samples * 1000000 * 2 / samplerate + 0.5);
	period_time = samples * samplerate / 1000000;


//printf("AudioALSA::set_params 1 %d %d %d\n", samples, buffer_time, period_time);
	snd_pcm_hw_params_set_buffer_time_near(dsp, 
		params,
		(unsigned int*)&buffer_time, 
		(int*)0);
	snd_pcm_hw_params_set_period_time_near(dsp, 
		params,
		(unsigned int*)&period_time, 
		(int*)0);
//printf("AudioALSA::set_params 5 %d %d\n", buffer_time, period_time);
	err = snd_pcm_hw_params(dsp, params);
	if(err < 0)
	{
		printf("AudioALSA::set_params: hw_params failed\n");
		return;
	}

	snd_pcm_uframes_t chunk_size = 1024;
	snd_pcm_uframes_t buffer_size = 262144;
	snd_pcm_hw_params_get_period_size(params, &chunk_size, 0);
	snd_pcm_hw_params_get_buffer_size(params, &buffer_size);
//printf("AudioALSA::set_params 10 %d %d\n", chunk_size, buffer_size);

	snd_pcm_sw_params_current(dsp, swparams);
	size_t xfer_align = 1 /* snd_pcm_sw_params_get_xfer_align(swparams) */;
	unsigned int sleep_min = 0;
	err = snd_pcm_sw_params_set_sleep_min(dsp, swparams, sleep_min);
	int n = chunk_size;
	err = snd_pcm_sw_params_set_avail_min(dsp, swparams, n);
	err = snd_pcm_sw_params_set_xfer_align(dsp, swparams, xfer_align);
	if(snd_pcm_sw_params(dsp, swparams) < 0)
	{
		printf("AudioALSA::set_params: snd_pcm_sw_params failed\n");
	}

	device_buffer = samples * bits / 8 * channels;

printf("AudioALSA::set_params 100 samples=%d device_buffer=%d\n", samples, device_buffer);

//	snd_pcm_hw_params_free(params);
//	snd_pcm_sw_params_free(swparams);
}



int open_output()
{
	int result = 0;
	result = snd_pcm_open(&dsp_out, SOUNDCARD, SND_PCM_STREAM_PLAYBACK, 0);
	if(result < 0)
	{
		dsp_out = 0;
		printf("AudioALSA::open_output %s: %s\n", SOUNDCARD, snd_strerror(result));
		return 1;
	}

	set_params(dsp_out, 
		CHANNELS, 
		BITS,
		SAMPLERATE,
		OUTPUT_FRAGMENT);
	return result;
}


int close_output()
{
	if(dsp_out)
	{
		snd_pcm_close(dsp_out);
		dsp_out = 0;
	}
	return 0;
}


void* status_thread(void *ptr)
{
	while(1)
	{
//		printf("Total bytes: %d total errors: %d\n", total_bytes, total_errors);
		sleep(1);
	}
}


void* writer_thread(void *ptr)
{
	unsigned char *buffer = malloc(device_buffer);
	while(1)
	{

		wait_fifo(&fifo);
		int size = fifo_size(&fifo);
		
		while(size >= device_buffer)
		{
			int fragment = size;
			if(fragment > device_buffer)
				fragment = device_buffer;
			copy_fifo_buffer(&fifo, buffer, 0, fragment);
			skip_fifo_bytes(&fifo, fragment);

			int attempts = 0;
			int done = 0;
			while(attempts < 2 && !done)
			{
//printf("writer_thread %d %d\n", __LINE__, size);
				if(snd_pcm_writei(dsp_out, 
					buffer, 
					fragment / (CHANNELS * BITS / 8)) < 0)
				{
					printf("AudioALSA::write_buffer underrun\n");
					close_output();
					open_output();
					attempts++;
				}
				else
				{
					done = 1;
				}
			}

			size -= fragment;
		}
	}
}


int main(int argc, char *argv[])
{
	int result = 0;
	int i;


// init sound driver
	open_output();
	
	libusb_init(0);


	devh = libusb_open_device_with_vid_pid(0, 0x04d8, 0x000b);
	if(!devh)
	{
		printf("main: Couldn't find CP33\n");
		return 1;
	}


	result = libusb_claim_interface(devh, 0);
	if(result < 0)
	{
		printf("main: CP33 in use\n");
		return 1;
	}



	pthread_t tid;
	pthread_attr_t  attr;
	pthread_attr_init(&attr);
	pthread_create(&tid, &attr, status_thread, 0);


	init_fifo(&fifo, FIFO_BYTES);
	pthread_create(&tid, &attr, writer_thread, 0);

	out_urb = libusb_alloc_transfer(0);
	unsigned char *out_buffer = calloc(1, EP1_SIZE);
	out_buffer[0] = FRAGMENT_SIZE & 0xff;
	out_buffer[1] = (FRAGMENT_SIZE >> 8) & 0xff;
	out_buffer[2] = (FRAGMENT_SIZE >> 16) & 0xff;
	out_buffer[3] = (FRAGMENT_SIZE >> 24) & 0xff;
	libusb_fill_bulk_transfer(out_urb,
		devh, 
		1 | LIBUSB_ENDPOINT_OUT,
		out_buffer, 
		EP1_SIZE, 
		out_callback,
		0, 
		1000);
// start streaming
	libusb_submit_transfer(out_urb);



	for(i = 0; i < TOTAL_URBS; i++)
	{
		in_urb[i] = libusb_alloc_transfer(0);
		libusb_fill_bulk_transfer(in_urb[i],
			devh, 
			1 | LIBUSB_ENDPOINT_IN,
			malloc(FRAGMENT_SIZE), 
			FRAGMENT_SIZE, 
			in_callback,
			0, 
			1000);
		libusb_submit_transfer(in_urb[i]);
	}


	while(1)
	{
		libusb_handle_events(0);
	}
	return 0;
}






