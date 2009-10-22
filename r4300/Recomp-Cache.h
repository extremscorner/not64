/* Recomp-Cache.h - Interface for allocating/freeing blocks of recompiled code
   by Mike Slegeir for Mupen64-GC
 */

#ifndef RECOMP_CACHE_H
#define RECOMP_CACHE_H

// Hold 8MB worth of recompiled data max on Wii, 5 on GC
#ifdef HW_RVL
#define RECOMP_CACHE_SIZE (8*1024*1024)
#else
#define RECOMP_CACHE_SIZE (5*1024*1024)
#endif

// Allocate and free memory to be used for recompiled code
//   Any memory allocated this way can be freed at any time
//   you must check invalid_code before you can access it
void RecompCache_Alloc(unsigned int size, unsigned int address, PowerPC_func* func);
void RecompCache_Realloc(PowerPC_func* func, unsigned int new_size);
void RecompCache_Free(unsigned int addr);
// Update the LRU info of the indicated block
//   (call when the block is accessed)
void RecompCache_Update(PowerPC_func* func);

#endif
