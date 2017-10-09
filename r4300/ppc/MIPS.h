/**
 * Wii64 - MIPS.h
 * Copyright (C) 2007, 2008, 2009 Mike Slegeir
 * 
 * Defines and macros for decoding MIPS instructions
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

/* Example usage:
	MIPS_instr x = mips_code[i++];
	if(MIPS_GET_OPCODE(x) == MIPS_OPCODE_R)
		if(MIPS_GET_FUNC(x) == MIPS_FUNC_ADD)
			printf("add r%d, r%d, r%d", MIPS_GET_RD(x),
			         MIPS_GET_RS(x), MIPS_GET_RT(x));
 */

#ifndef MIPS_H
#define MIPS_H

typedef unsigned int MIPS_instr;

// fields
#define MIPS_OPCODE_MASK  0x3F
#define MIPS_OPCODE_SHIFT 26
#define MIPS_GET_OPCODE(instr)  ((instr >> MIPS_OPCODE_SHIFT) & MIPS_OPCODE_MASK)

#define MIPS_REG_MASK     0x1F

#define MIPS_RA_MASK      MIPS_REG_MASK
#define MIPS_RA_SHIFT     21
#define MIPS_GET_RA(instr)      ((instr >> MIPS_RA_SHIFT)     & MIPS_RA_MASK)
#define MIPS_GET_RS  MIPS_GET_RA

#define MIPS_RB_MASK      MIPS_REG_MASK
#define MIPS_RB_SHIFT     16
#define MIPS_GET_RB(instr)      ((instr >> MIPS_RB_SHIFT)     & MIPS_RB_MASK)
#define MIPS_GET_RT  MIPS_GET_RB

#define MIPS_RD_MASK      MIPS_REG_MASK
#define MIPS_RD_SHIFT     11
#define MIPS_GET_RD(instr)      ((instr >> MIPS_RD_SHIFT)     & MIPS_RD_MASK)

#define MIPS_SA_MASK      0x1F
#define MIPS_SA_SHIFT     6
#define MIPS_GET_SA(instr)      ((instr >> MIPS_SA_SHIFT)     & MIPS_SA_MASK)

#define MIPS_FUNC_MASK    0x3F
#define MIPS_FUNC_SHIFT   0
#define MIPS_GET_FUNC(instr)    ((instr >> MIPS_FUNC_SHIFT)   & MIPS_FUNC_MASK)

#define MIPS_IMMED_MASK   0xFFFF
#define MIPS_IMMED_SHIFT  0
#define MIPS_GET_IMMED(instr)   ((instr >> MIPS_IMMED_SHIFT)  & MIPS_IMMED_MASK)

#define MIPS_LI_MASK      0x3FFFFFF
#define MIPS_LI_SHIFT     0
#define MIPS_GET_LI(instr)      ((instr >> MIPS_LI_SHIFT)     & MIPS_LI_MASK)
#define MIPS_GET_TARGET MIPS_GET_LI

#define MIPS_CC_MASK      0x7
#define MIPS_CC_SHIFT     18
#define MIPS_GET_CC(instr)      ((instr >> MIPS_CC_SHIFT)     & MIPS_CC_MASK)

#define MIPS_GET_FORMAT MIPS_GET_RS
#define MIPS_GET_FT     MIPS_GET_RT
#define MIPS_GET_FS     MIPS_GET_RD
#define MIPS_GET_FD     MIPS_GET_SA

// opcodes
#define MIPS_OPCODE_R       0
#define MIPS_OPCODE_B       1
#define MIPS_OPCODE_J       2
#define MIPS_OPCODE_JAL     3
#define MIPS_OPCODE_BEQ     4
#define MIPS_OPCODE_BNE     5
#define MIPS_OPCODE_BLEZ    6
#define MIPS_OPCODE_BGTZ    7
#define MIPS_OPCODE_ADDI    8
#define MIPS_OPCODE_ADDIU   9
#define MIPS_OPCODE_SLTI    10
#define MIPS_OPCODE_SLTIU   11
#define MIPS_OPCODE_ANDI    12
#define MIPS_OPCODE_ORI     13
#define MIPS_OPCODE_XORI    14
#define MIPS_OPCODE_LUI     15

#define MIPS_OPCODE_DADDI   24
#define MIPS_OPCODE_DADDIU  25

#define MIPS_OPCODE_COP0    16
#define MIPS_OPCODE_COP1    17
#define MIPS_OPCODE_COP2    18

