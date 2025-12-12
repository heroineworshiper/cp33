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


#include "capture.h"
#include "mwindow.h"
#include "readertheme.h"
#include "score.h"
#include <stdint.h>
#include <string.h>

// cursor position
int current_staff = 0; // number of staff or -1 if all
double selection_start = 3;  // beat
double selection_end = 3;
int selection_line = 0;
int cursor_x1 = 0;
int cursor_x2 = 0;
int current_key = KEY_F;
int current_score_page = 0;
char score_path[BCTEXTLEN] = { 0 };
int score_changed = 0;


// test MIDI input
uint8_t e_minor[] =
{
    0x90, 0x40, 0x54, 0x00, // middle E
    0x90, 0x43, 0x6c, 0x00, // middle G
    0x90, 0x47, 0x72, 0x00, // middle B
    0x90, 0x4c, 0x67, 0x00, // E4
    0x90, 0x43, 0x00, 0x00,
    0x90, 0x47, 0x00, 0x00,
    0x90, 0x4c, 0x00, 0x00,
    0x90, 0x40, 0x00, 0x00
};

uint8_t full_range[] =
{
    0x90, 0x15, 0x63, 0x00,  // min A
    0x90, 0x15, 0x00, 0x00,
    0x90, 0x6c, 0x71, 0x00,  // max C
    0x90, 0x6c, 0x00, 0x00
};

// 1 for each note in the octave of a major scale.
// Shift right by the number of the key
const int major_scale[] =
{
//  C  C# D  D# E  F  F# G  G# A  A# B
    1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1
};

// offset in the staff of each key, relative to middle C
const int key_to_staff[] =
{
    0, // KEY_C 
    1, // KEY_DF
    1, // KEY_D 
    2, // KEY_EF
    2, // KEY_E 
    3, // KEY_F 
    3, // KEY_FS
    4, // KEY_G 
    5, // KEY_AF
    5, // KEY_A 
    6, // KEY_BF
    6  // KEY_B
};

// 1 for keys indicated by flat accidentals
int key_is_flat[] = 
{
    0, // KEY_C
    1, // KEY_DF
    0, // KEY_D
    1, // KEY_EF
    0, // KEY_E
    1, // KEY_F
    0, // KEY_FS
    0, // KEY_G
    1, // KEY_AF
    0, // KEY_A
    1, // KEY_BF
    0  // KEY_B
};

// 1 for each note which is flattened or sharpened
uint32_t keysigs[] =
{
//    CDEFGAB
    0b0000000, // KEY_C
    0b0110111, // KEY_DF
    0b1001000, // KEY_D
    0b0010011, // KEY_EF
    0b1101100, // KEY_E
    0b0000001, // KEY_F
    0b1111110, // KEY_FS
    0b0001000, // KEY_G
    0b0110011, // KEY_AF
    0b1001100, // KEY_A
    0b0010001, // KEY_BF
    0b1101110  // KEY_B
};




Capture* Capture::instance = 0;

Capture::Capture()
{
}

void Capture::create_objects()
{
    mwindow = MWindow::instance;
    theme = mwindow->theme;

    for(int i = 0; i < UNDO_LEVELS; i++)
    {
        undo_before[i] = new CaptureUndo();
        undo_after[i] = new CaptureUndo();
    }

    quarter = new BC_Pixmap(mwindow, theme->new_image("quarter.png"), PIXMAP_ALPHA);
    sharp = new BC_Pixmap(mwindow, theme->new_image("sharp.png"), PIXMAP_ALPHA);
    flat = new BC_Pixmap(mwindow, theme->new_image("flat.png"), PIXMAP_ALPHA);
    natural = new BC_Pixmap(mwindow, theme->new_image("natural.png"), PIXMAP_ALPHA);
    ledger = new BC_Pixmap(mwindow, theme->new_image("ledger.png"), PIXMAP_ALPHA);
    treble = new BC_Pixmap(mwindow, theme->new_image("treble.png"), PIXMAP_ALPHA);
    bass = new BC_Pixmap(mwindow, theme->new_image("bass.png"), PIXMAP_ALPHA);
    octave = new BC_Pixmap(mwindow, theme->new_image("8va.png"), PIXMAP_ALPHA);
}




CaptureUndo::CaptureUndo()
{
    score = new Score;
}

CaptureUndo::~CaptureUndo()
{
    delete score;
}







