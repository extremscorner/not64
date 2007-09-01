/**
 * Mupen64 - cc.cpp
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

#include "global.h"
#include "cc.h"

CC::CC() : zero(0), oldCycle1(0), oldCycle2(0)
{
}

CC::~CC()
{
}

void CC::setCombineKey(int value)
{
   combineKey = value;
}

Color32* CC::getColorSource(int src, int var)
{
   switch(src)
     {
      case 0:
	return &combined;
	break;
      case 1:
	return &texel0;
	break;
      case 2:
	return &texel1;
	break;
      case 3:
	return &primColor;
	break;
      case 4:
	return &shade;
	break;
      case 5:
	return &envColor;
	break;
      case 7:
	if (var == 4)
	  return &zero;
	printf("CC:unknown color combiner source:%d,%d\n", src, var);
	return &combinedAlpha;
	break;
      case 8:
	if (var == 3)
	  return &texel0Alpha;
	else
	  printf("CC:unknown color combiner source:%d,%d\n", src, var);
	break;
      case 9:
	if (var == 3)
	  return &texel1Alpha;
	else
	  return &zero;
	break;
      case 12:
	if (var == 2)
	  return &zero;
	printf("CC:unknown color combiner source:%d,%d\n", src, var);
	break;
      case 13:
	if (var == 3)
	  return &LODFraction;
	else
	  printf("CC:unknown color combiner source:%d,%d\n", src, var);
	break;
      case 15:
	if (var == 1 || var == 2)
	  return &zero;
	printf("CC:unknown color combiner source:%d,%d\n", src, var);
	break;
      case 31:
	return &zero;
	break;
      default:
	printf("CC:unknown color combiner source:%d\n", src);
     }
   return &zero;
}

float* CC::getAlphaSource(int src, int var)
{
   switch(src)
     {
      case 0:
	if (var == 3)
	  return LODFraction.getAlphap();
	return combined.getAlphap();
	break;
      case 1:
	return texel0.getAlphap();
	break;
      case 2:
	return texel1.getAlphap();
	break;
      case 3:
	return primColor.getAlphap();
	break;
      case 4:
	return shade.getAlphap();
	break;
      case 5:
	return envColor.getAlphap();
	break;
      case 7:
	return zero.getAlphap();
	break;
      default:
	printf("CC:unknown alpha combiner source:%d\n", src);
     }
   return zero.getAlphap();
}

void CC::setCombineMode(int cycle1, int cycle2)
{
   if (cycle1 == oldCycle1 && cycle2 == oldCycle2) return;
   oldCycle1 = cycle1;
   oldCycle2 = cycle2;
   
   int a0,b0,c0,d0,Aa0,Ab0,Ac0,Ad0,a1,b1,c1,d1,Aa1,Ab1,Ac1,Ad1;
   a0 = (cycle1 >> 20) & 0xF;
   c0 = (cycle1 >> 15) & 0x1F;
   Aa0= (cycle1 >> 12) & 0x7;
   Ac0= (cycle1 >>  9) & 0x7;
   a1 = (cycle1 >>  5) & 0xF;
   c1 = (cycle1 >>  0) & 0x1F;
   b0 = (cycle2 >> 28) & 0xF;
   d0 = (cycle2 >> 15) & 0x7;
   Ab0= (cycle2 >> 12) & 0x7;
   Ad0= (cycle2 >>  9) & 0x7;
   b1 = (cycle2 >> 24) & 0xF;
   Aa1= (cycle2 >> 21) & 0x7;
   Ac1= (cycle2 >> 18) & 0x7;
   d1 = (cycle2 >>  6) & 0x7;
   Ab1= (cycle2 >>  3) & 0x7;
   Ad1= (cycle2 >>  0) & 0x7;
   
   pa0 = getColorSource(a0,1);
   pb0 = getColorSource(b0,2);
   pc0 = getColorSource(c0,3);
   pd0 = getColorSource(d0,4);
   pa1 = getColorSource(a1,1);
   pb1 = getColorSource(b1,2);
   pc1 = getColorSource(c1,3);
   pd1 = getColorSource(d1,4);
   pAa0 = getAlphaSource(Aa0,1);
   pAb0 = getAlphaSource(Ab0,2);
   pAc0 = getAlphaSource(Ac0,3);
   pAd0 = getAlphaSource(Ad0,4);
   pAa1 = getAlphaSource(Aa1,1);
   pAb1 = getAlphaSource(Ab1,2);
   pAc1 = getAlphaSource(Ac1,3);
   pAd1 = getAlphaSource(Ad1,4);
}

void CC::setPrimColor(int color, float m, float l)
{
   primColor = color;
   mLOD = m;
   lLOD = l;
}

void CC::setEnvColor(int color)
{
   envColor = color;
}

void CC::setShade(const Color32& c)
{
   shade = c;
}

Color32 CC::combine1(const Color32& texel)
{
   texel0 = texel;
   texel0Alpha = Color32(texel0.getAlpha(), texel0.getAlpha(), texel0.getAlpha(), texel0.getAlpha());
   Color32 c =  (*pa0 - *pb0)* *pc0 + *pd0;
   float Ac0 = *pAc0 / 255.0f;
   c.setAlpha((Ac0 * (*pAa0 - *pAb0)) + *pAd0);
   return c;
}

Color32 CC::combine2(const Color32& texela, const Color32& texelb)
{
   texel0 = texela;
   texel0Alpha = Color32(texel0.getAlpha(), texel0.getAlpha(), texel0.getAlpha(), texel0.getAlpha());
   texel1 = texelb;
   texel1Alpha = Color32(texel1.getAlpha(), texel1.getAlpha(), texel1.getAlpha(), texel1.getAlpha());
   
   combined = (*pa0 - *pb0)* *pc0 + *pd0;
   float Ac0 = *pAc0 / 255.0f;
   combined.setAlpha((Ac0 * (*pAa0 - *pAb0)) + *pAd0);
   
   Color32 c = (*pa1 - *pb1)* *pc1 + *pd1;
   float Ac1 = *pAc1 / 255.0f;
   c.setAlpha((Ac1 * (*pAa1 - *pAb1)) + *pAd1);
   
   return c;
}
