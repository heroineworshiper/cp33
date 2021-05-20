/*
 * Piano audio processor
 *
 * Copyright (C) 2021 Adam Williams <broadcast at earthling dot net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */



// augment the CP33 with networking & audio functions
// metronome, recording, reverb, someday compression
// uses I2S to USB converter


// scp metro.wav favicon.ico record.png stop.png single.gif piano:
// scp index.html piano:
// scp piano.c pianoserver.c piano.h piano:
// objcopy -B arm -I binary -O elf32-littlearm metro.wav metro.o
// gcc -O3 -o piano piano.c pianoserver.c metro.o -lm -lrt -lpthread -lasound -lusb-1.0 -D__USE_FILE_OFFSET64 -D__USE_LARGEFILE64
// nohup ./piano >& /dev/null&

#define _GNU_SOURCE
#include <alsa/asoundlib.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>
#include <ctype.h>
#include <libusb-1.0/libusb.h>
#include <math.h>
#include "piano.h"


#define CHANNELS 2
#define BITS_PER_SAMPLE 24
#define SAMPLESIZE (CHANNELS * BITS_PER_SAMPLE / 8)
#define PAGE_SIZE 4096

// USB values from recordcp33.c
#define TOTAL_URBS 256
#define EP1_SIZE 0x40
#define PLAYBACK_BUFFER 0x100000
// multiple of SAMPLESIZE.  30 minute buffer
#define RECORD_BUFFER (SAMPLESIZE * 44100 * 60 * 30)
// number of bytes to send from the CP33 at a time.  Limited by settings.h
#define I2S_FRAGMENT_SIZE (64 * CHANNELS * sizeof(int))

// reread the directory.  Causes recording overruns.
#define POLLING

// reverb
int do_reverb = 0;
// in samples
int reverb_len = SAMPLERATE;
int reverb_refs = 150;
// DEBUG
//int reverb_refs = 1;
// ms delay before 1st reflection in addition to the FFT delay
int reverb_delay1 = 20;
// 1st reflection level in DB
float reverb_level1 = -24.0f;
// DEBUG
//float reverb_level1 = 0.0f;
// last reflection level in DB
float reverb_level2 = -46.0f;
// lowpass cutoff in HZ
int reverb_lowpass = 6000;

float *reverb_bufs[CHANNELS];
int *reverb_channels[CHANNELS];
int *reverb_offsets[CHANNELS];
float *reverb_levels[CHANNELS];

#define FFT_WINDOW 2048
#define HALF_WINDOW (FFT_WINDOW / 2)
// bandpassed fragment to be added to accumulator
float *fft_dissolved[CHANNELS];
// amount of fft_dissolved transferred to ALSA
int fft_emptied = 0;
// amount of fft_input filled
int fft_filled = HALF_WINDOW;
float *fft_input_r[CHANNELS];
float *fft_output_r[CHANNELS];
float *fft_output_i[CHANNELS];
float *filtered_output_r[CHANNELS];
float *filtered_output_i[CHANNELS];


// recording
#define WAV_PATH "/home/mpeg/"
#define SETTINGS_PATH "/root/.pianorc"
#define HEADER_SIZE 44
char filename[TEXTLEN];
int next_file = 0;
#define MAXFILES 99
int64_t total_written = 0;
int64_t total_remane = 0;
int glitches = 0;
int need_recording = 0;
int need_stop = 0;
int recording = 0;
int write_fd = -1;
uint8_t page_buffer_[PAGE_SIZE * 2];
uint8_t *page_buffer = 0;
int page_used = 0;
sem_t write_sem;
pthread_mutex_t write_mutex;



// metronome
int do_metronome = 0;
int bpm = 60;
int preset1 = 60;
int preset2 = 60;
int preset3 = 60;
int preset4 = 60;
// 0 - 100
int metronome_level = 25;
int metro_offset = 0;
extern int _binary_metro_wav_start;
extern int _binary_metro_wav_size;
const int metro_wav_size = (int)&_binary_metro_wav_size - HEADER_SIZE;
const uint8_t *metro_wav_start = ((uint8_t*)&_binary_metro_wav_start) + HEADER_SIZE;




// ALSA values from usbmic.c
// samples
#define FRAGMENT 64
// number of hardware buffers
#define TOTAL_FRAGMENTS 3


// FIFO fill range
// drop the ALSA write if it's below this
#define MIN_FILL FRAGMENT
// drop the FIFO if it's above this
#define MAX_FILL (FRAGMENT * 3)
// stretch the FIFO if it's above this
#define MID_FILL (FRAGMENT * 2)


//#define TEST_AUDIO
//#ifdef TEST_AUDIO
// samples
//#define TEST_SIZE FRAGMENT
#define TEST_SIZE 512
int16_t test_wave[TEST_SIZE * CHANNELS];
int test_offset = 0;
//#endif

// 0-100
int speaker_volume = 50;
int line_volume = 0;

snd_pcm_t *dsp_out = 0;
struct libusb_device_handle *devh;
struct libusb_transfer* in_urb[TOTAL_URBS];
struct libusb_transfer* out_urb;
unsigned char *out_buffer;
unsigned char *in_buffer[TOTAL_URBS];
int usb_disconnected = 1;

int total_bytes = 0;
int total_errors = 0;
int usb_write_ready = 0;

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

fifo_t playback_fifo;
fifo_t record_fifo;


// server
extern pthread_mutex_t www_mutex;
void init_server();




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



unsigned int samples_to_bits(unsigned int samples)
{
    unsigned int i;

    for(i = 0; ; i++)
    {
        if(samples & (1 << i))
            return i;
    }
	return i;
}

unsigned int reverse_bits(unsigned int index, unsigned int bits)
{
    unsigned int i, rev;

    for(i = rev = 0; i < bits; i++)
    {
        rev = (rev << 1) | (index & 1);
        index >>= 1;
    }

    return rev;
}


int symmetry(float *freq_real, float *freq_imag)
{
    int h = FFT_WINDOW / 2;
    for(int i = h + 1; i < FFT_WINDOW; i++)
    {
        freq_real[i] = freq_real[FFT_WINDOW - i];
        freq_imag[i] = -freq_imag[FFT_WINDOW - i];
    }
	return 0;
}


