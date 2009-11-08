/**
 * Wii64 - main.cpp
 * Copyright (C) 2002 Hacktarux 
 * Copyright (C) 2007, 2008 sepp256
 * 
 * N64 GX plugin, based off Hacktarux's soft_gfx
 * by sepp256 for Mupen64-GC
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

#include <string.h>
#include <stdio.h>

#ifdef __PPC__
#include "SoftGFXPlugin.h"
#endif

#ifndef _WIN32
#include "../main/winlnxdefs.h"
#else
#include <windows.h>
#endif
#include "Gfx_#1.3.h"
#include "rsp_GX.h"

#ifdef __PPC__
#include "vi_GX.h"
#else // __PPC__
#ifndef _WIN32
#include "vi_SDL.h"
#else
#include "./win/vi.win.h"
#endif
#endif

static GFX_INFO gfxInfo;

VI *vi;

void gfx_PreRetraceCallback(u32 retraceCnt) {
	vi->PreRetraceCallback(retraceCnt);
}

void gfx_set_fb(unsigned int* fb1, unsigned int* fb2){
	vi->setFB(fb1, fb2);
}

void showLoadProgress(float percent){
	vi->showLoadProg(percent);
}

/******************************************************************
  Function: CaptureScreen
  Purpose:  This function dumps the current frame to a file
  input:    pointer to the directory to save the file to
  output:   none
*******************************************************************/ 
EXPORT void CALL CaptureScreen ( char * Directory )
{
	vi->setCaptureScreen();
}

/******************************************************************
  Function: ChangeWindow
  Purpose:  to change the window between fullscreen and window 
            mode. If the window was in fullscreen this should 
			change the screen to window mode and vice vesa.
  input:    none
  output:   none
*******************************************************************/ 
EXPORT void CALL ChangeWindow (void)
{
   static bool fullScreen = false;
   if (!fullScreen)
     {
	vi->switchFullScreenMode();
	fullScreen = true;
     }
   else
     {
	vi->switchWindowMode();
	fullScreen = false;
     }
}

/******************************************************************
  Function: CloseDLL
  Purpose:  This function is called when the emulator is closing
            down allowing the dll to de-initialise.
  input:    none
  output:   none
*******************************************************************/ 
EXPORT void CALL CloseDLL (void)
{
}

/******************************************************************
  Function: DllAbout
  Purpose:  This function is optional function that is provided
            to give further information about the DLL.
  input:    a handle to the window that calls this function
  output:   none
*******************************************************************/ 
EXPORT void CALL DllAbout ( HWND hParent )
{
}

/******************************************************************
  Function: DllConfig
  Purpose:  This function is optional function that is provided
            to allow the user to configure the dll
  input:    a handle to the window that calls this function
  output:   none
*******************************************************************/ 
EXPORT void CALL DllConfig ( HWND hParent )
{
}

/******************************************************************
  Function: DllTest
  Purpose:  This function is optional function that is provided
            to allow the user to test the dll
  input:    a handle to the window that calls this function
  output:   none
*******************************************************************/ 
EXPORT void CALL DllTest ( HWND hParent )
{
}

/******************************************************************
  Function: DrawScreen
  Purpose:  This function is called when the emulator receives a
            WM_PAINT message. This allows the gfx to fit in when
			it is being used in the desktop.
  input:    none
  output:   none
*******************************************************************/ 
EXPORT void CALL DrawScreen (void)
{
}

/******************************************************************
  Function: GetDllInfo
  Purpose:  This function allows the emulator to gather information
            about the dll by filling in the PluginInfo structure.
  input:    a pointer to a PLUGIN_INFO stucture that needs to be
            filled by the function. (see def above)
  output:   none
*******************************************************************/ 
EXPORT void CALL GetDllInfo ( PLUGIN_INFO * PluginInfo )
{
   PluginInfo->Version = 0x0103;
   PluginInfo->Type = PLUGIN_TYPE_GFX;
   strcpy(PluginInfo->Name, "Mupen64 GX gfx plugin (very unstable beta)");
   PluginInfo->NormalMemory = TRUE;
   PluginInfo->MemoryBswaped = TRUE;
}

/******************************************************************
  Function: InitiateGFX
  Purpose:  This function is called when the DLL is started to give
            information from the emulator that the n64 graphics
			uses. This is not called from the emulation thread.
  Input:    Gfx_Info is passed to this function which is defined
            above.
  Output:   TRUE on success
            FALSE on failure to initialise
             
  ** note on interrupts **:
  To generate an interrupt set the appropriate bit in MI_INTR_REG
  and then call the function CheckInterrupts to tell the emulator
  that there is a waiting interrupt.
*******************************************************************/ 
EXPORT BOOL CALL InitiateGFX (GFX_INFO Gfx_Info)
{
   gfxInfo = Gfx_Info;
   return TRUE;
}

