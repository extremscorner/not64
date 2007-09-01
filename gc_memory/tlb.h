/**
 * Mupen64 - tlb.h
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

#ifndef TLB_H
#define TLB_H

typedef struct _tlb
{
   short mask;
   long vpn2;
   char g;
   unsigned char asid;
   long pfn_even;
   char c_even;
   char d_even;
   char v_even;
   long pfn_odd;
   char c_odd;
   char d_odd;
   char v_odd;
   char r;
   //long check_parity_mask;
   
   unsigned long start_even;
   unsigned long end_even;
   unsigned long phys_even;
   unsigned long start_odd;
   unsigned long end_odd;
   unsigned long phys_odd;
} tlb;

extern unsigned long tlb_LUT_r[0x100000];
extern unsigned long tlb_LUT_w[0x100000];
unsigned long virtual_to_physical_address(unsigned long addresse, int w);
int probe_nop(unsigned long address);

#endif
