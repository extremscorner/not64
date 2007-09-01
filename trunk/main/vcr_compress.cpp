#include "../config.h"

#ifdef VCR_SUPPORT

#include "vcr_compress.h"

#include <videoencoder.h>
#include <avifile.h>
#include <aviplay.h>
#include <version.h>
#include <avm_creators.h>
#include <avm_fourcc.h>
#include <avm_except.h>
#include <iostream>
#define __MODULE__		"mupen64-vcr_compress"

#ifndef SAFE_DELETE
# define SAFE_DELETE(p) if (p) delete p;
#endif

using namespace std;

static BitmapInfo			*m_videoFormat = 0;
static WAVEFORMATEX			*m_audioFormat = 0;
static IAviWriteFile		*m_file = 0;
static IAviVideoWriteStream	*m_videoStream = 0;
static IAviAudioWriteStream	*m_audioStream = 0;

static avm::vector<const CodecInfo*> m_videoCodecs;
static avm::vector<const CodecInfo*> m_audioCodecs;
static int m_selectedVideoCodec = 0;
static int m_selectedAudioCodec = 0;


void
VCRComp_init()
{
	CodecInfo::Get( m_videoCodecs, CodecInfo::Video, CodecInfo::Encode );
	CodecInfo::Get( m_audioCodecs, CodecInfo::Audio, CodecInfo::Encode );
}


void
VCRComp_startFile( const char *filename, long width, long height, int fps )
{
	if (GetAvifileVersion() != AVIFILE_VERSION)
	{
		cout << "[VCR]: This binary was compiled for Avifile ver. " << AVIFILE_VERSION
		     << ", but the library is ver. " << GetAvifileVersion() << ". Aborting." << endl;
		return;
	}

	SAFE_DELETE(m_videoFormat);
	SAFE_DELETE(m_audioFormat);
	SAFE_DELETE(m_file);
	m_videoFormat = 0;
	m_audioFormat = 0;
	m_file = 0;
	m_videoStream = 0;
	m_audioStream = 0;

	try
	{
		m_videoFormat = new BitmapInfo( width, -height, 24 );	// RGB, flipped

		m_audioFormat = new WAVEFORMATEX;
		memset( m_audioFormat, 0, sizeof (WAVEFORMATEX) );
		m_audioFormat->wFormatTag = WAVE_FORMAT_PCM;
		m_audioFormat->nChannels = 2;
		m_audioFormat->nSamplesPerSec = 44100;
		m_audioFormat->nBlockAlign = 4;
		m_audioFormat->nAvgBytesPerSec = m_audioFormat->nSamplesPerSec * m_audioFormat->nBlockAlign;
		m_audioFormat->wBitsPerSample = 16;

		m_file = CreateIAviWriteFile( filename );
		m_videoStream = m_file->AddVideoStream( *m_videoCodecs[m_selectedVideoCodec], m_videoFormat, 1000000/fps );
		m_videoStream->SetQuality( 8000 );	// 0...10000
		m_videoStream->Start();
		m_audioStream = m_file->AddAudioStream( *m_audioCodecs[m_selectedAudioCodec], m_audioFormat, 24000 );	// 192 kbit/s
		m_audioStream->Start();
	}
	catch (FatalError &error)
	{
		error.Print();
		SAFE_DELETE(m_videoFormat);
		SAFE_DELETE(m_audioFormat);
		SAFE_DELETE(m_file);
		m_videoFormat = 0;
		m_audioFormat = 0;
		m_file = 0;
		m_videoStream = 0;
		m_audioStream = 0;
	}
	catch (...)
	{
		cout << "[VCR]: ERROR: Caught unknown exception!" << endl;
	}
}


void
VCRComp_finishFile()
{
	if (m_file == 0)
	{
		cout << "VCRComp_finishFile: not recording" << endl;
		return;
	}

	if (m_videoStream != 0)
		m_videoStream->Stop();
	if (m_audioStream != 0)
		m_audioStream->Stop();
	delete m_file;
	m_file = 0;
	SAFE_DELETE(m_videoFormat);
	SAFE_DELETE(m_audioFormat);
	m_videoFormat = 0;
	m_audioFormat = 0;
	m_videoStream = 0;
	m_audioStream = 0;

	cout << "[VCR]: Finished capturing to file." << endl;
}


