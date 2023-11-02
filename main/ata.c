/**
 * IDE EXI Driver for Gamecube & Wii
 *
 * Based loosely on code written by Dampro
 * Re-written by emu_kidid
**/

#include <stdio.h>
#include <gccore.h>		/*** Wrapper to include common libogc headers ***/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ogc/exi.h>
#include <ogc/machine/processor.h>
#include "ata.h"

#define IDE_EXI_V1 0
#define IDE_EXI_V2 1
#define IDE_EXI_V3 2

u16 buffer[256] ATTRIBUTE_ALIGN (32);
static int __ata_init[3] = {0,0,0};
static int _ideexi_version = IDE_EXI_V1;

// Drive information struct
typeDriveInfo ataDriveInfo;

// Returns 8 bits from the ATA Status register
static inline u8 ataReadStatusReg(int chn)
{
	int dev = EXI_DEVICE_0;
	if(chn == EXI_CHANNEL_2) {
		chn = EXI_CHANNEL_0;
		dev = EXI_DEVICE_2;
	}
	// read ATA_REG_CMDSTATUS1 | 0x00 (dummy)
	u16 dat = 0x1700;
	EXI_Lock(chn, dev, NULL);
	EXI_Select(chn,dev,EXI_SPEED32MHZ);
	EXI_ImmEx(chn,&dat,2,EXI_WRITE);
	EXI_ImmEx(chn,&dat,1,EXI_READ);
	EXI_Deselect(chn);
	EXI_Unlock(chn);
    return *(u8*)&dat;
}

// Returns 8 bits from the ATA Error register
static inline u8 ataReadErrorReg(int chn)
{
	int dev = EXI_DEVICE_0;
	if(chn == EXI_CHANNEL_2) {
		chn = EXI_CHANNEL_0;
		dev = EXI_DEVICE_2;
	}
	// read ATA_REG_ERROR | 0x00 (dummy)
	u16 dat = 0x1100;
	EXI_Lock(chn, dev, NULL);
	EXI_Select(chn,dev,EXI_SPEED32MHZ);
	EXI_ImmEx(chn,&dat,2,EXI_WRITE);
	EXI_ImmEx(chn,&dat,1,EXI_READ);
	EXI_Deselect(chn);
	EXI_Unlock(chn);
    return *(u8*)&dat;
}

// Writes 8 bits of data out to the specified ATA Register
static inline void ataWriteByte(int chn, u8 addr, u8 data)
{
	int dev = EXI_DEVICE_0;
	if(chn == EXI_CHANNEL_2) {
		chn = EXI_CHANNEL_0;
		dev = EXI_DEVICE_2;
	}
	u32 dat = 0x80000000 | (addr << 24) | (data<<16);
	EXI_Lock(chn, dev, NULL);
	EXI_Select(chn,dev,EXI_SPEED32MHZ);
	EXI_ImmEx(chn,&dat,3,EXI_WRITE);	
	EXI_Deselect(chn);
	EXI_Unlock(chn);
}

// Writes 16 bits to the ATA Data register
static inline void ataWriteu16(int chn, u16 data) 
{
	int dev = EXI_DEVICE_0;
	if(chn == EXI_CHANNEL_2) {
		chn = EXI_CHANNEL_0;
		dev = EXI_DEVICE_2;
	}
	// write 16 bit to ATA_REG_DATA | data LSB | data MSB | 0x00 (dummy)
	u32 dat = 0xD0000000 | (((data>>8) & 0xff)<<16) | ((data & 0xff)<<8);
	EXI_Lock(chn, dev, NULL);
	EXI_Select(chn,dev,EXI_SPEED32MHZ);
	EXI_ImmEx(chn,&dat,4,EXI_WRITE);
	EXI_Deselect(chn);
	EXI_Unlock(chn);
}


// Returns 16 bits from the ATA Data register
static inline u16 ataReadu16(int chn) 
{
	int dev = EXI_DEVICE_0;
	if(chn == EXI_CHANNEL_2) {
		chn = EXI_CHANNEL_0;
		dev = EXI_DEVICE_2;
	}
	// read 16 bit from ATA_REG_DATA | 0x00 (dummy)
	u16 dat = 0x5000;  	
	EXI_Lock(chn, dev, NULL);
	EXI_Select(chn,dev,EXI_SPEED32MHZ);
	EXI_ImmEx(chn,&dat,2,EXI_WRITE);
	EXI_ImmEx(chn,&dat,2,EXI_READ); // read LSB & MSB
	EXI_Deselect(chn);
	EXI_Unlock(chn);
    return dat;
}


