/* rsp_GX.cpp - N64 GX plugin, based off Hacktarux's soft_gfx
   by sepp256 for Mupen64-GC
 */

//#include <cstdio.h>
#include <stdio.h>
#include <math.h>

#include "global.h"
#include "rsp_GX.h"

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
   //TODO: Set these flags in GX, too?
   GX_SetCullMode (GX_CULL_NONE); // default in rsp init

   light_mask = GX_LIGHTNULL;
   GX_SetChanCtrl(GX_COLOR0A0,GX_DISABLE,GX_SRC_REG,GX_SRC_VTX,light_mask,GX_DF_CLAMP,GX_AF_NONE);
   GX_SetZMode(GX_DISABLE,GX_GEQUAL,GX_TRUE);



   //rdp = new RDP(info);
   
   executeDList();
}

RSP::~RSP()
{
   //delete rdp;
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
	//TODO: use matrix memory in place of stack
   unsigned long addr = seg2phys(*(currentCommand+1)) & 0x7FFFFF;
   int op = (*currentCommand >> 16) & 0xFF;
   bool updateprojection = false;

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
		 updateprojection = true;
	  }
	break;
      case 2: // MODELVIEW LOAD NOPUSH
	modelView = matrix;
	break;
      case 3: // PROJECTION LOAD NOPUSH
	  {
	     projection = matrix;
	     updateprojection = true;
	  }
	break;
      case 4: // MODELVIEW MUL PUSH
	  {
	     modelView.push();
	     Matrix<float,4> m = matrix * modelView;
	     modelView = m;
	  }
	break;
      case 6: // MODELVIEW LOAD PUSH
	  {
	modelView.push();
	modelView = matrix;
	  }
	break;
      default:
	printf("RSP: unknown MTX:%d\n", op);
	error = true;
     }
   MP = modelView * projection;

   //Send new modelView, projection, or normal matrices to GX
   if (updateprojection == true)
   {
      for (int j=0; j<4; j++)
      {
         for (int i=0; i<4; i++)
         {
            GXprojection[j][i] = projection(i,j);
//			printf("p[%i][%i] = %f\n",j,i,projection(j,i));
         }
	  }
	  int j = 2;
	  for (int i=0; i<4; i++)	//N64 Z clip space is backwards, so change p accordingly
	  {
		  GXprojection[j][i] = -GXprojection[j][i];
	  }
	  if(GXprojection[3][2] == -1) 
	  {
		  GX_LoadProjectionMtx(GXprojection, GX_PERSPECTIVE); 
//		  printf("Send Perspective Projection Matrix to GX\n");
	  }
	  else 
	  {
		  GX_LoadProjectionMtx(GXprojection, GX_ORTHOGRAPHIC); 
//		  printf("Send Orthographic Projection Matrix to GX\n");
	  }
/*	  printf("\tPrj =\t%f, %f, %f, %f\n",GXprojection[0][0],GXprojection[0][1],GXprojection[0][2],GXprojection[0][3]);
	  printf("\t     \t%f, %f, %f, %f\n",GXprojection[1][0],GXprojection[1][1],GXprojection[1][2],GXprojection[1][3]);
	  printf("\t     \t%f, %f, %f, %f\n",GXprojection[2][0],GXprojection[2][1],GXprojection[2][2],GXprojection[2][3]);
	  printf("\t     \t%f, %f, %f, %f\n",GXprojection[3][0],GXprojection[3][1],GXprojection[3][2],GXprojection[3][3]);*/
   }
   else
   {
      for (int j=0; j<3; j++)
      {
         for (int i=0; i<4; i++)
         {
            GXmodelView[j][i] = modelView(i,j);
//			printf("mv[%i][%i] = %f\n",j,i,modelView(j,i));
         }
	  }
      GX_LoadPosMtxImm(GXmodelView, GX_PNMTX0);
	  if(guMtxInverse(GXmodelView, GXnormal) == 0)	// normal matrix is the inverse transpose of the modelview matrix
		  printf("Normal matrix is singular\n");
	  guMtxTranspose(GXnormal, GXnormal);
      GX_LoadNrmMtxImm(GXnormal, GX_PNMTX0);
//	  printf("Send Normal Matrix to GX\n");
/*	  printf("\tMV =\t%f, %f, %f, %f\n",GXmodelView[0][0],GXmodelView[0][1],GXmodelView[0][2],GXmodelView[0][3]);
	  printf("\t\t%f, %f, %f, %f\n",GXmodelView[1][0],GXmodelView[1][1],GXmodelView[1][2],GXmodelView[1][3]);
	  printf("\t\t%f, %f, %f, %f\n",GXmodelView[2][0],GXmodelView[2][1],GXmodelView[2][2],GXmodelView[2][3]);*/
   }
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
	//TODO: Determine if viewport should be readusted here
	//Vscale and Voffset should be calculated automatically by GX based on viewport...
