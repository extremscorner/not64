/**
 * Wii64 - FocusManager.h
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

#ifndef FOCUSMANAGER_H
#define FOCUSMANAGER_H

#include "GuiTypes.h"

namespace menu {

class Focus
{
public:
	void updateFocus();
	void addComponent(Component* component);
	void removeComponent(Component* component);
	Frame* getCurrentFrame();
	void setCurrentFrame(Frame* frame);
	void setFocusActive(bool active);
	void clearInputData();
	void clearPrimaryFocus();
	void setFreezeAction(bool freezeAction);
	enum FocusDirection
	{
		DIRECTION_NONE=0,
		DIRECTION_LEFT,
		DIRECTION_RIGHT,
		DIRECTION_DOWN,
		DIRECTION_UP
	};
	enum FocusAction
	{
		ACTION_SELECT=1,
		ACTION_BACK=2
	};

	static Focus& getInstance()
	{
		static Focus obj;
		return obj;
	}

private:
	Focus();
	~Focus();
	bool focusActive, pressed, frameSwitch, clearInput, freezeAction;
	int buttonsPressed, previousButtonsPressed;
	u32 previousButtonsWii[4];
	u16 previousButtonsGC[4];
	ComponentList focusList;
	Component *primaryFocusOwner, *secondaryFocusOwner;
	Frame *currentFrame;

};

} //namespace menu 

#endif
