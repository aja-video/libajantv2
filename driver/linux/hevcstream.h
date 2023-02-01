/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
///////////////////////////////////////////////////////////////
//
// HEVC Linux Device Driver for AJA HEVC devices.
//
////////////////////////////////////////////////////////////
//
// Filename: hevcstream.h
// Purpose:	 HEVC command processing fuctions
// Notes:	 
//
///////////////////////////////////////////////////////////////

#ifndef HEVCSTREAM_H
#define HEVCSTREAM_H

#include "hevccommon.h"

// initialize and release stream queue resources
void hevcStreamInitialize(uint32_t devNum);
void hevcStreamRelease(uint32_t devNum);

// reset stream queues
void hevcStreamReset(uint32_t devNum);


// allocate a stream buffer
HevcStreamBufferGroup* hevcStreamAssignBuffer(uint32_t devNum, uint32_t streamType, uint32_t streamId);

// release a stream buffer
void hevcStreamReleaseBuffer(uint32_t devNum, HevcStreamBufferGroup* pBufferGroup);

// copy vei data to vei stream buffer
void hevcStreamCopyVeiBuffer(uint32_t devNum, HevcStreamBufferGroup* pBufferGroup, HevcTransferData* pTransfer);

// copy seo data from seo stream buffer
void hevcStreamCopySeoBuffer(uint32_t devNum, HevcStreamBufferGroup* pBufferGroup, HevcTransferData* pTransfer);

// enqueue a stream transfer
HevcStreamTask* hevcStreamEnqueue(uint32_t devNum, HevcStreamInfo* pStrInfo);

// wait for stream acknowledge
bool hevcStreamAckWait(uint32_t devNum, HevcStreamTask* pStrTask);

// wait for stream message
bool hevcStreamMsgWait(uint32_t devNum, HevcStreamTask* pStrTask);

// dequeue a stream transfer
bool hevcStreamDequeue(uint32_t devNum, HevcStreamTask* pStrTask);

// check for orphan tasks
void hevcStreamOrphan(uint32_t devNum, uint32_t streamType);

// record stream acknowledge information to task
void hevcStreamAck(uint32_t devNum, HevcStreamAckInfo* pAckInfo);

// record stream message information to task
void hevcStreamMsg(uint32_t devNum, HevcStreamMsgInfo* pMsgInfo);

// enqueue stream frame data
bool hevcStreamFrameReady(uint32_t devNum, HevcFrameData* pFrameData);

// dequeue stream frame data
bool hevcStreamFrameDone(uint32_t devNum, HevcFrameData* pFrameData);

// stream frame queue reset
void hevcStreamFrameReset(uint32_t devNum, uint32_t streamBits);

// clear all stream statistics
void hevcStreamClearAllStats(uint32_t devNum);

// clear stream statistics for streamBits
void hevcStreamClearStats(uint32_t devNum, uint32_t streamType, uint32_t streamBits);

// update transfer statistics
void hevcStreamTransferStats(uint32_t devNum, uint32_t streamType, uint32_t streamId, uint32_t transferSize);

// update copy statistics
void hevcStreamCopyStats(uint32_t devNum, uint32_t streamType, uint32_t streamId,
						 int64_t copyStartTime, int64_t copyStopTime);

// update dma statistics
void hevcStreamDmaStats(uint32_t devNum, uint32_t streamType, uint32_t streamId,
						int64_t enqueueTime, int64_t sendTime, int64_t ackTime, int64_t msgTime, int64_t dequeueTime);

// get current stream statistics
void hevcStreamGetStats(uint32_t devNum, uint32_t streamType, uint32_t streamId, HevcStreamStatistics* pStats);

// get current frame queue level
uint32_t hevcStreamGetFrameQueueLevel(uint32_t devNum, uint32_t streamId);

#endif
