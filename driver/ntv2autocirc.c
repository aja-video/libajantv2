/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
//==========================================================================
//
//  ntv2autocirc.c
//
//==========================================================================

#include "ntv2autocirc.h"
#include "ntv2video.h"
#include "ntv2audio.h"
#include "ntv2autofunc.h"
#include "ntv2fixed.h"

static char* CrosspointName[] =
{
	"Channel 1",
	"Channel 2",
	"Input 1",
	"Input 2",
	"Matte",
	"FGKey",
	"Channel 3",
	"Channel 4",
	"Input 3",
	"Input 4",
	"Channel 5",
	"Channel 6",
	"Channel 7",
	"Channel 8",
	"Input 5",
	"Input 6",
	"Input 7",
	"Input 8"
};

static const uint32_t	gChannelToOutputFrameReg[] = {
	kRegCh1OutputFrame, kRegCh2OutputFrame, kRegCh3OutputFrame, kRegCh4OutputFrame,
	kRegCh5OutputFrame, kRegCh6OutputFrame, kRegCh7OutputFrame, kRegCh8OutputFrame, 0 };
static const uint32_t	gChannelToInputFrameReg[] = {
	kRegCh1InputFrame, kRegCh2InputFrame, kRegCh3InputFrame, kRegCh4InputFrame,
	kRegCh5InputFrame, kRegCh6InputFrame, kRegCh7InputFrame, kRegCh8InputFrame, 0 };
static const uint32_t	gChannelToControlRegNum [] = {
	kRegCh1Control, kRegCh2Control, kRegCh3Control, kRegCh4Control, kRegCh5Control, kRegCh6Control,
	kRegCh7Control, kRegCh8Control, 0};
static const uint32_t gNTV2InputSourceToANCChannel[NTV2_NUM_INPUTSOURCES+1] = {
	NTV2_CHANNEL_INVALID, NTV2_CHANNEL_INVALID, NTV2_CHANNEL_INVALID, NTV2_CHANNEL_INVALID, NTV2_CHANNEL_INVALID,
	NTV2_CHANNEL1, NTV2_CHANNEL2, NTV2_CHANNEL3, NTV2_CHANNEL4,
	NTV2_CHANNEL5, NTV2_CHANNEL6, NTV2_CHANNEL7, NTV2_CHANNEL8, NTV2_CHANNEL_INVALID};

//-------------------------------------------------------------------------------------------------------
//	AutoCirculateControl
//-------------------------------------------------------------------------------------------------------
Ntv2Status AutoCirculateControl(NTV2AutoCirc* pAutoCirc, AUTOCIRCULATE_DATA* psControl)
{
	Ntv2Status status = NTV2_STATUS_SUCCESS;

	switch (psControl->eCommand)
	{
	case eInitAutoCirc:
		status = AutoCirculateInit (pAutoCirc,
									   psControl->channelSpec, 
									   psControl->lVal1, 
									   psControl->lVal2,
									   (NTV2AudioSystem)psControl->lVal3,
									   psControl->lVal4,
									   psControl->bVal1, 
									   psControl->bVal2, 
									   psControl->bVal3, 
									   psControl->bVal4, 
									   psControl->bVal5, 
									   psControl->bVal6, 
									   psControl->bVal7,
									   psControl->bVal8,
									   ((psControl->lVal6 & AUTOCIRCULATE_WITH_FIELDS) != 0),
									   ((psControl->lVal6 & AUTOCIRCULATE_WITH_HDMIAUX) != 0));
		break;

	case eStartAutoCirc:
		status = AutoCirculateStart(pAutoCirc, psControl->channelSpec, 0);
		break;

	case eStartAutoCircAtTime:
		{
			uint64_t ulHigh = (uint32_t)psControl->lVal1;
			uint64_t ulLow = (uint32_t)psControl->lVal2;
			int64_t startTime = (int64_t)((ulHigh << 32) | ulLow);
			status = AutoCirculateStart(pAutoCirc, psControl->channelSpec, startTime);
			break;
		}

	case eStopAutoCirc:
		status = AutoCirculateStop(pAutoCirc, psControl->channelSpec);
		break;

	case eAbortAutoCirc:
		status = AutoCirculateAbort(pAutoCirc, psControl->channelSpec);
		break;

	case ePauseAutoCirc:
		status = AutoCirculatePause(pAutoCirc, psControl->channelSpec, psControl->bVal1, psControl->bVal2);
		break;

	case eFlushAutoCirculate:
        status = AutoCirculateFlush(pAutoCirc, psControl->channelSpec, psControl->bVal1);
		break;

	case ePrerollAutoCirculate:
		status = AutoCirculatePreroll(pAutoCirc, psControl->channelSpec, psControl->lVal1);
		break;

	case eSetActiveFrame:
		status = AutoCirculateSetActiveFrame(pAutoCirc, psControl->channelSpec, psControl->lVal1);
		break;

	default:
		status = NTV2_STATUS_BAD_PARAMETER;
		break;
	}

	// sync channels for playback
	NTV2Crosspoint channel = NTV2CROSSPOINT_FGKEY;
	bool bDoSync = false;
	if(ntv2ReadVirtualRegister(pAutoCirc->pSysCon, kVRegSyncChannels) != 0)
	{
		uint32_t iChannel;
		for(iChannel = GetIndexForNTV2Crosspoint(NTV2CROSSPOINT_CHANNEL1); iChannel <= GetIndexForNTV2Crosspoint(NTV2CROSSPOINT_INPUT8); iChannel++)
		{
			if(iChannel == GetIndexForNTV2Crosspoint(psControl->channelSpec))
			{
				continue;
			}
			INTERNAL_AUTOCIRCULATE_STRUCT* pAuto = &pAutoCirc->autoCirculate[iChannel];
			if(pAuto->state != NTV2_AUTOCIRCULATE_DISABLED && pAuto->state != NTV2_AUTOCIRCULATE_STOPPING)
			{
				channel =  GetNTV2CrosspointForIndex(iChannel);
				pAutoCirc->syncChannel1 = psControl->channelSpec;
				pAutoCirc->syncChannel2 = channel;
				bDoSync = true;
				break;
			}
		}
	}

	if(bDoSync)
	{
		switch (psControl->eCommand)
		{
		case eStartAutoCirc:
			status = AutoCirculateStart(pAutoCirc, channel, 0);
			break;

		case eStartAutoCircAtTime:
			{
				uint64_t ulHigh = (uint32_t)psControl->lVal1;
				uint64_t ulLow = (uint32_t)psControl->lVal2;
				int64_t startTime = (int64_t)((ulHigh << 32) | ulLow);
				status = AutoCirculateStart(pAutoCirc, channel, startTime);
				break;
			}

		case eStopAutoCirc:
			status = AutoCirculateStop(pAutoCirc,channel);
			break;

		case eAbortAutoCirc:
			status = AutoCirculateAbort(pAutoCirc,channel);
			break;

		case ePauseAutoCirc:
			status = AutoCirculatePause(pAutoCirc,channel, psControl->bVal1, psControl->bVal2);
			break;

		case eFlushAutoCirculate:
            status = AutoCirculateFlush(pAutoCirc,channel, psControl->bVal1);
			break;

		case ePrerollAutoCirculate:
			status = AutoCirculatePreroll(pAutoCirc,channel, psControl->lVal1);
			break;

		case eSetActiveFrame:
			status = AutoCirculateSetActiveFrame(pAutoCirc, channel, psControl->lVal1);

		default:
			status = NTV2_STATUS_BAD_PARAMETER;
			break;
		}
	}
	else
	{
		pAutoCirc->syncChannel1 = NTV2CROSSPOINT_FGKEY;
		pAutoCirc->syncChannel2 = NTV2CROSSPOINT_FGKEY;
	}

	return status;
}

//-------------------------------------------------------------------------------------------------------
//	AutoCirculateInit
//-------------------------------------------------------------------------------------------------------
Ntv2Status AutoCirculateInit(NTV2AutoCirc* pAutoCirc,
								NTV2Crosspoint lChannelSpec, 
								int32_t lStartFrameNum,
								int32_t lEndFrameNum, 
								NTV2AudioSystem lAudioSystem,
								int32_t lChannelCount,
								bool bWithAudio, 
								bool bWithRP188, 
								bool bFbfChange, 
								bool bFboChange , 
								bool bWithColorCorrection, 
								bool bWithVidProc,
								bool bWithCustomAncData,
								bool bWithLTC,
								bool bWithFields,
								bool bWithHDMIAux)
{
	if (ILLEGAL_CHANNELSPEC(lChannelSpec))
        return NTV2_STATUS_BAD_PARAMETER;

	NTV2DeviceID deviceID = pAutoCirc->deviceID;
	Ntv2SystemContext* pSysCon = pAutoCirc->pSysCon;

	uint32_t channelRange = lEndFrameNum - lStartFrameNum + 1;
	uint32_t csIndex = GetIndexForNTV2Crosspoint(lChannelSpec);
	if(lChannelCount > 1)
		pAutoCirc->bMultiChannel = true;
	else
		pAutoCirc->bMultiChannel = false;

	if (!NTV2DeviceCanDoCustomAnc(deviceID))
		bWithCustomAncData = false;

	//Mac SpinlockAcquire

	for(int32_t loopCount = 0; loopCount < lChannelCount; loopCount++)
	{
		NTV2Crosspoint channelSpecAtIndex = GetNTV2CrosspointForIndex(csIndex);
		csIndex++;
		NTV2Channel ACChannel = GetNTV2ChannelForNTV2Crosspoint(channelSpecAtIndex);

		INTERNAL_AUTOCIRCULATE_STRUCT* pAuto = &pAutoCirc->autoCirculate[channelSpecAtIndex];
		if (pAuto->state != NTV2_AUTOCIRCULATE_DISABLED)
			return NTV2_STATUS_BUSY;

		AutoCirculateReset(pAutoCirc, channelSpecAtIndex);  // Reset AutoCirculate Database

		pAuto->pSysCon						= pAutoCirc->pSysCon;
		pAuto->pFunCon						= pAutoCirc->pFunCon;
		pAuto->channelSpec					= channelSpecAtIndex;
		pAuto->startFrame					= lStartFrameNum + (loopCount * channelRange);
		pAuto->endFrame						= lEndFrameNum + (loopCount * channelRange);
		pAuto->channelCount					= (loopCount == 0) ? lChannelCount : 0;
		pAuto->currentFrame					= lStartFrameNum + (loopCount * channelRange);
		pAuto->circulateWithAudio			= (loopCount == 0) ? bWithAudio : false;
		pAuto->circulateWithRP188			= (loopCount == 0) ? bWithRP188 : false;
		pAuto->enableFbfChange				= (loopCount == 0) ? bFbfChange : false;
		pAuto->enableFboChange				= (loopCount == 0) ? bFboChange : false;
		pAuto->circulateWithColorCorrection	= (loopCount == 0) ? bWithColorCorrection : false;
		pAuto->circulateWithVidProc			= (loopCount == 0) ? bWithVidProc : false;
		pAuto->circulateWithCustomAncData	= (loopCount == 0) ? bWithCustomAncData : false;
		pAuto->circulateWithLTC             = (loopCount == 0) ? bWithLTC : false;
		pAuto->circulateWithFields			= bWithFields;
		pAuto->circulateWithHDMIAux			= (loopCount == 0) ? bWithHDMIAux : false;
		pAuto->audioSystem					= lAudioSystem;

		// Setup register so next frame interrupt will clock in frame values.
		ntv2Message("CNTV2Device::AutoCirculateInit - Auto %s: using frames %d to %d\n",
			CrosspointName[pAuto->channelSpec], pAuto->startFrame, pAuto->endFrame);

		SetMode(pSysCon, ACChannel, NTV2_IS_INPUT_CROSSPOINT(pAuto->channelSpec) ? NTV2_MODE_CAPTURE : NTV2_MODE_DISPLAY);
		if (!NTV2DeviceCanDo12gRouting(deviceID))
		{
			if (Get425FrameEnable(pSysCon, NTV2_CHANNEL1) && (pAuto->channelSpec == NTV2CROSSPOINT_CHANNEL1 || pAuto->channelSpec == NTV2CROSSPOINT_INPUT1))
			{
				SetMode(pSysCon, NTV2_CHANNEL2, NTV2_IS_INPUT_CROSSPOINT(pAuto->channelSpec) ? NTV2_MODE_CAPTURE : NTV2_MODE_DISPLAY);
			}
			else if (Get425FrameEnable(pSysCon, NTV2_CHANNEL3) && (pAuto->channelSpec == NTV2CROSSPOINT_CHANNEL3 || pAuto->channelSpec == NTV2CROSSPOINT_INPUT3))
			{
				SetMode(pSysCon, NTV2_CHANNEL4, NTV2_IS_INPUT_CROSSPOINT(pAuto->channelSpec) ? NTV2_MODE_CAPTURE : NTV2_MODE_DISPLAY);
			}
			else if (Get425FrameEnable(pSysCon, NTV2_CHANNEL5) && (pAuto->channelSpec == NTV2CROSSPOINT_CHANNEL5 || pAuto->channelSpec == NTV2CROSSPOINT_INPUT5))
			{
				SetMode(pSysCon, NTV2_CHANNEL6, NTV2_IS_INPUT_CROSSPOINT(pAuto->channelSpec) ? NTV2_MODE_CAPTURE : NTV2_MODE_DISPLAY);
			}
			else if (Get425FrameEnable(pSysCon, NTV2_CHANNEL7) && (pAuto->channelSpec == NTV2CROSSPOINT_CHANNEL7 || pAuto->channelSpec == NTV2CROSSPOINT_INPUT7))
			{
				SetMode(pSysCon, NTV2_CHANNEL8, NTV2_IS_INPUT_CROSSPOINT(pAuto->channelSpec) ? NTV2_MODE_CAPTURE : NTV2_MODE_DISPLAY);
			}
		}
		else if (GetQuadQuadFrameEnable(pSysCon, NTV2_CHANNEL1))
		{
			SetMode(pSysCon, NTV2_CHANNEL2, NTV2_IS_INPUT_CROSSPOINT(pAuto->channelSpec) ? NTV2_MODE_CAPTURE : NTV2_MODE_DISPLAY);
			SetMode(pSysCon, NTV2_CHANNEL3, NTV2_IS_INPUT_CROSSPOINT(pAuto->channelSpec) ? NTV2_MODE_CAPTURE : NTV2_MODE_DISPLAY);
			SetMode(pSysCon, NTV2_CHANNEL4, NTV2_IS_INPUT_CROSSPOINT(pAuto->channelSpec) ? NTV2_MODE_CAPTURE : NTV2_MODE_DISPLAY);
		}
		
		if (Get4kSquaresEnable(pSysCon, NTV2_CHANNEL1) && (pAuto->channelSpec == NTV2CROSSPOINT_CHANNEL1 || pAuto->channelSpec == NTV2CROSSPOINT_INPUT1))
		{
			SetMode(pSysCon, NTV2_CHANNEL2, NTV2_IS_INPUT_CROSSPOINT(pAuto->channelSpec) ? NTV2_MODE_CAPTURE : NTV2_MODE_DISPLAY);
			SetMode(pSysCon, NTV2_CHANNEL3, NTV2_IS_INPUT_CROSSPOINT(pAuto->channelSpec) ? NTV2_MODE_CAPTURE : NTV2_MODE_DISPLAY);
			SetMode(pSysCon, NTV2_CHANNEL4, NTV2_IS_INPUT_CROSSPOINT(pAuto->channelSpec) ? NTV2_MODE_CAPTURE : NTV2_MODE_DISPLAY);
		}
		else if (Get4kSquaresEnable(pSysCon, NTV2_CHANNEL5) && (pAuto->channelSpec == NTV2CROSSPOINT_CHANNEL5 || pAuto->channelSpec == NTV2CROSSPOINT_INPUT5))
		{
			SetMode(pSysCon, NTV2_CHANNEL6, NTV2_IS_INPUT_CROSSPOINT(pAuto->channelSpec) ? NTV2_MODE_CAPTURE : NTV2_MODE_DISPLAY);
			SetMode(pSysCon, NTV2_CHANNEL7, NTV2_IS_INPUT_CROSSPOINT(pAuto->channelSpec) ? NTV2_MODE_CAPTURE : NTV2_MODE_DISPLAY);
			SetMode(pSysCon, NTV2_CHANNEL8, NTV2_IS_INPUT_CROSSPOINT(pAuto->channelSpec) ? NTV2_MODE_CAPTURE : NTV2_MODE_DISPLAY);
		}

		if (pAuto->circulateWithAudio)
		{
			if (NTV2_IS_INPUT_CROSSPOINT(pAuto->channelSpec))
			{
				StopAudioCapture(pSysCon, pAuto->audioSystem);
				StartAudioCapture(pSysCon, pAuto->audioSystem);
			}
			else
			{
				pAuto->nextAudioOutputAddress = 0;
				pAuto->audioDropsRequired = 0;
				pAuto->audioDropsCompleted = 0;
				StopAudioPlayback(pSysCon, pAuto->audioSystem);
			}
			ntv2Message("CNTV2Device::AutoCirculateInit - Auto %s: circulateWithAudio%d\n",
						CrosspointName[pAuto->channelSpec], pAuto->audioSystem);
		}

		pAutoCirc->ancInputChannel[ACChannel] = ACChannel;
		if (ntv2ReadVirtualRegister(pSysCon, kVRegEveryFrameTaskFilter) == NTV2_STANDARD_TASKS)
		{
			NTV2InputSource sourceSelect = (NTV2InputSource)ntv2ReadVirtualRegister(pSysCon, kVRegCustomAncInputSelect);
			pAutoCirc->ancInputChannel[ACChannel] = (NTV2Channel)gNTV2InputSourceToANCChannel[sourceSelect];
		}
		if (pAuto->circulateWithCustomAncData)
		{
			if (NTV2_IS_INPUT_CROSSPOINT(pAuto->channelSpec))
			{
				if (NTV2_IS_VALID_CHANNEL(pAutoCirc->ancInputChannel[ACChannel]))
				{
					EnableAncInserter(pSysCon, pAutoCirc->ancInputChannel[ACChannel], false);
					EnableAncExtractor(pSysCon, pAutoCirc->ancInputChannel[ACChannel], false);
					SetupAncExtractor(pSysCon, pAutoCirc->ancInputChannel[ACChannel]);
					EnableAncExtractor(pSysCon, pAutoCirc->ancInputChannel[ACChannel], true);
				}
			}
			else
			{
				EnableAncExtractor(pSysCon, ACChannel, false);
				EnableAncInserter(pSysCon, ACChannel, false);
				SetupAncInserter(pSysCon, ACChannel);
				EnableAncInserter(pSysCon, ACChannel, true);
				if (ntv2ReadVirtualRegister(pSysCon, kVRegEveryFrameTaskFilter) == NTV2_STANDARD_TASKS)
				{
					//For retail mode we will setup all the anc inserters to read from the same location
					for (uint32_t i = 0; i < NTV2DeviceGetNumVideoOutputs(deviceID); i++)
					{
						EnableAncExtractor(pSysCon, (NTV2Channel)i, false);
						EnableAncInserter(pSysCon, (NTV2Channel)i, false);
						SetupAncInserter(pSysCon, (NTV2Channel)i);
						EnableAncInserter(pSysCon, (NTV2Channel)i, true);
					}
				}
				
			}

			ntv2Message("CNTV2Device::AutoCirculateInit - Auto %s: circulateWithANC\n",
						CrosspointName[pAuto->channelSpec]);
		}
		else if (NTV2DeviceCanDoCustomAnc(deviceID))
		{
			if (NTV2_IS_INPUT_CROSSPOINT(pAuto->channelSpec))
			{
				if (NTV2_IS_VALID_CHANNEL(pAutoCirc->ancInputChannel[ACChannel]))
				{
					EnableAncInserter(pSysCon, pAutoCirc->ancInputChannel[ACChannel], false);
					EnableAncExtractor(pSysCon, pAutoCirc->ancInputChannel[ACChannel], false);
				}
			}
			else
			{
				EnableAncExtractor(pSysCon, ACChannel, false);
				EnableAncInserter(pSysCon, ACChannel, false);
				if (ntv2ReadVirtualRegister(pSysCon, kVRegEveryFrameTaskFilter) == NTV2_STANDARD_TASKS)
				{
					//For retail mode we will setup all the anc inserters to read from the same location
					for (uint32_t i = 0; i < NTV2DeviceGetNumVideoOutputs(deviceID); i++)
					{
						EnableAncExtractor(pSysCon, (NTV2Channel)i, false);
						EnableAncInserter(pSysCon, (NTV2Channel)i, false);
					}
				}

			}
		}

		if (pAuto->circulateWithRP188 || pAuto->circulateWithLTC)
		{
			SetRP188Mode(pSysCon, ACChannel, NTV2_RP188_OUTPUT);
			if ((GetQuadFrameEnable(pSysCon, NTV2_CHANNEL1) || GetQuadQuadFrameEnable(pSysCon, NTV2_CHANNEL1)) && (pAuto->channelSpec == NTV2CROSSPOINT_CHANNEL1 || pAuto->channelSpec == NTV2CROSSPOINT_INPUT1))
			{
				SetRP188Mode(pSysCon, NTV2_CHANNEL2, NTV2_RP188_OUTPUT);
				SetRP188Mode(pSysCon, NTV2_CHANNEL3, NTV2_RP188_OUTPUT);
				SetRP188Mode(pSysCon, NTV2_CHANNEL4, NTV2_RP188_OUTPUT);
			}
			if (GetQuadFrameEnable(pSysCon, NTV2_CHANNEL5) && (pAuto->channelSpec == NTV2CROSSPOINT_CHANNEL5 || pAuto->channelSpec == NTV2CROSSPOINT_INPUT5))
			{
				SetRP188Mode(pSysCon, NTV2_CHANNEL6, NTV2_RP188_OUTPUT);
				SetRP188Mode(pSysCon, NTV2_CHANNEL7, NTV2_RP188_OUTPUT);
				SetRP188Mode(pSysCon, NTV2_CHANNEL8, NTV2_RP188_OUTPUT);
			}
			if (ntv2ReadVirtualRegister(pSysCon, kVRegEveryFrameTaskFilter) == NTV2_STANDARD_TASKS)
			{
				//For retail mode we will setup all the anc inserters to read from the same location
				for (uint32_t i = 0; i < NTV2DeviceGetNumVideoOutputs(deviceID); i++)
				{
					SetRP188Mode(pSysCon, (NTV2Channel)i, NTV2_RP188_OUTPUT);
				}
			}
			ntv2Message("CNTV2Device::AutoCirculateInit - Auto %s: circulateWithRP188\n",
						CrosspointName[pAuto->channelSpec]);
		}
   
		// Mod to allow GetBufferLevel to work after an Init
		//pAuto->activeFrame = pAuto->startFrame;
		// This fix was causing other problems relating to autocirculate start-up.
		// activeFrame needs to be '-1' during INIT state!
		// So, we fixed GetBufferLevel() so that it works with activeFrame=-1.

		pAuto->activeFrame = NTV2_INVALID_FRAME;
		pAuto->state = NTV2_AUTOCIRCULATE_INIT;
		ntv2WriteVirtualRegister(pSysCon, kVRegChannelCrosspointFirst + ACChannel, channelSpecAtIndex);
	}

	//Mac spinlockrelease
    
	return NTV2_STATUS_SUCCESS;
}

