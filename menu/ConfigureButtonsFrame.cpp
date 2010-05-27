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
#include "../libgui/GuiTypes.h"
#include "../libgui/GuiResources.h"
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

void Func_NextPad();
void Func_DefaultConfig();
void Func_ToggleConfigSlot();
void Func_LoadConfig();
void Func_SaveConfig();

void Func_ToggleButtonL();
void Func_ToggleButtonR();
void Func_ToggleButtonStart();
void Func_ToggleButtonDup();
void Func_ToggleButtonDleft();
void Func_ToggleButtonDright();
void Func_ToggleButtonDdown();
void Func_ToggleButtonCup();
void Func_ToggleButtonCleft();
void Func_ToggleButtonCright();
void Func_ToggleButtonCdown();
void Func_ToggleButtonZ();
void Func_ToggleButtonB();
void Func_ToggleButtonA();
void Func_ToggleAnalogStick();
void Func_ToggleInvertY();
void Func_ToggleButtonExit();

void Func_ReturnFromConfigureButtonsFrame();


#define NUM_FRAME_BUTTONS 22
#define FRAME_BUTTONS configureButtonsFrameButtons
#define FRAME_STRINGS configureButtonsFrameStrings
#define NUM_FRAME_TEXTBOXES 2
#define FRAME_TEXTBOXES configureButtonsFrameTextBoxes
#define TITLE_STRING configureButtonsTitleString

static char FRAME_STRINGS[23][20] =
	{ "Next Pad",
	  "Load Default",
	  "Slot 1",
	  "Load",
	  "Save",
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
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[0],	 40.0,	 90.0,	115.0,	40.0,	20,	 5,	 4,	 1,	Func_NextPad,			Func_ReturnFromConfigureButtonsFrame }, // Next Pad
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[1],	175.0,	 90.0,	145.0,	40.0,	20,	 5,	 0,	 2,	Func_DefaultConfig,		Func_ReturnFromConfigureButtonsFrame }, // Restore Default Config
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[2],	340.0,	 90.0,	 80.0,	40.0,	19,	 7,	 1,	 3,	Func_ToggleConfigSlot,	Func_ReturnFromConfigureButtonsFrame }, // Cycle Through Config Slots
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[3],	440.0,	 90.0,	 70.0,	40.0,	18,	 6,	 2,	 4,	Func_LoadConfig,		Func_ReturnFromConfigureButtonsFrame }, // Load Config
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[4],	530.0,	 90.0,	 70.0,	40.0,	21,	 6,	 3,	 0,	Func_SaveConfig,		Func_ReturnFromConfigureButtonsFrame }, // Save Config

	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[5],	140.0,	150.0,	 80.0,	40.0,	 0,	 8,	 6,	 7,	Func_ToggleButtonL,		Func_ReturnFromConfigureButtonsFrame }, // Toggle Button L
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[6],	420.0,	150.0,	 80.0,	40.0,	 3,	12,	 7,	 5,	Func_ToggleButtonR,		Func_ReturnFromConfigureButtonsFrame }, // Toggle Button R
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[7],	280.0,	140.0,	 80.0,	40.0,	 2,	16,	 5,	 6,	Func_ToggleButtonStart,	Func_ReturnFromConfigureButtonsFrame }, // Toggle Button Start

	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[8],	 85.0,	200.0,	 80.0,	40.0,	 5,	 9,	12,	12,	Func_ToggleButtonDup,	Func_ReturnFromConfigureButtonsFrame }, // Toggle Button D-up
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[9],	 40.0,	250.0,	 80.0,	40.0,	 8,	11,	14,	10,	Func_ToggleButtonDleft,	Func_ReturnFromConfigureButtonsFrame }, // Toggle Button D-left
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[10],	130.0,	250.0,	 80.0,	40.0,	 8,	11,	 9,	13,	Func_ToggleButtonDright,Func_ReturnFromConfigureButtonsFrame }, // Toggle Button D-right
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[11],	 85.0,	300.0,	 80.0,	40.0,	 9,	16,	15,	16,	Func_ToggleButtonDdown,	Func_ReturnFromConfigureButtonsFrame }, // Toggle Button D-down

	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[12],	475.0,	200.0,	 80.0,	40.0,	 6,	13,	 8,	 8,	Func_ToggleButtonCup,	Func_ReturnFromConfigureButtonsFrame }, // Toggle Button C-up
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[13],	430.0,	250.0,	 80.0,	40.0,	12,	15,	10,	14,	Func_ToggleButtonCleft,	Func_ReturnFromConfigureButtonsFrame }, // Toggle Button C-left
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[14],	520.0,	250.0,	 80.0,	40.0,	12,	15,	13,	 9,	Func_ToggleButtonCright,Func_ReturnFromConfigureButtonsFrame }, // Toggle Button C-right
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[15],	475.0,	300.0,	 80.0,	40.0,	13,	17,	17,	11,	Func_ToggleButtonCdown,	Func_ReturnFromConfigureButtonsFrame }, // Toggle Button C-down

	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[16],	210.0,	345.0,	 80.0,	40.0,	11,	19,	11,	17,	Func_ToggleButtonZ,		Func_ReturnFromConfigureButtonsFrame }, // Toggle Button Z
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[17],	370.0,	345.0,	 80.0,	40.0,	15,	18,	16,	15,	Func_ToggleButtonB,		Func_ReturnFromConfigureButtonsFrame }, // Toggle Button B
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[18],	395.0,	395.0,	 80.0,	40.0,	17,	 3,	19,	21,	Func_ToggleButtonA,		Func_ReturnFromConfigureButtonsFrame }, // Toggle Button A
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[19],	200.0,	395.0,	160.0,	40.0,	16,	 2,	20,	18,	Func_ToggleAnalogStick,	Func_ReturnFromConfigureButtonsFrame }, // Toggle Analog Stick
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[20],	 60.0,	395.0,	130.0,	40.0,	16,	 0,	21,	19,	Func_ToggleInvertY,		Func_ReturnFromConfigureButtonsFrame }, // Toggle Analog Invert Y
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[21],	495.0,	395.0,	100.0,	40.0,	17,	 4,	18,	20,	Func_ToggleButtonExit,	Func_ReturnFromConfigureButtonsFrame }, // Toggle Button Exit
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
	{	NULL,	FRAME_STRINGS[22],	545.0,	380.0,	 0.8,	true }, // Menu Combo
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
	for (int i = 0; i < 5; i++)
		FRAME_BUTTONS[i].button->setFontSize(0.8);
	for (int i = 5; i < NUM_FRAME_BUTTONS; i++)
		FRAME_BUTTONS[i].button->setFontSize(0.7);

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

