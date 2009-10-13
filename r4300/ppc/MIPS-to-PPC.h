/* MIPS-to-PPC.h - Function headers for converting MIPS code to PPC
   by Mike Slegeir for Mupen64-GC
 */

#ifndef MIPS_TO_PPC_H
#define MIPS_TO_PPC_H

#include "MIPS.h"
#include "PowerPC.h"

#define CONVERT_ERROR   -1
#define CONVERT_SUCCESS  0
#define CONVERT_WARNING  1
#define INTERPRETED      2

/* These functions must be implemented
   by code using this module           */
extern MIPS_instr get_next_src(void);
extern MIPS_instr peek_next_src(void);
extern void       set_next_dst(PowerPC_instr);
// These are unfortunate hacks necessary for jumping to delay slots
extern PowerPC_instr* get_curr_dst(void);
extern void unget_last_src(void);
extern void nop_ignored(void);
extern unsigned int get_src_pc(void);
// Adjust code_addr to not include flushing of previous mappings
void reset_code_addr(void);
/* Adds src and dst address, and src jump address to tables
    it returns a unique address identifier.
   This data should be used to fill in addresses in pass two. */
extern int  add_jump(int old_address, int is_li, int is_aa);
extern int  is_j_out(int branch, int is_aa);
// Use these for jumps that won't be known until later in compile time
extern int  add_jump_special(int is_j);
extern void set_jump_special(int which, int new_jump);
// Set up appropriate register mappings
void start_new_block(void);
void start_new_mapping(void);

/* Convert one conceptual instruction
    this may use and/or generate more
    than one actual instruction       */
int convert(void);

#endif
