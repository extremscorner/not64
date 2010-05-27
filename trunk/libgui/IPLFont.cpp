/**
 * Wii64 - IPLFont.cpp
 * Copyright (C) 2009 sepp256
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

#include "IPLFont.h"
#include "../main/wii64config.h"

#ifdef HW_RVL
#include "../gc_memory/MEM2.h"
#endif

namespace menu {

#define FONT_TEX_SIZE_I4 ((512*512)>>1)
#define FONT_SIZE_ANSI (288 + 131072)

#define STRHEIGHT_OFFSET 6

extern "C" void __SYS_ReadROM(void *buf,u32 len,u32 offset);

IplFont::IplFont()
		: frameWidth(640)
{
#ifdef HW_RVL
	fontFont = (unsigned char*)(FONT_LO);
#endif
	memset(fontFont,0,FONT_TEX_SIZE_I4);
	initFont();
}

IplFont::~IplFont()
{
}

void IplFont::initFont()
{
	#ifndef WII
	//lowlevel Qoob Modchip disable for cube
	setIplConfig(6);
	#endif

	void* fontArea = memalign(32,FONT_SIZE_ANSI);
	memset(fontArea,0,FONT_SIZE_ANSI);
	void* packed_data = (void*)(((u32)fontArea+119072)&~31);
	void* unpacked_data = (void*)((u32)fontArea+288);
	__SYS_ReadROM(packed_data,0x3000,0x1FCF00);
	decodeYay0(packed_data,unpacked_data);

	sys_fontheader *fontData = (sys_fontheader*)unpacked_data;

	convertI2toI4((void*)fontFont, (void*)((u32)unpacked_data+fontData->sheet_image), fontData->sheet_width, fontData->sheet_height);
	DCFlushRange(fontFont, FONT_TEX_SIZE_I4);

	for (int i=0; i<256; ++i)
	{
		int c = i;

		if ((c < fontData->first_char) || (c > fontData->last_char)) c = fontData->inval_char;
		else c -= fontData->first_char;

		fontChars.font_size[i] = ((unsigned char*)fontData)[fontData->width_table + c];

		int r = c / fontData->sheet_column;
		c %= fontData->sheet_column;

		fontChars.s[i] = c * fontData->cell_width;
		fontChars.t[i] = r * fontData->cell_height;
	}
	
	fontChars.fheight = fontData->cell_height;

	free(fontArea);
}

void IplFont::setIplConfig(unsigned char c)
{
	//lowlevel Qoob Modchip disable
	volatile unsigned long* exi = (volatile unsigned long*)0xCC006800;
	unsigned long val,addr;
	addr=0xc0000000;
	val = c << 24;
	exi[0] = ((((exi[0]) & 0x405) | 256) | 48);	//select IPL
	//write addr of IPL
	exi[0 * 5 + 4] = addr;
	exi[0 * 5 + 3] = ((4 - 1) << 4) | (1 << 2) | 1;
	while (exi[0 * 5 + 3] & 1);
	//write the ipl we want to send
	exi[0 * 5 + 4] = val;
	exi[0 * 5 + 3] = ((4 - 1) << 4) | (1 << 2) | 1;
	while (exi[0 * 5 + 3] & 1);

	exi[0] &= 0x405;	//deselect IPL
}

typedef struct _yay0header {
	unsigned int id ATTRIBUTE_PACKED;
	unsigned int dec_size ATTRIBUTE_PACKED;
	unsigned int links_offset ATTRIBUTE_PACKED;
	unsigned int chunks_offset ATTRIBUTE_PACKED;
} yay0header;

void IplFont::decodeYay0(void *src, void *dest)
{
	/* Yay0 decompression */
	u32 i,k,link;
	u8 *dest8,*tmp;
	u32 loff,coff,roff;
	u32 size,cnt,cmask,bcnt;
	yay0header *header;

	dest8 = (u8*)dest;
	header = (yay0header*)src;
	size = header->dec_size;
	loff = header->links_offset;
	coff = header->chunks_offset;

	roff = sizeof(yay0header);
	cmask = 0;
	cnt = 0;
	bcnt = 0;

	do {
		if(!bcnt) {
			cmask = *(u32*)((u32)src+roff);
			roff += 4;
			bcnt = 32;
		}

		if(cmask&0x80000000) {
			dest8[cnt++] = *(u8*)((u32)src+coff);
			coff++;
		} else {
			link = *(u16*)((u32)src+loff);
			loff += 2;

			tmp = dest8+(cnt-(link&0x0fff)-1);
			k = link>>12;
			if(k==0) {
				k = (*(u8*)((u32)src+coff))+18;
				coff++;
			} else k += 2;

			for(i=0;i<k;i++) {
				dest8[cnt++] = tmp[i];
			}
		}
		cmask <<= 1;
		bcnt--;
	} while(cnt<size);
}

