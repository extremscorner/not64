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
void Func_NativeSaveFAT();
void Func_NativeSaveCardA();
void Func_NativeSaveCardB();
void Func_SaveStateSD();
void Func_SaveStateFAT();
void Func_CpuPureInterp();
void Func_CpuDynarec();
void Func_ClockDivider1();
void Func_ClockDivider2();
void Func_ClockDivider3();
void Func_SaveSettingsSD();
void Func_SaveSettingsFAT();

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
void Func_SaveButtonsFAT();
void Func_ToggleButtonLoad();

void Func_DisableAudioYes();
void Func_DisableAudioNo();
void Func_SpeedLimitOff();
void Func_SpeedLimitVi();
void Func_SpeedLimitDl();
void Func_ScalePitchYes();
void Func_ScalePitchNo();

void Func_AutoLoadSaveYes();
void Func_AutoLoadSaveNo();
void Func_AutoSaveYes();
void Func_AutoSaveNo();
void Func_CopySaves();
void Func_DeleteSaves();
void Func_ReturnFromSettingsFrame();


#define NUM_FRAME_BUTTONS 48
#define NUM_TAB_BUTTONS 5
#define FRAME_BUTTONS settingsFrameButtons
#define FRAME_STRINGS settingsFrameStrings
#define NUM_FRAME_TEXTBOXES 17
#define FRAME_TEXTBOXES settingsFrameTextBoxes

