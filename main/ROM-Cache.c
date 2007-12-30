/* ROM-Cache.c - This is how the ROM should be accessed, this way the ROM doesn't waste RAM
   by Mike Slegeir for Mupen64-GC
 */

#include <ogc/arqueue.h>
#include <gccore.h>
#include "../gc_memory/ARAM.h"
#include "ROM-Cache.h"
#include "../fileBrowser/fileBrowser.h"

#ifdef USE_GUI
#include "../gui/GUI.h"
#define PRINT GUI_print
#else
#define PRINT printf
#endif

static char ROM_too_big;
static char* ROM, * ROM_blocks[64];
static u32 ROM_size;
//static char ROM_filename[SDCARD_MAX_PATH_LEN];
static fileBrowser_file* ROM_file;
static int ROM_byte_swap;

static ARQRequest ARQ_request;
//extern unsigned int rom_offsetDVD;	//dvd
//extern int isFromDVD;			//dvd
static void byte_swap(char* buffer, unsigned int length);
void showLoadProgress(float progress);

void ROMCache_init(u32 romSize){
	ARQ_Reset();
	ARQ_Init();
	ROM_too_big = romSize > (ARAM_block_available_contiguous() * BLOCK_SIZE);
	ROM_size = romSize;
	romFile_init( romFile_topLevel );
}

void ROMCache_deinit(){
	if(ROM_too_big){
		int i;
		for(i=0; i<64; ++i)
			if(ROM_blocks[i])
				ARAM_block_free(&ROM_blocks[i]);
	} else
		if(ROM) ARAM_block_free_contiguous(&ROM, ROM_size / BLOCK_SIZE);
	romFile_deinit( romFile_topLevel );
}

static void inline ROMCache_load_block(char* block, int rom_offset){
#if 0
	if(isFromDVD) {
		unsigned int tempDVDOffset = rom_offsetDVD+rom_offset;
		//printf("Loading ROM block %08x from DVD with offset %08x\n", block, rom_offset);
		int bytes_read, offset=0, bytes_to_read=ARQ_GetChunkSize();
		char* buffer = memalign(32, bytes_to_read);
		int loads_til_update = 0;
		do {
			bytes_read = read_safe(buffer, tempDVDOffset, bytes_to_read);
			byte_swap(buffer, bytes_read);
			DCFlushRange(buffer, bytes_read);
			ARQ_PostRequest(&ARQ_request, 0x10AD, AR_MRAMTOARAM, ARQ_PRIO_HI,
			                block + offset, buffer, bytes_read);
			offset += bytes_read;
			tempDVDOffset+=bytes_read;
			
			if(!loads_til_update){
				showLoadProgress( (float)offset/BLOCK_SIZE );
				loads_til_update = 16;
			}
			--loads_til_update;
			
		} while(offset != BLOCK_SIZE && bytes_read == bytes_to_read);
		free(buffer);
		showLoadProgress(1.0f);
		//printf("Success\n", block);
	}
	else {
		//printf("Loading ROM block %08x from SD-Card with offset %08x\n", block, rom_offset);
		sd_file* rom = SDCARD_OpenFile(ROM_filename, "rb");
		SDCARD_SeekFile(rom, rom_offset, SDCARD_SEEK_SET);
		int bytes_read, offset=0, bytes_to_read=ARQ_GetChunkSize();
		char* buffer = memalign(32, bytes_to_read);
		int loads_til_update = 0;
		do {
			bytes_read = SDCARD_ReadFile(rom, buffer, bytes_to_read);
			byte_swap(buffer, bytes_read);
			DCFlushRange(buffer, bytes_read);
			ARQ_PostRequest(&ARQ_request, 0x10AD, AR_MRAMTOARAM, ARQ_PRIO_HI,
			                block + offset, buffer, bytes_read);
			offset += bytes_read;
			
			if(!loads_til_update){
				showLoadProgress( (float)offset/BLOCK_SIZE );
				loads_til_update = 16;
			}
			--loads_til_update;
			
		} while(offset != BLOCK_SIZE && bytes_read == bytes_to_read);
		free(buffer);
		SDCARD_CloseFile(rom);
		showLoadProgress(1.0f);
		//printf("Success\n", block);
	}
#endif
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
			showLoadProgress( (float)offset/BLOCK_SIZE );
			loads_til_update = 16;
		}
		
	} while(offset != BLOCK_SIZE && bytes_read == bytes_to_read);
	free(buffer);
	showLoadProgress(1.0f);
}

