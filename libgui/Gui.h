/**
 * Wii64 - Gui.h
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

#ifndef GUI_H
#define GUI_H

#include "GuiTypes.h"
#include "Frame.h"
#include "Logo.h"
#include "GraphicsGX.h"

namespace menu {

class Gui
{
public:
	void setVmode(GXRModeObj *rmode);
	void addFrame(Frame* frame);
	void removeFrame(Frame* frame);
	void draw();
	void drawBackground();
	static Gui& getInstance()
	{
		static Gui obj;
		return obj;
	}
	Graphics *gfx;
	Logo* menuLogo;

private:
	Gui();
	~Gui();
	FrameList frameList;
	char fade;
};

} //namespace menu 

#endif
