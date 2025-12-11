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
#include "readermenu.h"
#include "readertheme.h"
#include "mwindow.h"
#include "capture.h"
#include "clip.h"
#include <string.h>

MenuWindow* MenuWindow::instance = 0;
PaletteWindow* PaletteWindow::palette_window = 0;

LoadFileWindow::LoadFileWindow(int x, int y, char *path)
 : BC_FileBox(x, //x
    y, // y
    path, // init_path
    "Load file", // title
    "Select a file to load") // caption
{
}






LoadFileThread::LoadFileThread()
 : BC_DialogThread()
{
}

BC_Window* LoadFileThread::new_gui()
{
	char default_path[BCTEXTLEN];

	sprintf(default_path, READER_PATH);
	MWindow::instance->defaults->get("DEFAULT_LOADPATH", default_path);


// clamp the window size
    if(MWindow::instance->get_resources()->filebox_w > MWindow::instance->root_w)
    {
        MWindow::instance->get_resources()->filebox_w = MWindow::instance->root_w;
    }
    if(MWindow::instance->get_resources()->filebox_h > MWindow::instance->root_h)
    {
        MWindow::instance->get_resources()->filebox_h = MWindow::instance->root_h;
    }


    int x = MWindow::instance->get_abs_cursor_x(1) - 
        MWindow::instance->get_resources()->filebox_w / 2;
    int y = MWindow::instance->get_abs_cursor_y(1) - 
        MWindow::instance->get_resources()->filebox_h / 2;

    if(x + MWindow::instance->get_resources()->filebox_w > MWindow::instance->root_w)
    {
        x = MWindow::instance->root_w - MWindow::instance->get_resources()->filebox_w;
    }
    if(y + MWindow::instance->get_resources()->filebox_h > MWindow::instance->root_h)
    {
        y = MWindow::instance->root_h - MWindow::instance->get_resources()->filebox_h;
    }
    if(x < 0)
    {
        x = 0;
    }
    if(y < 0)
    {
        y = 0;
    }


	gui = new LoadFileWindow(x, y, default_path);
    gui->get_filters()->remove_all_objects();


    char string[BCTEXTLEN];
    if(mode == READER_MODE)
        sprintf(string, "*%s", READER_SUFFIX);
    else
        sprintf(string, "*.txt");
//printf("LoadFileThread::new_gui %d %s\n", __LINE__, string);

    gui->get_filters()->append(new BC_ListBoxItem(string));
    strcpy(BC_WindowBase::get_resources()->filebox_filter, string);

	gui->create_objects();
	return gui;
}

void LoadFileThread::handle_done_event(int result)
{
// printf("LoadFileThread::handle_done_event %d result=%d path=%s\n", 
// __LINE__, 
// result,
// gui->get_submitted_path());
    gui->lock_window();
    gui->hide_window();
    gui->unlock_window();

    MWindow::instance->defaults->update("DEFAULT_LOADPATH", gui->get_submitted_path());
    MWindow::instance->save_defaults();

    if(result == 0)
    {
        if(mode == READER_MODE)
            ::load_file_entry(gui->get_submitted_path());
        else
            ::load_score(gui->get_submitted_path());
    }
    else
    {
#ifndef BG_PIXMAP
// refresh background
        MWindow::instance->show_page(MWindow::instance->current_page);
#endif
    }
}







PaletteButton::PaletteButton(int x, 
    int y, 
    int is_top, 
    int color, 
    VFrame **data)
 : BC_Button(x, y, data)
{
    this->is_top = is_top;
    this->color = color;
}

int PaletteButton::handle_event()
{
    if(is_top)
    {
        MWindow::instance->top_color = color;
    }
    else
    {
        MWindow::instance->bottom_color = color;
    }
    MenuWindow::instance->update_colors();

    MenuWindow::instance->hide_palette();
    return 1;
}


PaletteWindow::PaletteWindow(MenuWindow *gui,
    int x, 
    int y, 
    int w, 
    int h, 
    BC_Pixmap *bg_pixmap)
 : BC_Popup(gui,
    x,
    y,
    w,
    h,
    WHITE,
    1,
    bg_pixmap)
{
    this->gui = gui;
}
PaletteWindow::~PaletteWindow()
{
}

