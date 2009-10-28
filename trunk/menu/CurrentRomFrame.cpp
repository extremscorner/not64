#include "MenuContext.h"
#include "CurrentRomFrame.h"
#include "../libgui/Button.h"
#include "../libgui/resources.h"
#include "../libgui/FocusManager.h"
#include "../libgui/CursorManager.h"
#include "../libgui/MessageBox.h"
#include "../main/wii64config.h"

extern "C" {
#include "../gc_memory/memory.h"
#include "../gc_memory/Saves.h"
#include "../main/plugin.h"
#include "../main/savestates.h"
#include "../fileBrowser/fileBrowser.h"
#include "../fileBrowser/fileBrowser-libfat.h"
#include "../fileBrowser/fileBrowser-CARD.h"
}

void Func_ShowRomInfo();
void Func_ResetROM();
void Func_LoadSave();
void Func_SaveGame();
void Func_LoadState();
void Func_SaveState();
void Func_StateCycle();
void Func_ReturnFromCurrentRomFrame();

#define NUM_FRAME_BUTTONS 7
#define FRAME_BUTTONS currentRomFrameButtons
#define FRAME_STRINGS currentRomFrameStrings

static char FRAME_STRINGS[7][25] =
	{ "Show ROM Info",
	  "Restart Game",
	  "Load Save File",
	  "Save Game",
	  "Load State",
	  "Save State",
	  "Slot 0"};

