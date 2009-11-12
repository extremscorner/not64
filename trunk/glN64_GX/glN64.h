/**
 * glN64_GX - glN64.h
 * Copyright (C) 2003 Orkin
 *
 * glN64 homepage: http://gln64.emulation64.com
 * Wii64 homepage: http://www.emulatemii.com
 *
**/

#ifndef GLN64_H
#define GLN64_H

#ifndef __LINUX__
#include <windows.h>
//#include <commctrl.h>
#else
# include "../main/winlnxdefs.h"
#endif

//#define DEBUG
//#define RSPTHREAD

#ifndef __LINUX__
extern HWND			hWnd;
//extern HWND			hFullscreen;
extern HWND			hStatusBar;
extern HWND			hToolBar;
extern HINSTANCE	hInstance;
#endif // !__LINUX__

extern char			pluginName[];

extern void (*CheckInterrupts)( void );
extern char *screenDirectory;

#endif

