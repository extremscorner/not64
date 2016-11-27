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
#include "../gc_memory/MEM2.h"
#include "ROM-Cache.h"
#include "rom.h"
#include "gczip.h"
#include "wii_vm.h"

void LoadingBar_showBar(float percent, const char* string);

static char  ROMTooBig;
static char  ROMCompressed;
static char* ROMBase = ROMCACHE_LO;

void ROMCache_init(fileBrowser_file* file)
{
	PKZIPHEADER pkzip;
	romFile_readFile(file, &pkzip, sizeof(PKZIPHEADER));
	
	if (pkzip.zipid == PKZIPID) {
		rom_length = bswap32(pkzip.uncompressedSize);
		inflate_init(&pkzip);
		ROMCompressed = 1;
	} else {
		rom_length = file->size;
		ROMCompressed = 0;
	}
	
	ROMTooBig = rom_length > ROMCACHE_SIZE;
	romFile_seekFile(file, 0, FILE_BROWSER_SEEK_SET);
}

void ROMCache_deinit()
{
	if (ROMTooBig) {
		ROMBase = ROMCACHE_LO;
		VM_Deinit();
	}
}

void* ROMCache_pointer(u32 rom_offset)
{
	return ROMBase + rom_offset;
}

void ROMCache_read(u8* ram_dest, u32 rom_offset, u32 length)
{
	memcpy(ram_dest, ROMBase + rom_offset, length);
}

int ROMCache_load(fileBrowser_file* file)
{
	char txt[128];
	int bytes_read;
	unsigned i = 0, loads_til_update = 0;
	
	if (ROMTooBig) {
		void* VMBase = VM_Init(rom_length, ROMCACHE_SIZE);
		if (VMBase == NULL)
			return ROM_CACHE_ERROR_READ;
		
		ROMBase = VMBase;
	}
	
	sprintf(txt, "Loading ROM %s into MEM2", ROMTooBig ? "partially" : "fully");
	
	if (ROMCompressed) {
		unsigned char buf[ZIPCHUNK];
		do {
			bytes_read = romFile_readFile(file, buf, ZIPCHUNK);
			if (bytes_read < 0)
				return ROM_CACHE_ERROR_READ;
			
			bytes_read = inflate_chunk(ROMBase + i, buf, bytes_read);
			if (bytes_read < 0)
				return ROM_CACHE_ERROR_READ;
			
			i += bytes_read;
			
			if (!loads_til_update--) {
				LoadingBar_showBar((float)i / rom_length, txt);
				loads_til_update = 256;
			}
		} while (bytes_read > 0);
		
		if (init_byte_swap(*(u32*)ROMBase) == BYTE_SWAP_BAD) {
			romFile_deinit(file);
			return ROM_CACHE_INVALID_ROM;
		}
		
		LoadingBar_showBar(0.0, "Byteswapping ROM");
		byte_swap(ROMBase, rom_length);
	} else {
		do {
			bytes_read = romFile_readFile(file, ROMBase + i, 32*KB);
			if (bytes_read < 0)
				return ROM_CACHE_ERROR_READ;
			
			if (i == 0 && init_byte_swap(*(u32*)ROMBase) == BYTE_SWAP_BAD) {
				romFile_deinit(file);
				return ROM_CACHE_INVALID_ROM;
			}
			
			byte_swap(ROMBase + i, bytes_read);
			i += bytes_read;
			
			if (!loads_til_update--) {
				LoadingBar_showBar((float)i / rom_length, txt);
				loads_til_update = 16;
			}
		} while (bytes_read > 0);
	}
	
	return 0;
}
