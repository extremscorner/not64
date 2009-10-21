/* DEBUG.h - DEBUG interface
   by Mike Slegeir for Mupen64-GC
 */

#ifndef DEBUG_H
#define DEBUG_H

//#define SDPRINT

#define DBG_DLIST 0
#define DBG_DLISTTYPE 1
#define DBG_RSPINFO 2
#define DBG_RSPINFO1 3
#define DBG_TXINFO 4
#define DBG_TXINFO1 5
#define DBG_VIINFO 6
#define DBG_CCINFO 7
#define DBG_BLINFO 8
#define DBG_AUDIOINFO 9
#define DBG_MEMFREEINFO 10
#define DBG_CACHEINFO 11
#define DBG_PROFILE_GFX 19
#define DBG_PROFILE_AUDIO 20
#define DBG_PROFILE_IDLE 21
#define DBG_PROFILE_TLB 22
#define DBG_PROFILE_FP 23
#define DBG_PROFILE_COMP 24
#define DBG_PROFILE_INTERP 25
#define DBG_PROFILE_TRAMP 26
#define DBG_PROFILE_FUNCS 27
#define DBG_STATSBASE 12 // ALL stats print from this line onwards
#define DBG_SDGECKOOPEN 0xFC
#define DBG_SDGECKOCLOSE 0xFD
#define DBG_SDGECKOPRINT 0xFE
#define DBG_USBGECKO 0xFF

//DEBUG_stats defines
#define STAT_TYPE_ACCUM 0
#define STAT_TYPE_AVGE  1
#define STAT_TYPE_CLEAR 2

#define STATS_RECOMPCACHE 	0
#define STATS_CACHEMISSES	1
#define STATS_FCOUNTER		2	//FRAME counter
#define STATS_THREE			3

extern char txtbuffer[1024];
// Amount of time each string will be held onto
#define DEBUG_STRING_LIFE 5
// Dimensions of array returned by get_text
#define DEBUG_TEXT_WIDTH  100
#define DEBUG_TEXT_HEIGHT 40

#ifdef __cplusplus
extern "C" {
#endif

// Pre-formatted string (use sprintf before sending to print)
void DEBUG_print(char* string,int pos);
void DEBUG_stats(int stats_id, char *info, unsigned int stats_type, unsigned int adjustment_value);

// Should be called before get_text. Ages the strings, and remove old ones
void DEBUG_update(void);

// Returns pointer to an array of char*
char** DEBUG_get_text(void);

#ifdef __cplusplus
}
#endif

#endif


