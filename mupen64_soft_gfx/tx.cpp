/**
 * Mupen64 - tx.cpp
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

#include "tx.h"
#include "global.h"

TX::TX(GFX_INFO info) : gfxInfo(info)
{
   for(int i=0; i<8; i++) unpackTexel[i] = NULL;
}

TX::~TX()
{
}

void TX::setTextureLUT(int value)
{
   textureLUT = value;
}

void TX::setTextureLOD(int value)
{
   textureLOD = value;
}

void TX::setTextureDetail(int value)
{
   textureDetail = value;
}

void TX::setTexturePersp(int value)
{
   texturePersp = value;
}

void TX::setTImg(int f, int s, int w, void *t)
{
   format = f;
   size = s;
   width = w;
   tImg = t;
}

void TX::setTile(int f, int s, int l, int t, int tile, int p,
		 int ct, int mt, int st, int cs, int ms, int ss)
{
   descriptor[tile].format  = f;
   descriptor[tile].size    = s;
   descriptor[tile].line    = l;
   descriptor[tile].tmem    = t;
   descriptor[tile].palette = p;
   descriptor[tile].cmt     = ct;
   descriptor[tile].maskt   = mt;
   descriptor[tile].shiftt  = st;
   descriptor[tile].cms     = cs;
   descriptor[tile].masks   = ms;
   descriptor[tile].shifts  = ss;
   
   switch(descriptor[tile].format)
     {
      case 0: // RGBA
	switch(descriptor[tile].size)
	  {
	   case 0: // doesn't exist
	     break;
	   case 2: // RGBA16
	     unpackTexel[tile] = &TX::unpack_RGBA16;
	     break;
	   default:
	     printf("TX:unknown setTile RGBA size : %d\n", descriptor[tile].size);
	  }
	break;
      case 2: // CI
	switch(descriptor[tile].size)
	  {
	   case 1: // CI8
	     if (textureLUT == 2)
	       unpackTexel[tile] = &TX::unpack_CI8_RGBA16;
	     else
	       printf("TX:unknoqn setTile CI8 LUT format:%d\n", textureLUT);
	     break;
	   default:
	     printf("TX:unknown setTile CI size : %d\n", descriptor[tile].size);
	  }
	break;
      case 3: // IA
	switch(descriptor[tile].size)
	  {
	   case 0: // IA4
	     unpackTexel[tile] = &TX::unpack_IA4;
	     break;
	   case 1: // IA8
	     unpackTexel[tile] = &TX::unpack_IA8;
	     break;
	   case 2: // IA16
	     unpackTexel[tile] = &TX::unpack_IA16;
	     break;
	   default:
	     printf("TX:unknown setTile IA size : %d\n", descriptor[tile].size);
	  }
	break;
      default:
	printf("TX:unknown setTile format : %d\n", descriptor[tile].format);
     }
}

void TX::loadBlock(float uls, float ult, int tile, float lrs, int dxt)
{
   if ((int)uls != 0 || (int)ult != 0) printf("tx:unknown loadBlock\n");
   for (int i=0; i<((int)lrs+1)*8; i++)
     tmem[descriptor[tile].tmem*8+i] = ((unsigned char*)tImg)[i];
}

void TX::loadTile(int tile, float uls, float ult, float lrs, float lrt)
{
   if (!size) printf("loadtile tries to load a 4 bit texture\n");
   for (int i=(int)ult; i<=(int)lrt; i++)
	{
	   for (int j=(int)uls*size; j<=(int)lrs*size; j++)
	     {
		tmem[descriptor[tile].tmem*8+(i-(int)ult)*descriptor[tile].line*8+(j-(int)uls*size)^S8]
		  = ((unsigned char*)tImg)[i*width*size+j^S8];
	     }
	}
}

void TX::loadTLUT(int tile, int count)
{
   for (int i=0; i<count*8; i++)
     tmem[descriptor[tile].tmem*8+i] = ((unsigned char*)tImg)[i];
}

void TX::setTileSize(float uls, float ult, float lrs, float lrt, int tile)
{
   descriptor[tile].uls = uls;
   descriptor[tile].ult = ult;
   descriptor[tile].lrs = lrs;
   descriptor[tile].lrt = lrt;
}

Color32 TX::unpack_RGBA16(int tile, int s, int t)
{
   if(!translateCoordinates(s,t, tile)) return Color32(0,0,0,0);
   short *p = (short*)tmem;
   int c = p[descriptor[tile].tmem*4+t*descriptor[tile].line*4+s^S16];
   Color32 color(((c>>11)&0x1F)<<3, ((c>>6)&0x1F)<<3, ((c>>1)&0x1F)<<3, (c&1) != 0 ? 0xFF : 0);
   return color;
}

Color32 TX::unpack_CI8_RGBA16(int tile, int s, int t)
{
   if(!translateCoordinates(s,t, tile)) return Color32(0,0,0,0);
   unsigned char *p = (unsigned char*)tmem;
   short *p16 = (short*)tmem;
   int i = p[descriptor[tile].tmem*8+t*descriptor[tile].line*8+s^S8];
   int c = p16[256*4 + i^S16];
   Color32 color(((c>>11)&0x1F)<<3, ((c>>6)&0x1F)<<3, ((c>>1)&0x1F)<<3, (c&1) != 0 ? 0xFF : 0);
   return color;
}

Color32 TX::unpack_IA16(int tile, int s, int t)
{
   if(!translateCoordinates(s,t, tile)) return Color32(0,0,0,0);
   short *p = (short*)tmem;
   int c = p[descriptor[tile].tmem*4+t*descriptor[tile].line*4+s^S16];
   Color32 color((c>>8)&0xFF, (c>>8)&0xFF, (c>>8)&0xFF, c & 0xFF);
   return color;
}

Color32 TX::unpack_IA8(int tile, int s, int t)
{
   if(!translateCoordinates(s,t, tile)) return Color32(0,0,0,0);
   unsigned char *p = (unsigned char*)tmem;
   int c = p[descriptor[tile].tmem*8+t*descriptor[tile].line*8+s^S8];
   Color32 color(((c>>4)&0xF)<<4, ((c>>4)&0xF)<<4, ((c>>4)&0xF)<<4, (c & 0xF)<<4);
   return color;
}

Color32 TX::unpack_IA4(int tile, int s, int t)
{
   if(!translateCoordinates(s,t, tile)) return Color32(0,0,0,0);
   unsigned char *p = (unsigned char*)tmem;
   int c;
   if (s&1)
     c = p[descriptor[tile].tmem*8+t*descriptor[tile].line*8+(s/2)^S8] & 0xF;
   else
     c = p[descriptor[tile].tmem*8+t*descriptor[tile].line*8+(s/2)^S8] >> 4;
   Color32 color(c<<4, c<<4, c<<4, (c&1) ? 0xFF : 0x00);
   return color;
}

bool TX::translateCoordinates(int &s, int &t, int tile)
{
   if (textureLOD || textureDetail)
     printf("TX:getTexel:textureLUT=%d,textureLOD=%d,textureDetail=%d\n",
	    textureLUT, textureLOD, textureDetail);
   if (descriptor[tile].shifts || descriptor[tile].shiftt)
     printf("tx:getTexel:shifts=%d,shiftt=%d\n",
	    descriptor[tile].shifts, descriptor[tile].shiftt);
   
   int w = (int)(descriptor[tile].lrs) - (int)(descriptor[tile].uls);
   int h = (int)(descriptor[tile].lrt) - (int)(descriptor[tile].ult);
   
   bool invertedS = false, invertedT = false;
   
   if (descriptor[tile].cms & 2)
     {
	if(s < 0) s = 0;
	if(s >= w) s = w-1;
     }
   if (descriptor[tile].cmt & 2)
     {
	if(t < 0) t = 0;
	if(t >= h) t = h-1;
     }
   
   if (descriptor[tile].cms & 1 && s & (1<<descriptor[tile].masks)) invertedS = true;
   if (descriptor[tile].cmt & 1 && t & (1<<descriptor[tile].maskt)) invertedT = true;
   if (descriptor[tile].masks) s &= (1<<descriptor[tile].masks)-1;
   if (descriptor[tile].maskt) t &= (1<<descriptor[tile].maskt)-1;
   if (invertedS) s = w - s;
   if (invertedT) t = h - t;
   
   if ( s<0 || t<0 || s>w || t>h)
     {
	//printf("TX: out of texture read ?\n");
	return false;
     }
   return true;
}

Color32 TX::getTexel(float _s, float _t, int tile, TF* tf)
{
   if (tf && tf->getTextureConvert() != 6)
     printf("TX:textureConvert=%x\n", tf->getTextureConvert());
   float s = _s - descriptor[tile].uls;
   float t = _t - descriptor[tile].ult;
   
   if(unpackTexel[tile] == NULL) return Color32(0,0,0,0);

   if ((_s - floorf(_s)) == 0.0f && (_t - floorf(_t)) == 0.0f)
     {
	return (this->*unpackTexel[tile])(tile, (int)s, (int)t);
     }
   else
     {
	Color32 nearestTexels[4];
	float nearestTexelsDistances[4];
	
	nearestTexels[0] = (this->*unpackTexel[tile])(tile, (int)s, (int)t);
	nearestTexelsDistances[0] = (s - (int)s)*(s - (int)s) + (t - (int)t)*(t - (int)t);
	nearestTexels[1] = (this->*unpackTexel[tile])(tile, (int)(s+1), (int)t);
	nearestTexelsDistances[1] = ((int)(s+1) - s)*((int)(s+1) - s) + (t - (int)t)*(t - (int)t);
	nearestTexels[2] = (this->*unpackTexel[tile])(tile, (int)(s+1), (int)(t+1));
	nearestTexelsDistances[2] = ((int)(s+1) - s)*((int)(s+1) - s) + ((int)(t+1) - t)*((int)(t+1) - t);
	nearestTexels[3] = (this->*unpackTexel[tile])(tile, (int)s, (int)(t+1));
	nearestTexelsDistances[3] = (s - (int)s)*(s - (int)s) + ((int)(t+1) - t)*((int)(t+1) - t);
	return tf->filter(nearestTexels, nearestTexelsDistances);
     }
   return 0;
}
