/**
 * glN64_GX - FrameBuffer.cpp
 * Copyright (C) 2003 Orkin
 * Copyright (C) 2008, 2009, 2010 sepp256 (Port to Wii/Gamecube/PS3)
 *
 * glN64 homepage: http://gln64.emulation64.com
 * Wii64 homepage: http://www.emulatemii.com
 * email address: sepp256@gmail.com
 *
**/

#ifdef __GX__
#include <gccore.h>
#include <ogc/lwp_heap.h>
#include <string.h>
#endif // __GX__

#ifndef __LINUX__
# include <windows.h>
#else
# include "../main/winlnxdefs.h"
# include <stdlib.h>
#endif // __LINUX__
#include "OpenGL.h"
#include "FrameBuffer.h"
#include "RSP.h"
#include "RDP.h"
#include "Textures.h"
#include "Combiner.h"
#include "Types.h"

FrameBufferInfo frameBuffer;

#ifdef __GX__
extern heap_cntrl* GXtexCache;
#endif //__GX__

void FrameBuffer_Init()
{
	frameBuffer.current = NULL;
	frameBuffer.top = NULL;
	frameBuffer.bottom = NULL;
	frameBuffer.numBuffers = 0;
}

void FrameBuffer_RemoveBottom()
{
	FrameBuffer *newBottom = frameBuffer.bottom->higher;

#ifdef __GX__
	frameBuffer.bottom->texture->frameBufferTexture = false;
#endif //__GX__
	TextureCache_Remove( frameBuffer.bottom->texture );

	if (frameBuffer.bottom == frameBuffer.top)
		frameBuffer.top = NULL;

#ifdef __GX__
	if (gDP.loadTile->frameBuffer == frameBuffer.bottom)
		gDP.loadTile->frameBuffer = NULL;
#endif //__GX__

	free( frameBuffer.bottom );

    frameBuffer.bottom = newBottom;
	
	if (frameBuffer.bottom != NULL)
		frameBuffer.bottom->lower = NULL;

	frameBuffer.numBuffers--;
}

void FrameBuffer_Remove( FrameBuffer *buffer )
{
	if ((buffer == frameBuffer.bottom) &&
		(buffer == frameBuffer.top))
	{
		frameBuffer.top = NULL;
		frameBuffer.bottom = NULL;
	}
	else if (buffer == frameBuffer.bottom)
	{
		frameBuffer.bottom = buffer->higher;

		if (frameBuffer.bottom)
			frameBuffer.bottom->lower = NULL;
	}
	else if (buffer == frameBuffer.top)
	{
		frameBuffer.top = buffer->lower;

		if (frameBuffer.top)
			frameBuffer.top->higher = NULL;
	}
	else
	{
		buffer->higher->lower = buffer->lower;
		buffer->lower->higher = buffer->higher;
	}

	if (buffer->texture)
		TextureCache_Remove( buffer->texture );

#ifdef __GX__
	if (gDP.loadTile->frameBuffer == buffer)
		gDP.loadTile->frameBuffer = NULL;
#endif //__GX__

	free( buffer );

	frameBuffer.numBuffers--;
}

void FrameBuffer_RemoveBuffer( u32 address )
{
	FrameBuffer *current = frameBuffer.bottom;

	while (current != NULL)
	{
		if (current->startAddress == address)
		{
			current->texture = NULL;
			FrameBuffer_Remove( current );
			return;
		}
		current = current->higher;
	}
}

FrameBuffer *FrameBuffer_AddTop()
{
	FrameBuffer *newtop = (FrameBuffer*)malloc( sizeof( FrameBuffer ) );

	newtop->texture = TextureCache_AddTop();
#ifdef __GX__
	newtop->texture->VIcount = 0;
#endif //__GX__

	newtop->lower = frameBuffer.top;
	newtop->higher = NULL;

	if (frameBuffer.top)
		frameBuffer.top->higher = newtop;

	if (!frameBuffer.bottom)
		frameBuffer.bottom = newtop;

    frameBuffer.top = newtop;

	frameBuffer.numBuffers++;

	return newtop;
}

