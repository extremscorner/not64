#include <stdio.h>
#include <gccore.h>		/*** Wrapper to include common libogc headers ***/
#include <ogcsys.h>		/*** Needed for console support ***/
#include <sdcard.h>		/*** SDCard Header. Remember to add -lsdcard in your Makefile ***/
#include <stdlib.h>

#define CLEAR() printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n")

char* textFileBrowser(char* directory){
	DIR *sddir = NULL;
	
	int entries_found = SDCARD_ReadDir(directory, (DIR **) & sddir);
	if(entries_found <= 0){ if(sddir) free(sddir); return NULL; }
	
	int currentSelection = 0;
	while(1){
		CLEAR();
		printf("browsing %s:\n", directory); 
		int i = MIN(MAX(0,currentSelection-13),MAX(0,entries_found-13));
		int max = MIN(entries_found, currentSelection+25);
		for(; i<max; ++i){
			if(i == currentSelection)
				printf("*");
			printf("\t%-32s\t%s\n", sddir[i].fname, (sddir[i].fattr&SDCARD_ATTR_DIR) ? "DIR" : "");
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
				printf("MOVING TO %s. Press B\n",newDir);
				while (!(PAD_ButtonsHeld(0) & PAD_BUTTON_B));
				return textFileBrowser(newDir);
			} else {
				char* newDir = malloc(SDCARD_MAX_PATH_LEN);
				sprintf(newDir, "%s\\%s", directory, sddir[currentSelection].fname);
				if(sddir) free(sddir);
				CLEAR();
				printf("SELECTING %s. Press B\n",newDir);
				while (!(PAD_ButtonsHeld(0) & PAD_BUTTON_B));
				return newDir;
			}
		}
	}
}

