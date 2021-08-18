/**
 * Wii64 - Recomp-Cache.h
 * Copyright (C) 2008, 2009, 2010 Mike Slegeir
 * 
 * Interface for allocating/freeing blocks of recompiled code
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

#ifndef RECOMP_CACHE_H
#define RECOMP_CACHE_H

// Hold 10MB worth of recompiled data max on Wii, 7 on GC
#ifdef HW_RVL
#define RECOMP_CACHE_SIZE (10*1024*1024)
#else
#define RECOMP_CACHE_SIZE (7*1024*1024)
#endif

extern unsigned int nextLRU;

// Allocate and free memory to be used for recompiled code
//   Any memory allocated this way can be freed at any time
//   you must check invalid_code before you can access it
void RecompCache_Alloc(unsigned int size, unsigned int address, PowerPC_func* func);
void RecompCache_Realloc(PowerPC_func* func, unsigned int new_size);
void RecompCache_Free(unsigned int addr);
// Update the LRU info of the indicated block
//   (call when the block is accessed)
void RecompCache_Update(PowerPC_func* func);
void RecompCache_Link(PowerPC_func* src_func, PowerPC_instr* src_instr,
                      PowerPC_func* dst_func, PowerPC_instr* dst_instr);
void RecompCache_Init(void);

// Allocate memory from the meta cache
//   This will free from both the recomp and meta caches if capacity is hit
void* MetaCache_Alloc(unsigned int num_bytes);
// Free data from the meta cache
void MetaCache_Free(void* ptr);

#endif
