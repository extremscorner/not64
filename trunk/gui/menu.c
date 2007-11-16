/* menu.c - Main menu and submenus
   by Mike Slegeir for Mupen64-GC
 */

#include "menu.h"
#include "GUI.h"



// -- ACTUAL MENU LAYOUT --

#define MAIN_MENU_SIZE 0

/* Example - "Play Game" */

static void playGame_func(void){ /* Start up all the plugins and run go? */ }
static menu_item playGame_item =
	{ "Play Game", // caption
	  0,           // hasSubmenu
	  { .func = playGame_func } // func
	 };

/* End of "Play Game" example */

/* "Select CPU Mode" menu item */

	extern unsigned long dynacore;
	static void choose_PureInterpreter(void){ dynacore = 2; }
	static void choose_Dynarec(void){ dynacore = 1; }
	static void choose_Interpreter(void){ dynacore = 0; }
	
	static menu_item selectCPU_subMenu[3] =
		{{ "Pure Interpreter",
		   0,
		   { .func = choose_PureInterpreter }
		  },
		  
		 { "Dynamic Recompiler",
		   0,
		   { .func = choose_Dynarec }
		  },
		 
		 { "Interpreter",
		   0,
		   { .func = choose_Interpreter }
		  }
		 };

static menu_item selectCPU_item =
	{ "Select CPU Core", // caption
	  1, // hasSubmenu
	  // subMenu
	  { 3, // num_items
	    &selectCPU_subMenu[0] // items
	   }
	 };	

/* End of "Select CPU" item */

/* "Toggle Audio" menu item */

extern char audioEnabled;
static void toggleAudio_func(void);
static char toggleAudio_strings[2][30]  = 
	{ "Toggle Audio (currently off)",
	  "Toggle Audio (currently on)"};

static menu_item toggleAudio_item =
	{ &toggleAudio_strings[0][0],
	  0,
	  { .func = toggleAudio_func }
	 };

static void toggleAudio_func(void){
	audioEnabled ^= 1;
	toggleAudio_item.caption = &toggleAudio_strings[audioEnabled][0];
}

/* End of "Toggle Audio" item */

/* "Load ROM" menu item */
#include "../main/rom.h"
char* textFileBrowser(char*);
char* textFileBrowserDVD();

	static void loadROMSD_func(void){
		char* romfile = textFileBrowser("dev0:\\N64ROMS");
		rom_read(romfile);
		free(romfile);
	}
	static void loadROMDVD_func(void){
		rom_read( textFileBrowserDVD() );
	}
	
static menu_item loadROM_submenu[2] =
	{{ "Load from SD",
	   0,
	   { .func = loadROMSD_func }
	  },
	 { "Load from DVD",
	   0,
	   { .func = loadROMDVD_func }
	  }
	 };
	
static menu_item loadROM_item =
	{ "Load ROM",
	  1,
	  { 2,
	    &loadROM_submenu[0]
	   }
	 };

/* End of "Load ROM" item */
	
static menu_item main_menu[MAIN_MENU_SIZE] = 
	{ 
	 };

// -- END MENU --



// -- MENU IMPLEMENTATION --

// -- MENU STACK --
// Allow 8 levels of submenus
#define MENU_STACK_SIZE 8
static menu_item*   menu_stack[MENU_STACK_SIZE];
static unsigned int stack_top_i;

static inline menu_item* menuStack_top(){ return menu_stack[stack_top_i];   }
static inline menu_item* menuStack_pop(){ return menu_stack[stack_top_i--]; }

static inline void menuStack_push(menu_item* item){ menu_stack[stack_top_i++] = item; }

static inline unsigned int menuStack_size(){ return stack_top_i; }

// -- END MENU STACK CODE --

static menu_item top_level =
	{ 0, // caption
	  1, // hasSubmenu
	  // subMenu
	  { MAIN_MENU_SIZE, // num_items
	    &main_menu[0]   // items
	   }
	 };

void menuInit(void){
	// Set up stack
	stack_top_i = 0;
	menuStack_push( &top_level );
}

static unsigned int selected_i;

static void subMenuDisplay(unsigned int num_items, menu_item* itemz){
	GUI_clear();
	int i;
	for(i=0; i<num_items; ++i)
		GUI_print( itemz[i].caption );
}

void menuDisplay(void){
	menu_item* curr = menuStack_top();
	subMenuDisplay(curr->subMenu.num_items, curr->subMenu.items);
}

void menuNavigate(int d){
	selected_i = (selected_i + d) % menuStack_top()->subMenu.num_items;
}

void menuSelect(){
	menu_item* curr = menuStack_top();
	if(curr->subMenu.items[selected_i].hasSubmenu){
		// We're moving to a new submenu, push it on our stack
		menuStack_push( &curr->subMenu.items[selected_i] );
	} else {
		// Otherwise, run the menu item's function
		curr->subMenu.items[selected_i].func();
	}
}

void menuBack(){
	// Make sure we don't empy our stack, we always need to keep the top level
	if(menuStack_size() <= 1) return;
	// Otherwise, just pop
	menuStack_pop();
}

// -- END MENU IMPLEMENTATION --

