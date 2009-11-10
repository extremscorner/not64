/**
 * Wii64 - fileBrowser-CARD.h
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


#ifndef FILE_BROWSER_CARD_H
#define FILE_BROWSER_CARD_H

#include "fileBrowser.h"

extern fileBrowser_file topLevel_CARD_SlotA;
extern fileBrowser_file topLevel_CARD_SlotB;
#define saveDir_CARD_SlotA topLevel_CARD_SlotA
#define saveDir_CARD_SlotB topLevel_CARD_SlotB

int fileBrowser_CARD_readDir(fileBrowser_file*, fileBrowser_file**);
int fileBrowser_CARD_readFile(fileBrowser_file*, void*, unsigned int);
int fileBrowser_CARD_writeFile(fileBrowser_file*, void*, unsigned int);
int fileBrowser_CARD_seekFile(fileBrowser_file*, unsigned int, unsigned int);
int fileBrowser_CARD_init(fileBrowser_file* file);
int fileBrowser_CARD_deinit(fileBrowser_file* file);

#endif

