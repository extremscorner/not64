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
	WPADData* wpad = Input::getInstance().getWpad();
#endif

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
			previousButtonsWii[i] = WPAD_ButtonsHeld(i);
#endif
		}
		clearInput = false;
	}

	for (int i=0; i<4; i++)
	{
		u16 currentButtonsGC = PAD_ButtonsHeld(i);
#ifdef HW_RVL
		u32 currentButtonsWii = WPAD_ButtonsHeld(i);
#endif
		if (currentButtonsGC ^ previousButtonsGC[i])
		{
			u16 currentButtonsDownGC = (currentButtonsGC ^ previousButtonsGC[i]) & currentButtonsGC;
			previousButtonsGC[i] = currentButtonsGC;
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
			break;
		}
#ifdef HW_RVL
		else if (currentButtonsWii ^ previousButtonsWii[i])
		{
			u32 currentButtonsDownWii = (currentButtonsWii ^ previousButtonsWii[i]) & currentButtonsWii;
			previousButtonsWii[i] = currentButtonsWii;
			switch (wpad[i].exp.type)
			{
			case WPAD_EXP_CLASSIC:
			case WPAD_EXP_WIIUPRO:
				switch (currentButtonsDownWii & 0x3c00000) {
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
				if (currentButtonsDownWii & (WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A)) buttonsDown |= ACTION_SELECT;
				if (currentButtonsDownWii & (WPAD_BUTTON_B | WPAD_CLASSIC_BUTTON_B)) buttonsDown |= ACTION_BACK;
				break;
			case WPAD_EXP_NES:
			case WPAD_EXP_SNES:
			case WPAD_EXP_N64:
				switch (currentButtonsDownWii & 0xf0000) {
				case WPAD_NES_BUTTON_LEFT:
					focusDirection = DIRECTION_LEFT;
					break;
				case WPAD_NES_BUTTON_RIGHT:
					focusDirection = DIRECTION_RIGHT;
					break;
				case WPAD_NES_BUTTON_DOWN:
					focusDirection = DIRECTION_DOWN;
					break;
				case WPAD_NES_BUTTON_UP:
					focusDirection = DIRECTION_UP;
					break;
				default:
					focusDirection = DIRECTION_NONE;
				}
				if (currentButtonsDownWii & (WPAD_BUTTON_A | WPAD_NES_BUTTON_A)) buttonsDown |= ACTION_SELECT;
				if (currentButtonsDownWii & (WPAD_BUTTON_B | WPAD_NES_BUTTON_B)) buttonsDown |= ACTION_BACK;
				break;
			case WPAD_EXP_GC:
				switch (currentButtonsDownWii & 0xf0000) {
				case WPAD_GC_BUTTON_LEFT:
					focusDirection = DIRECTION_LEFT;
					break;
				case WPAD_GC_BUTTON_RIGHT:
					focusDirection = DIRECTION_RIGHT;
					break;
				case WPAD_GC_BUTTON_DOWN:
					focusDirection = DIRECTION_DOWN;
					break;
				case WPAD_GC_BUTTON_UP:
					focusDirection = DIRECTION_UP;
					break;
				default:
					focusDirection = DIRECTION_NONE;
				}
				if (currentButtonsDownWii & (WPAD_BUTTON_A | WPAD_GC_BUTTON_A)) buttonsDown |= ACTION_SELECT;
				if (currentButtonsDownWii & (WPAD_BUTTON_B | WPAD_GC_BUTTON_B)) buttonsDown |= ACTION_BACK;
				break;
			default:
				if (!wpad[i].ir.raw_valid && wpad[i].exp.type == WPAD_EXP_NONE)
				{
					if (wpad[i].orient.roll <= 0.0f)
					{
						switch (currentButtonsDownWii & 0xf) {
						case WPAD_BUTTON_UP:
							focusDirection = DIRECTION_LEFT;
							break;
						case WPAD_BUTTON_DOWN:
							focusDirection = DIRECTION_RIGHT;
							break;
						case WPAD_BUTTON_LEFT:
							focusDirection = DIRECTION_DOWN;
							break;
						case WPAD_BUTTON_RIGHT:
							focusDirection = DIRECTION_UP;
							break;
						default:
							focusDirection = DIRECTION_NONE;
						}
					}
					else
					{
						switch (currentButtonsDownWii & 0xf) {
						case WPAD_BUTTON_DOWN:
							focusDirection = DIRECTION_LEFT;
							break;
						case WPAD_BUTTON_UP:
							focusDirection = DIRECTION_RIGHT;
							break;
						case WPAD_BUTTON_RIGHT:
							focusDirection = DIRECTION_DOWN;
							break;
						case WPAD_BUTTON_LEFT:
							focusDirection = DIRECTION_UP;
							break;
						default:
							focusDirection = DIRECTION_NONE;
						}
					}
					if (currentButtonsDownWii & WPAD_BUTTON_2) buttonsDown |= ACTION_SELECT;
					if (currentButtonsDownWii & WPAD_BUTTON_1) buttonsDown |= ACTION_BACK;
				}
				else
				{
					switch (currentButtonsDownWii & 0xf) {
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
					if (currentButtonsDownWii & WPAD_BUTTON_A) buttonsDown |= ACTION_SELECT;
					if (currentButtonsDownWii & WPAD_BUTTON_B) buttonsDown |= ACTION_BACK;
				}
				break;
			}
			if (freezeAction)
			{
				focusDirection = DIRECTION_NONE;
				buttonsDown = 0;
			}
			if (primaryFocusOwner) primaryFocusOwner = primaryFocusOwner->updateFocus(focusDirection,buttonsDown);
			else primaryFocusOwner = currentFrame->updateFocus(focusDirection,buttonsDown);
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
	if (focusActive && primaryFocusOwner)
		currentFrame->setDefaultFocus(primaryFocusOwner);
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
