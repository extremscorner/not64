/**
 * Mupen64 - r4300.c
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
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 *
**/

#include "../config.h"
#include "../main/ROM-Cache.h"
#include "r4300.h"
#include "ops.h"
#include "../gc_memory/memory.h"
#include "exception.h"
#include "interupt.h"
#include "macros.h"
#include "recomp.h"
#include "recomph.h"
#include "Invalid_Code.h"
#include "Recomp-Cache.h"
#include "ARAM-blocks.h"
#include <malloc.h>

#ifdef DBG
extern int debugger_mode;
extern void update_debugger();
#endif

unsigned long i, dynacore = 0, interpcore = 0;
int no_audio_delay = 0;
int no_compiled_jump = 0;
int stop, llbit;
long long int reg[34] __attribute__((section(".sbss")));
long long int local_rs, local_rt;
unsigned long reg_cop0[32] __attribute__((section(".sbss")));
long local_rs32, local_rt32;
unsigned long jump_target;
float *reg_cop1_simple[32] __attribute__((section(".sbss")));
double *reg_cop1_double[32] __attribute__((section(".sbss")));
long long int reg_cop1_fgr_64[32] __attribute__((section(".sbss")));
unsigned long FCR0, FCR31;
tlb tlb_e[32];
unsigned long delay_slot, skip_jump = 0, dyna_interp = 0, last_addr;
unsigned long next_interupt, CIC_Chip;
precomp_instr *PC;
//char invalid_code[0x100000];

#ifdef PPC_DYNAREC
#include "ppc/Recompile.h"
#ifdef HW_RVL
#include "../gc_memory/MEM2.h"
PowerPC_block **const blocks = (PowerPC_block*)(BLOCKS_LO);
#else
#ifndef ARAM_BLOCKCACHE
PowerPC_block *blocks[0x100000];
#endif
#endif
PowerPC_block *actual;
#else
precomp_block *blocks[0x100000], *actual;
#endif
int rounding_mode = 0x33F, trunc_mode = 0xF3F, round_mode = 0x33F,
    ceil_mode = 0xB3F, floor_mode = 0x73F;
void (*code)();

/*#define check_memory() \
   if (!invalid_code[address>>12]) \
       invalid_code[address>>12] = 1;*/

#ifdef PPC_DYNAREC
#define check_memory() invalid_code_set(address>>12, 1);
#else
#define check_memory() \
   if (!invalid_code_get(address>>12)) \
       if (blocks[address>>12]->block[(address&0xFFF)/4].ops != NOTCOMPILED) \
	 invalid_code_set(address>>12, 1);
#endif

void NI()
{
   printf("NI() @ %x\n", (int)PC->addr);
   printf("opcode not implemented : ");
   if (PC->addr >= 0xa4000000 && PC->addr < 0xa4001000)
     printf("%x:%x\n", (int)PC->addr, (int)SP_DMEM[(PC->addr-0xa4000000)/4]);
   else
     printf("%x:%x\n", (int)PC->addr, (int)rdram[(PC->addr-0x80000000)/4]);
   stop=1;
}

void RESERVED()
{
   printf("reserved opcode : ");
   if (PC->addr >= 0xa4000000 && PC->addr < 0xa4001000)
     printf("%x:%x\n", (int)PC->addr, (int)SP_DMEM[(PC->addr-0xa4000000)/4]);
   else
     printf("%x:%x\n", (int)PC->addr, (int)rdram[(PC->addr-0x80000000)/4]);
   stop=1;
}

void FIN_BLOCK()
{
   if (!delay_slot)
     {
	jump_to((PC-1)->addr+4);
	PC->ops();
	if (dynacore) dyna_jump();
     }
   else
     {
#ifdef PPC_DYNAREC
	PowerPC_block* blk = actual;
#else
	precomp_block *blk = actual;
#endif
	precomp_instr *inst = PC;
	jump_to((PC-1)->addr+4);

	if (!skip_jump)
	  {
	     PC->ops();
	     actual = blk;
	     PC = inst+1;
	  }
	else
	  PC->ops();

	if (dynacore) dyna_jump();
     }
}

void J()
{
   PC++;
   delay_slot=1;
   PC->ops();
   update_count();
   delay_slot=0;
#ifndef PPC_DYNAREC
   if (!skip_jump)
     PC=actual->block+
     (((((PC-2)->f.j.inst_index<<2) | ((PC-1)->addr & 0xF0000000))-actual->start)>>2);
#endif
   last_addr = PC->addr;
   if (next_interupt <= Count) gen_interupt();
}

void J_OUT()
{
   jump_target = (PC->addr & 0xF0000000) | (PC->f.j.inst_index<<2);
   PC++;
   delay_slot=1;
   PC->ops();
   update_count();
   delay_slot=0;
   if (!skip_jump)
     jump_to(jump_target);
   last_addr = PC->addr;
   if (next_interupt <= Count) gen_interupt();
}

void J_IDLE()
{
   long skip;
   update_count();
   skip = next_interupt - Count;
   if (skip > 3) Count += (skip & 0xFFFFFFFC);
   else J();
}

void JAL()
{
   PC++;
   delay_slot=1;
   PC->ops();
   update_count();
   delay_slot=0;
   if (!skip_jump)
     {
	reg[31]=PC->addr;
	sign_extended(reg[31]);
#ifndef PPC_DYNAREC
	PC=actual->block+
	  (((((PC-2)->f.j.inst_index<<2) | ((PC-1)->addr & 0xF0000000))-actual->start)>>2);
#endif
     }
   last_addr = PC->addr;
   if (next_interupt <= Count) gen_interupt();
}

void JAL_OUT()
{
   jump_target = (PC->addr & 0xF0000000) | (PC->f.j.inst_index<<2);
   PC++;
   delay_slot=1;
   PC->ops();
   update_count();
   delay_slot=0;
   if (!skip_jump)
     {
	reg[31]=PC->addr;
	sign_extended(reg[31]);

	jump_to(jump_target);
     }
   last_addr = PC->addr;
   if (next_interupt <= Count) gen_interupt();
}

void JAL_IDLE()
{
   long skip;
   update_count();
   skip = next_interupt - Count;
   if (skip > 3) Count += (skip & 0xFFFFFFFC);
   else JAL();
}

void BEQ()
{
   local_rs = irs;
   local_rt = irt;
   PC++;
   delay_slot=1;
   PC->ops();
   update_count();
   delay_slot=0;
   if (local_rs == local_rt && !skip_jump)
     PC += (PC-2)->f.i.immediate-1;
   last_addr = PC->addr;
   if (next_interupt <= Count) gen_interupt();
}

