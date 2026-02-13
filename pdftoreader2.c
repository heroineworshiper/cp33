/*
 * MUSIC READER
 * Copyright (C) 2021-2025 Adam Williams <broadcast at earthling dot net>
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




// convert a PDF or other image file to a baked image stack for reader.C
// gcc -O3 -g -o pdftoreader2 pdftoreader2.c -lpng -ljpeg




#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <png.h>
#include "jpeglib.h"
#include <setjmp.h>


#define DISPLAY_W 768
#define DISPLAY_H 1366

//#define DISPLAY_W 1080
//#define DISPLAY_H 1920

#define TEXTLEN 1024
#define DPI 300
// default greyscale value for white
#define THRESHOLD 128

int page1 = 1;
int page2 = 9999;
char **sources = 0;
int total_sources = 0;
// dest without the .gz
char *dest = 0;
// dest with the .gz
char *dest2 = 0;
int crop_x = 0;
int crop_y = 0;
int crop_w = 9999;
int crop_h = 9999;
int crop_w2, crop_h2;
int autocrop = 0;
int dpi = DPI;
int threshold = THRESHOLD;
int grey = 0; // greyscale
FILE *dest_fd;

char* get_suffix(char *dst, char *path)
{
    char *ptr = path + strlen(path) - 1;
    *dst = 0;
    while(ptr > path && *ptr != '.')
    {
        ptr--;
    }
    if(ptr == path) return 0;
// skip the .
    ptr++;
    char *ptr2 = dst;
    while(*ptr != '\"' && *ptr != 0)
        *ptr2++ = *ptr++;
    *ptr2 = 0;
    return dst;
}

char* skipwhite(char *ptr)
{
    while(*ptr != 0 && (*ptr == ' ' || *ptr == '\t' || *ptr == '\n'))
        ptr++;
    
    return ptr;
}

// convert a space separated string to an array of ints
void texttoarray(int *dst, int *len, char *text)
{
    char *ptr = text;
    char *end = text + strlen(text);
    int i = 0;
//    printf("texttoarray %d: input: %s", __LINE__, text);
    while(ptr < end)
    {
        char *ptr2 = ptr;
        while(*ptr2 != 0 && *ptr2 != ' ')
        {
            ptr2++;
        }

        *ptr2 = 0;
        dst[i] = atoi(ptr);
        i++;
        ptr = ptr2 + 1;
    }
    *len = i;

//     printf("Values: ");
//     for(i = 0; i < *len; i++)
//     {
//         printf("%d ", dst[i]);
//     }
//     printf("\n");
}

// convert an x,y string to x & y
void argtoxy(int *x, int *y, char *arg)
{
    *x = 0;
    *y = 0;
    char *ptr = strchr(arg, ',');
    if(!ptr)
    {
        ptr = strchr(arg, 'x');
    }
    if(ptr)
    {
        *ptr = 0;
        ptr++;
    }
    else
    {
        printf("argtoxy %d: malformed coordinate %s\n", __LINE__, arg);
        return;
    }
    *x = atoi(arg);
    *y = atoi(ptr);
}


void process_frame(uint8_t *src, int src_w, int src_h)
{
// convert to 8 bits per pixel in screen resolution
// each output row is divided into 3 for virtual RGB rows, giving
// an output height 3x higher than the number of rows
    int y_table[DISPLAY_H * 3 + 1];
    int x_table[DISPLAY_W + 1];
// extra space for 16 bit
    uint8_t dst_row[DISPLAY_W * 2];
    uint16_t *dst_row16 = (uint16_t*)dst_row;
    int row;
    int subrow;
    int col;
    int i, j;

// search for black area.  Doesn't work because source always has noise
    if(autocrop)
    {
        crop_x = 0;
        crop_y = 0;
        crop_w2 = src_w;
        crop_h2 = src_h;

        int count = 0;
        int min_x_count = src_w / 100;
        int min_y_count = src_h / 100;
        for(i = 0; i < src_h; i++)
        {
            count = 0;
            uint8_t *row = src + i * src_w;
            for(j = 0; j < src_w; j++)
            {
                if(row[j] < THRESHOLD)
                {
                    count++;
                    if(count >= min_x_count)
                    {
                        crop_y = i;
                        crop_h2 -= i;
                        i = src_h;
                        break;
                    }
                }
            }
        }

        for(i = src_h - 1; i >= 0; i--)
        {
            count = 0;
            uint8_t *row = src + i * src_w;
            for(j = 0; j < src_w; j++)
            {
                if(row[j] < THRESHOLD)
                {
                    count++;
                    if(count >= min_x_count)
                    {
                        crop_h2 = i - crop_y;
                        i = 0;
                        break;
                    }
                }
            }
        }

        for(i = 0; i < src_w; i++)
        {
            count = 0;
            for(j = 0; j < src_h; j++)
            {
                if(src[j * src_w + i] < THRESHOLD)
                {
                    count++;
                    if(count >= min_y_count)
                    {
                        crop_x = i;
                        crop_w2 -= i;
                        i = src_w;
                        break;
                    }
                }
            }
        }

        for(i = src_w - 1; i >= 0; i--)
        {
            count = 0;
            for(j = 0; j < src_h; j++)
            {
                if(src[j * src_w + i] < THRESHOLD)
                {
                    count++;
                    if(count >= min_y_count)
                    {
                        crop_w2 = i - crop_x;
                        i = 0;
                        break;
                    }
                }
            }
        }
printf("autocrop: x=%d y=%d w=%d h=%d\n", crop_x, crop_y, crop_w2, crop_h2);
    }

    for(i = 0; i < DISPLAY_H * 3 + 1; i++)
    {
        y_table[i] = crop_y + i * crop_h2 / (DISPLAY_H * 3);
        if(y_table[i] >= crop_y + crop_h2)
        {
            y_table[i] = crop_y + crop_h2 - 1;
        }
    }

    for(i = 0; i < DISPLAY_W + 1; i++)
    {
// assume the source R is the same as G & B
        x_table[i] = crop_x + i * crop_w2 / DISPLAY_W;
        if(x_table[i] >= crop_x + crop_w2)
        {
            x_table[i] = crop_x + crop_w2 - 1;
        }
//printf("out_x=%d in_x=%d\n", i, x_table[i]);
    }

    for(row = 0; row < DISPLAY_H; row++)
    {
        bzero(dst_row, DISPLAY_W * 2);
// 3 source rows become each destination row
        for(subrow = 0; subrow < 3; subrow++)
        {
            int src_row0 = y_table[row * 3 + subrow];
            int src_row1 = y_table[row * 3 + subrow + 1];
            if(src_row1 == src_row0)
            {
                src_row1++;
            }
//printf("row=%d src_rows=%d-%d\n", row, src_row0, src_row1);
            const uint8_t components[] = { 1, 2, 4 };
            uint8_t component = components[subrow];
            
            for(col = 0; col < DISPLAY_W; col++)
            {
                int src_col0 = x_table[col];
                int src_col1 = x_table[col + 1];
                uint32_t value;
                if(grey) 
                    value = 0;
                else
                    value = 0xff;
                if(src_col1 == src_col0)
                {
                    src_col1++;
                }
                
// get darkest pixel or average of rectangle
                for(i = src_row0; i < src_row1; i++)
                {
                    for(j = src_col0; j < src_col1; j++)
                    {
                        uint8_t new_value = *(src + src_w * i + j);
                        if(grey) 
                            value += new_value;
                        else
                            if(new_value < value)
                            {
                                value = new_value;
                            }
                    }
                }

                if(grey)
                {
                    value /= (src_row1 - src_row0) * (src_col1 - src_col0);
                    switch(subrow)
                    {
                        case 0:
                            dst_row16[col] |= (((value * 0b1111100000000000) / 255) & 0b1111100000000000);
                            break;
                        case 1:
                            dst_row16[col] |= (((value * 0b11111100000) / 255) & 0b11111100000);
                            break;
                        case 2:
                            dst_row16[col] |= (((value * 0b11111) / 255) & 0b11111);
                            break;
                    }
                }
                else
                if(value > threshold)
                {
                    dst_row[col] |= component;
                }
            }
        }

        if(grey)
            fwrite(dst_row, 1, DISPLAY_W * 2, dest_fd);
        else
            fwrite(dst_row, 1, DISPLAY_W, dest_fd);
    }
}


uint8_t *read_ptr = 0;
uint8_t *read_end = 0;
void png_read_function(png_structp png_ptr,
    png_bytep data, 
    png_size_t length)
{
	if(read_ptr + length > read_end)
	{
		printf("read_function %d: overrun\n", __LINE__);
		length = read_end - read_ptr;
	}

	memcpy(data, read_ptr, length);
	read_ptr += length;
};

uint8_t* read_source(char *path, int *size_return)
{
    FILE *fd = fopen(path, "r");
    if(!fd)
    {
        printf("Couldn't open source %s\n", path);
        exit(1);
    }
    fseek(fd, 0, SEEK_END);
    int size = ftell(fd);
    fseek(fd, 0, SEEK_SET);
    uint8_t *buffer = malloc(size);
    int _ = fread(buffer, 1, size, fd);
    fclose(fd);
    *size_return = size;
    return buffer;
}


typedef struct 
{
	struct jpeg_source_mgr pub;	/* public fields */

	JOCTET * buffer;		/* start of buffer */
	int bytes;             /* total size of buffer */
} jpeg_source_mgr_t;
typedef jpeg_source_mgr_t* jpeg_src_ptr;


