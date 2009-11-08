/**
 * Wii64 - rsp_GX.cpp
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

//#include <cstdio.h>
#include <stdio.h>
#include <math.h>

#include "global.h"
#include "rsp_GX.h"

#include "../gui/DEBUG.h"

RSP::RSP(GFX_INFO info) : gfxInfo(info), error(false), end(false)
{
   for (int i=0; i<0x100; i++) commands[i]=&RSP::NI;
   commands[0x00]=&RSP::SPNOOP;
   commands[0x01]=&RSP::MTX;
   commands[0x03]=&RSP::MOVEMEM;
   commands[0x04]=&RSP::VTX;
   commands[0x06]=&RSP::DL;
   commands[0x09]=&RSP::SPRITE2D;
   commands[0xb2]=&RSP::RDPHALF_CONT;
   commands[0xb3]=&RSP::RDPHALF_2;
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
//   commands[0xe4]=&RSP::SPNOOP;
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
//   commands[0xf6]=&RSP::SPNOOP;
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

//   GX_SetClipMode(GX_CLIP_ENABLE);

//   light_mask = GX_LIGHTNULL;
//   GX_SetChanCtrl(GX_COLOR0A0,GX_DISABLE,GX_SRC_REG,GX_SRC_VTX,light_mask,GX_DF_CLAMP,GX_AF_NONE);
   GX_SetZMode(GX_DISABLE,GX_LEQUAL,GX_TRUE);

   GXfillColor = (GXColor){0,0,0,0};

   //initialize matrices for 2D gfx rendering
   guMtxIdentity(GXmodelView2D);
//   guMtxIdentity(GXnormal2D);
   GX_LoadPosMtxImm(GXmodelView2D,GX_PNMTX1);
//   GX_LoadNrmMtxImm(GXnormal2D,GX_PNMTX0);

      //reset swap table from GUI/DEBUG
	GX_SetTevSwapModeTable(GX_TEV_SWAP0, GX_CH_RED, GX_CH_GREEN, GX_CH_BLUE, GX_CH_ALPHA);
	GX_SetTevSwapMode(GX_TEVSTAGE0, GX_TEV_SWAP0, GX_TEV_SWAP0);

//	guOrtho(GXprojection2D, 0, 239, 0, 319, 0, 1021);

	GXmtxStatus = 0; //invalid Mtx status
	GXnew2Dproj = true;

   TXblock.newblock = 0;

	numVector = 0;
	numVectorMP = 0;

   //rdp = new RDP(info);
	tx = new TX(info);
	bl = new BL(info);
	cc = new CC();

   DEBUG_print((char*)"Executing DList...",DBG_DLIST);
   executeDList();
}

RSP::~RSP()
{
   //delete rdp;
   delete tx;
   delete bl;
   delete cc;
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
         sprintf(txtbuffer,"rsp:unknown DL: push=%x", push);
	   	 DEBUG_print(txtbuffer,DBG_RSPINFO); 
	error = true;
     }
}

void RSP::NI()
{
   sprintf(txtbuffer,"RSP: NI:%x", (int)(*currentCommand>>24));
   DEBUG_print(txtbuffer,DBG_RSPINFO); 
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
        sprintf(txtbuffer,"RSP: unknown MTX:%d", op);
   		DEBUG_print(txtbuffer,DBG_RSPINFO); 
	error = true;
     }
   MP = modelView * projection;

   //Send new modelView, projection, or normal matrices to GX
   if (updateprojection)
   {
	  //Mario64 does something weird with the projection matrix during opening.
	  //Still investigating how to solve this:
	  if((projection(2,3) == -1) && (projection(3,3) != 0))
//	  if(true)
	  {
		  GXuseMatrix = false;
   		  DEBUG_print((char*)"Projection matrix is both Persp and Ortho!",DBG_RSPINFO);
		  guMtxIdentity(GXmodelView);
		  guMtxIdentity(GXprojection);
		  GXprojection[3][2] = 0;
		  GXprojection[3][3] = 1;
//		  GXprojection[2][2] = -GXprojection[2][2];	//test
/*	      for (int j=0; j<3; j++)
		  {
			  for (int i=0; i<4; i++)
		      {
			    GXmodelView[j][i] = MP(i,j);
		      }
		  }*/
	      GX_LoadPosMtxImm(GXmodelView, GX_PNMTX0);
	  }
	  else
	  {
		if(!GXuseMatrix)
		{
			for (int j=0; j<3; j++)
			{
				for (int i=0; i<4; i++)
				{
					GXmodelView[j][i] = modelView(i,j);
				}
			}
			GX_LoadPosMtxImm(GXmodelView, GX_PNMTX0);
		}
		GXuseMatrix = true;
		for (int j=0; j<4; j++)
		{
			for (int i=0; i<4; i++)
			{
				GXprojection[j][i] = projection(i,j);
//				printf("p[%i][%i] = %f\n",j,i,projection(j,i));
			}
		}
	  }
	  //N64 Z clip space is backwards, so mult z components by -1
	  //N64 Z [-1,1] whereas GC Z [-1,0], so mult by 0.5 and shift by -0.5
	  GXprojection[2][2] = 0.5*GXprojection[2][2] - 0.5*GXprojection[3][2];
	  GXprojection[2][3] = 0.5*GXprojection[2][3] - 0.5*GXprojection[3][3];
//	  GXprojection[2][2] = 0.5;
//	  GXprojection[2][3] = -0.5;


//	  GXprojection[2][3] = 0.5*GXprojection[2][3] - 0.5;
//	  GXprojection[2][2] = -0.5*GXprojection[2][2] - 0.5*GXprojection[3][2];
//	  GXprojection[2][3] = -0.5*GXprojection[2][3] - 0.5;