void BEQ_OUT()
{
   local_rs = irs;
   local_rt = irt;
   jump_target = (long)PC->f.i.immediate;
   PC++;
   delay_slot=1;
   PC->ops();
   update_count();
   delay_slot=0;
   if (!skip_jump && local_rs == local_rt)
     jump_to(PC->addr + ((jump_target-1)<<2));
   last_addr = PC->addr;
   if (next_interupt <= Count) gen_interupt();
}

void BEQ_IDLE()
{
   long skip;
   if (irs == irt)
     {
	update_count();
	skip = next_interupt - Count;
	if (skip > 3) Count += (skip & 0xFFFFFFFC);
	else BEQ();
     }
   else BEQ();
}

void BNE()
{
   local_rs = irs;
   local_rt = irt;
   PC++;
   delay_slot=1;
   PC->ops();
   update_count();
   delay_slot=0;
   if (local_rs != local_rt && !skip_jump)
     PC += (PC-2)->f.i.immediate-1;
   last_addr = PC->addr;
   if (next_interupt <= Count) gen_interupt();
}

void BNE_OUT()
{
   local_rs = irs;
   local_rt = irt;
   jump_target = (long)PC->f.i.immediate;
   PC++;
   delay_slot=1;
   PC->ops();
   update_count();
   delay_slot=0;
   if (!skip_jump && local_rs != local_rt)
     jump_to(PC->addr + ((jump_target-1)<<2));
   last_addr = PC->addr;
   if (next_interupt <= Count) gen_interupt();
}

void BNE_IDLE()
{
   long skip;
   if (irs != irt)
     {
	update_count();
	skip = next_interupt - Count;
	if (skip > 3) Count += (skip & 0xFFFFFFFC);
	else BNE();
     }
   else BNE();
}

void BLEZ()
{
   local_rs = irs;
   PC++;
   delay_slot=1;
   PC->ops();
   update_count();
   delay_slot=0;
   if (local_rs <= 0 && !skip_jump)
     PC += (PC-2)->f.i.immediate-1;
   last_addr = PC->addr;
   if (next_interupt <= Count) gen_interupt();
}

void BLEZ_OUT()
{
   local_rs = irs;
   jump_target = (long)PC->f.i.immediate;
   PC++;
   delay_slot=1;
   PC->ops();
   update_count();
   delay_slot=0;
   if (!skip_jump && local_rs <= 0)
     jump_to(PC->addr + ((jump_target-1)<<2));
   last_addr = PC->addr;
   if (next_interupt <= Count) gen_interupt();
}

void BLEZ_IDLE()
{
   long skip;
   if (irs <= irt)
     {
	update_count();
	skip = next_interupt - Count;
	if (skip > 3) Count += (skip & 0xFFFFFFFC);
	else BLEZ();
     }
   else BLEZ();
}

void BGTZ()
{
   local_rs = irs;
   PC++;
   delay_slot=1;
   PC->ops();
   update_count();
   delay_slot=0;
   if (local_rs > 0 && !skip_jump)
     PC += (PC-2)->f.i.immediate-1;
   last_addr = PC->addr;
   if (next_interupt <= Count) gen_interupt();
}

void BGTZ_OUT()
{
   local_rs = irs;
   jump_target = (long)PC->f.i.immediate;
   PC++;
   delay_slot=1;
   PC->ops();
   update_count();
   delay_slot=0;
   if (!skip_jump && local_rs > 0)
     jump_to(PC->addr + ((jump_target-1)<<2));
   last_addr = PC->addr;
   if (next_interupt <= Count) gen_interupt();
}

void BGTZ_IDLE()
{
   long skip;
   if (irs > irt)
     {
	update_count();
	skip = next_interupt - Count;
	if (skip > 3) Count += (skip & 0xFFFFFFFC);
	else BGTZ();
     }
   else BGTZ();
}

void ADDI()
{
   irt32 = irs32 + iimmediate;
   sign_extended(irt);
   PC++;
}

void ADDIU()
{
   irt32 = irs32 + iimmediate;
   sign_extended(irt);
   PC++;
}

void SLTI()
{
   if (irs < iimmediate) irt = 1;
   else irt = 0;
   PC++;
}

void SLTIU()
{
   if ((unsigned long long)irs < (unsigned long long)((long long)iimmediate))
     irt = 1;
   else irt = 0;
   PC++;
}

void ANDI()
{
   irt = irs & (unsigned short)iimmediate;
   PC++;
}

void ORI()
{
   irt = irs | (unsigned short)iimmediate;
   PC++;
}

void XORI()
{
   irt = irs ^ (unsigned short)iimmediate;
   PC++;
}

void LUI()
{
   irt32 = iimmediate << 16;
   sign_extended(irt);
   PC++;
}

void BEQL()
{
   if (irs == irt)
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
     {
	PC+=2;
	update_count();
     }
   last_addr = PC->addr;
   if (next_interupt <= Count) gen_interupt();
}

void BEQL_OUT()
{
   if (irs == irt)
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
     {
	PC+=2;
	update_count();
     }
   last_addr = PC->addr;
   if (next_interupt <= Count) gen_interupt();
}

void BEQL_IDLE()
{
   long skip;
   if (irs == irt)
     {
	update_count();
	skip = next_interupt - Count;
	if (skip > 3) Count += (skip & 0xFFFFFFFC);
	else BEQL();
     }
   else BEQL();
}

void BNEL()
{
   if (irs != irt)
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
     {
	PC+=2;
	update_count();
     }
   last_addr = PC->addr;
   if (next_interupt <= Count) gen_interupt();
}

void BNEL_OUT()
{
   if (irs != irt)
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
     {
	PC+=2;
	update_count();
     }
   last_addr = PC->addr;
   if (next_interupt <= Count) gen_interupt();
}

void BNEL_IDLE()
{
   long skip;
   if (irs != irt)
     {
	update_count();
	skip = next_interupt - Count;
	if (skip > 3) Count += (skip & 0xFFFFFFFC);
	else BNEL();
     }
   else BNEL();
}

void BLEZL()
{
   if (irs <= 0)
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
     {
	PC+=2;
	update_count();
     }
   last_addr = PC->addr;
   if (next_interupt <= Count) gen_interupt();
}

void BLEZL_OUT()
{
   if (irs <= 0)
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
     {
	PC+=2;
	update_count();
     }
   last_addr = PC->addr;
   if (next_interupt <= Count) gen_interupt();
}

void BLEZL_IDLE()
{
   long skip;
   if (irs <= irt)
     {
	update_count();
	skip = next_interupt - Count;
	if (skip > 3) Count += (skip & 0xFFFFFFFC);
	else BLEZL();
     }
   else BLEZL();
}

