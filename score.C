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

#include "capture.h"
#include "mwindow.h"
#include "score.h"
#include <string.h>

Score* Score::instance = 0;


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
    return (pixmap == Capture::instance->flat ||
        pixmap == Capture::instance->sharp ||
        pixmap == Capture::instance->natural);
}

int DrawObject::get_x2()
{
    return x + pixmap->get_w();
}

int DrawObject::get_y2()
{
    return y + pixmap->get_h();
}


Score::Score()
{
}

Score::~Score()
{
    staves.remove_all_objects();
    beats.remove_all_objects();
    lines.remove_all_objects();
}

void Score::test()
{
// create test notes
    Staff *treble = staves.append(new Staff);
    Staff *bass = staves.append(new Staff);
    
    Group *group;
    int time = 0;

    group = treble->groups.append(new Group(time, IS_CLEFF));
    group->cleff = TREBLE;

    group = bass->groups.append(new Group(time, IS_CLEFF));
    group->cleff = BASS;

    time++;


// 8va tests
// // bass 8va
// // TODO: automatically sort the 8va before the notes
//     group = bass->groups.append(new Group(time, IS_OCTAVE));
//     group->octave = -1;
//     group->length = 3;
// 
// 
// // bass lowest A
//     group = bass->groups.append(new Group(time, IS_CHORD));
//     group->append(new Note(0, MIDDLE_A - OCTAVE * 4));
//     time++;
// 
//     group = bass->groups.append(new Group(time, IS_CHORD));
//     group->append(new Note(0, MIDDLE_A - OCTAVE * 3));
//     time++;
// 
//     group = bass->groups.append(new Group(time, IS_CHORD));
//     group->append(new Note(0, MIDDLE_A - OCTAVE * 4));
//     time++;
// 
//     group = bass->groups.append(new Group(time, IS_CHORD));
//     group->append(new Note(0, MIDDLE_A - OCTAVE * 3));
//     time++;
// 
// // treble 8va
//     group = treble->groups.append(new Group(time, IS_OCTAVE));
//     group->octave = 1;
//     group->length = 3;
// 
//     group = treble->groups.append(new Group(time, IS_CHORD));
//     group->append(new Note(0, MIDDLE_C + OCTAVE * 4));
//     time++;
// 
//     group = treble->groups.append(new Group(time, IS_CHORD));
//     group->append(new Note(0, MIDDLE_C + OCTAVE * 4));
//     time++;
// 
//     group = treble->groups.append(new Group(time, IS_CHORD));
//     group->append(new Note(0, MIDDLE_C + OCTAVE * 4));
//     time++;
// 
//     group = treble->groups.append(new Group(time, IS_CHORD));
//     group->append(new Note(0, MIDDLE_C + OCTAVE * 2));
//     time++;



// scales & keys
#if 1
    group = treble->groups.append(new Group(time, IS_KEY));
    group->key = KEY_DF;

    group = bass->groups.append(new Group(time, IS_KEY));
    group->key = KEY_DF;

    time++;

// bass chord
    group = bass->groups.append(new Group(time, IS_CHORD));
    group->append(new Note(0, MIDDLE_DF - OCTAVE));
    group->append(new Note(0, MIDDLE_F - OCTAVE));
    group->append(new Note(0, MIDDLE_AF - OCTAVE));
    group->append(new Note(0, MIDDLE_DF));
    time++;

// treble F
    group = treble->groups.append(new Group(time, IS_CHORD));
    group->append(new Note(0, MIDDLE_F));
    time++;



// treble E
    group = treble->groups.append(new Group);
    group->time = time;
    group->type = IS_CHORD;
    group->append(new Note(FLAT, MIDDLE_E)); // Note resulting in E when flat


    time++;

    group = treble->groups.append(new Group);
    group->time = time++;
    group->type = IS_CHORD;
    group->append(new Note(0, MIDDLE_EF));

    group = treble->groups.append(new Group);
    group->time = time++;
    group->type = IS_CHORD;
    group->append(new Note(0, MIDDLE_C));

    group = treble->groups.append(new Group);
    group->time = time++;
    group->type = IS_CHORD;
    group->append(new Note(0, MIDDLE_BF - OCTAVE));

    group = treble->groups.append(new Group);
    group->time = time++;
    group->type = IS_CHORD;
    group->append(new Note(0, MIDDLE_AF - OCTAVE));

    group = treble->groups.append(new Group);
    group->time = time++;
    group->type = IS_CHORD;
    group->append(new Note(0, MIDDLE_GF - OCTAVE));

    group = treble->groups.append(new Group);
    group->time = time++;
    group->type = IS_CHORD;
    group->append(new Note(0, MIDDLE_F - OCTAVE));

    group = treble->groups.append(new Group);
    group->time = time++;
    group->type = IS_CHORD;
    group->append(new Note(0, MIDDLE_DF));
    group->append(new Note(0, MIDDLE_E));
    group->append(new Note(0, MIDDLE_G));
    group->append(new Note(0, MIDDLE_DF + OCTAVE));

    group = treble->groups.append(new Group);
    group->time = time++;
    group->type = IS_CHORD;
    group->append(new Note(0, MIDDLE_C));
    group->append(new Note(0, MIDDLE_E));
    group->append(new Note(0, MIDDLE_G));
    group->append(new Note(0, MIDDLE_C + OCTAVE));

    group = treble->groups.append(new Group);
    group->time = time++;
    group->type = IS_CHORD;
    group->append(new Note(0, MIDDLE_DF + OCTAVE));

    group = treble->groups.append(new Group);
    group->time = time++;
    group->type = IS_CHORD;
    group->append(new Note(0, MIDDLE_EF + OCTAVE));

    group = treble->groups.append(new Group);
    group->time = time++;
    group->type = IS_CHORD;
    group->append(new Note(0, MIDDLE_F + OCTAVE));

    group = treble->groups.append(new Group);
    group->time = time++;
    group->type = IS_CHORD;
    group->append(new Note(0, MIDDLE_GF + OCTAVE));

    group = treble->groups.append(new Group);
    group->time = time++;
    group->type = IS_CHORD;
    group->append(new Note(0, MIDDLE_AF + OCTAVE));

    group = treble->groups.append(new Group);
    group->time = time++;
    group->type = IS_CHORD;
    group->append(new Note(0, MIDDLE_BF + OCTAVE));

    group = treble->groups.append(new Group);
    group->time = time++;
    group->type = IS_CHORD;
    group->append(new Note(0, MIDDLE_C + OCTAVE * 2));

    group = treble->groups.append(new Group);
    group->time = time++;
    group->type = IS_CHORD;
    group->append(new Note(0, MIDDLE_DF + OCTAVE * 2));

    Staff *staff = bass;
    group = staff->groups.append(new Group(time, IS_OCTAVE));
    group->octave = -1;
    group->length = 15 * 5;
    for(int i = 0; i < 5; i++)
    {
#define OCTAVES 2
        group = staff->groups.append(new Group(time++, IS_CHORD));
        group->append(new Note(0, MIDDLE_DF - OCTAVE * (OCTAVES + 1)));
        group = staff->groups.append(new Group(time++, IS_CHORD));
        group->append(new Note(0, MIDDLE_EF - OCTAVE * (OCTAVES + 1)));
        group = staff->groups.append(new Group(time++, IS_CHORD));
        group->append(new Note(0, MIDDLE_F - OCTAVE * (OCTAVES + 1)));
        group = staff->groups.append(new Group(time++, IS_CHORD));
        group->append(new Note(0, MIDDLE_GF - OCTAVE * (OCTAVES + 1)));
        group = staff->groups.append(new Group(time++, IS_CHORD));
        group->append(new Note(0, MIDDLE_AF - OCTAVE * (OCTAVES + 1)));
        group = staff->groups.append(new Group(time++, IS_CHORD));
        group->append(new Note(0, MIDDLE_BF - OCTAVE * (OCTAVES + 1)));
        group = staff->groups.append(new Group(time++, IS_CHORD));
        group->append(new Note(0, MIDDLE_C - OCTAVE * OCTAVES));
        group = staff->groups.append(new Group(time++, IS_CHORD));
        group->append(new Note(0, MIDDLE_DF - OCTAVE * OCTAVES));

        group = staff->groups.append(new Group(time++, IS_CHORD));
        group->append(new Note(0, MIDDLE_C - OCTAVE * OCTAVES));
        group = staff->groups.append(new Group(time++, IS_CHORD));
        group->append(new Note(0, MIDDLE_BF - OCTAVE * (OCTAVES + 1)));
        group = staff->groups.append(new Group(time++, IS_CHORD));
        group->append(new Note(0, MIDDLE_AF - OCTAVE * (OCTAVES + 1)));
        group = staff->groups.append(new Group(time++, IS_CHORD));
        group->append(new Note(0, MIDDLE_GF - OCTAVE * (OCTAVES + 1)));
        group = staff->groups.append(new Group(time++, IS_CHORD));
        group->append(new Note(0, MIDDLE_F - OCTAVE * (OCTAVES + 1)));
        group = staff->groups.append(new Group(time++, IS_CHORD));
        group->append(new Note(0, MIDDLE_EF - OCTAVE * (OCTAVES + 1)));
//printf("Score::test %d time=%d\n", __LINE__, time);
        group = staff->groups.append(new Group(time++, IS_CHORD));
        group->append(new Note(0, MIDDLE_DF - OCTAVE * (OCTAVES + 1)));
//         if(staff == bass) 
//             staff = treble;
//         else
//             staff = bass;
    }

#endif

//    dump();
}

