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
void genCallDynaMem(memType type, int base, short immed);
void RecompCache_Update(PowerPC_func*);
static int inline mips_is_jump(MIPS_instr);
void jump_to(unsigned int);
void check_interupt();
extern int llbit;

double __floatdidf(long long);
float __floatdisf(long long);
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
		
		GEN_CMP(ppc, ra, rb, 4);
		set_next_dst(ppc);
	} else {
		RegMapping ra = mapRegister64(_ra), rb = mapRegister64(_rb);
		
		GEN_CMP(ppc, ra.hi, rb.hi, 4);
		set_next_dst(ppc);
		// Skip low word comparison if high words are mismatched
		GEN_BNE(ppc, 4, 2, 0, 0);
		set_next_dst(ppc);
		// Compare low words if hi words don't match
		GEN_CMPL(ppc, ra.lo, rb.lo, 4);
		set_next_dst(ppc);
	}
}

static void genCmpi64(int cr, int _ra, short immed){
	PowerPC_instr ppc;
	
	if(getRegisterMapping(_ra) == MAPPING_32){
		// If we've mapped this register as 32-bit, don't bother with 64-bit
		int ra = mapRegister(_ra);
		
		GEN_CMPI(ppc, ra, immed, 4);
		set_next_dst(ppc);
	} else {
		RegMapping ra = mapRegister64(_ra);
		
		GEN_CMPI(ppc, ra.hi, (immed&0x8000) ? ~0 : 0, 4);
		set_next_dst(ppc);
		// Skip low word comparison if high words are mismatched
		GEN_BNE(ppc, 4, 2, 0, 0);
		set_next_dst(ppc);
		// Compare low words if hi words don't match
		GEN_CMPLI(ppc, ra.lo, immed, 4);
		set_next_dst(ppc);
	}
}

typedef enum { NONE=0, EQ, NE, LT, GT, LE, GE } condition;
// Branch a certain offset (possibly conditionally, linking, or likely)
//   offset: N64 instructions from current N64 instruction to branch
//   cond: type of branch to execute depending on cr 7
//   link: if nonzero, branch and link
//   likely: if nonzero, the delay slot will only be executed when cond is true
static int branch(int offset, condition cond, int link, int likely){
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
		int lr = mapRegisterNew(MIPS_REG_LR);
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
	GEN_LI(ppc, 4, 0, 1);
	set_next_dst(ppc);
	if(likely){
		GEN_B(ppc, 2, 0, 0);
		set_next_dst(ppc);
		GEN_LI(ppc, 4, 0, 0);
		set_next_dst(ppc);

		set_jump_special(likely_id, delaySlot+2+1);
	}
#else
	if(likely) set_jump_special(likely_id, delaySlot+1);
#endif

	genUpdateCount(1); // Sets cr2 to (next_interupt ? Count)

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
		GEN_LIS(ppc, 3, (get_src_pc()+4)>>16);
		set_next_dst(ppc);
		GEN_ORI(ppc, 3, 3, get_src_pc()+4);
		set_next_dst(ppc);
		// If taking the interrupt, return to the trampoline
		GEN_BLELR(ppc, 2, 0);
		set_next_dst(ppc);

#ifndef INTERPRET_BRANCH
	} else {
		// last_addr = naddr
		if(cond != NONE){
			GEN_BC(ppc, 4, 0, 0, bo, bi);
			set_next_dst(ppc);
			GEN_LIS(ppc, 3, (get_src_pc()+4)>>16);
			set_next_dst(ppc);
			GEN_ORI(ppc, 3, 3, get_src_pc()+4);
			set_next_dst(ppc);
			GEN_B(ppc, 3, 0, 0);
			set_next_dst(ppc);
		}
		GEN_LIS(ppc, 3, (get_src_pc() + (offset<<2))>>16);
		set_next_dst(ppc);
		GEN_ORI(ppc, 3, 3, get_src_pc() + (offset<<2));
		set_next_dst(ppc);
		GEN_STW(ppc, 3, 0, DYNAREG_LADDR);
		set_next_dst(ppc);

		// If taking the interrupt, return to the trampoline
		GEN_BLELR(ppc, 2, 0);
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
		if(is_j_dst() && !is_j_out(0, 0)){
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
	GEN_LI(ppc, 4, 0, 1);
	set_next_dst(ppc);
#endif
	// Sets cr2 to (next_interupt ? Count)
	genUpdateCount(1);

#ifdef INTERPRET_J
	genJumpTo(MIPS_GET_LI(mips), JUMPTO_ADDR);
#else // INTERPRET_J
	// If we're jumping out, we can't just use a branch instruction
	if(is_j_out(MIPS_GET_LI(mips), 1)){
		genJumpTo(MIPS_GET_LI(mips), JUMPTO_ADDR);
	} else {
		// last_addr = naddr
		GEN_LIS(ppc, 3, naddr>>16);
		set_next_dst(ppc);
		GEN_ORI(ppc, 3, 3, naddr);
		set_next_dst(ppc);
		GEN_STW(ppc, 3, 0, DYNAREG_LADDR);
		set_next_dst(ppc);

		// if(next_interupt <= Count) return;
		GEN_BLELR(ppc, 2, 0);
		set_next_dst(ppc);

		// Even though this is an absolute branch
		//   in pass 2, we generate a relative branch
		GEN_B(ppc, add_jump(MIPS_GET_LI(mips), 1, 0), 0, 0);
		set_next_dst(ppc);
	}
#endif

	// Let's still recompile the delay slot in place in case its branched to
	if(delaySlot){ if(is_j_dst()){ unget_last_src(); delaySlotNext = 2; } }
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
	GEN_LI(ppc, 4, 0, 1);
	set_next_dst(ppc);
#endif
	// Sets cr2 to (next_interupt ? Count)
	genUpdateCount(1);

	// Set LR to next instruction
	int lr = mapRegisterNew(MIPS_REG_LR);
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
		GEN_LIS(ppc, 3, naddr>>16);
		set_next_dst(ppc);
		GEN_ORI(ppc, 3, 3, naddr);
		set_next_dst(ppc);
		GEN_STW(ppc, 3, 0, DYNAREG_LADDR);
		set_next_dst(ppc);

		/// if(next_interupt <= Count) return;
		GEN_BLELR(ppc, 2, 0);
		set_next_dst(ppc);

		// Even though this is an absolute branch
		//   in pass 2, we generate a relative branch
		GEN_B(ppc, add_jump(MIPS_GET_LI(mips), 1, 0), 0, 0);
		set_next_dst(ppc);
	}
#endif

	// Let's still recompile the delay slot in place in case its branched to
	if(delaySlot){ if(is_j_dst()){ unget_last_src(); delaySlotNext = 2; } }
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
	
	genCmp64(4, MIPS_GET_RA(mips), MIPS_GET_RB(mips));

	return branch(signExtend(MIPS_GET_IMMED(mips),16), EQ, 0, 0);
}

static int BNE(MIPS_instr mips){
	PowerPC_instr  ppc;

	if(CANT_COMPILE_DELAY()){
		genCallInterp(mips);
		return INTERPRETED;
	}

	genCmp64(4, MIPS_GET_RA(mips), MIPS_GET_RB(mips));

	return branch(signExtend(MIPS_GET_IMMED(mips),16), NE, 0, 0);
}

static int BLEZ(MIPS_instr mips){
	PowerPC_instr  ppc;

	if(CANT_COMPILE_DELAY()){
		genCallInterp(mips);
		return INTERPRETED;
	}

	genCmpi64(4, MIPS_GET_RA(mips), 0);

	return branch(signExtend(MIPS_GET_IMMED(mips),16), LE, 0, 0);
}

static int BGTZ(MIPS_instr mips){
	PowerPC_instr  ppc;

	if(CANT_COMPILE_DELAY()){
		genCallInterp(mips);
		return INTERPRETED;
	}

	genCmpi64(4, MIPS_GET_RA(mips), 0);

	return branch(signExtend(MIPS_GET_IMMED(mips),16), GT, 0, 0);
}

static int ADDIU(MIPS_instr mips){
	PowerPC_instr ppc;
	int rs = mapRegister( MIPS_GET_RS(mips) );
	GEN_ADDI(ppc,
	         mapRegisterNew( MIPS_GET_RT(mips) ),
	         rs,
	         MIPS_GET_IMMED(mips));
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
	int rt = mapRegisterNew( MIPS_GET_RT(mips) );
	int tmp = (rs == rt) ? mapRegisterTemp() : rt;

	// tmp = immed (sign extended)
	GEN_ADDI(ppc, tmp, 0, MIPS_GET_IMMED(mips));
	set_next_dst(ppc);
	// carry = rs < immed ? 0 : 1 (unsigned)
	GEN_SUBFC(ppc, 0, tmp, rs);
	set_next_dst(ppc);
	// rt = ~(rs ^ immed)
	GEN_EQV(ppc, rt, tmp, rs);
	set_next_dst(ppc);
	// rt = sign(rs) == sign(immed) ? 1 : 0
	GEN_SRWI(ppc, rt, rt, 31);
	set_next_dst(ppc);
	// rt += carry
	GEN_ADDZE(ppc, rt, rt);
	set_next_dst(ppc);
	// rt &= 1 ( = (sign(rs) == sign(immed)) xor (rs < immed (unsigned)) )
	GEN_RLWINM(ppc, rt, rt, 0, 31, 31);
	set_next_dst(ppc);

	if(rs == rt) unmapRegisterTemp(tmp);

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
	int rt = mapRegisterNew( MIPS_GET_RT(mips) );

	// r0 = EXTS(immed)
	GEN_ADDI(ppc, 0, 0, MIPS_GET_IMMED(mips));
	set_next_dst(ppc);
	// carry = rs < immed ? 0 : 1
	GEN_SUBFC(ppc, rt, 0, rs);
	set_next_dst(ppc);
	// rt = carry - 1 ( = rs < immed ? -1 : 0 )
	GEN_SUBFE(ppc, rt, rt, rt);
	set_next_dst(ppc);
	// rt = !carry ( = rs < immed ? 1 : 0 )
	GEN_NEG(ppc, rt, rt);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
#endif
}

static int ANDI(MIPS_instr mips){
	PowerPC_instr ppc;
	int rs = mapRegister( MIPS_GET_RS(mips) );
	int rt = mapRegisterNew( MIPS_GET_RT(mips) );

	GEN_ANDI(ppc, rt, rs, MIPS_GET_IMMED(mips));
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
}

static int ORI(MIPS_instr mips){
	PowerPC_instr ppc;
	RegMapping rs = mapRegister64( MIPS_GET_RS(mips) );
	RegMapping rt = mapRegister64New( MIPS_GET_RT(mips) );

	GEN_OR(ppc, rt.hi, rs.hi, rs.hi);
	set_next_dst(ppc);
	GEN_ORI(ppc, rt.lo, rs.lo, MIPS_GET_IMMED(mips));
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
}

static int XORI(MIPS_instr mips){
	PowerPC_instr ppc;
	RegMapping rs = mapRegister64( MIPS_GET_RS(mips) );
	RegMapping rt = mapRegister64New( MIPS_GET_RT(mips) );

	GEN_OR(ppc, rt.hi, rs.hi, rs.hi);
	set_next_dst(ppc);
	GEN_XORI(ppc, rt.lo, rs.lo, MIPS_GET_IMMED(mips));
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
}

static int LUI(MIPS_instr mips){
	PowerPC_instr ppc;
	GEN_LIS(ppc,
	        mapRegisterNew( MIPS_GET_RT(mips) ),
	        MIPS_GET_IMMED(mips));
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
}

static int BEQL(MIPS_instr mips){
	PowerPC_instr  ppc;

	if(CANT_COMPILE_DELAY()){
		genCallInterp(mips);
		return INTERPRETED;
	}

	genCmp64(4, MIPS_GET_RA(mips), MIPS_GET_RB(mips));

	return branch(signExtend(MIPS_GET_IMMED(mips),16), EQ, 0, 1);
}

