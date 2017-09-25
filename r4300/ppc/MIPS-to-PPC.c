/**
 * Wii64 - MIPS-to-PPC.c
 * Copyright (C) 2007, 2008, 2009, 2010 Mike Slegeir
 * 
 * Convert MIPS code into PPC (take 2 1/2)
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

/* TODO: FP conversion to/from longs
         Optimize idle branches (generate a call to gen_interrupt)
         Optimize instruction scheduling & reduce branch instructions
   FIXME: Branch comparisons need to operate on 64-bit values when necessary
 */

#include <string.h>
#include "MIPS-to-PPC.h"
#include "Register-Cache.h"
#include "Interpreter.h"
#include "Wrappers.h"
#include "../Recomp-Cache.h"
#include <math.h>

#include <assert.h>

// Prototypes for functions used and defined in this file
static void genCallInterp(MIPS_instr);
#define JUMPTO_REG  0
#define JUMPTO_OFF  1
#define JUMPTO_ADDR 2
#define JUMPTO_REG_SIZE  2
#define JUMPTO_OFF_SIZE  11
#define JUMPTO_ADDR_SIZE 11
static void genJumpTo(unsigned int loc, unsigned int type);
static void genUpdateCount(int checkCount);
static void genCheckFP(void);
static void genCallDynaMem(memType type, int count, int _rs, int _rt, short immed);
void RecompCache_Update(PowerPC_func*);
static int inline mips_is_jump(MIPS_instr);
void jump_to(unsigned int);
void check_interupt();
extern int llbit;
extern long local_rs32, local_rt32;
extern unsigned long FCR0, FCR31;
extern unsigned long next_interupt, last_addr;

unsigned long long __udivmoddi4(unsigned long long, unsigned long long, unsigned long long*);
double __ieee754_sqrt(double);
float __ieee754_sqrtf(float);
long long __fixdfdi(double);
long long __fixsfdi(float);

#define CANT_COMPILE_DELAY() \
	((get_src_pc()&0xFFF) == 0xFFC && \
	 (get_src_pc() <  0x80000000 || \
	  get_src_pc() >= 0xC0000000))

static inline unsigned short extractUpper16(void* address){
	unsigned int addr = (unsigned int)address;
	return (addr>>16) + ((addr>>15)&1);
}

static inline short extractLower16(void* address){
	unsigned int addr = (unsigned int)address;
	return addr&0x8000 ? (addr&0xffff)-0x10000 : addr&0xffff;
}

static int FP_need_check;

// Variable to indicate whether the next recompiled instruction
//   is a delay slot (which needs to have its registers flushed)
//   and the current instruction
static int delaySlotNext, isDelaySlot;
// This should be called before the jump is recompiled
static inline int check_delaySlot(void){
	if(peek_next_src() == 0){ // MIPS uses 0 as a NOP
		get_next_src();   // Get rid of the NOP
		return 0;
	} else {
		if(mips_is_jump(peek_next_src())) return CONVERT_WARNING;
		delaySlotNext = 1;
		convert(); // This just moves the delay slot instruction ahead of the branch
		return 1;
	}
}

#define MIPS_REG_HI 32
#define MIPS_REG_LO 33

// Initialize register mappings
void start_new_block(void){
	invalidateRegisters();
	// Check if the previous instruction was a branch
	//   and thus whether this block begins with a delay slot
	unget_last_src();
	if(mips_is_jump(get_next_src())) delaySlotNext = 2;
	else delaySlotNext = 0;
}
void start_new_mapping(void){
	flushRegisters();
	FP_need_check = 1;
	reset_code_addr();
}

static inline int signExtend(int value, int size){
	int signMask = 1 << (size-1);
	int negMask = 0xffffffff << (size-1);
	if(value & signMask) value |= negMask;
	return value;
}

static void genCmp64(int cr, int _ra, int _rb){
	PowerPC_instr ppc;
	
	if(getRegisterMapping(_ra) == MAPPING_32 ||
	   getRegisterMapping(_rb) == MAPPING_32){
		// Here we cheat a little bit: if either of the registers are mapped
		// as 32-bit, only compare the 32-bit values
		int ra = mapRegister(_ra), rb = mapRegister(_rb);
		
		GEN_CMP(ppc, cr, ra, rb);
		set_next_dst(ppc);
	} else {
		RegMapping ra = mapRegister64(_ra), rb = mapRegister64(_rb);
		
		GEN_CMP(ppc, cr, ra.hi, rb.hi);
		set_next_dst(ppc);
		// Skip low word comparison if high words are mismatched
		GEN_BNE(ppc, cr, 2, 0, 0);
		set_next_dst(ppc);
		// Compare low words if hi words don't match
		GEN_CMPL(ppc, cr, ra.lo, rb.lo);
		set_next_dst(ppc);
	}
}

static void genCmpi64(int cr, int _ra, short immed){
	PowerPC_instr ppc;
	
	if(getRegisterMapping(_ra) == MAPPING_32){
		// If we've mapped this register as 32-bit, don't bother with 64-bit
		int ra = mapRegister(_ra);
		
		GEN_CMPI(ppc, cr, ra, immed);
		set_next_dst(ppc);
	} else {
		RegMapping ra = mapRegister64(_ra);
		
		GEN_CMPI(ppc, cr, ra.hi, (immed&0x8000) ? ~0 : 0);
		set_next_dst(ppc);
		// Skip low word comparison if high words are mismatched
		GEN_BNE(ppc, cr, 2, 0, 0);
		set_next_dst(ppc);
		// Compare low words if hi words don't match
		GEN_CMPLI(ppc, cr, ra.lo, immed);
		set_next_dst(ppc);
	}
}

typedef enum { NONE=0, EQ, NE, LT, GT, LE, GE } condition;
// Branch a certain offset (possibly conditionally, linking, or likely)
//   offset: N64 instructions from current N64 instruction to branch
//   cond: type of branch to execute depending on cr 7
//   link: if nonzero, branch and link
//   likely: if nonzero, the delay slot will only be executed when cond is true
static int branch(short offset, condition cond, int link, int likely){
	PowerPC_instr ppc;
	int likely_id;
	// Condition codes for bc (and their negations)
	int bo, bi, nbo;
	switch(cond){
		case EQ:
			bo = 0xc, nbo = 0x4, bi = 18;
			break;
		case NE:
			bo = 0x4, nbo = 0xc, bi = 18;
			break;
		case LT:
			bo = 0xc, nbo = 0x4, bi = 16;
			break;
		case GE:
			bo = 0x4, nbo = 0xc, bi = 16;
			break;
		case GT:
			bo = 0xc, nbo = 0x4, bi = 17;
			break;
		case LE:
			bo = 0x4, nbo = 0xc, bi = 17;
			break;
		default:
			bo = 0x14; nbo = 0x4; bi = 19;
			break;
	}

	flushRegisters();

	if(link){
		// Set LR to next instruction
		int lr = mapRegisterNew(MIPS_REG_LR, 1);
		// lis	lr, pc@ha(0)
		GEN_LIS(ppc, lr, (get_src_pc()+8)>>16);
		set_next_dst(ppc);
		// la	lr, pc@l(lr)
		GEN_ORI(ppc, lr, lr, get_src_pc()+8);
		set_next_dst(ppc);

		flushRegisters();
	}

	if(likely){
		// b[!cond] <past delay to update_count>
		likely_id = add_jump_special(0);
		GEN_BC(ppc, likely_id, 0, 0, nbo, bi);
		set_next_dst(ppc);
	}

	// Check the delay slot, and note how big it is
	PowerPC_instr* preDelay = get_curr_dst();
	check_delaySlot();
	int delaySlot = get_curr_dst() - preDelay;

#ifdef COMPARE_CORE
	GEN_LI(ppc, R4, 1);
	set_next_dst(ppc);
	if(likely){
		GEN_B(ppc, 2, 0, 0);
		set_next_dst(ppc);
		GEN_LI(ppc, R4, 0);
		set_next_dst(ppc);

		set_jump_special(likely_id, delaySlot+2+1);
	}
#else
	if(likely) set_jump_special(likely_id, delaySlot+1);
#endif

	genUpdateCount(1); // Sets cr3 to (next_interupt ? Count)

#ifndef INTERPRET_BRANCH
	// If we're jumping out, we need to trampoline using genJumpTo
	if(is_j_out(offset, 0)){
#endif // INTEPRET_BRANCH

		// b[!cond] <past jumpto & delay>
		//   Note: if there's a delay slot, I will branch to the branch over it
		GEN_BC(ppc, JUMPTO_OFF_SIZE+1, 0, 0, nbo, bi);
		set_next_dst(ppc);

		genJumpTo(offset, JUMPTO_OFF);

		// The branch isn't taken, but we need to check interrupts
		// Load the address of the next instruction
		GEN_LIS(ppc, R3, (get_src_pc()+4)>>16);
		set_next_dst(ppc);
		GEN_ORI(ppc, R3, R3, get_src_pc()+4);
		set_next_dst(ppc);
		// If taking the interrupt, return to the trampoline
		GEN_BLELR(ppc, CR3, 0);
		set_next_dst(ppc);

#ifndef INTERPRET_BRANCH
	} else {
		// last_addr = naddr
		if(cond != NONE){
			GEN_BC(ppc, 4, 0, 0, bo, bi);
			set_next_dst(ppc);
			GEN_LIS(ppc, R3, (get_src_pc()+4)>>16);
			set_next_dst(ppc);
			GEN_ORI(ppc, R3, R3, get_src_pc()+4);
			set_next_dst(ppc);
			GEN_B(ppc, 3, 0, 0);
			set_next_dst(ppc);
		}
		GEN_LIS(ppc, R3, (get_src_pc() + (offset<<2))>>16);
		set_next_dst(ppc);
		GEN_ORI(ppc, R3, R3, get_src_pc() + (offset<<2));
		set_next_dst(ppc);
		GEN_STW(ppc, R3, SDAREL(last_addr), R13);
		set_next_dst(ppc);

		// If taking the interrupt, return to the trampoline
		GEN_BLELR(ppc, CR3, 0);
		set_next_dst(ppc);

		// The actual branch
#if 0
		// FIXME: Reenable this when blocks are small enough to BC within
		//          Make sure that pass2 uses BD/LI as appropriate
		GEN_BC(ppc, add_jump(offset, 0, 0), 0, 0, bo, bi);
		set_next_dst(ppc);
#else
		GEN_BC(ppc, 2, 0, 0, nbo, bi);
		set_next_dst(ppc);
		GEN_B(ppc, add_jump(offset, 0, 0), 0, 0);
		set_next_dst(ppc);
#endif

	}
#endif // INTERPRET_BRANCH

	// Let's still recompile the delay slot in place in case its branched to
	// Unless the delay slot is in the next block, in which case there's nothing to skip
	//   Testing is_j_out with an offset of 0 checks whether the delay slot is out
	if(delaySlot){
		if(is_j_dst(0) && !is_j_out(0, 0)){
			// Step over the already executed delay slot if the branch isn't taken
			// b delaySlot+1
			GEN_B(ppc, delaySlot+1, 0, 0);
			set_next_dst(ppc);

			unget_last_src();
			delaySlotNext = 2;
		}
	} else nop_ignored();

#ifdef INTERPRET_BRANCH
	return INTERPRETED;
#else // INTERPRET_BRANCH
	return CONVERT_SUCCESS;
#endif
}


static int (*gen_ops[64])(MIPS_instr);

int convert(void){
	int needFlush = delaySlotNext;
	isDelaySlot = (delaySlotNext == 1);
	delaySlotNext = 0;

	MIPS_instr mips = get_next_src();
	int result = gen_ops[MIPS_GET_OPCODE(mips)](mips);

	if(needFlush) flushRegisters();
	return result;
}

static int NI(){
	return CONVERT_ERROR;
}

// -- Primary Opcodes --

static int J(MIPS_instr mips){
	PowerPC_instr  ppc;
	unsigned int naddr = (MIPS_GET_LI(mips)<<2)|((get_src_pc()+4)&0xf0000000);

	if(naddr == get_src_pc() || CANT_COMPILE_DELAY()){
		// J_IDLE || virtual delay
		genCallInterp(mips);
		return INTERPRETED;
	}

	flushRegisters();
	reset_code_addr();

	// Check the delay slot, and note how big it is
	PowerPC_instr* preDelay = get_curr_dst();
	check_delaySlot();
	int delaySlot = get_curr_dst() - preDelay;

#ifdef COMPARE_CORE
	GEN_LI(ppc, R4, 1);
	set_next_dst(ppc);
#endif
	// Sets cr3 to (next_interupt ? Count)
	genUpdateCount(1);

#ifdef INTERPRET_J
	genJumpTo(MIPS_GET_LI(mips), JUMPTO_ADDR);
#else // INTERPRET_J
	// If we're jumping out, we can't just use a branch instruction
	if(is_j_out(MIPS_GET_LI(mips), 1)){
		genJumpTo(MIPS_GET_LI(mips), JUMPTO_ADDR);
	} else {
		// last_addr = naddr
		GEN_LIS(ppc, R3, naddr>>16);
		set_next_dst(ppc);
		GEN_ORI(ppc, R3, R3, naddr);
		set_next_dst(ppc);
		GEN_STW(ppc, R3, SDAREL(last_addr), R13);
		set_next_dst(ppc);

		// if(next_interupt <= Count) return;
		GEN_BLELR(ppc, CR3, 0);
		set_next_dst(ppc);

		// Even though this is an absolute branch
		//   in pass 2, we generate a relative branch
		GEN_B(ppc, add_jump(MIPS_GET_LI(mips), 1, 0), 0, 0);
		set_next_dst(ppc);
	}
#endif

	// Let's still recompile the delay slot in place in case its branched to
	if(delaySlot){ if(is_j_dst(0)){ unget_last_src(); delaySlotNext = 2; } }
	else nop_ignored();

#ifdef INTERPRET_J
	return INTERPRETED;
#else // INTERPRET_J
	return CONVERT_SUCCESS;
#endif
}

static int JAL(MIPS_instr mips){
	PowerPC_instr  ppc;
	unsigned int naddr = (MIPS_GET_LI(mips)<<2)|((get_src_pc()+4)&0xf0000000);

	if(CANT_COMPILE_DELAY()){
		genCallInterp(mips);
		return INTERPRETED;
	}

	flushRegisters();
	reset_code_addr();

	// Check the delay slot, and note how big it is
	PowerPC_instr* preDelay = get_curr_dst();
	check_delaySlot();
	int delaySlot = get_curr_dst() - preDelay;

#ifdef COMPARE_CORE
	GEN_LI(ppc, R4, 1);
	set_next_dst(ppc);
#endif
	// Sets cr3 to (next_interupt ? Count)
	genUpdateCount(1);

	// Set LR to next instruction
	int lr = mapRegisterNew(MIPS_REG_LR, 1);
	// lis	lr, pc@ha(0)
	GEN_LIS(ppc, lr, (get_src_pc()+4)>>16);
	set_next_dst(ppc);
	// la	lr, pc@l(lr)
	GEN_ORI(ppc, lr, lr, get_src_pc()+4);
	set_next_dst(ppc);

	flushRegisters();

#ifdef INTERPRET_JAL
	genJumpTo(MIPS_GET_LI(mips), JUMPTO_ADDR);
#else // INTERPRET_JAL
	// If we're jumping out, we can't just use a branch instruction
	if(is_j_out(MIPS_GET_LI(mips), 1)){
		genJumpTo(MIPS_GET_LI(mips), JUMPTO_ADDR);
	} else {
		// last_addr = naddr
		GEN_LIS(ppc, R3, naddr>>16);
		set_next_dst(ppc);
		GEN_ORI(ppc, R3, R3, naddr);
		set_next_dst(ppc);
		GEN_STW(ppc, R3, SDAREL(last_addr), R13);
		set_next_dst(ppc);

		/// if(next_interupt <= Count) return;
		GEN_BLELR(ppc, CR3, 0);
		set_next_dst(ppc);

		// Even though this is an absolute branch
		//   in pass 2, we generate a relative branch
		GEN_B(ppc, add_jump(MIPS_GET_LI(mips), 1, 0), 0, 0);
		set_next_dst(ppc);
	}
#endif

	// Let's still recompile the delay slot in place in case its branched to
	if(delaySlot){ if(is_j_dst(0)){ unget_last_src(); delaySlotNext = 2; } }
	else nop_ignored();

#ifdef INTERPRET_JAL
	return INTERPRETED;
#else // INTERPRET_JAL
	return CONVERT_SUCCESS;
#endif
}

static int BEQ(MIPS_instr mips){
	PowerPC_instr  ppc;

	if((MIPS_GET_IMMED(mips) == 0xffff &&
	    MIPS_GET_RA(mips) == MIPS_GET_RB(mips)) ||
	   CANT_COMPILE_DELAY()){
		// BEQ_IDLE || virtual delay
		genCallInterp(mips);
		return INTERPRETED;
	}

	genCmp64(CR4, MIPS_GET_RA(mips), MIPS_GET_RB(mips));

	return branch(MIPS_GET_IMMED(mips), EQ, 0, 0);
}

static int BNE(MIPS_instr mips){
	PowerPC_instr  ppc;

	if(CANT_COMPILE_DELAY()){
		genCallInterp(mips);
		return INTERPRETED;
	}

	genCmp64(CR4, MIPS_GET_RA(mips), MIPS_GET_RB(mips));

	return branch(MIPS_GET_IMMED(mips), NE, 0, 0);
}

static int BLEZ(MIPS_instr mips){
	PowerPC_instr  ppc;

	if(CANT_COMPILE_DELAY()){
		genCallInterp(mips);
		return INTERPRETED;
	}

	genCmpi64(CR4, MIPS_GET_RA(mips), 0);

	return branch(MIPS_GET_IMMED(mips), LE, 0, 0);
}

static int BGTZ(MIPS_instr mips){
	PowerPC_instr  ppc;

	if(CANT_COMPILE_DELAY()){
		genCallInterp(mips);
		return INTERPRETED;
	}

	genCmpi64(CR4, MIPS_GET_RA(mips), 0);

	return branch(MIPS_GET_IMMED(mips), GT, 0, 0);
}

static int ADDIU(MIPS_instr mips){
	PowerPC_instr ppc;
	int _rs = MIPS_GET_RS(mips), _rt = MIPS_GET_RT(mips);
	short immed = MIPS_GET_IMMED(mips);

	int rs = mapRegister(_rs);
	int rt = mapConstantNew(_rt, isRegisterConstant(_rs));
	setRegisterConstant(_rt, getRegisterConstant(_rs) + immed);

	GEN_ADDI(ppc, rt, rs, immed);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
}

static int ADDI(MIPS_instr mips){
	return ADDIU(mips);
}

static int SLTI(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_SLTI
	genCallInterp(mips);
	return INTERPRETED;
#else
	// FIXME: Do I need to worry about 64-bit values?
	int rs = mapRegister( MIPS_GET_RS(mips) );
	int rt = mapRegisterNew( MIPS_GET_RT(mips), 0 );

	GEN_CMPI(ppc, CR0, rs, MIPS_GET_IMMED(mips));
	set_next_dst(ppc);
	GEN_MFCR(ppc, R2);
	set_next_dst(ppc);
	GEN_RLWINM(ppc, rt, R2, 1, 31, 31);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
#endif
}

static int SLTIU(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_SLTIU
	genCallInterp(mips);
	return INTERPRETED;
#else
	// FIXME: Do I need to worry about 64-bit values?
	int rs = mapRegister( MIPS_GET_RS(mips) );
	int rt = mapRegisterNew( MIPS_GET_RT(mips), 0 );

	GEN_NOT(ppc, R0, rs);
	set_next_dst(ppc);
	GEN_ADDIC(ppc, R0, R0, MIPS_GET_IMMED(mips));
	set_next_dst(ppc);
	GEN_ADDZE(ppc, rt, DYNAREG_ZERO);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
#endif
}

static int ANDI(MIPS_instr mips){
	PowerPC_instr ppc;
	int rs = mapRegister( MIPS_GET_RS(mips) );
	int rt = mapRegisterNew( MIPS_GET_RT(mips), 0 );

	GEN_ANDI(ppc, rt, rs, MIPS_GET_IMMED(mips));
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
}

static int ORI(MIPS_instr mips){
	PowerPC_instr ppc;
	RegMapping rs = mapRegister64( MIPS_GET_RS(mips) );
	RegMapping rt = mapRegister64New( MIPS_GET_RT(mips) );

	GEN_MR(ppc, rt.hi, rs.hi);
	set_next_dst(ppc);
	GEN_ORI(ppc, rt.lo, rs.lo, MIPS_GET_IMMED(mips));
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
}

static int XORI(MIPS_instr mips){
	PowerPC_instr ppc;
	RegMapping rs = mapRegister64( MIPS_GET_RS(mips) );
	RegMapping rt = mapRegister64New( MIPS_GET_RT(mips) );

	GEN_MR(ppc, rt.hi, rs.hi);
	set_next_dst(ppc);
	GEN_XORI(ppc, rt.lo, rs.lo, MIPS_GET_IMMED(mips));
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
}

