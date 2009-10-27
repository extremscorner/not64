#ifndef MESSAGEBOX_H
#define MESSAGEBOX_H

#include "Frame.h"
#include "Button.h"
#include "Image.h"
#include "Gui.h"
#include "GuiTypes.h"

namespace menu {

class MessageBox : public Frame
{
public:
	void setMessage(const char* text);
	void deactivate();
	bool getActive();
	void drawMessageBox(Graphics& gfx);

	static MessageBox& getInstance()
	{
		static MessageBox obj;
		return obj;
	}

private:
	MessageBox();
	~MessageBox();
	Image *buttonImage;
	Image *buttonFocusImage;
	bool messageBoxActive;
	Frame *currentCursorFrame;
	Frame *currentFocusFrame;
	GXColor boxColor, textColor;

};

} //namespace menu 

#endif
