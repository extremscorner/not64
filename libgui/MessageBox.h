/**
 * Wii64 - MessageBox.h
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

#ifndef MESSAGEBOX_H
#define MESSAGEBOX_H

#include "Frame.h"
#include "Image.h"
#include "Gui.h"
#include "GuiTypes.h"

namespace menu {

class MessageBox : public Frame
{
public:
	void setMessage(const char* text);
	int askMessage(const char* text);
	void fadeMessage(const char* text);
	void setReturnValue(int returnValue);
	int getReturnValue();
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
	int returnValue;
	float messageFade;

};

} //namespace menu 

#endif
