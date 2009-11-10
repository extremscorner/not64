/**
 * Wii64 - fileBrowser.c
 * Copyright (C) 2007, 2008, 2009 Mike Slegeir
 * Copyright (C) 2007, 2008, 2009 emu_kidid
 * 
 * Actual declarations of all the fileBrowser function pointers, etc
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

#include "fileBrowser.h"

#define NULL 0

fileBrowser_file* romFile_topLevel;
fileBrowser_file* saveFile_dir;

int (*romFile_init)(fileBrowser_file*) = NULL;
int (*romFile_readDir)(fileBrowser_file*, fileBrowser_file**) = NULL;
int (*romFile_readFile)(fileBrowser_file*, void*, unsigned int) = NULL;
int (*romFile_seekFile)(fileBrowser_file*, unsigned int, unsigned int) = NULL;
int (*romFile_deinit)(fileBrowser_file*) = NULL;

int (*saveFile_init)(fileBrowser_file*) = NULL;
//int (*saveFile_exists)(fileBrowser_file*) = NULL;
int (*saveFile_readFile)(fileBrowser_file*, void*, unsigned int) = NULL;
int (*saveFile_writeFile)(fileBrowser_file*, void*, unsigned int) = NULL;
int (*saveFile_deinit)(fileBrowser_file*) = NULL;

