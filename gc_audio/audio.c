/* audio.c - Low-level Audio plugin for the gamecube
   by Mike Slegeir for Mupen64-GC
   ----------------------------------------------------
   MEMORY USAGE:
     STATIC:
   	Audio Buffer: 2x BUFFER_SIZE (currently 4kb each)
 */

#include "../main/winlnxdefs.h"
#include <stdio.h>
#include <ogc/audio.h>
#include <ogc/cache.h>
#ifdef THREADED_AUDIO
#include <ogc/lwp.h>
#include <ogc/semaphore.h>
#endif

#include "AudioPlugin.h"
#include "Audio_#1.1.h"
#include "../gui/DEBUG.h"

AUDIO_INFO AudioInfo;

#define NUM_BUFFERS 4
#define BUFFER_SIZE 2*1024
static char buffer[NUM_BUFFERS][BUFFER_SIZE] __attribute__((aligned(32)));
static int which_buffer = 0;
static unsigned int buffer_offset = 0;
#define NEXT(x) (x=(x+1)%NUM_BUFFERS)

#ifdef THREADED_AUDIO
static lwp_t audio_thread;
static sem_t buffer_full;
static sem_t buffer_empty;
static sem_t audio_free;
static int   thread_running;
#define AUDIO_STACK_SIZE 1024 // MEM: I could get away with a smaller stack
static char  audio_stack[AUDIO_STACK_SIZE];
#define AUDIO_PRIORITY 100
static int   thread_buffer = 0;
static int   audio_paused = 0;
#else // !THREADED_AUDIO
#define thread_buffer which_buffer
#endif

char audioEnabled;

EXPORT void CALL
AiDacrateChanged( int SystemType )
{
	// Taken from mupen_audio
	int f = 32000; //default to 32khz incase we get a bad systemtype
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
		AUDIO_SetDSPSampleRate(AI_SAMPLERATE_32KHZ);
	else if ( f == 48000 )
		AUDIO_SetDSPSampleRate(AI_SAMPLERATE_48KHZ);
	else {
		 sprintf(txtbuffer,"error initializing frequency: %d", f);
	   	 DEBUG_print(txtbuffer,DBG_AUDIOINFO); 
	}
}

#ifdef THREADED_AUDIO
static void done_playing(void){
	// We're done playing, so we're releasing a buffer and the audio
	LWP_SemPost(buffer_empty);
	LWP_SemPost(audio_free);
}
#endif

static void inline play_buffer(void){
#ifndef THREADED_AUDIO
	// We should wait for the other buffer to finish its DMA transfer first
	while( AUDIO_GetDMABytesLeft() );
	AUDIO_StopDMA();
	
#else // THREADED_AUDIO
	// This thread will keep giving buffers to the audio as they come
	while(thread_running){
	
	// Wait for a buffer to be processed and the audio to be ready
	LWP_SemWait(buffer_full);
	LWP_SemWait(audio_free);
#endif
	
	DCFlushRange (buffer[thread_buffer], BUFFER_SIZE);
	AUDIO_InitDMA(buffer[thread_buffer], BUFFER_SIZE);
	AUDIO_StartDMA();
#ifdef THREADED_AUDIO
	NEXT(thread_buffer);
	}
#endif
}

static void inline copy_to_buffer(char* buffer, char* stream, unsigned int length){
	memcpy(buffer, stream, length);
}

static void inline add_to_buffer(void* stream, unsigned int length){
	// This shouldn't lose any data and works for any size
	unsigned int lengthi;
	unsigned int lengthLeft = length;
	unsigned int stream_offset = 0;
	while(1){
		lengthi = (buffer_offset + lengthLeft < BUFFER_SIZE) ?
		           lengthLeft : (BUFFER_SIZE - buffer_offset);
	
#ifdef THREADED_AUDIO
		// Wait for a buffer we can copy into
		LWP_SemWait(buffer_empty);
#endif		
		copy_to_buffer(buffer[which_buffer] + buffer_offset,
		               stream + stream_offset, lengthi);
		
		if(buffer_offset + lengthLeft < BUFFER_SIZE){
			buffer_offset += lengthi;
#ifdef THREADED_AUDIO
			// This is a little weird, but we didn't fill this buffer.
			//   So it is still considered 'empty', but since we 'decremented'
			//   buffer_empty coming in here, we want to set it back how
			//   it was, so we don't cause a deadlock
			LWP_SemPost(buffer_empty);
#endif
			return;
		}
		
		lengthLeft    -= lengthi;
		stream_offset += lengthi;
		
#ifdef THREADED_AUDIO
		// Let the audio thread know that we've filled a new buffer
		LWP_SemPost(buffer_full);
#else
		play_buffer();
#endif
		
		NEXT(which_buffer);
		buffer_offset = 0;
	}
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
	AUDIO_Init(NULL);
	return TRUE;
}

EXPORT void CALL RomOpen()
{
#ifdef THREADED_AUDIO
	// Create our semaphores and start/resume the audio thread; reset the buffer index
	LWP_SemInit(&buffer_full, 0, NUM_BUFFERS);
	LWP_SemInit(&buffer_empty, NUM_BUFFERS, NUM_BUFFERS);
	LWP_SemInit(&audio_free, 0, 1);
	thread_running = 1;
	LWP_CreateThread(&audio_thread, play_buffer, NULL, audio_stack, AUDIO_STACK_SIZE, AUDIO_PRIORITY);
	AUDIO_RegisterDMACallback(done_playing);
	thread_buffer = which_buffer = 0;
	audio_paused = 1;
#endif
}

EXPORT void CALL
RomClosed( void )
{
#ifdef THREADED_AUDIO
	// Destroy semaphores and suspend the thread so audio can't play
	thread_running = 0;
	LWP_SemDestroy(buffer_full);
	LWP_SemDestroy(buffer_empty);
	LWP_SemDestroy(audio_free);
	LWP_JoinThread(audio_thread, NULL);
	audio_paused = 0;
#endif
	AUDIO_StopDMA(); // So we don't have a buzzing sound when we exit the game
}

EXPORT void CALL
ProcessAlist( void )
{
}

void pauseAudio(void){
#ifdef THREADED_AUDIO
	// We just grab the audio_free 'lock' and don't let go
	//   when we have this lock, audio_thread must be waiting
	LWP_SemWait(audio_free);
	audio_paused = 1;
#endif
	AUDIO_StopDMA();
}

void resumeAudio(void){
#ifdef THREADED_AUDIO
	if(audio_paused){
		// When we're want the audio to resume, release the 'lock'
		LWP_SemPost(audio_free);
		audio_paused = 0;
	}
#endif
}

