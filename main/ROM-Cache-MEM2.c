/* ROM-Cache-MEM2.c - ROM Cache utilizing MEM2 on Wii
   by Mike Slegeir for Mupen64-GC
 ******************************************************
   Optimization: Store whatever blocks that don't fit
                   in MEM2 cache on the filesystem
                   under /tmp; keep an extra block for
                   asynchronous write-back
                 Create a L1 cache in MEM1
 */

#include <stdio.h>
#include <stdint.h>
#include <malloc.h>
#include "ROM-Cache.h"
#include "../gc_memory/MEM2.h"
#include "../fileBrowser/fileBrowser.h"
#include "gczip.h"
#include "zlib.h"


#ifdef USE_GUI
#include "../gui/GUI.h"
#include "../gui/gui_GX-menu.h"
#define PRINT GUI_print
#else
#define PRINT printf
#endif
#include "../gui/DEBUG.h"

#define BLOCK_SIZE (1024*1024)
#define LOAD_SIZE  (4*1024)
static u32   ROMSize;
static int   ROMTooBig;
static int   ROMCompressed;
static int   ROMHeaderSize;
static char* ROMBlocks[64];
static int   ROMBlocksLRU[64];
static fileBrowser_file* ROMFile;

void* memcpy(void* dst, void* src, int len);
void showLoadProgress(float);

#ifdef USE_ROM_CACHE_L1
static u8  L1[256*1024];
static u32 L1tag;
#endif

PKZIPHEADER pkzip;

void ROMCache_init(fileBrowser_file* f){
	romFile_readFile(f, &pkzip, sizeof(PKZIPHEADER));
	if(pkzip.zipid != PKZIPID){		//PKZIP magic
		ROMSize = f->size;
		ROMTooBig = ROMSize > ROMCACHE_SIZE;
		ROMCompressed = 0;
	}
	else	// Compressed file found.
	{
		ROMCompressed = 1;
		ROMTooBig = 0;		// Currently no way to swap out compressed ROMs from filesystem.
		ROMSize = FLIP32(pkzip.uncompressedSize);
		ROMHeaderSize = (sizeof(PKZIPHEADER) + FLIP16(pkzip.filenameLength) + FLIP16(pkzip.extraDataLength));
		inflate_init(&pkzip);
	}

	romFile_seekFile(f, 0, FILE_BROWSER_SEEK_SET);	// Lets be nice and keep the file at 0.
#ifdef USE_ROM_CACHE_L1
	L1tag = -1;
#endif	
	//romFile_init( romFile_topLevel );
}

void ROMCache_deinit(){
	//romFile_deinit( romFile_topLevel );
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
	// Display stats for reads
	static int last_block = -1;
	if( offset>>18 == last_block )
		DEBUG_stats(3, "ROMCache same block", STAT_TYPE_ACCUM, 1);
	else
		DEBUG_stats(4, "ROMCache different block", STAT_TYPE_ACCUM, 1);
	last_block = offset >> 18;
	DEBUG_stats(5, "ROMCache avg length", STAT_TYPE_AVGE, length);
	
	if(ROMTooBig){
		u32 block = offset>>20;
		u32 length2 = length;
		u32 offset2 = offset&0xFFFFF;
		
		while(length2){
			if(!ROMBlocks[block]){
				// The block we're trying to read isn't in the cache
				// Find the Least Recently Used Block
				int i, max_i = 0;
				for(i=0; i<64; ++i)
					if(ROMBlocks[i] && ROMBlocksLRU[i] > ROMBlocksLRU[max_i])
						max_i = i;
				ROMBlocks[block] = ROMBlocks[max_i]; // Take its place
				ROMCache_load_block(ROMBlocks[block], offset&0xFFF00000);
				ROMBlocks[max_i] = 0; // Evict the LRU block
			}
			
			// Set length to the length for this block
			if(length2 > BLOCK_SIZE - offset2)
				length = BLOCK_SIZE - offset2;
			else length = length2;
		
			// Increment LRU's; set this one to 0
			int i;
			for(i=0; i<64; ++i) ++ROMBlocksLRU[i];
			ROMBlocksLRU[block] = 0;
			
			// Actually read for this block
			memcpy(dest, ROMBlocks[block] + offset2, length);
			
			// In case the read spans multiple blocks, increment state
			++block; length2 -= length; offset2 = 0; dest += length/4; offset += length;
		}
	} else {
#ifdef USE_ROM_CACHE_L1
		if(offset >> 18 == (offset+length-1) >> 18){
			// Only worry about using L1 cache if the read falls
			//   within only one block for the L1 for now
			if(offset >> 18 != L1tag){
				DEBUG_stats(6, "ROMCache L1 misses", STAT_TYPE_ACCUM, 1);
				memcpy(L1, ROMCACHE_LO + (offset&(~0x3FFFF)), 256*1024);
				L1tag = offset >> 18;
			}
			DEBUG_stats(7, "ROMCache L1 transfers", STAT_TYPE_ACCUM, 1);
			memcpy(dest, L1 + (offset&0x3FFFF), length);
		} else
#endif
		{
			memcpy(dest, ROMCACHE_LO + offset, length);
		}
	}
}

