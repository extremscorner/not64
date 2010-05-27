/**
 * Wii64 - Interpreter.h
 * Copyright (C) 2007, 2008, 2009, 2010 Mike Slegeir
 * 
 * Defines and functions for calling the interpreter
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

#ifndef INTERPRETER_H
#define INTERPRETER_H

/* decodeNInterpret(MIPS_instr, unsigned int PC):
	1) Saves state
	2) Calls prefetch_opcode(instr)               (Decodes)
	3) Calls interp_ops[MIPS_GET_OPCODE(instr)]() (Interprets)
	4) Restores state
 */
unsigned int decodeNInterpret();

// These defines tell the recompiler to interpret
//  rather than recompile the instruction
#if 0
#define INTERPRET_LB
#define INTERPRET_LBU
#define INTERPRET_LH
#define INTERPRET_LHU
#define INTERPRET_LW
#define INTERPRET_LWU
#define INTERPRET_LD
#endif
#define INTERPRET_LWL
#define INTERPRET_LWR

#if 0
#define INTERPRET_SB
#define INTERPRET_SH
#define INTERPRET_SW
#define INTERPRET_SD
#endif
#define INTERPRET_SWL
#define INTERPRET_SWR

#define INTERPRET_LDL
#define INTERPRET_LDR

#define INTERPRET_SDL
#define INTERPRET_SDR

#if 0
#define INTERPRET_LWC1
#define INTERPRET_LDC1
#define INTERPRET_SWC1
#define INTERPRET_SDC1
#endif

//#define INTERPRET_DW

#if 0
#define INTERPRET_DADDIU
#define INTERPRET_DSLLV
#define INTERPRET_DSRLV
#define INTERPRET_DSRAV
#define INTERPRET_DADDU
#define INTERPRET_DSUBU
#define INTERPRET_DSLL
#define INTERPRET_DSRL
#define INTERPRET_DSRA
#define INTERPRET_DSLL32
#define INTERPRET_DSRL32
#define INTERPRET_DSRA32
#endif

#define INTERPRET_DMULT
#define INTERPRET_DMULTU
#define INTERPRET_DDIV
#define INTERPRET_DDIVU

//#define INTERPRET_HILO

#if 0
#define INTERPRET_MULT
#define INTERPRET_MULTU
#define INTERPRET_DIV
#define INTERPRET_DIVU
#endif

#if 0
#define INTERPRET_SLT
#define INTERPRET_SLTU
#define INTERPRET_SLTI
#define INTERPRET_SLTIU
#endif

//#define INTERPRET_J
//#define INTERPRET_JAL
#define INTERPRET_JR
#define INTERPRET_JALR
//#define INTERPRET_BC
//#define INTERPRET_BRANCH

#define INTERPRET_SYSCALL
#define INTERPRET_BREAK
#define INTERPRET_TRAPS

#define INTERPRET_LL
#define INTERPRET_SC

//#define INTERPRET_COP0
//#define INTERPRET_MFC0
//#define INTERPRET_MTC0
//#define INTERPRET_TLB
//#define INTERPRET_ERET
#define INTERPRET_TLBR
#define INTERPRET_TLBWI
#define INTERPRET_TLBWR
#define INTERPRET_TLBP

//#define INTERPRET_FP

#if 0
#define INTERPRET_MFC1
#define INTERPRET_DMFC1
#define INTERPRET_CFC1
#define INTERPRET_MTC1
#define INTERPRET_DMTC1
#define INTERPRET_CTC1
#endif

#if 0
#define INTERPRET_FP_S
#define INTERPRET_FP_D
#define INTERPRET_FP_W
#define INTERPRET_FP_L
#endif

#if 0
#define INTERPRET_FP_ADD
#define INTERPRET_FP_SUB
#define INTERPRET_FP_MUL
#define INTERPRET_FP_DIV
#define INTERPRET_FP_ABS
#define INTERPRET_FP_MOV
#define INTERPRET_FP_NEG
#endif
//#define INTERPRET_FP_SQRT

#if 0
#define INTERPRET_FP_ROUND_L
#define INTERPRET_FP_TRUNC_L
#define INTERPRET_FP_FLOOR_L
#define INTERPRET_FP_CEIL_L
#endif
#if 0
#define INTERPRET_FP_ROUND_W
#define INTERPRET_FP_TRUNC_W
#define INTERPRET_FP_FLOOR_W
#define INTERPRET_FP_CEIL_W
#endif
#if 0
#define INTERPRET_FP_CVT_S
#define INTERPRET_FP_CVT_D
#define INTERPRET_FP_CVT_W
#define INTERPRET_FP_CVT_L
#endif

#if 0
#define INTERPRET_FP_C_F
#define INTERPRET_FP_C_UN
#define INTERPRET_FP_C_EQ
#define INTERPRET_FP_C_UEQ
#define INTERPRET_FP_C_OLT
#define INTERPRET_FP_C_ULT
#define INTERPRET_FP_C_OLE
#define INTERPRET_FP_C_ULE
#define INTERPRET_FP_C_SF
#define INTERPRET_FP_C_NGLE
#define INTERPRET_FP_C_SEQ
#define INTERPRET_FP_C_NGL
#define INTERPRET_FP_C_LT
#define INTERPRET_FP_C_NGE
#define INTERPRET_FP_C_LE
#define INTERPRET_FP_C_NGT
#endif

#endif

