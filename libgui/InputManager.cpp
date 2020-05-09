/**
 * Wii64 - InputManager.cpp
 * Copyright (C) 2009 sepp256
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

#include "InputManager.h"
#include "FocusManager.h"
#include "CursorManager.h"
#include "../main/wii64config.h"

extern char shutdown;
extern int stop;

namespace menu {

void PowerCallback();
void ResetCallback();

Input::Input()
{
	PAD_SetSamplingRate(pollRate);
	PAD_Init();
#ifdef HW_RVL
	WPAD_Init();
	WPAD_SetVRes(WPAD_CHAN_ALL, 640, 480);
	WPAD_SetDataFormat(WPAD_CHAN_ALL, WPAD_FMT_BTNS_ACC_IR); 
	SYS_SetPowerCallback(PowerCallback);
#endif
	SYS_SetResetCallback(ResetCallback);
}

Input::~Input()
{
}

void Input::refreshInput()
{
	PAD_ScanPads();
#ifdef HW_RVL
	WPAD_ScanPads();
#endif
}

#ifdef HW_RVL
WPADData* Input::getWpad()
{
	return WPAD_Data(0);
}
#endif

void Input::clearInputData()
{
	Focus::getInstance().clearInputData();
	Cursor::getInstance().clearInputData();
}

void PowerCallback()
{
	shutdown = 1;
	stop = 1;
}

void ResetCallback()
{
	stop = 1;
	PAD_Recalibrate(PAD_CHAN0_BIT | PAD_CHAN1_BIT | PAD_CHAN2_BIT | PAD_CHAN3_BIT);
}

} //namespace menu 
