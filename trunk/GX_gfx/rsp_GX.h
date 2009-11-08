/**
 * Wii64 - rsp_GX.h
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


#ifndef RSP_H
#define RSP_H

#include "../main/winlnxdefs.h"
#include "Gfx_#1.3.h"
//#include "rdp.h"
#include "tx_GX.h"
#include "bl_GX.h"
#include "cc_GX.h"
#include "color.h"
#include "vektor.h"
#include "matrix.h"

#include "ogc/gu.h"
#include <gccore.h>

typedef struct
{
   unsigned long type;
   unsigned long flags;
   
   unsigned long ucode_boot;
   unsigned long ucode_boot_size;

   unsigned long ucode;
   unsigned long ucode_size;
   
   unsigned long ucode_data;
   unsigned long ucode_data_size;
   
   unsigned long dram_stack;
   unsigned long dram_stack_size;
   
   unsigned long output_buff;
   unsigned long output_buff_size;
   
   unsigned long data_ptr;
   unsigned long data_size;
   
   unsigned long yield_data_ptr;
   unsigned long yield_data_size;
} OSTask_t;

typedef struct
{
   float vscale[4];
   float vtrans[4];
}  Vp_t;

typedef struct
{
   float sc;
   float tc;
   int level;
   bool enabled;
   int tile;
} TextureScales;

typedef struct 
{
   Color32   col; /* Diffuse light value (RGBA) */
   Color32   colc;/* Copy of diffuse light value (RGBA) */
   Vektor<float,4>     dir; /* Direction toward light source (normalized) */
    /* Important: the size of "dir" must not exceed 127  */
} Light_t;

typedef struct 
{
   Color32   col; /* Ambient light value (RGBA) */
   Color32   colc;/* Copy of ambient light value (RGBA) */
} Ambient_t;

typedef struct
{
   Vektor<float,4> v;
   float s;
   float t;
   Color32 c;
   Vektor<float,4> n;
   int alpha;
} Vertex;

typedef struct
{
   int newblock;
   int tile;
   float uls;
   float ult;
   float lrs;
   int dxt;
} TXBlock;

/*typedef struct
{
   int format;
   int size;
   int line;
   int tmem;
   int tile;
   int palette;
   int cmt;
   int maskt;
   int shiftt;
   int cms;
   int masks;
   int shifts;
} TXSetTile;*/

/*typedef struct
{
   u8 GXtexfmt;
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
} Descriptor;*/


class RSP;
typedef void (RSP::*COMMANDS)();

class RSP
{
   // general members
   GFX_INFO gfxInfo;
   //RDP *rdp;
   TX *tx;
   BL *bl;
   CC *cc;
   bool error;
   bool end;
   unsigned long *currentCommand;
   COMMANDS commands[0x100];

   // GX variables
   Mtx44 GXprojection;
   Mtx GXmodelView;
   Mtx GXnormal;
   Mtx44 GXprojection2D;
   Mtx GXmodelView2D;
   Mtx GXnormal2D;
   Matrix<float, 4> normal;
   bool GXuseMatrix;
   GXColor GXcol;
   GXLightObj GXlight;
   u8 light_mask;
   int fillColor;
   GXColor GXfillColor;
//   GXTexObj	GXtex;
//   GXTlutObj GXtlut;
   int numVector;
   int numVectorMP;
   int GXmtxStatus;	// 0 = invalid; 1 = 3D; 2 = 2D
   bool GXnew2Dproj; // flag to only use load the first viewport

   // rdp variables
   int cycleType;

   // texture img variables
/*   void *tImg;
   int TXformat;
   int TXsize;
   int textureLUT;*/
   int TXwidth;
   Descriptor descriptor[8];
   TXBlock TXblock;
//   TXSetTile TXsetTile;
   int currentTile;

   // rsp ucode variables
   unsigned long segments[16];
   Vp_t viewport;
   TextureScales textureScales;
   int numLight;
   int clipRatio_RNX;
   int clipRatio_RNY;
   int clipRatio_RPX;
   int clipRatio_RPY;
   Ambient_t ambientLight;
   Light_t spotLight[8];
   Light_t lookAtX;
   Light_t lookAtY;
   Matrix<float, 4> modelView;
   Matrix<float, 4> projection;
   Matrix<float, 4> MP;
   Vertex vtx[16];
   int fm;
   int fo;
   
   // geometry modes
   bool texture_gen_linear;
   bool texture_gen;
   bool lighting;
   bool fog;
   bool cull_back;
   bool cull_front;
   bool shading_smooth;
   bool shade;
   bool zbuffer;
   int geometryMode;
   
   // rsp commands
   void NI();
   void SPNOOP();
   void MTX();
   void MOVEMEM();
   void VTX();
   void DL();
   void SPRITE2D();
   void RDPHALF_1();
   void RDPHALF_2();
   void RDPHALF_CONT();
   void CLEARGEOMETRYMODE();
   void SETGEOMETRYMODE();
   void ENDDL();
   void SETOTHERMODE_L();
   void SETOTHERMODE_H();
   void TEXTURE();
   void MOVEWORD();
   void POPMTX();
   void TRI1();
   void TEXRECT();
   void RDPLOADSYNC();
   void RDPPIPESYNC();
   void RDPTILESYNC();
   void RDPFULLSYNC();
   void SETSCISSOR();
   void LOADTLUT();
   void SETTILESIZE();
   void LOADBLOCK();
   void LOADTILE();
   void SETTILE();
   void FILLRECT();
   void SETFILLCOLOR();
   void SETFOGCOLOR();
   void SETBLENDCOLOR();
   void SETPRIMCOLOR();
   void SETENVCOLOR();
   void SETCOMBINE();
   void SETTIMG();
   void SETZIMG();
   void SETCIMG();
   
   unsigned long seg2phys(unsigned long seg);
   void executeDList();
   
 public:
   RSP(GFX_INFO);
   ~RSP();
};

#endif
