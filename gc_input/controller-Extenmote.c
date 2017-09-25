/**
 * Wii64 - controller-Extenmote.c
 * Copyright (C) 2007, 2008, 2009, 2010 Mike Slegeir
 * Copyright (C) 2007, 2008, 2009, 2010 sepp256
 * 
 * Extenmote input module
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
	STICK_AS_ANALOG = 1, SUBSTICK_AS_ANALOG = 2, CBUTTON_AS_ANALOG = 2, BUTTON_AS_ANALOG = 3, NO_ANALOG = 4,
};

enum {
	STICK_L    = 0x01 << 16,
	STICK_R    = 0x02 << 16,
	STICK_U    = 0x04 << 16,
	STICK_D    = 0x08 << 16,
	SUBSTICK_L = 0x10 << 16,
	SUBSTICK_R = 0x20 << 16,
	SUBSTICK_U = 0x40 << 16,
	SUBSTICK_D = 0x80 << 16,
	TRIGGER_L  = 0x01 << 24,
	TRIGGER_R  = 0x02 << 24,
};

static button_t buttons_nes[] = {
	{ 0, ~0,                          "None" },
	{ 1, EXTENMOTE_NES_BUTTON_UP,     "D-Up" },
	{ 2, EXTENMOTE_NES_BUTTON_LEFT,   "D-Left" },
	{ 3, EXTENMOTE_NES_BUTTON_RIGHT,  "D-Right" },
	{ 4, EXTENMOTE_NES_BUTTON_DOWN,   "D-Down" },
	{ 5, EXTENMOTE_NES_BUTTON_A,      "A" },
	{ 6, EXTENMOTE_NES_BUTTON_B,      "B" },
	{ 7, EXTENMOTE_NES_BUTTON_START,  "Start" },
	{ 8, EXTENMOTE_NES_BUTTON_SELECT, "Select" },
};

static button_t buttons_snes[] = {
	{  0, ~0,                           "None" },
	{  1, EXTENMOTE_SNES_BUTTON_UP,     "D-Up" },
	{  2, EXTENMOTE_SNES_BUTTON_LEFT,   "D-Left" },
	{  3, EXTENMOTE_SNES_BUTTON_RIGHT,  "D-Right" },
	{  4, EXTENMOTE_SNES_BUTTON_DOWN,   "D-Down" },
	{  5, EXTENMOTE_SNES_BUTTON_L,      "L" },
	{  6, EXTENMOTE_SNES_BUTTON_R,      "R" },
	{  7, EXTENMOTE_SNES_BUTTON_A,      "A" },
	{  8, EXTENMOTE_SNES_BUTTON_B,      "B" },
	{  9, EXTENMOTE_SNES_BUTTON_X,      "X" },
	{ 10, EXTENMOTE_SNES_BUTTON_Y,      "Y" },
	{ 11, EXTENMOTE_SNES_BUTTON_START,  "Start" },
	{ 12, EXTENMOTE_SNES_BUTTON_SELECT, "Select" },
};

static button_t buttons_n64[] = {
	{  0, ~0,                           "None" },
	{  1, EXTENMOTE_N64_BUTTON_UP,      "D-Up" },
	{  2, EXTENMOTE_N64_BUTTON_LEFT,    "D-Left" },
	{  3, EXTENMOTE_N64_BUTTON_RIGHT,   "D-Right" },
	{  4, EXTENMOTE_N64_BUTTON_DOWN,    "D-Down" },
	{  5, EXTENMOTE_N64_BUTTON_Z,       "Z" },
	{  6, EXTENMOTE_N64_BUTTON_L,       "L" },
	{  7, EXTENMOTE_N64_BUTTON_R,       "R" },
	{  8, EXTENMOTE_N64_BUTTON_A,       "A" },
	{  9, EXTENMOTE_N64_BUTTON_B,       "B" },
	{ 10, EXTENMOTE_N64_BUTTON_START,   "Start" },
	{ 11, EXTENMOTE_N64_BUTTON_C_UP,    "C-Up" },
	{ 12, EXTENMOTE_N64_BUTTON_C_LEFT,  "C-Left" },
	{ 13, EXTENMOTE_N64_BUTTON_C_RIGHT, "C-Right" },
	{ 14, EXTENMOTE_N64_BUTTON_C_DOWN,  "C-Down" },
	{ 15, STICK_U,                      "A-Up" },
	{ 16, STICK_L,                      "A-Left" },
	{ 17, STICK_R,                      "A-Right" },
	{ 18, STICK_D,                      "A-Down" },
};

static button_t buttons_gc[] = {
	{  0, ~0,                        "None" },
	{  1, EXTENMOTE_GC_BUTTON_UP,    "D-Up" },
	{  2, EXTENMOTE_GC_BUTTON_LEFT,  "D-Left" },
	{  3, EXTENMOTE_GC_BUTTON_RIGHT, "D-Right" },
	{  4, EXTENMOTE_GC_BUTTON_DOWN,  "D-Down" },
	{  5, EXTENMOTE_GC_BUTTON_Z,     "Z" },
	{  6, EXTENMOTE_GC_BUTTON_L,     "L" },
	{  7, EXTENMOTE_GC_BUTTON_R,     "R" },
	{  8, EXTENMOTE_GC_BUTTON_A,     "A" },
	{  9, EXTENMOTE_GC_BUTTON_B,     "B" },
	{ 10, EXTENMOTE_GC_BUTTON_X,     "X" },
	{ 11, EXTENMOTE_GC_BUTTON_Y,     "Y" },
	{ 12, EXTENMOTE_GC_BUTTON_START, "Start" },
	{ 13, SUBSTICK_U,                "C-Up" },
	{ 14, SUBSTICK_L,                "C-Left" },
	{ 15, SUBSTICK_R,                "C-Right" },
	{ 16, SUBSTICK_D,                "C-Down" },
	{ 17, STICK_U,                   "A-Up" },
	{ 18, STICK_L,                   "A-Left" },
	{ 19, STICK_R,                   "A-Right" },
	{ 20, STICK_D,                   "A-Down" },
	{ 21, TRIGGER_L,                 "L-Mid" },
	{ 22, TRIGGER_R,                 "R-Mid" },
};

static button_t analog_sources[] = {
	{ 0, BUTTON_AS_ANALOG, "D-Pad" },
	{ 1, NO_ANALOG,        "None" },
};

static button_t analog_sources_n64[] = {
	{ 0, STICK_AS_ANALOG,   "Analog Stick" },
	{ 1, CBUTTON_AS_ANALOG, "C-Pad" },
	{ 2, BUTTON_AS_ANALOG,  "D-Pad" },
};

static button_t analog_sources_gc[] = {
	{ 0, STICK_AS_ANALOG,    "Analog Stick" },
	{ 1, SUBSTICK_AS_ANALOG, "C-Stick" },
	{ 2, BUTTON_AS_ANALOG,   "D-Pad" },
};

#define NUM_NES_COMBOS 1
static button_t menu_combos[] = {
	{ 0, EXTENMOTE_NES_BUTTON_START|EXTENMOTE_NES_BUTTON_SELECT, "Start+Select" },
	{ 1, EXTENMOTE_SNES_BUTTON_L|EXTENMOTE_SNES_BUTTON_R,        "L+R" },
	{ 2, EXTENMOTE_SNES_BUTTON_A|EXTENMOTE_SNES_BUTTON_X,        "A+X" },
};

static button_t menu_combos_n64[] = {
	{ 0, EXTENMOTE_N64_BUTTON_START|EXTENMOTE_N64_BUTTON_Z,       "Start+Z" },
	{ 1, EXTENMOTE_N64_BUTTON_L|EXTENMOTE_N64_BUTTON_R,           "L+R" },
	{ 2, EXTENMOTE_N64_BUTTON_C_DOWN|EXTENMOTE_N64_BUTTON_C_LEFT, "C-DownLeft" },
};

static button_t menu_combos_gc[] = {
	{ 0, EXTENMOTE_GC_BUTTON_X|EXTENMOTE_GC_BUTTON_Y,     "X+Y" },
	{ 1, EXTENMOTE_GC_BUTTON_START|EXTENMOTE_GC_BUTTON_X, "Start+X" },
	{ 2, EXTENMOTE_GC_BUTTON_START|EXTENMOTE_GC_BUTTON_Y, "Start+Y" },
	{ 3, EXTENMOTE_GC_BUTTON_START|EXTENMOTE_GC_BUTTON_Z, "Start+Z" },
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
	case WPAD_EXP_NES:
		controller_ExtenmoteNES.available[Control] = 1;
		break;
	case WPAD_EXP_SNES:
		controller_ExtenmoteSNES.available[Control] = 1;
		break;
	case WPAD_EXP_N64:
		controller_ExtenmoteN64.available[Control] = 1;
		break;
	case WPAD_EXP_GC:
		controller_ExtenmoteGC.available[Control] = 1;
		break;
	}

	return expType;
}

static unsigned int getButtonsNES(extenmote_nes_t* controller){
	return controller->btns;
}

static int availableNES(int Control){
	if(checkType(Control, WPAD_EXP_NES) != WPAD_EXP_NES){
		controller_ExtenmoteNES.available[Control] = 0;
		return 0;
	} else {
		return 1;
	}
}

static int GetKeysNES(int Control, BUTTONS * Keys, controller_config_t* config)
{
	WPADData* wpad = WPAD_Data(Control);
	BUTTONS* c = Keys;
	memset(c, 0, sizeof(BUTTONS));

	// Only use a connected extenmote nes
	if(!availableNES(Control))
		return 0;

	WPAD_ReadPending(Control, NULL);

	unsigned int b = getButtonsNES(&wpad->exp.nes);
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

	if(config->analog->mask == BUTTON_AS_ANALOG){
		if(b & EXTENMOTE_NES_BUTTON_RIGHT)
			c->X_AXIS = +80;
		else if(b & EXTENMOTE_NES_BUTTON_LEFT)
			c->X_AXIS = -80;
		if(b & EXTENMOTE_NES_BUTTON_UP)
			c->Y_AXIS = +80;
		else if(b & EXTENMOTE_NES_BUTTON_DOWN)
			c->Y_AXIS = -80;
	}
	if(config->invertedY) c->Y_AXIS = -c->Y_AXIS;

	// Return whether the exit button(s) are pressed
	return isHeld(config->exit);
}

static unsigned int getButtonsSNES(extenmote_snes_t* controller){
	return controller->btns;
}

static int availableSNES(int Control){
	if(checkType(Control, WPAD_EXP_SNES) != WPAD_EXP_SNES){
		controller_ExtenmoteSNES.available[Control] = 0;
		return 0;
	} else {
		return 1;
	}
}

static int GetKeysSNES(int Control, BUTTONS * Keys, controller_config_t* config)
{
	WPADData* wpad = WPAD_Data(Control);
	BUTTONS* c = Keys;
	memset(c, 0, sizeof(BUTTONS));

	// Only use a connected extenmote snes
	if(!availableSNES(Control))
		return 0;

	WPAD_ReadPending(Control, NULL);

	unsigned int b = getButtonsSNES(&wpad->exp.snes);
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

	if(config->analog->mask == BUTTON_AS_ANALOG){
		if(b & EXTENMOTE_SNES_BUTTON_RIGHT)
			c->X_AXIS = +80;
		else if(b & EXTENMOTE_SNES_BUTTON_LEFT)
			c->X_AXIS = -80;
		if(b & EXTENMOTE_SNES_BUTTON_UP)
			c->Y_AXIS = +80;
		else if(b & EXTENMOTE_SNES_BUTTON_DOWN)
			c->Y_AXIS = -80;
	}
	if(config->invertedY) c->Y_AXIS = -c->Y_AXIS;

	// Return whether the exit button(s) are pressed
	return isHeld(config->exit);
}

static unsigned int getButtonsN64(extenmote_n64_t* controller){
	unsigned int b = controller->btns;

	float stickX    = getStickValue(&controller->js, STICK_X, 1);
	float stickY    = getStickValue(&controller->js, STICK_Y, 1);

	if(stickX    < -.5) b |= STICK_L;
	if(stickX    >  .5) b |= STICK_R;
	if(stickY    >  .5) b |= STICK_U;
	if(stickY    < -.5) b |= STICK_D;

	return b;
}

static int availableN64(int Control){
	if(checkType(Control, WPAD_EXP_N64) != WPAD_EXP_N64){
		controller_ExtenmoteN64.available[Control] = 0;
		return 0;
	} else {
		return 1;
	}
}

static int GetKeysN64(int Control, BUTTONS * Keys, controller_config_t* config)
{
	WPADData* wpad = WPAD_Data(Control);
	BUTTONS* c = Keys;
	memset(c, 0, sizeof(BUTTONS));

	// Only use a connected extenmote n64
	if(!availableN64(Control))
		return 0;

	WPAD_ReadPending(Control, NULL);

	unsigned int b = getButtonsN64(&wpad->exp.n64);
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
		c->X_AXIS = wpad->exp.n64.js.pos.x - wpad->exp.n64.js.center.x;
		c->Y_AXIS = wpad->exp.n64.js.pos.y - wpad->exp.n64.js.center.y;
	} else if(config->analog->mask == CBUTTON_AS_ANALOG){
		if(b & EXTENMOTE_N64_BUTTON_C_RIGHT)
			c->X_AXIS = +80;
		else if(b & EXTENMOTE_N64_BUTTON_C_LEFT)
			c->X_AXIS = -80;
		if(b & EXTENMOTE_N64_BUTTON_C_UP)
			c->Y_AXIS = +80;
		else if(b & EXTENMOTE_N64_BUTTON_C_DOWN)
			c->Y_AXIS = -80;
	} else if(config->analog->mask == BUTTON_AS_ANALOG){
		if(b & EXTENMOTE_N64_BUTTON_RIGHT)
			c->X_AXIS = +80;
		else if(b & EXTENMOTE_N64_BUTTON_LEFT)
			c->X_AXIS = -80;
		if(b & EXTENMOTE_N64_BUTTON_UP)
			c->Y_AXIS = +80;
		else if(b & EXTENMOTE_N64_BUTTON_DOWN)
			c->Y_AXIS = -80;
	}
	if(config->invertedY) c->Y_AXIS = -c->Y_AXIS;

	// Return whether the exit button(s) are pressed
	return isHeld(config->exit);
}

static unsigned int getButtonsGC(extenmote_gc_t* controller){
	unsigned int b = controller->btns;

	float stickX    = getStickValue(&controller->ljs, STICK_X, 1);
	float stickY    = getStickValue(&controller->ljs, STICK_Y, 1);
	float substickX = getStickValue(&controller->rjs, STICK_X, 1);
	float substickY = getStickValue(&controller->rjs, STICK_Y, 1);

	float triggerL = controller->l_shoulder;
	float triggerR = controller->r_shoulder;

	if(stickX    < -.5) b |= STICK_L;
	if(stickX    >  .5) b |= STICK_R;
	if(stickY    >  .5) b |= STICK_U;
	if(stickY    < -.5) b |= STICK_D;

	if(substickX < -.5) b |= SUBSTICK_L;
	if(substickX >  .5) b |= SUBSTICK_R;
	if(substickY >  .5) b |= SUBSTICK_U;
	if(substickY < -.5) b |= SUBSTICK_D;

	if(triggerL  >  .5) b |= TRIGGER_L;
	if(triggerR  >  .5) b |= TRIGGER_R;

	return b;
}

static int availableGC(int Control){
	if(checkType(Control, WPAD_EXP_GC) != WPAD_EXP_GC){
		controller_ExtenmoteGC.available[Control] = 0;
		return 0;
	} else {
		return 1;
	}
}

static int GetKeysGC(int Control, BUTTONS * Keys, controller_config_t* config)
{
	WPADData* wpad = WPAD_Data(Control);
	BUTTONS* c = Keys;
	memset(c, 0, sizeof(BUTTONS));

	// Only use a connected extenmote gc
	if(!availableGC(Control))
		return 0;

	WPAD_ReadPending(Control, NULL);

	unsigned int b = getButtonsGC(&wpad->exp.gc);
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
		c->X_AXIS = getStickValue(&wpad->exp.gc.ljs, STICK_X, 80);
		c->Y_AXIS = getStickValue(&wpad->exp.gc.ljs, STICK_Y, 80);
	} else if(config->analog->mask == SUBSTICK_AS_ANALOG){
		c->X_AXIS = getStickValue(&wpad->exp.gc.rjs, STICK_X, 80);
		c->Y_AXIS = getStickValue(&wpad->exp.gc.rjs, STICK_Y, 80);
	} else if(config->analog->mask == BUTTON_AS_ANALOG){
		if(b & EXTENMOTE_GC_BUTTON_RIGHT)
			c->X_AXIS = +80;
		else if(b & EXTENMOTE_GC_BUTTON_LEFT)
			c->X_AXIS = -80;
		if(b & EXTENMOTE_GC_BUTTON_UP)
			c->Y_AXIS = +80;
		else if(b & EXTENMOTE_GC_BUTTON_DOWN)
			c->Y_AXIS = -80;
	}
	if(config->invertedY) c->Y_AXIS = -c->Y_AXIS;

	// Return whether the exit button(s) are pressed
	return isHeld(config->exit);
}

static void pause(int Control){
	WPAD_Rumble(Control, 0);
}

static void resume(int Control){
	WPAD_SetDataFormat(Control, WPAD_FMT_BTNS_IR);
}

static void rumble(int Control, int rumble){
	WPAD_Rumble(Control, rumble ? 1 : 0);
}

static void configure(int Control, controller_config_t* config){
	// Don't know how this should be integrated
}

static void assign(int p, int v){
	WPAD_ControlLed(p, WPAD_LED_1 << v);
}

static void refreshAvailableNES(void);
static void refreshAvailableSNES(void);
static void refreshAvailableN64(void);
static void refreshAvailableGC(void);

controller_t controller_ExtenmoteNES =
	{ 'F',
	  GetKeysNES,
	  configure,
	  assign,
	  pause,
	  resume,
	  rumble,
	  refreshAvailableNES,
	  {0, 0, 0, 0},
	  sizeof(buttons_nes)/sizeof(buttons_nes[0]),
	  buttons_nes,
	  sizeof(analog_sources)/sizeof(analog_sources[0]),
	  analog_sources,
	  NUM_NES_COMBOS,
	  menu_combos,
	  { .DU        = &buttons_nes[1], // D-Pad Up
	    .DL        = &buttons_nes[2], // D-Pad Left
	    .DR        = &buttons_nes[3], // D-Pad Right
	    .DD        = &buttons_nes[4], // D-Pad Down
	    .Z         = &buttons_nes[8], // Select
	    .L         = &buttons_nes[0], // None
	    .R         = &buttons_nes[0], // None
	    .A         = &buttons_nes[5], // A
	    .B         = &buttons_nes[6], // B
	    .START     = &buttons_nes[7], // Start
	    .CU        = &buttons_nes[0], // None
	    .CL        = &buttons_nes[0], // None
	    .CR        = &buttons_nes[0], // None
	    .CD        = &buttons_nes[0], // None
	    .analog    = &analog_sources[1],
	    .exit      = &menu_combos[0],
	    .invertedY = 0,
	  }
	 };

controller_t controller_ExtenmoteSNES =
	{ 'S',
	  GetKeysSNES,
	  configure,
	  assign,
	  pause,
	  resume,
	  rumble,
	  refreshAvailableSNES,
	  {0, 0, 0, 0},
	  sizeof(buttons_snes)/sizeof(buttons_snes[0]),
	  buttons_snes,
	  sizeof(analog_sources)/sizeof(analog_sources[0]),
	  analog_sources,
	  sizeof(menu_combos)/sizeof(menu_combos[0]),
	  menu_combos,
	  { .DU        = &buttons_snes[1],  // D-Pad Up
	    .DL        = &buttons_snes[2],  // D-Pad Left
	    .DR        = &buttons_snes[3],  // D-Pad Right
	    .DD        = &buttons_snes[4],  // D-Pad Down
	    .Z         = &buttons_snes[12], // Select
	    .L         = &buttons_snes[0],  // None
	    .R         = &buttons_snes[0],  // None
	    .A         = &buttons_snes[8],  // B
	    .B         = &buttons_snes[10], // Y
	    .START     = &buttons_snes[11], // Start
	    .CU        = &buttons_snes[9],  // X
	    .CL        = &buttons_snes[5],  // Left Trigger
	    .CR        = &buttons_snes[6],  // Right Trigger
	    .CD        = &buttons_snes[7],  // A
	    .analog    = &analog_sources[1],
	    .exit      = &menu_combos[0],
	    .invertedY = 0,
	  }
	 };

controller_t controller_ExtenmoteN64 =
	{ 'U',
	  GetKeysN64,
	  configure,
	  assign,
	  pause,
	  resume,
	  rumble,
	  refreshAvailableN64,
	  {0, 0, 0, 0},
	  sizeof(buttons_n64)/sizeof(buttons_n64[0]),
	  buttons_n64,
	  sizeof(analog_sources_n64)/sizeof(analog_sources_n64[0]),
	  analog_sources_n64,
	  sizeof(menu_combos_n64)/sizeof(menu_combos_n64[0]),
	  menu_combos_n64,
	  { .DU        = &buttons_n64[1],  // D-Pad Up
	    .DL        = &buttons_n64[2],  // D-Pad Left
	    .DR        = &buttons_n64[3],  // D-Pad Right
	    .DD        = &buttons_n64[4],  // D-Pad Down
	    .Z         = &buttons_n64[5],  // Z
	    .L         = &buttons_n64[6],  // Left Trigger
	    .R         = &buttons_n64[7],  // Right Trigger
	    .A         = &buttons_n64[8],  // A
	    .B         = &buttons_n64[9],  // B
	    .START     = &buttons_n64[10], // Start
	    .CU        = &buttons_n64[11], // C-Up
	    .CL        = &buttons_n64[12], // C-Left
	    .CR        = &buttons_n64[13], // C-Right
	    .CD        = &buttons_n64[14], // C-Down
	    .analog    = &analog_sources_n64[0],
	    .exit      = &menu_combos_n64[0],
	    .invertedY = 0,
	  }
	 };

controller_t controller_ExtenmoteGC =
	{ 'D',
	  GetKeysGC,
	  configure,
	  assign,
	  pause,
	  resume,
	  rumble,
	  refreshAvailableGC,
	  {0, 0, 0, 0},
	  sizeof(buttons_gc)/sizeof(buttons_gc[0]),
	  buttons_gc,
	  sizeof(analog_sources_gc)/sizeof(analog_sources_gc[0]),
	  analog_sources_gc,
	  sizeof(menu_combos_gc)/sizeof(menu_combos_gc[0]),
	  menu_combos_gc,
	  { .DU        = &buttons_gc[1],  // D-Pad Up
	    .DL        = &buttons_gc[2],  // D-Pad Left
	    .DR        = &buttons_gc[3],  // D-Pad Right
	    .DD        = &buttons_gc[4],  // D-Pad Down
	    .Z         = &buttons_gc[21], // Left Trigger
	    .L         = &buttons_gc[5],  // Z
	    .R         = &buttons_gc[22], // Right Trigger
	    .A         = &buttons_gc[8],  // A
	    .B         = &buttons_gc[9],  // B
	    .START     = &buttons_gc[12], // Start
	    .CU        = &buttons_gc[13], // C-Stick Up
	    .CL        = &buttons_gc[14], // C-Stick Left
	    .CR        = &buttons_gc[15], // C-Stick Right
	    .CD        = &buttons_gc[16], // C-Stick Down
	    .analog    = &analog_sources_gc[0],
	    .exit      = &menu_combos_gc[0],
	    .invertedY = 0,
	  }
	 };

static void refreshAvailableNES(void){
	int i;
	for(i=0; i<4; ++i){
		availableNES(i);
	}
}

static void refreshAvailableSNES(void){
	int i;
	for(i=0; i<4; ++i){
		availableSNES(i);
	}
}

static void refreshAvailableN64(void){
	int i;
	for(i=0; i<4; ++i){
		availableN64(i);
	}
}

static void refreshAvailableGC(void){
	int i;
	for(i=0; i<4; ++i){
		availableGC(i);
	}
}