// always centered around X=0
void Capture::process_group(Staff *staff, Group *group)
{
    group->images.remove_all_objects();

    int note_w = quarter->get_w();
    int note_h = quarter->get_h();
    int ledger_w = ledger->get_w();
    switch(group->type)
    {
        case IS_CLEFF:
        {
            staff->current_cleff = group->cleff;
            staff->current_cleff_obj = group;
            if(staff->current_cleff == TREBLE)
                group->images.append(new DrawObject(-treble->get_w() / 2,
                    LINE_SPACING * 2 - treble->get_h() / 2,
                    treble));
            else
                group->images.append(new DrawObject(-bass->get_w() / 2,
                    LINE_SPACING * 2 - bass->get_h() / 2,
                    bass));
            group->advance = BEAT_PAD;
            break;
        }

        case IS_KEY:
        {
            BC_Pixmap *pixmap;
            staff->current_key = group;
            current_key = group->key;
// reset the accidental table
//                 int accidental;
//                 if(key_is_flat[current_key])
//                     accidental = FLAT;
//                 else
//                     accidental = SHARP;
//                 for(int i = 0; i < MAJOR_SCALE; i++)
//                 {
//                     if(keysigs[current_key] & (0b1000000 >> i))
//                         accidentals[i] = accidental;
//                     else
//                         accidentals[i] = NO_ACCIDENTAL;
//                 }

            if(key_is_flat[current_key])
            {
                int y = -flat->get_h() * 2 / 3 + 1;
                int x = 0;

                int flat_order[] = 
                {
                //    CDEFGAB, pixels from top of staff
                    0b0000001, LINE_SPACING * 2,     
                    0b0010000, LINE_SPACING / 2,     
                    0b0000010, LINE_SPACING * 5 / 2, 
                    0b0100000, LINE_SPACING * 1,     
                    0b0000100, LINE_SPACING * 3,     
                    0b1000000, LINE_SPACING * 3 / 2, 
                };






                for(int j = 0; 
                    j < sizeof(flat_order) / sizeof(int); 
                    j += 2)
                {
                    if(keysigs[current_key] & flat_order[j])
                    {
                        int y2 = y + flat_order[j + 1];
                        if(staff->current_cleff == BASS)
                            y2 += LINE_SPACING;
                        group->images.append(new DrawObject(x,
                            y2,
                            flat));
                        x += flat->get_w();
                    }
                }
            }
            else
            {
                int y = -sharp->get_h() / 2;
                int x = 0;

                int sharp_order[] =
                {
                //    CDEFGAB, pixels from top of staff
                    0b0001000, 0,                    
                    0b1000000, LINE_SPACING * 3 / 2, 
                    0b0000100, -LINE_SPACING / 2,    
                    0b0100000, LINE_SPACING,         
                    0b0000010, LINE_SPACING * 5 / 2, 
                    0b0010000, LINE_SPACING / 2,     
                };






                for(int j = 0; 
                    j < sizeof(sharp_order) / sizeof(int); 
                    j += 2)
                {
                    if(keysigs[current_key] & sharp_order[j])
                    {
                        int y2 = y + sharp_order[j + 1];
                        if(staff->current_cleff == BASS)
                            y2 += LINE_SPACING;
                        group->images.append(new DrawObject(x,
                            y2,
                            sharp));
                        x += sharp->get_w();
                    }
                }
            }
            group->advance = BEAT_PAD * 2;
            break;
        }

        case IS_BAR:
// reset accidental table
            for(int j = 0; j < MAJOR_SCALE; j++)
                staff->accidentals[j] = NO_ACCIDENTAL;
            group->advance = BEAT_PAD;
            break;

        case IS_OCTAVE:
        {
            staff->current_8va = group;
            int y;
            if(group->octave < 0)
                y = LINE_SPACING * 4;
            else
                y = -octave->get_h();
// must determine coords after laying all the notes out
            group->images.append(new DrawObject(-octave->get_w() / 2,
                y,
                octave));
            group->advance = BEAT_PAD;
// printf("MWindow::process_group %d current_8va=%d octave_remane=%f\n", 
// __LINE__,
// staff->current_8va,
// staff->octave_remane);
            break;
        }

        case IS_CHORD:
        default:
        {
            int have_8va_marker = 0;
            for(int j = 0; j < group->notes.size(); j++)
            {
                Note *note = group->notes.get(j);

// compute the current octave
                int octave = (note->pitch - MIN_C) / OCTAVE;
// compute the position in the current octave
                int scale_position2 = ((note->pitch - MIN_C) % OCTAVE);
// convert the MIDI code to a position & octave in the current scale
                int scale_position = scale_position2 - current_key;
//printf("MWindow::process_group %d scale_position2=%d scale_position=%d octave=%d\n", 
//__LINE__, scale_position2, scale_position, octave);

                while(scale_position < 0)
                {
                    scale_position += OCTAVE;
                    octave--;
                }
//printf("MWindow::process_group %d scale_position=%d octave=%d\n", 
//__LINE__, scale_position, octave);


// shift for 8va
                if(staff->current_8va &&
                    staff->current_8va->time + staff->current_8va->length > group->time)
                {
                    have_8va_marker = 1;
                    octave -= staff->current_8va->octave;
                }

// compute the accidental
                int need_accidental = 0;
                if(note->accidental)
                {
                    need_accidental = note->accidental;
// adjust scale position based on the reverse accidental to obtain the
// same pitch
                    switch(note->accidental)
                    {
                        case SHARP:
                            scale_position--;
                            break;
                        case FLAT:
                            scale_position++;
                            break;
                    }

                    if(scale_position < 0)
                    {
                        octave--;
                        scale_position += OCTAVE;
                    }
                    else
                    if(scale_position >= OCTAVE)
                    {
                        octave++;
                        scale_position -= OCTAVE;
                    }
                }
                else
                if(!major_scale[scale_position])
                {
// not on the major scale
                    if(key_is_flat[current_key])
                    {
// add a sharp to get on a major note
                        scale_position--;
                        if(scale_position < 0)
                        {
                            octave--;
                            scale_position += OCTAVE;
                        }

// test for a natural in C MAJ
                        if(major_scale[scale_position])
                            need_accidental = NATURAL;
                        else
                            need_accidental = SHARP;
                    }
                    else
                    {
// add a flat to get on a major note
                        scale_position++;
                        if(scale_position >= OCTAVE)
                        {
                            octave++;
                            scale_position -= OCTAVE;
                        }
// test for a natural in C MAJ
                        if(major_scale[scale_position])
                            need_accidental = NATURAL;
                        else
                            need_accidental = FLAT;
                    }
                }

// position in the major scale of the current key
                int major_scale_position = 0;
                for(int k = 0; k < scale_position; k++)
                    if(major_scale[k]) major_scale_position++;
// position in the C MAJ scale
                int cmaj_position = key_to_staff[current_key] + 
                    major_scale_position;
// position in the staff is the current octave start + C MAJ scale position
                int staff_position = cmaj_position +
                    (octave - MIDDLE_OCTAVE) * MAJOR_SCALE;
//printf("MWindow::process_group %d pitch=%d scale_position=%d major_scale_position=%d cmaj_position=%d staff_position=%d\n", 
//__LINE__, note->pitch, scale_position, major_scale_position, cmaj_position, staff_position);

// convert to the position in the accidental table
                cmaj_position %= MAJOR_SCALE;

// hide accidentals which have already been drawn
//                    if(need_accidental == accidentals[cmaj_position])
//                        need_accidental = 0;
//                    else
// update table of accidentals
                if(need_accidental != staff->accidentals[cmaj_position])
                {
//printf("MWindow::process_group %d accidentals[cmaj_position]=%d need_accidental=%d\n", 
//__LINE__, accidentals[cmaj_position], need_accidental);
                    staff->accidentals[cmaj_position] = need_accidental;
// cancel an accidental based on the position in the current scale
                    if(need_accidental == NO_ACCIDENTAL)
                    {
// key signature has an accidental
                        if(keysigs[current_key] & (0b1000000 >> cmaj_position))
                        {
                            if(key_is_flat[current_key])
                                need_accidental = FLAT;
                            else
                                need_accidental = SHARP;
                        }
                        else
// key signature has no accidental
                        {
                            need_accidental = NATURAL;
                        }
                    }
                }

// Y coordinate of the note
                int x = 0;
                int center_y;
                
                if(staff->current_cleff == TREBLE)
                    center_y = LINE_SPACING * 5 - 
                        staff_position * LINE_SPACING / 2;
                else
                    center_y = -LINE_SPACING - 
                        staff_position * LINE_SPACING / 2;


                int y = center_y - quarter->get_h() / 2;

// check for overlap
                for(int k = 0; k < group->images.size(); k++)
                {
                    DrawObject *object = group->images.get(k);
                    if(object->pixmap == quarter &&
                        x < object->x + quarter->get_w() && 
                        y + quarter->get_h() > object->y)
                    {
// printf("MWindow::process_group %d %d %d %d\n",
// __LINE__, y + quarter->get_h(), object->y, object->x + quarter->get_w());
                        x = object->x + quarter->get_w();
                        break;
                    }
                }

// printf("MWindow::process_group %d x=%d y=%d\n", 
// __LINE__, x, y);
                group->images.append(new DrawObject(x, y, quarter));


// ledger lines above staff
                if(center_y < 0)
                {
                    for(int k = LINE_SPACING; k >= y; k -= LINE_SPACING)
                    {
                        group->images.append(new DrawObject(x + note_w / 2 - ledger_w / 2,
                            k,
                            ledger));
                    }
                }
                else
// ledger lines below staff
                if(center_y > LINE_SPACING * 4)
                {
//printf("MWindow::process_group %d y=%d %d\n", __LINE__, y, y1 + LINE_SPACING * 4);
                    for(int k = LINE_SPACING * 5; k <= center_y; k += LINE_SPACING)
                    {
//printf("MWindow::process_group %d k=%d\n", __LINE__, k);
                        group->images.append(new DrawObject(x + note_w / 2 - ledger_w / 2,
                            k,
                            ledger));
                    }
                }

// accidental
                DrawObject *accidental = 0;
                if(need_accidental == FLAT)
                    accidental = new DrawObject(-flat->get_w(),
                        center_y - flat->get_h() * 2 / 3,
                        flat);
                else
                if(need_accidental == SHARP)
                    accidental = new DrawObject(-sharp->get_w(),
                        center_y - sharp->get_h() / 2,
                        sharp);
                else
                if(need_accidental == NATURAL)
                    accidental = new DrawObject(-natural->get_w(),
                        center_y - natural->get_h() / 2,
                        natural);

                if(accidental) 
                {
// check for overlap
                    int done = 0;
                    while(!done)
                    {
                        done = 1;
                        int accidental_x1 = accidental->x;
                        int accidental_y1 = accidental->y;
                        int accidental_x2 = accidental->x + accidental->pixmap->get_w();
                        int accidental_y2 = accidental->y + accidental->pixmap->get_h();
                        for(int k = 0; k < group->images.size(); k++)
                        {
                            DrawObject *object = group->images.get(k);
                            if(object->is_accidental() &&
                                accidental_x2 > object->x &&
                                accidental_x1 < object->get_x2() &&
                                accidental_y2 > object->y &&
                                accidental_y1 < object->get_y2())
                            {
                                done = 0;
// shift right of overlapping object
                                accidental->x = object->get_x2();
                                break;
                            }
                        }
                    }

                    group->images.append(accidental);
                }
            }

// shift all accidentals left of notes
            int max_x = -0x7fffffff;
            for(int j = 0; j < group->images.size(); j++)
            {
                DrawObject *object = group->images.get(j);
                if(object->is_accidental() &&
                    object->get_x2() > max_x)
                    max_x = object->get_x2();
            }

            for(int j = 0; j < group->images.size(); j++)
            {
                DrawObject *object = group->images.get(j);
                if(object->is_accidental())
                    object->x -= max_x;
            }

// shift octave marker
            if(have_8va_marker)
            {
                DrawObject *octave_marker = staff->current_8va->images.get(0);
                for(int j = 0; j < group->images.size(); j++)
                {
                    DrawObject *object = group->images.get(j);
                    
                    if(staff->current_8va->octave < 0)
                    {
                        int y2 = object->get_y2();
                        if(y2 > octave_marker->y)
                            octave_marker->y = y2;
                    }
                    else
                    {
                        int y1 = object->y;
                        if(y1 < octave_marker->get_y2())
                            octave_marker->y = y1 - octave_marker->pixmap->get_h();
                    }
                }
            }

// left justify all objects
//             int min_x = 0x7fffffff;
//             for(int j = 0; j < group->images.size(); j++)
//             {
//                 DrawObject *object = group->images.get(j);
//                 if(object->x < min_x)
//                     min_x = object->x;
//             }
//             for(int j = 0; j < group->images.size(); j++)
//             {
//                 DrawObject *object = group->images.get(j);
//                 object->x -= min_x;
//             }
            group->advance = BEAT_PAD;
            break;
        }
    }
}

