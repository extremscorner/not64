/* fileBrowser-CARD.h - fileBrowser CARD module
   by emu_kidid for Mupen64-GC
 */

#ifndef FILE_BROWSER_CARD_H
#define FILE_BROWSER_CARD_H

#include "fileBrowser.h"

extern fileBrowser_file topLevel_CARD_SlotA;
extern fileBrowser_file topLevel_CARD_SlotB;
#define saveDir_CARD_SlotA topLevel_CARD_SlotA
#define saveDir_CARD_SlotB topLevel_CARD_SlotB

int fileBrowser_CARD_readDir(fileBrowser_file*, fileBrowser_file**);
int fileBrowser_CARD_readFile(fileBrowser_file*, void*, unsigned int);
int fileBrowser_CARD_writeFile(fileBrowser_file*, void*, unsigned int);
int fileBrowser_CARD_seekFile(fileBrowser_file*, unsigned int, unsigned int);
int fileBrowser_CARD_init(fileBrowser_file* file);
int fileBrowser_CARD_deinit(fileBrowser_file* file);

#endif

