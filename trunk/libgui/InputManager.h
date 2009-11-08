/**
 * Wii64 - InputManager.h
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

#ifndef INPUTMANAGER_H
#define INPUTMANAGER_H

#include "GuiTypes.h"

namespace menu {

class Input
{
public:
	void refreshInput();
#ifdef HW_RVL
	WPADData* getWpad();
#endif
	PADStatus* getPad();
	void clearInputData();
	static Input& getInstance()
	{
		static Input obj;
		return obj;
	}

private:
	Input();
	~Input();
	PADStatus gcPad[4];
#ifdef HW_RVL
	WPADData *wiiPad;
#endif

};

} //namespace menu 

#endif
