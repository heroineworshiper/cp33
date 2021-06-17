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

#include "reader.h"
#include "readertheme.h"
#include "readerwindow.h"



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
    new_button("load.png",
		"button_up.png",
		"button_hi.png",
		"button_dn.png",
        "load");
    new_button("save.png",
		"button_up.png",
		"button_hi.png",
		"button_dn.png",
        "save");
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
    new_toggle("erase.png",
		"button_up.png",
		"button_hi.png",
		"check.png",
		"button_dn.png",
		"check_hi.png",
		"erase");
    new_toggle("draw.png",
		"button_up.png",
		"button_hi.png",
		"check.png",
		"button_dn.png",
		"check_hi.png",
		"draw");

    new_button("hollow_brush.png",
		"button_up.png",
		"button_hi.png",
		"button_dn.png",
		"hollow_brush");
    new_button("filled_brush.png",
		"button_up.png",
		"button_hi.png",
		"button_dn.png",
		"filled_brush");
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
    
    
}

void ReaderTheme::fill_box(VFrame *dst, uint32_t color)
{
    uint8_t **rows = dst->get_rows();
    uint8_t r = (color & 0xff0000) >> 16;
    uint8_t g = (color & 0xff00) >> 8;
    uint8_t b = color & 0xff0;
    for(int i = 5; i < 29; i++)
    {
        uint8_t *row = rows[i];
        for(int j = 5; j < 30; j++)
        {
            row[j * 4 + 0] = r;
            row[j * 4 + 1] = g;
            row[j * 4 + 2] = b;
            row[j * 4 + 3] = 0xff;
        }
    }
}






