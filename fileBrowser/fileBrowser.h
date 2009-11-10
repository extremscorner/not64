/**
 * Wii64 - fileBrowser.h
 * Copyright (C) 2007, 2008, 2009 Mike Slegeir
 * Copyright (C) 2007, 2008, 2009 emu_kidid
 * 
 * Standard protoypes for accessing files from anywhere
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

#ifndef FILE_BROWSER_H
#define FILE_BROWSER_H

#include <stdint.h>

#define FILE_BROWSER_MAX_PATH_LEN 128

#define FILE_BROWSER_ATTR_DIR     0x10

#define FILE_BROWSER_ERROR         -1
#define FILE_BROWSER_ERROR_NO_FILE -2

#define FILE_BROWSER_SEEK_SET 1
#define FILE_BROWSER_SEEK_CUR 2
#define FILE_BROWSER_SEEK_END 3

typedef struct {
	char         name[FILE_BROWSER_MAX_PATH_LEN];
	uint64_t discoffset; // Only necessary for DVD
	unsigned int offset; // Keep track of our offset in the file
	unsigned int size;
	unsigned int attr;
} fileBrowser_file;

// When you invoke a fileBrowser for ROMs, it should be invoked with this
extern fileBrowser_file* romFile_topLevel;

// Set this to directory for save games
extern fileBrowser_file* saveFile_dir;

// -- romFile function pointers --
/* Must be called before any using other functions */
extern int (*romFile_init)(fileBrowser_file*);

/* readDir functions should return the number of directory entries
     or an error of the given file pointer and fill out the file array */
extern int (*romFile_readDir)(fileBrowser_file*, fileBrowser_file**);

/* readFile returns the status of the read and reads if it can
     arguments: file*, buffer, length */
extern int (*romFile_readFile)(fileBrowser_file*, void*, unsigned int);

/* seekFile returns the status of the seek and seeks if it can
     arguments: file*, offset, seek type */
extern int (*romFile_seekFile)(fileBrowser_file*, unsigned int, unsigned int);

/* Should be called after done using other functions */
extern int (*romFile_deinit)(fileBrowser_file*);

// -- saveFile function pointers --
extern int (*saveFile_init)(fileBrowser_file*);

/* Checks whether the file exists, and fills out any inconsistent info */
//extern int (*saveFile_exists)(fileBrowser_file*);

extern int (*saveFile_readFile)(fileBrowser_file*, void*, unsigned int);

extern int (*saveFile_writeFile)(fileBrowser_file*, void*, unsigned int);

extern int (*saveFile_deinit)(fileBrowser_file*);

#endif

