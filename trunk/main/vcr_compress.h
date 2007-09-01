#include "../config.h"

#ifdef VCR_SUPPORT

#ifndef __VCR_COMPRESS_H__
#define __VCR_COMPRESS_H__

#if defined(__cplusplus)
extern "C" {
#endif

#define ATTRIB_INTEGER		1
#define ATTRIB_STRING		2
#define ATTRIB_SELECT		3
#define ATTRIB_FLOAT		4

void VCRComp_init();

void VCRComp_startFile( const char *filename, long width, long height, int fps );
void VCRComp_finishFile();
void VCRComp_addVideoFrame( const unsigned char *data );
void VCRComp_addAudioData( const unsigned char *data, int len );

int         VCRComp_numVideoCodecs();
const char *VCRComp_videoCodecName( int index );
int         VCRComp_numVideoCodecAttribs( int index );
const char *VCRComp_videoCodecAttribName( int cindex, int aindex );
int         VCRComp_videoCodecAttribKind( int cindex, int aindex );
const char *VCRComp_videoCodecAttribValue( int cindex, int aindex );
void        VCRComp_videoCodecAttribSetValue( int cindex, int aindex, const char *val );
int         VCRComp_numVideoCodecAttribOptions( int cindex, int aindex );
const char *VCRComp_videoCodecAttribOption( int cindex, int aindex, int oindex );
void        VCRComp_selectVideoCodec( int index );

int         VCRComp_numAudioCodecs();
const char *VCRComp_audioCodecName( int index );
int         VCRComp_numAudioCodecAttribs( int index );
const char *VCRComp_audioCodecAttribName( int cindex, int aindex );
int         VCRComp_audioCodecAttribKind( int cindex, int aindex );
const char *VCRComp_audioCodecAttribValue( int cindex, int aindex );
void        VCRComp_audioCodecAttribSetValue( int cindex, int aindex, const char *val );
int         VCRComp_numAudioCodecAttribOptions( int cindex, int aindex );
const char *VCRComp_audioCodecAttribOption( int cindex, int aindex, int oindex );
void        VCRComp_selectAudioCodec( int index );

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // __VCR_COMPRESS_H__

#endif // VCR_SUPPORT
