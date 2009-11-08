/* controller.h - Standard prototypes for accessing different controllers
   by Mike Slegeir for Mupen64-GC
 */

#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "../main/winlnxdefs.h"
#include "Controller_#1.1.h"

extern char padNeedScan, wpadNeedScan;
extern u32 gc_connected;

void control_info_init(void);

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

typedef struct _virtualControllers_t {
	BOOL          inUse;   // This virtual controller is being controlled
	controller_t* control; // The type of controller being used
	int           number;  // The physical controller number
} virtualControllers_t;

extern virtualControllers_t virtualControllers[4];

// List of all the defined controller_t's
#if defined(WII) && !defined(NO_BT)

#define num_controller_t 3
extern controller_t controller_GC;
extern controller_t controller_Classic;
extern controller_t controller_WiimoteNunchuk;
extern controller_t* controller_ts[num_controller_t];

#else // WII && !NO_BT

#define num_controller_t 1
extern controller_t controller_GC;
controller_t* controller_ts[num_controller_t];

#endif // WII && !NO_BT

void init_controller_ts(void);
void assign_controller(int whichVirtual, controller_t*, int whichPhysical);
void unassign_controller(int whichVirtual);

#endif