static int LUI(MIPS_instr mips){
	PowerPC_instr ppc;
	int _rt = MIPS_GET_RT(mips);
	short immed = MIPS_GET_IMMED(mips);

	int rt = mapConstantNew(_rt, 1);
	setRegisterConstant(_rt, immed << 16);

	GEN_LIS(ppc, rt, immed);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
}

static int BEQL(MIPS_instr mips){
	PowerPC_instr  ppc;

	if(CANT_COMPILE_DELAY()){
		genCallInterp(mips);
		return INTERPRETED;
	}

	genCmp64(CR4, MIPS_GET_RA(mips), MIPS_GET_RB(mips));

	return branch(MIPS_GET_IMMED(mips), EQ, 0, 1);
}

static int BNEL(MIPS_instr mips){
	PowerPC_instr  ppc;

	if(CANT_COMPILE_DELAY()){
		genCallInterp(mips);
		return INTERPRETED;
	}

	genCmp64(CR4, MIPS_GET_RA(mips), MIPS_GET_RB(mips));

	return branch(MIPS_GET_IMMED(mips), NE, 0, 1);
}

static int BLEZL(MIPS_instr mips){
	PowerPC_instr  ppc;

	if(CANT_COMPILE_DELAY()){
		genCallInterp(mips);
		return INTERPRETED;
	}

	genCmpi64(CR4, MIPS_GET_RA(mips), 0);

	return branch(MIPS_GET_IMMED(mips), LE, 0, 1);
}

static int BGTZL(MIPS_instr mips){
	PowerPC_instr  ppc;

	if(CANT_COMPILE_DELAY()){
		genCallInterp(mips);
		return INTERPRETED;
	}

	genCmpi64(CR4, MIPS_GET_RA(mips), 0);

	return branch(MIPS_GET_IMMED(mips), GT, 0, 1);
}

static int DADDIU(MIPS_instr mips){
	PowerPC_instr ppc;
#if defined(INTERPRET_DW) || defined(INTERPRET_DADDIU)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_DW || INTERPRET_DADDIU

	RegMapping rs = mapRegister64( MIPS_GET_RS(mips) );
	RegMapping rt = mapRegister64New( MIPS_GET_RT(mips) );

	// Add the immediate to the LSW
	GEN_ADDIC(ppc, rt.lo, rs.lo, MIPS_GET_IMMED(mips));
	set_next_dst(ppc);
	// Add the MSW with the sign-extension and the carry
	if(MIPS_GET_IMMED(mips)&0x8000){
		GEN_ADDME(ppc, rt.hi, rs.hi);
		set_next_dst(ppc);
	} else {
		GEN_ADDZE(ppc, rt.hi, rs.hi);
		set_next_dst(ppc);
	}

	return CONVERT_SUCCESS;
#endif
}

static int DADDI(MIPS_instr mips){
	return DADDIU(mips);
}

static int LDL(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_LDL
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_LDL
	genCallDynaMem(MEM_LDL, 1, MIPS_GET_RS(mips), MIPS_GET_RT(mips), MIPS_GET_IMMED(mips));
	return CONVERT_SUCCESS;
#endif
}

static int LDR(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_LDR
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_LDR
	genCallDynaMem(MEM_LDR, 1, MIPS_GET_RS(mips), MIPS_GET_RT(mips), MIPS_GET_IMMED(mips));
	return CONVERT_SUCCESS;
#endif
}

static int LB(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_LB
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_LB
	int count = 1;
	int _rs = MIPS_GET_RS(mips), _rt = MIPS_GET_RT(mips);
	short immed = MIPS_GET_IMMED(mips);

#ifdef FASTMEM
	if(!isDelaySlot && !is_j_dst(1)){
		MIPS_instr peek = peek_next_src();
		if(MIPS_GET_OPCODE(peek) == MIPS_OPCODE_LB && MIPS_GET_RS(peek) == _rs && MIPS_GET_RS(peek) != _rt){
			if(MIPS_GET_RT(peek) == _rt + 1 && MIPS_GET_IMMED(peek) == immed + 1){
				mips = get_next_src();
				count++;
				while(has_next_src() && !is_j_dst(1)){
					peek = peek_next_src();
					if(MIPS_GET_OPCODE(peek) != MIPS_OPCODE_LB)
						break;
					if(MIPS_GET_RS(peek) != MIPS_GET_RS(mips))
						break;
					if(MIPS_GET_RS(peek) == MIPS_GET_RT(mips))
						break;
					if(MIPS_GET_RT(peek) != MIPS_GET_RT(mips) + 1)
						break;
					if(MIPS_GET_IMMED(peek) != MIPS_GET_IMMED(mips) + 1)
						break;
					mips = get_next_src();
					count++;
				}
			} else if(MIPS_GET_RT(peek) == _rt - 1 && MIPS_GET_IMMED(peek) == immed - 1){
				mips = get_next_src();
				count++;
				while(has_next_src() && !is_j_dst(1)){
					peek = peek_next_src();
					if(MIPS_GET_OPCODE(peek) != MIPS_OPCODE_LB)
						break;
					if(MIPS_GET_RS(peek) != MIPS_GET_RS(mips))
						break;
					if(MIPS_GET_RS(peek) == MIPS_GET_RT(mips))
						break;
					if(MIPS_GET_RT(peek) != MIPS_GET_RT(mips) - 1)
						break;
					if(MIPS_GET_IMMED(peek) != MIPS_GET_IMMED(mips) - 1)
						break;
					mips = get_next_src();
					count++;
				}
				_rt = MIPS_GET_RT(mips);
				immed = MIPS_GET_IMMED(mips);
			}
		}
	}
#endif

	genCallDynaMem(MEM_LB, count, _rs, _rt, immed);
	return CONVERT_SUCCESS;
#endif
}

static int LH(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_LH
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_LH
	int count = 1;
	int _rs = MIPS_GET_RS(mips), _rt = MIPS_GET_RT(mips);
	short immed = MIPS_GET_IMMED(mips);

#ifdef FASTMEM
	if(!isDelaySlot && !is_j_dst(1)){
		MIPS_instr peek = peek_next_src();
		if(MIPS_GET_OPCODE(peek) == MIPS_OPCODE_LH && MIPS_GET_RS(peek) == _rs && MIPS_GET_RS(peek) != _rt){
			if(MIPS_GET_RT(peek) == _rt + 1 && MIPS_GET_IMMED(peek) == immed + 2){
				mips = get_next_src();
				count++;
				while(has_next_src() && !is_j_dst(1)){
					peek = peek_next_src();
					if(MIPS_GET_OPCODE(peek) != MIPS_OPCODE_LH)
						break;
					if(MIPS_GET_RS(peek) != MIPS_GET_RS(mips))
						break;
					if(MIPS_GET_RS(peek) == MIPS_GET_RT(mips))
						break;
					if(MIPS_GET_RT(peek) != MIPS_GET_RT(mips) + 1)
						break;
					if(MIPS_GET_IMMED(peek) != MIPS_GET_IMMED(mips) + 2)
						break;
					mips = get_next_src();
					count++;
				}
			} else if(MIPS_GET_RT(peek) == _rt - 1 && MIPS_GET_IMMED(peek) == immed - 2){
				mips = get_next_src();
				count++;
				while(has_next_src() && !is_j_dst(1)){
					peek = peek_next_src();
					if(MIPS_GET_OPCODE(peek) != MIPS_OPCODE_LH)
						break;
					if(MIPS_GET_RS(peek) != MIPS_GET_RS(mips))
						break;
					if(MIPS_GET_RS(peek) == MIPS_GET_RT(mips))
						break;
					if(MIPS_GET_RT(peek) != MIPS_GET_RT(mips) - 1)
						break;
					if(MIPS_GET_IMMED(peek) != MIPS_GET_IMMED(mips) - 2)
						break;
					mips = get_next_src();
					count++;
				}
				_rt = MIPS_GET_RT(mips);
				immed = MIPS_GET_IMMED(mips);
			}
		}
	}
#endif

	genCallDynaMem(MEM_LH, count, _rs, _rt, immed);
	return CONVERT_SUCCESS;
#endif
}

static int LWL(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_LWL
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_LWL
	genCallDynaMem(MEM_LWL, 1, MIPS_GET_RS(mips), MIPS_GET_RT(mips), MIPS_GET_IMMED(mips));
	return CONVERT_SUCCESS;
#endif
}

static int LW(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_LW
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_LW
	int count = 1;
	int _rs = MIPS_GET_RS(mips), _rt = MIPS_GET_RT(mips);
	short immed = MIPS_GET_IMMED(mips);

#ifdef FASTMEM
	if(!isDelaySlot && !is_j_dst(1)){
		MIPS_instr peek = peek_next_src();
		if(MIPS_GET_OPCODE(peek) == MIPS_OPCODE_LW && MIPS_GET_RS(peek) == _rs && MIPS_GET_RS(peek) != _rt){
			if(MIPS_GET_RT(peek) == _rt + 1 && MIPS_GET_IMMED(peek) == immed + 4){
				mips = get_next_src();
				count++;
				while(has_next_src() && !is_j_dst(1)){
					peek = peek_next_src();
					if(MIPS_GET_OPCODE(peek) != MIPS_OPCODE_LW)
						break;
					if(MIPS_GET_RS(peek) != MIPS_GET_RS(mips))
						break;
					if(MIPS_GET_RS(peek) == MIPS_GET_RT(mips))
						break;
					if(MIPS_GET_RT(peek) != MIPS_GET_RT(mips) + 1)
						break;
					if(MIPS_GET_IMMED(peek) != MIPS_GET_IMMED(mips) + 4)
						break;
					mips = get_next_src();
					count++;
				}
			} else if(MIPS_GET_RT(peek) == _rt - 1 && MIPS_GET_IMMED(peek) == immed - 4){
				mips = get_next_src();
				count++;
				while(has_next_src() && !is_j_dst(1)){
					peek = peek_next_src();
					if(MIPS_GET_OPCODE(peek) != MIPS_OPCODE_LW)
						break;
					if(MIPS_GET_RS(peek) != MIPS_GET_RS(mips))
						break;
					if(MIPS_GET_RS(peek) == MIPS_GET_RT(mips))
						break;
					if(MIPS_GET_RT(peek) != MIPS_GET_RT(mips) - 1)
						break;
					if(MIPS_GET_IMMED(peek) != MIPS_GET_IMMED(mips) - 4)
						break;
					mips = get_next_src();
					count++;
				}
				_rt = MIPS_GET_RT(mips);
				immed = MIPS_GET_IMMED(mips);
			}
		}
	}
#endif

	genCallDynaMem(MEM_LW, count, _rs, _rt, immed);
	return CONVERT_SUCCESS;
#endif
}

static int LBU(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_LBU
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_LBU
	int count = 1;
	int _rs = MIPS_GET_RS(mips), _rt = MIPS_GET_RT(mips);
	short immed = MIPS_GET_IMMED(mips);

#ifdef FASTMEM
	if(!isDelaySlot && !is_j_dst(1)){
		MIPS_instr peek = peek_next_src();
		if(MIPS_GET_OPCODE(peek) == MIPS_OPCODE_LBU && MIPS_GET_RS(peek) == _rs && MIPS_GET_RS(peek) != _rt){
			if(MIPS_GET_RT(peek) == _rt + 1 && MIPS_GET_IMMED(peek) == immed + 1){
				mips = get_next_src();
				count++;
				while(has_next_src() && !is_j_dst(1)){
					peek = peek_next_src();
					if(MIPS_GET_OPCODE(peek) != MIPS_OPCODE_LBU)
						break;
					if(MIPS_GET_RS(peek) != MIPS_GET_RS(mips))
						break;
					if(MIPS_GET_RS(peek) == MIPS_GET_RT(mips))
						break;
					if(MIPS_GET_RT(peek) != MIPS_GET_RT(mips) + 1)
						break;
					if(MIPS_GET_IMMED(peek) != MIPS_GET_IMMED(mips) + 1)
						break;
					mips = get_next_src();
					count++;
				}
			} else if(MIPS_GET_RT(peek) == _rt - 1 && MIPS_GET_IMMED(peek) == immed - 1){
				mips = get_next_src();
				count++;
				while(has_next_src() && !is_j_dst(1)){
					peek = peek_next_src();
					if(MIPS_GET_OPCODE(peek) != MIPS_OPCODE_LBU)
						break;
					if(MIPS_GET_RS(peek) != MIPS_GET_RS(mips))
						break;
					if(MIPS_GET_RS(peek) == MIPS_GET_RT(mips))
						break;
					if(MIPS_GET_RT(peek) != MIPS_GET_RT(mips) - 1)
						break;
					if(MIPS_GET_IMMED(peek) != MIPS_GET_IMMED(mips) - 1)
						break;
					mips = get_next_src();
					count++;
				}
				_rt = MIPS_GET_RT(mips);
				immed = MIPS_GET_IMMED(mips);
			}
		}
	}
#endif

	genCallDynaMem(MEM_LBU, count, _rs, _rt, immed);
	return CONVERT_SUCCESS;
#endif
}

static int LHU(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_LHU
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_LHU
	int count = 1;
	int _rs = MIPS_GET_RS(mips), _rt = MIPS_GET_RT(mips);
	short immed = MIPS_GET_IMMED(mips);

#ifdef FASTMEM
	if(!isDelaySlot && !is_j_dst(1)){
		MIPS_instr peek = peek_next_src();
		if(MIPS_GET_OPCODE(peek) == MIPS_OPCODE_LHU && MIPS_GET_RS(peek) == _rs && MIPS_GET_RS(peek) != _rt){
			if(MIPS_GET_RT(peek) == _rt + 1 && MIPS_GET_IMMED(peek) == immed + 2){
				mips = get_next_src();
				count++;
				while(has_next_src() && !is_j_dst(1)){
					peek = peek_next_src();
					if(MIPS_GET_OPCODE(peek) != MIPS_OPCODE_LHU)
						break;
					if(MIPS_GET_RS(peek) != MIPS_GET_RS(mips))
						break;
					if(MIPS_GET_RS(peek) == MIPS_GET_RT(mips))
						break;
					if(MIPS_GET_RT(peek) != MIPS_GET_RT(mips) + 1)
						break;
					if(MIPS_GET_IMMED(peek) != MIPS_GET_IMMED(mips) + 2)
						break;
					mips = get_next_src();
					count++;
				}
			} else if(MIPS_GET_RT(peek) == _rt - 1 && MIPS_GET_IMMED(peek) == immed - 2){
				mips = get_next_src();
				count++;
				while(has_next_src() && !is_j_dst(1)){
					peek = peek_next_src();
					if(MIPS_GET_OPCODE(peek) != MIPS_OPCODE_LHU)
						break;
					if(MIPS_GET_RS(peek) != MIPS_GET_RS(mips))
						break;
					if(MIPS_GET_RS(peek) == MIPS_GET_RT(mips))
						break;
					if(MIPS_GET_RT(peek) != MIPS_GET_RT(mips) - 1)
						break;
					if(MIPS_GET_IMMED(peek) != MIPS_GET_IMMED(mips) - 2)
						break;
					mips = get_next_src();
					count++;
				}
				_rt = MIPS_GET_RT(mips);
				immed = MIPS_GET_IMMED(mips);
			}
		}
	}
#endif

	genCallDynaMem(MEM_LHU, count, _rs, _rt, immed);
	return CONVERT_SUCCESS;
#endif
}

static int LWR(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_LWR
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_LWR
	genCallDynaMem(MEM_LWR, 1, MIPS_GET_RS(mips), MIPS_GET_RT(mips), MIPS_GET_IMMED(mips));
	return CONVERT_SUCCESS;
#endif
}

static int LWU(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_LWU
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_LWU
	int count = 1;
	int _rs = MIPS_GET_RS(mips), _rt = MIPS_GET_RT(mips);
	short immed = MIPS_GET_IMMED(mips);

#ifdef FASTMEM
	if(!isDelaySlot && !is_j_dst(1)){
		MIPS_instr peek = peek_next_src();
		if(MIPS_GET_OPCODE(peek) == MIPS_OPCODE_LWU && MIPS_GET_RS(peek) == _rs && MIPS_GET_RS(peek) != _rt){
			if(MIPS_GET_RT(peek) == _rt + 1 && MIPS_GET_IMMED(peek) == immed + 4){
				mips = get_next_src();
				count++;
				while(has_next_src() && !is_j_dst(1)){
					peek = peek_next_src();
					if(MIPS_GET_OPCODE(peek) != MIPS_OPCODE_LWU)
						break;
					if(MIPS_GET_RS(peek) != MIPS_GET_RS(mips))
						break;
					if(MIPS_GET_RS(peek) == MIPS_GET_RT(mips))
						break;
					if(MIPS_GET_RT(peek) != MIPS_GET_RT(mips) + 1)
						break;
					if(MIPS_GET_IMMED(peek) != MIPS_GET_IMMED(mips) + 4)
						break;
					mips = get_next_src();
					count++;
				}
			} else if(MIPS_GET_RT(peek) == _rt - 1 && MIPS_GET_IMMED(peek) == immed - 4){
				mips = get_next_src();
				count++;
				while(has_next_src() && !is_j_dst(1)){
					peek = peek_next_src();
					if(MIPS_GET_OPCODE(peek) != MIPS_OPCODE_LWU)
						break;
					if(MIPS_GET_RS(peek) != MIPS_GET_RS(mips))
						break;
					if(MIPS_GET_RS(peek) == MIPS_GET_RT(mips))
						break;
					if(MIPS_GET_RT(peek) != MIPS_GET_RT(mips) - 1)
						break;
					if(MIPS_GET_IMMED(peek) != MIPS_GET_IMMED(mips) - 4)
						break;
					mips = get_next_src();
					count++;
				}
				_rt = MIPS_GET_RT(mips);
				immed = MIPS_GET_IMMED(mips);
			}
		}
	}
#endif

	genCallDynaMem(MEM_LWU, count, _rs, _rt, immed);
	return CONVERT_SUCCESS;
#endif
}

static int SB(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_SB
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_SB
	int count = 1;
	int _rs = MIPS_GET_RS(mips), _rt = MIPS_GET_RT(mips);
	short immed = MIPS_GET_IMMED(mips);

#ifdef FASTMEM
	if(!isDelaySlot && !is_j_dst(1)){
		MIPS_instr peek = peek_next_src();
		if(MIPS_GET_OPCODE(peek) == MIPS_OPCODE_SB && MIPS_GET_RS(peek) == _rs){
			if(MIPS_GET_RT(peek) == _rt + 1 && MIPS_GET_IMMED(peek) == immed + 1){
				mips = get_next_src();
				count++;
				while(has_next_src() && !is_j_dst(1)){
					peek = peek_next_src();
					if(MIPS_GET_OPCODE(peek) != MIPS_OPCODE_SB)
						break;
					if(MIPS_GET_RS(peek) != MIPS_GET_RS(mips))
						break;
					if(MIPS_GET_RT(peek) != MIPS_GET_RT(mips) + 1)
						break;
					if(MIPS_GET_IMMED(peek) != MIPS_GET_IMMED(mips) + 1)
						break;
					mips = get_next_src();
					count++;
				}
			} else if(MIPS_GET_RT(peek) == _rt - 1 && MIPS_GET_IMMED(peek) == immed - 1){
				mips = get_next_src();
				count++;
				while(has_next_src() && !is_j_dst(1)){
					peek = peek_next_src();
					if(MIPS_GET_OPCODE(peek) != MIPS_OPCODE_SB)
						break;
					if(MIPS_GET_RS(peek) != MIPS_GET_RS(mips))
						break;
					if(MIPS_GET_RT(peek) != MIPS_GET_RT(mips) - 1)
						break;
					if(MIPS_GET_IMMED(peek) != MIPS_GET_IMMED(mips) - 1)
						break;
					mips = get_next_src();
					count++;
				}
				_rt = MIPS_GET_RT(mips);
				immed = MIPS_GET_IMMED(mips);
			}
		}
	}
#endif

	genCallDynaMem(MEM_SB, count, _rs, _rt, immed);
	return CONVERT_SUCCESS;
#endif
}

