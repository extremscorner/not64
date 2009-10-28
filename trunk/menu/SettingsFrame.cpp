#include "MenuContext.h"
#include "SettingsFrame.h"
#include "../libgui/Button.h"
#include "../libgui/TextBox.h"
#include "../libgui/resources.h"
#include "../libgui/FocusManager.h"
#include "../libgui/CursorManager.h"
#include "../libgui/MessageBox.h"
//#include "../main/timers.h"
#include "../main/wii64config.h"

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

void Func_ShowFpsOn();
void Func_ShowFpsOff();
void Func_ShowDebugOn();
void Func_ShowDebugOff();
void Func_Widescreen4_3();
void Func_Widescreen16_9();
void Func_2xSaiTexturesOn();
void Func_2xSaiTexturesOff();
void Func_FbTexturesOn();
void Func_FbTexturesOff();

void Func_ConfigureInput();
void Func_ConfigurePaks();

void Func_DisableAudioYes();
void Func_DisableAudioNo();

void Func_AutoLoadNativeYes();
void Func_AutoLoadNativeNo();
void Func_AutoSaveNativeYes();
void Func_AutoSaveNativeNo();
void Func_CopySaves();
void Func_DeleteSaves();
void Func_ReturnFromSettingsFrame();


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

#define NUM_FRAME_BUTTONS 33
#define NUM_TAB_BUTTONS 5
#define FRAME_BUTTONS settingsFrameButtons
#define FRAME_STRINGS settingsFrameStrings
#define NUM_FRAME_TEXTBOXES 11
#define FRAME_TEXTBOXES settingsFrameTextBoxes

