/**
 * Mupen64 - rsp.cpp
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

#include "global.h"
#include "rsp.h"

RSP::RSP(GFX_INFO info) : gfxInfo(info), error(false), end(false)
{
   for (int i=0; i<0x100; i++) commands[i]=&RSP::NI;
   commands[0x00]=&RSP::SPNOOP;
   commands[0x01]=&RSP::MTX;
   commands[0x03]=&RSP::MOVEMEM;
   commands[0x04]=&RSP::VTX;
   commands[0x06]=&RSP::DL;
   commands[0x09]=&RSP::SPRITE2D;
   commands[0xb4]=&RSP::RDPHALF_1;
   commands[0xb6]=&RSP::CLEARGEOMETRYMODE;
   commands[0xb7]=&RSP::SETGEOMETRYMODE;
   commands[0xb8]=&RSP::ENDDL;
   commands[0xb9]=&RSP::SETOTHERMODE_L;
   commands[0xba]=&RSP::SETOTHERMODE_H;
   commands[0xbb]=&RSP::TEXTURE;
   commands[0xbc]=&RSP::MOVEWORD;
   commands[0xbd]=&RSP::POPMTX;
   commands[0xbf]=&RSP::TRI1;
   commands[0xc0]=&RSP::SPNOOP;
   commands[0xe4]=&RSP::TEXRECT;
   commands[0xe6]=&RSP::RDPLOADSYNC;
   commands[0xe7]=&RSP::RDPPIPESYNC;
   commands[0xe8]=&RSP::RDPTILESYNC;
   commands[0xe9]=&RSP::RDPFULLSYNC;
   commands[0xed]=&RSP::SETSCISSOR;
   commands[0xf0]=&RSP::LOADTLUT;
   commands[0xf2]=&RSP::SETTILESIZE;
   commands[0xf3]=&RSP::LOADBLOCK;
   commands[0xf4]=&RSP::LOADTILE;
   commands[0xf5]=&RSP::SETTILE;
   commands[0xf6]=&RSP::FILLRECT;
   commands[0xf7]=&RSP::SETFILLCOLOR;
   commands[0xf8]=&RSP::SETFOGCOLOR;
   commands[0xf9]=&RSP::SETBLENDCOLOR;
   commands[0xfa]=&RSP::SETPRIMCOLOR;
   commands[0xfb]=&RSP::SETENVCOLOR;
   commands[0xfc]=&RSP::SETCOMBINE;
   commands[0xfd]=&RSP::SETTIMG;
   commands[0xfe]=&RSP::SETZIMG;
   commands[0xff]=&RSP::SETCIMG;
   
   texture_gen_linear = false;
   texture_gen = false;
   lighting = false;
   fog = false;
   cull_back = false;
   cull_front = false;
   shading_smooth = false;
   shade = false;
   zbuffer = false;
   geometryMode = 0;
   
   rdp = new RDP(info);
   
   executeDList();
}

RSP::~RSP()
{
   delete rdp;
}

unsigned long RSP::seg2phys(unsigned long seg)
{
   return segments[(seg>>24)&0xF] + (seg & 0x7FFFFF);
}

void RSP::executeDList()
{
   OSTask_t *task = (OSTask_t*)(gfxInfo.DMEM+0xFC0);
   unsigned long *start = (unsigned long*)(gfxInfo.RDRAM + task->data_ptr);
   //unsigned long length = task->data_size;
   currentCommand = start;
   
   while(!end /*&& i < length*/ /*&& !error*/)
     {
	(this->*commands[*currentCommand>>24])();
	currentCommand+=2;
     }
}

void RSP::DL()
{
   int push = (*currentCommand>>16) & 0xFF;
   unsigned long addr = seg2phys(*(currentCommand+1)) & 0x7FFFFF;
   unsigned long *returnCommand = currentCommand;
   switch(push)
     {
      case 0: // DL_PUSH
	currentCommand = (unsigned long*)(gfxInfo.RDRAM + addr);
	while ((*currentCommand >> 24) != 0xB8) // 0xB8 -> ENDDL code
	  {
	     (this->*commands[*currentCommand>>24])();
	     currentCommand+=2;
	  }
	currentCommand = returnCommand;
	break;
      case 1: // NO_PUSH
	currentCommand = (unsigned long*)(gfxInfo.RDRAM + addr) - 2;
	break;
      default:
	printf("unknown DL: push=%x\n", push);
	error = true;
     }
}

void RSP::NI()
{
   printf("NI:%x\n", (int)(*currentCommand>>24));
   //if (!error) getchar();
   error = true;
}

void RSP::SPNOOP()
{
}

void RSP::MTX()
{
   unsigned long addr = seg2phys(*(currentCommand+1)) & 0x7FFFFF;
   int op = (*currentCommand >> 16) & 0xFF;
   
   unsigned short* p = (unsigned short*)(gfxInfo.RDRAM + addr);
   Matrix<float,4> matrix;
   float temp(16);
   for (int j=0; j<4; j++)
     {
	for (int i=0; i<4; i++)
	  {
	     temp = (((int)p[j*4+i^S16] << 16) | (int)p[16+j*4+i^S16]) / 65536.0f;
	     matrix(j,i) = temp;
	  }
     }
   
   switch (op)
     {
      case 0: // MODELVIEW MUL NOPUSH
	  {
	     Matrix<float,4> m = matrix * modelView;
	     modelView = m;
	  }
	break;
      case 1: // PROJECTION MUL NOPUSH
	  {
	     Matrix<float,4> m = matrix * projection;
	     projection = m;
	  }
	break;
      case 2: // MODELVIEW LOAD NOPUSH
	modelView = matrix;
	break;
      case 3: // PROJECTION LOAD NOPUSH
	projection = matrix;
	break;
      case 4: // MODELVIEW MUL PUSH
	  {
	     modelView.push();
	     Matrix<float,4> m = matrix * modelView;
	     modelView = m;
	  }
	break;
      case 6: // MODELVIEW LOAD PUSH
	modelView.push();
	modelView = matrix;
	break;
      default:
	printf("RSP: unknown MTX:%d\n", op);
	error = true;
     }
   MP = modelView * projection;
}

