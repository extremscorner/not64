/**
 * Wii64 - gui_GX-menu.h
 * Copyright (C) 2007, 2008, 2009 sepp256
 *
 * gui that uses GX graphics
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

#ifndef GUI_GX_H
#define GUI_GX_H

#include "GUI.h"
#include <gccore.h>

typedef struct
{
   unsigned int* xfb[2];
   int which_fb;
   int width;
   int height;
} GUIinfo;


void GUI_setFB(unsigned int*, unsigned int*);
void GUI_init();
void GUI_toggle();
void GUI_draw();
void GUI_displayText();
int GUI_loadBGtex();
void GUI_drawWiiN64(float, float, float, float);
void GUI_drawLogo(float, float, float);
void GUI_centerText(bool);
void GUI_setLoadProg(float percent);
void GUI_drawLoadProg();
void GUI_creditScreen();
void GUI_writeString(guVector pos, guVector axis1, guVector axis2, guVector axis3, guVector rot, GXColor fontColor, char *string, float scale);

#endif
