/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
//==========================================================================
//
//  ntv2rp188.cpp
//
//==========================================================================

#include "ntv2rp188.h"

static const ULWord	gChannelToRP188ModeMasks[] = { kRegMaskRP188ModeCh1, kRegMaskRP188ModeCh2, kRegMaskRP188ModeCh3, kRegMaskRP188ModeCh4,
													kRegMaskRP188ModeCh5, (ULWord)kRegMaskRP188ModeCh6, kRegMaskRP188ModeCh7, kRegMaskRP188ModeCh8, 0 };
static const ULWord	gChannelToRP188ModeShifts[] = { kRegShiftRP188ModeCh1, kRegShiftRP188ModeCh2, kRegShiftRP188ModeCh3, kRegShiftRP188ModeCh4,
													kRegShiftRP188ModeCh5, kRegShiftRP188ModeCh6, kRegShiftRP188ModeCh7, kRegShiftRP188ModeCh8, 0 };
static const ULWord	gChannelToRP188DBBRegisterNum[] = { kRegRP188InOut1DBB, kRegRP188InOut2DBB, kRegRP188InOut3DBB, kRegRP188InOut4DBB,
														kRegRP188InOut5DBB, kRegRP188InOut6DBB, kRegRP188InOut7DBB, kRegRP188InOut8DBB, 0 };
static const ULWord	gChannelToRP188ModeGCRegisterNum[] = { kRegGlobalControl, kRegGlobalControl, kRegGlobalControl2, kRegGlobalControl2,
															kRegGlobalControl2, kRegGlobalControl2, kRegGlobalControl2, kRegGlobalControl2, 0 };
static const ULWord	gChannelToRP188Bits031RegisterNum[] = { kRegRP188InOut1Bits0_31, kRegRP188InOut2Bits0_31, kRegRP188InOut3Bits0_31, kRegRP188InOut4Bits0_31,
															kRegRP188InOut5Bits0_31, kRegRP188InOut6Bits0_31, kRegRP188InOut7Bits0_31, kRegRP188InOut8Bits0_31, 0 };
static const ULWord	gChannelToRP188Bits3263RegisterNum[] = { kRegRP188InOut1Bits32_63, kRegRP188InOut2Bits32_63, kRegRP188InOut3Bits32_63, kRegRP188InOut4Bits32_63,
															kRegRP188InOut5Bits32_63, kRegRP188InOut6Bits32_63, kRegRP188InOut7Bits32_63, kRegRP188InOut8Bits32_63, 0 };
static const ULWord	gChannelToRP188Bits031_2RegisterNum[] = { kRegRP188InOut1Bits0_31_2, kRegRP188InOut2Bits0_31_2, kRegRP188InOut3Bits0_31_2, kRegRP188InOut4Bits0_31_2,
																kRegRP188InOut5Bits0_31_2, kRegRP188InOut6Bits0_31_2, kRegRP188InOut7Bits0_31_2, kRegRP188InOut8Bits0_31_2, 0 };
static const ULWord	gChannelToRP188Bits3263_2RegisterNum[] = { kRegRP188InOut1Bits32_63_2, kRegRP188InOut2Bits32_63_2, kRegRP188InOut3Bits32_63_2, kRegRP188InOut4Bits32_63_2,
																kRegRP188InOut5Bits32_63_2, kRegRP188InOut6Bits32_63_2, kRegRP188InOut7Bits32_63_2, kRegRP188InOut8Bits32_63_2, 0 };
static const ULWord	gChannelToLTCEmbeddedBits031RegisterNum[] = { kRegLTCEmbeddedBits0_31, kRegLTC2EmbeddedBits0_31, kRegLTC3EmbeddedBits0_31, kRegLTC4EmbeddedBits0_31,
															kRegLTC5EmbeddedBits0_31, kRegLTC6EmbeddedBits0_31, kRegLTC7EmbeddedBits0_31, kRegLTC8EmbeddedBits0_31, 0 };
static const ULWord	gChannelToLTCEmbeddedBits3263RegisterNum[] = { kRegLTCEmbeddedBits32_63, kRegLTC2EmbeddedBits32_63, kRegLTC3EmbeddedBits32_63, kRegLTC4EmbeddedBits32_63,
															kRegLTC5EmbeddedBits32_63, kRegLTC6EmbeddedBits32_63, kRegLTC7EmbeddedBits32_63, kRegLTC8EmbeddedBits32_63, 0 };

#define	RP188_IS_VALID(_n_)					((_n_).DBB != 0xFFFFFFFF || (_n_).Low != 0xFFFFFFFF || (_n_).High != 0xFFFFFFFF)

static const ULWord	gChannelToRXSDIStatusRegs[] = { kRegRXSDI1Status, kRegRXSDI2Status, kRegRXSDI3Status, kRegRXSDI4Status, kRegRXSDI5Status, kRegRXSDI6Status, kRegRXSDI7Status, kRegRXSDI8Status, 0 };
static const ULWord	gChannelToRXSDICRCErrorCountRegs[] = { kRegRXSDI1CRCErrorCount, kRegRXSDI2CRCErrorCount, kRegRXSDI3CRCErrorCount, kRegRXSDI4CRCErrorCount, kRegRXSDI5CRCErrorCount, kRegRXSDI6CRCErrorCount, kRegRXSDI7CRCErrorCount, kRegRXSDI8CRCErrorCount, 0 };
static const ULWord	gChannelToRXSDIFrameCountLoRegs[] = { kRegRXSDI1FrameCountLow, kRegRXSDI2FrameCountLow, kRegRXSDI3FrameCountLow, kRegRXSDI4FrameCountLow, kRegRXSDI5FrameCountLow, kRegRXSDI6FrameCountLow, kRegRXSDI7FrameCountLow, kRegRXSDI8FrameCountLow, 0 };
static const ULWord	gChannelToRXSDIFrameCountHiRegs[] = { kRegRXSDI1FrameCountHigh, kRegRXSDI2FrameCountHigh, kRegRXSDI3FrameCountHigh, kRegRXSDI4FrameCountHigh, kRegRXSDI5FrameCountHigh, kRegRXSDI6FrameCountHigh, kRegRXSDI7FrameCountHigh, kRegRXSDI8FrameCountHigh, 0 };
static const ULWord	gChannelToRXSDIFrameRefCountLoRegs[] = { kRegRXSDI1FrameRefCountLow, kRegRXSDI2FrameRefCountLow, kRegRXSDI3FrameRefCountLow, kRegRXSDI4FrameRefCountLow, kRegRXSDI5FrameRefCountLow, kRegRXSDI6FrameRefCountLow, kRegRXSDI7FrameRefCountLow, kRegRXSDI8FrameRefCountLow, 0 };
static const ULWord	gChannelToRXSDIFrameRefCountHiRegs[] = { kRegRXSDI1FrameRefCountHigh, kRegRXSDI2FrameRefCountHigh, kRegRXSDI3FrameRefCountHigh, kRegRXSDI4FrameRefCountHigh, kRegRXSDI5FrameRefCountHigh, kRegRXSDI6FrameRefCountHigh, kRegRXSDI7FrameRefCountHigh, kRegRXSDI8FrameRefCountHigh, 0 };

bool InitRP188(Ntv2SystemContext* context)
{
	INTERNAL_TIMECODE_STRUCT frameStampTCArray;
	ULWord i = 0;
	NTV2DeviceID deviceID = (NTV2DeviceID)ntv2ReadRegister(context, kRegBoardID);
	for (i = 0; i < NTV2DeviceGetNumVideoOutputs(deviceID); i++)
	{
		SetRP188Mode(context, (NTV2Channel)i, NTV2_RP188_OUTPUT);
		if (ntv2ReadVirtualRegister(context, kVRegUserDefinedDBB) == 0)
		{
			ULWord inputFilter = 0x00;
			if (NTV2DeviceCanDoVITC2(deviceID))
				inputFilter = 0x02;
			else if (deviceID != DEVICE_ID_KONALHI)
				inputFilter = 0x01;
			ntv2WriteRegisterMS(context, gChannelToRP188DBBRegisterNum[i], inputFilter, (ULWord)kRegMaskRP188SourceSelect, kRegShiftRP188Source);
			ntv2WriteRegisterMS(context, gChannelToRP188DBBRegisterNum[i], deviceID == DEVICE_ID_KONALHI ? 0x00 : 0xFF, kRegMaskRP188DBB, kRegShiftRP188DBB);
		}
	}

	if (NTV2DeviceCanDoWidget(deviceID, NTV2_WgtSDIMonOut1))
	{
		SetRP188Mode(context, NTV2_CHANNEL5, NTV2_RP188_OUTPUT);
		if (ntv2ReadVirtualRegister(context, kVRegUserDefinedDBB) == 0)
		{
			ntv2WriteRegisterMS(context, gChannelToRP188DBBRegisterNum[NTV2_CHANNEL5], 0xFF, kRegMaskRP188DBB, kRegShiftRP188DBB);
		}
	}
	
	memset(&frameStampTCArray, 0x00, sizeof(INTERNAL_TIMECODE_STRUCT));
	CopyFrameStampTCArrayToHardware(context, &frameStampTCArray);

	return true;
}

