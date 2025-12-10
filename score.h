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



class Note
{
public:
    Note();
    Note(int accidental,
        int pitch);
    ~Note();

    void reset();
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

    void append(Note *note);
    int save(FILE *fd);
    void dump();

// absolute starting time in beats
    double time;
// length in beats for octave shift
    double length;

    int type;
#define IS_CHORD 0
#define IS_CLEFF 1
#define IS_KEY 2
#define IS_BAR 3
#define IS_OCTAVE 4
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
    void dump();
    ArrayList<Group*> groups;

// values computed during drawing
    int current_cleff;
    int current_key;
// last octave marker encountered
    Group *current_octave;
// current accidental for each position in the octave
    int accidentals[MAJOR_SCALE];
// range of groups to draw at the current beat, for X alignment
    int beat_start;
    int beat_end;
// range of groups to draw in the current line
    int line_start;
    int line_end;
// range of y, relative to top line
    int min_y;
    int max_y;
// y of top line
    int y1;
// TODO: possibly a range of groups for the current measure
};

// Coordinate of each beat.  Beat covers all the staves.
class Beat
{
public:
    Beat();
    Beat(double time, int x);

    void reset();

    double time;
// abs coordinates on the screen
    int x;
    int y1;
    int y2;
};

class Score
{
public:
    Score();
    ~Score();
    
    void test();
    int save(FILE *fd);
    void delete_beat();
    void dump();
    double max_beat();
    void dump_beats();

// the mane score
    static Score *instance;
    ArrayList<Staff*> staves;
    ArrayList<Beat*> beats;
};






#endif