void BGTZL()
{
   if (irs > 0)
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
     {
	PC+=2;
	update_count();
     }
   last_addr = PC->addr;
   if (next_interupt <= Count) gen_interupt();
}

void BGTZL_OUT()
{
   if (irs > 0)
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
     {
	PC+=2;
	update_count();
     }
   last_addr = PC->addr;
   if (next_interupt <= Count) gen_interupt();
}

void BGTZL_IDLE()
{
   long skip;
   if (irs > irt)
     {
	update_count();
	skip = next_interupt - Count;
	if (skip > 3) Count += (skip & 0xFFFFFFFC);
	else BGTZL();
     }
   else BGTZL();
}

void DADDI()
{
   irt = irs + iimmediate;
   PC++;
}

void DADDIU()
{
   irt = irs + iimmediate;
   PC++;
}

void LDL()
{
   unsigned long long int word = 0;
   PC++;
   switch ((lsaddr) & 7)
     {
      case 0:
	address = lsaddr;
	rdword = &lsrt;
	read_dword_in_memory();
	break;
      case 1:
	address = (lsaddr) & 0xFFFFFFF8;
	rdword = &word;
	read_dword_in_memory();
	if(address)
	  lsrt = (lsrt & 0xFF) | (word << 8);
	break;
      case 2:
	address = (lsaddr) & 0xFFFFFFF8;
	rdword = &word;
	read_dword_in_memory();
	if(address)
	  lsrt = (lsrt & 0xFFFF) | (word << 16);
	break;
      case 3:
	address = (lsaddr) & 0xFFFFFFF8;
	rdword = &word;
	read_dword_in_memory();
	if(address)
	  lsrt = (lsrt & 0xFFFFFF) | (word << 24);
	break;
      case 4:
	address = (lsaddr) & 0xFFFFFFF8;
	rdword = &word;
	read_dword_in_memory();
	if(address)
	  lsrt = (lsrt & 0xFFFFFFFF) | (word << 32);
	break;
      case 5:
	address = (lsaddr) & 0xFFFFFFF8;
	rdword = &word;
	read_dword_in_memory();
	if(address)
	  lsrt = (lsrt & 0xFFFFFFFFFFLL) | (word << 40);
	break;
      case 6:
	address = (lsaddr) & 0xFFFFFFF8;
	rdword = &word;
	read_dword_in_memory();
	if(address)
	  lsrt = (lsrt & 0xFFFFFFFFFFFFLL) | (word << 48);
	break;
      case 7:
	address = (lsaddr) & 0xFFFFFFF8;
	rdword = &word;
	read_dword_in_memory();
	if(address)
	  lsrt = (lsrt & 0xFFFFFFFFFFFFFFLL) | (word << 56);
	break;
     }
}

void LDR()
{
   unsigned long long int word = 0;
   PC++;
   switch ((lsaddr) & 7)
     {
      case 0:
	address = (lsaddr) & 0xFFFFFFF8;
	rdword = &word;
	read_dword_in_memory();
	if(address)
	  lsrt = (lsrt & 0xFFFFFFFFFFFFFF00LL) | (word >> 56);
	break;
      case 1:
	address = (lsaddr) & 0xFFFFFFF8;
	rdword = &word;
	read_dword_in_memory();
	if(address)
	  lsrt = (lsrt & 0xFFFFFFFFFFFF0000LL) | (word >> 48);
	break;
      case 2:
	address = (lsaddr) & 0xFFFFFFF8;
	rdword = &word;
	read_dword_in_memory();
	if(address)
	  lsrt = (lsrt & 0xFFFFFFFFFF000000LL) | (word >> 40);
	break;
      case 3:
	address = (lsaddr) & 0xFFFFFFF8;
	rdword = &word;
	read_dword_in_memory();
	if(address)
	  lsrt = (lsrt & 0xFFFFFFFF00000000LL) | (word >> 32);
	break;
      case 4:
	address = (lsaddr) & 0xFFFFFFF8;
	rdword = &word;
	read_dword_in_memory();
	if(address)
	  lsrt = (lsrt & 0xFFFFFF0000000000LL) | (word >> 24);
	break;
      case 5:
	address = (lsaddr) & 0xFFFFFFF8;
	rdword = &word;
	read_dword_in_memory();
	if(address)
	  lsrt = (lsrt & 0xFFFF000000000000LL) | (word >> 16);
	break;
      case 6:
	address = (lsaddr) & 0xFFFFFFF8;
	rdword = &word;
	read_dword_in_memory();
	if(address)
	  lsrt = (lsrt & 0xFF00000000000000LL) | (word >> 8);
	break;
      case 7:
	address = (lsaddr) & 0xFFFFFFF8;
	rdword = &lsrt;
	read_dword_in_memory();
	break;
     }
}

void LB()
{
   PC++;
   address = lsaddr;
   rdword = &lsrt;
   read_byte_in_memory();
   if (address)
     sign_extendedb(lsrt);
}

void LH()
{
   PC++;
   address = lsaddr;
   rdword = &lsrt;
   read_hword_in_memory();
   if (address)
     sign_extendedh(lsrt);
}

void LWL()
{
   unsigned long long int word = 0;
   PC++;
   switch ((lsaddr) & 3)
     {
      case 0:
	address = lsaddr;
	rdword = &lsrt;
	read_word_in_memory();
	break;
      case 1:
	address = (lsaddr) & 0xFFFFFFFC;
	rdword = &word;
	read_word_in_memory();
	if(address)
	  lsrt = (lsrt & 0xFF) | (word << 8);
	break;
      case 2:
	address = (lsaddr) & 0xFFFFFFFC;
	rdword = &word;
	read_word_in_memory();
	if(address)
	  lsrt = (lsrt & 0xFFFF) | (word << 16);
	break;
      case 3:
	address = (lsaddr) & 0xFFFFFFFC;
	rdword = &word;
	read_word_in_memory();
	if(address)
	  lsrt = (lsrt & 0xFFFFFF) | (word << 24);
	break;
     }
   if(address)
     sign_extended(lsrt);
}

void LW()
{
   PC++;
   address = lsaddr;
   rdword = &lsrt;
   read_word_in_memory();
   if (address)
     sign_extended(lsrt);
}

void LBU()
{
   PC++;
   address = lsaddr;
   rdword = &lsrt;
   read_byte_in_memory();
}

void LHU()
{
   PC++;
   address = lsaddr;
   rdword = &lsrt;
   read_hword_in_memory();
}

