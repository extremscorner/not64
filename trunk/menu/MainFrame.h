#ifndef MAINFRAME_H
#define MAINFRAME_H

#include "../libgui/Frame.h"
#include "../libgui/InputStatusBar.h"
#include "MenuTypes.h"

class MainFrame : public menu::Frame
{
public:
	MainFrame();
	~MainFrame();

private:
	menu::InputStatusBar *inputStatusBar;
};

#endif