static char FRAME_STRINGS[32][23] =
	{ "General",
	  "Video",
	  "Input",
	  "Audio",
	  "Saves",
	//Strings for General tab
	  "Native Saves Device",
	  "Save States Device",
	  "Select CPU Core",
	  "SD",
	  "USB",
	  "CardA",
	  "CardB",
	  "Pure Interp",
	  "Dynarec",
	//Strings for Video tab
	  "Show FPS",
	  "Show Debug",
	  "Screen Mode",
	  "2xSaI Tex",
	  "FB Textures",
	  "On",
	  "Off",
	  "4:3",
	  "16:9",
	//Strings for Input tab
	  "Configure Mappings",
	  "Configure Paks",
	//Strings for Audio tab
	  "Disable Audio",
	  "Yes",
	  "No",
	//Strings for Saves tab
	  "Auto Load Native Saves",
	  "Auto Save Native Saves",
	  "Copy Saves",
	  "Delete Saves"};

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
	//Buttons for Tabs (starts at button[0])
	{	NULL,	FRAME_STRINGS[0],	 25.0,	 30.0,	110.0,	40.0,	-1,	-1,	 4,	 1,	Func_TabGeneral,		Func_ReturnFromSettingsFrame }, // General tab
	{	NULL,	FRAME_STRINGS[1],	155.0,	 30.0,	100.0,	40.0,	-1,	-1,	 0,	 2,	Func_TabVideo,			Func_ReturnFromSettingsFrame }, // Video tab
	{	NULL,	FRAME_STRINGS[2],	275.0,	 30.0,	100.0,	40.0,	-1,	-1,	 1,	 3,	Func_TabInput,			Func_ReturnFromSettingsFrame }, // Input tab
	{	NULL,	FRAME_STRINGS[3],	395.0,	 30.0,	100.0,	40.0,	-1,	-1,	 2,	 4,	Func_TabAudio,			Func_ReturnFromSettingsFrame }, // Audio tab
	{	NULL,	FRAME_STRINGS[4],	515.0,	 30.0,	100.0,	40.0,	-1,	-1,	 3,	 0,	Func_TabSaves,			Func_ReturnFromSettingsFrame }, // Saves tab
	//Buttons for General Tab (starts at button[5])
	{	NULL,	FRAME_STRINGS[8],	295.0,	130.0,	 55.0,	40.0,	 0,	 9,	 8,	 6,	Func_NativeSaveSD,		Func_ReturnFromSettingsFrame }, // Native Save: SD
	{	NULL,	FRAME_STRINGS[9],	360.0,	130.0,	 70.0,	40.0,	 0,	10,	 5,	 7,	Func_NativeSaveUSB,		Func_ReturnFromSettingsFrame }, // Native Save: USB
	{	NULL,	FRAME_STRINGS[10],	440.0,	130.0,	 90.0,	40.0,	 0,	10,	 6,	 8,	Func_NativeSaveCardA,	Func_ReturnFromSettingsFrame }, // Native Save: Card A
	{	NULL,	FRAME_STRINGS[11],	540.0,	130.0,	 90.0,	40.0,	 0,	10,	 7,	 5,	Func_NativeSaveCardB,	Func_ReturnFromSettingsFrame }, // Native Save: Card B
	{	NULL,	FRAME_STRINGS[8],	295.0,	230.0,	 55.0,	40.0,	 5,	11,	10,	10,	Func_SaveStateSD,		Func_ReturnFromSettingsFrame }, // Save State: SD
	{	NULL,	FRAME_STRINGS[9],	360.0,	230.0,	 70.0,	40.0,	 6,	11,	 9,	 9,	Func_SaveStateUSB,		Func_ReturnFromSettingsFrame }, // Save State: USB
	{	NULL,	FRAME_STRINGS[12],	295.0,	330.0,	160.0,	40.0,	 9,	 0,	12,	12,	Func_CpuPureInterp,		Func_ReturnFromSettingsFrame }, // CPU: Pure Interp
	{	NULL,	FRAME_STRINGS[13],	465.0,	330.0,	130.0,	40.0,	10,	 0,	11,	11,	Func_CpuDynarec,		Func_ReturnFromSettingsFrame }, // CPU: Dynarec
	//Buttons for Video Tab (starts at button[13])
	{	NULL,	FRAME_STRINGS[19],	325.0,	100.0,	 75.0,	40.0,	 1,	15,	14,	14,	Func_ShowFpsOn,			Func_ReturnFromSettingsFrame }, // Show FPS: On
	{	NULL,	FRAME_STRINGS[20],	420.0,	100.0,	 75.0,	40.0,	 1,	16,	13,	13,	Func_ShowFpsOff,		Func_ReturnFromSettingsFrame }, // Show FPS: Off
	{	NULL,	FRAME_STRINGS[19],	325.0,	170.0,	 75.0,	40.0,	13,	17,	16,	16,	Func_ShowDebugOn,		Func_ReturnFromSettingsFrame }, // Show Debug: On
	{	NULL,	FRAME_STRINGS[20],	420.0,	170.0,	 75.0,	40.0,	14,	18,	15,	15,	Func_ShowDebugOff,		Func_ReturnFromSettingsFrame }, // Show Debug: Off
	{	NULL,	FRAME_STRINGS[21],	325.0,	240.0,	 75.0,	40.0,	15,	19,	18,	18,	Func_Widescreen4_3,		Func_ReturnFromSettingsFrame }, // Widescreen: 4:3
	{	NULL,	FRAME_STRINGS[22],	420.0,	240.0,	 75.0,	40.0,	16,	20,	17,	17,	Func_Widescreen16_9,	Func_ReturnFromSettingsFrame }, // Widescreen: 16:9
	{	NULL,	FRAME_STRINGS[19],	325.0,	310.0,	 75.0,	40.0,	17,	21,	20,	20,	Func_2xSaiTexturesOn,	Func_ReturnFromSettingsFrame }, // 2xSai: On
	{	NULL,	FRAME_STRINGS[20],	420.0,	310.0,	 75.0,	40.0,	18,	22,	19,	19,	Func_2xSaiTexturesOff,	Func_ReturnFromSettingsFrame }, // 2xSai: Off
	{	NULL,	FRAME_STRINGS[19],	325.0,	380.0,	 75.0,	40.0,	19,	 1,	22,	22,	Func_FbTexturesOn,		Func_ReturnFromSettingsFrame }, // FbTex: On
	{	NULL,	FRAME_STRINGS[20],	420.0,	380.0,	 75.0,	40.0,	20,	 1,	21,	21,	Func_FbTexturesOff,		Func_ReturnFromSettingsFrame }, // FbTex: Off
	//Buttons for Input Tab (starts at button[23])
	{	NULL,	FRAME_STRINGS[23],	180.0,	150.0,	280.0,	40.0,	 2,	24,	-1,	-1,	Func_ConfigureInput,	Func_ReturnFromSettingsFrame }, // Show FPS: On
	{	NULL,	FRAME_STRINGS[24],	180.0,	250.0,	280.0,	40.0,	23,	 2,	-1,	-1,	Func_ConfigurePaks,		Func_ReturnFromSettingsFrame }, // Show FPS: Off
	//Buttons for Audio Tab (starts at button[25])
	{	NULL,	FRAME_STRINGS[26],	345.0,	150.0,	 75.0,	40.0,	 3,	 3,	26,	26,	Func_DisableAudioYes,	Func_ReturnFromSettingsFrame }, // Disable Audio: Yes
	{	NULL,	FRAME_STRINGS[27],	440.0,	150.0,	 75.0,	40.0,	 3,	 3,	25,	25,	Func_DisableAudioNo,	Func_ReturnFromSettingsFrame }, // Disable Audio: No
	//Buttons for Saves Tab (starts at button[27])
	{	NULL,	FRAME_STRINGS[26],	375.0,	100.0,	 75.0,	40.0,	 4,	29,	28,	28,	Func_AutoLoadNativeYes,	Func_ReturnFromSettingsFrame }, // Auto Load Native: Yes
	{	NULL,	FRAME_STRINGS[27],	470.0,	100.0,	 75.0,	40.0,	 4,	30,	27,	27,	Func_AutoLoadNativeNo,	Func_ReturnFromSettingsFrame }, // Auto LoadNative: No
	{	NULL,	FRAME_STRINGS[26],	375.0,	170.0,	 75.0,	40.0,	27,	31,	30,	30,	Func_AutoSaveNativeYes,	Func_ReturnFromSettingsFrame }, // Auto Save Native: Yes
	{	NULL,	FRAME_STRINGS[27],	470.0,	170.0,	 75.0,	40.0,	28,	31,	29,	29,	Func_AutoSaveNativeNo,	Func_ReturnFromSettingsFrame }, // Auto Save Native: No
	{	NULL,	FRAME_STRINGS[30],	375.0,	240.0,	170.0,	40.0,	29,	32,	-1,	-1,	Func_CopySaves,			Func_ReturnFromSettingsFrame }, // Copy Saves
	{	NULL,	FRAME_STRINGS[31],	375.0,	310.0,	170.0,	40.0,	31,	 4,	-1,	-1,	Func_DeleteSaves,		Func_ReturnFromSettingsFrame }, // Delete Saves
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
	{	NULL,	FRAME_STRINGS[5],	155.0,	150.0,	 1.0,	true }, // Native Save Device: SD/USB/CardA/CardB
	{	NULL,	FRAME_STRINGS[6],	155.0,	250.0,	 1.0,	true }, // Save State Device: SD/USB
	{	NULL,	FRAME_STRINGS[7],	155.0,	350.0,	 1.0,	true }, // CPU Core: Pure Interp/Dynarec
	//TextBoxes for Video Tab (starts at textBox[3])
	{	NULL,	FRAME_STRINGS[14],	190.0,	120.0,	 1.0,	true }, // Show FPS: On/Off
	{	NULL,	FRAME_STRINGS[15],	190.0,	190.0,	 1.0,	true }, // Show DEBUG: On/Off
	{	NULL,	FRAME_STRINGS[16],	190.0,	260.0,	 1.0,	true }, // Widescreen: 4x3/16x9
	{	NULL,	FRAME_STRINGS[17],	190.0,	330.0,	 1.0,	true }, // 2xSai: On/Off
	{	NULL,	FRAME_STRINGS[18],	190.0,	400.0,	 1.0,	true }, // FBTex: On/Off
	//TextBoxes for Input Tab (starts at textBox[])
	//TextBoxes for Audio Tab (starts at textBox[8])
	{	NULL,	FRAME_STRINGS[25],	210.0,	170.0,	 1.0,	true }, // Disable Audio: Yes/No
	//TextBoxes for Saves Tab (starts at textBox[9])
	{	NULL,	FRAME_STRINGS[28],	200.0,	120.0,	 1.0,	true }, // Auto Load Native Save: Yes/No
	{	NULL,	FRAME_STRINGS[29],	200.0,	190.0,	 1.0,	true }, // Auto Save Native Save: Yes/No
};

