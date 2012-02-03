#ifndef __CAST_H__
#define __CAST_H__

#include <gctypes.h>

#define GQR0			912
#define GQR1			913
#define	GQR2			914
#define	GQR3			915
#define	GQR4			916
#define	GQR5			917
#define	GQR6			918
#define	GQR7			919

#define GQR_TYPE_F32	0
#define GQR_TYPE_U8		4
#define GQR_TYPE_U16	5
#define GQR_TYPE_S8		6
#define GQR_TYPE_S16	7

#define GQR_CAST_U8		4
#define GQR_CAST_U16	5
#define GQR_CAST_S8		6
#define GQR_CAST_S16	7

#ifdef __cplusplus
extern "C" {
#endif

#ifdef GEKKO

#define __set_gqr(_reg,_val)	asm volatile("mtspr %0,%1" : : "i"(_reg), "b"(_val))

static inline void CAST_SetGQR2(u32 type,u32 scale)
{
	register u32 val = (((((scale)<<8)|(type))<<16)|(((scale)<<8)|(type)));
	__set_gqr(GQR2,val);
}

static inline void CAST_SetGQR3(u32 type,u32 scale)
{
	register u32 val = (((((scale)<<8)|(type))<<16)|(((scale)<<8)|(type)));
	__set_gqr(GQR3,val);
}

/******************************************************************/
/*																  */
/* cast from int to float										  */
/*																  */
/******************************************************************/

static inline f32 castu8f32(u8 in)
{
	register f32 rval;
	asm("psq_l%X1 %0,%1,1,4" : "=f"(rval) : "m"(in) : "memory");
	return rval;
}

static inline f32 castu16f32(u16 in)
{
	register f32 rval;
	asm("psq_l%X1 %0,%1,1,5" : "=f"(rval) : "m"(in) : "memory");
	return rval;
}

static inline f32 casts8f32(s8 in)
{
	register f32 rval;
	asm("psq_l%X1 %0,%1,1,6" : "=f"(rval) : "m"(in) : "memory");
	return rval;
}

static inline f32 casts16f32(s16 in)
{
	register f32 rval;
	asm("psq_l%X1 %0,%1,1,7" : "=f"(rval) : "m"(in) : "memory");
	return rval;
}

static inline f32 castsfp8f32(s8 in, u32 bits)
{
	register f32 rval;
	register u32 val = ((bits << 8) | GQR_TYPE_S8) << 16;
	__set_gqr(GQR1, val);
	asm("psq_l%X1 %0,%1,1,1" : "=f"(rval) : "m"(in) : "memory");
	return rval;
}

static inline f32 castsfp16f32(s16 in, u32 bits)
{
	register f32 rval;
	register u32 val = ((bits << 8) | GQR_TYPE_S16) << 16;
	__set_gqr(GQR1, val);
	asm("psq_l%X1 %0,%1,1,1" : "=f"(rval) : "m"(in) : "memory");
	return rval;
}

static inline f32 castufp8f32(u8 in, u32 bits)
{
	register f32 rval;
	register u32 val = ((bits << 8) | GQR_TYPE_U8) << 16;
	__set_gqr(GQR1, val);
	asm("psq_l%X1 %0,%1,1,1" : "=f"(rval) : "m"(in) : "memory");
	return rval;
}

static inline f32 castufp16f32(u16 in, u32 bits)
{
	register f32 rval;
	register u32 val = ((bits << 8) | GQR_TYPE_U16) << 16;
	__set_gqr(GQR1, val);
	asm("psq_l%X1 %0,%1,1,1" : "=f"(rval) : "m"(in) : "memory");
	return rval;
}

/******************************************************************/
/*																  */
/* cast from float to int										  */
/*																  */
/******************************************************************/

static inline u8 castf32u8(register f32 in)
{
	u8 rval;
	asm("psq_st%X0 %1,%0,1,4" : "=m"(rval) : "f"(in) : "memory");
	return rval;
}

static inline u16 castf32u16(register f32 in)
{
	u16 rval;
	asm("psq_st%X0 %1,%0,1,5\n" : "=m"(rval) : "f"(in) : "memory");
	return rval;
}

static inline s8 castf32s8(register f32 in)
{
	s8 rval;
	asm("psq_st%X0 %1,%0,1,6" : "=m"(rval) : "f"(in) : "memory");
	return rval;
}

static inline s16 castf32s16(register f32 in)
{
	s16 rval;
	asm("psq_st%X0 %1,%0,1,7" : "=m"(rval) : "f"(in) : "memory");
	return rval;
}

static inline s8 castf32sfp8(register f32 in, u32 bits)
{
	s8 rval;
	register u32 val = (bits << 8) | GQR_TYPE_S8;
	__set_gqr(GQR1, val);
	asm("psq_st%X0 %1,%0,1,1\n" : "=m"(rval) : "f"(in) : "memory");
	return rval;
}

static inline s16 castf32sfp16(register f32 in, u32 bits)
{
	s16 rval;
	register u32 val = (bits << 8) | GQR_TYPE_S16;
	__set_gqr(GQR1, val);
	asm("psq_st%X0 %1,%0,1,1\n" : "=m"(rval) : "f"(in) : "memory");
	return rval;
}

static inline u8 castf32ufp8(register f32 in, u32 bits)
{
	u8 rval;
	register u32 val = (bits << 8) | GQR_TYPE_U8;
	__set_gqr(GQR1, val);
	asm("psq_st%X0 %1,%0,1,1\n" : "=m"(rval) : "f"(in) : "memory");
	return rval;
}

static inline u16 castf32ufp16(register f32 in, u32 bits)
{
	u16 rval;
	register u32 val = (bits << 8) | GQR_TYPE_U16;
	__set_gqr(GQR1, val);
	asm("psq_st%X0 %1,%0,1,1\n" : "=m"(rval) : "f"(in) : "memory");
	return rval;
}

#endif //GEKKO

#ifdef __cplusplus
}
#endif

#endif