void IplFont::convertI2toI4(void *dst, void *src, int xres, int yres)
{
	// I4 has 8x8 tiles
	int x, y;
	unsigned char *d = (unsigned char*)dst;
	unsigned char *s = (unsigned char*)src;

	for (y = 0; y < yres; y += 8)
		for (x = 0; x < xres; x += 8)
		{
			int iy, ix;
			for (iy = 0; iy < 8; ++iy, s+=2)
			{
				for (ix = 0; ix < 2; ++ix)
				{
					int v = s[ix];
					*d++ = (((v>>6)&3)<<6) | (((v>>6)&3)<<4) | (((v>>4)&3)<<2) | ((v>>4)&3);
					*d++ = (((v>>2)&3)<<6) | (((v>>2)&3)<<4) | (((v)&3)<<2) | ((v)&3);
				}
			}
		}
}

void IplFont::setVmode(GXRModeObj *rmode)
{
	vmode = rmode;
}

extern "C" char menuActive;

void IplFont::drawInit(GXColor fontColor)
{
	//FixMe: vmode access
	Mtx44 GXprojection2D;
	Mtx GXmodelView2D;

	// Reset various parameters from gfx plugin
	GX_SetCoPlanar(GX_DISABLE);
	GX_SetClipMode(GX_CLIP_ENABLE);
//	GX_SetScissor(0,0,vmode->fbWidth,vmode->efbHeight);
	GX_SetAlphaCompare(GX_ALWAYS,0,GX_AOP_AND,GX_ALWAYS,0);

	guMtxIdentity(GXmodelView2D);
	GX_LoadTexMtxImm(GXmodelView2D,GX_TEXMTX0,GX_MTX2x4);
//	guMtxTransApply (GXmodelView2D, GXmodelView2D, 0.0F, 0.0F, -5.0F);
	GX_LoadPosMtxImm(GXmodelView2D,GX_PNMTX0);
	if(screenMode && menuActive)
		guOrtho(GXprojection2D, 0, 479, -104, 743, 0, 700);
	else if(screenMode == SCREENMODE_16x9_PILLARBOX)
		guOrtho(GXprojection2D, 0, 479, -104, 743, 0, 700);
	else
		guOrtho(GXprojection2D, 0, 479, 0, 639, 0, 700);
	GX_LoadProjectionMtx(GXprojection2D, GX_ORTHOGRAPHIC);
//	GX_SetViewport (0, 0, vmode->fbWidth, vmode->efbHeight, 0, 1);

	GX_SetZMode(GX_DISABLE,GX_ALWAYS,GX_TRUE);

	GX_ClearVtxDesc();
	GX_SetVtxDesc(GX_VA_PTNMTXIDX, GX_PNMTX0);
	GX_SetVtxDesc(GX_VA_TEX0MTXIDX, GX_TEXMTX0);
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
	//set vertex attribute formats here
	GX_SetVtxAttrFmt(GX_VTXFMT1, GX_VA_POS, GX_POS_XYZ, GX_S16, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT1, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
//	GX_SetVtxAttrFmt(GX_VTXFMT1, GX_VA_TEX0, GX_TEX_ST, GX_U16, 7);
	GX_SetVtxAttrFmt(GX_VTXFMT1, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);

	//enable textures
	GX_SetNumChans (1);
//	GX_SetChanCtrl(GX_COLOR0A0,GX_DISABLE,GX_SRC_REG,GX_SRC_VTX,GX_LIGHTNULL,GX_DF_NONE,GX_AF_NONE);
	GX_SetNumTexGens (1);
	GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);

	GX_InvalidateTexAll();
	GX_InitTexObj(&fontTexObj, &fontFont[0], 512, 512, GX_TF_I4, GX_CLAMP, GX_CLAMP, GX_FALSE);
	GX_LoadTexObj(&fontTexObj, GX_TEXMAP0);

	GX_SetTevColor(GX_TEVREG1,fontColor);
//	GX_SetTevKColor(GX_KCOLOR0, fontColor);
//	GX_SetTevKColorSel(GX_TEVSTAGE0,GX_TEV_KCSEL_K0);
//	GX_SetTevKAlphaSel(GX_TEVSTAGE0,GX_TEV_KCSEL_K0_A);

	GX_SetNumTevStages (1);
	GX_SetTevOrder (GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0); // change to (u8) tile later
	GX_SetTevColorIn (GX_TEVSTAGE0, GX_CC_C1, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO);
//	GX_SetTevColorIn (GX_TEVSTAGE0, GX_CC_KONST, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO);
	GX_SetTevColorOp (GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV);
	GX_SetTevAlphaIn (GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_A1, GX_CA_TEXA, GX_CA_ZERO);
	GX_SetTevAlphaOp (GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV);
//	GX_SetTevSwapModeTable(GX_TEV_SWAP1, GX_CH_RED, GX_CH_GREEN, GX_CH_BLUE, GX_CH_RED);
//	GX_SetTevSwapModeTable(GX_TEV_SWAP0, GX_CH_RED, GX_CH_GREEN, GX_CH_BLUE, GX_CH_ALPHA);
//	GX_SetTevSwapMode(GX_TEVSTAGE0, GX_TEV_SWAP0, GX_TEV_SWAP1);

	//set blend mode
	GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR); //Fix src alpha
	GX_SetColorUpdate(GX_ENABLE);