void VCRComp_addVideoFrame( const unsigned char *data )
{
	static int count = 0;
	if (m_videoFormat == 0 || m_file == 0 || m_videoStream == 0)
	{
		cout << "VCRComp_addVideoFrame: not recording" << endl;
		SAFE_DELETE(m_videoFormat);
		SAFE_DELETE(m_audioFormat);
		SAFE_DELETE(m_file);
		m_videoFormat = 0;
		m_audioFormat = 0;
		m_file = 0;
		m_videoStream = 0;
		m_audioStream = 0;
		return;
	}

	try
	{
		CImage im( (BitmapInfo *)m_videoFormat, data, false );
		m_videoStream->AddFrame( &im );
		count++;
		if ((count % 50) == 0)
			cout << "[VCR]: " << count << " frames captured..." << endl;
	}
	catch (FatalError &error)
	{
		error.Print();
		SAFE_DELETE(m_videoFormat);
		SAFE_DELETE(m_audioFormat);
		SAFE_DELETE(m_file);
		m_videoFormat = 0;
		m_audioFormat = 0;
		m_file = 0;
		m_videoStream = 0;
		m_audioStream = 0;
	}
	catch (...)
	{
		cout << "[VCR]: ERROR: Caught unknown exception!" << endl;
	}
}


void
VCRComp_addAudioData( const unsigned char *data, int len )
{
	try
	{
		m_audioStream->AddData( (void *)data, len );
	}
	catch (FatalError &error)
	{
		error.Print();
		SAFE_DELETE(m_videoFormat);
		SAFE_DELETE(m_audioFormat);
		SAFE_DELETE(m_file);
		m_videoFormat = 0;
		m_audioFormat = 0;
		m_file = 0;
		m_videoStream = 0;
		m_audioStream = 0;
	}
	catch (...)
	{
		cout << "[VCR]: ERROR: Caught unknown exception!" << endl;
	}
}



// codec stuff
int
VCRComp_numVideoCodecs()
{
	return m_videoCodecs.size();
}

const char *
VCRComp_videoCodecName( int index )
{
	if (index < 0 || index >= (int)m_videoCodecs.size())
		return "<cindex out of range>";

	return m_videoCodecs[index]->GetName();
}

int
VCRComp_numVideoCodecAttribs( int index )
{
	if (index < 0 || index >= (int)m_videoCodecs.size())
		return 0;

	return m_videoCodecs[index]->encoder_info.size();
}

const char *
VCRComp_videoCodecAttribName( int cindex, int aindex )
{
	if (cindex < 0 || cindex >= (int)m_videoCodecs.size())
		return "<cindex out of range>";
	if (aindex < 0 || aindex >= (int)m_videoCodecs[cindex]->encoder_info.size())
		return "<aindex out of range>";

	return m_videoCodecs[cindex]->encoder_info[aindex].GetName();
}

int
VCRComp_videoCodecAttribKind( int cindex, int aindex )
{
	if (cindex < 0 || cindex >= (int)m_videoCodecs.size())
		return -1;
	if (aindex < 0 || aindex >= (int)m_videoCodecs[cindex]->encoder_info.size())
		return -1;

	switch (m_videoCodecs[cindex]->encoder_info[aindex].GetKind())
	{
	case AttributeInfo::Integer:
		return ATTRIB_INTEGER;
		break;

	case AttributeInfo::String:
		return ATTRIB_STRING;
		break;

	case AttributeInfo::Select:
		return ATTRIB_SELECT;
		break;

	case AttributeInfo::Float:
		return ATTRIB_FLOAT;
		break;
	}

	return -1;
}

const char *
VCRComp_videoCodecAttribValue( int cindex, int aindex )
{
	static char buf[256];
	char *cVal;
	int iVal;
        //float fVal;

	if (cindex < 0 || cindex >= (int)m_videoCodecs.size())
		return "<cindex out of range>";
	if (aindex < 0 || aindex >= (int)m_videoCodecs[cindex]->encoder_info.size())
		return "<aindex out of range>";

	switch (VCRComp_videoCodecAttribKind( cindex, aindex ))
	{
	case ATTRIB_INTEGER:
		Creators::GetCodecAttr( *m_videoCodecs[cindex],
						VCRComp_videoCodecAttribName( cindex, aindex ), iVal );
		sprintf( buf, "%d", iVal );
		break;

	case ATTRIB_STRING:
		Creators::GetCodecAttr( *m_videoCodecs[cindex],
						VCRComp_videoCodecAttribName( cindex, aindex ), (const char **)&cVal );
		strncpy( buf, cVal, 256 );
		free( cVal );
		break;

	case ATTRIB_SELECT:
		Creators::GetCodecAttr( *m_videoCodecs[cindex],
						VCRComp_videoCodecAttribName( cindex, aindex ), iVal );
		strncpy( buf, VCRComp_videoCodecAttribOption( cindex, aindex, iVal ), 256 );
		break;

/*	case ATTRIB_FLOAT:
		IVideoEncoder::GetExtendedAttr( m_videoCodecs[cindex]->fourcc,
						VCRComp_videoCodecAttribName( cindex, aindex ), fVal );
		sprintf( buf, "%f", fVal );
		break;*/

	default:
		strcpy( buf, "Unknown type!" );
		break;
	}

	return buf;
}

