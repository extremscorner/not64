#ifndef GUI_H
#define GUI_H

#include "GuiTypes.h"
#include "Frame.h"
#include "GraphicsGX.h"

namespace menu {

class Gui
{
public:
	void setVmode(GXRModeObj *rmode);
	Gui(GXRModeObj *vmode);
	void addFrame(Frame* frame);
	void removeFrame(Frame* frame);
	void draw();
	static Gui& getInstance()
	{
		static Gui obj;
		return obj;
	}

private:
	Gui();
	~Gui();
	Graphics *gfx;
	FrameList frameList;
};

} //namespace menu 

#endif