//-------------------------------------------------------------------------------------------------------
//	AutoCirculateStart
//-------------------------------------------------------------------------------------------------------
Ntv2Status AutoCirculateStart(NTV2AutoCirc* pAutoCirc, NTV2Crosspoint channelSpec, int64_t startTime)
{
	if (ILLEGAL_CHANNELSPEC(channelSpec))
        return NTV2_STATUS_BAD_PARAMETER;
    
	NTV2DeviceID deviceID = pAutoCirc->deviceID;
	Ntv2SystemContext* pSysCon = pAutoCirc->pSysCon;

    // Use the primay channelSpec to get params in the event we are ganging channels
	INTERNAL_AUTOCIRCULATE_STRUCT* pAutoPrimary = &pAutoCirc->autoCirculate[channelSpec];

    if (pAutoPrimary->state != NTV2_AUTOCIRCULATE_INIT)
        return NTV2_STATUS_BAD_PARAMETER;

	//Mac SpinlockAcquire
	uint32_t csIndex = GetIndexForNTV2Crosspoint(channelSpec);

	for(int32_t loopCount = 0; loopCount < pAutoPrimary->channelCount; loopCount++)
	{
		NTV2Crosspoint channelSpecAtIndex = GetNTV2CrosspointForIndex(csIndex);
		csIndex++;

		INTERNAL_AUTOCIRCULATE_STRUCT* pAuto = &pAutoCirc->autoCirculate[channelSpecAtIndex];

		// Setup register so next frame interrupt will clock in frame values.
		int32_t startFrame = pAuto->startFrame;
		INTERNAL_FRAME_STAMP_STRUCT* pInternalFrameStamp = &pAuto->frameStamp[startFrame];

		// set register update mode for autocirculate
		NTV2Channel channel = NTV2_CHANNEL1;
		if(IsMultiFormatActive(pSysCon))
		{
			channel = GetNTV2ChannelForNTV2Crosspoint(pAuto->channelSpec);
		}
		if (IsProgressiveStandard(pSysCon, channel) || pAuto->circulateWithFields)
		{
			SetRegisterWritemode(pSysCon, NTV2_REGWRITE_SYNCTOFIELD, channel);
		}
		else
		{
			SetRegisterWritemode(pSysCon, NTV2_REGWRITE_SYNCTOFRAME, channel);
		}

		NTV2Channel ACChannel = GetNTV2ChannelForNTV2Crosspoint(pAuto->channelSpec);
		NTV2Crosspoint pautoChannelSpec = pAuto->channelSpec;

		if (NTV2_IS_INPUT_CROSSPOINT(pautoChannelSpec))
		{
			SetInputFrame(pSysCon, ACChannel, pAuto->startFrame);
			//InitRP188(pAuto);
			if (pAuto->circulateWithRP188 || pAuto->circulateWithLTC)
			{
				CopyRP188HardwareToFrameStampTCArray(pSysCon, &pInternalFrameStamp->internalTCArray);
			}

			if (NTV2DeviceCanDoSDIErrorChecks(deviceID))
			{
				CopySDIStatusHardwareToFrameStampSDIStatusArray(pSysCon, &pInternalFrameStamp->internalSDIStatusArray);
			}

			if (pAuto->circulateWithCustomAncData)
			{
				if (NTV2_IS_VALID_CHANNEL(pAutoCirc->ancInputChannel[ACChannel]))
				{
					SetAncExtWriteParams(pSysCon, pAutoCirc->ancInputChannel[ACChannel], pAuto->startFrame);
				}
			}
		}
		else // NTV2_IS_INPUT_CROSSPOINT(pautoChannelSpec)
		{
			SetOutputFrame(pSysCon, ACChannel, pAuto->startFrame);
			//InitRP188(pAuto);
			if (pAuto->circulateWithRP188 || pAuto->circulateWithLTC)
			{
				CopyFrameStampTCArrayToHardware(pSysCon, &pInternalFrameStamp->internalTCArray);
			}

			if (pAuto->circulateWithCustomAncData)
			{
				SetAncInsReadParams(pSysCon, ACChannel, pAuto->startFrame, 
									pInternalFrameStamp->ancTransferSize);
			}
			if (pAuto->circulateWithHDMIAux)
			{
				AutoCirculateWriteHDMIAux(pAutoCirc, pInternalFrameStamp->auxData, pInternalFrameStamp->auxDataSize);
			}
			if (pAuto->circulateWithColorCorrection)
			{
				AutoCirculateSetupColorCorrector(pAutoCirc, pautoChannelSpec, &pAuto->frameStamp[pAuto->startFrame].colorCorrectionInfo);
			}
			if (pAuto->circulateWithVidProc)
			{
				AutoCirculateSetupVidProc(pAutoCirc, pautoChannelSpec, &pAuto->frameStamp[pAuto->startFrame].vidProcInfo);
			}
			if (pInternalFrameStamp->xena2RoutingTable.numEntries > 0)
			{
				AutoCirculateSetupXena2Routing(pAutoCirc, &pInternalFrameStamp->xena2RoutingTable);
			}
		}

		pAuto->activeFrame = pAuto->startFrame;
		pAuto->nextFrame = pAuto->startFrame;
		pAuto->startTime = startTime;
		pAuto->state = NTV2_AUTOCIRCULATE_STARTING;
	    
		if(pAuto->startTime == 0)
		{
			ntv2Message("CNTV2Device:AutoCirculateStart - Auto %s: AutoCirculateStart completed\n",
						CrosspointName[pAuto->channelSpec]);
		}
		else
		{
			int64_t currentTime = ntv2Time100ns();
			ntv2Message("CNTV2Device:AutoCirculateStart - Auto %s: AutoCirculateStartAtTime %lld  current %lld\n",
						CrosspointName[pAuto->channelSpec],
						(long long)startTime, (long long)currentTime);
		}
	}

	//Mac SpinlockRelease
    
	return NTV2_STATUS_SUCCESS;
}

//-------------------------------------------------------------------------------------------------------
//	AutoCirculateStop
//-------------------------------------------------------------------------------------------------------
Ntv2Status AutoCirculateStop(NTV2AutoCirc* pAutoCirc, NTV2Crosspoint channelSpec)
{

    if (ILLEGAL_CHANNELSPEC(channelSpec))
		return NTV2_STATUS_BAD_PARAMETER;

    INTERNAL_AUTOCIRCULATE_STRUCT* pAutoPrimary = &pAutoCirc->autoCirculate[channelSpec]; 
	uint32_t csIndex = GetIndexForNTV2Crosspoint(channelSpec);

	//Mac SpinlockAcquire

	for(int32_t loopCount = 0; loopCount < pAutoPrimary->channelCount; loopCount++)
	{
		NTV2Crosspoint channelSpecAtIndex = GetNTV2CrosspointForIndex(csIndex);
		csIndex++;

		INTERNAL_AUTOCIRCULATE_STRUCT* pAuto = &pAutoCirc->autoCirculate[channelSpecAtIndex];

		if(pAuto->state == NTV2_AUTOCIRCULATE_STARTING ||
		   pAuto->state == NTV2_AUTOCIRCULATE_INIT ||
		   pAuto->state == NTV2_AUTOCIRCULATE_PAUSED ||
		   pAuto->state == NTV2_AUTOCIRCULATE_RUNNING) 
		{
			pAuto->state = NTV2_AUTOCIRCULATE_STOPPING;
		}

		if (pAuto->circulateWithRP188 || pAuto->circulateWithLTC)
		{
			//Nothing to do here
		}
	}

	//Mac SpinlockRelease
    
	ntv2Message("CNTV2Device:AutoCirculateStop - Auto %s: stop completed\n",
				CrosspointName[pAutoPrimary->channelSpec]);

	return NTV2_STATUS_SUCCESS;
}

//-------------------------------------------------------------------------------------------------------
//	AutoCirculateAbort
//-------------------------------------------------------------------------------------------------------
Ntv2Status AutoCirculateAbort(NTV2AutoCirc* pAutoCirc, NTV2Crosspoint channelSpec)
{
    if (ILLEGAL_CHANNELSPEC(channelSpec))
		return NTV2_STATUS_BAD_PARAMETER;

	NTV2DeviceID deviceID = pAutoCirc->deviceID;
	Ntv2SystemContext* pSysCon = pAutoCirc->pSysCon;

    INTERNAL_AUTOCIRCULATE_STRUCT* pAutoPrimary = &pAutoCirc->autoCirculate[channelSpec];
	uint32_t csIndex = GetIndexForNTV2Crosspoint(channelSpec);

	//Mac SpinlockAcquire

	for(int32_t loopCount = 0; loopCount < pAutoPrimary->channelCount; loopCount++)
	{
		NTV2Crosspoint channelSpecAtIndex = GetNTV2CrosspointForIndex(csIndex);
		csIndex++;

		INTERNAL_AUTOCIRCULATE_STRUCT* pAuto = &pAutoCirc->autoCirculate[channelSpecAtIndex];
		NTV2Channel ACChannel = GetNTV2ChannelForNTV2Crosspoint(pAuto->channelSpec);
		if(pAuto->state != NTV2_AUTOCIRCULATE_DISABLED) 
		{
			if (pAuto->recording)
			{
				if(pAuto->circulateWithAudio) 
				{
					StopAudioCapture(pSysCon, pAuto->audioSystem);
				}

				if (pAuto->circulateWithCustomAncData)
				{
					if (NTV2_IS_VALID_CHANNEL(pAutoCirc->ancInputChannel[ACChannel]))
					{
						EnableAncExtractor(pSysCon, pAutoCirc->ancInputChannel[ACChannel], false);
					}
				}
			}
			else
			{
				if(pAuto->circulateWithAudio) 
				{
					StopAudioPlayback(pSysCon, pAuto->audioSystem);
				}
				else
				{
					if (NTV2_IS_OUTPUT_CROSSPOINT(channelSpec) && pAutoCirc->globalAudioPlaybackMode == NTV2_AUDIOPLAYBACK_1STAUTOCIRCULATEFRAME)
					{
						// not using autocirculate for audio but want it to be synced....crazy.
						StopAudioPlayback(pSysCon, pAuto->audioSystem);
					}
				}

				if (pAuto->circulateWithCustomAncData)
				{
					EnableAncInserter(pSysCon, GetNTV2ChannelForNTV2Crosspoint(pAuto->channelSpec), false);
					if (ntv2ReadVirtualRegister(pSysCon, kVRegEveryFrameTaskFilter) == NTV2_STANDARD_TASKS)
					{
						//For retail mode we will setup all the anc inserters to read from the same location
						for (uint32_t i = 0; i < NTV2DeviceGetNumVideoOutputs(deviceID); i++)
						{
							EnableAncInserter(pSysCon, (NTV2Channel)i, false);
						}
					}
				}

				if (pAuto->circulateWithRP188 || pAuto->circulateWithLTC)
				{
					//Nothing to do here.
				}
			 }
			pAuto->state = NTV2_AUTOCIRCULATE_DISABLED;
			ntv2WriteVirtualRegister(pSysCon, kVRegChannelCrosspointFirst + (uint32_t)GetNTV2ChannelForNTV2Crosspoint(channelSpecAtIndex), NTV2CROSSPOINT_INVALID);
		}
	}

	//Mac SpinlockRelease
    
	ntv2Message("CNTV2Device::AutoCirculateAbort - Auto %s: abort completed\n",
				CrosspointName[pAutoPrimary->channelSpec]);

	return NTV2_STATUS_SUCCESS;
}

//-------------------------------------------------------------------------------------------------------
//	AutoCirculatePause - Only works for playback
//-------------------------------------------------------------------------------------------------------
Ntv2Status AutoCirculatePause(NTV2AutoCirc* pAutoCirc, NTV2Crosspoint channelSpec, bool bPlay, bool bClearDF)
{
    if (ILLEGAL_CHANNELSPEC(channelSpec))
        return NTV2_STATUS_BAD_PARAMETER;
    
    INTERNAL_AUTOCIRCULATE_STRUCT* pAutoPrimary = &pAutoCirc->autoCirculate[channelSpec]; 
	uint32_t csIndex = GetIndexForNTV2Crosspoint(channelSpec);

	//Mac SpinlockAcquire

	for(int32_t loopCount = 0; loopCount < pAutoPrimary->channelCount; loopCount++)
	{
		NTV2Crosspoint channelSpecAtIndex = GetNTV2CrosspointForIndex(csIndex);
		csIndex++;
		INTERNAL_AUTOCIRCULATE_STRUCT* pAuto = &pAutoCirc->autoCirculate[channelSpecAtIndex];

		if (!bPlay && (pAuto->state == NTV2_AUTOCIRCULATE_RUNNING)) 
		{
			// Play to pause
			pAuto->state = NTV2_AUTOCIRCULATE_PAUSED;
		} 
		else if(bPlay && (pAuto->state == NTV2_AUTOCIRCULATE_PAUSED)) 
		{
			// Pause to play
			pAuto->state = NTV2_AUTOCIRCULATE_RUNNING;
			if (bClearDF)
				pAuto->droppedFrames = 0;
		}
	}

	//Mac SpinlockRelease
    
	ntv2Message("CNTV2Device::AutoCirculatePause - Auto %s: AutoCirculatePause, bPlay=%d\n", 
				CrosspointName[pAutoPrimary->channelSpec], (int) bPlay);

	return NTV2_STATUS_SUCCESS;
}

//-------------------------------------------------------------------------------------------------------
//	AutoCirculateFlush
//-------------------------------------------------------------------------------------------------------
Ntv2Status AutoCirculateFlush(NTV2AutoCirc* pAutoCirc, NTV2Crosspoint channelSpec, bool bClearDF)
{
    if (ILLEGAL_CHANNELSPEC(channelSpec))
        return NTV2_STATUS_BAD_PARAMETER;
    
	NTV2DeviceID deviceID = pAutoCirc->deviceID;
	Ntv2SystemContext* pSysCon = pAutoCirc->pSysCon;

    INTERNAL_AUTOCIRCULATE_STRUCT* pAutoPrimary = &pAutoCirc->autoCirculate[channelSpec];

	if(pAutoPrimary->state != NTV2_AUTOCIRCULATE_INIT && 
	   pAutoPrimary->state != NTV2_AUTOCIRCULATE_RUNNING && 
	   pAutoPrimary->state != NTV2_AUTOCIRCULATE_PAUSED)
		return NTV2_STATUS_SUCCESS;

	uint32_t csIndex = GetIndexForNTV2Crosspoint(channelSpec);

	//Mac SpinlockAcquire
	for(int32_t loopCount = 0; loopCount < pAutoPrimary->channelCount; loopCount++)
	{
		NTV2Crosspoint channelSpecAtIndex = GetNTV2CrosspointForIndex(csIndex);
		csIndex++;

		INTERNAL_AUTOCIRCULATE_STRUCT* pAuto = &pAutoCirc->autoCirculate[channelSpecAtIndex];

		NTV2AutoCirculateState autoState = pAuto->state;
		pAuto->state = NTV2_AUTOCIRCULATE_PAUSED;
        if (bClearDF)
            pAuto->droppedFrames = 0;

		uint32_t lCurFrame = ntv2ReadRegister(pSysCon, pAuto->activeFrameRegister);	// this is usually 1 frame back! pAuto->activeFrame;
		if ((lCurFrame < (uint32_t)pAuto->startFrame) || (lCurFrame > (uint32_t)pAuto->endFrame))
		{
			lCurFrame = pAuto->startFrame;
		}
		uint32_t lStartFrame = lCurFrame;

		if(pAuto->recording) 
		{
			// Flush recorded frames
			lCurFrame = KAUTO_PREVFRAME(lCurFrame, pAuto);
			while(lCurFrame != lStartFrame && pAuto->frameStamp[lCurFrame].validCount != 0) 
			{
				// Mark every frame as available for record except
				// the current (active) frame
				pAuto->frameStamp[lCurFrame].validCount = 0;
				pAuto->frameStamp[lCurFrame].videoTransferPending = false;
				lCurFrame = KAUTO_PREVFRAME(lCurFrame, pAuto);
			}
		}	
		else 
		{
			ntv2Message("CNTV2Device::AutoCirculateFlush - Auto %s: flush active=%d, first flush=%d\n", 
						CrosspointName[pAuto->channelSpec], lCurFrame, KAUTO_NEXTFRAME(lCurFrame, pAuto));
			// Flush and frames queued for playback (normally
			// occurs in pause mode, but play would work as well
			lCurFrame = KAUTO_NEXTFRAME(lCurFrame, pAuto);
			while(lCurFrame != lStartFrame) 
			{
				// Mark each frame as empty starting with active frame + 1 and 
				// ending (after loop) with active frame which remains valid.  This
				// will be zeroed by the ISR
				pAuto->frameStamp[lCurFrame].validCount = 0;
				pAuto->frameStamp[lCurFrame].videoTransferPending = false;
				pAuto->frameStamp[lCurFrame].audioExpectedAddress = 1;
				lCurFrame = KAUTO_NEXTFRAME(lCurFrame, pAuto);
			}
			pAuto->frameStamp[lStartFrame].audioExpectedAddress = 1;

			// clear all if init state
			if (autoState == NTV2_AUTOCIRCULATE_INIT)
			{
				pAuto->frameStamp[lStartFrame].validCount = 0;
				pAuto->frameStamp[lStartFrame].videoTransferPending = false;
			}

			ntv2Message("CNTV2Device::AutoCirculateFlush - Auto %s: flushEnd active=%d, start flush=%d\n", 
						CrosspointName[pAuto->channelSpec], lCurFrame, lStartFrame);

			if (pAuto->circulateWithAudio)
			{
				StopAudioPlayback(pSysCon, pAuto->audioSystem);
				pAuto->nextAudioOutputAddress = 0;
			}
		}

		pAuto->state = autoState;
	}

	return NTV2_STATUS_SUCCESS;
}

//-------------------------------------------------------------------------------------------------------
//	AutoCirculatePreroll
//-------------------------------------------------------------------------------------------------------
Ntv2Status AutoCirculatePreroll(NTV2AutoCirc* pAutoCirc, NTV2Crosspoint channelSpec, int32_t lPrerollFrames)
{	   
	Ntv2SystemContext* pSysCon = pAutoCirc->pSysCon;

	if (ILLEGAL_CHANNELSPEC(channelSpec))
		return NTV2_STATUS_BAD_PARAMETER;
	   
	INTERNAL_AUTOCIRCULATE_STRUCT* pAutoPrimary = &pAutoCirc->autoCirculate[channelSpec];
	if (pAutoPrimary->state != NTV2_AUTOCIRCULATE_RUNNING &&
		pAutoPrimary->state != NTV2_AUTOCIRCULATE_STARTING &&
		pAutoPrimary->state != NTV2_AUTOCIRCULATE_PAUSED)
		return NTV2_STATUS_SUCCESS;
		
	if (pAutoPrimary->activeFrame < pAutoPrimary->startFrame ||
		pAutoPrimary->activeFrame > pAutoPrimary->endFrame)
		return NTV2_STATUS_BAD_PARAMETER;

	uint32_t csIndex = GetIndexForNTV2Crosspoint(channelSpec);

	//Mac SpinlockAcquire
	for(int32_t loopCount = 0; loopCount < pAutoPrimary->channelCount; loopCount++)
	{
		NTV2Crosspoint channelSpecAtIndex = GetNTV2CrosspointForIndex(csIndex);
		csIndex++;

		INTERNAL_AUTOCIRCULATE_STRUCT* pAuto = &pAutoCirc->autoCirculate[channelSpecAtIndex];
	   
		uint32_t lCurFrame = pAuto->activeFrame;
		   	   
		if (!pAuto->recording) 
		{
			// Always preroll last frame (rev->fwd transitions)
			bool bRet = AutoCirculateFindNextAvailFrame(pAuto);
			if (bRet)
				lCurFrame = pAuto->nextTransferFrame;
			else
			{
				//Mac SpinlockRelease
				lCurFrame = NTV2_INVALID_FRAME;
				return NTV2_STATUS_BAD_PARAMETER;
			}
		
			if(lCurFrame != (uint32_t)pAuto->activeFrame)
				lCurFrame = KAUTO_PREVFRAME(lCurFrame, pAuto);
		
			if(lCurFrame == (uint32_t)pAuto->activeFrame) 
			{
				pAuto->state = NTV2_AUTOCIRCULATE_STARTING;
				if(IsAudioPlaying(pSysCon, pAuto->audioSystem))
				{
					StopAudioPlayback(pSysCon, pAuto->audioSystem);
				}
		
				pAuto->nextAudioOutputAddress = 0;
			}
			// Use signed arithmetic to all reduction of preroll as well as addition
			pAuto->frameStamp[lCurFrame].validCount = 
				(int32_t)pAuto->frameStamp[lCurFrame].validCount + lPrerollFrames;
			ntv2Message("CNTV2Device::AutoCirculatePreroll - Auto %s: preroll %d frame (%d) == %d\n", 
						CrosspointName[pAuto->channelSpec], lCurFrame, lPrerollFrames, pAuto->frameStamp[lCurFrame].validCount);	
		}
	}

	//Mac SpinlockRelease

	return NTV2_STATUS_SUCCESS;   
}