void Capture::draw_group(Line *line, int y, Staff *staff, Group *group)
{
    for(int k = 0; k < group->images.size(); k++)
    {
        DrawObject *image = group->images.get(k);
        mwindow->draw_pixmap(image->pixmap,
            image->x - line->x1 + line->x_pad,
            image->y + y);
    }

    switch(group->type)
    {
        case IS_CLEFF:
            staff->current_cleff = group->cleff;
            staff->current_cleff_obj = group;
            break;
        case IS_KEY:
            staff->current_key = group;
            break;
        case IS_OCTAVE:
        {
            staff->current_8va = group;
// draw octave line
            DrawObject *image = group->images.get(0);
            int y2 = image->y + y + image->pixmap->get_h() / 2;
            int x1 = image->x - line->x1 + line->x_pad + image->pixmap->get_w();
            int x2 = image->x2 - line->x1;
            
            draw_8va_line(group, x1, x2, y2);
            break;
        }
    }
}

void Capture::draw_8va_line(Group *group, int x1, int x2, int y2)
{
    int direction = -1;
    if(group->octave > 0) direction = 1;
    mwindow->set_line_dashes(2);
    mwindow->set_line_width(2);
    mwindow->BC_Window::draw_line(
        x1,
        y2,
        x2,
        y2);
    mwindow->BC_Window::draw_line(
        x2,
        y2,
        x2,
        y2 + 5 * direction);
    mwindow->set_line_dashes(0);
    mwindow->set_line_width(1);
}


