/**
 * Wii64 - bl_GX.cpp
 * Copyright (C) 2002 Hacktarux 
 * Copyright (C) 2007, 2008 sepp256
 * 
 * N64 GX plugin, based off Hacktarux's soft_gfx
 * by sepp256 for Mupen64-GC
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
#include <stdlib.h>

#include "bl_GX.h"
#include "../gui/DEBUG.h"

unsigned short* BL::zLUT = NULL;

BL::BL(GFX_INFO info) : gfxInfo(info), zero(0), one(0xFFFFFFFF)
{
/*   if(zLUT == NULL)
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
     }*/

	//default blend mode:
	GXdstfactor = GX_BL_INVSRCALPHA;
	GXsrcfactor = GX_BL_SRCALPHA;

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

/*void BL::setCImg(int f, int s, int w, void *c)
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
}*/

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
	sprintf(txtbuffer,"bl: unknown blender source:%d,%d,%d\n", src, pos, cycle);
	DEBUG_print(txtbuffer,DBG_BLINFO);
	break;
      case 3:
	if (pos == 2 || pos == 4)
	  return &zero;
	else if (pos == 1)
	  return &fogColor;
	sprintf(txtbuffer,"bl: unknown blender source:%d,%d,%d\n", src, pos, cycle);
	DEBUG_print(txtbuffer,DBG_BLINFO);
	break;
      default:
	sprintf(txtbuffer,"bl: unknown blender source:%d,%d,%d\n", src, pos, cycle);
	DEBUG_print(txtbuffer,DBG_BLINFO);
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
   if (value & ~0xffff7ff8) {
		sprintf(txtbuffer,"bl: unknwown render mode:%x\n", value & ~0xffff7ff8);
		DEBUG_print(txtbuffer,DBG_BLINFO);
   }
   
   // blender modes
   if (oldBlenderMode == (value>>16)) return;
   oldBlenderMode = value>>16;
   
//   int sa1,sb1,ca1,cb1, sa2, sb2, ca2, cb2;
   blendSrc[0][0] = (value >> 30) & 3;	//sa1
   blendSrc[1][0] = (value >> 28) & 3;	//sa2
   blendSrc[0][1] = (value >> 26) & 3;	//ca1
   blendSrc[1][1] = (value >> 24) & 3;	//ca2
   blendSrc[0][2] = (value >> 22) & 3;	//sb1
   blendSrc[1][2] = (value >> 20) & 3;	//sb2
   blendSrc[0][3] = (value >> 18) & 3;	//cb1
   blendSrc[1][3] = (value >> 16) & 3;	//cb2

   //default blend mode:
   GXdstfactor = GX_BL_ZERO; //INVSRCALPHA;
   GXsrcfactor = GX_BL_ZERO; //SRCALPHA;

