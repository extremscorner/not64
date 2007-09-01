/**
 * Mupen64 - regimm.c
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
#include "interupt.h"
#include "../gc_memory/memory.h"
#include "ops.h"
#include "macros.h"

void BLTZ()
{
   local_rs = irs;
   PC++;
   delay_slot=1;
   PC->ops();
   update_count();
   delay_slot=0;
   if (local_rs < 0 && !skip_jump)
     PC += (PC-2)->f.i.immediate-1;
   last_addr = PC->addr;
   if (next_interupt <= Count) gen_interupt();
}

void BLTZ_OUT()
{
   local_rs = irs;
   jump_target = (long)PC->f.i.immediate;
   PC++;
   delay_slot=1;
   PC->ops();
   update_count();
   delay_slot=0;
   if (!skip_jump && local_rs < 0)
     jump_to(PC->addr + ((jump_target-1)<<2));
   last_addr = PC->addr;
   if (next_interupt <= Count) gen_interupt();
}

void BLTZ_IDLE()
{
   long skip;
   if (irs < 0)
     {
	update_count();
	skip = next_interupt - Count;
	if (skip > 3) Count += (skip & 0xFFFFFFFC);
	else BLTZ();
     }
   else BLTZ();
}

void BGEZ()
{
   local_rs = irs;
   PC++;
   delay_slot=1;
   PC->ops();
   update_count();
   delay_slot=0;
   if (local_rs >= 0 && !skip_jump)
     PC += (PC-2)->f.i.immediate - 1;
   last_addr = PC->addr;
   if (next_interupt <= Count) gen_interupt();
}

void BGEZ_OUT()
{
   local_rs = irs;
   jump_target = (long)PC->f.i.immediate;
   PC++;
   delay_slot=1;
   PC->ops();
   update_count();
   delay_slot=0;
   if (!skip_jump && local_rs >= 0)
     jump_to(PC->addr + ((jump_target-1)<<2));
   last_addr = PC->addr;
   if (next_interupt <= Count) gen_interupt();
}

void BGEZ_IDLE()
{
   long skip;
   if (irs >= 0)
     {
	update_count();
	skip = next_interupt - Count;
	if (skip > 3) Count += (skip & 0xFFFFFFFC);
	else BGEZ();
     }
   else BGEZ();
}

void BLTZL()
{
   if (irs < 0)
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

void BLTZL_OUT()
{
   if (irs < 0)
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

void BLTZL_IDLE()
{
   long skip;
   if (irs < 0)
     {
	update_count();
	skip = next_interupt - Count;
	if (skip > 3) Count += (skip & 0xFFFFFFFC);
	else BLTZL();
     }
   else BLTZL();
}

void BGEZL()
{
   if (irs >= 0)
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

void BGEZL_OUT()
{
   if (irs >= 0)
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

void BGEZL_IDLE()
{
   long skip;
   if (irs >= 0)
     {
	update_count();
	skip = next_interupt - Count;
	if (skip > 3) Count += (skip & 0xFFFFFFFC);
	else BGEZL();
     }
   else BGEZL();
}

void BLTZAL()
{
   local_rs = irs;
   reg[31]=PC->addr+8;
   if((&irs)!=(reg+31))
     {
	PC++;
	delay_slot=1;
	PC->ops();
	update_count();
	delay_slot=0;
	if(local_rs < 0 && !skip_jump)
	  PC += (PC-2)->f.i.immediate-1;
     }
   else printf("erreur dans bltzal\n");
   last_addr = PC->addr;
   if (next_interupt <= Count) gen_interupt();
}

void BLTZAL_OUT()
{
   local_rs = irs;
   reg[31]=PC->addr+8;
   if((&irs)!=(reg+31))
     {
	jump_target = (long)PC->f.i.immediate;
	PC++;
	delay_slot=1;
	PC->ops();
	update_count();
	delay_slot=0;
	if(!skip_jump && local_rs < 0)
	  jump_to(PC->addr + ((jump_target-1)<<2));
     }
   else printf("erreur dans bltzal\n");
   last_addr = PC->addr;
   if (next_interupt <= Count) gen_interupt();
}

void BLTZAL_IDLE()
{
   long skip;
   if (irs < 0)
     {
	update_count();
	skip = next_interupt - Count;
	if (skip > 3) Count += (skip & 0xFFFFFFFC);
	else BLTZAL();
     }
   else BLTZAL();
}

void BGEZAL()
{
   local_rs = irs;
   reg[31]=PC->addr+8;
   if((&irs)!=(reg+31))
     {
	PC++;
	delay_slot=1;
	PC->ops();
	update_count();
	delay_slot=0;
	if(local_rs >= 0 && !skip_jump)
	  PC += (PC-2)->f.i.immediate-1;
     }
   else printf("erreur dans bgezal\n");
   last_addr = PC->addr;
   if (next_interupt <= Count) gen_interupt();
}

void BGEZAL_OUT()
{
   local_rs = irs;
   reg[31]=PC->addr+8;
   if((&irs)!=(reg+31))
     {
	jump_target = (long)PC->f.i.immediate;
	PC++;
	delay_slot=1;
	PC->ops();
	update_count();
	delay_slot=0;
	if(!skip_jump && local_rs >= 0)
	  jump_to(PC->addr + ((jump_target-1)<<2));
     }
   else printf("erreur dans bgezal\n");
   last_addr = PC->addr;
   if (next_interupt <= Count) gen_interupt();
}

void BGEZAL_IDLE()
{
   long skip;
   if (irs >=0)
     {
	update_count();
	skip = next_interupt - Count;
	if (skip > 3) Count += (skip & 0xFFFFFFFC);
	else BGEZAL();
     }
   else BGEZAL();
}

void BLTZALL()
{
   local_rs = irs;
   reg[31]=PC->addr+8;
   if((&irs)!=(reg+31))
     {
	if (local_rs < 0)
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
     }
   else printf("erreur dans bltzall\n");
   last_addr = PC->addr;
   if (next_interupt <= Count) gen_interupt();
}

void BLTZALL_OUT()
{
   local_rs = irs;
   reg[31]=PC->addr+8;
   if((&irs)!=(reg+31))
     {
	if (local_rs < 0)
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
     }
   else printf("erreur dans bltzall\n");
   last_addr = PC->addr;
   if (next_interupt <= Count) gen_interupt();
}

void BLTZALL_IDLE()
{
   long skip;
   if (irs < 0)
     {
	update_count();
	skip = next_interupt - Count;
	if (skip > 3) Count += (skip & 0xFFFFFFFC);
	else BLTZALL();
     }
   else BLTZALL();
}

void BGEZALL()
{
   local_rs = irs;
   reg[31]=PC->addr+8;
   if((&irs)!=(reg+31))
     {
	if (local_rs >= 0)
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
     }
   else printf("erreur dans bgezall\n");
   last_addr = PC->addr;
   if (next_interupt <= Count) gen_interupt();
}

void BGEZALL_OUT()
{
   local_rs = irs;
   reg[31]=PC->addr+8;
   if((&irs)!=(reg+31))
     {
	if (local_rs >= 0)
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
     }
   else printf("erreur dans bgezall\n");
   last_addr = PC->addr;
   if (next_interupt <= Count) gen_interupt();
}

void BGEZALL_IDLE()
{
   long skip;
   if (irs >= 0)
     {
	update_count();
	skip = next_interupt - Count;
	if (skip > 3) Count += (skip & 0xFFFFFFFC);
	else BGEZALL();
     }
   else BGEZALL();
}
