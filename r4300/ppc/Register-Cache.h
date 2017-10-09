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
// Create a mapping for a 32-bit register (gpr) to a HW register (returned)
// Loading the register's value if the mapping doesn't already exist
int mapRegister(int gpr);
// Create a mapping for a 32-bit register (gpr) to a HW register (returned)
// Marking the mapping dirty so that it is stored when flushed
int mapRegisterNew(int gpr, int sign);
// Create a mapping for a 64-bit register (gpr) to 2 HW registers (returned)
// Loading the register's value if the mapping doesn't already exist
RegMapping mapRegister64(int gpr);
// Create a mapping for a 64-bit register (gpr) to 2 HW registers (returned)
// Marking the mapping dirty so that it is stored when flushed
RegMapping mapRegister64New(int gpr);
// Unmap a register (gpr) without storing, even if its marked dirty
void invalidateRegister(int gpr);
// Unmap a register (gpr), storing if dirty
void flushRegister(int gpr);
// Return the type of mapping for a register (gpr)
// Does not alter mappings in any way
RegMappingType getRegisterMapping(int gpr);
// Constant Propagation
// Create a mapping for a 32-bit register (gpr) to a HW register (returned)
// The value mapped may have a constant value (constant) to be set later
int mapConstantNew(int gpr, int constant, int sign);
// Create a mapping for a 64-bit register (gpr) to 2 HW registers (returned)
// The value mapped may have a constant value (constant) to be set later
RegMapping mapConstant64New(int gpr, int constant);
// Return whether a register (gpr) has a constant value mapped to it
int isRegisterConstant(int gpr);
// Get the constant value held by a 32-bit register (gpr)
long getRegisterConstant(int gpr);
// Get the constant value held by a 64-bit register (gpr)
long long getRegisterConstant64(int gpr);
// Set the constant value (value) held by a 32-bit register (gpr)
void setRegisterConstant(int gpr, long value);
// Set the constant value (value) held by a 64-bit register (gpr)
void setRegisterConstant64(int gpr, long long value);


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
void flushRegisters(void);
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
