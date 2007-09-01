/**
 * Mupen64 - bl.cpp
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
#include <stdlib.h>

#include "bl.h"

unsigned short* BL::zLUT = NULL;

BL::BL(GFX_INFO info) : gfxInfo(info), zero(0), one(0xFFFFFFFF)
{
   if(zLUT == NULL)
     {
	zLUT = new unsigned short[0x40000];
	
	for(int i=0; i<0x40000; i++)
	  {
	     unsigned long exponent = 0;
	     unsigned long testbit = 1 << 17;
	     while((i & testbit) && (exponent < 7))
	       {
		  exponent++;
		  testbit = 1 << (17 - exponent);
	       }
	     
	     unsigned short mantissa = (i >> (6 - (6 < exponent ? 6 : exponent))) & 0x7ff;
	     zLUT[i] = ((exponent << 11) | mantissa) << 2;
	  }
     }
}

BL::~BL()
{
}

void BL::setAlphaCompare(int value)
{
   alphaCompare = value;
}

void BL::setColorDither(int value)
{
   colorDither = value;
}

void BL::setAlphaDither(int value)
{
   alphaDither = value;
}

void BL::setDepthSource(int value)
{
   depthSource = value;
}

void BL::setCImg(int f, int s, int w, void *c)
{
   if (f != 0 || s != 2) printf("bl: unknown framebuffer format\n");
   format = f;
   size = s;
   width = w;
   cImg = c;
}

void BL::setZImg(void *z)
{
   zImg = z;
}

Color32* BL::getBlenderSource(int src, int pos, int cycle)
{
   switch(src)
     {
      case 0:
	if (pos == 1 || pos == 3)
	  {
	     if (cycle == 1)
	       return &pixelColor;
	     else
	       return &blendedPixelColor;
	  }
	else if (pos == 2)
	  return &pixelColor;
	else 
	  return &invertedAlpha;
	break;
      case 1:
	if (pos == 1 || pos == 3)
	  return &memoryColor;
	else if (pos == 2)
	  return &fogColor;
	else if (pos == 4)
	  return &memoryColor;
	break;
      case 2:
	if (pos == 4)
	  return &one;
	else if (pos == 2)
	  return &shadeColor;
	printf("bl: unknown blender source:%d,%d,%d\n", src, pos, cycle);
	break;
      case 3:
	if (pos == 2 || pos == 4)
	  return &zero;
	else if (pos == 1)
	  return &fogColor;
	printf("bl: unknown blender source:%d,%d,%d\n", src, pos, cycle);
	break;
      default:
	printf("bl: unknown blender source:%d,%d,%d\n", src, pos, cycle);
     }
   return NULL;
}

void BL::setBlender(int value)
{
   // render modes
   aa_en         = (value & 0x0008) != 0;
   z_cmp         = (value & 0x0010) != 0;
   z_upd         = (value & 0x0020) != 0;
   im_rd         = (value & 0x0040) != 0;
   clr_on_cvg    = (value & 0x0080) != 0;
   cvg_dst_wrap  = (value & 0x0100) != 0;
   cvg_dst_full  = (value & 0x0200) != 0;
   zmode_inter   = (value & 0x0400) != 0;
   zmode_xlu     = (value & 0x0800) != 0;
   cvg_x_alpha   = (value & 0x1000) != 0;
   alpha_cvg_sel = (value & 0x2000) != 0;
   force_bl      = (value & 0x4000) != 0;
   renderMode = value & 0xffff;
   if (value & ~0xffff7ff8)
     printf("bl: unknwown render mode:%x\n", value & ~0xffff7ff8);
   
   // blender modes
   if (oldBlenderMode == (value>>16)) return;
   oldBlenderMode = value>>16;
   
   int sa1,sb1,ca1,cb1, sa2, sb2, ca2, cb2;
   sa1 = (value >> 30) & 3;
   sa2 = (value >> 28) & 3;
   ca1 = (value >> 26) & 3;
   ca2 = (value >> 24) & 3;
   sb1 = (value >> 22) & 3;
   sb2 = (value >> 20) & 3;
   cb1 = (value >> 18) & 3;
   cb2 = (value >> 16) & 3;

   psa1 = getBlenderSource(sa1, 1, 1);
   psa2 = getBlenderSource(sa2, 1, 2);
   pca1 = getBlenderSource(ca1, 2, 1);
   pca2 = getBlenderSource(ca2, 2, 2);
   psb1 = getBlenderSource(sb1, 3, 1);
   psb2 = getBlenderSource(sb2, 3, 2);
   pcb1 = getBlenderSource(cb1, 4, 1);
   pcb2 = getBlenderSource(cb2, 4, 2);
}

void BL::setFillColor(int color)
{
   fillColor = color;
}

void BL::setFogColor(int color)
{
   fogColor = color;
}

void BL::setBlendColor(int color)
{
   blendColor = color;
}

void BL::fillModeDraw(int x, int y)
{
   int *p = (int*)cImg;
   p[(y*width+x)/2] = fillColor;
   //vi->debug_plot(x,y,(fillColor>>16)&0xffff);
   //vi->debug_plot(x+1,y,fillColor&0xffff);
}


void BL::cycle1ModeDraw(int x, int y, Color32 c, float z, Color32 shade)
{
   short *p = (short*)cImg;
   
   // extract colors
   pixelColor = c;
   memoryColor = ((((p[y*width+x^S16] >> 11)&0x1F)<<3)<<24 |
		  (((p[y*width+x^S16] >>  6)&0x1F)<<3)<<16 |
		  (((p[y*width+x^S16] >>  1)&0x1F)<<3)<<8);
   shadeColor = shade;
   
   // encode z value
   int fz = (int)(z*8.0f+0.5f);
   unsigned short encodedZ = zLUT[fz];
   unsigned short *pz = (unsigned short*)zImg;
   
   if (1/*!force_bl*/)
     {
	if(alpha_cvg_sel)
	  {
	     if (cvg_x_alpha)
	       {
		  if(alphaCompare == 0)
		    {
		       if(!pixelColor.getAlpha()) return;
		    }
		  else if(alphaCompare == 1) printf("alpha_cvg_sel + cvg_x_alpha + alphaCompare = 1\n");
		  else printf("alpha_cvg_sel + cvg_x_alpha + alphaCompare = 2\n");
	       }
	     else
	       {
		  pixelColor.setAlpha(255.0f);
		  if(alphaCompare == 0)
		    {
		       if(!pixelColor.getAlpha()) return;
		    }
		  else if(alphaCompare == 1) printf("alpha_cvg_sel + !cvg_x_alpha + alphaCompare = 1\n");
		  else printf("alpha_cvg_sel + !cvg_x_alpha + alphaCompare = 2\n");
	       }
	  }
	//else printf("!alpha_cvg_sel\n");
     }
   
   if (z_cmp)
     {
	if (depthSource)
	  printf("BL:depth_source:%d\n", depthSource);
	
	if(fz < 0) return;
	if(fz >= 0x40000) return; // over this value it can't be encoded
	if(zmode_inter && zmode_xlu)
	  {
	     if(encodedZ > pz[y*width+x^S16]+256) return;
	  }
	else
	  if (encodedZ > pz[y*width+x^S16]) return;
     }
   
   if (z_upd && !(zmode_inter && zmode_xlu))
     pz[y*width+x^S16] = encodedZ;
   
   invertedAlpha = Color32(255.0f-pca1->getAlpha(), 255.0f-pca1->getAlpha(),
			   255.0f-pca1->getAlpha(), 255.0f-pca1->getAlpha());
   
   // extracting coefficients
   float ca = pca1->getAlpha() / 255.0f;
   float cb = pcb1->getAlpha() / 255.0f;
   
   // do the actual blending
   Color32 blendedColor = (*psa1 * ca + *psb1 * cb);
   
   // writing color on screen
   int colorValue;
   colorValue = (int)blendedColor;
   
   colorValue = 
     ((((colorValue >> 24)&0xFF)>>3)<<11) |
     ((((colorValue >> 16)&0xFF)>>3)<< 6) |
     ((((colorValue >>  8)&0xFF)>>3)<< 1);
   p[y*width+x^S16] = colorValue;
   
   //vi->debug_plot(x,y,colorValue);
   //vi->flush();
}

