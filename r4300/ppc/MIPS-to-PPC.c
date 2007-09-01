/* MIPS-to-PPC.c - convert MIPS code into PPC
   by Mike Slegeir for Mupen64-GC
 ************************************************
   TODO: Finish all the important instructions
         Confirm hi/lo work properly for each case
           it probably breaks if a mult is followed by a madd
         If the lr is used in anything besides
           lw/sw/add(move) that should be supported
         Verify likely branches work properly
         The Gekko uses a modified FPU,
           make sure the single precision is compatible
         If jr is used for anything besides blr,
           we need some way to convert the register
         Maybe accesses to the stack can be recompiled
         This file is getting big, it should probably be split
 */

#include "MIPS-to-PPC.h"
#include "Interpreter.h"
#include "Wrappers.h"

// If this is defined true, delay slots will get moved
#define SUPPORT_DELAY_SLOT 1

// This is my do-anything variable
static int temp;
// Support for seperated mult/div and mfhi/lo
// Number of instructions to execute on mfhi/lo
static int hi_instr_count, lo_instr_count;
// Instructions to execute
static PowerPC_instr hi_instr[4], lo_instr[4];
// If rd is used in instruction, shift it this amount (if > 0)
static char hi_shift[4][2], lo_shift[4][2];

static int convert_R  (MIPS_instr);
static int convert_B  (MIPS_instr);
static int convert_M  (MIPS_instr);
static int convert_CoP(MIPS_instr, int z);
static int convert_FP (MIPS_instr, int precision);
static void genCallInterp(MIPS_instr);
static int inline mips_is_jump(MIPS_instr);

// This should be called before the jump is recompiled
static inline int check_delaySlot(void){
#if SUPPORT_DELAY_SLOT
	if(peek_next_src() == 0) // MIPS uses 0 as a NOP
		get_next_src();  // Get rid of the NOP
	else {
		if(mips_is_jump(peek_next_src())) return CONVERT_WARNING;
		convert(); // This just moves the delay slot instruction ahead of the branch
	}
	return CONVERT_SUCCESS;
#else // SUPPORT_DELAY_SLOT
	return CONVERT_SUCCESS;
#endif
}

static inline int signExtend(int value, int size){
	int signMask = 1 << (size-1);
	int negMask = 0xffffffff << (size-1);
	if(value & signMask) value |= negMask;
	return value;
}

#define CONVERT_I_TYPE(ppc,mips) \
	do { PPC_SET_RD   (ppc, MIPS_GET_RT(mips));    \
	     PPC_SET_RA   (ppc, MIPS_GET_RS(mips));    \
	     PPC_SET_IMMED(ppc, MIPS_GET_IMMED(mips)); } while(0)

#define CONVERT_I_TYPE2(ppc,mips) \
	do { PPC_SET_RD   (ppc, MIPS_GET_RS(mips));    \
	     PPC_SET_RA   (ppc, MIPS_GET_RT(mips));    \
	     PPC_SET_IMMED(ppc, MIPS_GET_IMMED(mips)); } while(0)