#define MIPS_OPCODE_BEQL    20
#define MIPS_OPCODE_BNEL    21
#define MIPS_OPCODE_BLEZL   22
#define MIPS_OPCODE_BGTZL   23

#define MIPS_OPCODE_M       28

#define MIPS_OPCODE_LB      32
#define MIPS_OPCODE_LH      33
#define MIPS_OPCODE_LWL     34
#define MIPS_OPCODE_LW      35
#define MIPS_OPCODE_LBU     36
#define MIPS_OPCODE_LHU     37
#define MIPS_OPCODE_LWR     38
#define MIPS_OPCODE_LWU     39

#define MIPS_OPCODE_LD      55
#define MIPS_OPCODE_LDL     26
#define MIPS_OPCODE_LDR     27
#define MIPS_OPCODE_LLD     52

#define MIPS_OPCODE_SB      40
#define MIPS_OPCODE_SH      41
#define MIPS_OPCODE_SWL     42
#define MIPS_OPCODE_SW      43

#define MIPS_OPCODE_SD      63
#define MIPS_OPCODE_SDL     44
#define MIPS_OPCODE_SDR     45
#define MIPS_OPCODE_SCD     60

#define MIPS_OPCODE_SWR     46
#define MIPS_OPCODE_CACHE   47
#define MIPS_OPCODE_LL      48
#define MIPS_OPCODE_LWC1    49
#define MIPS_OPCODE_LWC2    50
#define MIPS_OPCODE_PREF    51

#define MIPS_OPCODE_LDC1    53
#define MIPS_OPCODE_LDC2    54

#define MIPS_OPCODE_SC      56
#define MIPS_OPCODE_SWC1    57
#define MIPS_OPCODE_SWC2    58

#define MIPS_OPCODE_SDC1    61
#define MIPS_OPCODE_SDC2    62

// function codes
// R-type
#define MIPS_FUNC_SLL       0
#define MIPS_FUNC_MOV       1 // bit 16: 0 = from, 1 = to
#define MIPS_FUNC_SRL       2
#define MIPS_FUNC_SRA       3
#define MIPS_FUNC_SLLV      4
#define MIPS_FUNC_SRLV      6
#define MIPS_FUNC_SRAV      7

#define MIPS_FUNC_JR        8
#define MIPS_FUNC_JALR      9
#define MIPS_FUNC_MOVZ      10
#define MIPS_FUNC_MOVN      11
#define MIPS_FUNC_SYSCALL   12
#define MIPS_FUNC_BREAK     13
#define MIPS_FUNC_SYNC      15

#define MIPS_FUNC_MFHI      16
#define MIPS_FUNC_MTHI      17
#define MIPS_FUNC_MFLO      18
#define MIPS_FUNC_MTLO      19

#define MIPS_FUNC_MULT      24
#define MIPS_FUNC_MULTU     25
#define MIPS_FUNC_DIV       26
#define MIPS_FUNC_DIVU      27
#define MIPS_FUNC_ADD       32
#define MIPS_FUNC_ADDU      33
#define MIPS_FUNC_SUB       34
#define MIPS_FUNC_SUBU      35
#define MIPS_FUNC_AND       36
#define MIPS_FUNC_OR        37
#define MIPS_FUNC_XOR       38
#define MIPS_FUNC_NOR       39

#define MIPS_FUNC_DADD      44
#define MIPS_FUNC_DADDU     45
#define MIPS_FUNC_DDIV      30
#define MIPS_FUNC_DDIVU     31
#define MIPS_FUNC_DMULT     28
#define MIPS_FUNC_DMULTU    29
#define MIPS_FUNC_DSLL      56
#define MIPS_FUNC_DSLL32    60
#define MIPS_FUNC_DSLLV     20
#define MIPS_FUNC_DSRA      59
#define MIPS_FUNC_DSRA32    63
#define MIPS_FUNC_DSRAV     23
#define MIPS_FUNC_DSRL      58
#define MIPS_FUNC_DSRL32    62
#define MIPS_FUNC_DSRLV     22
#define MIPS_FUNC_DSUB      46
#define MIPS_FUNC_DSUBU     47

#define MIPS_FUNC_SLT       42
#define MIPS_FUNC_SLTU      43

#define MIPS_FUNC_TGE       48
#define MIPS_FUNC_TGEU      49
#define MIPS_FUNC_TLT       50
#define MIPS_FUNC_TLTU      51
#define MIPS_FUNC_TEQ       52
#define MIPS_FUNC_TNE       54