static char controllerTypeStrings[5][17] =
	{ "Gamecube",
	  "Classic",
	  "Wiimote+Nunchuck",
	  "Wiimote",
	  "NULL"};

enum ActivePadType
{
	ACTIVEPADTYPE_GAMECUBE=0,
	ACTIVEPADTYPE_CLASSIC,
	ACTIVEPADTYPE_WIIMOTENUNCHUCK,
	ACTIVEPADTYPE_WIIMOTE,
	ACTIVEPADTYPE_NONE,
};

enum ActivePadAssigned
{
	ACTIVEPADASSIGNED_FALSE=0,
	ACTIVEPADASSIGNED_TRUE,
};

int activePad=ConfigureButtonsFrame::SUBMENU_N64_PAD0;
int activePadType=ACTIVEPADTYPE_NONE;
int activePadAssigned=ACTIVEPADASSIGNED_TRUE;

void ConfigureButtonsFrame::activateSubmenu(int submenu)
{
	controller_config_t* currentConfig;

	if (submenu != SUBMENU_N64_PADNONE)
	{
		activePad = submenu;
		menu::Gui::getInstance().menuLogo->setVisible(false);
	}

	//Fill out title text
	if (virtualControllers[activePad].control == &controller_GC)
		activePadType = ACTIVEPADTYPE_GAMECUBE;
#ifdef HW_RVL
	else if (virtualControllers[activePad].control == &controller_Classic)
		activePadType = ACTIVEPADTYPE_CLASSIC;
	else if (virtualControllers[activePad].control == &controller_WiimoteNunchuk)
		activePadType = ACTIVEPADTYPE_WIIMOTENUNCHUCK;
	else if (virtualControllers[activePad].control == &controller_Wiimote)
		activePadType = ACTIVEPADTYPE_WIIMOTE;
#endif //HW_RVL
	else
		activePadType = ACTIVEPADTYPE_NONE;

	if (activePadType == ACTIVEPADTYPE_NONE)
	{
		sprintf(TITLE_STRING, "N64 Pad %d: No Physical Controller Assigned", activePad+1 );

		if (activePadAssigned == ACTIVEPADASSIGNED_TRUE) //Reset to "Next Pad" button
		{
			activePadAssigned = ACTIVEPADASSIGNED_FALSE;
			menu::Focus::getInstance().clearPrimaryFocus();
		}
		for (int i = 5; i < 22; i++)
			strcpy(FRAME_STRINGS[i], "");
		FRAME_BUTTONS[0].button->setNextFocus(menu::Focus::DIRECTION_UP, NULL);
		FRAME_BUTTONS[0].button->setNextFocus(menu::Focus::DIRECTION_DOWN, NULL);
		FRAME_BUTTONS[0].button->setNextFocus(menu::Focus::DIRECTION_LEFT, NULL);
		FRAME_BUTTONS[0].button->setNextFocus(menu::Focus::DIRECTION_RIGHT, NULL);
		for (int i = 1; i < NUM_FRAME_BUTTONS; i++)
			FRAME_BUTTONS[i].button->setActive(false);
	}
	else
	{
		sprintf(TITLE_STRING, "N64 Pad %d: %s Pad %d Mapping", activePad+1, controllerTypeStrings[activePadType], virtualControllers[activePad].number+1 );
	
		currentConfig = virtualControllers[activePad].config;

		if (activePadAssigned == ACTIVEPADASSIGNED_FALSE) //Reset to "Next Pad" button
		{
			activePadAssigned = ACTIVEPADASSIGNED_TRUE;
		}

		FRAME_BUTTONS[0].button->setNextFocus(menu::Focus::DIRECTION_UP, FRAME_BUTTONS[FRAME_BUTTONS[0].focusUp].button);
		FRAME_BUTTONS[0].button->setNextFocus(menu::Focus::DIRECTION_DOWN, FRAME_BUTTONS[FRAME_BUTTONS[0].focusDown].button);
		FRAME_BUTTONS[0].button->setNextFocus(menu::Focus::DIRECTION_LEFT, FRAME_BUTTONS[FRAME_BUTTONS[0].focusLeft].button);
		FRAME_BUTTONS[0].button->setNextFocus(menu::Focus::DIRECTION_RIGHT, FRAME_BUTTONS[FRAME_BUTTONS[0].focusRight].button);
		for (int i = 1; i < NUM_FRAME_BUTTONS; i++)
			FRAME_BUTTONS[i].button->setActive(true);

		//Assign text to each button
		strcpy(FRAME_STRINGS[5], currentConfig->L->name);
		strcpy(FRAME_STRINGS[6], currentConfig->R->name);
		strcpy(FRAME_STRINGS[7], currentConfig->START->name);
		strcpy(FRAME_STRINGS[8], currentConfig->DU->name);
		strcpy(FRAME_STRINGS[9], currentConfig->DL->name);
		strcpy(FRAME_STRINGS[10], currentConfig->DR->name);
		strcpy(FRAME_STRINGS[11], currentConfig->DD->name);
		strcpy(FRAME_STRINGS[12], currentConfig->CU->name);
		strcpy(FRAME_STRINGS[13], currentConfig->CL->name);
		strcpy(FRAME_STRINGS[14], currentConfig->CR->name);
		strcpy(FRAME_STRINGS[15], currentConfig->CD->name);
		strcpy(FRAME_STRINGS[16], currentConfig->Z->name);
		strcpy(FRAME_STRINGS[17], currentConfig->B->name);
		strcpy(FRAME_STRINGS[18], currentConfig->A->name);
		strcpy(FRAME_STRINGS[19], currentConfig->analog->name);
		if (currentConfig->invertedY)
			strcpy(FRAME_STRINGS[20], "Inverted Y");
		else
			strcpy(FRAME_STRINGS[20], "Normal Y");

		strcpy(FRAME_STRINGS[21], currentConfig->exit->name);
	}
}

