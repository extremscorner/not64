#include "TextBox.h"
#include "GraphicsGX.h"
#include "IPLFont.h"

namespace menu {

TextBox::TextBox(char** label, float x, float y, float scale, bool centered)
		: centered(centered),
		  textBoxText(label),
		  x(x),
		  y(y),
		  scale(scale)
{
	setType(TYPE_TEXTBOX);
					//Label color
	GXColor color = {255, 255, 255, 255};
	setColor(&color);
}

TextBox::~TextBox()
{
}

void TextBox::setColor(GXColor *colors)
{
	labelColor.r = colors[0].r;
	labelColor.g = colors[0].g;
	labelColor.b = colors[0].b;
	labelColor.a = colors[0].a;
}

void TextBox::setText(char** strPtr)
{
	textBoxText = strPtr;
}

void TextBox::drawComponent(Graphics& gfx)
{
	IplFont::getInstance().drawInit(labelColor);
	IplFont::getInstance().drawString((int) x, (int) y, *textBoxText, scale, centered);
}

} //namespace menu 
