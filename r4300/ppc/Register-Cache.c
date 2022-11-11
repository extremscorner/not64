/**
 * Wii64 - Register-Cache.c
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

#include <string.h>
#include "Register-Cache.h"
#include "PowerPC.h"
#include "Wrappers.h"
#include <string.h>

// -- GPR mappings --
static struct {
	// Holds the value of the physical reg or -1 (hi, lo)
	RegMapping map;
	int sign;
	int dirty; // Nonzero means the register must be flushed to memory
	int lru;   // LRU value for flushing; higher is newer
	int constant; // Nonzero means there is a constant value mapped
	long long value; // Value if this mapping holds a constant
} regMap[34];

static int nextLRUVal;
static char availableRegsDefault[32] = {
	0, /* r0 is mostly used for saving/restoring lr: used as a temp */
	0, /* sp: leave alone! */
	0, /* r2: used as a temp */
	1,1,1,1,1,1,1,1, /* Volatile argument registers */
	1,1, /* Volatile registers */
	0, /* r13 is small data area: leave alone! */
	/* Non-volatile registers: using might be too costly */
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	};
static char availableRegs[32];

// Actually perform the store for a dirty register mapping
static void _flushRegister(int gpr){
	PowerPC_instr ppc;
	// Store the LSW
	GEN_STW(ppc, regMap[gpr].map.lo, SDAREL(reg) + gpr*8+4, R13);
	set_next_dst(ppc);
	
	if(regMap[gpr].map.hi >= 0){
		// Simply store the mapped MSW
		GEN_STW(ppc, regMap[gpr].map.hi, SDAREL(reg) + gpr*8, R13);
		set_next_dst(ppc);
	} else if(regMap[gpr].sign){
		// Sign extend to 64-bits
		GEN_SRAWI(ppc, R0, regMap[gpr].map.lo, 31);
		set_next_dst(ppc);
		// Store the MSW
		GEN_STW(ppc, R0, SDAREL(reg) + gpr*8, R13);
		set_next_dst(ppc);
	} else {
		GEN_STW(ppc, DYNAREG_ZERO, SDAREL(reg) + gpr*8, R13);
		set_next_dst(ppc);
	}
}
// Find an available HW reg or -1 for none
static int getAvailableHWReg(void){
	int i;
	// Iterate over the HW registers and find one that's available
	for(i=0; i<32; ++i){
		if(availableRegs[i]){
			availableRegs[i] = 0;
			return i;
		}
	}
	return -1;
}

static RegMapping flushLRURegister(void){
	int i, lru_i = 0, lru_v = 0x7fffffff;
	for(i=1; i<34; ++i){
		if(regMap[i].map.lo >= 0 && regMap[i].lru < lru_v){
			lru_i = i; lru_v = regMap[i].lru;
		}
	}
	RegMapping map = regMap[lru_i].map;
	// Flush the register if its dirty
	if(regMap[lru_i].dirty) _flushRegister(lru_i);
	// Mark unmapped
	regMap[lru_i].map.hi = regMap[lru_i].map.lo = -1;
	return map;
}

int mapRegisterNew(int gpr, int sign){
	if(!gpr) return 0; // Discard any writes to r0
	regMap[gpr].lru = nextLRUVal++;
	regMap[gpr].sign = sign;
	regMap[gpr].dirty = 1; // Since we're writing to this reg, its dirty
	regMap[gpr].constant = 0; // This is a dynamic or unknowable value
	
	// If its already been mapped, just return that value
	if(regMap[gpr].map.lo >= 0){
		// If the hi value is mapped, free the mapping
		if(regMap[gpr].map.hi >= 0){
			availableRegs[regMap[gpr].map.hi] = 1;
			regMap[gpr].map.hi = -1;
		}
		return regMap[gpr].map.lo;
	}
	
	// Try to find any already available register
	int available = getAvailableHWReg();
	if(available >= 0) return regMap[gpr].map.lo = available;
	// We didn't find an available register, so flush one
	RegMapping lru = flushLRURegister();
	if(lru.hi >= 0) availableRegs[lru.hi] = 1;
	
	return regMap[gpr].map.lo = lru.lo;
}

