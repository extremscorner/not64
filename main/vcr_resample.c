#include "../config.h"
#ifdef VCR_SUPPORT

#include "vcr_resample.h"

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static const volatile unsigned char Four2Eight[16] =
{
	  0, // 0000 = 00000000
	 17, // 0001 = 00010001
	 34, // 0010 = 00100010
	 51, // 0011 = 00110011
	 68, // 0100 = 01000100
	 85, // 0101 = 01010101
	102, // 0110 = 01100110
	119, // 0111 = 01110111
	136, // 1000 = 10001000
	153, // 1001 = 10011001
	170, // 1010 = 10101010
	187, // 1011 = 10111011
	204, // 1100 = 11001100
	221, // 1101 = 11011101
	238, // 1110 = 11101110
	255  // 1111 = 11111111
};

int
VCR_resample( short **dst, int dst_freq,
              const short *src, int src_freq, int src_bitrate, int src_len )
{
	int dst_len, i;
	float ratio;
	int buf_len;
	short *buf = 0, *left, *right;

	// convert bitrate to 16 bits
	if (src_bitrate != 16)
	{
		buf_len = src_len * (16 / src_bitrate);
		buf = malloc( buf_len );
		if (src_bitrate == 4)
		{
			for (i = 0; i < (src_len*2); i++)
			{
				short s = ((char *)src)[i>>1];
				if (i & 1)
				{
					s &= 0xF0;
					s |= (s >> 4);
					s |= (s << 8);
				}
				else
				{
					s &= 0x0F;
					s |= (s << 4);
					s |= (s << 8);
				}
				((short *)buf)[i] = s;
			}
		}
		else if (src_bitrate == 8)
		{
			for (i = 0; i < src_len; i++)
			{
				short s = ((char *)src)[i];
				s |= (s << 8);
				((short *)buf)[i] = s;
			}
		}
		else
		{
			printf( "[VCR]: resample: Cannot convert sample size from %d to 16 bits\n", src_bitrate );
			free( buf );
			return -1;	// unknown result
		}
		src = buf;
		src_len = buf_len;
	}

	ratio = src_freq / (float)dst_freq;
	dst_len = src_len / ratio;

	// de-interlace
	left = malloc( dst_len>>1 );
	right = malloc( dst_len>>1 );
	for (i = 0; i < (src_len/2); i += 2)
	{
		left[i>>1] = src[i];
		right[i>>1] = src[i+1];
	}
	if (buf)
		free( buf );

	*dst = malloc( dst_len );

	// convert sample rate/re-interlace
/*	// very simple algorithm (nearest sample/sample duplication)
	for (i = 0; i < (dst_len/2); i += 2)
	{
		short l, r;
		int pos = i*ratio;
		l = left[pos];
		r = right[pos];
		(*dst)[i  ] = l;
		(*dst)[i+1] = r;
	}*/

	// linear interpolation
	for (i = 0; i < (dst_len/2); i += 2)
	{
		short l1, l2, r1, r2, l, r;
		float pos = (i/2.0)*ratio;
		l1 = left[(int)pos+0];
		r1 = right[(int)pos+0];
		if (pos+1 < (src_len/2))
		{
			l2 = left[(int)pos+1];
			r2 = right[(int)pos+1];
		}
		else
		{
			l2 = l1;
			r2 = r1;
		}
		pos = pos-((float)(int)pos);
		l = (l1 * (1.0-pos)) + (l2 * pos);
		r = (r1 * (1.0-pos)) + (r2 * pos);
		(*dst)[i  ] = l;
		(*dst)[i+1] = r;
	}

	free( left );
	free( right );

	return dst_len;
}

#endif // VCR_SUPPORT
