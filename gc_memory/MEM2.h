/* MEM2.h - MEM2 boundaries for different chunks of memory
   by Mike Slegeir for Mupen64-Wii
 */

#ifndef MEM2_H
#define MEM2_H

// Define a MegaByte/KiloByte
#define MB (1024*1024)
#define KB (1024)

// MEM2 begins at MEM2_LO, the Starlet's Dedicated Memory begins at MEM2_HI
#define MEM2_LO   ((char*)0x90080000)
#define MEM2_HI   ((char*)0x933E0000)
#define MEM2_SIZE (MEM2_HI - MEM2_LO)

// We want 16MB for our ROM Cache
#define ROMCACHE_SIZE (16*MB)
#define ROMCACHE_LO   (MEM2_LO)
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

// todo: port new /tehpola r4300 and stuff in the next day or so
#define RECOMPMETA_SIZE (4*MB)

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
