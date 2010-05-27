/**
 * Wii64 - Register-Cache.h
 * Copyright (C) 2009, 2010 Mike Slegeir
 * 
 * Handle mappings from MIPS to PPC registers
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

#ifndef REGISTER_CACHE_H
#define REGISTER_CACHE_H

// -- GPRs --
typedef struct { int hi, lo; } RegMapping;
typedef enum { MAPPING_NONE, MAPPING_32, MAPPING_64 } RegMappingType;
// Create a mapping for a 32-bit register (reg) to a HW register (returned)
// Loading the register's value if the mapping doesn't already exist
int mapRegister(int reg);
// Create a mapping for a 32-bit register (reg) to a HW register (returned)
// Marking the mapping dirty so that it is stored when flushed
int mapRegisterNew(int reg);
// Create a mapping for a 64-bit register (reg) to 2 HW registers (returned)
// Loading the register's value if the mapping doesn't already exist
RegMapping mapRegister64(int reg);
// Create a mapping for a 64-bit register (reg) to 2 HW registers (returned)
// Marking the mapping dirty so that it is stored when flushed
RegMapping mapRegister64New(int reg);
// Unmap a register (reg) without storing, even if its marked dirty
void invalidateRegister(int reg);
// Unmap a register (reg), storing if dirty
void flushRegister(int reg);
// Return the type of mapping for a register (reg)
// Does not alter mappings in any way
RegMappingType getRegisterMapping(int reg);


// -- FPRs --
// Create a mapping for a FPR (fpr) treated as double or single (dbl)
// Loading the FPR's value if the mapping doesn't already exist
int mapFPR(int fpr, int dbl);
// Create a mapping for a FPR (fpr) treated as double or single (dbl)
// Marking the mapping dirty so that it is stored when flushed
int mapFPRNew(int fpr, int dbl);
// Unmap a FPR (fpr) without storing, even if its marked dirty
void invalidateFPR(int fpr);
// Unmap a FPR (fpr), storing if dirty
void flushFPR(int fpr);


// Unmap all registers, storing any dirty registers
int flushRegisters(void);
// Unmap all registers without storing any
void invalidateRegisters(void);
// Reserve a HW register to be used but not associated with any registers
// When the register is no longer needed, be sure to call unmapRegisterTemp
int mapRegisterTemp(void);
// Frees a previously reserved register
void unmapRegisterTemp(int tmp);
// Temporary FPR management
int mapFPRTemp(void);
void unmapFPRTemp(int tmp);

#endif
