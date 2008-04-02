#ifndef TEXTURE_ENV_COMBINE_H
#define TEXTURE_ENV_COMBINE_H

struct TexEnvCombinerArg
{
	GLenum source, operand;
};

struct TexEnvCombinerStage
{
	WORD constant;
	BOOL used;
	GLenum combine;
	TexEnvCombinerArg arg0, arg1, arg2;
	WORD outputTexture;
};

struct TexEnvCombiner
{
	BOOL usesT0, usesT1, usesNoise;

	WORD usedUnits;
	
	struct
	{
		WORD color, secondaryColor, alpha;
	} vertex;

	TexEnvCombinerStage color[8];
	TexEnvCombinerStage alpha[8];
};

void Init_texture_env_combine();
TexEnvCombiner *Compile_texture_env_combine( Combiner *color, Combiner *alpha );
void Set_texture_env_combine( TexEnvCombiner *envCombiner );
void Update_texture_env_combine_Colors( TexEnvCombiner* );
void Uninit_texture_env_combine();
void BeginTextureUpdate_texture_env_combine();
void EndTextureUpdate_texture_env_combine();
#endif

