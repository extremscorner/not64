#include "../config.h"
#ifdef VCR_SUPPORT

#include "vcr.h"
#include "vcr_compress.h"
#include "vcr_resample.h"

#include "plugin.h"
#include "rom.h"
#include "savestates.h"
#include "../memory/memory.h"

#include <errno.h>
#include <limits.h>
#include <malloc.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <zlib.h>

enum ETask
{
	Idle = 0,
	StartRecording,
	Recording,
	StartPlayback,
	Playback
};

static const char *m_taskName[] =
{
	"Idle",
	"StartRecording",
	"Recording",
	"StartPlayback",
	"Playback"
};

static char   m_filename[PATH_MAX];
static gzFile m_file = 0;
static int    m_task = Idle;
static long   m_count;					// number of recorded samples
static long   m_pos;					// playback position
static int    m_capture = 0;			// capture movie
static int    m_audioFreq = 33000;		//0x30018;
static int    m_audioBitrate = 16;		// 16 bits
static float  m_videoFrame = 0;
static float  m_audioFrame = 0;


static int fpsByCountrycode()
{
	switch(ROM_HEADER->Country_code&0xFF)
	{
	case 0x44:
	case 0x46:
	case 0x49:
	case 0x50:
	case 0x53:
	case 0x55:
	case 0x58:
	case 0x59:
		return 25;
		break;

	case 0x37:
	case 0x41:
	case 0x45:
	case 0x4a:
		return 30;
		break;
	}

	printf( "[VCR]: Warning - unknown country code, using 30 FPS for video.\n" );
	return 30;
}


void
VCR_getKeys( int Control, BUTTONS *Keys )
{
	if (m_task != Playback) // && m_task != StartPlayback)
		getKeys( Control, Keys );

	if (m_task == Idle)
		return;

	if (m_task == StartRecording)
	{
		// wait until state is saved, then record
		if ((savestates_job & SAVESTATE) == 0)
		{
			printf( "[VCR]: Starting recording...\n" );
			m_task = Recording;
			m_count = 0;
		}
		else
			return;
	}

	if (m_task == Recording)
	{
		long cont = Control;
		gzwrite( m_file, &cont, sizeof (long) );
		gzwrite( m_file, Keys, sizeof (BUTTONS) );
		m_count++;
		return;
	}

	if (m_task == StartPlayback)
	{
		// wait until state is loaded, then playback
		if ((savestates_job & LOADSTATE) == 0)
		{
			printf( "[VCR]: Starting playback...\n" );
			m_task = Playback;
			m_pos = 0;
		}
		else
			return;
	}

	if (m_task == Playback)
	{
		long cont;
		gzread( m_file, &cont, sizeof (long) );
		if (cont == -1)	// end
		{
			if (m_capture != 0)
				VCR_stopCapture();
			else
				VCR_stopPlayback();
			return;
		}
		if (cont != Control)
		{
			printf( "[VCR]: Warning - controller num from file doesn't match requested number\n" );
			// ...
		}
		gzread( m_file, Keys, sizeof (BUTTONS) );
		m_pos++;
	}
}


void
VCR_updateScreen()
{
	void *image;
	long width, height;
	static int frame = 0;
	float desync;

	if (m_capture == 0 || readScreen == 0 || m_task != Playback)
	{
		updateScreen();
		return;
	}

	frame ^= 1;
	if (!frame)
		return;

	updateScreen();
	readScreen( &image, &width, &height );
	if (image == 0)
	{
		fprintf( stderr, "[VCR]: Couldn't read screen (out of memory?)\n" );
		return;
	}

	VCRComp_addVideoFrame( image );
	free( image );

	m_videoFrame += 1.0;
	desync = m_videoFrame - m_audioFrame;
	if (desync >= 5.0)
	{
		int len;
		char *buf;
		printf( "[VCR]: A/V desyncronization detected: %+f frames\n", desync );
		len = (44100/(float)fpsByCountrycode()) * desync;
		len <<= 2;
		buf = malloc( len );
		memset( buf, 0, len );
		VCRComp_addAudioData( (char *)buf, len );
		free( buf );
		m_audioFrame += ((len/4)/44100.0)*fpsByCountrycode();
	}
	else if (desync <= -5.0)
	{
		printf( "[VCR]: A/V desyncronization detected: %+f frames\n", desync );
	}
}


void
VCR_aiDacrateChanged( int SystemType )
{
	aiDacrateChanged( SystemType );

	m_audioBitrate = ai_register.ai_bitrate+1;
	switch (SystemType)
	{
	case SYSTEM_NTSC:
		m_audioFreq = 48681812 / (ai_register.ai_dacrate + 1);
		break;
	case SYSTEM_PAL:
		m_audioFreq = 49656530 / (ai_register.ai_dacrate + 1);
		break;
	case SYSTEM_MPAL:
		m_audioFreq = 48628316 / (ai_register.ai_dacrate + 1);
		break;
	}
}


void VCR_aiLenChanged()
{
	short *p = (short *)((char*)rdram + (ai_register.ai_dram_addr & 0xFFFFFF));
	unsigned int len = ai_register.ai_len;
	int ret;
	short *buf;

	aiLenChanged();
	if (m_capture == 0)
		return;

	// hack - mupen64 updates bitrate after calling aiDacrateChanged
	m_audioBitrate = ai_register.ai_bitrate+1;

	ret = VCR_resample( &buf, 44100, p, m_audioFreq, m_audioBitrate, len );
	if (ret > 0)
	{
		VCRComp_addAudioData( (char *)buf, ret );
		free( buf );
		m_audioFrame += ((ret/4)/44100.0)*fpsByCountrycode();
	}
}