int convert(void){
	MIPS_instr    mips = get_next_src();
	PowerPC_instr ppc  = NEW_PPC_INSTR();
	int bo;
	
	switch(MIPS_GET_OPCODE(mips)){
	
	case MIPS_OPCODE_R:
		return convert_R(mips);
	case MIPS_OPCODE_B:
		return convert_B(mips);
	case MIPS_OPCODE_M:
		return convert_M(mips);
	case MIPS_OPCODE_JAL:
	case MIPS_OPCODE_J:
		check_delaySlot();
		// temp is used for is_out
		temp = 0;
		if(is_j_out(MIPS_GET_LI(mips), 1)){
			temp = 1;
			// Allocate space for jumping out, 5 NOPs
			PPC_SET_OPCODE(ppc, PPC_OPCODE_ORI);
			set_next_dst(ppc);
			set_next_dst(ppc);
			set_next_dst(ppc);
			set_next_dst(ppc);
			set_next_dst(ppc);
			ppc = NEW_PPC_INSTR();
		}
		PPC_SET_OPCODE(ppc, PPC_OPCODE_B);
		if(MIPS_GET_OPCODE(mips == MIPS_OPCODE_JAL))
			PPC_SET_LK(ppc, 1);
		PPC_SET_LI    (ppc, add_jump(MIPS_GET_LI(mips), 1, temp));
		set_next_dst(ppc);
		// Add space to zero r0 if its not taken
		if(temp){
			PPC_SET_OPCODE(ppc, PPC_OPCODE_ORI);
			set_next_dst(ppc);
		}
		return CONVERT_SUCCESS;
	case MIPS_OPCODE_BEQL:
	case MIPS_OPCODE_BNEL:
	case MIPS_OPCODE_BEQ:
	case MIPS_OPCODE_BNE:
		bo = (MIPS_GET_OPCODE(mips) == MIPS_OPCODE_BEQ  ||
		      MIPS_GET_OPCODE(mips) == MIPS_OPCODE_BEQL) ?
		           0xc : 0x4;
		// cmp
		PPC_SET_OPCODE(ppc, PPC_OPCODE_X);
		PPC_SET_FUNC  (ppc, PPC_FUNC_CMP);
		PPC_SET_RA    (ppc, MIPS_GET_RA(mips));
		PPC_SET_RB    (ppc, MIPS_GET_RB(mips));
		PPC_SET_CRF   (ppc, 7); // Use CR7
		set_next_dst(ppc);
		// Likely branches skip the delay slot if they're not taken
		if(MIPS_GET_OPCODE(mips) == MIPS_OPCODE_BEQL ||
		   MIPS_GET_OPCODE(mips) == MIPS_OPCODE_BNEL ){
			// b[!cond] <past delay & branch>
			ppc = NEW_PPC_INSTR();
			PPC_SET_OPCODE(ppc, PPC_OPCODE_BC);
			PPC_SET_BD(ppc, (peek_next_src() == 0) ? 2 : 3); // past delay & branch
			PPC_SET_BO(ppc, bo^0x8); // !cond
			PPC_SET_BI(ppc, 30);     // Check CR bit 30 (CR7, EQ FIELD)
			set_next_dst(ppc);
		}
		// delay slot
		check_delaySlot();
		// temp is used for is_out
		temp = 0;
		if(is_j_out(signExtend(MIPS_GET_IMMED(mips),16), 0)){
			temp = 1;
			// Allocate space for jumping out, 5 NOPs
			PPC_SET_OPCODE(ppc, PPC_OPCODE_ORI);
			set_next_dst(ppc);
			set_next_dst(ppc);
			set_next_dst(ppc);
			set_next_dst(ppc);
			set_next_dst(ppc);
			ppc = NEW_PPC_INSTR();
		}
		// bc
		ppc = NEW_PPC_INSTR();
		PPC_SET_OPCODE(ppc, PPC_OPCODE_BC);
		PPC_SET_BD(ppc, add_jump(signExtend(MIPS_GET_IMMED(mips),16), 0, temp));
		PPC_SET_BO(ppc, bo);  // Test if CR is 1 or 0
		PPC_SET_BI(ppc, 30);  // Check CR bit 30 (CR7, EQ FIELD)
		set_next_dst(ppc);
		// Add space to zero r0 if its not taken
		if(temp){
			PPC_SET_OPCODE(ppc, PPC_OPCODE_ORI);
			set_next_dst(ppc);
		}
		return CONVERT_SUCCESS;
	case MIPS_OPCODE_BLEZL:
	case MIPS_OPCODE_BGTZL:
	case MIPS_OPCODE_BLEZ:
	case MIPS_OPCODE_BGTZ:
		bo = (MIPS_GET_OPCODE(mips) == MIPS_OPCODE_BLEZ  ||
		      MIPS_GET_OPCODE(mips) == MIPS_OPCODE_BLEZL) ?
		           0x4 : 0xc;
		// cmpi to 0
		PPC_SET_OPCODE(ppc, PPC_OPCODE_CMPI);
		PPC_SET_RA    (ppc, MIPS_GET_RA(mips));
		PPC_SET_IMMED (ppc, 0);
		PPC_SET_CRF   (ppc, 7); // Use CR7
		set_next_dst(ppc);
		// Likely branches skip the delay slot if they're not taken
		if(MIPS_GET_OPCODE(mips) == MIPS_OPCODE_BLEZL ||
		   MIPS_GET_OPCODE(mips) == MIPS_OPCODE_BGTZL ){
			// b[!cond] <past delay & branch>
			ppc = NEW_PPC_INSTR();
			PPC_SET_OPCODE(ppc, PPC_OPCODE_BC);
			PPC_SET_BD(ppc, (peek_next_src() == 0) ? 2 : 3); // past delay & branch
			PPC_SET_BO(ppc, bo^0x8); // !cond
			PPC_SET_BI(ppc, 29);     // Check CR bit 29 (CR7, GT FIELD)
			set_next_dst(ppc);
		}
		// delay slot
		check_delaySlot();
		// temp is used for is_out
		temp = 0;
		if(is_j_out(signExtend(MIPS_GET_IMMED(mips),16), 0)){
			temp = 1;
			// Allocate space for jumping out, 5 NOPs
			PPC_SET_OPCODE(ppc, PPC_OPCODE_ORI);
			set_next_dst(ppc);
			set_next_dst(ppc);
			set_next_dst(ppc);
			set_next_dst(ppc);
			set_next_dst(ppc);
			ppc = NEW_PPC_INSTR();
		}
		// bc
		ppc = NEW_PPC_INSTR();
		PPC_SET_OPCODE(ppc, PPC_OPCODE_BC);
		PPC_SET_BD(ppc, add_jump(signExtend(MIPS_GET_IMMED(mips),16), 0, temp));
		PPC_SET_BO(ppc, bo);  // Test if CR is 1 or 0
		PPC_SET_BI(ppc, 29);  // Check CR bit 29 (CR7, GT FIELD)
		set_next_dst(ppc);
		// Add space to zero r0 if its not taken
		if(temp){
			PPC_SET_OPCODE(ppc, PPC_OPCODE_ORI);
			set_next_dst(ppc);
		}
		return CONVERT_SUCCESS;
	case MIPS_OPCODE_ADDIU:
	case MIPS_OPCODE_ADDI:
		// FIXME: It seems addi never generates OE
		PPC_SET_OPCODE(ppc, PPC_OPCODE_ADDI);
		CONVERT_I_TYPE(ppc, mips);
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
	case MIPS_OPCODE_SLTI:
	case MIPS_OPCODE_SLTIU:
		// cmpi
		PPC_SET_OPCODE(ppc, (MIPS_GET_OPCODE(mips) == MIPS_OPCODE_SLTI)
		                     ? PPC_OPCODE_CMPI : PPC_OPCODE_CMPLI);
		PPC_SET_RA    (ppc, MIPS_GET_RS(mips));
		PPC_SET_CRF   (ppc, 7); // Use CR7
		PPC_SET_IMMED (ppc, MIPS_GET_IMMED(mips));
		set_next_dst(ppc);
		// mfcr
		ppc = NEW_PPC_INSTR();
		PPC_SET_OPCODE(ppc, PPC_OPCODE_X);
		PPC_SET_FUNC  (ppc, PPC_FUNC_MFCR);
		PPC_SET_RD    (ppc, MIPS_GET_RD(mips));
		set_next_dst(ppc);
		// rlwinm
		ppc = NEW_PPC_INSTR();
		PPC_SET_OPCODE(ppc, PPC_OPCODE_RLWINM);
		PPC_SET_RD    (ppc, MIPS_GET_RD(mips));
		PPC_SET_RA    (ppc, MIPS_GET_RD(mips));
		PPC_SET_SH    (ppc, 29); // Rotate LT bit to position 0
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
	case MIPS_OPCODE_ANDI:
		PPC_SET_OPCODE(ppc, PPC_OPCODE_ANDI);
		CONVERT_I_TYPE2(ppc, mips);
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
	case MIPS_OPCODE_ORI:
		PPC_SET_OPCODE(ppc, PPC_OPCODE_ORI);
		CONVERT_I_TYPE2(ppc, mips);
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
	case MIPS_OPCODE_XORI:
		PPC_SET_OPCODE(ppc, PPC_OPCODE_XORI);
		CONVERT_I_TYPE2(ppc, mips);
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
	case MIPS_OPCODE_LUI:
		PPC_SET_OPCODE(ppc, PPC_OPCODE_ADDIS);
		PPC_SET_RD    (ppc, MIPS_GET_RT(mips));
		PPC_SET_IMMED (ppc, MIPS_GET_IMMED(mips));
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
	case MIPS_OPCODE_DADDI:
	case MIPS_OPCODE_DADDIU:
#ifdef INTERPRET_DW
		genCallInterp(mips);
		return INTERPRETED;
#else
		return CONVERT_ERROR;
#endif
	case MIPS_OPCODE_COP0:
		return convert_CoP(mips, 0);
	case MIPS_OPCODE_COP1:
		return convert_CoP(mips, 1);
	case MIPS_OPCODE_COP2:
		return convert_CoP(mips, 2);
	case MIPS_OPCODE_LB:
#ifdef INTERPRET_LB
		genCallInterp(mips);
		return INTERPRETED;
#else
		PPC_SET_OPCODE(ppc, PPC_OPCODE_LBZ);
		CONVERT_I_TYPE(ppc, mips);
		set_next_dst(ppc);
		ppc = NEW_PPC_INSTR();
		PPC_SET_OPCODE(ppc, PPC_OPCODE_X);
		PPC_SET_FUNC  (ppc, PPC_FUNC_EXTSB);
		PPC_SET_RD    (ppc, MIPS_GET_RT(mips));
		PPC_SET_RA    (ppc, MIPS_GET_RT(mips));
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
#endif
	case MIPS_OPCODE_LH:
#ifdef INTERPRET_LH
		genCallInterp(mips);
		return INTERPRETED;
#else
		PPC_SET_OPCODE(ppc, PPC_OPCODE_LHA);
		CONVERT_I_TYPE(ppc, mips);
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
#endif
	case MIPS_OPCODE_LWL:
#ifdef INTERPRET_LWL
		genCallInterp(mips);
		return INTERPRETED;
#else
		return CONVERT_ERROR;
#endif
	case MIPS_OPCODE_LW:
#ifdef INTERPRET_LW
		// We don't have to worry about moving to the lr
		// because this is handled in the call to the interpreter
		genCallInterp(mips);
		return INTERPRETED;
#else
		PPC_SET_OPCODE(ppc, PPC_OPCODE_LWZ);
		CONVERT_I_TYPE(ppc, mips);
		set_next_dst(ppc);
		// If we're loading into the LR, do that in PPC
		if(MIPS_GET_RD(mips)==MIPS_REG_LR){
			// mtlr mips_lr
			ppc = NEW_PPC_INSTR();
			PPC_SET_OPCODE(ppc, PPC_OPCODE_X);
			PPC_SET_FUNC  (ppc, PPC_FUNC_MTSPR);
			PPC_SET_RD    (ppc, MIPS_REG_LR);
			PPC_SET_SPR   (ppc, 0x100);
			set_next_dst(ppc);
		}
		return CONVERT_SUCCESS;
#endif
	case MIPS_OPCODE_LBU:
#ifdef INTERPRET_LBU
		genCallInterp(mips);
		return INTERPRETED;
#else
		PPC_SET_OPCODE(ppc, PPC_OPCODE_LBZ);
		CONVERT_I_TYPE(ppc, mips);
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
#endif
	case MIPS_OPCODE_LHU:
#ifdef INTERPRET_LHU
		genCallInterp(mips);
		return INTERPRETED;
#else
		PPC_SET_OPCODE(ppc, PPC_OPCODE_LHZ);
		CONVERT_I_TYPE(ppc, mips);
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
#endif
	case MIPS_OPCODE_LWR:
#ifdef INTERPRET_LWR
		genCallInterp(mips);
		return INTERPRETED;
#else
		return CONVERT_ERROR;
#endif
	case MIPS_OPCODE_LD:
	case MIPS_OPCODE_LDL:
	case MIPS_OPCODE_LDR:
	case MIPS_OPCODE_LLD:
#ifdef INTERPRET_DW
		genCallInterp(mips);
		return INTERPRETED;
#else
		return CONVERT_ERROR;
#endif
	case MIPS_OPCODE_SB:
#ifdef INTERPRET_SB
		genCallInterp(mips);
		return INTERPRETED;
#else
		PPC_SET_OPCODE(ppc, PPC_OPCODE_STB);
		CONVERT_I_TYPE(ppc, mips);
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
#endif
	case MIPS_OPCODE_SH:
#ifdef INTERPRET_SH
		genCallInterp(mips);
		return INTERPRETED;
#else
		PPC_SET_OPCODE(ppc, PPC_OPCODE_STH);
		CONVERT_I_TYPE(ppc, mips);
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
#endif
	case MIPS_OPCODE_SWL:
#ifdef INTERPRET_SWL
		genCallInterp(mips);
		return INTERPRETED;
#else
		return CONVERT_ERROR;
#endif
	case MIPS_OPCODE_SW:
#ifdef INTERPRET_SW
		// We don't have to worry about moving from the lr
		// because this is handled in the call to the interpreter
		genCallInterp(mips);
		return INTERPRETED;
#else
		// Check to see if we're trying to save the lr
		if(MIPS_GET_RD(mips)==MIPS_REG_LR){
			// mflr mips_lr
			PPC_SET_OPCODE(ppc, PPC_OPCODE_X);
			PPC_SET_FUNC  (ppc, PPC_FUNC_MFSPR);
			PPC_SET_RD    (ppc, MIPS_REG_LR);
			PPC_SET_SPR   (ppc, 0x100);
			set_next_dst(ppc);
			ppc = NEW_PPC_INSTR();
		}
		PPC_SET_OPCODE(ppc, PPC_OPCODE_STW);
		CONVERT_I_TYPE(ppc, mips);
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
#endif
	case MIPS_OPCODE_SWR:
#ifdef INTERPRET_SWR
		genCallInterp(mips);
		return INTERPRETED;
#else
		return CONVERT_ERROR;
#endif
	case MIPS_OPCODE_SD:
	case MIPS_OPCODE_SDL:
	case MIPS_OPCODE_SDR:
	case MIPS_OPCODE_SCD:
#ifdef INTERPRET_DW
		genCallInterp(mips);
		return INTERPRETED;
#else
		return CONVERT_ERROR;
#endif
	case MIPS_OPCODE_CACHE:
		return CONVERT_ERROR;
	case MIPS_OPCODE_LL:
#ifdef INTERPRET_LL
		genCallInterp(mips);
		return INTERPRETED;
#else
		return CONVERT_ERROR;
#endif
	case MIPS_OPCODE_LWC1:
		PPC_SET_OPCODE(ppc, PPC_OPCODE_LFS);
		CONVERT_I_TYPE(ppc, mips);
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
	case MIPS_OPCODE_LWC2:
		return CONVERT_ERROR;
	case MIPS_OPCODE_PREF:
		return CONVERT_ERROR;
	case MIPS_OPCODE_LDC1:
		PPC_SET_OPCODE(ppc, PPC_OPCODE_LFD);
		CONVERT_I_TYPE(ppc, mips);
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
	case MIPS_OPCODE_LDC2:
		return CONVERT_ERROR;
	case MIPS_OPCODE_SC:
#ifdef INTERPRET_SC
		genCallInterp(mips);
		return INTERPRETED;
#else
		return CONVERT_ERROR;
#endif
	case MIPS_OPCODE_SWC1:
		PPC_SET_OPCODE(ppc, PPC_OPCODE_STFS);
		CONVERT_I_TYPE(ppc, mips);
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
	case MIPS_OPCODE_SWC2:
		return CONVERT_ERROR;
	case MIPS_OPCODE_SDC1:
		PPC_SET_OPCODE(ppc, PPC_OPCODE_STFD);
		CONVERT_I_TYPE(ppc, mips);
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
	case MIPS_OPCODE_SDC2:
		return CONVERT_ERROR;
	
	default:
		return CONVERT_ERROR;
	}
}

#define CONVERT_REGS(ppc,mips)  \
	do { PPC_SET_RD(ppc, MIPS_GET_RD(mips)); \
	     PPC_SET_RA(ppc, MIPS_GET_RS(mips)); \
	     PPC_SET_RB(ppc, MIPS_GET_RT(mips)); } while(0)

