#include "MenuContext.h"
#include "CurrentRomFrame.h"
#include "../libgui/Button.h"
#include "../libgui/resources.h"
#include "../libgui/FocusManager.h"
#include "../libgui/CursorManager.h"
#include "../libgui/MessageBox.h"

extern "C" {
#include "../gc_memory/memory.h"
#include "../main/plugin.h"
}

void Func_ShowRomInfo();
void Func_ResetROM();
void Func_LoadSave();
void Func_SaveGame();
void Func_StateManage();
void Func_ReturnFromCurrentRomFrame();

#define NUM_FRAME_BUTTONS 5
#define FRAME_BUTTONS currentRomFrameButtons
#define FRAME_STRINGS currentRomFrameStrings

static char FRAME_STRINGS[5][25] =
	{ "Show ROM Info",
	  "Restart Game",
	  "Load Save File",
	  "Save Game",
	  "State Management"};

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
	{	NULL,	FRAME_STRINGS[0],	150.0,	 80.0,	340.0,	40.0,	 4,	 1,	-1,	-1,	Func_ShowRomInfo,	Func_ReturnFromCurrentRomFrame }, // Show ROM Info
	{	NULL,	FRAME_STRINGS[1],	150.0,	160.0,	340.0,	40.0,	 0,	 2,	-1,	-1,	Func_ResetROM,		Func_ReturnFromCurrentRomFrame }, // Reset ROM
	{	NULL,	FRAME_STRINGS[2],	150.0,	240.0,	340.0,	40.0,	 1,	 3,	-1,	-1,	Func_LoadSave,		Func_ReturnFromCurrentRomFrame }, // Load Native Save
	{	NULL,	FRAME_STRINGS[3],	150.0,	320.0,	340.0,	40.0,	 2,	 4,	-1,	-1,	Func_SaveGame,		Func_ReturnFromCurrentRomFrame }, // Save Native Save
	{	NULL,	FRAME_STRINGS[4],	150.0,	400.0,	340.0,	40.0,	 3,	 0,	-1,	-1,	Func_StateManage,	Func_ReturnFromCurrentRomFrame }, // Save State Management
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
	pMenuContext->setActiveFrame(MenuContext::FRAME_LOADSAVE);
}

void Func_SaveGame()
{
	pMenuContext->setActiveFrame(MenuContext::FRAME_SAVEGAME);
}

void Func_StateManage()
{
	menu::MessageBox::getInstance().setMessage("Save states not implemented");
}

void Func_ReturnFromCurrentRomFrame()
{
	pMenuContext->setActiveFrame(MenuContext::FRAME_MAIN);
}