SettingsFrame::SettingsFrame()
		: activeSubmenu(SUBMENU_GENERAL)
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
		FRAME_BUTTONS[i].button->setSelectedImage(buttonFocusImage);
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
	for (int i = 0; i < NUM_FRAME_BUTTONS; i++)
	{
		menu::Cursor::getInstance().removeComponent(this, FRAME_BUTTONS[i].button);
		delete FRAME_BUTTONS[i].button;
	}
	delete buttonFocusImage;
	delete buttonImage;

}

void SettingsFrame::activateSubmenu(int submenu)
{
	activeSubmenu = submenu;

	//All buttons: hide; unselect
	for (int i = 0; i < NUM_FRAME_BUTTONS; i++)
	{
		FRAME_BUTTONS[i].button->setVisible(false);
		FRAME_BUTTONS[i].button->setSelected(false);
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
				FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_UP, FRAME_BUTTONS[11].button);
			}
			for (int i = 0; i < 3; i++)
				FRAME_TEXTBOXES[i].textBox->setVisible(true);
			FRAME_BUTTONS[0].button->setSelected(true);
			FRAME_BUTTONS[5+nativeSaveDevice].button->setSelected(true);
			FRAME_BUTTONS[9+saveStateDevice].button->setSelected(true);
			if (dynacore == DYNACORE_PURE_INTERP)	FRAME_BUTTONS[11].button->setSelected(true);
			else									FRAME_BUTTONS[12].button->setSelected(true);
			for (int i = 5; i < 13; i++)
				FRAME_BUTTONS[i].button->setVisible(true);
			break;
		case SUBMENU_VIDEO:
			setDefaultFocus(FRAME_BUTTONS[1].button);
			for (int i = 0; i < NUM_TAB_BUTTONS; i++)
			{
				FRAME_BUTTONS[i].button->setVisible(true);
				FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_DOWN, FRAME_BUTTONS[13].button);
				FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_UP, FRAME_BUTTONS[21].button);
			}
			for (int i = 3; i < 8; i++)
				FRAME_TEXTBOXES[i].textBox->setVisible(true);
			FRAME_BUTTONS[1].button->setSelected(true);
			if (showFPSonScreen == FPS_SHOW)	FRAME_BUTTONS[13].button->setSelected(true);
			else								FRAME_BUTTONS[14].button->setSelected(true);
			if (printToScreen == DEBUG_SHOW)	FRAME_BUTTONS[15].button->setSelected(true);
			else								FRAME_BUTTONS[16].button->setSelected(true);
			if (widescreen == SCREENMODE_4x3)	FRAME_BUTTONS[17].button->setSelected(true);
			else								FRAME_BUTTONS[18].button->setSelected(true);
			if (glN64_use2xSaiTextures == GLN64_2XSAI_ENABLE)	FRAME_BUTTONS[19].button->setSelected(true);
			else												FRAME_BUTTONS[20].button->setSelected(true);
			if (glN64_useFrameBufferTextures == GLN64_FBTEX_ENABLE)	FRAME_BUTTONS[21].button->setSelected(true);
			else													FRAME_BUTTONS[22].button->setSelected(true);
			for (int i = 13; i < 23; i++)
				FRAME_BUTTONS[i].button->setVisible(true);
			break;
		case SUBMENU_INPUT:
			setDefaultFocus(FRAME_BUTTONS[2].button);
			for (int i = 0; i < NUM_TAB_BUTTONS; i++)
			{
				FRAME_BUTTONS[i].button->setVisible(true);
				FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_DOWN, FRAME_BUTTONS[23].button);
				FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_UP, FRAME_BUTTONS[24].button);
			}
			FRAME_BUTTONS[2].button->setSelected(true);
			for (int i = 23; i < 25; i++)
				FRAME_BUTTONS[i].button->setVisible(true);
			break;
		case SUBMENU_AUDIO:
			setDefaultFocus(FRAME_BUTTONS[3].button);
			for (int i = 0; i < NUM_TAB_BUTTONS; i++)
			{
				FRAME_BUTTONS[i].button->setVisible(true);
				FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_DOWN, FRAME_BUTTONS[25].button);
				FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_UP, FRAME_BUTTONS[25].button);
			}
			for (int i = 8; i < 9; i++)
				FRAME_TEXTBOXES[i].textBox->setVisible(true);
			FRAME_BUTTONS[3].button->setSelected(true);
			if (audioEnabled == AUDIO_DISABLE)	FRAME_BUTTONS[25].button->setSelected(true);
			else								FRAME_BUTTONS[26].button->setSelected(true);
			for (int i = 25; i < 27; i++)
				FRAME_BUTTONS[i].button->setVisible(true);
			break;
		case SUBMENU_SAVES:
			setDefaultFocus(FRAME_BUTTONS[4].button);
			for (int i = 0; i < NUM_TAB_BUTTONS; i++)
			{
				FRAME_BUTTONS[i].button->setVisible(true);
				FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_DOWN, FRAME_BUTTONS[27].button);
				FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_UP, FRAME_BUTTONS[32].button);
			}
			for (int i = 9; i < 11; i++)
				FRAME_TEXTBOXES[i].textBox->setVisible(true);
			FRAME_BUTTONS[4].button->setSelected(true);
			if (autoLoad == AUTOLOAD_ENABLE)	FRAME_BUTTONS[27].button->setSelected(true);
			else								FRAME_BUTTONS[28].button->setSelected(true);
			if (autoSave == AUTOSAVE_ENABLE)	FRAME_BUTTONS[29].button->setSelected(true);
			else								FRAME_BUTTONS[30].button->setSelected(true);
			for (int i = 27; i < NUM_FRAME_BUTTONS; i++)
				FRAME_BUTTONS[i].button->setVisible(true);
			break;
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

