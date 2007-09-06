/* Wrappers.s - Wrappers for recompiled code, these start and return from N64 code
   by Mike Slegeir for Mupen64-GC
 ***********************************************************************************
   TODO: Make sure no important registers are clobbered
         Make sure the registers are saved properly
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

	.extern lr
	.type   lr, @object

	.extern lr_i
	.type   lr_i, @object

	.extern return_address
	.type   return_address, @object

	.extern prefetch_opcode
	.type   prefetch_opcode, @function

	.extern interp_ops
	.type   interp_ops, @function

/* void start(PowerPC_block*, unsigned int offset); */
	.align	2
	.globl	start
        .type   start, @function	
start: /* The block* is passed through r3 */
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
	
	/* DCFlush - We flush the block to main memory so that it
	             can be fetched by the instruction cache */
	lwz	9, 0(3)	/* lwz code(block) */
	lwz	8, 4(3)	/* lwz length(block) */
	srawi	8, 8, 4	/* length*(4 bytes/instr)/32 */ 
	addi	8, 8, 1 /* Make sure it runs once */
	mtctr	8
.DC_FLUSH_LOOP:
	dcbst	0, 9
	icbi	0, 9
	addi	9, 9, 32
	bdnz	.DC_FLUSH_LOOP
	
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
	
	PUSH_LR
	
	/* Restore emulator regs */
	lis	10, emu_reg@ha
	la	10, emu_reg@l(10)
	RESTORE_REGS 0,  9
	RESTORE_REGS 11, 31
	lwz	10, 40(10)
	
	/* TODO: Whatever comes next - For now: b return_address */
	lis	9, return_address@ha
	lwz	9, return_address@l(9)
	mtctr	9
	bctr
	
	.size return_from_code, .-return_from_code
	
	
	
/* void decodeNInterpret(void); // instr passed through r1 */	
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
	
	PUSH_LR
	
	/* Restore emulator regs */
	lis	10, emu_reg@ha
	la	10, emu_reg@l(10)
	RESTORE_REGS 0,  9
	RESTORE_REGS 11, 31
	
	/* Call functions */
	mfctr	3	/* The instr we want to decode */
	lis	10, prefetch_opcode@ha
	la	10, prefetch_opcode@l(10)
	mtctr	10
	bctrl		/* Call prefetch_opcode(instr) */
	/* We've probably lost our instr register by now, load it */
	lis	4, reg@ha
	la	4, reg@l(4)	/* It was saved in reg[1] */
	lwz	3, 0(4)	/* (int)reg[0] */
	andi.	0, 0, 0
	stw	0, 0(4) /* zero out reg[0], we don't want any confusion */
	lis	10, interp_ops@ha
	la	10, interp_ops@l(10)
	rlwinm	3, 3, 5, 27, 31	/* MIPS_GET_OPCODE(instr) */
	rlwinm	3, 3, 2, 25, 29	/* opcode * sizeof(func_ptr) */
	add	10, 10, 3
	lwz	10, 0(10)	/* Dereference function pointer */
	mtctr	10
	bctrl		/* Call interp_ops[opcode]() */
	
	/* Restore state */
	POP_LR
	
	lis	10, emu_reg@ha
	la	10, emu_reg@l(10)
	SAVE_REGS 0, 31
	
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

