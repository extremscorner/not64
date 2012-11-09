/**
 * Wii64 - Wrappers.c
 * Copyright (C) 2008, 2009, 2010 Mike Slegeir
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

#include <stdlib.h>
#include "../../gui/DEBUG.h"
#include "../Invalid_Code.h"
#include "../ARAM-blocks.h"
#include "../../gc_memory/memory.h"
#include "../interupt.h"
#include "../r4300.h"
#include "../macros.h"
#include "../Recomp-Cache.h"
#include "Recompile.h"
#include "Wrappers.h"

extern int stop;
extern unsigned long instructionCount;
extern void (*interp_ops[64])(void);
inline unsigned long update_invalid_addr(unsigned long addr);
unsigned int dyna_check_cop1_unusable(unsigned int, int);
unsigned int dyna_mem(unsigned int, unsigned int, memType, unsigned int, int);

int noCheckInterrupt = 0;

static PowerPC_instr* link_branch = NULL;
static PowerPC_func* last_func;

/* Recompiled code stack frame:
 *  $sp+12  |
 *  $sp+8   | old cr
 *  $sp+4   | old lr
 *  $sp	    | old sp
 */

inline unsigned int dyna_run(PowerPC_func* func, unsigned int (*code)(void)){
	unsigned int naddr;
	PowerPC_instr* return_addr;

	register void* r25 __asm__("r25") = reg;
	register void* r26 __asm__("r26") = reg_cop0;
	register void* r27 __asm__("r27") = reg_cop1_simple;
	register void* r28 __asm__("r28") = reg_cop1_double;
	register void* r29 __asm__("r29") = rdram;
	register void* r30 __asm__("r30") = func;
	register void* r31 __asm__("r31") = NULL;

	end_section(TRAMP_SECTION);

	__asm__ volatile(
		"stwu   1, -8(1)  \n"
		"bl     1f        \n"
		"mflr   %0        \n"
		"lwz    %1, 12(1) \n"
		"mr     %2, 30    \n"
		"mr     %3, 3     \n"
		"addi   1, 1, 8   \n"
		"b      2f        \n"
		"1:               \n"
		"mtctr  %4        \n"
		"mflr   2         \n"
		"stw    2, 12(1)  \n"
		"bctr             \n"
		"2:               \n"
		: "=r" (link_branch), "=r" (return_addr), "=r" (last_func), "=r" (naddr)
		: "r" (code), "r" (r25), "r" (r26), "r" (r27), "r" (r28), "r" (r29), "r" (r30), "r" (r31)
		: "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12");

	link_branch = link_branch == return_addr ? NULL : link_branch - 1;
	
	return naddr;
}

