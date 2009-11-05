#ifndef FOCUSMANAGER_H
#define FOCUSMANAGER_H

#include "GuiTypes.h"

namespace menu {

class Focus
{
public:
	void updateFocus();
	void addComponent(Component* component);
	void removeComponent(Component* component);
	Frame* getCurrentFrame();
	void setCurrentFrame(Frame* frame);
	void setFocusActive(bool active);
	void clearInputData();
	void clearPrimaryFocus();
	void setFreezeAction(bool freezeAction);
	enum FocusDirection
	{
		DIRECTION_NONE=0,
		DIRECTION_LEFT,
		DIRECTION_RIGHT,
		DIRECTION_DOWN,
		DIRECTION_UP
	};
	enum FocusAction
	{
		ACTION_SELECT=1,
		ACTION_BACK=2
	};

	static Focus& getInstance()
	{
		static Focus obj;
		return obj;
	}

private:
	Focus();
	~Focus();
	bool focusActive, pressed, frameSwitch, clearInput, freezeAction;
	int buttonsPressed, previousButtonsPressed;
	u32 previousButtonsWii[4];
	u16 previousButtonsGC[4];
	ComponentList focusList;
	Component *primaryFocusOwner, *secondaryFocusOwner;
	Frame *currentFrame;

};

} //namespace menu 

#endif
