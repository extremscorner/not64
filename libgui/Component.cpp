#include "Component.h"

namespace menu {

Component::Component()
		: componentList(0),
		  visible(true),
		  enabled(false),
		  focus(false),
		  parent(0),
		  type(TYPE_OTHER)
{
	nextFocus[0] = nextFocus[1] = nextFocus[2] = nextFocus[3] = nextFocus[4] = NULL;
}

Component::~Component()
{
}

void Component::setEnabled(bool enable)
{
//	printf("Component setEnabled\n");
	enabled = enable;
}

bool Component::isEnabled() const
{
	return enabled;
}

void Component::setVisible(bool visibleBool)
{
	visible = visibleBool;
}

bool Component::isVisible() const
{
	return visible;
}

void Component::setFocus(bool focusBool)
{
	focus = focusBool;
}

bool Component::getFocus()
{
	return focus;
}

void Component::setType(int typeInt)
{
	type = typeInt;
}

int Component::getType()
{
	return type;
}

void Component::setParent(Component* parent)
{
	this->parent = parent;
}

Component* Component::getParent() const
{
	return parent;
}

void Component::draw(Graphics& gfx)
{
	//push Graphics Context to Component variables
	//Draw object
	//maybe call paintComponent() and then paintChildren()
	//pop Graphics context
//	printf("Component draw\n");
	if (isVisible())
		drawComponent(gfx);
}

void Component::drawComponent(Graphics& gfx)
{
//	printf("virtual Component drawComponent\n");
	//Overload for custom draw routine
}

void Component::drawChildren(Graphics& gfx) const
{
	//Overload when Component class contains children
}

void Component::updateTime(float deltaTime)
{
	//Overload in Component class
	//Add interpolator class & update here?
}

void Component::setNextFocus(int direction, Component* component)
{
	nextFocus[direction] = component;
}

Component* Component::getNextFocus(int direction)
{
	return nextFocus[direction];
}

Component*  Component::updateFocus(int direction, int buttonsPressed)
{
	//Overload in Component class
	return NULL;
}

/*void Component::doFocusEvent(int type, bool buttonDown)
{
	//Overload in Component class
}*/

} //namespace menu 
