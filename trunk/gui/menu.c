/* menu.c - Main menu and submenus
   by Mike Slegeir for Mupen64-GC
 */

#include "menu.h"
#include "GUI.h"
#include "../fileBrowser/fileBrowser.h"
#include "../fileBrowser/fileBrowser-SD.h"
#include "../fileBrowser/fileBrowser-DVD.h"
#include "../fileBrowser/fileBrowser-CARD.h"
#include "menuFileBrowser.h"

// -- ACTUAL MENU LAYOUT --

#define MAIN_MENU_SIZE 7

/* Message menu_item: used for feedback, set the caption to what you want it to say */

static menu_item _message_item =
	{ NULL,
	  MENU_ATTR_NONE,
	  { .func = menuBack }
	 };

static menu_item message_item = 
	{ NULL, // caption
	  MENU_ATTR_HASSUBMENU,  // attr
	 { 1, // num_items
	   &_message_item
	  }
	 };
static void setMessage( char* msg ){
	_message_item.caption = msg;
}	

/* End of Message menu_item */

/* Example - "Play Game" */
extern BOOL hasLoadedROM;
	static char* playGame_func(){
		if(!hasLoadedROM) return "Please load a ROM first";
		
		GUI_toggle();
		go();
		GUI_toggle();
		return NULL;
	}

#define PLAY_GAME_INDEX MAIN_MENU_SIZE - 1 /* We want it to be the last item */
#define PLAY_GAME_ITEM \
	{ "Play Game", /* caption */ \
	  MENU_ATTR_SPECIAL, /* attr */ \
	  { .func = playGame_func } /* func */ \
	 }

/* End of "Play Game" example */

/* "Select CPU Mode" menu item */

	extern unsigned long dynacore;
	static char* choose_PureInterpreter(){
		dynacore = 2;
		return "Running Pure Interpreter Mode";
	}
	static char* choose_Dynarec(){
		dynacore = 1;
		return "Running Dynarec Mode";
	}
	static char* choose_Interpreter(){
		dynacore = 0;
		return "Running Interpreter Mode";
	}
	
	static menu_item selectCPU_subMenu[3] =
		{{ "Pure Interpreter",
		   MENU_ATTR_SPECIAL,
		   { .func = choose_PureInterpreter }
		  },
		  
		 { "Dynamic Recompiler",
		   MENU_ATTR_NONE,
		   { .func = choose_Dynarec }
		  },
		 
		 { "Interpreter",
		   MENU_ATTR_NONE,
		   { .func = choose_Interpreter }
		  }
		 };

#define SELECT_CPU_INDEX 3
#define SELECT_CPU_ITEM \
	{ "Select CPU Core", /* caption */ \
	  MENU_ATTR_HASSUBMENU, /* attr */ \
	  /* subMenu */ \
	  { 3, /* num_items */ \
	    &selectCPU_subMenu[0] /* items */ \
	   } \
	 }	

/* End of "Select CPU" item */

/* "Toggle Audio" menu item */

	extern char  audioEnabled;
	static char* toggleAudio_func();
	static char  toggleAudio_strings[2][30]  = 
		{ "Toggle Audio (currently off)",
		  "Toggle Audio (currently on)"};

#define TOGGLE_AUDIO_INDEX MAIN_MENU_SIZE - 3
#define TOGGLE_AUDIO_ITEM \
	{ &toggleAudio_strings[0][0], \
	  MENU_ATTR_NONE, \
	  { .func = toggleAudio_func } \
	 }

/* End of "Toggle Audio" item */

/* "Load ROM" menu item */
	#include "../main/rom.h"
	char* textFileBrowser(char*);
	char* textFileBrowserDVD();
	static inline void menuStack_push(menu_item*);

	static char* loadROMSD_func(){
		// Change all the romFile pointers
		romFile_topLevel = &topLevel_SD_SlotA;
		romFile_readDir  = fileBrowser_SD_readDir;
		romFile_readFile = fileBrowser_SD_readFile;
		romFile_seekFile = fileBrowser_SD_seekFile;
		romFile_init     = fileBrowser_SD_init;
		romFile_deinit   = fileBrowser_SD_deinit;
		// Then push the file browser onto the menu
		menuStack_push( menuFileBrowser(romFile_topLevel) );
		
		return NULL;
	}
	static char* loadROMDVD_func(){
		// Change all the romFile pointers
		romFile_topLevel = &topLevel_DVD;
		romFile_readDir  = fileBrowser_DVD_readDir;
		romFile_readFile = fileBrowser_DVD_readFile;
		romFile_seekFile = fileBrowser_DVD_seekFile;
		romFile_init     = fileBrowser_DVD_init;
		romFile_deinit   = fileBrowser_DVD_deinit;
		// Then push the file browser onto the menu
		menuStack_push( menuFileBrowser(romFile_topLevel) );
		
		return NULL;
	}
	
	static menu_item loadROM_submenu[2] =
		{{ "Load from SD",
		   MENU_ATTR_NONE,
		   { .func = loadROMSD_func }
		  },
		 { "Load from DVD",
		   MENU_ATTR_NONE,
		   { .func = loadROMDVD_func }
		  }
		 };

