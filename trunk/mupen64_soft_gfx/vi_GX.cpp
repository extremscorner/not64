/**
 * Wii64 - vi_GX.cpp
 * Copyright (C) 2007, 2008, 2009 Wii64 Team
 * 
 * vi for Gamecube by Mike Slegeir
 * load progress bar and GX hardware scaling by sepp256
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

#include <stdio.h>
#include <ogc/system.h>
#include <ogc/video.h>
#include "vi_GX.h"
//#include "font.h" //This file is outdated
#include "../gui/font.h"
#include "../gui/DEBUG.h"


//Global variables set by menu
//Note: DEBUG&FPS functionality is not available in soft_gfx
char printToScreen;
char showFPSonScreen;


VI_GX::VI_GX(GFX_INFO info) : VI(info), width(0), height(0), which_fb(1){
	init_font();
	updateOSD = true;
	copy_fb = false;
}

void VI_GX::setFB(unsigned int* fb1, unsigned int* fb2){
	xfb[0] = fb1;
	xfb[1] = fb2;
}

VI_GX::~VI_GX(){
}

void VI_GX::setVideoMode(int w, int h){
   width = w;
   height = h;
}

void VI_GX::switchFullScreenMode(){ }

void VI_GX::switchWindowMode(){ }

unsigned int* VI_GX::getScreenPointer(){ return xfb[which_fb]; }

void VI_GX::blit(){
	showFPS();
	showDEBUG();
	if(updateOSD)
	{
		if(copy_fb)
			VIDEO_WaitVSync();
		GX_CopyDisp (xfb[which_fb], GX_FALSE); 
		GX_DrawDone();
//		doCaptureScreen();
		updateOSD = false;
		copy_fb = true;
	}
	//The following has been moved to the Pre-Retrace callback
/*	VIDEO_SetNextFramebuffer(xfb[which_fb]);
	VIDEO_Flush();
	which_fb ^= 1;
	VIDEO_WaitVSync();*/
}

void VI_GX::setGamma(float gamma){ }

extern "C" {
extern long long gettime();
extern unsigned int diff_sec(long long start,long long end);
};

void VI_GX::showFPS(){
	static long long lastTick=0;
	static int frames=0;
	static int VIs=0;
	static char caption[20];
	
	long long nowTick = gettime();
	VIs++;
	if (updateOSD)
		frames++;
	if (diff_sec(lastTick,nowTick)>=1) {
		//Note that for soft_gfx we're only counting VI's, not frames (dlists)
		sprintf(caption, "%02d VI/s",VIs);
		frames = 0;
		VIs = 0;
		lastTick = nowTick;
	}
	
	if (updateOSD)
	{
		GXColor fontColor = {150,255,150,255};
		write_font_init_GX(fontColor);
		if(showFPSonScreen)
			write_font(10,35,caption, 1.0);

		//reset swap table from GUI/DEBUG
		GX_SetTevSwapModeTable(GX_TEV_SWAP0, GX_CH_RED, GX_CH_GREEN, GX_CH_BLUE, GX_CH_ALPHA);
		GX_SetTevSwapMode(GX_TEVSTAGE0, GX_TEV_SWAP0, GX_TEV_SWAP0);
	}
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

void VI_GX::updateDEBUG()
{
	updateOSD = true;
}

extern char text[DEBUG_TEXT_HEIGHT][DEBUG_TEXT_WIDTH];

void VI_GX::showDEBUG()
{
	if (updateOSD)
	{
		int i = 0;
		GXColor fontColor = {150, 255, 150, 255};
		DEBUG_update();
		write_font_init_GX(fontColor);
		if(printToScreen)
			for (i=0;i<DEBUG_TEXT_HEIGHT;i++)
				write_font(10,(10*i+60),text[i], 0.5); 
		
	   //reset swap table from GUI/DEBUG
		GX_SetTevSwapModeTable(GX_TEV_SWAP0, GX_CH_RED, GX_CH_GREEN, GX_CH_BLUE, GX_CH_ALPHA);
		GX_SetTevSwapMode(GX_TEVSTAGE0, GX_TEV_SWAP0, GX_TEV_SWAP0);
	}
}

void VI_GX::PreRetraceCallback(u32 retraceCnt)
{
	if(copy_fb)
	{
		VIDEO_SetNextFramebuffer(xfb[which_fb]);
		VIDEO_Flush();
		which_fb ^= 1;
		copy_fb = false;
	}
}
