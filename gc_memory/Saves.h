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

void loadEeprom(fileBrowser_file* savepath);
void saveEeprom(fileBrowser_file* savepath);

void loadMempak(fileBrowser_file* savepath);
void saveMempak(fileBrowser_file* savepath);

void loadSram(fileBrowser_file* savepath);
void saveSram(fileBrowser_file* savepath);

void loadFlashram(fileBrowser_file* savepath);
void saveFlashram(fileBrowser_file* savepath);

#endif

