/**
 * Wii64 - MessageBox.cpp
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

#include "MessageBox.h"
#include "GraphicsGX.h"
#include "GuiResources.h"
#include "resources.h"
#include "Button.h"
#include "CursorManager.h"
#include "FocusManager.h"
#include "InputManager.h"
#include "IPLFont.h"

namespace menu {

void Func_MessageBoxOK();
void Func_MessageBoxCancel();

#define NUM_FRAME_BUTTONS 3
#define FRAME_BUTTONS messageBoxButtons
#define FRAME_STRINGS messageBoxStrings

static char FRAME_STRINGS[2][7] =
	{ "OK",
	  "Cancel"};

#define MESSAGEBOX_TEXT_WIDTH 512
char messageBoxText[MESSAGEBOX_TEXT_WIDTH];

struct ButtonInfo
{
	menu::Button	*button;
	int				buttonStyle;
	char*			buttonString;
	float			x;
	float			y;
	float			width;
	float			height;
	int				focusUp;
	int				focusDown;
	int				focusLeft;
	int				focusRight;
	ButtonFunc		clickedFunc;
	ButtonFunc		returnFunc;
} FRAME_BUTTONS[NUM_FRAME_BUTTONS] =
{ //	button	buttonStyle	buttonString		x		y		width	height	Up	Dwn	Lft	Rt	clickFunc				returnFunc
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[0],	220.0,	340.0,	200.0,	56.0,	-1,	-1,	-1,	-1,	Func_MessageBoxCancel,	Func_MessageBoxCancel }, // OK
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[0],	145.0,	340.0,	150.0,	56.0,	-1,	-1,	 2,	 2,	Func_MessageBoxOK,		Func_MessageBoxCancel }, // OK (OK/Cancel)
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[1],	345.0,	340.0,	150.0,	56.0,	-1,	-1,	 1,	 1,	Func_MessageBoxCancel,	Func_MessageBoxCancel }, // Cancel (OK/Cancel)
};

MessageBox::MessageBox()
		: buttonImage(0),
		  buttonFocusImage(0),
		  messageBoxActive(false),
		  currentCursorFrame(0),
		  currentFocusFrame(0),
		  returnValue(0)
{
	buttonImage = Resources::getInstance().getImage(Resources::IMAGE_DEFAULT_BUTTON);
	buttonFocusImage = Resources::getInstance().getImage(Resources::IMAGE_DEFAULT_BUTTONFOCUS);

	for (int i = 0; i < NUM_FRAME_BUTTONS; i++)
		FRAME_BUTTONS[i].button = new menu::Button(FRAME_BUTTONS[i].buttonStyle, &FRAME_BUTTONS[i].buttonString, 
										FRAME_BUTTONS[i].x, FRAME_BUTTONS[i].y, 
										FRAME_BUTTONS[i].width, FRAME_BUTTONS[i].height);

	for (int i = 0; i < NUM_FRAME_BUTTONS; i++)
	{
		if (FRAME_BUTTONS[i].focusUp != -1) FRAME_BUTTONS[i].button->setNextFocus(Focus::DIRECTION_UP, FRAME_BUTTONS[FRAME_BUTTONS[i].focusUp].button);
		if (FRAME_BUTTONS[i].focusDown != -1) FRAME_BUTTONS[i].button->setNextFocus(Focus::DIRECTION_DOWN, FRAME_BUTTONS[FRAME_BUTTONS[i].focusDown].button);
		if (FRAME_BUTTONS[i].focusLeft != -1) FRAME_BUTTONS[i].button->setNextFocus(Focus::DIRECTION_LEFT, FRAME_BUTTONS[FRAME_BUTTONS[i].focusLeft].button);
		if (FRAME_BUTTONS[i].focusRight != -1) FRAME_BUTTONS[i].button->setNextFocus(Focus::DIRECTION_RIGHT, FRAME_BUTTONS[FRAME_BUTTONS[i].focusRight].button);
		FRAME_BUTTONS[i].button->setActive(true);
		if (FRAME_BUTTONS[i].clickedFunc) FRAME_BUTTONS[i].button->setClicked(FRAME_BUTTONS[i].clickedFunc);
		if (FRAME_BUTTONS[i].returnFunc) FRAME_BUTTONS[i].button->setReturn(FRAME_BUTTONS[i].returnFunc);
		add(FRAME_BUTTONS[i].button);
		Cursor::getInstance().addComponent(this, FRAME_BUTTONS[i].button, FRAME_BUTTONS[i].x, 
												FRAME_BUTTONS[i].x+FRAME_BUTTONS[i].width, FRAME_BUTTONS[i].y, 
												FRAME_BUTTONS[i].y+FRAME_BUTTONS[i].height);
	}

	setDefaultFocus(FRAME_BUTTONS[0].button);
	setBackFunc(Func_MessageBoxCancel);
	setEnabled(true);

	showFrame();
	boxColor = (GXColor) {100, 100, 100, 210};
	textColor = (GXColor) {255, 255, 255, 255};

}

MessageBox::~MessageBox()
{
	for (int i = 0; i < NUM_FRAME_BUTTONS; i++)
	{
		Cursor::getInstance().removeComponent(this, FRAME_BUTTONS[i].button);
		delete FRAME_BUTTONS[i].button;
	}
}

void MessageBox::setMessage(const char* string)
{
	if (messageFade > 0.0f) messageFade = 0.0f;
	messageBoxActive = true;
	FRAME_BUTTONS[0].button->setVisible(true);
	FRAME_BUTTONS[0].button->setActive(true);
	FRAME_BUTTONS[1].button->setVisible(false);
	FRAME_BUTTONS[1].button->setActive(false);
	FRAME_BUTTONS[2].button->setVisible(false);
	FRAME_BUTTONS[2].button->setActive(false);
	setDefaultFocus(FRAME_BUTTONS[0].button);

	currentCursorFrame = Cursor::getInstance().getCurrentFrame();
	Cursor::getInstance().setCurrentFrame(this);
	currentFocusFrame = Focus::getInstance().getCurrentFrame();
	Focus::getInstance().setCurrentFrame(this);
	memset(messageBoxText, 0, MESSAGEBOX_TEXT_WIDTH);
	strncpy(messageBoxText, string, MESSAGEBOX_TEXT_WIDTH);

	while (messageBoxActive)
		menu::Gui::getInstance().draw();
//	Input::getInstance().clearInputData();
}

int MessageBox::askMessage(const char* string)
{
	if (messageFade > 0.0f) messageFade = 0.0f;
	messageBoxActive = true;
	FRAME_BUTTONS[0].button->setVisible(false);
	FRAME_BUTTONS[0].button->setActive(false);
	FRAME_BUTTONS[1].button->setVisible(true);
	FRAME_BUTTONS[1].button->setActive(true);
	FRAME_BUTTONS[2].button->setVisible(true);
	FRAME_BUTTONS[2].button->setActive(true);
	setDefaultFocus(FRAME_BUTTONS[2].button);

	currentCursorFrame = Cursor::getInstance().getCurrentFrame();
	Cursor::getInstance().setCurrentFrame(this);
	currentFocusFrame = Focus::getInstance().getCurrentFrame();
	Focus::getInstance().setCurrentFrame(this);
	memset(messageBoxText, 0, MESSAGEBOX_TEXT_WIDTH);
	strncpy(messageBoxText, string, MESSAGEBOX_TEXT_WIDTH);

	setReturnValue(0);
	while (messageBoxActive)
		menu::Gui::getInstance().draw();
//	Input::getInstance().clearInputData();
	return getReturnValue();
}

void MessageBox::fadeMessage(const char* string)
{
	messageBoxActive = true;
	FRAME_BUTTONS[0].button->setVisible(false);
	FRAME_BUTTONS[0].button->setActive(false);
	FRAME_BUTTONS[1].button->setVisible(false);
	FRAME_BUTTONS[1].button->setActive(false);
	FRAME_BUTTONS[2].button->setVisible(false);
	FRAME_BUTTONS[2].button->setActive(false);
	setDefaultFocus(this);

//	currentCursorFrame = Cursor::getInstance().getCurrentFrame();
//	Cursor::getInstance().setCurrentFrame(this);
//	currentFocusFrame = Focus::getInstance().getCurrentFrame();
//	Focus::getInstance().setCurrentFrame(this);
	memset(messageBoxText, 0, MESSAGEBOX_TEXT_WIDTH);
	strncpy(messageBoxText, string, MESSAGEBOX_TEXT_WIDTH);

	messageFade = 1.0f;

//	while (messageBoxActive)
//		menu::Gui::getInstance().draw();
//	Input::getInstance().clearInputData();
}

void MessageBox::setReturnValue(int returnVal)
{
	returnValue = returnVal;
}

int MessageBox::getReturnValue()
{
	return returnValue;
}

void MessageBox::deactivate()
{
	messageBoxActive = false;
	if (currentCursorFrame) Cursor::getInstance().setCurrentFrame(currentCursorFrame);
	currentCursorFrame = NULL;
	Focus::getInstance().clearPrimaryFocus();
	if (currentFocusFrame) Focus::getInstance().setCurrentFrame(currentFocusFrame);
	currentFocusFrame = NULL;
}

bool MessageBox::getActive()
{
	return messageBoxActive;
}

void MessageBox::drawMessageBox(Graphics& gfx)
{
	GXColor tmpBoxColor = boxColor;
	GXColor tmpTextColor = textColor;
	float x = 40; float y = 60; float width = 560; float height = 360;
	if (messageFade > 0.0f)
	{
		tmpBoxColor.a = tmpBoxColor.a*messageFade;
		tmpTextColor.a = tmpTextColor.a*messageFade;
		y = 140;
		height = 200;
		messageFade -= 0.01f;
		if (!(messageFade > 0.0f)) messageBoxActive = false;
	}

	gfx.setColor(tmpBoxColor);

	gfx.enableBlending(true);
	gfx.setTEV(GX_MODULATE);

	gfx.setDepth(-10.0f);
	gfx.newModelView();
	gfx.loadModelView();
	gfx.loadOrthographic();

	buttonImage->activateImage(GX_TEXMAP0);
	gfx.drawImage(0, x, y, width/2, height/2, 0.0, width/16.0, 0.0, height/16.0);
	gfx.drawImage(0, x+width/2, y, width/2, height/2, width/16.0, 0.0, 0.0, height/16.0);
	gfx.drawImage(0, x, y+height/2, width/2, height/2, 0.0, width/16.0, height/16.0, 0.0);
	gfx.drawImage(0, x+width/2, y+height/2, width/2, height/2, width/16.0, 0.0, height/16.0, 0.0);

	//detect number of lines
#define MAX_LINES 12
	int ind = 0;
	int numLines = 0;
	int lineStart[MAX_LINES];
	int lineEnd[MAX_LINES];
	lineStart[numLines] = ind;
	while (1)
	{
		if ((messageBoxText[ind]=='\n') || (messageBoxText[ind]=='\0')) 
		{
			lineEnd[numLines++] = ind;
			if (numLines==MAX_LINES) break;
			if (messageBoxText[ind]=='\0') break;
			lineStart[numLines] = ind+1;
		}
		ind++;
	}
	
	int lineSpacing = 20 + 2*MIN((MAX_LINES - numLines),6);
	char tempStr[256];
	IplFont::getInstance().drawInit(tmpTextColor);
	for (int i = 0; i < numLines; i++)
	{
		int heightOffset = -lineSpacing*numLines/2+lineSpacing*i;
		int numChar = lineEnd[i]-lineStart[i]; 
		numChar = numChar <= 255 ? numChar : 255;
		strncpy(tempStr,&messageBoxText[lineStart[i]],numChar);
		tempStr[numChar] = '\0';
		IplFont::getInstance().drawString((int) (x+width/2), (int) (y+height/2)-20+heightOffset, tempStr, 1.0, true);
	}
//	IplFont::getInstance().drawString((int) (x+width/2), (int) (y+height/2)-20+heightOffset, messageBoxText, 1.0, true);

	drawChildren(gfx);

}

void Func_MessageBoxOK()
{
	MessageBox::getInstance().setReturnValue(1);
	MessageBox::getInstance().deactivate();
}

void Func_MessageBoxCancel()
{
	MessageBox::getInstance().deactivate();
}

} //namespace menu 
