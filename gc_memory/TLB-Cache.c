/**
 * Wii64 - TLB-Cache.c (Deprecated)
 * Copyright (C) 2007, 2008, 2009 Mike Slegeir
 * 
 * This is how the TLB LUT should be accessed, this way it won't waste RAM
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

/* 
   FIXME: The DMA transfers seem to overflow small buffers
*/

#include <ogc/arqueue.h>
#include "ARAM.h"
#include "TLB-Cache.h"

static int* TLB_LUT_r, * TLB_LUT_w;
// We must moved 32-byte aligned, 32 byte chunks of the TLB
static unsigned int value[8] __attribute__((aligned(32)));
static ARQRequest ARQ_request;

void TLBCache_init(void){
	ARQ_Init();
	// FIXME: I'm going to assume that nothings in the ARAM yet
	ARAM_block_alloc_contiguous(&TLB_LUT_r, 'T', 4);
	ARAM_block_alloc_contiguous(&TLB_LUT_w, 'T', 4);
	
	// Zero out the LUTs
	int chunkSize = ARQ_GetChunkSize(), offset = 0;
	char* zeroes = memalign(32, chunkSize);
	memset(zeroes, 0, chunkSize);
	DCFlushRange(zeroes, chunkSize);
	while(offset < 4 * 1024 * 1024){
		ARQ_PostRequest(&ARQ_request, 0x0, AR_MRAMTOARAM, ARQ_PRIO_HI,
		                TLB_LUT_r + offset, zeroes, chunkSize);
		ARQ_PostRequest(&ARQ_request, 0x0, AR_MRAMTOARAM, ARQ_PRIO_HI,
		                TLB_LUT_w + offset, zeroes, chunkSize);
		offset += chunkSize;
	}
	free(zeroes);
}

void TLBCache_deinit(void){
	ARAM_block_free_contiguous(&TLB_LUT_r, 4);
	ARAM_block_free_contiguous(&TLB_LUT_w, 4);
}

unsigned int inline TLBCache_get_r(unsigned int page){
	//printf("TLBCache_get_r(%08x)\n", page);
	DCInvalidateRange(value, 32);
	ARQ_PostRequest(&ARQ_request, 0x718, AR_ARAMTOMRAM, ARQ_PRIO_LO,
	                TLB_LUT_r + (page&(~0x7)), value, 32);
	return value[page&0x7];
}

unsigned int inline TLBCache_get_w(unsigned int page){
	//printf("TLBCache_get_w(%08x)\n", page);
	DCInvalidateRange(value, 32);
	ARQ_PostRequest(&ARQ_request, 0x718, AR_ARAMTOMRAM, ARQ_PRIO_LO,
	                TLB_LUT_w + (page&(~0x7)), value, 32);
	return value[page&0x7];
}

void inline TLBCache_set_r(unsigned int page, unsigned int val){
	//printf("TLBCache_set_r(%08x, %08x)\n", page, val);
	DCInvalidateRange(value, 32);
	ARQ_PostRequest(&ARQ_request, 0x718, AR_ARAMTOMRAM, ARQ_PRIO_LO,
	                TLB_LUT_r + (page&(~0x7)), value, 32);
	value[page&0x7] = val;
	DCFlushRange(value, 32);
	ARQ_PostRequest(&ARQ_request, 0x718, AR_MRAMTOARAM, ARQ_PRIO_LO,
	                TLB_LUT_r + (page&(~0x7)), value, 32);
}

void inline TLBCache_set_w(unsigned int page, unsigned int val){
	//printf("TLBCache_set_w(%08x, %08x)\n", page, val);
	DCInvalidateRange(value, 32);
	ARQ_PostRequest(&ARQ_request, 0x718, AR_ARAMTOMRAM, ARQ_PRIO_LO,
	                TLB_LUT_w + (page&(~0x7)), value, 32);
	value[page&0x7] = val;
	DCFlushRange(value, 32);
	ARQ_PostRequest(&ARQ_request, 0x718, AR_MRAMTOARAM, ARQ_PRIO_LO,
	                TLB_LUT_w + (page&(~0x7)), value, 32);
}


