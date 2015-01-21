/**
 * glN64_GX - VI.cpp
 * Copyright (C) 2003 Orkin
 * Copyright (C) 2008, 2009 sepp256 (Port to Wii/Gamecube/PS3)
 *
 * glN64 homepage: http://gln64.emulation64.com
 * Wii64 homepage: http://www.emulatemii.com
 * email address: sepp256@gmail.com
 *
**/

#ifdef __GX__
#include <stdio.h>
#include <gccore.h>
#include <malloc.h>
#include <ogc/lwp_heap.h>
# ifdef MENU_V2
#include "../libgui/IPLFont.h"
#include "../menu/MenuResources.h"
# else // MENU_V2
#include "../gui/font.h"
# endif //!MENU_V2
#include "../gui/DEBUG.h"
#include "../main/timers.h"
//#include "Textures.h"
#endif // __GX__

#include <math.h>
#include "glN64.h"
#include "Types.h"
#include "VI.h"
#include "OpenGL.h"
#include "N64.h"
#include "gSP.h"
#include "gDP.h"
#include "RSP.h"
#include "FrameBuffer.h"
#include "Debug.h"
#ifdef __GX__
#include "Textures.h"
#endif // __GX__

VIInfo VI;

extern GXRModeObj *vmode, *rmode;
extern int GX_xfb_offset;

#ifdef __GX__
char printToScreen;
char showFPSonScreen;
char renderCpuFramebuffer;

/*bool updateDEBUGflag;
bool new_fb;
unsigned int* xfb[2];
int which_fb;*/
#endif // __GX__

void VI_UpdateSize()
{
	f32 xScale = _FIXED2FLOAT( _SHIFTR( *REG.VI_X_SCALE, 0, 12 ), 10 );
	//f32 xOffset = _FIXED2FLOAT( _SHIFTR( *REG.VI_X_SCALE, 16, 12 ), 10 );

	f32 yScale = _FIXED2FLOAT( _SHIFTR( *REG.VI_Y_SCALE, 0, 12 ), 10 );
	//f32 yOffset = _FIXED2FLOAT( _SHIFTR( *REG.VI_Y_SCALE, 16, 12 ), 10 );

	u32 hEnd = _SHIFTR( *REG.VI_H_START, 0, 10 );
	u32 hStart = _SHIFTR( *REG.VI_H_START, 16, 10 );

	u32 vEnd = _SHIFTR( *REG.VI_V_START, 0, 10 );
	u32 vStart = _SHIFTR( *REG.VI_V_START, 16, 10 );

	VI.width = hEnd == hStart ? *REG.VI_WIDTH :
	           lrintf((hEnd - hStart) * xScale);
	VI.height = lrintf((vEnd - vStart) * yScale
	          * (*REG.VI_WIDTH > VI.width ? 1.0125f : 0.50625f));

	if (VI.width == 0.0f) VI.width = 320.0f;
	if (VI.height == 0.0f) VI.height = 240.0f;
}