//	  if((GXprojection[3][2] == -1) && (GXprojection[3][3] == 0)) 
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
	  if(GXuseMatrix) {
      for (int j=0; j<3; j++)
      {
         for (int i=0; i<4; i++)
         {
            GXmodelView[j][i] = modelView(i,j);
//            if(GXuseMatrix) GXmodelView[j][i] = modelView(i,j);
//			else GXmodelView[j][i] = MP(i,j);
//			printf("mv[%i][%i] = %f\n",j,i,modelView(j,i));
         }
	  }
	  }
	  else 
		  guMtxIdentity(GXmodelView);
      GX_LoadPosMtxImm(GXmodelView, GX_PNMTX0);
	  if(guMtxInverse(GXmodelView, GXnormal) == 0)	// normal matrix is the inverse transpose of the modelview matrix
   		  DEBUG_print((char*)"Normal matrix is singular",DBG_RSPINFO);
	  guMtxTranspose(GXnormal, GXnormal);
      GX_LoadNrmMtxImm(GXnormal, GX_PNMTX0);
//	  printf("Send Normal Matrix to GX\n");
/*	  printf("\tMV =\t%f, %f, %f, %f\n",GXmodelView[0][0],GXmodelView[0][1],GXmodelView[0][2],GXmodelView[0][3]);
	  printf("\t\t%f, %f, %f, %f\n",GXmodelView[1][0],GXmodelView[1][1],GXmodelView[1][2],GXmodelView[1][3]);
	  printf("\t\t%f, %f, %f, %f\n",GXmodelView[2][0],GXmodelView[2][1],GXmodelView[2][2],GXmodelView[2][3]);*/
   }
   GXmtxStatus = 0; // invalid mtx state (load matrices on next 3D triangle)
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
	// Calculated 2D projection matrix here
	if(GXnew2Dproj) {
		guOrtho(GXprojection2D, viewport.vtrans[1]-viewport.vscale[1], viewport.vtrans[1]+viewport.vscale[1]-1,
								viewport.vtrans[0]-viewport.vscale[0], viewport.vtrans[0]+viewport.vscale[0]-1,
								viewport.vtrans[2]-viewport.vscale[2],viewport.vtrans[2]+viewport.vscale[2]-1);
		GXnew2Dproj = false;
	}

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
//		  GX_InitLightColor(&GXlight,GXcol);
		  //init light direction (normalized & transformed by modelview matrix)
//		  GX_InitLightDir(&GXlight,spotLight[n].dir[0],spotLight[n].dir[1],spotLight[n].dir[2]);
		  //no light position specified
		  //TODO: Calculate position at infinity??
		  //update light_mask
//		  light_mask = light_mask | (GX_LIGHT0<<n);
		  //add this light to color channel 0
//		  GX_SetChanCtrl(GX_COLOR0A0,GX_ENABLE,GX_SRC_REG,GX_SRC_VTX,light_mask,GX_DF_CLAMP,GX_AF_NONE);
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
//		  GX_SetChanAmbColor(GX_COLOR0A0,GXcol);
	       }
	  }
	break;
      default:
      	  sprintf(txtbuffer,"unknown MOVEMEM:%x", dest);
   		  DEBUG_print(txtbuffer,DBG_RSPINFO);
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
	if(!GXuseMatrix)
	{
		vtx[v0+i].v = vtx[v0+i].v * MP;
		vtx[v0+i].v[0] = vtx[v0+i].v[0] / vtx[v0+i].v[3];
		vtx[v0+i].v[1] = vtx[v0+i].v[1] / vtx[v0+i].v[3];
		vtx[v0+i].v[2] = vtx[v0+i].v[2] / vtx[v0+i].v[3];
		numVectorMP++;
	}
	else
		numVector++;

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
	     vtx[v0+i].n = vtx[v0+i].n * modelView;	//<- want this?
	     vtx[v0+i].n.normalize();
	     
	     // This should now be calculated by GX
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
   
   DEBUG_print((char*)"Sprite2D call",DBG_RSPINFO);

   if (SourceImageBitSize == 0) {
	 sprintf(txtbuffer,"RSP:SPRITE2D image type=%d bitsize=%d", SourceImageType, SourceImageBitSize);
   	 DEBUG_print(txtbuffer,DBG_RSPINFO1);
   }
   if (FlipTextureX || FlipTextureY)
     DEBUG_print((char*)"RSP:SPRITE2D flip",DBG_RSPINFO1);
   
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
//	   light_mask = GX_LIGHTNULL;
//	   GX_SetChanCtrl(GX_COLOR0A0,GX_DISABLE,GX_SRC_REG,GX_SRC_VTX,light_mask,GX_DF_CLAMP,GX_AF_NONE);
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
//	   GX_SetNumChans(1);
   }
   if (mode & 0x10000) fog = true;
   if (mode & 0x2000) cull_back = true;
   if (mode & 0x200) shading_smooth = true;
   if (mode & 0x4) shade = true;
   if (mode & 0x1) 
   {
	   zbuffer = true;
	   GX_SetZMode(GX_ENABLE,GX_LEQUAL,GX_TRUE);
   }
   
   if (mode & ~0x72205) {
	 sprintf(txtbuffer,"rsp unknown SETGEOMETRYMODE:%x", mode & ~0x72205);
   	 DEBUG_print(txtbuffer,DBG_RSPINFO);
	}
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
	sprintf(txtbuffer,"RSP: vect = %d; vectMP = %d", numVector, numVectorMP);
	DEBUG_print(txtbuffer,11);

   end = true;
}

void RSP::RDPHALF_1()
{
}

void RSP::RDPHALF_2()
{
}

void RSP::RDPHALF_CONT()
{
}

