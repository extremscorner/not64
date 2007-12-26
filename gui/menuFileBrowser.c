/* menuFileBrowser.c - File browser using the menu system
   by Mike Slegeir for Mupen64-GC
 */

#include <string.h>
#include "menu.h"
#include "../fileBrowser/fileBrowser.h"
#include "../fileBrowser/fileBrowser-SD.h"
#include "../fileBrowser/fileBrowser-DVD.h"
#include "menuFileBrowser.h"

static menu_item  currentDirMenu =
	{ NULL, // caption
	  1,    // hasSubmenu
	  // subMenu
	  { 0,    // num_items
	    NULL  // items
	   }
	 };
static menu_item*        menu_items;
static fileBrowser_file* dir_entries;

void loadROM(fileBrowser_file*);
static void recurseOrSelect(int i){
	if(dir_entries[i].attr & FILE_BROWSER_ATTR_DIR){
		// Here we are 'recursing' into a subdirectory
		// We have to do a little dance here to avoid a dangling pointer
		fileBrowser_file* dir = malloc( sizeof(fileBrowser_file) );
		memcpy(dir, dir_entries+i, sizeof(fileBrowser_file));
		menuFileBrowser(dir);
		free(dir);
	} else {
		// We must select this file
		// TODO: Probably some sort of feedback via GUI if possible
		// FIXME: I've 'hardwired' loadROM into this function
		//          but it'd be relatively simple to adapt
		loadROM( &dir_entries[i] );
		// And a simple hack to get back out of the file browser
		menuBack();
	}
}

static void inline fillItems(int num_entries, fileBrowser_file* entries){
	int i;
	for(i=0; i<num_entries; ++i){
		// FIXME: The caption should reflect if its a DIR
		//          or it needs some sort of marker
		menu_items[i].caption = entries[i].name;
		
		// Here we want to extract from the absolute path
		//   just the filename
		// First we move the pointer all the way to the end
		//   of the the string so we can work our way back
		while( *menu_items[i].caption ) ++menu_items[i].caption;
		// Now, just move it back to the last '/' or the start
		//   of the string
		while( menu_items[i].caption != entries[i].name &&
		       *(menu_items[i].caption-1) != '/')
			--menu_items[i].caption;
		
		menu_items[i].hasSubmenu = 0;
		menu_items[i].func       = recurseOrSelect;
	}
	
	currentDirMenu.subMenu.num_items = num_entries;
	currentDirMenu.subMenu.items     = menu_items;
}

menu_item* menuFileBrowser(fileBrowser_file* dir){
	// Free the old menu stuff
	if(menu_items){  free(menu_items);  menu_items  = NULL; }
	if(dir_entries){ free(dir_entries); dir_entries = NULL; }
	
	// Read the directories and return on error
	int num_entries = romFile_readDir(dir, &dir_entries);
	if(num_entries <= 0){ if(dir_entries) free(dir_entries); return; }
	
	// Allocate and fill the entries
	menu_items = malloc(num_entries * sizeof(menu_item));
	fillItems(num_entries, dir_entries);
	
	// This should be pushed onto the menu stack
	return &currentDirMenu;
}

