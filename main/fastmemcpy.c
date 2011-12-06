/*
 * Copyright (c) 2010-2011 Extrems <metaradil@gmail.com>
 *
 * This file is part of MPlayer CE.
 *
 * MPlayer CE is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * MPlayer CE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with MPlayer CE; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stddef.h>
#include <stdint.h>

static inline void small_memcpy(void *to, const void *from, size_t len)
{
	__asm__ volatile (
		"mtxer  %2\n"
		"lswx   %%r5,%y1\n"
		"stswx  %%r5,%y0\n"
		: "=Z"(*(unsigned char *)to)
		: "Z"(*(const unsigned char *)from), "r"(len)
		: "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12"
	);
}

void *fast_memcpy(void *to, const void *from, size_t len)
{
	void *retval = to;
	int delta;
	
	if (len < 32)
		goto end;
	
	if ((delta = (intptr_t)to & 31)) {
		delta = 32 - delta;
		len -= delta;
		small_memcpy(to, from, delta);
		
		from = (const unsigned char *)from + delta;
		to = (unsigned char *)to + delta;
	}
	
	if (len > 32) {
		size_t lines = len / 32;
		len &= 31;
		
		if ((intptr_t)from & 7) {
			do {
				__asm__ volatile (
					"lwz   %%r5,0(%0)\n"
					"lwz   %%r6,4(%0)\n"
					"lwz   %%r7,8(%0)\n"
					"lwz   %%r8,12(%0)\n"
					"lwz   %%r9,16(%0)\n"
					"lwz   %%r10,20(%0)\n"
					"lwz   %%r11,24(%0)\n"
					"lwz   %%r12,28(%0)\n"
					"dcbz  0,%1\n"
					"stw   %%r5,0(%1)\n"
					"stw   %%r6,4(%1)\n"
					"stw   %%r7,8(%1)\n"
					"stw   %%r8,12(%1)\n"
					"stw   %%r9,16(%1)\n"
					"stw   %%r10,20(%1)\n"
					"stw   %%r11,24(%1)\n"
					"stw   %%r12,28(%1)\n"
					:: "b"(from), "b"(to)
					: "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12"
				);
				
				from = (const unsigned char *)from + 32;
				to = (unsigned char *)to + 32;
			} while (--lines);
		} else {
			do {
				__asm__ volatile (
					"lfd   %%f0,0(%0)\n"
					"lfd   %%f1,8(%0)\n"
					"lfd   %%f2,16(%0)\n"
					"lfd   %%f3,24(%0)\n"
					"dcbz  0,%1\n"
					"stfd  %%f0,0(%1)\n"
					"stfd  %%f1,8(%1)\n"
					"stfd  %%f2,16(%1)\n"
					"stfd  %%f3,24(%1)\n"
					:: "b"(from), "b"(to)
					: "fr0", "fr1", "fr2", "fr3"
				);
				
				from = (const unsigned char *)from + 32;
				to = (unsigned char *)to + 32;
			} while (--lines);
		}
	}
	
end:
	if (len) small_memcpy(to, from, len);
	return retval;
}