//set cycle1 blend mode
	if (blendSrc[0][0] == 1) { // A -> EFB
		if (blendSrc[0][1] == 0) GXdstfactor = GX_BL_SRCALPHA;
//		else if (blendSrc[0][1] == 1) GXdstfactor = GX_BL_SRCALPHA; //fog alpha NI
		else if (blendSrc[0][1] == 2) GXdstfactor = GX_BL_SRCALPHA; //shade alpha NI
		else if (blendSrc[0][1] == 3) GXdstfactor = GX_BL_ZERO;
	} 
	if (blendSrc[0][0] == 0) { //A -> TEVOUT
		if (blendSrc[0][1] == 0) GXsrcfactor = GX_BL_SRCALPHA;
//		else if (blendSrc[0][1] == 1) GXsrcfactor = GX_BL_SRCALPHA; //fog alpha NI
		else if (blendSrc[0][1] == 2) GXsrcfactor = GX_BL_SRCALPHA; //shade alpha NI
		else if (blendSrc[0][1] == 3) GXsrcfactor = GX_BL_ZERO;
	} 
	if (blendSrc[0][0] == 3) { //A -> fogcolor NI
//		GXsrcfactor = GX_BL_SRCALPHA;
	}
	if (blendSrc[0][2] == 1) { // B -> EFB
		if (blendSrc[0][3] == 0) GXdstfactor = GX_BL_INVSRCALPHA;
//		else if (blendSrc[0][3] == 1) GXdstfactor = GX_BL_DSTALPHA; //EFB alpha
		else if (blendSrc[0][3] == 1) GXdstfactor = GX_BL_ZERO; //EFB alpha (never read by soft_gfx)
		else if (blendSrc[0][3] == 2) GXdstfactor = GX_BL_ONE; //one
		else if (blendSrc[0][3] == 3) GXdstfactor = GX_BL_ZERO;
	} 
	if (blendSrc[0][2] == 0) { //B -> TEVOUT
		if (blendSrc[0][3] == 0) GXsrcfactor = GX_BL_INVSRCALPHA;
//		else if (blendSrc[0][3] == 1) GXsrcfactor = GX_BL_DSTALPHA; //EFB alpha
		else if (blendSrc[0][3] == 1) GXsrcfactor = GX_BL_ZERO; //EFB alpha (never read by soft_gfx)
		else if (blendSrc[0][3] == 2) GXsrcfactor = GX_BL_ONE; //one
		else if (blendSrc[0][3] == 3) GXsrcfactor = GX_BL_ZERO;
	} 
	if (blendSrc[0][2] == 3) { //B -> nothingfogcolor NI
//		GXsrcfactor = GX_BL_SRCALPHA;
	}

//set cycle2 blend mode

/*   psa1 = getBlenderSource(sa1, 1, 1);
   psa2 = getBlenderSource(sa2, 1, 2);
   pca1 = getBlenderSource(ca1, 2, 1);
   pca2 = getBlenderSource(ca2, 2, 2);
   psb1 = getBlenderSource(sb1, 3, 1);
   psb2 = getBlenderSource(sb2, 3, 2);
   pcb1 = getBlenderSource(cb1, 4, 1);
   pcb2 = getBlenderSource(cb2, 4, 2);*/
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

