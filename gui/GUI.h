/* GUI.h - GUI interface
   by Mike Slegeir for Mupen64-GC
 */

#ifndef GUI_H
#define GUI_H

// Dimensions of array returned by get_text
#define GUI_TEXT_WIDTH  35
#define GUI_TEXT_HEIGHT 17

// Pre-formatted string (use sprintf before sending to print)
void GUI_print(char* string);

// Clear text to be displayed each frame
void GUI_clear(void);

// Should be called before a call to get_text, reformats text
void GUI_update(void);

// Returns pointer to an array of char*
char** GUI_get_text(void);

#endif

