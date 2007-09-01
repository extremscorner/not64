/**
 * Mupen64 - rdp.h
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

#ifndef RDP_H
#define RDP_H

#include "../main/winlnxdefs.h"
#include "Gfx_#1.3.h"
#include "tx.h"
#include "rs.h"
#include "tf.h"
#include "cc.h"
#include "bl.h"

class RDP
{
   GFX_INFO gfxInfo;
   TX *tx;
   RS *rs;
   TF *tf;
   CC *cc;
   BL *bl;
   
   int cycleType;
   
 public:
   RDP(GFX_INFO);
   ~RDP();
   
   void setOtherMode_h(int mode, int data);
   void setOtherMode_l(int mode, int data);
   void setScissor(float ulx, float uly, float lrx, float lry, int mode);
   void setCombineMode(int cycle1, int cycle2);
   void setCImg(int format, int size, int width, void *cimg);
   void setTImg(int format, int size, int width, void *timg);
   void setZImg(void *zimg);
   void setFillColor(int color);
   void setFogColor(int color);
   void setBlendColor(int color);
   void setEnvColor(int color);
   void setPrimColor(int color, float mLOD, float lLOD);
   void fillRect(float ulx, float uly, float lrx, float lry);
   void setTile(int format, int size, int line, int tmem, int tile, int palette, 
		int cmt, int maskt, int shiftt, int cms, int masks, int shifts);
   void loadBlock(float uls, float ult, int tile, float lrs, int dxt);
   void setTileSize(float uls, float ult, float lrs, float lrt, int tile);
   void texRect(int tile, float ulx, float uly, float lrx, float lry, float s, float t, float dsdx, float dtdy);
   void loadTLUT(int tile, int count);
   void loadTile(int tile, float uls, float ult, float lrs, float lrt);
   void debug_tri(Vector<float,4>& v0, Vector<float,4>& v1, Vector<float,4>& v2);   
   void tri_shade_zbuff(Vector<float,4>& v0, Vector<float,4>& v1, Vector<float,4>& v2,
			Color32& c0, Color32& c1, Color32& c2, float z0, float z1, float z2);
   void tri_shade_txtr_zbuff(Vector<float,4>& v0, Vector<float,4>& v1, Vector<float,4>& v2,
			     Color32& c0, Color32& c1, Color32& c2,
			     float s0, float t0, float s1, float t1, float s2, float t2, int tile, 
			     float w0, float w1, float w2, float z0, float z1, float z2);
   void tri_shade_txtr(Vector<float,4>& v0, Vector<float,4>& v1, Vector<float,4>& v2,
		       Color32& c0, Color32& c1, Color32& c2,
		       float s0, float t0, float s1, float t1, float s2, float t2, int tile, float w0, float w1, float w2);
   void tri_shade(Vector<float,4>& v0, Vector<float,4>& v1, Vector<float,4>& v2,
		  Color32& c0, Color32& c1, Color32& c2);
   
   // friend rasterizer functions
   friend void RS::fillRect(float ulx, float uly, float lrx, float lry, RDP* rdp);
   friend void RS::texRect(int tile, float ulx, float uly, float lrx, float lry, float s, float t, float dsdx, float dtdy, RDP* rdp);
   friend void RS::debug_tri(Vector<float,4>& v0, Vector<float,4>& v1, Vector<float,4>& v2, RDP* rdp);
   friend void RS::tri_shade_zbuff(Vector<float,4>& v0, Vector<float,4>& v1, Vector<float,4>& v2,
				   Color32& c0, Color32& c1, Color32& c2, float z0, float z1, float z2, RDP* rdp);
   friend void RS::tri_shade_txtr_zbuff(Vector<float,4>& v0, Vector<float,4>& v1, Vector<float,4>& v2,
					Color32& c0, Color32& c1, Color32& c2,
					float s0, float t0, float s1, float t1, float s2, float t2, int tile,
					float w0, float w1, float w2, float z0, float z1, float z2, RDP* rdp);
   friend void RS::tri_shade_txtr(Vector<float,4>& v0, Vector<float,4>& v1, Vector<float,4>& v2,
				  Color32& c0, Color32& c1, Color32& c2,
				  float s0, float t0, float s1, float t1, float s2, float t2, int tile, float w0, float w1, float w2, RDP* rdp);
   friend void RS::tri_shade(Vector<float,4>& v0, Vector<float,4>& v1, Vector<float,4>& v2,
			     Color32& c0, Color32& c1, Color32& c2, RDP* rdp);
};

#endif // RDP_H
