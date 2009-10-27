#include "Button.h"
#include "GraphicsGX.h"
#include "IPLFont.h"
#include "Image.h"
#include "FocusManager.h"

namespace menu {

Button::Button(Image *image, char** label, float x, float y, float width, float height)
		: active(false),
		  normalImage(0),
		  pressedImage(0),
		  focusImage(0),
		  buttonText(label),
		  x(x),
		  y(y),
		  width(width),
		  height(height),
		  clickedFunc(0),
		  returnFunc(0)
{
	setType(TYPE_BUTTON);
	setNormalImage(image);
	GXColor colors[4] = {{255, 100, 100, 255}, {255, 255, 255, 150}, {100, 100, 255, 255}, {100, 255, 100, 255}};
	setButtonColors(colors);
}

Button::~Button()
{
}

void Button::setActive(bool activeBool)
{
	active = activeBool;
}

bool Button::getActive()
{
	return active;
}

void Button::setReturn(ButtonFunc newReturnFunc)
{
	returnFunc = newReturnFunc;
}

void Button::doReturn()
{
	if (returnFunc) returnFunc();
}

void Button::setClicked(ButtonFunc newClickedFunc)
{
	clickedFunc = newClickedFunc;
}

void Button::doClicked()
{
	if (clickedFunc) clickedFunc();
}

void Button::setText(char** strPtr)
{
	buttonText = strPtr;
}

void Button::setNormalImage(Image *image)
{
	normalImage = image;
}

void Button::setPressedImage(Image *image)
{
	pressedImage = image;
}

void Button::setFocusImage(Image *image)
{
	focusImage = image;
}

void Button::drawComponent(Graphics& gfx)
{
//	printf("Button drawComponent\n");

	gfx.setColor(activeColor);

	//activate relevant texture
	if(active)
	{
		//draw normalImage with/without gray mask and with alpha test on
		//printf("Button Active\n");
		gfx.setColor(activeColor);
	}
	if(getFocus())
	{
		//draw focus indicator (extra border for button?)
		//printf("Button in Focus\n");
		gfx.setColor(focusColor);
	}
	//draw buttonLabel?

	gfx.enableBlending(true);
//	gfx.setTEV(GX_PASSCLR);
	gfx.setTEV(GX_MODULATE);

//	gfx.setColor(focusColor);
	gfx.setDepth(-10.0f);
	gfx.newModelView();
	gfx.loadModelView();
	gfx.loadOrthographic();

//	gfx.fillRect(x, y, width, height);
	normalImage->activateImage(GX_TEXMAP0);
	gfx.drawImage(0, x, y, width/2, height/2, 0.0, width/16.0, 0.0, height/16.0);
	gfx.drawImage(0, x+width/2, y, width/2, height/2, width/16.0, 0.0, 0.0, height/16.0);
	gfx.drawImage(0, x, y+height/2, width/2, height/2, 0.0, width/16.0, height/16.0, 0.0);
	gfx.drawImage(0, x+width/2, y+height/2, width/2, height/2, width/16.0, 0.0, height/16.0, 0.0);
//	gfx.drawImage(0, x, y, width, height, 0.0, 1.0, 0.0, 1.0);

	if (buttonText)
	{
		IplFont::getInstance().drawInit(labelColor);
		IplFont::getInstance().drawString((int) (x+width/2), (int) (y+height/2), *buttonText, 1.0, true);
	}
}

Component* Button::updateFocus(int direction, int buttonsPressed)
{
	Component* newFocus = NULL;
	if(!getFocus() && active)
	{
		setFocus(true);
		newFocus = this;
	}
	else if (!getFocus() && !active)
	{
		//try to move direction, and if new component found send to focus manager and remove focus for current component
		if (getNextFocus(direction))
			newFocus = (getNextFocus(direction))->updateFocus(direction, buttonsPressed);
		else
			newFocus = getNextFocus(direction);
	}
	else
	{
		if (getNextFocus(direction))
			newFocus = (getNextFocus(direction))->updateFocus(direction, buttonsPressed);
		if (newFocus)
			setFocus(false);
		else
			newFocus = this;
	}
	if (newFocus == this)
	{
		//finally update button behavior based on buttons pressed
		if(buttonsPressed & Focus::ACTION_BACK)
			doReturn();
		else if (buttonsPressed & Focus::ACTION_SELECT)
			doClicked();
	}
	return newFocus;
}

void Button::setButtonColors(GXColor *colors)
{
	focusColor.r = colors[0].r;
	focusColor.g = colors[0].g;
	focusColor.b = colors[0].b;
	focusColor.a = colors[0].a;
	activeColor.r = colors[1].r;
	activeColor.g = colors[1].g;
	activeColor.b = colors[1].b;
	activeColor.a = colors[1].a;
	pressedColor.r = colors[2].r;
	pressedColor.g = colors[2].g;
	pressedColor.b = colors[2].b;
	pressedColor.a = colors[2].a;
	labelColor.r = colors[3].r;
	labelColor.g = colors[3].g;
	labelColor.b = colors[3].b;
	labelColor.a = colors[3].a;
}

} //namespace menu 