void FrameBuffer_MoveToTop( FrameBuffer *newtop )
{
	if (newtop == frameBuffer.top)
		return;

	if (newtop == frameBuffer.bottom)
	{
		frameBuffer.bottom = newtop->higher;
		frameBuffer.bottom->lower = NULL;
	}
	else
	{
		newtop->higher->lower = newtop->lower;
		newtop->lower->higher = newtop->higher;
	}

	newtop->higher = NULL;
	newtop->lower = frameBuffer.top;
	frameBuffer.top->higher = newtop;
	frameBuffer.top = newtop;

	TextureCache_MoveToTop( newtop->texture );
#ifdef __GX__
	newtop->texture->VIcount = 0;
#endif //__GX__
}

void FrameBuffer_Destroy()
{
	while (frameBuffer.bottom)
		FrameBuffer_RemoveBottom();
}

void FrameBuffer_SaveBuffer( u32 address, u16 size, u16 width, u16 height )
{
	FrameBuffer *current = frameBuffer.top;

	// Search through saved frame buffers
	while (current != NULL)
	{
		if ((current->startAddress == address) &&
			(current->width == width) &&
			(current->height == height) &&
			(current->size == size))
		{
#ifndef __GX__
			if ((current->scaleX != OGL.scaleX) ||
				(current->scaleY != OGL.scaleY))
#else //!__GX__
			if ((current->scaleX != OGL.GXscaleX) ||
				(current->scaleY != OGL.GXscaleY))
#endif //__GX__
			{
				FrameBuffer_Remove( current );
				break;
			}
#ifndef __GX__
			glBindTexture( GL_TEXTURE_2D, current->texture->glName );
			glCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, OGL.height - current->texture->height + OGL.heightOffset, current->texture->width, current->texture->height );
#else // !__GX__
			//Note: texture realWidth and realHeight should be multiple of 2!
			GX_SetTexCopySrc(OGL.GXorigX, OGL.GXorigY,(u16) current->texture->realWidth,(u16) current->texture->realHeight);
			GX_SetTexCopyDst((u16) current->texture->realWidth,(u16) current->texture->realHeight, current->texture->GXtexfmt, GX_FALSE);
			if (current->texture->GXtexture) GX_CopyTex(current->texture->GXtexture, GX_FALSE);
			GX_PixModeSync();
#endif // __GX__

			*(u32*)&RDRAM[current->startAddress] = current->startAddress;

			current->changed = TRUE;

			FrameBuffer_MoveToTop( current );

			gSP.changed |= CHANGED_TEXTURE;
			return;
		}
		current = current->lower;
	}

	// Wasn't found, create a new one
	current = FrameBuffer_AddTop();

	current->startAddress = address;
	current->endAddress = address + ((width * height << size >> 1) - 1);
	current->width = width;
	current->height = height;
	current->size = size;
#ifndef __GX__
	current->scaleX = OGL.scaleX;
	current->scaleY = OGL.scaleY;
#else //!__GX__
	current->scaleX = OGL.GXscaleX;
	current->scaleY = OGL.GXscaleY;
#endif //__GX__

#ifndef __GX__
	current->texture->width = (unsigned long)(current->width * OGL.scaleX);
	current->texture->height = (unsigned long)(current->height * OGL.scaleY);
#else //!__GX__
	current->texture->width = (unsigned long)(current->width * OGL.GXscaleX);
	current->texture->height = (unsigned long)(current->height * OGL.GXscaleY);
#endif //__GX__
	current->texture->clampS = 1;
	current->texture->clampT = 1;
	current->texture->address = current->startAddress;
	current->texture->clampWidth = current->width;
	current->texture->clampHeight = current->height;
	current->texture->frameBufferTexture = TRUE;
	current->texture->maskS = 0;
	current->texture->maskT = 0;
	current->texture->mirrorS = 0;
	current->texture->mirrorT = 0;
