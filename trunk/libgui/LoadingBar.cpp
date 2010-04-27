/**
 * Wii64 - LoadingBar.cpp
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

#include "LoadingBar.h"
#include "GraphicsGX.h"
#include "GuiResources.h"
#include "resources.h"
#include "CursorManager.h"
#include "FocusManager.h"
#include "IPLFont.h"
#include <stdio.h>
#include <debug.h>

namespace menu {

#define LOADINGBAR_TEXT_WIDTH 100
char loadingBarText[LOADINGBAR_TEXT_WIDTH];

LoadingBar::LoadingBar()
		: buttonImage(0),
		  buttonFocusImage(0),
		  loadingBarActive(false),
		  currentCursorFrame(0),
		  currentFocusFrame(0),
		  percentComplete(0.0f)
{
	buttonImage = Resources::getInstance().getImage(Resources::IMAGE_DEFAULT_BUTTON);
	buttonFocusImage = Resources::getInstance().getImage(Resources::IMAGE_DEFAULT_BUTTONFOCUS);

	showFrame();

	boxColor = (GXColor) {100, 100, 100, 210};
	backColor = (GXColor) {0, 0, 0, 210};
//	barColor = (GXColor) {255, 100, 100, 255};
	barColor = (GXColor) {104, 125, 205, 210};
	textColor = (GXColor) {255, 255, 255, 255};

}

LoadingBar::~LoadingBar()
{
}

void LoadingBar::showBar(float percent, const char* string)
{
	loadingBarActive = true;
	currentCursorFrame = Cursor::getInstance().getCurrentFrame();
	Cursor::getInstance().setCurrentFrame(NULL);
	currentFocusFrame = Focus::getInstance().getCurrentFrame();
	Focus::getInstance().setCurrentFrame(NULL);
	percentComplete = percent;
	memset(loadingBarText, 0, LOADINGBAR_TEXT_WIDTH);
	strncpy(loadingBarText, string, LOADINGBAR_TEXT_WIDTH);

	menu::Gui::getInstance().draw();

	loadingBarActive = false;
	if (currentCursorFrame) Cursor::getInstance().setCurrentFrame(currentCursorFrame);
	currentCursorFrame = NULL;
	if (currentFocusFrame) Focus::getInstance().setCurrentFrame(currentFocusFrame);
	currentFocusFrame = NULL;
}

bool LoadingBar::getActive()
{
	return loadingBarActive;
}

void LoadingBar::drawLoadingBar(Graphics& gfx)
{
	gfx.setColor(boxColor);

	gfx.enableBlending(true);
	gfx.setTEV(GX_MODULATE);

	gfx.setDepth(-10.0f);
	gfx.newModelView();
	gfx.loadModelView();
	gfx.loadOrthographic();

	//draw box
	float x = 120; float y = 160; float width = 400; float height = 150;
	buttonImage->activateImage(GX_TEXMAP0);
	gfx.drawImage(0, x, y, width/2, height/2, 0.0, width/16.0, 0.0, height/16.0);
	gfx.drawImage(0, x+width/2, y, width/2, height/2, width/16.0, 0.0, 0.0, height/16.0);
	gfx.drawImage(0, x, y+height/2, width/2, height/2, 0.0, width/16.0, height/16.0, 0.0);
	gfx.drawImage(0, x+width/2, y+height/2, width/2, height/2, width/16.0, 0.0, height/16.0, 0.0);

	//draw bar background
	gfx.setColor(backColor);
	x = 140; y = 245; width = 360; height = 30;
	buttonImage->activateImage(GX_TEXMAP0);
	gfx.drawImage(0, x, y, width/2, height/2, 0.0, width/16.0, 0.0, height/16.0);
	gfx.drawImage(0, x+width/2, y, width/2, height/2, width/16.0, 0.0, 0.0, height/16.0);
	gfx.drawImage(0, x, y+height/2, width/2, height/2, 0.0, width/16.0, height/16.0, 0.0);
	gfx.drawImage(0, x+width/2, y+height/2, width/2, height/2, width/16.0, 0.0, height/16.0, 0.0);

	//draw bar
	gfx.setColor(barColor);
	x = 140; y = 245; width = 360*percentComplete; height = 30;
	buttonImage->activateImage(GX_TEXMAP0);
	gfx.drawImage(0, x, y, width/2, height/2, 0.0, width/16.0, 0.0, height/16.0);
	gfx.drawImage(0, x+width/2, y, width/2, height/2, width/16.0, 0.0, 0.0, height/16.0);
	gfx.drawImage(0, x, y+height/2, width/2, height/2, 0.0, width/16.0, height/16.0, 0.0);
	gfx.drawImage(0, x+width/2, y+height/2, width/2, height/2, width/16.0, 0.0, height/16.0, 0.0);

	//draw text 'box'
	x = 120; y = 200; width = 400; height = 40;
	IplFont::getInstance().drawInit(textColor);
	IplFont::getInstance().drawString((int) (x+width/2), (int) (y+height/2), loadingBarText, 0.9, true);
}

extern "C" {
	void LoadingBar_showBar(float percent, const char* string);
}

void LoadingBar_showBar(float percent, const char* string)
{
	LoadingBar::getInstance().showBar(percent, string);
}

} //namespace menu 