void dynarec(unsigned int address){
	while(!stop){
		refresh_stat();
		
		start_section(TRAMP_SECTION);
		PowerPC_block* dst_block = blocks_get(address>>12);
		unsigned long paddr = update_invalid_addr(address);
		/*
		sprintf(txtbuffer, "trampolining to 0x%08x\n", address);
		DEBUG_print(txtbuffer, DBG_USBGECKO);
		*/
		if(!paddr){ 
			address = paddr = update_invalid_addr(interp_addr);
			dst_block = blocks_get(address>>12); 
		}
		
		if(!dst_block){
			/*sprintf(txtbuffer, "block at %08x doesn't exist\n", address&~0xFFF);
			DEBUG_print(txtbuffer, DBG_USBGECKO);*/
			dst_block = malloc(sizeof(PowerPC_block));
			blocks_set(address>>12, dst_block);
			//dst_block->code_addr     = NULL;
			dst_block->funcs         = NULL;
			dst_block->start_address = address & ~0xFFF;
			dst_block->end_address   = (address & ~0xFFF) + 0x1000;
			if((paddr >= 0xb0000000 && paddr < 0xc0000000) ||
			   (paddr >= 0x90000000 && paddr < 0xa0000000)){
				init_block(NULL, dst_block);
			} else {
				init_block(rdram+(((paddr-(address-dst_block->start_address)) & 0x1FFFFFFF)>>2),
						   dst_block);
			}
		} else if(invalid_code_get(address>>12)){
			invalidate_block(dst_block);
		}

		PowerPC_func* func = find_func(&dst_block->funcs, address);

		if(!func || !func->code_addr[(address-func->start_addr)>>2]){
			/*sprintf(txtbuffer, "code at %08x is not compiled\n", address);
			DEBUG_print(txtbuffer, DBG_USBGECKO);*/
			if((paddr >= 0xb0000000 && paddr < 0xc0000000) ||
			   (paddr >= 0x90000000 && paddr < 0xa0000000))
				dst_block->mips_code =
					ROMCache_pointer((paddr-(address-dst_block->start_address))&0x0FFFFFFF);
			start_section(COMPILER_SECTION);
			func = recompile_block(dst_block, address);
			end_section(COMPILER_SECTION);
		} else {
#ifdef USE_RECOMP_CACHE
			RecompCache_Update(func);
#endif
		}

		int index = (address - func->start_addr)>>2;

		// Recompute the block offset
		unsigned int (*code)(void);
		code = (unsigned int (*)(void))func->code_addr[index];
		
		// Create a link if possible
		if(link_branch && !func_was_freed(last_func))
			RecompCache_Link(last_func, link_branch, func, code);
		clear_freed_funcs();
		
		interp_addr = address = dyna_run(func, code);

		if(!noCheckInterrupt){
			last_addr = interp_addr;
			// Check for interrupts
			if(next_interupt <= Count){
				gen_interupt();
				address = interp_addr;
			}
		}
		noCheckInterrupt = 0;
	}
	interp_addr = address;
}

unsigned int decodeNInterpret(MIPS_instr mips, unsigned int pc,
                              int isDelaySlot){
	delay_slot = isDelaySlot; // Make sure we set delay_slot properly
	PC->addr = interp_addr = pc;
	start_section(INTERP_SECTION);
	prefetch_opcode(mips);
	interp_ops[MIPS_GET_OPCODE(mips)]();
	end_section(INTERP_SECTION);
	delay_slot = 0;

	if(interp_addr != pc + 4) noCheckInterrupt = 1;

	return interp_addr != pc + 4 ? interp_addr : 0;
}