static int BNEL(MIPS_instr mips){
	PowerPC_instr  ppc;

	if(CANT_COMPILE_DELAY()){
		genCallInterp(mips);
		return INTERPRETED;
	}

	genCmp64(4, MIPS_GET_RA(mips), MIPS_GET_RB(mips));

	return branch(signExtend(MIPS_GET_IMMED(mips),16), NE, 0, 1);
}

static int BLEZL(MIPS_instr mips){
	PowerPC_instr  ppc;

	if(CANT_COMPILE_DELAY()){
		genCallInterp(mips);
		return INTERPRETED;
	}

	genCmpi64(4, MIPS_GET_RA(mips), 0);

	return branch(signExtend(MIPS_GET_IMMED(mips),16), LE, 0, 1);
}

static int BGTZL(MIPS_instr mips){
	PowerPC_instr  ppc;

	if(CANT_COMPILE_DELAY()){
		genCallInterp(mips);
		return INTERPRETED;
	}

	genCmpi64(4, MIPS_GET_RA(mips), 0);

	return branch(signExtend(MIPS_GET_IMMED(mips),16), GT, 0, 1);
}

static int DADDIU(MIPS_instr mips){
	PowerPC_instr ppc;
#if defined(INTERPRET_DW) || defined(INTERPRET_DADDIU)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_DW || INTERPRET_DADDIU

	RegMapping rs = mapRegister64( MIPS_GET_RS(mips) );
	RegMapping rt = mapRegister64New( MIPS_GET_RT(mips) );

	// Sign extend the immediate for the MSW
	GEN_ADDI(ppc, 0, 0, (MIPS_GET_IMMED(mips)&0x8000) ? ~0 : 0);
	set_next_dst(ppc);
	// Add the immediate to the LSW
	GEN_ADDIC(ppc, rt.lo, rs.lo, MIPS_GET_IMMED(mips));
	set_next_dst(ppc);
	// Add the MSW with the sign-extension and the carry
	GEN_ADDE(ppc, rt.hi, rs.hi, 0);
	set_next_dst(ppc);

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
	// TODO: ldl
	return CONVERT_ERROR;
#endif
}

static int LDR(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_LDR
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_LDR
	// TODO: ldr
	return CONVERT_ERROR;
#endif
}

static int LB(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_LB
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_LB

	flushRegisters();
	reset_code_addr();

	int rd = mapRegisterTemp(); // r3 = rd
	int base = mapRegister( MIPS_GET_RS(mips) ); // r4 = addr

	invalidateRegisters();

#ifdef FASTMEM
	// If base in physical memory
#ifdef USE_EXPANSION
	GEN_LIS(ppc, 0, 0x8080);
#else
	GEN_LIS(ppc, 0, 0x8040);
#endif
	set_next_dst(ppc);
	GEN_CMP(ppc, base, 0, 1);
	set_next_dst(ppc);
	GEN_BGE(ppc, 1, 9, 0, 0);
	set_next_dst(ppc);

	// Use rdram
#ifdef USE_EXPANSION
	// Mask sp with 0x007FFFFF
	GEN_RLWINM(ppc, base, base, 0, 9, 31);
	set_next_dst(ppc);
#else
	// Mask sp with 0x003FFFFF
	GEN_RLWINM(ppc, base, base, 0, 10, 31);
	set_next_dst(ppc);
#endif
	// Add rdram pointer
	GEN_ADD(ppc, base, DYNAREG_RDRAM, base);
	set_next_dst(ppc);
	// Perform the actual load
	GEN_LBZ(ppc, 3, MIPS_GET_IMMED(mips), base);
	set_next_dst(ppc);
	// extsb rt
	GEN_EXTSB(ppc, 3, 3);
	set_next_dst(ppc);
	// Have the value in r3 stored to rt
	mapRegisterNew( MIPS_GET_RT(mips) );
	flushRegisters();
	// Skip over else
	int not_fastmem_id = add_jump_special(1);
	GEN_B(ppc, not_fastmem_id, 0, 0);
	set_next_dst(ppc);
	PowerPC_instr* preCall = get_curr_dst();
#endif // FASTMEM

	// load into rt
	GEN_LI(ppc, 3, 0, MIPS_GET_RT(mips));
	set_next_dst(ppc);

	genCallDynaMem(MEM_LB, base, MIPS_GET_IMMED(mips));

#ifdef FASTMEM
	int callSize = get_curr_dst() - preCall;
	set_jump_special(not_fastmem_id, callSize+1);
#endif

	return CONVERT_SUCCESS;
#endif
}

static int LH(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_LH
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_LH

	flushRegisters();
	reset_code_addr();

	int rd = mapRegisterTemp(); // r3 = rd
	int base = mapRegister( MIPS_GET_RS(mips) ); // r4 = addr

	invalidateRegisters();

#ifdef FASTMEM
	// If base in physical memory
#ifdef USE_EXPANSION
	GEN_LIS(ppc, 0, 0x8080);
#else
	GEN_LIS(ppc, 0, 0x8040);
#endif
	set_next_dst(ppc);
	GEN_CMP(ppc, base, 0, 1);
	set_next_dst(ppc);
	GEN_BGE(ppc, 1, 8, 0, 0);
	set_next_dst(ppc);

	// Use rdram
#ifdef USE_EXPANSION
	// Mask sp with 0x007FFFFF
	GEN_RLWINM(ppc, base, base, 0, 9, 31);
	set_next_dst(ppc);
#else
	// Mask sp with 0x003FFFFF
	GEN_RLWINM(ppc, base, base, 0, 10, 31);
	set_next_dst(ppc);
#endif
	// Add rdram pointer
	GEN_ADD(ppc, base, DYNAREG_RDRAM, base);
	set_next_dst(ppc);
	// Perform the actual load
	GEN_LHA(ppc, 3, MIPS_GET_IMMED(mips), base);
	set_next_dst(ppc);
	// Have the value in r3 stored to rt
	mapRegisterNew( MIPS_GET_RT(mips) );
	flushRegisters();
	// Skip over else
	int not_fastmem_id = add_jump_special(1);
	GEN_B(ppc, not_fastmem_id, 0, 0);
	set_next_dst(ppc);
	PowerPC_instr* preCall = get_curr_dst();
#endif // FASTMEM

	// load into rt
	GEN_LI(ppc, 3, 0, MIPS_GET_RT(mips));
	set_next_dst(ppc);

	genCallDynaMem(MEM_LH, base, MIPS_GET_IMMED(mips));

#ifdef FASTMEM
	int callSize = get_curr_dst() - preCall;
	set_jump_special(not_fastmem_id, callSize+1);
#endif

	return CONVERT_SUCCESS;
#endif
}

static int LWL(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_LWL
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_LWL
	// TODO: lwl
	return CONVERT_ERROR;
#endif
}

static int LW(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_LW
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_LW

	flushRegisters();
	reset_code_addr();

	int rd = mapRegisterTemp(); // r3 = rd
	int base = mapRegister( MIPS_GET_RS(mips) ); // r4 = addr

	invalidateRegisters();

#ifdef FASTMEM
	// If base in physical memory
#ifdef USE_EXPANSION
	GEN_LIS(ppc, 0, 0x8080);
#else
	GEN_LIS(ppc, 0, 0x8040);
#endif
	set_next_dst(ppc);
	GEN_CMP(ppc, base, 0, 1);
	set_next_dst(ppc);
	GEN_BGE(ppc, 1, 8, 0, 0);
	set_next_dst(ppc);

	// Use rdram
#ifdef USE_EXPANSION
	// Mask sp with 0x007FFFFF
	GEN_RLWINM(ppc, base, base, 0, 9, 31);
	set_next_dst(ppc);
#else
	// Mask sp with 0x003FFFFF
	GEN_RLWINM(ppc, base, base, 0, 10, 31);
	set_next_dst(ppc);
#endif
	// Add rdram pointer
	GEN_ADD(ppc, base, DYNAREG_RDRAM, base);
	set_next_dst(ppc);
	// Perform the actual load
	GEN_LWZ(ppc, 3, MIPS_GET_IMMED(mips), base);
	set_next_dst(ppc);
	// Have the value in r3 stored to rt
	mapRegisterNew( MIPS_GET_RT(mips) );
	flushRegisters();
	// Skip over else
	int not_fastmem_id = add_jump_special(1);
	GEN_B(ppc, not_fastmem_id, 0, 0);
	set_next_dst(ppc);
	PowerPC_instr* preCall = get_curr_dst();
#endif // FASTMEM

	// load into rt
	GEN_LI(ppc, 3, 0, MIPS_GET_RT(mips));
	set_next_dst(ppc);

	genCallDynaMem(MEM_LW, base, MIPS_GET_IMMED(mips));

#ifdef FASTMEM
	int callSize = get_curr_dst() - preCall;
	set_jump_special(not_fastmem_id, callSize+1);
#endif

	return CONVERT_SUCCESS;
#endif
}

static int LBU(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_LBU
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_LBU

	flushRegisters();
	reset_code_addr();

	int rd = mapRegisterTemp(); // r3 = rd
	int base = mapRegister( MIPS_GET_RS(mips) ); // r4 = addr

	invalidateRegisters();

#ifdef FASTMEM
	// If base in physical memory
#ifdef USE_EXPANSION
	GEN_LIS(ppc, 0, 0x8080);
#else
	GEN_LIS(ppc, 0, 0x8040);
#endif
	set_next_dst(ppc);
	GEN_CMP(ppc, base, 0, 1);
	set_next_dst(ppc);
	GEN_BGE(ppc, 1, 8, 0, 0);
	set_next_dst(ppc);

	// Use rdram
#ifdef USE_EXPANSION
	// Mask sp with 0x007FFFFF
	GEN_RLWINM(ppc, base, base, 0, 9, 31);
	set_next_dst(ppc);
#else
	// Mask sp with 0x003FFFFF
	GEN_RLWINM(ppc, base, base, 0, 10, 31);
	set_next_dst(ppc);
#endif
	// Add rdram pointer
	GEN_ADD(ppc, base, DYNAREG_RDRAM, base);
	set_next_dst(ppc);
	// Perform the actual load
	GEN_LBZ(ppc, 3, MIPS_GET_IMMED(mips), base);
	set_next_dst(ppc);
	// Have the value in r3 stored to rt
	mapRegisterNew( MIPS_GET_RT(mips) );
	flushRegisters();
	// Skip over else
	int not_fastmem_id = add_jump_special(1);
	GEN_B(ppc, not_fastmem_id, 0, 0);
	set_next_dst(ppc);
	PowerPC_instr* preCall = get_curr_dst();
#endif // FASTMEM

	// load into rt
	GEN_LI(ppc, 3, 0, MIPS_GET_RT(mips));
	set_next_dst(ppc);

	genCallDynaMem(MEM_LBU, base, MIPS_GET_IMMED(mips));

#ifdef FASTMEM
	int callSize = get_curr_dst() - preCall;
	set_jump_special(not_fastmem_id, callSize+1);
#endif

	return CONVERT_SUCCESS;
#endif
}

