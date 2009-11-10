/**
 * Wii64 - TLB-Cache-hash.c
 * Copyright (C) 2007, 2008, 2009 Mike Slegeir
 * Copyright (C) 2007, 2008, 2009 emu_kidid
 * 
 * This is how the TLB LUT should be accessed, using a hash map
 *
 * Wii64 homepage: http://www.emulatemii.com
 * email address: tehpola@gmail.com
 *                emukidid@gmail.com
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

/* ----------------------------------------------------
   FIXME: This should be profiled to determine the best size,
            currently, the linked lists' length can be up to ~16,000
   ----------------------------------------------------
   MEMORY USAGE:
     STATIC:
     	TLB LUT r: NUM_SLOTS * 4 (currently 256 bytes)
     	TLB LUT w: NUM_SLOTS * 4 (currently 256 bytes)
     HEAP:
     	TLB hash nodes: 2 (r/w) * 12 bytes * O( 2^20 ) entries
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "TLB-Cache.h"
#include "../gui/DEBUG.h"
#include <stdio.h>
#include <zlib.h>

#ifdef USE_TLB_CACHE

static TLB_hash_node* TLB_LUT_r[TLB_NUM_SLOTS];
static TLB_hash_node* TLB_LUT_w[TLB_NUM_SLOTS];

static unsigned int TLB_hash_shift;

void TLBCache_init(void){
  TLBCache_deinit();
	unsigned int temp = TLB_NUM_SLOTS;
	while(temp){
		temp >>= 1;
		++TLB_hash_shift;
	}
	TLB_hash_shift = TLB_BITS_PER_PAGE_NUM - TLB_hash_shift + 1;
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

char* TLBCache_dump(){
	int i;
	DEBUG_print("\n\nTLB Cache r dump:\n", DBG_USBGECKO);
	for(i=0; i<TLB_NUM_SLOTS; ++i){
		sprintf(txtbuffer, "%d\t", i);
		DEBUG_print(txtbuffer, DBG_USBGECKO);
		TLB_hash_node* node = TLB_LUT_r[i];
		for(; node != NULL; node = node->next){
			sprintf(txtbuffer, "%05x,%05x -> ", node->page, node->value);
			DEBUG_print(txtbuffer, DBG_USBGECKO);
		}
		DEBUG_print("\n", DBG_USBGECKO);
	}
	DEBUG_print("\n\nTLB Cache w dump:\n", DBG_USBGECKO);
	for(i=0; i<TLB_NUM_SLOTS; ++i){
		sprintf(txtbuffer, "%d\t", i);
		DEBUG_print(txtbuffer, DBG_USBGECKO);
		TLB_hash_node* node = TLB_LUT_w[i];
		for(; node != NULL; node = node->next){
			sprintf(txtbuffer, "%05x,%05x -> ", node->page, node->value);
			DEBUG_print(txtbuffer, DBG_USBGECKO);
		}
		DEBUG_print("\n", DBG_USBGECKO);
	}
	return "TLB Cache dumped to USB Gecko";
}

void TLBCache_dump_r(gzFile *f)
{
	int i = 0,total=0;
		for(i=0; i<TLB_NUM_SLOTS; ++i){
		TLB_hash_node* node = TLB_LUT_r[i];
		for(; node != NULL; node = node->next){
			total++;	
		}
	}
	gzwrite(f, &total, sizeof(int));
	for(i=0; i<TLB_NUM_SLOTS; ++i){
		TLB_hash_node* node = TLB_LUT_r[i];
		for(; node != NULL; node = node->next){
			gzwrite(f, &node->page, 4);
			gzwrite(f, &node->value, 4);		
		}
	}
}

void TLBCache_dump_w(gzFile *f)
{
	int i = 0,total=0;
	//traverse to get total
	for(i=0; i<TLB_NUM_SLOTS; ++i){
		TLB_hash_node* node = TLB_LUT_w[i];
		for(; node != NULL; node = node->next){
			total++;
		}
	}
	gzwrite(f, &total, sizeof(int));
	for(i=0; i<TLB_NUM_SLOTS; ++i){
		TLB_hash_node* node = TLB_LUT_w[i];
		for(; node != NULL; node = node->next){
			gzwrite(f, &node->page, 4);
			gzwrite(f, &node->value, 4);		
		}
	}
}
#endif
