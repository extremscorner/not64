/**
 * Mupen64 - rs.cpp
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

#include "rs.h"

RS::RS() : mode(0)
{
}

RS::~RS()
{
}

void RS::setScissor(float ux, float uy, float lx, float ly, int m)
{
   sulx = ux;
   suly = uy;
   slrx = lx;
   slry = ly;
   mode = m;
}

void RS::fillRect(float ux, float uy, float lx, float ly, RDP *rdp)
{
   int ulx, uly, lrx, lry;
   ulx = sulx < ux ? (int)ux : (int)sulx;
   uly = suly < uy ? (int)uy : (int)suly;
   lrx = slrx > lx ? (int)lx : (int)slrx;
   lry = slry > ly ? (int)ly : (int)slry;
   if (rdp->cycleType == 3) 
     {
	lrx++;
	lry++;
	
	for (int i=uly; i<lry; i++)
	  for (int j=ulx; j<lrx; j+=2)
	    rdp->bl->fillModeDraw(j,i);
     }
   else if (rdp->cycleType == 0)
     {
	for (int i=uly; i<lry; i++)
	  {
	     for (int j=ulx; j<lrx; j++)
	       {
		  Color32 c = rdp->cc->combine1(0);
		  rdp->bl->cycle1ModeDraw(j,i,c);
	       }
	  }
     }
   else printf("rs:fillRect not fill mode ? %d\n", rdp->cycleType);
}

void RS::texRect(int tile, float ux, float uy, float lx, float ly, float s, float t, float dsdx, float dtdy, RDP *rdp)
{
   if (rdp->cycleType == 0) // 1 cycle mode
     {
	int ulx, uly, lrx, lry;
	ulx = sulx < ux ? (int)ux : (int)sulx;
	uly = suly < uy ? (int)uy : (int)suly;
	lrx = slrx > lx ? (int)lx : (int)slrx;
	lry = slry > ly ? (int)ly : (int)slry;
	if (sulx > ux) s += (ux - sulx) * dsdx;
	if (suly > uy) t += (uy - suly) * dtdy;
	float ps = s;
	float pt = t;
	for (int i=uly; i<lry; i++,pt+=dtdy)
	  {
	     ps = s;
	     for (int j=ulx; j<lrx; j++,ps+=dsdx)
	       {
		  Color32 t = rdp->tx->getTexel(ps, pt, tile, rdp->tf);
		  Color32 c = rdp->cc->combine1(t);
		  rdp->bl->cycle1ModeDraw(j,i,c);
	       }
	  }
     }
   else if (rdp->cycleType == 2) // copy mode
     {
	int ulx, uly, lrx, lry;
	ulx = sulx < ux ? (int)ux : (int)sulx;
	uly = suly < uy ? (int)uy : (int)suly;
	lrx = slrx > lx ? (int)(lx+1) : (int)slrx;
	lry = slry > ly ? (int)(ly+1) : (int)slry;
	if (sulx > ux) s += (ux - sulx) * dsdx;
	if (suly > uy) t += (uy - suly) * dtdy;
	float ps = s;
	float pt = t;
	float pdsdx = dsdx;
	pdsdx = pdsdx / 4;
	for (int i=uly; i<lry; i++,pt+=dtdy)
	  {
	     ps = s;
	     
	     for (int j=ulx; j<lrx; j++,ps+=pdsdx)
	       {
		  Color32 t = rdp->tx->getTexel(ps, pt, tile, NULL);
		  rdp->bl->copyModeDraw(j,i,t);
	       }
	  }
     }
   else
     printf("RS:unknown cycle type in texRect:%d\n", rdp->cycleType);
}

void RS::tri_shade_txtr_zbuff(Vektor<float,4>& v0, Vektor<float,4>& v1, Vektor<float,4>& v2, 
			      Color32& c0, Color32& c1, Color32& c2, 
			      float s0, float t0, float s1, float t1, float s2, float t2, int tile,
			      float w0, float w1, float w2, float z0, float z1, float z2, RDP *rdp)
{
   if (rdp->cycleType == 0)
     {
	// sorting vertex by y values
	Vektor<float,4>* v[3] = { &v0, &v1, &v2 };
	Color32* c[3];
	float* z[3];
	float* s[3];
	float* t[3];
	float* w[3];
	Vektor<float,4> *temp;
	if ((*v[0])[1] > (*v[1])[1]) { temp = v[0]; v[0] = v[1]; v[1] = temp; }
	if ((*v[1])[1] > (*v[2])[1]) { temp = v[1]; v[1] = v[2]; v[2] = temp; }
	if ((*v[0])[1] > (*v[1])[1]) { temp = v[0]; v[0] = v[1]; v[1] = temp; }
	if ((*v[1])[1] > (*v[2])[1]) { temp = v[1]; v[1] = v[2]; v[2] = temp; }
	for (int i=0; i<3; i++)
	  {
	     if (v[i] == &v0) { c[i] = &c0; z[i] = &z0; s[i] = &s0; t[i] = &t0; w[i] = &w0; }
	     if (v[i] == &v1) { c[i] = &c1; z[i] = &z1; s[i] = &s1; t[i] = &t1; w[i] = &w1; }
	     if (v[i] == &v2) { c[i] = &c2; z[i] = &z2; s[i] = &s2; t[i] = &t2; w[i] = &w2; }
	  }
	
	// initiate coordinates :
	float dx1 = 0.0f, dx2 = 0.0f;
	float lim1 = (*v[0])[0];
	float lim2 = lim1;
	if ((*v[1])[1] != (*v[0])[1]) dx1 = ((*v[1])[0] - lim1) / ((*v[1])[1] - (*v[0])[1]);
	if ((*v[2])[1] != (*v[0])[1]) dx2 = ((*v[2])[0] - lim2) / ((*v[2])[1] - (*v[0])[1]);
	// initiate shade :
       	float limR1, limG1, limB1, limA1, limR2, limG2, limB2, limA2;
	float dR1 = 0.0f, dG1 = 0.0f, dB1 = 0.0f, dA1 = 0.0f, dR2 = 0.0f, dG2 = 0.0f, dB2 = 0.0f, dA2 = 0.0f;
	limR1 = limR2 = c[0]->getR();
	limG1 = limG2 = c[0]->getG();
	limB1 = limB2 = c[0]->getB();
	limA1 = limA2 = c[0]->getAlpha();
	if ((*v[1])[1] != (*v[0])[1]) dR1 = (c[1]->getR() - limR1) / ((*v[1])[1] - (*v[0])[1]);
	if ((*v[2])[1] != (*v[0])[1]) dR2 = (c[2]->getR() - limR2) / ((*v[2])[1] - (*v[0])[1]);
	if ((*v[1])[1] != (*v[0])[1]) dG1 = (c[1]->getG() - limG1) / ((*v[1])[1] - (*v[0])[1]);
	if ((*v[2])[1] != (*v[0])[1]) dG2 = (c[2]->getG() - limG2) / ((*v[2])[1] - (*v[0])[1]);
	if ((*v[1])[1] != (*v[0])[1]) dB1 = (c[1]->getB() - limB1) / ((*v[1])[1] - (*v[0])[1]);
	if ((*v[2])[1] != (*v[0])[1]) dB2 = (c[2]->getB() - limB2) / ((*v[2])[1] - (*v[0])[1]);
	if ((*v[1])[1] != (*v[0])[1]) dA1 = (c[1]->getAlpha() - limA1) / ((*v[1])[1] - (*v[0])[1]);
	if ((*v[2])[1] != (*v[0])[1]) dA2 = (c[2]->getAlpha() - limA2) / ((*v[2])[1] - (*v[0])[1]);
	// initiate z :
	float limZ1, limZ2, dZ1 = 0.0f, dZ2 = 0.0f;
	limZ1 = limZ2 = *z[0];
	if ((*v[1])[1] != (*v[0])[1]) dZ1 = (*z[1] - limZ1) / ((*v[1])[1] - (*v[0])[1]);
	if ((*v[2])[1] != (*v[0])[1]) dZ2 = (*z[2] - limZ2) / ((*v[2])[1] - (*v[0])[1]);
	// initiate s and t :
	s0 /= w0; t0 /= w0;
	s1 /= w1; t1 /= w1;
	s2 /= w2; t2 /= w2;
	float limS1, limT1, limS2, limT2, dS1 = 0.0f, dT1 = 0.0f, dS2 = 0.0f, dT2 = 0.0f;
	limS1 = limS2 = *s[0];
	limT1 = limT2 = *t[0];
	if ((*v[1])[1] != (*v[0])[1]) dS1 = (*s[1] - limS1) / ((*v[1])[1] - (*v[0])[1]);
	if ((*v[2])[1] != (*v[0])[1]) dS2 = (*s[2] - limS2) / ((*v[2])[1] - (*v[0])[1]);
	if ((*v[1])[1] != (*v[0])[1]) dT1 = (*t[1] - limT1) / ((*v[1])[1] - (*v[0])[1]);
	if ((*v[2])[1] != (*v[0])[1]) dT2 = (*t[2] - limT2) / ((*v[2])[1] - (*v[0])[1]);
	// initiate w :
	w0 = 1.0f / w0;
	w1 = 1.0f / w1;
	w2 = 1.0f / w2;
	float limW1, limW2, dW1 = 0.0f, dW2 = 0.0f;
	limW1 = limW2 = *w[0];
	if ((*v[1])[1] != (*v[0])[1]) dW1 = (*w[1] - limW1) / ((*v[1])[1] - (*v[0])[1]);
	if ((*v[2])[1] != (*v[0])[1]) dW2 = (*w[2] - limW2) / ((*v[2])[1] - (*v[0])[1]);
	
	// first half of the triangle
	int y;
	for (y = (int)((*v[0])[1]+0.5f); y < (int)((*v[1])[1]+0.5f); y++, lim1+=dx1, lim2+=dx2, 
	     limR1+=dR1, limR2+=dR2, limG1+=dG1, limG2+=dG2, limB1+=dB1, limB2+=dB2, limA1+=dA1, limA2+=dA2,
	     limZ1+=dZ1, limZ2+=dZ2, limS1+=dS1, limS2+=dS2, limT1+=dT1, limT2+=dT2, limW1+=dW1, limW2+=dW2)
	  {
	     if (y >= suly && y < slry)
	       {
		  int min, max;
		  float r, g, b, a, dr = 0.0f, dg = 0.0f, db = 0.0f, da = 0.0f;
		  float z, dz = 0.0f;
		  float s, t, ds = 0.0f, dt = 0.0f;
		  float w, dw = 0.0f;
		  if (lim1 < lim2)
		    {
		       min = (int)(lim1+0.5f);
		       max = (int)(lim2+0.5f);
		       r = limR1;
		       g = limG1;
		       b = limB1;
		       a = limA1;
		       if (lim1 != lim2) dr = (limR2 - limR1) / (lim2 - lim1);
		       if (lim1 != lim2) dg = (limG2 - limG1) / (lim2 - lim1);
		       if (lim1 != lim2) db = (limB2 - limB1) / (lim2 - lim1);
		       if (lim1 != lim2) da = (limA2 - limA1) / (lim2 - lim1);
		       z = limZ1;
		       if (lim1 != lim2) dz = (limZ2 - limZ1) / (lim2 - lim1);
		       s = limS1;
		       t = limT1;
		       if (lim1 != lim2) ds = (limS2 - limS1) / (lim2 - lim1);
		       if (lim1 != lim2) dt = (limT2 - limT1) / (lim2 - lim1);
		       w = limW1;
		       if (lim1 != lim2) dw = (limW2 - limW1) / (lim2 - lim1);
		    }
		  else
		    {
		       min = (int)(lim2+0.5f);
		       max = (int)(lim1+0.5f);
		       r = limR2;
		       g = limG2;
		       b = limB2;
		       a = limA2;
		       if (lim1 != lim2) dr = (limR1 - limR2) / (lim1 - lim2);
		       if (lim1 != lim2) dg = (limG1 - limG2) / (lim1 - lim2);
		       if (lim1 != lim2) db = (limB1 - limB2) / (lim1 - lim2);
		       if (lim1 != lim2) da = (limA1 - limA2) / (lim1 - lim2);
		       z = limZ2;
		       if (lim1 != lim2) dz = (limZ1 - limZ2) / (lim1 - lim2);
		       s = limS2;
		       t = limT2;
		       if (lim1 != lim2) ds = (limS1 - limS2) / (lim1 - lim2);
		       if (lim1 != lim2) dt = (limT1 - limT2) / (lim1 - lim2);
		       w = limW2;
		       if (lim1 != lim2) dw = (limW1 - limW2) / (lim1 - lim2);
		    }
		  for (int x=min; x<max; x++, r+=dr, g+=dg, b+=db, a+=da, z+=dz, s+=ds, t+=dt, w+=dw)
		    {
		       if (x >= sulx && x < slrx)
			 {
			    Color32 shade((int)(r+0.5f), (int)(g+0.5f), (int)(b+0.5f), (int)(a+0.5f));
			    rdp->cc->setShade(shade);
			    Color32 txt= rdp->tx->getTexel(s/w, t/w, tile, rdp->tf);
			    Color32 c  = rdp->cc->combine1(txt);
			    rdp->bl->cycle1ModeDraw(x,y,c,z,shade);
			 }
		    }
	       }
	  }
	
	// initiate coordinates 2 :
	lim1 = (*v[1])[0];
	if ((*v[2])[1] != (*v[1])[1]) dx1 = ((*v[2])[0] - lim1) / ((*v[2])[1] - (*v[1])[1]);
	// initiate shade 2 :
	limR1 = c[1]->getR();
	limG1 = c[1]->getG();
	limB1 = c[1]->getB();
	limA1 = c[1]->getAlpha();
	if ((*v[2])[1] != (*v[1])[1]) dR1 = (c[2]->getR() - limR1) / ((*v[2])[1] - (*v[1])[1]);
	if ((*v[2])[1] != (*v[1])[1]) dG1 = (c[2]->getG() - limG1) / ((*v[2])[1] - (*v[1])[1]);
	if ((*v[2])[1] != (*v[1])[1]) dB1 = (c[2]->getB() - limB1) / ((*v[2])[1] - (*v[1])[1]);
	if ((*v[2])[1] != (*v[1])[1]) dA1 = (c[2]->getAlpha() - limA1) / ((*v[2])[1] - (*v[1])[1]);
	// initiate z 2 :
	limZ1 = *z[1];
	if ((*v[2])[1] != (*v[1])[1]) dZ1 = (*z[2] - limZ1) / ((*v[2])[1] - (*v[1])[1]);
	// initiate s and t 2:
	limS1 = *s[1];
	limT1 = *t[1];
	if ((*v[2])[1] != (*v[1])[1]) dS1 = (*s[2] - limS1) / ((*v[2])[1] - (*v[1])[1]);
	if ((*v[2])[1] != (*v[1])[1]) dT1 = (*t[2] - limT1) / ((*v[2])[1] - (*v[1])[1]);
	// initiate w 2 :
	limW1 = *w[1];
	if ((*v[2])[1] != (*v[1])[1]) dW1 = (*w[2] - limW1) / ((*v[2])[1] - (*v[1])[1]);
	
	// second half of the triangle
	for (; y < (int)((*v[2])[1]+0.5f); y++, lim1=lim1+dx1, lim2=lim2+dx2,
	     limR1+=dR1, limR2+=dR2, limG1+=dG1, limG2+=dG2, limB1+=dB1, limB2+=dB2, limA1+=dA1, limA2+=dA2,
	     limZ1+=dZ1, limZ2+=dZ2, limS1+=dS1, limS2+=dS2, limT1+=dT1, limT2+=dT2, limW1+=dW1, limW2+=dW2)
	  {
	     if (y >= suly && y < slry)
	       {
		  int min, max;
		  float r, g, b, a, dr = 0.0f, dg = 0.0f, db = 0.0f, da = 0.0f;
		  float z, dz = 0.0f;
		  float s, t, ds = 0.0f, dt = 0.0f;
		  float w, dw = 0.0f;
		  if (lim1 < lim2)
		    {
		       min = (int)(lim1+0.5f);
		       max = (int)(lim2+0.5f);
		       r = limR1;
		       g = limG1;
		       b = limB1;
		       a = limA1;
		       if (lim1 != lim2) dr = (limR2 - limR1) / (lim2 - lim1);
		       if (lim1 != lim2) dg = (limG2 - limG1) / (lim2 - lim1);
		       if (lim1 != lim2) db = (limB2 - limB1) / (lim2 - lim1);
		       if (lim1 != lim2) da = (limA2 - limA1) / (lim2 - lim1);
		       z = limZ1;
		       if (lim1 != lim2) dz = (limZ2 - limZ1) / (lim2 - lim1);
		       s = limS1;
		       t = limT1;
		       if (lim1 != lim2) ds = (limS2 - limS1) / (lim2 - lim1);
		       if (lim1 != lim2) dt = (limT2 - limT1) / (lim2 - lim1);
		       w = limW1;
		       if (lim1 != lim2) dw = (limW2 - limW1) / (lim2 - lim1);
		    }
		  else
		    {
		       min = (int)(lim2+0.5f);
		       max = (int)(lim1+0.5f);
		       r = limR2;
		       g = limG2;
		       b = limB2;
		       a = limA2;
		       if (lim1 != lim2) dr = (limR1 - limR2) / (lim1 - lim2);
		       if (lim1 != lim2) dg = (limG1 - limG2) / (lim1 - lim2);
		       if (lim1 != lim2) db = (limB1 - limB2) / (lim1 - lim2);
		       if (lim1 != lim2) da = (limA1 - limA2) / (lim1 - lim2);
		       z = limZ2;
		       if (lim1 != lim2) dz = (limZ1 - limZ2) / (lim1 - lim2);
		       s = limS2;
		       t = limT2;
		       if (lim1 != lim2) ds = (limS1 - limS2) / (lim1 - lim2);
		       if (lim1 != lim2) dt = (limT1 - limT2) / (lim1 - lim2);
		       w = limW2;
		       if (lim1 != lim2) dw = (limW1 - limW2) / (lim1 - lim2);
		    }
		  for (int x=min; x<max; x++, r+=dr, g+=dg, b+=db, a+=da, z+=dz, s+=ds, t+=dt, w+=dw)
		    {
		       if (x >= sulx && x < slrx)
			 {
			    Color32 shade((int)(r+0.5f), (int)(g+0.5f), (int)(b+0.5f), (int)(a+0.5f));
			    rdp->cc->setShade(shade);
			    Color32 txt = rdp->tx->getTexel(s/w, t/w, tile, rdp->tf);
			    Color32 c = rdp->cc->combine1(txt);
			    rdp->bl->cycle1ModeDraw(x,y,c,z,shade);
			 }
		    }
	       }
	  }
     }
   else if (rdp->cycleType == 1)
     {
	// sorting vertex by y values
	Vektor<float,4>* v[3] = { &v0, &v1, &v2 };
	Color32* c[3];
	float* z[3];
	float* s[3];
	float* t[3];
	float* w[3];
	Vektor<float,4> *temp;
	if ((*v[0])[1] > (*v[1])[1]) { temp = v[0]; v[0] = v[1]; v[1] = temp; }
	if ((*v[1])[1] > (*v[2])[1]) { temp = v[1]; v[1] = v[2]; v[2] = temp; }
	if ((*v[0])[1] > (*v[1])[1]) { temp = v[0]; v[0] = v[1]; v[1] = temp; }
	if ((*v[1])[1] > (*v[2])[1]) { temp = v[1]; v[1] = v[2]; v[2] = temp; }
	for (int i=0; i<3; i++)
	  {
	     if (v[i] == &v0) { c[i] = &c0; z[i] = &z0; s[i] = &s0; t[i] = &t0; w[i] = &w0; }
	     if (v[i] == &v1) { c[i] = &c1; z[i] = &z1; s[i] = &s1; t[i] = &t1; w[i] = &w1; }
	     if (v[i] == &v2) { c[i] = &c2; z[i] = &z2; s[i] = &s2; t[i] = &t2; w[i] = &w2; }
	  }
	
	// initiate coordinates :
	float dx1 = 0.0f, dx2 = 0.0f;
	float lim1 = (*v[0])[0];
	float lim2 = lim1;
	if ((*v[1])[1] != (*v[0])[1]) dx1 = ((*v[1])[0] - lim1) / ((*v[1])[1] - (*v[0])[1]);
	if ((*v[2])[1] != (*v[0])[1]) dx2 = ((*v[2])[0] - lim2) / ((*v[2])[1] - (*v[0])[1]);
	// initiate shade :
       	float limR1, limG1, limB1, limA1, limR2, limG2, limB2, limA2;
	float dR1 = 0.0f, dG1 = 0.0f, dB1 = 0.0f, dA1 = 0.0f, dR2 = 0.0f, dG2 = 0.0f, dB2 = 0.0f, dA2 = 0.0f;
	limR1 = limR2 = c[0]->getR();
	limG1 = limG2 = c[0]->getG();
	limB1 = limB2 = c[0]->getB();
	limA1 = limA2 = c[0]->getAlpha();
	if ((*v[1])[1] != (*v[0])[1]) dR1 = (c[1]->getR() - limR1) / ((*v[1])[1] - (*v[0])[1]);
	if ((*v[2])[1] != (*v[0])[1]) dR2 = (c[2]->getR() - limR2) / ((*v[2])[1] - (*v[0])[1]);
	if ((*v[1])[1] != (*v[0])[1]) dG1 = (c[1]->getG() - limG1) / ((*v[1])[1] - (*v[0])[1]);
	if ((*v[2])[1] != (*v[0])[1]) dG2 = (c[2]->getG() - limG2) / ((*v[2])[1] - (*v[0])[1]);
	if ((*v[1])[1] != (*v[0])[1]) dB1 = (c[1]->getB() - limB1) / ((*v[1])[1] - (*v[0])[1]);
	if ((*v[2])[1] != (*v[0])[1]) dB2 = (c[2]->getB() - limB2) / ((*v[2])[1] - (*v[0])[1]);
	if ((*v[1])[1] != (*v[0])[1]) dA1 = (c[1]->getAlpha() - limA1) / ((*v[1])[1] - (*v[0])[1]);
	if ((*v[2])[1] != (*v[0])[1]) dA2 = (c[2]->getAlpha() - limA2) / ((*v[2])[1] - (*v[0])[1]);
	// initiate z :
	float limZ1, limZ2, dZ1 = 0.0f, dZ2 = 0.0f;
	limZ1 = limZ2 = *z[0];
	if ((*v[1])[1] != (*v[0])[1]) dZ1 = (*z[1] - limZ1) / ((*v[1])[1] - (*v[0])[1]);
	if ((*v[2])[1] != (*v[0])[1]) dZ2 = (*z[2] - limZ2) / ((*v[2])[1] - (*v[0])[1]);
	// initiate s and t :
	s0 /= w0; t0 /= w0;
	s1 /= w1; t1 /= w1;
	s2 /= w2; t2 /= w2;
	float limS1, limT1, limS2, limT2, dS1 = 0.0f, dT1 = 0.0f, dS2 = 0.0f, dT2 = 0.0f;
	limS1 = limS2 = *s[0];
	limT1 = limT2 = *t[0];
	if ((*v[1])[1] != (*v[0])[1]) dS1 = (*s[1] - limS1) / ((*v[1])[1] - (*v[0])[1]);
	if ((*v[2])[1] != (*v[0])[1]) dS2 = (*s[2] - limS2) / ((*v[2])[1] - (*v[0])[1]);
	if ((*v[1])[1] != (*v[0])[1]) dT1 = (*t[1] - limT1) / ((*v[1])[1] - (*v[0])[1]);
	if ((*v[2])[1] != (*v[0])[1]) dT2 = (*t[2] - limT2) / ((*v[2])[1] - (*v[0])[1]);
	// initiate w :
	w0 = 1.0f / w0;
	w1 = 1.0f / w1;
	w2 = 1.0f / w2;
	float limW1, limW2, dW1 = 0.0f, dW2 = 0.0f;
	limW1 = limW2 = *w[0];
	if ((*v[1])[1] != (*v[0])[1]) dW1 = (*w[1] - limW1) / ((*v[1])[1] - (*v[0])[1]);
	if ((*v[2])[1] != (*v[0])[1]) dW2 = (*w[2] - limW2) / ((*v[2])[1] - (*v[0])[1]);
	
	// first half of the triangle
	int y;
	for (y = (int)((*v[0])[1]+0.5f); y < (int)((*v[1])[1]+0.5f); y++, lim1+=dx1, lim2+=dx2, 
	     limR1+=dR1, limR2+=dR2, limG1+=dG1, limG2+=dG2, limB1+=dB1, limB2+=dB2, limA1+=dA1, limA2+=dA2,
	     limZ1+=dZ1, limZ2+=dZ2, limS1+=dS1, limS2+=dS2, limT1+=dT1, limT2+=dT2, limW1+=dW1, limW2+=dW2)
	  {
	     if (y >= suly && y < slry)
	       {
		  int min, max;
		  float r, g, b, a, dr = 0.0f, dg = 0.0f, db = 0.0f, da = 0.0f;
		  float z, dz = 0.0f;
		  float s, t, ds = 0.0f, dt = 0.0f;
		  float w, dw = 0.0f;
		  if (lim1 < lim2)
		    {
		       min = (int)(lim1+0.5f);
		       max = (int)(lim2+0.5f);
		       r = limR1;
		       g = limG1;
		       b = limB1;
		       a = limA1;
		       if (lim1 != lim2) dr = (limR2 - limR1) / (lim2 - lim1);
		       if (lim1 != lim2) dg = (limG2 - limG1) / (lim2 - lim1);
		       if (lim1 != lim2) db = (limB2 - limB1) / (lim2 - lim1);
		       if (lim1 != lim2) da = (limA2 - limA1) / (lim2 - lim1);
		       z = limZ1;
		       if (lim1 != lim2) dz = (limZ2 - limZ1) / (lim2 - lim1);
		       s = limS1;
		       t = limT1;
		       if (lim1 != lim2) ds = (limS2 - limS1) / (lim2 - lim1);
		       if (lim1 != lim2) dt = (limT2 - limT1) / (lim2 - lim1);
		       w = limW1;
		       if (lim1 != lim2) dw = (limW2 - limW1) / (lim2 - lim1);
		    }
		  else
		    {
		       min = (int)(lim2+0.5f);
		       max = (int)(lim1+0.5f);
		       r = limR2;
		       g = limG2;
		       b = limB2;
		       a = limA2;
		       if (lim1 != lim2) dr = (limR1 - limR2) / (lim1 - lim2);
		       if (lim1 != lim2) dg = (limG1 - limG2) / (lim1 - lim2);
		       if (lim1 != lim2) db = (limB1 - limB2) / (lim1 - lim2);
		       if (lim1 != lim2) da = (limA1 - limA2) / (lim1 - lim2);
		       z = limZ2;
		       if (lim1 != lim2) dz = (limZ1 - limZ2) / (lim1 - lim2);
		       s = limS2;
		       t = limT2;
		       if (lim1 != lim2) ds = (limS1 - limS2) / (lim1 - lim2);
		       if (lim1 != lim2) dt = (limT1 - limT2) / (lim1 - lim2);
		       w = limW2;
		       if (lim1 != lim2) dw = (limW1 - limW2) / (lim1 - lim2);
		    }
		  for (int x=min; x<max; x++, r+=dr, g+=dg, b+=db, a+=da, z+=dz, s+=ds, t+=dt, w+=dw)
		    {
		       if (x >= sulx && x < slrx)
			 {
			    Color32 shade((int)(r+0.5f), (int)(g+0.5f), (int)(b+0.5f), (int)(a+0.5f));
			    rdp->cc->setShade(shade);
			    Color32 txt= rdp->tx->getTexel(s/w, t/w, tile, rdp->tf);
			    Color32 txt2=rdp->tx->getTexel(s/w, t/w, tile+1,rdp->tf);
			    Color32 c  = rdp->cc->combine2(txt, txt2);
			    rdp->bl->cycle2ModeDraw(x,y,c,z,shade);
			 }
		    }
	       }
	  }
	
	// initiate coordinates 2 :
	lim1 = (*v[1])[0];
	if ((*v[2])[1] != (*v[1])[1]) dx1 = ((*v[2])[0] - lim1) / ((*v[2])[1] - (*v[1])[1]);
	// initiate shade 2 :
	limR1 = c[1]->getR();
	limG1 = c[1]->getG();
	limB1 = c[1]->getB();
	limA1 = c[1]->getAlpha();
	if ((*v[2])[1] != (*v[1])[1]) dR1 = (c[2]->getR() - limR1) / ((*v[2])[1] - (*v[1])[1]);
	if ((*v[2])[1] != (*v[1])[1]) dG1 = (c[2]->getG() - limG1) / ((*v[2])[1] - (*v[1])[1]);
	if ((*v[2])[1] != (*v[1])[1]) dB1 = (c[2]->getB() - limB1) / ((*v[2])[1] - (*v[1])[1]);
	if ((*v[2])[1] != (*v[1])[1]) dA1 = (c[2]->getAlpha() - limA1) / ((*v[2])[1] - (*v[1])[1]);
	// initiate z 2 :
	limZ1 = *z[1];
	if ((*v[2])[1] != (*v[1])[1]) dZ1 = (*z[2] - limZ1) / ((*v[2])[1] - (*v[1])[1]);
	// initiate s and t 2:
	limS1 = *s[1];
	limT1 = *t[1];
	if ((*v[2])[1] != (*v[1])[1]) dS1 = (*s[2] - limS1) / ((*v[2])[1] - (*v[1])[1]);
	if ((*v[2])[1] != (*v[1])[1]) dT1 = (*t[2] - limT1) / ((*v[2])[1] - (*v[1])[1]);
	// initiate w 2 :
	limW1 = *w[1];
	if ((*v[2])[1] != (*v[1])[1]) dW1 = (*w[2] - limW1) / ((*v[2])[1] - (*v[1])[1]);
	
	// second half of the triangle
	for (; y < (int)((*v[2])[1]+0.5f); y++, lim1=lim1+dx1, lim2=lim2+dx2,
	     limR1+=dR1, limR2+=dR2, limG1+=dG1, limG2+=dG2, limB1+=dB1, limB2+=dB2, limA1+=dA1, limA2+=dA2,
	     limZ1+=dZ1, limZ2+=dZ2, limS1+=dS1, limS2+=dS2, limT1+=dT1, limT2+=dT2, limW1+=dW1, limW2+=dW2)
	  {
	     if (y >= suly && y < slry)
	       {
		  int min, max;
		  float r, g, b, a, dr = 0.0f, dg = 0.0f, db = 0.0f, da = 0.0f;
		  float z, dz = 0.0f;
		  float s, t, ds = 0.0f, dt = 0.0f;
		  float w, dw = 0.0f;
		  if (lim1 < lim2)
		    {
		       min = (int)(lim1+0.5f);
		       max = (int)(lim2+0.5f);
		       r = limR1;
		       g = limG1;
		       b = limB1;
		       a = limA1;
		       if (lim1 != lim2) dr = (limR2 - limR1) / (lim2 - lim1);
		       if (lim1 != lim2) dg = (limG2 - limG1) / (lim2 - lim1);
		       if (lim1 != lim2) db = (limB2 - limB1) / (lim2 - lim1);
		       if (lim1 != lim2) da = (limA2 - limA1) / (lim2 - lim1);
		       z = limZ1;
		       if (lim1 != lim2) dz = (limZ2 - limZ1) / (lim2 - lim1);
		       s = limS1;
		       t = limT1;
		       if (lim1 != lim2) ds = (limS2 - limS1) / (lim2 - lim1);
		       if (lim1 != lim2) dt = (limT2 - limT1) / (lim2 - lim1);
		       w = limW1;
		       if (lim1 != lim2) dw = (limW2 - limW1) / (lim2 - lim1);
		    }
		  else
		    {
		       min = (int)(lim2+0.5f);
		       max = (int)(lim1+0.5f);
		       r = limR2;
		       g = limG2;
		       b = limB2;
		       a = limA2;
		       if (lim1 != lim2) dr = (limR1 - limR2) / (lim1 - lim2);
		       if (lim1 != lim2) dg = (limG1 - limG2) / (lim1 - lim2);
		       if (lim1 != lim2) db = (limB1 - limB2) / (lim1 - lim2);
		       if (lim1 != lim2) da = (limA1 - limA2) / (lim1 - lim2);
		       z = limZ2;
		       if (lim1 != lim2) dz = (limZ1 - limZ2) / (lim1 - lim2);
		       s = limS2;
		       t = limT2;
		       if (lim1 != lim2) ds = (limS1 - limS2) / (lim1 - lim2);
		       if (lim1 != lim2) dt = (limT1 - limT2) / (lim1 - lim2);
		       w = limW2;
		       if (lim1 != lim2) dw = (limW1 - limW2) / (lim1 - lim2);
		    }
		  for (int x=min; x<max; x++, r+=dr, g+=dg, b+=db, a+=da, z+=dz, s+=ds, t+=dt, w+=dw)
		    {
		       if (x >= sulx && x < slrx)
			 {
			    Color32 shade((int)(r+0.5f), (int)(g+0.5f), (int)(b+0.5f), (int)(a+0.5f));
			    rdp->cc->setShade(shade);
			    Color32 txt= rdp->tx->getTexel(s/w, t/w, tile, rdp->tf);
			    Color32 txt2=rdp->tx->getTexel(s/w, t/w, tile+1,rdp->tf);
			    Color32 c  = rdp->cc->combine2(txt, txt2);
			    rdp->bl->cycle2ModeDraw(x,y,c,z,shade);
			 }
		    }
	       }
	  }
     }
   else
     printf("RS:unknown cycle type in tri_shade_txtr_zbuff:%d\n", rdp->cycleType);
}

void RS::tri_shade_zbuff(Vektor<float,4>& v0, Vektor<float,4>& v1, Vektor<float,4>& v2, 
			 Color32& c0, Color32& c1, Color32& c2, float z0, float z1, float z2, RDP *rdp)
{
   if (rdp->cycleType == 0)
     {
	// sorting vertex by y values
	Vektor<float,4>* v[3] = { &v0, &v1, &v2 };
	Color32* c[3];
	float* z[3];
	Vektor<float,4> *temp;
	if ((*v[0])[1] > (*v[1])[1]) { temp = v[0]; v[0] = v[1]; v[1] = temp; }
	if ((*v[1])[1] > (*v[2])[1]) { temp = v[1]; v[1] = v[2]; v[2] = temp; }
	if ((*v[0])[1] > (*v[1])[1]) { temp = v[0]; v[0] = v[1]; v[1] = temp; }
	if ((*v[1])[1] > (*v[2])[1]) { temp = v[1]; v[1] = v[2]; v[2] = temp; }
	for (int i=0; i<3; i++)
	  {
	     if (v[i] == &v0) { c[i] = &c0; z[i] = &z0; }
	     if (v[i] == &v1) { c[i] = &c1; z[i] = &z1; }
	     if (v[i] == &v2) { c[i] = &c2; z[i] = &z2; }
	  }
	
	// initiate coordinates :
	float lim1, lim2, dx1 = 0.0f, dx2 = 0.0f;
	lim1 = (*v[0])[0];
	lim2 = lim1;
	if ((*v[1])[1] != (*v[0])[1]) dx1 = ((*v[1])[0] - lim1) / ((*v[1])[1] - (*v[0])[1]);
	if ((*v[2])[1] != (*v[0])[1]) dx2 = ((*v[2])[0] - lim2) / ((*v[2])[1] - (*v[0])[1]);
	// initiate shade :
       	float limR1, limG1, limB1, limA1, limR2, limG2, limB2, limA2;
	float dR1 = 0.0f, dG1 = 0.0f, dB1 = 0.0f, dA1 = 0.0f, dR2 = 0.0f, dG2 = 0.0f, dB2 = 0.0f, dA2 = 0.0f;
	limR1 = limR2 = c[0]->getR();
	limG1 = limG2 = c[0]->getG();
	limB1 = limB2 = c[0]->getB();
	limA1 = limA2 = c[0]->getAlpha();
	if ((*v[1])[1] != (*v[0])[1]) dR1 = (c[1]->getR() - limR1) / ((*v[1])[1] - (*v[0])[1]);
	if ((*v[2])[1] != (*v[0])[1]) dR2 = (c[2]->getR() - limR2) / ((*v[2])[1] - (*v[0])[1]);
	if ((*v[1])[1] != (*v[0])[1]) dG1 = (c[1]->getG() - limG1) / ((*v[1])[1] - (*v[0])[1]);
	if ((*v[2])[1] != (*v[0])[1]) dG2 = (c[2]->getG() - limG2) / ((*v[2])[1] - (*v[0])[1]);
	if ((*v[1])[1] != (*v[0])[1]) dB1 = (c[1]->getB() - limB1) / ((*v[1])[1] - (*v[0])[1]);
	if ((*v[2])[1] != (*v[0])[1]) dB2 = (c[2]->getB() - limB2) / ((*v[2])[1] - (*v[0])[1]);
	if ((*v[1])[1] != (*v[0])[1]) dA1 = (c[1]->getAlpha() - limA1) / ((*v[1])[1] - (*v[0])[1]);
	if ((*v[2])[1] != (*v[0])[1]) dA2 = (c[2]->getAlpha() - limA2) / ((*v[2])[1] - (*v[0])[1]);
	// initiate z :
	float limZ1, limZ2, dZ1 = 0.0f, dZ2 = 0.0f;
	limZ1 = limZ2 = *z[0];
	if ((*v[1])[1] != (*v[0])[1]) dZ1 = (*z[1] - limZ1) / ((*v[1])[1] - (*v[0])[1]);
	if ((*v[2])[1] != (*v[0])[1]) dZ2 = (*z[2] - limZ2) / ((*v[2])[1] - (*v[0])[1]);
	
	// first half of the triangle
	int y;
	for (y = (int)((*v[0])[1]+0.5f); y < (int)((*v[1])[1]+0.5f); y++, lim1+=dx1, lim2+=dx2, 
	     limR1+=dR1, limR2+=dR2, limG1+=dG1, limG2+=dG2, limB1+=dB1, limB2+=dB2, limA1+=dA1, limA2+=dA2,
	     limZ1+=dZ1, limZ2+=dZ2)
	  {
	     if (y >= suly && y < slry)
	       {
		  int min, max;
		  float r, g, b, a, dr = 0.0f, dg = 0.0f, db = 0.0f, da = 0.0f;
		  float z, dz = 0.0f;
		  if (lim1 < lim2)
		    {
		       min = (int)(lim1+0.5f);
		       max = (int)(lim2+0.5f);
		       r = limR1;
		       g = limG1;
		       b = limB1;
		       a = limA1;
		       if (lim1 != lim2) dr = (limR2 - limR1) / (lim2 - lim1);
		       if (lim1 != lim2) dg = (limG2 - limG1) / (lim2 - lim1);
		       if (lim1 != lim2) db = (limB2 - limB1) / (lim2 - lim1);
		       if (lim1 != lim2) da = (limA2 - limA1) / (lim2 - lim1);
		       z = limZ1;
		       if (lim1 != lim2) dz = (limZ2 - limZ1) / (lim2 - lim1);
		    }
		  else
		    {
		       min = (int)(lim2+0.5f);
		       max = (int)(lim1+0.5f);
		       r = limR2;
		       g = limG2;
		       b = limB2;
		       a = limA2;
		       if (lim1 != lim2) dr = (limR1 - limR2) / (lim1 - lim2);
		       if (lim1 != lim2) dg = (limG1 - limG2) / (lim1 - lim2);
		       if (lim1 != lim2) db = (limB1 - limB2) / (lim1 - lim2);
		       if (lim1 != lim2) da = (limA1 - limA2) / (lim1 - lim2);
		       z = limZ2;
		       if (lim1 != lim2) dz = (limZ1 - limZ2) / (lim1 - lim2);
		    }
		  for (int x=min; x<max; x++, r+=dr, g+=dg, b+=db, a+=da, z+=dz)
		    {
		       if (x >= sulx && x < slrx)
			 {
			    Color32 shade((int)(r+0.5f), (int)(g+0.5f), (int)(b+0.5f), (int)(a+0.5f));
			    rdp->cc->setShade(shade);
			    Color32 t = 0;
			    Color32 c = rdp->cc->combine1(t);
			    rdp->bl->cycle1ModeDraw(x,y,c,z,shade);
			 }
		    }
	       }
	  }
	
	// initiate coordinates 2 :
	lim1 = (*v[1])[0];
	if ((*v[2])[1] != (*v[1])[1]) dx1 = ((*v[2])[0] - lim1) / ((*v[2])[1] - (*v[1])[1]);
	// initiate shade 2 :
	limR1 = c[1]->getR();
	limG1 = c[1]->getG();
	limB1 = c[1]->getB();
	limA1 = c[1]->getAlpha();
	if ((*v[2])[1] != (*v[1])[1]) dR1 = (c[2]->getR() - limR1) / ((*v[2])[1] - (*v[1])[1]);
	if ((*v[2])[1] != (*v[1])[1]) dG1 = (c[2]->getG() - limG1) / ((*v[2])[1] - (*v[1])[1]);
	if ((*v[2])[1] != (*v[1])[1]) dB1 = (c[2]->getB() - limB1) / ((*v[2])[1] - (*v[1])[1]);
	if ((*v[2])[1] != (*v[1])[1]) dA1 = (c[2]->getAlpha() - limA1) / ((*v[2])[1] - (*v[1])[1]);
	// initiate z 2 :
	limZ1 = *z[1];
	if ((*v[2])[1] != (*v[1])[1]) dZ1 = (*z[2] - limZ1) / ((*v[2])[1] - (*v[1])[1]);
	
	// second half of the triangle
	for (; y < (int)((*v[2])[1]+0.5f); y++, lim1=lim1+dx1, lim2=lim2+dx2,
	     limR1+=dR1, limR2+=dR2, limG1+=dG1, limG2+=dG2, limB1+=dB1, limB2+=dB2, limA1+=dA1, limA2+=dA2,
	     limZ1+=dZ1, limZ2+=dZ2)
	  {
	     if (y >= suly && y < slry)
	       {
		  int min, max;
		  float r, g, b, a, dr = 0.0f, dg = 0.0f, db = 0.0f, da = 0.0f;
		  float z, dz = 0.0f;
		  if (lim1 < lim2)
		    {
		       min = (int)(lim1+0.5f);
		       max = (int)(lim2+0.5f);
		       r = limR1;
		       g = limG1;
		       b = limB1;
		       a = limA1;
		       if (lim1 != lim2) dr = (limR2 - limR1) / (lim2 - lim1);
		       if (lim1 != lim2) dg = (limG2 - limG1) / (lim2 - lim1);
		       if (lim1 != lim2) db = (limB2 - limB1) / (lim2 - lim1);
		       if (lim1 != lim2) da = (limA2 - limA1) / (lim2 - lim1);
		       z = limZ1;
		       if (lim1 != lim2) dz = (limZ2 - limZ1) / (lim2 - lim1);
		    }
		  else
		    {
		       min = (int)(lim2+0.5f);
		       max = (int)(lim1+0.5f);
		       r = limR2;
		       g = limG2;
		       b = limB2;
		       a = limA2;
		       if (lim1 != lim2) dr = (limR1 - limR2) / (lim1 - lim2);
		       if (lim1 != lim2) dg = (limG1 - limG2) / (lim1 - lim2);
		       if (lim1 != lim2) db = (limB1 - limB2) / (lim1 - lim2);
		       if (lim1 != lim2) da = (limA1 - limA2) / (lim1 - lim2);
		       z = limZ2;
		       if (lim1 != lim2) dz = (limZ1 - limZ2) / (lim1 - lim2);
		    }
		  for (int x=min; x<max; x++, r+=dr, g+=dg, b+=db, a+=da, z+=dz)
		    {
		       if (x >= sulx && x < slrx)
			 {
			    Color32 shade((int)(r+0.5f), (int)(g+0.5f), (int)(b+0.5f), (int)(a+0.5f));
			    rdp->cc->setShade(shade);
			    Color32 t = 0;
			    Color32 c = rdp->cc->combine1(t);
			    rdp->bl->cycle1ModeDraw(x,y,c,z,shade);
			 }
		    }
	       }
	  }
     }
   else
     printf("RS:unknown cycle type in tri_shade_zbuff:%d\n", rdp->cycleType);
}

void RS::tri_shade_txtr(Vektor<float,4>& v0, Vektor<float,4>& v1, Vektor<float,4>& v2, 
			Color32& c0, Color32& c1, Color32& c2, 
			float s0, float t0, float s1, float t1, float s2, float t2, int tile, float w0, float w1, float w2, RDP *rdp)
{
   if (rdp->cycleType == 0)
     {
	// sorting vertex by y values
	Vektor<float,4>* v[3] = { &v0, &v1, &v2 };
	Color32* c[3];
	float* s[3];
	float* t[3];
	float* w[3];
	Vektor<float,4> *temp;
	if ((*v[0])[1] > (*v[1])[1]) { temp = v[0]; v[0] = v[1]; v[1] = temp; }
	if ((*v[1])[1] > (*v[2])[1]) { temp = v[1]; v[1] = v[2]; v[2] = temp; }
	if ((*v[0])[1] > (*v[1])[1]) { temp = v[0]; v[0] = v[1]; v[1] = temp; }
	if ((*v[1])[1] > (*v[2])[1]) { temp = v[1]; v[1] = v[2]; v[2] = temp; }
	for (int i=0; i<3; i++)
	  {
	     if (v[i] == &v0) { c[i] = &c0; s[i] = &s0; t[i] = &t0; w[i] = &w0; }
	     if (v[i] == &v1) { c[i] = &c1; s[i] = &s1; t[i] = &t1; w[i] = &w1; }
	     if (v[i] == &v2) { c[i] = &c2; s[i] = &s2; t[i] = &t2; w[i] = &w2; }
	  }
	
	// initiate coordinates :
	float lim1, lim2, dx1 = 0.0f, dx2 = 0.0f;
	lim1 = (*v[0])[0];
	lim2 = lim1;
	if ((*v[1])[1] != (*v[0])[1]) dx1 = ((*v[1])[0] - lim1) / ((*v[1])[1] - (*v[0])[1]);
	if ((*v[2])[1] != (*v[0])[1]) dx2 = ((*v[2])[0] - lim2) / ((*v[2])[1] - (*v[0])[1]);
	// initiate shade :
       	float limR1, limG1, limB1, limA1, limR2, limG2, limB2, limA2;
	float dR1 = 0.0f, dG1 = 0.0f, dB1 = 0.0f, dA1 = 0.0f, dR2 = 0.0f, dG2 = 0.0f, dB2 = 0.0f, dA2 = 0.0f;
	limR1 = limR2 = c[0]->getR();
	limG1 = limG2 = c[0]->getG();
	limB1 = limB2 = c[0]->getB();
	limA1 = limA2 = c[0]->getAlpha();
	if ((*v[1])[1] != (*v[0])[1]) dR1 = (c[1]->getR() - limR1) / ((*v[1])[1] - (*v[0])[1]);
	if ((*v[2])[1] != (*v[0])[1]) dR2 = (c[2]->getR() - limR2) / ((*v[2])[1] - (*v[0])[1]);
	if ((*v[1])[1] != (*v[0])[1]) dG1 = (c[1]->getG() - limG1) / ((*v[1])[1] - (*v[0])[1]);
	if ((*v[2])[1] != (*v[0])[1]) dG2 = (c[2]->getG() - limG2) / ((*v[2])[1] - (*v[0])[1]);
	if ((*v[1])[1] != (*v[0])[1]) dB1 = (c[1]->getB() - limB1) / ((*v[1])[1] - (*v[0])[1]);
	if ((*v[2])[1] != (*v[0])[1]) dB2 = (c[2]->getB() - limB2) / ((*v[2])[1] - (*v[0])[1]);
	if ((*v[1])[1] != (*v[0])[1]) dA1 = (c[1]->getAlpha() - limA1) / ((*v[1])[1] - (*v[0])[1]);
	if ((*v[2])[1] != (*v[0])[1]) dA2 = (c[2]->getAlpha() - limA2) / ((*v[2])[1] - (*v[0])[1]);
	// initiate s and t :
	s0 /= w0; t0 /= w0;
	s1 /= w1; t1 /= w1;
	s2 /= w2; t2 /= w2;
	float limS1, limT1, limS2, limT2, dS1 = 0.0f, dT1 = 0.0f, dS2 = 0.0f, dT2 = 0.0f;
	limS1 = limS2 = *s[0];
	limT1 = limT2 = *t[0];
	if ((*v[1])[1] != (*v[0])[1]) dS1 = (*s[1] - limS1) / ((*v[1])[1] - (*v[0])[1]);
	if ((*v[2])[1] != (*v[0])[1]) dS2 = (*s[2] - limS2) / ((*v[2])[1] - (*v[0])[1]);
	if ((*v[1])[1] != (*v[0])[1]) dT1 = (*t[1] - limT1) / ((*v[1])[1] - (*v[0])[1]);
	if ((*v[2])[1] != (*v[0])[1]) dT2 = (*t[2] - limT2) / ((*v[2])[1] - (*v[0])[1]);
	// initiate w :
	w0 = 1.0f / w0;
	w1 = 1.0f / w1;
	w2 = 1.0f / w2;
	float limW1, limW2, dW1 = 0.0f, dW2 = 0.0f;
	limW1 = limW2 = *w[0];
	if ((*v[1])[1] != (*v[0])[1]) dW1 = (*w[1] - limW1) / ((*v[1])[1] - (*v[0])[1]);
	if ((*v[2])[1] != (*v[0])[1]) dW2 = (*w[2] - limW2) / ((*v[2])[1] - (*v[0])[1]);
	
	// first half of the triangle
	int y;
	for (y = (int)((*v[0])[1]+0.5f); y < (int)((*v[1])[1]+0.5f); y++, lim1+=dx1, lim2+=dx2, 
	     limR1+=dR1, limR2+=dR2, limG1+=dG1, limG2+=dG2, limB1+=dB1, limB2+=dB2, limA1+=dA1, limA2+=dA2,
	     limS1+=dS1, limS2+=dS2, limT1+=dT1, limT2+=dT2, limW1+=dW1, limW2+=dW2)
	  {
	     if (y >= suly && y < slry)
	       {
		  int min, max;
		  float r, g, b, a, dr = 0.0f, dg = 0.0f, db = 0.0f, da = 0.0f;
		  float s, t, ds = 0.0f, dt = 0.0f;
		  float w, dw = 0.0f;
		  if (lim1 < lim2)
		    {
		       min = (int)(lim1+0.5f);
		       max = (int)(lim2+0.5f);
		       r = limR1;
		       g = limG1;
		       b = limB1;
		       a = limA1;
		       if (lim1 != lim2) dr = (limR2 - limR1) / (lim2 - lim1);
		       if (lim1 != lim2) dg = (limG2 - limG1) / (lim2 - lim1);
		       if (lim1 != lim2) db = (limB2 - limB1) / (lim2 - lim1);
		       if (lim1 != lim2) da = (limA2 - limA1) / (lim2 - lim1);
		       s = limS1;
		       t = limT1;
		       if (lim1 != lim2) ds = (limS2 - limS1) / (lim2 - lim1);
		       if (lim1 != lim2) dt = (limT2 - limT1) / (lim2 - lim1);
		       w = limW1;
		       if (lim1 != lim2) dw = (limW2 - limW1) / (lim2 - lim1);
		    }
		  else
		    {
		       min = (int)(lim2+0.5f);
		       max = (int)(lim1+0.5f);
		       r = limR2;
		       g = limG2;
		       b = limB2;
		       a = limA2;
		       if (lim1 != lim2) dr = (limR1 - limR2) / (lim1 - lim2);
		       if (lim1 != lim2) dg = (limG1 - limG2) / (lim1 - lim2);
		       if (lim1 != lim2) db = (limB1 - limB2) / (lim1 - lim2);
		       if (lim1 != lim2) da = (limA1 - limA2) / (lim1 - lim2);
		       s = limS2;
		       t = limT2;
		       if (lim1 != lim2) ds = (limS1 - limS2) / (lim1 - lim2);
		       if (lim1 != lim2) dt = (limT1 - limT2) / (lim1 - lim2);
		       w = limW2;
		       if (lim1 != lim2) dw = (limW1 - limW2) / (lim1 - lim2);
		    }
		  for (int x=min; x<max; x++, r+=dr, g+=dg, b+=db, a+=da, s+=ds, t+=dt, w+=dw)
		    {
		       if (x >= sulx && x < slrx)
			 {
			    Color32 shade((int)(r+0.5f), (int)(g+0.5f), (int)(b+0.5f), (int)(a+0.5f));
			    rdp->cc->setShade(shade);
			    Color32 txt= rdp->tx->getTexel(s/w, t/w, tile, rdp->tf);
			    Color32 c  = rdp->cc->combine1(txt);
			    rdp->bl->cycle1ModeDraw(x,y,c,0,shade);
			 }
		    }
	       }
	  }
	
	// initiate coordinates 2 :
	lim1 = (*v[1])[0];
	if ((*v[2])[1] != (*v[1])[1]) dx1 = ((*v[2])[0] - lim1) / ((*v[2])[1] - (*v[1])[1]);
	// initiate shade 2 :
	limR1 = c[1]->getR();
	limG1 = c[1]->getG();
	limB1 = c[1]->getB();
	limA1 = c[1]->getAlpha();
	if ((*v[2])[1] != (*v[1])[1]) dR1 = (c[2]->getR() - limR1) / ((*v[2])[1] - (*v[1])[1]);
	if ((*v[2])[1] != (*v[1])[1]) dG1 = (c[2]->getG() - limG1) / ((*v[2])[1] - (*v[1])[1]);
	if ((*v[2])[1] != (*v[1])[1]) dB1 = (c[2]->getB() - limB1) / ((*v[2])[1] - (*v[1])[1]);
	if ((*v[2])[1] != (*v[1])[1]) dA1 = (c[2]->getAlpha() - limA1) / ((*v[2])[1] - (*v[1])[1]);
	// initiate s and t 2:
	limS1 = *s[1];
	limT1 = *t[1];
	if ((*v[2])[1] != (*v[1])[1]) dS1 = (*s[2] - limS1) / ((*v[2])[1] - (*v[1])[1]);
	if ((*v[2])[1] != (*v[1])[1]) dT1 = (*t[2] - limT1) / ((*v[2])[1] - (*v[1])[1]);
	// initiate w 2 :
	limW1 = *w[1];
	if ((*v[2])[1] != (*v[1])[1]) dW1 = (*w[2] - limW1) / ((*v[2])[1] - (*v[1])[1]);
	
	// second half of the triangle
	for (; y < (int)((*v[2])[1]+0.5f); y++, lim1=lim1+dx1, lim2=lim2+dx2,
	     limR1+=dR1, limR2+=dR2, limG1+=dG1, limG2+=dG2, limB1+=dB1, limB2+=dB2, limA1+=dA1, limA2+=dA2,
	     limS1+=dS1, limS2+=dS2, limT1+=dT1, limT2+=dT2, limW1+=dW1, limW2+=dW2)
	  {
	     if (y >= suly && y < slry)
	       {
		  int min, max;
		  float r, g, b, a, dr = 0.0f, dg = 0.0f, db = 0.0f, da = 0.0f;
		  float s, t, ds = 0.0f, dt = 0.0f;
		  float w, dw = 0.0f;
		  if (lim1 < lim2)
		    {
		       min = (int)(lim1+0.5f);
		       max = (int)(lim2+0.5f);
		       r = limR1;
		       g = limG1;
		       b = limB1;
		       a = limA1;
		       if (lim1 != lim2) dr = (limR2 - limR1) / (lim2 - lim1);
		       if (lim1 != lim2) dg = (limG2 - limG1) / (lim2 - lim1);
		       if (lim1 != lim2) db = (limB2 - limB1) / (lim2 - lim1);
		       if (lim1 != lim2) da = (limA2 - limA1) / (lim2 - lim1);
		       s = limS1;
		       t = limT1;
		       if (lim1 != lim2) ds = (limS2 - limS1) / (lim2 - lim1);
		       if (lim1 != lim2) dt = (limT2 - limT1) / (lim2 - lim1);
		       w = limW1;
		       if (lim1 != lim2) dw = (limW2 - limW1) / (lim2 - lim1);
		    }
		  else
		    {
		       min = (int)(lim2+0.5f);
		       max = (int)(lim1+0.5f);
		       r = limR2;
		       g = limG2;
		       b = limB2;
		       a = limA2;
		       if (lim1 != lim2) dr = (limR1 - limR2) / (lim1 - lim2);
		       if (lim1 != lim2) dg = (limG1 - limG2) / (lim1 - lim2);
		       if (lim1 != lim2) db = (limB1 - limB2) / (lim1 - lim2);
		       if (lim1 != lim2) da = (limA1 - limA2) / (lim1 - lim2);
		       s = limS2;
		       t = limT2;
		       if (lim1 != lim2) ds = (limS1 - limS2) / (lim1 - lim2);
		       if (lim1 != lim2) dt = (limT1 - limT2) / (lim1 - lim2);
		       w = limW2;
		       if (lim1 != lim2) dw = (limW1 - limW2) / (lim1 - lim2);
		    }
		  for (int x=min; x<max; x++, r+=dr, g+=dg, b+=db, a+=da, s+=ds, t+=dt, w+=dw)
		    {
		       if (x >= sulx && x < slrx)
			 {
			    Color32 shade((int)(r+0.5f), (int)(g+0.5f), (int)(b+0.5f), (int)(a+0.5f));
			    rdp->cc->setShade(shade);
			    Color32 txt = rdp->tx->getTexel(s/w, t/w, tile, rdp->tf);
			    Color32 c = rdp->cc->combine1(txt);
			    rdp->bl->cycle1ModeDraw(x,y,c,0,shade);
			 }
		    }
	       }
	  }
     }
   else
     printf("RS:unknown cycle type in tri_shade_txtr_zbuff:%d\n", rdp->cycleType);
}

void RS::tri_shade(Vektor<float,4>& v0, Vektor<float,4>& v1, Vektor<float,4>& v2, 
		   Color32& c0, Color32& c1, Color32& c2, RDP *rdp)
{
   if (rdp->cycleType == 0)
     {
	// sorting vertex by y values
	Vektor<float,4>* v[3] = { &v0, &v1, &v2 };
	Color32* c[3];
	Vektor<float,4> *temp;
	if ((*v[0])[1] > (*v[1])[1]) { temp = v[0]; v[0] = v[1]; v[1] = temp; }
	if ((*v[1])[1] > (*v[2])[1]) { temp = v[1]; v[1] = v[2]; v[2] = temp; }
	if ((*v[0])[1] > (*v[1])[1]) { temp = v[0]; v[0] = v[1]; v[1] = temp; }
	if ((*v[1])[1] > (*v[2])[1]) { temp = v[1]; v[1] = v[2]; v[2] = temp; }
	for (int i=0; i<3; i++)
	  {
	     if (v[i] == &v0) { c[i] = &c0; }
	     if (v[i] == &v1) { c[i] = &c1; }
	     if (v[i] == &v2) { c[i] = &c2; }
	  }
	
	// initiate coordinates :
	float lim1, lim2, dx1 = 0.0f, dx2 = 0.0f;
	lim1 = (*v[0])[0];
	lim2 = lim1;
	if ((*v[1])[1] != (*v[0])[1]) dx1 = ((*v[1])[0] - lim1) / ((*v[1])[1] - (*v[0])[1]);
	if ((*v[2])[1] != (*v[0])[1]) dx2 = ((*v[2])[0] - lim2) / ((*v[2])[1] - (*v[0])[1]);
	// initiate shade :
       	float limR1, limG1, limB1, limA1, limR2, limG2, limB2, limA2;
	float dR1 = 0.0f, dG1 = 0.0f, dB1 = 0.0f, dA1 = 0.0f, dR2 = 0.0f, dG2 = 0.0f, dB2 = 0.0f, dA2 = 0.0f;
	limR1 = limR2 = c[0]->getR();
	limG1 = limG2 = c[0]->getG();
	limB1 = limB2 = c[0]->getB();
	limA1 = limA2 = c[0]->getAlpha();
	if ((*v[1])[1] != (*v[0])[1]) dR1 = (c[1]->getR() - limR1) / ((*v[1])[1] - (*v[0])[1]);
	if ((*v[2])[1] != (*v[0])[1]) dR2 = (c[2]->getR() - limR2) / ((*v[2])[1] - (*v[0])[1]);
	if ((*v[1])[1] != (*v[0])[1]) dG1 = (c[1]->getG() - limG1) / ((*v[1])[1] - (*v[0])[1]);
	if ((*v[2])[1] != (*v[0])[1]) dG2 = (c[2]->getG() - limG2) / ((*v[2])[1] - (*v[0])[1]);
	if ((*v[1])[1] != (*v[0])[1]) dB1 = (c[1]->getB() - limB1) / ((*v[1])[1] - (*v[0])[1]);
	if ((*v[2])[1] != (*v[0])[1]) dB2 = (c[2]->getB() - limB2) / ((*v[2])[1] - (*v[0])[1]);
	if ((*v[1])[1] != (*v[0])[1]) dA1 = (c[1]->getAlpha() - limA1) / ((*v[1])[1] - (*v[0])[1]);
	if ((*v[2])[1] != (*v[0])[1]) dA2 = (c[2]->getAlpha() - limA2) / ((*v[2])[1] - (*v[0])[1]);
	
	// first half of the triangle
	int y;
	for (y = (int)((*v[0])[1]+0.5f); y < (int)((*v[1])[1]+0.5f); y++, lim1+=dx1, lim2+=dx2, 
	     limR1+=dR1, limR2+=dR2, limG1+=dG1, limG2+=dG2, limB1+=dB1, limB2+=dB2, limA1+=dA1, limA2+=dA2)
	  {
	     if (y >= suly && y < slry)
	       {
		  int min, max;
		  float r, g, b, a, dr = 0.0f, dg = 0.0f, db = 0.0f, da = 0.0f;
		  if (lim1 < lim2)
		    {
		       min = (int)(lim1+0.5f);
		       max = (int)(lim2+0.5f);
		       r = limR1;
		       g = limG1;
		       b = limB1;
		       a = limA1;
		       if (lim1 != lim2) dr = (limR2 - limR1) / (lim2 - lim1);
		       if (lim1 != lim2) dg = (limG2 - limG1) / (lim2 - lim1);
		       if (lim1 != lim2) db = (limB2 - limB1) / (lim2 - lim1);
		       if (lim1 != lim2) da = (limA2 - limA1) / (lim2 - lim1);
		    }
		  else
		    {
		       min = (int)(lim2+0.5f);
		       max = (int)(lim1+0.5f);
		       r = limR2;
		       g = limG2;
		       b = limB2;
		       a = limA2;
		       if (lim1 != lim2) dr = (limR1 - limR2) / (lim1 - lim2);
		       if (lim1 != lim2) dg = (limG1 - limG2) / (lim1 - lim2);
		       if (lim1 != lim2) db = (limB1 - limB2) / (lim1 - lim2);
		       if (lim1 != lim2) da = (limA1 - limA2) / (lim1 - lim2);
		    }
		  for (int x=min; x<max; x++, r+=dr, g+=dg, b+=db, a+=da)
		    {
		       if (x >= sulx && x < slrx)
			 {
			    Color32 shade((int)(r+0.5f), (int)(g+0.5f), (int)(b+0.5f), (int)(a+0.5f));
			    rdp->cc->setShade(shade);
			    Color32 t = 0;
			    Color32 c = rdp->cc->combine1(t);
			    rdp->bl->cycle1ModeDraw(x,y,c,0,shade);
			 }
		    }
	       }
	  }
	
	// initiate coordinates 2 :
	lim1 = (*v[1])[0];
	if ((*v[2])[1] != (*v[1])[1]) dx1 = ((*v[2])[0] - lim1) / ((*v[2])[1] - (*v[1])[1]);
	// initiate shade 2 :
	limR1 = c[1]->getR();
	limG1 = c[1]->getG();
	limB1 = c[1]->getB();
	limA1 = c[1]->getAlpha();
	if ((*v[2])[1] != (*v[1])[1]) dR1 = (c[2]->getR() - limR1) / ((*v[2])[1] - (*v[1])[1]);
	if ((*v[2])[1] != (*v[1])[1]) dG1 = (c[2]->getG() - limG1) / ((*v[2])[1] - (*v[1])[1]);
	if ((*v[2])[1] != (*v[1])[1]) dB1 = (c[2]->getB() - limB1) / ((*v[2])[1] - (*v[1])[1]);
	if ((*v[2])[1] != (*v[1])[1]) dA1 = (c[2]->getAlpha() - limA1) / ((*v[2])[1] - (*v[1])[1]);
	
	// second half of the triangle
	for (; y < (int)((*v[2])[1]+0.5f); y++, lim1=lim1+dx1, lim2=lim2+dx2,
	     limR1+=dR1, limR2+=dR2, limG1+=dG1, limG2+=dG2, limB1+=dB1, limB2+=dB2, limA1+=dA1, limA2+=dA2)
	  {
	     if (y >= suly && y < slry)
	       {
		  int min, max;
		  float r, g, b, a, dr = 0.0f, dg = 0.0f, db = 0.0f, da = 0.0f;
		  if (lim1 < lim2)
		    {
		       min = (int)(lim1+0.5f);
		       max = (int)(lim2+0.5f);
		       r = limR1;
		       g = limG1;
		       b = limB1;
		       a = limA1;
		       if (lim1 != lim2) dr = (limR2 - limR1) / (lim2 - lim1);
		       if (lim1 != lim2) dg = (limG2 - limG1) / (lim2 - lim1);
		       if (lim1 != lim2) db = (limB2 - limB1) / (lim2 - lim1);
		       if (lim1 != lim2) da = (limA2 - limA1) / (lim2 - lim1);
		    }
		  else
		    {
		       min = (int)(lim2+0.5f);
		       max = (int)(lim1+0.5f);
		       r = limR2;
		       g = limG2;
		       b = limB2;
		       a = limA2;
		       if (lim1 != lim2) dr = (limR1 - limR2) / (lim1 - lim2);
		       if (lim1 != lim2) dg = (limG1 - limG2) / (lim1 - lim2);
		       if (lim1 != lim2) db = (limB1 - limB2) / (lim1 - lim2);
		       if (lim1 != lim2) da = (limA1 - limA2) / (lim1 - lim2);
		    }
		  for (int x=min; x<max; x++, r+=dr, g+=dg, b+=db, a+=da)
		    {
		       if (x >= sulx && x < slrx)
			 {
			    Color32 shade((int)(r+0.5f), (int)(g+0.5f), (int)(b+0.5f), (int)(a+0.5f));
			    rdp->cc->setShade(shade);
			    Color32 t = 0;
			    Color32 c = rdp->cc->combine1(t);
			    rdp->bl->cycle1ModeDraw(x,y,c,0,shade);
			 }
		    }
	       }
	  }
     }
   else
     printf("RS:unknown cycle type in tri_shade_txtr_zbuff:%d\n", rdp->cycleType);
}

void RS::debug_tri(Vektor<float,4>& v0, Vektor<float,4>& v1, Vektor<float,4>& v2, RDP *rdp)
{
   // 0 -> 1
   if (v0[1] == v1[1])
     {
	if (v0[0] < v1[0])
	  for (int i=(int)v0[0]; i<=v1[0]; i++)
	    {
	       if (i >= sulx && i < slrx && v0[1] >= suly && v0[1] < slry)
		 rdp->bl->debug_plot(i, (int)v0[1], 0xFFFF);
	    }
	else
	  for (int i=(int)v1[0]; i<=v0[0]; i++)
	    {
	       if (i >= sulx && i < slrx && v0[1] >= suly && v0[1] < slry)
		 rdp->bl->debug_plot(i, (int)v0[1], 0xFFFF);
	    }
     }
   else
     {
	if (v0[1] < v1[1])
	  {
	     float dx = (v1[0] - v0[0]) / (v1[1] - v0[1]);
	     float x = v0[0];
	     for (int j=(int)v0[1]; j<=v1[1]; j++,x+=dx)
	       {
		  if ((int)x >= sulx && (int)x < slrx && j >= suly && j < slry)
		    rdp->bl->debug_plot((int)x, j, 0xFFFF);
	       }
	  }
	else
	  {
	     float dx = (v0[0] - v1[0]) / (v0[1] - v1[1]);
	     float x = v1[0];
	     for (int j=(int)v1[1]; j<=v0[1]; j++,x+=dx)
	       {
		  if ((int)x >= sulx && (int)x < slrx && j >= suly && j < slry)
		    rdp->bl->debug_plot((int)x, j, 0xFFFF);
	       }
	  }
     }
   
   // 1 -> 2
   if (v1[1] == v2[1])
     {
	if (v1[0] < v2[0])
	  for (int i=(int)v1[0]; i<=v2[0]; i++)
	    {
	       if (i >= sulx && i < slrx && v1[1] >= suly && v1[1] < slry)
		 rdp->bl->debug_plot(i, (int)v1[1], 0xFFFF);
	    }
	else
	  for (int i=(int)v2[0]; i<=v1[0]; i++)
	    {
	       if (i >= sulx && i < slrx && v1[1] >= suly && v1[1] < slry)
		 rdp->bl->debug_plot(i, (int)v1[1], 0xFFFF);
	    }
     }
   else
     {
	if (v1[1] < v2[1])
	  {
	     float dx = (v2[0] - v1[0]) / (v2[1] - v1[1]);
	     float x = v1[0];
	     for (int j=(int)v1[1]; j<=v2[1]; j++,x+=dx)
	       {
		  if ((int)x >= sulx && (int)x < slrx && j >= suly && j < slry)
		    rdp->bl->debug_plot((int)x, j, 0xFFFF);
	       }
	  }
	else
	  {
	     float dx = (v1[0] - v2[0]) / (v1[1] - v2[1]);
	     float x = v2[0];
	     for (int j=(int)v2[1]; j<=v1[1]; j++,x+=dx)
	       {
		  if ((int)x >= sulx && (int)x < slrx && j >= suly && j < slry)
		    rdp->bl->debug_plot((int)x, j, 0xFFFF);
	       }
	  }
     }
   
   // 2 -> 0
   if (v2[1] == v0[1])
     {
	if (v2[0] < v0[0])
	  for (int i=(int)v2[0]; i<=v0[0]; i++)
	    {
	       if (i >= sulx && i < slrx && v2[1] >= suly && v2[1] < slry)
		 rdp->bl->debug_plot(i, (int)v2[1], 0xFFFF);
	    }
	else
	  for (int i=(int)v0[0]; i<=v2[0]; i++)
	    {
	       if (i >= sulx && i < slrx && v2[1] >= suly && v2[1] < slry)
		 rdp->bl->debug_plot(i, (int)v2[1], 0xFFFF);
	    }
     }
   else
     {
	if (v2[1] < v0[1])
	  {
	     float dx = (v0[0] - v2[0]) / (v0[1] - v2[1]);
	     float x = v2[0];
	     for (int j=(int)v2[1]; j<=v0[1]; j++,x+=dx)
	       {
		  if ((int)x >= sulx && (int)x < slrx && j >= suly && j < slry)
		    rdp->bl->debug_plot((int)x, j, 0xFFFF);
	       }
	  }
	else
	  {
	     float dx = (v2[0] - v0[0]) / (v2[1] - v0[1]);
	     float x = v0[0];
	     for (int j=(int)v0[1]; j<=v2[1]; j++,x+=dx)
	       {
		  if ((int)x >= sulx && (int)x < slrx && j >= suly && j < slry)
		    rdp->bl->debug_plot((int)x, j, 0xFFFF);
	       }
	  }
     }
}
