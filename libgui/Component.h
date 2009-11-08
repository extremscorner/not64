/**
 * Wii64 - Component.h
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

#ifndef COMPONENT_H
#define COMPONENT_H

#include "GuiTypes.h"

namespace menu {

class Component
{
public:
	Component();
	virtual ~Component();
	virtual void setEnabled(bool enable);
	bool isEnabled() const;
	void setVisible(bool visible);
	bool isVisible() const;
	void setFocus(bool focus);
	bool getFocus();
	void setType(int type);
	int getType();
	void setParent(Component* parent);
	Component* getParent() const;
	void draw(Graphics& gfx);
	virtual void drawComponent(Graphics& gfx);
	virtual void drawChildren(Graphics& gfx) const;
	virtual void updateTime(float deltaTime);
	void setNextFocus(int direction, Component* component);
	Component* getNextFocus(int direction);
	virtual Component* updateFocus(int direction, int buttonsPressed);
//	virtual void doFocusEvent(int type, bool buttonDown);
	enum ComponentType
	{
		TYPE_OTHER,
		TYPE_FRAME,
		TYPE_BUTTON,
		TYPE_TEXTBOX
	};

protected:
	ComponentList componentList;

private:
	bool visible, enabled, focus;
	Component* parent;
	Component* nextFocus[5];
	int type;

};

} //namespace menu 

#endif
