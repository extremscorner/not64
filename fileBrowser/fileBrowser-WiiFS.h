/* fileBrowser-WiiFS.h - fileBrowser Wii FileSystem module
   by Mike Slegeir for Mupen64-GC
 */

#ifndef FILE_BROWSER_WIIFS_H
#define FILE_BROWSER_WIIFS_H

#include "fileBrowser.h"

extern fileBrowser_file topLevel_WiiFS;
extern fileBrowser_file saveDir_WiiFS;

int fileBrowser_WiiFS_readDir(fileBrowser_file*, fileBrowser_file**);
int fileBrowser_WiiFS_readFile(fileBrowser_file*, void*, unsigned int);
int fileBrowser_WiiFS_writeFile(fileBrowser_file*, void*, unsigned int);
int fileBrowser_WiiFS_seekFile(fileBrowser_file*, unsigned int, unsigned int);
int fileBrowser_WiiFS_init(fileBrowser_file* f);
int fileBrowser_WiiFS_deinit(fileBrowser_file* f);

int fileBrowser_WiiFSROM_readFile(fileBrowser_file*, void*, unsigned int);
int fileBrowser_WiiFSROM_init(fileBrowser_file*);
int fileBrowser_WiiFSROM_deinit(fileBrowser_file*);

#endif

