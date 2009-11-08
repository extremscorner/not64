/**
 * Wii64 - cc_GX.h
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

#include "color.h"
#include <gccore.h>

#define CC_TEXNULL 0
#define CC_TEX0 1
#define CC_TEX1 2


class CC
{
   int combineKey;
//   Color32 *pa0, *pb0, *pc0, *pd0, *pa1, *pb1, *pc1, *pd1;
//   float *pAa0, *pAb0, *pAc0, *pAd0, *pAa1, *pAb1, *pAc1, *pAd1;
   bool newCycle;
//   int a0,b0,c0,d0,Aa0,Ab0,Ac0,Ad0,a1,b1,c1,d1,Aa1,Ab1,Ac1,Ad1;
 
   // GX parameters
   u8 GXtevcolorarg[2][4];
   u8 GXtevalphaarg[2][4];
   int texSrc[2][4];
   int colorSrc[2][4]; //a0,b0,c0,d0,a1,b1,c1,d1
   int alphaSrc[2][4]; //Aa0,Ab0,Ac0,Ad0,Aa1,Ab1,Ac1,Ad1
   GXColor GXprimColor, GXenvColor, GXshade;

   // constants
//   Color32 zero;
   
   // to be set by the pipeline
//   Color32 texel0;
//   Color32 texel1;
//   Color32 texel0Alpha;
//   Color32 texel1Alpha;
//   Color32 LODFraction;
//   Color32 shade;
   
   // temp value inside the combiner
//   Color32 combined;
//   Color32 combinedAlpha;
   
   int oldCycle1, oldCycle2;
//   Color32* getColorSource(int src, int var);
//   float* getAlphaSource(int src, int var);
   
   // other registers
//   Color32 primColor;
//   Color32 envColor;
   float mLOD;
   float lLOD;
   
 public:
   CC();
   ~CC();
   
   void setCombineKey(int value);
   void setCombineMode(int cycle1, int cycle2);
   void setPrimColor(int color, float mLOD, float lLOD);
   void setEnvColor(int color);
   
   void setShade(const Color32& c);
   void combine1(u8 tile0, bool tex_en);
   void combine2(u8 tile0, u8 tile1, bool tex_en);
};