void VI_UpdateScreen()
{
#ifndef __GX__
	glFinish();

	if (OGL.frameBufferTextures)
	{
		FrameBuffer *current = FrameBuffer_FindBuffer( *REG.VI_ORIGIN );

		if ((*REG.VI_ORIGIN != VI.lastOrigin) || ((current) && current->changed))
		{
			if (gDP.colorImage.changed)
			{
				FrameBuffer_SaveBuffer( gDP.colorImage.address, gDP.colorImage.size, gDP.colorImage.width, gDP.colorImage.height );
				gDP.colorImage.changed = FALSE;
			}

			FrameBuffer_RenderBuffer( *REG.VI_ORIGIN );

			gDP.colorImage.changed = FALSE;
			VI.lastOrigin = *REG.VI_ORIGIN;
#ifdef DEBUG
			while (Debug.paused && !Debug.step);
			Debug.step = FALSE;
#endif
		}
	}
	else
	{
		if (gSP.changed & CHANGED_COLORBUFFER)
		{
#ifndef __LINUX__
			SwapBuffers( OGL.hDC );
#else
			OGL_SwapBuffers();
#endif
			gSP.changed &= ~CHANGED_COLORBUFFER;
#ifdef DEBUG
			while (Debug.paused && !Debug.step);
			Debug.step = FALSE;
#endif
		}
	}
	glFinish();
#else // !__GX__
	if (renderCpuFramebuffer || (RSP.DList == 0))
	{
		//Only render N64 framebuffer in RDRAM and not EFB
		VI_GX_cleanUp();
		VI_GX_renderCpuFramebuffer();
		VI_GX_showLoadIcon();
		VI_GX_showFPS();
		VI_GX_showDEBUG();
		GX_SetCopyClear ((GXColor){0,0,0,255}, 0xFFFFFF);
		if (VI.copy_fb)	GX_CopyDisp (VI.xfb[VI.which_fb^1]+GX_xfb_offset, GX_TRUE);
		else			GX_CopyDisp (VI.xfb[VI.which_fb]+GX_xfb_offset, GX_TRUE);
		GX_DrawDone(); //Wait until EFB->XFB copy is complete
		VI.enableLoadIcon = false;
		VI.copy_fb = true;
	}
	else if (OGL.frameBufferTextures)
	{
		FrameBuffer *current = FrameBuffer_FindBuffer( *REG.VI_ORIGIN );

		if ((*REG.VI_ORIGIN != VI.lastOrigin) || ((current) && current->changed))
		{
			FrameBuffer_IncrementVIcount();
			if (gDP.colorImage.changed)
			{
				FrameBuffer_SaveBuffer( gDP.colorImage.address, gDP.colorImage.size, gDP.colorImage.width, gDP.colorImage.height );
				gDP.colorImage.changed = FALSE;
			}

			FrameBuffer_RenderBuffer( *REG.VI_ORIGIN );

			//Draw DEBUG to screen
			VI_GX_cleanUp();
			VI_GX_showLoadIcon();
			VI_GX_showFPS();
			VI_GX_showDEBUG();
			GX_SetCopyClear ((GXColor){0,0,0,255}, 0xFFFFFF);
			//Copy EFB->XFB
			if (VI.copy_fb)	GX_CopyDisp (VI.xfb[VI.which_fb^1]+GX_xfb_offset, GX_TRUE);
			else			GX_CopyDisp (VI.xfb[VI.which_fb]+GX_xfb_offset, GX_TRUE);
			GX_DrawDone(); //Wait until EFB->XFB copy is complete
			VI.enableLoadIcon = false;
			VI.copy_fb = true;

			//Restore current EFB
			FrameBuffer_RestoreBuffer( gDP.colorImage.address, gDP.colorImage.size, gDP.colorImage.width );

			gDP.colorImage.changed = FALSE;
			VI.lastOrigin = *REG.VI_ORIGIN;
		}
	}
	else
	{
		if (gSP.changed & CHANGED_COLORBUFFER)
		{
			VI_GX_cleanUp();
			VI_GX_showLoadIcon();
			VI_GX_showFPS();
			VI_GX_showDEBUG();
			GX_SetCopyClear ((GXColor){0,0,0,255}, 0xFFFFFF);
			if (VI.copy_fb)	GX_CopyDisp (VI.xfb[VI.which_fb^1]+GX_xfb_offset, GX_TRUE);
			else			GX_CopyDisp (VI.xfb[VI.which_fb]+GX_xfb_offset, GX_TRUE);
			GX_DrawDone(); //Wait until EFB->XFB copy is complete
			VI.enableLoadIcon = false;
			VI.copy_fb = true;
			gSP.changed &= ~CHANGED_COLORBUFFER;
		}
	}
#endif // __GX__

}

#ifdef __GX__
extern "C" {
extern long long gettime();
extern unsigned int diff_sec(long long start,long long end);
};

void VI_GX_init() {
	//init_font();
/*	updateDEBUGflag = true;
	new_fb = false;
	which_fb = 1;*/
	VI.updateOSD = true;
	VI.enableLoadIcon = false;
	VI.EFBcleared = true;
	VI.copy_fb = false;
	VI.which_fb = 1;
}

void VI_GX_setFB(unsigned int* fb1, unsigned int* fb2){
	VI.xfb[0] = fb1;
	VI.xfb[1] = fb2;
}

