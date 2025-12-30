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
    abs_x = abs_y = 0x7fffffff;
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
    if(pixmap)
        return x + pixmap->get_w();
    else
        return x;
}

int DrawObject::get_y2()
{
    if(pixmap)
        return y + pixmap->get_h();
    else
        return y;
}


Score::Score()
{
}

Score::~Score()
{
    clear();
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

// stacked notes
#if 0
    group = treble->groups.append(new Group(time, IS_CHORD));
    group->append(new Note(0, MIDDLE_C));
    group->append(new Note(0, MIDDLE_D));
    group->append(new Note(0, MIDDLE_E));
    group->append(new Note(0, MIDDLE_F));
#endif // 0


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
//    group->key = KEY_E;

    group = bass->groups.append(new Group(time, IS_KEY));
    group->key = KEY_DF;
//    group->key = KEY_E;

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
// treble EF
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

// line wrap 8va
    Staff *staff = bass;
//     group = staff->groups.append(new Group(time, IS_OCTAVE));
//     group->octave = -1;
//     group->length = 15 * 5;


    for(int i = 0; i < 5; i++)
    {
#define OCTAVES 0
        group = treble->groups.append(new Group(time, IS_BAR));
        group = bass->groups.append(new Group(time, IS_BAR));
        time++;

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

#endif // 1

    clean();
//    dump();
}

void Score::clear()
{
    staves.remove_all_objects();
    beats.remove_all_objects();
    lines.remove_all_objects();
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

// doesn't delete 8vas
void Score::delete_beat()
{
    Score *score = Score::instance;
    double new_position = -1;
    Group *group = 0;

    for(int i = 0; i < staves.size(); i++)
    {
        if(current_staff == i || current_staff < 0)
        {
            Staff *staff = staves.get(i);
// discover the 1st object behind the current position
            int start = -1;
            for(int j = staff->groups.size() - 1; j >= 0; j--)
            {
                group = staff->groups.get(j);
//printf("Score::delete_beat %d j=%d time=%f selection_start=%f\n",
//__LINE__, j, group->time, selection_start);
                if(group->time < selection_start)
                {
                    start = j;
                    new_position = group->time;
                    break;
                }
            }
//printf("Score::delete_beat %d total=%d start=%d\n",
//__LINE__, staff->groups.size(), start);

            if(start >= 0)
            {
// delete the bar in all the staves
                if(group->type == IS_BAR)
                    delete_object(0, group->time, group->length);
                else
// delete the object in 1 staff
                    delete_object(staff, group->time, group->length);
                
// update the current position
                selection_start = selection_end = new_position;
            }
        }
    }
//    dump();
}

void Score::copy_from(Score *src)
{
    clear();
    for(int i = 0; i < src->staves.size(); i++)
    {
        Staff *staff = this->staves.append(new Staff);
        staff->copy_from(src->staves.get(i));
    }
}

// fill gaps with rests.  Sort groups by time.
// return 1 to flip ptr1 & ptr2
static int compare(const void *ptr1, const void *ptr2)
{
	Group *item1 = *(Group**)ptr1;
	Group *item2 = *(Group**)ptr2;
// Order in a simultaneous beat must be the enum number
    if(item1->time == item2->time)
        return item1->type > item2->type;
    
	return item1->time >= item2->time;
}

void Score::clean()
{
    for(int i = 0; i < staves.size(); i++)
    {
        Staff *staff = this->staves.get(i);
// sort by time & type
        qsort(staff->groups.values, staff->groups.size(), sizeof(Group*), compare);

// fill gaps with rests
        double current_time = 0;
        for(int j = 0; j < staff->groups.size(); j++)
        {
            Group *group = staff->groups.get(j);
            if(group->time > current_time + 1)
            {
                while(group->time > current_time + 1)
                {
                    current_time += 1;
                    staff->groups.insert(new Group(current_time, IS_REST), j);
                    j++;
                }
                current_time = group->time;
            }
            else
            {
                current_time = group->time;
            }
        }
    }
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

int Score::find_delete_object(int cursor_x, 
    int cursor_y, 
    int *staff_n, 
    int *group_n, 
    int *image_n,
    int *x,
    int *y,
    int *w,
    int *h)
{
    *staff_n = 0;
    *group_n = 0;
    *image_n = 0;
    for(*staff_n = 0; *staff_n < staves.size(); (*staff_n)++)
    {
        Staff *staff = staves.get(*staff_n);
        for(*group_n = 0; *group_n < staff->groups.size(); (*group_n)++)
        {
            Group *group = staff->groups.get(*group_n);
            switch(group->type)
            {
                case IS_KEY:
// image_n is overwritten by intervening searches
                    *image_n = 0;
                    *x = group->get_abs_x();
                    *y = group->get_abs_y();
                    *w = group->get_w();
                    *h = group->get_h();
                    if(cursor_x >= *x &&
                        cursor_y >= *y &&
                        cursor_x < *x + *w &&
                        cursor_y < *y + *h)
                    {
                        return 1;
                    }
                    break;
                case IS_CLEFF:
                case IS_OCTAVE:
                case IS_REST:
                {
                    DrawObject *image = group->images.get(0);
// image_n is overwritten by intervening searches
                    *image_n = 0;
                    *x = image->abs_x;
                    *y = image->abs_y;
                    *w = image->pixmap->get_w();
                    *h = image->pixmap->get_h();
                    if(cursor_x >= *x &&
                        cursor_y >= *y &&
                        cursor_x < *x + *w &&
                        cursor_y < *y + *h)
                    {
                        return 1;
                    }
                    break;
                }
                case IS_BAR:
// test bar for 1st staff only
                if(*staff_n == 0)
                {
                    DrawObject *image = group->images.get(0);
                    *image_n = 0;
                    *x = image->abs_x - 1;
                    *y = image->abs_y;
                    *w = image->w;
                    *h = image->h;
//printf("Score::find_delete_object %d %d %d %d %d\n",
//__LINE__, *x, *y, *w, *h);
                    if(cursor_x >= *x &&
                        cursor_y >= *y &&
                        cursor_x < *x + *w &&
                        cursor_y < *y + *h)
                    {
                        return 1;
                    }
                    break;
                }
                case IS_CHORD:
                    for(*image_n = 0; *image_n < group->images.size(); (*image_n)++)
                    {
                        DrawObject *image = group->images.get(*image_n);
                        if(image->pixmap == Capture::instance->quarter)
                        {
                            *x = image->abs_x;
                            *y = image->abs_y;
                            *w = image->pixmap->get_w();
                            *h = image->pixmap->get_h();
                            if(cursor_x >= *x &&
                                cursor_y >= *y &&
                                cursor_x < *x + *w &&
                                cursor_y < *y + *h)
                            {
//printf("Score::find_delete_object %d cursor_x=%d cursor_y=%d staff_n=%d group_n=%d image_n=%d\n", 
//__LINE__, cursor_x, cursor_y, *staff_n, *group_n, *image_n);
                                return 1;
                            }
                        }
                    }
                    break;
            }
        }
    }
    return 0;
}

// delete an object & shift all objects after the deletion
// doesn't delete 8vas
// shift all of the later groups except bars left
// shrink the 8vas
// if staff is 0, delete the object & shift in all the staves
void Score::delete_object(Staff *staff, double time, double length)
{
// delete the object
    for(int i = 0; i < staves.size(); i++)
    {
        Staff *staff2 = staves.get(i);
        if(staff == 0 || staff == staff2)
        {
            for(int j = 0; j < staff2->groups.size(); j++)
            {
                Group *group2 = staff2->groups.get(j);

                if(group2->time == time &&
                    group2->type != IS_OCTAVE)
                {
                    staff2->groups.remove_object_number(j);
                    j--;
                }

// shift all objects left
                if(group2->time > time)
                    group2->time -= length;

// shrink 8va
                if(group2->type == IS_OCTAVE &&
                    group2->time <= time &&
                    group2->time + group2->length > time)
                {
                    group2->length -= length;
                }
            }
        }
    }

// shift bars in lower staves to follow deletions in the top staff
    if(staff == staves.get(0))
    {
        for(int i = 1; i < staves.size(); i++)
        {
            Staff *staff2 = staves.get(i);
            for(int j = 1; j < staff2->groups.size(); j++)
            {
                Group *group2 = staff2->groups.get(j);
                if(group2->time > time && group2->type == IS_BAR)
                {
                    Group *prev = staff2->groups.get(j - 1);
                    double prev_time = prev->time;
                    double next_time = group2->time;
                    staff2->groups.set(j - 1, group2);
                    staff2->groups.set(j, prev);
                    prev->time = next_time;
                    group2->time = prev_time;
                }
            }
        }
    }
    else
    if(staff != 0)
// shift bars in a single lower staff right to ignore deletion
    {
        for(int j = staff->groups.size() - 1; j >= 0; j--)
        {
            Group *group2 = staff->groups.get(j);
            if(group2->time > time && group2->type == IS_BAR)
            {
// last object is a bar
                if(j == staff->groups.size() - 1)
                    group2->time += 1;
                else
                {
                    Group *next = staff->groups.get(j + 1);
                    double prev_time = group2->time;
                    double next_time = next->time;
                    staff->groups.set(j + 1, group2);
                    staff->groups.set(j, next);
                    group2->time = next_time;
                    next->time = prev_time;
                }
            }
        }
    }
}

// shift objects before an insertion
// if staff is 0, shift in all the staves
// if object is nonzero, object is inserted in the staff after an adjustment
// to its time to reflect an insert on top of a bar
void Score::insert_object(Staff *staff, Group *object, double time, double length)
{
    for(int i = 0; i < staves.size(); i++)
    {
        Staff *staff2 = staves.get(i);
        if(staff == 0 || staff == staff2)
        {
            for(int j = 0; j < staff2->groups.size(); j++)
            {
                Group *group2 = staff2->groups.get(j);
// shift all objects right
                if(group2->time >= time)
                    group2->time += length;
// extend 8va
                if(group2->type == IS_OCTAVE &&
                    group2->time <= time &&
                    group2->time + group2->length > time)
                {
                    group2->length += length;
                }
            }
        }
    }

// insert single group after the shift
    if(object != 0 && staff != 0)
    {
        staff->insert_before(time, object);
    }

// shift bars in lower staves right to follow insertions in the top staff
    if(staff == staves.get(0))
    {
        for(int i = 1; i < staves.size(); i++)
        {
            Staff *staff2 = staves.get(i);
            for(int j = staff2->groups.size() - 1; j >= 0; j--)
            {
                Group *group2 = staff2->groups.get(j);
                if(group2->time >= time && group2->type == IS_BAR)
                {
// last object is a bar
                    if(j == staff2->groups.size() - 1)
                        group2->time += 1;
                    else
                    {
                        Group *next = staff2->groups.get(j + 1);
                        double prev_time = group2->time;
                        double next_time = next->time;
                        staff2->groups.set(j + 1, group2);
                        staff2->groups.set(j, next);
                        group2->time = next_time;
                        next->time = prev_time;
                    }
                }
            }
        }
    }
    else
    if(staff != 0)
// shift bars in a single lower staff left to ignore insertion
    {
        for(int j = 1; j < staff->groups.size(); j++)
        {
            Group *group2 = staff->groups.get(j);
            if(group2->time >= time && group2->type == IS_BAR)
            {
                Group *prev = staff->groups.get(j - 1);
                double prev_time = prev->time;
                double next_time = group2->time;
                staff->groups.set(j - 1, group2);
                staff->groups.set(j, prev);
                prev->time = next_time;
                group2->time = prev_time;
            }
        }
    }
}




Staff::Staff()
{
    reset(0);
}

Staff::~Staff()
{
    groups.remove_all_objects();
}

void Staff::reset(int all)
{
    current_cleff = TREBLE;
    current_8va = 0;
    current_cleff_obj = 0;
    current_key = KEY_C;
    beat_start = 0;
    beat_end = 0;
    bzero(accidentals, sizeof(int) * MAJOR_SCALE);
    if(all)
    {
        for(int i = 0; i < groups.size(); i++)
        {
            Group *group = groups.get(i);
            for(int j = 0; j < group->images.size(); j++)
            {
                DrawObject *image = group->images.get(j);
                image->abs_x = image->abs_y = 0x7fffffff;
                image->note = 0;
            }
        }
    }
}

void Staff::copy_from(Staff *src)
{
    for(int i = 0; i < src->groups.size(); i++)
    {
        Group *group = this->groups.append(new Group);
        group->copy_from(src->groups.get(i));
    }
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

// fudge it if the last group is length 0
//         if(i == groups.size() - 1 && group->length == 0)
//             end = group->time + 1;
        if(end > result) result = end;
    }
    return result;
}

Group* Staff::insert_before(double time, Group *new_group)
{
    for(int i = 0; i < groups.size(); i++)
    {
        Group *group = groups.get(i);
        if(group->time >= time)
        {
            groups.insert(new_group, i);
            return new_group;
        }
    }

// append it instead
    groups.append(new_group);
    return new_group;
}

int Staff::get_by_time(double time)
{
    for(int i = 0; i < groups.size(); i++)
    {
        Group *group = groups.get(i);
        if(group->time == time)
            return i;
    }
    return -1;
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

void Group::copy_from(Group *src)
{
    this->time = src->time;
    this->length = src->length;
    this->type = src->type;
    this->cleff = src->cleff;
    this->key = src->key;
    this->octave = src->octave;
    for(int i = 0; i < src->notes.size(); i++)
    {
        Note *note = this->notes.append(new Note);
        note->copy_from(src->notes.get(i));
    }
}

void Group::dump()
{
    printf("Group::dump %d type=%s time=%f length=%f octave=%d\n",
        __LINE__,
        type_to_text(type),
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

int Group::get_h()
{
    int min_y = 0x7fffffff;
    int max_y = -0x7fffffff;
    for(int i = 0; i < images.size(); i++)
    {
        int y1 = images.get(i)->y;
        int y2 = images.get(i)->get_y2();
        if(y1 < min_y) min_y = y1;
        if(y2 > max_y) max_y = y2;
    }
    return max_y - min_y;
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

int Group::get_abs_x()
{
    int min_x = 0x7fffffff;
    for(int i = 0; i < images.size(); i++)
    {
        int x1 = images.get(i)->abs_x;
        if(x1 < min_x) min_x = x1;
    }
    return min_x;
}

int Group::get_abs_y()
{
    int min_y = 0x7fffffff;
    for(int i = 0; i < images.size(); i++)
    {
        int y1 = images.get(i)->abs_y;
        if(y1 < min_y) min_y = y1;
    }
    return min_y;
}



// sort from lowest to highest to simplify drawing
void Group::append(Note *note)
{
    for(int i = 0; i < notes.size(); i++)
    {
        if(notes.get(i)->pitch > note->pitch)
        {
            notes.insert(note, i);
            return;
        }
        else
        if(notes.get(i)->pitch == note->pitch)
        {
            printf("Group::append %d exists\n", __LINE__);
            delete note;
            return;
        }
    }

    notes.append(note);
}

int Group::save(FILE *fd)
{
    int result = 0;
    fprintf(fd, "    GROUP TYPE=%s TIME=%f LENGTH=%f CLEFF=%d KEY=%d OCTAVE=%d\n",
        type_to_text(type),
        time,
        length,
        cleff,
        key,
        octave);

    for(int i = 0; i < notes.size() && !result; i++)
        result = notes.get(i)->save(fd);
    return result;
}


static const char* type_text[] = 
{
    "BAR",
    "CLEFF",
    "KEY",
    "OCTAVE",
    "CHORD",
    "REST"
};

const char* Group::type_to_text(int type)
{
    if(type < sizeof(type_text) / sizeof(char*))
        return type_text[type];
    return "";
}

int Group::text_to_type(const char *type)
{
    for(int i = 0; i < sizeof(type_text) / sizeof(char*); i++)
    {
        if(!strcasecmp(type, type_text[i])) return i;
    }
    return 0;
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

void Note::copy_from(Note *src)
{
    this->accidental = src->accidental;
    this->pitch = src->pitch;
    this->duration = src->duration;
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







