/**
 * glN64_GX - OpenGL.cpp
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
#include "../gui/DEBUG.h"
#include <math.h>
#include "3DMath.h"
#include <malloc.h>
#endif // __GX__

#ifndef __LINUX__
# include <windows.h>
# include <GL/gl.h>
# include "glext.h"
#else // !__LINUX__
# include "../main/winlnxdefs.h"
#define GL_GLEXT_PROTOTYPES
#define __WIN32__
#include "gl.h" 
#include "glext.h"
#undef __WIN32__
//# include <GL/gl.h>
//# include <GL/glext.h>
#ifndef __GX__
# include "SDL.h"
#endif // !__GX__
# include <string.h>
# include <time.h>
# include <stdlib.h>

# ifndef min
#  define min(a,b) ((a) < (b) ? (a) : (b))
# endif
# ifndef max
#  define max(a,b) ((a) > (b) ? (a) : (b))
# endif
# define timeGetTime() time(NULL)

#endif // __LINUX__
#include <math.h>
#include <stdio.h>
#include "glN64.h"
#include "OpenGL.h"
#include "Types.h"
#include "N64.h"
#include "gSP.h"
#include "gDP.h"
#include "Textures.h"
#include "Combiner.h"
#include "VI.h"

#ifndef GL_BGR
#define GL_BGR GL_BGR_EXT
#endif

#ifdef DEBUGON
extern "C" { void _break(); }
#endif

#ifdef __GX__
extern char glN64_useFrameBufferTextures;
extern char glN64_use2xSaiTextures;
#endif // __GX__

GLInfo OGL;

#ifndef __LINUX__
// NV_register_combiners functions
PFNGLCOMBINERPARAMETERFVNVPROC glCombinerParameterfvNV;
PFNGLCOMBINERPARAMETERFNVPROC glCombinerParameterfNV;
PFNGLCOMBINERPARAMETERIVNVPROC glCombinerParameterivNV;
PFNGLCOMBINERPARAMETERINVPROC glCombinerParameteriNV;
PFNGLCOMBINERINPUTNVPROC glCombinerInputNV;
PFNGLCOMBINEROUTPUTNVPROC glCombinerOutputNV;
PFNGLFINALCOMBINERINPUTNVPROC glFinalCombinerInputNV;
PFNGLGETCOMBINERINPUTPARAMETERFVNVPROC glGetCombinerInputParameterfvNV;
PFNGLGETCOMBINERINPUTPARAMETERIVNVPROC glGetCombinerInputParameterivNV;
PFNGLGETCOMBINEROUTPUTPARAMETERFVNVPROC glGetCombinerOutputParameterfvNV;
PFNGLGETCOMBINEROUTPUTPARAMETERIVNVPROC glGetCombinerOutputParameterivNV;
PFNGLGETFINALCOMBINERINPUTPARAMETERFVNVPROC glGetFinalCombinerInputParameterfvNV;
PFNGLGETFINALCOMBINERINPUTPARAMETERIVNVPROC glGetFinalCombinerInputParameterivNV;

// ARB_multitexture functions
PFNGLACTIVETEXTUREARBPROC glActiveTextureARB;
PFNGLCLIENTACTIVETEXTUREARBPROC glClientActiveTextureARB;
PFNGLMULTITEXCOORD2FARBPROC glMultiTexCoord2fARB;

// EXT_fog_coord functions
PFNGLFOGCOORDFEXTPROC glFogCoordfEXT;
PFNGLFOGCOORDFVEXTPROC glFogCoordfvEXT;
PFNGLFOGCOORDDEXTPROC glFogCoorddEXT;
PFNGLFOGCOORDDVEXTPROC glFogCoorddvEXT;
PFNGLFOGCOORDPOINTEREXTPROC glFogCoordPointerEXT;

// EXT_secondary_color functions
PFNGLSECONDARYCOLOR3BEXTPROC glSecondaryColor3bEXT;
PFNGLSECONDARYCOLOR3BVEXTPROC glSecondaryColor3bvEXT;
PFNGLSECONDARYCOLOR3DEXTPROC glSecondaryColor3dEXT;
PFNGLSECONDARYCOLOR3DVEXTPROC glSecondaryColor3dvEXT;
PFNGLSECONDARYCOLOR3FEXTPROC glSecondaryColor3fEXT;
PFNGLSECONDARYCOLOR3FVEXTPROC glSecondaryColor3fvEXT;
PFNGLSECONDARYCOLOR3IEXTPROC glSecondaryColor3iEXT;
PFNGLSECONDARYCOLOR3IVEXTPROC glSecondaryColor3ivEXT;
PFNGLSECONDARYCOLOR3SEXTPROC glSecondaryColor3sEXT;
PFNGLSECONDARYCOLOR3SVEXTPROC glSecondaryColor3svEXT;
PFNGLSECONDARYCOLOR3UBEXTPROC glSecondaryColor3ubEXT;
PFNGLSECONDARYCOLOR3UBVEXTPROC glSecondaryColor3ubvEXT;
PFNGLSECONDARYCOLOR3UIEXTPROC glSecondaryColor3uiEXT;
PFNGLSECONDARYCOLOR3UIVEXTPROC glSecondaryColor3uivEXT;
PFNGLSECONDARYCOLOR3USEXTPROC glSecondaryColor3usEXT;
PFNGLSECONDARYCOLOR3USVEXTPROC glSecondaryColor3usvEXT;
PFNGLSECONDARYCOLORPOINTEREXTPROC glSecondaryColorPointerEXT;
#endif // !__LINUX__

BOOL isExtensionSupported( const char *extension )
{
#ifndef __GX__
	const GLubyte *extensions = NULL;
	const GLubyte *start;
	GLubyte *where, *terminator;

	where = (GLubyte *) strchr(extension, ' ');
	if (where || *extension == '\0')
		return 0;

	extensions = glGetString(GL_EXTENSIONS);

	start = extensions;
	for (;;)
	{
		where = (GLubyte *) strstr((const char *) start, extension);
		if (!where)
			break;

		terminator = where + strlen(extension);
		if (where == start || *(where - 1) == ' ')
			if (*terminator == ' ' || *terminator == '\0')
				return TRUE;

		start = terminator;
	}
#endif // !__GX__
	return FALSE;
}

void OGL_InitExtensions()
{
	if ((OGL.NV_register_combiners = isExtensionSupported( "GL_NV_register_combiners" )))
	{
#ifndef __LINUX__
		glCombinerParameterfvNV = (PFNGLCOMBINERPARAMETERFVNVPROC)wglGetProcAddress( "glCombinerParameterfvNV" );
		glCombinerParameterfNV = (PFNGLCOMBINERPARAMETERFNVPROC)wglGetProcAddress( "glCombinerParameterfNV" );
		glCombinerParameterivNV = (PFNGLCOMBINERPARAMETERIVNVPROC)wglGetProcAddress( "glCombinerParameterivNV" );
		glCombinerParameteriNV = (PFNGLCOMBINERPARAMETERINVPROC)wglGetProcAddress( "glCombinerParameteriNV" );
		glCombinerInputNV = (PFNGLCOMBINERINPUTNVPROC)wglGetProcAddress( "glCombinerInputNV" );
		glCombinerOutputNV = (PFNGLCOMBINEROUTPUTNVPROC)wglGetProcAddress( "glCombinerOutputNV" );
		glFinalCombinerInputNV = (PFNGLFINALCOMBINERINPUTNVPROC)wglGetProcAddress( "glFinalCombinerInputNV" );
		glGetCombinerInputParameterfvNV = (PFNGLGETCOMBINERINPUTPARAMETERFVNVPROC)wglGetProcAddress( "glGetCombinerInputParameterfvNV" );
		glGetCombinerInputParameterivNV = (PFNGLGETCOMBINERINPUTPARAMETERIVNVPROC)wglGetProcAddress( "glGetCombinerInputParameterivNV" );
		glGetCombinerOutputParameterfvNV = (PFNGLGETCOMBINEROUTPUTPARAMETERFVNVPROC)wglGetProcAddress( "glGetCombinerOutputParameterfvNV" );
		glGetCombinerOutputParameterivNV = (PFNGLGETCOMBINEROUTPUTPARAMETERIVNVPROC)wglGetProcAddress( "glGetCombinerOutputParameterivNV" );
		glGetFinalCombinerInputParameterfvNV = (PFNGLGETFINALCOMBINERINPUTPARAMETERFVNVPROC)wglGetProcAddress( "glGetFinalCombinerInputParameterfvNV" );
		glGetFinalCombinerInputParameterivNV = (PFNGLGETFINALCOMBINERINPUTPARAMETERIVNVPROC)wglGetProcAddress( "glGetFinalCombinerInputParameterivNV" );
#endif // !__LINUX__
#ifndef __GX__ 
		glGetIntegerv( GL_MAX_GENERAL_COMBINERS_NV, &OGL.maxGeneralCombiners );
#endif // !__GX__
	}

	if ((OGL.ARB_multitexture = isExtensionSupported( "GL_ARB_multitexture" )))
	{
#ifndef __LINUX__
		glActiveTextureARB			= (PFNGLACTIVETEXTUREARBPROC)wglGetProcAddress( "glActiveTextureARB" );
		glClientActiveTextureARB	= (PFNGLCLIENTACTIVETEXTUREARBPROC)wglGetProcAddress( "glClientActiveTextureARB" );
		glMultiTexCoord2fARB		= (PFNGLMULTITEXCOORD2FARBPROC)wglGetProcAddress( "glMultiTexCoord2fARB" );
#endif // !__LINUX__
#ifndef __GX__
		glGetIntegerv( GL_MAX_TEXTURE_UNITS_ARB, &OGL.maxTextureUnits );
		OGL.maxTextureUnits = min( 8, OGL.maxTextureUnits ); // The plugin only supports 8, and 4 is really enough
#endif // !__GX__
	}
#ifdef __GX__
	OGL.maxTextureUnits = 8;
	OGL.ARB_multitexture = TRUE;
#endif // __GX__


	if ((OGL.EXT_fog_coord = isExtensionSupported( "GL_EXT_fog_coord" )))
	{
#ifndef __LINUX__
		glFogCoordfEXT = (PFNGLFOGCOORDFEXTPROC)wglGetProcAddress( "glFogCoordfEXT" );
		glFogCoordfvEXT = (PFNGLFOGCOORDFVEXTPROC)wglGetProcAddress( "glFogCoordfvEXT" );
		glFogCoorddEXT = (PFNGLFOGCOORDDEXTPROC)wglGetProcAddress( "glFogCoorddEXT" );
		glFogCoorddvEXT = (PFNGLFOGCOORDDVEXTPROC)wglGetProcAddress( "glFogCoorddvEXT" );
		glFogCoordPointerEXT = (PFNGLFOGCOORDPOINTEREXTPROC)wglGetProcAddress( "glFogCoordPointerEXT" );
#endif // !__LINUX__
	}

	if ((OGL.EXT_secondary_color = isExtensionSupported( "GL_EXT_secondary_color" )))
	{
#ifndef __LINUX__
		glSecondaryColor3bEXT = (PFNGLSECONDARYCOLOR3BEXTPROC)wglGetProcAddress( "glSecondaryColor3bEXT" );
		glSecondaryColor3bvEXT = (PFNGLSECONDARYCOLOR3BVEXTPROC)wglGetProcAddress( "glSecondaryColor3bvEXT" );
		glSecondaryColor3dEXT = (PFNGLSECONDARYCOLOR3DEXTPROC)wglGetProcAddress( "glSecondaryColor3dEXT" );
		glSecondaryColor3dvEXT = (PFNGLSECONDARYCOLOR3DVEXTPROC)wglGetProcAddress( "glSecondaryColor3dvEXT" );
		glSecondaryColor3fEXT = (PFNGLSECONDARYCOLOR3FEXTPROC)wglGetProcAddress( "glSecondaryColor3fEXT" );
		glSecondaryColor3fvEXT = (PFNGLSECONDARYCOLOR3FVEXTPROC)wglGetProcAddress( "glSecondaryColor3fvEXT" );
		glSecondaryColor3iEXT = (PFNGLSECONDARYCOLOR3IEXTPROC)wglGetProcAddress( "glSecondaryColor3iEXT" );
		glSecondaryColor3ivEXT = (PFNGLSECONDARYCOLOR3IVEXTPROC)wglGetProcAddress( "glSecondaryColor3ivEXT" );
		glSecondaryColor3sEXT = (PFNGLSECONDARYCOLOR3SEXTPROC)wglGetProcAddress( "glSecondaryColor3sEXT" );
		glSecondaryColor3svEXT = (PFNGLSECONDARYCOLOR3SVEXTPROC)wglGetProcAddress( "glSecondaryColor3svEXT" );
		glSecondaryColor3ubEXT = (PFNGLSECONDARYCOLOR3UBEXTPROC)wglGetProcAddress( "glSecondaryColor3ubEXT" );
		glSecondaryColor3ubvEXT = (PFNGLSECONDARYCOLOR3UBVEXTPROC)wglGetProcAddress( "glSecondaryColor3ubvEXT" );
		glSecondaryColor3uiEXT = (PFNGLSECONDARYCOLOR3UIEXTPROC)wglGetProcAddress( "glSecondaryColor3uiEXT" );
		glSecondaryColor3uivEXT = (PFNGLSECONDARYCOLOR3UIVEXTPROC)wglGetProcAddress( "glSecondaryColor3uivEXT" );
		glSecondaryColor3usEXT = (PFNGLSECONDARYCOLOR3USEXTPROC)wglGetProcAddress( "glSecondaryColor3usEXT" );
		glSecondaryColor3usvEXT = (PFNGLSECONDARYCOLOR3USVEXTPROC)wglGetProcAddress( "glSecondaryColor3usvEXT" );
		glSecondaryColorPointerEXT = (PFNGLSECONDARYCOLORPOINTEREXTPROC)wglGetProcAddress( "glSecondaryColorPointerEXT" );
#endif // !__LINUX__
	}

	OGL.ARB_texture_env_combine = isExtensionSupported( "GL_ARB_texture_env_combine" );
	OGL.ARB_texture_env_crossbar = isExtensionSupported( "GL_ARB_texture_env_crossbar" );
	OGL.EXT_texture_env_combine = isExtensionSupported( "GL_EXT_texture_env_combine" );
	OGL.ATI_texture_env_combine3 = isExtensionSupported( "GL_ATI_texture_env_combine3" );
	OGL.ATIX_texture_env_route = isExtensionSupported( "GL_ATIX_texture_env_route" );
	OGL.NV_texture_env_combine4 = isExtensionSupported( "GL_NV_texture_env_combine4" );;
}

void OGL_InitStates()
{
#ifndef __GX__
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();

	glVertexPointer( 4, GL_FLOAT, sizeof( GLVertex ), &OGL.vertices[0].x );
	glEnableClientState( GL_VERTEX_ARRAY );

	glColorPointer( 4, GL_FLOAT, sizeof( GLVertex ), &OGL.vertices[0].color.r );
	glEnableClientState( GL_COLOR_ARRAY );

	if (OGL.EXT_secondary_color)
	{
		glSecondaryColorPointerEXT( 3, GL_FLOAT, sizeof( GLVertex ), &OGL.vertices[0].secondaryColor.r );
		glEnableClientState( GL_SECONDARY_COLOR_ARRAY_EXT );
	}

	if (OGL.ARB_multitexture)
	{
		glClientActiveTextureARB( GL_TEXTURE0_ARB );
		glTexCoordPointer( 2, GL_FLOAT, sizeof( GLVertex ), &OGL.vertices[0].s0 );
		glEnableClientState( GL_TEXTURE_COORD_ARRAY );

		glClientActiveTextureARB( GL_TEXTURE1_ARB );
		glTexCoordPointer( 2, GL_FLOAT, sizeof( GLVertex ), &OGL.vertices[0].s1 );
		glEnableClientState( GL_TEXTURE_COORD_ARRAY );
	}
	else
	{
		glTexCoordPointer( 2, GL_FLOAT, sizeof( GLVertex ), &OGL.vertices[0].s0 );
		glEnableClientState( GL_TEXTURE_COORD_ARRAY );
	}

	if (OGL.EXT_fog_coord)
	{
		glFogi( GL_FOG_COORDINATE_SOURCE_EXT, GL_FOG_COORDINATE_EXT );

		glFogi( GL_FOG_MODE, GL_LINEAR );
		glFogf( GL_FOG_START, 0.0f );
		glFogf( GL_FOG_END, 255.0f );

		glFogCoordPointerEXT( GL_FLOAT, sizeof( GLVertex ), &OGL.vertices[0].fog );
		glEnableClientState( GL_FOG_COORDINATE_ARRAY_EXT );
	}

	glPolygonOffset( -3.0f, -3.0f );

	glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
	glClear( GL_COLOR_BUFFER_BIT );
#else // !__GX__
	// TODO: Init GX variables here...
#endif // __GX__

	srand( timeGetTime() );

	for (int i = 0; i < 32; i++)
	{
		for (int j = 0; j < 8; j++)
			for (int k = 0; k < 128; k++)
				OGL.stipplePattern[i][j][k] =((i > (rand() >> 10)) << 7) |
											((i > (rand() >> 10)) << 6) |
											((i > (rand() >> 10)) << 5) |
											((i > (rand() >> 10)) << 4) |
											((i > (rand() >> 10)) << 3) |
											((i > (rand() >> 10)) << 2) |
											((i > (rand() >> 10)) << 1) |
											((i > (rand() >> 10)) << 0);
	}

#ifndef __LINUX__
	SwapBuffers( wglGetCurrentDC() );
#else
	OGL_SwapBuffers();
#endif
}

void OGL_UpdateScale()
{
	OGL.scaleX = OGL.width / (float)VI.width;
	OGL.scaleY = OGL.height / (float)VI.height;
#ifdef __GX__
	OGL.GXscaleX = (float)OGL.GXwidth / (float)VI.width;
	OGL.GXscaleY = (float)OGL.GXheight / (float)VI.height;
#endif //__GX__
}

void OGL_ResizeWindow()
{
#ifndef __LINUX__
	RECT	windowRect, statusRect, toolRect;

	if (OGL.fullscreen)
	{
		OGL.width = OGL.fullscreenWidth;
		OGL.height = OGL.fullscreenHeight;
		OGL.heightOffset = 0;

		SetWindowPos( hWnd, NULL, 0, 0,	OGL.width, OGL.height, SWP_NOACTIVATE | SWP_NOZORDER | SWP_SHOWWINDOW );
	}
	else
	{
		OGL.width = OGL.windowedWidth;
		OGL.height = OGL.windowedHeight;

		GetClientRect( hWnd, &windowRect );
		GetWindowRect( hStatusBar, &statusRect );

		if (hToolBar)
			GetWindowRect( hToolBar, &toolRect );
		else
			toolRect.bottom = toolRect.top = 0;

		OGL.heightOffset = (statusRect.bottom - statusRect.top);
		windowRect.right = windowRect.left + OGL.windowedWidth - 1;
		windowRect.bottom = windowRect.top + OGL.windowedHeight - 1 + OGL.heightOffset;

		AdjustWindowRect( &windowRect, GetWindowLong( hWnd, GWL_STYLE ), GetMenu( hWnd ) != NULL );

		SetWindowPos( hWnd, NULL, 0, 0,	windowRect.right - windowRect.left + 1,
						windowRect.bottom - windowRect.top + 1 + toolRect.bottom - toolRect.top + 1, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE );
	}
#else // !__LINUX__
#endif // __LINUX__
	// This is mainly initializing OGL.heightOffset because I don't think it's inited otherwise.
	OGL.fullscreen = true;
	OGL.fullscreenWidth = 640;
	OGL.fullscreenHeight = 480;
	OGL.windowedWidth = 640;
	OGL.windowedHeight = 480;
	OGL.heightOffset = 0;
#ifdef __GX__

#endif // __GX__
}

bool OGL_Start()
{
#ifndef __GX__
#ifndef __LINUX__
	int		pixelFormat;

	PIXELFORMATDESCRIPTOR pfd = {
		sizeof(PIXELFORMATDESCRIPTOR),    // size of this pfd
		1,                                // version number
		PFD_DRAW_TO_WINDOW |              // support window
		PFD_SUPPORT_OPENGL |              // support OpenGL
		PFD_DOUBLEBUFFER,                 // double buffered
		PFD_TYPE_RGBA,                    // RGBA type
		32,								  // color depth
		0, 0, 0, 0, 0, 0,                 // color bits ignored
		0,                                // no alpha buffer
		0,                                // shift bit ignored
		0,                                // no accumulation buffer
		0, 0, 0, 0,                       // accum bits ignored
		32,								  // z-buffer
		0,                                // no stencil buffer
		0,                                // no auxiliary buffer
		PFD_MAIN_PLANE,                   // main layer
		0,                                // reserved
		0, 0, 0                           // layer masks ignored
	};

	if ((OGL.hDC = GetDC( hWnd )) == NULL)
	{
		MessageBox( hWnd, "Error while getting a device context!", pluginName, MB_ICONERROR | MB_OK );
		return FALSE;
	}

	if ((pixelFormat = ChoosePixelFormat( OGL.hDC, &pfd )) == 0)
	{
		MessageBox( hWnd, "Unable to find a suitable pixel format!", pluginName, MB_ICONERROR | MB_OK );
		OGL_Stop();
		return FALSE;
	}

	if ((SetPixelFormat( OGL.hDC, pixelFormat, &pfd )) == FALSE)
	{
		MessageBox( hWnd, "Error while setting pixel format!", pluginName, MB_ICONERROR | MB_OK );
		OGL_Stop();
		return FALSE;
	}

	if ((OGL.hRC = wglCreateContext( OGL.hDC )) == NULL)
	{
		MessageBox( hWnd, "Error while creating OpenGL context!", pluginName, MB_ICONERROR | MB_OK );
		OGL_Stop();
		return FALSE;
	}

	if ((wglMakeCurrent( OGL.hDC, OGL.hRC )) == FALSE)
	{
		MessageBox( hWnd, "Error while making OpenGL context current!", pluginName, MB_ICONERROR | MB_OK );
		OGL_Stop();
		return FALSE;
	}
#else // !__LINUX__
	// init sdl & gl
	const SDL_VideoInfo *videoInfo;
	Uint32 videoFlags = 0;

	if (OGL.fullscreen)
	{
		OGL.width = OGL.fullscreenWidth;
		OGL.height = OGL.fullscreenHeight;
	}
	else
	{
		OGL.width = OGL.windowedWidth;
		OGL.height = OGL.windowedHeight;
	}


	/* Initialize SDL */
	printf( "[glN64]: (II) Initializing SDL video subsystem...\n" );
	if (SDL_InitSubSystem( SDL_INIT_VIDEO ) == -1)
	{
		printf( "[glN64]: (EE) Error initializing SDL video subsystem: %s\n", SDL_GetError() );
		return FALSE;
	}

	/* Video Info */
	printf( "[glN64]: (II) Getting video info...\n" );
	if (!(videoInfo = SDL_GetVideoInfo()))
	{
		printf( "[glN64]: (EE) Video query failed: %s\n", SDL_GetError() );
		SDL_QuitSubSystem( SDL_INIT_VIDEO );
		return FALSE;
	}

	/* Set the video mode */
	videoFlags |= SDL_OPENGL | SDL_GL_DOUBLEBUFFER | SDL_HWPALETTE;

	if (videoInfo->hw_available)
		videoFlags |= SDL_HWSURFACE;
	else
		videoFlags |= SDL_SWSURFACE;

	if (videoInfo->blit_hw)
		videoFlags |= SDL_HWACCEL;

	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
