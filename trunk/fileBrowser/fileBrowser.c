/* fileBrowser.c - Actual declarations of all the fileBrowser function pointers, etc
   by Mike Slegeir for Mupen64-GC
 */

#include "fileBrowser.h"

#define NULL 0

fileBrowser_file* romFile_topLevel;
fileBrowser_file* saveFile_dir;

int (*romFile_init)(void) = NULL;
int (*romFile_readDir)(fileBrowser_file*, fileBrowser_file**) = NULL;
int (*romFile_readFile)(fileBrowser_file*, void*, unsigned int) = NULL;
int (*romFile_seekFile)(fileBrowser_file*, unsigned int, unsigned int) = NULL;
int (*romFile_deinit)(void) = NULL;

int (*saveFile_init)(void) = NULL;
int (*saveFile_exists)(fileBrowser_file*) = NULL;
int (*saveFile_readFile)(fileBrowser_file*, void*, unsigned int) = NULL;
int (*saveFile_writeFile)(fileBrowser_file*, void*, unsigned int) = NULL;
int (*saveFile_deinit)(void) = NULL;

