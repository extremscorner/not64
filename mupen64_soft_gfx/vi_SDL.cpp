/**
 * Mupen64 - vi_SDL.cpp
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

#include "vi_SDL.h"

VI_SDL::VI_SDL(GFX_INFO info) : VI(info), screen(NULL), width(0), height(0)
{
   SDL_InitSubSystem(SDL_INIT_VIDEO);
   SDL_ShowCursor(SDL_DISABLE);
}

VI_SDL::~VI_SDL()
{
   SDL_ShowCursor(SDL_ENABLE);
   SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

void VI_SDL::setVideoMode(int w, int h)
{
   width = w;
   height = h;
   screen = SDL_SetVideoMode(w, h, 15, 0);
}

void VI_SDL::switchFullScreenMode()
{
   screen = SDL_SetVideoMode(width, height, 15, SDL_FULLSCREEN);
}

void VI_SDL::switchWindowMode()
{
   screen = SDL_SetVideoMode(width, height, 15, 0);
}

void* VI_SDL::getScreenPointer()
{
   return screen->pixels;
}

void VI_SDL::blit()
{
   SDL_UpdateRect(screen, 0, 0, 0, 0);
   showFPS();
}

void VI_SDL::setGamma(float gamma)
{
   SDL_SetGamma(gamma, gamma, gamma);
}

void VI_SDL::showFPS()
{
   static unsigned long lastTick=0;
   static int frames=0;
   unsigned long nowTick = SDL_GetTicks();
   frames++;
   if (lastTick + 5000 <= nowTick)
     {
	char caption[200];
	sprintf(caption, "%.3f VI/S",
		frames/5.0);
	SDL_WM_SetCaption(caption, caption);
	frames = 0;
	lastTick = nowTick;
     }
}
