/**
 * Wii64 - Image.h
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

#ifndef IMAGE_H
#define IMAGE_H

#include "GuiTypes.h"

namespace menu {

class Image
{
public:
	Image(void* texture, u16 wd, u16 ht, u8 fmt, u8 wrap_s, u8 wrap_t, u8 mipmap);
	Image(void* texture, u16 wd, u16 ht, u8 fmt, u8 wrap_s, u8 wrap_t, u8 mipmap, void* lut, u8 lut_fmt, u8 lut_name, u16 lut_size);
	~Image();
	void activateImage(u8 mapid);

private:
	GXTexObj obj;
	GXTlutObj tlut_obj;
	void *img_ptr;
	void *tlut_ptr;
	u16 width, height;
	u8 format, tlut_format, tlut_name;
	u16 tlut_size;

};

} //namespace menu 

#endif