bool CopyRP188HardwareToFrameStampTCArray(Ntv2SystemContext* context, INTERNAL_TIMECODE_STRUCT* tcStruct)
{
	NTV2DeviceID deviceID = (NTV2DeviceID)ntv2ReadRegister(context, kRegBoardID);

	if (tcStruct == NULL)
	{
		return false;
	}

	memset(tcStruct, 0xFF, sizeof(INTERNAL_TIMECODE_STRUCT));

	switch(NTV2DeviceGetNumVideoInputs(deviceID))
	{
	case 8:
		if(GetInputVideoFormat(context, NTV2_CHANNEL8) != NTV2_FORMAT_UNKNOWN)
			GetReceivedTCForChannel(context, NTV2_CHANNEL8, &tcStruct->LTCEmbedded8, &tcStruct->TCInOut8, &tcStruct->TCInOut8_2);
		// fall through
	case 7:
		if(GetInputVideoFormat(context, NTV2_CHANNEL7) != NTV2_FORMAT_UNKNOWN)
			GetReceivedTCForChannel(context, NTV2_CHANNEL7, &tcStruct->LTCEmbedded7, &tcStruct->TCInOut7, &tcStruct->TCInOut7_2);
		// fall through
	case 6:
		if(GetInputVideoFormat(context, NTV2_CHANNEL6) != NTV2_FORMAT_UNKNOWN)
			GetReceivedTCForChannel(context, NTV2_CHANNEL6, &tcStruct->LTCEmbedded6, &tcStruct->TCInOut6, &tcStruct->TCInOut6_2);
		// fall through
	case 5:
		if(GetInputVideoFormat(context, NTV2_CHANNEL5) != NTV2_FORMAT_UNKNOWN)
			GetReceivedTCForChannel(context, NTV2_CHANNEL5, &tcStruct->LTCEmbedded5, &tcStruct->TCInOut5, &tcStruct->TCInOut5_2);
		// fall through
	case 4:
		if(GetInputVideoFormat(context, NTV2_CHANNEL4) != NTV2_FORMAT_UNKNOWN)
			GetReceivedTCForChannel(context, NTV2_CHANNEL4, &tcStruct->LTCEmbedded4, &tcStruct->TCInOut4, &tcStruct->TCInOut4_2);
		// fall through
	case 3:
		if(GetInputVideoFormat(context, NTV2_CHANNEL3) != NTV2_FORMAT_UNKNOWN)
			GetReceivedTCForChannel(context, NTV2_CHANNEL3, &tcStruct->LTCEmbedded3, &tcStruct->TCInOut3, &tcStruct->TCInOut3_2);
		// fall through
	case 2:
		if(GetInputVideoFormat(context, NTV2_CHANNEL2) != NTV2_FORMAT_UNKNOWN)
			GetReceivedTCForChannel(context, NTV2_CHANNEL2, &tcStruct->LTCEmbedded2, &tcStruct->TCInOut2, &tcStruct->TCInOut2_2);
		// fall through
	case 1:
		if(GetInputVideoFormat(context, NTV2_CHANNEL1) != NTV2_FORMAT_UNKNOWN)
		GetReceivedTCForChannel(context, NTV2_CHANNEL1, &tcStruct->LTCEmbedded1, &tcStruct->TCInOut1, &tcStruct->TCInOut1_2);
		// fall through
	default:
		break;
	}

	if (NTV2DeviceCanDoLTCInN(deviceID, NTV2_CHANNEL1))
	{
		GetReceivedAnalogLTC(context, &tcStruct->LTCAnalog1, NULL);
	}

	if (NTV2DeviceCanDoLTCInN(deviceID, NTV2_CHANNEL2))
	{
		GetReceivedAnalogLTC(context, NULL, &tcStruct->LTCAnalog2);
	}

	return true;
}

bool CopyFrameStampTCArrayToHardware(Ntv2SystemContext* context, INTERNAL_TIMECODE_STRUCT* acFrameStampTCArray)
{
	RP188_STRUCT * pTCInOutEntry = NULL;
	RP188_STRUCT * pTCLTCEmbedEntry = NULL;
	UWord chan = 0;
	NTV2DeviceID deviceID = (NTV2DeviceID)ntv2ReadRegister(context, kRegBoardID);

	if (acFrameStampTCArray == NULL)
	{
		return false;
	}

	for (chan = 0; chan < NTV2DeviceGetNumVideoOutputs(deviceID); chan++)
	{

		switch (chan)
		{
		case 0:	pTCInOutEntry = &acFrameStampTCArray->TCInOut1;
			pTCLTCEmbedEntry = &acFrameStampTCArray->LTCEmbedded1; break;
		case 1:	pTCInOutEntry = &acFrameStampTCArray->TCInOut2;
			pTCLTCEmbedEntry = &acFrameStampTCArray->LTCEmbedded2; break;
		case 2:	pTCInOutEntry = &acFrameStampTCArray->TCInOut3;
			pTCLTCEmbedEntry = &acFrameStampTCArray->LTCEmbedded3; break;
		case 3:	pTCInOutEntry = &acFrameStampTCArray->TCInOut4;
			pTCLTCEmbedEntry = &acFrameStampTCArray->LTCEmbedded4; break;
		case 4:	pTCInOutEntry = &acFrameStampTCArray->TCInOut5;
			pTCLTCEmbedEntry = &acFrameStampTCArray->LTCEmbedded5; break;
		case 5:	pTCInOutEntry = &acFrameStampTCArray->TCInOut6;
			pTCLTCEmbedEntry = &acFrameStampTCArray->LTCEmbedded6; break;
		case 6:	pTCInOutEntry = &acFrameStampTCArray->TCInOut7;
			pTCLTCEmbedEntry = &acFrameStampTCArray->LTCEmbedded7; break;
		case 7:	pTCInOutEntry = &acFrameStampTCArray->TCInOut8;
			pTCLTCEmbedEntry = &acFrameStampTCArray->LTCEmbedded8; break;
		}

		if (RP188_IS_VALID(*pTCInOutEntry))
		{
			if (ntv2ReadVirtualRegister(context, kVRegUserDefinedDBB) == 0)
			{
				if (NTV2_IS_SD_VIDEO_FORMAT(GetBoardVideoFormat(context, (NTV2Channel)chan)) || deviceID == DEVICE_ID_KONALHI)
				{
					ntv2WriteRegisterMS(context, gChannelToRP188DBBRegisterNum[chan], 0x00, kRegMaskRP188DBB, kRegShiftRP188DBB);
				}
				else
				{
					ntv2WriteRegisterMS(context, gChannelToRP188DBBRegisterNum[chan], 0xFF, kRegMaskRP188DBB, kRegShiftRP188DBB);
				}
			}
			else
			{
				ntv2WriteRegisterMS(context, gChannelToRP188DBBRegisterNum[chan], pTCInOutEntry->DBB, kRegMaskRP188DBB, kRegShiftRP188DBB);
			}

			ntv2WriteRegister(context, gChannelToRP188Bits031RegisterNum[chan], pTCInOutEntry->Low);
			ntv2WriteRegister(context, gChannelToRP188Bits3263RegisterNum[chan], pTCInOutEntry->High);
		}

		if (NTV2DeviceCanDoLTCEmbeddedN(deviceID, chan) && RP188_IS_VALID(*pTCLTCEmbedEntry))
		{
			ntv2WriteRegister(context, gChannelToLTCEmbeddedBits031RegisterNum[chan], pTCLTCEmbedEntry->Low);
			ntv2WriteRegister(context, gChannelToLTCEmbeddedBits3263RegisterNum[chan], pTCLTCEmbedEntry->High);
		}
	}

	if (NTV2DeviceCanDoWidget(deviceID, NTV2_WgtSDIMonOut1))
	{
		if (RP188_IS_VALID(*pTCInOutEntry))
		{
			if (ntv2ReadVirtualRegister(context, kVRegUserDefinedDBB) == 0)
			{
				if (NTV2_IS_SD_VIDEO_FORMAT(GetBoardVideoFormat(context, NTV2_CHANNEL5)))
				{
					ntv2WriteRegisterMS(context, gChannelToRP188DBBRegisterNum[NTV2_CHANNEL5], 0x00, kRegMaskRP188DBB, kRegShiftRP188DBB);
				}
				else
				{
					ntv2WriteRegisterMS(context, gChannelToRP188DBBRegisterNum[NTV2_CHANNEL5], 0xFF, kRegMaskRP188DBB, kRegShiftRP188DBB);
				}
			}
			else
			{
				ntv2WriteRegisterMS(context, gChannelToRP188DBBRegisterNum[NTV2_CHANNEL5], acFrameStampTCArray->TCInOut5.DBB, kRegMaskRP188DBB, kRegShiftRP188DBB);
			}

			ntv2WriteRegister(context, gChannelToRP188Bits031RegisterNum[NTV2_CHANNEL5], acFrameStampTCArray->TCInOut5.Low);
			ntv2WriteRegister(context, gChannelToRP188Bits3263RegisterNum[NTV2_CHANNEL5], acFrameStampTCArray->TCInOut5.High);
		}

		ntv2WriteRegister(context, gChannelToLTCEmbeddedBits031RegisterNum[NTV2_CHANNEL5], acFrameStampTCArray->LTCEmbedded5.Low);
		ntv2WriteRegister(context, gChannelToLTCEmbeddedBits3263RegisterNum[NTV2_CHANNEL5], acFrameStampTCArray->LTCEmbedded5.High);
	}

	if (NTV2DeviceCanDoLTCOutN(deviceID, NTV2_CHANNEL1) && RP188_IS_VALID(acFrameStampTCArray->LTCAnalog1))
	{
		ntv2WriteRegister(context, kRegLTCAnalogBits0_31, acFrameStampTCArray->LTCAnalog1.Low);
		ntv2WriteRegister(context, kRegLTCAnalogBits32_63, acFrameStampTCArray->LTCAnalog1.High);
	}

	if (NTV2DeviceCanDoLTCOutN(deviceID, NTV2_CHANNEL2) && RP188_IS_VALID(acFrameStampTCArray->LTCAnalog2))
	{
		ntv2WriteRegister(context, kRegLTC2AnalogBits0_31, acFrameStampTCArray->LTCAnalog2.Low);
		ntv2WriteRegister(context, kRegLTC2AnalogBits32_63, acFrameStampTCArray->LTCAnalog2.High);
	}

	return true;
}