void ROMCache_read(u32* ram_dest, u32 rom_offset, u32 length){
	// Start of the transfer must be 32-byte aligned
	// and the length must be a multiple of 32-bytes
	u32 adjusted_offset = rom_offset, buffer_length = length;
	u32 buffer_offset = 0;
	if(rom_offset % 32 != 0){
		adjusted_offset -= rom_offset % 32;
		buffer_length   += rom_offset % 32;
		buffer_offset    = rom_offset % 32;
	}
	if(buffer_length % 32 != 0)
		buffer_length += 32 - (buffer_length % 32);
	
	int* buffer = memalign(32, buffer_length);
	
	if(ROM_too_big){ // The whole ROM isn't in ARAM, we might have to move blocks in/out
		char* block = ROM_blocks[rom_offset>>20];
		int length2;
		
		//printf("Reading %dKB beginning at %08x from ROM\n",
		//       length/1024, rom_offset);
		
		if(!block){ // This block is not alloced
			if(!ARAM_block_available())
				ARAM_block_free(ARAM_block_LRU('R'));
			block = ARAM_block_alloc(&ROM_blocks[rom_offset>>20], 'R');
			
			ROMCache_load_block(block, rom_offset&0xFFF00000);
		}
		if(rom_offset>>20 != (rom_offset+length)>>20){
			length2 = length;
			length = BLOCK_SIZE - (rom_offset&0xFFFFF);
			length2 = length2 - length;
		}
		
		ARQ_PostRequest(&ARQ_request, 0x2EAD, ARQ_ARAMTOMRAM, ARQ_PRIO_LO,
		                block + (adjusted_offset&0xFFFFF), buffer, buffer_length);
		DCInvalidateRange(buffer, buffer_length);
		memcpy(ram_dest, (char*)buffer + buffer_offset, length);
		ARAM_block_update_LRU(&ROM_blocks[rom_offset>>20]); 
			
		if(rom_offset>>20 != (rom_offset+length)>>20){ // The data extends to the next block	
			// FIXME: I'm assuming that we won't read more than 1 block size of data at once
			//        I can handle this case, and any case with a while loop
			block = ROM_blocks[(rom_offset+length)>>20];
			if(!block){
				if(ARAM_block_available())
					ARAM_block_free(ARAM_block_LRU('R'));
				block = ARAM_block_alloc(&ROM_blocks[(rom_offset+length)>>20], 'R');
				
				ROMCache_load_block(block, (rom_offset+length)&0xFFF00000);
			}
			
			ARQ_PostRequest(&ARQ_request, 0x2EAD, ARQ_ARAMTOMRAM, ARQ_PRIO_LO,
			                block, buffer, buffer_length);
			DCInvalidateRange(buffer, buffer_length);
			memcpy(ram_dest+length/4, buffer, length2);
			ARAM_block_update_LRU(&ROM_blocks[(rom_offset+length)>>20]);
		}
	} else { // The entire ROM is in ARAM contiguously
		ARQ_PostRequest(&ARQ_request, 0x2EAD, AR_ARAMTOMRAM, ARQ_PRIO_LO,
		                ROM + adjusted_offset, buffer, buffer_length);
		DCInvalidateRange(buffer, buffer_length);
		memcpy(ram_dest, (char*)buffer+buffer_offset, length);
	}
	
	free(buffer);
}

