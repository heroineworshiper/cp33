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

#include "clip.h"
#include "cursors.h"
#include "keys.h"
#include "reader.h"
//#include "readerbrushes.h"
#include "readermenu.h"
#include "readertheme.h"
#include "readerwindow.h"
#include <string.h>
#include <unistd.h>


MWindow* MWindow::mwindow = 0;
const uint32_t MWindow::top_colors[TOTAL_COLORS] = 
{
    0xffffff, // white/erase
    0x000000, // black
    0x404040, // grey
    0xff0000, // red
    0xcc44cc, // purple
    0x00ff00, // green
    0x0000ff, // blue
    0x664400, // brown
};

const uint32_t MWindow::bottom_colors[TOTAL_COLORS] = 
{
    0xffffff, // white/erase
    0xeeee77, // yellow
    0xc0c0c0, // light grey
    0xff7777, // light red
    0xaaffee, // cyan
    0xaaff66, // light green
    0x0088ff, // light blue
    0xdd8855, // orange
};

uint8_t MWindow::top_rgb888[(TOTAL_COLORS + 1) * 3];
uint8_t MWindow::bottom_rgb888[(TOTAL_COLORS + 1) * 3];
uint16_t MWindow::top_rgb565[TOTAL_COLORS + 1];
uint16_t MWindow::bottom_rgb565[TOTAL_COLORS + 1];

MWindow::MWindow() : BC_Window()
{
    current_operation = IDLE;
    cursor_visible = 0;
    current_undo = 0;
    total_undos = 0;
    zoom_x = 0;
    zoom_y = 0;
    zoom_factor = 1;

//#ifndef USE_WINDOW
//    set_border(0);
//#endif
};

static void to_rgb888(uint8_t *dst, uint32_t src)
{
    dst[0] = (src & 0xff0000) >> 16;
    dst[1] = (src & 0xff00) >> 8;
    dst[2] = (src & 0xff);
}

static void to_rgb565(uint16_t *dst, uint32_t src)
{
    *dst = (((src & 0xf80000) >> 8) |
        ((src & 0xfc00) >> 5) |
        ((src & 0xf8) >> 3));
}

void MWindow::init_colors()
{
// index 0 is always transparent
    for(int i = 0; i < TOTAL_COLORS; i++)
    {
        to_rgb888(&top_rgb888[i * 3], top_colors[i]);
        to_rgb888(&bottom_rgb888[i * 3], bottom_colors[i]);
        to_rgb565(&top_rgb565[i], top_colors[i]);
        to_rgb565(&bottom_rgb565[i], bottom_colors[i]);
    }
}


void MWindow::create_objects()
{
    load_defaults();

    theme = new ReaderTheme;
    theme->initialize();
    get_resources()->bg_color = WHITE;

    oval_table[0] = 0;
    for(int i = 1; i < OVAL_BASE; i++)
    {
// solve pythagorean therum for Y axis & scale to 0-1
        int j = (OVAL_BASE - 1 - i);
        oval_table[i] = sqrt(OVAL_BASE * OVAL_BASE - j * j) / OVAL_BASE;
//printf("MWindow::create_objects %d %f\n", __LINE__, oval_table[i]);
    }

    create_window("Reader", // title
        0, // x
        0, // y
        ROOT_W, // w
        ROOT_H, // h
        -1, // min_w
        -1, // min_h
        0, // allow_resize
        1, // private_color
        1, // hide
        WHITE, // bg_color
        "", // display_name
        0); // group_it
    root_w = get_w();
    root_h = get_h();
    if(client_mode)
    {
        set_cursor(TRANSPARENT_CURSOR, 0, 0);
    }
    else
    {
        for(int i = 0; i < UNDO_LEVELS; i++)
        {
            undo_before[i] = new VFrame(ROOT_W, ROOT_H, BC_A8);
            undo_after[i] = new VFrame(ROOT_W, ROOT_H, BC_A8);
        }

    }

// draw bitmaps to the foreground/win instead of the back buffer/pixmap
#ifndef BG_PIXMAP
    start_video();
#endif



    show_window(1);

    menu = new MenuThread;
    menu->create_objects();
    menu->start();
    
    load = new LoadFileThread;
//    printf("MWindow::create_objects %d color_model=%d\n", __LINE__, get_color_model());
}


void MWindow::load_defaults()
{
    if(!defaults)
    {
    	defaults = new BC_Hash("~/.readerrc");
    }

	defaults->load();
    defaults = new BC_Hash("~/.readerrc");
	defaults->load();
    BC_WindowBase::load_defaults(defaults);
    
    is_top = defaults->get("IS_TOP", 0);
    is_hollow = defaults->get("IS_HOLLOW", 0);
    top_color = defaults->get("TOP_COLOR", 0);
    bottom_color = defaults->get("BOTTOM_COLOR", 0);
    draw_size = defaults->get("DRAW_SIZE", 1);
    erase_size = defaults->get("ERASE_SIZE", 1);
    CLAMP(top_color, 0, TOTAL_COLORS - 1);
    CLAMP(bottom_color, 0, TOTAL_COLORS - 1);
    CLAMP(draw_size, 1, MAX_BRUSH);
    CLAMP(erase_size, 1, MAX_BRUSH);

    
    if(get_resources()->filebox_w > ROOT_W)
    {
        get_resources()->filebox_w = ROOT_W;
    }
    if(get_resources()->filebox_h > ROOT_H)
    {
        get_resources()->filebox_h = ROOT_H;
    }


    BC_WindowBase::load_defaults(defaults);

    char string[BCTEXTLEN];
    sprintf(string, "*%s", READER_SUFFIX);
    strcpy(get_resources()->filebox_filter, string);
}


void MWindow::save_defaults()
{
    BC_WindowBase::save_defaults(defaults);
    defaults->update("IS_TOP", is_top);
    defaults->update("IS_HOLLOW", is_hollow);
    defaults->update("TOP_COLOR", (int32_t)top_color);
    defaults->update("BOTTOM_COLOR", (int32_t)bottom_color);
    defaults->update("DRAW_SIZE", (int32_t)draw_size);
    defaults->update("ERASE_SIZE", (int32_t)erase_size);
    defaults->save();
}

void MWindow::show_error(char *text)
{
    lock_window();
    clear_box(0, 0, root_w, root_h, 0);
    set_color(BLACK);
    draw_text(theme->margin, 
        theme->margin + get_text_height(MEDIUMFONT), 
        text);
    flash();
    unlock_window();
}

void MWindow::update_save()
{
    if(pages.size())
    {
        file_changed = 1;
        unlock_window();
        MenuWindow::menu_window->lock_window();
        MenuWindow::menu_window->save->set_images(
            MWindow::mwindow->theme->get_image_set("save2"));
        MenuWindow::menu_window->save->draw_face(1);
        MenuWindow::menu_window->unlock_window();
        lock_window();
    }
}


