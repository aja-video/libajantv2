/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
//////////////////////////////////////////////////////////////
//
// HEVC Linux Device Driver for AJA HEVC boards.
//
// Boards supported include:
//
// OEM-HEVC
//
// Filename: hevcommon.c
// Purpose:	 Main module file.  Load, unload, fops, ioctls.
//
///////////////////////////////////////////////////////////////


#include "hevcstream.h"
#include "hevcregister.h"

#define HEVC_STREAM_TIMEOUT			3000000			    // 3 sec
#define HEVC_STREAM_AVR_FACTOR		128

typedef struct hevc_stream_segment
{
	uint32_t	srcPitch;			// source segment pitch
	uint32_t	dstPitch;			// destination segment pitch
	uint32_t	segSize;			// segment size
	uint32_t	segCount;			// segment count
	uint32_t	segIndex;			// segment current index
	uint32_t	srcAddress;			// source current address
	uint32_t	srcSize;			// source transfer size
	uint32_t	dstAddress;			// destination current address
	uint32_t	dstSize;			// destination transfer size
} HevcStreamSegment;

// vei and seo stream task send dpc
static void hevcStreamVeiDpc(Ntv2DpcData data);
static void hevcStreamSeoDpc(Ntv2DpcData data);

// send and complete a dma transfer to the codec
static void hevcStreamSend(uint32_t devNum, uint32_t streamType);
static bool hevcStreamDone(uint32_t devNum, HevcStreamTask* pStrTask, bool force);

// allocate and free a buffer group
static void hevcStreamAllocateBufferGroup(uint32_t devNum, uint32_t streamType, uint32_t streamId, uint32_t bufNum);
static void hevcStreamFreeBufferGroup(uint32_t devNum, uint32_t streamType, uint32_t streamId, uint32_t bufNum);

// allocate and free a contiguous buffer
static void hevcStreamAllocateBuffer(uint32_t devNum, uint32_t size, HevcStreamBuffer* pStreamBuffer);
static void hevcStreamFreeBuffer(uint32_t devNum, HevcStreamBuffer* pStreamBuffer);

// get current stream queue level
static uint32_t hevcStreamQueueLevel(uint32_t devNum, uint32_t streamType);

static void hevcStreamSegmentInit(uint32_t devNum,  HevcStreamSegment* pSegment,
								  uint32_t srcPitch, uint32_t dstPitch,
								  uint32_t segSize, uint32_t segCount);

static void hevcStreamSegmentSrcRange(uint32_t devNum, HevcStreamSegment* pSegment, uint32_t srcAddress, uint32_t srcSize);
static void hevcStreamSegmentDstRange(uint32_t devNum, HevcStreamSegment* pSegment, uint32_t dstAddress, uint32_t dstSize);
static bool hevcStreamSegmentTransfer(uint32_t devNum, HevcStreamSegment* pSegment,
									  uint32_t* pSrcOffset, uint32_t* pDstOffset, uint32_t* pSize);

void hevcStreamInitialize(uint32_t devNum)
{
	HevcModuleParams* pModParams = hevcGetModuleParams();
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	uint32_t is;
	uint32_t it;
	uint32_t id;
	uint32_t ib;

	if(pDevParams == NULL) return;

	HEVC_MSG_STREAM_INFO("%s(%d): hevcStreamInitialize()  max queue depth %d\n",
						 pModParams->pModuleName, devNum, HEVC_STREAM_TASK_MAX);

	// initialize task queue array
	memset(pDevParams->strQueue, 0, sizeof(pDevParams->strQueue));

	// initialize buffer pool array
	memset(pDevParams->strBuffers, 0, sizeof(pDevParams->strBuffers));

	// initialize codec queue array
	memset(pDevParams->strFrameQueue, 0, sizeof(pDevParams->strFrameQueue));

	// for each stream type
	for(is = 0; is < FMB_STREAM_TYPE_MAX; is++)
	{
		// inialize queue lock
		ntv2SpinLockOpen(&pDevParams->strQueueLock[is], &pDevParams->systemContext);

		// initialize queue ready and send pointers
		pDevParams->strQueueReady[is] = 0;
		pDevParams->strQueueSend[is] = 0;

		// initialize stream tasks
		for(it = 0; it < HEVC_STREAM_TASK_MAX; it++)
		{
			pDevParams->strQueue[is][it].taskNum = it;
			ntv2EventOpen(&pDevParams->strQueue[is][it].ackEvent, &pDevParams->systemContext);
			ntv2EventOpen(&pDevParams->strQueue[is][it].msgEvent, &pDevParams->systemContext);
		}

		// initialize stream send dpc
		switch(is)
		{
		case FMB_STREAM_TYPE_VEI:
			ntv2DpcOpen(&pDevParams->strQueueDpc[is],
						&pDevParams->systemContext,
						hevcStreamVeiDpc,
						devNum);
			pDevParams->strNumBuffers[is] = HEVC_STREAM_VEI_BUFFERS;
			break;
		case FMB_STREAM_TYPE_SEO:
			ntv2DpcOpen(&pDevParams->strQueueDpc[is],
						&pDevParams->systemContext,
						hevcStreamSeoDpc,
						devNum);
			pDevParams->strNumBuffers[is] = HEVC_STREAM_SEO_BUFFERS;
			break;
		default:
			break;
		}

		// for each stream id
		for(id = 0; id < FMB_STREAM_ID_MAX; id++)
		{
			// initialize buffer and statistics locks
			ntv2SpinLockOpen(&pDevParams->strBufferLock[is][id], &pDevParams->systemContext);
			ntv2SpinLockOpen(&pDevParams->strStatLock[is][id], &pDevParams->systemContext);

			// initalize buffer wait event
			ntv2EventOpen(&pDevParams->strBufferEvent[is][id], &pDevParams->systemContext);

			// allocate stream buffers
			for(ib = 0; ib < pDevParams->strNumBuffers[is]; ib++)
			{
				hevcStreamAllocateBufferGroup(devNum, is, id, ib);
			}
		}
	}

	// for each stream id
	for(id = 0; id < FMB_STREAM_ID_MAX; id++)
	{
		// initialize codec data lock
		ntv2SpinLockOpen(&pDevParams->strFrameLock[id], &pDevParams->systemContext);
		// initialize queue pointers
		pDevParams->strFrameReady[id] = 0;
		pDevParams->strFrameDone[id] = 0;
	}
}

void hevcStreamRelease(uint32_t devNum)
{
	HevcModuleParams* pModParams = hevcGetModuleParams();
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	uint32_t is;
	uint32_t it;
	uint32_t id;
	uint32_t ib;

	if(pDevParams == NULL) return;

	HEVC_MSG_STREAM_INFO("%s(%d): hevcStreamRelease()\n",
						 pModParams->pModuleName, devNum);

	// for each stream type
	for(is = 0; is < FMB_STREAM_TYPE_MAX; is++)
	{
		// kill the send dpc
		switch(is)
		{
		case FMB_STREAM_TYPE_VEI:
			ntv2DpcClose(&pDevParams->strQueueDpc[is]);
			break;
		case FMB_STREAM_TYPE_SEO:
			ntv2DpcClose(&pDevParams->strQueueDpc[is]);
			break;
		default:
			break;
		}

		// close lock
		ntv2SpinLockClose(&pDevParams->strQueueLock[is]);

		// for each task
		for(it = 0; it < HEVC_STREAM_TASK_MAX; it++)
		{
			// reset the flag and wake up waiting threads
			pDevParams->strQueue[is][it].apiReady = false;
			pDevParams->strQueue[is][it].ackReceived = true;
			pDevParams->strQueue[is][it].msgReceived = true;
			pDevParams->strQueue[is][it].apiDone = true;
			ntv2EventClose(&pDevParams->strQueue[is][it].ackEvent);
			ntv2EventClose(&pDevParams->strQueue[is][it].msgEvent);
		}

		// for each stream id
		for(id = 0; id < FMB_STREAM_ID_MAX; id++)
		{
			// close locks
			ntv2SpinLockClose(&pDevParams->strBufferLock[is][id]);
			ntv2SpinLockClose(&pDevParams->strStatLock[is][id]);

			// close event
			ntv2EventClose(&pDevParams->strBufferEvent[is][id]);

			// release stream buffers
			for(ib = 0; ib < pDevParams->strNumBuffers[is]; ib++)
			{
				hevcStreamFreeBufferGroup(devNum,  is, id, ib);
			}
		}
	}

	// for each stream id
	for(id = 0; id < FMB_STREAM_ID_MAX; id++)
	{
		// close data lock
		ntv2SpinLockClose(&pDevParams->strFrameLock[id]);
	}
}

void hevcStreamReset(uint32_t devNum)
{
	HevcModuleParams* pModParams = hevcGetModuleParams();
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	uint32_t is;
	uint32_t it;
	uint32_t id;
	uint32_t ib;

	if(pDevParams == NULL) return;

	HEVC_MSG_STREAM_INFO("%s(%d): hevcStreamReset() reset stream queues\n",
						 pModParams->pModuleName, devNum);

	// for each stream type
	for(is = 0; is < FMB_STREAM_TYPE_MAX; is++)
	{
		ntv2SpinLockAcquire(&pDevParams->strQueueLock[is]);

		// reset the task flags
		for(it = 0; it < HEVC_STREAM_TASK_MAX; it++)
		{
			pDevParams->strQueue[is][it].apiReady = false;
			pDevParams->strQueue[is][it].dmaSent = true;
			pDevParams->strQueue[is][it].ackReceived = true;
			pDevParams->strQueue[is][it].msgReceived = true;
			pDevParams->strQueue[is][it].apiDone = true;
		}

		// for each stream id
		for(id = 0; id < FMB_STREAM_ID_MAX; id++)
		{
			ntv2SpinLockAcquire(&pDevParams->strBufferLock[is][id]);

			// release the bounce buffer
			for(ib = 0; ib < pDevParams->strNumBuffers[is]; ib++)
			{
				pDevParams->strBuffers[is][id][ib].active = false;
			}

			ntv2SpinLockRelease(&pDevParams->strBufferLock[is][id]);
		}

		// reset queue pointers
		pDevParams->strQueueReady[is] = 0;
		pDevParams->strQueueSend[is] = 0;

		// reset queue counts
		pDevParams->strQueueCount[is] = 0;
		pDevParams->strQueueLevel[is] = 0;

		ntv2SpinLockRelease(&pDevParams->strQueueLock[is]);
	}

	// for each stream id
	for(id = 0; id < FMB_STREAM_ID_MAX; id++)
	{
		// reset queue pointers
		pDevParams->strFrameReady[id] = 0;
		pDevParams->strFrameDone[id] = 0;
		pDevParams->strFrameQueueLevel[id] = 0;
	}
}

