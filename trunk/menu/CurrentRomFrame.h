#ifndef CURRENTROMFRAME_H
#define CURRENTROMFRAME_H

#include "../libgui/Frame.h"
#include "../libgui/Button.h"
#include "../libgui/Image.h"
#include "MenuTypes.h"

class CurrentRomFrame : public menu::Frame
{
public:
	CurrentRomFrame();//(MenuContext *menuContext);
	~CurrentRomFrame();

private:
	MenuContext *menuContext;
	menu::Image *buttonImage;
	menu::Image *buttonFocusImage;
	
};

#endif
