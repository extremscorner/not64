/**
 * Wii64 - MEM2.h
 * Copyright (C) 2007, 2008, 2009 Mike Slegeir
 * Copyright (C) 2007, 2008, 2009 emu_kidid
 * 
 * MEM2 boundaries for different chunks of memory
 *
 * Wii64 homepage: http://www.emulatemii.com
 * email address: tehpola@gmail.com
 *                emukidid@gmail.com
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


#ifndef MEM2_H
#define MEM2_H

// Define a MegaByte/KiloByte
#define MB (1024*1024)
#define KB (1024)

// MEM2 begins at MEM2_LO, the Starlet's Dedicated Memory begins at MEM2_HI
#define MEM2_LO   ((char*)0x90080000)
#define MEM2_HI   ((char*)0x933E0000)
#define MEM2_SIZE (MEM2_HI - MEM2_LO)

// Testing the xfb in MEM2 (reduce Texture Cache by 2MB to accomodate)
/*#define XFB_SIZE (720*480*2) // XFB_SIZE*2 ~= 1.4MB
#define XFB0_LO	(MEM2_LO)
#define XFB1_LO	(XFB0_LO + XFB_SIZE)
#define XFB_HI	(XFB1_LO + XFB_SIZE)*/

// We want 16MB for our ROM Cache
#define ROMCACHE_SIZE (16*MB)
#define ROMCACHE_LO   (MEM2_LO)
//#define ROMCACHE_LO   (XFB_HI)
#define ROMCACHE_HI   (ROMCACHE_LO + ROMCACHE_SIZE)

// We want 8MB for TLB lut's
#define TLBLUT_SIZE (8*MB)
#define TLBLUT_LO   (ROMCACHE_HI)
#define TLBLUT_HI   (TLBLUT_LO + TLBLUT_SIZE)

// We want 16MB for a Texture Cache
#define TEXCACHE_SIZE (16*MB)
#define TEXCACHE_LO   (TLBLUT_HI)
#define TEXCACHE_HI   (TEXCACHE_LO + TEXCACHE_SIZE)

// We want 1MB for invalid_code
#define INVCODE_SIZE (1*MB)
#define INVCODE_LO   (TEXCACHE_HI)
#define INVCODE_HI   (INVCODE_LO + INVCODE_SIZE)

// We want 256KB for fontFont
#define FONT_SIZE (256*KB)
#define FONT_LO   (INVCODE_HI)
#define FONT_HI   (FONT_LO + FONT_SIZE)

// We want 128KB for flashram
#define FLASHRAM_SIZE (128*KB)
#define FLASHRAM_LO   (FONT_HI)
#define FLASHRAM_HI   (FLASHRAM_LO + FLASHRAM_SIZE)

// We want 32KB for sram
#define SRAM_SIZE (32*KB)
#define SRAM_LO   (FLASHRAM_HI)
#define SRAM_HI   (SRAM_LO + SRAM_SIZE)

// We want 32*4KB for mempacks
#define MEMPACK_SIZE (128*KB)
#define MEMPACK_LO   (SRAM_HI)
#define MEMPACK_HI   (MEMPACK_LO + MEMPACK_SIZE)

// We want 4MB for blocks
#define BLOCKS_SIZE (4*MB)
#define BLOCKS_LO   (MEMPACK_HI)
#define BLOCKS_HI   (BLOCKS_LO + BLOCKS_SIZE)

// We want 4MB for RecompCache metadata
#define RECOMPMETA_SIZE (4*MB)
#define RECOMPMETA_LO   (BLOCKS_HI)
#define RECOMPMETA_HI   (RECOMPMETA_LO + RECOMPMETA_SIZE)

// Unclaimed MEM2
#define UNCLAIMED_SIZE (MEM2_HI - BLOCKS_HI)
#define UNCLAIMED_LO   (BLOCKS_HI)
#define UNCLAIMED_HI   (MEM2_HI)

#define MEM2_USED_SIZE (ROMCACHE_SIZE + TLBLUT_SIZE \
                        + TEXCACHE_SIZE + INVCODE_SIZE \
                        + FONT_SIZE + FLASHRAM_SIZE \
                        + SRAM_SIZE + MEMPACK_SIZE \
                        + BLOCKS_SIZE + RECOMPMETA_SIZE)
#if MEM2_USED_SIZE > (0x933E0000-0x90080000)
#error Too much MEM2 used!
#endif

#endif