unsigned int* VI_GX_getScreenPointer(){ return VI.xfb[VI.which_fb]; }

void VI_GX_clearEFB(){
	GX_SetZMode(GX_ENABLE,GX_ALWAYS,GX_TRUE);
	GX_SetCopyClear ((GXColor){0,0,0,255}, 0xFFFFFF);
	GX_CopyDisp (VI.xfb[VI.which_fb]+GX_xfb_offset, GX_TRUE);	//clear the EFB before executing new Dlist
	GX_DrawDone(); //Wait until EFB->XFB copy is complete
}

extern timers Timers;
extern float VILimit;

void VI_GX_showFPS(){
	char caption[50];

	sprintf(caption, "%.1f VI/s (%.1fx), %.1f DL/s",Timers.vis,Timers.vis/VILimit,Timers.fps);
	
	GXColor fontColor = {150,255,150,255};
#ifndef MENU_V2
	write_font_init_GX(fontColor);
	if(showFPSonScreen)
		write_font(15,35,caption, 1.0);
#else
	menu::IplFont::getInstance().drawInit(fontColor);
	if(showFPSonScreen)
		menu::IplFont::getInstance().drawString(15,35,caption, 1.0, false);
#endif

	//reset swap table from GUI/DEBUG
//	GX_SetTevSwapModeTable(GX_TEV_SWAP0, GX_CH_RED, GX_CH_GREEN, GX_CH_BLUE, GX_CH_ALPHA);
	GX_SetTevSwapMode(GX_TEVSTAGE0, GX_TEV_SWAP0, GX_TEV_SWAP0);
}

void VI_GX_showLoadIcon()
{
	if (!VI.enableLoadIcon)
		return;

#ifndef MENU_V2
	GXColor GXcol1 = {0,128,255,255};
	GXColor GXcol2 = {0,64,128,255};
	float xbar[3] = {425,425,550};
	float ybar[2] = {75,90};
	Mtx44 GXprojection2D;
	Mtx GXmodelView2D;

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
	GX_SetZMode(GX_DISABLE,GX_ALWAYS,GX_FALSE);
	GX_SetZTexture(GX_ZT_DISABLE,GX_TF_Z16,0);	//GX_ZT_DISABLE or GX_ZT_REPLACE; set in gDP.cpp
	GX_SetZCompLoc(GX_TRUE);	// Do Z-compare before texturing.
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
#else //!MENU_V2
	float x = 530;
	float y = 30;
	float width = 80;
	float height = 56;

	Mtx44 GXprojection2D;
	Mtx GXmodelView2D;

	GXTexObj texObj;
//	GX_InitTexObj(&texObj, LoadingTexture, width, height, GX_TF_RGBA8, GX_CLAMP, GX_CLAMP, GX_FALSE);
	GX_InitTexObj(&texObj, LoadingTexture, width, height, GX_TF_RGB5A3, GX_CLAMP, GX_CLAMP, GX_FALSE);
	GX_LoadTexObj(&texObj, GX_TEXMAP0);

	guMtxIdentity(GXmodelView2D);
	GX_LoadPosMtxImm(GXmodelView2D,GX_PNMTX2);
	guOrtho(GXprojection2D, 0, 480, 0, 640, 0, 1);
	GX_LoadProjectionMtx(GXprojection2D, GX_ORTHOGRAPHIC); //load current 2D projection matrix
//	GX_SetViewport((f32) 0,(f32) 0,(f32) 640,(f32) 480, 0.0f, 1.0f);
	GX_SetViewport((f32) OGL.GXorigX,(f32) OGL.GXorigY,(f32) OGL.GXwidth,(f32) OGL.GXheight, 0.0f, 1.0f);
	GX_SetScissor((u32) 0,(u32) 0,(u32) OGL.width,(u32) OGL.height);	//Set to the same size as the viewport.

	GX_ClearVtxDesc();
	GX_SetVtxDesc(GX_VA_PTNMTXIDX, GX_PNMTX2);
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
	//set vertex attribute formats here
	GX_SetVtxAttrFmt(GX_VTXFMT1, GX_VA_POS, GX_POS_XY, GX_F32, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT1, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);

	//disable textures
	GX_SetNumChans (0);
	GX_SetNumTexGens (1);
	GX_SetTevOrder (GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLORNULL);
	GX_SetTevOp (GX_TEVSTAGE0, GX_REPLACE);
	//set blend mode
	GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR); //Fix src alpha
	GX_SetColorUpdate(GX_ENABLE);
	GX_SetAlphaUpdate(GX_ENABLE);
	GX_SetDstAlpha(GX_DISABLE, 0xFF);
	GX_SetZMode(GX_DISABLE,GX_ALWAYS,GX_FALSE);
	GX_SetZTexture(GX_ZT_DISABLE,GX_TF_Z16,0);	//GX_ZT_DISABLE or GX_ZT_REPLACE; set in gDP.cpp
	GX_SetZCompLoc(GX_TRUE);	// Do Z-compare before texturing.
	//set cull mode
	GX_SetCullMode (GX_CULL_NONE);

	GX_Begin(GX_QUADS, GX_VTXFMT1, 4);
		GX_Position2f32(x, y);
		GX_TexCoord2f32(0,0);
		GX_Position2f32(x+width, y);
		GX_TexCoord2f32(1,0);
		GX_Position2f32(x+width, y+height);
		GX_TexCoord2f32(1,1);
		GX_Position2f32(x, y+height);
		GX_TexCoord2f32(0,1);
	GX_End();

