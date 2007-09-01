#ifndef __VCR_H__
#define __VCR_H__

#include "../config.h"

#ifdef VCR_SUPPORT

#ifndef __WIN32__
# include "winlnxdefs.h"
#else
# include <windows.h>
#endif

#include "plugin.h"


extern void VCR_getKeys( int Control, BUTTONS *Keys );
extern void VCR_updateScreen();
extern void VCR_aiDacrateChanged( int SystemType );
extern void VCR_aiLenChanged();


extern int VCR_startRecord( const char *filename );
extern int VCR_stopRecord();
extern int VCR_startPlayback( const char *filename );
extern int VCR_stopPlayback();
extern int VCR_startCapture( const char *recFilename, const char *aviFilename );
extern int VCR_stopCapture();

extern void VCR_coreStopped();

#endif // VCR_SUPPORT

#endif // __VCR_H__
