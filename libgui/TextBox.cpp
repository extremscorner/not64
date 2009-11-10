/**
 * Wii64 - TextBox.cpp
 * Copyright (C) 2009 sepp256
 *
 * Wii64 homepage: http://www.emulatemii.com
 * email address: sepp256@gmail.com
 *
 *
 * This program is free software; you can redistribute it and/
 * or modify it under the terms of the GNU General Public Li-
 * cence as published by the Free Software Foundation; either
 * version 2 of the Licence, or any later version.
 *
 * This program is distributed in the hope that it will be use-
 * ful, but WITHOUT ANY WARRANTY; without even the implied war-
 * ranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public Licence for more details.
 *
**/

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
//	GXColor color = {255, 255, 255, 255};
	GXColor color = {56, 56, 56, 255};
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
