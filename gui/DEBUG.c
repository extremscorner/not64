/* DEBUG.c - DEBUG interface
   by Mike Slegeir for Mupen64-GC
 */

#include <string.h>
#include "DEBUG.h"
#include "TEXT.h"

typedef struct _string {
	
	long long       start_tick;
	char            text[DEBUG_TEXT_WIDTH];
	unsigned int    checksum;
	unsigned int    num_lines;
	struct _string* next;
	struct _string* prev;
	
} DEBUG_string;

static DEBUG_string* head;
static DEBUG_string* tail;
static char* text[DEBUG_TEXT_HEIGHT];
static int numLiveStrings;

extern char TEXT_split_lines[TEXT_MAX_SPLIT][TEXT_WIDTH];

static unsigned int checksum(char* string){
	unsigned int sum = 0;
	while(*string)
		sum += *string++;
	return sum;
}

long long gettime();
static void updateLife(void){
	long long this_tick = gettime();
	
	// Remove old strings
	DEBUG_string* x = head;
	while(x != NULL)
		if(this_tick - x->start_tick > DEBUG_STRING_LIFE){
			// This string has been alive long enough, remove and kill it
			head = x->next;
			if(x == tail) tail = NULL;
			free(x);
			--numLiveStrings;
			x = head;
		} else break;
}

void DEBUG_print(char* string){
	unsigned int cs = checksum( string );
	long long start_tick = gettime();
	
	// Then split the string into lines of text
	int num_lines = TEXT_split(string);
	
	// Next, expand tabs to spaces
	int i;
	for(i=0; i<num_lines; ++i) TEXT_expand( &TEXT_split_lines[i] );
	
	// Check for duplicate strings
	// FIXME: This is the most inefficient part of this system
	//          I'd like to be able to do away with it or improve it
	//          but without it like this, we could lose prints
	//          or have the screen flooded with prints in loops
	DEBUG_string* other = head;
	for(; other != NULL; other = other->next)
		if(cs == other->checksum && num_lines == other->num_lines){
			// We have a potential duplicate, verify it matches
			DEBUG_string* o = other;
			for(i=0; o != NULL && i < num_lines; o = o->next, ++i)
				if( strcmp(&o->text, &TEXT_split_lines[i]) )
					break;
			// If all num_lines matched, update the strings time
			//   and exit without adding the new ones
			if(i == num_lines){
				for(i=0; i<num_lines; ++i, other = other->next)
					other->start_tick = start_tick;
				return;
			}
			other = o;
		} else {
			// If not, don't bother checking the next lines
			for(i=0; other != NULL && i < num_lines; other = other->next, ++i);
		}
	
	// Finally, create a DEBUG_string for each line
	//   and fill in it's start time
	DEBUG_string* next;
	for(i=0; i<num_lines; ++i){
		next = malloc( sizeof(DEBUG_string) );
		strncpy(&next->text, &TEXT_split_lines[i], DEBUG_TEXT_WIDTH);
		next->start_tick = start_tick;
		next->checksum   = cs;
		next->num_lines  = num_lines;
		next->prev       = tail;
		next->next       = NULL;
		// Set the last string to point to this one
		if(tail) tail->next = next;
		tail = next;
		if(head == NULL) head = next;
	}
	
}

void DEBUG_update(void){
	updateLife();
	
	// Fill out text with living strings oldest first
	DEBUG_string* x = tail;
	int i = (numLiveStrings > DEBUG_TEXT_HEIGHT) ? DEBUG_TEXT_HEIGHT : DEBUG_TEXT_HEIGHT-numLiveStrings;
	for(; x != NULL; x = x->prev)
		text[--i] = &x->text[0];
}

char** DEBUG_get_text(void){
	return &text;
}