void LWR()
{
   unsigned long long int word = 0;
   PC++;
   switch ((lsaddr) & 3)
     {
      case 0:
	address = (lsaddr) & 0xFFFFFFFC;
	rdword = &word;
	read_word_in_memory();
	if(address)
	  lsrt = (lsrt & 0xFFFFFFFFFFFFFF00LL) | ((word >> 24) & 0xFF);
	break;
      case 1:
	address = (lsaddr) & 0xFFFFFFFC;
	rdword = &word;
	read_word_in_memory();
	if(address)
	  lsrt = (lsrt & 0xFFFFFFFFFFFF0000LL) | ((word >> 16) & 0xFFFF);
	break;
      case 2:
	address = (lsaddr) & 0xFFFFFFFC;
	rdword = &word;
	read_word_in_memory();
	if(address)
	  lsrt = (lsrt & 0xFFFFFFFFFF000000LL) | ((word >> 8) & 0XFFFFFF);
	break;
      case 3:
	address = (lsaddr) & 0xFFFFFFFC;
	rdword = &lsrt;
	read_word_in_memory();
	if(address)
	  sign_extended(lsrt);
     }
}

void LWU()
{
   PC++;
   address = lsaddr;
   rdword = &lsrt;
   read_word_in_memory();
}

void SB()
{
   PC++;
   address = lsaddr;
   byte = (unsigned char)(lsrt & 0xFF);
   write_byte_in_memory();
   check_memory();
}

void SH()
{
   PC++;
   address = lsaddr;
   hword = (unsigned short)(lsrt & 0xFFFF);
   write_hword_in_memory();
   check_memory();
}

void SWL()
{
   unsigned long long int old_word = 0;
   PC++;
   switch ((lsaddr) & 3)
     {
      case 0:
	address = (lsaddr) & 0xFFFFFFFC;
	word = (unsigned long)lsrt;
	write_word_in_memory();
	check_memory();
	break;
      case 1:
	address = (lsaddr) & 0xFFFFFFFC;
	rdword = &old_word;
	read_word_in_memory();
	if(address)
	  {
	     word = ((unsigned long)lsrt >> 8) | (old_word & 0xFF000000);
	     write_word_in_memory();
	     check_memory();
	  }
	break;
      case 2:
	address = (lsaddr) & 0xFFFFFFFC;
	rdword = &old_word;
	read_word_in_memory();
	if(address)
	  {
	     word = ((unsigned long)lsrt >> 16) | (old_word & 0xFFFF0000);
	     write_word_in_memory();
	     check_memory();
	  }
	break;
      case 3:
	address = lsaddr;
	byte = (unsigned char)(lsrt >> 24);
	write_byte_in_memory();
	check_memory();
	break;
     }
}

void SW()
{
   PC++;
   address = lsaddr;
   word = (unsigned long)(lsrt & 0xFFFFFFFF);
   write_word_in_memory();
   check_memory();
}

void SDL()
{
   unsigned long long int old_word = 0;
   PC++;
   switch ((lsaddr) & 7)
     {
      case 0:
	address = (lsaddr) & 0xFFFFFFF8;
	dword = lsrt;
	write_dword_in_memory();
	check_memory();
	break;
      case 1:
	address = (lsaddr) & 0xFFFFFFF8;
	rdword = &old_word;
	read_dword_in_memory();
	if(address)
	  {
	     dword = ((unsigned long long)lsrt >> 8)|(old_word & 0xFF00000000000000LL);
	     write_dword_in_memory();
	     check_memory();
	  }
	break;
      case 2:
	address = (lsaddr) & 0xFFFFFFF8;
	rdword = &old_word;
	read_dword_in_memory();
	if(address)
	  {
	     dword = ((unsigned long long)lsrt >> 16)|(old_word & 0xFFFF000000000000LL);
	     write_dword_in_memory();
	     check_memory();
	  }
	break;
      case 3:
	address = (lsaddr) & 0xFFFFFFF8;
	rdword = &old_word;
	read_dword_in_memory();
	if(address)
	  {
	     dword = ((unsigned long long)lsrt >> 24)|(old_word & 0xFFFFFF0000000000LL);
	     write_dword_in_memory();
	     check_memory();
	  }
	break;
      case 4:
	address = (lsaddr) & 0xFFFFFFF8;
	rdword = &old_word;
	read_dword_in_memory();
	if(address)
	  {
	     dword = ((unsigned long long)lsrt >> 32)|(old_word & 0xFFFFFFFF00000000LL);
	     write_dword_in_memory();
	     check_memory();
	  }
	break;
      case 5:
	address = (lsaddr) & 0xFFFFFFF8;
	rdword = &old_word;
	read_dword_in_memory();
	if(address)
	  {
	     dword = ((unsigned long long)lsrt >> 40)|(old_word & 0xFFFFFFFFFF000000LL);
	     write_dword_in_memory();
	     check_memory();
	  }
	break;
      case 6:
	address = (lsaddr) & 0xFFFFFFF8;
	rdword = &old_word;
	read_dword_in_memory();
	if(address)
	  {
	     dword = ((unsigned long long)lsrt >> 48)|(old_word & 0xFFFFFFFFFFFF0000LL);
	     write_dword_in_memory();
	     check_memory();
	  }
	break;
      case 7:
	address = (lsaddr) & 0xFFFFFFF8;
	rdword = &old_word;
	read_dword_in_memory();
	if(address)
	  {
	     dword = ((unsigned long long)lsrt >> 56)|(old_word & 0xFFFFFFFFFFFFFF00LL);
	     write_dword_in_memory();
	     check_memory();
	  }
	break;
     }
}

void SDR()
{
   unsigned long long int old_word = 0;
   PC++;
   switch ((lsaddr) & 7)
     {
      case 0:
	address = lsaddr;
	rdword = &old_word;
	read_dword_in_memory();
	if(address)
	  {
	     dword = (lsrt << 56) | (old_word & 0x00FFFFFFFFFFFFFFLL);
	     write_dword_in_memory();
	     check_memory();
	  }
	break;
      case 1:
	address = (lsaddr) & 0xFFFFFFF8;
	rdword = &old_word;
	read_dword_in_memory();
	if(address)
	  {
	     dword = (lsrt << 48) | (old_word & 0x0000FFFFFFFFFFFFLL);
	     write_dword_in_memory();
	     check_memory();
	  }
	break;
      case 2:
	address = (lsaddr) & 0xFFFFFFF8;
	rdword = &old_word;
	read_dword_in_memory();
	if(address)
	  {
	     dword = (lsrt << 40) | (old_word & 0x000000FFFFFFFFFFLL);
	     write_dword_in_memory();
	     check_memory();
	  }
	break;
      case 3:
	address = (lsaddr) & 0xFFFFFFF8;
	rdword = &old_word;
	read_dword_in_memory();
	if(address)
	  {
	     dword = (lsrt << 32) | (old_word & 0x00000000FFFFFFFFLL);
	     write_dword_in_memory();
	     check_memory();
	  }
	break;
      case 4:
	address = (lsaddr) & 0xFFFFFFF8;
	rdword = &old_word;
	read_dword_in_memory();
	if(address)
	  {
	     dword = (lsrt << 24) | (old_word & 0x0000000000FFFFFFLL);
	     write_dword_in_memory();
	     check_memory();
	  }
	break;
      case 5:
	address = (lsaddr) & 0xFFFFFFF8;
	rdword = &old_word;
	read_dword_in_memory();
	if(address)
	  {
	     dword = (lsrt << 16) | (old_word & 0x000000000000FFFFLL);
	     write_dword_in_memory();
	     check_memory();
	  }
	break;
      case 6:
	address = (lsaddr) & 0xFFFFFFF8;
	rdword = &old_word;
	read_dword_in_memory();
	if(address)
	  {
	     dword = (lsrt << 8) | (old_word & 0x00000000000000FFLL);
	     write_dword_in_memory();
	     check_memory();
	  }
	break;
      case 7:
	address = (lsaddr) & 0xFFFFFFF8;
	dword = lsrt;
	write_dword_in_memory();
	check_memory();
	break;
     }
}

