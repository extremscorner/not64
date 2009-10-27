#include "LoadingBar.h"
#include "GraphicsGX.h"
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
	buttonImage = new menu::Image(ButtonTexture, 16, 16, GX_TF_I8, GX_CLAMP, GX_CLAMP, GX_FALSE);
	buttonFocusImage = new menu::Image(ButtonFocusTexture, 16, 16, GX_TF_I8, GX_CLAMP, GX_CLAMP, GX_FALSE);

	showFrame();

	boxColor = (GXColor) {100, 100, 100, 200};
	backColor = (GXColor) {0, 0, 0, 255};
	barColor = (GXColor) {255, 100, 100, 255};
	textColor = (GXColor) {255, 255, 255, 255};

}

LoadingBar::~LoadingBar()
{
	delete buttonFocusImage;
	delete buttonImage;
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
	IplFont::getInstance().drawString((int) (x+width/2), (int) (y+height/2), loadingBarText, 0.7, true);
}

extern "C" {
	void LoadingBar_showBar(float percent, const char* string);
}

void LoadingBar_showBar(float percent, const char* string)
{
	LoadingBar::getInstance().showBar(percent, string);
}

} //namespace menu 
