/* DEBUG.h - DEBUG interface
   by Mike Slegeir for Mupen64-GC
 */

#ifndef DEBUG_H
#define DEBUG_H

// The max number of strings that will be held onto
#define DEBUG_NUM_STRINGS
// Amount of time each string will be held onto
#define DEBUG_STRING_LIFE
// Dimensions of array returned by get_text
#define GUI_TEXT_WIDTH  64
#define GUI_TEXT_HEIGHT 20

// Pre-formatted string (use sprintf before sending to print)
void DEBUG_print(char* string);

// Should be called before get_text. Ages the strings, and remove old ones
void DEBUG_update(float dt);

// Returns pointer to a 2D char array of dimensions WIDTH,HEIGHT
char* DEBUG_get_text(void);

#endif


