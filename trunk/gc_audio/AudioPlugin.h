/**
 * Wii64 - AudioPlugin.h
 * Copyright (C) 2007, 2008, 2009 Mike Slegeir
 * 
 * Mangles the plugin so it can be linked statically
 *
 * Wii64 homepage: http://www.emulatemii.com
 * email address: tehpola@gmail.com
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


#ifndef AUDIO_PLUGIN_H
#define AUDIO_PLUGIN_H

#define RomClosed  romClosed_audio
#define RomOpen    romOpen_audio
#define GetDllInfo getDllInfo_audio
#define DllConfig  dllConfig_audio
#define DllTest    dllTest_audio
#define DllAbout   dllAbout_audio
#define CloseDLL   closeDLL_audio

#define AiDacrateChanged aiDacrateChanged
#define AiLenChanged     aiLenChanged
#define AiReadLength     aiReadLength
#define AiUpdate         aiUpdate
#define InitiateAudio    initiateAudio
#define ProcessAlist     processAList

#endif