void RSP::SETOTHERMODE_L()
{
   int mode = (*currentCommand >> 8) & 0xFF;
   int length = *currentCommand & 0xFF;
   unsigned long data = *(currentCommand+1);
   
   int data2 = (data>>mode)&((1<<length)-1);

   //rdp->setOtherMode_l(mode, (data>>mode)&((1<<length)-1));
//   printf("Setothermode_l\n");

      switch(mode)
     {
      case 0:
	bl->setAlphaCompare(data2);
	break;
      case 2:
	bl->setDepthSource(data2);		//not used by soft_gfx
	break;
      case 3:
	bl->setBlender(data2<<3);
	break;
      default:
     sprintf(txtbuffer,"RDP: unknown setOtherMode_l:%d", mode);
   	 DEBUG_print(txtbuffer,DBG_RSPINFO);
     }

}

void RSP::SETOTHERMODE_H()
{
   int mode = (*currentCommand >> 8) & 0xFF;
   int length = *currentCommand & 0xFF;
   unsigned long data = *(currentCommand+1);
//   int data = (*(currentCommand+1)>>mode)&((1<<length)-1);
   
   int data2 = (data>>mode)&((1<<length)-1);

   //rdp->setOtherMode_h(mode, (data>>mode)&((1<<length)-1));
//   printf("setothermode_h\n");

   switch(mode)
     {
      case 4:
	bl->setAlphaDither(data2);
	break;
      case 6:
	bl->setColorDither(data2);
	break;
      case 8:
	cc->setCombineKey(data2);
	break;
      case 9:
//	tf->setTextureConvert(data2);
	break;
      case 12:
//	tf->setTextureFilter(data2);
	break;
      case 14:
//	textureLUT = data2;
	tx->setTextureLUT(data2);
	break;
      case 16:
	tx->setTextureLOD(data2);
	break;
      case 17:
	tx->setTextureDetail(data2);
	break;
      case 19:
	tx->setTexturePersp(data2);
	break;
      case 20:
	cycleType = data2;
	break;
      case 23:
	// ignoring pipeline mode
	break;
      default:
      sprintf(txtbuffer,"RDP: unknown setOtherMode_h:%d", mode);
   	 DEBUG_print(txtbuffer,DBG_RSPINFO);
     }
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
//	light_mask = GX_LIGHTNULL;
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
     sprintf(txtbuffer,"unknown MOVEWORD:%x", index);
   	 DEBUG_print(txtbuffer,DBG_RSPINFO);
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
	if(GXuseMatrix) {
      for (int j=0; j<3; j++)
      {
         for (int i=0; i<4; i++)
         {
            GXmodelView[j][i] = modelView(i,j);
//			printf("mv[%i][%i] = %f\n",j,i,modelView(j,i));
         }
	  }
	}
	else 
	  guMtxIdentity(GXmodelView);

//	GX_LoadPosMtxImm(GXmodelView, GX_PNMTX0);
	if(guMtxInverse(GXmodelView, GXnormal) == 0)
		DEBUG_print((char*)"Normal matrix is singular",DBG_RSPINFO);
	guMtxTranspose(GXnormal, GXnormal);
	GX_LoadNrmMtxImm(GXnormal, GX_PNMTX0);
//	  printf("Send Normal Matrix to GX\n");
//   GXmtxStatus = 0; // invalid mtx state (load matrices on next 3D triangle)
}

void RSP::TRI1()
{
   float ps0, ps1, ps2, pt0, pt1, pt2, w, h;
   int v1 = ((*(currentCommand+1) >> 16) & 0xFF) / 10; //TODO: Verify correct vertex ordering for GX
   int v2 = ((*(currentCommand+1) >> 8) & 0xFF) / 10;
   int v0 = (*(currentCommand+1) & 0xFF) / 10;
//   int s0, s1, s2, t0, t1, t2;
//   bool invertedS0 = false, invertedS1 = false, invertedS2 = false;
//   bool invertedT0 = false, invertedT1 = false, invertedT2 = false;
   
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
	   case 0x10005: // fog | shade | z_buffer	-> use vertex color from a
		   if (textureScales.enabled) {}
	       //rdp->tri_shade_txtr_zbuff(vx0, vx1, vx2, cache[a].c, cache[a].c, cache[a].c,
			//		 cache[a].s, cache[a].t, cache[b].s, cache[b].t, cache[i].s, cache[i].t, textureScales.tile,
			//		 cache[a].v[3], cache[b].v[3], cache[i].v[3], z0, z1, z2);
	     else {}
	       //rdp->tri_shade_zbuff(vx0, vx1, vx2, cache[a].c, cache[a].c, cache[a].c, z0, z1, z2);
	     break;
	   case 0x204: // shading_smooth | shade	-> use per vertex color
	     if (textureScales.enabled) {}
	       //rdp->tri_shade_txtr(vx0, vx1, vx2, cache[a].c, cache[b].c, cache[i].c,
			//	   cache[a].s, cache[a].t, cache[b].s, cache[b].t, cache[i].s, cache[i].t, textureScales.tile,
			//	   cache[a].v[3], cache[b].v[3], cache[i].v[3]);
	     else {}
	       //rdp->tri_shade(vx0, vx1, vx2, cache[a].c, cache[b].c, cache[i].c);
	     break;
	   case 0x205: // shading_smooth | shade | z_buffer
	   case 0x10205: // fog | shading_smooth | shade | z_buffer		-> use per vertex color
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
   //TODO: Investigate how cycleType affects rendering

	//TODO: Fog
	
	if (GXmtxStatus != 1) { //need to load 3D mtx's
		if(GXprojection[3][2] == -1) 
			GX_LoadProjectionMtx(GXprojection, GX_PERSPECTIVE); 
		else 
			GX_LoadProjectionMtx(GXprojection, GX_ORTHOGRAPHIC); 
		GX_LoadPosMtxImm(GXmodelView, GX_PNMTX0);
		GX_LoadNrmMtxImm(GXnormal, GX_PNMTX0);
		GXmtxStatus = 1; //3D mtx loaded
	}

   //set vertex description here
   GX_ClearVtxDesc();
   GX_SetVtxDesc(GX_VA_PTNMTXIDX, GX_PNMTX0);
   if (textureScales.enabled) GX_SetVtxDesc(GX_VA_TEX0MTXIDX, GX_IDENTITY);
   GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
//   if (lighting) GX_SetVtxDesc(GX_VA_NRM, GX_DIRECT);
   GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
   if (textureScales.enabled) GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);

   //set vertex attribute formats here
   GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
