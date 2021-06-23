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


#ifndef READERWINDOW_H
#define READERWINDOW_H

#include "guicast.h"
#include "reader.inc"


class MWindow : public BC_Window
{
public:
    MWindow();

    static void init_colors();
    void create_objects();
    void show_page(int number, int lock_it);
    void show_page_fragment(int number,
        int x1,
        int y1,
        int x2,
        int y2,
        int lock_it);

    int keypress_event();
    int button_release_event();
    int button_press_event();
    int cursor_motion_event();
    int cursor_leave_event();
    int cursor_enter_event();
    void load_defaults();
    void save_defaults();
    void toggle_zoom();
    void show_error(char *text);
    void draw_cursor();
    void update_cursor(int x, int y);
    void hide_cursor();
    void enter_drawing(int current_operation);
    void exit_drawing();
// draw a line with the brush
    void draw_segment(int erase, int cursor_x, int cursor_y);
// draw a single brush position
    void draw_brush(VFrame *dst, 
        int x, 
        int y, 
        uint8_t erase_mask,
        uint8_t draw_mask,
        int brush_size);
    int calculate_oval(int x, int x_axis, int y_axis);
    void draw_oval_preview(int x1, 
        int y1, 
        int x2, 
        int y2, 
        ArrayList<int> *preview_pixels);
    void draw_line_preview(int x1, int y1, int x2, int y2);
    void draw_pixel_preview(int x, int y);
    void finish_box();
    void finish_oval();
    void draw_pixel(uint8_t **rows, 
        int x, 
        int y, 
        uint8_t erase_mask, 
        uint8_t draw_mask);
    void draw_line(uint8_t **rows, 
        int x, 
        int y1, 
        int y2,
        uint8_t erase_mask, 
        uint8_t draw_mask);
    void compute_masks(uint8_t *erase_mask, uint8_t *draw_mask);

    void reset_undo();
    void push_undo_before();
    void push_undo_after();
    void pop_undo();
    void pop_redo();
    void update_save();

    int root_w;
    int root_h;
// the current layer
    int is_top;
// index of color in table
    int top_color;
    int bottom_color;
// brush sizes
    int draw_size;
    int erase_size;
    int is_hollow;
    int current_operation;
    int cursor_visible;
// motion & leave events are out of order because of event compression
    int cursor_entered;
    int cursor_x;
    int cursor_y;
// end of last segment
    int segment_x;
    int segment_y;
// start of polygon
    int polygon_x1;
    int polygon_y1;
    int polygon_state;
#define POLYGON_IDLE 0
#define POLYGON_CLICK1 1
#define POLYGON_CLICK2 2

    int current_undo;
    int total_undos;
    VFrame *undo_before[UNDO_LEVELS];
    VFrame *undo_after[UNDO_LEVELS];

    MenuThread *menu;
    LoadFileThread *load;
    ReaderTheme *theme;
    BC_Hash *defaults;
    static MWindow *mwindow;

    static const uint32_t top_colors[TOTAL_COLORS];
    static const uint32_t bottom_colors[TOTAL_COLORS];
// color palettes broken down into different formats for faster rendering
    static uint8_t top_rgb888[(TOTAL_COLORS + 1) * 3];
    static uint8_t bottom_rgb888[(TOTAL_COLORS + 1) * 3];
    static uint16_t top_rgb565[TOTAL_COLORS + 1];
    static uint16_t bottom_rgb565[TOTAL_COLORS + 1];
#define OVAL_BASE 1024
    float oval_table[OVAL_BASE];
// pixels from the last preview to be used for the final polygon
    ArrayList<int> preview_pixels;
    int outer_preview_offset;
// zoom top left
    int zoom_x;
    int zoom_y;
// 1 -> 1:1
// 3 -> 3:1
// need a multiple of 3 so RGB can each be a pixel
    int zoom_factor;


// start of drag
    int drag_x;
    int drag_y;
    int drag_zoom_x;
    int drag_zoom_y;
// accumulated drag motion
    int drag_accum_x;
    int drag_accum_y;
    int dragging;
};



#endif