#define LOAD_ROM_INDEX 0
#define LOAD_ROM_ITEM \
	{ "Load ROM", \
	  MENU_ATTR_HASSUBMENU, \
	  { 2, \
	    &loadROM_submenu[0] \
	   } \
	 }

/* End of "Load ROM" item */

/* "Load Save File" item */
#include "../gc_memory/Saves.h"

	// NOTE: I assume an even item # = Slot A, OW = B
	static char* loadSaveSD_func(int item_num){
		if(!hasLoadedROM) return "Please load a ROM first";
		// Adjust saveFile pointers
		saveFile_dir = (item_num%2) ? &saveDir_SD_SlotB : &saveDir_SD_SlotA;
		saveFile_readFile  = fileBrowser_SD_readFile;
		saveFile_writeFile = fileBrowser_SD_writeFile;
		saveFile_init      = 0;
		saveFile_deinit    = 0;
		
		// Try loading everything
		int result = 0;
		result += loadEeprom(saveFile_dir);
		result += loadSram(saveFile_dir);
		result += loadMempak(saveFile_dir);
		result += loadFlashram(saveFile_dir);
		
		return result ? "Loaded save from SD card" : "No saves found on SD card";
	}
	static char* loadSaveCARD_func(int item_num){
		if(!hasLoadedROM) return "Please load a ROM first";
		// Adjust saveFile pointers
		saveFile_dir = (item_num%2) ? &saveDir_CARD_SlotB : &saveDir_CARD_SlotA;
		saveFile_readFile  = fileBrowser_CARD_readFile;
		saveFile_writeFile = fileBrowser_CARD_writeFile;
		saveFile_init      = fileBrowser_CARD_init;
		saveFile_deinit    = fileBrowser_CARD_deinit;
		
		// Try loading everything
		int result = 0;
		saveFile_init(saveFile_dir);
		result += loadEeprom(saveFile_dir);
		result += loadSram(saveFile_dir);
		result += loadMempak(saveFile_dir);
		result += loadFlashram(saveFile_dir);
		saveFile_deinit(saveFile_dir);
		
		return result ? "Loaded save from memcard" : "No saves found on memcard";
	}

	static menu_item loadSave_submenu[4] =
		{{ "SD in Slot A",
		   MENU_ATTR_NONE,
		   { .func = loadSaveSD_func }
		  },
		 { "SD in Slot B",
		   MENU_ATTR_NONE,
		   { .func = loadSaveSD_func }
		  },
		 { "Memory Card in Slot A",
		   MENU_ATTR_NONE,
		   { .func = loadSaveCARD_func }
		  },
		 { "Memory Card in Slot B",
		   MENU_ATTR_NONE,
		   { .func = loadSaveCARD_func }
		  },
		 };

#define LOAD_SAVE_INDEX 1
#define LOAD_SAVE_ITEM \
	{ "Load Save File", \
	  MENU_ATTR_HASSUBMENU, \
	  { 4, \
	    &loadSave_submenu[0] \
	   } \
	 }

/* End of "Load Save File" item */

/* "Save Game" item */

	// NOTE: I assume an even item # = Slot A, OW = B
	static char* saveGameSD_func(int item_num){
		// Adjust saveFile pointers
		saveFile_dir = (item_num%2) ? &saveDir_SD_SlotB : &saveDir_SD_SlotA;
		saveFile_readFile  = fileBrowser_SD_readFile;
		saveFile_writeFile = fileBrowser_SD_writeFile;
		saveFile_init      = fileBrowser_SD_init;
		saveFile_deinit    = fileBrowser_SD_deinit;
		
		// Try loading everything
		int result = 0;
		result += saveEeprom(saveFile_dir);
		result += saveSram(saveFile_dir);
		result += saveMempak(saveFile_dir);
		result += saveFlashram(saveFile_dir);
		
		return result ? "Saved game to SD card" : "Nothing to save";
	}
	static char* saveGameCARD_func(int item_num){
		// Adjust saveFile pointers
		saveFile_dir = (item_num%2) ? &saveDir_CARD_SlotB : &saveDir_CARD_SlotA;
		saveFile_readFile  = fileBrowser_CARD_readFile;
		saveFile_writeFile = fileBrowser_CARD_writeFile;
		saveFile_init      = fileBrowser_CARD_init;
		saveFile_deinit    = fileBrowser_CARD_deinit;
		
		// Try loading everything
		int result = 0;
		saveFile_init(saveFile_dir);
		result += saveEeprom(saveFile_dir);
		result += saveSram(saveFile_dir);
		result += saveMempak(saveFile_dir);
		result += saveFlashram(saveFile_dir);
		saveFile_deinit(saveFile_dir);
		
		return result ? "Saved game to memcard" : "Nothing to save";
	}

	static menu_item saveGame_submenu[4] =
		{{ "SD in Slot A",
		   MENU_ATTR_NONE,
		   { .func = saveGameSD_func }
		  },
		 { "SD in Slot B",
		   MENU_ATTR_NONE,
		   { .func = saveGameSD_func }
		  },
		 { "Memory Card in Slot A",
		   MENU_ATTR_NONE,
		   { .func = saveGameCARD_func }
		  },
		 { "Memory Card in Slot B",
		   MENU_ATTR_NONE,
		   { .func = saveGameCARD_func }
		  },
		 };

