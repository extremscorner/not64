/* MEM2.h - MEM2 boundaries for different chunks of memory
   by Mike Slegeir for Mupen64-Wii
 */

#ifndef MEM2_H
#define MEM2_H

// Define a MegaByte
#define MB (1024*1024)

// MEM2 begins at MEM2_LO, the Starlet's Dedicated Memory begins at MEM2_HI
#define MEM2_LO   ((char*)0x90080000)
#define MEM2_HI   ((char*)0x933E0000)
#define MEM2_SIZE (MEM2_HI - MEM2_LO)

// We want 32MB for our ROM Cache
#define ROMCACHE_SIZE (32*MB)
#define ROMCACHE_LO   (MEM2_LO)
#define ROMCACHE_HI   (ROMCACHE_LO + ROMCACHE_SIZE)

// We want 8MB for TLB lut's
#define TLBLUT_SIZE (8*MB)
#define TLBLUT_LO   (ROMCACHE_HI)
#define TLBLUT_HI   (TLBLUT_LO + TLBLUT_SIZE)

// We want 8MB for a Texture Cache
#define TEXCACHE_SIZE (8*MB)
#define TEXCACHE_LO   (TLBLUT_HI)
#define TEXCACHE_HI   (TEXCACHE_LO + TEXCACHE_SIZE)

// Unclaimed MEM2
#define UNCLAIMED_SIZE (MEM2_HI - TEXCACHE_HI)
#define UNCLAIMED_LO   (TEXCACHE_HI)
#define UNCLAIMED_HI   (MEM2_HI)

#endif