void BL::cycle2ModeDraw(int x, int y, Color32 c, float z, Color32 shade)
{
   short *p = (short*)cImg;
   
   // extract colors
   pixelColor = c;
   memoryColor = ((((p[y*width+x^S16] >> 11)&0x1F)<<3)<<24 |
		  (((p[y*width+x^S16] >>  6)&0x1F)<<3)<<16 |
		  (((p[y*width+x^S16] >>  1)&0x1F)<<3)<<8);
   shadeColor = shade;
   
   // encode z value
   int fz = (int)(z*8.0f+0.5f);
   unsigned short encodedZ = zLUT[fz];
   unsigned short *pz = (unsigned short*)zImg;
   
   if (1/*!force_bl*/)
     {
	if(alpha_cvg_sel)
	  {
	     if (cvg_x_alpha)
	       {
		  if(alphaCompare == 0)
		    {
		       if(!pixelColor.getAlpha()) return;
		    }
		  else if(alphaCompare == 1) printf("alpha_cvg_sel + cvg_x_alpha + alphaCompare = 1\n");
		  else printf("alpha_cvg_sel + cvg_x_alpha + alphaCompare = 2\n");
	       }
	     else
	       {
		  pixelColor.setAlpha(255.0f);
		  if(alphaCompare == 0)
		    {
		       if(!pixelColor.getAlpha()) return;
		    }
		  else if(alphaCompare == 1) printf("alpha_cvg_sel + !cvg_x_alpha + alphaCompare = 1\n");
		  else printf("alpha_cvg_sel + !cvg_x_alpha + alphaCompare = 2\n");
	       }
	  }
	//else printf("!alpha_cvg_sel\n");
     }
   
   if (z_cmp)
     {
	if (depthSource)
	  printf("BL:depth_source:%d\n", depthSource);
	
	if(fz < 0) return;
	if(fz >= 0x40000) return; // over this value it can't be encoded
	if(zmode_inter && zmode_xlu)
	  {
	     if(encodedZ > pz[y*width+x^S16]+256) return;
	  }
	else
	  if (encodedZ > pz[y*width+x^S16]) return;
     }
   
   if (z_upd && !(zmode_inter && zmode_xlu))
     pz[y*width+x^S16] = encodedZ;
   
   invertedAlpha = Color32(255.0f-pca1->getAlpha(), 255.0f-pca1->getAlpha(),
			   255.0f-pca1->getAlpha(), 255.0f-pca1->getAlpha());
   
   // extracting coefficients
   float ca = pca1->getAlpha() / 255.0f;
   float cb = pcb1->getAlpha() / 255.0f;
   
   // do the actual blending
   blendedPixelColor = (*psa1 * ca + *psb1 * cb);
   
   // extracting coefficients for cycle 2
   ca = pca2->getAlpha() / 255.0f;
   cb = pcb2->getAlpha() / 255.0f;
   
   // do the actual blending for cycle 2
   Color32 blendedColor = (*psa2 * ca + *psb2 * cb);
   
   // writing color on screen
   int colorValue;
   colorValue = (int)blendedColor;
   
   colorValue = 
     ((((colorValue >> 24)&0xFF)>>3)<<11) |
     ((((colorValue >> 16)&0xFF)>>3)<< 6) |
     ((((colorValue >>  8)&0xFF)>>3)<< 1);
   p[y*width+x^S16] = colorValue;
   
   //vi->debug_plot(x,y,colorValue);
   //vi->flush();
}

void BL::copyModeDraw(int x, int y, Color32 c)
{
   short *p = (short*)cImg;
   if (!alphaCompare || alphaCompare!=1) printf("alphacompare:%d\n", alphaCompare);
   if (!c.getAlpha()) return;
   int colorValue = (int)c;
   colorValue = 
     ((((colorValue >> 24)&0xFF)>>3)<<11) |
     ((((colorValue >> 16)&0xFF)>>3)<< 6) |
     ((((colorValue >>  8)&0xFF)>>3)<< 1);
   p[y*width+x^S16] = colorValue;
   //vi->debug_plot(x,y,colorValue);
}

void BL::debug_plot(int x, int y, int c)
{
   short *p = (short*)cImg;
   p[y*width+x^S16] = c;
}