int Score::save(FILE *fd)
{
    int result = 0;

    fprintf(fd, "SCORE CURRENT_STAFF=%d SELECTION_START=%f SELECTION_END=%f\n",
        current_staff,
        selection_start,
        selection_end);

    for(int i = 0; i < staves.size() && !result; i++)
        result = staves.get(i)->save(fd);
    return result;
}

double Score::max_beat()
{
    double result = 0;
    for(int i = 0; i < staves.size(); i++)
    {
        Staff *staff = staves.get(i);
        double end = staff->max_beat();
        if(end > result) result = end;
    }
    return result;
}

void Score::delete_beat()
{
    Score *score = Score::instance;

    for(int i = 0; i < staves.size(); i++)
    {
        if(current_staff == i || current_staff < 0)
        {
            Staff *staff = staves.get(i);
// rewind 1 beat behind current position
            int start = -1;
            for(int j = staff->groups.size() - 1; j >= 0; j--)
            {
                Group *group = staff->groups.get(j);
                if(group->time < selection_start)
                {
                    start = j;
                    break;
                }
            }


            if(start >= 0)
            {
                staff->groups.remove_object_number(start);

// shift octaves left
                for(int j = 0; j < staff->groups.size(); j++)
                {
                    Group *group = staff->groups.get(j);
                    if(group->type == IS_OCTAVE &&
                        group->time <= selection_start &&
                        group->time + group->length >= selection_start)
                    {
                        group->length -= 1;
                        if(group->length <= 0)
                        {
                            staff->groups.remove_object_number(j);
                            j--;
                        }
                    }
                }

// shift times left
                for(int j = start; j < staff->groups.size(); j++)
                {
                    Group *group = staff->groups.get(j);
                    group->time -= 1;
                }
            }
        }
    }
//    dump();
}

