/**
 * Wii64 - cc_GX.cpp
 * Copyright (C) 2002 Hacktarux 
 * Copyright (C) 2007, 2008 sepp256
 * 
 * N64 GX plugin, based off Hacktarux's soft_gfx
 * by sepp256 for Mupen64-GC
 *
 * Wii64 homepage: http://www.emulatemii.com
 * email address: sepp256@gmail.com
 *
 *
 * This program is free software; you can redistribute it and/
 * or modify it under the terms of the GNU General Public Li-
 * cence as published by the Free Software Foundation; either
 * version 2 of the Licence, or any later version.
 *
 * This program is distributed in the hope that it will be use-
 * ful, but WITHOUT ANY WARRANTY; without even the implied war-
 * ranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public Licence for more details.
 *
**/

#include <stdio.h>

#include "global.h"
#include "cc_GX.h"
#include "../gui/DEBUG.h"

CC::CC() : oldCycle1(0), oldCycle2(0)
{
	newCycle = true;
}

CC::~CC()
{
}

void CC::setCombineKey(int value)
{
   combineKey = value;
}

void CC::setCombineMode(int cycle1, int cycle2)
{
   if (cycle1 == oldCycle1 && cycle2 == oldCycle2) return;
   oldCycle1 = cycle1;
   oldCycle2 = cycle2;

   newCycle = true;

   
//   int a0,b0,c0,d0,Aa0,Ab0,Ac0,Ad0,a1,b1,c1,d1,Aa1,Ab1,Ac1,Ad1;
   colorSrc[0][0] = (cycle1 >> 20) & 0xF;	//a0
   colorSrc[0][2] = (cycle1 >> 15) & 0x1F;	//c0
   alphaSrc[0][0] = (cycle1 >> 12) & 0x7;	//Aa0
   alphaSrc[0][2] = (cycle1 >>  9) & 0x7;	//Ac0
   colorSrc[1][0] = (cycle1 >>  5) & 0xF;	//a1
   colorSrc[1][2] = (cycle1 >>  0) & 0x1F;	//c1
   colorSrc[0][1] = (cycle2 >> 28) & 0xF;	//b0
   colorSrc[0][3] = (cycle2 >> 15) & 0x7;	//d0
   alphaSrc[0][1] = (cycle2 >> 12) & 0x7;	//Ab0
   alphaSrc[0][3] = (cycle2 >>  9) & 0x7;	//Ad0
   colorSrc[1][1] = (cycle2 >> 24) & 0xF;	//b1
   alphaSrc[1][0] = (cycle2 >> 21) & 0x7;	//Aa1
   alphaSrc[1][2] = (cycle2 >> 18) & 0x7;	//Ac1
   colorSrc[1][3] = (cycle2 >>  6) & 0x7;	//d1
   alphaSrc[1][1] = (cycle2 >>  3) & 0x7;	//Ab1
   alphaSrc[1][3] = (cycle2 >>  0) & 0x7;	//Ad1
   
//   int colorSrc[2][4]; //a0,b0,c0,d0,a1,b1,c1,d1
//   int alphaSrc[2][4]; //Aa0,Ab0,Ac0,Ad0,Aa1,Ab1,Ac1,Ad1


   //Use the TEV/KONST registers as follows:
   //old
   // GX_TEVREG0 -> combined
   // GX_TEVREG1 -> primColor
   // GX_TEVREG2 -> envColor
   //proposed new
   // GX_TEVREG0 -> combined
   // GX_TEVKREG0 -> primColor
   // GX_TEVKREG1 -> envColor
   // GX_TEVKREG2 -> fogColor (blending)
   // GX_TEVKREG3 -> shadeColor (not used for now)

   for (int i=0;i<2;i++){
	   for (int j=0;j<4;j++){
		   //Set color source
		   texSrc[i][j] = CC_TEXNULL;
		   switch(colorSrc[i][j])
		   {
		   case 0: //&combined
				GXtevcolorarg[i][j] = GX_CC_C0;
				break;
		   case 1: //&texel0
				GXtevcolorarg[i][j] = GX_CC_TEXC;
				texSrc[i][j] = CC_TEX0;
				break;
		   case 2: //&texel1
				GXtevcolorarg[i][j] = GX_CC_TEXC;
				texSrc[i][j] = CC_TEX1;
				break;
		   case 3: //&primColor;
				GXtevcolorarg[i][j] = GX_CC_C1;
				break;
		   case 4: //&shade 
				GXtevcolorarg[i][j] = GX_CC_RASC;
				break;
		   case 5: //&envColor
				GXtevcolorarg[i][j] = GX_CC_C2;
				break;
		   case 7:
				if (j == 3) //&zero
					GXtevcolorarg[i][j] = GX_CC_ZERO;
				else { //&combinedAlpha
					sprintf(txtbuffer,"CC:unknown color combiner source:%d,%d\n", i, j);
					DEBUG_print(txtbuffer,DBG_CCINFO);
					GXtevcolorarg[i][j] = GX_CC_A0;
				}
				break;
		   case 8:
				if (j == 2) { //&texel0Alpha
					GXtevcolorarg[i][j] = GX_CC_TEXA;
					texSrc[i][j] = CC_TEX0;
				}
				else { //&zero
					sprintf(txtbuffer,"CC:unknown color combiner source:%d,%d\n", i, j);
					DEBUG_print(txtbuffer,DBG_CCINFO);
					GXtevcolorarg[i][j] = GX_CC_ZERO;
				}
				break;
		   case 9:
				if (j == 2) { //&texel1Alpha
					GXtevcolorarg[i][j] = GX_CC_TEXA;
					texSrc[i][j] = CC_TEX1;
				}
				else  //&zero
					GXtevcolorarg[i][j] = GX_CC_ZERO;
				break;
		   case 12:
				if (j == 1) //&zero
					GXtevcolorarg[i][j] = GX_CC_ZERO;
				else { //&zero
					sprintf(txtbuffer,"CC:unknown color combiner source:%d,%d\n", i, j);
					DEBUG_print(txtbuffer,DBG_CCINFO);
					GXtevcolorarg[i][j] = GX_CC_ZERO;
				}
				break;
		   case 13:
				if (j == 2) //&LODFraction - not implemented or used?
					GXtevcolorarg[i][j] = GX_CC_ZERO;
				else { //&zero
					sprintf(txtbuffer,"CC:unknown color combiner source:%d,%d\n", i, j);
					DEBUG_print(txtbuffer,DBG_CCINFO);
					GXtevcolorarg[i][j] = GX_CC_ZERO;
				}
				break;
		   case 15:
				if (j == 0 || j == 1)
					GXtevcolorarg[i][j] = GX_CC_ZERO;
				else { //&zero
					sprintf(txtbuffer,"CC:unknown color combiner source:%d,%d\n", i, j);
					DEBUG_print(txtbuffer,DBG_CCINFO);
					GXtevcolorarg[i][j] = GX_CC_ZERO;
				}
				break;
		   case 31: //&zero
				GXtevcolorarg[i][j] = GX_CC_ZERO;
				break;
		   default:
				sprintf(txtbuffer,"CC:unknown color combiner source:%d\n", colorSrc[i][j]);
				DEBUG_print(txtbuffer,DBG_CCINFO);
				GXtevcolorarg[i][j] = GX_CC_ZERO;
		   }

		   //Set alpha source
		   switch(alphaSrc[i][j])
		   {
		   case 0:
				if (j == 2) //LODFraction.getAlphap() - not implemented or used?
					GXtevalphaarg[i][j] = GX_CA_ZERO;
				else  //&combined.getAlpha()
					GXtevalphaarg[i][j] = GX_CA_A0;
				break;
		   case 1: //texel0.getAlphap()
				GXtevalphaarg[i][j] = GX_CA_TEXA;
				if (texSrc[i][j] == CC_TEX1) DEBUG_print((char*)"CC:TEX0 and TEX1 on same stage!\n",DBG_CCINFO);
				texSrc[i][j] = CC_TEX0;
				break;
		   case 2: //texel1.getAlphap()
				GXtevalphaarg[i][j] = GX_CA_TEXA;
				if (texSrc[i][j] == CC_TEX0) DEBUG_print((char*)"CC:TEX0 and TEX1 on same stage!\n",DBG_CCINFO);
				texSrc[i][j] = CC_TEX1;
				break;
		   case 3: //primColor.getAlphap()
				GXtevalphaarg[i][j] = GX_CA_A1;
				break;
		   case 4: //shade.getAlphap()
				GXtevalphaarg[i][j] = GX_CA_RASA;
				break;
		   case 5: //envColor.getAlphap()
				GXtevalphaarg[i][j] = GX_CA_A2;
				break;
		   case 7: //zero.getAlphap()
				GXtevalphaarg[i][j] = GX_CA_ZERO;
				break;
		   default:
				sprintf(txtbuffer,"CC:unknown alpha combiner source:%d\n", alphaSrc[i][j]);
				DEBUG_print(txtbuffer,DBG_CCINFO);
				GXtevalphaarg[i][j] = GX_CA_ZERO;
		   }
	   }
   }
}

