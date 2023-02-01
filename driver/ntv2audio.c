/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
//==========================================================================
//
//  ntv2audio.c
//
//==========================================================================

#include "ntv2system.h"
#include "ntv2audio.h"
#include "ntv2devicefeatures.h"
#include "ntv2video.h"
#include "ntv2kona.h"

static const uint32_t gAudioRateHighMask [] = {
	kRegMaskAud1RateHigh, kRegMaskAud2RateHigh, kRegMaskAud3RateHigh, kRegMaskAud4RateHigh,
	kRegMaskAud5RateHigh, kRegMaskAud6RateHigh, kRegMaskAud7RateHigh, kRegMaskAud8RateHigh };

static const uint32_t gAudioRateHighShift [] = {
	kRegShiftAud1RateHigh, kRegShiftAud2RateHigh, kRegShiftAud3RateHigh, kRegShiftAud4RateHigh,
	kRegShiftAud5RateHigh, kRegShiftAud6RateHigh, kRegShiftAud7RateHigh, kRegShiftAud8RateHigh };

uint32_t GetAudioControlRegister(Ntv2SystemContext* context, NTV2AudioSystem audioSystem)
{
	NTV2DeviceID deviceID = (NTV2DeviceID)ntv2ReadRegister(context, kRegBoardID);

	if(NTV2DeviceCanDoAudioN(deviceID, NTV2_AUDIOSYSTEM_2) && audioSystem == NTV2_AUDIOSYSTEM_2)
		return kRegAud2Control;
	else if(NTV2DeviceCanDoAudioN(deviceID, NTV2_AUDIOSYSTEM_3) && audioSystem == NTV2_AUDIOSYSTEM_3)
		return kRegAud3Control;
	else if(NTV2DeviceCanDoAudioN(deviceID, NTV2_AUDIOSYSTEM_4) && audioSystem == NTV2_AUDIOSYSTEM_4)
		return kRegAud4Control;
	else if(NTV2DeviceCanDoAudioN(deviceID, NTV2_AUDIOSYSTEM_5) && audioSystem == NTV2_AUDIOSYSTEM_5)
		return kRegAud5Control;
	else if(NTV2DeviceCanDoAudioN(deviceID, NTV2_AUDIOSYSTEM_6) && audioSystem == NTV2_AUDIOSYSTEM_6)
		return kRegAud6Control;
	else if(NTV2DeviceCanDoAudioN(deviceID, NTV2_AUDIOSYSTEM_7) && audioSystem == NTV2_AUDIOSYSTEM_7)
		return kRegAud7Control;
	else if(NTV2DeviceCanDoAudioN(deviceID, NTV2_AUDIOSYSTEM_8) && audioSystem == NTV2_AUDIOSYSTEM_8)
		return kRegAud8Control;
	else
		return kRegAud1Control;
}

void AudioUpdateRegister(Ntv2SystemContext* context, uint32_t reg, uint32_t preOr, uint32_t postAnd)
{
	uint32_t data;

	data = ntv2ReadRegister(context, reg);
	data |= preOr;
	data &= postAnd;
	ntv2WriteRegister(context, reg, data);
}

void StartAudioPlayback(Ntv2SystemContext* context, NTV2AudioSystem audioSystem)
{
	uint32_t control = GetAudioControlRegister(context, audioSystem);
//	KdPrint(("StartAudioPlayback\n");

	AudioUpdateRegister(context, control, BIT_12, (~(BIT_9 | BIT_11 | BIT_14))); //Clear the Audio Output reset bit!
}

void StopAudioPlayback(Ntv2SystemContext* context, NTV2AudioSystem audioSystem)
{
	uint32_t control = GetAudioControlRegister(context, audioSystem);
//	ntv2Message("StopAudioPlayback(%d)\n", audioSystem);

	// Reset Audio Playback... basically stops it.
	AudioUpdateRegister(context, control, BIT_9 , (uint32_t)-1);
}

