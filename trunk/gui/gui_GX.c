/**
 * Wii64 - gui_GX.c
 * Copyright (C) 2007, 2008 sepp256
 *
 * gui that uses GX graphics
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
#include <sdcard.h>
#include <gccore.h>
#include "gui_GX.h"
#include "font.h"

#define X1 30
#define X2 13
#define Y1 30
#define Y2  5
#define Z1 30
#define Z2 13

// N64 logo vertex data
s8 logo[] ATTRIBUTE_ALIGN (32) =
{ // x y z
  -X1, -Y1,  Z1,		// 0 (side A, XY plane, Z=Z1)
  -X2, -Y1,  Z1,		// 1		6 7   8 9
   X2, -Y1,  Z1,		// 2		  5    
   X1, -Y1,  Z1,		// 3		      4  
   X2, -Y2,  Z1,		// 4		0 1   2 3
  -X2,  Y2,  Z1,		// 5
  -X1,  Y1,  Z1,		// 6
  -X2,  Y1,  Z1,		// 7
   X2,  Y1,  Z1,		// 8
   X1,  Y1,  Z1,		// 9
   Z1, -Y1,  X1,		// 10 (side B, -ZY plane, X=Z1)
   Z1, -Y1,  X2,		// 11		6 7   8 9
   Z1, -Y1, -X2,		// 12		  5    
   Z1, -Y1, -X1,		// 13		      4  
   Z1, -Y2, -X2,		// 14		0 1   2 3
   Z1,  Y2,  X2,		// 15
   Z1,  Y1,  X1,		// 16
   Z1,  Y1,  X2,		// 17
   Z1,  Y1, -X2,		// 18
   Z1,  Y1, -X1,		// 19
   X1, -Y1, -Z1,		// 20 (side C, -XY plane, Z=-Z1)
   X2, -Y1, -Z1,		// 21		6 7   8 9
  -X2, -Y1, -Z1,		// 22		  5    
  -X1, -Y1, -Z1,		// 23		      4  
  -X2, -Y2, -Z1,		// 24		0 1   2 3
   X2,  Y2, -Z1,		// 25
   X1,  Y1, -Z1,		// 26
   X2,  Y1, -Z1,		// 27
  -X2,  Y1, -Z1,		// 28
  -X1,  Y1, -Z1,		// 29
  -Z1, -Y1, -X1,		// 30 (side D, ZY plane, X=-Z1)
  -Z1, -Y1, -X2,		// 31		6 7   8 9
  -Z1, -Y1,  X2,		// 32		  5    
  -Z1, -Y1,  X1,		// 33		      4  
  -Z1, -Y2,  X2,		// 34		0 1   2 3
  -Z1,  Y2, -X2,		// 35
  -Z1,  Y1, -X1,		// 36
  -Z1,  Y1, -X2,		// 37
  -Z1,  Y1,  X2,		// 38
  -Z1,  Y1,  X1,		// 39
  -X2,  Y1,  Z2,		// 40,7A (top, XZ plane, Y=Y1)
   X2,  Y1,  Z2,		// 41,7B		
   X2,  Y1, -Z2,		// 42,7C		  7D 7C
  -X2,  Y1, -Z2,		// 43,7D		  7A 7B  
  -X2,  Y2,  Z2,		// 44,5A (upper-middle, XZ plane, Y=Y2)
   X2,  Y2,  Z2,		// 45,5B		
   X2,  Y2, -Z2,		// 46,5C		  5D 5C
  -X2,  Y2, -Z2,		// 47,5D		  5A 5B  
  -X2, -Y2,  Z2,		// 48,4D (lower-middle, XZ plane, Y=-Y2)
   X2, -Y2,  Z2,		// 49,4A		
   X2, -Y2, -Z2,		// 50,4B		  4C 4B
  -X2, -Y2, -Z2,		// 51,4C		  4D 4A  
  -X2, -Y1,  Z2,		// 52,1A (bottom, XZ plane, Y=-Y1)
   X2, -Y1,  Z2,		// 53,1B		
   X2, -Y1, -Z2,		// 54,1C		  1D 1C
  -X2, -Y1, -Z2,		// 55,1D		  1A 1B  
};
 
// N64 logo color data
u8 colors[] ATTRIBUTE_ALIGN (32) =
{ // r, g, b, a
	  8, 147,  48, 255,		// 0 green
	  1,  29, 169, 255,		// 1 blue
	254,  32,  21, 255,		// 2 orange/red
	255, 192,   1, 255,		// 3 yellow/gold
};

static GUIinfo GUI;
static int GUI_on = 0;
static int GXtoggleFlag = 1;
static lwp_t GUIthread;
lwp_t GXthread;
GXTexObj BGtex;
GXTlutObj BGtlut;
GXTexObj LOGOtex;

extern  u8 BGtexture[];
extern  u16 BGtextureCI[];
extern  u16 LOGOtexture[];
extern long BGtexture_length;
extern long BGtextureCI_length;
extern long LOGOtexture_length;

void draw_quad (u8, u8, u8, u8, u8);
void GUI_splashScreen();
void draw_rect(GXColor, float, float, float, float, float);
void draw_background();

void GUI_setFB(unsigned int* fb1, unsigned int* fb2){
	GUI.xfb[0] = fb1;
	GUI.xfb[1] = fb2;
	GUI.which_fb = 1;
}

void GUI_init(){
	s32 stat;

	init_font();
//	SDCARD_Init(); //for future functionality to load alternate background from SD card
	GUI_clear();

	//load BG texture from SD card and initialize
	GUI_loadBGtex();

	GUI_splashScreen();
	//TODO: init spinning cube display list

	GUI_on = 1;

	stat = LWP_CreateThread(&GUIthread, &GUI_main, NULL, NULL, 0, 80);
	if (stat<0)
		GUI_print("Error creating GUIthread.\n");

}

void GUI_toggle()
{
	s32 stat;

	if (GUI_on == 1) {
		stat = LWP_SuspendThread(GUIthread);
//		GXthread = GX_SetCurrentGXThread();
		GX_CopyDisp (GUI.xfb[GUI.which_fb], GX_TRUE); // This clears the efb
		GX_CopyDisp (GUI.xfb[GUI.which_fb], GX_TRUE); // This clears the xfb
		GX_Flush ();
		VIDEO_SetNextFramebuffer(GUI.xfb[GUI.which_fb]);
		VIDEO_Flush();
		GUI.which_fb ^= 1;
	}
	else {
		GUI_clear();
		GX_DrawDone();
		GUI_loadBGtex();
//		GX_CopyDisp (GUI.xfb[GUI.which_fb], GX_TRUE);

		GXtoggleFlag = 1;
		stat = LWP_ResumeThread(GUIthread);
	}
	if (stat<0)
		GUI_print("Error toggling GUIthread.");

	GUI_on ^= 1;
}

void GUI_main()
{
	Mtx44 GXprojection2D;
	Mtx GXmodelView2D;
//	int i;

	while(1) {

	if(GXtoggleFlag == 1) {
//		GXthread = GX_SetCurrentGXThread();
		GXtoggleFlag = 0;
	}

	guMtxIdentity(GXmodelView2D);
	GX_LoadTexMtxImm(GXmodelView2D,GX_TEXMTX0,GX_MTX2x4);
	guMtxTransApply (GXmodelView2D, GXmodelView2D, 0.0F, 0.0F, -5.0F);
	GX_LoadPosMtxImm(GXmodelView2D,GX_PNMTX0);
	guOrtho(GXprojection2D, 0, 479, 0, 639, 0, 100);
	GX_LoadProjectionMtx(GXprojection2D, GX_ORTHOGRAPHIC);

	GX_SetZMode(GX_ENABLE,GX_ALWAYS,GX_TRUE);

	GX_ClearVtxDesc();
	GX_SetVtxDesc(GX_VA_PTNMTXIDX, GX_PNMTX0);
	GX_SetVtxDesc(GX_VA_TEX0MTXIDX, GX_TEXMTX0);
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
	//set vertex attribute formats here
	GX_SetVtxAttrFmt(GX_VTXFMT1, GX_VA_POS, GX_POS_XY, GX_F32, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT1, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);

	//enable textures
	GX_SetNumChans (1);
	GX_SetNumTexGens (1);
	GX_SetTevOrder (GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0); // change to (u8) tile later
	GX_SetTevOp (GX_TEVSTAGE0, GX_REPLACE);
	//set blend mode
	GX_SetBlendMode(GX_BM_NONE, GX_BL_ONE, GX_BL_ZERO, GX_LO_CLEAR); //Fix src alpha
	GX_SetColorUpdate(GX_ENABLE);
	GX_SetAlphaUpdate(GX_ENABLE);
	GX_SetDstAlpha(GX_DISABLE, 0xFF);
	//set cull mode
	GX_SetCullMode (GX_CULL_NONE);

	GX_Begin(GX_QUADS, GX_VTXFMT1, 4);
	GX_Position2f32(0, 0);
	GX_TexCoord2f32(0,0);
	GX_Position2f32(639, 0);
	GX_TexCoord2f32(1,0);
	GX_Position2f32(639, 479);
	GX_TexCoord2f32(1,1);
	GX_Position2f32(0, 479);
	GX_TexCoord2f32(0,1);
	GX_End();

	GUI_displayText();

	GUI_drawWiiN64(319.5,66.5,-20.0,1);
	GUI_drawLogo(580.0, 70.0, -50.0);

	GX_DrawDone ();
	GX_CopyDisp (GUI.xfb[GUI.which_fb], GX_FALSE);
    GX_Flush ();
	VIDEO_SetNextFramebuffer(GUI.xfb[GUI.which_fb]);
	VIDEO_Flush();
	GUI.which_fb ^= 1;
	VIDEO_WaitVSync();
	}
}

void GUI_displayText(){
	int i = 1;
	char** temp_textptrs;
	GXColor fontColor = {255, 255, 255, 255};
	GXColor* fontColorPtr;

	temp_textptrs = GUI_get_text();
	fontColorPtr = GUI_get_colors();
	write_font_init_GX(fontColor);
	for (i=0;i<GUI_TEXT_HEIGHT;i++)
	{
		write_font_color(&fontColorPtr[i]);
//		GX_SetTevColor(GX_TEVREG1,fontColorPtr[i]);
		write_font(60,(20*i+105),temp_textptrs[i], 1.0); 
	}

   //reset swap table from GUI/DEBUG
	GX_SetTevSwapModeTable(GX_TEV_SWAP0, GX_CH_RED, GX_CH_GREEN, GX_CH_BLUE, GX_CH_ALPHA);
	GX_SetTevSwapMode(GX_TEVSTAGE0, GX_TEV_SWAP0, GX_TEV_SWAP0);
}

int GUI_loadBGtex(){

	//This code loads a texture & TLUT from SD card - to be added as a feature in the future
/*	char* backgroundfile = "dev0:\\N64ROMS\\bgD.tx";
	char* tlutfile = "dev0:\\N64ROMS\\bgD.tlt";
	sd_file *bg_file;

	printf("%s\n",backgroundfile);
	bg_file = SDCARD_OpenFile(backgroundfile, "rb");
	if(bg_file) printf("file found\n");
	else { printf("ERROR COULD NOT OPEN: %s\n", backgroundfile); return -1; }

	SDSTAT s;
	SDCARD_GetStats(bg_file, &s);
	SDCARD_SeekFile(bg_file, 0, SDCARD_SEEK_SET);
	int bg_length = s.size;
	printf ("background size: %d bytes (or %d Mb or %d Megabits)\n", 
		bg_length, bg_length/1024/1024, bg_length/1024/1024*8);
	printf("Loading Background: %s, please be patient...\n", backgroundfile);
//	int n = SDCARD_ReadFile(bg_file, BGtexture, 640*480*2);
	int n = SDCARD_ReadFile(bg_file, BGtexture, 640*480);
	SDCARD_CloseFile(bg_file);
	printf("Loaded %d bytes.\n", n);
	printf("First pixel value is: %x\n", BGtexture[0]);

	bg_file = SDCARD_OpenFile(tlutfile, "rb");
	if(bg_file) printf("file found\n");
	else { printf("ERROR COULD NOT OPEN: %s\n", tlutfile); return -1; }
	SDCARD_GetStats(bg_file, &s);
	SDCARD_SeekFile(bg_file, 0, SDCARD_SEEK_SET);
	bg_length = s.size;
	printf ("background size: %d bytes (or %d Mb or %d Megabits)\n", 
		bg_length, bg_length/1024/1024, bg_length/1024/1024*8);
	printf("Loading Background: %s, please be patient...\n", tlutfile);
//	n = SDCARD_ReadFile(bg_file, BGtextureCI, 256*2);
	n = SDCARD_ReadFile(bg_file, BGtextureCI, 256*4);
	SDCARD_CloseFile(bg_file);
	printf("Loaded %d bytes.\n", n);
	printf("First 2 CI values are: %x %x\n", BGtextureCI[0], BGtextureCI[1]);
*/

	//GX_TL_RGB5A3 == 0x02? 
	//GX_TL_RGB565 == 0x01? 0x05?
	GX_InitTlutObj(&BGtlut, BGtextureCI,(u8) 0x01,(u16) 256/16); //GX_TL_RGB565 = 0x01 is missing in gx.h
