#include "Gui.h"
#include "IPLFont.h"
#include "InputManager.h"
#include "CursorManager.h"
#include "FocusManager.h"
#include "MessageBox.h"
#include "LoadingBar.h"

namespace menu {

Gui::Gui()
{
}

Gui::~Gui()
{
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
	FrameList::const_iterator iteration;
	for (iteration = frameList.begin(); iteration != frameList.end(); iteration++)
	{
		(*iteration)->drawChildren(*gfx);
	}
	if (MessageBox::getInstance().getActive()) MessageBox::getInstance().drawMessageBox(*gfx);
	if (LoadingBar::getInstance().getActive()) LoadingBar::getInstance().drawLoadingBar(*gfx);
	Cursor::getInstance().drawCursor(*gfx);

	gfx->swapBuffers();
}

} //namespace menu 
