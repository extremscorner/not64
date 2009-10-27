#ifndef SAVEGAMEFRAME_H
#define SAVEGAMEFRAME_H

#include "../libgui/Frame.h"
#include "../libgui/Button.h"
#include "../libgui/Image.h"
#include "MenuTypes.h"

class SaveGameFrame : public menu::Frame
{
public:
	SaveGameFrame();//(MenuContext *menuContext);
	~SaveGameFrame();

private:
	MenuContext *menuContext;
	menu::Image *buttonImage;
	menu::Image *buttonFocusImage;
	
};

#endif
