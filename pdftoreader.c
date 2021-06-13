/*
 * MUSIC READER
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




// convert a PDF to a baked image stack for reader.C
// gcc -O3 -g -o pdftoreader pdftoreader.c -lpng




#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <png.h>


#define DISPLAY_W 768
#define DISPLAY_H 1366
#define TEXTLEN 1024
#define DPI 300


int page1;
int page2;
int total_sources = 0;
int crop_x, crop_y, crop_w, crop_h;
int crop_w2, crop_h2;
FILE *dest_fd;

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
    uint8_t dst_row[DISPLAY_W];
    int row;
    int subrow;
    int col;
    int i, j;

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
        bzero(dst_row, DISPLAY_W);
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
                uint8_t value = 0xff;
                if(src_col1 == src_col0)
                {
                    src_col1++;
                }
                
// get darkest pixel in rectangle
                for(i = src_row0; i < src_row1; i++)
                {
                    for(j = src_col0; j < src_col1; j++)
                    {
                        uint8_t new_value = *(src + src_w * i + j);
                        if(new_value < value)
                        {
                            value = new_value;
                        }
                    }
                }

                if(value > 0x80)
                {
                    dst_row[col] |= component;
                }
            }
        }

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


int main(int argc, char *argv[])
{
    if(argc < 6)
    {
        char *progname = argv[0];
        printf("Usage: %s <source files> <1st page> <last page inclusive> <crop x,y> <crop w,h> <dest file>\n", 
            progname);
        printf("Example: %s /home/archive/scherzo4b.pdf 1 99 135,0 2294,3567 scherzo4.reader\n", progname);
        printf("Example: %s /home/archive/rbg*.png 1 99 231,0 2982x4400 rbg.reader\n", progname);
        printf("Example: %s /home/archive/separate*.png 1 99 0,0 9999x9999 separate.reader\n", progname);
        exit(1);
    }
    
    char string[TEXTLEN];
    char **sources = &argv[1];
    total_sources = argc - 6;
    page1 = atoi(argv[1 + total_sources]);
    page2 = atoi(argv[2 + total_sources]);
    argtoxy(&crop_x, &crop_y, argv[3 + total_sources]);
    argtoxy(&crop_w, &crop_h, argv[4 + total_sources]);
    char *dest = argv[5 + total_sources];
    int i, j;
    
    printf("Sources:\n");
    for(i = 0; i < total_sources; i++)
    {
        printf("\t%s\n", sources[i]);
    }
    printf("Page 1: %d\n", page1);
    printf("Page 2: %d\n", page2);
    printf("Crop: %d,%d %dx%d\n", crop_x, crop_y, crop_w, crop_h);
    printf("Dest: %s\n", dest);
    
    dest_fd = fopen(dest, "w");
    if(!dest_fd)
    {
        printf("Couldn't open %s for writing.\n", dest);
        exit(1);
    }
    int display_w = DISPLAY_W;
    int display_h = DISPLAY_H;
    int temp = 0;
    fwrite(&display_w, 1, 4, dest_fd);
    fwrite(&display_h, 1, 4, dest_fd);
// number of pages
    int page_offset = ftell(dest_fd);
    fwrite(&temp, 1, 4, dest_fd);


    int current_page = 1;
    int total_pages = 0;
// open PDF
    if(!strcasecmp(sources[0] + strlen(sources[0]) - 4, ".pdf"))
    {
        sprintf(string, 
            "gs -dBATCH -dNOPAUSE -sDEVICE=pnm -o - -r%d %s", 
            DPI, 
            sources[0]);
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
    if(!strcasecmp(sources[0] + strlen(sources[0]) - 4, ".png"))
    {
        for(i = 0; i < total_sources; i++)
        {
            printf("Reading page %s\n", sources[i]);
            FILE *fd = fopen(sources[i], "r");
            fseek(fd, 0, SEEK_END);
            int size = ftell(fd);
            fseek(fd, 0, SEEK_SET);
            uint8_t *buffer = malloc(size);
            uint8_t *frame_buffer = 0;
            uint8_t **rows = 0;
            uint8_t *src_ptr;
            uint8_t *dst_ptr;
            int _ = fread(buffer, 1, size, fd);
            fclose(fd);
            
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
                
                default:
                    printf("main %d: unknown color space\n", __LINE__);
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
    {
        printf("Unrecognized input format.\n");
    }

// write the page count
    fseek(dest_fd, page_offset, SEEK_SET);
    fwrite(&total_pages, 1, 4, dest_fd);
    

    fclose(dest_fd);
    
    return 0;
}




