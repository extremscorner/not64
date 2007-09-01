/* ARAM.c - This is the ARAM manager
   by Mike Slegeir for Mupen64-GC
 */

#include <ogc/aram.h>
#include "ARAM.h"

typedef struct {
	unsigned char*  addr;
	unsigned char** ptr;
	unsigned short  lru;
	unsigned char   valid;
	unsigned char   owner;
} ARAM_block;
static ARAM_block* ARAM_blocks = NULL;
static int alloced_blocks;
static int max_blocks;
static int initialized=0;

void ARAM_manager_init(void){
	if(initialized) return;
	
	AR_Init(NULL, 0);
	
	max_blocks = (AR_GetSize() - AR_GetBaseAddress())/BLOCK_SIZE;
	ARAM_blocks = malloc(max_blocks * sizeof(ARAM_block));
	int i, addr = AR_GetBaseAddress();
	for(i=0; i<max_blocks; ++i){
		ARAM_blocks[i].valid = FALSE;
		ARAM_blocks[i].addr  = addr;
		ARAM_blocks[i].ptr   = NULL;
		ARAM_blocks[i].owner = 0;
		addr += BLOCK_SIZE;
	}
	
	alloced_blocks = 0;
	initialized = 1;
	
	printf("Initialized ARAM with %d blocks:", max_blocks);
	for(i=0; i<max_blocks; ++i){
		if(i%4 == 0) printf("\n");
		printf(" %d: 0x%08x", i, ARAM_blocks[i].addr);
	}
	printf("\n");
}

void ARAM_manager_deinit(void){
	AR_Reset();
	if(ARAM_blocks) free(ARAM_blocks);
	ARAM_blocks = NULL;
	initialized = 0;
}

int ARAM_block_available(void){
	if(!initialized) return 0;
	
	int count = 0, i;
	for(i=0; i<max_blocks; ++i)
		if(!ARAM_blocks[i].valid) ++count;
	return count;
}

int ARAM_block_available_contiguous(void){
	if(!initialized) return 0;
	
	int count = 0, max = 0, i;
	for(i=0; i<max_blocks; ++i){
		if(!ARAM_blocks[i].valid) ++count;
		else count = 0;
		if(count > max) max = count;
	}
	return max;
}

char* ARAM_block_alloc(unsigned char** ptr, unsigned char owner){
	if(!initialized || alloced_blocks == max_blocks) return NULL;
	
	int i;
	for(i=0; i<max_blocks; ++i) if(!ARAM_blocks[i].valid) break;
	ARAM_blocks[i].ptr = ptr;
	ARAM_blocks[i].owner = owner;
	ARAM_blocks[i].valid = TRUE;
	ARAM_blocks[i].lru  = 0;
	
	++alloced_blocks;
	
	return *ptr = ARAM_blocks[i].addr;
}

char* ARAM_block_alloc_contiguous(unsigned char** ptr, unsigned char owner, unsigned int num_blocks){
	printf("ARAM_block_alloc_contiguous:\n");
	if(!initialized || alloced_blocks+num_blocks > max_blocks) return NULL;
	
	int count = 0, block, i;
	for(i=0; i<max_blocks; ++i){
		if(!ARAM_blocks[i].valid){
			if(count++ == 0) block = i;
		} else count = 0;
		if(count >= num_blocks) break;
	}
	printf("count = %d, num_blocks = %d, block = %d\n", count, num_blocks, block);
	if(count < num_blocks) return NULL;
	
	for(i=block; i<num_blocks+block; ++i){
		ARAM_blocks[i].ptr = ptr;
		ARAM_blocks[i].owner = owner;
		ARAM_blocks[i].valid = TRUE;
		ARAM_blocks[i].lru  = 0;
		++alloced_blocks;
	}
	
	return *ptr = ARAM_blocks[block].addr;
}

void ARAM_block_free(unsigned char** ptr){
	if(!initialized) return;
	
	int i;
	for(i=0; i<max_blocks; ++i)
		if(ARAM_blocks[i].ptr == ptr){		
			ARAM_blocks[i].valid = FALSE;
			*ptr = NULL;
			--alloced_blocks;
			break;
		}
}

void ARAM_block_free_contiguous(unsigned char** ptr, unsigned int num_blocks){
	if(!initialized) return;
	
	int i;
	for(i=0; i<max_blocks; ++i)
		if(ARAM_blocks[i].ptr == ptr){
			unsigned char owner = ARAM_blocks[i].owner;
			for(; i<i+num_blocks; ++i)
				if(owner == ARAM_blocks[i].owner){
					ARAM_blocks[i].valid = FALSE;
					--alloced_blocks;
				} else break;
			*ptr = NULL;
			break;
		}
}

unsigned char** ARAM_block_LRU(unsigned char owner){
	if(!initialized) return NULL;
	
	char max=0,max_i=-1,i;
	for(i=0; i<max_blocks; ++i)
		if(ARAM_blocks[i].lru >= max &&
		   ARAM_blocks[i].valid && ARAM_blocks[i].owner == owner){
			max = ARAM_blocks[i].lru;
			max_i = i;
		}
	return ARAM_blocks[max_i].ptr;
}

void ARAM_block_update_LRU(unsigned char** ptr){
	if(!initialized) return;
	
	int i;
	for(i=0; i<max_blocks; ++i)
		if(ARAM_blocks[i].ptr == ptr)
			ARAM_blocks[i].lru = 0;
		else
			++ARAM_blocks[i].lru;
}

