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

#ifdef AJA_RDMA
struct nvidia_p2p_page_table;
struct nvidia_p2p_dma_mapping;
#endif

#define DMA_NUM_ENGINES		4
#define DMA_NUM_CONTEXTS	2

#define DMA_TRANSFERCOUNT_64     			0x10000000
#define DMA_TRANSFERCOUNT_TOHOST 			0x80000000
#define DMA_TRANSFERCOUNT_BYTES  			4
#define DMA_DESCRIPTOR_PAGES_MAX			1024

typedef enum _NTV2DmaMethod
{
	DmaMethodIllegal,
	DmaMethodAja,
	DmaMethodNwl,
	DmaMethodXlnx
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

// aja descriptors
typedef struct _dmaDescriptor32
{
	ULWord				ulHostAddress;
	ULWord				ulLocalAddress;
	ULWord				ulTransferCount;
	ULWord				ulNextAddress;
} DMA_DESCRIPTOR32, *PDMA_DESCRIPTOR32;

typedef struct _dmaDescriptor64
{
	ULWord				ulHostAddressLow;
	ULWord				ulLocalAddress;
	ULWord				ulTransferCount;
	ULWord				ulNextAddressLow;
	ULWord				ulHostAddressHigh;
	ULWord				ulNextAddressHigh;
	ULWord				ulReserved0;
	ULWord				ulReserved1;
} DMA_DESCRIPTOR64, *PDMA_DESCRIPTOR64;

// nwl dma descriptor
typedef struct _nwlDmaDescriptor
{
	ULWord				ulControl;				// descriptor processing/interrupt control
	ULWord				ulTransferCount;		// number of bytes to transfer
	ULWord64			llHostAddress;			// system address
	ULWord64			llLocalAddress;			// card address
	ULWord64			llNextAddress;			// next descriptor address (0 = last)
} NWL_DESCRIPTOR, *PNWL_DESCRIPTOR;

// xilinx descriptor
typedef struct _xlnxDmaDescriptor 
{
	ULWord				ulControl;				// descriptor processing/interrupt control
	ULWord				ulTransferCount;		// transfer length in bytes
	ULWord64    		llSrcAddress;			// source address
	ULWord64    		llDstAddress;			// destination address
	ULWord64    		llNextAddress;			// next desc address
} XLNX_DESCRIPTOR, *PXLNX_DESCRIPTOR;

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
	ULWord					numSgs;				// pages mapped
	struct scatterlist*		pSgList;			// scatter gather list
	ULWord					sgListSize;			// scatter list allocation
	LWord64					lockCount;			// lock access count
	LWord64					lockSize;			// locked bytes
	bool					rdma;				// use nvidia rdma
#ifdef AJA_RDMA
	ULWord64				rdmaAddress;		// rdma gpu aligned buffer address
	ULWord64				rdmaOffset;			// rdma gpu aligned offset
	ULWord64				rdmaLen;			// rdma buffer length
	ULWord64				rdmaAlignedLen;		// rdma gpu aligned buffer length
	struct nvidia_p2p_page_table*	rdmaPage;
	struct nvidia_p2p_dma_mapping*	rdmaMap;
#endif
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

// video scatter list to descriptor map
typedef struct _dmaVideoSegment
{
	ULWord				cardAddress;			// card address
	ULWord				cardSize;				// card total transfer size
	ULWord				cardPitch;				// card segment pitch
	ULWord				systemPitch;			// system segment pitch
	ULWord				segmentSize;			// segment size
	ULWord				segmentCount;			// segment count
	bool				invertImage;			// invert segments
	ULWord64			transferAddress;		// scatter transfer address
	ULWord				transferSize;			// scatter transfer size
	ULWord				systemOffset;			// system bytes transferred
	ULWord				segmentIndex;			// segment transfer index
} DMA_VIDEO_SEGMENT, *PDMA_VIDEO_SEGMENT;

// audio scatter list to descriptor map
#define MAX_NUM_AUDIO_LINKS 4
typedef struct _dmaAudioSegment
{
	//ULWord			engIndex;
	ULWord				systemSize;							// dma system buffer size
	ULWord				transferSize;						// audio transfer size
	ULWord				ringAddress[MAX_NUM_AUDIO_LINKS];	// ring buffer address
	ULWord				ringCount;							// number of rings
	ULWord				ringSize;							// ring buffer size
	ULWord64			pageAddress;						// page address
	ULWord				pageSize;							// page size
	ULWord				audioStart;							// audio transfer start offset
	ULWord				audioSize;							// audio size
	ULWord				ringIndex;							// transfer ring index
	ULWord				systemOffset;						// system transfer bytes
	ULWord				pageOffset;							// page transfer bytes
	ULWord	   			audioOffset;						// audio transfer bytes
} DMA_AUDIO_SEGMENT, *PDMA_AUDIO_SEGMENT;

// anc scatter list to descriptor map
typedef struct _dmaAncSegment
{
	ULWord				ancAddress;				// anc buffer address
	ULWord				ancSize;				// anc buffer size
	ULWord64			transferAddress;		// scatter transfer address
	ULWord				transferSize;			// scatter transfer size
	ULWord				systemOffset;			// system bytes transferred
	ULWord				transferOffset;			// scatter bytes transferred
} DMA_ANC_SEGMENT, *PDMA_ANC_SEGMENT;

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
	bool					doMessage;				// dma gma message (transfer data)
	PDMA_PAGE_BUFFER		pVideoPageBuffer;		// video page buffer
	PDMA_PAGE_BUFFER		pAudioPageBuffer;		// audio page buffer
	PDMA_PAGE_BUFFER		pAncF1PageBuffer;		// anc field 1 page buffer
	PDMA_PAGE_BUFFER		pAncF2PageBuffer;		// anc field 2 page buffer
	DMA_PAGE_BUFFER			videoPageBuffer;		// default video page buffer
	DMA_PAGE_BUFFER			audioPageBuffer;		// default audio page buffer
	DMA_PAGE_BUFFER			ancF1PageBuffer;		// default anc field 1 page buffer
	DMA_PAGE_BUFFER			ancF2PageBuffer;		// default anc field 2 page buffer
	DMA_VIDEO_SEGMENT		dmaVideoSegment;		// video segment data (transfer data)
	DMA_AUDIO_SEGMENT		dmaAudioSegment;		// audio segment data (transfer data)
	DMA_ANC_SEGMENT			dmaAncF1Segment;		// ancillary field 1 segment data (transfer data)
	DMA_ANC_SEGMENT			dmaAncF2Segment;		// ancillary field 2 segment data (transfer data)
	ULWord64				messageBusAddress;		// gma message bus target address
	ULWord					messageCardAddress;		// gma message frame buffer source address
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
	ULWord					maxDescriptors;			// maximum number of descriptors
	bool					transferP2P;			// is p2p transfer;
	NTV2DmaState			state;					// dma engine state
	spinlock_t				engineLock;				// engine data structure lock
	unsigned long			engineFlags;			// engine lock flags
	struct semaphore		contextSemaphore;		// context traffic control
	struct semaphore		transferSemaphore;		// hardware traffic control
	wait_queue_head_t		transferEvent;			// dma transfer complete event
	volatile unsigned long	transferDone;			// dma transfer done bit
	DMA_CONTEXT				dmaContext[DMA_NUM_CONTEXTS];	// dma transfer context
	PVOID					pDescriptorVirtual[DMA_DESCRIPTOR_PAGES_MAX];		// virtual descriptor aligned address
	dma_addr_t				descriptorPhysical[DMA_DESCRIPTOR_PAGES_MAX];		// physical descriptor aligned address
	ULWord					numDescriptorPages;		// number of allocated descriptor pages
	LWord64					programStartCount;		// count program hardware dma starts
	LWord64					programCompleteCount;	// count program hardware dma completes
	LWord64					programDescriptorCount;	// count program hardware descriptors
	LWord64					programErrorCount;		// count program errors
	LWord64					programBytes;			// count program hardware bytes transferred
	LWord64					programStartTime;		// program transfer start time
	LWord64					programStopTime;		// program transfer stop time
	LWord64					programTime;			// sum program hardware transfer time
	LWord64					interruptCount;			// count dma interrupts
	LWord64					scTransferCount;		// count to card transfers
	LWord64					scRdmaCount;			// count gpu to card transfers
	LWord64					scErrorCount;			// count errors
	LWord64					scDescriptorCount;		// count descriptors
	LWord64					scTransferBytes;		// sum bytes transferred
	LWord64					scTransferTime;			// sum software transfer time
	LWord64					scLockWaitTime;			// sum page lock time
	LWord64					scLockTime;				// sum page lock time
	LWord64					scDmaWaitTime;			// sum wait for dma hardware time
	LWord64					scDmaTime;				// sum hardware dma time
	LWord64					scUnlockTime;			// sum page unlock time
	LWord64					scHardTime;				// sum hardware dma time
	LWord64					scLastDisplayTime;		// last stat display time
	LWord64					csTransferCount;		// count from card transfers
	LWord64					csRdmaCount;			// count card to gpu transfers
	LWord64					csErrorCount;
	LWord64					csDescriptorCount;
	LWord64					csTransferBytes;
	LWord64					csTransferTime;
	LWord64					csLockWaitTime;
	LWord64					csLockTime;
	LWord64					csDmaWaitTime;
	LWord64					csDmaTime;
	LWord64					csUnlockTime;
	LWord64					csHardTime;
	LWord64					csLastDisplayTime;
} DMA_ENGINE, *PDMA_ENGINE;

int dmaInit(ULWord deviceNumber);
void dmaRelease(ULWord deviceNumber);

int dmaPageRootInit(ULWord deviceNumber, PDMA_PAGE_ROOT pRoot);
void dmaPageRootRelease(ULWord deviceNumber, PDMA_PAGE_ROOT pRoot);
int dmaPageRootAdd(ULWord deviceNumber, PDMA_PAGE_ROOT pRoot,
				   PVOID pAddress, ULWord size, bool rdma, bool map);
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
