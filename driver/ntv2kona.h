/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
////////////////////////////////////////////////////////////
//
// Filename: ntv2kona.h
// Purpose:	 Common configuration and status
//
///////////////////////////////////////////////////////////////

#ifndef NTV2KONA_HEADER
#define NTV2KONA_HEADER

#include "ntv2system.h"
#include "ntv2devicefeatures.h"
#include "ntv2xpt.h"
#include "ntv2vpid.h"
#include "ntv2rp188.h"
#include "ntv2anc.h"


///////////////////////
//board format routines
NTV2VideoFormat GetBoardVideoFormat(Ntv2SystemContext* context, NTV2Channel channel);
NTV2Standard GetStandard(Ntv2SystemContext* context, NTV2Channel channel);
NTV2FrameGeometry GetFrameGeometry(Ntv2SystemContext* context, NTV2Channel channel);
NTV2FrameRate GetFrameRate(Ntv2SystemContext* context, NTV2Channel channel);
bool IsProgressiveStandard(Ntv2SystemContext* context, NTV2Channel channel);
bool GetSmpte372(Ntv2SystemContext* context, NTV2Channel channel);
bool GetQuadFrameEnable(Ntv2SystemContext* context, NTV2Channel channel);
bool Get4kSquaresEnable (Ntv2SystemContext* context, NTV2Channel channel);
bool Get425FrameEnable (Ntv2SystemContext* context, NTV2Channel channel);
bool Get12GTSIFrameEnable (Ntv2SystemContext* context, NTV2Channel channel);
bool GetQuadQuadFrameEnable(Ntv2SystemContext* context, NTV2Channel channel);
bool GetQuadQuadSquaresEnable(Ntv2SystemContext* context, NTV2Channel channel);
bool IsMultiFormatActive (Ntv2SystemContext* context);
bool GetEnable4KDCPSFOutMode(Ntv2SystemContext* context);
NTV2FrameBufferFormat GetFrameBufferFormat(Ntv2SystemContext* context, NTV2Channel channel);
void SetFrameBufferFormat(Ntv2SystemContext* context, NTV2Channel channel, NTV2FrameBufferFormat value);
NTV2VideoFrameBufferOrientation GetFrameBufferOrientation(Ntv2SystemContext* context, NTV2Channel channel);
void SetFrameBufferOrientation(Ntv2SystemContext* context, NTV2Channel channel, NTV2VideoFrameBufferOrientation value);
bool GetConverterOutStandard(Ntv2SystemContext* context, NTV2Standard* value);
bool ReadFSHDRRegValues(Ntv2SystemContext* context, NTV2Channel channel, HDRDriverValues* hdrRegValues);

///////////////////////
NTV2Mode GetMode(Ntv2SystemContext* context, NTV2Channel channel);
void SetMode(Ntv2SystemContext* context, NTV2Channel channel, NTV2Mode value);
uint32_t GetOutputFrame(Ntv2SystemContext* context, NTV2Channel channel);
void SetOutputFrame(Ntv2SystemContext* context, NTV2Channel channel, uint32_t value);
uint32_t GetInputFrame(Ntv2SystemContext* context, NTV2Channel channel);
void SetInputFrame(Ntv2SystemContext* context, NTV2Channel channel, uint32_t value);
uint32_t GetPCIAccessFrame(Ntv2SystemContext* context, NTV2Channel channel);
void SetPCIAccessFrame(Ntv2SystemContext* context, NTV2Channel channel, uint32_t value);
bool Get2piCSC(Ntv2SystemContext* context, NTV2Channel channel);
bool Set2piCSC(Ntv2SystemContext* context, NTV2Channel channel, bool enable);
NTV2FrameBufferFormat GetDualLink5PixelFormat(Ntv2SystemContext* context);
void SetDualLink5PixelFormat(Ntv2SystemContext* context, NTV2FrameBufferFormat value);
ULWord GetHWFrameBufferSize(Ntv2SystemContext* context, NTV2Channel channel);
ULWord GetFrameBufferSize(Ntv2SystemContext* context, NTV2Channel channel);

///////////////////////
bool FieldDenotesStartOfFrame(Ntv2SystemContext* context, NTV2Crosspoint channelSpec);
bool IsFieldID0(Ntv2SystemContext* context, NTV2Crosspoint xpt);