#endif //MENU_V2
}

void VI_GX_updateDEBUG()
{
	VI.updateOSD = true;
}

extern char text[DEBUG_TEXT_HEIGHT][DEBUG_TEXT_WIDTH];

void VI_GX_showDEBUG()
{
	int i = 0;
	GXColor fontColor = {150, 255, 150, 255};
//	VI_GX_showStats();
	DEBUG_update();
#ifndef MENU_V2
	write_font_init_GX(fontColor);
	if(printToScreen)
		for (i=0;i<DEBUG_TEXT_HEIGHT;i++)
			write_font(10,(10*i+60),text[i], 0.5); 
#else
	menu::IplFont::getInstance().drawInit(fontColor);
	if(printToScreen)
		for (i=0;i<DEBUG_TEXT_HEIGHT;i++)
			menu::IplFont::getInstance().drawString(10,(10*i+60),text[i], 0.5, false); 
#endif

	//Reset any stats in DEBUG_stats
//	DEBUG_stats(8, "RecompCache Blocks Freed", STAT_TYPE_CLEAR, 1);

   //reset swap table from GUI/DEBUG
//	GX_SetTevSwapModeTable(GX_TEV_SWAP0, GX_CH_RED, GX_CH_GREEN, GX_CH_BLUE, GX_CH_ALPHA);
	GX_SetTevSwapMode(GX_TEVSTAGE0, GX_TEV_SWAP0, GX_TEV_SWAP0);
}

#ifdef SHOW_DEBUG
	extern int CntTriProj, CntTriProjW, CntTriOther, CntTriNear, CntTriPolyOffset;
#endif

void VI_GX_showStats()
{
#ifdef SHOW_DEBUG
	sprintf(txtbuffer,"texCache: %d bytes in %d cached textures; %d FB textures; %d max textures",cache.cachedBytes,cache.numCached,frameBuffer.numBuffers, GX_MAX_TEXTURES);
	DEBUG_print(txtbuffer,DBG_CACHEINFO); 

	sprintf(txtbuffer,"TriMatr: %d Proj; %d ProjW; %d Other; %d ProjWnear; %d PolyOff",CntTriProj,CntTriProjW,CntTriOther,CntTriNear,CntTriPolyOffset);
	DEBUG_print(txtbuffer,DBG_CACHEINFO+1); 
	CntTriProj = 0;
	CntTriProjW = 0;
	CntTriOther = 0;
	CntTriNear = 0;
	CntTriPolyOffset = 0;

#endif
}

