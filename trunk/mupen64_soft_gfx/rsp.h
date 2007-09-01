/**
 * Mupen64 hle rsp - rsp.h
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

#ifndef RSP_H
#define RSP_H

#include "../main/winlnxdefs.h"
#include "Gfx_#1.3.h"
#include "rdp.h"
#include "color.h"
#include "vector.h"
#include "matrix.h"

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
   Vector<float,4>     dir; /* Direction toward light source (normalized) */
    /* Important: the size of "dir" must not exceed 127  */
} Light_t;

typedef struct 
{
   Color32   col; /* Ambient light value (RGBA) */
   Color32   colc;/* Copy of ambient light value (RGBA) */
} Ambient_t;

typedef struct
{
   Vector<float,4> v;
   float s;
   float t;
   Color32 c;
   Vector<float,4> n;
   int alpha;
} Vtx;

class RSP;
typedef void (RSP::*COMMANDS)();

class RSP
{
   // general members
   GFX_INFO gfxInfo;
   RDP *rdp;
   bool error;
   bool end;
   unsigned long *currentCommand;
   COMMANDS commands[0x100];
   
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
   Vtx vtx[16];
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
