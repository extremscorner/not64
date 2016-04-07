/**
 * glN64_GX - TEV_combiner.cpp
 * Copyright (C) 2008, 2009 sepp256 (Port to Wii/Gamecube/PS3)
 *
 * glN64 homepage: http://gln64.emulation64.com
 * Wii64 homepage: http://www.emulatemii.com
 * email address: sepp256@gmail.com
 *
**/

//#ifdef __GX__
#include <gccore.h>
#include "../gui/DEBUG.h"
#include <stdio.h>
//#endif // __GX__

//#ifndef __LINUX__
//# include <windows.h>
//#else
# include "../main/winlnxdefs.h"
# include <stdlib.h> // malloc()

# ifndef max
#  define max(a,b) ((a) > (b) ? (a) : (b))
# endif
//#endif
#include "OpenGL.h"
#include "Combiner.h"
#include "TEV_combiner.h"

#define NULL_CONST_REG 7
#define MAX_K_REGS 4

static TEVconstantRegs TEVconstRegs[] =
{ //  Load_Const	Color_RGB_input		Color_A_input		Alpha_A_input
	{ GX_TEVPREV,	GX_TEV_KCSEL_K0,	GX_TEV_KCSEL_K0_A,	GX_TEV_KASEL_K0_A }, //Konst0
	{ GX_TEVREG0,	GX_TEV_KCSEL_K1,	GX_TEV_KCSEL_K1_A,	GX_TEV_KASEL_K1_A }, //Konst1
	{ GX_TEVREG1,	GX_TEV_KCSEL_K2,	GX_TEV_KCSEL_K2_A,	GX_TEV_KASEL_K2_A }, //Konst2
	{ GX_TEVREG2,	GX_TEV_KCSEL_K3,	GX_TEV_KCSEL_K3_A,	GX_TEV_KASEL_K3_A }, //Konst3
	{ GX_TEVREG2,	GX_CC_C2,			GX_CC_A2,			GX_CA_A2 }, //Reg2
	{ GX_TEVREG1,	GX_CC_C1,			GX_CC_A1,			GX_CA_A1 }, //Reg1
	{ GX_TEVREG0,	GX_CC_C0,			GX_CC_A0,			GX_CA_A0 },	//Reg0 - Only used with 7 constant inputs, but conflicts with COMBINED
	{ GX_TEVREG0,	GX_TEV_KCSEL_1,		GX_TEV_KCSEL_1,		GX_TEV_KASEL_1 }	//NULL_CONST_REG - just set these K inputs to 1.
};

static TEVCombinerArg TEVArgs[] =
{	//TEV_inpType,	TEV_colorIn,	TEV_alphaIn,	TEV_TexCoordMap,KonstAAA
	// CMB - Combined color
	{ TEV_CMB,		GX_CC_C0,		GX_CA_A0,		GX_TEXMAP_NULL,	FALSE },
//	{ TEV_CMB,		GX_CC_RASC,		GX_CA_RASA,		GX_TEXMAP_NULL,	FALSE }, // TEVArgs[i+4]
//	{ TEV_CMB,		GX_CC_CPREV,	GX_CA_APREV,	GX_TEXMAP_NULL,	FALSE }, // for non-zero fragment
	// T0
	{ TEV_TEX,		GX_CC_TEXC,		GX_CA_TEXA,		GX_TEXMAP0,		FALSE },
	// T1
	{ TEV_TEX,		GX_CC_TEXC,		GX_CA_TEXA,		GX_TEXMAP1,		FALSE },
	// PRIM
	{ TEV_PRIM,		GX_CC_KONST,	GX_CA_KONST,	GX_TEXMAP_NULL,	FALSE },
	// SHADE
	{ TEV_SHADE,	GX_CC_RASC,		GX_CA_RASA,		GX_TEXMAP_NULL, FALSE },
	// ENV
	{ TEV_ENV,		GX_CC_KONST,	GX_CA_KONST,	GX_TEXMAP_NULL, FALSE },
	// CENTER
	{ TEV_CENTER,	GX_CC_KONST,	GX_CA_KONST,	GX_TEXMAP_NULL, FALSE },
	// SCALE
	{ TEV_SCALE,	GX_CC_KONST,	GX_CA_KONST,	GX_TEXMAP_NULL, FALSE },
	// CMBALPHA
	{ TEV_CMB,		GX_CC_A0,		GX_CA_A0,		GX_TEXMAP_NULL, FALSE },
//	{ TEV_CMB,		GX_CC_RASA,		GX_CA_RASA,		GX_TEXMAP_NULL, FALSE }, // TEVArgs[i+4]
//	{ TEV_CMB,		GX_CC_APREV,	GX_CA_APREV,	GX_TEXMAP_NULL,	FALSE }, // for non-zero fragment
	// T0ALPHA
	{ TEV_TEX,		GX_CC_TEXA,		GX_CA_TEXA,		GX_TEXMAP0,		FALSE },
	// T1ALPHA
	{ TEV_TEX,		GX_CC_TEXA,		GX_CA_TEXA,		GX_TEXMAP1,		FALSE },
	// PRIMALPHA
	{ TEV_PRIM,		GX_CC_KONST,	GX_CA_KONST,	GX_TEXMAP_NULL,	TRUE },
	// SHADEALPHA
	{ TEV_SHADE,	GX_CC_RASA,		GX_CA_RASA,		GX_TEXMAP_NULL,	FALSE },
	// ENVALPHA
	{ TEV_ENV,		GX_CC_KONST,	GX_CA_KONST,	GX_TEXMAP_NULL,	TRUE },
	// LODFRAC
	{ TEV_LODFRAC,	GX_CC_KONST,	GX_CA_KONST,	GX_TEXMAP_NULL,	FALSE },
	// PRIMLODFRAC
	{ TEV_PRIMLODFRAC, GX_CC_KONST,	GX_CA_KONST,	GX_TEXMAP_NULL,	FALSE },
	// NOISE
	{ TEV_TEX,		GX_CC_TEXC,		GX_CA_TEXA,		GX_TEXMAP3,		FALSE },
	// K4
	{ TEV_K4,		GX_CC_KONST,	GX_CA_KONST,	GX_TEXMAP_NULL,	FALSE },
	// K5
	{ TEV_K5,		GX_CC_KONST,	GX_CA_KONST,	GX_TEXMAP_NULL,	FALSE },
	// ONE			set Kreg for alpha
	{ TEV_ONE,		GX_CC_ONE,		GX_CA_KONST,	GX_TEXMAP_NULL,	FALSE },
//	{ TEV_ZERO,		GX_CC_ZERO,		GX_CA_ZERO,		GX_TEXMAP_NULL,	FALSE },
	// ZERO
	{ TEV_ZERO,		GX_CC_ZERO,		GX_CA_ZERO,		GX_TEXMAP_NULL,	FALSE },
//	{ TEV_ONE,		GX_CC_ONE,		GX_CA_KONST,	GX_TEXMAP_NULL,	FALSE },
	// TEV_CMB_STAGE1 - stage 1, op > 0
	{ TEV_CMB,		GX_CC_CPREV,	GX_CA_APREV,	GX_TEXMAP_NULL,	FALSE } 
};

#define TEV_CMB_STAGE1 21

//SetAlphaTEV(tevstage, alphaA, alphaB, alphaC, alphaD, TEVop, alphaTEX, alphaCONST, AoutReg)
#define SetAlphaTEV(tevstage, AlphaA, AlphaB, AlphaC, AlphaD, TEVop, alphaTEX, alphaCONST, AoutReg) \
	TEVcombiner->TEVstage[tevstage].alphaA = AlphaA; \
	TEVcombiner->TEVstage[tevstage].alphaB = AlphaB; \
	TEVcombiner->TEVstage[tevstage].alphaC = AlphaC; \
	TEVcombiner->TEVstage[tevstage].alphaD = AlphaD; \
	TEVcombiner->TEVstage[tevstage].alphaTevop = TEVop; \
	TEVcombiner->TEVstage[tevstage].alphaTevRegOut = AoutReg; \
	if (TEVcombiner->TEVstage[tevstage].texcoord == GX_TEXMAP_NULL) TEVcombiner->TEVstage[tevstage].texcoord = alphaTEX; \
	if (TEVcombiner->TEVstage[tevstage].texmap == GX_TEXMAP_NULL) TEVcombiner->TEVstage[tevstage].texmap = alphaTEX; \
	TEVcombiner->TEVstage[tevstage].color = GX_COLOR0A0; \
	TEVcombiner->TEVstage[tevstage].tevKAlphaSel = GX_TEV_KASEL_1; \
	if (alphaCONST <= TEV_MAX_CONST) \
	{ \
		if (TEVcombiner->TEVconstant[alphaCONST] == NULL_CONST_REG) \
		{ \
			TEVcombiner->numConst++; \
			TEVcombiner->TEVconstant[alphaCONST] = TEVcombiner->numConst - 1; \
			TEVcombiner->TEVstage[tevstage].tevKAlphaSel = TEVconstRegs[TEVcombiner->TEVconstant[alphaCONST]].selA_A; \
		} \
		else \
		{ \
			TEVcombiner->TEVstage[tevstage].tevKAlphaSel = TEVconstRegs[TEVcombiner->TEVconstant[alphaCONST]].selA_A; \
		} \
		if (TEVcombiner->TEVconstant[alphaCONST] >= MAX_K_REGS) \
		{ \
			TEVcombiner->TEVstage[tevstage].tevKAlphaSel = TEVconstRegs[0].selA_A; \
			if (AlphaA == GX_CA_KONST) TEVcombiner->TEVstage[tevstage].alphaA = TEVconstRegs[TEVcombiner->TEVconstant[alphaCONST]].selA_A; \
			if (AlphaB == GX_CA_KONST) TEVcombiner->TEVstage[tevstage].alphaB = TEVconstRegs[TEVcombiner->TEVconstant[alphaCONST]].selA_A; \
			if (AlphaC == GX_CA_KONST) TEVcombiner->TEVstage[tevstage].alphaC = TEVconstRegs[TEVcombiner->TEVconstant[alphaCONST]].selA_A; \
			if (AlphaD == GX_CA_KONST) TEVcombiner->TEVstage[tevstage].alphaD = TEVconstRegs[TEVcombiner->TEVconstant[alphaCONST]].selA_A; \
		} \
	}

//SetAlphaTEVskip(tevstage)
#define SetAlphaTEVskip(tevstage) \
	TEVcombiner->TEVstage[tevstage].alphaA = GX_CA_ZERO; \
	TEVcombiner->TEVstage[tevstage].alphaB = GX_CA_ZERO; \
	TEVcombiner->TEVstage[tevstage].alphaC = GX_CA_ZERO; \
	TEVcombiner->TEVstage[tevstage].alphaD = GX_CA_APREV; \
	TEVcombiner->TEVstage[tevstage].alphaTevop = GX_TEV_ADD; \
	TEVcombiner->TEVstage[tevstage].alphaTevRegOut = GX_TEVPREV; \
	TEVcombiner->TEVstage[tevstage].tevKAlphaSel = GX_TEV_KASEL_1; 

