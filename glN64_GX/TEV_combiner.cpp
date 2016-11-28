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

#define NULL_CONST_REG 4
#define MAX_K_REGS 4

static TEVconstantRegs TEVconstRegs[] =
{ //  Load_Const	Color_RGB_input		Color_A_input		Alpha_A_input
	{ GX_KCOLOR0,	GX_TEV_KCSEL_K0,	GX_TEV_KCSEL_K0_A,	GX_TEV_KASEL_K0_A }, //Konst0
	{ GX_KCOLOR1,	GX_TEV_KCSEL_K1,	GX_TEV_KCSEL_K1_A,	GX_TEV_KASEL_K1_A }, //Konst1
	{ GX_KCOLOR2,	GX_TEV_KCSEL_K2,	GX_TEV_KCSEL_K2_A,	GX_TEV_KASEL_K2_A }, //Konst2
	{ GX_KCOLOR3,	GX_TEV_KCSEL_K3,	GX_TEV_KCSEL_K3_A,	GX_TEV_KASEL_K3_A }  //Konst3
};

static TEVCombinerArg TEVArgs[] =
{	//TEV_inpType,	TEV_colorIn,	TEV_alphaIn,	TEV_TexCoordMap,KonstAAA
	// CMB - Combined color
	{ TEV_CMB,		GX_CC_CPREV,	GX_CA_APREV,	GX_TEXMAP_NULL,	FALSE },
	// T0
	{ TEV_TEX,		GX_CC_TEXC,		GX_CA_TEXA,		GX_TEXMAP0,		FALSE },
	// T1
	{ TEV_TEX,		GX_CC_TEXC,		GX_CA_TEXA,		GX_TEXMAP1,		FALSE },
	// PRIM
	{ TEV_PRIM,		GX_CC_C1,		GX_CA_A1,		GX_TEXMAP_NULL,	FALSE },
	// SHADE
	{ TEV_SHADE,	GX_CC_RASC,		GX_CA_RASA,		GX_TEXMAP_NULL, FALSE },
	// ENV
	{ TEV_ENV,		GX_CC_C2,		GX_CA_A2,		GX_TEXMAP_NULL, FALSE },
	// CENTER
	{ TEV_CENTER,	GX_CC_KONST,	GX_CA_KONST,	GX_TEXMAP_NULL, FALSE },
	// SCALE
	{ TEV_SCALE,	GX_CC_KONST,	GX_CA_KONST,	GX_TEXMAP_NULL, FALSE },
	// CMBALPHA
	{ TEV_CMB,		GX_CC_APREV,	GX_CA_APREV,	GX_TEXMAP_NULL, FALSE },
	// T0ALPHA
	{ TEV_TEX,		GX_CC_TEXA,		GX_CA_TEXA,		GX_TEXMAP0,		FALSE },
	// T1ALPHA
	{ TEV_TEX,		GX_CC_TEXA,		GX_CA_TEXA,		GX_TEXMAP1,		FALSE },
	// PRIMALPHA
	{ TEV_PRIM,		GX_CC_A1,		GX_CA_A1,		GX_TEXMAP_NULL,	FALSE },
	// SHADEALPHA
	{ TEV_SHADE,	GX_CC_RASA,		GX_CA_RASA,		GX_TEXMAP_NULL,	FALSE },
	// ENVALPHA
	{ TEV_ENV,		GX_CC_A2,		GX_CA_A2,		GX_TEXMAP_NULL,	FALSE },
	// LODFRAC
	{ TEV_LODFRAC,	GX_CC_ZERO,		GX_CA_ZERO,		GX_TEXMAP_NULL,	FALSE },
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
	// ZERO
	{ TEV_ZERO,		GX_CC_ZERO,		GX_CA_ZERO,		GX_TEXMAP_NULL,	FALSE }
};

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
			if (TEVcombiner->numConst < MAX_K_REGS) \
			{ \
				TEVcombiner->TEVconstant[alphaCONST] = TEVcombiner->numConst++; \
				TEVcombiner->TEVstage[tevstage].tevKAlphaSel = TEVconstRegs[TEVcombiner->TEVconstant[alphaCONST]].selA_A; \
			} \
		} \
		else \
		{ \
			TEVcombiner->TEVstage[tevstage].tevKAlphaSel = TEVconstRegs[TEVcombiner->TEVconstant[alphaCONST]].selA_A; \
		} \
	}