void do_fft(int inverse,         // 0 = forward FFT, 1 = inverse
    float *real_in, 
    float *imag_in, 
    float *real_out,
    float *imag_out)
{
    unsigned int num_bits;    // Number of bits needed to store indices
    unsigned int i, j, k, n;
    unsigned int block_size, block_end;

    float angle_numerator = 2.0f * M_PI;
    float tr, ti;     // temp real, temp imaginary

    if(inverse)
    {
        angle_numerator = -angle_numerator;
    }

    num_bits = samples_to_bits(FFT_WINDOW);

// Do simultaneous data copy and bit-reversal ordering into outputs

    if(imag_in)
    {
        for(i = 0; i < FFT_WINDOW; i++)
        {
            j = reverse_bits(i, num_bits);
            real_out[j] = real_in[i];
            imag_out[j] = imag_in[i];
        }
    }
    else
    {
        for(i = 0; i < FFT_WINDOW; i++)
        {
            j = reverse_bits(i, num_bits);
            real_out[j] = real_in[i];
            imag_out[j] = 0.0;
        }
    }

// Do the FFT itself

    block_end = 1;
    float delta_angle;
    float sm2;
    float sm1;
    float cm2;
    float cm1;
    float w;
    float ar[3], ai[3];
    float temp;
    for(block_size = 2; block_size <= FFT_WINDOW; block_size <<= 1)
    {
        delta_angle = angle_numerator / (float)block_size;
        sm2 = sinf(-2 * delta_angle);
        sm1 = sinf(-delta_angle);
        cm2 = cosf(-2 * delta_angle);
        cm1 = cosf(-delta_angle);
        w = 2 * cm1;

        for(i = 0; i < FFT_WINDOW; i += block_size)
        {
            ar[2] = cm2;
            ar[1] = cm1;

            ai[2] = sm2;
            ai[1] = sm1;

            for(j = i, n = 0; n < block_end; j++, n++)
            {
                ar[0] = w * ar[1] - ar[2];
                ar[2] = ar[1];
                ar[1] = ar[0];

                ai[0] = w * ai[1] - ai[2];
                ai[2] = ai[1];
                ai[1] = ai[0];

                k = j + block_end;
                tr = ar[0] * real_out[k] - ai[0] * imag_out[k];
                ti = ar[0] * imag_out[k] + ai[0] * real_out[k];

                real_out[k] = real_out[j] - tr;
                imag_out[k] = imag_out[j] - ti;

                real_out[j] += tr;
                imag_out[j] += ti;
            }
        }

        block_end = block_size;
    }

// Normalize if inverse transform

    if(inverse)
    {
        float denom = (float)FFT_WINDOW;

        for (i = 0; i < FFT_WINDOW; i++)
        {
            real_out[i] /= denom;
            imag_out[i] /= denom;
        }
    }
}


// apply bandpass to audio fragment
void do_filter(int16_t *alsa_ptr0)
{
    int channel, i, j;
    
    for(channel = 0; channel < CHANNELS; channel++)
    {
        int16_t *alsa_ptr = alsa_ptr0 + channel;

// new fragment -> end of FFT window
        float *dst = fft_input_r[channel] + fft_filled;
        for(i = 0; i < FRAGMENT; i++)
        {
            *dst++ = *alsa_ptr;
            alsa_ptr += CHANNELS;
        }
    }
    fft_filled += FRAGMENT;
    
// perform FFT on a complete window
    if(fft_filled >= FFT_WINDOW)
    {
        for(channel = 0; channel < CHANNELS; channel++)
        {
// perform FFT
            do_fft(0, fft_input_r[channel], 0, fft_output_r[channel], fft_output_i[channel]);

// lowpass
            int cutoff = (int)(reverb_lowpass / 
                ((float)SAMPLERATE / 2 / HALF_WINDOW));
            bzero(fft_output_r[channel] + cutoff, sizeof(float) * (FFT_WINDOW / 2 - cutoff + 1));
            bzero(fft_output_i[channel] + cutoff, sizeof(float) * (FFT_WINDOW / 2 - cutoff + 1));

            symmetry(fft_output_r[channel], fft_output_i[channel]);


// perform inverse FFT
            do_fft(1, fft_output_r[channel], fft_output_i[channel], filtered_output_r[channel], filtered_output_i[channel]);

            float *dst = fft_dissolved[channel];
            float *filtered_output = filtered_output_r[channel];
            for(j = 0; j < HALF_WINDOW; j++)
            {
// dissolve a fragment into the reverb input buffer & shift it out
                dst[j] = (dst[j + HALF_WINDOW] * (HALF_WINDOW - j) + 
                        filtered_output[j] * j) / 
                    HALF_WINDOW;
            }
// shift next fragment in with no dissolve
            memcpy(dst + HALF_WINDOW, filtered_output + HALF_WINDOW, HALF_WINDOW * sizeof(float));

// shift FFT window out        
            memcpy(fft_input_r[channel], 
                fft_input_r[channel] + HALF_WINDOW, 
                HALF_WINDOW * sizeof(float));
        }
        
        fft_filled = HALF_WINDOW;
        fft_emptied = 0;
    }
}


void write_header()
{
	uint8_t *header = page_buffer;
   	int offset = 0;
    header[offset++] = 'R';
    header[offset++] = 'I';
    header[offset++] = 'F';
    header[offset++] = 'F';
    header[offset++] = 0xff;
    header[offset++] = 0xff;
    header[offset++] = 0xff;
    header[offset++] = 0x7f;
    header[offset++] = 'W';
    header[offset++] = 'A';
    header[offset++] = 'V';
    header[offset++] = 'E';
    header[offset++] = 'f';
    header[offset++] = 'm';
    header[offset++] = 't';
    header[offset++] = ' ';
    header[offset++] = 16;
    header[offset++] = 0;
    header[offset++] = 0;
    header[offset++] = 0;
    header[offset++] = 1;
    header[offset++] = 0;
    // channels
    header[offset++] = CHANNELS;
    header[offset++] = 0;
    // samplerate
    header[offset++] = (SAMPLERATE & 0xff);
    header[offset++] = ((SAMPLERATE >> 8) & 0xff);
    header[offset++] = ((SAMPLERATE >> 16) & 0xff);
    header[offset++] = ((SAMPLERATE >> 24) & 0xff);
    // byterate
    int byterate = SAMPLERATE * 3 * 2;
    header[offset++] = (byterate & 0xff);
    header[offset++] = ((byterate >> 8) & 0xff);
    header[offset++] = ((byterate >> 16) & 0xff);
    header[offset++] = ((byterate >> 24) & 0xff);
    // blockalign
    header[offset++] = BITS_PER_SAMPLE / 8 * CHANNELS;
    header[offset++] = 0;
    // bits per sample
    header[offset++] = BITS_PER_SAMPLE;
    header[offset++] = 0;
    header[offset++] = 'd';
    header[offset++] = 'a';
    header[offset++] = 't';
    header[offset++] = 'a';
    header[offset++] = 0xff;
    header[offset++] = 0xff;
    header[offset++] = 0xff;
    header[offset++] = 0x7f;
    
	page_used = offset;
}