//SetColorTEV(tevstage, colorA, colorB, colorC, colorD, TEVop, colorTEX, colorCONST, selC_RGBAAA, CoutReg)
#define SetColorTEV(tevstage, ColorA, ColorB, ColorC, ColorD, TEVop, colorTEX, colorCONST, KonstisAAA, CoutReg) \
	TEVcombiner->TEVstage[tevstage].colorA = ColorA; \
	TEVcombiner->TEVstage[tevstage].colorB = ColorB; \
	TEVcombiner->TEVstage[tevstage].colorC = ColorC; \
	TEVcombiner->TEVstage[tevstage].colorD = ColorD; \
	TEVcombiner->TEVstage[tevstage].colorTevop = TEVop; \
	TEVcombiner->TEVstage[tevstage].colorTevRegOut = CoutReg; \
	if (TEVcombiner->TEVstage[tevstage].texcoord == GX_TEXMAP_NULL) TEVcombiner->TEVstage[tevstage].texcoord = colorTEX; \
	if (TEVcombiner->TEVstage[tevstage].texmap == GX_TEXMAP_NULL) TEVcombiner->TEVstage[tevstage].texmap = colorTEX; \
	TEVcombiner->TEVstage[tevstage].color = GX_COLOR0A0; \
	TEVcombiner->TEVstage[tevstage].tevKColSel = GX_TEV_KCSEL_1; \
	if (colorCONST <= TEV_MAX_CONST) \
	{ \
		if (TEVcombiner->TEVconstant[colorCONST] == NULL_CONST_REG) \
		{ \
			TEVcombiner->numConst++; \
			TEVcombiner->TEVconstant[colorCONST] = TEVcombiner->numConst - 1; \
			if(KonstisAAA) 	TEVcombiner->TEVstage[tevstage].tevKColSel = TEVconstRegs[TEVcombiner->TEVconstant[colorCONST]].selC_AAA; \
			else			TEVcombiner->TEVstage[tevstage].tevKColSel = TEVconstRegs[TEVcombiner->TEVconstant[colorCONST]].selC_RGB; \
		} \
		else \
		{ \
			if(KonstisAAA) 	TEVcombiner->TEVstage[tevstage].tevKColSel = TEVconstRegs[TEVcombiner->TEVconstant[colorCONST]].selC_AAA; \
			else		 	TEVcombiner->TEVstage[tevstage].tevKColSel = TEVconstRegs[TEVcombiner->TEVconstant[colorCONST]].selC_RGB; \
		} \
		if (TEVcombiner->TEVconstant[colorCONST] >= MAX_K_REGS) \
		{ \
			if (KonstisAAA) \
			{ \
				TEVcombiner->TEVstage[tevstage].tevKColSel = TEVconstRegs[0].selC_AAA; \
				if (ColorA == GX_CC_KONST) TEVcombiner->TEVstage[tevstage].colorA = TEVconstRegs[TEVcombiner->TEVconstant[colorCONST]].selC_AAA; \
				if (ColorB == GX_CC_KONST) TEVcombiner->TEVstage[tevstage].colorB = TEVconstRegs[TEVcombiner->TEVconstant[colorCONST]].selC_AAA; \
				if (ColorC == GX_CC_KONST) TEVcombiner->TEVstage[tevstage].colorC = TEVconstRegs[TEVcombiner->TEVconstant[colorCONST]].selC_AAA; \
				if (ColorD == GX_CC_KONST) TEVcombiner->TEVstage[tevstage].colorD = TEVconstRegs[TEVcombiner->TEVconstant[colorCONST]].selC_AAA; \
			} \
			else \
			{ \
				TEVcombiner->TEVstage[tevstage].tevKColSel = TEVconstRegs[0].selC_RGB; \
				if (ColorA == GX_CC_KONST) TEVcombiner->TEVstage[tevstage].colorA = TEVconstRegs[TEVcombiner->TEVconstant[colorCONST]].selC_RGB; \
				if (ColorB == GX_CC_KONST) TEVcombiner->TEVstage[tevstage].colorB = TEVconstRegs[TEVcombiner->TEVconstant[colorCONST]].selC_RGB; \
				if (ColorC == GX_CC_KONST) TEVcombiner->TEVstage[tevstage].colorC = TEVconstRegs[TEVcombiner->TEVconstant[colorCONST]].selC_RGB; \
				if (ColorD == GX_CC_KONST) TEVcombiner->TEVstage[tevstage].colorD = TEVconstRegs[TEVcombiner->TEVconstant[colorCONST]].selC_RGB; \
			} \
		} \
	} 

//SetColorTEVskip(tevstage)
#define SetColorTEVskip(tevstage) \
	TEVcombiner->TEVstage[tevstage].colorA = GX_CC_ZERO; \
	TEVcombiner->TEVstage[tevstage].colorB = GX_CC_ZERO; \
	TEVcombiner->TEVstage[tevstage].colorC = GX_CC_ZERO; \
	TEVcombiner->TEVstage[tevstage].colorD = GX_CC_CPREV; \
	TEVcombiner->TEVstage[tevstage].colorTevop = GX_TEV_ADD; \
	TEVcombiner->TEVstage[tevstage].colorTevRegOut = GX_TEVPREV; \
	TEVcombiner->TEVstage[tevstage].tevKColSel = GX_TEV_KCSEL_1;

void Init_TEV_combine()	//Called at Combiner Init
{
	//Load dummy tex for first 2 textures. Maybe should for 2 more for Noise/FB textures.
	TextureCache_ActivateDummy( 0 );
	TextureCache_ActivateDummy( 1 );

	/*
	if ((OGL.ARB_texture_env_crossbar) || (OGL.NV_texture_env_combine4) || (OGL.ATIX_texture_env_route))
	{
		TexEnvArgs[TEXEL0].source = GL_TEXTURE0_ARB;
		TexEnvArgs[TEXEL0_ALPHA].source = GL_TEXTURE0_ARB;

		TexEnvArgs[TEXEL1].source = GL_TEXTURE1_ARB;
		TexEnvArgs[TEXEL1_ALPHA].source = GL_TEXTURE1_ARB;
	}

	if (OGL.ATI_texture_env_combine3)
	{
		TexEnvArgs[ONE].source = GL_ONE;
		TexEnvArgs[ZERO].source = GL_ZERO;
	}*/
}

void Uninit_TEV_combine()	//Never Called...
{
/*	for (int i = 0; i < OGL.maxTextureUnits; i++)
	{
		glActiveTextureARB( GL_TEXTURE0_ARB + i );
		glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );
	}*/
}

void Update_TEV_combine_Colors( TEVCombiner *TEVcombiner )	//Called from OGL_UpdateStates()
{
	// TODO: Load constants into the TEV here?
/*	GLcolor color;

	for (int i = 0; i < OGL.maxTextureUnits; i++)
	{
		SetConstant( color, envCombiner->color[i].constant, envCombiner->alpha[i].constant );

		glActiveTextureARB( GL_TEXTURE0_ARB + i );
		glTexEnvfv( GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, &color.r );
	}*/

	//Set Constant TEV registers
	GXColor GXconstantColor;

	//Set GX_TEVREG0 to GX_ZERO value in case Combine is used in the first stage.
	GXconstantColor.r = (u8) 255;//0;
	GXconstantColor.g = (u8) 255;//0;
	GXconstantColor.b = (u8) 255;//0;
	GXconstantColor.a = (u8) 255;//0;
	GX_SetTevColor(GX_TEVREG0,GXconstantColor);

	for (int i = 0; i < TEV_MAX_CONST; i++)
	{
		if ( TEVcombiner->TEVconstant[i] != NULL_CONST_REG )
		{
			switch (i)
			{
			case TEV_PRIM:
				GXconstantColor.r = GXcastf32u8(gDP.primColor.r);
				GXconstantColor.g = GXcastf32u8(gDP.primColor.g);
				GXconstantColor.b = GXcastf32u8(gDP.primColor.b);
				GXconstantColor.a = GXcastf32u8(gDP.primColor.a);
				break;
			case TEV_ENV:
				GXconstantColor.r = GXcastf32u8(gDP.envColor.r);
				GXconstantColor.g = GXcastf32u8(gDP.envColor.g);
				GXconstantColor.b = GXcastf32u8(gDP.envColor.b);
				GXconstantColor.a = GXcastf32u8(gDP.envColor.a);
				break;
			case TEV_LODFRAC:	//not sure if LODFRAC == PRIMLODFRAC...
			case TEV_PRIMLODFRAC:
				GXconstantColor.r = GXcastf32u8(gDP.primColor.l);
				GXconstantColor.g = GXcastf32u8(gDP.primColor.l);
				GXconstantColor.b = GXcastf32u8(gDP.primColor.l);
				GXconstantColor.a = GXcastf32u8(gDP.primColor.l);
				break;
			case TEV_K4:
			case TEV_K5:
			case TEV_CENTER:
			case TEV_SCALE:
			default:
				GXconstantColor.r = (u8) 255;
				GXconstantColor.g = (u8) 255;
				GXconstantColor.b = (u8) 255;
				GXconstantColor.a = (u8) 255;
			}
			if ( TEVcombiner->TEVconstant[i] < 4 ) // These constants go into Konst registers
				GX_SetTevKColor(TEVconstRegs[TEVcombiner->TEVconstant[i]].tev_regid,GXconstantColor);

			else // These constants go into normal TEV registers
				GX_SetTevColor(TEVconstRegs[TEVcombiner->TEVconstant[i]].tev_regid,GXconstantColor);

#ifdef DBGCOMBINE
#ifdef GLN64_SDLOG
			sprintf(txtbuffer,"SetTEVConst: TEVinp %d, TEVind %d, TEVregID %d, GXcol %d,%d,%d,%d\n", i, TEVcombiner->TEVconstant[i], TEVconstRegs[TEVcombiner->TEVconstant[i]].tev_regid, GXconstantColor.r, GXconstantColor.g, GXconstantColor.b, GXconstantColor.a);
			DEBUG_print(txtbuffer,DBG_SDGECKOPRINT);
#endif // GLN64_SDLOG
#endif // DBGCOMBINE
		}
	}
}

