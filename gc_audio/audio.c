/* audio.c - Low-level Audio plugin for the gamecube
   by Mike Slegeir for Mupen64-GC
 */

#include "../main/winlnxdefs.h"
#include <stdio.h>
#include <ogc/audio.h>
#include <ogc/cache.h>

#include "AudioPlugin.h"
#include "Audio_#1.1.h"

AUDIO_INFO AudioInfo;

#define BUFFER_SIZE 2048
static char buffer[2][BUFFER_SIZE] __attribute__((aligned(32)));
static char which_buffer = 0;
static unsigned int buffer_offset = 0;

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
		printf("error initializing frequency:%x\n", f);
		
	// FIXME: Trying to force 48khz
	AUDIO_SetStreamSampleRate(AI_SAMPLERATE_32KHZ);
}

static void inline play_buffer(void){
	AUDIO_StopDMA();
	
	DCFlushRange (buffer[which_buffer], BUFFER_SIZE);
	AUDIO_InitDMA(buffer[which_buffer], BUFFER_SIZE);
	AUDIO_StartDMA();
	
	which_buffer ^= 1;
	buffer_offset = 0;
}

static void inline add_to_buffer(char* stream, unsigned int length){
	if(buffer_offset + length > BUFFER_SIZE){
		// Only write some into this buffer
		unsigned int length1 = BUFFER_SIZE - buffer_offset;
		unsigned int length2 = length - length1;
		// FIXME: This chops off some data
		length2 = length2 > BUFFER_SIZE ? BUFFER_SIZE : length2;
		
		memcpy(buffer[which_buffer] + buffer_offset, stream, length1);
		
		play_buffer();
		
		memcpy(buffer[which_buffer], stream + length1, length2);
		buffer_offset = length2;
	} else {
		memcpy(buffer[which_buffer] + buffer_offset, stream, length);
		buffer_offset += length;
	}
}

EXPORT void CALL
AiLenChanged( void )
{
	// FIXME: We need near full speed before this is going to work
	return;
	
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
}

EXPORT void CALL
ProcessAlist( void )
{
}
