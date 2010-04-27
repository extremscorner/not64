/**
 * Wii64 - Frame.cpp
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

#include "Frame.h"
#include "FocusManager.h"

namespace menu {

Frame::Frame()
		: defaultFocus(0),
		  backFunc(0),
		  selectFunc(0)
{
	setVisible(false);
}

Frame::~Frame()
{
	removeAll();
}

void Frame::showFrame()
{
	setVisible(true);
}

void Frame::hideFrame()
{
	setVisible(false);
}

void Frame::updateTime(float deltaTime)
{
	ComponentList::const_iterator iteration;
	for (iteration = componentList.begin(); iteration != componentList.end(); iteration++)
	{
		(*iteration)->updateTime(deltaTime);
	}
}

void Frame::drawChildren(Graphics &gfx) const
{
	if(isVisible())
	{
//		printf("Frame drawChildren\n");
		ComponentList::const_iterator iteration;
		for (iteration = componentList.begin(); iteration != componentList.end(); iteration++)
		{
//			printf("Frame calling component::draw\n");
			(*iteration)->draw(gfx);
		}
	}
}

void Frame::setEnabled(bool enable)
{
//	printf("Frame setEnabled\n");
	ComponentList::const_iterator iterator;
	for(iterator = componentList.begin(); iterator != componentList.end(); ++iterator)
	{
		(*iterator)->setEnabled(enable);
	}
	Component::setEnabled(enable);
}

void Frame::remove(Component* component)
{
	ComponentList::iterator iter = std::find(componentList.begin(), componentList.end(),component);
	if(iter != componentList.end())
	{
		(*iter)->setParent(0);
		componentList.erase(iter);
	}
}

void Frame::removeAll()
{
	componentList.clear();
}

void Frame::add(Component* component)
{
	component->setParent(this);
	componentList.push_back(component);
}

void Frame::setDefaultFocus(Component* component)
{
	defaultFocus = component;
}

Component* Frame::getDefaultFocus()
{
	return defaultFocus;
}

void Frame::setBackFunc(FrameFunc backFn)
{
	backFunc = backFn;
}

void Frame::setSelectFunc(FrameFunc selectFn)
{
	selectFunc = selectFn;
}

Component* Frame::updateFocus(int direction, int buttonsPressed)
{
	//This function only called from CursorManager when wii-mote is not pointing to a button.
	Component* newFocus = NULL;

	if(buttonsPressed & Focus::ACTION_BACK && backFunc) backFunc();
	else if (buttonsPressed & Focus::ACTION_SELECT && selectFunc) selectFunc();

	return newFocus;
}

} //namespace menu 
