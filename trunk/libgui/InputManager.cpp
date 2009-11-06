#include "InputManager.h"
#include "FocusManager.h"
#include "CursorManager.h"

void ShutdownWii();



namespace menu {
	
Input::Input()
{
	PAD_Init();
#ifdef HW_RVL
	CONF_Init();
	WPAD_Init();
	WPAD_SetIdleTimeout(120);
	WPAD_SetVRes(WPAD_CHAN_ALL, 640, 480);
	WPAD_SetDataFormat(WPAD_CHAN_ALL, WPAD_FMT_BTNS_ACC_IR); 
	WPAD_SetPowerButtonCallback((WPADShutdownCallback)ShutdownWii);
	SYS_SetPowerCallback(ShutdownWii);

#endif
//	VIDEO_SetPostRetraceCallback (PAD_ScanPads);
}

Input::~Input()
{
}

void Input::refreshInput()
{
	PAD_ScanPads();
	PAD_Read(gcPad);
	PAD_Clamp(gcPad);
#ifdef HW_RVL
	for (int i = WPAD_CHAN_0; i < WPAD_MAX_WIIMOTES; i++)
		wiiPadError[i] = WPAD_ReadEvent(i, &wiiPad[i]);
#endif
}

#ifdef HW_RVL
WPADData* Input::getWpad()
{
	return &wiiPad[0];
}

int* Input::getWpadError()
{
	return &wiiPadError[0];
}
#endif

PADStatus* Input::getPad()
{
	return &gcPad[0];
}

void Input::clearInputData()
{
	Focus::getInstance().clearInputData();
	Cursor::getInstance().clearInputData();
}

} //namespace menu 