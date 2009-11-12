/**
 * glN64_GX - texture_env.h
 * Copyright (C) 2003 Orkin
 * Copyright (C) 2008, 2009 sepp256 (Port to Wii/Gamecube/PS3)
 *
 * glN64 homepage: http://gln64.emulation64.com
 * Wii64 homepage: http://www.emulatemii.com
 * email address: sepp256@gmail.com
 *
**/

#ifndef TEXTURE_ENV_H
#define TEXTURE_ENV_H

struct TexEnv
{
	GLint mode;

	struct
	{
		WORD color, alpha;
	} fragment;

	BOOL usesT0, usesT1;
};

void Init_texture_env();
TexEnv *Compile_texture_env( Combiner *color, Combiner *alpha );
void Set_texture_env( TexEnv *texEnv );
void Update_texture_env_Colors( TexEnv *texEnv );
void Uninit_texture_env();

#endif

