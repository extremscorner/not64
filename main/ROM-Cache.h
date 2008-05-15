/* ROM-Cache.h - This is how the ROM should be accessed, this way the ROM doesn't waste RAM
   by Mike Slegeir for Mupen64-GC
 */

#ifndef ROM_CACHE_H
#define ROM_CACHE_H

#include "../fileBrowser/fileBrowser.h"
#include <gctypes.h>

/* Rom Cache stuff */
// Note: All length/size/offsets are in bytes
void ROMCache_init(u32 romSize);
void ROMCache_deinit();
void ROMCache_read(u32* ram_dest, u32 rom_offset, u32 length);
void ROMCache_load(fileBrowser_file* file, int byteSwap);

/* Byteswapping stuff */
extern int ROM_byte_swap;
void byte_swap(char* buffer, unsigned int length);
#define BYTE_SWAP_BAD -1
#define BYTE_SWAP_NONE 0
#define BYTE_SWAP_HALF 1
#define BYTE_SWAP_BYTE 2



#endif

