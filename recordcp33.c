#include "settings.h"
#include "libusb.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>

#define TOTAL_URBS 16
#define EP1_SIZE 0x40
#define SAMPLERATE 44100
#define BUFFER_SIZE 0x10000

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
FILE *out;
int total_bytes = 0;
int total_errors = 0;
int usb_write_ready = 0;

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


void write_header()
{
	unsigned char header[44];
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
    header[offset++] = 2;
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
    header[offset++] = 6;
    header[offset++] = 0;
    // bits per sample
    header[offset++] = 24;
    header[offset++] = 0;
    header[offset++] = 'd';
    header[offset++] = 'a';
    header[offset++] = 't';
    header[offset++] = 'a';
    header[offset++] = 0xff;
    header[offset++] = 0xff;
    header[offset++] = 0xff;
    header[offset++] = 0x7f;
	if(!fwrite(header, sizeof(header), 1, out))
	{
		perror("write_header");
		exit(1);
	}
}


static void in_callback(struct libusb_transfer *transfer)
{
	if(transfer->status == LIBUSB_TRANSFER_COMPLETED)
	{
		int size = transfer->actual_length;
		total_bytes += size;

// pack into 3 bytes per sample
		int i;
		unsigned char *in_ptr = transfer->buffer;
		unsigned char *out_ptr = transfer->buffer;
		for(i = 0; i < size; i += 4)
		{
			out_ptr[0] = in_ptr[0];
			out_ptr[1] = in_ptr[1];
			out_ptr[2] = in_ptr[2];
			out_ptr += 3;
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



void* status_thread(void *ptr)
{
	while(1)
	{
		printf("Total bytes: %d total errors: %d\n", total_bytes, total_errors);
		sleep(1);
	}
}


void* writer_thread(void *ptr)
{
	unsigned char *buffer = malloc(BUFFER_SIZE);
	while(1)
	{
		wait_fifo(&fifo);
		int size = fifo_size(&fifo);
		
		while(size > 0)
		{
			int fragment = size;
			if(fragment > BUFFER_SIZE)
				fragment = BUFFER_SIZE;
			copy_fifo_buffer(&fifo, buffer, 0, fragment);
			skip_fifo_bytes(&fifo, fragment);
			fwrite(buffer, fragment, 1, out);
			size -= fragment;
		}
	}
}



int main(int argc, char *argv[])
{
	int result = 0;
	int i;
	
	
	if(argc < 2)
	{
		printf("Need a filename.\n");
		exit(1);
	}


	out = fopen(argv[1], "r");
	if(out)
	{
		printf("Won't overwrite file.\n");
		exit(1);
	}

	
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


	out = fopen(argv[1], "w");
	if(!out)
	{
		perror("Couldn't open file.");
		exit(1);
	}
	write_header();

	pthread_t tid;
	pthread_attr_t  attr;
	pthread_attr_init(&attr);
	pthread_create(&tid, &attr, status_thread, 0);


	init_fifo(&fifo, 0x400000);
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