#ifndef __GX__
	current->texture->realWidth = pow2( (unsigned long)(current->width * OGL.scaleX) );
	current->texture->realHeight = pow2( (unsigned long)(current->height * OGL.scaleY) );
	current->texture->textureBytes = current->texture->realWidth * current->texture->realHeight * 4;
#else //!__GX__
	//realWidth & realHeight should be multiple of 2 for EFB->Texture Copy
	if(current->texture->width & 0x1)	current->texture->realWidth = current->texture->width + 1;
	else								current->texture->realWidth = current->texture->width;
	if(!current->texture->realWidth)	current->texture->realWidth = 2;

	if(current->texture->height & 0x1)	current->texture->realHeight = current->texture->height + 1;
	else								current->texture->realHeight = current->texture->height;
	if(!current->texture->realHeight)	current->texture->realHeight = 2;

/*	if(current->texture->width % 2 || current->texture->width == 0)
		current->texture->realWidth = current->texture->width + 2 - (current->texture->width % 2);
	else
		current->texture->realWidth = current->texture->width;
	if(current->texture->height % 2 || current->texture->height == 0)
		current->texture->realHeight = current->texture->height + 2 - (current->texture->height % 2);
	else
		current->texture->realHeight = current->texture->height;*/

	//GXrealWidth and GXrealHeight should be multiple of 4 for GXtexture
	if(current->texture->realWidth & 0x2)	current->texture->GXrealWidth = current->texture->realWidth + 2;
	else									current->texture->GXrealWidth = current->texture->realWidth;
	if(current->texture->realHeight & 0x2)	current->texture->GXrealHeight = current->texture->realHeight + 2;
	else									current->texture->GXrealHeight = current->texture->realHeight;

/*	if(current->texture->realWidth % 4)
		current->texture->GXrealWidth = current->texture->realWidth + 4 - (current->texture->realWidth % 4);
	else
		current->texture->GXrealWidth = current->texture->realWidth;
	if(current->texture->realHeight % 4)
		current->texture->GXrealHeight = current->texture->realHeight + 4 - (current->texture->realHeight % 4);
	else
		current->texture->GXrealHeight = current->texture->realHeight;*/

#ifdef HW_RVL
	current->texture->textureBytes = (current->texture->GXrealWidth * current->texture->GXrealHeight) * 4;
	current->texture->GXtexfmt = GX_TF_RGBA8;
#else //HW_RVL
	current->texture->textureBytes = (current->texture->GXrealWidth * current->texture->GXrealHeight) * 2;
	current->texture->GXtexfmt = GX_TF_RGB565;
#endif //!HW_RVL

	current->texture->GXtexture = (u16*) __lwp_heap_allocate(GXtexCache,current->texture->textureBytes);
	while(!current->texture->GXtexture)
	{
		TextureCache_FreeNextTexture();
		current->texture->GXtexture = (u16*) __lwp_heap_allocate(GXtexCache,current->texture->textureBytes);
	}
#endif //__GX__
	cache.cachedBytes += current->texture->textureBytes;

#ifndef __GX__
	glBindTexture( GL_TEXTURE_2D, current->texture->glName );
	glCopyTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, 0, OGL.height - current->texture->height + OGL.heightOffset, current->texture->realWidth, current->texture->realHeight, 0 );
#else // !__GX__
	//Note: texture realWidth and realHeight should be multiple of 2!
	GX_SetTexCopySrc((u16) OGL.GXorigX, (u16) OGL.GXorigY,(u16) current->texture->realWidth,(u16) current->texture->realHeight);
	GX_SetTexCopyDst((u16) current->texture->realWidth,(u16) current->texture->realHeight, current->texture->GXtexfmt, GX_FALSE);
	if (current->texture->GXtexture) GX_CopyTex(current->texture->GXtexture, GX_FALSE);
	GX_PixModeSync();
