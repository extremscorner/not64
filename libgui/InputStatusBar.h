/**
 * Wii64 - InputStatusBar.h
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
