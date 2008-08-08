/* TLB-Cache.h - This is how the TLB LUT should be accessed, this way it won't waste RAM
   by Mike Slegeir for Mupen64-GC
 */

#ifndef TLB_CACHE_H
#define TLB_CACHE_H

#ifdef USE_TLB_CACHE

// Num Slots must be a power of 2!
#define TLB_NUM_SLOTS 64

// The amount of bits required to represent a page
//   number, don't change. Required for hash calc
#define TLB_BITS_PER_PAGE_NUM 20

typedef struct node {
	unsigned int value;
	unsigned int page;
	struct node* next;
} TLB_hash_node;

void TLBCache_init(void);
void TLBCache_deinit(void);

unsigned int inline TLBCache_get_r(unsigned int page);
unsigned int inline TLBCache_get_w(unsigned int page);

void inline TLBCache_set_r(unsigned int page, unsigned int val);
void inline TLBCache_set_w(unsigned int page, unsigned int val);

#endif

#endif

