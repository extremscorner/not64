/**
 * Wii64 - Button.cpp
 * Copyright (C) 2009 sepp256
 *
 * Wii64 homepage: http://www.emulatemii.com
 * email address: sepp256@gmail.com
 *
 *
 * This program is free software; you can redistribute it and/
 * or modify it under the terms of the GNU General Public Li-
 * cence as published by the Free Software Foundation; either
 * version 2 of the Licence, or any later version.
 *
 * This program is distributed in the hope that it will be use-
 * ful, but WITHOUT ANY WARRANTY; without even the implied war-
 * ranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public Licence for more details.
 *
**/

#include "Button.h"
#include "GuiResources.h"
#include "GraphicsGX.h"
#include "IPLFont.h"
#include "Image.h"
#include "FocusManager.h"
#include <math.h>

namespace menu {

Button::Button(int style, char** label, float x, float y, float width, float height)
		: active(false),
		  selected(false),
		  normalImage(0),
		  focusImage(0),
		  selectedImage(0),
		  selectedFocusImage(0),
		  buttonText(label),
		  buttonStyle(style),
		  labelMode(LABEL_CENTER),
		  labelScissor(0),
		  StartTime(0),
		  x(x),
		  y(y),
		  width(width),
		  height(height),
		  clickedFunc(0),
		  returnFunc(0)
{
						//Focus color			Inactive color		  Active color			Selected color		  Label color
	GXColor colors[5] = {{255, 100, 100, 255}, {255, 255, 255,  70}, {255, 255, 255, 130}, {255, 255, 255, 255}, {255, 255, 255, 255}};

	setType(TYPE_BUTTON);
	switch(buttonStyle)
	{
	case BUTTON_DEFAULT:
		setNormalImage(Resources::getInstance().getImage(Resources::IMAGE_DEFAULT_BUTTON));
		setFocusImage(Resources::getInstance().getImage(Resources::IMAGE_DEFAULT_BUTTONFOCUS));
		setSelectedImage(Resources::getInstance().getImage(Resources::IMAGE_DEFAULT_BUTTONFOCUS));
		break;
	case BUTTON_STYLEA_NORMAL:
		setNormalImage(Resources::getInstance().getImage(Resources::IMAGE_STYLEA_BUTTON));
		setFocusImage(Resources::getInstance().getImage(Resources::IMAGE_STYLEA_BUTTONFOCUS));
		height = 56;
		break;
	case BUTTON_STYLEA_SELECT:
		setNormalImage(Resources::getInstance().getImage(Resources::IMAGE_STYLEA_BUTTONSELECTOFF));
		setFocusImage(Resources::getInstance().getImage(Resources::IMAGE_STYLEA_BUTTONSELECTOFFFOCUS));
		setSelectedImage(Resources::getInstance().getImage(Resources::IMAGE_STYLEA_BUTTONSELECTON));
		setSelectedFocusImage(Resources::getInstance().getImage(Resources::IMAGE_STYLEA_BUTTONSELECTONFOCUS));
		height = 56;
		break;
	}
	if (buttonStyle != BUTTON_DEFAULT)
	{
		colors[0] = (GXColor) {255, 255, 255, 255};
		colors[1] = (GXColor) {200, 200, 200, 255};
		colors[2] = (GXColor) {255, 255, 255, 255};
	}

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

void Button::setSelected(bool selectedBool)
{
	selected = selectedBool;
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

#include "ogc/lwp_watchdog.h"

void Button::setLabelMode(int mode)
{
	labelMode = mode;
	if(labelMode >= LABEL_SCROLL) StartTime = ticks_to_microsecs(gettick());
}

void Button::setLabelScissor(int scissor)
{
	labelScissor = scissor;
}

void Button::setNormalImage(Image *image)
{
	normalImage = image;
}

void Button::setFocusImage(Image *image)
{
	focusImage = image;
}

void Button::setSelectedImage(Image *image)
{
	selectedImage = image;
}

void Button::setSelectedFocusImage(Image *image)
{
	selectedFocusImage = image;
}

#define SCROLL_PERIOD 4.0f

void Button::drawComponent(Graphics& gfx)
{
//	printf("Button drawComponent\n");

	gfx.setColor(inactiveColor);

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

	switch (buttonStyle)
	{
	case BUTTON_DEFAULT:
//	gfx.fillRect(x, y, width, height);
		normalImage->activateImage(GX_TEXMAP0);
		gfx.drawImage(0, x, y, width/2, height/2, 0.0, width/16.0, 0.0, height/16.0);
		gfx.drawImage(0, x+width/2, y, width/2, height/2, width/16.0, 0.0, 0.0, height/16.0);
		gfx.drawImage(0, x, y+height/2, width/2, height/2, 0.0, width/16.0, height/16.0, 0.0);
		gfx.drawImage(0, x+width/2, y+height/2, width/2, height/2, width/16.0, 0.0, height/16.0, 0.0);
//	gfx.drawImage(0, x, y, width, height, 0.0, 1.0, 0.0, 1.0);

		if (selected)
		{
			gfx.setColor(selectedColor);
			if(selectedImage) selectedImage->activateImage(GX_TEXMAP0);
			gfx.drawImage(0, x, y, width/2, height/2, 0.0, width/16.0, 0.0, height/16.0);
			gfx.drawImage(0, x+width/2, y, width/2, height/2, width/16.0, 0.0, 0.0, height/16.0);
			gfx.drawImage(0, x, y+height/2, width/2, height/2, 0.0, width/16.0, height/16.0, 0.0);
			gfx.drawImage(0, x+width/2, y+height/2, width/2, height/2, width/16.0, 0.0, height/16.0, 0.0);
		}
		break;
	case BUTTON_STYLEA_NORMAL:
		if (getFocus())	focusImage->activateImage(GX_TEXMAP0);
		else			normalImage->activateImage(GX_TEXMAP0);
		gfx.drawImage(0, x, y, width/2, height, 0.0, width/8.0, 0.0, 1.0);
		gfx.drawImage(0, x+width/2, y, width/2, height, width/8.0, 0.0, 0.0, 1.0);
		break;
	case BUTTON_STYLEA_SELECT:
		if (selected)
		{
			if (getFocus())	selectedFocusImage->activateImage(GX_TEXMAP0);
			else			selectedImage->activateImage(GX_TEXMAP0);
		}
		else
		{
			if (getFocus())	focusImage->activateImage(GX_TEXMAP0);
			else			normalImage->activateImage(GX_TEXMAP0);
		}
		gfx.drawImage(0, x, y, width/2, height, 0.0, width/8.0, 0.0, 1.0);
		gfx.drawImage(0, x+width/2, y, width/2, height, width/8.0, 0.0, 0.0, 1.0);
		break;
	}

	if (buttonText)
	{
		int strWidth, strHeight;
		unsigned long CurrentTime;
		float scrollWidth, time_sec, scrollOffset;
		gfx.enableScissor(x + labelScissor, y, width - 2*labelScissor, height);
		if(active)	IplFont::getInstance().drawInit(labelColor);
		else		IplFont::getInstance().drawInit(inactiveColor);
		switch (labelMode)
		{
			case LABEL_CENTER:
				IplFont::getInstance().drawString((int) (x+width/2), (int) (y+height/2), *buttonText, 1.0, true);
				break;
			case LABEL_LEFT:
				strWidth = IplFont::getInstance().getStringWidth(*buttonText, 1.0);
				strHeight = IplFont::getInstance().getStringHeight(*buttonText, 1.0);
				IplFont::getInstance().drawString((int) (x+labelScissor), (int) (y+(height-strHeight)/2), *buttonText, 1.0, false);
				break;
			case LABEL_SCROLL:
				strHeight = IplFont::getInstance().getStringHeight(*buttonText, 1.0);
				scrollWidth = IplFont::getInstance().getStringWidth(*buttonText, 1.0)-width+2*labelScissor;
				scrollWidth = scrollWidth < 0.0f ? 0.0 : scrollWidth;
				CurrentTime = ticks_to_microsecs(gettick());
				time_sec = (float)(CurrentTime - StartTime)/1000000.0f;
				if (time_sec > SCROLL_PERIOD) StartTime = ticks_to_microsecs(gettick());
				scrollOffset = fabsf(fmodf(time_sec,SCROLL_PERIOD)-SCROLL_PERIOD/2)/(SCROLL_PERIOD/2);
				IplFont::getInstance().drawString((int) (x+labelScissor-(int)(scrollOffset*scrollWidth)), (int) (y+(height-strHeight)/2), *buttonText, 1.0, false);
				break;
			case LABEL_SCROLLONFOCUS:
				if(getFocus())
				{
					strHeight = IplFont::getInstance().getStringHeight(*buttonText, 1.0);
					scrollWidth = IplFont::getInstance().getStringWidth(*buttonText, 1.0)-width+2*labelScissor;
					scrollWidth = scrollWidth < 0.0f ? 0.0 : scrollWidth;
					CurrentTime = ticks_to_microsecs(gettick());
					time_sec = (float)(CurrentTime - StartTime)/1000000.0f;
					if (time_sec > SCROLL_PERIOD) StartTime = ticks_to_microsecs(gettick());
					scrollOffset = fabsf(fmodf(time_sec,SCROLL_PERIOD)-SCROLL_PERIOD/2)/(SCROLL_PERIOD/2);
					IplFont::getInstance().drawString((int) (x+labelScissor-(int)(scrollOffset*scrollWidth)), (int) (y+(height-strHeight)/2), *buttonText, 1.0, false);
				}
				else
				{
				strWidth = IplFont::getInstance().getStringWidth(*buttonText, 1.0);
				strHeight = IplFont::getInstance().getStringHeight(*buttonText, 1.0);
				IplFont::getInstance().drawString((int) (x+labelScissor), (int) (y+(height-strHeight)/2), *buttonText, 1.0, false);
				}
				break;
		}
		gfx.disableScissor();
	}

}

void Button::updateTime(float deltaTime)
{
	//Overload in Component class
	//Add interpolator class & update here?
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
	inactiveColor.r = colors[1].r;
	inactiveColor.g = colors[1].g;
	inactiveColor.b = colors[1].b;
	inactiveColor.a = colors[1].a;
	activeColor.r = colors[2].r;
	activeColor.g = colors[2].g;
	activeColor.b = colors[2].b;
	activeColor.a = colors[2].a;
	selectedColor.r = colors[3].r;
	selectedColor.g = colors[3].g;
	selectedColor.b = colors[3].b;
	selectedColor.a = colors[3].a;
	labelColor.r = colors[4].r;
	labelColor.g = colors[4].g;
	labelColor.b = colors[4].b;
	labelColor.a = colors[4].a;
}

void Button::setLabelColor(GXColor color)
{
	labelColor.r = color.r;
	labelColor.g = color.g;
	labelColor.b = color.b;
	labelColor.a = color.a;
}

} //namespace menu 
