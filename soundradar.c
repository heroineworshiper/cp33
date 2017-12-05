// sound file radar



#include <stdio.h>
#include <math.h>
#include <string.h>
#include <png.h>

//#define USE_FMCW

#ifdef USE_FMCW
#define STACK_SIZE 16
#define WINDOW_SAMPLES 1024
#define DISPLAY_SAMPLES (WINDOW_SAMPLES / 2)
#else
#define STACK_SIZE 1
#define WINDOW_SAMPLES 16384
#define DISPLAY_SAMPLES 512
#endif


#define MAX_WINDOWS 1024

double sync_sample = 0;
double wave_sample = 0;
double fft_r_in[WINDOW_SAMPLES];
double fft_i_in[WINDOW_SAMPLES];
double fft_r_out[WINDOW_SAMPLES];
double fft_i_out[WINDOW_SAMPLES];
double fft_mag[WINDOW_SAMPLES];
int buffer_offset = 0;
int total_windows = 0;
int stack_total = 0;
unsigned char image[3 * DISPLAY_SAMPLES * MAX_WINDOWS];
unsigned char *rows[MAX_WINDOWS];


void (*adc_state)();
void wait_sync_lo();


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




int do_fft(unsigned int samples,  // must be a power of 2
    	int inverse,         // 0 = forward FFT, 1 = inverse
    	double *real_in,     // array of input's real samples
    	double *imag_in,     // array of input's imag samples
    	double *real_out,    // array of output's reals
    	double *imag_out)
{
    unsigned int num_bits;    // Number of bits needed to store indices
    unsigned int i, j, k, n;
    unsigned int block_size, block_end;

    double angle_numerator = 2.0 * M_PI;
    double tr, ti;     // temp real, temp imaginary

    if(inverse)
        angle_numerator = -angle_numerator;

    num_bits = samples_to_bits(samples);

// Do simultaneous data copy and bit-reversal ordering into outputs

    for(i = 0; i < samples; i++)
    {
        j = reverse_bits(i, num_bits);
        real_out[j] = real_in[i];
        imag_out[j] = (imag_in == 0) ? 0.0 : imag_in[i];
    }

// Do the FFT itself

    block_end = 1;
    double delta_angle;
    double sm2;
    double sm1;
    double cm2;
    double cm1;
    double w;
    double ar[3], ai[3];
    double temp;
    for(block_size = 2; block_size <= samples; block_size <<= 1)
    {
        delta_angle = angle_numerator / (double)block_size;
        sm2 = sin(-2 * delta_angle);
        sm1 = sin(-delta_angle);
        cm2 = cos(-2 * delta_angle);
        cm1 = cos(-delta_angle);
        w = 2 * cm1;

        for(i = 0; i < samples; i += block_size)
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
        double denom = (double)samples;

        for (i = 0; i < samples; i++)
        {
            real_out[i] /= denom;
            imag_out[i] /= denom;
        }
    }
	return 0;
}










void capture_waveform()
{
//	wave_sample = sin(buffer_offset * 2 * M_PI * 50 / 1024);


	fft_r_in[buffer_offset++] += wave_sample;
//	if(total_windows == 1) 
//	{
//		printf("%f\n", wave_sample);
//	}


	if(buffer_offset >= WINDOW_SAMPLES)
	{
		stack_total++;
		if(stack_total >= STACK_SIZE)
		{
			bzero(fft_i_in, sizeof(double) * WINDOW_SAMPLES);


			do_fft(WINDOW_SAMPLES,  // must be a power of 2
    			0,         // 0 = forward FFT, 1 = inverse
    			fft_r_in,     // array of input's real samples
    			fft_i_in,     // array of input's imag samples
    			fft_r_out,    // array of output's reals
    			fft_i_out);

			int i;
			double max = 0;
			for(i = 0; i < DISPLAY_SAMPLES; i++)
			{
				fft_mag[i] = sqrt(fft_r_out[i] * fft_r_out[i] +
					fft_i_out[i] * fft_i_out[i]);
				if(fft_mag[i] > max) max = fft_mag[i];
			}

			double min = 0;
			for(i = 0; i < DISPLAY_SAMPLES; i++)
			{
				double db = 5;
				if(fft_mag[i] > 0.0001)
					db = log10(fft_mag[i] / max);

				if(db < min) min = db;
				fft_mag[i] = db;
			}

//printf("%f\n", min);
			min = -3;
			unsigned char *row = rows[total_windows++];
			for(i = 0; i < DISPLAY_SAMPLES; i++)
			{
				double mag = 0;
				if(fft_mag[i] <= 0) mag = (fft_mag[i] - min) / (-min);
				int color = 255 * mag;
				if(color > 255) color = 255;
				if(color < 0) color = 0;
				row[i * 3 + 0] = color;
				row[i * 3 + 1] = color;
				row[i * 3 + 2] = color;
			}

			bzero(fft_r_in, sizeof(double) * WINDOW_SAMPLES);
			stack_total = 0;
		}

		buffer_offset = 0;
		adc_state = wait_sync_lo;
	}
}



