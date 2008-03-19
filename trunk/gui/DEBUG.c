/* DEBUG.c - DEBUG interface
   by Mike Slegeir for Mupen64-GC
 */

#include <string.h>
#include "DEBUG.h"
#include "TEXT.h"
#include "usb.h"

#define ALIVETIME 5 //amount of time in seconds to display a debug string

char text[DEBUG_TEXT_HEIGHT][DEBUG_TEXT_WIDTH];

#ifdef SHOW_DEBUG
long long texttimes[DEBUG_TEXT_HEIGHT];
extern long long gettime();
extern unsigned int diff_sec(long long start,long long end);
#endif
void DEBUG_update() {
	#ifdef SHOW_DEBUG
	int i;
	long long nowTick = gettime();
	for(i=0; i<DEBUG_TEXT_HEIGHT; i++){
		if(diff_sec(texttimes[i],nowTick)>=ALIVETIME) 
		{
			memset(text[i],0,DEBUG_TEXT_WIDTH);
		}
	}
	#endif
}


int flushed = 0;
void DEBUG_print(char* string,int pos){

	#ifdef SHOW_DEBUG
		if(pos == DBG_USBGECKO) {
			#ifdef PRINTGECKO
			if(!flushed){
				usb_flush();
				flushed = 1;
			}
			int size = strlen(string);
			usb_sendbuffer (&size,4);
			usb_sendbuffer (string,size);
			#endif
		}
		else {
			memset(text[pos],0,DEBUG_TEXT_WIDTH);
			strncpy(text[pos], string, DEBUG_TEXT_WIDTH);
			memset(text[DEBUG_TEXT_WIDTH-1],0,1);
			texttimes[pos] = gettime();
		}
	#endif
	
}

