/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
////////////////////////////////////////////////////////////
//
// Filename: ntv2xpt.h
// Purpose:	 Common XPT
//
///////////////////////////////////////////////////////////////

#ifndef NTV2XPT_HEADER
#define NTV2XPT_HEADER

#include "ntv2system.h"
#include "ntv2xptlookup.h"
#include "ntv2kona.h"


bool FindSDIOutputSource(Ntv2SystemContext* context, NTV2OutputXptID* source, NTV2Channel channel);
bool FindAnalogOutputSource(Ntv2SystemContext* context, NTV2OutputXptID* source);
bool FindHDMIOutputSource(Ntv2SystemContext* context, NTV2OutputXptID* source, NTV2Channel channel);

bool FindCrosspointSource(Ntv2SystemContext* context, NTV2OutputXptID* source, NTV2OutputXptID crosspoint);
NTV2XptLookupEntry GetCrosspointIDInput(NTV2OutputXptID crosspointID);

bool GetXptSDIOutInputSelect(Ntv2SystemContext* context, NTV2Channel channel, NTV2OutputXptID* value);
bool GetXptSDIOutDS2InputSelect(Ntv2SystemContext* context, NTV2Channel channel, NTV2OutputXptID* value);
bool SetXptSDIOutInputSelect(Ntv2SystemContext* context, NTV2Channel channel, NTV2OutputXptID value);
bool GetXptConversionModInputSelect(Ntv2SystemContext* context, NTV2OutputXptID* value);
bool GetXptDuallinkInInputSelect(Ntv2SystemContext* context, NTV2Channel channel, NTV2OutputXptID* value);
bool GetXptAnalogOutInputSelect(Ntv2SystemContext* context, NTV2OutputXptID* value);
bool GetXptFrameBuffer1InputSelect(Ntv2SystemContext* context, NTV2OutputXptID* value);
bool GetXptFrameBuffer2InputSelect(Ntv2SystemContext* context, NTV2OutputXptID* value);
bool GetXptHDMIOutInputSelect(Ntv2SystemContext* context, NTV2OutputXptID* value);
bool GetXptHDMIOutQ2InputSelect(Ntv2SystemContext* context, NTV2OutputXptID* value);
bool GetXptHDMIOutQ3InputSelect(Ntv2SystemContext* context, NTV2OutputXptID* value);
bool GetXptHDMIOutQ4InputSelect(Ntv2SystemContext* context, NTV2OutputXptID* value);
bool GetXptMultiLinkOutInputSelect(Ntv2SystemContext* context, NTV2Channel channel, NTV2OutputXptID* value);

#endif
