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
unsigned int dyna_mem(unsigned int, unsigned int, int, memType, unsigned int, int);

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
	PowerPC_instr* return_addr;

	register void* r3  __asm__("r3");
	register void* r29 __asm__("r29") = rdram;
	register void* r30 __asm__("r30") = func;
	register void* r31 __asm__("r31") = NULL;

	end_section(TRAMP_SECTION);

	__asm__ volatile(
		"mtctr  %4        \n"
		"bl     1f        \n"
		"mflr   %3        \n"
		"lwz    %2, 12(1) \n"
		"addi   1, 1, 8   \n"
		"b      2f        \n"
		"1:               \n"
		"stwu   1, -8(1)  \n"
		"mflr   0         \n"
		"stw    0, 12(1)  \n"
		"bctr             \n"
		"2:               \n"
		: "=r" (r3), "=r" (r30), "=r" (return_addr), "=r" (link_branch)
		: "r" (code), "r" (r29), "r" (r30), "r" (r31)
		: "r0", "r2", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12",
		  "fr0", "fr1", "fr2", "fr3", "fr4", "fr5", "fr6", "fr7", "fr8", "fr9", "fr10", "fr11", "fr12", "fr13",
		  "lr", "ctr", "cr0", "cr1", "cr2", "cr3", "cr4", "cr5", "cr6", "cr7", "ca");

	last_func = r30;
	link_branch = link_branch == return_addr ? NULL : link_branch - 1;

	return r3;
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
			link_branch = NULL;
			address = interp_addr;
			dst_block = blocks_get(address>>12); 
			paddr = update_invalid_addr(address);
		}
		
		if(!dst_block){
			/*sprintf(txtbuffer, "block at %08x doesn't exist\n", address&~0xFFF);
			DEBUG_print(txtbuffer, DBG_USBGECKO);*/
			dst_block = calloc(1, sizeof(PowerPC_block));
			blocks_set(address>>12, dst_block);
			dst_block->start_address = address & ~0xFFF;
			dst_block->end_address   = (address & ~0xFFF) + 0x1000;
			init_block(dst_block);
		} else if(invalid_code_get(address>>12)){
			invalidate_block(dst_block);
		}

		PowerPC_func* func = find_func(&dst_block->funcs, address);

		if(!func || !func->code_addr[(address-func->start_addr)>>2]){
			/*sprintf(txtbuffer, "code at %08x is not compiled\n", address);
			DEBUG_print(txtbuffer, DBG_USBGECKO);*/
			dst_block->mips_code = fast_mem_access(paddr & ~0xFFF);
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
	Count += ((pc - last_addr) >> 2) * count_per_op;
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

void invalidate_func(unsigned int addr){
	if(!invalid_code_get(addr>>12)){
		PowerPC_block* block = blocks_get(addr>>12);
		PowerPC_func* func = find_func(&block->funcs, addr);
		if(func)
			RecompCache_Free(func->start_addr);
	}
}

unsigned int dyna_mem(unsigned int addr, unsigned int value, int count,
                      memType type, unsigned int pc, int isDelaySlot){
	int i;

	PC->addr = interp_addr = pc;
	delay_slot = isDelaySlot;

	switch(type){
		case MEM_LW:
			for(i = 0; i < count; i++){
				address = addr + i*4;
				read_word_in_memory();
				if(!address) break;
				reg[value + i] = (signed long)word;
			}
			break;
		case MEM_LWU:
			for(i = 0; i < count; i++){
				address = addr + i*4;
				read_word_in_memory();
				if(!address) break;
				reg[value + i] = word;
			}
			break;
		case MEM_LH:
			for(i = 0; i < count; i++){
				address = addr + i*2;
				read_hword_in_memory();
				if(!address) break;
				reg[value + i] = (signed short)hword;
			}
			break;
		case MEM_LHU:
			for(i = 0; i < count; i++){
				address = addr + i*2;
				read_hword_in_memory();
				if(!address) break;
				reg[value + i] = hword;
			}
			break;
		case MEM_LB:
			for(i = 0; i < count; i++){
				address = addr + i;
				read_byte_in_memory();
				if(!address) break;
				reg[value + i] = (signed char)byte;
			}
			break;
		case MEM_LBU:
			for(i = 0; i < count; i++){
				address = addr + i;
				read_byte_in_memory();
				if(!address) break;
				reg[value + i] = byte;
			}
			break;
		case MEM_LD:
			for(i = 0; i < count; i++){
				address = addr + i*8;
				read_dword_in_memory();
				if(!address) break;
				reg[value + i] = dword;
			}
			break;
		case MEM_LWC1:
			for(i = 0; i < count; i++){
				address = addr + i*4;
				read_word_in_memory();
				if(!address) break;
				*((long*)reg_cop1_simple[value + i*2]) = word;
			}
			break;
		case MEM_LDC1:
			for(i = 0; i < count; i++){
				address = addr + i*8;
				read_dword_in_memory();
				if(!address) break;
				*((long long*)reg_cop1_double[value + i*2]) = dword;
			}
			break;
		case MEM_LL:
			address = addr;
			read_word_in_memory();
			if(!address) break;
			reg[value] = (signed long)word;
			llbit = 1;
			break;
		case MEM_LWL:
			address = addr & ~3;
			read_word_in_memory();
			if(!address) break;
			if((addr & 3) == 0){
				reg[value] = (signed long)word;
			} else {
				u32 shift = (addr & 3) * 8;
				u32 mask = 0xFFFFFFFF << shift;
				reg[value] = (reg[value] & ~mask) | ((word << shift) & mask);
			}
			break;
		case MEM_LWR:
			address = addr & ~3;
			read_word_in_memory();
			if(!address) break;
			if((~addr & 3) == 0){
				reg[value] = (signed long)word;
			} else {
				u32 shift = (~addr & 3) * 8;
				u32 mask = 0xFFFFFFFF >> shift;
				reg[value] = (reg[value] & ~mask) | ((word >> shift) & mask);
			}
			break;
		case MEM_LDL:
			address = addr & ~7;
			read_dword_in_memory();
			if(!address) break;
			if((addr & 7) == 0){
				reg[value] = dword;
			} else {
				u32 shift = (addr & 7) * 8;
				u64 mask = 0xFFFFFFFFFFFFFFFFLL << shift;
				reg[value] = (reg[value] & ~mask) | ((dword << shift) & mask);
			}
			break;
		case MEM_LDR:
			address = addr & ~7;
			read_dword_in_memory();
			if(!address) break;
			if((~addr & 7) == 0){
				reg[value] = dword;
			} else {
				u32 shift = (~addr & 7) * 8;
				u64 mask = 0xFFFFFFFFFFFFFFFFLL >> shift;
				reg[value] = (reg[value] & ~mask) | ((dword >> shift) & mask);
			}
			break;
		case MEM_SW:
			for(i = 0; i < count; i++){
				address = addr + i*4;
				word = reg[value + i];
				write_word_in_memory();
			}
			invalidate_func(addr);
			break;
		case MEM_SH:
			for(i = 0; i < count; i++){
				address = addr + i*2;
				hword = reg[value + i];
				write_hword_in_memory();
			}
			invalidate_func(addr);
			break;
		case MEM_SB:
			for(i = 0; i < count; i++){
				address = addr + i;
				byte = reg[value + i];
				write_byte_in_memory();
			}
			invalidate_func(addr);
			break;
		case MEM_SD:
			for(i = 0; i < count; i++){
				address = addr + i*8;
				dword = reg[value + i];
				write_dword_in_memory();
			}
			invalidate_func(addr);
			break;
		case MEM_SWC1:
			for(i = 0; i < count; i++){
				address = addr + i*4;
				word = *((long*)reg_cop1_simple[value + i*2]);
				write_word_in_memory();
			}
			invalidate_func(addr);
			break;
		case MEM_SDC1:
			for(i = 0; i < count; i++){
				address = addr + i*8;
				dword = *((long long*)reg_cop1_double[value + i*2]);
				write_dword_in_memory();
			}
			invalidate_func(addr);
			break;
		case MEM_SC:
			if(llbit){
				address = addr;
				word = reg[value];
				write_word_in_memory();
				invalidate_func(addr);
			}
			reg[value] = !!llbit;
			llbit = 0;
			break;
		case MEM_SWL:
			address = addr & ~3;
			read_word_in_memory();
			if((addr & 3) == 0){
				word = reg[value];
			} else {
				u32 shift = (addr & 3) * 8;
				u32 mask = 0xFFFFFFFF >> shift;
				word = (word & ~mask) | ((reg[value] >> shift) & mask);
			}
			write_word_in_memory();
			invalidate_func(addr);
			break;
		case MEM_SWR:
			address = addr & ~3;
			read_word_in_memory();
			if((~addr & 3) == 0){
				word = reg[value];
			} else {
				u32 shift = (~addr & 3) * 8;
				u32 mask = 0xFFFFFFFF << shift;
				word = (word & ~mask) | ((reg[value] << shift) & mask);
			}
			write_word_in_memory();
			invalidate_func(addr);
			break;
		case MEM_SDL:
			address = addr & ~7;
			read_dword_in_memory();
			if((addr & 7) == 0){
				dword = reg[value];
			} else {
				u32 shift = (addr & 7) * 8;
				u64 mask = 0xFFFFFFFFFFFFFFFFLL >> shift;
				dword = (dword & ~mask) | ((reg[value] >> shift) & mask);
			}
			write_dword_in_memory();
			invalidate_func(addr);
			break;
		case MEM_SDR:
			address = addr & ~7;
			read_dword_in_memory();
			if((~addr & 7) == 0){
				dword = reg[value];
			} else {
				u32 shift = (~addr & 7) * 8;
				u64 mask = 0xFFFFFFFFFFFFFFFFLL << shift;
				dword = (dword & ~mask) | ((reg[value] << shift) & mask);
			}
			write_dword_in_memory();
			invalidate_func(addr);
			break;
		default:
			stop = 1;
			break;
	}
	delay_slot = 0;

	if(interp_addr != pc) noCheckInterrupt = 1;

	return interp_addr != pc ? interp_addr : 0;
}