static int SH(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_SH
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_SH
	int count = 1;
	int _rs = MIPS_GET_RS(mips), _rt = MIPS_GET_RT(mips);
	short immed = MIPS_GET_IMMED(mips);

#ifdef FASTMEM
	if(!isDelaySlot && !is_j_dst(1)){
		MIPS_instr peek = peek_next_src();
		if(MIPS_GET_OPCODE(peek) == MIPS_OPCODE_SH && MIPS_GET_RS(peek) == _rs){
			if(MIPS_GET_RT(peek) == _rt + 1 && MIPS_GET_IMMED(peek) == immed + 2){
				mips = get_next_src();
				count++;
				while(has_next_src() && !is_j_dst(1)){
					peek = peek_next_src();
					if(MIPS_GET_OPCODE(peek) != MIPS_OPCODE_SH)
						break;
					if(MIPS_GET_RS(peek) != MIPS_GET_RS(mips))
						break;
					if(MIPS_GET_RT(peek) != MIPS_GET_RT(mips) + 1)
						break;
					if(MIPS_GET_IMMED(peek) != MIPS_GET_IMMED(mips) + 2)
						break;
					mips = get_next_src();
					count++;
				}
			} else if(MIPS_GET_RT(peek) == _rt - 1 && MIPS_GET_IMMED(peek) == immed - 2){
				mips = get_next_src();
				count++;
				while(has_next_src() && !is_j_dst(1)){
					peek = peek_next_src();
					if(MIPS_GET_OPCODE(peek) != MIPS_OPCODE_SH)
						break;
					if(MIPS_GET_RS(peek) != MIPS_GET_RS(mips))
						break;
					if(MIPS_GET_RT(peek) != MIPS_GET_RT(mips) - 1)
						break;
					if(MIPS_GET_IMMED(peek) != MIPS_GET_IMMED(mips) - 2)
						break;
					mips = get_next_src();
					count++;
				}
				_rt = MIPS_GET_RT(mips);
				immed = MIPS_GET_IMMED(mips);
			}
		}
	}
#endif

	genCallDynaMem(MEM_SH, count, _rs, _rt, immed);
	return CONVERT_SUCCESS;
#endif
}

static int SWL(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_SWL
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_SWL
	genCallDynaMem(MEM_SWL, 1, MIPS_GET_RS(mips), MIPS_GET_RT(mips), MIPS_GET_IMMED(mips));
	return CONVERT_SUCCESS;
#endif
}

static int SW(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_SW
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_SW
	int count = 1;
	int _rs = MIPS_GET_RS(mips), _rt = MIPS_GET_RT(mips);
	short immed = MIPS_GET_IMMED(mips);

#ifdef FASTMEM
	if(!isDelaySlot && !is_j_dst(1)){
		MIPS_instr peek = peek_next_src();
		if(MIPS_GET_OPCODE(peek) == MIPS_OPCODE_SW && MIPS_GET_RS(peek) == _rs){
			if(MIPS_GET_RT(peek) == _rt + 1 && MIPS_GET_IMMED(peek) == immed + 4){
				mips = get_next_src();
				count++;
				while(has_next_src() && !is_j_dst(1)){
					peek = peek_next_src();
					if(MIPS_GET_OPCODE(peek) != MIPS_OPCODE_SW)
						break;
					if(MIPS_GET_RS(peek) != MIPS_GET_RS(mips))
						break;
					if(MIPS_GET_RT(peek) != MIPS_GET_RT(mips) + 1)
						break;
					if(MIPS_GET_IMMED(peek) != MIPS_GET_IMMED(mips) + 4)
						break;
					mips = get_next_src();
					count++;
				}
			} else if(MIPS_GET_RT(peek) == _rt - 1 && MIPS_GET_IMMED(peek) == immed - 4){
				mips = get_next_src();
				count++;
				while(has_next_src() && !is_j_dst(1)){
					peek = peek_next_src();
					if(MIPS_GET_OPCODE(peek) != MIPS_OPCODE_SW)
						break;
					if(MIPS_GET_RS(peek) != MIPS_GET_RS(mips))
						break;
					if(MIPS_GET_RT(peek) != MIPS_GET_RT(mips) - 1)
						break;
					if(MIPS_GET_IMMED(peek) != MIPS_GET_IMMED(mips) - 4)
						break;
					mips = get_next_src();
					count++;
				}
				_rt = MIPS_GET_RT(mips);
				immed = MIPS_GET_IMMED(mips);
			}
		}
	}
#endif

	genCallDynaMem(MEM_SW, count, _rs, _rt, immed);
	return CONVERT_SUCCESS;
#endif
}

static int SDL(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_SDL
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_SDL
	genCallDynaMem(MEM_SDL, 1, MIPS_GET_RS(mips), MIPS_GET_RT(mips), MIPS_GET_IMMED(mips));
	return CONVERT_SUCCESS;
#endif
}

static int SDR(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_SDR
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_SDR
	genCallDynaMem(MEM_SDR, 1, MIPS_GET_RS(mips), MIPS_GET_RT(mips), MIPS_GET_IMMED(mips));
	return CONVERT_SUCCESS;
#endif
}

static int SWR(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_SWR
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_SWR
	genCallDynaMem(MEM_SWR, 1, MIPS_GET_RS(mips), MIPS_GET_RT(mips), MIPS_GET_IMMED(mips));
	return CONVERT_SUCCESS;
#endif
}

static int LD(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_LD
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_LD
	int count = 1;
	int _rs = MIPS_GET_RS(mips), _rt = MIPS_GET_RT(mips);
	short immed = MIPS_GET_IMMED(mips);

#ifdef FASTMEM
	if(!isDelaySlot && !is_j_dst(1)){
		MIPS_instr peek = peek_next_src();
		if(MIPS_GET_OPCODE(peek) == MIPS_OPCODE_LD && MIPS_GET_RS(peek) == _rs && MIPS_GET_RS(peek) != _rt){
			if(MIPS_GET_RT(peek) == _rt + 1 && MIPS_GET_IMMED(peek) == immed + 8){
				mips = get_next_src();
				count++;
				while(has_next_src() && !is_j_dst(1)){
					peek = peek_next_src();
					if(MIPS_GET_OPCODE(peek) != MIPS_OPCODE_LD)
						break;
					if(MIPS_GET_RS(peek) != MIPS_GET_RS(mips))
						break;
					if(MIPS_GET_RS(peek) == MIPS_GET_RT(mips))
						break;
					if(MIPS_GET_RT(peek) != MIPS_GET_RT(mips) + 1)
						break;
					if(MIPS_GET_IMMED(peek) != MIPS_GET_IMMED(mips) + 8)
						break;
					mips = get_next_src();
					count++;
				}
			} else if(MIPS_GET_RT(peek) == _rt - 1 && MIPS_GET_IMMED(peek) == immed - 8){
				mips = get_next_src();
				count++;
				while(has_next_src() && !is_j_dst(1)){
					peek = peek_next_src();
					if(MIPS_GET_OPCODE(peek) != MIPS_OPCODE_LD)
						break;
					if(MIPS_GET_RS(peek) != MIPS_GET_RS(mips))
						break;
					if(MIPS_GET_RS(peek) == MIPS_GET_RT(mips))
						break;
					if(MIPS_GET_RT(peek) != MIPS_GET_RT(mips) - 1)
						break;
					if(MIPS_GET_IMMED(peek) != MIPS_GET_IMMED(mips) - 8)
						break;
					mips = get_next_src();
					count++;
				}
				_rt = MIPS_GET_RT(mips);
				immed = MIPS_GET_IMMED(mips);
			}
		}
	}
#endif

	genCallDynaMem(MEM_LD, count, _rs, _rt, immed);
	return CONVERT_SUCCESS;
#endif
}

static int SD(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_SD
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_SD
	int count = 1;
	int _rs = MIPS_GET_RS(mips), _rt = MIPS_GET_RT(mips);
	short immed = MIPS_GET_IMMED(mips);

#ifdef FASTMEM
	if(!isDelaySlot && !is_j_dst(1)){
		MIPS_instr peek = peek_next_src();
		if(MIPS_GET_OPCODE(peek) == MIPS_OPCODE_SD && MIPS_GET_RS(peek) == _rs){
			if(MIPS_GET_RT(peek) == _rt + 1 && MIPS_GET_IMMED(peek) == immed + 8){
				mips = get_next_src();
				count++;
				while(has_next_src() && !is_j_dst(1)){
					peek = peek_next_src();
					if(MIPS_GET_OPCODE(peek) != MIPS_OPCODE_SD)
						break;
					if(MIPS_GET_RS(peek) != MIPS_GET_RS(mips))
						break;
					if(MIPS_GET_RT(peek) != MIPS_GET_RT(mips) + 1)
						break;
					if(MIPS_GET_IMMED(peek) != MIPS_GET_IMMED(mips) + 8)
						break;
					mips = get_next_src();
					count++;
				}
			} else if(MIPS_GET_RT(peek) == _rt - 1 && MIPS_GET_IMMED(peek) == immed - 8){
				mips = get_next_src();
				count++;
				while(has_next_src() && !is_j_dst(1)){
					peek = peek_next_src();
					if(MIPS_GET_OPCODE(peek) != MIPS_OPCODE_SD)
						break;
					if(MIPS_GET_RS(peek) != MIPS_GET_RS(mips))
						break;
					if(MIPS_GET_RT(peek) != MIPS_GET_RT(mips) - 1)
						break;
					if(MIPS_GET_IMMED(peek) != MIPS_GET_IMMED(mips) - 8)
						break;
					mips = get_next_src();
					count++;
				}
				_rt = MIPS_GET_RT(mips);
				immed = MIPS_GET_IMMED(mips);
			}
		}
	}
#endif

	genCallDynaMem(MEM_SD, count, _rs, _rt, immed);
	return CONVERT_SUCCESS;
#endif
}

static int LWC1(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_LWC1
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_LWC1
	int count = 1;
	int _rs = MIPS_GET_RS(mips), _rt = MIPS_GET_RT(mips);
	short immed = MIPS_GET_IMMED(mips);

#ifdef FASTMEM
	if(!isDelaySlot && !is_j_dst(1)){
		MIPS_instr peek = peek_next_src();
		if(MIPS_GET_OPCODE(peek) == MIPS_OPCODE_LWC1 && MIPS_GET_RS(peek) == _rs){
			if(MIPS_GET_RT(peek) == _rt + 2 && MIPS_GET_IMMED(peek) == immed + 4){
				mips = get_next_src();
				count++;
				while(has_next_src() && !is_j_dst(1)){
					peek = peek_next_src();
					if(MIPS_GET_OPCODE(peek) != MIPS_OPCODE_LWC1)
						break;
					if(MIPS_GET_RS(peek) != MIPS_GET_RS(mips))
						break;
					if(MIPS_GET_RT(peek) != MIPS_GET_RT(mips) + 2)
						break;
					if(MIPS_GET_IMMED(peek) != MIPS_GET_IMMED(mips) + 4)
						break;
					mips = get_next_src();
					count++;
				}
			} else if(MIPS_GET_RT(peek) == _rt - 2 && MIPS_GET_IMMED(peek) == immed - 4){
				mips = get_next_src();
				count++;
				while(has_next_src() && !is_j_dst(1)){
					peek = peek_next_src();
					if(MIPS_GET_OPCODE(peek) != MIPS_OPCODE_LWC1)
						break;
					if(MIPS_GET_RS(peek) != MIPS_GET_RS(mips))
						break;
					if(MIPS_GET_RT(peek) != MIPS_GET_RT(mips) - 2)
						break;
					if(MIPS_GET_IMMED(peek) != MIPS_GET_IMMED(mips) - 4)
						break;
					mips = get_next_src();
					count++;
				}
				_rt = MIPS_GET_RT(mips);
				immed = MIPS_GET_IMMED(mips);
			}
		}
	}
#endif

	genCallDynaMem(MEM_LWC1, count, _rs, _rt, immed);
	return CONVERT_SUCCESS;
#endif
}

static int LDC1(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_LDC1
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_LDC1
	int count = 1;
	int _rs = MIPS_GET_RS(mips), _rt = MIPS_GET_RT(mips);
	short immed = MIPS_GET_IMMED(mips);

#ifdef FASTMEM
	if(!isDelaySlot && !is_j_dst(1)){
		MIPS_instr peek = peek_next_src();
		if(MIPS_GET_OPCODE(peek) == MIPS_OPCODE_LDC1 && MIPS_GET_RS(peek) == _rs){
			if(MIPS_GET_RT(peek) == _rt + 2 && MIPS_GET_IMMED(peek) == immed + 8){
				mips = get_next_src();
				count++;
				while(has_next_src() && !is_j_dst(1)){
					peek = peek_next_src();
					if(MIPS_GET_OPCODE(peek) != MIPS_OPCODE_LDC1)
						break;
					if(MIPS_GET_RS(peek) != MIPS_GET_RS(mips))
						break;
					if(MIPS_GET_RT(peek) != MIPS_GET_RT(mips) + 2)
						break;
					if(MIPS_GET_IMMED(peek) != MIPS_GET_IMMED(mips) + 8)
						break;
					mips = get_next_src();
					count++;
				}
			} else if(MIPS_GET_RT(peek) == _rt - 2 && MIPS_GET_IMMED(peek) == immed - 8){
				mips = get_next_src();
				count++;
				while(has_next_src() && !is_j_dst(1)){
					peek = peek_next_src();
					if(MIPS_GET_OPCODE(peek) != MIPS_OPCODE_LDC1)
						break;
					if(MIPS_GET_RS(peek) != MIPS_GET_RS(mips))
						break;
					if(MIPS_GET_RT(peek) != MIPS_GET_RT(mips) - 2)
						break;
					if(MIPS_GET_IMMED(peek) != MIPS_GET_IMMED(mips) - 8)
						break;
					mips = get_next_src();
					count++;
				}
				_rt = MIPS_GET_RT(mips);
				immed = MIPS_GET_IMMED(mips);
			}
		}
	}
#endif

	genCallDynaMem(MEM_LDC1, count, _rs, _rt, immed);
	return CONVERT_SUCCESS;
#endif
}

static int SWC1(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_SWC1
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_SWC1
	int count = 1;
	int _rs = MIPS_GET_RS(mips), _rt = MIPS_GET_RT(mips);
	short immed = MIPS_GET_IMMED(mips);

#ifdef FASTMEM
	if(!isDelaySlot && !is_j_dst(1)){
		MIPS_instr peek = peek_next_src();
		if(MIPS_GET_OPCODE(peek) == MIPS_OPCODE_SWC1 && MIPS_GET_RS(peek) == _rs){
			if(MIPS_GET_RT(peek) == _rt + 2 && MIPS_GET_IMMED(peek) == immed + 4){
				mips = get_next_src();
				count++;
				while(has_next_src() && !is_j_dst(1)){
					peek = peek_next_src();
					if(MIPS_GET_OPCODE(peek) != MIPS_OPCODE_SWC1)
						break;
					if(MIPS_GET_RS(peek) != MIPS_GET_RS(mips))
						break;
					if(MIPS_GET_RT(peek) != MIPS_GET_RT(mips) + 2)
						break;
					if(MIPS_GET_IMMED(peek) != MIPS_GET_IMMED(mips) + 4)
						break;
					mips = get_next_src();
					count++;
				}
			} else if(MIPS_GET_RT(peek) == _rt - 2 && MIPS_GET_IMMED(peek) == immed - 4){
				mips = get_next_src();
				count++;
				while(has_next_src() && !is_j_dst(1)){
					peek = peek_next_src();
					if(MIPS_GET_OPCODE(peek) != MIPS_OPCODE_SWC1)
						break;
					if(MIPS_GET_RS(peek) != MIPS_GET_RS(mips))
						break;
					if(MIPS_GET_RT(peek) != MIPS_GET_RT(mips) - 2)
						break;
					if(MIPS_GET_IMMED(peek) != MIPS_GET_IMMED(mips) - 4)
						break;
					mips = get_next_src();
					count++;
				}
				_rt = MIPS_GET_RT(mips);
				immed = MIPS_GET_IMMED(mips);
			}
		}
	}
#endif

	genCallDynaMem(MEM_SWC1, count, _rs, _rt, immed);
	return CONVERT_SUCCESS;
#endif
}

static int SDC1(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_SDC1
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_SDC1
	int count = 1;
	int _rs = MIPS_GET_RS(mips), _rt = MIPS_GET_RT(mips);
	short immed = MIPS_GET_IMMED(mips);

#ifdef FASTMEM
	if(!isDelaySlot && !is_j_dst(1)){
		MIPS_instr peek = peek_next_src();
		if(MIPS_GET_OPCODE(peek) == MIPS_OPCODE_SDC1 && MIPS_GET_RS(peek) == _rs){
			if(MIPS_GET_RT(peek) == _rt + 2 && MIPS_GET_IMMED(peek) == immed + 8){
				mips = get_next_src();
				count++;
				while(has_next_src() && !is_j_dst(1)){
					peek = peek_next_src();
					if(MIPS_GET_OPCODE(peek) != MIPS_OPCODE_SDC1)
						break;
					if(MIPS_GET_RS(peek) != MIPS_GET_RS(mips))
						break;
					if(MIPS_GET_RT(peek) != MIPS_GET_RT(mips) + 2)
						break;
					if(MIPS_GET_IMMED(peek) != MIPS_GET_IMMED(mips) + 8)
						break;
					mips = get_next_src();
					count++;
				}
			} else if(MIPS_GET_RT(peek) == _rt - 2 && MIPS_GET_IMMED(peek) == immed - 8){
				mips = get_next_src();
				count++;
				while(has_next_src() && !is_j_dst(1)){
					peek = peek_next_src();
					if(MIPS_GET_OPCODE(peek) != MIPS_OPCODE_SDC1)
						break;
					if(MIPS_GET_RS(peek) != MIPS_GET_RS(mips))
						break;
					if(MIPS_GET_RT(peek) != MIPS_GET_RT(mips) - 2)
						break;
					if(MIPS_GET_IMMED(peek) != MIPS_GET_IMMED(mips) - 8)
						break;
					mips = get_next_src();
					count++;
				}
				_rt = MIPS_GET_RT(mips);
				immed = MIPS_GET_IMMED(mips);
			}
		}
	}
#endif

	genCallDynaMem(MEM_SDC1, count, _rs, _rt, immed);
	return CONVERT_SUCCESS;
#endif
}

static int CACHE(MIPS_instr mips){
	return CONVERT_ERROR;
}

static int LL(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_LL
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_LL
	genCallDynaMem(MEM_LL, 1, MIPS_GET_RS(mips), MIPS_GET_RT(mips), MIPS_GET_IMMED(mips));
	return CONVERT_SUCCESS;
#endif
}

static int SC(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_SC
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_SC
	genCallDynaMem(MEM_SC, 1, MIPS_GET_RS(mips), MIPS_GET_RT(mips), MIPS_GET_IMMED(mips));
	return CONVERT_SUCCESS;
#endif
}

// -- Special Functions --

static int SLL(MIPS_instr mips){
	PowerPC_instr ppc;

	int rt = mapRegister( MIPS_GET_RT(mips) );
	int rd = mapRegisterNew( MIPS_GET_RD(mips), 1 );

	GEN_SLWI(ppc, rd, rt, MIPS_GET_SA(mips));
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
}

static int SRL(MIPS_instr mips){
	PowerPC_instr ppc;

	int rt = mapRegister( MIPS_GET_RT(mips) );
	int rd = mapRegisterNew( MIPS_GET_RD(mips), 1 );

	GEN_SRWI(ppc, rd, rt, MIPS_GET_SA(mips));
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
}

static int SRA(MIPS_instr mips){
	PowerPC_instr ppc;

	int rt = mapRegister( MIPS_GET_RT(mips) );
	int rd = mapRegisterNew( MIPS_GET_RD(mips), 1 );

	GEN_SRAWI(ppc, rd, rt, MIPS_GET_SA(mips));
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
}

static int SLLV(MIPS_instr mips){
	PowerPC_instr ppc;
	int rt = mapRegister( MIPS_GET_RT(mips) );
	int rs = mapRegister( MIPS_GET_RS(mips) );
	int rd = mapRegisterNew( MIPS_GET_RD(mips), 1 );

	GEN_CLRLWI(ppc, R0, rs, 27); // Mask the lower 5-bits of rs
	set_next_dst(ppc);
	GEN_SLW(ppc, rd, rt, R0);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
}

static int SRLV(MIPS_instr mips){
	PowerPC_instr ppc;
	int rt = mapRegister( MIPS_GET_RT(mips) );
	int rs = mapRegister( MIPS_GET_RS(mips) );
	int rd = mapRegisterNew( MIPS_GET_RD(mips), 1 );

	GEN_CLRLWI(ppc, R0, rs, 27); // Mask the lower 5-bits of rs
	set_next_dst(ppc);
	GEN_SRW(ppc, rd, rt, R0);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
}

static int SRAV(MIPS_instr mips){
	PowerPC_instr ppc;
	int rt = mapRegister( MIPS_GET_RT(mips) );
	int rs = mapRegister( MIPS_GET_RS(mips) );
	int rd = mapRegisterNew( MIPS_GET_RD(mips), 1 );

	GEN_CLRLWI(ppc, R0, rs, 27); // Mask the lower 5-bits of rs
	set_next_dst(ppc);
	GEN_SRAW(ppc, rd, rt, R0);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
}