//	printf("viewport is adjusted.\n");
//	printf("vscale = %f, %f, %f, %f\n",viewport.vscale[0],viewport.vscale[1],viewport.vscale[2],viewport.vscale[3]);
//	printf("vtrans = %f, %f, %f, %f\n",viewport.vtrans[0],viewport.vtrans[1],viewport.vtrans[2],viewport.vtrans[3]);
	break;
      case 0x82:
	lookAtY.col = *((int*)(gfxInfo.RDRAM + addr) + 0);
	lookAtY.colc= *((int*)(gfxInfo.RDRAM + addr) + 1);
	lookAtY.dir[0] = *((char*)gfxInfo.RDRAM + addr + (8^S8)) / 128.0f;
	lookAtY.dir[1] = *((char*)gfxInfo.RDRAM + addr + (9^S8)) / 128.0f;
	lookAtY.dir[2] = *((char*)gfxInfo.RDRAM + addr + (10^S8)) / 128.0f;
	lookAtY.dir[3] = *((char*)gfxInfo.RDRAM + addr + (11^S8)) / 128.0f;
	//lookAtY doesn't seem to be used by soft_gfx?
	//printf("lookAtY is doing something.\n");
	break;
      case 0x84:
	lookAtX.col = *((int*)(gfxInfo.RDRAM + addr) + 0);
	lookAtX.colc= *((int*)(gfxInfo.RDRAM + addr) + 1);
	lookAtX.dir[0] = *((char*)gfxInfo.RDRAM + addr + (8^S8)) / 128.0f;
	lookAtX.dir[1] = *((char*)gfxInfo.RDRAM + addr + (9^S8)) / 128.0f;
	lookAtX.dir[2] = *((char*)gfxInfo.RDRAM + addr + (10^S8)) / 128.0f;
	lookAtX.dir[3] = *((char*)gfxInfo.RDRAM + addr + (11^S8)) / 128.0f;
	//lookAtX doesn't seem to be used by soft_gfx?
	//printf("lookAtX is doing something.\n");
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
		  //init light color
		  GXcol.r = (u8)spotLight[n].col.getR();
		  GXcol.g = (u8)spotLight[n].col.getG();
		  GXcol.b = (u8)spotLight[n].col.getB();
		  GXcol.a = (u8)spotLight[n].col.getAlpha();
		  GX_InitLightColor(&GXlight,GXcol);
		  //init light direction (normalized & transformed by modelview matrix)
		  GX_InitLightDir(&GXlight,spotLight[n].dir[0],spotLight[n].dir[1],spotLight[n].dir[2]);
		  //no light position specified
		  //TODO: Calculate position at infinity??
		  //update light_mask
		  light_mask = light_mask | (GX_LIGHT0<<n);
		  //add this light to color channel 0
		  GX_SetChanCtrl(GX_COLOR0A0,GX_ENABLE,GX_SRC_REG,GX_SRC_VTX,light_mask,GX_DF_CLAMP,GX_AF_NONE);
	       }
	     else
	       {
		  ambientLight.col = *((int*)(gfxInfo.RDRAM + addr) + 0);
		  ambientLight.colc= *((int*)(gfxInfo.RDRAM + addr) + 1);
		  //set ambient light
		  GXcol.r = (u8)ambientLight.col.getR();
		  GXcol.g = (u8)ambientLight.col.getG();
		  GXcol.b = (u8)ambientLight.col.getB();
		  GXcol.a = (u8)ambientLight.col.getAlpha();
		  GX_SetChanAmbColor(GX_COLOR0A0,GXcol);
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
	//vtx[v0+i].v = vtx[v0+i].v * MP;
	
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
	     //vtx[v0+i].n = vtx[v0+i].n * modelView;
	     vtx[v0+i].n.normalize();
	     
	     // This should now be calculated by GX
/*		 vtx[v0+i].c = ambientLight.col;
	     for (int j=0; j<numLight; j++)
	       {
		  float cosT = vtx[v0+i].n.scalar(spotLight[j].dir);
		  if (cosT > 0.0f)
		    vtx[v0+i].c += spotLight[j].col*cosT;
	       }
	     vtx[v0+i].c.clamp();
	     vtx[v0+i].c.setAlpha(*((unsigned char*)(p + i*16 + (15^S8))));
*/	     

	     if (texture_gen)
	       {
		  vtx[v0+i].s = (asinf(vtx[v0+i].n[0])/3.14f + 0.5f) * textureScales.sc * 1024.0f;
		  vtx[v0+i].t = (asinf(vtx[v0+i].n[1])/3.14f + 0.5f) * textureScales.tc * 1024.0f;
		  //TODO: assign t & s to vertex here?
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
   
   printf("Sprite2D call\n");

   if (SourceImageBitSize == 0)
     printf("RSP:SPRITE2D image type=%d bitsize=%d\n", SourceImageType, SourceImageBitSize);
   if (FlipTextureX || FlipTextureY)
     printf("RSP:SPRITE2D flip\n");
   
   if (SourceImageType == 2)
     {
	// loading the TLUT
/*	rdp->setOtherMode_h(14, 2);
	rdp->setTImg(0, 2, 1, TlutPointer);
	rdp->setTile(0, 0, 0, 256, 0, 0, 0, 0, 0, 0, 0, 0);
	rdp->loadTLUT(0, 255);*/
     }
   
   int tOffs, oldTOffs;
   tOffs = oldTOffs = SourceImageOffsetT;
   
/*   rdp->setOtherMode_h(20,0);
   rdp->setCombineMode(0xffffff, 0xfffcf3ff);
   rdp->setOtherMode_l(3, 0x0c087008>>3);
   rdp->setTImg(SourceImageType, SourceImageBitSize, Stride, SourceImagePointer);
   rdp->setTile(SourceImageType, SourceImageBitSize, (Stride*SourceImageBitSize)/8, 0, 0, 0, 0, 0, 0, 0, 0, 0);*/
   
   while(tOffs != (SourceImageOffsetT + SubImageHeight))
     {
	if ((tOffs-oldTOffs+2)*Stride*SourceImageBitSize < 2048)
	  tOffs++;
	else
	  {
	     float top = oldTOffs;
	     float bottom = tOffs;
	     //rdp->setTileSize(uls, top, lrs, bottom, 0);
	     //rdp->loadTile(0, uls, top, lrs, bottom);
	     
	     float uly = pScreenY+(oldTOffs-SourceImageOffsetT)*ScaleY;
	     float lry = pScreenY+(tOffs-SourceImageOffsetT)*ScaleY;
	     //rdp->texRect(0, ulx, uly, lrx, lry, uls, top, ScaleX, ScaleY);
	     
	     oldTOffs = tOffs++;
	  }
     }
}

void RSP::CLEARGEOMETRYMODE()
{
   int mode = *(currentCommand+1);
   
   if (mode & 0x80000) texture_gen_linear = false;
   if (mode & 0x40000) texture_gen = false;
   if (mode & 0x20000) 
   {
	   lighting = false;
	   light_mask = GX_LIGHTNULL;
	   GX_SetChanCtrl(GX_COLOR0A0,GX_DISABLE,GX_SRC_REG,GX_SRC_VTX,light_mask,GX_DF_CLAMP,GX_AF_NONE);
   }
   if (mode & 0x10000) fog = false;
   if (mode & 0x2000) cull_back = false;
   if (mode & 0x1000) cull_front = false;
   if (mode & 0x200) shading_smooth = false;
   if (mode & 0x4) shade = false;
   if (mode & 0x1) 
   {
	   zbuffer = false;
	   GX_SetZMode(GX_DISABLE,GX_LEQUAL,GX_TRUE);
   }

   
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
   if (mode & 0x20000)
   {
	   lighting = true;
//	   printf("lighting turned on but not in GX\n");
	   GX_SetNumChans(1);
   }
   if (mode & 0x10000) fog = true;
   if (mode & 0x2000) cull_back = true;
   if (mode & 0x200) shading_smooth = true;
   if (mode & 0x4) shade = true;
   if (mode & 0x1) 
   {
	   zbuffer = true;
	   GX_SetZMode(GX_ENABLE,GX_GEQUAL,GX_TRUE);
   }
   
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
   
   //rdp->setOtherMode_l(mode, (data>>mode)&((1<<length)-1));
//   printf("Setothermode_l\n");
}

void RSP::SETOTHERMODE_H()
{
   int mode = (*currentCommand >> 8) & 0xFF;
   int length = *currentCommand & 0xFF;
   unsigned long data = *(currentCommand+1);
   
   //rdp->setOtherMode_h(mode, (data>>mode)&((1<<length)-1));
//   printf("setothermode_h\n");
}

void RSP::TEXTURE()
{
   int tile = (*currentCommand >> 8) & 3;
   textureScales.tile = tile;
   textureScales.sc = (int)((*(currentCommand+1)>>16)&0xFFFF) / 65536.0;
   textureScales.tc = (int)(*(currentCommand+1)&0xFFFF) / 65536.0;
   textureScales.level = (*currentCommand >> 11) & 3;
   textureScales.enabled = *currentCommand & 1 ? true : false;
//   printf("set tesxture scales\n");
}

void RSP::MOVEWORD()
{
   int index = *currentCommand & 0xFF;
   int offset = (*currentCommand >> 8) & 0xFFFF;
   
   switch(index)
     {
      case 0x2: // NUMLIGHT
	numLight = ((*(currentCommand+1)-0x80000000)/32)-1;
	light_mask = GX_LIGHTNULL;
	//TODO: enable these lights in GX here??
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
//   printf("Moveword\n");
}

void RSP::POPMTX()
{
   int type = *(currentCommand+1) & 0xFF;
   
//   if (type != 0) printf("POPMTX on projection matrix\n");
   modelView.pop();
   //TODO:  Also copy this to GX
      for (int j=0; j<3; j++)
      {
         for (int i=0; i<4; i++)
         {
            GXmodelView[j][i] = modelView(i,j);
//			printf("mv[%i][%i] = %f\n",j,i,modelView(j,i));
         }
	  }
      GX_LoadPosMtxImm(GXmodelView, GX_PNMTX0);
	  if(guMtxInverse(GXmodelView, GXnormal) == 0)
		  printf("Normal matrix is singular\n");
	  guMtxTranspose(GXnormal, GXnormal);
      GX_LoadNrmMtxImm(GXnormal, GX_PNMTX0);
//	  printf("Send Normal Matrix to GX\n");
}

void RSP::TRI1()
{
   int v1 = ((*(currentCommand+1) >> 16) & 0xFF) / 10; //TODO: Verify correct vertex ordering for GX
   int v2 = ((*(currentCommand+1) >> 8) & 0xFF) / 10;
   int v0 = (*(currentCommand+1) & 0xFF) / 10;
   
   /* All of this should be handled by GX
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
		   if (textureScales.enabled) {}
	       //rdp->tri_shade_txtr_zbuff(vx0, vx1, vx2, cache[a].c, cache[a].c, cache[a].c,
			//		 cache[a].s, cache[a].t, cache[b].s, cache[b].t, cache[i].s, cache[i].t, textureScales.tile,
			//		 cache[a].v[3], cache[b].v[3], cache[i].v[3], z0, z1, z2);
	     else {}
	       //rdp->tri_shade_zbuff(vx0, vx1, vx2, cache[a].c, cache[a].c, cache[a].c, z0, z1, z2);
	     break;
	   case 0x204: // shading_smooth | shade
	     if (textureScales.enabled) {}
	       //rdp->tri_shade_txtr(vx0, vx1, vx2, cache[a].c, cache[b].c, cache[i].c,
			//	   cache[a].s, cache[a].t, cache[b].s, cache[b].t, cache[i].s, cache[i].t, textureScales.tile,
			//	   cache[a].v[3], cache[b].v[3], cache[i].v[3]);
	     else {}
	       //rdp->tri_shade(vx0, vx1, vx2, cache[a].c, cache[b].c, cache[i].c);
	     break;
	   case 0x205: // shading_smooth | shade | z_buffer
	   case 0x10205: // fog | shading_smooth | shade | z_buffer
	     if (textureScales.enabled) {}
	       //rdp->tri_shade_txtr_zbuff(vx0, vx1, vx2, cache[a].c, cache[b].c, cache[i].c, 
			//		 cache[a].s, cache[a].t, cache[b].s, cache[b].t, cache[i].s, cache[i].t, textureScales.tile,
			//		 cache[a].v[3], cache[b].v[3], cache[i].v[3], z0, z1, z2);
	     else {}
	       //rdp->tri_shade_zbuff(vx0, vx1, vx2, cache[a].c, cache[b].c, cache[i].c, z0, z1, z2);
	     break;
	   default:
	     printf("RSP:tri1 unknown geometry mode:%x\n", geometryMode);
	     //getchar();
	     //rdp->debug_tri(vx0, vx1, vx2);
	  }
	//rdp->debug_tri(vx0, vx1, vx2);
	
	b = i;
     }*/
   //TODO: Set flag showing if any vector attributes have changed
   //TODO: Maybe make use of vertex attribute format table or use GX_SetVtxDescv
   //set vertex description here
   GX_ClearVtxDesc();
   GX_SetVtxDesc(GX_VA_PTNMTXIDX, GX_PNMTX0);
   //GX_SetVtxDesc(GX_VA_TEX0MTXIDX, ...);
   GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
   if (lighting) GX_SetVtxDesc(GX_VA_NRM, GX_DIRECT);
   GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
   //GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);

   //set vertex attribute formats here
   GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
   if (lighting) GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_NRM, GX_NRM_XYZ, GX_F32, 0);
   GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);

   //printf("Sending GX a triangle\n");

	//testing this: disable textures
     GX_SetNumChans (1);
  GX_SetNumTexGens (0);
  GX_SetTevOrder (GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
  GX_SetTevOp (GX_TEVSTAGE0, GX_PASSCLR);


   GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 3);
     // vert 0
     GX_Position3f32(vtx[v0].v[0], vtx[v0].v[1], vtx[v0].v[2]);
	 if (lighting) GX_Normal3f32(vtx[v0].n[0], vtx[v0].n[1], vtx[v0].n[2]);
	 GXcol.r = (u8)vtx[v0].c.getR();
	 GXcol.g = (u8)vtx[v0].c.getG();
	 GXcol.b = (u8)vtx[v0].c.getB();
	 GXcol.a = (u8)vtx[v0].c.getAlpha();
	 GX_Color4u8(GXcol.r, GXcol.g, GXcol.b, GXcol.a);
//	 printf("\tV0 pos = %f, %f, %f, %f\n",vtx[v0].v[0],vtx[v0].v[1],vtx[v0].v[2],vtx[v0].v[3]);
//	 printf("\tV0 clr = %d, %d, %d, %d\n", GXcol.r, GXcol.g, GXcol.b, GXcol.a);
     // vert 1
     GX_Position3f32(vtx[v1].v[0], vtx[v1].v[1], vtx[v1].v[2]);
	 if (lighting) GX_Normal3f32(vtx[v1].n[0], vtx[v1].n[1], vtx[v1].n[2]);
	 GXcol.r = (u8)vtx[v1].c.getR();
	 GXcol.g = (u8)vtx[v1].c.getG();
	 GXcol.b = (u8)vtx[v1].c.getB();
	 GXcol.a = (u8)vtx[v1].c.getAlpha();
	 GX_Color4u8(GXcol.r, GXcol.g, GXcol.b, GXcol.a);
//	 printf("\tV1 pos = %f, %f, %f, %f\n",vtx[v1].v[0],vtx[v1].v[1],vtx[v1].v[2],vtx[v1].v[3]);
//	 printf("\tV1 clr = %d, %d, %d, %d\n", GXcol.r, GXcol.g, GXcol.b, GXcol.a);

     // vert 2
     GX_Position3f32(vtx[v2].v[0], vtx[v2].v[1], vtx[v2].v[2]);
	 if (lighting) GX_Normal3f32(vtx[v2].n[0], vtx[v2].n[1], vtx[v2].n[2]);
	 GXcol.r = (u8)vtx[v2].c.getR();
	 GXcol.g = (u8)vtx[v2].c.getG();
	 GXcol.b = (u8)vtx[v2].c.getB();
	 GXcol.a = (u8)vtx[v2].c.getAlpha();
	 GX_Color4u8(GXcol.r, GXcol.g, GXcol.b, GXcol.a);
//	 printf("\tV2 pos = %f, %f, %f, %f\n",vtx[v2].v[0],vtx[v2].v[1],vtx[v2].v[2],vtx[v2].v[3]);
//	 printf("\tV2 clr = %d, %d, %d, %d\n", GXcol.r, GXcol.g, GXcol.b, GXcol.a);
   GX_End();

//   for (int i=0; i<0xFFFFFF; i++) 
//   {
//   }
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
   //rdp->texRect(tile, ulx, uly, lrx, lry, s, t, dsdx, dtdy);
   currentCommand += 4;
//   printf("trying to send textrec\n");
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
   //rdp->setScissor(ulx, uly, lrx, lry, mode);
   //not sure if this is correct
//   GX_SetScissor((u32) ulx, (u32) uly, (u32) lrx-ulx, (u32) lry-uly);
//   printf("trying to set scissors\n");
}

