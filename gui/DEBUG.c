/* DEBUG.c - DEBUG interface
   by Mike Slegeir for Mupen64-GC
 */

#include <string.h>
#include "DEBUG.h"
#include "TEXT.h"

typedef struct {
	
	char  valid;
	float life;
	char  text[DEBUG_TEXT_WIDTH];
	
} DEBUG_string;

static DEBUG_string strings[DEBUG_NUM_STRINGS];
static int nextString;
static char* text[DEBUG_TEXT_HEIGHT];
static int numLiveStrings;

void DEBUG_print(char* string){
	// First split the string into lines of text
	int num_lines = TEXT_split(string);
	
	// Next, expand tabs to spaces
	int i;
	for(i=0; i<num_lines; ++i) TEXT_expand( &TEXT_split_lines[i] );
	
	// Finally, create a DEBUG_string for each line
	//   and mark it valid and fill in it's life
	int j=0;
	for(i=0; i<num_lines; ++i){
		for(; j<DEBUG_NUM_STRINGS; ++j)
			if(!strings[j].valid){
				strings[j].valid = 1;
				strings[j].life  = DEBUG_STRING_LIFE;
				strncpy( &strings[j].text, &TEXT_split_lines[i], DEBUG_TEXT_WIDTH );
				break;
			}
	}
}

void DEBUG_update(float dt){
	// Shorten the strings life by dt
	int i;
	for(i=0; i<DEBUG_NUM_STRINGS; ++i){
		strings[i].life -= dt;
		// Mark them as invalid if they've died
		if(strings[i].valid && strings[i].life <= 0.0f){
			strings[i].valid   = 0;
			strings[i].text[0] = 0;
			--numLiveStrings;
		}
	}
	
	// Fill out text with living strings oldest first
	
	// Selected is a boolean array of whether we've
	//   already selected each string to be in text
	static char selected[DEBUG_NUM_STRINGS];
	for(i=0; i<DEBUG_NUM_STRINGS; ++i) selected[i] = 0;
	
	for(i=0; i<numLiveStrings; ++i){
		// Find the oldest string of the ones we haven't selected
		int j,k;
		float min = DEBUG_STRING_LIFE;
		for(j=0; j<DEBUG_NUM_STRINGS; ++j){
			if(strings[j].valid && !selected[i] && strings[j].life <= min){
				k = j;
				min = strings[j].life;
			}
		}
		
		// Set the next line of text to this one
		text[i] = &strings[k].text;
		selected[k] = 1;
	}
}

char** DEBUG_get_text(void){
	return &text;
}

