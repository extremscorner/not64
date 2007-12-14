/* DEBUG.c - DEBUG interface
   by Mike Slegeir for Mupen64-GC
 */

#include <string.h>
#include "DEBUG.h"
#include "TEXT.h"

char text[DEBUG_TEXT_HEIGHT][DEBUG_TEXT_WIDTH];

void DEBUG_print(char* string,int pos){
	#ifdef SHOW_DEBUG
		memset(text[pos],0,DEBUG_TEXT_WIDTH);
		strncpy(text[pos], string, DEBUG_TEXT_WIDTH);
		memset(text[DEBUG_TEXT_WIDTH-1],0,1);
	#endif
	
}