static int JR(MIPS_instr mips){
	PowerPC_instr ppc;

	if(CANT_COMPILE_DELAY()){
		genCallInterp(mips);
		return INTERPRETED;
	}

	flushRegisters();
	reset_code_addr();

	GEN_STW(ppc, mapRegister(MIPS_GET_RS(mips)), SDAREL(local_rs32), R13);
	set_next_dst(ppc);

	// Check the delay slot, and note how big it is
	PowerPC_instr* preDelay = get_curr_dst();
	check_delaySlot();
	int delaySlot = get_curr_dst() - preDelay;

#ifdef COMPARE_CORE
	GEN_LI(ppc, R4, 1);
	set_next_dst(ppc);
#endif
	genUpdateCount(0);

	invalidateRegisters();

#ifdef INTERPRET_JR
	genJumpTo(SDAREL(local_rs32), JUMPTO_REG);
#else // INTERPRET_JR
	// TODO: jr
#endif

	// Let's still recompile the delay slot in place in case its branched to
	if(delaySlot){ if(is_j_dst(0)){ unget_last_src(); delaySlotNext = 2; } }
	else nop_ignored();

#ifdef INTERPRET_JR
	return INTERPRETED;
#else // INTERPRET_JR
	return CONVER_ERROR;
#endif
}

static int JALR(MIPS_instr mips){
	PowerPC_instr  ppc;

	if(CANT_COMPILE_DELAY()){
		genCallInterp(mips);
		return INTERPRETED;
	}

	flushRegisters();
	reset_code_addr();

	GEN_STW(ppc, mapRegister(MIPS_GET_RS(mips)), SDAREL(local_rs32), R13);
	set_next_dst(ppc);

	// Check the delay slot, and note how big it is
	PowerPC_instr* preDelay = get_curr_dst();
	check_delaySlot();
	int delaySlot = get_curr_dst() - preDelay;

#ifdef COMPARE_CORE
	GEN_LI(ppc, R4, 1);
	set_next_dst(ppc);
#endif
	genUpdateCount(0);

	// Set LR to next instruction
	int rd = mapRegisterNew( MIPS_GET_RD(mips), 1 );
	// lis	lr, pc@ha(0)
	GEN_LIS(ppc, rd, (get_src_pc()+4)>>16);
	set_next_dst(ppc);
	// la	lr, pc@l(lr)
	GEN_ORI(ppc, rd, rd, get_src_pc()+4);
	set_next_dst(ppc);

	flushRegisters();

#ifdef INTERPRET_JALR
	genJumpTo(SDAREL(local_rs32), JUMPTO_REG);
#else // INTERPRET_JALR
	// TODO: jalr
#endif

	// Let's still recompile the delay slot in place in case its branched to
	if(delaySlot){ if(is_j_dst(0)){ unget_last_src(); delaySlotNext = 2; } }
	else nop_ignored();

#ifdef INTERPRET_JALR
	return INTERPRETED;
#else // INTERPRET_JALR
	return CONVERT_ERROR;
#endif
}

static int SYSCALL(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_SYSCALL
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_SYSCALL
	// TODO: syscall
	return CONVERT_ERROR;
#endif
}

static int BREAK(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_BREAK
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_BREAK
	return CONVERT_ERROR;
#endif
}

static int SYNC(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_SYNC
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_SYNC
	return CONVERT_ERROR;
#endif
}

static int MFHI(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_HILO
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_HILO

	RegMapping hi = mapRegister64( MIPS_REG_HI );
	RegMapping rd = mapRegister64New( MIPS_GET_RD(mips) );

	// mr rd, hi
	GEN_MR(ppc, rd.lo, hi.lo);
	set_next_dst(ppc);
	GEN_MR(ppc, rd.hi, hi.hi);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
#endif
}

static int MTHI(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_HILO
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_HILO

	RegMapping rs = mapRegister64( MIPS_GET_RS(mips) );
	RegMapping hi = mapRegister64New( MIPS_REG_HI );

	// mr hi, rs
	GEN_MR(ppc, hi.lo, rs.lo);
	set_next_dst(ppc);
	GEN_MR(ppc, hi.hi, rs.hi);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
#endif
}

static int MFLO(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_HILO
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_HILO

	RegMapping lo = mapRegister64( MIPS_REG_LO );
	RegMapping rd = mapRegister64New( MIPS_GET_RD(mips) );

	// mr rd, lo
	GEN_MR(ppc, rd.lo, lo.lo);
	set_next_dst(ppc);
	GEN_MR(ppc, rd.hi, lo.hi);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
#endif
}

static int MTLO(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_HILO
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_HILO

	RegMapping rs = mapRegister64( MIPS_GET_RS(mips) );
	RegMapping lo = mapRegister64New( MIPS_REG_LO );

	// mr lo, rs
	GEN_MR(ppc, lo.lo, rs.lo);
	set_next_dst(ppc);
	GEN_MR(ppc, lo.hi, rs.hi);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
#endif
}

static int MULT(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_MULT
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_MULT
	int rs = mapRegister( MIPS_GET_RS(mips) );
	int rt = mapRegister( MIPS_GET_RT(mips) );
	int hi = mapRegisterNew( MIPS_REG_HI, 1 );
	int lo = mapRegisterNew( MIPS_REG_LO, 1 );

	// Don't multiply if they're using r0
	if(MIPS_GET_RS(mips) && MIPS_GET_RT(mips)){
		// mullw lo, rs, rt
		GEN_MULLW(ppc, lo, rs, rt);
		set_next_dst(ppc);
		// mulhw hi, rs, rt
		GEN_MULHW(ppc, hi, rs, rt);
		set_next_dst(ppc);
	} else {
		// li lo, 0
		GEN_LI(ppc, lo, 0);
		set_next_dst(ppc);
		// li hi, 0
		GEN_LI(ppc, hi, 0);
		set_next_dst(ppc);
	}

	return CONVERT_SUCCESS;
#endif
}

static int MULTU(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_MULTU
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_MULTU
	int rs = mapRegister( MIPS_GET_RS(mips) );
	int rt = mapRegister( MIPS_GET_RT(mips) );
	int hi = mapRegisterNew( MIPS_REG_HI, 1 );
	int lo = mapRegisterNew( MIPS_REG_LO, 1 );

	// Don't multiply if they're using r0
	if(MIPS_GET_RS(mips) && MIPS_GET_RT(mips)){
		// mullw lo, rs, rt
		GEN_MULLW(ppc, lo, rs, rt);
		set_next_dst(ppc);
		// mulhwu hi, rs, rt
		GEN_MULHWU(ppc, hi, rs, rt);
		set_next_dst(ppc);
	} else {
		// li lo, 0
		GEN_LI(ppc, lo, 0);
		set_next_dst(ppc);
		// li hi, 0
		GEN_LI(ppc, hi, 0);
		set_next_dst(ppc);
	}

	return CONVERT_SUCCESS;
#endif
}

static int DIV(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_DIV
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_DIV
	// This instruction computes the quotient and remainder
	//   and stores the results in lo and hi respectively
	int rs = mapRegister( MIPS_GET_RS(mips) );
	int rt = mapRegister( MIPS_GET_RT(mips) );
	int hi = mapRegisterNew( MIPS_REG_HI, 1 );
	int lo = mapRegisterNew( MIPS_REG_LO, 1 );

	// Don't divide if they're using r0
	if(MIPS_GET_RS(mips) && MIPS_GET_RT(mips)){
		// divw lo, rs, rt
		GEN_DIVW(ppc, lo, rs, rt);
		set_next_dst(ppc);
		// This is how you perform a mod in PPC
		// divw lo, rs, rt
		// NOTE: We already did that
		// mullw hi, lo, rt
		GEN_MULLW(ppc, hi, lo, rt);
		set_next_dst(ppc);
		// subf hi, hi, rs
		GEN_SUBF(ppc, hi, hi, rs);
		set_next_dst(ppc);
	}

	return CONVERT_SUCCESS;
#endif
}

static int DIVU(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_DIVU
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_DIVU
	// This instruction computes the quotient and remainder
	//   and stores the results in lo and hi respectively
	int rs = mapRegister( MIPS_GET_RS(mips) );
	int rt = mapRegister( MIPS_GET_RT(mips) );
	int hi = mapRegisterNew( MIPS_REG_HI, 1 );
	int lo = mapRegisterNew( MIPS_REG_LO, 1 );

	// Don't divide if they're using r0
	if(MIPS_GET_RS(mips) && MIPS_GET_RT(mips)){
		// divwu lo, rs, rt
		GEN_DIVWU(ppc, lo, rs, rt);
		set_next_dst(ppc);
		// This is how you perform a mod in PPC
		// divw lo, rs, rt
		// NOTE: We already did that
		// mullw hi, lo, rt
		GEN_MULLW(ppc, hi, lo, rt);
		set_next_dst(ppc);
		// subf hi, hi, rs
		GEN_SUBF(ppc, hi, hi, rs);
		set_next_dst(ppc);
	}

	return CONVERT_SUCCESS;
#endif
}

static int DSLLV(MIPS_instr mips){
	PowerPC_instr ppc;
#if defined(INTERPRET_DW) || defined(INTERPRET_DSLLV)
	genCallInterp(mips);
	return INTERPRETED;
#else  // INTERPRET_DW || INTERPRET_DSLLV

	int rs = mapRegister( MIPS_GET_RS(mips) );
	RegMapping rt = mapRegister64( MIPS_GET_RT(mips) );
	RegMapping rd = mapRegister64New( MIPS_GET_RD(mips) );

	GEN_CLRLWI(ppc, R2, rs, 26);
	set_next_dst(ppc);
	GEN_SUBIC_(ppc, R0, R2, 32);
	set_next_dst(ppc);
	GEN_BLT(ppc, CR0, 4, 0, 0);
	set_next_dst(ppc);
	GEN_SLW(ppc, rd.hi, rt.lo, R0);
	set_next_dst(ppc);
	GEN_LI(ppc, rd.lo, 0);
	set_next_dst(ppc);
	GEN_B(ppc, 6, 0, 0);
	set_next_dst(ppc);
	GEN_SUBFIC(ppc, R0, R2, 32);
	set_next_dst(ppc);
	GEN_SLW(ppc, rd.hi, rt.hi, R2);
	set_next_dst(ppc);
	GEN_SRW(ppc, R0, rt.lo, R0);
	set_next_dst(ppc);
	GEN_SLW(ppc, rd.lo, rt.lo, R2);
	set_next_dst(ppc);
	GEN_OR(ppc, rd.hi, rd.hi, R0);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
#endif
}

static int DSRLV(MIPS_instr mips){
	PowerPC_instr ppc;
#if defined(INTERPRET_DW) || defined(INTERPRET_DSRLV)
	genCallInterp(mips);
	return INTERPRETED;
#else  // INTERPRET_DW || INTERPRET_DSRLV

	int rs = mapRegister( MIPS_GET_RS(mips) );
	RegMapping rt = mapRegister64( MIPS_GET_RT(mips) );
	RegMapping rd = mapRegister64New( MIPS_GET_RD(mips) );

	GEN_CLRLWI(ppc, R2, rs, 26);
	set_next_dst(ppc);
	GEN_SUBIC_(ppc, R0, R2, 32);
	set_next_dst(ppc);
	GEN_BLT(ppc, CR0, 4, 0, 0);
	set_next_dst(ppc);
	GEN_SRW(ppc, rd.lo, rt.hi, R0);
	set_next_dst(ppc);
	GEN_LI(ppc, rd.hi, 0);
	set_next_dst(ppc);
	GEN_B(ppc, 6, 0, 0);
	set_next_dst(ppc);
	GEN_SUBFIC(ppc, R0, R2, 32);
	set_next_dst(ppc);
	GEN_SRW(ppc, rd.lo, rt.lo, R2);
	set_next_dst(ppc);
	GEN_SLW(ppc, R0, rt.hi, R0);
	set_next_dst(ppc);
	GEN_SRW(ppc, rd.hi, rt.hi, R2);
	set_next_dst(ppc);
	GEN_OR(ppc, rd.lo, rd.lo, R0);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
#endif
}

static int DSRAV(MIPS_instr mips){
	PowerPC_instr ppc;
#if defined(INTERPRET_DW) || defined(INTERPRET_DSRAV)
	genCallInterp(mips);
	return INTERPRETED;
#else  // INTERPRET_DW || INTERPRET_DSRAV

	int rs = mapRegister( MIPS_GET_RS(mips) );
	RegMapping rt = mapRegister64( MIPS_GET_RT(mips) );
	RegMapping rd = mapRegister64New( MIPS_GET_RD(mips) );

	GEN_CLRLWI(ppc, R2, rs, 26);
	set_next_dst(ppc);
	GEN_SUBIC_(ppc, R0, R2, 32);
	set_next_dst(ppc);
	GEN_BLT(ppc, CR0, 4, 0, 0);
	set_next_dst(ppc);
	GEN_SRAW(ppc, rd.lo, rt.hi, R0);
	set_next_dst(ppc);
	GEN_SRAWI(ppc, rd.hi, rt.hi, 31);
	set_next_dst(ppc);
	GEN_B(ppc, 6, 0, 0);
	set_next_dst(ppc);
	GEN_SUBFIC(ppc, R0, R2, 32);
	set_next_dst(ppc);
	GEN_SRW(ppc, rd.lo, rt.lo, R2);
	set_next_dst(ppc);
	GEN_SLW(ppc, R0, rt.hi, R0);
	set_next_dst(ppc);
	GEN_SRAW(ppc, rd.hi, rt.hi, R2);
	set_next_dst(ppc);
	GEN_OR(ppc, rd.lo, rd.lo, R0);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
#endif
}

static int DMULT(MIPS_instr mips){
	PowerPC_instr ppc;
#if defined(INTERPRET_DW) || defined(INTERPRET_DMULT)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_DW || INTERPRET_DMULT
	// TODO: dmult
	return CONVERT_ERROR;
#endif
}

static int DMULTU(MIPS_instr mips){
	PowerPC_instr ppc;
#if defined(INTERPRET_DW) || defined(INTERPRET_DMULTU)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_DW || INTERPRET_DMULTU
	// TODO: dmultu
	return CONVERT_ERROR;
#endif
}

static int DDIV(MIPS_instr mips){
	PowerPC_instr ppc;
#if defined(INTERPRET_DW) || defined(INTERPRET_DDIV)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_DW || INTERPRET_DDIV
	flushRegisters();
	RegMapping rs = mapRegister64( MIPS_GET_RS(mips) );
	RegMapping rt = mapRegister64( MIPS_GET_RT(mips) );
	invalidateRegisters();

	GEN_CMPI(ppc, CR2, rs.hi, 0);
	set_next_dst(ppc);
	GEN_CMPI(ppc, CR1, rt.hi, 0);
	set_next_dst(ppc);
	GEN_BGE(ppc, CR2, 3, 0, 0);
	set_next_dst(ppc);
	GEN_SUBFIC(ppc, rs.lo, rs.lo, 0);
	set_next_dst(ppc);
	GEN_SUBFZE(ppc, rs.hi, rs.hi);
	set_next_dst(ppc);
	GEN_BGE(ppc, CR1, 3, 0, 0);
	set_next_dst(ppc);
	GEN_SUBFIC(ppc, rt.lo, rt.lo, 0);
	set_next_dst(ppc);
	GEN_SUBFZE(ppc, rt.hi, rt.hi);
	set_next_dst(ppc);
	GEN_CRXOR(ppc, CR2*4+2, CR1*4+0, CR2*4+0);
	set_next_dst(ppc);

	// r7 = &reg[hi]
	GEN_ADDI(ppc, R7, R13, SDAREL(reg[MIPS_REG_HI]));
	set_next_dst(ppc);

	// divide
	GEN_B(ppc, add_jump(&__udivmoddi4, 1, 1), 0, 1);
	set_next_dst(ppc);

	RegMapping lo = mapRegister64New( MIPS_REG_LO );
	RegMapping hi = mapRegister64( MIPS_REG_HI );

	GEN_BNE(ppc, CR2, 3, 0, 0);
	set_next_dst(ppc);
	GEN_SUBFIC(ppc, lo.lo, lo.lo, 0);
	set_next_dst(ppc);
	GEN_SUBFZE(ppc, lo.hi, lo.hi);
	set_next_dst(ppc);
	GEN_BGE(ppc, CR2, 3, 0, 0);
	set_next_dst(ppc);
	GEN_SUBFIC(ppc, hi.lo, hi.lo, 0);
	set_next_dst(ppc);
	GEN_SUBFZE(ppc, hi.hi, hi.hi);
	set_next_dst(ppc);

	// Load old LR
	GEN_LWZ(ppc, R0, DYNAOFF_LR, R1);
	set_next_dst(ppc);
	// Restore LR
	GEN_MTLR(ppc, R0);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
#endif
}

static int DDIVU(MIPS_instr mips){
	PowerPC_instr ppc;
#if defined(INTERPRET_DW) || defined(INTERPRET_DDIVU)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_DW || INTERPRET_DDIVU
	flushRegisters();
	mapRegister64( MIPS_GET_RS(mips) );
	mapRegister64( MIPS_GET_RT(mips) );
	invalidateRegisters();

	// r7 = &reg[hi]
	GEN_ADDI(ppc, R7, R13, SDAREL(reg[MIPS_REG_HI]));
	set_next_dst(ppc);

	// divide
	GEN_B(ppc, add_jump(&__udivmoddi4, 1, 1), 0, 1);
	set_next_dst(ppc);

	mapRegister64New( MIPS_REG_LO );

	// Load old LR
	GEN_LWZ(ppc, R0, DYNAOFF_LR, R1);
	set_next_dst(ppc);
	// Restore LR
	GEN_MTLR(ppc, R0);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
#endif
}

static int DADDU(MIPS_instr mips){
	PowerPC_instr ppc;
#if defined(INTERPRET_DW) || defined(INTERPRET_DADDU)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_DW || INTERPRET_DADDU

	RegMapping rs = mapRegister64( MIPS_GET_RS(mips) );
	RegMapping rt = mapRegister64( MIPS_GET_RT(mips) );
	RegMapping rd = mapRegister64New( MIPS_GET_RD(mips) );

	GEN_ADDC(ppc, rd.lo, rs.lo, rt.lo);
	set_next_dst(ppc);
	GEN_ADDE(ppc, rd.hi, rs.hi, rt.hi);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
#endif
}

static int DADD(MIPS_instr mips){
	return DADDU(mips);
}

static int DSUBU(MIPS_instr mips){
	PowerPC_instr ppc;
#if defined(INTERPRET_DW) || defined(INTERPRET_DSUBU)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_DW || INTERPRET_DSUBU

	RegMapping rs = mapRegister64( MIPS_GET_RS(mips) );
	RegMapping rt = mapRegister64( MIPS_GET_RT(mips) );
	RegMapping rd = mapRegister64New( MIPS_GET_RD(mips) );

	GEN_SUBFC(ppc, rd.lo, rt.lo, rs.lo);
	set_next_dst(ppc);
	GEN_SUBFE(ppc, rd.hi, rt.hi, rs.hi);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
#endif
}

static int DSUB(MIPS_instr mips){
	return DSUBU(mips);
}

static int DSLL(MIPS_instr mips){
	PowerPC_instr ppc;
#if defined(INTERPRET_DW) || defined(INTERPRET_DSLL)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_DW || INTERPRET_DSLL

	RegMapping rt = mapRegister64( MIPS_GET_RT(mips) );
	RegMapping rd = mapRegister64New( MIPS_GET_RD(mips) );
	int sa = MIPS_GET_SA(mips);

	if(sa){
		// Shift MSW left by SA
		GEN_SLWI(ppc, rd.hi, rt.hi, sa);
		set_next_dst(ppc);
		// Extract the bits shifted out of the LSW
		// Insert those bits into the MSW
		GEN_RLWIMI(ppc, rd.hi, rt.lo, sa, 32-sa, 31);
		set_next_dst(ppc);
		// Shift LSW left by SA
		GEN_SLWI(ppc, rd.lo, rt.lo, sa);
		set_next_dst(ppc);
	} else {
		// Copy over the register
		GEN_MR(ppc, rd.hi, rt.hi);
		set_next_dst(ppc);
		GEN_MR(ppc, rd.lo, rt.lo);
		set_next_dst(ppc);
	}

	return CONVERT_SUCCESS;
#endif
}

static int DSRL(MIPS_instr mips){
	PowerPC_instr ppc;
#if defined(INTERPRET_DW) || defined(INTERPRET_DSRL)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_DW || INTERPRET_DSRL

	RegMapping rt = mapRegister64( MIPS_GET_RT(mips) );
	RegMapping rd = mapRegister64New( MIPS_GET_RD(mips) );
	int sa = MIPS_GET_SA(mips);

	if(sa){
		// Shift LSW right by SA
		GEN_SRWI(ppc, rd.lo, rt.lo, sa);
		set_next_dst(ppc);
		// Extract the bits shifted out of the MSW
		// Insert those bits into the LSW
		GEN_RLWIMI(ppc, rd.lo, rt.hi, 32-sa, 0, sa-1);
		set_next_dst(ppc);
		// Shift MSW right by SA
		GEN_SRWI(ppc, rd.hi, rt.hi, sa);
		set_next_dst(ppc);
	} else {
		// Copy over the register
		GEN_MR(ppc, rd.hi, rt.hi);
		set_next_dst(ppc);
		GEN_MR(ppc, rd.lo, rt.lo);
		set_next_dst(ppc);
	}

	return CONVERT_SUCCESS;
#endif
}

