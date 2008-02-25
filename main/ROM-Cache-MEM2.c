/* ROM-Cache-MEM2.c - ROM Cache utilizing MEM2 on Wii
   by Mike Slegeir for Mupen64-GC
 */

#include <stdio.h>
#include "ROM-Cache.h"
#include "../gc_memory/MEM2.h"
#include "../fileBrowser/fileBrowser.h"

#ifdef USE_GUI
#include "../gui/GUI.h"
#include "../gui/gui_GX-menu.h"
#define PRINT GUI_print
#else
#define PRINT printf
#endif

#define BLOCK_SIZE (1024*1024)
#define LOAD_SIZE  (4*1024)
static u32   ROMSize;
static int   ROMTooBig;
static int   ROMByteSwap;
static char* ROMBlocks[64];
static int   ROMBlocksLRU[64];
static fileBrowser_file* ROMFile;

static void byte_swap(char* buffer, u32 length);
void* memcpy(void* dst, void* src, int len);
void showLoadProgress(float);


void ROMCache_init(u32 size){
	ROMSize = size;
	ROMTooBig = size > ROMCACHE_SIZE;
	
	romFile_init( romFile_topLevel );
}

void ROMCache_deinit(){
	romFile_deinit( romFile_topLevel );
}

void ROMCache_load_block(char* dst, u32 rom_offset){
	romFile_seekFile(ROMFile, rom_offset, FILE_BROWSER_SEEK_SET);
	
	u32 offset = 0, bytes_read, loads_til_update = 0;
	while(offset < BLOCK_SIZE){
		bytes_read = romFile_readFile(ROMFile, dst + offset, LOAD_SIZE);
		byte_swap(dst + offset, bytes_read);
		offset += bytes_read;
		
		if(!loads_til_update--){
			showLoadProgress( (float)offset/BLOCK_SIZE );
			loads_til_update = 32;
		}
	}
	showLoadProgress( 1.0f );
}

void ROMCache_read(u32* dest, u32 offset, u32 length){
	if(ROMTooBig){
		u32 block = offset>>20;
		u32 length2 = length;
		while(length2){
			if(!ROMBlocks[block]){
				// The block we're trying to read isnt in the cache
				// Find the Least Recently Used Block
				int i, max_i = 0;
				for(i=0; i<64; ++i)
					if(ROMBlocksLRU[i] > ROMBlocksLRU[max_i])
						max_i = i;
				ROMBlocks[block] = ROMBlocks[max_i]; // Take its place
				ROMCache_load_block(ROMBlocks[block], offset&0xFFF00000);
				ROMBlocks[max_i] = 0; // Evict the LRU block
			}
			
			// Set length to the length for this block
			if(length2 > BLOCK_SIZE  - (offset&0xFFFFF))
				length = BLOCK_SIZE - (offset&0xFFFFF);
			else length = length2;
		
			// Increment LRU's; set this one to 0
			int i;
			for(i=0; i<64; ++i) ++ROMBlocksLRU[i];
			ROMBlocksLRU[block] = 0;
			
			// Actually read for this block
			memcpy(dest, ROMBlocks[block] + offset, length);
			
			// In case the read spans multiple blocks
			++block; length2 -= length; offset = 0; dest += length/4;
		}
	} else
		memcpy(dest, ROMCACHE_LO + offset, length);
}

void ROMCache_load(fileBrowser_file* f, int byteSwap){
	char txt[128];
	GUI_clear();
	GUI_centerText(true);
	sprintf(txt, "Loading ROM %s into MEM2.\n Please be patient...\n", ROMTooBig ? "partially" : "fully");
	PRINT(txt);
	
	ROMByteSwap = byteSwap;
	ROMFile = f;
	romFile_seekFile(f, 0, FILE_BROWSER_SEEK_SET);
	
	u32 offset = 0, bytes_read, loads_til_update = 0;
	u32 sizeToLoad = MIN(ROMCACHE_SIZE, ROMSize);
	while(offset < sizeToLoad){
		bytes_read = romFile_readFile(f, ROMCACHE_LO + offset, LOAD_SIZE);
		byte_swap(ROMCACHE_LO + offset, bytes_read);
		offset += bytes_read;
		
		if(!loads_til_update--){
			GUI_setLoadProg( (float)offset/sizeToLoad );
			GUI_draw();
			loads_til_update = 16;
		}
	}
	
	if(ROMTooBig){ // Set block pointers if we need to
		int i;
		for(i=0; i<ROMCACHE_SIZE/BLOCK_SIZE; ++i)
			ROMBlocks[i] = ROMCACHE_LO + i*BLOCK_SIZE;
		for(; i<ROMSize/BLOCK_SIZE; ++i)
			ROMBlocks[i] = 0;
	}
	
	GUI_setLoadProg( -1.0f );
}

static void byte_swap(char* buffer, u32 length){
        if(ROMByteSwap == BYTE_SWAP_NONE || ROMByteSwap == BYTE_SWAP_BAD)
                return;
        
        int i, temp;
        if(ROMByteSwap == BYTE_SWAP_HALF){
                for(i=0; i<length/2; i+=2){
                        temp                  = ((short*)buffer)[i];
                        ((short*)buffer)[i]   = ((short*)buffer)[i+1];
                        ((short*)buffer)[i+1] = temp;
                }
        } else if(ROMByteSwap == BYTE_SWAP_BYTE){
                for(i=0; i<length/4; i+=4){
                        temp        = buffer[i];
                        buffer[i]   = buffer[i+3];
                        buffer[i+3] = temp;
                        
                        temp        = buffer[i+1];
                        buffer[i+1] = buffer[i+2];
                        buffer[i+2] = temp;
                }
        }
}

