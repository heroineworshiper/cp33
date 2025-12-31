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
#include "capturemidi.h"
#include "cursors.h"
#include "reader.h"
#include "mwindow.h"
#include "reader.h"
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
    CaptureMIDI::instance->stop_recording();
    Reader::instance->do_reader();
    return 1;
}




LoadCapture::LoadCapture(int x, int y)
 : BC_Button(x, y, MWindow::instance->theme->get_image_set("load"))
{
    set_tooltip("Load captured score");
}

int LoadCapture::handle_event()
{
    unlock_window();
    MWindow::instance->load->start();
    lock_window();
    return 1;
}


NewCapture::NewCapture(int x, int y)
 : BC_Button(x, y, MWindow::instance->theme->get_image_set("new"))
{
    set_tooltip("New score");
}

int NewCapture::handle_event()
{
    unlock_window();
    Capture::instance->new_score();
    CaptureMIDI::instance->stop_recording();
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
    if(!Capture::instance->save_score())
        CaptureMenu::instance->update_save();
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
        CaptureMIDI::instance->start_recording();
    }
    else
    {
        MWindow::instance->set_cursor(ARROW_CURSOR, 0, 0);
        MWindow::instance->current_operation = IDLE;
        CaptureMIDI::instance->stop_recording();
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
        MWindow::instance->set_cursor(ARROW_CURSOR, 0, 0);
        MWindow::instance->current_operation = IDLE;
    }
    return 1;
}



Capture8va::Capture8va(int x, int y)
 : BC_Toggle(x, 
    y, 
    MWindow::instance->theme->get_image_set("8va_toggle"), 
    MWindow::instance->current_operation == RECORD_MIDI)
{
    set_tooltip("Draw 8va");
}


int Capture8va::handle_event()
{
    if(MWindow::instance->current_operation != DRAW_8VA_START &&
        MWindow::instance->current_operation != DRAW_8VA_END)
    {
        MWindow::instance->current_operation = DRAW_8VA_START;
        CaptureMenu::instance->update_buttons();
    }
    else
    {
        MWindow::instance->set_cursor(ARROW_CURSOR, 0, 0);
        MWindow::instance->current_operation = IDLE;
    }
    return 1;
}


CaptureRest::CaptureRest(int x, int y)
 : BC_Toggle(x, 
    y, 
    MWindow::instance->theme->get_image_set("rest_toggle"), 
    MWindow::instance->current_operation == CAPTURE_REST)
{
    set_tooltip("Draw rest");
}


int CaptureRest::handle_event()
{
    if(MWindow::instance->current_operation != CAPTURE_REST)
    {
        MWindow::instance->current_operation = CAPTURE_REST;
        CaptureMenu::instance->update_buttons();
    }
    else
    {
        MWindow::instance->set_cursor(ARROW_CURSOR, 0, 0);
        MWindow::instance->current_operation = IDLE;
    }
    return 1;
}


CaptureBar::CaptureBar(int x, int y)
 : BC_Toggle(x, 
    y, 
    MWindow::instance->theme->get_image_set("bar"), 
    MWindow::instance->current_operation == CAPTURE_BAR)
{
    set_tooltip("Draw bar");
}


int CaptureBar::handle_event()
{
    if(MWindow::instance->current_operation != CAPTURE_BAR)
    {
        MWindow::instance->current_operation = CAPTURE_BAR;
        CaptureMenu::instance->update_buttons();
    }
    else
    {
        MWindow::instance->set_cursor(ARROW_CURSOR, 0, 0);
        MWindow::instance->current_operation = IDLE;
    }
    return 1;
}

Start8va::Start8va(int x, int y)
 : BC_Toggle(x, 
    y, 
    MWindow::instance->theme->get_image_set("start_8va"), 
    MWindow::instance->current_operation == RECORD_MIDI)
{
    set_tooltip("Start 8va");
}


int Start8va::handle_event()
{
    if(MWindow::instance->current_operation != DRAW_8VA_START)
    {
        MWindow::instance->current_operation = DRAW_8VA_START;
        CaptureMenu::instance->update_buttons();
    }
    else
    {
        MWindow::instance->set_cursor(ARROW_CURSOR, 0, 0);
        MWindow::instance->current_operation = IDLE;
    }
    return 1;
}

End8va::End8va(int x, int y)
 : BC_Toggle(x, 
    y, 
    MWindow::instance->theme->get_image_set("end_8va"), 
    MWindow::instance->current_operation == RECORD_MIDI)
{
    set_tooltip("End 8va");
}


int End8va::handle_event()
{
    if(MWindow::instance->current_operation != DRAW_8VA_END)
    {
        MWindow::instance->current_operation = DRAW_8VA_END;
        CaptureMenu::instance->update_buttons();
    }
    else
    {
        MWindow::instance->set_cursor(ARROW_CURSOR, 0, 0);
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
        Capture::instance->push_undo_before();
        Score::instance->delete_beat();
//        selection_start -= 1;
//        selection_end = selection_start;
// printf("CaptureErase1::handle_event %d selection_start=%f\n",
// __LINE__, selection_start);
        Capture::instance->push_undo_after(1);
        Capture::instance->draw_score();
    }
    return 1;
}

KeyButton::KeyButton(int x, int y)
 : BC_Toggle(x, y, MWindow::instance->theme->get_image_set("key_toggle"),
    0)
{
    set_tooltip("Draw key signature");
}


