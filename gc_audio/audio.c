/*
 * Copyright (c) 2013 Extrems <metaradil@gmail.com>
 *
 * This file is part of Not64.
 *
 * Not64 is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Not64 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with Not64; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "../main/winlnxdefs.h"
#include <gccore.h>
#include <string.h>
#include <aesndlib.h>

#include "AudioPlugin.h"
#include "Audio_#1.1.h"
#include "../main/timers.h"
#include "../main/wii64config.h"

AUDIO_INFO AudioInfo;
extern float VILimit;
#define DEFAULT_FREQUENCY 33600
#define BUFFER_SIZE (DSP_STREAMBUFFER_SIZE * 64)
static char buffer[BUFFER_SIZE];
static const char *end_ptr = buffer + BUFFER_SIZE;
static char *write_ptr, *read_ptr;
static int buffered;
static unsigned int freq;
static AESNDPB *voice;

char audioEnabled;
char scalePitch;

static void aesnd_callback(AESNDPB *pb, uint32_t state)
{
	if (state == VOICE_STATE_STREAM) {
		if (buffered >= DSP_STREAMBUFFER_SIZE) {
			AESND_SetVoiceBuffer(pb, read_ptr, DSP_STREAMBUFFER_SIZE);
			read_ptr += DSP_STREAMBUFFER_SIZE;
			if (read_ptr >= end_ptr)
				read_ptr = buffer;
			buffered -= DSP_STREAMBUFFER_SIZE;
		}
	}
}

static void reset_buffer(void)
{
	write_ptr = buffer;
	read_ptr = buffer;
	buffered = 0;
}

EXPORT void CALL AiDacrateChanged(int SystemType)
{
	switch (SystemType) {
		case SYSTEM_NTSC:
			freq = 48681812 / (*AudioInfo.AI_DACRATE_REG + 1);
			break;
		case SYSTEM_PAL:
			freq = 49656530 / (*AudioInfo.AI_DACRATE_REG + 1);
			break;
		case SYSTEM_MPAL:
			freq = 48628316 / (*AudioInfo.AI_DACRATE_REG + 1);
			break;
		default:
			freq = DEFAULT_FREQUENCY;
	}
	
	AESND_SetVoiceFrequency(voice, freq);
}

EXPORT void CALL AiLenChanged(void)
{
	if (audioEnabled) {
		uint32_t level = IRQ_Disable();
		
		char *stream = AudioInfo.RDRAM + (*AudioInfo.AI_DRAM_ADDR_REG & 0xFFFFFF);
		int length = *AudioInfo.AI_LEN_REG;
		
		if (buffered + length < BUFFER_SIZE) {
			do {
				int size = MIN(end_ptr - write_ptr, length);
				memcpy(write_ptr, stream, size);
				stream += size;
				length -= size;
				
				write_ptr += size;
				if (write_ptr >= end_ptr)
					write_ptr = buffer;
				buffered += size;
			} while (length > 0);
		}
		
		if (scalePitch || Timers.vis > VILimit)
			AESND_SetVoiceFrequency(voice, freq * (Timers.vis / VILimit));
		
		IRQ_Restore(level);
	}
}

EXPORT void CALL CloseDLL(void)
{
	AESND_FreeVoice(voice);
	voice = NULL;
}

EXPORT BOOL CALL InitiateAudio(AUDIO_INFO Audio_Info)
{
	AudioInfo = Audio_Info;
	AESND_Init();
	
	voice = AESND_AllocateVoice(aesnd_callback);
	if (voice == NULL) return FALSE;
	
	AESND_SetVoiceFormat(voice, VOICE_STEREO16);
	AESND_SetVoiceStream(voice, true);
	
	return TRUE;
}

EXPORT void CALL RomOpen(void)
{
	reset_buffer();
	AESND_SetVoiceStop(voice, false);
}

EXPORT void CALL RomClosed(void)
{
	AESND_SetVoiceStop(voice, true);
}

EXPORT void CALL ProcessAlist(void)
{
}

void pauseAudio(void)
{
	AESND_SetVoiceLoop(voice, false);
	AESND_SetVoiceMute(voice, true);
}

void resumeAudio(void)
{
	AESND_SetVoiceFrequency(voice, freq);
	AESND_SetVoiceLoop(voice, !scalePitch);
	AESND_SetVoiceMute(voice, !audioEnabled);
}
