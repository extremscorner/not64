/**
 * Wii64 - fileBrowser-DVD.c
 * Copyright (C) 2007, 2008, 2009 emu_kidid
 * 
 * fileBrowser module for ISO9660 DVD Discs
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


#include "fileBrowser.h"
#include <ogc/machine/processor.h>
#include "../main/gc_dvd.h"
#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <ogc/dvd.h>
#ifdef HW_RVL
#include <di/di.h>
#endif

#ifdef HW_DOL
#define mfpvr()   ({unsigned int rval; \
      asm volatile("mfpvr %0" : "=r" (rval)); rval;})
#endif

/* DVD Globals */
#define GC_CPU_VERSION 0x00083214
extern int previously_initd;
int dvdInitialized = 0;

/* Worked out manually from my original Disc */
#define OOT_OFFSET 0x54FBEEF4ULL
#define MQ_OFFSET  0x52CCC5FCULL
#define ZELDA_SIZE 0x2000000

fileBrowser_file topLevel_DVD =
	{ "\\", // file name
	  0ULL,      // discoffset (u64)
	  0,         // offset
	  0,         // size
	  FILE_BROWSER_ATTR_DIR
	};

void init_dvd()
{
#ifdef HW_DOL
  if(mfpvr()!=GC_CPU_VERSION) //GC mode on Wii, modchip required
  {
    DVD_Reset(DVD_RESETHARD);
    dvd_read_id();
    if(!dvd_get_error())
      dvdInitialized=1;
  }
  else      //GC, no modchip even required :)
  {
    DVD_Reset(DVD_RESETHARD);
    DVD_Mount ();
    if(!dvd_get_error())
      dvdInitialized=1;
  }
#endif
#ifdef HW_RVL
  //Wiimode stuff is handled by DVDx
  u32 val;
  DI_GetCoverRegister(&val);
  if(val & 0x1) return; //no disc inserted
	DI_Mount();
	while(DI_GetStatus() & DVD_INIT) usleep(20000);
	dvdInitialized=1;
#endif
}
 
	 
int fileBrowser_DVD_readDir(fileBrowser_file* ffile, fileBrowser_file** dir){	
#ifdef HW_DOL
  if(dvd_get_error())
    dvdInitialized = 0;
#endif
  if(!dvdInitialized)
    init_dvd();
  if(!dvdInitialized) return FILE_BROWSER_ERROR;  //fails if No disc
	
  int num_entries = 0;
	
	if (!memcmp((void*)0x80000000, "D43U01", 6)) { //OoT bonus disc support.
		num_entries = 2;
		*dir = malloc( num_entries * sizeof(fileBrowser_file) );
		strcpy( (*dir)[0].name, "Zelda - Ocarina of Time");
		(*dir)[0].discoffset = OOT_OFFSET;
		(*dir)[0].offset = 0;
		(*dir)[0].size   = ZELDA_SIZE;
		(*dir)[0].attr	 = 0;
		strcpy( (*dir)[1].name, "Zelda - Ocarina of Time MQ" );
		(*dir)[1].discoffset = MQ_OFFSET;
		(*dir)[1].offset = 0;
		(*dir)[1].size   = ZELDA_SIZE;
		(*dir)[1].attr	 = 0;
		return num_entries;
	}
	
	// Call the corresponding DVD function
	num_entries = dvd_read_directoryentries(ffile->discoffset,ffile->size);
	
	// If it was not successful, just return the error
	if(num_entries <= 0) return FILE_BROWSER_ERROR;
	
	// Convert the DVD "file" data to fileBrowser_files
	*dir = malloc( num_entries * sizeof(fileBrowser_file) );
	int i;
	for(i=0; i<num_entries; ++i){
		strcpy( (*dir)[i].name, DVDToc->file[i].name );
		(*dir)[i].discoffset = (uint64_t)(((uint64_t)DVDToc->file[i].sector)*2048);
		(*dir)[i].offset = 0;
		(*dir)[i].size   = DVDToc->file[i].size;
		(*dir)[i].attr	 = 0;
		if(DVDToc->file[i].flags == 2)//on DVD, 2 is a dir
			(*dir)[i].attr   = FILE_BROWSER_ATTR_DIR; 
		if((*dir)[i].name[strlen((*dir)[i].name)-1] == '/' )
			(*dir)[i].name[strlen((*dir)[i].name)-1] = 0;	//get rid of trailing '/'
	}
	//kill the large TOC so we can have a lot more memory ingame (256k more)
	free(DVDToc);
  DVDToc = NULL;
		
	if(strlen((*dir)[0].name) == 0)
		strcpy( (*dir)[0].name, ".." );
	
	return num_entries;
}

int fileBrowser_DVD_seekFile(fileBrowser_file* file, unsigned int where, unsigned int type){
	if(type == FILE_BROWSER_SEEK_SET) file->offset = where;
	else if(type == FILE_BROWSER_SEEK_CUR) file->offset += where;
	else file->offset = file->size + where;
	return 0;
}

int fileBrowser_DVD_readFile(fileBrowser_file* file, void* buffer, unsigned int length){
	int bytesread = read_safe(buffer,file->discoffset+file->offset,length);
	if(bytesread > 0)
		file->offset += bytesread;
	return bytesread;
}

int fileBrowser_DVD_init(fileBrowser_file* file){
	return 0;
}

int fileBrowser_DVD_deinit(fileBrowser_file* file) {
	return 0;
}