struct my_jpeg_error_mgr {
  struct jpeg_error_mgr pub;	/* "public" fields */
  jmp_buf setjmp_buffer;	/* for return to caller */
};

typedef struct my_jpeg_error_mgr* my_jpeg_error_ptr;
struct my_jpeg_error_mgr my_jpeg_error;

METHODDEF(void) init_source(j_decompress_ptr cinfo)
{
    jpeg_src_ptr src = (jpeg_src_ptr) cinfo->src;
}

METHODDEF(boolean) fill_input_buffer(j_decompress_ptr cinfo)
{
	jpeg_src_ptr src = (jpeg_src_ptr) cinfo->src;
#define   M_EOI     0xd9

	src->buffer[0] = (JOCTET)0xFF;
	src->buffer[1] = (JOCTET)M_EOI;
	src->pub.next_input_byte = src->buffer;
	src->pub.bytes_in_buffer = 2;

	return TRUE;
}


METHODDEF(void) skip_input_data(j_decompress_ptr cinfo, long num_bytes)
{
	jpeg_src_ptr src = (jpeg_src_ptr)cinfo->src;

	src->pub.next_input_byte += (size_t)num_bytes;
	src->pub.bytes_in_buffer -= (size_t)num_bytes;
}


METHODDEF(void) term_source(j_decompress_ptr cinfo)
{
}

