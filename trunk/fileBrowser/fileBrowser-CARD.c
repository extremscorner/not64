/* fileBrowser-CARD.c - fileBrowser CARD module
   by emu_kidid for Mupen64-GC
 */

#include <ogc/card.h>
#include "fileBrowser.h"

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

int fileBrowser_CARD_readDir(fileBrowser_file* file, fileBrowser_file** dir){
/*	DIR* sddir = NULL;
	// Call the corresponding SDCARD function
	int num_entries = SDCARD_readDir(&file->name, &sddir);
	
	// If it was not successful, just return the error
	if(num_entries <= 0) return num_entries;
	
	// Convert the SDCARD data to fileBrowser_files
	*dir = malloc( num_entries * sizeof(fileBrowser_file) );
	int i;
	for(i=0; i<num_entries; ++i){
		sprintf((*dir)[i].name, "%s\\%s", file->name, sddir[i].fname);
		(*dir)[i].offset = 0;
		(*dir)[i].size   = sddir[i].fsize;
		(*dir)[i].attr   = sddir[i].fattr;
	}
	
	return num_entries;
	*/
	return 0;
}

int fileBrowser_CARD_seekFile(fileBrowser_file* file, unsigned int where, unsigned int type){
	if(type == FILE_BROWSER_SEEK_SET) file->offset = where;
	else if(type == FILE_BROWSER_SEEK_CUR) file->offset += where;
	else file->offset = file->size + where;
	
	return 0;
}

int fileBrowser_CARD_readFile(fileBrowser_file* file, void* buffer, unsigned int length){
	card_file CardFile;
	int slot = file->discoffset;
	
	if(CARD_Open(slot, &file->name[0], &CardFile) != CARD_ERROR_NOFILE){
		if(CARD_Read(&CardFile, buffer, length, file->offset) == 0)
			file->offset += length;
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
	
	status = CARD_Open(slot, &file->name[0], &CardFile);
	if(status == CARD_ERROR_NOFILE)
		status = CARD_Create(slot, &file->name[0], memcardLength, &CardFile);
	
	if(status == CARD_ERROR_READY) {
		if(CARD_Write(&CardFile, buffer, length, 0) == CARD_ERROR_READY) {
			file->offset += length;
			CARD_Close(&CardFile);
			return length;
		}
	}
	return -1;
}

