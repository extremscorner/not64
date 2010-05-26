/**
 * glN64_GX - Textures.h
 * Copyright (C) 2003 Orkin
 * Copyright (C) 2008, 2009 sepp256 (Port to Wii/Gamecube/PS3)
 *
 * glN64 homepage: http://gln64.emulation64.com
 * Wii64 homepage: http://www.emulatemii.com
 * email address: sepp256@gmail.com
 *
**/

#ifndef TEXTURES_H
#define TEXTURES_H

#ifndef __GX__
#include <GL/gl.h>
#else // !__GX__
#define __WIN32__
#include "gl.h" 
#include "glext.h"
#undef __WIN32__
#ifdef HW_RVL
#include "../gc_memory/MEM2.h"
#endif //HW_RVL
#endif // __GX__
#include "convert.h"

struct CachedTexture
{
#ifdef __GX__
	GXTexObj	GXtex;
	u16			*GXtexture;
	u8			GXtexfmt;
	u32			GXrealWidth, GXrealHeight;	// Actual dimensions of GX texture
	u32			VIcount;
#endif // __GX__

	GLuint	glName;
	u32		address;
	u32		crc;
//	float	fulS, fulT;
//	WORD	ulS, ulT, lrS, lrT;
	float	offsetS, offsetT;
	u32		maskS, maskT;
	u32		clampS, clampT;
	u32		mirrorS, mirrorT;
	u32		line;
	u32		size;
	u32		format;
	u32		tMem;
	u32		palette;
	u32		width, height;			  // N64 width and height
	u32		clampWidth, clampHeight;  // Size to clamp to
	u32		realWidth, realHeight;	  // Actual texture size
	f32		scaleS, scaleT;			  // Scale to map to 0.0-1.0
	f32		shiftScaleS, shiftScaleT; // Scale to shift
	u32		textureBytes;

	CachedTexture	*lower, *higher;
	u32		lastDList;

	u32		frameBufferTexture;
};


struct TextureCache
{
	CachedTexture	*bottom, *top;

	CachedTexture	*(current[2]);
	u32				maxBytes;
	u32				cachedBytes;
	u32				numCached;
	u32				hits, misses;
	GLuint			glNoiseNames[32];
	//GLuint			glDummyName;
	CachedTexture	*dummy;
	u32				enable2xSaI, bitDepth;
#ifdef __GX__
	int				VIcount;
	CachedTexture	*(GXprimDepthZ[2]);
	u32				GXprimDepthCnt,GXZTexPrimCnt,GXnoZTexPrimCnt;
#endif // __GX__
};

extern TextureCache cache;

#ifdef __GX__
# ifdef HW_RVL
# define GX_TEXTURE_CACHE_SIZE TEXCACHE_SIZE //8MB for Wii
# else //HW_RVL
# define GX_TEXTURE_CACHE_SIZE (2*1024*1024)
# endif //!HW_RVL
# define GX_MAX_TEXTURES (512*1024/sizeof( CachedTexture )) //Allow 512kB of Texture Meta
#endif //__GX__

inline u32 pow2( u32 dim )
{
	u32 i = 1;

	while (i < dim) i <<= 1;

	return i;
}

inline u32 powof( u32 dim )
{
	u32 num = 1;
	u32 i = 0;

	while (num < dim)
	{
		num <<= 1;
		i++;
	}

	return i;
}

CachedTexture *TextureCache_AddTop();
void TextureCache_MoveToTop( CachedTexture *newtop );
void TextureCache_Remove( CachedTexture *texture );
void TextureCache_RemoveBottom();
void TextureCache_Init();
void TextureCache_Destroy();
void TextureCache_Update( u32 t );
void TextureCache_ActivateTexture( u32 t, CachedTexture *texture );
void TextureCache_ActivateNoise( u32 t );
void TextureCache_ActivateDummy( u32 t );
BOOL TextureCache_Verify();
#ifdef __GX__
void TextureCache_FreeNextTexture();
void TextureCache_UpdatePrimDepthZtex( f32 z );
#endif // __GX__

#endif
