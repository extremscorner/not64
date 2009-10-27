#ifndef FRAME_H
#define FRAME_H

#include "GuiTypes.h"
#include "Component.h"

typedef void (*FrameFunc)( void );

namespace menu {

class Frame : public Component
{
public:
	Frame();
	~Frame();
	void showFrame();
	void hideFrame();
	void setEnabled(bool enable);
	void drawChildren(Graphics& gfx) const;
	void remove(Component* component);
	void removeAll();
	void add(Component* comp);
	void updateTime(float deltaTime);
	void setDefaultFocus(Component* comp);
	Component* getDefaultFocus();
	void setBackFunc(FrameFunc backFn);
	void setSelectFunc(FrameFunc selectFn);
	Component* updateFocus(int direction, int buttonsPressed);

private:
	Component* defaultFocus;
	FrameFunc backFunc, selectFunc;

};

} //namespace menu 

#endif
