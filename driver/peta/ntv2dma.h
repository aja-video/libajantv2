/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
////////////////////////////////////////////////////////////
//
// Filename: ntv2dma.h
// Purpose:	 ntv2 driver dma engines
//
///////////////////////////////////////////////////////////////

#ifndef NTV2DMA_HEADER
#define NTV2DMA_HEADER

#include <linux/dmaengine.h>

#define DMA_NUM_ENGINES		1
#define DMA_NUM_CONTEXTS	1

typedef enum _NTV2DmaMethod
{
	DmaMethodIllegal,
	DmaMethodAja,
	DmaMethodNwl,
	DmaMethodXlnx,
	DmaMethodZynq
} NTV2DmaMethod;

typedef enum _NTV2DmaState
{
	DmaStateUnknown,		// not configured
	DmaStateConfigure,		// configure engine
	DmaStateIdle,			// waiting for work
	DmaStateSetup,			// setup dma transfer
	DmaStateTransfer,		// dma transfer
	DmaStateFinish,			// finish dma
	DmaStateRelease,		// release resources
	DmaStateDead			// engine has failed
} NTV2DmaState;

// dma page map
typedef struct _dmaPageRoot
{
	struct list_head		bufferHead;			// locked buffer list
	spinlock_t				bufferLock;			// lock buffer list
	bool					lockAuto;			// automatically lock buffers
	bool					lockMap;			// automatically map buffers
	LWord64					lockCounter;		// lock access counter
	LWord64					lockTotalSize;		// current locked bytes
	LWord64					lockMaxSize;		// maximum locked bytes
} DMA_PAGE_ROOT, *PDMA_PAGE_ROOT;

typedef struct _dmaPageBuffer
{
	struct list_head		bufferEntry;		// locked buffer list
	LWord					refCount;			// reference count
	void*					pUserAddress;		// user buffer address
	ULWord					userSize;			// user buffer size
	ULWord 					direction;			// dma direction
	bool					pageLock;			// pages are locked
	bool					busMap;				// bus is mapped (p2p)
	bool					sgMap;				// segment mapped
	bool					sgHost;				// segment map synced to host
	ULWord					numPages;			// pages locked
	struct page**			pPageList;			// page lock list
	ULWord					pageListSize;		// page list allocation
	struct device*			pSgDevice;			// segment device
	ULWord					numSgs;				// pages mapped
	struct scatterlist*		pSgList;			// scatter gather list
	ULWord					sgListSize;			// scatter list allocation
	LWord64					lockCount;			// lock access count
	LWord64					lockSize;			// locked bytes
} DMA_PAGE_BUFFER, *PDMA_PAGE_BUFFER;

// dma transfer parameters
typedef struct _dmaParams
{
	ULWord				deviceNumber;			// device number
	PDMA_PAGE_ROOT		pPageRoot;				// dma locked page cache
	bool				toHost;					// transfer to host
	NTV2DMAEngine		dmaEngine;				// dma engine
	NTV2Channel			videoChannel;			// video channel for frame size
	PVOID				pVidUserVa;				// user video buffer
	ULWord64			videoBusAddress;		// p2p video bus address
	ULWord				videoBusSize;			// p2p video bus size
	ULWord64			messageBusAddress;		// p2p message bus address
	ULWord				messageData;			// p2p message data
	ULWord				videoFrame;				// card video frame
	ULWord				vidNumBytes;			// number of bytes per segment
	ULWord				frameOffset; 			// card video offset
	ULWord				vidUserPitch;			// user buffer pitch
	ULWord				vidFramePitch;			// card frame pitch
	ULWord				numSegments;			// number of segments
	PVOID				pAudUserVa;				// audio user buffer
	NTV2AudioSystem		audioSystem;			// audio system target
	ULWord				audNumBytes;			// number of audio bytes
	ULWord				audOffset;				// card audio offset
	PVOID				pAncF1UserVa;			// anc field 1 user buffer
	ULWord				ancF1Frame;				// anc field 1 frame
	ULWord				ancF1NumBytes;			// number of anc field 1 bytes
	ULWord				ancF1Offset;			// anc field 1 frame offset
	PVOID				pAncF2UserVa;			// anc field 2 user buffer
	ULWord				ancF2Frame;				// anc field 2 frame
	ULWord				ancF2NumBytes;			// number of anc field 2 bytes
	ULWord				ancF2Offset;			// anc field 2 frame offset
	ULWord				audioSystemCount;		// number of multi-link audio systems
} DMA_PARAMS, *PDMA_PARAMS;

// dma transfer context
typedef struct _dmaContext_
{
	ULWord					deviceNumber;			// device number
	ULWord					engIndex;				// engine index
	char*					engName;				// engine name string
	ULWord					conIndex;				// context index
	ULWord					dmaIndex;				// dma index
	bool					dmaC2H;					// dma to host
	bool					conInit;				// context initialized
	bool					inUse;					// context acquired
	bool					doVideo;				// dma video buffer (transfer data)
	bool					doAudio;				// dma audio buffer (transfer data)
	bool					doAncF1;				// dma ancillary field 1 buffer (transfer data)
	bool					doAncF2;				// dma ancillary field 2 buffer (transfer data)
	PDMA_PAGE_BUFFER		pVideoPageBuffer;		// video page buffer
	PDMA_PAGE_BUFFER		pAudioPageBuffer;		// audio page buffer
	PDMA_PAGE_BUFFER		pAncF1PageBuffer;		// anc field 1 page buffer
	PDMA_PAGE_BUFFER		pAncF2PageBuffer;		// anc field 2 page buffer
	DMA_PAGE_BUFFER			videoPageBuffer;		// default video page buffer
	DMA_PAGE_BUFFER			audioPageBuffer;		// default audio page buffer
	DMA_PAGE_BUFFER			ancF1PageBuffer;		// default anc field 1 page buffer
	DMA_PAGE_BUFFER			ancF2PageBuffer;		// default anc field 2 page buffer
	unsigned long			videoCardAddress;		// video card transfer address
	size_t					videoCardSize;			// video card transer size
	unsigned long			audioCardAddress[2];	// audio card transfer address
	size_t					audioCardSize[2];		// audio card transer size
	unsigned long			ancF1CardAddress;		// anc field 1 card transfer address
	size_t					ancF1CardSize;			// anc field 1 card transer size
	unsigned long			ancF2CardAddress;		// anc field 2 card transfer address
	size_t					ancF2CardSize;			// anc field 2 card transer size
} DMA_CONTEXT, *PDMA_CONTEXT;

