/* vi_GX.h - vi for Gamecube, based off vi_SDL
   by Mike Slegeir for Mupen64-GC
 */

#ifndef VI_GX_H
#define VI_GX_H

#include "vi.h"

class VI_GX : public VI
{
   unsigned int* xfb[2];
   int which_fb;
   int width;
   int height;
   
   void setFB(unsigned int*, unsigned int*);
   void showFPS();
   virtual void setVideoMode(int w, int h);
//   virtual void* getScreenPointer();
   virtual void blit();
   
 public:
   VI_GX(GFX_INFO);
   virtual ~VI_GX();
   
   virtual void* getScreenPointer();
   virtual void switchFullScreenMode();
   virtual void switchWindowMode();
   virtual void setGamma(float gamma);
};

#endif
