/**
 * Wii64 - ROM-Cache-MEM2.c (Wii ROM Cache)
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

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <malloc.h>
#include "../fileBrowser/fileBrowser.h"
#include "../gui/gui_GX-menu.h"
#include "../gc_memory/MEM2.h"
#include "../r4300/r4300.h"
#include "../gui/DEBUG.h"
#include "../gui/GUI.h"
#include "ROM-Cache.h"
#include "gczip.h"
#include "zlib.h"

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

#define BLOCK_SIZE  (512*1024)
#define BLOCK_MASK  (BLOCK_SIZE-1)
#define OFFSET_MASK (0xFFFFFFFF-BLOCK_MASK)
#define BLOCK_SHIFT (19)	//only change ME and BLOCK_SIZE
#define MAX_ROMSIZE (64*1024*1024)
#define NUM_BLOCKS  (MAX_ROMSIZE/BLOCK_SIZE)
#define LOAD_SIZE   (32*1024)

static u32   ROMSize;
static int   ROMTooBig;
//static int   ROMCompressed;
//static int   ROMHeaderSize;
static char* ROMBlocks[NUM_BLOCKS];
static int   ROMBlocksLRU[NUM_BLOCKS];
static fileBrowser_file* ROMFile;
static char readBefore = 0;

extern void showLoadProgress(float);
extern void pauseAudio(void);
extern void resumeAudio(void);
extern BOOL hasLoadedROM;

void DUMMY_print(char* string) { }
void DUMMY_setLoadProg(float percent) { }
void DUMMY_draw() { }

static void ensure_block(u32 block);

PKZIPHEADER pkzip;

void ROMCache_init(fileBrowser_file* f){
  readBefore = 0; //de-init byteswapping
  ROMFile = f;
	ROMSize = f->size;
	ROMTooBig = ROMSize > ROMCACHE_SIZE;

	romFile_seekFile(f, 0, FILE_BROWSER_SEEK_SET);	// Lets be nice and keep the file at 0.
}

void ROMCache_deinit(){
	//we don't de-init the romFile here because it takes too much time to fopen/fseek/fread/fclose
}

void* ROMCache_pointer(u32 rom_offset){
	if(ROMTooBig){
		u32 block = rom_offset >> BLOCK_SHIFT;
		u32 block_offset = rom_offset & BLOCK_MASK;
		
		ensure_block(block);
		
		return ROMBlocks[block] + block_offset;
	} else {
		return ROMCACHE_LO + rom_offset;
	}
}

static void ROMCache_load_block(char* dst, u32 rom_offset){
  if((hasLoadedROM) && (!stop))
    pauseAudio();
	showLoadProgress( 1.0f );
	u32 offset = 0, bytes_read, loads_til_update = 0;
	romFile_seekFile(ROMFile, rom_offset, FILE_BROWSER_SEEK_SET);
	while(offset < BLOCK_SIZE){
		bytes_read = romFile_readFile(ROMFile, dst + offset, LOAD_SIZE);
		byte_swap(dst + offset, bytes_read);
		offset += bytes_read;
		
		if(!loads_til_update--){
//			showLoadProgress( (float)offset/BLOCK_SIZE );
			loads_til_update = 32;
		}
	}
//	showLoadProgress( 1.0f );
	if((hasLoadedROM) && (!stop))
	  resumeAudio();
}

static void ensure_block(u32 block){
	if(!ROMBlocks[block]){
		// The block we're trying to read isn't in the cache
		// Find the Least Recently Used Block
		int i, max_i = 0, max_lru = 0;
		for(i=0; i<NUM_BLOCKS; ++i)
			if(ROMBlocks[i] && ROMBlocksLRU[i] > max_lru)
				max_i = i, max_lru = ROMBlocksLRU[i];
		ROMBlocks[block] = ROMBlocks[max_i]; // Take its place
		ROMCache_load_block(ROMBlocks[block], block << BLOCK_SHIFT);
		ROMBlocks[max_i] = 0; // Evict the LRU block
	}
}

void ROMCache_read(u8* dest, u32 offset, u32 length){
  if(ROMTooBig){
		u32 block = offset>>BLOCK_SHIFT;
		u32 length2 = length;
		u32 offset2 = offset&BLOCK_MASK;
		
		while(length2){
			ensure_block(block);
			
			// Set length to the length for this block
			if(length2 > BLOCK_SIZE - offset2)
				length = BLOCK_SIZE - offset2;
			else length = length2;
		
			// Increment LRU's; set this one to 0
			int i;
			for(i=0; i<NUM_BLOCKS; ++i) ++ROMBlocksLRU[i];
			ROMBlocksLRU[block] = 0;
			
			// Actually read for this block
			memcpy(dest, ROMBlocks[block] + offset2, length);
			
			// In case the read spans multiple blocks, increment state
			++block; length2 -= length; offset2 = 0; dest += length; offset += length;
		}
	} else {
		memcpy(dest, ROMCACHE_LO + offset, length);
	}
}

int ROMCache_load(fileBrowser_file* f){
	char txt[128];
#ifndef MENU_V2
	GUI_clear();
	GUI_centerText(true);
#endif
	sprintf(txt, "Loading ROM %s into MEM2",ROMTooBig ? "partially" : "fully");
	PRINT(txt);

	romFile_seekFile(ROMFile, 0, FILE_BROWSER_SEEK_SET);
	
	u32 offset = 0,loads_til_update = 0;
	int bytes_read;
	u32 sizeToLoad = MIN(ROMCACHE_SIZE, ROMSize);
	while(offset < sizeToLoad){
		bytes_read = romFile_readFile(ROMFile, ROMCACHE_LO + offset, LOAD_SIZE);
		
		if(bytes_read < 0){		// Read fail!

			SETLOADPROG( -1.0f );
			return -1;
		}
		//initialize byteswapping if it isn't already
		if(!readBefore)
		{
 			if(init_byte_swap(*(unsigned int*)ROMCACHE_LO) == BYTE_SWAP_BAD) {
 			  romFile_deinit(ROMFile);
 			  return -2;
		  }
 			readBefore = 1;
		}
		//byteswap
		byte_swap(ROMCACHE_LO + offset, bytes_read);
		
		offset += bytes_read;
		
		if(!loads_til_update--){
			SETLOADPROG( (float)offset/sizeToLoad );
			DRAWGUI();
#ifdef MENU_V2
			LoadingBar_showBar((float)offset/sizeToLoad, txt);
#endif
			loads_til_update = 16;
		}
	}
	
	if(ROMTooBig){ // Set block pointers if we need to
		int i;
		for(i=0; i<ROMCACHE_SIZE/BLOCK_SIZE; ++i)
			ROMBlocks[i] = ROMCACHE_LO + i*BLOCK_SIZE;
		for(; i<ROMSize/BLOCK_SIZE; ++i)
			ROMBlocks[i] = 0;
		for(i=0; i<ROMSize/BLOCK_SIZE; ++i)
			ROMBlocksLRU[i] = i;
	}
	
	SETLOADPROG( -1.0f );
	return 0;
}