void PaletteWindow::create_objects(int is_top)
{
    lock_window();
    int margin = MWindow::instance->theme->margin;
    int x = margin;
    int y = margin;
    VFrame ***graphics = 0;
    const uint32_t *colors;
    if(is_top)
    {
        graphics = MWindow::instance->theme->top_colors;
        colors = MWindow::top_colors;
    }
    else
    {
        graphics = MWindow::instance->theme->bottom_colors;
        colors = MWindow::bottom_colors;
    }
    
    for(int i = 0; i < 4; i++)
    {

// printf("PaletteWindow::create_objects %d %p %p %p\n",
// __LINE__,
// graphics[i][0],
// graphics[i][1],
// graphics[i][2]);

        add_subwindow(new PaletteButton(x, 
            y, 
            is_top, 
            i, 
            graphics[i]));
        add_subwindow(new PaletteButton(x, 
            y + graphics[0][0]->get_h(), 
            is_top, 
            i + 4, 
            graphics[i + 4]));
        x += graphics[0][0]->get_w();
    }
    
//dump_windows(0);
    
    show_window();
    set_active_subwindow(this);
    unlock_window();
}

int PaletteWindow::button_press_event()
{
//     printf("PaletteWindow::button_press_event %d event_win=%d x=%d y=%d\n", 
//         __LINE__,
//         is_event_subwin(),
//         get_cursor_x(),
//         get_cursor_y());
    if(!is_event_subwin())
    {
        MenuWindow::instance->hide_palette();
        return 1;
    }
    return 1;
}

int PaletteWindow::button_release_event()
{
//    printf("PaletteWindow::button_release_event %d\n", __LINE__);
    return 1;
}

int PaletteWindow::cursor_motion_event()
{
//    printf("PaletteWindow::cursor_motion_event %d\n", __LINE__);
    return 1;
}


LoadFile::LoadFile(int x, int y)
 : BC_Button(x, y, MWindow::instance->theme->get_image_set("load"))
{
    set_tooltip("Load file...");
}

int LoadFile::handle_event()
{
    unlock_window();
    MWindow::instance->exit_drawing();
    MenuWindow::instance->hide_windows(1);
    MWindow::instance->load->start();
    
    lock_window();
    return 1;
}




Save::Save(int x, int y)
 : BC_Button(x, y, MWindow::instance->theme->get_image_set("save"))
{
    set_tooltip("Save annotations");
}

int Save::handle_event()
{
    start_hourglass();
    if(!save_annotations_entry())
    {
        set_images(MWindow::instance->theme->get_image_set("save"));
        draw_face(1);
        file_changed = 0;
    }
    stop_hourglass();
    return 1;
}



Undo::Undo(int x, int y)
 : BC_Button(x, y, MWindow::instance->theme->get_image_set("undo"))
{
    set_tooltip("Undo");
}

int Undo::handle_event()
{
    unlock_window();
    MWindow::instance->lock_window();
    MWindow::instance->pop_undo();
    MWindow::instance->unlock_window();
    lock_window();
    return 1;
}

Redo::Redo(int x, int y)
 : BC_Button(x, y, MWindow::instance->theme->get_image_set("redo"))
{
    set_tooltip("Redo");
}

int Redo::handle_event()
{
    unlock_window();
    MWindow::instance->lock_window();
    MWindow::instance->pop_redo();
    MWindow::instance->unlock_window();
    lock_window();
    return 1;
}





PrevPage::PrevPage(int x, int y)
 : BC_Button(x, y, MWindow::instance->theme->get_image_set("prev_page"))
{
    set_tooltip("Prev page");
}

int PrevPage::handle_event()
{
    unlock_window();
    prev_page(1, 1);
    lock_window();
    return 1;
}

NextPage::NextPage(int x, int y)
 : BC_Button(x, y, MWindow::instance->theme->get_image_set("next_page"))
{
    set_tooltip("Next page");
}

int NextPage::handle_event()
{
    unlock_window();
    next_page(1, 1);
    lock_window();
    return 1;
}

