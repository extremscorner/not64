/* vi_GX.h - vi for Gamecube, based off vi_SDL
   by Mike Slegeir for Mupen64-GC
 */

#ifndef VI_GX_H
#define VI_GX_H

#include "vi.h"

class VI_GX : public VI
{
   bool updateDEBUGflag;
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
};

#endif