void MWindow::reset_undo()
{
    current_undo = 0;
    total_undos = 0;
}

void MWindow::push_undo_before()
{
    if(current_page < pages.size())
    {
        Page *page = pages.get(current_page);
// free up a level
        if(current_undo >= UNDO_LEVELS)
        {
            VFrame *temp_before = undo_before[0];
            VFrame *temp_after = undo_after[0];
            for(int i = 0; i < UNDO_LEVELS - 1; i++)
            {
                undo_before[i] = undo_before[i + 1];
                undo_after[i] = undo_after[i + 1];
            }
            undo_before[UNDO_LEVELS - 1] = temp_before;
            undo_after[UNDO_LEVELS - 1] = temp_after;
            current_undo--;
            total_undos--;
        }

// put in current level & truncate redos
        undo_before[current_undo]->copy_from(page->annotations);
    }
}

void MWindow::push_undo_after()
{
    if(current_page < pages.size())
    {
        Page *page = pages.get(current_page);
        
        undo_after[current_undo]->copy_from(page->annotations);
        
        current_undo++;
        total_undos = current_undo;
    }
}

void MWindow::pop_undo()
{
    if(current_page < pages.size())
    {
        Page *page = pages.get(current_page);
//printf("MWindow::pop_undo %d %d %d\n", __LINE__, current_undo, total_undos);
        if(current_undo > 0)
        {
            current_undo--;
            page->annotations->copy_from(undo_before[current_undo]);
            show_page(current_page, 0);
        }
    }
}

void MWindow::pop_redo()
{
    if(current_page < pages.size())
    {
        Page *page = pages.get(current_page);
//printf("MWindow::pop_redo %d %d %d\n", __LINE__, current_undo, total_undos);
        if(current_undo < total_undos)
        {
            page->annotations->copy_from(undo_after[current_undo]);
            current_undo++;
            show_page(current_page, 0);
        }
    }
}



// color palettes
const uint16_t rgb565_table[] =
{
    0b0000000000000000,
    0b1111100000000000,
    0b0000011111100000,
    0b1111111111100000,
    0b0000000000011111,
    0b1111100000011111,
    0b0000011111111111,
    0b1111111111111111
};

const uint8_t rgb8_table[] =
{
    0b00000000,
    0b11000000,
    0b00111000,
    0b11111000,
    0b00000111,
    0b11000111,
    0b00111111,
    0b11111111
};

// draw full page
void MWindow::show_page(int number, int lock_it)
{
    show_page_fragment(number,
        0,
        0,
        root_w,
        root_h,
        lock_it);
    return;
}

// draw part of a page
void MWindow::show_page_fragment(int number,
    int x1,
    int y1,
    int x2,
    int y2,
    int lock_it)
{
    if(number >= pages.size())
    {
        lock_window();
        clear_box(0, 0, root_w, root_h, 0);
        flash();
        unlock_window();
        return;
    }

    CLAMP(x1, zoom_x, zoom_x + root_w / zoom_factor);
    CLAMP(x2, zoom_x, zoom_x + root_w / zoom_factor);
    CLAMP(y1, zoom_y, zoom_y + root_h / zoom_factor);
    CLAMP(y2, zoom_y, zoom_y + root_h / zoom_factor);

// printf("MWindow::show_page_fragment %d %d %d %d %d\n",
// __LINE__,
// x1,
// x2, 
// y1, 
// y2);

    Page *page = pages.get(number);
    if(lock_it)
    {
        lock_window();
    }
    BC_Bitmap *bitmap = get_temp_bitmap(root_w,
        root_h,
        get_color_model());

// convert to display color model
    switch(get_color_model())
    {
        case BC_BGR8888:
            for(int i = y1; i < y2; i++)
            {
                uint8_t *src_row = page->image->get_rows()[i] + x1;
                uint8_t *annotation_row = page->annotations->get_rows()[i] + x1;
                uint8_t *dst_row = bitmap->get_row_pointers()[(i - zoom_y) * zoom_factor] + 
                    (x1 - zoom_x) * 4 * zoom_factor;
                for(int j = x1; j < x2; j++)
                {
                    uint8_t src_value = *src_row++;
                    uint8_t annotation_value = *annotation_row++;
                    uint8_t r = 0;
                    uint8_t g = 0;
                    uint8_t b = 0;

// foreground layer
                    if((annotation_value & 0xf0))
                    {
                        uint8_t *color = &top_rgb888[(annotation_value >> 4) * 3];
//printf("MWindow::show_page_fragment %d %d\n", __LINE__, annotation_value);
                        r = *color++;
                        g = *color++;
                        b = *color++;
                    }
                    else
                    if(src_value != 0x7)
                    {
// source bit mask to RGB
                        if((src_value & 0x1))
                        {
                            r = 0xff;
                        }

                        if((src_value & 0x2))
                        {
                            g = 0xff;
                        }

                        if((src_value & 0x4))
                        {
                            b = 0xff;
                        }
                    }
                    else
// background layer
                    if((annotation_value & 0x0f))
                    {
                        uint8_t *color = &bottom_rgb888[(annotation_value & 0x0f) * 3];
                        r = *color++;
                        g = *color++;
                        b = *color++;
                    }
                    else
// white
                    {
                        r = g = b = 0xff;
                    }

                    if(zoom_factor == 1)
                    {
                        dst_row[0] = b;
                        dst_row[1] = g;
                        dst_row[2] = r;
                        dst_row += 4;
                    }
                    else
                    {
                        for(int k = 0; k < zoom_factor; k++)
                        {
                            uint8_t *dst_row2 = dst_row + k * root_w * 4;
                            for(int l = 0; l < zoom_factor; l++)
                            {
                                dst_row2[0] = b;
                                dst_row2[1] = g;
                                dst_row2[2] = r;
                                dst_row2 += 4;
                            }
                        }
                        dst_row += 4 * zoom_factor;
                    }
                }
            }
            break;

        case BC_RGB565:
        {
            for(int i = y1; i < y2; i++)
            {
                uint8_t *src_row = page->image->get_rows()[i] + x1;
                uint8_t *annotation_row = page->annotations->get_rows()[i] + x1;
                uint16_t *dst_row = (uint16_t*)bitmap->get_row_pointers()[(i - zoom_y) * zoom_factor] + 
                    (x1 - zoom_x) * zoom_factor;
                for(int j = x1; j < x2; j++)
                {
                    uint8_t src_value = *src_row++;
                    uint16_t dst_value = 0;
                    uint8_t annotation_value = *annotation_row++;
// foreground layer
                    if((annotation_value & 0xf0))
                    {
                        dst_value = top_rgb565[annotation_value >> 4];
                    }
                    else
                    if(src_value != 0x7)
                    {
// source bit mask to RGB
                        dst_value = rgb565_table[src_value];
                    }
                    else
// background layer
                    if((annotation_value & 0x0f))
                    {
                        dst_value = bottom_rgb565[(annotation_value & 0x0f)];
                    }
                    else
                    {
// white
                        dst_value = 0xffff;
                    }
                    
                    
                    if(zoom_factor == 1)
                    {
                        *dst_row++ = dst_value;
                    }
                    else
                    {
                        for(int k = 0; k < zoom_factor; k++)
                        {
                            uint16_t *dst_row2 = dst_row + k * root_w;
                            for(int l = 0; l < zoom_factor; l++)
                            {
                                *dst_row2++ = dst_value;
                            }
                        }
                        dst_row += zoom_factor;
                    }
                }
            }
            break;
        }

        default:
            printf("MWindow::show_page_fragment %d unknown color model %d\n",
                __LINE__,
                get_color_model());
            break;
    }

    int cursor_x2 = cursor_x;
    int cursor_y2 = cursor_y;
    hide_cursor();
//    draw_bitmap(bitmap, 0);
//    flash(0);
    int x1_zoomed = (x1 - zoom_x) * zoom_factor;
    int y1_zoomed = (y1 - zoom_y) * zoom_factor;
    int w_zoomed = (x2 - x1) * zoom_factor;
    int h_zoomed = (y2 - y1) * zoom_factor;
    draw_bitmap(bitmap, 
        0,
        x1_zoomed,
        y1_zoomed,
        w_zoomed,
        h_zoomed,
        x1_zoomed,
        y1_zoomed,
        w_zoomed,
        h_zoomed,
        0);

//printf("MWindow::show_page_fragment %d %d %d %d %d\n", __LINE__, x1, y1, x2, y2);
    flash(x1_zoomed, y1_zoomed, w_zoomed, h_zoomed, 0);
    update_cursor(cursor_x2, cursor_y2);
    flush();
    if(lock_it)
    {
        unlock_window();
    }
}

