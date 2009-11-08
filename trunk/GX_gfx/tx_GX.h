/**
 * Wii64 - tx_GX.h
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

#ifndef TX_H
#define TX_H

#include "../main/winlnxdefs.h"
#include "Gfx_#1.3.h"
#include "color.h"
//#include "tf.h"
#include <gccore.h>

class TX;

typedef struct
{
   u8 GXtexfmt;
   int N64texfmt;
   int format;
   int size;
   int line;
   int tmem;
   int palette;
   int cmt;
   int maskt;
   int shiftt;
   int cms;
   int masks;
   int shifts;
   float uls;
   float ult;
   float lrs;
   float lrt;
} Descriptor;

class TX
{
   GFX_INFO gfxInfo;
   
   int textureLUT;
   int textureLOD;
   int textureDetail;
   int texturePersp;
   
   // texture img
   void *tImg;
   int format;
   int size;
   int width;
   int tile_width;

   bool new_load_block;
   int currentSetTile;
   
   // GX variables
   GXTexObj	GXtex;
   GXTlutObj GXtlut;
   //TODO: This can be a little smaller or maybe dynamically allocated
//    u16 GXtexture[512*8*2] ATTRIBUTE_ALIGN (32); //size of tmem*2 + tmem*2
//    u16 GXtextureCI[256] ATTRIBUTE_ALIGN (32); //assume max 256 16b colors
    u16* GXtexture; //size of tmem*2 + tmem*2
    u16* GXtextureCI; //assume max 256 16b colors

   Descriptor descriptor[8];
//   unsigned char tmem[512*8];
   
/*   Color32 (TX::*unpackTexel[8])(int tile, int s, int t);
   Color32 unpack_RGBA16(int tile, int s, int t);
   Color32 unpack_CI8_RGBA16(int tile, int s, int t);
   Color32 unpack_IA16(int tile, int s, int t);
   Color32 unpack_IA8(int tile, int s, int t);
   Color32 unpack_IA4(int tile, int s, int t);
   
   bool translateCoordinates(int &s, int &t, int tile);
*/   
 public:
   TX(GFX_INFO);
   ~TX();
   
   void setTextureLUT(int value);
   void setTextureLOD(int value);
   void setTextureDetail(int value);
   void setTexturePersp(int value);
   void setTImg(int format, int size, int width, void *timg);
   void setTile(int format, int size, int line, int tmem, int tile, int palette,
		int cmt, int maskt, int shiftt, int cms, int masks, int shifts);
   void loadBlock(float uls, float ult, int tile, float lrs, int dxt);
   void setTileSize(float uls, float ult, float lrs, float lrt, int tile);
   void loadTLUT(int tile, int count);
   void loadTile(int tile, float uls, float ult, float lrs, float lrt);
   
//   Color32 getTexel(float s, float t, int tile, TF* tf);
};

#endif // TX_H
