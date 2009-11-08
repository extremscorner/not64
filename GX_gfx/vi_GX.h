/**
 * Wii64 - vi_GX.h
 * Copyright (C) 2007, 2008, 2009 Wii64 Team
 * 
 * vi for Gamecube by Mike Slegeir
 * load progress bar and GX hardware scaling by sepp256
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

#ifndef VI_GX_H
#define VI_GX_H

#include "vi.h"

class VI_GX : public VI
{
//   bool updateDEBUGflag;
   bool updateOSD;
   bool copy_fb;
   bool captureScreenFlag;
   unsigned int* xfb[2];
   int which_fb;
   int width;
   int height;
   Mtx44 GXprojection2D;
   Mtx GXmodelView2D;

   void setFB(unsigned int*, unsigned int*);
   void showFPS();
   void showDEBUG();
   virtual void setVideoMode(int w, int h);
//   virtual void* getScreenPointer();
   virtual void blit();
   
 public:
   VI_GX(GFX_INFO);
   virtual ~VI_GX();
   
   virtual void switchFullScreenMode();
   virtual void switchWindowMode();
   virtual void setGamma(float gamma);
   unsigned int* getScreenPointer();
   void showLoadProg(float);
   void updateDEBUG();
   void setCaptureScreen();
   void doCaptureScreen();
   void PreRetraceCallback(u32 retraceCnt);
};

#endif