int
VCR_startRecord( const char *filename )
{
	char buf[PATH_MAX];

	if (m_task != Idle)
	{
		fprintf( stderr, "[VCR]: Cannot start recording, current task is \"%s\"\n", m_taskName[m_task] );
		return -1;
	}

	strncpy( m_filename, filename, PATH_MAX );

	// open record file
	strcpy( buf, m_filename );
	strncat( buf, ".rec", PATH_MAX );
	m_file = gzopen( buf, "wb" );
	if (m_file == 0)
	{
		fprintf( stderr, "[VCR]: Cannot start recording, could not open file '%s': %s\n", filename, strerror( errno ) );
		return -1;
	}

	// save state
	printf( "[VCR]: Saving state...\n" );
	strcpy( buf, m_filename );
	strncat( buf, ".st", PATH_MAX );
	savestates_select_filename( buf );
	savestates_job |= SAVESTATE;
	m_task = StartRecording;

	return 0;
}


int
VCR_stopRecord()
{
	if (m_task == StartRecording)
	{
		char buf[PATH_MAX];

		m_task = Idle;
		if (m_file)
		{
			gzclose( m_file );
			m_file = 0;
		}
		printf( "[VCR]: Removing files (nothing recorded)\n" );

		strcpy( buf, m_filename );
		strncat( m_filename, ".st", PATH_MAX );
		if (unlink( buf ) < 0)
			fprintf( stderr, "[VCR]: Couldn't remove save state: %s\n", strerror( errno ) );

		strcpy( buf, m_filename );
		strncat( m_filename, ".rec", PATH_MAX );
		if (unlink( buf ) < 0)
			fprintf( stderr, "[VCR]: Couldn't remove recorded file: %s\n", strerror( errno ) );

		return 0;
	}

	if (m_task == Recording)
	{
		long end = -1;

		m_task = Idle;
		gzwrite( m_file, &end, sizeof (long) );
		gzwrite( m_file, &m_count, sizeof (long) );
		gzclose( m_file );

		printf( "[VCR]: Record stopped. Recorded %ld input samples\n", m_count );
		return 0;
	}

	return -1;
}


int
VCR_startPlayback( const char *filename )
{
	char buf[PATH_MAX];

	if (m_task != Idle)
	{
		fprintf( stderr, "[VCR]: Cannot start playback, current task is \"%s\"\n", m_taskName[m_task] );
		return -1;
	}

	strncpy( m_filename, filename, PATH_MAX );
	char *p = strrchr( m_filename, '.' );
	if (p)
	{
		if (!strcasecmp( p, ".rec" ) || !strcasecmp( p, ".st" ))
			*p = '\0';
	}

	// open record file
	strcpy( buf, m_filename );
	strncat( buf, ".rec", PATH_MAX );
	m_file = gzopen( buf, "rb" );
	if (m_file == 0)
	{
		fprintf( stderr, "[VCR]: Cannot start playback, could not open .rec file '%s': %s\n", filename, strerror( errno ) );
		return -1;
	}
	m_pos = 0;

	// load state
	printf( "[VCR]: Loading state...\n" );
	strcpy( buf, m_filename );
	strncat( buf, ".st", PATH_MAX );
	savestates_select_filename( buf );
	savestates_job |= LOADSTATE;
	m_task = StartPlayback;

	return 0;
}


int
VCR_stopPlayback()
{
	if (m_task == StartPlayback)
	{
		m_task = Idle;
		if (m_file)
		{
			gzclose( m_file );
			m_file = 0;
		}
		return 0;
	}

	if (m_task == Playback)
	{
		m_task = Idle;
		if (m_file)
		{
			gzclose( m_file );
			m_file = 0;
		}
		printf( "[VCR]: Playback stopped (%ld samples played)\n", m_pos );
		return 0;
	}

	return -1;
}

#ifdef __WIN32__
void init_readScreen();
#endif

int
VCR_startCapture( const char *recFilename, const char *aviFilename )
{
#ifdef __WIN32__
    init_readScreen();
#endif
	if (readScreen == 0)
	{
		printf( "[VCR]: You need a video plugin which supports ReadScreen() to record movies!\n" );
		return -1;
	}

	m_videoFrame = 0.0;
	m_audioFrame = 0.0;
	void *dest;
	long width, height;
	readScreen( &dest, &width, &height );
	if (dest)
		free( dest );
	VCRComp_startFile( aviFilename, width, height, fpsByCountrycode() );
	m_capture = 1;
	if (VCR_startPlayback( recFilename ) < 0)
	{
		fprintf( stderr, "[VCR]: Cannot start capture, could not start playback!\n" );
		return -1;
	}
	printf( "[VCR]: Starting capture...\n" );

	return 0;
}


int
VCR_stopCapture()
{
	m_capture = 0;
#ifndef __WIN32__
	usleep( 100000 );
#endif
	VCR_stopPlayback();
	VCRComp_finishFile();
	printf( "[VCR]: Capture finished.\n" );
	return 0;
}




void
VCR_coreStopped()
{
	switch (m_task)
	{
	case StartRecording:
	case Recording:
		VCR_stopRecord();
		break;
	case StartPlayback:
	case Playback:
		if (m_capture != 0)
			VCR_stopCapture();
		else
			VCR_stopPlayback();
		break;
	}
}


#endif // VCR_SUPPORT