//SetAlphaTEVskip(tevstage)
#define SetAlphaTEVskip(tevstage) \
	TEVcombiner->TEVstage[tevstage].alphaA = GX_CA_ZERO; \
	TEVcombiner->TEVstage[tevstage].alphaB = GX_CA_ZERO; \
	TEVcombiner->TEVstage[tevstage].alphaC = GX_CA_ZERO; \
	TEVcombiner->TEVstage[tevstage].alphaD = tevstage == GX_TEVSTAGE0 ? GX_CA_ZERO : GX_CA_APREV; \
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
			if (TEVcombiner->numConst < MAX_K_REGS) \
			{ \
				TEVcombiner->TEVconstant[colorCONST] = TEVcombiner->numConst++; \
				if(KonstisAAA) 	TEVcombiner->TEVstage[tevstage].tevKColSel = TEVconstRegs[TEVcombiner->TEVconstant[colorCONST]].selC_AAA; \
				else			TEVcombiner->TEVstage[tevstage].tevKColSel = TEVconstRegs[TEVcombiner->TEVconstant[colorCONST]].selC_RGB; \
			} \
		} \
		else \
		{ \
			if(KonstisAAA) 	TEVcombiner->TEVstage[tevstage].tevKColSel = TEVconstRegs[TEVcombiner->TEVconstant[colorCONST]].selC_AAA; \
			else		 	TEVcombiner->TEVstage[tevstage].tevKColSel = TEVconstRegs[TEVcombiner->TEVconstant[colorCONST]].selC_RGB; \
		} \
	} 