void ConfigureButtonsFrame::updateFrame(float deltaTime)
{
	activateSubmenu(activePad);
}

#define NUM_LINES 10

void ConfigureButtonsFrame::drawChildren(menu::Graphics &gfx)
{
	if(isVisible())
	{
		int base_x = 216;
		int base_y = 182;
		int lines[NUM_LINES][4] = {{320, 160, 320, 237}, //START
								   {174, 235, 240, 235}, //D-pad
								   {466, 235, 404, 235}, //C-pad
								   {264, 197, 220, 170}, //L
								   {376, 197, 420, 170}, //R
								   {295, 313, 250, 365}, //Z
								   {320, 299, 320, 415}, //Analog
								   {320, 415, 125, 415}, //Invert Y
								   {373, 268, 373, 325}, //B
								   {373, 325, 435, 405}};//A

		GXColor controllerColors[5] = {	{  1,  29, 169, 255}, //blue
										{254,  32,  21, 255}, //orange/red
										{  8, 147,  48, 255}, //green
										{255, 192,   1, 255}, //yellow/gold
										{150, 150, 255, 255}};

		//Draw N64 Controller
		menu::Image* controllerIcon = NULL;
		gfx.setColor(controllerColors[activePad]);
		controllerIcon = menu::Resources::getInstance().getImage(menu::Resources::IMAGE_N64_CONTROLLER);
		controllerIcon->activateImage(GX_TEXMAP0);
		GX_SetTevColorIn(GX_TEVSTAGE0,GX_CC_ZERO,GX_CC_ZERO,GX_CC_ZERO,GX_CC_RASC);
		GX_SetTevColorOp(GX_TEVSTAGE0,GX_TEV_ADD,GX_TB_ZERO,GX_CS_SCALE_1,GX_TRUE,GX_TEVPREV);
		GX_SetTevAlphaIn(GX_TEVSTAGE0,GX_CA_ZERO,GX_CA_RASA,GX_CA_TEXA,GX_CA_ZERO);
		GX_SetTevAlphaOp(GX_TEVSTAGE0,GX_TEV_ADD,GX_TB_ZERO,GX_CS_SCALE_1,GX_TRUE,GX_TEVPREV);
		gfx.enableBlending(true);
		gfx.drawImage(0, base_x, base_y, 208, 200, 0, 1, 0, 1);
		gfx.setTEV(GX_PASSCLR);

		//Draw lines and circles
		gfx.setColor(controllerColors[4]);
		gfx.setLineWidth(2);
		gfx.drawCircle(125, 270, 60, 33);
		gfx.drawCircle(515, 270, 60, 33);

		for (int i=0; i<NUM_LINES; i++)
			gfx.drawLine(lines[i][0], lines[i][1], lines[i][2], lines[i][3]);

		//Draw buttons
		menu::ComponentList::const_iterator iteration;
		for (iteration = componentList.begin(); iteration != componentList.end(); iteration++)
		{
			(*iteration)->draw(gfx);
		}
	}
}