void RSP::MOVEMEM()
{
   int dest = (*currentCommand>>16)&0xFF;
   int length;
   length = *currentCommand & 0xFFFF;
   unsigned long addr = seg2phys(*(currentCommand+1)) & 0x7FFFFF;
   
   switch(dest)
     {
      case 0x80:
	viewport.vscale[0] = (int)*((short*)(gfxInfo.RDRAM + addr) + (0^S16)) / 4.0;
	viewport.vscale[1] = (int)*((short*)(gfxInfo.RDRAM + addr) + (1^S16)) / 4.0;
	viewport.vscale[2] = (int)*((short*)(gfxInfo.RDRAM + addr) + (2^S16));
	viewport.vscale[3] = (int)*((short*)(gfxInfo.RDRAM + addr) + (3^S16)) / 4.0;
	viewport.vtrans[0] = (int)*((short*)(gfxInfo.RDRAM + addr) + (4^S16)) / 4.0;
	viewport.vtrans[1] = (int)*((short*)(gfxInfo.RDRAM + addr) + (5^S16)) / 4.0;
	viewport.vtrans[2] = (int)*((short*)(gfxInfo.RDRAM + addr) + (6^S16));
	viewport.vtrans[3] = (int)*((short*)(gfxInfo.RDRAM + addr) + (7^S16)) / 4.0;
	break;
      case 0x82:
	lookAtY.col = *((int*)(gfxInfo.RDRAM + addr) + 0);
	lookAtY.colc= *((int*)(gfxInfo.RDRAM + addr) + 1);
	lookAtY.dir[0] = *((char*)gfxInfo.RDRAM + addr + (8^S8)) / 128.0f;
	lookAtY.dir[1] = *((char*)gfxInfo.RDRAM + addr + (9^S8)) / 128.0f;
	lookAtY.dir[2] = *((char*)gfxInfo.RDRAM + addr + (10^S8)) / 128.0f;
	lookAtY.dir[3] = *((char*)gfxInfo.RDRAM + addr + (11^S8)) / 128.0f;
	break;
      case 0x84:
	lookAtX.col = *((int*)(gfxInfo.RDRAM + addr) + 0);
	lookAtX.colc= *((int*)(gfxInfo.RDRAM + addr) + 1);
	lookAtX.dir[0] = *((char*)gfxInfo.RDRAM + addr + (8^S8)) / 128.0f;
	lookAtX.dir[1] = *((char*)gfxInfo.RDRAM + addr + (9^S8)) / 128.0f;
	lookAtX.dir[2] = *((char*)gfxInfo.RDRAM + addr + (10^S8)) / 128.0f;
	lookAtX.dir[3] = *((char*)gfxInfo.RDRAM + addr + (11^S8)) / 128.0f;
	break;
      case 0x86:
      case 0x88:
      case 0x8a:
	  {
	     int n = (dest-0x86)/2;
	     if (n< numLight)
	       {
		  spotLight[n].col = *((int*)(gfxInfo.RDRAM + addr) + 0);
		  spotLight[n].colc= *((int*)(gfxInfo.RDRAM + addr) + 1);
		  spotLight[n].dir[0] = *((char*)gfxInfo.RDRAM + addr + (8^S8)) / 128.0f;
		  spotLight[n].dir[1] = *((char*)gfxInfo.RDRAM + addr + (9^S8)) / 128.0f;
		  spotLight[n].dir[2] = *((char*)gfxInfo.RDRAM + addr + (10^S8)) / 128.0f;
		  spotLight[n].dir[3] = 0.0f;
		  spotLight[n].dir.normalize();
	       }
	     else
	       {
		  ambientLight.col = *((int*)(gfxInfo.RDRAM + addr) + 0);
		  ambientLight.colc= *((int*)(gfxInfo.RDRAM + addr) + 1);
	       }
	  }
	break;
      default:
	printf("unknown MOVEMEM:%x\n", dest);
	error=true;
     }
}

void RSP::VTX()
{
   unsigned long addr = seg2phys(*(currentCommand+1)) & 0x7FFFFF;
   char* p = (char*)(gfxInfo.RDRAM + addr);
   int v0 = (*currentCommand >> 16) & 0xF;
   int n = ((*currentCommand >> 20) & 0xF)+1;
   //int length = *currentCommand & 0xFFFF;
   
   for (int i=0; i<n; i++)
     {
	vtx[v0+i].v[0] = (int)(*((short*)(p + i*16 + (0^(S16<<1)))));
	vtx[v0+i].v[1] = (int)(*((short*)(p + i*16 + (2^(S16<<1)))));
	vtx[v0+i].v[2] = (int)(*((short*)(p + i*16 + (4^(S16<<1)))));
	vtx[v0+i].v[3] = 1;
	vtx[v0+i].v = vtx[v0+i].v * MP;
	
	vtx[v0+i].s = *((short*)(p + i*16 + (8^(S16<<1)))) / 32.0f;
	vtx[v0+i].t = *((short*)(p + i*16 + (10^(S16<<1)))) / 32.0f;
	vtx[v0+i].s *= textureScales.sc;
	vtx[v0+i].t *= textureScales.tc;
	
	if (lighting)
	  {
	     vtx[v0+i].n[0] = *((char*)(p + i*16 + (12^S8))) / 128.0f;
	     vtx[v0+i].n[1] = *((char*)(p + i*16 + (13^S8))) / 128.0f;
	     vtx[v0+i].n[2] = *((char*)(p + i*16 + (14^S8))) / 128.0f;
	     vtx[v0+i].n[3] = 0.0f;
	     vtx[v0+i].n = vtx[v0+i].n * modelView;
	     vtx[v0+i].n.normalize();
	     
	     vtx[v0+i].c = ambientLight.col;
	     for (int j=0; j<numLight; j++)
	       {
		  float cosT = vtx[v0+i].n.scalar(spotLight[j].dir);
		  if (cosT > 0.0f)
		    vtx[v0+i].c += spotLight[j].col*cosT;
	       }
	     vtx[v0+i].c.clamp();
	     vtx[v0+i].c.setAlpha(*((unsigned char*)(p + i*16 + (15^S8))));
	     
	     if (texture_gen)
	       {
		  vtx[v0+i].s = (asinf(vtx[v0+i].n[0])/3.14f + 0.5f) * textureScales.sc * 1024.0f;
		  vtx[v0+i].t = (asinf(vtx[v0+i].n[1])/3.14f + 0.5f) * textureScales.tc * 1024.0f;
	       }
	  }
	else
	  {
	     vtx[v0+i].c.setR(*((unsigned char*)(p + i*16 + (12^S8))));
	     vtx[v0+i].c.setG(*((unsigned char*)(p + i*16 + (13^S8))));
	     vtx[v0+i].c.setB(*((unsigned char*)(p + i*16 + (14^S8))));
	     vtx[v0+i].c.setAlpha(*((unsigned char*)(p + i*16 + (15^S8))));
	  }
     }
}

void RSP::SPRITE2D()
{
   unsigned long addr = seg2phys(*(currentCommand+1)) & 0x7FFFFF;
   void* SourceImagePointer = (void*)(gfxInfo.RDRAM + (seg2phys(*((int*)(gfxInfo.RDRAM + addr))) & 0x7FFFFF));
   void* TlutPointer = (void*)(gfxInfo.RDRAM + (seg2phys(*((int*)(gfxInfo.RDRAM + addr + 4))) & 0x7FFFFF));
   short Stride = *((short*)(gfxInfo.RDRAM + addr + (8^(S16<<1))));
   short SubImageWidth = *((short*)(gfxInfo.RDRAM + addr + (10^(S16<<1))));
   short SubImageHeight = *((short*)(gfxInfo.RDRAM + addr + (12^(S16<<1))));
   char SourceImageType = *((char*)(gfxInfo.RDRAM + addr + (14^S8)));
   char SourceImageBitSize = *((char*)(gfxInfo.RDRAM + addr + (15^S8)));
   float ScaleX = *((short*)(gfxInfo.RDRAM + addr + (16^(S16<<1)))) / 1024.0f;
   float ScaleY = *((short*)(gfxInfo.RDRAM + addr + (18^(S16<<1)))) / 1024.0f;
   char FlipTextureX = *((char*)(gfxInfo.RDRAM + addr + (20^S8)));
   char FlipTextureY = *((char*)(gfxInfo.RDRAM + addr + (21^S8)));
   short SourceImageOffsetS = *((short*)(gfxInfo.RDRAM + addr + (22^(S16<<1))));
   short SourceImageOffsetT = *((short*)(gfxInfo.RDRAM + addr + (24^(S16<<1))));
   short PScreenX = *((short*)(gfxInfo.RDRAM + addr + (26^(S16<<1))));
   short PScreenY = *((short*)(gfxInfo.RDRAM + addr + (28^(S16<<1))));
   float pScreenY = PScreenY / 4.0f;
   
   float uls = (int)SourceImageOffsetS;
   float lrs = (int)(SourceImageOffsetS + SubImageWidth - 1);
   
   float ulx = PScreenX;
   float lrx = ulx + SubImageWidth - 1;
   
   if (SourceImageBitSize == 0)
     printf("RSP:SPRITE2D image type=%d bitsize=%d\n", SourceImageType, SourceImageBitSize);
   if (FlipTextureX || FlipTextureY)
     printf("RSP:SPRITE2D flip\n");
   
   if (SourceImageType == 2)
     {
	// loading the TLUT
	rdp->setOtherMode_h(14, 2);
	rdp->setTImg(0, 2, 1, TlutPointer);
	rdp->setTile(0, 0, 0, 256, 0, 0, 0, 0, 0, 0, 0, 0);
	rdp->loadTLUT(0, 255);
     }
   
   int tOffs, oldTOffs;
   tOffs = oldTOffs = SourceImageOffsetT;
   
   rdp->setOtherMode_h(20,0);
   rdp->setCombineMode(0xffffff, 0xfffcf3ff);
   rdp->setOtherMode_l(3, 0x0c087008>>3);
   rdp->setTImg(SourceImageType, SourceImageBitSize, Stride, SourceImagePointer);
   rdp->setTile(SourceImageType, SourceImageBitSize, (Stride*SourceImageBitSize)/8, 0, 0, 0, 0, 0, 0, 0, 0, 0);
   
   while(tOffs != (SourceImageOffsetT + SubImageHeight))
     {
	if ((tOffs-oldTOffs+2)*Stride*SourceImageBitSize < 2048)
	  tOffs++;
	else
	  {
	     float top = oldTOffs;
	     float bottom = tOffs;
	     rdp->setTileSize(uls, top, lrs, bottom, 0);
	     rdp->loadTile(0, uls, top, lrs, bottom);
	     
	     float uly = pScreenY+(oldTOffs-SourceImageOffsetT)*ScaleY;
	     float lry = pScreenY+(tOffs-SourceImageOffsetT)*ScaleY;
	     rdp->texRect(0, ulx, uly, lrx, lry, uls, top, ScaleX, ScaleY);
	     
	     oldTOffs = tOffs++;
	  }
     }
}