static int LHU(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_LHU
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_LHU

	flushRegisters();
	reset_code_addr();

	int rd = mapRegisterTemp(); // r3 = rd
	int base = mapRegister( MIPS_GET_RS(mips) ); // r4 = addr

	invalidateRegisters();

#ifdef FASTMEM
	// If base in physical memory
#ifdef USE_EXPANSION
	GEN_LIS(ppc, 0, 0x8080);
#else
	GEN_LIS(ppc, 0, 0x8040);
#endif
	set_next_dst(ppc);
	GEN_CMP(ppc, base, 0, 1);
	set_next_dst(ppc);
	GEN_BGE(ppc, 1, 8, 0, 0);
	set_next_dst(ppc);

	// Use rdram
#ifdef USE_EXPANSION
	// Mask sp with 0x007FFFFF
	GEN_RLWINM(ppc, base, base, 0, 9, 31);
	set_next_dst(ppc);
#else
	// Mask sp with 0x003FFFFF
	GEN_RLWINM(ppc, base, base, 0, 10, 31);
	set_next_dst(ppc);
#endif
	// Add rdram pointer
	GEN_ADD(ppc, base, DYNAREG_RDRAM, base);
	set_next_dst(ppc);
	// Perform the actual load
	GEN_LHZ(ppc, 3, MIPS_GET_IMMED(mips), base);
	set_next_dst(ppc);
	// Have the value in r3 stored to rt
	mapRegisterNew( MIPS_GET_RT(mips) );
	flushRegisters();
	// Skip over else
	int not_fastmem_id = add_jump_special(1);
	GEN_B(ppc, not_fastmem_id, 0, 0);
	set_next_dst(ppc);
	PowerPC_instr* preCall = get_curr_dst();
#endif // FASTMEM

	// load into rt
	GEN_LI(ppc, 3, 0, MIPS_GET_RT(mips));
	set_next_dst(ppc);

	genCallDynaMem(MEM_LHU, base, MIPS_GET_IMMED(mips));

#ifdef FASTMEM
	int callSize = get_curr_dst() - preCall;
	set_jump_special(not_fastmem_id, callSize+1);
#endif

	return CONVERT_SUCCESS;
#endif
}

static int LWR(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_LWR
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_LWR
	// TODO: lwr
	return CONVERT_ERROR;
#endif
}

static int LWU(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_LWU
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_LWU

	flushRegisters();
	reset_code_addr();

	int rd = mapRegisterTemp(); // r3 = rd
	int base = mapRegister( MIPS_GET_RS(mips) ); // r4 = addr

	invalidateRegisters();

#ifdef FASTMEM
	// If base in physical memory
#ifdef USE_EXPANSION
	GEN_LIS(ppc, 0, 0x8080);
#else
	GEN_LIS(ppc, 0, 0x8040);
#endif
	set_next_dst(ppc);
	GEN_CMP(ppc, base, 0, 1);
	set_next_dst(ppc);
	GEN_BGE(ppc, 1, 8, 0, 0);
	set_next_dst(ppc);

	// Use rdram
#ifdef USE_EXPANSION
	// Mask sp with 0x007FFFFF
	GEN_RLWINM(ppc, base, base, 0, 9, 31);
	set_next_dst(ppc);
#else
	// Mask sp with 0x003FFFFF
	GEN_RLWINM(ppc, base, base, 0, 10, 31);
	set_next_dst(ppc);
#endif
	// Add rdram pointer
	GEN_ADD(ppc, base, DYNAREG_RDRAM, base);
	set_next_dst(ppc);
	// Create a mapping for this value
	RegMapping value = mapRegister64New( MIPS_GET_RT(mips) );
	// Perform the actual load
	GEN_LWZ(ppc, value.lo, MIPS_GET_IMMED(mips), base);
	set_next_dst(ppc);
	// Zero out the upper word
	GEN_LI(ppc, value.hi, 0, 0);
	set_next_dst(ppc);
	// Write the value out to reg
	flushRegisters();
	// Skip over else
	int not_fastmem_id = add_jump_special(1);
	GEN_B(ppc, not_fastmem_id, 0, 0);
	set_next_dst(ppc);
	PowerPC_instr* preCall = get_curr_dst();
#endif // FASTMEM

	// load into rt
	GEN_LI(ppc, 3, 0, MIPS_GET_RT(mips));
	set_next_dst(ppc);

	genCallDynaMem(MEM_LWU, base, MIPS_GET_IMMED(mips));

#ifdef FASTMEM
	int callSize = get_curr_dst() - preCall;
	set_jump_special(not_fastmem_id, callSize+1);
#endif

	return CONVERT_SUCCESS;
#endif
}

static int SB(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_SB
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_SB

	flushRegisters();
	reset_code_addr();

	if( MIPS_GET_RT(mips) ){
		mapRegister( MIPS_GET_RT(mips) ); // r3 = value
	} else {
		mapRegisterTemp();
		GEN_LI(ppc, 3, 0, 0); // r3 = 0
		set_next_dst(ppc);
	}
	int base = mapRegister( MIPS_GET_RS(mips) ); // r4 = addr

	invalidateRegisters();

	genCallDynaMem(MEM_SB, base, MIPS_GET_IMMED(mips));

	return CONVERT_SUCCESS;
#endif
}

static int SH(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_SH
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_SH

	flushRegisters();
	reset_code_addr();

	if( MIPS_GET_RT(mips) ){
		mapRegister( MIPS_GET_RT(mips) ); // r3 = value
	} else {
		mapRegisterTemp();
		GEN_LI(ppc, 3, 0, 0); // r3 = 0
		set_next_dst(ppc);
	}
	int base = mapRegister( MIPS_GET_RS(mips) ); // r4 = addr

	invalidateRegisters();

	genCallDynaMem(MEM_SH, base, MIPS_GET_IMMED(mips));

	return CONVERT_SUCCESS;
#endif
}

static int SWL(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_SWL
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_SWL
	// TODO: swl
	return CONVERT_ERROR;
#endif
}

static int SW(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_SW
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_SW

	flushRegisters();
	reset_code_addr();

	if( MIPS_GET_RT(mips) ){
		mapRegister( MIPS_GET_RT(mips) ); // r3 = value
	} else {
		mapRegisterTemp();
		GEN_LI(ppc, 3, 0, 0); // r3 = 0
		set_next_dst(ppc);
	}
	int base = mapRegister( MIPS_GET_RS(mips) ); // r4 = addr

	invalidateRegisters();

	genCallDynaMem(MEM_SW, base, MIPS_GET_IMMED(mips));

	return CONVERT_SUCCESS;
#endif
}

static int SDL(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_SDL
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_SDL
	// TODO: sdl
	return CONVERT_ERROR;
#endif
}

static int SDR(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_SDR
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_SDR
	// TODO: sdr
	return CONVERT_ERROR;
#endif
}

static int SWR(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_SWR
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_SWR
	// TODO: swr
	return CONVERT_ERROR;
#endif
}

static int LD(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_LD
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_LD

	flushRegisters();
	reset_code_addr();

	int rd = mapRegisterTemp(); // r3 = rd
	int base = mapRegister( MIPS_GET_RS(mips) ); // r4 = addr

	invalidateRegisters();

#ifdef FASTMEM
	// If base in physical memory
#ifdef USE_EXPANSION
	GEN_LIS(ppc, 0, 0x8080);
#else
	GEN_LIS(ppc, 0, 0x8040);
#endif
	set_next_dst(ppc);
	GEN_CMP(ppc, base, 0, 1);
	set_next_dst(ppc);
	GEN_BGE(ppc, 1, 8, 0, 0);
	set_next_dst(ppc);

	// Use rdram
#ifdef USE_EXPANSION
	// Mask sp with 0x007FFFFF
	GEN_RLWINM(ppc, base, base, 0, 9, 31);
	set_next_dst(ppc);
#else
	// Mask sp with 0x003FFFFF
	GEN_RLWINM(ppc, base, base, 0, 10, 31);
	set_next_dst(ppc);
#endif
	// Add rdram pointer
	GEN_ADD(ppc, base, DYNAREG_RDRAM, base);
	set_next_dst(ppc);
	// Create a mapping for this value
	RegMapping value = mapRegister64New( MIPS_GET_RT(mips) );
	// Perform the actual load
	GEN_LWZ(ppc, value.lo, MIPS_GET_IMMED(mips)+4, base);
	set_next_dst(ppc);
	GEN_LWZ(ppc, value.hi, MIPS_GET_IMMED(mips), base);
	set_next_dst(ppc);
	// Write the value out to reg
	flushRegisters();
	// Skip over else
	int not_fastmem_id = add_jump_special(1);
	GEN_B(ppc, not_fastmem_id, 0, 0);
	set_next_dst(ppc);
	PowerPC_instr* preCall = get_curr_dst();
#endif // FASTMEM

	// load into rt
	GEN_LI(ppc, 3, 0, MIPS_GET_RT(mips));
	set_next_dst(ppc);

	genCallDynaMem(MEM_LD, base, MIPS_GET_IMMED(mips));

#ifdef FASTMEM
	int callSize = get_curr_dst() - preCall;
	set_jump_special(not_fastmem_id, callSize+1);
#endif

	return CONVERT_SUCCESS;
#endif
}

static int SD(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_SD
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_SD

	flushRegisters();
	reset_code_addr();

	int rd = mapRegisterTemp(); // r3 = rd
	int base = mapRegister( MIPS_GET_RS(mips) ); // r4 = addr

	invalidateRegisters();

	// store from rt
	GEN_LI(ppc, 3, 0, MIPS_GET_RT(mips));
	set_next_dst(ppc);

	genCallDynaMem(MEM_SD, base, MIPS_GET_IMMED(mips));

	return CONVERT_SUCCESS;
#endif
}

static int LWC1(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_LWC1
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_LWC1

	flushRegisters();
	reset_code_addr();
	
	genCheckFP();

	int rd = mapRegisterTemp(); // r3 = rd
	int base = mapRegister( MIPS_GET_RS(mips) ); // r4 = addr
	int addr = mapRegisterTemp(); // r5 = fpr_addr

	invalidateRegisters();

#ifdef FASTMEM
	// If base in physical memory
#ifdef USE_EXPANSION
	GEN_LIS(ppc, 0, 0x8080);
#else
	GEN_LIS(ppc, 0, 0x8040);
#endif
	set_next_dst(ppc);
	GEN_CMP(ppc, base, 0, 1);
	set_next_dst(ppc);
	GEN_BGE(ppc, 1, 7, 0, 0);
	set_next_dst(ppc);

	// Use rdram
#ifdef USE_EXPANSION
	// Mask sp with 0x007FFFFF
	GEN_RLWINM(ppc, base, base, 0, 9, 31);
	set_next_dst(ppc);
#else
	// Mask sp with 0x003FFFFF
	GEN_RLWINM(ppc, base, base, 0, 10, 31);
	set_next_dst(ppc);
#endif
	// Add rdram pointer
	GEN_ADD(ppc, base, DYNAREG_RDRAM, base);
	set_next_dst(ppc);
	// Perform the actual load
	GEN_LWZ(ppc, 3, MIPS_GET_IMMED(mips), base);
	set_next_dst(ppc);
	// addr = reg_cop1_simple[frt]
	GEN_LWZ(ppc, addr, MIPS_GET_RT(mips)*4, DYNAREG_FPR_32);
	set_next_dst(ppc);
	// *addr = frs
	GEN_STW(ppc, 3, 0, addr);
	set_next_dst(ppc);
	// Skip over else
	int not_fastmem_id = add_jump_special(1);
	GEN_B(ppc, not_fastmem_id, 0, 0);
	set_next_dst(ppc);
	PowerPC_instr* preCall = get_curr_dst();
#endif // FASTMEM

	// load into frt
	GEN_LI(ppc, 3, 0, MIPS_GET_RT(mips));
	set_next_dst(ppc);

	genCallDynaMem(MEM_LWC1, base, MIPS_GET_IMMED(mips));

#ifdef FASTMEM
	int callSize = get_curr_dst() - preCall;
	set_jump_special(not_fastmem_id, callSize+1);
#endif

	return CONVERT_SUCCESS;
#endif
}