extern MenuContext *pMenuContext;

void Func_NextPad()
{
	activePad = (activePad+1) %4;

	pMenuContext->getFrame(MenuContext::FRAME_CONFIGUREBUTTONS)->activateSubmenu(activePad);
}

void Func_DefaultConfig()
{
	controller_config_t* currentConfig = virtualControllers[activePad].config;
	memcpy(currentConfig, &virtualControllers[activePad].control->config_default, sizeof(controller_config_t));

	pMenuContext->getFrame(MenuContext::FRAME_CONFIGUREBUTTONS)->activateSubmenu(activePad);
}

static unsigned int which_slot = 0;

void Func_ToggleConfigSlot()
{
	which_slot = (which_slot+1) %4;
	FRAME_STRINGS[2][5] = which_slot + '1';
}

void Func_LoadConfig()
{
	controller_config_t* currentConfig = virtualControllers[activePad].config;
	memcpy(currentConfig, &virtualControllers[activePad].control->config_slot[which_slot], sizeof(controller_config_t));

	pMenuContext->getFrame(MenuContext::FRAME_CONFIGUREBUTTONS)->activateSubmenu(activePad);
}

void Func_SaveConfig()
{
	char buffer [50] = "";
	controller_config_t* currentConfig = virtualControllers[activePad].config;
	memcpy(&virtualControllers[activePad].control->config_slot[which_slot], currentConfig, sizeof(controller_config_t));

	//todo: save button configuration to file here

	sprintf(buffer,"Saved current button mapping to slot %d.",which_slot+1);
	menu::MessageBox::getInstance().fadeMessage(buffer);
//	menu::MessageBox::getInstance().setMessage(buffer);
}