void Score::dump()
{
    printf("Score::dump %d current_staff=%d selection_start=%f selection_end=%f\n",
        __LINE__,
        current_staff,
        selection_start,
        selection_end);
    for(int i = 0; i < staves.size(); i++)
        staves.get(i)->dump();
}

void Score::dump_beats()
{
    for(int i = 0; i < beats.size(); i++)
    {
        Beat *beat = beats.get(i);
        printf("Score::dump_beats %d %f %d\n", 
            __LINE__, 
            beat->time,
            beat->x);
    }
}







Staff::Staff()
{
    reset();
}

Staff::~Staff()
{
    groups.remove_all_objects();
}

void Staff::reset()
{
    current_cleff = TREBLE;
    current_8va = 0;
    current_cleff_obj = 0;
    current_key = KEY_C;
    beat_start = 0;
    beat_end = 0;
    bzero(accidentals, sizeof(int) * MAJOR_SCALE);
}

int Staff::save(FILE *fd)
{
    int result = 0;
    fprintf(fd, "STAFF\n");

    for(int i = 0; i < groups.size() && !result; i++)
        result = groups.get(i)->save(fd);
    return result;
}

double Staff::max_beat()
{
    double result = 0;
    for(int i = 0; i < groups.size(); i++)
    {
        Group *group = groups.get(i);
        double end = group->time + group->length;
        if(end > result) result = end;
    }
    return result;
}

