/* Wrappers.h - Wrappers for recompiled code, these start and return from N64 code
   by Mike Slegeir for Mupen64-GC
 */

#ifndef WRAPPERS_H
#define WRAPPERS_H

#include "Recompile.h"

// Set return_address to where the N64 should return to 
extern void (*return_address)(void);
extern long long int reg[32]; // game's registers
extern long long int reg_cop1_fgr_64[32];
extern int emu_reg[32]; // emulators registers
extern double emu_fpr[32];
extern int fpu_in_use;
extern int lr[8]; // link register stack
extern int lr_i;

void start(PowerPC_block*, unsigned int offset);
void return_from_code(void);
void decodeNInterpret(); // instr passed through r0
void fp_restore(void); // Restore's N64 FPRs and sets fpu_in_use

#endif