void RSP::CLEARGEOMETRYMODE()
{
   int mode = *(currentCommand+1);
   
   if (mode & 0x80000) texture_gen_linear = false;
   if (mode & 0x40000) texture_gen = false;
   if (mode & 0x20000) lighting = false;
   if (mode & 0x10000) fog = false;
   if (mode & 0x2000) cull_back = false;
   if (mode & 0x1000) cull_front = false;
   if (mode & 0x200) shading_smooth = false;
   if (mode & 0x4) shade = false;
   if (mode & 0x1) zbuffer = false;
   
   geometryMode = 
     zbuffer                  | 
     shade              << 2  |
     shading_smooth     << 9  |
     cull_front         << 12 |
     fog                << 16 |
     texture_gen_linear << 19;
}

void RSP::SETGEOMETRYMODE()
{
   int mode = *(currentCommand+1);
   
   if (mode & 0x40000) texture_gen = true;
   if (mode & 0x20000) lighting = true;
   if (mode & 0x10000) fog = true;
   if (mode & 0x2000) cull_back = true;
   if (mode & 0x200) shading_smooth = true;
   if (mode & 0x4) shade = true;
   if (mode & 0x1) zbuffer = true;
   
   if (mode & ~0x72205)
     printf("unknown SETGEOMETRYMODE:%x\n", mode & ~0x72205);
   geometryMode = 
     zbuffer                  | 
     shade              << 2  |
     shading_smooth     << 9  |
     cull_front         << 12 |
     fog                << 16 |
     texture_gen_linear << 19;
}

void RSP::ENDDL()
{
   end = true;
}

void RSP::RDPHALF_1()
{
}

void RSP::SETOTHERMODE_L()
{
   int mode = (*currentCommand >> 8) & 0xFF;
   int length = *currentCommand & 0xFF;
   unsigned long data = *(currentCommand+1);
   
   rdp->setOtherMode_l(mode, (data>>mode)&((1<<length)-1));
}

void RSP::SETOTHERMODE_H()
{
   int mode = (*currentCommand >> 8) & 0xFF;
   int length = *currentCommand & 0xFF;
   unsigned long data = *(currentCommand+1);
   
   rdp->setOtherMode_h(mode, (data>>mode)&((1<<length)-1));
}

void RSP::TEXTURE()
{
   int tile = (*currentCommand >> 8) & 3;
   textureScales.tile = tile;
   textureScales.sc = (int)((*(currentCommand+1)>>16)&0xFFFF) / 65536.0;
   textureScales.tc = (int)(*(currentCommand+1)&0xFFFF) / 65536.0;
   textureScales.level = (*currentCommand >> 11) & 3;
   textureScales.enabled = *currentCommand & 1 ? true : false;
}

void RSP::MOVEWORD()
{
   int index = *currentCommand & 0xFF;
   int offset = (*currentCommand >> 8) & 0xFFFF;
   
   switch(index)
     {
      case 0x2: // NUMLIGHT
	numLight = ((*(currentCommand+1)-0x80000000)/32)-1;
	break;
      case 0x4: // CLIPRATIO
	if (offset == 0x4) clipRatio_RNX = *(currentCommand+1);
	else if (offset == 0xc) clipRatio_RNY = *(currentCommand+1);
	else if (offset == 0x14) clipRatio_RPX = *(currentCommand+1);
	else if (offset == 0x1c) clipRatio_RPY = *(currentCommand+1);
	break;
      case 0x6: // SEGMENT
	segments[(offset>>2)&0xF] = *(currentCommand+1) & 0x7FFFFF;
	break;
      case 0x8: // FOG
	fm = *(currentCommand+1) >> 16;
	fo = (short)(*(currentCommand+1) & 0xFFFF);
	break;
      default:
	printf("unknown MOVEWORD:%x\n", index);
	error=true;
     }
}

void RSP::POPMTX()
{
   int type = *(currentCommand+1) & 0xFF;
   
   if (type != 0) printf("POPMTX on projection matrix\n");
   modelView.pop();
}

