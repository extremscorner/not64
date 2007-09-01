/**
 * Mupen64 - bl.h
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

#include "../main/winlnxdefs.h"
#include "Gfx_#1.3.h"
#include "color.h"
#include "global.h"

class BL
{
   GFX_INFO gfxInfo;
   
   int alphaCompare;
   int colorDither;
   int alphaDither;
   int depthSource;
   
   // render modes
   bool aa_en;
   bool z_cmp;
   bool z_upd;
   bool im_rd;
   bool clr_on_cvg;
   bool cvg_dst_wrap;
   bool cvg_dst_full;
   bool zmode_inter;
   bool zmode_xlu;
   bool cvg_x_alpha;
   bool alpha_cvg_sel;
   bool force_bl;
   int renderMode;
   
   // blender modes
   int oldBlenderMode;
   Color32 *psa1, *psb1, *pca1, *pcb1, *psa2, *psb2, *pca2, *pcb2;
   
   // constant colors
   Color32 zero;
   Color32 one;
   
   // color variables (to be set before blending)
   Color32 pixelColor;
   Color32 blendedPixelColor;
   Color32 memoryColor;
   Color32 invertedAlpha;
   Color32 shadeColor;
   
   // color image
   int format;
   int size;
   int width;
   void *cImg;
   
   // zimage
   void *zImg;
   
   // BL registers
   int fillColor;
   Color32 blendColor;
   Color32 fogColor;
   
   Color32 *getBlenderSource(int src, int pos, int cycle);
   
   // z encoding LUT
   static unsigned short *zLUT;
   
 public:
   BL(GFX_INFO);
   ~BL();
   
   void setAlphaCompare(int value);
   void setColorDither(int value);
   void setAlphaDither(int value);
   void setDepthSource(int value);
   void setCImg(int format, int size, int width, void *cImg);
   void setZImg(void *zImg);
   void setBlender(int value);
   void setFillColor(int color);
   void setFogColor(int color);
   void setBlendColor(int color);
   
   // pipeline members
   void fillModeDraw(int x, int y);
   void cycle1ModeDraw(int x, int y, Color32 c, float z=0, Color32 shade = Color32(0,0,0,0));
   void cycle2ModeDraw(int x, int y, Color32 c, float z=0, Color32 shade = Color32(0,0,0,0));
   void copyModeDraw(int x, int y, Color32 c);
   void debug_plot(int x, int y, int c);
};