static int LDC1(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_LDC1
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_LDC1

	flushRegisters();
	reset_code_addr();
	
	genCheckFP();

	int rd = mapRegisterTemp(); // r3 = rd
	int base = mapRegister( MIPS_GET_RS(mips) ); // r4 = addr
	int addr = mapRegisterTemp(); // r5 = fpr_addr

	invalidateRegisters();

#ifdef FASTMEM
	// If base in physical memory
#ifdef USE_EXPANSION
	GEN_LIS(ppc, 0, 0x8080);
#else
	GEN_LIS(ppc, 0, 0x8040);
#endif
	set_next_dst(ppc);
	GEN_CMP(ppc, base, 0, 1);
	set_next_dst(ppc);
	GEN_BGE(ppc, 1, 9, 0, 0);
	set_next_dst(ppc);

	// Use rdram
#ifdef USE_EXPANSION
	// Mask sp with 0x007FFFFF
	GEN_RLWINM(ppc, base, base, 0, 9, 31);
	set_next_dst(ppc);
#else
	// Mask sp with 0x003FFFFF
	GEN_RLWINM(ppc, base, base, 0, 10, 31);
	set_next_dst(ppc);
#endif
	// Add rdram pointer
	GEN_ADD(ppc, base, DYNAREG_RDRAM, base);
	set_next_dst(ppc);
	// Perform the actual load
	GEN_LWZ(ppc, 3, MIPS_GET_IMMED(mips), base);
	set_next_dst(ppc);
	GEN_LWZ(ppc, 6, MIPS_GET_IMMED(mips)+4, base);
	set_next_dst(ppc);
	// addr = reg_cop1_double[frt]
	GEN_LWZ(ppc, addr, MIPS_GET_RT(mips)*4, DYNAREG_FPR_64);
	set_next_dst(ppc);
	// *addr = frs
	GEN_STW(ppc, 3, 0, addr);
	set_next_dst(ppc);
	GEN_STW(ppc, 6, 4, addr);
	set_next_dst(ppc);
	// Skip over else
	int not_fastmem_id = add_jump_special(1);
	GEN_B(ppc, not_fastmem_id, 0, 0);
	set_next_dst(ppc);
	PowerPC_instr* preCall = get_curr_dst();
#endif // FASTMEM

	// load into frt
	GEN_LI(ppc, 3, 0, MIPS_GET_RT(mips));
	set_next_dst(ppc);

	genCallDynaMem(MEM_LDC1, base, MIPS_GET_IMMED(mips));

#ifdef FASTMEM
	int callSize = get_curr_dst() - preCall;
	set_jump_special(not_fastmem_id, callSize+1);
#endif

	return CONVERT_SUCCESS;
#endif
}

static int SWC1(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_SWC1
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_SWC1

	flushRegisters();
	reset_code_addr();

	genCheckFP();

	int rd = mapRegisterTemp(); // r3 = rd
	int base = mapRegister( MIPS_GET_RS(mips) ); // r4 = addr

	invalidateRegisters();

	// store from rt
	GEN_LI(ppc, 3, 0, MIPS_GET_RT(mips));
	set_next_dst(ppc);

	genCallDynaMem(MEM_SWC1, base, MIPS_GET_IMMED(mips));

	return CONVERT_SUCCESS;
#endif
}

static int SDC1(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_SDC1
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_SDC1

	flushRegisters();
	reset_code_addr();

	genCheckFP();

	int rd = mapRegisterTemp(); // r3 = rd
	int base = mapRegister( MIPS_GET_RS(mips) ); // r4 = addr

	invalidateRegisters();

	// store from rt
	GEN_LI(ppc, 3, 0, MIPS_GET_RT(mips));
	set_next_dst(ppc);

	genCallDynaMem(MEM_SDC1, base, MIPS_GET_IMMED(mips));

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
	// TODO: ll
	return CONVERT_ERROR;
#endif
}

static int SC(MIPS_instr mips){
	PowerPC_instr ppc;
#ifdef INTERPRET_SC
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_SC
	// TODO: sc
	return CONVERT_ERROR;
#endif
}

// -- Special Functions --

static int SLL(MIPS_instr mips){
	PowerPC_instr ppc;

	int rt = mapRegister( MIPS_GET_RT(mips) );
	int rd = mapRegisterNew( MIPS_GET_RD(mips) );

	GEN_SLWI(ppc, rd, rt, MIPS_GET_SA(mips));
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
}

static int SRL(MIPS_instr mips){
	PowerPC_instr ppc;

	int rt = mapRegister( MIPS_GET_RT(mips) );
	int rd = mapRegisterNew( MIPS_GET_RD(mips) );

	GEN_SRWI(ppc, rd, rt, MIPS_GET_SA(mips));
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
}

static int SRA(MIPS_instr mips){
	PowerPC_instr ppc;

	int rt = mapRegister( MIPS_GET_RT(mips) );
	int rd = mapRegisterNew( MIPS_GET_RD(mips) );

	GEN_SRAWI(ppc, rd, rt, MIPS_GET_SA(mips));
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
}

static int SLLV(MIPS_instr mips){
	PowerPC_instr ppc;
	int rt = mapRegister( MIPS_GET_RT(mips) );
	int rs = mapRegister( MIPS_GET_RS(mips) );
	int rd = mapRegisterNew( MIPS_GET_RD(mips) );
	GEN_RLWINM(ppc, 0, rs, 0, 27, 31); // Mask the lower 5-bits of rs
	set_next_dst(ppc);
	GEN_SLW(ppc, rd, rt, 0);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
}

static int SRLV(MIPS_instr mips){
	PowerPC_instr ppc;
	int rt = mapRegister( MIPS_GET_RT(mips) );
	int rs = mapRegister( MIPS_GET_RS(mips) );
	int rd = mapRegisterNew( MIPS_GET_RD(mips) );
	GEN_RLWINM(ppc, 0, rs, 0, 27, 31); // Mask the lower 5-bits of rs
	set_next_dst(ppc);
	GEN_SRW(ppc, rd, rt, 0);
	set_next_dst(ppc);

	return CONVERT_SUCCESS;
}

static int SRAV(MIPS_instr mips){
	PowerPC_instr ppc;
	int rt = mapRegister( MIPS_GET_RT(mips) );
	int rs = mapRegister( MIPS_GET_RS(mips) );
	int rd = mapRegisterNew( MIPS_GET_RD(mips) );
	GEN_RLWINM(ppc, 0, rs, 0, 27, 31); // Mask the lower 5-bits of rs
	set_next_dst(ppc);
	GEN_SRAW(ppc, rd, rt, 0);
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

	// Check the delay slot, and note how big it is
	PowerPC_instr* preDelay = get_curr_dst();
	check_delaySlot();
	int delaySlot = get_curr_dst() - preDelay;

#ifdef COMPARE_CORE
	GEN_LI(ppc, 4, 0, 1);
	set_next_dst(ppc);
#endif
	genUpdateCount(0);

#ifdef INTERPRET_JR
	genJumpTo(MIPS_GET_RS(mips), JUMPTO_REG);
#else // INTERPRET_JR
	// TODO: jr
#endif

	// Let's still recompile the delay slot in place in case its branched to
	if(delaySlot){ if(is_j_dst()){ unget_last_src(); delaySlotNext = 2; } }
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

	// Check the delay slot, and note how big it is
	PowerPC_instr* preDelay = get_curr_dst();
	check_delaySlot();
	int delaySlot = get_curr_dst() - preDelay;

#ifdef COMPARE_CORE
	GEN_LI(ppc, 4, 0, 1);
	set_next_dst(ppc);
#endif
	genUpdateCount(0);

	// Set LR to next instruction
	int rd = mapRegisterNew(MIPS_GET_RD(mips));
	// lis	lr, pc@ha(0)
	GEN_LIS(ppc, rd, (get_src_pc()+4)>>16);
	set_next_dst(ppc);
	// la	lr, pc@l(lr)
	GEN_ORI(ppc, rd, rd, get_src_pc()+4);
	set_next_dst(ppc);

	flushRegisters();

#ifdef INTERPRET_JALR
	genJumpTo(MIPS_GET_RS(mips), JUMPTO_REG);
#else // INTERPRET_JALR
	// TODO: jalr
#endif

	// Let's still recompile the delay slot in place in case its branched to
	if(delaySlot){ if(is_j_dst()){ unget_last_src(); delaySlotNext = 2; } }
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
	GEN_OR(ppc, rd.lo, hi.lo, hi.lo);
	set_next_dst(ppc);
	GEN_OR(ppc, rd.hi, hi.hi, hi.hi);
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
	GEN_OR(ppc, hi.lo, rs.lo, rs.lo);
	set_next_dst(ppc);
	GEN_OR(ppc, hi.hi, rs.hi, rs.hi);
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
	GEN_OR(ppc, rd.lo, lo.lo, lo.lo);
	set_next_dst(ppc);
	GEN_OR(ppc, rd.hi, lo.hi, lo.hi);
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
	GEN_OR(ppc, lo.lo, rs.lo, rs.lo);
	set_next_dst(ppc);
	GEN_OR(ppc, lo.hi, rs.hi, rs.hi);
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
	int hi = mapRegisterNew( MIPS_REG_HI );
	int lo = mapRegisterNew( MIPS_REG_LO );

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
		GEN_LI(ppc, lo, 0, 0);
		set_next_dst(ppc);
		// li hi, 0
		GEN_LI(ppc, hi, 0, 0);
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
	int hi = mapRegisterNew( MIPS_REG_HI );
	int lo = mapRegisterNew( MIPS_REG_LO );

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
		GEN_LI(ppc, lo, 0, 0);
		set_next_dst(ppc);
		// li hi, 0
		GEN_LI(ppc, hi, 0, 0);
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
	int hi = mapRegisterNew( MIPS_REG_HI );
	int lo = mapRegisterNew( MIPS_REG_LO );

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
	int hi = mapRegisterNew( MIPS_REG_HI );
	int lo = mapRegisterNew( MIPS_REG_LO );

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
	int sa = mapRegisterTemp();
	RegMapping rt = mapRegister64( MIPS_GET_RT(mips) );
	RegMapping rd = mapRegister64New( MIPS_GET_RD(mips) );

	// Mask off the shift amount (0x3f)
	GEN_RLWINM(ppc, sa, rs, 0, 26, 31);
	set_next_dst(ppc);
	// Shift the MSW
	GEN_SLW(ppc, rd.hi, rt.hi, sa);
	set_next_dst(ppc);
	// Calculate 32-sh
	GEN_SUBFIC(ppc, 0, sa, 32);
	set_next_dst(ppc);
	// Extract the bits that will be shifted out the LSW (sh < 32)
	GEN_SRW(ppc, 0, rt.lo, 0);
	set_next_dst(ppc);
	// Insert the bits into the MSW
	GEN_OR(ppc, rd.hi, rd.hi, 0);
	set_next_dst(ppc);
	// Calculate sh-32
	GEN_ADDI(ppc, 0, sa, -32);
	set_next_dst(ppc);
	// Extract the bits that will be shifted out the LSW (sh > 31)
	GEN_SLW(ppc, 0, rt.lo, 0);
	set_next_dst(ppc);
	// Insert the bits into the MSW
	GEN_OR(ppc, rd.hi, rd.hi, 0);
	set_next_dst(ppc);
	// Shift the LSW
	GEN_SLW(ppc, rd.lo, rt.lo, sa);
	set_next_dst(ppc);

	unmapRegisterTemp(sa);

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
	int sa = mapRegisterTemp();
	RegMapping rt = mapRegister64( MIPS_GET_RT(mips) );
	RegMapping rd = mapRegister64New( MIPS_GET_RD(mips) );

	// Mask off the shift amount (0x3f)
	GEN_RLWINM(ppc, sa, rs, 0, 26, 31);
	set_next_dst(ppc);
	// Shift the LSW
	GEN_SRW(ppc, rd.lo, rt.lo, sa);
	set_next_dst(ppc);
	// Calculate 32-sh
	GEN_SUBFIC(ppc, 0, sa, 32);
	set_next_dst(ppc);
	// Extract the bits that will be shifted out the MSW (sh < 32)
	GEN_SLW(ppc, 0, rt.hi, 0);
	set_next_dst(ppc);
	// Insert the bits into the LSW
	GEN_OR(ppc, rd.lo, rd.lo, 0);
	set_next_dst(ppc);
	// Calculate sh-32
	GEN_ADDI(ppc, 0, sa, -32);
	set_next_dst(ppc);
	// Extract the bits that will be shifted out the MSW (sh > 31)
	GEN_SRW(ppc, 0, rt.hi, 0);
	set_next_dst(ppc);
	// Insert the bits into the LSW
	GEN_OR(ppc, rd.lo, rd.lo, 0);
	set_next_dst(ppc);
	// Shift the MSW
	GEN_SRW(ppc, rd.hi, rt.hi, sa);
	set_next_dst(ppc);

	unmapRegisterTemp(sa);

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
	int sa = mapRegisterTemp();
	RegMapping rt = mapRegister64( MIPS_GET_RT(mips) );
	RegMapping rd = mapRegister64New( MIPS_GET_RD(mips) );

	// Mask off the shift amount (0x3f)
	GEN_RLWINM(ppc, sa, rs, 0, 26, 31);
	set_next_dst(ppc);
	// Check whether the shift amount is < 32
	GEN_CMPI(ppc, sa, 32, 1);
	set_next_dst(ppc);
	// Shift the LSW
	GEN_SRW(ppc, rd.lo, rt.lo, sa);
	set_next_dst(ppc);
	// Skip over this code if sh >= 32
	GEN_BGE(ppc, 1, 5, 0, 0);
	set_next_dst(ppc);
	// Calculate 32-sh
	GEN_SUBFIC(ppc, 0, sa, 32);
	set_next_dst(ppc);
	// Extract the bits that will be shifted out the MSW (sh < 32)
	GEN_SLW(ppc, 0, rt.hi, 0);
	set_next_dst(ppc);
	// Insert the bits into the LSW
	GEN_OR(ppc, rd.lo, rd.lo, 0);
	set_next_dst(ppc);
	// Skip over the else
	GEN_B(ppc, 4, 0, 0);
	set_next_dst(ppc);
	// Calculate sh-32
	GEN_ADDI(ppc, 0, sa, -32);
	set_next_dst(ppc);
	// Extract the bits that will be shifted out the MSW (sh > 31)
	GEN_SRAW(ppc, 0, rt.hi, 0);
	set_next_dst(ppc);
	// Insert the bits into the LSW
	GEN_OR(ppc, rd.lo, rd.lo, 0);
	set_next_dst(ppc);
	// Shift the MSW
	GEN_SRAW(ppc, rd.hi, rt.hi, sa);
	set_next_dst(ppc);

	unmapRegisterTemp(sa);

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
	// TODO: ddiv
	return CONVERT_ERROR;
#endif
}

