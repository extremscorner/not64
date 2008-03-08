/* Recompile.h - Headers and data structures for recompiling blocks of code
   by Mike Slegeir  for Mupen64-GC
 */

#ifndef RECOMPILE_H
#define RECOMPILE_H

#include "MIPS-to-PPC.h"

typedef unsigned int uint;

typedef struct {
	PowerPC_instr*  code;          // The actual recompiled code
	uint            length;        // Length of the code
	uint            max_length;    // Length of the instruction buffer
	MIPS_instr*     mips_code;     // The code to recompile
	uint            start_address; // The address this code begins for the 64
	uint            end_address;
	PowerPC_instr** code_addr;     // table of block offsets to code pointer,
	                               //   its length is end_addr - start_addr
	unsigned long   adler32;       // Used for TLB
} PowerPC_block;

#define MAX_JUMPS        1024
#define JUMP_TYPE_J      1   // uses a long immed & abs addr
#define JUMP_TYPE_OUT    2   // the jump leaves the block
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

/* These functions are used to initialize, recompile, and deinit a block
   init assumes that all pointers in the block fed it it are NULL or allocated
   memory. Deinit frees a block with the same preconditions.
 */
void recompile_block(PowerPC_block* ppc_block);
void init_block  (MIPS_instr* mips_code, PowerPC_block* ppc_block);
void deinit_block(PowerPC_block* ppc_block);

extern PowerPC_block* blocks[0x100000];

#endif