snd_pcm_format_t bits_to_format(int bits)
{
	switch(bits)
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
// ALSA uses the term "period" as the buffer size.
	int period, 
// The number of buffers
	int nperiods)
{
	snd_pcm_hw_params_t *hwparams;
	snd_pcm_sw_params_t *swparams;
	int err;

	snd_pcm_hw_params_alloca(&hwparams);
	snd_pcm_sw_params_alloca(&swparams);
	err = snd_pcm_hw_params_any(dsp, hwparams);

	if (err < 0) 
	{
		printf("set_params: no PCM configurations available\n");
		return;
	}


// 	printf("set_params %d\n", __LINE__);
//     snd_output_t *log;
//     snd_output_stdio_attach(&log, stdout, 0);
//     snd_pcm_hw_params_dump(hwparams, log);

    
    

    snd_pcm_hw_params_set_access(dsp, 
		hwparams,
		SND_PCM_ACCESS_MMAP_INTERLEAVED);

	snd_pcm_hw_params_set_format(dsp, 
		hwparams, 
		bits_to_format(bits));
	snd_pcm_hw_params_set_channels(dsp, 
		hwparams, 
		channels);
	snd_pcm_hw_params_set_rate_near(dsp, 
		hwparams, 
		(unsigned int*)&samplerate, 
		(int*)0);




    snd_pcm_hw_params_set_period_size(dsp, 
        hwparams,
        period,
        0);
    unsigned int nperiodsp = nperiods;
    snd_pcm_hw_params_set_periods_min(dsp, hwparams, &nperiodsp, NULL);
    snd_pcm_hw_params_set_periods_near(dsp, hwparams, &nperiodsp, NULL);
    snd_pcm_hw_params_set_buffer_size(dsp, 
        hwparams,
        nperiodsp * period);

// check the values
	snd_pcm_uframes_t real_buffer_size;
	snd_pcm_uframes_t real_period_size;
	snd_pcm_hw_params_get_buffer_size(hwparams, &real_buffer_size );
	snd_pcm_hw_params_get_period_size(hwparams, &real_period_size, 0);

	printf("set_params %d: got buffer=%d period=%d\n", __LINE__, real_buffer_size, real_period_size);

/* write the parameters to device */
	err = snd_pcm_hw_params(dsp, hwparams);
	if(err < 0)
	{
		printf("set_params: hw_params failed\n");
		return;
	}


/* get the current swparams */
	snd_pcm_sw_params_current(dsp, swparams);
/* start the transfer when the buffer is full */
	snd_pcm_sw_params_set_start_threshold(dsp, swparams, 0);
	snd_pcm_sw_params_set_stop_threshold(dsp, swparams, period * nperiods);
/* allow the transfer when at least a period can be processed */
	snd_pcm_sw_params_set_avail_min(dsp, swparams, period );
/* align all transfers to 1 sample */
//	snd_pcm_sw_params_set_xfer_align(dsp, swparams, 1);
/* write the parameters to the playback device */
	snd_pcm_sw_params(dsp, swparams);
}


void close_alsa()
{
	if(dsp_out)
	{
		snd_pcm_close(dsp_out);
		dsp_out = 0;
	}
}


int open_alsa()
{
// playback
	int result = snd_pcm_open(&dsp_out, 
		"hw:1", 
		SND_PCM_STREAM_PLAYBACK, 
		SND_PCM_NONBLOCK);


	if(result < 0)
	{
		dsp_out = 0;
//		printf("open_alsa %d: %s\n", __LINE__, snd_strerror(result));
		return 1;
	}

	set_params(dsp_out, 
		CHANNELS, 
		16, // bits
		SAMPLERATE,
		FRAGMENT, // samples per fragment
		TOTAL_FRAGMENTS);

// fill with 0
	const snd_pcm_channel_area_t *areas;
	snd_pcm_uframes_t offset, frames;
	snd_pcm_avail_update(dsp_out);
	snd_pcm_mmap_begin(dsp_out,
		&areas, 
		&offset, 
		&frames);
    if(!areas[0].addr)
    {
        printf("open_alsa %d failed to mmap offset=%d frames=%d\n", 
            __LINE__, 
            offset, 
            frames);
        close_alsa();
        return 1;
    }

// assume this points to the entire buffer & write the entire buffer
	bzero(areas[0].addr, FRAGMENT * TOTAL_FRAGMENTS * CHANNELS * 2);

	snd_pcm_mmap_commit(dsp_out, offset, FRAGMENT * TOTAL_FRAGMENTS);

	snd_pcm_start(dsp_out);
    return 0;
}