void BL::fillModeDraw()
{
//   int *p = (int*)cImg;
//   p[(y*width+x)/2] = fillColor;
   //vi->debug_plot(x,y,(fillColor>>16)&0xffff);
   //vi->debug_plot(x+1,y,fillColor&0xffff);

//No input, directly use fillcolor & alpha

	//testing this: disable textures
	GX_SetNumChans (1);
	GX_SetNumTexGens (0);
	GX_SetTevOrder (GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
	GX_SetTevOp (GX_TEVSTAGE0, GX_PASSCLR);

	GX_SetAlphaCompare(GX_ALWAYS,0,GX_AOP_AND,GX_ALWAYS,0);
	GX_SetZMode(GX_DISABLE,GX_ALWAYS,GX_FALSE);

	//Set FillModeDraw blend modes here
	GX_SetBlendMode(GX_BM_NONE, GX_BL_ONE, GX_BL_ZERO, GX_LO_CLEAR); //Fix src alpha
	GX_SetColorUpdate(GX_ENABLE);
	GX_SetAlphaUpdate(GX_ENABLE);
	GX_SetDstAlpha(GX_DISABLE, 0xFF);

}


void BL::cycle1ModeDraw()
{
	u8 GXzcmp, GXzupd;
	/*
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
   
   if (1)//!force_bl)
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
   p[y*width+x^S16] = colorValue;*/
   
   //vi->debug_plot(x,y,colorValue);
   //vi->flush();

//1 input (Combine 1); optional z and shade
// Combine1: (A-B)C+D
//Tevstage 0 -> A-B
//Tevstage 1 -> Tevprev*C + D
//Blender -> Ca*Aa + Cb*Ab (Tevout, efb, fog)

	//TODO: set up alpha compare
	if(alpha_cvg_sel && cvg_x_alpha && (alphaCompare == 0))
	{
		GX_SetZCompLoc(GX_FALSE);	//do Z compare after texturing (GX_FALSE)
		GX_SetAlphaCompare(GX_GREATER,128,GX_AOP_AND,GX_ALWAYS,0);
//		GX_SetAlphaCompare(GX_GREATER,0,GX_AOP_AND,GX_ALWAYS,0);
		//GX_SetAlphaCompare(u8 comp0,u8 ref0,u8 aop,u8 comp1,u8 ref1)
	}

	//Set up Zbuffering
	if (z_cmp) GXzcmp = GX_ENABLE;
	else GXzcmp = GX_DISABLE;
	if (z_upd && !(zmode_inter && zmode_xlu)) GXzupd = GX_TRUE;
	else GXzupd = GX_FALSE;

	GX_SetZMode(GXzcmp,GX_LEQUAL,GXzupd);

	//Set Cycle1ModeDraw blend modes here
	GX_SetBlendMode(GX_BM_BLEND, GXsrcfactor, GXdstfactor, GX_LO_CLEAR); 
	GX_SetColorUpdate(GX_ENABLE);
	GX_SetAlphaUpdate(GX_ENABLE);
	GX_SetDstAlpha(GX_DISABLE, 0xFF);

}

void BL::cycle2ModeDraw(int x, int y, Color32 c, float z, Color32 shade)
{
	/*
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
   
   if (1)//!force_bl)
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
   p[y*width+x^S16] = colorValue;*/
   
   //vi->debug_plot(x,y,colorValue);
   //vi->flush();

//1 input (combine1) + z + shade

	//TODO: set up alpha compare

/*	//Set up Zbuffering
	if (z_cmp) GXzcmp = GX_ENABLE;
	else GXzcmp = GX_DISABLE;
	if (z_upd && !(zmode_inter && zmode_xlu)) GXzupd = GX_TRUE;
	else GXzupd = GX_FALSE;

	GX_SetZMode(GXzcmp,GX_LEQUAL,GXzupd);*/

	//Set Cycle1ModeDraw blend modes here
	GX_SetBlendMode(GX_BM_BLEND, GXsrcfactor, GXdstfactor, GX_LO_CLEAR); 
	GX_SetColorUpdate(GX_ENABLE);
	GX_SetAlphaUpdate(GX_ENABLE);
	GX_SetDstAlpha(GX_DISABLE, 0xFF);

}

void BL::copyModeDraw()
{
/*   short *p = (short*)cImg;
   if (!alphaCompare || alphaCompare!=1) printf("alphacompare:%d\n", alphaCompare);
   if (!c.getAlpha()) return;
   int colorValue = (int)c;
   colorValue = 
     ((((colorValue >> 24)&0xFF)>>3)<<11) |
     ((((colorValue >> 16)&0xFF)>>3)<< 6) |
     ((((colorValue >>  8)&0xFF)>>3)<< 1);
   p[y*width+x^S16] = colorValue;
   //vi->debug_plot(x,y,colorValue);*/

// 1 input (texture); directly use input, if alpha = 0 don't write
	//testing this: re-enable textures
	GX_SetNumChans (1);
	GX_SetNumTexGens (1);
	// GX_SetTexCoordGen TexCoord0 should be set to an identity 2x4 matrix in GXinit
	GX_SetTevOrder (GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0); // change to (u8) tile later
//	GX_SetTevOp(GX_TEVSTAGE0, GX_MODULATE);	// change this to tile as well...
	GX_SetTevOp (GX_TEVSTAGE0, GX_REPLACE);

	GX_SetAlphaCompare(GX_GREATER,0,GX_AOP_AND,GX_ALWAYS,0);
	GX_SetZCompLoc(GX_TRUE);	//do Z compare before texturing (GX_TRUE)
	GX_SetZMode(GX_DISABLE,GX_LEQUAL,GX_TRUE);

	//Set CopyModeDraw blend modes here
	GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR); //Fix src alpha
	GX_SetColorUpdate(GX_ENABLE);
	GX_SetAlphaUpdate(GX_ENABLE);
	GX_SetDstAlpha(GX_DISABLE, 0xFF);

}

/*void BL::debug_plot(int x, int y, int c)
{
   short *p = (short*)cImg;
   p[y*width+x^S16] = c;
}*/