int MWindow::export_page(const char *export_path, int number)
{
    Page *page = pages.get(number);
// scale it to draw RGB oversampling
    int dst_w = root_w * 3;
    int dst_h = root_h * 3;
    VFrame *dst = new VFrame(dst_w, dst_h, BC_RGB888);
    
    for(int i = 0; i < root_h; i++)
    {
        uint8_t *src_row = page->image->get_rows()[i];
        uint8_t *annotation_row = page->annotations->get_rows()[i];
        uint8_t *dst_row0 = dst->get_rows()[i * 3 + 0];
        uint8_t *dst_row1 = dst->get_rows()[i * 3 + 1];
        uint8_t *dst_row2 = dst->get_rows()[i * 3 + 2];
        for(int j = 0; j < root_w; j++)
        {
            uint8_t src_value = *src_row++;
            uint8_t annotation_value = *annotation_row++;
            uint8_t r0 = 0;
            uint8_t g0 = 0;
            uint8_t b0 = 0;
            uint8_t r1 = 0;
            uint8_t g1 = 0;
            uint8_t b1 = 0;
            uint8_t r2 = 0;
            uint8_t g2 = 0;
            uint8_t b2 = 0;

// foreground layer
            if((annotation_value & 0xf0))
            {
                uint8_t *color = &top_rgb888[(annotation_value >> 4) * 3];
//printf("MWindow::show_page_fragment %d %d\n", __LINE__, annotation_value);
                r0 = r1 = r2 = *color++;
                g0 = g1 = g2 = *color++;
                b0 = b1 = b2 = *color++;
            }
            else
            if(src_value != 0x7)
            {
// source bit mask to RGB
                if((src_value & 0x1))
                {
                    r0 = g0 = b0 = 0xff;
                }

                if((src_value & 0x2))
                {
                    r1 = g1 = b1 = 0xff;
                }

                if((src_value & 0x4))
                {
                    r2 = g2 = b2 = 0xff;
                }
            }
            else
// background layer
            if((annotation_value & 0x0f))
            {
                uint8_t *color = &bottom_rgb888[(annotation_value & 0x0f) * 3];
                r0 = r1 = r2 = *color++;
                g0 = g1 = g2 = *color++;
                b0 = b1 = b2 = *color++;
            }
            else
// white
            {
                r0 = r1 = r2 = 0xff;
                g0 = g1 = g2 = 0xff;
                b0 = b1 = b2 = 0xff;
            }

            for(int k = 0; k < 3; k++)
            {
                dst_row0[0] = r0;
                dst_row0[1] = g0;
                dst_row0[2] = b0;
                dst_row0 += 3;
                dst_row1[0] = r1;
                dst_row1[1] = g1;
                dst_row1[2] = b1;
                dst_row1 += 3;
                dst_row2[0] = r2;
                dst_row2[1] = g2;
                dst_row2[2] = b2;
                dst_row2 += 3;
            }
        }
    }

    
    
    char path[BCTEXTLEN];
    sprintf(path, "%s%02d.png", export_path, number);
    if(dst->write_png(path, 9)) 
    {
        printf("MWindow::export_page %d: error writing %s\n", __LINE__, path);
        return 1;
    }
    delete dst;
    printf("MWindow::export_page %d: exported %s\n", __LINE__, path);

    return 0;
}

void MWindow::toggle_zoom()
{
    if(zoom_factor == 1)
    {
        zoom_factor = 3;
// what's under the cursor stays under the cursor
        zoom_x = get_cursor_x() - root_w / 6;
        zoom_y = get_cursor_y() - root_h / 6;
        zoom_x -= (get_cursor_x() - root_w / 2) / zoom_factor;
        zoom_y -= (get_cursor_y() - root_h / 2) / zoom_factor;
        CLAMP(zoom_x, 0, root_w - root_w / 3);
        CLAMP(zoom_y, 0, root_h - root_h / 3);
        show_page(current_page, 0);
    }
    else
    {
        zoom_factor = 1;
        zoom_x = 0;
        zoom_y = 0;
        show_page(current_page, 0);
    }
}

// debugging only
int MWindow::keypress_event()
{
    switch(get_keypress())
    {
// keyboard page turns are for testing only
        case LEFT:
            prev_page(2, 0);
            break;

        case RIGHT:
            next_page(2, 0);
            break;

// keyboard zoom is for testing only
        case 'z':
            toggle_zoom();
            break;

        case ESC:
            set_done(1);
            break;
    }

    return 1;
}

