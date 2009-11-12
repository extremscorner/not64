/**
 * Wii64 - Recomp-Cache-Heap.c
 * Copyright (C) 2009 Mike Slegeir
 * 
 * Cache for recompiled code using a heap to maintain LRU order
 *
 * Wii64 homepage: http://www.emulatemii.com
 * email address: tehpola@gmail.com
 *
 *
 * This program is free software; you can redistribute it and/
 * or modify it under the terms of the GNU General Public Li-
 * cence as published by the Free Software Foundation; either
 * version 2 of the Licence, or any later version.
 *
 * This program is distributed in the hope that it will be use-
 * ful, but WITHOUT ANY WARRANTY; without even the implied war-
 * ranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public Licence for more details.
 *
**/

#include <ogc/lwp_heap.h>
#include <stdlib.h>
#include "../gc_memory/MEM2.h"
#include "r4300.h"
#include "ppc/Recompile.h"
#include "Invalid_Code.h"
#include "Recomp-Cache.h"

typedef struct _meta_node {
	unsigned int  addr;
	PowerPC_func* func;
	unsigned int  size;
} CacheMetaNode;

static heap_cntrl* cache = NULL, * meta_cache = NULL;
static int cacheSize = 0;

#define HEAP_CHILD1(i) ((i<<1)+1)
#define HEAP_CHILD2(i) ((i<<1)+2)
#define HEAP_PARENT(i) ((i-1)>>2)

#define INITIAL_HEAP_SIZE (64)
static unsigned int heapSize = 0;
static unsigned int maxHeapSize = 0;
static CacheMetaNode** cacheHeap = NULL;

static void heapSwap(int i, int j){
	CacheMetaNode* t = cacheHeap[i];
	cacheHeap[i] = cacheHeap[j];
	cacheHeap[j] = t;
}

static void heapUp(int i){
	// While the given element is out of order
	while(i && cacheHeap[i]->func->lru < cacheHeap[HEAP_PARENT(i)]->func->lru){
		// Swap the child with its parent
		heapSwap(i, HEAP_PARENT(i));
		// Consider its new position
		i = HEAP_PARENT(i);
	}
}

static void heapDown(int i){
	// While the given element is out of order
	while(1){
		unsigned int lru = cacheHeap[i]->func->lru;
		CacheMetaNode* c1 = (HEAP_CHILD1(i) < heapSize) ?
		                     cacheHeap[HEAP_CHILD1(i)] : NULL;
		CacheMetaNode* c2 = (HEAP_CHILD2(i) < heapSize) ?
		                     cacheHeap[HEAP_CHILD2(i)] : NULL;
		// Check against the children, swapping with the min if parent isn't
		if(c1 && lru > c1->func->lru &&
		   (!c2 || c1->func->lru < c2->func->lru)){
			heapSwap(i, HEAP_CHILD1(i));
			i = HEAP_CHILD1(i);
		} else if(c2 && lru > c2->func->lru){
			heapSwap(i, HEAP_CHILD2(i));
			i = HEAP_CHILD2(i);
		} else break;
	}
}

static void heapify(void){
	int i;
	for(i=1; i<heapSize; ++i) heapUp(i);
}

static void heapPush(CacheMetaNode* node){
	if(heapSize == maxHeapSize){
		maxHeapSize = 3*maxHeapSize/2 + 10;
		cacheHeap = realloc(cacheHeap, maxHeapSize*sizeof(void*));
	}
	// Simply add it to the end of the heap
	// No need to heapUp since its the most recently used
	cacheHeap[heapSize++] = node;
}

static CacheMetaNode* heapPop(void){
	heapSwap(0, --heapSize);
	heapDown(0);
	return cacheHeap[heapSize];
}

static void free_func(PowerPC_func* func, unsigned int addr){
	// Free the code associated with the func
	__lwp_heap_free(cache, func->code);
	__lwp_heap_free(meta_cache, func->code_addr);
	// Remove any holes into this func
	PowerPC_func_hole_node* hole, * next;
	for(hole = func->holes; hole != NULL; hole = next){
		next = hole->next;
		free(hole);
	}

	// Remove any pointers to this code
	PowerPC_block* block = blocks[addr>>12];
	remove_func(&block->funcs, func);
	free(func);
}

