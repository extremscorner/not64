/**
 * Wii64 - MenuContext.h
 * Copyright (C) 2009, 2010 sepp256
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

#ifndef MENUCONTEXT_H
#define MENUCONTEXT_H

#include "../libgui/Gui.h"
#include "../libgui/Frame.h"
#include "../libgui/InputManager.h"
#include "MainFrame.h"
#include "LoadRomFrame.h"
#include "FileBrowserFrame.h"
#include "CurrentRomFrame.h"
#include "LoadSaveFrame.h"
#include "SaveGameFrame.h"
#include "SettingsFrame.h"
#include "SelectCPUFrame.h"
#include "ConfigureInputFrame.h"
#include "ConfigurePaksFrame.h"
#include "ConfigureButtonsFrame.h"

#include "MenuTypes.h"

class MenuContext
{
public:
	MenuContext(GXRModeObj *vmode);
	~MenuContext();
	bool isRunning();
	void setActiveFrame(int frameIndex);
	void setActiveFrame(int frameIndex, int submenu);
	menu::Frame* getFrame(int frameIndex);
	enum FrameIndices
	{
		FRAME_MAIN=1,
		FRAME_LOADROM,
		FRAME_FILEBROWSER,
		FRAME_CURRENTROM,
		FRAME_LOADSAVE,
		FRAME_SAVEGAME,
		FRAME_SETTINGS,
		FRAME_SELECTCPU,
		FRAME_CONFIGUREINPUT,
		FRAME_CONFIGUREPAKS,
		FRAME_CONFIGUREBUTTONS
		
	};

private:
	void draw();
	menu::Frame *currentActiveFrame;
	MainFrame *mainFrame;
	LoadRomFrame *loadRomFrame;
	FileBrowserFrame *fileBrowserFrame;
	CurrentRomFrame *currentRomFrame;
	LoadSaveFrame *loadSaveFrame;
	SaveGameFrame *saveGameFrame;
	SettingsFrame *settingsFrame;
	SelectCPUFrame *selectCPUFrame;
	ConfigureInputFrame *configureInputFrame;
	ConfigurePaksFrame *configurePaksFrame;
	ConfigureButtonsFrame *configureButtonsFrame;

};

#endif