int KeyButton::handle_event()
{
    if(MWindow::instance->current_operation != CAPTURE_KEY)
    {
        MWindow::instance->current_operation = CAPTURE_KEY;
        CaptureMenu::instance->update_buttons();
    }
    else
    {
        MWindow::instance->current_operation = IDLE;
    }
    return 1;
}

KeySelector::KeySelector(int x, int y, int w)
 : BC_PopupTextBox(CaptureMenu::instance,
    &CaptureMenu::instance->key_items,
    CaptureMenu::instance->key_items.get(Capture::instance->current_key)->get_text(),
    x,
    y,
    w,
    DP(256))
{
    set_tooltip("Key");
}

int KeySelector::handle_event()
{
    Capture::instance->current_key = get_number();
    MWindow::instance->save_defaults();
    return 1;
}




BarsFollowEdits::BarsFollowEdits(int x, int y)
 : BC_CheckBox(x, 
    y,
    Capture::instance->bars_follow_edits,
    "Shift bars")
{
}

int BarsFollowEdits::handle_event()
{
    Capture::instance->bars_follow_edits = get_value();
    return 1;
}





CaptureMenu::CaptureMenu()
 : BC_Window("Capture Menu", 
    0,
    0,
	MWindow::instance->theme->get_image_set("reader")[0]->get_w() * 4 + 
        MWindow::instance->theme->margin * 2, 
	MWindow::instance->theme->get_image_set("reader")[0]->get_h() * 7 + 
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
    key_items.remove_all_objects();
}

void CaptureMenu::create_objects()
{
    int margin = MWindow::instance->theme->margin;
    int x1 = MWindow::instance->theme->margin;
    int x = x1;
    int y = MWindow::instance->theme->margin;
    BC_SubWindow *window;

    const char* key_titles[] = 
    {
        "C", 
        "D Flat", 
        "D", 
        "E Flat", 
        "E", 
        "F", 
        "F Sharp", 
        "G", 
        "A Flat", 
        "A", 
        "B Flat", 
        "B", 
    };
    for(int i = 0; i < sizeof(key_titles) / sizeof(char*); i++)
        key_items.append(new BC_ListBoxItem(key_titles[i]));

// 4 wide so we can grab the window border
    add_tool(window = new LoadCapture(x, y));
    x += window->get_w();
    add_tool(save = new SaveCapture(x, y));
    x += save->get_w();
    add_tool(window = new Undo(x, y));
    x += window->get_w();
    add_tool(window = new Redo(x, y));
    x += window->get_w();

    x = x1;
    y += window->get_h();
    add_tool(window = new NewCapture(x, y));
    x += window->get_w() * 2;
    add_tool(window = new PrevPage(x, y));
    x += window->get_w();
    add_tool(window = new NextPage(x, y));
    x += window->get_w();

    x = x1;
    y += window->get_h();
    add_tool(record = new CaptureRecord(x, y));
    x += record->get_w();
    add_tool(erase1 = new CaptureErase1(x, y));
    x += erase1->get_w();
    add_tool(erase = new CaptureErase(x, y));
    x += erase->get_w();
    add_tool(key = new KeyButton(x, y));
    x += key->get_w();

    x = x1;
    y += window->get_h();
    key_selector = new KeySelector(x, 
        y, 
        get_w() - margin * 2 - BC_PopupTextBox::calculate_w());
    key_selector->create_objects();

    x = x1;
    y += key_selector->get_h();
    add_tool(start_8va = new Start8va(x, y));
    x += start_8va->get_w();
    add_tool(end_8va = new End8va(x, y));
    x += end_8va->get_w();
    add_tool(rest = new CaptureRest(x, y));
    x += rest->get_w();
    add_tool(bar = new CaptureBar(x, y));

    x = x1;
    y += erase1->get_h();
    add_tool(window = new BarsFollowEdits(x, y));

    x = x1;
    y += window->get_h();



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
    update_buttons();
    reposition_window(x, y);
    raise_window(1);
    unlock_window();
}

void CaptureMenu::update_buttons()
{
    if(MWindow::instance->current_operation != RECORD_MIDI)
        CaptureMIDI::instance->stop_recording();
// hide the cursor for these modes
    if(MWindow::instance->current_operation == ERASE_NOTES)
        MWindow::instance->set_cursor(TRANSPARENT_CURSOR, 0, 0);
    else
        MWindow::instance->set_cursor(ARROW_CURSOR, 0, 0);
    record->set_value(MWindow::instance->current_operation == RECORD_MIDI);
    erase->set_value(MWindow::instance->current_operation == ERASE_NOTES);
    start_8va->set_value(MWindow::instance->current_operation == DRAW_8VA_START);
    end_8va->set_value(MWindow::instance->current_operation == DRAW_8VA_END);
    rest->set_value(MWindow::instance->current_operation == CAPTURE_REST);
    key->set_value(MWindow::instance->current_operation == CAPTURE_KEY);
    bar->set_value(MWindow::instance->current_operation == CAPTURE_BAR);
}

void CaptureMenu::update_save()
{
    put_event([](void *ptr)
    {
        CaptureMenu *menu = CaptureMenu::instance;
        ReaderTheme * theme = MWindow::instance->theme;
        if(score_changed)
            menu->save->set_images(theme->get_image_set("save2"));
        else
            menu->save->set_images(theme->get_image_set("save"));
        menu->save->draw_face(1);
    }, 0);
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