TEVCombiner *Compile_TEV_combine( Combiner *color, Combiner *alpha )
{
	u8 alphaTEX, alphaCONST, AoutReg, colorTEX, colorCONST, CoutReg;
	bool colorCONST_AAA;
//	bool useT0, useT1;
//	int curUnitA, curUnitC, combinedUnit; //, iA, iC, jA, jC, numConst;
	u8 currStageA, currStageC;

	TEVCombiner *TEVcombiner = (TEVCombiner*)malloc( sizeof( TEVCombiner ) );


/*	for (int i = 0; i < OGL.maxTextureUnits; i++)
	{
		envCombiner->color[i].combine = GL_REPLACE;
		envCombiner->alpha[i].combine = GL_REPLACE;

		SetColorCombinerValues( i, arg0, GL_PREVIOUS_ARB, GL_SRC_COLOR );
		SetColorCombinerValues( i, arg1, GL_PREVIOUS_ARB, GL_SRC_COLOR );
		SetColorCombinerValues( i, arg2, GL_PREVIOUS_ARB, GL_SRC_COLOR );
		envCombiner->color[i].constant = COMBINED;
		envCombiner->color[i].outputTexture = GL_TEXTURE0_ARB + i;

		SetAlphaCombinerValues( i, arg0, GL_PREVIOUS_ARB, GL_SRC_ALPHA );
		SetAlphaCombinerValues( i, arg1, GL_PREVIOUS_ARB, GL_SRC_ALPHA );
		SetAlphaCombinerValues( i, arg2, GL_PREVIOUS_ARB, GL_SRC_ALPHA );
		envCombiner->alpha[i].constant = COMBINED;
		envCombiner->alpha[i].outputTexture = GL_TEXTURE0_ARB + i;
	}*/

	TEVcombiner->usesT0 = FALSE;
	TEVcombiner->usesT1 = FALSE;
	
	for (int i = 0; i < TEV_MAX_CONST; i++)
		TEVcombiner->TEVconstant[i] = NULL_CONST_REG;
	TEVcombiner->numConst = 0;

	for (int i = 0; i < TEV_MAX_STAGES; i++)
	{
		TEVcombiner->TEVstage[i].texmap = GX_TEXMAP_NULL;
		TEVcombiner->TEVstage[i].texcoord = GX_TEXCOORDNULL;
		TEVcombiner->TEVstage[i].color = GX_COLOR0A0;
		TEVcombiner->TEVstage[i].colorClamp = GX_DISABLE;
		TEVcombiner->TEVstage[i].alphaClamp = GX_DISABLE;
	}

/*	envCombiner->vertex.color = COMBINED;
	envCombiner->vertex.secondaryColor = COMBINED;
	envCombiner->vertex.alpha = COMBINED;*/

	currStageA = 0;
	currStageC = 0;
//	iA = 0;
//	iC = 0;
//	jA = 0;
//	jC = 0;

	
	//If we're in the first combine stage, use Rasterized color instead of Combine register.
	for (int j = 0; j < alpha->stage[0].numOps; j++)
	{
		if (TEVArgs[alpha->stage[0].op[j].param1].TEV_inpType == TEV_CMB) alpha->stage[0].op[j].param1 += 4;
		if (alpha->stage[0].op[j].op == INTER)
		{
			if (TEVArgs[alpha->stage[0].op[j].param2].TEV_inpType == TEV_CMB) alpha->stage[0].op[j].param2 += 4;
			if (TEVArgs[alpha->stage[0].op[j].param3].TEV_inpType == TEV_CMB) alpha->stage[0].op[j].param3 += 4;
		}
	}
	for (int j = 0; j < color->stage[0].numOps; j++)
	{
		if (TEVArgs[color->stage[0].op[j].param1].TEV_inpType == TEV_CMB) color->stage[0].op[j].param1 += 4;
		if (color->stage[0].op[j].op == INTER)
		{
			if (TEVArgs[color->stage[0].op[j].param2].TEV_inpType == TEV_CMB) color->stage[0].op[j].param2 += 4;
			if (TEVArgs[color->stage[0].op[j].param3].TEV_inpType == TEV_CMB) color->stage[0].op[j].param3 += 4;
		}
	}

/*	if (TEVArgs[alpha->stage[0].op[0].param1].TEV_inpType == TEV_CMB) alpha->stage[0].op[0].param1 += 4;
	if (alpha->stage[0].op[0].op == INTER)
	{
		if (TEVArgs[alpha->stage[0].op[0].param2].TEV_inpType == TEV_CMB) alpha->stage[0].op[0].param2 += 4;
		if (TEVArgs[alpha->stage[0].op[0].param3].TEV_inpType == TEV_CMB) alpha->stage[0].op[0].param3 += 4;
	}
	if (TEVArgs[color->stage[0].op[0].param1].TEV_inpType == TEV_CMB) color->stage[0].op[0].param1 += 4;
	if (color->stage[0].op[0].op == INTER)
	{
		if (TEVArgs[color->stage[0].op[0].param2].TEV_inpType == TEV_CMB) color->stage[0].op[0].param2 += 4;
		if (TEVArgs[color->stage[0].op[0].param3].TEV_inpType == TEV_CMB) color->stage[0].op[0].param3 += 4;
	}
	//If we're in the first combine stage, use Rasterized color instead of Combine register.
	for (int j = 1; j < alpha->stage[0].numOps; j++)
	{
		if (TEVArgs[alpha->stage[0].op[j].param1].TEV_inpType == TEV_CMB) alpha->stage[0].op[j].param1 = TEV_CMB_STAGE1;
		if (alpha->stage[0].op[j].op == INTER)
		{
			if (TEVArgs[alpha->stage[0].op[j].param2].TEV_inpType == TEV_CMB) alpha->stage[0].op[j].param2 = TEV_CMB_STAGE1;
			if (TEVArgs[alpha->stage[0].op[j].param3].TEV_inpType == TEV_CMB) alpha->stage[0].op[j].param3 = TEV_CMB_STAGE1;
		}
	}
	for (int j = 1; j < color->stage[0].numOps; j++)
	{
		if (TEVArgs[color->stage[0].op[j].param1].TEV_inpType == TEV_CMB) color->stage[0].op[j].param1 = TEV_CMB_STAGE1;
		if (color->stage[0].op[j].op == INTER)
		{
			if (TEVArgs[color->stage[0].op[j].param2].TEV_inpType == TEV_CMB) color->stage[0].op[j].param2 = TEV_CMB_STAGE1;
			if (TEVArgs[color->stage[0].op[j].param3].TEV_inpType == TEV_CMB) color->stage[0].op[j].param3 = TEV_CMB_STAGE1;
		}
	}*/

//	while ( (iA < alpha->numStages) && (jA < alpha->stage[iA].numOps) && (iC < color->numStages) && (jC < alpha->stage[iC].numOps)) 
	for (int i = 0; i < alpha->numStages; i++)
	{
		//Process Alpha for current Stage (iA) & op (jA)
		for (int j = 0; j < alpha->stage[i].numOps; j++)
		{
			float sb = 0.0f;

			if (alpha->stage[i].op[j].param1 == PRIMITIVE_ALPHA)
				sb = gDP.primColor.a;
			else if (alpha->stage[i].op[j].param1 == ENV_ALPHA)
				sb = gDP.envColor.a;
			else if (alpha->stage[i].op[j].param1 == ONE)
				sb = 1.0f;

			//The following code solves some problems due to (A - B) not being a signed value.
			//Instead of (A - B)*C + D do D - (A - B)*C
			if (((alpha->stage[i].numOps - j) >= 3) &&
				(alpha->stage[i].op[j].op == SUB) &&
				(alpha->stage[i].op[j+1].op == MUL) &&
				(alpha->stage[i].op[j+2].op == ADD) &&
				(sb > 0.5f))
			{
				TEVcombiner->usesT0 |= alpha->stage[i].op[j].param1 == TEXEL0_ALPHA;
				TEVcombiner->usesT1 |= alpha->stage[i].op[j].param1 == TEXEL1_ALPHA;
				TEVcombiner->usesT0 |= alpha->stage[i].op[j+1].param1 == TEXEL0_ALPHA;
				TEVcombiner->usesT1 |= alpha->stage[i].op[j+1].param1 == TEXEL1_ALPHA;
				TEVcombiner->usesT0 |= alpha->stage[i].op[j+2].param1 == TEXEL0_ALPHA;
				TEVcombiner->usesT1 |= alpha->stage[i].op[j+2].param1 == TEXEL1_ALPHA;

				if (alpha->stage[i].op[j].param1 == ONE)
				{
//					SetAlphaCombinerValues( curUnitA, arg0, envCombiner->alpha[curUnit].arg0.source, GL_ONE_MINUS_SRC_ALPHA );

/*					useT0 = (TEVArgs[alpha->stage[i].op[j+1].param1].TEV_TexCoordMap == GX_TEXMAP0) || (TEVArgs[alpha->stage[i].op[j+2].param1].TEV_TexCoordMap == GX_TEXMAP0);
					useT1 = (TEVArgs[alpha->stage[i].op[j+1].param1].TEV_TexCoordMap == GX_TEXMAP1) || (TEVArgs[alpha->stage[i].op[j+2].param1].TEV_TexCoordMap == GX_TEXMAP1);
					numConst = 0;
					if ( TEVArgs[alpha->stage[i].op[j+1].param1].TEV_inpType <= TEV_MAX_CONST ) numConst++;
					if ( TEVArgs[alpha->stage[i].op[j+2].param1].TEV_inpType <= TEV_MAX_CONST ) numConst++;
*/
					//if ( (useT0 && useT1) || (numConst > 1) )	//Two TEV stages are required.
					if ( !(((TEVArgs[alpha->stage[i].op[j+1].param1].TEV_TexCoordMap == GX_TEXMAP0) ||
						(TEVArgs[alpha->stage[i].op[j+2].param1].TEV_TexCoordMap == GX_TEXMAP0)) && 
						((TEVArgs[alpha->stage[i].op[j+1].param1].TEV_TexCoordMap == GX_TEXMAP1) ||
						(TEVArgs[alpha->stage[i].op[j+2].param1].TEV_TexCoordMap == GX_TEXMAP1))) && //!(TEX0 && TEX1)
						!(((TEVArgs[alpha->stage[i].op[j+1].param1].TEV_inpType <= TEV_MAX_CONST) && //!(>1 unique constants)
						(TEVArgs[alpha->stage[i].op[j+2].param1].TEV_inpType <= TEV_MAX_CONST) &&
						(TEVArgs[alpha->stage[i].op[j+1].param1].TEV_inpType != TEVArgs[alpha->stage[i].op[j+2].param1].TEV_inpType))))
					{
						//TEVstage[currStageA++] -> *-* ( a[PREV] - ONE(param1) ) * c(param2);
						SetAlphaTEV(currStageA, TEVArgs[alpha->stage[i].op[j+1].param1].TEV_alphaIn, GX_CA_ZERO, GX_CA_APREV, 
							GX_CA_ZERO, GX_TEV_SUB, TEVArgs[alpha->stage[i].op[j+1].param1].TEV_TexCoordMap, 
							TEVArgs[alpha->stage[i].op[j+1].param1].TEV_inpType, GX_TEVPREV);
						//SetAlphaTEV(tevstage, alphaA, alphaB, alphaC, alphaD, TEVop, alphaTEX, alphaCONST, AoutReg)
						currStageA++;
						//TEVstage[currStageA++] -> d(param3) +(*-*) PREV;
						AoutReg = ( ((j+2) == (alpha->stage[i].numOps - 1)) && (i < (alpha->numStages-1))) ? GX_TEVREG0 : GX_TEVPREV;
						SetAlphaTEV(currStageA, TEVArgs[alpha->stage[i].op[j+2].param1].TEV_alphaIn, GX_CA_ZERO, GX_CA_ZERO, GX_CA_APREV, 
							GX_TEV_ADD, TEVArgs[alpha->stage[i].op[j+2].param1].TEV_TexCoordMap, TEVArgs[alpha->stage[i].op[j+2].param1].TEV_inpType, AoutReg);
						if (AoutReg == GX_TEVREG0) TEVcombiner->TEVstage[currStageA].alphaClamp = GX_ENABLE; //Clamp COMBINED!
						currStageA++;
						j+=2;
					}
					else //Can handle this case in a single TEV stage.
					{
						//TEVstage[currStageA++] -> *-* ( a[PREV] - ONE(param1) ) * c(param2) + d(param3);
						alphaTEX = (TEVArgs[alpha->stage[i].op[j+1].param1].TEV_TexCoordMap == GX_TEXMAP_NULL) ? 
							TEVArgs[alpha->stage[i].op[j+2].param1].TEV_TexCoordMap : TEVArgs[alpha->stage[i].op[j+1].param1].TEV_TexCoordMap;
						alphaCONST = (TEVArgs[alpha->stage[i].op[j+1].param1].TEV_inpType <= TEV_MAX_CONST) ?
							TEVArgs[alpha->stage[i].op[j+1].param1].TEV_inpType : TEVArgs[alpha->stage[i].op[j+2].param1].TEV_inpType;
						AoutReg = ( ((j+2) == (alpha->stage[i].numOps - 1)) && (i < (alpha->numStages-1))) ? GX_TEVREG0 : GX_TEVPREV;
						SetAlphaTEV(currStageA, TEVArgs[alpha->stage[i].op[j+1].param1].TEV_alphaIn, GX_CA_ZERO, GX_CA_APREV, 
							TEVArgs[alpha->stage[i].op[j+2].param1].TEV_alphaIn, GX_TEV_SUB, alphaTEX, alphaCONST, AoutReg);
						if (AoutReg == GX_TEVREG0) TEVcombiner->TEVstage[currStageA].alphaClamp = GX_ENABLE; //Clamp COMBINED!
						//SetAlphaTEV(tevstage, alphaA, alphaB, alphaC, alphaD, TEVop, alphaTEX, alphaCONST, AoutReg)
						currStageA++;
						j+=2;
					}
				}
				else //Need 3 TEV stages for this case.
				{
/*					envCombiner->alpha[curUnit].combine = GL_SUBTRACT_ARB;
					SetAlphaCombinerValues( curUnit, arg1, envCombiner->alpha[curUnit].arg0.source, GL_SRC_ALPHA );
					SetAlphaCombinerArg( curUnit, arg0, alpha->stage[i].op[j].param1 );

					curUnit++;*/

					//TEVstage[currStageA++] -> ( d[PREV] - a(param1) )
					SetAlphaTEV(currStageA, TEVArgs[alpha->stage[i].op[j].param1].TEV_alphaIn, GX_CA_ZERO, GX_CA_ZERO, GX_CA_APREV, 
						GX_TEV_SUB, TEVArgs[alpha->stage[i].op[j].param1].TEV_TexCoordMap, 
						TEVArgs[alpha->stage[i].op[j].param1].TEV_inpType, GX_TEVPREV);
					//SetAlphaTEV(tevstage, alphaA, alphaB, alphaC, alphaD, TEVop, alphaTEX, alphaCONST, AoutReg)
					currStageA++;
					//TEVstage[currStageA++] -> *-*( c[PREV] ) * b(param2);
					SetAlphaTEV(currStageA, GX_CA_ZERO, GX_CA_APREV,  TEVArgs[alpha->stage[i].op[j+1].param1].TEV_alphaIn, GX_CA_ZERO,
						GX_TEV_SUB, TEVArgs[alpha->stage[i].op[j+1].param1].TEV_TexCoordMap, 
						TEVArgs[alpha->stage[i].op[j+1].param1].TEV_inpType, GX_TEVPREV);
					//SetAlphaTEVinputs(tevstage, alphaA, alphaB, alphaC, alphaD, alphaTEX, alphaCONST, AoutReg)
					TEVcombiner->TEVstage[currStageA-1].alphaClamp = GX_ENABLE; //TEVPREV used in Mult!
					currStageA++;
					//TEVstage[currStageA++] -> d(param3) + PREV;
					AoutReg = ( ((j+2) == (alpha->stage[i].numOps - 1)) && (i < (alpha->numStages-1))) ? GX_TEVREG0 : GX_TEVPREV;
					SetAlphaTEV(currStageA, TEVArgs[alpha->stage[i].op[j+2].param1].TEV_alphaIn, GX_CA_ZERO, GX_CA_ZERO, GX_CA_APREV,
						GX_TEV_ADD, TEVArgs[alpha->stage[i].op[j+2].param1].TEV_TexCoordMap, TEVArgs[alpha->stage[i].op[j+2].param1].TEV_inpType, AoutReg);
					if (AoutReg == GX_TEVREG0) TEVcombiner->TEVstage[currStageA].alphaClamp = GX_ENABLE; //Clamp COMBINED!
					currStageA++;
					j+=2;
				}

/*				j++;

				envCombiner->usesT0 |= alpha->stage[i].op[j].param1 == TEXEL0_ALPHA;
				envCombiner->usesT1 |= alpha->stage[i].op[j].param1 == TEXEL1_ALPHA;

				envCombiner->alpha[curUnit].combine = GL_MODULATE;
				SetAlphaCombinerArg( curUnit, arg1, alpha->stage[i].op[j].param1 );

				curUnit++;
				j++;

				envCombiner->usesT0 |= alpha->stage[i].op[j].param1 == TEXEL0_ALPHA;
				envCombiner->usesT1 |= alpha->stage[i].op[j].param1 == TEXEL1_ALPHA;

				envCombiner->alpha[curUnit].combine = GL_SUBTRACT_ARB;
				SetAlphaCombinerArg( curUnit, arg0, alpha->stage[i].op[j].param1 );

				curUnit++;
*/
			}
			else
			{
				TEVcombiner->usesT0 |= alpha->stage[i].op[j].param1 == TEXEL0_ALPHA;
				TEVcombiner->usesT1 |= alpha->stage[i].op[j].param1 == TEXEL1_ALPHA;

				switch (alpha->stage[i].op[j].op)
				{
					case LOAD:
/*						if (!(OGL.ARB_texture_env_crossbar || OGL.NV_texture_env_combine4) &&
							(alpha->stage[i].op[j].param1 == TEXEL1_ALPHA) && (curUnit == 0))
							curUnit++;

						envCombiner->alpha[curUnit].combine = GL_REPLACE;

						SetAlphaCombinerArg( curUnit, arg0, alpha->stage[i].op[j].param1 );
*/
						//TEVstage[currStageA++] -> ( d(param1) )
						AoutReg = ( (j == (alpha->stage[i].numOps - 1)) && (i < (alpha->numStages-1))) ? GX_TEVREG0 : GX_TEVPREV;
						SetAlphaTEV(currStageA, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, TEVArgs[alpha->stage[i].op[j].param1].TEV_alphaIn, 
							GX_TEV_ADD, TEVArgs[alpha->stage[i].op[j].param1].TEV_TexCoordMap, 
							TEVArgs[alpha->stage[i].op[j].param1].TEV_inpType, AoutReg);
						if (AoutReg == GX_TEVREG0) TEVcombiner->TEVstage[currStageA].alphaClamp = GX_ENABLE; //Clamp COMBINED!
						//SetAlphaTEV(tevstage, alphaA, alphaB, alphaC, alphaD, TEVop, alphaTEX, alphaCONST, AoutReg)
						currStageA++;
						break;
					case SUB:
/*						if (!OGL.ARB_texture_env_combine)
							break;

						if (!(OGL.ARB_texture_env_crossbar || OGL.NV_texture_env_combine4) &&
							(alpha->stage[i].op[j].param1 == TEXEL1_ALPHA) && (curUnit == 0))
							curUnit++;

						if ((j > 0) && (alpha->stage[i].op[j-1].op == LOAD) && (alpha->stage[i].op[j-1].param1 == ONE))
						{
							SetAlphaCombinerArg( curUnit, arg0, alpha->stage[i].op[j].param1 );
							envCombiner->alpha[curUnit].arg0.operand = GL_ONE_MINUS_SRC_ALPHA;
						}
						else if ((OGL.ATI_texture_env_combine3) && (curUnit > 0) && (envCombiner->alpha[curUnit - 1].combine == GL_MODULATE))
						{
							curUnit--;
							SetAlphaCombinerValues( curUnit, arg2, envCombiner->alpha[curUnit].arg1.source, envCombiner->alpha[curUnit].arg1.operand );
							envCombiner->alpha[curUnit].combine = GL_MODULATE_SUBTRACT_ATI;
							SetAlphaCombinerArg( curUnit, arg1, alpha->stage[i].op[j].param1 );
							curUnit++;
						}
						else
						{
							envCombiner->alpha[curUnit].combine = GL_SUBTRACT_ARB;
							SetAlphaCombinerArg( curUnit, arg1, alpha->stage[i].op[j].param1 );
							curUnit++;
						}
*/
						//TODO: Fix alpha input == ONE cases.
						if ( (alpha->stage[i].op[j-1].param1 == ONE) && (alpha->stage[i].op[j-1].op == LOAD) )
						{
							currStageA--;
							//TEVstage[currStageA++] -> ( ONE(param1) - c(param2) );
							AoutReg = ( (j == (alpha->stage[i].numOps - 1)) && (i < (alpha->numStages-1))) ? GX_TEVREG0 : GX_TEVPREV;
							SetAlphaTEV(currStageA, GX_CA_KONST, GX_CA_ZERO, TEVArgs[alpha->stage[i].op[j].param1].TEV_alphaIn, GX_CA_ZERO,
//							SetAlphaTEV(currStageA, GX_CA_ONE, GX_CA_ZERO, TEVArgs[alpha->stage[i].op[j].param1].TEV_alphaIn, GX_CA_ZERO,
								GX_TEV_ADD, TEVArgs[alpha->stage[i].op[j].param1].TEV_TexCoordMap, TEVArgs[alpha->stage[i].op[j].param1].TEV_inpType, AoutReg);
							if (AoutReg == GX_TEVREG0) TEVcombiner->TEVstage[currStageA].alphaClamp = GX_ENABLE; //Clamp COMBINED!
							currStageA++;
						}
						else
						{
							//TEVstage[currStageA++] -> ( d(PREV) - a(param1) );
							AoutReg = ( (j == (alpha->stage[i].numOps - 1)) && (i < (alpha->numStages-1))) ? GX_TEVREG0 : GX_TEVPREV;
							SetAlphaTEV(currStageA, TEVArgs[alpha->stage[i].op[j].param1].TEV_alphaIn, GX_CA_ZERO, GX_CA_ZERO, GX_CA_APREV,
								GX_TEV_SUB, TEVArgs[alpha->stage[i].op[j].param1].TEV_TexCoordMap, TEVArgs[alpha->stage[i].op[j].param1].TEV_inpType, AoutReg);
							if (AoutReg == GX_TEVREG0) TEVcombiner->TEVstage[currStageA].alphaClamp = GX_ENABLE; //Clamp COMBINED!
							currStageA++;
						}

						break;
					case MUL:
/*						if (!(OGL.ARB_texture_env_crossbar || OGL.NV_texture_env_combine4) &&
							(alpha->stage[i].op[j].param1 == TEXEL1_ALPHA) && (curUnit == 0))
							curUnit++;

						envCombiner->alpha[curUnit].combine = GL_MODULATE;

						SetAlphaCombinerArg( curUnit, arg1, alpha->stage[i].op[j].param1 );
						curUnit++;
*/

						//TEVstage[currStageA++] -> ( b(PREV) * c(param1) );
						AoutReg = ( (j == (alpha->stage[i].numOps - 1)) && (i < (alpha->numStages-1))) ? GX_TEVREG0 : GX_TEVPREV;
						SetAlphaTEV(currStageA, GX_CA_ZERO, GX_CA_APREV, TEVArgs[alpha->stage[i].op[j].param1].TEV_alphaIn, GX_CA_ZERO,
							GX_TEV_ADD, TEVArgs[alpha->stage[i].op[j].param1].TEV_TexCoordMap, TEVArgs[alpha->stage[i].op[j].param1].TEV_inpType, AoutReg);
						if (currStageA) TEVcombiner->TEVstage[currStageA-1].alphaClamp = GX_ENABLE; //TEVPREV used in Mult!
						if (AoutReg == GX_TEVREG0) TEVcombiner->TEVstage[currStageA].alphaClamp = GX_ENABLE; //Clamp COMBINED!
						currStageA++;
						break;
					case ADD:
/*						if (!(OGL.ARB_texture_env_crossbar || OGL.NV_texture_env_combine4) &&
							(alpha->stage[i].op[j].param1 == TEXEL1_ALPHA) && (curUnit == 0))
							curUnit++;

						if ((OGL.ATI_texture_env_combine3) && (curUnit > 0) && (envCombiner->alpha[curUnit - 1].combine == GL_MODULATE))
						{
							curUnit--;
							SetAlphaCombinerValues( curUnit, arg2, envCombiner->alpha[curUnit].arg1.source, envCombiner->alpha[curUnit].arg1.operand );
							envCombiner->alpha[curUnit].combine = GL_MODULATE_ADD_ATI;
							SetAlphaCombinerArg( curUnit, arg1, alpha->stage[i].op[j].param1 );
						}
						else
						{
							envCombiner->alpha[curUnit].combine = GL_ADD;
							SetAlphaCombinerArg( curUnit, arg1, alpha->stage[i].op[j].param1 );
						}
						curUnit++;
*/
						//TEVstage[currStageA++] -> ( d(PREV) + a(param1) );
						AoutReg = ( (j == (alpha->stage[i].numOps - 1)) && (i < (alpha->numStages-1))) ? GX_TEVREG0 : GX_TEVPREV;
						SetAlphaTEV(currStageA, TEVArgs[alpha->stage[i].op[j].param1].TEV_alphaIn, GX_CA_ZERO, GX_CA_ZERO, GX_CA_APREV,
							GX_TEV_ADD, TEVArgs[alpha->stage[i].op[j].param1].TEV_TexCoordMap, TEVArgs[alpha->stage[i].op[j].param1].TEV_inpType, AoutReg);
						if (AoutReg == GX_TEVREG0) TEVcombiner->TEVstage[currStageA].alphaClamp = GX_ENABLE; //Clamp COMBINED!
						currStageA++;
						break;
					case INTER:
						TEVcombiner->usesT0 |= (alpha->stage[i].op[j].param2 == TEXEL0_ALPHA) || (alpha->stage[i].op[j].param3 == TEXEL0_ALPHA);
						TEVcombiner->usesT1 |= (alpha->stage[i].op[j].param2 == TEXEL1_ALPHA) || (alpha->stage[i].op[j].param3 == TEXEL1_ALPHA);

/*						envCombiner->alpha[curUnit].combine = GL_INTERPOLATE_ARB;

						SetAlphaCombinerArg( curUnit, arg0, alpha->stage[i].op[j].param1 );
						SetAlphaCombinerArg( curUnit, arg1, alpha->stage[i].op[j].param2 );
						SetAlphaCombinerArg( curUnit, arg2, alpha->stage[i].op[j].param3 );

						curUnit++;*/
/*						(TEVArgs[alpha->stage[i].op[j].param1].TEV_inpType <= TEV_MAX_CONST) &&
						(TEVArgs[alpha->stage[i].op[j].param2].TEV_inpType <= TEV_MAX_CONST) &&
						(TEVArgs[alpha->stage[i].op[j].param1].TEV_inpType != TEVArgs[alpha->stage[i].op[j].param2].TEV_inpType)
						TEVArgs[alpha->stage[i].op[j].param3].TEV_inpType <= TEV_MAX_CONST*/
						if (	!(((TEVArgs[alpha->stage[i].op[j].param1].TEV_TexCoordMap == GX_TEXMAP0) ||
								(TEVArgs[alpha->stage[i].op[j].param2].TEV_TexCoordMap == GX_TEXMAP0) ||
								(TEVArgs[alpha->stage[i].op[j].param3].TEV_TexCoordMap == GX_TEXMAP0)) && 
								((TEVArgs[alpha->stage[i].op[j].param1].TEV_TexCoordMap == GX_TEXMAP1) ||
								(TEVArgs[alpha->stage[i].op[j].param2].TEV_TexCoordMap == GX_TEXMAP1) ||
								(TEVArgs[alpha->stage[i].op[j].param3].TEV_TexCoordMap == GX_TEXMAP1))) && //!(TEX0 && TEX1)
								!(((TEVArgs[alpha->stage[i].op[j].param1].TEV_inpType <= TEV_MAX_CONST) && //!(>1 unique constants)
								(TEVArgs[alpha->stage[i].op[j].param2].TEV_inpType <= TEV_MAX_CONST) &&
								(TEVArgs[alpha->stage[i].op[j].param1].TEV_inpType != TEVArgs[alpha->stage[i].op[j].param2].TEV_inpType)) ||
								((TEVArgs[alpha->stage[i].op[j].param1].TEV_inpType <= TEV_MAX_CONST) &&
								(TEVArgs[alpha->stage[i].op[j].param3].TEV_inpType <= TEV_MAX_CONST) &&
								(TEVArgs[alpha->stage[i].op[j].param1].TEV_inpType != TEVArgs[alpha->stage[i].op[j].param3].TEV_inpType)) ||
								((TEVArgs[alpha->stage[i].op[j].param2].TEV_inpType <= TEV_MAX_CONST) &&
								(TEVArgs[alpha->stage[i].op[j].param3].TEV_inpType <= TEV_MAX_CONST) &&
								(TEVArgs[alpha->stage[i].op[j].param2].TEV_inpType != TEVArgs[alpha->stage[i].op[j].param3].TEV_inpType))))
						{ //This can be done in 1 TEV Stage
							//TEVstage[currStageA++] -> a(param2)*(1 - c(param3)) + b(param1)*c(param3);
							alphaTEX = GX_TEXMAP_NULL;
							alphaTEX = ((TEVArgs[alpha->stage[i].op[j].param1].TEV_TexCoordMap == GX_TEXMAP0) ||
								(TEVArgs[alpha->stage[i].op[j].param2].TEV_TexCoordMap == GX_TEXMAP0) ||
								(TEVArgs[alpha->stage[i].op[j].param3].TEV_TexCoordMap == GX_TEXMAP0)) ? GX_TEXMAP0 : alphaTEX;
							alphaTEX = ((TEVArgs[alpha->stage[i].op[j].param1].TEV_TexCoordMap == GX_TEXMAP1) ||
								(TEVArgs[alpha->stage[i].op[j].param2].TEV_TexCoordMap == GX_TEXMAP1) ||
								(TEVArgs[alpha->stage[i].op[j].param3].TEV_TexCoordMap == GX_TEXMAP1)) ? GX_TEXMAP1 : alphaTEX;
							alphaCONST = TEV_MAX_CONST + 1;
							alphaCONST = (TEVArgs[alpha->stage[i].op[j].param1].TEV_inpType <= TEV_MAX_CONST) ?
								TEVArgs[alpha->stage[i].op[j].param1].TEV_inpType : alphaCONST;
							alphaCONST = (TEVArgs[alpha->stage[i].op[j].param2].TEV_inpType <= TEV_MAX_CONST) ?
								TEVArgs[alpha->stage[i].op[j].param1].TEV_inpType : alphaCONST;
							alphaCONST = (TEVArgs[alpha->stage[i].op[j].param3].TEV_inpType <= TEV_MAX_CONST) ?
								TEVArgs[alpha->stage[i].op[j].param1].TEV_inpType : alphaCONST;
							AoutReg = ( (j == (alpha->stage[i].numOps - 1)) && (i < (alpha->numStages-1))) ? GX_TEVREG0 : GX_TEVPREV;
							SetAlphaTEV(currStageA, TEVArgs[alpha->stage[i].op[j].param2].TEV_alphaIn, 
								TEVArgs[alpha->stage[i].op[j].param1].TEV_alphaIn, TEVArgs[alpha->stage[i].op[j].param3].TEV_alphaIn, GX_CA_ZERO,
								GX_TEV_ADD, alphaTEX, alphaCONST, AoutReg);
							if (AoutReg == GX_TEVREG0) TEVcombiner->TEVstage[currStageA].alphaClamp = GX_ENABLE; //Clamp COMBINED!
							currStageA++;
						}
						else
						{
							if ( !(((TEVArgs[alpha->stage[i].op[j].param1].TEV_TexCoordMap == GX_TEXMAP0) ||
								(TEVArgs[alpha->stage[i].op[j].param2].TEV_TexCoordMap == GX_TEXMAP0)) && 
								((TEVArgs[alpha->stage[i].op[j].param1].TEV_TexCoordMap == GX_TEXMAP1) ||
								(TEVArgs[alpha->stage[i].op[j].param2].TEV_TexCoordMap == GX_TEXMAP1))) && //!(TEX0 && TEX1)
								!(((TEVArgs[alpha->stage[i].op[j].param1].TEV_inpType <= TEV_MAX_CONST) && //!(>1 unique constants)
								(TEVArgs[alpha->stage[i].op[j].param2].TEV_inpType <= TEV_MAX_CONST) &&
								(TEVArgs[alpha->stage[i].op[j].param1].TEV_inpType != TEVArgs[alpha->stage[i].op[j].param2].TEV_inpType))))
							{
								//TEVstage[currStageA++] -> d(param1) - a(param2);
								alphaTEX = (TEVArgs[alpha->stage[i].op[j].param1].TEV_TexCoordMap == GX_TEXMAP_NULL) ? 
									TEVArgs[alpha->stage[i].op[j].param2].TEV_TexCoordMap : TEVArgs[alpha->stage[i].op[j].param1].TEV_TexCoordMap;
								alphaCONST = (TEVArgs[alpha->stage[i].op[j].param1].TEV_inpType <= TEV_MAX_CONST) ?
									TEVArgs[alpha->stage[i].op[j].param1].TEV_inpType : TEVArgs[alpha->stage[i].op[j].param2].TEV_inpType;
								SetAlphaTEV(currStageA, TEVArgs[alpha->stage[i].op[j].param2].TEV_alphaIn, GX_CA_ZERO, GX_CA_ZERO, 
									TEVArgs[alpha->stage[i].op[j].param2].TEV_alphaIn, GX_TEV_SUB, alphaTEX, alphaCONST, GX_TEVPREV);
								//SetAlphaTEV(tevstage, alphaA, alphaB, alphaC, alphaD, TEVop, alphaTEX, alphaCONST, AoutReg)
								currStageA++;
							}
							else
							{
								//TEVstage[currStageA++] -> d(param1);
								SetAlphaTEV(currStageA, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, TEVArgs[alpha->stage[i].op[j].param1].TEV_alphaIn, 
									GX_TEV_ADD, TEVArgs[alpha->stage[i].op[j].param1].TEV_TexCoordMap, 
									TEVArgs[alpha->stage[i].op[j].param1].TEV_inpType, GX_TEVPREV);
								//SetAlphaTEV(tevstage, alphaA, alphaB, alphaC, alphaD, TEVop, alphaTEX, alphaCONST, AoutReg)
								currStageA++;
								//TEVstage[currStageA++] -> d(PREV) - a(param2);
								SetAlphaTEV(currStageA, TEVArgs[alpha->stage[i].op[j].param2].TEV_alphaIn, GX_CA_ZERO, GX_CA_ZERO, GX_CA_APREV,
									GX_TEV_SUB, TEVArgs[alpha->stage[i].op[j].param2].TEV_TexCoordMap, TEVArgs[alpha->stage[i].op[j].param2].TEV_inpType, GX_TEVPREV);
								currStageA++;
							}
							//TEVstage[currStageA++] -> b(PREV)*c(param3);
							SetAlphaTEV(currStageA, GX_CA_ZERO, GX_CA_APREV, TEVArgs[alpha->stage[i].op[j].param3].TEV_alphaIn, GX_CA_ZERO,
								GX_TEV_ADD, TEVArgs[alpha->stage[i].op[j].param3].TEV_TexCoordMap, TEVArgs[alpha->stage[i].op[j].param3].TEV_inpType, GX_TEVPREV);
							if (currStageA) TEVcombiner->TEVstage[currStageA-1].alphaClamp = GX_ENABLE; //TEVPREV used in Mult!
							currStageA++;
							//TEVstage[currStageA++] -> d(PREV) + a(param2);
							AoutReg = ( (j == (alpha->stage[i].numOps - 1)) && (i < (alpha->numStages-1))) ? GX_TEVREG0 : GX_TEVPREV;
							SetAlphaTEV(currStageA, TEVArgs[alpha->stage[i].op[j].param2].TEV_alphaIn, GX_CA_ZERO, GX_CA_ZERO, GX_CA_APREV,
								GX_TEV_ADD, TEVArgs[alpha->stage[i].op[j].param2].TEV_TexCoordMap, TEVArgs[alpha->stage[i].op[j].param2].TEV_inpType, AoutReg);
							if (AoutReg == GX_TEVREG0) TEVcombiner->TEVstage[currStageA].alphaClamp = GX_ENABLE; //Clamp COMBINED!
							currStageA++;
						}
						break;
				}
			}
		}
//		combinedUnit = max( curUnit - 1, 0 );
	}

//	envCombiner->usedUnits = max( curUnit, 1 );

//	curUnit = 0;
	for (int i = 0; i < color->numStages; i++)
	{
		for (int j = 0; j < color->stage[i].numOps; j++)
		{
			float sb = 0.0f;

			if (color->stage[i].op[j].param1 == PRIMITIVE)
				sb = (gDP.primColor.r + gDP.primColor.b + gDP.primColor.g) / 3.0f;
			else if (color->stage[i].op[j].param1 == ENVIRONMENT)
				sb = (gDP.envColor.r + gDP.envColor.b + gDP.envColor.g) / 3.0f;

			// This helps with problems caused by not using signed values between texture units
			//Instead of (A - B)*C + D do D - (A - B)*C
			if (((color->stage[i].numOps - j) >= 3) &&
				(color->stage[i].op[j].op == SUB) &&
				(color->stage[i].op[j+1].op == MUL) &&
				(color->stage[i].op[j+2].op == ADD) &&
				(sb > 0.5f))
			{
				TEVcombiner->usesT0 |= ((color->stage[i].op[j].param1 == TEXEL0)   || (color->stage[i].op[j].param1 == TEXEL0_ALPHA));
				TEVcombiner->usesT1 |= ((color->stage[i].op[j].param1 == TEXEL1)   || (color->stage[i].op[j].param1 == TEXEL1_ALPHA));
				TEVcombiner->usesT0 |= ((color->stage[i].op[j+1].param1 == TEXEL0) || (color->stage[i].op[j+1].param1 == TEXEL0_ALPHA));
				TEVcombiner->usesT1 |= ((color->stage[i].op[j+1].param1 == TEXEL1) || (color->stage[i].op[j+1].param1 == TEXEL1_ALPHA));
				TEVcombiner->usesT0 |= ((color->stage[i].op[j+2].param1 == TEXEL0) || (color->stage[i].op[j+2].param1 == TEXEL0_ALPHA));
				TEVcombiner->usesT1 |= ((color->stage[i].op[j+2].param1 == TEXEL1) || (color->stage[i].op[j+2].param1 == TEXEL1_ALPHA));

				//TEVstage[currStageC++] -> ( d[PREV] - a(param1) )
				while((TEVArgs[color->stage[i].op[j].param1].TEV_inpType == TEV_TEX) &&
					(TEVcombiner->TEVstage[currStageC].texmap != GX_TEXMAP_NULL) &&
					(TEVcombiner->TEVstage[currStageC].texmap != TEVArgs[color->stage[i].op[j].param1].TEV_TexCoordMap))
				{
					SetColorTEVskip(currStageC);
					currStageC++;
				}
				SetColorTEV(currStageC, TEVArgs[color->stage[i].op[j].param1].TEV_colorIn, GX_CC_ZERO, GX_CC_ZERO, GX_CC_CPREV, 
					GX_TEV_SUB, TEVArgs[color->stage[i].op[j].param1].TEV_TexCoordMap, 
					TEVArgs[color->stage[i].op[j].param1].TEV_inpType, TEVArgs[color->stage[i].op[j].param1].KonstAAA, GX_TEVPREV);
				//SetColorTEV(tevstage, colorA, colorB, colorC, colorD, TEVop, colorTEX, colorCONST, KonstisAAA, CoutReg)
				currStageC++;
				//TEVstage[currStageC++] -> *-*( a[PREV] ) * c(param2);
				while((TEVArgs[color->stage[i].op[j+1].param1].TEV_inpType == TEV_TEX) &&
					(TEVcombiner->TEVstage[currStageC].texmap != GX_TEXMAP_NULL) &&
					(TEVcombiner->TEVstage[currStageC].texmap != TEVArgs[color->stage[i].op[j+1].param1].TEV_TexCoordMap))
				{
					SetColorTEVskip(currStageC);
					currStageC++;
				}
				SetColorTEV(currStageC, GX_CC_ZERO, GX_CC_CPREV, TEVArgs[color->stage[i].op[j+1].param1].TEV_colorIn, GX_CC_ZERO,  
					GX_TEV_SUB, TEVArgs[color->stage[i].op[j+1].param1].TEV_TexCoordMap, 
					TEVArgs[color->stage[i].op[j+1].param1].TEV_inpType, TEVArgs[color->stage[i].op[j+1].param1].KonstAAA, GX_TEVPREV);
				if (currStageC) TEVcombiner->TEVstage[currStageC-1].colorClamp = GX_ENABLE; //TEVPREV used in Mult!
				//SetColorTEV(tevstage, colorA, colorB, colorC, colorD, TEVop, colorTEX, colorCONST, KonstisAAA, CoutReg)
				currStageC++;
				//TEVstage[currStageC++] -> d(param3) + a[PREV];
				while((TEVArgs[color->stage[i].op[j+2].param1].TEV_inpType == TEV_TEX) &&
					(TEVcombiner->TEVstage[currStageC].texmap != GX_TEXMAP_NULL) &&
					(TEVcombiner->TEVstage[currStageC].texmap != TEVArgs[color->stage[i].op[j+2].param1].TEV_TexCoordMap))
				{
					SetColorTEVskip(currStageC);
					currStageC++;
				}
				CoutReg = ( ((j+2) == (color->stage[i].numOps - 1)) && (i < (color->numStages-1))) ? GX_TEVREG0 : GX_TEVPREV;
				SetColorTEV(currStageC, TEVArgs[color->stage[i].op[j+2].param1].TEV_colorIn, GX_CC_ZERO, GX_CC_ZERO, GX_CC_CPREV,  
					GX_TEV_ADD, TEVArgs[color->stage[i].op[j+2].param1].TEV_TexCoordMap, 
					TEVArgs[color->stage[i].op[j+2].param1].TEV_inpType, TEVArgs[color->stage[i].op[j+2].param1].KonstAAA, CoutReg);
				if (CoutReg == GX_TEVREG0) TEVcombiner->TEVstage[currStageC].colorClamp = GX_ENABLE; //Clamp COMBINED!
				//SetColorTEV(tevstage, colorA, colorB, colorC, colorD, TEVop, colorTEX, colorCONST, KonstisAAA, CoutReg)
				currStageC++;
				j+=2;

/*				envCombiner->color[curUnit].combine = GL_SUBTRACT_ARB;
				SetColorCombinerValues( curUnit, arg1, envCombiner->color[curUnit].arg0.source, envCombiner->color[curUnit].arg0.operand );
				SetColorCombinerArg( curUnit, arg0, color->stage[i].op[j].param1 );

				curUnit++;
				j++;

				envCombiner->usesT0 |= ((color->stage[i].op[j].param1 == TEXEL0) || (color->stage[i].op[j].param1 == TEXEL0_ALPHA));
				envCombiner->usesT1 |= ((color->stage[i].op[j].param1 == TEXEL1) || (color->stage[i].op[j].param1 == TEXEL1_ALPHA));

				envCombiner->color[curUnit].combine = GL_MODULATE;
				SetColorCombinerArg( curUnit, arg1, color->stage[i].op[j].param1 );

				curUnit++;
				j++;

				envCombiner->usesT0 |= ((color->stage[i].op[j].param1 == TEXEL0) || (color->stage[i].op[j].param1 == TEXEL0_ALPHA));
				envCombiner->usesT1 |= ((color->stage[i].op[j].param1 == TEXEL1) || (color->stage[i].op[j].param1 == TEXEL1_ALPHA));

				envCombiner->color[curUnit].combine = GL_SUBTRACT_ARB;
				SetColorCombinerArg( curUnit, arg0, color->stage[i].op[j].param1 );

				curUnit++;*/
			}
			else
			{
				TEVcombiner->usesT0 |= ((color->stage[i].op[j].param1 == TEXEL0) || (color->stage[i].op[j].param1 == TEXEL0_ALPHA));
				TEVcombiner->usesT1 |= ((color->stage[i].op[j].param1 == TEXEL1) || (color->stage[i].op[j].param1 == TEXEL1_ALPHA));

				switch (color->stage[i].op[j].op)
				{
					case LOAD:
/*						if (!(OGL.ARB_texture_env_crossbar || OGL.NV_texture_env_combine4) &&
							((color->stage[i].op[j].param1 == TEXEL1) || (color->stage[i].op[j].param1 == TEXEL1_ALPHA)) && (curUnit == 0))
							curUnit++;

						envCombiner->color[curUnit].combine = GL_REPLACE;

						SetColorCombinerArg( curUnit, arg0, color->stage[i].op[j].param1 );
*/
						//TEVstage[currStageC++] -> ( d(param1) )
						while((TEVArgs[color->stage[i].op[j].param1].TEV_inpType == TEV_TEX) &&
							(TEVcombiner->TEVstage[currStageC].texmap != GX_TEXMAP_NULL) &&
							(TEVcombiner->TEVstage[currStageC].texmap != TEVArgs[color->stage[i].op[j].param1].TEV_TexCoordMap))
						{
							SetColorTEVskip(currStageC);
							currStageC++;
						}
						CoutReg = ( (j == (color->stage[i].numOps - 1)) && (i < (color->numStages-1))) ? GX_TEVREG0 : GX_TEVPREV;
						SetColorTEV(currStageC, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, TEVArgs[color->stage[i].op[j].param1].TEV_colorIn,  
							GX_TEV_ADD, TEVArgs[color->stage[i].op[j].param1].TEV_TexCoordMap, 
							TEVArgs[color->stage[i].op[j].param1].TEV_inpType, TEVArgs[color->stage[i].op[j].param1].KonstAAA, CoutReg);
						//SetColorTEV(tevstage, colorA, colorB, colorC, colorD, TEVop, colorTEX, colorCONST, KonstisAAA, CoutReg)
						if (CoutReg == GX_TEVREG0) TEVcombiner->TEVstage[currStageC].colorClamp = GX_ENABLE; //Clamp COMBINED!
						currStageC++;
						break;
					case SUB:
/*						if (!OGL.ARB_texture_env_combine)
							break;

						if (!(OGL.ARB_texture_env_crossbar || OGL.NV_texture_env_combine4) &&
							((color->stage[i].op[j].param1 == TEXEL1) || (color->stage[i].op[j].param1 == TEXEL1_ALPHA)) && (curUnit == 0))
							curUnit++;

						if ((j > 0) && (color->stage[i].op[j-1].op == LOAD) && (color->stage[i].op[j-1].param1 == ONE))
						{
							SetColorCombinerArg( curUnit, arg0, color->stage[i].op[j].param1 );
							envCombiner->color[curUnit].arg0.operand = GL_ONE_MINUS_SRC_COLOR;
						}
						else if ((OGL.ATI_texture_env_combine3) && (curUnit > 0) && (envCombiner->color[curUnit - 1].combine == GL_MODULATE))
						{
							curUnit--;
							SetColorCombinerValues( curUnit, arg2, envCombiner->color[curUnit].arg1.source, envCombiner->color[curUnit].arg1.operand );
							envCombiner->color[curUnit].combine = GL_MODULATE_SUBTRACT_ATI;
							SetColorCombinerArg( curUnit, arg1, color->stage[i].op[j].param1 );
							curUnit++;
						}
						else
						{
							envCombiner->color[curUnit].combine = GL_SUBTRACT_ARB;
							SetColorCombinerArg( curUnit, arg1, color->stage[i].op[j].param1 );
							curUnit++;
						}
*/
						if ( (color->stage[i].op[j-1].param1 == ONE) && (color->stage[i].op[j-1].op == LOAD) )
						{
							currStageC--;
							//TEVstage[currStageC++] -> ( ONE(param1) - c(param2) );
							while((TEVArgs[color->stage[i].op[j].param1].TEV_inpType == TEV_TEX) &&
								(TEVcombiner->TEVstage[currStageC].texmap != GX_TEXMAP_NULL) &&
								(TEVcombiner->TEVstage[currStageC].texmap != TEVArgs[color->stage[i].op[j].param1].TEV_TexCoordMap))
							{
								SetColorTEVskip(currStageC);
								currStageC++;
							}
							CoutReg = ( (j == (color->stage[i].numOps - 1)) && (i < (color->numStages-1))) ? GX_TEVREG0 : GX_TEVPREV;
							SetColorTEV(currStageC, GX_CC_ONE, GX_CC_ZERO, TEVArgs[color->stage[i].op[j].param1].TEV_colorIn, GX_CC_ZERO,
								GX_TEV_ADD, TEVArgs[color->stage[i].op[j].param1].TEV_TexCoordMap, 
								TEVArgs[color->stage[i].op[j].param1].TEV_inpType, TEVArgs[color->stage[i].op[j].param1].KonstAAA, CoutReg);
							if (CoutReg == GX_TEVREG0) TEVcombiner->TEVstage[currStageC].colorClamp = GX_ENABLE; //Clamp COMBINED!
							currStageC++;
						}
						else
						{
							//TEVstage[currStageC++] -> ( d(PREV) - a(param1) );
							while((TEVArgs[color->stage[i].op[j].param1].TEV_inpType == TEV_TEX) &&
								(TEVcombiner->TEVstage[currStageC].texmap != GX_TEXMAP_NULL) &&
								(TEVcombiner->TEVstage[currStageC].texmap != TEVArgs[color->stage[i].op[j].param1].TEV_TexCoordMap))
							{
								SetColorTEVskip(currStageC);
								currStageC++;
							}
							CoutReg = ( (j == (color->stage[i].numOps - 1)) && (i < (color->numStages-1))) ? GX_TEVREG0 : GX_TEVPREV;
							SetColorTEV(currStageC, TEVArgs[color->stage[i].op[j].param1].TEV_colorIn, GX_CC_ZERO, GX_CC_ZERO, GX_CC_CPREV,
								GX_TEV_SUB, TEVArgs[color->stage[i].op[j].param1].TEV_TexCoordMap, 
								TEVArgs[color->stage[i].op[j].param1].TEV_inpType, TEVArgs[color->stage[i].op[j].param1].KonstAAA, CoutReg);
							if (CoutReg == GX_TEVREG0) TEVcombiner->TEVstage[currStageC].colorClamp = GX_ENABLE; //Clamp COMBINED!
							currStageC++;
						}
						break;
					case MUL:
/*						if (!(OGL.ARB_texture_env_crossbar || OGL.NV_texture_env_combine4) &&
							((color->stage[i].op[j].param1 == TEXEL1) || (color->stage[i].op[j].param1 == TEXEL1_ALPHA)) && (curUnit == 0))
							curUnit++;

						envCombiner->color[curUnit].combine = GL_MODULATE;

						SetColorCombinerArg( curUnit, arg1, color->stage[i].op[j].param1 );
						curUnit++;
*/

						//TEVstage[currStageC++] -> ( b(PREV) * c(param1) );
						while((TEVArgs[color->stage[i].op[j].param1].TEV_inpType == TEV_TEX) &&
							(TEVcombiner->TEVstage[currStageC].texmap != GX_TEXMAP_NULL) &&
							(TEVcombiner->TEVstage[currStageC].texmap != TEVArgs[color->stage[i].op[j].param1].TEV_TexCoordMap))
						{
							SetColorTEVskip(currStageC);
							currStageC++;
						}
						CoutReg = ( (j == (color->stage[i].numOps - 1)) && (i < (color->numStages-1))) ? GX_TEVREG0 : GX_TEVPREV;
						SetColorTEV(currStageC, GX_CC_ZERO, GX_CC_CPREV, TEVArgs[color->stage[i].op[j].param1].TEV_colorIn, GX_CC_ZERO,
							GX_TEV_ADD, TEVArgs[color->stage[i].op[j].param1].TEV_TexCoordMap, 
							TEVArgs[color->stage[i].op[j].param1].TEV_inpType, TEVArgs[color->stage[i].op[j].param1].KonstAAA, CoutReg);
						if (currStageC) TEVcombiner->TEVstage[currStageC-1].colorClamp = GX_ENABLE; //TEVPREV used in Mult!
						if (CoutReg == GX_TEVREG0) TEVcombiner->TEVstage[currStageC].colorClamp = GX_ENABLE; //Clamp COMBINED!
						currStageC++;
						break;
					case ADD:
/*						if (!(OGL.ARB_texture_env_crossbar || OGL.NV_texture_env_combine4) &&
							((color->stage[i].op[j].param1 == TEXEL1) || (color->stage[i].op[j].param1 == TEXEL1_ALPHA)) && (curUnit == 0))
							curUnit++;

						// ATI_texture_env_combine3 adds GL_MODULATE_ADD_ATI; saves texture units
						if ((OGL.ATI_texture_env_combine3) && (curUnit > 0) && (envCombiner->color[curUnit - 1].combine == GL_MODULATE))
						{
							curUnit--;
							SetColorCombinerValues( curUnit, arg2, envCombiner->color[curUnit].arg1.source, envCombiner->color[curUnit].arg1.operand );
							envCombiner->color[curUnit].combine = GL_MODULATE_ADD_ATI;
							SetColorCombinerArg( curUnit, arg1, color->stage[i].op[j].param1 );
						}
						else
						{
							envCombiner->color[curUnit].combine = GL_ADD;
							SetColorCombinerArg( curUnit, arg1, color->stage[i].op[j].param1 );
						}
						curUnit++;
*/
						//TEVstage[currStageC++] -> ( d(PREV) + a(param1) );
						while((TEVArgs[color->stage[i].op[j].param1].TEV_inpType == TEV_TEX) &&
							(TEVcombiner->TEVstage[currStageC].texmap != GX_TEXMAP_NULL) &&
							(TEVcombiner->TEVstage[currStageC].texmap != TEVArgs[color->stage[i].op[j].param1].TEV_TexCoordMap))
						{
							SetColorTEVskip(currStageC);
							currStageC++;
						}
						CoutReg = ( (j == (color->stage[i].numOps - 1)) && (i < (color->numStages-1))) ? GX_TEVREG0 : GX_TEVPREV;
						SetColorTEV(currStageC, TEVArgs[color->stage[i].op[j].param1].TEV_colorIn, GX_CC_ZERO, GX_CC_ZERO, GX_CC_CPREV,
							GX_TEV_ADD, TEVArgs[color->stage[i].op[j].param1].TEV_TexCoordMap, 
							TEVArgs[color->stage[i].op[j].param1].TEV_inpType, TEVArgs[color->stage[i].op[j].param1].KonstAAA, CoutReg);
						if (CoutReg == GX_TEVREG0) TEVcombiner->TEVstage[currStageC].colorClamp = GX_ENABLE; //Clamp COMBINED!
						currStageC++;
						break;
					case INTER:
						TEVcombiner->usesT0 |= (color->stage[i].op[j].param2 == TEXEL0) || (color->stage[i].op[j].param2 == TEXEL0_ALPHA) || (color->stage[i].op[j].param3 == TEXEL0) || (color->stage[i].op[j].param3 == TEXEL0_ALPHA);
						TEVcombiner->usesT1 |= (color->stage[i].op[j].param2 == TEXEL1) || (color->stage[i].op[j].param2 == TEXEL1_ALPHA) || (color->stage[i].op[j].param3 == TEXEL1) || (color->stage[i].op[j].param3 == TEXEL1_ALPHA);

/*						if (!(OGL.ARB_texture_env_crossbar || OGL.NV_texture_env_combine4) &&
							((color->stage[i].op[j].param1 == TEXEL1) || (color->stage[i].op[j].param2 == TEXEL1) || (color->stage[i].op[j].param3 == TEXEL1) || (color->stage[i].op[j].param3 == TEXEL1_ALPHA)) && (curUnit == 0))
						{
							if (color->stage[i].op[j].param1 == TEXEL0)
							{
								envCombiner->color[curUnit].combine = GL_REPLACE;
								SetColorCombinerArg( curUnit, arg0, color->stage[i].op[j].param1 );
								color->stage[i].op[j].param1 = COMBINED;
							}
							if (color->stage[i].op[j].param2 == TEXEL0)
							{
								envCombiner->color[curUnit].combine = GL_REPLACE;
								SetColorCombinerArg( curUnit, arg0, color->stage[i].op[j].param2 )

								color->stage[i].op[j].param2 = COMBINED;
							}
							if (color->stage[i].op[j].param3 == TEXEL0)
							{
								envCombiner->color[curUnit].combine = GL_REPLACE;
								SetColorCombinerArg( curUnit, arg0, color->stage[i].op[j].param3 );
								color->stage[i].op[j].param3 = COMBINED;
							}
							if (color->stage[i].op[j].param3 == TEXEL0_ALPHA)
							{
								envCombiner->color[curUnit].combine = GL_REPLACE;
								SetColorCombinerArg( curUnit, arg0, color->stage[i].op[j].param3 );
								color->stage[i].op[j].param3 = COMBINED_ALPHA;
							}

							curUnit++;
						}

						envCombiner->color[curUnit].combine = GL_INTERPOLATE_ARB;

						SetColorCombinerArg( curUnit, arg0, color->stage[i].op[j].param1 );
						SetColorCombinerArg( curUnit, arg1, color->stage[i].op[j].param2 );
						SetColorCombinerArg( curUnit, arg2, color->stage[i].op[j].param3 );

						curUnit++;*/




						if (	!(((TEVArgs[color->stage[i].op[j].param1].TEV_TexCoordMap == GX_TEXMAP0) ||
								(TEVArgs[color->stage[i].op[j].param2].TEV_TexCoordMap == GX_TEXMAP0) ||
								(TEVArgs[color->stage[i].op[j].param3].TEV_TexCoordMap == GX_TEXMAP0)) && 
								((TEVArgs[color->stage[i].op[j].param1].TEV_TexCoordMap == GX_TEXMAP1) ||
								(TEVArgs[color->stage[i].op[j].param2].TEV_TexCoordMap == GX_TEXMAP1) ||
								(TEVArgs[color->stage[i].op[j].param3].TEV_TexCoordMap == GX_TEXMAP1))) && //!(TEX0 && TEX1)
								!(((TEVArgs[color->stage[i].op[j].param1].TEV_inpType <= TEV_MAX_CONST) && //!(>1 unique constants)
								(TEVArgs[color->stage[i].op[j].param2].TEV_inpType <= TEV_MAX_CONST) &&
								(TEVArgs[color->stage[i].op[j].param1].TEV_inpType != TEVArgs[color->stage[i].op[j].param2].TEV_inpType)) ||
								((TEVArgs[color->stage[i].op[j].param1].TEV_inpType <= TEV_MAX_CONST) &&
								(TEVArgs[color->stage[i].op[j].param3].TEV_inpType <= TEV_MAX_CONST) &&
								(TEVArgs[color->stage[i].op[j].param1].TEV_inpType != TEVArgs[color->stage[i].op[j].param3].TEV_inpType)) ||
								((TEVArgs[color->stage[i].op[j].param2].TEV_inpType <= TEV_MAX_CONST) &&
								(TEVArgs[color->stage[i].op[j].param3].TEV_inpType <= TEV_MAX_CONST) &&
								(TEVArgs[color->stage[i].op[j].param2].TEV_inpType != TEVArgs[color->stage[i].op[j].param3].TEV_inpType))))
						{ //This can be done in 1 TEV Stage
							//TEVstage[currStageC++] -> a(param2)*(1 - c(param3)) + b(param1)*c(param3);
							colorTEX = GX_TEXMAP_NULL;
							colorTEX = ((TEVArgs[color->stage[i].op[j].param1].TEV_TexCoordMap == GX_TEXMAP0) ||
								(TEVArgs[color->stage[i].op[j].param2].TEV_TexCoordMap == GX_TEXMAP0) ||
								(TEVArgs[color->stage[i].op[j].param3].TEV_TexCoordMap == GX_TEXMAP0)) ? GX_TEXMAP0 : colorTEX;
							colorTEX = ((TEVArgs[color->stage[i].op[j].param1].TEV_TexCoordMap == GX_TEXMAP1) ||
								(TEVArgs[color->stage[i].op[j].param2].TEV_TexCoordMap == GX_TEXMAP1) ||
								(TEVArgs[color->stage[i].op[j].param3].TEV_TexCoordMap == GX_TEXMAP1)) ? GX_TEXMAP1 : colorTEX;
							colorCONST = TEV_MAX_CONST + 1;
							colorCONST = (TEVArgs[color->stage[i].op[j].param1].TEV_inpType <= TEV_MAX_CONST) ?
								TEVArgs[color->stage[i].op[j].param1].TEV_inpType : colorCONST;
							colorCONST = (TEVArgs[color->stage[i].op[j].param2].TEV_inpType <= TEV_MAX_CONST) ?
								TEVArgs[color->stage[i].op[j].param1].TEV_inpType : colorCONST;
							colorCONST = (TEVArgs[color->stage[i].op[j].param3].TEV_inpType <= TEV_MAX_CONST) ?
								TEVArgs[color->stage[i].op[j].param1].TEV_inpType : colorCONST;
							//TODO: The following would break if both RGB and AAA versions of the same constant are used.
							colorCONST_AAA = TEVArgs[color->stage[i].op[j].param1].KonstAAA || TEVArgs[color->stage[i].op[j].param2].KonstAAA || TEVArgs[color->stage[i].op[j].param3].KonstAAA;
							while((TEVcombiner->TEVstage[currStageC].texmap != GX_TEXMAP_NULL) &&
								(TEVcombiner->TEVstage[currStageC].texmap != colorTEX))
							{
								SetColorTEVskip(currStageC);
								currStageC++;
							}
							CoutReg = ( (j == (color->stage[i].numOps - 1)) && (i < (color->numStages-1))) ? GX_TEVREG0 : GX_TEVPREV;
							SetColorTEV(currStageC, TEVArgs[color->stage[i].op[j].param2].TEV_colorIn, 
								TEVArgs[color->stage[i].op[j].param1].TEV_colorIn, TEVArgs[color->stage[i].op[j].param3].TEV_colorIn, GX_CC_ZERO,
								GX_TEV_ADD, colorTEX, colorCONST, colorCONST_AAA, CoutReg);
							if (CoutReg == GX_TEVREG0) TEVcombiner->TEVstage[currStageC].colorClamp = GX_ENABLE; //Clamp COMBINED!
							currStageC++;
						}
						else
						{
							if ( !(((TEVArgs[color->stage[i].op[j].param1].TEV_TexCoordMap == GX_TEXMAP0) ||
								(TEVArgs[color->stage[i].op[j].param2].TEV_TexCoordMap == GX_TEXMAP0)) && 
								((TEVArgs[color->stage[i].op[j].param1].TEV_TexCoordMap == GX_TEXMAP1) ||
								(TEVArgs[color->stage[i].op[j].param2].TEV_TexCoordMap == GX_TEXMAP1))) && //!(TEX0 && TEX1)
								!(((TEVArgs[color->stage[i].op[j].param1].TEV_inpType <= TEV_MAX_CONST) && //!(>1 unique constants)
								(TEVArgs[color->stage[i].op[j].param2].TEV_inpType <= TEV_MAX_CONST) &&
								(TEVArgs[color->stage[i].op[j].param1].TEV_inpType != TEVArgs[color->stage[i].op[j].param2].TEV_inpType))))
							{
								//TEVstage[currStageC++] -> d(param1) - a(param2);
								colorTEX = (TEVArgs[color->stage[i].op[j].param1].TEV_TexCoordMap == GX_TEXMAP_NULL) ? 
									TEVArgs[color->stage[i].op[j].param2].TEV_TexCoordMap : TEVArgs[color->stage[i].op[j].param1].TEV_TexCoordMap;
								colorCONST = (TEVArgs[color->stage[i].op[j].param1].TEV_inpType <= TEV_MAX_CONST) ?
									TEVArgs[color->stage[i].op[j].param1].TEV_inpType : TEVArgs[color->stage[i].op[j].param2].TEV_inpType;
								//TODO: The following would break if both RGB and AAA versions of the same constant are used.
								colorCONST_AAA = TEVArgs[color->stage[i].op[j].param1].KonstAAA || TEVArgs[color->stage[i].op[j].param2].KonstAAA;
								while((TEVcombiner->TEVstage[currStageC].texmap != GX_TEXMAP_NULL) &&
									(TEVcombiner->TEVstage[currStageC].texmap != colorTEX))
								{
									SetColorTEVskip(currStageC);
									currStageC++;
								}
								SetColorTEV(currStageC, TEVArgs[color->stage[i].op[j].param2].TEV_colorIn, GX_CC_ZERO, GX_CC_ZERO, 
									TEVArgs[color->stage[i].op[j].param1].TEV_colorIn, GX_TEV_SUB, colorTEX, colorCONST, colorCONST_AAA, GX_TEVPREV);
								currStageC++;
							}
							else
							{
								//TEVstage[currStageC++] -> d(param1);
								while((TEVArgs[color->stage[i].op[j].param1].TEV_inpType == TEV_TEX) &&
									(TEVcombiner->TEVstage[currStageC].texmap != GX_TEXMAP_NULL) &&
									(TEVcombiner->TEVstage[currStageC].texmap != TEVArgs[color->stage[i].op[j].param1].TEV_TexCoordMap))
								{
									SetColorTEVskip(currStageC);
									currStageC++;
								}
								SetColorTEV(currStageC, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, TEVArgs[color->stage[i].op[j].param1].TEV_colorIn,
									GX_TEV_ADD, TEVArgs[color->stage[i].op[j].param1].TEV_TexCoordMap, 
									TEVArgs[color->stage[i].op[j].param1].TEV_inpType, TEVArgs[color->stage[i].op[j].param1].KonstAAA, GX_TEVPREV);
								currStageC++;
								//TEVstage[currStageC++] -> d(PREV) - a(param2);
								while((TEVArgs[color->stage[i].op[j].param2].TEV_inpType == TEV_TEX) &&
									(TEVcombiner->TEVstage[currStageC].texmap != GX_TEXMAP_NULL) &&
									(TEVcombiner->TEVstage[currStageC].texmap != TEVArgs[color->stage[i].op[j].param2].TEV_TexCoordMap))
								{
									SetColorTEVskip(currStageC);
									currStageC++;
								}
								SetColorTEV(currStageC, TEVArgs[color->stage[i].op[j].param2].TEV_colorIn, GX_CC_ZERO, GX_CC_ZERO, GX_CC_CPREV,
									GX_TEV_SUB, TEVArgs[color->stage[i].op[j].param2].TEV_TexCoordMap, 
									TEVArgs[color->stage[i].op[j].param2].TEV_inpType, TEVArgs[color->stage[i].op[j].param2].KonstAAA, GX_TEVPREV);
								currStageC++;
							}
							//TEVstage[currStageC++] -> b(PREV)*c(param3);
							while((TEVArgs[color->stage[i].op[j].param3].TEV_inpType == TEV_TEX) &&
								(TEVcombiner->TEVstage[currStageC].texmap != GX_TEXMAP_NULL) &&
								(TEVcombiner->TEVstage[currStageC].texmap != TEVArgs[color->stage[i].op[j].param3].TEV_TexCoordMap))
							{
								SetColorTEVskip(currStageC);
								currStageC++;
							}
							SetColorTEV(currStageC, GX_CC_ZERO, GX_CC_CPREV, TEVArgs[color->stage[i].op[j].param3].TEV_colorIn, GX_CC_ZERO,
								GX_TEV_ADD, TEVArgs[color->stage[i].op[j].param3].TEV_TexCoordMap, 
								TEVArgs[color->stage[i].op[j].param3].TEV_inpType, TEVArgs[color->stage[i].op[j].param3].KonstAAA, GX_TEVPREV);
							if (currStageC) TEVcombiner->TEVstage[currStageC-1].colorClamp = GX_ENABLE; //TEVPREV used in Mult!
							currStageC++;
							//TEVstage[currStageA++] -> d(PREV) + a(param2);
							while((TEVArgs[color->stage[i].op[j].param2].TEV_inpType == TEV_TEX) &&
								(TEVcombiner->TEVstage[currStageC].texmap != GX_TEXMAP_NULL) &&
								(TEVcombiner->TEVstage[currStageC].texmap != TEVArgs[color->stage[i].op[j].param2].TEV_TexCoordMap))
							{
								SetColorTEVskip(currStageC);
								currStageC++;
							}
							CoutReg = ( (j == (color->stage[i].numOps - 1)) && (i < (color->numStages-1))) ? GX_TEVREG0 : GX_TEVPREV;
							SetColorTEV(currStageC, TEVArgs[color->stage[i].op[j].param2].TEV_colorIn, GX_CC_ZERO, GX_CC_ZERO, GX_CC_CPREV,
								GX_TEV_ADD, TEVArgs[color->stage[i].op[j].param2].TEV_TexCoordMap, 
								TEVArgs[color->stage[i].op[j].param2].TEV_inpType, TEVArgs[color->stage[i].op[j].param2].KonstAAA, GX_TEVPREV);
							currStageC++;
						}
						break;
				}
			}
		}
//		combinedUnit = max( curUnit - 1, 0 );
	}

//	envCombiner->usedUnits = max( curUnit, envCombiner->usedUnits );

	// Increment currStageA and currStageC until we have the same number of Alpha/Color TEV stages.
	while(currStageA < currStageC)
	{
		SetAlphaTEVskip(currStageA);
		currStageA++;
	}
	while(currStageC < currStageA)
	{
		SetColorTEVskip(currStageC);
		currStageC++;
	}
	TEVcombiner->TEVstage[currStageC-1].colorClamp = GX_ENABLE; //Clamp the last TEV stage
	TEVcombiner->TEVstage[currStageA-1].alphaClamp = GX_ENABLE; //Clamp the last TEV stage

	// Store TEV Environment parameters
	TEVcombiner->numTexGens = 0;		//input for GX_SetNumTexGens(u32 nr)
	if (TEVcombiner->usesT0) TEVcombiner->numTexGens++;
	if (TEVcombiner->usesT1) TEVcombiner->numTexGens++;
	TEVcombiner->numColChans = 1;	//input for GX_SetNumChans(u8 num)
	TEVcombiner->numTevStages = currStageA;	//input for GX_SetNumTevStages(u8 num)

#if 0 //def DBGCOMBINE
	sprintf(txtbuffer,"TEV:%d stages, %d tex, %d chans", TEVcombiner->numTevStages, TEVcombiner->numTexGens, TEVcombiner->numColChans);
	DEBUG_print(txtbuffer,18);

	for (u8 tevstage = 0; tevstage < TEVcombiner->numTevStages; tevstage++)
	{
		sprintf(txtbuffer,"stage%d: tx %d, map %d, col %d; inCol %d, %d, %d, %d; ColorOp %d, ColorOut %d", tevstage, TEVcombiner->TEVstage[tevstage].texcoord, TEVcombiner->TEVstage[tevstage].texmap, TEVcombiner->TEVstage[tevstage].color, TEVcombiner->TEVstage[tevstage].colorA, TEVcombiner->TEVstage[tevstage].colorB, TEVcombiner->TEVstage[tevstage].colorC, TEVcombiner->TEVstage[tevstage].colorD, TEVcombiner->TEVstage[tevstage].colorTevop, TEVcombiner->TEVstage[tevstage].colorTevRegOut);
		DEBUG_print(txtbuffer,19+2*tevstage);
		sprintf(txtbuffer," Kcol %d, Kalp %d;  inAlpha %d, %d, %d, %d; AlphaOp %d, AlphaOut %d", TEVcombiner->TEVstage[tevstage].tevKColSel, TEVcombiner->TEVstage[tevstage].tevKAlphaSel, TEVcombiner->TEVstage[tevstage].alphaA, TEVcombiner->TEVstage[tevstage].alphaB, TEVcombiner->TEVstage[tevstage].alphaC, TEVcombiner->TEVstage[tevstage].alphaD, TEVcombiner->TEVstage[tevstage].alphaTevop, TEVcombiner->TEVstage[tevstage].alphaTevRegOut);
		DEBUG_print(txtbuffer,20+2*tevstage);
	}
#endif

	return TEVcombiner;
}