int MWindow::button_press_event()
{
    if(get_buttonpress() == 3)
    {
//        hide_cursor();
//        flush();
        dragging = 1;
        drag_x = get_cursor_x();
        drag_y = get_cursor_y();
        drag_zoom_x = zoom_x;
        drag_zoom_y = zoom_y;
        drag_accum_x = 0;
        drag_accum_y = 0;
        return 0;
    }

// raise the menu
    if(!MenuWindow::menu_window->get_hidden() &&
        current_operation != IDLE)
    {
        MenuWindow::menu_window->lock_window();
        MenuWindow::menu_window->raise_window(1);
        MenuWindow::menu_window->unlock_window();
    }

// start a line segment
    switch(current_operation)
    {
        case DRAWING:
            update_save();
            push_undo_before();
            segment_x = get_cursor_x() / zoom_factor + zoom_x;
            segment_y = get_cursor_y() / zoom_factor + zoom_y;
            draw_segment(0, segment_x, segment_y);
            return 1;
            break;
        
        case ERASING:
            update_save();
            push_undo_before();
            segment_x = get_cursor_x() / zoom_factor + zoom_x;
            segment_y = get_cursor_y() / zoom_factor + zoom_y;
            draw_segment(1, segment_x, segment_y);
            return 1;
            break;
        
        case DRAW_LINE:
        case DRAW_CIRCLE:
        case DRAW_DISC:
        case DRAW_BOX:
            if(polygon_state == POLYGON_IDLE)
            {
// start the polygon
                hide_cursor();
                polygon_state = POLYGON_CLICK1;
                polygon_x1 = get_cursor_x() / zoom_factor + zoom_x;
                polygon_y1 = get_cursor_y() / zoom_factor + zoom_y;
                update_cursor(polygon_x1, polygon_y1);
            }
            return 1;
            break;
        
        default:
            return 0;
            break;
    }
}


int MWindow::button_release_event()
{
    if(dragging)
    {
        if(drag_accum_x == 0 &&
            drag_accum_y == 0)
        {
// zoom toggle
            toggle_zoom();
        }
        else
        if(current_operation != IDLE)
        {
            update_cursor(get_cursor_x() / zoom_factor + zoom_x, 
                get_cursor_y() / zoom_factor + zoom_y);
            flush();
        }

// raise the menu
        if(!MenuWindow::menu_window->get_hidden())
        {
            MenuWindow::menu_window->lock_window();
            MenuWindow::menu_window->raise_window(1);
            MenuWindow::menu_window->unlock_window();
        }
        dragging = 0;
        return 1;
    }

    switch(current_operation)
    {
        case DRAWING:
            push_undo_after();
            return 1;
            break;
        
        case ERASING:
            push_undo_after();
            return 1;
            break;
        
        case DRAW_LINE:
        case DRAW_CIRCLE:
        case DRAW_DISC:
        case DRAW_BOX:
            if(polygon_state == POLYGON_CLICK1)
            {
                polygon_state = POLYGON_CLICK2;
            }
            else
            if(polygon_state == POLYGON_CLICK2)
            {
// finish the polygon
                update_save();
                hide_cursor();
                push_undo_before();
                
                if(current_operation == DRAW_LINE)
                {
                    segment_x = polygon_x1;
                    segment_y = polygon_y1;
                    draw_segment(0, 
                        get_cursor_x() / zoom_factor + zoom_x, 
                        get_cursor_y() / zoom_factor + zoom_y);
                }
                else
                if(current_operation == DRAW_BOX)
                {
                    finish_box();
                }
                else
                {
                    finish_oval();
                }
                
                polygon_state = POLYGON_IDLE;
                push_undo_after();
                update_cursor(get_cursor_x() / zoom_factor + zoom_x, 
                    get_cursor_y() / zoom_factor + zoom_y);
                flush();
            }
            return 1;
            break;
    }



    int x = get_abs_cursor_x(0);
    int y = get_abs_cursor_y(0);
    unlock_window();

    MenuWindow::menu_window->lock_window();
    if(x + MenuWindow::menu_window->get_w() > root_w)
    {
        x = root_w - MenuWindow::menu_window->get_w();
    }
    if(y + MenuWindow::menu_window->get_h() > root_h)
    {
        y = root_h - MenuWindow::menu_window->get_h();
    }
    if(x < 0)
    {
        x = 0;
    }

    if(y < 0)
    {
        y = 0;
    }
    MenuWindow::menu_window->unlock_window();


    if(load->is_running())
    {
        load->lock_gui("MWindow::button_release_event");
        load->gui->raise_window(1);
        load->unlock_gui();
    }
    else
    if(MenuWindow::menu_window->get_hidden())
    {
// show the menu
//printf("MWindow::button_release_event %d %d %d\n", __LINE__, x, y);
        
        MenuWindow::menu_window->lock_window();
        MenuWindow::menu_window->show_window();
// have to rehide buttons after show_window
        MenuWindow::menu_window->update_buttons();
// have to reposition after showing
        MenuWindow::menu_window->reposition_window(x, y);
        MenuWindow::menu_window->raise_window(1);
        MenuWindow::menu_window->unlock_window();
    }
    else
    {
//printf("MWindow::button_release_event %d\n", __LINE__);
// close all the windows
        MenuWindow::menu_window->lock_window();
        MenuWindow::menu_window->hide_windows(0);
//        MenuWindow::menu_window->reposition_window(x, y);
//        MenuWindow::menu_window->raise_window(1);
        MenuWindow::menu_window->unlock_window();
        exit_drawing();
    }

    lock_window();
    return 1;
}



int MWindow::cursor_motion_event()
{
    int need_cursor = 0;
    if(!dragging && !cursor_entered)
    {
// motion event happened after cursor left
        return 0;
    }

    if(dragging)
    {
        if(zoom_factor > 1)
        {
            int xdiff = get_cursor_x() - drag_x;
            int ydiff = get_cursor_y() - drag_y;
//printf("MWindow::cursor_motion_event %d %d %d\n", __LINE__, xdiff, ydiff);
            drag_accum_x += xdiff;
            drag_accum_y += ydiff;
// rewind cursor back to the same point
            reposition_cursor(drag_x, drag_y);


            int new_zoom_x = drag_zoom_x + drag_accum_x;
            int new_zoom_y = drag_zoom_y + drag_accum_y;

// clamp accumulators & window position
            if(new_zoom_x < 0)
            {
                int diff = -new_zoom_x;
                drag_accum_x += diff;
                new_zoom_x = 0;
            }
            if(new_zoom_y < 0)
            {
                int diff = -new_zoom_y;
                drag_accum_y += diff;
                new_zoom_y = 0;
            }
            if(new_zoom_x > root_w - root_w / 3)
            {
                int diff = new_zoom_x - (root_w - root_w / 3);
                drag_accum_x -= diff;
                new_zoom_x = root_w - root_w / 3;
            }
            if(new_zoom_y > root_h - root_h / 3)
            {
                int diff = new_zoom_y - (root_h - root_h / 3);
                drag_accum_y -= diff;
                new_zoom_y = root_h - root_h / 3;
            }
            
            
            if(new_zoom_x != zoom_x ||
                new_zoom_y != zoom_y)
            {
                zoom_x = new_zoom_x;
                zoom_y = new_zoom_y;
                show_page(current_page, 0);
            }
        }
//        return 1;
    }

    switch(current_operation)
    {
        case DRAWING:
//printf("MWindow::cursor_motion_event %d\n", __LINE__);
            if(get_button_down() && get_buttonpress() == 1)
            {
                draw_segment(0, 
                    get_cursor_x() / zoom_factor + zoom_x, 
                    get_cursor_y() / zoom_factor + zoom_y);
            }
            need_cursor = 1;
            flush();
            break;
        
        case ERASING:
//printf("MWindow::cursor_motion_event %d\n", __LINE__);
            if(get_button_down() && get_buttonpress() == 1)
            {
                draw_segment(1, 
                    get_cursor_x() / zoom_factor + zoom_x, 
                    get_cursor_y() / zoom_factor + zoom_y);
            }
            need_cursor = 1;
            break;
        
        case DRAW_LINE:
        case DRAW_CIRCLE:
        case DRAW_DISC:
        case DRAW_BOX:
// draw the outline without drawing anything
            need_cursor = 1;
            break;
    }
    
    if(need_cursor)
    {
//printf("MWindow::cursor_motion_event %d %d\n", __LINE__, is_event_win());
        update_cursor(get_cursor_x() / zoom_factor + zoom_x, 
            get_cursor_y() / zoom_factor + zoom_y);
        flush();
// the raspberry pi accumulates drawing commands after the flush, so we need
// to wait for it to finish
        sync_display();
//printf("MWindow::cursor_motion_event %d\n", __LINE__);
    }
    return 1;
}

