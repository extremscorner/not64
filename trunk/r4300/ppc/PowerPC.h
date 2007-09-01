/* PowerPC.h - defines and macros for encoding PPC instructions
   by Mike Slegeir for Mupen64-GC
 **************************************************************
   Example usage (creating add r1, r2, r3):
   	PowerPC_instr x = NEW_PPC_INSTR();
   	PPC_SET_OPCODE(x, PPC_OPCODE_X);
	PPC_SET_FUNC  (x, PPC_FUNC_ADD);
   	PPC_SET_RD(x, R1);
   	PPC_SET_RA(x, R2);
   	PPC_SET_RB(x, R3);
 */

#ifndef POWERPC_H
#define POWERPC_H

// type definitions
typedef unsigned int PowerPC_instr;

// macros
#define NEW_PPC_INSTR() 0

// fields
#define PPC_OPCODE_MASK  0x3F
#define PPC_OPCODE_SHIFT 26
#define PPC_SET_OPCODE(instr,opcode) instr |= (opcode&PPC_OPCODE_MASK) << PPC_OPCODE_SHIFT

#define PPC_REG_MASK     0x1F

#define PPC_RD_MASK      PPC_REG_MASK
#define PPC_RD_SHIFT     21
#define PPC_SET_RD(instr,rd)         instr |= (rd&PPC_RD_MASK)         << PPC_RD_SHIFT

#define PPC_RA_MASK      PPC_REG_MASK
#define PPC_RA_SHIFT     16
#define PPC_SET_RA(instr,ra)         instr |= (ra&PPC_RA_MASK)         << PPC_RA_SHIFT

#define PPC_RB_MASK      PPC_REG_MASK
#define PPC_RB_SHIFT     11
#define PPC_SET_RB(instr,rb)         instr |= (rb&PPC_RB_MASK)         << PPC_RB_SHIFT

#define PPC_OE_MASK      0x1
#define PPC_OE_SHIFT     10
#define PPC_SET_OE(instr,oe)         instr |= (oe&PPC_OE_MASK)         << PPC_OE_SHIFT

#define PPC_FUNC_MASK    0x3FF
#define PPC_FUNC_SHIFT   1
#define PPC_SET_FUNC(instr,func)     instr |= (func&PPC_FUNC_MASK)     << PPC_FUNC_SHIFT

#define PPC_CR_MASK      0x1
#define PPC_CR_SHIFT     0
#define PPC_SET_CR(instr,cr)         instr |= (cr&PPC_CR_MASK)         << PPC_CR_SHIFT

#define PPC_IMMED_MASK   0xFFFF
#define PPC_IMMED_SHIFT  0
#define PPC_SET_IMMED(instr,immed)   instr |= (immed&PPC_IMMED_MASK)   << PPC_IMMED_SHIFT

#define PPC_LI_MASK      0xFFFFFF
#define PPC_LI_SHIFT     2
#define PPC_SET_LI(instr,li)         instr |= (li&PPC_LI_MASK)         << PPC_LI_SHIFT

#define PPC_AA_MASK      0x1
#define PPC_AA_SHIFT     1
#define PPC_SET_AA(instr,aa)         instr |= (aa&PPC_AA_MASK)         << PPC_AA_SHIFT

#define PPC_LK_MASK      0x1
#define PPC_LK_SHIFT     0
#define PPC_SET_LK(instr,lk)         instr |= (lk&PPC_LK_MASK)         << PPC_LK_SHIFT

#define PPC_SET_BO PPC_SET_RD
#define PPC_SET_BI PPC_SET_RA

#define PPC_BD_MASK      0x3FFF
#define PPC_BD_SHIFT     2
#define PPC_SET_BD(instr,bd)         instr |= (bd&PPC_BD_MASK)         << PPC_BD_SHIFT

#define PPC_CRF_MASK     0x7
#define PPC_CRF_SHIFT    23
#define PPC_SET_CRF(instr,crf)       instr |= (crf&PPC_CRF_MASK)       << PPC_CRF_SHIFT

#define PPC_SPR_MASK     0x3FF
#define PPC_SPR_SHIFT    11
#define PPC_SET_SPR(instr,spr)       instr |= (spr&PPC_SPR_MASK)       << PPC_SPR_SHIFT

#define PPC_MB_MASK      0x1F
#define PPC_MB_SHIFT     6
#define PPC_SET_MB(instr,mb)         instr |= (mb&PPC_MB_MASK)         << PPC_MB_SHIFT

#define PPC_ME_MASK      0x1F
#define PPC_ME_SHIFT     1
#define PPC_SET_ME(instr,me)         instr |= (me&PPC_ME_MASK)         << PPC_ME_SHIFT

#define PPC_SET_SH       PPC_SET_RB
#define PPC_SET_RC       PPC_SET_MB

// Opcodes
#define PPC_OPCODE_X           31
#define PPC_OPCODE_XL          19
#define PPC_OPCODE_WTF         4