void BeginTextureUpdate_TEV_combine()	//Called from OGL_UpdateStates() before loading new textures.
{
/*	for (int i = 0; i < OGL.maxTextureUnits; i++)
	{
		glActiveTextureARB( GL_TEXTURE0_ARB + i );
		glDisable( GL_TEXTURE_2D );
	}*/
}

void EndTextureUpdate_TEV_combine()	//Never called - Set_texture_env_combine() is called instead.
{
/*	for (int i = 0; i < ((TexEnvCombiner*)combiner.current->compiled)->usedUnits; i++)
	{
		glActiveTextureARB( GL_TEXTURE0_ARB + i );
		glEnable( GL_TEXTURE_2D );
	}*/
}

void Set_TEV_combine( TEVCombiner *TEVcombiner )	//Called from OGL_UpdateStates() after loading new textures.
{													//Also called from OGL_UpdateStates() when combiner changed.  2x necessary???
	combiner.usesT0 = TEVcombiner->usesT0;
	combiner.usesT1 = TEVcombiner->usesT1;
	combiner.usesNoise = FALSE;

	combiner.vertex.color = SHADE;
	combiner.vertex.secondaryColor = SHADE;
	combiner.vertex.alpha = SHADE_ALPHA;

	//Set TEV environment variables
	GX_SetNumChans(TEVcombiner->numColChans);
	GX_SetNumTexGens(TEVcombiner->numTexGens + 1);		//Add an extra TEV stage/texgen for Ztex.
	GX_SetNumTevStages(TEVcombiner->numTevStages + 1);	//TODO: Rearrange code to make this smarter.

#ifdef DBGCOMBINE
#ifdef GLN64_SDLOG
	sprintf(txtbuffer,"TEV:%d stages, %lu tex, %d chans\n", TEVcombiner->numTevStages, TEVcombiner->numTexGens, TEVcombiner->numColChans);
//	DEBUG_print(txtbuffer,18);
	DEBUG_print(txtbuffer,DBG_SDGECKOPRINT);
#endif // GLN64_SDLOG
#endif // DBGCOMBINE

	//Set TEV stage variables
	for (u8 tevstage = 0; tevstage < TEVcombiner->numTevStages; tevstage++)
	{
		GX_SetTevOrder (tevstage, TEVcombiner->TEVstage[tevstage].texcoord, TEVcombiner->TEVstage[tevstage].texmap, TEVcombiner->TEVstage[tevstage].color);
		GX_SetTevColorIn (tevstage, TEVcombiner->TEVstage[tevstage].colorA, TEVcombiner->TEVstage[tevstage].colorB, TEVcombiner->TEVstage[tevstage].colorC, TEVcombiner->TEVstage[tevstage].colorD);
		GX_SetTevColorOp (tevstage, TEVcombiner->TEVstage[tevstage].colorTevop, GX_TB_ZERO, GX_CS_SCALE_1, TEVcombiner->TEVstage[tevstage].colorClamp, TEVcombiner->TEVstage[tevstage].colorTevRegOut);
		GX_SetTevKColorSel(tevstage, TEVcombiner->TEVstage[tevstage].tevKColSel);
		GX_SetTevAlphaIn (tevstage, TEVcombiner->TEVstage[tevstage].alphaA, TEVcombiner->TEVstage[tevstage].alphaB, TEVcombiner->TEVstage[tevstage].alphaC, TEVcombiner->TEVstage[tevstage].alphaD);
		GX_SetTevAlphaOp (tevstage, TEVcombiner->TEVstage[tevstage].alphaTevop, GX_TB_ZERO, GX_CS_SCALE_1, TEVcombiner->TEVstage[tevstage].alphaClamp, TEVcombiner->TEVstage[tevstage].alphaTevRegOut);
		GX_SetTevKAlphaSel(tevstage, TEVcombiner->TEVstage[tevstage].tevKAlphaSel);

#ifdef DBGCOMBINE
#ifdef GLN64_SDLOG
		sprintf(txtbuffer,"stage%d: tx %d, map %d, col %d; inCol %d, %d, %d, %d; ColorOp %d, ColorOut %d\n", tevstage, TEVcombiner->TEVstage[tevstage].texcoord, TEVcombiner->TEVstage[tevstage].texmap, TEVcombiner->TEVstage[tevstage].color, TEVcombiner->TEVstage[tevstage].colorA, TEVcombiner->TEVstage[tevstage].colorB, TEVcombiner->TEVstage[tevstage].colorC, TEVcombiner->TEVstage[tevstage].colorD, TEVcombiner->TEVstage[tevstage].colorTevop, TEVcombiner->TEVstage[tevstage].colorTevRegOut);
//		DEBUG_print(txtbuffer,19+2*tevstage);
		DEBUG_print(txtbuffer,DBG_SDGECKOPRINT);
		sprintf(txtbuffer," Kcol %d, Kalp %d;  inAlpha %d, %d, %d, %d; AlphaOp %d, AlphaOut %d\n", TEVcombiner->TEVstage[tevstage].tevKColSel, TEVcombiner->TEVstage[tevstage].tevKAlphaSel, TEVcombiner->TEVstage[tevstage].alphaA, TEVcombiner->TEVstage[tevstage].alphaB, TEVcombiner->TEVstage[tevstage].alphaC, TEVcombiner->TEVstage[tevstage].alphaD, TEVcombiner->TEVstage[tevstage].alphaTevop, TEVcombiner->TEVstage[tevstage].alphaTevRegOut);
//		DEBUG_print(txtbuffer,20+2*tevstage);
		DEBUG_print(txtbuffer,DBG_SDGECKOPRINT);
#endif // GLN64_SDLOG
#endif // DBGCOMBINE
	}
	//Configure final stage for Ztexture
	GX_SetTevOp ((u8) TEVcombiner->numTevStages, GX_PASSCLR);
	GX_SetTevOrder ((u8) TEVcombiner->numTevStages, GX_TEXMAP2, GX_TEXMAP2, GX_COLORNULL);
}

