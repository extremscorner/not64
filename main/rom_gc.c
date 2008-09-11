/**
 * Mupen64 - rom_gc.c
 * Copyright (C) 2002 Hacktarux, 
 * Wii/GC Additional code by tehpola, emu_kidid
 *
 * Mupen64 homepage: http://mupen64.emulation64.com
 * email address: hacktarux@yahoo.fr
 * 
 * If you want to contribute to the project please contact
 * me first (maybe someone is already making what you are
 * planning to do).
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
 * You should have received a copy of the GNU General Public
 * Licence along with this program; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139,
 * USA.
 *
**/

/* This is the functions that load a rom into memory, it loads a roms
 * in multiple formats, gzipped or not. It searches the rom, in the roms
 * subdirectory or in the path specified in the path.cfg file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "rom.h"
#include "gc_dvd.h"
#include "ROM-Cache.h" 
#include "../gc_memory/memory.h"
#include "md5.h"

#include "../fileBrowser/fileBrowser.h"

#ifdef USE_GUI
#include "../gui/GUI.h"
#define PRINT GUI_print
#define CLEAR()
#else
#define PRINT printf
#define CLEAR() GUI_clear()
#endif

static fileBrowser_file* rom_file;
 
int rom_length;
unsigned char* rom;
int ROM_byte_swap;
rom_header* ROM_HEADER = NULL;
rom_settings ROM_SETTINGS;

int init_byte_swap(u32 magicWord){
	
	switch(magicWord >> 24){
	case 0x37:					//37804012 aka byteswapped
		ROM_byte_swap = BYTE_SWAP_BYTE;
		break;
	case 0x40:					//40123780 aka little endian, aka halfswapped
		ROM_byte_swap = BYTE_SWAP_HALF;
		break;
	case 0x80:
		ROM_byte_swap = BYTE_SWAP_NONE;
		break;
	default:
		ROM_byte_swap = BYTE_SWAP_BAD;
		break;
	}
	return ROM_byte_swap;
}

void byte_swap(char* buffer, unsigned int length){
	if(ROM_byte_swap == BYTE_SWAP_NONE || ROM_byte_swap == BYTE_SWAP_BAD)
		return;
	
	int i = 0;
	u8 aByte = 0;
	u16 aShort = 0;
	u16 *buffer_short = (unsigned short*)buffer;
	
	if(ROM_byte_swap == BYTE_SWAP_HALF){	//aka little endian (40123780) vs (80371240)
		for(i=0; i<length; i+=2) 	//get it from (40123780) to (12408037)
		{
			aByte 		= buffer[i];
			buffer[i] 	= buffer[i+1];
			buffer[i+1] = aByte;
		}
		for(i=0; i<length/2; i+=2)	//get it from (12408037) to (80371240)
		{ 
			aShort        		= buffer_short[i];
			buffer_short[i]   	= buffer_short[i+1];
			buffer_short[i+1] 	= aShort;
		}
	} else if(ROM_byte_swap == BYTE_SWAP_BYTE){	// (37804012) vs (80371240)
		for(i=0; i<length; i+=2){
			aByte 		= buffer[i];
			buffer[i] 	= buffer[i+1];
			buffer[i+1] = aByte;
		}
	}
}

int rom_read(fileBrowser_file* file){

   char buffer[1024];
   rom_file = file;
   rom_length = file->size;
   
   CLEAR();
   sprintf(buffer, "Loading ROM: %s, please be patient...\n", file->name);
   PRINT(buffer);
   
   // FIXME: This should be the same once zip support is added to GC
#ifdef WII   
   ROMCache_init(rom_file);
   int ret = ROMCache_load(rom_file);
#else
   int ret = 0,magicByte=0;
   ROMCache_init(rom_length);
   romFile_readFile(rom_file, &magicByte, 4);
   romFile_seekFile(rom_file, 0, FILE_BROWSER_SEEK_SET);
   ROMCache_load(rom_file,init_byte_swap(magicByte));
#endif

   CLEAR();
   if(!ROM_HEADER) ROM_HEADER = malloc(sizeof(rom_header));
   ROMCache_read((u32*)ROM_HEADER, 0, sizeof(rom_header));

   sprintf(buffer, "ROM (%s) loaded succesfully\n", ROM_HEADER->nom);
   PRINT(buffer);

   // Swap country code since I know it's used
   char temp = ((char*)&ROM_HEADER->Country_code)[0];
   ((char*)&ROM_HEADER->Country_code)[0] = ((char*)&ROM_HEADER->Country_code)[1];
   ((char*)&ROM_HEADER->Country_code)[1] = temp;

   // FIXME: ROM_SETTINGS.goodname needs to be filled out
   strcpy(ROM_SETTINGS.goodname, (char*)ROM_HEADER->nom);
   return ret;
}



