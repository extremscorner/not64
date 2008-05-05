/* Recomp-Cache.c - Allocating/freeing blocks of recompiled code
   by Mike Slegeir for Mupen64-GC
 */

#include <stdlib.h>
#include "../gui/DEBUG.h"
#include "r4300.h"
#include "Invalid_Code.h"
#include "Recomp-Cache.h"

// How much memory is currently allocated
static unsigned int cacheSize;

typedef struct _meta_node {
	unsigned int blockNum;
	void*        memory;
	unsigned int size;
	struct _meta_node* next;
	struct _meta_node* prev;
} CacheMetaNode;

static CacheMetaNode* head, * tail;

static inline void nodeAdd(CacheMetaNode* n){
	// Insert this block into front the linked list
	n->next = head;
	n->prev = NULL;
	if(head) head->prev = n;
	head = n;
	if(!tail) tail = n;
}

static inline void nodeRemove(CacheMetaNode* n){
	// Remove n from the linked list
	if(n == head) head = n->next;
	else n->prev->next = n->next;
	if(n == tail) tail = n->prev;
	else n->next->prev = n->prev;
}

static void release(int minNeeded){
	// Frees alloc'ed blocks so that at least minNeeded bytes are available
	CacheMetaNode* n;
	for(n = tail; n != NULL && minNeeded > 0; n = n->prev){
		sprintf(txtbuffer, "Releasing block %05x from RecompCache\n", n->blockNum);
		DEBUG_print(txtbuffer, DBG_USBGECKO);
		nodeRemove(n);
		// Free the block associated with n, and adjust the size accordingly
		free(n->memory);
		cacheSize -= n->size;
		minNeeded -= n->size;
		// Mark this block as invalid_code
		invalid_code_set(n->blockNum, 1);
		// And make sure the corresponding pointer is set to null
		// FIXME: Maybe it'd be better if they gave us the pointer to
		//          adjust directly rather than having to do this crap
#ifdef PPC_DYNAREC
		blocks[n->blockNum]->code = NULL;
#else
		blocks[n->blockNum]->block = NULL;
		if(n->blockNum < 0x80000 || n->blockNum >= 0xc0000){	
			unsigned long paddr;
			paddr = virtual_to_physical_address(n->blockNum<<12, 2);
			if(blocks[paddr>>12])
				blocks[paddr>>12]->block = NULL;
			paddr += 0xFFC;
			if(blocks[paddr>>12])
				blocks[paddr>>12]->block = NULL;
		} else {
			if(n->blockNum >= 0x80000 && n->blockNum < 0xa0000
			   && blocks[n->blockNum+0x20000])
				blocks[n->blockNum+0x20000]->block = NULL;
			if(n->blockNum >= 0xa0000 && n->blockNum < 0xc0000
			   && blocks[n->blockNum-0x20000])
				blocks[n->blockNum-0x20000]->block = NULL;
		}
#endif
		free(n);
	}
		
}

void* RecompCache_Alloc(unsigned int size, unsigned int blockNum){
	sprintf(txtbuffer, "RecompCache_Alloc(%d, %05x)\n", size, blockNum);
	DEBUG_print(txtbuffer, DBG_USBGECKO);
	CacheMetaNode* newBlock = malloc( sizeof(CacheMetaNode) );
	newBlock->blockNum = blockNum;
	newBlock->size = size;
	
	if(cacheSize + size > RECOMP_CACHE_SIZE)
		// Free up at least enough space for it to fit
		release(cacheSize + size - RECOMP_CACHE_SIZE);
	
	// We have the necessary space for this alloc, so just call malloc
	cacheSize += size;
	newBlock->memory = malloc(size);
	// Add it to the head of the linked list
	nodeAdd(newBlock);
	// Return the actual pointer
	return newBlock->memory;
}

void RecompCache_Free(unsigned int blockNum){
	// Remove from the linked list
	CacheMetaNode* n;
	for(n = head; n != NULL; n = n->next)
		if(n->blockNum == n->blockNum){
			nodeRemove(n);
			break;
		}
	if(!n) return;
	// Free n's memory
	free(n->memory);
	free(n);
}

void RecompCache_Update(unsigned int blockNum){
	sprintf(txtbuffer, "RecompCache_Update(%05x)\n", blockNum);
	DEBUG_print(txtbuffer, DBG_USBGECKO);
	// Update any equivalent addresses as well
	unsigned int blockNum2 = 0, blockNum3 = 0;
	if(blockNum < 0x80000 || blockNum >= 0xc0000){
		blockNum2 = virtual_to_physical_address(blockNum<<12, 2);
		blockNum3 = blockNum2 + 0xFFC;
	} else {
		if(blockNum >= 0x80000 && blockNum < 0xa0000)
			blockNum2 = blockNum + 0x20000;
		else if(blockNum >= 0xa0000 && blockNum < 0xc0000)
			blockNum2 = blockNum - 0x20000;
	}
	// We use the linked list to keep track of LRU
	// So move this block to the head of the list
	// Find it
	CacheMetaNode* n;
	for(n = head; n != NULL; n = n->next)
		if(n->blockNum == blockNum || n->blockNum == blockNum2
		   || n->blockNum == blockNum3){
			// Remove it
			nodeRemove(n);
			// Add it back at the head
			nodeAdd(n);
		}
}

