#ifndef CURSORMANAGER_H
#define CURSORMANAGER_H

#include "GuiTypes.h"

namespace menu {

class Cursor
{
public:
	void updateCursor();
	void drawCursor(Graphics& gfx);
	void setCursorFocus(Component* component);
	void addComponent(Frame* frame, Component* component, float x1, float x2, float y1, float y2);
	void removeComponent(Frame* frame, Component* component);
	void setCurrentFrame(Frame* frame);
	Frame* getCurrentFrame();
	void clearInputData();
	void clearCursorFocus();
	static Cursor& getInstance()
	{
		static Cursor obj;
		return obj;
	}

private:
	Cursor();
	~Cursor();

	class CursorEntry
	{
	public:
		Frame *frame;
		Component *comp;
		float xRange[2], yRange[2];
	};

	Frame *currentFrame;
	std::vector<CursorEntry> cursorList;
	Image *pointerImage, *grabImage;
	float cursorX, cursorY, cursorRot, imageCenterX, imageCenterY;
	Component *foundComponent, *hoverOverComponent;
	bool pressed, frameSwitch, clearInput;
	int buttonsPressed, previousButtonsPressed[4], activeChan;
};

} //namespace menu 

#endif
