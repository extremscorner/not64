#include "MenuContext.h"
#include "MainFrame.h"
#include "SettingsFrame.h"
#include "../libgui/Button.h"
#include "../libgui/resources.h"
#include "../libgui/FocusManager.h"
#include "../libgui/CursorManager.h"
#include "../libgui/MessageBox.h"
#include "../main/wii64config.h"
#ifdef DEBUGON
# include <debug.h>
#endif
extern "C" {
#ifdef WII
#include <di/di.h>
#endif 
#include "../gc_memory/memory.h"
#include "../gc_memory/Saves.h"
#include "../main/plugin.h"
#include "../main/savestates.h"
#include "../fileBrowser/fileBrowser.h"
#include "../fileBrowser/fileBrowser-libfat.h"
#include "../fileBrowser/fileBrowser-CARD.h"
#include "../main/gc_dvd.h"
}
#include <ogc/dvd.h>

void Func_LoadROM();
void Func_CurrentROM();
void Func_Settings();
void Func_Credits();
void Func_ExitToLoader();
void Func_PlayGame();

#define NUM_MAIN_BUTTONS 6
#define FRAME_BUTTONS mainFrameButtons
#define FRAME_STRINGS mainFrameStrings

char FRAME_STRINGS[7][20] =
	{ "Load ROM",
	  "Current ROM",
	  "Settings",
	  "Credits",
	  "Exit to Loader",
	  "Play Game",
	  "Resume Game"};


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
} FRAME_BUTTONS[NUM_MAIN_BUTTONS] =
{ //	button	buttonString		x		y		width	height	Up	Dwn	Lft	Rt	clickFunc				returnFunc
	{	NULL,	FRAME_STRINGS[0],	195.0,	 60.0,	250.0,	40.0,	 5,	 1,	-1,	-1,	Func_LoadROM,			NULL }, // Load ROM
	{	NULL,	FRAME_STRINGS[1],	195.0,	120.0,	250.0,	40.0,	 0,	 2,	-1,	-1,	Func_CurrentROM,		NULL }, // Current ROM
	{	NULL,	FRAME_STRINGS[2],	195.0,	180.0,	250.0,	40.0,	 1,	 3,	-1,	-1,	Func_Settings,			NULL }, // Settings
	{	NULL,	FRAME_STRINGS[3],	195.0,	240.0,	250.0,	40.0,	 2,	 4,	-1,	-1,	Func_Credits,			NULL }, // Credits
	{	NULL,	FRAME_STRINGS[4],	195.0,	300.0,	250.0,	40.0,	 3,	 5,	-1,	-1,	Func_ExitToLoader,		NULL }, // Exit to Loader
	{	NULL,	FRAME_STRINGS[5],	195.0,	360.0,	250.0,	40.0,	 4,	 0,	-1,	-1,	Func_PlayGame,			NULL }, // Play/Resume Game
};

MainFrame::MainFrame()
{
	buttonImage = new menu::Image(ButtonTexture, 16, 16, GX_TF_I8, GX_CLAMP, GX_CLAMP, GX_FALSE);
	buttonFocusImage = new menu::Image(ButtonFocusTexture, 16, 16, GX_TF_I8, GX_CLAMP, GX_CLAMP, GX_FALSE);
	for (int i = 0; i < NUM_MAIN_BUTTONS; i++)
		FRAME_BUTTONS[i].button = new menu::Button(buttonImage, &FRAME_BUTTONS[i].buttonString, 
										FRAME_BUTTONS[i].x, FRAME_BUTTONS[i].y, 
										FRAME_BUTTONS[i].width, FRAME_BUTTONS[i].height);

	for (int i = 0; i < NUM_MAIN_BUTTONS; i++)
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
	setEnabled(true);

}

MainFrame::~MainFrame()
{
	for (int i = 0; i < NUM_MAIN_BUTTONS; i++)
	{
		menu::Cursor::getInstance().removeComponent(this, FRAME_BUTTONS[i].button);
		delete FRAME_BUTTONS[i].button;
	}
	delete buttonFocusImage;
	delete buttonImage;

}

extern MenuContext *pMenuContext;

void Func_LoadROM()
{
	pMenuContext->setActiveFrame(MenuContext::FRAME_LOADROM);
}

extern BOOL hasLoadedROM;

void Func_CurrentROM()
{
	if(!hasLoadedROM)
	{
		menu::MessageBox::getInstance().setMessage("Please load a ROM first");
		return;
	}

	pMenuContext->setActiveFrame(MenuContext::FRAME_CURRENTROM);
}