void CC::setPrimColor(int color, float m, float l)
{
//   primColor = color;
   mLOD = m;
   lLOD = l;
	GXprimColor.r = (u8) (color >> 24) & 0xFF;
	GXprimColor.g = (u8) (color >> 16) & 0xFF;
	GXprimColor.b = (u8) (color >> 8) & 0xFF;
	GXprimColor.a = (u8) color&0xFF;

	// GX_TEVKREG0 -> primColor
	GX_SetTevColor(GX_TEVREG1,GXprimColor);
//	GX_SetTevKColor(GX_TEVKREG0,GXprimColor);
}

void CC::setEnvColor(int color)
{
//   envColor = color;
	GXenvColor.r = (u8) (color >> 24) & 0xFF;
	GXenvColor.g = (u8) (color >> 16) & 0xFF;
	GXenvColor.b = (u8) (color >> 8) & 0xFF;
	GXenvColor.a = (u8) color&0xFF;

    // GX_TEVKREG1 -> envColor
	GX_SetTevColor(GX_TEVREG2,GXenvColor);
//	GX_SetTevKColor(GX_TEVKREG1,GXenvColor);
}

void CC::setShade(const Color32& c)
{
//   shade = c;
/*   	 GXshade.r = (u8) c.getR();
	 GXshade.g = (u8) c.getG();
	 GXshade.b = (u8) c.getB();
	 GXshade.a = (u8) c.getAlpha();*/
//	Shading is handled by GX
}