RegMapping mapRegister64New(int gpr){
	if(!gpr) return (RegMapping){ 0, 0 };
	regMap[gpr].lru = nextLRUVal++;
	regMap[gpr].sign = 1;
	regMap[gpr].dirty = 1; // Since we're writing to this reg, its dirty
	regMap[gpr].constant = 0; // This is a dynamic or unknowable value
	
	// If its already been mapped, just return that value
	if(regMap[gpr].map.lo >= 0){
		// If the hi value is not mapped, find a mapping
		if(regMap[gpr].map.hi < 0){
			// Try to find any already available register
			int available = getAvailableHWReg();
			if(available >= 0){
				regMap[gpr].map.hi = available;
			} else {
				// We didn't find an available register, so flush one
				RegMapping lru = flushLRURegister();
				if(lru.hi >= 0) availableRegs[lru.hi] = 1;
				regMap[gpr].map.hi = lru.lo;
			}
		}
		// Return the mapping
		return regMap[gpr].map;
	}
	
	// Try to find any already available registers
	regMap[gpr].map.hi = getAvailableHWReg();
	regMap[gpr].map.lo = getAvailableHWReg();
	// If there weren't enough registers, we'll have to flush
	if(regMap[gpr].map.lo < 0){
		// We didn't find any available registers, so flush one
		RegMapping lru = flushLRURegister();
		if(regMap[gpr].map.hi < 0)
			regMap[gpr].map.hi = lru.hi;
		else if(lru.hi >= 0)
			availableRegs[lru.hi] = 1;
		regMap[gpr].map.lo = lru.lo;
	}
	if(regMap[gpr].map.hi < 0){
		// We didn't find an available register, so flush one
		RegMapping lru = flushLRURegister();
		if(lru.hi >= 0) availableRegs[lru.hi] = 1;
		regMap[gpr].map.hi = lru.lo;
	}
	// Return the mapping
	return regMap[gpr].map;
}

int mapRegister(int gpr){
	PowerPC_instr ppc;
	if(!gpr) return DYNAREG_ZERO; // Return r0 mapped to r31
	regMap[gpr].lru = nextLRUVal++;
	// If its already been mapped, just return that value
	if(regMap[gpr].map.lo >= 0){
		// Note: We don't want to free any 64-bit mapping that may exist
		//       because this may be a read-after-64-bit-write
		return regMap[gpr].map.lo;
	}
	regMap[gpr].dirty = 0; // If it hasn't previously been mapped, its clean
	// Iterate over the HW registers and find one that's available
	int available = getAvailableHWReg();
	if(available >= 0){
		GEN_LWZ(ppc, available, SDAREL(reg) + gpr*8+4, R13);
		set_next_dst(ppc);
		
		return regMap[gpr].map.lo = available;
	}
	// We didn't find an available register, so flush one
	RegMapping lru = flushLRURegister();
	if(lru.hi >= 0) availableRegs[lru.hi] = 1;
	// And load the registers value to the register we flushed
	GEN_LWZ(ppc, lru.lo, SDAREL(reg) + gpr*8+4, R13);
	set_next_dst(ppc);
	
	return regMap[gpr].map.lo = lru.lo;
}

RegMapping mapRegister64(int gpr){
	PowerPC_instr ppc;
	if(!gpr) return (RegMapping){ DYNAREG_ZERO, DYNAREG_ZERO };
	regMap[gpr].lru = nextLRUVal++;
	// If its already been mapped, just return that value
	if(regMap[gpr].map.lo >= 0){
		// If the hi value is not mapped, find a mapping
		if(regMap[gpr].map.hi < 0){
			// Try to find any already available register
			int available = getAvailableHWReg();
			if(available >= 0){
				regMap[gpr].map.hi = available;
			} else {
				// We didn't find an available register, so flush one
				RegMapping lru = flushLRURegister();
				if(lru.hi >= 0) availableRegs[lru.hi] = 1;
				regMap[gpr].map.hi = lru.lo;
			}
			if(regMap[gpr].sign){
				// Sign extend to 64-bits
				GEN_SRAWI(ppc, regMap[gpr].map.hi, regMap[gpr].map.lo, 31);
				set_next_dst(ppc);
			} else {
				GEN_LI(ppc, regMap[gpr].map.hi, 0);
				set_next_dst(ppc);
			}
		}
		// Return the mapping
		return regMap[gpr].map;
	}
	regMap[gpr].dirty = 0; // If it hasn't previously been mapped, its clean
	
	// Try to find any already available registers
	regMap[gpr].map.hi = getAvailableHWReg();
	regMap[gpr].map.lo = getAvailableHWReg();
	// If there weren't enough registers, we'll have to flush
	if(regMap[gpr].map.lo < 0){
		// We didn't find any available registers, so flush one
		RegMapping lru = flushLRURegister();
		if(regMap[gpr].map.hi < 0)
			regMap[gpr].map.hi = lru.hi;
		else if(lru.hi >= 0)
			availableRegs[lru.hi] = 1;
		regMap[gpr].map.lo = lru.lo;
	}
	if(regMap[gpr].map.hi < 0){
		// We didn't find an available register, so flush one
		RegMapping lru = flushLRURegister();
		if(lru.hi >= 0) availableRegs[lru.hi] = 1;
		regMap[gpr].map.hi = lru.lo;
	}
	// Load the values into the registers
	GEN_LWZ(ppc, regMap[gpr].map.hi, SDAREL(reg) + gpr*8, R13);
	set_next_dst(ppc);
	GEN_LWZ(ppc, regMap[gpr].map.lo, SDAREL(reg) + gpr*8+4, R13);
	set_next_dst(ppc);
	// Return the mapping
	return regMap[gpr].map;
}

