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

// notation capture

#ifndef CAPTURE_H
#define CAPTURE_H


#include "bcpixmap.inc"
#include "bcwindowbase.inc"
#include "mwindow.inc"
#include "reader.h"
#include "readertheme.inc"
#include "score.inc"

// selection
extern int current_staff;
extern double selection_start;  // beat
extern double selection_end;


extern int current_score_page;
extern char score_path[BCTEXTLEN];
extern int score_changed;




extern int save_score();
extern int load_score(char *path);
extern void prev_beat();
extern void next_beat();
extern void prev_staff();
extern void next_staff();



class CaptureUndo
{
public:
    CaptureUndo();
    ~CaptureUndo();

    Score *score;
    int current_staff;
    double selection_start;
    double selection_end;
};


class Capture
{
public:
    Capture();

    void create_objects();
    void push_undo_before();
    void push_undo_after();
    void pop_undo();
    void pop_redo();
    void draw_score();
    void handle_button_press();
    void process_group(Staff *staff, Group *group);
    void draw_group(Line *line, int y, Staff *staff, Group *group);
    void draw_8va_line(Group *group, int x1, int x2, int y2);


    
    
    static Capture *instance;
    CaptureUndo *undo_before[UNDO_LEVELS];
    CaptureUndo *undo_after[UNDO_LEVELS];
    MWindow *mwindow;
    ReaderTheme *theme;
    BC_Pixmap *sharp;
    BC_Pixmap *flat;
    BC_Pixmap *natural;
    BC_Pixmap *quarter;
    BC_Pixmap *ledger;
    BC_Pixmap *treble;
    BC_Pixmap *bass;
    BC_Pixmap *octave;
};





#endif



