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

// Score database

#ifndef SCORE_H
#define SCORE_H


#include "arraylist.h"
#include "capture.inc"



// key codes
#define KEY_C 0
#define KEY_DF 1
#define KEY_D 2
#define KEY_EF 3
#define KEY_E 4
#define KEY_F 5
#define KEY_FS 6
#define KEY_G 7
#define KEY_AF 8
#define KEY_A 9
#define KEY_BF 10
#define KEY_B 11

#define TREBLE 0
#define BASS 1

// pitch codes from MIDI
#define MIN_C 0x0c // starting pitch of octave 0

#define MIDDLE_C 0x3c
#define MIDDLE_CS 0x3d
#define MIDDLE_DF 0x3d
#define MIDDLE_D 0x3e
#define MIDDLE_DS 0x3f
#define MIDDLE_EF 0x3f
#define MIDDLE_E 0x40
#define MIDDLE_F 0x41
#define MIDDLE_FS 0x42
#define MIDDLE_GF 0x42
#define MIDDLE_G 0x43
#define MIDDLE_GS 0x44
#define MIDDLE_AF 0x44
#define MIDDLE_A 0x45
#define MIDDLE_AS 0x46
#define MIDDLE_BF 0x46
#define MIDDLE_B 0x47

// all positions in an octave
#define OCTAVE 0x0c
// major scale positions in an octave
#define MAJOR_SCALE 7
// octave number of middle C
#define MIDDLE_OCTAVE ((MIDDLE_C - MIN_C) / OCTAVE)

#define MAX_STAVES 16
#define LINE_SPACING 10
#define CLEFF_SPACING 50 // space between cleffs
#define STAFF_SPACING 80 // space between staffs
#define BEAT_PAD 5


class DrawObject
{
public:
    DrawObject();
    DrawObject(int x, int y, BC_Pixmap *pixmap);

    void set(int x, int y, BC_Pixmap *pixmap);
    int is_accidental();
// maximum X of the pixmap
    int get_x2();
// maximum Y of the pixmap
    int get_y2();

// relative x around the beat
    int x;
// relative y to the staff
    int y;
// endpoint if octave marker
    int x2;
// absolute x & y from the line wrapping & staff calculations
    int abs_x;
    int abs_y;
// parent note for deletion operations
    Note *note;
// pointer to the mwindow object
    BC_Pixmap *pixmap;
};

class Note
{
public:
    Note();
    Note(int accidental,
        int pitch);
    ~Note();

    void reset();
    void copy_from(Note *src);
    int save(FILE *fd);

    int pitch;
    int accidental;
#define NO_ACCIDENTAL 0
#define SHARP 1
#define FLAT 2
#define DOUBLE_SHARP 3
#define DOUBLE_FLAT 4
#define NATURAL 5

// duration in beats
    double duration;
};

class Group
{
public:
    Group();
    Group(double time, int type);
    ~Group();
    void reset();

// sorts the notes
// deletes the note argument if it exists
    void append(Note *note);
    int save(FILE *fd);
    static const char* type_to_text(int type);
    static int text_to_type(const char *type);
    void dump();
// width of all images
    int get_w();
    int get_h();
// minimum x of all images
    int get_x();
    int get_abs_x();
    int get_abs_y();
    void copy_from(Group *src);

// absolute starting time in beats
// playback will need to factor in object type
    double time;
// length in beats for octave shift
    double length;

    int type;
// enums are in the order they should appear in a single beat
#define IS_BAR 0
#define IS_CLEFF 1
#define IS_KEY 2
#define IS_OCTAVE 3
#define IS_CHORD 4
#define IS_REST 5
// cleff
    int cleff;
// key signature
    int key;
// octave shift
    int octave;

// group of notes, sorted from lowest to highest
    ArrayList<Note*> notes;
// values computed during drawing
    ArrayList<DrawObject*> images;
    int advance;
};

class Staff
{
public:
    Staff();
    ~Staff();
    
    int save(FILE *fd);
    double max_beat();
    void copy_from(Staff *src);
    Group* insert_before(double time, Group *new_group);
    void dump();
// reset temporaries for drawing
    void reset(int all);
    ArrayList<Group*> groups;

// values computed during drawing
    int current_cleff;
// groups for last octave marker, cleff, key encountered
    Group *current_8va;
    Group *current_cleff_obj;
    Group *current_key;
// current accidental for each position in the octave
    int accidentals[MAJOR_SCALE];
// range of groups to draw at the current beat, for X alignment
    int beat_start;
    int beat_end;
};

// Coordinate of each beat.  Beat covers all the staves.
class Beat
{
public:
    Beat();
    Beat(double time, int x);

    void reset();

    double time;
// x coordinate on infinitely wide line
    int x;
};

class Line
{
public:
    Line(double start_time, int x1, int x_pad, int staves);

    double start_time;
    double end_time;
// extents of each staff, relative to y1
    ArrayList<int> min_y;
    ArrayList<int> max_y;
// abs y of top line of each staff.  Draw the side bars using this
    ArrayList<int> y1;
// x to subtract from the objects, for line wrapping calculations
    int x1;
// x to add to objects, for replicated objects
    int x_pad;
};




class Score
{
public:
    Score();
    ~Score();
    
    void test();
    void clear();
    int save(FILE *fd);
    void copy_from(Score *src);
    void delete_beat();
    double max_beat();
    int find_delete_object(int cursor_x, 
        int cursor_y, 
        int *staff_n, 
        int *group_n, 
        int *image_n,
        int *x,
        int *y,
        int *w,
        int *h);
// fill gaps with rests.  Sort groups by time.
    void clean();

    void dump();
    void dump_beats();

// the mane score
    static Score *instance;
    ArrayList<Staff*> staves;
// objects computed for drawing
    ArrayList<Beat*> beats;
    ArrayList<Line*> lines;
};






#endif