void SWR()
{
   unsigned long long int old_word = 0;
   PC++;
   switch ((lsaddr) & 3)
     {
      case 0:
	address = lsaddr;
	rdword = &old_word;
	read_word_in_memory();
	if(address)
	  {
	     word = ((unsigned long)lsrt << 24) | (old_word & 0x00FFFFFF);
	     write_word_in_memory();
	     check_memory();
	  }
	break;
      case 1:
	address = (lsaddr) & 0xFFFFFFFC;
	rdword = &old_word;
	read_word_in_memory();
	if(address)
	  {
	     word = ((unsigned long)lsrt << 16) | (old_word & 0x0000FFFF);
	     write_word_in_memory();
	     check_memory();
	  }
	break;
      case 2:
	address = (lsaddr) & 0xFFFFFFFC;
	rdword = &old_word;
	read_word_in_memory();
	if(address)
	  {
	     word = ((unsigned long)lsrt << 8) | (old_word & 0x000000FF);
	     write_word_in_memory();
	     check_memory();
	  }
	break;
      case 3:
	address = (lsaddr) & 0xFFFFFFFC;
	word = (unsigned long)lsrt;
	write_word_in_memory();
	check_memory();
	break;
     }
}

void CACHE()
{
   PC++;
}

void LL()
{
   PC++;
   address = lsaddr;
   rdword = &lsrt;
   read_word_in_memory();
   if (address)
     {
	sign_extended(lsrt);
	llbit = 1;
     }
}

void LWC1()
{
   unsigned long long int temp;
   if (check_cop1_unusable()) return;
   PC++;
   address = lslfaddr;
   rdword = &temp;
   read_word_in_memory();
   if (address)
     *((long*)reg_cop1_simple[lslfft]) = *rdword;
}

void LDC1()
{
   if (check_cop1_unusable()) return;
   PC++;
   address = lslfaddr;
   rdword = (long long*)reg_cop1_double[lslfft];
   read_dword_in_memory();
}

void LD()
{
   PC++;
   address = lsaddr;
   rdword = &lsrt;
   read_dword_in_memory();
}

void SC()
{
   /*PC++;
   printf("SC\n");
   if (llbit) {
      address = lsaddr;
      word = (unsigned long)(lsrt & 0xFFFFFFFF);
      write_word_in_memory();
   }
   lsrt = llbit;*/

   PC++;
   if(llbit)
     {
	address = lsaddr;
	word = (unsigned long)(lsrt & 0xFFFFFFFF);
	write_word_in_memory();
	check_memory();
	llbit = 0;
	lsrt = 1;
     }
   else
     {
	lsrt = 0;
     }
}

void SWC1()
{
   if (check_cop1_unusable()) return;
   PC++;
   address = lslfaddr;
   word = *((long*)reg_cop1_simple[lslfft]);
   write_word_in_memory();
   check_memory();
}

void SDC1()
{
   if (check_cop1_unusable()) return;
   PC++;
   address = lslfaddr;
   dword = *((unsigned long long*)reg_cop1_double[lslfft]);
   write_dword_in_memory();
   check_memory();
}

void SD()
{
   PC++;
   address = lsaddr;
   dword = lsrt;
   write_dword_in_memory();
   check_memory();
}

void NOTCOMPILED()
{
   if ((PC->addr>>16) == 0xa400){
#ifdef PPC_DYNAREC
     //recompile_block(blocks[0xa4000000>>12]);
#else
     recompile_block(SP_DMEM, blocks[0xa4000000>>12], PC->addr);
#endif
   } else
     {
	unsigned long paddr = 0;
	if (PC->addr >= 0x80000000 && PC->addr < 0xc0000000) paddr = PC->addr;
	//else paddr = (tlb_LUT_r[PC->addr>>12]&0xFFFFF000)|(PC->addr&0xFFF);
	else paddr = virtual_to_physical_address(PC->addr, 2);
	if (paddr)
	  {
	     if ((paddr & 0x1FFFFFFF) >= 0x10000000)
	       {
		  //printf("not compiled rom:%x\n", paddr);
#ifdef PPC_DYNAREC
		  // FIXME: We need to read from the romcache into a buffer and recompile the buffer
		  //recompile_block(blocks[PC->addr>>12]);
#else
		  recompile_block((unsigned long*)rom+((((paddr-(PC->addr-blocks[PC->addr>>12]->start)) & 0x1FFFFFFF) - 0x10000000)>>2),
				  blocks[PC->addr>>12], PC->addr);
#endif
	       }
	     else {
#ifdef PPC_DYNAREC
		//recompile_block(blocks[PC->addr>>12]);
#else
	       recompile_block(rdram+(((paddr-(PC->addr-blocks[PC->addr>>12]->start)) & 0x1FFFFFFF)>>2),
			       blocks[PC->addr>>12], PC->addr);
#endif
		}
	  }
	else printf("not compiled exception\n");
     }
   PC->ops();
   if (dynacore)
#ifdef PPC_DYNAREC
     jump_to(PC->addr);
#else
     dyna_jump();
#endif
     //*return_address = (unsigned long)(blocks[PC->addr>>12]->code + PC->local_addr);
   //else
   //PC->ops();
}

void NOTCOMPILED2()
{
   NOTCOMPILED();
}

