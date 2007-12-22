/* GUI.h - GUI interface
   by Mike Slegeir for Mupen64-GC
 */

#ifndef GUI_H
#define GUI_H

#include <ogc/gx.h>

// Dimensions of array returned by get_text
#define GUI_TEXT_WIDTH  35
#define GUI_TEXT_HEIGHT 16

// Some defined colors to make things easier
extern GXColor GUI_color_default;
extern GXColor GUI_color_highlighted;
extern GXColor GUI_color_special;

// Pre-formatted string (use sprintf before sending to print)
void GUI_print(char* string);
void GUI_print_color(char* string, GXColor color);

// Clear text to be displayed each frame
void GUI_clear(void);

// Should be called before a call to get_text, reformats text
void GUI_update(void);

// Returns pointer to an array of char*
// Call BEFORE you call get_colors
char** GUI_get_text(void);

// Returns an array of GXColor
GXColor* GUI_get_colors(void);

#endif

