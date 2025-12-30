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

#ifndef CAPTUREMENU_H
#define CAPTUREMENU_H

#include "guicast.h"
#include "reader.inc"
#include "readermenu.inc"

class CaptureMenu;

class ReaderButton : public BC_Button
{
public:
    ReaderButton(int x, int y);
    int handle_event();
};


class NewCapture : public BC_Button
{
public:
    NewCapture(int x, int y);
    int handle_event();
};

class LoadCapture : public BC_Button
{
public:
    LoadCapture(int x, int y);
    int handle_event();
};

class SaveCapture : public BC_Button
{
public:
    SaveCapture(int x, int y);
    int handle_event();
};



class CaptureRecord : public BC_Toggle
{
public:
    CaptureRecord(int x, int y);
    int handle_event();
};

class Capture8va : public BC_Toggle
{
public:
    Capture8va(int x, int y);
    int handle_event();
};

class CaptureRest : public BC_Toggle
{
public:
    CaptureRest(int x, int y);
    int handle_event();
};

class CaptureBar : public BC_Toggle
{
public:
    CaptureBar(int x, int y);
    int handle_event();
};

class Start8va : public BC_Toggle
{
public:
    Start8va(int x, int y);
    int handle_event();
};

class End8va : public BC_Toggle
{
public:
    End8va(int x, int y);
    int handle_event();
};


class CaptureErase : public BC_Toggle
{
public:
    CaptureErase(int x, int y);
    int handle_event();
};

class CaptureErase1 : public BC_Button
{
public:
    CaptureErase1(int x, int y);
    int handle_event();
};


class KeyButton : public BC_Toggle
{
public:
    KeyButton(int x, int y);
    int handle_event();
};

class BarsFollowEdits : public BC_Toggle
{
public:
    BarsFollowEdits(int x, int y);
    int handle_event();
};

class KeySelector : public BC_PopupTextBox
{
public:
    KeySelector(int x, int y, int w);
    int handle_event();
};

class CaptureMenu : public BC_Window
{
public:
    CaptureMenu();
    ~CaptureMenu();

    void create_objects();
    int close_event();
    void show();
    void update_buttons();
    void update_save();

    SaveCapture *save;
    CaptureRecord *record;
    CaptureErase *erase;
    CaptureErase1 *erase1;
//    Capture8va *draw_8va;
    Start8va *start_8va;
    End8va *end_8va;
    CaptureRest *rest;
    CaptureBar *bar;
    KeyButton *key;
    KeySelector *key_selector;
    ArrayList <BC_ListBoxItem*> key_items;
    static CaptureMenu *instance;
};


class CaptureThread : public Thread
{
public:
    CaptureThread();
    void create_objects();
    void run();
};






#endif





