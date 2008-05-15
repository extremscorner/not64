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
//static int i, tmp, z;
 
int rom_length;
unsigned char* rom;
int ROM_byte_swap;
rom_header* ROM_HEADER = NULL;
rom_settings ROM_SETTINGS;


static int detectByteSwapping(void){
	if(!rom_file) return BYTE_SWAP_BAD;
	
	unsigned char magicWord[4];
	
	romFile_seekFile(rom_file, 0, FILE_BROWSER_SEEK_SET);
	romFile_readFile(rom_file, magicWord, 4);
	
	switch(magicWord[0]){
	case 0x37:					//37804012 aka byteswapped
		return BYTE_SWAP_BYTE;
	case 0x40:					//40123780 aka little endian, aka halfswapped
		return BYTE_SWAP_HALF;
	case 0x80:
		return BYTE_SWAP_NONE;
	default:
		return BYTE_SWAP_BAD;
	}
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

//int rom_read(const char *argv){
int rom_read(fileBrowser_file* file){
//   md5_state_t state;
//  md5_byte_t digest[16];
   //mupenEntry *entry;
   //char arg[1024];
   //strncpy(arg, argv, 1000);
   char buffer[1024];
   rom_file = file;
   rom_length = file->size;
   
   CLEAR();
   sprintf(buffer, "Loading ROM: %s, please be patient...\n", file->name);
   PRINT(buffer);
   ROMCache_init(rom_length);
   ROMCache_load(rom_file, detectByteSwapping());
   CLEAR();
  
   if(!ROM_HEADER) ROM_HEADER = malloc(sizeof(rom_header));
   ROMCache_read(ROM_HEADER, 0, sizeof(rom_header));
   
   //display_loading_progress(100);
   sprintf(buffer, "ROM (%s) loaded succesfully\n", ROM_HEADER->nom);
   PRINT(buffer);
#if 0
   sprintf(buffer, "%x %x %x %x\n", ROM_HEADER->init_PI_BSB_DOM1_LAT_REG,
	   ROM_HEADER->init_PI_BSB_DOM1_PGS_REG,
	   ROM_HEADER->init_PI_BSB_DOM1_PWD_REG,
	   ROM_HEADER->init_PI_BSB_DOM1_PGS_REG2);
   PRINT(buffer);
   sprintf(buffer, "ClockRate=%x\n", sl((unsigned int)ROM_HEADER->ClockRate));
   PRINT(buffer);
   sprintf(buffer, "Version:%x\n", sl((unsigned int)ROM_HEADER->Release));
   PRINT(buffer);
   sprintf(buffer, "CRC: %x %x\n", sl((unsigned int)ROM_HEADER->CRC1), sl((unsigned int)ROM_HEADER->CRC2));
   PRINT(buffer);
   sprintf(buffer, "name: %s\n", ROM_HEADER->nom);
   PRINT(buffer);
   if (sl(ROM_HEADER->Manufacturer_ID) == 'N') PRINT("Manufacturer: Nintendo\n");
   else { sprintf(buffer, "Manufacturer: %x\n", (unsigned int)(ROM_HEADER->Manufacturer_ID)); PRINT(buffer); }
   sprintf(buffer, "Cartridge_ID: %x\n", ROM_HEADER->Cartridge_ID);
   PRINT(buffer);
   
   // Swap country code since I know it's used
   char temp = ((char*)&ROM_HEADER->Country_code)[0];
   ((char*)&ROM_HEADER->Country_code)[0] = ((char*)&ROM_HEADER->Country_code)[1];
   ((char*)&ROM_HEADER->Country_code)[1] = temp;
   
   switch(ROM_HEADER->Country_code)
     {
      case 0x0044:
	PRINT("Country : Germany\n");
	break;
      case 0x0045:
	PRINT("Country : United States\n");
	break;
      case 0x004A:
	PRINT("Country : Japan\n");
	break;
      case 0x0050:
	PRINT("European cartridge\n");
	break;
      case 0x0055:
	PRINT("Country : Australie\n");
      default:
	sprintf(buffer, "Country Code : %x\n", ROM_HEADER->Country_code);
	PRINT(buffer);
     }
   sprintf(buffer, "size: %d\n", (unsigned int)(sizeof(rom_header)));
   PRINT(buffer);
   sprintf(buffer, "PC= %x\n", sl((unsigned int)ROM_HEADER->PC));
   PRINT(buffer);
#endif   
   // FIXME: ROM_SETTINGS.goodname needs to be filled out
   strcpy(ROM_SETTINGS.goodname, ROM_HEADER->nom);
   
/*#if 0 // I don't think I can/need to support this yet
   // loading rom settings and checking if it's a good dump
   md5_init(&state);
   md5_append(&state, (const md5_byte_t *)rom, rom_length);
   md5_finish(&state, digest);
   printf("md5 code:");
   for (i=0; i<16; i++) printf("%02X", digest[i]);
   printf("\n");
   
   ini_openFile();
   
   for (i=0; i<16; i++) sprintf(arg+i*2, "%02X", digest[i]);
   arg[32] = 0;
   strcpy(ROM_SETTINGS.MD5, arg);
   if ((entry = ini_search_by_md5(arg)) == NULL)
     {
	char mycrc[1024];
	printf("%x\n", (int)entry);
	sprintf(mycrc, "%08X-%08X-C%02X",
		(int)sl(ROM_HEADER->CRC1), (int)sl(ROM_HEADER->CRC2),
		ROM_HEADER->Country_code);
	if ((entry = ini_search_by_CRC(mycrc)) == NULL)
	  {
	     strcpy(ROM_SETTINGS.goodname, ROM_HEADER->nom);
	     strcat(ROM_SETTINGS.goodname, " (unknown rom)");
	     printf("%s\n", ROM_SETTINGS.goodname);
	     ROM_SETTINGS.eeprom_16kb = 0;
	     return 0;
	  }
	else
	  {
	     if (!ask_bad())
	       {
		  free(rom);
		  rom = NULL;
		  free(ROM_HEADER);
		  ROM_HEADER = NULL;
		  return 1;
	       }
	     strcpy(ROM_SETTINGS.goodname, entry->goodname);
	     strcat(ROM_SETTINGS.goodname, " (bad dump)");
	     if (strcmp(entry->refMD5, ""))
	       entry = ini_search_by_md5(entry->refMD5);
	     ROM_SETTINGS.eeprom_16kb = entry->eeprom16kb;
	     return 0;
	  }
     }
   s=entry->goodname;
   for (i=strlen(s); i > 0 && s[i-1] != '['; i--);
   if (i != 0)
     {
	if (s[i] == 'T' || s[i] == 't' || s[i] == 'h' || s[i] == 'f' || s[i] == 'o')
	  {
	     if (!ask_hack())
	       {
		  free(rom);
		  rom = NULL;
		  free(ROM_HEADER);
		  ROM_HEADER = NULL;
		  return 1;
	       }
	  }
	if (s[i] == 'b')
	  {
	     if (!ask_bad())
	       {
		  free(rom);
		  rom = NULL;
		  free(ROM_HEADER);
		  ROM_HEADER = NULL;
		  return 1;
	       }
	  }
     }
   strcpy(ROM_SETTINGS.goodname, entry->goodname);
   
   if (strcmp(entry->refMD5, ""))
     entry = ini_search_by_md5(entry->refMD5);
   ROM_SETTINGS.eeprom_16kb = entry->eeprom16kb;
   printf("eeprom type:%d\n", ROM_SETTINGS.eeprom_16kb);
#endif
   */
   return 0;
}
/*
int fill_header(const char *argv)
{
   char arg[1024];
   strncpy(arg, argv, 1000);
   rom_file = SDCARD_OpenFile(argv, "rb");
   if(!rom_file){
	printf ("file not found or wrong path\n");
	return 0;
   }
 
   findsize();
   if (rom) free(rom);
   rom = malloc(0x40);
   
   tmp=0;
   
   SDCARD_ReadFile(rom_file, rom, 0x40);

   SDCARD_CloseFile(rom_file);
   
   if (rom[0]==0x37)
     {
	for (i=0; i<(0x40/2); i++)
	  {
	     tmp=rom[i*2];
	     rom[i*2]=rom[i*2+1];
	     rom[i*2+1]=tmp;
	  }
     }
   if (rom[0]==0x40)
     {
	for (i=0; i<(0x40/4); i++)
	  {
	     tmp=rom[i*4];
	     rom[i*4]=rom[i*4+3];
	     rom[i*4+3]=tmp;
	     tmp=rom[i*4+1];
	     rom[i*4+1]=rom[i*4+2];
	     rom[i*4+2]=tmp;
	  }
     }
   else if ((rom[0] != 0x80) || (rom[1] != 0x37) || (rom[2] != 0x12) || (rom[3] != 0x40))
     {
	free(rom);
	rom = NULL;
	return 0;
     }
   
   if (ROM_HEADER == NULL) ROM_HEADER= malloc(sizeof(rom_header));
   memcpy(ROM_HEADER, rom, 0x40);
   free(rom);
   rom = NULL;
   return rom_length;
}

void calculateMD5(const char *argv, unsigned char digest[16])
{
   md5_state_t state;
   char arg[1024];
   
   strncpy(arg, argv, 1000);
   rom_file = SDCARD_OpenFile(arg, "rb");
   if(!rom_file){
	printf("file not found or wrong path\n");
	return;
   }

   findsize();
   if (rom) free(rom);
   rom = malloc(rom_length);
   
   for (i=0; i<rom_length;i+=SDCARD_ReadFile(rom_file, rom+i, 1000)){
   	if (tmp!=(int)((i/(float)rom_length)*100)){
   		tmp=(int)(i/(float)(rom_length)*100);
   		display_MD5calculating_progress(tmp);
   	}
   }
  
   SDCARD_CloseFile(rom_file);
   
   display_MD5calculating_progress(100);
   
   if (rom[0]==0x37)
     {
	printf ("byteswaping rom...\n");
	for (i=0; i<(rom_length/2); i++)
	  {
	     tmp=rom[i*2];
	     rom[i*2]=rom[i*2+1];
	     rom[i*2+1]=tmp;
	  }
	printf ("rom byteswaped\n");
     }
   if (rom[0]==0x40)
     {
	for (i=0; i<(rom_length/4); i++)
	  {
	     tmp=rom[i*4];
	     rom[i*4]=rom[i*4+3];
	     rom[i*4+3]=tmp;
	     tmp=rom[i*4+1];
	     rom[i*4+1]=rom[i*4+2];
	     rom[i*4+2]=tmp;
	  }
	printf("rom byteswaped\n");
     }
   else if ((rom[0] != 0x80) || (rom[1] != 0x37) || (rom[2] != 0x12) || (rom[3] != 0x40))
     {
	printf("wrong file format !\n");
	free(rom);
	rom = NULL;
	return;
     }
   md5_init(&state);
   md5_append(&state, (const md5_byte_t *)rom, rom_length);
   md5_finish(&state, digest);
   display_MD5calculating_progress(-1);
   free(rom);
   rom = NULL;
}
*/

