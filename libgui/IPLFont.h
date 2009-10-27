#ifndef IPLFONT_H
#define IPLFONT_H

#include "GuiTypes.h"

namespace menu {

class IplFont
{
public:
	void setVmode(GXRModeObj *rmode);
	void drawInit(GXColor fontColor);
	void setColor(GXColor fontColor);
	void setColor(GXColor* fontColorPtr);
	void drawString(int x, int y, char *string, float scale, bool centered);
	void drawStringAtOrigin(char *string, float scale);
	static IplFont& getInstance()
	{
		static IplFont obj;
		return obj;
	}

private:
	IplFont();
	~IplFont();
	void initFont();
	void setIplConfig(unsigned char c);
	void decodeYay0(void *src, void *dst);
	void convertI2toI4(void *dst, void *src, int xres, int yres);

	typedef struct {
		u16 s[256], t[256], font_size[256], fheight;
	} CHAR_INFO;

#ifdef HW_RVL
	unsigned char *fontFont;
#else //GC
	unsigned char fontFont[ 0x40000 ] __attribute__((aligned(32)));
#endif

	u16 frameWidth;
	CHAR_INFO fontChars;
	GXTexObj fontTexObj;
	GXRModeObj *vmode;
	GXColor fontColor;

};

} //namespace menu 

#endif