void
VCRComp_videoCodecAttribSetValue( int cindex, int aindex, const char *val )
{
	int i;

	if (cindex < 0 || cindex >= (int)m_videoCodecs.size())
		return;
	if (aindex < 0 || aindex >= (int)m_videoCodecs[cindex]->encoder_info.size())
		return;

	switch (VCRComp_videoCodecAttribKind( cindex, aindex ))
	{
	case ATTRIB_INTEGER:
		i = atoi( val );
		Creators::SetCodecAttr( *m_videoCodecs[cindex],
				VCRComp_videoCodecAttribName( cindex, aindex ), i );
		break;

	case ATTRIB_STRING:
		Creators::SetCodecAttr( *m_videoCodecs[cindex],
						VCRComp_videoCodecAttribName( cindex, aindex ), val );
		break;

	case ATTRIB_SELECT:
		for (int i = 0; i < VCRComp_numVideoCodecAttribOptions( cindex, aindex ); i++)
		{
			if (!strcmp( val, VCRComp_videoCodecAttribOption( cindex, aindex, i ) ))
			{
				Creators::SetCodecAttr( *m_videoCodecs[cindex],
						VCRComp_videoCodecAttribName( cindex, aindex ), i );
			}
		}
		break;
	}
}

int
VCRComp_numVideoCodecAttribOptions( int cindex, int aindex )
{
	if (cindex < 0 || cindex >= (int)m_videoCodecs.size())
		return 0;
	if (aindex < 0 || aindex >= (int)m_videoCodecs[cindex]->encoder_info.size())
		return 0;

	return m_videoCodecs[cindex]->encoder_info[aindex].options.size();
}

const char *
VCRComp_videoCodecAttribOption( int cindex, int aindex, int oindex )
{
	if (cindex < 0 || cindex >= (int)m_videoCodecs.size())
		return "<cindex out of range>";
	if (aindex < 0 || aindex >= (int)m_videoCodecs[cindex]->encoder_info.size())
		return "<aindex out of range>";
	if (oindex < 0 || oindex >= (int)m_videoCodecs[cindex]->encoder_info[aindex].options.size())
		return "<oindex out of range>";

	return m_videoCodecs[cindex]->encoder_info[aindex].options[oindex].c_str();
}

void
VCRComp_selectVideoCodec( int index )
{
	if (index < 0 || index >= (int)m_videoCodecs.size())
		return;

	m_selectedVideoCodec = index;
}



int
VCRComp_numAudioCodecs()
{
	return m_audioCodecs.size();
}

const char *
VCRComp_audioCodecName( int index )
{
	if (index < 0 || index >= (int)m_audioCodecs.size())
		return "<cindex out of range>";

	return m_audioCodecs[index]->GetName();
}

int
VCRComp_numAudioCodecAttribs( int index )
{
	if (index < 0 || index >= (int)m_audioCodecs.size())
		return 0;

	return m_audioCodecs[index]->encoder_info.size();
}

const char *
VCRComp_audioCodecAttribName( int cindex, int aindex )
{
	if (cindex < 0 || cindex >= (int)m_audioCodecs.size())
		return "<cindex out of range>";
	if (aindex < 0 || aindex >= (int)m_audioCodecs[cindex]->encoder_info.size())
		return "<aindex out of range>";

	return m_audioCodecs[cindex]->encoder_info[aindex].GetName();
}

int
VCRComp_audioCodecAttribKind( int cindex, int aindex )
{
	if (cindex < 0 || cindex >= (int)m_audioCodecs.size())
		return -1;
	if (aindex < 0 || aindex >= (int)m_audioCodecs[cindex]->encoder_info.size())
		return -1;

	switch (m_audioCodecs[cindex]->encoder_info[aindex].GetKind())
	{
	case AttributeInfo::Integer:
		return ATTRIB_INTEGER;
		break;

	case AttributeInfo::String:
		return ATTRIB_STRING;
		break;

	case AttributeInfo::Select:
		return ATTRIB_SELECT;
		break;

	case AttributeInfo::Float:
		return ATTRIB_FLOAT;
		break;
	}

	return -1;
}

