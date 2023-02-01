/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
////////////////////////////////////////////////////////////
//
// Filename: ntv2rp188.h
// Purpose:	 Common RP188
//
///////////////////////////////////////////////////////////////

#ifndef NTV2RP188_HEADER
#define NTV2RP188_HEADER

#include "ntv2kona.h"

typedef struct {
	RP188_STRUCT TCInOut1;
	RP188_STRUCT TCInOut2;
	RP188_STRUCT TCInOut3;
	RP188_STRUCT TCInOut4;
	RP188_STRUCT LTCEmbedded1;
	RP188_STRUCT LTCAnalog1;
	RP188_STRUCT LTCEmbedded2;
	RP188_STRUCT LTCAnalog2;
	RP188_STRUCT TCInOut5;
	RP188_STRUCT TCInOut6;
	RP188_STRUCT TCInOut7;
	RP188_STRUCT TCInOut8;
	RP188_STRUCT LTCEmbedded3;
	RP188_STRUCT LTCEmbedded4;
	RP188_STRUCT LTCEmbedded5;
	RP188_STRUCT LTCEmbedded6;
	RP188_STRUCT LTCEmbedded7;
	RP188_STRUCT LTCEmbedded8;
	RP188_STRUCT TCInOut1_2;
	RP188_STRUCT TCInOut2_2;
	RP188_STRUCT TCInOut3_2;
	RP188_STRUCT TCInOut4_2;
	RP188_STRUCT TCInOut5_2;
	RP188_STRUCT TCInOut6_2;
	RP188_STRUCT TCInOut7_2;
	RP188_STRUCT TCInOut8_2;
} INTERNAL_TIMECODE_STRUCT;

typedef struct {
	NTV2SDIInputStatus SDIStatus1;
	NTV2SDIInputStatus SDIStatus2;
	NTV2SDIInputStatus SDIStatus3;
	NTV2SDIInputStatus SDIStatus4;
	NTV2SDIInputStatus SDIStatus5;
	NTV2SDIInputStatus SDIStatus6;
	NTV2SDIInputStatus SDIStatus7;
	NTV2SDIInputStatus SDIStatus8;
} INTERNAL_SDI_STATUS_STRUCT;

bool InitRP188(Ntv2SystemContext* context);
extern bool CopyRP188HardwareToFrameStampTCArray(Ntv2SystemContext* context, INTERNAL_TIMECODE_STRUCT* tcStruct);
bool CopyFrameStampTCArrayToHardware(Ntv2SystemContext* context, INTERNAL_TIMECODE_STRUCT* acFrameStampTCArray);
bool CopyNTV2TimeCodeArrayToFrameStampTCArray(INTERNAL_TIMECODE_STRUCT * tcStruct, NTV2_RP188 * pInTCArray, ULWord inMaxBytes);
bool CopyFrameStampTCArrayToNTV2TimeCodeArray(INTERNAL_TIMECODE_STRUCT * tcStruct, NTV2_RP188 * pOutTCArray, ULWord inMaxBytes);
void SetRP188Mode(Ntv2SystemContext* context, NTV2Channel channel, NTV2_RP188Mode value);
bool GetReceivedTCForChannel(Ntv2SystemContext* context, NTV2Channel channel, RP188_STRUCT* LTCIn, RP188_STRUCT* VITC1In, RP188_STRUCT* VITC2In);
bool GetReceivedAnalogLTC(Ntv2SystemContext* context, RP188_STRUCT* LTCAnalog1In, RP188_STRUCT* LTCAnalog2In);
bool CopyFrameStampSDIStatusArrayToNTV2SDIStatusArray(INTERNAL_SDI_STATUS_STRUCT * tcStruct, NTV2SDIInputStatus * pOutTCArray, ULWord inMaxBytes);
bool CopySDIStatusHardwareToFrameStampSDIStatusArray(Ntv2SystemContext* context, INTERNAL_SDI_STATUS_STRUCT* sdiStruct);
bool CopyFrameRP188ToHardware(Ntv2SystemContext* context, RP188_STRUCT* rp188);
#endif

