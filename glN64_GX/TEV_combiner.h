#ifndef TEV_COMBINER_H
#define TEV_COMBINER_H

//TEV input sources
#define TEV_PRIM		1
#define TEV_SHADE		2
#define TEV_ENV			3
#define TEV_CENTER		4
#define TEV_SCALE		5
#define TEV_LODFRAC		6
#define TEV_PRIMLODFRAC	7
#define TEV_K4			8
#define TEV_K5			9
#define TEV_ONE			10
#define TEV_ZERO		11
#define TEV_CMB			12
#define TEV_TEX			13

struct TEVconstantRegs
{
	u8 tev_regid, selC_RGB, selC_AAA, selA_A;
}

struct TEVCombinerArg
{
	u8 TEV_inpType, TEV_colorIn, TEV_alphaIn;
};

struct TEVCombinerStage
{
	WORD constant;
	BOOL used;
	GLenum combine;
	TEVCombinerArg arg0, arg1, arg2;
	WORD outputTexture;
};

struct TEVCombiner
{
	BOOL usesT0, usesT1, usesNoise;

	WORD usedUnits;
	
	struct
	{
		WORD color, secondaryColor, alpha;
	} vertex;

	TEVCombinerStage color[8];
	TEVCombinerStage alpha[8];
};

void Init_TEV_combine();
void Uninit_TEV_combine();
void Update_TEV_combine_Colors( TEVCombiner* );
TEVCombiner *Compile_TEV_combine( Combiner *color, Combiner *alpha );
void BeginTextureUpdate_TEV_combine();
void EndTextureUpdate_TEV_combine();
void Set_TEV_combine( TEVCombiner *TEVcombiner );
#endif

