/**
 * Wii64 - Wrappers.h
 * Copyright (C) 2007, 2008, 2009, 2010 Mike Slegeir
 * 
 * Interface between emulator code and recompiled code
 *
 * Wii64 homepage: http://www.emulatemii.com
 * email address: tehpola@gmail.com
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
**/

#ifndef WRAPPERS_H
#define WRAPPERS_H

#include "Recompile.h"

#define DYNAREG_RDRAM  29
#define DYNAREG_FUNC   30
#define DYNAREG_ZERO   31

#define DYNAOFF_LR     12

#define SDAREL(symbol) ({ short offset; \
	__asm__("li %0,%1@sdarel" : "=r" (offset) : "U" (&symbol)); \
	offset; })

extern long long int reg[34] __attribute__((section(".sbss")));
extern unsigned long reg_cop0[32] __attribute__((section(".sbss")));
extern float*  reg_cop1_simple[32] __attribute__((section(".sbss")));
extern double* reg_cop1_double[32] __attribute__((section(".sbss")));

extern int noCheckInterrupt;

typedef enum { MEM_LW,   MEM_LH,   MEM_LB,   MEM_LD,
               MEM_LWU,  MEM_LHU,  MEM_LBU,
               MEM_LWC1, MEM_LDC1, MEM_LL,
               MEM_LWL,  MEM_LWR,  MEM_LDL,  MEM_LDR,
               MEM_SW,   MEM_SH,   MEM_SB,   MEM_SD,
               MEM_SWC1, MEM_SDC1, MEM_SC,
               MEM_SWL,  MEM_SWR,  MEM_SDL,  MEM_SDR } memType;

unsigned int decodeNInterpret(MIPS_instr, unsigned int, int);
#ifdef COMPARE_CORE
int dyna_update_count(unsigned int pc, int isDelaySlot);
#else
int dyna_update_count(unsigned int pc);
#endif
unsigned int dyna_check_cop1_unusable(unsigned int pc, int isDelaySlot);
void invalidate_func(unsigned int addr);
unsigned int dyna_mem(unsigned int addr, unsigned int value, int count,
                      memType type, unsigned int pc, int isDelaySlot);

//cop0 macros
#define Index reg_cop0[0]
#define Random reg_cop0[1]
#define EntryLo0 reg_cop0[2]
#define EntryLo1 reg_cop0[3]
#define Context reg_cop0[4]
#define PageMask reg_cop0[5]
#define Wired reg_cop0[6]
#define BadVAddr reg_cop0[8]
#define Count reg_cop0[9]
#define EntryHi reg_cop0[10]
#define Compare reg_cop0[11]
#define Status reg_cop0[12]
#define Cause reg_cop0[13]
#define EPC reg_cop0[14]
#define PRevID reg_cop0[15]
#define Config reg_cop0[16]
#define LLAddr reg_cop0[17]
#define WatchLo reg_cop0[18]
#define WatchHi reg_cop0[19]
#define XContext reg_cop0[20]
#define PErr reg_cop0[26]
#define CacheErr reg_cop0[27]
#define TagLo reg_cop0[28]
#define TagHi reg_cop0[29]
#define ErrorEPC reg_cop0[30]

#endif

