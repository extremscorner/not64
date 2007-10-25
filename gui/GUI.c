/* GUI.c - GUI interface
   by Mike Slegeir for Mupen64-GC
 */

#include <string.h>
#include "GUI.h"
#include "TEXT.h"

// This is the text that gets displayed
static char text[GUI_TEXT_HEIGHT][GUI_TEXT_WIDTH];
static char* textptrs[GUI_TEXT_HEIGHT];
// Rather than shifting text each time too much has been written, wrap around
// text_zero_line keeps track of the 'first' line
static int text_zero_line;
static int text_next_line;
// Keep track of whether we've had to wrap the text
static char isWrapped;

extern char TEXT_split_lines[TEXT_MAX_SPLIT][TEXT_WIDTH];


void GUI_print(char* string){
	// First split the string into lines of text
	int num_lines = TEXT_split(string);
	
	// Next, expand tabs to spaces
	int i;
	for(i=0; i<num_lines; ++i) TEXT_expand( &TEXT_split_lines[i] );
	
	// Finally, fill out the next lines of text with the strings
	for(i=0; i<num_lines; ++i)
		strncpy( textptrs[text_next_line++ % GUI_TEXT_HEIGHT],
		         &TEXT_split_lines[i], GUI_TEXT_WIDTH );
	
	if(isWrapped) text_zero_line = text_next_line % GUI_TEXT_HEIGHT;
	else { 
		if(text_next_line >= GUI_TEXT_HEIGHT){
			isWrapped = 1;
			text_zero_line = text_next_line % GUI_TEXT_HEIGHT;
		}
	}
}

void GUI_clear(void){
	// NULL out all the lines of text and reset ptrs
	int i;
	for(i=0; i<GUI_TEXT_HEIGHT; ++i){
		text[i][0]  = 0;
		textptrs[i] = &text[i];
	}
	
	// Reset state
	text_zero_line = 0;
	text_next_line = 0;
	isWrapped = 0;
}

void GUI_update(void){
	if(isWrapped){
#if 0
		// Unwrap the text so it is linear again
		//   we have two indices: i,j
		//   i begins at the 'zero' index
		//   j begins at the beginning of the array
		//   Run j through each index,
		//     increment i until the last, don't wrap
		//   Swap the string at i with that at j
		int i=text_zero_line, j;
		for(j=0; j<GUI_TEXT_HEIGHT; ++j){
			// Swap textptrs
			char* temp  = textptrs[j];
			textptrs[j] = textptrs[i];
			textptrs[i] = temp;
			// Only run i to the last ptr
			if(i < GUI_TEXT_HEIGHT-1) ++i;
		}
#else
		char* temp[GUI_TEXT_HEIGHT];
		int i,j=text_zero_line;
		for(i=0; i<GUI_TEXT_HEIGHT; ++i, ++j)
			temp[i] = textptrs[j % GUI_TEXT_HEIGHT];
		for(i=0; i<GUI_TEXT_HEIGHT; ++i)
			textptrs[i] = temp[i];
#endif
		// Reset state
		text_zero_line = 0;
		text_next_line = 0;
	}
	// Although we've reordered the text,
	//   isWrapped should still be true
	//   because the buffer still wraps
	//   around.
}

char** GUI_get_text(void){
	// Make sure the text we give is properly ordered
	if(isWrapped) GUI_update();

	// Simply return our array
	return &textptrs;
}

