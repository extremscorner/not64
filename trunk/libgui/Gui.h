#ifndef GUI_H
#define GUI_H

#include "GuiTypes.h"
#include "Frame.h"
#include "Logo.h"
#include "GraphicsGX.h"

namespace menu {

class Gui
{
public:
	void setVmode(GXRModeObj *rmode);
	void addFrame(Frame* frame);
	void removeFrame(Frame* frame);
	void draw();
	static Gui& getInstance()
	{
		static Gui obj;
		return obj;
	}
	Graphics *gfx;
	Logo* menuLogo;

private:
	Gui();
	~Gui();
	FrameList frameList;
	char fade;
};

} //namespace menu 

#endif
