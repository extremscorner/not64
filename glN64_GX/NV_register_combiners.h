/**
 * glN64_GX - NV_register_combiners.h
 * Copyright (C) 2003 Orkin
 * Copyright (C) 2008, 2009 sepp256 (Port to Wii/Gamecube/PS3)
 *
 * glN64 homepage: http://gln64.emulation64.com
 * Wii64 homepage: http://www.emulatemii.com
 * email address: sepp256@gmail.com
 *
**/

#ifndef __GX__
#include <GL/gl.h>
#else // !__GX__
#define __WIN32__
#include "gl.h" 
#include "glext.h"
#undef __WIN32__
#endif // __GX__

struct CombinerInput
{
	GLenum input;
	GLenum mapping;
	GLenum usage;
};

struct CombinerVariable
{
	GLenum input;
	GLenum mapping;
	GLenum usage;
	BOOL used;
};

struct GeneralCombiner
{
	CombinerVariable A, B, C, D;

	struct
	{
		GLenum ab;
		GLenum cd;
		GLenum sum;
	} output;
};

struct RegisterCombiners
{
	GeneralCombiner color[8];
	GeneralCombiner alpha[8];

	struct
	{
		CombinerVariable A, B, C, D, E, F, G;
	} final;

	struct 
	{
		WORD color, alpha;
	} constant[2];

	struct
	{
		WORD color, secondaryColor, alpha;
	} vertex;

	WORD numCombiners;
	BOOL usesT0, usesT1, usesNoise;
};

void Init_NV_register_combiners();
void Uninit_NV_register_combiners();
RegisterCombiners *Compile_NV_register_combiners( Combiner *color, Combiner *alpha );
void Update_NV_register_combiners_Colors( RegisterCombiners *regCombiners );
void Set_NV_register_combiners( RegisterCombiners *regCombiners );