HevcStreamBufferGroup* hevcStreamAssignBuffer(uint32_t devNum, uint32_t streamType, uint32_t streamId)
{
	HevcModuleParams* pModParams = hevcGetModuleParams();
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	HevcStreamBufferGroup* pBufferGroup = NULL;
	uint32_t ib = 0;
	bool success;

	if(pDevParams == NULL) return NULL;

	if((streamType >= FMB_STREAM_TYPE_MAX) ||
	   (streamId >= FMB_STREAM_ID_MAX)) return NULL;

	while(pBufferGroup == NULL)
	{
		ntv2EventClear(&pDevParams->strBufferEvent[streamType][streamId]);

		ntv2SpinLockAcquire(&pDevParams->strBufferLock[streamType][streamId]);

		// look for an unassigned buffer
		for(ib = 0; ib < pDevParams->strNumBuffers[streamType]; ib++)
		{
			if(!pDevParams->strBuffers[streamType][streamId][ib].active) break;
		}

		if(ib < pDevParams->strNumBuffers[streamType])
		{
			// mark buffer active
			pBufferGroup = &pDevParams->strBuffers[streamType][streamId][ib];
			pBufferGroup->active = true;
		}

		ntv2SpinLockRelease(&pDevParams->strBufferLock[streamType][streamId]);

		if(pBufferGroup == NULL)
		{
			// wait for a buffer
			success = ntv2EventWaitForSignal(&pDevParams->strBufferEvent[streamType][streamId],
											 HEVC_STREAM_TIMEOUT, true);
			if(!success) break;
		}
	}

	if(pBufferGroup != NULL)
	{
		HEVC_MSG_STREAM_STATE("%s(%d): hevcStreamAllocBuffer()  buffer found  stream type %d  id %d  buf %d\n",
							  pModParams->pModuleName, devNum, streamType, streamId, ib);
	}
	else
	{
		HEVC_MSG_STREAM_ERROR("%s(%d): hevcStreamAllocBuffer()  *error* no bounce buffer  stream type %d  id %d\n",
							  pModParams->pModuleName, devNum, streamType, streamId);
	}

	// return assigned buffer
	return pBufferGroup;
}

void hevcStreamReleaseBuffer(uint32_t devNum, HevcStreamBufferGroup* pBufferGroup)
{
	HevcModuleParams* pModParams = hevcGetModuleParams();
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	uint32_t streamType;
	uint32_t streamId;
	bool active;

	if((pDevParams == NULL) ||
	   (pBufferGroup == NULL)) return;

	streamType = pBufferGroup->streamType;
	streamId = pBufferGroup->streamId;

	ntv2SpinLockAcquire(&pDevParams->strBufferLock[streamType][streamId]);
	
	// release buffer
	active = pBufferGroup->active;
	pBufferGroup->active = false;
	pBufferGroup->videoDataSize = 0;
	pBufferGroup->infoDataSize = 0;

	ntv2SpinLockRelease(&pDevParams->strBufferLock[streamType][streamId]);

	ntv2EventSignal(&pDevParams->strBufferEvent[streamType][streamId]);

	if(active)
	{
		HEVC_MSG_STREAM_STATE("%s(%d): hevcStreamReleaseBuffer()  buffer released  stream type %d  id %d  buf %d\n",
							  pModParams->pModuleName, devNum, streamType,
							  pBufferGroup->streamId, pBufferGroup->bufNum);
	}
	else
	{
		HEVC_MSG_STREAM_ERROR("%s(%d): hevcStreamReleaseBuffer()  *error* buffer not active  stream type %d  id %d  buf %d\n",
							  pModParams->pModuleName, devNum, streamType,
							  pBufferGroup->streamId, pBufferGroup->bufNum);
	}

	return;
}

void hevcStreamCopyVeiBuffer(uint32_t devNum, HevcStreamBufferGroup* pBufferGroup, HevcTransferData* pTransfer)
{
	HevcModuleParams* pModParams = hevcGetModuleParams();
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	HevcStreamBuffer* pBuffer;
	Ntv2UserBuffer userBuffer;
	HevcStreamSegment strSegment;
	int64_t copyStartTime;
	int64_t copyStopTime;
	uint32_t copySize;
	uint32_t remainSize;
	uint32_t validSize;
	uint32_t ib;
	uint32_t srcAddress;
	uint32_t dstAddress;
	uint32_t segSize;
	bool success;

	if((pDevParams == NULL) ||
	   (pBufferGroup == NULL) ||
	   (pTransfer == NULL)) return;

	HEVC_MSG_STREAM_COPY("%s(%d): hevcStreamCopyVeiBuffer()  copy video from user  stream type %d  id %d  buf %d, size %d\n",
						 pModParams->pModuleName, devNum, pBufferGroup->streamType,
						 pBufferGroup->streamId, pBufferGroup->bufNum, pTransfer->videoDataSize);

	// setup for copy
	remainSize = pTransfer->videoDataSize;
	validSize = 0;

	// record copy start time
	copyStartTime = ntv2TimeCounter();

	// check user video buffer and copy size
	if((pTransfer->videoBuffer != 0) && (pTransfer->videoDataSize > 0))
	{
		// check user video buffer size
		if(pTransfer->videoDataSize <= pBufferGroup->videoBufferSize)
		{
			// prepare the user video buffer
			success = ntv2UserBufferPrepare(&userBuffer,
											&pDevParams->systemContext,
											pTransfer->videoBuffer,
											pTransfer->videoDataSize,
											false);
			if(success)
			{
				// initialize segment data
				if((pTransfer->segSize != 0) && (pTransfer->segCount != 0))
				{
					hevcStreamSegmentInit(devNum,
										  &strSegment,
										  pTransfer->segVideoPitch,
										  pTransfer->segCodecPitch,
										  pTransfer->segSize,
										  pTransfer->segCount);
				}
				else
				{
					hevcStreamSegmentInit(devNum, &strSegment, 0, 0, pTransfer->videoDataSize, 1);
				}

				// setup source range
				hevcStreamSegmentSrcRange(devNum, &strSegment, 0, pTransfer->videoDataSize);

				// for each dma buffer
				ib = 0;
				while(remainSize > 0)
				{
					pBuffer = &pBufferGroup->videoBuffers[ib];

					// verify dma buffer
					if((pBuffer->pBufferAddress == NULL) || (pBuffer->bufferSize == 0))
					{
						HEVC_MSG_STREAM_ERROR("%s(%d): hevcStreamCopyVeiBuffer()  *error* video buffer overrun  stream type %d  id %d  buf %d\n",
											  pModParams->pModuleName, devNum, pBufferGroup->streamType,
											  pBufferGroup->streamId, pBufferGroup->bufNum);
						break;
					}

					// determine copy size
					copySize = min(remainSize, pBuffer->bufferSize);
				
					// setup destination range
					hevcStreamSegmentDstRange(devNum, &strSegment, validSize, copySize);

					success = hevcStreamSegmentTransfer(devNum, &strSegment, &srcAddress, &dstAddress, &segSize);
					while(success)
					{
						// copy video data from user buffer to dma buffer
						success = ntv2UserBufferCopyFrom(&userBuffer,
														 srcAddress,
														 pBuffer->pBufferAddress + dstAddress,
														 segSize);
						if(!success)
						{
							HEVC_MSG_STREAM_ERROR("%s(%d): hevcStreamCopyVeiBuffer()  *error* video copy from user failed  stream type %d  id %d  buf %d\n",
												  pModParams->pModuleName, devNum, pBufferGroup->streamType,
												  pBufferGroup->streamId, pBufferGroup->bufNum);
							break;
						}
						success = hevcStreamSegmentTransfer(devNum, &strSegment, &srcAddress, &dstAddress, &segSize);
					}

					// track remaining size and valid size
					remainSize -= copySize;
					validSize += copySize;
					ib++;
					if(ib >= HEVC_STREAM_DESCRIPTOR_MAX)
					{
						HEVC_MSG_STREAM_ERROR("%s(%d): hevcStreamCopyVeiBuffer()  *error* video descriptor overrun  stream type %d  id %d  buf %d\n",
											  pModParams->pModuleName, devNum, pBufferGroup->streamType,
											  pBufferGroup->streamId, pBufferGroup->bufNum);
						break;
					}
				}

				// release user video buffer
				ntv2UserBufferRelease(&userBuffer);
			}
			else
			{
				HEVC_MSG_STREAM_ERROR("%s(%d): hevcStreamCopyVeiBuffer()  *error* prepare user buffer failed  stream type %d  id %d  buf %d\n",
									  pModParams->pModuleName, devNum, pBufferGroup->streamType,
									  pBufferGroup->streamId, pBufferGroup->bufNum);
			}
		}
		else
		{
			HEVC_MSG_STREAM_ERROR("%s(%d): hevcStreamCopyVeiBuffer()  *error* video buffer to large  stream type %d  id %d  buf %d\n",
								  pModParams->pModuleName, devNum, pBufferGroup->streamType,
								  pBufferGroup->streamId, pBufferGroup->bufNum);
		}
	}

	// set data size
	pBufferGroup->videoDataSize = validSize;

	// setup for copy
	validSize = 0;

	// check user info buffer and copy size
	if((pTransfer->infoBuffer != 0) && (pTransfer->infoDataSize > 0))
	{
		// check user info buffer size
		if(pTransfer->infoDataSize <= pBufferGroup->infoBufferSize)
		{
			// prepare the user info buffer
			success = ntv2UserBufferPrepare(&userBuffer,
											&pDevParams->systemContext,
											pTransfer->infoBuffer,
											pTransfer->infoDataSize,
											false);
			if(success)
			{
				HEVC_MSG_STREAM_COPY("%s(%d): hevcStreamCopyVeiBuffer()  copy pic data from user  stream type %d  id %d  buf %d, size %d\n",
									 pModParams->pModuleName, devNum, pBufferGroup->streamType,
									 pBufferGroup->streamId, pBufferGroup->bufNum, pTransfer->infoDataSize);

				// copy pic data from user info buffer to dma buffer
				success = ntv2UserBufferCopyFrom(&userBuffer, 0, pBufferGroup->infoBuffer.pBufferAddress, pTransfer->infoDataSize);
				if(success)
				{
					validSize = pTransfer->infoDataSize;
					HEVC_MSG_STREAM_COPY("%s(%d): hevcStreamCopyVeiBuffer() pic data  %08x  %08x  %08x  %08x  %08x\n",
										 pModParams->pModuleName, devNum,
										 *(((uint32_t*)pBufferGroup->infoBuffer.pBufferAddress)+0),
										 *(((uint32_t*)pBufferGroup->infoBuffer.pBufferAddress)+1),
										 *(((uint32_t*)pBufferGroup->infoBuffer.pBufferAddress)+2),
										 *(((uint32_t*)pBufferGroup->infoBuffer.pBufferAddress)+3),
										 *(((uint32_t*)pBufferGroup->infoBuffer.pBufferAddress)+4));
				}
				else
				{
					HEVC_MSG_STREAM_ERROR("%s(%d): hevcStreamCopyVeiBuffer()  *error* pic data copy from user failed  stream type %d  id %d  buf %d\n",
										  pModParams->pModuleName, devNum, pBufferGroup->streamType,
										  pBufferGroup->streamId, pBufferGroup->bufNum);
				}

				// release user info buffer
				ntv2UserBufferRelease(&userBuffer);
			}
			else
			{
				HEVC_MSG_STREAM_ERROR("%s(%d): hevcStreamCopyVeiBuffer()  *error* prepare pic data buffer failed  stream type %d  id %d  buf %d\n",
									  pModParams->pModuleName, devNum, pBufferGroup->streamType,
									  pBufferGroup->streamId, pBufferGroup->bufNum);
			}
		}
		else
		{
			HEVC_MSG_STREAM_ERROR("%s(%d): hevcStreamCopyVeiBuffer()  *error* pic data buffer to large  stream type %d  id %d  buf %d\n",
								  pModParams->pModuleName, devNum, pBufferGroup->streamType,
								  pBufferGroup->streamId, pBufferGroup->bufNum);
		}
	}

	// set data size
	pBufferGroup->infoDataSize = validSize;

	// record copy duration
	copyStopTime = ntv2TimeCounter();
	hevcStreamCopyStats(devNum, pBufferGroup->streamType, pBufferGroup->streamId,
						copyStartTime, copyStopTime);
}