// capture audio data from USB
static void in_callback(struct libusb_transfer *transfer)
{
	int i;
	if(transfer->status == LIBUSB_TRANSFER_COMPLETED)
	{
// length in bytes
		int size = transfer->actual_length;
		total_bytes += size;

// truncate if playback FIFO is full
	    pthread_mutex_lock(&playback_fifo.lock);
        if(size + playback_fifo.size > playback_fifo.allocated)
        {
            size = playback_fifo.allocated - playback_fifo.size;
            printf("in_callback %d: playback fifo full\n", __LINE__);
        }
	    pthread_mutex_unlock(&playback_fifo.lock);


        if(size > 0)
        {
// convert to little endian 32 bit & write to playback FIFO
		    unsigned char *in_ptr = transfer->buffer;
            unsigned char *out_ptr = playback_fifo.data + playback_fifo.in_position;
            unsigned char *out_end = playback_fifo.data + playback_fifo.allocated;
		    for(i = 0; i < size; i += 4)
		    {
			    out_ptr[3] = in_ptr[2];
			    out_ptr[2] = in_ptr[1];
			    out_ptr[1] = in_ptr[0];
                out_ptr[0] = 0;
			    out_ptr += 4;
			    in_ptr += 4;
                if(out_ptr >= out_end)
                {
                    out_ptr = playback_fifo.data;
                }
		    }
	        pthread_mutex_lock(&playback_fifo.lock);
            playback_fifo.in_position = out_ptr - playback_fifo.data;
            playback_fifo.size += size;
	        pthread_mutex_unlock(&playback_fifo.lock);
        }
        
        if(recording)
        {
            int samples = transfer->actual_length / CHANNELS / 4;
            
// truncate if record FIFO is full
	        pthread_mutex_lock(&record_fifo.lock);
            if(samples * SAMPLESIZE + record_fifo.size > record_fifo.allocated)
            {
                samples = (record_fifo.allocated - record_fifo.size) / SAMPLESIZE;
                if(total_written > 0)
                {
                    printf("in_callback %d: record fifo full\n", __LINE__);
                    glitches++;
                }
            }
    	    pthread_mutex_unlock(&record_fifo.lock);

            if(samples > 0)
            {
                unsigned char *in_ptr = transfer->buffer;
                unsigned char *out_ptr = record_fifo.data + record_fifo.in_position;
                unsigned char *out_end = record_fifo.data + record_fifo.allocated;
		        for(i = 0; i < samples * 2; i++)
		        {
			        out_ptr[2] = in_ptr[2];
			        out_ptr[1] = in_ptr[1];
			        out_ptr[0] = in_ptr[0];
			        out_ptr += 3;
			        in_ptr += 4;
                    if(out_ptr >= out_end)
                    {
                        out_ptr = record_fifo.data;
                    }
		        }
	            pthread_mutex_lock(&record_fifo.lock);
                record_fifo.in_position = out_ptr - record_fifo.data;
                record_fifo.size += samples * SAMPLESIZE;
	            pthread_mutex_unlock(&record_fifo.lock);
			    sem_post(&write_sem);
            }
        }
	}
	else
    if(transfer->status = LIBUSB_TRANSFER_STALL)
    {
// disconnected
        printf("in_callback %d disconnected\n", __LINE__);
        usb_disconnected = 1;
    }
    else
	{
        printf("in_callback %d status=%d\n", __LINE__, transfer->status);
		total_errors++;
	}

    if(!usb_disconnected)
    {
    	libusb_submit_transfer(transfer);
    }
}


static void out_callback(struct libusb_transfer *transfer)
{
	if(transfer->status != 0) printf("out_callback %d status=%d\n", __LINE__, transfer->status);
// Release write routine
	usb_write_ready = 1;
}

void set_speaker(int value)
{
    CLAMP(value, 0, 100);
    int value2 = value * 197 / 100;
    char string[TEXTLEN];
    sprintf(string, "amixer -D hw:1 -c 1 set 'Speaker' unmute %d", value2);
    system(string);
}

void set_line(int value)
{
    CLAMP(value, 0, 100);
    int value2 = value * 8065 / 100;
    char string[TEXTLEN];
    sprintf(string, "amixer -D hw:1 -c 1 set 'Line' unmute playback %d", value2);
    system(string);
}


void write_mixer()
{
    char string[TEXTLEN];
    set_speaker(speaker_volume);
    set_line(line_volume);
    
    sprintf(string, "amixer -D hw:1 -c 1 set 'PCM' unmute 65535");
    system(string);
    sprintf(string, "amixer -D hw:1 -c 1 set 'Mic' mute 0");
    system(string);
}

