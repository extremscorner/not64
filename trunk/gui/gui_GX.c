/* gui_GX.c - gui that uses GX graphics
   by sepp256 for Mupen64-GC
 */

#include <stdio.h>
#include <sdcard.h>
#include <gccore.h>
#include "gui_GX.h"
#include "font.h"


static GUIinfo GUI;
static int GUI_on = 0;
static lwp_t GUIthread;
GXTexObj BGtex;
GXTlutObj BGtlut;
//static u16 BGtexture[640*480] ATTRIBUTE_ALIGN (32); //size of background image
//static u8 BGtexture[640*480] ATTRIBUTE_ALIGN (32); //size of background image
//static u16 BGtextureCI[256] ATTRIBUTE_ALIGN (32); //size of 256 color index

extern  u8 BGtexture[];
extern  u16 BGtextureCI[];
extern long BGtexture_length;
extern long BGtextureCI_length;

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

	//TODO: init spinning cube display list

	GUI_on = 1;

	stat = LWP_CreateThread(&GUIthread, &GUI_main, NULL, NULL, 0, 100);
	if (stat<0)
		GUI_print("Error creating GUIthread.\n");

}

void GUI_toggle()
{
	s32 stat;

	if (GUI_on == 1) {
		stat = LWP_SuspendThread(GUIthread);
		GX_CopyDisp (GUI.xfb[GUI.which_fb], GX_TRUE); // This clears the efb
		GX_CopyDisp (GUI.xfb[GUI.which_fb], GX_TRUE); // This clears the xfb
		GX_Flush ();
		VIDEO_SetNextFramebuffer(GUI.xfb[GUI.which_fb]);
		VIDEO_Flush();
		GUI.which_fb ^= 1;
	}
	else {
		GUI_clear();
		GUI_loadBGtex();
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
	int i;

	while(1) {
//	PAD_ScanPads(0);

	guMtxIdentity(GXmodelView2D);
	GX_LoadPosMtxImm(GXmodelView2D,GX_PNMTX2);
	GX_LoadTexMtxImm(GXmodelView2D,GX_TEXMTX0,GX_MTX2x4);
	guOrtho(GXprojection2D, 0, 479, 0, 639, 0, 1);
	GX_LoadProjectionMtx(GXprojection2D, GX_ORTHOGRAPHIC);

	GX_SetZMode(GX_DISABLE,GX_GEQUAL,GX_TRUE);

	GX_ClearVtxDesc();
	GX_SetVtxDesc(GX_VA_PTNMTXIDX, GX_PNMTX2);
	GX_SetVtxDesc(GX_VA_TEX0MTXIDX, GX_TEXMTX0);
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
	//set vertex attribute formats here
	GX_SetVtxAttrFmt(GX_VTXFMT1, GX_VA_POS, GX_POS_XY, GX_F32, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT1, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);

	//disable textures
	GX_SetNumChans (1);
	GX_SetNumTexGens (1);
	GX_SetTevOrder (GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0); // change to (u8) tile later
	GX_SetTevOp (GX_TEVSTAGE0, GX_REPLACE);
	//set blend mode
//		GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR); //Fix src alpha
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

    GX_DrawDone ();
	GX_CopyDisp (GUI.xfb[GUI.which_fb], GX_FALSE);
    GX_Flush ();
	GUI_displayText();
	VIDEO_SetNextFramebuffer(GUI.xfb[GUI.which_fb]);
	VIDEO_Flush();
	GUI.which_fb ^= 1;
	VIDEO_WaitVSync();
	VIDEO_WaitVSync();
	}
}

void GUI_displayText(){
	int i = 1;
	char** temp_textptrs;

	temp_textptrs = GUI_get_text();
	for (i=0;i<GUI_TEXT_HEIGHT;i++)
		write_font(60,(20*i+85),temp_textptrs[i],GUI.xfb,GUI.which_fb); 
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
	//GX_TL_RGB565 == 0x05
	GX_InitTlutObj(&BGtlut, BGtextureCI,(u8) 0x01,(u16) 256/16); //GX_TL_RGB565 is missing in gx.h
	DCFlushRange(BGtextureCI, 256*2);
	GX_LoadTlut(&BGtlut, GX_TLUT0);	// use GX_TLUT0 or (u32) tile??

//	GX_InitTexObj(&BGtex, BGtexture, (u16) 640, (u16) 480, GX_TF_RGB565, GX_CLAMP, GX_CLAMP, GX_FALSE); 
//	DCFlushRange(BGtexture, 640*480*2);
	GX_InitTexObjCI(&BGtex, BGtexture, (u16) 640, (u16) 480, GX_TF_CI8, GX_CLAMP, GX_CLAMP, GX_FALSE, GX_TLUT0); 
	DCFlushRange(BGtexture, 640*480);
	GX_LoadTexObj(&BGtex, GX_TEXMAP0); // should set to (u8) tile or GX_TEXMAP0

	printf("Loaded Texture into TMEM.\n");

	return 1;
}

void GUI_drawLogo(){

	//draw test geometry here
/*  Mtx v, p;            // view and perspective matrices
  Vector cam = { 0.0F, 0.0F, 0.0F }, up =
  {
  0.0F, 1.0F, 0.0F}, look =
  {
  0.0F, 0.0F, -1.0F};
  guLookAt (v, &cam, &up, &look);
  guPerspective (p, 60, (f32) 4 / 3, 10.0F, 300.0F);
  GX_LoadProjectionMtx (p, GX_PERSPECTIVE);

  	  printf("\tPrj =\t%f, %f, %f, %f\n",p[0][0],p[0][1],p[0][2],p[0][3]);
	  printf("\t     \t%f, %f, %f, %f\n",p[1][0],p[1][1],p[1][2],p[1][3]);
	  printf("\t     \t%f, %f, %f, %f\n",p[2][0],p[2][1],p[2][2],p[2][3]);
	  printf("\t     \t%f, %f, %f, %f\n",p[3][0],p[3][1],p[3][2],p[3][3]);



    //draw cube
    Mtx m;            // model matrix.
  Mtx mv;            // modelview matrix.
  Vector axis = { -1, 1, 0 };
  static float rotateby = 0;
 
  rotateby++;
 
  // move the cube out in front of us and rotate it
  guMtxIdentity (m);
  guMtxRotAxisDeg (m, &axis, rotateby);
  guMtxTransApply (m, m, 0.0F, 0.0F, -200.0F);
  guMtxConcat (v, m, mv);
  // load the modelview matrix into matrix memory
  GX_LoadPosMtxImm (mv, GX_PNMTX0);
	  printf("\tMV =\t%f, %f, %f, %f\n",mv[0][0],mv[0][1],mv[0][2],mv[0][3]);
	  printf("\t\t%f, %f, %f, %f\n",mv[1][0],mv[1][1],mv[1][2],mv[1][3]);
	  printf("\t\t%f, %f, %f, %f\n",mv[2][0],mv[2][1],mv[2][2],mv[2][3]);

    GX_ClearVtxDesc();
   GX_SetVtxDesc(GX_VA_PTNMTXIDX, GX_PNMTX0);
   //GX_SetVtxDesc(GX_VA_TEX0MTXIDX, ...);
   GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
   GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);

      //set vertex attribute formats here
   GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
//   if (lighting) GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_NRM, GX_NRM_XYZ, GX_F32, 0);
   GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);

  GX_SetNumChans (1);
  GX_SetNumTexGens (0);
  GX_SetTevOrder (GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
  GX_SetTevOp (GX_TEVSTAGE0, GX_PASSCLR);

     GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 3);
     // vert 0
     GX_Position3f32((f32) -10, (f32) -10, (f32) 0);
//	 if (lighting) GX_Normal3f32(vtx[v0].n[0], vtx[v0].n[1], vtx[v0].n[2]);
//	 GXcol.r = (u8)vtx[v0].c.getR();
//	 GXcol.g = (u8)vtx[v0].c.getG();
//	 GXcol.b = (u8)vtx[v0].c.getB();
//	 GXcol.a = (u8)vtx[v0].c.getAlpha();
//	 GX_Color4u8(GXcol.r, GXcol.g, GXcol.b, GXcol.a);
	 GX_Color4u8((u8) 50, (u8) 250, (u8) 10, (u8) 255);

     // vert 1
     GX_Position3f32((f32) 0, (f32) 10, (f32) 0);
	 GX_Color4u8((u8) 50, (u8) 250, (u8) 10, (u8) 255);

     // vert 2
     GX_Position3f32((f32) 10, (f32) -10, (f32) 0);
	 GX_Color4u8((u8) 50, (u8) 250, (u8) 10, (u8) 255);
   GX_End();

*/

}
