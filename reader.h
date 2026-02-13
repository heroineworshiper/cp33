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


#ifndef READER_H
#define READER_H


#include "capture.inc"
#include "guicast.h"
#include "reader.inc"
#include <string>



#define ROOT_W 768
#define ROOT_H 1366
//#define ROOT_W 1080
//#define ROOT_H 1920

// where files are stored
#define READER_PATH "/reader/"
#define READER_SUFFIX ".reader.gz"
#define ANNOTATION_SUFFIX ".annotate.gz"

// undo levels for annotations
#define UNDO_LEVELS 10
// undo levels for notation entry
#define CAPTURE_UNDOS 100

// use double buffering
#define BG_PIXMAP

//#define CLIENT_ADDRESS "pi2"
//#define CLIENT_PORT 1234
#define PACKET_SIZE 1024
// timeout in ms
//#define TIMEOUT 1000
#define TIMEOUT 1000
#define RETRIES 5

// command IDs
#define LOAD_FILE 0
#define SHOW_PAGE 1
#define LOAD_ANNOTATIONS 2

// button GPIOs
#define UP_GPIO 14
#define DOWN_GPIO 13
#define DEBOUNCE 1
#define BUTTON_HZ 60
#define REPEAT1 (BUTTON_HZ / 2)
// as fast as the drawing routine
#define REPEAT2 (1)


// operations
#define IDLE 0
#define DRAWING 1
#define ERASING 2
#define DRAW_LINE 3
#define DRAW_CIRCLE 4
#define DRAW_DISC 5
#define DRAW_RECT 6
#define DRAW_BOX 7
#define RECORD_MIDI 8
#define ERASE_NOTES 9
#define DRAW_8VA_START 10
#define DRAW_8VA_END 11
#define CAPTURE_REST 12
#define CAPTURE_KEY 13
#define CAPTURE_BAR 14




extern int client_mode;
extern char reader_path[BCTEXTLEN];
extern int file_changed;
extern int current_page;



class Reader
{
public:
    Reader();

    int save_annotations();
    int save_annotations_entry();
    int load_annotations();
    int load_file(const char *path);
    void load_file_entry(const char *path);
    int send_command(int id, uint8_t *value, int value_size);
    int wait_command();
    void prev_page(int step, int lock_it);
    void next_page(int step, int lock_it);
    void do_reader();
    
    static Reader *instance;

// export parameters
    std::string export_path;
    int export1;
    int export2;

// editing mode
    static int mode;
#define READER_MODE 0
#define CAPTURE_MODE 1


// reader display configurations
    static int display_mode;
#define ONE_PAGE 0 // 1 page on 1 monitor
#define TWO_PAGE 1 // 2 page mode using the client
#define ONE_SCREEN 2 // 2 pages on 1 monitor
};



class Page
{
public:
    Page();
    ~Page();
    
    int x1, y1, x2, y2;
    string path;
    VFrame *image;
    VFrame *annotations;
};

extern ArrayList<Page*> pages;
extern int grey;

#endif






