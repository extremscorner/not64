/* fileBrowser-libfat.c - fileBrowser for any devices using libfat
   by Mike Slegeir for Mupen64-GC
 */

#include <fat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/dir.h>
#include "fileBrowser.h"
#include <sdcard/gcsd.h>

#if 1
#ifdef HW_RVL
#include <sdcard/wiisd_io.h>
#include <ogc/usbstorage.h>
const DISC_INTERFACE* frontsd = &__io_wiisd;
const DISC_INTERFACE* usb = &__io_usbstorage;
#endif
const DISC_INTERFACE* carda = &__io_gcsda;
const DISC_INTERFACE* cardb = &__io_gcsdb;
#endif

char sdMounted  = 0;
char usbMounted = 0;

fileBrowser_file topLevel_libfat_Default =
	{ "sd:/wii64/roms", // file name
	  0, // sector
	  0, // offset
	  0, // size
	  FILE_BROWSER_ATTR_DIR
	 };
	 
fileBrowser_file topLevel_libfat_USB =
	{ "usb:/wii64/roms", // file name
	  0, // sector
	  0, // offset
	  0, // size
	  FILE_BROWSER_ATTR_DIR
	 };

fileBrowser_file saveDir_libfat_Default =
	{ "sd:/wii64/saves/",
	  0,
	  0,
	  0,
	  FILE_BROWSER_ATTR_DIR
	 };
	 
fileBrowser_file saveDir_libfat_USB =
	{ "usb:/wii64/saves/",
	  0,
	  0,
	  0,
	  FILE_BROWSER_ATTR_DIR
	 };

int fileBrowser_libfat_readDir(fileBrowser_file* file, fileBrowser_file** dir){
	DIR_ITER* dp = diropen( file->name );
	if(!dp) return FILE_BROWSER_ERROR;
	struct stat fstat;
	
	// Set everything up to read
	char filename[MAXPATHLEN];
	int num_entries = 2, i = 0;
	*dir = malloc( num_entries * sizeof(fileBrowser_file) );
	// Read each entry of the directory
	while( dirnext(dp, filename, &fstat) == 0 ){
		// Make sure we have room for this one
		if(i == num_entries){
			++num_entries;
			*dir = realloc( *dir, num_entries * sizeof(fileBrowser_file) ); 
		}
		sprintf((*dir)[i].name, "%s/%s", file->name, filename);
		(*dir)[i].offset = 0;
		(*dir)[i].size   = fstat.st_size;
		(*dir)[i].attr   = (fstat.st_mode & S_IFDIR) ?
		                     FILE_BROWSER_ATTR_DIR : 0;
		++i;
	}
	
	dirclose(dp);
	return num_entries;
}

int fileBrowser_libfat_seekFile(fileBrowser_file* file, unsigned int where, unsigned int type){
	if(type == FILE_BROWSER_SEEK_SET) file->offset = where;
	else if(type == FILE_BROWSER_SEEK_CUR) file->offset += where;
	else file->offset = file->size + where;
	
	return 0;
}

int fileBrowser_libfat_readFile(fileBrowser_file* file, void* buffer, unsigned int length){
	FILE* f = fopen( file->name, "rb" );
	if(!f) return FILE_BROWSER_ERROR;
	
	fseek(f, file->offset, SEEK_SET);
	int bytes_read = fread(buffer, 1, length, f);
	if(bytes_read > 0) file->offset += bytes_read;
	
	fclose(f);
	return bytes_read;
}

int fileBrowser_libfat_writeFile(fileBrowser_file* file, void* buffer, unsigned int length){
	FILE* f = fopen( file->name, "wb" );
	if(!f) return FILE_BROWSER_ERROR;
	
	fseek(f, file->offset, SEEK_SET);
	int bytes_read = fwrite(buffer, 1, length, f);
	if(bytes_read > 0) file->offset += bytes_read;
	
	fclose(f);
	return bytes_read;
}

/* call fileBrowser_libfat_init as much as you like for all devices
    - returns 0 on device not present/error
    - returns 1 on ok
*/
int fileBrowser_libfat_init(fileBrowser_file* f){
 	int res = 0;
#ifdef HW_RVL
  if(f->name[0] == 's') {
    if(!sdMounted) {
     	if(frontsd->startup()) {
       	res |= fatMountSimple ("sd", frontsd);
     	}
     	else if(carda->startup() && !res) {
     	  res |= fatMountSimple ("sd", carda);
   	  }
   	  else if(cardb->startup() && !res) {
     	  res |= fatMountSimple ("sd", cardb);
   	  }
   	  if(res) sdMounted = 1;
   	  return res;
 	  }
 	  else
 	    return 1;
 	}
 	else if(f->name[0] == 'u') {
   	if(!usbMounted) {
     	if(usb->startup()) {
     	  res |= fatMountSimple ("usb", usb);
      }
      if(res) usbMounted = 1;
      return res;
    }
    else
      return 1;
  }
  return res;
#else
 	if(carda->startup()) {
   	res |= fatMountSimple ("sd", carda);
 	}
 	else if(cardb->startup() && !res) {
   	res |= fatMountSimple ("sd", cardb);
 	}
	return res;
#endif
}

int fileBrowser_libfat_deinit(fileBrowser_file* f){
  //we can't support multiple device re-insertion
  //because there's no device removed callbacks
	return 0;
}


/* Special for ROM loading only */
static FILE* fd;

int fileBrowser_libfatROM_deinit(fileBrowser_file* f){
	if(fd)
		fclose(fd);
	fd = NULL;
	
	return 0;
}
	
int fileBrowser_libfatROM_readFile(fileBrowser_file* file, void* buffer, unsigned int length){
	if(!fd) fd = fopen( file->name, "rb");
	
	fseek(fd, file->offset, SEEK_SET);
	int bytes_read = fread(buffer, 1, length, fd);
	if(bytes_read > 0) file->offset += bytes_read;

	return bytes_read;
}