/*	SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 5 );
	SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 5 );
	SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 5 );*/
	SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 16 );	// 32 bit z-buffer

	printf( "[glN64]: (II) Setting video mode %dx%d...\n", (int)OGL.width, (int)OGL.height );
	if (!(OGL.hScreen = SDL_SetVideoMode( OGL.width, OGL.height, 0, videoFlags )))
	{
		printf( "[glN64]: (EE) Error setting videomode %dx%d: %s\n", (int)OGL.width, (int)OGL.height, SDL_GetError() );
		SDL_QuitSubSystem( SDL_INIT_VIDEO );
		return FALSE;
	}

	SDL_WM_SetCaption( pluginName, pluginName );
#endif // __LINUX__
#else // !__GX__
	//Set 'window height' to efb dimensions
	OGL.width = 640;
	OGL.height = 480;

	//Reset gDP and combiner colors between ROMs. This fixes fillColor when changing ROMs.
	combiner.vertex.color = ZERO;
	combiner.vertex.secondaryColor = ZERO;
	combiner.vertex.alpha = ZERO;
	gDP.primColor.m = 0;
	gDP.primColor.r = 0.0f;
	gDP.primColor.g = 0.0f;
	gDP.primColor.b = 0.0f;
	gDP.primColor.a = 0.0f;
	gDP.primColor.l = 0.0f;
	gDP.envColor.r = 0.0f;
	gDP.envColor.g = 0.0f;
	gDP.envColor.b = 0.0f;
	gDP.envColor.a = 0.0f;
	gDP.fogColor.r = 0.0f;
	gDP.fogColor.g = 0.0f;
	gDP.fogColor.b = 0.0f;
	gDP.fogColor.a = 0.0f;
	gDP.blendColor.r = 0.0f;
	gDP.blendColor.g = 0.0f;
	gDP.blendColor.b = 0.0f;
	gDP.blendColor.a = 0.0f;
	gDP.fillColor.r = 0.0f;
	gDP.fillColor.g = 0.0f;
	gDP.fillColor.b = 0.0f;
	gDP.fillColor.a = 0.0f;
	gDP.fillColor.z = 0.0f;
	gDP.fillColor.dz = 0.0f;
	gDP.primDepth.z = 0.0f;
	gDP.primDepth.deltaZ = 0.0f;
#endif // __GX__
	OGL_InitExtensions();
	OGL_InitStates();

	TextureCache_Init();
	FrameBuffer_Init();
	Combiner_Init();

	gSP.changed = gDP.changed = 0xFFFFFFFF;
	OGL_UpdateScale();

	return TRUE;
}

void OGL_Stop()
{
	Combiner_Destroy();
	FrameBuffer_Destroy();
	TextureCache_Destroy();

#ifndef __GX__
#ifndef __LINUX__
	wglMakeCurrent( NULL, NULL );

	if (OGL.hRC)
	{
		wglDeleteContext( OGL.hRC );
		OGL.hRC = NULL;
	}

	if (OGL.hDC)
	{
		ReleaseDC( hWnd, OGL.hDC );
		OGL.hDC = NULL;
	}
#else // !__LINUX__
	SDL_QuitSubSystem( SDL_INIT_VIDEO );
	OGL.hScreen = NULL;
#endif // __LINUX__
#endif // !__GX__
}

