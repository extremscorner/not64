#ifndef SELECTCPUFRAME_H
#define SELECTCPUFRAME_H

#include "../libgui/Frame.h"
#include "../libgui/Button.h"
#include "../libgui/Image.h"
#include "MenuTypes.h"

class SelectCPUFrame : public menu::Frame
{
public:
	SelectCPUFrame();
	~SelectCPUFrame();

private:
	MenuContext *menuContext;
	menu::Image *buttonImage;
	menu::Image *buttonFocusImage;
	
};

#endif