int MWindow::cursor_leave_event()
{
//printf("MWindow::cursor_leave_event %d\n", __LINE__);
    hide_cursor();
    flush();
    cursor_entered = 0;
    return 1;
}

int MWindow::cursor_enter_event()
{
    if(is_event_win() &&
        current_operation != IDLE)
    {
//printf("MWindow::cursor_enter_event %d\n", __LINE__);
        update_cursor(get_cursor_x() / zoom_factor + zoom_x,
            get_cursor_y() / zoom_factor + zoom_y);
        flush();
        cursor_entered = 1;
        return 1;
    }
    return 0;
}


void MWindow::draw_brush(VFrame *dst, 
    int x, 
    int y, 
    uint8_t erase_mask,
    uint8_t draw_mask,
    int brush_size)
{
    int dst_w = dst->get_w();
    int dst_h = dst->get_h();
    int x1 = x - brush_size / 2;
    int y1 = y - brush_size / 2;
    int i;
    int j;

    for(i = 0; i < brush_size; i++)
    {
        if(y1 + i >= 0 && y1 + i < dst_h)
        {
            uint8_t *dst_row = (uint8_t*)dst->get_rows()[y1 + i] + x1;
            for(j = 0; j < brush_size; j++)
            {
                if(x1 + j >= 0 && x1 + j < dst_w)
                {
                    *dst_row &= erase_mask;
                    *dst_row |= draw_mask;
                }
                dst_row++;
            }
        }
    }
}

void MWindow::draw_segment(int erase, int x, int y)
{
    if(current_page >= pages.size())
    {
        return;
    }

    Page *page = pages.get(current_page);
    int brush_size;
    if(current_operation != ERASING)
    {
        brush_size = draw_size;
    }
    else
    {
        brush_size = erase_size;
    }

    VFrame *annotations = page->annotations;
    int is_erase = (current_operation == ERASING);
    int x1 = segment_x;
    int x2 = x;
    int y1 = segment_y;
    int y2 = y;
	int x_diff = labs(x2 - x1);
	int y_diff = labs(y2 - y1);
    uint8_t erase_mask;
    uint8_t draw_mask;
    compute_masks(&erase_mask, &draw_mask);

    if(!x_diff && !y_diff)
    {
        draw_brush(annotations, 
            segment_x, 
            segment_y, 
            erase_mask,
            draw_mask,
            brush_size);
    }
    else
    if(x_diff > y_diff)
    {
        if(x2 < x1)
        {
            int temp = x1;
            x1 = x2;
            x2 = temp;
            temp = y1;
            y1 = y2;
            y2 = temp;
        }
        
		int n = y2 - y1;
		int d = x2 - x1;
		for(int i = x1; i <= x2; i++)
		{
			int y = y1 + (int64_t)(i - x1) * (int64_t)n / (int64_t)d;
			draw_brush(annotations, 
                i, 
                y, 
                erase_mask,
                draw_mask,
                brush_size);
		}
    }
    else
    {
        if(y2 < y1)
        {
            int temp = y1;
            y1 = y2;
            y2 = temp;
            temp = x1;
            x1 = x2;
            x2 = temp;
        }
        
		int n = x2 - x1;
		int d = y2 - y1;
		for(int i = y1; i <= y2; i++)
		{
			int x = x1 + (int64_t)(i - y1) * (int64_t)n / (int64_t)d;
			draw_brush(annotations, 
                x, 
                i, 
                erase_mask,
                draw_mask,
                brush_size);
		}
    }

    int x3 = MIN(x1, x2);
    int y3 = MIN(y1, y2);
    int x4 = MAX(x1, x2);
    int y4 = MAX(y1, y2);
    x3 -= brush_size / 2 + 1;
    y3 -= brush_size / 2 + 1;
    x4 += brush_size / 2 + 1;
    y4 += brush_size / 2 + 1;
    CLAMP(x3, 0, root_w);
    CLAMP(x4, 0, root_w);
    CLAMP(y3, 0, root_h);
    CLAMP(y4, 0, root_h);
//printf("MWindow::draw_segment %d\n", __LINE__);
    show_page_fragment(current_page,
        x3,
        y3,
        x4,
        y4,
        0);
//printf("MWindow::draw_segment %d\n", __LINE__);

    segment_x = x;
    segment_y = y;
}

// draw a pixel on the bitmap
void MWindow::draw_pixel(uint8_t **rows, 
    int x, 
    int y, 
    uint8_t erase_mask, 
    uint8_t draw_mask)
{
    CLAMP(x, 0, root_w - 1);
    CLAMP(y, 0, root_h - 1);
    uint8_t *dst_row = rows[y] + x;
    *dst_row &= erase_mask;
    *dst_row |= draw_mask;
}

// draw a vertical line on the bitmap
void MWindow::draw_line(uint8_t **rows, 
    int x, 
    int y1, 
    int y2,
    uint8_t erase_mask, 
    uint8_t draw_mask)
{
    CLAMP(x, 0, root_w - 1);
    CLAMP(y1, 0, root_h);
    CLAMP(y2, 0, root_h);
    for(int i = y1; i <= y2; i++)
    {
        uint8_t *dst_row = rows[i] + x;
        *dst_row &= erase_mask;
        *dst_row |= draw_mask;
    }
}

