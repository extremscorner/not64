#ifndef BUTTON_H
#define BUTTON_H

//#include "GuiTypes.h"
#include "Component.h"

#define BTN_DEFAULT menu::Button::BUTTON_DEFAULT
#define BTN_A_NRM menu::Button::BUTTON_STYLEA_NORMAL
#define BTN_A_SEL menu::Button::BUTTON_STYLEA_SELECT

typedef void (*ButtonFunc)( void );

namespace menu {

class Button : public Component
{
public:
	Button(int style, char** label, float x, float y, float width, float height);
	~Button();
	void setActive(bool active);
	bool getActive();
	void setSelected(bool selected);
	void setReturn(ButtonFunc returnFn);
	void doReturn();
	void setClicked(ButtonFunc clickedFn);
	void doClicked();
	void setText(char** strPtr);
	void setLabelMode(int mode);
	void setLabelScissor(int scissor);
	void setNormalImage(Image *image);
	void setFocusImage(Image *image);
	void setSelectedImage(Image *image);
	void setSelectedFocusImage(Image *image);
	void updateTime(float deltaTime);
	void drawComponent(Graphics& gfx);
	Component* updateFocus(int direction, int buttonsPressed);
	void setButtonColors(GXColor *colors);
	enum LabelMode
	{
		LABEL_CENTER=0,
		LABEL_LEFT,
		LABEL_SCROLL,
		LABEL_SCROLLONFOCUS
	};

	enum ButtonStyle
	{
		BUTTON_DEFAULT=0,
		BUTTON_STYLEA_NORMAL,
		BUTTON_STYLEA_SELECT
	};

private:
	bool active, selected;
	Image	*normalImage;
	Image	*focusImage;
	Image	*selectedImage;
	Image	*selectedFocusImage;
	char** buttonText;
	int buttonStyle, labelMode, labelScissor;
	unsigned long StartTime;
	float x, y, width, height;
	GXColor	focusColor, inactiveColor, activeColor, selectedColor, labelColor;
	ButtonFunc clickedFunc, returnFunc;

};

} //namespace menu 

#endif
