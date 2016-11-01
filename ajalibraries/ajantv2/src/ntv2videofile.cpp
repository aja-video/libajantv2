/**
	@file		ntv2videofile.cpp
	@brief		Implements the CNTV2VideoFile class.
	@copyright	(C) 2011-2016 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#include "ntv2diskio.h"
#include "ntv2videofile.h"
#include "ntv2utils.h"

#include <math.h>


CNTV2VideoFile::CNTV2VideoFile():
	_hasVideo(false),
    _hasAudio(false),
    _hasRP188(false),
    _videoFormat(NTV2_FORMAT_UNKNOWN),
	_frameBufferFormat(NTV2_FBF_10BIT_YCBCR),
	_frameRate(NTV2_FRAMERATE_UNKNOWN),
	_audioRate(NTV2_AUDIO_48K),
	_audioNumChannels(0)
{
	
}

CNTV2VideoFile::~CNTV2VideoFile()
{

}

////////////
// Setters

// video
void CNTV2VideoFile::SetHasVideo(bool b)
{
	_hasVideo = b;
}

void CNTV2VideoFile::SetVideoFormat(NTV2VideoFormat vf)
{
	_videoFormat = vf;
}

void CNTV2VideoFile::SetFrameBufferFormat(NTV2FrameBufferFormat fbf)
{
	_frameBufferFormat = fbf;
}

void CNTV2VideoFile::SetFrameRate(NTV2FrameRate fr)
{
	_frameRate = fr;
}

void CNTV2VideoFile::SetNumFrames(ULWord64 nf)
{
	_numFrames = nf;
}

// audio
void CNTV2VideoFile::SetHasAudio(bool b)
{
	_hasAudio = b;
}

void CNTV2VideoFile::SetAudioRate(NTV2AudioRate ar)
{
	_audioRate = ar;
}

void CNTV2VideoFile::SetAudioNumChannels(UByte ac)
{
	_audioNumChannels = ac;
}

void CNTV2VideoFile::SetAudioNumSamples(ULWord64 ans)
{
	_audioNumSamples = ans;
}


// RP188
void CNTV2VideoFile::SetHasRP188(bool b)
{
	_hasRP188 = b;
}




// Getters

// video

bool CNTV2VideoFile::HasVideo() const
{
	return _hasVideo;
}

bool CNTV2VideoFile::HasAudio() const
{
	return _hasAudio;
}

bool CNTV2VideoFile::HasRP188() const
{
	return _hasRP188;
}

NTV2VideoFormat CNTV2VideoFile::GetVideoFormat() const
{
	return _videoFormat;
}

NTV2FrameBufferFormat CNTV2VideoFile::GetFrameBufferFormat() const
{
	return _frameBufferFormat;
}

NTV2FrameRate CNTV2VideoFile::GetFrameRate() const
{
	return _frameRate;
}

ULWord64 CNTV2VideoFile::GetNumFrames() const
{
	return _numFrames;
}

ULWord CNTV2VideoFile::GetVideoFrameSize() const
{
	return GetVideoActiveSize(_videoFormat, _frameBufferFormat);
}

ULWord CNTV2VideoFile::GetVideoWriteSize() const
{
	return ::GetVideoWriteSize(_videoFormat, _frameBufferFormat);
}

ULWord CNTV2VideoFile::GetVideoDiskSize() const
{
	ULWord ulVideoDiskSize = GetVideoFrameSize();
    if (ulVideoDiskSize % DEFAULT_SECTOR_SIZE)
		ulVideoDiskSize = ((ulVideoDiskSize / DEFAULT_SECTOR_SIZE) + 1) * DEFAULT_SECTOR_SIZE;
    
    return ulVideoDiskSize;
}

ULWord CNTV2VideoFile::GetVideoRate() const
{
	// Caller expects a percent.  Multiply by 100 and round
	// to the nearest integer.
	return (ULWord)( 100.0 * GetFramesPerSecond(_frameRate) + 0.5 );
}

set<NTV2FrameBufferFormat> CNTV2VideoFile::GetSupportedFrameBufferFormats() const
{
	set<NTV2FrameBufferFormat> result;
	for( ULWord i = 0; i < (ULWord)NTV2_FBF_NUMFRAMEBUFFERFORMATS; i++)
	{
		result.insert((NTV2FrameBufferFormat)i);
	}
	return result;
}

// audio
NTV2AudioRate CNTV2VideoFile::GetAudioRate() const
{
	return _audioRate;
}

UByte CNTV2VideoFile::GetAudioNumChannels() const
{
	return _audioNumChannels;
}

ULWord64 CNTV2VideoFile::GetAudioNumSamples() const
{
	return _audioNumSamples;
}

ULWord CNTV2VideoFile::GetAudioSampleRate() const
{
	return (_audioRate == NTV2_AUDIO_48K) ? 48000 : 96000;
}

ULWord CNTV2VideoFile::GetAudioSampleSize() const
{
	return GetAudioNumChannels() * NTV2_AUDIOSAMPLESIZE;
}

NTV2FrameRate CNTV2VideoFile::GetFrameRate(double fps) const
{
	NTV2FrameRate result = NTV2_FRAMERATE_UNKNOWN;
	double best = 1000.0;
	
	NTV2FrameRate rates[] = {
		NTV2_FRAMERATE_6000,
		NTV2_FRAMERATE_5994,
		NTV2_FRAMERATE_3000,
		NTV2_FRAMERATE_2997,
		NTV2_FRAMERATE_2500,
		NTV2_FRAMERATE_2400,
		NTV2_FRAMERATE_2398,
		NTV2_FRAMERATE_5000,
		NTV2_FRAMERATE_1900,
		NTV2_FRAMERATE_1898,
		NTV2_FRAMERATE_1800,
		NTV2_FRAMERATE_1798,
		NTV2_FRAMERATE_1500,
		NTV2_FRAMERATE_1498
	};
	
	for(ULWord i = 0; i < sizeof(rates) / sizeof(NTV2FrameRate); i++)
	{
		NTV2FrameRate candidate = rates[i];
		double distance = fabs(GetFramesPerSecond(candidate) - fps);
		if( distance < best )
		{
			result = candidate;
			best = distance;
		}
	}
	
	return result;
}

NTV2VideoFormat CNTV2VideoFile::GetVideoFormatFromFrameRate( NTV2FrameRate frameRate, bool tenEighty, bool progressive ) const
{
	switch ( frameRate )
	{
		case NTV2_FRAMERATE_2500:
			if ( progressive )
			{
				if( tenEighty )
					return NTV2_FORMAT_1080p_2500;
				else
					return NTV2_FORMAT_720p_2500;
			}
			else
			{
				return NTV2_FORMAT_1080i_5000;
			}
			break;
		
		case NTV2_FRAMERATE_5994:
			if ( tenEighty )
				return NTV2_FORMAT_1080p_5994_A;
			else
				return NTV2_FORMAT_720p_5994;
		
		case NTV2_FRAMERATE_6000:
			return NTV2_FORMAT_720p_6000;
		
		case NTV2_FRAMERATE_3000:
			return NTV2_FORMAT_1080i_6000;
		
		case NTV2_FRAMERATE_5000:
			if ( tenEighty )
				return NTV2_FORMAT_1080p_5000_A;
			else
				return NTV2_FORMAT_720p_5000;
		
		case NTV2_FRAMERATE_2997:
			if ( progressive )
				return NTV2_FORMAT_1080p_2997;
			else
				return NTV2_FORMAT_1080i_5994;
		
		case NTV2_FRAMERATE_2398:
			if ( tenEighty )
				return NTV2_FORMAT_1080p_2398;
			else
				return NTV2_FORMAT_720p_2398;
		
		case NTV2_FRAMERATE_2400:
			return NTV2_FORMAT_4x2048x1080p_2400;
		
		case NTV2_FRAMERATE_1498:
			return NTV2_FORMAT_2K_1498;
		
		case NTV2_FRAMERATE_1500:
			return NTV2_FORMAT_2K_1500;
	}
	
	return NTV2_FORMAT_UNKNOWN;
}
