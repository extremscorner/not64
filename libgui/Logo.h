#ifndef LOGO_H
#define LOGO_H

//#include "GuiTypes.h"
#include "Component.h"

namespace menu {

class Logo : public Component
{
public:
	Logo();
	~Logo();
	void setLocation(float x, float y, float z);
	void setSize(float size);
	void setMode(int mode);
	void updateTime(float deltaTime);
	void drawComponent(Graphics& gfx);
	enum LogoMode
	{
		LOGO_N=0,
		LOGO_M,
		LOGO_W
	};

private:
	void drawQuad(u8 v0, u8 v1, u8 v2, u8 v3, u8 c);
	int logoMode;
	float x, y, z, size;
	float rotateAuto, rotateX, rotateY;
/*	unsigned long StartTime;
	float x, y, width, height;
	GXColor	focusColor, inactiveColor, activeColor, selectedColor, labelColor;*/

};

} //namespace menu 

#endif
