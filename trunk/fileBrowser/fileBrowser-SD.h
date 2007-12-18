/* fileBrowser-SD.h - fileBrowser SD module
   by Mike Slegeir for Mupen64-GC
 */

#include "fileBrowser.h"

extern fileBrowser_file topLevel_SD_SlotA;
extern fileBrowser_file topLevel_SD_SlotB;

int fileBrowser_SD_readDir(fileBrowser_file*, fileBrowser_file**);
int fileBrowser_SD_readFile(fileBrowser_file*, void*, unsigned int);
int fileBrowser_SD_writeFile(fileBrowser_file*, void*, unsigned int);
int fileBrowser_SD_seekFile(fileBrowser_file*, unsigned int, unsigned int);

