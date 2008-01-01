/* vi_GX.cpp - vi for Gamecube, based off vi_GX
   by Mike Slegeir for Mupen64-GC
 */

#include <stdio.h>
#include <ogc/system.h>
#include <ogc/video.h>
#include "vi_GX.h"
#include "font.h" //This file is outdated

VI_GX::VI_GX(GFX_INFO info) : VI(info), width(0), height(0), which_fb(1){
	init_font();
	// FIXME: Instead of creating our own fb, we should use main's
	//xfb[0] = (unsigned int*) MEM_K0_TO_K1(SYS_AllocateFramebuffer(&TVNtsc480IntDf));
	//xfb[1] = (unsigned int*) MEM_K0_TO_K1(SYS_AllocateFramebuffer(&TVNtsc480IntDf));
}

void VI_GX::setFB(unsigned int* fb1, unsigned int* fb2){
	xfb[0] = fb1;
	xfb[1] = fb2;
}

VI_GX::~VI_GX(){
	// FIXME: If we create our own fb, we need to free it somehow
}

void VI_GX::setVideoMode(int w, int h){
   width = w;
   height = h;
}

void VI_GX::switchFullScreenMode(){ }

void VI_GX::switchWindowMode(){ }

void* VI_GX::getScreenPointer(){ return xfb[which_fb]; }

void VI_GX::blit(){
	//printf("Should be blitting.");
//	showFPS(); //Doesn't work with the GX font setup
	VIDEO_SetNextFramebuffer(xfb[which_fb]);
	VIDEO_Flush();
	which_fb ^= 1;
	VIDEO_WaitVSync();
}

void VI_GX::setGamma(float gamma){ }

extern "C" {
extern long long gettime();
extern unsigned int diff_sec(long long start,long long end);
};

void VI_GX::showFPS(){
	static long long lastTick=0;
	static int frames=0;
	static char caption[16];
	
	long long nowTick = gettime();
	frames++;
	if (diff_sec(lastTick,nowTick)>=1) {
		sprintf(caption, "%02d FPS",frames);
		frames = 0;
		lastTick = nowTick;
	}
	
	write_font(10,10,caption,xfb,which_fb);
}
