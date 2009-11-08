/**
 * Wii64 - Component.cpp
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

#include "Component.h"

namespace menu {

Component::Component()
		: componentList(0),
		  visible(true),
		  enabled(false),
		  focus(false),
		  parent(0),
		  type(TYPE_OTHER)
{
	nextFocus[0] = nextFocus[1] = nextFocus[2] = nextFocus[3] = nextFocus[4] = NULL;
}

Component::~Component()
{
}

void Component::setEnabled(bool enable)
{
//	printf("Component setEnabled\n");
	enabled = enable;
}

bool Component::isEnabled() const
{
	return enabled;
}

void Component::setVisible(bool visibleBool)
{
	visible = visibleBool;
}

bool Component::isVisible() const
{
	return visible;
}

void Component::setFocus(bool focusBool)
{
	focus = focusBool;
}

bool Component::getFocus()
{
	return focus;
}

void Component::setType(int typeInt)
{
	type = typeInt;
}

int Component::getType()
{
	return type;
}

void Component::setParent(Component* parent)
{
	this->parent = parent;
}

Component* Component::getParent() const
{
	return parent;
}

void Component::draw(Graphics& gfx)
{
	//push Graphics Context to Component variables
	//Draw object
	//maybe call paintComponent() and then paintChildren()
	//pop Graphics context
//	printf("Component draw\n");
	if (isVisible())
		drawComponent(gfx);
}

void Component::drawComponent(Graphics& gfx)
{
//	printf("virtual Component drawComponent\n");
	//Overload for custom draw routine
}

void Component::drawChildren(Graphics& gfx) const
{
	//Overload when Component class contains children
}

void Component::updateTime(float deltaTime)
{
	//Overload in Component class
	//Add interpolator class & update here?
}

void Component::setNextFocus(int direction, Component* component)
{
	nextFocus[direction] = component;
}

Component* Component::getNextFocus(int direction)
{
	return nextFocus[direction];
}

Component*  Component::updateFocus(int direction, int buttonsPressed)
{
	//Overload in Component class
	return NULL;
}

/*void Component::doFocusEvent(int type, bool buttonDown)
{
	//Overload in Component class
}*/

} //namespace menu 
