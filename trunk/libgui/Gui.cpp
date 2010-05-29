/**
 * Wii64 - Gui.cpp
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

#include "Gui.h"
#include "IPLFont.h"
#include "InputManager.h"
#include "CursorManager.h"
#include "FocusManager.h"
#include "MessageBox.h"
#include "LoadingBar.h"
#include "GuiResources.h"
#include "../main/wii64config.h"

extern "C" {
#include "../gc_input/controller.h"
#ifdef WII
#include <di/di.h>
#endif 
}

extern char shutdown;
extern unsigned int dvd_hard_init;

namespace menu {

Gui::Gui()
	: fade(9)
{
	menuLogo = new Logo();
	menuLogo->setLocation(570.0, 70.0, -150.0);
	menuLogo->setVisible(true);
}

Gui::~Gui()
{
	delete menuLogo;
	delete gfx;
}

void Gui::setVmode(GXRModeObj *vmode)
{
	gfx = new Graphics(vmode);
	IplFont::getInstance().setVmode(vmode);
}

void Gui::addFrame(Frame *frame)
{
	frameList.push_back(frame);
}

void Gui::removeFrame(Frame *frame)
{
	frameList.erase(std::remove(frameList.begin(),frameList.end(),frame),frameList.end());
}

void Gui::draw()
{
//	printf("Gui draw\n");
	Input::getInstance().refreshInput();
	Cursor::getInstance().updateCursor();
	Focus::getInstance().updateFocus();
	if(padAutoAssign) auto_assign_controllers();
	//Update time??
	//Get graphics framework and pass to Frame draw fns?
	gfx->drawInit();
	drawBackground();
	FrameList::const_iterator iteration;
	for (iteration = frameList.begin(); iteration != frameList.end(); iteration++)
	{
		(*iteration)->updateTime(0.0f); //TODO: Pass deltaTime
		(*iteration)->drawChildren(*gfx);
	}
//	menuLogo->drawComponent(*gfx);
	menuLogo->draw(*gfx);
	if (MessageBox::getInstance().getActive()) MessageBox::getInstance().drawMessageBox(*gfx);
	if (LoadingBar::getInstance().getActive()) LoadingBar::getInstance().drawLoadingBar(*gfx);
	Cursor::getInstance().drawCursor(*gfx);

	if(shutdown)
	{
		Cursor::getInstance().setFreezeAction(true);
		Focus::getInstance().setFreezeAction(true);
		gfx->enableBlending(true);
		gfx->setTEV(GX_PASSCLR);
		gfx->setDepth(-10.0f);
		gfx->newModelView();
		gfx->loadModelView();
		gfx->loadOrthographic();

		gfx->setColor((GXColor){0, 0, 0, fade});
		if(screenMode)	gfx->fillRect(-104, 0, 848, 480);
		else			gfx->fillRect(0, 0, 640, 480);
		
		if(fade == 255)
		{
			VIDEO_SetBlack(true);
			VIDEO_Flush();
		 	VIDEO_WaitVSync();
			if(shutdown==1)	//Power off System
				SYS_ResetSystem(SYS_POWEROFF, 0, 0);
			else			//Return to Loader
			{
#ifdef WII
        if(dvd_hard_init) {
				  DI_Close();
			  }
#endif
				void (*rld)() = (void (*)()) 0x80001800;
				rld();
			}
		}

		char increment = 3;
		fade = fade +increment > 255 ? 255 : fade + increment;
	}

	gfx->swapBuffers();
}

void Gui::drawBackground()
{
	//Draw Menu Backdrop
	Resources::getInstance().getImage(Resources::IMAGE_MENU_BACKGROUND)->activateImage(GX_TEXMAP0);
//	gfx->setTEV(GX_REPLACE);
	GXColor muxCol = (GXColor){0,17,85,255};
	GX_SetTevColor(GX_TEVREG0,muxCol);
	GX_SetTevColorIn(GX_TEVSTAGE0,GX_CC_C0,GX_CC_ZERO,GX_CC_TEXC,GX_CC_TEXC);
	GX_SetTevColorOp(GX_TEVSTAGE0,GX_TEV_ADD,GX_TB_ZERO,GX_CS_SCALE_1,GX_TRUE,GX_TEVPREV);
	gfx->enableBlending(false);
	if(screenMode)	gfx->drawImage(0, -104, 0, 848, 480, 0, 1, 0, 1);
	else			gfx->drawImage(0, 0, 0, 640, 480, (848.0-640.0)/2/848.0, 1.0 - (848.0-640.0)/2/848.0, 0, 1);
	gfx->setTEV(GX_PASSCLR);
}

} //namespace menu 
