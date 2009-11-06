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