///////////////////////
//sdi routines
bool SetVideoOutputStandard(Ntv2SystemContext* context, NTV2Channel channel);
bool SetSDIOutStandard(Ntv2SystemContext* context, NTV2Channel channel, NTV2Standard value);
bool SetSDIOut_2Kx1080Enable(Ntv2SystemContext* context, NTV2Channel channel, bool enable);
bool GetSDIOut3GEnable(Ntv2SystemContext* context, NTV2Channel channel, bool* enable);
bool SetSDIOut3GEnable(Ntv2SystemContext* context, NTV2Channel channel, bool enable);
bool GetSDIOut3GbEnable(Ntv2SystemContext* context, NTV2Channel channel, bool* enable);
bool SetSDIOut3GbEnable(Ntv2SystemContext* context, NTV2Channel channel, bool enable);
bool GetSDIOut6GEnable(Ntv2SystemContext* context, NTV2Channel channel, bool* enable);
bool SetSDIOut6GEnable(Ntv2SystemContext* context, NTV2Channel channel, bool enable);
bool GetSDIOut12GEnable(Ntv2SystemContext* context, NTV2Channel channel, bool* enable);
bool SetSDIOut12GEnable(Ntv2SystemContext* context, NTV2Channel channel, bool enable);
bool GetSDIOutRGBLevelAConversion(Ntv2SystemContext* context, NTV2Channel channel, bool* enable);
bool GetSDIOutLevelAtoLevelBConversion(Ntv2SystemContext* context, NTV2Channel channel, bool* enable);
bool GetSDIInLevelBtoLevelAConversion(Ntv2SystemContext* context, NTV2Channel channel, bool* enable);
bool GetSDIIn6GEnable(Ntv2SystemContext* context, NTV2Channel channel);
bool GetSDIIn12GEnable(Ntv2SystemContext* context, NTV2Channel channel);


///////////////////////
//hdmi routines
bool SetLHiHDMIOutputStandard(Ntv2SystemContext* context);
bool SetHDMIOutputStandard(Ntv2SystemContext* context);
bool SetHDMIV2LevelBEnable(Ntv2SystemContext* context, bool enable);
bool SetMultiRasterInputStandard(Ntv2SystemContext* context, NTV2Standard mrStandard, NTV2Channel mrChannel);
bool SetEnableMultiRasterCapture(Ntv2SystemContext* context, bool bEnable);
bool HasMultiRasterWidget(Ntv2SystemContext* context);
bool IsMultiRasterEnabled(Ntv2SystemContext* context);

///////////////////////
//hdr routines
bool EnableHDMIHDR(Ntv2SystemContext* context, bool inEnableHDMIHDR);
bool GetEnableHDMIHDR(Ntv2SystemContext* context);
bool SetHDRData(Ntv2SystemContext* context, HDRDriverValues inRegisterValues);
bool GetHDRData(Ntv2SystemContext* context, HDRDriverValues* inRegisterValues);

///////////////////////
//analog routines
bool SetLHiAnalogOutputStandard(Ntv2SystemContext* context);

///////////////////////
//converter routines
bool GetK2ConverterOutFormat(Ntv2SystemContext* context, NTV2VideoFormat* format);

///////////////////////
//input routines
bool GetSourceVideoFormat(Ntv2SystemContext* context, NTV2VideoFormat* format, NTV2OutputXptID crosspoint, bool* quadMode, bool* quadQuadMode, HDRDriverValues* hdrRegValues);
NTV2VideoFormat GetInputVideoFormat(Ntv2SystemContext* context, NTV2Channel channel);
NTV2VideoFormat GetHDMIInputVideoFormat(Ntv2SystemContext* context);
NTV2VideoFormat GetAnalogInputVideoFormat(Ntv2SystemContext* context);

///////////////////////
//interrupt routines
bool UpdateAudioMixerGainFromRotaryEncoder(Ntv2SystemContext* context);

///////////////////////
//util routines
ULWord IsScanGeometry2Kx1080(NTV2ScanGeometry scanGeometry);
bool IsVideoFormat2Kx1080(NTV2VideoFormat videoFormat);
NTV2Crosspoint GetNTV2CrosspointChannelForIndex(ULWord index);
ULWord GetIndexForNTV2CrosspointChannel(NTV2Crosspoint channel);
NTV2Crosspoint GetNTV2CrosspointInputForIndex(ULWord index);
ULWord GetIndexForNTV2CrosspointInput(NTV2Crosspoint channel);
NTV2Crosspoint GetNTV2CrosspointForIndex(ULWord index);
ULWord GetIndexForNTV2Crosspoint(NTV2Crosspoint channel);
NTV2Channel GetNTV2ChannelForNTV2Crosspoint(NTV2Crosspoint crosspoint);
NTV2VideoFormat GetVideoFormatFromState(NTV2Standard standard, NTV2FrameRate frameRate, ULWord is2Kx1080, ULWord smpte372Enabled);
NTV2Standard GetNTV2StandardFromVideoFormat(NTV2VideoFormat videoFormat);
NTV2FrameRate GetNTV2FrameRateFromVideoFormat(NTV2VideoFormat videoFormat);
NTV2Channel GetOutXptChannel(NTV2OutputCrosspointID inXpt, bool multiFormatActive);
NTV2Standard GetStandardFromScanGeometry(NTV2ScanGeometry scanGeometry, ULWord progressive);
NTV2VideoFormat GetQuadSizedVideoFormat(NTV2VideoFormat videoFormat);
NTV2VideoFormat Get12GVideoFormat(NTV2VideoFormat videoFormat);
NTV2VideoFormat GetQuadQuadSizedVideoFormat(NTV2VideoFormat videoFormat);
NTV2VideoFormat GetHDSizedVideoFormat(NTV2VideoFormat videoFormat);
bool HDRIsChanging(HDRDriverValues inCurrentHDR, HDRDriverValues inNewHDR);

#endif
