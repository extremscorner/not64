#include "MenuContext.h"
#include "SettingsFrame.h"
#include "../libgui/Button.h"
#include "../libgui/resources.h"
#include "../libgui/FocusManager.h"
#include "../libgui/CursorManager.h"
#include "../libgui/MessageBox.h"
#include "../main/timers.h"

void Func_ToggleFPS();
void Func_ToggleScreenDebug();
void Func_ToggleVILimit();
void Func_ToggleGlN64useFbTex();
void Func_ToggleGlN64use2xSai();
void Func_ToggleWidescreen();
void Func_SelectCPU();
void Func_ConfigureInput();
void Func_ConfigurePaks();
void Func_ToggleAudio();
void Func_ToggleAutoLoad();
void Func_ToggleAutoSave();
void Func_ReturnFromSettingsFrame();

#define NUM_FRAME_BUTTONS 12
#define FRAME_BUTTONS settingsFrameButtons
#define FRAME_STRINGS settingsFrameStrings

static char FRAME_STRINGS[22][28] =
	{ "Show FPS: Off",
	  "Show FPS: On",
	  "Debug on Screen: Off",
	  "Debug on Screen: On",
	  "VI Limit: disabled",
	  "VI Limit: enabled",
	  "VI Limit: wait for frame",
	  "FB Textures: disabled",
	  "FB Textures: enabled",
	  "2xSaI Textures: Off",
	  "2xSaI Textures: On",
	  "Widescreen: 4:3",
	  "Widescreen: 16:9",
	  "Select CPU Core",
	  "Configure Input",
	  "Configure Paks",
	  "Audio: Disabled",
	  "Audio: Enabled",
	  "Auto Load Saves: Off",
	  "Auto Load Saves: On",
	  "Auto Save Saves: Off",
	  "Auto Save Saves: On"};

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
	{	NULL,	FRAME_STRINGS[1],	 45.0,	 50.0,	250.0,	40.0,	 5,	 1,	 6,	 6,	Func_ToggleFPS,				Func_ReturnFromSettingsFrame }, // Toggle FPS
	{	NULL,	FRAME_STRINGS[2],	 45.0,	110.0,	250.0,	40.0,	 0,	 2,	 7,	 7,	Func_ToggleScreenDebug,		Func_ReturnFromSettingsFrame }, // Toggle Debug
	{	NULL,	FRAME_STRINGS[4],	 45.0,	170.0,	250.0,	40.0,	 1,	 3,	 8,	 8,	Func_ToggleVILimit,			Func_ReturnFromSettingsFrame }, // Toggle VI limit
	{	NULL,	FRAME_STRINGS[7],	 45.0,	230.0,	250.0,	40.0,	 2,	 4,	 9,	 9,	Func_ToggleGlN64useFbTex,	Func_ReturnFromSettingsFrame }, // Toggle FB Tex
	{	NULL,	FRAME_STRINGS[9],	 45.0,	290.0,	250.0,	40.0,	 3,	 5,	10,	10,	Func_ToggleGlN64use2xSai,	Func_ReturnFromSettingsFrame }, // Toggle 2xSai
	{	NULL,	FRAME_STRINGS[11],	 45.0,	350.0,	250.0,	40.0,	 4,	 0,	11,	11,	Func_ToggleWidescreen,		Func_ReturnFromSettingsFrame }, // Toggle Widescreen
//	{	NULL,	FRAME_STRINGS[6],	 45.0,	410.0,	250.0,	40.0,	 5,	 0,	-1,	-1,	Func_StopDVD,				NULL }, // Stop/Swap DVD
	{	NULL,	FRAME_STRINGS[13],	345.0,	 50.0,	250.0,	40.0,	11,	 7,	 0,	 0,	Func_SelectCPU,				Func_ReturnFromSettingsFrame }, // Select CPU
	{	NULL,	FRAME_STRINGS[14],	345.0,	110.0,	250.0,	40.0,	 6,	 8,	 1,	 1,	Func_ConfigureInput,		Func_ReturnFromSettingsFrame }, // Configure Input
	{	NULL,	FRAME_STRINGS[15],	345.0,	170.0,	250.0,	40.0,	 7,	 9,	 2,	 2,	Func_ConfigurePaks,			Func_ReturnFromSettingsFrame }, // Configure Paks
	{	NULL,	FRAME_STRINGS[17],	345.0,	230.0,	250.0,	40.0,	 8,	10,	 3,	 3,	Func_ToggleAudio,			Func_ReturnFromSettingsFrame }, // Toggle Audio
	{	NULL,	FRAME_STRINGS[18],	345.0,	290.0,	250.0,	40.0,	 9,	11,	 4,	 4,	Func_ToggleAutoLoad,		Func_ReturnFromSettingsFrame }, // Toggle Auto Load
	{	NULL,	FRAME_STRINGS[20],	345.0,	350.0,	250.0,	40.0,	10,	 7,	 5,	 5,	Func_ToggleAutoSave,		Func_ReturnFromSettingsFrame }, // Toggle Auto Save
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
	FRAME_BUTTONS[2].buttonString = FRAME_STRINGS[Timers.limitVIs + 4];
}

extern char glN64_useFrameBufferTextures;

void Func_ToggleGlN64useFbTex()
{
	glN64_useFrameBufferTextures ^= 1;
	if (glN64_useFrameBufferTextures) FRAME_BUTTONS[3].buttonString = FRAME_STRINGS[8];
	else FRAME_BUTTONS[3].buttonString = FRAME_STRINGS[7];
}

extern char glN64_use2xSaiTextures;

void Func_ToggleGlN64use2xSai()
{
	glN64_use2xSaiTextures ^= 1;
	if (glN64_use2xSaiTextures) FRAME_BUTTONS[4].buttonString = FRAME_STRINGS[10];
	else FRAME_BUTTONS[4].buttonString = FRAME_STRINGS[9];
}

extern char widescreen;

void Func_ToggleWidescreen()
{
	widescreen ^= 1;
	if (widescreen) FRAME_BUTTONS[5].buttonString = FRAME_STRINGS[12];
	else FRAME_BUTTONS[5].buttonString = FRAME_STRINGS[11];
}

extern MenuContext *pMenuContext;

void Func_SelectCPU()
{
	pMenuContext->setActiveFrame(MenuContext::FRAME_SELECTCPU);
}

void Func_ConfigureInput()
{
	menu::MessageBox::getInstance().setMessage("Input configuration not implemented");
}

void Func_ConfigurePaks()
{
	menu::MessageBox::getInstance().setMessage("Controller Paks not implemented");
}

extern char  audioEnabled;

void Func_ToggleAudio()
{
	audioEnabled ^= 1;
	if (audioEnabled) FRAME_BUTTONS[9].buttonString = FRAME_STRINGS[17];
	else FRAME_BUTTONS[9].buttonString = FRAME_STRINGS[16];
}

void Func_ToggleAutoLoad()
{
	menu::MessageBox::getInstance().setMessage("Auto Load not implemented");
}

void Func_ToggleAutoSave()
{
	menu::MessageBox::getInstance().setMessage("Auto Save not implemented");
}

void Func_ReturnFromSettingsFrame()
{
	pMenuContext->setActiveFrame(MenuContext::FRAME_MAIN);
}
