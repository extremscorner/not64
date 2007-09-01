/* InputPlugin.h - Mangles the plugin so it can be linked staticly
   by Mike Slegeir for Mupen64-GC
 */

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

