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

static const uint32_t gAudioControlReg [] = {
    kRegAud1Control, kRegAud2Control, kRegAud3Control, kRegAud4Control,
    kRegAud5Control, kRegAud6Control, kRegAud7Control, kRegAud8Control,0 };

static const ULWord    gAudioSystemToAudioSrcSelRegNum []    =
{
    kRegAud1SourceSelect,    kRegAud2SourceSelect,    kRegAud3SourceSelect,    kRegAud4SourceSelect,
    kRegAud5SourceSelect,    kRegAud6SourceSelect,    kRegAud7SourceSelect,    kRegAud8SourceSelect,    0
};

uint32_t GetAudioSourceSelectRegister(Ntv2SystemContext* context, NTV2AudioSystem audioSystem)
{
    return gAudioSystemToAudioSrcSelRegNum[audioSystem];
}

uint32_t GetAudioControlRegister(Ntv2SystemContext* context, NTV2AudioSystem audioSystem)
{
    return gAudioControlReg[audioSystem];
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

bool IsAudioRecording(Ntv2SystemContext* context, NTV2AudioSystem audioSystem)
{
    uint32_t control = GetAudioControlRegister(context, audioSystem);

    // Audio is (supposed) to be recording if BIT_8 is cleared (not in reset)
    if ((ntv2ReadRegister(context, control) & BIT_8) == 0)
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

void SetNumAudioChannels(
                         Ntv2SystemContext* context,
                         NTV2AudioSystem audioSystem,
                         ULWord numChannels)
{
    uint32_t control = GetAudioControlRegister(context, audioSystem);
    switch(numChannels) {
        case 16:
            AudioUpdateRegister(context, control, BIT_20, 0xFFFFFFFF);
            break;
        case 8:
            AudioUpdateRegister(context, control, BIT_16, ~BIT_20);
            break;
        default:
            AudioUpdateRegister(context, control, BIT_20, ~(BIT_20 | BIT_16));
            break;
            
    }
}

bool IsAudioInputRunning(Ntv2SystemContext* context, NTV2AudioSystem audioSystem)
{
    if (audioSystem >= NTV2_NUM_AUDIOSYSTEMS) {
        return false;
    }
    uint32_t stopped (0);
    auto control = GetAudioControlRegister(context, audioSystem);
    ntv2ReadRegisterMS(
                       context,
                       control,
                       &stopped,
                       kRegMaskResetAudioInput,
                       kRegShiftResetAudioInput
                       );
    
    return stopped == 0;
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
		default:	break;
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
		default:	break;
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
	default:	break;
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
    if (SupportStackedAudio(context))
    {
        *value = NTV2_AUDIO_BUFFER_SIZE_4MB;
        return true;
    }
    
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

bool SupportStackedAudio(Ntv2SystemContext* context)
{
    auto deviceID = (NTV2DeviceID)ntv2ReadRegister(context, kRegBoardID);
    
    switch (deviceID)
    {
        case DEVICE_ID_CORVID24:
        case DEVICE_ID_CORVID44:
        case DEVICE_ID_CORVID44_2X4K:
        case DEVICE_ID_CORVID44_8K:
        case DEVICE_ID_CORVID44_8KMK:
        case DEVICE_ID_CORVID44_PLNR:
        case DEVICE_ID_CORVID88:
        case DEVICE_ID_CORVIDHBR:
        case DEVICE_ID_CORVIDHEVC:
        case DEVICE_ID_IO4K:
        case DEVICE_ID_IO4KPLUS:
        case DEVICE_ID_IO4KUFC:
        case DEVICE_ID_IOIP_2022:
        case DEVICE_ID_IOIP_2110:
        case DEVICE_ID_IOIP_2110_RGB12:
        case DEVICE_ID_IOX3:
        case DEVICE_ID_KONA1:
        case DEVICE_ID_KONA3GQUAD:
        case DEVICE_ID_KONA4:
        case DEVICE_ID_KONA4UFC:
        case DEVICE_ID_KONA5:
        case DEVICE_ID_KONA5_2X4K:
        case DEVICE_ID_KONA5_3DLUT:
        case DEVICE_ID_KONA5_8K:
        case DEVICE_ID_KONA5_8KMK:
        case DEVICE_ID_KONA5_8K_MV_TX:
        case DEVICE_ID_KONA5_OE1:
        case DEVICE_ID_KONA5_OE10:
        case DEVICE_ID_KONA5_OE11:
        case DEVICE_ID_KONA5_OE12:
        case DEVICE_ID_KONA5_OE2:
        case DEVICE_ID_KONA5_OE3:
        case DEVICE_ID_KONA5_OE4:
        case DEVICE_ID_KONA5_OE5:
        case DEVICE_ID_KONA5_OE6:
        case DEVICE_ID_KONA5_OE7:
        case DEVICE_ID_KONA5_OE8:
        case DEVICE_ID_KONA5_OE9:
        case DEVICE_ID_KONAHDMI:
        case DEVICE_ID_KONAIP_1RX_1TX_1SFP_J2K:
        case DEVICE_ID_KONAIP_1RX_1TX_2110:
        case DEVICE_ID_KONAIP_2022:
        case DEVICE_ID_KONAIP_2110:
        case DEVICE_ID_KONAIP_2110_RGB12:
        case DEVICE_ID_KONAIP_25G:
        case DEVICE_ID_KONAIP_2TX_1SFP_J2K:
        case DEVICE_ID_KONAIP_4CH_2SFP:
        case DEVICE_ID_KONAX:
        case DEVICE_ID_KONAXM:
        case DEVICE_ID_SOFTWARE:
        case DEVICE_ID_SOJI_3DLUT:
        case DEVICE_ID_SOJI_DIAGS:
        case DEVICE_ID_SOJI_OE1:
        case DEVICE_ID_SOJI_OE2:
        case DEVICE_ID_SOJI_OE3:
        case DEVICE_ID_SOJI_OE4:
        case DEVICE_ID_SOJI_OE5:
        case DEVICE_ID_SOJI_OE6:
        case DEVICE_ID_SOJI_OE7:
        case DEVICE_ID_TTAP_PRO:
            return true;
    #if defined(_DEBUG)
        case DEVICE_ID_CORVID1:
        case DEVICE_ID_CORVID22:
        case DEVICE_ID_CORVID3G:
        case DEVICE_ID_IOEXPRESS:
        case DEVICE_ID_IOXT:
        case DEVICE_ID_KONA3G:
        case DEVICE_ID_KONALHEPLUS:
        case DEVICE_ID_KONALHI:
        case DEVICE_ID_KONALHIDVI:
        case DEVICE_ID_NOTFOUND:
        case DEVICE_ID_TTAP:
    #else
        default:
    #endif
            break;
    }    //    switch on inDeviceID
    return false;
}
