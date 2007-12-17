/**
 * Mupen64 - flashram.c
 * Copyright (C) 2002 Hacktarux
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

#include "../r4300/r4300.h"
#include "memory.h"
#include "../main/guifuncs.h"
#include "../gui/DEBUG.h"
#include <sdcard.h>
#include <ogc/card.h>
#include "Saves.h"

#ifdef USE_GUI
#include "../gui/GUI.h"
#define PRINT GUI_print
#else
#define PRINT printf
#endif

int use_flashram;

typedef enum flashram_mode
{
   NOPES_MODE = 0,
   ERASE_MODE,
   WRITE_MODE,
   READ_MODE,
   STATUS_MODE
} Flashram_mode;

static int mode;
static unsigned long long status;
static unsigned char flashram[0x20000] __attribute__((aligned(32)));
static unsigned long erase_offset, write_pointer;

static BOOL flashramWritten = FALSE;

void loadFlashram(void){
	int i; 
	char* filename = malloc(strlen(savepath)+
	                        strlen(ROM_SETTINGS.goodname)+4+1);
	strcpy(filename, savepath);
	strcat(filename, ROM_SETTINGS.goodname);
	strcat(filename, ".fla");
	
	if(savetype & SELECTION_TYPE_SD){
		DIR* sddir = NULL;
		sd_file *f;
		
		if (SDCARD_ReadDir(filename, &sddir) > 0){
			PRINT("Loading flashram, please be patient...\n");
			f = SDCARD_OpenFile(filename, "rb");
			SDCARD_ReadFile (f, flashram, 0x20000);
			SDCARD_CloseFile(f);
			PRINT("OK\n");
		} else for (i=0; i<0x20000; i++) flashram[i] = 0xff;
		
		if(sddir) free(sddir);
	} else {
		card_file CardFile;
		int slot = (savetype & SELECTION_SLOT_B) ? CARD_SLOTB : CARD_SLOTA;
		
		if(CARD_Open(slot, filename, &CardFile) != CARD_ERROR_NOFILE){
			PRINT("Loading flashram, please be patient...\n");			
			CARD_Read (&CardFile, flashram, 0x20000, 0);
			CARD_Close(&CardFile);
			PRINT("OK\n");
		} else for (i=0; i<0x20000; i++) flashram[i] = 0xff;
	}
	
	free(filename);
	flashramWritten = FALSE;
}

void saveFlashram(void){
	if(!flashramWritten) return;
	PRINT("Please wait, saving flashram,\n do NOT turn off the console...\n");
	
	char* filename = malloc(strlen(savepath)+
	                        strlen(ROM_SETTINGS.goodname)+4+1);
	strcpy(filename, savepath);
	strcat(filename, ROM_SETTINGS.goodname);
	strcat(filename, ".fla");
	
	if(savetype & SELECTION_TYPE_SD){
		sd_file* f;
		
		f = SDCARD_OpenFile(filename, "wb");
		SDCARD_WriteFile(f, flashram, 0x20000);
		SDCARD_CloseFile(f);
	} else {
		card_file CardFile;
		int slot = (savetype & SELECTION_SLOT_B) ? CARD_SLOTB : CARD_SLOTA;
		
		if(CARD_Open(slot, filename, &CardFile) == CARD_ERROR_NOFILE)
			CARD_Create(slot, filename, 0x20000, &CardFile);
		CARD_Write(&CardFile, flashram, 0x20000, 0);
		CARD_Close(&CardFile);
	}
	
	free(filename);
	PRINT("OK\n");
}

void save_flashram_infos(char *buf)
{
   memcpy(buf+0 , &use_flashram , 4);
   memcpy(buf+4 , &mode         , 4);
   memcpy(buf+8 , &status       , 8);
   memcpy(buf+16, &erase_offset , 4);
   memcpy(buf+20, &write_pointer, 4);
}

void load_flashram_infos(char *buf)
{
   memcpy(&use_flashram , buf+0 , 4);
   memcpy(&mode         , buf+4 , 4);
   memcpy(&status       , buf+8 , 8);
   memcpy(&erase_offset , buf+16, 4);
   memcpy(&write_pointer, buf+20, 4);
}

void init_flashram()
{
   mode = NOPES_MODE;
   status = 0;
}

unsigned long flashram_status()
{
   return (status >> 32);
}

void flashram_command(unsigned long command)
{
   switch(command & 0xff000000)
     {
      case 0x4b000000:
	erase_offset = (command & 0xffff) * 128;
	break;
      case 0x78000000:
	mode = ERASE_MODE;
	status = 0x1111800800c20000LL;
	break;
      case 0xa5000000:
	erase_offset = (command & 0xffff) * 128;
	status = 0x1111800400c20000LL;
	break;
      case 0xb4000000:
	mode = WRITE_MODE;
	break;
      case 0xd2000000:  // execute
	switch (mode)
	  {
	   case NOPES_MODE:
	     break;
	   case ERASE_MODE:
	       {
		  int i;
		  flashramWritten = TRUE;
		  
		  for (i=erase_offset; i<(erase_offset+128); i++)
		    flashram[i^S8] = 0xff;
 
	       }
	     break;
	   case WRITE_MODE:
	       {
	       	  	int i;
		  		flashramWritten = TRUE;
                  
		  for (i=0; i<128; i++)
		    flashram[(erase_offset+i)^S8]=
		    ((unsigned char*)rdram)[(write_pointer+i)^S8];

	       }
	     break;
	   case STATUS_MODE:
	     break;
	   default:
	     printf("unknown flashram command with mode:%x\n", (int)mode);
	     stop=1;
	  }
	mode = NOPES_MODE;
	break;
      case 0xe1000000:
	mode = STATUS_MODE;
	status = 0x1111800100c20000LL;
	break;
      case 0xf0000000:
	mode = READ_MODE;
	status = 0x11118004f0000000LL;
	break;
      default:
	printf("unknown flashram command:%x\n", (int)command);
	//stop=1;
     }
}

void dma_read_flashram()
{
   int i;

   switch(mode)
     {
      case STATUS_MODE:
      DEBUG_print("Get Status FlashRAM",DBG_SAVEINFO);
	rdram[pi_register.pi_dram_addr_reg/4] = (unsigned long)(status >> 32);
	rdram[pi_register.pi_dram_addr_reg/4+1] = (unsigned long)(status);
	break;
      case READ_MODE:
      DEBUG_print("Read FlashRAM",DBG_SAVEINFO);

	for (i=0; i<(pi_register.pi_wr_len_reg & 0x0FFFFFF)+1; i++)
	  ((unsigned char*)rdram)[(pi_register.pi_dram_addr_reg+i)^S8]=
	  flashram[(((pi_register.pi_cart_addr_reg-0x08000000)&0xFFFF)*2+i)^S8];
	break;
      default:	
	printf("unknown dma_read_flashram:%x\n", mode);
	stop=1;
     }
}

void dma_write_flashram()
{
   switch(mode)
     {
      case WRITE_MODE:
      DEBUG_print("Write FlashRAM",DBG_SAVEINFO);
	write_pointer = pi_register.pi_dram_addr_reg;
	break;
      default:
	printf("unknown dma_read_flashram:%x\n", mode);
	stop=1;
     }
}