void Func_ToggleButtonL()
{
	controller_config_t* currentConfig = virtualControllers[activePad].config;
	int currentButton = currentConfig->L->index;
	currentButton = (currentButton+1) %virtualControllers[activePad].control->num_buttons;
	currentConfig->L = &virtualControllers[activePad].control->buttons[currentButton];
	strcpy(FRAME_STRINGS[5], currentConfig->L->name);
}

void Func_ToggleButtonR()
{
	controller_config_t* currentConfig = virtualControllers[activePad].config;
	int currentButton = currentConfig->R->index;
	currentButton = (currentButton+1) %virtualControllers[activePad].control->num_buttons;
	currentConfig->R = &virtualControllers[activePad].control->buttons[currentButton];
	strcpy(FRAME_STRINGS[6], currentConfig->R->name);
}

void Func_ToggleButtonStart()
{
	controller_config_t* currentConfig = virtualControllers[activePad].config;
	int currentButton = currentConfig->START->index;
	currentButton = (currentButton+1) %virtualControllers[activePad].control->num_buttons;
	currentConfig->START = &virtualControllers[activePad].control->buttons[currentButton];
	strcpy(FRAME_STRINGS[7], currentConfig->START->name);
}

void Func_ToggleButtonDup()
{
	controller_config_t* currentConfig = virtualControllers[activePad].config;
	int currentButton = currentConfig->DU->index;
	currentButton = (currentButton+1) %virtualControllers[activePad].control->num_buttons;
	currentConfig->DU = &virtualControllers[activePad].control->buttons[currentButton];
	strcpy(FRAME_STRINGS[8], currentConfig->DU->name);
}

void Func_ToggleButtonDleft()
{
	controller_config_t* currentConfig = virtualControllers[activePad].config;
	int currentButton = currentConfig->DL->index;
	currentButton = (currentButton+1) %virtualControllers[activePad].control->num_buttons;
	currentConfig->DL = &virtualControllers[activePad].control->buttons[currentButton];
	strcpy(FRAME_STRINGS[9], currentConfig->DL->name);
}

void Func_ToggleButtonDright()
{
	controller_config_t* currentConfig = virtualControllers[activePad].config;
	int currentButton = currentConfig->DR->index;
	currentButton = (currentButton+1) %virtualControllers[activePad].control->num_buttons;
	currentConfig->DR = &virtualControllers[activePad].control->buttons[currentButton];
	strcpy(FRAME_STRINGS[10], currentConfig->DR->name);
}

void Func_ToggleButtonDdown()
{
	controller_config_t* currentConfig = virtualControllers[activePad].config;
	int currentButton = currentConfig->DD->index;
	currentButton = (currentButton+1) %virtualControllers[activePad].control->num_buttons;
	currentConfig->DD = &virtualControllers[activePad].control->buttons[currentButton];
	strcpy(FRAME_STRINGS[11], currentConfig->DD->name);
}

void Func_ToggleButtonCup()
{
	controller_config_t* currentConfig = virtualControllers[activePad].config;
	int currentButton = currentConfig->CU->index;
	currentButton = (currentButton+1) %virtualControllers[activePad].control->num_buttons;
	currentConfig->CU = &virtualControllers[activePad].control->buttons[currentButton];
	strcpy(FRAME_STRINGS[12], currentConfig->CU->name);
}

