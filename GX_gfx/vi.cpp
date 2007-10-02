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

#include "vi.h"
#include "global.h"
#include "color.h"

VI::VI(GFX_INFO info) : gfxInfo(info), bpp(0)
{
}

VI::~VI()
{
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
/*	
   //printf("Should be updating screen: bpp = %d, width_reg = %d\n", bpp, *gfxInfo.VI_WIDTH_REG);
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
   int *buf16 = (int*)getScreenPointer();
   int minx = 0;//(640-(h_end-h_start))/2;
   int maxx = 640-minx;
   int miny = 0;//(480-(v_end-v_start))/2;
   int maxy = 480-miny;
   float px, py;
   py=0.0f;
   //printf("Beginning to copy framebuffer... N64FB offset = %08x", *gfxInfo.VI_ORIGIN_REG & 0x7FFFFF);
   //printf("\nmin: (%d,%d) max: (%d,%d), GCFB = %08x, N64FB = %08x\n",
   //        minx, miny, maxx, maxy, buf16, im16);
   //printf("scale_x = %f, scale_y = %f\n", scale_x, scale_y);
   //fflush(stdout);
   // Here I'm disabling antialiasing to try to track down the bug
   if (TRUE || (*gfxInfo.VI_STATUS_REG & 0x30) == 0x30) // not antialiased
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
     }
	 */
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
//   blit();
}
