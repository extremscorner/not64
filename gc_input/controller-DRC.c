/*
 * Not64 - controller-DRC.c (based in controller-Classic.c)
 * Original from emu_kidid (imported by saulfabreg)
 * Copyright (c) 2013 Extrems <metaradil@gmail.com>
 *
 * This file is part of Not64.
 *
 * Not64 is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Not64 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with Not64; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <string.h>
#include <math.h>
#include "controller.h"
#include <wiidrc/wiidrc.h>

#define DRC_DEADZONE 6
#define _DRC_BUILD_TMPSTICK(inval) \
	tmp_stick16 = (inval*1.08f); \
	if(tmp_stick16 > DRC_DEADZONE) tmp_stick16 = (tmp_stick16-DRC_DEADZONE)*1.08f; \
	else if(tmp_stick16 < -DRC_DEADZONE) tmp_stick16 = (tmp_stick16+DRC_DEADZONE)*1.08f; \
	else tmp_stick16 = 0; \
	if(tmp_stick16 > 80) tmp_stick8 = 80; \
	else if(tmp_stick16 < -80) tmp_stick8 = -80; \
	else tmp_stick8 = (s8)tmp_stick16;

static int getDRCValue(int in)
{
	//do scale, deadzone and clamp
	s8 tmp_stick8; s16 tmp_stick16;
	_DRC_BUILD_TMPSTICK(in);
	return tmp_stick8;
}

enum {
	L_STICK_AS_ANALOG = 1, R_STICK_AS_ANALOG = 2,
};

enum {
	L_STICK_L = 0x01 << 16,
	L_STICK_R = 0x02 << 16,
	L_STICK_U = 0x04 << 16,
	L_STICK_D = 0x08 << 16,
	R_STICK_L = 0x10 << 16,
	R_STICK_R = 0x20 << 16,
	R_STICK_U = 0x40 << 16,
	R_STICK_D = 0x80 << 16,
};

static button_t buttons[] = {
	{  0, ~0,                         "None" },
	{  1, WIIDRC_BUTTON_UP,     "D-Up" },
	{  2, WIIDRC_BUTTON_LEFT,   "D-Left" },
	{  3, WIIDRC_BUTTON_RIGHT,  "D-Right" },
	{  4, WIIDRC_BUTTON_DOWN,   "D-Down" },
	{  5, WIIDRC_BUTTON_L, "L" },
	{  6, WIIDRC_BUTTON_R, "R" },
	{  7, WIIDRC_BUTTON_ZL,     "Left Z" },
	{  8, WIIDRC_BUTTON_ZR,     "Right Z" },
	{  9, WIIDRC_BUTTON_A,      "A" },
	{ 10, WIIDRC_BUTTON_B,      "B" },
	{ 11, WIIDRC_BUTTON_X,      "X" },
	{ 12, WIIDRC_BUTTON_Y,      "Y" },
	{ 13, WIIDRC_BUTTON_PLUS,   "+" },
	{ 14, WIIDRC_BUTTON_MINUS,  "-" },
	{ 15, WIIDRC_BUTTON_HOME,   "Home" },
	{ 16, R_STICK_U,                  "RS-Up" },
	{ 17, R_STICK_L,                  "RS-Left" },
	{ 18, R_STICK_R,                  "RS-Right" },
	{ 19, R_STICK_D,                  "RS-Down" },
	{ 20, L_STICK_U,                  "LS-Up" },
	{ 21, L_STICK_L,                  "LS-Left" },
	{ 22, L_STICK_R,                  "LS-Right" },
	{ 23, L_STICK_D,                  "LS-Down" },
};

static button_t analog_sources[] = {
	{ 0, L_STICK_AS_ANALOG,  "Left Stick" },
	{ 1, R_STICK_AS_ANALOG,  "Right Stick" },
};

static button_t menu_combos[] = {
	{ 0, WIIDRC_BUTTON_X|WIIDRC_BUTTON_Y, "X+Y" },
	{ 1, WIIDRC_BUTTON_ZL|WIIDRC_BUTTON_ZR, "ZL+ZR" },
};

static unsigned int getButtons()
{
	const struct WiiDRCData *data = WiiDRC_Data();

	unsigned int b = data->button;

	if(data->xAxisL < -20) b |= L_STICK_L;
	if(data->xAxisL >  20) b |= L_STICK_R;
	if(data->yAxisL >  20) b |= L_STICK_U;
	if(data->yAxisL < -20) b |= L_STICK_D;

	if(data->xAxisR < -20) b |= R_STICK_L;
	if(data->xAxisR >  20) b |= R_STICK_R;
	if(data->yAxisR >  20) b |= R_STICK_U;
	if(data->yAxisR < -20) b |= R_STICK_D;
	
	return b;
}

static int available(int Control) {
	if(Control == 0 && WiiDRC_Inited() && WiiDRC_Connected())
	{
		controller_DRC.available[Control] = 1;
		return 1;
	}
	else
	{
		controller_DRC.available[Control] = 0;
		return 0;
	}
}

static int _GetKeys(int Control, BUTTONS * Keys, controller_config_t* config)
{
	if(drcNeedScan){ WiiDRC_ScanPads(); drcNeedScan = 0; }
	BUTTONS* c = Keys;
	memset(c, 0, sizeof(BUTTONS));

	// Only use a connected classic controller
	if(!available(Control))
		return 0;

	unsigned int b = getButtons();
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

	if(config->analog->mask == L_STICK_AS_ANALOG){
		c->X_AXIS = getDRCValue(WiiDRC_lStickX());
		c->Y_AXIS = getDRCValue(WiiDRC_lStickY());
	} else if(config->analog->mask == R_STICK_AS_ANALOG){
		c->X_AXIS = getDRCValue(WiiDRC_rStickX());
		c->Y_AXIS = getDRCValue(WiiDRC_rStickY());
	}
	if(config->invertedY) c->Y_AXIS = -c->Y_AXIS;

	// Return whether the exit button(s) are pressed
	return isHeld(config->exit);
}

static void pause(int Control){ }

static void resume(int Control){ }

static void rumble(int Control, int rumble){ }

static void configure(int Control, controller_config_t* config){
	// Don't know how this should be integrated
}

static void assign(int p, int v){
	// TODO: Light up the LEDs appropriately
}

static void refreshAvailable(void);

controller_t controller_DRC =
	{ 'D',
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
	    .Z         = &buttons[7],  // Left Z
	    .L         = &buttons[5],  // Left Trigger
	    .R         = &buttons[6],  // Right Trigger
	    .A         = &buttons[9],  // A
	    .B         = &buttons[10], // B
	    .START     = &buttons[13], // +
	    .CU        = &buttons[16], // Right Stick Up
	    .CL        = &buttons[17], // Right Stick Left
	    .CR        = &buttons[18], // Right Stick Right
	    .CD        = &buttons[19], // Right Stick Down
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
