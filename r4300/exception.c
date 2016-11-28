/**
 * Mupen64 - exception.c
 * Copyright (C) 2002 Hacktarux
 *               2010 emu_kidid
 *
 * Mupen64 homepage: http://mupen64.emulation64.com
 * email address: hacktarux@yahoo.fr
 *                emu_kidid@gmail.com
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
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 *
**/

#include "r4300.h"
#include "macros.h"
#include "recomph.h"
#include "exception.h"
#include "../gui/DEBUG.h"
#include "../gc_memory/memory.h"

extern unsigned long interp_addr;

#ifdef DEBUGON
#define doBreak() _break();
#else
#define doBreak()
#endif

void address_error_exception()
{
  printf("address_error_exception\n");
  stop=1;
  doBreak();     
}

void TLB_invalid_exception()
{
  if (delay_slot) {
    skip_jump = 1;
    printf("delay slot\nTLB refill exception\n");
    stop=1;
    doBreak();
  }
  printf("TLB invalid exception\n");
  stop=1;
  doBreak();
}

void XTLB_refill_exception(unsigned long long int addresse)
{
  printf("XTLB refill exception\n");
  stop=1;
  doBreak();   
}

void TLB_refill_exception(unsigned long address, int w)
{
  int usual_handler = 0, i = 0;
  
  if (!dynacore && w != 2) {
    update_count();
  }
  Cause = (w == 1) ? (3 << 2):(2 << 2);
  BadVAddr = address;
  Context = (Context & 0xFF80000F) | ((address >> 9) & 0x007FFFF0);
  EntryHi = address & 0xFFFFE000;
  if (Status & 0x2) { // Test de EXL
    interp_addr = 0x80000180;
    if(delay_slot==1 || delay_slot==3) {
      Cause |= 0x80000000;
    }
    else {
      Cause &= 0x7FFFFFFF;
    }
  }
  else {
    if (dynacore && w == 2) {
      EPC = address;
    }
    else {
      EPC = interp_addr;
    }
       
    Cause &= ~0x80000000;
    Status |= 0x2; //EXL=1

    if (address >= 0x80000000 && address < 0xc0000000) {
      usual_handler = 1;
    }
    for (i=0; i<32; i++) {
      if (address >= tlb_e[i].start_even && address <= tlb_e[i].end_even) {
        usual_handler = 1;
      }
      if (address >= tlb_e[i].start_odd && address <= tlb_e[i].end_odd) {
        usual_handler = 1;
      }
    }
    if (usual_handler) {
      interp_addr = 0x80000180;
    }
    else {
      interp_addr = 0x80000000;
    }
  }
  
  if(delay_slot==1 || delay_slot==3) {
    Cause |= 0x80000000;
    EPC-=4;
  }
  else {
    Cause &= 0x7FFFFFFF;
  }
  if (w != 2) EPC -= 4;
  
  last_addr = interp_addr;
  
  if (delay_slot) {
    skip_jump = interp_addr;
    next_interupt = 0;
  }
}

void TLB_mod_exception()
{
  printf("TLB mod exception\n");
  stop=1;
  doBreak();
}

void integer_overflow_exception()
{
  printf("integer overflow exception\n");
  stop=1;
  doBreak();
}

void coprocessor_unusable_exception()
{
  printf("coprocessor_unusable_exception\n");
  stop=1;
  doBreak();
}

void exception_general()
{
  update_count();
  Status |= 2;
   
  EPC = interp_addr;

  if(delay_slot==1 || delay_slot==3) {
    Cause |= 0x80000000;
    EPC-=4;
  }
  else {
    Cause &= 0x7FFFFFFF;
  }
  
  interp_addr = 0x80000180;
  last_addr = interp_addr;
  
  if (delay_slot) {
    skip_jump = interp_addr;
    next_interupt = 0;
  }
}