void RSP::LOADTLUT()
{
   int tile = (*(currentCommand+1) >> 24) & 7;
   int count = ((*(currentCommand+1) >> 14) & 0x3FF)+1;
   //rdp->loadTLUT(tile, count);
//   printf("loadTLUT\n");
}

void RSP::SETTILESIZE()
{
   float uls, ult, lrs, lrt;
   uls = ((*currentCommand>>12) & 0xFFF) / 4.0f;
   ult = (*currentCommand & 0xFFF) / 4.0f;
   lrs = ((*(currentCommand+1)>>12) & 0xFFF) / 4.0f;
   lrt = (*(currentCommand+1) & 0xFFF) / 4.0f;
   int tile = (*(currentCommand+1)>>24) & 7;
   //rdp->setTileSize(uls, ult, lrs, lrt, tile);
//   printf("setTileSize\n");
}

void RSP::LOADBLOCK()
{
   float uls, ult, lrs;
   uls = ((*currentCommand >> 12) & 0xFFF) / 4.0f;
   ult = (*currentCommand & 0xFFF) / 4.0f;
   int tile = (*(currentCommand+1) >> 24) & 7;
   lrs = ((*(currentCommand+1) >> 12) & 0xFFF) / 4.0f;
   int dxt = *(currentCommand+1) & 0xFFF;
   //rdp->loadBlock(uls, ult, tile, lrs, dxt);
//   printf("loadblock\n");
}