/******************************************************************
  Function: MoveScreen
  Purpose:  This function is called in response to the emulator
            receiving a WM_MOVE passing the xpos and ypos passed
			from that message.
  input:    xpos - the x-coordinate of the upper-left corner of the
            client area of the window.
			ypos - y-coordinate of the upper-left corner of the
			client area of the window. 
  output:   none
*******************************************************************/ 
EXPORT void CALL MoveScreen (int xpos, int ypos)
{
#ifdef _WIN32
   vi->moveScreen(xpos, ypos);
#endif // _WIN32
}

/******************************************************************
  Function: ProcessDList
  Purpose:  This function is called when there is a Dlist to be
            processed. (High level GFX list)
  input:    none
  output:   none
*******************************************************************/ 
EXPORT void CALL ProcessDList(void)
{
//   GX_SetCopyClear ((GXColor){0,0,0,255}, 0x000000);
   GX_SetCopyClear ((GXColor){0,0,0,255}, 0xFFFFFF);
   GX_CopyDisp (vi->getScreenPointer(), GX_TRUE);	//clear the EFB before executing new Dlist
   GX_DrawDone ();
   RSP rsp(gfxInfo);
   vi->updateDEBUG();
   /*static int firstTime=0;
   
   if(firstTime< 1)
     {
	firstTime++;
	
	bool found = true;
	int level = 1;
	OSTask_t *task = (OSTask_t*)(gfxInfo.DMEM+0xFC0);
	char *udata = (char*)gfxInfo.RDRAM + task->ucode_data;
	int length = task->ucode_data_size;
	
	while (found)
	  {
	     printf("searching strings... (level %d)\n", level);
	     found = false;
	     for (int i=level; i<length; i++)
	       {
		  if(udata[i^3] >= 32 && udata[i^3] <=126)
		    {
		       bool isString = true;
		       for (int j=1; j<=level; j++)
			 if (udata[(i-j)^3] < 32 || udata[(i-j)^3] > 126)
			   isString = false;
		       if (isString)
			 {
			    found = true;
			    for (int j=level; j>=0; j--)
			      printf("%c", udata[(i-j)^3]);
			    printf("\n");
			    getchar();
			 }
		    }
	       }
	     level++;
	  }
     }*/
}

/******************************************************************
  Function: ProcessRDPList
  Purpose:  This function is called when there is a Dlist to be
            processed. (Low level GFX list)
  input:    none
  output:   none
*******************************************************************/ 
EXPORT void CALL ProcessRDPList(void)
{
}

/******************************************************************
  Function: RomClosed
  Purpose:  This function is called when a rom is closed.
  input:    none
  output:   none
*******************************************************************/ 
EXPORT void CALL RomClosed (void)
{
   //vi->setGamma(1.0);
   VIDEO_SetPreRetraceCallback(NULL);
   delete vi;
}

/******************************************************************
  Function: RomOpen
  Purpose:  This function is called when a rom is open. (from the 
            emulation thread)
  input:    none
  output:   none
*******************************************************************/ 
EXPORT void CALL RomOpen (void)
{
#ifdef __PPC__
   vi = new VI_GX(gfxInfo);
   VIDEO_SetPreRetraceCallback(gfx_PreRetraceCallback);
#else // __PPC__
#ifndef _WIN32
   vi = new VI_SDL(gfxInfo);
   //vi->setGamma(2.222);
   vi->setGamma(1.0);
#else
   vi = new VI_WIN(gfxInfo);
   //vi->setGamma(2.222);
   vi->setGamma(1.0);
#endif
#endif
}

/******************************************************************
  Function: ShowCFB
  Purpose:  Useally once Dlists are started being displayed, cfb is
            ignored. This function tells the dll to start displaying
			them again.
  input:    none
  output:   none
*******************************************************************/ 
EXPORT void CALL ShowCFB (void)
{
}

/******************************************************************
  Function: UpdateScreen
  Purpose:  This function is called in response to a vsync of the
            screen were the VI bit in MI_INTR_REG has already been
			set
  input:    none
  output:   none
*******************************************************************/ 
EXPORT void CALL UpdateScreen (void)
{
   vi->updateScreen();
}

/******************************************************************
  Function: ViStatusChanged
  Purpose:  This function is called to notify the dll that the
            ViStatus registers value has been changed.
  input:    none
  output:   none
*******************************************************************/ 
EXPORT void CALL ViStatusChanged (void)
{
   vi->statusChanged();
}

/******************************************************************
  Function: ViWidthChanged
  Purpose:  This function is called to notify the dll that the
            ViWidth registers value has been changed.
  input:    none
  output:   none
*******************************************************************/ 
EXPORT void CALL ViWidthChanged (void)
{
   vi->widthChanged();
}