inline unsigned long update_invalid_addr(unsigned long addr)
{
   if (addr >= 0x80000000 && addr < 0xc0000000)
     {
	if (invalid_code_get(addr>>12)) invalid_code_set((addr^0x20000000)>>12, 1);
	if (invalid_code_get((addr^0x20000000)>>12)) invalid_code_set(addr>>12, 1);
	return addr;
     }
   else
     {
	unsigned long paddr = virtual_to_physical_address(addr, 2);
	if (paddr)
	  {
	     unsigned long beg_paddr = paddr - (addr - (addr&~0xFFF));
	     update_invalid_addr(paddr);
	     if (invalid_code_get((beg_paddr+0x000)>>12)) invalid_code_set(addr>>12, 1);
	     if (invalid_code_get((beg_paddr+0xFFC)>>12)) invalid_code_set(addr>>12, 1);
	     if (invalid_code_get(addr>>12)) invalid_code_set((beg_paddr+0x000)>>12, 1);
	     if (invalid_code_get(addr>>12)) invalid_code_set((beg_paddr+0xFFC)>>12, 1);
	  }
	return paddr;
     }
}

#define addr jump_to_address
unsigned long jump_to_address;
#ifdef PPC_DYNAREC
#define jump_to_func() jump_to(addr)
#else
inline void jump_to_func()
{
   unsigned long paddr;
   if (skip_jump) return;
   paddr = update_invalid_addr(addr);
   if (!paddr) return;
   actual = blocks[addr>>12];
   if (invalid_code_get(addr>>12))
     {
	if (!blocks[addr>>12])
	  {
	     blocks[addr>>12] = malloc(sizeof(precomp_block));
	     actual = blocks[addr>>12];
	     blocks[addr>>12]->code = NULL;
	     blocks[addr>>12]->block = NULL;
	     blocks[addr>>12]->jumps_table = NULL;
	  }
	blocks[addr>>12]->start = addr & ~0xFFF;
	blocks[addr>>12]->end = (addr & ~0xFFF) + 0x1000;
	init_block(rdram+(((paddr-(addr-blocks[addr>>12]->start)) & 0x1FFFFFFF)>>2),
		   blocks[addr>>12]);
     }
#ifdef USE_RECOMP_CACHE
	else RecompCache_Update(addr>>12);
#endif
   PC=actual->block+((addr-actual->start)>>2);

   //if (dynacore) dyna_jump();
}
#endif
#undef addr

/* Refer to Figure 6-2 on page 155 and explanation on page B-11
   of MIPS R4000 Microprocessor User's Manual (Second Edition)
   by Joe Heinrich.
*/
void shuffle_fpr_data(int oldStatus, int newStatus)
{
#if defined(_BIG_ENDIAN)
    const int isBigEndian = 1;
#else
    const int isBigEndian = 0;
#endif

    if ((newStatus & 0x04000000) != (oldStatus & 0x04000000))
    {
        int i;
        int temp_fgr_32[32];

        // pack or unpack the FGR register data
        if (newStatus & 0x04000000)
        {   // switching into 64-bit mode
            // retrieve 32 FPR values from packed 32-bit FGR registers
            for (i = 0; i < 32; i++)
            {
                temp_fgr_32[i] = *((int *) &reg_cop1_fgr_64[i>>1] + ((i & 1) ^ isBigEndian));
            }
            // unpack them into 32 64-bit registers, taking the high 32-bits from their temporary place in the upper 16 FGRs
            for (i = 0; i < 32; i++)
            {
                int high32 = *((int *) &reg_cop1_fgr_64[(i>>1)+16] + (i & 1));
                *((int *) &reg_cop1_fgr_64[i] + isBigEndian)     = temp_fgr_32[i];
                *((int *) &reg_cop1_fgr_64[i] + (isBigEndian^1)) = high32;
            }
        }
        else
        {   // switching into 32-bit mode
            // retrieve the high 32 bits from each 64-bit FGR register and store in temp array
            for (i = 0; i < 32; i++)
            {
                temp_fgr_32[i] = *((int *) &reg_cop1_fgr_64[i] + (isBigEndian^1));
            }
            // take the low 32 bits from each register and pack them together into 64-bit pairs
            for (i = 0; i < 16; i++)
            {
                unsigned int least32 = *((unsigned int *) &reg_cop1_fgr_64[i*2] + isBigEndian);
                unsigned int most32 = *((unsigned int *) &reg_cop1_fgr_64[i*2+1] + isBigEndian);
                reg_cop1_fgr_64[i] = ((unsigned long long) most32 << 32) | (unsigned long long) least32;
            }
            // store the high bits in the upper 16 FGRs, which wont be accessible in 32-bit mode
            for (i = 0; i < 32; i++)
            {
                *((int *) &reg_cop1_fgr_64[(i>>1)+16] + (i & 1)) = temp_fgr_32[i];
            }
        }
    }
}

void set_fpr_pointers(int newStatus)
{
    int i;
#if defined(_BIG_ENDIAN)
    const int isBigEndian = 1;
#else
    const int isBigEndian = 0;
#endif

    // update the FPR register pointers
    if (newStatus & 0x04000000)
    {
        for (i = 0; i < 32; i++)
        {
            reg_cop1_double[i] = (double*) &reg_cop1_fgr_64[i];
            reg_cop1_simple[i] = ((float*) &reg_cop1_fgr_64[i]) + isBigEndian;
        }
    }
    else
    {
        for (i = 0; i < 32; i++)
        {
            reg_cop1_double[i] = (double*) &reg_cop1_fgr_64[i>>1];
            reg_cop1_simple[i] = ((float*) &reg_cop1_fgr_64[i>>1]) + ((i & 1) ^ isBigEndian);
        }
    }
}

int check_cop1_unusable()
{
   if (!(Status & 0x20000000))
     {
	Cause = (11 << 2) | 0x10000000;
	exception_general();
	return 1;
     }
   return 0;
}

#include "../gui/DEBUG.h"

void update_count()
{
   if (dynacore || interpcore)
     {
     	//sprintf(txtbuffer, "trace: addr = 0x%08x\n", interp_addr);
#ifdef SHOW_DEBUG
     	if(interp_addr < last_addr){
     		sprintf(txtbuffer, "interp_addr (%08lx) < last_addr (%08lx)\n", interp_addr, last_addr);
     		DEBUG_print(txtbuffer, DBG_USBGECKO);
     	}
#endif
	Count = Count + (interp_addr - last_addr)/2;
	last_addr = interp_addr;
     }
   else
     {
	if (PC->addr < last_addr)
	  {
	     printf("PC->addr < last_addr\n");
	  }
	Count = Count + (PC->addr - last_addr)/2;
	last_addr = PC->addr;
     }
#ifdef COMPARE_CORE
   if (delay_slot)
     compare_core();
#endif
#ifdef DBG
   if (debugger_mode) update_debugger();
#endif
}