static void ClearFrameStampTCArray(INTERNAL_TIMECODE_STRUCT* frameStampTCArray)
{
	memset(frameStampTCArray, 0xFF, sizeof(INTERNAL_TIMECODE_STRUCT));
}

bool CopyNTV2TimeCodeArrayToFrameStampTCArray(INTERNAL_TIMECODE_STRUCT * internalTCArray, NTV2_RP188 * pInTCArray, ULWord inMaxBytes)
{
	ULWord	maxNumElements = inMaxBytes / sizeof(NTV2_RP188);
	if (!pInTCArray)
		return false;
	if (!maxNumElements)
		return false;
	if (maxNumElements < NTV2_MAX_NUM_TIMECODE_INDEXES)
		return false;	//	It's all or nothing

	ClearFrameStampTCArray(internalTCArray);

	if(NTV2_RP188_IS_VALID(pInTCArray[NTV2_TCINDEX_SDI1]))
		RP188_STRUCT_from_NTV2_RP188(internalTCArray->TCInOut1, pInTCArray[NTV2_TCINDEX_SDI1]);
	if (NTV2_RP188_IS_VALID(pInTCArray[NTV2_TCINDEX_SDI1_2]))
		RP188_STRUCT_from_NTV2_RP188(internalTCArray->TCInOut1_2, pInTCArray[NTV2_TCINDEX_SDI1_2]);
	if (NTV2_RP188_IS_VALID(pInTCArray[NTV2_TCINDEX_SDI1_LTC]))
		RP188_STRUCT_from_NTV2_RP188(internalTCArray->LTCEmbedded1, pInTCArray[NTV2_TCINDEX_SDI1_LTC]);

	if (NTV2_RP188_IS_VALID(pInTCArray[NTV2_TCINDEX_SDI2]))
		RP188_STRUCT_from_NTV2_RP188(internalTCArray->TCInOut2, pInTCArray[NTV2_TCINDEX_SDI2]);
	if (NTV2_RP188_IS_VALID(pInTCArray[NTV2_TCINDEX_SDI2_2]))
		RP188_STRUCT_from_NTV2_RP188(internalTCArray->TCInOut2_2, pInTCArray[NTV2_TCINDEX_SDI2_2]);
	if (NTV2_RP188_IS_VALID(pInTCArray[NTV2_TCINDEX_SDI2_LTC]))
		RP188_STRUCT_from_NTV2_RP188(internalTCArray->LTCEmbedded2, pInTCArray[NTV2_TCINDEX_SDI2_LTC]);

	if (NTV2_RP188_IS_VALID(pInTCArray[NTV2_TCINDEX_SDI3]))
		RP188_STRUCT_from_NTV2_RP188(internalTCArray->TCInOut3, pInTCArray[NTV2_TCINDEX_SDI3]);
	if (NTV2_RP188_IS_VALID(pInTCArray[NTV2_TCINDEX_SDI3_2]))
		RP188_STRUCT_from_NTV2_RP188(internalTCArray->TCInOut3_2, pInTCArray[NTV2_TCINDEX_SDI3_2]);
	if (NTV2_RP188_IS_VALID(pInTCArray[NTV2_TCINDEX_SDI3_LTC]))
		RP188_STRUCT_from_NTV2_RP188(internalTCArray->LTCEmbedded3, pInTCArray[NTV2_TCINDEX_SDI3_LTC]);

	if (NTV2_RP188_IS_VALID(pInTCArray[NTV2_TCINDEX_SDI4]))
		RP188_STRUCT_from_NTV2_RP188(internalTCArray->TCInOut4, pInTCArray[NTV2_TCINDEX_SDI4]);
	if (NTV2_RP188_IS_VALID(pInTCArray[NTV2_TCINDEX_SDI4_2]))
		RP188_STRUCT_from_NTV2_RP188(internalTCArray->TCInOut4_2, pInTCArray[NTV2_TCINDEX_SDI4_2]);
	if (NTV2_RP188_IS_VALID(pInTCArray[NTV2_TCINDEX_SDI4_LTC]))
		RP188_STRUCT_from_NTV2_RP188(internalTCArray->LTCEmbedded4, pInTCArray[NTV2_TCINDEX_SDI4_LTC]);

	if (NTV2_RP188_IS_VALID(pInTCArray[NTV2_TCINDEX_SDI5]))
		RP188_STRUCT_from_NTV2_RP188(internalTCArray->TCInOut5, pInTCArray[NTV2_TCINDEX_SDI5]);
	if (NTV2_RP188_IS_VALID(pInTCArray[NTV2_TCINDEX_SDI5_2]))
		RP188_STRUCT_from_NTV2_RP188(internalTCArray->TCInOut5_2, pInTCArray[NTV2_TCINDEX_SDI5_2]);
	if (NTV2_RP188_IS_VALID(pInTCArray[NTV2_TCINDEX_SDI5_LTC]))
		RP188_STRUCT_from_NTV2_RP188(internalTCArray->LTCEmbedded5, pInTCArray[NTV2_TCINDEX_SDI5_LTC]);

	if (NTV2_RP188_IS_VALID(pInTCArray[NTV2_TCINDEX_SDI6]))
		RP188_STRUCT_from_NTV2_RP188(internalTCArray->TCInOut6, pInTCArray[NTV2_TCINDEX_SDI6]);
	if (NTV2_RP188_IS_VALID(pInTCArray[NTV2_TCINDEX_SDI6_2]))
		RP188_STRUCT_from_NTV2_RP188(internalTCArray->TCInOut6_2, pInTCArray[NTV2_TCINDEX_SDI6_2]);
	if (NTV2_RP188_IS_VALID(pInTCArray[NTV2_TCINDEX_SDI6_LTC]))
		RP188_STRUCT_from_NTV2_RP188(internalTCArray->LTCEmbedded6, pInTCArray[NTV2_TCINDEX_SDI6_LTC]);

	if (NTV2_RP188_IS_VALID(pInTCArray[NTV2_TCINDEX_SDI7]))
		RP188_STRUCT_from_NTV2_RP188(internalTCArray->TCInOut7, pInTCArray[NTV2_TCINDEX_SDI7]);
	if (NTV2_RP188_IS_VALID(pInTCArray[NTV2_TCINDEX_SDI7_2]))
		RP188_STRUCT_from_NTV2_RP188(internalTCArray->TCInOut7_2, pInTCArray[NTV2_TCINDEX_SDI7_2]);
	if (NTV2_RP188_IS_VALID(pInTCArray[NTV2_TCINDEX_SDI7_LTC]))
		RP188_STRUCT_from_NTV2_RP188(internalTCArray->LTCEmbedded7, pInTCArray[NTV2_TCINDEX_SDI7_LTC]);

	if (NTV2_RP188_IS_VALID(pInTCArray[NTV2_TCINDEX_SDI8]))
		RP188_STRUCT_from_NTV2_RP188(internalTCArray->TCInOut8, pInTCArray[NTV2_TCINDEX_SDI8]);
	if (NTV2_RP188_IS_VALID(pInTCArray[NTV2_TCINDEX_SDI8_2]))
		RP188_STRUCT_from_NTV2_RP188(internalTCArray->TCInOut8_2, pInTCArray[NTV2_TCINDEX_SDI8_2]);
	if (NTV2_RP188_IS_VALID(pInTCArray[NTV2_TCINDEX_SDI8_LTC]))
		RP188_STRUCT_from_NTV2_RP188(internalTCArray->LTCEmbedded8, pInTCArray[NTV2_TCINDEX_SDI8_LTC]);

	if (NTV2_RP188_IS_VALID(pInTCArray[NTV2_TCINDEX_LTC1]))
		RP188_STRUCT_from_NTV2_RP188(internalTCArray->LTCAnalog1, pInTCArray[NTV2_TCINDEX_LTC1]);
	if (NTV2_RP188_IS_VALID(pInTCArray[NTV2_TCINDEX_LTC2]))
		RP188_STRUCT_from_NTV2_RP188(internalTCArray->LTCAnalog2, pInTCArray[NTV2_TCINDEX_LTC2]);

	return true;
}