void MWindow::compute_masks(uint8_t *erase_mask, uint8_t *draw_mask)
{
    if(is_top)
    {
        *erase_mask = 0x0f;
        if(current_operation == ERASING ||
            top_color == 0)
        {
            *draw_mask = 0;
        }
        else
        {
            *draw_mask = top_color << 4;
        }
    }
    else
    {
        *erase_mask = 0xf0;
        if(current_operation == ERASING ||
            bottom_color == 0)
        {
            *draw_mask = 0;
        }
        else
        {
            *draw_mask = bottom_color;
        }
    }
}

void MWindow::finish_box()
{
    if(current_page >= pages.size())
    {
        return;
    }

    Page *page = pages.get(current_page);
    VFrame *annotations = page->annotations;
    uint8_t erase_mask;
    uint8_t draw_mask;
    compute_masks(&erase_mask, &draw_mask);
    
    int x1 = MIN(polygon_x1, get_cursor_x() / zoom_factor + zoom_x);
    int y1 = MIN(polygon_y1, get_cursor_y() / zoom_factor + zoom_y);
    int x2 = MAX(polygon_x1, get_cursor_x() / zoom_factor + zoom_x);
    int y2 = MAX(polygon_y1, get_cursor_y() / zoom_factor + zoom_y);
    CLAMP(x1, 0, root_w);
    CLAMP(x2, 0, root_w);
    CLAMP(y1, 0, root_h);
    CLAMP(y2, 0, root_h);
    uint8_t **rows = (uint8_t**)annotations->get_rows();
    for(int i = y1; i < y2; i++)
    {
        uint8_t *row = rows[i] + x1;
        for(int j = x1; j < x2; j++)
        {
            *row &= erase_mask;
            *row |= draw_mask;
            row++;
        }
    }

    show_page_fragment(current_page,
        x1 - 1,
        y1 - 1,
        x2 + 1,
        y2 + 1,
        0);
}




void MWindow::finish_oval()
{
    if(current_page >= pages.size())
    {
        return;
    }

    Page *page = pages.get(current_page);
    VFrame *annotations = page->annotations;
    uint8_t erase_mask;
    uint8_t draw_mask;

    compute_masks(&erase_mask, &draw_mask);
    
    int x1 = MIN(polygon_x1, get_cursor_x() / zoom_factor + zoom_x);
    int y1 = MIN(polygon_y1, get_cursor_y() / zoom_factor + zoom_y);
    int x2 = MAX(polygon_x1, get_cursor_x() / zoom_factor + zoom_x);
    int y2 = MAX(polygon_y1, get_cursor_y() / zoom_factor + zoom_y);
    int brush_size = this->draw_size;

    int x_axis = (x2 - x1) / 2;
    int y_axis = (y2 - y1) / 2;
    if(current_operation == DRAW_DISC)
    {
        uint8_t **rows = (uint8_t**)annotations->get_rows();
        brush_size = 1;
// center column
        for(int j = y1; j <= y1 + y_axis * 2; j++)
        {
            draw_pixel(rows, 
                x1 + x_axis, 
                j, 
                erase_mask, 
                draw_mask);
        }

        for(int i = 0; i < x_axis; i++)
        {
            int y3 = calculate_oval(i + 1, x_axis, y_axis);
            for(int j = 0; j <= y3; j++)
            {
                draw_pixel(rows, 
                    x1 + i, 
                    y1 + y_axis - j, 
                    erase_mask, 
                    draw_mask);
                draw_pixel(rows, 
                    x1 + i, 
                    y1 + y_axis + j, 
                    erase_mask, 
                    draw_mask);
                draw_pixel(rows, 
                    x2 - 1 - i, 
                    y1 + y_axis - j, 
                    erase_mask, 
                    draw_mask);
                draw_pixel(rows, 
                    x2 - 1 - i, 
                    y1 + y_axis + j, 
                    erase_mask, 
                    draw_mask);
            }
        }
    }
    else
    {
// circle
        uint8_t **rows = (uint8_t**)annotations->get_rows();
        int topleft = brush_size / 2;
        int bottomright = brush_size / 2;
        if(!(brush_size % 2))
        {
            bottomright -= 1;
        }

        if(brush_size == 1)
        {
            for(int i = 0; i < preview_pixels.size(); i += 10)
            {

// left
                int x3 = preview_pixels.get(i);
// top
                int y3 = preview_pixels.get(i + 1);
                int y4 = preview_pixels.get(i + 2);
// bottom
                int y5 = preview_pixels.get(i + 3);
                int y6 = preview_pixels.get(i + 4);
                draw_line(rows, 
                    x3, 
                    y3, 
                    y4,
                    erase_mask, 
                    draw_mask);
                draw_line(rows, 
                    x3, 
                    y5, 
                    y6,
                    erase_mask, 
                    draw_mask);

// right
                x3 = preview_pixels.get(i + 5);
// top
                y3 = preview_pixels.get(i + 6);
                y4 = preview_pixels.get(i + 7);
// bottom
                y5 = preview_pixels.get(i + 8);
                y6 = preview_pixels.get(i + 9);

                draw_line(rows, 
                    x3, 
                    y3, 
                    y4,
                    erase_mask, 
                    draw_mask);
                draw_line(rows, 
                    x3, 
                    y5, 
                    y6,
                    erase_mask, 
                    draw_mask);
            }
        }
        else
        {
// brush_size > 1
            int inner_index = 0;
            for(int outer_index = outer_preview_offset; 
                outer_index < preview_pixels.size();
                outer_index += 10)
            {
// left side
                int x3 = preview_pixels.get(outer_index);
// top
                int y3 = preview_pixels.get(outer_index + 1);
                int y4 = preview_pixels.get(outer_index + 2);
// bottom
                int y5 = preview_pixels.get(outer_index + 3);
                int y6 = preview_pixels.get(outer_index + 4);

// ovals overlap
                if(inner_index < outer_preview_offset &&
                    x3 >= preview_pixels.get(inner_index))
                {
// inner top
                    int y7 = preview_pixels.get(inner_index + 1);
                    int y8 = preview_pixels.get(inner_index + 2);
// inner bottom
                    int y9 = preview_pixels.get(inner_index + 3);
                    int y10 = preview_pixels.get(inner_index + 4);
                    draw_line(rows, 
                        x3, 
                        y3, 
                        y8,
                        erase_mask, 
                        draw_mask);
                    draw_line(rows, 
                        x3, 
                        y9, 
                        y6,
                        erase_mask, 
                        draw_mask);
                }
                else
// outer oval only
                {
                    draw_line(rows, 
                        x3, 
                        y3, 
                        y6,
                        erase_mask, 
                        draw_mask);
                }

// right side
                x3 = preview_pixels.get(outer_index + 5);
// top
                y3 = preview_pixels.get(outer_index + 6);
                y4 = preview_pixels.get(outer_index + 7);
// bottom
                y5 = preview_pixels.get(outer_index + 8);
                y6 = preview_pixels.get(outer_index + 9);


// ovals overlap
                if(inner_index < outer_preview_offset &&
                    x3 <= preview_pixels.get(inner_index + 5))
                {
// inner top
                    int y7 = preview_pixels.get(inner_index + 6);
                    int y8 = preview_pixels.get(inner_index + 7);
// inner bottom
                    int y9 = preview_pixels.get(inner_index + 8);
                    int y10 = preview_pixels.get(inner_index + 9);
                    draw_line(rows, 
                        x3, 
                        y3, 
                        y8,
                        erase_mask, 
                        draw_mask);
                    draw_line(rows, 
                        x3, 
                        y9, 
                        y6,
                        erase_mask, 
                        draw_mask);
                    inner_index += 10;
                }
                else
// outer oval only
                {
                    draw_line(rows, 
                        x3, 
                        y3, 
                        y6,
                        erase_mask, 
                        draw_mask);
                }
            }
        }
    }

    show_page_fragment(current_page,
        x1 - brush_size / 2 - 1,
        y1 - brush_size / 2 - 1,
        x2 + brush_size / 2 + 1,
        y2 + brush_size / 2 + 1,
        0);
}

