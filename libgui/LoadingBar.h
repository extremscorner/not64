#ifndef LOADINGBAR_H
#define LOADINGBAR_H

#include "Frame.h"
#include "Button.h"
#include "Image.h"
#include "Gui.h"
#include "GuiTypes.h"

namespace menu {

class LoadingBar : public Frame
{
public:
	void showBar(float percent, const char* text);
	bool getActive();
	void drawLoadingBar(Graphics& gfx);

	static LoadingBar& getInstance()
	{
		static LoadingBar obj;
		return obj;
	}

private:
	LoadingBar();
	~LoadingBar();
	Image *buttonImage;
	Image *buttonFocusImage;
	bool loadingBarActive;
	Frame *currentCursorFrame;
	Frame *currentFocusFrame;
	float percentComplete;
	GXColor boxColor, backColor, barColor, textColor;

};

} //namespace menu 

#endif