// ****************************
// TEV Debugging Code
/*	if (combiner.current->combine.muxs1 == 0x350ce37f || combiner.current->combine.muxs1 == 0x00272c60)
	{
		sprintf(txtbuffer,"Combiner: usesT1 = %d, OGL.ARB_multi = %d; GXtex1 = %x", combiner.usesT1, OGL.ARB_multitexture,(u32)cache.current[1]->GXtexture);
		DEBUG_print(txtbuffer,15);
		sprintf(txtbuffer,"Combiner: mux0 = %x, mux1 = %x", combiner.current->combine.muxs0, combiner.current->combine.muxs1);
		DEBUG_print(txtbuffer,16);
		TEVCombiner* TEVcombiner = (TEVCombiner*)combiner.current->compiled;
		sprintf(txtbuffer,"TEV:%d stages, %d tex, %d chans, %d constCols", TEVcombiner->numTevStages, TEVcombiner->numTexGens, TEVcombiner->numColChans, TEVcombiner->numConst);
		DEBUG_print(txtbuffer,17);
		sprintf(txtbuffer,"const Cols: %d, %d, %d, %d, %d, %d, %d, %d", TEVcombiner->TEVconstant[0], TEVcombiner->TEVconstant[1], TEVcombiner->TEVconstant[2], TEVcombiner->TEVconstant[3], TEVcombiner->TEVconstant[4], TEVcombiner->TEVconstant[5], TEVcombiner->TEVconstant[6], TEVcombiner->TEVconstant[7]);
		DEBUG_print(txtbuffer,18);

		for (u8 tevstage = 0; tevstage < TEVcombiner->numTevStages; tevstage++)
		{
			sprintf(txtbuffer,"stage%d: tx %d, map %d, col %d; inCol %d, %d, %d, %d; ColorClamp %d, ColorOp %d, ColorOut %d", tevstage, TEVcombiner->TEVstage[tevstage].texcoord, TEVcombiner->TEVstage[tevstage].texmap, TEVcombiner->TEVstage[tevstage].color, TEVcombiner->TEVstage[tevstage].colorA, TEVcombiner->TEVstage[tevstage].colorB, TEVcombiner->TEVstage[tevstage].colorC, TEVcombiner->TEVstage[tevstage].colorD, TEVcombiner->TEVstage[tevstage].colorClamp, TEVcombiner->TEVstage[tevstage].colorTevop, TEVcombiner->TEVstage[tevstage].colorTevRegOut);
			DEBUG_print(txtbuffer,19+2*tevstage);
			sprintf(txtbuffer," Kcol %d, Kalp %d;  inAlpha %d, %d, %d, %d; AlphaClamp %d, AlphaOp %d, AlphaOut %d", TEVcombiner->TEVstage[tevstage].tevKColSel, TEVcombiner->TEVstage[tevstage].tevKAlphaSel, TEVcombiner->TEVstage[tevstage].alphaA, TEVcombiner->TEVstage[tevstage].alphaB, TEVcombiner->TEVstage[tevstage].alphaC, TEVcombiner->TEVstage[tevstage].alphaD, TEVcombiner->TEVstage[tevstage].alphaClamp, TEVcombiner->TEVstage[tevstage].alphaTevop, TEVcombiner->TEVstage[tevstage].alphaTevRegOut);
			DEBUG_print(txtbuffer,20+2*tevstage);
		}
	}*/
// ****************************

