/* DEBUG.h - DEBUG interface
   by Mike Slegeir for Mupen64-GC
 */

#ifndef DEBUG_H
#define DEBUG_H

// The max number of strings that will be held onto
#define DEBUG_NUM_STRINGS 25
// Amount of time each string will be held onto
#define DEBUG_STRING_LIFE 2.0f
// Dimensions of array returned by get_text
#define DEBUG_TEXT_WIDTH  64
#define DEBUG_TEXT_HEIGHT 20

// Pre-formatted string (use sprintf before sending to print)
void DEBUG_print(char* string);

// Should be called before get_text. Ages the strings, and remove old ones
void DEBUG_update(float dt);

// Returns pointer to an array of char*
char** GUI_get_text(void);

#endif