void* playback_thread(void *ptr)
{
    int i, j;
    int result;
// 	struct sched_param params;
// 	params.sched_priority = 1;
// 	if(sched_setscheduler(0, SCHED_RR, &params))
// 		perror("sched_setscheduler");


    while(1)
    {
        if(!dsp_out)
        {
//            printf("playback_thread %d: reopening\n", __LINE__);
            sleep(1);
            open_alsa();
            if(dsp_out)
            {
                write_mixer();
            }
            continue;
        }

// test for disconnect
        snd_pcm_status_t *status;
        snd_pcm_status_malloc(&status);
        snd_pcm_status(dsp_out, status);
        int state = snd_pcm_status_get_state(status);
        snd_pcm_status_free(status);

        if(state == 0)
        {
            printf("playback_thread %d: disconnected\n", __LINE__);
            close_alsa();
            continue;
        }

// wait for playback to free up a fragment
		int total_fds = snd_pcm_poll_descriptors_count(dsp_out);
		struct pollfd fds_out[total_fds];
		snd_pcm_poll_descriptors(dsp_out,
			fds_out,
			total_fds);
		result = poll(fds_out, total_fds, -1);
//printf("playback_thread %d total_fds=%d result=%d\n", __LINE__, total_fds, result);


		const snd_pcm_channel_area_t *areas_out;
		snd_pcm_uframes_t offset_out, frames_out;
		int avail_out = snd_pcm_avail_update(dsp_out);
		snd_pcm_mmap_begin(dsp_out,
			&areas_out, 
			&offset_out, 
			&frames_out);
//printf("playback_thread %d total_fds=%d result=%d\n", __LINE__, total_fds, result);
//printf("playback_thread %d avail_out=%d\n", __LINE__, avail_out);

        if(avail_out < 0 && devh)
        {
            printf("playback_thread %d: glitch\n", __LINE__);
            if(recording && total_written > 0)
            {
                glitches++;
            }
            close_alsa();
            open_alsa();
            continue;
        }
        
		int16_t *out_ptr0 = (int16_t*)((uint8_t*)areas_out[0].addr + offset_out * CHANNELS * 2);
        int16_t *out_ptr = out_ptr0;

#ifndef TEST_AUDIO
// must write 1 fragment at a time, regardless of avail_out
        int fifo_samples = fifo_size(&playback_fifo) / CHANNELS / sizeof(int);

// debug the FIFO
        if(0)
        {
            static int min_fifo_samples = 65535;
            static int max_fifo_samples = 0;
            static int debug_counter = 0;

            if(fifo_samples > max_fifo_samples)
            {
                max_fifo_samples = fifo_samples;
            }
            if(fifo_samples < min_fifo_samples) 
            {
                min_fifo_samples = fifo_samples;
            }
            debug_counter++;
            if(debug_counter >= 100)
            {
                debug_counter = 0;
                printf("playback_thread %d fifo_samples=%d min=%d max=%d\n", 
                    __LINE__, 
                    fifo_samples,
                    min_fifo_samples,
                    max_fifo_samples);
                min_fifo_samples = 65535;
                max_fifo_samples = 0;
            }
        }

// drop the ALSA write
        if(fifo_samples < MIN_FILL)
        {
            bzero(out_ptr, FRAGMENT * CHANNELS * sizeof(int16_t));

            if(!devh)
            {
                if(!usb_disconnected)
                {
                    printf("playback_thread %d instrument not connected\n", __LINE__);
                    usb_disconnected = 1;
                }
                sleep(1);
            }
            else
            {
//                 printf("playback_thread %d dropping ALSA fifo_samples=%d\n", __LINE__, fifo_samples);
            }
        }
        else
// drop the FIFO
        if(fifo_samples > MAX_FILL)
        {
            printf("playback_thread %d dropping FIFO fifo_samples=%d\n", __LINE__, fifo_samples);
            int drop = fifo_samples - MID_FILL;
            skip_fifo_bytes(&playback_fifo, (fifo_samples - MID_FILL) * CHANNELS * sizeof(int));
        }

// copy from the FIFO
        if(fifo_samples >= MIN_FILL)
        {
            int32_t *in_ptr = (int32_t*)(playback_fifo.data + playback_fifo.out_position);
            int32_t *fifo_end = (int32_t*)(playback_fifo.data + playback_fifo.allocated);
            if(fifo_samples > MID_FILL)
            {
// interpolate
                int32_t *in_ptr2 = in_ptr + 2;
                for(i = 0; i < FRAGMENT; i++)
                {
                    out_ptr[1] = (((*in_ptr++) >> 16) * (FRAGMENT - i) +
                        ((*in_ptr2++) >> 16) * i) / FRAGMENT;
                    out_ptr[0] = (((*in_ptr++) >> 16) * (FRAGMENT - i) +
                        ((*in_ptr2++) >> 16) * i) / FRAGMENT;
                    out_ptr += 2;
                    if(in_ptr >= fifo_end)
                    {
                        in_ptr = (int32_t*)playback_fifo.data;
                    }
                    if(in_ptr2 >= fifo_end)
                    {
                        in_ptr2 = (int32_t*)playback_fifo.data;
                    }
                }

	            pthread_mutex_lock(&playback_fifo.lock);
                playback_fifo.out_position = (uint8_t*)in_ptr2 - playback_fifo.data;
                playback_fifo.size -= (FRAGMENT + 1) * CHANNELS * sizeof(int);
	            pthread_mutex_unlock(&playback_fifo.lock);
            }
            else
            {
                for(i = 0; i < FRAGMENT; i++)
		        {
// swap channels for monitoring
                    out_ptr[1] = (*in_ptr++) >> 16;
                    out_ptr[0] = (*in_ptr++) >> 16;
                    out_ptr += 2;
                    if(in_ptr >= fifo_end)
                    {
                        in_ptr = (int32_t*)playback_fifo.data;
                    }
                }

	            pthread_mutex_lock(&playback_fifo.lock);
                playback_fifo.out_position = (uint8_t*)in_ptr - playback_fifo.data;
                playback_fifo.size -= FRAGMENT * CHANNELS * sizeof(int);
	            pthread_mutex_unlock(&playback_fifo.lock);
            }
        }

// DEBUG
//         out_ptr = out_ptr0;
//         for(i = 0; i < FRAGMENT; i++)
// 		{
//             *out_ptr++ = test_wave[test_offset++];
//             *out_ptr++ = test_wave[test_offset++];
//             if(test_offset >= TEST_SIZE * CHANNELS)
//             {
//                 test_offset = 0;
//             }
//         }



// add reverb
        if(do_reverb)
        {
// bandpass filter the latest fragment
            do_filter(out_ptr0);
// DEBUG
//bzero(out_ptr0, FRAGMENT * sizeof(int16_t) * CHANNELS);

            float *left_out;
            float *right_out;
// read waveform from lowpass filter & write to reverb buffers
            for(i = 0; i < reverb_refs; i++)
            {
                left_out = reverb_bufs[reverb_channels[0][i]];
                right_out = reverb_bufs[reverb_channels[1][i]];
                left_out += reverb_offsets[0][i];
                right_out += reverb_offsets[1][i];
                float left_level = reverb_levels[0][i];
                float right_level = reverb_levels[1][i];
                
                float *left_in = fft_dissolved[0] + fft_emptied;
                float *right_in = fft_dissolved[1] + fft_emptied;
                for(j = 0; j < FRAGMENT; j++)
                {
                    *left_out++ += *left_in++ * left_level;
                    *right_out++ += *right_in++ * right_level;
                }
            }
            fft_emptied += FRAGMENT;
            
// read from reverb buffers & write to ALSA
            left_out = reverb_bufs[0];
            right_out = reverb_bufs[1];
            out_ptr = out_ptr0;
            for(i = 0; i < FRAGMENT; i++)
		    {
                int left_ = (int)(*left_out++) + out_ptr[0];
                int right_ = (int)(*right_out++) + out_ptr[1];
// DEBUG
//                int left_ = (int)(*left_out++);
//                int right_ = (int)(*right_out++);
                
                CLAMP(left_, -32767, 32767);
                CLAMP(right_, -32767, 32767);
                
                *out_ptr++ = left_;
                *out_ptr++ = right_;
            }

// shift reverb bufs
            memcpy(reverb_bufs[0], reverb_bufs[0] + FRAGMENT, reverb_len * sizeof(int));
            memcpy(reverb_bufs[1], reverb_bufs[1] + FRAGMENT, reverb_len * sizeof(int));
            bzero(reverb_bufs[0] + reverb_len, FRAGMENT * sizeof(int));
            bzero(reverb_bufs[1] + reverb_len, FRAGMENT * sizeof(int));
        }


        if(do_metronome)
        {
// add metronome to ALSA buffer
            out_ptr = out_ptr0;
            int metro_samples = SAMPLERATE * 60 / bpm;

            if(metro_offset >= metro_samples)
            {
                metro_offset = 0;
            }
            for(i = 0; i < FRAGMENT; )
            {
                int fragment2 = FRAGMENT - i;
//printf("playback_thread %d %d %d\n", __LINE__, metro_wav_size, metro_offset);
                if(metro_offset >= metro_wav_size)
                {
                    if(metro_samples - metro_offset < fragment2)
                    {
                        fragment2 = metro_samples - metro_offset;
                    }
                    
                    // do nothing to output
                    out_ptr += fragment2;
                    metro_offset += fragment2;
                }
                else
                {
                    if(metro_wav_size - metro_offset < fragment2)
                    {
                        fragment2 = metro_wav_size - metro_offset;
                    }
                    
                    for(j = 0; j < fragment2; j++)
                    {
                        int mvalue = metronome_level * (((int)metro_wav_start[metro_offset]) - 0x80);
// left
                        int value = *out_ptr + mvalue;
                        CLAMP(value, -0x7fff, 0x7fff);
                        *out_ptr++ = value;
// right
                        value = *out_ptr + mvalue;
                        CLAMP(value, -0x7fff, 0x7fff);
                        *out_ptr++ = value;
                        
                        metro_offset++;
                    }
                }
                
                if(fragment2 <= 0)
                {
                    break;
                }
                
                i += fragment2;
                if(metro_offset >= metro_samples)
                {
                    metro_offset = 0;
                }
            }
        }


// static FILE *debug_fd = 0;
// if(debug_fd == 0)
// {
// debug_fd = fopen("test.pcm", "w");
// }
// fwrite(out_ptr0, FRAGMENT * CHANNELS * 2, 1, debug_fd);
// fflush(debug_fd);






#else // !TEST_AUDIO

		for(i = 0; i < FRAGMENT; i++)
		{
            *out_ptr++ = test_wave[test_offset++];
            *out_ptr++ = test_wave[test_offset++];
//            *out_ptr++ = 0;
//            *out_ptr++ = 0;
            if(test_offset >= TEST_SIZE * CHANNELS)
            {
                test_offset = 0;
            }
        }
#endif // TEST_AUDIO
//printf("playback_thread %d\n", __LINE__);

		snd_pcm_mmap_commit(dsp_out, offset_out, FRAGMENT);

    }
}