bool IsAudioPlaybackStopped(Ntv2SystemContext* context, NTV2AudioSystem audioSystem)
{
	uint32_t control = GetAudioControlRegister(context, audioSystem);
	uint32_t regValue = ntv2ReadRegister(context, control);

	if (regValue & BIT_9)
		return true;
	else
		return false;
}

bool IsAudioPlaying(Ntv2SystemContext* context, NTV2AudioSystem audioSystem)
{ 
	uint32_t control = GetAudioControlRegister(context, audioSystem);

	// Audio is (supposed) to be playing if BIT_9 is cleared (not in reset)
	if ((ntv2ReadRegister(context, control) & BIT_9) == 0)
		return 1;

	return 0;
}

void PauseAudioPlayback(Ntv2SystemContext* context, NTV2AudioSystem audioSystem)
{
	uint32_t control = GetAudioControlRegister(context, audioSystem);

	// Reset Audio Playback... basically stops it.
	if(!IsAudioPlaybackPaused(context, audioSystem))
	{
		AudioUpdateRegister(context, control, BIT_11 , (uint32_t)-1);
	}
}

void UnPauseAudioPlayback(Ntv2SystemContext* context, NTV2AudioSystem audioSystem)
{
	uint32_t control = GetAudioControlRegister(context, audioSystem);

	// Reset Audio Playback... basically stops it.
	if(IsAudioPlaybackPaused(context, audioSystem))
	{
		AudioUpdateRegister(context, control, BIT_11 , ~BIT_11);
	}
}

bool IsAudioPlaybackPaused(Ntv2SystemContext* context, NTV2AudioSystem audioSystem)
{
	uint32_t control = GetAudioControlRegister(context, audioSystem);
	uint32_t regValue = ntv2ReadRegister(context, control);

	if (regValue & BIT_11)
		return true;
	else
		return false;
}

void StartAudioCapture(Ntv2SystemContext* context, NTV2AudioSystem audioSystem)
{
	ntv2Message("StartAudioCapture(%d)\n", audioSystem);
	uint32_t control = GetAudioControlRegister(context, audioSystem);

	AudioUpdateRegister(context, control, BIT_8, (~(BIT_0 | BIT_14))	);
	AudioUpdateRegister(context, control, BIT_0, (~BIT_8)	);
}

void StopAudioCapture(Ntv2SystemContext* context, NTV2AudioSystem audioSystem)
{
	ntv2Message("StopAudioCapture(%d)\n", audioSystem);
	uint32_t control = GetAudioControlRegister(context, audioSystem);

	AudioUpdateRegister(context, control, BIT_8, ~BIT_0	);
}

bool IsAudioCaptureStopped(Ntv2SystemContext* context, NTV2AudioSystem audioSystem)
{
	uint32_t control = GetAudioControlRegister(context, audioSystem);
	uint32_t regValue = ntv2ReadRegister(context, control);

	if ((regValue & BIT_8) || !(regValue & BIT_0))
		return true;
	else
		return false;
}

uint32_t GetNumAudioChannels(Ntv2SystemContext* context, NTV2AudioSystem audioSystem)
{
	uint32_t control = GetAudioControlRegister(context, audioSystem);
	uint32_t regValue = ntv2ReadRegister(context, control);

	if (regValue & BIT(20) )
		return 16;

	if (regValue & BIT(16))
		return 8;
	else
		return 6;
}

uint32_t oemAudioSampleAlign(Ntv2SystemContext* context, NTV2AudioSystem audioSystem, uint32_t ulReadSample) 
{
	// 6 (samples) * 4 (bytes per sample)
	uint32_t numBytesPerAudioSample = GetNumAudioChannels(context, audioSystem)*4;
	return ((ulReadSample / numBytesPerAudioSample) * numBytesPerAudioSample);
}

