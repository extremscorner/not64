/**
 * Wii64 - ConfigureButtonsFrame.cpp
 * Copyright (C) 2010 sepp256
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
#include "ConfigureButtonsFrame.h"
#include "../libgui/Button.h"
#include "../libgui/TextBox.h"
#include "../libgui/resources.h"
#include "../libgui/FocusManager.h"
#include "../libgui/CursorManager.h"
#include "../libgui/MessageBox.h"
//#include "../main/timers.h"
#include "../main/wii64config.h"

extern "C" {
#include "../gc_input/controller.h"
}

void Func_DefaultConfig();
void Func_ToggleConfigSlot();
void Func_LoadConfig();
void Func_SaveConfig();

void Func_ToggleButtonL();
void Func_ToggleButtonR();
void Func_ToggleButtonZ();
void Func_ToggleButtonDup();
void Func_ToggleButtonDleft();
void Func_ToggleButtonDright();
void Func_ToggleButtonDdown();
void Func_ToggleButtonCup();
void Func_ToggleButtonCleft();
void Func_ToggleButtonCright();
void Func_ToggleButtonCdown();
void Func_ToggleButtonStart();
void Func_ToggleButtonB();
void Func_ToggleButtonA();
void Func_ToggleAnalogStick();
void Func_ToggleInvertY();
void Func_ToggleButtonExit();

void Func_ReturnFromConfigureButtonsFrame();


#define NUM_FRAME_BUTTONS 21
#define FRAME_BUTTONS configureButtonsFrameButtons
#define FRAME_STRINGS configureButtonsFrameStrings
#define NUM_FRAME_TEXTBOXES 2
#define FRAME_TEXTBOXES configureButtonsFrameTextBoxes
#define TITLE_STRING configureButtonsTitleString

static char FRAME_STRINGS[22][20] =
	{ "Default Config",
	  "Slot 1",
	  "Load Config",
	  "Save Config",
	  "Btn L",
	  "Btn R",
	  "Btn Z",
	  "Btn Dup",
	  "Btn Dleft",
	  "Btn Dright",
	  "Btn Ddown",
	  "Btn Cup",
	  "Btn Cleft",
	  "Btn Cright",
	  "Btn Cdown",
	  "Btn Start",
	  "Btn B",
	  "Btn A",
	  "Analog Stick",
	  "Invert Y",
	  "X+Y",
	  "Menu Combo:"};

static char TITLE_STRING[50] = "Gamecube Pad 1 to N64 Pad 1 Mapping";

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
{ //	button	buttonStyle buttonString		x		y		width	height	Up	Dwn	Lft	Rt	clickFunc				returnFunc
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[0],	 40.0,	 90.0,	160.0,	40.0,	19,	 4,	 3,	 1,	Func_DefaultConfig,		Func_ReturnFromConfigureButtonsFrame }, // Restore Default Config
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[1],	220.0,	 90.0,	 80.0,	40.0,	18,	 6,	 0,	 2,	Func_ToggleConfigSlot,	Func_ReturnFromConfigureButtonsFrame }, // Cycle Through Config Slots
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[2],	320.0,	 90.0,	130.0,	40.0,	17,	 6,	 1,	 3,	Func_LoadConfig,		Func_ReturnFromConfigureButtonsFrame }, // Load Config
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[3],	470.0,	 90.0,	130.0,	40.0,	20,	 5,	 2,	 0,	Func_SaveConfig,		Func_ReturnFromConfigureButtonsFrame }, // Save Config

	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[4],	140.0,	150.0,	 80.0,	40.0,	 0,	 7,	 5,	 6,	Func_ToggleButtonL,		Func_ReturnFromConfigureButtonsFrame }, // Toggle Button L
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[5],	420.0,	150.0,	 80.0,	40.0,	 1,	11,	 6,	 4,	Func_ToggleButtonR,		Func_ReturnFromConfigureButtonsFrame }, // Toggle Button R
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[6],	280.0,	150.0,	 80.0,	40.0,	 3,	15,	 4,	 5,	Func_ToggleButtonZ,		Func_ReturnFromConfigureButtonsFrame }, // Toggle Button Z

	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[7],	 85.0,	200.0,	 80.0,	40.0,	 4,	 8,	11,	11,	Func_ToggleButtonDup,	Func_ReturnFromConfigureButtonsFrame }, // Toggle Button D-up
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[8],	 40.0,	250.0,	 80.0,	40.0,	 7,	10,	13,	 9,	Func_ToggleButtonDleft,	Func_ReturnFromConfigureButtonsFrame }, // Toggle Button D-left
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[9],	130.0,	250.0,	 80.0,	40.0,	 7,	10,	 8,	12,	Func_ToggleButtonDright,Func_ReturnFromConfigureButtonsFrame }, // Toggle Button D-right
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[10],	 85.0,	300.0,	 80.0,	40.0,	 8,	15,	14,	15,	Func_ToggleButtonDdown,	Func_ReturnFromConfigureButtonsFrame }, // Toggle Button D-down

	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[11],	475.0,	200.0,	 80.0,	40.0,	 5,	12,	 7,	 7,	Func_ToggleButtonCup,	Func_ReturnFromConfigureButtonsFrame }, // Toggle Button C-up
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[12],	430.0,	250.0,	 80.0,	40.0,	11,	14,	 9,	13,	Func_ToggleButtonCleft,	Func_ReturnFromConfigureButtonsFrame }, // Toggle Button C-left
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[13],	520.0,	250.0,	 80.0,	40.0,	11,	14,	12,	 8,	Func_ToggleButtonCright,Func_ReturnFromConfigureButtonsFrame }, // Toggle Button C-right
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[14],	475.0,	300.0,	 80.0,	40.0,	12,	16,	16,	10,	Func_ToggleButtonCdown,	Func_ReturnFromConfigureButtonsFrame }, // Toggle Button C-down

	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[15],	200.0,	345.0,	 80.0,	40.0,	10,	18,	10,	16,	Func_ToggleButtonStart,	Func_ReturnFromConfigureButtonsFrame }, // Toggle Button Start
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[16],	350.0,	345.0,	 80.0,	40.0,	14,	17,	15,	14,	Func_ToggleButtonB,		Func_ReturnFromConfigureButtonsFrame }, // Toggle Button B
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[17],	385.0,	395.0,	 80.0,	40.0,	16,	 2,	18,	20,	Func_ToggleButtonA,		Func_ReturnFromConfigureButtonsFrame }, // Toggle Button A
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[18],	160.0,	395.0,	160.0,	40.0,	15,	 1,	19,	17,	Func_ToggleAnalogStick,	Func_ReturnFromConfigureButtonsFrame }, // Toggle Analog Stick
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[19],	 50.0,	395.0,	160.0,	40.0,	15,	 0,	20,	18,	Func_ToggleInvertY,		Func_ReturnFromConfigureButtonsFrame }, // Toggle Analog Invert Y
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[20],	500.0,	395.0,	160.0,	40.0,	16,	 3,	17,	19,	Func_ToggleButtonExit,	Func_ReturnFromConfigureButtonsFrame }, // Toggle Button Exit
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
	{	NULL,	TITLE_STRING,		320.0,	 60.0,	 1.0,	true }, // ______ Pad X to N64 Pad Y Mapping
	{	NULL,	FRAME_STRINGS[21],	540.0,	365.0,	 1.0,	true }, // Menu Combo
};

ConfigureButtonsFrame::ConfigureButtonsFrame()
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
	setBackFunc(Func_ReturnFromConfigureButtonsFrame);
	setEnabled(true);
	activateSubmenu(SUBMENU_N64_PADNONE);
}

ConfigureButtonsFrame::~ConfigureButtonsFrame()
{
	for (int i = 0; i < NUM_FRAME_TEXTBOXES; i++)
		delete FRAME_TEXTBOXES[i].textBox;
	for (int i = 0; i < NUM_FRAME_BUTTONS; i++)
	{
		menu::Cursor::getInstance().removeComponent(this, FRAME_BUTTONS[i].button);
		delete FRAME_BUTTONS[i].button;
	}

}

static char controllerTypeStrings[4][17] =
	{ "Gamecube",
	  "Classic",
	  "Wiimote+Nunchuck",
	  "NULL"};

enum ActivePadType
{
	ACTIVEPADTYPE_GAMECUBE=0,
	ACTIVEPADTYPE_CLASSIC,
	ACTIVEPADTYPE_WIIMOTENUNCHUCK,
	ACTIVEPADTYPE_NONE,
};

int activePad;
int activePadType;
controller_config_t currentConfig;

void ConfigureButtonsFrame::activateSubmenu(int submenu)
{
	activePad = submenu;

	if (activePad == SUBMENU_N64_PADNONE)
		return;

	//Fill out title text
	if (virtualControllers[activePad].control == &controller_GC)
		activePadType = ACTIVEPADTYPE_GAMECUBE;
#ifdef HW_RVL
	else if (virtualControllers[activePad].control == &controller_Classic)
		activePadType = ACTIVEPADTYPE_CLASSIC;
	else if (virtualControllers[activePad].control == &controller_WiimoteNunchuk)
		activePadType = ACTIVEPADTYPE_WIIMOTENUNCHUCK;
#endif //HW_RVL
	else
		activePadType = ACTIVEPADTYPE_NONE;

	sprintf(TITLE_STRING, "%s Pad %d to N64 Pad %d Mapping", controllerTypeStrings[activePadType], virtualControllers[activePad].number, activePad );
	
	memcpy(&currentConfig, &virtualControllers[activePad].control->config[activePad], sizeof(controller_config_t));

	//Assign text to each button
	strcpy(FRAME_STRINGS[4], virtualControllers[activePad].control->buttons[currentConfig.L].name);
	strcpy(FRAME_STRINGS[5], virtualControllers[activePad].control->buttons[currentConfig.R].name);
	strcpy(FRAME_STRINGS[6], virtualControllers[activePad].control->buttons[currentConfig.Z].name);
	strcpy(FRAME_STRINGS[7], virtualControllers[activePad].control->buttons[currentConfig.DU].name);
	strcpy(FRAME_STRINGS[8], virtualControllers[activePad].control->buttons[currentConfig.DL].name);
	strcpy(FRAME_STRINGS[9], virtualControllers[activePad].control->buttons[currentConfig.DR].name);
	strcpy(FRAME_STRINGS[10], virtualControllers[activePad].control->buttons[currentConfig.DD].name);
	strcpy(FRAME_STRINGS[11], virtualControllers[activePad].control->buttons[currentConfig.CU].name);
	strcpy(FRAME_STRINGS[12], virtualControllers[activePad].control->buttons[currentConfig.CL].name);
	strcpy(FRAME_STRINGS[13], virtualControllers[activePad].control->buttons[currentConfig.CR].name);
	strcpy(FRAME_STRINGS[14], virtualControllers[activePad].control->buttons[currentConfig.CD].name);
	strcpy(FRAME_STRINGS[15], virtualControllers[activePad].control->buttons[currentConfig.START].name);
	strcpy(FRAME_STRINGS[16], virtualControllers[activePad].control->buttons[currentConfig.B].name);
	strcpy(FRAME_STRINGS[17], virtualControllers[activePad].control->buttons[currentConfig.A].name);
	strcpy(FRAME_STRINGS[18], virtualControllers[activePad].control->analog_sources[currentConfig.analog].name);
	if (currentConfig.invertedY)
		strcpy(FRAME_STRINGS[19], "Inverted Y");
	else
		strcpy(FRAME_STRINGS[19], "Normal Y");

	strcpy(FRAME_STRINGS[20], virtualControllers[activePad].control->menu_combos[currentConfig.exit].name);
}

extern MenuContext *pMenuContext;

void Func_DefaultConfig()
{
	menu::MessageBox::getInstance().setMessage("Load Default Config not implemented");
}

static unsigned int which_slot = 0;

void Func_ToggleConfigSlot()
{
	which_slot = (which_slot+1) %4;
	FRAME_STRINGS[1][5] = which_slot + '1';
}

void Func_LoadConfig()
{
	menu::MessageBox::getInstance().setMessage("Load Config not implemented");
	//load config from which_slot+1
//	pMenuContext->getFrame(MenuContext::FRAME_CONFIGUREBUTTONS)->activateSubmenu(activePad);
}

void Func_SaveConfig()
{
	menu::MessageBox::getInstance().setMessage("Save Config not implemented");
	//save config to wich_slot+1
}

void Func_ToggleButtonL()
{
	int currentButton = virtualControllers[activePad].control->config[activePad].L;
	currentButton = (currentButton+1) %virtualControllers[activePad].control->num_buttons;
	virtualControllers[activePad].control->config[activePad].L = currentButton;
	strcpy(FRAME_STRINGS[4], virtualControllers[activePad].control->buttons[currentButton].name);
}

void Func_ToggleButtonR()
{
	int currentButton = virtualControllers[activePad].control->config[activePad].R;
	currentButton = (currentButton+1) %virtualControllers[activePad].control->num_buttons;
	virtualControllers[activePad].control->config[activePad].R = currentButton;
	strcpy(FRAME_STRINGS[5], virtualControllers[activePad].control->buttons[currentButton].name);
}

void Func_ToggleButtonZ()
{
	int currentButton = virtualControllers[activePad].control->config[activePad].Z;
	currentButton = (currentButton+1) %virtualControllers[activePad].control->num_buttons;
	virtualControllers[activePad].control->config[activePad].Z = currentButton;
	strcpy(FRAME_STRINGS[6], virtualControllers[activePad].control->buttons[currentButton].name);
}

void Func_ToggleButtonDup()
{
	int currentButton = virtualControllers[activePad].control->config[activePad].DU;
	currentButton = (currentButton+1) %virtualControllers[activePad].control->num_buttons;
	virtualControllers[activePad].control->config[activePad].DU = currentButton;
	strcpy(FRAME_STRINGS[7], virtualControllers[activePad].control->buttons[currentButton].name);
}

void Func_ToggleButtonDleft()
{
	int currentButton = virtualControllers[activePad].control->config[activePad].DL;
	currentButton = (currentButton+1) %virtualControllers[activePad].control->num_buttons;
	virtualControllers[activePad].control->config[activePad].DL = currentButton;
	strcpy(FRAME_STRINGS[8], virtualControllers[activePad].control->buttons[currentButton].name);
}

void Func_ToggleButtonDright()
{
	int currentButton = virtualControllers[activePad].control->config[activePad].DR;
	currentButton = (currentButton+1) %virtualControllers[activePad].control->num_buttons;
	virtualControllers[activePad].control->config[activePad].DR = currentButton;
	strcpy(FRAME_STRINGS[9], virtualControllers[activePad].control->buttons[currentButton].name);
}

void Func_ToggleButtonDdown()
{
	int currentButton = virtualControllers[activePad].control->config[activePad].DD;
	currentButton = (currentButton+1) %virtualControllers[activePad].control->num_buttons;
	virtualControllers[activePad].control->config[activePad].DD = currentButton;
	strcpy(FRAME_STRINGS[10], virtualControllers[activePad].control->buttons[currentButton].name);
}

void Func_ToggleButtonCup()
{
	int currentButton = virtualControllers[activePad].control->config[activePad].CU;
	currentButton = (currentButton+1) %virtualControllers[activePad].control->num_buttons;
	virtualControllers[activePad].control->config[activePad].CU = currentButton;
	strcpy(FRAME_STRINGS[11], virtualControllers[activePad].control->buttons[currentButton].name);
}

void Func_ToggleButtonCleft()
{
	int currentButton = virtualControllers[activePad].control->config[activePad].CL;
	currentButton = (currentButton+1) %virtualControllers[activePad].control->num_buttons;
	virtualControllers[activePad].control->config[activePad].CL = currentButton;
	strcpy(FRAME_STRINGS[12], virtualControllers[activePad].control->buttons[currentButton].name);
}

void Func_ToggleButtonCright()
{
	int currentButton = virtualControllers[activePad].control->config[activePad].CR;
	currentButton = (currentButton+1) %virtualControllers[activePad].control->num_buttons;
	virtualControllers[activePad].control->config[activePad].CR = currentButton;
	strcpy(FRAME_STRINGS[13], virtualControllers[activePad].control->buttons[currentButton].name);
}

void Func_ToggleButtonCdown()
{
	int currentButton = virtualControllers[activePad].control->config[activePad].CD;
	currentButton = (currentButton+1) %virtualControllers[activePad].control->num_buttons;
	virtualControllers[activePad].control->config[activePad].CD = currentButton;
	strcpy(FRAME_STRINGS[14], virtualControllers[activePad].control->buttons[currentButton].name);
}

void Func_ToggleButtonStart()
{
	int currentButton = virtualControllers[activePad].control->config[activePad].START;
	currentButton = (currentButton+1) %virtualControllers[activePad].control->num_buttons;
	virtualControllers[activePad].control->config[activePad].START = currentButton;
	strcpy(FRAME_STRINGS[15], virtualControllers[activePad].control->buttons[currentButton].name);
}

void Func_ToggleButtonB()
{
	int currentButton = virtualControllers[activePad].control->config[activePad].B;
	currentButton = (currentButton+1) %virtualControllers[activePad].control->num_buttons;
	virtualControllers[activePad].control->config[activePad].B = currentButton;
	strcpy(FRAME_STRINGS[16], virtualControllers[activePad].control->buttons[currentButton].name);
}

void Func_ToggleButtonA()
{
	int currentButton = virtualControllers[activePad].control->config[activePad].A;
	currentButton = (currentButton+1) %virtualControllers[activePad].control->num_buttons;
	virtualControllers[activePad].control->config[activePad].A = currentButton;
	strcpy(FRAME_STRINGS[17], virtualControllers[activePad].control->buttons[currentButton].name);
}

void Func_ToggleAnalogStick()
{
	int currentButton = virtualControllers[activePad].control->config[activePad].analog;
	currentButton = (currentButton+1) %virtualControllers[activePad].control->num_analog_sources;
	virtualControllers[activePad].control->config[activePad].analog = currentButton;
	strcpy(FRAME_STRINGS[18], virtualControllers[activePad].control->analog_sources[currentButton].name);
}

void Func_ToggleInvertY()
{
	int invertedY = virtualControllers[activePad].control->config[activePad].invertedY;
	if (invertedY)
	{
		virtualControllers[activePad].control->config[activePad].invertedY = 0;
		strcpy(FRAME_STRINGS[19], "Normal Y");
	}
	else
	{
		virtualControllers[activePad].control->config[activePad].invertedY = 1;
		strcpy(FRAME_STRINGS[19], "Inverted Y");
	}
}

void Func_ToggleButtonExit()
{
	int currentButton = virtualControllers[activePad].control->config[activePad].exit;
	currentButton = (currentButton+1) %virtualControllers[activePad].control->num_menu_combos;
	virtualControllers[activePad].control->config[activePad].exit = currentButton;
	strcpy(FRAME_STRINGS[20], virtualControllers[activePad].control->menu_combos[currentButton].name);
}

void Func_ReturnFromConfigureButtonsFrame()
{
	pMenuContext->setActiveFrame(MenuContext::FRAME_CONFIGUREINPUT,ConfigureInputFrame::SUBMENU_REINIT);
}
