/**
 * Wii64 - controller-Classic.c
 * Copyright (C) 2007, 2008, 2009 Mike Slegeir
 * Copyright (C) 2007, 2008, 2009 sepp256
 * 
 * Classic controller input module
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
#include <math.h>
#include <wiiuse/wpad.h>
#include "controller.h"

#ifndef PI
#define PI 3.14159f
#endif

enum { STICK_X, STICK_Y };
static int getStickValue(joystick_t* j, int axis, int maxAbsValue){
	double angle = PI * j->ang/180.0f;
	double magnitude = (j->mag > 1.0f) ? 1.0f :
	                    (j->mag < -1.0f) ? -1.0f : j->mag;
	double value;
	if(axis == STICK_X)
		value = magnitude * sin( angle );
	else
		value = magnitude * cos( angle );
	return (int)(value * maxAbsValue);
}

enum {
	L_STICK_AS_ANALOG = 1, R_STICK_AS_ANALOG = 2,
};

enum {
	L_STICK_L  = 0x01 << 16,
	L_STICK_R  = 0x02 << 16,
	L_STICK_U  = 0x04 << 16,
	L_STICK_D  = 0x08 << 16,
	R_STICK_L = 0x10 << 16,
	R_STICK_R = 0x20 << 16,
	R_STICK_U = 0x40 << 16,
	R_STICK_D = 0x80 << 16,
};

static button_t buttons[] = {
	{  0, ~0,                         "None" },
	{  1, CLASSIC_CTRL_BUTTON_UP,     "D-Pad Up" },
	{  2, CLASSIC_CTRL_BUTTON_LEFT,   "D-Pad Left" },
	{  3, CLASSIC_CTRL_BUTTON_RIGHT,  "D-Pad Right" },
	{  4, CLASSIC_CTRL_BUTTON_DOWN,   "D-Pad Down" },
	{  5, CLASSIC_CTRL_BUTTON_FULL_L, "L" },
	{  6, CLASSIC_CTRL_BUTTON_FULL_R, "R" },
	{  7, CLASSIC_CTRL_BUTTON_ZL,     "Left Z" },
	{  8, CLASSIC_CTRL_BUTTON_ZR,     "Right Z" },
	{  9, CLASSIC_CTRL_BUTTON_A,      "A" },
	{ 10, CLASSIC_CTRL_BUTTON_B,      "B" },
	{ 11, CLASSIC_CTRL_BUTTON_X,      "X" },
	{ 12, CLASSIC_CTRL_BUTTON_Y,      "Y" },
	{ 13, CLASSIC_CTRL_BUTTON_PLUS,   "+" },
	{ 14, CLASSIC_CTRL_BUTTON_MINUS,  "-" },
	{ 15, CLASSIC_CTRL_BUTTON_HOME,   "Home" },
	{ 16, R_STICK_U,                  "Right Stick Up" },
	{ 17, R_STICK_L,                  "Right Stick Left" },
	{ 18, R_STICK_R,                  "Right Stick Right" },
	{ 19, R_STICK_D,                  "Right Stick Down" },
	{ 20, L_STICK_U,                  "Left Stick Up" },
	{ 21, L_STICK_L,                  "Left Stick Left" },
	{ 22, L_STICK_R,                  "Left Stick Right" },
	{ 23, L_STICK_D,                  "Left Stick Down" },
};

static button_t analog_sources[] = {
	{ 0, L_STICK_AS_ANALOG,  "Left Stick" },
	{ 1, R_STICK_AS_ANALOG,  "Right Stick" },
};

static button_t menu_combos[] = {
	{ 0, CLASSIC_CTRL_BUTTON_X|CLASSIC_CTRL_BUTTON_Y, "X+Y" },
};

static unsigned int getButtons(classic_ctrl_t* controller)
{
	unsigned int b = (unsigned)controller->btns;
	s8 stickX      = getStickValue(&controller->ljs, STICK_X, 7);
	s8 stickY      = getStickValue(&controller->ljs, STICK_Y, 7);
	s8 substickX   = getStickValue(&controller->rjs, STICK_X, 7);
	s8 substickY   = getStickValue(&controller->rjs, STICK_Y, 7);
	
	if(stickX    < -3) b |= L_STICK_L;
	if(stickX    >  3) b |= L_STICK_R;
	if(stickY    >  3) b |= L_STICK_U;
	if(stickY    < -3) b |= L_STICK_D;
	
	if(substickX < -3) b |= R_STICK_L;
	if(substickY >  3) b |= R_STICK_R;
	if(substickY >  3) b |= R_STICK_U;
	if(substickY < -3) b |= R_STICK_D;
	
	return b;
}

static int available(int Control, WPADData* wpad) {
	if(wpad->err == WPAD_ERR_NONE &&
	   wpad->exp.type == WPAD_EXP_CLASSIC){
		controller_Classic.available[Control] = 1;
		return 1;
	} else {
		controller_Classic.available[Control] = 0;
		if(wpad->err == WPAD_ERR_NONE &&
		   wpad->exp.type == WPAD_EXP_NUNCHUK){
			controller_WiimoteNunchuk.available[Control] = 1;
		}
		return 0;
	}
}

static int _GetKeys(int Control, BUTTONS * Keys, controller_config_t* config)
{
	if(wpadNeedScan){ WPAD_ScanPads(); wpadNeedScan = 0; }
	WPADData* wpad = WPAD_Data(Control);
	BUTTONS* c = Keys;
	memset(c, 0, sizeof(BUTTONS));

	// Only use a connected classic controller
	if(!available(Control, wpad))
		return 0;

	unsigned int b = getButtons(&wpad->exp.classic);
	int isHeld(button_tp button){ return (b & button->mask) == button->mask; }
	
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

	if(config->analog->mask & L_STICK_AS_ANALOG){
		c->X_AXIS = getStickValue(&wpad->exp.classic.ljs, STICK_X, 127);
		c->Y_AXIS = getStickValue(&wpad->exp.classic.ljs, STICK_Y, 127);
	} else if(config->analog->mask & R_STICK_AS_ANALOG){
		c->X_AXIS = getStickValue(&wpad->exp.classic.rjs, STICK_X, 127);
		c->Y_AXIS = getStickValue(&wpad->exp.classic.rjs, STICK_Y, 127);
	}
	if(config->invertedY) c->Y_AXIS = -c->Y_AXIS;

	// Return whether the exit button(s) are pressed
	return isHeld(config->exit);
}

static void pause(int Control){
	WPAD_Rumble(Control, 0);
}

static void resume(int Control){ }

static void rumble(int Control, int rumble){
	WPAD_Rumble(Control, rumble ? 1 : 0);
}

static void configure(int Control){
	// Don't know how this should be integrated
}

static void assign(int p, int v){
	// TODO: Light up the LEDs appropriately
}

static void init(void);

controller_t controller_Classic =
	{ _GetKeys,
	  configure,
	  init,
	  assign,
	  pause,
	  resume,
	  rumble,
	  {0, 0, 0, 0},
	  sizeof(buttons)/sizeof(buttons[0]),
	  buttons,
	  sizeof(analog_sources)/sizeof(analog_sources[0]),
	  analog_sources,
	  sizeof(menu_combos)/sizeof(menu_combos[0]),
	  menu_combos
	 };

static void init(void){
	int i;
	WPAD_ScanPads();
	for(i=0; i<4; ++i){
		WPADData* wpad = WPAD_Data(i);
		// Only use a connected classic controller
		if(wpad->err == WPAD_ERR_NONE &&
		   wpad->exp.type == WPAD_EXP_CLASSIC){
			controller_Classic.available[i] = 1;
			WPAD_SetDataFormat(i, WPAD_DATA_EXPANSION);
		} else
			controller_Classic.available[i] = 0;
	}
	
	controller_Classic.config_default.DU        = &buttons[1];  // D-Pad Up
	controller_Classic.config_default.DL        = &buttons[2];  // D-Pad Left
	controller_Classic.config_default.DR        = &buttons[3];  // D-Pad Right
	controller_Classic.config_default.DD        = &buttons[4];  // D-Pad Down
	controller_Classic.config_default.Z         = &buttons[7];  // Left Z
	controller_Classic.config_default.L         = &buttons[5];  // Left Trigger
	controller_Classic.config_default.R         = &buttons[6];  // Right Trigger
	controller_Classic.config_default.A         = &buttons[9];  // A
	controller_Classic.config_default.B         = &buttons[10]; // B
	controller_Classic.config_default.START     = &buttons[13]; // +
	controller_Classic.config_default.CU        = &buttons[16]; // Right Stick Up
	controller_Classic.config_default.CL        = &buttons[17]; // Right Stick Left
	controller_Classic.config_default.CR        = &buttons[18]; // Right Stick Right
	controller_Classic.config_default.CD        = &buttons[19]; // Right Stick Down
	controller_Classic.config_default.analog    = &analog_sources[0];
	controller_Classic.config_default.exit      = &menu_combos[0];
	controller_Classic.config_default.invertedY = 0;

	for(i=0; i<4; ++i)
	{
		memcpy(&controller_Classic.config[i], &controller_Classic.config_default, sizeof(controller_config_t));
		memcpy(&controller_Classic.config_slot[i], &controller_Classic.config_default, sizeof(controller_config_t));
	}
}
