/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
//========================================================================
//
//  ntv2audio.h
//
//==========================================================================

#ifndef NTV2AUDIO_H
#define NTV2AUDIO_H

#include "ntv2system.h"
#include "ntv2publicinterface.h"

#define NTV2_AUDIOSAMPLESIZE                (sizeof(uint32_t))
#define NTV2_AUDIO_WRAPADDRESS               0xFF000
#define NTV2_AUDIO_WRAPADDRESS_MEDIUM       (0xFF000*2)
#define NTV2_AUDIO_WRAPADDRESS_BIG          (0xFF000*4)
#define NTV2_AUDIO_WRAPADDRESS_BIGGER       (0xFF000*8)			// used with KiPro Mini 99 video buffers
#define NTV2_AUDIO_READBUFFEROFFSET          0x100000
#define NTV2_AUDIO_READBUFFEROFFSET_MEDIUM  (0x100000*2)
#define NTV2_AUDIO_READBUFFEROFFSET_BIG     (0x100000*4)

void StartAudioPlayback(Ntv2SystemContext* context, NTV2AudioSystem audioSystem);
void StopAudioPlayback(Ntv2SystemContext* context, NTV2AudioSystem audioSystem);
bool IsAudioPlaybackStopped(Ntv2SystemContext* context, NTV2AudioSystem audioSystem);
bool IsAudioPlaying(Ntv2SystemContext* context, NTV2AudioSystem audioSystem);
void PauseAudioPlayback(Ntv2SystemContext* context, NTV2AudioSystem audioSystem);
void UnPauseAudioPlayback(Ntv2SystemContext* context, NTV2AudioSystem audioSystem);
bool IsAudioPlaybackPaused(Ntv2SystemContext* context, NTV2AudioSystem audioSystem);
void StartAudioCapture(Ntv2SystemContext* context, NTV2AudioSystem audioSystem);
void StopAudioCapture(Ntv2SystemContext* context, NTV2AudioSystem audioSystem);
bool IsAudioCaptureStopped(Ntv2SystemContext* context, NTV2AudioSystem audioSystem);
uint32_t GetNumAudioChannels(Ntv2SystemContext* context, NTV2AudioSystem audioSystem);
uint32_t oemAudioSampleAlign(Ntv2SystemContext* context, NTV2AudioSystem audioSystem, uint32_t ulReadSample);
uint32_t GetAudioLastOut(Ntv2SystemContext* context, NTV2AudioSystem audioSystem);
uint32_t GetAudioLastIn(Ntv2SystemContext* context, NTV2AudioSystem audioSystem);
uint32_t GetAudioSamplesPerFrame(Ntv2SystemContext* context,
							   NTV2AudioSystem audioSystem,
							   uint32_t cadenceFrame,
							   bool fieldMode);
NTV2AudioRate GetAudioRate(Ntv2SystemContext* context, NTV2AudioSystem audioSystem);
uint32_t GetAudioSamplesPerSecond(Ntv2SystemContext* context, NTV2AudioSystem audioSystem);
uint32_t GetAudioTransferInfo(Ntv2SystemContext* context,
							NTV2AudioSystem audioSystem,
							uint32_t currentOffset,
							uint32_t numBytesToTransfer,
							uint32_t* preWrapBytes,
							uint32_t* postWrapBytes);
uint32_t GetAudioWrapAddress(Ntv2SystemContext* context, NTV2AudioSystem audioSystem);
bool GetAudioBufferSize(Ntv2SystemContext* context, NTV2AudioSystem audioSystem, NTV2AudioBufferSize *value);
uint32_t GetAudioReadOffset(Ntv2SystemContext* context, NTV2AudioSystem audioSystem);
uint32_t GetAudioFrameBufferNumber(Ntv2SystemContext* context, NTV2AudioSystem audioSystem);

#endif
