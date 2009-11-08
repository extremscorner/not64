/**
 * Wii64 - LoadingBar.h
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
