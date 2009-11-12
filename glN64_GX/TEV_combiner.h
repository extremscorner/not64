/**
 * glN64_GX - TEV_combiner.h
 * Copyright (C) 2008, 2009 sepp256 (Port to Wii/Gamecube/PS3)
 *
 * glN64 homepage: http://gln64.emulation64.com
 * Wii64 homepage: http://www.emulatemii.com
 * email address: sepp256@gmail.com
 *
**/

#ifndef TEV_COMBINER_H
#define TEV_COMBINER_H

//TEV input sources
#define TEV_PRIM		0	//Constant
#define TEV_ENV			1	//Constant
#define TEV_CENTER		2	//Constant
#define TEV_SCALE		3	//Constant
#define TEV_LODFRAC		4	//Constant
#define TEV_PRIMLODFRAC	5	//Constant
#define TEV_K4			6	//Constant
#define TEV_K5			7	//Constant
#define TEV_SHADE		9	//Raster Color
#define TEV_ONE			10	//Static
#define TEV_ZERO		11	//Static
#define TEV_CMB			12	//TEVreg0
#define TEV_TEX0		13	//Texture
#define TEV_TEX1		14	//Texture
#define TEV_TEX			15	//Texture (type)

#define TEV_MAX_CONST	8
#define TEV_MAX_STAGES	16

struct TEVconstantRegs
{
	u8 tev_regid, selC_RGB, selC_AAA, selA_A;
};

struct TEVCombinerArg
{
	u8 TEV_inpType, TEV_colorIn, TEV_alphaIn, TEV_TexCoordMap;
	bool KonstAAA;
};

struct TEVCombinerStage
{
	WORD constant;
//	BOOL used;
//	GLenum combine;
//	TEVCombinerArg arg0, arg1, arg2;
//	WORD outputTexture;

	u8 texcoord, texmap, color;					//inputs for GX_SetTevOrder(u8 tevstage,u8 texcoord,u32 texmap,u8 color)
	u8 colorA, colorB, colorC, colorD;			//inputs for GX_SetTevColorIn(u8 tevstage,u8 a,u8 b,u8 c,u8 d)
	u8 colorTevop, colorClamp, colorTevRegOut;	//inputs for GX_SetTevColorOp(u8 tevstage,u8 tevop,u8 tevbias,u8 tevscale,u8 clamp,u8 tevregid)
	u8 alphaA, alphaB, alphaC, alphaD;			//inputs for GX_SetTevAlphaIn(u8 tevstage,u8 a,u8 b,u8 c,u8 d)
	u8 alphaTevop, alphaClamp, alphaTevRegOut;	//inputs for GX_SetTevAlphaOp(u8 tevstage,u8 tevop,u8 tevbias,u8 tevscale,u8 clamp,u8 tevregid)
	u8 tevKColSel, tevKAlphaSel;				//inputs for GX_SetTevKColorSel(u8 tevstage,u8 sel) & GX_SetTevKAlphaSel(u8 tevstage,u8 sel)

};

struct TEVCombiner
{
	BOOL usesT0, usesT1, usesNoise;

	u32 numTexGens;		//input for GX_SetNumTexGens(u32 nr)
	u8 numColChans;		//input for GX_SetNumChans(u8 num)
	u8 numTevStages;	//input for GX_SetNumTevStages(u8 num)
	u8 TEVconstant[TEV_MAX_CONST];	//for determining Konst color regs
	u8 numConst;

//	WORD usedUnits;
	
/*	struct
	{
		WORD color, secondaryColor, alpha;
	} vertex;*/

	TEVCombinerStage TEVstage[TEV_MAX_STAGES];
};

void Init_TEV_combine();
void Uninit_TEV_combine();
void Update_TEV_combine_Colors( TEVCombiner* );
TEVCombiner *Compile_TEV_combine( Combiner *color, Combiner *alpha );
void BeginTextureUpdate_TEV_combine();
void EndTextureUpdate_TEV_combine();
void Set_TEV_combine( TEVCombiner *TEVcombiner );
#endif

