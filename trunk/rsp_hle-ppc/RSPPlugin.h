/* RSPPlugin.h - Mangles the plugin so it can be linked staticly
   by Mike Slegeir for Mupen64-GC
 */

#ifndef RSP_PLUGIN_H
#define RSP_PLUGIN_H

#define RomClosed  romClosed_RSP
#define RomOpen    romOpen_RSP
#define GetDllInfo getDllInfo_RSP
#define DllConfig  dllConfig_RSP
#define DllTest    dllTest_RSP
#define DllAbout   dllAbout_RSP
#define CloseDLL   closeDLL_RSP

#define GetRspDebugInfo     getRspDebugInfo
#define InitiateRSP         initiateRSP
#define InitiateRSPDebugger initiateRSPDebugger

#define DoRspCycles         doRspCycles

#endif

