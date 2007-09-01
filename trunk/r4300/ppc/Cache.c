/* Cache.c - Helps control the memory used by initialized blocks
   by Mike Slegeir for Mupen64-GC
 */

#include "Cache.h"
#include "PowerPC.h"

// This value can be adjusted to change
// how much memory will be used
#define MAX_INITED_BLOCKS 1024

// TODO: LRU may hold the start number, i.e. every time we execute a block,
//         we increment the count, and store that in the lru[address>>12]
//       Shrink these to 1-bit booleans
unsigned char lru[0x100000];
char inited_block[0x100000];
int  num_inited_blocks;

// TODO: Garbage collect invalid blocks not valid ones if possible
static void garbageCollect(void){
	if(num_inited_blocks > MAX_INITED_BLOCKS){
		// Find least recently used
		int i, max=0, max_i=-1;
		// FIXME: This is O(n), maybe just take random samples?
		for(i=0; i<0x100000; ++i){
			if(max > lru[i]){
				max_i = i;
				max   = lru[i];
			}
		}
		
		// deinit the block
		if(max_i != -1)
			delete_block(blocks[max_i]);
	}
}

void new_block(PowerPC_block* block){
	if(!inited_block[block->start_address>>12]){
		garbageCollect();
		inited_block[block->start_address>>12] = 1;
		++num_inited_blocks;
		init_block(block);
	}
}

void delete_block(PowerPC_block* block){
	if(inited_block[block->start_address>>12]){
		inited_block[block->start_address>>12] = 0;
		--num_inited_blocks;
		deinit_block(block);
	}
}

