/**
 * Wii64 - InputPlugin.h
 * Copyright (C) 2007, 2008, 2009 Mike Slegeir
 * 
 * Mangles the plugin so it can be linked staticly
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


#ifndef INPUT_PLUGIN_H
#define INPUT_PLUGIN_H

#define RomClosed  romClosed_input
#define RomOpen    romOpen_input
#define GetDllInfo getDllInfo_input
#define DllConfig  dllConfig_input
#define DllTest    dllTest_input
#define DllAbout   dllAbout_input
#define CloseDLL   closeDLL_input

#define ControllerCommand   controllerCommand
#define GetKeys             getKeys
#define InitiateControllers initiateControllers
#define ReadController      readController
#define WM_KeyDown          wM_KeyDown
#define WM_KeyUp            wM_KeyUp

#endif

