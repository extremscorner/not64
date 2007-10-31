/* audio.c - Low-level Audio plugin for the gamecube
   by Mike Slegeir for Mupen64-GC
   ----------------------------------------------------
   FIXME: This probably needs to be threaded so that we
            can start playing the next buffer once the
            playing buffer is finished.  However, this
            is nontrivial since we need to make sure
            the buffer has a length divisble by 32
   ----------------------------------------------------
   MEMORY USAGE:
     STATIC:
   	Audio Buffer: 2x BUFFER_SIZE (currently 4kb each)
 */

#include "../main/winlnxdefs.h"
#include <stdio.h>
#include <ogc/audio.h>
#include <ogc/cache.h>

#include "AudioPlugin.h"
#include "Audio_#1.1.h"

AUDIO_INFO AudioInfo;

#define BUFFER_SIZE 4*1024
static char buffer[2][BUFFER_SIZE] __attribute__((aligned(32)));
static char which_buffer = 0;
static unsigned int buffer_offset = 0;

char audioEnabled;

EXPORT void CALL
AiDacrateChanged( int SystemType )
{
	// Taken from mupen_audio
	int f;
	switch (SystemType){
	      case SYSTEM_NTSC:
		f = 48681812 / (*AudioInfo.AI_DACRATE_REG + 1);
		break;
	      case SYSTEM_PAL:
		f = 49656530 / (*AudioInfo.AI_DACRATE_REG + 1);
		break;
	      case SYSTEM_MPAL:
		f = 48628316 / (*AudioInfo.AI_DACRATE_REG + 1);
		break;
	}
	
	if      ( f == 32000 )
		AUDIO_SetStreamSampleRate(AI_SAMPLERATE_32KHZ);
	else if ( f == 48000 )
		AUDIO_SetStreamSampleRate(AI_SAMPLERATE_48KHZ);
	else 
		printf("error initializing frequency: %d\n", f);
		
	// FIXME: Trying to force 48khz
	AUDIO_SetStreamSampleRate(AI_SAMPLERATE_48KHZ);
}

// THREADME: This should probably be a DMA finished call back
static void inline play_buffer(void){
	// THREADME: This line should be deleted
	// We should wait for the other buffer to finish its DMA transfer first
	while( AUDIO_GetDMABytesLeft() );
	
	AUDIO_StopDMA();
	
	// THREADME: sem_wait( buffer_full )
	
	DCFlushRange (buffer[which_buffer], BUFFER_SIZE);
	AUDIO_InitDMA(buffer[which_buffer], BUFFER_SIZE);
	AUDIO_StartDMA();
}

static void inline copy_to_buffer(char* buffer, char* stream, unsigned int length){
	// FIXME: We need to fix the endian-ness issue from the source: RSP
	// Here we're byte-swapping 16-bit samples
	int i;
	for(i=0; i<length; ++i)
		buffer[i] = stream[i];
}

static void inline add_to_buffer(char* stream, unsigned int length){
#if 0
	if(buffer_offset + length > BUFFER_SIZE){
		// Only write some into this buffer
		unsigned int length1 = BUFFER_SIZE - buffer_offset;
		unsigned int length2 = length - length1;
		// FIXME: This potentially chops off some data
		//          we could do a while loop?
		length2 = length2 > BUFFER_SIZE ? BUFFER_SIZE : length2;
		
		memcpy(buffer[which_buffer] + buffer_offset, stream, length1);
		
		play_buffer();
		
		// Now write into the other buffer
		which_buffer ^= 1;
		memcpy(buffer[which_buffer], stream + length1, length2);
		buffer_offset = length2;
	} else {
		// All the data fits in this buffer
		memcpy(buffer[which_buffer] + buffer_offset, stream, length);
		buffer_offset += length;
	}
#else
		// This shouldn't lose any data and works for any size
		unsigned int lengthi;
		unsigned int lengthLeft = length;
		unsigned int stream_offset = 0;
		while(1){
			lengthi = (buffer_offset + lengthLeft < BUFFER_SIZE) ?
			           lengthLeft : (BUFFER_SIZE - buffer_offset);
		
			//memcpy(buffer[which_buffer] + buffer_offset, stream + stream_offset, lengthi);
			copy_to_buffer(buffer[which_buffer] + buffer_offset,
			               stream + stream_offset, lengthi);
			
			if(buffer_offset + lengthLeft < BUFFER_SIZE){
				buffer_offset += lengthi;
				return;
			}
			
			lengthLeft    -= lengthi;
			stream_offset += lengthi;
			
			/* THREADME: Replace play_buffer with:
				sem_post( buffer_full )
			 */
			play_buffer();
			
			which_buffer ^= 1;
			buffer_offset = 0;
		}
#endif
}

EXPORT void CALL
AiLenChanged( void )
{
	// FIXME: We may need near full speed before this is going to work
	if(!audioEnabled) return;
	
	short* stream = (short*)(AudioInfo.RDRAM + 
		         (*AudioInfo.AI_DRAM_ADDR_REG & 0xFFFFFF));
	unsigned int length = *AudioInfo.AI_LEN_REG;
	
	add_to_buffer(stream, length);
}

EXPORT DWORD CALL
AiReadLength( void )
{
	// I don't know if this is the data they're trying to get
	return AUDIO_GetDMABytesLeft();
}

EXPORT void CALL
AiUpdate( BOOL Wait )
{
}

EXPORT void CALL
CloseDLL( void )
{
}

EXPORT void CALL
DllAbout( HWND hParent )
{
	printf ("Gamecube audio plugin\n\tby Mike Slegeir" );
}

EXPORT void CALL
DllConfig ( HWND hParent )
{
}

EXPORT void CALL
DllTest ( HWND hParent )
{
}

EXPORT void CALL
GetDllInfo( PLUGIN_INFO * PluginInfo )
{
	PluginInfo->Version = 0x0101;
	PluginInfo->Type    = PLUGIN_TYPE_AUDIO;
	sprintf(PluginInfo->Name,"Gamecube audio plugin\n\tby Mike Slegeir");
	PluginInfo->NormalMemory  = TRUE;
	PluginInfo->MemoryBswaped = TRUE;
}

EXPORT BOOL CALL
InitiateAudio( AUDIO_INFO Audio_Info )
{
	AudioInfo = Audio_Info;
	AUDIO_Init(0);
	return TRUE;
}

EXPORT void CALL RomOpen()
{
}

EXPORT void CALL
RomClosed( void )
{
	AUDIO_StopDMA(); // So we don't have a buzzing sound when we exit the game
}

EXPORT void CALL
ProcessAlist( void )
{
}