void ROMCache_load(fileBrowser_file* file, int byteSwap){
	char txt[64];
	
	if(byteSwap == BYTE_SWAP_BAD) return;
	ROM_byte_swap = byteSwap;
	if(byteSwap == BYTE_SWAP_BYTE) PRINT("Byte swapped\n");
	else if(byteSwap == BYTE_SWAP_HALF) PRINT("Halfword swapped\n");
	
	/*
	sprintf(txt, "Loading ROM into ARAM %s\n", ROM_too_big ? "(ROM_too_big)" : "");
	PRINT(txt);
	if(ROM_too_big) sprintf(txt, "%d blocks available\n", ARAM_block_available());
	else            sprintf(txt, "%d contiguous blocks available\n", ARAM_block_available_contiguous());
	PRINT(txt);
	*/
	
	GUI_clear();
	sprintf(txt, "Loading ROM %s into ARAM.\n Please be patient...\n", ROM_too_big ? "partially" : "fully");
	PRINT(txt);
	
	ROM_file = file;
	romFile_seekFile(ROM_file, 0, FILE_BROWSER_SEEK_SET);

	int bytes_to_read = ARQ_GetChunkSize();
	int* buffer = memalign(32, bytes_to_read);
	if(ROM_too_big){ // We can't load the entire ROM
		int i, block, available = ARAM_block_available();
		for(i=0; i<available; ++i){
			block = ARAM_block_alloc(&ROM_blocks[i], 'R');
			//sprintf(txt, "ROM_blocks[%d] = 0x%08x\n", i, block);
			//PRINT(txt);
			int bytes_read, offset=0;
			int loads_til_update = 0;
			do {
				bytes_read = romFile_readFile(ROM_file, buffer, bytes_to_read);
				byte_swap(buffer, bytes_read);
				DCFlushRange(buffer, bytes_read);
				ARQ_PostRequest(&ARQ_request, 0x10AD, AR_MRAMTOARAM, ARQ_PRIO_HI,
				                block + offset, buffer, bytes_read);
				offset += bytes_read;
				
				if(!loads_til_update--){
					GUI_setLoadProg( (float)offset/BLOCK_SIZE );
					GUI_draw();
					loads_til_update = 32;
				}
				
			} while(offset != BLOCK_SIZE);
			GUI_setLoadProg( -1.0f );
		}
	} else {
		ARAM_block_alloc_contiguous(&ROM, 'R', ROM_size / BLOCK_SIZE);
		//sprintf(txt, "ROM = 0x%08x using %d blocks\n", ROM, ROM_size/BLOCK_SIZE);
		//PRINT(txt);
		int bytes_read, offset=0;
		int loads_til_update = 0;
		do {
			bytes_read = romFile_readFile(ROM_file, buffer, bytes_to_read);
			byte_swap(buffer, bytes_read);
			DCFlushRange(buffer, bytes_read);
			ARQ_PostRequest(&ARQ_request, 0x10AD, AR_MRAMTOARAM, ARQ_PRIO_HI,
			                ROM + offset, buffer, bytes_read);
			offset += bytes_read;
			
			if(!loads_til_update--){
				GUI_setLoadProg( (float)offset/ROM_size );
				GUI_draw();
				loads_til_update = 32;
			}
			
		} while(bytes_read == bytes_to_read && offset != ROM_size);
		GUI_setLoadProg( -1.0f );
	}
	free(buffer);
}

#if 0
// TODO: Support byte-swapped ROMs
void ROMCache_load_SDCard(char* filename, int byteSwap){
	char txt[64];
	
	if(byteSwap == BYTE_SWAP_BAD) return;
	ROM_byte_swap = byteSwap;
	if(byteSwap == BYTE_SWAP_BYTE) PRINT("Byte swapped\n");
	else if(byteSwap == BYTE_SWAP_HALF) PRINT("Halfword swapped\n");
	
	sprintf(txt, "Loading ROM into ARAM %s\n", ROM_too_big ? "(ROM_too_big)" : "");
	PRINT(txt);
	if(ROM_too_big) sprintf(txt, "%d blocks available\n", ARAM_block_available());
	else            sprintf(txt, "%d contiguous blocks available\n", ARAM_block_available_contiguous());
	PRINT(txt);
	
	strncpy(ROM_filename, filename, SDCARD_MAX_PATH_LEN);
	sd_file* rom = SDCARD_OpenFile(filename, "rb");
	SDCARD_SeekFile(rom, 0, SDCARD_SEEK_SET);

	int bytes_to_read = ARQ_GetChunkSize();
	int* buffer = memalign(32, bytes_to_read);
	if(ROM_too_big){ // We can't load the entire ROM
		int i, block, available = ARAM_block_available();
		for(i=0; i<available; ++i){
			block = ARAM_block_alloc(&ROM_blocks[i], 'R');
			sprintf(txt, "ROM_blocks[%d] = 0x%08x\n", i, block);
			PRINT(txt);
			int bytes_read, offset=0;
			do {
				bytes_read = SDCARD_ReadFile(rom, buffer, bytes_to_read);
				byte_swap(buffer, bytes_read);
				DCFlushRange(buffer, bytes_read);
				ARQ_PostRequest(&ARQ_request, 0x10AD, AR_MRAMTOARAM, ARQ_PRIO_HI,
				                block + offset, buffer, bytes_read);
				offset += bytes_read;
			} while(offset != BLOCK_SIZE);
		}
	} else {
		ARAM_block_alloc_contiguous(&ROM, 'R', ROM_size / BLOCK_SIZE);
		sprintf(txt, "ROM = 0x%08x using %d blocks\n", ROM, ROM_size/BLOCK_SIZE);
		PRINT(txt);
		int bytes_read, offset=0;
		do {
			bytes_read = SDCARD_ReadFile(rom, buffer, bytes_to_read);
			byte_swap(buffer, bytes_read);
			DCFlushRange(buffer, bytes_read);
			ARQ_PostRequest(&ARQ_request, 0x10AD, AR_MRAMTOARAM, ARQ_PRIO_HI,
			                ROM + offset, buffer, bytes_read);
			offset += bytes_read;
		} while(bytes_read == bytes_to_read && offset != ROM_size);
	}
	free(buffer);
	SDCARD_CloseFile(rom);
}