void VI_GX_cleanUp()
{
	GX_SetNumTevStages(1);
	GX_SetTevOp(GX_TEVSTAGE0,GX_MODULATE);

	GX_SetFog(GX_FOG_NONE,0,1,0,1,(GXColor){0,0,0,255});
	GX_SetViewport((f32) OGL.GXorigX,(f32) OGL.GXorigY,(f32) OGL.GXwidth,(f32) OGL.GXheight, 0.0f, 1.0f);
	GX_SetCoPlanar(GX_DISABLE);
	GX_SetClipMode(GX_CLIP_ENABLE);
	GX_SetScissor((u32) 0,(u32) 0,(u32) OGL.width,(u32) OGL.height);
	GX_SetAlphaCompare(GX_ALWAYS,0,GX_AOP_AND,GX_ALWAYS,0);
	GX_SetZCompLoc(GX_TRUE);	// Do Z-compare before texturing.
}

extern heap_cntrl* GXtexCache;

void VI_GX_renderCpuFramebuffer()
{
	//Only render N64 framebuffer in RDRAM and not EFB drawn by glN64
	if (!(*REG.VI_STATUS & 3)) //bpp != 16 or 32
	{
#ifdef SHOW_DEBUG
		sprintf(txtbuffer,"VI (CpuFramebuffer): No bits per pixel specified");
		DEBUG_print(txtbuffer,DBG_VIINFO); 
#endif
		return; 
	}
	if (!*REG.VI_H_START)
	{
#ifdef SHOW_DEBUG
		sprintf(txtbuffer,"VI (CpuFramebuffer): VI_H_START_REG is NULL");
		DEBUG_print(txtbuffer,DBG_VIINFO); 
#endif
		return; 
	}
	VI_UpdateSize();
	u32 FBtexW = (VI.width + 3) & ~3;
	u32 FBtexH = (VI.height + 3) & ~3;

	//Init texture cache heap if not yet inited
	if(!GXtexCache)
	{
		GXtexCache = (heap_cntrl*)malloc(sizeof(heap_cntrl));
#ifdef HW_RVL
		__lwp_heap_init(GXtexCache, TEXCACHE_LO,GX_TEXTURE_CACHE_SIZE, 32);
#else //HW_RVL
		__lwp_heap_init(GXtexCache, memalign(32,GX_TEXTURE_CACHE_SIZE),GX_TEXTURE_CACHE_SIZE, 32);
#endif //!HW_RVL
	}
	u16* FBtex = (u16*) __lwp_heap_allocate(GXtexCache,FBtexW*FBtexH*2);
	while(!FBtex)
	{
		TextureCache_FreeNextTexture();
		FBtex = (u16*) __lwp_heap_allocate(GXtexCache,FBtexW*FBtexH*2);
	}
	GXTexObj	FBtexObj;

	//N64 Framebuffer is in RGB5A1 format, so shift by 1 and retile.
	GX_RedirectWriteGatherPipe(FBtex);

	u32 address = RSP_SegmentToPhysical( *REG.VI_ORIGIN );
	u32 stride = *REG.VI_WIDTH * 2;

	if (*REG.VI_V_CURRENT_LINE & 1)
		address -= stride;

	u8 *src1 = &RDRAM[address - 8];
	u8 *src2 = src1 + stride;
	u8 *src3 = src2 + stride;
	u8 *src4 = src3 + stride;

	int rowpitch = stride * 4 - (FBtexW * 2);
	int rows = FBtexH >> 2;

	while (rows--) {
		int tiles = FBtexW >> 2;

		do {
			__asm__ volatile(
				"lwzu    2, 8(%0) \n"
				"lwz     3, 4(%0) \n"
				"lwzu    4, 8(%1) \n"
				"lwz     5, 4(%1) \n"
				"lwzu    6, 8(%2) \n"
				"lwz     7, 4(%2) \n"
				"lwzu    8, 8(%3) \n"
				"lwz     9, 4(%3) \n"

				"rotrwi  2, 2, 1 \n"
				"rotrwi  3, 3, 1 \n"
				"rotrwi  4, 4, 1 \n"
				"rotrwi  5, 5, 1 \n"
				"rotrwi  6, 6, 1 \n"
				"rotrwi  7, 7, 1 \n"
				"rotrwi  8, 8, 1 \n"
				"rotrwi  9, 9, 1 \n"

				"or  2, 2, %4 \n"
				"or  3, 3, %4 \n"
				"or  4, 4, %4 \n"
				"or  5, 5, %4 \n"
				"or  6, 6, %4 \n"
				"or  7, 7, %4 \n"
				"or  8, 8, %4 \n"
				"or  9, 9, %4 \n"

				"stw     2, 0(%5) \n"
				"stw     3, 0(%5) \n"
				"stw     4, 0(%5) \n"
				"stw     5, 0(%5) \n"
				"stw     6, 0(%5) \n"
				"stw     7, 0(%5) \n"
				"stw     8, 0(%5) \n"
				"stw     9, 0(%5) \n"
				: "+b" (src1), "+b" (src2), "+b" (src3), "+b" (src4)
				: "r" (0x80008000), "b" (wgPipe)
				: "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9",
				  "memory");
		} while (--tiles);

		src1 += rowpitch;
		src2 += rowpitch;
		src3 += rowpitch;
		src4 += rowpitch;
	}

	GX_RestoreWriteGatherPipe();

	//Initialize texture
	GX_InvalidateTexAll();
	GX_InitTexObj(&FBtexObj, FBtex, FBtexW, FBtexH, GX_TF_RGB5A3, GX_CLAMP, GX_CLAMP, GX_FALSE); 
	GX_LoadTexObj(&FBtexObj, GX_TEXMAP0);

	GX_SetBlendMode(GX_BM_NONE, GX_BL_ONE, GX_BL_ZERO, GX_LO_CLEAR); 
	GX_SetAlphaCompare(GX_ALWAYS,0,GX_AOP_AND,GX_ALWAYS,0);
	GX_SetZMode(GX_DISABLE,GX_ALWAYS,GX_FALSE);
	GX_SetCullMode (GX_CULL_NONE);
	GX_SetFog(GX_FOG_NONE,0.1,1.0,0.0,1.0,(GXColor) {0,0,0,255});

	Mtx44 GXprojection;
	guMtxIdentity(GXprojection);
	guOrtho(GXprojection, 0, 480, 0, 640, 0.0f, 1.0f);
	GX_LoadProjectionMtx(GXprojection, GX_ORTHOGRAPHIC); 
	Mtx	GXmodelViewIdent;
	guMtxIdentity(GXmodelViewIdent);
	GX_LoadPosMtxImm(GXmodelViewIdent,GX_PNMTX0);
	GX_SetViewport((f32) OGL.GXorigX,(f32) OGL.GXorigY,(f32) OGL.GXwidth,(f32) OGL.GXheight, 0.0f, 1.0f);
	GX_SetScissor((u32) 0,(u32) 0,(u32) OGL.width,(u32) OGL.height);	//Set to the same size as the viewport.

	float u1, v1;

	u1 = (float)VI.width / (float)FBtexW;
	v1 = (float)VI.height / (float)FBtexH;

	//set vertex description
	GX_ClearVtxDesc();
	GX_SetVtxDesc(GX_VA_PTNMTXIDX, GX_PNMTX0);
	GX_SetVtxDesc(GX_VA_TEX0MTXIDX, GX_TEXMTX0);
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
	//set vertex attribute formats
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_F32, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);

	//enable textures
	GX_SetNumChans (0);
	GX_SetNumTexGens (1);
	GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
	GX_SetNumTevStages (1);
	GX_SetTevOrder (GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
	GX_SetTevOp (GX_TEVSTAGE0, GX_REPLACE);

	GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
		GX_Position2f32( 0.0f, 0.0f );
		GX_TexCoord2f32( 0.0f, 0.0f );
		GX_Position2f32( 640.0f, 0.0f );
		GX_TexCoord2f32( u1, 0.0f );
		GX_Position2f32( 640.0f, 480.0f );
		GX_TexCoord2f32( u1, v1 );
		GX_Position2f32( 0.0f, 480.0f );
		GX_TexCoord2f32( 0.0f, v1 );
	GX_End();

	__lwp_heap_free(GXtexCache, FBtex);
}

void VI_GX_PreRetraceCallback(u32 retraceCnt)
{
	if(VI.copy_fb)
	{
		VIDEO_SetNextFramebuffer(VI.xfb[VI.which_fb]);
		VIDEO_Flush();
		VI.which_fb ^= 1;
		VI.copy_fb = false;
	}
}
#endif // __GX__
