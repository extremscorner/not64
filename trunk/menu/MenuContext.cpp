#include "MenuContext.h"
#include "../libgui/FocusManager.h"
#include "../libgui/CursorManager.h"

MenuContext *pMenuContext;

MenuContext::MenuContext(GXRModeObj *vmode)
		: currentActiveFrame(0),
		  mainFrame(0),
		  loadRomFrame(0),
		  fileBrowserFrame(0),
		  currentRomFrame(0),
		  loadSaveFrame(0),
		  saveGameFrame(0),
		  settingsFrame(0)
{
	pMenuContext = this;

	menu::Gui::getInstance().setVmode(vmode);

	mainFrame = new MainFrame();
	loadRomFrame = new LoadRomFrame();
	fileBrowserFrame = new FileBrowserFrame();
	currentRomFrame = new CurrentRomFrame();
	loadSaveFrame = new LoadSaveFrame();
	saveGameFrame = new SaveGameFrame();
	settingsFrame = new SettingsFrame();

	menu::Gui::getInstance().addFrame(mainFrame);
	menu::Gui::getInstance().addFrame(loadRomFrame);
	menu::Gui::getInstance().addFrame(fileBrowserFrame);
	menu::Gui::getInstance().addFrame(currentRomFrame);
	menu::Gui::getInstance().addFrame(loadSaveFrame);
	menu::Gui::getInstance().addFrame(saveGameFrame);
	menu::Gui::getInstance().addFrame(settingsFrame);

	menu::Focus::getInstance().setFocusActive(true);
	setActiveFrame(FRAME_MAIN);
}

MenuContext::~MenuContext()
{
	delete settingsFrame;
	delete saveGameFrame;
	delete loadSaveFrame;
	delete currentRomFrame;
	delete fileBrowserFrame;
	delete loadRomFrame;
	delete mainFrame;
	pMenuContext = NULL;
}

bool MenuContext::isRunning()
{
	bool isRunning = true;
//	printf("MenuContext isRunning\n");
	draw();

/*	PADStatus* gcPad = menu::Input::getInstance().getPad();
	if(gcPad[0].button & PAD_BUTTON_START)
		isRunning = false;*/
	
	return isRunning;
}

void MenuContext::setActiveFrame(int frameIndex)
{
	if(currentActiveFrame)
		currentActiveFrame->hideFrame();

	switch(frameIndex) {
	case FRAME_MAIN:
		currentActiveFrame = mainFrame;
		break;
	case FRAME_LOADROM:
		currentActiveFrame = loadRomFrame;
		break;
	case FRAME_FILEBROWSER:
		currentActiveFrame = fileBrowserFrame;
		break;
	case FRAME_CURRENTROM:
		currentActiveFrame = loadSaveFrame;
		break;
	case FRAME_LOADSAVE:
		currentActiveFrame = loadSaveFrame;
		break;
	case FRAME_SAVEGAME:
		currentActiveFrame = saveGameFrame;
		break;
	case FRAME_SETTINGS:
		currentActiveFrame = settingsFrame;
		break;
	}

	if(currentActiveFrame)
	{
		currentActiveFrame->showFrame();
		menu::Focus::getInstance().setCurrentFrame(currentActiveFrame);
		menu::Cursor::getInstance().setCurrentFrame(currentActiveFrame);
	}
}

void MenuContext::draw()
{
	menu::Gui::getInstance().draw();
}