//-------------------------------------------------------------------------------------------------------
//	AutoCirculateSetActiveFrame
//-------------------------------------------------------------------------------------------------------
Ntv2Status AutoCirculateSetActiveFrame(NTV2AutoCirc* pAutoCirc, NTV2Crosspoint channelSpec, int32_t lActiveFrame)
{
	NTV2DeviceID deviceID = pAutoCirc->deviceID;
	Ntv2SystemContext* pSysCon = pAutoCirc->pSysCon;

	if (ILLEGAL_CHANNELSPEC(channelSpec))
		return NTV2_STATUS_BAD_PARAMETER;

	//if (GetKernelModeDebugLevel() > 6)
	//DebugLog("AutoCirculateSetActiveFrame %s activeFrame %d\n", CrosspointName[channelSpec], lActiveFrame);

	// Use the primay channelSpec to get params in the event we are ganging channels
	INTERNAL_AUTOCIRCULATE_STRUCT* pAutoPrimary = &pAutoCirc->autoCirculate[channelSpec];

	if (pAutoPrimary->state != NTV2_AUTOCIRCULATE_RUNNING &&
		pAutoPrimary->state != NTV2_AUTOCIRCULATE_STARTING &&
		pAutoPrimary->state != NTV2_AUTOCIRCULATE_PAUSED)
		return NTV2_STATUS_SUCCESS;

	if (pAutoPrimary->activeFrame < pAutoPrimary->startFrame ||
		pAutoPrimary->activeFrame > pAutoPrimary->endFrame)
		return NTV2_STATUS_BAD_PARAMETER;

	if (lActiveFrame < pAutoPrimary->startFrame || lActiveFrame > pAutoPrimary->endFrame)
		return NTV2_STATUS_BAD_PARAMETER;

	uint32_t channelRange = pAutoPrimary->endFrame - pAutoPrimary->startFrame + 1;
	uint32_t csIndex = GetIndexForNTV2Crosspoint(channelSpec);
	uint32_t activeFrame = lActiveFrame;

	//MAC LockAcquire(mPAutoCirculateLock, "AutoCirculatePreroll");

	for (uint32_t loopCount = 0; loopCount < (uint32_t)pAutoPrimary->channelCount; loopCount++)
	{

		NTV2Crosspoint channelSpecAtIndex = GetNTV2CrosspointForIndex(csIndex);
		csIndex++;

		INTERNAL_AUTOCIRCULATE_STRUCT* pAuto = &pAutoCirc->autoCirculate[channelSpecAtIndex];

		pAuto->activeFrame = activeFrame;
		ntv2WriteRegister(pSysCon, pAuto->activeFrameRegister, activeFrame);
		CopyFrameStampTCArrayToHardware(pSysCon, &pAuto->frameStamp[activeFrame].internalTCArray);

		// compute next channels active frame so they shadow the primary channel, which 
		// is based on the channel stride.
		activeFrame += channelRange;
	}

	//MAC LockRelease(mPAutoCirculateLock);

	return NTV2_STATUS_SUCCESS;
}
//-------------------------------------------------------------------------------------------------------
//	AutoCirculateReset
//-------------------------------------------------------------------------------------------------------
void AutoCirculateReset(NTV2AutoCirc* pAutoCirc, NTV2Crosspoint channelSpec)
{
	Ntv2SystemContext* pSysCon = pAutoCirc->pSysCon;

    if (ILLEGAL_CHANNELSPEC(channelSpec))
        return;
    
    INTERNAL_AUTOCIRCULATE_STRUCT* pAuto = &pAutoCirc->autoCirculate[channelSpec]; 
    
    memset(pAuto, 0, sizeof(INTERNAL_AUTOCIRCULATE_STRUCT));
	NTV2Channel ACChannel = GetNTV2ChannelForNTV2Crosspoint(channelSpec);
    pAuto->channelSpec = channelSpec;
	if (NTV2_IS_INPUT_CROSSPOINT(channelSpec))
	{
		pAuto->recording = true;
		pAuto->activeFrameRegister = gChannelToInputFrameReg[ACChannel];
	}
	else
	{
		pAuto->recording = false;
		pAuto->activeFrameRegister = gChannelToOutputFrameReg[ACChannel];
	}
	
    for (int j=0; j < MAX_FRAMEBUFFERS; j++)
    {
        memset(&(pAuto->frameStamp[j]), 0, sizeof(INTERNAL_FRAME_STAMP_STRUCT));
		memset(&(pAuto->frameStamp[j].internalTCArray), 0xFF, sizeof(INTERNAL_TIMECODE_STRUCT));
    }

	pAuto->state = NTV2_AUTOCIRCULATE_DISABLED;
	ntv2WriteVirtualRegister(pSysCon, kVRegChannelCrosspointFirst + (uint32_t)ACChannel, NTV2CROSSPOINT_INVALID);
}

Ntv2Status AutoCirculateGetStatus(NTV2AutoCirc* pAutoCirc, AUTOCIRCULATE_STATUS* pUserOutBuffer)
{
	Ntv2SystemContext* pSysCon = pAutoCirc->pSysCon;

	//ntv2Message("CNTV2Device::AutoCirculateGetStatus %08x\n", pUserOutBuffer);
	if (ILLEGAL_CHANNELSPEC(pUserOutBuffer->acCrosspoint))
		return NTV2_STATUS_BAD_PARAMETER;

	INTERNAL_AUTOCIRCULATE_STRUCT* pAuto = &pAutoCirc->autoCirculate[pUserOutBuffer->acCrosspoint];

	pUserOutBuffer->acState = pAuto->state;
	pUserOutBuffer->acStartFrame = pAuto->startFrame;
	pUserOutBuffer->acEndFrame = pAuto->endFrame;
	pUserOutBuffer->acActiveFrame = pAuto->activeFrame;
	pUserOutBuffer->acRDTSCStartTime = pAuto->startTimeStamp;
	pUserOutBuffer->acAudioClockStartTime = pAuto->startAudioClockTimeStamp;

	pUserOutBuffer->acRDTSCCurrentTime = ntv2Time100ns();
	pUserOutBuffer->acAudioClockCurrentTime = AutoGetAudioClock(pAutoCirc->pFunCon);
	pUserOutBuffer->acFramesProcessed = pAuto->framesProcessed;
	pUserOutBuffer->acFramesDropped = pAuto->droppedFrames;
	pUserOutBuffer->acBufferLevel = AutoCirculateGetBufferLevel(pAuto);

	pUserOutBuffer->acAudioSystem = pAuto->circulateWithAudio ? pAuto->audioSystem : NTV2_AUDIOSYSTEM_INVALID;

	pUserOutBuffer->acOptionFlags = 0;
	pUserOutBuffer->acOptionFlags = (pAuto->circulateWithRP188 ? AUTOCIRCULATE_WITH_RP188 : 0) |
									(pAuto->circulateWithLTC ? AUTOCIRCULATE_WITH_LTC : 0) |
									(pAuto->enableFbfChange ? AUTOCIRCULATE_WITH_FBFCHANGE : 0) |
									(pAuto->enableFboChange ? AUTOCIRCULATE_WITH_FBOCHANGE : 0) |
									(pAuto->circulateWithColorCorrection ? AUTOCIRCULATE_WITH_COLORCORRECT : 0) |
									(pAuto->circulateWithVidProc ? AUTOCIRCULATE_WITH_VIDPROC : 0) |
									(pAuto->circulateWithCustomAncData ? AUTOCIRCULATE_WITH_ANC : 0) |
									(pAuto->circulateWithFields? AUTOCIRCULATE_WITH_FIELDS : 0);

	return NTV2_STATUS_SUCCESS;
}

Ntv2Status AutoCirculateGetFrameStamp (NTV2AutoCirc* pAutoCirc,
										NTV2Crosspoint channelSpec,
										int32_t ulFrameNum,
									   FRAME_STAMP_STRUCT *pFrameStamp)
{
	Ntv2SystemContext* pSysCon = pAutoCirc->pSysCon;

	if (ILLEGAL_CHANNELSPEC(channelSpec))
		return NTV2_STATUS_BAD_PARAMETER;

    INTERNAL_AUTOCIRCULATE_STRUCT* pAuto = &pAutoCirc->autoCirculate[channelSpec];

	if (pAuto->state != NTV2_AUTOCIRCULATE_RUNNING &&
        pAuto->state != NTV2_AUTOCIRCULATE_STARTING &&
        pAuto->state != NTV2_AUTOCIRCULATE_PAUSED) 
    {
		memset(pFrameStamp, 0, sizeof(FRAME_STAMP_STRUCT));
		pFrameStamp->currentFrame = NTV2_INVALID_FRAME;
		// Always can set these
		pFrameStamp->audioClockCurrentTime = AutoGetAudioClock(pAutoCirc->pFunCon);
		pFrameStamp->currentTime = ntv2Time100ns();
		pFrameStamp->currentLineCount = ntv2ReadRegister(pSysCon, kRegLineCount);
		pFrameStamp->currentFrameTime = pAuto->VBILastRDTSC;

		return NTV2_STATUS_SUCCESS;
	}
    
    if (ulFrameNum < pAuto->startFrame && ulFrameNum > pAuto->endFrame)
        ulFrameNum = NTV2_INVALID_FRAME;

    // The following was commented out for 4.0.6 release, approved by James Brooks 
    if (ulFrameNum != NTV2_INVALID_FRAME /*&& pAuto->frameStamp[ulFrameNum].validCount != 0*/)
	{
		pFrameStamp->frame = ulFrameNum;
        pFrameStamp->frameTime              = pAuto->frameStamp[ulFrameNum].frameTime;
        pFrameStamp->audioClockTimeStamp    = pAuto->frameStamp[ulFrameNum].audioClockTimeStamp;
        pFrameStamp->audioExpectedAddress   = pAuto->frameStamp[ulFrameNum].audioExpectedAddress;
		// For record, use the values captured by the ISR/DPC
        pFrameStamp->audioInStartAddress    = pAuto->frameStamp[ulFrameNum].audioInStartAddress;
        pFrameStamp->audioInStopAddress     = pAuto->frameStamp[ulFrameNum].audioInStopAddress;
		// For play, give the next start in both locations.  The user will
		// decide the amount to send
        pFrameStamp->audioOutStartAddress   = pAuto->frameStamp[ulFrameNum].audioOutStartAddress;
        pFrameStamp->audioOutStopAddress    = pAuto->frameStamp[ulFrameNum].audioOutStopAddress;
	} 
	else 
	{
		// Error
		pFrameStamp->frame = NTV2_INVALID_FRAME;
        pFrameStamp->frameTime              = 0;
        pFrameStamp->audioClockTimeStamp    = 0;
        pFrameStamp->audioExpectedAddress   = 0;
        pFrameStamp->audioInStartAddress    = 0;
        pFrameStamp->audioInStopAddress     = 0;
        pFrameStamp->audioOutStartAddress   = 0;
        pFrameStamp->audioOutStopAddress    = 0;
	}

    int32_t lCurrentFrame = pAuto->activeFrame;
    if (lCurrentFrame < pAuto->startFrame || lCurrentFrame > pAuto->endFrame)
        lCurrentFrame = pAuto->startFrame;

    pFrameStamp->currentFrame = lCurrentFrame;
    pFrameStamp->audioClockCurrentTime = AutoGetAudioClock(pAutoCirc->pFunCon);
    pFrameStamp->currentTime = ntv2Time100ns();

    pFrameStamp->currentRP188 = pAuto->frameStamp[lCurrentFrame].rp188;
    if (pAuto->recording)
		// In record this is correct
		pFrameStamp->currentFrameTime = pAuto->frameStamp[lCurrentFrame].frameTime;
	else
		// In play, this has not been set until frame moves on
		pFrameStamp->currentFrameTime = pAuto->VBILastRDTSC;

    bool bField0 = IsFieldID0(pSysCon, channelSpec);
    if (bField0)
        pFrameStamp->currentFieldCount = 0;
    else
        pFrameStamp->currentFieldCount = 1;

    if (pAuto->recording)
    {
        pFrameStamp->currentAudioExpectedAddress = pAuto->frameStamp[lCurrentFrame].audioInStartAddress;
        pFrameStamp->currentAudioStartAddress    = pAuto->frameStamp[lCurrentFrame].audioInStartAddress;
        pFrameStamp->audioClockCurrentTime       = pAuto->frameStamp[lCurrentFrame].audioClockTimeStamp;
    }
    else
    {
        pFrameStamp->currentAudioExpectedAddress = pAuto->frameStamp[lCurrentFrame].audioExpectedAddress;
        pFrameStamp->currentAudioStartAddress    = pAuto->frameStamp[lCurrentFrame].audioOutStartAddress;
        pFrameStamp->audioClockCurrentTime       = pAuto->frameStamp[lCurrentFrame].audioClockTimeStamp;
    }
    
    pFrameStamp->currentLineCount = ntv2ReadRegister(pSysCon, kRegLineCount);
    pFrameStamp->currentReps  = pAuto->frameStamp[lCurrentFrame].validCount;
    pFrameStamp->currenthUser = (uint32_t)pAuto->frameStamp[lCurrentFrame].hUser;
    
	return NTV2_STATUS_SUCCESS;
}