static int convert_R(MIPS_instr mips){
	PowerPC_instr ppc = NEW_PPC_INSTR();
	int i;
	
	switch(MIPS_GET_FUNC(mips)){
	
	case MIPS_FUNC_SLL:
		PPC_SET_OPCODE(ppc,     PPC_OPCODE_RLWINM);
		PPC_SET_RA    (ppc,     MIPS_GET_RD(mips));
		PPC_SET_RD    (ppc,     MIPS_GET_RT(mips));
		PPC_SET_SH    (ppc,     MIPS_GET_SA(mips));
		PPC_SET_ME    (ppc, (31-MIPS_GET_SA(mips)));
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
	case MIPS_FUNC_MOV:
		// movt/movf
		// beq <past addi>
		PPC_SET_OPCODE(ppc, PPC_OPCODE_BC);
		PPC_SET_BD(ppc, 2);
		PPC_SET_BO(ppc, (mips&0x10000) ? 0xc : 0x4);  // Test if cc is t/f
		PPC_SET_BI(ppc, (20 + MIPS_GET_CC(mips)));      // Check cc (CR5/6)
		set_next_dst(ppc);
		// addi rd, rs, 0
		ppc = NEW_PPC_INSTR();
		PPC_SET_OPCODE(ppc, PPC_OPCODE_ADDI);
		PPC_SET_RD    (ppc, MIPS_GET_RD(mips));
		PPC_SET_RA    (ppc, MIPS_GET_RS(mips));
		PPC_SET_IMMED (ppc, 0);
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
	case MIPS_FUNC_SRL:
		PPC_SET_OPCODE(ppc,     PPC_OPCODE_RLWINM);
		PPC_SET_RA    (ppc,     MIPS_GET_RD(mips));
		PPC_SET_RD    (ppc,     MIPS_GET_RT(mips));
		PPC_SET_SH    (ppc, (32-MIPS_GET_SA(mips)));
		PPC_SET_MB    (ppc,     MIPS_GET_SA(mips));
		PPC_SET_ME    (ppc,  31);
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
	case MIPS_FUNC_SRA:
		PPC_SET_OPCODE(ppc, PPC_OPCODE_X);
		PPC_SET_FUNC  (ppc, PPC_FUNC_SRAWI);
		PPC_SET_RD    (ppc, MIPS_GET_RT(mips));
		PPC_SET_RA    (ppc, MIPS_GET_RD(mips));
		PPC_SET_SH    (ppc, MIPS_GET_SA(mips));
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
	case MIPS_FUNC_SLLV:
		PPC_SET_OPCODE(ppc, PPC_OPCODE_X);
		PPC_SET_FUNC  (ppc, PPC_FUNC_SLW);
		PPC_SET_RD    (ppc, MIPS_GET_RT(mips));
		PPC_SET_RA    (ppc, MIPS_GET_RD(mips));
		PPC_SET_RB    (ppc, MIPS_GET_RS(mips));
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
	case MIPS_FUNC_SRLV:
		PPC_SET_OPCODE(ppc, PPC_OPCODE_X);
		PPC_SET_FUNC  (ppc, PPC_FUNC_SRW);
		PPC_SET_RD    (ppc, MIPS_GET_RT(mips));
		PPC_SET_RA    (ppc, MIPS_GET_RD(mips));
		PPC_SET_RB    (ppc, MIPS_GET_RS(mips));
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
	case MIPS_FUNC_SRAV:
		PPC_SET_OPCODE(ppc, PPC_OPCODE_X);
		PPC_SET_FUNC  (ppc, PPC_FUNC_SRAW);
		PPC_SET_RD    (ppc, MIPS_GET_RT(mips));
		PPC_SET_RA    (ppc, MIPS_GET_RD(mips));
		PPC_SET_RB    (ppc, MIPS_GET_RS(mips));
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
	// FIXME: These will have issues if there are any hash functions
	case MIPS_FUNC_JR:
		check_delaySlot();
		// Check to see if this is a jlr
		if(MIPS_GET_RS(mips)==MIPS_REG_LR){
			PPC_SET_OPCODE(ppc, PPC_OPCODE_XL);
			PPC_SET_FUNC  (ppc, PPC_FUNC_BCLR);
			PPC_SET_BO    (ppc, 0x14);
			set_next_dst(ppc);
			return CONVERT_SUCCESS;
		} else {
			// mtctr
			PPC_SET_OPCODE(ppc, PPC_OPCODE_X);
			PPC_SET_FUNC  (ppc, PPC_FUNC_MTSPR);
			PPC_SET_RD    (ppc, MIPS_GET_RS(mips));
			PPC_SET_SPR   (ppc, 0x120);
			set_next_dst(ppc);
			// bcctr
			ppc = NEW_PPC_INSTR();
			PPC_SET_OPCODE(ppc, PPC_OPCODE_XL);
			PPC_SET_FUNC  (ppc, PPC_FUNC_BCCTR);
			PPC_SET_BO    (ppc, 0x14);
			set_next_dst(ppc);
			return CONVERT_WARNING; // This is a warning because the address wasn't converted
		}
	case MIPS_FUNC_JALR:
		check_delaySlot();
		// Check to see if this is a jlr
		if(MIPS_GET_RS(mips)!=MIPS_REG_LR){ // If its not, move the appropriate reg
			// mtlr
			PPC_SET_OPCODE(ppc, PPC_OPCODE_X);
			PPC_SET_FUNC  (ppc, PPC_FUNC_MTSPR);
			PPC_SET_RD    (ppc, MIPS_GET_RS(mips));
			PPC_SET_SPR   (ppc, 0x100);
			set_next_dst(ppc);
			ppc = NEW_PPC_INSTR();
		}
		// bclr
		ppc = NEW_PPC_INSTR();
		PPC_SET_OPCODE(ppc, PPC_OPCODE_XL);
		PPC_SET_FUNC  (ppc, PPC_FUNC_BCLR);
		PPC_SET_BO    (ppc, 0x14);
		PPC_SET_LK    (ppc, 1);
		set_next_dst(ppc);
		return (MIPS_GET_RS(mips)==MIPS_REG_LR)
		       ? CONVERT_SUCCESS : CONVERT_WARNING; // Address wasn't converted
	case MIPS_FUNC_MOVN:
#ifdef INTERPRET_MOVN
		genCallInterp(mips);
		return INTERPRETED;
#else
		// cmpi rt, 0
		PPC_SET_OPCODE(ppc, PPC_OPCODE_CMPI);
		PPC_SET_RA    (ppc, MIPS_GET_RT(mips));
		PPC_SET_IMMED (ppc, 0);
		PPC_SET_CRF   (ppc, 7); // Use CR7
		set_next_dst(ppc);
		// beq <past addi>
		ppc = NEW_PPC_INSTR();
		PPC_SET_OPCODE(ppc, PPC_OPCODE_BC);
		PPC_SET_BD(ppc, 2);
		PPC_SET_BO(ppc, 0x4);  // Test if CR is 0
		PPC_SET_BI(ppc, 30);  // Check CR bit 30 (CR7, EQ FIELD)
		set_next_dst(ppc);
		// addi rd, rs, 0
		ppc = NEW_PPC_INSTR();
		PPC_SET_OPCODE(ppc, PPC_OPCODE_ADDI);
		PPC_SET_RD    (ppc, MIPS_GET_RD(mips));
		PPC_SET_RA    (ppc, MIPS_GET_RS(mips));
		PPC_SET_IMMED (ppc, 0);
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
#endif
	case MIPS_FUNC_MOVZ:
#ifdef INTERPRET_MOVZ
		genCallInterp(mips);
		return INTERPRETED;
#else
		// cmpi rt, 0
		PPC_SET_OPCODE(ppc, PPC_OPCODE_CMPI);
		PPC_SET_RA    (ppc, MIPS_GET_RT(mips));
		PPC_SET_IMMED (ppc, 0);
		PPC_SET_CRF   (ppc, 7); // Use CR7
		set_next_dst(ppc);
		// bne <past addi>
		ppc = NEW_PPC_INSTR();
		PPC_SET_OPCODE(ppc, PPC_OPCODE_BC);
		PPC_SET_BD(ppc, 2);
		PPC_SET_BO(ppc, 0xc);  // Test if CR is 1
		PPC_SET_BI(ppc, 30);  // Check CR bit 30 (CR7, EQ FIELD)
		set_next_dst(ppc);
		// addi rd, rs, 0
		ppc = NEW_PPC_INSTR();
		PPC_SET_OPCODE(ppc, PPC_OPCODE_ADDI);
		PPC_SET_RD    (ppc, MIPS_GET_RD(mips));
		PPC_SET_RA    (ppc, MIPS_GET_RS(mips));
		PPC_SET_IMMED (ppc, 0);
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
#endif
	case MIPS_FUNC_SYSCALL:
#ifdef INTERPRET_SYSCALL
		genCallInterp(mips);
		return INTERPRETED;
#else
		return CONVERT_ERROR;
#endif
	case MIPS_FUNC_BREAK:
#ifdef INTERPRET_BREAK
		genCallInterp(mips);
		return INTERPRETED;
#else
		return CONVERT_ERROR;
#endif
	case MIPS_FUNC_SYNC:
		return CONVERT_ERROR;
	case MIPS_FUNC_MFHI:
		if(hi_instr_count){
			for(temp=0; temp<hi_instr_count; ++temp){
				ppc = hi_instr[temp];
				if(hi_shift[temp][0] >= 0)
					ppc |= MIPS_GET_RD(mips) << hi_shift[temp][0];
				if(hi_shift[temp][1] >= 0)
					ppc |= MIPS_GET_RD(mips) << hi_shift[temp][1];
				set_next_dst(ppc);
			}
		} else {
#ifdef INTERPRET_HILO
			genCallInterp(mips);
			return INTERPRETED;
#else
			// TODO: Can simply do a load/store
			return CONVERT_ERROR;
#endif
		}
// -------------------------------------------
	case MIPS_FUNC_MFLO:
		if(lo_instr_count){
			for(temp=0; temp<lo_instr_count; ++temp){
				ppc = lo_instr[temp];
				if(lo_shift[temp][0] >= 0)
					ppc |= MIPS_GET_RD(mips) << lo_shift[temp][0];
				if(lo_shift[temp][1] >= 0)
					ppc |= MIPS_GET_RD(mips) << lo_shift[temp][1];
				set_next_dst(ppc);
			}
		} else {
#ifdef INTERPRET_HILO
			genCallInterp(mips);
			return INTERPRETED;
#else
			// TODO: Can simply do a load/store
			return CONVERT_ERROR;
#endif
		}
// -------------------------------------------
	case MIPS_FUNC_MTHI:
		hi_instr_count = 0;
#ifdef INTERPRET_HILO
		genCallInterp(mips);
		return INTERPRETED;
#else
		// TODO: Can simply do a load/store
		return CONVERT_ERROR;
#endif
// -------------------------------------------
	case MIPS_FUNC_MTLO:
		lo_instr_count = 0;
#ifdef INTERPRET_HILO
		genCallInterp(mips);
		return INTERPRETED;
#else
		// TODO: Can simply do a load/store
		return CONVERT_ERROR;
#endif
	case MIPS_FUNC_MULT:
	case MIPS_FUNC_MULTU:
		
		hi_instr_count = lo_instr_count = 1;
		
		hi_instr[0] = NEW_PPC_INSTR();
		PPC_SET_OPCODE(hi_instr[0], PPC_OPCODE_X);
		PPC_SET_FUNC  (hi_instr[0], (MIPS_GET_FUNC(mips) == MIPS_FUNC_MULT)
		                             ? PPC_FUNC_MULHW : PPC_FUNC_MULHWU);
		PPC_SET_RA    (hi_instr[0], MIPS_GET_RA(mips));
		PPC_SET_RB    (hi_instr[0], MIPS_GET_RB(mips));
		hi_shift[0][0] = PPC_RD_SHIFT;
		hi_shift[0][1] = -1;
		
		lo_instr[0] = NEW_PPC_INSTR();
		PPC_SET_OPCODE(lo_instr[0], PPC_OPCODE_X);
		PPC_SET_FUNC  (lo_instr[0], PPC_FUNC_MULLW);
		PPC_SET_RA    (lo_instr[0], MIPS_GET_RA(mips));
		PPC_SET_RB    (lo_instr[0], MIPS_GET_RB(mips));
		lo_shift[0][0] = PPC_RD_SHIFT;
		lo_shift[0][1] = -1;
		/*
		// mullw   | Mult instructions will be paired with mfhi/lo
		// mulhw   | so they can be converted appropriately
		// FIXME: This assumes that the move from hi/lo comes
		//         immediately after the mult, or never at all.
		//         This may be a fair assumption... It's not
		//        It may be a good idea to store hi/lo if they aren't used
		i = 2;
		while(i){
		MIPS_instr next = peek_next_src();
		if(MIPS_GET_OPCODE(next) == MIPS_OPCODE_R){
			int func = MIPS_GET_FUNC(next);
			if(func == MIPS_FUNC_MFHI){
				get_next_src(); // Since we're using next, pop it
				PPC_SET_OPCODE(ppc, PPC_OPCODE_X);
				PPC_SET_FUNC  (ppc, (MIPS_GET_FUNC(mips) == MIPS_FUNC_MULT)
				                      ? PPC_FUNC_MULHW : PPC_FUNC_MULHWU);
				PPC_SET_RD    (ppc, MIPS_GET_RD(next));
				PPC_SET_RA    (ppc, MIPS_GET_RA(mips));
				PPC_SET_RB    (ppc, MIPS_GET_RB(mips));
				PPC_SET_OE    (ppc, (MIPS_GET_FUNC(mips) == MIPS_FUNC_MULT) ? 1 : 0);
				set_next_dst  (ppc);
			} else if(func == MIPS_FUNC_MFLO){
				get_next_src(); // Since we're using next, pop it
				PPC_SET_OPCODE(ppc, PPC_OPCODE_X);
				PPC_SET_FUNC  (ppc, PPC_FUNC_MULLW);
				PPC_SET_RD    (ppc, MIPS_GET_RD(next));
				PPC_SET_RA    (ppc, MIPS_GET_RA(mips));
				PPC_SET_RB    (ppc, MIPS_GET_RB(mips));
				set_next_dst  (ppc);
			} else if(i == 2) return CONVERT_WARNING; // Warning because the mfhi/lo is later
		} else if(i == 2) return CONVERT_WARNING;
		--i;
		}*/
		return CONVERT_SUCCESS;
	case MIPS_FUNC_DIV:
	case MIPS_FUNC_DIVU:
		hi_instr_count = 3;
		//int rs, rt;
		#define ra MIPS_GET_RA(mips)
		#define rb MIPS_GET_RB(mips)
		
		// divw  rD, rA, rB
		hi_instr[0] = NEW_PPC_INSTR();
		PPC_SET_OPCODE(hi_instr[0], PPC_OPCODE_X);
		PPC_SET_FUNC  (hi_instr[0], (MIPS_GET_FUNC(mips) == MIPS_FUNC_DIV)
		                             ? PPC_FUNC_DIVW : PPC_FUNC_DIVWU);
		PPC_SET_RA    (hi_instr[0], ra);
		PPC_SET_RB    (hi_instr[0], rb);
		hi_shift[0][0] = PPC_RD_SHIFT;
		hi_shift[0][1] = -1;
		
		// mullw rD, rD, rB
		hi_instr[1] = NEW_PPC_INSTR();
		PPC_SET_OPCODE(hi_instr[1], PPC_OPCODE_X);
		PPC_SET_FUNC  (hi_instr[1], PPC_FUNC_MULLW);
		PPC_SET_RB    (hi_instr[1], rb);
		hi_shift[1][0] = PPC_RD_SHIFT;
		hi_shift[1][1] = PPC_RA_SHIFT;
		
		// subf rD, rD, rA
		hi_instr[2] = NEW_PPC_INSTR();
		PPC_SET_OPCODE(hi_instr[2], PPC_OPCODE_X);
		PPC_SET_FUNC  (hi_instr[2], PPC_FUNC_SUBF);
		PPC_SET_RB    (hi_instr[2], ra);
		hi_shift[2][0] = PPC_RD_SHIFT;
		hi_shift[2][1] = PPC_RA_SHIFT;
		
		#undef ra
		#undef rb
		
		lo_instr_count = 1;
		lo_instr[0] = NEW_PPC_INSTR();
		PPC_SET_OPCODE(lo_instr[0], PPC_OPCODE_X);
		PPC_SET_FUNC  (lo_instr[0], (MIPS_GET_FUNC(mips) == MIPS_FUNC_DIV)
		                      ? PPC_FUNC_DIVW : PPC_FUNC_DIVWU);
		PPC_SET_RA    (lo_instr[0], MIPS_GET_RA(mips));
		PPC_SET_RB    (lo_instr[0], MIPS_GET_RB(mips));
		lo_shift[0][0] = PPC_RD_SHIFT;
		lo_shift[0][1] = -1;
		
		/*
		// FIXME:  This assumes that the move from hi/lo comes
		//         immediately after the div, or never at all.
		//         This may be a fair assumption... It's not
		i = 2;
		while(i){
		MIPS_instr next = peek_next_src();
		if(MIPS_GET_OPCODE(next) == MIPS_OPCODE_R){
			int func = MIPS_GET_FUNC(next);
			if(func == MIPS_FUNC_MFHI){ // Getting the remainder
				get_next_src();     // Since we're using next, pop it
				//int rd, rs, rt;
				#define rd MIPS_GET_RD(next)
				#define ra MIPS_GET_RA(mips)
				#define rb MIPS_GET_RB(mips)
				// divw  rD, rA, rB
				PPC_SET_OPCODE(ppc, PPC_OPCODE_X);
				PPC_SET_FUNC  (ppc, (MIPS_GET_FUNC(mips) == MIPS_FUNC_DIV)
				                      ? PPC_FUNC_DIVW : PPC_FUNC_DIVWU);
				PPC_SET_RD    (ppc, rd);
				PPC_SET_RA    (ppc, ra);
				PPC_SET_RB    (ppc, rb);
				set_next_dst  (ppc);
				// mullw rD, rD, rB
				ppc = NEW_PPC_INSTR();
				PPC_SET_OPCODE(ppc, PPC_OPCODE_X);
				PPC_SET_FUNC  (ppc, PPC_FUNC_MULLW);
				PPC_SET_RD    (ppc, rd);
				PPC_SET_RA    (ppc, rd);
				PPC_SET_RB    (ppc, rb);
				set_next_dst  (ppc);
				// subf rD, rD, rA
				ppc = NEW_PPC_INSTR();
				PPC_SET_OPCODE(ppc, PPC_OPCODE_X);
				PPC_SET_FUNC  (ppc, PPC_FUNC_SUBF);
				PPC_SET_RD    (ppc, rd);
				PPC_SET_RA    (ppc, rd);
				PPC_SET_RB    (ppc, ra);
				set_next_dst  (ppc);
				#undef rd
				#undef ra
				#undef rb
			} else if(func == MIPS_FUNC_MFLO){ // Getting the quotient
				get_next_src();            // Since we're using next, pop it
				PPC_SET_OPCODE(ppc, PPC_OPCODE_X);
				PPC_SET_FUNC  (ppc, (MIPS_GET_FUNC(mips) == MIPS_FUNC_DIV)
				                      ? PPC_FUNC_DIVW : PPC_FUNC_DIVWU);
				PPC_SET_RD    (ppc, MIPS_GET_RD(next));
				PPC_SET_RA    (ppc, MIPS_GET_RA(mips));
				PPC_SET_RB    (ppc, MIPS_GET_RB(mips));
				set_next_dst  (ppc);
			} else if(i == 2) return CONVERT_WARNING; // Warning because the mfhi/lo is later
		} else if(i == 2) return CONVERT_WARNING;
		--i;
		}
		*/
		return CONVERT_SUCCESS;
	case MIPS_FUNC_ADD:
		PPC_SET_OE    (ppc, 1);
	case MIPS_FUNC_ADDU:
		// Check to see if we're moving the lr
		if(MIPS_GET_RT(mips) == MIPS_REG_LR){
			// mflr mips_lr
			PPC_SET_OPCODE(ppc, PPC_OPCODE_X);
			PPC_SET_FUNC  (ppc, PPC_FUNC_MFSPR);
			PPC_SET_RD    (ppc, MIPS_REG_LR);
			PPC_SET_SPR   (ppc, 0x100);
			set_next_dst(ppc);
			ppc = NEW_PPC_INSTR();
		}
		PPC_SET_OPCODE(ppc, PPC_OPCODE_X);
		PPC_SET_FUNC  (ppc, PPC_FUNC_ADD);
		CONVERT_REGS  (ppc, mips);
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
	case MIPS_FUNC_SUB:
		PPC_SET_OE    (ppc, 1);
	case MIPS_FUNC_SUBU:
		PPC_SET_OPCODE(ppc, PPC_OPCODE_X);
		PPC_SET_FUNC  (ppc, PPC_FUNC_SUBF);
		PPC_SET_RD    (ppc, MIPS_GET_RD(mips));
		PPC_SET_RA    (ppc, MIPS_GET_RB(mips));
		PPC_SET_RB    (ppc, MIPS_GET_RA(mips));
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
	case MIPS_FUNC_AND:
		PPC_SET_OPCODE(ppc, PPC_OPCODE_X);
		PPC_SET_FUNC  (ppc, PPC_FUNC_AND);
		CONVERT_REGS  (ppc, mips);
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
	case MIPS_FUNC_OR:
		PPC_SET_OPCODE(ppc, PPC_OPCODE_X);
		PPC_SET_FUNC  (ppc, PPC_FUNC_OR);
		CONVERT_REGS  (ppc, mips);
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
	case MIPS_FUNC_XOR:
		PPC_SET_OPCODE(ppc, PPC_OPCODE_X);
		PPC_SET_FUNC  (ppc, PPC_FUNC_XOR);
		CONVERT_REGS  (ppc, mips);
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
	case MIPS_FUNC_NOR:
		PPC_SET_OPCODE(ppc, PPC_OPCODE_X);
		PPC_SET_FUNC  (ppc, PPC_FUNC_NOR);
		CONVERT_REGS  (ppc, mips);
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
	case MIPS_FUNC_SLT:
	case MIPS_FUNC_SLTU:
		// cmp
		PPC_SET_OPCODE(ppc, PPC_OPCODE_X);
		PPC_SET_FUNC  (ppc, (MIPS_GET_FUNC(mips) == MIPS_FUNC_SLT)
		                     ? PPC_FUNC_CMP : PPC_FUNC_CMPL);
		PPC_SET_RA    (ppc, MIPS_GET_RS(mips));
		PPC_SET_RB    (ppc, MIPS_GET_RT(mips));
		PPC_SET_CRF   (ppc, 7); // Use CR7
		set_next_dst(ppc);
		// mfcr
		ppc = NEW_PPC_INSTR();
		PPC_SET_OPCODE(ppc, PPC_OPCODE_X);
		PPC_SET_FUNC  (ppc, PPC_FUNC_MFCR);
		PPC_SET_RD    (ppc, MIPS_GET_RD(mips));
		set_next_dst(ppc);
		// rlwinm
		ppc = NEW_PPC_INSTR();
		PPC_SET_OPCODE(ppc, PPC_OPCODE_RLWINM);
		PPC_SET_RD    (ppc, MIPS_GET_RD(mips));
		PPC_SET_RA    (ppc, MIPS_GET_RD(mips));
		PPC_SET_SH    (ppc, 29); // Rotate LT bit to position 0
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
	case MIPS_FUNC_TGE:
	case MIPS_FUNC_TGEU:
	case MIPS_FUNC_TLT:
	case MIPS_FUNC_TLTU:
	case MIPS_FUNC_TEQ:
	case MIPS_FUNC_TNE:
#ifdef INTERPRET_TRAPS
		genCallInterp(mips);
		return INTERPRETED;
#else
		return CONVERT_ERROR;
#endif
	// TODO: Some of these can be recompiled with some work
	case MIPS_FUNC_DADD:
	case MIPS_FUNC_DADDU:
	case MIPS_FUNC_DDIV:
	case MIPS_FUNC_DDIVU:
	case MIPS_FUNC_DMULT:
	case MIPS_FUNC_DMULTU:
	case MIPS_FUNC_DSLL:
	case MIPS_FUNC_DSLL32:
	case MIPS_FUNC_DSLLV:
	case MIPS_FUNC_DSRA:
	case MIPS_FUNC_DSRA32:
	case MIPS_FUNC_DSRAV:
	case MIPS_FUNC_DSRL:
	case MIPS_FUNC_DSRL32:
	case MIPS_FUNC_DSRLV:
	case MIPS_FUNC_DSUB:
	case MIPS_FUNC_DSUBU:
#ifdef INTERPRET_DW
		genCallInterp(mips);
		return INTERPRETED;
#else
		return CONVERT_ERROR;
#endif

	default:
		return CONVERT_ERROR;
	}
}

