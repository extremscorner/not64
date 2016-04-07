/**
 * Wii64 - GraphicsGX.cpp
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

#include <math.h>
#include "GraphicsGX.h"
#include "../main/wii64config.h"

#ifdef HW_RVL
#include "../gc_memory/MEM2.h"
#endif

void video_mode_init(GXRModeObj *rmode, unsigned int *fb1, unsigned int *fb2);

namespace menu {

Graphics::Graphics(GXRModeObj *rmode)
		: vmode(rmode),
		  which_fb(0),
		  first_frame(true),
		  depth(-10.0f),
		  transparency(1.0f)
{
	VIDEO_Init();
	switch (videoMode)
	{
	case VIDEOMODE_AUTO:
		vmode = VIDEO_GetPreferredMode(&vmode_phys);
		break;
	case VIDEOMODE_PAL60:
		vmode = &TVEurgb60Hz480IntDf;
		memcpy( &vmode_phys, vmode, sizeof(GXRModeObj));
		break;
	case VIDEOMODE_240P:
		vmode = &TVEurgb60Hz240DsAa;
		memcpy( &vmode_phys, vmode, sizeof(GXRModeObj));
		break;
	case VIDEOMODE_480P:
		vmode = &TVEurgb60Hz480Prog;
		memcpy( &vmode_phys, vmode, sizeof(GXRModeObj));
		break;
	case VIDEOMODE_PAL:
		vmode = &TVPal576IntDfScale;
		memcpy( &vmode_phys, vmode, sizeof(GXRModeObj));
		break;
	case VIDEOMODE_288P:
		vmode = &TVPal288DsAaScale;
		memcpy( &vmode_phys, vmode, sizeof(GXRModeObj));
		break;
	case VIDEOMODE_576P:
		vmode = &TVPal576ProgScale;
		memcpy( &vmode_phys, vmode, sizeof(GXRModeObj));
		break;
	}

	vmode->viWidth = 720;
	vmode->viXOrigin = 0;
#ifdef HW_RVL
	VIDEO_SetTrapFilter(trapFilter);
	if(screenMode)	VIDEO_SetAspectRatio(VI_DISPLAY_BOTH, VI_ASPECT_1_1);
	else			VIDEO_SetAspectRatio(VI_DISPLAY_BOTH, VI_ASPECT_3_4);
#endif

	VIDEO_Configure(vmode);

#ifdef HW_RVL
	xfb[0] = XFB0_LO;
	xfb[1] = XFB1_LO;
#else
	xfb[0] = SYS_AllocateFramebuffer(vmode);
	xfb[1] = SYS_AllocateFramebuffer(vmode);
#endif

	VIDEO_SetNextFramebuffer(xfb[which_fb]);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	which_fb ^= 1;

	//Pass vmode, xfb[0] and xfb[1] back to main program
	video_mode_init(vmode, (unsigned int*)xfb[0], (unsigned int*)xfb[1]);

	//Perform GX init stuff here?
	//GX_init here or in main?
	init();
}

Graphics::~Graphics()
{
}

void Graphics::init()
{
	f32 yscale;
	void *gpfifo = NULL;

	gpfifo = memalign(32,GX_FIFO_MINSIZE);
	GX_Init(gpfifo,GX_FIFO_MINSIZE);

	yscale = GX_GetYScaleFactor(vmode->efbHeight,vmode->xfbHeight);
	GX_SetDispCopyYScale(yscale);
	GX_SetDispCopySrc(0,0,vmode->fbWidth,vmode->efbHeight);
	GX_SetDispCopyDst(vmode->fbWidth,vmode->xfbHeight);
	GX_SetCopyFilter(vmode->aa,vmode->sample_pattern,GX_TRUE,vmode->vfilter);
	GX_SetFieldMode(GX_DISABLE,((vmode->viHeight==2*vmode->xfbHeight)?GX_ENABLE:GX_DISABLE));
 
	if (vmode->aa)
		GX_SetPixelFmt(GX_PF_RGB565_Z16, GX_ZC_LINEAR);
}

void Graphics::drawInit()
{
	// Reset various parameters from gfx plugin
	GX_SetZTexture(GX_ZT_DISABLE,GX_TF_Z16,0);	//GX_ZT_DISABLE or GX_ZT_REPLACE; set in gDP.cpp
	GX_SetZCompLoc(GX_TRUE);	// Do Z-compare before texturing.
	GX_SetFog(GX_FOG_NONE,0,1,0,1,(GXColor){0,0,0,255});
	GX_SetViewport(0,0,vmode->fbWidth,vmode->efbHeight,0,1);
	GX_SetCoPlanar(GX_DISABLE);
	GX_SetClipMode(GX_CLIP_ENABLE);
	GX_SetScissor(0,0,vmode->fbWidth,vmode->efbHeight);
	GX_SetAlphaCompare(GX_ALWAYS,0,GX_AOP_AND,GX_ALWAYS,0);

	GX_SetZMode(GX_ENABLE,GX_ALWAYS,GX_TRUE);

	//set blend mode
	GX_SetBlendMode(GX_BM_NONE, GX_BL_ONE, GX_BL_ZERO, GX_LO_CLEAR); //Fix src alpha
	GX_SetColorUpdate(GX_ENABLE);
	GX_SetAlphaUpdate(GX_ENABLE);
	GX_SetDstAlpha(GX_DISABLE, 0xFF);
	//set cull mode
	GX_SetCullMode (GX_CULL_NONE);

	GX_InvVtxCache();
	GX_InvalidateTexAll();

	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_NRM, GX_NRM_XYZ, GX_F32, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);

	GX_ClearVtxDesc();
	GX_SetVtxDesc(GX_VA_PTNMTXIDX, GX_PNMTX0);
	GX_SetVtxDesc(GX_VA_TEX0MTXIDX, GX_TEXMTX0);
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);

	setTEV(GX_PASSCLR);
	newModelView();
	loadModelView();
	loadOrthographic();
}

void Graphics::swapBuffers()
{
	GX_SetCopyClear((GXColor){0, 0, 0, 0xFF}, GX_MAX_Z24);
	GX_CopyDisp(xfb[which_fb],GX_TRUE);
	GX_Flush();

	VIDEO_SetNextFramebuffer(xfb[which_fb]);
	if(first_frame) {
		first_frame = false;
		VIDEO_SetBlack(false);
	}
	VIDEO_Flush();
 	VIDEO_WaitVSync();
	which_fb ^= 1;
}

void Graphics::clearEFB(GXColor color, u32 zvalue)
{
	GX_SetColorUpdate(GX_ENABLE);
	GX_SetAlphaUpdate(GX_ENABLE);
	GX_SetZMode(GX_ENABLE,GX_ALWAYS,GX_TRUE);
	GX_SetCopyClear(color, zvalue);
	GX_CopyDisp(xfb[which_fb],GX_TRUE);
	GX_Flush();
}

void Graphics::newModelView()
{
	guMtxIdentity(currentModelViewMtx);
}

void Graphics::translate(float x, float y, float z)
{
	Mtx tmp;
	guMtxTrans (tmp, x, y, z);
	guMtxConcat (currentModelViewMtx, tmp, currentModelViewMtx);
}

void Graphics::translateApply(float x, float y, float z)
{
	guMtxTransApply(currentModelViewMtx,currentModelViewMtx,x,y,z);
}

void Graphics::rotate(float degrees)
{
	guMtxRotDeg(currentModelViewMtx,'Z',degrees);
}

void Graphics::loadModelView()
{
	GX_LoadPosMtxImm(currentModelViewMtx,GX_PNMTX0);
}

void Graphics::loadOrthographic()
{
	if(screenMode)	guOrtho(currentProjectionMtx, 0, 480, -104, 744, 0, 700);
	else			guOrtho(currentProjectionMtx, 0, 480, 0, 640, 0, 700);
	GX_LoadProjectionMtx(currentProjectionMtx, GX_ORTHOGRAPHIC);
}

void Graphics::setDepth(float newDepth)
{
	depth = newDepth;
}

float Graphics::getDepth()
{
	return depth;
}

void Graphics::setColor(GXColor color)
{
	for (int i = 0; i < 4; i++){
		currentColor[i].r = color.r;
		currentColor[i].g = color.g;
		currentColor[i].b = color.b;
		currentColor[i].a = color.a;
	}
	applyCurrentColor();
}

void Graphics::setColor(GXColor* color)
{
	for (int i = 0; i < 4; i++){
		currentColor[i].r = color[i].r;
		currentColor[i].g = color[i].g;
		currentColor[i].b = color[i].b;
		currentColor[i].a = color[i].a;
	}
	applyCurrentColor();
}

void Graphics::drawRect(int x, int y, int width, int height)
{
	GX_Begin(GX_LINESTRIP, GX_VTXFMT0, 4);
		GX_Position3f32((float) x,(float) y, depth );
		GX_Color4u8(appliedColor[0].r, appliedColor[0].g, appliedColor[0].b, appliedColor[0].a);
		GX_TexCoord2f32(0.0f,0.0f);
		GX_Position3f32((float) (x+width),(float) y, depth );
		GX_Color4u8(appliedColor[1].r, appliedColor[1].g, appliedColor[1].b, appliedColor[1].a);
		GX_TexCoord2f32(0.0f,0.0f);
		GX_Position3f32((float) (x+width),(float) (y+height), depth );
		GX_Color4u8(appliedColor[2].r, appliedColor[2].g, appliedColor[2].b, appliedColor[2].a);
		GX_TexCoord2f32(0.0f,0.0f);
		GX_Position3f32((float) x,(float) (y+height), depth );
		GX_Color4u8(appliedColor[3].r, appliedColor[3].g, appliedColor[3].b, appliedColor[3].a);
		GX_TexCoord2f32(0.0f,0.0f);
	GX_End();
}

void Graphics::fillRect(int x, int y, int width, int height)
{
	GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
		GX_Position3f32((float) x,(float) y, depth );
		GX_Color4u8(appliedColor[0].r, appliedColor[0].g, appliedColor[0].b, appliedColor[0].a);
		GX_TexCoord2f32(0.0f,0.0f);
		GX_Position3f32((float) (x+width),(float) y, depth );
		GX_Color4u8(appliedColor[1].r, appliedColor[1].g, appliedColor[1].b, appliedColor[1].a);
		GX_TexCoord2f32(0.0f,0.0f);
		GX_Position3f32((float) (x+width),(float) (y+height), depth );
		GX_Color4u8(appliedColor[2].r, appliedColor[2].g, appliedColor[2].b, appliedColor[2].a);
		GX_TexCoord2f32(0.0f,0.0f);
		GX_Position3f32((float) x,(float) (y+height), depth );
		GX_Color4u8(appliedColor[3].r, appliedColor[3].g, appliedColor[3].b, appliedColor[3].a);
		GX_TexCoord2f32(0.0f,0.0f);
	GX_End();
}

void Graphics::drawImage(int textureId, int x, int y, int width, int height, float s1, float s2, float t1, float t2)
{
	//Init texture here or in calling code?
	GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
		GX_Position3f32((float) x,(float) y, depth );
		GX_Color4u8(appliedColor[0].r, appliedColor[0].g, appliedColor[0].b, appliedColor[0].a);
		GX_TexCoord2f32(s1,t1);
		GX_Position3f32((float) (x+width),(float) y, depth );
		GX_Color4u8(appliedColor[1].r, appliedColor[1].g, appliedColor[1].b, appliedColor[1].a);
		GX_TexCoord2f32(s2,t1);
		GX_Position3f32((float) (x+width),(float) (y+height), depth );
		GX_Color4u8(appliedColor[2].r, appliedColor[2].g, appliedColor[2].b, appliedColor[2].a);
		GX_TexCoord2f32(s2,t2);
		GX_Position3f32((float) x,(float) (y+height), depth );
		GX_Color4u8(appliedColor[3].r, appliedColor[3].g, appliedColor[3].b, appliedColor[3].a);
		GX_TexCoord2f32(s1,t2);
	GX_End();
}

void Graphics::drawLine(int x1, int y1, int x2, int y2)
{
	GX_Begin(GX_LINES, GX_VTXFMT0, 2);
		GX_Position3f32((float) x1,(float) y1, depth );
		GX_Color4u8(appliedColor[0].r, appliedColor[0].g, appliedColor[0].b, appliedColor[0].a);
		GX_TexCoord2f32(0.0f,0.0f);
		GX_Position3f32((float) x2,(float) y2, depth );
		GX_Color4u8(appliedColor[1].r, appliedColor[1].g, appliedColor[1].b, appliedColor[1].a);
		GX_TexCoord2f32(0.0f,0.0f);
	GX_End();
}

void Graphics::drawCircle(int x, int y, int radius, int numSegments)
{

	GX_Begin(GX_LINESTRIP, GX_VTXFMT0, numSegments+1);

	for (int i = 0; i<=numSegments; i++)
	{
		float angle, point_x, point_y;
		angle = M_TWOPI * i/numSegments;
		point_x = (float)x + (float)radius * cos( angle );
		point_y = (float)y + (float)radius * sin( angle );

		GX_Position3f32((float) point_x,(float) point_y, depth );
		GX_Color4u8(appliedColor[0].r, appliedColor[0].g, appliedColor[0].b, appliedColor[0].a);
		GX_TexCoord2f32(0.0f,0.0f);
	}

	GX_End();
}

void Graphics::drawString(int x, int y, std::string str)
{
	//todo
}

void Graphics::drawPoint(int x, int y, int radius)
{
	GX_SetPointSize(u8 (radius *3 ),GX_TO_ZERO);
	GX_Begin(GX_POINTS, GX_VTXFMT0, 1);
		GX_Position3f32((float) x,(float) y, depth );
		GX_Color4u8(appliedColor[0].r, appliedColor[0].g, appliedColor[0].b, appliedColor[0].a);
		GX_TexCoord2f32(0.0f,0.0f);
	GX_End();
}

void Graphics::setLineWidth(int width)
{
	GX_SetLineWidth((u8) (width * 6), GX_TO_ZERO );
}

void Graphics::pushDepth(float d)
{
	depthStack.push(getDepth());
	setDepth(d);
}

void Graphics::popDepth()
{
	depthStack.pop();
	if(depthStack.size() != 0)
	{
		setDepth(depthStack.top());
	}
	else
	{
		setDepth(1.0f);
	}
}

void Graphics::enableScissor(int x, int y, int width, int height)
{
	if(screenMode)
	{
		int x1 = (x+104)*640/848;
		int x2 = (x+width+104)*640/848;
		GX_SetScissor((u32) x1,(u32) y*vmode->efbHeight/480,(u32) x2-x1,(u32) height*vmode->efbHeight/480);
	}
	else
		GX_SetScissor((u32) x,(u32) y*vmode->efbHeight/480,(u32) width,(u32) height*vmode->efbHeight/480);
}

void Graphics::disableScissor()
{
	GX_SetScissor((u32) 0,(u32) 0,(u32) vmode->fbWidth,(u32) vmode->efbHeight);
}

void Graphics::enableBlending(bool blend)
{
	if (blend)
		GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
	else
		GX_SetBlendMode(GX_BM_NONE, GX_BL_ONE, GX_BL_ZERO, GX_LO_CLEAR);
}

void Graphics::setTEV(int tev_op)
{
	GX_SetNumTevStages(1);
	GX_SetNumChans (1);
	GX_SetNumTexGens (1);
	GX_SetTevOrder (GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
	GX_SetTevOp (GX_TEVSTAGE0, tev_op);
}

void Graphics::pushTransparency(float f)
{
	transparencyStack.push(getTransparency());
	setTransparency(f);
}

void Graphics::popTransparency()
{
	transparencyStack.pop();
	if(transparencyStack.size() != 0)
	{
		setTransparency(transparencyStack.top());
	}
	else
	{
		setTransparency(1.0f);
	}
}

void Graphics::setTransparency(float f)
{
	transparency = f;
	applyCurrentColor();
}

float Graphics::getTransparency()
{
	return transparency;
}

void Graphics::applyCurrentColor()
{
	for (int i = 0; i < 4; i++){
		appliedColor[i].r = currentColor[i].r;
		appliedColor[i].g = currentColor[i].g;
		appliedColor[i].b = currentColor[i].b;
		appliedColor[i].a = (u8) (getCurrentTransparency(i) * 255.0f);
	}
}

float Graphics::getCurrentTransparency(int index)
{
	float alpha = (float)currentColor[index].a/255.0f;
	float val = alpha * transparency;
	return val;
}

} //namespace menu 
