#ifndef LOADROMFRAME_H
#define LOADROMFRAME_H

#include "../libgui/Frame.h"
#include "../libgui/Button.h"
#include "../libgui/Image.h"
#include "MenuTypes.h"

class LoadRomFrame : public menu::Frame
{
public:
	LoadRomFrame();//(MenuContext *menuContext);
	~LoadRomFrame();

private:
	MenuContext *menuContext;
	menu::Image *buttonImage;
	menu::Image *buttonFocusImage;
	
};

#endif