static inline void update_lru(PowerPC_func* func){
	static unsigned int nextLRU = 0;
	/*if(func->lru != nextLRU-1)*/ func->lru = nextLRU++;

	if(!nextLRU){
		// Handle nextLRU overflows
		// By heap-sorting and assigning new LRUs
		heapify();
		// Since you can't do an in-place min-heap ascending-sort
		//   I have to create a new heap
		CacheMetaNode** newHeap = malloc(maxHeapSize * sizeof(CacheMetaNode*));
		int i, savedSize = heapSize;
		for(i=0; heapSize > 0; ++i){
			newHeap[i] = heapPop();
			newHeap[i]->func->lru = i;
		}
		free(cacheHeap);
		cacheHeap = newHeap;

		nextLRU = heapSize = savedSize;
	}
}

static void release(int minNeeded){
	// Frees alloc'ed blocks so that at least minNeeded bytes are available
	int toFree = minNeeded * 2; // Free 2x what is needed
	// Restore the heap properties to pop the LRU
	heapify();
	// Release nodes' memory until we've freed enough
	while(toFree > 0 && cacheSize){
		// Pop the LRU to be freed
		CacheMetaNode* n = heapPop();
		// Free the function it contains
		free_func(n->func, n->addr);
		toFree    -= n->size;
		cacheSize -= n->size;
		// And the cache node itself
		free(n);
	}
}

void RecompCache_Alloc(unsigned int size, unsigned int address, PowerPC_func* func){
	CacheMetaNode* newBlock = malloc( sizeof(CacheMetaNode) );
	newBlock->addr = address;
	newBlock->size = size;
	newBlock->func = func;

	// Allocate new memory for this code
	void* code = __lwp_heap_allocate(cache, size);
	while(!code){
		release(size);
		code = __lwp_heap_allocate(cache, size);
	}
	int num_instrs = ((func->end_addr ? func->end_addr : 0x10000) -
                      func->start_addr) >> 2;
	void* code_addr = __lwp_heap_allocate(meta_cache, num_instrs * sizeof(void*));
	while(!code_addr){
		release(num_instrs * sizeof(void*));
		code_addr = __lwp_heap_allocate(meta_cache, num_instrs * sizeof(void*));
	}

	cacheSize += size;
	newBlock->func->code = code;
	newBlock->func->code_addr = code_addr;
	// Add it to the heap
	heapPush(newBlock);
	// Make this function the LRU
	update_lru(func);
}

void RecompCache_Realloc(PowerPC_func* func, unsigned int new_size){
	// There should be no need for the code to be preserved
	__lwp_heap_free(cache, func->code);
	func->code = __lwp_heap_allocate(cache, new_size);
	while(!func->code){
		release(new_size);
		func->code = __lwp_heap_allocate(cache, new_size);
	}

	// Update the size for the cache
	int i;
	for(i=heapSize-1; i>=0; --i){
		if(cacheHeap[i]->func == func){
			cacheSize += new_size - cacheHeap[i]->size;
			cacheHeap[i]->size = new_size;
			break;
		}
	}
}

void RecompCache_Free(unsigned int addr){
	int i;
	CacheMetaNode* n = NULL;
	// Find the corresponding node
	for(i=heapSize-1; i>=0; --i){
		if(cacheHeap[i]->addr == addr){
			n = cacheHeap[i];
			// Remove from the heap
			heapSwap(i, --heapSize);
			// Free n's func
			free_func(n->func, addr);
			cacheSize -= n->size;
			// Free the cache node
			free(n);
			return;
		}
	}
}

void RecompCache_Update(PowerPC_func* func){
	update_lru(func);
}

void RecompCache_Init(void){
	if(!cache){
		cache = malloc(sizeof(heap_cntrl));
		__lwp_heap_init(cache, malloc(RECOMP_CACHE_SIZE),
		                RECOMP_CACHE_SIZE, 32);
	}
	
	if(!meta_cache){
		meta_cache = malloc(sizeof(heap_cntrl));
#ifdef HW_RVL		
		__lwp_heap_init(meta_cache, RECOMPMETA_LO,
		                RECOMPMETA_SIZE, 32);
#else
    __lwp_heap_init(meta_cache, malloc(1*1024*1024),
		                1*1024*1024, 32);
#endif
	}
}

