/**
 * glN64_GX - 2xSAI.h
 * Copyright (C) 2003 Orkin
 * Copyright (C) 2008, 2009 sepp256 (Port to Wii/Gamecube/PS3)
 * Copyright (C) 2009 tehpola
 *
 * glN64 homepage: http://gln64.emulation64.com
 * Wii64 homepage: http://www.emulatemii.com
 * email address: sepp256@gmail.com
 *
**/

#ifndef _2XSAI_H
#define _2XSAI_H
#include "Types.h"

#define COLORDEPTH32
#ifdef COLORDEPTH32
#include "convert.h"
#endif

struct PixelIterator {
	virtual void operator ++ () = 0;
	virtual void operator += (int) = 0;
	virtual u32 operator [] (int) = 0;
	virtual void set(int offset, u32 value) = 0;
};


struct Interpolator {
	virtual u16 getTileH(u16 height) = 0;
	virtual u16 getTileW(u16 width) = 0;
	virtual bool getSkipTile(){ return false; }
	virtual u32 interpolate(u32, u32);
	virtual u32 interpolate(u32, u32, u32, u32);
	virtual PixelIterator* iterator(void*) = 0;
protected:
	virtual u32 getHigh1(u32 color) = 0;
	virtual u32 getLow1(u32 color) = 0;
	virtual u32 getHigh2(u32 color) = 0;
	virtual u32 getLow2(u32 color) = 0;
	virtual u32 getZ(u32, u32, u32, u32){ return 0; }
};

class Interpolator4444 : public Interpolator {
public:
	u16 getTileH(u16 height){ return height; }
	u16 getTileW(u16 width){ return width; }
	PixelIterator* iterator(void*);
protected:
	u32 getHigh1(u32 color){ return color & 0xEEEE; }
	u32 getLow1(u32 color){ return color & 0x1111; }
	u32 getHigh2(u32 color){ return color & 0xCCCC; }
	u32 getLow2(u32 color){ return color & 0x3333; }
};

class Interpolator5551 : public Interpolator {
public:
	u16 getTileH(u16 height){ return height; }
	u16 getTileW(u16 width){ return width; }
	PixelIterator* iterator(void*);
protected:
	u32 getHigh1(u32 color){ return color & 0xF7BC; }
	u32 getLow1(u32 color){ return color & 0x0843; }
	u32 getHigh2(u32 color){ return color & 0xE738; }
	u32 getLow2(u32 color){ return color & 0x18C6; }
	u32 getZ(u32 A, u32 B, u32 C, u32 D){
		return ((A & 1) + (B & 1) + (C & 1) + (D & 1)) > 2 ? 1 : 0;
	}
};

class Interpolator8888 : public Interpolator {
public:
	u16 getTileH(u16 height){ return height; }
	u16 getTileW(u16 width){ return width; }
	PixelIterator* iterator(void*);
protected:
	u32 getHigh1(u32 color){ return color & 0xFEFEFEFE; }
	u32 getLow1(u32 color){ return color & 0x01010101; }
	u32 getHigh2(u32 color){ return color & 0xFCFCFCFC; }
	u32 getLow2(u32 color){ return color & 0x03030303; }
};

#ifdef __GX__
class InterpolatorGXIA4 : public Interpolator {
public:
	u16 getTileH(u16 height){ return 2; }
	u16 getTileW(u16 width){ return 4; }
	PixelIterator* iterator(void*);
protected:
	u32 getHigh1(u32 color){ return color & 0xEE; }
	u32 getLow1(u32 color){ return color & 0x11; }
	u32 getHigh2(u32 color){ return color & 0xCC; }
	u32 getLow2(u32 color){ return color & 0x33; }
};

class InterpolatorGXIA8 : public Interpolator {
public:
	u16 getTileH(u16 height){ return 2; }
	u16 getTileW(u16 width){ return 2; }
	PixelIterator* iterator(void*);
protected:
	u32 getHigh1(u32 color){ return color & 0xFEFE; }
	u32 getLow1(u32 color){ return color & 0x0101; }
	u32 getHigh2(u32 color){ return color & 0xFCFC; }
	u32 getLow2(u32 color){ return color & 0x0303; }
};

