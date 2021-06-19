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
#include "readermenu.h"
#include "readertheme.h"
#include "readerwindow.h"
#include "clip.h"

MenuWindow* MenuWindow::menu_window = 0;
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
	MWindow::mwindow->defaults->get("DEFAULT_LOADPATH", default_path);

//     int x = MWindow::mwindow->get_abs_cursor_x(1);
//     int y = MWindow::mwindow->get_abs_cursor_y(1);
// 
//     if(x + MWindow::mwindow->get_resources()->filebox_w > MWindow::mwindow->root_w)
//     {
//         x = MWindow::mwindow->root_w - MWindow::mwindow->get_resources()->filebox_w;
//     }
//     if(y + MWindow::mwindow->get_resources()->filebox_h > MWindow::mwindow->root_h)
//     {
//         y = MWindow::mwindow->root_h - MWindow::mwindow->get_resources()->filebox_h;
//     }
//     if(x < 0)
//     {
//         x = 0;
//     }
//     if(y < 0)
//     {
//         y = 0;
//     }

    if(MWindow::mwindow->get_resources()->filebox_w > MWindow::mwindow->root_w)
    {
        MWindow::mwindow->get_resources()->filebox_w = MWindow::mwindow->root_w;
    }
    if(MWindow::mwindow->get_resources()->filebox_h > MWindow::mwindow->root_h)
    {
        MWindow::mwindow->get_resources()->filebox_h = MWindow::mwindow->root_h;
    }

	gui = new LoadFileWindow(/* x, y,*/ 0, 0, default_path);
    gui->get_filters()->remove_all_objects();
    char string[BCTEXTLEN];
    sprintf(string, "*%s", READER_SUFFIX);
    gui->get_filters()->append(new BC_ListBoxItem(string));

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

    MWindow::mwindow->defaults->update("DEFAULT_LOADPATH", gui->get_submitted_path());
    MWindow::mwindow->save_defaults();

    if(result == 0)
    {
        ::load_file_entry(gui->get_submitted_path());
    }
    else
    {
#ifndef BG_PIXMAP
// refresh background
        MWindow::mwindow->show_page(MWindow::mwindow->current_page);
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
        MWindow::mwindow->top_color = color;
    }
    else
    {
        MWindow::mwindow->bottom_color = color;
    }
    MenuWindow::menu_window->update_colors();

    MenuWindow::menu_window->hide_palette();
    return 1;
}


PaletteWindow::PaletteWindow(int x, 
    int y, 
    int w, 
    int h, 
    BC_Pixmap *bg_pixmap)
 : BC_Popup(x,
    y,
    w,
    h,
    WHITE,
    1,
    bg_pixmap)
{
}
PaletteWindow::~PaletteWindow()
{
}

