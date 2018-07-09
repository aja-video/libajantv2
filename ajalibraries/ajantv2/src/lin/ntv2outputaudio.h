/////////////////////////////////////////////////////////////////////////////
// NTV2OutputAudio.h
#ifndef NTV2OutputAudio_H
#define NTV2OutputAudio_H

#include "ntv2card.h"
#if defined(MSWindows)
  #include "ntv2windriverinterface.h"
#endif

#if defined(AJALinux)
  #include "ntv2linuxdriverinterface.h"
#endif

class CNTV2OutputAudio : public CNTV2Card 
{
public:  // Constructors
	CNTV2OutputAudio() {};
	CNTV2OutputAudio(UWord boardNumber);
	~CNTV2OutputAudio();

	virtual bool Open(UWord boardNumber=0);

	bool SetupAudio();

public:  // Methods
	virtual bool SetBoard(UWord boardNumber);
	
	bool CanAddAudioSamples( ULWord bufferSize);
	bool AddAudioSamples(ULWord* buffer, ULWord bufferSize);
	bool AddAudioSamples(ULWord* buffer, UWord numSamples); // based on current GetNumAudioChannels
	void ResetAudioOutput();
	void PauseAudioOutput();
	void UnPauseAudioOutput();
	void StartAudio(NTV2_GlobalAudioPlaybackMode mode);
	bool IsAudioRunning();

	void SetDMAEngine(NTV2DMAEngine dmaEngine) { _dmaEngine = dmaEngine;}
	NTV2DMAEngine GetDMAEngine() {return _dmaEngine;}

	bool InitAudioBit();
	bool DeinitAudioBit();

protected:  // Methods
	ULWord GetHardwareAudioOutputPointer();
	ULWord GetCurrentHardwareAudioOutputPointer();

protected:  // Data
	ULWord                       _currentAudioOffset;
	int iFrameNumber;
	ULWord		  _audioWrapAddress;
	ULWord		  _audioReadBufferOffset;
	NTV2DMAEngine _dmaEngine;
};

#endif
