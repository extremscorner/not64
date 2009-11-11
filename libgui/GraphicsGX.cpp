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

#include "GraphicsGX.h"
#include "../main/wii64config.h"

#define DEFAULT_FIFO_SIZE		(256 * 1024)


extern "C" unsigned int usleep(unsigned int us);
void video_mode_init(GXRModeObj *rmode, unsigned int *fb1, unsigned int *fb2);

namespace menu {

Graphics::Graphics(GXRModeObj *rmode)
		: vmode(rmode),
		  which_fb(0),
		  first_frame(true),
		  depth(1.0f),
		  transparency(1.0f),
		  viewportWidth(640.0f),
		  viewportHeight(480.0f)
{
//	printf("Graphics constructor\n");

	setColor((GXColor) {0,0,0,0});

#ifdef HW_RVL
	CONF_Init();
#endif
	VIDEO_Init();
	//vmode = VIDEO_GetPreferredMode(NULL);
	vmode = VIDEO_GetPreferredMode(&vmode_phys);
#if 0
	if(CONF_GetAspectRatio()) {
		vmode->viWidth = 678;
		vmode->viXOrigin = (VI_MAX_WIDTH_PAL - 678) / 2;
	}
#endif
	vmode->efbHeight = viewportHeight;

	VIDEO_Configure(vmode);
	

	xfb[0] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(vmode));
	xfb[1] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(vmode));

	console_init (xfb[0], 20, 64, vmode->fbWidth, vmode->xfbHeight, vmode->fbWidth * 2);

	VIDEO_SetNextFramebuffer(xfb[which_fb]);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	if(vmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();
	which_fb ^= 1;

	//Pass vmode, xfb[0] and xfb[1] back to main program
	video_mode_init(vmode, (unsigned int*)xfb[0], (unsigned int*)xfb[1]);

	//Perform GX init stuff here?
	//GX_init here or in main?
	//GX_SetViewport( 0.0f, 0.0f, viewportWidth, viewportHeight );
	init();
}

Graphics::~Graphics()
{
}

void Graphics::init()
{

	f32 yscale;
	u32 xfbHeight;
	void *gpfifo = NULL;
	GXColor background = {0, 0, 0, 0xff};

	gpfifo = memalign(32,DEFAULT_FIFO_SIZE);
	memset(gpfifo,0,DEFAULT_FIFO_SIZE);
	GX_Init(gpfifo,DEFAULT_FIFO_SIZE);
	GX_SetCopyClear(background, GX_MAX_Z24);

	GX_SetViewport(0,0,vmode->fbWidth,vmode->efbHeight,0,1);
	yscale = GX_GetYScaleFactor(vmode->efbHeight,vmode->xfbHeight);
	xfbHeight = GX_SetDispCopyYScale(yscale);
	GX_SetScissor(0,0,vmode->fbWidth,vmode->efbHeight);
	GX_SetDispCopySrc(0,0,vmode->fbWidth,vmode->efbHeight);
	GX_SetDispCopyDst(vmode->fbWidth,xfbHeight);
	GX_SetCopyFilter(vmode->aa,vmode->sample_pattern,GX_TRUE,vmode->vfilter);
	GX_SetFieldMode(vmode->field_rendering,((vmode->viHeight==2*vmode->xfbHeight)?GX_ENABLE:GX_DISABLE));
 
	if (vmode->aa)
        GX_SetPixelFmt(GX_PF_RGB565_Z16, GX_ZC_LINEAR);
    else
        GX_SetPixelFmt(GX_PF_RGB8_Z24, GX_ZC_LINEAR);

	GX_SetCullMode(GX_CULL_NONE);
	GX_SetDispCopyGamma(GX_GM_1_0);

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
//	printf("Graphics swapBuffers\n");
//	if(which_fb==1) usleep(1000000);
	GX_SetCopyClear((GXColor){0, 0, 0, 0xFF}, GX_MAX_Z24);
	GX_CopyDisp(xfb[which_fb],GX_TRUE);
	GX_Flush();

	VIDEO_SetNextFramebuffer(xfb[which_fb]);
	if(first_frame) {
		first_frame = false;
		VIDEO_SetBlack(GX_FALSE);
	}
	VIDEO_Flush();
 	VIDEO_WaitVSync();
	which_fb ^= 1;
//	printf("Graphics endSwapBuffers\n");
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
	if(screenMode)	guOrtho(currentProjectionMtx, 0, 479, -104, 743, 0, 700);
	else			guOrtho(currentProjectionMtx, 0, 479, 0, 639, 0, 700);
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
		GX_SetScissor((u32) x1,(u32) y,(u32) x2-x1,(u32) height);
	}
	else
		GX_SetScissor((u32) x,(u32) y,(u32) width,(u32) height);
}

void Graphics::disableScissor()
{
	GX_SetScissor((u32) 0,(u32) 0,(u32) viewportWidth,(u32) viewportHeight); //Set to the same size as the viewport.
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
