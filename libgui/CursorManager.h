/**
 * Wii64 - CursorManager.h
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

#ifndef CURSORMANAGER_H
#define CURSORMANAGER_H

#include "GuiTypes.h"

namespace menu {

class Cursor
{
public:
	void updateCursor();
	void drawCursor(Graphics& gfx);
	void setCursorFocus(Component* component);
	void addComponent(Frame* frame, Component* component, float x1, float x2, float y1, float y2);
	void removeComponent(Frame* frame, Component* component);
	void setCurrentFrame(Frame* frame);
	Frame* getCurrentFrame();
	void clearInputData();
	void clearCursorFocus();
	void setFreezeAction(bool freezeAction);
	static Cursor& getInstance()
	{
		static Cursor obj;
		return obj;
	}

private:
	Cursor();
	~Cursor();

	class CursorEntry
	{
	public:
		Frame *frame;
		Component *comp;
		float xRange[2], yRange[2];
	};

	Frame *currentFrame;
	std::vector<CursorEntry> cursorList;
	Image *pointerImage, *grabImage;
	float cursorX, cursorY, cursorRot, imageCenterX, imageCenterY;
	Component *foundComponent, *hoverOverComponent;
	bool pressed, frameSwitch, clearInput, freezeAction;
	int buttonsPressed, previousButtonsPressed[4], activeChan;
};

} //namespace menu 

#endif
