/**
 * Wii64 - SettingsFrame.h
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

#ifndef SETTINGSFRAME_H
#define SETTINGSFRAME_H

#include "../libgui/Frame.h"
#include "MenuTypes.h"

class SettingsFrame : public menu::Frame
{
public:
	SettingsFrame();
	~SettingsFrame();
	void activateSubmenu(int submenu);

	enum SettingsSubmenus
	{
		SUBMENU_GENERAL=0,
		SUBMENU_VIDEO,
		SUBMENU_INPUT,
		SUBMENU_AUDIO,
		SUBMENU_SAVES
	};

private:
	int activeSubmenu;

};

#endif