//	GX_InitTlutObj(&BGtlut, BGtextureCI,(u8) GX_TL_RGB565,(u16) 256/16); //GX_TL_RGB565 = 0x01 is missing in gx.h
	DCFlushRange(BGtextureCI, 256*2);
	GX_InvalidateTexAll();
	GX_LoadTlut(&BGtlut, GX_TLUT0);	

//	GX_InitTexObj(&BGtex, BGtexture, (u16) 640, (u16) 480, GX_TF_RGB565, GX_CLAMP, GX_CLAMP, GX_FALSE); 
//	DCFlushRange(BGtexture, 640*480*2);
	GX_InitTexObjCI(&BGtex, BGtexture, (u16) 640, (u16) 480, GX_TF_CI8, GX_CLAMP, GX_CLAMP, GX_FALSE, GX_TLUT0); 
	DCFlushRange(BGtexture, 640*480);
	GX_LoadTexObj(&BGtex, GX_TEXMAP0); 

	GX_InitTexObj(&LOGOtex, LOGOtexture, (u16) 216, (u16) 84, GX_TF_RGB5A3, GX_CLAMP, GX_CLAMP, GX_FALSE); 
	DCFlushRange(LOGOtexture, 216*84*2);
	GX_LoadTexObj(&LOGOtex, GX_TEXMAP2); 

