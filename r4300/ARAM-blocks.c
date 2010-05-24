/**
 * Wii64 - ARAM-blocks.c
 * Copyright (C) 2007, 2008, 2009, 2010 emu_kidid
 * 
 * ARAM cache version of blocks array for gamecube
 *
 * Wii64 homepage: http://www.emulatemii.com
 * email address:  emukidid@gmail.com
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
#ifdef ARAM_BLOCKCACHE

#include <ogc/arqueue.h>
#include <gctypes.h>
#include <gccore.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include "ogc/lwp_watchdog.h"
#include "ppc/Recompile.h"
#include "ARAM-blocks.h"
#include "../gui/DEBUG.h"

static ARQRequest ARQ_request_blocks;

#define BLOCKS_CACHE_ADDR   0xC00000
#define NUM_CACHE_BLOCKS           8
#define CACHED_BLOCK_ENTRIES    1024
#define CACHED_BLOCK_SIZE       CACHED_BLOCK_ENTRIES * 4
PowerPC_block*  cached_block[NUM_CACHE_BLOCKS][CACHED_BLOCK_ENTRIES] __attribute__((aligned(32))); //last 4kb chunk of ptrs pulled
static u32      cached_dirty[NUM_CACHE_BLOCKS]     = {0,0,0,0,0,0,0,0};
static u32      cached_last_addr[NUM_CACHE_BLOCKS] = {0,0,0,0,0,0,0,0};
static u32      cached_block_lru[NUM_CACHE_BLOCKS] = {0,0,0,0,0,0,0,0};

void ARAM_ReadBlock(u32 addr, int block_num);
void ARAM_WriteBlock(u32 addr, int block_num);

PowerPC_block* blocks_get(u32 addr){
  int max_lru = cached_block_lru[0], max_lru_block = 0, i = 0, block_byte_address = ((addr - (addr % CACHED_BLOCK_ENTRIES)) * 4);
  
  for(i = 0; i < NUM_CACHE_BLOCKS; i++) {
    if(block_byte_address == cached_last_addr[i]) {  //if addr is in this block
      cached_block_lru[i] = gettick();
      return cached_block[i][(addr % CACHED_BLOCK_ENTRIES)];
    }
    if(max_lru > cached_block_lru[i]) {
      max_lru = cached_block_lru[i];
      max_lru_block = i;
    }
  }
  //we didn't find it, put it in the least recently used block
  if(cached_dirty[max_lru_block]) { //this chunk needs to be written back to ARAM
      ARAM_WriteBlock(cached_last_addr[max_lru_block], max_lru_block);
      cached_dirty[max_lru_block]=0;
    }
  ARAM_ReadBlock(block_byte_address, max_lru_block);         //read the block the addr we want lies in (aligned to 4kb)
  cached_last_addr[max_lru_block] = block_byte_address;      //the start of this block
  
  return cached_block[max_lru_block][(addr % CACHED_BLOCK_ENTRIES)];
}

void blocks_set(u32 addr, PowerPC_block* ptr){
  int max_lru = cached_block_lru[0], max_lru_block = 0, i = 0, block_byte_address = ((addr - (addr % CACHED_BLOCK_ENTRIES)) * 4);
  
  for(i = 0; i < NUM_CACHE_BLOCKS; i++) {
    if(block_byte_address == cached_last_addr[i]) {  //the block we want to write to is cached
      cached_block[i][(addr % CACHED_BLOCK_ENTRIES)] = ptr;   //set the value
      cached_dirty[i]=1;                               // this block, is dirty now
      cached_block_lru[i] = gettick();
      return;
    }
    if(max_lru > cached_block_lru[i]) {
      max_lru = cached_block_lru[i];
      max_lru_block = i;
    }
  }
  //we don't have it, we need to grab it from ARAM
  ARAM_WriteBlock(cached_last_addr[max_lru_block], max_lru_block);          //we need to put the current chunk back into ARAM and pull out another
  ARAM_ReadBlock(block_byte_address, max_lru_block);         //read the block the addr we want lies in (aligned to 4kb)
  cached_last_addr[max_lru_block] = block_byte_address;      //the start of this block
  cached_block[max_lru_block][(addr % CACHED_BLOCK_ENTRIES)] = ptr;   //set the value
  cached_dirty[max_lru_block]=1;                               // this block, although new, is dirty from now 
  cached_block_lru[max_lru_block] = 0;
}

//addr == addr of 4kb block of PowerPC_block ptrs to pull out from ARAM
void ARAM_ReadBlock(u32 addr, int block_num)
{
  ARQ_PostRequest(&ARQ_request_blocks, 0x2EAD, AR_ARAMTOMRAM, ARQ_PRIO_LO,
			                (int)(BLOCKS_CACHE_ADDR + addr), (int)&cached_block[block_num][0], CACHED_BLOCK_SIZE);
	DCInvalidateRange((void*)&cached_block[block_num][0], CACHED_BLOCK_SIZE);
}

//addr == addr of 4kb block of PowerPC_block ptrs to pull out from ARAM
void ARAM_WriteBlock(u32 addr, int block_num)
{
  DCFlushRange((void*)&cached_block[block_num][0], CACHED_BLOCK_SIZE);
	ARQ_PostRequest(&ARQ_request_blocks, 0x10AD, AR_MRAMTOARAM, ARQ_PRIO_HI,
			                (int)(BLOCKS_CACHE_ADDR + addr), (int)&cached_block[block_num][0], CACHED_BLOCK_SIZE);
}

#else //inlined wrapper for Wii

#include <gctypes.h>
#include "ppc/Recompile.h"

inline PowerPC_block* blocks_get(u32 addr){
  return blocks[addr];
}


inline void blocks_set(u32 addr, PowerPC_block* ptr){
  blocks[addr] = ptr;
}

#endif