void hevcStreamCopySeoBuffer(uint32_t devNum, HevcStreamBufferGroup* pBufferGroup, HevcTransferData* pTransfer)
{
	HevcModuleParams* pModParams = hevcGetModuleParams();
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	HevcStreamBuffer* pBuffer;
	Ntv2UserBuffer userBuffer;
	int64_t copyStartTime;
	int64_t copyStopTime;
	uint32_t remainSize;
	uint32_t validSize;
	uint32_t copySize;
	uint32_t ib;
	bool success;

	if((pDevParams == NULL) ||
	   (pBufferGroup == NULL) ||
	   (pTransfer == NULL)) return;

	HEVC_MSG_STREAM_COPY("%s(%d): hevcStreamCopySeoBuffer()  copy hevc to user  stream type %d  id %d  buf %d  size %d\n",
						 pModParams->pModuleName, devNum, pBufferGroup->streamType,
						 pBufferGroup->streamId, pBufferGroup->bufNum, pBufferGroup->videoDataSize);

	// setup for copy
	remainSize = pBufferGroup->videoDataSize;
	validSize = 0;

	// record copy start time
	copyStartTime = ntv2TimeCounter();

	// check user video buffer and copy size
	if((pTransfer->videoBuffer != 0) && (pBufferGroup->videoDataSize > 0))
	{
		// check user buffer size
		if(pTransfer->videoBufferSize >= pBufferGroup->videoDataSize)
		{
			// prepare the user video buffer
			success = ntv2UserBufferPrepare(&userBuffer,
											&pDevParams->systemContext,
											pTransfer->videoBuffer,
											pBufferGroup->videoDataSize,
											true);
			if(success)
			{
				// for each dma buffer
				ib = 0;
				while(remainSize > 0)
				{
					pBuffer = &pBufferGroup->videoBuffers[ib];

					// verify dma buffer
					if((pBuffer->pBufferAddress == NULL) || (pBuffer->bufferSize == 0))
					{
						HEVC_MSG_STREAM_ERROR("%s(%d): hevcStreamCopySeoBuffer()  *error* hevc buffer overrun  stream type %d  id %d  buf %d\n",
											  pModParams->pModuleName, devNum, pBufferGroup->streamType,
											  pBufferGroup->streamId, pBufferGroup->bufNum);
						break;
					}

					// determine copy size
					copySize = min(remainSize, pBuffer->bufferSize);

					// copy video data from dma buffer to user buffer
					success = ntv2UserBufferCopyTo(&userBuffer, validSize, pBuffer->pBufferAddress, copySize);
					if(!success)
					{
						HEVC_MSG_STREAM_ERROR("%s(%d): hevcStreamCopySeoBuffer()  *error* hevc copy to user failed  stream type %d  id %d  buf %d\n",
											  pModParams->pModuleName, devNum, pBufferGroup->streamType,
											  pBufferGroup->streamId, pBufferGroup->bufNum);
						break;
					}

					// track remaining size and copy size
					remainSize -= copySize;
					validSize += copySize;
					ib++;
					if(ib >= HEVC_STREAM_DESCRIPTOR_MAX)
					{
						HEVC_MSG_STREAM_ERROR("%s(%d): hevcStreamCopySeoBuffer()  *error* hevc descriptor overrun  stream type %d  id %d  buf %d\n",
											  pModParams->pModuleName, devNum, pBufferGroup->streamType,
											  pBufferGroup->streamId, pBufferGroup->bufNum);
						break;
					}

					pTransfer->videoDataSize = validSize;
				}
				
				// release user video buffer
				ntv2UserBufferRelease(&userBuffer);
			}
			else
			{
				HEVC_MSG_STREAM_ERROR("%s(%d): hevcStreamCopySeoBuffer()  *error* prepare user video buffer failed  stream type %d  id %d  buf %d\n",
									  pModParams->pModuleName, devNum, pBufferGroup->streamType,
									  pBufferGroup->streamId, pBufferGroup->bufNum);
			}
		}
		else
		{
			HEVC_MSG_STREAM_ERROR("%s(%d): hevcStreamCopySeoBuffer()  *error* hevc buffer to large  stream type %d  id %d  buf %d\n",
								  pModParams->pModuleName, devNum, pBufferGroup->streamType,
								  pBufferGroup->streamId, pBufferGroup->bufNum);
		}
	}

	// check user info buffer and info to copy
	if((pTransfer->infoBuffer != 0) && (pBufferGroup->infoDataSize > 0))
	{
		// check size of user info buffer
		if(pTransfer->infoBufferSize >= pBufferGroup->infoDataSize)
		{
			// prepare the user info buffer
			success = ntv2UserBufferPrepare(&userBuffer,
											&pDevParams->systemContext,
											pTransfer->infoBuffer,
											pBufferGroup->infoDataSize,
											true);
			if(success)
			{
				HEVC_MSG_STREAM_COPY("%s(%d): hevcStreamCopySeoBuffer()  copy es data to user  stream type %d  id %d  buf %d, size %d\n",
									 pModParams->pModuleName, devNum, pBufferGroup->streamType,
									 pBufferGroup->streamId, pBufferGroup->bufNum, pBufferGroup->infoDataSize);

				// copy es data from dma buffer to user buffer
				success = ntv2UserBufferCopyTo(&userBuffer, 0, pBufferGroup->infoBuffer.pBufferAddress, pBufferGroup->infoDataSize);
				if(success)
				{
					pTransfer->infoDataSize = pBufferGroup->infoDataSize;
					HEVC_MSG_STREAM_COPY("%s(%d): hevcStreamCopySeoBuffer() es data[0]  %08x  %08x  %08x  %08x  %08x  %08x  %08x  %08x\n",
										 pModParams->pModuleName, devNum,
										 *(((uint32_t*)pBufferGroup->infoBuffer.pBufferAddress)+0),
										 *(((uint32_t*)pBufferGroup->infoBuffer.pBufferAddress)+1),
										 *(((uint32_t*)pBufferGroup->infoBuffer.pBufferAddress)+2),
										 *(((uint32_t*)pBufferGroup->infoBuffer.pBufferAddress)+3),
										 *(((uint32_t*)pBufferGroup->infoBuffer.pBufferAddress)+4),
										 *(((uint32_t*)pBufferGroup->infoBuffer.pBufferAddress)+5),
										 *(((uint32_t*)pBufferGroup->infoBuffer.pBufferAddress)+6),
										 *(((uint32_t*)pBufferGroup->infoBuffer.pBufferAddress)+7));
					HEVC_MSG_STREAM_COPY("%s(%d): hevcStreamCopySeoBuffer() es data[8]  %08x  %08x  %08x  %08x  %08x  %08x\n",
										 pModParams->pModuleName, devNum,
										 *(((uint32_t*)pBufferGroup->infoBuffer.pBufferAddress)+8),
										 *(((uint32_t*)pBufferGroup->infoBuffer.pBufferAddress)+9),
										 *(((uint32_t*)pBufferGroup->infoBuffer.pBufferAddress)+10),
										 *(((uint32_t*)pBufferGroup->infoBuffer.pBufferAddress)+11),
										 *(((uint32_t*)pBufferGroup->infoBuffer.pBufferAddress)+12),
										 *(((uint32_t*)pBufferGroup->infoBuffer.pBufferAddress)+13));
				}
				else
				{
					HEVC_MSG_STREAM_ERROR("%s(%d): hevcStreamCopySeoBuffer()  *error* es data copy to user failed  stream type %d  id %d  buf %d\n",
										  pModParams->pModuleName, devNum, pBufferGroup->streamType,
										  pBufferGroup->streamId, pBufferGroup->bufNum);
				}

				// release user info buffer
				ntv2UserBufferRelease(&userBuffer);
			}
			else
			{
				HEVC_MSG_STREAM_ERROR("%s(%d): hevcStreamCopySeoBuffer()  *error* prepare user info buffer failed  stream type %d  id %d  buf %d\n",
									  pModParams->pModuleName, devNum, pBufferGroup->streamType,
									  pBufferGroup->streamId, pBufferGroup->bufNum);
			}
		}
		else
		{
			HEVC_MSG_STREAM_ERROR("%s(%d): hevcStreamCopySeoBuffer()  *error* es data buffer to large  stream type %d  id %d  buf %d\n",
								  pModParams->pModuleName, devNum, pBufferGroup->streamType,
								  pBufferGroup->streamId, pBufferGroup->bufNum);
		}
	}

	// record copy duration
	copyStopTime = ntv2TimeCounter();
	hevcStreamCopyStats(devNum, pBufferGroup->streamType, pBufferGroup->streamId,
						copyStartTime, copyStopTime);
}

HevcStreamTask* hevcStreamEnqueue(uint32_t devNum, HevcStreamInfo* pStrInfo)
{
	HevcModuleParams* pModParams = hevcGetModuleParams();
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	HevcStreamBufferGroup* pBufferGroup;
	uint32_t streamType;
	uint32_t streamId;
	uint32_t taskNum;
	uint32_t queueLevel;
	HevcStreamTask* pStrTask = NULL;
	int it;

	if((pDevParams == NULL) ||
	   (pStrInfo == NULL)) return NULL;

	streamType = pStrInfo->streamType;
	streamId = pStrInfo->streamId;
	pBufferGroup = pStrInfo->pBufferGroup;

	if((streamType >= FMB_STREAM_TYPE_MAX) ||
	   (streamId >= FMB_STREAM_ID_MAX) ||
	   (pBufferGroup == NULL)) return NULL;

	ntv2SpinLockAcquire(&pDevParams->strQueueLock[streamType]);

	// save the initial queue level
	queueLevel = hevcStreamQueueLevel(devNum, streamType);

	// queue the task to the next ready entry
	taskNum = pDevParams->strQueueReady[streamType];
	for(it = 0; it < HEVC_STREAM_TASK_MAX; it++)
	{
		if(!pDevParams->strQueue[streamType][taskNum].apiReady) break;
		taskNum = (taskNum + 1)%HEVC_STREAM_TASK_MAX;
	}

	if(!pDevParams->strQueue[streamType][taskNum].apiReady)
	{
		// copy stream info and set status flags
		pStrTask = &pDevParams->strQueue[streamType][taskNum];
		pStrTask->apiReady = true;
		pStrTask->dmaSent = false;
		pStrTask->ackReceived = false;
		pStrTask->msgReceived = false;
		pStrTask->apiDone = false;
		pStrTask->cntCount = 0;
		pStrTask->seqNum = pDevParams->strQueueCount[streamType];
		pStrTask->strInfo = *pStrInfo;
		memset(&pStrTask->ackInfo, 0, sizeof(HevcStreamAckInfo));
		memset(&pStrTask->msgInfo, 0, sizeof(HevcStreamMsgInfo));
		ntv2EventClear(&pStrTask->ackEvent);
		ntv2EventClear(&pStrTask->msgEvent);

		// increment the ready point and update stats
		pDevParams->strQueueReady[streamType] = taskNum;
		pDevParams->strQueueCount[streamType]++;
		pDevParams->strQueueLevel[streamType] = hevcStreamQueueLevel(devNum, streamType);
	}

	ntv2SpinLockRelease(&pDevParams->strQueueLock[streamType]);

	if(pStrTask != NULL)
	{
		// record enqueue time
		pStrTask->enqueueTime = ntv2TimeCounter();
		pStrTask->sendTime = pStrTask->enqueueTime;
		pStrTask->ackTime = pStrTask->enqueueTime;
		pStrTask->msgTime = pStrTask->enqueueTime;
		pStrTask->dequeueTime = pStrTask->enqueueTime;

		HEVC_MSG_STREAM_STATE("%s(%d): hevcStreamEnqueue()  stream type %d  id %d  task %d  buf %d  qlevel %d\n",
							  pModParams->pModuleName, devNum, streamType, streamId, taskNum,
							  pBufferGroup->bufNum, queueLevel);
	}
	else
	{
		HEVC_MSG_STREAM_ERROR("%s(%d): hevcStreamEnqueue()  *error* stream type %d  id %d  qlevel %d  queue is full\n",
							  pModParams->pModuleName, devNum, streamType, streamId, queueLevel);
	}

	// try to send queued dmas
	ntv2DpcSchedule(&pDevParams->strQueueDpc[streamType]);

	return pStrTask;
}

