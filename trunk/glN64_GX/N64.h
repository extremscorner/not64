/**
 * glN64_GX - N64.h
 * Copyright (C) 2003 Orkin
 *
 * glN64 homepage: http://gln64.emulation64.com
 * Wii64 homepage: http://www.emulatemii.com
 *
**/

#ifndef N64_H
#define N64_H

#include "Types.h"

#define MI_INTR_DP		0x20		// Bit 5: DP intr 

struct N64Regs
{
	unsigned long *MI_INTR;

	unsigned long *DPC_START;
	unsigned long *DPC_END;
	unsigned long *DPC_CURRENT;
	unsigned long *DPC_STATUS;
	unsigned long *DPC_CLOCK;
	unsigned long *DPC_BUFBUSY;
	unsigned long *DPC_PIPEBUSY;
	unsigned long *DPC_TMEM;

	unsigned long *VI_STATUS;
	unsigned long *VI_ORIGIN;
	unsigned long *VI_WIDTH;
	unsigned long *VI_INTR;
	unsigned long *VI_V_CURRENT_LINE;
	unsigned long *VI_TIMING;
	unsigned long *VI_V_SYNC;
	unsigned long *VI_H_SYNC;
	unsigned long *VI_LEAP;
	unsigned long *VI_H_START;
	unsigned long *VI_V_START;
	unsigned long *VI_V_BURST;
	unsigned long *VI_X_SCALE;
	unsigned long *VI_Y_SCALE;
};

extern N64Regs REG;
extern u8 *DMEM;
extern u8 *IMEM;
extern u8 *RDRAM;
extern u64 TMEM[512];
extern u32 RDRAMSize;

#endif

