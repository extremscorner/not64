/* fileBrowser-DVD.c - fileBrowser DVD module
   by emu_kidid for Mupen64-GC
 */

#include "fileBrowser.h"
#include "../main/gc_dvd.h"
#include <string.h>
#include <malloc.h>
#include <ogc/dvd.h>

extern unsigned int isWii;

fileBrowser_file topLevel_DVD =
	{ "\\", // file name
	  0,         // discoffset
	  0,         // offset
	  0,         // size
	  FILE_BROWSER_ATTR_DIR
	 };

int fileBrowser_DVD_readDir(fileBrowser_file* ffile, fileBrowser_file** dir){	
	//Lets execute a normal command
	dvd_read_id();
	//If it failed, we must mount the drive again
	if(dvd_get_error() != 0) {
		if(!isWii)
			DVD_Mount ();
		if(isWii) {
			DVD_Reset(DVD_RESETHARD);
			dvd_read_id();
		}
	}

	// Call the corresponding DVD function
	int num_entries = dvd_read_directoryentries(ffile->discoffset,ffile->size);
	
	// If it was not successful, just return the error
	if(num_entries <= 0) return num_entries;
	
	// Convert the DVD "file" data to fileBrowser_files
	*dir = malloc( num_entries * sizeof(fileBrowser_file) );
	int i;
	for(i=0; i<num_entries; ++i){
		strcpy( (*dir)[i].name, file[i].name );
		(*dir)[i].discoffset = ((file[i].sector)*2048);
		(*dir)[i].offset = 0;
		(*dir)[i].size   = file[i].size;
		(*dir)[i].attr   = file[i].flags; //2 is a dir
	}
	return num_entries;
}

int fileBrowser_DVD_seekFile(fileBrowser_file* file, unsigned int where, unsigned int type){
	if(type == FILE_BROWSER_SEEK_SET) file->offset = where;
	else if(type == FILE_BROWSER_SEEK_CUR) file->offset += where;
	else file->offset = file->size + where;
	return 0;
}

int fileBrowser_DVD_readFile(fileBrowser_file* file, void* buffer, unsigned int length){
	int bytesread = read_safe(buffer,file->discoffset,length);
	file->offset += bytesread;
	return bytesread;
}

int fileBrowser_DVD_init(fileBrowser_file* file) {
	DVD_Init();
	if(!isWii)
		DVD_Mount ();
	if(isWii) {
		DVD_Reset(DVD_RESETHARD);
		dvd_read_id();
	}
	if(dvd_get_error() == 0)
		return 0;
	return dvd_get_error();
}

int fileBrowser_DVD_deinit(fileBrowser_file* file) {
	dvd_motor_off();
	return 0;
}