static char FRAME_STRINGS[46][23] =
	{ "General",
	  "Video",
	  "Input",
	  "Audio",
	  "Saves",
	//Strings for General tab [5]
	  "Native Saves Device",
	  "Save States Device",
	  "CPU Emulator",
	  "CPU Clock Divider",
	  "Save Settings",
	  "SD",
	  "FAT",
	  "CardA",
	  "CardB",
	  "Interpreter",
	  "Recompiler",
	  "1",
	  "2",
	  "3",
	//Strings for Video tab [19]
	  "Show FPS",
	  "Screen Mode",
	  "CPU Framebuffer",
	  "2xSaI Textures",
	  "FB Textures",
	  "On",
	  "Off",
	  "4:3",
	  "16:9",
	  "Force 16:9",
	//Strings for Input tab [29]
	  "Configure Input",
	  "Configure Paks",
	  "Configure Buttons",
	  "Save Configuration",
	  "Auto Load Slot",
	  "Default",
	//Strings for Audio tab [35]
	  "Disable Audio",
	  "Speed Limit",
	  "Scale Pitch",
	  "Yes",
	  "No",
	  "VI",
	  "DL",
	//Strings for Saves tab [42]
	  "Auto Load Native Saves",
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
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[10],	295.0,	100.0,	 55.0,	56.0,	 0,	 9,	 8,	 6,	Func_NativeSaveSD,		Func_ReturnFromSettingsFrame }, // Native Save: SD
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[11],	360.0,	100.0,	 70.0,	56.0,	 0,	10,	 5,	 7,	Func_NativeSaveFAT,		Func_ReturnFromSettingsFrame }, // Native Save: FAT
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[12],	440.0,	100.0,	 90.0,	56.0,	 0,	10,	 6,	 8,	Func_NativeSaveCardA,	Func_ReturnFromSettingsFrame }, // Native Save: Card A
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[13],	540.0,	100.0,	 90.0,	56.0,	 0,	10,	 7,	 5,	Func_NativeSaveCardB,	Func_ReturnFromSettingsFrame }, // Native Save: Card B
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[10],	295.0,	170.0,	 55.0,	56.0,	 5,	11,	10,	10,	Func_SaveStateSD,		Func_ReturnFromSettingsFrame }, // Save State: SD
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[11],	360.0,	170.0,	 70.0,	56.0,	 6,	11,	 9,	 9,	Func_SaveStateFAT,		Func_ReturnFromSettingsFrame }, // Save State: FAT
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[14],	295.0,	240.0,	160.0,	56.0,	 9,	13,	12,	12,	Func_CpuPureInterp,		Func_ReturnFromSettingsFrame }, // CPU Emulator: Interpreter
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[15],	465.0,	240.0,	160.0,	56.0,	10,	15,	11,	11,	Func_CpuDynarec,		Func_ReturnFromSettingsFrame }, // CPU Emulator: Recompiler
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[16],	295.0,	310.0,	 55.0,	56.0,	11,	16,	15,	14,	Func_ClockDivider1,		Func_ReturnFromSettingsFrame }, // CPU Clock Divider: 1
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[17],	360.0,	310.0,	 55.0,	56.0,	11,	17,	13,	15,	Func_ClockDivider2,		Func_ReturnFromSettingsFrame }, // CPU Clock Divider: 2
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[18],	425.0,	310.0,	 55.0,	56.0,	11,	17,	14,	13,	Func_ClockDivider3,		Func_ReturnFromSettingsFrame }, // CPU Clock Divider: 3
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[10],	295.0,	380.0,	 55.0,	56.0,	13,	 0,	17,	17,	Func_SaveSettingsSD,	Func_ReturnFromSettingsFrame }, // Save Settings: SD
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[11],	360.0,	380.0,	 70.0,	56.0,	14,	 0,	16,	16,	Func_SaveSettingsFAT,	Func_ReturnFromSettingsFrame }, // Save Settings: FAT
	//Buttons for Video Tab (starts at button[18])
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[24],	325.0,	100.0,	 75.0,	56.0,	 1,	21,	19,	19,	Func_ShowFpsOn,			Func_ReturnFromSettingsFrame }, // Show FPS: On
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[25],	420.0,	100.0,	 75.0,	56.0,	 1,	22,	18,	18,	Func_ShowFpsOff,		Func_ReturnFromSettingsFrame }, // Show FPS: Off
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[26],	230.0,	170.0,	 75.0,	56.0,	18,	23,	22,	21,	Func_ScreenMode4_3,		Func_ReturnFromSettingsFrame }, // Screen Mode: 4:3
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[27],	325.0,	170.0,	 75.0,	56.0,	18,	23,	20,	22,	Func_ScreenMode16_9,	Func_ReturnFromSettingsFrame }, // Screen Mode: 16:9
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[28],	420.0,	170.0,	155.0,	56.0,	19,	24,	21,	20,	Func_ScreenForce16_9,	Func_ReturnFromSettingsFrame }, // Screen Mode: Force 16:9
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[24],	325.0,	240.0,	 75.0,	56.0,	21,	25,	24,	24,	Func_CpuFramebufferOn,	Func_ReturnFromSettingsFrame }, // CPU Framebuffer: On
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[25],	420.0,	240.0,	 75.0,	56.0,	22,	26,	23,	23,	Func_CpuFramebufferOff,	Func_ReturnFromSettingsFrame }, // CPU Framebuffer: Off
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[24],	325.0,	310.0,	 75.0,	56.0,	23,	27,	26,	26,	Func_2xSaiTexturesOn,	Func_ReturnFromSettingsFrame }, // 2xSaI Textures: On
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[25],	420.0,	310.0,	 75.0,	56.0,	24,	28,	25,	25,	Func_2xSaiTexturesOff,	Func_ReturnFromSettingsFrame }, // 2xSaI Textures: Off
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[24],	325.0,	380.0,	 75.0,	56.0,	25,	 1,	28,	28,	Func_FbTexturesOn,		Func_ReturnFromSettingsFrame }, // FB Textures: On
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[25],	420.0,	380.0,	 75.0,	56.0,	26,	 1,	27,	27,	Func_FbTexturesOff,		Func_ReturnFromSettingsFrame }, // FB Textures: Off
	//Buttons for Input Tab (starts at button[29])
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[29],	180.0,	100.0,	280.0,	56.0,	 2,	30,	-1,	-1,	Func_ConfigureInput,	Func_ReturnFromSettingsFrame }, // Configure Mappings
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[30],	180.0,	170.0,	280.0,	56.0,	29,	31,	-1,	-1,	Func_ConfigurePaks,		Func_ReturnFromSettingsFrame }, // Configure Paks
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[31],	180.0,	240.0,	280.0,	56.0,	30,	32,	-1,	-1,	Func_ConfigureButtons,	Func_ReturnFromSettingsFrame }, // Configure Buttons
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[10],	295.0,	310.0,	 55.0,	56.0,	31,	34,	33,	33,	Func_SaveButtonsSD,		Func_ReturnFromSettingsFrame }, // Save Configuration: SD
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[11],	360.0,	310.0,	 70.0,	56.0,	31,	34,	32,	32,	Func_SaveButtonsFAT,	Func_ReturnFromSettingsFrame }, // Save Configuration: FAT
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[34],	295.0,	380.0,	135.0,	56.0,	32,	 2,	-1,	-1,	Func_ToggleButtonLoad,	Func_ReturnFromSettingsFrame }, // Toggle Button Load Slot
	//Buttons for Audio Tab (starts at button[35])
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[38],	345.0,	100.0,	 75.0,	56.0,	 3,	38,	36,	36,	Func_DisableAudioYes,	Func_ReturnFromSettingsFrame }, // Disable Audio: Yes
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[39],	440.0,	100.0,	 75.0,	56.0,	 3,	39,	35,	35,	Func_DisableAudioNo,	Func_ReturnFromSettingsFrame }, // Disable Audio: No
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[25],	250.0,	170.0,	 75.0,	56.0,	35,	40,	39,	38,	Func_SpeedLimitOff,		Func_ReturnFromSettingsFrame }, // Speed Limit: Off
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[40],	345.0,	170.0,	 75.0,	56.0,	35,	40,	37,	39,	Func_SpeedLimitVi,		Func_ReturnFromSettingsFrame }, // Speed Limit: VI
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[41],	440.0,	170.0,	 75.0,	56.0,	36,	41,	38,	37,	Func_SpeedLimitDl,		Func_ReturnFromSettingsFrame }, // Speed Limit: DL
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[38],	345.0,	240.0,	 75.0,	56.0,	38,	 3,	41,	41,	Func_ScalePitchYes,		Func_ReturnFromSettingsFrame }, // Scale Pitch: Yes
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[39],	440.0,	240.0,	 75.0,	56.0,	39,	 3,	40,	40,	Func_ScalePitchNo,		Func_ReturnFromSettingsFrame }, // Scale Pitch: No
	//Buttons for Saves Tab (starts at button[42])
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[38],	375.0,	100.0,	 75.0,	56.0,	 4,	44,	43,	43,	Func_AutoLoadSaveYes,	Func_ReturnFromSettingsFrame }, // Auto Load Native Saves: Yes
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[39],	470.0,	100.0,	 75.0,	56.0,	 4,	45,	42,	42,	Func_AutoLoadSaveNo,	Func_ReturnFromSettingsFrame }, // Auto Load Native Saves: No
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[38],	375.0,	170.0,	 75.0,	56.0,	42,	46,	45,	45,	Func_AutoSaveYes,		Func_ReturnFromSettingsFrame }, // Auto Save Native Saves: Yes
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[39],	470.0,	170.0,	 75.0,	56.0,	43,	46,	44,	44,	Func_AutoSaveNo,		Func_ReturnFromSettingsFrame }, // Auto Save Native Saves: No
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[44],	365.0,	240.0,	190.0,	56.0,	44,	47,	-1,	-1,	Func_CopySaves,			Func_ReturnFromSettingsFrame }, // Copy Saves
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[45],	365.0,	310.0,	190.0,	56.0,	46,	 4,	-1,	-1,	Func_DeleteSaves,		Func_ReturnFromSettingsFrame }, // Delete Saves
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
	{	NULL,	FRAME_STRINGS[5],	155.0,	128.0,	 1.0,	true }, // Native Save Device: SD/FAT/CardA/CardB
	{	NULL,	FRAME_STRINGS[6],	155.0,	198.0,	 1.0,	true }, // Save State Device: SD/FAT
	{	NULL,	FRAME_STRINGS[7],	155.0,	268.0,	 1.0,	true }, // CPU Emulator: Interpreter/Recompiler
	{	NULL,	FRAME_STRINGS[8],	155.0,	338.0,	 1.0,	true }, // CPU Clock Divider: 1/2/3
	{	NULL,	FRAME_STRINGS[9],	155.0,	408.0,	 1.0,	true }, // Save Settings: SD/FAT
	//TextBoxes for Video Tab (starts at textBox[4])
	{	NULL,	FRAME_STRINGS[19],	190.0,	128.0,	 1.0,	true }, // Show FPS: On/Off
	{	NULL,	FRAME_STRINGS[20],	130.0,	198.0,	 1.0,	true }, // ScreenMode: 4:3/16:9/Force 16:9
	{	NULL,	FRAME_STRINGS[21],	190.0,	268.0,	 1.0,	true }, // CPU Framebuffer: On/Off
	{	NULL,	FRAME_STRINGS[22],	190.0,	338.0,	 1.0,	true }, // 2xSaI Textures: On/Off
	{	NULL,	FRAME_STRINGS[23],	190.0,	408.0,	 1.0,	true }, // FB Textures: On/Off
	//TextBoxes for Input Tab (starts at textBox[9])
	{	NULL,	FRAME_STRINGS[32],	155.0,	338.0,	 1.0,	true }, // Save Configuration: SD/FAT
	{	NULL,	FRAME_STRINGS[33],	155.0,	408.0,	 1.0,	true }, // Toggle Button Load Slot
	//TextBoxes for Audio Tab (starts at textBox[11])
	{	NULL,	FRAME_STRINGS[35],	210.0,	128.0,	 1.0,	true }, // Disable Audio: Yes/No
	{	NULL,	FRAME_STRINGS[36],	150.0,	198.0,	 1.0,	true }, // Speed Limit: Off/VI/DL
	{	NULL,	FRAME_STRINGS[37],	210.0,	268.0,	 1.0,	true }, // Scale Pitch: Yes/No
	//TextBoxes for Saves Tab (starts at textBox[14])
	{	NULL,	FRAME_STRINGS[42],	200.0,	128.0,	 1.0,	true }, // Auto Load Native Saves: Yes/No
	{	NULL,	FRAME_STRINGS[43],	200.0,	198.0,	 1.0,	true }, // Auto Save Native Saves: Yes/No
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
				FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_UP, FRAME_BUTTONS[16].button);
				FRAME_BUTTONS[i].button->setActive(true);
			}
			for (int i = 0; i < 5; i++)
				FRAME_TEXTBOXES[i].textBox->setVisible(true);
			FRAME_BUTTONS[0].button->setSelected(true);
			FRAME_BUTTONS[5+nativeSaveDevice].button->setSelected(true);
			FRAME_BUTTONS[9+saveStateDevice].button->setSelected(true);
			if (dynacore == DYNACORE_PURE_INTERP)	FRAME_BUTTONS[11].button->setSelected(true);
			else									FRAME_BUTTONS[12].button->setSelected(true);
			if (count_per_op == COUNT_PER_OP_1)			FRAME_BUTTONS[13].button->setSelected(true);
			else if (count_per_op == COUNT_PER_OP_2)	FRAME_BUTTONS[14].button->setSelected(true);
			else										FRAME_BUTTONS[15].button->setSelected(true);
			for (int i = 5; i < 18; i++)
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
				FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_DOWN, FRAME_BUTTONS[18].button);
				FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_UP, FRAME_BUTTONS[27].button);
				FRAME_BUTTONS[i].button->setActive(true);
			}
			for (int i = 5; i < 10; i++)
				FRAME_TEXTBOXES[i].textBox->setVisible(true);
			FRAME_BUTTONS[1].button->setSelected(true);
			if (showFPSonScreen == FPS_SHOW)	FRAME_BUTTONS[18].button->setSelected(true);
			else								FRAME_BUTTONS[19].button->setSelected(true);
			if (screenMode == SCREENMODE_4x3)		FRAME_BUTTONS[20].button->setSelected(true);
			else if (screenMode == SCREENMODE_16x9)	FRAME_BUTTONS[21].button->setSelected(true);
			else									FRAME_BUTTONS[22].button->setSelected(true);
			if (renderCpuFramebuffer == CPUFRAMEBUFFER_ENABLE)	FRAME_BUTTONS[23].button->setSelected(true);
			else												FRAME_BUTTONS[24].button->setSelected(true);
			if (glN64_use2xSaiTextures == GLN64_2XSAI_ENABLE)	FRAME_BUTTONS[25].button->setSelected(true);
			else												FRAME_BUTTONS[26].button->setSelected(true);
			if (glN64_useFrameBufferTextures == GLN64_FBTEX_ENABLE)	FRAME_BUTTONS[27].button->setSelected(true);
			else													FRAME_BUTTONS[28].button->setSelected(true);
			for (int i = 18; i < 29; i++)
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
				FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_DOWN, FRAME_BUTTONS[29].button);
				FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_UP, FRAME_BUTTONS[34].button);
				FRAME_BUTTONS[i].button->setActive(true);
			}
			for (int i = 10; i < 12; i++)
				FRAME_TEXTBOXES[i].textBox->setVisible(true);
			FRAME_BUTTONS[2].button->setSelected(true);
			if (loadButtonSlot == LOADBUTTON_DEFAULT)	strcpy(FRAME_STRINGS[34], "Default");
			else										sprintf(FRAME_STRINGS[34], "Slot %d", loadButtonSlot+1);
			for (int i = 29; i < 35; i++)
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
				FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_DOWN, FRAME_BUTTONS[35].button);
				FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_UP, FRAME_BUTTONS[40].button);
				FRAME_BUTTONS[i].button->setActive(true);
			}
			for (int i = 12; i < 15; i++)
				FRAME_TEXTBOXES[i].textBox->setVisible(true);
			FRAME_BUTTONS[3].button->setSelected(true);
			if (audioEnabled == AUDIO_DISABLE)	FRAME_BUTTONS[35].button->setSelected(true);
			else								FRAME_BUTTONS[36].button->setSelected(true);
			if (Timers.limitVIs == LIMITVIS_NONE)				FRAME_BUTTONS[37].button->setSelected(true);
			else if (Timers.limitVIs == LIMITVIS_WAIT_FOR_VI)	FRAME_BUTTONS[38].button->setSelected(true);
			else												FRAME_BUTTONS[39].button->setSelected(true);
			if (scalePitch == SCALEPITCH_ENABLE)	FRAME_BUTTONS[40].button->setSelected(true);
			else									FRAME_BUTTONS[41].button->setSelected(true);
			for (int i = 35; i < 42; i++)
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
				FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_DOWN, FRAME_BUTTONS[42].button);
				FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_UP, FRAME_BUTTONS[47].button);
				FRAME_BUTTONS[i].button->setActive(true);
			}
			for (int i = 15; i < NUM_FRAME_TEXTBOXES; i++)
				FRAME_TEXTBOXES[i].textBox->setVisible(true);
			FRAME_BUTTONS[4].button->setSelected(true);
			if (autoLoadSave == AUTOLOADSAVE_ENABLE)	FRAME_BUTTONS[42].button->setSelected(true);
			else										FRAME_BUTTONS[43].button->setSelected(true);
			if (autoSave == AUTOSAVE_ENABLE)	FRAME_BUTTONS[44].button->setSelected(true);
			else								FRAME_BUTTONS[45].button->setSelected(true);
			for (int i = 42; i < NUM_FRAME_BUTTONS; i++)
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
		WPADData* wpad = menu::Input::getInstance().getWpad();
