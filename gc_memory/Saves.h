/* Saves.h - Defines/globals for saving files
   by Mike Slegeir for Mupen64-GC
 */

#ifndef SAVES_H
#define SAVES_H

extern int  savetype;
extern char savepath[];

#define SELECTION_SLOT_A    0
#define SELECTION_SLOT_B    1
#define SELECTION_TYPE_SD   2
#define SELECTION_TYPE_MEM  0

void loadEeprom(void);
void saveEeprom(void);

void loadMempak(void);
void saveMempak(void);

void loadSram(void);
void saveSram(void);

void loadFlashram(void);
void saveFlashram(void);

#endif

