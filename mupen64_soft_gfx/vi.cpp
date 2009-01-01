/**
 * Mupen64 - vi.cpp
 * Copyright (C) 2002 Hacktarux
 *
 * Mupen64 homepage: http://mupen64.emulation64.com
 * email address: hacktarux@yahoo.fr
 * 
 * If you want to contribute to the project please contact
 * me first (maybe someone is already making what you are
 * planning to do).
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
 * You should have received a copy of the GNU General Public
 * Licence along with this program; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139,
 * USA.
 *
**/

#include <stdio.h>
#include <math.h>
#include <malloc.h>

#include "vi.h"
#include "global.h"
#include "color.h"
#ifdef DEBUGON
# include <debug.h>
#endif


VI::VI(GFX_INFO info) : gfxInfo(info), bpp(0)
{
	FBtex = (u16*) memalign(32,640*480*2);
}

VI::~VI()
{
	free(FBtex);
}

void VI::statusChanged()
{
   switch (*gfxInfo.VI_STATUS_REG & 3)
     {
      case 2:
	if (bpp != 16)
	  {
	     bpp = 16;
	     setVideoMode(640, 480);
	  }
	break;
      case 3:
	if (bpp != 32)
	  {
	     printf("VI:32bits\n");
	     bpp =32;
	  }
	break;
     }
}

void VI::widthChanged()
{
   /*switch(gfxInfo.HEADER[0x3c])
     {
      case 0x44:
      case 0x46:
      case 0x49:
      case 0x50:
      case 0x53:
      case 0x55:
      case 0x58:
      case 0x59:
	printf("VI:pal rom\n");
	break;
     }
   width = *gfxInfo.VI_WIDTH_REG;
   height = width * 3 / 4;
   initMode();*/
}

unsigned int convert_pixels(short src1, short src2){
	char b1 = ((src1 >>  0) & 0x1F) * (256/32);
	char g1 = ((src1 >>  5) & 0x1F) * (256/32);
	char r1 = ((src1 >> 10) & 0x1F) * (256/32);
	char b2 = ((src2 >>  0) & 0x1F) * (256/32);
	char g2 = ((src2 >>  5) & 0x1F) * (256/32);
	char r2 = ((src2 >> 10) & 0x1F) * (256/32);
	
	int y1, cb1, cr1, y2, cb2, cr2, cb, cr;
	
	y1 = (299 * r1 + 587 * g1 + 114 * b1) / 1000;
	cb1 = (-16874 * r1 - 33126 * g1 + 50000 * b1 + 12800000) / 100000;
	cr1 = (50000 * r1 - 41869 * g1 - 8131 * b1 + 12800000) / 100000;
	
	y2 = (299 * r2 + 587 * g2 + 114 * b2) / 1000;
	cb2 = (-16874 * r2 - 33126 * g2 + 50000 * b2 + 12800000) / 100000;
	cr2 = (50000 * r2 - 41869 * g2 - 8131 * b2 + 12800000) / 100000;
	 
	cb = (cb1 + cb2) >> 1;
	cr = (cr1 + cr2) >> 1;
	 
	return (y1 << 24) | (cb << 16) | (y2 << 8) | cr;
}

