#ifndef TEXTBOX_H
#define TEXTBOX_H

//#include "GuiTypes.h"
#include "Component.h"

namespace menu {

class TextBox : public Component
{
public:
	TextBox(char** label, float x, float y, float scale, bool centered);
	~TextBox();
	void setColor(GXColor *labelColor);
	void setText(char** strPtr);
	void drawComponent(Graphics& gfx);

private:
	bool centered;
	char** textBoxText;
	float x, y, scale;
	GXColor	labelColor;

};

} //namespace menu 

#endif
