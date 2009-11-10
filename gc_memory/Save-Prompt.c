/**
 * Wii64 - Save-Prompt.c (Deprecated)
 * Copyright (C) 2007, 2008, 2009 Mike Slegeir
 * 
 * prompts the user which location to save to
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

/* 
   TODO: Verify save location so data is not lost?
*/

#include <stdio.h>
#include <string.h>

#include <gcutil.h>
#include <sdcard.h>
#include <ogc/card.h>
#include <ogc/pad.h>
#include "Saves.h"

#ifdef USE_GUI
#include "../gui/GUI.h"
#define PRINT GUI_print
#else
#define PRINT printf
#endif

#define MAX_SAVE_PATH       64

// globals set when user chooses location
int  savetype;
char savepath[MAX_SAVE_PATH];

// Used for Memory card working area
extern unsigned char SysArea[CARD_WORKAREA] ATTRIBUTE_ALIGN (32);
extern void card_removed_cb(s32 chn, s32 result);

// This should be called when the game begins
// or when the user wants to save somewhere else
void select_location(void){
	int selection = 0;
	char buffer[64];
	
	// Get user selection here
	BOOL inited = FALSE;
	while(!inited){

	PRINT("--SAVE GAME--\n");
	PRINT("Choose slot:\n"
	      "  (A) Slot A\n"
	      "  (B) Slot B\n");
	while(!(PAD_ButtonsHeld(0) & PAD_BUTTON_A || 
	        PAD_ButtonsHeld(0) & PAD_BUTTON_B )); // Wait for input
	if(PAD_ButtonsHeld(0) & PAD_BUTTON_A) selection |= SELECTION_SLOT_A;
	else                                  selection |= SELECTION_SLOT_B;
	while((PAD_ButtonsHeld(0) & PAD_BUTTON_A || 
	       PAD_ButtonsHeld(0) & PAD_BUTTON_B )); // Wait for button release

	PRINT("Choose card type:\n"
	      "  (A) Memory Card\n"
	      "  (B) SD Card\n");
	while(!(PAD_ButtonsHeld(0) & PAD_BUTTON_A || 
	        PAD_ButtonsHeld(0) & PAD_BUTTON_B )); // Wait for input
	if(PAD_ButtonsHeld(0) & PAD_BUTTON_A) selection |= SELECTION_TYPE_MEM;
	else                                  selection |= SELECTION_TYPE_SD;
	while((PAD_ButtonsHeld(0) & PAD_BUTTON_A || 
	       PAD_ButtonsHeld(0) & PAD_BUTTON_B )); // Wait for button release

	sprintf(buffer, "Saving to %s in slot %c...\n",
	        (selection & SELECTION_TYPE_SD) ? "SD card" : "memory card",
	        (selection & SELECTION_SLOT_B)  ? 'B'       : 'A');
	PRINT(buffer);
	
	// Determine savepath
	if(selection & SELECTION_TYPE_SD){
		SDCARD_Init();
		if(selection & SELECTION_SLOT_B)
			strcpy(savepath, "dev1:\\N64SAVES\\");
		else
			strcpy(savepath, "dev0:\\N64SAVES\\");
		inited = TRUE;
	} else {
		CARD_Init("N64E", "OS");
		savepath[0] = 0;
		if(CARD_Mount((selection&SELECTION_SLOT_B) ? CARD_SLOTB : CARD_SLOTA,
			                             SysArea, card_removed_cb) < 0){
			                             
			sprintf(buffer, "Cannot mount Memory Card in Slot %c\n",
			        (selection&SELECTION_SLOT_B) ? 'B' : 'A');
			PRINT(buffer);

		} else inited = TRUE;
	}
	}
	
	savetype = selection;
}
