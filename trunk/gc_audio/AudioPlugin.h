/* AudioPlugin.h - Mangles the plugin so it can be linked staticly
   by Mike Slegeir for Mupen64-GC
 */

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