// TODO: Support byte-swapped ROMs
void ROMCache_load_DVD(char* filename, int byteSwap){
	char txt[64];
	
	if(byteSwap == BYTE_SWAP_BAD) return;
	ROM_byte_swap = byteSwap;
	if(byteSwap == BYTE_SWAP_BYTE) PRINT("Byte swapped\n");
	else if(byteSwap == BYTE_SWAP_HALF) PRINT("Halfword swapped\n");
	
	sprintf(txt, "Loading ROM into ARAM %s\n", ROM_too_big ? "(ROM_too_big)" : "");
	PRINT(txt);
	if(ROM_too_big) sprintf(txt, "%d blocks available\n", ARAM_block_available());
	else            sprintf(txt, "%d contiguous blocks available\n", ARAM_block_available_contiguous());
	PRINT(txt);
	
	strncpy(ROM_filename, filename, SDCARD_MAX_PATH_LEN);

	int bytes_to_read = ARQ_GetChunkSize();
	int* buffer = memalign(32, bytes_to_read);
	unsigned int tempDVDOffset = rom_offsetDVD;
	if(ROM_too_big){ // We can't load the entire ROM
		int i, block, available = ARAM_block_available();
		for(i=0; i<available; ++i){
			block = ARAM_block_alloc(&ROM_blocks[i], 'R');
			sprintf(txt, "ROM_blocks[%d] = 0x%08x\n", i, block);
			PRINT(txt);
			int bytes_read, offset=0;
			do {
				bytes_read = read_safe(buffer,tempDVDOffset, bytes_to_read);
				byte_swap(buffer, bytes_read);
				DCFlushRange(buffer, bytes_read);
				ARQ_PostRequest(&ARQ_request, 0x10AD, AR_MRAMTOARAM, ARQ_PRIO_HI,
				                block + offset, buffer, bytes_read);
				offset += bytes_read;
				tempDVDOffset +=bytes_read;
			} while(offset != BLOCK_SIZE);
		}
	} else {
		ARAM_block_alloc_contiguous(&ROM, 'R', ROM_size / BLOCK_SIZE);
		sprintf(txt, "ROM = 0x%08x using %d blocks\n", ROM, ROM_size/BLOCK_SIZE);
		PRINT(txt);
		int bytes_read, offset=0;
		do {
			bytes_read = read_safe(buffer, tempDVDOffset, bytes_to_read);
			byte_swap(buffer, bytes_read);
			DCFlushRange(buffer, bytes_read);
			ARQ_PostRequest(&ARQ_request, 0x10AD, AR_MRAMTOARAM, ARQ_PRIO_HI,
			                ROM + offset, buffer, bytes_read);
			offset += bytes_read;
			tempDVDOffset +=bytes_read;
		}
		 while(bytes_read == bytes_to_read && offset != ROM_size);
		 PRINT("Stopping motor, as you wont need it :)\n");
		dvd_motor_off();
	}
	free(buffer);
	//SDCARD_CloseFile(rom);

}
#endif

static void byte_swap(char* buffer, unsigned int length){
	if(ROM_byte_swap == BYTE_SWAP_NONE || ROM_byte_swap == BYTE_SWAP_BAD)
		return;
	
	int i, temp;
	if(ROM_byte_swap == BYTE_SWAP_HALF){
		for(i=0; i<length/2; i+=2){
			temp                  = ((short*)buffer)[i];
			((short*)buffer)[i]   = ((short*)buffer)[i+1];
			((short*)buffer)[i+1] = temp;
		}
	} else if(ROM_byte_swap == BYTE_SWAP_BYTE){
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

