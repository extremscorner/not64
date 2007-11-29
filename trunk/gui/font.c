/*
	font functions	
	modified by sepp256 to work as textures instead of writing to the xfb
*/

#include <gccore.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <ogcsys.h>
#include <string.h>
#include "font.h"
#include <ogc/color.h>

/* Backdrop Frame Width (to avoid writing outside of the background frame) */
u16 back_framewidth = 640;

typedef struct
{
  unsigned short font_type, first_char, last_char, subst_char, ascent_units, descent_units, widest_char_width,
                 leading_space, cell_width, cell_height;
  unsigned long texture_size;
  unsigned short texture_format, texture_columns, texture_rows, texture_width, texture_height, offset_charwidth;
  unsigned long offset_tile, size_tile;
} FONT_HEADER;

typedef struct
{
	u16 s[256], t[256], font_size[256], fheight;
} CHAR_INFO;

unsigned char* GXfontTexture;
static unsigned char fontFont[ 0x40000 ] __attribute__((aligned(32)));

/****************************************************************************
 * YAY0 Decoding
 ****************************************************************************/
/* Yay0 decompression */
void yay0_decode(unsigned char *s, unsigned char *d)
{
	int i, j, k, p, q, cnt;

	i = *(unsigned long *)(s + 4);	  // size of decoded data
	j = *(unsigned long *)(s + 8);	  // link table
	k = *(unsigned long *)(s + 12);	 // byte chunks and count modifiers

	q = 0;					// current offset in dest buffer
	cnt = 0;				// mask bit counter
	p = 16;					// current offset in mask table

	unsigned long r22 = 0, r5;
	
	do
	{
		// if all bits are done, get next mask
		if(cnt == 0)
		{
			// read word from mask data block
			r22 = *(unsigned long *)(s + p);
			p += 4;
			cnt = 32;   // bit counter
		}
		// if next bit is set, chunk is non-linked
		if(r22 & 0x80000000)
		{
			// get next byte
			*(unsigned char *)(d + q) = *(unsigned char *)(s + k);
			k++, q++;
		}
		// do copy, otherwise
		else
		{
			// read 16-bit from link table
			int r26 = *(unsigned short *)(s + j);
			j += 2;
			// 'offset'
			int r25 = q - (r26 & 0xfff);
			// 'count'
			int r30 = r26 >> 12;
			if(r30 == 0)
			{
				// get 'count' modifier
				r5 = *(unsigned char *)(s + k);
				k++;
				r30 = r5 + 18;
			}
			else r30 += 2;
			// do block copy
			unsigned char *pt = ((unsigned char*)d) + r25;
			int i;
			for(i=0; i<r30; i++)
			{
				*(unsigned char *)(d + q) = *(unsigned char *)(pt - 1);
				q++, pt++;
			}
		}
		// next bit in mask
		r22 <<= 1;
		cnt--;

	} while(q < i);
}

/*void untile(unsigned char *dst, unsigned char *src, int xres, int yres)
{
	// 8x8 tiles
	int x, y;
	int t=0;
	for (y = 0; y < yres; y += 8)
		for (x = 0; x < xres; x += 8)
		{
			t = !t;
			int iy, ix;
			for (iy = 0; iy < 8; ++iy, src+=2)
			{
				unsigned char *d = dst + (y + iy) * xres + x;
				for (ix = 0; ix < 2; ++ix)
				{
					int v = src[ix];
					*d++ = ((v>>6)&3);
					*d++ = ((v>>4)&3);
					*d++ = ((v>>2)&3);
					*d++ = ((v)&3);
				}
			}
		}
}*/

void TF_I2toI4(unsigned char *dst, unsigned char *src, int xres, int yres)
{
	// 8x8 tiles
	int x, y;
	unsigned char *d = dst;
//	int t=0;
	for (y = 0; y < yres; y += 8)
		for (x = 0; x < xres; x += 8)
		{
//			t = !t;
			int iy, ix;
			for (iy = 0; iy < 8; ++iy, src+=2)
			{
//				unsigned char *d = dst + ((y + iy) * xres + x) / 2;
				for (ix = 0; ix < 2; ++ix)
				{
					int v = src[ix];
					*d++ = (((v>>6)&3)<<6) | (((v>>6)&3)<<4) | (((v>>4)&3)<<2) | ((v>>4)&3);
					*d++ = (((v>>2)&3)<<6) | (((v>>2)&3)<<4) | (((v)&3)<<2) | ((v)&3);
				}
			}
		}
}

