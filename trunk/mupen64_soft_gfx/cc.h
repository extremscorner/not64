/**
 * Mupen64 - cc.h
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

#include "color.h"

class CC
{
   int combineKey;
   Color32 *pa0, *pb0, *pc0, *pd0, *pa1, *pb1, *pc1, *pd1;
   float *pAa0, *pAb0, *pAc0, *pAd0, *pAa1, *pAb1, *pAc1, *pAd1;
   
   // constants
   Color32 zero;
   
   // to be set by the pipeline
   Color32 texel0;
   Color32 texel1;
   Color32 texel0Alpha;
   Color32 texel1Alpha;
   Color32 LODFraction;
   Color32 shade;
   
   // temp value inside the combiner
   Color32 combined;
   Color32 combinedAlpha;
   
   int oldCycle1, oldCycle2;
   Color32* getColorSource(int src, int var);
   float* getAlphaSource(int src, int var);
   
   // other registers
   Color32 primColor;
   Color32 envColor;
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
   Color32 combine1(const Color32& texel);
   Color32 combine2(const Color32& texel1, const Color32& texel2);
};