int MWindow::calculate_oval(int x, int x_axis, int y_axis)
{
    if(x == 0)
    {
        return 0;
    }

    float x_f = (float)x * OVAL_BASE / x_axis;
    int index1 = (int)floor(x_f);
    int index2 = (int)floor(x_f + 1);
// printf("MWindow::calculate_oval %d x_f=%f index1=%d index2=%d\n", 
// __LINE__,
// x_f,
// index1,
// index2);
    CLAMP(index1, 0, OVAL_BASE - 1);
    CLAMP(index2, 0, OVAL_BASE - 1);
    float y1 = oval_table[index1];
    float y2 = oval_table[index2];
    if(index1 == index2)
    {
        return (int)(0.5 + y1 * y_axis);
    }
    else
    {
        return (int)(0.5 + y_axis * (y1 * (x_f - index1) + y2 * (index2 - x_f)));
    }
}

void MWindow::draw_oval_preview(int x1, 
    int y1, 
    int x2, 
    int y2,
    ArrayList<int> *preview_pixels)
{
    int x_axis = (x2 - x1) / 2;
    int y_axis = (y2 - y1) / 2;
    for(int i = 0; i <= x_axis; i++)
    {
        int y3 = calculate_oval(i, x_axis, y_axis);
        int y4 = calculate_oval(i + 1, x_axis, y_axis);
        if(y4 <= y3 + 1)
        {
            y4 = y3;
            draw_pixel_preview(x1 + i, y1 + y_axis - y3);
            draw_pixel_preview(x1 + i, y1 + y_axis + y3);
            draw_pixel_preview(x2 - 1 - i, y1 + y_axis - y3);
            draw_pixel_preview(x2 - 1 - i, y1 + y_axis + y3);
        }
        else
        {
            y4--;
            draw_line_preview(x1 + i, y1 + y_axis - y3, x1 + i, y1 + y_axis - y4);
            draw_line_preview(x1 + i, y1 + y_axis + y3, x1 + i, y1 + y_axis + y4);
            draw_line_preview(x2 - 1 - i, y1 + y_axis - y3, x2 - 1 - i, y1 + y_axis - y4);
            draw_line_preview(x2 - 1 - i, y1 + y_axis + y3, x2 - 1 - i, y1 + y_axis + y4);
        }

// save the pixels for later bitmap drawing
        if(preview_pixels)
        {
            preview_pixels->append(x1 + i);
            preview_pixels->append(y1 + y_axis - y4);
            preview_pixels->append(y1 + y_axis - y3);
            preview_pixels->append(y1 + y_axis + y3);
            preview_pixels->append(y1 + y_axis + y4);

            preview_pixels->append(x2 - 1 - i);
            preview_pixels->append(y1 + y_axis - y4);
            preview_pixels->append(y1 + y_axis - y3);
            preview_pixels->append(y1 + y_axis + y3);
            preview_pixels->append(y1 + y_axis + y4);
        }
    }
}


void MWindow::draw_pixel_preview(int x, int y)
{
    draw_fg_box((x - zoom_x) * zoom_factor, 
        (y - zoom_y) * zoom_factor, 
        zoom_factor, 
        zoom_factor);
}


// draw FG line with zooming
void MWindow::draw_line_preview(int x1, int y1, int x2, int y2)
{
    if(zoom_factor == 1)
    {
        draw_fg_line(x1, 
            y1, 
            x2, 
            y2);
        return;
    }

	int x_diff = labs(x2 - x1);
	int y_diff = labs(y2 - y1);
    if(!x_diff && !y_diff)
    {
        draw_pixel_preview(x1, y1);
    }
    else
    if(x_diff > y_diff)
    {
        if(x2 < x1)
        {
            int temp = x1;
            x1 = x2;
            x2 = temp;
            temp = y1;
            y1 = y2;
            y2 = temp;
        }

		int n = y2 - y1;
		int d = x2 - x1;
		for(int i = x1; i <= x2; i++)
		{
			int y = y1 + (int64_t)(i - x1) * (int64_t)n / (int64_t)d;
            draw_pixel_preview(i, y);
		}
    }
    else
    {
        if(y2 < y1)
        {
            int temp = y1;
            y1 = y2;
            y2 = temp;
            temp = x1;
            x1 = x2;
            x2 = temp;
        }

		int n = x2 - x1;
		int d = y2 - y1;
		for(int i = y1; i <= y2; i++)
		{
			int x = x1 + (int64_t)(i - y1) * (int64_t)n / (int64_t)d;
			draw_pixel_preview(x, i);
		}
    }
}

