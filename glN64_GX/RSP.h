/**
 * glN64_GX - RSP.h
 * Copyright (C) 2003 Orkin
 * Copyright (C) 2008, 2009 sepp256 (Port to Wii/Gamecube/PS3)
 *
 * glN64 homepage: http://gln64.emulation64.com
 * Wii64 homepage: http://www.emulatemii.com
 * email address: sepp256@gmail.com
 *
**/

#ifndef RSP_H
#define RSP_H

#ifndef __LINUX__
# include <windows.h>
#else
# include "../main/winlnxdefs.h"
# ifndef __GX__
# include "SDL.h"
# include "SDL_thread.h"
# endif // !__GX__
#endif
#include "N64.h"
#include "GBI.h"
#include "gSP.h"
#include "Types.h"

#define RSPMSG_CLOSE			0
#define RSPMSG_UPDATESCREEN		1
#define RSPMSG_PROCESSDLIST		2
#define RSPMSG_CAPTURESCREEN	3
#define RSPMSG_DESTROYTEXTURES	4
#define RSPMSG_INITTEXTURES 	5

typedef struct
{
#ifdef RSPTHREAD
# ifndef __LINUX__
	HANDLE thread;
	// Events for thread messages, see defines at the top, or RSP_Thread
	HANDLE			threadMsg[6];
	// Event to notify main process that the RSP is finished with what it was doing
	HANDLE			threadFinished;
# else
	SDL_Thread *thread;
	int        threadMsg[6];
# endif // !__LINUX__
#endif // RSPTHREAD

	u32 PC[18], PCi, busy, halt, close, DList, uc_start, uc_dstart, cmd, nextCmd, count;
} RSPInfo;

extern RSPInfo RSP;

#define RSP_SegmentToPhysical( segaddr ) ((gSP.segment[(segaddr >> 24) & 0x0F] + (segaddr & 0x00FFFFFF)) & 0x00FFFFFF)

void RSP_Init();
void RSP_ProcessDList();
#ifdef RSPTHREAD
DWORD WINAPI RSP_ThreadProc( LPVOID lpParameter );
#endif
void RSP_LoadMatrix( f32 mtx[4][4], u32 address );

#endif
