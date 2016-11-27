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
#include <ogc/si.h>
#include <ogc/pad.h>
#include "controller.h"

enum {
	STICK_AS_ANALOG = 1, SUBSTICK_AS_ANALOG = 2, BUTTON_AS_ANALOG = 3,
};

static button_t buttons[] = {
	{  0, ~0,                 "None" },
	{  1, PAD_BUTTON_UP,      "D-Up" },
	{  2, PAD_BUTTON_LEFT,    "D-Left" },
	{  3, PAD_BUTTON_RIGHT,   "D-Right" },
	{  4, PAD_BUTTON_DOWN,    "D-Down" },
	{  5, PAD_TRIGGER_Z,      "Z" },
	{  6, PAD_TRIGGER_L,      "L" },
	{  7, PAD_TRIGGER_R,      "R" },
	{  8, PAD_BUTTON_A,       "A" },
	{  9, PAD_BUTTON_B,       "B" },
	{ 10, PAD_BUTTON_X,       "X" },
	{ 11, PAD_BUTTON_Y,       "Y" },
	{ 12, PAD_BUTTON_START,   "Start" },
	{ 13, PAD_SUBSTICK_UP,    "C-Up" },
	{ 14, PAD_SUBSTICK_LEFT,  "C-Left" },
	{ 15, PAD_SUBSTICK_RIGHT, "C-Right" },
	{ 16, PAD_SUBSTICK_DOWN,  "C-Down" },
	{ 17, PAD_STICK_UP,       "A-Up" },
	{ 18, PAD_STICK_LEFT,     "A-Left" },
	{ 19, PAD_STICK_RIGHT,    "A-Right" },
	{ 20, PAD_STICK_DOWN,     "A-Down" },
};

static button_t analog_sources[] = {
	{ 0, STICK_AS_ANALOG,    "Analog Stick" },
	{ 1, SUBSTICK_AS_ANALOG, "C-Stick" },
	{ 2, BUTTON_AS_ANALOG,   "D-Pad" },
};

static button_t menu_combos[] = {
	{ 0, PAD_BUTTON_X|PAD_BUTTON_Y,      "X+Y" },
	{ 1, PAD_BUTTON_START|PAD_BUTTON_X,  "Start+X" },
	{ 2, PAD_BUTTON_START|PAD_BUTTON_Y,  "Start+Y" },
	{ 3, PAD_BUTTON_START|PAD_TRIGGER_Z, "Start+Z" },
};

static unsigned int getButtons(int Control){
	return PAD_ButtonsHeld(Control);
}

static int available(int Control) {
	u32 type;
	type = SI_GetType(Control);
	if ((type & SI_TYPE_MASK) == SI_TYPE_GC && (type & SI_GC_STANDARD)) {
		controller_GC.available[Control] = 1;
		return 1;
	} else {
		controller_GC.available[Control] = 0;
		return 0;
	}
}

static int _GetKeys(int Control, BUTTONS * Keys, controller_config_t* config)
{
	BUTTONS* c = Keys;
	memset(c, 0, sizeof(BUTTONS));

	PAD_ScanPads();
	if(!available(Control))
		return 0;

	unsigned int b = getButtons(Control);
	inline int isHeld(button_tp button){
		return (b & button->mask) == button->mask;
	}
	
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

	c->R_CBUTTON    = isHeld(config->CR);
	c->L_CBUTTON    = isHeld(config->CL);
	c->D_CBUTTON    = isHeld(config->CD);
	c->U_CBUTTON    = isHeld(config->CU);

	if(config->analog->mask == STICK_AS_ANALOG){
		c->X_AXIS = PAD_StickX(Control);
		c->Y_AXIS = PAD_StickY(Control);
	} else if(config->analog->mask == SUBSTICK_AS_ANALOG){
		c->X_AXIS = PAD_SubStickX(Control);
		c->Y_AXIS = PAD_SubStickY(Control);
	} else if(config->analog->mask == BUTTON_AS_ANALOG){
		if(b & PAD_BUTTON_RIGHT)
			c->X_AXIS = +80;
		else if(b & PAD_BUTTON_LEFT)
			c->X_AXIS = -80;
		else
			c->X_AXIS = 0;

		if(b & PAD_BUTTON_UP)
			c->Y_AXIS = +80;
		else if(b & PAD_BUTTON_DOWN)
			c->Y_AXIS = -80;
		else
			c->Y_AXIS = 0;
	}
	if(config->invertedY) c->Y_AXIS = -c->Y_AXIS;

	// Return whether the exit button(s) are pressed
	return isHeld(config->exit);
}

static void pause(int Control){
	PAD_ControlMotor(Control, PAD_MOTOR_STOP);
}

static void resume(int Control){ }

static void rumble(int Control, int rumble){
	PAD_ControlMotor(Control, rumble ? PAD_MOTOR_RUMBLE : PAD_MOTOR_STOP);
}

static void configure(int Control, controller_config_t* config){
	// Don't know how this should be integrated
}

static void assign(int p, int v){
	// Nothing to do here
}

static void init(void);
static void refreshAvailable(void);

controller_t controller_GC =
	{ 'G',
	  _GetKeys,
	  configure,
	  assign,
	  pause,
	  resume,
	  rumble,
	  refreshAvailable,
	  {0, 0, 0, 0},
	  sizeof(buttons)/sizeof(buttons[0]),
	  buttons,
	  sizeof(analog_sources)/sizeof(analog_sources[0]),
	  analog_sources,
	  sizeof(menu_combos)/sizeof(menu_combos[0]),
	  menu_combos,
	  { .DU        = &buttons[1],  // D-Pad Up
	    .DL        = &buttons[2],  // D-Pad Left
	    .DR        = &buttons[3],  // D-Pad Right
	    .DD        = &buttons[4],  // D-Pad Down
	    .Z         = &buttons[6],  // Z
	    .L         = &buttons[5],  // Left Trigger
	    .R         = &buttons[7],  // Right Trigger
	    .A         = &buttons[8],  // A
	    .B         = &buttons[9],  // B
	    .START     = &buttons[12], // Start
	    .CU        = &buttons[13], // C-Stick Up
	    .CL        = &buttons[14], // C-Stick Left
	    .CR        = &buttons[15], // C-Stick Right
	    .CD        = &buttons[16], // C-Stick Down
	    .analog    = &analog_sources[0],
	    .exit      = &menu_combos[0],
	    .invertedY = 0,
	  }
	 };

static void refreshAvailable(void){
	int i;
	for(i=0; i<4; ++i){
		available(i);
	}
}