void OGL_UpdateCullFace()
{
#ifndef __GX__
	if (gSP.geometryMode & G_CULL_BOTH)
	{
		glEnable( GL_CULL_FACE );

		if (gSP.geometryMode & G_CULL_BACK)
			glCullFace( GL_BACK );
		else
			glCullFace( GL_FRONT );
	}
	else
		glDisable( GL_CULL_FACE );
#else // !__GX__
	if (gSP.geometryMode & G_CULL_BOTH)
	{
		if (gSP.geometryMode & G_CULL_BACK)
			GX_SetCullMode (GX_CULL_FRONT);	// GC vertex winding is backwards.
		else
			GX_SetCullMode (GX_CULL_BACK);	// GC vertex winding is backwards.
	}
	else
		GX_SetCullMode (GX_CULL_NONE);
#endif // __GX__
}

void OGL_UpdateViewport()
{
#ifndef __GX__
	glViewport( (int)(gSP.viewport.x * OGL.scaleX), (int)((VI.height - (gSP.viewport.y + gSP.viewport.height)) * OGL.scaleY + OGL.heightOffset),
	            (int)(gSP.viewport.width * OGL.scaleX), (int)(gSP.viewport.height * OGL.scaleY) );
	glDepthRange( 0.0f, 1.0f );//gSP.viewport.nearz, gSP.viewport.farz );
#else // !__GX__
	GX_SetViewport((f32) (OGL.GXorigX + gSP.viewport.x * OGL.GXscaleX),(f32) (OGL.GXorigY + gSP.viewport.y * OGL.GXscaleY),
		(f32) (gSP.viewport.width * OGL.GXscaleX),(f32) (gSP.viewport.height * OGL.GXscaleY), 0.0f, 1.0f);
#endif // __GX__
}

void OGL_UpdateDepthUpdate()
{
#ifndef __GX__
	if (gDP.otherMode.depthUpdate)
		glDepthMask( TRUE );
	else
		glDepthMask( FALSE );
#else // !__GX__
	//This should now be taken care of in OGL_UpdateStates()
#endif // __GX__
}

void OGL_UpdateStates()
{
#ifdef __GX__
	if (OGL.GXclearColorBuffer || OGL.GXclearDepthBuffer)
		OGL_GXclearEFB();
#endif //__GX__

	if (gSP.changed & CHANGED_GEOMETRYMODE)
	{
		OGL_UpdateCullFace();
#ifndef __GX__
		if ((gSP.geometryMode & G_FOG) && OGL.EXT_fog_coord && OGL.fog)
			glEnable( GL_FOG );
		else
			glDisable( GL_FOG );
#else // !__GX__
		if ((gSP.geometryMode & G_FOG) && OGL.fog)
			OGL.GXfogType = GX_FOG_ORTHO_LIN;
		else
			OGL.GXfogType = GX_FOG_NONE;
		OGL.GXupdateFog = true;
#endif // __GX__

		gSP.changed &= ~CHANGED_GEOMETRYMODE;
	}

#ifndef __GX__
	if (gSP.geometryMode & G_ZBUFFER)
		glEnable( GL_DEPTH_TEST );
	else
		glDisable( GL_DEPTH_TEST );

	if (gDP.changed & CHANGED_RENDERMODE)
	{
		if (gDP.otherMode.depthCompare)
			glDepthFunc( GL_LEQUAL );
		else
			glDepthFunc( GL_ALWAYS );

		OGL_UpdateDepthUpdate();

		if (gDP.otherMode.depthMode == ZMODE_DEC)
			glEnable( GL_POLYGON_OFFSET_FILL );
		else
		{
//			glPolygonOffset( -3.0f, -3.0f );
			glDisable( GL_POLYGON_OFFSET_FILL );
		}
	}
#else // !__GX__
	//Zbuffer settings
	static u8 GXenableZmode, GXZfunc = GX_ALWAYS, GXZupdate = GX_FALSE;
	if (gSP.geometryMode & G_ZBUFFER)
//		glEnable( GL_DEPTH_TEST );
		GXenableZmode = GX_ENABLE;
	else
//		glDisable( GL_DEPTH_TEST );
		GXenableZmode = GX_DISABLE;

	if (gDP.changed & CHANGED_RENDERMODE)
	{
		if (gDP.otherMode.depthCompare)
//			glDepthFunc( GL_LEQUAL );
			GXZfunc = GX_LEQUAL;
		else
//			glDepthFunc( GL_ALWAYS );
			GXZfunc = GX_ALWAYS;

//		OGL_UpdateDepthUpdate();
		if (gDP.otherMode.depthUpdate)
//			glDepthMask( TRUE );
			GXZupdate = GX_TRUE;
		else
//			glDepthMask( FALSE );
			GXZupdate = GX_FALSE;

		if ((gDP.otherMode.depthMode == ZMODE_DEC) && !OGL.GXpolyOffset)
		{
			OGL.GXpolyOffset = true;
			OGL.GXupdateMtx = true;
		}
		else if (!(gDP.otherMode.depthMode == ZMODE_DEC) && OGL.GXpolyOffset)
		{
			OGL.GXpolyOffset = false;
			OGL.GXupdateMtx = true;
		}
	}
	GX_SetZMode(GXenableZmode,GXZfunc,GXZupdate);
#endif // __GX__

#ifndef __GX__
	if ((gDP.changed & CHANGED_ALPHACOMPARE) || (gDP.changed & CHANGED_RENDERMODE))
	{
		// Enable alpha test for threshold mode
		if ((gDP.otherMode.alphaCompare == G_AC_THRESHOLD) && !(gDP.otherMode.alphaCvgSel))
		{
			glEnable( GL_ALPHA_TEST );

			glAlphaFunc( (gDP.blendColor.a > 0.0f) ? GL_GEQUAL : GL_GREATER, gDP.blendColor.a );
		}
		// Used in TEX_EDGE and similar render modes
		else if (gDP.otherMode.cvgXAlpha)
		{
			glEnable( GL_ALPHA_TEST );

			// Arbitrary number -- gives nice results though
			glAlphaFunc( GL_GEQUAL, 0.5f );
		}
		else
			glDisable( GL_ALPHA_TEST );

		if (OGL.usePolygonStipple && (gDP.otherMode.alphaCompare == G_AC_DITHER) && !(gDP.otherMode.alphaCvgSel))
			glEnable( GL_POLYGON_STIPPLE );
		else
			glDisable( GL_POLYGON_STIPPLE );
	}
#else // !__GX__
	//GX alpha compare update

	if ((gDP.changed & CHANGED_ALPHACOMPARE) || (gDP.changed & CHANGED_RENDERMODE))
	{
		u8 GXalphaFunc, GXalphaRef;
//		u8 GXZcompLoc = GX_FALSE; //do Z compare after texturing (GX_FALSE)
		OGL.GXuseAlphaCompare = true;
		// Enable alpha test for threshold mode
		if ((gDP.otherMode.alphaCompare == G_AC_THRESHOLD) && !(gDP.otherMode.alphaCvgSel))
		{
//			glEnable( GL_ALPHA_TEST );
//			glAlphaFunc( (gDP.blendColor.a > 0.0f) ? GL_GEQUAL : GL_GREATER, gDP.blendColor.a );
			GXalphaFunc = (gDP.blendColor.a > 0.0f) ? GX_GEQUAL : GX_GREATER;
			GXalphaRef = (u8) (gDP.blendColor.a*255);
		}
		// Used in TEX_EDGE and similar render modes
		else if (gDP.otherMode.cvgXAlpha)
		{
//			glEnable( GL_ALPHA_TEST );
			// Arbitrary number -- gives nice results though
//			glAlphaFunc( GL_GEQUAL, 0.5f );
			GXalphaFunc = GX_GEQUAL;
			GXalphaRef = (u8) 128;
		}
		else
		{
//			glDisable( GL_ALPHA_TEST );
			GXalphaFunc = GX_ALWAYS;
			GXalphaRef = (u8) 0;
//			GXZcompLoc = GX_TRUE; //do Z compare before texturing (GX_TRUE)
			OGL.GXuseAlphaCompare = false;
		}
		//TODO: Add more code to implement the following line.
//		GX_SetZCompLoc(GXZcompLoc);
		GX_SetAlphaCompare(GXalphaFunc,GXalphaRef,GX_AOP_AND,GX_ALWAYS,0);

#ifdef GLN64_SDLOG
		sprintf(txtbuffer,"SetAlphaCompare: GXalphaFunc %d, GXalphaRef %d, GXZcompLoc %d\n", GXalphaFunc, GXalphaRef, GXZcompLoc);
		DEBUG_print(txtbuffer,DBG_SDGECKOPRINT);
#endif // GLN64_SDLOG

//TODO: Implement PolygonStipple?
/*		if (OGL.usePolygonStipple && (gDP.otherMode.alphaCompare == G_AC_DITHER) && !(gDP.otherMode.alphaCvgSel))
			glEnable( GL_POLYGON_STIPPLE );
		else
			glDisable( GL_POLYGON_STIPPLE );*/

		if (gDP.otherMode.depthSource == G_ZS_PRIM)
			GX_SetZTexture(GX_ZT_REPLACE,GX_TF_Z16,0);
		else
			GX_SetZTexture(GX_ZT_DISABLE,GX_TF_Z16,0);
	}
#endif // __GX__

#ifndef __GX__
	if (gDP.changed & CHANGED_SCISSOR)
	{
		glScissor( (int)(gDP.scissor.ulx * OGL.scaleX), (int)((VI.height - gDP.scissor.lry) * OGL.scaleY + OGL.heightOffset),
			(int)((gDP.scissor.lrx - gDP.scissor.ulx) * OGL.scaleX), (int)((gDP.scissor.lry - gDP.scissor.uly) * OGL.scaleY) );
	}
#else // !__GX__
	if ((gDP.changed & CHANGED_SCISSOR) || (gSP.changed & CHANGED_VIEWPORT))
	{
		float ulx = max(OGL.GXorigX + max(gDP.scissor.ulx,gSP.viewport.x) * OGL.GXscaleX, 0);
		float uly = max(OGL.GXorigY + max(gDP.scissor.uly,gSP.viewport.y) * OGL.GXscaleY, 0);
		float lrx = max(OGL.GXorigX + min(min(gDP.scissor.lrx,gSP.viewport.x + gSP.viewport.width) * OGL.GXscaleX,OGL.GXwidth), 0);
		float lry = max(OGL.GXorigY + min(min(gDP.scissor.lry,gSP.viewport.y + gSP.viewport.height) * OGL.GXscaleY,OGL.GXheight), 0);
		GX_SetScissor((u32) ulx,(u32) uly,(u32) (lrx - ulx),(u32) (lry - uly));
	}
#endif // __GX__

	if (gSP.changed & CHANGED_VIEWPORT)
	{
		OGL_UpdateViewport();
	}

	if ((gDP.changed & CHANGED_COMBINE) || (gDP.changed & CHANGED_CYCLETYPE))
	{
		if (gDP.otherMode.cycleType == G_CYC_COPY)
			Combiner_SetCombine( EncodeCombineMode( 0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0 ) );
		else if (gDP.otherMode.cycleType == G_CYC_FILL)
			Combiner_SetCombine( EncodeCombineMode( 0, 0, 0, SHADE, 0, 0, 0, 1, 0, 0, 0, SHADE, 0, 0, 0, 1 ) );
		else
			Combiner_SetCombine( gDP.combine.mux );
	}

	if (gDP.changed & CHANGED_COMBINE_COLORS)
	{
		Combiner_UpdateCombineColors();
	}

	if ((gSP.changed & CHANGED_TEXTURE) || (gDP.changed & CHANGED_TILE) || (gDP.changed & CHANGED_TMEM))
	{
		Combiner_BeginTextureUpdate();
		if (combiner.usesT0)
		{
			TextureCache_Update( 0 );

			gSP.changed &= ~CHANGED_TEXTURE;
			gDP.changed &= ~CHANGED_TILE;
			gDP.changed &= ~CHANGED_TMEM;
		}
		else
		{
			TextureCache_ActivateDummy( 0 );
		}

		if (combiner.usesT1)
		{
			TextureCache_Update( 1 );

			gSP.changed &= ~CHANGED_TEXTURE;
			gDP.changed &= ~CHANGED_TILE;
			gDP.changed &= ~CHANGED_TMEM;
		}
		else
		{
			TextureCache_ActivateDummy( 1 );
		}
		Combiner_EndTextureUpdate();
	}

#ifndef __GX__
	if ((gDP.changed & CHANGED_FOGCOLOR) && OGL.fog)
		glFogfv( GL_FOG_COLOR, &gDP.fogColor.r );
#else // !__GX__
	if ((gDP.changed & CHANGED_FOGCOLOR) && OGL.fog) 
	{
		OGL.GXfogColor.r = (u8) (gDP.fogColor.r*255);
		OGL.GXfogColor.g = (u8) (gDP.fogColor.g*255);
		OGL.GXfogColor.b = (u8) (gDP.fogColor.b*255);
		OGL.GXfogColor.a = (u8) (gDP.fogColor.a*255);
		OGL.GXupdateFog = true;
	}

	if(OGL.GXupdateFog)
	{
		GX_SetFog(OGL.GXfogType,OGL.GXfogStartZ,OGL.GXfogEndZ,-1.0,1.0,OGL.GXfogColor);
		OGL.GXupdateFog = false;
		if(OGL.GXfogType == GX_FOG_ORTHO_LIN)
		{
#ifdef SHOW_DEBUG
			sprintf(txtbuffer,"SetFog: StartZ %f, EndZ %f, color (%d,%d,%d,%d), fo %f, fm %f", OGL.GXfogStartZ, OGL.GXfogEndZ, OGL.GXfogColor.r, OGL.GXfogColor.g, OGL.GXfogColor.b, OGL.GXfogColor.a, (float)gSP.fog.offset, (float)gSP.fog.multiplier);
			DEBUG_print(txtbuffer,DBG_RSPINFO1);
#endif
		}
	}
#endif // __GX__

#ifndef __GX__
	if ((gDP.changed & CHANGED_RENDERMODE) || (gDP.changed & CHANGED_CYCLETYPE))
	{
		if ((gDP.otherMode.forceBlender) &&
			(gDP.otherMode.cycleType != G_CYC_COPY) &&
			(gDP.otherMode.cycleType != G_CYC_FILL) &&
			!(gDP.otherMode.alphaCvgSel))
		{
 			glEnable( GL_BLEND );

			switch (gDP.otherMode.l >> 16)
			{
				case 0x0448: // Add
				case 0x055A:
					glBlendFunc( GL_ONE, GL_ONE );
					break;
				case 0x0C08: // 1080 Sky
				case 0x0F0A: // Used LOTS of places
					glBlendFunc( GL_ONE, GL_ZERO );
					break;
				case 0xC810: // Blends fog
				case 0xC811: // Blends fog
				case 0x0C18: // Standard interpolated blend
				case 0x0C19: // Used for antialiasing
				case 0x0050: // Standard interpolated blend
				case 0x0055: // Used for antialiasing
					glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
					break;
				case 0x0FA5: // Seems to be doing just blend color - maybe combiner can be used for this?
				case 0x5055: // Used in Paper Mario intro, I'm not sure if this is right...
					glBlendFunc( GL_ZERO, GL_ONE );
					break;
				default:
					glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
					break;
			}
		}
		else
			glDisable( GL_BLEND );

		if (gDP.otherMode.cycleType == G_CYC_FILL)
		{
			glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
			glEnable( GL_BLEND );
		}
	}
#else // !__GX__
	u8 GXblenddstfactor, GXblendsrcfactor, GXblendmode;

	if ((gDP.changed & CHANGED_RENDERMODE) || (gDP.changed & CHANGED_CYCLETYPE))
	{
		if ((gDP.otherMode.forceBlender) &&
			(gDP.otherMode.cycleType != G_CYC_COPY) &&
			(gDP.otherMode.cycleType != G_CYC_FILL) &&
			!(gDP.otherMode.alphaCvgSel))
		{
// 			glEnable( GL_BLEND );
			GXblendmode = GX_BM_BLEND;

			switch (gDP.otherMode.l >> 16)
			{
				case 0x0448: // Add
				case 0x055A:
//					glBlendFunc( GL_ONE, GL_ONE );
					GXblendsrcfactor = GX_BL_ONE;
					GXblenddstfactor = GX_BL_ONE;
					break;
				case 0x0C08: // 1080 Sky
				case 0x0F0A: // Used LOTS of places
//					glBlendFunc( GL_ONE, GL_ZERO );
					GXblendsrcfactor = GX_BL_ONE;
					GXblenddstfactor = GX_BL_ZERO;
					break;
				case 0xC810: // Blends fog
				case 0xC811: // Blends fog
				case 0x0C18: // Standard interpolated blend
				case 0x0C19: // Used for antialiasing
				case 0x0050: // Standard interpolated blend
				case 0x0055: // Used for antialiasing
//					glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
					GXblendsrcfactor = GX_BL_SRCALPHA;
					GXblenddstfactor = GX_BL_INVSRCALPHA;
					break;
				case 0x0FA5: // Seems to be doing just blend color - maybe combiner can be used for this?
				case 0x5055: // Used in Paper Mario intro, I'm not sure if this is right...
//					glBlendFunc( GL_ZERO, GL_ONE );
					GXblendsrcfactor = GX_BL_ZERO;
					GXblenddstfactor = GX_BL_ONE;
					break;
				default:
//					glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
					GXblendsrcfactor = GX_BL_SRCALPHA;
					GXblenddstfactor = GX_BL_INVSRCALPHA;
					break;
			}
		}
		else {
//			glDisable( GL_BLEND );
			GXblendmode = GX_BM_NONE;
			GXblendsrcfactor = GX_BL_ONE;
			GXblenddstfactor = GX_BL_ZERO;
		}

		if (gDP.otherMode.cycleType == G_CYC_FILL)
		{
//			glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
//			glEnable( GL_BLEND );
			GXblendmode = GX_BM_BLEND;
			GXblendsrcfactor = GX_BL_SRCALPHA;
			GXblenddstfactor = GX_BL_INVSRCALPHA;
		}
		GX_SetBlendMode(GXblendmode, GXblendsrcfactor, GXblenddstfactor, GX_LO_CLEAR); 
		GX_SetColorUpdate(GX_ENABLE);
		GX_SetAlphaUpdate(GX_ENABLE);
		GX_SetDstAlpha(GX_DISABLE, 0xFF);

#ifdef GLN64_SDLOG
		sprintf(txtbuffer,"SetBlendMode: GXblendmode %d, SrcFactor %d, DestFactor %d\n", GXblendmode, GXblendsrcfactor, GXblenddstfactor);
		DEBUG_print(txtbuffer,DBG_SDGECKOPRINT);
#endif // GLN64_SDLOG
	}
#endif // __GX__

	gDP.changed &= CHANGED_TILE | CHANGED_TMEM;
	gSP.changed &= CHANGED_TEXTURE | CHANGED_MATRIX;
}