uint32_t GetAudioLastOut(Ntv2SystemContext* context, NTV2AudioSystem audioSystem)
{
	uint32_t regNum;
	switch(audioSystem)
	{
	case NTV2_AUDIOSYSTEM_2:
		regNum = kRegAud2OutputLastAddr;
		break;
	case NTV2_AUDIOSYSTEM_3:
		regNum = kRegAud3OutputLastAddr;
		break;
	case NTV2_AUDIOSYSTEM_4:
		regNum = kRegAud4OutputLastAddr;
		break;
	case NTV2_AUDIOSYSTEM_5:
		regNum = kRegAud5OutputLastAddr;
		break;
	case NTV2_AUDIOSYSTEM_6:
		regNum = kRegAud6OutputLastAddr;
		break;
	case NTV2_AUDIOSYSTEM_7:
		regNum = kRegAud7OutputLastAddr;
		break;
	case NTV2_AUDIOSYSTEM_8:
		regNum = kRegAud8OutputLastAddr;
		break;
	default:
		regNum = kRegAud1OutputLastAddr;
		break;
	}
	return ntv2ReadRegister(context, regNum);
}

uint32_t GetAudioLastIn(Ntv2SystemContext* context, NTV2AudioSystem audioSystem)
{
	uint32_t regNum;
	switch(audioSystem)
	{
	case NTV2_AUDIOSYSTEM_2:
		regNum = kRegAud2InputLastAddr;
		break;
	case NTV2_AUDIOSYSTEM_3:
		regNum = kRegAud3InputLastAddr;
		break;
	case NTV2_AUDIOSYSTEM_4:
		regNum = kRegAud4InputLastAddr;
		break;
	case NTV2_AUDIOSYSTEM_5:
		regNum = kRegAud5InputLastAddr;
		break;
	case NTV2_AUDIOSYSTEM_6:
		regNum = kRegAud6InputLastAddr;
		break;
	case NTV2_AUDIOSYSTEM_7:
		regNum = kRegAud7InputLastAddr;
		break;
	case NTV2_AUDIOSYSTEM_8:
		regNum = kRegAud8InputLastAddr;
		break;
	default:
		regNum = kRegAud1InputLastAddr;
		break;
	}
	return ntv2ReadRegister(context, regNum);
}

