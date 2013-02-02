#include <ogc/machine/asm.h>

	.section	.text.dsi_exceptionhandler,"ax",@progbits
	.globl	dsi_exceptionhandler
dsi_exceptionhandler:
	stw		r11,GPR1_OFFSET(sp)
	stmw	r13,GPR13_OFFSET(sp)

	mfdsisr	r3
	mfdar	r4
	bl		vm_dsi_handler
	cmpwi	r3,0
	bne		1f

	mr		r3,sp
	b		c_default_exceptionhandler

1:	lwz		r0,CR_OFFSET(sp)
	mtcr	r0
	lwz		r0,LR_OFFSET(sp)
	mtlr	r0
	lwz		r0,CTR_OFFSET(sp)
	mtctr	r0
	lwz		r0,XER_OFFSET(sp)
	mtxer	r0
	lwz		r0,SRR0_OFFSET(sp)
	mtsrr0	r0
	lwz		r0,SRR1_OFFSET(sp)
	rlwinm	r0,r0,0,19,17
	mtsrr1	r0
	lwz		r12,GPR12_OFFSET(sp)
	lwz		r11,GPR11_OFFSET(sp)
	lwz		r10,GPR10_OFFSET(sp)
	lwz		r9,GPR9_OFFSET(sp)
	lwz		r8,GPR8_OFFSET(sp)
	lwz		r7,GPR7_OFFSET(sp)
	lwz		r6,GPR6_OFFSET(sp)
	lwz		r5,GPR5_OFFSET(sp)
	lwz		r4,GPR4_OFFSET(sp)
	lwz		r3,GPR3_OFFSET(sp)
	lwz		r2,GPR2_OFFSET(sp)
	lwz		r0,GPR0_OFFSET(sp)
	addi	sp,sp,EXCEPTION_FRAME_END
	rfi