// Reads 512 bytes
static inline void ata_read_buffer(int chn, u32 *dst) 
{
	int dev = EXI_DEVICE_0;
	if(chn == EXI_CHANNEL_2) {
		chn = EXI_CHANNEL_0;
		dev = EXI_DEVICE_2;
	}
	u16 dwords = 128;	// 128 * 4 = 512 bytes
	// (31:29) 011b | (28:24) 10000b | (23:16) <num_words_LSB> | (15:8) <num_words_MSB> | (7:0) 00h (4 bytes)
	u32 dat = 0x70000000 | ((dwords&0xff) << 16) | (((dwords>>8)&0xff) << 8);
	EXI_Lock(chn, dev, NULL);
	EXI_Select(chn,dev,EXI_SPEED32MHZ);
	EXI_ImmEx(chn,&dat,4,EXI_WRITE);
	if(_ideexi_version == IDE_EXI_V1) {
		// IDE_EXI_V1, select / deselect for every 4 bytes
		EXI_Deselect(chn);
		EXI_Unlock(chn);
		u32 i = 0;
		u32 *ptr = dst;
		for(i = 0; i < dwords; i++) {
			EXI_Lock(chn, dev, NULL);
			EXI_Select(chn,dev,EXI_SPEED32MHZ);
			EXI_ImmEx(chn,ptr,4,EXI_READ);
			ptr++;
			EXI_Deselect(chn);
			EXI_Unlock(chn);
		}
		EXI_Lock(chn, dev, NULL);
		EXI_Select(chn,dev,EXI_SPEED32MHZ);
		EXI_ImmEx(chn,&dat,4,EXI_READ);
		EXI_Deselect(chn);
		EXI_Unlock(chn);
	}
	else {
		// IDE_EXI_V2, no need to select / deselect all the time
		EXI_DmaEx(chn,dst,512,EXI_READ);
		EXI_Deselect(chn);
		EXI_Unlock(chn);
	}
}

static inline void ata_write_buffer(int chn, u32 *src) 
{
	int dev = EXI_DEVICE_0;
	if(chn == EXI_CHANNEL_2) {
		chn = EXI_CHANNEL_0;
		dev = EXI_DEVICE_2;
	}
	u16 dwords = 128;	// 128 * 4 = 512 bytes
	// (23:21) 111b | (20:16) 10000b | (15:8) <num_words_LSB> | (7:0) <num_words_MSB> (3 bytes)
	u32 dat = 0xF0000000 | ((dwords&0xff) << 16) | (((dwords>>8)&0xff) << 8);
	EXI_Lock(chn, dev, NULL);
	EXI_Select(chn,dev,EXI_SPEED32MHZ);
	EXI_ImmEx(chn,&dat,3,EXI_WRITE);
	EXI_DmaEx(chn, src,512,EXI_WRITE);
	dat = 0;
	EXI_ImmEx(chn,&dat,1,EXI_WRITE);	// Burn an extra cycle for the IDE-EXI to know to stop serving data
	EXI_Deselect(chn);
	EXI_Unlock(chn);
}

int _ideExiVersion(int chn) {
	int dev = EXI_DEVICE_0;
	if(chn == EXI_CHANNEL_2) {
		chn = EXI_CHANNEL_0;
		dev = EXI_DEVICE_2;
	}
	u32 cid = 0;
	EXI_GetID(chn,dev,&cid);
	if((cid&~0xff)==0x49444500) {
		return (cid&0xff)-'1';
	}
	else {
		return IDE_EXI_V1;
	}
}


