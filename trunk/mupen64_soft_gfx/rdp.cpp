/**
 * Mupen64 - rdp.cpp
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

#include "rdp.h"

RDP::RDP(GFX_INFO info) : gfxInfo(info)
{
   tx = new TX(info);
   rs = new RS();
   tf = new TF();
   cc = new CC();
   bl = new BL(info);
}

RDP::~RDP()
{
   delete tx;
   delete rs;
   delete tf;
   delete cc;
   delete bl;
}

void RDP::setOtherMode_l(int mode, int data)
{
   switch(mode)
     {
      case 0:
	bl->setAlphaCompare(data);
	break;
      case 2:
	bl->setDepthSource(data);
	break;
      case 3:
	bl->setBlender(data<<3);
	break;
      default:
	printf("RDP: unknown setOtherMode_l:%d\n", mode);
     }
}

void RDP::setOtherMode_h(int mode, int data)
{
   switch(mode)
     {
      case 4:
	bl->setAlphaDither(data);
	break;
      case 6:
	bl->setColorDither(data);
	break;
      case 8:
	cc->setCombineKey(data);
	break;
      case 9:
	tf->setTextureConvert(data);
	break;
      case 12:
	tf->setTextureFilter(data);
	break;
      case 14:
	tx->setTextureLUT(data);
	break;
      case 16:
	tx->setTextureLOD(data);
	break;
      case 17:
	tx->setTextureDetail(data);
	break;
      case 19:
	tx->setTexturePersp(data);
	break;
      case 20:
	cycleType = data;
	break;
      case 23:
	// ignoring pipeline mode
	break;
      default:
	printf("RDP: unknown setOtherMode_h:%d\n", mode);
     }
}

void RDP::setScissor(float ulx, float uly, float lrx, float lry, int mode)
{
   rs->setScissor(ulx, uly, lrx, lry, mode);
}

void RDP::setCombineMode(int cycle1, int cycle2)
{
   cc->setCombineMode(cycle1, cycle2);
}

void RDP::setCImg(int format, int size, int width, void *cimg)
{
   bl->setCImg(format, size, width, cimg);
}

void RDP::setTImg(int format, int size, int width, void *timg)
{
   tx->setTImg(format, size, width, timg);
}

void RDP::setZImg(void *zimg)
{
   bl->setZImg(zimg);
}

void RDP::setFillColor(int color)
{
   bl->setFillColor(color);
}

void RDP::setFogColor(int color)
{
   bl->setFogColor(color);
}

void RDP::setBlendColor(int color)
{
   bl->setBlendColor(color);
}

void RDP::setEnvColor(int color)
{
   cc->setEnvColor(color);
}

void RDP::setPrimColor(int color, float mLOD, float lLOD)
{
   cc->setPrimColor(color, mLOD, lLOD);
}

void RDP::fillRect(float ulx, float uly, float lrx, float lry)
{
   rs->fillRect(ulx, uly, lrx, lry, this);
}

void RDP::setTile(int format, int size, int line, int tmem, int tile, int palette,
		  int cmt, int maskt, int shiftt, int cms, int masks, int shifts)
{
   tx->setTile(format, size, line, tmem, tile, palette, cmt, maskt, shiftt, cms, masks, shifts);
}

void RDP::loadBlock(float uls, float ult, int tile, float lrs, int dxt)
{
   tx->loadBlock(uls, ult, tile, lrs, dxt);
}

void RDP::setTileSize(float uls, float ult, float lrs, float lrt, int tile)
{
   tx->setTileSize(uls, ult, lrs, lrt, tile);
}

void RDP::texRect(int tile, float ulx, float uly, float lrx, float lry, float s, float t, float dsdx, float dtdy)
{
   rs->texRect(tile, ulx, uly, lrx, lry, s, t, dsdx, dtdy, this);
}

void RDP::loadTLUT(int tile, int count)
{
   tx->loadTLUT(tile, count);
}

void RDP::loadTile(int tile, float uls, float ult, float lrs, float lrt)
{
   tx->loadTile(tile, uls, ult, lrs, lrt);
}

void RDP::debug_tri(Vektor<float,4>& v0, Vektor<float,4>& v1, Vektor<float,4>& v2)
{
   rs->debug_tri(v0, v1, v2, this);
}

void RDP::tri_shade_zbuff(Vektor<float,4>& v0, Vektor<float,4>& v1, Vektor<float,4>& v2,
			  Color32& c0, Color32& c1, Color32& c2, float z0, float z1, float z2)
{
   rs->tri_shade_zbuff(v0, v1, v2, c0, c1, c2, z0, z1, z2, this);
}

void RDP::tri_shade_txtr_zbuff(Vektor<float,4>& v0, Vektor<float,4>& v1, Vektor<float,4>& v2,
			       Color32& c0, Color32& c1, Color32& c2, 
			       float s0, float t0, float s1, float t1, float s2, float t2, int tile,
			       float w0, float w1, float w2, float z0, float z1, float z2)
{
   rs->tri_shade_txtr_zbuff(v0, v1, v2, c0, c1, c2, s0, t0, s1, t1, s2, t2, tile, w0, w1, w2, z0, z1, z2, this);
}

void RDP::tri_shade_txtr(Vektor<float,4>& v0, Vektor<float,4>& v1, Vektor<float,4>& v2,
			 Color32& c0, Color32& c1, Color32& c2, 
			 float s0, float t0, float s1, float t1, float s2, float t2, int tile, float w0, float w1, float w2)
{
   rs->tri_shade_txtr(v0, v1, v2, c0, c1, c2, s0, t0, s1, t1, s2, t2, tile, w0, w1, w2, this);
}

void RDP::tri_shade(Vektor<float,4>& v0, Vektor<float,4>& v1, Vektor<float,4>& v2,
		    Color32& c0, Color32& c1, Color32& c2)
{
   rs->tri_shade(v0, v1, v2, c0, c1, c2, this);
}
