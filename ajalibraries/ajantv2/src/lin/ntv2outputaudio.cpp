/////////////////////////////////////////////////////////////////////////////
// NTV2OutputAudio.cpp 
//
// Description:  


#include "ntv2outputaudio.h"
#include "ntv2debug.h"
#include "ntv2devicefeatures.h"
#include <iostream>
#include <stdlib.h>

#if defined(AJALinux)
#include "ntv2winlinhacks.h"
#endif

using namespace std;



CNTV2OutputAudio::CNTV2OutputAudio(UWord boardNumber,bool displayErrorMessage, NTV2DeviceType ulBoardType )
:   CNTV2Status(boardNumber, displayErrorMessage,  ulBoardType) 

{
	_currentAudioOffset = 0;
	_dmaEngine = NTV2_DMA_FIRST_AVAILABLE; // reasonable default
	SetupAudio();
}



CNTV2OutputAudio::~CNTV2OutputAudio()
{

}

bool CNTV2OutputAudio::Open(UWord boardNumber, bool displayError, NTV2DeviceType eBoardType )
{
	CNTV2LinuxDriverInterface::Open(boardNumber,displayError,eBoardType);
	return SetupAudio();
}


bool CNTV2OutputAudio::SetupAudio()
{
	if ( IsOpen() )
	{
		iFrameNumber		= GetAudioFrameBufferNumber();
		ULWord fpgaRevision=0;
		ReadRegister(kRegStatus,&fpgaRevision,kRegMaskFPGAVersion,kRegShiftFPGAVersion);
		#if !defined (NTV2_DEPRECATE)
		if ( GetBoardID() == BOARD_ID_XENAX2 && fpgaRevision > 0 )
		{
			ULWord channel1Compressed;
			ULWord channel2Compressed;
			ReadRegister(kRegCh1Control,&channel1Compressed,kRegMaskChannelCompressed,kRegShiftChannelCompressed);
			ReadRegister(kRegCh2Control,&channel2Compressed,kRegMaskChannelCompressed,kRegShiftChannelCompressed);
			if ( channel1Compressed || channel2Compressed )
				iFrameNumber = 31;
			else
				iFrameNumber = 15;
		}
		#endif	//	!defined (NTV2_DEPRECATE)
		if ( NTV2DeviceGetMaxAudioChannels(GetDeviceID()) == 16 )
		{
			SetNumberAudioChannels(16);
			SetAudioBufferSize(NTV2_AUDIO_BUFFER_BIG);
		}

		InitAudioBit();
		GetAudioWrapAddress(_audioWrapAddress);

		return true;
	}
	else
		return false;
}

bool CNTV2OutputAudio::SetBoard(UWord boardNumber)
{
	return CNTV2Card::Open(boardNumber);
}

ULWord CNTV2OutputAudio::GetHardwareAudioOutputPointer()
{
	ULWord value;
	ReadRegister(kRegAud1OutputLastAddr,&value);
	return value;
}


ULWord CNTV2OutputAudio::GetCurrentHardwareAudioOutputPointer()
{
	ULWord value;
	ReadRegister(kRegAud1Counter,&value);
	return value;
}


void CNTV2OutputAudio::ResetAudioOutput()
{
	WriteRegister(kRegAud1Control,1,kRegMaskResetAudioOutput,kRegShiftResetAudioOutput);
	_currentAudioOffset = 0;
}

void CNTV2OutputAudio::PauseAudioOutput()
{
	WriteRegister(kRegAud1Control,1,kRegMaskPauseAudio,kRegShiftPauseAudio);
}

void CNTV2OutputAudio::UnPauseAudioOutput()
{
	WriteRegister(kRegAud1Control,0,kRegMaskPauseAudio,kRegShiftPauseAudio);
}

void CNTV2OutputAudio::StartAudio(NTV2_GlobalAudioPlaybackMode mode)
{
	SetAudioOutputMode(mode);
}

bool CNTV2OutputAudio::IsAudioRunning()
{
	ULWord value;
	ReadRegister(kRegAud1Control,&value,kRegMaskResetAudioOutput,kRegShiftResetAudioOutput);
	return !value;
}

bool CNTV2OutputAudio::InitAudioBit()
{
	WriteRegister(kRegAud1Control,1,BIT(12),12);
	return true;
}

bool CNTV2OutputAudio::DeinitAudioBit()
{
	WriteRegister(kRegAud1Control,0,BIT(12),12);
	return true;
}