void OGL_AddTriangle( SPVertex *vertices, int v0, int v1, int v2 )
{
	int v[] = { v0, v1, v2 };

	if (gSP.changed || gDP.changed)
		OGL_UpdateStates();

//	Playing around with lod fraction junk...
//	float ds = max( max( fabs( vertices[v0].s - vertices[v1].s ), fabs( vertices[v0].s - vertices[v2].s ) ), fabs( vertices[v1].s - vertices[v2].s ) ) * cache.current[0]->shiftScaleS * gSP.texture.scales;
//	float dx = max( max( fabs( vertices[v0].x / vertices[v0].w - vertices[v1].x / vertices[v1].w ), fabs( vertices[v0].x / vertices[v0].w - vertices[v2].x / vertices[v2].w ) ), fabs( vertices[v1].x / vertices[v1].w - vertices[v2].x / vertices[v2].w ) ) * gSP.viewport.vscale[0];
//	float lod = ds / dx;
//	float lod_fraction = min( 1.0f, max( 0.0f, lod - 1.0f ) / max( 1.0f, gSP.texture.level ) );


	for (int i = 0; i < 3; i++)
	{
		OGL.vertices[OGL.numVertices].x = vertices[v[i]].x;
		OGL.vertices[OGL.numVertices].y = vertices[v[i]].y;
#ifndef __GX__
		OGL.vertices[OGL.numVertices].z = gDP.otherMode.depthSource == G_ZS_PRIM ? gDP.primDepth.z * vertices[v[i]].w : vertices[v[i]].z;
#else // !__GX__
		//TODO: primDepthZ should now be handled with a Ztex. Verify this.
		//Note: Could also handle primDepthZ by manipulating the Projection matrix
//		OGL.vertices[OGL.numVertices].z = (gDP.otherMode.depthSource == G_ZS_PRIM) && !(OGL.GXuseProj) ? gDP.primDepth.z * vertices[v[i]].w : vertices[v[i]].z;
		OGL.vertices[OGL.numVertices].z = vertices[v[i]].z;
		OGL.vertices[OGL.numVertices].zPrime = vertices[v[i]].zPrime;
#endif // __GX__
		OGL.vertices[OGL.numVertices].w = vertices[v[i]].w;

		OGL.vertices[OGL.numVertices].color.r = vertices[v[i]].r;
		OGL.vertices[OGL.numVertices].color.g = vertices[v[i]].g;
		OGL.vertices[OGL.numVertices].color.b = vertices[v[i]].b;
		OGL.vertices[OGL.numVertices].color.a = vertices[v[i]].a;
		SetConstant( OGL.vertices[OGL.numVertices].color, combiner.vertex.color, combiner.vertex.alpha );
		//SetConstant( OGL.vertices[OGL.numVertices].secondaryColor, combiner.vertex.secondaryColor, ONE );

		if (OGL.EXT_secondary_color)
		{
			OGL.vertices[OGL.numVertices].secondaryColor.r = 0.0f;//lod_fraction; //vertices[v[i]].r;
			OGL.vertices[OGL.numVertices].secondaryColor.g = 0.0f;//lod_fraction; //vertices[v[i]].g;
			OGL.vertices[OGL.numVertices].secondaryColor.b = 0.0f;//lod_fraction; //vertices[v[i]].b;
			OGL.vertices[OGL.numVertices].secondaryColor.a = 1.0f;
			SetConstant( OGL.vertices[OGL.numVertices].secondaryColor, combiner.vertex.secondaryColor, ONE );
		}

#ifndef __GX__
		if ((gSP.geometryMode & G_FOG) && OGL.EXT_fog_coord && OGL.fog)
		{
			if (vertices[v[i]].z < -vertices[v[i]].w)
				OGL.vertices[OGL.numVertices].fog = max( 0.0f, -(float)gSP.fog.multiplier + (float)gSP.fog.offset );
			else
				OGL.vertices[OGL.numVertices].fog = max( 0.0f, vertices[v[i]].z / vertices[v[i]].w * (float)gSP.fog.multiplier + (float)gSP.fog.offset );
		}
#else //!__GX__
		//Fog is taken care of in hardware with GX.
#endif //__GX__

		if (combiner.usesT0)
		{
			if (cache.current[0]->frameBufferTexture)
			{
/*				OGL.vertices[OGL.numVertices].s0 = (cache.current[0]->offsetS + (vertices[v[i]].s * cache.current[0]->shiftScaleS * gSP.texture.scales - gSP.textureTile[0]->fuls)) * cache.current[0]->scaleS;
				OGL.vertices[OGL.numVertices].t0 = (cache.current[0]->offsetT - (vertices[v[i]].t * cache.current[0]->shiftScaleT * gSP.texture.scalet - gSP.textureTile[0]->fult)) * cache.current[0]->scaleT;*/

				if (gSP.textureTile[0]->masks)
					OGL.vertices[OGL.numVertices].s0 = (cache.current[0]->offsetS + (vertices[v[i]].s * cache.current[0]->shiftScaleS * gSP.texture.scales - fmod( gSP.textureTile[0]->fuls, 1 << gSP.textureTile[0]->masks ))) * cache.current[0]->scaleS;
				else
					OGL.vertices[OGL.numVertices].s0 = (cache.current[0]->offsetS + (vertices[v[i]].s * cache.current[0]->shiftScaleS * gSP.texture.scales - gSP.textureTile[0]->fuls)) * cache.current[0]->scaleS;

#ifndef __GX__
				if (gSP.textureTile[0]->maskt)
					OGL.vertices[OGL.numVertices].t0 = (cache.current[0]->offsetT - (vertices[v[i]].t * cache.current[0]->shiftScaleT * gSP.texture.scalet - fmod( gSP.textureTile[0]->fult, 1 << gSP.textureTile[0]->maskt ))) * cache.current[0]->scaleT;
				else
					OGL.vertices[OGL.numVertices].t0 = (cache.current[0]->offsetT - (vertices[v[i]].t * cache.current[0]->shiftScaleT * gSP.texture.scalet - gSP.textureTile[0]->fult)) * cache.current[0]->scaleT;
#else //!__GX__
				if (gSP.textureTile[0]->maskt)
					OGL.vertices[OGL.numVertices].t0 = (cache.current[0]->offsetT + (vertices[v[i]].t * cache.current[0]->shiftScaleT * gSP.texture.scalet - fmod( gSP.textureTile[0]->fult, 1 << gSP.textureTile[0]->maskt ))) * cache.current[0]->scaleT;
				else
					OGL.vertices[OGL.numVertices].t0 = (cache.current[0]->offsetT + (vertices[v[i]].t * cache.current[0]->shiftScaleT * gSP.texture.scalet - gSP.textureTile[0]->fult)) * cache.current[0]->scaleT;
#endif //__GX__
			}
			else
			{
				OGL.vertices[OGL.numVertices].s0 = (vertices[v[i]].s * cache.current[0]->shiftScaleS * gSP.texture.scales - gSP.textureTile[0]->fuls + cache.current[0]->offsetS) * cache.current[0]->scaleS; 
				OGL.vertices[OGL.numVertices].t0 = (vertices[v[i]].t * cache.current[0]->shiftScaleT * gSP.texture.scalet - gSP.textureTile[0]->fult + cache.current[0]->offsetT) * cache.current[0]->scaleT;
			}
		}

		if (combiner.usesT1)
		{
			if (cache.current[0]->frameBufferTexture)
			{
				OGL.vertices[OGL.numVertices].s1 = (cache.current[1]->offsetS + (vertices[v[i]].s * cache.current[1]->shiftScaleS * gSP.texture.scales - gSP.textureTile[1]->fuls)) * cache.current[1]->scaleS;
#ifndef __GX__
				OGL.vertices[OGL.numVertices].t1 = (cache.current[1]->offsetT - (vertices[v[i]].t * cache.current[1]->shiftScaleT * gSP.texture.scalet - gSP.textureTile[1]->fult)) * cache.current[1]->scaleT;
#else //!__GX__
				OGL.vertices[OGL.numVertices].t1 = (cache.current[1]->offsetT + (vertices[v[i]].t * cache.current[1]->shiftScaleT * gSP.texture.scalet - gSP.textureTile[1]->fult)) * cache.current[1]->scaleT;
#endif //__GX__
			}
			else
			{
				OGL.vertices[OGL.numVertices].s1 = (vertices[v[i]].s * cache.current[1]->shiftScaleS * gSP.texture.scales - gSP.textureTile[1]->fuls + cache.current[1]->offsetS) * cache.current[1]->scaleS; 
				OGL.vertices[OGL.numVertices].t1 = (vertices[v[i]].t * cache.current[1]->shiftScaleT * gSP.texture.scalet - gSP.textureTile[1]->fult + cache.current[1]->offsetT) * cache.current[1]->scaleT;
			}
		}
		OGL.numVertices++;
	}
	OGL.numTriangles++;

	if (OGL.numVertices >= 255)
		OGL_DrawTriangles();
}