int ROMCache_load(fileBrowser_file* f){
	char txt[128];
	void* buf;
	int ret;
	GUI_clear();
	GUI_centerText(true);
	sprintf(txt, "%s ROM %s into MEM2.\n Please be patient...\n", ROMCompressed ? "Uncompressing" : "Loading",ROMTooBig ? "partially" : "fully");
	PRINT(txt);

	ROMFile = f;
	romFile_seekFile(f, 0, FILE_BROWSER_SEEK_SET);
	
	u32 offset = 0,loads_til_update = 0;
	int bytes_read;
	u32 sizeToLoad = MIN(ROMCACHE_SIZE, ROMSize);
	if(ROMCompressed){
		buf = malloc(LOAD_SIZE);
		do{
			bytes_read = romFile_readFile(f, buf, LOAD_SIZE);

			if(bytes_read < 0){		// Read fail!
				GUI_setLoadProg( -1.0f );
				free(buf);
				return -1;
			}

			ret = inflate_chunk(ROMCACHE_LO + offset, buf, bytes_read);
			if(ret > 0)
				offset += ret;

			if(!loads_til_update--){
				GUI_setLoadProg( (float)offset/sizeToLoad );
				GUI_draw();
				loads_til_update = 16;
			}
		}while(ret > 0);
		free(buf);
		if(ret){	// Uh oh, decompression fail!
			GUI_setLoadProg( -1.0f );
			return -1;
		}
	}
	else
	{
		while(offset < sizeToLoad){
			bytes_read = romFile_readFile(f, ROMCACHE_LO + offset, LOAD_SIZE);

			if(bytes_read < 0){		// Read fail!
				GUI_setLoadProg( -1.0f );
				return -1;
			}

			offset += bytes_read;
		
			if(!loads_til_update--){
				GUI_setLoadProg( (float)offset/sizeToLoad );
				GUI_draw();
				loads_til_update = 16;
			}
		}
	}	
	init_byte_swap(*((uint32_t*)ROMCACHE_LO));
	byte_swap(ROMCACHE_LO + offset, sizeToLoad);
	
	if(ROMTooBig){ // Set block pointers if we need to
		int i;
		for(i=0; i<ROMCACHE_SIZE/BLOCK_SIZE; ++i)
			ROMBlocks[i] = ROMCACHE_LO + i*BLOCK_SIZE;
		for(; i<ROMSize/BLOCK_SIZE; ++i)
			ROMBlocks[i] = 0;
		for(i=0; i<ROMSize/BLOCK_SIZE; ++i)
			ROMBlocksLRU[i] = i;
	}
	
	GUI_setLoadProg( -1.0f );
	return 0;
}