void PaletteWindow::create_objects(int is_top)
{
    lock_window();
    int margin = MWindow::mwindow->theme->margin;
    int x = margin;
    int y = margin;
    VFrame ***graphics = 0;
    const uint32_t *colors;
    if(is_top)
    {
        graphics = MWindow::mwindow->theme->top_colors;
        colors = MWindow::top_colors;
    }
    else
    {
        graphics = MWindow::mwindow->theme->bottom_colors;
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
        MenuWindow::menu_window->hide_palette();
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
 : BC_Button(x, y, MWindow::mwindow->theme->get_image_set("load"))
{
    set_tooltip("Load file...");
}

int LoadFile::handle_event()
{
    unlock_window();
    MWindow::mwindow->exit_drawing();
    MenuWindow::menu_window->hide_windows(1);
    MWindow::mwindow->load->start();
    
    lock_window();
    return 1;
}




Save::Save(int x, int y)
 : BC_Button(x, y, MWindow::mwindow->theme->get_image_set("save"))
{
    set_tooltip("Save annotations");
}

int Save::handle_event()
{
    if(!save_annotations_entry())
    {
        set_images(MWindow::mwindow->theme->get_image_set("save"));
        draw_face(1);
        file_changed = 0;
    }
    return 1;
}



Undo::Undo(int x, int y)
 : BC_Button(x, y, MWindow::mwindow->theme->get_image_set("undo"))
{
    set_tooltip("Undo");
}

int Undo::handle_event()
{
    unlock_window();
    MWindow::mwindow->lock_window();
    MWindow::mwindow->pop_undo();
    MWindow::mwindow->unlock_window();
    lock_window();
    return 1;
}

Redo::Redo(int x, int y)
 : BC_Button(x, y, MWindow::mwindow->theme->get_image_set("redo"))
{
    set_tooltip("Redo");
}

int Redo::handle_event()
{
    unlock_window();
    MWindow::mwindow->lock_window();
    MWindow::mwindow->pop_redo();
    MWindow::mwindow->unlock_window();
    lock_window();
    return 1;
}





PrevPage::PrevPage(int x, int y)
 : BC_Button(x, y, MWindow::mwindow->theme->get_image_set("prev_page"))
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
 : BC_Button(x, y, MWindow::mwindow->theme->get_image_set("next_page"))
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





Draw::Draw(int x, int y)
 : BC_Toggle(x, 
    y, 
    MWindow::mwindow->theme->get_image_set("draw"), 
    MWindow::mwindow->current_operation == DRAWING)
{
    set_tooltip("Draw");
}

int Draw::handle_event()
{
    if(get_value())
    {
        MWindow::mwindow->enter_drawing(DRAWING);
    }
    else
    {
        MWindow::mwindow->exit_drawing();
    }
    MenuWindow::menu_window->update_buttons();
    return 1;
}




Erase::Erase(int x, int y)
 : BC_Toggle(x, 
    y, 
    MWindow::mwindow->theme->get_image_set("erase"), 
    MWindow::mwindow->current_operation == ERASING)
{
    set_tooltip("Erase");
}

int Erase::handle_event()
{
    if(get_value())
    {
        MWindow::mwindow->enter_drawing(ERASING);
    }
    else
    {
        MWindow::mwindow->exit_drawing();
    }
    MenuWindow::menu_window->update_buttons();
    return 1;
}



Line::Line(int x, int y)
 : BC_Toggle(x, 
    y, 
    MWindow::mwindow->theme->get_image_set("line"),
    MWindow::mwindow->current_operation == DRAW_LINE)
{
    set_tooltip("Line");
}

int Line::handle_event()
{
    if(get_value())
    {
        MWindow::mwindow->enter_drawing(DRAW_LINE);
    }
    else
    {
        MWindow::mwindow->exit_drawing();
    }
    MenuWindow::menu_window->update_buttons();
    return 1;
}




Circle::Circle(int x, int y)
 : BC_Toggle(x, 
    y, 
    MWindow::mwindow->theme->get_image_set("circle"),
    MWindow::mwindow->current_operation == DRAW_CIRCLE)
{
    set_tooltip("Circle");
}

int Circle::handle_event()
{
    if(get_value())
    {
        MWindow::mwindow->enter_drawing(DRAW_CIRCLE);
    }
    else
    {
        MWindow::mwindow->exit_drawing();
    }
    MenuWindow::menu_window->update_buttons();
    return 1;
}




Disc::Disc(int x, int y)
 : BC_Toggle(x, 
    y, 
    MWindow::mwindow->theme->get_image_set("disc"),
    MWindow::mwindow->current_operation == DRAW_DISC)
{
    set_tooltip("Disc");
}

int Disc::handle_event()
{
    if(get_value())
    {
        MWindow::mwindow->enter_drawing(DRAW_DISC);
    }
    else
    {
        MWindow::mwindow->exit_drawing();
    }
    MenuWindow::menu_window->update_buttons();
    return 1;
}


Box::Box(int x, int y)
 : BC_Toggle(x, 
    y, 
    MWindow::mwindow->theme->get_image_set("box"),
    MWindow::mwindow->current_operation == DRAW_BOX)
{
    set_tooltip("Disc");
}

int Box::handle_event()
{
    if(get_value())
    {
        MWindow::mwindow->enter_drawing(DRAW_BOX);
    }
    else
    {
        MWindow::mwindow->exit_drawing();
    }
    MenuWindow::menu_window->update_buttons();
    return 1;
}




Top::Top(int x, int y)
 : BC_Button(x, y, MWindow::mwindow->theme->get_image_set("top_layer"))
{
    set_tooltip("Top layer");
}

int Top::handle_event()
{
    hide_window();
    MWindow::mwindow->is_top = 0;
    MenuWindow::menu_window->bottom->show_window();
    MenuWindow::menu_window->bottom_color->show_window();
    MenuWindow::menu_window->top_color->hide_window();
    return 1;
}

Bottom::Bottom(int x, int y)
 : BC_Button(x, y, MWindow::mwindow->theme->get_image_set("bottom_layer"))
{
    set_tooltip("Bottom layer");
}

int Bottom::handle_event()
{
    hide_window();
    MWindow::mwindow->is_top = 1;
    MenuWindow::menu_window->top->show_window();
    MenuWindow::menu_window->bottom_color->hide_window();
    MenuWindow::menu_window->top_color->show_window();
    return 1;
}




TopColor::TopColor(int x, int y)
 : BC_Button(x, 
    y, 
    MWindow::mwindow->theme->top_colors[MWindow::mwindow->top_color])
{
    set_tooltip("Top color");
}

int TopColor::handle_event()
{
    MenuWindow::menu_window->show_palette(1);
    return 1;
}

BottomColor::BottomColor(int x, int y)
 : BC_Button(x, 
    y, 
    MWindow::mwindow->theme->bottom_colors[MWindow::mwindow->bottom_color])
{
    set_tooltip("Bottom color");
}

int BottomColor::handle_event()
{
    MenuWindow::menu_window->show_palette(0);
    return 1;
}



DrawSize::DrawSize(int x, int y, int w)
 : BC_TumbleTextBox(MenuWindow::menu_window, 
	MWindow::mwindow->draw_size,
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
    MenuWindow::menu_window->unlock_window();
    MWindow::mwindow->lock_window();
    int cursor_x = MWindow::mwindow->cursor_x;
    int cursor_y = MWindow::mwindow->cursor_y;
    MWindow::mwindow->hide_cursor();
    int value = atoi(get_text());
    CLAMP(value, 1, MAX_BRUSH);
    MWindow::mwindow->draw_size = value;
    MWindow::mwindow->update_cursor(cursor_x, cursor_y);
    MWindow::mwindow->unlock_window();
    MenuWindow::menu_window->lock_window();
    return 1;
}



EraseSize::EraseSize(int x, int y, int w)
 : BC_TumbleTextBox(MenuWindow::menu_window, 
	MWindow::mwindow->erase_size,
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
    MenuWindow::menu_window->unlock_window();
    MWindow::mwindow->lock_window();
    int cursor_x = MWindow::mwindow->cursor_x;
    int cursor_y = MWindow::mwindow->cursor_y;
    MWindow::mwindow->hide_cursor();
    int value = atoi(get_text());
    CLAMP(value, 1, MAX_BRUSH);
    MWindow::mwindow->erase_size = value;
    MWindow::mwindow->update_cursor(cursor_x, cursor_y);
    MWindow::mwindow->unlock_window();
    MenuWindow::menu_window->lock_window();
    return 1;
}






MenuWindow::MenuWindow()
 : BC_Window("Menu", 
    0,
    0,
	MWindow::mwindow->theme->get_image_set("load")[0]->get_w() * 4 + 
        MWindow::mwindow->theme->margin * 2, 
	MWindow::mwindow->theme->get_image_set("load")[0]->get_h() * 5 + 
        MWindow::mwindow->theme->margin * 2,
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
    lock_window();
    int margin = MWindow::mwindow->theme->margin;
    int x1 = MWindow::mwindow->theme->margin;
    int x = x1;
    int y = MWindow::mwindow->theme->margin;
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
    add_tool(line = new Line(x, y));
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

    x += top->get_w() + margin;
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

    unlock_window();
}

int MenuWindow::close_event()
{
    MWindow::mwindow->exit_drawing();
    hide_windows(0);

#ifndef BG_PIXMAP
// refresh
    MWindow::mwindow->show_page(MWindow::mwindow->current_page);
#endif
    return 1;
}

void MenuWindow::update_buttons()
{
    draw->set_value(MWindow::mwindow->current_operation == DRAWING);
    erase->set_value(MWindow::mwindow->current_operation == ERASING);
    line->set_value(MWindow::mwindow->current_operation == DRAW_LINE);
    circle->set_value(MWindow::mwindow->current_operation == DRAW_CIRCLE);
    disc->set_value(MWindow::mwindow->current_operation == DRAW_DISC);
    box->set_value(MWindow::mwindow->current_operation == DRAW_BOX);
    if(MWindow::mwindow->is_top)
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
        int w = MWindow::mwindow->theme->get_image_set("load")[0]->get_w() * 4 + 
            MWindow::mwindow->theme->margin * 2;
        int h = MWindow::mwindow->theme->get_image_set("load")[0]->get_h() * 2 + 
            MWindow::mwindow->theme->margin * 2;
        if(x + w > MWindow::mwindow->root_w)
        {
            x = MWindow::mwindow->root_w - w;
        }
        if(y + h > MWindow::mwindow->root_h)
        {
            y = MWindow::mwindow->root_h - h;
        }
        if(!bg_pixmap)
        {
            bg_pixmap = new BC_Pixmap(this, get_w(), get_h());
            draw_9segment(0, 
		        0, 
		        w, 
                h,
		        MWindow::mwindow->theme->get_image("palette_bg"),
		        bg_pixmap);
        }
        add_subwindow(PaletteWindow::palette_window = new PaletteWindow(
            x,
            y,
            w,
            h,
            bg_pixmap));
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
    top_color->set_images(MWindow::mwindow->theme->top_colors[MWindow::mwindow->top_color]);
    top_color->draw_face(1);
    bottom_color->set_images(MWindow::mwindow->theme->bottom_colors[MWindow::mwindow->bottom_color]);
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
	gui = MenuWindow::menu_window = new MenuWindow;
    gui->create_objects();
}

void MenuThread::run()
{
    gui->run_window();
}















