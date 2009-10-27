#include "Image.h"

namespace menu {

Image::Image(void* texture, u16 wd, u16 ht, u8 fmt, u8 wrap_s, u8 wrap_t, u8 mipmap)
		: img_ptr(texture),
		  width(wd),
		  height(ht),
		  format(fmt)
{
	GX_InitTexObj(&obj, img_ptr, width, height, format, wrap_s, wrap_t, mipmap);
}

Image::~Image()
{
}

void Image::activateImage(u8 mapid)
{
	GX_LoadTexObj(&obj, mapid);
}

} //namespace menu 
