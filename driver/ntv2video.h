/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
//========================================================================
//
//  ntv2video.h
//
//==========================================================================

#ifndef NTV2VIDEO_H
#define NTV2VIDEO_H

#include "ntv2system.h"
#include "ntv2publicinterface.h"

void SetRegisterWritemode(Ntv2SystemContext* context, NTV2RegisterWriteMode value, NTV2Channel channel);

int64_t GetFramePeriod(Ntv2SystemContext* context, NTV2Channel channel);

void InitLUTRegs(Ntv2SystemContext* context);
bool Has12BitLUTSupport(Ntv2SystemContext* context);
bool DownloadLinearLUTToHW(Ntv2SystemContext* context, NTV2Channel inChannel, int inBank);
void LoadLUTValues(Ntv2SystemContext* context);
bool SetLUTEnable(Ntv2SystemContext* context, bool inEnable, NTV2Channel inChannel);

void SetColorCorrectionHostAccessBank(Ntv2SystemContext* context, NTV2ColorCorrectionHostAccessBank value);
NTV2ColorCorrectionHostAccessBank GetColorCorrectionHostAccessBank(Ntv2SystemContext* context, NTV2Channel channel);
void SetColorCorrectionSaturation(Ntv2SystemContext* context, NTV2Channel channel, uint32_t value);
uint32_t GetColorCorrectionSaturation(Ntv2SystemContext* context, NTV2Channel channel);
void SetColorCorrectionOutputBank(Ntv2SystemContext* context, NTV2Channel channel, uint32_t bank);
uint32_t GetColorCorrectionOutputBank(Ntv2SystemContext* context, NTV2Channel channel);
void SetLUTV2HostAccessBank(Ntv2SystemContext* context, NTV2ColorCorrectionHostAccessBank value);
uint32_t GetLUTV2HostAccessBank(Ntv2SystemContext* context, NTV2Channel inChannel);
void SetLUTV2OutputBank(Ntv2SystemContext* context, NTV2Channel channel, uint32_t bank);
uint32_t GetLUTV2OutputBank(Ntv2SystemContext* context, NTV2Channel channel);
void SetColorCorrectionMode(Ntv2SystemContext* context, NTV2Channel channel, NTV2ColorCorrectionMode mode);
NTV2ColorCorrectionMode GetColorCorrectionMode(Ntv2SystemContext* context, NTV2Channel channel);

void SetForegroundVideoCrosspoint(Ntv2SystemContext* context, NTV2Crosspoint crosspoint);
void SetForegroundKeyCrosspoint(Ntv2SystemContext* context, NTV2Crosspoint crosspoint);
void SetBackgroundVideoCrosspoint(Ntv2SystemContext* context, NTV2Crosspoint crosspoint);
void SetBackgroundKeyCrosspoint(Ntv2SystemContext* context, NTV2Crosspoint crosspoint);

#endif