static int convert_B(MIPS_instr mips){
	PowerPC_instr ppc = NEW_PPC_INSTR();
	int bo;

	switch(MIPS_GET_RT(mips)){

	case MIPS_RT_BLTZL:
	case MIPS_RT_BGEZL:
	case MIPS_RT_BLTZ:
	case MIPS_RT_BGEZ:
		bo = (MIPS_GET_OPCODE(mips) == MIPS_RT_BLTZ  ||
		      MIPS_GET_OPCODE(mips) == MIPS_RT_BLTZL) ?
		           0xc : 0x4;
		// cmpi to 0
		PPC_SET_OPCODE(ppc, PPC_OPCODE_CMPI);
		PPC_SET_RA    (ppc, MIPS_GET_RA(mips));
		PPC_SET_IMMED (ppc, 0);
		PPC_SET_CRF   (ppc, 7); // Use CR7
		set_next_dst(ppc);
		// Likely branches skip the delay slot if they're not taken
		if(MIPS_GET_RT(mips) == MIPS_RT_BLTZL ||
		   MIPS_GET_RT(mips) == MIPS_RT_BGEZL ){
			// b[!cond] <past delay & branch>
			ppc = NEW_PPC_INSTR();
			PPC_SET_OPCODE(ppc, PPC_OPCODE_BC);
			PPC_SET_BD(ppc, (peek_next_src() == 0) ? 2 : 3); // past delay & branch
			PPC_SET_BO(ppc, bo^0x8); // !cond
			PPC_SET_BI(ppc, 28);     // Check CR bit 28 (CR7, LT FIELD)
			set_next_dst(ppc);
		}
		// delay slot
		check_delaySlot();
		// temp is used for is_out
		temp = 0;
		ppc = NEW_PPC_INSTR();
		if(is_j_out(signExtend(MIPS_GET_IMMED(mips),16), 0)){
			temp = 1;
			// Allocate space for jumping out, 5 NOPs
			PPC_SET_OPCODE(ppc, PPC_OPCODE_ORI);
			set_next_dst(ppc);
			set_next_dst(ppc);
			set_next_dst(ppc);
			set_next_dst(ppc);
			set_next_dst(ppc);
			ppc = NEW_PPC_INSTR();
		}
		// bc
		PPC_SET_OPCODE(ppc, PPC_OPCODE_BC);
		PPC_SET_BD(ppc, add_jump(signExtend(MIPS_GET_IMMED(mips),16), 0, temp));
		PPC_SET_BO(ppc, bo);  // Test if CR is 1 or 0
		PPC_SET_BI(ppc, 28);  // Check CR bit 28 (CR7, LT FIELD)
		set_next_dst(ppc);
		// Add space to zero r0 if its not taken
		if(temp){
			PPC_SET_OPCODE(ppc, PPC_OPCODE_ORI);
			set_next_dst(ppc);
		}
		return CONVERT_SUCCESS;
	case MIPS_RT_TGEI:
	case MIPS_RT_TGEIU:
	case MIPS_RT_TLTI:
	case MIPS_RT_TLTIU:
	case MIPS_RT_TEGI:
	case MIPS_RT_TNEI:
#ifdef INTERPRET_TRAPS
		genCallInterp(mips);
		return INTERPRETED;
#else
		return CONVERT_ERROR;
#endif
	case MIPS_RT_BLTZALL:
	case MIPS_RT_BGEZALL:
	case MIPS_RT_BLTZAL:
	case MIPS_RT_BGEZAL:
		bo = (MIPS_GET_RT(mips) == MIPS_RT_BLTZAL  ||
		      MIPS_GET_RT(mips) == MIPS_RT_BLTZALL) ?
		           0xc : 0x4;
		// cmpi to 0
		PPC_SET_OPCODE(ppc, PPC_OPCODE_CMPI);
		PPC_SET_RA    (ppc, MIPS_GET_RA(mips));
		PPC_SET_IMMED (ppc, 0);
		PPC_SET_CRF   (ppc, 7); // Use CR7
		set_next_dst(ppc);
		// Likely branches skip the delay slot if they're not taken
		if(MIPS_GET_RT(mips) == MIPS_RT_BLTZALL ||
		   MIPS_GET_RT(mips) == MIPS_RT_BGEZALL ){
			// b[!cond] <past delay & branch>
			ppc = NEW_PPC_INSTR();
			PPC_SET_OPCODE(ppc, PPC_OPCODE_BC);
			PPC_SET_BD(ppc, (peek_next_src() == 0) ? 2 : 3); // past delay & branch
			PPC_SET_BO(ppc, bo^0x8); // !cond
			PPC_SET_BI(ppc, 28);     // Check CR bit 28 (CR7, LT FIELD)
			set_next_dst(ppc);
		}
		// delay slot
		check_delaySlot();
		// temp is used for is_out
		temp = 0;
		ppc = NEW_PPC_INSTR();
		if(is_j_out(signExtend(MIPS_GET_IMMED(mips),16), 0)){
			temp = 1;
			// Allocate space for jumping out, 5 NOPs
			PPC_SET_OPCODE(ppc, PPC_OPCODE_ORI);
			set_next_dst(ppc);
			set_next_dst(ppc);
			set_next_dst(ppc);
			set_next_dst(ppc);
			set_next_dst(ppc);
			ppc = NEW_PPC_INSTR();
		}
		// bc
		PPC_SET_OPCODE(ppc, PPC_OPCODE_BC);
		PPC_SET_BD(ppc, add_jump(signExtend(MIPS_GET_IMMED(mips),16), 0, temp));
		PPC_SET_BO(ppc, bo);  // Test if CR is 1 or 0
		PPC_SET_BI(ppc, 28);  // Check CR bit 28 (CR7, LT FIELD)
		PPC_SET_LK(ppc, 1);
		set_next_dst(ppc);
		// Add space to zero r0 if its not taken
		if(temp){
			PPC_SET_OPCODE(ppc, PPC_OPCODE_ORI);
			set_next_dst(ppc);
		}
		return CONVERT_SUCCESS;
	default:
		return CONVERT_ERROR;
	}
}

