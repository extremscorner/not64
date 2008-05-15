/* fileBrowser-CARD.c - fileBrowser CARD module
   by emu_kidid for Mupen64-GC
 */
#include <stdio.h>
#include <string.h>
#include <gccore.h>
#include <malloc.h>
#include <ogc/card.h>
#include "fileBrowser.h"

unsigned char SysArea[CARD_WORKAREA] ATTRIBUTE_ALIGN (32);
void card_removed_cb(s32 chn, s32 result){ CARD_Unmount(chn); }

fileBrowser_file topLevel_CARD_SlotA =
	{ "\0", // file name
	  CARD_SLOTA, // slot
	  0, // offset
	  0, // size
	  FILE_BROWSER_ATTR_DIR
	 };

fileBrowser_file topLevel_CARD_SlotB =
	{ "\0", // file name
	  CARD_SLOTB, // slot
	  0, // offset
	  0, // size
	  FILE_BROWSER_ATTR_DIR
	 };
	 
int mount_card(int slot) {
	/*** Pass company identifier and number ***/
    CARD_Init ("N64E", "OS");
	
    int Slot_error = CARD_Mount (slot, SysArea, card_removed_cb);
 
    
    if (Slot_error < 0) {
    	int i = 0;
    	for(i = 0; i<50; i++)
    		Slot_error = CARD_Mount (slot, SysArea, card_removed_cb);
	}
	return Slot_error;
}

int fileBrowser_CARD_readDir(fileBrowser_file* file, fileBrowser_file** dir){
	return 0;
}

int fileBrowser_CARD_seekFile(fileBrowser_file* file, unsigned int where, unsigned int type){
	if(type == FILE_BROWSER_SEEK_SET) file->offset = where;
	else if(type == FILE_BROWSER_SEEK_CUR) file->offset += where;
	else file->offset = file->size + where;
	
	return 0;
}

int fileBrowser_CARD_readFile(fileBrowser_file* file, void* buffer, unsigned int length){
	char *tbuffer;
	card_file CardFile;
	int slot = file->discoffset;
	unsigned int SectorSize = 0;
    CARD_GetSectorSize (slot, &SectorSize);
    
	if(CARD_Open(slot, (const char*)file->name, &CardFile) != CARD_ERROR_NOFILE){
		int size = length;
	    if((size % SectorSize) != 0)
	      	size = ((length/SectorSize)+1) * SectorSize;
	    tbuffer = memalign(32,size);
		if(CARD_Read(&CardFile,tbuffer, size, file->offset) == 0){
			file->offset += length;
			memcpy(buffer,tbuffer,length);
		}
		else 
		{
			CARD_Close(&CardFile);
			return -1;
		}
		return length;
   }
	return -1;
}

int fileBrowser_CARD_writeFile(fileBrowser_file* file, void* buffer, unsigned int length){
	
	card_file CardFile;
	int slot = file->discoffset;
	int status = 0;
	
	//sectorsize stuff to keep memcard filesizes and writes happy
	unsigned int SectorSize = 0;
    CARD_GetSectorSize (slot, &SectorSize);
	int memcardLength = length % SectorSize;
	if(memcardLength)
		memcardLength = (((length/SectorSize)*SectorSize) + SectorSize);
	else
		memcardLength = length;
	status = CARD_Open(slot, (const char*)file->name, &CardFile);
	if(status == CARD_ERROR_NOFILE){
		status = CARD_Create(slot, (const char*)file->name, memcardLength, &CardFile);
	}
	
	if(status == CARD_ERROR_READY) {
		if(CARD_Write(&CardFile, buffer, length, 0) == CARD_ERROR_READY) {
			file->offset += length;
			CARD_Close(&CardFile);
			return length;
		}
	}
	return -1;
}

int fileBrowser_CARD_init(fileBrowser_file* file) {
	int slot = file->discoffset;
	return mount_card(slot); //mount via slot number
}

int fileBrowser_CARD_deinit(fileBrowser_file* file) {
	int slot = file->discoffset;
	CARD_Unmount(slot); //unmount via slot number
	return 0;
}

