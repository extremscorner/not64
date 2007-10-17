/* TEXT.h - Helper functions for reformatting text
   by Mike Slegeir for Mupen64-GC
 */

#ifndef TEXT_H
#define TEXT_H

// Maximum number of lines that can be in one split
#define TEXT_MAX_SPLIT 5
#define TEXT_WIDTH     35

//extern char TEXT_split_lines[TEXT_MAX_SPLIT][TEXT_WIDTH];

// Splits the text on newlines
// string:  the string to split
// RETURNS: number of lines split
//          Fills out TEXT_split[0 thru return value]
int TEXT_split(char* string);

#define TEXT_SPACE_PER_TAB 3
// Expands tabs into TEXT_SPACE_PER_TAB spaces
// string:      string to expand
// Assumes buffer size is TEXT_WIDTH, won't overflow that size
void TEXT_expand(char* string);

#endif

