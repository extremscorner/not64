/**
 * Wii64 - controller-Classic.c
 * Copyright (C) 2007, 2008, 2009, 2010 Mike Slegeir
 * Copyright (C) 2007, 2008, 2009, 2010 sepp256
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

enum { STICK_X, STICK_Y };
static float getStickValue(joystick_t* j, int axis, float maxAbsValue){
	float angle = M_PI * j->ang / 180.0f;
	float magnitude = j->mag * maxAbsValue;
	float value;
	if(axis == STICK_X)
		value = magnitude * sinf( angle );
	else
		value = magnitude * cosf( angle );
	return value;
}

enum {
	LSTICK_AS_ANALOG = 1, RSTICK_AS_ANALOG = 2, BUTTON_AS_ANALOG = 3,
};

enum {
	LSTICK_L = 0x01 << 24,
	LSTICK_R = 0x02 << 24,
	LSTICK_U = 0x04 << 24,
	LSTICK_D = 0x08 << 24,
	RSTICK_L = 0x10 << 24,
	RSTICK_R = 0x20 << 24,
	RSTICK_U = 0x40 << 24,
	RSTICK_D = 0x80 << 24,
};

#define NUM_CLASSIC_BUTTONS 24
static button_t buttons[] = {
	{  0, ~0,                         "None" },
	{  1, CLASSIC_CTRL_BUTTON_UP,     "D-Up" },
	{  2, CLASSIC_CTRL_BUTTON_LEFT,   "D-Left" },
	{  3, CLASSIC_CTRL_BUTTON_RIGHT,  "D-Right" },
	{  4, CLASSIC_CTRL_BUTTON_DOWN,   "D-Down" },
	{  5, CLASSIC_CTRL_BUTTON_FULL_L, "L" },
	{  6, CLASSIC_CTRL_BUTTON_FULL_R, "R" },
	{  7, CLASSIC_CTRL_BUTTON_ZL,     "ZL" },
	{  8, CLASSIC_CTRL_BUTTON_ZR,     "ZR" },
	{  9, CLASSIC_CTRL_BUTTON_A,      "A" },
	{ 10, CLASSIC_CTRL_BUTTON_B,      "B" },
	{ 11, CLASSIC_CTRL_BUTTON_X,      "X" },
	{ 12, CLASSIC_CTRL_BUTTON_Y,      "Y" },
	{ 13, CLASSIC_CTRL_BUTTON_PLUS,   "+" },
	{ 14, CLASSIC_CTRL_BUTTON_MINUS,  "-" },
	{ 15, CLASSIC_CTRL_BUTTON_HOME,   "Home" },
	{ 16, RSTICK_U,                   "RS-Up" },
	{ 17, RSTICK_L,                   "RS-Left" },
	{ 18, RSTICK_R,                   "RS-Right" },
	{ 19, RSTICK_D,                   "RS-Down" },
	{ 20, LSTICK_U,                   "LS-Up" },
	{ 21, LSTICK_L,                   "LS-Left" },
	{ 22, LSTICK_R,                   "LS-Right" },
	{ 23, LSTICK_D,                   "LS-Down" },
	{ 24, WIIU_PRO_CTRL_BUTTON_L3,    "L3" },
	{ 25, WIIU_PRO_CTRL_BUTTON_R3,    "R3" },
};

static button_t analog_sources[] = {
	{ 0, LSTICK_AS_ANALOG, "Left Stick" },
	{ 1, RSTICK_AS_ANALOG, "Right Stick" },
	{ 2, BUTTON_AS_ANALOG, "D-Pad" },
};

static button_t menu_combos[] = {
	{ 0, CLASSIC_CTRL_BUTTON_X|CLASSIC_CTRL_BUTTON_Y, "X+Y" },
	{ 1, CLASSIC_CTRL_BUTTON_PLUS|CLASSIC_CTRL_BUTTON_MINUS, "+&-" },
	{ 2, CLASSIC_CTRL_BUTTON_HOME, "Home" },
};

static int checkType(int Control, int type){
	int err;
	u32 expType;
	err = WPAD_Probe(Control, &expType);

	if(err != WPAD_ERR_NONE)
		return -1;

	switch(expType){
	case WPAD_EXP_NONE:
		controller_Wiimote.available[Control] = 1;
		break;
	case WPAD_EXP_NUNCHUK:
		controller_WiimoteNunchuk.available[Control] = 1;
		break;
	case WPAD_EXP_CLASSIC:
		controller_Classic.available[Control] = 1;
		break;
	case WPAD_EXP_WIIUPRO:
		controller_WiiUPro.available[Control] = 1;
		break;
	}

	return expType;
}

static unsigned int getButtonsCC(classic_ctrl_t* controller)
{
	unsigned int b = controller->btns;

	float stickX    = getStickValue(&controller->ljs, STICK_X, 1);
	float stickY    = getStickValue(&controller->ljs, STICK_Y, 1);
	float substickX = getStickValue(&controller->rjs, STICK_X, 1);
	float substickY = getStickValue(&controller->rjs, STICK_Y, 1);

	if(stickX    < -.5) b |= LSTICK_L;
	if(stickX    >  .5) b |= LSTICK_R;
	if(stickY    >  .5) b |= LSTICK_U;
	if(stickY    < -.5) b |= LSTICK_D;

	if(substickX < -.5) b |= RSTICK_L;
	if(substickX >  .5) b |= RSTICK_R;
	if(substickY >  .5) b |= RSTICK_U;
	if(substickY < -.5) b |= RSTICK_D;

	return b;
}

static int availableCC(int Control){
	if(checkType(Control, WPAD_EXP_CLASSIC) != WPAD_EXP_CLASSIC){
		controller_Classic.available[Control] = 0;
		return 0;
	} else {
		return 1;
	}
}

static int GetKeysCC(int Control, BUTTONS * Keys, controller_config_t* config)
{
	WPADData* wpad = WPAD_Data(Control);
	BUTTONS* c = Keys;
	memset(c, 0, sizeof(BUTTONS));

	// Only use a connected classic controller
	if(!availableCC(Control))
		return 0;

	WPAD_ReadPending(Control, NULL);

	unsigned int b = getButtonsCC(&wpad->exp.classic);
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

	if(config->analog->mask == LSTICK_AS_ANALOG){
		c->X_AXIS = getStickValue(&wpad->exp.classic.ljs, STICK_X, 80);
		c->Y_AXIS = getStickValue(&wpad->exp.classic.ljs, STICK_Y, 80);
	} else if(config->analog->mask == RSTICK_AS_ANALOG){
		c->X_AXIS = getStickValue(&wpad->exp.classic.rjs, STICK_X, 80);
		c->Y_AXIS = getStickValue(&wpad->exp.classic.rjs, STICK_Y, 80);
	} else if(config->analog->mask == BUTTON_AS_ANALOG){
		if(b & CLASSIC_CTRL_BUTTON_RIGHT)
			c->X_AXIS = +80;
		else if(b & CLASSIC_CTRL_BUTTON_LEFT)
			c->X_AXIS = -80;
		else
			c->X_AXIS = 0;

		if(b & CLASSIC_CTRL_BUTTON_UP)
			c->Y_AXIS = +80;
		else if(b & CLASSIC_CTRL_BUTTON_DOWN)
			c->Y_AXIS = -80;
		else
			c->Y_AXIS = 0;
	}
	if(config->invertedY) c->Y_AXIS = -c->Y_AXIS;

	// Return whether the exit button(s) are pressed
	return isHeld(config->exit);
}

static unsigned int getButtonsWUP(wiiu_pro_ctrl_t* controller)
{
	unsigned int b = controller->btns;

	float stickX    = getStickValue(&controller->ljs, STICK_X, 1);
	float stickY    = getStickValue(&controller->ljs, STICK_Y, 1);
	float substickX = getStickValue(&controller->rjs, STICK_X, 1);
	float substickY = getStickValue(&controller->rjs, STICK_Y, 1);

	if(stickX    < -.5) b |= LSTICK_L;
	if(stickX    >  .5) b |= LSTICK_R;
	if(stickY    >  .5) b |= LSTICK_U;
	if(stickY    < -.5) b |= LSTICK_D;

	if(substickX < -.5) b |= RSTICK_L;
	if(substickX >  .5) b |= RSTICK_R;
	if(substickY >  .5) b |= RSTICK_U;
	if(substickY < -.5) b |= RSTICK_D;

	return b;
}

static int availableWUP(int Control){
	if(checkType(Control, WPAD_EXP_WIIUPRO) != WPAD_EXP_WIIUPRO){
		controller_WiiUPro.available[Control] = 0;
		return 0;
	} else {
		return 1;
	}
}

static int GetKeysWUP(int Control, BUTTONS * Keys, controller_config_t* config)
{
	WPADData* wpad = WPAD_Data(Control);
	BUTTONS* c = Keys;
	memset(c, 0, sizeof(BUTTONS));

	// Only use a connected wiiu pro controller
	if(!availableWUP(Control))
		return 0;

	WPAD_ReadPending(Control, NULL);

	unsigned int b = getButtonsWUP(&wpad->exp.wup);
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

	if(config->analog->mask == LSTICK_AS_ANALOG){
		c->X_AXIS = getStickValue(&wpad->exp.wup.ljs, STICK_X, 80);
		c->Y_AXIS = getStickValue(&wpad->exp.wup.ljs, STICK_Y, 80);
	} else if(config->analog->mask == RSTICK_AS_ANALOG){
		c->X_AXIS = getStickValue(&wpad->exp.wup.rjs, STICK_X, 80);
		c->Y_AXIS = getStickValue(&wpad->exp.wup.rjs, STICK_Y, 80);
	} else if(config->analog->mask == BUTTON_AS_ANALOG){
		if(b & WIIU_PRO_CTRL_BUTTON_RIGHT)
			c->X_AXIS = +80;
		else if(b & WIIU_PRO_CTRL_BUTTON_LEFT)
			c->X_AXIS = -80;
		else
			c->X_AXIS = 0;

		if(b & WIIU_PRO_CTRL_BUTTON_UP)
			c->Y_AXIS = +80;
		else if(b & WIIU_PRO_CTRL_BUTTON_DOWN)
			c->Y_AXIS = -80;
		else
			c->Y_AXIS = 0;
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

static void configure(int Control, controller_config_t* config){
	// Don't know how this should be integrated
}

static void assign(int p, int v){
	// TODO: Light up the LEDs appropriately
}

static void refreshAvailableCC(void);
static void refreshAvailableWUP(void);

controller_t controller_Classic =
	{ 'C',
	  GetKeysCC,
	  configure,
	  assign,
	  pause,
	  resume,
	  rumble,
	  refreshAvailableCC,
	  {0, 0, 0, 0},
	  NUM_CLASSIC_BUTTONS,
	  buttons,
	  sizeof(analog_sources)/sizeof(analog_sources[0]),
	  analog_sources,
	  sizeof(menu_combos)/sizeof(menu_combos[0]),
	  menu_combos,
	  { .DU        = &buttons[1],  // D-Pad Up
	    .DL        = &buttons[2],  // D-Pad Left
	    .DR        = &buttons[3],  // D-Pad Right
	    .DD        = &buttons[4],  // D-Pad Down
	    .Z         = &buttons[5],  // Left Trigger
	    .L         = &buttons[8],  // Right Z
	    .R         = &buttons[6],  // Right Trigger
	    .A         = &buttons[9],  // A
	    .B         = &buttons[10], // B
	    .START     = &buttons[13], // +
	    .CU        = &buttons[16], // Right Stick Up
	    .CL        = &buttons[17], // Right Stick Left
	    .CR        = &buttons[18], // Right Stick Right
	    .CD        = &buttons[19], // Right Stick Down
	    .analog    = &analog_sources[0],
	    .exit      = &menu_combos[2],
	    .invertedY = 0,
	  }
	 };

controller_t controller_WiiUPro =
	{ 'P',
	  GetKeysWUP,
	  configure,
	  assign,
	  pause,
	  resume,
	  rumble,
	  refreshAvailableWUP,
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
	    .Z         = &buttons[7],  // Left Z
	    .L         = &buttons[6],  // Right Trigger
	    .R         = &buttons[8],  // Right Z
	    .A         = &buttons[9],  // A
	    .B         = &buttons[10], // B
	    .START     = &buttons[13], // +
	    .CU        = &buttons[16], // Right Stick Up
	    .CL        = &buttons[17], // Right Stick Left
	    .CR        = &buttons[18], // Right Stick Right
	    .CD        = &buttons[19], // Right Stick Down
	    .analog    = &analog_sources[0],
	    .exit      = &menu_combos[2],
	    .invertedY = 0,
	  }
	 };

static void refreshAvailableCC(void){
	int i;
	for(i=0; i<4; ++i){
		availableCC(i);
	}
}

static void refreshAvailableWUP(void){
	int i;
	for(i=0; i<4; ++i){
		availableWUP(i);
	}
}
