/* vi_GX.cpp - vi for Gamecube, based off vi_GX
   by Mike Slegeir for Mupen64-GC
 */

#include <stdio.h>
#include <ogc/system.h>
#include <ogc/video.h>
#include <ogc/gx.h>
#include <ogc/gu.h>
#include "vi_GX.h"
#include "font.h"

VI_GX::VI_GX(GFX_INFO info) : VI(info), width(0), height(0), which_fb(1){
	init_font();
	// FIXME: Instead of creating our own fb, we should use main's
	//xfb[0] = (unsigned int*) MEM_K0_TO_K1(SYS_AllocateFramebuffer(&TVNtsc480IntDf));
	//xfb[1] = (unsigned int*) MEM_K0_TO_K1(SYS_AllocateFramebuffer(&TVNtsc480IntDf));
}

void VI_GX::setFB(unsigned int* fb1, unsigned int* fb2){
	xfb[0] = fb1;
	xfb[1] = fb2;
}

VI_GX::~VI_GX(){
	// FIXME: If we create our own fb, we need to free it somehow
}

void VI_GX::setVideoMode(int w, int h){
   width = w;
   height = h;
}

void VI_GX::switchFullScreenMode(){ }

void VI_GX::switchWindowMode(){ }

void* VI_GX::getScreenPointer(){ return xfb[which_fb]; }

void VI_GX::blit(){
	//printf("Should be blitting.");
    GX_DrawDone (); //needed?
	GX_CopyDisp (xfb[which_fb], GX_FALSE); //TODO: Figure out where the UpdateScreen interrupts are coming from!
    GX_Flush (); //needed?
	showFPS();
	VIDEO_SetNextFramebuffer(xfb[which_fb]);
	VIDEO_Flush();
	which_fb ^= 1;
	VIDEO_WaitVSync();
}

void VI_GX::setGamma(float gamma){ }

extern "C" {
extern long long gettime();
extern unsigned int diff_sec(long long start,long long end);
};

void VI_GX::showFPS(){
	static long long lastTick=0;
	static int frames=0;
	static char caption[16];
	
	long long nowTick = gettime();
	frames++;
	if (diff_sec(lastTick,nowTick)>=1) {
		sprintf(caption, "%02d FPS",frames);
		frames = 0;
		lastTick = nowTick;
	}
	
	write_font(10,10,caption,xfb,which_fb);
}

void VI_GX::showLoadProg(float percent)
{
	GXColor GXcol1 = {0,128,255,255};
	GXColor GXcol2 = {0,64,128,255};
	float xbar[3] = {425,425,550};
	float ybar[2] = {75,90};

	xbar[1] = xbar[0] + (xbar[2]-xbar[0])*percent;

	guMtxIdentity(GXmodelView2D);
	GX_LoadPosMtxImm(GXmodelView2D,GX_PNMTX2);
	guOrtho(GXprojection2D, 0, 480, 0, 640, 0, 1);
	GX_LoadProjectionMtx(GXprojection2D, GX_ORTHOGRAPHIC); //load current 2D projection matrix
	//draw rectangle from ulx,uly to lrx,lry
	GX_ClearVtxDesc();
	GX_SetVtxDesc(GX_VA_PTNMTXIDX, GX_PNMTX2);
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
	//set vertex attribute formats here
	GX_SetVtxAttrFmt(GX_VTXFMT1, GX_VA_POS, GX_POS_XY, GX_F32, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT1, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);

	//disable textures
	GX_SetNumChans (1);
	GX_SetNumTexGens (0);
	GX_SetTevOrder (GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
	GX_SetTevOp (GX_TEVSTAGE0, GX_PASSCLR);
	//set blend mode
	GX_SetBlendMode(GX_BM_NONE, GX_BL_ONE, GX_BL_ZERO, GX_LO_CLEAR); //Fix src alpha
	GX_SetColorUpdate(GX_ENABLE);
	GX_SetAlphaUpdate(GX_ENABLE);
	GX_SetDstAlpha(GX_DISABLE, 0xFF);
	//set cull mode
	GX_SetCullMode (GX_CULL_NONE);

	GX_Begin(GX_QUADS, GX_VTXFMT1, 8);
	// background rectangle
	GX_Position2f32(xbar[0], ybar[0]);
	GX_Color4u8(GXcol2.r, GXcol2.g, GXcol2.b, GXcol2.a);
	GX_Position2f32(xbar[2], ybar[0]);
	GX_Color4u8(GXcol2.r, GXcol2.g, GXcol2.b, GXcol2.a);
	GX_Position2f32(xbar[2], ybar[1]);
	GX_Color4u8(GXcol2.r, GXcol2.g, GXcol2.b, GXcol2.a);
	GX_Position2f32(xbar[0], ybar[1]);
	GX_Color4u8(GXcol2.r, GXcol2.g, GXcol2.b, GXcol2.a);
	// progress rectangle
	GX_Position2f32(xbar[0], ybar[0]);
	GX_Color4u8(GXcol1.r, GXcol1.g, GXcol1.b, GXcol1.a);
	GX_Position2f32(xbar[1], ybar[0]);
	GX_Color4u8(GXcol1.r, GXcol1.g, GXcol1.b, GXcol1.a);
	GX_Position2f32(xbar[1], ybar[1]);
	GX_Color4u8(GXcol1.r, GXcol1.g, GXcol1.b, GXcol1.a);
	GX_Position2f32(xbar[0], ybar[1]);
	GX_Color4u8(GXcol1.r, GXcol1.g, GXcol1.b, GXcol1.a);
	GX_End();

//    GX_DrawDone ();
	GX_CopyDisp (xfb[which_fb], GX_FALSE);
    GX_Flush ();
	VIDEO_SetNextFramebuffer(xfb[which_fb]);
	VIDEO_Flush();
	which_fb ^= 1;
	VIDEO_WaitVSync();
}