static int DSRA(MIPS_instr mips){
	PowerPC_instr ppc;
#if defined(INTERPRET_DW) || defined(INTERPRET_DSRA)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_DW || INTERPRET_DSRA

	RegMapping rt = mapRegister64( MIPS_GET_RT(mips) );
	RegMapping rd = mapRegister64New( MIPS_GET_RD(mips) );
	int sa = MIPS_GET_SA(mips);

	if(sa){
		// Shift LSW right by SA
		GEN_SRWI(ppc, rd.lo, rt.lo, sa);
		set_next_dst(ppc);
		// Extract the bits shifted out of the MSW
		// Insert those bits into the LSW
		GEN_RLWIMI(ppc, rd.lo, rt.hi, 32-sa, 0, sa-1);
		set_next_dst(ppc);
		// Shift (arithmetically) MSW right by SA
		GEN_SRAWI(ppc, rd.hi, rt.hi, sa);
		set_next_dst(ppc);
	} else {
		// Copy over the register
		GEN_MR(ppc, rd.hi, rt.hi);
		set_next_dst(ppc);
		GEN_MR(ppc, rd.lo, rt.lo);
		set_next_dst(ppc);
	}

	return CONVERT_SUCCESS;
#endif
}

static int DSLL32(MIPS_instr mips){
	PowerPC_instr ppc;
#if defined(INTERPRET_DW) || defined(INTERPRET_DSLL32)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_DW || INTERPRET_DSLL32

	RegMapping rt = mapRegister64( MIPS_GET_RT(mips) );
	RegMapping rd = mapRegister64New( MIPS_GET_RD(mips) );
	int sa = MIPS_GET_SA(mips);

	// Shift LSW into MSW and by SA
	GEN_SLWI(ppc, rd.hi, rt.lo, sa);
	set_next_dst(ppc);
	// Clear out LSW
	GEN_LI(ppc, rd.lo, 0);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
#endif
}

static int DSRL32(MIPS_instr mips){
	PowerPC_instr ppc;
#if defined(INTERPRET_DW) || defined(INTERPRET_DSRL32)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_DW || INTERPRET_DSRL32

	RegMapping rt = mapRegister64( MIPS_GET_RT(mips) );
	RegMapping rd = mapRegister64New( MIPS_GET_RD(mips) );
	int sa = MIPS_GET_SA(mips);

	// Shift MSW into LSW and by SA
	GEN_SRWI(ppc, rd.lo, rt.hi, sa);
	set_next_dst(ppc);
	// Clear out MSW
	GEN_LI(ppc, rd.hi, 0);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
#endif
}

static int DSRA32(MIPS_instr mips){
	PowerPC_instr ppc;
#if defined(INTERPRET_DW) || defined(INTERPRET_DSRA32)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_DW || INTERPRET_DSRA32

	RegMapping rt = mapRegister64( MIPS_GET_RT(mips) );
	RegMapping rd = mapRegister64New( MIPS_GET_RD(mips) );
	int sa = MIPS_GET_SA(mips);

	// Shift (arithmetically) MSW into LSW and by SA
	GEN_SRAWI(ppc, rd.lo, rt.hi, sa);
	set_next_dst(ppc);
	// Fill MSW with sign of MSW
	GEN_SRAWI(ppc, rd.hi, rt.hi, 31);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
#endif
}

static int ADDU(MIPS_instr mips){
	PowerPC_instr ppc;
	int rt = mapRegister( MIPS_GET_RT(mips) );
	int rs = mapRegister( MIPS_GET_RS(mips) );
	int rd = mapRegisterNew( MIPS_GET_RD(mips), 1 );

	GEN_ADD(ppc, rd, rs, rt);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
}

static int ADD(MIPS_instr mips){
	return ADDU(mips);
}

static int SUBU(MIPS_instr mips){
	PowerPC_instr ppc;
	int rt = mapRegister( MIPS_GET_RT(mips) );
	int rs = mapRegister( MIPS_GET_RS(mips) );
	int rd = mapRegisterNew( MIPS_GET_RD(mips), 1 );

	GEN_SUB(ppc, rd, rs, rt);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
}

static int SUB(MIPS_instr mips){
	return SUBU(mips);
}

static int AND(MIPS_instr mips){
	PowerPC_instr ppc;
	RegMapping rt = mapRegister64( MIPS_GET_RT(mips) );
	RegMapping rs = mapRegister64( MIPS_GET_RS(mips) );
	RegMapping rd = mapRegister64New( MIPS_GET_RD(mips) );

	GEN_AND(ppc, rd.hi, rs.hi, rt.hi);
	set_next_dst(ppc);
	GEN_AND(ppc, rd.lo, rs.lo, rt.lo);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
}

static int OR(MIPS_instr mips){
	PowerPC_instr ppc;
	RegMapping rt = mapRegister64( MIPS_GET_RT(mips) );
	RegMapping rs = mapRegister64( MIPS_GET_RS(mips) );
	RegMapping rd = mapRegister64New( MIPS_GET_RD(mips) );

	GEN_OR(ppc, rd.hi, rs.hi, rt.hi);
	set_next_dst(ppc);
	GEN_OR(ppc, rd.lo, rs.lo, rt.lo);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
}

static int XOR(MIPS_instr mips){
	PowerPC_instr ppc;
	RegMapping rt = mapRegister64( MIPS_GET_RT(mips) );
	RegMapping rs = mapRegister64( MIPS_GET_RS(mips) );
	RegMapping rd = mapRegister64New( MIPS_GET_RD(mips) );

	GEN_XOR(ppc, rd.hi, rs.hi, rt.hi);
	set_next_dst(ppc);
	GEN_XOR(ppc, rd.lo, rs.lo, rt.lo);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
}

static int NOR(MIPS_instr mips){
	PowerPC_instr ppc;
	RegMapping rt = mapRegister64( MIPS_GET_RT(mips) );
	RegMapping rs = mapRegister64( MIPS_GET_RS(mips) );
	RegMapping rd = mapRegister64New( MIPS_GET_RD(mips) );

	GEN_NOR(ppc, rd.hi, rs.hi, rt.hi);
	set_next_dst(ppc);
	GEN_NOR(ppc, rd.lo, rs.lo, rt.lo);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
}

static int SLT(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_SLT
	genCallInterp(mips);
	return INTERPRETED;
#else
	// FIXME: Do I need to worry about 64-bit values?
	int rt = mapRegister( MIPS_GET_RT(mips) );
	int rs = mapRegister( MIPS_GET_RS(mips) );
	int rd = mapRegisterNew( MIPS_GET_RD(mips), 0 );

	GEN_CMP(ppc, CR0, rs, rt);
	set_next_dst(ppc);
	GEN_MFCR(ppc, R2);
	set_next_dst(ppc);
	GEN_RLWINM(ppc, rd, R2, 1, 31, 31);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
#endif
}

static int SLTU(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_SLTU
	genCallInterp(mips);
	return INTERPRETED;
#else
	// FIXME: Do I need to worry about 64-bit values?
	int rt = mapRegister( MIPS_GET_RT(mips) );
	int rs = mapRegister( MIPS_GET_RS(mips) );
	int rd = mapRegisterNew( MIPS_GET_RD(mips), 0 );

	GEN_NOT(ppc, R0, rs);
	set_next_dst(ppc);
	GEN_ADDC(ppc, R0, R0, rt);
	set_next_dst(ppc);
	GEN_ADDZE(ppc, rd, DYNAREG_ZERO);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
#endif
}

static int TEQ(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_TRAPS
	genCallInterp(mips);
	return INTERPRETED;
#else
	return CONVERT_ERROR;
#endif
}

static int (*gen_special[64])(MIPS_instr) =
{
   SLL , NI   , SRL , SRA , SLLV   , NI    , SRLV  , SRAV  ,
   JR  , JALR , NI  , NI  , SYSCALL, BREAK , NI    , SYNC  ,
   MFHI, MTHI , MFLO, MTLO, DSLLV  , NI    , DSRLV , DSRAV ,
   MULT, MULTU, DIV , DIVU, DMULT  , DMULTU, DDIV  , DDIVU ,
   ADD , ADDU , SUB , SUBU, AND    , OR    , XOR   , NOR   ,
   NI  , NI   , SLT , SLTU, DADD   , DADDU , DSUB  , DSUBU ,
   NI  , NI   , NI  , NI  , TEQ    , NI    , NI    , NI    ,
   DSLL, NI   , DSRL, DSRA, DSLL32 , NI    , DSRL32, DSRA32
};

static int SPECIAL(MIPS_instr mips){
	return gen_special[MIPS_GET_FUNC(mips)](mips);
}

// -- RegImmed Instructions --

// Since the RegImmed instructions are very similar:
//   BLTZ, BGEZ, BLTZL, BGEZL, BLZAL, BGEZAL, BLTZALL, BGEZALL
//   It's less work to handle them all in one function
static int REGIMM(MIPS_instr mips){
	PowerPC_instr  ppc;
	int which = MIPS_GET_RT(mips);
	int cond   = which & 1; // t = GE, f = LT
	int likely = which & 2;
	int link   = which & 16;

	if(MIPS_GET_IMMED(mips) == 0xffff || CANT_COMPILE_DELAY()){
		// REGIMM_IDLE || virtual delay
		genCallInterp(mips);
		return INTERPRETED;
	}

	genCmpi64(CR4, MIPS_GET_RA(mips), 0);

	return branch(MIPS_GET_IMMED(mips), cond ? GE : LT, link, likely);
}

// -- COP0 Instructions --

static int TLBR(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_TLBR
	genCallInterp(mips);
	return INTERPRETED;
#else
	return CONVERT_ERROR;
#endif
}

static int TLBWI(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_TLBWI
	genCallInterp(mips);
	return INTERPRETED;
#else
	return CONVERT_ERROR;
#endif
}

static int TLBWR(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_TLBWR
	genCallInterp(mips);
	return INTERPRETED;
#else
	return CONVERT_ERROR;
#endif
}

static int TLBP(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_TLBP
	genCallInterp(mips);
	return INTERPRETED;
#else
	return CONVERT_ERROR;
#endif
}

static int ERET(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_ERET
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_ERET

	flushRegisters();

	genUpdateCount(0);
	// Load Status
	GEN_LWZ(ppc, R3, SDAREL(Status), R13);
	set_next_dst(ppc);
	// Status & 0xFFFFFFFD
	GEN_RLWINM(ppc, R3, R3, 0, 31, 29);
	set_next_dst(ppc);
	// llbit = 0
	GEN_STW(ppc, DYNAREG_ZERO, SDAREL(llbit), R13);
	set_next_dst(ppc);
	// Store updated Status
	GEN_STW(ppc, R3, SDAREL(Status), R13);
	set_next_dst(ppc);
	// check_interupt()
	GEN_B(ppc, add_jump(&check_interupt, 1, 1), 0, 1);
	set_next_dst(ppc);
	// Load the old LR
	GEN_LWZ(ppc, R0, DYNAOFF_LR, R1);
	set_next_dst(ppc);
	// interp_addr = EPC
	GEN_LWZ(ppc, R3, SDAREL(EPC), R13);
	set_next_dst(ppc);
	// Restore the LR
	GEN_MTLR(ppc, R0);
	set_next_dst(ppc);
	// Return to trampoline with EPC
	GEN_BLR(ppc, 0);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
#endif
}

static int (*gen_tlb[64])(MIPS_instr) =
{
   NI  , TLBR, TLBWI, NI, NI, NI, TLBWR, NI,
   TLBP, NI  , NI   , NI, NI, NI, NI   , NI,
   NI  , NI  , NI   , NI, NI, NI, NI   , NI,
   ERET, NI  , NI   , NI, NI, NI, NI   , NI,
   NI  , NI  , NI   , NI, NI, NI, NI   , NI,
   NI  , NI  , NI   , NI, NI, NI, NI   , NI,
   NI  , NI  , NI   , NI, NI, NI, NI   , NI,
   NI  , NI  , NI   , NI, NI, NI, NI   , NI
};

static int MFC0(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_MFC0
	genCallInterp(mips);
	return INTERPRETED;
#else

	int rt = mapRegisterNew( MIPS_GET_RT(mips), 1 );
	// *rt = reg_cop0[rd]
	GEN_LWZ(ppc, rt, SDAREL(reg_cop0) + MIPS_GET_RD(mips)*4, R13);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
#endif
}

static int MTC0(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_MTC0
	genCallInterp(mips);
	return INTERPRETED;
#else
	
	int rt = MIPS_GET_RT(mips), rrt;
	int rd = MIPS_GET_RD(mips);
	
	switch(rd){
	case 0: // Index
		rrt = mapRegister(rt);
		// r0 = rt & 0x8000003F
		GEN_RLWINM(ppc, R0, rrt, 0, 26, 0);
		set_next_dst(ppc);
		// reg_cop0[rd] = r0
		GEN_STW(ppc, R0, SDAREL(reg_cop0) + rd*4, R13);
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
	
	case 2: // EntryLo0
	case 3: // EntryLo1
		rrt = mapRegister(rt);
		// r0 = rt & 0x3FFFFFFF
		GEN_RLWINM(ppc, R0, rrt, 0, 2, 31);
		set_next_dst(ppc);
		// reg_cop0[rd] = r0
		GEN_STW(ppc, R0, SDAREL(reg_cop0) + rd*4, R13);
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
	
	case 4: // Context
		rrt = mapRegister(rt);
		// r0 = reg_cop0[rd]
		GEN_LWZ(ppc, R0, SDAREL(reg_cop0) + rd*4, R13);
		set_next_dst(ppc);
		// r0 = (rt & 0xFF800000) | (r0 & 0x007FFFFF)
		GEN_RLWIMI(ppc, R0, rrt, 0, 0, 8);
		set_next_dst(ppc);
		// reg_cop0[rd] = r0
		GEN_STW(ppc, R0, SDAREL(reg_cop0) + rd*4, R13);
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
	
	case 5: // PageMask
		rrt = mapRegister(rt);
		// r0 = rt & 0x01FFE000
		GEN_RLWINM(ppc, R0, rrt, 0, 7, 18);
		set_next_dst(ppc);
		// reg_cop0[rd] = r0
		GEN_STW(ppc, R0, SDAREL(reg_cop0) + rd*4, R13);
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
	
	case 6: // Wired
		rrt = mapRegister(rt);
		// r0 = 31
		GEN_LI(ppc, R0, 31);
		set_next_dst(ppc);
		// reg_cop0[rd] = rt
		GEN_STW(ppc, rrt, SDAREL(reg_cop0) + rd*4, R13);
		set_next_dst(ppc);
		// reg_cop0[1] = r0
		GEN_STW(ppc, R0, SDAREL(Random), R13);
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
	
	case 10: // EntryHi
		rrt = mapRegister(rt);
		// r0 = rt & 0xFFFFE0FF
		GEN_RLWINM(ppc, R0, rrt, 0, 24, 18);
		set_next_dst(ppc);
		// reg_cop0[rd] = r0
		GEN_STW(ppc, R0, SDAREL(reg_cop0) + rd*4, R13);
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
	
	case 13: // Cause
	case 14: // EPC
	case 16: // Config
	case 18: // WatchLo
	case 19: // WatchHi
		rrt = mapRegister(rt);
		// reg_cop0[rd] = rt
		GEN_STW(ppc, rrt, SDAREL(reg_cop0) + rd*4, R13);
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
	
	case 28: // TagLo
		rrt = mapRegister(rt);
		// r0 = rt & 0x0FFFFFC0
		GEN_RLWINM(ppc, R0, rrt, 0, 4, 25);
		set_next_dst(ppc);
		// reg_cop0[rd] = r0
		GEN_STW(ppc, R0, SDAREL(reg_cop0) + rd*4, R13);
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
	
	case 29: // TagHi
		// reg_cop0[rd] = 0
		GEN_STW(ppc, DYNAREG_ZERO, SDAREL(reg_cop0) + rd*4, R13);
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
	
	case 1: // Random
	case 8: // BadVAddr
	case 15: // PRevID
	case 27: // CacheErr
		// Do nothing
		return CONVERT_SUCCESS;
	
	case 9: // Count
	case 11: // Compare
	case 12: // Status
	default:
		genCallInterp(mips);
		return INTERPRETED;
	}
	
#endif
}

static int TLB(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_TLB
	genCallInterp(mips);
	return INTERPRETED;
#else
	return gen_tlb[mips&0x3f](mips);
#endif
}

static int (*gen_cop0[32])(MIPS_instr) =
{
   MFC0, NI, NI, NI, MTC0, NI, NI, NI,
   NI  , NI, NI, NI, NI  , NI, NI, NI,
   TLB , NI, NI, NI, NI  , NI, NI, NI,
   NI  , NI, NI, NI, NI  , NI, NI, NI
};

static int COP0(MIPS_instr mips){
#ifdef INTERPRET_COP0
	genCallInterp(mips);
	return INTERPRETED;
#else
	return gen_cop0[MIPS_GET_RS(mips)](mips);
#endif
}

// -- COP1 Instructions --

static int MFC1(MIPS_instr mips){
	PowerPC_instr ppc;
#if defined(INTERPRET_FP) || defined(INTERPRET_MFC1)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP

	genCheckFP();

	int fs = MIPS_GET_FS(mips);
	int rt = mapRegisterNew( MIPS_GET_RT(mips), 1 );
	flushFPR(fs);

	// r2 = reg_cop1_simple[fs]
	GEN_LWZ(ppc, R2, SDAREL(reg_cop1_simple) + fs*4, R13);
	set_next_dst(ppc);
	// rt = *r2
	GEN_LWZ(ppc, rt, 0, R2);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
#endif
}

static int DMFC1(MIPS_instr mips){
	PowerPC_instr ppc;
#if defined(INTERPRET_FP) || defined(INTERPRET_DMFC1)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP

	genCheckFP();

	int fs = MIPS_GET_FS(mips);
	RegMapping rt = mapRegister64New( MIPS_GET_RT(mips) );
	flushFPR(fs);

	// r2 = reg_cop1_double[fs]
	GEN_LWZ(ppc, R2, SDAREL(reg_cop1_double) + fs*4, R13);
	set_next_dst(ppc);
	// rt[hi] = *r2
	GEN_LWZ(ppc, rt.hi, 0, R2);
	set_next_dst(ppc);
	// rt[lo] = *(r2+4)
	GEN_LWZ(ppc, rt.lo, 4, R2);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
#endif
}

static int CFC1(MIPS_instr mips){
	PowerPC_instr ppc;
#if defined(INTERPRET_FP) || defined(INTERPRET_CFC1)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_CFC1

	genCheckFP();

	if(MIPS_GET_FS(mips) == 31){
		int rt = mapRegisterNew( MIPS_GET_RT(mips), 1 );

		GEN_LWZ(ppc, rt, SDAREL(FCR31), R13);
		set_next_dst(ppc);
	} else if(MIPS_GET_FS(mips) == 0){
		int rt = mapRegisterNew( MIPS_GET_RT(mips), 0 );

		GEN_LI(ppc, rt, 0x511);
		set_next_dst(ppc);
	}

	return CONVERT_SUCCESS;
#endif
}

static int MTC1(MIPS_instr mips){
	PowerPC_instr ppc;
#if defined(INTERPRET_FP) || defined(INTERPRET_MTC1)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP

	genCheckFP();

	int rt = mapRegister( MIPS_GET_RT(mips) );
	int fs = MIPS_GET_FS(mips);
	invalidateFPR(fs);

	// r2 = reg_cop1_simple[fs]
	GEN_LWZ(ppc, R2, SDAREL(reg_cop1_simple) + fs*4, R13);
	set_next_dst(ppc);
	// *r2 = rt
	GEN_STW(ppc, rt, 0, R2);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
#endif
}

static int DMTC1(MIPS_instr mips){
	PowerPC_instr ppc;
#if defined(INTERPRET_FP) || defined(INTERPRET_DMTC1)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP

	genCheckFP();

	RegMapping rt = mapRegister64( MIPS_GET_RT(mips) );
	int fs = MIPS_GET_FS(mips);
	invalidateFPR(fs);

	GEN_LWZ(ppc, R2, SDAREL(reg_cop1_double) + fs*4, R13);
	set_next_dst(ppc);
	GEN_STW(ppc, rt.hi, 0, R2);
	set_next_dst(ppc);
	GEN_STW(ppc, rt.lo, 4, R2);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
#endif
}

static int CTC1(MIPS_instr mips){
	PowerPC_instr ppc;
#if defined(INTERPRET_FP) || defined(INTERPRET_CTC1)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_CTC1

	genCheckFP();

	if(MIPS_GET_FS(mips) == 31){
		int rt = mapRegister( MIPS_GET_RT(mips) );

		GEN_STW(ppc, rt, SDAREL(FCR31), R13);
		set_next_dst(ppc);
	}

	return CONVERT_SUCCESS;
#endif
}

