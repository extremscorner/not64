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
		FRAME_CONFIGUREPAKS
		
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

};

#endif
