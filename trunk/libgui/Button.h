#ifndef BUTTON_H
#define BUTTON_H

//#include "GuiTypes.h"
#include "Component.h"

typedef void (*ButtonFunc)( void );

namespace menu {

class Button : public Component
{
public:
	Button(Image *image, char** label, float x, float y, float width, float height);
	~Button();
	void setActive(bool active);
	bool getActive();
	void setReturn(ButtonFunc returnFn);
	void doReturn();
	void setClicked(ButtonFunc clickedFn);
	void doClicked();
	void setText(char** strPtr);
	void setNormalImage(Image *image);
	void setPressedImage(Image *image);
	void setFocusImage(Image *image);
	void drawComponent(Graphics& gfx);
	Component* updateFocus(int direction, int buttonsPressed);
	void setButtonColors(GXColor *colors);

private:
	bool active;
	Image	*normalImage;
	Image	*pressedImage;
	Image	*focusImage;
	char** buttonText;
	float x, y, width, height;
	GXColor	focusColor, activeColor, pressedColor, labelColor;
	ButtonFunc clickedFunc, returnFunc;

};

} //namespace menu 

#endif
