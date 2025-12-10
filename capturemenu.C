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


#include "capture.h"
#include "capturemenu.h"
#include "reader.h"
#include "mwindow.h"
#include "readermenu.h"
#include "readertheme.h"
#include "score.h"

CaptureMenu* CaptureMenu::instance = 0;

ReaderButton::ReaderButton(int x, int y)
 : BC_Button(x, y, MWindow::instance->theme->get_image_set("reader"))
{
    set_tooltip("Music reader");
}

int ReaderButton::handle_event()
{
    do_reader();
    return 1;
}




LoadCapture::LoadCapture(int x, int y)
 : BC_Button(x, y, MWindow::instance->theme->get_image_set("load"))
{
    set_tooltip("Load captured score");
}

int LoadCapture::handle_event()
{
//    CaptureMenu::instance->hide_window();
    unlock_window();
    MWindow::instance->load->start();
    lock_window();
    return 1;
}



SaveCapture::SaveCapture(int x, int y)
 : BC_Button(x, y, MWindow::instance->theme->get_image_set("save"))
{
    set_tooltip("Save captured score");
}

int SaveCapture::handle_event()
{
    unlock_window();
    if(!save_score())
    {
        lock_window();
        set_images(MWindow::instance->theme->get_image_set("save"));
        draw_face(1);
        score_changed = 0;
    }
    else
        lock_window();
    return 1;
}



CaptureRecord::CaptureRecord(int x, int y)
 : BC_Toggle(x, 
    y, 
    MWindow::instance->theme->get_image_set("draw"), 
    MWindow::instance->current_operation == RECORD_MIDI)
{
    set_tooltip("Capture MIDI");
}


int CaptureRecord::handle_event()
{
    if(MWindow::instance->current_operation != RECORD_MIDI)
    {
        MWindow::instance->current_operation = RECORD_MIDI;
        CaptureMenu::instance->update_buttons();
    }
    else
    {
        MWindow::instance->current_operation = IDLE;
    }
    return 1;
}




CaptureErase::CaptureErase(int x, int y)
 : BC_Toggle(x, 
    y, 
    MWindow::instance->theme->get_image_set("erase"), 
    MWindow::instance->current_operation == ERASE_NOTES)
{
    set_tooltip("Erase objects");
}


int CaptureErase::handle_event()
{
    if(MWindow::instance->current_operation != ERASE_NOTES)
    {
        MWindow::instance->current_operation = ERASE_NOTES;
        CaptureMenu::instance->update_buttons();
    }
    else
    {
        MWindow::instance->current_operation = IDLE;
    }
    return 1;
}

CaptureErase1::CaptureErase1(int x, int y)
 : BC_Button(x, 
    y, 
    MWindow::instance->theme->get_image_set("erase1"))
{
    set_tooltip("Delete beat");
}


int CaptureErase1::handle_event()
{
    if(selection_start > 0)
    {
        Score::instance->delete_beat();
        selection_start -= 1;
        selection_end = selection_start;
        MWindow::instance->draw_score();
    }
    return 1;
}



CaptureMenu::CaptureMenu()
 : BC_Window("Capture Menu", 
    0,
    0,
	MWindow::instance->theme->get_image_set("reader")[0]->get_w() * 4 + 
        MWindow::instance->theme->margin * 2, 
	MWindow::instance->theme->get_image_set("reader")[0]->get_h() * 4 + 
        MWindow::instance->theme->margin * 2,
    -1,
    -1,
    0,
    0,
    1, // hide
    WHITE) // bg_color
{
//printf("CaptureMenu::CaptureMenu %d %p\n", __LINE__, this);
}
CaptureMenu::~CaptureMenu()
{
}

void CaptureMenu::create_objects()
{
    int margin = MWindow::instance->theme->margin;
    int x1 = MWindow::instance->theme->margin;
    int x = x1;
    int y = MWindow::instance->theme->margin;
    BC_SubWindow *window;
    add_tool(window = new LoadCapture(x, y));
    x += window->get_w();
    add_tool(save = new SaveCapture(x, y));
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


    add_tool(record = new CaptureRecord(x, y));
    x += record->get_w();
    add_tool(erase = new CaptureErase(x, y));
    x += erase->get_w();

    y += window->get_h();
    x = x1;
    add_tool(erase1 = new CaptureErase1(x, y));
    x += erase1->get_w();

    y += erase1->get_h();
    x = x1;
    add_tool(window = new ReaderButton(x, y));
    x += window->get_w();
}

int CaptureMenu::close_event()
{
//    hide_window();
    return 1;
}

void CaptureMenu::show()
{
    MWindow::instance->lock_window();
    int x = MenuWindow::instance->get_x() +
        get_resources()->get_left_border();
    int y = MenuWindow::instance->get_y() +
        get_resources()->get_top_border();
//    int x = MWindow::instance->get_abs_cursor_x(0);
//    int y = MWindow::instance->get_abs_cursor_y(0);
    if(x + get_w() > MWindow::instance->get_w())
    {
        x = MWindow::instance->get_w() - get_w();
    }
    if(y + get_h() > MWindow::instance->get_h())
    {
        y = MWindow::instance->get_h() - get_h();
    }
    if(x < 0)
    {
        x = 0;
    }

    if(y < 0)
    {
        y = 0;
    }
    MWindow::instance->unlock_window();

    lock_window();
    show_window();
    reposition_window(x, y);
    raise_window(1);
    unlock_window();

}

void CaptureMenu::update_buttons()
{
    record->set_value(MWindow::instance->current_operation == RECORD_MIDI);
    erase->set_value(MWindow::instance->current_operation == ERASE_NOTES);
}








CaptureThread::CaptureThread()
 : Thread()
{
}

void CaptureThread::create_objects()
{
	CaptureMenu::instance = new CaptureMenu;
    CaptureMenu::instance->create_objects();
}

void CaptureThread::run()
{
    CaptureMenu::instance->run_window();
}