#define SAVE_GAME_INDEX 2
#define SAVE_GAME_ITEM \
	{ "Save Game", \
	  MENU_ATTR_HASSUBMENU, \
	  { 4, \
	    &saveGame_submenu[0] \
	   } \
	 }

/* End of "Save Game" item */

/* "Exit to SDLOAD" item */

	static void reload(){
		void (*rld)() = (void (*)()) 0x80001800;
		rld();
	}
#define EXIT_INDEX MAIN_MENU_SIZE -2
#define EXIT_ITEM \
	{ "Exit to SDLOAD", \
	  MENU_ATTR_NONE, \
	  { .func = reload } \
	 }

/* End of "Exit to SDLOAD" item */
	
static menu_item main_menu[MAIN_MENU_SIZE] = 
	{ LOAD_ROM_ITEM,
	  LOAD_SAVE_ITEM,
	  SAVE_GAME_ITEM,
	  SELECT_CPU_ITEM,
	  TOGGLE_AUDIO_ITEM,
	  EXIT_ITEM,
	  PLAY_GAME_ITEM
	 };


// Unfortunately, this has to be down here
static char* toggleAudio_func(){
	audioEnabled ^= 1;
	main_menu[TOGGLE_AUDIO_INDEX].caption = &toggleAudio_strings[audioEnabled][0];
	return NULL;
}

// -- END MENU --



// -- MENU IMPLEMENTATION --

// -- MENU STACK --
// Allow 8 levels of submenus
#define MENU_STACK_SIZE 8
static menu_item*   menu_stack[MENU_STACK_SIZE];
static unsigned int stack_top_i;

static inline menu_item* menuStack_top(){ return menu_stack[stack_top_i];   }
static inline menu_item* menuStack_pop(){ return menu_stack[stack_top_i--]; }

static inline void menuStack_push(menu_item* item){ menu_stack[++stack_top_i] = item; }

static inline unsigned int menuStack_size(){ return stack_top_i+1; }

// -- END MENU STACK CODE --

static menu_item top_level =
	{ 0, // caption
	  MENU_ATTR_HASSUBMENU, // attr
	  // subMenu
	  { MAIN_MENU_SIZE, // num_items
	    &main_menu[0]   // items
	   }
	 };

static unsigned int selected_i;

void menuInit(void){
	// Set up stack
	stack_top_i = -1;
	menuStack_push( &top_level );
	selected_i = 0;
}

static void subMenuDisplay(int num_items, menu_item* itemz){
	GUI_clear();
	int i     = MAX( 0, MIN(num_items - GUI_TEXT_HEIGHT, (int)selected_i - GUI_TEXT_HEIGHT/2) );
	int limit = MIN( num_items, MAX(GUI_TEXT_HEIGHT, (int)selected_i + GUI_TEXT_HEIGHT/2) );
	
	for(; i<limit; ++i)
		GUI_print_color( itemz[i].caption, (i == selected_i) ?
		                   GUI_color_highlighted : (itemz[i].attr & MENU_ATTR_SPECIAL) ?
		                     GUI_color_special : GUI_color_default );
}

void menuDisplay(void){
	menu_item* curr = menuStack_top();
	GUI_centerText( !(curr->attr & MENU_ATTR_LEFTALIGN) );
	subMenuDisplay(curr->subMenu.num_items, curr->subMenu.items);
}

void menuNavigate(int d){
	selected_i = (selected_i + menuStack_top()->subMenu.num_items + d) %
	                menuStack_top()->subMenu.num_items;
}

void menuSelect(){
	menu_item* curr = menuStack_top();
	if(curr->subMenu.items[selected_i].attr & MENU_ATTR_HASSUBMENU){
		// We're moving to a new submenu, push it on our stack
		menuStack_push( &curr->subMenu.items[selected_i] );
		selected_i = 0;
	} else {
		// Otherwise, run the menu item's function
		setMessage( curr->subMenu.items[selected_i].func(selected_i) );
		if(_message_item.caption){
			menuBack();
			menuStack_push( &message_item );
		}
		selected_i = 0;
	}
}

void menuBack(){
	// Make sure we don't empy our stack, we always need to keep the top level
	if(menuStack_size() <= 1) return;
	// Otherwise, just pop
	menuStack_pop();
	selected_i = 0;
}

// -- END MENU IMPLEMENTATION --

