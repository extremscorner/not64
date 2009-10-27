#include "MessageBox.h"
#include "GraphicsGX.h"
#include "resources.h"
#include "Button.h"
#include "CursorManager.h"
#include "FocusManager.h"
#include "InputManager.h"
#include "IPLFont.h"

namespace menu {

void Func_MessageBoxOK();

#define NUM_MESSAGEBOX_BUTTONS 1
#define FRAME_BUTTONS messageBoxButtons
#define FRAME_STRINGS messageBoxStrings

static char FRAME_STRINGS[1][3] =
	{ "OK"};

#define MESSAGEBOX_TEXT_WIDTH 100
char messageBoxText[MESSAGEBOX_TEXT_WIDTH];

struct ButtonInfo
{
	menu::Button	*button;
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
} FRAME_BUTTONS[NUM_MESSAGEBOX_BUTTONS] =
{ //	button	buttonString		x		y		width	height	Up	Dwn	Lft	Rt	clickFunc			returnFunc
	{	NULL,	FRAME_STRINGS[0],	220.0,	340.0,	200.0,	50.0,	-1,	-1,	-1,	-1,	Func_MessageBoxOK,	Func_MessageBoxOK }, // OK
};

MessageBox::MessageBox()
		: buttonImage(0),
		  buttonFocusImage(0),
		  messageBoxActive(false),
		  currentCursorFrame(0),
		  currentFocusFrame(0)
{
	buttonImage = new menu::Image(ButtonTexture, 16, 16, GX_TF_I8, GX_CLAMP, GX_CLAMP, GX_FALSE);
	buttonFocusImage = new menu::Image(ButtonFocusTexture, 16, 16, GX_TF_I8, GX_CLAMP, GX_CLAMP, GX_FALSE);

	FRAME_BUTTONS[0].button = new menu::Button(buttonImage, &FRAME_BUTTONS[0].buttonString, 
										FRAME_BUTTONS[0].x, FRAME_BUTTONS[0].y, 
										FRAME_BUTTONS[0].width, FRAME_BUTTONS[0].height);

	FRAME_BUTTONS[0].button->setFocusImage(buttonFocusImage);
	FRAME_BUTTONS[0].button->setActive(true);
	if (FRAME_BUTTONS[0].clickedFunc) FRAME_BUTTONS[0].button->setClicked(FRAME_BUTTONS[0].clickedFunc);
	if (FRAME_BUTTONS[0].returnFunc) FRAME_BUTTONS[0].button->setReturn(FRAME_BUTTONS[0].returnFunc);
	add(FRAME_BUTTONS[0].button);
	Cursor::getInstance().addComponent(this, FRAME_BUTTONS[0].button, FRAME_BUTTONS[0].x, 
												FRAME_BUTTONS[0].x+FRAME_BUTTONS[0].width, FRAME_BUTTONS[0].y, 
												FRAME_BUTTONS[0].y+FRAME_BUTTONS[0].height);
	setDefaultFocus(FRAME_BUTTONS[0].button);
	setEnabled(true);

	showFrame();
	boxColor = (GXColor) {100, 100, 100, 200};
	textColor = (GXColor) {255, 255, 255, 255};

}

MessageBox::~MessageBox()
{
	Cursor::getInstance().removeComponent(this, FRAME_BUTTONS[0].button);
	delete FRAME_BUTTONS[0].button;
	delete buttonFocusImage;
	delete buttonImage;
}

void MessageBox::setMessage(const char* string)
{
	messageBoxActive = true;
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

void MessageBox::deactivate()
{
	messageBoxActive = false;
	if (currentCursorFrame) Cursor::getInstance().setCurrentFrame(currentCursorFrame);
	currentCursorFrame = NULL;
	if (currentFocusFrame) Focus::getInstance().setCurrentFrame(currentFocusFrame);
	currentFocusFrame = NULL;
}

bool MessageBox::getActive()
{
	return messageBoxActive;
}

void MessageBox::drawMessageBox(Graphics& gfx)
{
	gfx.setColor(boxColor);

	gfx.enableBlending(true);
	gfx.setTEV(GX_MODULATE);

	gfx.setDepth(-10.0f);
	gfx.newModelView();
	gfx.loadModelView();
	gfx.loadOrthographic();

	float x = 40; float y = 60; float width = 560; float height = 360;
	buttonImage->activateImage(GX_TEXMAP0);
	gfx.drawImage(0, x, y, width/2, height/2, 0.0, width/16.0, 0.0, height/16.0);
	gfx.drawImage(0, x+width/2, y, width/2, height/2, width/16.0, 0.0, 0.0, height/16.0);
	gfx.drawImage(0, x, y+height/2, width/2, height/2, 0.0, width/16.0, height/16.0, 0.0);
	gfx.drawImage(0, x+width/2, y+height/2, width/2, height/2, width/16.0, 0.0, height/16.0, 0.0);

	IplFont::getInstance().drawInit(textColor);
	IplFont::getInstance().drawString((int) (x+width/2), (int) (y+height/2)-20, messageBoxText, 1.0, true);

	drawChildren(gfx);
}

void Func_MessageBoxOK()
{
	MessageBox::getInstance().deactivate();
}

} //namespace menu 