#define PPC_OPCODE_ADDI        14
#define PPC_OPCODE_ADDIC       12
#define PPC_OPCODE_ADDIC_      13
#define PPC_OPCODE_ADDIS       15

#define PPC_OPCODE_ANDI        28
#define PPC_OPCODE_ANDIS       29

#define PPC_OPCODE_CMPI        11
#define PPC_OPCODE_CMPLI       10

#define PPC_OPCODE_LBZ         34
#define PPC_OPCODE_LBZU        35
#define PPC_OPCODE_LHA         42
#define PPC_OPCODE_LHAU        43
#define PPC_OPCODE_LHZ         40
#define PPC_OPCODE_LHZU        41
#define PPC_OPCODE_LMW         46
#define PPC_OPCODE_LWZ         32
#define PPC_OPCODE_LWZU        33

#define PPC_OPCODE_LFS         48
#define PPC_OPCODE_LFD         50
#define PPC_OPCODE_STFS        52
#define PPC_OPCODE_STFD        54

#define PPC_OPCODE_FPD         63
#define PPC_OPCODE_FPS         59

#define PPC_OPCODE_MULLI       7

#define PPC_OPCODE_ORI         24
#define PPC_OPCODE_ORIS        25

#define PPC_OPCODE_STB         38
#define PPC_OPCODE_STBU        39
#define PPC_OPCODE_STBUX       31
#define PPC_OPCODE_STH         44
#define PPC_OPCODE_STHU        45
#define PPC_OPCODE_STMW        47
#define PPC_OPCODE_STW         36
#define PPC_OPCODE_STWU        37

#define PPC_OPCODE_SUBFIC      8

#define PPC_OPCODE_TWI         3

#define PPC_OPCODE_XORI        26
#define PPC_OPCODE_XORIS       27

#define PPC_OPCODE_B           18

#define PPC_OPCODE_BC          16

#define PPC_OPCODE_RLWIMI      20
#define PPC_OPCODE_RLWINM      21
#define PPC_OPCODE_RLWNM       23

#define PPC_OPCODE_SC          17

// Function codes
// X-Form
#define PPC_FUNC_ADD             266
#define PPC_FUNC_ADDC            10
#define PPC_FUNC_ADDE            38
#define PPC_FUNC_ADDME           234
#define PPC_FUNC_ADDZE           202

#define PPC_FUNC_AND             28
#define PPC_FUNC_ANDC            60

// XL-Form
#define PPC_FUNC_BCCTR           528
#define PPC_FUNC_BCLR            16

// X-Form
#define PPC_FUNC_CMP             0
#define PPC_FUNC_CMPL            32

#define PPC_FUNC_CNTLZW          26

// XL-Form
#define PPC_FUNC_CRAND           257
#define PPC_FUNC_CRANDC          129
#define PPC_FUNC_CREQV           289
#define PPC_FUNC_CRNAND          225
#define PPC_FUNC_CRNOR           33
#define PPC_FUNC_CROR            449
#define PPC_FUNC_CRORC           417
#define PPC_FUNC_CRXOR           193

// X-Form
#define PPC_FUNC_DCBA            758
#define PPC_FUNC_DCBF            86
#define PPC_FUNC_DCBI            470
#define PPC_FUNC_DCBST           54
#define PPC_FUNC_DCBT            278
#define PPC_FUNC_DCBTST          246
#define PPC_FUNC_DCBZ            1014
#define PPC_FUNC_DCCCI           454
#define PPC_FUNC_DCREAD          486

// XO-Form
#define PPC_FUNC_DIVW            491
#define PPC_FUNC_DIVWU           459

// X-Form
#define PPC_FUNC_EIEIO           854

#define PPC_FUNC_EQV             284

#define PPC_FUNC_EXTSB           954
#define PPC_FUNC_EXTSH           922

// F-Form
#define PPC_FUNC_FABS            264
#define PPC_FUNC_FADD            21
#define PPC_FUNC_FCFID           846
#define PPC_FUNC_FCMPO           32
#define PPC_FUNC_FCMPU           0
#define PPC_FUNC_FCTID           814
#define PPC_FUNC_FCTIDZ          815
#define PPC_FUNC_FCTIW           14
#define PPC_FUNC_FCTIWZ          15
#define PPC_FUNC_FDIV            18
#define PPC_FUNC_FMADD           29
#define PPC_FUNC_FMR             72
#define PPC_FUNC_FMSUB           28
#define PPC_FUNC_FMUL            25
#define PPC_FUNC_FNABS           136
#define PPC_FUNC_FNEG            40
#define PPC_FUNC_FNMADD          31
#define PPC_FUNC_FNMSUB          30
#define PPC_FUNC_FRSP            12
#define PPC_FUNC_FRSQRTE         26
#define PPC_FUNC_FSEL            23
#define PPC_FUNC_FSQRT           22
#define PPC_FUNC_FSUB            20
#define PPC_FUNC_MTFSB0          70
#define PPC_FUNC_MTFSB1          38

// X-Form
#define PPC_FUNC_ICBI            982
#define PPC_FUNC_ICBT            262
#define PPC_FUNC_ICCCI           966
#define PPC_FUNC_ICREAD          998
#define PPC_FUNC_ISYNC           150

