/* Wrappers.s - Wrappers for recompiled code, these start and return from N64 code
   by Mike Slegeir for Mupen64-GC
 ***********************************************************************************
   TODO: Make sure no important registers are clobbered
         Make sure the registers are saved properly
		 Save emu_reg where the exception handler would if possible
 */

	.file	"Wrappers.s"
	.text

/* The save/restore macros assume r10 contains
   the base address of the array to save/restore */
.macro SAVE_REGS from=1 to=31 size=4
        stw	\from, (\from*\size)(10)
        .if	\to-\from
        	SAVE_REGS	"(\from+1)",\to,\size
        .endif
.endm

.macro RESTORE_REGS from=1 to=31 size=4
        lwz	\from, (\from*\size)(10)
        .if	\to-\from
        	RESTORE_REGS	"(\from+1)",\to,\size
        .endif
.endm

.macro SAVE_FPR from=1 to=31 size=8
	stfd	\from, (\from*\size)(10)
	.if	\to-\from
		SAVE_FPR	"(\from+1)",\to,\size
	.endif
.endm

.macro RESTORE_FPR from=1 to=31 size=8
	lfd	\from, (\from*\size)(10)
	.if	\to-\from
		SAVE_FPR	"(\from+1)",\to,\size
	.endif
.endm

.macro	PUSH_LR
	mflr	0
	lis	9, lr_i@ha
	lwz	8, lr_i@l(9)
	lis	7, lr@ha
	la	7, lr@l(7)
	mulli	10, 8, 4
	add	7, 7, 10
	stw	0, 0(7)
	addi	8, 8, 1
	stw	8, lr_i@l(9)
.endm

.macro	POP_LR
	lis	9, lr_i@ha
	lwz	8, lr_i@l(9)
	lis	10, lr@ha
	la	10, lr@l(10)
	addi	8, 8, -1
	stw	8, lr_i@l(9)
	mulli	8, 8, 4
	add	10, 10, 8
	lwz	10, 0(10)
	mtlr	10
.endm

	.extern emu_reg
	.type   emu_reg, @object

	.extern reg
	.type   reg, @object
	
	.extern emu_fpr
	.type	emu_fpr, @object
	
	.extern reg_cop1_fgr_64
	.type	reg_cop1_fgr_64, @object
	
	.extern fpu_in_use
	.type	fpu_in_use, @object

	.extern lr
	.type   lr, @object

	.extern lr_i
	.type   lr_i, @object
	
	.extern instructionCount
	.type   instructionCount, @object

	.extern return_address
	.type   return_address, @object

	.extern prefetch_opcode
	.type   prefetch_opcode, @function

	.extern interp_ops
	.type   interp_ops, @function
	
	.globl	_fp_handler_old
	.type	_fp_handler_old, @object
	_fp_handler_old:
		.long 0

/* void start(PowerPC_block*, unsigned int offset); */
	.align	2
	.globl	start
        .type   start, @function	
start: /* The block* is passed through r3 */
	/* Pop the stack frame */
	lwz	1, 0(1)
	/* Save emu_regs */
	mtctr	10
	lis	10, emu_reg@ha
	la	10, emu_reg@l(10)
	SAVE_REGS 0,  9
	SAVE_REGS 11, 31
	mfctr	9
	stw	9, 40(10)
	
	/* if (lr_i){ */
	lis	9, lr_i@ha
	lwz	8, lr_i@l(9)
	cmpwi	0, 8, 0
	beq	0, .END_IF_LR_I
	
	POP_LR
	/* } */
.END_IF_LR_I:
	
.if 0 /* We now only flush a new block */
	/* DCFlush - We flush the block to main memory so that it
	             can be fetched by the instruction cache */
	lwz	9, 0(3)	/* lwz code(block) */
	lwz	8, 4(3)	/* lwz length(block) */
	srawi	8, 8, 3	/* length*(4 bytes/instr)/32 */ 
	addi	8, 8, 1 /* Make sure it runs once */
	mtctr	8
