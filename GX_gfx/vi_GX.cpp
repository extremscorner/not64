/* vi_GX.cpp - vi for Gamecube, based off vi_GX
   by Mike Slegeir for Mupen64-GC
 */

#include <stdio.h>
#include <ogc/system.h>
#include <ogc/video.h>
#include <ogc/gx.h>
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
