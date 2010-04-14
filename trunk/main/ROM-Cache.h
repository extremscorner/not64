/**
 * Wii64 - ROM-Cache.h
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

#ifndef ROM_CACHE_H
#define ROM_CACHE_H

#include "../fileBrowser/fileBrowser.h"
#include <gctypes.h>

/* Rom Cache stuff */
// Note: All length/size/offsets are in bytes

void ROMCache_init(fileBrowser_file*);
void ROMCache_deinit();
void ROMCache_read(u32* ram_dest, u32 rom_offset, u32 length);
int ROMCache_load(fileBrowser_file* file);

/* Byteswapping stuff */
extern int ROM_byte_swap;
void byte_swap(char* buffer, unsigned int length);
#define BYTE_SWAP_BAD -1
#define BYTE_SWAP_NONE 0
#define BYTE_SWAP_HALF 1
#define BYTE_SWAP_BYTE 2

#define ROM_CACHE_ERROR_READ  -1
#define ROM_CACHE_INVALID_ROM -2

#endif