#ifdef SHOW_DEBUG
	int CntTriProj, CntTriProjW, CntTriOther, CntTriNear, CntTriPolyOffset;
#endif

void OGL_DrawTriangles()
{
	if (OGL.usePolygonStipple && (gDP.otherMode.alphaCompare == G_AC_DITHER) && !(gDP.otherMode.alphaCvgSel))
	{
		OGL.lastStipple = (OGL.lastStipple + 1) & 0x7;
#ifndef __GX__
		glPolygonStipple( OGL.stipplePattern[(BYTE)(gDP.envColor.a * 255.0f) >> 3][OGL.lastStipple] );
#else // !__GX__
		//TODO: find equivalent
#endif // __GX__
	}

#ifndef __GX__
	glDrawArrays( GL_TRIANGLES, 0, OGL.numVertices );
#else // !__GX__
	GXColor GXcol;
	float invW;

#ifdef GLN64_SDLOG
	sprintf(txtbuffer,"OGL_DrawTris: numTri %d, numVert %d, useT0 %d, useT1 %d\n", OGL.numTriangles, OGL.numVertices, combiner.usesT0, combiner.usesT1);
	DEBUG_print(txtbuffer,DBG_SDGECKOPRINT);
#endif // GLN64_SDLOG

	//Update MV & P Matrices
	if(OGL.GXupdateMtx)
	{
		if(OGL.GXpolyOffset)
		{
			if(OGL.GXuseCombW)
			{
				if(!OGL.GXuseProjWnear)
				{
					CopyMatrix( OGL.GXprojTemp, OGL.GXcombW );
					OGL.GXprojTemp[2][2] += GXpolyOffsetFactor;
					GX_LoadProjectionMtx(OGL.GXprojTemp, GX_PERSPECTIVE); 
				}
				else
				{
					GX_LoadProjectionMtx(OGL.GXprojWnear, GX_PERSPECTIVE); 
				}
			}
			else
			{
				CopyMatrix( OGL.GXprojTemp, OGL.GXprojIdent );
				OGL.GXprojTemp[2][3] -= GXpolyOffsetFactor;
				GX_LoadProjectionMtx(OGL.GXprojTemp, GX_ORTHOGRAPHIC); 
			}
		}
		else
		{
			//TODO: handle this case when !G_ZBUFFER
/*			if(!(gSP.geometryMode & G_ZBUFFER))
			{
				CopyMatrix( OGL.GXprojTemp, OGL.GXproj );
				OGL.GXprojTemp[2][2] =  0.0;
				OGL.GXprojTemp[2][3] = -1.0;
				if(OGL.GXprojTemp[3][2] == -1)
					GX_LoadProjectionMtx(OGL.GXprojTemp, GX_PERSPECTIVE);
				else
					GX_LoadProjectionMtx(OGL.GXprojTemp, GX_ORTHOGRAPHIC); 
			}*/
			if(OGL.GXuseCombW)
			{
				if(!OGL.GXuseProjWnear)
				{
					GX_LoadProjectionMtx(OGL.GXcombW, GX_PERSPECTIVE); 
				}
				else
				{
					GX_LoadProjectionMtx(OGL.GXprojWnear, GX_PERSPECTIVE); 
				}
			}
			else
				GX_LoadProjectionMtx(OGL.GXprojIdent, GX_ORTHOGRAPHIC); 
		}
		OGL.GXupdateMtx = false;
	}

	if ((gDP.otherMode.depthSource == G_ZS_PRIM)||OGL.GXuseAlphaCompare)
	{
		GX_SetZCompLoc(GX_FALSE);	// Do Z-compare after texturing. (i.e. use Ztex)
		cache.GXZTexPrimCnt++;
	}
	else
	{
		GX_SetZCompLoc(GX_TRUE);	// Do Z-compare before texturing.
		cache.GXnoZTexPrimCnt++;
	}

	//set vertex description here
	GX_ClearVtxDesc();
	GX_SetVtxDesc(GX_VA_PTNMTXIDX, GX_PNMTX0);
//	if (combiner.usesT0) GX_SetVtxDesc(GX_VA_TEX0MTXIDX, GX_TEXMTX0);
//	if (combiner.usesT1) GX_SetVtxDesc(GX_VA_TEX1MTXIDX, GX_TEXMTX0);
	GX_SetVtxDesc(GX_VA_TEX0MTXIDX, GX_TEXMTX0);
	GX_SetVtxDesc(GX_VA_TEX1MTXIDX, GX_TEXMTX0);
	GX_SetVtxDesc(GX_VA_TEX2MTXIDX, GX_TEXMTX0);
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
//	if (lighting) GX_SetVtxDesc(GX_VA_NRM, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
//	if (combiner.usesT0) GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
//	if (combiner.usesT1) GX_SetVtxDesc(GX_VA_TEX1, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_TEX1, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_TEX2, GX_DIRECT);

	//set vertex attribute formats here
	//TODO: These only need to be set once during init.
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
//	if (lighting) GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_NRM, GX_NRM_XYZ, GX_F32, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
//	if (combiner.usesT0) GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
//	if (combiner.usesT1) GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX1, GX_TEX_ST, GX_F32, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX1, GX_TEX_ST, GX_F32, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX2, GX_TEX_ST, GX_F32, 0);


#ifdef SHOW_DEBUG
	if (OGL.GXuseCombW) CntTriProjW += OGL.numTriangles;
	else CntTriOther += OGL.numTriangles;
	if (OGL.GXuseCombW && OGL.GXuseProjWnear) CntTriNear += OGL.numTriangles;
	if (OGL.GXpolyOffset) CntTriPolyOffset += OGL.numTriangles;
#endif

	GX_Begin(GX_TRIANGLES, GX_VTXFMT0, OGL.numVertices);
	for (int i = 0; i < OGL.numVertices; i++) {
		if(OGL.GXuseCombW)
			GX_Position3f32( OGL.vertices[i].x, OGL.vertices[i].y, -OGL.vertices[i].w );
//			GX_Position3f32( OGL.vertices[i].x, OGL.vertices[i].y, OGL.vertices[i].zPrime );
		else
		{
			invW = (OGL.vertices[i].w != 0) ? 1/OGL.vertices[i].w : 0.0f;
			GX_Position3f32( OGL.vertices[i].x*invW, OGL.vertices[i].y*invW, OGL.vertices[i].z*invW );
		}
//		GX_Position3f32(OGL.vertices[i].x/OGL.vertices[i].w, OGL.vertices[i].y/OGL.vertices[i].w, OGL.vertices[i].z/OGL.vertices[i].w);
		GXcol.r = (u8) (min(OGL.vertices[i].color.r,1.0)*255);
		GXcol.g = (u8) (min(OGL.vertices[i].color.g,1.0)*255);
		GXcol.b = (u8) (min(OGL.vertices[i].color.b,1.0)*255);
		GXcol.a = (u8) (min(OGL.vertices[i].color.a,1.0)*255);
		GX_Color4u8(GXcol.r, GXcol.g, GXcol.b, GXcol.a); 
//		if (combiner.usesT0) GX_TexCoord2f32(OGL.vertices[i].s0,OGL.vertices[i].t0);
//		if (combiner.usesT1) GX_TexCoord2f32(OGL.vertices[i].s1,OGL.vertices[i].t1);
		if (combiner.usesT0) GX_TexCoord2f32(OGL.vertices[i].s0,OGL.vertices[i].t0);
		else GX_TexCoord2f32(0.0f,0.0f);
		if (combiner.usesT1) GX_TexCoord2f32(OGL.vertices[i].s1,OGL.vertices[i].t1);
		else GX_TexCoord2f32(0.0f,0.0f);
		GX_TexCoord2f32(0.0f,0.0f);
#ifdef GLN64_SDLOG
		sprintf(txtbuffer," Vert%d: Pos x = %.2f, y = %.2f, z = %.2f, w = %.2f, RGBA = %d, %d, %d, %d, VertCol RGBA = %.2f, %.2f, %.2f, %.2f\n", i, OGL.vertices[i].x/OGL.vertices[i].w, OGL.vertices[i].y/OGL.vertices[i].w, OGL.vertices[i].z/OGL.vertices[i].w, OGL.vertices[i].w, GXcol.r, GXcol.g, GXcol.b, GXcol.a, OGL.vertices[i].color.r, OGL.vertices[i].color.g, OGL.vertices[i].color.b, OGL.vertices[i].color.a);
		DEBUG_print(txtbuffer,DBG_SDGECKOPRINT);
#endif // GLN64_SDLOG
	}
	GX_End();
#endif // __GX__
	OGL.numTriangles = OGL.numVertices = 0;
}

void OGL_DrawLine( SPVertex *vertices, int v0, int v1, float width )
{
	int v[] = { v0, v1 };

	GLcolor color;

	if (gSP.changed || gDP.changed)
		OGL_UpdateStates();

#ifndef __GX__
	glLineWidth( width * OGL.scaleX );

	glBegin( GL_LINES );
		for (int i = 0; i < 2; i++)
		{
			color.r = vertices[v[i]].r;
			color.g = vertices[v[i]].g;
			color.b = vertices[v[i]].b;
			color.a = vertices[v[i]].a;
			SetConstant( color, combiner.vertex.color, combiner.vertex.alpha );
			glColor4fv( &color.r );

			if (OGL.EXT_secondary_color)
			{
				color.r = vertices[v[i]].r;
				color.g = vertices[v[i]].g;
				color.b = vertices[v[i]].b;
				color.a = vertices[v[i]].a;
				SetConstant( color, combiner.vertex.secondaryColor, combiner.vertex.alpha );
				glSecondaryColor3fvEXT( &color.r );
			}

			glVertex4f( vertices[v[i]].x, vertices[v[i]].y, vertices[v[i]].z, vertices[v[i]].w );
		}
	glEnd();
#else // !__GX__		//TODO: Implement secondary color.
//	GX_SetLineWidth( width * OGL.scaleX, GX_TO_ZERO );
	GX_SetLineWidth( width * OGL.GXscaleX * 6, GX_TO_ZERO );

	GXColor GXcol;
	float invW;

	//Update MV & P Matrices
	if(OGL.GXupdateMtx)
	{
		if(OGL.GXpolyOffset)
		{
			if(OGL.GXuseCombW)
			{
				CopyMatrix( OGL.GXprojTemp, OGL.GXcombW );
				OGL.GXprojTemp[2][2] += GXpolyOffsetFactor;
				GX_LoadProjectionMtx(OGL.GXprojTemp, GX_PERSPECTIVE); 
			}
			else
			{
				CopyMatrix( OGL.GXprojTemp, OGL.GXprojIdent );
				OGL.GXprojTemp[2][3] -= GXpolyOffsetFactor;
				GX_LoadProjectionMtx(OGL.GXprojTemp, GX_ORTHOGRAPHIC); 
			}
		}
		else
		{
			if(OGL.GXuseCombW)
				GX_LoadProjectionMtx(OGL.GXcombW, GX_PERSPECTIVE); 
			else
				GX_LoadProjectionMtx(OGL.GXprojIdent, GX_ORTHOGRAPHIC); 
		}
		OGL.GXupdateMtx = false;
	}

	if ((gDP.otherMode.depthSource == G_ZS_PRIM)||OGL.GXuseAlphaCompare)
	{
		GX_SetZCompLoc(GX_FALSE);	// Do Z-compare after texturing. (i.e. use Ztex)
		cache.GXZTexPrimCnt++;
	}
	else
	{
		GX_SetZCompLoc(GX_TRUE);	// Do Z-compare before texturing.
		cache.GXnoZTexPrimCnt++;
	}

	//set vertex description here
	GX_ClearVtxDesc();
	GX_SetVtxDesc(GX_VA_PTNMTXIDX, GX_PNMTX0);
	GX_SetVtxDesc(GX_VA_TEX2MTXIDX, GX_TEXMTX0);
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_TEX2, GX_DIRECT);
	//set vertex attribute formats here
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX2, GX_TEX_ST, GX_F32, 0);
	GX_Begin(GX_LINES, GX_VTXFMT0, 2);
		for (int i = 0; i < 2; i++)
		{
			if(OGL.GXuseCombW)
				GX_Position3f32( OGL.vertices[i].x, OGL.vertices[i].y, -OGL.vertices[i].w );
			else
			{
				invW = (OGL.vertices[i].w != 0) ? 1/OGL.vertices[i].w : 0.0f;
				GX_Position3f32( OGL.vertices[i].x*invW, OGL.vertices[i].y*invW, OGL.vertices[i].z*invW );
			}
			color.r = vertices[v[i]].r;
			color.g = vertices[v[i]].g;
			color.b = vertices[v[i]].b;
			color.a = vertices[v[i]].a;
			SetConstant( color, combiner.vertex.color, combiner.vertex.alpha );
			GXcol.r = (u8) (color.r*255);
			GXcol.g = (u8) (color.g*255);
			GXcol.b = (u8) (color.b*255);
			GXcol.a = (u8) (color.a*255);
			GX_Color4u8(GXcol.r, GXcol.g, GXcol.b, GXcol.a); 
			GX_TexCoord2f32(0.0f,0.0f);
		}
	GX_End();
