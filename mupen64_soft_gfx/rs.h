/**
 * Mupen64 - rs.h
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

#ifndef RS_H
#define RS_H

#include "color.h"
#include "vektor.h"

class RDP;

class RS
{
   // scissor
   float sulx, suly, slrx, slry;
   int mode;
   
 public:
   RS();
   ~RS();
   
   void setScissor(float ulx, float uly, float lrx, float lry, int mode);
   void fillRect(float ulx, float uly, float lrx, float lry, RDP *rdp);
   void texRect(int tile, float ulx, float uly, float lrx, float lry, float s, float t, float dsdx, float dtdy, RDP *rdp);
   void debug_tri(Vektor<float,4>& v0, Vektor<float,4>& v1, Vektor<float,4>& v2, RDP *rdp);
   void tri_shade_zbuff(Vektor<float,4>& v0, Vektor<float,4>& v1, Vektor<float,4>& v2, 
			Color32& c0, Color32& c1, Color32& c2, float z0, float z1, float z2, RDP *rdp);
   void tri_shade_txtr_zbuff(Vektor<float,4>& v0, Vektor<float,4>& v1, Vektor<float,4>& v2, 
			     Color32& c0, Color32& c1, Color32& c2,
			     float s0, float t0, float s1, float t1, float s2, float t2, int tile,
			     float w0, float w1, float w2, float z0, float z1, float z2, RDP *rdp);
   void tri_shade_txtr(Vektor<float,4>& v0, Vektor<float,4>& v1, Vektor<float,4>& v2, 
		       Color32& c0, Color32& c1, Color32& c2,
		       float s0, float t0, float s1, float t1, float s2, float t2, int tile, float w0, float w1, float w2, RDP *rdp);
   void tri_shade(Vektor<float,4>& v0, Vektor<float,4>& v1, Vektor<float,4>& v2, 
		  Color32& c0, Color32& c1, Color32& c2, RDP *rdp);
};

#include "rdp.h"

#endif // RS_H
