/* GUI.c - GUI interface
   by Mike Slegeir for Mupen64-GC
 */

#include "GUI.h"

// This is the text that gets displayed
static char text[GUI_TEXT_HEIGHT][GUI_TEXT_WIDTH];
// Rather than shifting text each time too much has been written, wrap around
// text_zero_line keeps track of the 'first' line
static int text_zero_line;
static int text_next_line;
// Keep track of whether we've had to wrap the text
static char isWrapped;

void GUI_print(char* string){
	// First split the string into lines of text
	// TODO: Split('\n')
	
	// Next, expand tabs to spaces
	// TODO: Expand tabs
	
	// Finally, fill out the next lines of text with the strings
}

void GUI_clear(void){
	// NULL out all the lines of text
	int i;
	for(i=0; i<GUI_TEXT_HEIGHT; ++i)
		text[i][0] = 0;
}

void GUI_update(void){
	if(isWrapped){
		// Unwrap the text so it is linear again
		// TOOD: Unwrap
	}
	isWrapped = 0;
}

char* GUI_get_text(void){
	// Make sure the text we give is properly ordered
	if(isWrapped) GUI_update();
	
	// Simply return our array
	return &text;
}

