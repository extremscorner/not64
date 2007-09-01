/* SoftGFXPlugin.h - Mangles the plugin so it can be linked staticly
   by Mike Slegeir for Mupen64-GC
 */

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