#define PRECISION_SINGLE 1
#define PRECISION_DOUBLE 2
#define PRECISION_WORD   0
#define PRECISION_LONG   3
static int convert_CoP(MIPS_instr mips, int z){
	PowerPC_instr ppc = NEW_PPC_INSTR();
	
	switch(MIPS_GET_RS(mips)){
	// FIXME: mfc1/mtc1 shouldn't do lwz/stw * temp@l(*)
	case MIPS_FRMT_MFC:
		if(z == 0){
#ifdef INTEPRET_COP0
			genCallInterp(mips);
			return INTERPRETED;
#else
			return CONVERT_ERROR;
#endif
		} else if(z == 1){
			// lis	rd, temp@ha(0)
			PPC_SET_OPCODE(ppc, PPC_OPCODE_ADDIS);
			PPC_SET_RD    (ppc, MIPS_GET_RT(mips));
			PPC_SET_IMMED (ppc, (unsigned int)&temp>>16);
			set_next_dst(ppc);
			// la	rd, temp@l(rd)
			ppc = NEW_PPC_INSTR();
			PPC_SET_OPCODE(ppc, PPC_OPCODE_ORI);
			PPC_SET_RD    (ppc, MIPS_GET_RT(mips));
			PPC_SET_RA    (ppc, MIPS_GET_RT(mips));
			PPC_SET_IMMED (ppc, (unsigned int)&temp);
			// stfs	fs, 0(rd)
			ppc = NEW_PPC_INSTR();
			PPC_SET_OPCODE(ppc, PPC_OPCODE_STFS);
			PPC_SET_RD    (ppc, MIPS_GET_FS(mips));
			PPC_SET_RA    (ppc, MIPS_GET_RT(mips));
			set_next_dst(ppc);
			// lwz	rd, 0(rd)
			ppc = NEW_PPC_INSTR();
			PPC_SET_OPCODE(ppc, PPC_OPCODE_LWZ);
			PPC_SET_RD    (ppc, MIPS_GET_RT(mips));
			PPC_SET_RA    (ppc, MIPS_GET_RT(mips));
			set_next_dst(ppc);
			return CONVERT_SUCCESS;
		} else
			return CONVERT_ERROR;
	case MIPS_FRMT_DMFC:
		if(z == 1){
			// lis	rd, reg@ha(0)
			PPC_SET_OPCODE(ppc, PPC_OPCODE_ADDIS);
			PPC_SET_RD    (ppc, MIPS_GET_RT(mips));
			PPC_SET_IMMED (ppc, (unsigned int)reg>>16);
			set_next_dst(ppc);
			// la	rd, reg@l(rd)
			ppc = NEW_PPC_INSTR();
			PPC_SET_OPCODE(ppc, PPC_OPCODE_ORI);
			PPC_SET_RD    (ppc, MIPS_GET_RT(mips));
			PPC_SET_RA    (ppc, MIPS_GET_RT(mips));
			PPC_SET_IMMED (ppc, (unsigned int)reg);
			set_next_dst(ppc);
			// stfd	fs, rd*8(rd)
			ppc = NEW_PPC_INSTR();
			PPC_SET_OPCODE(ppc,  PPC_OPCODE_STFD);
			PPC_SET_RD    (ppc,  MIPS_GET_FS(mips));
			PPC_SET_RA    (ppc,  MIPS_GET_RT(mips));
			PPC_SET_IMMED (ppc, (MIPS_GET_RT(mips)*8));
			set_next_dst(ppc);
			// lwz	rd, rd*8+4(rd)
			ppc = NEW_PPC_INSTR();
			PPC_SET_OPCODE(ppc,  PPC_OPCODE_LWZ);
			PPC_SET_RD    (ppc,  MIPS_GET_RT(mips));
			PPC_SET_RA    (ppc,  MIPS_GET_RT(mips));
			PPC_SET_IMMED (ppc, (MIPS_GET_RT(mips)*8+4));
			set_next_dst(ppc);
			return CONVERT_SUCCESS;
		} else return CONVERT_ERROR;
	case MIPS_FRMT_CFC:
		return CONVERT_ERROR;
	case MIPS_FRMT_MTC:
		if(z == 0){
#ifdef INTERPRET_COP0
			genCallInterp(mips);
			return INTERPRETED;
#else
			return CONVERT_ERROR;
#endif
		} else if(z == 1){
			// move	r0, rd
			PPC_SET_OPCODE(ppc, PPC_OPCODE_ADDI);
			PPC_SET_RA    (ppc, MIPS_GET_RT(mips));
			set_next_dst(ppc);
			// lis	rd, temp@ha(0)
			ppc = NEW_PPC_INSTR();
			PPC_SET_OPCODE(ppc, PPC_OPCODE_ADDIS);
			PPC_SET_RD    (ppc, MIPS_GET_RT(mips));
			PPC_SET_IMMED (ppc, (unsigned int)&temp>>16);
			set_next_dst(ppc);
			// la	rd, temp@l(rd)
			ppc = NEW_PPC_INSTR();
			PPC_SET_OPCODE(ppc, PPC_OPCODE_ORI);
			PPC_SET_RD    (ppc, MIPS_GET_RT(mips));
			PPC_SET_RA    (ppc, MIPS_GET_RT(mips));
			PPC_SET_IMMED (ppc, (unsigned int)&temp);
			// stw	r0, 0(rd)
			ppc = NEW_PPC_INSTR();
			PPC_SET_OPCODE(ppc, PPC_OPCODE_STW);
			PPC_SET_RD    (ppc, 0);
			PPC_SET_RA    (ppc, MIPS_GET_RT(mips));
			set_next_dst(ppc);
			// lfs	fs, 0(rd)
			ppc = NEW_PPC_INSTR();
			PPC_SET_OPCODE(ppc, PPC_OPCODE_LFS);
			PPC_SET_RD    (ppc, MIPS_GET_FS(mips));
			PPC_SET_RA    (ppc, MIPS_GET_RT(mips));
			set_next_dst(ppc);
			// move	rd, r0 (mtctr, mfctr)
			ppc = NEW_PPC_INSTR();
			PPC_SET_OPCODE(ppc, PPC_OPCODE_X);
			PPC_SET_FUNC  (ppc, PPC_FUNC_MTSPR);
			PPC_SET_RD    (ppc, 0);
			PPC_SET_SPR   (ppc, 0x120);
			set_next_dst(ppc);
			ppc = NEW_PPC_INSTR();
			PPC_SET_OPCODE(ppc, PPC_OPCODE_X);
			PPC_SET_FUNC  (ppc, PPC_FUNC_MFSPR);
			PPC_SET_RD    (ppc, MIPS_GET_RT(mips));
			PPC_SET_SPR   (ppc, 0x120);
			set_next_dst(ppc);
			// andi	r0, r0, 0
			ppc = NEW_PPC_INSTR();
			PPC_SET_OPCODE(ppc, PPC_OPCODE_ANDI);
			set_next_dst(ppc);
			return CONVERT_SUCCESS;
		} else
			return CONVERT_ERROR;
	case MIPS_FRMT_DMTC:
		if(z == 1){
			// temp designates which register to hold the reg addr
			temp = (MIPS_GET_RT(mips) == 1) ? 2 : 1;
			// mtctr r1
			PPC_SET_OPCODE(ppc, PPC_OPCODE_X);
			PPC_SET_FUNC  (ppc, PPC_FUNC_MTSPR);
			PPC_SET_RD    (ppc, temp);
			PPC_SET_SPR   (ppc, 0x120);
			set_next_dst(ppc);
			// lis	r1, reg@ha(0)
			ppc = NEW_PPC_INSTR();
			PPC_SET_OPCODE(ppc, PPC_OPCODE_ADDIS);
			PPC_SET_RD    (ppc, temp);
			PPC_SET_IMMED (ppc, (unsigned int)reg>>16);
			set_next_dst(ppc);
			// la	r1, reg@l(r1)
			ppc = NEW_PPC_INSTR();
			PPC_SET_OPCODE(ppc, PPC_OPCODE_ORI);
			PPC_SET_RD    (ppc, temp);
			PPC_SET_RA    (ppc, temp);
			PPC_SET_IMMED (ppc, (unsigned int)reg);
			set_next_dst(ppc);
			// stw	ra, ra*8+4(r1)
			ppc = NEW_PPC_INSTR();
			PPC_SET_OPCODE(ppc, PPC_OPCODE_STW);
			PPC_SET_RD    (ppc, MIPS_GET_RT(mips));
			PPC_SET_RA    (ppc, temp);
			PPC_SET_IMMED (ppc, (MIPS_GET_RT(mips)*8 + 4));
			set_next_dst(ppc);
			// lfd	fd, ra*8(r1)
			ppc = NEW_PPC_INSTR();
			PPC_SET_OPCODE(ppc, PPC_OPCODE_LFD);
			PPC_SET_RD    (ppc, MIPS_GET_FS(mips));
			PPC_SET_RA    (ppc, temp);
			PPC_SET_IMMED (ppc, (MIPS_GET_RT(mips)*8));
			set_next_dst(ppc);
			// mfctr r1
			ppc = NEW_PPC_INSTR();
			PPC_SET_OPCODE(ppc, PPC_OPCODE_X);
			PPC_SET_FUNC  (ppc, PPC_FUNC_MFSPR);
			PPC_SET_RD    (ppc, temp);
			PPC_SET_SPR   (ppc, 0x120);
			set_next_dst(ppc);
			return CONVERT_SUCCESS;
		} else return CONVERT_ERROR;		
	case MIPS_FRMT_CTC:
		return CONVERT_ERROR;
	case MIPS_FRMT_BC:
		if(z == 1)
			switch((mips >> 16) & 0x3){
			case 3: //bczfl
				// Likely branches skip the delay slot if they're not taken
				// b[!cond] <past delay & branch>
				ppc = NEW_PPC_INSTR();
				PPC_SET_OPCODE(ppc, PPC_OPCODE_BC);
				PPC_SET_BD(ppc, (peek_next_src() == 0) ? 2 : 3); // past delay & branch
				PPC_SET_BO(ppc, 0xc); // !cond
				PPC_SET_BI(ppc, (20 + MIPS_GET_CC(mips)));     // Check cc
				set_next_dst(ppc);
				ppc = NEW_PPC_INSTR();
			case 0: //bczf
				check_delaySlot();
				// temp is used for is_out
				temp = 0;
				if(is_j_out(signExtend(MIPS_GET_IMMED(mips),16), 0)){
					temp = 1;
					// Allocate space for jumping out, 5 NOPs
					PPC_SET_OPCODE(ppc, PPC_OPCODE_ORI);
					set_next_dst(ppc);
					set_next_dst(ppc);
					set_next_dst(ppc);
					set_next_dst(ppc);
					set_next_dst(ppc);
					ppc = NEW_PPC_INSTR();
				}
				PPC_SET_OPCODE(ppc, PPC_OPCODE_BC);
				PPC_SET_BD(ppc, add_jump(signExtend(MIPS_GET_IMMED(mips),16), 0, temp));
				PPC_SET_BO(ppc, 0x4);  // Test if cc is 1 or 0
				PPC_SET_BI(ppc, (20 + MIPS_GET_CC(mips)));  // Check cc
				set_next_dst(ppc);
				// Add space to zero r0 if its not taken
				if(temp){
					PPC_SET_OPCODE(ppc, PPC_OPCODE_ORI);
					set_next_dst(ppc);
				}
				return CONVERT_SUCCESS;
			case 2: //bcztl
				// Likely branches skip the delay slot if they're not taken
				// b[!cond] <past delay & branch>
				ppc = NEW_PPC_INSTR();
				PPC_SET_OPCODE(ppc, PPC_OPCODE_BC);
				PPC_SET_BD(ppc, (peek_next_src() == 0) ? 2 : 3); // past delay & branch
				PPC_SET_BO(ppc, 0x4); // !cond
				PPC_SET_BI(ppc, (20 + MIPS_GET_CC(mips)));     // Check cc
				set_next_dst(ppc);
				ppc = NEW_PPC_INSTR();
			case 1: //bczt
				check_delaySlot();
				// temp is used for is_out
				temp = 0;
				if(is_j_out(signExtend(MIPS_GET_IMMED(mips),16), 0)){
					temp = 1;
					// Allocate space for jumping out, 5 NOPs
					PPC_SET_OPCODE(ppc, PPC_OPCODE_ORI);
					set_next_dst(ppc);
					set_next_dst(ppc);
					set_next_dst(ppc);
					set_next_dst(ppc);
					set_next_dst(ppc);
					ppc = NEW_PPC_INSTR();
				}
				PPC_SET_OPCODE(ppc, PPC_OPCODE_BC);
				PPC_SET_BD(ppc, add_jump(signExtend(MIPS_GET_IMMED(mips),16), 0, temp));
				PPC_SET_BO(ppc, 0xc);  // Test if cc is 1 or 0
				PPC_SET_BI(ppc, (20 + MIPS_GET_CC(mips)));  // Check cc
				set_next_dst(ppc);
				// Add space to zero r0 if its not taken
				if(temp){
					PPC_SET_OPCODE(ppc, PPC_OPCODE_ORI);
					set_next_dst(ppc);
				}
				return CONVERT_SUCCESS;
			}
		else
			return CONVERT_ERROR;
	case MIPS_FRMT_COP1:
		if(z == 1)
			return convert_FP(mips, PRECISION_SINGLE);
		else if(z == 0) // TLB instructions
			switch(MIPS_GET_FUNC(mips)){
			case MIPS_FUNC_TLBR:
			case MIPS_FUNC_TLBWI:
			case MIPS_FUNC_TLBWR:
			case MIPS_FUNC_TLBP:
			case MIPS_FUNC_ERET:
			case MIPS_FUNC_DERET:
#ifdef INTERPRET_COP0
				genCallInterp(mips);
				return INTERPRETED;
#else
				return CONVERT_ERROR;
#endif
			default:
				return CONVERT_ERROR;
			}
		else
			return CONVERT_ERROR;
	case MIPS_FRMT_COP2:
		if(z == 1)
			return convert_FP(mips, PRECISION_DOUBLE);
		else      
			return CONVERT_ERROR;
	case MIPS_FRMT_COP3:
		if(z == 1)
			return convert_FP(mips, PRECISION_WORD);
		else
			return CONVERT_ERROR;
	case MIPS_FRMT_COP4:
		if(z == 1)
			return convert_FP(mips, PRECISION_LONG);
		else
			return CONVERT_ERROR;
		
	default:
		return CONVERT_ERROR;
	}
}

