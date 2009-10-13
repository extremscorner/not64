/* Recomp-Cache.h - Interface for allocating/freeing blocks of recompiled code
   by Mike Slegeir for Mupen64-GC
 */

#ifndef RECOMP_CACHE_H
#define RECOMP_CACHE_H

// Hold 4MB worth of recompiled data max
#define RECOMP_CACHE_SIZE (4*1024*1024)

// Allocate and free memory to be used for recompiled code
//   Any memory allocated this way can be freed at any time
//   you must check invalid_code before you can access it
void RecompCache_Alloc(unsigned int size, unsigned int address, PowerPC_func* func);
void RecompCache_Realloc(PowerPC_func* func, unsigned int size);
void RecompCache_Free(unsigned int addr);
// Update the LRU info of the indicated block
//   (call when the block is accessed)
void RecompCache_Update(unsigned int addr);

#endif
