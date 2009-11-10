/**
 * Wii64 - fileBrowser-WiiFS.c
 * Copyright (C) 2007, 2008, 2009 Mike Slegeir
 * 
 * fileBrowser Wii FileSystem module
 *
 * Wii64 homepage: http://www.emulatemii.com
 * email address: tehpola@gmail.com
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


#define HW_RVL
#include <stdlib.h>
#include <string.h>
#include <ogc/es.h>
#include <ogc/isfs.h>
#include "HW64/customTitle.h"
#include "fileBrowser.h"

void* memalign(unsigned int, unsigned int);

static int isfsOpen = 0;
static int identified = 0;
static long long titleID = TITLE_ID;
static int recOpenLib(void){ return (isfsOpen++) ? 0 : ISFS_Initialize(); }
static int recCloseLib(void){ return (--isfsOpen) ? ISFS_Deinitialize() : 0; }

fileBrowser_file topLevel_WiiFS =
	{ "/title/00010000/", // file name
	  0, // sector
	  0, // offset
	  0, // size
	  FILE_BROWSER_ATTR_DIR
	 };

fileBrowser_file saveDir_WiiFS =
	{ "/title/00010000/", // file name
	  0, // sector
	  0, // offset
	  0, // size
	  FILE_BROWSER_ATTR_DIR
	 };

static void shrinkFilename(char* absname){
	// Takes an absolute filename, shrinks the filename to 8.3
	char* name = absname; // the actual filename w/o path
	
	// Move name to after the last '/'
	while(*(++absname) != 0)
		if(*absname == '/') name = absname+1;
	
	int len = absname - name;
	if(len <= 12) return; // If our name is smaller than 8.3 we're ok
	
	// Create a checksum of the filename as the new name
	int chksum = 0;
	absname = name; // use this as a temp pointer
	int i = 0;
	// Calculate checksum from name
	while(*(++absname) != '.')
		chksum += (*absname) << i++;
	// Write checksum as hex string over name
	for(i=7; i>=0; --i){
		int nibble = chksum & 0xF;
		char ch;
		
		if(nibble > 9) ch = nibble - 10 + 97;
		else ch = nibble + 48;
		name[i] = ch;
		
		chksum >>= 4;
	}
	// Append the extension to the new name
	name[8]  = absname[0]; // .
	name[9]  = absname[1]; // e
	name[10] = absname[2]; // x
	name[11] = absname[3]; // t
	name[12] = 0;
	
}

// FIXME: dhewg says this is unnecessary
static inline char* getAlignedName(char* name){
	static char alignedBuf[64] __attribute__((aligned(32)));
	if((int)name % 32){
		strncpy(alignedBuf, name, 64);
		return alignedBuf;
	} else return name;
}

int fileBrowser_WiiFS_readDir(fileBrowser_file* file, fileBrowser_file** dir){
	static char dirents[32*(8+1+3+1)] __attribute__((aligned(32)));
	unsigned int numDirents = 32;
	// Call the corresponding ISFS function
	char* name = getAlignedName(&file->name);
	int ret = ISFS_ReadDir(name, dirents, &numDirents);
	
	// If it was not successful, just return the error
	if(ret < 0) return ret;
	
	// Convert the ISFS data to fileBrowser_files
	*dir = malloc( numDirents * sizeof(fileBrowser_file) );
	int i; char* dirent = &dirents[0];
	for(i=0; i<numDirents; ++i){
		sprintf((*dir)[i].name, "%s/%s", name, dirent);
		// Collect info about this file
		int fd = ISFS_Open( getAlignedName((*dir)[i].name), 1 );
		if(fd >= 0){
			fstats* stats = memalign( 32, sizeof(fstats) );
			ISFS_GetFileStats(fd, stats);
			(*dir)[i].attr = 0;
			(*dir)[i].size = stats->file_length;
			free(stats);
			ISFS_Close(fd);
		} else {
			(*dir)[i].attr = FILE_BROWSER_ATTR_DIR;
			(*dir)[i].size = 0;
		}
		(*dir)[i].offset = 0;
		
		while(*(dirent++)); // Advance to the next dirent
	}
	
	return numDirents;
}

int fileBrowser_WiiFS_seekFile(fileBrowser_file* file, unsigned int where, unsigned int type){
	if(type == FILE_BROWSER_SEEK_SET) file->offset = where;
	else if(type == FILE_BROWSER_SEEK_CUR) file->offset += where;
	else file->offset = file->size + where;
	
	return 0;
}

int fileBrowser_WiiFS_readFile(fileBrowser_file* file, void* buffer, unsigned int length){
	// Make sure everything is aligned (address and length)
	int isUnaligned = ((int)buffer)%32 | length%32;
	int alignedLen = (length+31)&0xffffffe0;
	char* alignedBuf;
	if(isUnaligned) alignedBuf = memalign(32, alignedLen);
	else alignedBuf = buffer;
	
	// Make sure the filename is 8.3 and open the short filename
	shrinkFilename( &file->name );
	int f = ISFS_Open( getAlignedName(&file->name), 1 );
	if(f < 0) return FILE_BROWSER_ERROR;
	
	// Do the actual read, into the aligned buffer if we need to
	ISFS_Seek(f, file->offset, 0);
	int bytes_read = ISFS_Read(f, alignedBuf, alignedLen);
	if(bytes_read > 0) file->offset += bytes_read;
	
	// If it was unaligned, you have to copy it and clean up
	if(isUnaligned){
		memcpy(buffer, alignedBuf, length);
		free(alignedBuf);
	}
	
	ISFS_Close(f);
	return bytes_read;
}

int fileBrowser_WiiFS_writeFile(fileBrowser_file* file, void* buffer, unsigned int length){
	// Make sure everything is aligned (address and length)
	int isUnaligned = ((int)buffer)%32 | length%32;
	int alignedLen = (length+31)&0xffffffe0;
	char* alignedBuf;
	if(isUnaligned){
		alignedBuf = memalign(32, alignedLen);
		memcpy(alignedBuf, buffer, length);
	} else alignedBuf = buffer;
	
	// Make sure the filename is 8.3 and open the short filename
	shrinkFilename( &file->name );
	char* name = getAlignedName(&file->name);
	int f = ISFS_Open( name, 2 );
	if(f < 0){ // Create rw file: rwrwr-
		if((f = ISFS_CreateFile( name, 0, 1|2, 1|2, 1 )) < 0)
			return FILE_BROWSER_ERROR;
		else f = ISFS_Open( name, 2 );
	}
	
	// Do the actual write, from the aligned buffer if need be
	ISFS_Seek(f, file->offset, 0);
	int bytes_read = ISFS_Write(f, alignedBuf, alignedLen);
	if(bytes_read > 0) file->offset += bytes_read;
	
	// Clean up
	if(isUnaligned) free(alignedBuf);
	
	ISFS_Close(f);
	return bytes_read;
}

static void fillInTitleDir(fileBrowser_file* f){
#if 0
	// Convert the gameCode in memory to hex
	// and concat the hex string to gameDir
	
	char* id = f->name + 16; // start after "/title/00010000/"
	unsigned int gameCode = *((unsigned int*)0x80000000);
	
	int i;
	for(i=7; i>=0; --i){
		int nibble = gameCode & 0xF;
		char ch;
		
		if(nibble > 9) ch = nibble - 10 + 97;
		else ch = nibble + 48;
		*(id+i) = ch;
		
		gameCode >>= 4;
	}
	f->name[16+7+1] = 0;
	strcat(&f->name, "/data");
#else
	// We have ES functions to do this for us now!
	char* dataDir = memalign(32, 64);
	ES_GetDataDir(titleID, dataDir);
	strncpy(&f->name, dataDir, 64);
	free(dataDir);
#endif
}

static int identify(void){
	// Identify as our own title
	void* dvdCert = NULL, * dvdTMD = NULL, * dvdTicket = NULL;
	unsigned int dvdCertSize, dvdTMDSize, dvdTicketSize, keyid;
	int ret;
	
	if(!customCert){ // If there's no certificate supplied
#if 0
		// Use the one from the DVD
		ret = getTitle(&dvdCert,   &dvdCertSize,
		               &dvdTMD,    &dvdTMDSize,
		               &dvdTicket, &dvdTicketSize);
		customCert = dvdCert;
		customCertSize = dvdCertSize;
		if(ret < 0) return ret;
#else
		return identified = 0;
#endif
	}
	
	ret = ES_Identify(customCert,   customCertSize,
	                  customTMD,    customTMDSize,
	                  customTicket, customTicketSize,
	                  &keyid);
	if(ret >= 0){
		ES_GetTitleID(&titleID);
		ISFS_Initialize();
		// If we haven't identified this title before
		// we'll need to set some things up
		char* path = memalign(32, 64);;
		ES_GetDataDir(titleID, path);
		strncat(path, "/banner.bin", 64);
		ret = ISFS_Open(path, 1);
		
		if(ret < 0){
			// If the banner doesn't exist
			// Create our banner.bin
			ret = ISFS_CreateFile(path, 0, 3, 3, 1);
			if(ret < 0) return 0;
			ret = ISFS_Open(path, 2);
			if(ret < 0) return 0;
			ISFS_Write(ret, customBanner, customBannerSize);
			ISFS_Close(ret);
			// Create the N64SAVES directory
			ES_GetDataDir(titleID, path);
			strncat(path, "/N64SAVES", 64);
			ISFS_CreateDir(path, 0, 3, 3, 1);
		} else ISFS_Close(ret);
		
		free(path);
		return identified = 1;
	}
#if 0
	// If that still fails, try to identify from the discs certs
	if(!dvdCert || !dvdTMD || !dvdTicket)
		ret = getTitle(&dvdCert,   &dvdCertSize,
			       &dvdTMD,    &dvdTMDSize,
			       &dvdTicket, &dvdTicketSize);
	else ret = 0;
	
	if(ret >= 0){
		ret = ES_Identify(dvdCert,   dvdCertSize,
		                  dvdTMD,    dvdTMDSize,
		                  dvdTicket, dvdTicketSize,
		                  &keyid);
		ES_GetTitleID(&titleID);
		ISFS_Initialize();
		return identified = (ret >= 0);
	}
#endif
	return identified = 0;
}

int fileBrowser_WiiFS_init(fileBrowser_file* f){
	if(!identified) identify();
	fillInTitleDir(f);
	strcat(&f->name, "/N64SAVES");
	
	recOpenLib();
	
	// Make sure our save game directory exists
	char* name = getAlignedName(&f->name);
	if(ISFS_ReadDir(name, NULL, NULL) < 0)
		ISFS_CreateDir(name, 0, 3, 3, 1);
	strcat(&f->name, "/");
	
	return 0;
}

int fileBrowser_WiiFS_deinit(fileBrowser_file* f){ return recCloseLib(); }


// Special functions for ROM loading only
static int fd = -1;

int fileBrowser_WiiFSROM_init(fileBrowser_file* f){
	if(!identified) identify();
	fillInTitleDir(f);
	strcat(&f->name, "/N64ROMS");
	
	recOpenLib();
	
	// Make sure our ROM directory exists
	char* name = getAlignedName(&f->name);
	if(ISFS_ReadDir(name, NULL, NULL) < 0)
		ISFS_CreateDir(name, 0, 3, 3, 1);
	
	return 0;
}

int fileBrowser_WiiFSROM_readFile(fileBrowser_file* file, void* buffer, unsigned int length){
	if(fd < 0) fd = ISFS_Open( getAlignedName(&file->name), 1 );
	if(fd < 0) return FILE_BROWSER_ERROR;
	// Make sure everything is aligned (address and length)
	int isUnaligned = ((int)buffer)%32 | length%32;
	int alignedLen = (length+31)&0xffffffe0;
	char* alignedBuf = isUnaligned ? 
		memalign(32, alignedLen) : buffer;
	
	// Do the actual read, into the aligned buffer if we need to
	ISFS_Seek(fd, file->offset, 0);
	int bytes_read = ISFS_Read(fd, alignedBuf, alignedLen);
	if(bytes_read > 0) file->offset += bytes_read;
	
	// If it was unaligned, you have to copy it and clean up
	if(isUnaligned){
		memcpy(buffer, alignedBuf, length);
		free(alignedBuf);
	}

	return bytes_read;
}

int fileBrowser_WiiFSROM_deinit(fileBrowser_file* f){
	if(fd >= 0) ISFS_Close(fd);
	fd = -1;
	return recCloseLib();
}