void Capture::draw_score()
{
    Score *score = Score::instance;
    int margin = 5;
    char temp[BCTEXTLEN];
// reset the temporaries
    score->beats.remove_all_objects();
    score->lines.remove_all_objects();
// 1st beat
    score->beats.append(new Beat(0, 0));

    mwindow->lock_window();
// finale background color
    mwindow->set_color(0xfdfcf5);
    mwindow->draw_box(0, 0, mwindow->root_w, mwindow->root_h, 0);
    mwindow->set_color(BLACK);

// draw the title
    if(score_path[0] == 0)
        sprintf(temp, "Untitled");
    else
        strcpy(temp, score_path);
    mwindow->set_font(SMALLFONT);
    mwindow->draw_text(margin, 
        margin + mwindow->get_text_ascent(SMALLFONT), temp);


// start of current line
    int y1 = 0;


// pass 1: determine the extents
    double current_time = 0;
    double prev_time = 0;

    Line *line = 0;
    int new_line = 1;
// current position relative to the infinitely wide line
    int x = 0;
    int prev_x = x;
// extents of the line
    int x1 = margin;
    int x2 = mwindow->root_w - margin;
// available pixels in the current line
    int line_w = x2 - x1;

// initialize the staves
    for(int i = 0; i < score->staves.size(); i++)
    {
        Staff *staff = score->staves.get(i);
        staff->reset();
    }

    cursor_x1 = cursor_x2 = -1;

// advance through all the staves at the same time
// until we hit the end of the score
    while(1)
    {
        double next_time = 0x7fffffff;
        int got_next = 0;

        if(new_line)
        {
            new_line = 0;
// adjust the start & end of the line after line 1
            int cleff_w = 0;
            int key_w = 0;
            if(score->lines.size() > 0)
            {
                x1 = margin;
                x2 = mwindow->root_w - margin;

                for(int i = 0; i < score->staves.size(); i++)
                {
                    Staff *staff = score->staves.get(i);
                    if(staff->current_cleff_obj && 
                        staff->current_cleff_obj->get_w() > cleff_w)
                        cleff_w = staff->current_cleff_obj->get_w() + 
                            staff->current_cleff_obj->advance;
                    if(staff->current_key &&
                        staff->current_key->get_w() > key_w)
                        key_w = staff->current_key->get_w() + 
                            staff->current_key->advance;
                }
                line_w = x2 - x1 - cleff_w - key_w;
            }

// assume the previous beat was moved to the next line.  Use its X & time.
            line = score->lines.append(new Line(
                prev_time, 
                prev_x, 
                BEAT_PAD + cleff_w + key_w,
                score->staves.size()));

// compute the min & max Y from the 8va extensions
            for(int i = 0; i < score->staves.size(); i++)
            {
                Staff *staff = score->staves.get(i);
                if(staff->current_8va)
                {
                    Group *group = staff->current_8va;
                    DrawObject *image = group->images.get(0);
                    if(image->x2 > line->x1)
                    {
                        if(image->y < line->min_y.get(i))
                            line->min_y.set(i, image->y);
                        if(image->get_y2() > line->max_y.get(i))
                            line->max_y.set(i, image->get_y2());
                    }
                }
            }

// printf("MWindow::draw_score %d lines=%d current_time=%f\n",
// __LINE__, score->lines.size(), current_time);
        }

// draw all the objects at the current time
        for(int i = 0; i < score->staves.size(); i++)
        {
            Staff *staff = score->staves.get(i);
            staff->beat_start = staff->beat_end;
            for(int j = staff->beat_start; j < staff->groups.size(); j++)
            {
                Group *group = staff->groups.get(j);
                if(group->time == current_time)
                {
// draw it
                    process_group(staff, group);
                    staff->beat_end = j + 1;
                }
                else
// future beat
                if(group->time > current_time &&
                    next_time > group->time)
                {
                    next_time = group->time;
                    got_next = 1;
                }
            }
        }

// get X,Y extents of the beat
        int min_x = 0x7fffffff;
        int max_x = 0;
        int max_pad = 0;
        for(int i = 0; i < score->staves.size(); i++)
        {
            Staff *staff = score->staves.get(i);
            for(int j = staff->beat_start; j < staff->beat_end; j++)
            {
                Group *group = staff->groups.get(j);
                for(int k = 0; k < group->images.size(); k++)
                {
                    DrawObject *image = group->images.get(k);
                    if(image->x < min_x)
                        min_x = image->x;
                    if(image->get_x2() > max_x)
                        max_x = image->get_x2();
                    if(image->y < line->min_y.get(i))
                        line->min_y.set(i, image->y);
                    if(image->get_y2() > line->max_y.get(i))
                        line->max_y.set(i, image->get_y2());
                }

                if(group->advance > max_pad) max_pad = group->advance;
            }
        }

        if(selection_start == selection_end &&
            current_time == selection_start)
        {
            cursor_x1 = x;
            cursor_x2 = x;
        }
        score->beats.append(new Beat(current_time, x));

        for(int i = 0; i < score->staves.size(); i++)
        {
            Staff *staff = score->staves.get(i);
// left justify all X on the beat
            for(int j = staff->beat_start; j < staff->beat_end; j++)
            {
                Group *group = staff->groups.get(j);
                for(int k = 0; k < group->images.size(); k++)
                {
                    DrawObject *image = group->images.get(k);
                    image->x += x + (-min_x);
                }
            }

// update the octave markers
            if(staff->current_8va &&
                staff->current_8va->time + staff->current_8va->length > current_time)
            {
                DrawObject *octave_marker = staff->current_8va->images.get(0);
                octave_marker->x2 = x + (max_x - min_x) + max_pad / 2;
// printf("MWindow::draw_score %d x=%d max_x=%d min_x=%d max_pad=%d\n",
// __LINE__,
// x,
// max_x,
// min_x,
// max_pad);
            }
        }


// advance X
        prev_x = x;
        x += (max_x - min_x) + max_pad;

// if we hit the end of the line, end it before the current beat & 
// start a new line
        if(x - line->x1 >= line_w || !got_next)
        {
// printf("MWindow::draw_score %d x=%d line->x1=%d line_w=%d current_time=%f\n",
// __LINE__, x, line->x1, line_w, current_time);

            line->end_time = current_time;

// get y of each staff
            for(int i = 0; i < score->staves.size(); i++)
            {
                Staff *staff = score->staves.get(i);

// space above staff
                if(i == 0)
                {
                    int top = -line->min_y.get(i);
                    if(top > STAFF_SPACING) 
                        y1 += top;
                    else
                        y1 += STAFF_SPACING;
                }
                else
                {
// gap between staffs
                    int gap = line->max_y.get(i - 1) - 
                        LINE_SPACING * 4 + 
                        -line->min_y.get(i);
                    if(gap > CLEFF_SPACING)
                        y1 += gap;
                    else
                        y1 += CLEFF_SPACING;
                }

                line->y1.set(i, y1);
                y1 += 4 * LINE_SPACING;
            }

            new_line = 1;
        }

// if we hit the end of the score, quit
        if(!got_next)
        {
// printf("MWindow::draw_score %d x=%d line->x1=%d line_w=%d current_time=%f\n",
// __LINE__, x, line->x1, line_w, current_time);
            current_time = score->max_beat();
            line->end_time = current_time;

            if(line->start_time == line->end_time)
                score->lines.remove_object();
            break;
        }

// advance the time to the next beat in all the staves
        prev_time = current_time;
        current_time = next_time;
    }

// cursor after end of score
    if(cursor_x1 < 0)
    {
        cursor_x1 = x;
        cursor_x2 = x;
        selection_start = selection_end = prev_time;
        selection_line = score->lines.size() - 1;
    }
//printf("MWindow::draw_score %d cursor_x1=%d cursor_x2=%d\n",
//__LINE__, cursor_x1, cursor_x2);
    if(cursor_x2 < cursor_x1 + 2) cursor_x2 += 2;
// end of score
    score->beats.append(new Beat(current_time, x));



// pass 2: draw the score & divide into lines
    current_time = 0;
    double line_start_time = 0;
// the cursor extents for the current line
    int cursor_y1 = 0;
    int cursor_y2 = 0;

// reset the staves
    for(int i = 0; i < score->staves.size(); i++)
    {
        Staff *staff = score->staves.get(i);
        staff->reset();;
    }

//printf("MWindow::draw_score %d lines=%d\n", __LINE__, score->lines.size());
    for(int line_n = 0; line_n < score->lines.size(); line_n++)
    {
        line = score->lines.get(line_n);
        current_time = line->start_time;
//printf("MWindow::draw_score %d line=%d current_time=%f\n",
//__LINE__, line_n, current_time);

// draw the cursor
        if(selection_end >= line->start_time &&
            selection_start <= line->end_time &&
            selection_line == line_n)
        {
// single staff
            if(current_staff >= 0 && current_staff < score->staves.size())
            {
                cursor_y1 = line->y1.get(current_staff) + line->min_y.get(current_staff);
                cursor_y2 = line->y1.get(current_staff) + line->max_y.get(current_staff);
            }
            else
            if(current_staff < 0 && score->staves.size() > 0)
            {
                int last = score->staves.size() - 1;
                cursor_y1 = line->y1.get(0) + line->min_y.get(0);
                cursor_y2 = line->y1.get(last) + line->max_y.get(last);
            }

            mwindow->set_color(BLUE);
            mwindow->draw_box(cursor_x1 - line->x1 + line->x_pad, 
                cursor_y1, 
                cursor_x2 - cursor_x1, 
                cursor_y2 - cursor_y1);
            mwindow->set_color(BLACK);
        }

//printf("MWindow::draw_score %d line_n=%d %f %f\n", 
//__LINE__, line_n, line->start_time, line->end_time);
// side bars
        int last = score->staves.size() - 1;
        int min_y = line->y1.get(0);
        int max_y = line->y1.get(last) + 4 * LINE_SPACING;
        mwindow->BC_Window::draw_line(x1, min_y, x1, max_y);
        mwindow->BC_Window::draw_line(x2, min_y, x2, max_y);

// draw staff lines
        for(int i = 0; i < score->staves.size(); i++)
        {
            Staff *staff = score->staves.get(i);
            for(int j = 0; j < 5; j++)
            {
                mwindow->BC_Window::draw_line(x1, 
                    line->y1.get(i) + j * LINE_SPACING, 
                    x2, 
                    line->y1.get(i) + j * LINE_SPACING);
            }

// replicate cleffs, key signatures, 8va markers
            if(line_n > 0)
            {
                x = x1;
                if(staff->current_cleff_obj)
                {
                    Group *group = staff->current_cleff_obj;
                    int min_x = group->get_x();
                    DrawObject *image = group->images.get(0);
// printf("MWindow::draw_score %d x=%d image->x=%d min_x=%d\n", 
// __LINE__, x, image->x, min_x);
                    mwindow->draw_pixmap(image->pixmap,
                        x + image->x - min_x,
                        image->y + line->y1.get(i));
                    x += group->get_w() + BEAT_PAD;
                }

                if(staff->current_key)
                {
                    Group *group = staff->current_key;
                    int min_x = group->get_x();
                    for(int k = 0; k < group->images.size(); k++)
                    {
                        DrawObject *image = group->images.get(k);
                        mwindow->draw_pixmap(image->pixmap,
                            x + image->x - min_x,
                            image->y + line->y1.get(i));
                    }
                    x += group->get_w() + BEAT_PAD;
                }

// extend the 8va
                if(staff->current_8va)
                {
                    Group *group = staff->current_8va;
                    DrawObject *image = group->images.get(0);
                    if(image->x2 > line->x1)
                    {
//printf("MWindow::draw_score %d line_n=%d 8va continues\n", __LINE__, line_n);
                        int min_x = group->get_x();
                        mwindow->draw_pixmap(image->pixmap,
                            x + image->x - min_x,
                            image->y + line->y1.get(i));
                        int x1 = x + image->x - min_x + image->pixmap->get_w();
                        int x2 = x + image->x2 - line->x1;
                        int y2 = image->y + line->y1.get(i) + image->pixmap->get_h() / 2;
                        draw_8va_line(group, x1, x2, y2);
                    }
                }
            }
        }


//printf("MWindow::draw_score %d start_time=%f end_time=%f current_time=%f x1=%d\n",
//__LINE__, line->start_time, line->end_time, current_time, line->x1);

        while(current_time < line->end_time)
        {
            double next_time = 0x7fffffff;
            int got_next = 0;
// draw all the objects at the current time
            for(int i = 0; i < score->staves.size(); i++)
            {
                Staff *staff = score->staves.get(i);
                int y = line->y1.get(i);

// array range used by the current beat
                staff->beat_start = staff->beat_end;
                for(int j = staff->beat_start; j < staff->groups.size(); j++)
                {
                    Group *group = staff->groups.get(j);

                    if(group->time == current_time)
                    {
    // draw it
//printf("MWindow::draw_score %d line=%d %f current_time=%f\n",
//__LINE__, line_n, line->end_time, current_time);
                        draw_group(line, y, staff, group);
                        staff->beat_end = j + 1;
                    }
                    else
    // future beat
                    if(group->time > current_time &&
                        next_time > group->time)
                    {
                        next_time = group->time;
                        got_next = 1;
                    }
                }
            }

            if(!got_next) break;
    // advance the time to the next beat in all the staves
            current_time = next_time;
        }
    }


    mwindow->flash();
    mwindow->unlock_window();

//    score->dump_beats();
}