class InterpolatorGXRGB5A3 : public Interpolator {
public:
	u16 getTileH(u16 height){ return 2; }
	u16 getTileW(u16 width){ return 2; }
	u32 interpolate(u32 A, u32 B){
		if(A != B)
		{
#ifdef COLORDEPTH32
			u32 A32 = (A & 0x8000) ? 0xFF000000 | (Five2Eight[(A & 0x7c00) >> 10]<<16) | (Five2Eight[(A & 0x03e0) >> 5]<<8) | (Five2Eight[(A & 0x001f)]) :
				(Four2Eight[(A & 0x0f00) >> 8]<<16) | (Four2Eight[(A & 0x00f0) >> 4]<<8) | (Four2Eight[(A & 0x000f)]);
			u32 B32 = (B & 0x8000) ? 0xFF000000 | (Five2Eight[(B & 0x7c00) >> 10]<<16) | (Five2Eight[(B & 0x03e0) >> 5]<<8) | (Five2Eight[(B & 0x001f)]) :
				(Four2Eight[(B & 0x0f00) >> 8]<<16) | (Four2Eight[(B & 0x00f0) >> 4]<<8) | (Four2Eight[(B & 0x000f)]);
			u32 tempval = (getHigh1(A32) >> 1) + (getHigh1(B32) >> 1) | (getLow1(A32 & B32));
			return (tempval & 0x80000000) ? 0x8000 | ((tempval >> 9) & 0x7c00) | ((tempval >> 6) & 0x03e0) | ((tempval >> 3) & 0x001f) :
				((tempval >> 17) & 0x7000) | ((tempval >> 12) & 0x0f00) | ((tempval >> 8) & 0x00f0) | ((tempval >> 4) & 0x000f);
#else
			u32 tempval = (getHigh1(A) >> 1) + (getHigh1(B) >> 1) | (getLow1(A & B));
			return (tempval & 0x8000) ? tempval : 0x0000 | ((tempval>>3) & 0x0F00) | ((tempval>>2) & 0x00F0) | ((tempval>>1) & 0x000F);
//			return (tempval & 0x8000) ? tempval : (((A|B)&0x8000)>>2) | ((tempval>>3) & 0x0F00) | ((tempval>>2) & 0x00F0) | ((tempval>>1) & 0x000F);
#endif
		}
		else
			return A;
	}
	u32 interpolate(u32 A, u32 B, u32 C, u32 D){
#ifdef COLORDEPTH32
		u32 A32 = (A & 0x8000) ? 0xFF000000 | (Five2Eight[(A & 0x7c00) >> 10]<<16) | (Five2Eight[(A & 0x03e0) >> 5]<<8) | (Five2Eight[(A & 0x001f)]) :
			(Four2Eight[(A & 0x0f00) >> 8]<<16) | (Four2Eight[(A & 0x00f0) >> 4]<<8) | (Four2Eight[(A & 0x000f)]);
		u32 B32 = (B & 0x8000) ? 0xFF000000 | (Five2Eight[(B & 0x7c00) >> 10]<<16) | (Five2Eight[(B & 0x03e0) >> 5]<<8) | (Five2Eight[(B & 0x001f)]) :
			(Four2Eight[(B & 0x0f00) >> 8]<<16) | (Four2Eight[(B & 0x00f0) >> 4]<<8) | (Four2Eight[(B & 0x000f)]);
		u32 C32 = (C & 0x8000) ? 0xFF000000 | (Five2Eight[(C & 0x7c00) >> 10]<<16) | (Five2Eight[(C & 0x03e0) >> 5]<<8) | (Five2Eight[(C & 0x001f)]) :
			(Four2Eight[(C & 0x0f00) >> 8]<<16) | (Four2Eight[(C & 0x00f0) >> 4]<<8) | (Four2Eight[(C & 0x000f)]);
		u32 D32 = (D & 0x8000) ? 0xFF000000 | (Five2Eight[(D & 0x7c00) >> 10]<<16) | (Five2Eight[(D & 0x03e0) >> 5]<<8) | (Five2Eight[(D & 0x001f)]) :
			(Four2Eight[(D & 0x0f00) >> 8]<<16) | (Four2Eight[(D & 0x00f0) >> 4]<<8) | (Four2Eight[(D & 0x000f)]);

		u32 x = (getHigh2(A32) >> 2) +
				(getHigh2(B32) >> 2) +
			    (getHigh2(C32) >> 2) +
		        (getHigh2(D32) >> 2);
		u32 y = getLow2((getLow2(A32) +
				         getLow2(B32) +
			             getLow2(C32) +
		                 getLow2(D32)) >> 2);
		u32 tempval = x | y;
		return (tempval & 0x80000000) ? 0x8000 | ((tempval >> 9) & 0x7c00) | ((tempval >> 6) & 0x03e0) | ((tempval >> 3) & 0x001f) :
			((tempval >> 17) & 0x7000) | ((tempval >> 12) & 0x0f00) | ((tempval >> 8) & 0x00f0) | ((tempval >> 4) & 0x000f);
#else
		u32 x = (getHigh2(A) >> 2) +
				(getHigh2(B) >> 2) +
			    (getHigh2(C) >> 2) +
		        (getHigh2(D) >> 2);
		u32 y = getLow2((getLow2(A) +
				         getLow2(B) +
			             getLow2(C) +
		                 getLow2(D)) >> 2);
		u32 z = getZ(A, B, C, D);
		u32 tempval = x | y;
		return (z > 0x4000) ? tempval | 0x8000 : ((tempval>>3) & 0x0F00) | ((tempval>>2) & 0x00F0) | ((tempval>>1) & 0x000F);
//		return (z > 0x4000) ? tempval | 0x8000 : z | ((tempval>>3) & 0x0F00) | ((tempval>>2) & 0x00F0) | ((tempval>>1) & 0x000F);
#endif
	}
	PixelIterator* iterator(void*);
protected:
#ifdef COLORDEPTH32
	u32 getHigh1(u32 color){ return color & 0xFEFEFEFE; }
	u32 getLow1(u32 color){ return color & 0x01010101; }
	u32 getHigh2(u32 color){ return color & 0xFCFCFCFC; }
	u32 getLow2(u32 color){ return color & 0x03030303; }
#else
	u32 getHigh1(u32 color){ 
		return (color & 0x8000) ? color & 0x7BDE : ((color & 0x0F00)<<3)|((color & 0x00F0)<<2)|((color & 0x000F)<<1); 
	}
	u32 getLow1(u32 color){ return (color & 0x8000) ? color & 0x8421 : 0x0000; }
	u32 getHigh2(u32 color){ 
		return (color & 0x8000) ? color & 0x739C : ((color & 0x0E00)<<3)|((color & 0x00E0)<<2)|((color & 0x000E)<<1); 
	}
	u32 getLow2(u32 color){ 
		return (color & 0x8000) ? color & 0x0C63 : ((color & 0x0100)<<3)|((color & 0x0010)<<2)|((color & 0x0001)<<1); 
	}
	u32 getZ(u32 A, u32 B, u32 C, u32 D){
		return ((A & 0x8000) + (B & 0x8000) + (C & 0x8000) + (D & 0x8000)) >> 2;
	}
#endif
};

class InterpolatorGXRGBA8 : public Interpolator {
public:
	u16 getTileH(u16 height){ return 2; }
	u16 getTileW(u16 width){ return 2; }
	bool getSkipTile(){ return true; }
	PixelIterator* iterator(void*);
protected:
	u32 getHigh1(u32 color){ return color & 0xFEFEFEFE; }
	u32 getLow1(u32 color){ return color & 0x01010101; }
	u32 getHigh2(u32 color){ return color & 0xFCFCFCFC; }
	u32 getLow2(u32 color){ return color & 0x03030303; }
};
#endif //__GX__

void _2xSaI( void *srcPtr, void *destPtr,
             u16 width, u16 height, s32 clampS, s32 clampT,
             Interpolator* interpolator );

#endif

