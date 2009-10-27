#ifndef SETTINGSFRAME_H
#define SETTINGSFRAME_H

#include "../libgui/Frame.h"
#include "../libgui/Button.h"
#include "../libgui/Image.h"
#include "MenuTypes.h"

class SettingsFrame : public menu::Frame
{
public:
	SettingsFrame();
	~SettingsFrame();

private:
	MenuContext *menuContext;
	menu::Image *buttonImage;
	menu::Image *buttonFocusImage;
	
};

#endif
