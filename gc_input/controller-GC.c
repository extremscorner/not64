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
	ANALOG_AS_ANALOG = 1, C_STICK_AS_ANALOG = 2,
};

enum {
	ANALOG_L  = 0x01 << 16,
	ANALOG_R  = 0x02 << 16,
	ANALOG_U  = 0x04 << 16,
	ANALOG_D  = 0x08 << 16,
	C_STICK_L = 0x10 << 16,
	C_STICK_R = 0x20 << 16,
	C_STICK_U = 0x40 << 16,
	C_STICK_D = 0x80 << 16,
};

static button_t buttons[] = {
	{  0, ~0,                "None" },
	{  1, PAD_BUTTON_UP,    "D-Up" },
	{  2, PAD_BUTTON_LEFT,  "D-Left" },
	{  3, PAD_BUTTON_RIGHT, "D-Right" },
	{  4, PAD_BUTTON_DOWN,  "D-Down" },
	{  5, PAD_TRIGGER_Z,    "Z" },
	{  6, PAD_TRIGGER_L,    "L" },
	{  7, PAD_TRIGGER_R,    "R" },
	{  8, PAD_BUTTON_A,     "A" },
	{  9, PAD_BUTTON_B,     "B" },
	{ 10, PAD_BUTTON_X,     "X" },
	{ 11, PAD_BUTTON_Y,     "Y" },
	{ 12, PAD_BUTTON_START, "Start" },
	{ 13, C_STICK_U,        "C-Up" },
	{ 14, C_STICK_L,        "C-Left" },
	{ 15, C_STICK_R,        "C-Right" },
	{ 16, C_STICK_D,        "C-Down" },
	{ 17, ANALOG_U,         "A-Up" },
	{ 18, ANALOG_L,         "A-Left" },
	{ 19, ANALOG_R,         "A-Right" },
	{ 20, ANALOG_D,         "A-Down" },
};

static button_t analog_sources[] = {
	{ 0, ANALOG_AS_ANALOG,  "Analog Stick" },
	{ 1, C_STICK_AS_ANALOG, "C-Stick" },
};

static button_t menu_combos[] = {
	{ 0, PAD_BUTTON_X|PAD_BUTTON_Y, "X+Y" },
	{ 1, PAD_BUTTON_START|PAD_BUTTON_X, "Start+X" },
};

u32 gc_connected;

static unsigned int getButtons(int Control)
{
	unsigned int b = PAD_ButtonsHeld(Control);
	s8 stickX      = PAD_StickX(Control);
	s8 stickY      = PAD_StickY(Control);
	s8 substickX   = PAD_SubStickX(Control);
	s8 substickY   = PAD_SubStickY(Control);
	
	if(stickX    < -48) b |= ANALOG_L;
	if(stickX    >  48) b |= ANALOG_R;
	if(stickY    >  48) b |= ANALOG_U;
	if(stickY    < -48) b |= ANALOG_D;
	
	if(substickX < -48) b |= C_STICK_L;
	if(substickX >  48) b |= C_STICK_R;
	if(substickY >  48) b |= C_STICK_U;
	if(substickY < -48) b |= C_STICK_D;
	
	return b;
}

static int _GetKeys(int Control, BUTTONS * Keys, controller_config_t* config)
{
	if(padNeedScan){ gc_connected = PAD_ScanPads(); padNeedScan = 0; }
	BUTTONS* c = Keys;
	memset(c, 0, sizeof(BUTTONS));

	controller_GC.available[Control] = (gc_connected & (1<<Control)) ? 1 : 0;
	if (!controller_GC.available[Control]) return 0;

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

	if(config->analog->mask == ANALOG_AS_ANALOG){
		c->X_AXIS = PAD_StickX(Control);
		c->Y_AXIS = PAD_StickY(Control);
	} else if(config->analog->mask == C_STICK_AS_ANALOG){
		c->X_AXIS = PAD_SubStickX(Control);
		c->Y_AXIS = PAD_SubStickY(Control);
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
	    .Z         = &buttons[5],  // Z
	    .L         = &buttons[6],  // Left Trigger
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

	if(padNeedScan){ gc_connected = PAD_ScanPads(); padNeedScan = 0; }

	int i;
	for(i=0; i<4; ++i)
		controller_GC.available[i] = (gc_connected & (1<<i));
}