void Staff::dump()
{
    printf("Staff::dump %d\n", __LINE__);
    for(int i = 0; i < groups.size(); i++)
        groups.get(i)->dump();
}



Group::Group(double time, int type)
{
    reset();
    this->time = time;
    this->type = type;
    this->length = 1;
}

Group::Group()
{
    reset();
}

Group::~Group()
{
    notes.remove_all_objects();
    images.remove_all_objects();
}

void Group::reset()
{
    time = 0;
    length = 1;
    type = IS_CHORD;
    cleff = TREBLE;
    key = KEY_C;
    octave = 0;
}

void Group::dump()
{
    printf("Group::dump %d type=%d time=%f length=%f octave=%d\n",
        __LINE__,
        type,
        time,
        length,
        octave);
}

int Group::get_w()
{
    int min_x = 0x7fffffff;
    int max_x = -0x7fffffff;
    for(int i = 0; i < images.size(); i++)
    {
        int x1 = images.get(i)->x;
        int x2 = images.get(i)->get_x2();
        if(x1 < min_x) min_x = x1;
        if(x2 > max_x) max_x = x2;
    }
    return max_x - min_x;
}

int Group::get_x()
{
    int min_x = 0x7fffffff;
    for(int i = 0; i < images.size(); i++)
    {
        int x1 = images.get(i)->x;
        if(x1 < min_x) min_x = x1;
    }
    return min_x;
}


// sort from lowest to highest to simplify drawing
void Group::append(Note *note)
{
    int got_it = 0;
    for(int i = notes.size() - 1; i >= 0; i--)
    {
        if(notes.get(i)->pitch < note->pitch)
        {
            notes.insert_after(note, i);
            got_it = 1;
            break;
        }
        else
        if(notes.get(i)->pitch == note->pitch)
        {
printf("Group::append %d exists\n", __LINE__);
            got_it = 1;
            break;
        }
    }
    
    if(!got_it)
    {
        notes.append(note);
    }
}

int Group::save(FILE *fd)
{
    int result = 0;
    fprintf(fd, "    GROUP TYPE=%d TIME=%f LENGTH=%f CLEFF=%d KEY=%d OCTAVE=%d\n",
        type,
        time,
        length,
        cleff,
        key,
        octave);

    for(int i = 0; i < notes.size() && !result; i++)
        result = notes.get(i)->save(fd);
    return result;
}


Note::Note()
{
    reset();
}

Note::Note(int accidental, int pitch)
{
    reset();
    this->accidental = accidental;
    this->pitch = pitch;
}

Note::~Note()
{
}

void Note::reset()
{
    accidental = NO_ACCIDENTAL;
    pitch = -1;
    duration = -1;
}

int Note::save(FILE *fd)
{
    int result = 0;
    fprintf(fd, "        NOTE PITCH=%d ACCIDENTAL=%d\n",
        pitch,
        accidental);
    return result;
}


Beat::Beat()
{
    reset();
}

Beat::Beat(double time, int x)
{
    reset();
    this->time = time;
    this->x = x;
}


void Beat::reset()
{
    time = 0;
    x = 0;
    y1 = 0;
    y2 = 0;
}




Line::Line(double start_time, int x1, int x_pad, int staves)
{
    this->start_time = this->end_time = start_time;
    this->x1 = x1;
    this->x_pad = x_pad;
    for(int i = 0; i < staves; i++)
    {
        max_y.append(LINE_SPACING * 4);
        min_y.append(0);
        y1.append(0);
    }
}