bool CopyFrameStampTCArrayToNTV2TimeCodeArray(INTERNAL_TIMECODE_STRUCT * tcStruct, NTV2_RP188 * pOutTCArray, ULWord inMaxBytes)
{
	const ULWord maxNumElements = inMaxBytes / sizeof(NTV2_RP188);
	if (!pOutTCArray)
		return false;
	if (!maxNumElements)
		return false;
	if (maxNumElements < NTV2_MAX_NUM_TIMECODE_INDEXES)
		return false;	//	It's all or nothing

	NTV2_RP188_from_RP188_STRUCT(pOutTCArray[NTV2_TCINDEX_SDI1], tcStruct->TCInOut1);
	NTV2_RP188_from_RP188_STRUCT(pOutTCArray[NTV2_TCINDEX_SDI1_2], tcStruct->TCInOut1_2);
	NTV2_RP188_from_RP188_STRUCT(pOutTCArray[NTV2_TCINDEX_SDI1_LTC], tcStruct->LTCEmbedded1);

	NTV2_RP188_from_RP188_STRUCT(pOutTCArray[NTV2_TCINDEX_SDI2], tcStruct->TCInOut2);
	NTV2_RP188_from_RP188_STRUCT(pOutTCArray[NTV2_TCINDEX_SDI2_2], tcStruct->TCInOut2_2);
	NTV2_RP188_from_RP188_STRUCT(pOutTCArray[NTV2_TCINDEX_SDI2_LTC], tcStruct->LTCEmbedded2);

	NTV2_RP188_from_RP188_STRUCT(pOutTCArray[NTV2_TCINDEX_SDI3], tcStruct->TCInOut3);
	NTV2_RP188_from_RP188_STRUCT(pOutTCArray[NTV2_TCINDEX_SDI3_2], tcStruct->TCInOut3_2);
	NTV2_RP188_from_RP188_STRUCT(pOutTCArray[NTV2_TCINDEX_SDI3_LTC], tcStruct->LTCEmbedded3);

	NTV2_RP188_from_RP188_STRUCT(pOutTCArray[NTV2_TCINDEX_SDI4], tcStruct->TCInOut4);
	NTV2_RP188_from_RP188_STRUCT(pOutTCArray[NTV2_TCINDEX_SDI4_2], tcStruct->TCInOut4_2);
	NTV2_RP188_from_RP188_STRUCT(pOutTCArray[NTV2_TCINDEX_SDI4_LTC], tcStruct->LTCEmbedded4);

	NTV2_RP188_from_RP188_STRUCT(pOutTCArray[NTV2_TCINDEX_SDI5], tcStruct->TCInOut5);
	NTV2_RP188_from_RP188_STRUCT(pOutTCArray[NTV2_TCINDEX_SDI5_2], tcStruct->TCInOut5_2);
	NTV2_RP188_from_RP188_STRUCT(pOutTCArray[NTV2_TCINDEX_SDI5_LTC], tcStruct->LTCEmbedded5);

	NTV2_RP188_from_RP188_STRUCT(pOutTCArray[NTV2_TCINDEX_SDI6], tcStruct->TCInOut6);
	NTV2_RP188_from_RP188_STRUCT(pOutTCArray[NTV2_TCINDEX_SDI6_2], tcStruct->TCInOut6_2);
	NTV2_RP188_from_RP188_STRUCT(pOutTCArray[NTV2_TCINDEX_SDI6_LTC], tcStruct->LTCEmbedded6);

	NTV2_RP188_from_RP188_STRUCT(pOutTCArray[NTV2_TCINDEX_SDI7], tcStruct->TCInOut7);
	NTV2_RP188_from_RP188_STRUCT(pOutTCArray[NTV2_TCINDEX_SDI7_2], tcStruct->TCInOut7_2);
	NTV2_RP188_from_RP188_STRUCT(pOutTCArray[NTV2_TCINDEX_SDI7_LTC], tcStruct->LTCEmbedded7);

	NTV2_RP188_from_RP188_STRUCT(pOutTCArray[NTV2_TCINDEX_SDI8], tcStruct->TCInOut8);
	NTV2_RP188_from_RP188_STRUCT(pOutTCArray[NTV2_TCINDEX_SDI8_2], tcStruct->TCInOut8_2);
	NTV2_RP188_from_RP188_STRUCT(pOutTCArray[NTV2_TCINDEX_SDI8_LTC], tcStruct->LTCEmbedded8);

	NTV2_RP188_from_RP188_STRUCT(pOutTCArray[NTV2_TCINDEX_LTC1], tcStruct->LTCAnalog1);
	NTV2_RP188_from_RP188_STRUCT(pOutTCArray[NTV2_TCINDEX_LTC2], tcStruct->LTCAnalog2);

	return true;	
}

void SetRP188Mode(Ntv2SystemContext* context, NTV2Channel channel, NTV2_RP188Mode value)
{
	ntv2WriteRegisterMS(context, gChannelToRP188ModeGCRegisterNum[channel], value, gChannelToRP188ModeMasks[channel], gChannelToRP188ModeShifts[channel]);
}

/**	Currently unused
static NTV2_RP188Mode GetRP188Mode(Ntv2SystemContext* context, NTV2Channel channel)
{
	ULWord regValue = 0;
	ntv2ReadRegisterMS(context, gChannelToRP188ModeGCRegisterNum[channel], &regValue, gChannelToRP188ModeMasks[channel], gChannelToRP188ModeShifts[channel]);
	return (NTV2_RP188Mode)regValue;
}
**/

