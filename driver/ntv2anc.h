/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
////////////////////////////////////////////////////////////
//
// Filename: ntv2anc.h
// Purpose:	 Common RP188
//
///////////////////////////////////////////////////////////////

#ifndef NTV2ANC_HEADER
#define NTV2ANC_HEADER

#include "ntv2kona.h"

bool SetupAncExtractor(Ntv2SystemContext* context, NTV2Channel channel);
bool EnableAncExtractor(Ntv2SystemContext* context, NTV2Channel channel, bool bEnable);
bool SetAncExtWriteParams(Ntv2SystemContext* context, NTV2Channel channel, ULWord frameNumber);
bool SetAncExtField2WriteParams(Ntv2SystemContext* context, NTV2Channel channel, ULWord frameNumber);
bool EnableAncExtHancY(Ntv2SystemContext* context, NTV2Channel channel, bool bEnable);
bool EnableAncExtHancC(Ntv2SystemContext* context, NTV2Channel channel, bool bEnable);
bool EnableAncExtVancY(Ntv2SystemContext* context, NTV2Channel channel, bool bEnable);
bool EnableAncExtVancC(Ntv2SystemContext* context, NTV2Channel channel, bool bEnable);
bool SetAncExtSDDemux(Ntv2SystemContext* context, NTV2Channel channel, bool bEnable);
bool SetAncExtProgressive(Ntv2SystemContext* context, NTV2Channel channel, bool bEnable);
bool SetAncExtSynchro(Ntv2SystemContext* context, NTV2Channel channel);
bool SetAncExtField1StartAddr(Ntv2SystemContext* context, NTV2Channel channel, ULWord addr);
bool SetAncExtLSBEnable(Ntv2SystemContext* context, NTV2Channel channel, bool bEnable);
bool SetAncExtField1StartAddr(Ntv2SystemContext* context, NTV2Channel channel, ULWord addr);
bool SetAncExtField1EndAddr(Ntv2SystemContext* context, NTV2Channel channel, ULWord addr);
bool SetAncExtField2StartAddr(Ntv2SystemContext* context, NTV2Channel channel, ULWord addr);
bool SetAncExtField2EndAddr(Ntv2SystemContext* context, NTV2Channel channel, ULWord addr);
bool SetAncExtField1CutoffLine(Ntv2SystemContext* context, NTV2Channel channel, ULWord lineNumber);
bool SetAncExtField2CutoffLine(Ntv2SystemContext* context, NTV2Channel channel, ULWord lineNumber);
bool IsAncExtOverrun(Ntv2SystemContext* context, NTV2Channel channel);
ULWord GetAncExtField1Bytes(Ntv2SystemContext* context, NTV2Channel channel);
bool IsAncExtField1Overrun(Ntv2SystemContext* context, NTV2Channel channel);
ULWord GetAncExtField2Bytes(Ntv2SystemContext* context, NTV2Channel channel);
bool IsAncExtField2Overrun(Ntv2SystemContext* context, NTV2Channel channel);
bool SetAncExtField1StartLine(Ntv2SystemContext* context, NTV2Channel channel, ULWord lineNumber);
bool SetAncExtField2StartLine(Ntv2SystemContext* context, NTV2Channel channel, ULWord lineNumber);
bool SetAncExtTotalFrameLines(Ntv2SystemContext* context, NTV2Channel channel, ULWord totalFrameLines);
bool SetAncExtFidLow(Ntv2SystemContext* context, NTV2Channel channel, ULWord lineNumber);
bool SetAncExtFidHi(Ntv2SystemContext* context, NTV2Channel channel, ULWord lineNumber);
bool SetAncExtField1AnalogStartLine(Ntv2SystemContext* context, NTV2Channel channel, ULWord lineNumber);
bool SetAncExtField2AnalogStartLine(Ntv2SystemContext* context, NTV2Channel channel, ULWord lineNumber);
bool SetAncExtField1AnalogYFilter(Ntv2SystemContext* context, NTV2Channel channel, ULWord lineFilter);
bool SetAncExtField2AnalogYFilter(Ntv2SystemContext* context, NTV2Channel channel, ULWord lineFilter);
bool SetAncExtField1AnalogCFilter(Ntv2SystemContext* context, NTV2Channel channel, ULWord lineFilter);
bool SetAncExtField2AnalogCFilter(Ntv2SystemContext* context, NTV2Channel channel, ULWord lineFilter);

bool SetupAncInserter(Ntv2SystemContext* context, NTV2Channel channel);
bool EnableAncInserter(Ntv2SystemContext* context, NTV2Channel channel, bool bEnable);
bool SetAncInsReadParams(Ntv2SystemContext* context, NTV2Channel channel, ULWord frameNumber, ULWord field1Size);
bool SetAncInsReadField2Params(Ntv2SystemContext* context, NTV2Channel channel, ULWord frameNumber, ULWord field2Size);
bool SetAncInsField1Bytes(Ntv2SystemContext* context, NTV2Channel channel, ULWord numberOfBytes);
bool SetAncInsField2Bytes(Ntv2SystemContext* context, NTV2Channel channel, ULWord numberOfBytes);
bool EnableAncInsHancY(Ntv2SystemContext* context, NTV2Channel channel, bool bEnable);
bool EnableAncInsHancC(Ntv2SystemContext* context, NTV2Channel channel, bool bEnable);
bool EnableAncInsVancY(Ntv2SystemContext* context, NTV2Channel channel, bool bEnable);
bool EnableAncInsVancC(Ntv2SystemContext* context, NTV2Channel channel, bool bEnable);
bool SetAncInsProgressive(Ntv2SystemContext* context, NTV2Channel channel, bool isProgressive);
bool SetAncInsSDPacketSplit(Ntv2SystemContext* context, NTV2Channel channel, bool inEnable);
bool SetAncInsField1StartAddr(Ntv2SystemContext* context, NTV2Channel channel, ULWord startAddr);
bool SetAncInsField2StartAddr(Ntv2SystemContext* context, NTV2Channel channel, ULWord startAddr);
bool SetAncInsHancPixelDelay(Ntv2SystemContext* context, NTV2Channel channel, ULWord numberOfPixels);
bool SetAncInsVancPixelDelay(Ntv2SystemContext* context, NTV2Channel channel, ULWord numberOfPixels);
bool SetAncInsField1ActiveLine(Ntv2SystemContext* context, NTV2Channel channel, ULWord activeLineNumber);
bool SetAncInsField2ActiveLine(Ntv2SystemContext* context, NTV2Channel channel, ULWord activeLineNumber);
bool SetAncInsHActivePixels(Ntv2SystemContext* context, NTV2Channel channel, ULWord numberOfActiveLinePixels);
bool SetAncInsHTotalPixels(Ntv2SystemContext* context, NTV2Channel channel, ULWord numberOfLinePixels);
bool SetAncInsTotalLines(Ntv2SystemContext* context, NTV2Channel channel, ULWord numberOfLines);
bool SetAncInsFidHi(Ntv2SystemContext* context, NTV2Channel channel, ULWord lineNumber);
bool SetAncInsFidLow(Ntv2SystemContext* context, NTV2Channel channel, ULWord lineNumber);

#endif