static int BC(MIPS_instr mips){
	PowerPC_instr ppc;
#if defined(INTERPRET_BC)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_BC

	if(CANT_COMPILE_DELAY()){
		genCallInterp(mips);
		return INTERPRETED;
	}

	genCheckFP();

	int cond   = mips & 0x00010000;
	int likely = mips & 0x00020000;

	GEN_LWZ(ppc, R0, SDAREL(FCR31), R13);
	set_next_dst(ppc);
	GEN_RLWINM(ppc, R0, R0, 9, 31, 31);
	set_next_dst(ppc);
	GEN_CMPI(ppc, CR4, R0, 0);
	set_next_dst(ppc);

	return branch(MIPS_GET_IMMED(mips), cond ? NE : EQ, 0, likely);
#endif
}

// -- Floating Point Arithmetic --
static int ADD_FP(MIPS_instr mips, int dbl){
	PowerPC_instr ppc;
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_ADD)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_ADD

	genCheckFP();

	int fs = mapFPR( MIPS_GET_FS(mips), dbl );
	int ft = mapFPR( MIPS_GET_FT(mips), dbl );
	int fd = mapFPRNew( MIPS_GET_FD(mips), dbl );

	GEN_FADD(ppc, fd, fs, ft, dbl);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
#endif
}

static int SUB_FP(MIPS_instr mips, int dbl){
	PowerPC_instr ppc;
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_SUB)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_SUB

	genCheckFP();

	int fs = mapFPR( MIPS_GET_FS(mips), dbl );
	int ft = mapFPR( MIPS_GET_FT(mips), dbl );
	int fd = mapFPRNew( MIPS_GET_FD(mips), dbl );

	GEN_FSUB(ppc, fd, fs, ft, dbl);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
#endif
}

static int MUL_FP(MIPS_instr mips, int dbl){
	PowerPC_instr ppc;
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_MUL)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_MUL

	genCheckFP();

	int fs = mapFPR( MIPS_GET_FS(mips), dbl );
	int ft = mapFPR( MIPS_GET_FT(mips), dbl );
	int fd = mapFPRNew( MIPS_GET_FD(mips), dbl );

	GEN_FMUL(ppc, fd, fs, ft, dbl);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
#endif
}

static int DIV_FP(MIPS_instr mips, int dbl){
	PowerPC_instr ppc;
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_DIV)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_DIV

	genCheckFP();

	int fs = mapFPR( MIPS_GET_FS(mips), dbl );
	int ft = mapFPR( MIPS_GET_FT(mips), dbl );
	int fd = mapFPRNew( MIPS_GET_FD(mips), dbl );

	GEN_FDIV(ppc, fd, fs, ft, dbl);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
#endif
}

static int SQRT_FP(MIPS_instr mips, int dbl){
	PowerPC_instr ppc;
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_SQRT)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_SQRT

	genCheckFP();

	flushRegisters();
	mapFPR( MIPS_GET_FS(mips), dbl ); // maps to f1 (FP argument)
	invalidateRegisters();

	// call sqrt
	GEN_B(ppc, add_jump(dbl ? &__ieee754_sqrt : &__ieee754_sqrtf, 1, 1), 0, 1);
	set_next_dst(ppc);

	mapFPRNew( MIPS_GET_FD(mips), dbl ); // maps to f1 (FP return)

	// Load old LR
	GEN_LWZ(ppc, R0, DYNAOFF_LR, R1);
	set_next_dst(ppc);
	// Restore LR
	GEN_MTLR(ppc, R0);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
#endif
}

static int ABS_FP(MIPS_instr mips, int dbl){
	PowerPC_instr ppc;
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_ABS)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_ABS

	genCheckFP();

	int fs = mapFPR( MIPS_GET_FS(mips), dbl );
	int fd = mapFPRNew( MIPS_GET_FD(mips), dbl );

	GEN_FABS(ppc, fd, fs);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
#endif
}

static int MOV_FP(MIPS_instr mips, int dbl){
	PowerPC_instr ppc;
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_MOV)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_MOV

	genCheckFP();

	int fs = mapFPR( MIPS_GET_FS(mips), dbl );
	int fd = mapFPRNew( MIPS_GET_FD(mips), dbl );

	GEN_FMR(ppc, fd, fs);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
#endif
}

static int NEG_FP(MIPS_instr mips, int dbl){
	PowerPC_instr ppc;
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_NEG)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_NEG

	genCheckFP();

	int fs = mapFPR( MIPS_GET_FS(mips), dbl );
	int fd = mapFPRNew( MIPS_GET_FD(mips), dbl );

	GEN_FNEG(ppc, fd, fs);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
#endif
}

// -- Floating Point Rounding/Conversion --
#define PPC_ROUNDING_NEAREST 0
#define PPC_ROUNDING_TRUNC   1
#define PPC_ROUNDING_CEIL    2
#define PPC_ROUNDING_FLOOR   3
static void set_rounding(int rounding_mode){
	PowerPC_instr ppc;

	GEN_MTFSFI(ppc, CR7, rounding_mode);
	set_next_dst(ppc);
}

static void set_rounding_reg(int fs){
	PowerPC_instr ppc;

	GEN_MTFSF(ppc, 1, fs);
	set_next_dst(ppc);
}

static int ROUND_L_FP(MIPS_instr mips, int dbl){
	PowerPC_instr ppc;
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_ROUND_L)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_ROUND_L
	
	genCheckFP();

	flushRegisters();
	int fd = MIPS_GET_FD(mips);
	int fs = mapFPR( MIPS_GET_FS(mips), dbl );
	invalidateFPR( MIPS_GET_FS(mips) );

	// round
	GEN_B(ppc, add_jump(dbl ? &round : &roundf, 1, 1), 0, 1);
	set_next_dst(ppc);
	// convert
	GEN_B(ppc, add_jump(dbl ? &__fixdfdi : &__fixsfdi, 1, 1), 0, 1);
	set_next_dst(ppc);

	// r2 = reg_cop1_double[fd]
	GEN_LWZ(ppc, R2, SDAREL(reg_cop1_double) + fd*4, R13);
	set_next_dst(ppc);
	// Load old LR
	GEN_LWZ(ppc, R0, DYNAOFF_LR, R1);
	set_next_dst(ppc);
	// stw r3, 0(r2)
	GEN_STW(ppc, R3, 0, R2);
	set_next_dst(ppc);
	// Restore LR
	GEN_MTLR(ppc, R0);
	set_next_dst(ppc);
	// stw r4, 4(r2)
	GEN_STW(ppc, R4, 4, R2);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
#endif
}

static int TRUNC_L_FP(MIPS_instr mips, int dbl){
	PowerPC_instr ppc;
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_TRUNC_L)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_TRUNC_L

	genCheckFP();

	flushRegisters();
	int fd = MIPS_GET_FD(mips);
	int fs = mapFPR( MIPS_GET_FS(mips), dbl );
	invalidateFPR( MIPS_GET_FS(mips) );

	// convert
	GEN_B(ppc, add_jump(dbl ? &__fixdfdi : &__fixsfdi, 1, 1), 0, 1);
	set_next_dst(ppc);

	// r2 = reg_cop1_double[fd]
	GEN_LWZ(ppc, R2, SDAREL(reg_cop1_double) + fd*4, R13);
	set_next_dst(ppc);
	// Load old LR
	GEN_LWZ(ppc, R0, DYNAOFF_LR, R1);
	set_next_dst(ppc);
	// stw r3, 0(r2)
	GEN_STW(ppc, R3, 0, R2);
	set_next_dst(ppc);
	// Restore LR
	GEN_MTLR(ppc, R0);
	set_next_dst(ppc);
	// stw r4, 4(r2)
	GEN_STW(ppc, R4, 4, R2);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
#endif
}

static int CEIL_L_FP(MIPS_instr mips, int dbl){
	PowerPC_instr ppc;
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_CEIL_L)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_CEIL_L

	genCheckFP();

	flushRegisters();
	int fd = MIPS_GET_FD(mips);
	int fs = mapFPR( MIPS_GET_FS(mips), dbl );
	invalidateFPR( MIPS_GET_FS(mips) );

	// ceil
	GEN_B(ppc, add_jump(dbl ? &ceil : &ceilf, 1, 1), 0, 1);
	set_next_dst(ppc);
	// convert
	GEN_B(ppc, add_jump(dbl ? &__fixdfdi : &__fixsfdi, 1, 1), 0, 1);
	set_next_dst(ppc);

	// r2 = reg_cop1_double[fd]
	GEN_LWZ(ppc, R2, SDAREL(reg_cop1_double) + fd*4, R13);
	set_next_dst(ppc);
	// Load old LR
	GEN_LWZ(ppc, R0, DYNAOFF_LR, R1);
	set_next_dst(ppc);
	// stw r3, 0(r2)
	GEN_STW(ppc, R3, 0, R2);
	set_next_dst(ppc);
	// Restore LR
	GEN_MTLR(ppc, R0);
	set_next_dst(ppc);
	// stw r4, 4(r2)
	GEN_STW(ppc, R4, 4, R2);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
#endif
}

static int FLOOR_L_FP(MIPS_instr mips, int dbl){
	PowerPC_instr ppc;
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_FLOOR_L)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_FLOOR_L

	genCheckFP();

	flushRegisters();
	int fd = MIPS_GET_FD(mips);
	int fs = mapFPR( MIPS_GET_FS(mips), dbl );
	invalidateFPR( MIPS_GET_FS(mips) );

	// round
	GEN_B(ppc, add_jump(dbl ? &floor : &floorf, 1, 1), 0, 1);
	set_next_dst(ppc);
	// convert
	GEN_B(ppc, add_jump(dbl ? &__fixdfdi : &__fixsfdi, 1, 1), 0, 1);
	set_next_dst(ppc);

	// r2 = reg_cop1_double[fd]
	GEN_LWZ(ppc, R2, SDAREL(reg_cop1_double) + fd*4, R13);
	set_next_dst(ppc);
	// Load old LR
	GEN_LWZ(ppc, R0, DYNAOFF_LR, R1);
	set_next_dst(ppc);
	// stw r3, 0(r2)
	GEN_STW(ppc, R3, 0, R2);
	set_next_dst(ppc);
	// Restore LR
	GEN_MTLR(ppc, R0);
	set_next_dst(ppc);
	// stw r4, 4(r2)
	GEN_STW(ppc, R4, 4, R2);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
#endif
}

static int ROUND_W_FP(MIPS_instr mips, int dbl){
	PowerPC_instr ppc;
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_ROUND_W)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_ROUND_W

	genCheckFP();

	int fd = MIPS_GET_FD(mips);
	int fs = mapFPR( MIPS_GET_FS(mips), dbl );
	invalidateFPR(fd);

	// fctiw f0, fs
	GEN_FCTIW(ppc, F0, fs);
	set_next_dst(ppc);
	// r0 = reg_cop1_simple[fd]
	GEN_LWZ(ppc, R0, SDAREL(reg_cop1_simple) + fd*4, R13);
	set_next_dst(ppc);
	// stfiwx f0, 0, r0
	GEN_STFIWX(ppc, F0, 0, R0);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
#endif
}

static int TRUNC_W_FP(MIPS_instr mips, int dbl){
	PowerPC_instr ppc;
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_TRUNC_W)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_TRUNC_W

	genCheckFP();

	int fd = MIPS_GET_FD(mips);
	int fs = mapFPR( MIPS_GET_FS(mips), dbl );
	invalidateFPR(fd);

	// fctiwz f0, fs
	GEN_FCTIWZ(ppc, F0, fs);
	set_next_dst(ppc);
	// r0 = reg_cop1_simple[fd]
	GEN_LWZ(ppc, R0, SDAREL(reg_cop1_simple) + fd*4, R13);
	set_next_dst(ppc);
	// stfiwx f0, 0, r0
	GEN_STFIWX(ppc, F0, 0, R0);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
#endif
}

static int CEIL_W_FP(MIPS_instr mips, int dbl){
	PowerPC_instr ppc;
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_CEIL_W)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_CEIL_W

	genCheckFP();

	set_rounding(PPC_ROUNDING_CEIL);

	int fd = MIPS_GET_FD(mips);
	int fs = mapFPR( MIPS_GET_FS(mips), dbl );
	invalidateFPR(fd);

	// fctiw f0, fs
	GEN_FCTIW(ppc, F0, fs);
	set_next_dst(ppc);
	// r0 = reg_cop1_simple[fd]
	GEN_LWZ(ppc, R0, SDAREL(reg_cop1_simple) + fd*4, R13);
	set_next_dst(ppc);
	// stfiwx f0, 0, r0
	GEN_STFIWX(ppc, F0, 0, R0);
	set_next_dst(ppc);

	set_rounding(PPC_ROUNDING_NEAREST);

	return CONVERT_SUCCESS;
#endif
}

static int FLOOR_W_FP(MIPS_instr mips, int dbl){
	PowerPC_instr ppc;
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_FLOOR_W)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_FLOOR_W

	genCheckFP();

	set_rounding(PPC_ROUNDING_FLOOR);

	int fd = MIPS_GET_FD(mips);
	int fs = mapFPR( MIPS_GET_FS(mips), dbl );
	invalidateFPR(fd);

	// fctiw f0, fs
	GEN_FCTIW(ppc, F0, fs);
	set_next_dst(ppc);
	// r0 = reg_cop1_simple[fd]
	GEN_LWZ(ppc, R0, SDAREL(reg_cop1_simple) + fd*4, R13);
	set_next_dst(ppc);
	// stfiwx f0, 0, r0
	GEN_STFIWX(ppc, F0, 0, R0);
	set_next_dst(ppc);

	set_rounding(PPC_ROUNDING_NEAREST);

	return CONVERT_SUCCESS;
#endif
}

static int CVT_S_FP(MIPS_instr mips, int dbl){
	PowerPC_instr ppc;
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_CVT_S)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_CVT_S

	genCheckFP();

	int fs = mapFPR( MIPS_GET_FS(mips), dbl );
	int fd = mapFPRNew( MIPS_GET_FD(mips), 0 );

	GEN_FRSP(ppc, fd, fs);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
#endif
}

static int CVT_D_FP(MIPS_instr mips, int dbl){
	PowerPC_instr ppc;
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_CVT_D)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_CVT_D

	genCheckFP();

	int fs = mapFPR( MIPS_GET_FS(mips), dbl );
	int fd = mapFPRNew( MIPS_GET_FD(mips), 1 );

	GEN_FMR(ppc, fd, fs);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
#endif
}

static int CVT_W_FP(MIPS_instr mips, int dbl){
	PowerPC_instr ppc;
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_CVT_W)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_CVT_W

	genCheckFP();

	// Set rounding mode according to FCR31
	GEN_LFD(ppc, F0, SDAREL(FCR31-1), R13);
	set_next_dst(ppc);

	// FIXME: Here I have the potential to disable IEEE mode
	//          and enable inexact exceptions
	set_rounding_reg(F0);

	int fd = MIPS_GET_FD(mips);
	int fs = mapFPR( MIPS_GET_FS(mips), dbl );
	invalidateFPR(fd);

	// fctiw f0, fs
	GEN_FCTIW(ppc, F0, fs);
	set_next_dst(ppc);
	// r0 = reg_cop1_simple[fd]
	GEN_LWZ(ppc, R0, SDAREL(reg_cop1_simple) + fd*4, R13);
	set_next_dst(ppc);
	// stfiwx f0, 0, r0
	GEN_STFIWX(ppc, F0, 0, R0);
	set_next_dst(ppc);

	set_rounding(PPC_ROUNDING_NEAREST);

	return CONVERT_SUCCESS;
#endif
}

static int CVT_L_FP(MIPS_instr mips, int dbl){
	PowerPC_instr ppc;
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_CVT_L)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_CVT_L

	genCheckFP();

	flushRegisters();
	int fd = MIPS_GET_FD(mips);
	int fs = mapFPR( MIPS_GET_FS(mips), dbl );
	invalidateFPR( MIPS_GET_FS(mips) );

	// FIXME: I'm fairly certain this will always trunc
	// convert
	GEN_B(ppc, add_jump(dbl ? &__fixdfdi : &__fixsfdi, 1, 1), 0, 1);
	set_next_dst(ppc);

	// r2 = reg_cop1_double[fd]
	GEN_LWZ(ppc, R2, SDAREL(reg_cop1_double) + fd*4, R13);
	set_next_dst(ppc);
	// Load old LR
	GEN_LWZ(ppc, R0, DYNAOFF_LR, R1);
	set_next_dst(ppc);
	// stw r3, 0(r2)
	GEN_STW(ppc, R3, 0, R2);
	set_next_dst(ppc);
	// Restore LR
	GEN_MTLR(ppc, R0);
	set_next_dst(ppc);
	// stw r4, 4(r2)
	GEN_STW(ppc, R4, 4, R2);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
#endif
}

// -- Floating Point Comparisons --
static int C_F_FP(MIPS_instr mips, int dbl){
	PowerPC_instr ppc;
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_C_F)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_C_F

	genCheckFP();

	// lwz r0, 0(&fcr31)
	GEN_LWZ(ppc, R0, SDAREL(FCR31), R13);
	set_next_dst(ppc);
	// and r0, r0, 0xff7fffff (clear cond)
	GEN_RLWINM(ppc, R0, R0, 0, 9, 7);
	set_next_dst(ppc);
	// stw r0, 0(&fcr31)
	GEN_STW(ppc, R0, SDAREL(FCR31), R13);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
#endif
}

static int C_UN_FP(MIPS_instr mips, int dbl){
	PowerPC_instr ppc;
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_C_UN)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_C_UN

	genCheckFP();

	int fs = mapFPR( MIPS_GET_FS(mips), dbl );
	int ft = mapFPR( MIPS_GET_FT(mips), dbl );

	GEN_FCMPU(ppc, CR1, fs, ft);
	set_next_dst(ppc);
	GEN_LWZ(ppc, R0, SDAREL(FCR31), R13);
	set_next_dst(ppc);
	GEN_MFCR(ppc, R2);
	set_next_dst(ppc);
	GEN_RLWIMI(ppc, R0, R2, 31, 8, 8);
	set_next_dst(ppc);
	GEN_STW(ppc, R0, SDAREL(FCR31), R13);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
#endif
}

static int C_EQ_FP(MIPS_instr mips, int dbl){
	PowerPC_instr ppc;
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_C_EQ)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_C_EQ

	genCheckFP();

	int fs = mapFPR( MIPS_GET_FS(mips), dbl );
	int ft = mapFPR( MIPS_GET_FT(mips), dbl );

	GEN_FCMPU(ppc, CR1, fs, ft);
	set_next_dst(ppc);
	GEN_LWZ(ppc, R0, SDAREL(FCR31), R13);
	set_next_dst(ppc);
	GEN_MFCR(ppc, R2);
	set_next_dst(ppc);
	GEN_RLWIMI(ppc, R0, R2, 30, 8, 8);
	set_next_dst(ppc);
	GEN_STW(ppc, R0, SDAREL(FCR31), R13);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
#endif
}

static int C_UEQ_FP(MIPS_instr mips, int dbl){
	PowerPC_instr ppc;
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_C_UEQ)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_C_UEQ

	genCheckFP();

	int fs = mapFPR( MIPS_GET_FS(mips), dbl );
	int ft = mapFPR( MIPS_GET_FT(mips), dbl );

	GEN_FCMPU(ppc, CR1, fs, ft);
	set_next_dst(ppc);
	GEN_LWZ(ppc, R0, SDAREL(FCR31), R13);
	set_next_dst(ppc);
	GEN_CROR(ppc, CR1*4+2, CR1*4+3, CR1*4+2);
	set_next_dst(ppc);
	GEN_MFCR(ppc, R2);
	set_next_dst(ppc);
	GEN_RLWIMI(ppc, R0, R2, 30, 8, 8);
	set_next_dst(ppc);
	GEN_STW(ppc, R0, SDAREL(FCR31), R13);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
#endif
}

static int C_OLT_FP(MIPS_instr mips, int dbl){
	PowerPC_instr ppc;
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_C_OLT)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_C_OLT

	genCheckFP();

	int fs = mapFPR( MIPS_GET_FS(mips), dbl );
	int ft = mapFPR( MIPS_GET_FT(mips), dbl );

	GEN_FCMPU(ppc, CR1, fs, ft);
	set_next_dst(ppc);
	GEN_LWZ(ppc, R0, SDAREL(FCR31), R13);
	set_next_dst(ppc);
	GEN_MFCR(ppc, R2);
	set_next_dst(ppc);
	GEN_RLWIMI(ppc, R0, R2, 28, 8, 8);
	set_next_dst(ppc);
	GEN_STW(ppc, R0, SDAREL(FCR31), R13);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
#endif
}

static int C_ULT_FP(MIPS_instr mips, int dbl){
	PowerPC_instr ppc;
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_C_ULT)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_C_ULT

	genCheckFP();

	int fs = mapFPR( MIPS_GET_FS(mips), dbl );
	int ft = mapFPR( MIPS_GET_FT(mips), dbl );

	GEN_FCMPU(ppc, CR1, fs, ft);
	set_next_dst(ppc);
	GEN_LWZ(ppc, R0, SDAREL(FCR31), R13);
	set_next_dst(ppc);
	GEN_CROR(ppc, CR1*4+0, CR1*4+3, CR1*4+0);
	set_next_dst(ppc);
	GEN_MFCR(ppc, R2);
	set_next_dst(ppc);
	GEN_RLWIMI(ppc, R0, R2, 28, 8, 8);
	set_next_dst(ppc);
	GEN_STW(ppc, R0, SDAREL(FCR31), R13);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
