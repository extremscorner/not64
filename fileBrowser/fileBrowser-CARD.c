/**
 * Wii64 - fileBrowser-CARD.c
 * Copyright (C) 2007, 2008, 2009 emu_kidid
 * 
 * fileBrowser module for Nintendo Gamecube Memory Cards
 *
 * Wii64 homepage: http://www.emulatemii.com
 * email address:  emukidid@gmail.com
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
#include <gccore.h>
#include <malloc.h>
#include <ogc/card.h>
#include "fileBrowser.h"
#include "imagedata/mupenIcon.h"  //32x32 icon

unsigned char *SysArea = NULL;
void card_removed_cb(s32 chn, s32 result){ CARD_Unmount(chn); }

fileBrowser_file topLevel_CARD_SlotA =
	{ "\0", // file name
	  CARD_SLOTA, // slot
	  0, // offset
	  0, // size
	  FILE_BROWSER_ATTR_DIR
	 };

fileBrowser_file topLevel_CARD_SlotB =
	{ "\0", // file name
	  CARD_SLOTB, // slot
	  0, // offset
	  0, // size
	  FILE_BROWSER_ATTR_DIR
	 };
	 
int mount_card(int slot) {
  /* Pass company identifier and number */
  CARD_Init ("N64E", "OS");
	if(!SysArea) SysArea = memalign(32,CARD_WORKAREA);
  int Slot_error = CARD_Mount (slot, SysArea, card_removed_cb);
    
  /* Lets try 50 times to mount it. Sometimes it takes a while */
  if (Slot_error < 0) {
   	int i = 0;
   	for(i = 0; i<50; i++)
   		Slot_error = CARD_Mount (slot, SysArea, card_removed_cb);
	}
	return Slot_error;
}

int fileBrowser_CARD_readDir(fileBrowser_file* file, fileBrowser_file** dir){
  return 0; /* Not required, use IPL to delete/copy files */
}

int fileBrowser_CARD_seekFile(fileBrowser_file* file, unsigned int where, unsigned int type){
	if(type == FILE_BROWSER_SEEK_SET) file->offset = where;
	else if(type == FILE_BROWSER_SEEK_CUR) file->offset += where;
	else file->offset = file->size + where;
	
	return 0;
}

int fileBrowser_CARD_readFile(fileBrowser_file* file, void* buffer, unsigned int length){
	char *tbuffer;
	card_file CardFile;
	int slot = file->discoffset;
	unsigned int SectorSize = 0;
  CARD_GetSectorSize (slot, &SectorSize);
    
	if(CARD_Open(slot, (const char*)file->name, &CardFile) != CARD_ERROR_NOFILE){
	  /* Allocate a temporary buffer */
  	tbuffer = memalign(32,CardFile.len);
	  /* Read the file */
		if(CARD_Read(&CardFile,tbuffer, CardFile.len, 0) == 0){
			file->offset += length;
			memcpy(buffer,tbuffer+0x40+sizeof(CARDIcon),length);
		}
		else            /* card removed or read failed */
		{
  		if(tbuffer)
		    free(tbuffer);
      CARD_Close(&CardFile);
      return -1;
		}
		if(tbuffer)
		  free(tbuffer);
    return length;  /* success! */
  }
  return -1;        /* file doesn't exist or other error */
}

int fileBrowser_CARD_writeFile(fileBrowser_file* file, void* buffer, unsigned int length){
	
	card_file CardFile;
	card_stat CardStat;
	int slot = file->discoffset;
	char *tmpBuffer = NULL;
	unsigned int status,SectorSize,newLength;
	
	newLength = length + sizeof(CARDIcon) + 0x40;
	
	CARD_GetSectorSize (slot, &SectorSize);

	/* Round the size up to the SectorSize if needed, else leave it */
	if(newLength % SectorSize)
		newLength = (((newLength/SectorSize)*SectorSize) + SectorSize);

	status = CARD_Open(slot, (const char*)file->name, &CardFile);
	/* if the open failed, then try to create it */
	if(status == CARD_ERROR_NOFILE)
		status = CARD_Create(slot, (const char*)file->name, newLength, &CardFile);
	
	if(status == CARD_ERROR_READY) { 
	  /* Clear out the status */
	  memset(&CardStat, 0, sizeof(card_stat));
	  /* update status from the new file */
	  CARD_GetStatus(slot,CardFile.filenum,&CardStat);	
	  
	  time_t gc_time;
	  gc_time = time (NULL);
  	CardStat.icon_fmt = 2;
	  CardStat.icon_speed = 1;
	  CardStat.comment_addr = 0;
	  CardStat.banner_fmt = 0;
	  CardStat.icon_addr = 0x40;
	  tmpBuffer = memalign(32,newLength);
	  memset(tmpBuffer,0,sizeof(tmpBuffer));
	  strcpy(tmpBuffer,ctime (&gc_time));
	  strcpy(tmpBuffer+0x20,file->name);
	  memcpy(tmpBuffer+0x40,CARDIcon,sizeof(CARDIcon));       // copy icon
	  memcpy(tmpBuffer+0x40+sizeof(CARDIcon),buffer,length);  // copy file data
	  status = CARD_SetStatus(slot,CardFile.filenum,&CardStat);
  }
  		
	if(status == CARD_ERROR_READY) {
		if(CARD_Write(&CardFile, tmpBuffer, newLength, 0) == CARD_ERROR_READY) {
			file->offset += length;
			CARD_Close(&CardFile);
			if(tmpBuffer)
			  free(tmpBuffer);
			return length;
		}
		if(tmpBuffer)
		  free(tmpBuffer);
	}
	if(tmpBuffer)
		free(tmpBuffer);
	return -1;  /* failed to find or create a new file */
}

int fileBrowser_CARD_init(fileBrowser_file* file) {
	int slot = file->discoffset;
	return mount_card(slot); //mount via slot number
}

int fileBrowser_CARD_deinit(fileBrowser_file* file) {
	int slot = file->discoffset;
	CARD_Unmount(slot); //unmount via slot number
	if(SysArea){
  	free(SysArea); SysArea = NULL;
  }
	return 0;
}

