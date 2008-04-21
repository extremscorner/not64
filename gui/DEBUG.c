/* DEBUG.c - DEBUG interface
   by Mike Slegeir for Mupen64-GC
 */

#include <string.h>
#include <stdio.h>
#include <sdcard.h>
#include "DEBUG.h"
#include "TEXT.h"
#include "usb.h"

char text[DEBUG_TEXT_HEIGHT][DEBUG_TEXT_WIDTH];
char printToSD;

#ifdef SHOW_DEBUG
char txtbuffer[1024];
long long texttimes[DEBUG_TEXT_HEIGHT];
extern long long gettime();
extern unsigned int diff_sec(long long start,long long end);
static void check_heap_space(void){
	int space = 16 * 1024 * 1024, *ptr=NULL;
	while(space > 0)
		if(ptr = malloc(space)) break;
		else space -= 4096;
	if(ptr) free(ptr);
	sprintf(txtbuffer,"At least %dKB or %dMB space available\n", space/1024, space/1024/1024);
	DEBUG_print(txtbuffer,10);

}
#endif
void DEBUG_update() {
	#ifdef SHOW_DEBUG
	int i;
	long long nowTick = gettime();
	for(i=0; i<DEBUG_TEXT_HEIGHT; i++){
		if(diff_sec(texttimes[i],nowTick)>=DEBUG_STRING_LIFE) 
		{
			memset(text[i],0,DEBUG_TEXT_WIDTH);
		}
	}
	check_heap_space();
	#endif
}


int flushed = 0;
int writtenbefore = 0;
int amountwritten = 0;
char *dump_filename = "dev0:\\N64ROMS\\debug.txt";
sd_file* f = NULL;
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
		else if(pos == DBG_SDGECKOOPEN) {
#ifdef SDPRINT
			if(!f && printToSD)
				f = SDCARD_OpenFile( dump_filename, "wb");
#endif
		}
		else if(pos == DBG_SDGECKOCLOSE) {
#ifdef SDPRINT
			if(f)
				SDCARD_CloseFile(f);
#endif
		}
		else if(pos == DBG_SDGECKOPRINT) {			
#ifdef SDPRINT
			if(!f || (printToSD == 0))
				return;
			SDCARD_WriteFile(f, string, strlen(string));
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