//	printf("Loaded Texture into TMEM.\n");

	return 1;
}

void GUI_drawWiiN64(float x0, float y0, float z, float scale){

	Mtx44 GXprojection2D;
	Mtx GXmodelView2D;
	float ulx, uly, lrx, lry;

	ulx = x0 - scale*108;
	lrx = x0 + scale*108;
	uly = y0 - scale*42;
	lry = y0 + scale*42;

	guMtxIdentity(GXmodelView2D);
	GX_LoadTexMtxImm(GXmodelView2D,GX_TEXMTX0,GX_MTX2x4);
	guMtxTransApply (GXmodelView2D, GXmodelView2D, 0.0F, 0.0F, z);
	GX_LoadPosMtxImm(GXmodelView2D,GX_PNMTX0);
	guOrtho(GXprojection2D, 0, 479, 0, 639, 0, 100);
	GX_LoadProjectionMtx(GXprojection2D, GX_ORTHOGRAPHIC);

	GX_SetZMode(GX_ENABLE,GX_GEQUAL,GX_TRUE);

	GX_ClearVtxDesc();
	GX_SetVtxDesc(GX_VA_PTNMTXIDX, GX_PNMTX0);
	GX_SetVtxDesc(GX_VA_TEX0MTXIDX, GX_TEXMTX0);
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
	//set vertex attribute formats here
	GX_SetVtxAttrFmt(GX_VTXFMT1, GX_VA_POS, GX_POS_XY, GX_F32, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT1, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);

	//enable textures
	GX_SetNumChans (1);
	GX_SetNumTexGens (1);
	GX_SetTevOrder (GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP2, GX_COLOR0A0);
	GX_SetTevOp (GX_TEVSTAGE0, GX_REPLACE);
	//set blend mode
	GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR); //Fix src alpha
	GX_SetColorUpdate(GX_ENABLE);
	GX_SetAlphaUpdate(GX_DISABLE);
	GX_SetDstAlpha(GX_DISABLE, 0xFF);
	//set cull mode
	GX_SetCullMode (GX_CULL_NONE);

	GX_Begin(GX_QUADS, GX_VTXFMT1, 4);
	GX_Position2f32(ulx, uly);
	GX_TexCoord2f32(0,0);
	GX_Position2f32(lrx, uly);
	GX_TexCoord2f32(1,0);
	GX_Position2f32(lrx, lry);
	GX_TexCoord2f32(1,1);
	GX_Position2f32(ulx, lry);
	GX_TexCoord2f32(0,1);
	GX_End();

}