bool hevcStreamAckWait(uint32_t devNum, HevcStreamTask* pStrTask)
{
	HevcModuleParams* pModParams = hevcGetModuleParams();
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	bool success;

	if((pDevParams == NULL) ||
	   (pStrTask == NULL)) return false;

	success = ntv2EventWaitForSignal(&pStrTask->ackEvent, HEVC_STREAM_TIMEOUT, true);
	if(!success)
	{
		HEVC_MSG_STREAM_ERROR("%s(%d): hevcStreamAckWait()  *error* stream type %d  id %d  cnt %d  task %d  ack wait timeout\n",
							  pModParams->pModuleName, devNum, pStrTask->strInfo.streamType,
							  pStrTask->strInfo.streamId, pStrTask->cntCount, pStrTask->taskNum);
	}

	return success;
}

bool hevcStreamMsgWait(uint32_t devNum, HevcStreamTask* pStrTask)
{
	HevcModuleParams* pModParams = hevcGetModuleParams();
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	bool success;

	if((pDevParams == NULL) ||
	   (pStrTask == NULL)) return false;

	success = ntv2EventWaitForSignal(&pStrTask->msgEvent, HEVC_STREAM_TIMEOUT, true);
	if(!success)
	{
		HEVC_MSG_STREAM_ERROR("%s(%d): hevcStreamMsgWait()  *error* stream type %d  id %d  cnt %d  task %d  msg wait timeout\n",
							  pModParams->pModuleName, devNum, pStrTask->strInfo.streamType,
							  pStrTask->strInfo.streamId, pStrTask->cntCount, pStrTask->taskNum);
	}

	return success;
}

bool hevcStreamDequeue(uint32_t devNum, HevcStreamTask* pStrTask)
{
	HevcModuleParams* pModParams = hevcGetModuleParams();
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	bool ready = false;
	uint32_t streamType;
	uint32_t streamId;
	uint32_t taskNum = 0;
	uint32_t bufNum = 0;
	
	if((pDevParams == NULL) ||
	   (pStrTask == NULL)) return false;

	streamType = pStrTask->strInfo.streamType;
	streamId = pStrTask->strInfo.streamId;

	if((streamType >= FMB_STREAM_TYPE_MAX) ||
	   (streamId >= FMB_STREAM_ID_MAX)) return false;

	// record dequeue time
	pStrTask->dequeueTime = ntv2TimeCounter();

	ntv2SpinLockAcquire(&pDevParams->strQueueLock[streamType]);

	if(pStrTask->apiReady)
	{
		pStrTask->apiDone = true;
		ready = true;

		// save data for report
		taskNum = pStrTask->taskNum;
		bufNum = HEVC_STREAM_BUFFER_MAX;
		if(pStrTask->strInfo.pBufferGroup != NULL)
		{
			bufNum = pStrTask->strInfo.pBufferGroup->bufNum;
		}
	}

	ntv2SpinLockRelease(&pDevParams->strQueueLock[streamType]);

	// check for task done
	hevcStreamDone(devNum, pStrTask, false);

	if(ready)
	{
		HEVC_MSG_STREAM_STATE("%s(%d): hevcStreamDequeue()  stream type %d  id %d  task %d  buf %d\n",
							  pModParams->pModuleName, devNum, streamType, streamId, taskNum, bufNum);
	}
	else
	{
		HEVC_MSG_STREAM_ERROR("%s(%d): hevcStreamDequeue()  *error* stream type %d  id %d  task %d not ready\n",
							  pModParams->pModuleName, devNum, streamType, streamId, taskNum);
	}

	return ready;
}

void hevcStreamOrphan(uint32_t devNum, uint32_t streamType)
{
	HevcModuleParams* pModParams = hevcGetModuleParams();
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	HevcStreamTask* pStrTask = NULL;
	uint32_t it;

	// not sure how to do this yet so always return
	if(devNum < 1000) return;

	// this algorithm did not work
	if((pDevParams == NULL) ||
	   (streamType >= FMB_STREAM_TYPE_MAX)) return;

	// search for an orphan stream task
	for(it = 0; it < HEVC_STREAM_TASK_MAX; it++)
	{
		pStrTask = &pDevParams->strQueue[streamType][it];
		if(pStrTask->apiReady &&
		   (pStrTask->seqNum < (pDevParams->strQueueCount[streamType] - 2*HEVC_STREAM_TASK_MAX)))
		{
			HEVC_MSG_STREAM_ERROR("%s(%d): hevcStreamOrphan()  *error* orphan task  type %d  id %d  cnt %d  snd %d  ack %d  msg %d  api %d\n",
								  pModParams->pModuleName, devNum, streamType, pStrTask->strInfo.streamId, pStrTask->cntCount,
								  pStrTask->dmaSent, pStrTask->ackReceived, pStrTask->msgReceived, pStrTask->apiDone);

			hevcStreamDone(devNum, pStrTask, true);
		}
	}
}

void hevcStreamAck(uint32_t devNum, HevcStreamAckInfo* pAckInfo)
{
	HevcModuleParams* pModParams = hevcGetModuleParams();
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	HevcStreamTask* pStrTask = NULL;
	uint32_t streamType;
	uint32_t streamId;
	uint32_t it;
	int64_t ackTime = 0;
	uint32_t count = 0;
	uint32_t task = HEVC_STREAM_TASK_MAX;
	uint32_t dupTask = HEVC_STREAM_TASK_MAX;

	if((pDevParams == NULL) ||
	   (pAckInfo == NULL)) return;

	streamType = pAckInfo->streamType;
	streamId = pAckInfo->streamId;

	if(streamType >= FMB_STREAM_TYPE_MAX) return;
	   
	if(streamId >= FMB_STREAM_ID_MAX)
	{
		HEVC_MSG_STREAM_ERROR("%s(%d): hevcStreamAck()  *error* bad stream id  type %d  id %d  cnt %d  result 0x%08x\n",
							  pModParams->pModuleName, devNum, streamType, streamId,
							  pAckInfo->cntCount, pAckInfo->result0);
		return;
	}

	if(pAckInfo->result0 != FMB_DMA_RESULT_OK)
	{
		HEVC_MSG_STREAM_ERROR("%s(%d): hevcStreamAck()  *error* dma failed  type %d  id %d  cnt %d  result 0x%08x\n",
							  pModParams->pModuleName, devNum, streamType, streamId,
							  pAckInfo->cntCount, pAckInfo->result0);
		return;
	}

	// get the ack time
	ackTime = ntv2TimeCounter();

	ntv2SpinLockAcquire(&pDevParams->strQueueLock[streamType]);

	// search for a matching stream task
	for(it = 0; it < HEVC_STREAM_TASK_MAX; it++)
	{
		if((pDevParams->strQueue[streamType][it].strInfo.streamId == streamId) &&
		   pDevParams->strQueue[streamType][it].apiReady &&
		   pDevParams->strQueue[streamType][it].dmaSent &&
		   !pDevParams->strQueue[streamType][it].ackReceived)
		{
			if(pStrTask == NULL) 
			{
				pStrTask = &pDevParams->strQueue[streamType][it];
				task = it;
			}
			else
			{
				dupTask = it;
			}
		}
	}

	if(pStrTask != NULL)
	{
		if(pStrTask->cntCount != pAckInfo->cntCount) count = pStrTask->cntCount;

		// copy ack info to task and set received status
		pStrTask->ackInfo = *pAckInfo;
		pStrTask->ackReceived = true;
		pStrTask->ackTime = ackTime;
		ntv2EventSignal(&pStrTask->ackEvent);
	}

	ntv2SpinLockRelease(&pDevParams->strQueueLock[streamType]);

	if(pStrTask != NULL)
	{
		HEVC_MSG_STREAM_STATE("%s(%d): hevcStreamAck()  ack received  stream type %d  id %d  task %d  cnt %d  result 0x%08x\n",
							  pModParams->pModuleName, devNum, streamType, streamId, task,
							  pAckInfo->cntCount, pAckInfo->result0);
		// report duplicate
		if(dupTask != HEVC_STREAM_TASK_MAX)
		{
			HEVC_MSG_STREAM_ERROR("%s(%d): hevcStreamAck()  *error* duplicate task  type %d  id %d  task %d  cnt %d  dupTask %d\n",
								  pModParams->pModuleName, devNum, streamType, streamId, task,
								  pAckInfo->cntCount, dupTask);
		}
		// report bad continuity count
		if(count != 0)
		{
			HEVC_MSG_STREAM_ERROR("%s(%d): hevcStreamAck()  *error* count match failed  type %d  id %d  task %d  ackCnt %d  taskCnt %d\n",
								  pModParams->pModuleName, devNum, streamType, streamId, task,
								  pAckInfo->cntCount, count);
		}
		// check for task done
		hevcStreamDone(devNum, pStrTask, false);
	}
	else
	{
		HEVC_MSG_STREAM_ERROR("%s(%d): hevcStreamAck()  *error* task not found  type %d  id %d  cnt %d  result 0x%08x\n",
							  pModParams->pModuleName, devNum, streamType, streamId,
							  pAckInfo->cntCount, pAckInfo->result0);
	}

	// try to send queued transfers
	ntv2DpcSchedule(&pDevParams->strQueueDpc[streamType]);
}

