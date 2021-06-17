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
#include "readerbrushes.h"
#include "readermenu.h"
#include "readertheme.h"
#include "readerwindow.h"


MWindow* MWindow::mwindow = 0;
const uint32_t MWindow::top_colors[TOTAL_COLORS] = 
{
    0x000000, // black
    0x808080, // grey
    0xff0000, // red
    0xcc44cc, // purple
    0x00ff00, // green
    0x0000ff, // blue
    0x664400, // brown
};

const uint32_t MWindow::bottom_colors[TOTAL_COLORS] = 
{
    0xeeee77, // yellow
    0xbbbbbb, // light grey
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
    current_page = 0;
    current_operation = IDLE;
    cursor_visible = 0;
    current_undo = 0;
    total_undos = 0;

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
        to_rgb888(&top_rgb888[3 + i * 3], top_colors[i]);
        to_rgb888(&bottom_rgb888[3 + i * 3], bottom_colors[i]);
        to_rgb565(&top_rgb565[1 + i], top_colors[i]);
        to_rgb565(&bottom_rgb565[1 + i], bottom_colors[i]);
    }
}


void MWindow::create_objects()
{
    load_defaults();

    theme = new ReaderTheme;
    theme->initialize();
    get_resources()->bg_color = WHITE;

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
    sprintf(get_resources()->filebox_filter, "*.reader");

    BC_WindowBase::load_defaults(defaults);
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

// draw full page without annotations
void MWindow::show_page(int number, int lock_it)
{
    show_page_fragment(number,
        0,
        0,
        root_w,
        root_h,
        lock_it);
    return;

#if 0
    if(number >= pages.size())
    {
        lock_window();
        clear_box(0, 0, root_w, root_h, 0);
        flash();
        unlock_window();
        return;
    }


    Page *page = pages.get(number);
    if(lock_it)
    {
        lock_window();
    }
    BC_Bitmap *bitmap = get_temp_bitmap(root_w,
        root_h,
        get_color_model());
//printf("MWindow::show_page %d color_model=%d\n", __LINE__, get_color_model());

// convert to display color model
    switch(get_color_model())
    {
        case BC_BGR8888:
            for(int i = 0; i < root_h; i++)
            {
                uint8_t *src_row = page->image->get_rows()[i];
                uint8_t *dst_row = bitmap->get_row_pointers()[i];
                for(int j = 0; j < root_w; j++)
                {
                    uint8_t src_value = *src_row++;
                    uint8_t r = 0;
                    uint8_t g = 0;
                    uint8_t b = 0;
    // bit mask to RGB
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

                    dst_row[0] = b;
                    dst_row[1] = g;
                    dst_row[2] = r;
                    dst_row += 4;
                }
            }
            break;

        case BC_RGB565:
        {
            for(int i = 0; i < root_h; i++)
            {
                uint8_t *src_row = page->image->get_rows()[i];
                uint16_t *dst_row = (uint16_t*)bitmap->get_row_pointers()[i];
                for(int j = 0; j < root_w; j++)
                {
// bit mask to RGB
                    *dst_row++ = rgb565_table[*src_row++];
                }
            }
            break;
        }

        case BC_RGB8:
        {
            for(int i = 0; i < root_h; i++)
            {
                uint8_t *src_row = page->image->get_rows()[i];
                uint8_t *dst_row = bitmap->get_row_pointers()[i];
                for(int j = 0; j < root_w; j++)
                {
// bit mask to RGB
                    *dst_row++ = rgb8_table[*src_row++];
                }
            }
            break;
        }

        default:
            printf("MWindow::show_page %d unknown color model %d\n",
                __LINE__,
                get_color_model());
            break;
    }

    draw_bitmap(bitmap, 0);
#ifdef BG_PIXMAP
    flash();
#endif
    if(lock_it)
    {
        unlock_window();
    }
#endif // 0
}

// draw part of a page with annotations
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
                uint8_t *dst_row = bitmap->get_row_pointers()[i] + x1 * 4;
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

                    dst_row[0] = b;
                    dst_row[1] = g;
                    dst_row[2] = r;
                    dst_row += 4;
                }
            }
            break;

        case BC_RGB565:
        {
            for(int i = y1; i < y2; i++)
            {
                uint8_t *src_row = page->image->get_rows()[i] + x1;
                uint8_t *annotation_row = page->annotations->get_rows()[i] + x1;
                uint16_t *dst_row = (uint16_t*)bitmap->get_row_pointers()[i] + x1;
                for(int j = x1; j < x2; j++)
                {
                    uint8_t src_value = *src_row++;
                    uint8_t annotation_value = *annotation_row++;
// foreground layer
                    if((annotation_value & 0xf0))
                    {
                        *dst_row++ = top_rgb565[annotation_value >> 4];
                    }
                    else
                    if(src_value != 0x7)
                    {
// source bit mask to RGB
                        *dst_row++ = rgb565_table[src_value];
                    }
                    else
// background layer
                    if((annotation_value & 0x0f))
                    {
                        *dst_row++ = bottom_rgb565[(annotation_value & 0x0f)];
                    }
                    else
                    {
// white
                        *dst_row++ = 0xffff;
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
    draw_bitmap(bitmap, 
        0,
        x1,
        y1,
        x2 - x1,
        y2 - y1,
        x1,
        y1,
        x2 - y1,
        y2 - y1,
        0);
    flash(x1, y1, x2 - x1, y2 - y1, 0);
    update_cursor(cursor_x2, cursor_y2);
    flush();
    if(lock_it)
    {
        unlock_window();
    }
}

// debugging only
int MWindow::keypress_event()
{
    switch(get_keypress())
    {
        case LEFT:
            prev_page(2, 0);
            break;

        case RIGHT:
            next_page(2, 0);
            break;

        case ESC:
            set_done(1);
            break;
    }

    return 1;
}

int MWindow::button_press_event()
{
// raise the menu
    if(!menu->gui->get_hidden() &&
        current_operation != IDLE)
    {
        menu->gui->lock_window();
        menu->gui->raise_window(1);
        menu->gui->unlock_window();
    }

// start a line segment
    switch(current_operation)
    {
        case DRAWING:
            push_undo_before();
            segment_x = get_cursor_x();
            segment_y = get_cursor_y();
            draw_segment(0);
            return 1;
            break;
        
        case ERASING:
            push_undo_before();
            segment_x = get_cursor_x();
            segment_y = get_cursor_y();
            draw_segment(1);
            return 1;
            break;
        
        default:
            return 0;
            break;
    }
}


int MWindow::button_release_event()
{
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
    if(menu->gui->get_hidden())
    {
// show the menu
//printf("MWindow::button_release_event %d %d %d\n", __LINE__, x, y);
        
        menu->gui->lock_window();
        menu->gui->show_window();
// have to rehide buttons after show_window
        menu->gui->update_buttons();
// have to reposition after showing
        menu->gui->reposition_window(x, y);
        menu->gui->raise_window(1);
        menu->gui->unlock_window();
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
    switch(current_operation)
    {
        case DRAWING:
//printf("MWindow::cursor_motion_event %d\n", __LINE__);
            if(get_button_down())
            {
                draw_segment(0);
            }
            need_cursor = 1;
            flush();
            break;
        
        case ERASING:
//printf("MWindow::cursor_motion_event %d\n", __LINE__);
            if(get_button_down())
            {
                draw_segment(1);
            }
            need_cursor = 1;
            break;
    }
    
    if(need_cursor)
    {
        update_cursor(get_cursor_x(), get_cursor_y());
//printf("MWindow::cursor_motion_event %d\n", __LINE__);
        flush();
//printf("MWindow::cursor_motion_event %d\n", __LINE__);
    }
    return 1;
}

int MWindow::cursor_leave_event()
{
//printf("MWindow::cursor_leave_event %d\n", __LINE__);
    hide_cursor();
    flush();
    return 1;
}

int MWindow::cursor_enter_event()
{
//     printf("MWindow::cursor_enter_event %d %d %d %d\n", 
//         __LINE__, 
//         is_event_win(),
//         get_cursor_x(), 
//         get_cursor_y());
    if(is_event_win() &&
        (current_operation == DRAWING ||
        current_operation == ERASING))
    {
        update_cursor(get_cursor_x(), get_cursor_y());
        flush();
        return 1;
    }
    return 0;
}


void MWindow::draw_brush(VFrame *brush, 
    VFrame *dst, 
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
            uint8_t *src_row = (uint8_t*)brush->get_rows()[i];
            for(j = 0; j < brush_size; j++)
            {
                if(x1 + j >= 0 && x1 + j < dst_w)
                {
                    if(*src_row)
                    {
                        *dst_row &= erase_mask;
                        *dst_row |= draw_mask;
                    }
                }
                dst_row++;
                src_row++;
            }
        }
    }
}

void MWindow::draw_segment(int erase)
{
    if(current_page >= pages.size())
    {
        return;
    }

    Page *page = pages.get(current_page);
    Brush *brush;
    int brush_size;
    if(current_operation == DRAWING)
    {
        brush_size = draw_size;
    }
    else
    {
        brush_size = erase_size;
    }
    if(is_hollow &&
        current_operation == DRAWING)
    {
        brush = outline_brushes.get(brush_size - 1);
    }
    else
    {
        brush = solid_brushes.get(brush_size - 1);
    }


    VFrame *brush_image = brush->image;
    VFrame *annotations = page->annotations;
    int is_erase = (current_operation == ERASING);
    int x1 = segment_x;
    int x2 = get_cursor_x();
    int y1 = segment_y;
    int y2 = get_cursor_y();
	int x_diff = labs(x2 - x1);
	int y_diff = labs(y2 - y1);
    uint8_t erase_mask;
    uint8_t draw_mask;

    if(is_top)
    {
        erase_mask = 0x0f;
        if(current_operation == ERASING)
        {
            draw_mask = 0;
        }
        else
        {
            draw_mask = (top_color + 1) << 4;
        }
    }
    else
    {
        erase_mask = 0xf0;
        if(current_operation == ERASING)
        {
            draw_mask = 0;
        }
        else
        {
            draw_mask = (bottom_color + 1);
        }
    }

    if(!x_diff && !y_diff)
    {
        draw_brush(brush_image, 
            annotations, 
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
			draw_brush(brush_image, 
                annotations, 
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
			draw_brush(brush_image, 
                annotations, 
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

    segment_x = get_cursor_x();
    segment_y = get_cursor_y();
}

void MWindow::draw_cursor()
{
    int margin = 5;
    set_line_dashes(0);
    set_color(WHITE);
    set_inverse();

    Brush *brush;
    Brush *hollow_brush;
    int brush_size;
    if(current_operation == DRAWING)
    {
        brush_size = draw_size;
    }
    else
    {
        brush_size = erase_size;
    }
    if(is_hollow)
    {
        brush = outline_brushes.get(brush_size - 1);
    }
    else
    {
        brush = solid_brushes.get(brush_size - 1);
    }
    hollow_brush = outline_brushes.get(brush_size - 1);

    uint8_t **rows = (uint8_t**)hollow_brush->image->get_rows();
    for(int i = 0; i < brush_size; i++)
    {
        for(int j = 0; j < brush_size; j++)
        {
            if(rows[i][j])
            {
                draw_fg_pixel(cursor_x - brush_size / 2 + j, 
                    cursor_y - brush_size / 2 + i);
            }
        }
    }

    set_line_dashes(1);
    draw_fg_line(0, 
        cursor_y, 
        cursor_x - brush_size / 2 - margin, 
        cursor_y);
    draw_fg_line(cursor_x + brush_size / 2 + margin, 
        cursor_y, 
        get_w(), 
        cursor_y);
    draw_fg_line(cursor_x,
        0,
        cursor_x,
        cursor_y - brush_size / 2 - margin);
    draw_fg_line(cursor_x,
        cursor_y + brush_size / 2 + margin,
        cursor_x,
        get_h());
    set_opaque();
    set_line_dashes(0);
}

void MWindow::update_cursor(int x, int y)
{
    hide_cursor();

// show new cursor position
    cursor_x = x;
    cursor_y = y;
    if((current_operation == ERASING ||
        current_operation == DRAWING) &&
        x >= 0 && 
        y >= 0)
    {
//printf("MWindow::update_cursor %d\n", __LINE__);
        draw_cursor();
        cursor_visible = 1;
    }
}

void MWindow::hide_cursor()
{
    if(cursor_visible)
    {
// hide previous cursor position
//printf("MWindow::hide_cursor %d\n", __LINE__);
        draw_cursor();
        cursor_visible = 0;
        cursor_x = -1;
        cursor_y = -1;
    }
}


void MWindow::enter_drawing(int current_operation)
{
    this->current_operation = current_operation;
    set_cursor(TRANSPARENT_CURSOR, 0, 0);
}

void MWindow::exit_drawing()
{
    this->current_operation = IDLE;
    set_cursor(ARROW_CURSOR, 0, 0);
    hide_cursor();
}





