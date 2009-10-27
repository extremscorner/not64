#ifndef IMAGE_H
#define IMAGE_H

#include "GuiTypes.h"

namespace menu {

class Image
{
public:
	Image(void* texture, u16 wd, u16 ht, u8 fmt, u8 wrap_s, u8 wrap_t, u8 mipmap);
	~Image();
	void activateImage(u8 mapid);

private:
	GXTexObj obj;
	void *img_ptr;
	u16 width, height;
	u8 format;

};

} //namespace menu 

#endif