#ifdef COMPARE_CORE
int dyna_update_count(unsigned int pc, int isDelaySlot){
#else
int dyna_update_count(unsigned int pc){
#endif
	Count += (pc - last_addr)/2;
	last_addr = pc;

#ifdef COMPARE_CORE
	if(isDelaySlot){
		interp_addr = pc;
		compare_core();
	}
#endif

	return next_interupt - Count;
}

unsigned int dyna_check_cop1_unusable(unsigned int pc, int isDelaySlot){
	// Set state so it can be recovered after exception
	delay_slot = isDelaySlot;
	PC->addr = interp_addr = pc;
	// Take a FP unavailable exception
	Cause = (11 << 2) | 0x10000000;
	exception_general();
	// Reset state
	delay_slot = 0;
	noCheckInterrupt = 1;
	// Return the address to trampoline to
	return interp_addr;
}

static void invalidate_func(unsigned int addr){
	PowerPC_block* block = blocks_get(addr>>12);
	PowerPC_func* func = find_func(&block->funcs, addr);
	if(func)
		RecompCache_Free(func->start_addr);
}

#define check_memory() \
	if(!invalid_code_get(address>>12)/* && \
	   blocks[address>>12]->code_addr[(address&0xfff)>>2]*/) \
		invalidate_func(address);

unsigned int dyna_mem(unsigned int value, unsigned int addr,
                      memType type, unsigned int pc, int isDelaySlot){
	static unsigned long long dyna_rdword;

	address = addr;
	rdword = &dyna_rdword;
	PC->addr = interp_addr = pc;
	delay_slot = isDelaySlot;

	switch(type){
		case MEM_LW:
			read_word_in_memory();
			reg[value] = (long long)((long)dyna_rdword);
			break;
		case MEM_LWU:
			read_word_in_memory();
			reg[value] = (unsigned long long)((long)dyna_rdword);
			break;
		case MEM_LH:
			read_hword_in_memory();
			reg[value] = (long long)((short)dyna_rdword);
			break;
		case MEM_LHU:
			read_hword_in_memory();
			reg[value] = (unsigned long long)((unsigned short)dyna_rdword);
			break;
		case MEM_LB:
			read_byte_in_memory();
			reg[value] = (long long)((signed char)dyna_rdword);
			break;
		case MEM_LBU:
			read_byte_in_memory();
			reg[value] = (unsigned long long)((unsigned char)dyna_rdword);
			break;
		case MEM_LD:
			read_dword_in_memory();
			reg[value] = dyna_rdword;
			break;
		case MEM_LWC1:
			read_word_in_memory();
			*((long*)reg_cop1_simple[value]) = (long)dyna_rdword;
			break;
		case MEM_LDC1:
			read_dword_in_memory();
			*((long long*)reg_cop1_double[value]) = dyna_rdword;
			break;
		case MEM_LWL:
			address = addr & ~3;
			read_word_in_memory();
			switch(addr&3){
				case 0:
					reg[value] = dyna_rdword;
					break;
				case 1:
					reg[value] = (reg[value]&0x000000FF) | (dyna_rdword<<8);
					break;
				case 2:
					reg[value] = (reg[value]&0x0000FFFF) | (dyna_rdword<<16);
					break;
				case 3:
					reg[value] = (reg[value]&0x00FFFFFF) | (dyna_rdword<<24);
					break;
			}
			sign_extended(reg[value]);
			break;
		case MEM_LWR:
			address = addr & ~3;
			read_word_in_memory();
			switch(addr&3){
				case 0:
					reg[value] = (reg[value]&0xFFFFFF00) | (dyna_rdword>>24);
					break;
				case 1:
					reg[value] = (reg[value]&0xFFFF0000) | (dyna_rdword>>16);
					break;
				case 2:
					reg[value] = (reg[value]&0xFF000000) | (dyna_rdword>>8);
					break;
				case 3:
					reg[value] = dyna_rdword;
					break;
			}
			sign_extended(reg[value]);
			break;
		case MEM_SW:
			word = value;
			write_word_in_memory();
			check_memory();
			break;
		case MEM_SH:
			hword = value;
			write_hword_in_memory();
			check_memory();
			break;
		case MEM_SB:
			byte = value;
			write_byte_in_memory();
			check_memory();
			break;
		case MEM_SD:
			dword = reg[value];
			write_dword_in_memory();
			check_memory();
			break;
		case MEM_SWC1:
			word = *((long*)reg_cop1_simple[value]);
			write_word_in_memory();
			check_memory();
			break;
		case MEM_SDC1:
			dword = *((unsigned long long*)reg_cop1_double[value]);
			write_dword_in_memory();
			check_memory();
			break;
		case MEM_SWL:
			address = addr & ~3;
			read_word_in_memory();
			switch(addr&3){
				case 0:
					word = value;
					break;
				case 1:
					word = (value>>8) | (dyna_rdword&0xFF000000);
					break;
				case 2:
					word = (value>>16) | (dyna_rdword&0xFFFF0000);
					break;
				case 3:
					word = (value>>24) | (dyna_rdword&0xFFFFFF00);
					break;
			}
			write_word_in_memory();
			check_memory();
			break;
		case MEM_SWR:
			address = addr & ~3;
			read_word_in_memory();
			switch(addr&3){
				case 0:
					word = (value<<24) | (dyna_rdword&0x00FFFFFF);
					break;
				case 1:
					word = (value<<16) | (dyna_rdword&0x0000FFFF);
					break;
				case 2:
					word = (value<<8) | (dyna_rdword&0x000000FF);
					break;
				case 3:
					word = value;
					break;
			}
			write_word_in_memory();
			check_memory();
			break;
		default:
			stop = 1;
			break;
	}
	delay_slot = 0;

	if(interp_addr != pc) noCheckInterrupt = 1;

	return interp_addr != pc ? interp_addr : 0;
}

