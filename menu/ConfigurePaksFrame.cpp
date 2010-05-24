/**
 * Wii64 - ConfigurePaksFrame.cpp
 * Copyright (C) 2009, 2010 sepp256
 *
 * Wii64 homepage: http://www.emulatemii.com
 * email address: sepp256@gmail.com
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

#include "MenuContext.h"
#include "ConfigurePaksFrame.h"
#include "../libgui/Button.h"
#include "../libgui/TextBox.h"
#include "../libgui/resources.h"
#include "../libgui/FocusManager.h"
#include "../libgui/CursorManager.h"
#include "../libgui/MessageBox.h"

#include "../main/wii64config.h"

extern "C" {
#include "../main/plugin.h"
#include "../main/rom.h"
#include "../gc_memory/pif.h"
}

void Func_Controller1Mempak();
void Func_Controller1Rumble();
void Func_Controller2Mempak();
void Func_Controller2Rumble();
void Func_Controller3Mempak();
void Func_Controller3Rumble();
void Func_Controller4Mempak();
void Func_Controller4Rumble();
void Func_ReturnFromConfigurePaksFrame();


#define NUM_FRAME_BUTTONS 8
#define FRAME_BUTTONS configurePaksFrameButtons
#define FRAME_STRINGS configurePaksFrameStrings
#define NUM_FRAME_TEXTBOXES 8
#define FRAME_TEXTBOXES configurePaksFrameTextBoxes

static char FRAME_STRINGS[7][13] =
	{ "Mempak",
	  "Rumblepak",
	  "Controller 1",
	  "Controller 2",
	  "Controller 3",
	  "Controller 4",
	  "Unavailable"
};

struct ButtonInfo
{
	menu::Button	*button;
	int				buttonStyle;
	char*			buttonString;
	float			x;
	float			y;
	float			width;
	float			height;
	int				focusUp;
	int				focusDown;
	int				focusLeft;
	int				focusRight;
	ButtonFunc		clickedFunc;
	ButtonFunc		returnFunc;
} FRAME_BUTTONS[NUM_FRAME_BUTTONS] =
{ //	button	buttonStyle	buttonString		x		y		width	height	Up	Dwn	Lft	Rt	clickFunc				returnFunc
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[0],	295.0,	100.0,	160.0,	56.0,	 6,	 2,	 1,	 1,	Func_Controller1Mempak,	Func_ReturnFromConfigurePaksFrame }, // Controller 1: Mempak
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[1],	465.0,	100.0,	160.0,	56.0,	 7,	 3,	 0,	 0,	Func_Controller1Rumble,	Func_ReturnFromConfigurePaksFrame }, // Controller 1: Rumblepak
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[0],	295.0,	170.0,	160.0,	56.0,	 0,	 4,	 3,	 3,	Func_Controller2Mempak,	Func_ReturnFromConfigurePaksFrame }, // Controller 2: Mempak
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[1],	465.0,	170.0,	160.0,	56.0,	 1,	 5,	 2,	 2,	Func_Controller2Rumble,	Func_ReturnFromConfigurePaksFrame }, // Controller 2: Rumblepak
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[0],	295.0,	240.0,	160.0,	56.0,	 2,	 6,	 5,	 5,	Func_Controller3Mempak,	Func_ReturnFromConfigurePaksFrame }, // Controller 3: Mempak
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[1],	465.0,	240.0,	160.0,	56.0,	 3,	 7,	 4,	 4,	Func_Controller3Rumble,	Func_ReturnFromConfigurePaksFrame }, // Controller 3: Rumblepak
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[0],	295.0,	310.0,	160.0,	56.0,	 4,	 0,	 7,	 7,	Func_Controller4Mempak,	Func_ReturnFromConfigurePaksFrame }, // Controller 4: Mempak
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[1],	465.0,	310.0,	160.0,	56.0,	 5,	 1,	 6,	 6,	Func_Controller4Rumble,	Func_ReturnFromConfigurePaksFrame }, // Controller 4: Rumblepak
};

struct TextBoxInfo
{
	menu::TextBox	*textBox;
	char*			textBoxString;
	float			x;
	float			y;
	float			scale;
	bool			centered;
} FRAME_TEXTBOXES[NUM_FRAME_TEXTBOXES] =
{ //	textBox	textBoxString		x		y		scale	centered
	{	NULL,	FRAME_STRINGS[2],	190.0,	128.0,	 1.0,	true }, // Controller 1
	{	NULL,	FRAME_STRINGS[3],	190.0,	198.0,	 1.0,	true }, // Controller 2
	{	NULL,	FRAME_STRINGS[4],	190.0,	268.0,	 1.0,	true }, // Controller 3
	{	NULL,	FRAME_STRINGS[5],	190.0,	338.0,	 1.0,	true }, // Controller 4
	{	NULL,	FRAME_STRINGS[6],	450.0,	128.0,	 1.0,	true }, // Unavailable
	{	NULL,	FRAME_STRINGS[6],	450.0,	198.0,	 1.0,	true }, // Unavailable
	{	NULL,	FRAME_STRINGS[6],	450.0,	268.0,	 1.0,	true }, // Unavailable
	{	NULL,	FRAME_STRINGS[6],	450.0,	338.0,	 1.0,	true }, // Unavailable
};

ConfigurePaksFrame::ConfigurePaksFrame()
{
	for (int i = 0; i < NUM_FRAME_BUTTONS; i++)
		FRAME_BUTTONS[i].button = new menu::Button(FRAME_BUTTONS[i].buttonStyle, &FRAME_BUTTONS[i].buttonString, 
										FRAME_BUTTONS[i].x, FRAME_BUTTONS[i].y, 
										FRAME_BUTTONS[i].width, FRAME_BUTTONS[i].height);

	for (int i = 0; i < NUM_FRAME_BUTTONS; i++)
	{
		if (FRAME_BUTTONS[i].focusUp != -1) FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_UP, FRAME_BUTTONS[FRAME_BUTTONS[i].focusUp].button);
		if (FRAME_BUTTONS[i].focusDown != -1) FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_DOWN, FRAME_BUTTONS[FRAME_BUTTONS[i].focusDown].button);
		if (FRAME_BUTTONS[i].focusLeft != -1) FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_LEFT, FRAME_BUTTONS[FRAME_BUTTONS[i].focusLeft].button);
		if (FRAME_BUTTONS[i].focusRight != -1) FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_RIGHT, FRAME_BUTTONS[FRAME_BUTTONS[i].focusRight].button);
		FRAME_BUTTONS[i].button->setActive(true);
		if (FRAME_BUTTONS[i].clickedFunc) FRAME_BUTTONS[i].button->setClicked(FRAME_BUTTONS[i].clickedFunc);
		if (FRAME_BUTTONS[i].returnFunc) FRAME_BUTTONS[i].button->setReturn(FRAME_BUTTONS[i].returnFunc);
		add(FRAME_BUTTONS[i].button);
		menu::Cursor::getInstance().addComponent(this, FRAME_BUTTONS[i].button, FRAME_BUTTONS[i].x, 
												FRAME_BUTTONS[i].x+FRAME_BUTTONS[i].width, FRAME_BUTTONS[i].y, 
												FRAME_BUTTONS[i].y+FRAME_BUTTONS[i].height);
	}

	for (int i = 0; i < NUM_FRAME_TEXTBOXES; i++)
	{
		FRAME_TEXTBOXES[i].textBox = new menu::TextBox(&FRAME_TEXTBOXES[i].textBoxString, 
										FRAME_TEXTBOXES[i].x, FRAME_TEXTBOXES[i].y, 
										FRAME_TEXTBOXES[i].scale, FRAME_TEXTBOXES[i].centered);
		add(FRAME_TEXTBOXES[i].textBox);
	}

	setDefaultFocus(FRAME_BUTTONS[0].button);
	setBackFunc(Func_ReturnFromConfigurePaksFrame);
	setEnabled(true);
	activateSubmenu(SUBMENU_NONE);
}

ConfigurePaksFrame::~ConfigurePaksFrame()
{
	for (int i = 0; i < NUM_FRAME_TEXTBOXES; i++)
		delete FRAME_TEXTBOXES[i].textBox;
	for (int i = 0; i < NUM_FRAME_BUTTONS; i++)
	{
		menu::Cursor::getInstance().removeComponent(this, FRAME_BUTTONS[i].button);
		delete FRAME_BUTTONS[i].button;
	}

}

void ConfigurePaksFrame::activateSubmenu(int submenu)
{
	Component* defaultFocus = this;

	//All buttons: hide; unselect
	for (int i = 0; i < NUM_FRAME_BUTTONS; i++)
	{
		FRAME_BUTTONS[i].button->setVisible(false);
		FRAME_BUTTONS[i].button->setSelected(false);
		FRAME_BUTTONS[i].button->setActive(false);
	}
	//Unavailable textBoxes: hide
	for (int i = 4; i < NUM_FRAME_TEXTBOXES; i++)
	{
		FRAME_TEXTBOXES[i].textBox->setVisible(false);
	}

	for (int i = 3; i >= 0; i--)
	{
		if(Controls[i].Present)
		{
			//if available, set defaultFocus
			defaultFocus = FRAME_BUTTONS[2*i].button;

			FRAME_BUTTONS[2*i].button->setVisible(true);
			FRAME_BUTTONS[2*i].button->setActive(true);
			FRAME_BUTTONS[2*i+1].button->setVisible(true);
			FRAME_BUTTONS[2*i+1].button->setActive(true);

			if(Controls[i].Plugin == PLUGIN_MEMPAK)
				FRAME_BUTTONS[2*i].button->setSelected(true);	//"Mempak"
			else	
				FRAME_BUTTONS[2*i+1].button->setSelected(true);	//"Rumblepak"

			//set up button
			for (int j = 3; j > 0; j--)
			{
				int controllerInd = (i+j)%4;
				if (Controls[controllerInd].Present) 
				{
					FRAME_BUTTONS[2*i].button->setNextFocus(menu::Focus::DIRECTION_UP, FRAME_BUTTONS[2*controllerInd].button);
					FRAME_BUTTONS[2*i+1].button->setNextFocus(menu::Focus::DIRECTION_UP, FRAME_BUTTONS[2*controllerInd+1].button);
					break;
				}
				if (j == 3)
				{
					FRAME_BUTTONS[2*i].button->setNextFocus(menu::Focus::DIRECTION_UP, NULL);
					FRAME_BUTTONS[2*i+1].button->setNextFocus(menu::Focus::DIRECTION_UP, NULL);
				}
			}
			//set down button
			for (int j = 1; j < 4; j++)
			{
				int controllerInd = (i+j)%4;
				if (Controls[controllerInd].Present) 
				{
					FRAME_BUTTONS[2*i].button->setNextFocus(menu::Focus::DIRECTION_DOWN, FRAME_BUTTONS[2*controllerInd].button);
					FRAME_BUTTONS[2*i+1].button->setNextFocus(menu::Focus::DIRECTION_DOWN, FRAME_BUTTONS[2*controllerInd+1].button);
					break;
				}
				if (j == 3)
				{
					FRAME_BUTTONS[2*i].button->setNextFocus(menu::Focus::DIRECTION_DOWN, NULL);
					FRAME_BUTTONS[2*i+1].button->setNextFocus(menu::Focus::DIRECTION_DOWN, NULL);
				}
			}
		} 
		else 
		{
			FRAME_TEXTBOXES[i+4].textBox->setVisible(true);
		}
	}

	//Set Frame defaultFocus
	setDefaultFocus(defaultFocus);

}

void Func_Controller1Mempak()
{
	FRAME_BUTTONS[0].button->setSelected(true);
	FRAME_BUTTONS[1].button->setSelected(false);
	Controls[0].Plugin = PLUGIN_MEMPAK;
	pakMode[0] = PAKMODE_MEMPAK;
}

void Func_Controller1Rumble()
{
	FRAME_BUTTONS[0].button->setSelected(false);
	FRAME_BUTTONS[1].button->setSelected(true);
	Controls[0].Plugin = PLUGIN_RAW;
	pakMode[0] = PAKMODE_RUMBLEPAK;
}

void Func_Controller2Mempak()
{
	FRAME_BUTTONS[2].button->setSelected(true);
	FRAME_BUTTONS[3].button->setSelected(false);
	Controls[1].Plugin = PLUGIN_MEMPAK;
	pakMode[1] = PAKMODE_MEMPAK;
}

void Func_Controller2Rumble()
{
	FRAME_BUTTONS[2].button->setSelected(false);
	FRAME_BUTTONS[3].button->setSelected(true);
	Controls[1].Plugin = PLUGIN_RAW;
	pakMode[1] = PAKMODE_RUMBLEPAK;
}

void Func_Controller3Mempak()
{
	FRAME_BUTTONS[4].button->setSelected(true);
	FRAME_BUTTONS[5].button->setSelected(false);
	Controls[2].Plugin = PLUGIN_MEMPAK;
	pakMode[2] = PAKMODE_MEMPAK;
}

void Func_Controller3Rumble()
{
	FRAME_BUTTONS[4].button->setSelected(false);
	FRAME_BUTTONS[5].button->setSelected(true);
	Controls[2].Plugin = PLUGIN_RAW;
	pakMode[2] = PAKMODE_RUMBLEPAK;
}

void Func_Controller4Mempak()
{
	FRAME_BUTTONS[6].button->setSelected(true);
	FRAME_BUTTONS[7].button->setSelected(false);
	Controls[3].Plugin = PLUGIN_MEMPAK;
	pakMode[3] = PAKMODE_MEMPAK;
}

void Func_Controller4Rumble()
{
	FRAME_BUTTONS[6].button->setSelected(false);
	FRAME_BUTTONS[7].button->setSelected(true);
	Controls[3].Plugin = PLUGIN_RAW;
	pakMode[3] = PAKMODE_RUMBLEPAK;
}

extern MenuContext *pMenuContext;

void Func_ReturnFromConfigurePaksFrame()
{
	pMenuContext->setActiveFrame(MenuContext::FRAME_SETTINGS,SettingsFrame::SUBMENU_INPUT);
}
