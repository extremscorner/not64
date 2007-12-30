/* Saves.h - Defines/globals for saving files
   by Mike Slegeir for Mupen64-GC
 */

#ifndef SAVES_H
#define SAVES_H

#include "../fileBrowser/fileBrowser.h"

/*
extern char saveEnabled;
extern int  savetype;
extern char savepath[];

#define SELECTION_SLOT_A    0
#define SELECTION_SLOT_B    1
#define SELECTION_TYPE_SD   2
#define SELECTION_TYPE_MEM  0*/

// Return 0 if load/save fails, 1 otherwise

int loadEeprom(fileBrowser_file* savepath);
int saveEeprom(fileBrowser_file* savepath);

int loadMempak(fileBrowser_file* savepath);
int saveMempak(fileBrowser_file* savepath);

int loadSram(fileBrowser_file* savepath);
int saveSram(fileBrowser_file* savepath);

int loadFlashram(fileBrowser_file* savepath);
int saveFlashram(fileBrowser_file* savepath);

#endif

