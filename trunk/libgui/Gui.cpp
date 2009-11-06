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
#ifdef WII
#include <di/di.h>
#endif 
}

extern char shutdown;

namespace menu {

Gui::Gui()
	: fade(9)
{
	menuLogo = new Logo();
	menuLogo->setLocation(580.0, 70.0, -50.0);
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
	//Update time??
	//Get graphics framework and pass to Frame draw fns?
	gfx->drawInit();
	//Draw Menu Backdrop
	Resources::getInstance().getImage(Resources::IMAGE_MENU_BACKGROUND)->activateImage(GX_TEXMAP0);
	gfx->setTEV(GX_REPLACE);
	gfx->enableBlending(false);
	if(screenMode)	gfx->drawImage(0, 0, 0, 640, 480, 0, 1, 0, 1);
	else			gfx->drawImage(0, 0, 0, 640, 480, (848.0-640.0)/2/848.0, 1.0 - (848.0-640.0)/2/848.0, 0, 1);
	FrameList::const_iterator iteration;
	for (iteration = frameList.begin(); iteration != frameList.end(); iteration++)
	{
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
		gfx->fillRect(0, 0, 640, 480);
		
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
				DI_Close();
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

} //namespace menu 
