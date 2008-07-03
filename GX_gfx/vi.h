/**
 * Mupen64 - vi.h
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

#ifndef VI_H
#define VI_H

#include "../main/winlnxdefs.h"
#include "Gfx_#1.3.h"

class VI
{
   GFX_INFO gfxInfo;
   int bpp;
   
   // the plugin assume the class that's implementing these functions 
   // accept 16bpp 5551 mode, maybe conversion is needed on some systems
   // it's also assumed that it's initialized in window mode by default
   virtual void setVideoMode(int w, int h) = 0;
//   virtual void* getScreenPointer() = 0;
   virtual void blit() = 0;
   
 public:
   VI(GFX_INFO);
   virtual ~VI();
   
   virtual unsigned int* getScreenPointer() = 0;
   virtual void setFB(unsigned int*, unsigned int*) = 0;
   virtual void switchFullScreenMode() = 0;
   virtual void switchWindowMode() = 0;
   virtual void setGamma(float gamma) = 0;
   virtual void showLoadProg(float) = 0;
   virtual void updateDEBUG() = 0;
   virtual void setCaptureScreen() = 0;
   virtual void doCaptureScreen() = 0;
   virtual void PreRetraceCallback(u32) = 0;
   void statusChanged();
   void widthChanged();
   void updateScreen();
   void debug_plot(int x, int y, int c);
   void flush();
};


#endif // VI_H
