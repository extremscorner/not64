/* TLB-Cache.c - This is how the TLB LUT should be accessed, this way it won't waste RAM
   by Mike Slegeir for Mupen64-GC
 *****************************************************************************************
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


