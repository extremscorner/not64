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
	int dirty; // Nonzero means the register must be flushed to memory
	int lru;   // LRU value for flushing; higher is newer
} regMap[34];

static unsigned int nextLRUVal;
static int availableRegsDefault[32] = {
	0, /* r0 is mostly used for saving/restoring lr: used as a temp */
	0, /* sp: leave alone! */
	0, /* gp: leave alone! */
	1,1,1,1,1,1,1,1, /* Volatile argument registers */
	1,1, /* Volatile registers */
	/* Non-volatile registers: using might be too costly */
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	};
static int availableRegs[32];

// Actually perform the store for a dirty register mapping
static void _flushRegister(int reg){
	PowerPC_instr ppc;
	if(regMap[reg].map.hi >= 0){
		// Simply store the mapped MSW
		GEN_STW(ppc, regMap[reg].map.hi, reg*8, DYNAREG_REG);
		set_next_dst(ppc);
	} else {
		// Sign extend to 64-bits
		GEN_SRAWI(ppc, 0, regMap[reg].map.lo, 31);
		set_next_dst(ppc);
		// Store the MSW
		GEN_STW(ppc, 0, reg*8, DYNAREG_REG);
		set_next_dst(ppc);
	}
	// Store the LSW
	GEN_STW(ppc, regMap[reg].map.lo, reg*8+4, DYNAREG_REG);
	set_next_dst(ppc);
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

int mapRegisterNew(int reg){
	if(!reg) return 0; // Discard any writes to r0
	regMap[reg].lru = nextLRUVal++;
	regMap[reg].dirty = 1; // Since we're writing to this reg, its dirty
	
	// If its already been mapped, just return that value
	if(regMap[reg].map.lo >= 0){
		// If the hi value is mapped, free the mapping
		if(regMap[reg].map.hi >= 0){
			availableRegs[regMap[reg].map.hi] = 1;
			regMap[reg].map.hi = -1;
		}
		return regMap[reg].map.lo;
	}
	
	// Try to find any already available register
	int available = getAvailableHWReg();
	if(available >= 0) return regMap[reg].map.lo = available;
	// We didn't find an available register, so flush one
	RegMapping lru = flushLRURegister();
	if(lru.hi >= 0) availableRegs[lru.hi] = 1;
	
	return regMap[reg].map.lo = lru.lo;
}

RegMapping mapRegister64New(int reg){
	if(!reg) return (RegMapping){ 0, 0 };
	regMap[reg].lru = nextLRUVal++;
	regMap[reg].dirty = 1; // Since we're writing to this reg, its dirty
	// If its already been mapped, just return that value
	if(regMap[reg].map.lo >= 0){
		// If the hi value is not mapped, find a mapping
		if(regMap[reg].map.hi < 0){
			// Try to find any already available register
			int available = getAvailableHWReg();
			if(available >= 0) regMap[reg].map.hi = available;
			else {
				// We didn't find an available register, so flush one
				RegMapping lru = flushLRURegister();
				if(lru.hi >= 0) availableRegs[lru.hi] = 1;
				regMap[reg].map.hi = lru.lo;
			}
		}
		// Return the mapping
		return regMap[reg].map;
	}
	
	// Try to find any already available registers
	regMap[reg].map.lo = getAvailableHWReg();
	regMap[reg].map.hi = getAvailableHWReg();
	// If there weren't enough registers, we'll have to flush
	if(regMap[reg].map.lo < 0){
		// We didn't find any available registers, so flush one
		RegMapping lru = flushLRURegister();
		if(lru.hi >= 0) regMap[reg].map.hi = lru.hi;
		regMap[reg].map.lo = lru.lo;
	}
	if(regMap[reg].map.hi < 0){
		// We didn't find an available register, so flush one
		RegMapping lru = flushLRURegister();
		if(lru.hi >= 0) availableRegs[lru.hi] = 1;
		regMap[reg].map.hi = lru.lo;
	}
	// Return the mapping
	return regMap[reg].map;
}

int mapRegister(int reg){
	PowerPC_instr ppc;
	if(!reg) return DYNAREG_ZERO; // Return r0 mapped to r14
	regMap[reg].lru = nextLRUVal++;
	// If its already been mapped, just return that value
	if(regMap[reg].map.lo >= 0){
		// Note: We don't want to free any 64-bit mapping that may exist
		//       because this may be a read-after-64-bit-write
		return regMap[reg].map.lo;
	}
	regMap[reg].dirty = 0; // If it hasn't previously been mapped, its clean
	// Iterate over the HW registers and find one that's available
	int available = getAvailableHWReg();
	if(available >= 0){
		GEN_LWZ(ppc, available, reg*8+4, DYNAREG_REG);
		set_next_dst(ppc);
		
		return regMap[reg].map.lo = available;
	}
	// We didn't find an available register, so flush one
	RegMapping lru = flushLRURegister();
	if(lru.hi >= 0) availableRegs[lru.hi] = 1;
	// And load the registers value to the register we flushed
	GEN_LWZ(ppc, lru.lo, reg*8+4, DYNAREG_REG);
	set_next_dst(ppc);
	
	return regMap[reg].map.lo = lru.lo;
}

RegMapping mapRegister64(int reg){
	PowerPC_instr ppc;
	if(!reg) return (RegMapping){ DYNAREG_ZERO, DYNAREG_ZERO };
	regMap[reg].lru = nextLRUVal++;
	// If its already been mapped, just return that value
	if(regMap[reg].map.lo >= 0){
		// If the hi value is not mapped, find a mapping
		if(regMap[reg].map.hi < 0){
			// Try to find any already available register
			int available = getAvailableHWReg();
			if(available >= 0) regMap[reg].map.hi = available;
			else {
				// We didn't find an available register, so flush one
				RegMapping lru = flushLRURegister();
				if(lru.hi >= 0) availableRegs[lru.hi] = 1;
				regMap[reg].map.hi = lru.lo;
			}
			// Sign extend to 64-bits
			GEN_SRAWI(ppc, regMap[reg].map.hi, regMap[reg].map.lo, 31);
			set_next_dst(ppc);
		}
		// Return the mapping
		return regMap[reg].map;
	}
	regMap[reg].dirty = 0; // If it hasn't previously been mapped, its clean
	
	// Try to find any already available registers
	regMap[reg].map.lo = getAvailableHWReg();
	regMap[reg].map.hi = getAvailableHWReg();
	// If there weren't enough registers, we'll have to flush
	if(regMap[reg].map.lo < 0){
		// We didn't find any available registers, so flush one
		RegMapping lru = flushLRURegister();
		if(lru.hi >= 0) regMap[reg].map.hi = lru.hi;
		regMap[reg].map.lo = lru.lo;
	}
	if(regMap[reg].map.hi < 0){
		// We didn't find an available register, so flush one
		RegMapping lru = flushLRURegister();
		if(lru.hi >= 0) availableRegs[lru.hi] = 1;
		regMap[reg].map.hi = lru.lo;
	}
	// Load the values into the registers
	GEN_LWZ(ppc, regMap[reg].map.hi, reg*8, DYNAREG_REG);
	set_next_dst(ppc);
	GEN_LWZ(ppc, regMap[reg].map.lo, reg*8+4, DYNAREG_REG);
	set_next_dst(ppc);
	// Return the mapping
	return regMap[reg].map;
}

void invalidateRegister(int reg){
	if(regMap[reg].map.hi >= 0)
		availableRegs[ regMap[reg].map.hi ] = 1;
	if(regMap[reg].map.lo >= 0)
		availableRegs[ regMap[reg].map.lo ] = 1;
	regMap[reg].map.hi = regMap[reg].map.lo = -1;
}

void flushRegister(int reg){
	if(regMap[reg].map.lo >= 0){
		if(regMap[reg].dirty) _flushRegister(reg);
		if(regMap[reg].map.hi >= 0)
			availableRegs[ regMap[reg].map.hi ] = 1;
		availableRegs[ regMap[reg].map.lo ] = 1;
	}
	regMap[reg].map.hi = regMap[reg].map.lo = -1;
}

RegMappingType getRegisterMapping(int reg){
	if(regMap[reg].map.hi >= 0)
		return MAPPING_64;
	else if(regMap[reg].map.lo >= 0)
		return MAPPING_32;
	else
		return MAPPING_NONE;
}

// -- FPR mappings --
static struct {
	int map;   // Holds the value of the physical fpr or -1
	int dbl;   // Double-precision
	int dirty; // Nonzero means the register must be flushed to memory
	int lru;   // LRU value for flushing; higher is newer
} fprMap[32];

static unsigned int nextLRUValFPR;
static int availableFPRsDefault[32] = {
	0, /* Volatile: used as a temp */
	1,1,1,1,1,1,1,1, /* Volatile argument registers */
	1,1,1,1,1, /* Volatile registers */
	/* Non-volatile registers: using might be too costly */
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	};
static int availableFPRs[32];

// Actually perform the store for a dirty register mapping
static void _flushFPR(int reg){
	PowerPC_instr ppc;
	// Store the register to memory (indirectly)
	int addr = mapRegisterTemp();
	
	if(fprMap[reg].dbl){
		GEN_LWZ(ppc, addr, reg*4, DYNAREG_FPR_64);
		set_next_dst(ppc);
		GEN_STFD(ppc, fprMap[reg].map, 0, addr);
		set_next_dst(ppc);
	} else {
		GEN_LWZ(ppc, addr, reg*4, DYNAREG_FPR_32);
		set_next_dst(ppc);
		GEN_STFS(ppc, fprMap[reg].map, 0, addr);
		set_next_dst(ppc);
	}
	
	unmapRegisterTemp(addr);
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
	int addr = mapRegisterTemp();
	
	if(dbl){
		GEN_LWZ(ppc, addr, fpr*4, DYNAREG_FPR_64);
		set_next_dst(ppc);
		GEN_LFD(ppc, fprMap[fpr].map, 0, addr);
		set_next_dst(ppc);
	} else {
		GEN_LWZ(ppc, addr, fpr*4, DYNAREG_FPR_32);
		set_next_dst(ppc);
		GEN_LFS(ppc, fprMap[fpr].map, 0, addr);
		set_next_dst(ppc);
	}
	
	unmapRegisterTemp(addr);	
	
	return fprMap[fpr].map;
}

void invalidateFPR(int fpr){
	if(fprMap[fpr].map >= 0)
		availableFPRs[ fprMap[fpr].map ] = 1;
	fprMap[fpr].map = -1;
}

void flushFPR(int fpr){
	if(fprMap[fpr].map >= 0){
		if(fprMap[fpr].dirty) _flushFPR(fpr);
		availableFPRs[ fprMap[fpr].map ] = 1;
	}
	fprMap[fpr].map = -1;
}


// Unmapping registers
int flushRegisters(void){
	int i, flushed = 0;
	// Flush GPRs
	for(i=1; i<34; ++i){
		if(regMap[i].map.lo >= 0 && regMap[i].dirty){
			_flushRegister(i);
			++flushed;
		}
		// Mark unmapped
		regMap[i].map.hi = regMap[i].map.lo = -1;
	}
	memcpy(availableRegs, availableRegsDefault, 32*sizeof(int));
	nextLRUVal = 0;
	// Flush FPRs
	for(i=0; i<32; ++i){
		if(fprMap[i].map >= 0 && fprMap[i].dirty){
			_flushFPR(i);
			++flushed;
		}
		// Mark unmapped
		fprMap[i].map = -1;
	}
	memcpy(availableFPRs, availableFPRsDefault, 32*sizeof(int));
	nextLRUValFPR = 0;
	
	return flushed;
}

void invalidateRegisters(void){
	int i;
	// Invalidate GPRs
	for(i=0; i<34; ++i) invalidateRegister(i);
	memcpy(availableRegs, availableRegsDefault, 32*sizeof(int));
	nextLRUVal = 0;
	// Invalidate FPRs
	for(i=0; i<32; ++i) invalidateFPR(i);
	memcpy(availableFPRs, availableFPRsDefault, 32*sizeof(int));
	nextLRUValFPR = 0;
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

void unmapRegisterTemp(int reg){
	availableRegs[reg] = 1;
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

void unmapFPRTemp(int fpr){
	availableFPRs[fpr] = 1;
}

