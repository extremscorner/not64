/* ARAM.h - This is the ARAM manager
   by Mike Slegeir for Mupen64-GC
 */

#ifndef ARAM_H
#define ARAM_H

#define BLOCK_SIZE (1024 * 1024)

void ARAM_manager_init(void);
void ARAM_manager_deinit(void);

// Returns the number of available blocks
int ARAM_block_available(void);
int ARAM_block_available_contiguous(void);

// Allocs/Frees blocks in aram
// ptr [in/out]: the pointer to the new ARAM_block pointer, e.g. &ROM_blocks[rom_offset>>20]
// owner [in]: a char unique to the owner of the block
// [out]: the address of the new block
char* ARAM_block_alloc(unsigned char** ptr, unsigned char owner);
char* ARAM_block_alloc_contiguous(unsigned char** ptr, unsigned char owner, unsigned int num_blocks);
void ARAM_block_free(unsigned char** ptr);
void ARAM_block_free_contiguous(unsigned char** ptr, unsigned int num_blocks);

// Finds the least recently used block of the specified owner
unsigned char** ARAM_block_LRU(unsigned char owner);
void ARAM_block_update_LRU(unsigned char** ptr);

#endif

