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

static long long time_in_section[NUM_SECTIONS+1];
static long long last_start[NUM_SECTIONS+1];
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
   if(diff_sec(last_refresh, this_tick) >= 1)
     {
	time_in_section[0] = this_tick - last_start[0];
	
	sprintf(txtbuffer, "gfx=%f%%", 100.0f * (float)time_in_section[GFX_SECTION] / (float)time_in_section[0]);
	DEBUG_print(txtbuffer, DBG_PROFILE_GFX);
	
	sprintf(txtbuffer, "audio=%f%%", 100.0f * (float)time_in_section[AUDIO_SECTION] / (float)time_in_section[0]);
	DEBUG_print(txtbuffer, DBG_PROFILE_AUDIO);
	
	sprintf(txtbuffer, "tlb=%f%%", 100.0f * (float)time_in_section[TLB_SECTION] / (float)time_in_section[0]);
	DEBUG_print(txtbuffer, DBG_PROFILE_TLB);
	
	sprintf(txtbuffer, "fp=%f%%", 100.0f * (float)time_in_section[FP_SECTION] / (float)time_in_section[0]);
	DEBUG_print(txtbuffer, DBG_PROFILE_FP);
	
	sprintf(txtbuffer, "comp=%f%%", 100.0f * (float)time_in_section[COMPILER_SECTION] / (float)time_in_section[0]);
	DEBUG_print(txtbuffer, DBG_PROFILE_COMP);
	
	sprintf(txtbuffer, "interp=%f%%", 100.0f * (float)time_in_section[INTERP_SECTION] / (float)time_in_section[0]);
	DEBUG_print(txtbuffer, DBG_PROFILE_INTERP);
	
	sprintf(txtbuffer, "tramp=%f%%", 100.0f * (float)time_in_section[TRAMP_SECTION] / (float)time_in_section[0]);
	DEBUG_print(txtbuffer, DBG_PROFILE_TRAMP);
	
	int i;
	for(i=1; i<=NUM_SECTIONS; ++i) time_in_section[i] = 0;
	last_start[0] = this_tick;
	last_refresh = this_tick;
     }
}

#endif
