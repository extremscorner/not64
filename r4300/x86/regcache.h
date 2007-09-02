/**
 * Mupen64 - regcache.h
 * Copyright (C) 2002 Hacktarux
 *
 * Mupen64 homepage: http://mupen64.emulation64.com
 * email address: hacktarux@yahoo.fr
 * 
 * If you want to contribute to the project please contact
 * me first (maybe someone is already making what you are
 * planning to do).
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
 * You should have received a copy of the GNU General Public
 * Licence along with this program; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139,
 * USA.
 *
**/

#ifndef REGCACHE_H
#define REGCACHE_H

#include "../recomp.h"

void init_cache(precomp_instr* start);
void free_all_registers();
void free_register(int reg);
int allocate_register(unsigned long *addr);
int allocate_64_register1(unsigned long *addr);
int allocate_64_register2(unsigned long *addr);
int is64(unsigned long *addr);
void build_wrapper(precomp_instr*, unsigned char*, precomp_block*);
void build_wrappers(precomp_instr*, int, int, precomp_block*);
int lru_register();
int allocate_register_w(unsigned long *addr);
int allocate_64_register1_w(unsigned long *addr);
int allocate_64_register2_w(unsigned long *addr);
void set_register_state(int reg, unsigned long *addr, int dirty);
void set_64_register_state(int reg1, int reg2, unsigned long *addr, int dirty);
void lock_register(int reg);
void unlock_register(int reg);
void allocate_register_manually(int reg, unsigned long *addr);
void allocate_register_manually_w(int reg, unsigned long *addr, int load);
void force_32(int reg);
int lru_register_exc1(int exc1);
void simplify_access();

#endif // REGCACHE_H