void invalidateRegister(int gpr){
	if(regMap[gpr].map.hi >= 0)
		availableRegs[ regMap[gpr].map.hi ] = 1;
	if(regMap[gpr].map.lo >= 0)
		availableRegs[ regMap[gpr].map.lo ] = 1;
	regMap[gpr].map.hi = regMap[gpr].map.lo = -1;
}

void flushRegister(int gpr){
	if(regMap[gpr].map.lo >= 0){
		if(regMap[gpr].dirty) _flushRegister(gpr);
		if(regMap[gpr].map.hi >= 0)
			availableRegs[ regMap[gpr].map.hi ] = 1;
		availableRegs[ regMap[gpr].map.lo ] = 1;
	}
	regMap[gpr].map.hi = regMap[gpr].map.lo = -1;
}

RegMappingType getRegisterMapping(int gpr){
	if(regMap[gpr].map.hi >= 0)
		return MAPPING_64;
	else if(regMap[gpr].map.lo >= 0)
		return MAPPING_32;
	else
		return MAPPING_NONE;
}

int mapConstantNew(int gpr, int constant, int sign){
	int mapping = mapRegisterNew(gpr, sign); // Get the normal mapping
	regMap[gpr].constant = constant; // Set the constant field
	return mapping;
}

RegMapping mapConstant64New(int gpr, int constant){
	RegMapping mapping = mapRegister64New(gpr); // Get the normal mapping
	regMap[gpr].constant = constant; // Set the constant field
	return mapping;
}

void invalidateConstant(int gpr){
	regMap[gpr].constant = 0;
	regMap[gpr].value = 0;
}

int isRegisterConstant(int gpr){
	// Always constant for r0
	return gpr ? regMap[gpr].constant : 1;
}

long getRegisterConstant(int gpr){
	return getRegisterConstant64(gpr);
}

long long getRegisterConstant64(int gpr){
	// Always return 0 for r0 
	return gpr ? regMap[gpr].value : 0;
}

void setRegisterConstant(int gpr, long value){
	setRegisterConstant64(gpr, value);
}

void setRegisterConstant64(int gpr, long long value){
	if(regMap[gpr].constant)
		regMap[gpr].value = value;
}

// -- FPR mappings --
static struct {
	int map;   // Holds the value of the physical fpr or -1
	int dbl;   // Double-precision
	int dirty; // Nonzero means the register must be flushed to memory
	int lru;   // LRU value for flushing; higher is newer
} fprMap[32];

static int nextLRUValFPR;
static char availableFPRsDefault[32] = {
	0, /* Volatile: used as a temp */
	1,1,1,1,1,1,1,1, /* Volatile argument registers */
	1,1,1,1,1, /* Volatile registers */
	/* Non-volatile registers: using might be too costly */
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	};
static char availableFPRs[32];

// Actually perform the store for a dirty register mapping
static void _flushFPR(int fpr){
	PowerPC_instr ppc;
	// Store the register to memory (indirectly)
	if(fprMap[fpr].dbl){
		GEN_LWZ(ppc, R0, SDAREL(reg_cop1_double) + fpr*4, R13);
		set_next_dst(ppc);
		GEN_STFDX(ppc, fprMap[fpr].map, 0, R0);
		set_next_dst(ppc);
	} else {
		GEN_LWZ(ppc, R0, SDAREL(reg_cop1_simple) + fpr*4, R13);
		set_next_dst(ppc);
		GEN_STFSX(ppc, fprMap[fpr].map, 0, R0);
		set_next_dst(ppc);
	}
}
// Find an available HW reg or -1 for none
static int getAvailableFPR(void){
	int i;
	// Iterate over the HW registers and find one that's available
	for(i=0; i<32; ++i){
		if(availableFPRs[i]){
			availableFPRs[i] = 0;
			return i;
		}
	}
	return -1;
}

static int flushLRUFPR(void){
	int i, lru_i = 0, lru_v = 0x7fffffff;
	for(i=0; i<32; ++i){
		if(fprMap[i].map >= 0 && fprMap[i].lru < lru_v){
			lru_i = i; lru_v = fprMap[i].lru;
		}
	}
	int map = fprMap[lru_i].map;
	// Flush the register if its dirty
	if(fprMap[lru_i].dirty) _flushFPR(lru_i);
	// Mark unmapped
	fprMap[lru_i].map = -1;
	return map;
}