Ntv2Status AutoCirculateTransfer(NTV2AutoCirc* pAutoCirc,
									AUTOCIRCULATE_TRANSFER* pTransferStruct)
{
	NTV2DeviceID deviceID = pAutoCirc->deviceID;
	Ntv2SystemContext* pSysCon = pAutoCirc->pSysCon;

	NTV2Crosspoint channelSpec = pTransferStruct->acCrosspoint;
	if (ILLEGAL_CHANNELSPEC(channelSpec))
	{
		ntv2Message("AutoCirculateTransfer invalid crosspoint channel %d\n", channelSpec);
		return NTV2_STATUS_BAD_PARAMETER;
	}

	NTV2Channel channel = GetNTV2ChannelForNTV2Crosspoint(channelSpec);

	INTERNAL_AUTOCIRCULATE_STRUCT* pAuto = &pAutoCirc->autoCirculate[channelSpec];
	if (pAuto->recording && pAuto->state != NTV2_AUTOCIRCULATE_RUNNING &&
		pAuto->state != NTV2_AUTOCIRCULATE_STARTING)
	{
		ntv2Message("Auto %s: no transfer frames available (not started)\n",
					CrosspointName[pAuto->channelSpec]);
		pTransferStruct->acTransferStatus.acTransferFrame = NTV2_INVALID_FRAME;
		return NTV2_STATUS_SUCCESS;
	}
	else if (pAuto->recording == false && pAuto->state == NTV2_AUTOCIRCULATE_DISABLED)
	{
		ntv2Message("Auto %s: no transfer frames available (disabled)\n",
					CrosspointName[pAuto->channelSpec]);
		pTransferStruct->acTransferStatus.acTransferFrame = NTV2_INVALID_FRAME;
		return NTV2_STATUS_SUCCESS;
	}

	uint32_t frameNumber;
	if (pTransferStruct->acDesiredFrame == NTV2_INVALID_FRAME)
	{
		// -1 indicates we should automatically choose the next frame.
		// We will increment pAuto->NextFrame, and use that value.
		bool bRet = AutoCirculateFindNextAvailFrame(pAuto);
		if (bRet)
			frameNumber = pAuto->nextTransferFrame;
		else
		{
			ntv2Message("Auto %s: no transfer frames available\n",
						CrosspointName[pAuto->channelSpec]);
			frameNumber = NTV2_INVALID_FRAME;
		}
	}
	else
	{
		frameNumber = pTransferStruct->acDesiredFrame;
	}

	if (frameNumber == NTV2_INVALID_FRAME && pAuto->recording)
	{
		// Must be first transfer
		frameNumber = pAuto->startFrame;
		ntv2Message("CNTV2Device::AutoCirculateTransfer - Auto %s: could not find valid transfer frame, setting to %d\n",
					CrosspointName[pAuto->channelSpec], frameNumber);
	}

	// If doing a partial frame, insure that we have valid settings
	uint32_t ulFrameOffset = pTransferStruct->acInVideoDMAOffset;
	if (ulFrameOffset)
	{
		//NTV2FrameGeometry frameGeometry = GetFrameGeometry();
		if ((ulFrameOffset + pTransferStruct->acVideoBuffer.fByteCount >= GetFrameBufferSize(pSysCon, channel)) &&
			(ntv2ReadVirtualRegister(pSysCon, kVRegAdvancedIndexing) == 0))
		{
			ntv2Message("CNTV2Device::AutoCirculateTransfer - Auto %s: OutofRange vbs(%08X) fo(%08X)\n",
						CrosspointName[pAuto->channelSpec], pTransferStruct->acVideoBuffer.fByteCount, ulFrameOffset);
			pTransferStruct->acTransferStatus.acTransferFrame = NTV2_INVALID_FRAME;
			return NTV2_STATUS_FAIL;
		}
	}

	Ntv2Status status = NTV2_STATUS_SUCCESS;
	if (frameNumber != NTV2_INVALID_FRAME)
	{
		// This information will only be returned to the caller on error.
		// This value is required for the DPC to update the correct slot
		//pTransferStruct->acDesiredFrame = frameNumber;
		//ntv2Message("ACFlags = %08x\n", pTransferStruct->acPeerToPeerFlags);

		// Try to grab a DMA engine
		NTV2DMAEngine eVideoDmaEngine = NTV2_PIO;
		NTV2DMAEngine eAudioDmaEngine = NTV2_PIO;
		NTV2DMAEngine eAncDmaEngine = NTV2_PIO;

		if (((pTransferStruct->acPeerToPeerFlags & AUTOCIRCULATE_P2P_PREPARE) != AUTOCIRCULATE_P2P_PREPARE) &&
			((pTransferStruct->acPeerToPeerFlags & AUTOCIRCULATE_P2P_TARGET) != AUTOCIRCULATE_P2P_TARGET) &&
			((pTransferStruct->acPeerToPeerFlags & AUTOCIRCULATE_P2P_COMPLETE) != AUTOCIRCULATE_P2P_COMPLETE))
		{
			eVideoDmaEngine = NTV2_DMA1;
			eAudioDmaEngine = eVideoDmaEngine;
		}

		//Mac SpinlockAcquire
		INTERNAL_AUTOCIRCULATE_STRUCT* pAutoPrimary = &pAutoCirc->autoCirculate[channelSpec];
		uint32_t csIndex = GetIndexForNTV2Crosspoint(channelSpec);
		uint32_t stride = pAutoPrimary->endFrame - pAutoPrimary->startFrame + 1;
		bool updateValid = false;
		bool transferPending = false;
		bool audioTransferDone = false;
		bool ancTransferDone = false;

		if (((pTransferStruct->acPeerToPeerFlags & AUTOCIRCULATE_P2P_PREPARE) == AUTOCIRCULATE_P2P_PREPARE) ||
			((pTransferStruct->acPeerToPeerFlags & AUTOCIRCULATE_P2P_TARGET) == AUTOCIRCULATE_P2P_TARGET))
		{
			if (pAutoPrimary->channelCount != 1)
			{
				ntv2Message("CNTV2Device::AutoCirculateTransfer - Auto %s: DMA frame %d P2P multiple channels not supported (prepare)\n",
							CrosspointName[pAuto->channelSpec], frameNumber);
				return NTV2_STATUS_BAD_PARAMETER;
			}

			AutoBeginAutoCirculateTransfer(frameNumber, pTransferStruct, pAuto);
			updateValid = true;

			if (pAuto->recording)
			{
				ntv2Message("CNTV2Device::AutoCirculateTransfer - Auto %s: DMA frame %d P2P video dma from host transfer not supported (prepare)\n",
					CrosspointName[pAuto->channelSpec], frameNumber);
				status = NTV2_STATUS_BAD_PARAMETER;
			}
			else if (!AutoBoardCanDoP2P(pAutoCirc->pFunCon))
			{
				ntv2Message("CNTV2Device::AutoCirculateTransfer - Auto %s: DMA frame %d P2P target not supported (prepare)\n",
							CrosspointName[pAuto->channelSpec], frameNumber);

				status = NTV2_STATUS_BAD_PARAMETER;
			}
			else
			{
				AUTOCIRCULATE_P2P_STRUCT dmaData;
				memset(&dmaData, 0, sizeof(AUTOCIRCULATE_P2P_STRUCT));
				dmaData.p2pSize = sizeof(AUTOCIRCULATE_P2P_STRUCT);

				uint32_t ulVideoOffset = frameNumber*GetFrameBufferSize(pSysCon, channel) + ulFrameOffset;
				uint32_t ulVideoSize = GetFrameBufferSize(pSysCon, channel) - ulFrameOffset;

				uint64_t paFrameBuffer = AutoGetFrameAperturePhysicalAddress(pAutoCirc->pFunCon);
				uint32_t ulFrameBufferSize = AutoGetFrameApertureBaseSize(pAutoCirc->pFunCon);

				if ((paFrameBuffer != 0) &&
					((ulVideoOffset + ulVideoSize) <= ulFrameBufferSize))
				{
					// fill in p2p structure
					dmaData.videoBusAddress = paFrameBuffer + ulVideoOffset;
					dmaData.videoBusSize = ulVideoSize;

					// for target transfers (vs prepare) the source will also do a message transfer
					if ((pTransferStruct->acPeerToPeerFlags & AUTOCIRCULATE_P2P_TARGET) == AUTOCIRCULATE_P2P_TARGET)
					{
						dmaData.messageBusAddress = AutoGetMessageAddress(pAutoCirc->pFunCon, channelSpec);
						if (dmaData.messageBusAddress != 0)
						{
							dmaData.messageData = frameNumber;
						}
						else
						{
							ntv2Message("CNTV2Device::AutoCirculateTransfer - Auto %s: DMA frame %d error - message register physical address is 0 (prepare)\n",
										CrosspointName[pAuto->channelSpec], frameNumber);
							status = NTV2_STATUS_BAD_PARAMETER;
						}
					}
				}
				else
				{
					ntv2Message("CNTV2Device::AutoCirculateTransfer - Auto %s: DMA frame %d error - no P2P target memory aperture (prepare)\n",
								CrosspointName[pAuto->channelSpec], frameNumber);
					status = NTV2_STATUS_BAD_PARAMETER;
				}

				if (AutoCirculateP2PCopy(pAutoCirc,
											&dmaData,
											(PAUTOCIRCULATE_P2P_STRUCT)pTransferStruct->acVideoBuffer.fUserSpacePtr,
											false))
				{
					AutoWriteFrameApertureOffset(pAutoCirc->pFunCon, 0);
					transferPending = true;
				}
				else
				{
					ntv2Message("CNTV2Device::AutoCirculateTransfer - Auto %s: DMA frame %d P2P buffer mapping error (prepare)\n",
								CrosspointName[pAuto->channelSpec], frameNumber);
					status = NTV2_STATUS_BAD_PARAMETER;
				}
			}
		}
		else if ((pTransferStruct->acPeerToPeerFlags & AUTOCIRCULATE_P2P_COMPLETE) == AUTOCIRCULATE_P2P_COMPLETE)
		{
			if (pAuto->recording)
			{
				ntv2Message("CNTV2Device::AutoCirculateTransfer - Auto %s: DMA frame %d P2P video dma from host transfer not supported (complete)\n",
							CrosspointName[pAuto->channelSpec], frameNumber);
				status = NTV2_STATUS_BAD_PARAMETER;
			}
			else if (!AutoBoardCanDoP2P(pAutoCirc->pFunCon))
			{
				ntv2Message("CNTV2Device::AutoCirculateTransfer - Auto %s: DMA frame %d P2P target not supported (complete)\n",
							CrosspointName[pAuto->channelSpec], frameNumber);
				status = NTV2_STATUS_BAD_PARAMETER;
			}
			AutoBeginAutoCirculateTransfer(frameNumber, pTransferStruct, pAuto);
		}
		else if ((pTransferStruct->acPeerToPeerFlags & AUTOCIRCULATE_P2P_TRANSFER) == AUTOCIRCULATE_P2P_TRANSFER)
		{
			if (pAutoPrimary->channelCount != 1)
			{
				ntv2Message("CNTV2Device::AutoCirculateTransfer - Auto %s: DMA frame %d P2P multiple channels not supported (transfer)\n",
							CrosspointName[pAuto->channelSpec], frameNumber);
				return NTV2_STATUS_BAD_PARAMETER;
			}

			AutoBeginAutoCirculateTransfer(frameNumber, pTransferStruct, pAuto);
			updateValid = true;

			if (!pAuto->recording)
			{
				ntv2Message("CNTV2Device::AutoCirculateTransfer - Auto %s: DMA frame %d P2P video dma from host transfer not supported (transfer)\n",
							CrosspointName[pAuto->channelSpec], frameNumber);
				status = NTV2_STATUS_BAD_PARAMETER;
			}
			else if (!AutoBoardCanDoP2P(pAutoCirc->pFunCon))
			{
				ntv2Message("CNTV2Device::AutoCirculateTransfer - Auto %s: DMA frame %d P2P transfer not supported (transfer)\n",
							CrosspointName[pAuto->channelSpec], frameNumber);
				status = NTV2_STATUS_BAD_PARAMETER;
			}
			else
			{
				AUTOCIRCULATE_P2P_STRUCT dmaData;
				if (AutoCirculateP2PCopy(pAutoCirc,
											&dmaData,
											(PAUTOCIRCULATE_P2P_STRUCT)pTransferStruct->acVideoBuffer.fUserSpacePtr,
											true))
				{
					AUTO_DMA_PARAMS dmaParams;
					memset(&dmaParams, 0, sizeof(AUTO_DMA_PARAMS));
					dmaParams.toHost = pAuto->recording;
					dmaParams.dmaEngine = eVideoDmaEngine;
					dmaParams.videoChannel = channel;
					dmaParams.videoBusAddress = dmaData.videoBusAddress;
					dmaParams.videoBusSize = dmaData.videoBusSize;
					dmaParams.messageBusAddress = dmaData.messageBusAddress;
					dmaParams.messageData = dmaData.messageData;
					dmaParams.videoFrame = frameNumber;
					dmaParams.vidNumBytes = pTransferStruct->acVideoBuffer.fByteCount;
					dmaParams.frameOffset = ulFrameOffset;
					dmaParams.vidUserPitch = pTransferStruct->acInSegmentedDMAInfo.acSegmentHostPitch;
					dmaParams.vidFramePitch = pTransferStruct->acInSegmentedDMAInfo.acSegmentDevicePitch;
					dmaParams.numSegments = pTransferStruct->acInSegmentedDMAInfo.acNumSegments;

					status = AutoDmaTransfer(pAutoCirc->pFunCon, &dmaParams);
				}
				else
				{
					ntv2Message("CNTV2Device::AutoCirculateTransfer - Auto %s: DMA frame %d P2P buffer mapping error (transfer)\n",
								CrosspointName[pAuto->channelSpec], frameNumber);
					status = NTV2_STATUS_BAD_PARAMETER;
				}
			}
		}
		else
		{
			for (int32_t loopCount = 0; loopCount < pAutoPrimary->channelCount; loopCount++)
			{
				NTV2Crosspoint channelSpecAtIndex = GetNTV2CrosspointForIndex(csIndex);
				csIndex++;

				INTERNAL_AUTOCIRCULATE_STRUCT* pAutoTemp = &pAutoCirc->autoCirculate[channelSpecAtIndex];
				AutoBeginAutoCirculateTransfer((stride * loopCount) + frameNumber, pTransferStruct, pAutoTemp);
			}
			updateValid = true;
			//Mac SpinlockRelease
			bool withAnc = pAuto->circulateWithCustomAncData;
			bool withAudio = pAuto->circulateWithAudio;

			// setup dma
			{
				bool bRet = true;
				// synchronize audio setup with interrupt
				if (withAudio)
					bRet = AutoCirculateDmaAudioSetup(pAuto);
				if (!bRet)
				{
					ntv2Message("CNTV2Device::AutoCirculateTransfer - Auto %s: DMA frame %d  audio dma setup failed\n",
								CrosspointName[pAuto->channelSpec], frameNumber);
					return NTV2_STATUS_FAIL;
				}

				// use only the first xlnx engine for performance
				if (NTV2DeviceHasXilinxDMA(deviceID))
				{
					eVideoDmaEngine = NTV2_DMA1;
				}
				else
				{
					switch (channel)
					{
					default:
					case NTV2_CHANNEL1:
					case NTV2_CHANNEL2:
					case NTV2_CHANNEL7:
					case NTV2_CHANNEL8:
						eVideoDmaEngine = NTV2_DMA1;
						break;
					case NTV2_CHANNEL3:
					case NTV2_CHANNEL4:
					case NTV2_CHANNEL5:
					case NTV2_CHANNEL6:
						eVideoDmaEngine = NTV2_DMA2;
						break;
					}
				}

				if (pAuto->circulateWithFields)
				{
					AUTOCIRCULATE_TRANSFER transfer = *pTransferStruct;

					if (!pAuto->recording)
					{
						// update playback field data
						pAuto->frameStamp[frameNumber].frameFlags = pTransferStruct->acPeerToPeerFlags;
					}

					// transfer the active field
					AutoCirculateTransferFields(pAuto, &transfer, frameNumber, false);

					AUTO_DMA_PARAMS dmaParams;
					memset(&dmaParams, 0, sizeof(AUTO_DMA_PARAMS));
					dmaParams.toHost = pAuto->recording;
					dmaParams.dmaEngine = eVideoDmaEngine;
					dmaParams.videoChannel = channel;
					dmaParams.pVidUserVa = (PVOID)transfer.acVideoBuffer.fUserSpacePtr;
					dmaParams.videoFrame = frameNumber;
					dmaParams.vidNumBytes = transfer.acVideoBuffer.fByteCount;
					dmaParams.frameOffset = transfer.acInVideoDMAOffset;
					dmaParams.vidUserPitch = transfer.acInSegmentedDMAInfo.acSegmentHostPitch;
					dmaParams.vidFramePitch = transfer.acInSegmentedDMAInfo.acSegmentDevicePitch;
					dmaParams.numSegments = transfer.acInSegmentedDMAInfo.acNumSegments;
					dmaParams.pAudUserVa = withAudio ? (PVOID)transfer.acAudioBuffer.fUserSpacePtr : NULL;
					dmaParams.audioSystem = pAuto->audioSystem;
					dmaParams.audNumBytes = withAudio ? pAuto->audioTransferSize : 0;
					dmaParams.audOffset = withAudio ? pAuto->audioTransferOffset : 0;
					dmaParams.pAncF1UserVa = withAnc ? (PVOID)transfer.acANCBuffer.fUserSpacePtr : NULL;
					dmaParams.ancF1Frame = frameNumber;
					dmaParams.ancF1NumBytes = withAnc ? pAuto->ancTransferSize : 0;
					dmaParams.ancF1Offset = withAnc ? pAuto->ancTransferOffset : 0;
					dmaParams.pAncF2UserVa = withAnc ? (PVOID)transfer.acANCField2Buffer.fUserSpacePtr : NULL;
					dmaParams.ancF2Frame = frameNumber;
					dmaParams.ancF2NumBytes = withAnc ? pAuto->ancField2TransferSize : 0;
					dmaParams.ancF2Offset = withAnc ? pAuto->ancField2TransferOffset : 0;

					status = AutoDmaTransfer(pAutoCirc->pFunCon, &dmaParams);
					if (pAuto->recording && (status == NTV2_STATUS_SUCCESS))
					{
						// update capture field data
						pTransferStruct->acPeerToPeerFlags = pAuto->frameStamp[frameNumber].frameFlags;
					}
					else
					{
						// transfer the drop field
						transfer = *pTransferStruct;
						AutoCirculateTransferFields(pAuto, &transfer, frameNumber, true);

						AUTO_DMA_PARAMS dmaParams;
						memset(&dmaParams, 0, sizeof(AUTO_DMA_PARAMS));
						dmaParams.toHost = pAuto->recording;
						dmaParams.dmaEngine = eVideoDmaEngine;
						dmaParams.videoChannel = channel;
						dmaParams.pVidUserVa = (PVOID)transfer.acVideoBuffer.fUserSpacePtr;
						dmaParams.videoFrame = frameNumber;
						dmaParams.vidNumBytes = transfer.acVideoBuffer.fByteCount;
						dmaParams.frameOffset = transfer.acInVideoDMAOffset;
						dmaParams.vidUserPitch = transfer.acInSegmentedDMAInfo.acSegmentHostPitch;
						dmaParams.vidFramePitch = transfer.acInSegmentedDMAInfo.acSegmentDevicePitch;
						dmaParams.numSegments = transfer.acInSegmentedDMAInfo.acNumSegments;

						AutoDmaTransfer(pAutoCirc->pFunCon, &dmaParams);
					}
				}
				else
				{
					AUTO_DMA_PARAMS dmaParams;
					memset(&dmaParams, 0, sizeof(AUTO_DMA_PARAMS));
					dmaParams.toHost = pAuto->recording;
					dmaParams.dmaEngine = eVideoDmaEngine;
					dmaParams.videoChannel = channel;
					dmaParams.pVidUserVa = (PVOID)pTransferStruct->acVideoBuffer.fUserSpacePtr;
					dmaParams.videoFrame = frameNumber;
					dmaParams.vidNumBytes = pTransferStruct->acVideoBuffer.fByteCount;
					dmaParams.frameOffset = pTransferStruct->acInVideoDMAOffset;
					dmaParams.vidUserPitch = pTransferStruct->acInSegmentedDMAInfo.acSegmentHostPitch;
					dmaParams.vidFramePitch = pTransferStruct->acInSegmentedDMAInfo.acSegmentDevicePitch;
					dmaParams.numSegments = pTransferStruct->acInSegmentedDMAInfo.acNumSegments;
					dmaParams.pAudUserVa = withAudio ? (PVOID)pTransferStruct->acAudioBuffer.fUserSpacePtr : NULL;
					dmaParams.audioSystem = pAuto->audioSystem;
					dmaParams.audNumBytes = withAudio ? pAuto->audioTransferSize : 0;
					dmaParams.audOffset = withAudio ? pAuto->audioTransferOffset : 0;
					dmaParams.pAncF1UserVa = withAnc ? (PVOID)pTransferStruct->acANCBuffer.fUserSpacePtr : NULL;
					dmaParams.ancF1Frame = frameNumber;
					dmaParams.ancF1NumBytes = withAnc ? pAuto->ancTransferSize : 0;
					dmaParams.ancF1Offset = withAnc ? pAuto->ancTransferOffset : 0;
					dmaParams.pAncF2UserVa = withAnc ? (PVOID)pTransferStruct->acANCField2Buffer.fUserSpacePtr : NULL;
					dmaParams.ancF2Frame = frameNumber;
					dmaParams.ancF2NumBytes = withAnc ? pAuto->ancField2TransferSize : 0;
					dmaParams.ancF2Offset = withAnc ? pAuto->ancField2TransferOffset : 0;

					status = AutoDmaTransfer(pAutoCirc->pFunCon, &dmaParams);
				}
				if (withAudio)
					audioTransferDone = true;
				if (withAnc)
					ancTransferDone = true;
			}
		}

		if (status != NTV2_STATUS_SUCCESS)
		{
			ntv2Message("CNTV2Device::AutoCirculateTransfer - Auto %s: DMA frame %d  transfer failed  Ntv2Status = %d\n",
						CrosspointName[pAuto->channelSpec], frameNumber, status);
			return status;
		}

		if (pAuto->circulateWithAudio && !audioTransferDone)
		{
			if (eAudioDmaEngine == NTV2_PIO)
			{
				eAudioDmaEngine = NTV2_DMA1;
			}

			// synchronize audio setup with interrupt
			bool bRet = AutoCirculateDmaAudioSetup(pAuto);
			if (!bRet)
			{
				ntv2Message("CNTV2Device::AutoCirculateTransfer - Auto %s: DMA frame %d  audio dma setup failed\n",
							CrosspointName[pAuto->channelSpec], frameNumber);
				return NTV2_STATUS_FAIL;
			}

			// dma only audio
			AUTO_DMA_PARAMS dmaParams;
			memset(&dmaParams, 0, sizeof(AUTO_DMA_PARAMS));
			dmaParams.toHost = pAuto->recording;
			dmaParams.dmaEngine = eAudioDmaEngine;
			dmaParams.videoChannel = channel;
			dmaParams.pAudUserVa = (PVOID)pTransferStruct->acAudioBuffer.fUserSpacePtr;
			dmaParams.audioSystem = pAuto->audioSystem;
			dmaParams.audNumBytes = pAuto->audioTransferSize;
			dmaParams.audOffset = pAuto->audioTransferOffset;

			status = AutoDmaTransfer(pAutoCirc->pFunCon, &dmaParams);
			if (status != NTV2_STATUS_SUCCESS)
			{
				ntv2Message("CNTV2Device::AutoCirculateTransfer - Auto %s: DMA audio transfer failed  Ntv2Status = %d\n",
							CrosspointName[pAuto->channelSpec], status);
				return status;
			}
		}

		if (pAuto->circulateWithCustomAncData && !ancTransferDone)
		{
			if (eAncDmaEngine == NTV2_PIO)
			{
				eAncDmaEngine = NTV2_DMA1;
			}

			// dma only anc
			AUTO_DMA_PARAMS dmaParams;
			memset(&dmaParams, 0, sizeof(AUTO_DMA_PARAMS));
			dmaParams.toHost = pAuto->recording;
			dmaParams.dmaEngine = eAncDmaEngine;
			dmaParams.videoChannel = channel;
			dmaParams.pAncF1UserVa = (PVOID)pTransferStruct->acANCBuffer.fUserSpacePtr;
			dmaParams.ancF1Frame = frameNumber;
			dmaParams.ancF1NumBytes = pAuto->ancTransferSize;
			dmaParams.ancF1Offset = pAuto->ancTransferOffset;
			dmaParams.pAncF2UserVa = (PVOID)pTransferStruct->acANCField2Buffer.fUserSpacePtr;
			dmaParams.ancF2Frame = frameNumber;
			dmaParams.ancF2NumBytes = pAuto->ancField2TransferSize;
			dmaParams.ancF2Offset = pAuto->ancField2TransferOffset;

			status = AutoDmaTransfer(pAutoCirc->pFunCon, &dmaParams);
			if (status != NTV2_STATUS_SUCCESS)
			{
				ntv2Message("CNTV2Device::AutoCirculateTransfer - Auto %s: DMA ANC transfer failed  Ntv2Status = %d\n",
							CrosspointName[pAuto->channelSpec], status);
				return status;
			}
		}

		csIndex = GetIndexForNTV2Crosspoint(channelSpec);
		for (int32_t loopCount = 0; loopCount < pAutoPrimary->channelCount; loopCount++)
		{
			NTV2Crosspoint channelSpecAtIndex = GetNTV2CrosspointForIndex(csIndex);
			csIndex++;

			INTERNAL_AUTOCIRCULATE_STRUCT* pAutoTemp = &pAutoCirc->autoCirculate[channelSpecAtIndex];

			AutoCompleteAutoCirculateTransfer((stride * loopCount) + frameNumber,
											  &(pTransferStruct->acTransferStatus),
											  pAutoTemp, updateValid, transferPending);
		}
	}
	else
	{
		ntv2Message("CNTV2Device::AutoCirculateTransfer - Auto %s: NTV2_INVALID_FRAME error\n",
					CrosspointName[pAuto->channelSpec]);
		pTransferStruct->acTransferStatus.acTransferFrame = NTV2_INVALID_FRAME;
		status = NTV2_STATUS_FAIL;
	}
	return status;
}

Ntv2Status AutoCirclateAudioPlaybackMode(NTV2AutoCirc* pAutoCirc,
											NTV2AudioSystem audioSystem,
											NTV2_GlobalAudioPlaybackMode mode)
{
	Ntv2SystemContext* pSysCon = pAutoCirc->pSysCon;
	
	pAutoCirc->globalAudioPlaybackMode = mode;
	switch (pAutoCirc->globalAudioPlaybackMode)
	{
	case NTV2_AUDIOPLAYBACK_NOW:
		StartAudioPlayback(pSysCon, audioSystem);
		break;
	case NTV2_AUDIOPLAYBACK_NEXTFRAME:
		pAutoCirc->startAudioNextFrame = true;
		break;
	case NTV2_AUDIOPLAYBACK_NORMALAUTOCIRCULATE:
		break;
	case NTV2_AUDIOPLAYBACK_1STAUTOCIRCULATEFRAME:
		break;
	default:
		return NTV2_STATUS_BAD_PARAMETER;
	}
	return NTV2_STATUS_SUCCESS;
}

//-------------------------------------------------------------------------------------------------------
//	AutoCirculateGetBufferLevel
//  recording - how many frames ready to record
//  playback - how many frames buffered up
//-------------------------------------------------------------------------------------------------------
uint32_t AutoCirculateGetBufferLevel (INTERNAL_AUTOCIRCULATE_STRUCT* pAuto)
{
	int32_t   lRange = (pAuto->endFrame - pAuto->startFrame) + 1;
	uint32_t  dwBufferLevel = 0;
	int32_t   i;
  
	int32_t   lCurFrame;
	if (pAuto->state == NTV2_AUTOCIRCULATE_INIT) 
        // activeFrame is -1;  We want to know how many have been pre-loaded, starting at startFrame
	    lCurFrame = pAuto->startFrame;
    else
        // we want to know how many are valid, starting from the active frame.
	    lCurFrame = pAuto->activeFrame;

	if((pAuto->state != NTV2_AUTOCIRCULATE_INIT) && 
        ((pAuto->activeFrame < pAuto->startFrame) || (pAuto->activeFrame > pAuto->endFrame)))
        // Bail out if activeFrame is not valid, and we're past the INIT stage.
        // (In INIT state, activeFrame will be -1, so we can't bail out because of that.
        // We want the BufferLevel to represent the number of pre-loaded frames.)
        return 0;
    
	if(pAuto->recording) 
    {
        // No frames available to record till state = NTV2_AUTOCIRCULATE_RUNNING
        if (pAuto->state != NTV2_AUTOCIRCULATE_RUNNING)
            return 0;

        // If ISR has updated validCount on the active frame (and set new active that we won't
        //  update until next VBI)
        if (pAuto->frameStamp[lCurFrame].validCount)
            dwBufferLevel++;

        // Search backward until a '0' is found which indicates the first oldest frame recorded.
		for(i = 1; i < lRange; i++) 
        {
			// Normalize for non zero starts
			lCurFrame = KAUTO_PREVFRAME(lCurFrame, pAuto);
			if(pAuto->frameStamp[lCurFrame].validCount == 0)	// We're done
            {
				return dwBufferLevel;
			}
			dwBufferLevel++;	// pAuto->frameStamp[lCurFrame].validCount always 0 or 1 for now
        }
	} 
    else 
    {  

		// Search forward for a '0' which indicates the next free buffer
		dwBufferLevel = pAuto->frameStamp[lCurFrame].validCount;

		for(i = 1; i < (pAuto->endFrame - pAuto->startFrame) + 1; i++) 
        {
			lCurFrame = KAUTO_NEXTFRAME(lCurFrame, pAuto);
			if(pAuto->frameStamp[lCurFrame].validCount == 0 ||	// Found empty frame
			   i >= lRange)	// Buffers completely full
            {
				// Found it
				return dwBufferLevel;
			}
			dwBufferLevel += pAuto->frameStamp[lCurFrame].validCount;
		}
    }
    
	return dwBufferLevel;	// None available, or play full
}

//-------------------------------------------------------------------------------------------------------
//	AutoCirculateFindNextAvailFrame
//-------------------------------------------------------------------------------------------------------
// Adjusts pAuto->nextFrame (based on validCount), and returns true (or returns FALSE for failure)
bool AutoCirculateFindNextAvailFrame(INTERNAL_AUTOCIRCULATE_STRUCT* pAuto)
{
	int32_t lRange = (pAuto->endFrame - pAuto->startFrame) + 1;
	int32_t lStart = 0;
	int32_t i;

    int32_t lCurFrame;
    if (pAuto->state == NTV2_AUTOCIRCULATE_INIT)
    {
        // When pre-loading (NTV2_AUTOCIRCULATE_INIT state), start from startFrame. (activeFrame is not valid)
        lCurFrame = pAuto->startFrame-1;    // (will be incremented before use)
		lStart = 0;
    }
    else
    {
        lCurFrame = pAuto->activeFrame;
	    if ((lCurFrame < pAuto->startFrame) || (lCurFrame > pAuto->endFrame))
        {
            // frame out of range
            return false;
	    }
		lStart = 1;
    }

   
	if(pAuto->recording) 
    {
        // No frames available to record unless state = NTV2_AUTOCIRCULATE_RUNNING
        if (pAuto->state != NTV2_AUTOCIRCULATE_RUNNING)
            return false;

		// Search forward for a '1' which indicates frame ready to record.
		for(i = 0; i < lRange; i++) 
        {
			// Normalize for non zero starts
			lCurFrame = KAUTO_NEXTFRAME(lCurFrame, pAuto);
			if(pAuto->frameStamp[lCurFrame].validCount > 0) 
            {
				// Found it
                pAuto->nextTransferFrame = lCurFrame;
				return true;
			}
        }
	} 
    else // playback
    {
        if (pAuto->state == NTV2_AUTOCIRCULATE_DISABLED)
		{
            return false;
        }

		// Search forward for a '0' which indicates next available frame to transfer to.
		for(i = lStart; i < lRange; i++) 
        {
			// Normalize for non zero starts
			lCurFrame = KAUTO_NEXTFRAME(lCurFrame, pAuto);

			if(pAuto->frameStamp[lCurFrame].validCount == 0) 
            {
				// Found it
				pAuto->nextTransferFrame = lCurFrame;
                return true;
			}
		}
    }
    
	return false;	// None available
}

