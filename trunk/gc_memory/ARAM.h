/**
 * Wii64 - ARAM.h
 * Copyright (C) 2007, 2008, 2009 Mike Slegeir
 * 
 * This is the ARAM manager
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


#ifndef ARAM_H
#define ARAM_H

#define BLOCK_SIZE (256*1024)

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

