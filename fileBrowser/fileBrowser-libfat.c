/**
 * Wii64 - fileBrowser-libfat.c
 * Copyright (C) 2007, 2008, 2009 Mike Slegeir
 * Copyright (C) 2007, 2008, 2009 emu_kidid
 * 
 * fileBrowser for any devices using libfat
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


#include <fat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/dir.h>
#include "fileBrowser.h"
#include "../main/ata.h"
#include <sdcard/gcsd.h>

extern int stop;

#ifdef HW_RVL
#define DEVICE_REMOVAL_THREAD
#endif

#ifdef HW_RVL
#include <sdcard/wiisd_io.h>
#include <ogc/usbstorage.h>
const DISC_INTERFACE* frontsd = &__io_wiisd;
const DISC_INTERFACE* usb = &__io_usbstorage;
#else
#include <ogc/dvd.h>
const DISC_INTERFACE* gcloader = &__io_gcode;
#endif
const DISC_INTERFACE* carda = &__io_gcsda;
const DISC_INTERFACE* cardb = &__io_gcsdb;
const DISC_INTERFACE* cardc = &__io_gcsd2;
const DISC_INTERFACE* ideexia = &__io_ataa;
const DISC_INTERFACE* ideexib = &__io_atab;

// Threaded insertion/removal detection
#define THREAD_SLEEP 100
#define CARD_A   1
#define CARD_B   2
#define CARD_C   3
#define GCLOADER 3
#define FRONTSD  3
#define USB      3
static lwp_t removalThread = LWP_THREAD_NULL;
static int rThreadRun = 0;
static int rThreadCreated = 0;
static char sdMounted  = 0;
static char sdNeedsUnmount  = 0;
static char fatMounted = 0;
static char fatNeedsUnmount = 0;

fileBrowser_file topLevel_libfat_Auto =
	{ "\0",
	  0,
	  0,
	  0,
	  FILE_BROWSER_ATTR_DIR
	 };

fileBrowser_file topLevel_libfat_Default =
	{ "sd:/not64/roms", // file name
	  0, // sector
	  0, // offset
	  0, // size
	  FILE_BROWSER_ATTR_DIR
	 };
	 
fileBrowser_file topLevel_libfat =
	{ "fat:/not64/roms", // file name
	  0, // sector
	  0, // offset
	  0, // size
	  FILE_BROWSER_ATTR_DIR
	 };

fileBrowser_file saveDir_libfat_Default =
	{ "sd:/not64/saves",
	  0,
	  0,
	  0,
	  FILE_BROWSER_ATTR_DIR
	 };
	 
fileBrowser_file saveDir_libfat =
	{ "fat:/not64/saves",
	  0,
	  0,
	  0,
	  FILE_BROWSER_ATTR_DIR
	 };

void continueRemovalThread()
{
#ifdef DEVICE_REMOVAL_THREAD 
  if(rThreadRun)
    return;
  rThreadRun = 1;
  LWP_ResumeThread(removalThread);
#endif
}

void pauseRemovalThread()
{
#ifdef DEVICE_REMOVAL_THREAD  
  if(!rThreadRun)
    return;
  rThreadRun = 0;

  // wait for thread to finish
  while(!LWP_ThreadIsSuspended(removalThread)) usleep(THREAD_SLEEP);
#endif
}

static int devsleep = 1*1000*1000;

static void *removalCallback (void *arg)
{
  while(devsleep > 0)
  {
    if(!rThreadRun)
      LWP_SuspendThread(removalThread);
      usleep(THREAD_SLEEP);
      devsleep -= THREAD_SLEEP;
  }

  while (1)
  {
#ifdef HW_RVL
    if(sdMounted==FRONTSD)
      if(!frontsd->isInserted()) {
        sdNeedsUnmount=FRONTSD;
        sdMounted=0;
      }
    if(fatMounted==USB)
      if(!usb->isInserted()) {
        fatNeedsUnmount=USB;
        fatMounted = 0;
      }
#endif      
      
    devsleep = 1000*1000; // 1 sec
    while(devsleep > 0)
    {
      if(!rThreadRun)
        LWP_SuspendThread(removalThread);
      usleep(THREAD_SLEEP);
      devsleep -= THREAD_SLEEP;
    }
  }
  return NULL;
}

void InitRemovalThread()
{
#ifdef DEVICE_REMOVAL_THREAD 
  LWP_CreateThread (&removalThread, removalCallback, NULL, NULL, 0, LWP_PRIO_NORMAL);
  rThreadCreated = 1;
#endif
}


int fileBrowser_libfat_readDir(fileBrowser_file* file, fileBrowser_file** dir){
  
  pauseRemovalThread();
	
	DIR* dp = opendir( file->name );
	if(!dp) return FILE_BROWSER_ERROR;
	struct dirent* ent;
	struct stat fstat;
	
	// Set everything up to read
	int num_entries = 2, i = 0;
	*dir = malloc( num_entries * sizeof(fileBrowser_file) );
	// Read each entry of the directory
	while( (ent = readdir(dp)) ){
		// Make sure we have room for this one
		if(i == num_entries){
			++num_entries;
			*dir = realloc( *dir, num_entries * sizeof(fileBrowser_file) ); 
		}
		sprintf((*dir)[i].name, "%s/%s", file->name, ent->d_name);
		stat((*dir)[i].name, &fstat);
		(*dir)[i].offset = 0;
		(*dir)[i].size   = fstat.st_size;
		(*dir)[i].attr   = S_ISDIR(fstat.st_mode) ?
		                     FILE_BROWSER_ATTR_DIR : 0;
		++i;
	}
	
	closedir(dp);
	continueRemovalThread();

	return num_entries;
}

int fileBrowser_libfat_seekFile(fileBrowser_file* file, unsigned int where, unsigned int type){
	if(type == FILE_BROWSER_SEEK_SET) file->offset = where;
	else if(type == FILE_BROWSER_SEEK_CUR) file->offset += where;
	else file->offset = file->size + where;
	
	return 0;
}

int fileBrowser_libfat_readFile(fileBrowser_file* file, void* buffer, unsigned int length){
  pauseRemovalThread();
	FILE* f = fopen( file->name, "rb" );
	if(!f) return FILE_BROWSER_ERROR;
	
	fseek(f, file->offset, SEEK_SET);
	int bytes_read = fread(buffer, 1, length, f);
	if(bytes_read > 0) file->offset += bytes_read;
	
	fclose(f);
	continueRemovalThread();
	return bytes_read;
}

int fileBrowser_libfat_writeFile(fileBrowser_file* file, void* buffer, unsigned int length){
  pauseRemovalThread();
	FILE* f = fopen( file->name, "wb" );
	if(!f) return FILE_BROWSER_ERROR;
	
	fseek(f, file->offset, SEEK_SET);
	int bytes_read = fwrite(buffer, 1, length, f);
	if(bytes_read > 0) file->offset += bytes_read;
	
	fclose(f);
	continueRemovalThread();
	return bytes_read;
}

/* call fileBrowser_libfat_init as much as you like for all devices
    - returns 0 on device not present/error
    - returns 1 on ok
*/
int fileBrowser_libfat_init(fileBrowser_file* f){
 	
  int res = 0;
 	
 	if(!rThreadCreated) InitRemovalThread();
#ifdef HW_RVL
  if(f->name[0] == 's') {      //SD
    if(!sdMounted) {           //if there's nothing currently mounted
      pauseRemovalThread();
      if(sdNeedsUnmount==FRONTSD) {
        fatUnmount("sd:");
        frontsd->shutdown();
        sdNeedsUnmount = 0;
      }
    	if(fatMountSimple ("sd", frontsd)) {
       	sdMounted = FRONTSD;
       	res = 1;
     	}
     	else if(fatMountSimple ("sd", cardb)) {
     	  sdMounted = CARD_B;
     	  res = 1;
   	  }
   	  else if(fatMountSimple ("sd", carda)) {
     	  sdMounted = CARD_A;
     	  res = 1;
   	  }
   	  continueRemovalThread();
   	  return res;
 	  }
 	  else
 	    return 1;
 	}
 	else if(f->name[0] == 'f') {
   	if(!fatMounted) {
     	pauseRemovalThread();
     	if(fatNeedsUnmount==USB) {
     	  fatUnmount("fat:");
     	  usb->shutdown();
     	  fatNeedsUnmount=0;
      }
     	usleep(devsleep);
     	if(fatMountSimple ("fat", usb)) {
        fatMounted = USB;
        res = 1;
     	}
     	else if(fatMountSimple ("fat", ideexib)) {
     	  fatMounted = CARD_B;
     	  res = 1;
      }
      else if(fatMountSimple ("fat", ideexia)) {
     	  fatMounted = CARD_A;
     	  res = 1;
      }
      continueRemovalThread();
      return res;
    }
    else
      return 1;
  }
#else
  if(f->name[0] == 's') {
    if(!sdMounted) {
    	if(fatMountSimple ("sd", cardc)) {
     	  sdMounted = CARD_C;
     	  res = 1;
   	  }
   	  else if(fatMountSimple ("sd", cardb)) {
     	  sdMounted = CARD_B;
     	  res = 1;
   	  }
   	  else if(fatMountSimple ("sd", carda)) {
     	  sdMounted = CARD_A;
     	  res = 1;
   	  }
   	  return res;
 	  }
 	  else
 	    return 1;
 	}
 	else if(f->name[0] == 'f') {
   	if(!fatMounted) {
    	if(fatMountSimple ("fat", gcloader)) {
     	  fatMounted = GCLOADER;
     	  res = 1;
   	  }
   	  else if(fatMountSimple ("fat", ideexib)) {
     	  fatMounted = CARD_B;
     	  res = 1;
      }
      else if(fatMountSimple ("fat", ideexia)) {
     	  fatMounted = CARD_A;
     	  res = 1;
      }
      return res;
    }
    else
      return 1;
  }
#endif
 	return res;
}

int fileBrowser_libfat_deinit(fileBrowser_file* f){
  //we can't support multiple device re-insertion
  //because there's no device removed callbacks
	return 0;
}


/* Special for ROM loading only */
static FILE* fd;

int fileBrowser_libfatROM_deinit(fileBrowser_file* f){
  pauseRemovalThread();
	if(fd)
		fclose(fd);
	fd = NULL;
	continueRemovalThread();
	
	return 0;
}
	
int fileBrowser_libfatROM_readFile(fileBrowser_file* file, void* buffer, unsigned int length){
  if(stop)     //do this only in the menu
    pauseRemovalThread();
	if(!fd) fd = fopen( file->name, "rb");
	
	fseek(fd, file->offset, SEEK_SET);
	int bytes_read = fread(buffer, 1, length, fd);
	if(bytes_read > 0) file->offset += bytes_read;
  
	if(stop)
	  continueRemovalThread();
	return bytes_read;
}

