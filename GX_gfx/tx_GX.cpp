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


#include <malloc.h>
#include <stdio.h>
#include <math.h>

#include "tx_GX.h"
#include "global.h"
#include "../gui/DEBUG.h"

TX::TX(GFX_INFO info) : gfxInfo(info)
{
	for(int i=0; i<8; i++) descriptor[i].N64texfmt = 0;
	GXtexture = (u16*) memalign(32,512*8*2*2); //size of tmem*2 + tmem*2
	GXtextureCI = (u16*) memalign(32,256*2); //assume max 256 16b colors
	new_load_block = false;
	currentSetTile = 0;
}

TX::~TX()
{
	free(GXtexture);
	free(GXtextureCI);
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
   
   currentSetTile = tile;

   descriptor[tile].N64texfmt = 0;

   switch(descriptor[tile].format)
     {
      case 0: // RGBA
	switch(descriptor[tile].size)
	  {
	   case 0: // doesn't exist
	     break;
	   case 2: // RGBA16
//	     unpackTexel[tile] = &TX::unpack_RGBA16;
		 descriptor[tile].N64texfmt = 1;
	     break;
	   default:
	   		sprintf(txtbuffer,"TX:unknown setTile RGBA size : %d", descriptor[tile].size);
	   		DEBUG_print(txtbuffer,DBG_TXINFO);
	  }
	break;
      case 2: // CI
	switch(descriptor[tile].size)
	  {
	   case 1: // CI8
	     if (textureLUT == 2)
//	       unpackTexel[tile] = &TX::unpack_CI8_RGBA16;
		   descriptor[tile].N64texfmt = 2;
	     else{
	     	sprintf(txtbuffer,"TX:unknown setTile CI8 LUT format:%d", textureLUT);
	   		DEBUG_print(txtbuffer,DBG_TXINFO); 
   		}
	     break;
	   default:
	   	  	sprintf(txtbuffer,"TX:unknown setTile CI size : %d", descriptor[tile].size);
	   		DEBUG_print(txtbuffer,DBG_TXINFO); 
	  }
	break;
      case 3: // IA
	switch(descriptor[tile].size)
	  {
	   case 0: // IA4
//	     unpackTexel[tile] = &TX::unpack_IA4;
		 descriptor[tile].N64texfmt = 3;
	     break;
	   case 1: // IA8
//	     unpackTexel[tile] = &TX::unpack_IA8;
		 descriptor[tile].N64texfmt = 4;
	     break;
	   case 2: // IA16
//	     unpackTexel[tile] = &TX::unpack_IA16;
		 descriptor[tile].N64texfmt = 5;
	     break;
	   default:
	   	 sprintf(txtbuffer,"TX:unknown setTile IA size : %d", descriptor[tile].size);
	   	 DEBUG_print(txtbuffer,DBG_TXINFO); 
	  }
	break;
      default:
      	 sprintf(txtbuffer,"TX:unknown setTile format : %d", descriptor[tile].format);
	   	 DEBUG_print(txtbuffer,DBG_TXINFO); 
     }
}

void TX::loadBlock(float uls, float ult, int tile, float lrs, int dxt)
{
   if ((int)uls != 0 || (int)ult != 0) DEBUG_print("tx:unknown loadBlock",DBG_TXINFO);
//   for (int i=0; i<((int)lrs+1)*8; i++)
//     tmem[descriptor[tile].tmem*8+i] = ((unsigned char*)tImg)[i];

   new_load_block = true;

   tile = 0; //May need to detect the latest from setTile
//   tile = currentSetTile;
   loadTile(tile, descriptor[tile].uls, descriptor[tile].ult, descriptor[tile].lrs, descriptor[tile].lrt); //problem: can't use width...
}

