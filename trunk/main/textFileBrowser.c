#include <stdio.h>
#include <math.h>
#include <gccore.h>		/*** Wrapper to include common libogc headers ***/
#include <ogcsys.h>		/*** Needed for console support ***/
#include <sdcard.h>		/*** SDCard Header. Remember to add -lsdcard in your Makefile ***/
#include <stdlib.h>
#include "gc_dvd.h"

#ifdef USE_GUI
#include "../gui/GUI.h"
#define MAXLINES 17
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
		int i = MIN(MAX(0,currentSelection-floor((MAXLINES-0.5)/2)),MAX(0,entries_found-(MAXLINES-2)));
		int max = MIN(entries_found, MAX(currentSelection+ceil((MAXLINES)/2)-1,MAXLINES-2));
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
	DVD_Mount ();
	PRINT("Reading DVD...\n");

	int sector = 16;
	static unsigned char buffer[2048] __attribute__((aligned(32)));

	struct pvd_s* pvd = 0;
	struct pvd_s* svd = 0;
	while (sector < 32)
	{
		if (read_sector(buffer, sector))
			PRINT("FATAL ERROR...\n");
		if (!memcmp(((struct pvd_s *)buffer)->id, "\2CD001\1", 8))
		{
			svd = (void*)buffer;
			break;
		}
		++sector;
	}

	if (!svd)
	{
		sector = 16;
		while (sector < 32)
		{
			if (read_sector(buffer, sector))
				PRINT("FATAL ERROR...\n");

			if (!memcmp(((struct pvd_s *)buffer)->id, "\1CD001\1", 8))
			{
				pvd = (void*)buffer;
				break;
			}
			++sector;
		}
	}

	if ((!pvd) && (!svd))
	{
		PRINT("No ISO9660 DVD Found!\n");
		return;
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
		int i = MIN(MAX(0,currentSelection-floor((MAXLINES-0.5)/2)),MAX(0,entries_found-(MAXLINES-2)));
		int max = MIN(entries_found, MAX(currentSelection+ceil((MAXLINES)/2)-1,MAXLINES-2));
		for(; i<max; ++i){
			if(i == currentSelection)
				sprintf(buffer, "->");
			else    sprintf(buffer, "  ");
			sprintf(buffer, "%s\t%-32s\t%s\n", buffer,
			        file[i].name,(file[i].flags & 2) ? "DIR" : "");
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