//-------------------------------------------------------------------------------------------------------
//	oemBeginAutoCirculateTransfer
//-------------------------------------------------------------------------------------------------------
void AutoBeginAutoCirculateTransfer(uint32_t frameNumber,
									AUTOCIRCULATE_TRANSFER *pTransferStruct,
									INTERNAL_AUTOCIRCULATE_STRUCT *pAuto)
{
	Ntv2SystemContext* pSysCon = pAuto->pSysCon;

	pAuto->transferFrame = frameNumber;
	pAuto->audioTransferOffset = 0;
	pAuto->audioTransferSize = 0;
	pAuto->ancTransferOffset = 0;
	pAuto->ancField2TransferOffset = 0;
	pAuto->ancTransferSize = 0;
	pAuto->ancField2TransferSize = 0;

	if (pAuto->circulateWithCustomAncData)
	{
		NTV2Channel channel = GetNTV2ChannelForNTV2Crosspoint(pAuto->channelSpec);
		uint32_t frameSize = GetFrameBufferSize(pSysCon, (channel < NTV2_CHANNEL5) ? NTV2_CHANNEL1 : NTV2_CHANNEL5);
		pAuto->ancTransferOffset = frameSize - ntv2ReadVirtualRegister(pSysCon, kVRegAncField1Offset);
		pAuto->ancField2TransferOffset = frameSize - ntv2ReadVirtualRegister(pSysCon, kVRegAncField2Offset);
	}

	//ntv2Message("CNTV2Device::oemBeginAutoCirculateTransfer - Begin FrameNumber %d\n", frameNumber);
	if (pAuto->recording)
	{
		if (pAuto->circulateWithCustomAncData)
		{
			pAuto->ancTransferSize = pTransferStruct->acANCBuffer.fByteCount;
			pAuto->ancField2TransferSize = pTransferStruct->acANCField2Buffer.fByteCount;
		}
	}
	else
	{
		NTV2_RP188*		pInTCArray = (NTV2_RP188*)pTransferStruct->acOutputTimeCodes.fUserSpacePtr;
		uint32_t			byteCount = pTransferStruct->acOutputTimeCodes.fByteCount;

		//	On playout, if the user-space client allocated the acOutputTimeCodes field of the AUTOCIRCULATE_TRANSFER struct,
		//	it's supposed to be large enough to hold up to NTV2_MAX_NUM_TIMECODE_DESTS of NTV2_RP188 structs. Only "valid"
		//	NTV2_RP188 values will be played out (i.e., those whose DBB, Hi and Lo fields are not 0xFFFFFFFF)...
		CopyNTV2TimeCodeArrayToFrameStampTCArray(&pAuto->frameStamp[frameNumber].internalTCArray, pInTCArray, byteCount);

		//	Fill in PLAY data for frame stamp...
		if (pInTCArray && byteCount && NTV2_RP188_IS_VALID(pInTCArray[NTV2_TCINDEX_DEFAULT]))
		{
			RP188_STRUCT_from_NTV2_RP188(pAuto->frameStamp[frameNumber].rp188, pInTCArray[NTV2_TCINDEX_DEFAULT]);
		}
		else if (NTV2_RP188_IS_VALID(pTransferStruct->acRP188))
		{
			RP188_STRUCT_from_NTV2_RP188(pAuto->frameStamp[frameNumber].rp188, pTransferStruct->acRP188);	//	acRP188 field is deprecated
		}
		pAuto->frameStamp[frameNumber].repeatCount = pTransferStruct->acFrameRepeatCount;
		pAuto->frameStamp[frameNumber].hUser = pTransferStruct->acInUserCookie;
		pAuto->frameStamp[frameNumber].frameBufferFormat = pTransferStruct->acFrameBufferFormat;
		pAuto->frameStamp[frameNumber].frameBufferOrientation = pTransferStruct->acFrameBufferOrientation;

		if (pAuto->circulateWithAudio && pTransferStruct->acAudioBuffer.fUserSpacePtr != 0)
			pAuto->audioTransferSize = pTransferStruct->acAudioBuffer.fByteCount;

		if (pAuto->circulateWithCustomAncData)
		{
			if (pTransferStruct->acANCBuffer.fUserSpacePtr != 0)
			{
				pAuto->ancTransferSize = pTransferStruct->acANCBuffer.fByteCount;
			}
			else if (pTransferStruct->acANCBuffer.fUserSpacePtr == 0 || pTransferStruct->acANCBuffer.fByteCount == 0)
			{
				pAuto->ancTransferSize = 0;
			}
			if (pTransferStruct->acANCField2Buffer.fUserSpacePtr != 0)
			{
				pAuto->ancField2TransferSize = pTransferStruct->acANCField2Buffer.fByteCount;
			}
			else if (pTransferStruct->acANCField2Buffer.fUserSpacePtr == 0 || pTransferStruct->acANCField2Buffer.fByteCount == 0)
			{
				pAuto->ancField2TransferSize = 0;
			}
		}
		pAuto->frameStamp[frameNumber].ancTransferSize = pAuto->ancTransferSize;
		pAuto->frameStamp[frameNumber].ancField2TransferSize = pAuto->ancField2TransferSize;

		if (pAuto->circulateWithHDMIAux)
		{
			uint32_t maxCount = NTV2_HDMIAuxMaxFrames * NTV2_HDMIAuxDataSize;
			memset(pAuto->frameStamp[frameNumber].auxData, 0, maxCount);
			if (pTransferStruct->acHDMIAuxData.fUserSpacePtr != 0)
			{
				void* pInAuxDataArray = (void*)(pTransferStruct->acHDMIAuxData.fUserSpacePtr);
				uint32_t byteCount = pTransferStruct->acHDMIAuxData.fByteCount;
					
				if (byteCount > maxCount)
					byteCount = maxCount;
				NTV2_TRY
				{
					memcpy(pAuto->frameStamp[frameNumber].auxData, (const void*)pInAuxDataArray, byteCount);
				}
				NTV2_CATCH
				{
					byteCount = 0;
				}
				pAuto->frameStamp[frameNumber].auxDataSize = byteCount;
			}
		}
	}
}

//-------------------------------------------------------------------------------------------------------
//	oemCompleteAutoCirculateTransfer
//-------------------------------------------------------------------------------------------------------
void AutoCompleteAutoCirculateTransfer(uint32_t frameNumber, 
									   AUTOCIRCULATE_TRANSFER_STATUS *pUserOutBuffer,
									   INTERNAL_AUTOCIRCULATE_STRUCT *pAuto,
									   bool updateValid, bool transferPending)
{
	Ntv2SystemContext* pSysCon = pAuto->pSysCon;

	//ntv2Message("CNTV2Device::oemCompleteAutoCirculateTransfer - Complete FrameNumber %d\n", frameNumber);
	// Settings relevant to the app specified frameNumber
	if (frameNumber >= (uint32_t)pAuto->startFrame && frameNumber <= (uint32_t)pAuto->endFrame)
	{
		pUserOutBuffer->acTransferFrame = frameNumber;
		pUserOutBuffer->acFrameStamp.acAudioClockTimeStamp = pAuto->frameStamp[frameNumber].audioClockTimeStamp;
		pUserOutBuffer->acFrameStamp.acCurrentFieldCount = 0;
		pUserOutBuffer->acFrameStamp.acCurrentLineCount = 0;
		pUserOutBuffer->acFrameStamp.acCurrentUserCookie = pAuto->frameStamp[frameNumber].hUser;

		// If recording, the app specified frame (just dma'd) has all relevant timing data
		if (pAuto->recording)
		{
			if (updateValid)
			{
				pAuto->frameStamp[frameNumber].validCount = 0;
			}
			pUserOutBuffer->acFrameStamp.acFrame = frameNumber;
			pUserOutBuffer->acFrameStamp.acFrameTime = pAuto->frameStamp[frameNumber].frameTime;
			pUserOutBuffer->acFrameStamp.acAudioInStartAddress = pAuto->frameStamp[frameNumber].audioInStartAddress;
			pUserOutBuffer->acFrameStamp.acAudioInStopAddress = pAuto->frameStamp[frameNumber].audioInStopAddress;
			pUserOutBuffer->acFrameStamp.acStartSample = pAuto->frameStamp[frameNumber].audioInStartAddress;
			pUserOutBuffer->acFrameStamp.acCurrentReps = pAuto->frameStamp[frameNumber].validCount;	// For drop detect
			pUserOutBuffer->acAudioTransferSize = pAuto->audioTransferSize;
			pUserOutBuffer->acAudioStartSample = pAuto->audioStartSample;
			NTV2_RP188P_from_RP188_STRUCT(&pUserOutBuffer->acFrameStamp.acRP188, pAuto->frameStamp[frameNumber].rp188);
			CopyFrameStampTCArrayToNTV2TimeCodeArray(&pAuto->frameStamp[frameNumber].internalTCArray,
													 (NTV2_RP188 *)pUserOutBuffer->acFrameStamp.acTimeCodes.fUserSpacePtr,
				pUserOutBuffer->acFrameStamp.acTimeCodes.fByteCount);
// 			CopyFrameStampSDIStatusArrayToNTV2SDIStatusArray(&pAuto->frameStamp[frameNumber].internalSDIStatusArray, reinterpret_cast <NTV2SDIInputStatus *> (pUserOutBuffer->acFrameStamp.acSDIInputStatusArray.fUserSpacePtr),
// 				pUserOutBuffer->acFrameStamp.acSDIInputStatusArray.fByteCount);
			pUserOutBuffer->acAncTransferSize = pAuto->frameStamp[frameNumber].ancTransferSize;
			pUserOutBuffer->acAncField2TransferSize = pAuto->frameStamp[frameNumber].ancField2TransferSize;
		}
		// if playing, the frame just dma'd to card doesn't have timing data
		else
		{
			pAuto->frameStamp[frameNumber].videoTransferPending = transferPending;
			if (updateValid)
			{
				pAuto->frameStamp[frameNumber].validCount = pAuto->frameStamp[frameNumber].repeatCount;
			}
			pUserOutBuffer->acFrameStamp.acCurrentReps = pAuto->frameStamp[frameNumber].repeatCount;
		}
	}
	else
		// ERROR!
		pUserOutBuffer->acTransferFrame = NTV2_INVALID_FRAME;

	// Global settings
	int64_t RDTSC = ntv2Time100ns();
	pUserOutBuffer->acFrameStamp.acCurrentTime = RDTSC;
	pUserOutBuffer->acBufferLevel = AutoCirculateGetBufferLevel(pAuto);
	pUserOutBuffer->acState = pAuto->state;
	pUserOutBuffer->acFramesDropped = pAuto->droppedFrames;
	pUserOutBuffer->acFramesProcessed = pAuto->framesProcessed;
	pUserOutBuffer->acFrameStamp.acCurrentAudioExpectedAddress = pUserOutBuffer->acFrameStamp.acAudioExpectedAddress;

	// Current (active) frame settings
	uint32_t ulCurrentFrame = pAuto->activeFrame;
	pUserOutBuffer->acFrameStamp.acCurrentFrame = ulCurrentFrame;
	if (ulCurrentFrame >= (uint32_t)pAuto->startFrame && ulCurrentFrame <= (uint32_t)pAuto->endFrame)
	{
		pUserOutBuffer->acFrameStamp.acCurrentFrameTime = pAuto->frameStamp[ulCurrentFrame].frameTime;
		pUserOutBuffer->acFrameStamp.acAudioClockCurrentTime = AutoGetAudioClock(pAuto->pFunCon);

		// If playing, fill in timing data from the current (active) frame
		if (!pAuto->recording)
		{
			pUserOutBuffer->acFrameStamp.acFrame = ulCurrentFrame;
			pUserOutBuffer->acFrameStamp.acFrameTime = pAuto->frameStamp[ulCurrentFrame].frameTime;
			pUserOutBuffer->acFrameStamp.acAudioOutStartAddress = pAuto->frameStamp[ulCurrentFrame].audioOutStartAddress;
			pUserOutBuffer->acFrameStamp.acAudioOutStopAddress = pAuto->frameStamp[ulCurrentFrame].audioOutStopAddress;
			pUserOutBuffer->acFrameStamp.acStartSample = pAuto->frameStamp[ulCurrentFrame].audioInStartAddress;
			pUserOutBuffer->acFrameStamp.acAudioExpectedAddress = pAuto->frameStamp[ulCurrentFrame].audioExpectedAddress;
		}
	}
	else
	{

		pUserOutBuffer->acFrameStamp.acCurrentFrameTime = 0;
		pUserOutBuffer->acFrameStamp.acAudioClockCurrentTime = AutoGetAudioClock(pAuto->pFunCon);

		// If playing, fill in timing data from the current (active) frame
		if (!pAuto->recording)
		{
			// It can't be all zeros???????????
			pUserOutBuffer->acFrameStamp.acFrame = ulCurrentFrame;
			pUserOutBuffer->acFrameStamp.acFrameTime = 0;
			pUserOutBuffer->acFrameStamp.acAudioOutStartAddress = 0;
			pUserOutBuffer->acFrameStamp.acAudioOutStopAddress = 0;
			pUserOutBuffer->acFrameStamp.acStartSample = 0;
			pUserOutBuffer->acFrameStamp.acAudioExpectedAddress = 0;
		}
	}
}

void AutoCirculateTransferFields(INTERNAL_AUTOCIRCULATE_STRUCT* pAuto, 
									AUTOCIRCULATE_TRANSFER* pTransfer, 
									uint32_t frameNumber, bool drop)
{
	Ntv2SystemContext* pSysCon = pAuto->pSysCon;

	if ((pTransfer->acInVideoDMAOffset != 0) || 
		(pTransfer->acInSegmentedDMAInfo.acNumSegments > 1))
		return;

	if (!AutoCirculateCanDoFieldMode(pAuto))
		return;

	// get the format channel
	NTV2Channel syncChannel = NTV2_CHANNEL1;
	if(IsMultiFormatActive(pSysCon))
	{
		syncChannel = GetNTV2ChannelForNTV2Crosspoint(pAuto->channelSpec);
	}

	// which field to transfer
	bool syncField0 = (pAuto->frameStamp[frameNumber].frameFlags & AUTOCIRCULATE_FRAME_FIELD0) != 0;
	if (drop)
		syncField0 = !syncField0;

	bool top = syncField0;
	uint32_t pixels = 0;
	uint32_t lines = 0;
	uint32_t pitch = 0;

	// get pixels and lines
	NTV2FrameGeometry fbGeometry = GetFrameGeometry(pSysCon, syncChannel);
	switch (fbGeometry)
	{
	case NTV2_FG_720x486:
		pixels = 720;
		lines = 486;
		top = !syncField0;
		break;
	case NTV2_FG_720x514:
		pixels = 720;
		lines = 514;
		top = !syncField0;
		break;
	case NTV2_FG_720x576:
		pixels = 720;
		lines = 576;
		break;
	case NTV2_FG_720x612:
		pixels = 720;
		lines = 612;
		break;
	case NTV2_FG_1920x1080:
		pixels = 1920;
		lines = 1080;
		break;
	case NTV2_FG_1920x1112:
		pixels = 1920;
		lines = 1112;
		break;
	case NTV2_FG_1920x1114:
		pixels = 1920;
		lines = 1114;
		break;
	default:
		return;
	}

	// get line pitch
	NTV2FrameBufferFormat fbFormat = GetFrameBufferFormat(pSysCon, syncChannel);
	switch (fbFormat)
	{
	case NTV2_FBF_10BIT_YCBCR:
	case NTV2_FBF_10BIT_YCBCR_DPX:
		pitch = pixels * 16 / 6;
		break;
	case NTV2_FBF_8BIT_YCBCR:
	case NTV2_FBF_8BIT_YCBCR_YUY2:
		pitch = pixels * 2;
		break;
	case NTV2_FBF_ARGB:
	case NTV2_FBF_RGBA:
	case NTV2_FBF_10BIT_RGB:
	case NTV2_FBF_ABGR:
	case NTV2_FBF_10BIT_DPX:
	case NTV2_FBF_10BIT_YCBCRA:
	case NTV2_FBF_10BIT_DPX_LE:
		pitch = pixels * 4;
		break;
	case NTV2_FBF_24BIT_RGB:
	case NTV2_FBF_24BIT_BGR:
		pitch = pixels * 3 / 2;
		break;
	case NTV2_FBF_48BIT_RGB:
		pitch = pixels * 6;
		break;
	default:
		return;
	}
	
	// fill in segment transfer data
	pTransfer->acInVideoDMAOffset = top? 0 : pitch;
	pTransfer->acVideoBuffer.fByteCount = pitch;
	pTransfer->acInSegmentedDMAInfo.acNumActiveBytesPerRow = pitch;
	pTransfer->acInSegmentedDMAInfo.acSegmentDevicePitch = pitch * 2;
	pTransfer->acInSegmentedDMAInfo.acSegmentHostPitch = pitch;
	pTransfer->acInSegmentedDMAInfo.acNumSegments = lines / 2;
}

