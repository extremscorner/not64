/**
 * Wii64 - textFileBrowser.c (1337 but Deprecated)
 * Copyright (C) 2007, 2008, 2009 Mike Slegeir
 * 
 * This is how the ROM should be accessed, this way the ROM doesn't waste RAM
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

#include <stdio.h>
#include <math.h>
#include <gccore.h>		/*** Wrapper to include common libogc headers ***/
#include <ogcsys.h>		/*** Needed for console support ***/
#include <sdcard.h>		/*** SDCard Header. Remember to add -lsdcard in your Makefile ***/
#include <stdlib.h>
#include "gc_dvd.h"

/* Worked out manually from my original Disc */
#define OOT_OFFSET 0x54FBEEF4
#define MQ_OFFSET  0x52CCC5FC
#define ZELDA_SIZE 0x2000000
extern unsigned int isWii;
#include <string.h>
#ifdef USE_GUI
#include "../gui/GUI.h"
#define MAXLINES 16
#define PRINT GUI_print
#else
#define MAXLINES 30
#define PRINT printf
#endif

#ifdef USE_GUI
#define CLEAR() GUI_clear()
#else
#define CLEAR() printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n")
#endif

static char buffer[96];

char* textFileBrowser(char* directory){
	DIR *sddir = NULL;
	
	int entries_found = SDCARD_ReadDir(directory, (DIR **) & sddir);
	if(entries_found <= 0){ if(sddir) free(sddir); return NULL; }
	
	int currentSelection = 0;
	while(1){
		CLEAR();
		sprintf(buffer, "browsing %s:\n\n", directory);
		PRINT(buffer);
		int i = MIN(MAX(0,currentSelection-7),MAX(0,entries_found-14));
		int max = MIN(entries_found, MAX(currentSelection+7,14));
//		int i = MIN(MAX(0,currentSelection-floor((MAXLINES-0.5)/2)),MAX(0,entries_found-(MAXLINES-2)));
//		int max = MIN(entries_found, MAX(currentSelection+ceil((MAXLINES)/2)-1,MAXLINES-2));
//		sprintf(buffer,"entr=%d;curr=%d;i=%d;max=%d",entries_found,currentSelection,i,max);
//		PRINT(buffer);
		for(; i<max; ++i){
			if(i == currentSelection)
				sprintf(buffer, "*");
			else    sprintf(buffer, " ");
			sprintf(buffer, "%s\t%-32s\t%s\n", buffer,
			        sddir[i].fname, (sddir[i].fattr&SDCARD_ATTR_DIR) ? "DIR" : "");
			PRINT(buffer);
		}
		
		/*** Wait for A/up/down press ***/
		while (!(PAD_ButtonsHeld(0) & PAD_BUTTON_A) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_UP) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_DOWN));
		if(PAD_ButtonsHeld(0) & PAD_BUTTON_UP)   currentSelection = (--currentSelection < 0) ? entries_found-1 : currentSelection;
		if(PAD_ButtonsHeld(0) & PAD_BUTTON_DOWN) currentSelection = (currentSelection + 1) % entries_found;
		if(PAD_ButtonsHeld(0) & PAD_BUTTON_A){
			if(sddir[currentSelection].fattr & SDCARD_ATTR_DIR){
				char newDir[SDCARD_MAX_PATH_LEN];
				sprintf(newDir, "%s\\%s", directory, sddir[currentSelection].fname);		
				if(sddir) free(sddir);
				CLEAR();
				sprintf(buffer,"MOVING TO %s.\nPress B to continue.\n",newDir);
				PRINT(buffer);
				while (!(PAD_ButtonsHeld(0) & PAD_BUTTON_B));
				return textFileBrowser(newDir);
			} else {
				char* newDir = malloc(SDCARD_MAX_PATH_LEN);
				sprintf(newDir, "%s\\%s", directory, sddir[currentSelection].fname);
				if(sddir) free(sddir);
				CLEAR();
				sprintf(buffer,"SELECTING %s.\nPress B to continue.\n",newDir);
				PRINT(buffer);
				while (!(PAD_ButtonsHeld(0) & PAD_BUTTON_B));
				return newDir;
			}
		}
		/*** Wait for up/down button release ***/
		while (!(!(PAD_ButtonsHeld(0) & PAD_BUTTON_A) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_UP) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_DOWN)));
	}
}

/* DVD ROM LOADING */