bool GetReceivedTCForChannel(Ntv2SystemContext* context, NTV2Channel channel, RP188_STRUCT* LTCIn, RP188_STRUCT* VITC1In, RP188_STRUCT* VITC2In)
{
	ULWord receivedAnyRP188 = 0, receivedLTC = 0, receivedVITC1 = 0, receivedVITC2 = 0, inputFilter = 0x02;
	NTV2VideoFormat channelFormat = GetBoardVideoFormat(context, channel);

	//Check if any TC is available
	ntv2ReadRegisterMS(context, gChannelToRP188DBBRegisterNum[channel], &receivedAnyRP188, BIT(16), 16);
	if (receivedAnyRP188 == 0)
		return false;

	//Read the input filter, could be overridden by software AC inits to FF only once
	ntv2ReadRegisterMS(context, gChannelToRP188DBBRegisterNum[channel], &inputFilter, (ULWord)kRegMaskRP188SourceSelect, kRegShiftRP188Source);
	//if the filter is not FF only fill in the selected, if received
	switch (inputFilter)
	{
	case 0x0000:
		//user requested ltc only
		ntv2ReadRegisterMS(context, gChannelToRP188DBBRegisterNum[channel], &receivedLTC, BIT(17), 17);
		if (receivedLTC == 1)
		{
			LTCIn->DBB = ntv2ReadRegister(context, gChannelToRP188DBBRegisterNum[channel]);
			LTCIn->Low = ntv2ReadRegister(context, gChannelToRP188Bits031RegisterNum[channel]);
			LTCIn->High = ntv2ReadRegister(context, gChannelToRP188Bits3263RegisterNum[channel]);
		}
		break;
	case 0x0001:
		ntv2ReadRegisterMS(context, gChannelToRP188DBBRegisterNum[channel], &receivedLTC, BIT(18), 18);
		if (receivedLTC == 0 && channel == NTV2_CHANNEL1)
			ntv2ReadRegisterMS(context, 95, &receivedLTC, BIT(9), 9);
		if (receivedLTC == 1)
		{
			LTCIn->DBB = ntv2ReadRegister(context, gChannelToRP188DBBRegisterNum[channel]);
			LTCIn->DBB |= BIT(18);
			LTCIn->Low = ntv2ReadRegister(context, gChannelToLTCEmbeddedBits031RegisterNum[channel]);
			LTCIn->High = ntv2ReadRegister(context, gChannelToLTCEmbeddedBits3263RegisterNum[channel]);
		}
		ntv2ReadRegisterMS(context, gChannelToRP188DBBRegisterNum[channel], &receivedVITC1, BIT(17), 17);
		if (receivedVITC1 == 1)
		{
			VITC1In->DBB = ntv2ReadRegister(context, gChannelToRP188DBBRegisterNum[channel]);
			VITC1In->Low = ntv2ReadRegister(context, gChannelToRP188Bits031RegisterNum[channel]);
			VITC1In->High = ntv2ReadRegister(context, gChannelToRP188Bits3263RegisterNum[channel]);
		}
		break;
	default:
		//Could have LTC so lets check
		ntv2ReadRegisterMS(context, gChannelToRP188DBBRegisterNum[channel], &receivedLTC, BIT(18), 18);
		if (receivedLTC == 0 && channel == NTV2_CHANNEL1)
			ntv2ReadRegisterMS(context, 95, &receivedLTC, BIT(9), 9);
		if (receivedLTC == 1)
		{
			LTCIn->DBB = ntv2ReadRegister(context, gChannelToRP188DBBRegisterNum[channel]);
			LTCIn->DBB |= BIT(18);
			LTCIn->Low = ntv2ReadRegister(context, gChannelToLTCEmbeddedBits031RegisterNum[channel]);
			LTCIn->High = ntv2ReadRegister(context, gChannelToLTCEmbeddedBits3263RegisterNum[channel]);
		}

		//Could have VITC1/VITC2 so lets check
		ntv2ReadRegisterMS(context, gChannelToRP188DBBRegisterNum[channel], &receivedVITC1, BIT(19), 19);
		ntv2ReadRegisterMS(context, gChannelToRP188DBBRegisterNum[channel], &receivedVITC2, BIT(17), 17);
		if (receivedVITC1 == 1 && receivedVITC2 == 1)
		{
			VITC1In->DBB = ntv2ReadRegister(context, gChannelToRP188DBBRegisterNum[channel]);
			VITC1In->Low = ntv2ReadRegister(context, gChannelToRP188Bits031_2RegisterNum[channel]);
			VITC1In->High = ntv2ReadRegister(context, gChannelToRP188Bits3263_2RegisterNum[channel]);

			VITC2In->DBB = ntv2ReadRegister(context, gChannelToRP188DBBRegisterNum[channel]);
			VITC2In->Low = ntv2ReadRegister(context, gChannelToRP188Bits031RegisterNum[channel]);
			VITC2In->High = ntv2ReadRegister(context, gChannelToRP188Bits3263RegisterNum[channel]);
		}
		else
		{
			if (receivedVITC1 == 1)
			{
				VITC1In->DBB = ntv2ReadRegister(context, gChannelToRP188DBBRegisterNum[channel]);
				VITC1In->Low = ntv2ReadRegister(context, gChannelToRP188Bits031_2RegisterNum[channel]);
				VITC1In->High = ntv2ReadRegister(context, gChannelToRP188Bits3263_2RegisterNum[channel]);
			}
			if (receivedVITC2 == 1)
			{
				if (!NTV2_VIDEO_FORMAT_HAS_PROGRESSIVE_PICTURE(channelFormat))
				{
					VITC2In->DBB = ntv2ReadRegister(context, gChannelToRP188DBBRegisterNum[channel]);
					VITC2In->Low = ntv2ReadRegister(context, gChannelToRP188Bits031RegisterNum[channel]);
					VITC2In->High = ntv2ReadRegister(context, gChannelToRP188Bits3263RegisterNum[channel]);
				}
				else
				{
					VITC1In->DBB = ntv2ReadRegister(context, gChannelToRP188DBBRegisterNum[channel]);
					VITC1In->Low = ntv2ReadRegister(context, gChannelToRP188Bits031RegisterNum[channel]);
					VITC1In->High = ntv2ReadRegister(context, gChannelToRP188Bits3263RegisterNum[channel]);
				}
			}
		}
	}
	return true;
}

bool GetReceivedAnalogLTC(Ntv2SystemContext* context, RP188_STRUCT* LTCAnalog1In, RP188_STRUCT* LTCAnalog2In)
{
	//Check for Analog LTC 1 in multiple locations
	ULWord receivedAnalogLTC1 = 0, receivedAnalogLTC2 = 0;
	ntv2ReadRegisterMS(context, kRegLTCStatusControl, &receivedAnalogLTC1, BIT(0), 0);
	if (receivedAnalogLTC1 == 0)
		ntv2ReadRegisterMS(context, kRegStatus, &receivedAnalogLTC1, BIT(17), 17);
	if (receivedAnalogLTC1 == 0)
		ntv2ReadRegisterMS(context, kRegFS1ReferenceSelect, &receivedAnalogLTC1, BIT(6), 6);
	if (receivedAnalogLTC1 == 1 && LTCAnalog1In != NULL)
	{
		LTCAnalog1In->DBB = 0x0;
		LTCAnalog1In->Low = ntv2ReadRegister(context, kRegLTCAnalogBits0_31);
		LTCAnalog1In->High = ntv2ReadRegister(context, kRegLTCAnalogBits32_63);
	}

	ntv2ReadRegisterMS(context, kRegLTCStatusControl, &receivedAnalogLTC2, BIT(8), 8);
	if (receivedAnalogLTC2 == 1 && LTCAnalog2In != NULL)
	{
		LTCAnalog2In->DBB = 0x0;
		LTCAnalog2In->Low = ntv2ReadRegister(context, kRegLTC2AnalogBits0_31);
		LTCAnalog2In->High = ntv2ReadRegister(context, kRegLTC2AnalogBits32_63);

	}
	return true;
}

bool CopyFrameStampSDIStatusArrayToNTV2SDIStatusArray(INTERNAL_SDI_STATUS_STRUCT * sdiStruct, NTV2SDIInputStatus * pOutStatusArray, ULWord inMaxBytes)
{
	const ULWord maxNumElements = inMaxBytes / sizeof(NTV2SDIInputStatus);
	if (!pOutStatusArray)
		return false;
	if (!maxNumElements)
		return false;
	if (maxNumElements < NTV2_MAX_NUM_CHANNELS)
		return false;	//	It's all or nothing

	pOutStatusArray[NTV2_CHANNEL1] = sdiStruct->SDIStatus1;
	pOutStatusArray[NTV2_CHANNEL2] = sdiStruct->SDIStatus2;
	pOutStatusArray[NTV2_CHANNEL3] = sdiStruct->SDIStatus3;
	pOutStatusArray[NTV2_CHANNEL4] = sdiStruct->SDIStatus4;
	pOutStatusArray[NTV2_CHANNEL5] = sdiStruct->SDIStatus5;
	pOutStatusArray[NTV2_CHANNEL6] = sdiStruct->SDIStatus6;
	pOutStatusArray[NTV2_CHANNEL7] = sdiStruct->SDIStatus7;
	pOutStatusArray[NTV2_CHANNEL8] = sdiStruct->SDIStatus8;
	return true;
}	//	CopyFrameStampSDIStatusArrayToNTV2SDIStatusArray


