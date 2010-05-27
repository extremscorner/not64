/**
 * Wii64 - controller-WiimoteNunchuk.c
 * Copyright (C) 2007, 2008, 2009, 2010 Mike Slegeir
 * Copyright (C) 2007, 2008, 2009, 2010 sepp256
 * 
 * Wiimote + Nunchuk input module
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
	NUNCHUK_AS_ANALOG, IR_AS_ANALOG,
	TILT_AS_ANALOG, WHEEL_AS_ANALOG,
	NO_ANALOG,
};

enum {
	NUNCHUK_L = 0x10 << 16,
	NUNCHUK_R = 0x20 << 16,
	NUNCHUK_U = 0x40 << 16,
	NUNCHUK_D = 0x80 << 16,
};

#define NUM_WIIMOTE_BUTTONS 12
static button_t buttons[] = {
	{  0, ~0,                    "None" },
	{  1, WPAD_BUTTON_UP,        "D-Up" },
	{  2, WPAD_BUTTON_LEFT,      "D-Left" },
	{  3, WPAD_BUTTON_RIGHT,     "D-Right" },
	{  4, WPAD_BUTTON_DOWN,      "D-Down" },
	{  5, WPAD_BUTTON_A,         "A" },
	{  6, WPAD_BUTTON_B,         "B" },
	{  7, WPAD_BUTTON_PLUS,      "+" },
	{  8, WPAD_BUTTON_MINUS,     "-" },
	{  9, WPAD_BUTTON_HOME,      "Home" },
	{ 10, WPAD_BUTTON_1,         "1" },
	{ 11, WPAD_BUTTON_2,         "2" },
	{ 12, WPAD_NUNCHUK_BUTTON_C, "C" },
	{ 13, WPAD_NUNCHUK_BUTTON_Z, "Z" },
	{ 14, NUNCHUK_U,             "NC-Up" },
	{ 15, NUNCHUK_L,             "NC-Left" },
	{ 16, NUNCHUK_R,             "NC-Right" },
	{ 17, NUNCHUK_D,             "NC-Down" },
};

static button_t analog_sources_wmn[] = {
	{ 0, NUNCHUK_AS_ANALOG,  "Nunchuk" },
	{ 1, IR_AS_ANALOG,       "IR" },
};

static button_t analog_sources_wm[] = {
	{ 0, TILT_AS_ANALOG,     "Tilt" },
	{ 1, WHEEL_AS_ANALOG,    "Wheel" },
	{ 2, IR_AS_ANALOG,       "IR" },
	{ 3, NO_ANALOG,          "None" },
};

static button_t menu_combos[] = {
	{ 0, WPAD_BUTTON_1|WPAD_BUTTON_2, "1+2" },
	{ 1, WPAD_BUTTON_PLUS|WPAD_BUTTON_MINUS, "+&-" },
};

static int _GetKeys(int Control, BUTTONS * Keys, controller_config_t* config,
                    int (*available)(int),
                    unsigned int (*getButtons)(WPADData*))
{
	if(wpadNeedScan){ WPAD_ScanPads(); wpadNeedScan = 0; }
	WPADData* wpad = WPAD_Data(Control);
	BUTTONS* c = Keys;
	memset(c, 0, sizeof(BUTTONS));

	// Only use a connected nunchuck controller
	if(!available(Control))
		return 0;

	unsigned int b = getButtons(wpad);
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

	if(config->analog->mask == NUNCHUK_AS_ANALOG){
		c->X_AXIS = getStickValue(&wpad->exp.nunchuk.js, STICK_X, 127);
		c->Y_AXIS = getStickValue(&wpad->exp.nunchuk.js, STICK_Y, 127);
	} else if(config->analog->mask == IR_AS_ANALOG){
		if(wpad->ir.smooth_valid){
			c->X_AXIS = ((short)(wpad->ir.sx - 512)) >> 2;
			c->Y_AXIS = -(signed char)((wpad->ir.sy - 384) / 3);
		} else {
			c->X_AXIS = 0;
			c->Y_AXIS = 0;
		}
	} else if(config->analog->mask == TILT_AS_ANALOG){
		c->X_AXIS = -wpad->orient.pitch;
		c->Y_AXIS = wpad->orient.roll;
	} else if(config->analog->mask == WHEEL_AS_ANALOG){
		c->X_AXIS = 512 - wpad->accel.y;
		c->Y_AXIS = wpad->accel.z - 512;
	}
	if(config->invertedY) c->Y_AXIS = -c->Y_AXIS;

	// Return whether the exit button(s) are pressed
	return isHeld(config->exit);
}

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
	}
	
	return expType;
}

static int availableWM(int Control){
	if(checkType(Control, WPAD_EXP_NONE) != WPAD_EXP_NONE){
		controller_Wiimote.available[Control] = 0;
		return 0;
	} else {
		return 1;
	}
}

static int availableWMN(int Control){
	if(checkType(Control, WPAD_EXP_NUNCHUK) != WPAD_EXP_NUNCHUK){
		controller_WiimoteNunchuk.available[Control] = 0;
		return 0;
	} else {
		return 1;
	}
}

static unsigned int getButtonsWM(WPADData* controller){
	return controller->btns_h;
}

static unsigned int getButtonsWMN(WPADData* controller){
	unsigned int b = controller->btns_h;
	s8 stickX      = getStickValue(&controller->exp.nunchuk.js, STICK_X, 7);
	s8 stickY      = getStickValue(&controller->exp.nunchuk.js, STICK_Y, 7);
	
	if(stickX    < -3) b |= NUNCHUK_L;
	if(stickX    >  3) b |= NUNCHUK_R;
	if(stickY    >  3) b |= NUNCHUK_U;
	if(stickY    < -3) b |= NUNCHUK_D;
	
	return b;
}

static int GetKeysWM(int con, BUTTONS * keys, controller_config_t* cfg){
	return _GetKeys(con, keys, cfg, availableWM, getButtonsWM);
}

static int GetKeysWMN(int con, BUTTONS * keys, controller_config_t* cfg){
	return _GetKeys(con, keys, cfg, availableWMN, getButtonsWMN);
}

static void pause(int Control){
	WPAD_Rumble(Control, 0);
}

static void resume(int Control){ }

static void rumble(int Control, int rumble){
	WPAD_Rumble(Control, rumble ? 1 : 0);
}

static void configure(int Control, controller_config_t* config){
	static s32 analog_fmts[] = {
		WPAD_DATA_EXPANSION, // Nunchuk
		WPAD_DATA_IR,        // IR
		WPAD_DATA_ACCEL,     // Tilt
		WPAD_DATA_ACCEL,     // Wheel
		WPAD_DATA_BUTTONS,   // None
	};
	WPAD_SetDataFormat(Control, analog_fmts[config->analog->mask]);
}

static void assign(int p, int v){
	// TODO: Light up the LEDs appropriately
}

static void refreshAvailableWM(void);
static void refreshAvailableWMN(void);

controller_t controller_Wiimote =
	{ 'W',
	  GetKeysWM,
	  configure,
	  assign,
	  pause,
	  resume,
	  rumble,
	  refreshAvailableWM,
	  {0, 0, 0, 0},
	  NUM_WIIMOTE_BUTTONS,
	  buttons,
	  sizeof(analog_sources_wm)/sizeof(analog_sources_wm[0]),
	  analog_sources_wm,
	  sizeof(menu_combos)/sizeof(menu_combos[0]),
	  menu_combos,
	  { .DU    = &buttons[0],  // None
		.DL    = &buttons[0],  // None
		.DR    = &buttons[0],  // None
		.DD    = &buttons[0],  // None
		.Z     = &buttons[6],  // B
		.L     = &buttons[8],  // -
		.R     = &buttons[7],  // +
		.A     = &buttons[11], // 2
		.B     = &buttons[10], // 1
		.START = &buttons[9],  // Home
		.CU    = &buttons[3],  // D Right
		.CL    = &buttons[1],  // D Up
		.CR    = &buttons[4],  // D Down
		.CD    = &buttons[2],  // D Left
		.analog    = &analog_sources_wm[0],
		.exit      = &menu_combos[0],
		.invertedY = 0
	  }
	};

controller_t controller_WiimoteNunchuk =
	{ 'N',
	  GetKeysWMN,
	  configure,
	  assign,
	  pause,
	  resume,
	  rumble,
	  refreshAvailableWMN,
	  {0, 0, 0, 0},
	  sizeof(buttons)/sizeof(buttons[0]),
	  buttons,
	  sizeof(analog_sources_wmn)/sizeof(analog_sources_wmn[0]),
	  analog_sources_wmn,
	  sizeof(menu_combos)/sizeof(menu_combos[0]),
	  menu_combos,
	  { .DU    = &buttons[0],  // None
	    .DL    = &buttons[0],  // None
	    .DR    = &buttons[0],  // None
	    .DD    = &buttons[0],  // None
	    .Z     = &buttons[13], // Z
	    .L     = &buttons[12], // C
	    .R     = &buttons[6],  // B
	    .A     = &buttons[5],  // A
	    .B     = &buttons[7],  // +
	    .START = &buttons[9],  // Home
	    .CU    = &buttons[1], // D Up
	    .CL    = &buttons[2], // D Left
	    .CR    = &buttons[3], // D Right
	    .CD    = &buttons[4], // D Down
	    .analog    = &analog_sources_wmn[0],
	    .exit      = &menu_combos[0],
	    .invertedY = 0
	  }
	 };

static void refreshAvailableWM(void){
	int i;
	WPAD_ScanPads();
	for(i=0; i<4; ++i){
		availableWM(i);
	}
}

static void refreshAvailableWMN(void){
	int i;
	WPAD_ScanPads();
	for(i=0; i<4; ++i){
		availableWMN(i);
	}
}