bool AutoCirculate (NTV2AutoCirc* pAutoCirc, NTV2Crosspoint channelSpec, int32_t isrTimeStamp)
{
	NTV2DeviceID deviceID = pAutoCirc->deviceID;
	Ntv2SystemContext* pSysCon = pAutoCirc->pSysCon;
	uint64_t    audioCounter = 0;
	uint32_t      audioOutLastAddress = 0;
	INTERNAL_AUTOCIRCULATE_STRUCT* pAuto = NULL;
	int32_t       lastActiveFrame = 0;
	INTERNAL_FRAME_STAMP_STRUCT* pActiveFrameStamp = NULL;
	INTERNAL_FRAME_STAMP_STRUCT* pLastFrameStamp = NULL;	
	static bool bDropped = false;
	NTV2FrameBufferFormat currentFBF;
	NTV2VideoFrameBufferOrientation currentFBO;
	uint64_t time = 0;
	bool bValidFrame = false;

#ifdef HDNTV
	return false;
#endif

	if (ILLEGAL_CHANNELSPEC(channelSpec))
		return false;

	pAuto = &pAutoCirc->autoCirculate[channelSpec];

	NTV2Channel acChannel = GetNTV2ChannelForNTV2Crosspoint(pAuto->channelSpec);
	NTV2Crosspoint pautoChannelSpec = pAuto->channelSpec;

	NTV2Channel syncChannel = NTV2_CHANNEL1;
	if(IsMultiFormatActive(pSysCon))
	{
		syncChannel = GetNTV2ChannelForNTV2Crosspoint(pautoChannelSpec);
	}
	bool syncProgressive = IsProgressiveStandard(pSysCon, syncChannel);
	bool fieldMode = false;
	bool syncField0 = true;
	if (!syncProgressive)
	{
		syncField0 = IsFieldID0(pSysCon, pAuto->channelSpec);
		if (pAuto->circulateWithFields)
		{
			fieldMode = AutoCirculateCanDoFieldMode(pAuto);
		}
	}

	bool changeEvent = false;
	if (syncField0 || fieldMode)
	{
		//Legacy start for out of band AC control
		if (pAutoCirc->startAudioNextFrame && NTV2_IS_OUTPUT_CROSSPOINT(pautoChannelSpec))
		{
			UnPauseAudioPlayback(pSysCon, pAuto->audioSystem);
			StartAudioPlayback(pSysCon, pAuto->audioSystem);
			pAutoCirc->startAudioNextFrame = false;
		}

		//Legacy stop for out of band AC control
		if (pAutoCirc->stopAudioNextFrame && NTV2_IS_OUTPUT_CROSSPOINT(pautoChannelSpec))
		{
			StopAudioPlayback(pSysCon, pAuto->audioSystem);
			pAutoCirc->stopAudioNextFrame = false;
		}

		//In band start on interrupt
		if (pAuto->startAudioNextFrame && NTV2_IS_OUTPUT_CROSSPOINT(pautoChannelSpec))
		{
			UnPauseAudioPlayback(pSysCon, pAuto->audioSystem);
			StartAudioPlayback(pSysCon, pAuto->audioSystem);
			pAuto->startAudioNextFrame = false;
		}

		//In band stop on interrupt
		if (pAuto->stopAudioNextFrame && NTV2_IS_OUTPUT_CROSSPOINT(pautoChannelSpec))
		{
			StopAudioPlayback(pSysCon, pAuto->audioSystem);
			pAuto->stopAudioNextFrame = false;
		}

		if(pAuto->state == NTV2_AUTOCIRCULATE_RUNNING ||
			pAuto->state == NTV2_AUTOCIRCULATE_PAUSED  ||
			pAuto->state == NTV2_AUTOCIRCULATE_INIT  ||
			pAuto->state == NTV2_AUTOCIRCULATE_STARTING) 
		{
			// Update the last vertical blank time
			pAuto->VBILastRDTSC = pAuto->VBIRDTSC;
			// Update the current vertical blank time
			pAuto->VBIRDTSC = isrTimeStamp;

			audioCounter = AutoGetAudioClock(pAutoCirc->pFunCon);
		}

		if(pAuto->state == NTV2_AUTOCIRCULATE_RUNNING ||
			pAuto->state == NTV2_AUTOCIRCULATE_PAUSED  ||
			pAuto->state == NTV2_AUTOCIRCULATE_STARTING) 
		{
#if OEM_PARALLEL_PORT_TIMING
			WRITE_PORT_UCHAR((PUCHAR)0x378, 0x00);
#endif
			// Always align
			// Read the audio out time
			pAuto->VBIAudioOut = oemAudioSampleAlign(pSysCon, pAuto->audioSystem, GetAudioLastOut(pSysCon, pAuto->audioSystem));
			audioOutLastAddress = pAuto->VBIAudioOut;
			//ntv2Message("CNTV2Device::AutoCirculate - Auto %s: %08x  %08x\n", CrosspointName[pAuto->channelSpec], audioOutLastAddress, pAuto->nextAudioOutputAddress);

			lastActiveFrame = pAuto->activeFrame;
			pAuto->activeFrame = ntv2ReadRegister(pSysCon, pAuto->activeFrameRegister);
			if(pAuto->activeFrame < pAuto->startFrame ||
				pAuto->activeFrame > pAuto->endFrame)
			{
					pAuto->activeFrame = pAuto->startFrame;
			}
			pActiveFrameStamp = &pAuto->frameStamp[pAuto->activeFrame];
			pAuto->prevInterruptTime = pAuto->lastInterruptTime;
			pAuto->lastInterruptTime = pAuto->VBIRDTSC;
			pAuto->lastAudioClockTimeStamp = audioCounter;
		}

		switch (pAuto->state)
		{
		case NTV2_AUTOCIRCULATE_INIT:
		{
			pAuto->startTimeStamp = pAuto->VBIRDTSC;
		}
		break;
		case NTV2_AUTOCIRCULATE_STARTING_AT_TIME:
		{
		}
		case NTV2_AUTOCIRCULATE_STARTING:
		{
			// if there is a start time set then wait for it
			if (pAuto->startTime > pAuto->VBIRDTSC)
			{
				break;
			}

			// When starting an auto circulate, ignore the currently playing
			// or recording frame.  Start the real play or record on the 
			// next frame
			changeEvent = true;

			// First valid frame
			int32_t nextFrame = KAUTO_NEXTFRAME(pAuto->activeFrame, pAuto);

			if (pAuto->recording)
			{
				ntv2Message("CNTV2Device::AutoCirculate - Auto %s: state NTV2_AUTOCIRCULATE_STARTING\n", CrosspointName[pAuto->channelSpec]);

				// frame not valid (audio, rp188, etc)
				pAuto->frameStamp[pAuto->activeFrame].validCount = 0;	// Do not use frame
				if (pAuto->circulateWithRP188 || pAuto->circulateWithLTC)
				{
					CopyRP188HardwareToFrameStampTCArray(pSysCon, &pAuto->frameStamp[lastActiveFrame].internalTCArray);
				}

				if (NTV2DeviceCanDoSDIErrorChecks(deviceID))
				{
					CopySDIStatusHardwareToFrameStampSDIStatusArray(pSysCon,
																	&pAuto->frameStamp[lastActiveFrame].internalSDIStatusArray);
				}

				// get current audio input buffer address
				uint32_t actualLastIn = GetAudioLastIn(pSysCon, pAuto->audioSystem);
				uint32_t audioLastInAddress = oemAudioSampleAlign(pSysCon,
																pAuto->audioSystem,
																actualLastIn) + GetAudioReadOffset(pSysCon,
																								   pAuto->audioSystem);

				// update frame stamp start info
				pActiveFrameStamp->frameTime = pAuto->VBIRDTSC;	// Record will start record NOW for this frame
				pActiveFrameStamp->audioInStartAddress = audioLastInAddress;
				pActiveFrameStamp->audioClockTimeStamp = audioCounter;
				pActiveFrameStamp->ancTransferSize = 0;
				pActiveFrameStamp->ancField2TransferSize = 0;
				pAuto->startAudioClockTimeStamp = audioCounter;
				pAuto->startTimeStamp = pAuto->VBIRDTSC;			// Start of first frame is NOW
				if(fieldMode)
					pActiveFrameStamp->frameFlags = syncField0? AUTOCIRCULATE_FRAME_FIELD0 : AUTOCIRCULATE_FRAME_FIELD1;
				else 
					pActiveFrameStamp->frameFlags = AUTOCIRCULATE_FRAME_FULL;

				if (pAuto->circulateWithAudio)
				{
					// calculate last audio address for current frame
					uint32_t numSamplesPerFrame = GetAudioSamplesPerFrame(pSysCon, pAuto->audioSystem, pAuto->framesProcessed, fieldMode);
					pActiveFrameStamp->audioInStopAddress = GetAudioTransferInfo(pSysCon, pAuto->audioSystem,
						pActiveFrameStamp->audioInStartAddress - GetAudioReadOffset(pSysCon, pAuto->audioSystem),
						numSamplesPerFrame*GetNumAudioChannels(pSysCon, pAuto->audioSystem) * 4,
						&pActiveFrameStamp->audioPreWrapBytes,
						&pActiveFrameStamp->audioPostWrapBytes);
					pActiveFrameStamp->audioInStopAddress += GetAudioReadOffset(pSysCon, pAuto->audioSystem);
				}

				if (pAuto->circulateWithCustomAncData)
				{
					if (NTV2_IS_VALID_CHANNEL(pAutoCirc->ancInputChannel[acChannel]))
					{
						SetAncExtWriteParams(pSysCon, pAutoCirc->ancInputChannel[acChannel], nextFrame);
					}
				}

				// start capture on field 0
				if (syncField0)
				{
					// write next video frame number to active frame register
					ntv2WriteRegister(pSysCon, pAuto->activeFrameRegister, nextFrame);
					pAuto->nextFrame = nextFrame;

					// off and running
					pAuto->state = NTV2_AUTOCIRCULATE_RUNNING;
				}
			}
			else  // playback
			{
				// mark active frame with current time
				pActiveFrameStamp->frameTime = pAuto->VBIRDTSC;

				// decrement play count
				bValidFrame = false;
				if ((pActiveFrameStamp->validCount > 0) &&
					(!pActiveFrameStamp->videoTransferPending))
				{
					pActiveFrameStamp->validCount--;
					bValidFrame = true;
					ntv2Message("CNTV2Device::AutoCirculate - Auto %s: state NTV2_AUTOCIRCULATE_STARTING\n", CrosspointName[pAuto->channelSpec]);
				}

				// check frame done
				if (pActiveFrameStamp->validCount == 0)
				{
					bool bDropFrame = false;

					// the frame must not be empty
					if ((pAuto->frameStamp[nextFrame].validCount == 0) ||
						(pAuto->frameStamp[nextFrame].videoTransferPending))
					{
						bDropFrame = true;
#ifdef OEM_DROP_FRAME
						ntv2Message("CNTV2Device::AutoCirculate - Auto %s: DropNextFrame processed %d  dropped %d  valid %d  pending %d\n", 
									CrosspointName[pAuto->channelSpec],
									pAuto->framesProcessed,
									pAuto->droppedFrames,
									pAuto->frameStamp[nextFrame].validCount,
									pAuto->frameStamp[nextFrame].videoTransferPending);
#endif
					}

					// if synced channel is dropping frames then drop this one
					if (!bDropFrame && (ntv2ReadVirtualRegister(pSysCon, kVRegSyncChannels) != 0))
					{
						if (AutoDropSyncFrame(pAutoCirc, pAuto->channelSpec))
						{
							bDropFrame = true;
						}
					}

					if (fieldMode)
					{
						// only start when next field is different
						if (syncField0 == ((pAuto->frameStamp[nextFrame].frameFlags & AUTOCIRCULATE_FRAME_FIELD0) != 0))
						{
							bDropFrame = true;
						}
					}

					// it is time to go to the next frame
					if (!bDropFrame)
					{
						// The next frame is ready
						pAuto->frameStamp[nextFrame].frameTime = pAuto->VBIRDTSC;
						pAuto->frameStamp[nextFrame].audioClockTimeStamp = audioCounter;
						pAuto->startAudioClockTimeStamp = audioCounter;
						pAuto->startTimeStamp = pAuto->VBIRDTSC;

						// Advance to the next frame
						ntv2WriteRegister(pSysCon, pAuto->activeFrameRegister, nextFrame);
						pAuto->nextFrame = nextFrame;
						pAuto->framesProcessed++;

						// Frame Buffer Format
						if (pAuto->enableFbfChange)
						{
							currentFBF = GetFrameBufferFormat(pSysCon, acChannel);
							if (currentFBF != pAuto->frameStamp[nextFrame].frameBufferFormat)
							{
								SetFrameBufferFormat(pSysCon, acChannel, pAuto->frameStamp[nextFrame].frameBufferFormat);
							}
						}

						// Frame Buffer Orientation
						if (pAuto->enableFboChange)
						{
							currentFBO = GetFrameBufferOrientation(pSysCon, acChannel);
							if (currentFBO != pAuto->frameStamp[nextFrame].frameBufferOrientation)
							{
								SetFrameBufferOrientation(pSysCon, acChannel, pAuto->frameStamp[nextFrame].frameBufferOrientation);
							}
						}

						// Start audio playback
						if (pAuto->circulateWithAudio)
						{
							if (bValidFrame)
							{
								if (IsAudioPlaybackPaused(pSysCon, pAuto->audioSystem))
								{
									UnPauseAudioPlayback(pSysCon, pAuto->audioSystem);
								}
								StartAudioPlayback(pSysCon, pAuto->audioSystem);
								pAuto->frameStamp[nextFrame].audioOutStartAddress = audioOutLastAddress;
								pActiveFrameStamp->audioOutStopAddress = audioOutLastAddress;
							}
							else
							{
								// this will start audio when no valid frames on start
								pAuto->audioDropsRequired = 2;
							}
						}
						else
						{
							if (NTV2_IS_OUTPUT_CROSSPOINT(pautoChannelSpec) &&
								(pAutoCirc->globalAudioPlaybackMode == NTV2_AUDIOPLAYBACK_1STAUTOCIRCULATEFRAME))
							{
								// not using autocirculate for audio but want it to be synced....crazy.
								StartAudioPlayback(pSysCon, pAuto->audioSystem);
							}
						}

						if (pAuto->circulateWithRP188 || pAuto->circulateWithLTC)
						{
							CopyFrameStampTCArrayToHardware(pSysCon, &pAuto->frameStamp[nextFrame].internalTCArray);
						}

						if (pAuto->circulateWithColorCorrection)
						{
							AutoCirculateSetupColorCorrector(pAutoCirc,
																pAuto->channelSpec,
																&pAuto->frameStamp[nextFrame].colorCorrectionInfo);
						}
						if (pAuto->circulateWithVidProc)
						{
							AutoCirculateSetupVidProc(pAutoCirc,
														 pAuto->channelSpec,
														 &pAuto->frameStamp[nextFrame].vidProcInfo);
						}
						if (pAuto->circulateWithCustomAncData)
						{
							SetAncInsReadParams(pSysCon, acChannel, nextFrame, pAuto->frameStamp[nextFrame].ancTransferSize);
						}
						if (pAuto->circulateWithHDMIAux)
						{
							AutoCirculateWriteHDMIAux(pAutoCirc,
														 pAuto->frameStamp[nextFrame].auxData,
														 pAuto->frameStamp[nextFrame].auxDataSize);
						}
						AutoCirculateSetupXena2Routing(pAutoCirc, &pAuto->frameStamp[nextFrame].xena2RoutingTable);

						pAuto->state = NTV2_AUTOCIRCULATE_RUNNING;
					}
					else
					{
						if (ntv2ReadVirtualRegister(pSysCon, kVRegSyncChannels) != 0)
						{
							pAuto->droppedFrames++;
						}
					}
				}
				else
				{
					//Not a valid frame
				}
			}
		}
		break;

		case NTV2_AUTOCIRCULATE_RUNNING:
		{
			//
			// Change to RDTSC/Audio handling.  We want to know the audio and 
			// time information for the vertical blank at which the record or
			// play started.  Previous code set the info at the end of the
			// audio video capture. 
			changeEvent = true;
			time = 0;

			if (pAuto->recording)
			{
				// RECORD
				pLastFrameStamp = &pAuto->frameStamp[lastActiveFrame];

				// the frame just captured is done
				if (pLastFrameStamp->validCount == 0)
				{
					pLastFrameStamp->validCount = 1;
				}

				// calculate time difference of last two frame interrupts
				int64_t timeDiff = pAuto->lastInterruptTime - pAuto->prevInterruptTime;

				// bail if interrupt period was too short
				int32_t framePeriod = (int32_t)GetFramePeriod(pSysCon, acChannel);
				if (GetSmpte372(pSysCon, acChannel) || fieldMode)
				{
					framePeriod /= 2;
				}
				if (timeDiff < (int64_t)framePeriod / 5)   // < 20% frame period
				{
					ntv2Message("CNTV2Device::AutoCirculate - Auto %s: interrupt too fast, bailing on this one...  duration %lld\n",
								CrosspointName[pAuto->channelSpec], (long long)timeDiff);
					return false;
				}

				uint32_t newStartAddress = pLastFrameStamp->audioInStopAddress;

				if (pAuto->circulateWithAudio)
				{
					// calculate the error in the video interrupt period
					int32_t discontinuityTime = 10000000;
					if (timeDiff > 0 && timeDiff < 10000000)
					{
						discontinuityTime = (int32_t)timeDiff - framePeriod;
					}

					// if video interrupt period error less than 30% of audio sync tolerance then check audio sync
					if (abs(discontinuityTime) < (int32_t)(ntv2ReadVirtualRegister(pSysCon, kVRegAudioSyncTolerance) * 3))
					{
						// get the current audio address
						uint32_t actualLastIn = GetAudioLastIn(pSysCon, pAuto->audioSystem);
						uint32_t startAddress = newStartAddress - GetAudioReadOffset(pSysCon, pAuto->audioSystem);
						// calculate the difference between the expected start address and the actual address
						uint64_t delta = abs(actualLastIn - startAddress);
						if (delta > GetAudioWrapAddress(pSysCon, pAuto->audioSystem) / 2)
						{
							delta = GetAudioWrapAddress(pSysCon, pAuto->audioSystem) - delta;
						}
						// convert the address delta to a time delta
						uint64_t time = delta * 10000 /
							(GetAudioSamplesPerSecond(pSysCon, pAuto->audioSystem)/1000) /
							(GetNumAudioChannels(pSysCon, pAuto->audioSystem) * 4);
#ifdef OEM_AUDIO_TIMING
						if((pAuto->framesProcessed % ((framePeriod < 250000)?200:100)) == 0)
						{
							ntv2Message("Auto %s:  frame %d  drops %d  audio sync %lld\n", 
										CrosspointName[pAuto->channelSpec], pAuto->framesProcessed, pAuto->droppedFrames, time);
						}
#endif
						// if the time difference is larger than the tolerance correct the expected start address
						if (time > (uint64_t)(ntv2ReadVirtualRegister(pSysCon, kVRegAudioSyncTolerance) * 10))
						{
							newStartAddress = oemAudioSampleAlign(
								pAutoCirc->pFunCon,
								pAuto->audioSystem, actualLastIn) + GetAudioReadOffset(pSysCon, pAuto->audioSystem);
							ntv2Message("CNTV2Device::AutoCirculate - Auto %s:  frame %d  correct audio sync start %d  actual %d  time %lld\n",
										CrosspointName[pAuto->channelSpec], pAuto->framesProcessed,
										startAddress, actualLastIn, (long long)time);
						}
					}
				}

				// save timing and audio data
				pActiveFrameStamp->audioInStartAddress = newStartAddress;
				pActiveFrameStamp->audioClockTimeStamp = audioCounter;
				pActiveFrameStamp->frameTime = pAuto->VBIRDTSC;

				// use the correct frame counter to determine number of audio samples per frame
				uint32_t numSamplesPerFrame = 0;
				if (lastActiveFrame != pAuto->activeFrame)
				{
					numSamplesPerFrame = GetAudioSamplesPerFrame(pSysCon, pAuto->audioSystem, pAuto->framesProcessed, fieldMode);
				}
				else
				{
					numSamplesPerFrame = GetAudioSamplesPerFrame(pSysCon, pAuto->audioSystem, pAuto->droppedFrames, fieldMode);
				}

				// calculate the last audio address for the current frame
				pActiveFrameStamp->audioInStopAddress = GetAudioTransferInfo(pSysCon, pAuto->audioSystem,
																			pActiveFrameStamp->audioInStartAddress - GetAudioReadOffset(pSysCon, pAuto->audioSystem),
																			numSamplesPerFrame*GetNumAudioChannels(pSysCon, pAuto->audioSystem) * 4,
																			&pActiveFrameStamp->audioPreWrapBytes,
																			&pActiveFrameStamp->audioPostWrapBytes);
				pActiveFrameStamp->audioInStopAddress += GetAudioReadOffset(pSysCon, pAuto->audioSystem);

				// set the frame flags
				if(fieldMode)
					pActiveFrameStamp->frameFlags = syncField0? AUTOCIRCULATE_FRAME_FIELD0 : AUTOCIRCULATE_FRAME_FIELD1;
				else 
					pActiveFrameStamp->frameFlags = AUTOCIRCULATE_FRAME_FULL;

				bool bDropSync = false;
				if (ntv2ReadVirtualRegister(pSysCon, kVRegSyncChannels) != 0)
				{
					if (pAuto->channelSpec == pAutoCirc->syncChannel1)
					{
						if (pAutoCirc->autoCirculate[pAutoCirc->syncChannel1].droppedFrames <
							pAutoCirc->autoCirculate[pAutoCirc->syncChannel2].droppedFrames)
						{
							bDropSync = true;
						}
					}
					if (pAuto->channelSpec == pAutoCirc->syncChannel2)
					{
						if (pAutoCirc->autoCirculate[pAutoCirc->syncChannel2].droppedFrames <
							pAutoCirc->autoCirculate[pAutoCirc->syncChannel1].droppedFrames)
						{
							bDropSync = true;
						}
					}
				}

				if (fieldMode)
				{
					// do not repeat the same field when dropping
					uint32_t fieldFlags = AUTOCIRCULATE_FRAME_FIELD0 | AUTOCIRCULATE_FRAME_FIELD1;
					int32_t prevFrame = KAUTO_PREVFRAME(pAuto->activeFrame, pAuto);
					if ((pAuto->frameStamp[prevFrame].frameFlags & fieldFlags) ==
						(pActiveFrameStamp->frameFlags & fieldFlags))
					{
						bDropSync = true;
					}
				}

				//ntv2Message("CNTV2Device::AutoCirculate - Auto %s:  frame %d  drops %d  level %d\n", 
				//		 CrosspointName[pAuto->channelSpec], pAuto->framesProcessed, pAuto->droppedFrames, 
				//		 AutoCirculateGetBufferLevel(pAuto));

				// get the next frame to record
				int32_t nextFrame = KAUTO_NEXTFRAME(pAuto->activeFrame, pAuto);
#if 0
				int32_t theFrame = pAuto->activeFrame;
				ntv2Message("\n");
				do
				{
					ntv2Message("frame %d  valid %d  ff %08x\n", 
								theFrame, pAuto->frameStamp[theFrame].validCount,
								pAuto->frameStamp[theFrame].frameFlags);
					theFrame = KAUTO_NEXTFRAME(theFrame, pAuto);
				}
				while(theFrame != pAuto->activeFrame);
#endif
				// update rp188 data for captured frame
				if (pAuto->circulateWithRP188 || pAuto->circulateWithLTC)
				{
					CopyRP188HardwareToFrameStampTCArray(pSysCon, &pAuto->frameStamp[lastActiveFrame].internalTCArray);
				}

				if (NTV2DeviceCanDoSDIErrorChecks(deviceID))
				{
					CopySDIStatusHardwareToFrameStampSDIStatusArray(pSysCon, &pAuto->frameStamp[lastActiveFrame].internalSDIStatusArray);
				}

				if (pAuto->circulateWithCustomAncData)
				{
					if (NTV2_IS_VALID_CHANNEL(pAutoCirc->ancInputChannel[acChannel]))
					{
						pAuto->frameStamp[lastActiveFrame].ancTransferSize = GetAncExtField1Bytes(pSysCon, pAutoCirc->ancInputChannel[acChannel]);
						pAuto->frameStamp[lastActiveFrame].ancField2TransferSize = GetAncExtField2Bytes(pSysCon, pAutoCirc->ancInputChannel[acChannel]);
						SetAncExtWriteParams(pSysCon, pAutoCirc->ancInputChannel[acChannel], nextFrame);
					}
					else
					{
						pAuto->frameStamp[lastActiveFrame].ancTransferSize = 0;
						pAuto->frameStamp[lastActiveFrame].ancField2TransferSize = 0;
					}
				}

				// the frame must be empty
				if (pAuto->frameStamp[nextFrame].validCount == 0 && !bDropSync)
				{
					// advance to next frame for capture
					ntv2WriteRegister(pSysCon, pAuto->activeFrameRegister, nextFrame);
					pAuto->nextFrame = nextFrame;

					// increment frames processed
					pAuto->framesProcessed++;

				}
				else
				{
					// Application not reading frames fast enough.
					// This may be a temporary state during non record.	 Need to keep active status 
					// in this case.  User can see droppedFrames increment to indicate problem.
					pAuto->droppedFrames++;
					// Tell user this is the frame the drop occurred at (returned in
					// frameStamp->currentReps
					pActiveFrameStamp->validCount++;
#ifdef OEM_DROP_FRAME
					ntv2Message("Auto %s: frame %d  dropped frame %d\n", 
								CrosspointName[pAuto->channelSpec], pAuto->framesProcessed, pAuto->droppedFrames);
#endif
				}

				//ntv2Message("CNTV2Device::AutoCirculate - FP=%d,LAF=%d,AF=%d,NF=%d,Start=%x,Stop=%x\n",pAuto->framesProcessed,lastActiveFrame,pAuto->activeFrame,nextFrame,pLastFrameStamp->audioInStartAddress,pLastFrameStamp->audioInStopAddress);
			}
			else // PLAY
			{
				// record stats for 
				if (pActiveFrameStamp->validCount > 0)
				{
					pActiveFrameStamp->validCount--;
					if (pActiveFrameStamp->validCount == 0)
					{
						// Record audio out point the first time we are at 0
						pActiveFrameStamp->audioOutStopAddress = audioOutLastAddress;
					}
					// Mark each frame so timeStamp + (validCount * frametime) is always valid
					pActiveFrameStamp->frameTime = pAuto->VBIRDTSC;
					pActiveFrameStamp->audioClockTimeStamp = audioCounter;
				}

				// calculate time since previous interrupt
				int64_t timeDiff = pAuto->lastInterruptTime - pAuto->prevInterruptTime;

				//ntv2Message("Auto %s:  frame %d  drops %d  level %d\n", 
				//		 CrosspointName[pAuto->channelSpec], pAuto->framesProcessed, pAuto->droppedFrames, 
				//		 AutoCirculateGetBufferLevel(pAuto));

				// check for frame complete
				if (pActiveFrameStamp->validCount == 0)
				{
					bool bDropFrame = false;

					// find the next frame
					int32_t nextFrame = KAUTO_NEXTFRAME(pAuto->activeFrame, pAuto);

					// the frame must be empty
					if ((pAuto->frameStamp[nextFrame].validCount == 0) ||
						(pAuto->frameStamp[nextFrame].videoTransferPending))
					{
						bDropFrame = true;
#ifdef OEM_DROP_FRAME
						ntv2Message("CNTV2Device::AutoCirculate - Auto %s: DropNextFrame processed %d  dropped %d  valid %d  pending %d\n", 
									CrosspointName[pAuto->channelSpec],
									pAuto->framesProcessed,
									pAuto->droppedFrames,
									pAuto->frameStamp[nextFrame].validCount,
									pAuto->frameStamp[nextFrame].videoTransferPending);
#endif
					}

					// if synced channel is dropping frames then drop this one
					if (!bDropFrame && (ntv2ReadVirtualRegister(pSysCon, kVRegSyncChannels) != 0))
					{
						if (AutoDropSyncFrame(pAutoCirc, pAuto->channelSpec))
						{
							bDropFrame = true;
						}
					}

					if (fieldMode)
					{
						// only move on when next field is different
						if (syncField0 == ((pAuto->frameStamp[nextFrame].frameFlags & AUTOCIRCULATE_FRAME_FIELD0) != 0))
						{
							bDropFrame = true;
						}
					}

					if (pAuto->circulateWithAudio)
					{
						// record audio address to play during frame

						if (IsAudioPlaybackStopped(pSysCon, pAuto->audioSystem))
						{
//							ntv2Message("CNTV2Device::AutoCirculate - AC-Running: expected address %d  drops req %d  com %d", 
//								pActiveFrameStamp->audioExpectedAddress, pAuto->audioDropsRequired, pAuto->audioDropsCompleted);
							if ((pActiveFrameStamp->audioExpectedAddress == 0) &&
								(pAuto->audioDropsRequired == pAuto->audioDropsCompleted))
							{
								UnPauseAudioPlayback(pSysCon, pAuto->audioSystem);
								StartAudioPlayback(pSysCon, pAuto->audioSystem);
								//pAuto->startAudioNextFrame = true;
							}
						}
					}
					// it is time to go to the next frame
					if (!bDropFrame)
					{
						// next frame is ready
						ntv2WriteRegister(pSysCon, pAuto->activeFrameRegister, nextFrame);
						pAuto->nextFrame = nextFrame;
						pAuto->framesProcessed++;

						if (pAuto->enableFbfChange)
						{
							currentFBF = GetFrameBufferFormat(pSysCon, acChannel);
							if (currentFBF != pAuto->frameStamp[nextFrame].frameBufferFormat)
							{
								SetFrameBufferFormat(pSysCon, acChannel, pAuto->frameStamp[nextFrame].frameBufferFormat);
							}
						}

						// frame buffer orientation
						if (pAuto->enableFboChange)
						{
							currentFBO = GetFrameBufferOrientation(pSysCon, acChannel);
							if (currentFBO != pAuto->frameStamp[nextFrame].frameBufferOrientation)
							{
								SetFrameBufferOrientation(pSysCon, acChannel, pAuto->frameStamp[nextFrame].frameBufferOrientation);
							}
						}

						if (pAuto->circulateWithAudio)
						{
							// record audio address to play during frame
							pAuto->frameStamp[nextFrame].audioOutStartAddress = audioOutLastAddress;
							if (IsAudioPlaybackPaused(pSysCon, pAuto->audioSystem))
							{
								UnPauseAudioPlayback(pSysCon, pAuto->audioSystem);
							}
							if (IsAudioPlaybackStopped(pSysCon, pAuto->audioSystem))
							{
								if ((pActiveFrameStamp->audioExpectedAddress == 0) &&
									(pAuto->audioDropsRequired >= pAuto->audioDropsCompleted))
								{
									//StartAudioPlayback(pSysCon, pAuto->audioSystem);
									pAuto->startAudioNextFrame = true;
								}
							}
							else
							{
								// calculate the error in the video interrupt period
								int32_t framePeriod = (int32_t)GetFramePeriod(pSysCon, syncChannel);
								if (GetSmpte372(pSysCon, syncChannel) || fieldMode)
								{
									framePeriod /= 2;
								}
								int32_t discontinuityTime = 10000000;
								if (timeDiff > 0 && timeDiff < 10000000)
								{
									discontinuityTime = (int32_t)timeDiff - framePeriod;
								}

								// if video interrupt period error less than 30% of audio sync tolerance then check audio sync
								if (abs(discontinuityTime) < (int32_t)(ntv2ReadVirtualRegister(pSysCon, kVRegAudioSyncTolerance) * 3))   // interrupt discontinuity
								{
									// calculate the difference between the expected start address and the actual address
									uint32_t startAddress = pActiveFrameStamp->audioExpectedAddress;
									uint64_t delta = abs(audioOutLastAddress - startAddress);
									if (delta > GetAudioWrapAddress(pSysCon, pAuto->audioSystem) / 2)
									{
										delta = GetAudioWrapAddress(pSysCon, pAuto->audioSystem) - delta;
									}
									// convert the address delta to a time delta
									uint64_t time = delta * 10000 /
										(GetAudioSamplesPerSecond(pSysCon, pAuto->audioSystem)/1000) /
										(GetNumAudioChannels(pSysCon, pAuto->audioSystem) * 4);
#ifdef OEM_AUDIO_TIMING
									uint32_t frameCount = pAuto->framesProcessed - 1;
									if((frameCount % ((framePeriod < 250000)?200:100)) == 0)
									{
										ntv2Message("CNTV2Device::AutoCirculate - Auto %s:  frame %d  drops %d  audio sync %lld\n", 
													CrosspointName[pAuto->channelSpec], frameCount, pAuto->droppedFrames, time);
									}
#endif
									// if the time difference is larger than the tolerance restart the audio
									if (time > (uint64_t)(ntv2ReadVirtualRegister(pSysCon, kVRegAudioSyncTolerance) * 10))
									{
										//StopAudioPlayback(pSysCon, pAuto->audioSystem);
										pAuto->stopAudioNextFrame = true;
										pAuto->audioDropsRequired++;
#ifdef OEM_AUDIO_TIMING
										ntv2Message("CNTV2Device::AutoCirculate - Auto %s:  frame %d  correct audio sync start %d  actual %d  time %lld\n",
													CrosspointName[pAuto->channelSpec], frameCount, startAddress, audioOutLastAddress, time);
#endif
									}
								}
							}
						}

						if (pAuto->circulateWithRP188 || pAuto->circulateWithLTC)
						{
							CopyFrameStampTCArrayToHardware(pSysCon, &pAuto->frameStamp[nextFrame].internalTCArray);
						}

						if (pAuto->circulateWithColorCorrection)
						{
							AutoCirculateSetupColorCorrector(pAutoCirc,
																pAuto->channelSpec,
																&pAuto->frameStamp[nextFrame].colorCorrectionInfo);
						}
						if (pAuto->circulateWithVidProc)
						{
							AutoCirculateSetupVidProc(pAutoCirc,
														 pAuto->channelSpec,
														 &pAuto->frameStamp[nextFrame].vidProcInfo);
						}
						if (pAuto->circulateWithCustomAncData)
						{
							SetAncInsReadParams(pSysCon,
												acChannel,
												nextFrame,
												pAuto->frameStamp[nextFrame].ancTransferSize);							
						}
						if (pAuto->circulateWithHDMIAux)
						{
							AutoCirculateWriteHDMIAux(pAutoCirc,
														 pAuto->frameStamp[nextFrame].auxData,
														 pAuto->frameStamp[nextFrame].auxDataSize);
						}
						AutoCirculateSetupXena2Routing(pAutoCirc, &pAuto->frameStamp[nextFrame].xena2RoutingTable);
					}
					else
					{
						// Application not supplying frames fast enough.
						// This may be a temporary state during non record.  Need to keep active status 
						// in this case.  User can see droppedFrames increment to indicate problem.
						pAuto->droppedFrames++;
						if (pAuto->circulateWithAudio)
						{
							if (!IsAudioPlaybackStopped(pSysCon, pAuto->audioSystem))
							{
								//StopAudioPlayback(pSysCon, pAuto->audioSystem);
								pAuto->stopAudioNextFrame = true;
								pAuto->audioDropsRequired++;
							}
							else
							{
								if ((pActiveFrameStamp->audioExpectedAddress == 0) &&
									(pAuto->audioDropsRequired == pAuto->audioDropsCompleted))
								{
									pAuto->audioDropsRequired++;
								}
							}
						}
						bDropped = true;

#ifdef OEM_DROP_FRAME
						ntv2Message("CNTV2Device::AutoCirculate - Auto %s: frame %d  dropped frame %d\n", 
									CrosspontName[pAuto->channelSpec], pAuto->framesProcessed, pAuto->droppedFrames);
#endif
					}
				}
				else
				{
					// frame repeat
				}
			}
			break;

		case NTV2_AUTOCIRCULATE_STOPPING:
		{
			ntv2Message("CNTV2Device::AutoCirculate - Auto %s: state NTV2_AUTOCIRCULATE_STOPPING\n", CrosspointName[pAuto->channelSpec]);

			// calculate stop duration
			int64_t stopTime = ntv2Time100ns();
			int64_t stopDuration = stopTime - pAuto->lastInterruptTime;

			ntv2Message("Auto %s: last frame duration %lld\n",
						CrosspointName[pAuto->channelSpec], (long long)stopDuration);

			if (pAuto->recording)
			{
				if (pAuto->circulateWithAudio)
				{
					StopAudioCapture(pSysCon, pAuto->audioSystem);
				}

				if (pAuto->circulateWithCustomAncData)
				{
					if (NTV2_IS_VALID_CHANNEL(pAutoCirc->ancInputChannel[acChannel]))
					{
						EnableAncExtractor(pSysCon, pAutoCirc->ancInputChannel[acChannel], false);
					}
				}
			}
			else
			{
				if (pAuto->circulateWithAudio)
				{
					StopAudioPlayback(pSysCon, pAuto->audioSystem);
				}
				else if (NTV2_IS_OUTPUT_CROSSPOINT(pAuto->channelSpec) &&
					pAutoCirc->globalAudioPlaybackMode == NTV2_AUDIOPLAYBACK_1STAUTOCIRCULATEFRAME)
				{
					// not using autocirculate for audio but want it to be synced....crazy.
					StopAudioPlayback(pSysCon, pAuto->audioSystem);
				}

				if (pAuto->circulateWithCustomAncData)
				{
					EnableAncInserter(pSysCon, acChannel, false);
					if (ntv2ReadVirtualRegister(pSysCon, kVRegEveryFrameTaskFilter) == NTV2_STANDARD_TASKS)
					{
						//For retail mode we will setup all the anc inserters to read from the same location
						for (uint32_t i = 0; i < NTV2DeviceGetNumVideoOutputs(deviceID); i++)
						{
							EnableAncInserter(pSysCon, (NTV2Channel)i, false);
						}
					}
				}

				if (pAuto->circulateWithRP188 || pAuto->circulateWithLTC)
				{
					//Nothing to do here
				}
			}

			pAuto->state = NTV2_AUTOCIRCULATE_DISABLED;
			ntv2WriteVirtualRegister(pSysCon, kVRegChannelCrosspointFirst + (uint32_t)GetNTV2ChannelForNTV2Crosspoint(pAuto->channelSpec), NTV2CROSSPOINT_INVALID);
		}
		break;

		case NTV2_AUTOCIRCULATE_PAUSED:
		{
			//pAuto->activeFrame = lastActiveFrame;
			if (pAuto->circulateWithAudio)
			{
				if (!pAuto->recording && !IsAudioPlaybackPaused(pSysCon, pAuto->audioSystem))
				{
					PauseAudioPlayback(pSysCon, pAuto->audioSystem);
				}
			}
		}
		break;

		case NTV2_AUTOCIRCULATE_DISABLED:
		default:
			// nothing needs to be done.
			break;
		}
#if 0
		if (pAuto->state != NTV2_AUTOCIRCULATE_DISABLED)
		{
			ntv2Message("CNTV2Device::AutoCirculate - Count:ST(%d) LAF(%d) AF(%d) FP(%d) BL(%d)",pAuto->state,lastActiveFrame,pAuto->activeFrame,pAuto->framesProcessed,
						AutoCirculateGetBufferLevel (pAuto));
		}
#endif
		}
	}
	else if ((pAuto->state == NTV2_AUTOCIRCULATE_STARTING ||
			  pAuto->state == NTV2_AUTOCIRCULATE_RUNNING) &&
			 pAuto->circulateWithCustomAncData)
	{
		if (pAuto->recording)
		{
			//Do someting on a field
			SetAncExtField2WriteParams(pSysCon,
									   acChannel,
									   pAuto->nextFrame);
		}
		else
		{
			//Program the inserters field 2 data
			SetAncInsReadField2Params(pSysCon,
									  acChannel,
									  pAuto->nextFrame,
									  pAuto->frameStamp[pAuto->nextFrame].ancField2TransferSize);
		}
	}
	else
	{
		if(pAuto->state == NTV2_AUTOCIRCULATE_RUNNING ||
			pAuto->state == NTV2_AUTOCIRCULATE_PAUSED  ||
			pAuto->state == NTV2_AUTOCIRCULATE_STARTING) 
		{
#if OEM_PARALLEL_PORT_TIMING
			WRITE_PORT_UCHAR((PUCHAR)0x378, 0xff);
#endif
		}
	}

	return changeEvent;
}