//	GX_SetAlphaUpdate(GX_ENABLE);
//	GX_SetDstAlpha(GX_DISABLE, 0xFF);
	//set cull mode
	GX_SetCullMode (GX_CULL_NONE);

}

void IplFont::setColor(GXColor fontColour)
{
	GX_SetTevColor(GX_TEVREG1, fontColour);
//	GX_SetTevKColor(GX_KCOLOR0, fontColour);
	fontColor.r = fontColour.r;
	fontColor.g = fontColour.g;
	fontColor.b = fontColour.b;
	fontColor.a = fontColour.a;
}

void IplFont::setColor(GXColor* fontColorPtr)
{
	GX_SetTevColor(GX_TEVREG1, *fontColorPtr);
//	GX_SetTevKColor(GX_KCOLOR0, *fontColorPtr);
	fontColor.r = fontColorPtr->r;
	fontColor.g = fontColorPtr->g;
	fontColor.b = fontColorPtr->b;
	fontColor.a = fontColorPtr->a;
}

void IplFont::drawString(int x, int y, char *string, float scale, bool centered)
{
	if(centered)
	{
		int strWidth = 0;
		int strHeight = (fontChars.fheight+STRHEIGHT_OFFSET) * scale;
		char* string_work = string;
//		while(*string_work && (x < back_framewidth))
		while(*string_work)
		{
			unsigned char c = *string_work;
			strWidth += (int) fontChars.font_size[c] * scale;
			string_work++;
		}
//		x0 = (int) MAX(0, (back_framewidth - x)/2);
//		x = (int) (frameWidth - x)/2;
		x = (int) x - strWidth/2;
		y = (int) y - strHeight/2;
//		write_font(x0,y,string,scale);
	}

//	int ox = x;
	
//	while (*string && (x < (ox + back_framewidth)))
	while (*string)
	{
		//blit_char(axfb,whichfb,x, y, *string, norm_blit ? blit_lookup_norm : blit_lookup);
		unsigned char c = *string;
		int i;
		GX_Begin(GX_QUADS, GX_VTXFMT1, 4);
		for (i=0; i<4; i++) {
			int s = (i & 1) ^ ((i & 2) >> 1) ? fontChars.font_size[c] : 1;
			int t = (i & 2) ? fontChars.fheight : 1;
			float s0 = ((float) (fontChars.s[c] + s))/512;
			float t0 = ((float) (fontChars.t[c] + t))/512;
			s = (int) s * scale;
			t = (int) t * scale;
			GX_Position3s16(x + s, y + t, 0);
			GX_Color4u8(fontColor.r, fontColor.g, fontColor.b, fontColor.a);
//			GX_Color4u8(fontState->colour.r, fontState->colour.g, fontState->colour.b, fontState->colour.a);
//			GX_TexCoord2f32(((float) (fontChars.s[c] + s))/512, ((float) (fontChars.t[c] + t))/512);
//			GX_TexCoord2u16(fontChars.s[c] + s, fontChars.t[c] + t);
			GX_TexCoord2f32(s0, t0);
		}
		GX_End();

		x += (int) fontChars.font_size[c] * scale;
		string++;
	}

}