void hevcStreamMsg(uint32_t devNum, HevcStreamMsgInfo* pMsgInfo)
{
	HevcModuleParams* pModParams = hevcGetModuleParams();
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	HevcStreamTask* pStrTask = NULL;
	uint32_t streamType;
	uint32_t streamId;
	uint32_t it;
	int64_t msgTime = 0;
	uint32_t task = HEVC_STREAM_TASK_MAX;
	uint32_t count = 0;

	if((pDevParams == NULL) ||
	   (pMsgInfo == NULL)) return;

	streamType = pMsgInfo->streamType;
	streamId = pMsgInfo->streamId;

	if(streamType >= FMB_STREAM_TYPE_MAX) return;

	if(streamId >= FMB_STREAM_ID_MAX)
	{
		HEVC_MSG_STREAM_ERROR("%s(%d): hevcStreamMsg()  *error* bad stream id  type %d  id %d  cnt %d  result 0x%08x  last 0x%08x\n",
							  pModParams->pModuleName, devNum, streamType, streamId, 
							  pMsgInfo->cntCount, pMsgInfo->result0, pMsgInfo->lastFrame);
		return;
	}

	if(pMsgInfo->result0 != FMB_DMA_RESULT_OK)
	{
		HEVC_MSG_STREAM_ERROR("%s(%d): hevcStreamMsg()  *error* dma failed  type %d  id %d  cnt %d  result 0x%08x  last 0x%08x\n",
							  pModParams->pModuleName, devNum, streamType, streamId,
							  pMsgInfo->cntCount, pMsgInfo->result0, pMsgInfo->lastFrame);
		return;
	}

	// get the msg time
	msgTime = ntv2TimeCounter();
					
	ntv2SpinLockAcquire(&pDevParams->strQueueLock[streamType]);

	// search for a matching stream task with lowest sequence number
	for(it = 0; it < HEVC_STREAM_TASK_MAX; it++)
	{
		if((pDevParams->strQueue[streamType][it].strInfo.streamId == streamId) &&
		   pDevParams->strQueue[streamType][it].apiReady &&
		   pDevParams->strQueue[streamType][it].dmaSent &&
		   !pDevParams->strQueue[streamType][it].msgReceived)
		{
			if(pStrTask == NULL) 
			{
				pStrTask = &pDevParams->strQueue[streamType][it];
				task = it;
			}
			else
			{
				if(pDevParams->strQueue[streamType][it].seqNum < pStrTask->seqNum)
				{
					pStrTask = &pDevParams->strQueue[streamType][it];
					task = it;
				}
			}
		}
	}

	if(pStrTask != NULL)
	{
		if(pStrTask->cntCount != pMsgInfo->cntCount) count = pStrTask->cntCount;

		// copy msg info to task and set received status
		pStrTask->msgInfo = *pMsgInfo;
		pStrTask->msgReceived = true;
		pStrTask->msgTime = msgTime;
	
		if(streamType == FMB_STREAM_TYPE_SEO)
		{
			// set bytes transferred to buffer
			if(pStrTask->strInfo.pBufferGroup != NULL)
			{
				pStrTask->strInfo.pBufferGroup->videoDataSize = pMsgInfo->videoDataSize;
				pStrTask->strInfo.pBufferGroup->infoDataSize = pMsgInfo->infoDataSize;
			}
			// update last frame flag
			pStrTask->strInfo.isLastFrame =	(pMsgInfo->lastFrame == FMB_REG_DMA_SEO_LAST_ES);
		}

		ntv2EventSignal(&pStrTask->msgEvent);
	}

	ntv2SpinLockRelease(&pDevParams->strQueueLock[streamType]);

	if(pStrTask != NULL)
	{
		HEVC_MSG_STREAM_STATE("%s(%d): hevcStreamMsg()  msg received  type %d  id %d  task %d  cnt %d  vid_size 0x%08x  info_size %08x  result 0x%08x  last 0x%08x\n",
							  pModParams->pModuleName, devNum, streamType, streamId, task,
							  pMsgInfo->cntCount, pMsgInfo->videoDataSize, pMsgInfo->infoDataSize,
							  pMsgInfo->result0, pMsgInfo->lastFrame);
		// report bad continuity count
		if(count != 0)
		{
			HEVC_MSG_STREAM_ERROR("%s(%d): hevcStreamMsg()  *error* count match failed  type %d  id %d  task %d  msgCnt %d  taskCnt %d\n",
								  pModParams->pModuleName, devNum, streamType, streamId, task,
								  pMsgInfo->cntCount, count);
		}
		// check for task done
		hevcStreamDone(devNum, pStrTask, false);
	}
	else
	{
		HEVC_MSG_STREAM_ERROR("%s(%d): hevcStreamMsg()  *error* task not found  type %d  id %d  cnt %d  result 0x%08x  last 0x%08x\n",
							  pModParams->pModuleName, devNum, streamType, streamId,
							  pMsgInfo->cntCount, pMsgInfo->result0, pMsgInfo->lastFrame);
	}

	// try to send queued transfers
	ntv2DpcSchedule(&pDevParams->strQueueDpc[streamType]);
}

bool hevcStreamFrameReady(uint32_t devNum, HevcFrameData* pFrameData)
{
	HevcModuleParams* pModParams = hevcGetModuleParams();
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	uint32_t streamId;
	uint32_t next;

	if((pDevParams == NULL) ||
	   (pFrameData == NULL)) return false;

	streamId = pFrameData->streamId;
	if(streamId >= FMB_STREAM_ID_MAX) return false;

	HEVC_MSG_STREAM_FRAME("%s(%d): hevcStreamFrameReady()   stream id %d  serial %d  itc %02x:%08x  ext %08x  time %lld (100ns)\n",
						  pModParams->pModuleName, devNum, streamId,
						  pFrameData->syncCount, pFrameData->itcValueHigh,
						  pFrameData->itcValueLow, pFrameData->itcExtension,
						  pFrameData->encodeTime);

	ntv2SpinLockAcquire(&pDevParams->strFrameLock[streamId]);

	// check for queue full
	next = (pDevParams->strFrameReady[streamId] + 1)%HEVC_FRAME_DATA_MAX;
	if(next == pDevParams->strFrameDone[streamId])
	{
		ntv2SpinLockRelease(&pDevParams->strFrameLock[streamId]);
		HEVC_MSG_STREAM_ERROR("%s(%d): hevcStreamFrameReady()  *error* codec frame queue full - stream id %d\n",
							  pModParams->pModuleName, devNum, streamId);
		return false;
	}

	// data to queue
	pDevParams->strFrameQueue[streamId][pDevParams->strFrameReady[streamId]] = *pFrameData;
	pDevParams->strFrameReady[streamId] = next;

	pDevParams->strFrameQueueLevel[streamId] =
		(pDevParams->strFrameReady[streamId] + HEVC_FRAME_DATA_MAX -
		 pDevParams->strFrameDone[streamId]) % HEVC_FRAME_DATA_MAX;

	ntv2SpinLockRelease(&pDevParams->strFrameLock[streamId]);

	return true;
}

bool hevcStreamFrameDone(uint32_t devNum, HevcFrameData* pFrameData)
{
	HevcModuleParams* pModParams = hevcGetModuleParams();
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	uint32_t streamId;

	if((pDevParams == NULL) ||
	   (pFrameData == NULL)) return false;

	streamId = pFrameData->streamId;
	if(streamId >= FMB_STREAM_ID_MAX) return false;

	ntv2SpinLockAcquire(&pDevParams->strFrameLock[streamId]);

	// check for queue empty
	if(pDevParams->strFrameDone[streamId] == pDevParams->strFrameReady[streamId])
	{
		ntv2SpinLockRelease(&pDevParams->strFrameLock[streamId]);
		HEVC_MSG_STREAM_ERROR("%s(%d): hevcStreamFrameDone()  *error* codec data queue empty - stream id %d\n",
							  pModParams->pModuleName, devNum, streamId);
		return false;
	}

	// data from queue
	*pFrameData = pDevParams->strFrameQueue[streamId][pDevParams->strFrameDone[streamId]];
	pDevParams->strFrameDone[streamId] = (pDevParams->strFrameDone[streamId] + 1)%HEVC_FRAME_DATA_MAX;

	pDevParams->strFrameQueueLevel[streamId] =
		(pDevParams->strFrameReady[streamId] + HEVC_FRAME_DATA_MAX -
		 pDevParams->strFrameDone[streamId]) % HEVC_FRAME_DATA_MAX;

	ntv2SpinLockRelease(&pDevParams->strFrameLock[streamId]);

	HEVC_MSG_STREAM_FRAME("%s(%d): hevcStreamFrameDone()    stream id %d  serial %d  itc %02x:%08x  ext %08x  time %lld (100ns)\n",
						  pModParams->pModuleName, devNum, streamId,
						  pFrameData->syncCount, pFrameData->itcValueHigh,
						  pFrameData->itcValueLow, pFrameData->itcExtension,
						  pFrameData->encodeTime);

	return true;
}

void hevcStreamFrameReset(uint32_t devNum, uint32_t streamBits)
{
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	int i;

	if(pDevParams == NULL) return;

	// clear queue for streams with bit set
	for(i = 0; i < HEVC_STREAM_MAX; i++)
	{
		if((streamBits & (1 << i)) != 0)
		{
			// reset queue pointers
			pDevParams->strFrameReady[i] = 0;
			pDevParams->strFrameDone[i] = 0;
			pDevParams->strFrameQueueLevel[i] = 0;
		}
	}
}

static void hevcStreamVeiDpc(unsigned long data)
{
	hevcStreamSend((uint32_t)data, FMB_STREAM_TYPE_VEI);
}

static void hevcStreamSeoDpc(unsigned long data)
{
	hevcStreamSend((uint32_t)data, FMB_STREAM_TYPE_SEO);
}

