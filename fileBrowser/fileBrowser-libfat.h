/**
 * Wii64 - fileBrowser-libfat.h
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


#ifndef FILE_BROWSER_LIBFAT_H
#define FILE_BROWSER_LIBFAT_H

extern fileBrowser_file topLevel_libfat_Default;  //GC SD Slots & Wii Front SD Slot
extern fileBrowser_file topLevel_libfat_USB;      //Wii only, USB
extern fileBrowser_file saveDir_libfat_Default;   //GC SD Slots & Wii Front SD Slot
extern fileBrowser_file saveDir_libfat_USB;       //Wii only, USB

int fileBrowser_libfat_readDir(fileBrowser_file*, fileBrowser_file**);
int fileBrowser_libfat_readFile(fileBrowser_file*, void*, unsigned int);
int fileBrowser_libfat_writeFile(fileBrowser_file*, void*, unsigned int);
int fileBrowser_libfat_seekFile(fileBrowser_file*, unsigned int, unsigned int);
int fileBrowser_libfat_init(fileBrowser_file* f);
int fileBrowser_libfat_deinit(fileBrowser_file* f);

int fileBrowser_libfatROM_readFile(fileBrowser_file*, void*, unsigned int);
int fileBrowser_libfatROM_deinit(fileBrowser_file* f);

void pauseRemovalThread();
void continueRemovalThread();

#endif