uint32_t GetAudioSamplesPerFrame(Ntv2SystemContext* context, NTV2AudioSystem audioSystem, uint32_t cadenceFrame, bool fieldMode)
{
	NTV2Channel channel = NTV2_CHANNEL1;
	if(IsMultiFormatActive(context))
	{
		switch(audioSystem)
		{
		case NTV2_AUDIOSYSTEM_1:
			channel = NTV2_CHANNEL1;
			break;
		case NTV2_AUDIOSYSTEM_2:
			channel = NTV2_CHANNEL2;
			break;
		case NTV2_AUDIOSYSTEM_3:
			channel = NTV2_CHANNEL3;
			break;
		case NTV2_AUDIOSYSTEM_4:
			channel = NTV2_CHANNEL4;
			break;
		case NTV2_AUDIOSYSTEM_5:
			channel = NTV2_CHANNEL5;
			break;
		case NTV2_AUDIOSYSTEM_6:
			channel = NTV2_CHANNEL6;
			break;
		case NTV2_AUDIOSYSTEM_7:
			channel = NTV2_CHANNEL7;
			break;
		case NTV2_AUDIOSYSTEM_8:
			channel = NTV2_CHANNEL8;
			break;
		default:	break;
		}
	}
	NTV2FrameRate frameRate=GetFrameRate(context, channel);
	NTV2AudioRate audioRate=GetAudioRate(context, audioSystem);	
	bool smpte372Enabled = GetSmpte372(context, channel);
	uint32_t audioSamplesPerFrame=0;
	cadenceFrame %= 5;
	// adjust framerate if doing 1080p5994,1080p60 or 1080p50

	if(smpte372Enabled || fieldMode)
	{
		switch (frameRate)
		{
		case NTV2_FRAMERATE_3000:
			frameRate = NTV2_FRAMERATE_6000;
			break;
		case NTV2_FRAMERATE_2997:
			frameRate = NTV2_FRAMERATE_5994;
			break;
		case NTV2_FRAMERATE_2500:
			frameRate = NTV2_FRAMERATE_5000;
			break;
		case NTV2_FRAMERATE_2400:
			frameRate = NTV2_FRAMERATE_4800;
			break;
		case NTV2_FRAMERATE_2398:
			frameRate = NTV2_FRAMERATE_4795;
			break;
		default:	break;
		}
	}

	switch (audioRate)
	{
	case NTV2_AUDIO_48K:
		switch (frameRate)
		{
		case NTV2_FRAMERATE_12000:
			audioSamplesPerFrame = 400;
			break;
		case NTV2_FRAMERATE_11988:
			switch (cadenceFrame)
			{
			case 0:
			case 2:
			case 4:
				audioSamplesPerFrame = 400;
				break;
			case 1:
			case 3:
				audioSamplesPerFrame = 401;
				break;
			}
			break;
		case NTV2_FRAMERATE_6000:
			audioSamplesPerFrame = 800;
			break;
		case NTV2_FRAMERATE_5994:
			switch (cadenceFrame)
			{
			case 0:
				audioSamplesPerFrame = 800;
				break;					
			case 1:
				audioSamplesPerFrame = 801;
				break;
			case 2:
				audioSamplesPerFrame = 801;
				break;
			case 3:
				audioSamplesPerFrame = 801;
				break;
			case 4:
				audioSamplesPerFrame = 801;
				break;
			}
			break;
		case NTV2_FRAMERATE_5000:
			audioSamplesPerFrame = 1920/2;
			break;
		case NTV2_FRAMERATE_4800:
			audioSamplesPerFrame = 1000;
			break;
		case NTV2_FRAMERATE_4795:
			audioSamplesPerFrame = 1001;
			break;
		case NTV2_FRAMERATE_3000:
			audioSamplesPerFrame = 1600;
			break;
		case NTV2_FRAMERATE_2997:
			// depends on cadenceFrame;
			switch (cadenceFrame)
			{
			case 0:
				audioSamplesPerFrame = 1602;
				break;					
			case 1:
				audioSamplesPerFrame = 1601;
				break;
			case 2:
				audioSamplesPerFrame = 1602;
				break;
			case 3:
				audioSamplesPerFrame = 1601;
				break;
			case 4:
				audioSamplesPerFrame = 1602;
				break;
			}
			break;
		case NTV2_FRAMERATE_2500:
			audioSamplesPerFrame = 1920;
			break;
		case NTV2_FRAMERATE_2400:
			audioSamplesPerFrame = 2000;
			break;
		case NTV2_FRAMERATE_2398:
			audioSamplesPerFrame = 2002;
			break;
		case NTV2_FRAMERATE_1500:
			audioSamplesPerFrame = 3200;
			break;
		case NTV2_FRAMERATE_1498:
			// depends on cadenceFrame;
			switch (cadenceFrame)
			{
			case 0:
				audioSamplesPerFrame = 3204;
				break;					
			case 1:
			case 2:
			case 3:
			case 4:
				audioSamplesPerFrame = 3203;
				break;
			}
			break;
		case NTV2_FRAMERATE_UNKNOWN:
			audioSamplesPerFrame = 0;
			break;
		}
		break;
		
	case NTV2_AUDIO_96K:
		switch (frameRate)
		{
		case NTV2_FRAMERATE_6000:
			audioSamplesPerFrame = 800*2;
			break;
		case NTV2_FRAMERATE_5000:
			audioSamplesPerFrame = 1920;
			break;
		case NTV2_FRAMERATE_5994:
			switch (cadenceFrame)
			{
			case 0:
				audioSamplesPerFrame = 1602;
				break;					
			case 1:
				audioSamplesPerFrame = 1601;
				break;
			case 2:
				audioSamplesPerFrame = 1602;
				break;
			case 3:
				audioSamplesPerFrame = 1601;
				break;
			case 4:
				audioSamplesPerFrame = 1602;
				break;
			}
			break;
		case NTV2_FRAMERATE_3000:
			audioSamplesPerFrame = 1600*2;
			break;
		case NTV2_FRAMERATE_2997:
			// depends on cadenceFrame;
			switch (cadenceFrame)
			{
			case 0:
				audioSamplesPerFrame = 3204;
				break;					
			case 1:
				audioSamplesPerFrame = 3203;
				break;
			case 2:
				audioSamplesPerFrame = 3203;
				break;
			case 3:
				audioSamplesPerFrame = 3203;
				break;
			case 4:
				audioSamplesPerFrame = 3203;
				break;
			}
			break;
		case NTV2_FRAMERATE_2500:
			audioSamplesPerFrame = 1920*2;
			break;
		case NTV2_FRAMERATE_2400:
			audioSamplesPerFrame = 2000*2;
			break;
		case NTV2_FRAMERATE_2398:
			audioSamplesPerFrame = 2002*2;
			break;
		case NTV2_FRAMERATE_1500:
			audioSamplesPerFrame = 3200*2;
			break;
		case NTV2_FRAMERATE_1498:
			// depends on cadenceFrame;
			switch (cadenceFrame)
			{
			case 0:
				audioSamplesPerFrame = 3204*2;
				break;					
			case 1:
			case 2:
			case 3:
			case 4:
				audioSamplesPerFrame = 3203*2;
				break;
			}
			break;				
		case NTV2_FRAMERATE_UNKNOWN:
			audioSamplesPerFrame = 0*2; //haha
			break;
		}

		break;

	case NTV2_AUDIO_192K:
		switch (frameRate)
		{
		case NTV2_FRAMERATE_12000:
			audioSamplesPerFrame = 1600;
			break;
		case NTV2_FRAMERATE_11988:
			switch (cadenceFrame)
			{
			case 0:
			case 2:
			case 4:
				audioSamplesPerFrame = 1602;
				break;
			case 1:
			case 3:
				audioSamplesPerFrame = 1601;
				break;
			}
			break;
		case NTV2_FRAMERATE_6000:
			audioSamplesPerFrame = 3200;
			break;
		case NTV2_FRAMERATE_5994:
			switch (cadenceFrame)
			{
			case 0:
				audioSamplesPerFrame = 3204;
				break;
			case 1:
			case 2:
			case 3:
			case 4:
				audioSamplesPerFrame = 3203;
				break;
			}
			break;
		case NTV2_FRAMERATE_5000:
			audioSamplesPerFrame = 3840;
			break;
		case NTV2_FRAMERATE_4800:
			audioSamplesPerFrame = 4000;
			break;
		case NTV2_FRAMERATE_4795:
			audioSamplesPerFrame = 4004;
			break;
		case NTV2_FRAMERATE_3000:
			audioSamplesPerFrame = 6400;
			break;
		case NTV2_FRAMERATE_2997:
			// depends on cadenceFrame;
			switch (cadenceFrame)
			{
			case 0:
			case 1:
				audioSamplesPerFrame = 6407;
				break;
			case 2:
			case 3:
			case 4:
				audioSamplesPerFrame = 6406;
				break;
			}
			break;
		case NTV2_FRAMERATE_2500:
			audioSamplesPerFrame = 7680;
			break;
		case NTV2_FRAMERATE_2400:
			audioSamplesPerFrame = 8000;
			break;
		case NTV2_FRAMERATE_2398:
			audioSamplesPerFrame = 8008;
			break;
		case NTV2_FRAMERATE_1500:
			audioSamplesPerFrame = 12800;
			break;
		case NTV2_FRAMERATE_1498:
			// depends on cadenceFrame;
			switch (cadenceFrame)
			{
			case 0:
			case 1:
			case 2:
			case 3:
				audioSamplesPerFrame = 12813;
				break;
			case 4:
				audioSamplesPerFrame = 12812;
				break;
			}
			break;
#if !defined(NTV2_DEPRECATE_16_0)
		case NTV2_FRAMERATE_1900:	// Not supported yet
		case NTV2_FRAMERATE_1898:	// Not supported yet
		case NTV2_FRAMERATE_1800: 	// Not supported yet
		case NTV2_FRAMERATE_1798:	// Not supported yet
#endif	//	!defined(NTV2_DEPRECATE_16_0)
		case NTV2_FRAMERATE_UNKNOWN:
		case NTV2_NUM_FRAMERATES:
			audioSamplesPerFrame = 0*2; //haha
			break;
		}
		break;
	}

	return audioSamplesPerFrame;
}