static void hevcStreamSend(uint32_t devNum, uint32_t streamType)
{
	HevcModuleParams* pModParams = hevcGetModuleParams();
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	HevcStreamInfo strInfo;
	HevcStreamTask* pStrTask = NULL;
	uint32_t strNext[HEVC_STREAM_MAX];
	uint32_t taskNum = 0;
	uint32_t streamId = 0;
	uint32_t bufNum = 0;
	uint32_t count = 0;
	bool busy = false;
	bool send = false;
	int id; 
	int it;

	if(pDevParams == NULL) return;

	for(id = 0; id < HEVC_STREAM_MAX; id++)
	{
		strNext[id] = HEVC_STREAM_TASK_MAX;
	}

	ntv2SpinLockAcquire(&pDevParams->strQueueLock[streamType]);

	// scan queue for ready tasks not sent for each stream
	taskNum = pDevParams->strQueueSend[streamType];
	for(it = 0; it < HEVC_STREAM_TASK_MAX; it++)
	{
		pStrTask = &pDevParams->strQueue[streamType][taskNum];
		// if a raw dma has not been acked then busy
		if(pStrTask->apiReady &&
		   pStrTask->dmaSent &&
		   !pStrTask->ackReceived) 
		{
			busy = true;
			break;
		}
		
		// look for tasks not sent to codec
		if(pStrTask->apiReady && !pStrTask->dmaSent)
		{
			// determine the dma task with lowest sequence number for each stream
			if(strNext[pStrTask->strInfo.streamId] < HEVC_STREAM_TASK_MAX)
			{
				if(pStrTask->seqNum < pDevParams->strQueue[streamType][strNext[pStrTask->strInfo.streamId]].seqNum)
				{
					strNext[pStrTask->strInfo.streamId] = taskNum;
				}
			}
			else
			{
				strNext[pStrTask->strInfo.streamId] = taskNum;
			}
		}
		taskNum = (taskNum + 1)%HEVC_STREAM_TASK_MAX;
	}

	if(!busy)
	{
		// use round robin to determine which stream task to send
		streamId = pDevParams->strQueueId[streamType];
		for(id = 0; id < HEVC_STREAM_MAX; id++)
		{
			streamId = (streamId + 1)%HEVC_STREAM_MAX;
			// if this stream has a task ready to send
			if(strNext[streamId] < HEVC_STREAM_TASK_MAX)
			{
				// found a task to send
				pStrTask = &pDevParams->strQueue[streamType][strNext[streamId]]; 
				// copy the stream info
				strInfo = pStrTask->strInfo;
				// mark task as transfer sent
				pStrTask->dmaSent = true;
				// update send and round robin pointer
				pDevParams->strQueueSend[streamType] = pStrTask->taskNum;
				pDevParams->strQueueId[streamType] = streamId;
				// send this transfer
				send = true;
				// save info for report
				taskNum = pStrTask->taskNum;
				if(pStrTask->strInfo.pBufferGroup != NULL)
				{
					bufNum = pStrTask->strInfo.pBufferGroup->bufNum;
				}
				break;
			}
		}
	}

	ntv2SpinLockRelease(&pDevParams->strQueueLock[streamType]);

	if(send)
	{
		// submit transfer (1000+ registers)
		switch(streamType)
		{ 
		case FMB_STREAM_TYPE_VEI:
			// setup transfer
			send = hevcSetupVeiTransfer(devNum, &strInfo, &count);
			if(send)
			{
				// record continuity count
				pStrTask->cntCount = count;
				// record send time for stats
				pStrTask->sendTime = ntv2TimeCounter();
				// submit transfer
				hevcSubmitVeiTransfer(devNum);
			}
			
			break;
		case FMB_STREAM_TYPE_SEO:
			// setup transfer
			send = hevcSetupSeoTransfer(devNum, &strInfo, &count);
			if(send)
			{
				// record continuity count
				pStrTask->cntCount = count;
				// record send time for stats
				pStrTask->sendTime = ntv2TimeCounter();
				// submit transfer
				hevcSubmitSeoTransfer(devNum);
			}
			break;
		default:
			break;
		}

		if(send)
		{
			HEVC_MSG_STREAM_STATE("%s(%d): hevcStreamSend()  send stream type %d  id %d  task %d  cnt %d  buf %d\n",
								  pModParams->pModuleName, devNum, streamType, streamId,
								  taskNum, count, bufNum);
		}
		else
		{
			HEVC_MSG_STREAM_ERROR("%s(%d): hevcStreamSend()  *error* send failed  stream type %d  id %d  task %d  cnt %d  buf %d\n",
								  pModParams->pModuleName, devNum, streamType, streamId,
								  taskNum, count, bufNum);
		}
	}

	return;
}

static bool hevcStreamDone(uint32_t devNum, HevcStreamTask* pStrTask, bool force)
{
	HevcModuleParams* pModParams = hevcGetModuleParams();
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	HevcStreamBufferGroup* pBufferGroup = NULL;
	bool done = false;
	bool doStats = false;
	uint32_t streamType;
	uint32_t streamId;
	uint32_t taskNum = 0;
	uint32_t bufNum = 0;
	int64_t enqueueTime = 0;
	int64_t sendTime = 0;
	int64_t ackTime = 0;
	int64_t msgTime = 0;
	int64_t dequeueTime = 0;
	
	if((pDevParams == NULL) ||
	   (pStrTask == NULL)) return false;

	streamType = pStrTask->strInfo.streamType;
	streamId = pStrTask->strInfo.streamId;

	if((streamType >= FMB_STREAM_TYPE_MAX) ||
	   (streamId >= FMB_STREAM_ID_MAX)) return false;

	ntv2SpinLockAcquire(&pDevParams->strQueueLock[streamType]);

	// check for done
	if((pStrTask->apiReady &&
		pStrTask->apiDone &&
		pStrTask->dmaSent &&
		pStrTask->ackReceived &&
		pStrTask->msgReceived) || force)
	{
		// clear status flags
		pStrTask->apiReady = false;
		pStrTask->apiDone = true;
		pStrTask->dmaSent = true;
		pStrTask->ackReceived = true;
		pStrTask->msgReceived = true;

		// save data for report
		taskNum = pStrTask->taskNum;
		bufNum = HEVC_STREAM_BUFFER_MAX;
		if(pStrTask->strInfo.pBufferGroup != NULL)
		{
			bufNum = pStrTask->strInfo.pBufferGroup->bufNum;
		}

		// release buffer
		if(streamType == FMB_STREAM_TYPE_VEI)
		{
			pBufferGroup = pStrTask->strInfo.pBufferGroup;
			pStrTask->strInfo.pBufferGroup = NULL;
		}

		// update queue level
		pDevParams->strQueueLevel[streamType] = hevcStreamQueueLevel(devNum, streamType);

		// compute stats for good dma
		if(pStrTask->msgInfo.result0 == FMB_DMA_RESULT_OK)
		{
			enqueueTime = pStrTask->enqueueTime;
			sendTime = pStrTask->sendTime;
			ackTime = pStrTask->ackTime;
			msgTime = pStrTask->msgTime;
			dequeueTime = pStrTask->dequeueTime;
			doStats = true;
		}

		// task done
		done = true;
	}

	ntv2SpinLockRelease(&pDevParams->strQueueLock[streamType]);

	// release buffer
	if(pBufferGroup != NULL)
	{
		hevcStreamReleaseBuffer(devNum, pBufferGroup);
	}

	// log state
	if(done)
	{
		if(force)
		{
			HEVC_MSG_STREAM_ERROR("%s(%d): hevcStreamDone()  force done  type %d  id %d  task %d  buf %d\n",
								  pModParams->pModuleName, devNum, streamType, streamId, taskNum, bufNum);
		}
		else
		{
			HEVC_MSG_STREAM_STATE("%s(%d): hevcStreamDone()  task done  type %d  id %d  task %d  buf %d\n",
								  pModParams->pModuleName, devNum, streamType, streamId, taskNum, bufNum);
		}
	}

	// update stats
	if(doStats)
	{
		hevcStreamDmaStats(devNum, streamType, streamId,
						   enqueueTime, sendTime, ackTime, msgTime, dequeueTime);
	}

	return done;
}

static void hevcStreamAllocateBufferGroup(uint32_t devNum, uint32_t streamType, uint32_t streamId, uint32_t bufNum)
{
	HevcModuleParams* pModParams = hevcGetModuleParams();
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	HevcStreamBufferGroup* pBufferGroup;
	uint32_t bufferSize = 0;
	uint32_t allocSize = 0;
	uint32_t allocCount = 0;
	uint32_t infoSize = 0;
	uint32_t ib;

	if((pDevParams == NULL) ||
	   (streamType >= FMB_STREAM_TYPE_MAX) ||
	   (streamId >= FMB_STREAM_ID_MAX) ||
	   (bufNum >= HEVC_STREAM_BUFFER_MAX)) return;

	// initalize buffer data
	pBufferGroup = &pDevParams->strBuffers[streamType][streamId][bufNum];
	pBufferGroup->streamType = streamType;
	pBufferGroup->streamId = streamId;
	pBufferGroup->bufNum = bufNum;

	// determine video buffer size and number of buffers
	switch(streamType)
	{
	case FMB_STREAM_TYPE_VEI:
		if (streamId == 0)
		{
			bufferSize = FMB_TRANSFER_VEI_BUFFER_SIZE_CH0;
		}
		else
		{
			bufferSize = FMB_TRANSFER_VEI_BUFFER_SIZE;
		}
		allocCount = FMB_REG_DMA_VEI_DESCRIPTOR_NUM;
		infoSize = FMB_TRANSFER_VEI_PIC_INFO_MAX_SIZE;
		break;
	case FMB_STREAM_TYPE_SEO:
		bufferSize = FMB_TRANSFER_SEO_BUFFER_SIZE;
		allocCount = FMB_REG_DMA_SEO_DESCRIPTOR_NUM;
		infoSize = FMB_TRANSFER_SEO_ES_INFO_SIZE;
		break;
	default:
		return;
	}

	// limit number of buffers to max
	if(allocCount >= HEVC_STREAM_DESCRIPTOR_MAX) allocCount = HEVC_STREAM_DESCRIPTOR_MAX;

	// record video buffer size
	pBufferGroup->videoBufferSize = bufferSize;
	pBufferGroup->videoDataSize = 0;

	// determine allocation size (round up to page size)
	allocSize = bufferSize / allocCount;
	allocSize = (allocSize + PAGE_SIZE - 1) / PAGE_SIZE * PAGE_SIZE;

	HEVC_MSG_STREAM_INFO("%s(%d): hevcStreamAllocateBufferGroup()  stream type %d  id %d  num %d  vidSize %d  allocSize %d  infoSize %d\n",
						 pModParams->pModuleName, devNum, streamType, streamId, bufNum, bufferSize, allocSize, infoSize);

	// allocate the buffers
	if(allocSize != 0)
	{
		for(ib = 0; ib < allocCount; ib++)
		{
			hevcStreamAllocateBuffer(devNum, allocSize, &pBufferGroup->videoBuffers[ib]);
		}
	}

	// record info buffer size
	pBufferGroup->infoBufferSize = infoSize;
	pBufferGroup->infoDataSize = 0;

	// allocate info buffer
	if(infoSize != 0)
	{
		hevcStreamAllocateBuffer(devNum, infoSize, &pBufferGroup->infoBuffer);
	}
}

static void hevcStreamFreeBufferGroup(uint32_t devNum, uint32_t streamType, uint32_t streamId, uint32_t bufNum)
{
	HevcModuleParams* pModParams = hevcGetModuleParams();
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	HevcStreamBufferGroup* pBufferGroup;
	uint32_t ib;

	if((pDevParams == NULL) ||
	   (streamType >= FMB_STREAM_TYPE_MAX) ||
	   (streamId >= FMB_STREAM_ID_MAX) ||
	   (bufNum >= HEVC_STREAM_BUFFER_MAX)) return;

	pBufferGroup = &pDevParams->strBuffers[streamType][streamId][bufNum];

	HEVC_MSG_STREAM_INFO("%s(%d): hevcStreamFreeBufferGroup()  stream type %d  id %d  num %d\n",
						 pModParams->pModuleName, devNum,
						 pBufferGroup->streamType, pBufferGroup->streamId, pBufferGroup->bufNum);

	// free all video buffers
	for(ib = 0; ib < HEVC_STREAM_DESCRIPTOR_MAX; ib++)
	{
		hevcStreamFreeBuffer(devNum, &pBufferGroup->videoBuffers[ib]);
	}

	// free es info buffers
	hevcStreamFreeBuffer(devNum, &pBufferGroup->infoBuffer);
}