void GUI_drawLogo(float x0, float y0, float z0){

  Mtx v, m, mv, tmp;            // view, model, modelview, and perspective matrices
//  Mtx p;
  Vector cam = { 0.0F, 0.0F, 0.0F }, 
		 up = {0.0F, 1.0F, 0.0F}, 
		 look = {0.0F, 0.0F, -1.0F},
		 axisX = {1.0F, 0.0F, 0.0F},
		 axisY = {0.0F, 1.0F, 0.0F};
  static float rotateby = 45,
			   rotatebyX = 0,
			   rotatebyY = 0;
  s8 stickX,stickY;
//  int i, j;


  guLookAt (v, &cam, &up, &look);
  rotateby++;

  stickX = PAD_SubStickX(0);
  stickY = PAD_SubStickY(0);
  if(stickX > 18 || stickX < -18) rotatebyX += stickX/32;
  if(stickY > 18 || stickY < -18) rotatebyY += stickY/32;

  // move the logo out in front of us and rotate it
  guMtxIdentity (m);
  guMtxRotAxisDeg (tmp, &axisX, 25);			//change to isometric view
  guMtxConcat (m, tmp, m);
  guMtxRotAxisDeg (tmp, &axisX, -rotatebyY);
  guMtxConcat (m, tmp, m);
  guMtxRotAxisDeg (tmp, &axisY, -rotatebyX);
  guMtxConcat (m, tmp, m);
  guMtxRotAxisDeg (tmp, &axisY, rotateby);		//slowly rotate logo
  guMtxConcat (m, tmp, m);
  guMtxTransApply (m, m, x0, y0, z0);
  guMtxConcat (v, m, mv);
  // load the modelview matrix into matrix memory
  GX_LoadPosMtxImm (mv, GX_PNMTX0);

  GX_SetCullMode (GX_CULL_BACK); // show only the outside facing quads
  GX_SetZMode(GX_ENABLE,GX_GEQUAL,GX_TRUE);

  // setup the vertex descriptor
  GX_ClearVtxDesc ();
  GX_SetVtxDesc(GX_VA_PTNMTXIDX, GX_PNMTX0);
  GX_SetVtxDesc (GX_VA_POS, GX_INDEX8);
  GX_SetVtxDesc (GX_VA_CLR0, GX_INDEX8);
 
  // setup the vertex attribute table
  GX_SetVtxAttrFmt (GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_S8, 0);
  GX_SetVtxAttrFmt (GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
 
  // set the array stride
  GX_SetArray (GX_VA_POS, logo, 3 * sizeof (s8));
  GX_SetArray (GX_VA_CLR0, colors, 4 * sizeof (u8));
 
  GX_SetNumChans (1);
  GX_SetNumTexGens (0);
  GX_SetTevOrder (GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
  GX_SetTevOp (GX_TEVSTAGE0, GX_PASSCLR);

  GX_Begin (GX_QUADS, GX_VTXFMT0, 160);  //40 quads, so 160 verts

  draw_quad ( 0,  6,  7,  1, 0); //Side A, green
  draw_quad ( 7,  4,  2,  5, 0);
  draw_quad ( 2,  8,  9,  3, 0);
  draw_quad (31, 35, 47, 55, 0); //Side A1
  draw_quad (55, 51, 42, 46, 0);
  draw_quad (50, 42, 18, 14, 0);
  draw_quad (10, 16, 17, 11, 1); //Side B, blue
  draw_quad (17, 14, 12, 15, 1);
  draw_quad (12, 18, 19, 13, 1);
  draw_quad ( 1,  5, 44, 52, 1); //Side B1
  draw_quad (52, 48, 43, 47, 1);
  draw_quad (51, 43, 28, 24, 1);
  draw_quad (20, 26, 27, 21, 0); //Side C, green
  draw_quad (27, 24, 22, 25, 0);
  draw_quad (22, 28, 29, 23, 0);
  draw_quad (11, 15, 45, 53, 0); //Side C1
  draw_quad (53, 49, 40, 44, 0);
  draw_quad (48, 40, 38, 34, 0);
  draw_quad (30, 36, 37, 31, 1); //Side D, blue
  draw_quad (37, 34, 32, 35, 1);
  draw_quad (32, 38, 39, 33, 1);
  draw_quad (21, 25, 46, 54, 1); //Side D1
  draw_quad (54, 50, 41, 45, 1);
  draw_quad (49, 41,  8,  4, 1);
  draw_quad ( 6, 38, 40,  7, 3); //Top, yellow
  draw_quad ( 8, 41, 17,  9, 3);
  draw_quad (42, 27, 19, 18, 3);
  draw_quad (37, 29, 28, 43, 3); 
  draw_quad ( 7, 40, 49,  4, 2); //Top, red(green?)
  draw_quad (17, 41, 50, 14, 0);
  draw_quad (27, 42, 51, 24, 2);
  draw_quad (37, 43, 48, 34, 0); 
  draw_quad ( 0,  1, 52, 32, 3); //Bottom, yellow
  draw_quad ( 3, 11, 53,  2, 3);
  draw_quad (13, 21, 54, 12, 3);
  draw_quad (23, 31, 55, 22, 3); 
  draw_quad ( 2, 53, 44,  5, 2); //Bottom, red(green?)
  draw_quad (12, 54, 45, 15, 0);
  draw_quad (22, 55, 46, 25, 2);
  draw_quad (32, 52, 47, 35, 0); 

  GX_End ();
}

// draws a quad from 4 vertex idx and one color idx
void draw_quad (u8 v0, u8 v1, u8 v2, u8 v3, u8 c)
{
  // one 8bit position idx
  GX_Position1x8 (v0);
  // one 8bit color idx
  GX_Color1x8 (c);
  GX_Position1x8 (v1);
  GX_Color1x8 (c);
  GX_Position1x8 (v2);
  GX_Color1x8 (c);
  GX_Position1x8 (v3);
  GX_Color1x8 (c);
}

#define SPLASH_FADE 100
#define SPLASH_CUBE_DROP 50
#define SPLASH_CUBE_SPIN 200
#define SPLASH_CUBE_TRANSLATE 50

void GUI_splashScreen()
{
	int i = 0, splash_step = 1;
	float x0, y0, xstart = 0, ystart = 0, scale;
	GXColor rectColor = {0,0,0,255};
	bool button_down = false;

	while(1) {
	switch(splash_step) 
	{
		case 1: //Fade in WiiN64 logo
			draw_rect((GXColor){0,0,0,255},0,0,639,479,-1);
			GUI_drawWiiN64(319.5,300,-20.0,2);
			rectColor.a = (u8) ((float)(SPLASH_FADE - i)/SPLASH_FADE * 255);
			draw_rect(rectColor,0,0,639,479,-25);
			if (i>=SPLASH_FADE) {
				i = 0;
				splash_step++;
			}
			i++;
			break;
		case 2: //Drop large N64 cube from top
			draw_rect((GXColor){0,0,0,255},0,0,639,479,-1);
			GUI_drawWiiN64(319.5,300,-20.0,2);
			y0 = -100 + 250*i/SPLASH_CUBE_DROP;
			GUI_drawLogo(320, y0, -50.0);
			if (i>=SPLASH_CUBE_DROP) {
				i = 0;
				splash_step++;
			}
			i++;
			break;
		case 3: //Bounce/spin N64 cube for a couple seconds
			draw_rect((GXColor){0,0,0,255},0,0,639,479,-1);
			GUI_drawWiiN64(319.5,300,-20.0,2);
			x0 = 320 + PAD_StickX(0)*0.6;
			y0 = 150 - PAD_StickY(0)*0.5;
			GUI_drawLogo(x0, y0, -50.0);
			if (i>=SPLASH_CUBE_SPIN) {
				i = 0;
				splash_step++;
				xstart = x0;
				ystart = y0;
			}
			i++;
			break;
		case 4: //shrink/translate cube & logo to correct places on screen
			//final logo position = (580.0F, 70.0F, -50.0F)
			//final WiiN64 position = (319.5,66.5,-20.0,1)
			draw_background();
			rectColor.a = (u8) ((float)(SPLASH_CUBE_TRANSLATE - i)/SPLASH_CUBE_TRANSLATE * 255);
			draw_rect(rectColor,0,0,639,479,-6);
			y0 = 300 + (66.5 - 300)*i/SPLASH_CUBE_TRANSLATE;
			scale = 2.0 + (1.0 - 2.0)*i/SPLASH_CUBE_TRANSLATE;
			GUI_drawWiiN64(319.5,y0,-20.0,scale);
			x0 = xstart + (580 - xstart)*i/SPLASH_CUBE_TRANSLATE;
			y0 = ystart + (70 - ystart)*i/SPLASH_CUBE_TRANSLATE;
			GUI_drawLogo(x0, y0, -50.0);
			if (i>=SPLASH_CUBE_TRANSLATE)
				return;
			i++;
			break;
	}

	GX_DrawDone ();
	GX_CopyDisp (GUI.xfb[GUI.which_fb], GX_TRUE);
	GX_Flush ();
	VIDEO_SetNextFramebuffer(GUI.xfb[GUI.which_fb]);
	VIDEO_Flush();
	GUI.which_fb ^= 1;
	VIDEO_WaitVSync();

	if(PAD_ButtonsHeld(0) & PAD_BUTTON_A) 
		button_down = true;
	if(!(PAD_ButtonsHeld(0) & PAD_BUTTON_A) && (button_down == true))
		return;
	}
	return;
}

void draw_rect(GXColor rectColor, float ulx, float uly, float lrx, float lry, float z) 
{
	Mtx44 GXprojection2D;
	Mtx GXmodelView2D;

	GX_SetTevColor(GX_TEVREG1,rectColor);

	guMtxIdentity(GXmodelView2D);
	GX_LoadTexMtxImm(GXmodelView2D,GX_TEXMTX0,GX_MTX2x4);
	guMtxTransApply (GXmodelView2D, GXmodelView2D, 0.0F, 0.0F, z);
	GX_LoadPosMtxImm(GXmodelView2D,GX_PNMTX0);
	guOrtho(GXprojection2D, 0, 479, 0, 639, 0, 100);
	GX_LoadProjectionMtx(GXprojection2D, GX_ORTHOGRAPHIC);

	GX_SetZMode(GX_ENABLE,GX_GEQUAL,GX_TRUE);

	GX_ClearVtxDesc();
	GX_SetVtxDesc(GX_VA_PTNMTXIDX, GX_PNMTX0);
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
	//set vertex attribute formats here
	GX_SetVtxAttrFmt(GX_VTXFMT1, GX_VA_POS, GX_POS_XY, GX_F32, 0);

	//enable textures
	GX_SetNumChans (1);
	GX_SetNumTexGens (0);
	GX_SetTevOrder (GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
//	GX_SetTevOp (GX_TEVSTAGE0, GX_REPLACE);
	GX_SetTevColorIn (GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_C1);
	GX_SetTevColorOp (GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV);
	GX_SetTevAlphaIn (GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_A1);
	GX_SetTevAlphaOp (GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV);
	//set blend mode
	GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR); //Fix src alpha
	GX_SetColorUpdate(GX_ENABLE);
	GX_SetAlphaUpdate(GX_DISABLE);
	GX_SetDstAlpha(GX_DISABLE, 0xFF);
	//set cull mode
	GX_SetCullMode (GX_CULL_NONE);

	GX_Begin(GX_QUADS, GX_VTXFMT1, 4);
	GX_Position2f32(ulx, uly);
	GX_Position2f32(lrx, uly);
	GX_Position2f32(lrx, lry);
	GX_Position2f32(ulx, lry);
	GX_End();
}

void draw_background() 
{
	Mtx44 GXprojection2D;
	Mtx GXmodelView2D;

	guMtxIdentity(GXmodelView2D);
	GX_LoadTexMtxImm(GXmodelView2D,GX_TEXMTX0,GX_MTX2x4);
	guMtxTransApply (GXmodelView2D, GXmodelView2D, 0.0F, 0.0F, -5.0F);
	GX_LoadPosMtxImm(GXmodelView2D,GX_PNMTX0);
	guOrtho(GXprojection2D, 0, 479, 0, 639, 0, 100);
	GX_LoadProjectionMtx(GXprojection2D, GX_ORTHOGRAPHIC);

	GX_SetZMode(GX_ENABLE,GX_ALWAYS,GX_TRUE);

	GX_ClearVtxDesc();
	GX_SetVtxDesc(GX_VA_PTNMTXIDX, GX_PNMTX0);
	GX_SetVtxDesc(GX_VA_TEX0MTXIDX, GX_TEXMTX0);
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
	//set vertex attribute formats here
	GX_SetVtxAttrFmt(GX_VTXFMT1, GX_VA_POS, GX_POS_XY, GX_F32, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT1, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);

	//enable textures
	GX_SetNumChans (1);
	GX_SetNumTexGens (1);
	GX_SetTevOrder (GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0); // change to (u8) tile later
	GX_SetTevOp (GX_TEVSTAGE0, GX_REPLACE);
	//set blend mode
	GX_SetBlendMode(GX_BM_NONE, GX_BL_ONE, GX_BL_ZERO, GX_LO_CLEAR); //Fix src alpha
	GX_SetColorUpdate(GX_ENABLE);
	GX_SetAlphaUpdate(GX_ENABLE);
	GX_SetDstAlpha(GX_DISABLE, 0xFF);
	//set cull mode
	GX_SetCullMode (GX_CULL_NONE);

	GX_Begin(GX_QUADS, GX_VTXFMT1, 4);
	GX_Position2f32(0, 0);
	GX_TexCoord2f32(0,0);
	GX_Position2f32(639, 0);
	GX_TexCoord2f32(1,0);
	GX_Position2f32(639, 479);
	GX_TexCoord2f32(1,1);
	GX_Position2f32(0, 479);
	GX_TexCoord2f32(0,1);
	GX_End();
}