CaptureButton::CaptureButton(int x, int y)
 : BC_Button(x, y, MWindow::instance->theme->get_image_set("capture"))
{
    set_tooltip("Notation entry");
}

int CaptureButton::handle_event()
{
    do_capture();
    return 1;
}





Draw::Draw(int x, int y)
 : BC_Toggle(x, 
    y, 
    MWindow::instance->theme->get_image_set("draw"), 
    MWindow::instance->current_operation == DRAWING)
{
    set_tooltip("Draw");
}

int Draw::handle_event()
{
    if(get_value())
    {
        MWindow::instance->enter_drawing(DRAWING);
    }
    else
    {
        MWindow::instance->exit_drawing();
    }
    MenuWindow::instance->update_buttons();
    return 1;
}




Erase::Erase(int x, int y)
 : BC_Toggle(x, 
    y, 
    MWindow::instance->theme->get_image_set("erase"), 
    MWindow::instance->current_operation == ERASING)
{
    set_tooltip("Erase");
}

int Erase::handle_event()
{
    if(get_value())
    {
        MWindow::instance->enter_drawing(ERASING);
    }
    else
    {
        MWindow::instance->exit_drawing();
    }
    MenuWindow::instance->update_buttons();
    return 1;
}



DrawLine::DrawLine(int x, int y)
 : BC_Toggle(x, 
    y, 
    MWindow::instance->theme->get_image_set("line"),
    MWindow::instance->current_operation == DRAW_LINE)
{
    set_tooltip("Line");
}

int DrawLine::handle_event()
{
    if(get_value())
    {
        MWindow::instance->enter_drawing(DRAW_LINE);
    }
    else
    {
        MWindow::instance->exit_drawing();
    }
    MenuWindow::instance->update_buttons();
    return 1;
}




Circle::Circle(int x, int y)
 : BC_Toggle(x, 
    y, 
    MWindow::instance->theme->get_image_set("circle"),
    MWindow::instance->current_operation == DRAW_CIRCLE)
{
    set_tooltip("Circle");
}

int Circle::handle_event()
{
    if(get_value())
    {
        MWindow::instance->enter_drawing(DRAW_CIRCLE);
    }
    else
    {
        MWindow::instance->exit_drawing();
    }
    MenuWindow::instance->update_buttons();
    return 1;
}




Disc::Disc(int x, int y)
 : BC_Toggle(x, 
    y, 
    MWindow::instance->theme->get_image_set("disc"),
    MWindow::instance->current_operation == DRAW_DISC)
{
    set_tooltip("Disc");
}

int Disc::handle_event()
{
    if(get_value())
    {
        MWindow::instance->enter_drawing(DRAW_DISC);
    }
    else
    {
        MWindow::instance->exit_drawing();
    }
    MenuWindow::instance->update_buttons();
    return 1;
}


Box::Box(int x, int y)
 : BC_Toggle(x, 
    y, 
    MWindow::instance->theme->get_image_set("box"),
    MWindow::instance->current_operation == DRAW_BOX)
{
    set_tooltip("Disc");
}

int Box::handle_event()
{
    if(get_value())
    {
        MWindow::instance->enter_drawing(DRAW_BOX);
    }
    else
    {
        MWindow::instance->exit_drawing();
    }
    MenuWindow::instance->update_buttons();
    return 1;
}




Top::Top(int x, int y)
 : BC_Button(x, y, MWindow::instance->theme->get_image_set("top_layer"))
{
    set_tooltip("Top layer");
}

int Top::handle_event()
{
    hide_window();
    MWindow::instance->is_top = 0;
    MenuWindow::instance->bottom->show_window();
    MenuWindow::instance->bottom_color->show_window();
    MenuWindow::instance->top_color->hide_window();
    return 1;
}

Bottom::Bottom(int x, int y)
 : BC_Button(x, y, MWindow::instance->theme->get_image_set("bottom_layer"))
{
    set_tooltip("Bottom layer");
}