//   if (lighting) GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_NRM, GX_NRM_XYZ, GX_F32, 0);
   GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
   if (textureScales.enabled) GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0); // should change this to tile

   if (textureScales.enabled)
   {
		//if there is a new texture to load, load it!
		if (TXblock.newblock == 1)
		{
			tx->loadBlock(TXblock.uls, TXblock.ult, TXblock.tile, TXblock.lrs, TXblock.dxt);
			TXblock.newblock = 0;
		}

/*		GX_SetNumChans (1);
		GX_SetNumTexGens (1);
		// GX_SetTexCoordGen TexCoord0 should be set to an identity 2x4 matrix in GXinit
		GX_SetTevOrder (GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0); // change to (u8) tile later
//		GX_SetTevOp(GX_TEVSTAGE0, GX_MODULATE);	// change this to tile as well...
		GX_SetTevOp (GX_TEVSTAGE0, GX_REPLACE);
//		GX_SetTevOp (GX_TEVSTAGE0, GX_PASSCLR);*/

//		int tile = 0;	// May need to detect latest tile
		int tile = currentTile;

		//This block attempted to properly handle masks/maskt, but it doesn't 
/*		w = (descriptor[tile].lrs) - (descriptor[tile].uls);
		h = (descriptor[tile].lrt) - (descriptor[tile].ult);

		s0 = vtx[v0].s - descriptor[tile].uls;
		s1 = vtx[v1].s - descriptor[tile].uls;
		s2 = vtx[v2].s - descriptor[tile].uls;
		t0 = vtx[v0].t - descriptor[tile].ult;
		t1 = vtx[v1].t - descriptor[tile].ult;
		t2 = vtx[v2].t - descriptor[tile].ult;

		if (descriptor[tile].cms & 1 && s0 & (1<<descriptor[tile].masks)) invertedS0 = true;
		if (descriptor[tile].cms & 1 && s1 & (1<<descriptor[tile].masks)) invertedS1 = true;
		if (descriptor[tile].cms & 1 && s2 & (1<<descriptor[tile].masks)) invertedS2 = true;
		if (descriptor[tile].cmt & 1 && t0 & (1<<descriptor[tile].maskt)) invertedT0 = true;
		if (descriptor[tile].cmt & 1 && t1 & (1<<descriptor[tile].maskt)) invertedT1 = true;
		if (descriptor[tile].cmt & 1 && t2 & (1<<descriptor[tile].maskt)) invertedT2 = true;
		if (descriptor[tile].masks) {
			s0 &= (1<<descriptor[tile].masks)-1;
			s1 &= (1<<descriptor[tile].masks)-1;
			s2 &= (1<<descriptor[tile].masks)-1;
		}
		if (descriptor[tile].maskt) {
			t0 &= (1<<descriptor[tile].maskt)-1;
			t1 &= (1<<descriptor[tile].maskt)-1;
			t2 &= (1<<descriptor[tile].maskt)-1;
		}
		if (invertedS0) s0 = w - s0;
		if (invertedS1) s1 = w - s1;
		if (invertedS2) s2 = w - s2;
		if (invertedT0) t0 = h - t0;
		if (invertedT1) t1 = h - t1;
		if (invertedT2) t2 = h - t2;
		ps0 = s0/w;
		ps1 = s1/w;
		ps2 = s2/w;
		pt0 = t0/w;
		pt1 = t1/w;
		pt2 = t2/w;*/

		//This is more elegant, but it breaks when the mask is bigger than the texture width
/*		if(descriptor[tile].masks) w = (1<<descriptor[tile].masks)-1;
		else w = (descriptor[tile].lrs) - (descriptor[tile].uls);
		if(descriptor[tile].maskt) h = (1<<descriptor[tile].maskt)-1;
		else h = (descriptor[tile].lrt) - (descriptor[tile].ult);*/

		w = (descriptor[tile].lrs) - (descriptor[tile].uls);
		h = (descriptor[tile].lrt) - (descriptor[tile].ult);

		ps0 = (vtx[v0].s - descriptor[tile].uls)/w;
		ps1 = (vtx[v1].s - descriptor[tile].uls)/w;
		ps2 = (vtx[v2].s - descriptor[tile].uls)/w;
		pt0 = (vtx[v0].t - descriptor[tile].ult)/h;
		pt1 = (vtx[v1].t - descriptor[tile].ult)/h;
		pt2 = (vtx[v2].t - descriptor[tile].ult)/h;
   }
   else
   {
/*		GX_SetNumChans (1);
		GX_SetNumTexGens (0);
		GX_SetTevOrder (GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
		GX_SetTevOp (GX_TEVSTAGE0, GX_PASSCLR);*/
   }

   if (cycleType == 1) {
		DEBUG_print((char*)"TRI1: CopyModeDraw2\n",DBG_RSPINFO);
		cc->combine2(textureScales.tile,textureScales.tile+1,textureScales.enabled);
   }
   else
	   cc->combine1(textureScales.tile,textureScales.enabled);

   bl->cycle1ModeDraw();