void init_blocks()
{
   int i;
   for (i=0; i<0x100000; i++)
     {
	invalid_code_set(i, 1);
	blocks_set(i, NULL);
     }
#ifndef PPC_DYNAREC
   blocks[0xa4000000>>12] = malloc(sizeof(precomp_block));
   blocks[0xa4000000>>12]->code = NULL;
   blocks[0xa4000000>>12]->block = NULL;
   blocks[0xa4000000>>12]->jumps_table = NULL;
   blocks[0xa4000000>>12]->start = 0xa4000000;
   blocks[0xa4000000>>12]->end = 0xa4001000;
#else
   PowerPC_block* temp_block = calloc(1, sizeof(PowerPC_block));
   blocks_set(0xa4000000>>12, temp_block);
   temp_block->start_address = 0xa4000000;
   temp_block->end_address = 0xa4001000;
#endif
   actual=temp_block;
   init_block(temp_block);
#ifdef PPC_DYNAREC
	PC = malloc(sizeof(precomp_instr));
#else
   PC=actual->block+(0x40/4);
#endif

#ifdef DBG
   if (debugger_mode) // debugger shows initial state (before 1st instruction).
     update_debugger();
#endif
}

static int cpu_inited;
void go()
{
	stop = 0;

   if (!dynacore)
     {
	//printf ("interpreter\n");
	if(cpu_inited){
		init_blocks();
		last_addr = PC->addr;
		cpu_inited = 0;
	}
	while (!stop)
	  {
	     //if ((debug_count+Count) >= 0x78a8091) break; // obj 0x16aeb8a
	     //if ((debug_count+Count) >= 0x16b1360)
	     /*if ((debug_count+Count) >= 0xf203ae0)
	       {
		  printf ("PC=%x:%x\n", (unsigned int)(PC->addr),
			  (unsigned int)(rdram[(PC->addr&0xFFFFFF)/4]));
		  for (j=0; j<16; j++)
		    printf ("reg[%2d]:%8x%8x        reg[%d]:%8x%8x\n",
			    j,
			    (unsigned int)(reg[j] >> 32),
			    (unsigned int)reg[j],
			    j+16,
			    (unsigned int)(reg[j+16] >> 32),
			    (unsigned int)reg[j+16]);
		  printf("hi:%8x%8x        lo:%8x%8x\n",
			 (unsigned int)(hi >> 32),
			 (unsigned int)hi,
			 (unsigned int)(lo >> 32),
			 (unsigned int)lo);
		  printf("apr�s %d instructions soit %x\n",(unsigned int)(debug_count+Count)
			 ,(unsigned int)(debug_count+Count));
		  getchar();
	       }*/
	     /*if ((debug_count+Count) >= 0x80000000)
	       printf("%x:%x, %x\n", (int)PC->addr,
		      (int)rdram[(PC->addr & 0xFFFFFF)/4],
		      (int)(debug_count+Count));*/
#ifdef COMPARE_CORE
	     if (PC->ops == FIN_BLOCK &&
		 (PC->addr < 0x80000000 || PC->addr >= 0xc0000000))
		 virtual_to_physical_address(PC->addr, 2);
	     compare_core();
#endif
	     PC->ops();
	     /*if (j!= (Count & 0xFFF00000))
	       {
		  j = (Count & 0xFFF00000);
		  printf("%x\n", j);
	       }*/
	     //check_PC;
#ifdef DBG
	     if (debugger_mode)
	       update_debugger();
#endif
	  }
     }
   else if (dynacore == 2)
     {
	dynacore = 0;
	interpcore = 1;
	pure_interpreter();
	dynacore = 2;
     }
   else
     {
       interpcore = 0;
	dynacore = 1;
	//printf("dynamic recompiler\n");
	if(cpu_inited){
		RecompCache_Init();
		init_blocks();
		cpu_inited = 0;
	}
#ifdef PPC_DYNAREC
	//jump_to(0xa4000040);
	dynarec(interp_addr);
#else
	code = (void *)(actual->code+(actual->block[0x40/4].local_addr));
	dyna_start(code);
#endif
	//PC++;
     }
}

