/**
 * WiiSX - fileBrowser-SMB.c
 * Copyright (C) 2010 emu_kidid
 * 
 * fileBrowser module for Samba based shares
 *
 * WiiSX homepage: http://www.emulatemii.com
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

#ifdef HW_RVL

#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <network.h>
#include <ogcsys.h>
#include <smb.h>
#include "fileBrowser.h"
#include "fileBrowser-libfat.h"
#include "fileBrowser-SMB.h"

/* SMB Globals */
int net_initialized = 0;
int smb_initialized = 0;
// net init thread
static lwp_t initnetthread = LWP_THREAD_NULL;
static int netInitHalted = 0;
static int netInitPending = 0;

extern char smbUserName[];
extern char smbPassWord[];
extern char smbShareName[];
extern char smbIpAddr[];

fileBrowser_file topLevel_SMB =
	{ "smb:/", // file name
	  0ULL,      // discoffset (u64)
	  0,         // offset
	  0,         // size
	  FILE_BROWSER_ATTR_DIR
	};
 
void resume_netinit_thread() {
  if(initnetthread != LWP_THREAD_NULL) {
    netInitHalted = 0;
    LWP_ResumeThread(initnetthread);
  }
}

void pause_netinit_thread() {
  if(initnetthread != LWP_THREAD_NULL) {
    netInitHalted = 1;
    
    if(netInitPending) {
      return;
    }
  
    // until it's completed for this iteration.
    while(!LWP_ThreadIsSuspended(initnetthread)) {
      usleep(100);
    }
  }
}

	
// Init the GC/Wii net interface (wifi/bba/etc)
static void* init_network(void *args) {
 
  char ip[16];
  int res = 0, netsleep = 1*1000*1000;
  
  while(netsleep > 0) {
      if(netInitHalted) {
        LWP_SuspendThread(initnetthread);
      }
        usleep(100);
        netsleep -= 100;
  }

  while(1) {

    if(!net_initialized) {
      netInitPending = 1;
      res = if_config(ip, NULL, NULL, true);
      if(res >= 0) {
        net_initialized = 1;
      }
      else {
        net_initialized = 0;
      }
      netInitPending = 0;
    }

    netsleep = 1000*1000; // 1 sec
    while(netsleep > 0) {
      if(netInitHalted) {
        LWP_SuspendThread(initnetthread);
      }
      usleep(100);
      netsleep -= 100;
    }
  }
  return NULL;
}

void init_network_thread() {
  LWP_CreateThread (&initnetthread, init_network, NULL, NULL, 0, 40);
}

// Connect to the share specified in settings.cfg
void init_samba() {
  
  int res = 0;
  
  if(smb_initialized) {
    return;
  }
  res = smbInit(&smbUserName[0], &smbPassWord[0], &smbShareName[0], &smbIpAddr[0]);
  if(res) {
    smb_initialized = 1;
  }
  else {
    smb_initialized = 0;
  }
}

	
int fileBrowser_SMB_readDir(fileBrowser_file* ffile, fileBrowser_file** dir){	
   
  // We need at least a share name and ip addr in the settings filled out
  if(!strlen(&smbShareName[0]) || !strlen(&smbIpAddr[0])) {
    return SMB_SMBCFGERR;
  }
  
  if(!net_initialized) {       //Init if we have to
    return SMB_NETINITERR;
  } 
  
  if(!smb_initialized) {       //Connect to the share
    init_samba();
    if(!smb_initialized) {
      return SMB_SMBERR; //fail
    }
  }
		
	// Call the corresponding FAT function
	return fileBrowser_libfat_readDir(ffile, dir);
}

int fileBrowser_SMB_seekFile(fileBrowser_file* file, unsigned int where, unsigned int type){
	return fileBrowser_libfat_seekFile(file,where,type);
}

int fileBrowser_SMB_readFile(fileBrowser_file* file, void* buffer, unsigned int length){
	return fileBrowser_libfatROM_readFile(file,buffer,length);
}

int fileBrowser_SMB_init(fileBrowser_file* file){
	return 0;
}

int fileBrowser_SMB_deinit(fileBrowser_file* file) {
  /*if(smb_initialized) {
    smbClose("smb");
    smb_initialized = 0;
  }*/
	return fileBrowser_libfatROM_deinit(file);
}

#endif
