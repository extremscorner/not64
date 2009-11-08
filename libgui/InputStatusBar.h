#ifndef INPUTSTATUSBAR_H
#define INPUTSTATUSBAR_H

//#include "GuiTypes.h"
#include "Component.h"

namespace menu {

class InputStatusBar : public Component
{
public:
	InputStatusBar(float x, float y);
	~InputStatusBar();
	void updateTime(float deltaTime);
	void drawComponent(Graphics& gfx);
	Component* updateFocus(int direction, int buttonsPressed);

private:
/*	Image	*normalImage;
	Image	*focusImage;
	Image	*selectedImage;
	Image	*selectedFocusImage;
	char** buttonText;
	int buttonStyle, labelMode, labelScissor;
	unsigned long StartTime;*/
	float x, y;/*, width, height;
	GXColor	focusColor, inactiveColor, activeColor, selectedColor, labelColor;*/

};

} //namespace menu 

#endif