// Sends the IDENTIFY command to the HDD
// Returns 0 on success, -1 otherwise
u32 _ataDriveIdentify(int chn) {
  	u16 tmp,retries = 50;
  	u32 i = 0;

  	memset(&ataDriveInfo, 0, sizeof(typeDriveInfo));

	// Get the ID to see if it's a V2+
	_ideexi_version = _ideExiVersion(chn);
	if(_ideexi_version == IDE_EXI_V1 && chn == EXI_CHANNEL_2) {
		return -1;
	}
  	
  	// Select the device
  	ataWriteByte(chn, ATA_REG_DEVICE, 0);
  	
	// Wait for drive to be ready (BSY to clear) - 5 sec timeout
	do {
		tmp = ataReadStatusReg(chn);
		usleep(100000);	//sleep for 0.1 seconds
		retries--;
	}
	while((tmp & ATA_SR_BSY) && retries);
	if(!retries) {
		return -1;
	}
    
	// Write the identify command
  	ataWriteByte(chn, ATA_REG_COMMAND, ATA_CMD_IDENTIFY);

	// Wait for drive to request data transfer - 1 sec timeout
	retries = 10;
	do { 
		tmp = ataReadStatusReg(chn); 
		usleep(100000);	//sleep for 0.1 seconds
		retries--;
	}
	while((!(tmp & ATA_SR_DRQ)) && retries);
	if(!retries) {
		return -1;
	}
	usleep(2000);
    
	u16 *ptr = (u16*)(&buffer[0]);
	
	// Read Identify data from drive
	for (i=0; i<256; i++) {
		tmp = ataReadu16(chn); // get data
	   *ptr++ = bswap16(tmp); // swap
	}
	
	// Get the info out of the Identify data buffer
	// From the command set, check if LBA48 is supported
	u16 commandSet = *(u16*)(&buffer[ATA_IDENT_COMMANDSET]);
	ataDriveInfo.lba48Support = (commandSet>>8) & ATA_IDENT_LBA48MASK;
	
	if(ataDriveInfo.lba48Support) {
		u16 lbaHi = *(u16*) (&buffer[ATA_IDENT_LBA48SECTORS+2]);
		u16 lbaMid = *(u16*) (&buffer[ATA_IDENT_LBA48SECTORS+1]);
		u16 lbaLo = *(u16*) (&buffer[ATA_IDENT_LBA48SECTORS]);
		ataDriveInfo.sizeInSectors = (u64)(((u64)lbaHi << 32) | (lbaMid << 16) | lbaLo);
	}
	else {
		ataDriveInfo.sizeInSectors =	((*(u16*) &buffer[ATA_IDENT_LBASECTORS+1])<<16) | 
										(*(u16*) &buffer[ATA_IDENT_LBASECTORS]);
	}
	
	i = 20;
	// copy serial string
	memcpy(&ataDriveInfo.serial[0], &buffer[ATA_IDENT_SERIAL],20);
	// cut off the string (usually has trailing spaces)
	while((ataDriveInfo.serial[i] == ' ' || !ataDriveInfo.serial[i]) && i >=0) {
		ataDriveInfo.serial[i] = 0;
		i--;
	}
	// copy model string
	memcpy(&ataDriveInfo.model[0], &buffer[ATA_IDENT_MODEL],40);
	// cut off the string (usually has trailing spaces)
	i = 40;
	while((ataDriveInfo.model[i] == ' ' || !ataDriveInfo.model[i]) && i >=0) {
		ataDriveInfo.model[i] = 0;
		i--;
	}
	
	// Return ok
	return 0;
}

// Unlocks a ATA HDD with a password
// Returns 0 on success, -1 on failure.
int ataUnlock(int chn, int useMaster, char *password)
{
	u32 i;
  	
  	// Wait for drive to be ready (BSY to clear)
	while(ataReadStatusReg(chn) & ATA_SR_BSY);
  	
	// Select the device
  	ataWriteByte(chn, ATA_REG_DEVICE, 0);

	// Write the appropriate unlock command
  	ataWriteByte(chn, ATA_REG_COMMAND, ATA_CMD_UNLOCK);

	while(!(ataReadStatusReg(chn) & ATA_SR_DRQ));
	
	// Fill an unlock struct
	unlockStruct unlock;
	memset(&unlock, 0, sizeof(unlockStruct));
	unlock.type = useMaster;
	memcpy(unlock.password, password, strlen(password));

	// write data to the drive 
	u16 *ptr = (u16*)&unlock;
	for (i=0; i<256; i++) {
		ataWriteu16(chn, ptr[i]);
	}
	
	// Wait for the write
	while(ataReadStatusReg(chn) & ATA_SR_BSY);
	
	return !(ataReadErrorReg(chn) & ATA_ER_ABRT);
}