int rom_sizeDVD = 0;
unsigned int rom_offsetDVD = 0;
char *textFileBrowserDVD(){
	PRINT("Mounting DVD...\n");
	if(!isWii)
		DVD_Mount ();
	if(isWii) {
		DVD_Reset(DVD_RESETHARD);
		dvd_read_id();
	}
	PRINT("Reading DVD...\n");

	int sector = 16;
	static unsigned char bufferDVD[2048] __attribute__((aligned(32)));

	read_sector(bufferDVD, 0);
	if (!memcmp(bufferDVD, "D43U01", 6)) {
		CLEAR();
		PRINT("\n\nDisc Contents:\n\n");
		PRINT("Zelda Ocarina of Time MQ (PUSH X)\n");
		PRINT("Zelda Ocarina of Time     (PUSH Y) \n\n");
		while (!(PAD_ButtonsHeld(0) & PAD_BUTTON_X) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_Y));
		if(PAD_ButtonsHeld(0) & PAD_BUTTON_X) {
			rom_offsetDVD = MQ_OFFSET;
			rom_sizeDVD = ZELDA_SIZE;
			CLEAR();
			return "Ocarina of Time - Master Quest";
		}
		if(PAD_ButtonsHeld(0) & PAD_BUTTON_Y) {
			rom_offsetDVD = OOT_OFFSET;
			rom_sizeDVD = ZELDA_SIZE;
			CLEAR();
			return "Ocarina of Time";
		}
	}
	
	
	struct pvd_s* pvd = 0;
	struct pvd_s* svd = 0;
	while (sector < 32)
	{
		if (read_sector(bufferDVD, sector))
			PRINT("FATAL ERROR...\n");
		if (!memcmp(((struct pvd_s *)bufferDVD)->id, "\2CD001\1", 8))
		{
			svd = (void*)bufferDVD;
			break;
		}
		++sector;
	}

	if (!svd)
	{
		sector = 16;
		while (sector < 32)
		{
			if (read_sector(bufferDVD, sector))
				PRINT("FATAL ERROR...\n");

			if (!memcmp(((struct pvd_s *)bufferDVD)->id, "\1CD001\1", 8))
			{
				pvd = (void*)bufferDVD;
				break;
			}
			++sector;
		}
	}

	if ((!pvd) && (!svd))
	{
		PRINT("No ISO9660 DVD Found!\n");
		return "ISO9660 ERROR!";
	}

	files = 0;
	if (svd)
	{
		is_unicode = 1;	
		read_direntry(svd->root_direntry);
	}
	else
	{
		is_unicode = 0;
		read_direntry(pvd->root_direntry);
	}

	// enter root
	read_directory(file[0].sector, file[0].size);
	
	int entries_found = files;
	if(entries_found <= 0){ return NULL; }
	
	int currentSelection = 0;
	while(1){
		entries_found = files;
		CLEAR();
		PRINT("browsing DVD:\n\n"); 
		int i = MIN(MAX(0,currentSelection-7),MAX(0,entries_found-14));
		int max = MIN(entries_found, MAX(currentSelection+7,14));
//		int i = MIN(MAX(0,currentSelection-floor((MAXLINES-0.5)/2)),MAX(0,entries_found-(MAXLINES-2)));
//		int max = MIN(entries_found, MAX(currentSelection+ceil((MAXLINES)/2)-1,MAXLINES-2));

		for(; i<max; ++i){
			if(i == currentSelection)
				sprintf(buffer, "->");
			else    sprintf(buffer, "  ");
			sprintf(buffer, "%s\t%-32s\t%s\n", buffer,
			        file[i].name,(file[i].flags & 2) ? "DIR" : "");
			PRINT(buffer);
		}
				// Wait for A/up/down press
		while (!(PAD_ButtonsHeld(0) & PAD_BUTTON_A) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_UP) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_DOWN));
		if(PAD_ButtonsHeld(0) & PAD_BUTTON_UP)   currentSelection = (--currentSelection < 0) ? entries_found-1 : currentSelection;
		if(PAD_ButtonsHeld(0) & PAD_BUTTON_DOWN) currentSelection = (currentSelection + 1) % entries_found;
		if(PAD_ButtonsHeld(0) & PAD_BUTTON_A){
			if(file[currentSelection].flags & 2){
				while (PAD_ButtonsHeld(0) & PAD_BUTTON_A);
				//CLEAR();
				read_directory(file[currentSelection].sector, file[currentSelection].size);
				currentSelection = 0;
			} else {
				CLEAR();
				sprintf(buffer,"SELECTING %s.\nPress B to continue.\n",file[currentSelection].name);
				PRINT(buffer);
				while (!(PAD_ButtonsHeld(0) & PAD_BUTTON_B));
				rom_sizeDVD = file[currentSelection].size;
				rom_offsetDVD = file[currentSelection].sector*2048;
				return file[currentSelection].name; 
				
			}
		}
		/*** Wait for up/down button release ***/
		while (!(!(PAD_ButtonsHeld(0) & PAD_BUTTON_A) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_UP) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_DOWN)));
	}
	
}

