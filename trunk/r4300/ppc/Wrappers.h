/* Wrappers.h - Wrappers for recompiled code, these start and return from N64 code
   by Mike Slegeir for Mupen64-GC
 */

#ifndef WRAPPERS_H
#define WRAPPERS_H

#include "Recompile.h"

// Set return_address to where the N64 should return to 
extern void (*return_address)(void);
extern long long int reg[32]; // game's registers
extern int emu_reg[32]; // emulators registers
extern int lr[8]; // link register stack
extern int lr_i;

void start(PowerPC_block*, unsigned int offset);
void return_from_code(void);

#endif

