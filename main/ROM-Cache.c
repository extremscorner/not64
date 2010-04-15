/**
 * Wii64 - ROM-Cache.c (Gamecube ROM Cache)
 * Copyright (C) 2007, 2008, 2009 Mike Slegeir
 * Copyright (C) 2007, 2008, 2009 emu_kidid
 * 
 * This is how the ROM should be accessed, this way the ROM doesn't waste RAM
 *
 * Wii64 homepage: http://www.emulatemii.com
 * email address: tehpola@gmail.com
 *                emukidid@gmail.com
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


#include <ogc/arqueue.h>
#include <gccore.h>
#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include "../fileBrowser/fileBrowser.h"
#include "../gui/gui_GX-menu.h"
#include "../gc_memory/ARAM.h"
#include "../r4300/r4300.h"
#include "../gui/DEBUG.h"
#include "../gui/GUI.h"
#include "ROM-Cache.h"
#include "rom.h"

void DUMMY_print(char* string) { }
void DUMMY_setLoadProg(float percent) { }
void DUMMY_draw() { }

#ifdef MENU_V2
void LoadingBar_showBar(float percent, const char* string);
#define PRINT DUMMY_print
#define SETLOADPROG DUMMY_setLoadProg
#define DRAWGUI DUMMY_draw
#else
#define PRINT GUI_print
#define SETLOADPROG GUI_setLoadProg
#define DRAWGUI GUI_draw
#endif

#define BLOCK_MASK  (BLOCK_SIZE-1)
#define OFFSET_MASK (0xFFFFFFFF-BLOCK_MASK)
#define BLOCK_SHIFT (18)	//only change ME and BLOCK_SIZE in gc_memory/aram.h
#define MAX_ROMSIZE (64*1024*1024)
#define NUM_BLOCKS  (MAX_ROMSIZE/BLOCK_SIZE)

static char ROM_too_big;
static char* ROM, * ROM_blocks[NUM_BLOCKS];
static u32 ROM_size;
static fileBrowser_file* ROM_file;
static char readBefore = 0;

#ifdef USE_ROM_CACHE_L1
#define L1_BLOCK_SIZE  (4*1024)
#define L1_BLOCK_MASK  (0xFFF)
#define L1_BLOCK_SHIFT (12)
#define L1_NUM_BLOCKS  (8)
static u8  L1[L1_NUM_BLOCKS][L1_BLOCK_SIZE];
static int L1tag[L1_NUM_BLOCKS];
static u32 L1LRU[L1_NUM_BLOCKS];
static u32 nextL1LRUValue;
#endif

static ARQRequest ARQ_request;
extern void showLoadProgress(float progress);
extern void pauseAudio(void);
extern void resumeAudio(void);
extern BOOL hasLoadedROM;

void ROMCache_init(fileBrowser_file* file){
  readBefore = 0; //de-init byteswapping
	ARQ_Reset();
	ARQ_Init();
	ROM_too_big = (file->size) > (ARAM_block_available_contiguous() * BLOCK_SIZE);
	ROM_size = (file->size);
#ifdef USE_ROM_CACHE_L1
	nextL1LRUValue = 0;
	int i;
	for(i=0; i<L1_NUM_BLOCKS; ++i) L1tag[i] = -1;
#endif
	
	//romFile_init( romFile_topLevel );
}

void ROMCache_deinit(){
	if(ROM_too_big){
		int i;
		for(i=0; i<NUM_BLOCKS; ++i)
			if(ROM_blocks[i])
				ARAM_block_free(&ROM_blocks[i]);
	} else
		if(ROM) ARAM_block_free_contiguous(&ROM, ROM_size / BLOCK_SIZE);
	//we don't de-init the romFile here because it takes too much time to fopen/fseek/fread/fclose
}

static void inline ROMCache_load_block(char* block, int rom_offset){
  if((hasLoadedROM) && (!stop))
    pauseAudio();
	showLoadProgress(1.0f);
	romFile_seekFile(ROM_file, rom_offset, FILE_BROWSER_SEEK_SET);
	int bytes_read, offset=0, bytes_to_read=ARQ_GetChunkSize();
	char* buffer = memalign(32, bytes_to_read);
	int loads_til_update = 0;
	do {
		bytes_read = romFile_readFile(ROM_file, buffer, bytes_to_read);
		byte_swap(buffer, bytes_read);
		DCFlushRange(buffer, bytes_read);
		ARQ_PostRequest(&ARQ_request, 0x10AD, ARQ_MRAMTOARAM, ARQ_PRIO_HI,
		                block + offset, buffer, bytes_read);
		offset += bytes_read;
		
		if(!loads_til_update--){
//			showLoadProgress( (float)offset/BLOCK_SIZE );
			loads_til_update = 16;
		}
		
	} while(offset != BLOCK_SIZE && bytes_read == bytes_to_read);
	free(buffer);
//	showLoadProgress(1.0f);
  if((hasLoadedROM) && (!stop))
    resumeAudio();
}


//handles all alignment
void ARAM_ReadFromBlock(char *block,int startOffset, int bytes, char *dest)
{
  int originalStartOffset = startOffset;
  int originalBytes = bytes;
  
  if(startOffset%32 !=0)  //misaligned startoffset
  {
    startOffset -= startOffset % 32;
    bytes+=(originalStartOffset%32);  //adjust for the extra startOffset now
  }
  
  if(bytes%32 !=0)  //adjust again if misaligned size
    bytes += 32 - (bytes%32);
    
  char* buffer = memalign(32,bytes);
  
  ARQ_PostRequest(&ARQ_request, 0x2EAD, AR_ARAMTOMRAM, ARQ_PRIO_LO,
			                block + startOffset, buffer, bytes);
	DCInvalidateRange(buffer, bytes);
	memcpy(dest, buffer+(originalStartOffset%32), originalBytes);
	
	free(buffer);
}

void ROMCache_read(u32* ram_dest, u32 rom_offset, u32 length){
#ifdef PROFILE	
  start_section(ROM_SECTION);
#endif

	if(ROM_too_big){ // The whole ROM isn't in ARAM, we might have to move blocks in/out
		u32 length2 = length;
		u32 offset2 = rom_offset&BLOCK_MASK;
		char *dest = (char*)ram_dest;
		while(length2){
  		char* block = ROM_blocks[rom_offset>>BLOCK_SHIFT];
  		if(!block){ // This block is not alloced
  			if(!ARAM_block_available())
  				ARAM_block_free(ARAM_block_LRU('R'));
  			block = ARAM_block_alloc(&ROM_blocks[rom_offset>>BLOCK_SHIFT], 'R');
  			ROMCache_load_block(block, rom_offset&OFFSET_MASK);
  		}
			
			// Set length to the length for this block
			if(length2 > BLOCK_SIZE - offset2)
				length = BLOCK_SIZE - offset2;
			else length = length2;
					
			// Actually read for this block
			ARAM_ReadFromBlock(block,offset2,length,dest);
			ARAM_block_update_LRU(&block);
			// In case the read spans multiple blocks, increment state
			length2 -= length; offset2 = 0; dest += length; rom_offset += length;
		}
	  
	} else { // The entire ROM is in ARAM contiguously
#ifdef USE_ROM_CACHE_L1
		if(rom_offset >> L1_BLOCK_SHIFT == (rom_offset+length-1) >> L1_BLOCK_SHIFT){
			// Only worry about using L1 cache if the read falls
			//   within only one block for the L1 for now
			int i, min_i = 0;
			for(i=0; i<L1_NUM_BLOCKS; ++i){
				if(rom_offset >> L1_BLOCK_SHIFT == L1tag[i]){
					DEBUG_stats(7, "ROMCache L1 transfers", STAT_TYPE_ACCUM, 1);
					memcpy(ram_dest, L1[i] + (rom_offset&L1_BLOCK_MASK), length);
					return;
				} else if(L1tag[i] < 0 || L1LRU[i] < L1LRU[min_i])
					min_i = i;
			}
			
			DEBUG_stats(6, "ROMCache L1 misses", STAT_TYPE_ACCUM, 1);
			L1tag[min_i] = rom_offset >> L1_BLOCK_SHIFT;
			ARAM_ReadFromBlock(ROM,(rom_offset&(~L1_BLOCK_MASK)),L1_BLOCK_SIZE,(char*)L1[min_i]);
			L1LRU[min_i] = nextL1LRUValue++;
			
			DEBUG_stats(7, "ROMCache L1 transfers", STAT_TYPE_ACCUM, 1);
			memcpy(ram_dest, L1[min_i] + (rom_offset&L1_BLOCK_MASK), length);
		} else
#endif
		{
  		//just read
  		ARAM_ReadFromBlock(ROM,rom_offset,length,(char*)ram_dest);
		}
	}
#ifdef PROFILE	
	end_section(ROM_SECTION);
#endif
}

int ROMCache_load(fileBrowser_file* file){
	char txt[64];
	
#ifndef MENU_V2
	GUI_clear();
	GUI_centerText(true);
#endif
	sprintf(txt, "Loading ROM %s into ARAM", ROM_too_big ? "partially" : "fully");
	PRINT(txt);
	
	ROM_file = file;
	romFile_seekFile(ROM_file, 0, FILE_BROWSER_SEEK_SET);

	int bytes_to_read = ARQ_GetChunkSize();
	int* buffer = memalign(32, bytes_to_read);
	if(ROM_too_big){ // We can't load the entire ROM
		int i, block, available = ARAM_block_available();
		for(i=0; i<available; ++i)
		{
			block = ARAM_block_alloc(&ROM_blocks[i], 'R');
			int bytes_read, offset=0;
			int loads_til_update = 0;
			do {
				bytes_read = romFile_readFile(ROM_file, buffer, bytes_to_read);
				//initialize byteswapping
			  if(!readBefore)
			  {
  			  if(init_byte_swap(*(unsigned int*)buffer) == BYTE_SWAP_BAD) {
    			  romFile_deinit(ROM_file);
    			  free(buffer);
    			  return -2;
  			  }
  			  readBefore = 1;
			  }
				byte_swap((char*)buffer, bytes_read);
				DCFlushRange(buffer, bytes_read);
				ARQ_PostRequest(&ARQ_request, 0x10AD, AR_MRAMTOARAM, ARQ_PRIO_HI,
				                block + offset, buffer, bytes_read);
				offset += bytes_read;
				
				if(!loads_til_update--){
					SETLOADPROG((float)i/available + (float)offset/(available*BLOCK_SIZE));
					DRAWGUI();
#ifdef MENU_V2
					LoadingBar_showBar((float)i/available + (float)offset/(available*BLOCK_SIZE), txt);
#endif
					loads_til_update = 32;
				}
				
			} while(offset != BLOCK_SIZE);
			SETLOADPROG( -1.0f );
		}
	} else {
		ARAM_block_alloc_contiguous(&ROM, 'R', ROM_size / BLOCK_SIZE);
		int bytes_read, offset=0;
		int loads_til_update = 0;
		do {
			bytes_read = romFile_readFile(ROM_file, buffer, bytes_to_read);
			//initialize byteswapping
			if(!readBefore)
			{
  			if(init_byte_swap(*(unsigned int*)buffer) == BYTE_SWAP_BAD) {
    		  romFile_deinit(ROM_file);
    			free(buffer);
    			return -2;
  			}
  			readBefore = 1;
			}
			byte_swap((char*)buffer, bytes_read);
			DCFlushRange(buffer, bytes_read);
			ARQ_PostRequest(&ARQ_request, 0x10AD, AR_MRAMTOARAM, ARQ_PRIO_HI,
			                ROM + offset, buffer, bytes_read);
			offset += bytes_read;
			
			if(!loads_til_update--){
				SETLOADPROG( (float)offset/ROM_size );
				DRAWGUI();
#ifdef MENU_V2
				LoadingBar_showBar((float)offset/ROM_size, txt);
#endif
				loads_til_update = 32;
			}
			
		} while(bytes_read == bytes_to_read && offset != ROM_size);
		SETLOADPROG( -1.0f );
	}
	free(buffer);
	return 0; //should fix to return if reads were successful
}