void Func_ShowFpsOn()
{
	for (int i = 13; i <= 14; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[13].button->setSelected(true);
	showFPSonScreen = FPS_SHOW;
}

void Func_ShowFpsOff()
{
	for (int i = 13; i <= 14; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[14].button->setSelected(true);
	showFPSonScreen = FPS_HIDE;
}

void Func_ShowDebugOn()
{
	for (int i = 15; i <= 16; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[15].button->setSelected(true);
	printToScreen = DEBUG_SHOW;
}

void Func_ShowDebugOff()
{
	for (int i = 15; i <= 16; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[16].button->setSelected(true);
	printToScreen = DEBUG_HIDE;
}

void Func_Widescreen4_3()
{
	for (int i = 17; i <= 18; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[17].button->setSelected(true);
	widescreen = SCREENMODE_4x3;
}

void Func_Widescreen16_9()
{
	for (int i = 17; i <= 18; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[18].button->setSelected(true);
	widescreen = SCREENMODE_16x9;
}

void Func_2xSaiTexturesOn()
{
	for (int i = 19; i <= 20; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[19].button->setSelected(true);
	glN64_use2xSaiTextures = GLN64_2XSAI_ENABLE;
}

void Func_2xSaiTexturesOff()
{
	for (int i = 19; i <= 20; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[20].button->setSelected(true);
	glN64_use2xSaiTextures = GLN64_2XSAI_DISABLE;
}

void Func_FbTexturesOn()
{
	for (int i = 21; i <= 22; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[21].button->setSelected(true);
	glN64_useFrameBufferTextures = GLN64_FBTEX_ENABLE;
}

void Func_FbTexturesOff()
{
	for (int i = 21; i <= 22; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[22].button->setSelected(true);
	glN64_useFrameBufferTextures = GLN64_FBTEX_DISABLE;
}

void Func_ConfigureInput()
{
	menu::MessageBox::getInstance().setMessage("Input configuration not implemented");
}

void Func_ConfigurePaks()
{
	menu::MessageBox::getInstance().setMessage("Controller Paks not implemented");
}

void Func_DisableAudioYes()
{
	for (int i = 25; i <= 26; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[25].button->setSelected(true);
	audioEnabled = AUTOLOAD_DISABLE;
}

void Func_DisableAudioNo()
{
	for (int i = 25; i <= 26; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[26].button->setSelected(true);
	audioEnabled = AUTOLOAD_ENABLE;
}

void Func_AutoLoadNativeYes()
{
	for (int i = 27; i <= 28; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[27].button->setSelected(true);
	autoLoad = AUTOLOAD_ENABLE;
}

void Func_AutoLoadNativeNo()
{
	for (int i = 27; i <= 28; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[28].button->setSelected(true);
	autoLoad = AUTOLOAD_DISABLE;
}

void Func_AutoSaveNativeYes()
{
	for (int i = 29; i <= 30; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[29].button->setSelected(true);
	autoSave = AUTOSAVE_ENABLE;
}

void Func_AutoSaveNativeNo()
{
	for (int i = 29; i <= 30; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[30].button->setSelected(true);
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
	pMenuContext->setActiveFrame(MenuContext::FRAME_MAIN);
}

/*extern timers Timers;

void Func_ToggleVILimit()
{
	Timers.limitVIs = (Timers.limitVIs+1) % 3;
	FRAME_BUTTONS[2].buttonString = FRAME_STRINGS[Timers.limitVIs + 4];
}*/
