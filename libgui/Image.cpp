#include "Image.h"

namespace menu {

Image::Image(void* texture, u16 wd, u16 ht, u8 fmt, u8 wrap_s, u8 wrap_t, u8 mipmap)
		: img_ptr(texture),
		  tlut_ptr(0),
		  width(wd),
		  height(ht),
		  format(fmt),
		  tlut_format(0)
{
	GX_InitTexObj(&obj, img_ptr, width, height, format, wrap_s, wrap_t, mipmap);
}

Image::Image(void* texture, u16 wd, u16 ht, u8 fmt, u8 wrap_s, u8 wrap_t, u8 mipmap, void* lut, u8 lut_fmt, u8 lut_name, u16 lut_size)
		: img_ptr(texture),
		  tlut_ptr(lut),
		  width(wd),
		  height(ht),
		  format(fmt),
		  tlut_format(lut_fmt),
		  tlut_name(lut_name),
		  tlut_size(lut_size)
{
	GX_InitTlutObj(&tlut_obj, tlut_ptr, tlut_format, tlut_size);
	GX_InitTexObjCI(&obj, img_ptr, width, height, format, wrap_s, wrap_t, mipmap, tlut_name);
}

Image::~Image()
{
}

void Image::activateImage(u8 mapid)
{
	if (tlut_ptr) GX_LoadTlut(&tlut_obj, tlut_name);	
	GX_LoadTexObj(&obj, mapid);
}

} //namespace menu 
