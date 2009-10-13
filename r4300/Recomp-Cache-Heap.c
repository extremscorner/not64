

#include <stdlib.h>
#include "r4300.h"
#include "Invalid_Code.h"
#include "Recomp-Cache.h"

typedef struct _meta_node {
	unsigned int  addr;
	PowerPC_func* func;
	unsigned int  size;
} CacheMetaNode;

static int cacheSize;

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
	free(func->code);
	
	// Remove any pointers to this code
	PowerPC_block* block = blocks[addr>>12];
	// Null out the corresponding code_addr entries
	int i;
	int start = (func->start_addr&0xfff)>>2;
	int end = (func->end_addr - func->start_addr)>>2;
	if(end < 0) end = 1024;
	else end += start;
	for(i=start; i<end; ++i)
		block->code_addr[i] = NULL;
	// Remove the function from the linked list
	PowerPC_func_node* fn = block->funcs;
	if(fn && fn->function == func){
		block->funcs = fn->next;
		free(fn->function);
		free(fn);
	} else {
		for(; fn != NULL; fn = fn->next){
			if(fn->next && fn->next->function == func){
				PowerPC_func_node* t = fn->next;
				fn->next = t->next;
				free(t->function);
				free(t);
				break;
			}
		}
	}
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
		for(i=0; i<heapSize; ++i){
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
	while(toFree > 0 || !cacheSize){
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
	
	if(cacheSize + size > RECOMP_CACHE_SIZE)
		// Free up at least enough space for it to fit
		release(cacheSize + size - RECOMP_CACHE_SIZE);
	
	// We have the necessary space for this alloc, so just call malloc
	cacheSize += size;
	newBlock->func->code = malloc(size);
	// Add it to the heap
	heapPush(newBlock);
	// Make this function the LRU
	update_lru(func);
}

void RecompCache_Realloc(PowerPC_func* func, unsigned int size){
	int i;
	CacheMetaNode* n = NULL;
	// Find the corresponding node
	for(i=heapSize-1; !n; --i)
		if(cacheHeap[i]->func == func)
			n = cacheHeap[i];
	// Make this function the LRU
	update_lru(func);
	
	int neededSpace = size - n->size;
	
	if(cacheSize + neededSpace > RECOMP_CACHE_SIZE)
		// Free up at least enough space for it to fit
		release(cacheSize + neededSpace - RECOMP_CACHE_SIZE);
	
	// We have the necessary space for this alloc, so just call malloc
	cacheSize += neededSpace;
	n->func->code = realloc(n->func->code, size);
	n->size = size;
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
			break;
		}
	}
}

void RecompCache_Update(unsigned int addr){
	PowerPC_func_node* n = blocks[addr>>12]->funcs;
	addr &= 0xffff;
	for(; n != NULL; n = n->next){
		if(addr >= n->function->start_addr &&
		   addr <  n->function->end_addr){
			update_lru(n->function);
			break;
		}
	}
}

