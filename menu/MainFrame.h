#ifndef MAINFRAME_H
#define MAINFRAME_H

#include "../libgui/Frame.h"
#include "../libgui/Button.h"
#include "../libgui/Image.h"
#include "MenuTypes.h"

class MainFrame : public menu::Frame
{
public:
	MainFrame();//(MenuContext* menuContext);
	~MainFrame();

private:
	MenuContext *menuContext;
/*	menu::Button *button1;
	menu::Button *button2;
	menu::Button *button3;
	menu::Button *button4;*/
	menu::Image *buttonImage;
	menu::Image *buttonFocusImage;
	
};

#endif