NTV2AudioRate GetAudioRate(Ntv2SystemContext* context, NTV2AudioSystem audioSystem)
{
	NTV2AudioRate outRate = NTV2_AUDIO_48K;
	uint32_t		control;
	uint32_t		rateLow;
	uint32_t		rateHigh;

	control = GetAudioControlRegister(context, audioSystem);
	ntv2ReadRegisterMS(context, control, &rateLow, kRegMaskAudioRate, kRegShiftAudioRate);
	if (rateLow == 1)
		outRate = NTV2_AUDIO_96K;
	
	ntv2ReadRegisterMS(context,
					   kRegAudioControl2,
					   &rateHigh,
					   gAudioRateHighMask[audioSystem],
					   gAudioRateHighShift[audioSystem]);
	if (rateHigh == 1)
		outRate = NTV2_AUDIO_192K;
		
	return outRate;
}

uint32_t GetAudioSamplesPerSecond(Ntv2SystemContext* context, NTV2AudioSystem audioSystem)
{
	NTV2AudioRate rate;
	uint32_t sps = 48000;

	rate = GetAudioRate(context, audioSystem);
	if (rate == NTV2_AUDIO_96K)
		sps = 96000;
	if (rate == NTV2_AUDIO_192K)
		sps = 192000;

	return sps;		
}

