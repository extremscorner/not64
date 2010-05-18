/**
 * Wii64 - TLB-Cache.c
 * Copyright (C) 2007, 2008, 2009 emu_kidid
 * Copyright (C) 2007, 2008, 2009 Mike Slegeir
 * 
 * This is how the TLB LUT should be accessed, this way it won't waste RAM, but ARAM
 *
 * Wii64 homepage: http://www.emulatemii.com
 * email address: tehpola@gmail.com
 * email address: emukidid@gmail.com
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

#ifdef ARAM_TLBCACHE

#include <ogc/arqueue.h>
#include <gccore.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include "ARAM.h"
#include "TLB-Cache.h"
#include "../gui/DEBUG.h"

static ARQRequest ARQ_request_TLB;

#define TLB_W_TYPE                                0
#define TLB_R_TYPE                                1
#define TLB_W_CACHE_ADDR                   0x400000
#define TLB_R_CACHE_ADDR                   0x800000
#define CACHED_TLB_ENTRIES                     1024
#define CACHED_TLB_SIZE      CACHED_TLB_ENTRIES * 4

unsigned long tlb_block[2][CACHED_TLB_ENTRIES] __attribute__((aligned(32)));
static u32    tlb_dirty[2]     = { 0, 0 };
static u32    tlb_last_addr[2] = { 0, 0 };


void TLBCache_init(void){
	int i = 0;
	for(i=0;i<0x100000;i++) {
  	TLBCache_set_w(i,0);
  	TLBCache_set_r(i,0);
	}
}

void TLBCache_deinit(void){
	TLBCache_init();
}

static inline int calc_block_addr(unsigned int page){
  return ((page - (page % CACHED_TLB_ENTRIES)) * 4);
}

static inline int calc_index(unsigned int page){
  return page % CACHED_TLB_ENTRIES;
}

static inline void ensure_fetched(int type, int block_addr){
  if(block_addr != tlb_last_addr[type]){
    if(tlb_dirty[type])
      ARAM_WriteTLBBlock(tlb_last_addr[type], type);

    ARAM_ReadTLBBlock(block_addr, type);
    tlb_last_addr[type] = block_addr;
    tlb_dirty[type] = 0;
  }
}

static unsigned int TLBCache_get(int type, unsigned int page){
  ensure_fetched(type, calc_block_addr(page));
  
  return tlb_block[type][calc_index(page)];
}

unsigned int TLBCache_get_r(unsigned int page){
  return TLBCache_get(TLB_R_TYPE, page);
}

unsigned int TLBCache_get_w(unsigned int page){
  return TLBCache_get(TLB_W_TYPE, page);
}

static inline void TLBCache_set(int type, unsigned int page, unsigned int val){
  ensure_fetched(type, calc_block_addr(page));
  
  tlb_block[type][calc_index(page)] = val;
  tlb_dirty[type] = 1;
}

void TLBCache_set_r(unsigned int page, unsigned int val){
  TLBCache_set(TLB_R_TYPE, page, val);
}

void TLBCache_set_w(unsigned int page, unsigned int val){
  TLBCache_set(TLB_W_TYPE, page, val);
}

void TLBCache_dump_w(gzFile *f) {
  int i = 0;
  for(i=0;i<0x100000;i++) {
    unsigned long val = TLBCache_get_w(i);
  	gzwrite(f, &val, sizeof(unsigned long));
	}
}

void TLBCache_dump_r(gzFile *f) {
  int i = 0;
  for(i=0;i<0x100000;i++) {
    unsigned long val = TLBCache_get_r(i);
  	gzwrite(f, &val, sizeof(unsigned long));
	}
}

//addr == addr of 4kb block of PowerPC_block ptrs to pull out from ARAM
void ARAM_ReadTLBBlock(u32 addr, int type)
{
  int base_addr = (type == TLB_W_TYPE) ? TLB_W_CACHE_ADDR : TLB_R_CACHE_ADDR;
  int dest_addr = (type == TLB_W_TYPE) ? (int)&tlb_block[TLB_W_TYPE] : (int)&tlb_block[TLB_R_TYPE];
  ARQ_PostRequest(&ARQ_request_TLB, 0x2EAD, AR_ARAMTOMRAM, ARQ_PRIO_LO,
			                (int)(base_addr + addr), dest_addr, CACHED_TLB_SIZE);
	DCInvalidateRange((void*)dest_addr, CACHED_TLB_SIZE);
}

//addr == addr of 4kb block of PowerPC_block ptrs to pull out from ARAM
void ARAM_WriteTLBBlock(u32 addr, int type)
{
  int base_addr = (type == TLB_W_TYPE) ? TLB_W_CACHE_ADDR : TLB_R_CACHE_ADDR;
  int dest_addr = (type == TLB_W_TYPE) ? (int)&tlb_block[TLB_W_TYPE] : (int)&tlb_block[TLB_R_TYPE];
  DCFlushRange((void*)dest_addr, CACHED_TLB_SIZE);
	ARQ_PostRequest(&ARQ_request_TLB, 0x10AD, AR_MRAMTOARAM, ARQ_PRIO_HI,
			                (int)(base_addr + addr), dest_addr, CACHED_TLB_SIZE);
}

#endif


