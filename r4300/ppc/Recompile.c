/* Recompile.c - Recompiles a block of MIPS code to PPC
   by Mike Slegeir for Mupen64-GC
 **********************************************************
   TODO: Try to conform more to the interface mupen64 uses.
         If we have the entire RAM and recompiled code in memory,
           we'll run out of room, we should implement a recompiled
           code cache (e.g. free blocks which haven't been used lately)
         If it's possible, only use the blocks physical addresses (saves space)
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "../../gc_memory/memory.h"
#include "../Invalid_Code.h"
#include "../interupt.h"
#include "Recompile.h"
#include "Wrappers.h"

// Just for debugging
#include <ogc/video.h>
#include "../../gui/DEBUG.h"

static MIPS_instr*    src;
static PowerPC_instr* dst;
static MIPS_instr*    src_last;
static MIPS_instr*    src_first;
static unsigned int*  code_length;
static unsigned int   addr_first;
static unsigned int   addr_last;
static PowerPC_instr* jump_pad;
static jump_node    jump_table[MAX_JUMPS];
static unsigned int current_jump;

int    emu_reg[32]; // State of emulator
double emu_fpr[32];
int lr[8]; // link register stack
int lr_i;

static void pass2(PowerPC_block* ppc_block);
//static void genRecompileBlock(PowerPC_block*);
static void genJumpPad(PowerPC_block*);
int resizeCode(PowerPC_block* block, int newSize);

MIPS_instr get_next_src(void) { return *(src++); }
MIPS_instr peek_next_src(void){ return *src;     }
int has_next_src(void){ return (src_last-src) > 0; }
// Hacks for delay slots
 // Undoes a get_next_src
 void unget_last_src(void){ --src; }
 // Used for finding how many instructions were generated
 PowerPC_instr* get_curr_dst(void){ return dst; }
// Returns the MIPS PC
unsigned int get_src_pc(void){ return addr_first + ((src-src_last)<<2); }

void set_next_dst(PowerPC_instr i){ *(dst++) = i; ++(*code_length); }

int add_jump(int old_jump, int is_j, int is_out){
	int id = current_jump;
	jump_node* jump = &jump_table[current_jump++];
	jump->old_jump  = old_jump;
	jump->new_jump  = 0;     // This should be filled in when known
	jump->src_instr = src-1; // src points to the next
	jump->dst_instr = dst;   // set_next hasn't happened
	jump->type      = (is_j   ? JUMP_TYPE_J   : 0)
	                | (is_out ? JUMP_TYPE_OUT : 0);
	return id;
}

// Converts a sequence of MIPS instructions to a PowerPC block
void recompile_block(PowerPC_block* ppc_block){
	src = src_first = ppc_block->mips_code;
	dst = ppc_block->code;
	src_last = src + (ppc_block->end_address - ppc_block->start_address)/4;
	code_length = &ppc_block->length;
	addr_first = ppc_block->start_address;
	addr_last  = ppc_block->end_address;
	current_jump = 0;
	
	while(has_next_src()){
		// Make sure the code doesn't overflow
		// FIXME: The resize factor may not be optimal
		//          maybe we can make a guess based on
		//          how far along we are now
		if(*code_length + 16 >= ppc_block->max_length)
			resizeCode(ppc_block, ppc_block->max_length * 3/2);
		
		ppc_block->code_addr[src-src_first] = dst;
		if( convert() == CONVERT_ERROR ){
			sprintf(txtbuffer,"Error converting MIPS instruction:\n"
			       "0x%08x   0x%08x\n",
			        ppc_block->start_address + (int)(src-1-src_first)*4, *(src-1));
			DEBUG_print(txtbuffer, DBG_USBGECKO);
			//int i=16; while(i--) VIDEO_WaitVSync();
		}
#ifdef DEBUG_DYNAREC
		// This allows us to return to the debugger every instruction
		PowerPC_instr ppc = NEW_PPC_INSTR();
		PPC_SET_OPCODE(ppc, PPC_OPCODE_XL);
		PPC_SET_FUNC  (ppc, PPC_FUNC_BCLR);
		PPC_SET_BO    (ppc, 0x14);
		PPC_SET_LK    (ppc, 1);
		set_next_dst(ppc);
#endif
	}
	
	// This simplifies jumps and branches out of the block
	genJumpPad(ppc_block);
	
	// Here we recompute jumps and branches
	pass2(ppc_block);
	invalid_code_set(ppc_block->start_address>>12, 0);
	// Since this is a fresh block of code,
	// Make sure it wil show up in the ICache
	DCFlushRange(ppc_block->code, ppc_block->length*sizeof(PowerPC_instr));
	ICInvalidateRange(ppc_block->code, ppc_block->length*sizeof(PowerPC_instr));
}

void init_block(MIPS_instr* mips_code, PowerPC_block* ppc_block){
	unsigned int length = (ppc_block->end_address - ppc_block->start_address)/sizeof(MIPS_instr);
	if(!ppc_block->code){
		ppc_block->max_length = 3*length;
		ppc_block->code = malloc(ppc_block->max_length
		                         * sizeof(PowerPC_instr));
	}
	if(ppc_block->code_addr)
		free(ppc_block->code_addr);
	ppc_block->code_addr = malloc(length * sizeof(PowerPC_instr*));
	ppc_block->mips_code = mips_code;
	
	// TODO: We should probably point all equivalent addresses to this block as well
	//         or point to the same code without leaving a dangling pointer
	
	ppc_block->length = 0;
	// We haven't actually recompiled the block
	invalid_code_set(ppc_block->start_address>>12, 1);
}

void deinit_block(PowerPC_block* ppc_block){
	if(ppc_block->code){
		free(ppc_block->code);
		ppc_block->code = NULL;
	}
	if(ppc_block->code_addr){
		free(ppc_block->code_addr);
		ppc_block->code_addr = NULL;
	}
	invalid_code_set(ppc_block->start_address>>12, 1);
	
	// TODO: We should probably mark all equivalent addresses as invalid
}

int is_j_out(int branch, int is_aa){
	if(is_aa)
		return ((branch << 2 | (addr_first & 0xF0000000)) < addr_first ||
		        (branch << 2 | (addr_first & 0xF0000000)) > addr_last);
	else {
		int dst_instr = (src - src_first) + branch;
		return (dst_instr < 0 || dst_instr > (addr_last-addr_first)>>2);
	}
}

// Pass 2 fills in all the new addresses
static void pass2(PowerPC_block* ppc_block){
	int i;
	PowerPC_instr* current;
	for(i=0; i<current_jump; ++i){
		current = jump_table[i].dst_instr;
		if(!(jump_table[i].type & JUMP_TYPE_J)){ // Branch instruction
			int jump_offset = (unsigned int)jump_table[i].old_jump + 
				         ((unsigned int)jump_table[i].src_instr - (unsigned int)src_first)/4;
			
			if(!(jump_table[i].type & JUMP_TYPE_OUT)){
				jump_table[i].new_jump = ppc_block->code_addr[jump_offset] - current;

				sprintf(txtbuffer,"Converting old_jump = %d, to new_jump = %d\n",
				       jump_table[i].old_jump, jump_table[i].new_jump);
				DEBUG_print(txtbuffer, DBG_USBGECKO);
				
				*current &= ~(PPC_BD_MASK << PPC_BD_SHIFT);
				PPC_SET_BD(*current, jump_table[i].new_jump);
				
			} else { // jump couldn't be recalculated, should jump out
				unsigned int dest = (jump_offset << 2) + ppc_block->start_address;
				sprintf(txtbuffer,"Branching out to 0x%08x\n", dest);
				DEBUG_print(txtbuffer, DBG_USBGECKO);
				current -= 4;
				// mtctr r1
				*current = NEW_PPC_INSTR();
				PPC_SET_OPCODE(*current, PPC_OPCODE_X);
				PPC_SET_FUNC  (*current, PPC_FUNC_MTSPR);
				PPC_SET_RD    (*current, 1);
				PPC_SET_SPR   (*current, 0x120);
				++current;
				// lis	r1, dest@ha(0)
				*current = NEW_PPC_INSTR();
				PPC_SET_OPCODE(*current, PPC_OPCODE_ADDIS);
				PPC_SET_RD    (*current, 1);
				PPC_SET_IMMED (*current, dest>>16);
				++current;
				// la	r0, dest@l(r1)
				*current = NEW_PPC_INSTR();
				PPC_SET_OPCODE(*current, PPC_OPCODE_ORI);
				PPC_SET_RD    (*current, 1);
				PPC_SET_RA    (*current, 0);
				PPC_SET_IMMED (*current, dest);
				++current;
				// mfctr r1
				*current = NEW_PPC_INSTR();
				PPC_SET_OPCODE(*current, PPC_OPCODE_X);
				PPC_SET_FUNC  (*current, PPC_FUNC_MFSPR);
				PPC_SET_RD    (*current, 1);
				PPC_SET_SPR   (*current, 0x120);
				++current;
				
				// bc	<jump_pad>     
				*current &= ~(PPC_BD_MASK << PPC_BD_SHIFT);
				PPC_SET_BD(*current, ((unsigned int)jump_pad-(unsigned int)current)>>2);
				++current;
					
				// andi	r0, r0, 0
				*current = NEW_PPC_INSTR();
				PPC_SET_OPCODE(*current, PPC_OPCODE_ANDI);
			}
			
		} else { // Jump instruction
			// The jump_address is actually calculated with the delay slot address
			unsigned int jump_address = (jump_table[i].old_jump << 2) |
			                            ((unsigned int)ppc_block->start_address & 0xF0000000);
			
			if(!(jump_table[i].type & JUMP_TYPE_OUT)){
				// We're jumping within this block, find out where
				int jump_offset = (jump_address - ppc_block->start_address) >> 2;
				jump_table[i].new_jump = ppc_block->code_addr[jump_offset] - current;

				sprintf(txtbuffer,"Jumping to 0x%08x; jump_offset = %d\n", jump_address, jump_offset);
				DEBUG_print(txtbuffer, DBG_USBGECKO);
				*current &= ~(PPC_LI_MASK << PPC_LI_SHIFT);
				PPC_SET_LI(*current, jump_table[i].new_jump);
				
			} else {
				// We're jumping out of this block
				sprintf(txtbuffer,"Jumping out to 0x%08x\n", jump_address);
				DEBUG_print(txtbuffer, DBG_USBGECKO);
				current -= 4;
				// mtctr r1
				*current = NEW_PPC_INSTR();
				PPC_SET_OPCODE(*current, PPC_OPCODE_X);
				PPC_SET_FUNC  (*current, PPC_FUNC_MTSPR);
				PPC_SET_RD    (*current, 1);
				PPC_SET_SPR   (*current, 0x120);
				++current;
				// lis	r1, dest@ha(0)
				*current = NEW_PPC_INSTR();
				PPC_SET_OPCODE(*current, PPC_OPCODE_ADDIS);
				PPC_SET_RD    (*current, 1);
				PPC_SET_IMMED (*current, jump_address>>16);
				++current;
				// la	r0, dest@l(r1)
				*current = NEW_PPC_INSTR();
				PPC_SET_OPCODE(*current, PPC_OPCODE_ORI);
				PPC_SET_RD    (*current, 1);
				PPC_SET_RA    (*current, 0);
				PPC_SET_IMMED (*current, jump_address);
				++current;
				// mfctr r1
				*current = NEW_PPC_INSTR();
				PPC_SET_OPCODE(*current, PPC_OPCODE_X);
				PPC_SET_FUNC  (*current, PPC_FUNC_MFSPR);
				PPC_SET_RD    (*current, 1);
				PPC_SET_SPR   (*current, 0x120);
				++current;
				         
				// b	<jump_pad>
				*current &= ~(PPC_LI_MASK << PPC_LI_SHIFT);
				PPC_SET_LI(*current, ((unsigned int)jump_pad-(unsigned int)current)>>2);
				++current;
				         
				// andi	r0, r0, 0
				*current = NEW_PPC_INSTR();
				PPC_SET_OPCODE(*current, PPC_OPCODE_ANDI);
				
			}
		}
	}
}

inline unsigned long update_invalid_addr(unsigned long addr);
void jump_to(unsigned int address){
	PowerPC_block* dst_block = blocks[address>>12];
	unsigned long paddr = update_invalid_addr(address);
	if (!paddr) return;
	
	// Check for interrupts
	update_count();
	if (next_interupt <= Count) gen_interupt();
	
	sprintf(txtbuffer, "jump_to 0x%08x\n", address);
	DEBUG_print(txtbuffer, DBG_USBGECKO);
	
	if(!dst_block){
		blocks[address>>12] = malloc(sizeof(PowerPC_block));
		dst_block = blocks[address>>12];
		dst_block->code          = NULL;
		dst_block->code_addr     = NULL;
		dst_block->start_address = address & ~0xFFF;
		dst_block->end_address   = (address & ~0xFFF) + 0x1000;
		init_block(rdram+(((paddr-(address-dst_block->start_address)) & 0x1FFFFFFF)>>2),
			   dst_block);
	}
	// TODO: If we deinit blocks we haven't used recently, we should do something like:
	//         new_block(dst_block); // This checks if its not inited, if not, it inits
	
	if(invalid_code_get(address>>12))
		recompile_block(dst_block);
	
	// Support the cache
	//update_lru();
	//lru[address>>12] = 0;
	
	// Recompute the block offset
	int offset = 0;
	if((address&0xFFF) > 0)
		offset = dst_block->code_addr[(address&0xFFF)>>2] - dst_block->code;
	
	start(dst_block, offset);
}
extern unsigned long jump_to_address;
void dyna_jump(){ jump_to(jump_to_address); }
void dyna_stop(){ }
void jump_to_func(){ jump_to(jump_to_address); }

// Warning: This is a slow operation, try not to use it
int resizeCode(PowerPC_block* block, int newSize){
	if(!block->code) return 0;
	
	// Creating the new block and copying the code
	PowerPC_instr* oldCode = block->code;
	block->code = malloc(newSize * sizeof(PowerPC_instr));
	if(!block->code) return 0;
	memcpy(block->code, oldCode,
	       ((block->max_length < newSize) ? block->max_length : newSize)*sizeof(PowerPC_instr));
	
	// Readjusting pointers
	// Optimization: Sepp256 suggested storing offsets instead of pointers
	//                 then this step won't be necessary
	dst = block->code + (dst - oldCode);
	int i;
	for(i=0; i<(block->end_address - block->start_address)/4; ++i)
		block->code_addr[i] = block->code + (block->code_addr[i] - oldCode);
	for(i=0; i<current_jump; ++i)
		jump_table[i].dst_instr = block->code + (jump_table[i].dst_instr - oldCode);
	
	free(oldCode);
	block->max_length = newSize;
	return newSize;
}

// FIXME: Some instructions can be rearranged to remove some mf/mt ctr isntructions
static void genJumpPad(PowerPC_block* ppc_block){
	static PowerPC_instr padCode[16];
	static int generated = 0;
	
	if(*code_length + 22 >= ppc_block->max_length)
		resizeCode(ppc_block, ppc_block->max_length + 22);
	
	jump_pad = dst;
	
	// (dest address saved in r0):
	PowerPC_instr ppc = NEW_PPC_INSTR();
	
	//cmpi	cr7, r0, 0 // If r0 is 0 we probably should just go to
	PPC_SET_OPCODE(ppc, PPC_OPCODE_CMPI);
	PPC_SET_CRF   (ppc, 7);
	set_next_dst(ppc);
	//bne	cr7, 5     // the next block of code
	ppc = NEW_PPC_INSTR();
	PPC_SET_OPCODE(ppc, PPC_OPCODE_BC);
	PPC_SET_BD(ppc, 5);
	PPC_SET_BO(ppc, 0xc);  // Test if CR is 1
	PPC_SET_BI(ppc, 30);  // Check CR bit 30 (CR7, EQ FIELD)
	set_next_dst(ppc);
	//mtctr	r1
	ppc = NEW_PPC_INSTR();
	PPC_SET_OPCODE(ppc, PPC_OPCODE_X);
	PPC_SET_FUNC  (ppc, PPC_FUNC_MTSPR);
	PPC_SET_RD    (ppc, 1);
	PPC_SET_SPR   (ppc, 0x120);
	set_next_dst(ppc);
	//lis	r1, (ppc_block->end_address+4)@ha(0)
	ppc = NEW_PPC_INSTR();
	PPC_SET_OPCODE(ppc, PPC_OPCODE_ADDIS);
	PPC_SET_RD    (ppc, 1);
	PPC_SET_IMMED (ppc, (ppc_block->end_address+4)>>16);
	set_next_dst(ppc);
	//la	r0, (ppc_block->end_address+4)@l(r1)
	ppc = NEW_PPC_INSTR();
	PPC_SET_OPCODE(ppc, PPC_OPCODE_ORI);
	PPC_SET_RD    (ppc, 1);
	PPC_SET_IMMED (ppc, (ppc_block->end_address+4));
	set_next_dst(ppc);
	//mfctr	r1
	ppc = NEW_PPC_INSTR();
	PPC_SET_OPCODE(ppc, PPC_OPCODE_X);
	PPC_SET_FUNC  (ppc, PPC_FUNC_MFSPR);
	PPC_SET_RD    (ppc, 1);
	PPC_SET_SPR   (ppc, 0x120);
	set_next_dst(ppc);
	
	if(!generated){
		generated = 1;
		PowerPC_instr* current = padCode;
		
		//mtctr	r1
		PPC_SET_OPCODE(*current, PPC_OPCODE_X);
		PPC_SET_FUNC  (*current, PPC_FUNC_MTSPR);
		PPC_SET_RD    (*current, 1);
		PPC_SET_SPR   (*current, 0x120);
		++current;
		//lis	r1, emu_reg@ha(0)
		PPC_SET_OPCODE(*current, PPC_OPCODE_ADDIS);
		PPC_SET_RD    (*current, 1);
		PPC_SET_IMMED (*current, (unsigned int)emu_reg>>16);
		++current;
		//la	r1, emu_reg@l(r1)
		PPC_SET_OPCODE(*current, PPC_OPCODE_ORI);
		PPC_SET_RD    (*current, 1);
		PPC_SET_RA    (*current, 1);
		PPC_SET_IMMED (*current, (unsigned int)emu_reg);
		++current;
		//stw	r0, 3*4(r1)       // pass dest address as arg0
		PPC_SET_OPCODE(*current, PPC_OPCODE_STW);
		PPC_SET_RA    (*current, 1);
		PPC_SET_IMMED (*current, 3*4);
		++current;
		//lis	r1, jump_to@ha(0)
		PPC_SET_OPCODE(*current, PPC_OPCODE_ADDIS);
		PPC_SET_RD    (*current, 1);
		PPC_SET_IMMED (*current, (unsigned int)jump_to>>16);
		++current;
		//la	r1, jump_to@l(r1)
		PPC_SET_OPCODE(*current, PPC_OPCODE_ORI);
		PPC_SET_RD    (*current, 1);
		PPC_SET_RA    (*current, 1);
		PPC_SET_IMMED (*current, (unsigned int)jump_to);
		++current;
		//move	r0, r1
		PPC_SET_OPCODE(*current, PPC_OPCODE_ADDI);
		PPC_SET_RA    (*current, 1);
		++current;
		//lis	r1, &return_address@ha(0)
		PPC_SET_OPCODE(*current, PPC_OPCODE_ADDIS);
		PPC_SET_RD    (*current, 1);
		PPC_SET_IMMED (*current, (unsigned int)&return_address>>16);
		++current;
		//la	r1, &return_address@l(r1)
		PPC_SET_OPCODE(*current, PPC_OPCODE_ORI);
		PPC_SET_RD    (*current, 1);
		PPC_SET_RA    (*current, 1);
		PPC_SET_IMMED (*current, (unsigned int)&return_address);
		++current;
		//stw	r0, 0(r1) // return to jump_to(dest)
		PPC_SET_OPCODE(*current, PPC_OPCODE_STW);
		PPC_SET_RA    (*current, 1);
		++current;
		//lis	r1, return_from_code@ha(0)
		PPC_SET_OPCODE(*current, PPC_OPCODE_ADDIS);
		PPC_SET_RD    (*current, 1);
		PPC_SET_IMMED (*current, (unsigned int)return_from_code>>16);
		++current;
		//la	r1, return_from_code@l(r1)
		PPC_SET_OPCODE(*current, PPC_OPCODE_ORI);
		PPC_SET_RD    (*current, 1);
		PPC_SET_RA    (*current, 1);
		PPC_SET_IMMED (*current, (unsigned int)return_from_code);
		++current;
		//move	r0, r1
		PPC_SET_OPCODE(*current, PPC_OPCODE_ADDI);
		PPC_SET_RA    (*current, 1);
		++current;
		//mfctr	r1
		PPC_SET_OPCODE(*current, PPC_OPCODE_X);
		PPC_SET_FUNC  (*current, PPC_FUNC_MFSPR);
		PPC_SET_RD    (*current, 1);
		PPC_SET_SPR   (*current, 0x120);
		++current;
		//mtctr	r0
		PPC_SET_OPCODE(*current, PPC_OPCODE_X);
		PPC_SET_FUNC  (*current, PPC_FUNC_MTSPR);
		PPC_SET_SPR   (*current, 0x120);
		++current;
		//bctr		// return_from_code();
		PPC_SET_OPCODE(*current, PPC_OPCODE_XL);
		PPC_SET_FUNC  (*current, PPC_FUNC_BCCTR);
		PPC_SET_BO    (*current, 0x14);
	 }
	 
	 *code_length += 16;
	 memcpy(dst, padCode, 16*sizeof(PowerPC_instr));
}

#if 0
// NO LONGER USED
// This creates a call to recompile the block
static void genRecompileBlock(PowerPC_block* ppc_block){
	static PowerPC_instr callCode[32];
	static int generated = 0;
	
	if(!generated){
		generated = 1;
		PowerPC_instr* current = callCode;
		PowerPC_instr ppc = NEW_PPC_INSTR();
		
		// -- First save the used registers --
		PPC_SET_OPCODE(ppc, PPC_OPCODE_ADDI);
		PPC_SET_RD    (ppc, 0);
		PPC_SET_RA    (ppc, 1);
		*(current++) = ppc;
		ppc = NEW_PPC_INSTR();
		PPC_SET_OPCODE(ppc, PPC_OPCODE_ADDIS);
		PPC_SET_RD    (ppc, 1);
		PPC_SET_IMMED (ppc, (unsigned int)reg>>16);
		*(current++) = ppc;
		ppc = NEW_PPC_INSTR();
		PPC_SET_OPCODE(ppc, PPC_OPCODE_ADDI);
		PPC_SET_RD    (ppc, 1);
		PPC_SET_RA    (ppc, 1);
		PPC_SET_IMMED (ppc, (unsigned int)reg);
		*(current++) = ppc;
		ppc = NEW_PPC_INSTR();
		PPC_SET_OPCODE(ppc, PPC_OPCODE_STW);
		PPC_SET_RD    (ppc, 0);
		PPC_SET_RA    (ppc, 1);
		PPC_SET_IMMED (ppc, 1*8 + 4);
		*(current++) = ppc;
		ppc = NEW_PPC_INSTR();
		PPC_SET_OPCODE(ppc, PPC_OPCODE_STW);
		PPC_SET_RD    (ppc, 2);
		PPC_SET_RA    (ppc, 1);
		PPC_SET_IMMED (ppc, 2*8 + 4);
		*(current++) = ppc;
		
		// lis	1, ppc_block@ha(0)
		ppc = NEW_PPC_INSTR();
		PPC_SET_OPCODE(ppc, PPC_OPCODE_ADDIS);
		PPC_SET_RD    (ppc, 2);
		PPC_SET_IMMED (ppc, (unsigned int)ppc_block>>16);
		*(current++) = ppc;
		// li	1, ppc_block@l(1)
		ppc = NEW_PPC_INSTR();
		PPC_SET_OPCODE(ppc, PPC_OPCODE_ORI);
		PPC_SET_RD    (ppc, 2);
		PPC_SET_RA    (ppc, 2);
		PPC_SET_IMMED (ppc, (unsigned int)ppc_block);
		*(current++) = ppc;
		// addi	0, 1, 0
		ppc = NEW_PPC_INSTR();
		PPC_SET_OPCODE(ppc, PPC_OPCODE_ADDI);
		PPC_SET_RD    (ppc, 0);
		PPC_SET_RA    (ppc, 1);
		*(current++) = ppc;
		// lis	1, emu_reg@ha(0)
		ppc = NEW_PPC_INSTR();
		PPC_SET_OPCODE(ppc, PPC_OPCODE_ADDIS);
		PPC_SET_RD    (ppc, 1);
		PPC_SET_IMMED (ppc, (unsigned int)emu_reg>>16);
		*(current++) = ppc;
		// li	1, emu_reg@l(1)
		ppc = NEW_PPC_INSTR();
		PPC_SET_OPCODE(ppc, PPC_OPCODE_ORI);
		PPC_SET_RD    (ppc, 1);
		PPC_SET_RA    (ppc, 1);
		PPC_SET_IMMED (ppc, (unsigned int)emu_reg);
		*(current++) = ppc;
		// stw	0, 3*4(1)	// Store arg0 in emu_reg[3]
		ppc = NEW_PPC_INSTR();
		PPC_SET_OPCODE(ppc, PPC_OPCODE_STW);
		PPC_SET_RD    (ppc, 0);
		PPC_SET_RA    (ppc, 1);
		PPC_SET_IMMED (ppc, 3*4);
		*(current++) = ppc;
		
		// lis	2, recompile_block@ha(0)
		ppc = NEW_PPC_INSTR();
		PPC_SET_OPCODE(ppc, PPC_OPCODE_ADDIS);
		PPC_SET_RD    (ppc, 2);
		PPC_SET_IMMED (ppc, (unsigned int)recompile_block>>16);
		*(current++) = ppc;
		// li	2, recompile_block@l(2)
		ppc = NEW_PPC_INSTR();
		PPC_SET_OPCODE(ppc, PPC_OPCODE_ORI);
		PPC_SET_RD    (ppc, 2);
		PPC_SET_RA    (ppc, 2);
		PPC_SET_IMMED (ppc, (unsigned int)recompile_block);
		*(current++) = ppc;
		// lis	1, return_address@ha(0)
		ppc = NEW_PPC_INSTR();
		PPC_SET_OPCODE(ppc, PPC_OPCODE_ADDIS);
		PPC_SET_RD    (ppc, 1);
		PPC_SET_IMMED (ppc, (unsigned int)&return_address>>16);
		*(current++) = ppc;
		// li	1, return_address@l(1)
		ppc = NEW_PPC_INSTR();
		PPC_SET_OPCODE(ppc, PPC_OPCODE_ORI);
		PPC_SET_RD    (ppc, 1);
		PPC_SET_RA    (ppc, 1);
		PPC_SET_IMMED (ppc, (unsigned int)&return_address);
		*(current++) = ppc;
		// stw	2, 0(1)	// recompile_block is the return address
		ppc = NEW_PPC_INSTR();
		PPC_SET_OPCODE(ppc, PPC_OPCODE_STW);
		PPC_SET_RD    (ppc, 2);
		PPC_SET_RA    (ppc, 1);
		*(current++) = ppc;
		
		// lis	1, return_from_code@ha(0)
		ppc = NEW_PPC_INSTR();
		PPC_SET_OPCODE(ppc, PPC_OPCODE_ADDIS);
		PPC_SET_RD    (ppc, 1);
		PPC_SET_IMMED (ppc, (unsigned int)return_from_code>>16);
		*(current++) = ppc;
		// li	1, return_from_code@l(1)
		ppc = NEW_PPC_INSTR();
		PPC_SET_OPCODE(ppc, PPC_OPCODE_ORI);
		PPC_SET_RD    (ppc, 1);
		PPC_SET_RA    (ppc, 1);
		PPC_SET_IMMED (ppc, (unsigned int)return_from_code);
		*(current++) = ppc;
		// mtctr 1
		ppc = NEW_PPC_INSTR();
		PPC_SET_OPCODE(ppc, PPC_OPCODE_X);
		PPC_SET_FUNC  (ppc, PPC_FUNC_MTSPR);
		PPC_SET_RD    (ppc, 1);
		PPC_SET_SPR   (ppc, 0x120);
		*(current++) = ppc;
		
		// -- Lastly restore the registers --
		ppc = NEW_PPC_INSTR();
		PPC_SET_OPCODE(ppc, PPC_OPCODE_ADDIS);
		PPC_SET_RD    (ppc, 1);
		PPC_SET_IMMED (ppc, (unsigned int)reg>>16);
		*(current++) = ppc;
		ppc = NEW_PPC_INSTR();
		PPC_SET_OPCODE(ppc, PPC_OPCODE_ORI);
		PPC_SET_RD    (ppc, 1);
		PPC_SET_RA    (ppc, 1);
		PPC_SET_IMMED (ppc, (unsigned int)reg);
		*(current++) = ppc;
		ppc = NEW_PPC_INSTR();
		PPC_SET_OPCODE(ppc, PPC_OPCODE_LWZ);
		PPC_SET_RD    (ppc, 2);
		PPC_SET_RA    (ppc, 1);
		PPC_SET_IMMED (ppc, 2*8 + 4);
		*(current++) = ppc;
		ppc = NEW_PPC_INSTR();
		PPC_SET_OPCODE(ppc, PPC_OPCODE_LWZ);
		PPC_SET_RD    (ppc, 1);
		PPC_SET_RA    (ppc, 1);
		PPC_SET_IMMED (ppc, 1*8 + 4);
		*(current++) = ppc;
		
		// bctr
		ppc = NEW_PPC_INSTR();
		PPC_SET_OPCODE(ppc, PPC_OPCODE_XL);
		PPC_SET_FUNC  (ppc, PPC_FUNC_BCCTR);
		PPC_SET_BO    (ppc, 0x14);
		*(current++) = ppc;
	}
	memcpy(ppc_block->code, callCode, 32*sizeof(PowerPC_instr));
}
#endif

