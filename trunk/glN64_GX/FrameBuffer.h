/**
 * glN64_GX - FrameBuffer.cpp
 * Copyright (C) 2003 Orkin
 * Copyright (C) 2008, 2009 sepp256 (Port to Wii/Gamecube/PS3)
 *
 * glN64 homepage: http://gln64.emulation64.com
 * Wii64 homepage: http://www.emulatemii.com
 * email address: sepp256@gmail.com
 *
**/

#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include "Types.h"
#include "Textures.h"

struct FrameBuffer
{
	FrameBuffer *higher, *lower;

	CachedTexture *texture;

	u32 startAddress, endAddress;
	u32 size, width, height, changed;
	float scaleX, scaleY;
};

struct FrameBufferInfo
{
	FrameBuffer *top, *bottom, *current;
	int numBuffers;
};

extern FrameBufferInfo frameBuffer;

void FrameBuffer_Init();
void FrameBuffer_Destroy();
void FrameBuffer_SaveBuffer( u32 address, u16 size, u16 width, u16 height );
void FrameBuffer_RenderBuffer( u32 address );
void FrameBuffer_RestoreBuffer( u32 address, u16 size, u16 width );
void FrameBuffer_RemoveBuffer( u32 address );
FrameBuffer *FrameBuffer_FindBuffer( u32 address );
void FrameBuffer_ActivateBufferTexture( s16 t, FrameBuffer *buffer );
#ifdef __GX__
void FrameBuffer_RemoveBottom();
void FrameBuffer_MoveToTop( FrameBuffer *newtop );
void FrameBuffer_IncrementVIcount();
#endif //__GX__

#endif