void CC::combine1(u8 tile0, bool tex_en)
{
/*   texel0 = texel;
   texel0Alpha = Color32(texel0.getAlpha(), texel0.getAlpha(), texel0.getAlpha(), texel0.getAlpha());
   Color32 c =  (*pa0 - *pb0)* *pc0 + *pd0;
   float Ac0 = *pAc0 / 255.0f;
   c.setAlpha((Ac0 * (*pAa0 - *pAb0)) + *pAd0);
   return c;*/

// Combine1: (A-B)C+D
//Tevstage 0 -> A-B
//Tevstage 1 -> Tevprev*C + D

	u8 GXclrtmp[2][4];
	u8 GXalphatmp[2][4];

	for (int i=0;i<4;i++) {
		GXclrtmp[0][i] = GXtevcolorarg[0][i];
		GXalphatmp[0][i] = GXtevalphaarg[0][i];
		if (!tex_en && (GXclrtmp[0][i] == GX_CC_TEXC || GXclrtmp[0][i] == GX_CC_TEXA)) GXclrtmp[0][i] = GX_CC_ZERO;
		if (!tex_en && (GXalphatmp[0][i] == GX_CA_TEXA)) GXalphatmp[0][i] = GX_CC_ZERO;
	}

/*   	GX_SetNumChans (1);
	if (tex_en)	GX_SetNumTexGens (1);
	else GX_SetNumTexGens (0);
	GX_SetNumTevStages (2);
//Set Tevstage 0 -> A-B
	if (tex_en) GX_SetTevOrder (GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0); // change to (u8) tile later
	else GX_SetTevOrder (GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
//	GX_SetTevColorIn (GX_TEVSTAGE0, GXtevcolorarg[0][1], GX_CC_ZERO, GX_CC_ZERO, GXtevcolorarg[0][0]);
	GX_SetTevColorIn (GX_TEVSTAGE0, GXclrtmp[0][1], GX_CC_ZERO, GX_CC_ZERO, GXclrtmp[0][0]);
	GX_SetTevColorOp (GX_TEVSTAGE0, GX_TEV_SUB, GX_TB_ZERO, GX_CS_SCALE_1, GX_DISABLE, GX_TEVPREV);
//	GX_SetTevAlphaIn (GX_TEVSTAGE0, GXtevalphaarg[0][1], GX_CA_ZERO, GX_CA_ZERO, GXtevalphaarg[0][0]);
	GX_SetTevAlphaIn (GX_TEVSTAGE0, GXalphatmp[0][1], GX_CA_ZERO, GX_CA_ZERO, GXalphatmp[0][0]);
	GX_SetTevAlphaOp (GX_TEVSTAGE0, GX_TEV_SUB, GX_TB_ZERO, GX_CS_SCALE_1, GX_DISABLE, GX_TEVPREV);
//Set Tevstage 1 -> Tevprev*C+D
	if (tex_en) GX_SetTevOrder (GX_TEVSTAGE1, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0); // change to (u8) tile later
	else GX_SetTevOrder (GX_TEVSTAGE1, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
//	GX_SetTevColorIn (GX_TEVSTAGE1, GX_CC_ZERO, GX_CC_CPREV, GXtevcolorarg[0][2], GXtevcolorarg[0][3]);
	GX_SetTevColorIn (GX_TEVSTAGE1, GX_CC_ZERO, GX_CC_CPREV, GXclrtmp[0][2], GXclrtmp[0][3]);
	GX_SetTevColorOp (GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV);
//	GX_SetTevAlphaIn (GX_TEVSTAGE1, GX_CA_ZERO, GX_CA_APREV, GXtevalphaarg[0][2], GXtevalphaarg[0][3]);
	GX_SetTevAlphaIn (GX_TEVSTAGE1, GX_CA_ZERO, GX_CA_APREV, GXalphatmp[0][2], GXalphatmp[0][3]);
	GX_SetTevAlphaOp (GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV);*/


// Combine1: (A-B)C+D
//Tevstage 0 -> A*C
//Tevstage 1 -> Tevprev(inputD) - B*C
//Tevstage 2 -> Tevprev(inputD) + D

	GX_SetNumChans (1);
	if (tex_en)	GX_SetNumTexGens (1);
	else GX_SetNumTexGens (0);
	GX_SetNumTevStages (3);
//Set Tevstage 0 -> A*C
//	if (tex_en && ((colorSrc[0][0]==1)||(colorSrc[0][0]==2)) GX_SetTevOrder (GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0); // change to (u8) tile later
	if (tex_en) GX_SetTevOrder (GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0); // change to (u8) tile later
	else GX_SetTevOrder (GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
	GX_SetTevColorIn (GX_TEVSTAGE0, GX_CC_ZERO, GXclrtmp[0][0], GXclrtmp[0][2], GX_CC_ZERO);
	GX_SetTevColorOp (GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV);
	GX_SetTevAlphaIn (GX_TEVSTAGE0, GX_CA_ZERO, GXalphatmp[0][0], GXalphatmp[0][2], GX_CA_ZERO);
	GX_SetTevAlphaOp (GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV);
//Set Tevstage 1 -> Tevprev(inputD) - B*C
	if (tex_en) GX_SetTevOrder (GX_TEVSTAGE1, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0); // change to (u8) tile later
	else GX_SetTevOrder (GX_TEVSTAGE1, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
	GX_SetTevColorIn (GX_TEVSTAGE1, GX_CC_ZERO, GXclrtmp[0][1], GXclrtmp[0][2], GX_CC_CPREV);
	GX_SetTevColorOp (GX_TEVSTAGE1, GX_TEV_SUB, GX_TB_ZERO, GX_CS_SCALE_1, GX_DISABLE, GX_TEVPREV);
	GX_SetTevAlphaIn (GX_TEVSTAGE1, GX_CA_ZERO, GXalphatmp[0][1], GXalphatmp[0][2], GX_CA_APREV);
	GX_SetTevAlphaOp (GX_TEVSTAGE1, GX_TEV_SUB, GX_TB_ZERO, GX_CS_SCALE_1, GX_DISABLE, GX_TEVPREV);
//Set Tevstage 2 -> Tevprev(inputD) + D
	if (tex_en) GX_SetTevOrder (GX_TEVSTAGE2, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0); // change to (u8) tile later
	else GX_SetTevOrder (GX_TEVSTAGE2, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
	GX_SetTevColorIn (GX_TEVSTAGE2, GXclrtmp[0][3], GX_CC_ZERO, GX_CC_ZERO, GX_CC_CPREV);
	GX_SetTevColorOp (GX_TEVSTAGE2, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV);
	GX_SetTevAlphaIn (GX_TEVSTAGE2, GXalphatmp[0][3], GX_CA_ZERO, GX_CA_ZERO, GX_CA_APREV);
	GX_SetTevAlphaOp (GX_TEVSTAGE2, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV);



//GXSetTevColor
//GXSetTevClampMode - fn doesn't exist
//GXSetAlphaCompare
}

void CC::combine2(u8 tile0, u8 tile1, bool tex_en)
{
/*   texel0 = texela;
   texel0Alpha = Color32(texel0.getAlpha(), texel0.getAlpha(), texel0.getAlpha(), texel0.getAlpha());
   texel1 = texelb;
   texel1Alpha = Color32(texel1.getAlpha(), texel1.getAlpha(), texel1.getAlpha(), texel1.getAlpha());
   
   combined = (*pa0 - *pb0)* *pc0 + *pd0;
   float Ac0 = *pAc0 / 255.0f;
   combined.setAlpha((Ac0 * (*pAa0 - *pAb0)) + *pAd0);
   
   Color32 c = (*pa1 - *pb1)* *pc1 + *pd1;
   float Ac1 = *pAc1 / 255.0f;
   c.setAlpha((Ac1 * (*pAa1 - *pAb1)) + *pAd1);
   
   return c;*/

// Combine2: (A-B)C+D
// Combine1: (A-B)C+D
//Tevstage 3 -> A-B
//Tevstage 2 -> Tevprev*C + D -> GX_TEVREG0
//Tevstage 1 -> A-B
//Tevstage 0 -> Tevprev*C + D

/*   	GX_SetNumChans (1);
	GX_SetNumTexGens (1); //change for 2 textures
	GX_SetNumTevStages (4);
//Set Tevstage 3 -> A-B
	GX_SetTevOrder (GX_TEVSTAGE3, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0); // change to (u8) tile1 later
	GX_SetTevColorIn (GX_TEVSTAGE3, GXtevcolorarg[1][1], GX_CC_ZERO, GX_CC_ZERO, GXtevcolorarg[1][0]);
	GX_SetTevColorOp (GX_TEVSTAGE3, GX_TEV_SUB, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV);
	GX_SetTevAlphaIn (GX_TEVSTAGE3, GXtevalphaarg[1][1], GX_CA_ZERO, GX_CA_ZERO, GXtevalphaarg[1][0]);
	GX_SetTevAlphaOp (GX_TEVSTAGE3, GX_TEV_SUB, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV);
//Set Tevstage 2 -> Tevprev*C+D -> GX_TEVREG0
	GX_SetTevOrder (GX_TEVSTAGE2, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0); // change to (u8) tile1 later
	GX_SetTevColorIn (GX_TEVSTAGE2, GX_CC_ZERO, GX_CC_CPREV, GXtevcolorarg[1][2], GXtevcolorarg[1][3]);
	GX_SetTevColorOp (GX_TEVSTAGE2, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVREG0);
	GX_SetTevAlphaIn (GX_TEVSTAGE2, GX_CA_ZERO, GX_CA_APREV, GXtevalphaarg[1][2], GXtevalphaarg[1][3]);
	GX_SetTevAlphaOp (GX_TEVSTAGE2, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVREG0);
//Set Tevstage 1 -> A-B
	GX_SetTevOrder (GX_TEVSTAGE1, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0); // change to (u8) tile0 later
	GX_SetTevColorIn (GX_TEVSTAGE1, GXtevcolorarg[0][1], GX_CC_ZERO, GX_CC_ZERO, GXtevcolorarg[0][0]);
	GX_SetTevColorOp (GX_TEVSTAGE1, GX_TEV_SUB, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV);
	GX_SetTevAlphaIn (GX_TEVSTAGE1, GXtevalphaarg[0][1], GX_CA_ZERO, GX_CA_ZERO, GXtevalphaarg[0][0]);
	GX_SetTevAlphaOp (GX_TEVSTAGE1, GX_TEV_SUB, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV);
//Set Tevstage 0 -> Tevprev*C+D
	GX_SetTevOrder (GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0); // change to (u8) tile0 later
	GX_SetTevColorIn (GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_CPREV, GXtevcolorarg[0][2], GXtevcolorarg[0][3]);
	GX_SetTevColorOp (GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV);
	GX_SetTevAlphaIn (GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_APREV, GXtevalphaarg[0][2], GXtevalphaarg[0][3]);
	GX_SetTevAlphaOp (GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV);*/


// Combine2:	(A0-B0)C0+D0 -> combined
//				(A1-B1)C1+D1 -> color/alpha
//Tevstage 0 -> A0
//Tevstage 1 -> Tevprev - B0
//Tevstage 2 -> Tevprev * C0
//Tevstage 3 -> Tevprev + D0 -> GX_TEVREG0
//Tevstage 4 -> A1
//Tevstage 5 -> Tevprev - B1
//Tevstage 6 -> Tevprev * C1
//Tevstage 7 -> Tevprev + D1

	u8 GXclrtmp[2][4];
	u8 GXalphatmp[2][4];

/*	u8 GXtexcoord0 = GX_TEXCOORD0;
	u8 GXtexcoord1 = GX_TEXCOORD1;
	u8 GXtexmap0 = GX_TEXMAP0;
	u8 GXtexmap1 = GX_TEXMAP1;*/

	for (int i=0;i<4;i++) {
		GXclrtmp[0][i] = GXtevcolorarg[0][i];
		GXclrtmp[1][i] = GXtevcolorarg[1][i];
		GXalphatmp[0][i] = GXtevalphaarg[0][i];
		GXalphatmp[1][i] = GXtevalphaarg[1][i];
		if (!tex_en && (GXclrtmp[0][i] == GX_CC_TEXC || GXclrtmp[0][i] == GX_CC_TEXA)) GXclrtmp[0][i] = GX_CC_ZERO;
		if (!tex_en && (GXclrtmp[1][i] == GX_CC_TEXC || GXclrtmp[1][i] == GX_CC_TEXA)) GXclrtmp[1][i] = GX_CC_ZERO;
		if (!tex_en && (GXalphatmp[0][i] == GX_CA_TEXA)) GXalphatmp[0][i] = GX_CC_ZERO;
		if (!tex_en && (GXalphatmp[1][i] == GX_CA_TEXA)) GXalphatmp[1][i] = GX_CC_ZERO;
	}

	GX_SetNumChans (1);
	if (tex_en)	GX_SetNumTexGens (2); // may need to set this according to # of active textures
	else GX_SetNumTexGens (0);
	GX_SetNumTevStages (8);
//Set Tevstage 0 -> A0
//	if (tex_en && ((colorSrc[0][0]==1)||(colorSrc[0][0]==2)) GX_SetTevOrder (GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0); // change to (u8) tile later
	if (tex_en && texSrc[0][0] == CC_TEX0) GX_SetTevOrder (GX_TEVSTAGE0, (u8) tile0, (u8) tile0, GX_COLOR0A0);
	else if (tex_en && texSrc[0][0] == CC_TEX1) GX_SetTevOrder (GX_TEVSTAGE0, (u8) tile1, (u8) tile1, GX_COLOR0A0);
	else GX_SetTevOrder (GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
	GX_SetTevColorIn (GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GXclrtmp[0][0]);
	GX_SetTevColorOp (GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV);
	GX_SetTevAlphaIn (GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GXalphatmp[0][0]);
	GX_SetTevAlphaOp (GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV);
//Set Tevstage 1 -> Tevprev(inputD) - B0
	if (tex_en && texSrc[0][1] == CC_TEX0) GX_SetTevOrder (GX_TEVSTAGE1, (u8) tile0, (u8) tile0, GX_COLOR0A0);
	else if (tex_en && texSrc[0][1] == CC_TEX1) GX_SetTevOrder (GX_TEVSTAGE1, (u8) tile1, (u8) tile1, GX_COLOR0A0);
	else GX_SetTevOrder (GX_TEVSTAGE1, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
	GX_SetTevColorIn (GX_TEVSTAGE1, GXclrtmp[0][1], GX_CC_ZERO, GX_CC_ZERO, GX_CC_CPREV);
	GX_SetTevColorOp (GX_TEVSTAGE1, GX_TEV_SUB, GX_TB_ZERO, GX_CS_SCALE_1, GX_DISABLE, GX_TEVPREV);
	GX_SetTevAlphaIn (GX_TEVSTAGE1, GXalphatmp[0][1], GX_CA_ZERO, GX_CA_ZERO, GX_CA_APREV);
	GX_SetTevAlphaOp (GX_TEVSTAGE1, GX_TEV_SUB, GX_TB_ZERO, GX_CS_SCALE_1, GX_DISABLE, GX_TEVPREV);
//Set Tevstage 2 -> Tevprev(inputD) * C0
	if (tex_en && texSrc[0][2] == CC_TEX0) GX_SetTevOrder (GX_TEVSTAGE2, (u8) tile0, (u8) tile0, GX_COLOR0A0);
	else if (tex_en && texSrc[0][2] == CC_TEX1) GX_SetTevOrder (GX_TEVSTAGE2, (u8) tile1, (u8) tile1, GX_COLOR0A0);
	else GX_SetTevOrder (GX_TEVSTAGE2, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
	GX_SetTevColorIn (GX_TEVSTAGE2, GX_CC_ZERO, GX_CC_CPREV, GXclrtmp[0][2], GX_CC_ZERO);
	GX_SetTevColorOp (GX_TEVSTAGE2, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_DISABLE, GX_TEVPREV);
	GX_SetTevAlphaIn (GX_TEVSTAGE2, GX_CA_ZERO, GX_CA_APREV, GXalphatmp[0][2], GX_CA_ZERO);
	GX_SetTevAlphaOp (GX_TEVSTAGE2, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_DISABLE, GX_TEVPREV);
//Set Tevstage 3 -> Tevprev(inputD) + D0 -> GX_TEVREG0
	if (tex_en && texSrc[0][3] == CC_TEX0) GX_SetTevOrder (GX_TEVSTAGE3, (u8) tile0, (u8) tile0, GX_COLOR0A0);
	else if (tex_en && texSrc[0][3] == CC_TEX1) GX_SetTevOrder (GX_TEVSTAGE3, (u8) tile1, (u8) tile1, GX_COLOR0A0);
	else GX_SetTevOrder (GX_TEVSTAGE3, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
	GX_SetTevColorIn (GX_TEVSTAGE3, GXclrtmp[0][3], GX_CC_ZERO, GX_CC_ZERO, GX_CC_CPREV);
	GX_SetTevColorOp (GX_TEVSTAGE3, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_DISABLE, GX_TEVREG0);
	GX_SetTevAlphaIn (GX_TEVSTAGE3, GXalphatmp[0][3], GX_CA_ZERO, GX_CA_ZERO, GX_CA_APREV);
	GX_SetTevAlphaOp (GX_TEVSTAGE3, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_DISABLE, GX_TEVREG0);
//Set Tevstage 4 -> A1
//	if (tex_en && ((colorSrc[0][0]==1)||(colorSrc[0][0]==2)) GX_SetTevOrder (GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0); // change to (u8) tile later
	if (tex_en && texSrc[1][0] == CC_TEX0) GX_SetTevOrder (GX_TEVSTAGE4, (u8) tile0, (u8) tile0, GX_COLOR0A0);
	else if (tex_en && texSrc[1][0] == CC_TEX1) GX_SetTevOrder (GX_TEVSTAGE4, (u8) tile1, (u8) tile1, GX_COLOR0A0);
	else GX_SetTevOrder (GX_TEVSTAGE4, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
	GX_SetTevColorIn (GX_TEVSTAGE4, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GXclrtmp[1][0]);
	GX_SetTevColorOp (GX_TEVSTAGE4, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV);
	GX_SetTevAlphaIn (GX_TEVSTAGE4, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GXalphatmp[1][0]);
	GX_SetTevAlphaOp (GX_TEVSTAGE4, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV);
//Set Tevstage 5 -> Tevprev(inputD) - B1
	if (tex_en && texSrc[1][1] == CC_TEX0) GX_SetTevOrder (GX_TEVSTAGE5, (u8) tile0, (u8) tile0, GX_COLOR0A0);
	else if (tex_en && texSrc[1][1] == CC_TEX1) GX_SetTevOrder (GX_TEVSTAGE5, (u8) tile1, (u8) tile1, GX_COLOR0A0);
	else GX_SetTevOrder (GX_TEVSTAGE5, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
	GX_SetTevColorIn (GX_TEVSTAGE5, GXclrtmp[1][1], GX_CC_ZERO, GX_CC_ZERO, GX_CC_CPREV);
	GX_SetTevColorOp (GX_TEVSTAGE5, GX_TEV_SUB, GX_TB_ZERO, GX_CS_SCALE_1, GX_DISABLE, GX_TEVPREV);
	GX_SetTevAlphaIn (GX_TEVSTAGE5, GXalphatmp[1][1], GX_CA_ZERO, GX_CA_ZERO, GX_CA_APREV);
	GX_SetTevAlphaOp (GX_TEVSTAGE5, GX_TEV_SUB, GX_TB_ZERO, GX_CS_SCALE_1, GX_DISABLE, GX_TEVPREV);
//Set Tevstage 6 -> Tevprev(inputD) * C1
	if (tex_en && texSrc[1][2] == CC_TEX0) GX_SetTevOrder (GX_TEVSTAGE6, (u8) tile0, (u8) tile0, GX_COLOR0A0);
	else if (tex_en && texSrc[1][2] == CC_TEX1) GX_SetTevOrder (GX_TEVSTAGE6, (u8) tile1, (u8) tile1, GX_COLOR0A0);
	else GX_SetTevOrder (GX_TEVSTAGE6, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
	GX_SetTevColorIn (GX_TEVSTAGE6, GX_CC_ZERO, GX_CC_CPREV, GXclrtmp[1][2], GX_CC_ZERO);
	GX_SetTevColorOp (GX_TEVSTAGE6, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_DISABLE, GX_TEVPREV);
	GX_SetTevAlphaIn (GX_TEVSTAGE6, GX_CA_ZERO, GX_CA_APREV, GXalphatmp[1][2], GX_CA_ZERO);
	GX_SetTevAlphaOp (GX_TEVSTAGE6, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_DISABLE, GX_TEVPREV);
//Set Tevstage 7 -> Tevprev(inputD) + D1
	if (tex_en && texSrc[1][3] == CC_TEX0) GX_SetTevOrder (GX_TEVSTAGE7, (u8) tile0, (u8) tile0, GX_COLOR0A0);
	else if (tex_en && texSrc[1][3] == CC_TEX1) GX_SetTevOrder (GX_TEVSTAGE7, (u8) tile1, (u8) tile1, GX_COLOR0A0);
	else GX_SetTevOrder (GX_TEVSTAGE7, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
	GX_SetTevColorIn (GX_TEVSTAGE7, GXclrtmp[1][3], GX_CC_ZERO, GX_CC_ZERO, GX_CC_CPREV);
	GX_SetTevColorOp (GX_TEVSTAGE7, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV);
	GX_SetTevAlphaIn (GX_TEVSTAGE7, GXalphatmp[1][3], GX_CA_ZERO, GX_CA_ZERO, GX_CA_APREV);
	GX_SetTevAlphaOp (GX_TEVSTAGE7, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV);


//GXSetTevColor
//GXSetTevClampMode
//GXSetAlphaCompare
}
