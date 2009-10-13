/* Wrappers.h - Wrappers for recompiled code, these start and return from N64 code
   by Mike Slegeir for Mupen64-GC
 */

#ifndef WRAPPERS_H
#define WRAPPERS_H

#include "Recompile.h"

#define DYNAREG_REG    14
#define DYNAREG_ZERO   15
#define DYNAREG_INTERP 16
#define DYNAREG_UCOUNT 17
#define DYNAREG_LADDR  18
#define DYNAREG_RDRAM  19
#define DYNAREG_SPDMEM 20
#define DYNAREG_FPR_32 21
#define DYNAREG_FPR_64 22
#define DYNAREG_FCR31  23
#define DYNAREG_CHKFP  24

#define DYNAOFF_LR     20

extern long long int reg[34]; // game's registers
extern float*  reg_cop1_simple[32]; // 32-bit fprs
extern double* reg_cop1_double[32]; // 64-bit fprs

extern int noCheckInterrupt;

unsigned int decodeNInterpret(MIPS_instr, unsigned int, int);
int dyna_update_count(unsigned int pc);

//cop0 macros
#define Index reg_cop0[0]
#define Random reg_cop0[1]
#define EntryLo0 reg_cop0[2]
#define EntryLo1 reg_cop0[3]
#define Context reg_cop0[4]
#define PageMask reg_cop0[5]
#define Wired reg_cop0[6]
#define BadVAddr reg_cop0[8]
#define Count reg_cop0[9]
#define EntryHi reg_cop0[10]
#define Compare reg_cop0[11]
#define Status reg_cop0[12]
#define Cause reg_cop0[13]
#define EPC reg_cop0[14]
#define PRevID reg_cop0[15]
#define Config reg_cop0[16]
#define LLAddr reg_cop0[17]
#define WatchLo reg_cop0[18]
#define WatchHi reg_cop0[19]
#define XContext reg_cop0[20]
#define PErr reg_cop0[26]
#define CacheErr reg_cop0[27]
#define TagLo reg_cop0[28]
#define TagHi reg_cop0[29]
#define ErrorEPC reg_cop0[30]

#endif