//int font_offset[256], font_size[256], fheight;
CHAR_INFO fontChars;
extern void __SYS_ReadROM(void *buf,u32 len,u32 offset);

void init_font(void)
{
	static unsigned char fontWork[ 0x20000 ] __attribute__((aligned(32)));
	int i;

	// dont read system rom fonts because this breaks on qoob modchip
	__SYS_ReadROM((unsigned char *)&fontFont,0x3000,0x1FCF00);
	//memcpy(&fontFont, &arial, sizeof(arial));
	yay0_decode((unsigned char *)&fontFont, (unsigned char *)&fontWork);
	FONT_HEADER *fnt;

	fnt = ( FONT_HEADER * )&fontWork;

//	GXfontTexture = (unsigned char*) memalign(32,512*256);
//	memcpy(&GXfontTexture, &fontWork[fnt->offset_tile], 512*256);
	
	printf("fontFont addy: %x",fontFont);

//	for (i=0;i<512*256;i++)
//		fontFont[i] = fontWork[fnt->offset_tile + i];
//	memcpy(&fontFont, &fontWork[fnt->offset_tile], 512*256);
	TF_I2toI4((unsigned char*)&fontFont, (unsigned char*)&fontWork[fnt->offset_tile], fnt->texture_width, fnt->texture_height);
	DCFlushRange(fontFont, 512*256);
	//untile((unsigned char*)&fontFont, (unsigned char*)&fontWork[fnt->offset_tile], fnt->texture_width, fnt->texture_height);

	for (i=0; i<256; ++i)
	{
		int c = i;

		if ((c < fnt->first_char) || (c > fnt->last_char)) c = fnt->subst_char;
		else c -= fnt->first_char;

		fontChars.font_size[i] = ((unsigned char*)fnt)[fnt->offset_charwidth + c];

		int r = c / fnt->texture_columns;
		c %= fnt->texture_columns;

		fontChars.s[i] = c * fnt->cell_width;
		fontChars.t[i] = r * fnt->cell_height;
		//font_offset[i] = (r * fnt->cell_height) * fnt->texture_width + (c * fnt->cell_width);
	}
	
	fontChars.fheight = fnt->cell_height;
}

GXTexObj fontTexObj;

void write_font_init_GX(GXColor fontColor)
{
	Mtx44 GXprojection2D;
	Mtx GXmodelView2D;

	guMtxIdentity(GXmodelView2D);
	GX_LoadTexMtxImm(GXmodelView2D,GX_TEXMTX0,GX_MTX2x4);
//	guMtxTransApply (GXmodelView2D, GXmodelView2D, 0.0F, 0.0F, -5.0F);
	GX_LoadPosMtxImm(GXmodelView2D,GX_PNMTX0);
	guOrtho(GXprojection2D, 0, 479, 0, 639, 0, 100);
	GX_LoadProjectionMtx(GXprojection2D, GX_ORTHOGRAPHIC);

	GX_SetZMode(GX_DISABLE,GX_ALWAYS,GX_TRUE);

	GX_ClearVtxDesc();
	GX_SetVtxDesc(GX_VA_PTNMTXIDX, GX_PNMTX0);
	GX_SetVtxDesc(GX_VA_TEX0MTXIDX, GX_TEXMTX0);
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
	//set vertex attribute formats here
	GX_SetVtxAttrFmt(GX_VTXFMT1, GX_VA_POS, GX_POS_XYZ, GX_S16, 0);
//	GX_SetVtxAttrFmt(GX_VTXFMT1, GX_VA_TEX0, GX_TEX_ST, GX_U16, 7);
	GX_SetVtxAttrFmt(GX_VTXFMT1, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);

	//enable textures
	GX_SetNumChans (1);
	GX_SetChanCtrl(GX_COLOR0A0,GX_DISABLE,GX_SRC_REG,GX_SRC_VTX,GX_LIGHTNULL,GX_DF_NONE,GX_AF_NONE);
	GX_SetNumTexGens (1);
	GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);

	GX_InvalidateTexAll();
//	GX_InitTexObj(&fontTexObj, fontFont, 512, 512, GX_TF_I4, GX_MIRROR, GX_MIRROR, GX_FALSE);
	GX_InitTexObj(&fontTexObj, &fontFont, 512, 512, GX_TF_I4, GX_CLAMP, GX_CLAMP, GX_FALSE);
