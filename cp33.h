#ifndef CP33_H
#define CP33_H


typedef struct
{
	unsigned char *buffer;
	int read_offset;
	int write_offset;
	int size;
	int total_bytes;
	int timer_high;
	int fragment_size;
} cp33_t;

extern cp33_t cp33;


#endif



