/*
 * MUSIC READER
 * Copyright (C) 2025 Adam Williams <broadcast at earthling dot net>
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

#include "capture.h"
#include "capturemenu.h"
#include "capturemidi.h"
#include "score.h"
#include "sema.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#define MIDI_DEVICE "/dev/midi1"
#define MIDI_NOTE 0x90
#define MIDI_PEDAL 0xb0
    #define MIDI_RIGHT_PEDAL 0x40
    #define MIDI_LEFT_PEDAL 0x43
#define MIDI_ALIVE 0xf8

// midi delays
#define REPEAT_COUNT1 25
#define REPEAT_COUNT2 4
#define MIDI_DEBOUNCE 4

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

CaptureMIDI* CaptureMIDI::instance = 0;

CaptureMIDI::CaptureMIDI()
 : Thread()
{
    command_ready = new Sema(0);
    command_done = new Sema(0);
    done = 1;
}

void CaptureMIDI::initialize()
{
    Thread::start();
}

void CaptureMIDI::run()
{
    while(1)
    {
        command_ready->lock();
        
        int fd = open(MIDI_DEVICE, O_RDWR);
        if(fd < 0)
        {
            printf("CaptureMIDI::run %d: failed to open device %s\n", 
                __LINE__, 
                MIDI_DEVICE);
        }
        else
        {
            int left_pedal = 0;
            int right_pedal = 0;
            int repeat_counter = 0;
            int repeat_time = 0;
            int left_debounce = 0;
            int right_debounce = 0;
#define GET_START_CODE 0
#define GET_DATA 1
            int state = GET_START_CODE;
            int counter = 0;
            int size = 0;
            uint8_t packet[4];
            while(!done)
            {
                uint8_t c;
                int result = read(fd, &c, 1);
                if(result <= 0)
                {
                    printf("CaptureMIDI::run %d: unplugged\n", __LINE__);
                    break;
                }

                if(state == GET_START_CODE)
                {
                    if(c == MIDI_NOTE ||
                        c == MIDI_PEDAL)
                    {
                        state = GET_DATA;
                        bzero(packet, sizeof(packet));
                        packet[0] = c;
                        counter = 1;
                        size = 3;
                    }
                    else
                    if(c == MIDI_ALIVE)
                    {
                        if(repeat_time > 0)
                        {
                            repeat_counter++;
                            if(repeat_counter >= repeat_time)
                            {
                                repeat_counter = 0;
                                repeat_time = REPEAT_COUNT2;
                                if(left_pedal) Capture::instance->prev_beat();
                                if(right_pedal) Capture::instance->next_beat();
                            }
                        }
                        if(left_debounce > 0) left_debounce--;
                        if(right_debounce > 0) right_debounce--;
                    }
                }
                else
                {
                    packet[counter++] = c;
                    if(counter >= size)
                    {
                        state = GET_START_CODE;

                        switch(packet[0])
                        {
                            case MIDI_PEDAL:
                                if(packet[1] == MIDI_RIGHT_PEDAL)
                                {
        // right pedal
                                    int value = packet[2];
                                    if(value == 0 && right_pedal)
                                    {
        // up
                                        right_pedal = 0;
                                        repeat_time = 0;
                                    }
                                    else
                                    if(value > 0x70 && !right_pedal && right_debounce <= 0)
                                    {
        // down
                                        right_pedal = 1;
                                        left_pedal = 0;
                                        repeat_time = REPEAT_COUNT1;
                                        repeat_counter = 0;
                                        right_debounce = MIDI_DEBOUNCE;
                                        Capture::instance->next_beat();
                                    }
                                }
                                else
                                if(packet[1] = MIDI_LEFT_PEDAL)
                                {
        // left pedal
                                    int value = packet[2];
                                    if(value == 0 && left_pedal)
                                    {
        // up
                                        left_pedal = 0;
                                        repeat_time = 0;
                                    }
                                    else
                                    if(value > 0x70 && !left_pedal && left_debounce <= 0)
                                    {
        // down
                                        left_pedal = 1;
                                        right_pedal = 0;
                                        repeat_time = REPEAT_COUNT1;
                                        repeat_counter = 0;
                                        left_debounce = MIDI_DEBOUNCE;
                                        Capture::instance->prev_beat();
                                    }
                                }
                                break;

                            case MIDI_NOTE:
                            {
        // key event
                                int note = packet[1];
                                int velocity = packet[2];
                                if(velocity > 0)
                                {
        // down
        // test for chord at the current position
                                    Score *score = Score::instance;
                                    int got_it = 0;
                                    Group *group = 0;
                                    if(current_staff < score->staves.size())
                                    {
                                        Capture::instance->push_undo_before();

                                        Staff *staff = score->staves.get(current_staff);
                                        for(int i = 0; i < staff->groups.size(); i++)
                                        {
                                            group = staff->groups.get(i);
                                            if(group->time == selection_start)
                                            {
                                                got_it = 1;
                                                if(group->type != IS_CHORD)
                                                {
// insert before the beat & shift all right
                                                    group = new Group(selection_start, IS_CHORD);
                                                    score->insert_object(staff, group, group->time, group->length);
                                                }
                                                break;
                                            }
                                            else
                                            if(group->time > selection_start)
                                            {
// insert before the beat & don't shift right
// shouldn't get here, as the score is padded with rests
                                                got_it = 1;
                                                group = new Group(selection_start, IS_CHORD);
                                                staff->groups.insert(group, i);
                                                break;
                                            }
                                        }
// after end of staff
                                        if(!got_it)
                                        {
                                            group = new Group(selection_start, IS_CHORD);
                                            staff->groups.append(group);
                                            got_it = 1;
                                        }

                                        if(got_it)
                                        {
                                            group->append(new Note(0, note));
                                            score->clean();
                                            score_changed = 1;
                                            CaptureMenu::instance->update_save();
                                            Capture::instance->draw_score(1, 1);
                                        }

                                        Capture::instance->push_undo_after(1);
                                    }
                                }
                                break;
                            }
                        }
                    }
                }


//                 if(packet[0] != 0xf8 && packet[0] != 0xfe)
//                     printf("0x%02x, 0x%02x, 0x%02x, 0x%02x,\n", 
//                         packet[0],
//                         packet[1],
//                         packet[2],
//                         packet[3]);
//                 fflush(stdout);
            }

            close(fd);
        }
        
        command_done->unlock();
    }
}

void CaptureMIDI::start_recording()
{
    if(done)
    {
        done = 0;
        command_ready->unlock();
    }
}

void CaptureMIDI::stop_recording()
{
    if(!done)
    {
        done = 1;
        command_done->lock();
    }
}






