/*

  Module : CNTV2ToneGenerator.cpp

  Copyright (C) 2004 AJA Video Systems, Inc.  Proprietary and Confidential information.

  Note that it is important to call CNTV2ToneGenerator::Quit() before deleting the object.

*/

#include "ntv2tonegenerator.h"
#include "ntv2utils.h"
#include "fixed.h"
#include <stdio.h>

#if defined(AJALinux)
# include <signal.h>
# include <errno.h>
#endif

double tonefrequencies[16] = {	220.00000000000,	293.33333333326,	391.111111110916,	521.48148148109,	695.308641974613,	927.078189299252,	1236.10425239869,	1648.13900319785,
								2197.51867092991,	2930.02489457248,	3906.69985942900,	5208.93314590403,	6945.24419453698,	9260.32559271365,	12347.1007902818,	16462.8010537049};
double toneamplitudes[16] = {	0.05,	0.10,	0.15,	0.20,	0.25,	0.30,	0.35,	0.40,	0.45,	0.50,	0.55,	0.60,	0.65,	0.70,	0.75,	0.80};


void CNTV2ToneGenerator::InitThread()
{
#if defined(AJALinux)
	_isDying = 0;
#endif
}

 
void CNTV2ToneGenerator::Loop()
{
	fprintf(stderr, "%s called id(%d)\n", __FUNCTION__, _isDying);
	_ntv2Card.Open( _boardNumber,false,_boardType );

	if ( _ntv2Card.IsOpen() )
	{
		_ntv2Card.SetAudioRate(NTV2_AUDIO_48K);
		#if !defined (NTV2_DEPRECATE)
		if ( _ntv2Card.GetDeviceID() == BOARD_ID_XENAX2 || _ntv2Card.GetDeviceID() == BOARD_ID_XENA2) {
			_ntv2Card.SetAudioBufferSize(NTV2_AUDIO_BUFFER_BIG);
			_ntv2Card.SetNumberAudioChannels(16);
		}
		else
		{
			_ntv2Card.SetAudioBufferSize(NTV2_AUDIO_BUFFER_STANDARD);
			_ntv2Card.SetNumberAudioChannels(8);
		}
		#else	//	defined (NTV2_DEPRECATE)
		_ntv2Card.SetAudioBufferSize(NTV2_AUDIO_BUFFER_BIG);
		_ntv2Card.SetNumberAudioChannels(NTV2BoardGetMaxAudioChannels (_ntv2Card.GetDeviceID()), NTV2_AUDIOSYSTEM_1);
		#endif	//	defined (NTV2_DEPRECATE)

		// reset audio and load up some tone and then let her go.
		_ntv2Card.SubscribeOutputVerticalEvent(NTV2_CHANNEL1);
		_ntv2Card.ResetAudioOutput();
		NTV2FrameRate frameRate;
		_ntv2Card.GetFrameRate(&frameRate);
		ULWord numChannels;
		_ntv2Card.GetNumberAudioChannels(numChannels);

		_ntv2Card.InitAudioBit(); /// NOTE: This must be called for proper behavior of output audio class.

		ULWord currentSample = 0;
		ULWord currentFrame = 0;
		ULWord* audioBuffer = new ULWord[400000];

		// Preload some audio
		ULWord audioBufferSize;
		ULWord numSamples = (ULWord)GetAudioSamplesPerFrame(frameRate,_audioRate,currentFrame++);

		double audioSampleRate = 48000.0;
		if ( _audioRate == NTV2_AUDIO_96K )
			audioSampleRate = 96000.0;

#if defined(MSWindows)
		if ( _toneFrequency == -1.0 )
		{
			HANDLE h;
			if ( numChannels == 6)
				h = CreateFile("ChannelCount6x48k",GENERIC_READ,0,NULL,OPEN_EXISTING,0,NULL);
			else if ( numChannels == 8)
				h = CreateFile("ChannelCount8x48k",GENERIC_READ,0,NULL,OPEN_EXISTING,0,NULL);
			else if ( numChannels == 16)
				h = CreateFile("ChannelCount16x48k",GENERIC_READ,0,NULL,OPEN_EXISTING,0,NULL);

			if ( h  != INVALID_HANDLE_VALUE )
			{
				_audioFileBuffer = new ULWord[numChannels*48000];
				DWORD numBytesRead;
				ReadFile(h ,(void*)_audioFileBuffer,numChannels*48000*4,&numBytesRead,NULL);
				CloseHandle(h);
				_audioReadSamplePointer = 0;
				_numSamplesInFile = 48000;
				audioBufferSize = AddAudioFromFileBuffer((ULWord*)audioBuffer,numSamples,numChannels);
			}
			else
			{
				_toneFrequency = 0.0;
			}
		}
		else
#endif
		{
			if ( _toneFrequency == -2.0 )
			{
				audioBufferSize = AddAudioTestPattern((ULWord*)audioBuffer,currentSample,numSamples,1024,false,numChannels);
			}
			else
			{
				if ( _toneFrequency == 0.0 ) {
					audioBufferSize = AddAudioTone((ULWord*)audioBuffer,currentSample,numSamples,audioSampleRate,toneamplitudes,tonefrequencies,32,false,numChannels);
				} else {
					audioBufferSize = AddAudioTone((ULWord*)audioBuffer,currentSample,numSamples,audioSampleRate,_toneAmplitude,_toneFrequency,32,false,numChannels);
				}

				_ntv2Card.AddAudioSamples(audioBuffer,audioBufferSize);

				numSamples = (ULWord)GetAudioSamplesPerFrame(frameRate,_audioRate,currentFrame++);
			}
			if (  _toneFrequency == -1.0)
			{
				audioBufferSize = AddAudioFromFileBuffer((ULWord*)audioBuffer,numSamples,numChannels);
			}
			else
			{
				if ( _toneFrequency == -2.0 ) {
					audioBufferSize = AddAudioTestPattern(audioBuffer,currentSample,numSamples,1024,false,numChannels);
				} else {
					if ( _toneFrequency == 0.0 ) {
						audioBufferSize = AddAudioTone(audioBuffer,currentSample,numSamples,audioSampleRate,toneamplitudes,tonefrequencies,32,false,numChannels);
					} else {
						audioBufferSize = AddAudioTone(audioBuffer,currentSample,numSamples,audioSampleRate,_toneAmplitude,_toneFrequency,32,false,numChannels);
					}
				}
			}
			_ntv2Card.AddAudioSamples(audioBuffer,audioBufferSize);

		_ntv2Card.StartAudio(NTV2_AUDIOPLAYBACK_NOW);

        bool done = false;
        while ( !done )
        {
			numSamples = (ULWord)GetAudioSamplesPerFrame(frameRate,_audioRate,currentFrame++);
			if (  _toneFrequency == -1.0)
				audioBufferSize = AddAudioFromFileBuffer((ULWord*)audioBuffer,numSamples,numChannels);
			else
			if ( _toneFrequency == -2.0 )
				audioBufferSize = AddAudioTestPattern(audioBuffer,currentSample,numSamples,1024,false,numChannels);
			else
			if ( _toneFrequency == 0.0 )
				audioBufferSize = AddAudioTone((ULWord*)audioBuffer,currentSample,numSamples,audioSampleRate,toneamplitudes,tonefrequencies,32,false,numChannels);
			else
				audioBufferSize = AddAudioTone((ULWord*)audioBuffer,currentSample,numSamples,audioSampleRate,_toneAmplitude,_toneFrequency,32,false,numChannels);

			bool status;
			do {
				// check to see if we're feeding audio too fast.
				status = _ntv2Card.AddAudioSamples(audioBuffer,audioBufferSize);
//				fprintf(stderr, "%s:  _isDying(%d) status(%d) done(%d)\n", __FUNCTION__, _isDying, status, done);

				if ( _isDying )
					done = true;

			} while ( status == false && done == false);

			_ntv2Card.WaitForOutputFieldID (NTV2_FIELD0); // luckily works for progressive modes as well
		}

		_ntv2Card.ResetAudioOutput();
		_ntv2Card.SetAudioOutputMode(NTV2_AUDIOPLAYBACK_NORMALAUTOCIRCULATE);

		delete [] audioBuffer;
		if ( _audioFileBuffer )
			delete [] _audioFileBuffer;
		_audioFileBuffer = 0;
		}
	}
	_active = false;
}


ULWord CNTV2ToneGenerator::AddAudioFromFileBuffer(ULWord* audioBuffer,ULWord numSamples, ULWord numChannels)
{
	ULWord audioTransferSize = numChannels*numSamples*4;
	if ( (_audioReadSamplePointer + numSamples) >= _numSamplesInFile )
	{
		ULWord numSamplesTillWrap = _numSamplesInFile-_audioReadSamplePointer;
		::CopyMemory(audioBuffer,&_audioFileBuffer[numChannels*_audioReadSamplePointer],numChannels*numSamplesTillWrap*4);
		audioTransferSize -= numChannels*numSamplesTillWrap*4;
		::CopyMemory(audioBuffer+(numSamplesTillWrap*numChannels),_audioFileBuffer,audioTransferSize);

		_audioReadSamplePointer = numSamples - numSamplesTillWrap;
	}
	else
	{
		::CopyMemory(audioBuffer,&_audioFileBuffer[numChannels*_audioReadSamplePointer],audioTransferSize);
		_audioReadSamplePointer += numSamples;
	}

	return numChannels*numSamples*4;
}

void CNTV2ToneGenerator::Quit()
{
	_isDying++;
	while ( _active )
		Sleep(100);
}

