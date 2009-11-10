/**
 * Wii64 - Saves.h
 * Copyright (C) 2007, 2008, 2009 Mike Slegeir
 * 
 * Defines/globals for saving files
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


#ifndef SAVES_H
#define SAVES_H

#include "../fileBrowser/fileBrowser.h"

/*
extern char saveEnabled;
extern int  savetype;
extern char savepath[];

#define SELECTION_SLOT_A    0
#define SELECTION_SLOT_B    1
#define SELECTION_TYPE_SD   2
#define SELECTION_TYPE_MEM  0*/

// Return 0 if load/save fails, 1 otherwise

int loadEeprom(fileBrowser_file* savepath);
int saveEeprom(fileBrowser_file* savepath);

int loadMempak(fileBrowser_file* savepath);
int saveMempak(fileBrowser_file* savepath);

int loadSram(fileBrowser_file* savepath);
int saveSram(fileBrowser_file* savepath);

int loadFlashram(fileBrowser_file* savepath);
int saveFlashram(fileBrowser_file* savepath);

#endif