void MWindow::draw_cursor()
{
    int margin = 5;
    set_line_dashes(0);
    set_color(WHITE);
    set_inverse();


//usleep(100000);

    int brush_size;
    if(current_operation == DRAWING ||
        current_operation == DRAW_LINE ||
        current_operation == DRAW_CIRCLE ||
        current_operation == DRAW_RECT)
    {
        brush_size = draw_size;
    }
    else
    if(current_operation == DRAW_DISC ||
        current_operation == DRAW_BOX)
    {
        brush_size = 1;
    }
    else
    {
        brush_size = erase_size;
    }

// draw the square
    int topleft = brush_size / 2;
    int bottomright = brush_size / 2;
    if(!(brush_size % 2))
    {
        bottomright -= 1;
    }

// draw polygon previews
    if(polygon_state != POLYGON_IDLE)
    {
        switch(current_operation)
        {
            case DRAW_LINE:
            {
                if(brush_size == 1)
                {
                    draw_line_preview(polygon_x1, 
                        polygon_y1, 
                        cursor_x, 
                        cursor_y);
                }
                else
                {
                    int x1 = polygon_x1;
                    int y1 = polygon_y1;
                    int x2 = cursor_x;
                    int y2 = cursor_y;
                    if(x2 < x1)
                    {
                        int temp = x1;
                        x1 = x2;
                        x2 = temp;
                        temp = y1;
                        y1 = y2;
                        y2 = temp;
                    }

                    if(y2 > y1)
                    {
                        draw_line_preview(x1 - topleft, 
                            y1 - topleft, 
                            x1 + bottomright, 
                            y1 - topleft);
                        draw_line_preview(x1 - topleft, 
                            y1 - topleft + 1, 
                            x1 - topleft, 
                            y1 + bottomright);
                        draw_line_preview(x2 + bottomright, 
                            y2 + bottomright - 1, 
                            x2 + bottomright, 
                            y2 - topleft);
                        draw_line_preview(x2 + bottomright, 
                            y2 + bottomright, 
                            x2 - topleft, 
                            y2 + bottomright);
                        draw_line_preview(x1 + bottomright, 
                            y1 - topleft,
                            x2 + bottomright, 
                            y2 - topleft);
                        draw_line_preview(x1 - topleft, 
                            y1 + bottomright,
                            x2 - topleft, 
                            y2 + bottomright);
                    }
                    else
                    {
                        draw_line_preview(x1 - topleft, 
                            y1 + bottomright, 
                            x1 + bottomright, 
                            y1 + bottomright);
                        draw_line_preview(x1 - topleft, 
                            y1 + bottomright - 1, 
                            x1 - topleft, 
                            y1 - topleft);
                        draw_line_preview(x2 + bottomright, 
                            y2 - topleft, 
                            x2 - topleft, 
                            y2 - topleft);
                        draw_line_preview(x2 + bottomright, 
                            y2 - topleft + 1, 
                            x2 + bottomright, 
                            y2 + bottomright);
                        draw_line_preview(x1 - topleft, 
                            y1 - topleft,
                            x2 - topleft, 
                            y2 - topleft);
                        draw_line_preview(x1 + bottomright, 
                            y1 + bottomright,
                            x2 + bottomright, 
                            y2 + bottomright);
                    }
                }
                break;
            }
            
            
            case DRAW_RECT:
            case DRAW_BOX:
                draw_line_preview(polygon_x1, 
                    polygon_y1, 
                    cursor_x, 
                    polygon_y1);
                draw_line_preview(cursor_x, 
                    polygon_y1, 
                    cursor_x, 
                    cursor_y);
                draw_line_preview(cursor_x, 
                    cursor_y, 
                    polygon_x1, 
                    cursor_y);
                draw_line_preview(polygon_x1, 
                    cursor_y, 
                    polygon_x1, 
                    polygon_y1);
                break;
            
            case DRAW_DISC:
            case DRAW_CIRCLE:
            {
                int x1 = MIN(polygon_x1, cursor_x);
                int y1 = MIN(polygon_y1, cursor_y);
                int x2 = MAX(polygon_x1, cursor_x);
                int y2 = MAX(polygon_y1, cursor_y);
                
                if(current_operation == DRAW_CIRCLE)
                {
                    preview_pixels.remove_all();
                    if(brush_size > 1)
                    {
// inner oval 1st
                        draw_oval_preview(x1 + bottomright, 
                            y1 + bottomright, 
                            x2 - bottomright, 
                            y2 - bottomright,
                            &preview_pixels);
                        outer_preview_offset = preview_pixels.size();
                    }
// outer oval
                    draw_oval_preview(x1 - topleft, 
                        y1 - topleft, 
                        x2 + topleft, 
                        y2 + topleft,
                        &preview_pixels);
                }
                else
                {
// disc ignores brush size
                    draw_oval_preview(x1, 
                        y1, 
                        x2, 
                        y2,
                        0);
                }
                break;
            }
        }
    }
    else
    {
// draw the brush
        if(brush_size == 1)
        {
// single pixel
            draw_pixel_preview(cursor_x, cursor_y);
        }
        else
        {
// top side
            draw_line_preview(cursor_x - topleft,
                cursor_y - topleft,
                cursor_x + bottomright,
                cursor_y - topleft);
// bottom side
            draw_line_preview(cursor_x - topleft,
                cursor_y + bottomright,
                cursor_x + bottomright,
                cursor_y + bottomright);
// left side
            if(brush_size > 2)
            {
                draw_line_preview(cursor_x - topleft,
                    cursor_y - topleft + 1,
                    cursor_x - topleft,
                    cursor_y + bottomright - 1);
// right side
                draw_line_preview(cursor_x + bottomright,
                    cursor_y - topleft + 1,
                    cursor_x + bottomright,
                    cursor_y + bottomright - 1);
            }
        }
    }

// the crosshair
    if(polygon_state == POLYGON_IDLE)
    {
        if(zoom_factor == 1)
        {
            set_line_dashes(1);
        }
        draw_line_preview(zoom_x, 
            cursor_y, 
            cursor_x - brush_size / 2 - margin, 
            cursor_y);
        draw_line_preview(cursor_x + brush_size / 2 + margin, 
            cursor_y, 
            zoom_x + get_w() / zoom_factor, 
            cursor_y);
        draw_line_preview(cursor_x,
            zoom_y,
            cursor_x,
            cursor_y - brush_size / 2 - margin);
        draw_line_preview(cursor_x,
            cursor_y + brush_size / 2 + margin,
            cursor_x,
            zoom_y + get_h() / zoom_factor);
    }
    set_opaque();
    set_line_dashes(0);
}

void MWindow::update_cursor(int x, int y)
{
    hide_cursor();

    if(current_operation != IDLE)
    {
// show new cursor position
        cursor_x = x;
        cursor_y = y;
        if(current_operation != IDLE &&
            x >= 0 && 
            y >= 0)
        {
            draw_cursor();
//printf("MWindow::update_cursor %d\n", __LINE__);
            cursor_visible = 1;
        }
    }
}

void MWindow::hide_cursor()
{
    if(cursor_visible)
    {
// hide previous cursor position
        draw_cursor();
//printf("MWindow::hide_cursor %d\n", __LINE__);
        cursor_visible = 0;
        cursor_x = -1;
        cursor_y = -1;
    }
}


void MWindow::enter_drawing(int current_operation)
{
    this->current_operation = current_operation;
    polygon_state = POLYGON_IDLE;
    set_cursor(TRANSPARENT_CURSOR, 0, 0);
}

void MWindow::exit_drawing()
{
    this->current_operation = IDLE;
    polygon_state = POLYGON_IDLE;
    set_cursor(ARROW_CURSOR, 0, 0);
    hide_cursor();
}





