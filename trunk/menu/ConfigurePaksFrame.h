#ifndef CONFIGUREPAKSFRAME_H
#define CONFIGUREPAKSFRAME_H

#include "../libgui/Frame.h"
#include "MenuTypes.h"

class ConfigurePaksFrame : public menu::Frame
{
public:
	ConfigurePaksFrame();
	~ConfigurePaksFrame();
	void activateSubmenu(int submenu);

	enum ConfigurePaksSubmenus
	{
		SUBMENU_NONE=0,
		SUBMENU_REINIT
	};

private:

};

#endif