const char *
VCRComp_audioCodecAttribValue( int cindex, int aindex )
{
	static char buf[256];
	char *cVal;
	int iVal;
	//float fVal;

	if (cindex < 0 || cindex >= (int)m_audioCodecs.size())
		return "<cindex out of range>";
	if (aindex < 0 || aindex >= (int)m_audioCodecs[cindex]->encoder_info.size())
		return "<aindex out of range>";

	switch (VCRComp_audioCodecAttribKind( cindex, aindex ))
	{
	case ATTRIB_INTEGER:
		Creators::GetCodecAttr( *m_audioCodecs[cindex],
						VCRComp_audioCodecAttribName( cindex, aindex ), iVal );
		sprintf( buf, "%d", iVal );
		break;

	case ATTRIB_STRING:
		Creators::GetCodecAttr( *m_audioCodecs[cindex],
						VCRComp_audioCodecAttribName( cindex, aindex ), (const char **)&cVal );
		strncpy( buf, cVal, 256 );
		free( cVal );
		break;

	case ATTRIB_SELECT:
		Creators::GetCodecAttr( *m_audioCodecs[cindex],
						VCRComp_audioCodecAttribName( cindex, aindex ), iVal );
		strncpy( buf, VCRComp_audioCodecAttribOption( cindex, aindex, iVal ), 256 );
		break;

/*	case ATTRIB_FLOAT:
		IVideoEncoder::GetExtendedAttr( m_videoCodecs[cindex]->fourcc,
						VCRComp_videoCodecAttribName( cindex, aindex ), fVal );
		sprintf( buf, "%f", fVal );
		break;*/

	default:
		strcpy( buf, "Unknown type!" );
		break;
	}

	return buf;
}

void
VCRComp_audioCodecAttribSetValue( int cindex, int aindex, const char *val )
{
	int i;

	if (cindex < 0 || cindex >= (int)m_audioCodecs.size())
		return;
	if (aindex < 0 || aindex >= (int)m_audioCodecs[cindex]->encoder_info.size())
		return;

	switch (VCRComp_audioCodecAttribKind( cindex, aindex ))
	{
	case ATTRIB_INTEGER:
		i = atoi( val );
		Creators::SetCodecAttr( *m_audioCodecs[cindex],
				VCRComp_audioCodecAttribName( cindex, aindex ), i );
		break;

	case ATTRIB_STRING:
		Creators::SetCodecAttr( *m_audioCodecs[cindex],
						VCRComp_audioCodecAttribName( cindex, aindex ), val );
		break;

	case ATTRIB_SELECT:
		for (int i = 0; i < VCRComp_numAudioCodecAttribOptions( cindex, aindex ); i++)
		{
			if (!strcmp( val, VCRComp_audioCodecAttribOption( cindex, aindex, i ) ))
			{
				Creators::SetCodecAttr( *m_audioCodecs[cindex],
						VCRComp_audioCodecAttribName( cindex, aindex ), i );
			}
		}
		break;
	}
}

int
VCRComp_numAudioCodecAttribOptions( int cindex, int aindex )
{
	if (cindex < 0 || cindex >= (int)m_audioCodecs.size())
		return 0;
	if (aindex < 0 || aindex >= (int)m_audioCodecs[cindex]->encoder_info.size())
		return 0;

        return m_audioCodecs[cindex]->encoder_info[aindex].options.size();
}

const char *
VCRComp_audioCodecAttribOption( int cindex, int aindex, int oindex )
{
	if (cindex < 0 || cindex >= (int)m_audioCodecs.size())
		return "<cindex out of range>";
	if (aindex < 0 || aindex >= (int)m_audioCodecs[cindex]->encoder_info.size())
		return "<aindex out of range>";
	if (oindex < 0 || oindex >= (int)m_audioCodecs[cindex]->encoder_info[aindex].options.size())
		return "<oindex out of range>";

	return m_audioCodecs[cindex]->encoder_info[aindex].options[oindex].c_str();
}

void
VCRComp_selectAudioCodec( int index )
{
	if (index < 0 || index >= (int)m_audioCodecs.size())
		return;

	m_selectedAudioCodec = index;
}

#endif // VCR_SUPPORT