#endif // __GX__

	*(u32*)&RDRAM[current->startAddress] = current->startAddress;

	current->changed = TRUE;

	gSP.changed |= CHANGED_TEXTURE;
}

void FrameBuffer_RenderBuffer( u32 address )
{
	FrameBuffer *current = frameBuffer.top;

	while (current != NULL)
	{
		if ((current->startAddress <= address) &&
			(current->endAddress >= address))
		{
#ifndef __GX__
			glPushAttrib( GL_ENABLE_BIT | GL_VIEWPORT_BIT );
#endif // __GX__

			Combiner_BeginTextureUpdate();
			TextureCache_ActivateTexture( 0, current->texture );
			Combiner_SetCombine( EncodeCombineMode( 0, 0, 0, TEXEL0, 0, 0, 0, 1, 0, 0, 0, TEXEL0, 0, 0, 0, 1 ) );
/*			if (OGL.ARB_multitexture)
			{
				for (int i = 0; i < OGL.maxTextureUnits; i++)
				{
					glActiveTextureARB( GL_TEXTURE0_ARB + i );
					glDisable( GL_TEXTURE_2D );
				}

				glActiveTextureARB( GL_TEXTURE0_ARB );
			}

			TextureCache_ActivateTexture( 0, current->texture );
			glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );
			glEnable( GL_TEXTURE_2D );*/

#ifndef __GX__
			glDisable( GL_BLEND );
			glDisable( GL_ALPHA_TEST );
			glDisable( GL_DEPTH_TEST );
			glDisable( GL_CULL_FACE );
			glDisable( GL_POLYGON_OFFSET_FILL );
//			glDisable( GL_REGISTER_COMBINERS_NV );
			glDisable( GL_FOG );

			glMatrixMode( GL_PROJECTION );
			glLoadIdentity();
 			glOrtho( 0, OGL.width, 0, OGL.height, -1.0f, 1.0f );
			glViewport( 0, OGL.heightOffset, OGL.width, OGL.height );
			glDisable( GL_SCISSOR_TEST );

			float u1, v1;

			u1 = (float)current->texture->width / (float)current->texture->realWidth;
			v1 = (float)current->texture->height / (float)current->texture->realHeight;

			glDrawBuffer( GL_FRONT );
			glBegin(GL_QUADS);
 				glTexCoord2f( 0.0f, 0.0f );
				glVertex2f( 0.0f, OGL.height - current->texture->height );

				glTexCoord2f( 0.0f, v1 );
				glVertex2f( 0.0f, OGL.height );

 				glTexCoord2f( u1,  v1 );
				glVertex2f( current->texture->width, OGL.height );

 				glTexCoord2f( u1, 0.0f );
				glVertex2f( current->texture->width, OGL.height - current->texture->height );
			glEnd();
			glDrawBuffer( GL_BACK );

/*			glEnable( GL_TEXTURE_2D );
			glActiveTextureARB( GL_TEXTURE0_ARB );
			glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB );*/
			glPopAttrib();
#else // !__GX__

			GX_SetBlendMode(GX_BM_NONE, GX_BL_ONE, GX_BL_ZERO, GX_LO_CLEAR); 
			GX_SetAlphaCompare(GX_ALWAYS,0,GX_AOP_AND,GX_ALWAYS,0);
			GX_SetZMode(GX_DISABLE,GX_ALWAYS,GX_FALSE);
			GX_SetCullMode (GX_CULL_NONE);
			GX_SetFog(GX_FOG_NONE,0.1,1.0,0.0,1.0,(GXColor) {0,0,0,255});

			Mtx44 GXprojection;
			guMtxIdentity(GXprojection);
			guOrtho(GXprojection, 0, OGL.height, 0, OGL.width, 0.0f, 1.0f);
			GX_LoadProjectionMtx(GXprojection, GX_ORTHOGRAPHIC); 
			GX_LoadPosMtxImm(OGL.GXmodelViewIdent,GX_PNMTX0);
			GX_SetViewport((f32) 0,(f32) 0,(f32) OGL.width,(f32) OGL.height, 0.0f, 1.0f);
			GX_SetScissor((u32) 0,(u32) 0,(u32) OGL.width,(u32) OGL.height);	//Set to the same size as the viewport.

			float u1, v1;

			u1 = (float)current->texture->width / (float)current->texture->realWidth;
			v1 = (float)current->texture->height / (float)current->texture->realHeight;

//			glDrawBuffer( GL_FRONT );

			//set vertex description here
			GX_ClearVtxDesc();
			GX_SetVtxDesc(GX_VA_PTNMTXIDX, GX_PNMTX0);
			GX_SetVtxDesc(GX_VA_TEX0MTXIDX, GX_TEXMTX0);
			GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
			GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
			//set vertex attribute formats here
			GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_F32, 0);
			GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
			GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
				GX_Position2f32( (f32) OGL.GXorigX, (f32) OGL.GXorigY );
				GX_TexCoord2f32( 0.0f, 0.0f );
				GX_Position2f32( (f32) OGL.GXorigX + current->texture->width, (f32) OGL.GXorigY );
				GX_TexCoord2f32( u1, 0.0f );
				GX_Position2f32( (f32) OGL.GXorigX + current->texture->width, (f32) OGL.GXorigY + current->texture->height );
				GX_TexCoord2f32( u1, v1 );
				GX_Position2f32( (f32) OGL.GXorigX, (f32) OGL.GXorigY + current->texture->height );
				GX_TexCoord2f32( 0.0f, v1 );
			GX_End();

//			glDrawBuffer( GL_BACK );

			OGL.GXupdateMtx = true;

#endif // __GX__

			current->changed = FALSE;

			FrameBuffer_MoveToTop( current );
#ifndef __GX__
			gSP.changed |= CHANGED_TEXTURE | CHANGED_VIEWPORT;
			gDP.changed |= CHANGED_COMBINE;
#else //!__GX__
			gSP.changed |= CHANGED_TEXTURE | CHANGED_VIEWPORT | CHANGED_GEOMETRYMODE;
			gDP.changed |= CHANGED_COMBINE | CHANGED_SCISSOR | CHANGED_RENDERMODE;
#endif //__GX__
			return;
		}
		current = current->lower;
	}
}

