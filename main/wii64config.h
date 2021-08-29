/**
 * Wii64 - wii64config.h
 * Copyright (C) 2007, 2008, 2009 sepp256
 * 
 * External declaration and enumeration of config variables
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


#ifndef WII64CONFIG_H
#define WII64CONFIG_H

#include "timers.h"


extern char skipMenu;
enum skipMenu
{
	SKIPMENU_DISABLE=0,
	SKIPMENU_ENABLE
};

extern char audioEnabled;
enum audioEnabled
{
	AUDIO_DISABLE=0,
	AUDIO_ENABLE
};

extern char scalePitch;
enum scalePitch
{
	SCALEPITCH_DISABLE=0,
	SCALEPITCH_ENABLE
};

extern char showFPSonScreen;
enum showFPSonScreen
{
	FPS_HIDE=0,
	FPS_SHOW
};

extern char printToScreen;
enum printToScreen
{
	DEBUG_HIDE=0,
	DEBUG_SHOW
};

extern char printToSD;
enum printToSD
{
	SDLOG_DISABLE=0,
	SDLOG_ENABLE
};

extern timers Timers;			//Timers.limitVIs: 
enum limitVIs
{
	LIMITVIS_NONE=0,
	LIMITVIS_WAIT_FOR_VI,
	LIMITVIS_WAIT_FOR_FRAME
};

extern char nativeSaveDevice;
enum nativeSaveDevice
{
	NATIVESAVEDEVICE_NONE=-1,
	NATIVESAVEDEVICE_SD,
	NATIVESAVEDEVICE_FAT,
	NATIVESAVEDEVICE_CARDA,
	NATIVESAVEDEVICE_CARDB
};

extern char saveStateDevice;
enum saveStateDevice
{
	SAVESTATEDEVICE_SD=0,
	SAVESTATEDEVICE_FAT
};

extern char autoLoadSave;
enum autoLoadSave
{
	AUTOLOADSAVE_DISABLE=0,
	AUTOLOADSAVE_ENABLE
};

extern char autoSave;
enum autoSave
{
	AUTOSAVE_DISABLE=0,
	AUTOSAVE_ENABLE
};

extern char creditsScrolling;	//deprecated?

extern unsigned long dynacore;
enum dynacore
{
	DYNACORE_INTERPRETER=0,
	DYNACORE_DYNAREC,
	DYNACORE_PURE_INTERP
};

extern unsigned long count_per_op;
enum count_per_op
{
	COUNT_PER_OP_1=1,
	COUNT_PER_OP_2,
	COUNT_PER_OP_3
};

extern char screenMode;
enum screenMode
{
	SCREENMODE_4x3=0,
	SCREENMODE_16x9,
	SCREENMODE_16x9_PILLARBOX
};

extern char videoFormat;
enum videoFormat
{
	VIDEOFORMAT_NTSC=0,
	VIDEOFORMAT_PAL,
	VIDEOFORMAT_MPAL
};

extern char videoMode;
enum videoMode
{
	VIDEOMODE_AUTO=0,
	VIDEOMODE_480I,
	VIDEOMODE_480SF,
	VIDEOMODE_240P,
	VIDEOMODE_480P,
	VIDEOMODE_576I,
	VIDEOMODE_576SF,
	VIDEOMODE_288P,
	VIDEOMODE_576P
};

extern char videoWidth;
enum videoWidth
{
	VIDEOWIDTH_640=0,
	VIDEOWIDTH_704,
	VIDEOWIDTH_720,
	VIDEOWIDTH_1280,
	VIDEOWIDTH_1408,
	VIDEOWIDTH_1440
};

extern char pixelClock;
enum pixelClock
{
	PIXELCLOCK_AUTO=0,
	PIXELCLOCK_27MHZ,
	PIXELCLOCK_54MHZ
};

extern char trapFilter;
enum trapFilter
{
	TRAPFILTER_DISABLE=0,
	TRAPFILTER_ENABLE
};

extern char pollRate;
enum pollRate
{
	POLLRATE_VSYNC=0,
	POLLRATE_1MS,
	POLLRATE_2MS,
	POLLRATE_3MS,
	POLLRATE_4MS,
	POLLRATE_5MS,
	POLLRATE_6MS,
	POLLRATE_7MS,
	POLLRATE_8MS,
	POLLRATE_9MS,
	POLLRATE_10MS,
	POLLRATE_11MS
};

extern char padAutoAssign;
enum padAutoAssign
{
	PADAUTOASSIGN_MANUAL=0,
	PADAUTOASSIGN_AUTOMATIC
};

extern char padType[4];
enum padType
{
	PADTYPE_NONE=-1,
	PADTYPE_N64,
	PADTYPE_GAMECUBE,
	PADTYPE_WII
};

extern char padAssign[4];
enum padAssign
{
	PADASSIGN_INPUT0=0,
	PADASSIGN_INPUT1,
	PADASSIGN_INPUT2,
	PADASSIGN_INPUT3
};

extern char pakMode[4];
enum pakMode
{
	PAKMODE_MEMPAK=0,
	PAKMODE_RUMBLEPAK
};

extern char loadButtonSlot;
enum loadButtonSlot
{
	LOADBUTTON_SLOT0=0,
	LOADBUTTON_SLOT1,
	LOADBUTTON_SLOT2,
	LOADBUTTON_SLOT3,
	LOADBUTTON_DEFAULT
};


//#ifdef GLN64_GX
extern char glN64_useFrameBufferTextures;
enum glN64_useFrameBufferTextures
{
	GLN64_FBTEX_DISABLE=0,
	GLN64_FBTEX_ENABLE
};

extern char glN64_use2xSaiTextures;
enum glN64_use2xSaiTextures
{
	GLN64_2XSAI_DISABLE=0,
	GLN64_2XSAI_ENABLE
};

extern char renderCpuFramebuffer;
enum renderCpuFramebuffer
{
	CPUFRAMEBUFFER_DISABLE=0,
	CPUFRAMEBUFFER_ENABLE
};
//#endif //GLN64_GX


#endif //WII64CONFIG_H