#endif
}

static int C_OLE_FP(MIPS_instr mips, int dbl){
	PowerPC_instr ppc;
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_C_OLE)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_C_OLE

	genCheckFP();

	int fs = mapFPR( MIPS_GET_FS(mips), dbl );
	int ft = mapFPR( MIPS_GET_FT(mips), dbl );

	GEN_FCMPU(ppc, CR1, fs, ft);
	set_next_dst(ppc);
	GEN_LWZ(ppc, R0, SDAREL(FCR31), R13);
	set_next_dst(ppc);
	GEN_CRNOR(ppc, CR1*4+2, CR1*4+3, CR1*4+1);
	set_next_dst(ppc);
	GEN_MFCR(ppc, R2);
	set_next_dst(ppc);
	GEN_RLWIMI(ppc, R0, R2, 30, 8, 8);
	set_next_dst(ppc);
	GEN_STW(ppc, R0, SDAREL(FCR31), R13);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
#endif
}

static int C_ULE_FP(MIPS_instr mips, int dbl){
	PowerPC_instr ppc;
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_C_ULE)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_C_ULE

	genCheckFP();

	int fs = mapFPR( MIPS_GET_FS(mips), dbl );
	int ft = mapFPR( MIPS_GET_FT(mips), dbl );

	GEN_FCMPU(ppc, CR1, fs, ft);
	set_next_dst(ppc);
	GEN_LWZ(ppc, R0, SDAREL(FCR31), R13);
	set_next_dst(ppc);
	GEN_CRORC(ppc, CR1*4+2, CR1*4+3, CR1*4+1);
	set_next_dst(ppc);
	GEN_MFCR(ppc, R2);
	set_next_dst(ppc);
	GEN_RLWIMI(ppc, R0, R2, 30, 8, 8);
	set_next_dst(ppc);
	GEN_STW(ppc, R0, SDAREL(FCR31), R13);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
#endif
}

static int C_SF_FP(MIPS_instr mips, int dbl){
	PowerPC_instr ppc;
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_C_SF)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_C_SF

	genCheckFP();

	// lwz r0, 0(&fcr31)
	GEN_LWZ(ppc, R0, SDAREL(FCR31), R13);
	set_next_dst(ppc);
	// and r0, r0, 0xff7fffff (clear cond)
	GEN_RLWINM(ppc, R0, R0, 0, 9, 7);
	set_next_dst(ppc);
	// stw r0, 0(&fcr31)
	GEN_STW(ppc, R0, SDAREL(FCR31), R13);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
#endif
}

static int C_NGLE_FP(MIPS_instr mips, int dbl){
	PowerPC_instr ppc;
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_C_NGLE)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_C_NGLE

	genCheckFP();

	// lwz r0, 0(&fcr31)
	GEN_LWZ(ppc, R0, SDAREL(FCR31), R13);
	set_next_dst(ppc);
	// and r0, r0, 0xff7fffff (clear cond)
	GEN_RLWINM(ppc, R0, R0, 0, 9, 7);
	set_next_dst(ppc);
	// stw r0, 0(&fcr31)
	GEN_STW(ppc, R0, SDAREL(FCR31), R13);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
#endif
}

static int C_SEQ_FP(MIPS_instr mips, int dbl){
	PowerPC_instr ppc;
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_C_SEQ)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_C_SEQ

	genCheckFP();

	int fs = mapFPR( MIPS_GET_FS(mips), dbl );
	int ft = mapFPR( MIPS_GET_FT(mips), dbl );

	GEN_FCMPU(ppc, CR1, fs, ft);
	set_next_dst(ppc);
	GEN_LWZ(ppc, R0, SDAREL(FCR31), R13);
	set_next_dst(ppc);
	GEN_MFCR(ppc, R2);
	set_next_dst(ppc);
	GEN_RLWIMI(ppc, R0, R2, 30, 8, 8);
	set_next_dst(ppc);
	GEN_STW(ppc, R0, SDAREL(FCR31), R13);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
#endif
}

static int C_NGL_FP(MIPS_instr mips, int dbl){
	PowerPC_instr ppc;
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_C_NGL)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_C_NGL

	genCheckFP();

	int fs = mapFPR( MIPS_GET_FS(mips), dbl );
	int ft = mapFPR( MIPS_GET_FT(mips), dbl );

	GEN_FCMPU(ppc, CR1, fs, ft);
	set_next_dst(ppc);
	GEN_LWZ(ppc, R0, SDAREL(FCR31), R13);
	set_next_dst(ppc);
	GEN_MFCR(ppc, R2);
	set_next_dst(ppc);
	GEN_RLWIMI(ppc, R0, R2, 30, 8, 8);
	set_next_dst(ppc);
	GEN_STW(ppc, R0, SDAREL(FCR31), R13);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
#endif
}

static int C_LT_FP(MIPS_instr mips, int dbl){
	PowerPC_instr ppc;
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_C_LT)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_C_LT

	genCheckFP();

	int fs = mapFPR( MIPS_GET_FS(mips), dbl );
	int ft = mapFPR( MIPS_GET_FT(mips), dbl );

	GEN_FCMPU(ppc, CR1, fs, ft);
	set_next_dst(ppc);
	GEN_LWZ(ppc, R0, SDAREL(FCR31), R13);
	set_next_dst(ppc);
	GEN_MFCR(ppc, R2);
	set_next_dst(ppc);
	GEN_RLWIMI(ppc, R0, R2, 28, 8, 8);
	set_next_dst(ppc);
	GEN_STW(ppc, R0, SDAREL(FCR31), R13);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
#endif
}

static int C_NGE_FP(MIPS_instr mips, int dbl){
	PowerPC_instr ppc;
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_C_NGE)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_C_NGE

	genCheckFP();

	int fs = mapFPR( MIPS_GET_FS(mips), dbl );
	int ft = mapFPR( MIPS_GET_FT(mips), dbl );

	GEN_FCMPU(ppc, CR1, fs, ft);
	set_next_dst(ppc);
	GEN_LWZ(ppc, R0, SDAREL(FCR31), R13);
	set_next_dst(ppc);
	GEN_MFCR(ppc, R2);
	set_next_dst(ppc);
	GEN_RLWIMI(ppc, R0, R2, 28, 8, 8);
	set_next_dst(ppc);
	GEN_STW(ppc, R0, SDAREL(FCR31), R13);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
#endif
}

static int C_LE_FP(MIPS_instr mips, int dbl){
	PowerPC_instr ppc;
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_C_LE)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_C_LE

	genCheckFP();

	int fs = mapFPR( MIPS_GET_FS(mips), dbl );
	int ft = mapFPR( MIPS_GET_FT(mips), dbl );

	GEN_FCMPU(ppc, CR1, fs, ft);
	set_next_dst(ppc);
	GEN_LWZ(ppc, R0, SDAREL(FCR31), R13);
	set_next_dst(ppc);
	GEN_CRNOR(ppc, CR1*4+2, CR1*4+3, CR1*4+1);
	set_next_dst(ppc);
	GEN_MFCR(ppc, R2);
	set_next_dst(ppc);
	GEN_RLWIMI(ppc, R0, R2, 30, 8, 8);
	set_next_dst(ppc);
	GEN_STW(ppc, R0, SDAREL(FCR31), R13);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
#endif
}

static int C_NGT_FP(MIPS_instr mips, int dbl){
	PowerPC_instr ppc;
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_C_NGT)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_C_NGT

	genCheckFP();

	int fs = mapFPR( MIPS_GET_FS(mips), dbl );
	int ft = mapFPR( MIPS_GET_FT(mips), dbl );

	GEN_FCMPU(ppc, CR1, fs, ft);
	set_next_dst(ppc);
	GEN_LWZ(ppc, R0, SDAREL(FCR31), R13);
	set_next_dst(ppc);
	GEN_CRNOR(ppc, CR1*4+2, CR1*4+3, CR1*4+1);
	set_next_dst(ppc);
	GEN_MFCR(ppc, R2);
	set_next_dst(ppc);
	GEN_RLWIMI(ppc, R0, R2, 30, 8, 8);
	set_next_dst(ppc);
	GEN_STW(ppc, R0, SDAREL(FCR31), R13);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
#endif
}

static int (*gen_cop1_fp[64])(MIPS_instr, int) =
{
   ADD_FP    ,SUB_FP    ,MUL_FP   ,DIV_FP    ,SQRT_FP   ,ABS_FP    ,MOV_FP   ,NEG_FP    ,
   ROUND_L_FP,TRUNC_L_FP,CEIL_L_FP,FLOOR_L_FP,ROUND_W_FP,TRUNC_W_FP,CEIL_W_FP,FLOOR_W_FP,
   NI        ,NI        ,NI       ,NI        ,NI        ,NI        ,NI       ,NI        ,
   NI        ,NI        ,NI       ,NI        ,NI        ,NI        ,NI       ,NI        ,
   CVT_S_FP  ,CVT_D_FP  ,NI       ,NI        ,CVT_W_FP  ,CVT_L_FP  ,NI       ,NI        ,
   NI        ,NI        ,NI       ,NI        ,NI        ,NI        ,NI       ,NI        ,
   C_F_FP    ,C_UN_FP   ,C_EQ_FP  ,C_UEQ_FP  ,C_OLT_FP  ,C_ULT_FP  ,C_OLE_FP ,C_ULE_FP  ,
   C_SF_FP   ,C_NGLE_FP ,C_SEQ_FP ,C_NGL_FP  ,C_LT_FP   ,C_NGE_FP  ,C_LE_FP  ,C_NGT_FP
};

static int S(MIPS_instr mips){
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_S)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_S
	return gen_cop1_fp[ MIPS_GET_FUNC(mips) ](mips, 0);
#endif
}

static int D(MIPS_instr mips){
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_D)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_D
	return gen_cop1_fp[ MIPS_GET_FUNC(mips) ](mips, 1);
#endif
}

static const float two16 = 0x1p16;

static int CVT_FP_W(MIPS_instr mips, int dbl){
	PowerPC_instr ppc;

	genCheckFP();

	int fs = MIPS_GET_FS(mips);
	flushFPR(fs);
	int fd = mapFPRNew( MIPS_GET_FD(mips), dbl );
	int tmp = mapFPRTemp();

	GEN_LWZ(ppc, R2, SDAREL(reg_cop1_simple) + fs*4, R13);
	set_next_dst(ppc);
	GEN_LFS(ppc, F0, SDAREL(two16), R13);
	set_next_dst(ppc);
	GEN_PSQ_L(ppc, fd, 0, R2, QR7);
	set_next_dst(ppc);
	GEN_PSQ_L(ppc, tmp, 2, R2, QR5);
	set_next_dst(ppc);
	GEN_FMADD(ppc, fd, fd, F0, tmp, dbl);
	set_next_dst(ppc);

	unmapFPRTemp(tmp);

	return CONVERT_SUCCESS;
}

static int W(MIPS_instr mips){
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_W)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_W

	int func = MIPS_GET_FUNC(mips);

	if(func == MIPS_FUNC_CVT_S_) return CVT_FP_W(mips, 0);
	if(func == MIPS_FUNC_CVT_D_) return CVT_FP_W(mips, 1);
	else return CONVERT_ERROR;
#endif
}

static int CVT_FP_L(MIPS_instr mips, int dbl){
	PowerPC_instr ppc;
	
	genCheckFP();

	int fs = MIPS_GET_FS(mips);
	flushFPR(fs);
	int fd = mapFPRNew( MIPS_GET_FD(mips), dbl );
	int tmp = mapFPRTemp();

	GEN_LWZ(ppc, R2, SDAREL(reg_cop1_double) + fs*4, R13);
	set_next_dst(ppc);
	GEN_LFS(ppc, F0, SDAREL(two16), R13);
	set_next_dst(ppc);
	GEN_PSQ_L(ppc, fd, 0, R2, QR7);
	set_next_dst(ppc);
	GEN_PSQ_L(ppc, tmp, 2, R2, QR5);
	set_next_dst(ppc);
	GEN_FMADD(ppc, fd, fd, F0, tmp, dbl);
	set_next_dst(ppc);
	GEN_PSQ_L(ppc, tmp, 4, R2, QR5);
	set_next_dst(ppc);
	GEN_FMADD(ppc, fd, fd, F0, tmp, dbl);
	set_next_dst(ppc);
	GEN_PSQ_L(ppc, tmp, 6, R2, QR5);
	set_next_dst(ppc);
	GEN_FMADD(ppc, fd, fd, F0, tmp, dbl);
	set_next_dst(ppc);

	unmapFPRTemp(tmp);

	return CONVERT_SUCCESS;
}

static int L(MIPS_instr mips){
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_L)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_L

	int func = MIPS_GET_FUNC(mips);

	if(func == MIPS_FUNC_CVT_S_) return CVT_FP_L(mips, 0);
	if(func == MIPS_FUNC_CVT_D_) return CVT_FP_L(mips, 1);
	else return CONVERT_ERROR;
#endif
}

static int (*gen_cop1[32])(MIPS_instr) =
{
   MFC1, DMFC1, CFC1, NI, MTC1, DMTC1, CTC1, NI,
   BC  , NI   , NI  , NI, NI  , NI   , NI  , NI,
   S   , D    , NI  , NI, W   , L    , NI  , NI,
   NI  , NI   , NI  , NI, NI  , NI   , NI  , NI
};

static int COP1(MIPS_instr mips){
	return gen_cop1[MIPS_GET_RS(mips)](mips);
}

static int (*gen_ops[64])(MIPS_instr) =
{
   SPECIAL, REGIMM, J   , JAL  , BEQ , BNE , BLEZ , BGTZ ,
   ADDI   , ADDIU , SLTI, SLTIU, ANDI, ORI , XORI , LUI  ,
   COP0   , COP1  , NI  , NI   , BEQL, BNEL, BLEZL, BGTZL,
   DADDI  , DADDIU, LDL , LDR  , NI  , NI  , NI   , NI   ,
   LB     , LH    , LWL , LW   , LBU , LHU , LWR  , LWU  ,
   SB     , SH    , SWL , SW   , SDL , SDR , SWR  , CACHE,
   LL     , LWC1  , NI  , NI   , NI  , LDC1, NI   , LD   ,
   SC     , SWC1  , NI  , NI   , NI  , SDC1, NI   , SD
};



static void genCallInterp(MIPS_instr mips){
	PowerPC_instr ppc = NEW_PPC_INSTR();
	flushRegisters();
	reset_code_addr();
	// Pass in whether this instruction is in the delay slot
	GEN_LI(ppc, R5, isDelaySlot);
	set_next_dst(ppc);
	// Load our argument into r3 (mips)
	GEN_LIS(ppc, R3, mips>>16);
	set_next_dst(ppc);
	// Load the current PC as the second arg
	GEN_LIS(ppc, R4, get_src_pc()>>16);
	set_next_dst(ppc);
	// Load the lower halves of mips and PC
	GEN_ORI(ppc, R3, R3, mips);
	set_next_dst(ppc);
	GEN_ORI(ppc, R4, R4, get_src_pc());
	set_next_dst(ppc);
	// Branch to decodeNInterpret
	GEN_B(ppc, add_jump(&decodeNInterpret, 1, 1), 0, 1);
	set_next_dst(ppc);
	// Load the old LR
	GEN_LWZ(ppc, R0, DYNAOFF_LR, R1);
	set_next_dst(ppc);
	// Check if the PC changed
	GEN_CMPI(ppc, CR5, R3, 0);
	set_next_dst(ppc);
	// Restore the LR
	GEN_MTLR(ppc, R0);
	set_next_dst(ppc);
	// if decodeNInterpret returned an address
	//   jumpTo it
	GEN_BNELR(ppc, CR5, 0);
	set_next_dst(ppc);

	if(mips_is_jump(mips)) delaySlotNext = 2;
}

static void genJumpTo(unsigned int loc, unsigned int type){
	PowerPC_instr ppc = NEW_PPC_INSTR();

	if(type == JUMPTO_REG){
		// Load the register as the return value
		GEN_LWZ(ppc, R3, loc, R13);
		set_next_dst(ppc);
		GEN_BLR(ppc, 0);
		set_next_dst(ppc);
	} else {
		// Calculate the destination address
		loc <<= 2;
		if(type == JUMPTO_OFF) loc += get_src_pc();
		else loc |= get_src_pc() & 0xf0000000;
		// Create space to load destination func*
		set_next_dst(PPC_NOP);
		set_next_dst(PPC_NOP);
		// Make this function the LRU
		GEN_LWZ(ppc, R2, SDAREL(nextLRU), R13);
		set_next_dst(ppc);
		GEN_STW(ppc, R2, offsetof(PowerPC_func, lru), DYNAREG_FUNC);
		set_next_dst(ppc);
		GEN_ADDI(ppc, R0, R2, 1);
		set_next_dst(ppc);
		GEN_STW(ppc, R0, SDAREL(nextLRU), R13);
		set_next_dst(ppc);
		// Load the address as the return value
		GEN_LIS(ppc, R3, loc >> 16);
		set_next_dst(ppc);
		GEN_ORI(ppc, R3, R3, loc);
		set_next_dst(ppc);
		// Since we could be linking, return on interrupt
		GEN_BLELR(ppc, CR3, 0);
		set_next_dst(ppc);
		// Store last_addr for linking
		GEN_STW(ppc, R3, SDAREL(last_addr), R13);
		set_next_dst(ppc);
		GEN_BLR(ppc, 1);
		set_next_dst(ppc);
	}
}

// Updates Count, and sets cr3 to (next_interupt ? Count)
static void genUpdateCount(int checkCount){
	PowerPC_instr ppc = NEW_PPC_INSTR();
#ifndef COMPARE_CORE
	// Dynarec inlined code equivalent:
	// lis    r2,  pc >> 16
	GEN_LIS(ppc, R2, (get_src_pc()+4)>>16);
	set_next_dst(ppc);
	// lwz    r0,  0(&last_addr)     // r0 = last_addr
	GEN_LWZ(ppc, R0, SDAREL(last_addr), R13);
	set_next_dst(ppc);
	// ori    r2,  r2,  pc & 0xffff  // r2 = pc
	GEN_ORI(ppc, R2, R2, get_src_pc()+4);
	set_next_dst(ppc);
	// stw    r2,  0(&last_addr)     // last_addr = pc
	GEN_STW(ppc, R2, SDAREL(last_addr), R13);
	set_next_dst(ppc);
	// subf   r0,  r0,  r2           // r0 = pc - last_addr
	GEN_SUBF(ppc, R0, R0, R2);
	set_next_dst(ppc);
	// lwz    r2,  9*4(reg_cop0)     // r2 = Count
	GEN_LWZ(ppc, R2, SDAREL(Count), R13);
	set_next_dst(ppc);
	// srwi r0, r0, 1                // r0 = (pc - last_addr)/2
	GEN_SRWI(ppc, R0, R0, 1);
	set_next_dst(ppc);
	// add    r0,  r0,  r2           // r0 += Count
	GEN_ADD(ppc, R0, R0, R2);
	set_next_dst(ppc);
	if(checkCount){
		// lwz    r2,  0(&next_interupt) // r2 = next_interupt
		GEN_LWZ(ppc, R2, SDAREL(next_interupt), R13);
		set_next_dst(ppc);
	}
	// stw    r0,  9*4(reg_cop0)    // Count = r0
	GEN_STW(ppc, R0, SDAREL(Count), R13);
	set_next_dst(ppc);
	if(checkCount){
		// cmpl   cr3,  r2,  r0  // cr3 = next_interupt ? Count
		GEN_CMPL(ppc, CR3, R2, R0);
		set_next_dst(ppc);
	}
#else
	// Load the current PC as the argument
	GEN_LIS(ppc, R3, (get_src_pc()+4)>>16);
	set_next_dst(ppc);
	GEN_ORI(ppc, R3, R3, get_src_pc()+4);
	set_next_dst(ppc);
	// Call dyna_update_count
	GEN_B(ppc, add_jump(&dyna_update_count, 1, 1), 0, 1);
	set_next_dst(ppc);
	// Load the lr
	GEN_LWZ(ppc, R0, DYNAOFF_LR, R1);
	set_next_dst(ppc);
	GEN_MTLR(ppc, R0);
	set_next_dst(ppc);
	if(checkCount){
		// If next_interupt <= Count (cr3)
		GEN_CMPI(ppc, CR3, R3, 0);
		set_next_dst(ppc);
	}
#endif
}