void Capture::handle_button_press()
{
    Score *score = Score::instance;
    for(int line_n = 0; line_n < score->lines.size(); line_n++)
    {
        Line *line = score->lines.get(line_n);
        for(int i = 0; i < score->staves.size(); i++)
        {
            Staff *staff = score->staves.get(i);
            if(mwindow->get_relative_cursor_y() >= line->y1.get(i) + line->min_y.get(i) &&
                mwindow->get_relative_cursor_y() < line->y1.get(i) + line->max_y.get(i))
            {
                current_staff = i;
                int got_it = 0;
                for(int j = score->beats.size() - 1; j >= 0; j--)
                {
                    Beat *beat = score->beats.get(j);
                    int x1 = beat->x - line->x1 + line->x_pad;

// cursor right of the current beat
                    if(x1 < mwindow->get_relative_cursor_x() ||  
// cursor left of the 1st original beat in the line
                        beat->time == line->start_time)
                    {
                        selection_start = selection_end = beat->time;
                        selection_line = line_n;
                        got_it = 1;
                        break;
                    }
                }

//printf("MWindow::button_press %d %d %f\n", __LINE__, got_it, selection_start);
//                 if(!got_it)
//                 {
//                     double max_beat = staff->max_beat();
//                     selection_start = max_beat;
//                 }

                selection_end = selection_start;
                break;
            }
        }
    }
}

