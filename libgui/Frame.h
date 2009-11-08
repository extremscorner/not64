/**
 * Wii64 - Frame.h
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

#ifndef FRAME_H
#define FRAME_H

#include "GuiTypes.h"
#include "Component.h"

typedef void (*FrameFunc)( void );

namespace menu {

class Frame : public Component
{
public:
	Frame();
	~Frame();
	void showFrame();
	void hideFrame();
	void setEnabled(bool enable);
	void drawChildren(Graphics& gfx) const;
	void remove(Component* component);
	void removeAll();
	void add(Component* comp);
	void updateTime(float deltaTime);
	void setDefaultFocus(Component* comp);
	Component* getDefaultFocus();
	void setBackFunc(FrameFunc backFn);
	void setSelectFunc(FrameFunc selectFn);
	Component* updateFocus(int direction, int buttonsPressed);
	virtual void activateSubmenu(int submenu) {};

private:
	Component* defaultFocus;
	FrameFunc backFunc, selectFunc;

};

} //namespace menu 

#endif
