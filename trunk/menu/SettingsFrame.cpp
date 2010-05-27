/**
 * Wii64 - SettingsFrame.cpp
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
#include "SettingsFrame.h"
#include "ConfigurePaksFrame.h"
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
#include "../fileBrowser/fileBrowser.h"
#include "../fileBrowser/fileBrowser-libfat.h"
#include "../fileBrowser/fileBrowser-CARD.h"
}

void Func_TabGeneral();
void Func_TabVideo();
void Func_TabInput();
void Func_TabAudio();
void Func_TabSaves();

void Func_NativeSaveSD();
void Func_NativeSaveUSB();
void Func_NativeSaveCardA();
void Func_NativeSaveCardB();
void Func_SaveStateSD();
void Func_SaveStateUSB();
void Func_CpuPureInterp();
void Func_CpuDynarec();
void Func_SaveSettingsSD();
void Func_SaveSettingsUSB();

void Func_ShowFpsOn();
void Func_ShowFpsOff();
void Func_ScreenMode4_3();
void Func_ScreenMode16_9();
void Func_ScreenForce16_9();
void Func_CpuFramebufferOn();
void Func_CpuFramebufferOff();
void Func_2xSaiTexturesOn();
void Func_2xSaiTexturesOff();
void Func_FbTexturesOn();
void Func_FbTexturesOff();

void Func_ConfigureInput();
void Func_ConfigurePaks();
void Func_ConfigureButtons();
void Func_SaveButtonsSD();
void Func_SaveButtonsUSB();
void Func_ToggleButtonLoad();

void Func_DisableAudioYes();
void Func_DisableAudioNo();

void Func_AutoSaveNativeYes();
void Func_AutoSaveNativeNo();
void Func_CopySaves();
void Func_DeleteSaves();
void Func_ReturnFromSettingsFrame();


#define NUM_FRAME_BUTTONS 38
#define NUM_TAB_BUTTONS 5
#define FRAME_BUTTONS settingsFrameButtons
#define FRAME_STRINGS settingsFrameStrings
#define NUM_FRAME_TEXTBOXES 13
#define FRAME_TEXTBOXES settingsFrameTextBoxes

static char FRAME_STRINGS[37][23] =
	{ "General",
	  "Video",
	  "Input",
	  "Audio",
	  "Saves",
	//Strings for General tab [5]
	  "Native Saves Device",
	  "Save States Device",
	  "Select CPU Core",
	  "Save settings.cfg",
	  "SD",
	  "USB",
	  "CardA",
	  "CardB",
	  "Pure Interp",
	  "Dynarec",
	//Strings for Video tab [15]
	  "Show FPS",
	  "Screen Mode",
	  "CPU Framebuffer",
	  "2xSaI Tex",
	  "FB Textures",
	  "On",
	  "Off",
	  "4:3",
	  "16:9",
	  "Force 16:9", //[24]
	//Strings for Input tab [25]
	  "Configure Input",
	  "Configure Paks",
	  "Configure Buttons",
	  "Save Button Configs",
	  "Auto Load Slot:",
	  "Default",
	//Strings for Audio tab [31]
	  "Disable Audio",
	  "Yes",
	  "No",
	//Strings for Saves tab [34]
	  "Auto Save Native Saves",
	  "Copy Saves",
	  "Delete Saves"};

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
	//Buttons for Tabs (starts at button[0])
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[0],	 25.0,	 30.0,	110.0,	56.0,	-1,	-1,	 4,	 1,	Func_TabGeneral,		Func_ReturnFromSettingsFrame }, // General tab
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[1],	155.0,	 30.0,	100.0,	56.0,	-1,	-1,	 0,	 2,	Func_TabVideo,			Func_ReturnFromSettingsFrame }, // Video tab
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[2],	275.0,	 30.0,	100.0,	56.0,	-1,	-1,	 1,	 3,	Func_TabInput,			Func_ReturnFromSettingsFrame }, // Input tab
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[3],	395.0,	 30.0,	100.0,	56.0,	-1,	-1,	 2,	 4,	Func_TabAudio,			Func_ReturnFromSettingsFrame }, // Audio tab
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[4],	515.0,	 30.0,	100.0,	56.0,	-1,	-1,	 3,	 0,	Func_TabSaves,			Func_ReturnFromSettingsFrame }, // Saves tab
	//Buttons for General Tab (starts at button[5])
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[9],	295.0,	100.0,	 55.0,	56.0,	 0,	 9,	 8,	 6,	Func_NativeSaveSD,		Func_ReturnFromSettingsFrame }, // Native Save: SD
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[10],	360.0,	100.0,	 70.0,	56.0,	 0,	10,	 5,	 7,	Func_NativeSaveUSB,		Func_ReturnFromSettingsFrame }, // Native Save: USB
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[11],	440.0,	100.0,	 90.0,	56.0,	 0,	10,	 6,	 8,	Func_NativeSaveCardA,	Func_ReturnFromSettingsFrame }, // Native Save: Card A
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[12],	540.0,	100.0,	 90.0,	56.0,	 0,	10,	 7,	 5,	Func_NativeSaveCardB,	Func_ReturnFromSettingsFrame }, // Native Save: Card B
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[9],	295.0,	170.0,	 55.0,	56.0,	 5,	11,	10,	10,	Func_SaveStateSD,		Func_ReturnFromSettingsFrame }, // Save State: SD
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[10],	360.0,	170.0,	 70.0,	56.0,	 6,	11,	 9,	 9,	Func_SaveStateUSB,		Func_ReturnFromSettingsFrame }, // Save State: USB
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[13],	295.0,	240.0,	160.0,	56.0,	 9,	13,	12,	12,	Func_CpuPureInterp,		Func_ReturnFromSettingsFrame }, // CPU: Pure Interp
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[14],	465.0,	240.0,	130.0,	56.0,	10,	14,	11,	11,	Func_CpuDynarec,		Func_ReturnFromSettingsFrame }, // CPU: Dynarec
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[9],	295.0,	310.0,	 55.0,	56.0,	11,	 0,	14,	14,	Func_SaveSettingsSD,	Func_ReturnFromSettingsFrame }, // Save Settings: SD
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[10],	360.0,	310.0,	 70.0,	56.0,	11,	 0,	13,	13,	Func_SaveSettingsUSB,	Func_ReturnFromSettingsFrame }, // Save Settings: USB
	//Buttons for Video Tab (starts at button[15])
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[20],	325.0,	100.0,	 75.0,	56.0,	 1,	18,	16,	16,	Func_ShowFpsOn,			Func_ReturnFromSettingsFrame }, // Show FPS: On
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[21],	420.0,	100.0,	 75.0,	56.0,	 1,	19,	15,	15,	Func_ShowFpsOff,		Func_ReturnFromSettingsFrame }, // Show FPS: Off
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[22],	230.0,	170.0,	 75.0,	56.0,	15,	20,	19,	18,	Func_ScreenMode4_3,		Func_ReturnFromSettingsFrame }, // ScreenMode: 4:3
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[23],	325.0,	170.0,	 75.0,	56.0,	15,	20,	17,	19,	Func_ScreenMode16_9,	Func_ReturnFromSettingsFrame }, // ScreenMode: 16:9
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[24],	420.0,	170.0,	155.0,	56.0,	16,	21,	18,	17,	Func_ScreenForce16_9,	Func_ReturnFromSettingsFrame }, // ScreenMode: Force 16:9 in-game
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[20],	325.0,	240.0,	 75.0,	56.0,	18,	22,	21,	21,	Func_CpuFramebufferOn,	Func_ReturnFromSettingsFrame }, // CPU FB: On
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[21],	420.0,	240.0,	 75.0,	56.0,	19,	23,	20,	20,	Func_CpuFramebufferOff,	Func_ReturnFromSettingsFrame }, // CPU FB: Off
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[20],	325.0,	310.0,	 75.0,	56.0,	20,	24,	23,	23,	Func_2xSaiTexturesOn,	Func_ReturnFromSettingsFrame }, // 2xSai: On
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[21],	420.0,	310.0,	 75.0,	56.0,	21,	25,	22,	22,	Func_2xSaiTexturesOff,	Func_ReturnFromSettingsFrame }, // 2xSai: Off
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[20],	325.0,	380.0,	 75.0,	56.0,	22,	 1,	25,	25,	Func_FbTexturesOn,		Func_ReturnFromSettingsFrame }, // FbTex: On
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[21],	420.0,	380.0,	 75.0,	56.0,	23,	 1,	24,	24,	Func_FbTexturesOff,		Func_ReturnFromSettingsFrame }, // FbTex: Off
	//Buttons for Input Tab (starts at button[26])
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[25],	180.0,	100.0,	280.0,	56.0,	 2,	27,	-1,	-1,	Func_ConfigureInput,	Func_ReturnFromSettingsFrame }, // Configure Mappings
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[26],	180.0,	170.0,	280.0,	56.0,	26,	28,	-1,	-1,	Func_ConfigurePaks,		Func_ReturnFromSettingsFrame }, // Configure Paks
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[27],	180.0,	240.0,	280.0,	56.0,	27,	29,	-1,	-1,	Func_ConfigureButtons,	Func_ReturnFromSettingsFrame }, // Configure Buttons
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[9],	295.0,	310.0,	 55.0,	56.0,	28,	31,	30,	30,	Func_SaveButtonsSD,		Func_ReturnFromSettingsFrame }, // Save Button Configs to SD
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[10],	360.0,	310.0,	 70.0,	56.0,	28,	31,	29,	29,	Func_SaveButtonsUSB,	Func_ReturnFromSettingsFrame }, // Save Button Configs to USB
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[30],	295.0,	380.0,	135.0,	56.0,	29,	 2,	-1,	-1,	Func_ToggleButtonLoad,	Func_ReturnFromSettingsFrame }, // Toggle Button Load Slot
	//Buttons for Audio Tab (starts at button[32])
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[32],	345.0,	100.0,	 75.0,	56.0,	 3,	 3,	33,	33,	Func_DisableAudioYes,	Func_ReturnFromSettingsFrame }, // Disable Audio: Yes
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[33],	440.0,	100.0,	 75.0,	56.0,	 3,	 3,	32,	32,	Func_DisableAudioNo,	Func_ReturnFromSettingsFrame }, // Disable Audio: No
	//Buttons for Saves Tab (starts at button[34])
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[32],	375.0,	100.0,	 75.0,	56.0,	 4,	36,	35,	35,	Func_AutoSaveNativeYes,	Func_ReturnFromSettingsFrame }, // Auto Save Native: Yes
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[33],	470.0,	100.0,	 75.0,	56.0,	 4,	36,	34,	34,	Func_AutoSaveNativeNo,	Func_ReturnFromSettingsFrame }, // Auto Save Native: No
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[35],	365.0,	170.0,	190.0,	56.0,	34,	37,	-1,	-1,	Func_CopySaves,			Func_ReturnFromSettingsFrame }, // Copy Saves
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[36],	365.0,	240.0,	190.0,	56.0,	36,	 4,	-1,	-1,	Func_DeleteSaves,		Func_ReturnFromSettingsFrame }, // Delete Saves
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
	//TextBoxes for General Tab (starts at textBox[0])
	{	NULL,	FRAME_STRINGS[5],	155.0,	128.0,	 1.0,	true }, // Native Save Device: SD/USB/CardA/CardB
	{	NULL,	FRAME_STRINGS[6],	155.0,	198.0,	 1.0,	true }, // Save State Device: SD/USB
	{	NULL,	FRAME_STRINGS[7],	155.0,	268.0,	 1.0,	true }, // CPU Core: Pure Interp/Dynarec
	{	NULL,	FRAME_STRINGS[8],	155.0,	338.0,	 1.0,	true }, // Save settings.cfg: SD/USB
	//TextBoxes for Video Tab (starts at textBox[4])
	{	NULL,	FRAME_STRINGS[15],	190.0,	128.0,	 1.0,	true }, // Show FPS: On/Off
	{	NULL,	FRAME_STRINGS[16],	130.0,	198.0,	 1.0,	true }, // ScreenMode: 4x3/16x9/Force16x9
	{	NULL,	FRAME_STRINGS[17],	190.0,	268.0,	 1.0,	true }, // CPU Framebuffer: On/Off
	{	NULL,	FRAME_STRINGS[18],	190.0,	338.0,	 1.0,	true }, // 2xSai: On/Off
	{	NULL,	FRAME_STRINGS[19],	190.0,	408.0,	 1.0,	true }, // FBTex: On/Off
	//TextBoxes for Input Tab (starts at textBox[9])
	{	NULL,	FRAME_STRINGS[28],	155.0,	338.0,	 1.0,	true }, // 2xSai: On/Off
	{	NULL,	FRAME_STRINGS[29],	155.0,	408.0,	 1.0,	true }, // 2xSai: On/Off
	//TextBoxes for Audio Tab (starts at textBox[11])
	{	NULL,	FRAME_STRINGS[31],	210.0,	128.0,	 1.0,	true }, // Disable Audio: Yes/No
	//TextBoxes for Saves Tab (starts at textBox[12])
	{	NULL,	FRAME_STRINGS[34],	200.0,	128.0,	 1.0,	true }, // Auto Save Native Save: Yes/No
};

SettingsFrame::SettingsFrame()
		: activeSubmenu(SUBMENU_GENERAL)
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
	setBackFunc(Func_ReturnFromSettingsFrame);
	setEnabled(true);
	activateSubmenu(SUBMENU_GENERAL);
}

SettingsFrame::~SettingsFrame()
{
	for (int i = 0; i < NUM_FRAME_TEXTBOXES; i++)
		delete FRAME_TEXTBOXES[i].textBox;
	for (int i = 0; i < NUM_FRAME_BUTTONS; i++)
	{
		menu::Cursor::getInstance().removeComponent(this, FRAME_BUTTONS[i].button);
		delete FRAME_BUTTONS[i].button;
	}

}

void SettingsFrame::activateSubmenu(int submenu)
{
	activeSubmenu = submenu;

	//All buttons: hide; unselect
	for (int i = 0; i < NUM_FRAME_BUTTONS; i++)
	{
		FRAME_BUTTONS[i].button->setVisible(false);
		FRAME_BUTTONS[i].button->setSelected(false);
		FRAME_BUTTONS[i].button->setActive(false);
	}
	//All textBoxes: hide
	for (int i = 0; i < NUM_FRAME_TEXTBOXES; i++)
	{
		FRAME_TEXTBOXES[i].textBox->setVisible(false);
	}
	switch (activeSubmenu)	//Tab buttons: set visible; set focus up/down; set selected
	{						//Config buttons: set visible; set selected
		case SUBMENU_GENERAL:
			setDefaultFocus(FRAME_BUTTONS[0].button);
			for (int i = 0; i < NUM_TAB_BUTTONS; i++)
			{
				FRAME_BUTTONS[i].button->setVisible(true);
				FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_DOWN, FRAME_BUTTONS[5].button);
				FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_UP, FRAME_BUTTONS[13].button);
				FRAME_BUTTONS[i].button->setActive(true);
			}
			for (int i = 0; i < 4; i++)
				FRAME_TEXTBOXES[i].textBox->setVisible(true);
			FRAME_BUTTONS[0].button->setSelected(true);
			FRAME_BUTTONS[5+nativeSaveDevice].button->setSelected(true);
			FRAME_BUTTONS[9+saveStateDevice].button->setSelected(true);
			if (dynacore == DYNACORE_PURE_INTERP)	FRAME_BUTTONS[11].button->setSelected(true);
			else									FRAME_BUTTONS[12].button->setSelected(true);
			for (int i = 5; i < 15; i++)
			{
				FRAME_BUTTONS[i].button->setVisible(true);
				FRAME_BUTTONS[i].button->setActive(true);
			}
			break;
		case SUBMENU_VIDEO:
			setDefaultFocus(FRAME_BUTTONS[1].button);
			for (int i = 0; i < NUM_TAB_BUTTONS; i++)
			{
				FRAME_BUTTONS[i].button->setVisible(true);
				FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_DOWN, FRAME_BUTTONS[15].button);
				FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_UP, FRAME_BUTTONS[24].button);
				FRAME_BUTTONS[i].button->setActive(true);
			}
			for (int i = 4; i < 9; i++)
				FRAME_TEXTBOXES[i].textBox->setVisible(true);
			FRAME_BUTTONS[1].button->setSelected(true);
			if (showFPSonScreen == FPS_SHOW)	FRAME_BUTTONS[15].button->setSelected(true);
			else								FRAME_BUTTONS[16].button->setSelected(true);
			if (screenMode == SCREENMODE_4x3)		FRAME_BUTTONS[17].button->setSelected(true);
			else if (screenMode == SCREENMODE_16x9)	FRAME_BUTTONS[18].button->setSelected(true);
			else									FRAME_BUTTONS[19].button->setSelected(true);
			if (renderCpuFramebuffer == CPUFRAMEBUFFER_ENABLE)	FRAME_BUTTONS[20].button->setSelected(true);
			else												FRAME_BUTTONS[21].button->setSelected(true);
			if (glN64_use2xSaiTextures == GLN64_2XSAI_ENABLE)	FRAME_BUTTONS[22].button->setSelected(true);
			else												FRAME_BUTTONS[23].button->setSelected(true);
			if (glN64_useFrameBufferTextures == GLN64_FBTEX_ENABLE)	FRAME_BUTTONS[24].button->setSelected(true);
			else													FRAME_BUTTONS[25].button->setSelected(true);
			for (int i = 15; i < 26; i++)
			{
				FRAME_BUTTONS[i].button->setVisible(true);
				FRAME_BUTTONS[i].button->setActive(true);
			}
			break;
		case SUBMENU_INPUT:
			setDefaultFocus(FRAME_BUTTONS[2].button);
			for (int i = 0; i < NUM_TAB_BUTTONS; i++)
			{
				FRAME_BUTTONS[i].button->setVisible(true);
				FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_DOWN, FRAME_BUTTONS[26].button);
				FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_UP, FRAME_BUTTONS[31].button);
				FRAME_BUTTONS[i].button->setActive(true);
			}
			for (int i = 9; i < 11; i++)
				FRAME_TEXTBOXES[i].textBox->setVisible(true);
			FRAME_BUTTONS[2].button->setSelected(true);
			if (loadButtonSlot == LOADBUTTON_DEFAULT)	strcpy(FRAME_STRINGS[30], "Default");
			else										sprintf(FRAME_STRINGS[30], "Slot %d", loadButtonSlot+1);
			for (int i = 26; i < 32; i++)
			{
				FRAME_BUTTONS[i].button->setVisible(true);
				FRAME_BUTTONS[i].button->setActive(true);
			}
			break;
		case SUBMENU_AUDIO:
			setDefaultFocus(FRAME_BUTTONS[3].button);
			for (int i = 0; i < NUM_TAB_BUTTONS; i++)
			{
				FRAME_BUTTONS[i].button->setVisible(true);
				FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_DOWN, FRAME_BUTTONS[32].button);
				FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_UP, FRAME_BUTTONS[32].button);
				FRAME_BUTTONS[i].button->setActive(true);
			}
			for (int i = 11; i < 12; i++)
				FRAME_TEXTBOXES[i].textBox->setVisible(true);
			FRAME_BUTTONS[3].button->setSelected(true);
			if (audioEnabled == AUDIO_DISABLE)	FRAME_BUTTONS[32].button->setSelected(true);
			else								FRAME_BUTTONS[33].button->setSelected(true);
			for (int i = 32; i < 34; i++)
			{
				FRAME_BUTTONS[i].button->setVisible(true);
				FRAME_BUTTONS[i].button->setActive(true);
			}
			break;
		case SUBMENU_SAVES:
			setDefaultFocus(FRAME_BUTTONS[4].button);
			for (int i = 0; i < NUM_TAB_BUTTONS; i++)
			{
				FRAME_BUTTONS[i].button->setVisible(true);
				FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_DOWN, FRAME_BUTTONS[34].button);
				FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_UP, FRAME_BUTTONS[37].button);
				FRAME_BUTTONS[i].button->setActive(true);
			}
			for (int i = 12; i < 13; i++)
				FRAME_TEXTBOXES[i].textBox->setVisible(true);
			FRAME_BUTTONS[4].button->setSelected(true);
			if (autoSave == AUTOSAVE_ENABLE)	FRAME_BUTTONS[34].button->setSelected(true);
			else								FRAME_BUTTONS[35].button->setSelected(true);
			for (int i = 34; i < NUM_FRAME_BUTTONS; i++)
			{
				FRAME_BUTTONS[i].button->setVisible(true);
				FRAME_BUTTONS[i].button->setActive(true);
			}
			break;
	}
}

void SettingsFrame::drawChildren(menu::Graphics &gfx)
{
	if(isVisible())
	{
#ifdef HW_RVL
		WPADData* wiiPad = menu::Input::getInstance().getWpad();
#endif
		for (int i=0; i<4; i++)
		{
			u16 currentButtonsGC = PAD_ButtonsHeld(i);
			if (currentButtonsGC ^ previousButtonsGC[i])
			{
				u16 currentButtonsDownGC = (currentButtonsGC ^ previousButtonsGC[i]) & currentButtonsGC;
				previousButtonsGC[i] = currentButtonsGC;
				if (currentButtonsDownGC & PAD_TRIGGER_R)
				{
					//move to next tab
					if(activeSubmenu < SUBMENU_SAVES) 
					{
						activateSubmenu(activeSubmenu+1);
						menu::Focus::getInstance().clearPrimaryFocus();
					}
					break;
				}
				else if (currentButtonsDownGC & PAD_TRIGGER_L)
				{
					//move to the previous tab
					if(activeSubmenu > SUBMENU_GENERAL) 
					{
						activateSubmenu(activeSubmenu-1);
						menu::Focus::getInstance().clearPrimaryFocus();
					}
					break;
				}
			}
#ifdef HW_RVL
			else if (wiiPad[i].btns_h ^ previousButtonsWii[i])
			{
				u32 currentButtonsDownWii = (wiiPad[i].btns_h ^ previousButtonsWii[i]) & wiiPad[i].btns_h;
				previousButtonsWii[i] = wiiPad[i].btns_h;
				if (wiiPad[i].exp.type == WPAD_EXP_CLASSIC)
				{
					if (currentButtonsDownWii & WPAD_CLASSIC_BUTTON_FULL_R)
					{
						//move to next tab
						if(activeSubmenu < SUBMENU_SAVES) 
						{
							activateSubmenu(activeSubmenu+1);
							menu::Focus::getInstance().clearPrimaryFocus();
						}
						break;
					}
					else if (currentButtonsDownWii & WPAD_CLASSIC_BUTTON_FULL_L)
					{
						//move to the previous tab
						if(activeSubmenu > SUBMENU_GENERAL) 
						{
							activateSubmenu(activeSubmenu-1);
							menu::Focus::getInstance().clearPrimaryFocus();
						}
						break;
					}
				}
				else
				{
					if (currentButtonsDownWii & WPAD_BUTTON_PLUS)
					{
						//move to next tab
						if(activeSubmenu < SUBMENU_SAVES) 
						{
							activateSubmenu(activeSubmenu+1);
							menu::Focus::getInstance().clearPrimaryFocus();
						}
						break;
					}
					else if (currentButtonsDownWii & WPAD_BUTTON_MINUS)
					{
						//move to the previous tab
						if(activeSubmenu > SUBMENU_GENERAL) 
						{
							activateSubmenu(activeSubmenu-1);
							menu::Focus::getInstance().clearPrimaryFocus();
						}
						break;
					}
				}
			}
#endif //HW_RVL
		}

		//Draw buttons
		menu::ComponentList::const_iterator iteration;
		for (iteration = componentList.begin(); iteration != componentList.end(); iteration++)
		{
			(*iteration)->draw(gfx);
		}
	}
}

extern MenuContext *pMenuContext;

void Func_TabGeneral()
{
	pMenuContext->setActiveFrame(MenuContext::FRAME_SETTINGS,SettingsFrame::SUBMENU_GENERAL);
}

void Func_TabVideo()
{
	pMenuContext->setActiveFrame(MenuContext::FRAME_SETTINGS,SettingsFrame::SUBMENU_VIDEO);
}

void Func_TabInput()
{
	pMenuContext->setActiveFrame(MenuContext::FRAME_SETTINGS,SettingsFrame::SUBMENU_INPUT);
}

void Func_TabAudio()
{
	pMenuContext->setActiveFrame(MenuContext::FRAME_SETTINGS,SettingsFrame::SUBMENU_AUDIO);
}

void Func_TabSaves()
{
	pMenuContext->setActiveFrame(MenuContext::FRAME_SETTINGS,SettingsFrame::SUBMENU_SAVES);
}

void Func_NativeSaveSD()
{
	for (int i = 5; i <= 8; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[5].button->setSelected(true);
	nativeSaveDevice = NATIVESAVEDEVICE_SD;
}

void Func_NativeSaveUSB()
{
	for (int i = 5; i <= 8; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[6].button->setSelected(true);
	nativeSaveDevice = NATIVESAVEDEVICE_USB;
}

void Func_NativeSaveCardA()
{
	for (int i = 5; i <= 8; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[7].button->setSelected(true);
	nativeSaveDevice = NATIVESAVEDEVICE_CARDA;
}

void Func_NativeSaveCardB()
{
	for (int i = 5; i <= 8; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[8].button->setSelected(true);
	nativeSaveDevice = NATIVESAVEDEVICE_CARDB;
}

void Func_SaveStateSD()
{
	for (int i = 9; i <= 10; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[9].button->setSelected(true);
	saveStateDevice = SAVESTATEDEVICE_SD;
}

void Func_SaveStateUSB()
{
	for (int i = 9; i <= 10; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[10].button->setSelected(true);
	saveStateDevice = SAVESTATEDEVICE_USB;
}

extern "C" {
void cpu_init();
void cpu_deinit();
}
extern BOOL hasLoadedROM;

void Func_CpuPureInterp()
{
	for (int i = 11; i <= 12; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[11].button->setSelected(true);

	int needInit = 0;
	if(hasLoadedROM && dynacore != DYNACORE_PURE_INTERP){ cpu_deinit(); needInit = 1; }
	dynacore = DYNACORE_PURE_INTERP;
	if(hasLoadedROM && needInit) cpu_init();
}

void Func_CpuDynarec()
{
	for (int i = 11; i <= 12; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[12].button->setSelected(true);

	int needInit = 0;
	if(hasLoadedROM && dynacore != DYNACORE_DYNAREC){ cpu_deinit(); needInit = 1; }
	dynacore = DYNACORE_DYNAREC;
	if(hasLoadedROM && needInit) cpu_init();
}

extern void writeConfig(FILE* f);

void Func_SaveSettingsSD()
{
	fileBrowser_file* configFile_file;
	int (*configFile_init)(fileBrowser_file*) = fileBrowser_libfat_init;
	int num_written = 0;
	configFile_file = &saveDir_libfat_Default;
	if(configFile_init(configFile_file)) {                //only if device initialized ok
		FILE* f = fopen( "sd:/wii64/settings.cfg", "wb" );  //attempt to open file
		if(f) {
			writeConfig(f);                                   //write out the config
			fclose(f);
			num_written++;
		}
	}
	if (num_written == 1)
		menu::MessageBox::getInstance().setMessage("Saved settings.cfg to SD");
	else
		menu::MessageBox::getInstance().setMessage("Error saving settings.cfg to SD");
}

void Func_SaveSettingsUSB()
{
	fileBrowser_file* configFile_file;
	int (*configFile_init)(fileBrowser_file*) = fileBrowser_libfat_init;
	int num_written = 0;
	configFile_file = &saveDir_libfat_USB;
	if(configFile_init(configFile_file)) {                //only if device initialized ok
		FILE* f = fopen( "usb:/wii64/settings.cfg", "wb" ); //attempt to open file
		if(f) {
			writeConfig(f);                                   //write out the config
			fclose(f);
			num_written++;
		}
	}
	if (num_written == 1)
		menu::MessageBox::getInstance().setMessage("Saved settings.cfg to USB");
	else
		menu::MessageBox::getInstance().setMessage("Error saving settings.cfg to USB");
}

void Func_ShowFpsOn()
{
	for (int i = 15; i <= 16; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[15].button->setSelected(true);
	showFPSonScreen = FPS_SHOW;
}

void Func_ShowFpsOff()
{
	for (int i = 15; i <= 16; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[16].button->setSelected(true);
	showFPSonScreen = FPS_HIDE;
}

void Func_ShowDebugOn()
{
	for (int i = 17; i <= 18; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[17].button->setSelected(true);
	printToScreen = DEBUG_SHOW;
}

void Func_ShowDebugOff()
{
	for (int i = 17; i <= 18; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[18].button->setSelected(true);
	printToScreen = DEBUG_HIDE;
}

void gfx_set_window(int x, int y, int width, int height);

void Func_ScreenMode4_3()
{
	for (int i = 17; i <= 19; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[17].button->setSelected(true);
	screenMode = SCREENMODE_4x3;
	gfx_set_window( 0, 0, 640, 480);
}

void Func_ScreenMode16_9()
{
	for (int i = 17; i <= 19; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[18].button->setSelected(true);
	screenMode = SCREENMODE_16x9;
	gfx_set_window( 0, 0, 640, 480);
}

void Func_ScreenForce16_9()
{
	for (int i = 17; i <= 19; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[19].button->setSelected(true);
	screenMode = SCREENMODE_16x9_PILLARBOX;
	gfx_set_window( 78, 0, 483, 480);
}

void Func_CpuFramebufferOn()
{
	for (int i = 20; i <= 21; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[20].button->setSelected(true);
	renderCpuFramebuffer = CPUFRAMEBUFFER_ENABLE;
}

void Func_CpuFramebufferOff()
{
	for (int i = 20; i <= 21; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[21].button->setSelected(true);
	renderCpuFramebuffer = CPUFRAMEBUFFER_DISABLE;
}

void Func_2xSaiTexturesOn()
{
	for (int i = 22; i <= 23; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[22].button->setSelected(true);
	glN64_use2xSaiTextures = GLN64_2XSAI_ENABLE;
}

void Func_2xSaiTexturesOff()
{
	for (int i = 22; i <= 23; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[23].button->setSelected(true);
	glN64_use2xSaiTextures = GLN64_2XSAI_DISABLE;
}

void Func_FbTexturesOn()
{
	for (int i = 24; i <= 25; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[24].button->setSelected(true);
	glN64_useFrameBufferTextures = GLN64_FBTEX_ENABLE;
}

void Func_FbTexturesOff()
{
	for (int i = 24; i <= 25; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[25].button->setSelected(true);
	glN64_useFrameBufferTextures = GLN64_FBTEX_DISABLE;
}

void Func_ConfigureInput()
{
//	menu::MessageBox::getInstance().setMessage("Input configuration not implemented");
	pMenuContext->setActiveFrame(MenuContext::FRAME_CONFIGUREINPUT,ConfigureInputFrame::SUBMENU_REINIT);
}

void Func_ConfigurePaks()
{
//	menu::MessageBox::getInstance().setMessage("Controller Paks not implemented");
	pMenuContext->setActiveFrame(MenuContext::FRAME_CONFIGUREPAKS,ConfigurePaksFrame::SUBMENU_REINIT);
}

void Func_ConfigureButtons()
{
//	menu::MessageBox::getInstance().setMessage("Controller Buttons not implemented");
	pMenuContext->setActiveFrame(MenuContext::FRAME_CONFIGUREBUTTONS,ConfigureButtonsFrame::SUBMENU_N64_PADNONE);
}

void Func_SaveButtonsSD()
{
	fileBrowser_file* configFile_file;
	int (*configFile_init)(fileBrowser_file*) = fileBrowser_libfat_init;
	int num_written = 0;
	configFile_file = &saveDir_libfat_Default;
	if(configFile_init(configFile_file)) {                //only if device initialized ok
		FILE* f = fopen( "sd:/wii64/controlG.cfg", "wb" );  //attempt to open file
		if(f) {
			save_configurations(f, &controller_GC);					//write out GC controller mappings
			fclose(f);
			num_written++;
		}
#ifdef HW_RVL
		f = fopen( "sd:/wii64/controlC.cfg", "wb" );  //attempt to open file
		if(f) {
			save_configurations(f, &controller_Classic);			//write out Classic controller mappings
			fclose(f);
			num_written++;
		}
		f = fopen( "sd:/wii64/controlN.cfg", "wb" );  //attempt to open file
		if(f) {
			save_configurations(f, &controller_WiimoteNunchuk);	//write out WM+NC controller mappings
			fclose(f);
			num_written++;
		}
		f = fopen( "sd:/wii64/controlW.cfg", "wb" );  //attempt to open file
		if(f) {
			save_configurations(f, &controller_Wiimote);			//write out Wiimote controller mappings
			fclose(f);
			num_written++;
		}
#endif //HW_RVL
	}
	if (num_written == num_controller_t)
		menu::MessageBox::getInstance().setMessage("Saved Button Configs to SD");
	else
		menu::MessageBox::getInstance().setMessage("Error saving Button Configs to SD");
}

void Func_SaveButtonsUSB()
{
	fileBrowser_file* configFile_file;
	int (*configFile_init)(fileBrowser_file*) = fileBrowser_libfat_init;
	int num_written = 0;
	configFile_file = &saveDir_libfat_USB;
	if(configFile_init(configFile_file)) {                //only if device initialized ok
		FILE* f = fopen( "usb:/wii64/controlG.cfg", "wb" );  //attempt to open file
		if(f) {
			save_configurations(f, &controller_GC);					//write out GC controller mappings
			fclose(f);
			num_written++;
		}
#ifdef HW_RVL
		f = fopen( "usb:/wii64/controlC.cfg", "wb" );  //attempt to open file
		if(f) {
			save_configurations(f, &controller_Classic);			//write out Classic controller mappings
			fclose(f);
			num_written++;
		}
		f = fopen( "usb:/wii64/controlN.cfg", "wb" );  //attempt to open file
		if(f) {
			save_configurations(f, &controller_WiimoteNunchuk);	//write out WM+NC controller mappings
			fclose(f);
			num_written++;
		}
		f = fopen( "usb:/wii64/controlW.cfg", "wb" );  //attempt to open file
		if(f) {
			save_configurations(f, &controller_Wiimote);			//write out Wiimote controller mappings
			fclose(f);
			num_written++;
		}
#endif //HW_RVL
	}
	if (num_written == num_controller_t)
		menu::MessageBox::getInstance().setMessage("Saved Button Configs to USB");
	else
		menu::MessageBox::getInstance().setMessage("Error saving Button Configs to USB");
}

void Func_ToggleButtonLoad()
{
	loadButtonSlot = (loadButtonSlot + 1) % 5;
	if (loadButtonSlot == LOADBUTTON_DEFAULT)
		strcpy(FRAME_STRINGS[30], "Default");
	else
		sprintf(FRAME_STRINGS[30], "Slot %d", loadButtonSlot+1);
}

void Func_DisableAudioYes()
{
	for (int i = 32; i <= 33; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[32].button->setSelected(true);
	audioEnabled = AUDIO_DISABLE;
}

void Func_DisableAudioNo()
{
	for (int i = 32; i <= 33; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[33].button->setSelected(true);
	audioEnabled = AUDIO_ENABLE;
}

void Func_AutoSaveNativeYes()
{
	for (int i = 34; i <= 35; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[34].button->setSelected(true);
	autoSave = AUTOSAVE_ENABLE;
}

void Func_AutoSaveNativeNo()
{
	for (int i = 34; i <= 35; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[35].button->setSelected(true);
	autoSave = AUTOSAVE_DISABLE;
}

void Func_CopySaves()
{
	menu::MessageBox::getInstance().setMessage("Copy Saves not implemented");
}

void Func_DeleteSaves()
{
	menu::MessageBox::getInstance().setMessage("Delete Saves not implemented");
}

void Func_ReturnFromSettingsFrame()
{
	menu::Gui::getInstance().menuLogo->setLocation(580.0, 70.0, -50.0);
	pMenuContext->setActiveFrame(MenuContext::FRAME_MAIN);
}

/*extern timers Timers;

void Func_ToggleVILimit()
{
	Timers.limitVIs = (Timers.limitVIs+1) % 3;
	FRAME_BUTTONS[2].buttonString = FRAME_STRINGS[Timers.limitVIs + 4];
}*/