void Capture::pop_undo()
{
}

void Capture::pop_redo()
{
}



int save_score()
{
    int result = 0;
// construct new path
    if(score_path[0] == 0)
    {
        int got_it = 0;
        for(int i = 0; i < 255; i++)
        {
            sprintf(score_path, "%sscore%d.txt", READER_PATH, i);
            FILE *fd = fopen(score_path, "r");
            if(!fd)
            {
                got_it = 1;
                break;
            }
            fclose(fd);
        }
        if(!got_it)
        {
            score_path[0] = 0;
            return 1;
        }
    }

    FILE *fd = fopen(score_path, "w");
    if(Score::instance->save(fd))
    {
        score_path[0] = 0;
        result = 1;
    }
    fclose(fd);
    
    
    Capture::instance->draw_score();
    return result;
}

char* skipwhite(char *ptr)
{
    while(*ptr != 0 && (*ptr == ' ' || *ptr == '\t' || *ptr == '\n'))
        ptr++;
    
    return ptr;
}

char* getcommand(char *ptr, char *result)
{
    result[0] = 0;
    char *ptr2 = result;
    while(*ptr != 0 && *ptr != ' ' && *ptr != '\t' && *ptr != '\n')
        *ptr2++ = *ptr++;
    *ptr2 = 0;
    return ptr;
}