/*   //TODO: These blend modes need to be fixed!
   if (textureScales.enabled)
   {
	//Set CopyModeDraw blend modes here
	GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR); //Fix src alpha
	GX_SetColorUpdate(GX_ENABLE);
	GX_SetAlphaUpdate(GX_ENABLE);
	GX_SetDstAlpha(GX_DISABLE, 0xFF);
   }
   else
   {
	//Set FillModeDraw blend modes here
	GX_SetBlendMode(GX_BM_NONE, GX_BL_ONE, GX_BL_ZERO, GX_LO_CLEAR); //Fix src alpha
	GX_SetColorUpdate(GX_ENABLE);
	GX_SetAlphaUpdate(GX_ENABLE);
	GX_SetDstAlpha(GX_DISABLE, 0xFF);
   }*/

   GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 3);
     // vert 0
     GX_Position3f32(vtx[v0].v[0], vtx[v0].v[1], vtx[v0].v[2]);
//	 if (lighting) GX_Normal3f32(vtx[v0].n[0], vtx[v0].n[1], vtx[v0].n[2]);
	 if (shading_smooth) { //use per vertex color
		 GXcol.r = (u8)vtx[v0].c.getR();
		 GXcol.g = (u8)vtx[v0].c.getG();
		 GXcol.b = (u8)vtx[v0].c.getB();
		 GXcol.a = (u8)vtx[v0].c.getAlpha();
	 }
	 else {	//use volor from a (v1)
		 GXcol.r = (u8)vtx[v1].c.getR();
		 GXcol.g = (u8)vtx[v1].c.getG();
		 GXcol.b = (u8)vtx[v1].c.getB();
		 GXcol.a = (u8)vtx[v1].c.getAlpha();
	 }
	 GX_Color4u8(GXcol.r, GXcol.g, GXcol.b, GXcol.a); 
	 if (textureScales.enabled) GX_TexCoord2f32(ps0,pt0);

     // vert 1
     GX_Position3f32(vtx[v1].v[0], vtx[v1].v[1], vtx[v1].v[2]);
//	 if (lighting) GX_Normal3f32(vtx[v1].n[0], vtx[v1].n[1], vtx[v1].n[2]);
	 if (shading_smooth) { //use per vertex color
		 GXcol.r = (u8)vtx[v1].c.getR();
		 GXcol.g = (u8)vtx[v1].c.getG();
		 GXcol.b = (u8)vtx[v1].c.getB();
		 GXcol.a = (u8)vtx[v1].c.getAlpha();
	 }
	 else {	//use volor from a (v1)
		 GXcol.r = (u8)vtx[v1].c.getR();
		 GXcol.g = (u8)vtx[v1].c.getG();
		 GXcol.b = (u8)vtx[v1].c.getB();
		 GXcol.a = (u8)vtx[v1].c.getAlpha();
	 }
	 GX_Color4u8(GXcol.r, GXcol.g, GXcol.b, GXcol.a);
	 if (textureScales.enabled) GX_TexCoord2f32(ps1,pt1);

     // vert 2
     GX_Position3f32(vtx[v2].v[0], vtx[v2].v[1], vtx[v2].v[2]);
//	 if (lighting) GX_Normal3f32(vtx[v2].n[0], vtx[v2].n[1], vtx[v2].n[2]);
	 if (shading_smooth) { //use per vertex color
		 GXcol.r = (u8)vtx[v2].c.getR();
		 GXcol.g = (u8)vtx[v2].c.getG();
		 GXcol.b = (u8)vtx[v2].c.getB();
		 GXcol.a = (u8)vtx[v2].c.getAlpha();
	 }
	 else {	//use volor from a (v1)
		 GXcol.r = (u8)vtx[v1].c.getR();
		 GXcol.g = (u8)vtx[v1].c.getG();
		 GXcol.b = (u8)vtx[v1].c.getB();
		 GXcol.a = (u8)vtx[v1].c.getAlpha();
	 }
	 GX_Color4u8(GXcol.r, GXcol.g, GXcol.b, GXcol.a);
	 if (textureScales.enabled) GX_TexCoord2f32(ps2,pt2);
   GX_End();

}

void RSP::TEXRECT()
{
   float ulx, uly, lrx, lry, s, t, dsdx, dtdy, s2, s2c, t2, w, h;
   float px1, px2, py1, py2, ps1, ps2, ps2c, pt1, pt2;
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

   if (TXblock.newblock == 1)
   {
	   tx->loadBlock(TXblock.uls, TXblock.ult, TXblock.tile, TXblock.lrs, TXblock.dxt);
	   TXblock.newblock = 0;
   }

//   s2 = (lrx - ulx) * 1 + s;
   s2 = (lrx - ulx) * dsdx + s; 
   s2c = (lrx - ulx) * dsdx/4 + s; //divide by 4 only for copy mode!
   t2 = (lry - uly) * dtdy + t;

//   int w = (int)(descriptor[tile].lrs - descriptor[tile].uls)/dsdx;
//   int w = TXwidth;
	if(descriptor[tile].masks) w = (1<<descriptor[tile].masks)-1 + 1;
	else w = (descriptor[tile].lrs) - (descriptor[tile].uls) + 1;
	if(descriptor[tile].maskt) h = (1<<descriptor[tile].maskt)-1 + 1;
	else h = (descriptor[tile].lrt) - (descriptor[tile].ult) + 1;
//   w = ceil((descriptor[tile].lrs - descriptor[tile].uls + 1)/8)*8;
//   h = ceil((descriptor[tile].lrt) - (descriptor[tile].ult + 1)/4)*4;
   ps1 = (s - descriptor[tile].uls)/w;
   pt1 = (t - descriptor[tile].ult)/h;
   ps2 = (s2 - descriptor[tile].uls)/w;
   ps2c = (s2c - descriptor[tile].uls)/w;
   pt2 = (t2 - descriptor[tile].ult)/h;
   px1 =  ulx;
   py1 =  uly;
   px2 = lrx+1;
   py2 = lry+1;
//   px2 = lrx;
//   py2 = lry;

   	if (GXmtxStatus != 2) { //need to load 2D mtx's
		GX_LoadProjectionMtx(GXprojection2D, GX_ORTHOGRAPHIC); //load current 2D projection matrix
		GXmtxStatus = 2; //2D mtx loaded
	}

	// scissoring should be handled automatically by GX
   if (cycleType == 0) // 1 cycle mode
     {
		//draw textured rectangle from ulx,uly to lrx,lry
		GX_ClearVtxDesc();
//		GX_SetVtxDesc(GX_VA_PTNMTXIDX, GX_PNMTX1);
		GX_SetVtxDesc(GX_VA_PTNMTXIDX, GX_IDENTITY);
		GX_SetVtxDesc(GX_VA_TEX0MTXIDX, GX_IDENTITY);
		GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
//		GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
		GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
		//set vertex attribute formats here
		GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_F32, 0);
//		GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
		GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0); // should change this to tile


