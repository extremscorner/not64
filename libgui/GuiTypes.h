/**
 * Wii64 - GuiTypes.h
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

#ifndef GUIDEFS_H
#define GUIDEFS_H

#include <gccore.h>
#ifdef HW_RVL
#include <wiiuse/wpad.h>
#endif
#include <vector>
#include <stack>
#include <string>
#include <malloc.h>
#include <algorithm>

extern "C" {
#include <string.h>
#include <stdio.h>
}

namespace menu {

class Graphics;
class Component;
class Frame;
class Button;
class Input;
class Cursor;
class Focus;
class Image;
class IplFont;

typedef	std::vector<Frame*> FrameList;
typedef std::vector<Component*> ComponentList;
typedef std::stack<float> FloatStack;
//typedef std::stack<Mtx> MatrixStack;

} //namespace menu 

#endif
