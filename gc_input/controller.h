/* controller.h - Standard prototypes for accessing different controllers
   by Mike Slegeir for Mupen64-GC
 */

#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "../main/winlnxdefs.h"
#include "Controller_#1.1.h"

extern char padNeedScan, wpadNeedScan;

typedef struct {
	// Call GetKeys to read in BUTTONS for a controller of this type
	// You should pass in controller num for this type
	//   Not for the player number assigned
	//   (eg use GC Controller 1, not player 1)
	int (*GetKeys)(int, BUTTONS*);
	// Interactively configure the button mapping
	void (*configure)(int);
	// Initialize the controllers, filling out available
	void (*init)(void);
	// Assign actual controller to virtual controller
	void (*assign)(int,int);
	// Pause/Resume a controller
	void (*pause)(int);
	void (*resume)(int);
	// Rumble controller (0 stops rumble)
	void (*rumble)(int, int);
	// Controllers plugged in/available of this type
	char available[4];
} controller_t;

#endif
