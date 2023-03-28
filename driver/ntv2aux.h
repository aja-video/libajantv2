/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
////////////////////////////////////////////////////////////
//
// Filename: ntv2aux.h
// Purpose:	 Common HDMI aux
//
///////////////////////////////////////////////////////////////

#ifndef NTV2AUX_HEADER
#define NTV2AUX_HEADER

#include "ntv2kona.h"

bool SetupAuxExtractor(Ntv2SystemContext* context, NTV2Channel channel);
bool EnableAuxExtractor(Ntv2SystemContext* context, NTV2Channel channel, bool bEnable);
bool SetAuxExtWriteParams(Ntv2SystemContext* context, NTV2Channel channel, ULWord frameNumber);
bool SetAuxExtField2WriteParams(Ntv2SystemContext* context, NTV2Channel channel, ULWord frameNumber);
bool SetAuxExtProgressive(Ntv2SystemContext* context, NTV2Channel channel, bool bEnable);
bool SetAuxExtSynchro(Ntv2SystemContext* context, NTV2Channel channel);
bool SetAuxExtField1StartAddr(Ntv2SystemContext* context, NTV2Channel channel, ULWord addr);
bool SetAuxExtField1EndAddr(Ntv2SystemContext* context, NTV2Channel channel, ULWord addr);
bool SetAuxExtField2StartAddr(Ntv2SystemContext* context, NTV2Channel channel, ULWord addr);
bool SetAuxExtField2EndAddr(Ntv2SystemContext* context, NTV2Channel channel, ULWord addr);
bool SetAuxExtField1CutoffLine(Ntv2SystemContext* context, NTV2Channel channel, ULWord lineNumber);
bool SetAuxExtField2CutoffLine(Ntv2SystemContext* context, NTV2Channel channel, ULWord lineNumber);
bool IsAuxExtOverrun(Ntv2SystemContext* context, NTV2Channel channel);
ULWord GetAuxExtField1Bytes(Ntv2SystemContext* context, NTV2Channel channel);
bool IsAuxExtField1Overrun(Ntv2SystemContext* context, NTV2Channel channel);
ULWord GetAuxExtField2Bytes(Ntv2SystemContext* context, NTV2Channel channel);
bool IsAuxExtField2Overrun(Ntv2SystemContext* context, NTV2Channel channel);
bool SetAuxExtField1StartLine(Ntv2SystemContext* context, NTV2Channel channel, ULWord lineNumber);
bool SetAuxExtField2StartLine(Ntv2SystemContext* context, NTV2Channel channel, ULWord lineNumber);
bool SetAuxExtTotalFrameLines(Ntv2SystemContext* context, NTV2Channel channel, ULWord totalFrameLines);
bool SetAuxExtFidLow(Ntv2SystemContext* context, NTV2Channel channel, ULWord lineNumber);
bool SetAuxExtFidHi(Ntv2SystemContext* context, NTV2Channel channel, ULWord lineNumber);

#endif

