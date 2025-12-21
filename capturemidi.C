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
#include "capturemidi.h"
#include "sema.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#define MIDI_DEVICE "/dev/midi1"
#define MIDI_NOTEON 0x90


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
    command_ready = new Sema;
    command_done = new Sema;
    done = 0;
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
            while(!done)
            {
                uint8_t packet[4];
                bzero(packet, 4);
                int result = read(fd, packet, sizeof(packet));
                if(result <= 0)
                {
                    printf("CaptureMIDI::run %d: unplugged\n", __LINE__);
                    break;
                }


                switch(packet[0])
                {
                    case 0xb0:
                        if(packet[1] == 0x40)
                        {
// right pedal
                            int value = packet[2];
                            if(value == 0 && right_pedal)
                            {
// up
                                right_pedal = 0;
                            }
                            else
                            if(value != 0 && !right_pedal)
                            {
// down
                                right_pedal = 1;
                                Capture::instance->next_beat();
                            }
                        }
                        else
                        if(packet[1] = 0x43)
                        {
// left pedal
                            int value = packet[2];
                            if(value == 0 && left_pedal)
                            {
// up
                                left_pedal = 0;
                            }
                            else
                            if(value != 0 && !left_pedal)
                            {
// down
                                left_pedal = 1;
                                Capture::instance->prev_beat();
                            }
                        }
                        break;
                    case 0x90:
                    {
// key event
                        int note = packet[1];
                        int velocity = packet[2];
                        if(velocity > 0)
                        {
// down
                            
                        }
                        break;
                    }
                }

//                if(packet[0] != 0xf8 && packet[0] != 0xfe)
                    printf("0x%02x, 0x%02x, 0x%02x, 0x%02x,\n", 
                        packet[0],
                        packet[1],
                        packet[2],
                        packet[3]);
                fflush(stdout);
            }

            close(fd);
        }
        
        command_done->unlock();
    }
}

void CaptureMIDI::start_recording()
{
    done = 0;
    command_ready->unlock();
}

void CaptureMIDI::stop_recording()
{
    done = 1;
    command_done->lock();
}