void FrameBuffer_RestoreBuffer( u32 address, u16 size, u16 width )
{
	FrameBuffer *current = frameBuffer.top;

	while (current != NULL)
	{
		if ((current->startAddress == address) &&
			(current->width == width) &&
			(current->size == size))
		{
#ifndef __GX__
			glPushAttrib( GL_ENABLE_BIT | GL_VIEWPORT_BIT );
#endif // __GX__

/*			if (OGL.ARB_multitexture)
			{
				for (int i = 0; i < OGL.maxTextureUnits; i++)
				{
					glActiveTextureARB( GL_TEXTURE0_ARB + i );
					glDisable( GL_TEXTURE_2D );
				}

				glActiveTextureARB( GL_TEXTURE0_ARB );
			}

			TextureCache_ActivateTexture( 0, current->texture );
			glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );
			glEnable( GL_TEXTURE_2D );*/
			Combiner_BeginTextureUpdate();
			TextureCache_ActivateTexture( 0, current->texture );
			Combiner_SetCombine( EncodeCombineMode( 0, 0, 0, TEXEL0, 0, 0, 0, 1, 0, 0, 0, TEXEL0, 0, 0, 0, 1 ) );
			
#ifndef __GX__
			glDisable( GL_BLEND );
			glDisable( GL_ALPHA_TEST );
			glDisable( GL_DEPTH_TEST );
			glDisable( GL_SCISSOR_TEST );
			glDisable( GL_CULL_FACE );
			glDisable( GL_POLYGON_OFFSET_FILL );
			//glDisable( GL_REGISTER_COMBINERS_NV );
			glDisable( GL_FOG );
//			glDepthMask( FALSE );

			glMatrixMode( GL_PROJECTION );
			glLoadIdentity();
 			glOrtho( 0, OGL.width, 0, OGL.height, -1.0f, 1.0f );
//			glOrtho( 0, RDP.width, RDP.height, 0, -1.0f, 1.0f );
			glViewport( 0, OGL.heightOffset, OGL.width, OGL.height );

			float u1, v1;

			u1 = (float)current->texture->width / (float)current->texture->realWidth;
			v1 = (float)current->texture->height / (float)current->texture->realHeight;

			glBegin(GL_QUADS); 
 				glTexCoord2f( 0.0f, 0.0f );
				glVertex2f( 0.0f, OGL.height - current->texture->height );

				glTexCoord2f( 0.0f, v1 );
				glVertex2f( 0.0f, OGL.height );

 				glTexCoord2f( u1,  v1 );
				glVertex2f( current->texture->width, OGL.height );

 				glTexCoord2f( u1, 0.0f );
				glVertex2f( current->texture->width, OGL.height - current->texture->height );
			glEnd();

			glLoadIdentity();
			glPopAttrib();
#else //!__GX__

			GX_SetBlendMode(GX_BM_NONE, GX_BL_ONE, GX_BL_ZERO, GX_LO_CLEAR); 
			GX_SetAlphaCompare(GX_ALWAYS,0,GX_AOP_AND,GX_ALWAYS,0);
			GX_SetZMode(GX_DISABLE,GX_ALWAYS,GX_FALSE);
			GX_SetCullMode (GX_CULL_NONE);
			GX_SetFog(GX_FOG_NONE,0.1,1.0,0.0,1.0,(GXColor) {0,0,0,255});

			Mtx44 GXprojection;
			guMtxIdentity(GXprojection);
			guOrtho(GXprojection, 0, OGL.height, 0, OGL.width, 0.0f, 1.0f);
			GX_LoadProjectionMtx(GXprojection, GX_ORTHOGRAPHIC); 
			GX_LoadPosMtxImm(OGL.GXmodelViewIdent,GX_PNMTX0);
			GX_SetViewport((f32) 0,(f32) 0,(f32) OGL.width,(f32) OGL.height, 0.0f, 1.0f);
			GX_SetScissor((u32) 0,(u32) 0,(u32) OGL.width,(u32) OGL.height);	//Set to the same size as the viewport.

			float u1, v1;

			u1 = (float)current->texture->width / (float)current->texture->realWidth;
			v1 = (float)current->texture->height / (float)current->texture->realHeight;

			//set vertex description here
			GX_ClearVtxDesc();
			GX_SetVtxDesc(GX_VA_PTNMTXIDX, GX_PNMTX0);
			GX_SetVtxDesc(GX_VA_TEX0MTXIDX, GX_TEXMTX0);
			GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
			GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
			//set vertex attribute formats here
			GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_F32, 0);
			GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
			GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
				GX_Position2f32( (f32) OGL.GXorigX, (f32) OGL.GXorigY );
				GX_TexCoord2f32( 0.0f, 0.0f );
				GX_Position2f32( (f32) OGL.GXorigX + current->texture->width, (f32) OGL.GXorigY );
				GX_TexCoord2f32( u1, 0.0f );
				GX_Position2f32( (f32) OGL.GXorigX + current->texture->width, (f32) OGL.GXorigY + current->texture->height );
				GX_TexCoord2f32( u1, v1 );
				GX_Position2f32( (f32) OGL.GXorigX, (f32) OGL.GXorigY + current->texture->height );
				GX_TexCoord2f32( 0.0f, v1 );
			GX_End();

			OGL.GXupdateMtx = true;

#endif // __GX__

			FrameBuffer_MoveToTop( current );

#ifndef __GX__
			gSP.changed |= CHANGED_TEXTURE | CHANGED_VIEWPORT;
			gDP.changed |= CHANGED_COMBINE;
#else //!__GX__
			gSP.changed |= CHANGED_TEXTURE | CHANGED_VIEWPORT | CHANGED_GEOMETRYMODE;
			gDP.changed |= CHANGED_COMBINE | CHANGED_SCISSOR | CHANGED_RENDERMODE;
#endif //__GX__
			return;
		}
		current = current->lower;
	}
