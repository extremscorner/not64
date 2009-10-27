#ifndef FILEBROWSERFRAME_H
#define FILEBROWSERFRAME_H

#include "../libgui/Frame.h"
#include "../libgui/Button.h"
#include "../libgui/Image.h"
#include "MenuTypes.h"

class FileBrowserFrame : public menu::Frame
{
public:
	FileBrowserFrame();//(MenuContext *menuContext);
	~FileBrowserFrame();

private:
	MenuContext *menuContext;
	menu::Image *buttonImage;
	menu::Image *buttonFocusImage;
	
};

#endif