#endif // __GX__
}

void OGL_DrawRect( int ulx, int uly, int lrx, int lry, float *color )
{
	OGL_UpdateStates();

#ifndef __GX__
	glDisable( GL_SCISSOR_TEST );
	glDisable( GL_CULL_FACE );
	glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
	glOrtho( 0, VI.width, VI.height, 0, 1.0f, -1.0f );
	glViewport( 0, OGL.heightOffset, OGL.width, OGL.height );
	glDepthRange( 0.0f, 1.0f );

	glColor4f( color[0], color[1], color[2], color[3] );

	glBegin( GL_QUADS );
		glVertex4f( ulx, uly, (gDP.otherMode.depthSource == G_ZS_PRIM) ? gDP.primDepth.z : gSP.viewport.nearz, 1.0f );
		glVertex4f( lrx, uly, (gDP.otherMode.depthSource == G_ZS_PRIM) ? gDP.primDepth.z : gSP.viewport.nearz, 1.0f );
		glVertex4f( lrx, lry, (gDP.otherMode.depthSource == G_ZS_PRIM) ? gDP.primDepth.z : gSP.viewport.nearz, 1.0f );
		glVertex4f( ulx, lry, (gDP.otherMode.depthSource == G_ZS_PRIM) ? gDP.primDepth.z : gSP.viewport.nearz, 1.0f );
	glEnd();

	glLoadIdentity();
	OGL_UpdateCullFace();
	OGL_UpdateViewport();
	glEnable( GL_SCISSOR_TEST );
#else // !__GX__
	GX_SetScissor((u32) 0,(u32) 0,(u32) OGL.width+1,(u32) OGL.height+1);	//Disable Scissor
	GX_SetCullMode (GX_CULL_NONE);
	Mtx44 GXprojection;
	guMtxIdentity(GXprojection);
	guOrtho(GXprojection, 0, VI.height, 0, VI.width, 1.0f, -1.0f);
	if(OGL.GXpolyOffset)
		GXprojection[2][3] -= GXpolyOffsetFactor;
	GX_LoadProjectionMtx(GXprojection, GX_ORTHOGRAPHIC); 
	GX_LoadPosMtxImm(OGL.GXmodelViewIdent,GX_PNMTX0);

	GX_SetViewport((f32) OGL.GXorigX,(f32) OGL.GXorigY,(f32) OGL.GXwidth,(f32) OGL.GXheight, 0.0f, 1.0f);

	GXColor GXcol;
	GXcol.r = (u8) (color[0]*255);
	GXcol.g = (u8) (color[1]*255);
	GXcol.b = (u8) (color[2]*255);
	GXcol.a = (u8) (color[3]*255);

	if ((gDP.otherMode.depthSource == G_ZS_PRIM)||OGL.GXuseAlphaCompare)
	{
		GX_SetZCompLoc(GX_FALSE);	// Do Z-compare after texturing. (i.e. use Ztex)
		cache.GXZTexPrimCnt++;
	}
	else
	{
		GX_SetZCompLoc(GX_TRUE);	// Do Z-compare before texturing.
		cache.GXnoZTexPrimCnt++;
	}

	//set vertex description here
	GX_ClearVtxDesc();
	GX_SetVtxDesc(GX_VA_PTNMTXIDX, GX_PNMTX0);
	GX_SetVtxDesc(GX_VA_TEX2MTXIDX, GX_TEXMTX0);
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_TEX2, GX_DIRECT);

	//set vertex attribute formats here
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX2, GX_TEX_ST, GX_F32, 0);
	GX_Begin(GX_QUADS, GX_VTXFMT0, 4);	//TODO: This may fail for G_ZS_PRIM... log and fix, maybe with guOrtho
//		GX_Position3f32( ulx, uly, (gDP.otherMode.depthSource == G_ZS_PRIM) ? gDP.primDepth.z : gSP.viewport.nearz );
		GX_Position3f32( ulx, uly, gSP.viewport.nearz );
		GX_Color4u8(GXcol.r, GXcol.g, GXcol.b, GXcol.a); 
		GX_TexCoord2f32(0.0f,0.0f);
//		GX_Position3f32( lrx, uly, (gDP.otherMode.depthSource == G_ZS_PRIM) ? gDP.primDepth.z : gSP.viewport.nearz );
		GX_Position3f32( lrx, uly, gSP.viewport.nearz );
		GX_Color4u8(GXcol.r, GXcol.g, GXcol.b, GXcol.a); 
		GX_TexCoord2f32(0.0f,0.0f);
//		GX_Position3f32( lrx, lry, (gDP.otherMode.depthSource == G_ZS_PRIM) ? gDP.primDepth.z : gSP.viewport.nearz );
		GX_Position3f32( lrx, lry, gSP.viewport.nearz );
		GX_Color4u8(GXcol.r, GXcol.g, GXcol.b, GXcol.a); 
		GX_TexCoord2f32(0.0f,0.0f);
//		GX_Position3f32( ulx, lry, (gDP.otherMode.depthSource == G_ZS_PRIM) ? gDP.primDepth.z : gSP.viewport.nearz );
		GX_Position3f32( ulx, lry, gSP.viewport.nearz );
		GX_Color4u8(GXcol.r, GXcol.g, GXcol.b, GXcol.a); 
		GX_TexCoord2f32(0.0f,0.0f);
	GX_End();

	OGL.GXupdateMtx = true;

	OGL_UpdateCullFace();
	OGL_UpdateViewport();
	gDP.changed |= CHANGED_SCISSOR;	//Restore scissor in OGL_UpdateStates() before drawing next geometry.
#endif // __GX__
}