// Reads sectors from the specified lba, for the specified slot
// Returns 0 on success, -1 on failure.
int _ataReadSector(int chn, u64 lba, u32 *Buffer)
{
	u32 temp = 0;
  	
  	// Wait for drive to be ready (BSY to clear)
	while(ataReadStatusReg(chn) & ATA_SR_BSY);
  	
	// Select the device differently based on 28 or 48bit mode
	if(ataDriveInfo.lba48Support) {
		// Select the device (ATA_HEAD_USE_LBA is 0x40 for master, 0x50 for slave)
		ataWriteByte(chn, ATA_REG_DEVICE, ATA_HEAD_USE_LBA);
	}
	else {
		// Select the device (ATA_HEAD_USE_LBA is 0x40 for master, 0x50 for slave)
		ataWriteByte(chn, ATA_REG_DEVICE, 0xE0 | (u8)((lba >> 24) & 0x0F));
	}
  		
	// check if drive supports LBA 48-bit
	if(ataDriveInfo.lba48Support) {  		
		ataWriteByte(chn, ATA_REG_SECCOUNT, 0);								// Sector count (Hi)
		ataWriteByte(chn, ATA_REG_LBALO, (u8)((lba>>24)& 0xFF));			// LBA 4
		ataWriteByte(chn, ATA_REG_LBAMID, (u8)((lba>>32) & 0xFF));			// LBA 5
		ataWriteByte(chn, ATA_REG_LBAHI, (u8)((lba>>40) & 0xFF));			// LBA 6
		ataWriteByte(chn, ATA_REG_SECCOUNT, 1);								// Sector count (Lo)
		ataWriteByte(chn, ATA_REG_LBALO, (u8)(lba & 0xFF));					// LBA 1
  		ataWriteByte(chn, ATA_REG_LBAMID, (u8)((lba>>8) & 0xFF));			// LBA 2
  		ataWriteByte(chn, ATA_REG_LBAHI, (u8)((lba>>16) & 0xFF));			// LBA 3
	}
	else {
		ataWriteByte(chn, ATA_REG_SECCOUNT, 1);								// Sector count
		ataWriteByte(chn, ATA_REG_LBALO, (u8)(lba & 0xFF));					// LBA Lo
  		ataWriteByte(chn, ATA_REG_LBAMID, (u8)((lba>>8) & 0xFF));			// LBA Mid
  		ataWriteByte(chn, ATA_REG_LBAHI, (u8)((lba>>16) & 0xFF));			// LBA Hi
	}

	// Write the appropriate read command
  	ataWriteByte(chn, ATA_REG_COMMAND, ataDriveInfo.lba48Support ? ATA_CMD_READSECTEXT : ATA_CMD_READSECT);

	// Wait for BSY to clear
	while((temp = ataReadStatusReg(chn)) & ATA_SR_BSY);
	
	// If the error bit was set, fail.
	if(temp & ATA_SR_ERR) {
		return 1;
	}

	// Wait for drive to request data transfer
	while(!(ataReadStatusReg(chn) & ATA_SR_DRQ));
	
	// read data from drive
	ata_read_buffer(chn, Buffer);

	temp = ataReadStatusReg(chn);
	// If the error bit was set, fail.
	if(temp & ATA_SR_ERR) {
		return 1;
	}
	return temp & ATA_SR_ERR;
}