void RSP::TRI1()
{
   int v0 = ((*(currentCommand+1) >> 16) & 0xFF) / 10;
   int v1 = ((*(currentCommand+1) >> 8) & 0xFF) / 10;
   int v2 = (*(currentCommand+1) & 0xFF) / 10;
   
   Vertex cache[140];
   int cache_size = 0;
   
   cache[cache_size++] = vtx[v0];
   cache[cache_size++] = vtx[v1];
   cache[cache_size++] = vtx[v2];
   
   Vertex copy[140];
   int copy_size = 0;
   
   // clip w
   for(int i=0; i<cache_size; i++)
     {
	int j = i+1;
	if(i == cache_size-1) j = 0;
	
	if(cache[i].v[3] > 0.0f)
	  {
	     if(cache[j].v[3] > 0.0f)
	       copy[copy_size++] = cache[j];
	     else
	       {
		  float percent = (-cache[i].v[3]) / (cache[j].v[3] - cache[i].v[3]);
		  copy[copy_size].v[0] = cache[i].v[0] + (cache[j].v[0] - cache[i].v[0]) * percent;
		  copy[copy_size].v[1] = cache[i].v[1] + (cache[j].v[1] - cache[i].v[1]) * percent;
		  copy[copy_size].v[2] = cache[i].v[2] + (cache[j].v[2] - cache[i].v[2]) * percent;
		  copy[copy_size].v[3] = 0.01f;
		  copy[copy_size].s = cache[i].s + (cache[j].s - cache[i].s) * percent;
		  copy[copy_size].t = cache[i].t + (cache[j].t - cache[i].t) * percent;
		  copy[copy_size].c.setR(cache[i].c.getR() + (cache[j].c.getR() - cache[i].c.getR()) * percent);
		  copy[copy_size].c.setG(cache[i].c.getG() + (cache[j].c.getG() - cache[i].c.getG()) * percent);
		  copy[copy_size].c.setB(cache[i].c.getB() + (cache[j].c.getB() - cache[i].c.getB()) * percent);
		  copy[copy_size].c.setAlpha(cache[i].c.getAlpha() + (cache[j].c.getAlpha() - cache[i].c.getAlpha()) * percent);
		  copy_size++;
	       }
	  }
	else
	  {
	     if(cache[j].v[3] > 0.0f)
	       {
		  float percent = (-cache[j].v[3]) / (cache[i].v[3] - cache[j].v[3]);
		  copy[copy_size].v[0] = cache[j].v[0] + (cache[i].v[0] - cache[j].v[0]) * percent;
		  copy[copy_size].v[1] = cache[j].v[1] + (cache[i].v[1] - cache[j].v[1]) * percent;
		  copy[copy_size].v[2] = cache[j].v[2] + (cache[i].v[2] - cache[j].v[2]) * percent;
		  copy[copy_size].v[3] = 0.01f;
		  copy[copy_size].s = cache[j].s + (cache[i].s - cache[j].s) * percent;
		  copy[copy_size].t = cache[j].t + (cache[i].t - cache[j].t) * percent;
		  copy[copy_size].c.setR(cache[j].c.getR() + (cache[i].c.getR() - cache[j].c.getR()) * percent);
		  copy[copy_size].c.setG(cache[j].c.getG() + (cache[i].c.getG() - cache[j].c.getG()) * percent);
		  copy[copy_size].c.setB(cache[j].c.getB() + (cache[i].c.getB() - cache[j].c.getB()) * percent);
		  copy[copy_size].c.setAlpha(cache[j].c.getAlpha() + (cache[i].c.getAlpha() - cache[j].c.getAlpha()) * percent);
		  copy_size++;
		  
		  copy[copy_size++] = cache[j];
	       }
	  }
     }
   
   if(copy_size == 0) return;
   
   for(int i=0; i<copy_size; i++)
     {
	copy[i].v[0] = viewport.vtrans[0] + copy[i].v[0] / copy[i].v[3] * viewport.vscale[0];
	copy[i].v[1] = viewport.vtrans[1] - copy[i].v[1] / copy[i].v[3] * viewport.vscale[1];
	if(zbuffer)
	  {
	     copy[i].v[2] = (viewport.vtrans[2] + copy[i].v[2] / copy[i].v[3] * viewport.vscale[2])*32.0f;
	  }
     }
   
   if (cull_back)
     {
	float ABx = copy[0].v[0] - copy[1].v[0];
	float ABy = copy[0].v[1] - copy[1].v[1];
	float ACx = copy[2].v[0] - copy[1].v[0];
	float ACy = copy[2].v[1] - copy[1].v[1];
	bool cull = (ABx * ACy - ABy * ACx) < 0;
	if (cull) return;
     }
   
   // clip xmin
   cache_size = 0;
   
   for(int i=0; i<copy_size; i++)
     {
	int j = i+1;
	if(i == copy_size-1) j = 0;
	
	if(copy[i].v[0] >= viewport.vtrans[0] - viewport.vscale[0])
	  {
	     if(copy[j].v[0] >= viewport.vtrans[0] - viewport.vscale[0])
	       cache[cache_size++] = copy[j];
	     else
	       {
		  float percent = (viewport.vtrans[0] - viewport.vscale[0] - copy[i].v[0]) / (copy[j].v[0] - copy[i].v[0]);
		  cache[cache_size].v[0] = viewport.vtrans[0] - viewport.vscale[0];
		  cache[cache_size].v[1] = copy[i].v[1] + (copy[j].v[1] - copy[i].v[1]) * percent;
		  cache[cache_size].v[2] = copy[i].v[2] + (copy[j].v[2] - copy[i].v[2]) * percent;
		  cache[cache_size].v[3] = 1.0f / (1.0f/copy[i].v[3] + (1.0f/copy[j].v[3] - 1.0f/copy[i].v[3]) * percent);
		  cache[cache_size].s = (copy[i].s/copy[i].v[3] + (copy[j].s/copy[j].v[3] - copy[i].s/copy[i].v[3]) * percent)*cache[cache_size].v[3];
		  cache[cache_size].t = (copy[i].t/copy[i].v[3] + (copy[j].t/copy[j].v[3] - copy[i].t/copy[i].v[3]) * percent)*cache[cache_size].v[3];
		  cache[cache_size].c.setR(copy[i].c.getR() + (copy[j].c.getR() - copy[i].c.getR()) * percent);
		  cache[cache_size].c.setG(copy[i].c.getG() + (copy[j].c.getG() - copy[i].c.getG()) * percent);
		  cache[cache_size].c.setB(copy[i].c.getB() + (copy[j].c.getB() - copy[i].c.getB()) * percent);
		  cache[cache_size].c.setAlpha(copy[i].c.getAlpha() + (copy[j].c.getAlpha() - copy[i].c.getAlpha()) * percent);
		  cache_size++;
	       }
	  }
	else
	  {
	     if(copy[j].v[0] >= viewport.vtrans[0] - viewport.vscale[0])
	       {
		  float percent = (viewport.vtrans[0] - viewport.vscale[0] - copy[j].v[0]) / (copy[i].v[0] - copy[j].v[0]);
		  cache[cache_size].v[0] = viewport.vtrans[0] - viewport.vscale[0];
		  cache[cache_size].v[1] = copy[j].v[1] + (copy[i].v[1] - copy[j].v[1]) * percent;
		  cache[cache_size].v[2] = copy[j].v[2] + (copy[i].v[2] - copy[j].v[2]) * percent;
		  cache[cache_size].v[3] = 1.0f / (1.0f/copy[j].v[3] + (1.0f/copy[i].v[3] - 1.0f/copy[j].v[3]) * percent);
		  cache[cache_size].s = (copy[j].s/copy[j].v[3] + (copy[i].s/copy[i].v[3] - copy[j].s/copy[j].v[3]) * percent)*cache[cache_size].v[3];
		  cache[cache_size].t = (copy[j].t/copy[j].v[3] + (copy[i].t/copy[i].v[3] - copy[j].t/copy[j].v[3]) * percent)*cache[cache_size].v[3];
		  cache[cache_size].c.setR(copy[j].c.getR() + (copy[i].c.getR() - copy[j].c.getR()) * percent);
		  cache[cache_size].c.setG(copy[j].c.getG() + (copy[i].c.getG() - copy[j].c.getG()) * percent);
		  cache[cache_size].c.setB(copy[j].c.getB() + (copy[i].c.getB() - copy[j].c.getB()) * percent);
		  cache[cache_size].c.setAlpha(copy[j].c.getAlpha() + (copy[i].c.getAlpha() - copy[j].c.getAlpha()) * percent);
		  cache_size++;
		  
		  cache[cache_size++] = copy[j];
	       }
	  }
     }
   
   // clip xmax
   copy_size = 0;
   
   for(int i=0; i<cache_size; i++)
     {
	int j = i+1;
	if(i == cache_size-1) j = 0;
	
	if(cache[i].v[0] < viewport.vtrans[0] + viewport.vscale[0])
	  {
	     if(cache[j].v[0] < viewport.vtrans[0] + viewport.vscale[0])
	       copy[copy_size++] = cache[j];
	     else
	       {
		  float percent = (viewport.vtrans[0] + viewport.vscale[0] - cache[i].v[0]) / (cache[j].v[0] - cache[i].v[0]);
		  copy[copy_size].v[0] = viewport.vtrans[0] + viewport.vscale[0];
		  copy[copy_size].v[1] = cache[i].v[1] + (cache[j].v[1] - cache[i].v[1]) * percent;
		  copy[copy_size].v[2] = cache[i].v[2] + (cache[j].v[2] - cache[i].v[2]) * percent;
		  copy[copy_size].v[3] = 1.0f / (1.0f/cache[i].v[3] + (1.0f/cache[j].v[3] - 1.0f/cache[i].v[3]) * percent);
		  copy[copy_size].s = (cache[i].s/cache[i].v[3] + (cache[j].s/cache[j].v[3] - cache[i].s/cache[i].v[3]) * percent)*copy[copy_size].v[3];
		  copy[copy_size].t = (cache[i].t/cache[i].v[3] + (cache[j].t/cache[j].v[3] - cache[i].t/cache[i].v[3]) * percent)*copy[copy_size].v[3];
		  copy[copy_size].c.setR(cache[i].c.getR() + (cache[j].c.getR() - cache[i].c.getR()) * percent);
		  copy[copy_size].c.setG(cache[i].c.getG() + (cache[j].c.getG() - cache[i].c.getG()) * percent);
		  copy[copy_size].c.setB(cache[i].c.getB() + (cache[j].c.getB() - cache[i].c.getB()) * percent);
		  copy[copy_size].c.setAlpha(cache[i].c.getAlpha() + (cache[j].c.getAlpha() - cache[i].c.getAlpha()) * percent);
		  copy_size++;
	       }
	  }
	else
	  {
	     if(cache[j].v[0] < viewport.vtrans[0] + viewport.vscale[0])
	       {
		  float percent = (viewport.vtrans[0] + viewport.vscale[0] - cache[j].v[0]) / (cache[i].v[0] - cache[j].v[0]);
		  copy[copy_size].v[0] = viewport.vtrans[0] + viewport.vscale[0];
		  copy[copy_size].v[1] = cache[j].v[1] + (cache[i].v[1] - cache[j].v[1]) * percent;
		  copy[copy_size].v[2] = cache[j].v[2] + (cache[i].v[2] - cache[j].v[2]) * percent;
		  copy[copy_size].v[3] = 1.0f / (1.0f/cache[j].v[3] + (1.0f/cache[i].v[3] - 1.0f/cache[j].v[3]) * percent);
		  copy[copy_size].s = (cache[j].s/cache[j].v[3] + (cache[i].s/cache[i].v[3] - cache[j].s/cache[j].v[3]) * percent)*copy[copy_size].v[3];
		  copy[copy_size].t = (cache[j].t/cache[j].v[3] + (cache[i].t/cache[i].v[3] - cache[j].t/cache[j].v[3]) * percent)*copy[copy_size].v[3];
		  copy[copy_size].c.setR(cache[j].c.getR() + (cache[i].c.getR() - cache[j].c.getR()) * percent);
		  copy[copy_size].c.setG(cache[j].c.getG() + (cache[i].c.getG() - cache[j].c.getG()) * percent);
		  copy[copy_size].c.setB(cache[j].c.getB() + (cache[i].c.getB() - cache[j].c.getB()) * percent);
		  copy[copy_size].c.setAlpha(cache[j].c.getAlpha() + (cache[i].c.getAlpha() - cache[j].c.getAlpha()) * percent);
		  copy_size++;
		  
		  copy[copy_size++] = cache[j];
	       }
	  }
     }
   
   // clip ymin
   cache_size = 0;
   
   for(int i=0; i<copy_size; i++)
     {
	int j = i+1;
	if(i == copy_size-1) j = 0;
	
	if(copy[i].v[1] >= viewport.vtrans[1] - viewport.vscale[1])
	  {
	     if(copy[j].v[1] >= viewport.vtrans[1] - viewport.vscale[1])
	       cache[cache_size++] = copy[j];
	     else
	       {
		  float percent = (viewport.vtrans[1] - viewport.vscale[1] - copy[i].v[1]) / (copy[j].v[1] - copy[i].v[1]);
		  cache[cache_size].v[0] = copy[i].v[0] + (copy[j].v[0] - copy[i].v[0]) * percent;
		  cache[cache_size].v[1] = viewport.vtrans[1] - viewport.vscale[1];
		  cache[cache_size].v[2] = copy[i].v[2] + (copy[j].v[2] - copy[i].v[2]) * percent;
		  cache[cache_size].v[3] = 1.0f / (1.0f/copy[i].v[3] + (1.0f/copy[j].v[3] - 1.0f/copy[i].v[3]) * percent);
		  cache[cache_size].s = (copy[i].s/copy[i].v[3] + (copy[j].s/copy[j].v[3] - copy[i].s/copy[i].v[3]) * percent)*cache[cache_size].v[3];
		  cache[cache_size].t = (copy[i].t/copy[i].v[3] + (copy[j].t/copy[j].v[3] - copy[i].t/copy[i].v[3]) * percent)*cache[cache_size].v[3];
		  cache[cache_size].c.setR(copy[i].c.getR() + (copy[j].c.getR() - copy[i].c.getR()) * percent);
		  cache[cache_size].c.setG(copy[i].c.getG() + (copy[j].c.getG() - copy[i].c.getG()) * percent);
		  cache[cache_size].c.setB(copy[i].c.getB() + (copy[j].c.getB() - copy[i].c.getB()) * percent);
		  cache[cache_size].c.setAlpha(copy[i].c.getAlpha() + (copy[j].c.getAlpha() - copy[i].c.getAlpha()) * percent);
		  cache_size++;
	       }
	  }
	else
	  {
	     if(copy[j].v[1] >= viewport.vtrans[1] - viewport.vscale[1])
	       {
		  float percent = (viewport.vtrans[1] - viewport.vscale[1] - copy[j].v[1]) / (copy[i].v[1] - copy[j].v[1]);
		  cache[cache_size].v[0] = copy[j].v[0] + (copy[i].v[0] - copy[j].v[0]) * percent;
		  cache[cache_size].v[1] = viewport.vtrans[1] - viewport.vscale[1];
		  cache[cache_size].v[2] = copy[j].v[2] + (copy[i].v[2] - copy[j].v[2]) * percent;
		  cache[cache_size].v[3] = 1.0f / (1.0f/copy[j].v[3] + (1.0f/copy[i].v[3] - 1.0f/copy[j].v[3]) * percent);
		  cache[cache_size].s = (copy[j].s/copy[j].v[3] + (copy[i].s/copy[i].v[3] - copy[j].s/copy[j].v[3]) * percent)*cache[cache_size].v[3];
		  cache[cache_size].t = (copy[j].t/copy[j].v[3] + (copy[i].t/copy[i].v[3] - copy[j].t/copy[j].v[3]) * percent)*cache[cache_size].v[3];
		  cache[cache_size].c.setR(copy[j].c.getR() + (copy[i].c.getR() - copy[j].c.getR()) * percent);
		  cache[cache_size].c.setG(copy[j].c.getG() + (copy[i].c.getG() - copy[j].c.getG()) * percent);
		  cache[cache_size].c.setB(copy[j].c.getB() + (copy[i].c.getB() - copy[j].c.getB()) * percent);
		  cache[cache_size].c.setAlpha(copy[j].c.getAlpha() + (copy[i].c.getAlpha() - copy[j].c.getAlpha()) * percent);
		  cache_size++;
		  
		  cache[cache_size++] = copy[j];
	       }
	  }
     }
   
   // clip ymax
   copy_size = 0;
   
   for(int i=0; i<cache_size; i++)
     {
	int j = i+1;
	if(i == cache_size-1) j = 0;
	
	if(cache[i].v[1] < viewport.vtrans[1] + viewport.vscale[1])
	  {
	     if(cache[j].v[1] < viewport.vtrans[1] + viewport.vscale[1])
	       copy[copy_size++] = cache[j];
	     else
	       {
		  float percent = (viewport.vtrans[1] + viewport.vscale[1] - cache[i].v[1]) / (cache[j].v[1] - cache[i].v[1]);
		  copy[copy_size].v[0] = cache[i].v[0] + (cache[j].v[0] - cache[i].v[0]) * percent;
		  copy[copy_size].v[1] = viewport.vtrans[1] + viewport.vscale[1];
		  copy[copy_size].v[2] = cache[i].v[2] + (cache[j].v[2] - cache[i].v[2]) * percent;
		  copy[copy_size].v[3] = 1.0f / (1.0f/cache[i].v[3] + (1.0f/cache[j].v[3] - 1.0f/cache[i].v[3]) * percent);
		  copy[copy_size].s = (cache[i].s/cache[i].v[3] + (cache[j].s/cache[j].v[3] - cache[i].s/cache[i].v[3]) * percent)*copy[copy_size].v[3];
		  copy[copy_size].t = (cache[i].t/cache[i].v[3] + (cache[j].t/cache[j].v[3] - cache[i].t/cache[i].v[3]) * percent)*copy[copy_size].v[3];
		  copy[copy_size].c.setR(cache[i].c.getR() + (cache[j].c.getR() - cache[i].c.getR()) * percent);
		  copy[copy_size].c.setG(cache[i].c.getG() + (cache[j].c.getG() - cache[i].c.getG()) * percent);
		  copy[copy_size].c.setB(cache[i].c.getB() + (cache[j].c.getB() - cache[i].c.getB()) * percent);
		  copy[copy_size].c.setAlpha(cache[i].c.getAlpha() + (cache[j].c.getAlpha() - cache[i].c.getAlpha()) * percent);
		  copy_size++;
	       }
	  }
	else
	  {
	     if(cache[j].v[1] < viewport.vtrans[1] + viewport.vscale[1])
	       {
		  float percent = (viewport.vtrans[1] + viewport.vscale[1] - cache[j].v[1]) / (cache[i].v[1] - cache[j].v[1]);
		  copy[copy_size].v[0] = cache[j].v[0] + (cache[i].v[0] - cache[j].v[0]) * percent;
		  copy[copy_size].v[1] = viewport.vtrans[1] + viewport.vscale[1];
		  copy[copy_size].v[2] = cache[j].v[2] + (cache[i].v[2] - cache[j].v[2]) * percent;
		  copy[copy_size].v[3] = 1.0f / (1.0f/cache[j].v[3] + (1.0f/cache[i].v[3] - 1.0f/cache[j].v[3]) * percent);
		  copy[copy_size].s = (cache[j].s/cache[j].v[3] + (cache[i].s/cache[i].v[3] - cache[j].s/cache[j].v[3]) * percent)*copy[copy_size].v[3];
		  copy[copy_size].t = (cache[j].t/cache[j].v[3] + (cache[i].t/cache[i].v[3] - cache[j].t/cache[j].v[3]) * percent)*copy[copy_size].v[3];
		  copy[copy_size].c.setR(cache[j].c.getR() + (cache[i].c.getR() - cache[j].c.getR()) * percent);
		  copy[copy_size].c.setG(cache[j].c.getG() + (cache[i].c.getG() - cache[j].c.getG()) * percent);
		  copy[copy_size].c.setB(cache[j].c.getB() + (cache[i].c.getB() - cache[j].c.getB()) * percent);
		  copy[copy_size].c.setAlpha(cache[j].c.getAlpha() + (cache[i].c.getAlpha() - cache[j].c.getAlpha()) * percent);
		  copy_size++;
		  
		  copy[copy_size++] = cache[j];
	       }
	  }
     }
   
   if (zbuffer)
     {
	// clip zmin
	cache_size = 0;
	
	for(int i=0; i<copy_size; i++)
	  {
	     int j = i+1;
	     if(i == copy_size-1) j = 0;
	     
	     if(copy[i].v[2] >= (viewport.vtrans[2] - viewport.vscale[2])*32.0f)
	       {
		  if(copy[j].v[2] >= (viewport.vtrans[2] - viewport.vscale[2])*32.0f)
		    cache[cache_size++] = copy[j];
		  else
		    {
		       float percent = ((viewport.vtrans[2] - viewport.vscale[2])*32.0f - copy[i].v[2]) / (copy[j].v[2] - copy[i].v[2]);
		       cache[cache_size].v[0] = copy[i].v[0] + (copy[j].v[0] - copy[i].v[0]) * percent;
		       cache[cache_size].v[1] = copy[i].v[1] + (copy[j].v[1] - copy[i].v[1]) * percent;
		       cache[cache_size].v[2] = (viewport.vtrans[2] - viewport.vscale[2])*32.0f;
		       cache[cache_size].v[3] = 1.0f / (1.0f/copy[i].v[3] + (1.0f/copy[j].v[3] - 1.0f/copy[i].v[3]) * percent);
		       cache[cache_size].s = (copy[i].s/copy[i].v[3] + (copy[j].s/copy[j].v[3] - copy[i].s/copy[i].v[3]) * percent)*cache[cache_size].v[3];
		       cache[cache_size].t = (copy[i].t/copy[i].v[3] + (copy[j].t/copy[j].v[3] - copy[i].t/copy[i].v[3]) * percent)*cache[cache_size].v[3];
		       cache[cache_size].c.setR(copy[i].c.getR() + (copy[j].c.getR() - copy[i].c.getR()) * percent);
		       cache[cache_size].c.setG(copy[i].c.getG() + (copy[j].c.getG() - copy[i].c.getG()) * percent);
		       cache[cache_size].c.setB(copy[i].c.getB() + (copy[j].c.getB() - copy[i].c.getB()) * percent);
		       cache[cache_size].c.setAlpha(copy[i].c.getAlpha() + (copy[j].c.getAlpha() - copy[i].c.getAlpha()) * percent);
		       cache_size++;
		    }
	       }
	     else
	       {
		  if(copy[j].v[2] >= (viewport.vtrans[2] - viewport.vscale[2])*32.0f)
		    {
		       float percent = ((viewport.vtrans[2] - viewport.vscale[2])*32.0f - copy[j].v[2]) / (copy[i].v[2] - copy[j].v[2]);
		       cache[cache_size].v[0] = copy[j].v[0] + (copy[i].v[0] - copy[j].v[0]) * percent;
		       cache[cache_size].v[1] = copy[j].v[1] + (copy[i].v[1] - copy[j].v[1]) * percent;
		       cache[cache_size].v[2] = (viewport.vtrans[2] - viewport.vscale[2])*32.0f;
		       cache[cache_size].v[3] = 1.0f / (1.0f/copy[j].v[3] + (1.0f/copy[i].v[3] - 1.0f/copy[j].v[3]) * percent);
		       cache[cache_size].s = (copy[j].s/copy[j].v[3] + (copy[i].s/copy[i].v[3] - copy[j].s/copy[j].v[3]) * percent)*cache[cache_size].v[3];
		       cache[cache_size].t = (copy[j].t/copy[j].v[3] + (copy[i].t/copy[i].v[3] - copy[j].t/copy[j].v[3]) * percent)*cache[cache_size].v[3];
		       cache[cache_size].c.setR(copy[j].c.getR() + (copy[i].c.getR() - copy[j].c.getR()) * percent);
		       cache[cache_size].c.setG(copy[j].c.getG() + (copy[i].c.getG() - copy[j].c.getG()) * percent);
		       cache[cache_size].c.setB(copy[j].c.getB() + (copy[i].c.getB() - copy[j].c.getB()) * percent);
		       cache[cache_size].c.setAlpha(copy[j].c.getAlpha() + (copy[i].c.getAlpha() - copy[j].c.getAlpha()) * percent);
		       cache_size++;
		       
		       cache[cache_size++] = copy[j];
		    }
	       }
	  }
	
	// clip zmax
	copy_size = 0;
	
	for(int i=0; i<cache_size; i++)
	  {
	     int j = i+1;
	     if(i == cache_size-1) j = 0;
	     
	     if(cache[i].v[2] < (viewport.vtrans[2] + viewport.vscale[2])*32.0f)
	       {
		  if(cache[j].v[2] < (viewport.vtrans[2] + viewport.vscale[2])*32.0f)
		    copy[copy_size++] = cache[j];
		  else
		    {
		       float percent = ((viewport.vtrans[2] + viewport.vscale[2])*32.0f - cache[i].v[2]) / (cache[j].v[2] - cache[i].v[2]);
		       copy[copy_size].v[0] = cache[i].v[0] + (cache[j].v[0] - cache[i].v[0]) * percent;
		       copy[copy_size].v[1] = cache[i].v[1] + (cache[j].v[1] - cache[i].v[1]) * percent;
		       copy[copy_size].v[2] = (viewport.vtrans[2] + viewport.vscale[2])*32.0f;
		       copy[copy_size].v[3] = 1.0f / (1.0f/cache[i].v[3] + (1.0f/cache[j].v[3] - 1.0f/cache[i].v[3]) * percent);
		       copy[copy_size].s = (cache[i].s/cache[i].v[3] + (cache[j].s/cache[j].v[3] - cache[i].s/cache[i].v[3]) * percent)*copy[copy_size].v[3];
		       copy[copy_size].t = (cache[i].t/cache[i].v[3] + (cache[j].t/cache[j].v[3] - cache[i].t/cache[i].v[3]) * percent)*copy[copy_size].v[3];
		       copy[copy_size].c.setR(cache[i].c.getR() + (cache[j].c.getR() - cache[i].c.getR()) * percent);
		       copy[copy_size].c.setG(cache[i].c.getG() + (cache[j].c.getG() - cache[i].c.getG()) * percent);
		       copy[copy_size].c.setB(cache[i].c.getB() + (cache[j].c.getB() - cache[i].c.getB()) * percent);
		       copy[copy_size].c.setAlpha(cache[i].c.getAlpha() + (cache[j].c.getAlpha() - cache[i].c.getAlpha()) * percent);
		       copy_size++;
		    }
	       }
	     else
	       {
		  if(cache[j].v[2] < (viewport.vtrans[2] + viewport.vscale[2])*32.0f)
		    {
		       float percent = ((viewport.vtrans[2] + viewport.vscale[2])*32.0f - cache[j].v[2]) / (cache[i].v[2] - cache[j].v[2]);
		       copy[copy_size].v[0] = cache[j].v[0] + (cache[i].v[0] - cache[j].v[0]) * percent;
		       copy[copy_size].v[1] = cache[j].v[1] + (cache[i].v[1] - cache[j].v[1]) * percent;
		       copy[copy_size].v[2] = (viewport.vtrans[2] + viewport.vscale[2])*32.0f;
		       copy[copy_size].v[3] = 1.0f / (1.0f/cache[j].v[3] + (1.0f/cache[i].v[3] - 1.0f/cache[j].v[3]) * percent);
		       copy[copy_size].s = (cache[j].s/cache[j].v[3] + (cache[i].s/cache[i].v[3] - cache[j].s/cache[j].v[3]) * percent)*copy[copy_size].v[3];
		       copy[copy_size].t = (cache[j].t/cache[j].v[3] + (cache[i].t/cache[i].v[3] - cache[j].t/cache[j].v[3]) * percent)*copy[copy_size].v[3];
		       copy[copy_size].c.setR(cache[j].c.getR() + (cache[i].c.getR() - cache[j].c.getR()) * percent);
		       copy[copy_size].c.setG(cache[j].c.getG() + (cache[i].c.getG() - cache[j].c.getG()) * percent);
		       copy[copy_size].c.setB(cache[j].c.getB() + (cache[i].c.getB() - cache[j].c.getB()) * percent);
		       copy[copy_size].c.setAlpha(cache[j].c.getAlpha() + (cache[i].c.getAlpha() - cache[j].c.getAlpha()) * percent);
		       copy_size++;
		       
		       copy[copy_size++] = cache[j];
		    }
	       }
	  }
     }
   
   cache_size = 0;
   
   for (int i=0; i<copy_size; i++)
     cache[cache_size++] = copy[i];
   
   int a = 0, b = 1;
   
   for(int i=2; i<cache_size; i++)
     {
	Vektor<float,4> vx0, vx1, vx2;
	vx0[0] = cache[a].v[0];
	vx0[1] = cache[a].v[1];
	vx1[0] = cache[b].v[0];
	vx1[1] = cache[b].v[1];
	vx2[0] = cache[i].v[0];
	vx2[1] = cache[i].v[1];
   
	float z0=0.0f, z1=0.0f, z2=0.0f;
	if (zbuffer)
	  {
	     z0 = cache[a].v[2];
	     z1 = cache[b].v[2];
	     z2 = cache[i].v[2];
	  }
	
	if(fog)
	  {
	     float alpha0 = ((z0 / 32.0f - viewport.vtrans[2]) / viewport.vscale[2]) * fm + fo;
	     float alpha1 = ((z1 / 32.0f - viewport.vtrans[2]) / viewport.vscale[2]) * fm + fo;
	     float alpha2 = ((z2 / 32.0f - viewport.vtrans[2]) / viewport.vscale[2]) * fm + fo;
	     if(alpha0 < 0) alpha0 = 0;
	     if(alpha1 < 0) alpha1 = 0;
	     if(alpha2 < 0) alpha2 = 0;
	     if(alpha0 > 255) alpha0 = 255;
	     if(alpha1 > 255) alpha1 = 255;
	     if(alpha2 > 255) alpha2 = 255;
	     cache[a].c.setAlpha(alpha0);
	     cache[b].c.setAlpha(alpha1);
	     cache[i].c.setAlpha(alpha2);
	  }
	
	switch(geometryMode)
	  {
	   case 0x5:   // shade | z_buffer
	   case 0x10005: // fog | shade | z_buffer
	     if (textureScales.enabled)
	       rdp->tri_shade_txtr_zbuff(vx0, vx1, vx2, cache[a].c, cache[a].c, cache[a].c,
					 cache[a].s, cache[a].t, cache[b].s, cache[b].t, cache[i].s, cache[i].t, textureScales.tile,
					 cache[a].v[3], cache[b].v[3], cache[i].v[3], z0, z1, z2);
	     else
	       rdp->tri_shade_zbuff(vx0, vx1, vx2, cache[a].c, cache[a].c, cache[a].c, z0, z1, z2);
	     break;
	   case 0x204: // shading_smooth | shade
	     if (textureScales.enabled)
	       rdp->tri_shade_txtr(vx0, vx1, vx2, cache[a].c, cache[b].c, cache[i].c,
				   cache[a].s, cache[a].t, cache[b].s, cache[b].t, cache[i].s, cache[i].t, textureScales.tile,
				   cache[a].v[3], cache[b].v[3], cache[i].v[3]);
	     else
	       rdp->tri_shade(vx0, vx1, vx2, cache[a].c, cache[b].c, cache[i].c);
	     break;
	   case 0x205: // shading_smooth | shade | z_buffer
	   case 0x10205: // fog | shading_smooth | shade | z_buffer
	     if (textureScales.enabled)
	       rdp->tri_shade_txtr_zbuff(vx0, vx1, vx2, cache[a].c, cache[b].c, cache[i].c, 
					 cache[a].s, cache[a].t, cache[b].s, cache[b].t, cache[i].s, cache[i].t, textureScales.tile,
					 cache[a].v[3], cache[b].v[3], cache[i].v[3], z0, z1, z2);
	     else
	       rdp->tri_shade_zbuff(vx0, vx1, vx2, cache[a].c, cache[b].c, cache[i].c, z0, z1, z2);
	     break;
	   default:
	     printf("RSP:tri1 unknown geometry mode:%x\n", geometryMode);
	     //getchar();
	     rdp->debug_tri(vx0, vx1, vx2);
	  }
	//rdp->debug_tri(vx0, vx1, vx2);
	
	b = i;
     }
}

