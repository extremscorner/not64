/**
 * Mupen64 - bc.c
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
#include "ops.h"
#include "macros.h"
#include "interupt.h"

void BC1F()
{
   if (check_cop1_unusable()) return;
   PC++;
   delay_slot=1;
   PC->ops();
   update_count();
   delay_slot=0;
   if ((FCR31 & 0x800000)==0 && !skip_jump)
     PC += (PC-2)->f.i.immediate-1;
   last_addr = PC->addr;
   if (next_interupt <= Count) gen_interupt();
}

void BC1F_OUT()
{
   if (check_cop1_unusable()) return;
   jump_target = (long)PC->f.i.immediate;
   PC++;
   delay_slot=1;
   PC->ops();
   update_count();
   delay_slot=0;
   if (!skip_jump && (FCR31 & 0x800000)==0)
     jump_to(PC->addr + ((jump_target-1)<<2));
   last_addr = PC->addr;
   if (next_interupt <= Count) gen_interupt();
}

void BC1F_IDLE()
{
   long skip;
   if ((FCR31 & 0x800000)==0)
     {
	update_count();
	skip = next_interupt - Count;
	if (skip > 3) Count += (skip & 0xFFFFFFFC);
	else BC1F();
     }
   else BC1F();
}

void BC1T()
{
   if (check_cop1_unusable()) return;
   PC++;
   delay_slot=1;
   PC->ops();
   update_count();
   delay_slot=0;
   if ((FCR31 & 0x800000)!=0 && !skip_jump)
     PC += (PC-2)->f.i.immediate-1;
   last_addr = PC->addr;
   if (next_interupt <= Count) gen_interupt();
}

void BC1T_OUT()
{
   if (check_cop1_unusable()) return;
   jump_target = (long)PC->f.i.immediate;
   PC++;
   delay_slot=1;
   PC->ops();
   update_count();
   delay_slot=0;
   if (!skip_jump && (FCR31 & 0x800000)!=0)
     jump_to(PC->addr + ((jump_target-1)<<2));
   last_addr = PC->addr;
   if (next_interupt <= Count) gen_interupt();
}

void BC1T_IDLE()
{
   long skip;
   if ((FCR31 & 0x800000)!=0)
     {
	update_count();
	skip = next_interupt - Count;
	if (skip > 3) Count += (skip & 0xFFFFFFFC);
	else BC1T();
     }
   else BC1T();
}

void BC1FL()
{
   if (check_cop1_unusable()) return;
   if ((FCR31 & 0x800000)==0)
     {
	PC++;
	delay_slot=1;
	PC->ops();
	update_count();
	delay_slot=0;
	if(!skip_jump)
	  PC += (PC-2)->f.i.immediate-1;
     }
   else
     PC+=2;
   last_addr = PC->addr;
   if (next_interupt <= Count) gen_interupt();
}

void BC1FL_OUT()
{
   if (check_cop1_unusable()) return;
   if ((FCR31 & 0x800000)==0)
     {
	jump_target = (long)PC->f.i.immediate;
	PC++;
	delay_slot=1;
	PC->ops();
	update_count();
	delay_slot=0;
	if (!skip_jump)
	  jump_to(PC->addr + ((jump_target-1)<<2));
     }
   else
     PC+=2;
   last_addr = PC->addr;
   if (next_interupt <= Count) gen_interupt();
}

void BC1FL_IDLE()
{
   long skip;
   if ((FCR31 & 0x800000)==0)
     {
	update_count();
	skip = next_interupt - Count;
	if (skip > 3) Count += (skip & 0xFFFFFFFC);
	else BC1FL();
     }
   else BC1FL();
}

void BC1TL()
{
   if (check_cop1_unusable()) return;
   if ((FCR31 & 0x800000)!=0)
     {
	PC++;
	delay_slot=1;
	PC->ops();
	update_count();
	delay_slot=0;
	if(!skip_jump)
	  PC += (PC-2)->f.i.immediate-1;
     }
   else
     PC+=2;
   last_addr = PC->addr;
   if (next_interupt <= Count) gen_interupt();
}

void BC1TL_OUT()
{
   if (check_cop1_unusable()) return;
   if ((FCR31 & 0x800000)!=0)
     {
	jump_target = (long)PC->f.i.immediate;
	PC++;
	delay_slot=1;
	PC->ops();
	update_count();
	delay_slot=0;
	if (!skip_jump)
	  jump_to(PC->addr + ((jump_target-1)<<2));
     }
   else
     PC+=2;
   last_addr = PC->addr;
   if (next_interupt <= Count) gen_interupt();
}

void BC1TL_IDLE()
{
   long skip;
   if ((FCR31 & 0x800000)!=0)
     {
	update_count();
	skip = next_interupt - Count;
	if (skip > 3) Count += (skip & 0xFFFFFFFC);
	else BC1TL();
     }
   else BC1TL();
}