// Writes sectors to the specified lba, for the specified slot
// Returns 0 on success, -1 on failure.
int _ataWriteSector(int chn, u64 lba, u32 *Buffer)
{
	u32 i, temp;
  	
  	// Wait for drive to be ready (BSY to clear)
	while(ataReadStatusReg(chn) & ATA_SR_BSY);
  	
	// Select the device differently based on 28 or 48bit mode
	if(ataDriveInfo.lba48Support) {
		// Select the device (ATA_HEAD_USE_LBA is 0x40 for master, 0x50 for slave)
		ataWriteByte(chn, ATA_REG_DEVICE, ATA_HEAD_USE_LBA);
	}
	else {
		// Select the device (ATA_HEAD_USE_LBA is 0x40 for master, 0x50 for slave)
		ataWriteByte(chn, ATA_REG_DEVICE, 0xE0 | (u8)((lba >> 24) & 0x0F));
	}
  		
	// check if drive supports LBA 48-bit
	if(ataDriveInfo.lba48Support) {  		
		ataWriteByte(chn, ATA_REG_SECCOUNT, 0);								// Sector count (Hi)
		ataWriteByte(chn, ATA_REG_LBALO, (u8)((lba>>24)& 0xFF));			// LBA 4
		ataWriteByte(chn, ATA_REG_LBAMID, (u8)((lba>>32) & 0xFF));			// LBA 4
		ataWriteByte(chn, ATA_REG_LBAHI, (u8)((lba>>40) & 0xFF));			// LBA 5
  		ataWriteByte(chn, ATA_REG_SECCOUNT, 1);								// Sector count (Lo)
		ataWriteByte(chn, ATA_REG_LBALO, (u8)(lba & 0xFF));					// LBA 1
  		ataWriteByte(chn, ATA_REG_LBAMID, (u8)((lba>>8) & 0xFF));			// LBA 2
  		ataWriteByte(chn, ATA_REG_LBAHI, (u8)((lba>>16) & 0xFF));			// LBA 3
	}
	else {
  		ataWriteByte(chn, ATA_REG_SECCOUNT, 1);								// Sector count
		ataWriteByte(chn, ATA_REG_LBALO, (u8)(lba & 0xFF));					// LBA Lo
  		ataWriteByte(chn, ATA_REG_LBAMID, (u8)((lba>>8) & 0xFF));			// LBA Mid
  		ataWriteByte(chn, ATA_REG_LBAHI, (u8)((lba>>16) & 0xFF));			// LBA Hi
	}

	// Write the appropriate write command
  	ataWriteByte(chn, ATA_REG_COMMAND, ataDriveInfo.lba48Support ? ATA_CMD_WRITESECTEXT : ATA_CMD_WRITESECT);

  	// Wait for BSY to clear
	while((temp = ataReadStatusReg(chn)) & ATA_SR_BSY);
	
	// If the error bit was set, fail.
	if(temp & ATA_SR_ERR) {
		return 1;
	}
	// Wait for drive to request data transfer
	while(!(ataReadStatusReg(chn) & ATA_SR_DRQ));

	// Write data to the drive
	if(_ideexi_version < IDE_EXI_V3) {
		u16 *ptr = (u16*)Buffer;
		for (i=0; i<256; i++) {
			ataWriteu16(chn, ptr[i]);
		}
	}
	else {
		ata_write_buffer(chn, Buffer);
	}
	
	// Wait for the write to finish
	while(ataReadStatusReg(chn) & ATA_SR_BSY);
	
	temp = ataReadStatusReg(chn);
	// If the error bit was set, fail.
	if(temp & ATA_SR_ERR) {
		return 1;
	}
	return temp & ATA_SR_ERR;
}

// Wrapper to read a number of sectors
// 0 on Success, -1 on Error
int ataReadSectors(int chn, u64 sector, unsigned int numSectors, unsigned char *dest) 
{
	int ret = 0;
	while(numSectors) {
		if((ret=_ataReadSector(chn,sector,(u32*)dest))) {
			return -1;
		}
		dest+=512;
		sector++;
		numSectors--;
	}
	return 0;
}

// Wrapper to write a number of sectors
// 0 on Success, -1 on Error
int ataWriteSectors(int chn, u64 sector,unsigned int numSectors, unsigned char *src) 
{
	int ret = 0;
	while(numSectors) {
		if((ret=_ataWriteSector(chn,sector,(u32*)src))) {
			return -1;
		}
		src+=512;
		sector++;
		numSectors--;
	}
	return 0;
}

// Is an ATA device inserted?
bool ataIsInserted(int chn) {
	if(__ata_init[chn]) {
		return true;
	}
	if(_ataDriveIdentify(chn)) {
		return false;
	}
	__ata_init[chn] = 1;
	return true;
}

