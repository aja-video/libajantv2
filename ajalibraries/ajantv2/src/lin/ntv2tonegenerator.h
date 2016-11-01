/*
  Module: NTV2ToneGenerator.h

  Copyright (C) 2004 AJA Video Systems, Inc.  Proprietary and Confidential information.

*/

#ifndef NTV2ToneGenerator_H
#define NTV2ToneGenerator_H

#include "ajatypes.h"
#include "ntv2enums.h"
#include "audiodefines.h"
#include "ntv2active.h"
#include "ntv2outputaudio.h"

class CNTV2ToneGenerator : public ActiveObject
{
public:
	CNTV2ToneGenerator(UWord boardNumber,
			  float frequency,
			  NTV2AudioRate audioRate,
			  NTV2DeviceType boardType= DEVICETYPE_UNKNOWN,
			  float amplitude=.25
			)
        : _boardNumber(boardNumber),
		  _toneFrequency(frequency),
		  _audioRate(audioRate),
		  _boardType(boardType),
		  _toneAmplitude(amplitude),
		  _audioFileBuffer(0),
		  _audioReadSamplePointer(0),
          _active(true) 

	{
		_thread.Resume ();
	}

    ~CNTV2ToneGenerator() { Kill(); }
	void Quit();
	bool Active() { return _active; }


    virtual void InitThread ();
    virtual void Loop ();
    virtual void FlushThread () {_active=false; }

	ULWord AddAudioFromFileBuffer(ULWord* audioBuffer,ULWord numSamples, ULWord numChannels);

protected:

	CNTV2OutputAudio	_ntv2Card;
	UWord				_boardNumber;
	float				_toneFrequency;
	NTV2AudioRate		_audioRate;
	NTV2DeviceType		_boardType;
	float				_toneAmplitude;
	ULWord*				_audioFileBuffer;
	ULWord				_numSamplesInFile;
	ULWord				_audioReadSamplePointer;
    bool				_active;

};

#endif
