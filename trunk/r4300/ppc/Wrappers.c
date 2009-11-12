/**
 * Wii64 - Wrappers.c
 * Copyright (C) 2008, 2009 Mike Slegeir
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
#include "../../gc_memory/memory.h"
#include "../interupt.h"
#include "../r4300.h"
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

/* Recompiled code stack frame:
 *  $sp+12  |
 *  $sp+8   | old cr
 *  $sp+4   | old lr
 *  $sp	    | old sp
 */

inline unsigned int dyna_run(unsigned int (*code)(void)){
	unsigned int naddr;

	__asm__ volatile(
		// Create the stack frame for code
		"stwu	1, -16(1) \n"
		"mfcr	14        \n"
		"stw	14, 8(1)  \n"
		// Setup saved registers for code
		"mr	14, %0    \n"
		"mr	15, %1    \n"
		"mr	16, %2    \n"
		"mr	17, %3    \n"
		"mr	18, %4    \n"
		"mr	19, %5    \n"
		"mr	20, %6    \n"
		"mr	21, %7    \n"
		"addi	22, 0, 0  \n"
		:: "r" (reg), "r" (reg_cop0),
		   "r" (reg_cop1_simple), "r" (reg_cop1_double),
		   "r" (&FCR31), "r" (rdram),
		   "r" (&last_addr), "r" (&next_interupt)
		: "14", "15", "16", "17", "18", "19", "20", "21", "22");

	end_section(TRAMP_SECTION);

	// naddr = code();
	__asm__ volatile(
		// Save the lr so the recompiled code won't have to
		"bl	4         \n"
		"mtctr	%1        \n"
		"mflr	4         \n"
		"addi	4, 4, 20  \n"
		"stw	4, 20(1)  \n"
		// Execute the code
		"bctrl           \n"
		"mr	%0, 3     \n"
		// Pop the stack
		"lwz	1, 0(1)   \n"
		: "=r" (naddr)
		: "r" (code)
		: "3", "4", "5", "6", "7", "8", "9", "10", "11", "12");

	return naddr;
}

void dynarec(unsigned int address){
	while(!stop){
		start_section(TRAMP_SECTION);
		PowerPC_block* dst_block = blocks[address>>12];
		unsigned long paddr = update_invalid_addr(address);
		/*
		sprintf(txtbuffer, "trampolining to 0x%08x\n", address);
		DEBUG_print(txtbuffer, DBG_USBGECKO);
		*/
		if(!paddr){ stop=1; return; }

		if(!dst_block){
			/*sprintf(txtbuffer, "block at %08x doesn't exist\n", address&~0xFFF);
			DEBUG_print(txtbuffer, DBG_USBGECKO);*/
			dst_block = blocks[address>>12] = malloc(sizeof(PowerPC_block));
			//dst_block->code_addr     = NULL;
			dst_block->funcs         = NULL;
			dst_block->start_address = address & ~0xFFF;
			dst_block->end_address   = (address & ~0xFFF) + 0x1000;
			init_block(rdram+(((paddr-(address-dst_block->start_address)) & 0x1FFFFFFF)>>2),
				   dst_block);
		} else if(invalid_code_get(address>>12)){
			invalidate_block(dst_block);
		}

		PowerPC_func* func = find_func(&dst_block->funcs, address&0xFFFF);

		if(!func || !func->code_addr[((address&0xFFFF)-func->start_addr)>>2]){
			/*sprintf(txtbuffer, "code at %08x is not compiled\n", address);
			DEBUG_print(txtbuffer, DBG_USBGECKO);*/
			start_section(COMPILER_SECTION);
			func = recompile_block(dst_block, address);
			end_section(COMPILER_SECTION);
		} else {
#ifdef USE_RECOMP_CACHE
			RecompCache_Update(func);
#endif
		}

		int index = ((address&0xFFFF) - func->start_addr)>>2;

		// Recompute the block offset
		unsigned int (*code)(void);
		code = (unsigned int (*)(void))func->code_addr[index];
		address = dyna_run(code);

		if(!noCheckInterrupt){
			last_addr = interp_addr = address;
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

int dyna_update_count(unsigned int pc){
	Count += (pc - last_addr)/2;
	last_addr = pc;

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
	PowerPC_block* block = blocks[addr>>12];
	PowerPC_func* func = find_func(&block->funcs, addr&0xffff);
	if(func)
		RecompCache_Free(block->start_address | func->start_addr);
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
		default:
			stop = 1;
			break;
	}
	delay_slot = 0;

	if(interp_addr != pc) noCheckInterrupt = 1;

	return interp_addr != pc ? interp_addr : 0;
}