bool
oemIsAutoCirculateInterrupt(NTV2AutoCirc* pAutoCirc, NTV2Crosspoint channelSpec)
{
	Ntv2SystemContext* pSysCon = pAutoCirc->pSysCon;
	NTV2Channel channel = NTV2_CHANNEL1;
	if(IsMultiFormatActive(pSysCon))
	{
		channel = GetNTV2ChannelForNTV2Crosspoint(channelSpec);
	}
	if (IsProgressiveStandard(pSysCon, channel))
	{
		return true;
	}
	else {
		// The field start should be set by an IOCTL.  This mode 
		// (the opposite of original driver) makes the kona SD compatible 
		// with the QuickTime/OMF/GEN/JS uncompressed standard as well 
		// as matching other uncompressed boards (VideoPump, RTV, DVS, etc)
		// TESTED;		NTSC, PAL
		// NOT TESTED:	1080i, 1035/6i
		// NOTE:  Change moved up to support frame stamp returns against next field
		//INTERNAL_AUTOCIRCULATE_STRUCT * pAuto = &pAutoCirc->autoCirculate[channelSpec];
		return IsFieldID0 (pSysCon, channelSpec);
	}
}

int32_t KAUTO_NEXTFRAME(int32_t __dwCurFrame_, INTERNAL_AUTOCIRCULATE_STRUCT* __pAuto_)
{ 
	// Get around -1 mod
	if(__dwCurFrame_ < __pAuto_->endFrame) {
		return __dwCurFrame_ + 1;
	}
	return __pAuto_->startFrame;
}

int32_t KAUTO_PREVFRAME(int32_t __dwCurFrame_, INTERNAL_AUTOCIRCULATE_STRUCT* __pAuto_)
{ 
	// Get around -1 mod
	if(__dwCurFrame_ > __pAuto_->startFrame) {
		return __dwCurFrame_ - 1;
	}
	return __pAuto_->endFrame;
}

bool AutoDropSyncFrame(NTV2AutoCirc* pAutoCirc, NTV2Crosspoint channelSpec)
{
	INTERNAL_AUTOCIRCULATE_STRUCT* pAuto = &pAutoCirc->autoCirculate[channelSpec];
	uint32_t syncChannel1 = pAutoCirc->syncChannel1;
	uint32_t syncChannel2 = pAutoCirc->syncChannel2;
	INTERNAL_AUTOCIRCULATE_STRUCT* pAutoChannel1 = &pAutoCirc->autoCirculate[syncChannel1];
	INTERNAL_AUTOCIRCULATE_STRUCT* pAutoChannel2 = &pAutoCirc->autoCirculate[syncChannel2];
	uint32_t fpChannel1 = pAutoChannel1->framesProcessed;
	uint32_t fpChannel2 = pAutoChannel2->framesProcessed;
	uint32_t dfChannel1 = pAutoChannel1->droppedFrames;
	uint32_t dfChannel2 = pAutoChannel2->droppedFrames;
	int32_t nextFrame = 0;

	if((syncChannel1 == NTV2CROSSPOINT_FGKEY) ||
		(syncChannel2 == NTV2CROSSPOINT_FGKEY))
	{
		return false;
	}

	if((uint32_t)pAuto->channelSpec == syncChannel1)
	{
		if(dfChannel1 < dfChannel2)
		{
#ifdef OEM_DROP_FRAME
			ntv2Message("CNTV2Device::DropSyncFrame - Auto %s: DropSyncFrame processed %d  dropped %d(%s) < %d(%s)\n", 
						CrosspointName[pAuto->channelSpec],
						fpChannel1,
						dfChannel1, CrosspointName[syncChannel1],
						dfChannel2, CrosspointName[syncChannel2]);
#endif
			return true;
		}
		if(fpChannel1 > fpChannel2)
		{
#ifdef OEM_DROP_FRAME
			ntv2Message("CNTV2Device::DropSyncFrame - Auto %s: DropSyncFrame processed %d(%s) > %d(%s)  dropped %d\n", 
						CrosspointName[pAuto->channelSpec], 
						fpChannel1, CrosspointName[syncChannel1],
						fpChannel2, CrosspointName[syncChannel2],
						dfChannel1);
#endif
			return true;
		}
		else if(fpChannel1 == fpChannel2)
		{
			nextFrame = KAUTO_NEXTFRAME(pAutoChannel2->nextFrame, pAutoChannel2);
			if(pAutoChannel2->frameStamp[nextFrame].validCount == 0)
			{
#ifdef OEM_DROP_FRAME
				ntv2Message("CNTV2Device::DropSyncFrame - Auto %s: DropSyncFrame processed %d  dropped %d  %s validCount == 0\n", 
							CrosspointName[pAuto->channelSpec], fpChannel1, dfChannel1, CrosspointName[syncChannel2]);
#endif
				return true;
			}
		}
		return false;
	}
	if((uint32_t)pAuto->channelSpec == syncChannel2)
	{
		if(dfChannel2 < dfChannel1)
		{
#ifdef OEM_DROP_FRAME
			ntv2Message("CNTV2Device::DropSyncFrame - Auto %s: DropSyncFrame processed %d  dropped %d(%s) < %d(%s)\n", 
						CrosspointName[pAuto->channelSpec],
						fpChannel2,
						dfChannel2, CrosspointName[syncChannel2],
						dfChannel1, CrosspointName[syncChannel1]);
#endif
			return true;
		}
		if(fpChannel2 > fpChannel1)
		{
#ifdef OEM_DROP_FRAME
			ntv2Message("CNTV2Device::DropSyncFrame - Auto %s: DropSyncFrame processed %d(%s) > %d(%s)  dropped %d\n", 
						CrosspointName[pAuto->channelSpec], 
						fpChannel2, CrosspointName[syncChannel2],
						fpChannel1, CrosspointName[syncChannel1],
						dfChannel2);
#endif
			return true;
		}
		else if(fpChannel2 == fpChannel1)
		{
			nextFrame = KAUTO_NEXTFRAME(pAutoChannel1->nextFrame, pAutoChannel1);
			if(pAutoChannel1->frameStamp[nextFrame].validCount == 0)
			{
#ifdef OEM_DROP_FRAME
				ntv2Message("CNTV2Device::DropSyncFrame - Auto %s: DropSyncFrame processed %d  dropped %d  %s validCount == 0\n", 
							CrosspointName[pAuto->channelSpec], fpChannel2, dfChannel2, CrosspointName[syncChannel1]);
#endif
				return true;
			}
		}
		return false;
	}

	return false;
}

void AutoCirculateSetupColorCorrector(NTV2AutoCirc* pAutoCirc,
										 NTV2Crosspoint channelSpec,
										 INTERNAL_COLOR_CORRECTION_STRUCT *ccInfo)
{
	NTV2DeviceID deviceID = pAutoCirc->deviceID;
	Ntv2SystemContext* pSysCon = pAutoCirc->pSysCon;
	uint32_t regIndex;

	if (!NTV2DeviceCanDoColorCorrection(deviceID))
		return;

	// Find current output bank and make host access bank the other bank.
	switch(channelSpec)
	{
	case NTV2CROSSPOINT_CHANNEL1:
		if (GetColorCorrectionOutputBank(pSysCon, NTV2_CHANNEL1) == 1)
		{
			SetColorCorrectionHostAccessBank(pSysCon, NTV2_CCHOSTACCESS_CH1BANK0);	// happens immediatedly
			SetColorCorrectionOutputBank(pSysCon, NTV2_CHANNEL1, 0);					// happens next frame
		}
		else
		{
			SetColorCorrectionHostAccessBank(pSysCon, NTV2_CCHOSTACCESS_CH1BANK1);	// happens immediatedly
			SetColorCorrectionOutputBank(pSysCon, NTV2_CHANNEL1, 1);					// happens next frame
		}
		SetColorCorrectionSaturation(pSysCon, NTV2_CHANNEL1, ccInfo->saturationValue);
		SetColorCorrectionMode(pSysCon, NTV2_CHANNEL1, ccInfo->mode);
		break;
	case NTV2CROSSPOINT_CHANNEL2:
		if (GetColorCorrectionOutputBank(pSysCon, NTV2_CHANNEL2) == 1)
		{
			SetColorCorrectionHostAccessBank(pSysCon, NTV2_CCHOSTACCESS_CH2BANK0);	// happens immediatedly
			SetColorCorrectionOutputBank(pSysCon, NTV2_CHANNEL2, 0);					// happens next frame
		}
		else
		{
			SetColorCorrectionHostAccessBank(pSysCon, NTV2_CCHOSTACCESS_CH2BANK1);	// happens immediatedly
			SetColorCorrectionOutputBank(pSysCon, NTV2_CHANNEL2, 1);					// happens next frame
		}
		SetColorCorrectionSaturation(pSysCon, NTV2_CHANNEL2, ccInfo->saturationValue);
		SetColorCorrectionMode(pSysCon, NTV2_CHANNEL2, ccInfo->mode);
		break;
	case NTV2CROSSPOINT_CHANNEL3:
		if (GetColorCorrectionOutputBank(pSysCon, NTV2_CHANNEL3) == 1)
		{
			SetColorCorrectionHostAccessBank(pSysCon, NTV2_CCHOSTACCESS_CH3BANK0);	// happens immediatedly
			SetColorCorrectionOutputBank(pSysCon, NTV2_CHANNEL3, 0);					// happens next frame
		}
		else
		{
			SetColorCorrectionHostAccessBank(pSysCon, NTV2_CCHOSTACCESS_CH3BANK1);	// happens immediatedly
			SetColorCorrectionOutputBank(pSysCon, NTV2_CHANNEL3, 1);				// happens next frame
		}
		break;
	case NTV2CROSSPOINT_CHANNEL4:
		if (GetColorCorrectionOutputBank(pSysCon, NTV2_CHANNEL4) == 1)
		{
			SetColorCorrectionHostAccessBank(pSysCon, NTV2_CCHOSTACCESS_CH4BANK0);	// happens immediatedly
			SetColorCorrectionOutputBank(pSysCon, NTV2_CHANNEL4, 0);					// happens next frame
		}
		else
		{
			SetColorCorrectionHostAccessBank(pSysCon, NTV2_CCHOSTACCESS_CH4BANK1);	// happens immediatedly
			SetColorCorrectionOutputBank(pSysCon, NTV2_CHANNEL4, 1);					// happens next frame
		}
		break;
	case NTV2CROSSPOINT_CHANNEL5:
		if (GetColorCorrectionOutputBank(pSysCon, NTV2_CHANNEL5) == 1)
		{
			SetColorCorrectionHostAccessBank(pSysCon, NTV2_CCHOSTACCESS_CH5BANK0);	// happens immediatedly
			SetColorCorrectionOutputBank(pSysCon, NTV2_CHANNEL5, 0);					// happens next frame
		}
		else
		{
			SetColorCorrectionHostAccessBank(pSysCon, NTV2_CCHOSTACCESS_CH5BANK1);	// happens immediatedly
			SetColorCorrectionOutputBank(pSysCon, NTV2_CHANNEL5, 1);					// happens next frame
		}
		break;
	case NTV2CROSSPOINT_CHANNEL6:
		if (GetColorCorrectionOutputBank(pSysCon, NTV2_CHANNEL6) == 1)
		{
			SetColorCorrectionHostAccessBank(pSysCon, NTV2_CCHOSTACCESS_CH6BANK0);	// happens immediatedly
			SetColorCorrectionOutputBank(pSysCon, NTV2_CHANNEL6, 0);					// happens next frame
		}
		else
		{
			SetColorCorrectionHostAccessBank(pSysCon, NTV2_CCHOSTACCESS_CH6BANK1);	// happens immediatedly
			SetColorCorrectionOutputBank(pSysCon, NTV2_CHANNEL6, 1);					// happens next frame
		}
		break;
	case NTV2CROSSPOINT_CHANNEL7:
		if (GetColorCorrectionOutputBank(pSysCon, NTV2_CHANNEL7) == 1)
		{
			SetColorCorrectionHostAccessBank(pSysCon, NTV2_CCHOSTACCESS_CH7BANK0);	// happens immediatedly
			SetColorCorrectionOutputBank(pSysCon, NTV2_CHANNEL7, 0);				// happens next frame
		}
		else
		{
			SetColorCorrectionHostAccessBank(pSysCon, NTV2_CCHOSTACCESS_CH7BANK1);	// happens immediatedly
			SetColorCorrectionOutputBank(pSysCon, NTV2_CHANNEL7, 1);					// happens next frame
		}
		break;
	case NTV2CROSSPOINT_CHANNEL8:
		if (GetColorCorrectionOutputBank(pSysCon, NTV2_CHANNEL8) == 1)
		{
			SetColorCorrectionHostAccessBank(pSysCon, NTV2_CCHOSTACCESS_CH8BANK0);	// happens immediatedly
			SetColorCorrectionOutputBank(pSysCon, NTV2_CHANNEL8, 0);					// happens next frame
		}
		else
		{
			SetColorCorrectionHostAccessBank(pSysCon, NTV2_CCHOSTACCESS_CH8BANK1);	// happens immediatedly
			SetColorCorrectionOutputBank(pSysCon, NTV2_CHANNEL8, 1);				// happens next frame
		}
		break;
	}

	// Now fill color correction buffer
	for (regIndex = 0; regIndex < NTV2_COLORCORRECTOR_TABLESIZE/4; regIndex++)
	{
		ntv2WriteRegister(pSysCon, 512 + regIndex, ccInfo->ccLookupTables[regIndex]);
	}
}

