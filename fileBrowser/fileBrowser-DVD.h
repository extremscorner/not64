/* fileBrowser-DVD.h - fileBrowser DVD module
   by emu_kidid for Mupen64-GC
 */

#ifndef FILE_BROWSER_DVD_H
#define FILE_BROWSER_DVD_H

#include "fileBrowser.h"

extern fileBrowser_file topLevel_DVD;

int fileBrowser_DVD_readDir(fileBrowser_file*, fileBrowser_file**);
int fileBrowser_DVD_readFile(fileBrowser_file*, void*, unsigned int);
int fileBrowser_DVD_seekFile(fileBrowser_file*, unsigned int, unsigned int);
int fileBrowser_DVD_init(void);
int fileBrowser_DVD_deinit(void);

#endif