void VI::updateScreen()
{
//   printf("Should be updating screen: bpp = %d, width_reg = %d\n", bpp, *gfxInfo.VI_WIDTH_REG);
   if (!bpp) return;
   if (!*gfxInfo.VI_WIDTH_REG) return;
   int h_end = *gfxInfo.VI_H_START_REG & 0x3FF;
   int h_start = (*gfxInfo.VI_H_START_REG >> 16) & 0x3FF;
   int v_end = *gfxInfo.VI_V_START_REG & 0x3FF;
   int v_start = (*gfxInfo.VI_V_START_REG >> 16) & 0x3FF;
   float scale_x = ((int)*gfxInfo.VI_X_SCALE_REG & 0xFFF) / 1024.0f;
   float scale_y = (((int)*gfxInfo.VI_Y_SCALE_REG & 0xFFF)>>1) / 1024.0f;
   
   short *im16 = (short*)((char*)gfxInfo.RDRAM +
			  (*gfxInfo.VI_ORIGIN_REG & 0x7FFFFF));
//   int *buf16 = (int*)getScreenPointer();
   int minx = (640-(h_end-h_start))/2;
   int maxx = 640-minx;
   int miny = (480-(v_end-v_start))/2;
   int maxy = 480-miny;
   int ind = 0;
   float px, py;
   py=0.0f;

#ifdef DEBUGON
   _break();
#endif
   //printf("Beginning to copy framebuffer... N64FB offset = %08x", *gfxInfo.VI_ORIGIN_REG & 0x7FFFFF);
   //printf("\nmin: (%d,%d) max: (%d,%d), GCFB = %08x, N64FB = %08x\n",
   //        minx, miny, maxx, maxy, buf16, im16);
   //printf("scale_x = %f, scale_y = %f\n", scale_x, scale_y);
   //fflush(stdout);
   // Here I'm disabling antialiasing to try to track down the bug
/*   if (TRUE || (*gfxInfo.VI_STATUS_REG & 0x30) == 0x30) // not antialiased
     {
     	//printf(" Not antialiased ");
     	//fflush(stdout);
	for (int j=0; j<480; j++)
	  {
	     if (j < miny || j > maxy)
	       for (int i=0; i<640/2; i++)
		 buf16[j*640/2+i] = 0;
	     else
	       {
		  px=0.0f;
		  for (int i=0; i<640/2; i++)
		    {
		       if (i < minx || i > maxx)
			 buf16[j*640/2+i] = 0;
		       else
			 {
			    short pix1 = im16[((int)py*(*gfxInfo.VI_WIDTH_REG)+(int)px)]>>1;
			    px += scale_x;
			    short pix2 = im16[((int)py*(*gfxInfo.VI_WIDTH_REG)+(int)px)]>>1;
			    px += scale_x;
			    buf16[j*640/2+i] = convert_pixels(pix1, pix2);
			 }
			//printf(" (%d,%d) ", i, j); fflush(stdout);
		    }
		  py += scale_y;
	       }
	  }
     }
   else
     {
     	//printf(" Antialiased ");
     	//fflush(stdout);
	for (int j=0; j<480; j++)
	  {
	     if (j < miny || j > maxy)
	       for (int i=0; i<640; i++)
		 buf16[j*640+i] = 0;
	     else
	       {
		  px=0;
		  for (int i=0; i<640; i++)
		    {
		       if (i < minx || i > maxx)
			 buf16[j*640+i] = 0;
		       else
			 {
			    bool xint = (px - (int)px) == 0.0f, yint = (py - (int)py) == 0.0f;
			    if (xint && yint)
			      {
				 buf16[j*640+i] = 
				   im16[((int)py*(*gfxInfo.VI_WIDTH_REG)+(int)px)^S16]>>1;
			      }
			    else if (yint)
			      {
				 Color16 l,r;
				 int w = *gfxInfo.VI_WIDTH_REG;
				 l=im16[((int)py*w+(int)px)^S16];
				 r=im16[((int)py*w+(int)(px+1.0f))^S16];
				 buf16[j*640+i] = 
				   (int)(l*(1.0f-(px-(int)px))+r*(px-(int)px))>>1;
			      }
			    else if (xint)
			      {
				 Color16 t,b;
				 int w = *gfxInfo.VI_WIDTH_REG;
				 t=im16[((int)py*w+(int)px)^S16];
				 b=im16[((int)(py+1)*w+(int)px)^S16];
				 buf16[j*640+i] = 
				   (int)(t*(1-(py-(int)py))+b*(py-(int)py))>>1;
			      }
			    else
			      {
				 Color16 t,b,l,r;
				 int w = *gfxInfo.VI_WIDTH_REG;
				 l=im16[((int)py*w+(int)px)^S16];
				 r=im16[((int)py*w+(int)(px+1))^S16];
				 t=l*(1-(px-(int)px))+r*(px-(int)px);
				 l=im16[((int)(py+1)*w+(int)px)^S16];
				 r=im16[((int)(py+1)*w+(int)(px+1))^S16];
				 b=l*(1-(px-(int)px))+r*(px-(int)px);
				 buf16[j*640+i] = 
				   (int)(t*(1-(py-(int)py))+b*(py-(int)py))>>1;
			      }
			    px += scale_x;
			 }
		    }
		  py += scale_y;
	       }
	  }
     }*/

   //N64 Framebuffer is in RGB5A1 format, so shift by 1 and retile.
	for (int j=0; j<480; j+=4)
	{
		for (int i=0; i<640; i+=4)
		{
			for (int jj=0; jj<4; jj++)
			{
				if (j+jj < miny || j+jj > maxy)
				{
					FBtex[ind++] = 0;
					FBtex[ind++] = 0;
					FBtex[ind++] = 0;
					FBtex[ind++] = 0;
				}
				else
				{
					px = scale_x*i;
					py = scale_y*(j+jj);
					for (int ii=0; ii<4; ii++)
					{
						if (i+ii < minx || i+ii > maxx)
							FBtex[ind++] = 0;
						else
							FBtex[ind++] = 0x8000 | (im16[((int)py*(*gfxInfo.VI_WIDTH_REG)+(int)px)]>>1);
						px += scale_x;
					}
				}
			}
		}
	}

	GX_SetCopyClear ((GXColor){0,0,0,255}, 0xFFFFFF);
	GX_CopyDisp (vi->getScreenPointer(), GX_TRUE);	//clear the EFB before executing new Dlist
	GX_DrawDone ();
	vi->updateDEBUG();

	//Initialize texture
	GX_InitTexObj(&FBtexObj, FBtex, 640, 480, GX_TF_RGB5A3, GX_CLAMP, GX_CLAMP, GX_FALSE); 
	DCFlushRange(FBtex, 640*480*2);
	GX_InvalidateTexAll();
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
	GX_SetViewport((f32) 0,(f32) 0,(f32) 640,(f32) 480, 0.0f, 1.0f);
	GX_SetScissor((u32) 0,(u32) 0,(u32) 640,(u32) 480);	//Set to the same size as the viewport.
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
		GX_TexCoord2f32( 1.0f, 0.0f );
		GX_Position2f32( 640.0f, 480.0f );
		GX_TexCoord2f32( 1.0f, 1.0f );
		GX_Position2f32( 0.0f, 480.0f );
		GX_TexCoord2f32( 0.0f, 1.0f );
	GX_End();
	GX_DrawDone();

   //printf(" done.\nBlitting...");
   //fflush(stdout);
   blit();
   //printf(" done.\n");
}

void VI::debug_plot(int x, int y, int c)
{
   short *buf16 = (short*)getScreenPointer();
   buf16[y*640+x] = c>>1;
}

void VI::flush()
{
   blit();
}