static int DDIVU(MIPS_instr mips){
	PowerPC_instr ppc;
#if defined(INTERPRET_DW) || defined(INTERPRET_DDIVU)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_DW || INTERPRET_DDIVU
	// TODO: ddivu
	return CONVERT_ERROR;
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
		GEN_RLWINM(ppc, 0, rt.lo, sa, 32-sa, 31);
		set_next_dst(ppc);
		// Insert those bits into the MSW
		GEN_OR(ppc, rd.hi, rd.hi, 0);
		set_next_dst(ppc);
		// Shift LSW left by SA
		GEN_SLWI(ppc, rd.lo, rt.lo, sa);
		set_next_dst(ppc);
	} else {
		// Copy over the register
		GEN_ADDI(ppc, rd.hi, rt.hi, 0);
		set_next_dst(ppc);
		GEN_ADDI(ppc, rd.lo, rt.lo, 0);
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
		GEN_RLWINM(ppc, 0, rt.hi, 32-sa, 0, sa-1);
		set_next_dst(ppc);
		// Insert those bits into the LSW
		GEN_OR(ppc, rd.lo, rd.lo, 0);
		set_next_dst(ppc);
		// Shift MSW right by SA
		GEN_SRWI(ppc, rd.hi, rt.hi, sa);
		set_next_dst(ppc);
	} else {
		// Copy over the register
		GEN_ADDI(ppc, rd.hi, rt.hi, 0);
		set_next_dst(ppc);
		GEN_ADDI(ppc, rd.lo, rt.lo, 0);
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
		GEN_RLWINM(ppc, 0, rt.hi, 32-sa, 0, sa-1);
		set_next_dst(ppc);
		// Insert those bits into the LSW
		GEN_OR(ppc, rd.lo, rd.lo, 0);
		set_next_dst(ppc);
		// Shift (arithmetically) MSW right by SA
		GEN_SRAWI(ppc, rd.hi, rt.hi, sa);
		set_next_dst(ppc);
	} else {
		// Copy over the register
		GEN_ADDI(ppc, rd.hi, rt.hi, 0);
		set_next_dst(ppc);
		GEN_ADDI(ppc, rd.lo, rt.lo, 0);
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
	GEN_ADDI(ppc, rd.lo, 0, 0);
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
	GEN_ADDI(ppc, rd.hi, 0, 0);
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
	GEN_ADD(ppc,
	        mapRegisterNew( MIPS_GET_RD(mips) ),
	        rs,
	        rt);
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
	GEN_SUB(ppc,
	        mapRegisterNew( MIPS_GET_RD(mips) ),
	        rs,
	        rt);
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
	int rd = mapRegisterNew( MIPS_GET_RD(mips) );

	// carry = rs < rt ? 0 : 1 (unsigned)
	GEN_SUBFC(ppc, 0, rt, rs);
	set_next_dst(ppc);
	// rd = ~(rs ^ rt)
	GEN_EQV(ppc, rd, rt, rs);
	set_next_dst(ppc);
	// rd = sign(rs) == sign(rt) ? 1 : 0
	GEN_SRWI(ppc, rd, rd, 31);
	set_next_dst(ppc);
	// rd += carry
	GEN_ADDZE(ppc, rd, rd);
	set_next_dst(ppc);
	// rt &= 1 ( = (sign(rs) == sign(rt)) xor (rs < rt (unsigned)) )
	GEN_RLWINM(ppc, rd, rd, 0, 31, 31);
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
	int rd = mapRegisterNew( MIPS_GET_RD(mips) );
	// carry = rs < rt ? 0 : 1
	GEN_SUBFC(ppc, rd, rt, rs);
	set_next_dst(ppc);
	// rd = carry - 1 ( = rs < rt ? -1 : 0 )
	GEN_SUBFE(ppc, rd, rd, rd);
	set_next_dst(ppc);
	// rd = !carry ( = rs < rt ? 1 : 0 )
	GEN_NEG(ppc, rd, rd);
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

	genCmpi64(4, MIPS_GET_RA(mips), 0);

	return branch(signExtend(MIPS_GET_IMMED(mips),16),
	              cond ? GE : LT, link, likely);
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
	GEN_LWZ(ppc, 3, 12*4, DYNAREG_COP0);
	set_next_dst(ppc);
	// Load upper address of llbit
	GEN_LIS(ppc, 4, extractUpper16(&llbit));
	set_next_dst(ppc);
	// Status & 0xFFFFFFFD
	GEN_RLWINM(ppc, 3, 3, 0, 31, 29);
	set_next_dst(ppc);
	// llbit = 0
	GEN_STW(ppc, DYNAREG_ZERO, extractLower16(&llbit), 4);
	set_next_dst(ppc);
	// Store updated Status
	GEN_STW(ppc, 3, 12*4, DYNAREG_COP0);
	set_next_dst(ppc);
	// check_interupt()
	GEN_B(ppc, add_jump(&check_interupt, 1, 1), 0, 1);
	set_next_dst(ppc);
	// Load the old LR
	GEN_LWZ(ppc, 0, DYNAOFF_LR, 1);
	set_next_dst(ppc);
	// interp_addr = EPC
	GEN_LWZ(ppc, 3, 14*4, DYNAREG_COP0);
	set_next_dst(ppc);
	// Restore the LR
	GEN_MTLR(ppc, 0);
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

	int rt = mapRegisterNew(MIPS_GET_RT(mips));
	// *rt = reg_cop0[rd]
	GEN_LWZ(ppc, rt, MIPS_GET_RD(mips)*4, DYNAREG_COP0);
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
	int tmp;
	
	switch(rd){
	case 0: // Index
		rrt = mapRegister(rt);
		// r0 = rt & 0x8000003F
		GEN_RLWINM(ppc, 0, rrt, 0, 26, 0);
		set_next_dst(ppc);
		// reg_cop0[rd] = r0
		GEN_STW(ppc, 0, rd*4, DYNAREG_COP0);
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
	
	case 2: // EntryLo0
	case 3: // EntryLo1
		rrt = mapRegister(rt);
		// r0 = rt & 0x3FFFFFFF
		GEN_RLWINM(ppc, 0, rrt, 0, 2, 31);
		set_next_dst(ppc);
		// reg_cop0[rd] = r0
		GEN_STW(ppc, 0, rd*4, DYNAREG_COP0);
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
	
	case 4: // Context
		rrt = mapRegister(rt), tmp = mapRegisterTemp();
		// tmp = reg_cop0[rd]
		GEN_LWZ(ppc, tmp, rd*4, DYNAREG_COP0);
		set_next_dst(ppc);
		// r0 = rt & 0xFF800000
		GEN_RLWINM(ppc, 0, rrt, 0, 0, 8);
		set_next_dst(ppc);
		// tmp &= 0x007FFFF0
		GEN_RLWINM(ppc, tmp, tmp, 0, 9, 27);
		set_next_dst(ppc);
		// tmp |= r0
		GEN_OR(ppc, tmp, tmp, 0);
		set_next_dst(ppc);
		// reg_cop0[rd] = tmp
		GEN_STW(ppc, tmp, rd*4, DYNAREG_COP0);
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
	
	case 5: // PageMask
		rrt = mapRegister(rt);
		// r0 = rt & 0x01FFE000
		GEN_RLWINM(ppc, 0, rrt, 0, 7, 18);
		set_next_dst(ppc);
		// reg_cop0[rd] = r0
		GEN_STW(ppc, 0, rd*4, DYNAREG_COP0);
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
	
	case 6: // Wired
		rrt = mapRegister(rt);
		// r0 = 31
		GEN_ADDI(ppc, 0, 0, 31);
		set_next_dst(ppc);
		// reg_cop0[rd] = rt
		GEN_STW(ppc, rrt, rd*4, DYNAREG_COP0);
		set_next_dst(ppc);
		// reg_cop0[1] = r0
		GEN_STW(ppc, 0, 1*4, DYNAREG_COP0);
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
	
	case 10: // EntryHi
		rrt = mapRegister(rt);
		// r0 = rt & 0xFFFFE0FF
		GEN_RLWINM(ppc, 0, rrt, 0, 24, 18);
		set_next_dst(ppc);
		// reg_cop0[rd] = r0
		GEN_STW(ppc, 0, rd*4, DYNAREG_COP0);
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
	
	case 13: // Cause
		rrt = mapRegister(rt);
		// TODO: Ensure that rrt == 0?
		// reg_cop0[rd] = rt
		GEN_STW(ppc, rrt, rd*4, DYNAREG_COP0);
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
	
	case 14: // EPC
	case 16: // Config
	case 18: // WatchLo
	case 19: // WatchHi
		rrt = mapRegister(rt);
		// reg_cop0[rd] = rt
		GEN_STW(ppc, rrt, rd*4, DYNAREG_COP0);
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
	
	case 28: // TagLo
		rrt = mapRegister(rt);
		// r0 = rt & 0x0FFFFFC0
		GEN_RLWINM(ppc, 0, rrt, 0, 4, 25);
		set_next_dst(ppc);
		// reg_cop0[rd] = r0
		GEN_STW(ppc, 0, rd*4, DYNAREG_COP0);
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
	
	case 29: // TagHi
		// reg_cop0[rd] = 0
		GEN_STW(ppc, DYNAREG_ZERO, rd*4, DYNAREG_COP0);
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
	int rt = mapRegisterNew( MIPS_GET_RT(mips) );
	flushFPR(fs);

	// rt = reg_cop1_simple[fs]
	GEN_LWZ(ppc, rt, fs*4, DYNAREG_FPR_32);
	set_next_dst(ppc);
	// rt = *rt
	GEN_LWZ(ppc, rt, 0, rt);
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
	int addr = mapRegisterTemp();
	flushFPR(fs);

	// addr = reg_cop1_double[fs]
	GEN_LWZ(ppc, addr, fs*4, DYNAREG_FPR_64);
	set_next_dst(ppc);
	// rt[hi] = *addr
	GEN_LWZ(ppc, rt.hi, 0, addr);
	set_next_dst(ppc);
	// rt[lo] = *(addr+4)
	GEN_LWZ(ppc, rt.lo, 4, addr);
	set_next_dst(ppc);

	unmapRegisterTemp(addr);

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
		int rt = mapRegisterNew( MIPS_GET_RT(mips) );

		GEN_LWZ(ppc, rt, 0, DYNAREG_FCR31);
		set_next_dst(ppc);
	} else if(MIPS_GET_FS(mips) == 0){
		int rt = mapRegisterNew( MIPS_GET_RT(mips) );

		GEN_LI(ppc, rt, 0, 0x511);
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
	int addr = mapRegisterTemp();
	invalidateFPR(fs);

	// addr = reg_cop1_simple[fs]
	GEN_LWZ(ppc, addr, fs*4, DYNAREG_FPR_32);
	set_next_dst(ppc);
	// *addr = rt
	GEN_STW(ppc, rt, 0, addr);
	set_next_dst(ppc);

	unmapRegisterTemp(addr);

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
	int addr = mapRegisterTemp();
	invalidateFPR(fs);

	GEN_LWZ(ppc, addr, fs*4, DYNAREG_FPR_64);
	set_next_dst(ppc);
	GEN_STW(ppc, rt.hi, 0, addr);
	set_next_dst(ppc);
	GEN_STW(ppc, rt.lo, 4, addr);
	set_next_dst(ppc);

	unmapRegisterTemp(addr);

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

		GEN_STW(ppc, rt, 0, DYNAREG_FCR31);
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

	GEN_LWZ(ppc, 0, 0, DYNAREG_FCR31);
	set_next_dst(ppc);
	GEN_RLWINM(ppc, 0, 0, 9, 31, 31);
	set_next_dst(ppc);
	GEN_CMPI(ppc, 0, 0, 4);
	set_next_dst(ppc);

	return branch(signExtend(MIPS_GET_IMMED(mips),16), cond?NE:EQ, 0, likely);
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
	GEN_B(ppc, add_jump(dbl ? &sqrt : &sqrtf, 1, 1), 0, 1);
	set_next_dst(ppc);

	mapFPRNew( MIPS_GET_FD(mips), dbl ); // maps to f1 (FP return)

	// Load old LR
	GEN_LWZ(ppc, 0, DYNAOFF_LR, 1);
	set_next_dst(ppc);
	// Restore LR
	GEN_MTLR(ppc, 0);
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

	GEN_MTFSFI(ppc, 7, rounding_mode);
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
	
	int addr = 5; // Use r5 for the addr (to not clobber r3/r4)
	// addr = reg_cop1_double[fd]
	GEN_LWZ(ppc, addr, fd*4, DYNAREG_FPR_64);
	set_next_dst(ppc);
	// Load old LR
	GEN_LWZ(ppc, 0, DYNAOFF_LR, 1);
	set_next_dst(ppc);
	// stw r3, 0(addr)
	GEN_STW(ppc, 3, 0, addr);
	set_next_dst(ppc);
	// Restore LR
	GEN_MTLR(ppc, 0);
	set_next_dst(ppc);
	// stw r4, 4(addr)
	GEN_STW(ppc, 4, 4, addr);
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
	
	int addr = 5; // Use r5 for the addr (to not clobber r3/r4)
	// addr = reg_cop1_double[fd]
	GEN_LWZ(ppc, addr, fd*4, DYNAREG_FPR_64);
	set_next_dst(ppc);
	// Load old LR
	GEN_LWZ(ppc, 0, DYNAOFF_LR, 1);
	set_next_dst(ppc);
	// stw r3, 0(addr)
	GEN_STW(ppc, 3, 0, addr);
	set_next_dst(ppc);
	// Restore LR
	GEN_MTLR(ppc, 0);
	set_next_dst(ppc);
	// stw r4, 4(addr)
	GEN_STW(ppc, 4, 4, addr);
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
	
	int addr = 5; // Use r5 for the addr (to not clobber r3/r4)
	// addr = reg_cop1_double[fd]
	GEN_LWZ(ppc, addr, fd*4, DYNAREG_FPR_64);
	set_next_dst(ppc);
	// Load old LR
	GEN_LWZ(ppc, 0, DYNAOFF_LR, 1);
	set_next_dst(ppc);
	// stw r3, 0(addr)
	GEN_STW(ppc, 3, 0, addr);
	set_next_dst(ppc);
	// Restore LR
	GEN_MTLR(ppc, 0);
	set_next_dst(ppc);
	// stw r4, 4(addr)
	GEN_STW(ppc, 4, 4, addr);
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
	
	int addr = 5; // Use r5 for the addr (to not clobber r3/r4)
	// addr = reg_cop1_double[fd]
	GEN_LWZ(ppc, addr, fd*4, DYNAREG_FPR_64);
	set_next_dst(ppc);
	// Load old LR
	GEN_LWZ(ppc, 0, DYNAOFF_LR, 1);
	set_next_dst(ppc);
	// stw r3, 0(addr)
	GEN_STW(ppc, 3, 0, addr);
	set_next_dst(ppc);
	// Restore LR
	GEN_MTLR(ppc, 0);
	set_next_dst(ppc);
	// stw r4, 4(addr)
	GEN_STW(ppc, 4, 4, addr);
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

	set_rounding(PPC_ROUNDING_NEAREST); // TODO: Presume its already set?

	int fd = MIPS_GET_FD(mips);
	int fs = mapFPR( MIPS_GET_FS(mips), dbl );
	invalidateFPR(fd);
	int addr = mapRegisterTemp();

	// fctiw f0, fs
	GEN_FCTIW(ppc, 0, fs);
	set_next_dst(ppc);
	// addr = reg_cop1_simple[fd]
	GEN_LWZ(ppc, addr, fd*4, DYNAREG_FPR_32);
	set_next_dst(ppc);
	// stfiwx f0, 0, addr
	GEN_STFIWX(ppc, 0, 0, addr);
	set_next_dst(ppc);

	unmapRegisterTemp(addr);

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
	int addr = mapRegisterTemp();

	// fctiwz f0, fs
	GEN_FCTIWZ(ppc, 0, fs);
	set_next_dst(ppc);
	// addr = reg_cop1_simple[fd]
	GEN_LWZ(ppc, addr, fd*4, DYNAREG_FPR_32);
	set_next_dst(ppc);
	// stfiwx f0, 0, addr
	GEN_STFIWX(ppc, 0, 0, addr);
	set_next_dst(ppc);

	unmapRegisterTemp(addr);

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
	int addr = mapRegisterTemp();

	// fctiw f0, fs
	GEN_FCTIW(ppc, 0, fs);
	set_next_dst(ppc);
	// addr = reg_cop1_simple[fd]
	GEN_LWZ(ppc, addr, fd*4, DYNAREG_FPR_32);
	set_next_dst(ppc);
	// stfiwx f0, 0, addr
	GEN_STFIWX(ppc, 0, 0, addr);
	set_next_dst(ppc);

	unmapRegisterTemp(addr);
	
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
	int addr = mapRegisterTemp();

	// fctiw f0, fs
	GEN_FCTIW(ppc, 0, fs);
	set_next_dst(ppc);
	// addr = reg_cop1_simple[fd]
	GEN_LWZ(ppc, addr, fd*4, DYNAREG_FPR_32);
	set_next_dst(ppc);
	// stfiwx f0, 0, addr
	GEN_STFIWX(ppc, 0, 0, addr);
	set_next_dst(ppc);

	unmapRegisterTemp(addr);
	
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

	GEN_FMR(ppc, fd, fs);
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
	GEN_LFD(ppc, 0, -4, DYNAREG_FCR31);
	set_next_dst(ppc);

	// FIXME: Here I have the potential to disable IEEE mode
	//          and enable inexact exceptions
	set_rounding_reg(0);

	int fd = MIPS_GET_FD(mips);
	int fs = mapFPR( MIPS_GET_FS(mips), dbl );
	invalidateFPR(fd);
	int addr = mapRegisterTemp();

	// fctiw f0, fs
	GEN_FCTIW(ppc, 0, fs);
	set_next_dst(ppc);
	// addr = reg_cop1_simple[fd]
	GEN_LWZ(ppc, addr, fd*4, DYNAREG_FPR_32);
	set_next_dst(ppc);
	// stfiwx f0, 0, addr
	GEN_STFIWX(ppc, 0, 0, addr);
	set_next_dst(ppc);

	unmapRegisterTemp(addr);
	
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
	
	int addr = 5; // Use r5 for the addr (to not clobber r3/r4)
	// addr = reg_cop1_double[fd]
	GEN_LWZ(ppc, addr, fd*4, DYNAREG_FPR_64);
	set_next_dst(ppc);
	// Load old LR
	GEN_LWZ(ppc, 0, DYNAOFF_LR, 1);
	set_next_dst(ppc);
	// stw r3, 0(addr)
	GEN_STW(ppc, 3, 0, addr);
	set_next_dst(ppc);
	// Restore LR
	GEN_MTLR(ppc, 0);
	set_next_dst(ppc);
	// stw r4, 4(addr)
	GEN_STW(ppc, 4, 4, addr);
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
	GEN_LWZ(ppc, 0, 0, DYNAREG_FCR31);
	set_next_dst(ppc);
	// and r0, r0, 0xff7fffff (clear cond)
	GEN_RLWINM(ppc, 0, 0, 0, 9, 7);
	set_next_dst(ppc);
	// stw r0, 0(&fcr31)
	GEN_STW(ppc, 0, 0, DYNAREG_FCR31);
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

	// lwz r0, 0(&fcr31)
	GEN_LWZ(ppc, 0, 0, DYNAREG_FCR31);
	set_next_dst(ppc);
	// fcmpu cr0, fs, ft
	GEN_FCMPU(ppc, fs, ft, 0);
	set_next_dst(ppc);
	// and r0, r0, 0xff7fffff (clear cond)
	GEN_RLWINM(ppc, 0, 0, 0, 9, 7);
	set_next_dst(ppc);
	// bord cr0, 2 (past setting cond)
	GEN_BC(ppc, 2, 0, 0, 0x4, 3);
	set_next_dst(ppc);
	// oris r0, r0, 0x0080 (set cond)
	GEN_ORIS(ppc, 0, 0, 0x0080);
	set_next_dst(ppc);
	// stw r0, 0(&fcr31)
	GEN_STW(ppc, 0, 0, DYNAREG_FCR31);
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

	// lwz r0, 0(&fcr31)
	GEN_LWZ(ppc, 0, 0, DYNAREG_FCR31);
	set_next_dst(ppc);
	// fcmpu cr0, fs, ft
	GEN_FCMPU(ppc, fs, ft, 0);
	set_next_dst(ppc);
	// and r0, r0, 0xff7fffff (clear cond)
	GEN_RLWINM(ppc, 0, 0, 0, 9, 7);
	set_next_dst(ppc);
	// bne cr0, 2 (past setting cond)
	GEN_BNE(ppc, 0, 2, 0, 0);
	set_next_dst(ppc);
	// oris r0, r0, 0x0080 (set cond)
	GEN_ORIS(ppc, 0, 0, 0x0080);
	set_next_dst(ppc);
	// stw r0, 0(&fcr31)
	GEN_STW(ppc, 0, 0, DYNAREG_FCR31);
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

	// lwz r0, 0(&fcr31)
	GEN_LWZ(ppc, 0, 0, DYNAREG_FCR31);
	set_next_dst(ppc);
	// fcmpu cr0, fs, ft
	GEN_FCMPU(ppc, fs, ft, 0);
	set_next_dst(ppc);
	// and r0, r0, 0xff7fffff (clear cond)
	GEN_RLWINM(ppc, 0, 0, 0, 9, 7);
	set_next_dst(ppc);
	// cror cr0[eq], cr0[eq], cr0[un]
	GEN_CROR(ppc, 2, 2, 3);
	set_next_dst(ppc);
	// bne cr0, 2 (past setting cond)
	GEN_BNE(ppc, 0, 2, 0, 0);
	set_next_dst(ppc);
	// oris r0, r0, 0x0080 (set cond)
	GEN_ORIS(ppc, 0, 0, 0x0080);
	set_next_dst(ppc);
	// stw r0, 0(&fcr31)
	GEN_STW(ppc, 0, 0, DYNAREG_FCR31);
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

	// lwz r0, 0(&fcr31)
	GEN_LWZ(ppc, 0, 0, DYNAREG_FCR31);
	set_next_dst(ppc);
	// fcmpu cr0, fs, ft
	GEN_FCMPU(ppc, fs, ft, 0);
	set_next_dst(ppc);
	// and r0, r0, 0xff7fffff (clear cond)
	GEN_RLWINM(ppc, 0, 0, 0, 9, 7);
	set_next_dst(ppc);
	// bge cr0, 2 (past setting cond)
	GEN_BGE(ppc, 0, 2, 0, 0);
	set_next_dst(ppc);
	// oris r0, r0, 0x0080 (set cond)
	GEN_ORIS(ppc, 0, 0, 0x0080);
	set_next_dst(ppc);
	// stw r0, 0(&fcr31)
	GEN_STW(ppc, 0, 0, DYNAREG_FCR31);
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

	// lwz r0, 0(&fcr31)
	GEN_LWZ(ppc, 0, 0, DYNAREG_FCR31);
	set_next_dst(ppc);
	// fcmpu cr0, fs, ft
	GEN_FCMPU(ppc, fs, ft, 0);
	set_next_dst(ppc);
	// and r0, r0, 0xff7fffff (clear cond)
	GEN_RLWINM(ppc, 0, 0, 0, 9, 7);
	set_next_dst(ppc);
	// cror cr0[lt], cr0[lt], cr0[un]
	GEN_CROR(ppc, 0, 0, 3);
	set_next_dst(ppc);
	// bge cr0, 2 (past setting cond)
	GEN_BGE(ppc, 0, 2, 0, 0);
	set_next_dst(ppc);
	// oris r0, r0, 0x0080 (set cond)
	GEN_ORIS(ppc, 0, 0, 0x0080);
	set_next_dst(ppc);
	// stw r0, 0(&fcr31)
	GEN_STW(ppc, 0, 0, DYNAREG_FCR31);
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

	// lwz r0, 0(&fcr31)
	GEN_LWZ(ppc, 0, 0, DYNAREG_FCR31);
	set_next_dst(ppc);
	// fcmpu cr0, fs, ft
	GEN_FCMPU(ppc, fs, ft, 0);
	set_next_dst(ppc);
	// and r0, r0, 0xff7fffff (clear cond)
	GEN_RLWINM(ppc, 0, 0, 0, 9, 7);
	set_next_dst(ppc);
	// cror cr0[gt], cr0[gt], cr0[un]
	GEN_CROR(ppc, 1, 1, 3);
	set_next_dst(ppc);
	// bgt cr0, 2 (past setting cond)
	GEN_BGT(ppc, 0, 2, 0, 0);
	set_next_dst(ppc);
	// oris r0, r0, 0x0080 (set cond)
	GEN_ORIS(ppc, 0, 0, 0x0080);
	set_next_dst(ppc);
	// stw r0, 0(&fcr31)
	GEN_STW(ppc, 0, 0, DYNAREG_FCR31);
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

	// lwz r0, 0(&fcr31)
	GEN_LWZ(ppc, 0, 0, DYNAREG_FCR31);
	set_next_dst(ppc);
	// fcmpu cr0, fs, ft
	GEN_FCMPU(ppc, fs, ft, 0);
	set_next_dst(ppc);
	// and r0, r0, 0xff7fffff (clear cond)
	GEN_RLWINM(ppc, 0, 0, 0, 9, 7);
	set_next_dst(ppc);
	// bgt cr0, 2 (past setting cond)
	GEN_BGT(ppc, 0, 2, 0, 0);
	set_next_dst(ppc);
	// oris r0, r0, 0x0080 (set cond)
	GEN_ORIS(ppc, 0, 0, 0x0080);
	set_next_dst(ppc);
	// stw r0, 0(&fcr31)
	GEN_STW(ppc, 0, 0, DYNAREG_FCR31);
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
	GEN_LWZ(ppc, 0, 0, DYNAREG_FCR31);
	set_next_dst(ppc);
	// and r0, r0, 0xff7fffff (clear cond)
	GEN_RLWINM(ppc, 0, 0, 0, 9, 7);
	set_next_dst(ppc);
	// stw r0, 0(&fcr31)
	GEN_STW(ppc, 0, 0, DYNAREG_FCR31);
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
	GEN_LWZ(ppc, 0, 0, DYNAREG_FCR31);
	set_next_dst(ppc);
	// and r0, r0, 0xff7fffff (clear cond)
	GEN_RLWINM(ppc, 0, 0, 0, 9, 7);
	set_next_dst(ppc);
	// stw r0, 0(&fcr31)
	GEN_STW(ppc, 0, 0, DYNAREG_FCR31);
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

	// lwz r0, 0(&fcr31)
	GEN_LWZ(ppc, 0, 0, DYNAREG_FCR31);
	set_next_dst(ppc);
	// fcmpu cr0, fs, ft
	GEN_FCMPU(ppc, fs, ft, 0);
	set_next_dst(ppc);
	// and r0, r0, 0xff7fffff (clear cond)
	GEN_RLWINM(ppc, 0, 0, 0, 9, 7);
	set_next_dst(ppc);
	// bne cr0, 2 (past setting cond)
	GEN_BNE(ppc, 0, 2, 0, 0);
	set_next_dst(ppc);
	// oris r0, r0, 0x0080 (set cond)
	GEN_ORIS(ppc, 0, 0, 0x0080);
	set_next_dst(ppc);
	// stw r0, 0(&fcr31)
	GEN_STW(ppc, 0, 0, DYNAREG_FCR31);
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

	// lwz r0, 0(&fcr31)
	GEN_LWZ(ppc, 0, 0, DYNAREG_FCR31);
	set_next_dst(ppc);
	// fcmpu cr0, fs, ft
	GEN_FCMPU(ppc, fs, ft, 0);
	set_next_dst(ppc);
	// and r0, r0, 0xff7fffff (clear cond)
	GEN_RLWINM(ppc, 0, 0, 0, 9, 7);
	set_next_dst(ppc);
	// bne cr0, 2 (past setting cond)
	GEN_BNE(ppc, 0, 2, 0, 0);
	set_next_dst(ppc);
	// oris r0, r0, 0x0080 (set cond)
	GEN_ORIS(ppc, 0, 0, 0x0080);
	set_next_dst(ppc);
	// stw r0, 0(&fcr31)
	GEN_STW(ppc, 0, 0, DYNAREG_FCR31);
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

	// lwz r0, 0(&fcr31)
	GEN_LWZ(ppc, 0, 0, DYNAREG_FCR31);
	set_next_dst(ppc);
	// fcmpu cr0, fs, ft
	GEN_FCMPU(ppc, fs, ft, 0);
	set_next_dst(ppc);
	// and r0, r0, 0xff7fffff (clear cond)
	GEN_RLWINM(ppc, 0, 0, 0, 9, 7);
	set_next_dst(ppc);
	// bge cr0, 2 (past setting cond)
	GEN_BGE(ppc, 0, 2, 0, 0);
	set_next_dst(ppc);
	// oris r0, r0, 0x0080 (set cond)
	GEN_ORIS(ppc, 0, 0, 0x0080);
	set_next_dst(ppc);
	// stw r0, 0(&fcr31)
	GEN_STW(ppc, 0, 0, DYNAREG_FCR31);
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

	// lwz r0, 0(&fcr31)
	GEN_LWZ(ppc, 0, 0, DYNAREG_FCR31);
	set_next_dst(ppc);
	// fcmpu cr0, fs, ft
	GEN_FCMPU(ppc, fs, ft, 0);
	set_next_dst(ppc);
	// and r0, r0, 0xff7fffff (clear cond)
	GEN_RLWINM(ppc, 0, 0, 0, 9, 7);
	set_next_dst(ppc);
	// bge cr0, 2 (past setting cond)
	GEN_BGE(ppc, 0, 2, 0, 0);
	set_next_dst(ppc);
	// oris r0, r0, 0x0080 (set cond)
	GEN_ORIS(ppc, 0, 0, 0x0080);
	set_next_dst(ppc);
	// stw r0, 0(&fcr31)
	GEN_STW(ppc, 0, 0, DYNAREG_FCR31);
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

	// lwz r0, 0(&fcr31)
	GEN_LWZ(ppc, 0, 0, DYNAREG_FCR31);
	set_next_dst(ppc);
	// fcmpu cr0, fs, ft
	GEN_FCMPU(ppc, fs, ft, 0);
	set_next_dst(ppc);
	// and r0, r0, 0xff7fffff (clear cond)
	GEN_RLWINM(ppc, 0, 0, 0, 9, 7);
	set_next_dst(ppc);
	// bgt cr0, 2 (past setting cond)
	GEN_BGT(ppc, 0, 2, 0, 0);
	set_next_dst(ppc);
	// oris r0, r0, 0x0080 (set cond)
	GEN_ORIS(ppc, 0, 0, 0x0080);
	set_next_dst(ppc);
	// stw r0, 0(&fcr31)
	GEN_STW(ppc, 0, 0, DYNAREG_FCR31);
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

	// lwz r0, 0(&fcr31)
	GEN_LWZ(ppc, 0, 0, DYNAREG_FCR31);
	set_next_dst(ppc);
	// fcmpu cr0, fs, ft
	GEN_FCMPU(ppc, fs, ft, 0);
	set_next_dst(ppc);
	// and r0, r0, 0xff7fffff (clear cond)
	GEN_RLWINM(ppc, 0, 0, 0, 9, 7);
	set_next_dst(ppc);
	// bgt cr0, 2 (past setting cond)
	GEN_BGT(ppc, 0, 2, 0, 0);
	set_next_dst(ppc);
	// oris r0, r0, 0x0080 (set cond)
	GEN_ORIS(ppc, 0, 0, 0x0080);
	set_next_dst(ppc);
	// stw r0, 0(&fcr31)
	GEN_STW(ppc, 0, 0, DYNAREG_FCR31);
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

static int CVT_FP_W(MIPS_instr mips, int dbl){
	PowerPC_instr ppc;

	genCheckFP();

	int fs = MIPS_GET_FS(mips);
	flushFPR(fs);
	int fd = mapFPRNew( MIPS_GET_FD(mips), dbl );
	int tmp = mapRegisterTemp();

	// Get the integer value into a GPR
	// tmp = fpr32[fs]
	GEN_LWZ(ppc, tmp, fs*4, DYNAREG_FPR_32);
	set_next_dst(ppc);
	// tmp = *tmp (src)
	GEN_LWZ(ppc, tmp, 0, tmp);
	set_next_dst(ppc);

	// lis r0, 0x4330
	GEN_LIS(ppc, 0, 0x4330);
	set_next_dst(ppc);
	// stw r0, -8(r1)
	GEN_STW(ppc, 0, -8, 1);
	set_next_dst(ppc);
	// lis r0, 0x8000
	GEN_LIS(ppc, 0, 0x8000);
	set_next_dst(ppc);
	// stw r0, -4(r1)
	GEN_STW(ppc, 0, -4, 1);
	set_next_dst(ppc);
	// xor r0, src, 0x80000000
	GEN_XOR(ppc, 0, tmp, 0);
	set_next_dst(ppc);
	// lfd f0, -8(r1)
	GEN_LFD(ppc, 0, -8, 1);
	set_next_dst(ppc);
	// stw r0 -4(r1)
	GEN_STW(ppc, 0, -4, 1);
	set_next_dst(ppc);
	// lfd fd, -8(r1)
	GEN_LFD(ppc, fd, -8, 1);
	set_next_dst(ppc);
	// fsub fd, fd, f0
	GEN_FSUB(ppc, fd, fd, 0, dbl);
	set_next_dst(ppc);

	unmapRegisterTemp(tmp);

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

	flushRegisters();
	int fs = MIPS_GET_FS(mips);
	int fd = mapFPRNew( MIPS_GET_FD(mips), dbl ); // f1
	int hi = mapRegisterTemp(); // r3
	int lo = mapRegisterTemp(); // r4

	// Get the long value into GPRs
	// lo = fpr64[fs]
	GEN_LWZ(ppc, lo, fs*4, DYNAREG_FPR_64);
	set_next_dst(ppc);
	// hi = *lo (hi word)
	GEN_LWZ(ppc, hi, 0, lo);
	set_next_dst(ppc);
	// lo = *(lo+4) (lo word)
	GEN_LWZ(ppc, lo, 4, lo);
	set_next_dst(ppc);

	// convert
	GEN_B(ppc, add_jump(dbl ? &__floatdidf : &__floatdisf, 1, 1), 0, 1);
	set_next_dst(ppc);
	
	// Load old LR
	GEN_LWZ(ppc, 0, DYNAOFF_LR, 1);
	set_next_dst(ppc);
	// Restore LR
	GEN_MTLR(ppc, 0);
	set_next_dst(ppc);
	
	unmapRegisterTemp(hi);
	unmapRegisterTemp(lo);

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
	GEN_LI(ppc, 5, 0, isDelaySlot ? 1 : 0);
	set_next_dst(ppc);
	// Move the address of decodeNInterpret to ctr for a bctr
	//GEN_MTCTR(ppc, DYNAREG_INTERP);
	//set_next_dst(ppc);
	// Load our argument into r3 (mips)
	GEN_LIS(ppc, 3, mips>>16);
	set_next_dst(ppc);
	// Load the current PC as the second arg
	GEN_LIS(ppc, 4, get_src_pc()>>16);
	set_next_dst(ppc);
	// Load the lower halves of mips and PC
	GEN_ORI(ppc, 3, 3, mips);
	set_next_dst(ppc);
	GEN_ORI(ppc, 4, 4, get_src_pc());
	set_next_dst(ppc);
	// Branch to decodeNInterpret
	//GEN_BCTRL(ppc);
	GEN_B(ppc, add_jump(&decodeNInterpret, 1, 1), 0, 1);
	set_next_dst(ppc);
	// Load the old LR
	GEN_LWZ(ppc, 0, DYNAOFF_LR, 1);
	set_next_dst(ppc);
	// Check if the PC changed
	GEN_CMPI(ppc, 3, 0, 6);
	set_next_dst(ppc);
	// Restore the LR
	GEN_MTLR(ppc, 0);
	set_next_dst(ppc);
	// if decodeNInterpret returned an address
	//   jumpTo it
	GEN_BNELR(ppc, 6, 0);
	set_next_dst(ppc);

	if(mips_is_jump(mips)) delaySlotNext = 2;
}

static void genJumpTo(unsigned int loc, unsigned int type){
	PowerPC_instr ppc = NEW_PPC_INSTR();

	if(type == JUMPTO_REG){
		// Load the register as the return value
		GEN_LWZ(ppc, 3, loc*8+4, DYNAREG_REG);
		set_next_dst(ppc);
	} else {
		// Calculate the destination address
		loc <<= 2;
		if(type == JUMPTO_OFF) loc += get_src_pc();
		else loc |= get_src_pc() & 0xf0000000;
		// Create space to load destination func*
		GEN_ORI(ppc, 0, 0, 0);
		set_next_dst(ppc);
		set_next_dst(ppc);
		// Move func* into r3 as argument
		GEN_ADDI(ppc, 3, DYNAREG_FUNC, 0);
		set_next_dst(ppc);
		// Call RecompCache_Update(func)
		GEN_B(ppc, add_jump(&RecompCache_Update, 1, 1), 0, 1);
		set_next_dst(ppc);
		// Restore LR
		GEN_LWZ(ppc, 0, DYNAOFF_LR, 1);
		set_next_dst(ppc);
		GEN_MTLR(ppc, 0);
		set_next_dst(ppc);
		// Load the address as the return value
		GEN_LIS(ppc, 3, loc >> 16);
		set_next_dst(ppc);
		GEN_ORI(ppc, 3, 3, loc);
		set_next_dst(ppc);
		// Since we could be linking, return on interrupt
		GEN_BLELR(ppc, 2, 0);
		set_next_dst(ppc);
		// Store last_addr for linking
		GEN_STW(ppc, 3, 0, DYNAREG_LADDR);
		set_next_dst(ppc);
	}

	GEN_BLR(ppc, (type != JUMPTO_REG));
	set_next_dst(ppc);
}

// Updates Count, and sets cr2 to (next_interupt ? Count)
static void genUpdateCount(int checkCount){
	PowerPC_instr ppc = NEW_PPC_INSTR();
#ifndef COMPARE_CORE
	// Dynarec inlined code equivalent:
	int tmp = mapRegisterTemp();
	// lis    tmp, pc >> 16
	GEN_LIS(ppc, tmp, (get_src_pc()+4)>>16);
	set_next_dst(ppc);
	// lwz    r0,  0(&last_addr)     // r0 = last_addr
	GEN_LWZ(ppc, 0, 0, DYNAREG_LADDR);
	set_next_dst(ppc);
	// ori    tmp, tmp, pc & 0xffff  // tmp = pc
	GEN_ORI(ppc, tmp, tmp, get_src_pc()+4);
	set_next_dst(ppc);
	// stw    tmp, 0(&last_addr)     // last_addr = pc
	GEN_STW(ppc, tmp, 0, DYNAREG_LADDR);
	set_next_dst(ppc);
	// subf   r0,  r0, tmp           // r0 = pc - last_addr
	GEN_SUBF(ppc, 0, 0, tmp);
	set_next_dst(ppc);
	// lwz    tmp, 9*4(reg_cop0)     // tmp = Count
	GEN_LWZ(ppc, tmp, 9*4, DYNAREG_COP0);
	set_next_dst(ppc);
	// srwi r0, r0, 1                // r0 = (pc - last_addr)/2
	GEN_SRWI(ppc, 0, 0, 1);
	set_next_dst(ppc);
	// add    r0,  r0, tmp           // r0 += Count
	GEN_ADD(ppc, 0, 0, tmp);
	set_next_dst(ppc);
	if(checkCount){
		// lwz    tmp, 0(&next_interupt) // tmp = next_interupt
		GEN_LWZ(ppc, tmp, 0, DYNAREG_NINTR);
		set_next_dst(ppc);
	}
	// stw    r0,  9*4(reg_cop0)    // Count = r0
	GEN_STW(ppc, 0, 9*4, DYNAREG_COP0);
	set_next_dst(ppc);
	if(checkCount){
		// cmpl   cr2,  tmp, r0  // cr2 = next_interupt ? Count
		GEN_CMPL(ppc, tmp, 0, 2);
		set_next_dst(ppc);
	}
	// Free tmp register
	unmapRegisterTemp(tmp);
#else
	// Load the current PC as the argument
	GEN_LIS(ppc, 3, (get_src_pc()+4)>>16);
	set_next_dst(ppc);
	GEN_ORI(ppc, 3, 3, get_src_pc()+4);
	set_next_dst(ppc);
	// Call dyna_update_count
	GEN_B(ppc, add_jump(&dyna_update_count, 1, 1), 0, 1);
	set_next_dst(ppc);
	// Load the lr
	GEN_LWZ(ppc, 0, DYNAOFF_LR, 1);
	set_next_dst(ppc);
	GEN_MTLR(ppc, 0);
	set_next_dst(ppc);
	if(checkCount){
		// If next_interupt <= Count (cr2)
		GEN_CMPI(ppc, 3, 0, 2);
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
		GEN_LWZ(ppc, 0, 12*4, DYNAREG_COP0);
		set_next_dst(ppc);
		// andis. r0, r0, 0x2000
		GEN_ANDIS(ppc, 0, 0, 0x2000);
		set_next_dst(ppc);
		// bne cr0, end
		GEN_BNE(ppc, 0, 8, 0, 0);
		set_next_dst(ppc);
		// Move &dyna_check_cop1_unusable to ctr for call
		//GEN_MTCTR(ppc, DYNAREG_CHKFP);
		//set_next_dst(ppc);
		// Load the current PC as arg 1 (upper half)
		GEN_LIS(ppc, 3, get_src_pc()>>16);
		set_next_dst(ppc);
		// Pass in whether this instruction is in the delay slot as arg 2
		GEN_LI(ppc, 4, 0, isDelaySlot ? 1 : 0);
		set_next_dst(ppc);
		// Current PC (lower half)
		GEN_ORI(ppc, 3, 3, get_src_pc());
		set_next_dst(ppc);
		// Call dyna_check_cop1_unusable
		//GEN_BCTRL(ppc);
		GEN_B(ppc, add_jump(&dyna_check_cop1_unusable, 1, 1), 0, 1);
		set_next_dst(ppc);
		// Load the old LR
		GEN_LWZ(ppc, 0, DYNAOFF_LR, 1);
		set_next_dst(ppc);
		// Restore the LR
		GEN_MTLR(ppc, 0);
		set_next_dst(ppc);
		// Return to trampoline
		GEN_BLR(ppc, 0);
		set_next_dst(ppc);
		// Don't check for the rest of this mapping
		// Unless this instruction is in a delay slot
		FP_need_check = isDelaySlot;
	}
}

void genCallDynaMem(memType type, int base, short immed){
	PowerPC_instr ppc;
	// PRE: value to store, or register # to load into should be in r3
	// Pass PC as arg 4 (upper half)
	GEN_LIS(ppc, 6, (get_src_pc()+4)>>16);
	set_next_dst(ppc);
	// addr = base + immed (arg 2)
	GEN_ADDI(ppc, 4, base, immed);
	set_next_dst(ppc);
	// type passed as arg 3
	GEN_LI(ppc, 5, 0, type);
	set_next_dst(ppc);
	// Lower half of PC
	GEN_ORI(ppc, 6, 6, get_src_pc()+4);
	set_next_dst(ppc);
	// isDelaySlot as arg 5
	GEN_LI(ppc, 7, 0, isDelaySlot ? 1 : 0);
	set_next_dst(ppc);
	// call dyna_mem
	GEN_B(ppc, add_jump(&dyna_mem, 1, 1), 0, 1);
	set_next_dst(ppc);
	// Load old LR
	GEN_LWZ(ppc, 0, DYNAOFF_LR, 1);
	set_next_dst(ppc);
	// Check whether we need to take an interrupt
	GEN_CMPI(ppc, 3, 0, 6);
	set_next_dst(ppc);
	// Restore LR
	GEN_MTLR(ppc, 0);
	set_next_dst(ppc);
	// If so, return to trampoline
	GEN_BNELR(ppc, 6, 0);
	set_next_dst(ppc);
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

