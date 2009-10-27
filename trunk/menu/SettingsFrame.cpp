#include "MenuContext.h"
#include "SettingsFrame.h"
#include "../libgui/Button.h"
#include "../libgui/resources.h"
#include "../libgui/FocusManager.h"
#include "../libgui/CursorManager.h"
#include "../main/timers.h"


void Func_ToggleFPS();
void Func_ToggleScreenDebug();
void Func_ToggleVILimit();
void Func_ToggleSDDebug();
void Func_ToggleGlN64useFbTex();
void Func_ReturnFromSettingsFrame();

#define NUM_FRAME_BUTTONS 4
#define FRAME_BUTTONS settingsFrameButtons
#define FRAME_STRINGS settingsFrameStrings

static char FRAME_STRINGS[11][28] =
	{ "Show FPS: Off",
	  "Show FPS: On",
	  "Debug on Screen: Off",
	  "Debug on Screen: On",
	  "Debug to SD: file Closed",
	  "Debug to SD: file Open",
	  "VI Limit: disabled",
	  "VI Limit: enabled",
	  "VI Limit: wait for frame",
	  "FB Textures: disabled",
	  "FB Textures: enabled"};

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
{ //	button	buttonString		x		y		width	height	Up	Dwn	Lft	Rt	clickFunc					returnFunc
	{	NULL,	FRAME_STRINGS[1],	150.0,	 50.0,	340.0,	40.0,	 3,	 1,	-1,	-1,	Func_ToggleFPS,				Func_ReturnFromSettingsFrame }, // 
	{	NULL,	FRAME_STRINGS[3],	150.0,	150.0,	340.0,	40.0,	 0,	 2,	-1,	-1,	Func_ToggleScreenDebug,		Func_ReturnFromSettingsFrame }, // 
	{	NULL,	FRAME_STRINGS[7],	150.0,	250.0,	340.0,	40.0,	 1,	 3,	-1,	-1,	Func_ToggleVILimit,			Func_ReturnFromSettingsFrame }, // 
	{	NULL,	FRAME_STRINGS[9],	150.0,	350.0,	340.0,	40.0,	 2,	 0,	-1,	-1,	Func_ToggleGlN64useFbTex,	Func_ReturnFromSettingsFrame }, // 
};

SettingsFrame::SettingsFrame()
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
	setBackFunc(Func_ReturnFromSettingsFrame);
	setEnabled(true);

}

SettingsFrame::~SettingsFrame()
{
	for (int i = 0; i < NUM_FRAME_BUTTONS; i++)
	{
		menu::Cursor::getInstance().removeComponent(this, FRAME_BUTTONS[i].button);
		delete FRAME_BUTTONS[i].button;
	}
	delete buttonFocusImage;
	delete buttonImage;

}

extern char showFPSonScreen;

void Func_ToggleFPS()
{
	showFPSonScreen ^= 1;
	if (showFPSonScreen) FRAME_BUTTONS[0].buttonString = FRAME_STRINGS[1];
	else FRAME_BUTTONS[0].buttonString = FRAME_STRINGS[0];
}

extern char printToScreen;

void Func_ToggleScreenDebug()
{
	printToScreen ^= 1;
	if (printToScreen) FRAME_BUTTONS[1].buttonString = FRAME_STRINGS[3];
	else FRAME_BUTTONS[1].buttonString = FRAME_STRINGS[2];
}

extern timers Timers;

void Func_ToggleVILimit()
{
	Timers.limitVIs = (Timers.limitVIs+1) % 3;
	FRAME_BUTTONS[2].buttonString = FRAME_STRINGS[Timers.limitVIs + 6];
}

extern char printToSD;

void Func_ToggleSDDebug()
{
	//This button is not implemented yet
/*	printToSD ^= 1;
	if(printToSD)
		DEBUG_print("open",DBG_SDGECKOOPEN);
	else
		DEBUG_print("close",DBG_SDGECKOCLOSE);
	devFeatures_submenu[NUM_DEV_STD].caption = &toggleSDDebug_strings[printToSD][0];*/
}

extern char glN64_useFrameBufferTextures;

void Func_ToggleGlN64useFbTex()
{
	glN64_useFrameBufferTextures ^= 1;
	if (glN64_useFrameBufferTextures) FRAME_BUTTONS[3].buttonString = FRAME_STRINGS[10];
	else FRAME_BUTTONS[3].buttonString = FRAME_STRINGS[9];
}

extern MenuContext *pMenuContext;

void Func_ReturnFromSettingsFrame()
{
	pMenuContext->setActiveFrame(MenuContext::FRAME_MAIN);
}