uint32_t GetAudioTransferInfo(Ntv2SystemContext* context,
							NTV2AudioSystem audioSystem,
							uint32_t currentOffset,
							uint32_t numBytesToTransfer,
							uint32_t* preWrapBytes,
							uint32_t* postWrapBytes)
{
	uint32_t nextOffset = currentOffset + numBytesToTransfer;
	uint32_t wrapAddress = GetAudioWrapAddress(context, audioSystem);
	if (nextOffset >=  wrapAddress)
	{
		*preWrapBytes = wrapAddress - currentOffset;
		*postWrapBytes = nextOffset - wrapAddress;
		nextOffset  = *postWrapBytes;
	}
	else
	{
		*preWrapBytes = nextOffset-currentOffset;
		*postWrapBytes = 0;
	}

	return nextOffset;
}

uint32_t GetAudioWrapAddress(Ntv2SystemContext* context, NTV2AudioSystem audioSystem)
{
	NTV2AudioBufferSize bufferSize;
	GetAudioBufferSize(context, audioSystem, &bufferSize);

	if (bufferSize == NTV2_AUDIO_BUFFER_BIG)
		return NTV2_AUDIO_WRAPADDRESS_BIG;
	else 
		return NTV2_AUDIO_WRAPADDRESS;

}

bool GetAudioBufferSize(Ntv2SystemContext* context, NTV2AudioSystem audioSystem, NTV2AudioBufferSize *value)
{
	uint32_t control = GetAudioControlRegister(context, audioSystem);

	return ntv2ReadRegisterMS(context,
							  control,
							  (uint32_t*)value,
							  (uint32_t)kK2RegMaskAudioBufferSize,
							  kK2RegShiftAudioBufferSize);
}

uint32_t GetAudioReadOffset(Ntv2SystemContext* context, NTV2AudioSystem audioSystem)
{
	NTV2AudioBufferSize bufferSize;
	GetAudioBufferSize(context, audioSystem, &bufferSize);

	if (bufferSize == NTV2_AUDIO_BUFFER_BIG)
		return NTV2_AUDIO_READBUFFEROFFSET_BIG;
	else 
		return NTV2_AUDIO_READBUFFEROFFSET;
}

