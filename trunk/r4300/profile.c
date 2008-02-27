/**
 * Mupen64 - profile.c
 * Copyright (C) 2002 Hacktarux
 *
 * Mupen64 homepage: http://mupen64.emulation64.com
 * email address: hacktarux@yahoo.fr
 * 
 * If you want to contribute to the project please contact
 * me first (maybe someone is already making what you are
 * planning to do).
 *
 *
 * This program is free software; you can redistribute it and/
 * or modify it under the terms of the GNU General Public Li-
 * cence as published by the Free Software Foundation; either
 * version 2 of the Licence, or any later version.
 *
 * This program is distributed in the hope that it will be use-
 * ful, but WITHOUT ANY WARRANTY; without even the implied war-
 * ranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public Licence for more details.
 *
 * You should have received a copy of the GNU General Public
 * Licence along with this program; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139,
 * USA.
 *
**/

#include "r4300.h"
#include "sys/time.h"
#include "../gui/DEBUG.h"

#ifdef PROFILE

extern long long gettime();
extern unsigned int diff_sec(long long, long long);

static long long time_in_section[5];
static long long last_start[5];
static long long last_refresh;

void start_section(int section_type)
{
   last_start[section_type] = gettime();
}

void end_section(int section_type)
{
   long long end = gettime();
   time_in_section[section_type] += end - last_start[section_type];
}

void refresh_stat()
{
   long long this_tick = gettime();
   if(diff_sec(this_tick, last_refresh) >= 2)
     {
	time_in_section[0] = this_tick - last_start[0];
	
	sprintf(txtbuffer,
	        "gfx=%f%% - audio=%f%% - compiler=%f%%, idle=%f%%\r",
	         100.0f * (float)time_in_section[1] / (float)time_in_section[0],
	         100.0f * (float)time_in_section[2] / (float)time_in_section[0],
	         100.0f * (float)time_in_section[3] / (float)time_in_section[0],
	         100.0f * (float)time_in_section[4] / (float)time_in_section[0]);
	DEBUG_print(txtbuffer, DBG_PROFILE);
	
	time_in_section[1] = 0;
	time_in_section[2] = 0;
	time_in_section[3] = 0;
	time_in_section[4] = 0;
	last_start[0] = this_tick;
	last_refresh = this_tick;
     }
}

#endif
