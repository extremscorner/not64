/* fileBrowser-DVD.c - fileBrowser DVD module
   by emu_kidid for Mupen64-GC
 */

#include "fileBrowser.h"
#include "../main/gc_dvd.h"
#include <string.h>
#include <malloc.h>
#include <ogc/dvd.h>

/* DVD Globals */
extern unsigned int isWii;
extern int previously_initd;
int dvdInitialized;

extern struct
{
	char name[128];
	int flags;
	int sector, size;
} file[MAXIMUM_ENTRIES_PER_DIR]; //150 files per dir, MAXIMUM.

/* Worked out manually from my original Disc */
#define OOT_OFFSET 0x54FBEEF4
#define MQ_OFFSET  0x52CCC5FC
#define ZELDA_SIZE 0x2000000

fileBrowser_file topLevel_DVD =
	{ "\\", // file name
	  0,         // discoffset
	  0,         // offset
	  0,         // size
	  FILE_BROWSER_ATTR_DIR
	};

int DVD_check_state() {

    if(dvd_get_error() == 0){
            dvdInitialized = 1;
            return 0;
    }
    else {
        while(dvd_get_error()) {
            if(!isWii){
                DVD_Mount ();   
                if(dvd_get_error())
                    DVD_Reset(DVD_RESETHARD);
            }
            if(isWii) {
                DVD_Reset(DVD_RESETHARD);
                dvd_read_id();
            }
        }
    }
    dvdInitialized = 1;
    return 0;

}
		 
	 
int fileBrowser_DVD_readDir(fileBrowser_file* ffile, fileBrowser_file** dir){	
	
	dvd_read_id();
	DVD_check_state();
	
	int num_entries = 0;
	
	if (!memcmp((void*)0x80000000, "D43U01", 6)) { //OoT bonus disc support.
		num_entries = 2;
		*dir = malloc( num_entries * sizeof(fileBrowser_file) );
		strcpy( (*dir)[0].name, "Zelda - Ocarina of Time");
		(*dir)[0].discoffset = OOT_OFFSET;
		(*dir)[0].offset = 0;
		(*dir)[0].size   = ZELDA_SIZE;
		(*dir)[0].attr	 = 0;
		strcpy( (*dir)[1].name, "Zelda - Ocarina of Time MQ" );
		(*dir)[1].discoffset = MQ_OFFSET;
		(*dir)[1].offset = 0;
		(*dir)[1].size   = ZELDA_SIZE;
		(*dir)[1].attr	 = 0;
		return num_entries;
	}
	
	// Call the corresponding DVD function
	num_entries = dvd_read_directoryentries(ffile->discoffset,ffile->size);
	
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
		(*dir)[i].attr	 = 0;
		if(file[i].flags == 2)//on DVD, 2 is a dir
			(*dir)[i].attr   = FILE_BROWSER_ATTR_DIR; 
		if((*dir)[i].name[strlen((*dir)[i].name)-1] == '/' )
			(*dir)[i].name[strlen((*dir)[i].name)-1] = 0;	//get rid of trailing '/'
	}
		
	if(strlen((*dir)[0].name) == 0)
		strcpy( (*dir)[0].name, ".." );
	
	return num_entries;
}

int fileBrowser_DVD_seekFile(fileBrowser_file* file, unsigned int where, unsigned int type){
	if(type == FILE_BROWSER_SEEK_SET) file->offset = where;
	else if(type == FILE_BROWSER_SEEK_CUR) file->offset += where;
	else file->offset = file->size + where;
	return 0;
}

int fileBrowser_DVD_readFile(fileBrowser_file* file, void* buffer, unsigned int length){
	DVD_check_state();
	int bytesread = read_safe(buffer,file->discoffset+file->offset,length);
	file->offset += bytesread;
	return bytesread;
}

int fileBrowser_DVD_init(fileBrowser_file* file) {

	dvd_read_id();
	if(dvd_get_error() == 0)
		return 0;
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
	return 0;
}