/*		//testing this: re-enable textures
		GX_SetNumChans (1);
		GX_SetNumTexGens (1);
		// GX_SetTexCoordGen TexCoord0 should be set to an identity 2x4 matrix in GXinit
		GX_SetTevOrder (GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0); // change to (u8) tile later
//		GX_SetTevOp(GX_TEVSTAGE0, GX_MODULATE);	// change this to tile as well...
		GX_SetTevOp (GX_TEVSTAGE0, GX_REPLACE);*/

		cc->combine1(tile,true);
		bl->cycle1ModeDraw();

/*		//Set CopyModeDraw blend modes here
		GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR); //Fix src alpha
		GX_SetColorUpdate(GX_ENABLE);
		GX_SetAlphaUpdate(GX_ENABLE);
		GX_SetDstAlpha(GX_DISABLE, 0xFF);*/


		GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
		// vert 0
		GX_Position2f32(px1, py1);
//		GX_Color4u8(GXfillColor.r, GXfillColor.g, GXfillColor.b, GXfillColor.a);
		GX_TexCoord2f32(ps1,pt1);
		// vert 1
		GX_Position2f32(px2, py1);
//		GX_Color4u8(GXfillColor.r, GXfillColor.g, GXfillColor.b, GXfillColor.a);
		GX_TexCoord2f32(ps2,pt1);
		// vert 2
		GX_Position2f32(px2, py2);
//		GX_Color4u8(GXfillColor.r, GXfillColor.g, GXfillColor.b, GXfillColor.a);
		GX_TexCoord2f32(ps2,pt2);
		// vert 3
		GX_Position2f32(px1, py2);
//		GX_Color4u8(GXfillColor.r, GXfillColor.g, GXfillColor.b, GXfillColor.a);
		GX_TexCoord2f32(ps1,pt2);
		GX_End();
//		TODO: Implement 1 cycle mode
//		  Color32 t = rdp->tx->getTexel(ps, pt, tile, rdp->tf);
//		  Color32 c = rdp->cc->combine1(t);
//		  rdp->bl->cycle1ModeDraw(j,i,c);
     }
   else if (cycleType == 2) // copy mode
     {
		//draw textured rectangle from ulx,uly to lrx,lry
		GX_ClearVtxDesc();
//		GX_SetVtxDesc(GX_VA_PTNMTXIDX, GX_PNMTX1);
		GX_SetVtxDesc(GX_VA_PTNMTXIDX, GX_IDENTITY);
		GX_SetVtxDesc(GX_VA_TEX0MTXIDX, GX_IDENTITY);
		GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
//		GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
		GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
		//set vertex attribute formats here
		GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_F32, 0);
//		GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
		GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0); // should change this to tile


		//testing this: re-enable textures
/*		GX_SetNumChans (1);
		GX_SetNumTexGens (1);
		// GX_SetTexCoordGen TexCoord0 should be set to an identity 2x4 matrix in GXinit
		GX_SetTevOrder (GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0); // change to (u8) tile later
//		GX_SetTevOp(GX_TEVSTAGE0, GX_MODULATE);	// change this to tile as well...
		GX_SetTevOp (GX_TEVSTAGE0, GX_REPLACE);


		//Set CopyModeDraw blend modes here
		GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR); //Fix src alpha
		GX_SetColorUpdate(GX_ENABLE);
		GX_SetAlphaUpdate(GX_ENABLE);
		GX_SetDstAlpha(GX_DISABLE, 0xFF);*/
		bl->copyModeDraw();

		GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
		// vert 0
		GX_Position2f32(px1, py1);
//		GX_Color4u8(GXfillColor.r, GXfillColor.g, GXfillColor.b, GXfillColor.a);
		GX_TexCoord2f32(ps1,pt1);
		// vert 1
		GX_Position2f32(px2, py1);
//		GX_Color4u8(GXfillColor.r, GXfillColor.g, GXfillColor.b, GXfillColor.a);
		GX_TexCoord2f32(ps2c,pt1);
		// vert 2
		GX_Position2f32(px2, py2);
//		GX_Color4u8(GXfillColor.r, GXfillColor.g, GXfillColor.b, GXfillColor.a);
		GX_TexCoord2f32(ps2c,pt2);
		// vert 3
		GX_Position2f32(px1, py2);
//		GX_Color4u8(GXfillColor.r, GXfillColor.g, GXfillColor.b, GXfillColor.a);
		GX_TexCoord2f32(ps1,pt2);
		GX_End();
//		TODO: Implement Copy mode
//		  Color32 t = rdp->tx->getTexel(ps, pt, tile, NULL);
//		  rdp->bl->copyModeDraw(j,i,t);
     }
   else {
	   sprintf(txtbuffer,"RS:unknown cycle type in texRect:%d", cycleType);
   	   DEBUG_print(txtbuffer,DBG_RSPINFO);
	}

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
   tx->loadTLUT(tile, count);
   //rdp->loadTLUT(tile, count);