int mapFPRNew(int fpr, int dbl){
	fprMap[fpr].lru = nextLRUValFPR++;
	fprMap[fpr].dirty = 1; // Since we're writing to this reg, its dirty
	fprMap[fpr].dbl = dbl; // Set whether this is a double-precision
	
	// If its already been mapped, just return that value
	if(fprMap[fpr].map >= 0) return fprMap[fpr].map;
	
	// Try to find any already available register
	int available = getAvailableFPR();
	if(available >= 0) return fprMap[fpr].map = available;
	
	// We didn't find an available register, so flush one
	return fprMap[fpr].map = flushLRUFPR();
}

int mapFPR(int fpr, int dbl){
	PowerPC_instr ppc;
	
	if(fprMap[fpr].dbl ^ dbl)
		flushFPR(fpr);
	if(fprMap[fpr ^ 1].dbl ^ dbl)
		flushFPR(fpr ^ 1);
	
	fprMap[fpr].lru = nextLRUValFPR++;
	fprMap[fpr].dbl = dbl; // Set whether this is a double-precision
	
	// If its already been mapped, just return that value
	// FIXME: Do I need to worry about conversions between single and double?
	if(fprMap[fpr].map >= 0) return fprMap[fpr].map;
	
	fprMap[fpr].dirty = 0; // If it hasn't previously been mapped, its clean
	// Try to find any already available register
	fprMap[fpr].map = getAvailableFPR();
	// If didn't find an available register, flush one
	if(fprMap[fpr].map < 0) fprMap[fpr].map = flushLRUFPR();
	
	// Load the register from memory (indirectly)
	if(dbl){
		GEN_LWZ(ppc, R0, SDAREL(reg_cop1_double) + fpr*4, R13);
		set_next_dst(ppc);
		GEN_LFDX(ppc, fprMap[fpr].map, 0, R0);
		set_next_dst(ppc);
	} else {
		GEN_LWZ(ppc, R0, SDAREL(reg_cop1_simple) + fpr*4, R13);
		set_next_dst(ppc);
		GEN_LFSX(ppc, fprMap[fpr].map, 0, R0);
		set_next_dst(ppc);
	}
	
	return fprMap[fpr].map;
}

void invalidateFPR(int fpr){
	if(fprMap[fpr].map >= 0)
		availableFPRs[ fprMap[fpr].map ] = 1;
	fprMap[fpr].map = -1;
	if(fprMap[fpr ^ 1].dbl ^ fprMap[fpr].dbl)
		flushFPR(fpr ^ 1);
}

void flushFPR(int fpr){
	if(fprMap[fpr].map >= 0){
		if(fprMap[fpr].dirty) _flushFPR(fpr);
		availableFPRs[ fprMap[fpr].map ] = 1;
	}
	fprMap[fpr].map = -1;
}


// Unmapping registers
void flushRegisters(void){
	int i;
	// Flush GPRs
	for(i=1; i<34; ++i) flushRegister(i);
	memcpy(availableRegs, availableRegsDefault, 32);
	nextLRUVal = 0;
	// Flush FPRs
	for(i=0; i<32; ++i) flushFPR(i);
	memcpy(availableFPRs, availableFPRsDefault, 32);
	nextLRUValFPR = 0;
}

void invalidateRegisters(void){
	int i;
	// Invalidate GPRs
	for(i=0; i<34; ++i) invalidateRegister(i);
	memcpy(availableRegs, availableRegsDefault, 32);
	nextLRUVal = 0;
	// Invalidate FPRs
	for(i=0; i<32; ++i) invalidateFPR(i);
	memcpy(availableFPRs, availableFPRsDefault, 32);
	nextLRUValFPR = 0;
}

void invalidateConstants(void){
	int i;
	for(i=0; i<34; ++i) invalidateConstant(i);
}

int mapRegisterTemp(void){
	// Try to find an already available register
	int available = getAvailableHWReg();
	if(available >= 0) return available;
	// If there are none, flush the LRU and use it
	RegMapping lru = flushLRURegister();
	if(lru.hi >= 0) availableRegs[lru.hi] = 1;
	return lru.lo;
}

void mapRegisterFixed(int tmp){
	int i;
	for(i=1; i<34; ++i)
		if(regMap[i].map.hi == tmp ||
		   regMap[i].map.lo == tmp) flushRegister(i);
	availableRegs[tmp] = 0;
}

void unmapRegisterTemp(int tmp){
	availableRegs[tmp] = 1;
}

int mapFPRTemp(void){
	// Try to find an already available FPR
	int available = getAvailableFPR();
	// If didn't find an available register, flush one
	if(available >= 0) return available;
	// If there are none, flush the LRU and use it
	int lru = flushLRUFPR();
	return lru;
}

void mapFPRFixed(int tmp){
	int i;
	for(i=0; i<32; ++i)
		if(fprMap[i].map == tmp) flushFPR(i);
	availableFPRs[tmp] = 0;
}

void unmapFPRTemp(int tmp){
	availableFPRs[tmp] = 1;
}

