/**
 * Wii64 - font.h
 * Copyright (C) 2007, 2008, 2009 Wii64 Team
 *
 * IPL font functions	
 * modified by sepp256 to work as textures instead of writing to the xfb
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

#ifdef __cplusplus
extern "C" {
#endif
extern void init_font(void);
void write_font_init_GX(GXColor fontColor);
void write_font_color(GXColor* fontColorPtr);
void write_font(int x, int y, char *string, float scale);
void write_font_centered(int y, char *string, float scale);
void write_font_origin(char *string, float scale);

#ifdef __cplusplus
}
#endif