struct ButtonInfo
{
	menu::Button	*button;
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
{ //	button	buttonString		x		y		width	height	Up	Dwn	Lft	Rt	clickFunc			returnFunc
	{	NULL,	FRAME_STRINGS[0],	150.0,	 60.0,	340.0,	40.0,	 5,	 1,	-1,	-1,	Func_ShowRomInfo,	Func_ReturnFromCurrentRomFrame }, // Show ROM Info
	{	NULL,	FRAME_STRINGS[1],	150.0,	120.0,	340.0,	40.0,	 0,	 2,	-1,	-1,	Func_ResetROM,		Func_ReturnFromCurrentRomFrame }, // Reset ROM
	{	NULL,	FRAME_STRINGS[2],	150.0,	180.0,	340.0,	40.0,	 1,	 3,	-1,	-1,	Func_LoadSave,		Func_ReturnFromCurrentRomFrame }, // Load Native Save
	{	NULL,	FRAME_STRINGS[3],	150.0,	240.0,	340.0,	40.0,	 2,	 4,	-1,	-1,	Func_SaveGame,		Func_ReturnFromCurrentRomFrame }, // Save Native Save
	{	NULL,	FRAME_STRINGS[4],	150.0,	300.0,	220.0,	40.0,	 3,	 5,	 6,	 6,	Func_LoadState,		Func_ReturnFromCurrentRomFrame }, // Load State 
	{	NULL,	FRAME_STRINGS[5],	150.0,	360.0,	220.0,	40.0,	 4,	 0,	 6,	 6,	Func_SaveState,		Func_ReturnFromCurrentRomFrame }, // Save State 
	{	NULL,	FRAME_STRINGS[6],	390.0,	330.0,	100.0,	40.0,	 3,	 0,	 4,	 4,	Func_StateCycle,	Func_ReturnFromCurrentRomFrame }, // Cycle State 
};

CurrentRomFrame::CurrentRomFrame()
{
	buttonImage = new menu::Image(ButtonTexture, 16, 16, GX_TF_I8, GX_CLAMP, GX_CLAMP, GX_FALSE);
	buttonFocusImage = new menu::Image(ButtonFocusTexture, 16, 16, GX_TF_I8, GX_CLAMP, GX_CLAMP, GX_FALSE);
	for (int i = 0; i < NUM_FRAME_BUTTONS; i++)
		FRAME_BUTTONS[i].button = new menu::Button(buttonImage, &FRAME_BUTTONS[i].buttonString, 
										FRAME_BUTTONS[i].x, FRAME_BUTTONS[i].y, 
										FRAME_BUTTONS[i].width, FRAME_BUTTONS[i].height);

	for (int i = 0; i < NUM_FRAME_BUTTONS; i++)
	{
		FRAME_BUTTONS[i].button->setFocusImage(buttonFocusImage);
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
	setDefaultFocus(FRAME_BUTTONS[0].button);
	setBackFunc(Func_ReturnFromCurrentRomFrame);
	setEnabled(true);

}

CurrentRomFrame::~CurrentRomFrame()
{
	for (int i = 0; i < NUM_FRAME_BUTTONS; i++)
	{
		menu::Cursor::getInstance().removeComponent(this, FRAME_BUTTONS[i].button);
		delete FRAME_BUTTONS[i].button;
	}
	delete buttonFocusImage;
	delete buttonImage;

}

extern MenuContext *pMenuContext;

void Func_ShowRomInfo()
{
	menu::MessageBox::getInstance().setMessage("Show ROM Info not implemented");
}

extern BOOL hasLoadedROM;

extern "C" {
void cpu_init();
void cpu_deinit();
}

void Func_ResetROM()
{
	if(hasLoadedROM)
	{
		cpu_deinit();
		romClosed_RSP();
		romClosed_input();
		romClosed_audio();
		romClosed_gfx();
		free_memory();
		
		init_memory();
		romOpen_gfx();
		romOpen_audio();
		romOpen_input();
		cpu_init();
		menu::MessageBox::getInstance().setMessage("Game restarted");
	}
	else	
	{
		menu::MessageBox::getInstance().setMessage("Please load a ROM first");
	}
}

void Func_LoadSave()
{
	if(!hasLoadedROM)
	{
		menu::MessageBox::getInstance().setMessage("Please load a ROM first");
		return;
	}

	switch (nativeSaveDevice)
	{
		case NATIVESAVEDEVICE_SD:
			// Adjust saveFile pointers
			saveFile_dir = &saveDir_libfat_Default;
			saveFile_readFile  = fileBrowser_libfat_readFile;
			saveFile_writeFile = fileBrowser_libfat_writeFile;
			saveFile_init      = fileBrowser_libfat_init;
			saveFile_deinit    = fileBrowser_libfat_deinit;
			break;
		case NATIVESAVEDEVICE_USB:
			// Adjust saveFile pointers
			saveFile_dir = &saveDir_libfat_USB;
			saveFile_readFile  = fileBrowser_libfat_readFile;
			saveFile_writeFile = fileBrowser_libfat_writeFile;
			saveFile_init      = fileBrowser_libfat_init;
			saveFile_deinit    = fileBrowser_libfat_deinit;
			break;
		case NATIVESAVEDEVICE_CARDA:
			// Adjust saveFile pointers
			saveFile_dir       = &saveDir_CARD_SlotA;
			saveFile_readFile  = fileBrowser_CARD_readFile;
			saveFile_writeFile = fileBrowser_CARD_writeFile;
			saveFile_init      = fileBrowser_CARD_init;
			saveFile_deinit    = fileBrowser_CARD_deinit;
			break;
		case NATIVESAVEDEVICE_CARDB:
			// Adjust saveFile pointers
			saveFile_dir       = &saveDir_CARD_SlotB;
			saveFile_readFile  = fileBrowser_CARD_readFile;
			saveFile_writeFile = fileBrowser_CARD_writeFile;
			saveFile_init      = fileBrowser_CARD_init;
			saveFile_deinit    = fileBrowser_CARD_deinit;
			break;
	}

	// Try loading everything
	int result = 0;
	saveFile_init(saveFile_dir);
	result += loadEeprom(saveFile_dir);
	result += loadSram(saveFile_dir);
	result += loadMempak(saveFile_dir);
	result += loadFlashram(saveFile_dir);
	saveFile_deinit(saveFile_dir);

	switch (nativeSaveDevice)
	{
		case NATIVESAVEDEVICE_SD:
			if (result) menu::MessageBox::getInstance().setMessage("Loaded save from SD card");
			else		menu::MessageBox::getInstance().setMessage("No saves found on SD card");
			break;
		case NATIVESAVEDEVICE_USB:
			if (result) menu::MessageBox::getInstance().setMessage("Loaded save from USB device");
			else		menu::MessageBox::getInstance().setMessage("No saves found on USB device");
			break;
		case NATIVESAVEDEVICE_CARDA:
			if (result) menu::MessageBox::getInstance().setMessage("Loaded save from memcard in slot A");
			else		menu::MessageBox::getInstance().setMessage("No saves found on memcard A");
			break;
		case NATIVESAVEDEVICE_CARDB:
			if (result) menu::MessageBox::getInstance().setMessage("Loaded save from memcard in slot A");
			else		menu::MessageBox::getInstance().setMessage("No saves found on memcard B");
			break;
	}
}

extern BOOL sramWritten;
extern BOOL eepromWritten;
extern BOOL mempakWritten;
extern BOOL flashramWritten;

void Func_SaveGame()
{
  if(!flashramWritten && !sramWritten && !eepromWritten && !mempakWritten) {
    menu::MessageBox::getInstance().setMessage("Nothing to save");
    return;
  }
	switch (nativeSaveDevice)
	{
		case NATIVESAVEDEVICE_SD:
			// Adjust saveFile pointers
			saveFile_dir = &saveDir_libfat_Default;
			saveFile_readFile  = fileBrowser_libfat_readFile;
			saveFile_writeFile = fileBrowser_libfat_writeFile;
			saveFile_init      = fileBrowser_libfat_init;
			saveFile_deinit    = fileBrowser_libfat_deinit;
			break;
		case NATIVESAVEDEVICE_USB:
			// Adjust saveFile pointers
			saveFile_dir = &saveDir_libfat_USB;
			saveFile_readFile  = fileBrowser_libfat_readFile;
			saveFile_writeFile = fileBrowser_libfat_writeFile;
			saveFile_init      = fileBrowser_libfat_init;
			saveFile_deinit    = fileBrowser_libfat_deinit;
			break;
		case NATIVESAVEDEVICE_CARDA:
			// Adjust saveFile pointers
			saveFile_dir       = &saveDir_CARD_SlotA;
			saveFile_readFile  = fileBrowser_CARD_readFile;
			saveFile_writeFile = fileBrowser_CARD_writeFile;
			saveFile_init      = fileBrowser_CARD_init;
			saveFile_deinit    = fileBrowser_CARD_deinit;
			break;
		case NATIVESAVEDEVICE_CARDB:
			// Adjust saveFile pointers
			saveFile_dir       = &saveDir_CARD_SlotB;
			saveFile_readFile  = fileBrowser_CARD_readFile;
			saveFile_writeFile = fileBrowser_CARD_writeFile;
			saveFile_init      = fileBrowser_CARD_init;
			saveFile_deinit    = fileBrowser_CARD_deinit;
			break;
	}

	// Try saving everything
	int amountSaves = flashramWritten + sramWritten + eepromWritten + mempakWritten;
	int result = 0;
	saveFile_init(saveFile_dir);
	result += saveEeprom(saveFile_dir);
	result += saveSram(saveFile_dir);
	result += saveMempak(saveFile_dir);
	result += saveFlashram(saveFile_dir);
	saveFile_deinit(saveFile_dir);

	if (result==amountSaves)	
		switch (nativeSaveDevice)
		{
			case NATIVESAVEDEVICE_SD:
				menu::MessageBox::getInstance().setMessage("Saved game to SD card");
				break;
			case NATIVESAVEDEVICE_USB:
				menu::MessageBox::getInstance().setMessage("Saved game to USB device");
				break;
			case NATIVESAVEDEVICE_CARDA:
				menu::MessageBox::getInstance().setMessage("Saved game to memcard in Slot A");
				break;
			case NATIVESAVEDEVICE_CARDB:
				menu::MessageBox::getInstance().setMessage("Saved game to memcard in Slot B");
				break;
		}
	else		menu::MessageBox::getInstance().setMessage("Failed to Save");
}

void Func_LoadState()
{
	char* txt = savestates_load();
	menu::MessageBox::getInstance().setMessage(&txt[0]);
}

void Func_SaveState()
{
	char* txt = savestates_save();
	menu::MessageBox::getInstance().setMessage(&txt[0]);
}

static unsigned int which_slot = 0;

void Func_StateCycle()
{
	which_slot = (which_slot+1) %10;
	savestates_select_slot(which_slot);
	FRAME_STRINGS[6][5] = which_slot + '0';
}

void Func_ReturnFromCurrentRomFrame()
{
	pMenuContext->setActiveFrame(MenuContext::FRAME_MAIN);
}
