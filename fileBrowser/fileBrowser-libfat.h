/* fileBrowser-libfat.h - fileBrowser for any devices using libfat
   by Mike Slegeir for Mupen64-GC
 */

#ifndef FILE_BROWSER_LIBFAT_H
#define FILE_BROWSER_LIBFAT_H

extern fileBrowser_file topLevel_libfat_Default;  //GC SD Slots & Wii Front SD Slot
extern fileBrowser_file topLevel_libfat_USB;      //Wii only, USB
extern fileBrowser_file saveDir_libfat_Default;   //GC SD Slots & Wii Front SD Slot
extern fileBrowser_file saveDir_libfat_USB;       //Wii only, USB

int fileBrowser_libfat_readDir(fileBrowser_file*, fileBrowser_file**);
int fileBrowser_libfat_readFile(fileBrowser_file*, void*, unsigned int);
int fileBrowser_libfat_writeFile(fileBrowser_file*, void*, unsigned int);
int fileBrowser_libfat_seekFile(fileBrowser_file*, unsigned int, unsigned int);
int fileBrowser_libfat_init(fileBrowser_file* f);
int fileBrowser_libfat_deinit(fileBrowser_file* f);

int fileBrowser_libfatROM_readFile(fileBrowser_file*, void*, unsigned int);
int fileBrowser_libfatROM_deinit(fileBrowser_file* f);

void pauseRemovalThread();
void continueRemovalThread();

#endif