void wait_sync_hi()
{
//	if(sync_sample > 0) 
	if(wave_sample > 0.5) 
	{
		adc_state = capture_waveform;
		buffer_offset = 0;
	}
}



void wait_sync_lo()
{
//	if(sync_sample < 0) adc_state = wait_sync_hi;
	if(wave_sample < 0.5) adc_state = wait_sync_hi;
#ifndef USE_FMCW
	adc_state = capture_waveform;
#endif
}



int main(int argc, char *argv[])
{
	if(argc < 2) 
	{
		printf("main: need a filename\n");
		return 1;
	}
	
	FILE *in = fopen(argv[1], "r");
	if(!in)
	{
		printf("main: couldn't open %s\n", argv[1]);
		return 1;
	}


	int i;
	for(i = 0; i < MAX_WINDOWS; i++)
	{
		rows[i] = image + 3 * DISPLAY_SAMPLES * i;
	}
	

	
	adc_state = wait_sync_lo;
	fseek(in, 0x394, SEEK_SET);
	unsigned char buffer[6];
	do
	{
		fread(buffer, 6, 1, in);

		int temp = (buffer[0] |
			(buffer[1] << 8) |
			(buffer[2] << 16));
		if(temp & 0x800000) temp -= 0x1000000;
		wave_sample = (double)temp / 0x7fffff;

		temp = (buffer[3] |
			(buffer[4] << 8) |
			(buffer[5] << 16));
		if(temp & 0x800000) temp -= 0x1000000;
		sync_sample = (double)temp / 0x7fffff;


/*
 * static int samples = 0;
 * if(samples < 4000)
 * {
 * samples++;
 * printf("%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d\n",
 * (buffer[5] & 0x80) ? 1 : 0,
 * (buffer[5] & 0x40) ? 1 : 0,
 * (buffer[5] & 0x20) ? 1 : 0,
 * (buffer[5] & 0x10) ? 1 : 0,
 * (buffer[5] & 0x8) ? 1 : 0,
 * (buffer[5] & 0x4) ? 1 : 0,
 * (buffer[5] & 0x2) ? 1 : 0,
 * (buffer[5] & 0x1) ? 1 : 0,
 * (buffer[4] & 0x80) ? 1 : 0,
 * (buffer[4] & 0x40) ? 1 : 0,
 * (buffer[4] & 0x20) ? 1 : 0,
 * (buffer[4] & 0x10) ? 1 : 0,
 * (buffer[4] & 0x8) ? 1 : 0,
 * (buffer[4] & 0x4) ? 1 : 0,
 * (buffer[4] & 0x2) ? 1 : 0,
 * (buffer[4] & 0x1) ? 1 : 0,
 * (buffer[3] & 0x80) ? 1 : 0,
 * (buffer[3] & 0x40) ? 1 : 0,
 * (buffer[3] & 0x20) ? 1 : 0,
 * (buffer[3] & 0x10) ? 1 : 0,
 * (buffer[3] & 0x8) ? 1 : 0,
 * (buffer[3] & 0x4) ? 1 : 0,
 * (buffer[3] & 0x2) ? 1 : 0,
 * (buffer[3] & 0x1) ? 1 : 0);
 * }
 */


		adc_state();
	} while(!feof(in) && total_windows < MAX_WINDOWS);
	
	
	printf("total_windows=%d\n", total_windows);
	
	png_structp png_ptr;
	png_infop info_ptr;
	png_infop end_info = 0;	
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	info_ptr = png_create_info_struct(png_ptr);
	FILE *out_fd = fopen("spectrum.png", "w");
	png_init_io(png_ptr, out_fd);
	png_set_compression_level(png_ptr, 9);
	png_set_IHDR(png_ptr, 
		info_ptr, 
		DISPLAY_SAMPLES, 
		total_windows,
    	8, 
		PNG_COLOR_TYPE_RGB, 
		PNG_INTERLACE_NONE, 
		PNG_COMPRESSION_TYPE_DEFAULT, 
		PNG_FILTER_TYPE_DEFAULT);
	png_write_info(png_ptr, info_ptr);
	png_write_image(png_ptr, rows);
	png_write_end(png_ptr, info_ptr);
	png_destroy_write_struct(&png_ptr, &info_ptr);
	fclose(out_fd);

	
	
	
	return 0;
}