char* getparam(char *ptr, char *param, char *value)
{
    param[0] = 0;
    value[0] = 0;
    char *ptr2 = param;
    while(*ptr != 0 && *ptr != ' ' && *ptr != '\t' && *ptr != '\n' && *ptr != '=')
        *ptr2++ = *ptr++;
    *ptr2 = 0;

    if(*ptr == '=')
        ptr++;

    ptr2 = value;
    while(*ptr != 0 && *ptr != ' ' && *ptr != '\t' && *ptr != '\n')
        *ptr2++ = *ptr++;
    *ptr2 = 0;

    return ptr;
}

int load_score(char *path)
{
    int result = 0;
    char string[BCTEXTLEN];
    char param[BCTEXTLEN];
    char value[BCTEXTLEN];

    strcpy(score_path, path);
    FILE *fd = fopen(score_path, "r");

    if(fd)
    {
        delete Score::instance;
        Score::instance = new Score;
        Staff *staff = 0;
        Group *group = 0;

        while(!feof(fd))
        {
            char *line = fgets(string, BCTEXTLEN, fd);
            if(!line || line[0] == 0) break;

            char *ptr = skipwhite(line);
// comment line or empty
            if(*ptr == '#' || *ptr == 0) continue;
            char command[BCTEXTLEN];
            ptr = getcommand(ptr, command);

            if(!strcmp(command, "SCORE"))
            {
#define GETPARAMS1 \
    while(*ptr != 0 && *ptr != '\n') \
    { \
        ptr = skipwhite(ptr); \
        ptr = getparam(ptr, param, value);

                GETPARAMS1
                    if(!strcmp(param, "CURRENT_STAFF"))
                        current_staff = atoi(value);
                    else
                    if(!strcmp(param, "SELECTION_START"))
                        selection_start = atoi(value);
                    else
                    if(!strcmp(param, "SELECTION_END"))
                        selection_end = atoi(value);
                }
            }
            else
            if(!strcmp(command, "STAFF"))
            {
                staff = Score::instance->staves.append(new Staff);
                group = 0;
//                staff->dump();
            }
            else
            if(!strcmp(command, "GROUP"))
            {
                group = staff->groups.append(new Group);
                GETPARAMS1
                    if(!strcmp(param, "TYPE"))
                        group->type = atoi(value);
                    else
                    if(!strcmp(param, "TIME"))
                        group->time = atof(value);
                    else
                    if(!strcmp(param, "LENGTH"))
                        group->length = atof(value);
                    else
                    if(!strcmp(param, "CLEFF"))
                        group->cleff = atoi(value);
                    else
                    if(!strcmp(param, "KEY"))
                        group->key = atoi(value);
                    else
                    if(!strcmp(param, "OCTAVE"))
                        group->octave = atoi(value);
                }
//                group->dump();
            }
            else
            if(!strcmp(command, "NOTE"))
            {
                Note *current_note = group->notes.append(new Note);
                GETPARAMS1
                    if(!strcmp(param, "PITCH"))
                        current_note->pitch = atoi(value);
                    else
                    if(!strcmp(param, "ACCIDENTAL"))
                        current_note->accidental = atoi(value);
                }
            }

            
        }
        fclose(fd);
    }

    Score::instance->dump();

    Capture::instance->draw_score();

    return result;
}

