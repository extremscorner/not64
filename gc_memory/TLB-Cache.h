/**
 * Wii64 - TLB-Cache.h
 * Copyright (C) 2007, 2008, 2009 Mike Slegeir
 * 
 * This is how the TLB LUT should be accessed, this way it won't waste RAM
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


#ifndef TLB_CACHE_H
#define TLB_CACHE_H

#ifdef USE_TLB_CACHE

#include <zlib.h>

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

// for savestates
void TLBCache_dump_r(gzFile *f);
void TLBCache_dump_w(gzFile *f);

void ARAM_ReadTLBBlock(unsigned int addr, int type);
void ARAM_WriteTLBBlock(unsigned int addr, int type);

#endif

#endif

