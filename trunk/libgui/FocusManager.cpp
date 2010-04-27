/**
 * Wii64 - FocusManager.cpp
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

#include "FocusManager.h"
#include "InputManager.h"
#include "Frame.h"
#include "IPLFont.h"

namespace menu {

Focus::Focus()
		: focusActive(false),
		  pressed(false),
		  frameSwitch(true),
		  clearInput(true),
		  freezeAction(false),
		  buttonsPressed(0),
		  focusList(0),
		  primaryFocusOwner(0),
		  secondaryFocusOwner(0)
{
	for (int i=0; i<4; i++) {
		previousButtonsWii[i] = 0;
		previousButtonsGC[i] = 0;
	}
}

Focus::~Focus()
{
}

void Focus::updateFocus()
{
	int focusDirection = 0;
	int buttonsDown = 0;
#ifdef HW_RVL
	WPADData* wiiPad = Input::getInstance().getWpad();
#endif
//	PADStatus* gcPad = Input::getInstance().getPad();

	if (!focusActive) return;

	if (frameSwitch)
	{
		if(primaryFocusOwner) primaryFocusOwner->setFocus(false);
		if (currentFrame) primaryFocusOwner = currentFrame->getDefaultFocus();
		else primaryFocusOwner = NULL;
		frameSwitch = false;
	}

	if(clearInput)
	{
		for (int i=0; i<4; i++)
		{
			previousButtonsGC[i] = PAD_ButtonsHeld(i);
#ifdef HW_RVL
			previousButtonsWii[i] = wiiPad[i].btns_h;
#endif
		}
		clearInput = false;
	}

	for (int i=0; i<4; i++)
	{
		u16 currentButtonsGC = PAD_ButtonsHeld(i);
		if (currentButtonsGC ^ previousButtonsGC[i])
		{
			u16 currentButtonsDownGC = (currentButtonsGC ^ previousButtonsGC[i]) & currentButtonsGC;
			switch (currentButtonsDownGC & 0xf) {
			case PAD_BUTTON_LEFT:
				focusDirection = DIRECTION_LEFT;
				break;
			case PAD_BUTTON_RIGHT:
				focusDirection = DIRECTION_RIGHT;
				break;
			case PAD_BUTTON_DOWN:
				focusDirection = DIRECTION_DOWN;
				break;
			case PAD_BUTTON_UP:
				focusDirection = DIRECTION_UP;
				break;
			default:
				focusDirection = DIRECTION_NONE;
			}
			if (currentButtonsDownGC & PAD_BUTTON_A) buttonsDown |= ACTION_SELECT;
			if (currentButtonsDownGC & PAD_BUTTON_B) buttonsDown |= ACTION_BACK;
			if (freezeAction)
			{
				focusDirection = DIRECTION_NONE;
				buttonsDown = 0;
			}
			if (primaryFocusOwner) primaryFocusOwner = primaryFocusOwner->updateFocus(focusDirection,buttonsDown);
			else primaryFocusOwner = currentFrame->updateFocus(focusDirection,buttonsDown);
			previousButtonsGC[i] = currentButtonsGC;
			break;
		}
#ifdef HW_RVL
		else if (wiiPad[i].btns_h ^ previousButtonsWii[i])
		{
			u32 currentButtonsDownWii = (wiiPad[i].btns_h ^ previousButtonsWii[i]) & wiiPad[i].btns_h;
			if (wiiPad[i].exp.type == WPAD_EXP_CLASSIC)
			{
				switch (currentButtonsDownWii & 0xc0030000) {
				case WPAD_CLASSIC_BUTTON_LEFT:
					focusDirection = DIRECTION_LEFT;
					break;
				case WPAD_CLASSIC_BUTTON_RIGHT:
					focusDirection = DIRECTION_RIGHT;
					break;
				case WPAD_CLASSIC_BUTTON_DOWN:
					focusDirection = DIRECTION_DOWN;
					break;
				case WPAD_CLASSIC_BUTTON_UP:
					focusDirection = DIRECTION_UP;
					break;
				default:
					focusDirection = DIRECTION_NONE;
				}
			}
			else
			{
				switch (currentButtonsDownWii & 0xf00) {
				case WPAD_BUTTON_LEFT:
					focusDirection = DIRECTION_LEFT;
					break;
				case WPAD_BUTTON_RIGHT:
					focusDirection = DIRECTION_RIGHT;
					break;
				case WPAD_BUTTON_DOWN:
					focusDirection = DIRECTION_DOWN;
					break;
				case WPAD_BUTTON_UP:
					focusDirection = DIRECTION_UP;
					break;
				default:
					focusDirection = DIRECTION_NONE;
				}
			}
			if (currentButtonsDownWii & (WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A)) buttonsDown |= ACTION_SELECT;
			if (currentButtonsDownWii & (WPAD_BUTTON_B | WPAD_CLASSIC_BUTTON_B)) buttonsDown |= ACTION_BACK;
			if (freezeAction)
			{
				focusDirection = DIRECTION_NONE;
				buttonsDown = 0;
			}
			if (primaryFocusOwner) primaryFocusOwner = primaryFocusOwner->updateFocus(focusDirection,buttonsDown);
			else primaryFocusOwner = currentFrame->updateFocus(focusDirection,buttonsDown);
			previousButtonsWii[i] = wiiPad[i].btns_h;
			break;
		}
#endif
	}
}

void Focus::addComponent(Component* component)
{
	focusList.push_back(component);
}

void Focus::removeComponent(Component* component)
{
	ComponentList::iterator iter = std::find(focusList.begin(), focusList.end(),component);
	if(iter != focusList.end())
	{
		focusList.erase(iter);
	}
}

Frame* Focus::getCurrentFrame()
{
	return currentFrame;
}

void Focus::setCurrentFrame(Frame* frame)
{
	currentFrame = frame;
	frameSwitch = true;
	Input::getInstance().clearInputData();
}

void Focus::setFocusActive(bool focusActiveBool)
{
	focusActive = focusActiveBool;
	if (primaryFocusOwner) primaryFocusOwner->setFocus(focusActive);
}

void Focus::clearInputData()
{
	clearInput = true;
}

void Focus::clearPrimaryFocus()
{
	if(primaryFocusOwner) primaryFocusOwner->setFocus(false);
	primaryFocusOwner = NULL;
	frameSwitch = true;
}

void Focus::setFreezeAction(bool freeze)
{
	freezeAction = freeze;
}

} //namespace menu 
