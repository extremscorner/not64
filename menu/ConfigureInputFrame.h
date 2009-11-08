#ifndef CONFIGUREINPUTFRAME_H
#define CONFIGUREINPUTFRAME_H

#include "../libgui/Frame.h"
#include "MenuTypes.h"

class ConfigureInputFrame : public menu::Frame
{
public:
	ConfigureInputFrame();
	~ConfigureInputFrame();
	void activateSubmenu(int submenu);

	enum ConfigureInputSubmenus
	{
		SUBMENU_NONE=0,
		SUBMENU_REINIT
	};

private:

};

#endif
