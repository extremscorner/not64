/**
 * Wii64 - controller-GC.c
 * Copyright (C) 2007, 2008, 2009 Mike Slegeir
 * Copyright (C) 2007, 2008, 2009 sepp256
 * 
 * Gamecube controller input module
 *
 * Wii64 homepage: http://www.emulatemii.com
 * email address: tehpola@gmail.com
 *                sepp256@gmail.com
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


#include <string.h>
#include <ogc/pad.h>
#include "controller.h"

u32 gc_connected;

static int _GetKeys(int Control, BUTTONS * Keys )
{
	if(padNeedScan){ gc_connected = PAD_ScanPads(); padNeedScan = 0; }
	BUTTONS* c = Keys;
	memset(c, 0, sizeof(BUTTONS));

	controller_GC.available[Control] = (gc_connected & (1<<Control)) ? 1 : 0;
	if (!controller_GC.available[Control]) return 0;

	unsigned short b = PAD_ButtonsHeld(Control);
	int isHeld(unsigned short button){ return (b & button) == button; }
	
	c->R_DPAD       = isHeld(PAD_BUTTON_RIGHT);
	c->L_DPAD       = isHeld(PAD_BUTTON_LEFT);
	c->D_DPAD       = isHeld(PAD_BUTTON_DOWN);
	c->U_DPAD       = isHeld(PAD_BUTTON_UP);
	c->START_BUTTON = isHeld(PAD_BUTTON_START);
	c->B_BUTTON     = isHeld(PAD_BUTTON_B);
	c->A_BUTTON     = isHeld(PAD_BUTTON_A);

	c->Z_TRIG       = isHeld(PAD_TRIGGER_Z);
	c->R_TRIG       = isHeld(PAD_TRIGGER_R);
	c->L_TRIG       = isHeld(PAD_TRIGGER_L);

	s8 substickX = PAD_SubStickX(Control);
	c->R_CBUTTON    = (substickX >  48) ? 1 : 0;
	c->L_CBUTTON    = (substickX < -48) ? 1 : 0;
	s8 substickY = PAD_SubStickY(Control);
	c->D_CBUTTON    = (substickY < -48) ? 1 : 0;
	c->U_CBUTTON    = (substickY >  48) ? 1 : 0;

	c->X_AXIS       = PAD_StickX(Control);
	c->Y_AXIS       = PAD_StickY(Control);

	// X+Y quits to menu
	return isHeld(PAD_BUTTON_X | PAD_BUTTON_Y);
}

static void pause(int Control){
	PAD_ControlMotor(Control, PAD_MOTOR_STOP);
}

static void resume(int Control){ }

static void rumble(int Control, int rumble){
	if(rumble) PAD_ControlMotor(Control, PAD_MOTOR_RUMBLE);
	else PAD_ControlMotor(Control, PAD_MOTOR_STOP);
}

static void configure(int Control){
	// Don't know how this should be integrated
}

static void assign(int p, int v){
	// Nothing to do here
}

static void init(void);

controller_t controller_GC =
	{ _GetKeys,
	  configure,
	  init,
	  assign,
	  pause,
	  resume,
	  rumble,
	  {0, 0, 0, 0}
	 };

static void init(void){

	if(padNeedScan){ gc_connected = PAD_ScanPads(); padNeedScan = 0; }

	int i;
	for(i=0; i<4; ++i)
		controller_GC.available[i] = (gc_connected & (1<<i));
}