//   printf("loadTLUT\n");

//   GX_InitTlutObj(&GXtlut, tImg,GX_TF_RGB5A3,count);
//   GX_LoadTlut(&GXtlut, GX_TLUT0);	// use GX_TLUT0 or (u32) tile??

//void GX_InitTexObjCI(GXTexObj *obj,void *img_ptr,u16 wd,u16 ht,u8 fmt,u8 wrap_s,u8 wrap_t,u8 mipmap,u32 tlut_name);

}

void RSP::SETTILESIZE()
{
   float uls, ult, lrs, lrt;
   uls = ((*currentCommand>>12) & 0xFFF) / 4.0f;
   ult = (*currentCommand & 0xFFF) / 4.0f;
   lrs = ((*(currentCommand+1)>>12) & 0xFFF) / 4.0f;
   lrt = (*(currentCommand+1) & 0xFFF) / 4.0f;
   int tile = (*(currentCommand+1)>>24) & 7;
   tx->setTileSize(uls, ult, lrs, lrt, tile);

	currentTile = tile;
	descriptor[tile].uls = uls;
	descriptor[tile].ult = ult;
	descriptor[tile].lrs = lrs;
	descriptor[tile].lrt = lrt;
   //rdp->setTileSize(uls, ult, lrs, lrt, tile);
//   printf("setTileSize\n");
}

void RSP::LOADBLOCK()
{
   TXblock.uls = ((*currentCommand >> 12) & 0xFFF) / 4.0f;
   TXblock.ult = (*currentCommand & 0xFFF) / 4.0f;
   TXblock.tile = (*(currentCommand+1) >> 24) & 7;
   TXblock.lrs = ((*(currentCommand+1) >> 12) & 0xFFF) / 4.0f;
   TXblock.dxt = *(currentCommand+1) & 0xFFF;

   TXblock.newblock = 1;

	//rdp->loadBlock(uls, ult, tile, lrs, dxt);
//   printf("loadblock\n");
}

void RSP::LOADTILE()
{
   float uls, ult, lrs, lrt;
//   u8 wrap_s, wrap_t;
   uls = ((*currentCommand >> 12) & 0xFFF) / 4.0f;
   ult = (*currentCommand & 0xFFF) / 4.0f;
   int tile = (*(currentCommand+1) >> 24) & 7;
   lrs = ((*(currentCommand+1) >> 12) & 0xFFF) / 4.0f;
   lrt = (*(currentCommand+1) & 0xFFF) / 4.0f;
   //rdp->loadTile(tile, uls, ult, lrs, lrt);
//   printf("loadtile\n");

   tx->loadTile(tile, uls, ult, lrs, lrt);
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
   tx->setTile(format, size, line, tmem, tile, palette, cmt, maskt, shiftt, cms, masks, shifts);

	descriptor[tile].format = format;
	descriptor[tile].size = size;
	descriptor[tile].line = line;
	descriptor[tile].tmem = tmem;
	descriptor[tile].palette = palette;
	descriptor[tile].cmt = cmt;
	descriptor[tile].maskt = maskt;
	descriptor[tile].shiftt = shiftt;
	descriptor[tile].cms = cms;
	descriptor[tile].masks = masks;
	descriptor[tile].shifts = shifts;
}

void RSP::FILLRECT()
{
   float ulx, uly, lrx, lry;
   ulx = ((*(currentCommand+1) >> 12) & 0xFFF) / 4.0f;
   uly = (*(currentCommand+1) & 0xFFF) / 4.0f;
   lrx = ((*currentCommand >> 12) & 0xFFF) / 4.0f;
   lry = (*currentCommand & 0xFFF) / 4.0f;
   //rdp->fillRect(ulx, uly, lrx, lry);
//   printf("fillrect?? Cycle = %d, fill = %d %d %d %d\n", cycleType,GXfillColor.r,GXfillColor.g,
//	   GXfillColor.b,GXfillColor.a);

//   GX_LoadPosMtxImm(GXmodelView2D,GX_PNMTX2);
//   GX_LoadNrmMtxImm(GXnormal2D,GX_PNMTX2);

   	if (GXmtxStatus != 2) { //need to load 2D mtx's
		GX_LoadProjectionMtx(GXprojection2D, GX_ORTHOGRAPHIC); //load current 2D projection matrix
		GXmtxStatus = 2; //2D mtx loaded
	}

	// scissoring should be handled automatically by GX
   if (cycleType == 3) //use fillColor
     {
		//draw rectangle from ulx,uly to lrx,lry
		GX_ClearVtxDesc();
//		GX_SetVtxDesc(GX_VA_PTNMTXIDX, GX_PNMTX1);
		GX_SetVtxDesc(GX_VA_PTNMTXIDX, GX_IDENTITY);
		GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
		GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
		//set vertex attribute formats here
		GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_F32, 0);
		GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);

		//testing this: disable textures
/*		GX_SetNumChans (1);
		GX_SetNumTexGens (0);
		GX_SetTevOrder (GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
		GX_SetTevOp (GX_TEVSTAGE0, GX_PASSCLR);

		//Set FillModeDraw blend modes here
		GX_SetBlendMode(GX_BM_NONE, GX_BL_ONE, GX_BL_ZERO, GX_LO_CLEAR); //Fix src alpha
		GX_SetColorUpdate(GX_ENABLE);
		GX_SetAlphaUpdate(GX_ENABLE);
		GX_SetDstAlpha(GX_DISABLE, 0xFF);*/
		bl->fillModeDraw();

		GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
		// vert 0
		GX_Position2f32(ulx, uly);
