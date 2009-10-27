#include "MenuContext.h"
#include "SelectCPUFrame.h"
#include "../libgui/Button.h"
#include "../libgui/resources.h"
#include "../libgui/FocusManager.h"
#include "../libgui/CursorManager.h"
#include "../libgui/MessageBox.h"

void Func_ChoosePureInterp();
void Func_ChooseDynarec();
void Func_ReturnFromSelectCPUFrame();

#define NUM_FRAME_BUTTONS 2
#define FRAME_BUTTONS selectCPUFrameButtons
#define FRAME_STRINGS selectCPUFrameStrings

static char FRAME_STRINGS[2][19] =
	{ "Pure Interpreter",
	  "Dynamic Recompiler"};

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
{ //	button	buttonString		x		y		width	height	Up	Dwn	Lft	Rt	clickFunc				returnFunc
	{	NULL,	FRAME_STRINGS[0],	150.0,	150.0,	340.0,	40.0,	 1,	 1,	-1,	-1,	Func_ChoosePureInterp,	Func_ReturnFromSelectCPUFrame }, // Pure Interpreter
	{	NULL,	FRAME_STRINGS[1],	150.0,	250.0,	340.0,	40.0,	 0,	 0,	-1,	-1,	Func_ChooseDynarec,		Func_ReturnFromSelectCPUFrame }, // Dynarec
};

SelectCPUFrame::SelectCPUFrame()
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
	setBackFunc(Func_ReturnFromSelectCPUFrame);
	setEnabled(true);

}

SelectCPUFrame::~SelectCPUFrame()
{
	for (int i = 0; i < NUM_FRAME_BUTTONS; i++)
	{
		menu::Cursor::getInstance().removeComponent(this, FRAME_BUTTONS[i].button);
		delete FRAME_BUTTONS[i].button;
	}
	delete buttonFocusImage;
	delete buttonImage;

}

extern "C" {
void cpu_init();
void cpu_deinit();
}
extern BOOL hasLoadedROM;
extern unsigned long dynacore;
extern MenuContext *pMenuContext;

void Func_ChoosePureInterp()
{
	int needInit = 0;
	if(hasLoadedROM && dynacore != 2){ cpu_deinit(); needInit = 1; }
	dynacore = 2;
	if(hasLoadedROM && needInit) cpu_init();
	menu::MessageBox::getInstance().setMessage("Running Pure Interpreter Mode");
	pMenuContext->setActiveFrame(MenuContext::FRAME_MAIN);
}

void Func_ChooseDynarec()
{
	int needInit = 0;
	if(hasLoadedROM && dynacore != 1){ cpu_deinit(); needInit = 1; }
	dynacore = 1;
	if(hasLoadedROM && needInit) cpu_init();
	menu::MessageBox::getInstance().setMessage("Running Dynarec Mode");
	pMenuContext->setActiveFrame(MenuContext::FRAME_MAIN);
}

void Func_ReturnFromSelectCPUFrame()
{
	pMenuContext->setActiveFrame(MenuContext::FRAME_SETTINGS);
}