#define CONVERT_FPRS(ppc, mips) do { \
        PPC_SET_RD    (ppc, MIPS_GET_FD(mips)); \
	PPC_SET_RA    (ppc, MIPS_GET_FS(mips)); \
	PPC_SET_RB    (ppc, MIPS_GET_FT(mips)); } while(0)

// FIXME: The floating-point instructions need to be rewritten
//        PowerPC uses doubles for all calculations, it wouldn't hurt
static int convert_FP(MIPS_instr mips, int precision){
	PowerPC_instr ppc = NEW_PPC_INSTR();
	
	// FIXME: I don't think it works quite like this
	/*if(precision == PRECISION_SINGLE)
		PPC_SET_OPCODE(ppc, PPC_OPCODE_FPS);
	else if(precision == PRECISION_DOUBLE)
		PPC_SET_OPCODE(ppc, PPC_OPCODE_FPD);
	else return CONVERT_ERROR;*/
	
	PPC_SET_OPCODE(ppc, PPC_OPCODE_FPD);
	
	switch(MIPS_GET_FUNC(mips)){
	
	case MIPS_FUNC_ADD_:
		PPC_SET_FUNC(ppc, PPC_FUNC_FADD);
		CONVERT_FPRS(ppc, mips);
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
	case MIPS_FUNC_SUB_:
		PPC_SET_FUNC(ppc, PPC_FUNC_FSUB);
		CONVERT_FPRS(ppc, mips);
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
	case MIPS_FUNC_MUL_:
		PPC_SET_FUNC(ppc, PPC_FUNC_FMUL);
		PPC_SET_RD  (ppc, MIPS_GET_FD(mips));
		PPC_SET_RA  (ppc, MIPS_GET_FS(mips));
		PPC_SET_RC  (ppc, MIPS_GET_FT(mips));
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
	case MIPS_FUNC_DIV_:
		PPC_SET_FUNC(ppc, PPC_FUNC_FDIV);
		CONVERT_FPRS(ppc, mips);
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
	case MIPS_FUNC_SQRT_:
		PPC_SET_FUNC(ppc, PPC_FUNC_FSQRT);
		PPC_SET_RD  (ppc, MIPS_GET_FD(mips));
		PPC_SET_RB  (ppc, MIPS_GET_FS(mips));
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
	case MIPS_FUNC_ABS_:
		PPC_SET_FUNC(ppc, PPC_FUNC_FABS);
		PPC_SET_RD  (ppc, MIPS_GET_FD(mips));
		PPC_SET_RB  (ppc, MIPS_GET_FS(mips));
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
	case MIPS_FUNC_MOV_:
		PPC_SET_FUNC(ppc, PPC_FUNC_FMR);
		PPC_SET_RD  (ppc, MIPS_GET_FD(mips));
		PPC_SET_RB  (ppc, MIPS_GET_FS(mips));
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
	case MIPS_FUNC_NEG_:
		PPC_SET_FUNC(ppc, PPC_FUNC_FNEG);
		PPC_SET_RD  (ppc, MIPS_GET_FD(mips));
		PPC_SET_RB  (ppc, MIPS_GET_FS(mips));
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
	case MIPS_FUNC_ROUND_W_:
		PPC_SET_FUNC  (ppc, PPC_FUNC_MTFSB0);
		PPC_SET_RD    (ppc, 30);
		set_next_dst  (ppc);
		ppc = NEW_PPC_INSTR();
		PPC_SET_OPCODE(ppc, PPC_OPCODE_FPD);
		PPC_SET_FUNC  (ppc, PPC_FUNC_MTFSB0);
		PPC_SET_RD    (ppc, 31);
		set_next_dst(ppc);
		
		ppc = NEW_PPC_INSTR();
		PPC_SET_OPCODE(ppc, PPC_OPCODE_FPD);
		PPC_SET_FUNC  (ppc, PPC_FUNC_FCTIW);
		PPC_SET_RD    (ppc, MIPS_GET_FD(mips));
		PPC_SET_RB    (ppc, MIPS_GET_FS(mips));
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
	case MIPS_FUNC_ROUND_L_:
		PPC_SET_FUNC  (ppc, PPC_FUNC_MTFSB0);
		PPC_SET_RD    (ppc, 30);
		set_next_dst  (ppc);
		ppc = NEW_PPC_INSTR();
		PPC_SET_OPCODE(ppc, PPC_OPCODE_FPD);
		PPC_SET_FUNC  (ppc, PPC_FUNC_MTFSB0);
		PPC_SET_RD    (ppc, 31);
		set_next_dst(ppc);
		
		ppc = NEW_PPC_INSTR();
		PPC_SET_OPCODE(ppc, PPC_OPCODE_FPD);
		PPC_SET_FUNC  (ppc, PPC_FUNC_FCTID);
		PPC_SET_RD    (ppc, MIPS_GET_FD(mips));
		PPC_SET_RB    (ppc, MIPS_GET_FS(mips));
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
	case MIPS_FUNC_FLOOR_W_:
		PPC_SET_FUNC  (ppc, PPC_FUNC_MTFSB1);
		PPC_SET_RD    (ppc, 30);
		set_next_dst(ppc);
		ppc = NEW_PPC_INSTR();
		PPC_SET_OPCODE(ppc, PPC_OPCODE_FPD);
		PPC_SET_FUNC  (ppc, PPC_FUNC_MTFSB1);
		PPC_SET_RD    (ppc, 31);
		set_next_dst(ppc);
		
		ppc = NEW_PPC_INSTR();
		PPC_SET_FUNC(ppc, PPC_FUNC_FCTIW);
		PPC_SET_RD  (ppc, MIPS_GET_FD(mips));
		PPC_SET_RB  (ppc, MIPS_GET_FS(mips));
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
	case MIPS_FUNC_FLOOR_L_:
		PPC_SET_FUNC  (ppc, PPC_FUNC_MTFSB1);
		PPC_SET_RD    (ppc, 30);
		set_next_dst(ppc);
		ppc = NEW_PPC_INSTR();
		PPC_SET_OPCODE(ppc, PPC_OPCODE_FPD);
		PPC_SET_FUNC  (ppc, PPC_FUNC_MTFSB1);
		PPC_SET_RD    (ppc, 31);
		set_next_dst(ppc);
		
		ppc = NEW_PPC_INSTR();
		PPC_SET_OPCODE(ppc, PPC_OPCODE_FPD);
		PPC_SET_FUNC  (ppc, PPC_FUNC_FCTID);
		PPC_SET_RD    (ppc, MIPS_GET_FD(mips));
		PPC_SET_RB    (ppc, MIPS_GET_FS(mips));
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
	case MIPS_FUNC_TRUNC_W_:
		PPC_SET_FUNC(ppc, PPC_FUNC_FCTIWZ);
		PPC_SET_RD  (ppc, MIPS_GET_FD(mips));
		PPC_SET_RB  (ppc, MIPS_GET_FS(mips));
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
	case MIPS_FUNC_TRUNC_L_:
		PPC_SET_FUNC(ppc, PPC_FUNC_FCTIDZ);
		PPC_SET_RD  (ppc, MIPS_GET_FD(mips));
		PPC_SET_RB  (ppc, MIPS_GET_FS(mips));
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
	case MIPS_FUNC_CEIL_W_:
		PPC_SET_FUNC  (ppc, PPC_FUNC_MTFSB1);
		PPC_SET_RD    (ppc, 30);
		set_next_dst(ppc);
		ppc = NEW_PPC_INSTR();
		PPC_SET_OPCODE(ppc, PPC_OPCODE_FPD);
		PPC_SET_FUNC  (ppc, PPC_FUNC_MTFSB0);
		PPC_SET_RD    (ppc, 31);
		set_next_dst(ppc);
		
		ppc = NEW_PPC_INSTR();
		PPC_SET_OPCODE(ppc, PPC_OPCODE_FPD);
		PPC_SET_FUNC  (ppc, PPC_FUNC_FCTIW);
		PPC_SET_RD    (ppc, MIPS_GET_FD(mips));
		PPC_SET_RB    (ppc, MIPS_GET_FS(mips));
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
	case MIPS_FUNC_CEIL_L_:
		PPC_SET_FUNC  (ppc, PPC_FUNC_MTFSB1);
		PPC_SET_RD    (ppc, 30);
		set_next_dst(ppc);
		ppc = NEW_PPC_INSTR();
		PPC_SET_OPCODE(ppc, PPC_OPCODE_FPD);
		PPC_SET_FUNC  (ppc, PPC_FUNC_MTFSB0);
		PPC_SET_RD    (ppc, 31);
		set_next_dst(ppc);
		
		ppc = NEW_PPC_INSTR();
		PPC_SET_OPCODE(ppc, PPC_OPCODE_FPD);
		PPC_SET_FUNC  (ppc, PPC_FUNC_FCTID);
		PPC_SET_RD    (ppc, MIPS_GET_FD(mips));
		PPC_SET_RB    (ppc, MIPS_GET_FS(mips));
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
	case MIPS_FUNC_MOV__:
		// movt.f/movf.f
		// beq <past addi>
		ppc = NEW_PPC_INSTR();
		PPC_SET_OPCODE(ppc, PPC_OPCODE_BC);
		PPC_SET_BD(ppc, 2);
		PPC_SET_BO(ppc, (mips&0x10000) ? 0xc : 0x4); // Test if cc is t/f
		PPC_SET_BI(ppc, (20 + MIPS_GET_CC(mips)));   // Check cc (CR5/6)
		set_next_dst(ppc);
		// fmr fd, fs
		ppc = NEW_PPC_INSTR();
		PPC_SET_OPCODE(ppc, PPC_OPCODE_FPD);
		PPC_SET_FUNC  (ppc, PPC_FUNC_FMR);
		PPC_SET_RD    (ppc, MIPS_GET_FD(mips));
		PPC_SET_RB    (ppc, MIPS_GET_FS(mips));
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
	case MIPS_FUNC_MOVZ_:
	case MIPS_FUNC_MOVN_:
		// cmpi rt, 0
		ppc = NEW_PPC_INSTR();
		PPC_SET_OPCODE(ppc, PPC_OPCODE_CMPI);
		PPC_SET_RA    (ppc, MIPS_GET_RT(mips));
		PPC_SET_IMMED (ppc, 0);
		PPC_SET_CRF   (ppc, 7); // Use CR7
		set_next_dst(ppc);
		// bne <past addi>
		ppc = NEW_PPC_INSTR();
		PPC_SET_OPCODE(ppc, PPC_OPCODE_BC);
		PPC_SET_BD(ppc, 2);
		PPC_SET_BO(ppc, (MIPS_GET_FUNC(mips) == MIPS_FUNC_MOVZ)
		                 ? 0xc : 0x4);
		PPC_SET_BI(ppc, 30);  // Check CR bit 30 (CR7, EQ FIELD)
		set_next_dst(ppc);
		// fmr fd, fs
		ppc = NEW_PPC_INSTR();
		PPC_SET_OPCODE(ppc, PPC_OPCODE_FPD);
		PPC_SET_FUNC  (ppc, PPC_FUNC_FMR);
		PPC_SET_RD    (ppc, MIPS_GET_FD(mips));
		PPC_SET_RB    (ppc, MIPS_GET_FS(mips));
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
	case MIPS_FUNC_CVT_S_:
	case MIPS_FUNC_CVT_D_:
		// It shouldn't matter what precision float we're in
		if(precision == PRECISION_SINGLE || precision == PRECISION_DOUBLE)
			return CONVERT_SUCCESS;
		PPC_SET_FUNC(ppc, PPC_FUNC_FCFID);
		PPC_SET_RD  (ppc, MIPS_GET_FD(mips));
		PPC_SET_RB  (ppc, MIPS_GET_FS(mips));
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
	case MIPS_FUNC_CVT_W_:
		PPC_SET_FUNC(ppc, PPC_FUNC_FCTIWZ);
		PPC_SET_RD  (ppc, MIPS_GET_FD(mips));
		PPC_SET_RB  (ppc, MIPS_GET_FS(mips));
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
	case MIPS_FUNC_CVT_L_:
		PPC_SET_FUNC(ppc, PPC_FUNC_FCTIDZ);
		PPC_SET_RD  (ppc, MIPS_GET_FD(mips));
		PPC_SET_RB  (ppc, MIPS_GET_FS(mips));
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
	// Note: The emulator uses cr bits 20-27 for cc 0-7
	case MIPS_FUNC_C_F_:
		return CONVERT_ERROR;
	case MIPS_FUNC_C_UN_:
		PPC_SET_FUNC(ppc, PPC_FUNC_FCMPU);
		PPC_SET_RA  (ppc, MIPS_GET_FS(mips));
		PPC_SET_RB  (ppc, MIPS_GET_FT(mips));
		PPC_SET_CRF (ppc, 1);
		set_next_dst(ppc);
		// cr arithmetic
		// Clear the cc bit
		ppc = NEW_PPC_INSTR();
		PPC_SET_OPCODE(ppc, PPC_OPCODE_XL);
		PPC_SET_FUNC  (ppc, PPC_FUNC_CRXOR);
		PPC_SET_RD    (ppc, (MIPS_GET_CC(mips) + 20));
		PPC_SET_RA    (ppc, (MIPS_GET_CC(mips) + 20));
		PPC_SET_RB    (ppc, (MIPS_GET_CC(mips) + 20));
		set_next_dst(ppc);
		// Set the cc bit with the un bit
		ppc = NEW_PPC_INSTR();
		PPC_SET_OPCODE(ppc, PPC_OPCODE_XL);
		PPC_SET_FUNC  (ppc, PPC_FUNC_CROR);
		PPC_SET_RD    (ppc, (MIPS_GET_CC(mips) + 20));
		PPC_SET_RA    (ppc, (MIPS_GET_CC(mips) + 20));
		PPC_SET_RD    (ppc, 7); // unordered bit of CR-1
		set_next_dst(ppc);
		return CONVERT_SUCCESS; 
	case MIPS_FUNC_C_EQ_:
		PPC_SET_FUNC(ppc, PPC_FUNC_FCMPO);
		PPC_SET_RA  (ppc, MIPS_GET_FS(mips));
		PPC_SET_RB  (ppc, MIPS_GET_FT(mips));
		PPC_SET_CRF (ppc, 1);
		set_next_dst(ppc);
		// cr arithmetic
		// Clear the cc bit
		ppc = NEW_PPC_INSTR();
		PPC_SET_OPCODE(ppc, PPC_OPCODE_XL);
		PPC_SET_FUNC  (ppc, PPC_FUNC_CRXOR);
		PPC_SET_RD    (ppc, (MIPS_GET_CC(mips) + 20));
		PPC_SET_RA    (ppc, (MIPS_GET_CC(mips) + 20));
		PPC_SET_RB    (ppc, (MIPS_GET_CC(mips) + 20));
		set_next_dst(ppc);
		// Set the cc bit with the eq bit
		ppc = NEW_PPC_INSTR();
		PPC_SET_OPCODE(ppc, PPC_OPCODE_XL);
		PPC_SET_FUNC  (ppc, PPC_FUNC_CROR);
		PPC_SET_RD    (ppc, (MIPS_GET_CC(mips) + 20));
		PPC_SET_RA    (ppc, (MIPS_GET_CC(mips) + 20));
		PPC_SET_RD    (ppc, 6); // EQ bit of CR-1
		set_next_dst(ppc);
		return CONVERT_SUCCESS; 
	case MIPS_FUNC_C_OLT_: // ordered or lt
	case MIPS_FUNC_C_ULT_:
	case MIPS_FUNC_C_OLE_:
	case MIPS_FUNC_C_ULE_:
	case MIPS_FUNC_C_SF_:
	case MIPS_FUNC_C_NGLE_:
	case MIPS_FUNC_C_SEQ_:
	case MIPS_FUNC_C_NGL_:
		return CONVERT_ERROR;
	case MIPS_FUNC_C_NGE_:
	case MIPS_FUNC_C_LT_:
		PPC_SET_FUNC(ppc, PPC_FUNC_FCMPO);
		PPC_SET_RA  (ppc, MIPS_GET_FS(mips));
		PPC_SET_RB  (ppc, MIPS_GET_FT(mips));
		PPC_SET_CRF (ppc, 1);
		set_next_dst(ppc);
		// cr arithmetic
		// Clear the cc bit
		ppc = NEW_PPC_INSTR();
		PPC_SET_OPCODE(ppc, PPC_OPCODE_XL);
		PPC_SET_FUNC  (ppc, PPC_FUNC_CRXOR);
		PPC_SET_RD    (ppc, (MIPS_GET_CC(mips) + 20));
		PPC_SET_RA    (ppc, (MIPS_GET_CC(mips) + 20));
		PPC_SET_RB    (ppc, (MIPS_GET_CC(mips) + 20));
		set_next_dst(ppc);
		// Set the cc bit with the lt bit
		ppc = NEW_PPC_INSTR();
		PPC_SET_OPCODE(ppc, PPC_OPCODE_XL);
		PPC_SET_FUNC  (ppc, PPC_FUNC_CROR);
		PPC_SET_RD    (ppc, (MIPS_GET_CC(mips) + 20));
		PPC_SET_RA    (ppc, (MIPS_GET_CC(mips) + 20));
		PPC_SET_RD    (ppc, 4); // LT bit of CR-1
		set_next_dst(ppc);
		return CONVERT_SUCCESS; 
	case MIPS_FUNC_C_NGT_:
	case MIPS_FUNC_C_LE_:
		PPC_SET_FUNC(ppc, PPC_FUNC_FCMPO);
		PPC_SET_RA  (ppc, MIPS_GET_FS(mips));
		PPC_SET_RB  (ppc, MIPS_GET_FT(mips));
		PPC_SET_CRF (ppc, 1);
		set_next_dst(ppc);
		// cr arithmetic
		// Clear the cc bit
		ppc = NEW_PPC_INSTR();
		PPC_SET_OPCODE(ppc, PPC_OPCODE_XL);
		PPC_SET_FUNC  (ppc, PPC_FUNC_CRXOR);
		PPC_SET_RD    (ppc, (MIPS_GET_CC(mips) + 20));
		PPC_SET_RA    (ppc, (MIPS_GET_CC(mips) + 20));
		PPC_SET_RB    (ppc, (MIPS_GET_CC(mips) + 20));
		set_next_dst(ppc);
		// Set the cc bit with the !gt bit
		ppc = NEW_PPC_INSTR();
		PPC_SET_OPCODE(ppc, PPC_OPCODE_XL);
		PPC_SET_FUNC  (ppc, PPC_FUNC_CRNOR);
		PPC_SET_RD    (ppc, (MIPS_GET_CC(mips) + 20));
		PPC_SET_RA    (ppc, (MIPS_GET_CC(mips) + 20));
		PPC_SET_RD    (ppc, 5); // GT bit of CR-1
		set_next_dst(ppc);
		return CONVERT_SUCCESS; 
	default:
		return CONVERT_ERROR;
	}
}