bool CopySDIStatusHardwareToFrameStampSDIStatusArray(Ntv2SystemContext* context, INTERNAL_SDI_STATUS_STRUCT* sdiStruct)
{
	ULWord countHi = 0, countLo = 0;
	ULWord isLocked = 0, VPIDValid = 0, trsError = 0;
	NTV2DeviceID deviceID = (NTV2DeviceID)ntv2ReadRegister(context, kRegBoardID);
	ULWord numSDIInputs = NTV2DeviceGetNumVideoInputs(deviceID);

	if (sdiStruct == NULL)
	{
		return false;
	}

	switch (numSDIInputs)
	{
	case 8:
		ntv2ReadRegisterMS(context, gChannelToRXSDIStatusRegs[NTV2_CHANNEL8], &sdiStruct->SDIStatus8.mUnlockTally, kRegMaskSDIInUnlockCount, kRegShiftSDIInUnlockCount);
		ntv2ReadRegisterMS(context, gChannelToRXSDIStatusRegs[NTV2_CHANNEL8], &isLocked, kRegMaskSDIInLocked, kRegShiftSDIInLocked);
		sdiStruct->SDIStatus8.mLocked = isLocked ? true : false;
		isLocked = 0;
		ntv2ReadRegisterMS(context, gChannelToRXSDIStatusRegs[NTV2_CHANNEL8], &VPIDValid, kRegMaskSDIInVpidValidA, kRegShiftSDIInVpidValidA);
		sdiStruct->SDIStatus8.mVPIDValidA = VPIDValid ? true : false;
		VPIDValid = 0;
		ntv2ReadRegisterMS(context, gChannelToRXSDIStatusRegs[NTV2_CHANNEL8], &VPIDValid, kRegMaskSDIInVpidValidB, kRegShiftSDIInVpidValidB);
		sdiStruct->SDIStatus8.mVPIDValidB = VPIDValid ? true : false;
		VPIDValid = 0;
		ntv2ReadRegisterMS(context, gChannelToRXSDIStatusRegs[NTV2_CHANNEL8], &trsError, kRegMaskSDIInTRSError, kRegShiftSDIInTRSError);
		sdiStruct->SDIStatus8.mFrameTRSError = trsError ? true : false;
		trsError = 0;
		ntv2ReadRegisterMS(context, gChannelToRXSDICRCErrorCountRegs[NTV2_CHANNEL8], (ULWord*)&sdiStruct->SDIStatus8.mCRCTallyA, kRegMaskSDIInCRCErrorCountA, kRegShiftSDIInCRCErrorCountA);
		ntv2ReadRegisterMS(context, gChannelToRXSDICRCErrorCountRegs[NTV2_CHANNEL8], (ULWord*)&sdiStruct->SDIStatus8.mCRCTallyB, (ULWord)kRegMaskSDIInCRCErrorCountB, kRegShiftSDIInCRCErrorCountB);
		countHi = ntv2ReadRegister(context, gChannelToRXSDIFrameRefCountHiRegs[NTV2_CHANNEL8]);
		countLo = ntv2ReadRegister(context, gChannelToRXSDIFrameRefCountLoRegs[NTV2_CHANNEL8]);
		sdiStruct->SDIStatus8.mGlobalClockCount = (uint64_t)countHi << 32 | countLo;
		countHi = 0;
		countLo = 0;
		// fall through
	case 7:
		ntv2ReadRegisterMS(context, gChannelToRXSDIStatusRegs[NTV2_CHANNEL7], &sdiStruct->SDIStatus7.mUnlockTally, kRegMaskSDIInUnlockCount, kRegShiftSDIInUnlockCount);
		ntv2ReadRegisterMS(context, gChannelToRXSDIStatusRegs[NTV2_CHANNEL7], &isLocked, kRegMaskSDIInLocked, kRegShiftSDIInLocked);
		sdiStruct->SDIStatus7.mLocked = isLocked ? true : false;
		ntv2ReadRegisterMS(context, gChannelToRXSDIStatusRegs[NTV2_CHANNEL7], &VPIDValid, kRegMaskSDIInVpidValidA, kRegShiftSDIInVpidValidA);
		sdiStruct->SDIStatus7.mVPIDValidA = VPIDValid ? true : false;
		ntv2ReadRegisterMS(context, gChannelToRXSDIStatusRegs[NTV2_CHANNEL7], &VPIDValid, kRegMaskSDIInVpidValidB, kRegShiftSDIInVpidValidB);
		sdiStruct->SDIStatus7.mVPIDValidB = VPIDValid ? true : false;
		ntv2ReadRegisterMS(context, gChannelToRXSDIStatusRegs[NTV2_CHANNEL7], &trsError, kRegMaskSDIInTRSError, kRegShiftSDIInTRSError);
		sdiStruct->SDIStatus7.mFrameTRSError = trsError ? true : false;
		ntv2ReadRegisterMS(context, gChannelToRXSDICRCErrorCountRegs[NTV2_CHANNEL7], (ULWord*)&sdiStruct->SDIStatus7.mCRCTallyA, kRegMaskSDIInCRCErrorCountA, kRegShiftSDIInCRCErrorCountA);
		ntv2ReadRegisterMS(context, gChannelToRXSDICRCErrorCountRegs[NTV2_CHANNEL7], (ULWord*)&sdiStruct->SDIStatus7.mCRCTallyB, (ULWord)kRegMaskSDIInCRCErrorCountB, kRegShiftSDIInCRCErrorCountB);
		countHi = ntv2ReadRegister(context, gChannelToRXSDIFrameRefCountHiRegs[NTV2_CHANNEL7]);
		countLo = ntv2ReadRegister(context, gChannelToRXSDIFrameRefCountLoRegs[NTV2_CHANNEL7]);
		sdiStruct->SDIStatus7.mGlobalClockCount = (uint64_t)countHi << 32 | countLo;
		// fall through
	case 6:
		ntv2ReadRegisterMS(context, gChannelToRXSDIStatusRegs[NTV2_CHANNEL6], &sdiStruct->SDIStatus6.mUnlockTally, kRegMaskSDIInUnlockCount, kRegShiftSDIInUnlockCount);
		ntv2ReadRegisterMS(context, gChannelToRXSDIStatusRegs[NTV2_CHANNEL6], &isLocked, kRegMaskSDIInLocked, kRegShiftSDIInLocked);
		sdiStruct->SDIStatus6.mLocked = isLocked ? true : false;
		ntv2ReadRegisterMS(context, gChannelToRXSDIStatusRegs[NTV2_CHANNEL6], &VPIDValid, kRegMaskSDIInVpidValidA, kRegShiftSDIInVpidValidA);
		sdiStruct->SDIStatus6.mVPIDValidA = VPIDValid ? true : false;
		ntv2ReadRegisterMS(context, gChannelToRXSDIStatusRegs[NTV2_CHANNEL6], &VPIDValid, kRegMaskSDIInVpidValidB, kRegShiftSDIInVpidValidB);
		sdiStruct->SDIStatus6.mVPIDValidB = VPIDValid ? true : false;
		ntv2ReadRegisterMS(context, gChannelToRXSDIStatusRegs[NTV2_CHANNEL6], &trsError, kRegMaskSDIInTRSError, kRegShiftSDIInTRSError);
		sdiStruct->SDIStatus6.mFrameTRSError = trsError ? true : false;
		ntv2ReadRegisterMS(context, gChannelToRXSDICRCErrorCountRegs[NTV2_CHANNEL6], (ULWord*)&sdiStruct->SDIStatus6.mCRCTallyA, kRegMaskSDIInCRCErrorCountA, kRegShiftSDIInCRCErrorCountA);
		ntv2ReadRegisterMS(context, gChannelToRXSDICRCErrorCountRegs[NTV2_CHANNEL6], (ULWord*)&sdiStruct->SDIStatus6.mCRCTallyB, (ULWord)kRegMaskSDIInCRCErrorCountB, kRegShiftSDIInCRCErrorCountB);
		countHi = ntv2ReadRegister(context, gChannelToRXSDIFrameRefCountHiRegs[NTV2_CHANNEL6]);
		countLo = ntv2ReadRegister(context, gChannelToRXSDIFrameRefCountLoRegs[NTV2_CHANNEL6]);
		sdiStruct->SDIStatus6.mGlobalClockCount = (uint64_t)countHi << 32 | countLo;
		// fall through
	case 5:
		ntv2ReadRegisterMS(context, gChannelToRXSDIStatusRegs[NTV2_CHANNEL5], &sdiStruct->SDIStatus5.mUnlockTally, kRegMaskSDIInUnlockCount, kRegShiftSDIInUnlockCount);
		ntv2ReadRegisterMS(context, gChannelToRXSDIStatusRegs[NTV2_CHANNEL5], &isLocked, kRegMaskSDIInLocked, kRegShiftSDIInLocked);
		sdiStruct->SDIStatus5.mLocked = isLocked ? true : false;
		ntv2ReadRegisterMS(context, gChannelToRXSDIStatusRegs[NTV2_CHANNEL5], &VPIDValid, kRegMaskSDIInVpidValidA, kRegShiftSDIInVpidValidA);
		sdiStruct->SDIStatus5.mVPIDValidA = VPIDValid ? true : false;
		ntv2ReadRegisterMS(context, gChannelToRXSDIStatusRegs[NTV2_CHANNEL5], &VPIDValid, kRegMaskSDIInVpidValidB, kRegShiftSDIInVpidValidB);
		sdiStruct->SDIStatus5.mVPIDValidB = VPIDValid ? true : false;
		ntv2ReadRegisterMS(context, gChannelToRXSDIStatusRegs[NTV2_CHANNEL5], &trsError, kRegMaskSDIInTRSError, kRegShiftSDIInTRSError);
		sdiStruct->SDIStatus5.mFrameTRSError = trsError ? true : false;
		ntv2ReadRegisterMS(context, gChannelToRXSDICRCErrorCountRegs[NTV2_CHANNEL5], (ULWord*)&sdiStruct->SDIStatus5.mCRCTallyA, kRegMaskSDIInCRCErrorCountA, kRegShiftSDIInCRCErrorCountA);
		ntv2ReadRegisterMS(context, gChannelToRXSDICRCErrorCountRegs[NTV2_CHANNEL5], (ULWord*)&sdiStruct->SDIStatus5.mCRCTallyB, (ULWord)kRegMaskSDIInCRCErrorCountB, kRegShiftSDIInCRCErrorCountB);
		countHi = ntv2ReadRegister(context, gChannelToRXSDIFrameRefCountHiRegs[NTV2_CHANNEL5]);
		countLo = ntv2ReadRegister(context, gChannelToRXSDIFrameRefCountLoRegs[NTV2_CHANNEL5]);
		sdiStruct->SDIStatus5.mGlobalClockCount = (uint64_t)countHi << 32 | countLo;
		// fall through
	case 4:
		ntv2ReadRegisterMS(context, gChannelToRXSDIStatusRegs[NTV2_CHANNEL4], &sdiStruct->SDIStatus4.mUnlockTally, kRegMaskSDIInUnlockCount, kRegShiftSDIInUnlockCount);
		ntv2ReadRegisterMS(context, gChannelToRXSDIStatusRegs[NTV2_CHANNEL4], &isLocked, kRegMaskSDIInLocked, kRegShiftSDIInLocked);
		sdiStruct->SDIStatus4.mLocked = isLocked ? true : false;
		ntv2ReadRegisterMS(context, gChannelToRXSDIStatusRegs[NTV2_CHANNEL4], &VPIDValid, kRegMaskSDIInVpidValidA, kRegShiftSDIInVpidValidA);
		sdiStruct->SDIStatus4.mVPIDValidA = VPIDValid ? true : false;
		ntv2ReadRegisterMS(context, gChannelToRXSDIStatusRegs[NTV2_CHANNEL4], &VPIDValid, kRegMaskSDIInVpidValidB, kRegShiftSDIInVpidValidB);
		sdiStruct->SDIStatus4.mVPIDValidB = VPIDValid ? true : false;
		ntv2ReadRegisterMS(context, gChannelToRXSDIStatusRegs[NTV2_CHANNEL4], &trsError, kRegMaskSDIInTRSError, kRegShiftSDIInTRSError);
		sdiStruct->SDIStatus4.mFrameTRSError = trsError ? true : false;
		ntv2ReadRegisterMS(context, gChannelToRXSDICRCErrorCountRegs[NTV2_CHANNEL4], (ULWord*)&sdiStruct->SDIStatus4.mCRCTallyA, kRegMaskSDIInCRCErrorCountA, kRegShiftSDIInCRCErrorCountA);
		ntv2ReadRegisterMS(context, gChannelToRXSDICRCErrorCountRegs[NTV2_CHANNEL4], (ULWord*)&sdiStruct->SDIStatus4.mCRCTallyB, (ULWord)kRegMaskSDIInCRCErrorCountB, kRegShiftSDIInCRCErrorCountB);
		countHi = ntv2ReadRegister(context, gChannelToRXSDIFrameRefCountHiRegs[NTV2_CHANNEL4]);
		countLo = ntv2ReadRegister(context, gChannelToRXSDIFrameRefCountLoRegs[NTV2_CHANNEL4]);
		sdiStruct->SDIStatus4.mGlobalClockCount = (uint64_t)countHi << 32 | countLo;
		// fall through
	case 3:
		ntv2ReadRegisterMS(context, gChannelToRXSDIStatusRegs[NTV2_CHANNEL3], &sdiStruct->SDIStatus3.mUnlockTally, kRegMaskSDIInUnlockCount, kRegShiftSDIInUnlockCount);
		ntv2ReadRegisterMS(context, gChannelToRXSDIStatusRegs[NTV2_CHANNEL3], &isLocked, kRegMaskSDIInLocked, kRegShiftSDIInLocked);
		sdiStruct->SDIStatus3.mLocked = isLocked ? true : false;
		ntv2ReadRegisterMS(context, gChannelToRXSDIStatusRegs[NTV2_CHANNEL3], &VPIDValid, kRegMaskSDIInVpidValidA, kRegShiftSDIInVpidValidA);
		sdiStruct->SDIStatus3.mVPIDValidA = VPIDValid ? true : false;
		ntv2ReadRegisterMS(context, gChannelToRXSDIStatusRegs[NTV2_CHANNEL3], &VPIDValid, kRegMaskSDIInVpidValidB, kRegShiftSDIInVpidValidB);
		sdiStruct->SDIStatus3.mVPIDValidB = VPIDValid ? true : false;
		ntv2ReadRegisterMS(context, gChannelToRXSDIStatusRegs[NTV2_CHANNEL3], &trsError, kRegMaskSDIInTRSError, kRegShiftSDIInTRSError);
		sdiStruct->SDIStatus3.mFrameTRSError = trsError ? true : false;
		ntv2ReadRegisterMS(context, gChannelToRXSDICRCErrorCountRegs[NTV2_CHANNEL3], (ULWord*)&sdiStruct->SDIStatus3.mCRCTallyA, kRegMaskSDIInCRCErrorCountA, kRegShiftSDIInCRCErrorCountA);
		ntv2ReadRegisterMS(context, gChannelToRXSDICRCErrorCountRegs[NTV2_CHANNEL3], (ULWord*)&sdiStruct->SDIStatus3.mCRCTallyB, (ULWord)kRegMaskSDIInCRCErrorCountB, kRegShiftSDIInCRCErrorCountB);
		countHi = ntv2ReadRegister(context, gChannelToRXSDIFrameRefCountHiRegs[NTV2_CHANNEL3]);
		countLo = ntv2ReadRegister(context, gChannelToRXSDIFrameRefCountLoRegs[NTV2_CHANNEL3]);
		sdiStruct->SDIStatus3.mGlobalClockCount = (uint64_t)countHi << 32 | countLo;
		// fall through
	case 2:
		ntv2ReadRegisterMS(context, gChannelToRXSDIStatusRegs[NTV2_CHANNEL2], &sdiStruct->SDIStatus2.mUnlockTally, kRegMaskSDIInUnlockCount, kRegShiftSDIInUnlockCount);
		ntv2ReadRegisterMS(context, gChannelToRXSDIStatusRegs[NTV2_CHANNEL2], &isLocked, kRegMaskSDIInLocked, kRegShiftSDIInLocked);
		sdiStruct->SDIStatus2.mLocked = isLocked ? true : false;
		ntv2ReadRegisterMS(context, gChannelToRXSDIStatusRegs[NTV2_CHANNEL2], &VPIDValid, kRegMaskSDIInVpidValidA, kRegShiftSDIInVpidValidA);
		sdiStruct->SDIStatus2.mVPIDValidA = VPIDValid ? true : false;
		ntv2ReadRegisterMS(context, gChannelToRXSDIStatusRegs[NTV2_CHANNEL2], &VPIDValid, kRegMaskSDIInVpidValidB, kRegShiftSDIInVpidValidB);
		sdiStruct->SDIStatus2.mVPIDValidB = VPIDValid ? true : false;
		ntv2ReadRegisterMS(context, gChannelToRXSDIStatusRegs[NTV2_CHANNEL2], &trsError, kRegMaskSDIInTRSError, kRegShiftSDIInTRSError);
		sdiStruct->SDIStatus2.mFrameTRSError = trsError ? true : false;
		ntv2ReadRegisterMS(context, gChannelToRXSDICRCErrorCountRegs[NTV2_CHANNEL2], (ULWord*)&sdiStruct->SDIStatus2.mCRCTallyA, kRegMaskSDIInCRCErrorCountA, kRegShiftSDIInCRCErrorCountA);
		ntv2ReadRegisterMS(context, gChannelToRXSDICRCErrorCountRegs[NTV2_CHANNEL2], (ULWord*)&sdiStruct->SDIStatus2.mCRCTallyB, (ULWord)kRegMaskSDIInCRCErrorCountB, kRegShiftSDIInCRCErrorCountB);
		countHi = ntv2ReadRegister(context, gChannelToRXSDIFrameRefCountHiRegs[NTV2_CHANNEL2]);
		countLo = ntv2ReadRegister(context, gChannelToRXSDIFrameRefCountLoRegs[NTV2_CHANNEL2]);
		sdiStruct->SDIStatus2.mGlobalClockCount = (uint64_t)countHi << 32 | countLo;
		// fall through
	case 1:
		ntv2ReadRegisterMS(context, gChannelToRXSDIStatusRegs[NTV2_CHANNEL1], &sdiStruct->SDIStatus1.mUnlockTally, kRegMaskSDIInUnlockCount, kRegShiftSDIInUnlockCount);
		ntv2ReadRegisterMS(context, gChannelToRXSDIStatusRegs[NTV2_CHANNEL1], &isLocked, kRegMaskSDIInLocked, kRegShiftSDIInLocked);
		sdiStruct->SDIStatus1.mLocked = isLocked ? true : false;
		ntv2ReadRegisterMS(context, gChannelToRXSDIStatusRegs[NTV2_CHANNEL1], &VPIDValid, kRegMaskSDIInVpidValidA, kRegShiftSDIInVpidValidA);
		sdiStruct->SDIStatus1.mVPIDValidA = VPIDValid ? true : false;
		ntv2ReadRegisterMS(context, gChannelToRXSDIStatusRegs[NTV2_CHANNEL1], &VPIDValid, kRegMaskSDIInVpidValidB, kRegShiftSDIInVpidValidB);
		sdiStruct->SDIStatus1.mVPIDValidB = VPIDValid ? true : false;
		ntv2ReadRegisterMS(context, gChannelToRXSDIStatusRegs[NTV2_CHANNEL1], &trsError, kRegMaskSDIInTRSError, kRegShiftSDIInTRSError);
		sdiStruct->SDIStatus1.mFrameTRSError = trsError ? true : false;
		ntv2ReadRegisterMS(context, gChannelToRXSDICRCErrorCountRegs[NTV2_CHANNEL1], (ULWord*)&sdiStruct->SDIStatus1.mCRCTallyA, kRegMaskSDIInCRCErrorCountA, kRegShiftSDIInCRCErrorCountA);
		ntv2ReadRegisterMS(context, gChannelToRXSDICRCErrorCountRegs[NTV2_CHANNEL1], (ULWord*)&sdiStruct->SDIStatus1.mCRCTallyB, (ULWord)kRegMaskSDIInCRCErrorCountB, kRegShiftSDIInCRCErrorCountB);
		countHi = ntv2ReadRegister(context, gChannelToRXSDIFrameRefCountHiRegs[NTV2_CHANNEL1]);
		countLo = ntv2ReadRegister(context, gChannelToRXSDIFrameRefCountLoRegs[NTV2_CHANNEL1]);
		sdiStruct->SDIStatus1.mGlobalClockCount = (uint64_t)countHi << 32 | countLo;
		// fall through
	default:
		break;
	}
	return true;
}