// dma engine parameters
typedef struct _dmaEngine_
{
	ULWord					deviceNumber;			// device number
	ULWord					engIndex;				// engine index
	char*					engName;				// engine name string
	bool					engInit;				// engine initialized
	bool					dmaEnable;				// transfer enable
	NTV2DmaMethod			dmaMethod;				// dma method				
	ULWord					dmaIndex;				// dma index
	bool					dmaC2H;					// dma to host
	ULWord					maxVideoSize;			// maximum video transfer size
	ULWord					maxVideoPages;			// maximum video pages
	ULWord					maxAudioSize;			// maximum audio transfer size
	ULWord					maxAudioPages;			// maximum audio pages
	ULWord					maxAncSize;				// maximum anc transfer size
	ULWord					maxAncPages;			// maximum anc pages
	NTV2DmaState			state;					// dma engine state
	spinlock_t				engineLock;				// engine data structure lock
	unsigned long			engineFlags;			// engine lock flags
	struct semaphore		contextSemaphore;		// context traffic control
	struct semaphore		transferSemaphore;		// hardware traffic control
	wait_queue_head_t		transferEvent;			// dma transfer complete event
	volatile unsigned long	transferDone;			// dma transfer done bit
	DMA_CONTEXT				dmaContext[DMA_NUM_CONTEXTS];	// dma transfer context
	struct device			dmaDevice;				// zynq dma device
	struct dma_chan*		dmaChannel;				// zynq dma channel
    uint64_t                zdma_addr;              // zynq device address;
	dma_cookie_t			videoDmaCookie;			// video dma cookie
	dma_cookie_t			audioDmaCookie;			// audio dma cookie
	dma_cookie_t			ancF1DmaCookie;			// anc field 1 dma cookie
	dma_cookie_t			ancF2DmaCookie;			// anc field 2 dma cookie
	bool					sbVideo;				// submit dma video
	bool					sbAudio;				// submit dma audio
	bool					sbAncF1;				// submit dma ancillary field 1
	bool					sbAncF2;				// submit dma ancillary field 2
	LWord64					scTransferCount;		// count system to card transfers
	LWord64					scErrorCount;			// count errors
	LWord64					scTransferBytes;		// sum bytes transferred
	LWord64					scTransferTime;			// sum software transfer time
	LWord64					scLockWaitTime;			// sum page lock time
	LWord64					scLockTime;				// sum page lock time
	LWord64					scDmaWaitTime;			// sum wait for dma hardware time
	LWord64					scDmaTime;				// sum hardware dma time
	LWord64					scUnlockTime;			// sum page unlock time
	LWord64					scLastDisplayTime;		// last stat display time
	LWord64					csTransferCount;		// card to system the same
	LWord64					csErrorCount;
	LWord64					csTransferBytes;
	LWord64					csTransferTime;
	LWord64					csLockWaitTime;
	LWord64					csLockTime;
	LWord64					csDmaWaitTime;
	LWord64					csDmaTime;
	LWord64					csUnlockTime;
	LWord64					csLastDisplayTime;
} DMA_ENGINE, *PDMA_ENGINE;

int dmaInit(ULWord deviceNumber);
void dmaRelease(ULWord deviceNumber);

int dmaPageRootInit(ULWord deviceNumber, PDMA_PAGE_ROOT pRoot);
void dmaPageRootRelease(ULWord deviceNumber, PDMA_PAGE_ROOT pRoot);
int dmaPageRootAdd(ULWord deviceNumber, PDMA_PAGE_ROOT pRoot,
				   PVOID pAddress, ULWord size, bool map);
int dmaPageRootRemove(ULWord deviceNumber, PDMA_PAGE_ROOT pRoot,
					  PVOID pAddress, ULWord size);
int dmaPageRootPrune(ULWord deviceNumber, PDMA_PAGE_ROOT pRoot, ULWord size);
void dmaPageRootAuto(ULWord deviceNumber, PDMA_PAGE_ROOT pRoot,
					 bool lockAuto, bool lockMap, ULWord64 maxSize);

PDMA_PAGE_BUFFER dmaPageRootFind(ULWord deviceNumber, PDMA_PAGE_ROOT pRoot,
										PVOID pAddress, ULWord size);
void dmaPageRootFree(ULWord deviceNumber, PDMA_PAGE_ROOT pRoot, PDMA_PAGE_BUFFER pBuffer);

int dmaEnable(ULWord deviceNumber);
void dmaDisable(ULWord deviceNumber);

int dmaTransfer(PDMA_PARAMS pDmaParams);
int dmaTargetP2P(ULWord deviceNumber, NTV2_DMA_P2P_CONTROL_STRUCT* pParams);

void dmaInterrupt(ULWord deviceNumber, ULWord intStatus);

#endif
