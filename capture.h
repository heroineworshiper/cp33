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

// notation capture database

#ifndef CAPTURE_H
#define CAPTURE_H


#include "bcpixmap.inc"
#include "bcwindowbase.inc"

extern int current_staff;
extern double selection_start;  // beat
extern double selection_end;


extern double current_beat;
extern int current_score_page;
extern char score_path[BCTEXTLEN];
extern int score_changed;
extern int save_score();
extern int load_score(char *path);
extern void prev_beat();
extern void next_beat();
extern void prev_staff();
extern void next_staff();


#endif