void TX::loadTile(int tile, float uls, float ult, float lrs, float lrt)
{
   u8 wrap_s, wrap_t;
   u16 c,txl;
   int numtiles_s,numtiles_t,i_s,i_t;
   int count = 0;
   unsigned char *p = (unsigned char*)tImg;
   short *p16 = (short*)tImg;
   if (!size) DEBUG_print("loadtile tries to load a 4 bit texture",DBG_TXINFO);
/*   for (int i=(int)ult; i<=(int)lrt; i++)
	{
	   for (int j=(int)uls*size; j<=(int)lrs*size; j++)
	     {
		tmem[descriptor[tile].tmem*8+(i-(int)ult)*descriptor[tile].line*8+(j-(int)uls*size)^S8]
		  = ((unsigned char*)tImg)[i*width*size+j^S8];
	     }
	}*/

   // Let GX finish with all previous commands before loading the new tex
	GX_DrawDone();

	if (new_load_block) 
	{
		tile_width = (int) lrs-uls+1;
//		tile = currentSetTile;
//		tile = 0;
		new_load_block = false;
	}
	else tile_width = width;

	if (descriptor[tile].cms & 2) wrap_s = GX_CLAMP;
	else if (descriptor[tile].cms & 1) wrap_s = GX_MIRROR;
	else wrap_s = GX_REPEAT;
	if (descriptor[tile].cmt & 2) wrap_t = GX_CLAMP;
	else if (descriptor[tile].cmt & 1) wrap_t = GX_MIRROR;
	else wrap_t = GX_REPEAT;
	//TODO: Check on s & t wrap modes and mipmap modes

//	if (descriptor[tile].N64texfmt != 2) printf("Unimplemented N64 tex: %d\n",descriptor[tile].N64texfmt);

	//Convert texture to GC format
	switch (descriptor[tile].N64texfmt)
   {
	   case 1: //N64 RGBA16 -> GC GX_TF_RGB5A3
		   numtiles_s = (int) ceil((lrs-uls+1)/4);
		   numtiles_t = (int) ceil((lrt-ult+1)/4);
		   for (int i=0;i<numtiles_t;i++)
		   {
			   for (int j=0;j<numtiles_s;j++)
			   {
				   for (int ii=0;ii<4;ii++) //fill 4x4 RGB5A3 tile to make 32B
				   {
					   i_t = (int)ult + i*4+ii;
					   for(int jj=0;jj<4;jj++) //1 N64 texel for 1 GX
					   {
						   i_s = (int)uls + j*4+jj;
						   if((i_s<(lrs+1))&&(i_t<(lrt+1))) c = p16[i_t*tile_width+i_s^S16];
						   else c = 0;
						   if ((c&1) != 0) GXtexture[count^S16] = 0x8000|(((c>>11)&0x1F)<<10)|(((c>>6)&0x1F)<<5)|((c>>1)&0x1F);   //opaque texel
						   else GXtexture[count^S16] = 0x0000|(((c>>12)&0xF)<<8)|(((c>>7)&0xF)<<4)|((c>>2)&0xF);   //transparent texel
						   count++;
					   }
				   }
			   }
		   }
		   descriptor[tile].GXtexfmt = GX_TF_RGB5A3;
		   break;
	   case 2: //N64 CI8_RGBA16 -> GC CI GX_TF_RGB5A3
		   numtiles_s = (int) ceil((lrs-uls+1)/8);
		   numtiles_t = (int) ceil((lrt-ult+1)/4);
		   for (int i=0;i<numtiles_t;i++)
		   {
			   for (int j=0;j<numtiles_s;j++)
			   {
				   for (int ii=0;ii<4;ii++) //fill 4x4 RGB5A3 tile to make 32B
				   {
					   i_t = (int)ult + i*4+ii;
					   for(int jj=0;jj<8;jj+=2) //2 N64 texels for 1 GX
					   {
						   i_s = (int)uls + j*8+jj;
						   if((i_s<(lrs+1))&&(i_t<(lrt+1))) c = p[i_t*tile_width+i_s^S8];
						   else c = 0;
						   txl = ((c&0xFF)<<8);
						   if((i_s+1<(lrs+1))&&(i_t<(lrt+1))) c = p[i_t*tile_width+(i_s+1)^S8];
						   else c = 0;
						   GXtexture[count^S16] = (u16) txl|(c&0xFF);   //2 u8 CI texels
						   count++;
					   }
				   }
			   }
		   }
		   descriptor[tile].GXtexfmt = GX_TF_CI8;
		   break;
	   case 3: //N64 IA4 -> GC GX_TF_IA4
		   numtiles_s = (int) ceil((lrs-uls+1)/8);
		   numtiles_t = (int) ceil((lrt-ult+1)/4);
		   for (int i=0;i<numtiles_t;i++)
		   {
			   for (int j=0;j<numtiles_s;j++)
			   {
				   for (int ii=0;ii<4;ii++) //fill 8x4 IA4 tile to make 32B
				   {
					   i_t = (int)ult + i*4+ii;
					   for(int jj=0;jj<8;jj+=2) //2 N64 texels for 1 GX
					   {
						   i_s = (int)uls + j*8+jj;
						   if((i_s<(lrs+1))&&(i_t<(lrt+1))) 
						   {
							   c = p[i_t*tile_width+(i_s/2)^S8]; // 2 texels: AB
							   if((i_s+1)<(lrs+1)) c = c & 0xF0;
						   }
						   else c = 0;
						   txl = ((c&0x10) != 0) ? (0xF0|((c&0xF0)>>4))<<8 : (0x00|((c&0xF0)>>4))<<8;
						   GXtexture[count^S16] = ((c&1) != 0) ? txl|(c&0x0F|0xF0) : txl|(c&0x0F|0x00);
						   count++;
					   }
				   }
			   }
		   }
		   descriptor[tile].GXtexfmt = GX_TF_IA4;
		   break;
	   case 4: //N64 IA8 -> GC GX_TF_IA4
		   numtiles_s = (int) ceil((lrs-uls+1)/8);
		   numtiles_t = (int) ceil((lrt-ult+1)/4);
		   for (int i=0;i<numtiles_t;i++)
		   {
			   for (int j=0;j<numtiles_s;j++)
			   {
				   for (int ii=0;ii<4;ii++) //fill 8x4 IA4 tile to make 32B
				   {
					   i_t = (int)ult + i*4+ii;
					   for(int jj=0;jj<8;jj+=2) //2 N64 texels for 1 GX
					   {
						   i_s = (int)uls + j*8+jj;
						   if((i_s<(lrs+1))&&(i_t<(lrt+1))) c = p[i_t*tile_width+i_s^S8]; 
						   else c = 0;
						   txl = (((c&0xF0)>>4)|((c&0x0F)<<4))<<8;
						   if((i_s+1<(lrs+1))&&(i_t<(lrt+1))) c = p[i_t*tile_width+(i_s+1)^S8]; 
						   else c = 0;
						   GXtexture[count^S16] = txl|((c&0xF0)>>4)|((c&0x0F)<<4);
						   count++;
					   }
				   }
			   }
		   }
		   descriptor[tile].GXtexfmt = GX_TF_IA4;
		   break;
	   case 5: //N64 IA16 -> GC GX_TF_IA8
		   numtiles_s = (int) ceil((lrs-uls+1)/4);
		   numtiles_t = (int) ceil((lrt-ult+1)/4);
		   for (int i=0;i<numtiles_t;i++)
		   {
			   for (int j=0;j<numtiles_s;j++)
			   {
				   for (int ii=0;ii<4;ii++) //fill 4x4 IA8 tile to make 32B
				   {
					   i_t = (int)ult + i*4+ii;
					   for(int jj=0;jj<4;jj++) //1 N64 texel for 1 GX
					   {
						   i_s = (int)uls + j*4+jj;
						   if((i_s<(lrs+1))&&(i_t<(lrt+1))) c = p16[i_t*tile_width+i_s^S16]; 
						   else c = 0;
						   GXtexture[count^S16] = ((c&0xFF00)>>8)|((c&0x00FF)<<8);
						   count++;
					   }
				   }
			   }
		   }
		   descriptor[tile].GXtexfmt = GX_TF_IA8;
		   break;
	   default:
		   DEBUG_print("loadTile: Unkown texture format. No texture loaded",DBG_TXINFO);
		   return;
   }

	u16	GXwidth = (u16) lrs-uls+1;
	u16 GXheight = (u16) lrt-ult+1;
//	if (textureLUT == 2) GX_InitTexObjCI(&GXtex, GXtexture, (u16) numtiles_s*8, (u16) numtiles_t*4, 
	if (textureLUT == 2) GX_InitTexObjCI(&GXtex, GXtexture, GXwidth, GXheight, 
							descriptor[tile].GXtexfmt, wrap_s, wrap_t, GX_FALSE, GX_TLUT0);
	else GX_InitTexObj(&GXtex, GXtexture, GXwidth, GXheight, 
							descriptor[tile].GXtexfmt, wrap_s, wrap_t, GX_FALSE); 
	DCFlushRange(GXtexture, 512*8*2*2);
	GX_InvalidateTexAll();
	GX_LoadTexObj(&GXtex, GX_TEXMAP0); // should set to (u8) tile or GX_TEXMAP0
}

