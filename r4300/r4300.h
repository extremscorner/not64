/**
 * Mupen64 - r4300.h
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

#ifndef R4300_H
#define R4300_H

#include <stdio.h>
#include <string.h>
#include "../main/rom.h"
#include "../gc_memory/tlb.h"
#include "recomp.h"

extern precomp_instr *PC;
#ifdef PPC_DYNAREC
#include "ppc/Recompile.h"
#ifdef HW_RVL
extern PowerPC_block **blocks; 
#else
extern PowerPC_block *blocks[0x100000];
#endif
extern PowerPC_block *actual;
#else
extern precomp_block *blocks[0x100000], *actual;
#endif
extern int stop, llbit;
extern long long int reg[34];
#define hi (reg[32])
#define lo (reg[33])
extern long long int local_rs, local_rt;
extern unsigned long reg_cop0[32];
extern long local_rs32, local_rt32;
extern unsigned long jump_target;
extern double *reg_cop1_double[32];
extern float *reg_cop1_simple[32];
extern long reg_cop1_fgr_32[32];
extern long long int reg_cop1_fgr_64[32];
extern long FCR0, FCR31;
extern tlb tlb_e[32];
extern unsigned long delay_slot, skip_jump, dyna_interp;
extern unsigned long long int debug_count;
extern unsigned long dynacore;
extern unsigned long interpcore;
extern unsigned int next_interupt, CIC_Chip;
extern int rounding_mode, trunc_mode, round_mode, ceil_mode, floor_mode;
extern unsigned long last_addr, interp_addr;
//extern char invalid_code[0x100000];
extern unsigned long jump_to_address;
extern int no_audio_delay;
extern int no_compiled_jump;

void cpu_init(void);
void cpu_deinit(void);
void go();
void pure_interpreter();
void compare_core();
inline void jump_to_func();
void update_count();
int check_cop1_unusable();

#ifndef PPC_DYNAREC
#define jump_to(a) { jump_to_address = a; jump_to_func(); }
#else
void jump_to(unsigned int);
#endif

#ifdef __PPC__
#define dyna_start(x)
#define dyna_jump()
#define dyna_stop()
#endif

// profiling

#define GFX_SECTION 1
#define AUDIO_SECTION 2
#define COMPILER_SECTION 3
#define IDLE_SECTION 4
#define TLB_SECTION 5
#define FP_SECTION 6
#define INTERP_SECTION 7
#define TRAMP_SECTION 8
#define FUNCS_SECTION 9
#define NUM_SECTIONS 9

//#define PROFILE

#ifdef PROFILE

void start_section(int section_type);
void end_section(int section_type);
void refresh_stat();

#else

#define start_section(a)
#define end_section(a)
#define refresh_stat()

#endif

#endif

