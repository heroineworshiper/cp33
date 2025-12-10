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
#include "score.h"
#include <stdint.h>
#include <string.h>

// cursor position
int current_staff = 0; // number of staff or -1 if all
double selection_start = 3;  // beat
double selection_end = 3;
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












DrawObject::DrawObject(int x,
    int y,
    BC_Pixmap *pixmap)
{
    this->x = x;
    this->y = y;
    this->pixmap = pixmap;
}

void DrawObject::set(int x,
    int y,
    BC_Pixmap *pixmap)
{
    this->x = x;
    this->y = y;
    this->pixmap = pixmap;
}

int DrawObject::is_accidental()
{
    return (pixmap == MWindow::flat ||
        pixmap == MWindow::sharp ||
        pixmap == MWindow::natural);
}

int DrawObject::get_x2()
{
    return x + pixmap->get_w();
}

int DrawObject::get_y2()
{
    return y + pixmap->get_h();
}




#define LINE_SPACING 10
#define CLEFF_SPACING 50 // space between cleffs
#define STAFF_SPACING 80 // space between staffs
#define BEAT_PAD 5
// always centered around X=0
void MWindow::draw_group(Staff *staff, Group *group)
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
            staff->current_octave = group;
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
// printf("MWindow::draw_group %d current_octave=%d octave_remane=%f\n", 
// __LINE__,
// staff->current_octave,
// staff->octave_remane);
            break;
        }

        case IS_CHORD:
        default:
        {
            int have_octave_marker = 0;
            for(int j = 0; j < group->notes.size(); j++)
            {
                Note *note = group->notes.get(j);

// compute the current octave
                int octave = (note->pitch - MIN_C) / OCTAVE;
// compute the position in the current octave
                int scale_position2 = ((note->pitch - MIN_C) % OCTAVE);
// convert the MIDI code to a position & octave in the current scale
                int scale_position = scale_position2 - current_key;
//printf("MWindow::draw_group %d scale_position2=%d scale_position=%d octave=%d\n", 
//__LINE__, scale_position2, scale_position, octave);

                while(scale_position < 0)
                {
                    scale_position += OCTAVE;
                    octave--;
                }
//printf("MWindow::draw_group %d scale_position=%d octave=%d\n", 
//__LINE__, scale_position, octave);


// shift for 8va
                if(staff->current_octave &&
                    staff->current_octave->time + staff->current_octave->length > group->time)
                {
                    have_octave_marker = 1;
                    octave -= staff->current_octave->octave;
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
//printf("MWindow::draw_group %d pitch=%d scale_position=%d major_scale_position=%d cmaj_position=%d staff_position=%d\n", 
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
//printf("MWindow::draw_group %d accidentals[cmaj_position]=%d need_accidental=%d\n", 
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
// printf("MWindow::draw_group %d %d %d %d\n",
// __LINE__, y + quarter->get_h(), object->y, object->x + quarter->get_w());
                        x = object->x + quarter->get_w();
                        break;
                    }
                }

// printf("MWindow::draw_group %d x=%d y=%d\n", 
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
//printf("MWindow::draw_group %d y=%d %d\n", __LINE__, y, y1 + LINE_SPACING * 4);
                    for(int k = LINE_SPACING * 5; k <= center_y; k += LINE_SPACING)
                    {
//printf("MWindow::draw_group %d k=%d\n", __LINE__, k);
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
            if(have_octave_marker)
            {
                DrawObject *octave_marker = staff->current_octave->images.get(0);
                for(int j = 0; j < group->images.size(); j++)
                {
                    DrawObject *object = group->images.get(j);
                    
                    if(staff->current_octave->octave < 0)
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

void MWindow::draw_score()
{
    Score *score = Score::instance;
    int margin = 5;
    char temp[BCTEXTLEN];
    score->beats.remove_all_objects();
// TODO: rewind & set the Y for the staff in the beats
    score->beats.append(new Beat(0, 0));

    lock_window();
// finale background color
    set_color(0xfdfcf5);
    draw_box(0, 0, root_w, root_h, 0);
    set_color(BLACK);


    if(score_path[0] == 0)
        sprintf(temp, "Untitled");
    else
        strcpy(temp, score_path);
    BC_Window::set_font(SMALLFONT);
    BC_Window::draw_text(margin, 
        margin + BC_Window::get_text_ascent(SMALLFONT), temp);

// pass 1: determine the extents
    double current_time = 0;
// current position on an infinitely wide page
    int x = BEAT_PAD;

// initialize the staves
    for(int i = 0; i < score->staves.size(); i++)
    {
        Staff *staff = score->staves.get(i);
        staff->current_cleff = TREBLE;
        staff->current_key = KEY_C;
        staff->current_octave = 0;
        staff->line_start = 0;
        staff->line_end = 0;
        staff->beat_start = 0;
        staff->beat_end = 0;
        staff->max_y = LINE_SPACING * 4;
        staff->min_y = 0;
        bzero(staff->accidentals, sizeof(int) * MAJOR_SCALE);
    }

    cursor_x1 = cursor_x2 = -1;

// advance through all the staves at the same time
// until we hit the end of the score
    while(1)
    {
        double next_time = 0x7fffffff;
        int got_next = 0;

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
                    draw_group(staff, group);
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
                    if(image->y < staff->min_y)
                        staff->min_y = image->y;
                    if(image->get_y2() > staff->max_y)
                        staff->max_y = image->get_y2();
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
            if(staff->current_octave &&
                staff->current_octave->time + staff->current_octave->length > current_time)
            {
                DrawObject *octave_marker = staff->current_octave->images.get(0);
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
        x += (max_x - min_x) + max_pad;

// if we hit the end of the score, quit
        if(!got_next)
        {
            current_time = score->max_beat();
            break;
        }

// advance the time to the next beat in all the staves
        current_time = next_time;
    }

// cursor after end of score
    if(cursor_x1 < 0)
    {
        cursor_x1 = x;
        cursor_x2 = x;
    }
    if(cursor_x2 < cursor_x1 + 2) cursor_x2 += 2;
// end of score
    score->beats.append(new Beat(current_time, x));



// pass 2: draw the score & divide into lines
    current_time = 0;
    double line_start_time = 0;
// the cursor extents for the current line
    int cursor_y1 = 0;
    int cursor_y2 = 0;
// start of current line
    int y1 = 0;
    int x1 = margin;
// end of line
    int x2 = root_w - margin;
    int new_line = 1;
    

    for(int i = 0; i < score->staves.size(); i++)
    {
        Staff *staff = score->staves.get(i);
        staff->beat_start = 0;
        staff->beat_end = 0;
    }

    while(1)
    {
//printf("MWindow::draw_score %d\n", __LINE__);
// draw new line
        if(new_line)
        {
            new_line = 0;

            int y = y1;

// Y range of the lines
            int min_y;
            int max_y;
            for(int i = 0; i < score->staves.size(); i++)
            {
                Staff *staff = score->staves.get(i);

// space above staff
                if(i == 0)
                {
                    if(-staff->min_y > STAFF_SPACING) 
                        y += -staff->min_y;
                    else
                        y += STAFF_SPACING;
                    min_y = y;
                }
                else
                {
                    Staff *staff2 = score->staves.get(i - 1);
                    if(staff2->max_y - LINE_SPACING * 4 + -staff->min_y > CLEFF_SPACING)
                        y += staff2->max_y - LINE_SPACING * 4 + -staff->min_y;
                    else
                        y += CLEFF_SPACING;
                }

                staff->y1 = y;
                y += 4 * LINE_SPACING;
// bottom of lines
                max_y = y;
            }

// single staff
            if(current_staff >= 0 && current_staff < score->staves.size())
            {
                Staff *staff = score->staves.get(current_staff);
                cursor_y1 = staff->y1 + staff->min_y;
                cursor_y2 = staff->y1 + staff->max_y;
            }
            else
            if(current_staff < 0 && score->staves.size() > 0)
            {
                Staff *staff1 = score->staves.get(0);
                Staff *staff2 = score->staves.get(score->staves.size() - 1);
                cursor_y1 = staff1->y1 + staff1->min_y;
                cursor_y2 = staff2->y1 + staff2->max_y;
            }

// draw cursor
            BC_Window::set_color(BLUE);
            BC_Window::draw_box(cursor_x1, 
                cursor_y1, 
                cursor_x2 - cursor_x1, 
                cursor_y2 - cursor_y1);
            BC_Window::set_color(BLACK);

// draw staff lines
            for(int i = 0; i < score->staves.size(); i++)
            {
                Staff *staff = score->staves.get(i);
                for(int j = 0; j < 5; j++)
                {
                    BC_Window::draw_line(x1, 
                        staff->y1 + j * LINE_SPACING, 
                        x2, 
                        staff->y1 + j * LINE_SPACING);
                }
            }

// side bars
            BC_Window::draw_line(x1, min_y, x1, max_y);
            BC_Window::draw_line(x2, min_y, x2, max_y);
        }

        double next_time = 0x7fffffff;
        int got_next = 0;
// draw all the objects at the current time
        for(int i = 0; i < score->staves.size(); i++)
        {
            Staff *staff = score->staves.get(i);
            int y = staff->y1;

            staff->beat_start = staff->beat_end;
            for(int j = staff->beat_start; j < staff->groups.size(); j++)
            {
                Group *group = staff->groups.get(j);
                if(group->time == current_time)
                {
// draw it
                    for(int k = 0; k < group->images.size(); k++)
                    {
                        DrawObject *image = group->images.get(k);
                        BC_Window::draw_pixmap(image->pixmap,
                            image->x + x1,
                            image->y + y);
                    }

// draw octave line
                    if(group->type == IS_OCTAVE)
                    {
                        DrawObject *image = group->images.get(0);
                        int y2 = image->y + y + image->pixmap->get_h() / 2;
                        int x2 = image->x2 + x1;
                        int direction = -1;
                        if(group->octave > 0) direction = 1;
                        BC_Window::set_line_dashes(2);
                        BC_Window::set_line_width(2);
                        BC_Window::draw_line(
                            image->x + x1 + image->pixmap->get_w(),
                            y2,
                            x2,
                            y2);
                        BC_Window::draw_line(
                            x2,
                            y2,
                            x2,
                            y2 + 5 * direction);
                        BC_Window::set_line_dashes(0);
                        BC_Window::set_line_width(1);
                    }

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


    flash();
    unlock_window();

//    score->dump_beats();
}

void MWindow::button_press()
{
    Score *score = Score::instance;
    for(int i = 0; i < score->staves.size(); i++)
    {
        Staff *staff = score->staves.get(i);
        if(get_relative_cursor_y() >= staff->y1 + staff->min_y &&
            get_relative_cursor_y() < staff->y1 + staff->max_y)
        {
            current_staff = i;
            for(int j = score->beats.size() - 1; j >= 0; j--)
            {
                Beat *beat = score->beats.get(j);
                if(beat->x < get_relative_cursor_x())
                {
                    selection_start = selection_end = beat->time;
                    break;
                }
            }

//                         double max_beat = staff->max_beat();
//                         if(selection_start > max_beat)
//                             selection_start = max_beat;
            selection_end = selection_start;
            break;
        }
    }
}

void MWindow::pop_capture_undo()
{
}

void MWindow::pop_capture_redo()
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
    
    
    MWindow::instance->draw_score();
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

    MWindow::instance->draw_score();

    return result;
}

void prev_beat()
{
    Score *score = Score::instance;

    if(selection_start > 0)
    {
        selection_start -= 1;
        selection_end = selection_start;
        MWindow::instance->draw_score();
    }
    else
    if(current_staff < score->staves.size())
    {
        double max_beat = score->staves.get(current_staff)->max_beat();
        selection_start = max_beat;
        selection_end = selection_start;
        MWindow::instance->draw_score();
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
        MWindow::instance->draw_score();
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
        MWindow::instance->draw_score();
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
        MWindow::instance->draw_score();
    }
}