void cpu_init(void){
   long long CRC = 0;

   ROMCache_read((char*)SP_DMEM+0x40, 0x40, 0xFC0);
   delay_slot=0;
   stop = 0;
   for (i=0;i<32;i++)
     {
	reg[i]=0;
	reg_cop0[i]=0;
	reg_cop1_fgr_64[i]=0;

	// --------------tlb------------------------
	tlb_e[i].mask=0;
	tlb_e[i].vpn2=0;
	tlb_e[i].g=0;
	tlb_e[i].asid=0;
	tlb_e[i].pfn_even=0;
	tlb_e[i].c_even=0;
	tlb_e[i].d_even=0;
	tlb_e[i].v_even=0;
	tlb_e[i].pfn_odd=0;
	tlb_e[i].c_odd=0;
	tlb_e[i].d_odd=0;
	tlb_e[i].v_odd=0;
	tlb_e[i].r=0;
	//tlb_e[i].check_parity_mask=0x1000;

	tlb_e[i].start_even=0;
	tlb_e[i].end_even=0;
	tlb_e[i].phys_even=0;
	tlb_e[i].start_odd=0;
	tlb_e[i].end_odd=0;
	tlb_e[i].phys_odd=0;
     }
#ifndef USE_TLB_CACHE
   for (i=0; i<0x100000; i++)
     {
	tlb_LUT_r[i] = 0;
	tlb_LUT_w[i] = 0;
     }
#endif
   llbit=0;
   hi=0;
   lo=0;
   FCR0=0x511;
   FCR31=0;

   //--------
   /*reg[20]=1;
   reg[22]=0x3F;
   reg[29]=0xFFFFFFFFA0400000LL;
   Random=31;
   Status=0x70400004;
   Config=0x66463;
   PRevID=0xb00;*/
   //--------

   // the following values are extracted from the pj64 source code
   // thanks to Zilmar and Jabo

   reg[6] = 0xFFFFFFFFA4001F0CLL;
   reg[7] = 0xFFFFFFFFA4001F08LL;
   reg[8] = 0x00000000000000C0LL;
   reg[10]= 0x0000000000000040LL;
   reg[11]= 0xFFFFFFFFA4000040LL;
   reg[29]= 0xFFFFFFFFA4001FF0LL;
   reg[31]= 0xFFFFFFFFA4001550LL;

   Random = 31;
   Status= 0x34000000;
   set_fpr_pointers(Status);
   Config= 0x6e463;
   PRevID = 0xb00;
   Count = 0x5000;
   Cause = 0x5C;
   Context = 0x7FFFF0;
   EPC = 0xFFFFFFFF;
   BadVAddr = 0xFFFFFFFF;
   ErrorEPC = 0xFFFFFFFF;

   for (i = 0x40/4; i < (0x1000/4); i++)
     CRC += SP_DMEM[i];
   switch(CRC) {
    case 0x000000D0027FDF31LL:
    case 0x000000CFFB631223LL:
      CIC_Chip = 1;
      break;
    case 0x000000D057C85244LL:
      CIC_Chip = 2;
      break;
    case 0x000000D6497E414BLL:
      CIC_Chip = 3;
      break;
    case 0x0000011A49F60E96LL:
      CIC_Chip = 5;
      break;
    case 0x000000D6D5BE5580LL:
      CIC_Chip = 6;
      break;
    case 0x000001053BC19870LL:
    case 0x000001053B8A9870LL:
      CIC_Chip = 7;
      break;
    default:
      CIC_Chip = 2;
   }

   switch(ROM_HEADER.Country_code&0xFF)
     {
      case 0x44:
      case 0x46:
      case 0x49:
      case 0x50:
      case 0x53:
      case 0x55:
      case 0x58:
      case 0x59:
	switch (CIC_Chip) {
	 case 2:
	   reg[5] = 0xFFFFFFFFC0F1D859LL;
	   reg[14]= 0x000000002DE108EALL;
	   break;
	 case 3:
	   reg[5] = 0xFFFFFFFFD4646273LL;
	   reg[14]= 0x000000001AF99984LL;
	   break;
	 case 5:
	   reg[5] = 0xFFFFFFFFDECAAAD1LL;
	   reg[14]= 0x000000000CF85C13LL;
	   reg[24]= 0x0000000000000002LL;
	   break;
	 case 6:
	   reg[5] = 0xFFFFFFFFB04DC903LL;
	   reg[14]= 0x000000001AF99984LL;
	   reg[24]= 0x0000000000000002LL;
	   break;
	}
	reg[23]= 0x0000000000000006LL;
	break;
      case 0x37:
      case 0x41:
      case 0x45:
      case 0x4A:
      default:
	switch (CIC_Chip) {
	 case 2:
	   reg[5] = 0xFFFFFFFFC95973D5LL;
	   reg[14]= 0x000000002449A366LL;
	   break;
	 case 3:
	   reg[5] = 0xFFFFFFFF95315A28LL;
	   reg[14]= 0x000000005BACA1DFLL;
	   break;
	 case 5:
	   reg[5] = 0x000000005493FB9ALL;
	   reg[14]= 0xFFFFFFFFC2C20384LL;
	   break;
	 case 6:
	   reg[5] = 0xFFFFFFFFE067221FLL;
	   reg[14]= 0x000000005CD2B70FLL;
	   break;
	}
	reg[20]= 0x0000000000000001LL;
	reg[24]= 0x0000000000000003LL;
     }
   switch (CIC_Chip) {
    case 1:
      reg[22]= 0x000000000000003FLL;
      break;
    case 2:
      reg[1] = 0x0000000000000001LL;
      reg[2] = 0x000000000EBDA536LL;
      reg[3] = 0x000000000EBDA536LL;
      reg[4] = 0x000000000000A536LL;
      reg[12]= 0xFFFFFFFFED10D0B3LL;
      reg[13]= 0x000000001402A4CCLL;
      reg[15]= 0x000000003103E121LL;
      reg[22]= 0x000000000000003FLL;
      reg[25]= 0xFFFFFFFF9DEBB54FLL;
      break;
    case 3:
      reg[1] = 0x0000000000000001LL;
      reg[2] = 0x0000000049A5EE96LL;
      reg[3] = 0x0000000049A5EE96LL;
      reg[4] = 0x000000000000EE96LL;
      reg[12]= 0xFFFFFFFFCE9DFBF7LL;
      reg[13]= 0xFFFFFFFFCE9DFBF7LL;
      reg[15]= 0x0000000018B63D28LL;
      reg[22]= 0x0000000000000078LL;
      reg[25]= 0xFFFFFFFF825B21C9LL;
      break;
    case 5:
      SP_IMEM[0] = 0x3C0DBFC0;
      SP_IMEM[1] = 0x8DA807FC;
      SP_IMEM[2] = 0x25AD07C0;
      SP_IMEM[3] = 0x31080080;
      SP_IMEM[4] = 0x5500FFFC;
      SP_IMEM[5] = 0x3C0DBFC0;
      SP_IMEM[6] = 0x8DA80024;
      SP_IMEM[7] = 0x3C0BB000;
      reg[2] = 0xFFFFFFFFF58B0FBFLL;
      reg[3] = 0xFFFFFFFFF58B0FBFLL;
      reg[4] = 0x0000000000000FBFLL;
      reg[12]= 0xFFFFFFFF9651F81ELL;
      reg[13]= 0x000000002D42AAC5LL;
      reg[15]= 0x0000000056584D60LL;
      reg[22]= 0x0000000000000091LL;
      reg[25]= 0xFFFFFFFFCDCE565FLL;
      break;
    case 6:
      reg[2] = 0xFFFFFFFFA95930A4LL;
      reg[3] = 0xFFFFFFFFA95930A4LL;
      reg[4] = 0x00000000000030A4LL;
      reg[12]= 0xFFFFFFFFBCB59510LL;
      reg[13]= 0xFFFFFFFFBCB59510LL;
      reg[15]= 0x000000007A3C07F4LL;
      reg[22]= 0x0000000000000085LL;
      reg[25]= 0x00000000465E3F72LL;
      break;
    case 7:
      reg[22]= 0x00000000000000DDLL;
      break;
   }

   rounding_mode = 0x33F;

   last_addr = 0xa4000040;
   next_interupt = 624999;
   init_interupt();
   interpcore = 0;

   // I'm adding this from pure_interpreter()
   interp_addr = 0xa4000040;
   // Hack for the interpreter
   cpu_inited = 1;
}

void cpu_deinit(void){
	// No need to check these if we were in the pure interp
	if(dynacore != 2 && !cpu_inited){
		for (i=0; i<0x100000; i++) {
  		PowerPC_block* temp_block = blocks_get(i);
		if (temp_block) {
#ifdef PPC_DYNAREC
			deinit_block(temp_block);
#else
			if (temp_block->block) {
#ifdef USE_RECOMP_CACHE
				invalidate_block(temp_block);
#else
				free(temp_block->block);
#endif
				temp_block->block = NULL;
			}
			if (temp_block->code) {
				free(temp_block->code);
				temp_block->code = NULL;
			}
			if (temp_block->jumps_table) {
				free(temp_block->jumps_table);
				temp_block->jumps_table = NULL;
			}
#endif
			free(temp_block);
			blocks_set(i, NULL);
		}
		}
	}
   // tehpola: modified condition from !dynacore && interpcore
   if (dynacore) { free(PC); PC = NULL; }
}

