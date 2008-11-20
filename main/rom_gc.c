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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "rom.h"
#include "ROM-Cache.h" 
#include "../gc_memory/memory.h"
#include "../fileBrowser/fileBrowser.h"
#include "../gui/GUI.h"

#define PRINT GUI_print

unsigned char* rom;
static fileBrowser_file* rom_file;
int rom_length;
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

/* Loads the ROM into the ROM cache */
int rom_read(fileBrowser_file* file){

   char buffer[1024];
   int i;
   rom_file = file;
   rom_length = file->size;
   
   /*sprintf(buffer, "Loading ROM: %s, please be patient...\n", file->name);
   PRINT(buffer);*/
   
   ROMCache_init(rom_file);
   int ret = ROMCache_load(rom_file);

   if(!ROM_HEADER) ROM_HEADER = malloc(sizeof(rom_header));
   ROMCache_read((u32*)ROM_HEADER, 0, sizeof(rom_header));

   /*sprintf(buffer, "ROM (%s) loaded succesfully\n", ROM_HEADER->nom);
   PRINT(buffer);*/

   // Swap country code since I know it's used
   char temp = ((char*)&ROM_HEADER->Country_code)[0];
   ((char*)&ROM_HEADER->Country_code)[0] = ((char*)&ROM_HEADER->Country_code)[1];
   ((char*)&ROM_HEADER->Country_code)[1] = temp;
  
  //Copy header name as Goodname (in the .ini we can use CRC to identify ROMS)
  memset((char*)buffer,0,1024);
  strcpy(buffer, (char*)ROM_HEADER->nom);
  //Maximum ROM name is 32 bytes. Lets make sure we cut off trailing spaces
  for(i = strlen(buffer); i>0; i--)
  {
    if(buffer[i-1] !=  ' ') {
  		strncpy(&ROM_SETTINGS.goodname[0],&buffer[0],i);
  		ROM_SETTINGS.goodname[i] = 0; //terminate it too
  		break;
    }
  }

   return ret;
}