bool CNTV2OutputAudio::CanAddAudioSamples( ULWord audioSize)
{
	ULWord prebytes = 0;
 	ULWord postbytes = 0;

	ULWord currentAudioOutputPointer = GetHardwareAudioOutputPointer();

    if ((_currentAudioOffset + audioSize) <= _audioWrapAddress)
    {
	    prebytes = audioSize;
		postbytes = 0;

		// see if this audioBuffer crosses over the currentAudioOutputPointer
		if (( currentAudioOutputPointer > _currentAudioOffset ) && 
			( currentAudioOutputPointer < (_currentAudioOffset + audioSize )))
		{
			return false;
		}
		else
			return true;

    }
    else
    {
		bool audioRunning = IsAudioRunning();

		if ( !audioRunning )
			return false;    // wrapped before starting audio

		prebytes = _audioWrapAddress - _currentAudioOffset;
		postbytes = audioSize - prebytes;


		// see if this audioBuffer crosses over the currentAudioOutputPointer
		if ( currentAudioOutputPointer > _currentAudioOffset )
		{
			return false;
		}
		else if ( currentAudioOutputPointer < postbytes )
		{
			return false;
		}
		else
			return true;
    }
}

bool CNTV2OutputAudio::AddAudioSamples(ULWord* audioBuffer, ULWord audioSize)
{
	ULWord prebytes = 0;
 	ULWord postbytes = 0;

	ULWord currentAudioOutputPointer = GetHardwareAudioOutputPointer();
	bool audioRunning = IsAudioRunning();

    if ((_currentAudioOffset + audioSize) < _audioWrapAddress)
    {
	    prebytes = audioSize;
		postbytes = 0;

		if ( audioRunning)
		{
			// see if this audioBuffer crosses over the currentAudioOutputPointer
			if (( currentAudioOutputPointer > _currentAudioOffset ) && 
				( currentAudioOutputPointer < (_currentAudioOffset + audioSize )))
			{
				return false;
			}

		}
    }
    else
    {
		if ( !audioRunning )
			return false;    // wrapped before starting audio

		prebytes = _audioWrapAddress - _currentAudioOffset;
		postbytes = audioSize - prebytes;

		if ( audioRunning)
		{
			// see if this audioBuffer crosses over the currentAudioOutputPointer
			if ( currentAudioOutputPointer > _currentAudioOffset ) 
			{
				return false;
			}
			else
			if ( currentAudioOutputPointer < postbytes )
			{
				return false;
			}
		}

    }


	// Since audiosize is non zero we can assume we will always have some prebytes to DMA
	// Also calculate the new dmaAudioOffset_ (this may change in the wrap case but most of the
	// time this will be the new value.
	// This call blocks until DMA is complete
//	odprintf("wrap(%d) current(%d) hw(%d) as(%d) pre(%d) post(%d)\n", _audioWrapAddress, _currentAudioOffset, currentAudioOutputPointer, audioSize, prebytes, postbytes);
	bool status;
	char debugString[80];
	_dmaEngine = NTV2_DMA_FIRST_AVAILABLE; // reasonable default
	if ( prebytes )
	{
		status = DMAWrite(iFrameNumber, audioBuffer, _currentAudioOffset, prebytes);
		if ( status == false )
		{
			sprintf(debugString,"pre ...DmaWrite wrap retrying...Buffer(%08X),Offset(%08X),NumBytes(%d)\n",
				(ULWord) *(audioBuffer),_currentAudioOffset,prebytes);
			OutputDebugString(debugString);
			exit (1);
		}
	}
	_currentAudioOffset += prebytes;

	// Now postbytes will only be non zero in the case where we wrap around the audio buffer so
	// check for this and do the partial DMA to get the rest of the audio that wrapped around and
	// then reset dmaAudioOffset_.
	if (postbytes)
	{
		status = DMAWrite(iFrameNumber, audioBuffer + (prebytes / 4), 0, postbytes);
		if ( status == false )
		{
			sprintf(debugString,"post ...DmaWrite wrap retrying...Buffer(%08X),Offset(%08X),NumBytes(%d)\n",
				(ULWord) *(audioBuffer + (prebytes / 4)),0,postbytes);
			OutputDebugString(debugString);
		}
		_currentAudioOffset = postbytes ;
	}

	return true;
}
bool CNTV2OutputAudio::AddAudioSamples(ULWord* audioBuffer, UWord numSamples) // based on current NumberAudioChannels
{
	ULWord numChannels;
	GetNumberAudioChannels(numChannels);
	return AddAudioSamples(audioBuffer, (ULWord)numSamples*numChannels*4);
}