void prev_beat()
{
    Score *score = Score::instance;

    if(selection_start > 0)
    {
        selection_start -= 1;
        selection_end = selection_start;
        Capture::instance->draw_score();
    }
    else
    if(current_staff < score->staves.size())
    {
        double max_beat = score->staves.get(current_staff)->max_beat();
        selection_start = max_beat;
        selection_end = selection_start;
        Capture::instance->draw_score();
    }
}

void next_beat()
{
    Score *score = Score::instance;

    if(current_staff < score->staves.size())
    {
        selection_start += 1;
        double max_beat = score->staves.get(current_staff)->max_beat();
        if(selection_start > max_beat)
            selection_start = 0;
        selection_end = selection_start;
        Capture::instance->draw_score();
    }
}

void prev_staff()
{
    Score *score = Score::instance;

    current_staff--;
    if(current_staff < 0)
        current_staff = score->staves.size() - 1;
    if(current_staff < 0)
        current_staff = 0;

    if(current_staff < score->staves.size())
    {
        double max_beat = score->staves.get(current_staff)->max_beat();
        if(selection_start > max_beat)
            selection_start = max_beat;
        selection_end = selection_start;
        Capture::instance->draw_score();
    }
}

void next_staff()
{
    Score *score = Score::instance;

    current_staff++;
    if(current_staff >= score->staves.size())
        current_staff = 0;

    if(current_staff < score->staves.size())
    {
        double max_beat = score->staves.get(current_staff)->max_beat();
        if(selection_start > max_beat)
            selection_start = max_beat;
        selection_end = selection_start;
        Capture::instance->draw_score();
    }
}