void RSP::LOADTILE()
{
   float uls, ult, lrs, lrt;
   uls = ((*currentCommand >> 12) & 0xFFF) / 4.0f;
   ult = (*currentCommand & 0xFFF) / 4.0f;
   int tile = (*(currentCommand+1) >> 24) & 7;
   lrs = ((*(currentCommand+1) >> 12) & 0xFFF) / 4.0f;
   lrt = (*(currentCommand+1) & 0xFFF) / 4.0f;
   //rdp->loadTile(tile, uls, ult, lrs, lrt);
//   printf("loadtile\n");
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
   //rdp->setTile(format, size, line, tmem, tile, palette, cmt, maskt, shiftt, cms, masks, shifts);
//   printf("settile\n");
}

void RSP::FILLRECT()
{
   float ulx, uly, lrx, lry;
   ulx = ((*(currentCommand+1) >> 12) & 0xFFF) / 4.0f;
   uly = (*(currentCommand+1) & 0xFFF) / 4.0f;
   lrx = ((*currentCommand >> 12) & 0xFFF) / 4.0f;
   lry = (*currentCommand & 0xFFF) / 4.0f;
   //rdp->fillRect(ulx, uly, lrx, lry);
//   printf("fillrect??\n");
}

void RSP::SETFILLCOLOR()
{
   //rdp->setFillColor(*(currentCommand+1));
//	printf("setfillcolor\n");
}