METHODDEF(void) my_jpeg_output (j_common_ptr cinfo)
{
}


METHODDEF(void) my_jpeg_error_exit (j_common_ptr cinfo)
{
/* cinfo->err really points to a mjpeg_error_mgr struct, so coerce pointer */
  	my_jpeg_error_ptr mjpegerr = (my_jpeg_error_ptr) cinfo->err;

printf("my_jpeg_error_exit %d\n", __LINE__);
/* Always display the message. */
/* We could postpone this until after returning, if we chose. */
  	(*cinfo->err->output_message) (cinfo);

/* Return control to the setjmp point */
  	longjmp(mjpegerr->setjmp_buffer, 1);
}

int main(int argc, char *argv[])
{
    if(argc < 2)
    {
        char *progname = argv[0];
        printf("Usage: %s <config file>\n", progname);
        printf("The config file contains commands:\n");
        printf("# comment line\n");
        printf("GREY enable greyscale mode\n");
        printf("SRC <source file> this appears multiple times if individual image files\n");
        printf("DST <dest file> this appears once.  If it ends in .gz, it's compressed\n");
        printf("DPI <source dpi if PDF> default %d\n", DPI);
        printf("CROP <x y w h> cropping dimensions.  W & H are truncated to the source.\n");
        printf("THRESHOLD <0-255> greyscale value for white.  default %d\n", THRESHOLD);
        printf("    Only used for cropping in greyscale mode.\n");
        printf("    Pixels are averaged in greyscale mode.\n");
        printf("PAGES <start & end page if PDF> starting page starts at 1.  Ending page is inclusive & truncated to the source pages.\n");
        printf("\nExample config file:\n");
        printf("SRC /home/archive/scherzo4.pdf\n");
        printf("DST /home/archive/scherzo4.reader.gz\n");
        printf("PAGES 1 9999\n");
        printf("CROP 135 0 2294 3567\n");
        exit(1);
    }


    printf("DISPLAY_W=%d DISPLAY_H=%d\n", DISPLAY_W, DISPLAY_H);
    char string[TEXTLEN];
    FILE *config_fd = fopen(argv[1], "r");
    if(!config_fd)
    {
        printf("Couldn't open config file %s\n", argv[1]);
        exit(1);
    }

    while(!feof(config_fd))
    {
        char *line = fgets(string, TEXTLEN, config_fd);
        if(!line || line[0] == 0) break;
        char *ptr = skipwhite(line);
// comment line or empty
        if(*ptr == '#' || *ptr == 0) continue;

// get the command
        char command[TEXTLEN];
        char *ptr2 = command;
        while(*ptr != 0 && *ptr != ' ' && *ptr != '\t' && *ptr != '\n')
            *ptr2++ = *ptr++;
        *ptr2 = 0;

        ptr = skipwhite(ptr);

// get the arguments
        char args[TEXTLEN];
        ptr2 = args;
        while(*ptr != 0 && *ptr != '\n')
            *ptr2++ = *ptr++;
        *ptr2 = 0;
        
//        printf("command=%s args=%s\n", command, args);
        if(!strcasecmp(command, "GREY"))
        {
            grey = 1;
        }
        else
        if(!strcasecmp(command, "SRC"))
        {
            sources = realloc(sources, (total_sources + 1) * sizeof(char*));
            sources[total_sources] = strdup(args);
            total_sources++;
        }
        else
        if(!strcasecmp(command, "DST"))
        {
            dest = strdup(args);
        }
        else
        if(!strcasecmp(command, "DPI"))
        {
            dpi = atoi(args);
        }
        else
        if(!strcasecmp(command, "CROP"))
        {
            int dst[4];
            int len;
            texttoarray(dst, &len, args);
            crop_x = dst[0];
            crop_y = dst[1];
            crop_w = dst[2];
            crop_h = dst[3];
            if(crop_x < 0) autocrop = 1;
        }
        else
        if(!strcasecmp(command, "PAGES"))
        {
            int dst[2];
            int len;
            texttoarray(dst, &len, args);
            page1 = dst[0];
            page2 = dst[1];
        }
        else
        if(!strcasecmp(command, "THRESHOLD"))
        {
            threshold = atoi(args);
        }
    }
    fclose(config_fd);
    
    int i, j;
    printf("Sources:\n");
    for(i = 0; i < total_sources; i++)
    {
        printf("\t%s\n", sources[i]);
    }
    printf("Page 1: %d\n", page1);
    printf("Page 2: %d\n", page2);
    if(autocrop)
        printf("AUTO CROP\n");
    else
        printf("Crop: %d,%d %dx%d\n", crop_x, crop_y, crop_w, crop_h);
    printf("Dest: %s\n", dest);
    printf("DPI: %d\n", dpi);
    printf("Threshold: %d\n", threshold);
    crop_w2 = crop_w;
    crop_h2 = crop_h;

    if(!strcasecmp(dest + strlen(dest) - 3, ".gz"))
    {
        dest2 = strdup(dest);
        dest[strlen(dest) - 3] = 0;
    }

    dest_fd = fopen(dest, "w");
    if(!dest_fd)
    {
        printf("Couldn't open %s for writing.\n", dest);
        exit(1);
    }
    int display_w = DISPLAY_W;
    int display_h = DISPLAY_H;
    int temp = 0;
    if(grey)
    {
// some simple header
        uint8_t header[] = { 
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
            'R',  'E',  'A',  'D',  'E',  'R',  'G',  'R', 
            'E',  'Y',  0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        };
        fwrite(header, 1, sizeof(header), dest_fd);
    }
    fwrite(&display_w, 1, 4, dest_fd);
    fwrite(&display_h, 1, 4, dest_fd);
// placeholder for number of pages
    int page_offset = ftell(dest_fd);
    fwrite(&temp, 1, 4, dest_fd);


    int current_page = 1;
    int total_pages = 0;
    char suffix[TEXTLEN];
    get_suffix(suffix, sources[0]);
// open PDF
    if(!strcasecmp(suffix, "pdf"))
    {
        sprintf(string, 
            "gs -dBATCH -dNOPAUSE -sDEVICE=pnm -o - -r%d %s", 
            dpi, 
            sources[0]);

// escape the special characters
        char *ptr = string;
        while(*ptr)
        {
            if(*ptr == '(' ||
                *ptr == ')')
            {
                int len = strlen(string);
                for(i = len + 1; i > ptr - string; i--)
                {
                    string[i] = string[i - 1];
                }
                *ptr = '\\';
                ptr++;
            }
            ptr++;
        }

        printf("Running %s\n", string);
        FILE *fd = popen(string, "r");
        if(!fd)
        {
            printf("Couldn't run gs command.\n");
            exit(1);
        }

        int line = 0;
        int temp[TEXTLEN];
        int src_w = 0;
        int src_h = 0;
        int len = 0;
        int buflen = 0;
        uint8_t *buffer = 0;
        int done = 0;
        while(!feof(fd) && !done)
        {
            if(!fgets(string, TEXTLEN, fd)) 
            {
                printf("main %d\n", __LINE__);
                break;
            }

    // comment line
            if(string[0] == '#')
            {
                continue;
            }

    // PPM image
            switch(line)
            {
                case 0:
                    if(!strncmp(string, "P3", 2))
                    {
                        line++;
                        printf("reading page %d\n", current_page);
                    }
                    break;

                case 1:
                    texttoarray(temp, &len, string);
                    line++;
                    src_w = temp[0];
                    src_h = temp[1];
                    printf("main %d: src_w=%d src_h=%d\n", __LINE__, src_w, src_h);
                    crop_w2 = crop_w;
                    crop_h2 = crop_h;
                    if(src_w < crop_x + crop_w2)
                    {
                        crop_w2 = src_w - crop_x;
                        printf("main %d: truncating crop_w to %d\n", __LINE__, crop_w2);
                    }

                    if(src_h < crop_y + crop_h2)
                    {
                        crop_h2 = src_h - crop_y;
                        printf("main %d: truncating crop_h to %d\n", __LINE__, crop_h2);
                    }
                    buffer = (uint8_t*)malloc(src_w * src_h * 3 + 
                        sizeof(temp) / sizeof(int));
                    break;

                case 2:
    // ignore maximum color value
                    line++;
                    break;

                default:
    // RGB triplets
                    texttoarray(temp, &len, string);
                    if(len > sizeof(temp) / sizeof(int))
                    {
                        printf("main %d: temp overflowed len=%d\n", __LINE__, len);
                    }
                    for(i = 0; i < len; i++)
                    {
                        buffer[buflen++] = temp[i];
                    }

                    if(buflen >= src_w * src_h * 3)
                    {
                        if(current_page >= page1 && current_page <= page2)
                        {
// RGB -> greyscale
                            uint8_t *src_ptr = buffer;
                            uint8_t *dst_ptr = buffer;
                            for(j = 0; j < src_w * src_h; j++)
                            {
                                *dst_ptr++ = *src_ptr;
                                src_ptr += 3;
                            }
                            process_frame(buffer, src_w, src_h);
                            total_pages++;
                        }


                        line = 0;
                        buflen = 0;
                        free(buffer);
                        current_page++;
                        if(current_page > page2)
                        {
                            printf("main %d: Done\n", __LINE__);
                            done = 1;
                        }
                    }
                    break;
            }
        }

//        printf("main %d: feof(fd)=%d done=%d\n", __LINE__, feof(fd), done);


        fclose(fd);
    }
    else
    if(!strcasecmp(suffix, "png"))
    {
        for(i = 0; i < total_sources; i++)
        {
            printf("Reading page %s\n", sources[i]);
            int size;
            uint8_t *buffer = read_source(sources[i], &size);
            uint8_t *frame_buffer = 0;
            uint8_t **rows = 0;
            uint8_t *src_ptr;
            uint8_t *dst_ptr;

            read_ptr = buffer;
            read_end = buffer + size;
		    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
		    png_infop info_ptr = png_create_info_struct(png_ptr);
		    png_set_read_fn(png_ptr, 0, png_read_function);
		    png_read_info(png_ptr, info_ptr);

            int src_w = png_get_image_width(png_ptr, info_ptr);
            int src_h = png_get_image_height(png_ptr, info_ptr);
            rows = malloc(sizeof(uint8_t*) * src_h);
            
            printf("main %d: src_w=%d src_h=%d\n", __LINE__, src_w, src_h);
            crop_w2 = crop_w;
            crop_h2 = crop_h;
            if(src_w < crop_x + crop_w2)
            {
                crop_w2 = src_w - crop_x;
                printf("main %d: truncating crop_w to %d\n", __LINE__, crop_w2);
            }

            if(src_h < crop_y + crop_h2)
            {
                crop_h2 = src_h - crop_y;
                printf("main %d: truncating crop_h to %d\n", __LINE__, crop_h2);
            }
            
            switch(png_get_color_type(png_ptr, info_ptr))
            {
                case PNG_COLOR_TYPE_RGB:
                    printf("main %d: RGB\n", __LINE__);
                    frame_buffer = (uint8_t*)malloc(src_w * src_h * 3);
                    
                    for(j = 0; j < src_h; j++)
                    {
                        rows[j] = frame_buffer + j * src_w * 3;
                    }
                    png_read_image(png_ptr, rows);

// RGB -> greyscale
                    src_ptr = frame_buffer;
                    dst_ptr = frame_buffer;
                    for(j = 0; j < src_w * src_h; j++)
                    {
                        *dst_ptr++ = *src_ptr;
                        src_ptr += 3;
                    }

                    process_frame(frame_buffer, src_w, src_h);
                    break;
                
                case PNG_COLOR_TYPE_GRAY_ALPHA:
                case PNG_COLOR_TYPE_RGB_ALPHA:
                    printf("main %d: RGBA\n", __LINE__);
                    frame_buffer = (uint8_t*)malloc(src_w * src_h * 4);
                    
                    for(j = 0; j < src_h; j++)
                    {
                        rows[j] = frame_buffer + j * src_w * 4;
                    }
                    png_read_image(png_ptr, rows);
                    
// RGBA -> greyscale
                    src_ptr = frame_buffer;
                    dst_ptr = frame_buffer;
                    for(j = 0; j < src_w * src_h; j++)
                    {
                        *dst_ptr++ = *src_ptr;
                        src_ptr += 4;
                    }

                    process_frame(frame_buffer, src_w, src_h);
                    break;
                
                case PNG_COLOR_TYPE_PALETTE:
                {
                    png_set_palette_to_rgb(png_ptr);
	                if (png_get_bit_depth(png_ptr, info_ptr) <= 8)
                        png_set_expand(png_ptr);
                    frame_buffer = (uint8_t*)malloc(src_w * src_h * 3);
                    for(j = 0; j < src_h; j++)
                    {
                        rows[j] = frame_buffer + j * src_w * 3;
                    }
                    png_read_image(png_ptr, rows);

                    src_ptr = frame_buffer;
                    dst_ptr = frame_buffer;
                    for(j = 0; j < src_w * src_h; j++)
                    {
                        *dst_ptr++ = *src_ptr;
                        src_ptr += 3;
                    }
                    process_frame(frame_buffer, src_w, src_h);
                    break;
                }
                
                default:
                    printf("main %d: unknown color space %d\n", 
                        __LINE__, 
                        png_get_color_type(png_ptr, info_ptr));
                    break;
            }
            
            
            free(buffer);
            free(frame_buffer);
            free(rows);
            current_page++;
            total_pages++;
        }
    }
    else
    if(!strcasecmp(suffix, "jpg"))
    {
        for(i = 0; i < total_sources; i++)
        {
            printf("Reading page %s\n", sources[i]);
            int size;
            uint8_t *buffer = read_source(sources[i], &size);
            uint8_t *frame_buffer = 0;
            uint8_t **rows = 0;
            uint8_t *src_ptr;
            uint8_t *dst_ptr;

	        struct jpeg_decompress_struct cinfo;
	        struct my_jpeg_error_mgr jerr;
	        cinfo.err = jpeg_std_error(&(my_jpeg_error.pub));
            my_jpeg_error.pub.error_exit = my_jpeg_error_exit;
            my_jpeg_error.pub.output_message = my_jpeg_output;
	        jpeg_create_decompress(&cinfo);
	        cinfo.src = (struct jpeg_source_mgr*)
    	        (*cinfo.mem->alloc_small)((j_common_ptr)&cinfo, 
                JPOOL_PERMANENT,
		        sizeof(jpeg_source_mgr_t));
	        jpeg_src_ptr src = (jpeg_src_ptr)cinfo.src;
	        src->pub.init_source = init_source;
	        src->pub.fill_input_buffer = fill_input_buffer;
	        src->pub.skip_input_data = skip_input_data;
	        src->pub.resync_to_restart = jpeg_resync_to_restart; /* use default method */
	        src->pub.term_source = term_source;
	        src->pub.bytes_in_buffer = size;
	        src->pub.next_input_byte = buffer;
	        src->buffer = buffer;
	        src->bytes = size;
	        jpeg_read_header(&cinfo, 1);
	        jpeg_start_decompress(&cinfo);

            int src_w = cinfo.output_width;
            int src_h = cinfo.output_height;
            
            
            if(cinfo.jpeg_color_space == JCS_RGB)
            {
                frame_buffer = (uint8_t*)malloc(src_w * src_h * 3);
                rows = malloc(src_h * sizeof(uint8_t*));
                for(j = 0; j < src_h; j++)
                {
                    rows[j] = frame_buffer + src_w * j * 3;
                }
            }
            else
            if(cinfo.jpeg_color_space == JCS_GRAYSCALE)
            {
                frame_buffer = (uint8_t*)malloc(src_w * src_h);
                rows = malloc(src_h * sizeof(uint8_t*));
                for(j = 0; j < src_h; j++)
                {
                    rows[j] = frame_buffer + src_w * j;
                }
            }
            else
            if(cinfo.jpeg_color_space == JCS_YCbCr)
            {
                frame_buffer = (uint8_t*)malloc(src_w * src_h * 3);
                rows = malloc(src_h * sizeof(uint8_t*));
                for(j = 0; j < src_h; j++)
                {
                    rows[j] = frame_buffer + src_w * j * 3;
                }
            }
            else
            {
                printf("main %d: Unknown JPEG colorspace %d\n", 
                    __LINE__, cinfo.jpeg_color_space);
                exit(1);
            }

	        while(cinfo.output_scanline < src_h)
	        {
		        int num_scanlines = jpeg_read_scanlines(&cinfo, 
			        &rows[cinfo.output_scanline],
			        src_h - cinfo.output_scanline);
	        }

// RGB -> greyscale
            if(cinfo.jpeg_color_space == JCS_RGB)
            {
                src_ptr = frame_buffer;
                dst_ptr = frame_buffer;
                for(j = 0; j < src_w * src_h; j++)
                {
// take red only
                    *dst_ptr++ = *src_ptr;
                    src_ptr += 3;
                }
            }
            else
            if(cinfo.jpeg_color_space == JCS_YCbCr)
            {
// packed YUV
                src_ptr = frame_buffer;
                dst_ptr = frame_buffer;

// printf("main %d: %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
// __LINE__,
// src_ptr[0],
// src_ptr[1],
// src_ptr[2],
// src_ptr[3],
// src_ptr[4],
// src_ptr[5],
// src_ptr[6],
// src_ptr[7],
// src_ptr[8]);

                for(j = 0; j < src_w * src_h; j++)
                {
// take Y only
                    *dst_ptr++ = *src_ptr;
                    src_ptr += 3;
                }
            }

            jpeg_destroy_decompress(&cinfo);

            crop_w2 = crop_w;
            crop_h2 = crop_h;
            if(src_w < crop_x + crop_w2)
            {
                crop_w2 = src_w - crop_x;
                printf("main %d: truncating crop_w to %d\n", __LINE__, crop_w2);
            }

            if(src_h < crop_y + crop_h2)
            {
                crop_h2 = src_h - crop_y;
                printf("main %d: truncating crop_h to %d\n", __LINE__, crop_h2);
            }


            process_frame(frame_buffer, src_w, src_h);
            free(buffer);
            free(frame_buffer);
            free(rows);
            current_page++;
            total_pages++;
        }
    }
    else
    {
        printf("Unrecognized input format.\n");
    }

// write the page count
    fseek(dest_fd, page_offset, SEEK_SET);
    fwrite(&total_pages, 1, 4, dest_fd);
    

    fclose(dest_fd);
    if(dest2)
    {
        sprintf(string, "gzip -9 %s", dest);
        printf("Compressing\n");
        int _ = system(string);
    }    


    return 0;
}