void RSP::TEXRECT()
{
   float ulx, uly, lrx, lry, s, t, dsdx, dtdy;
   lrx = ((*currentCommand >> 12) & 0xFFF) / 4.0f;
   lry = (*currentCommand & 0xFFF) / 4.0f;
   int tile = (*(currentCommand+1) >> 24) & 7;
   ulx = ((*(currentCommand+1) >> 12) & 0xFFF) / 4.0f;
   uly = (*(currentCommand+1) & 0xFFF) / 4.0f;
   s = (int)((short)((*(currentCommand+3) >> 16) & 0xFFFF)) / 32.0f;
   t = (int)((short)(*(currentCommand+3) & 0xFFFF)) / 32.0f;
   dsdx = (int)((short)((*(currentCommand+5) >> 16) & 0xFFFF)) / 1024.0f;
   dtdy = (int)((short)(*(currentCommand+5) & 0xFFFF)) / 1024.0f;
   rdp->texRect(tile, ulx, uly, lrx, lry, s, t, dsdx, dtdy);
   currentCommand += 4;
}

void RSP::RDPLOADSYNC()
{
}

void RSP::RDPPIPESYNC()
{
}

void RSP::RDPTILESYNC()
{
}

void RSP::RDPFULLSYNC()
{
}

void RSP::SETSCISSOR()
{
   float ulx, uly, lrx, lry;
   ulx = ((*currentCommand>>12) & 0xFFF) / 4.0f;
   uly = (*currentCommand & 0xFFF) / 4.0f;
   lrx = ((*(currentCommand+1)>>12) & 0xFFF) / 4.0f;
   lry = (*(currentCommand+1) & 0xFFF) / 4.0f;
   int mode = (*(currentCommand+1)>>24) & 3;
   rdp->setScissor(ulx, uly, lrx, lry, mode);
}

