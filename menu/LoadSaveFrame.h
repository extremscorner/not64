#ifndef LOADSAVEFRAME_H
#define LOADSAVEFRAME_H

#include "../libgui/Frame.h"
#include "../libgui/Button.h"
#include "../libgui/Image.h"
#include "MenuTypes.h"

class LoadSaveFrame : public menu::Frame
{
public:
	LoadSaveFrame();//(MenuContext *menuContext);
	~LoadSaveFrame();

private:
	MenuContext *menuContext;
	menu::Image *buttonImage;
	menu::Image *buttonFocusImage;
	
};

#endif