//-------------------------------------------------------------------------------------------------------
//	AutoCirculateSetupVidProc
//  Unfortunately only updates hardware every frame even in interlaced modes.
//-------------------------------------------------------------------------------------------------------
void AutoCirculateSetupVidProc(NTV2AutoCirc* pAutoCirc,
								  NTV2Crosspoint channelSpec,
								  AutoCircVidProcInfo* vidProcInfo)
{
	NTV2DeviceID deviceID = pAutoCirc->deviceID;
	Ntv2SystemContext* pSysCon = pAutoCirc->pSysCon;
	NTV2FrameGeometry frameGeometry;
	uint32_t regValue;
	uint32_t max;
	uint32_t offset;
	uint32_t splitModeValue;
	
	SetForegroundVideoCrosspoint(pSysCon, vidProcInfo->foregroundVideoCrosspoint);
	SetForegroundKeyCrosspoint(pSysCon, vidProcInfo->foregroundKeyCrosspoint);
	SetBackgroundVideoCrosspoint(pSysCon, vidProcInfo->backgroundVideoCrosspoint);
	SetBackgroundKeyCrosspoint(pSysCon, vidProcInfo->backgroundKeyCrosspoint);

	regValue = ntv2ReadRegister(pSysCon, kRegVidProc1Control);
	regValue &= ~(VIDPROCMUX1MASK + VIDPROCMUX2MASK + VIDPROCMUX3MASK);

	max = 0;
	offset = 0;
	frameGeometry = GetFrameGeometry(pSysCon, NTV2_CHANNEL1);
	splitModeValue = 0;
	switch (vidProcInfo->mode)
	{
	case AUTOCIRCVIDPROCMODE_MIX:
		regValue |= (BIT_0+BIT_2);
		ntv2WriteRegister(pSysCon, kRegMixer1Coefficient, vidProcInfo->transitionCoefficient);
		break;
	case AUTOCIRCVIDPROCMODE_HORZWIPE:
		regValue |= (BIT_0+BIT_3);
		switch (frameGeometry)
		{
		case NTV2_FG_1920x1080:
			max = 1920;
			offset = 8;
			break;
		case NTV2_FG_1280x720:
			max = 1280;
			offset = 8;
			break;
		case NTV2_FG_720x486:
		case NTV2_FG_720x576:
			max = 720*2;
			offset = 8;
			break;
		default:
			// not supported at this time;
			return;

		}
		break;
	case AUTOCIRCVIDPROCMODE_VERTWIPE:
		regValue |= (BIT_0+BIT_3);
		splitModeValue = BIT_30;
		switch (frameGeometry)
		{
		case NTV2_FG_1920x1080:
			offset = 19;
			max = 1080+1;
		case NTV2_FG_1280x720:
			offset = 7;
			max = 720+1;
			break;
		case NTV2_FG_720x486:
			max = (486/2)+1;
			offset = 8;
			break;
		case NTV2_FG_720x576:
			max = (576/2)+1;
			offset = 8;
			break;
		default:
			// not supported at this time;
			return;
		}
		break;
	case AUTOCIRCVIDPROCMODE_KEY:
		regValue |= (BIT_0);
		break;
	}

	ntv2WriteRegister(pSysCon, kRegVidProc1Control, regValue);

	if (vidProcInfo->mode == AUTOCIRCVIDPROCMODE_MIX ||
		vidProcInfo->mode == AUTOCIRCVIDPROCMODE_KEY)
		return; ///we're done

	uint32_t positionValue = (Word)FixedMix(0,(Word)max, vidProcInfo->transitionCoefficient);
	uint32_t softnessPixels = 0x1FFF;
	uint32_t softnessSlope = 0x1FFF;
	if (vidProcInfo->transitionSoftness == 0)
	{
		softnessSlope = 0x1FFF;
		softnessPixels = 1;
	}
	else
	{
		// need to tame softness to based on position.
		// 1st find out what the maximum softness is
		uint32_t maxSoftness;
		if (positionValue > max/2)
			maxSoftness = max - positionValue;
		else
			maxSoftness = positionValue;

		// softness is limited to 1/4 of max
		if (maxSoftness > max/4)
		{
			maxSoftness = max/4;
		}

		if (maxSoftness == 0)
		{
			softnessPixels = 1;
		}
		else
		{
			softnessPixels = (Word)FixedMix(1,(Word)maxSoftness,vidProcInfo->transitionSoftness);

		}
	}
	if (softnessPixels == 0)
		softnessPixels = 1; // shouldn't happen but....
	softnessSlope = 0x1FFF/softnessPixels;
	positionValue -= (softnessPixels/2);

	ntv2WriteRegister(pSysCon, kRegSplitControl,splitModeValue | (softnessSlope<<16) | ((positionValue+offset)<<2));
}

void AutoCirculateSetupXena2Routing(NTV2AutoCirc* pAutoCirc, NTV2RoutingTable* pXena2Routing)
{
	Ntv2SystemContext* pSysCon = pAutoCirc->pSysCon;
	uint32_t numEntries = pXena2Routing->numEntries;
	uint32_t i;

	if (numEntries == 0 || numEntries >= MAX_ROUTING_ENTRIES)
		return;

	for (i = 0; i < numEntries; i++)
	{
		NTV2RoutingEntry* entry = &pXena2Routing->routingEntry[i];
//		if (m_pRegisters->IsSaveRecallRegister(entry->registerNum))
		{
			ntv2WriteRegisterMS(pSysCon,
								entry->registerNum,
								entry->value,
								entry->mask,
								entry->shift);
		}
	}
}

void AutoCirculateWriteHDMIAux(NTV2AutoCirc* pAutoCirc, uint32_t* pAuxData, uint32_t auxDataSize)
{
	NTV2DeviceID deviceID = pAutoCirc->deviceID;
	Ntv2SystemContext* pSysCon = pAutoCirc->pSysCon;
	uint32_t* pAux = pAuxData;
	uint32_t numData = NTV2_HDMIAuxDataSize;
	uint32_t numAux = auxDataSize/numData;
	uint32_t auxReg;
	uint32_t iAux;
	uint32_t iData;
	NTV2Channel channel = NTV2_CHANNEL1; 

	if (channel >= NTV2DeviceGetNumHDMIVideoOutputs(deviceID))
		return;
	if (numAux == 0)
		return;
	
	for (iAux = 0; iAux < numAux; iAux++)
	{
		if (NTV2DeviceGetHDMIVersion(deviceID) == 2)
		{
			auxReg = kRegHDMIOutputAuxData;
			for (iData = 0; iData < numData/4; iData++, auxReg++)
				ntv2WriteRegister(pSysCon, auxReg, pAux[iData]);
		}
		if (NTV2DeviceGetHDMIVersion(deviceID) == 4)
		{
//			ntv2_hdmiout4_write_info_frame(m_pHDMIOut4Monitor[channel],
//										   numData, ((uint8_t*)pAux) + (iAux * numData));
		}
		pAux += numData/4;
	}
}

//-------------------------------------------------------------------------------------------------------
//	AutoCirculateDmaAudioSetup
//-------------------------------------------------------------------------------------------------------
bool AutoCirculateDmaAudioSetup(INTERNAL_AUTOCIRCULATE_STRUCT* pAuto)
{
	Ntv2SystemContext* pSysCon = pAuto->pSysCon;
	
	if (!pAuto->circulateWithAudio)
	{
		return false;
	}

	uint32_t ulFrameNumber = pAuto->transferFrame;
	uint32_t ulAudioWrapAddress = GetAudioWrapAddress(pSysCon, pAuto->audioSystem);
	uint32_t ulAudioReadOffset = GetAudioReadOffset(pSysCon, pAuto->audioSystem);
	uint32_t ulPreWrapSize = 0;
	uint32_t ulPostWrapSize = 0;

	if (pAuto->recording)
	{
		uint32_t ulAudioEnd = pAuto->frameStamp[ulFrameNumber].audioInStopAddress;
		uint32_t ulAudioStart = pAuto->frameStamp[ulFrameNumber].audioInStartAddress;
		
		pAuto->audioTransferOffset = ulAudioStart - ulAudioReadOffset;
		if (ulAudioEnd < ulAudioStart)
		{
			ulPreWrapSize = ulAudioWrapAddress - (ulAudioStart - ulAudioReadOffset);
			ulPostWrapSize = ulAudioEnd - ulAudioReadOffset;
		}
		else
		{
			ulPreWrapSize = ulAudioEnd - ulAudioStart;
			ulPostWrapSize = 0;
		}

		// save this for oemCompleteAutoCirculateTransfer
		pAuto->audioTransferSize = ulPreWrapSize + ulPostWrapSize;
		pAuto->audioStartSample = 0;
	}
	else
	{
		uint32_t ulAudioBytes = pAuto->audioTransferSize;

		if(pAuto->audioDropsRequired > pAuto->audioDropsCompleted)
		{
			pAuto->nextAudioOutputAddress = 0;
			pAuto->audioDropsCompleted++;
#ifdef OEM_DROP_FRAME
			ntv2Message("CNTV2Device::AutoCirculateDmaAudioSetup - Auto %s: drop audio output required %d  completed %d\n", 
						CrosspointName[pAuto->channelSpec], pAuto->audioDropsRequired, pAuto->audioDropsCompleted);
#endif
		}

		// Audio start default
		pAuto->audioTransferOffset = pAuto->nextAudioOutputAddress;
		// Remember actual start
		pAuto->frameStamp[ulFrameNumber].audioExpectedAddress = pAuto->nextAudioOutputAddress;

		//		ntv2Message("Auto %s: frame number %d  expected address %08x\n", 
		//				 CrosspointName[pAuto->channelSpec], ulFrameNumber, pAuto->frameStamp[ulFrameNumber].audioExpectedAddress);

		// NOTE: '<' should be '<=', but when it is '==', we want to set _ulNextAudioOutputAddr to 0 
		if (pAuto->nextAudioOutputAddress + ulAudioBytes < ulAudioWrapAddress)
		{
			// No audio Wrap required so write out all the audio data at once.
			pAuto->nextAudioOutputAddress += ulAudioBytes;
			ulPreWrapSize = ulAudioBytes;
			ulPostWrapSize = 0;
		}
		else 
		{
			// a Wrap will be required to reach the target address.
			ulPreWrapSize = ulAudioWrapAddress - pAuto->nextAudioOutputAddress;
			ulPostWrapSize = ulAudioBytes - ulPreWrapSize;       
			pAuto->nextAudioOutputAddress = ulPostWrapSize;
		}
	}

	return true;
}

//-------------------------------------------------------------------------------------------------------
//	AutoCirculateP2PCopy
//-------------------------------------------------------------------------------------------------------
bool AutoCirculateP2PCopy(NTV2AutoCirc* pAutoCirc,
							 PAUTOCIRCULATE_P2P_STRUCT pDriverBuffer, 
							 PAUTOCIRCULATE_P2P_STRUCT pUserBuffer,
							 bool bToDriver)
{
	Ntv2UserBuffer userBuffer;
	Ntv2SystemContext* pSysCon = pAutoCirc->pSysCon;

	if((pDriverBuffer == NULL) || (pUserBuffer == NULL))
	{
		ntv2Message("CNTV2Device::AutoCirculateP2PCopy - NULL buffer %s\n", "");
		return false;
	}

	if(!bToDriver && (pDriverBuffer->p2pSize != sizeof(AUTOCIRCULATE_P2P_STRUCT)))
	{
		ntv2Message("CNTV2Device::AutoCirculateP2PCopy - bad driver P2P struct size %d\n",
					pDriverBuffer->p2pSize);
		return false;
	}

	if(bToDriver && (pUserBuffer->p2pSize != sizeof(AUTOCIRCULATE_P2P_STRUCT)))
	{
		ntv2Message("CNTV2Device::AutoCirculateP2PCopy - bad user P2P struct size %d\n",
					pUserBuffer->p2pSize);
		return false;
	}
	
	if (!ntv2UserBufferPrepare(&userBuffer, pSysCon,
							   (uint64_t)pUserBuffer,
							   sizeof(AUTOCIRCULATE_P2P_STRUCT),
							   !bToDriver))
	{	
		ntv2Message("CNTV2Device::AutoCirculateP2PCopy - prepare user buffer size %d failed\n",
					(int)sizeof(AUTOCIRCULATE_P2P_STRUCT));
		return false;
	}

	if(bToDriver)
	{
		ntv2UserBufferCopyFrom(&userBuffer, 0, pDriverBuffer, sizeof(AUTOCIRCULATE_P2P_STRUCT));
	}
	else
	{
		ntv2UserBufferCopyTo(&userBuffer, 0, pDriverBuffer, sizeof(AUTOCIRCULATE_P2P_STRUCT));
	}

	ntv2UserBufferRelease(&userBuffer);

	return true;
}

void CopyFrameStampOldToNew(const FRAME_STAMP_STRUCT * pInOldStruct, FRAME_STAMP * pOutNewStruct)
{
	pOutNewStruct->acFrameTime = pInOldStruct->frameTime;
	pOutNewStruct->acRequestedFrame = pInOldStruct->frame;
	pOutNewStruct->acAudioClockTimeStamp = pInOldStruct->audioClockTimeStamp;
	pOutNewStruct->acAudioExpectedAddress = pInOldStruct->audioExpectedAddress;
	pOutNewStruct->acAudioInStartAddress = pInOldStruct->audioInStartAddress;
	pOutNewStruct->acAudioInStopAddress = pInOldStruct->audioInStopAddress;
	pOutNewStruct->acAudioOutStopAddress = pInOldStruct->audioOutStopAddress;
	pOutNewStruct->acAudioOutStartAddress = pInOldStruct->audioOutStartAddress;
	pOutNewStruct->acTotalBytesTransferred = pInOldStruct->bytesRead;
	pOutNewStruct->acStartSample = pInOldStruct->startSample;
	//	pOutNewStruct->acTimeCodes						= pInOldStruct->???no equivalent???;
	pOutNewStruct->acCurrentTime = pInOldStruct->currentTime;
	pOutNewStruct->acCurrentFrame = pInOldStruct->currentFrame;
	pOutNewStruct->acCurrentFrameTime = pInOldStruct->currentFrameTime;
	pOutNewStruct->acAudioClockCurrentTime = pInOldStruct->audioClockCurrentTime;
	pOutNewStruct->acCurrentAudioExpectedAddress = pInOldStruct->currentAudioExpectedAddress;
	pOutNewStruct->acCurrentAudioStartAddress = pInOldStruct->currentAudioStartAddress;
	pOutNewStruct->acCurrentFieldCount = pInOldStruct->currentFieldCount;
	pOutNewStruct->acCurrentLineCount = pInOldStruct->currentLineCount;
	pOutNewStruct->acCurrentReps = pInOldStruct->currentReps;
	pOutNewStruct->acCurrentUserCookie = pInOldStruct->currenthUser;
	pOutNewStruct->acFrame = pInOldStruct->frame;
	//	pOutNewStruct->acRP188							= pInOldStruct->currentRP188;
	NTV2_RP188_from_RP188_STRUCT(pOutNewStruct->acRP188, pInOldStruct->currentRP188);
}

bool AutoCirculateFrameStampImmediate(NTV2AutoCirc* pAutoCirc, FRAME_STAMP * pInOutFrameStamp)
{
	Ntv2SystemContext*	pSysCon = pAutoCirc->pSysCon;
	NTV2Channel			channel;
	uint32_t			modeValue;
	NTV2Crosspoint		crosspoint;
	FRAME_STAMP_STRUCT	oldFrameStampStruct;
	INTERNAL_AUTOCIRCULATE_STRUCT*	pAuto;
	Ntv2Status status;
	int32_t frameNumber;
	
	//	On entry...
	//		FRAME_STAMP.acFrameTime			== requested NTV2Channel (in least significant byte)
	//		FRAME_STAMP.acRequestedFrame	== requested frame number
	if (!pInOutFrameStamp)
		return false;
	if (pInOutFrameStamp->acFrameTime < 0)
		return false;	//	Bad channel value

	channel = (NTV2Channel)pInOutFrameStamp->acFrameTime;
	if (!NTV2_IS_VALID_CHANNEL(channel))
		return false;	//	Bad channel value

	//	Use the current mode for the specified NTV2Channel to determine the NTV2Crosspoint...
	modeValue = 0;
	if (!ntv2ReadRegisterMS(pSysCon, gChannelToControlRegNum[channel], &modeValue, kRegMaskMode, kRegShiftMode))
	{
		return false;	//	No such register -- i.e., illegal channel value for this device?
	}

	crosspoint = (modeValue == NTV2_MODE_DISPLAY) ?
		GetNTV2CrosspointChannelForIndex(channel) : GetNTV2CrosspointInputForIndex(channel);
	if (ILLEGAL_CHANNELSPEC(crosspoint))
	{
		return false;
	}

	pAuto = &pAutoCirc->autoCirculate[crosspoint];
// 	if (pAuto->recording  &&  modeValue != NTV2_MODE_CAPTURE)
// 		ntv2Message("AutoCirculateFrameStampImmediate: pAuto->recording=true, but mode=output for crosspoint %d channel %d\n", crosspoint, channel);
// 	else if (!pAuto->recording  &&  modeValue != NTV2_MODE_DISPLAY)
// 		ntv2Message("AutoCirculateFrameStampImmediate: pAuto->recording=false, but mode=input for crosspoint %d channel %d\n", crosspoint, channel);

	memset(&oldFrameStampStruct, 0x00, sizeof(oldFrameStampStruct));

	//	Call the old GetFrameStamp API, then convert old FRAME_STAMP_STRUCT to new FRAME_STAMP struct...
	oldFrameStampStruct.channelSpec = crosspoint;
	status = AutoCirculateGetFrameStamp(pAutoCirc,
										crosspoint,
										pInOutFrameStamp->acRequestedFrame,
										&oldFrameStampStruct);
	if (status != NTV2_STATUS_SUCCESS)
	{
		return false;	//	old function failed
	}
	CopyFrameStampOldToNew(&oldFrameStampStruct, pInOutFrameStamp);

	//	Grab all available timecodes for the requested frame, regardless of capture or playout...
	frameNumber = pInOutFrameStamp->acRequestedFrame;
	if (frameNumber < pAuto->startFrame || frameNumber > pAuto->endFrame)
		frameNumber = pAuto->activeFrame;		//	Use active frame if requested frame invalid
	if (frameNumber < pAuto->startFrame || frameNumber > pAuto->endFrame)
		frameNumber = pAuto->startFrame;		//	Use start frame if active frame invalid
	NTV2_RP188_from_RP188_STRUCT(pInOutFrameStamp->acRP188, pAuto->frameStamp[frameNumber].rp188);	//	NOTE:  acRP188 field is deprecated
	if (!CopyFrameStampTCArrayToNTV2TimeCodeArray(&pAuto->frameStamp[frameNumber].internalTCArray,
												  (NTV2_RP188*)pInOutFrameStamp->acTimeCodes.fUserSpacePtr,
												  pInOutFrameStamp->acTimeCodes.fByteCount))
	{
		//("AutoCirculateFrameStampImmediate: CopyFrameStampTCArrayToNTV2TimeCodeArray failed, frame=%d, byteCount=%d\n", frameNumber, pInOutFrameStamp->acTimeCodes.fByteCount);
		return false;
	}
	
	return true;
}

bool AutoCirculateCanDoFieldMode(INTERNAL_AUTOCIRCULATE_STRUCT* pAuto)
{
	Ntv2SystemContext* pSysCon = pAuto->pSysCon;
	NTV2Channel syncChannel = NTV2_CHANNEL1;
	bool fieldMode = false;

	if(IsMultiFormatActive(pSysCon))
	{
		syncChannel = GetNTV2ChannelForNTV2Crosspoint(pAuto->channelSpec);
	}

	NTV2VideoFormat vidFormat = GetBoardVideoFormat(pSysCon, syncChannel);
	switch (vidFormat)
	{
	case NTV2_FORMAT_525_5994:
	case NTV2_FORMAT_625_5000:
	case NTV2_FORMAT_1080i_5000:
	case NTV2_FORMAT_1080i_5994:
	case NTV2_FORMAT_1080i_6000:
		fieldMode = true;
		break;
	default:
		break;
	}

	return fieldMode;
}