//		GX_Position3f32(ulx, uly, (f32) 0);
		GX_Color4u8(GXfillColor.r, GXfillColor.g, GXfillColor.b, GXfillColor.a);
		// vert 1
		GX_Position2f32(lrx, uly);
		GX_Color4u8(GXfillColor.r, GXfillColor.g, GXfillColor.b, GXfillColor.a);
		// vert 2
		GX_Position2f32(lrx, lry);
		GX_Color4u8(GXfillColor.r, GXfillColor.g, GXfillColor.b, GXfillColor.a);
		// vert 3
		GX_Position2f32(ulx, lry);
		GX_Color4u8(GXfillColor.r, GXfillColor.g, GXfillColor.b, GXfillColor.a);
		GX_End();
     }
   else if (cycleType == 0) //use cycle1ModeDraw
     {
		//draw rectangle from ulx,uly to lrx,lry
		GX_ClearVtxDesc();
//		GX_SetVtxDesc(GX_VA_PTNMTXIDX, GX_PNMTX1);
		GX_SetVtxDesc(GX_VA_PTNMTXIDX, GX_IDENTITY);
		GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
//		GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
		//set vertex attribute formats here
//		GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_F32, 0);
		GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
//		GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);

		//testing this: disable textures
/*		GX_SetNumChans (1);
		GX_SetNumTexGens (0);
		GX_SetTevOrder (GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
		GX_SetTevOp (GX_TEVSTAGE0, GX_PASSCLR);*/

		cc->combine1(0,false);
		bl->cycle1ModeDraw();

		GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
		// vert 0
//		GX_Position2f32(ulx, uly);
		GX_Position3f32(ulx, uly, (f32) 0);
//		GX_Color4u8(GXfillColor.r, GXfillColor.g, GXfillColor.b, GXfillColor.a);
		// vert 1
//		GX_Position2f32(lrx, uly);
		GX_Position3f32(lrx, uly, (f32) 0);
//		GX_Color4u8(GXfillColor.r, GXfillColor.g, GXfillColor.b, GXfillColor.a);
		// vert 2
//		GX_Position2f32(lrx, lry);
		GX_Position3f32(lrx, lry, (f32) 0);
//		GX_Color4u8(GXfillColor.r, GXfillColor.g, GXfillColor.b, GXfillColor.a);
		// vert 3
//		GX_Position2f32(ulx, lry);
		GX_Position3f32(ulx, lry, (f32) 0);
//		GX_Color4u8(GXfillColor.r, GXfillColor.g, GXfillColor.b, GXfillColor.a);
		GX_End();
//		  Color32 c = rdp->cc->combine1(0);
//		  rdp->bl->cycle1ModeDraw(j,i,c);
 
		//TODO: Implement this case
//		 printf("fillRect cycleType 0: NI\n");
     }
   else {
	   	   sprintf(txtbuffer,"rs:fillRect not fill mode ? %d", cycleType);
   	   DEBUG_print(txtbuffer,DBG_RSPINFO);
	}

}

void RSP::SETFILLCOLOR()
{
   //rdp->setFillColor(*(currentCommand+1));
//	printf("setfillcolor\n");

// Note that there are 2 fill colors here because 2 pixels are written per clock cycle in FILLRECT
// Ignoring 2nd fillcolor for now
	fillColor = *(currentCommand+1);
	GXfillColor.r = (u8) (((fillColor >> 27) & 0x1F) << 3) & ((fillColor >> 29) & 0x3);
	GXfillColor.g = (u8) (((fillColor >> 22) & 0x1F) << 3) & ((fillColor >> 24) & 0x3);
	GXfillColor.b = (u8) (((fillColor >> 17) & 0x1F) << 3) & ((fillColor >> 19) & 0x3);
	GXfillColor.a = (u8) ((fillColor >> 16) & 0x1) ? 255 : 0;

	bl->setFillColor(fillColor);
}

void RSP::SETFOGCOLOR()
{
   //rdp->setFogColor(*(currentCommand+1));
//	printf("setFogColor\n");

	bl->setFogColor((int) *(currentCommand+1));
}

void RSP::SETBLENDCOLOR()
{
   //rdp->setBlendColor(*(currentCommand+1));
//	printf("setBlendColor\n");

	//not used by soft_gfx
	bl->setBlendColor((int) *(currentCommand+1));
}

void RSP::SETPRIMCOLOR()
{
   float mLOD, lLOD;
   mLOD = ((*currentCommand >> 8) & 0xFF) / 256.0f;
   lLOD = ((*currentCommand) & 0xFF) / 256.0f;
   //rdp->setPrimColor(*(currentCommand+1), mLOD, lLOD);
//   printf("setPrimcolor\n");
    //  cc->setPrimColor(color, mLOD, lLOD);
	cc->setPrimColor((int) *(currentCommand+1),(float) mLOD,(float) lLOD);
}

void RSP::SETENVCOLOR()
{
   //rdp->setEnvColor(*(currentCommand+1));
//	printf("setEnvcolor\n");
	//   cc->setEnvColor(color);
	cc->setEnvColor((int) *(currentCommand+1));
}

void RSP::SETCOMBINE()
{
   //rdp->setCombineMode(*currentCommand & 0xFFFFFF, *(currentCommand+1));
//	printf("setcominemode\n");
	cc->setCombineMode((int) *currentCommand & 0xFFFFFF,(int) *(currentCommand+1));

}

void RSP::SETTIMG()
{
   int format = (*currentCommand >> 21) & 0x7;
   int size = (*currentCommand >> 19) & 0x3;
   int width = (*currentCommand & 0xFFF) + 1;
   void *timg = gfxInfo.RDRAM + (seg2phys(*(currentCommand+1)) & 0x7FFFFF);
   tx->setTImg(format, size, width, timg);
   TXwidth = width;
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