//	GX_InitTexObj(&fontTexObj, GXfontTexture, 512, 512, GX_TF_I4, GX_CLAMP, GX_CLAMP, GX_FALSE);
	GX_LoadTexObj(&fontTexObj, GX_TEXMAP1);

	GX_SetTevColor(GX_TEVREG1,fontColor);
	GX_SetNumTevStages (1);
	GX_SetTevOrder (GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP1, GX_COLOR0A0); // change to (u8) tile later
	GX_SetTevColorIn (GX_TEVSTAGE0, GX_CC_C1, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO);
//	GX_SetTevColorIn (GX_TEVSTAGE0, GX_CC_TEXC, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO);
	GX_SetTevColorOp (GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV);
	GX_SetTevAlphaIn (GX_TEVSTAGE0, GX_CA_TEXA, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
	GX_SetTevAlphaOp (GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV);
	GX_SetTevSwapModeTable(GX_TEV_SWAP0, GX_CH_RED, GX_CH_GREEN, GX_CH_BLUE, GX_CH_RED);
//	GX_SetTevSwapModeTable(GX_TEV_SWAP0, GX_CH_RED, GX_CH_GREEN, GX_CH_BLUE, GX_CH_ALPHA);
	GX_SetTevSwapMode(GX_TEVSTAGE0, GX_TEV_SWAP0, GX_TEV_SWAP0);

	//GX_SetTevOp (GX_TEVSTAGE0, GX_REPLACE);

	//set blend mode
	GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR); //Fix src alpha
	GX_SetColorUpdate(GX_ENABLE);
	GX_SetAlphaUpdate(GX_ENABLE);
	GX_SetDstAlpha(GX_DISABLE, 0xFF);
	//set cull mode
	GX_SetCullMode (GX_CULL_NONE);

}

void write_font(int x, int y, char *string, float scale)
{
	int ox = x;
	
	while (*string && (x < (ox + back_framewidth)))
	{
		//blit_char(axfb,whichfb,x, y, *string, norm_blit ? blit_lookup_norm : blit_lookup);
		unsigned char c = *string;
		int i;
		GX_Begin(GX_QUADS, GX_VTXFMT1, 4);
		for (i=0; i<4; i++) {
			int s = (i & 1) ^ ((i & 2) >> 1) ? fontChars.font_size[c] : 0;
			int t = (i & 2) ? fontChars.fheight : 0;
			float s0 = ((float) (fontChars.s[c] + s))/512;
			float t0 = ((float) (fontChars.t[c] + t))/512;
			s = (int) s * scale;
			t = (int) t * scale;
			GX_Position3s16(x + s, y + t, 0);
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

/*
#define TRANSPARENCY (COLOR_WHITE)

unsigned int blit_lookup_inv[4] = {COLOR_WHITE, COLOR_WHITE, COLOR_WHITE, COLOR_WHITE};
unsigned int blit_lookup[4] = {COLOR_WHITE, COLOR_WHITE, COLOR_WHITE, COLOR_WHITE};
unsigned int blit_lookup_norm[4] = {COLOR_WHITE, COLOR_BLACK, COLOR_BLACK, COLOR_WHITE};

void blit_char(u32 **axfb,int whichfb,int x, int y, unsigned char c, unsigned int *lookup)
{
	unsigned char *fnt = ((unsigned char*)fontFont) + font_offset[c];
	int ay, ax;
	unsigned int llookup;

	for (ay=0; ay<fheight; ++ay)
	{
		int h = (ay + y) * 320;

		for (ax=0; ax<font_size[c]; ax++)
		{
			int v0 = fnt[ax];
			int p = h + (( ax + x ) >> 1);
			unsigned long o = axfb[whichfb][p];

			llookup = lookup[v0];

			if ((v0== 0) && (llookup == TRANSPARENCY))
				llookup = o;

			if ((ax+x) & 1)
			{
				o &= ~0x00FFFFFF;
				o |= llookup & 0x00FFFFFF;
			}
			else
			{
				o &= ~0xFF000000;
				o |= llookup & 0xFF000000;
			}

			axfb[whichfb][p] = o;
		}
		
		fnt += 512;
	}
}

u8 norm_blit = 1;

void write_font(int x, int y, char *string,u32 **axfb,int whichfb)
{
	int ox = x;
	while (*string && (x < (ox + back_framewidth)))
	{
		blit_char(axfb,whichfb,x, y, *string, norm_blit ? blit_lookup_norm : blit_lookup);
		x += font_size[*string];
		string++;
	}
}*/
