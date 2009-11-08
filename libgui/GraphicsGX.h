/**
 * Wii64 - GraphicsGX.h
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

#ifndef GRAPHICSGX_H
#define GRAPHICSGX_H

#include "GuiTypes.h"
#include <gccore.h>

namespace menu {

class Graphics
{
public:
	Graphics(GXRModeObj *vmode);
	~Graphics();
	void init();
	void drawInit();
	void swapBuffers();
	void clearEFB(GXColor color, u32 zvalue);
	void newModelView();
	void translate(float x, float y, float z);
	void translateApply(float x, float y, float z);
	void rotate(float degrees);
	void loadModelView();
	void loadOrthographic();
	void setDepth(float newDepth);
	float getDepth();
	void setColor(GXColor color);
	void setColor(GXColor* color);
	void drawRect(int x, int y, int width, int height);
	void fillRect(int x, int y, int width, int height);
	void drawImage(int textureId, int x, int y, int width, int height, float s1, float s2, float t1, float t2);
	void drawLine(int x1, int y1, int x2, int y2);
	void drawString(int x, int y, std::string str);
	void drawPoint(int x, int y, int radius);
	void setLineWidth(int width);
	void pushDepth(float d);
	void popDepth();
	void enableScissor(int x, int y, int width, int height);
	void disableScissor();
	void enableBlending(bool blend);
	void setTEV(int tev_op);
	void pushTransparency(float f);
	void popTransparency();
	void setTransparency(float f);
	float getTransparency();

private:
	void applyCurrentColor();
	float getCurrentTransparency(int index);
	GXRModeObj *vmode;
	GXRModeObj vmode_phys;
	int which_fb;
	bool first_frame;
	void *xfb[2];
	float depth, transparency;
	float viewportWidth, viewportHeight;
	FloatStack depthStack, transparencyStack;
	GXColor currentColor[4], appliedColor[4];
	Mtx currentModelViewMtx;
	Mtx44 currentProjectionMtx;
//	MatrixStack modelViewMtxStack;
};

} //namespace menu 

#endif