void RSP::LOADTLUT()
{
   int tile = (*(currentCommand+1) >> 24) & 7;
   int count = ((*(currentCommand+1) >> 14) & 0x3FF)+1;
   rdp->loadTLUT(tile, count);
}

void RSP::SETTILESIZE()
{
   float uls, ult, lrs, lrt;
   uls = ((*currentCommand>>12) & 0xFFF) / 4.0f;
   ult = (*currentCommand & 0xFFF) / 4.0f;
   lrs = ((*(currentCommand+1)>>12) & 0xFFF) / 4.0f;
   lrt = (*(currentCommand+1) & 0xFFF) / 4.0f;
   int tile = (*(currentCommand+1)>>24) & 7;
   rdp->setTileSize(uls, ult, lrs, lrt, tile);
}

void RSP::LOADBLOCK()
{
   float uls, ult, lrs;
   uls = ((*currentCommand >> 12) & 0xFFF) / 4.0f;
   ult = (*currentCommand & 0xFFF) / 4.0f;
   int tile = (*(currentCommand+1) >> 24) & 7;
   lrs = ((*(currentCommand+1) >> 12) & 0xFFF) / 4.0f;
   int dxt = *(currentCommand+1) & 0xFFF;
   rdp->loadBlock(uls, ult, tile, lrs, dxt);
}

