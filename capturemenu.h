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

class CaptureMenu : public BC_Window
{
public:
    CaptureMenu();
    ~CaptureMenu();

    void create_objects();
    int close_event();
    void show();
    void update_buttons();

    SaveCapture *save;
    CaptureRecord *record;
    CaptureErase *erase;
    CaptureErase1 *erase1;
    static CaptureMenu *instance;
};


class CaptureThread: public Thread
{
public:
    CaptureThread();
    void create_objects();
    void run();
};






#endif





