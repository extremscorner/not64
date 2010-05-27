/**
 * Wii64 - Recompile.h
 * Copyright (C) 2007, 2008, 2009, 2010 Mike Slegeir
 * 
 * Functions and data structures for recompiling blocks of code
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

#ifndef RECOMPILE_H
#define RECOMPILE_H

#include "MIPS-to-PPC.h"

typedef unsigned int uint;

typedef struct hole_node {
	unsigned short addr;
	struct hole_node* next;
} PowerPC_func_hole_node;

struct func;

typedef struct func_node {
	struct func* function;
	struct func_node* left;
	struct func_node* right;
} PowerPC_func_node;

typedef struct link_node {
	PowerPC_instr*    branch;
	struct func*      func;
	struct link_node* next;
} PowerPC_func_link_node;

typedef struct func {
	unsigned int start_addr;
	unsigned int end_addr;
	PowerPC_instr* code;
	unsigned int   lru;
	PowerPC_func_hole_node* holes;
	PowerPC_func_link_node* links_in;
	PowerPC_func_node*      links_out;
	PowerPC_instr** code_addr;
} PowerPC_func;

PowerPC_func* find_func(PowerPC_func_node** root, unsigned int addr);
void insert_func(PowerPC_func_node** root, PowerPC_func* func);
void remove_func(PowerPC_func_node** root, PowerPC_func* func);

typedef struct {
	MIPS_instr*     mips_code;     // The code to recompile
	uint            start_address; // The address this code begins for the 64
	uint            end_address;
	//PowerPC_instr** code_addr;     // table of block offsets to code pointer,
	                               //   its length is end_addr - start_addr
	PowerPC_func_node* funcs;      // BST of functions in this block
	unsigned long   adler32;       // Used for TLB
} PowerPC_block;

#define MAX_JUMPS        4096
#define JUMP_TYPE_J      1   // uses a long immed & abs addr
#define JUMP_TYPE_CALL   2   // the jump is to a C function
#define JUMP_TYPE_SPEC   4   // special jump, destination precomputed
typedef struct {
	MIPS_instr*    src_instr;
	PowerPC_instr* dst_instr;
	int            old_jump;
	int            new_jump;
	uint           type;
} jump_node;

MIPS_instr get_next_src(void);
MIPS_instr peek_next_src(void);
int        has_next_src(void);
void       set_next_dst(PowerPC_instr);
int        add_jump(int old_jump, int is_j, int is_out);
int        is_j_out(int branch, int is_aa);
// Use these for jumps that won't be known until later in compile time
int        add_jump_special(int is_j);
void       set_jump_special(int which, int new_jump);

int  func_was_freed(PowerPC_func*);
void clear_freed_funcs(void);

/* These functions are used to initialize, recompile, and deinit a block
   init assumes that all pointers in the block fed it it are NULL or allocated
   memory. Deinit frees a block with the same preconditions.
 */
PowerPC_func* recompile_block(PowerPC_block* ppc_block, unsigned int addr);
void init_block  (MIPS_instr* mips_code, PowerPC_block* ppc_block);
void deinit_block(PowerPC_block* ppc_block);

#ifdef HW_RVL
#include "../../gc_memory/MEM2.h"
extern PowerPC_block **blocks;
#else
#ifndef ARAM_BLOCKCACHE
extern PowerPC_block *blocks[0x100000];
#endif
#endif

#endif