static void hevcStreamAllocateBuffer(uint32_t devNum, uint32_t size, HevcStreamBuffer* pStreamBuffer)
{
	HevcModuleParams* pModParams = hevcGetModuleParams();
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);

	if((pDevParams == NULL) ||
	   (size == 0) ||
	   (pStreamBuffer == NULL)) return;

	// clear stream buffer object
	memset(pStreamBuffer, 0, sizeof(HevcStreamBuffer));

	// allocate contiguous buffer
	ntv2DmaMemoryAlloc(&pStreamBuffer->dmaMemory, &pDevParams->systemContext, size);
	pStreamBuffer->pBufferAddress = ntv2DmaMemoryVirtual(&pStreamBuffer->dmaMemory);
	pStreamBuffer->dmaAddress = ntv2DmaMemoryPhysical(&pStreamBuffer->dmaMemory);

	if((pStreamBuffer->pBufferAddress != NULL) && (pStreamBuffer->dmaAddress != 0))
	{
		pStreamBuffer->bufferSize = size;
		HEVC_MSG_MEMORY_ALLOC("%s(%d): hevcStreamAllocateBuffer()  allocate buffer size %d  address 0x%p  dma 0x%llx\n",
							  pModParams->pModuleName, devNum,
							  pStreamBuffer->bufferSize, pStreamBuffer->pBufferAddress, pStreamBuffer->dmaAddress);


		if((pStreamBuffer->dmaAddress % FMB_TRANSFER_ALIGNMENT_SIZE) != 0)
		{
			HEVC_MSG_MEMORY_ERROR("%s(%d): hevcStreamAllocateBuffer()  *error* buffer not transfer aligned (%08x)\n",
								  pModParams->pModuleName, devNum, FMB_TRANSFER_ALIGNMENT_SIZE);
		}
	}
	else
	{
		HEVC_MSG_MEMORY_ERROR("%s(%d): hevcStreamAllocateBuffer()  *error* allocate buffer size %08x failed\n",
							  pModParams->pModuleName, devNum, size);
	}
}

static void hevcStreamFreeBuffer(uint32_t devNum, HevcStreamBuffer* pStreamBuffer)
{
	HevcModuleParams* pModParams = hevcGetModuleParams();
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);

	if((pDevParams == NULL) ||
	   (pStreamBuffer == NULL)) return;

	// free contiguous buffer
	if((pStreamBuffer->pBufferAddress != NULL) &&
	   (pStreamBuffer->bufferSize != 0))
	{
		ntv2DmaMemoryFree(&pStreamBuffer->dmaMemory);

		HEVC_MSG_MEMORY_ALLOC("%s(%d): hevcStreamFreeBuffer()  free buffer size %d  address 0x%px  dma 0x%llx\n",
							  pModParams->pModuleName, devNum, pStreamBuffer->bufferSize,
							  pStreamBuffer->pBufferAddress, pStreamBuffer->dmaAddress);
	}

	// clear stream buffer object
	memset(pStreamBuffer, 0, sizeof(HevcStreamBuffer));
}

void hevcStreamClearAllStats(uint32_t devNum)
{
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	int i;

	if(pDevParams == NULL) return;

	// for each stream clear all stats
	for(i = 0; i < FMB_STREAM_TYPE_MAX; i++)
	{
		hevcStreamClearStats(devNum, i, 0xffffffff);
	}
}

void hevcStreamClearStats(uint32_t devNum, uint32_t streamType, uint32_t streamBits)
{
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	int i;

	if(pDevParams == NULL) return;

	// clear stats for streams with bit set
	for(i = 0; i < HEVC_STREAM_MAX; i++)
	{
		if((streamBits & (1 << i)) != 0)
		{
			ntv2SpinLockAcquire(&pDevParams->strStatLock[streamType][i]);
			memset(&pDevParams->strStats[streamType][i], 0, sizeof(HevcStreamStatistics));
			ntv2SpinLockRelease(&pDevParams->strStatLock[streamType][i]);
		}
	}
}

void hevcStreamTransferStats(uint32_t devNum, uint32_t streamType, uint32_t streamId, uint32_t transferSize)
{
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
//	int64_t duration;
	int64_t time;
	int64_t frequency;
	int64_t delta;
	int64_t tranSize = (int64_t)transferSize;
	int64_t avrSize;
	int64_t avrTime;

	if((pDevParams == NULL) ||
	   (streamType >= FMB_STREAM_TYPE_MAX) ||
	   (streamId >= HEVC_STREAM_MAX)) return;

	// stop stats when device fails
	if(!hevcIsDeviceAlive(devNum)) return;

	// get current time and time frequency
	time = ntv2TimeCounter();
	frequency = ntv2TimeFrequency();

	ntv2SpinLockAcquire(&pDevParams->strStatLock[streamType][streamId]);

	// count transfers
	pDevParams->strStats[streamType][streamId].transferCount++;

	if(pDevParams->strStats[streamType][streamId].transferCount > 2)
	{
		// compute time between transfers
		delta = (time - pDevParams->strStatTransferLastTime[streamType][streamId]) * 1000000000 / frequency;

		// save minimum transfer time
		if(pDevParams->strStats[streamType][streamId].minTransferTime == 0)
			pDevParams->strStats[streamType][streamId].minTransferTime = delta;

		pDevParams->strStats[streamType][streamId].minTransferTime = 
			min(pDevParams->strStats[streamType][streamId].minTransferTime, delta);

		// save maximum transfer time
		pDevParams->strStats[streamType][streamId].maxTransferTime = 
			max(pDevParams->strStats[streamType][streamId].maxTransferTime, delta);

		// determine average transfer time
		avrTime = pDevParams->strStats[streamType][streamId].avrTransferTime;
		if(avrTime != 0) 
		{
			avrTime += (delta - avrTime)/HEVC_STREAM_AVR_FACTOR;
		}
		avrTime = max(avrTime, pDevParams->strStats[streamType][streamId].minTransferTime);
		avrTime = min(avrTime, pDevParams->strStats[streamType][streamId].maxTransferTime);
		pDevParams->strStats[streamType][streamId].avrTransferTime = avrTime;
	}
	else
	{
		pDevParams->strStats[streamType][streamId].minTransferTime = 0;
		pDevParams->strStats[streamType][streamId].avrTransferTime = 0;
		pDevParams->strStats[streamType][streamId].maxTransferTime = 0;
	}

	// save last transfer time
	pDevParams->strStatTransferLastTime[streamType][streamId] = time;

	// save minimum transfer size
	if(pDevParams->strStats[streamType][streamId].minTransferSize == 0)
		pDevParams->strStats[streamType][streamId].minTransferSize = tranSize;

	pDevParams->strStats[streamType][streamId].minTransferSize =
		min(pDevParams->strStats[streamType][streamId].minTransferSize, tranSize);

	// save maximum transfer size
	pDevParams->strStats[streamType][streamId].maxTransferSize =
		max(pDevParams->strStats[streamType][streamId].maxTransferSize, tranSize);

	// determine average transfer size
	avrSize = pDevParams->strStats[streamType][streamId].avrTransferSize;
	if(avrSize != 0) avrSize += (tranSize - avrSize)/HEVC_STREAM_AVR_FACTOR;
	avrSize = max(avrSize, pDevParams->strStats[streamType][streamId].minTransferSize);
	avrSize = min(avrSize, pDevParams->strStats[streamType][streamId].maxTransferSize);
	pDevParams->strStats[streamType][streamId].avrTransferSize = avrSize;

	ntv2SpinLockRelease(&pDevParams->strStatLock[streamType][streamId]);
}

void hevcStreamCopyStats(uint32_t devNum, uint32_t streamType, uint32_t streamId,
						 int64_t copyStartTime, int64_t copyStopTime)
{
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	int64_t frequency;
	int64_t copyDuration;
	int64_t avrDuration;

	if((pDevParams == NULL) ||
	   (streamType >= FMB_STREAM_TYPE_MAX) ||
	   (streamId >= HEVC_STREAM_MAX)) return;

	// stop stats when device fails
	if(!hevcIsDeviceAlive(devNum)) return;

	// get time frequency
	frequency = ntv2TimeFrequency();

	ntv2SpinLockAcquire(&pDevParams->strStatLock[streamType][streamId]);

	// calculate copy duration
	copyDuration = (copyStopTime - copyStartTime) * 1000000000 / frequency;

	// save minimum copy duration
	if(pDevParams->strStats[streamType][streamId].minCopyDuration == 0)
		pDevParams->strStats[streamType][streamId].minCopyDuration = copyDuration;

	pDevParams->strStats[streamType][streamId].minCopyDuration =
		min(pDevParams->strStats[streamType][streamId].minCopyDuration, copyDuration);

	// save maximum copy duration
	pDevParams->strStats[streamType][streamId].maxCopyDuration =
		max(pDevParams->strStats[streamType][streamId].maxCopyDuration, copyDuration);

	// determine average copy duration
	avrDuration = pDevParams->strStats[streamType][streamId].avrCopyDuration;
	if(avrDuration != 0) avrDuration += (copyDuration - avrDuration)/HEVC_STREAM_AVR_FACTOR;
	avrDuration = max(avrDuration, pDevParams->strStats[streamType][streamId].minCopyDuration);
	avrDuration = min(avrDuration, pDevParams->strStats[streamType][streamId].maxCopyDuration);
	pDevParams->strStats[streamType][streamId].avrCopyDuration = avrDuration;

	ntv2SpinLockRelease(&pDevParams->strStatLock[streamType][streamId]);
}