//SetColorTEVskip(tevstage)
#define SetColorTEVskip(tevstage) \
	TEVcombiner->TEVstage[tevstage].colorA = GX_CC_ZERO; \
	TEVcombiner->TEVstage[tevstage].colorB = GX_CC_ZERO; \
	TEVcombiner->TEVstage[tevstage].colorC = GX_CC_ZERO; \
	TEVcombiner->TEVstage[tevstage].colorD = tevstage == GX_TEVSTAGE0 ? GX_CC_ZERO : GX_CC_CPREV; \
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
	//Set Constant TEV registers
	GXColor GXconstantColor;

	GXconstantColor.r = GXcastf32u8(gDP.primColor.r);
	GXconstantColor.g = GXcastf32u8(gDP.primColor.g);
	GXconstantColor.b = GXcastf32u8(gDP.primColor.b);
	GXconstantColor.a = GXcastf32u8(gDP.primColor.a);
	GX_SetTevColor(GX_TEVREG1, GXconstantColor);

	GXconstantColor.r = GXcastf32u8(gDP.envColor.r);
	GXconstantColor.g = GXcastf32u8(gDP.envColor.g);
	GXconstantColor.b = GXcastf32u8(gDP.envColor.b);
	GXconstantColor.a = GXcastf32u8(gDP.envColor.a);
	GX_SetTevColor(GX_TEVREG2, GXconstantColor);

	for (int i = 0; i < TEV_MAX_CONST; i++)
	{
		if ( TEVcombiner->TEVconstant[i] != NULL_CONST_REG )
		{
			switch (i)
			{
			case TEV_CENTER:
				GXconstantColor.r = GXcastf32u8(gDP.key.center.r);
				GXconstantColor.g = GXcastf32u8(gDP.key.center.g);
				GXconstantColor.b = GXcastf32u8(gDP.key.center.b);
				GXconstantColor.a = GXcastf32u8(gDP.key.center.a);
				break;
			case TEV_SCALE:
				GXconstantColor.r = GXcastf32u8(gDP.key.scale.r);
				GXconstantColor.g = GXcastf32u8(gDP.key.scale.g);
				GXconstantColor.b = GXcastf32u8(gDP.key.scale.b);
				GXconstantColor.a = GXcastf32u8(gDP.key.scale.a);
				break;
			case TEV_PRIMLODFRAC:
				GXconstantColor.r =
				GXconstantColor.g =
				GXconstantColor.b =
				GXconstantColor.a = GXcastf32u8(gDP.primColor.l);
				break;
			case TEV_K4:
				GXconstantColor.r =
				GXconstantColor.g =
				GXconstantColor.b =
				GXconstantColor.a = gDP.convert.k4;
				break;
			case TEV_K5:
				GXconstantColor.r =
				GXconstantColor.g =
				GXconstantColor.b =
				GXconstantColor.a = gDP.convert.k5;
				break;
			}
			GX_SetTevKColor(TEVconstRegs[TEVcombiner->TEVconstant[i]].tev_regid, GXconstantColor);

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
	u8 alphaTEX, alphaCONST, colorTEX, colorCONST;
	bool colorCONST_AAA;
//	bool useT0, useT1;
//	int curUnitA, curUnitC, combinedUnit; //, iA, iC, jA, jC, numConst;
	u8 currStageA, currStageC;

	TEVCombiner *TEVcombiner = (TEVCombiner*)malloc( sizeof( TEVCombiner ) );

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

	currStageA = 0;
	currStageC = 0;
//	iA = 0;
//	iC = 0;
//	jA = 0;
//	jC = 0;

	for (int i = 0; i < alpha->numStages; i++)
	{
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
			//Instead of (A - B)*C + D do A*C - B*C + D
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
					//TEVstage[currStageA++] -> ( d(param3) - a(param2) * (ONE(param1) - c(PREV)) );
					alphaTEX = (TEVArgs[alpha->stage[i].op[j+1].param1].TEV_TexCoordMap == GX_TEXMAP_NULL) ?
						TEVArgs[alpha->stage[i].op[j+2].param1].TEV_TexCoordMap : TEVArgs[alpha->stage[i].op[j+1].param1].TEV_TexCoordMap;
					alphaCONST = (TEVArgs[alpha->stage[i].op[j+1].param1].TEV_inpType <= TEV_MAX_CONST) ?
						TEVArgs[alpha->stage[i].op[j+1].param1].TEV_inpType : TEVArgs[alpha->stage[i].op[j+2].param1].TEV_inpType;
					SetAlphaTEV(currStageA, TEVArgs[alpha->stage[i].op[j+1].param1].TEV_alphaIn, GX_CA_ZERO,
						GX_CA_A0, TEVArgs[alpha->stage[i].op[j+2].param1].TEV_alphaIn,
						GX_TEV_SUB, alphaTEX, alphaCONST, GX_TEVREG0);
					if (currStageA) TEVcombiner->TEVstage[currStageA-1].alphaClamp = GX_ENABLE; //TEVPREV used in Mult!
					currStageA++;
					j+=2;
				}
				else
				{
					//TEVstage[currStageA++] -> ( d(param3) + b(PREV) * c(param2) );
					alphaTEX = (TEVArgs[alpha->stage[i].op[j+1].param1].TEV_TexCoordMap == GX_TEXMAP_NULL) ?
						TEVArgs[alpha->stage[i].op[j+2].param1].TEV_TexCoordMap : TEVArgs[alpha->stage[i].op[j+1].param1].TEV_TexCoordMap;
					alphaCONST = (TEVArgs[alpha->stage[i].op[j+1].param1].TEV_inpType <= TEV_MAX_CONST) ?
						TEVArgs[alpha->stage[i].op[j+1].param1].TEV_inpType : TEVArgs[alpha->stage[i].op[j+2].param1].TEV_inpType;
					SetAlphaTEV(currStageA, GX_CC_ZERO, GX_CA_A0, TEVArgs[alpha->stage[i].op[j+1].param1].TEV_alphaIn,
						TEVArgs[alpha->stage[i].op[j+2].param1].TEV_alphaIn,
						GX_TEV_ADD, alphaTEX, alphaCONST, GX_TEVREG0);
					if (currStageA) TEVcombiner->TEVstage[currStageA-1].alphaClamp = GX_ENABLE; //TEVPREV used in Mult!
					currStageA++;
					//TEVstage[currStageA++] -> ( d(PREV) - b(param1) * c(param2) );
					alphaTEX = (TEVArgs[alpha->stage[i].op[j].param1].TEV_TexCoordMap == GX_TEXMAP_NULL) ?
						TEVArgs[alpha->stage[i].op[j+1].param1].TEV_TexCoordMap : TEVArgs[alpha->stage[i].op[j].param1].TEV_TexCoordMap;
					alphaCONST = (TEVArgs[alpha->stage[i].op[j].param1].TEV_inpType <= TEV_MAX_CONST) ?
						TEVArgs[alpha->stage[i].op[j].param1].TEV_inpType : TEVArgs[alpha->stage[i].op[j+1].param1].TEV_inpType;
					SetAlphaTEV(currStageA, GX_CC_ZERO, TEVArgs[alpha->stage[i].op[j].param1].TEV_alphaIn,
						TEVArgs[alpha->stage[i].op[j+1].param1].TEV_alphaIn, GX_CA_A0,
						GX_TEV_SUB, alphaTEX, alphaCONST, GX_TEVREG0);
					currStageA++;
					j+=2;
				}
			}
			else
			{
				TEVcombiner->usesT0 |= alpha->stage[i].op[j].param1 == TEXEL0_ALPHA;
				TEVcombiner->usesT1 |= alpha->stage[i].op[j].param1 == TEXEL1_ALPHA;

				switch (alpha->stage[i].op[j].op)
				{
					case LOAD:
						//TEVstage[currStageA++] -> ( d(param1) )
						SetAlphaTEV(currStageA, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, TEVArgs[alpha->stage[i].op[j].param1].TEV_alphaIn, 
							GX_TEV_ADD, TEVArgs[alpha->stage[i].op[j].param1].TEV_TexCoordMap, TEVArgs[alpha->stage[i].op[j].param1].TEV_inpType, GX_TEVREG0);
						currStageA++;
						break;
					case SUB:
						//TODO: Fix alpha input == ONE cases.
						if ( (alpha->stage[i].op[j-1].param1 == ONE) && (alpha->stage[i].op[j-1].op == LOAD) )
						{
							currStageA--;
							//TEVstage[currStageA++] -> ( ONE(param1) - c(param2) );
							SetAlphaTEV(currStageA, GX_CA_KONST, GX_CA_ZERO, TEVArgs[alpha->stage[i].op[j].param1].TEV_alphaIn, GX_CA_ZERO,
								GX_TEV_ADD, TEVArgs[alpha->stage[i].op[j].param1].TEV_TexCoordMap, TEVArgs[alpha->stage[i].op[j].param1].TEV_inpType, GX_TEVREG0);
							currStageA++;
						}
						else
						{
							//TEVstage[currStageA++] -> ( d(PREV) - a(param1) );
							SetAlphaTEV(currStageA, TEVArgs[alpha->stage[i].op[j].param1].TEV_alphaIn, GX_CA_ZERO, GX_CA_ZERO, GX_CA_A0,
								GX_TEV_SUB, TEVArgs[alpha->stage[i].op[j].param1].TEV_TexCoordMap, TEVArgs[alpha->stage[i].op[j].param1].TEV_inpType, GX_TEVREG0);
							currStageA++;
						}
						break;
					case MUL:
						//TEVstage[currStageA++] -> ( b(PREV) * c(param1) );
						SetAlphaTEV(currStageA, GX_CA_ZERO, GX_CA_A0, TEVArgs[alpha->stage[i].op[j].param1].TEV_alphaIn, GX_CA_ZERO,
							GX_TEV_ADD, TEVArgs[alpha->stage[i].op[j].param1].TEV_TexCoordMap, TEVArgs[alpha->stage[i].op[j].param1].TEV_inpType, GX_TEVREG0);
						if (currStageA) TEVcombiner->TEVstage[currStageA-1].alphaClamp = GX_ENABLE; //TEVPREV used in Mult!
						currStageA++;
						break;
					case ADD:
						//TEVstage[currStageA++] -> ( d(PREV) + a(param1) );
						SetAlphaTEV(currStageA, TEVArgs[alpha->stage[i].op[j].param1].TEV_alphaIn, GX_CA_ZERO, GX_CA_ZERO, GX_CA_A0,
							GX_TEV_ADD, TEVArgs[alpha->stage[i].op[j].param1].TEV_TexCoordMap, TEVArgs[alpha->stage[i].op[j].param1].TEV_inpType, GX_TEVREG0);
						currStageA++;
						break;
					case INTER:
						TEVcombiner->usesT0 |= (alpha->stage[i].op[j].param2 == TEXEL0_ALPHA) || (alpha->stage[i].op[j].param3 == TEXEL0_ALPHA);
						TEVcombiner->usesT1 |= (alpha->stage[i].op[j].param2 == TEXEL1_ALPHA) || (alpha->stage[i].op[j].param3 == TEXEL1_ALPHA);

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
								TEVArgs[alpha->stage[i].op[j].param2].TEV_inpType : alphaCONST;
							alphaCONST = (TEVArgs[alpha->stage[i].op[j].param3].TEV_inpType <= TEV_MAX_CONST) ?
								TEVArgs[alpha->stage[i].op[j].param3].TEV_inpType : alphaCONST;
							SetAlphaTEV(currStageA, TEVArgs[alpha->stage[i].op[j].param2].TEV_alphaIn, 
								TEVArgs[alpha->stage[i].op[j].param1].TEV_alphaIn, TEVArgs[alpha->stage[i].op[j].param3].TEV_alphaIn, GX_CA_ZERO,
								GX_TEV_ADD, alphaTEX, alphaCONST, GX_TEVREG0);
							currStageA++;
						}
						else
						{
							//TEVstage[currStageA++] -> ( a(param2) * (1 - c(param3)) );
							alphaTEX = (TEVArgs[alpha->stage[i].op[j].param2].TEV_TexCoordMap == GX_TEXMAP_NULL) ? 
								TEVArgs[alpha->stage[i].op[j].param3].TEV_TexCoordMap : TEVArgs[alpha->stage[i].op[j].param2].TEV_TexCoordMap;
							alphaCONST = (TEVArgs[alpha->stage[i].op[j].param2].TEV_inpType <= TEV_MAX_CONST) ?
								TEVArgs[alpha->stage[i].op[j].param2].TEV_inpType : TEVArgs[alpha->stage[i].op[j].param3].TEV_inpType;
							SetAlphaTEV(currStageA, TEVArgs[alpha->stage[i].op[j].param2].TEV_alphaIn, GX_CA_ZERO,
								TEVArgs[alpha->stage[i].op[j].param3].TEV_alphaIn, GX_CA_ZERO,
								GX_TEV_ADD, alphaTEX, alphaCONST, GX_TEVREG0);
							currStageA++;
							//TEVstage[currStageA++] -> ( d(PREV) + b(param1) * c(param3) );
							alphaTEX = (TEVArgs[alpha->stage[i].op[j].param1].TEV_TexCoordMap == GX_TEXMAP_NULL) ? 
								TEVArgs[alpha->stage[i].op[j].param3].TEV_TexCoordMap : TEVArgs[alpha->stage[i].op[j].param1].TEV_TexCoordMap;
							alphaCONST = (TEVArgs[alpha->stage[i].op[j].param1].TEV_inpType <= TEV_MAX_CONST) ?
								TEVArgs[alpha->stage[i].op[j].param1].TEV_inpType : TEVArgs[alpha->stage[i].op[j].param3].TEV_inpType;
							SetAlphaTEV(currStageA, GX_CA_ZERO, TEVArgs[alpha->stage[i].op[j].param1].TEV_alphaIn,
								TEVArgs[alpha->stage[i].op[j].param3].TEV_alphaIn, GX_CA_A0,
								GX_TEV_ADD, alphaTEX, alphaCONST, GX_TEVREG0);
							currStageA++;
						}
						break;
				}
			}
			if (j == alpha->stage[i].numOps-1)
			{
				TEVcombiner->TEVstage[currStageA-1].alphaClamp = GX_ENABLE;
				TEVcombiner->TEVstage[currStageA-1].alphaTevRegOut = GX_TEVPREV;
			}
		}
	}

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
			//Instead of (A - B)*C + D do A*C - B*C + D
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

				//TEVstage[currStageC++] -> ( d(param3) + b(PREV) * c(param2) );
				colorTEX = (TEVArgs[color->stage[i].op[j+1].param1].TEV_TexCoordMap == GX_TEXMAP_NULL) ?
					TEVArgs[color->stage[i].op[j+2].param1].TEV_TexCoordMap : TEVArgs[color->stage[i].op[j+1].param1].TEV_TexCoordMap;
				colorCONST = (TEVArgs[color->stage[i].op[j+1].param1].TEV_inpType <= TEV_MAX_CONST) ?
					TEVArgs[color->stage[i].op[j+1].param1].TEV_inpType : TEVArgs[color->stage[i].op[j+2].param1].TEV_inpType;
				//TODO: The following would break if both RGB and AAA versions of the same constant are used.
				colorCONST_AAA = TEVArgs[color->stage[i].op[j+1].param1].KonstAAA || TEVArgs[color->stage[i].op[j+2].param1].KonstAAA;
				while((TEVcombiner->TEVstage[currStageC].texmap != GX_TEXMAP_NULL) &&
					(TEVcombiner->TEVstage[currStageC].texmap != colorTEX))
				{
					SetColorTEVskip(currStageC);
					currStageC++;
				}
				SetColorTEV(currStageC, GX_CC_ZERO, GX_CC_C0, TEVArgs[color->stage[i].op[j+1].param1].TEV_colorIn,
					TEVArgs[color->stage[i].op[j+2].param1].TEV_colorIn,
					GX_TEV_ADD, colorTEX, colorCONST, colorCONST_AAA, GX_TEVREG0);
				if (currStageC) TEVcombiner->TEVstage[currStageC-1].colorClamp = GX_ENABLE; //TEVPREV used in Mult!
				currStageC++;
				//TEVstage[currStageC++] -> ( d(PREV) - b(param1) * c(param2) );
				colorTEX = (TEVArgs[color->stage[i].op[j].param1].TEV_TexCoordMap == GX_TEXMAP_NULL) ?
					TEVArgs[color->stage[i].op[j+1].param1].TEV_TexCoordMap : TEVArgs[color->stage[i].op[j].param1].TEV_TexCoordMap;
				colorCONST = (TEVArgs[color->stage[i].op[j].param1].TEV_inpType <= TEV_MAX_CONST) ?
					TEVArgs[color->stage[i].op[j].param1].TEV_inpType : TEVArgs[color->stage[i].op[j+1].param1].TEV_inpType;
				//TODO: The following would break if both RGB and AAA versions of the same constant are used.
				colorCONST_AAA = TEVArgs[color->stage[i].op[j].param1].KonstAAA || TEVArgs[color->stage[i].op[j+1].param1].KonstAAA;
				while((TEVcombiner->TEVstage[currStageC].texmap != GX_TEXMAP_NULL) &&
					(TEVcombiner->TEVstage[currStageC].texmap != colorTEX))
				{
					SetColorTEVskip(currStageC);
					currStageC++;
				}
				SetColorTEV(currStageC, GX_CC_ZERO, TEVArgs[color->stage[i].op[j].param1].TEV_colorIn,
					TEVArgs[color->stage[i].op[j+1].param1].TEV_colorIn, GX_CC_C0,
					GX_TEV_SUB, colorTEX, colorCONST, colorCONST_AAA, GX_TEVREG0);
				currStageC++;
				j+=2;
			}
			else
			{
				TEVcombiner->usesT0 |= ((color->stage[i].op[j].param1 == TEXEL0) || (color->stage[i].op[j].param1 == TEXEL0_ALPHA));
				TEVcombiner->usesT1 |= ((color->stage[i].op[j].param1 == TEXEL1) || (color->stage[i].op[j].param1 == TEXEL1_ALPHA));

				switch (color->stage[i].op[j].op)
				{
					case LOAD:
						//TEVstage[currStageC++] -> ( d(param1) )
						while((TEVArgs[color->stage[i].op[j].param1].TEV_inpType == TEV_TEX) &&
							(TEVcombiner->TEVstage[currStageC].texmap != GX_TEXMAP_NULL) &&
							(TEVcombiner->TEVstage[currStageC].texmap != TEVArgs[color->stage[i].op[j].param1].TEV_TexCoordMap))
						{
							SetColorTEVskip(currStageC);
							currStageC++;
						}
						SetColorTEV(currStageC, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, TEVArgs[color->stage[i].op[j].param1].TEV_colorIn,  
							GX_TEV_ADD, TEVArgs[color->stage[i].op[j].param1].TEV_TexCoordMap, 
							TEVArgs[color->stage[i].op[j].param1].TEV_inpType, TEVArgs[color->stage[i].op[j].param1].KonstAAA, GX_TEVREG0);
						currStageC++;
						break;
					case SUB:
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
							SetColorTEV(currStageC, GX_CC_ONE, GX_CC_ZERO, TEVArgs[color->stage[i].op[j].param1].TEV_colorIn, GX_CC_ZERO,
								GX_TEV_ADD, TEVArgs[color->stage[i].op[j].param1].TEV_TexCoordMap, 
								TEVArgs[color->stage[i].op[j].param1].TEV_inpType, TEVArgs[color->stage[i].op[j].param1].KonstAAA, GX_TEVREG0);
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
							SetColorTEV(currStageC, TEVArgs[color->stage[i].op[j].param1].TEV_colorIn, GX_CC_ZERO, GX_CC_ZERO, GX_CC_C0,
								GX_TEV_SUB, TEVArgs[color->stage[i].op[j].param1].TEV_TexCoordMap, 
								TEVArgs[color->stage[i].op[j].param1].TEV_inpType, TEVArgs[color->stage[i].op[j].param1].KonstAAA, GX_TEVREG0);
							currStageC++;
						}
						break;
					case MUL:
						//TEVstage[currStageC++] -> ( b(PREV) * c(param1) );
						while((TEVArgs[color->stage[i].op[j].param1].TEV_inpType == TEV_TEX) &&
							(TEVcombiner->TEVstage[currStageC].texmap != GX_TEXMAP_NULL) &&
							(TEVcombiner->TEVstage[currStageC].texmap != TEVArgs[color->stage[i].op[j].param1].TEV_TexCoordMap))
						{
							SetColorTEVskip(currStageC);
							currStageC++;
						}
						SetColorTEV(currStageC, GX_CC_ZERO, GX_CC_C0, TEVArgs[color->stage[i].op[j].param1].TEV_colorIn, GX_CC_ZERO,
							GX_TEV_ADD, TEVArgs[color->stage[i].op[j].param1].TEV_TexCoordMap, 
							TEVArgs[color->stage[i].op[j].param1].TEV_inpType, TEVArgs[color->stage[i].op[j].param1].KonstAAA, GX_TEVREG0);
						if (currStageC) TEVcombiner->TEVstage[currStageC-1].colorClamp = GX_ENABLE; //TEVPREV used in Mult!
						currStageC++;
						break;
					case ADD:
						//TEVstage[currStageC++] -> ( d(PREV) + a(param1) );
						while((TEVArgs[color->stage[i].op[j].param1].TEV_inpType == TEV_TEX) &&
							(TEVcombiner->TEVstage[currStageC].texmap != GX_TEXMAP_NULL) &&
							(TEVcombiner->TEVstage[currStageC].texmap != TEVArgs[color->stage[i].op[j].param1].TEV_TexCoordMap))
						{
							SetColorTEVskip(currStageC);
							currStageC++;
						}
						SetColorTEV(currStageC, TEVArgs[color->stage[i].op[j].param1].TEV_colorIn, GX_CC_ZERO, GX_CC_ZERO, GX_CC_C0,
							GX_TEV_ADD, TEVArgs[color->stage[i].op[j].param1].TEV_TexCoordMap, 
							TEVArgs[color->stage[i].op[j].param1].TEV_inpType, TEVArgs[color->stage[i].op[j].param1].KonstAAA, GX_TEVREG0);
						currStageC++;
						break;
					case INTER:
						TEVcombiner->usesT0 |= (color->stage[i].op[j].param2 == TEXEL0) || (color->stage[i].op[j].param2 == TEXEL0_ALPHA) || (color->stage[i].op[j].param3 == TEXEL0) || (color->stage[i].op[j].param3 == TEXEL0_ALPHA);
						TEVcombiner->usesT1 |= (color->stage[i].op[j].param2 == TEXEL1) || (color->stage[i].op[j].param2 == TEXEL1_ALPHA) || (color->stage[i].op[j].param3 == TEXEL1) || (color->stage[i].op[j].param3 == TEXEL1_ALPHA);

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
								TEVArgs[color->stage[i].op[j].param2].TEV_inpType : colorCONST;
							colorCONST = (TEVArgs[color->stage[i].op[j].param3].TEV_inpType <= TEV_MAX_CONST) ?
								TEVArgs[color->stage[i].op[j].param3].TEV_inpType : colorCONST;
							//TODO: The following would break if both RGB and AAA versions of the same constant are used.
							colorCONST_AAA = TEVArgs[color->stage[i].op[j].param1].KonstAAA || TEVArgs[color->stage[i].op[j].param2].KonstAAA || TEVArgs[color->stage[i].op[j].param3].KonstAAA;
							while((TEVcombiner->TEVstage[currStageC].texmap != GX_TEXMAP_NULL) &&
								(TEVcombiner->TEVstage[currStageC].texmap != colorTEX))
							{
								SetColorTEVskip(currStageC);
								currStageC++;
							}
							SetColorTEV(currStageC, TEVArgs[color->stage[i].op[j].param2].TEV_colorIn, 
								TEVArgs[color->stage[i].op[j].param1].TEV_colorIn, TEVArgs[color->stage[i].op[j].param3].TEV_colorIn, GX_CC_ZERO,
								GX_TEV_ADD, colorTEX, colorCONST, colorCONST_AAA, GX_TEVREG0);
							currStageC++;
						}
						else
						{
							//TEVstage[currStageC++] -> ( a(param2) * (1 - c(param3)) );
							colorTEX = (TEVArgs[color->stage[i].op[j].param2].TEV_TexCoordMap == GX_TEXMAP_NULL) ?
								TEVArgs[color->stage[i].op[j].param3].TEV_TexCoordMap : TEVArgs[color->stage[i].op[j].param2].TEV_TexCoordMap;
							colorCONST = (TEVArgs[color->stage[i].op[j].param2].TEV_inpType <= TEV_MAX_CONST) ?
								TEVArgs[color->stage[i].op[j].param2].TEV_inpType : TEVArgs[color->stage[i].op[j].param3].TEV_inpType;
							//TODO: The following would break if both RGB and AAA versions of the same constant are used.
							colorCONST_AAA = TEVArgs[color->stage[i].op[j].param2].KonstAAA || TEVArgs[color->stage[i].op[j].param3].KonstAAA;
							while((TEVcombiner->TEVstage[currStageC].texmap != GX_TEXMAP_NULL) &&
								(TEVcombiner->TEVstage[currStageC].texmap != colorTEX))
							{
								SetColorTEVskip(currStageC);
								currStageC++;
							}
							SetColorTEV(currStageC, TEVArgs[color->stage[i].op[j].param2].TEV_colorIn, GX_CC_ZERO,
								TEVArgs[color->stage[i].op[j].param3].TEV_colorIn, GX_CC_ZERO,
								GX_TEV_ADD, colorTEX, colorCONST, colorCONST_AAA, GX_TEVREG0);
							currStageC++;
							//TEVstage[currStageC++] -> ( d(PREV) + b(param1) * c(param3) );
							colorTEX = (TEVArgs[color->stage[i].op[j].param1].TEV_TexCoordMap == GX_TEXMAP_NULL) ?
								TEVArgs[color->stage[i].op[j].param3].TEV_TexCoordMap : TEVArgs[color->stage[i].op[j].param1].TEV_TexCoordMap;
							colorCONST = (TEVArgs[color->stage[i].op[j].param1].TEV_inpType <= TEV_MAX_CONST) ?
								TEVArgs[color->stage[i].op[j].param1].TEV_inpType : TEVArgs[color->stage[i].op[j].param3].TEV_inpType;
							//TODO: The following would break if both RGB and AAA versions of the same constant are used.
							colorCONST_AAA = TEVArgs[color->stage[i].op[j].param1].KonstAAA || TEVArgs[color->stage[i].op[j].param3].KonstAAA;
							while((TEVcombiner->TEVstage[currStageC].texmap != GX_TEXMAP_NULL) &&
								(TEVcombiner->TEVstage[currStageC].texmap != colorTEX))
							{
								SetColorTEVskip(currStageC);
								currStageC++;
							}
							SetColorTEV(currStageC, GX_CC_ZERO, TEVArgs[color->stage[i].op[j].param1].TEV_colorIn,
								TEVArgs[color->stage[i].op[j].param3].TEV_colorIn, GX_CC_C0,
								GX_TEV_ADD, colorTEX, colorCONST, colorCONST_AAA, GX_TEVREG0);
							currStageC++;
						}
						break;
				}
			}
			if (j == color->stage[i].numOps-1)
			{
				TEVcombiner->TEVstage[currStageC-1].colorClamp = GX_ENABLE;
				TEVcombiner->TEVstage[currStageC-1].colorTevRegOut = GX_TEVPREV;
			}
		}
	}

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