int open_usb()
{
    int result = 0;
    int i;
    if(usb_disconnected)
    {
    // open USB input
	    libusb_init(0);

	    devh = libusb_open_device_with_vid_pid(0, 0x04d8, 0x000b);
	    if(!devh)
	    {
//		    printf("open_usb: Couldn't find CP33\n");
		    return 1;
	    }

	    result = libusb_claim_interface(devh, 0);
	    if(result < 0)
	    {
		    printf("open_usb: CP33 in use\n");
            devh = 0;
		    return 1;
	    }


        if(devh)
        {
            usb_disconnected = 0;

            printf("open_usb %d: instrument connected\n", __LINE__);
	        out_buffer[0] = I2S_FRAGMENT_SIZE & 0xff;
	        out_buffer[1] = (I2S_FRAGMENT_SIZE >> 8) & 0xff;
	        out_buffer[2] = (I2S_FRAGMENT_SIZE >> 16) & 0xff;
	        out_buffer[3] = (I2S_FRAGMENT_SIZE >> 24) & 0xff;
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
		        libusb_fill_bulk_transfer(in_urb[i],
			        devh, 
			        1 | LIBUSB_ENDPOINT_IN,
			        in_buffer[i], 
			        I2S_FRAGMENT_SIZE, 
			        in_callback,
			        0, 
			        1000);
		        libusb_submit_transfer(in_urb[i]);
	        }
        }
    }

    return 0;
}

// detect last filename & last file size.  Causes recording overrun.
void calculate_dir()
{
// must not be writing a file
    if(write_fd < 0)
    {
        int highest = -1;
        char highest_filename[TEXTLEN];
        char string[TEXTLEN];
        sprintf(string, "ls -al %s*.wav", WAV_PATH);
        FILE *fd = popen(string, "r");
        
        while(!feof(fd))
        {
            char *result = fgets(string, TEXTLEN, fd);
            if(!result)
            {
                break;
            }

// get filename
            char *ptr = strchr(string, '/');
// get 1st numeric char
            if(ptr)
            {
                char *ptr2 = ptr;
                ptr++;
                while(*ptr && (*ptr < '0' || *ptr > '9'))
                {
                    ptr++;
                }
                
                if(ptr)
                {
                    int current = atoi(ptr);
                    if(current > highest)
                    {
                        highest = current;
                        strcpy(highest_filename, ptr2);
// strip linefeed
                        while(strlen(highest_filename) > 0 &&
                            highest_filename[strlen(highest_filename) - 1] == '\n')
                        {
                            highest_filename[strlen(highest_filename) - 1] = 0;
                        }
                    }
                }
            }
        }
		fclose(fd);

        if(highest >= 0)
        {
            struct stat64 ostat;
			stat64(highest_filename, &ostat);
        	pthread_mutex_lock(&www_mutex);
			total_written = (ostat.st_size - HEADER_SIZE) / SAMPLESIZE;
// printf("calculate_dir %d highest_filename=%s ostat.st_size=%d,%lld total_written=%lld\n",
// __LINE__,
// highest_filename,
// sizeof(ostat.st_size),
// ostat.st_size,
// total_written);
			strcpy(filename, highest_filename);
            next_file = highest + 1;
        	pthread_mutex_unlock(&www_mutex);
        }
        else
        {
        	pthread_mutex_lock(&www_mutex);
            next_file = 0;
            total_written = 0;
            filename[0] = 0;
        	pthread_mutex_unlock(&www_mutex);
        }
    }
}


// calculate remaneing space in samples.
int64_t calculate_remane()
{
	FILE *fd = popen("df /", "r");
	char buffer[TEXTLEN];
	fgets(buffer, TEXTLEN, fd);
	fgets(buffer, TEXTLEN, fd);
    fclose(fd);

	char *ptr = buffer;
	int i;
	for(i = 0; i < 3; i++)
	{
		while(*ptr != 0 && *ptr != ' ')
		{
			ptr++;
		}

		while(*ptr != 0 && *ptr == ' ')
		{
			ptr++;
		}
	}

	int64_t remane = atol(ptr);
//printf("calculate_remane %d remane=%lld\n", __LINE__, remane);
	remane = remane * 1024LL / SAMPLESIZE;
    return remane;
}