void hevcStreamDmaStats(uint32_t devNum, uint32_t streamType, uint32_t streamId,
						int64_t enqueueTime, int64_t sendTime, int64_t ackTime, int64_t msgTime, int64_t dequeueTime)
{
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	int64_t frequency;
	int64_t enqueueDuration;
	int64_t sendDuration;
	int64_t dmaDuration;
	int64_t dequeueDuration;
	int64_t avrDuration;

	if((pDevParams == NULL) ||
	   (streamType >= FMB_STREAM_TYPE_MAX) ||
	   (streamId >= HEVC_STREAM_MAX)) return;

	// stop stats when device fails
	if(!hevcIsDeviceAlive(devNum)) return;

	// get time frequency
	frequency = ntv2TimeFrequency();

	ntv2SpinLockAcquire(&pDevParams->strStatLock[streamType][streamId]);

	// compute enqueue duration
	enqueueDuration = (sendTime - enqueueTime) * 1000000000 / frequency;

	// save minimum enqueue duration
	if(pDevParams->strStats[streamType][streamId].minEnqueueDuration == 0)
		pDevParams->strStats[streamType][streamId].minEnqueueDuration = enqueueDuration;
	pDevParams->strStats[streamType][streamId].minEnqueueDuration =
		min(pDevParams->strStats[streamType][streamId].minEnqueueDuration, enqueueDuration);
	
	// save maximum enqueue duration
	pDevParams->strStats[streamType][streamId].maxEnqueueDuration =
		max(pDevParams->strStats[streamType][streamId].maxEnqueueDuration, enqueueDuration);

	// determine average enqueue duration
	avrDuration = pDevParams->strStats[streamType][streamId].avrEnqueueDuration;
	if(avrDuration != 0) avrDuration += (enqueueDuration - avrDuration)/HEVC_STREAM_AVR_FACTOR;
	avrDuration = max(avrDuration, pDevParams->strStats[streamType][streamId].minEnqueueDuration);
	avrDuration = min(avrDuration, pDevParams->strStats[streamType][streamId].maxEnqueueDuration);
	pDevParams->strStats[streamType][streamId].avrEnqueueDuration = avrDuration;

	// compute send duration
	sendDuration = (ackTime - sendTime) * 1000000000 / frequency;

	// save minimum send duration
	if(pDevParams->strStats[streamType][streamId].minSendDuration == 0)
		pDevParams->strStats[streamType][streamId].minSendDuration = sendDuration;
	pDevParams->strStats[streamType][streamId].minSendDuration =
		min(pDevParams->strStats[streamType][streamId].minSendDuration, sendDuration);

	// save maximum send duration
	pDevParams->strStats[streamType][streamId].maxSendDuration =
		max(pDevParams->strStats[streamType][streamId].maxSendDuration, sendDuration);

	// determine average send duration
	avrDuration = pDevParams->strStats[streamType][streamId].avrSendDuration;
	if(avrDuration != 0) avrDuration += (sendDuration - avrDuration)/HEVC_STREAM_AVR_FACTOR;
	avrDuration = max(avrDuration, pDevParams->strStats[streamType][streamId].minSendDuration);
	avrDuration = min(avrDuration, pDevParams->strStats[streamType][streamId].maxSendDuration);
	pDevParams->strStats[streamType][streamId].avrSendDuration = avrDuration;

	// compute dma duration
	dmaDuration = (msgTime - ackTime) * 1000000000 / frequency;

	// save minimum dma duration
	if(pDevParams->strStats[streamType][streamId].minDmaDuration == 0)
		pDevParams->strStats[streamType][streamId].minDmaDuration = dmaDuration;
	pDevParams->strStats[streamType][streamId].minDmaDuration =
		min(pDevParams->strStats[streamType][streamId].minDmaDuration, dmaDuration);

	// save maximum dma duration
	pDevParams->strStats[streamType][streamId].maxDmaDuration =
		max(pDevParams->strStats[streamType][streamId].maxDmaDuration, dmaDuration);

	// determine average dma duration
	avrDuration = pDevParams->strStats[streamType][streamId].avrDmaDuration;
	if(avrDuration != 0) avrDuration += (dmaDuration - avrDuration)/HEVC_STREAM_AVR_FACTOR;
	avrDuration = max(avrDuration, pDevParams->strStats[streamType][streamId].minDmaDuration);
	avrDuration = min(avrDuration, pDevParams->strStats[streamType][streamId].maxDmaDuration);
	pDevParams->strStats[streamType][streamId].avrDmaDuration = avrDuration;

	// compute dequeue duration
	dequeueDuration = (dequeueTime - enqueueTime) * 1000000000 / frequency;

	// save minimum dequeue duration
	if(pDevParams->strStats[streamType][streamId].minDequeueDuration == 0)
		pDevParams->strStats[streamType][streamId].minDequeueDuration = dequeueDuration;
	pDevParams->strStats[streamType][streamId].minDequeueDuration =
		min(pDevParams->strStats[streamType][streamId].minDequeueDuration, dequeueDuration);

	// save maximum dequeue duration
	pDevParams->strStats[streamType][streamId].maxDequeueDuration =
		max(pDevParams->strStats[streamType][streamId].maxDequeueDuration, dequeueDuration);

	// determine average dequeue duration
	avrDuration = pDevParams->strStats[streamType][streamId].avrDequeueDuration;
	if(avrDuration != 0) avrDuration += (dequeueDuration - avrDuration)/HEVC_STREAM_AVR_FACTOR;
	avrDuration = max(avrDuration, pDevParams->strStats[streamType][streamId].minDequeueDuration);
	avrDuration = min(avrDuration, pDevParams->strStats[streamType][streamId].maxDequeueDuration);
	pDevParams->strStats[streamType][streamId].avrDequeueDuration = avrDuration;

	ntv2SpinLockRelease(&pDevParams->strStatLock[streamType][streamId]);
}

void hevcStreamGetStats(uint32_t devNum, uint32_t streamType, uint32_t streamId, HevcStreamStatistics* pStats)
{
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);

	if((pDevParams == NULL) ||
	   (streamType >= FMB_STREAM_TYPE_MAX) ||
	   (streamId >= HEVC_STREAM_MAX) ||
	   (pStats == NULL)) return;

	ntv2SpinLockAcquire(&pDevParams->strStatLock[streamType][streamId]);

	// copy stats
	*pStats = pDevParams->strStats[streamType][streamId];

	ntv2SpinLockRelease(&pDevParams->strStatLock[streamType][streamId]);
}

uint32_t hevcStreamGetFrameQueueLevel(uint32_t devNum, uint32_t streamId)
{
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);

	if((pDevParams == NULL) ||
	   (streamId >= HEVC_STREAM_MAX)) return 0;

	return pDevParams->strFrameQueueLevel[streamId];
}

static uint32_t hevcStreamQueueLevel(uint32_t devNum, uint32_t streamType)
{
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	uint32_t level = 0;
	uint32_t i;

	if((pDevParams == NULL) ||
	   (streamType >= FMB_STREAM_TYPE_MAX)) return 0;

	// count ready transfers
	for(i = 0; i < HEVC_STREAM_TASK_MAX; i++)
	{
		if(pDevParams->strQueue[streamType][i].apiReady) level++;
	}

	return level;
}

static void hevcStreamSegmentInit(uint32_t devNum, HevcStreamSegment* pSegment,
								  uint32_t srcPitch, uint32_t dstPitch,
								  uint32_t segSize, uint32_t segCount)
{
	HevcModuleParams* pModParams = hevcGetModuleParams();
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);

	if((pDevParams == NULL) || (pSegment == NULL)) return;

	memset(pSegment, 0, sizeof(HevcStreamSegment));

	pSegment->srcPitch = srcPitch;
	pSegment->dstPitch = dstPitch;
	pSegment->segSize = segSize;
	pSegment->segCount = segCount;

	HEVC_MSG_STREAM_SEGMENT("%s(%d): hevcStreamSegmentInit() srcPitch 0x%08x  dstPitch 0x%08x  segSize 0x%08x  segCount %d\n",
							pModParams->pModuleName, devNum,
							srcPitch, dstPitch, segSize, segCount);
}

static void hevcStreamSegmentSrcRange(uint32_t devNum, HevcStreamSegment* pSegment, uint32_t srcAddress, uint32_t srcSize)
{
	HevcModuleParams* pModParams = hevcGetModuleParams();
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);

	if((pDevParams == NULL) || (pSegment == NULL)) return;

	pSegment->srcAddress = srcAddress;
	pSegment->srcSize = srcSize;
	pSegment->segIndex = 0;

	HEVC_MSG_STREAM_SEGMENT("%s(%d): hevcStreamSegmentSrcRange() srcAddress 0x%08x  srcSize 0x%08x\n",
							pModParams->pModuleName, devNum, srcAddress, srcSize);
}

static void hevcStreamSegmentDstRange(uint32_t devNum, HevcStreamSegment* pSegment, uint32_t dstAddress, uint32_t dstSize)
{
	HevcModuleParams* pModParams = hevcGetModuleParams();
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);

	if((pDevParams == NULL) || (pSegment == NULL)) return;

	pSegment->dstAddress = dstAddress;
	pSegment->dstSize = dstSize;
	pSegment->segIndex = 0;

	HEVC_MSG_STREAM_SEGMENT("%s(%d): hevcStreamSegmentDstRange() dstAddress 0x%08x  dstSize 0x%08x\n",
							pModParams->pModuleName, devNum, dstAddress, dstSize);
}

static bool hevcStreamSegmentTransfer(uint32_t devNum, HevcStreamSegment* pSegment,
									  uint32_t* pSrcAddress, uint32_t* pDstAddress, uint32_t* pSize)
{
	HevcModuleParams* pModParams = hevcGetModuleParams();
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	uint32_t segSrcAddress;
	uint32_t segDstAddress;
	uint32_t segSrcIndex;
	uint32_t segDstIndex;
	uint32_t segIndex;
	uint32_t segSrcOffset;
	uint32_t segDstOffset;
	uint32_t segOffset;
	uint32_t segSrcSize;
	uint32_t segDstSize;
	uint32_t segSize;

	if((pDevParams == NULL) ||
	   (pSegment == NULL) ||
	   (pSrcAddress == NULL) ||
	   (pDstAddress == NULL) ||
	   (pSize == NULL)) return false;

	// check for valid init
	if((pSegment->segSize == 0) ||
	   (pSegment->segCount == 0) ||
	   (pSegment->srcSize == 0) ||
	   (pSegment->dstSize == 0)) return false;

	// determine first segment index for transfer (optimize)

	if(pSegment->segIndex == 0)
	{
		segSrcIndex = 0;
		segDstIndex = 0;
		if(pSegment->srcPitch > 0)
		{
			segSrcIndex = pSegment->srcAddress / pSegment->srcPitch;
		}
		if(pSegment->dstPitch > 0)
		{
			segDstIndex = pSegment->dstAddress / pSegment->dstPitch;
		}
		pSegment->segIndex = max(segSrcIndex, segDstIndex);
	}

	for(segIndex = pSegment->segIndex; segIndex < pSegment->segCount; segIndex++)
	{
		// calculate segment src address
		segSrcAddress = segIndex * pSegment->srcPitch;
		segSrcOffset = 0;
		segSrcSize = pSegment->segSize;

		// check for src segment address too big
		if(segSrcAddress >= (pSegment->srcAddress + pSegment->srcSize)) return false;
		// check for src segment address too small
		if((segSrcAddress + segSrcSize) <= pSegment->srcAddress) continue;

		// calculate segment dst address
		segDstAddress = segIndex*pSegment->dstPitch;
		segDstOffset = 0;
		segDstSize = pSegment->segSize;

		// check for dst segment address too big
		if(segDstAddress >= (pSegment->dstAddress + pSegment->dstSize)) return false;
		// check for dst segment address too small
		if((segDstAddress + segDstSize) <= pSegment->dstAddress) continue;

		// determine common segment offset
		if(segSrcAddress < pSegment->srcAddress)
		{
			segSrcOffset = pSegment->srcAddress - segSrcAddress;
		}
		if(segDstAddress < pSegment->dstAddress)
		{
			segDstOffset = pSegment->dstAddress - segDstAddress;
		}
		segOffset = max(segSrcOffset, segDstOffset);

		// determine common segment size
		if((segSrcAddress + segSrcSize) > (pSegment->srcAddress + pSegment->srcSize))
		{
			segSrcSize = pSegment->srcAddress + pSegment->srcSize - segSrcAddress;
		}
		if((segDstAddress + segDstSize) > (pSegment->dstAddress + pSegment->dstSize))
		{
			segDstSize = pSegment->dstAddress + pSegment->dstSize - segDstAddress;
		}
		segSize = min(segSrcSize, segDstSize);

		// check for bad transfer
		if((segSize == 0) ||
		   (segOffset >= segSize)) continue;

		// calculate new src and dst address and size
		*pSrcAddress = segSrcAddress + segOffset - pSegment->srcAddress;
		*pDstAddress = segDstAddress + segOffset - pSegment->dstAddress;
		*pSize = segSize - segOffset;;

		HEVC_MSG_STREAM_SEGMENT("%s(%d): hevcStreamSegmentTransfer() srcAddress 0x%08x  dstAddress 0x%08x  size 0x%08x\n",
								pModParams->pModuleName, devNum, *pSrcAddress, *pDstAddress, *pSize);

		// increment and save current segment index
		pSegment->segIndex = segIndex + 1;

		return true;
	}

	// ran out of segments
	return false;
}