.DC_FLUSH_LOOP:
	dcbst	0, 9
	icbi	0, 9
	addi	9, 9, 32
	bdnz	.DC_FLUSH_LOOP
.endif
	
	/* Make sure the instruction counter is running */
	mfmmcr0	9
	ori	9, 9, 2
	mtmmcr0	9
	/* Store the start value for the instruction counter */
	mfpmc2	9
	lis	10, instructionCount@ha
	stw	9, instructionCount@l(10)
	
	/* Disable external interrupts in game code */
	mfmsr	9
	andi.	9, 9, 0x7FFF
	mtmsr	9
	
	/* if(MSR[FP]) */
	rlwinm	10, 9, 5, 0x1
	cmpwi	0, 10, 0
	beq	0, .FP_NOT_IN_USE_S
	/* Save emu_fp */
	lis	10, emu_fpr@ha
	la	10, emu_fpr@l(10)
	SAVE_FPR
	/* Disable FP */
	xori	9, 9, 0x0800
	mtmsr	9
	/* TODO: Note that we should restore emu_fp on return */
	
.FP_NOT_IN_USE_S:
	/* Install our exception handler */
	lis	10, 0x80003000@ha
	la	10, 0x80003000@l(10)
	lwz	9, 7*4(10)
	lis	8, _fp_handler_old@ha
	stw	9, _fp_handler_old@l(8)
	lis	9, _fp_handler@ha
	la	9, _fp_handler@l(9)
	stw	9, 7*4(10)
	
	/* Load address to begin execution */
	lwz	0, 0(3)	/* lwz code(block) */
	mulli	4, 4, 4	/* offset *= sizeof(PowerPC_instr) */
	add	0, 0, 4	/* r0 += offset    */
	mtctr	0
	
	/* Restore game regs */
	lis	10, reg@ha
	la	10, reg@l(10)
	addi    10, 10, 4	/* We increment the reg pointer so
	    	         	   we load the low part of the longs */
	RESTORE_REGS 1,  9, 8
	RESTORE_REGS 11, 31, 8
	lwz	10, 80(10)
	
	andi.	0, 0, 0
	sync
	isync
	bctr
	.size	start, .-start



/* void return_from_code(void); */
	.align	2
	.globl	return_from_code
        .type   return_from_code, @function	
return_from_code:
	/* Save game regs */
	mtctr	10
	lis	10, reg@ha
	la	10, reg@l(10)
	addi	10, 10, 4
	SAVE_REGS 0,  9, 8
	SAVE_REGS 11, 31, 8
	mfctr	9
	stw	9, 80(10)
	
.if 0
	/* TODO: Replace fpu_in_use code with restoring exception handler */
	/* if(fpu_in_use) */
	lis	10, fpu_in_use@ha
	la	10, fpu_in_use@l(10)
	lwz	0, 0(10)
	cmpi	0, 0, 0
	beq	0, .END_IF_FPU2
	/* Save game regs */
	lis	10, reg_cop1_fgr_64@ha
	la	10, reg_cop1_fgr_64@l(10)
	SAVE_FPR
	/* Restore emulator regs */
	lis	10, emu_fpr@ha
	la	10, emu_fpr@l(10)
	RESTORE_FPR
	/* fpu_in_use = 0 */
	la	0, 1(0)
	stw	0, 0(10)
.END_IF_FPU2:
.endif
	
	/* if(MSR[FP]) */
	mfmsr	9
	rlwinm	10, 9, 5, 0x1
	cmpwi	0, 10, 0
	beq	0, .FP_NOT_IN_USE_R
	/* Save N64 FPRs */
	lis	10, reg_cop1_fgr_64@ha
	la	10, reg_cop1_fgr_64@l(10)
	SAVE_FPR
	/* Disable FP */
	xori	9, 9, 0x0800
	mtmsr	9
	/* TODO: Note that we should restore emu_fp on return */
	