int ataShutdown(int chn) {
	__ata_init[chn] = 0;
	return 1;
}


static bool __ataa_startup(void)
{
	return ataIsInserted(0);
}

static bool __ataa_isInserted(void)
{
	return ataIsInserted(0);
}

static bool __ataa_readSectors(sec_t sector, sec_t numSectors, void *buffer)
{
	return !ataReadSectors(0, (u64)sector, numSectors, buffer);
}

static bool __ataa_writeSectors(sec_t sector, sec_t numSectors, void *buffer)
{
	return !ataWriteSectors(0, (u64)sector, numSectors, buffer);
}

static bool __ataa_clearStatus(void)
{
	return true;
}

static bool __ataa_shutdown(void)
{
	return true;
}

static bool __atab_startup(void)
{
	return ataIsInserted(1);
}

static bool __atab_isInserted(void)
{
	return ataIsInserted(1);
}

static bool __atab_readSectors(sec_t sector, sec_t numSectors, void *buffer)
{
	return !ataReadSectors(1, (u64)sector, numSectors, buffer);
}

static bool __atab_writeSectors(sec_t sector, sec_t numSectors, void *buffer)
{
	return !ataWriteSectors(1, (u64)sector, numSectors, buffer);
}

static bool __atab_clearStatus(void)
{
	return true;
}

static bool __atab_shutdown(void)
{
	return true;
}

static bool __ata1_startup(void)
{
	return ataIsInserted(2);
}

static bool __ata1_isInserted(void)
{
	return ataIsInserted(2);
}

static bool __ata1_readSectors(sec_t sector, sec_t numSectors, void *buffer)
{
	return !ataReadSectors(2, (u64)sector, numSectors, buffer);
}

static bool __ata1_writeSectors(sec_t sector, sec_t numSectors, void *buffer)
{
	return !ataWriteSectors(2, (u64)sector, numSectors, buffer);
}

static bool __ata1_clearStatus(void)
{
	return true;
}

static bool __ata1_shutdown(void)
{
	return true;
}

const DISC_INTERFACE __io_ataa = {
	DEVICE_TYPE_GC_ATA,
	FEATURE_MEDIUM_CANREAD | FEATURE_MEDIUM_CANWRITE | FEATURE_GAMECUBE_SLOTA,
	(FN_MEDIUM_STARTUP)&__ataa_startup,
	(FN_MEDIUM_ISINSERTED)&__ataa_isInserted,
	(FN_MEDIUM_READSECTORS)&__ataa_readSectors,
	(FN_MEDIUM_WRITESECTORS)&__ataa_writeSectors,
	(FN_MEDIUM_CLEARSTATUS)&__ataa_clearStatus,
	(FN_MEDIUM_SHUTDOWN)&__ataa_shutdown
} ;
const DISC_INTERFACE __io_atab = {
	DEVICE_TYPE_GC_ATA,
	FEATURE_MEDIUM_CANREAD | FEATURE_MEDIUM_CANWRITE | FEATURE_GAMECUBE_SLOTB,
	(FN_MEDIUM_STARTUP)&__atab_startup,
	(FN_MEDIUM_ISINSERTED)&__atab_isInserted,
	(FN_MEDIUM_READSECTORS)&__atab_readSectors,
	(FN_MEDIUM_WRITESECTORS)&__atab_writeSectors,
	(FN_MEDIUM_CLEARSTATUS)&__atab_clearStatus,
	(FN_MEDIUM_SHUTDOWN)&__atab_shutdown
} ;
const DISC_INTERFACE __io_ata1 = {
	DEVICE_TYPE_GC_ATA,
	FEATURE_MEDIUM_CANREAD | FEATURE_MEDIUM_CANWRITE | FEATURE_GAMECUBE_PORT1,
	(FN_MEDIUM_STARTUP)&__ata1_startup,
	(FN_MEDIUM_ISINSERTED)&__ata1_isInserted,
	(FN_MEDIUM_READSECTORS)&__ata1_readSectors,
	(FN_MEDIUM_WRITESECTORS)&__ata1_writeSectors,
	(FN_MEDIUM_CLEARSTATUS)&__ata1_clearStatus,
	(FN_MEDIUM_SHUTDOWN)&__ata1_shutdown
} ;