void Func_ToggleButtonCleft()
{
	controller_config_t* currentConfig = virtualControllers[activePad].config;
	int currentButton = currentConfig->CL->index;
	currentButton = (currentButton+1) %virtualControllers[activePad].control->num_buttons;
	currentConfig->CL = &virtualControllers[activePad].control->buttons[currentButton];
	strcpy(FRAME_STRINGS[13], currentConfig->CL->name);
}

void Func_ToggleButtonCright()
{
	controller_config_t* currentConfig = virtualControllers[activePad].config;
	int currentButton = currentConfig->CR->index;
	currentButton = (currentButton+1) %virtualControllers[activePad].control->num_buttons;
	currentConfig->CR = &virtualControllers[activePad].control->buttons[currentButton];
	strcpy(FRAME_STRINGS[14], currentConfig->CR->name);
}

void Func_ToggleButtonCdown()
{
	controller_config_t* currentConfig = virtualControllers[activePad].config;
	int currentButton = currentConfig->CD->index;
	currentButton = (currentButton+1) %virtualControllers[activePad].control->num_buttons;
	currentConfig->CD = &virtualControllers[activePad].control->buttons[currentButton];
	strcpy(FRAME_STRINGS[15], currentConfig->CD->name);
}

void Func_ToggleButtonZ()
{
	controller_config_t* currentConfig = virtualControllers[activePad].config;
	int currentButton = currentConfig->Z->index;
	currentButton = (currentButton+1) %virtualControllers[activePad].control->num_buttons;
	currentConfig->Z = &virtualControllers[activePad].control->buttons[currentButton];
	strcpy(FRAME_STRINGS[16], currentConfig->Z->name);
}

void Func_ToggleButtonB()
{
	controller_config_t* currentConfig = virtualControllers[activePad].config;
	int currentButton = currentConfig->B->index;
	currentButton = (currentButton+1) %virtualControllers[activePad].control->num_buttons;
	currentConfig->B = &virtualControllers[activePad].control->buttons[currentButton];
	strcpy(FRAME_STRINGS[17], currentConfig->B->name);
}

void Func_ToggleButtonA()
{
	controller_config_t* currentConfig = virtualControllers[activePad].config;
	int currentButton = currentConfig->A->index;
	currentButton = (currentButton+1) %virtualControllers[activePad].control->num_buttons;
	currentConfig->A = &virtualControllers[activePad].control->buttons[currentButton];
	strcpy(FRAME_STRINGS[18], currentConfig->A->name);
}

void Func_ToggleAnalogStick()
{
	controller_config_t* currentConfig = virtualControllers[activePad].config;
	int currentButton = currentConfig->analog->index;
	currentButton = (currentButton+1) %virtualControllers[activePad].control->num_analog_sources;
	currentConfig->analog = &virtualControllers[activePad].control->analog_sources[currentButton];
	strcpy(FRAME_STRINGS[19], currentConfig->analog->name);
}

void Func_ToggleInvertY()
{
	int invertedY = virtualControllers[activePad].config->invertedY;
	if (invertedY)
	{
		virtualControllers[activePad].config->invertedY = 0;
		strcpy(FRAME_STRINGS[20], "Normal Y");
	}
	else
	{
		virtualControllers[activePad].config->invertedY = 1;
		strcpy(FRAME_STRINGS[20], "Inverted Y");
	}
}

void Func_ToggleButtonExit()
{
	controller_config_t* currentConfig = virtualControllers[activePad].config;
	int currentButton = currentConfig->exit->index;
	currentButton = (currentButton+1) %virtualControllers[activePad].control->num_menu_combos;
	currentConfig->exit = &virtualControllers[activePad].control->menu_combos[currentButton];
	strcpy(FRAME_STRINGS[21], currentConfig->exit->name);
}

void Func_ReturnFromConfigureButtonsFrame()
{
	menu::Gui::getInstance().menuLogo->setVisible(true);
	pMenuContext->setActiveFrame(MenuContext::FRAME_SETTINGS,SettingsFrame::SUBMENU_INPUT);
}
