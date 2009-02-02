#ifdef __GX__
#include <stdio.h>
#include <gccore.h>
#include "../gui/font.h"
#include "../gui/DEBUG.h"
#include "../main/timers.h"
//#include "Textures.h"
#endif // __GX__

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

extern GXRModeObj *vmode;

#ifdef __GX__
char printToScreen;
char showFPSonScreen;

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

	// These are in half-lines, so shift an extra bit
	u32 vEnd = _SHIFTR( *REG.VI_V_START, 1, 9 );
	u32 vStart = _SHIFTR( *REG.VI_V_START, 17, 9 );

	VI.width = (unsigned long)((hEnd - hStart) * xScale);
	VI.height = (unsigned long)((vEnd - vStart) * yScale * 1.0126582f);

	if (VI.width == 0.0f) VI.width = (unsigned long)320.0f;
	if (VI.height == 0.0f) VI.height = (unsigned long)240.0f;
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

	if (OGL.frameBufferTextures)
	{
		FrameBuffer *current = FrameBuffer_FindBuffer( *REG.VI_ORIGIN );

		if ((*REG.VI_ORIGIN != VI.lastOrigin) || ((current) && current->changed))
		{
			if (gDP.colorImage.changed)
			{
//				if(VI.copy_fb)
//					VIDEO_WaitVSync();
				FrameBuffer_SaveBuffer( gDP.colorImage.address, gDP.colorImage.size, gDP.colorImage.width, gDP.colorImage.height );
				gDP.colorImage.changed = FALSE;
			}

			VI.doFrameBufferRender = true;

			gDP.colorImage.changed = FALSE;
			VI.lastOrigin = *REG.VI_ORIGIN;
		}
	}
	else
		if (gSP.changed & CHANGED_COLORBUFFER)
			OGL_SwapBuffers();

	VI_GX_cleanUp();
	VI_GX_showStats();
	VI_GX_showFPS();
	VI_GX_showDEBUG();
	if(VI.updateOSD)
	{
//		DEBUG_stats(8, "RecompCache Blocks Freed", STAT_TYPE_CLEAR, 1);
//		if(VI.copy_fb)
//			VIDEO_WaitVSync();
		GX_SetCopyClear ((GXColor){0,0,0,255}, 0xFFFFFF);
//		GX_CopyDisp (VI.xfb[VI.which_fb], GX_TRUE);	//clear the EFB before executing new Dlist
		GX_CopyDisp (VI.xfb[VI.which_fb], GX_FALSE);
		GX_DrawDone(); //Wait until EFB->XFB copy is complete
//		doCaptureScreen();
		VI.updateOSD = false;
		VI.copy_fb = true;
	}
	if(VI.doFrameBufferRender)
	{
		FrameBuffer_RenderBuffer( *REG.VI_ORIGIN );
		VI.doFrameBufferRender = false;
	}
#endif // __GX__

}

#ifdef __GX__
extern "C" {
extern long long gettime();
extern unsigned int diff_sec(long long start,long long end);
};

void VI_GX_init() {
	init_font();
/*	updateDEBUGflag = true;
	new_fb = false;
	which_fb = 1;*/
	VI.updateOSD = true;
	VI.copy_fb = false;
	VI.which_fb = 1;
	VI.doFrameBufferRender = false;
}

void VI_GX_setFB(unsigned int* fb1, unsigned int* fb2){
	VI.xfb[0] = fb1;
	VI.xfb[1] = fb2;
}

unsigned int* VI_GX_getScreenPointer(){ return VI.xfb[VI.which_fb]; }

void VI_GX_clearEFB(){
	GX_SetCopyClear ((GXColor){0,0,0,255}, 0xFFFFFF);
	GX_CopyDisp (VI.xfb[VI.which_fb], GX_TRUE);	//clear the EFB before executing new Dlist
	GX_DrawDone(); //Wait until EFB->XFB copy is complete
}

extern timers Timers;

void VI_GX_showFPS(){
	static long long lastTick=0;
	static int frames=0;
	static int VIs=0;
	static char caption[25];
	
/*	long long nowTick = gettime();
	VIs++;
	if (VI.updateOSD)
		frames++;
	if (diff_sec(lastTick,nowTick)>=1) {
		sprintf(caption, "%02d VI/s, %02d FPS",VIs,frames);
		frames = 0;
		VIs = 0;
		lastTick = nowTick;
	}*/

	sprintf(caption, "%.1f VI/s, %.1f FPS",Timers.vis,Timers.fps);
	
	if (VI.updateOSD)
	{
		GXColor fontColor = {150,255,150,255};
		write_font_init_GX(fontColor);
		if(showFPSonScreen)
			write_font(10,35,caption, 1.0);
		//write_font(10,10,caption,xfb,which_fb);

		//reset swap table from GUI/DEBUG
		GX_SetTevSwapModeTable(GX_TEV_SWAP0, GX_CH_RED, GX_CH_GREEN, GX_CH_BLUE, GX_CH_ALPHA);
		GX_SetTevSwapMode(GX_TEVSTAGE0, GX_TEV_SWAP0, GX_TEV_SWAP0);
	}
}

void VI_GX_showLoadProg(float percent)
{
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
	GX_CopyDisp (VI.xfb[VI.which_fb], GX_FALSE);
    GX_DrawDone();
	VI.copy_fb = true;
//	VIDEO_SetNextFramebuffer(VI.xfb[VI.which_fb]);
//	VIDEO_Flush();
//	VI.which_fb ^= 1;
//	VIDEO_WaitVSync();
}

void VI_GX_updateDEBUG()
{
	VI.updateOSD = true;
}

extern char text[DEBUG_TEXT_HEIGHT][DEBUG_TEXT_WIDTH];

void VI_GX_showDEBUG()
{
	if (VI.updateOSD)
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

void VI_GX_showStats()
{
	if (VI.updateOSD)
	{
		sprintf(txtbuffer,"texCache: %d bytes in %d cached textures; %d FB textures",cache.cachedBytes,cache.numCached,frameBuffer.numBuffers);
		DEBUG_print(txtbuffer,DBG_CACHEINFO); 
	}
}

void VI_GX_cleanUp()
{
	GX_SetFog(GX_FOG_NONE,0,1,0,1,(GXColor){0,0,0,255});
	GX_SetViewport(0,0,vmode->fbWidth,vmode->efbHeight,0,1);
	GX_SetCoPlanar(GX_DISABLE);
	GX_SetClipMode(GX_CLIP_DISABLE);
	GX_SetScissor(0,0,vmode->fbWidth,vmode->efbHeight);
	GX_SetAlphaCompare(GX_ALWAYS,0,GX_AOP_AND,GX_ALWAYS,0);
	GX_SetZCompLoc(GX_TRUE);	// Do Z-compare before texturing.
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
