#include "MenuContext.h"
#include "MainFrame.h"
#include "../libgui/Button.h"
#include "../libgui/resources.h"
#include "../libgui/FocusManager.h"
#include "../libgui/CursorManager.h"
#include "../libgui/MessageBox.h"
#ifdef DEBUGON
# include <debug.h>
#endif
extern "C" {
#ifdef WII
#include <di/di.h>
#endif 
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

static char FRAME_STRINGS[14][20] =
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
{ //	button	buttonString			x		y		width	height	Up	Dwn	Lft	Rt	clickFunc				returnFunc
	{	NULL,	FRAME_STRINGS[0],	 45.0,	 50.0,	250.0,	40.0,	 5,	 1,	-1,	-1,	Func_LoadROM,			NULL }, // Load ROM
	{	NULL,	FRAME_STRINGS[1],	 45.0,	110.0,	250.0,	40.0,	 0,	 2,	-1,	-1,	Func_CurrentROM,		NULL }, // Current ROM
	{	NULL,	FRAME_STRINGS[2],	 45.0,	170.0,	250.0,	40.0,	 1,	 3,	-1,	-1,	Func_Settings,			NULL }, // Settings
	{	NULL,	FRAME_STRINGS[3],	 45.0,	230.0,	250.0,	40.0,	 2,	 4,	-1,	-1,	Func_Credits,			NULL }, // Credits
	{	NULL,	FRAME_STRINGS[4],	 45.0,	290.0,	250.0,	40.0,	 3,	 5,	-1,	-1,	Func_ExitToLoader,		NULL }, // Exit to Loader
	{	NULL,	FRAME_STRINGS[5],	 45.0,	350.0,	250.0,	40.0,	 4,	 0,	-1,	-1,	Func_PlayGame,			NULL }, // Play/Resume Game
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

void Func_CurrentROM()
{
	pMenuContext->setActiveFrame(MenuContext::FRAME_CURRENTROM);
}

void Func_Settings()
{
	pMenuContext->setActiveFrame(MenuContext::FRAME_SETTINGS);
}

void Func_Credits()
{
	menu::MessageBox::getInstance().setMessage("Credits not implemented");
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

extern BOOL hasLoadedROM;

extern "C" {
void pauseAudio(void);  void pauseInput(void);
void resumeAudio(void); void resumeInput(void);
void go(void); 
}

void Func_PlayGame()
{
	if(!hasLoadedROM)
	{
		menu::MessageBox::getInstance().setMessage("Please load a ROM first");
		return;
	}
	
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

	menu::Cursor::getInstance().clearCursorFocus();
}
