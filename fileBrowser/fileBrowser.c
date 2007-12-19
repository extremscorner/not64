/* fileBrowser.c - Actual declarations of all the fileBrowser function pointers, etc
   by Mike Slegeir for Mupen64-GC
 */

#include "fileBrowser.h"

fileBrowser_file* romFile_topLevel;

int (*romFile_readDir)(fileBrowser_file*, fileBrowser_file**);
int (*romFile_readFile)(fileBrowser_file*, void*, unsigned int);
int (*romFile_seekFile)(fileBrowser_file*, unsigned int, unsigned int);

int (*saveFile_readFile)(fileBrowser_file*, void*, unsigned int);
int (*saveFile_writeFile)(fileBrowser_file*, void*, unsigned int);

