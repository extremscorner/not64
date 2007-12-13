/**
 * Mupen64 - pif.c
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

#include "../config.h"
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#ifndef __WIN32__
#include "../main/winlnxdefs.h"
#else
#include <windows.h>
#endif

#include <sdcard.h>
#include <ogc/card.h>

#include "memory.h"
#include "pif.h"
#include "pif2.h"
#include "../r4300/r4300.h"
#include "../r4300/interupt.h"
#include "../main/plugin.h"
#include "../main/guifuncs.h"
#include "../main/vcr.h"
#include "Saves.h"
#include "mcbanner.h"

#ifdef USE_GUI
#include "../gui/GUI.h"
#define PRINT GUI_print
#else
#define PRINT printf
#endif

static unsigned char eeprom[0x800] __attribute__((aligned(32)));
static unsigned char mempack[4][0x8000] __attribute__((aligned(32)));
static BOOL eepromWritten = FALSE;
static BOOL mempakWritten = FALSE;
#define EEP_MC_OFFSET 0x1000

void check_input_sync(unsigned char *value);

void loadEeprom(void){
	int i;
	char* filename = malloc(strlen(savepath)+
	                        strlen(ROM_SETTINGS.goodname)+4+1);
	strcpy(filename, savepath);
	strcat(filename, ROM_SETTINGS.goodname);
	strcat(filename, ".eep");

	if(savetype & SELECTION_TYPE_SD){
		sd_file *f;
		DIR* sddir = NULL;
		
		if(SDCARD_ReadDir(filename, &sddir) > 0){
			PRINT("Loading EEPROM, please be patient...\n");
			f = SDCARD_OpenFile(filename, "rb");
	  		SDCARD_ReadFile (f, eeprom, 0x800);
	  		SDCARD_CloseFile(f);
	  		PRINT("OK\n");
	  	} else for (i=0; i<0x800; i++) eeprom[i] = 0;
	  	
	  	if(sddir) free(sddir);
	} else {
		card_file CardFile;
		int slot = (savetype & SELECTION_SLOT_B) ? CARD_SLOTB : CARD_SLOTA;
		unsigned int SectorSize = 0;
        CARD_GetSectorSize (slot, &SectorSize);
        	if(CARD_Open(slot, filename, &CardFile) != CARD_ERROR_NOFILE){
        		PRINT("Loading EEPROM, please be patient...\n");			
			CARD_Read(&CardFile, eeprom, 0x800, EEP_MC_OFFSET);
			CARD_Close(&CardFile);
			PRINT("OK\n");
        	} else for (i=0; i<0x800; i++) eeprom[i] = 0;
	}

	free(filename);
	eepromWritten = FALSE;
}

extern long long gettime();
// Note: must be called after load
void saveEeprom(void){
	if(!eepromWritten) return;
	PRINT("Please wait, saving EEPROM,\n do NOT turn off the console...\n");
	
	char* filename = malloc(strlen(savepath)+
	                        strlen(ROM_SETTINGS.goodname)+4+1);
	strcpy(filename, savepath);
	strcat(filename, ROM_SETTINGS.goodname);
	strcat(filename, ".eep");

	if(savetype & SELECTION_TYPE_SD){
		sd_file *f;
		f = SDCARD_OpenFile(filename, "wb");
	  	SDCARD_WriteFile(f, eeprom, 0x800);
	  	SDCARD_CloseFile(f);
	} else {
		card_file CardFile;
		card_stat CardStat;
		int slot = (savetype & SELECTION_SLOT_B) ? CARD_SLOTB : CARD_SLOTA;
		unsigned int SectorSize = 0;
        CARD_GetSectorSize (slot, &SectorSize);
		if(CARD_Open(slot, filename, &CardFile) == CARD_ERROR_NOFILE) {
			CARD_Create(slot, filename, SectorSize, &CardFile);
		}
		//Update card stats
        CARD_GetStatus(slot,CardFile.filenum,&CardStat);
        time_t gc_time;
        gc_time = time (NULL);
		CardStat.icon_fmt = 2;
		CardStat.icon_speed = 1;
		CardStat.banner_fmt = 0;
		CardStat.comment_addr = sizeof(mupenicon);
		CardStat.icon_addr = 0;
		char* buffer = memalign(32, 0x800 + 0x40 + sizeof(mupenicon) );
		memcpy(buffer,mupenicon,sizeof(mupenicon));
		strcpy(buffer+sizeof(mupenicon),ROM_SETTINGS.goodname);
		strcpy(buffer+0x20+sizeof(mupenicon),ctime (&gc_time));
		memcpy(buffer+EEP_MC_OFFSET,eeprom,0x800);
		CARD_SetStatus(slot,CardFile.filenum,&CardStat);
		CARD_Write(&CardFile, buffer, SectorSize, 0);
		CARD_Close(&CardFile);		
		free(buffer);
	}
	free(filename);
}

//#define DEBUG_PIF
#ifdef DEBUG_PIF
void print_pif()
{
   int i;
   for (i=0; i<(64/8); i++)
     printf("%x %x %x %x | %x %x %x %x\n",
	    PIF_RAMb[i*8+0], PIF_RAMb[i*8+1],PIF_RAMb[i*8+2], PIF_RAMb[i*8+3],
	    PIF_RAMb[i*8+4], PIF_RAMb[i*8+5],PIF_RAMb[i*8+6], PIF_RAMb[i*8+7]);
   getchar();
}
#endif

void EepromCommand(BYTE *Command)
{
   switch (Command[2])
     {
      case 0: // check
	if (Command[1] != 3)
	  {
	     Command[1] |= 0x40;
	     if ((Command[1] & 3) > 0)
	       Command[3] = 0;
	     if ((Command[1] & 3) > 1)
	       Command[4] = ROM_SETTINGS.eeprom_16kb == 0 ? 0x80 : 0xc0;
	     if ((Command[1] & 3) > 2)
	       Command[5] = 0;
	  }
	else
	  {
	     Command[3] = 0;
	     Command[4] = ROM_SETTINGS.eeprom_16kb == 0 ? 0x80 : 0xc0;
	     Command[5] = 0;
	  }
	break;
      case 4: // read
	  {
	     memcpy(&Command[4], eeprom + Command[3]*8, 8);
	  }
	break;
      case 5: // write
	  {
	     eepromWritten = TRUE;
	     memcpy(eeprom + Command[3]*8, &Command[4], 8);
	  }
	break;
      default:
	printf("unknown command in EepromCommand : %x\n", Command[2]);
     }
}

void format_mempacks()
{
   unsigned char init[] =
     {
	0x81,0x01,0x02,0x03, 0x04,0x05,0x06,0x07, 0x08,0x09,0x0a,0x0b, 0x0c,0x0d,0x0e,0x0f,
	0x10,0x11,0x12,0x13, 0x14,0x15,0x16,0x17, 0x18,0x19,0x1a,0x1b, 0x1c,0x1d,0x1e,0x1f,
	0xff,0xff,0xff,0xff, 0x05,0x1a,0x5f,0x13, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
	0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff, 0xff,0xff,0x01,0xff, 0x66,0x25,0x99,0xcd,
	0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
	0xff,0xff,0xff,0xff, 0x05,0x1a,0x5f,0x13, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
	0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff, 0xff,0xff,0x01,0xff, 0x66,0x25,0x99,0xcd,
	0xff,0xff,0xff,0xff, 0x05,0x1a,0x5f,0x13, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
	0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff, 0xff,0xff,0x01,0xff, 0x66,0x25,0x99,0xcd,
	0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
	0xff,0xff,0xff,0xff, 0x05,0x1a,0x5f,0x13, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
	0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff, 0xff,0xff,0x01,0xff, 0x66,0x25,0x99,0xcd,
	0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
	0x00,0x71,0x00,0x03, 0x00,0x03,0x00,0x03, 0x00,0x03,0x00,0x03, 0x00,0x03,0x00,0x03
     };
   int i,j;
   for (i=0; i<4; i++)
     {
	for (j=0; j<0x8000; j+=2)
	  {
	     mempack[i][j] = 0;
	     mempack[i][j+1] = 0x03;
	  }
	memcpy(mempack[i], init, 272);
     }
}

unsigned char mempack_crc(unsigned char *data)
{
   int i;
   unsigned char CRC = 0;
   for (i=0; i<=0x20; i++)
     {
	int mask;
	for (mask = 0x80; mask >= 1; mask >>= 1)
	  {
	     int xor_tap = (CRC & 0x80) ? 0x85 : 0x00;
	     CRC <<= 1;
	     if (i != 0x20 && (data[i] & mask)) CRC |= 1;
	     CRC ^= xor_tap;
	  }
     }
   return CRC;
}

void loadMempak(void){
	char* filename = malloc(strlen(savepath)+
	                        strlen(ROM_SETTINGS.goodname)+4+1);
	strcpy(filename, savepath);
	strcat(filename, ROM_SETTINGS.goodname);
	strcat(filename, ".mpk");

	if(savetype & SELECTION_TYPE_SD){
		sd_file *f;
		DIR* sddir = NULL;
		
		if(SDCARD_ReadDir(filename, &sddir) > 0){
			PRINT("Loading mempak, please be patient...\n");
			f = SDCARD_OpenFile(filename, "rb");
			SDCARD_ReadFile (f, mempack[0], 0x8000);
			SDCARD_ReadFile (f, mempack[1], 0x8000);
			SDCARD_ReadFile (f, mempack[2], 0x8000);
			SDCARD_ReadFile (f, mempack[3], 0x8000);
			SDCARD_CloseFile(f);
			PRINT("OK\n");
		} else format_mempacks();
		
	  	if(sddir) free(sddir);
	} else {
		card_file CardFile;
		int slot = (savetype & SELECTION_SLOT_B) ? CARD_SLOTB : CARD_SLOTA;
		
		if(CARD_Open(slot, filename, &CardFile) != CARD_ERROR_NOFILE){
			PRINT("Loading mempak, please be patient...\n");
			CARD_Read (&CardFile, mempack[0], 0x8000, 0);
			CARD_Read (&CardFile, mempack[1], 0x8000, 0x8000);
			CARD_Read (&CardFile, mempack[2], 0x8000, 0x8000*2);
			CARD_Read (&CardFile, mempack[3], 0x8000, 0x8000*3);
			CARD_Close(&CardFile);
			PRINT("OK\n");
		} else format_mempacks();
	}

	free(filename);
	mempakWritten = FALSE;
}

void saveMempak(void){
	if(!mempakWritten) return;
	PRINT("Please wait, saving mempak,\n do NOT turn off the console...\n");

	char* filename = malloc(strlen(savepath)+
	                        strlen(ROM_SETTINGS.goodname)+4+1);
	strcpy(filename, savepath);
	strcat(filename, ROM_SETTINGS.goodname);
	strcat(filename, ".mpk");
	
	if(savetype & SELECTION_TYPE_SD){
		sd_file *f;
		
		f = SDCARD_OpenFile(filename, "wb");
	  	SDCARD_WriteFile(f, mempack[0], 0x8000);
	  	SDCARD_WriteFile(f, mempack[1], 0x8000);
	  	SDCARD_WriteFile(f, mempack[2], 0x8000);
	  	SDCARD_WriteFile(f, mempack[3], 0x8000);
	  	SDCARD_CloseFile(f);
	} else {
		card_file CardFile;
		int slot = (savetype & SELECTION_SLOT_B) ? CARD_SLOTB : CARD_SLOTA;
		
		if(CARD_Open(slot, filename, &CardFile) == CARD_ERROR_NOFILE)
			CARD_Create(slot, filename, 0x8000*4, &CardFile);			
		CARD_Write(&CardFile, mempack[0], 0x8000, 0);
		CARD_Write(&CardFile, mempack[1], 0x8000, 0x8000);
		CARD_Write(&CardFile, mempack[2], 0x8000, 0x8000*2);
		CARD_Write(&CardFile, mempack[3], 0x8000, 0x8000*3);
		CARD_Close(&CardFile);
	}
	
	free(filename);
	PRINT("OK\n");
}

void internal_ReadController(int Control, BYTE *Command)
{
   switch (Command[2])
     {
      case 1:
	if (Controls[Control].Present)
	  {
	     BUTTONS Keys;
#ifdef VCR_SUPPORT
	     VCR_getKeys(Control, &Keys);
#else
	     getKeys(Control, &Keys);
#endif
	     *((unsigned long *)(Command + 3)) = Keys.Value;
#ifdef COMPARE_CORE
	     check_input_sync(Command+3);
#endif
	  }
	break;
      case 2: // read controller pack
	if (Controls[Control].Present)
	  {
	     if (Controls[Control].Plugin == PLUGIN_RAW)
	       if (controllerCommand) readController(Control, Command);
	  }
	break;
      case 3: // write controller pack
	if (Controls[Control].Present)
	  {
	     if (Controls[Control].Plugin == PLUGIN_RAW)
	       if (controllerCommand) readController(Control, Command);
	  }
	break;
     }
}

void internal_ControllerCommand(int Control, BYTE *Command)
{
   switch (Command[2])
     {
      case 0x00: // check
      case 0xFF:
	if ((Command[1] & 0x80))
	  break;
	if (Controls[Control].Present)
	  {
	     Command[3] = 0x05;
	     Command[4] = 0x00;
	     switch(Controls[Control].Plugin)
	       {
		case PLUGIN_MEMPAK:
		  Command[5] = 1;
		  break;
		case PLUGIN_RAW:
		  Command[5] = 1;
		  break;
		default:
		  Command[5] = 0;
		  break;
	       }
	  }
	else
	  Command[1] |= 0x80;
	break;
      case 0x01:
	if (!Controls[Control].Present)
	  Command[1] |= 0x80;
	break;
      case 0x02: // read controller pack
	if (Controls[Control].Present)
	  {
	     switch(Controls[Control].Plugin)
	       {
		case PLUGIN_MEMPAK:
		    {
		       int address = (Command[3] << 8) | Command[4];
		       if (address == 0x8001)
			 {
			    memset(&Command[5], 0, 0x20);
			    Command[0x25] = mempack_crc(&Command[5]);
			 }
		       else
			 {
			    address &= 0xFFE0;
			    if (address <= 0x7FE0)
			      {
				 	 memcpy(&Command[5], &mempack[Control][address], 0x20);
			      }
			    else
			      {
				 memset(&Command[5], 0, 0x20);
			      }
			    Command[0x25] = mempack_crc(&Command[5]);
			 }
		    }
		  break;
		case PLUGIN_RAW:
		  if (controllerCommand) controllerCommand(Control, Command);
		  break;
		default:
		  memset(&Command[5], 0, 0x20);
		  Command[0x25] = 0;
	       }
	  }
	else
	  Command[1] |= 0x80;
	break;
      case 0x03: // write controller pack
	if (Controls[Control].Present)
	  {
	     switch(Controls[Control].Plugin)
	       {
		case PLUGIN_MEMPAK:
		    {
		       int address = (Command[3] << 8) | Command[4];
		       if (address == 0x8001) 
			 Command[0x25] = mempack_crc(&Command[5]);
		       else
			 {
			    address &= 0xFFE0;
			    if (address <= 0x7FE0)
			      {
	                 mempakWritten = TRUE;
					 memcpy(&mempack[Control][address], &Command[5], 0x20);
				 	 Command[0x25] = mempack_crc(&Command[5]);
			 }
		    }
		  break;
		case PLUGIN_RAW:
		  if (controllerCommand) controllerCommand(Control, Command);
		  break;
		default:
		  Command[0x25] = mempack_crc(&Command[5]);
	       }
       }
	  }
	else
	  Command[1] |= 0x80;
	break;
     }
}

void update_pif_write()
{
   int i=0, channel=0;
#ifdef DEBUG_PIF
   printf("write\n");
   print_pif();
#endif
   if (PIF_RAMb[0x3F] > 1)
     {
	switch (PIF_RAMb[0x3F])
	  {
	   case 0x02:
	     for (i=0; i<sizeof(pif2_lut)/32; i++)
	       {
		  if (!memcmp(PIF_RAMb + 64-2*8, pif2_lut[i][0], 16))
		    {
		       memcpy(PIF_RAMb + 64-2*8, pif2_lut[i][1], 16);
		       return;
		    }
	       }
	     printf("unknown pif2 code:\n");
	     for (i=(64-2*8)/8; i<(64/8); i++)
	       printf("%x %x %x %x | %x %x %x %x\n",
		      PIF_RAMb[i*8+0], PIF_RAMb[i*8+1],PIF_RAMb[i*8+2], PIF_RAMb[i*8+3],
		      PIF_RAMb[i*8+4], PIF_RAMb[i*8+5],PIF_RAMb[i*8+6], PIF_RAMb[i*8+7]);
	     break;
	   case 0x08:
	     PIF_RAMb[0x3F] = 0;
	     break;
	   default:
	     printf("error in update_pif_write : %x\n", PIF_RAMb[0x3F]);
	  }
	return;
     }
   while (i<0x40)
     {
	switch(PIF_RAMb[i])
	  {
	   case 0x00:
	     channel++;
	     if (channel > 6) i=0x40;
	     break;
	   case 0xFF:
	     break;
	   default:
	     if (!(PIF_RAMb[i] & 0xC0))
	       {
		  if (channel < 4)
		    {
		       if (Controls[channel].Present && 
			   Controls[channel].RawData)
			 controllerCommand(channel, &PIF_RAMb[i]);
		       else
			 internal_ControllerCommand(channel, &PIF_RAMb[i]);
		    }
		  else if (channel == 4)
		    EepromCommand(&PIF_RAMb[i]);
		  else
		    printf("channel >= 4 in update_pif_write\n");
		  i += PIF_RAMb[i] + (PIF_RAMb[(i+1)] & 0x3F) + 1;
		  channel++;
	       }
	     else
	       i=0x40;
	  }
	i++;
     }
   //PIF_RAMb[0x3F] = 0;
   controllerCommand(-1, NULL);
#ifdef DEBUG_PIF
   print_pif();
#endif
}

void update_pif_read()
{
   int i=0, channel=0;
#ifdef DEBUG_PIF
   printf("read\n");
   print_pif();
#endif
   while (i<0x40)
     {
	switch(PIF_RAMb[i])
	  {
	   case 0x00:
	     channel++;
	     if (channel > 6) i=0x40;
	     break;
	   case 0xFE:
	     i = 0x40;
	     break;
	   case 0xFF:
	     break;
	   case 0xB4:
	   case 0x56:
	   case 0xB8:
	     break;
	   default:
	     if (!(PIF_RAMb[i] & 0xC0))
	       {
		  if (channel < 4)
		    {
		       if (Controls[channel].Present && 
			   Controls[channel].RawData)
			 readController(channel, &PIF_RAMb[i]);
		       else
			 internal_ReadController(channel, &PIF_RAMb[i]);
		    }
		  i += PIF_RAMb[i] + (PIF_RAMb[(i+1)] & 0x3F) + 1;
		  channel++;
	       }
	     else
	       i=0x40;
	  }
	i++;
     }
   readController(-1, NULL);
#ifdef DEBUG_PIF
   print_pif();
#endif
}
