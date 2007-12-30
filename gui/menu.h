/* menu.h - Main menu
   by Mike Slegeir for Mupen64-GC
 */

#ifndef MENU_H
#define MENU_H

typedef struct menu_item_t {
	char* caption;
	char  hasSubmenu; // TODO: Change this to attr: HAS_SUBMENU, SPECIAL, CENTERED
	union {
		struct {
			unsigned int        num_items;
			struct menu_item_t* items;
		} subMenu;
		char* (*func)(); // Returns a feedback string
	};
} menu_item;

// Initializes the menu, call first
void menuInit(void);
// Displays the menu via GUI
void menuDisplay(void);
// Moves the cursor by dx items
void menuNavigate(int dx);
// Select the item currently highlighted
void menuSelect(void);
// Go back in the menu (up a level)
void menuBack(void);

#endif
