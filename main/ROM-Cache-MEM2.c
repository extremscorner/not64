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
#include <string.h>
#include <ogc/system.h>
#include <ogc/video.h>
#include "../gc_memory/MEM2.h"
#include "ROM-Cache.h"
#include "rom.h"
#include "gczip.h"
#include "wii_vm.h"

void LoadingBar_showBar(float percent, const char* string);

static char  ROMTooBig;
static char  ROMCompressed;
static char* ROMCache = ROMCACHE_LO;
static int   ROMCacheSize = ROMCACHE_SIZE;

void ROMCache_init(fileBrowser_file* file)
{
	PKZIPHEADER pkzip;
	romFile_readFile(file, &pkzip, sizeof(PKZIPHEADER));
	romFile_seekFile(file, 0, FILE_BROWSER_SEEK_SET);
	
	if (pkzip.zipid == PKZIPID) {
		rom_length = bswap32(pkzip.uncompressedSize);
		inflate_init(&pkzip);
		ROMCompressed = 1;
	} else {
		rom_length = file->size;
		ROMCompressed = 0;
	}
	
	switch (SYS_GetPhysicalMem2Size()) {
		default:
			ROMCache = ROMCACHE_LO;
			ROMCacheSize = ROMCACHE_SIZE;
			break;
		case 128*MB:
			switch (SYS_GetSimulatedMem2Size()) {
				case 128*MB:
					ROMCache = ROMCACHE_128MB_LO;
					ROMCacheSize = ROMCACHE_128MB_SIZE;
					break;
				case 64*MB:
					ROMCache = ROMCACHE_64_128MB_LO;
					ROMCacheSize = ROMCACHE_64_128MB_SIZE;
					break;
				default:
					ROMCache = ROMCACHE_LO;
					ROMCacheSize = ROMCACHE_SIZE;
					break;
			}
			break;
		case 256*MB:
			switch (SYS_GetSimulatedMem2Size()) {
				case 256*MB:
					ROMCache = ROMCACHE_256MB_LO;
					ROMCacheSize = ROMCACHE_256MB_SIZE;
					break;
				case 128*MB:
					ROMCache = ROMCACHE_128_256MB_LO;
					ROMCacheSize = ROMCACHE_128_256MB_SIZE;
					break;
				case 64*MB:
					ROMCache = ROMCACHE_64_256MB_LO;
					ROMCacheSize = ROMCACHE_64_256MB_SIZE;
					break;
				default:
					ROMCache = ROMCACHE_LO;
					ROMCacheSize = ROMCACHE_SIZE;
					break;
			}
			break;
	}
	
	ROMTooBig = rom_length > ROMCacheSize;
}

void ROMCache_deinit()
{
	if (ROMTooBig) {
		ROMCache = NULL;
		VM_Deinit();
	}
}

void* ROMCache_pointer(u32 rom_offset)
{
	return ROMCache + rom_offset;
}

void ROMCache_read(u8* ram_dest, u32 rom_offset, u32 length)
{
	memcpy(ram_dest, ROMCache + rom_offset, length);
}

void ROMCache_write(u8* ram_src, u32 rom_offset, u32 length)
{
	memcpy(ROMCache + rom_offset, ram_src, length);
}

int ROMCache_load(fileBrowser_file* file)
{
	char txt[128];
	int bytes_read;
	unsigned i = 0, count = 0;
	
	if (ROMTooBig) {
		void* VMBase = VM_Init(rom_length, ROMCACHE_SIZE);
		if (VMBase == NULL)
			return ROM_CACHE_ERROR_READ;
		
		ROMCache = VMBase;
		ROMCacheSize = rom_length;
	}
	
	sprintf(txt, "Loading ROM %s into MEM2", ROMTooBig ? "partially" : "fully");
	
	if (ROMCompressed) {
		unsigned char buf[ZIPCHUNK];
		do {
			bytes_read = romFile_readFile(file, buf, ZIPCHUNK);
			if (bytes_read < 0)
				return ROM_CACHE_ERROR_READ;
			
			bytes_read = inflate_chunk(ROMCache + i, buf, bytes_read);
			if (bytes_read < 0)
				return ROM_CACHE_ERROR_READ;
			
			if (i == 0 && init_byte_swap(*(u32*)ROMCache) == BYTE_SWAP_BAD) {
				romFile_deinit(file);
				return ROM_CACHE_INVALID_ROM;
			}
			
			byte_swap(ROMCache + (i & ~3), bytes_read + (i & 3));
			i += bytes_read;
			
			if (VIDEO_GetRetraceCount() - count > 2) {
				LoadingBar_showBar((float)i / rom_length, txt);
				count = VIDEO_GetRetraceCount();
			}
		} while (i < rom_length);
	} else {
		do {
			bytes_read = romFile_readFile(file, ROMCache + i, 32*KB);
			if (bytes_read < 0)
				return ROM_CACHE_ERROR_READ;
			
			if (i == 0 && init_byte_swap(*(u32*)ROMCache) == BYTE_SWAP_BAD) {
				romFile_deinit(file);
				return ROM_CACHE_INVALID_ROM;
			}
			
			byte_swap(ROMCache + (i & ~3), bytes_read + (i & 3));
			i += bytes_read;
			
			if (VIDEO_GetRetraceCount() - count > 2) {
				LoadingBar_showBar((float)i / rom_length, txt);
				count = VIDEO_GetRetraceCount();
			}
		} while (i < rom_length);
	}
	
	return 0;
}