bool CopyFrameRP188ToHardware(Ntv2SystemContext* context, RP188_STRUCT* rp188)
{
	UWord chan = 0;
	NTV2DeviceID deviceID = (NTV2DeviceID)ntv2ReadRegister(context, kRegBoardID);
	
	if (RP188_IS_VALID(*rp188) == false)
		return true;

	for (chan = 0; chan < NTV2DeviceGetNumVideoOutputs(deviceID); chan++)
	{
		SetRP188Mode(context, (NTV2Channel)chan, NTV2_RP188_OUTPUT);
		
		if (ntv2ReadVirtualRegister(context, kVRegUserDefinedDBB) == 0)
		{
			if (NTV2_IS_SD_VIDEO_FORMAT(GetBoardVideoFormat(context, (NTV2Channel)chan)) || deviceID == DEVICE_ID_KONALHI)
			{
				ntv2WriteRegisterMS(context, gChannelToRP188DBBRegisterNum[chan], 0x00, kRegMaskRP188DBB, kRegShiftRP188DBB);
			}
			else
			{
				ntv2WriteRegisterMS(context, gChannelToRP188DBBRegisterNum[chan], 0xFF, kRegMaskRP188DBB, kRegShiftRP188DBB);
			}
		}
		else
		{
			ntv2WriteRegisterMS(context, gChannelToRP188DBBRegisterNum[chan], rp188->DBB, kRegMaskRP188DBB, kRegShiftRP188DBB);
		}
		
		ntv2WriteRegister(context, gChannelToRP188Bits031RegisterNum[chan], rp188->Low);
		ntv2WriteRegister(context, gChannelToRP188Bits3263RegisterNum[chan], rp188->High);
		
		if (NTV2DeviceCanDoLTCEmbeddedN(deviceID, chan))
		{
			ntv2WriteRegister(context, gChannelToLTCEmbeddedBits031RegisterNum[chan], rp188->Low);
			ntv2WriteRegister(context, gChannelToLTCEmbeddedBits3263RegisterNum[chan], rp188->High);
		}
	}
	
	if (NTV2DeviceCanDoWidget(deviceID, NTV2_WgtSDIMonOut1))
	{
		SetRP188Mode(context, NTV2_CHANNEL5, NTV2_RP188_OUTPUT);
		
		if (ntv2ReadVirtualRegister(context, kVRegUserDefinedDBB) == 0)
		{
			if (NTV2_IS_SD_VIDEO_FORMAT(GetBoardVideoFormat(context, (NTV2Channel)chan)))
			{
				ntv2WriteRegisterMS(context, gChannelToRP188DBBRegisterNum[NTV2_CHANNEL5], 0x00, kRegMaskRP188DBB, kRegShiftRP188DBB);
			}
			else
			{
				ntv2WriteRegisterMS(context, gChannelToRP188DBBRegisterNum[NTV2_CHANNEL5], 0xFF, kRegMaskRP188DBB, kRegShiftRP188DBB);
			}
		}
		else
		{
			ntv2WriteRegisterMS(context, gChannelToRP188DBBRegisterNum[NTV2_CHANNEL5], rp188->DBB, kRegMaskRP188DBB, kRegShiftRP188DBB);
		}
		
		ntv2WriteRegister(context, gChannelToRP188Bits031RegisterNum[NTV2_CHANNEL5], rp188->Low);
		ntv2WriteRegister(context, gChannelToRP188Bits3263RegisterNum[NTV2_CHANNEL5], rp188->High);
		
		ntv2WriteRegister(context, gChannelToLTCEmbeddedBits031RegisterNum[NTV2_CHANNEL5], rp188->Low);
		ntv2WriteRegister(context, gChannelToLTCEmbeddedBits3263RegisterNum[NTV2_CHANNEL5], rp188->High);
	}
	
	if (NTV2DeviceCanDoLTCOutN(deviceID, NTV2_CHANNEL1))
	{
		ntv2WriteRegister(context, kRegLTCAnalogBits0_31, rp188->Low);
		ntv2WriteRegister(context, kRegLTCAnalogBits32_63, rp188->High);
	}
	
	if (NTV2DeviceCanDoLTCOutN(deviceID, NTV2_CHANNEL2))
	{
		ntv2WriteRegister(context, kRegLTC2AnalogBits0_31, rp188->Low);
		ntv2WriteRegister(context, kRegLTC2AnalogBits32_63, rp188->High);
	}
	
	return true;
}