#endif
		for (int i=0; i<4; i++)
		{
			u16 currentButtonsGC = PAD_ButtonsHeld(i);
#ifdef HW_RVL
			u32 currentButtonsWii = WPAD_ButtonsHeld(i);
#endif
			if (currentButtonsGC ^ previousButtonsGC[i])
			{
				u16 currentButtonsDownGC = (currentButtonsGC ^ previousButtonsGC[i]) & currentButtonsGC;
				previousButtonsGC[i] = currentButtonsGC;
				if (currentButtonsDownGC & PAD_BUTTON_R)
				{
					//move to next tab
					if(activeSubmenu < SUBMENU_SAVES) 
					{
						activateSubmenu(activeSubmenu+1);
						menu::Focus::getInstance().clearPrimaryFocus();
					}
				}
				else if (currentButtonsDownGC & PAD_BUTTON_L)
				{
					//move to the previous tab
					if(activeSubmenu > SUBMENU_GENERAL) 
					{
						activateSubmenu(activeSubmenu-1);
						menu::Focus::getInstance().clearPrimaryFocus();
					}
				}
				break;
			}
#ifdef HW_RVL
			else if (currentButtonsWii ^ previousButtonsWii[i])
			{
				u32 currentButtonsDownWii = (currentButtonsWii ^ previousButtonsWii[i]) & currentButtonsWii;
				previousButtonsWii[i] = currentButtonsWii;
				switch (wpad[i].exp.type)
				{
				case WPAD_EXP_CLASSIC:
				case WPAD_EXP_WIIUPRO:
					if (currentButtonsDownWii & (WPAD_CLASSIC_BUTTON_FULL_R | WPAD_CLASSIC_BUTTON_ZR))
					{
						//move to next tab
						if(activeSubmenu < SUBMENU_SAVES) 
						{
							activateSubmenu(activeSubmenu+1);
							menu::Focus::getInstance().clearPrimaryFocus();
						}
					}
					else if (currentButtonsDownWii & (WPAD_CLASSIC_BUTTON_FULL_L | WPAD_CLASSIC_BUTTON_ZL))
					{
						//move to the previous tab
						if(activeSubmenu > SUBMENU_GENERAL) 
						{
							activateSubmenu(activeSubmenu-1);
							menu::Focus::getInstance().clearPrimaryFocus();
						}
					}
					break;
				case WPAD_EXP_NES:
				case WPAD_EXP_SNES:
				case WPAD_EXP_N64:
					if (currentButtonsDownWii & WPAD_SNES_BUTTON_R)
					{
						//move to next tab
						if(activeSubmenu < SUBMENU_SAVES) 
						{
							activateSubmenu(activeSubmenu+1);
							menu::Focus::getInstance().clearPrimaryFocus();
						}
					}
					else if (currentButtonsDownWii & WPAD_SNES_BUTTON_L)
					{
						//move to the previous tab
						if(activeSubmenu > SUBMENU_GENERAL) 
						{
							activateSubmenu(activeSubmenu-1);
							menu::Focus::getInstance().clearPrimaryFocus();
						}
					}
					break;
				case WPAD_EXP_GC:
					if (currentButtonsDownWii & WPAD_GC_BUTTON_R)
					{
						//move to next tab
						if(activeSubmenu < SUBMENU_SAVES) 
						{
							activateSubmenu(activeSubmenu+1);
							menu::Focus::getInstance().clearPrimaryFocus();
						}
					}
					else if (currentButtonsDownWii & WPAD_GC_BUTTON_L)
					{
						//move to the previous tab
						if(activeSubmenu > SUBMENU_GENERAL) 
						{
							activateSubmenu(activeSubmenu-1);
							menu::Focus::getInstance().clearPrimaryFocus();
						}
					}
					break;
				default:
					if (currentButtonsDownWii & WPAD_BUTTON_PLUS)
					{
						//move to next tab
						if(activeSubmenu < SUBMENU_SAVES) 
						{
							activateSubmenu(activeSubmenu+1);
							menu::Focus::getInstance().clearPrimaryFocus();
						}
					}
					else if (currentButtonsDownWii & WPAD_BUTTON_MINUS)
					{
						//move to the previous tab
						if(activeSubmenu > SUBMENU_GENERAL) 
						{
							activateSubmenu(activeSubmenu-1);
							menu::Focus::getInstance().clearPrimaryFocus();
						}
					}
					break;
				}
				break;
			}
			else if (i == 0 && WiiDRC_Inited() && WiiDRC_Connected() && (WiiDRC_ButtonsHeld() ^ previousButtonsDRC[i]))
			{
				u32 currentButtonsDownDRC = (WiiDRC_ButtonsHeld() ^ previousButtonsDRC[i]) & WiiDRC_ButtonsHeld();
				previousButtonsDRC[i] = WiiDRC_ButtonsHeld();
				if (currentButtonsDownDRC & WIIDRC_BUTTON_R)
				{
					//move to next tab
					if(activeSubmenu < SUBMENU_SAVES) 
					{
						activateSubmenu(activeSubmenu+1);
						menu::Focus::getInstance().clearPrimaryFocus();
					}
					break;
				}
				else if (currentButtonsDownDRC & WIIDRC_BUTTON_L)
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

void Func_NativeSaveFAT()
{
	for (int i = 5; i <= 8; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[6].button->setSelected(true);
	nativeSaveDevice = NATIVESAVEDEVICE_FAT;
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

void Func_SaveStateFAT()
{
	for (int i = 9; i <= 10; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[10].button->setSelected(true);
	saveStateDevice = SAVESTATEDEVICE_FAT;
}

void Func_CpuPureInterp()
{
	for (int i = 11; i <= 12; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[11].button->setSelected(true);
	dynacore = DYNACORE_PURE_INTERP;
}

void Func_CpuDynarec()
{
	for (int i = 11; i <= 12; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[12].button->setSelected(true);
	dynacore = DYNACORE_DYNAREC;
}

void Func_ResetROM();

void Func_ClockDivider1()
{
	for (int i = 13; i <= 15; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[13].button->setSelected(true);
	count_per_op = COUNT_PER_OP_1;
	Func_ResetROM();
}

void Func_ClockDivider2()
{
	for (int i = 13; i <= 15; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[14].button->setSelected(true);
	count_per_op = COUNT_PER_OP_2;
	Func_ResetROM();
}

void Func_ClockDivider3()
{
	for (int i = 13; i <= 15; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[15].button->setSelected(true);
	count_per_op = COUNT_PER_OP_3;
	Func_ResetROM();
}

extern void writeConfig(FILE* f);

void Func_SaveSettingsSD()
{
	fileBrowser_file* configFile_file;
	int (*configFile_init)(fileBrowser_file*) = fileBrowser_libfat_init;
	configFile_file = &saveDir_libfat_Default;
	if(configFile_init(configFile_file)) {                //only if device initialized ok
		FILE* f = fopen( "sd:/not64/settings.cfg", "wb" );  //attempt to open file
		if(f) {
			writeConfig(f);                                   //write out the config
			fclose(f);
			menu::MessageBox::getInstance().setMessage("Saved settings to SD");
			return;
		}
	}
	menu::MessageBox::getInstance().setMessage("Error saving settings to SD");
}

void Func_SaveSettingsFAT()
{
	fileBrowser_file* configFile_file;
	int (*configFile_init)(fileBrowser_file*) = fileBrowser_libfat_init;
	configFile_file = &saveDir_libfat;
	if(configFile_init(configFile_file)) {                //only if device initialized ok
		FILE* f = fopen( "fat:/not64/settings.cfg", "wb" ); //attempt to open file
		if(f) {
			writeConfig(f);                                   //write out the config
			fclose(f);
			menu::MessageBox::getInstance().setMessage("Saved settings to FAT");
			return;
		}
	}
	menu::MessageBox::getInstance().setMessage("Error saving settings to FAT");
}

void Func_ShowFpsOn()
{
	for (int i = 18; i <= 19; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[18].button->setSelected(true);
	showFPSonScreen = FPS_SHOW;
}

void Func_ShowFpsOff()
{
	for (int i = 18; i <= 19; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[19].button->setSelected(true);
	showFPSonScreen = FPS_HIDE;
}

extern GXRModeObj *vmode, *rmode;
void gfx_set_window(int x, int y, int width, int height);

void Func_ScreenMode4_3()
{
	for (int i = 20; i <= 22; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[20].button->setSelected(true);
	screenMode = SCREENMODE_4x3;
	gfx_set_window( 0, 0, 640, rmode->efbHeight);
#ifdef HW_RVL
	VIDEO_SetAspectRatio(VI_DISPLAY_BOTH, VI_ASPECT_3_4);
#endif
}

void Func_ScreenMode16_9()
{
	for (int i = 20; i <= 22; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[21].button->setSelected(true);
	screenMode = SCREENMODE_16x9;
	gfx_set_window( 0, 0, 640, rmode->efbHeight);
#ifdef HW_RVL
	VIDEO_SetAspectRatio(VI_DISPLAY_BOTH, VI_ASPECT_1_1);
#endif
}

void Func_ScreenForce16_9()
{
	for (int i = 20; i <= 22; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[22].button->setSelected(true);
	screenMode = SCREENMODE_16x9_PILLARBOX;
	gfx_set_window( 80, 0, 480, rmode->efbHeight);
#ifdef HW_RVL
	VIDEO_SetAspectRatio(VI_DISPLAY_BOTH, VI_ASPECT_1_1);
#endif
}

void Func_CpuFramebufferOn()
{
	for (int i = 23; i <= 24; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[23].button->setSelected(true);
	renderCpuFramebuffer = CPUFRAMEBUFFER_ENABLE;
}

void Func_CpuFramebufferOff()
{
	for (int i = 23; i <= 24; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[24].button->setSelected(true);
	renderCpuFramebuffer = CPUFRAMEBUFFER_DISABLE;
}

void Func_2xSaiTexturesOn()
{
	for (int i = 25; i <= 26; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[25].button->setSelected(true);
	glN64_use2xSaiTextures = GLN64_2XSAI_ENABLE;
}

void Func_2xSaiTexturesOff()
{
	for (int i = 25; i <= 26; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[26].button->setSelected(true);
	glN64_use2xSaiTextures = GLN64_2XSAI_DISABLE;
}

void Func_FbTexturesOn()
{
	for (int i = 27; i <= 28; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[27].button->setSelected(true);
	glN64_useFrameBufferTextures = GLN64_FBTEX_ENABLE;
}

void Func_FbTexturesOff()
{
	for (int i = 27; i <= 28; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[28].button->setSelected(true);
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
	menu::Gui::getInstance().menuLogo->setVisible(false);
	pMenuContext->setActiveFrame(MenuContext::FRAME_CONFIGUREBUTTONS,ConfigureButtonsFrame::SUBMENU_N64_PADNONE);
}

void Func_SaveButtonsSD()
{
	fileBrowser_file* configFile_file;
	int (*configFile_init)(fileBrowser_file*) = fileBrowser_libfat_init;
	int num_written = 0;
	configFile_file = &saveDir_libfat_Default;
	if(configFile_init(configFile_file)) {                //only if device initialized ok
		FILE* f = fopen( "sd:/not64/controlG.cfg", "wb" );  //attempt to open file
		if(f) {
			save_configurations(f, &controller_GC);					//write out GC controller mappings
			fclose(f);
			num_written++;
		}
#ifdef HW_RVL
		f = fopen( "sd:/not64/controlD.cfg", "wb" );  //attempt to open file
		if(f) {
			save_configurations(f, &controller_ExtenmoteGC);		//write out GC controller mappings
			fclose(f);
			num_written++;
		}
		f = fopen( "sd:/not64/controlU.cfg", "wb" );  //attempt to open file
		if(f) {
			save_configurations(f, &controller_ExtenmoteN64);		//write out N64 controller mappings
			fclose(f);
			num_written++;
		}
		f = fopen( "sd:/not64/controlS.cfg", "wb" );  //attempt to open file
		if(f) {
			save_configurations(f, &controller_ExtenmoteSNES);		//write out SNES controller mappings
			fclose(f);
			num_written++;
		}
		f = fopen( "sd:/not64/controlF.cfg", "wb" );  //attempt to open file
		if(f) {
			save_configurations(f, &controller_ExtenmoteNES);		//write out NES controller mappings
			fclose(f);
			num_written++;
		}
		f = fopen( "sd:/not64/controlP.cfg", "wb" );  //attempt to open file
		if(f) {
			save_configurations(f, &controller_WiiUPro);			//write out WiiU Pro controller mappings
			fclose(f);
			num_written++;
		}
		f = fopen( "sd:/not64/controlC.cfg", "wb" );  //attempt to open file
		if(f) {
			save_configurations(f, &controller_Classic);			//write out Classic controller mappings
			fclose(f);
			num_written++;
		}
		f = fopen( "sd:/not64/controlN.cfg", "wb" );  //attempt to open file
		if(f) {
			save_configurations(f, &controller_WiimoteNunchuk);	//write out WM+NC controller mappings
			fclose(f);
			num_written++;
		}
		f = fopen( "sd:/not64/controlW.cfg", "wb" );  //attempt to open file
		if(f) {
			save_configurations(f, &controller_Wiimote);			//write out Wiimote controller mappings
			fclose(f);
			num_written++;
		}
#endif //HW_RVL
	}
	if (num_written == num_controller_t)
		menu::MessageBox::getInstance().setMessage("Saved configuration to SD");
	else
		menu::MessageBox::getInstance().setMessage("Error saving configuration to SD");
}

void Func_SaveButtonsFAT()
{
	fileBrowser_file* configFile_file;
	int (*configFile_init)(fileBrowser_file*) = fileBrowser_libfat_init;
	int num_written = 0;
	configFile_file = &saveDir_libfat;
	if(configFile_init(configFile_file)) {                //only if device initialized ok
		FILE* f = fopen( "fat:/not64/controlG.cfg", "wb" );  //attempt to open file
		if(f) {
			save_configurations(f, &controller_GC);					//write out GC controller mappings
			fclose(f);
			num_written++;
		}
#ifdef HW_RVL
		f = fopen( "fat:/not64/controlD.cfg", "wb" );  //attempt to open file
		if(f) {
			save_configurations(f, &controller_ExtenmoteGC);		//write out GC controller mappings
			fclose(f);
			num_written++;
		}
		f = fopen( "fat:/not64/controlU.cfg", "wb" );  //attempt to open file
		if(f) {
			save_configurations(f, &controller_ExtenmoteN64);		//write out N64 controller mappings
			fclose(f);
			num_written++;
		}
		f = fopen( "fat:/not64/controlS.cfg", "wb" );  //attempt to open file
		if(f) {
			save_configurations(f, &controller_ExtenmoteSNES);		//write out SNES controller mappings
			fclose(f);
			num_written++;
		}
		f = fopen( "fat:/not64/controlF.cfg", "wb" );  //attempt to open file
		if(f) {
			save_configurations(f, &controller_ExtenmoteNES);		//write out NES controller mappings
			fclose(f);
			num_written++;
		}
		f = fopen( "fat:/not64/controlP.cfg", "wb" );  //attempt to open file
		if(f) {
			save_configurations(f, &controller_WiiUPro);			//write out WiiU Pro controller mappings
			fclose(f);
			num_written++;
		}
		f = fopen( "fat:/not64/controlC.cfg", "wb" );  //attempt to open file
		if(f) {
			save_configurations(f, &controller_Classic);			//write out Classic controller mappings
			fclose(f);
			num_written++;
		}
		f = fopen( "fat:/not64/controlN.cfg", "wb" );  //attempt to open file
		if(f) {
			save_configurations(f, &controller_WiimoteNunchuk);	//write out WM+NC controller mappings
			fclose(f);
			num_written++;
		}
		f = fopen( "fat:/not64/controlW.cfg", "wb" );  //attempt to open file
		if(f) {
			save_configurations(f, &controller_Wiimote);			//write out Wiimote controller mappings
			fclose(f);
			num_written++;
		}
#endif //HW_RVL
	}
	if (num_written == num_controller_t)
		menu::MessageBox::getInstance().setMessage("Saved configuration to FAT");
	else
		menu::MessageBox::getInstance().setMessage("Error saving configuration to FAT");
}

void Func_ToggleButtonLoad()
{
	loadButtonSlot = (loadButtonSlot + 1) % 5;
	if (loadButtonSlot == LOADBUTTON_DEFAULT)
		strcpy(FRAME_STRINGS[34], "Default");
	else
		sprintf(FRAME_STRINGS[34], "Slot %d", loadButtonSlot+1);
}

void Func_DisableAudioYes()
{
	for (int i = 35; i <= 36; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[35].button->setSelected(true);
	audioEnabled = AUDIO_DISABLE;
}

void Func_DisableAudioNo()
{
	for (int i = 35; i <= 36; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[36].button->setSelected(true);
	audioEnabled = AUDIO_ENABLE;
}

void Func_SpeedLimitOff()
{
	for (int i = 37; i <= 39; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[37].button->setSelected(true);
	Timers.limitVIs = LIMITVIS_NONE;
}

void Func_SpeedLimitVi()
{
	for (int i = 37; i <= 39; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[38].button->setSelected(true);
	Timers.limitVIs = LIMITVIS_WAIT_FOR_VI;
}

void Func_SpeedLimitDl()
{
	for (int i = 37; i <= 39; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[39].button->setSelected(true);
	Timers.limitVIs = LIMITVIS_WAIT_FOR_FRAME;
}

void Func_ScalePitchYes()
{
	for (int i = 40; i <= 41; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[40].button->setSelected(true);
	scalePitch = SCALEPITCH_ENABLE;
}

void Func_ScalePitchNo()
{
	for (int i = 40; i <= 41; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[41].button->setSelected(true);
	scalePitch = SCALEPITCH_DISABLE;
}

void Func_AutoLoadSaveYes()
{
	for (int i = 42; i <= 43; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[42].button->setSelected(true);
	autoLoadSave = AUTOLOADSAVE_ENABLE;
}

void Func_AutoLoadSaveNo()
{
	for (int i = 42; i <= 43; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[43].button->setSelected(true);
	autoLoadSave = AUTOLOADSAVE_DISABLE;
}

void Func_AutoSaveYes()
{
	for (int i = 44; i <= 45; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[44].button->setSelected(true);
	autoSave = AUTOSAVE_ENABLE;
}

void Func_AutoSaveNo()
{
	for (int i = 44; i <= 45; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[45].button->setSelected(true);
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