void flush_writer()
{
//    printf("flush_writer %d page_buffer=%p page_tmp=%p\n", 
//        __LINE__, page_buffer, page_tmp);
    
	pthread_mutex_lock(&record_fifo.lock);
	int size = record_fifo.size;
	pthread_mutex_unlock(&record_fifo.lock);
	
	int i = 0;
	while(i < size)
	{
		int fragment = size - i;
		if(record_fifo.out_position + fragment > record_fifo.allocated)
		{
			fragment = record_fifo.allocated - record_fifo.out_position;
		}
        
        if(fragment > PAGE_SIZE - page_used)
        {
            fragment = PAGE_SIZE - page_used;
        }

        memcpy(page_buffer + page_used, 
            record_fifo.data + record_fifo.out_position,
            fragment);

		record_fifo.out_position += fragment;
        page_used += fragment;
		if(record_fifo.out_position >= record_fifo.allocated)
		{
			record_fifo.out_position = 0;
		}
        if(page_used >= PAGE_SIZE)
        {
	        write(write_fd,
                page_buffer, 
                PAGE_SIZE);
            page_used = 0;
        }
        
		i += fragment;
	}


	pthread_mutex_lock(&record_fifo.lock);
	record_fifo.size -= size;
	pthread_mutex_unlock(&record_fifo.lock);

	pthread_mutex_lock(&www_mutex);
	total_written += size / SAMPLESIZE;
    total_remane -= size / SAMPLESIZE;
	pthread_mutex_unlock(&www_mutex);
}


void file_writer(void *ptr)
{
	while(1)
	{
        sem_wait(&write_sem);

		if(recording)
		{
			flush_writer();
		}

// stop a recording
		if(need_stop)
		{
			need_stop = 0;

			if(recording)
			{
				close(write_fd);
				write_fd = -1;
				pthread_mutex_lock(&www_mutex);
				recording = 0;
				total_remane = calculate_remane();
				pthread_mutex_unlock(&www_mutex);
			}
		}

// start a new recording
		if(need_recording)
		{
			need_recording = 0;
            glitches = 0;

			pthread_mutex_lock(&www_mutex);
// create a new file
			sprintf(filename, "%s%02d.wav", WAV_PATH, next_file);
			next_file++;
			pthread_mutex_unlock(&www_mutex);

			write_fd = open64(filename, 
                O_WRONLY | O_CREAT | O_DIRECT,
                0777);

			if(write_fd < 0)
			{
				printf("file_writer %d: couldn't open file %s\n", __LINE__, filename);
			}
			else
			{
				printf("file_writer %d: writing %s\n", __LINE__, filename);

// write the header
				write_header();
                flush_writer();

				pthread_mutex_lock(&www_mutex);
				recording = 1;
				total_written = 0;
				total_remane = calculate_remane();
				pthread_mutex_unlock(&www_mutex);

			}
		}
    }
}

void start_recording()
{
    if(!need_recording)
    {
    	need_recording = 1;
    	sem_post(&write_sem);
    }
}

void stop_recording()
{
    if(!need_stop)
    {
    	need_stop = 1;
	    sem_post(&write_sem);
    }
}

