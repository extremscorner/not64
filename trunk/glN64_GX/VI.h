#ifndef VI_H
#define VI_H
#include "Types.h"

struct VIInfo
{
	u32 width, height;
	u32 lastOrigin;
};

extern VIInfo VI;

void VI_UpdateSize();
void VI_UpdateScreen();

#ifdef __GX__

void VI_GX_init();
void VI_GX_setFB(unsigned int* fb1, unsigned int* fb2);
unsigned int* VI_GX_getScreenPointer();
void VI_GX_showFPS();
void VI_GX_showLoadProg(float percent);
void VI_GX_updateDEBUG();
void VI_GX_showDEBUG();
void VI_GX_showStats();
void VI_GX_cleanUp();

#endif // __GX__

#endif