void RSP::LOADTILE()
{
   float uls, ult, lrs, lrt;
   uls = ((*currentCommand >> 12) & 0xFFF) / 4.0f;
   ult = (*currentCommand & 0xFFF) / 4.0f;
   int tile = (*(currentCommand+1) >> 24) & 7;
   lrs = ((*(currentCommand+1) >> 12) & 0xFFF) / 4.0f;
   lrt = (*(currentCommand+1) & 0xFFF) / 4.0f;
   rdp->loadTile(tile, uls, ult, lrs, lrt);
}

void RSP::SETTILE()
{
   int format = (*currentCommand >> 21) & 7;
   int size = (*currentCommand >> 19) & 3;
   int line = (*currentCommand >> 9) & 0x1FF;
   int tmem = *currentCommand & 0x1FF;
   int tile = (*(currentCommand+1) >> 24) & 7;
   int palette = (*(currentCommand+1) >> 20) & 0xF;
   int cmt = (*(currentCommand+1) >> 18) & 3;
   int maskt = (*(currentCommand+1) >> 14) & 0xF;
   int shiftt = (*(currentCommand+1) >> 10) & 0xF;
   int cms = (*(currentCommand+1) >> 8) & 3;
   int masks = (*(currentCommand+1) >> 4) & 0xF;
   int shifts = *(currentCommand+1) & 0xF;
   rdp->setTile(format, size, line, tmem, tile, palette, cmt, maskt, shiftt, cms, masks, shifts);
}