#ifndef __GX__
	//No framebuffer was found
	float color[4] = {0.0f,0.0f,0.0f,0.0f};
	OGL_ClearColorBuffer( color );
#endif //!__GX__
}

FrameBuffer *FrameBuffer_FindBuffer( u32 address )
{
	FrameBuffer *current = frameBuffer.top;

	while (current)
	{
		if ((current->startAddress <= address) &&
			(current->endAddress >= address))
			return current;
		current = current->lower;
	}

	return NULL;
}

void FrameBuffer_ActivateBufferTexture( s16 t, FrameBuffer *buffer )
{
#ifndef __GX__
    buffer->texture->scaleS = OGL.scaleX / (float)buffer->texture->realWidth;
    buffer->texture->scaleT = OGL.scaleY / (float)buffer->texture->realHeight;
#else //!__GX__
    buffer->texture->scaleS = OGL.GXscaleX / (float)buffer->texture->realWidth;
    buffer->texture->scaleT = OGL.GXscaleY / (float)buffer->texture->realHeight;
#endif //__GX__

	if (gSP.textureTile[t]->shifts > 10)
		buffer->texture->shiftScaleS = (float)(1 << (16 - gSP.textureTile[t]->shifts));
	else if (gSP.textureTile[t]->shifts > 0)
		buffer->texture->shiftScaleS = 1.0f / (float)(1 << gSP.textureTile[t]->shifts);
	else
		buffer->texture->shiftScaleS = 1.0f;

	if (gSP.textureTile[t]->shiftt > 10)
		buffer->texture->shiftScaleT = (float)(1 << (16 - gSP.textureTile[t]->shiftt));
	else if (gSP.textureTile[t]->shiftt > 0)
		buffer->texture->shiftScaleT = 1.0f / (float)(1 << gSP.textureTile[t]->shiftt);
	else
		buffer->texture->shiftScaleT = 1.0f;

	if (gDP.loadType == LOADTYPE_TILE)
	{
		buffer->texture->offsetS = gDP.loadTile->uls;
#ifndef __GX__
		buffer->texture->offsetT = (float)buffer->height - (gDP.loadTile->ult + (gDP.textureImage.address - buffer->startAddress) / (buffer->width << buffer->size >> 1));
#else //!__GX__
		buffer->texture->offsetT = (float)(gDP.loadTile->ult + (gDP.textureImage.address - buffer->startAddress) / (buffer->width << buffer->size >> 1));
#endif //__GX__
	}
	else
	{
		buffer->texture->offsetS = 0.0f;
#ifndef __GX__
		buffer->texture->offsetT = (float)buffer->height - (gDP.textureImage.address - buffer->startAddress) / (buffer->width << buffer->size >> 1);
#else //!__GX__
		buffer->texture->offsetT = (float)(gDP.textureImage.address - buffer->startAddress) / (buffer->width << buffer->size >> 1);
#endif //__GX__
	}

	FrameBuffer_MoveToTop( buffer );
	TextureCache_ActivateTexture( t, buffer->texture );
}

#ifdef __GX__
void FrameBuffer_IncrementVIcount()
{
	FrameBuffer *buffer = frameBuffer.top;
	while(buffer)
	{
		buffer->texture->VIcount++;
		buffer = buffer->lower;
	}
}
#endif //__GX__