int Bottom::handle_event()
{
    hide_window();
    MWindow::instance->is_top = 1;
    MenuWindow::instance->top->show_window();
    MenuWindow::instance->bottom_color->hide_window();
    MenuWindow::instance->top_color->show_window();
    return 1;
}




TopColor::TopColor(int x, int y)
 : BC_Button(x, 
    y, 
    MWindow::instance->theme->top_colors[MWindow::instance->top_color])
{
    set_tooltip("Top color");
}

int TopColor::handle_event()
{
    MenuWindow::instance->show_palette(1);
    return 1;
}

BottomColor::BottomColor(int x, int y)
 : BC_Button(x, 
    y, 
    MWindow::instance->theme->bottom_colors[MWindow::instance->bottom_color])
{
    set_tooltip("Bottom color");
}

int BottomColor::handle_event()
{
    MenuWindow::instance->show_palette(0);
    return 1;
}



DrawSize::DrawSize(int x, int y, int w)
 : BC_TumbleTextBox(MenuWindow::instance, 
	MWindow::instance->draw_size,
	1,
	MAX_BRUSH,
	x, 
	y, 
	w)
{
    set_tooltip("Draw size");
}

int DrawSize::handle_event()
{
    MenuWindow::instance->unlock_window();
    MWindow::instance->lock_window();
    int cursor_x = MWindow::instance->cursor_x;
    int cursor_y = MWindow::instance->cursor_y;
    MWindow::instance->hide_cursor();
    int value = atoi(get_text());
    CLAMP(value, 1, MAX_BRUSH);
    MWindow::instance->draw_size = value;
    MWindow::instance->update_cursor(cursor_x, cursor_y);
    MWindow::instance->unlock_window();
    MenuWindow::instance->lock_window();
    return 1;
}



EraseSize::EraseSize(int x, int y, int w)
 : BC_TumbleTextBox(MenuWindow::instance, 
	MWindow::instance->erase_size,
	1,
	MAX_BRUSH,
	x, 
	y, 
	w)
{
    set_tooltip("Erase size");
}

int EraseSize::handle_event()
{
    MenuWindow::instance->unlock_window();
    MWindow::instance->lock_window();
    int cursor_x = MWindow::instance->cursor_x;
    int cursor_y = MWindow::instance->cursor_y;
    MWindow::instance->hide_cursor();
    int value = atoi(get_text());
    CLAMP(value, 1, MAX_BRUSH);
    MWindow::instance->erase_size = value;
    MWindow::instance->update_cursor(cursor_x, cursor_y);
    MWindow::instance->unlock_window();
    MenuWindow::instance->lock_window();
    return 1;
}






MenuWindow::MenuWindow()
 : BC_Window("Reader Menu", 
    0,
    0,
	MWindow::instance->theme->get_image_set("load")[0]->get_w() * 4 + 
        MWindow::instance->theme->margin * 2, 
	MWindow::instance->theme->get_image_set("load")[0]->get_h() * 6 + 
        MWindow::instance->theme->margin * 2,
    -1,
    -1,
    0,
    0,
    1, // hide
    WHITE) // bg_color
{
    bg_pixmap = 0;
}
MenuWindow::~MenuWindow()
{
    delete bg_pixmap;
}

void MenuWindow::create_objects()
{
//    lock_window();
    int margin = MWindow::instance->theme->margin;
    int x1 = MWindow::instance->theme->margin;
    int x = x1;
    int y = MWindow::instance->theme->margin;
    BC_SubWindow *window;
    add_tool(window = new LoadFile(x, y));
    x += window->get_w();
    add_tool(save = new Save(x, y));
    x += save->get_w();
    add_tool(window = new Undo(x, y));
    x += window->get_w();
    add_tool(window = new Redo(x, y));

    y += window->get_h();
    x = x1;
    add_tool(window = new PrevPage(x, y));
    x += window->get_w();
    add_tool(window = new NextPage(x, y));
    x += window->get_w();
    add_tool(line = new DrawLine(x, y));
    x += line->get_w();
    add_tool(circle = new Circle(x, y));

    x = x1;
    y += circle->get_h();
    add_tool(disc = new Disc(x, y));
    x += disc->get_w();
    add_tool(box = new Box(x, y));
    x += box->get_w();
    add_tool(top = new Top(x, y));
    add_tool(bottom = new Bottom(x, y));

    x += top->get_w();
    add_tool(top_color = new TopColor(x, y));
    add_tool(bottom_color = new BottomColor(x, y));



    y += window->get_h();
    x = x1;
    add_tool(draw = new Draw(x, y));
    x += draw->get_w();
    draw_size = new DrawSize(x, 
        y, 
        get_w() - x - margin - BC_Tumbler::calculate_w());
    draw_size->create_objects();

    y += window->get_h();
    x = x1;
    add_tool(erase = new Erase(x, y));
    x += erase->get_w();
    erase_size = new EraseSize(x, 
        y, 
        get_w() - x - margin - BC_Tumbler::calculate_w());
    erase_size->create_objects();

    y += window->get_h();
    x = x1;

    add_tool(window = new CaptureButton(x, y));


//    unlock_window();
}

