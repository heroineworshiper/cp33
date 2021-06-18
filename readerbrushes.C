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







// the X11 & gimp circle drawing routines suck

#include "reader.inc"
#include "readerbrushes.h"

ArrayList<Brush*> outline_brushes;
ArrayList<Brush*> solid_brushes;



#define BASE MAX_BRUSH
uint8_t base_bitmap[BASE * BASE];

Brush::Brush()
{
    image = 0;
}

Brush::~Brush()
{
    delete image;
}


uint8_t* make_quad(int size)
{
    int i, j, k, l;
    uint8_t *result = (uint8_t*)malloc(size * size);

//    printf("make_quad size=%d\n", size);
// downsample the base image
    for(i = 0; i < size; i++)
    {
        int y1 = i * BASE / size;
        int y2 = (i + 1) * BASE / size;
        for(j = 0; j < size; j++)
        {
            int x1 = j * BASE / size;
            int x2 = (j + 1) * BASE / size;
            int accum = 0;
            for(k = y1; k < y2; k++)
            {
                for(l = x1; l < x2; l++)
                {
                    if(base_bitmap[k * BASE + l])
                    {
                        accum++;
                    }
                }
            }


            if(accum > (x2 - x1) * (y2 - y1) / 2)
            {
                result[i * size + j] = 1;
//                printf("*");
            }
            else
            {
                result[i * size + j] = 0;
//                printf(" ");
            }
        }
//        printf("\n");
    }
    
    return result;
}

void init_brushes()
{
    int i, j, k, l;

    for(i = 0; i < BASE; i++)
    {
        for(j = 0; j < BASE; j++)
        {
            float dist = hypot(i, j);
            if(dist < BASE)
            {
                base_bitmap[i * BASE + j] = 1;
//                printf("*");
            }
            else
            {
                base_bitmap[i * BASE + j] = 0;
//                printf(" ");
            }
        }
//        printf("\n");
    }

    int quad_size;
    int brush_size = 1;
    for(quad_size = 1; quad_size < MAX_BRUSH / 2 + 1; quad_size++)
    {
        uint8_t *quad = make_quad(quad_size);

// construct 2 brush sizes from each quad size
        for(i = 0; i < 2; i++)
        {
            Brush *brush = new Brush;
            brush->size = brush_size;
            brush->image = new VFrame(brush_size, brush_size, BC_A8);

            uint8_t **rows = brush->image->get_rows();
            for(j = 0; j < quad_size; j++)
            {
                for(k = 0; k < quad_size; k++)
                {
                    int quad_value = quad[(quad_size - j - 1) * quad_size + (quad_size - k - 1)];
                    quad_value *= 0xff;
// top left
                    rows[j][k] = quad_value;
// top right
                    rows[j][brush_size - k - 1] = quad_value;
// bottom left
                    rows[brush_size - j - 1][k] = quad_value;
// bottom right
                    rows[brush_size - j - 1][brush_size - k - 1] = quad_value;
                }
            }


            solid_brushes.append(brush);

// create outline brush
            Brush *brush2 = new Brush;
            brush2->size = brush_size;
            brush2->image = new VFrame(brush_size, brush_size, BC_A8);
            if(brush_size < 3)
            {
                brush2->image->copy_from(brush->image);
            }
            else
            {
                uint8_t **src_rows = (uint8_t**)brush->image->get_rows();
                uint8_t **dst_rows = (uint8_t**)brush2->image->get_rows();
                for(j = 0; j < brush_size; j++)
                {
// copy 1st pixel on left side of row
                    for(k = 0; k < brush_size; k++)
                    {
                        if(src_rows[j][k])
                        {
                            dst_rows[j][k] = 0xff;
                            break;
                        }
                    }
// copy 1st pixel on right side of row
                    for(k = brush_size - 1; k >= 0; k--)
                    {
                        if(src_rows[j][k])
                        {
                            dst_rows[j][k] = 0xff;
                            break;
                        }
                    }
// copy 1st pixel on top of column
                    for(k = 0; k < brush_size; k++)
                    {
                        if(src_rows[k][j])
                        {
                            dst_rows[k][j] = 0xff;
                            break;
                        }
                    }
// copy 1st pixel on bottom of column
                    for(k = brush_size - 1; k >= 0; k--)
                    {
                        if(src_rows[k][j])
                        {
                            dst_rows[k][j] = 0xff;
                            break;
                        }
                    }
                }

// delete excess pixels
                for(j = 1; j < brush_size - 1; j++)
                {
                    for(k = 1; k < brush_size - 1; k++)
                    {
                        if(dst_rows[j][k] &&
                            ((dst_rows[j + 1][k] && dst_rows[j][k + 1]) ||
                            (dst_rows[j - 1][k] && dst_rows[j][k - 1]) ||
                            (dst_rows[j - 1][k] && dst_rows[j][k + 1]) ||
                            (dst_rows[j + 1][k] && dst_rows[j][k - 1])))
                        {
                            dst_rows[j][k] = 0;
                            src_rows[j][k] = 0;
                        }
                    }
//                    printf("\n");
                }


// printf("brush_size=%d\n", brush_size);
// for(j = 0; j < brush_size; j++)
// {
//     for(k = 0; k < brush_size; k++)
//     {
//         if(src_rows[j][k])
//         {
//             printf("*");
//         }
//         else
//         {
//             printf(" ");
//         }
//     }
//     printf("\n");
// }
            }


            outline_brushes.append(brush2);

            brush_size++;
        }

        free(quad);
    }
}







