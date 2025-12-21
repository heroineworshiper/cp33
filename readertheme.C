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

#include "reader.h"
#include "readertheme.h"
#include "mwindow.h"



ReaderTheme::ReaderTheme()
 : BC_Theme()
{
}


void ReaderTheme::initialize()
{
	BC_Resources *resources = BC_WindowBase::get_resources();
    extern unsigned char _binary_theme_data_start[];
    set_data(_binary_theme_data_start);
    
    BC_WindowBase::get_resources()->tooltip_bg_color = WHITE;
    margin = DP(10);

    resources->usethis_button_images = 
		resources->ok_images = new_button("ok.png",
		"bigbutton_up.png", 
		"bigbutton_hi.png", 
		"bigbutton_dn.png");

    resources->cancel_images = new_button("cancel.png",
		"bigbutton_up.png", 
		"bigbutton_hi.png", 
		"bigbutton_dn.png");

    new_button("record.png",
		"button_up.png",
		"button_hi.png",
		"button_dn.png",
        "record");
    new_button("stop.png",
		"button_up.png",
		"button_hi.png",
		"button_dn.png",
        "stop");
    new_button("load.png",
		"button_up.png",
		"button_hi.png",
		"button_dn.png",
        "load");
    new_button("new.png",
		"button_up.png",
		"button_hi.png",
		"button_dn.png",
        "new");
    new_button("save.png",
		"button_up.png",
		"button_hi.png",
		"button_dn.png",
        "save");
    new_button("save2.png",
		"button_up.png",
		"button_hi.png",
		"button_dn.png",
        "save2");
    new_button("undo.png",
		"button_up.png",
		"button_hi.png",
		"button_dn.png",
        "undo");
    new_button("redo.png",
		"button_up.png",
		"button_hi.png",
		"button_dn.png",
        "redo");
    new_button("capture.png",
		"button_up.png",
		"button_hi.png",
		"button_dn.png",
        "capture");
    new_button("reader.png",
		"button_up.png",
		"button_hi.png",
		"button_dn.png",
        "reader");



    new_button("prev_page.png",
		"button_up.png",
		"button_hi.png",
		"button_dn.png",
        "prev_page");
    new_button("next_page.png",
		"button_up.png",
		"button_hi.png",
		"button_dn.png",
        "next_page");
    new_button("erase1.png",
		"button_up.png",
		"button_hi.png",
		"button_dn.png",
        "erase1");
    new_toggle("erase.png", "erase");
    new_toggle("draw.png", "draw");
    new_toggle("start_8va_button.png", "start_8va");
    new_toggle("end_8va_button.png", "end_8va");
    new_toggle("8va_button.png", "8va_toggle");
    new_toggle("rest_button.png", "rest_toggle");
    new_toggle("key.png", "key_toggle");



    new_toggle("line.png", "line");
    new_toggle("box.png", "box");
    new_toggle("disc.png", "disc");
    new_toggle("circle.png", "circle");
    new_button("top_layer.png",
		"button_up.png",
		"button_hi.png",
		"button_dn.png",
		"top_layer");
    new_button("bottom_layer.png",
		"button_up.png",
		"button_hi.png",
		"button_dn.png",
		"bottom_layer");

    new_button("color.png",
		"button_up.png",
		"button_hi.png",
		"button_dn.png",
		"top_color");

    new_button("color.png",
		"button_up.png",
		"button_hi.png",
		"button_dn.png",
		"bottom_color");

	new_image("palette_bg", "palette_bg.png");

    VFrame *palette_fg = new_image("color.png");
    VFrame *palette_bg[3];
    palette_bg[0] = new_image("button_up.png");
    palette_bg[1] = new_image("button_hi.png");
    palette_bg[2] = new_image("button_dn.png");



    for(int i = 0; i < TOTAL_COLORS; i++)
    {
        top_colors[i] = new VFrame*[3];
        bottom_colors[i] = new VFrame*[3];
        fill_box(palette_fg, MWindow::top_colors[i]);
        for(int j = 0; j < 3; j++)
	    {
            top_colors[i][j] = new VFrame(*palette_bg[j]);
    	    overlay(top_colors[i][j], palette_fg, -1, -1, (j == 2));
	    }

        fill_box(palette_fg, MWindow::bottom_colors[i]);
        for(int j = 0; j < 3; j++)
	    {
            bottom_colors[i][j] = new VFrame(*palette_bg[j]);
    	    overlay(bottom_colors[i][j], palette_fg, -1, -1, (j == 2));
	    }
    }

//     printf("ReaderTheme::initialize %d %p %p\n", 
//         __LINE__, get_image_set("load"), get_image_set("save"));
    resources->dirbox_margin = 0;
	resources->filebox_margin = 0;
}

void ReaderTheme::fill_box(VFrame *dst, uint32_t color)
{
    uint8_t **rows = dst->get_rows();
    uint8_t r = (color & 0xff0000) >> 16;
    uint8_t g = (color & 0xff00) >> 8;
    uint8_t b = color & 0xff;
    for(int i = 5; i < 28; i++)
    {
        uint8_t *row = rows[i];
        for(int j = 5; j < 29; j++)
        {
            row[j * 4 + 0] = r;
            row[j * 4 + 1] = g;
            row[j * 4 + 2] = b;
            row[j * 4 + 3] = 0xff;
        }
    }
}

void ReaderTheme::invert(VFrame *dst)
{
    int components = 4;
    for(int i = 0; i < dst->get_h(); i++)
    {
        uint8_t *row = dst->get_rows()[i];
        for(int j = 0; j < dst->get_w(); j++)
        {
            if(row[j * components + 3] == 0)
            {
                row[j * components + 0] = 0;
                row[j * components + 1] = 0;
                row[j * components + 2] = 0;
                row[j * components + 3] = 0xff;
            }
            else
            {
                row[j * components + 0] ^= 0xff;
                row[j * components + 1] ^= 0xff;
                row[j * components + 2] ^= 0xff;
            }
        }
    }
}

VFrame** ReaderTheme::new_toggle(const char *overlay_path, 
	const char *title)
{
//printf("ReaderTheme::new_toggle %d %s\n", __LINE__, overlay_path);
	VFrame default_data;
	default_data.read_png(get_image_data(overlay_path), BC_Resources::dpi);
	BC_ThemeSet *result = new BC_ThemeSet(5, 1, title ? title : (char*)"");
	if(title) set_image_set(title, result);

	result->data[0] = new_image("button_up.png");
	result->data[1] = new_image("button_hi.png");
	result->data[2] = new_image("button_up.png"); // checked
	result->data[3] = new_image("button_dn.png");
	result->data[4] = new_image("button_hi.png"); // checked hi
	for(int i = 0; i < 5; i++)
		overlay(result->data[i], &default_data, -1, -1, (i == 3));
// invert the checked modes
    invert(result->data[2]);
    invert(result->data[4]);
	return result->data;
}