// B-type : These values go in the rt/rb field
#define MIPS_RT_BLTZ        0
#define MIPS_RT_BGEZ        1
#define MIPS_RT_BLTZL       2
#define MIPS_RT_BGEZL       3

#define MIPS_RT_TGEI        8
#define MIPS_RT_TGEIU       9
#define MIPS_RT_TLTI        10
#define MIPS_RT_TLTIU       11
#define MIPS_RT_TEGI        12
#define MIPS_RT_TNEI        14

#define MIPS_RT_BLTZAL      16
#define MIPS_RT_BGEZAL      17
#define MIPS_RT_BLTZALL     18
#define MIPS_RT_BGEZALL     19

// M Op
#define MIPS_FUNC_MADD      0
#define MIPS_FUNC_MADDU     1
#define MIPS_FUNC_MUL       2
#define MIPS_FUNC_MSUB      4
#define MIPS_FUNC_MSUBU     5

#define MIPS_FUNC_CLZ       32
#define MIPS_FUNC_CLO       33

// CoP-type : These values go in the format field
#define MIPS_FRMT_MFC       0
#define MIPS_FRMT_DMFC      1
#define MIPS_FRMT_CFC       2
#define MIPS_FRMT_MTC       4
#define MIPS_FRMT_DMTC      5
#define MIPS_FRMT_CTC       6
#define MIPS_FRMT_BC        8 // f/t and l determined by bits 17:16
#define MIPS_FRMT_COP1      16
#define MIPS_FRMT_COP2      17
#define MIPS_FRMT_COP3      20 // 3/4 Are Word and Long respectively
#define MIPS_FRMT_COP4      21 // according to some N64 docs, but not MIPS docs

// Floating point coproccessor instructions
#define MIPS_FUNC_ADD_      0
#define MIPS_FUNC_SUB_      1
#define MIPS_FUNC_MUL_      2
#define MIPS_FUNC_DIV_      3
#define MIPS_FUNC_SQRT_     4
#define MIPS_FUNC_ABS_      5
#define MIPS_FUNC_MOV_      6
#define MIPS_FUNC_NEG_      7

#define MIPS_FUNC_ROUND_W_  12
#define MIPS_FUNC_TRUNC_W_  13
#define MIPS_FUNC_CEIL_W_   14
#define MIPS_FUNC_FLOOR_W_  15
#define MIPS_FUNC_ROUND_L_  8
#define MIPS_FUNC_TRUNC_L_  9
#define MIPS_FUNC_CEIL_L_   10
#define MIPS_FUNC_FLOOR_L_  11

#define MIPS_FUNC_MOV__     17 // bit 16 determines t/f
#define MIPS_FUNC_MOVZ_     18
#define MIPS_FUNC_MOVN_     19

#define MIPS_FUNC_CVT_S_    32
#define MIPS_FUNC_CVT_D_    33
#define MIPS_FUNC_CVT_W_    36
#define MIPS_FUNC_CVT_L_    37

#define MIPS_FUNC_C_F_      48
#define MIPS_FUNC_C_UN_     49
#define MIPS_FUNC_C_EQ_     50
#define MIPS_FUNC_C_UEQ     51
#define MIPS_FUNC_C_OLT_    52
#define MIPS_FUNC_C_ULT_    53
#define MIPS_FUNC_C_OLE_    54
#define MIPS_FUNC_C_ULE_    55
#define MIPS_FUNC_C_SF_     56
#define MIPS_FUNC_C_NGLE_   57
#define MIPS_FUNC_C_SEQ_    58
#define MIPS_FUNC_C_NGL_    59
#define MIPS_FUNC_C_LT_     60
#define MIPS_FUNC_C_NGE_    61
#define MIPS_FUNC_C_LE_     62
#define MIPS_FUNC_C_NGT_    63

// COP0 Instructions
#define MIPS_FUNC_TLBR      1
#define MIPS_FUNC_TLBWI     2
#define MIPS_FUNC_TLBWR     6
#define MIPS_FUNC_TLBP      8
#define MIPS_FUNC_ERET      24
#define MIPS_FUNC_DERET     31

#define MIPS_REG_K0	26	// Kernel reserved
#define MIPS_REG_K1	27	// Kernel reserved
#define MIPS_REG_GP	28	// Global pointer
#define MIPS_REG_SP	29	// Stack  pointer
#define MIPS_REG_FP	30	// Frame  pointer
#define MIPS_REG_LR	31	// Link   register

#endif