static int convert_M(MIPS_instr mips){
	PowerPC_instr ppc = NEW_PPC_INSTR();
	switch(MIPS_GET_FUNC(mips)){
	
	case MIPS_FUNC_MADD:
		hi_instr_count = lo_instr_count = 0;
#ifdef INTERPRET_MADD
		genCallInterp(mips);
		return INTERPRETED;
#else
		return CONVERT_ERROR;
#endif
	case MIPS_FUNC_MADDU:
		hi_instr_count = lo_instr_count = 0;
#ifdef INTERPRET_MADDU
		genCallInterp(mips);
		return INTERPRETED;
#else
		return CONVERT_ERROR;
#endif
	case MIPS_FUNC_MUL:
		PPC_SET_OPCODE(ppc, PPC_OPCODE_X);
		PPC_SET_FUNC  (ppc, PPC_FUNC_MULLW);
		PPC_SET_RD    (ppc, MIPS_GET_RD(mips));
		PPC_SET_RA    (ppc, MIPS_GET_RA(mips));
		PPC_SET_RB    (ppc, MIPS_GET_RB(mips));
		set_next_dst  (ppc);
		return CONVERT_SUCCESS;
	case MIPS_FUNC_MSUB:
		hi_instr_count = lo_instr_count = 0;
#ifdef INTERPRET_MSUB
		genCallInterp(mips);
		return INTERPRETED;
#else
		return CONVERT_ERROR;
#endif
	case MIPS_FUNC_MSUBU:
		hi_instr_count = lo_instr_count = 0;
#ifdef INTERPRET_MSUBU
		genCallInterp(mips);
		return INTERPRETED;
#else
		return CONVERT_ERROR;
#endif
	case MIPS_FUNC_CLZ:
		PPC_SET_OPCODE(ppc, PPC_OPCODE_X);
		PPC_SET_FUNC  (ppc, PPC_FUNC_CNTLZW);
		PPC_SET_RD    (ppc, MIPS_GET_RS(mips));
		PPC_SET_RA    (ppc, MIPS_GET_RD(mips));
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
	case MIPS_FUNC_CLO:
#ifdef INTERPRET_CLO
		genCallInterp(mips);
		return INTERPRETED;
#else
		// neg
		PPC_SET_OPCODE(ppc, PPC_OPCODE_X);
		PPC_SET_FUNC  (ppc, PPC_FUNC_NEG);
		PPC_SET_RD    (ppc, MIPS_GET_RD(mips));
		PPC_SET_RA    (ppc, MIPS_GET_RS(mips));
		set_next_dst(ppc);
		// cntlzw
		ppc = NEW_PPC_INSTR();
		PPC_SET_OPCODE(ppc, PPC_OPCODE_X);
		PPC_SET_FUNC  (ppc, PPC_FUNC_CNTLZW);
		PPC_SET_RD    (ppc, MIPS_GET_RD(mips));
		PPC_SET_RA    (ppc, MIPS_GET_RD(mips));
		set_next_dst(ppc);
		return CONVERT_SUCCESS;
#endif
	
	default:
		return CONVERT_ERROR;
	}
}