.FP_NOT_IN_USE_R:
	/* Restore the exception handler */
	lis	10, 0x80003000@ha
	la	10, 0x80003000@l(10)
	lis	9, _fp_handler_old@ha
	lwz	9, _fp_handler_old@l(9)
	stw	9, 7*4(10)
	
	PUSH_LR
	
	/* Read the instruction counter and store the difference */
	mfpmc2	9
	lis	10, instructionCount@ha
	lwz	8, instructionCount@l(10)
	subf	8, 8, 9
	stw	8, instructionCount@l(10)
	
	/* Restore emulator regs */
	lis	10, emu_reg@ha
	la	10, emu_reg@l(10)
	RESTORE_REGS 0,  9
	RESTORE_REGS 11, 31
	lwz	10, 40(10)
	
	/* Enable external interrupts in emulator code */
	mfmsr	9
	ori	9, 9, 0x8000
	mtmsr	9
	
	/* TODO: Whatever comes next - For now: b return_address */
	lis	9, return_address@ha
	lwz	9, return_address@l(9)
	mtctr	9
	bctr
	
	.size return_from_code, .-return_from_code
	
	
	
/* void decodeNInterpret(); // instr passed through r0 */	
	.align	2
	.globl	decodeNInterpret
        .type   decodeNInterpret, @function	
decodeNInterpret:
	/* Save game regs - the instruction is passed through r0 */
	mtctr	10
	lis	10, reg@ha
	la	10, reg@l(10)
	addi    10, 10, 4
	SAVE_REGS 0,  9, 8
	SAVE_REGS 11, 31, 8
	mfctr	9
	stw	9, 80(10)
	mtctr	0	/* Hold the instr in ctr for now */
	
.if 0
	/* TODO: Replace this code with installing our own exception handler */
	/* if(fpu_in_use) */
	lis	10, fpu_in_use@ha
	la	10, fpu_in_use@l(10)
	lwz	0, 0(10)
	cmpi	0, 0, 0
	beq	0, .END_IF_FPU
	/* Save game regs */
	lis	10, reg_cop1_fgr_64@ha
	la	10, reg_cop1_fgr_64@l(10)
	SAVE_FPR
	/* Restore emulator regs */
	lis	10, emu_fpr@ha
	la	10, emu_fpr@l(10)
	RESTORE_FPR
	/* fpu_in_use = 0 */
	la	0, 1(0)
	stw	0, 0(10)
.END_IF_FPU:
.endif
	
	PUSH_LR
	
	/* Read the instruction counter and store the difference */
	mfpmc2	9
	lis	10, instructionCount@ha
	lwz	8, instructionCount@l(10)
	subf	8, 8, 9
	stw	8, instructionCount@l(10)
	
	/* if(MSR[FP]) */
	mfmsr	9
	rlwinm	10, 9, 5, 0x1
	cmpwi	0, 10, 0
	beq	0, .FP_NOT_IN_USE_D1
	/* Save N64 FPRs */
	lis	10, reg_cop1_fgr_64@ha
	la	10, reg_cop1_fgr_64@l(10)
	SAVE_FPR
	/* Disable FP */
	xori	9, 9, 0x0800
	mtmsr	9
	/* TODO: Note that we should restore emu_fp on return */
	