int MenuWindow::close_event()
{
    MWindow::instance->exit_drawing();
    hide_windows(0);

#ifndef BG_PIXMAP
// refresh
    MWindow::instance->show_page(MWindow::instance->current_page);
#endif
    return 1;
}

void MenuWindow::update_buttons()
{
    draw->set_value(MWindow::instance->current_operation == DRAWING);
    erase->set_value(MWindow::instance->current_operation == ERASING);
    line->set_value(MWindow::instance->current_operation == DRAW_LINE);
    circle->set_value(MWindow::instance->current_operation == DRAW_CIRCLE);
    disc->set_value(MWindow::instance->current_operation == DRAW_DISC);
    box->set_value(MWindow::instance->current_operation == DRAW_BOX);
    if(MWindow::instance->is_top)
    {
        bottom->hide_window();
        bottom_color->hide_window();
    }
    else
    {
        top->hide_window();
        top_color->hide_window();
    }
}

void MenuWindow::show_palette(int top)
{
    if(!PaletteWindow::palette_window)
    {
        int x = get_abs_cursor_x(0);
        int y = get_abs_cursor_y(0);
        int w = MWindow::instance->theme->get_image_set("load")[0]->get_w() * 4 + 
            MWindow::instance->theme->margin * 2;
        int h = MWindow::instance->theme->get_image_set("load")[0]->get_h() * 2 + 
            MWindow::instance->theme->margin * 2;
        if(x + w > MWindow::instance->root_w)
        {
            x = MWindow::instance->root_w - w;
        }
        if(y + h > MWindow::instance->root_h)
        {
            y = MWindow::instance->root_h - h;
        }
        if(!bg_pixmap)
        {
            bg_pixmap = new BC_Pixmap(this, get_w(), get_h());
            draw_9segment(0, 
		        0, 
		        w, 
                h,
		        MWindow::instance->theme->get_image("palette_bg"),
		        bg_pixmap);
        }
        PaletteWindow::palette_window = new PaletteWindow(
            this,
            x,
            y,
            w,
            h,
            bg_pixmap);
        PaletteWindow::palette_window->create_objects(top);
    }
}

void MenuWindow::hide_palette()
{
    if(PaletteWindow::palette_window)
    {
        delete PaletteWindow::palette_window;
        PaletteWindow::palette_window = 0;
    }
}

void MenuWindow::update_colors()
{
    top_color->set_images(MWindow::instance->theme->top_colors[MWindow::instance->top_color]);
    top_color->draw_face(1);
    bottom_color->set_images(MWindow::instance->theme->bottom_colors[MWindow::instance->bottom_color]);
    bottom_color->draw_face(1);
}

void MenuWindow::hide_windows(int lock_it)
{
    if(lock_it)
    {
        lock_window();
    }
    hide_window();
    hide_palette();
    if(lock_it)
    {
        unlock_window();
    }
}


MenuThread::MenuThread()
 : Thread()
{
}

void MenuThread::create_objects()
{
	MenuWindow::instance = new MenuWindow;
    MenuWindow::instance->create_objects();
}

void MenuThread::run()
{
    MenuWindow::instance->run_window();
}