// Check whether we need to take a FP unavailable exception
static void genCheckFP(void){
	PowerPC_instr ppc;
	if(FP_need_check || isDelaySlot){
		flushRegisters();
		reset_code_addr();
		// lwz r0, 12*4(reg_cop0)
		GEN_LWZ(ppc, R0, SDAREL(Status), R13);
		set_next_dst(ppc);
		// andis. r0, r0, 0x2000
		GEN_ANDIS(ppc, R0, R0, 0x2000);
		set_next_dst(ppc);
		// bne cr0, end
		GEN_BNE(ppc, CR0, 8, 0, 0);
		set_next_dst(ppc);
		// Load the current PC as arg 1 (upper half)
		GEN_LIS(ppc, R3, get_src_pc()>>16);
		set_next_dst(ppc);
		// Pass in whether this instruction is in the delay slot as arg 2
		GEN_LI(ppc, R4, isDelaySlot);
		set_next_dst(ppc);
		// Current PC (lower half)
		GEN_ORI(ppc, R3, R3, get_src_pc());
		set_next_dst(ppc);
		// Call dyna_check_cop1_unusable
		GEN_B(ppc, add_jump(&dyna_check_cop1_unusable, 1, 1), 0, 1);
		set_next_dst(ppc);
		// Load the old LR
		GEN_LWZ(ppc, R0, DYNAOFF_LR, R1);
		set_next_dst(ppc);
		// Restore the LR
		GEN_MTLR(ppc, R0);
		set_next_dst(ppc);
		// Return to trampoline
		GEN_BLR(ppc, 0);
		set_next_dst(ppc);
		// Don't check for the rest of this mapping
		// Unless this instruction is in a delay slot
		FP_need_check = isDelaySlot;
	}
}

#define check_memory() \
	invalidateRegisters(); \
	GEN_B(ppc, add_jump(&invalidate_func, 1, 1), 0, 1); \
	set_next_dst(ppc); \
	GEN_LWZ(ppc, R0, DYNAOFF_LR, R1); \
	set_next_dst(ppc); \
	GEN_MTLR(ppc, R0); \
	set_next_dst(ppc);

static void genCallDynaMem(memType type, int count, int _rs, int _rt, short immed){
	PowerPC_instr ppc;
	int isPhysical = 1, isVirtual = 1;
	int isConstant = isRegisterConstant(_rs);
	int constant = getRegisterConstant(_rs) + immed;
	int i;

#ifdef FASTMEM
	if(isConstant){
	#ifdef USE_EXPANSION
		if(constant >= 0x80000000 && constant < 0x80800000)
			isVirtual = 0;
		else if(constant >= 0xA0000000 && constant < 0xA0800000)
			isVirtual = 0;
	#else
		if(constant >= 0x80000000 && constant < 0x80400000)
			isVirtual = 0;
		else if(constant >= 0xA0000000 && constant < 0xA0400000)
			isVirtual = 0;
	#endif
		else
			isPhysical = 0;
	}
#endif

	if(type == MEM_LDL || type == MEM_LDR || type == MEM_SDL || type == MEM_SDR){
		isPhysical = 0;
		isVirtual = 1;
	}

	if(type == MEM_LWC1 || type == MEM_LDC1 || type == MEM_SWC1 || type == MEM_SDC1)
		genCheckFP();

	flushRegisters();
	reset_code_addr();

	int rd = mapRegisterTemp();
	int rs = mapRegister(_rs);

	// addr = rs + immed
	GEN_ADDI(ppc, R3, rs, immed);
	set_next_dst(ppc);

#ifdef FASTMEM
	int to_slow_id;
	PowerPC_instr* ts_preCall;
	if(isPhysical && isVirtual){
		// If base in physical memory
		GEN_XORIS(ppc, R0, R3, 0x8000);
		set_next_dst(ppc);
	#ifdef USE_EXPANSION
		GEN_ANDIS(ppc, R0, R0, 0xDF80);
		set_next_dst(ppc);
	#else
		GEN_ANDIS(ppc, R0, R0, 0xDFC0);
		set_next_dst(ppc);
	#endif
		to_slow_id = add_jump_special(0);
		GEN_BNE(ppc, CR0, to_slow_id, 0, 0);
		set_next_dst(ppc);
		ts_preCall = get_curr_dst();
	}

	if(isPhysical){
		switch(type){
			case MEM_LW:
			{
				for(i = 0; i < count; i++){
					int rt = mapRegisterNew(_rt + i, 1);
					if(i == 0){
						GEN_RLWINM(ppc, R2, R3, 0, 8, 29);
						set_next_dst(ppc);
						GEN_LWZUX(ppc, rt, R2, DYNAREG_RDRAM);
						set_next_dst(ppc);
					} else {
						GEN_LWZ(ppc, rt, i*4, R2);
						set_next_dst(ppc);
					}
				}
				break;
			}
			case MEM_LWU:
			{
				for(i = 0; i < count; i++){
					int rt = mapRegisterNew(_rt + i, 0);
					if(i == 0){
						GEN_RLWINM(ppc, R2, R3, 0, 8, 29);
						set_next_dst(ppc);
						GEN_LWZUX(ppc, rt, R2, DYNAREG_RDRAM);
						set_next_dst(ppc);
					} else {
						GEN_LWZ(ppc, rt, i*4, R2);
						set_next_dst(ppc);
					}
				}
				break;
			}
			case MEM_LH:
			{
				for(i = 0; i < count; i++){
					int rt = mapRegisterNew(_rt + i, 1);
					if(i == 0){
						GEN_RLWINM(ppc, R2, R3, 0, 8, 30);
						set_next_dst(ppc);
						GEN_LHAUX(ppc, rt, R2, DYNAREG_RDRAM);
						set_next_dst(ppc);
					} else {
						GEN_LHA(ppc, rt, i*2, R2);
						set_next_dst(ppc);
					}
				}
				break;
			}
			case MEM_LHU:
			{
				for(i = 0; i < count; i++){
					int rt = mapRegisterNew(_rt + i, 0);
					if(i == 0){
						GEN_RLWINM(ppc, R2, R3, 0, 8, 30);
						set_next_dst(ppc);
						GEN_LHZUX(ppc, rt, R2, DYNAREG_RDRAM);
						set_next_dst(ppc);
					} else {
						GEN_LHZ(ppc, rt, i*2, R2);
						set_next_dst(ppc);
					}
				}
				break;
			}
			case MEM_LB:
			{
				for(i = 0; i < count; i++){
					int rt = mapRegisterNew(_rt + i, 1);
					if(i == 0){
						GEN_RLWINM(ppc, R2, R3, 0, 8, 31);
						set_next_dst(ppc);
						GEN_LBZUX(ppc, rt, R2, DYNAREG_RDRAM);
						set_next_dst(ppc);
					} else {
						GEN_LBZ(ppc, rt, i, R2);
						set_next_dst(ppc);
					}
					GEN_EXTSB(ppc, rt, rt);
					set_next_dst(ppc);
				}
				break;
			}
			case MEM_LBU:
			{
				for(i = 0; i < count; i++){
					int rt = mapRegisterNew(_rt + i, 0);
					if(i == 0){
						GEN_RLWINM(ppc, R2, R3, 0, 8, 31);
						set_next_dst(ppc);
						GEN_LBZUX(ppc, rt, R2, DYNAREG_RDRAM);
						set_next_dst(ppc);
					} else {
						GEN_LBZ(ppc, rt, i, R2);
						set_next_dst(ppc);
					}
				}
				break;
			}
			case MEM_LD:
			{
				for(i = 0; i < count; i++){
					RegMapping rt = mapRegister64New(_rt + i);
					if(i == 0){
						GEN_RLWINM(ppc, R2, R3, 0, 8, 28);
						set_next_dst(ppc);
						GEN_LWZUX(ppc, rt.hi, R2, DYNAREG_RDRAM);
						set_next_dst(ppc);
					} else {
						GEN_LWZ(ppc, rt.hi, i*8, R2);
						set_next_dst(ppc);
					}
					GEN_LWZ(ppc, rt.lo, i*8+4, R2);
					set_next_dst(ppc);
				}
				break;
			}
			case MEM_LWC1:
			{
				for(i = 0; i < count; i++){
					int rt = mapFPRNew(_rt + i*2, 0);
					if(i == 0){
						GEN_RLWINM(ppc, R2, R3, 0, 8, 29);
						set_next_dst(ppc);
						GEN_LFSUX(ppc, rt, R2, DYNAREG_RDRAM);
						set_next_dst(ppc);
					} else {
						GEN_LFS(ppc, rt, i*4, R2);
						set_next_dst(ppc);
					}
				}
				break;
			}
			case MEM_LDC1:
			{
				for(i = 0; i < count; i++){
					int rt = mapFPRNew(_rt + i*2, 1);
					if(i == 0){
						GEN_RLWINM(ppc, R2, R3, 0, 8, 28);
						set_next_dst(ppc);
						GEN_LFDUX(ppc, rt, R2, DYNAREG_RDRAM);
						set_next_dst(ppc);
					} else {
						GEN_LFD(ppc, rt, i*8, R2);
						set_next_dst(ppc);
					}
				}
				break;
			}
			case MEM_LL:
			{
				int rt = mapRegisterNew(_rt, 1);
				GEN_RLWINM(ppc, R2, R3, 0, 8, 29);
				set_next_dst(ppc);
				GEN_LI(ppc, R0, 1);
				set_next_dst(ppc);
				GEN_LWZUX(ppc, rt, R2, DYNAREG_RDRAM);
				set_next_dst(ppc);
				GEN_STW(ppc, R0, SDAREL(llbit), R13);
				set_next_dst(ppc);
				break;
			}
			case MEM_LWL:
			{
				int rt = mapRegister(_rt);
				int word = mapRegisterTemp();
				int mask = mapRegisterTemp();
				GEN_RLWINM(ppc, R2, R3, 0, 8, 29);
				set_next_dst(ppc);
				GEN_CLRLSLWI(ppc, R0, R3, 30, 3);
				set_next_dst(ppc);
				GEN_LWZUX(ppc, word, R2, DYNAREG_RDRAM);
				set_next_dst(ppc);
				GEN_LI(ppc, mask, ~0);
				set_next_dst(ppc);
				GEN_SLW(ppc, mask, mask, R0);
				set_next_dst(ppc);
				GEN_ANDC(ppc, rt, rt, mask);
				set_next_dst(ppc);
				GEN_SLW(ppc, word, word, R0);
				set_next_dst(ppc);
				GEN_OR(ppc, rt, word, rt);
				set_next_dst(ppc);
				mapRegisterNew(_rt, 1);
				break;
			}
			case MEM_LWR:
			{
				int rt = mapRegister(_rt);
				int word = mapRegisterTemp();
				int mask = mapRegisterTemp();
				GEN_NOT(ppc, R0, R3);
				set_next_dst(ppc);
				GEN_RLWINM(ppc, R2, R3, 0, 8, 29);
				set_next_dst(ppc);
				GEN_CLRLSLWI(ppc, R0, R0, 30, 3);
				set_next_dst(ppc);
				GEN_LWZUX(ppc, word, R2, DYNAREG_RDRAM);
				set_next_dst(ppc);
				GEN_LI(ppc, mask, ~0);
				set_next_dst(ppc);
				GEN_SRW(ppc, mask, mask, R0);
				set_next_dst(ppc);
				GEN_ANDC(ppc, rt, rt, mask);
				set_next_dst(ppc);
				GEN_SRW(ppc, word, word, R0);
				set_next_dst(ppc);
				GEN_OR(ppc, rt, word, rt);
				set_next_dst(ppc);
				mapRegisterNew(_rt, 1);
				break;
			}
			case MEM_LDL:
				break;
			case MEM_LDR:
				break;
			case MEM_SW:
			{
				for(i = 0; i < count; i++){
					int rt = mapRegister(_rt + i);
					if(i == 0){
						GEN_RLWINM(ppc, R2, R3, 0, 8, 29);
						set_next_dst(ppc);
						GEN_STWUX(ppc, rt, R2, DYNAREG_RDRAM);
						set_next_dst(ppc);
					} else {
						GEN_STW(ppc, rt, i*4, R2);
						set_next_dst(ppc);
					}
				}
				check_memory();
				break;
			}
			case MEM_SH:
			{
				for(i = 0; i < count; i++){
					int rt = mapRegister(_rt + i);
					if(i == 0){
						GEN_RLWINM(ppc, R2, R3, 0, 8, 30);
						set_next_dst(ppc);
						GEN_STHUX(ppc, rt, R2, DYNAREG_RDRAM);
						set_next_dst(ppc);
					} else {
						GEN_STH(ppc, rt, i*2, R2);
						set_next_dst(ppc);
					}
				}
				check_memory();
				break;
			}
			case MEM_SB:
			{
				for(i = 0; i < count; i++){
					int rt = mapRegister(_rt + i);
					if(i == 0){
						GEN_RLWINM(ppc, R2, R3, 0, 8, 31);
						set_next_dst(ppc);
						GEN_STBUX(ppc, rt, R2, DYNAREG_RDRAM);
						set_next_dst(ppc);
					} else {
						GEN_STB(ppc, rt, i, R2);
						set_next_dst(ppc);
					}
				}
				check_memory();
				break;
			}
			case MEM_SD:
			{
				for(i = 0; i < count; i++){
					RegMapping rt = mapRegister64(_rt + i);
					if(i == 0){
						GEN_RLWINM(ppc, R2, R3, 0, 8, 28);
						set_next_dst(ppc);
						GEN_STWUX(ppc, rt.hi, R2, DYNAREG_RDRAM);
						set_next_dst(ppc);
					} else {
						GEN_STW(ppc, rt.hi, i*8, R2);
						set_next_dst(ppc);
					}
					GEN_STW(ppc, rt.lo, i*8+4, R2);
					set_next_dst(ppc);
				}
				check_memory();
				break;
			}
			case MEM_SWC1:
			{
				for(i = 0; i < count; i++){
					int rt = mapFPR(_rt + i*2, 0);
					if(i == 0){
						GEN_RLWINM(ppc, R2, R3, 0, 8, 29);
						set_next_dst(ppc);
						GEN_STFSUX(ppc, rt, R2, DYNAREG_RDRAM);
						set_next_dst(ppc);
					} else {
						GEN_STFS(ppc, rt, i*4, R2);
						set_next_dst(ppc);
					}
				}
				check_memory();
				break;
			}
			case MEM_SDC1:
			{
				for(i = 0; i < count; i++){
					int rt = mapFPR(_rt + i*2, 1);
					if(i == 0){
						GEN_RLWINM(ppc, R2, R3, 0, 8, 28);
						set_next_dst(ppc);
						GEN_STFDUX(ppc, rt, R2, DYNAREG_RDRAM);
						set_next_dst(ppc);
					} else {
						GEN_STFD(ppc, rt, i*8, R2);
						set_next_dst(ppc);
					}
				}
				check_memory();
				break;
			}
			case MEM_SC:
			{
				int rt = mapRegister(_rt);
				GEN_LWZ(ppc, R0, SDAREL(llbit), R13);
				set_next_dst(ppc);
				GEN_CMPLI(ppc, CR2, R0, 0);
				set_next_dst(ppc);
				GEN_BEQ(ppc, CR2, 6, 0, 0);
				set_next_dst(ppc);
				GEN_RLWINM(ppc, R2, R3, 0, 8, 29);
				set_next_dst(ppc);
				GEN_STWUX(ppc, rt, R2, DYNAREG_RDRAM);
				set_next_dst(ppc);
				check_memory();
				GEN_MFCR(ppc, R2);
				set_next_dst(ppc);
				GEN_RLWINM(ppc, mapRegisterNew(_rt, 0), R2, 10, 31, 31);
				set_next_dst(ppc);
				GEN_STW(ppc, DYNAREG_ZERO, SDAREL(llbit), R13);
				set_next_dst(ppc);
				break;
			}
			case MEM_SWL:
			{
				int rt = mapRegister(_rt);
				int word = mapRegisterTemp();
				int mask = mapRegisterTemp();
				GEN_RLWINM(ppc, R2, R3, 0, 8, 29);
				set_next_dst(ppc);
				GEN_CLRLSLWI(ppc, R0, R3, 30, 3);
				set_next_dst(ppc);
				GEN_LWZUX(ppc, word, R2, DYNAREG_RDRAM);
				set_next_dst(ppc);
				GEN_LI(ppc, mask, ~0);
				set_next_dst(ppc);
				GEN_SRW(ppc, mask, mask, R0);
				set_next_dst(ppc);
				GEN_ANDC(ppc, word, word, mask);
				set_next_dst(ppc);
				GEN_SRW(ppc, R0, rt, R0);
				set_next_dst(ppc);
				GEN_OR(ppc, R0, word, R0);
				set_next_dst(ppc);
				GEN_STW(ppc, R0, 0, R2);
				set_next_dst(ppc);
				check_memory();
				break;
			}
			case MEM_SWR:
			{
				int rt = mapRegister(_rt);
				int word = mapRegisterTemp();
				int mask = mapRegisterTemp();
				GEN_NOT(ppc, R0, R3);
				set_next_dst(ppc);
				GEN_RLWINM(ppc, R2, R3, 0, 8, 29);
				set_next_dst(ppc);
				GEN_CLRLSLWI(ppc, R0, R0, 30, 3);
				set_next_dst(ppc);
				GEN_LWZUX(ppc, word, R2, DYNAREG_RDRAM);
				set_next_dst(ppc);
				GEN_LI(ppc, mask, ~0);
				set_next_dst(ppc);
				GEN_SLW(ppc, mask, mask, R0);
				set_next_dst(ppc);
				GEN_ANDC(ppc, word, word, mask);
				set_next_dst(ppc);
				GEN_SLW(ppc, R0, rt, R0);
				set_next_dst(ppc);
				GEN_OR(ppc, R0, word, R0);
				set_next_dst(ppc);
				GEN_STW(ppc, R0, 0, R2);
				set_next_dst(ppc);
				check_memory();
				break;
			}
			case MEM_SDL:
				break;
			case MEM_SDR:
				break;
		}
	}

	int not_fastmem_id;
	PowerPC_instr* preCall;
	if(isPhysical && isVirtual){
		flushRegisters();
		// Skip over else
		not_fastmem_id = add_jump_special(1);
		GEN_B(ppc, not_fastmem_id, 0, 0);
		set_next_dst(ppc);
		preCall = get_curr_dst();

		int ts_callSize = get_curr_dst() - ts_preCall;
		set_jump_special(to_slow_id, ts_callSize+1);
	}
#endif // FASTMEM

	if(isVirtual){
		invalidateRegisters();
		GEN_LI(ppc, R4, _rt);
		set_next_dst(ppc);
		GEN_LI(ppc, R5, count);
		set_next_dst(ppc);
		// Pass PC as arg 5 (upper half)
		GEN_LIS(ppc, R7, (get_src_pc()+4)>>16);
		set_next_dst(ppc);
		// type passed as arg 4
		GEN_LI(ppc, R6, type);
		set_next_dst(ppc);
		// Lower half of PC
		GEN_ORI(ppc, R7, R7, get_src_pc()+4);
		set_next_dst(ppc);
		// isDelaySlot as arg 6
		GEN_LI(ppc, R8, isDelaySlot);
		set_next_dst(ppc);
		// call dyna_mem
		GEN_B(ppc, add_jump(&dyna_mem, 1, 1), 0, 1);
		set_next_dst(ppc);
		// Load old LR
		GEN_LWZ(ppc, R0, DYNAOFF_LR, R1);
		set_next_dst(ppc);
		// Check whether we need to take an interrupt
		GEN_CMPI(ppc, CR5, R3, 0);
		set_next_dst(ppc);
		// Restore LR
		GEN_MTLR(ppc, R0);
		set_next_dst(ppc);
		// If so, return to trampoline
		GEN_BNELR(ppc, CR5, 0);
		set_next_dst(ppc);
	}

#ifdef FASTMEM
	if(isPhysical && isVirtual){
		int callSize = get_curr_dst() - preCall;
		set_jump_special(not_fastmem_id, callSize+1);
	}
#endif
}

static int mips_is_jump(MIPS_instr instr){
	int opcode = MIPS_GET_OPCODE(instr);
	int format = MIPS_GET_RS    (instr);
	int func   = MIPS_GET_FUNC  (instr);
	return (opcode == MIPS_OPCODE_J     ||
                opcode == MIPS_OPCODE_JAL   ||
                opcode == MIPS_OPCODE_BEQ   ||
                opcode == MIPS_OPCODE_BNE   ||
                opcode == MIPS_OPCODE_BLEZ  ||
                opcode == MIPS_OPCODE_BGTZ  ||
                opcode == MIPS_OPCODE_BEQL  ||
                opcode == MIPS_OPCODE_BNEL  ||
                opcode == MIPS_OPCODE_BLEZL ||
                opcode == MIPS_OPCODE_BGTZL ||
                opcode == MIPS_OPCODE_B     ||
                (opcode == MIPS_OPCODE_R    &&
                 (func  == MIPS_FUNC_JR     ||
                  func  == MIPS_FUNC_JALR)) ||
                (opcode == MIPS_OPCODE_COP1 &&
                 format == MIPS_FRMT_BC)    );
}