.FP_NOT_IN_USE_D1:
	/* Restore the exception handler */
	lis	10, 0x80003000@ha
	la	10, 0x80003000@l(10)
	lis	9, _fp_handler_old@ha
	lwz	9, _fp_handler_old@l(9)
	stw	9, 7*4(10)
	
	/* Restore emulator regs */
	lis	10, emu_reg@ha
	la	10, emu_reg@l(10)
	RESTORE_REGS 0,  9
	RESTORE_REGS 11, 31
	
	/* Enable external interrupts in emulator code */
	mfmsr	3
	ori	3, 3, 0x8000
	mtmsr	3
	
	/* Call functions */
	mfctr	3	/* The instr we want to decode */
	lis	10, prefetch_opcode@ha
	la	10, prefetch_opcode@l(10)
	mtctr	10
	bctrl		/* Call prefetch_opcode(instr) */
	/* We've probably lost our instr register by now, load it */
	lis	4, reg@ha
	la	4, reg@l(4)	/* It was saved in reg[0] */
	lwz	3, 4(4)		/* (int)reg[0] = reg + 4 bytes for low */
	andi.	0, 0, 0
	stw	0, 4(4)		/* zero out reg[0], we don't want any confusion */
	lis	10, interp_ops@ha
	la	10, interp_ops@l(10)
	rlwinm	3, 3, 6, 26, 31	/* MIPS_GET_OPCODE(instr) */
	rlwinm	3, 3, 2, 24, 29	/* opcode * sizeof(func_ptr) */
	add	10, 10, 3
	lwz	10, 0(10)	/* Dereference function pointer */
	mtctr	10
	bctrl		/* Call interp_ops[opcode]() */
	
	/* Save emulator state */
	POP_LR
	
	lis	10, emu_reg@ha
	la	10, emu_reg@l(10)
	SAVE_REGS 0, 31
	
	/* Store the start value for the instruction counter */
	mfpmc2	9
	lis	10, instructionCount@ha
	stw	9, instructionCount@l(10)
	
	/* Disable external interrupts in game code */
	mfmsr	9
	andi.	9, 9, 0x7FFF
	mtmsr	9
	
	/* if(MSR[FP]) */
	rlwinm	10, 9, 5, 0x1
	cmpwi	0, 10, 0
	beq	0, .FP_NOT_IN_USE_D2
	/* Save emu_fp */
	lis	10, emu_fpr@ha
	la	10, emu_fpr@l(10)
	SAVE_FPR
	/* Disable FP */
	xori	9, 9, 0x0800
	mtmsr	9
	/* TODO: Note that we should restore emu_fp on return */
	
.FP_NOT_IN_USE_D2:
	/* Install our exception handler */
	lis	10, 0x80003000@ha
	la	10, 0x80003000@l(10)
	lwz	9, 7*4(10)
	lis	8, _fp_handler_old@ha
	stw	9, _fp_handler_old@l(8)
	lis	9, _fp_handler@ha
	la	9, _fp_handler@l(9)
	stw	9, 7*4(10)
	
	/* Restore game state */
	lis	10, reg@ha
	la	10, reg@l(10)
	addi	10, 10, 4
	RESTORE_REGS 1,  9, 8
	RESTORE_REGS 11, 31, 8
	lwz	10, 80(10)
	andi.	0, 0, 0
	
	/* Return */
	blr
	
	.size decodeNInterpret, .-decodeNInterpret


/* void fp_restore(void); // Restore's N64 FPRs and sets fpu_in_use */	
	.align	2
	.globl	fp_restore
        .type   fp_restore, @function	
fp_restore:
	mtctr	10
	/* Save emulator FPRs */
	lis	10, emu_fpr@ha
	la	10, emu_fpr@l(10)
	SAVE_FPR
	/* Load N64 FPRs */
	lis	10, reg_cop1_fgr_64@ha
	la	10, reg_cop1_fgr_64@l(10)
	RESTORE_FPR
	/* Set fpu_in_use to 1 */
	lis	10, fpu_in_use@ha
	la	10, fpu_in_use@l(10)
	la	0, 1(0)
	stw	0, 0(10)
	
	andi.	0, 0, 0
	mfctr	10
	blr
	
	.size fp_restore, .-fp_restore


/* _fp_handler: exception handler for restoring FP regs */
	.align	2
	.globl	_fp_handler
        .type   _fp_handler, @function	
_fp_handler:
	mtsprg4	10
	
	/* Enable FP */
	mfsrr1	10
	ori	10, 10, 0x0800
	mtsrr1	10
	/* Load N64 FPRs */
	lis	10, reg_cop1_fgr_64@ha
	la	10, reg_cop1_fgr_64@l(10)
	RESTORE_FPR
	
	mfsprg4	10
	rfi
	
	.size _fp_handler, .-_fp_handler
	


