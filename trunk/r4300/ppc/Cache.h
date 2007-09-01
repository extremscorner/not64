/* Cache.h - Helps control the memory used by initialized blocks
   by Mike Slegeir for Mupen64-GC
 */

#ifndef CACHE_H
#define CACHE_H

extern unsigned char lru[0x100000];
extern char inited_block[0x100000];
extern int  num_inited_blocks;

// Basically just inits block, sets state, and garbage collects
void new_block(PowerPC_block* block);
// Call to explicity, properly deinit the block
void delete_block(PowerPC_block* block);

#endif
