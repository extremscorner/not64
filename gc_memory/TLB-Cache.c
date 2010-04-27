/**
 * Wii64 - TLB-Cache.c (Deprecated)
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

//TLB LUT W
unsigned long   tlb_w_block[CACHED_TLB_ENTRIES] __attribute__((aligned(32))); //last 4kb chunk of ptrs pulled
static u32      tlb_w_dirty     = 0;
static u32      tlb_w_last_addr = 0;

//TLB LUT R
unsigned long   tlb_r_block[CACHED_TLB_ENTRIES] __attribute__((aligned(32))); //last 4kb chunk of ptrs pulled
static u32      tlb_r_dirty     = 0;
static u32      tlb_r_last_addr = 0;


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

unsigned int TLBCache_get_r(unsigned int page){
  int block_byte_address = ((page - (page % CACHED_TLB_ENTRIES)) * 4);
  
  if(block_byte_address != tlb_r_last_addr) {       //if addr isn't in the last block
    if(tlb_r_dirty) {                               //current chunk needs to be written back to ARAM
      ARAM_WriteTLBBlock(tlb_r_last_addr, TLB_R_TYPE);
      tlb_r_dirty=0;
    }
    ARAM_ReadTLBBlock(block_byte_address, TLB_R_TYPE); //read the block the addr we want lies in (aligned to 4kb)
    tlb_r_last_addr = block_byte_address;           //the start of this block
  }
  return tlb_r_block[(page % CACHED_TLB_ENTRIES)];
}

unsigned int TLBCache_get_w(unsigned int page){
  int block_byte_address = ((page - (page % CACHED_TLB_ENTRIES)) * 4);
  
  if(block_byte_address != tlb_w_last_addr) {       //if addr isn't in the last block
    if(tlb_w_dirty) {                               //current chunk needs to be written back to ARAM
      ARAM_WriteTLBBlock(tlb_w_last_addr, TLB_W_TYPE);
      tlb_w_dirty=0;
    }
    ARAM_ReadTLBBlock(block_byte_address, TLB_W_TYPE); //read the block the addr we want lies in (aligned to 4kb)
    tlb_w_last_addr = block_byte_address;           //the start of this block
  }
  return tlb_w_block[(page % CACHED_TLB_ENTRIES)];
}

void TLBCache_set_r(unsigned int page, unsigned int val){
  int block_byte_address = ((page - (page % CACHED_TLB_ENTRIES)) * 4);
  
  if(block_byte_address != tlb_r_last_addr) {       //the block we want to write to is not cached
    ARAM_WriteTLBBlock(tlb_r_last_addr, TLB_R_TYPE);   //we need to put the current chunk back into ARAM and pull out another
    ARAM_ReadTLBBlock(block_byte_address, TLB_R_TYPE); //read the block the addr we want lies in (aligned to 4kb)
    tlb_r_last_addr = block_byte_address;           //the start of this block
  }
  tlb_r_block[(page % CACHED_TLB_ENTRIES)] = val;   //set the value
  tlb_r_dirty=1;                                    // this block, although new, is dirty from now
}

void TLBCache_set_w(unsigned int page, unsigned int val){
  int block_byte_address = ((page - (page % CACHED_TLB_ENTRIES)) * 4);
  
  if(block_byte_address != tlb_w_last_addr) {       //the block we want to write to is not cached
    ARAM_WriteTLBBlock(tlb_w_last_addr, TLB_W_TYPE);   //we need to put the current chunk back into ARAM and pull out another
    ARAM_ReadTLBBlock(block_byte_address, TLB_W_TYPE); //read the block the addr we want lies in (aligned to 4kb)
    tlb_w_last_addr = block_byte_address;           //the start of this block
  }
  tlb_w_block[(page % CACHED_TLB_ENTRIES)] = val;   //set the value
  tlb_w_dirty=1;                                    // this block, although new, is dirty from now
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
  int dest_addr = (type == TLB_W_TYPE) ? (int)&tlb_w_block[0] : (int)&tlb_r_block[0];
  ARQ_PostRequest(&ARQ_request_TLB, 0x2EAD, AR_ARAMTOMRAM, ARQ_PRIO_LO,
			                (int)(base_addr + addr), dest_addr, CACHED_TLB_SIZE);
	DCInvalidateRange((void*)dest_addr, CACHED_TLB_SIZE);
}

//addr == addr of 4kb block of PowerPC_block ptrs to pull out from ARAM
void ARAM_WriteTLBBlock(u32 addr, int type)
{
  int base_addr = (type == TLB_W_TYPE) ? TLB_W_CACHE_ADDR : TLB_R_CACHE_ADDR;
  int dest_addr = (type == TLB_W_TYPE) ? (int)&tlb_w_block[0] : (int)&tlb_r_block[0];
  DCFlushRange((void*)dest_addr, CACHED_TLB_SIZE);
	ARQ_PostRequest(&ARQ_request_TLB, 0x10AD, AR_MRAMTOARAM, ARQ_PRIO_HI,
			                (int)(base_addr + addr), dest_addr, CACHED_TLB_SIZE);
}

#endif