int IplFont::drawStringWrap(int x, int y, char *string, float scale, bool centered, int maxWidth, int lineSpacing)
{
	int numLines = 0;
	int stringWidth = 0;
	int tokenWidth = 0;
	int numTokens = 0;
	char* lineStart = string;
	char* lineStop = string;
	char* stringWork = string;
	char* stringDraw = NULL;

	while(1)
	{
		if(*stringWork == 0) //end of string
		{
			if((stringWidth + tokenWidth <= maxWidth) || (numTokens = 0))
			{
				if (stringWidth + tokenWidth > 0)
				{
					drawString( x, y+numLines*lineSpacing, lineStart, scale, centered);
					numLines++;
				}
				break;
			}
			else
			{
				stringDraw = (char*)malloc(lineStop - lineStart + 1);
				for (int i = 0; i < lineStop-lineStart; i++)
					stringDraw[i] = lineStart[i];
				stringDraw[lineStop-lineStart] = 0;
				drawString( x, y+numLines*lineSpacing, stringDraw, scale, centered);
				free(stringDraw);
				numLines++;
				lineStart = lineStop+1;
				drawString( x, y+numLines*lineSpacing, lineStart, scale, centered);
				numLines++;
				break;
			}
		}

		if((*stringWork == ' ')) //end of token
		{
			if(stringWidth + tokenWidth <= maxWidth)
			{
				stringWidth += tokenWidth;
				numTokens++;
				tokenWidth = 0;
				lineStop = stringWork;
			}
			else
			{
				if (numTokens == 0)	//if the word is wider than maxWidth, just print it
					lineStop = stringWork;

				stringDraw = (char*)malloc(lineStop - lineStart + 1);
				for (int i = 0; i < lineStop-lineStart; i++)
					stringDraw[i] = lineStart[i];
				stringDraw[lineStop-lineStart] = 0;
				drawString( x, y+numLines*lineSpacing, stringDraw, scale, centered);
				free(stringDraw);
				numLines++;

				lineStart = lineStop+1;
				lineStop = lineStart;
				stringWork = lineStart;
				stringWidth = 0;
				tokenWidth = 0;
				numTokens = 0;
				continue;
			}
		}
		tokenWidth += (int) fontChars.font_size[(int)(*stringWork)] * scale;

		stringWork++;
	}

	return numLines;
}

void IplFont::drawStringAtOrigin(char *string, float scale)
{
	int x0, y0, x = 0;
	char* string_work = string;
	while(*string_work)
	{
		unsigned char c = *string_work;
		x += (int) fontChars.font_size[c] * scale;
		string_work++;
	}
	x0 = (int) -x/2;
	y0 = (int) -(fontChars.fheight+STRHEIGHT_OFFSET)*scale/2;
	drawString(x0, y0, string, scale, false);
}

int IplFont::getStringWidth(char *string, float scale)
{
	int strWidth = 0;
	char* string_work = string;
	while(*string_work)
	{
		unsigned char c = *string_work;
		strWidth += (int) fontChars.font_size[c] * scale;
		string_work++;
	}
	return strWidth;
}

int IplFont::getStringHeight(char *string, float scale)
{
	int strHeight = (fontChars.fheight+STRHEIGHT_OFFSET) * scale;

	return strHeight;
}

} //namespace menu 