void RSP::SETFOGCOLOR()
{
   //rdp->setFogColor(*(currentCommand+1));
//	printf("setFogColor\n");
}

void RSP::SETBLENDCOLOR()
{
   //rdp->setBlendColor(*(currentCommand+1));
//	printf("setBlendColor\n");
}

void RSP::SETPRIMCOLOR()
{
   float mLOD, lLOD;
   mLOD = ((*currentCommand >> 8) & 0xFF) / 256.0f;
   lLOD = ((*currentCommand) & 0xFF) / 256.0f;
   //rdp->setPrimColor(*(currentCommand+1), mLOD, lLOD);
//   printf("setPrimcolor\n");
}

void RSP::SETENVCOLOR()
{
   //rdp->setEnvColor(*(currentCommand+1));
//	printf("setEnvcolor\n");
}

void RSP::SETCOMBINE()
{
   //rdp->setCombineMode(*currentCommand & 0xFFFFFF, *(currentCommand+1));
//	printf("setcominemode\n");
}

void RSP::SETTIMG()
{
   int format = (*currentCommand >> 21) & 0x7;
   int size = (*currentCommand >> 19) & 0x3;
   int width = (*currentCommand & 0xFFF) + 1;
   void *timg = gfxInfo.RDRAM + (seg2phys(*(currentCommand+1)) & 0x7FFFFF);
   //rdp->setTImg(format, size, width, timg);
//   printf("setTimg\n");
}

void RSP::SETZIMG()
{
   void *zimg = gfxInfo.RDRAM + (seg2phys(*(currentCommand+1)) & 0x7FFFFF);
   //rdp->setZImg(zimg);
//   printf("setZimg\n");
}

void RSP::SETCIMG()
{
   int format = (*currentCommand >> 21) & 0x7;
   int size = (*currentCommand >> 19) & 0x3;
   int width = (*currentCommand & 0xFFF) + 1;
   void *cimg = gfxInfo.RDRAM + (seg2phys(*(currentCommand+1)) & 0x7FFFFF);
   //rdp->setCImg(format, size, width, cimg);
//   printf("setCimg\n");
}
