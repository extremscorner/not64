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
#ifdef THREADED_AUDIO
#include <ogc/lwp.h>
#include <ogc/semaphore.h>
#endif

#include "AudioPlugin.h"
#include "Audio_#1.1.h"
#include "../gui/DEBUG.h"

AUDIO_INFO AudioInfo;

#define NUM_BUFFERS 2
#define BUFFER_SIZE 4*1024
static char buffer[NUM_BUFFERS][BUFFER_SIZE] __attribute__((aligned(32)));
static int which_buffer = 0;
static unsigned int buffer_offset = 0;
#define NEXT(x) (x=(x+1)%NUM_BUFFERS)

#ifdef THREADED_AUDIO
static lwp_t audio_thread;
static sem_t buffer_full;
static sem_t buffer_empty;
static int   thread_inited;
#define AUDIO_STACK_SIZE 256
static char  audio_stack[256];
#define AUDIO_PRIORITY 15
static int   thread_buffer = 0;
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
		AUDIO_SetStreamSampleRate(AI_SAMPLERATE_32KHZ);
	else if ( f == 48000 )
		AUDIO_SetStreamSampleRate(AI_SAMPLERATE_48KHZ);
	else {
		 sprintf(txtbuffer,"error initializing frequency: %d", f);
	   	 DEBUG_print(txtbuffer,DBG_AUDIOINFO); 
	}
		
	// FIXME: Trying to force 48khz
	AUDIO_SetStreamSampleRate(AI_SAMPLERATE_48KHZ);
}

#ifdef THREADED_AUDIO
static void done_playing(void){
	// THREADME: This should probably be a DMA finished call back
	AUDIO_StopDMA();
	LWP_SemPost(buffer_empty);
}
#endif

static void inline play_buffer(void){
#ifndef THREADED_AUDIO
	// We should wait for the other buffer to finish its DMA transfer first
	while( AUDIO_GetDMABytesLeft() );
	AUDIO_StopDMA();
	
#else // THREADED_AUDIO
	while(1){
	
	// THREADME: sem_wait( buffer_full )
	LWP_SemWait(buffer_full);
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
	// FIXME: We need to fix the endian-ness issue from the source: RSP
	// Here we're byte-swapping 16-bit samples
	int i;
	for(i=0; i<length; ++i)
		buffer[i] = stream[i];
}

static void inline add_to_buffer(void* stream, unsigned int length){
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
#ifdef THREADED_AUDIO
			// This is a little weird, but we didn't fill this buffer.
			//   So it is still considered empty, but since we 'decremented'
			//   buffer_empty coming in here, we want to set it back how
			//   it was, so we don't cause a deadlock
			LWP_SemPost(buffer_empty);
#endif
			return;
		}
		
		lengthLeft    -= lengthi;
		stream_offset += lengthi;
		
#ifdef THREADED_AUDIO
		/* THREADME: Replace play_buffer with:
			sem_post( buffer_full )
		 */
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
	
#if THREADED_AUDIO
	LWP_SemWait(buffer_empty);
#endif
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
#ifdef THREADED_AUDIO
	// THREADME: CreateThread play_buffer, CreateSemaphore buffer_full(1)
	if(!thread_inited){
		LWP_SemInit(&buffer_full, 0, NUM_BUFFERS);
		LWP_SemInit(&buffer_empty, NUM_BUFFERS, NUM_BUFFERS);
		LWP_CreateThread(&audio_thread, play_buffer, NULL, audio_stack, AUDIO_STACK_SIZE, AUDIO_PRIORITY);
		AUDIO_RegisterDMACallback(done_playing);
		thread_inited = 1;
	} else LWP_ResumeThread(audio_thread);
#endif
}

EXPORT void CALL
RomClosed( void )
{
#ifdef THREADED_AUDIO
	// THREADME: KillThread play_buffer, DestroySemaphore buffer_full
	LWP_SuspendThread(audio_thread);
#endif
	AUDIO_StopDMA(); // So we don't have a buzzing sound when we exit the game
}

EXPORT void CALL
ProcessAlist( void )
{
}

