/* DEBUG.h - DEBUG interface
   by Mike Slegeir for Mupen64-GC
 */

#ifndef DEBUG_H
#define DEBUG_H

#define DBG_DLIST 0
#define DBG_DLISTTYPE 1
#define DBG_RSPINFO 2
#define DBG_RSPINFO1 3
#define DBG_TXINFO 4
#define DBG_TXINFO1 5
#define DBG_CCINFO 9
#define DBG_BLINFO 10
#define DBG_AUDIOINFO 6
#define DBG_SAVEINFO 7
#define DBG_CACHEINFO 8
#define DBG_PROFILE 9
#define DBG_DYNAREC_INTERP 11
#define DBG_DYNAREC_JUMP 12
#define DBG_USBGECKO 0xFF
static char txtbuffer[1024];
// Amount of time each string will be held onto
#define DEBUG_STRING_LIFE 3000
// Dimensions of array returned by get_text
#define DEBUG_TEXT_WIDTH  64
#define DEBUG_TEXT_HEIGHT 20

#ifdef __cplusplus
extern "C" {
#endif

// Pre-formatted string (use sprintf before sending to print)
void DEBUG_print(char* string,int pos);

// Should be called before get_text. Ages the strings, and remove old ones
void DEBUG_update(void);

// Returns pointer to an array of char*
char** DEBUG_get_text(void);

#ifdef __cplusplus
}
#endif

#endif