void RSP::FILLRECT()
{
   float ulx, uly, lrx, lry;
   ulx = ((*(currentCommand+1) >> 12) & 0xFFF) / 4.0f;
   uly = (*(currentCommand+1) & 0xFFF) / 4.0f;
   lrx = ((*currentCommand >> 12) & 0xFFF) / 4.0f;
   lry = (*currentCommand & 0xFFF) / 4.0f;
   rdp->fillRect(ulx, uly, lrx, lry);
}

void RSP::SETFILLCOLOR()
{
   rdp->setFillColor(*(currentCommand+1));
}

void RSP::SETFOGCOLOR()
{
   rdp->setFogColor(*(currentCommand+1));
}

void RSP::SETBLENDCOLOR()
{
   rdp->setBlendColor(*(currentCommand+1));
}

void RSP::SETPRIMCOLOR()
{
   float mLOD, lLOD;
   mLOD = ((*currentCommand >> 8) & 0xFF) / 256.0f;
   lLOD = ((*currentCommand) & 0xFF) / 256.0f;
   rdp->setPrimColor(*(currentCommand+1), mLOD, lLOD);
}

void RSP::SETENVCOLOR()
{
   rdp->setEnvColor(*(currentCommand+1));
}

void RSP::SETCOMBINE()
{
   rdp->setCombineMode(*currentCommand & 0xFFFFFF, *(currentCommand+1));
}

void RSP::SETTIMG()
{
   int format = (*currentCommand >> 21) & 0x7;
   int size = (*currentCommand >> 19) & 0x3;
   int width = (*currentCommand & 0xFFF) + 1;
   void *timg = gfxInfo.RDRAM + (seg2phys(*(currentCommand+1)) & 0x7FFFFF);
   rdp->setTImg(format, size, width, timg);
}

void RSP::SETZIMG()
{
   void *zimg = gfxInfo.RDRAM + (seg2phys(*(currentCommand+1)) & 0x7FFFFF);
   rdp->setZImg(zimg);
}

void RSP::SETCIMG()
{
   int format = (*currentCommand >> 21) & 0x7;
   int size = (*currentCommand >> 19) & 0x3;
   int width = (*currentCommand & 0xFFF) + 1;
   void *cimg = gfxInfo.RDRAM + (seg2phys(*(currentCommand+1)) & 0x7FFFFF);
   rdp->setCImg(format, size, width, cimg);
}
