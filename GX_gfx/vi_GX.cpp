/* vi_GX.cpp - vi for Gamecube, based off vi_GX
   by Mike Slegeir for Mupen64-GC
 */

#include <stdio.h>
#include <stdlib.h>
//#include <gccore.h>
#include <ogc/system.h>
#include <ogc/video.h>
#include <ogc/gx.h>
#include <ogc/gu.h>
#include <sdcard.h>
//#include <png/pngogc.h>
#include "vi_GX.h"
#include "../gui/font.h"
#include "../gui/DEBUG.h"

//Global variables set by menu
char printToScreen;
char showFPS;

VI_GX::VI_GX(GFX_INFO info) : VI(info), which_fb(1), width(0), height(0){
	init_font();
	updateDEBUGflag = true;
	captureScreenFlag = false;
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

unsigned int* VI_GX::getScreenPointer(){ return xfb[which_fb]; }

void VI_GX::blit(){
	//printf("Should be blitting.");
	showFPS();
	showDEBUG();
    GX_DrawDone(); //needed?
	GX_CopyDisp (xfb[which_fb], GX_FALSE); //TODO: Figure out where the UpdateScreen interrupts are coming from!
//    GX_Flush (); //needed?
    GX_DrawDone(); //Shagkur's recommendation
	doCaptureScreen();
	updateDEBUGflag = false;
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
	static int VIs=0;
	static char caption[20];
	
	long long nowTick = gettime();
	VIs++;
	if (updateDEBUGflag)
		frames++;
	if (diff_sec(lastTick,nowTick)>=1) {
		sprintf(caption, "%02d VI/s, %02d FPS",frames,VIs);
//		sprintf(caption, "%02d FPS",frames);
		frames = 0;
		VIs = 0;
		lastTick = nowTick;
	}
	
	if (updateDEBUGflag)
	{
		GXColor fontColor = {150,255,150,255};
		write_font_init_GX(fontColor);
		if(showFPS)
			write_font(10,35,caption, 1.0);
		//write_font(10,10,caption,xfb,which_fb);

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
	updateDEBUGflag = true;
}

extern char text[DEBUG_TEXT_HEIGHT][DEBUG_TEXT_WIDTH];

void VI_GX::showDEBUG()
{
	if (updateDEBUGflag)
	{
		int i = 0;
		GXColor fontColor = {150, 255, 150, 255};
		DEBUG_update();
		write_font_init_GX(fontColor);
		if(printToScreen)
			for (i=0;i<DEBUG_TEXT_HEIGHT;i++)
				write_font(10,(10*i+60),text[i], 0.5); 
//				write_font(10,(6*i+60),text[i], 0.5); 
		
	   //reset swap table from GUI/DEBUG
		GX_SetTevSwapModeTable(GX_TEV_SWAP0, GX_CH_RED, GX_CH_GREEN, GX_CH_BLUE, GX_CH_ALPHA);
		GX_SetTevSwapMode(GX_TEVSTAGE0, GX_TEV_SWAP0, GX_TEV_SWAP0);
	}
}

void VI_GX::setCaptureScreen()
{
	captureScreenFlag = true;
}

extern GXRModeObj *vmode;		/*** Graphics Mode Object declared as global in main_gc-menu.c ***/

void VI_GX::doCaptureScreen()
{
	if (updateDEBUGflag && captureScreenFlag)
	{
#if 0	//This code is for future Screen Capture functionality using libPNG

		//The following code is adapted from SoftDev's libpngogc example
		//requires -lpngogc -lpng in Makefile
		//I estimate the required free memory to be about 1MB for this to work
		/*** User defined types from pngogc ***/
		PNG_MEMFILE *mf;
		char *pic;		/*** Picture buffer ***/

		SDCARD_Init();	/*** SDCard may not have been initialized yet ***/

		pic = (char *)malloc(vmode->fbWidth * vmode->xfbHeight * 3); /*** Way too big, better to be safe though ***/
		mf = pngmem_fopen(pic, vmode->fbWidth * vmode->xfbHeight * 3);

		YUY2toRGB24(vmode, (u8 *) pic, (u8 *)((u32)xfb[which_fb] & 0x8fffffff) );	/* Take snapshot of current fb */

		/* Write the compressed image to the memory file */
		if(PNG_Write( pic, vmode->fbWidth, vmode->xfbHeight, mf ) == PNGOGC_SUCCESS)
		{
			/*** Write to SD ***/
			sd_file *handle;
			handle = SDCARD_OpenFile("dev0:\\N64SAVES\\gcscreen.png", "w");
			if(!handle){
				printf("file not found or wrong path\n");
				DEBUG_print("Unable to open screen capture file on SD card slot A...",DBG_VIINFO);
			}
			else{
				SDCARD_WriteFile(handle, pic, mf->currpos);
				SDCARD_CloseFile(handle);
				DEBUG_print("Screen capture saved to SD card in slot A...",DBG_VIINFO);
			}
		}

		/*** Close memory file and free picture buffer ***/
		pngmem_fclose(mf);
		free(pic);

#endif
		captureScreenFlag = false;
	}
}