void Func_Settings()
{
	pMenuContext->setActiveFrame(MenuContext::FRAME_SETTINGS,SettingsFrame::SUBMENU_GENERAL);
}

void Func_Credits()
{
	char CreditsInfo[256] = "";
	strcat(CreditsInfo,"Wii64 Team:\n");
	strcat(CreditsInfo,"tehpola - core  \n");
	strcat(CreditsInfo,"sepp256 - graphics\n");
	strcat(CreditsInfo,"    emu_kidid - general coding\n");
	strcat(CreditsInfo,"\n");
	strcat(CreditsInfo,"Special Thanks To:\n");
	strcat(CreditsInfo,"Hacktarux - for Mupen64\n");
	strcat(CreditsInfo,"Shagkur - for libOGC \n");
	strcat(CreditsInfo,"Team Twiizers - for Wii homebrew\n");

	menu::MessageBox::getInstance().setMessage(CreditsInfo);
}

void Func_ExitToLoader()
{
#ifdef WII
	DI_Close();
#endif
	void (*rld)() = (void (*)()) 0x80001800;
	rld();
}

extern "C" {
void cpu_init();
void cpu_deinit();
}

extern "C" {
void pauseAudio(void);  void pauseInput(void);
void resumeAudio(void); void resumeInput(void);
void go(void); 
}

extern char autoSave;
extern BOOL sramWritten;
extern BOOL eepromWritten;
extern BOOL mempakWritten;
extern BOOL flashramWritten;


void Func_PlayGame()
{
	if(!hasLoadedROM)
	{
		menu::MessageBox::getInstance().setMessage("Please load a ROM first");
		return;
	}
	
	menu::Gui::getInstance().gfx->clearEFB((GXColor){0, 0, 0, 0xFF}, 0x000000);

	resumeAudio();
	resumeInput();
#ifdef DEBUGON
	_break();
#endif
	go();
#ifdef DEBUGON
	_break();
#endif
	pauseInput();
	pauseAudio();

	//TODO: make the MessageBox here not require an "ok", and just do "Saving to X" and vanish
  if(autoSave==AUTOSAVE_ENABLE) {
    if(flashramWritten || sramWritten || eepromWritten || mempakWritten) {  //something needs saving
      //menu::MessageBox::getInstance().setMessage("Automatically saving game .. ");
      switch (nativeSaveDevice)
    	{
    		case NATIVESAVEDEVICE_SD:
    		case NATIVESAVEDEVICE_USB:
    			// Adjust saveFile pointers
    			saveFile_dir = (nativeSaveDevice==NATIVESAVEDEVICE_SD) ? &saveDir_libfat_Default:&saveDir_libfat_USB;
    			saveFile_readFile  = fileBrowser_libfat_readFile;
    			saveFile_writeFile = fileBrowser_libfat_writeFile;
    			saveFile_init      = fileBrowser_libfat_init;
    			saveFile_deinit    = fileBrowser_libfat_deinit;
    			break;
    		case NATIVESAVEDEVICE_CARDA:
    		case NATIVESAVEDEVICE_CARDB:
    			// Adjust saveFile pointers
    			saveFile_dir       = (nativeSaveDevice==NATIVESAVEDEVICE_CARDA) ? &saveDir_CARD_SlotA:&saveDir_CARD_SlotB;
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
    	if (result==amountSaves) {  //saved all of them ok	
    		switch (nativeSaveDevice)
    		{
    			case NATIVESAVEDEVICE_SD:
    				menu::MessageBox::getInstance().setMessage("Automatically saved to SD card");
    				break;
    			case NATIVESAVEDEVICE_USB:
    				menu::MessageBox::getInstance().setMessage("Automatically saved to USB device");
    				break;
    			case NATIVESAVEDEVICE_CARDA:
    				menu::MessageBox::getInstance().setMessage("Automatically saved to memcard in Slot A");
    				break;
    			case NATIVESAVEDEVICE_CARDB:
    				menu::MessageBox::getInstance().setMessage("Automatically saved to memcard in Slot B");
    				break;
    		}
    		flashramWritten = sramWritten = eepromWritten = mempakWritten = 0;  //nothing new written since save
  		}
  	  else		
  	    menu::MessageBox::getInstance().setMessage("Failed to Save"); //one or more failed to save
      
    }
  }
	FRAME_BUTTONS[5].buttonString = FRAME_STRINGS[6];
	menu::Cursor::getInstance().clearCursorFocus();
}

void Func_SetPlayGame()
{
	FRAME_BUTTONS[5].buttonString = FRAME_STRINGS[5];
}

void Func_SetResumeGame()
{
	FRAME_BUTTONS[5].buttonString = FRAME_STRINGS[6];
}