void TX::loadTLUT(int tile, int count)
{
   int c;

   if(count>256) 
   {
	   sprintf(txtbuffer,"loadTLUT: Unkown TLUT format. count = %d",count);
	   DEBUG_print(txtbuffer,DBG_TXINFO); 
	   return;
   }
   // Let GX finish with all previous commands before loading the new tex
	GX_DrawDone();
//	GX_SetDrawDone();

	for(int i=0; i<count; i++)//N64 RGBA16 -> GC GX_TF_RGB5A3
   {
	   c = ((short*)tImg)[i];
	   if ((c&1) != 0) GXtextureCI[i] = 0x8000|(((c>>11)&0x1F)<<10)|(((c>>6)&0x1F)<<5)|((c>>1)&0x1F);   //opaque texel
	   else GXtextureCI[i] = 0x0000|(((c>>12)&0xF)<<8)|(((c>>7)&0xF)<<4)|((c>>2)&0xF);   //transparent texel
   }
   //GX_TL_RGB5A3 == 0x02?
   //GX_TL_RGB565 == 0x01? 0x05?
   GX_InitTlutObj(&GXtlut, GXtextureCI,(u8) 0x02,(u16) count/16); //GX_TL_RGB5A3 is missing in gx.h
   DCFlushRange(GXtextureCI, 256*2);
//   GX_WaitDrawDone();
   GX_InvalidateTexAll();
   GX_LoadTlut(&GXtlut, GX_TLUT0);	// use GX_TLUT0 or (u32) tile??
}

void TX::setTileSize(float uls, float ult, float lrs, float lrt, int tile)
{
   descriptor[tile].uls = uls;
   descriptor[tile].ult = ult;
   descriptor[tile].lrs = lrs;
   descriptor[tile].lrt = lrt;
}