int parse_hex2(char *ptr)
{
    const char hex_table[] = { '0', '1', '2', '3', '4', '5', '6', '7', 
                               '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
    int i, j;
    int result = 0;
    for(i = 0; i < 2; i++)
    {
        for(j = 0; j < 16; j++)
        {
            if(hex_table[j] == tolower(ptr[i]))
            {
                result |= j;
                break;
            }
        }
        
        if(i < 1)
        {
            result <<= 4;
        }
    }
    
    return result;
}

void handle_input(char *state)
{
printf("handle_input %d state=%s\n", __LINE__, state);
    if(strlen(state) > 1)
    {
        int need_defaults = 0;
        uint8_t user_volume;
        uint8_t user_mvolume;
        uint8_t user_lvolume;
        uint8_t user_bpm;
        uint8_t user_preset1;
        uint8_t user_preset2;
        uint8_t user_preset3;
        uint8_t user_preset4;
        uint8_t user_metronome;
        uint8_t user_reverb;
        uint8_t user_record;

        user_volume = parse_hex2(state);
        state += 2;
        user_mvolume = parse_hex2(state);
        state += 2;
        user_lvolume = parse_hex2(state);
        state += 2;
        user_bpm = parse_hex2(state);
        CLAMP(user_bpm, 30, 200);
        state += 2;
        user_metronome = parse_hex2(state);
        state += 2;
        user_reverb = parse_hex2(state);
        state += 2;
        user_record = parse_hex2(state);
        state += 2;
        user_preset1 = parse_hex2(state);
        CLAMP(user_preset1, 30, 200);
        state += 2;
        user_preset2 = parse_hex2(state);
        CLAMP(user_preset2, 30, 200);
        state += 2;
        user_preset3 = parse_hex2(state);
        CLAMP(user_preset3, 30, 200);
        state += 2;
        user_preset4 = parse_hex2(state);
        CLAMP(user_preset4, 30, 200);
        state += 2;

printf("handle_input %d volume=%d mvolume=%d line=%d bpm=%d metronome=%d reverb=%d record=%d preset1=%d preset2=%d preset3=%d preset4=%d\n", 
__LINE__, 
user_volume,
user_mvolume,
user_lvolume,
user_bpm,
user_metronome,
user_reverb,
user_record,
user_preset1,
user_preset2,
user_preset3,
user_preset4);


        if(speaker_volume != user_volume)
        {
            need_defaults = 1;
            speaker_volume = user_volume;
            set_speaker(speaker_volume);
        }



        if(do_metronome != user_metronome)
        {
            need_defaults = 1;
            do_metronome = user_metronome;
            if(!do_metronome)
            {
                metro_offset = 0;
            }
        }
        
        if(line_volume != user_lvolume)
        {
            need_defaults = 1;
            line_volume = user_lvolume;
            set_line(line_volume);
        }

        if(preset1 != user_preset1 ||
            preset2 != user_preset2 ||
            preset3 != user_preset3 ||
            preset4 != user_preset4 ||
            metronome_level != user_mvolume ||
            bpm != user_bpm ||
            do_reverb != user_reverb)
        {
            need_defaults = 1;
            preset1 = user_preset1;
            preset2 = user_preset2;
            preset3 = user_preset3;
            preset4 = user_preset4;
            metronome_level = user_mvolume;
            bpm = user_bpm;
            do_reverb = user_reverb;
        }

        if(user_record && !recording)
        {
            start_recording();
        }
        else
        if(!user_record && recording)
        {
            stop_recording();
        }
        
        if(need_defaults)
        {
            save_settings();
        }
    }
}

// fill response to GUI
void handle_update(char *state)
{
//printf("handle_update %d total_written=%lld\n", __LINE__, total_written);
	sprintf(state, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%08x%08x%08x%s", 
        speaker_volume,
        metronome_level,
        line_volume,
        bpm,
        do_metronome,
        do_reverb,
        recording,
        preset1,
        preset2,
        preset3,
        preset4,
		(uint32_t)(total_written / SAMPLERATE), 
		(uint32_t)(total_remane / SAMPLERATE), 
        glitches,
		filename);
}

float from_db(float db)
{
    return pow(10, db / 20);
}

void init_reverb()
{
    int i, j;
    for(i = 0; i < CHANNELS; i++)
    {
        reverb_bufs[i] = calloc(sizeof(float), reverb_len + FRAGMENT);
        reverb_channels[i] = calloc(sizeof(int), reverb_refs);
        reverb_offsets[i] = calloc(sizeof(int), reverb_refs);
        reverb_levels[i] = calloc(sizeof(float), reverb_refs);
        
        reverb_channels[i][0] = i;
        reverb_offsets[i][0] = reverb_delay1;
        reverb_levels[i][0] = from_db(reverb_level1);
        for(j = 1; j < reverb_refs; j++)
        {
            reverb_channels[i][j] = rand() % CHANNELS;
            reverb_offsets[i][j] = reverb_delay1 + 
                (reverb_len - reverb_delay1) * j / reverb_refs +
                (rand() % (reverb_len - reverb_delay1) / reverb_refs);
            reverb_levels[i][j] = from_db(reverb_level1 + 
                (reverb_level2 - reverb_level1) * j / reverb_refs);
//printf("init_reverb %d i=%d j=%d level=%f\n", __LINE__, i, j, reverb_levels[i][j]);
        }
        
        fft_dissolved[i] = calloc(sizeof(float), FFT_WINDOW);
        fft_input_r[i] = calloc(sizeof(float), FFT_WINDOW);
        fft_output_r[i] = calloc(sizeof(float), FFT_WINDOW);
        fft_output_i[i] = calloc(sizeof(float), FFT_WINDOW);
        filtered_output_r[i] = calloc(sizeof(float), FFT_WINDOW);
        filtered_output_i[i] = calloc(sizeof(float), FFT_WINDOW);
//        fft_debug_r[i] = calloc(sizeof(float), FFT_WINDOW);
//        fft_debug_i[i] = calloc(sizeof(float), FFT_WINDOW);
    }
}


void load_settings()
{
    FILE *fd = fopen(SETTINGS_PATH, "r");
    if(fd)
    {
        char string[TEXTLEN];
        int bytes_read = fread(string, 1, TEXTLEN, fd);
        fclose(fd);
        string[bytes_read] = 0;
        printf("load_settings %d: %s\n", __LINE__, string);
        
        sscanf(string,
            "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
            &speaker_volume,
            &metronome_level,
            &line_volume,
            &bpm,
            &do_reverb,
            &do_metronome,
            &preset1,
            &preset2,
            &preset3,
            &preset4);
    }
    else
    {
        printf("load_settings %d: couldn't open %s\n", __LINE__, SETTINGS_PATH);
    }
}

void save_settings()
{
    FILE *fd = fopen(SETTINGS_PATH, "w");
    if(fd)
    {
        fprintf(fd, 
            "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",
            speaker_volume,
            metronome_level,
            line_volume,
            bpm,
            do_reverb,
            do_metronome,
            preset1,
            preset2,
            preset3,
            preset4);
        fclose(fd);
    }
    else
    {
        printf("save_settings %d: couldn't open %s\n", __LINE__, SETTINGS_PATH);
    }
}

int main(int argc, char *argv[])
{
    int result, i;
    load_settings();


	init_fifo(&playback_fifo, PLAYBACK_BUFFER);
	init_fifo(&record_fifo, RECORD_BUFFER);
    page_buffer = page_buffer_ + PAGE_SIZE - ((int)page_buffer_ % PAGE_SIZE);

// allocate USB buffers
	out_urb = libusb_alloc_transfer(0);
	out_buffer = calloc(1, EP1_SIZE);
	for(i = 0; i < TOTAL_URBS; i++)
	{
		in_urb[i] = libusb_alloc_transfer(0);
        in_buffer[i] = malloc(I2S_FRAGMENT_SIZE);
    }





// probe the last filename
#ifdef POLLING
    calculate_dir();
#else
	strcpy(filename, "");
	for(next_file = 0; next_file < MAXFILES; next_file++)
	{
		char next_filename[TEXTLEN];
		sprintf(next_filename, "%s%02d.wav", WAV_PATH, next_file);
		FILE *fd = fopen64(next_filename, "r");
		if(!fd)
		{
			break;
		}
		else
		{
			struct stat64 ostat;
			stat64(next_filename, &ostat);
			total_written = (ostat.st_size - HEADER_SIZE) / SAMPLESIZE;
			strcpy(filename, next_filename);
			fclose(fd);
		}
	}
#endif



	total_remane = calculate_remane();
    printf("main %d: next_file=%d total_written=%lld total_remane=%lld\n",
        __LINE__,
        next_file,
        total_written,
        total_remane);

	pthread_mutexattr_t attr2;
	pthread_mutexattr_init(&attr2);
	pthread_mutex_init(&write_mutex, &attr2);
	sem_init(&write_sem, 0, 0);

    init_server();

	pthread_t tid;
	pthread_attr_t  attr;
	pthread_attr_init(&attr);
	pthread_create(&tid, 
		&attr, 
		(void*)file_writer, 
		0);




    init_reverb();


	struct sched_param params;
	params.sched_priority = 1;
	if(sched_setscheduler(0, SCHED_RR, &params))
		perror("sched_setscheduler");

#ifdef TEST_AUDIO
// DEBUG
    for(i = 0; i < TEST_SIZE; i++)
    {
        test_wave[i * 2] = (int)(16384 * sin((float)i * 2 * M_PI / TEST_SIZE));
        test_wave[i * 2 + 1] = (int)(16384 * sin((float)i * 2 * M_PI / TEST_SIZE));
    }
#endif

// open audio output
    open_alsa();

    if(dsp_out)
    {
        write_mixer();
    }
	pthread_create(&tid, &attr, playback_thread, 0);

    if(open_usb())
    {
        printf("main %d: instrument disconnected\n", __LINE__);
    }

    while(1)
    {
        if(usb_disconnected)
        {
            open_usb();
        }

        while(!usb_disconnected)
        {
    		libusb_handle_events(0);
        }

// reset the library
        if(devh)
        {
            libusb_close(devh);
            devh = 0;
        }

// sleep without USB
        sleep(1);

// reread directory only when disconnected
#ifdef POLLING
        calculate_dir();
        total_remane = calculate_remane();
#endif
    }


}


















