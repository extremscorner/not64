/**
 * Wii64 - ARAM-blocks.h
 * Copyright (C) 2007, 2008, 2009 emu_kidid
 * 
 * ARAM cache version of blocks array for gamecube
 *
 * Wii64 homepage: http://www.emulatemii.com
 * email address:  emukidid@gmail.com
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

#ifndef ARAM_BLOCKS_H
#define ARAM_BLOCKS_H

#include <gctypes.h>
#include "ppc/Recompile.h"

#ifdef ARAM_BLOCKCACHE

PowerPC_block* blocks_get(u32 addr);
void blocks_set(u32 addr, PowerPC_block* ptr);

#else

inline PowerPC_block* blocks_get(u32 addr);
inline void blocks_set(u32 addr, PowerPC_block* ptr);

#endif
#endif