#define PPC_FUNC_LBZUX           119
#define PPC_FUNC_LBZX            87
#define PPC_FUNC_LHAUX           375
#define PPC_FUNC_LHAX            343
#define PPC_FUNC_LHBRX           790
#define PPC_FUNC_LHZUX           311
#define PPC_FUNC_LHZX            279
#define PPC_FUNC_LSWI            597
#define PPC_FUNC_LSWX            533
#define PPC_FUNC_LWARX           20
#define PPC_FUNC_LWBRX           534
#define PPC_FUNC_LWZUX           55
#define PPC_FUNC_LWZX            23

// XO-Form
// These use the WTF opcode
#define PPC_FUNC_MACCHW          172
#define PPC_FUNC_MACCHWS         236
#define PPC_FUNC_MACCHWSU        204
#define PPC_FUNC_MACCHWU         140
#define PPC_FUNC_MACHHW          44
#define PPC_FUNC_MACHHWS         108
#define PPC_FUNC_MACHHWSU        76
#define PPC_FUNC_MACHHWU         12
#define PPC_FUNC_MACLHW          428
#define PPC_FUNC_MACLHWS         492
#define PPC_FUNC_MACLHWSU        460
#define PPC_FUNC_MACLHWU         396

// XL-Form
#define PPC_FUNC_MCRF            0

// X-Form
#define PPC_FUNC_MCRXR           512
#define PPC_FUNC_MFCR            19
#define PPC_FUNC_MFDCR           323
#define PPC_FUNC_MFMSR           83
#define PPC_FUNC_MFSPR           339
#define PPC_FUNC_MFTB            371
#define PPC_FUNC_MTCRF           144
#define PPC_FUNC_MTDCR           451
#define PPC_FUNC_MTMSR           146
#define PPC_FUNC_MTSPR           467

// WTF-Op
#define PPC_FUNC_MULCHW          168
#define PPC_FUNC_MULCHWU         136
#define PPC_FUNC_MULHHW          40
#define PPC_FUNC_MULHHWU         8

// X-Op
#define PPC_FUNC_MULHW           75
#define PPC_FUNC_MULHWU          11

// WTF-Op
#define PPC_FUNC_MULLHW          424
#define PPC_FUNC_MULLHWU         392

// X-Op
#define PPC_FUNC_MULLW           235
#define PPC_FUNC_NAND            476
#define PPC_FUNC_NEG             104

// WTF-Op
#define PPC_FUNC_NMACCHW         174
#define PPC_FUNC_NMACCHWS        238
#define PPC_FUNC_NMACHHW         46
#define PPC_FUNC_NMACHHWS        110
#define PPC_FUNC_NMACLHW         430
#define PPC_FUNC_NMACLHWS        494

// X-Op
#define PPC_FUNC_NOR             124
#define PPC_FUNC_OR              444
#define PPC_FUNC_ORC             412

// XL-Op
#define PPC_FUNC_RFCI            51
#define PPC_FUNC_RFI             50

// X-Op
#define PPC_FUNC_SLW             24
#define PPC_FUNC_SRAW            792
#define PPC_FUNC_SRAWI           824
#define PPC_FUNC_SRW             536

#define PPC_FUNC_STBX            215
#define PPC_FUNC_STHBRX          918
#define PPC_FUNC_STHUX           439
#define PPC_FUNC_STHX            407
#define PPC_FUNC_STSWI           725
#define PPC_FUNC_STSWX           661
#define PPC_FUNC_STWBRX          662
#define PPC_FUNC_STWCX           150
#define PPC_FUNC_STWUX           183
#define PPC_FUNC_STWX            151

#define PPC_FUNC_SUBF            40
#define PPC_FUNC_SUBFC           8
#define PPC_FUNC_SUBFE           136
#define PPC_FUNC_SUBFME          232
#define PPC_FUNC_SUBFZE          200

#define PPC_FUNC_SYNC            598

#define PPC_FUNC_TLBIA           370
#define PPC_FUNC_TLBRE           946
#define PPC_FUNC_TLBSX           914
#define PPC_FUNC_TLBSYNC         566
#define PPC_FUNC_TLBWE           978

#define PPC_FUNC_TW              4

#define PPC_FUNC_WRTEE           131
#define PPC_FUNC_WRTEEI          163

#define PPC_FUNC_XOR             316

// Registers
#define R0  0
#define R1  1
#define R2  2 
#define R3  3
#define R4  4
#define R5  5 
#define R6  6
#define R7  7
#define R8  8
#define R9  9
#define R10 10
#define R11 11
#define R12 12
#define R13 13
#define R14 14
#define R15 15
#define R16 16
#define R17 17
#define R18 18
#define R19 19
#define R20 20
#define R21 21
#define R22 22
#define R23 23
#define R24 24
#define R25 25
#define R26 26
#define R27 27
#define R28 28
#define R29 29
#define R30 30
#define R31 31

#endif
