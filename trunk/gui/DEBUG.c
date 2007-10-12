/* DEBUG.c - DEBUG interface
   by Mike Slegeir for Mupen64-GC
 */

#include "DEBUG.h"

typedef struct {
	
	char  valid;
	float life;
	char  text[DEBUG_TEXT_WIDTH];
	
} DEBUG_string;

static DEBUG_string strings[DEBUG_NUM_STRINGS];
static int nextString;
static char* text[DEBUG_TEXT_HEIGHT];

void DEBUG_print(char* string){
	// First split the string into lines of text
	// TODO: Split('\n')
	
	// Next, expand tabs to spaces
	// TODO: Expand tabs
	
	// Finally, create a DEBUG_string for each line
	//   and mark it valid and fill in it's life
}

void DEBUG_update(float dt){
	// Shorten the strings life by dt
	int i;
	for(i=0; i<DEBUG_NUM_STRINGS; ++i){
		strings[i].life -= dt;
		// Mark them as invalid if they've died
		if(strings[i].life <= 0.0f)
			strings[i].valid = 0;
	}
	
	// Fill out text with living strings oldest first
	// TODO: Fill out text
}

char* DEBUG_get_text(void){
	// FIXME: This doesn't work properly, we're returning
	//          an array of char*, not strings
	return &text;
}