// FIXME: The call clobbers r1, we can save r1 in r0
static void genCallInterp(MIPS_instr mips){
	PowerPC_instr ppc = NEW_PPC_INSTR();
	// Move the dst address to ctr
	unsigned int addr = (unsigned int)&decodeNInterpret;
	// lis r1, addr@ha(0)
	ppc = NEW_PPC_INSTR();
	PPC_SET_OPCODE(ppc, PPC_OPCODE_ADDIS);
	PPC_SET_RD    (ppc, 1);
	PPC_SET_IMMED (ppc, addr>>16);
	set_next_dst(ppc);
	// li r1, addr@l(r1)
	ppc = NEW_PPC_INSTR();
	PPC_SET_OPCODE(ppc, PPC_OPCODE_ORI);
	PPC_SET_RD    (ppc, 1);
	PPC_SET_RA    (ppc, 1);
	PPC_SET_IMMED (ppc, addr);
	set_next_dst(ppc);
	// mtctr r1
	ppc = NEW_PPC_INSTR();
	PPC_SET_OPCODE(ppc, PPC_OPCODE_X);
	PPC_SET_FUNC  (ppc, PPC_FUNC_MTSPR);
	PPC_SET_RD    (ppc, 1);
	PPC_SET_SPR   (ppc, 0x120);
	set_next_dst(ppc);
	// Then save the lr
	// mflr
	ppc = NEW_PPC_INSTR();
	PPC_SET_OPCODE(ppc, PPC_OPCODE_X);
	PPC_SET_FUNC  (ppc, PPC_FUNC_MFSPR);
	PPC_SET_RD    (ppc, MIPS_REG_LR);
	PPC_SET_SPR   (ppc, 0x100);
	set_next_dst(ppc);
	// Store the instruction
	// lis r1, mips@ha(0)
	ppc = NEW_PPC_INSTR();
	PPC_SET_OPCODE(ppc, PPC_OPCODE_ADDIS);
	PPC_SET_RD    (ppc, 1);
	PPC_SET_IMMED (ppc, mips>>16);
	set_next_dst(ppc);
	// li r1, mips@l(r1)
	ppc = NEW_PPC_INSTR();
	PPC_SET_OPCODE(ppc, PPC_OPCODE_ORI);
	PPC_SET_RD    (ppc, 1);
	PPC_SET_RA    (ppc, 1);
	PPC_SET_IMMED (ppc, mips);
	set_next_dst(ppc);
	// Call the interpreter
	// bctrl
	ppc = NEW_PPC_INSTR();
	PPC_SET_OPCODE(ppc, PPC_OPCODE_XL);
	PPC_SET_FUNC  (ppc, PPC_FUNC_BCCTR);
	PPC_SET_BO    (ppc, 0x14);
	PPC_SET_LK    (ppc, 1);
	set_next_dst(ppc);
	// Restore the lr
	// mtlr
	ppc = NEW_PPC_INSTR();
	PPC_SET_OPCODE(ppc, PPC_OPCODE_X);
	PPC_SET_FUNC  (ppc, PPC_FUNC_MTSPR);
	PPC_SET_RD    (ppc, MIPS_REG_LR);
	PPC_SET_SPR   (ppc, 0x100);
	set_next_dst(ppc);
}

static int mips_is_jump(MIPS_instr instr){
	int opcode = MIPS_GET_OPCODE(instr);
	int func   = MIPS_GET_RT    (instr);
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
               (opcode == MIPS_OPCODE_B     &&
                 (func == MIPS_RT_BLTZ      ||
                  func == MIPS_RT_BGEZ      ||
                  func == MIPS_RT_BLTZL     ||
                  func == MIPS_RT_BGEZL     ||
                  func == MIPS_RT_BLTZAL    ||
                  func == MIPS_RT_BLTZALL   ||
                  func == MIPS_RT_BGEZALL)));
}