void OGL_DrawTexturedRect( float ulx, float uly, float lrx, float lry, float uls, float ult, float lrs, float lrt, bool flip )
{
#ifdef __GX__
	OGL.GXrenderTexRect = true;
#endif //__GX__
	GLVertex rect[2] =
	{	//TODO: This may fail for G_ZS_PRIM... log and fix, maybe with guOrtho
		{ ulx, uly, gDP.otherMode.depthSource == G_ZS_PRIM ? gDP.primDepth.z : gSP.viewport.nearz, 1.0f, { /*gDP.blendColor.r, gDP.blendColor.g, gDP.blendColor.b, gDP.blendColor.a */1.0f, 1.0f, 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, uls, ult, uls, ult, 0.0f },
		{ lrx, lry, gDP.otherMode.depthSource == G_ZS_PRIM ? gDP.primDepth.z : gSP.viewport.nearz, 1.0f, { /*gDP.blendColor.r, gDP.blendColor.g, gDP.blendColor.b, gDP.blendColor.a*/1.0f, 1.0f, 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, lrs, lrt, lrs, lrt, 0.0f },
	};

	OGL_UpdateStates();

#ifndef __GX__
	glDisable( GL_CULL_FACE );
	glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
	glOrtho( 0, VI.width, VI.height, 0, 1.0f, -1.0f );
	glViewport( 0, OGL.heightOffset, OGL.width, OGL.height );
#else // !__GX__
	//Note: Scissoring may need to be reworked here
	float ulx1 = max(OGL.GXorigX + gDP.scissor.ulx * OGL.GXscaleX, 0);
	float uly1 = max(OGL.GXorigY + gDP.scissor.uly * OGL.GXscaleY, 0);
	float lrx1 = max(OGL.GXorigX + min(gDP.scissor.lrx * OGL.GXscaleX,OGL.GXwidth), 0);
	float lry1 = max(OGL.GXorigY + min(gDP.scissor.lry * OGL.GXscaleY,OGL.GXheight), 0);
	GX_SetScissor((u32) ulx1,(u32) uly1,(u32) (lrx1 - ulx1),(u32) (lry1 - uly1));
	GX_SetCullMode (GX_CULL_NONE);
	Mtx44 GXprojection;
	guMtxIdentity(GXprojection);
	guOrtho(GXprojection, 0, VI.height, 0, VI.width, 1.0f, -1.0f);
	if(OGL.GXpolyOffset)
		GXprojection[2][3] -= GXpolyOffsetFactor;
	GX_LoadProjectionMtx(GXprojection, GX_ORTHOGRAPHIC); 

	GX_LoadPosMtxImm(OGL.GXmodelViewIdent,GX_PNMTX0);

	GX_SetViewport((f32) OGL.GXorigX,(f32) OGL.GXorigY,(f32) OGL.GXwidth,(f32) OGL.GXheight, 0.0f, 1.0f);
#endif // __GX__

	if (combiner.usesT0)
	{
		rect[0].s0 = rect[0].s0 * cache.current[0]->shiftScaleS - gSP.textureTile[0]->fuls;
		rect[0].t0 = rect[0].t0 * cache.current[0]->shiftScaleT - gSP.textureTile[0]->fult;
		rect[1].s0 = (rect[1].s0 + 1.0f) * cache.current[0]->shiftScaleS - gSP.textureTile[0]->fuls;
		rect[1].t0 = (rect[1].t0 + 1.0f) * cache.current[0]->shiftScaleT - gSP.textureTile[0]->fult;
		if ((cache.current[0]->maskS) && (fmod( rect[0].s0, cache.current[0]->width ) == 0.0f) && !(cache.current[0]->mirrorS))
		{
			rect[1].s0 -= rect[0].s0;
			rect[0].s0 = 0.0f;
		}

		if ((cache.current[0]->maskT) && (fmod( rect[0].t0, cache.current[0]->height ) == 0.0f) && !(cache.current[0]->mirrorT))
		{
			rect[1].t0 -= rect[0].t0;
			rect[0].t0 = 0.0f;
		}

		if (cache.current[0]->frameBufferTexture)
		{
#ifndef __GX__
			rect[0].s0 = cache.current[0]->offsetS + rect[0].s0;
			rect[0].t0 = cache.current[0]->offsetT - rect[0].t0;
			rect[1].s0 = cache.current[0]->offsetS + rect[1].s0;
			rect[1].t0 = cache.current[0]->offsetT - rect[1].t0;
#else //!__GX__
			rect[0].s0 = cache.current[0]->offsetS + rect[0].s0;
			rect[0].t0 = cache.current[0]->offsetT + rect[0].t0;
			rect[1].s0 = cache.current[0]->offsetS + rect[1].s0;
			rect[1].t0 = cache.current[0]->offsetT + rect[1].t0;
#endif //__GX__
		}

#ifndef __GX__	
		if (OGL.ARB_multitexture)
			glActiveTextureARB( GL_TEXTURE0_ARB );

		if ((rect[0].s0 >= 0.0f) && (rect[1].s0 <= cache.current[0]->width))
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		//glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
		//glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );

		if ((rect[0].t0 >= 0.0f) && (rect[1].t0 <= cache.current[0]->height))
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
#else // !__GX__
		if ((rect[0].s0 >= 0.0f) && (rect[1].s0 <= cache.current[0]->width))
			OGL.GXforceClampS0 = true;
		if ((rect[0].t0 >= 0.0f) && (rect[1].t0 <= cache.current[0]->height))
			OGL.GXforceClampT0 = true;
#endif // __GX__

//		GLint height;

//		glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height );

		rect[0].s0 *= cache.current[0]->scaleS;
		rect[0].t0 *= cache.current[0]->scaleT;
		rect[1].s0 *= cache.current[0]->scaleS;
		rect[1].t0 *= cache.current[0]->scaleT;
	}

	if (combiner.usesT1 && OGL.ARB_multitexture)
	{
		rect[0].s1 = rect[0].s1 * cache.current[1]->shiftScaleS - gSP.textureTile[1]->fuls;
		rect[0].t1 = rect[0].t1 * cache.current[1]->shiftScaleT - gSP.textureTile[1]->fult;
		rect[1].s1 = (rect[1].s1 + 1.0f) * cache.current[1]->shiftScaleS - gSP.textureTile[1]->fuls;
		rect[1].t1 = (rect[1].t1 + 1.0f) * cache.current[1]->shiftScaleT - gSP.textureTile[1]->fult;

		if ((cache.current[1]->maskS) && (fmod( rect[0].s1, cache.current[1]->width ) == 0.0f) && !(cache.current[1]->mirrorS))
		{
			rect[1].s1 -= rect[0].s1;
			rect[0].s1 = 0.0f;
		}

		if ((cache.current[1]->maskT) && (fmod( rect[0].t1, cache.current[1]->height ) == 0.0f) && !(cache.current[1]->mirrorT))
		{
			rect[1].t1 -= rect[0].t1;
			rect[0].t1 = 0.0f;
		}

		if (cache.current[1]->frameBufferTexture)
		{
#ifndef __GX__
			rect[0].s1 = cache.current[1]->offsetS + rect[0].s1;
			rect[0].t1 = cache.current[1]->offsetT - rect[0].t1;
			rect[1].s1 = cache.current[1]->offsetS + rect[1].s1;
			rect[1].t1 = cache.current[1]->offsetT - rect[1].t1;
#else //!__GX__
			rect[0].s1 = cache.current[1]->offsetS + rect[0].s1;
			rect[0].t1 = cache.current[1]->offsetT + rect[0].t1;
			rect[1].s1 = cache.current[1]->offsetS + rect[1].s1;
			rect[1].t1 = cache.current[1]->offsetT + rect[1].t1;
#endif //__GX__
		}

#ifndef __GX__	
		glActiveTextureARB( GL_TEXTURE1_ARB );

		if ((rect[0].s1 == 0.0f) && (rect[1].s1 <= cache.current[1]->width))
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );

		if ((rect[0].t1 == 0.0f) && (rect[1].t1 <= cache.current[1]->height))
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
#else // !__GX__
		if ((rect[0].s1 == 0.0f) && (rect[1].s1 <= cache.current[1]->width))
			OGL.GXforceClampS1 = true;
		if ((rect[0].t1 == 0.0f) && (rect[1].t1 <= cache.current[1]->height))
			OGL.GXforceClampT1 = true;
#endif // __GX__

		rect[0].s1 *= cache.current[1]->scaleS;
		rect[0].t1 *= cache.current[1]->scaleT;
		rect[1].s1 *= cache.current[1]->scaleS;
		rect[1].t1 *= cache.current[1]->scaleT;
	}

#ifndef __GX__	
	if ((gDP.otherMode.cycleType == G_CYC_COPY) && !OGL.forceBilinear)
	{
		if (OGL.ARB_multitexture)
			glActiveTextureARB( GL_TEXTURE0_ARB );

		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	}
#else // !__GX__
	//TODO: Set LOD texture filter modes here.
	if ((gDP.otherMode.cycleType == G_CYC_COPY) && !OGL.forceBilinear)
		OGL.GXuseMinMagNearest = true;

	if (combiner.usesT0 && cache.current[0]->GXtexture != NULL) 
	{
		if (cache.enable2xSaI && !cache.current[0]->frameBufferTexture)
			GX_InitTexObj(&cache.current[0]->GXtex, cache.current[0]->GXtexture, (u16) cache.current[0]->realWidth << 1, 
				(u16) cache.current[0]->realHeight << 1, cache.current[0]->GXtexfmt, 
				(cache.current[0]->clampS || OGL.GXforceClampS0) ? GX_CLAMP : GX_REPEAT, 
				(cache.current[0]->clampT || OGL.GXforceClampT0) ? GX_CLAMP : GX_REPEAT, GX_FALSE); 
		else
			GX_InitTexObj(&cache.current[0]->GXtex, cache.current[0]->GXtexture, (u16) cache.current[0]->realWidth, 
				(u16) cache.current[0]->realHeight, cache.current[0]->GXtexfmt, 
				(cache.current[0]->clampS || OGL.GXforceClampS0) ? GX_CLAMP : GX_REPEAT, 
				(cache.current[0]->clampT || OGL.GXforceClampT0) ? GX_CLAMP : GX_REPEAT, GX_FALSE); 
		if (OGL.GXuseMinMagNearest) GX_InitTexObjLOD(&cache.current[0]->GXtex, GX_NEAR, GX_NEAR, 0.0f, 0.0f, 0.0f, GX_FALSE, GX_FALSE, GX_ANISO_1);
		GX_LoadTexObj(&cache.current[0]->GXtex, GX_TEXMAP0); // t = 0 is GX_TEXMAP0 and t = 1 is GX_TEXMAP1
	}

	if (combiner.usesT1 && OGL.ARB_multitexture && cache.current[1]->GXtexture != NULL)
	{
		if (cache.enable2xSaI && !cache.current[1]->frameBufferTexture)
			GX_InitTexObj(&cache.current[1]->GXtex, cache.current[1]->GXtexture, (u16) cache.current[1]->realWidth << 1, 
				(u16) cache.current[1]->realHeight << 1, cache.current[1]->GXtexfmt, 
				(cache.current[1]->clampS || OGL.GXforceClampS1) ? GX_CLAMP : GX_REPEAT, 
				(cache.current[1]->clampT || OGL.GXforceClampT1) ? GX_CLAMP : GX_REPEAT, GX_FALSE); 
		else
			GX_InitTexObj(&cache.current[1]->GXtex, cache.current[1]->GXtexture, (u16) cache.current[1]->realWidth, 
				(u16) cache.current[1]->realHeight, cache.current[1]->GXtexfmt, 
				(cache.current[1]->clampS || OGL.GXforceClampS1) ? GX_CLAMP : GX_REPEAT, 
				(cache.current[1]->clampT || OGL.GXforceClampT1) ? GX_CLAMP : GX_REPEAT, GX_FALSE); 
		if (OGL.GXuseMinMagNearest) GX_InitTexObjLOD(&cache.current[1]->GXtex, GX_NEAR, GX_NEAR, 0.0f, 0.0f, 0.0f, GX_FALSE, GX_FALSE, GX_ANISO_1);
		GX_LoadTexObj(&cache.current[1]->GXtex, GX_TEXMAP1); // t = 0 is GX_TEXMAP0 and t = 1 is GX_TEXMAP1
	}

	OGL.GXforceClampS0 = false;
	OGL.GXforceClampT0 = false;
	OGL.GXforceClampS1 = false;
	OGL.GXforceClampT1 = false;
	OGL.GXuseMinMagNearest = false;
#endif // __GX__

	SetConstant( rect[0].color, combiner.vertex.color, combiner.vertex.alpha );

	if (OGL.EXT_secondary_color)
		SetConstant( rect[0].secondaryColor, combiner.vertex.secondaryColor, combiner.vertex.alpha );

#ifndef __GX__
	glBegin( GL_QUADS );
		glColor4f( rect[0].color.r, rect[0].color.g, rect[0].color.b, rect[0].color.a );
		if (OGL.EXT_secondary_color)
			glSecondaryColor3fEXT( rect[0].secondaryColor.r, rect[0].secondaryColor.g, rect[0].secondaryColor.b );

		if (OGL.ARB_multitexture)
		{
			glMultiTexCoord2fARB( GL_TEXTURE0_ARB, rect[0].s0, rect[0].t0 );
			glMultiTexCoord2fARB( GL_TEXTURE1_ARB, rect[0].s1, rect[0].t1 );
			glVertex4f( rect[0].x, rect[0].y, rect[0].z, 1.0f );

			glMultiTexCoord2fARB( GL_TEXTURE0_ARB, rect[1].s0, rect[0].t0 );
			glMultiTexCoord2fARB( GL_TEXTURE1_ARB, rect[1].s1, rect[0].t1 );
			glVertex4f( rect[1].x, rect[0].y, rect[0].z, 1.0f );

			glMultiTexCoord2fARB( GL_TEXTURE0_ARB, rect[1].s0, rect[1].t0 );
			glMultiTexCoord2fARB( GL_TEXTURE1_ARB, rect[1].s1, rect[1].t1 );
			glVertex4f( rect[1].x, rect[1].y, rect[0].z, 1.0f );

			glMultiTexCoord2fARB( GL_TEXTURE0_ARB, rect[0].s0, rect[1].t0 );
			glMultiTexCoord2fARB( GL_TEXTURE1_ARB, rect[0].s1, rect[1].t1 );
			glVertex4f( rect[0].x, rect[1].y, rect[0].z, 1.0f );
		}
		else
		{
			glTexCoord2f( rect[0].s0, rect[0].t0 );
			glVertex4f( rect[0].x, rect[0].y, rect[0].z, 1.0f );

			if (flip)
				glTexCoord2f( rect[1].s0, rect[0].t0 );
			else
				glTexCoord2f( rect[0].s0, rect[1].t0 );

			glVertex4f( rect[1].x, rect[0].y, rect[0].z, 1.0f );

			glTexCoord2f( rect[1].s0, rect[1].t0 );
			glVertex4f( rect[1].x, rect[1].y, rect[0].z, 1.0f );

			if (flip)
				glTexCoord2f( rect[1].s0, rect[0].t0 );
			else
				glTexCoord2f( rect[1].s0, rect[0].t0 );
			glVertex4f( rect[0].x, rect[1].y, rect[0].z, 1.0f );
		}
	glEnd();
#else // !__GX__
	GXColor GXcol;

	GXcol.r = (u8) (rect[0].color.r*255);
	GXcol.g = (u8) (rect[0].color.g*255);
	GXcol.b = (u8) (rect[0].color.b*255);
	GXcol.a = (u8) (rect[0].color.a*255);

	if ((gDP.otherMode.depthSource == G_ZS_PRIM)||OGL.GXuseAlphaCompare)
	{
		GX_SetZCompLoc(GX_FALSE);	// Do Z-compare after texturing. (i.e. use Ztex)
		cache.GXZTexPrimCnt++;
	}
	else
	{
		GX_SetZCompLoc(GX_TRUE);	// Do Z-compare before texturing.
		cache.GXnoZTexPrimCnt++;
	}

	//set vertex description here
	GX_ClearVtxDesc();
	GX_SetVtxDesc(GX_VA_PTNMTXIDX, GX_PNMTX0);
	GX_SetVtxDesc(GX_VA_TEX0MTXIDX, GX_TEXMTX0);
	GX_SetVtxDesc(GX_VA_TEX1MTXIDX, GX_TEXMTX0);
	GX_SetVtxDesc(GX_VA_TEX2MTXIDX, GX_TEXMTX0);
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_TEX1, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_TEX2, GX_DIRECT);
	//set vertex attribute formats here
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX1, GX_TEX_ST, GX_F32, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX2, GX_TEX_ST, GX_F32, 0);
	GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
		GX_Position3f32( rect[0].x, rect[0].y, rect[0].z );
		GX_Color4u8( GXcol.r, GXcol.g, GXcol.b, GXcol.a ); 
		GX_TexCoord2f32( rect[0].s0, rect[0].t0 );
		GX_TexCoord2f32( rect[0].s1, rect[0].t1 );
		GX_TexCoord2f32( 0.0, 0.0 );
		GX_Position3f32( rect[1].x, rect[0].y, rect[0].z );
		GX_Color4u8(GXcol.r, GXcol.g, GXcol.b, GXcol.a); 
		if (flip)
		{
			GX_TexCoord2f32( rect[0].s0, rect[1].t0 );
			GX_TexCoord2f32( rect[0].s1, rect[1].t1 );
		}
		else	
		{
			GX_TexCoord2f32( rect[1].s0, rect[0].t0 );
			GX_TexCoord2f32( rect[1].s1, rect[0].t1 );
		}
		GX_TexCoord2f32( 0.0, 0.0 );
		GX_Position3f32( rect[1].x, rect[1].y, rect[0].z );
		GX_Color4u8( GXcol.r, GXcol.g, GXcol.b, GXcol.a ); 
		GX_TexCoord2f32( rect[1].s0, rect[1].t0 );
		GX_TexCoord2f32( rect[1].s1, rect[1].t1 );
		GX_TexCoord2f32( 0.0, 0.0 );
		GX_Position3f32( rect[0].x, rect[1].y, rect[0].z );
		GX_Color4u8(GXcol.r, GXcol.g, GXcol.b, GXcol.a); 
		if (flip)
		{
			GX_TexCoord2f32( rect[1].s0, rect[0].t0 );
			GX_TexCoord2f32( rect[1].s1, rect[0].t1 );
		}
		else
		{
			GX_TexCoord2f32( rect[0].s0, rect[1].t0 );
			GX_TexCoord2f32( rect[0].s1, rect[1].t1 );
		}
		GX_TexCoord2f32( 0.0, 0.0 );
	GX_End();
#endif // __GX__

#ifndef __GX__
	glLoadIdentity();
#else // !__GX__
	OGL.GXrenderTexRect = false;
	OGL.GXupdateMtx = true;
	gDP.changed |= CHANGED_SCISSOR;	//Restore scissor in OGL_UpdateStates() before drawing next geometry.
#endif // __GX__
	OGL_UpdateCullFace();
	OGL_UpdateViewport();
}

void OGL_ClearDepthBuffer()
{
#ifndef __GX__
	glDisable( GL_SCISSOR_TEST );

	OGL_UpdateStates();
	glDepthMask( TRUE );
	glClear( GL_DEPTH_BUFFER_BIT );

	OGL_UpdateDepthUpdate();

	glEnable( GL_SCISSOR_TEST );
#else // !__GX__
	//Note: OGL_UpdateDepthUpdate() should not need to be called b/c DepthMask is set in OGL_UpdateStates()
	OGL.GXclearDepthBuffer = true;
	gDP.changed |= CHANGED_RENDERMODE;
#endif // __GX__
}

void OGL_ClearColorBuffer( float *color )
{
#ifndef __GX__
	glDisable( GL_SCISSOR_TEST );

	glClearColor( color[0], color[1], color[2], color[3] );
	glClear( GL_COLOR_BUFFER_BIT );

	glEnable( GL_SCISSOR_TEST );
#else // !__GX__
	OGL.GXclearColor.r = (u8) (color[0]*255);
	OGL.GXclearColor.g = (u8) (color[1]*255);
	OGL.GXclearColor.b = (u8) (color[2]*255);
	OGL.GXclearColor.a = (u8) (color[3]*255);

	OGL.GXclearColorBuffer = true;
	gDP.changed |= CHANGED_RENDERMODE;
#endif // __GX__
}

