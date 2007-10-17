/* TLB-Cache-hash.c - This is how the TLB LUT should be accessed, using a hash map
   by Mike Slegeir for Mupen64-GC
 */

#include <stdlib.h>
#include <assert.h>
#include "TLB-Cache.h"

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

static TLB_hash_node* TLB_LUT_r[TLB_NUM_SLOTS];
static TLB_hash_node* TLB_LUT_w[TLB_NUM_SLOTS];

static unsigned int TLB_hash_shift;

void TLBCache_init(void){
	unsigned int temp = TLB_NUM_SLOTS;
	while(temp){
		temp >>= 1;
		++TLB_hash_shift;
	}
	TLB_hash_shift = TLB_BITS_PER_PAGE_NUM - TLB_hash_shift;
}

void TLBCache_deinit(void){
	int i;
	TLB_hash_node* node, * next;
	for(i=0; i<TLB_NUM_SLOTS; ++i){
		// Clear r
		for(node = TLB_LUT_r[i]; node != NULL; node = next){
			next = node->next;
			free(node);
		}
		TLB_LUT_r[i] = NULL;
		
		// Clear w
		for(node = TLB_LUT_w[i]; node != NULL; node = next){
			next = node->next;
			free(node);
		}
		TLB_LUT_w[i] = NULL;
	}
}

static unsigned int inline TLB_hash(unsigned int page){
	return page >> TLB_hash_shift;
}

unsigned int inline TLBCache_get_r(unsigned int page){
	TLB_hash_node* node = TLB_LUT_r[ TLB_hash(page) ];
	
	for(; node != NULL; node = node->next)
		if(node->page == page) return node->value;
	
	return 0;
}

unsigned int inline TLBCache_get_w(unsigned int page){
	TLB_hash_node* node = TLB_LUT_w[ TLB_hash(page) ];
	
	for(; node != NULL; node = node->next)
		if(node->page == page) return node->value;
	
	return 0;
}

void inline TLBCache_set_r(unsigned int page, unsigned int val){
	TLB_hash_node* next = TLB_LUT_r[ TLB_hash(page) ];
	
	TLB_hash_node* node = malloc( sizeof(TLB_hash_node) );
	node->page  = page;
	node->value = val;
	node->next  = next;
	TLB_LUT_r[ TLB_hash(page) ] = node;
	
	// Remove any old values for this page from the linked list
	for(; node != NULL; node = node->next)
		if(node->next != NULL && node->next->page == page){
			TLB_hash_node* old_node = node->next;
			node->next = old_node->next;
			free( old_node );
			break;
		}
}

void inline TLBCache_set_w(unsigned int page, unsigned int val){
	TLB_hash_node* next = TLB_LUT_w[ TLB_hash(page) ];
	
	TLB_hash_node* node = malloc( sizeof(TLB_hash_node) );
	node->page  = page;
	node->value = val;
	node->next  = next;
	TLB_LUT_w[ TLB_hash(page) ] = node;
	
	// Remove any old values for this page from the linked list
	for(; node != NULL; node = node->next)
		if(node->next != NULL && node->next->page == page){
			TLB_hash_node* old_node = node->next;
			node->next = old_node->next;
			free( old_node );
			break;
		}
}


