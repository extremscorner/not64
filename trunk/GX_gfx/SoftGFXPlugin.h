/**
 * Wii64 - SoftGFXPlugin.h
 * Copyright (C) 2007 Mike Slegeir
 * 
 * Mangles the plugin so it can be linked staticly
 *
 * Wii64 homepage: http://www.emulatemii.com
 * email address: wii64team@gmail.com
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

#ifndef GFX_PLUGIN_H
#define GFX_PLUGIN_H

#define RomClosed  romClosed_gfx
#define RomOpen    romOpen_gfx
#define GetDllInfo getDllInfo_gfx
#define DllConfig  dllConfig_gfx
#define DllTest    dllTest_gfx
#define DllAbout   dllAbout_gfx
#define CloseDLL   closeDLL_gfx

#define CaptureScreen   captureScreen
#define ChangeWindow    changeWindow
#define DrawScreen      drawScreen
#define InitiateGFX     initiateGFX
#define MoveScreen      moveScreen
#define ProcessDList    processDList
#define ProcessRDPList  processRDPList
#define ShowCFB         showCFB
#define UpdateScreen    updateScreen
#define ViStatusChanged viStatusChanged
#define ViWidthChanged  viWidthChanged

#endif