void OGL_SaveScreenshot()
{
#ifndef __LINUX__
	BITMAPFILEHEADER fileHeader;
	BITMAPINFOHEADER infoHeader;
	HANDLE hBitmapFile;

	char *pixelData = (char*)malloc( OGL.width * OGL.height * 3 );

	glReadBuffer( GL_FRONT );
	glReadPixels( 0, OGL.heightOffset, OGL.width, OGL.height, GL_BGR_EXT, GL_UNSIGNED_BYTE, pixelData );
	glReadBuffer( GL_BACK );

	infoHeader.biSize = sizeof( BITMAPINFOHEADER );
	infoHeader.biWidth = OGL.width;
	infoHeader.biHeight = OGL.height;
	infoHeader.biPlanes = 1;
	infoHeader.biBitCount = 24;
	infoHeader.biCompression = BI_RGB;
	infoHeader.biSizeImage = OGL.width * OGL.height * 3;
	infoHeader.biXPelsPerMeter = 0;
	infoHeader.biYPelsPerMeter = 0;
	infoHeader.biClrUsed = 0;
	infoHeader.biClrImportant = 0;

	fileHeader.bfType = 19778;
	fileHeader.bfSize = sizeof( BITMAPFILEHEADER ) + sizeof( BITMAPINFOHEADER ) + infoHeader.biSizeImage;
	fileHeader.bfReserved1 = fileHeader.bfReserved2 = 0;
	fileHeader.bfOffBits = sizeof( BITMAPFILEHEADER ) + sizeof( BITMAPINFOHEADER );

	char filename[256];

	CreateDirectory( screenDirectory, NULL );

	int i = 0;
	do
	{
		sprintf( filename, "%sscreen%02i.bmp", screenDirectory, i );
		i++;

		if (i > 99)
			return;

		hBitmapFile = CreateFile( filename, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL );
	}
	while (hBitmapFile == INVALID_HANDLE_VALUE);
	
	DWORD written;

	WriteFile( hBitmapFile, &fileHeader, sizeof( BITMAPFILEHEADER ), &written, NULL );
    WriteFile( hBitmapFile, &infoHeader, sizeof( BITMAPINFOHEADER ), &written, NULL );
    WriteFile( hBitmapFile, pixelData, infoHeader.biSizeImage, &written, NULL );

 	CloseHandle( hBitmapFile );
	free( pixelData );
#else // !__LINUX__
#endif // __LINUX__
}

#ifdef __LINUX__
void
OGL_SwapBuffers()
{
#ifndef __GX__
	static int frames[5] = { 0, 0, 0, 0, 0 };
	static int framesIndex = 0;
	static Uint32 lastTicks = 0;
	Uint32 ticks = SDL_GetTicks();

	frames[framesIndex]++;
	if (ticks >= (lastTicks + 1000))
	{
		char caption[500];
		float fps = 0.0;
		for (int i = 0; i < 5; i++)
			fps += frames[i];
		fps /= 5.0;
		snprintf( caption, 500, "%s - %.2f fps", pluginName, fps );
		SDL_WM_SetCaption( caption, pluginName );
		framesIndex = (framesIndex + 1) % 5;
		frames[framesIndex] = 0;
		lastTicks = ticks;
	}

	SDL_GL_SwapBuffers();
#endif // !__GX__
}

void OGL_ReadScreen( void **dest, long *width, long *height )
{
#ifndef __GX__
	*width = OGL.width;
	*height = OGL.height;

	*dest = malloc( OGL.height * OGL.width * 3 );
	if (*dest == 0)
		return;

	GLint oldMode;
	glGetIntegerv( GL_READ_BUFFER, &oldMode );
	glReadBuffer( GL_FRONT );
//	glReadBuffer( GL_BACK );
	glReadPixels( 0, 0, OGL.width, OGL.height,
	              GL_BGR, GL_UNSIGNED_BYTE, *dest );
	glReadBuffer( oldMode );
#endif // !__GX__ Note: This is a VCR function.
}

#endif // __LINUX__

#ifdef __GX__
void OGL_GXinitDlist()
{
//	if(VI.copy_fb)
//		VIDEO_WaitVSync();

	OGL.frameBufferTextures = glN64_useFrameBufferTextures;
	OGL.enable2xSaI = glN64_use2xSaiTextures;

	// init primeDepthZtex, Ztexture, AlphaCompare, and Texture Clamping
	TextureCache_UpdatePrimDepthZtex( 1.0f );
	GX_SetZTexture(GX_ZT_DISABLE,GX_TF_Z16,0);	//GX_ZT_DISABLE or GX_ZT_REPLACE; set in gDP.cpp
	OGL.GXuseAlphaCompare = false;
	GX_SetZCompLoc(GX_TRUE);	// Do Z-compare before texturing.
	OGL.GXrenderTexRect = false;
	OGL.GXforceClampS0 = false;
	OGL.GXforceClampT0 = false;
	OGL.GXforceClampS1 = false;
	OGL.GXforceClampT1 = false;
	OGL.GXuseMinMagNearest = false;

	// init fog
	OGL.GXfogStartZ = -1.0f;
	OGL.GXfogEndZ = 1.0f;
	OGL.GXfogColor = (GXColor){0,0,0,255};
	OGL.GXfogType = GX_FOG_NONE;
	GX_SetFog(OGL.GXfogType,OGL.GXfogStartZ,OGL.GXfogEndZ,-1.0,1.0,OGL.GXfogColor);
	OGL.GXupdateFog = false;

	//Reset Modelview matrix
	guMtxIdentity(OGL.GXmodelViewIdent);
	GX_LoadPosMtxImm(OGL.GXmodelViewIdent,GX_PNMTX0);
	GX_LoadTexMtxImm(OGL.GXmodelViewIdent,GX_TEXMTX0,GX_MTX2x4);

	//Reset projection matrix
	guMtxIdentity(OGL.GXcombW);
	guMtxIdentity(OGL.GXprojWnear);
	guMtxIdentity(OGL.GXprojIdent);
//	guOrtho(OGL.GXprojIdent, -1, 1, -1, 1, 1.0f, -1.0f);
	//N64 Z clip space is backwards, so mult z components by -1
	//N64 Z [-1,1] whereas GC Z [-1,0], so mult by 0.5 and shift by -0.5
	OGL.GXcombW[3][2] = -1;
	OGL.GXcombW[3][3] = 0;
	OGL.GXprojWnear[2][2] = 0.0f;
	OGL.GXprojWnear[2][3] = -0.5f;
	OGL.GXprojWnear[3][2] = -1.0f;
	OGL.GXprojWnear[3][3] = 0.0f;
	OGL.GXprojIdent[2][2] = GXprojZScale; //0.5;
	OGL.GXprojIdent[2][3] = GXprojZOffset; //-0.5;
	GX_LoadProjectionMtx(OGL.GXprojIdent, GX_ORTHOGRAPHIC);
	OGL.GXpolyOffset = false;

	OGL.GXnumVtxMP = 0;
	cache.GXprimDepthCnt = cache.GXZTexPrimCnt = cache.GXnoZTexPrimCnt = 0;

	//Not sure if this is needed.  Clipping is a slow process...
	//Note: gx.h has GX_CLIP_ENABLE and GX_CLIP_DISABLE backwards!!
	GX_SetClipMode(GX_CLIP_ENABLE);
//	GX_SetClipMode(GX_CLIP_DISABLE);

	//These are here temporarily until combining/blending is sorted out...
	//Set PassColor TEV mode
	GX_SetNumChans (1);
	GX_SetNumTexGens (0);
	GX_SetTevOrder (GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
	GX_SetTevOp (GX_TEVSTAGE0, GX_PASSCLR);
	//Set CopyModeDraw blend modes here
	GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR); //Fix src alpha
	GX_SetColorUpdate(GX_ENABLE);
	GX_SetAlphaUpdate(GX_ENABLE);
	GX_SetDstAlpha(GX_DISABLE, 0xFF);
//	OGL.GXclearColorBuffer = false;
//	OGL.GXclearDepthBuffer = false;
	OGL.GXclearColor = (GXColor){0,0,0,255};
}

extern GXRModeObj *vmode, *rmode;

void OGL_GXclearEFB()
{
	//Note: EFB is RGB8, so no need to clear alpha
	if(OGL.GXclearColorBuffer)	GX_SetColorUpdate(GX_ENABLE);
	else						GX_SetColorUpdate(GX_DISABLE);
	if(OGL.GXclearDepthBuffer)	GX_SetZMode(GX_ENABLE,GX_GEQUAL,GX_TRUE);
	else						GX_SetZMode(GX_ENABLE,GX_GEQUAL,GX_FALSE);

	GX_SetNumChans(1);
	GX_SetNumTexGens(0);
	GX_SetNumTevStages(1);
	GX_SetTevOp (GX_TEVSTAGE0, GX_PASSCLR);
	GX_SetTevOrder (GX_TEVSTAGE0, GX_TEXMAP_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
	GX_SetBlendMode(GX_BM_NONE, GX_BL_ONE, GX_BL_ZERO, GX_LO_CLEAR); 
	GX_SetZTexture(GX_ZT_DISABLE,GX_TF_Z16,0);	//GX_ZT_DISABLE or GX_ZT_REPLACE; set in gDP.cpp
	GX_SetZCompLoc(GX_TRUE);	// Do Z-compare before texturing.
	GX_SetAlphaCompare(GX_ALWAYS,0,GX_AOP_AND,GX_ALWAYS,0);
	GX_SetFog(GX_FOG_NONE,0.1,1.0,0.0,1.0,(GXColor){0,0,0,255});
	GX_SetViewport((f32) OGL.GXorigX,(f32) OGL.GXorigY,(f32) OGL.GXwidth,(f32) OGL.GXheight, 0.0f, 1.0f);
	GX_SetScissor((u32) 0,(u32) 0,(u32) OGL.width+1,(u32) OGL.height+1);	//Disable Scissor
//	GX_SetScissor(0,0,rmode->fbWidth,rmode->efbHeight);
	GX_SetCullMode (GX_CULL_NONE);
	Mtx44 GXprojection;
	guMtxIdentity(GXprojection);
	guOrtho(GXprojection, 0, OGL.height-1, 0, OGL.width-1, 0.0f, 1.0f);
	GX_LoadProjectionMtx(GXprojection, GX_ORTHOGRAPHIC); 
	GX_LoadPosMtxImm(OGL.GXmodelViewIdent,GX_PNMTX0);

	//set vertex description here
	GX_ClearVtxDesc();
	GX_SetVtxDesc(GX_VA_PTNMTXIDX, GX_PNMTX0);
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
	f32 ZmaxDepth = (f32) -0xFFFFFF/0x1000000;
	GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
		GX_Position3f32(-1.0f, -1.0f, ZmaxDepth);
		GX_Color4u8(OGL.GXclearColor.r, OGL.GXclearColor.g, OGL.GXclearColor.b, OGL.GXclearColor.a); 
		GX_Position3f32((f32) OGL.width+1, -1.0f, ZmaxDepth);
		GX_Color4u8(OGL.GXclearColor.r, OGL.GXclearColor.g, OGL.GXclearColor.b, OGL.GXclearColor.a); 
		GX_Position3f32((f32) OGL.width+1,(f32) OGL.height+1, ZmaxDepth);
		GX_Color4u8(OGL.GXclearColor.r, OGL.GXclearColor.g, OGL.GXclearColor.b, OGL.GXclearColor.a); 
		GX_Position3f32(-1.0f,(f32) OGL.height+1, ZmaxDepth);
		GX_Color4u8(OGL.GXclearColor.r, OGL.GXclearColor.g, OGL.GXclearColor.b, OGL.GXclearColor.a); 
	GX_End();

	if (OGL.GXclearColorBuffer) VI.EFBcleared = true;
	OGL.GXclearColorBuffer = false;
	OGL.GXclearDepthBuffer = false;
	OGL.GXupdateMtx = true;
	OGL.GXupdateFog = true;
	GX_SetColorUpdate(GX_ENABLE);
	OGL_UpdateCullFace();
	OGL_UpdateViewport();
	gDP.changed |= CHANGED_SCISSOR | CHANGED_COMBINE | CHANGED_RENDERMODE;	//Restore scissor in OGL_UpdateStates() before drawing next geometry.

/*	OGL.GXclearBufferTex = (u8*) memalign(32,640*480/2);
	GX_SetCopyClear (OGL.GXclearColor, 0xFFFFFF);
	if(OGL.GXclearColorBuffer)	GX_SetColorUpdate(GX_ENABLE);
	else						GX_SetColorUpdate(GX_DISABLE);
	if(OGL.GXclearDepthBuffer)	GX_SetZMode(GX_ENABLE,GX_ALWAYS,GX_TRUE);
	else						GX_SetZMode(GX_ENABLE,GX_ALWAYS,GX_FALSE);
	GX_SetTexCopySrc(0, 0, 640, 480);
	GX_SetTexCopyDst(640, 480, GX_TF_I4, GX_FALSE);
	GX_CopyTex(OGL.GXclearBufferTex, GX_TRUE);
	GX_PixModeSync();
//	GX_DrawDone();
	GX_SetColorUpdate(GX_ENABLE);
	free(OGL.GXclearBufferTex);
	gDP.changed |= CHANGED_RENDERMODE;*/
}
#endif // __GX__
