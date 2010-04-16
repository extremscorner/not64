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

enum {
	C_STICK_AS_C = 1, ANALOG_AS_ANALOG = 2,
	C_STICK_AS_ANALOG = 4, ANALOG_AS_C = 8,
};

u32 gc_connected;

static int _GetKeys(int Control, BUTTONS * Keys, controller_config_t* config)
{
	if(padNeedScan){ gc_connected = PAD_ScanPads(); padNeedScan = 0; }
	BUTTONS* c = Keys;
	memset(c, 0, sizeof(BUTTONS));

	controller_GC.available[Control] = (gc_connected & (1<<Control)) ? 1 : 0;
	if (!controller_GC.available[Control]) return 0;

	unsigned short b = PAD_ButtonsHeld(Control);
	int isHeld(unsigned short button){ return (b & button) == button; }
	
	c->R_DPAD       = isHeld(config->DR);
	c->L_DPAD       = isHeld(config->DL);
	c->D_DPAD       = isHeld(config->DD);
	c->U_DPAD       = isHeld(config->DU);
	
	c->START_BUTTON = isHeld(config->START);
	c->B_BUTTON     = isHeld(config->B);
	c->A_BUTTON     = isHeld(config->A);

	c->Z_TRIG       = isHeld(config->Z);
	c->R_TRIG       = isHeld(config->R);
	c->L_TRIG       = isHeld(config->L);

	if(config->flags & C_STICK_AS_C){
		s8 substickX = PAD_SubStickX(Control);
		c->R_CBUTTON = (substickX >  48) ? 1 : 0;
		c->L_CBUTTON = (substickX < -48) ? 1 : 0;
		s8 substickY = PAD_SubStickY(Control);
		c->D_CBUTTON = (substickY < -48) ? 1 : 0;
		c->U_CBUTTON = (substickY >  48) ? 1 : 0;
	} else if(config->flags & ANALOG_AS_C){
		s8 stickX = PAD_StickX(Control);
		c->R_CBUTTON = (stickX >  48) ? 1 : 0;
		c->L_CBUTTON = (stickX < -48) ? 1 : 0;
		s8 stickY = PAD_StickY(Control);
		c->D_CBUTTON = (stickY < -48) ? 1 : 0;
		c->U_CBUTTON = (stickY >  48) ? 1 : 0;
	} else {
		c->R_CBUTTON = isHeld(config->CR);
		c->L_CBUTTON = isHeld(config->CL);
		c->D_CBUTTON = isHeld(config->CD);
		c->U_CBUTTON = isHeld(config->CU);
	}

	if(config->flags & ANALOG_AS_ANALOG){
		c->X_AXIS = PAD_StickX(Control);
		c->Y_AXIS = PAD_StickY(Control);
	} else if(config->flags & C_STICK_AS_ANALOG){
		c->X_AXIS = PAD_SubStickX(Control);
		c->Y_AXIS = PAD_SubStickY(Control);
	}

	// X+Y quits to menu
	return isHeld(config->exit);
}

static void pause(int Control){
	PAD_ControlMotor(Control, PAD_MOTOR_STOP);
}

static void resume(int Control){ }

static void rumble(int Control, int rumble){
	PAD_ControlMotor(Control, rumble ? PAD_MOTOR_RUMBLE : PAD_MOTOR_STOP);
}

static void configure(int Control){
	// Don't know how this should be integrated
}

static void assign(int p, int v){
	// Nothing to do here
}

static void init(void);

static controller_config_t configs[] = {
	{
		.DL = PAD_BUTTON_LEFT, .DR = PAD_BUTTON_RIGHT,
		.DU = PAD_BUTTON_UP, .DD = PAD_BUTTON_DOWN,
		.A = PAD_BUTTON_A, .B = PAD_BUTTON_B,
		.START = PAD_BUTTON_START,
		.L = PAD_TRIGGER_L, .R = PAD_TRIGGER_R,
		.Z = PAD_TRIGGER_Z,
		.CL = -1, .CR = -1, .CU = -1, .CD = -1,
		.flags = C_STICK_AS_C | ANALOG_AS_ANALOG,
		.exit = PAD_BUTTON_X | PAD_BUTTON_Y,
		.description = "Default settings"
	},
};

controller_t controller_GC =
	{ _GetKeys,
	  configure,
	  init,
	  assign,
	  pause,
	  resume,
	  rumble,
	  {0, 0, 0, 0},
	  sizeof(configs)/sizeof(configs[0]),
	  configs
	 };

static void init(void){

	if(padNeedScan){ gc_connected = PAD_ScanPads(); padNeedScan = 0; }

	int i;
	for(i=0; i<4; ++i)
		controller_GC.available[i] = (gc_connected & (1<<i));
}
