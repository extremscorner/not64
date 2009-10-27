#ifndef COMPONENT_H
#define COMPONENT_H

#include "GuiTypes.h"

namespace menu {

class Component
{
public:
	Component();
	virtual ~Component();
	virtual void setEnabled(bool enable);
	bool isEnabled() const;
	void setVisible(bool visible);
	bool isVisible() const;
	void setFocus(bool focus);
	bool getFocus();
	void setType(int type);
	int getType();
	void setParent(Component* parent);
	Component* getParent() const;
	void draw(Graphics& gfx);
	virtual void drawComponent(Graphics& gfx);
	virtual void drawChildren(Graphics& gfx) const;
	virtual void updateTime(float deltaTime);
	void setNextFocus(int direction, Component* component);
	Component* getNextFocus(int direction);
	virtual Component* updateFocus(int direction, int buttonsPressed);
//	virtual void doFocusEvent(int type, bool buttonDown);
	enum ComponentType
	{
		TYPE_OTHER,
		TYPE_FRAME,
		TYPE_BUTTON
	};

protected:
	ComponentList componentList;

private:
	bool visible, enabled, focus;
	Component* parent;
	Component* nextFocus[5];
	int type;

};

} //namespace menu 

#endif
