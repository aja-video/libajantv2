/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
//==========================================================================
//
//  ntv2dma.c
//
//==========================================================================

#if defined(CONFIG_SMP)
#define __SMP__
#endif

#include <linux/version.h>
#include <linux/semaphore.h>
#include <linux/pagemap.h>

#include "buildenv.h"
#include "ntv2enums.h"
#include "ntv2audiodefines.h"
#include "ntv2videodefines.h"

#include "ntv2publicinterface.h"
#include "ntv2linuxpublicinterface.h"
#include "ntv2devicefeatures.h"

#include "registerio.h"
#include "ntv2stream.h"
#include "ntv2dma.h"

#ifdef AJA_RDMA
#include <nv-p2p.h>

#ifdef AJA_IGPU
#define GPU_PAGE_SHIFT	12
#else
#define GPU_PAGE_SHIFT	16
#endif
#define GPU_PAGE_SIZE	(((ULWord64)1) << GPU_PAGE_SHIFT)
#define GPU_PAGE_OFFSET	(GPU_PAGE_SIZE - 1)
#define GPU_PAGE_MASK	(~GPU_PAGE_OFFSET)
#endif

/* debug messages */
#define NTV2_DEBUG_INFO					0x00000001
#define NTV2_DEBUG_ERROR				0x00000002
#define NTV2_DEBUG_STATE				0x00000010
#define NTV2_DEBUG_STATISTICS			0x00000020
#define NTV2_DEBUG_TRANSFER				0x00000040
#define NTV2_DEBUG_VIDEO_SEGMENT		0x00001000
#define NTV2_DEBUG_AUDIO_SEGMENT		0x00002000
#define NTV2_DEBUG_ANC_SEGMENT			0x00004000
#define NTV2_DEBUG_GMA_MESSAGE			0x00008000
#define NTV2_DEBUG_PAGE_MAP				0x00010000
#define NTV2_DEBUG_PROGRAM				0x00020000
#define NTV2_DEBUG_DESCRIPTOR			0x00040000

#define NTV2_DEBUG_ACTIVE(msg_mask) \
	((ntv2_debug_mask & msg_mask) != 0)

#define NTV2_MSG_PRINT(msg_mask, string, ...) \
	if(NTV2_DEBUG_ACTIVE(msg_mask)) ntv2Message(string, __VA_ARGS__);

#define NTV2_MSG_INFO(string, ...)					NTV2_MSG_PRINT(NTV2_DEBUG_INFO, string, __VA_ARGS__)
#define NTV2_MSG_ERROR(string, ...)					NTV2_MSG_PRINT(NTV2_DEBUG_ERROR, string, __VA_ARGS__)
#define NTV2_MSG_STATE(string, ...)					NTV2_MSG_PRINT(NTV2_DEBUG_STATE, string, __VA_ARGS__)
#define NTV2_MSG_STATISTICS(string, ...)			NTV2_MSG_PRINT(NTV2_DEBUG_STATISTICS, string, __VA_ARGS__)
#define NTV2_MSG_TRANSFER(string, ...)				NTV2_MSG_PRINT(NTV2_DEBUG_TRANSFER, string, __VA_ARGS__)
#define NTV2_MSG_VIDEO_SEGMENT(string, ...)			NTV2_MSG_PRINT(NTV2_DEBUG_VIDEO_SEGMENT, string, __VA_ARGS__)
#define NTV2_MSG_AUDIO_SEGMENT(string, ...)			NTV2_MSG_PRINT(NTV2_DEBUG_AUDIO_SEGMENT, string, __VA_ARGS__)
#define NTV2_MSG_ANC_SEGMENT(string, ...)			NTV2_MSG_PRINT(NTV2_DEBUG_ANC_SEGMENT, string, __VA_ARGS__)
#define NTV2_MSG_GMA_MESSAGE(string, ...)			NTV2_MSG_PRINT(NTV2_DEBUG_GMA_SEGMENT, string, __VA_ARGS__)
#define NTV2_MSG_PAGE_MAP(string, ...)				NTV2_MSG_PRINT(NTV2_DEBUG_PAGE_MAP, string, __VA_ARGS__)
#define NTV2_MSG_PROGRAM(string, ...)				NTV2_MSG_PRINT(NTV2_DEBUG_PROGRAM, string, __VA_ARGS__)
#define NTV2_MSG_DESCRIPTOR(string, ...)			NTV2_MSG_PRINT(NTV2_DEBUG_DESCRIPTOR, string, __VA_ARGS__)

#define DMA_S2D(tohost) ((tohost)? "c2h" : "h2c")
#define DMA_MSG_DEVICE "ntv2dma", deviceNumber
#define DMA_MSG_ENGINE  "ntv2dma", pDmaEngine->deviceNumber, pDmaEngine->engName, pDmaEngine->engIndex
#define DMA_MSG_CONTEXT "ntv2dma", pDmaContext->deviceNumber, pDmaContext->engName, pDmaContext->engIndex, DMA_S2D(pDmaContext->dmaC2H), pDmaContext->conIndex

static uint32_t ntv2_debug_mask =
//	NTV2_DEBUG_STATE |
//	NTV2_DEBUG_STATISTICS |
//  NTV2_DEBUG_TRANSFER |
//	NTV2_DEBUG_PAGE_MAP |
//	NTV2_DEBUG_PROGRAM |
//	NTV2_DEBUG_VIDEO_SEGMENT |
//	NTV2_DEBUG_DESCRIPTOR |
	NTV2_DEBUG_INFO | 
	NTV2_DEBUG_ERROR;

// maximium size of a hd frame (16 MB)
#define HD_MAX_FRAME_SIZE			(0x1000000)
#define HD_MAX_LINES_PER_FRAME		(1080)
#define HD_MAX_PAGES				((HD_MAX_FRAME_SIZE / PAGE_SIZE) + 1)
#define HD_MAX_DESCRIPTORS 			((HD_MAX_FRAME_SIZE / PAGE_SIZE) + (2 * HD_MAX_LINES_PER_FRAME) + 100)

// maximium size of a video frame (64 MB)
#define UHD_MAX_FRAME_SIZE			(0x4000000)
#define UHD_MAX_LINES_PER_FRAME		(2160)
#define UHD_MAX_PAGES				((UHD_MAX_FRAME_SIZE / PAGE_SIZE) + 1)
#define UHD_MAX_DESCRIPTORS			((UHD_MAX_FRAME_SIZE / PAGE_SIZE) + (2 * UHD_MAX_LINES_PER_FRAME) + 100)

// maximium size of a video frame (256 MB)
#define UHD2_MAX_FRAME_SIZE			(0x10000000)
#define UHD2_MAX_LINES_PER_FRAME	(4320)
#define UHD2_MAX_PAGES				((UHD2_MAX_FRAME_SIZE / PAGE_SIZE) + 1)
#define UHD2_MAX_DESCRIPTORS		((UHD2_MAX_FRAME_SIZE / PAGE_SIZE) + (2 * UHD2_MAX_LINES_PER_FRAME) + 100)

// maximum size of audio (4 MB)
#define AUDIO_MAX_SIZE				(0x400000)
#define AUDIO_MAX_PAGES				((AUDIO_MAX_SIZE / PAGE_SIZE) + 1)
#define AUDIO_MAX_DESCRIPTORS 		((AUDIO_MAX_SIZE / PAGE_SIZE) + 10)

// maximum size of anc (256 KB)
#define ANC_MAX_SIZE				(0x40000)
#define ANC_MAX_PAGES				((ANC_MAX_SIZE / PAGE_SIZE) + 1)
#define ANC_MAX_DESCRIPTORS 		((ANC_MAX_SIZE / PAGE_SIZE) + 2)

// hd descriptor list size
#define HD_TOT_DESCRIPTORS			(HD_MAX_DESCRIPTORS + AUDIO_MAX_DESCRIPTORS + (2 * ANC_MAX_DESCRIPTORS))

// uhd descriptor list size
#define UHD_TOT_DESCRIPTORS			(UHD_MAX_DESCRIPTORS + AUDIO_MAX_DESCRIPTORS + (2 * ANC_MAX_DESCRIPTORS))

// uhd2 descriptor list size
#define UHD2_TOT_DESCRIPTORS		(UHD2_MAX_DESCRIPTORS + AUDIO_MAX_DESCRIPTORS + (2 * ANC_MAX_DESCRIPTORS))

// common dma descriptor size
#define DMA_DESCRIPTOR_SIZE			(sizeof(DMA_DESCRIPTOR64))

// nwl control parameter bit definitions
#define NWL_CONTROL_IRQ_ON_COMPLETION       0x00000001
#define NWL_CONTROL_IRQ_ON_SHORT_ERR        0x00000002
#define NWL_CONTROL_IRQ_ON_SHORT_SW         0x00000004
#define NWL_CONTROL_IRQ_ON_SHORT_HW         0x00000008
#define NWL_CONTROL_SEQUENCE                0x00000010
#define NWL_CONTROL_CONTINUE                0x00000020

// xlnx control parameter bit definitions
#define XLNX_CONTROL_DESC_STOP				0x00000001
#define XLNX_CONTROL_DESC_COMPLETION		0x00000002
#define XLNX_CONTROL_DESC_EOP				0x00000010
#define XLNX_CONTROL_DESC_COUNT_MASK		0x00001F00
#define XLNX_CONTROL_DESC_COUNT_SHIFT		8
#define XLNX_CONTROL_DESC_MAGIC				0xAD4B0000

#define DMA_STATISTICS_INTERVAL				20000000	// 100ns
#define DMA_TRANSFER_TIMEOUT				20000000

#define XLNX_MAX_ADJACENT_COUNT		15

const NTV2DMAStatusBits dmaAjaIntClear[] = { NTV2_DMA1_CLEAR, NTV2_DMA2_CLEAR, NTV2_DMA3_CLEAR, NTV2_DMA4_CLEAR };

const ULWord dmaTransferRateC2HReg[] = { kVRegDmaTransferRateC2H1,
										 kVRegDmaTransferRateC2H2,
										 kVRegDmaTransferRateC2H3,
										 kVRegDmaTransferRateC2H4 };
const ULWord dmaTransferRateH2CReg[] = { kVRegDmaTransferRateH2C1,
										 kVRegDmaTransferRateH2C2,
										 kVRegDmaTransferRateH2C3,
										 kVRegDmaTransferRateH2C4 };
const ULWord dmaHardwareRateC2HReg[] = { kVRegDmaHardwareRateC2H1,
										 kVRegDmaHardwareRateC2H2,
										 kVRegDmaHardwareRateC2H3,
										 kVRegDmaHardwareRateC2H4 };
const ULWord dmaHardwareRateH2CReg[] = { kVRegDmaHardwareRateH2C1,
										 kVRegDmaHardwareRateH2C2,
										 kVRegDmaHardwareRateH2C3,
										 kVRegDmaHardwareRateH2C4 };

static PDMA_ENGINE getDmaEngine(ULWord deviceNumber, ULWord engIndex);
static PDMA_CONTEXT getDmaContext(ULWord deviceNumber, ULWord engIndex, ULWord conIndex);
static struct ntv2_stream* getDmaStream(ULWord deviceNumber, ULWord engIndex);

static void dmaFreeEngine(PDMA_ENGINE pDmaEngine);
static PDMA_ENGINE dmaMapEngine(ULWord deviceNumber, NTV2DMAEngine eDMAEngine, bool bToHost);
//static PDMA_ENGINE dmaMapStream(ULWord deviceNumber, NTV2Channel eChannel, bool bToHost);
static bool dmaHardwareInit(PDMA_ENGINE pDmaEngine);
static void dmaStatistics(PDMA_ENGINE pDmaEngine, bool dmaC2H);

static void dmaEngineLock(PDMA_ENGINE pDmaEngine);
static void dmaEngineUnlock(PDMA_ENGINE pDmaEngine);
static PDMA_CONTEXT dmaContextAcquire(PDMA_ENGINE pDmaEngine, ULWord timeout);
static void dmaContextRelease(PDMA_CONTEXT pDmaContext);
static int dmaHardwareAcquire(PDMA_ENGINE pDmaEngine, ULWord timeout);
static void dmaHardwareRelease(PDMA_ENGINE pDmaEngine);
static int dmaSerialAcquire(ULWord deviceNumber, ULWord timeout);
static void dmaSerialRelease(ULWord deviceNumber);

static inline bool dmaPageRootAutoLock(PDMA_PAGE_ROOT pRoot);
static inline bool dmaPageRootAutoMap(PDMA_PAGE_ROOT pRoot);

static int dmaPageBufferInit(ULWord deviceNumber, PDMA_PAGE_BUFFER pBuffer,
							 ULWord numPages, bool rdma);
static void dmaPageBufferRelease(ULWord deviceNumber, PDMA_PAGE_BUFFER pBuffer);
static int dmaPageLock(ULWord deviceNumber, PDMA_PAGE_BUFFER pBuffer,
					   PVOID pAddress, ULWord size, ULWord direction);
static void dmaPageUnlock(ULWord deviceNumber, PDMA_PAGE_BUFFER pBuffer);
#ifdef AJA_RDMA
static void rdmaFreeCallback(void* data);
static void dmaSgSetRdmaPage(struct scatterlist* pSg, struct nvidia_p2p_dma_mapping	*rdmaMap,
							 int index, ULWord64 length, ULWord64 offset);
#endif
static int dmaBusMap(ULWord deviceNumber, PDMA_PAGE_BUFFER pBuffer,
					 ULWord64 videoBusAddress, ULWord videoBusSize);
static int dmaSgMap(ULWord deviceNumber, PDMA_PAGE_BUFFER pBuffer);
static void dmaSgUnmap(ULWord deviceNumber, PDMA_PAGE_BUFFER pBuffer);
static void dmaSgDevice(ULWord deviceNumber, PDMA_PAGE_BUFFER pBuffer);
static void dmaSgHost(ULWord deviceNumber, PDMA_PAGE_BUFFER pBuffer);

static inline bool dmaPageLocked(PDMA_PAGE_BUFFER pBuffer);
static inline bool dmaSgMapped(PDMA_PAGE_BUFFER pBuffer);
static inline ULWord dmaSgSize(PDMA_PAGE_BUFFER pBuffer);
static inline ULWord64 dmaSgAddress(PDMA_PAGE_BUFFER pBuffer, ULWord index);
static inline ULWord dmaSgLength(PDMA_PAGE_BUFFER pBuffer, ULWord index);

static int dmaAjaProgram(PDMA_CONTEXT pDmaContext);
static void dmaAjaAbort(PDMA_ENGINE pDmaEngine);
static void dmaAjaInterrupt(PDMA_ENGINE pDmaEngine);

static int dmaNwlProgram(PDMA_CONTEXT pDmaContext);
static void dmaNwlAbort(PDMA_ENGINE pDmaEngine);
static void dmaNwlInterrupt(PDMA_ENGINE pDmaEngine);

static int dmaXlnxProgram(PDMA_CONTEXT pDmaContext);
static void dmaXlnxAbort(PDMA_ENGINE pDmaEngine);
static void dmaXlnxInterrupt(PDMA_ENGINE pDmaEngine);

static int dmaXlnxStreamBuild(PDMA_ENGINE pDmaEngine, PDMA_PAGE_BUFFER pPageBuffer, uint32_t index);
static int dmaXlnxStreamLink(PDMA_ENGINE pDmaEngine, uint32_t descIndex, uint32_t nextIndex);
static int dmaXlnxStreamStart(PDMA_ENGINE pDmaEngine, uint32_t startIndex);
static int dmaXlnxStreamStop(PDMA_ENGINE pDmaEngine);
static int dmaXlnxDescIndexToPage(uint32_t index, uint32_t* pageIndex, uint32_t* descIndex);

static bool dmaVideoSegmentInit(PDMA_CONTEXT pDmaContext, PDMA_VIDEO_SEGMENT pDmaSegment);
static bool dmaVideoSegmentConfig(PDMA_CONTEXT pDmaContext,
								  PDMA_VIDEO_SEGMENT pDmaSegment,
								  ULWord cardAddress,
								  ULWord cardSize,
								  ULWord cardPitch,
								  ULWord systemPitch,
								  ULWord segmentSize,
								  ULWord segmentCount,
								  bool invertImage);
static inline bool dmaVideoSegmentTransfer(PDMA_CONTEXT pDmaContext,
										   PDMA_VIDEO_SEGMENT pDmaSegment,
										   ULWord64 transferAddress, 
										   ULWord transferSize);
static inline bool dmaVideoSegmentDescriptor(PDMA_CONTEXT pDmaContext,
											 PDMA_VIDEO_SEGMENT pDmaSegment,
											 ULWord64* pSystemAddress, 
											 ULWord* pCardAddress, 
											 ULWord* pDescriptorSize);

static bool dmaAudioSegmentInit(PDMA_CONTEXT pDmaContext, PDMA_AUDIO_SEGMENT pDmaSegment);
static bool dmaAudioSegmentConfig(PDMA_CONTEXT pDmaContext,
								  PDMA_AUDIO_SEGMENT pDmaSegment,
								  ULWord systemSize,
								  ULWord transferSize,
								  ULWord* pRingAddress,
								  ULWord ringSize,
								  ULWord audioStart);
static inline bool dmaAudioSegmentTransfer(PDMA_CONTEXT pDmaContext,
										   PDMA_AUDIO_SEGMENT pDmaSegment,
										   ULWord64 pageAddress, 
										   ULWord pageSize);
static inline bool dmaAudioSegmentDescriptor(PDMA_CONTEXT pDmaContext,
											 PDMA_AUDIO_SEGMENT pDmaSegment,
											 ULWord64* pSystemAddress, 
											 ULWord* pCardAddress, 
											 ULWord* pDescriptorSize);

static bool dmaAncSegmentInit(PDMA_CONTEXT pDmaContext, PDMA_ANC_SEGMENT pDmaSegment);
static bool dmaAncSegmentConfig(PDMA_CONTEXT pDmaContext,
								PDMA_ANC_SEGMENT pDmaSegment,
								ULWord ancAddress,
								ULWord ancSize);
static inline bool dmaAncSegmentTransfer(PDMA_CONTEXT pDmaContext,
										 PDMA_ANC_SEGMENT pDmaSegment,
										 ULWord64 transferAddress, 
										 ULWord transferSize);
static inline bool dmaAncSegmentDescriptor(PDMA_CONTEXT pDmaContext,
										   PDMA_ANC_SEGMENT pDmaSegment,
										   ULWord64* pSystemAddress, 
										   ULWord* pCardAddress, 
										   ULWord* pDescriptorSize);

static inline uint32_t microsecondsToJiffies(int64_t timeout);


int dmaInit(ULWord deviceNumber)
{
	NTV2PrivateParams *pNTV2Params = getNTV2Params(deviceNumber);
	ULWord deviceID = pNTV2Params->_DeviceID;
	ULWord maxVideoSize;
	ULWord maxVideoPages;
	ULWord maxDescriptors;
	ULWord iEng;
	ULWord iCon;
	ULWord iDes;
//    Ntv2Status status;

	if (pNTV2Params->_dmaNumEngines != 0)
	{
		NTV2_MSG_INFO("%s%d: dmaInit called again (ignored)\n", DMA_MSG_DEVICE);
		return 0;
	}

	NTV2_MSG_INFO("%s%d: dmaInit begin\n", DMA_MSG_DEVICE);

#ifdef AJA_RDMA
	NTV2_MSG_INFO("%s%d: can do rdma\n", DMA_MSG_DEVICE);
#endif	

	for (iEng = 0; iEng < DMA_NUM_ENGINES; iEng++)
	{
		memset(&pNTV2Params->_dmaEngine[iEng], 0, sizeof(DMA_ENGINE));
	}

	pNTV2Params->_dmaNumEngines = NTV2DeviceGetNumDMAEngines(deviceID);

	sema_init(&pNTV2Params->_dmaSerialSemaphore, 1);

	// nwl and xlnx has seperate read and write engines
	if ((pNTV2Params->_dmaMethod == DmaMethodNwl) || 
		(pNTV2Params->_dmaMethod == DmaMethodXlnx))
	{
		pNTV2Params->_dmaNumEngines *= 2;
	}

	maxVideoSize = HD_MAX_FRAME_SIZE;
	maxVideoPages = HD_MAX_PAGES;
	maxDescriptors = HD_TOT_DESCRIPTORS;
	if (NTV2DeviceCanDo4KVideo(pNTV2Params->_DeviceID))
	{
		maxVideoSize = UHD_MAX_FRAME_SIZE;
		maxVideoPages = UHD_MAX_PAGES;
		maxDescriptors = UHD_TOT_DESCRIPTORS;
	}
	if (NTV2DeviceCanDo8KVideo(pNTV2Params->_DeviceID))
	{
		maxVideoSize = UHD2_MAX_FRAME_SIZE;
		maxVideoPages = UHD2_MAX_PAGES;
		maxDescriptors = UHD2_TOT_DESCRIPTORS;
	}

	for (iEng = 0; iEng < pNTV2Params->_dmaNumEngines; iEng++)
	{
		PDMA_ENGINE pDmaEngine = &pNTV2Params->_dmaEngine[iEng];

		// init state
		pDmaEngine->state = DmaStateConfigure;
		pDmaEngine->deviceNumber = deviceNumber;
		pDmaEngine->engIndex = iEng;
		pDmaEngine->dmaMethod = pNTV2Params->_dmaMethod;

		// init context lock
		spin_lock_init(&pDmaEngine->engineLock);
		// init setup semaphore
		sema_init(&pDmaEngine->contextSemaphore, DMA_NUM_CONTEXTS);
		// init transfer semaphore
		sema_init(&pDmaEngine->transferSemaphore, 1);
		// init transfer event
		init_waitqueue_head(&pDmaEngine->transferEvent);

		// configure engine type
		switch (pDmaEngine->dmaMethod)
		{
		case DmaMethodAja:
			pDmaEngine->dmaIndex = pDmaEngine->engIndex;
			pDmaEngine->dmaC2H = false; // not used
			pDmaEngine->engName = "aja";
			break;
		case DmaMethodNwl:
			pDmaEngine->dmaIndex = pDmaEngine->engIndex >> 1;
			pDmaEngine->dmaC2H = (pDmaEngine->engIndex & 0x1) != 0;
			pDmaEngine->engName = "nwl"; 
			break;
		case DmaMethodXlnx:
			pDmaEngine->dmaIndex = pDmaEngine->engIndex >> 1;
			pDmaEngine->dmaC2H = (pDmaEngine->engIndex & 0x1) != 0;
			pDmaEngine->engName = "xlx";
			break;
		default:
			pDmaEngine->dmaIndex = pDmaEngine->engIndex;
			pDmaEngine->dmaC2H = false;
			pDmaEngine->engName = "bad";
			NTV2_MSG_ERROR("%s%d:%s%d: dmaInit configuration failed\n", DMA_MSG_ENGINE);
			pDmaEngine->state = DmaStateDead;
			break;
		}

		// max resources
		pDmaEngine->maxVideoSize = maxVideoSize;
		pDmaEngine->maxVideoPages = maxVideoPages;
		pDmaEngine->maxAudioSize = AUDIO_MAX_SIZE;
		pDmaEngine->maxAudioPages = AUDIO_MAX_PAGES;
		pDmaEngine->maxAncSize = ANC_MAX_SIZE;
		pDmaEngine->maxAncPages = ANC_MAX_PAGES;
		pDmaEngine->maxDescriptors = maxDescriptors;
		
		// engine is initialized
		pDmaEngine->engInit = true;
		
		NTV2_MSG_INFO("%s%d:%s%d: dmaInit configure max  vid %d  aud %d  anc %d\n",
					  DMA_MSG_ENGINE, pDmaEngine->maxVideoSize,
					  pDmaEngine->maxAudioSize, pDmaEngine->maxAncSize);
		
		// init dma hardware dependent
		if (pDmaEngine->state == DmaStateConfigure)
		{
			if (dmaHardwareInit(pDmaEngine))
			{
				NTV2_MSG_INFO("%s%d:%s%d: dmaInit configure hardware\n", DMA_MSG_ENGINE);
			}
			else
			{
				NTV2_MSG_ERROR("%s%d:%s%d: dmaInit configure hardware failed\n", DMA_MSG_ENGINE);
				pDmaEngine->state = DmaStateDead;
			}

            NTV2_MSG_INFO("%s%d:%s%d: dmaInit  %s engine  alignment mask %08x  granularity mask %08x\n",
                          DMA_MSG_ENGINE, (pDmaEngine->dmaStream? "stream":"mapped"),
                          pDmaEngine->alignmentMask, pDmaEngine->granularityMask);
		}

		// configure dma context
		if (pDmaEngine->state == DmaStateConfigure)
		{
            if (pDmaEngine->dmaStream)
            {
                pDmaEngine->maxDescriptors = DMA_DESCRIPTOR_PAGES_MAX * (PAGE_SIZE / DMA_DESCRIPTOR_SIZE);
            }

            for (iCon = 0; iCon < DMA_NUM_CONTEXTS; iCon++)
            {
                PDMA_CONTEXT pDmaContext = &pDmaEngine->dmaContext[iCon];

                // copy for convenience
                pDmaContext->deviceNumber = pDmaEngine->deviceNumber;
                pDmaContext->engIndex = pDmaEngine->engIndex;
                pDmaContext->engName = pDmaEngine->engName;
                pDmaContext->conIndex = iCon;
                pDmaContext->dmaIndex = pDmaEngine->dmaIndex;
                pDmaContext->dmaC2H = pDmaEngine->dmaC2H;
                pDmaContext->conInit = true;

                NTV2_MSG_INFO("%s%d:%s%d:%s%d: dmaInit configure dma context\n",
                              DMA_MSG_CONTEXT);

                // allocate the default page and scatter list buffers
                if (dmaPageBufferInit(deviceNumber, &pDmaContext->videoPageBuffer, pDmaEngine->maxVideoPages, false) != 0)
                {
                    NTV2_MSG_ERROR("%s%d:%s%d:%s%d: dmaInit allocate video page buffer failed\n",
                                   DMA_MSG_CONTEXT);
                    pDmaEngine->state = DmaStateDead;
                }

                if (dmaPageBufferInit(deviceNumber, &pDmaContext->audioPageBuffer, pDmaEngine->maxAudioPages, false) != 0)
                {
                    NTV2_MSG_ERROR("%s%d:%s%d:%s%d: dmaInit allocate audio page buffer failed\n",
                                   DMA_MSG_CONTEXT);
                    pDmaEngine->state = DmaStateDead;
                }

                if (dmaPageBufferInit(deviceNumber, &pDmaContext->ancF1PageBuffer, pDmaEngine->maxAncPages, false) != 0)
                {
                    NTV2_MSG_ERROR("%s%d:%s%d:%s%d: dmaInit allocate anc field 1 page buffer failed\n",
                                   DMA_MSG_CONTEXT);
                    pDmaEngine->state = DmaStateDead;
                }

                if (dmaPageBufferInit(deviceNumber, &pDmaContext->ancF2PageBuffer, pDmaEngine->maxAncPages, false) != 0)
                {
                    NTV2_MSG_ERROR("%s%d:%s%d:%s%d: dmaInit allocate anc field 2 page buffer failed\n",
                                   DMA_MSG_CONTEXT);
                    pDmaEngine->state = DmaStateDead;
                }
            }
		}

		// allocate and map descriptor memory
		if (pDmaEngine->state == DmaStateConfigure)
		{
			// caculate number of descriptor pages
			pDmaEngine->numDescriptorPages = (pDmaEngine->maxDescriptors / (PAGE_SIZE / DMA_DESCRIPTOR_SIZE)) + 1;
			if (pDmaEngine->numDescriptorPages > DMA_DESCRIPTOR_PAGES_MAX)
				pDmaEngine->numDescriptorPages = DMA_DESCRIPTOR_PAGES_MAX;

			for (iDes = 0; iDes < pDmaEngine->numDescriptorPages; iDes++)
			{
				pDmaEngine->pDescriptorVirtual[iDes] =
					dma_alloc_coherent(&pNTV2Params->pci_dev->dev,
										 PAGE_SIZE,
										 &pDmaEngine->descriptorPhysical[iDes], GFP_ATOMIC);
				if ((pDmaEngine->pDescriptorVirtual[iDes] == NULL) ||
					(pDmaEngine->descriptorPhysical[iDes] == 0))
				{
					NTV2_MSG_ERROR("%s%d:%s%d: dmaInit allocate descriptor buffer failed\n", DMA_MSG_ENGINE);
					pDmaEngine->state = DmaStateDead;
				}
			}
		}

		if (pDmaEngine->state == DmaStateConfigure)
		{
			NTV2_MSG_INFO("%s%d:%s%d: dmaInit initialization succeeded\n", DMA_MSG_ENGINE);
			// ready for dma transfer
			NTV2_MSG_STATE("%s%d:%s%d: dmaInit dma state idle\n", DMA_MSG_ENGINE);
			pDmaEngine->state = DmaStateIdle;
		}
		else
		{
			NTV2_MSG_INFO("%s%d:%s%d: dmaInit initialization failed\n", DMA_MSG_ENGINE);
			dmaFreeEngine(pDmaEngine);
		}
	}

	NTV2_MSG_INFO("%s%d: dmaInit end\n", DMA_MSG_DEVICE);

	return 0;
}

static PDMA_ENGINE getDmaEngine(ULWord deviceNumber, ULWord engIndex)
{
	NTV2PrivateParams *pNTV2Params = getNTV2Params(deviceNumber);
	PDMA_ENGINE pDmaEngine = &pNTV2Params->_dmaEngine[engIndex];
	return pDmaEngine;
}

static struct ntv2_stream* getDmaStream(ULWord deviceNumber, ULWord engIndex)
{
	NTV2PrivateParams *pNTV2Params = getNTV2Params(deviceNumber);
	PDMA_ENGINE pDmaEngine = &pNTV2Params->_dmaEngine[engIndex];
    
    if (!pDmaEngine->dmaStream)
    {
        return NULL;
    }
    
	return pNTV2Params->m_pDmaStream[pDmaEngine->strIndex];
}

static PDMA_CONTEXT getDmaContext(ULWord deviceNumber, ULWord engIndex, ULWord conIndex)
{
	NTV2PrivateParams *pNTV2Params = getNTV2Params(deviceNumber);
	PDMA_ENGINE pDmaEngine = &pNTV2Params->_dmaEngine[engIndex];
	PDMA_CONTEXT pDmaContext = &pDmaEngine->dmaContext[conIndex];
	return pDmaContext;
}

static void dmaFreeEngine(PDMA_ENGINE pDmaEngine)
{
	ULWord deviceNumber = pDmaEngine->deviceNumber;
	ULWord engIndex = pDmaEngine->engIndex;
	NTV2PrivateParams *pNTV2Params = getNTV2Params(deviceNumber);
	ULWord iCon;
	ULWord iDes;

	NTV2_MSG_INFO("%s%d:%s%d: dmaFreeEngine\n", DMA_MSG_ENGINE);

	if (!pDmaEngine->engInit)
	{
		return;
	}

	pDmaEngine->dmaEnable = false;
	pDmaEngine->state = DmaStateUnknown;

	for (iCon = 0; iCon < DMA_NUM_CONTEXTS; iCon++)
	{
		PDMA_CONTEXT pDmaContext = getDmaContext(deviceNumber, engIndex, iCon);
		if (!pDmaContext->conInit)
		{
			continue;
		}

		dmaPageBufferRelease(deviceNumber, &pDmaContext->videoPageBuffer);
		dmaPageBufferRelease(deviceNumber, &pDmaContext->audioPageBuffer);
		dmaPageBufferRelease(deviceNumber, &pDmaContext->ancF1PageBuffer);
		dmaPageBufferRelease(deviceNumber, &pDmaContext->ancF2PageBuffer);
	}
	
	for (iDes = 0; iDes < pDmaEngine->numDescriptorPages; iDes++)
	{
		if ((pDmaEngine->pDescriptorVirtual[iDes] != NULL) &&
			(pDmaEngine->descriptorPhysical[iDes] != 0))
		{
			dma_free_coherent(&pNTV2Params->pci_dev->dev,
								PAGE_SIZE,
								pDmaEngine->pDescriptorVirtual[iDes],
								pDmaEngine->descriptorPhysical[iDes]);
		}
	}
}

void dmaRelease(ULWord deviceNumber)
{
	NTV2PrivateParams *pNTV2Params = getNTV2Params(deviceNumber);
	ULWord iEng;

	if (pNTV2Params->_dmaNumEngines == 0)
	{
		NTV2_MSG_INFO("%s%d: dmaRelease no engines to free\n", DMA_MSG_DEVICE);
		return;
	}

	NTV2_MSG_INFO("%s%d: dmaRelease begin\n", DMA_MSG_DEVICE);

	dmaDisable(deviceNumber);

	for(iEng = 0; iEng < pNTV2Params->_dmaNumEngines; iEng++)
	{
		dmaFreeEngine(getDmaEngine(deviceNumber, iEng));
	}

	pNTV2Params->_dmaNumEngines = 0;

	NTV2_MSG_INFO("%s%d: dmaRelease end\n", DMA_MSG_DEVICE);
}

int dmaEnable(ULWord deviceNumber)
{
	NTV2PrivateParams *pNTV2Params = getNTV2Params(deviceNumber);
	PDMA_ENGINE pDmaEngine;
	ULWord iEng;

	if (pNTV2Params->_dmaNumEngines == 0)
	{
		NTV2_MSG_INFO("%s%d: dmaEnable no engines to enable\n", DMA_MSG_DEVICE);
		return 0;
	}

	NTV2_MSG_INFO("%s%d: dmaEnable begin\n", DMA_MSG_DEVICE);

	// enable all engines
	for(iEng = 0; iEng < pNTV2Params->_dmaNumEngines; iEng++)
	{
		pDmaEngine = getDmaEngine(deviceNumber, iEng);
		if (!pDmaEngine->engInit)
		{
			return -EPERM;
		}

		pDmaEngine->dmaEnable = true;
	}

	NTV2_MSG_INFO("%s%d: dmaEnable end\n", DMA_MSG_DEVICE);

	return 0;
}

void dmaDisable(ULWord deviceNumber)
{
	NTV2PrivateParams *pNTV2Params = getNTV2Params(deviceNumber);
	PDMA_ENGINE pDmaEngine;
	ULWord timeoutJiffies = microsecondsToJiffies(DMA_TRANSFER_TIMEOUT/10);
	ULWord iEng;

	if (pNTV2Params->_dmaNumEngines == 0)
	{
		NTV2_MSG_INFO("%s%d: dmaDisable no engines to disable\n", DMA_MSG_DEVICE);
		return;
	}

	NTV2_MSG_INFO("%s%d: dmaDisable begin\n", DMA_MSG_DEVICE);

	// disable all engines
	for(iEng = 0; iEng < pNTV2Params->_dmaNumEngines; iEng++)
	{
		pDmaEngine = getDmaEngine(deviceNumber, iEng);
		if (!pDmaEngine->engInit)
		{
			continue;
		}

		// mark engine disabled
		dmaEngineLock(pDmaEngine);
		pDmaEngine->dmaEnable = false;
		dmaEngineUnlock(pDmaEngine);

		// wait for dma to complete
		dmaHardwareAcquire(pDmaEngine, timeoutJiffies);
		dmaHardwareRelease(pDmaEngine);
	}

	NTV2_MSG_INFO("%s%d: dmaDisable end\n", DMA_MSG_DEVICE);
}

static PDMA_ENGINE dmaMapEngine(ULWord deviceNumber, NTV2DMAEngine eDMAEngine, bool bToHost)
{
	NTV2PrivateParams *pNTV2Params = getNTV2Params(deviceNumber);
	ULWord engIndex = DMA_NUM_ENGINES;
	bool uni = ((pNTV2Params->_dmaMethod == DmaMethodNwl) || 
				(pNTV2Params->_dmaMethod == DmaMethodXlnx));

	// find the correct engine
	switch (eDMAEngine)
	{
	default:
	case NTV2_DMA1:
		if (uni)
		{
			engIndex = 0;
			if (bToHost) engIndex = 1;
		}
		else
		{
			engIndex = 0;
		}
		break;
	case NTV2_DMA2:
		if (uni)
		{
			engIndex = 2;
			if (bToHost) engIndex = 3;
		}
		else
		{
			engIndex = 1;
		}
		break;
	case NTV2_DMA3:
		if (uni)
		{
			engIndex = 4;
			if (bToHost) engIndex = 5;
		}
		else
		{
			engIndex = 2;
		}
		break;
	case NTV2_DMA4:
		if (uni)
		{
			engIndex = 6;
			if (bToHost) engIndex = 7;
		}
		else
		{
			engIndex = 3;
		}
		break;
	}

	if (engIndex > pNTV2Params->_dmaNumEngines) return NULL;
	
	return getDmaEngine(deviceNumber, engIndex);
}
#if 0
static PDMA_ENGINE dmaMapStream(ULWord deviceNumber, NTV2Channel eChannel, bool bToHost)
{
    return NULL;
}
#endif
static bool dmaHardwareInit(PDMA_ENGINE pDmaEngine)
{
	ULWord deviceNumber = pDmaEngine->deviceNumber;
    ULWord value = 0;
	bool present = false;
	
	switch (pDmaEngine->dmaMethod)
	{
	case DmaMethodAja:
		present = true;
		break;
	case DmaMethodNwl:
		present = IsNwlChannel(deviceNumber, pDmaEngine->dmaC2H, pDmaEngine->dmaIndex);
		break;
	case DmaMethodXlnx:
		present = IsXlnxChannel(deviceNumber, pDmaEngine->dmaC2H, pDmaEngine->dmaIndex);
        value = XlnxReadChannelIdentifier(deviceNumber, pDmaEngine->dmaC2H, pDmaEngine->dmaIndex);
        pDmaEngine->dmaStream = IsXlnxChannelStream(value);
#if 0 // does not work
        value = XlnxReadChannelAlignments(deviceNumber, pDmaEngine->dmaC2H, pDmaEngine->dmaIndex);
        if (value != 0)
        {
            pDmaEngine->alignmentMask = GetXlnxAddressAlignment(value) - 1;
            pDmaEngine->granularityMask = GetXlnxTransferAlignment(value) - 1;
        }
#endif
        if (pDmaEngine->dmaStream)
        {
            // 64 byte alignment
            pDmaEngine->alignmentMask = 0x3f;
            pDmaEngine->granularityMask = 0x3f;
        }

		break;
	default:
		return false;
	}

	if (present)
	{
		NTV2_MSG_INFO("%s%d:%s%d: dmaHardwareInit present\n", DMA_MSG_ENGINE);
	}
	else
	{
		NTV2_MSG_INFO("%s%d:%s%d: dmaHardwareInit not present\n", DMA_MSG_ENGINE);
		return false;
	}

	return true;
}

int dmaTransfer(PDMA_PARAMS pDmaParams)
{
	ULWord deviceNumber = pDmaParams->deviceNumber;
	NTV2PrivateParams *pNTV2Params = getNTV2Params(deviceNumber);
	PDMA_ENGINE pDmaEngine = NULL;
	PDMA_CONTEXT pDmaContext = NULL;
	PDMA_PAGE_BUFFER pVideoPageBuffer = NULL;
	PDMA_PAGE_BUFFER pAudioPageBuffer = NULL;
	PDMA_PAGE_BUFFER pAncF1PageBuffer = NULL;
	PDMA_PAGE_BUFFER pAncF2PageBuffer = NULL;
	bool dmaC2H = false;
	ULWord direction = 0;
	ULWord timeoutJiffies = microsecondsToJiffies(DMA_TRANSFER_TIMEOUT/10);
	int status = 0;
	int dmaStatus = 0;
	ULWord errorCount = 0;
	ULWord memorySize = 0;
	ULWord frameBufferSize = 0;
	bool hasVideo = false;
	bool hasAudio = false;
	bool hasAncF1 = false;
	bool hasAncF2 = false;
	bool hasMessage = false;
	bool findVideo = false;
	bool findAudio = false;
	bool findAncF1 = false;
	bool findAncF2 = false;
	bool lockVideo = false;
	bool lockAudio = false;
	bool lockAncF1 = false;
	bool lockAncF2 = false;
	bool mapVideo = false;
	bool mapAudio = false;
	bool mapAncF1 = false;
	bool mapAncF2 = false;
	bool doVideo = false;
	bool doAudio = false;
	bool doAncF1 = false;
	bool doAncF2 = false;
	bool doMessage = false;
	bool serialAcquired = false;
	bool engineAcquired = false;
	bool rdma = false;
	ULWord videoFrameOffset = 0;
	ULWord videoNumBytes = 0;
	ULWord videoFramePitch = 0;
	ULWord videoUserPitch = 0;
	ULWord videoNumSegments = 0;
	ULWord videoCardAddress = 0;
	ULWord videoCardBytes = 0;
	ULWord messageCardAddress = 0;
	ULWord audioRingOffset = 0;
	ULWord audioCardBytes = 0;
	ULWord audioRingAddress = 0;
	ULWord audioRingSize = 0;
	ULWord ancF1FrameOffset = 0;
	ULWord ancF1CardAddress = 0;
	ULWord ancF1CardBytes = 0;
	ULWord ancF2FrameOffset = 0;
	ULWord ancF2CardAddress = 0;
	ULWord ancF2CardBytes = 0;
	bool serialTransfer = false;
	LWord64 softStartTime = 0;
	LWord64 softLockTime = 0;
	LWord64 softDmaWaitTime = 0;
	LWord64 softDmaTime = 0;
	LWord64 softUnlockTime = 0;
	LWord64 softDoneTime = 0;
	Ntv2SystemContext systemContext;
	systemContext.devNum = deviceNumber;

	softStartTime = ntv2Time100ns();
	softLockTime = softStartTime;
	softDmaWaitTime = softStartTime;
	softDmaTime = softStartTime;
	softUnlockTime = softStartTime;
	softDoneTime = softStartTime;

	if (pDmaParams == NULL)
		return -EPERM;
	
	pDmaEngine = dmaMapEngine(deviceNumber, pDmaParams->dmaEngine, pDmaParams->toHost);
	if (pDmaEngine == NULL) 
	{
		NTV2_MSG_ERROR("%s%d: dmaTransfer can not find dma engine to match  toHost %d  dmaEngine %d\n",
					   DMA_MSG_DEVICE, pDmaParams->toHost, pDmaParams->dmaEngine);
		return -EPERM;
	}

 	NTV2_MSG_TRANSFER("%s%d:%s%d: dmaTransfer toHost %d  dmaEngine %d",
					  DMA_MSG_ENGINE, pDmaParams->toHost, pDmaParams->dmaEngine);
 	NTV2_MSG_TRANSFER("%s%d:%s%d: dmaTransfer pVidUserVa %016llx  vidChannel %d  vidFrame %d  vidNumBytes %d\n",
					  DMA_MSG_ENGINE, (ULWord64)pDmaParams->pVidUserVa, pDmaParams->videoChannel, 
					  pDmaParams->videoFrame, pDmaParams->vidNumBytes);
 	NTV2_MSG_TRANSFER("%s%d:%s%d: dmaTransfer vidBusAddress %016llx  vidBusSize %d  msgBusAddress %016llx  msgData %08x\n",
					  DMA_MSG_ENGINE, pDmaParams->videoBusAddress, pDmaParams->videoBusSize, 
					  pDmaParams->messageBusAddress, pDmaParams->messageData);
 	NTV2_MSG_TRANSFER("%s%d:%s%d: dmaTransfer frameOffset %d  vidUserPitch %d  framePitch %d  numSegments %d\n",
					  DMA_MSG_ENGINE, pDmaParams->frameOffset, pDmaParams->vidUserPitch, 
					  pDmaParams->vidFramePitch, pDmaParams->numSegments);
 	NTV2_MSG_TRANSFER("%s%d:%s%d: dmaTransfer pAudUserVa %016llx  audioSystem %d  audNumBytes %d  audOffset %d	audioSystemCount %d\n",
					  DMA_MSG_ENGINE, (ULWord64)pDmaParams->pAudUserVa, pDmaParams->audioSystem, 
					  pDmaParams->audNumBytes, pDmaParams->audOffset, pDmaParams->audioSystemCount);
 	NTV2_MSG_TRANSFER("%s%d:%s%d: dmaTransfer pAncF1UserVa %016llx  ancF1Frame %d  ancF1NumBytes %d  ancF1Offset %d\n",
					  DMA_MSG_ENGINE, (ULWord64)pDmaParams->pAncF1UserVa, pDmaParams->ancF1Frame,
					  pDmaParams->ancF1NumBytes, pDmaParams->ancF1Offset);
 	NTV2_MSG_TRANSFER("%s%d:%s%d: dmaTransfer pAncF2UserVa %016llx  ancF2Frame %d  ancF2NumBytes %d  ancF2Offset %d\n",
					  DMA_MSG_ENGINE, (ULWord64)pDmaParams->pAncF2UserVa, pDmaParams->ancF2Frame, 
					  pDmaParams->ancF2NumBytes, pDmaParams->ancF2Offset);

	// check for no video, audio or anc to transfer
	if(((pDmaParams->pVidUserVa == NULL) || (pDmaParams->vidNumBytes == 0)) &&
	   ((pDmaParams->videoBusAddress == 0) || (pDmaParams->videoBusSize == 0)) &&
	   ((pDmaParams->pAudUserVa == NULL) || (pDmaParams->audNumBytes == 0)) &&
	   ((pDmaParams->pAncF1UserVa == NULL) || (pDmaParams->ancF1NumBytes == 0)) &&
	   ((pDmaParams->pAncF2UserVa == NULL) || (pDmaParams->ancF2NumBytes == 0)))
	{
		return 0;
	}

	// check enabled
	if (!pDmaEngine->dmaEnable)
	{
		errorCount++;
		NTV2_MSG_ERROR("%s%d:%s%d: dmaTransfer engine not enabled\n", DMA_MSG_ENGINE);
		return -EPERM;
	}

	if (pDmaEngine->dmaStream)
	{
		errorCount++;
		NTV2_MSG_ERROR("%s%d:%s%d: dmaTransfer engine is streaming\n", DMA_MSG_ENGINE);
		return -EPERM;
	}

	// serialize all dma if set
	serialTransfer = ReadRegister(deviceNumber, kVRegDmaSerialize, NO_MASK, NO_SHIFT);

	// wait for page lock resource
	pDmaContext = dmaContextAcquire(pDmaEngine, timeoutJiffies);
	if (pDmaContext != NULL)
	{
		// check enabled
		if (!pDmaEngine->dmaEnable)
		{
			errorCount++;
			NTV2_MSG_ERROR("%s%d:%s%d: dmaTransfer engine not enabled\n", DMA_MSG_ENGINE);
			dmaStatus = -EPERM;
			dmaContextRelease(pDmaContext);
		}
	}
	else
	{
		errorCount++;
		NTV2_MSG_ERROR("%s%d:%s%d: dmaTransfer acquire context timeout\n", DMA_MSG_ENGINE);
		dmaStatus = -EBUSY;
	}

	if (dmaStatus == 0)
	{
		// record the lock start time
		softLockTime = ntv2Time100ns();
		
		// aja dma engines are bidirectional
		if (pDmaEngine->dmaMethod == DmaMethodAja)
		{
			pDmaContext->dmaC2H = pDmaParams->toHost;
		}
		dmaC2H = pDmaContext->dmaC2H;
		direction = dmaC2H ? DMA_FROM_DEVICE : DMA_TO_DEVICE;

		// do nothing by default
		pDmaContext->pVideoPageBuffer = NULL;
		pDmaContext->pAudioPageBuffer = NULL;
		pDmaContext->pAncF1PageBuffer = NULL;
		pDmaContext->pAncF2PageBuffer = NULL;
		pDmaContext->doVideo = false;
		pDmaContext->doAudio = false;
		pDmaContext->doAncF1 = false;
		pDmaContext->doAncF2 = false;
		pDmaContext->doMessage = false;

		// get video info
		memorySize = NTV2DeviceGetActiveMemorySize(pNTV2Params->_DeviceID);
		frameBufferSize = GetFrameBufferSize(&systemContext, pDmaParams->videoChannel);

		// look for video to dma
		hasVideo = true;

		dmaVideoSegmentInit(pDmaContext, &pDmaContext->dmaVideoSegment);

		// enforce 4 byte alignment
		videoFrameOffset = pDmaParams->frameOffset & 0xfffffffc;
		videoNumBytes = pDmaParams->vidNumBytes & 0xfffffffc;
		videoFramePitch = pDmaParams->vidFramePitch & 0xfffffffc;
		videoUserPitch = pDmaParams->vidUserPitch & 0xfffffffc;
		videoNumSegments = pDmaParams->numSegments;

		// verify number of data bytes
		if(videoNumBytes == 0)
		{
			hasVideo = false;
		}

		// verify number of segments
		if(videoNumSegments > 1000000)
		{
			hasVideo = false;
			NTV2_MSG_ERROR("%s%d:%s%d:%s%d: dmaTransfer number of video segment to large %d\n", 
						   DMA_MSG_CONTEXT, videoNumSegments);
		}
		if(videoNumSegments == 0)
		{
			videoNumSegments = 1;
		}

		// compute card address and size
		videoCardAddress = pDmaParams->videoFrame * frameBufferSize + videoFrameOffset;
		videoCardBytes = videoUserPitch*(videoNumSegments - 1) + videoNumBytes;

		if(hasVideo)
		{
			if(pDmaParams->pVidUserVa != NULL)
			{
				// check buffer cache
				pVideoPageBuffer = dmaPageRootFind(deviceNumber,
												   pDmaParams->pPageRoot,
												   pDmaParams->pVidUserVa,
												   videoCardBytes);
				if (pVideoPageBuffer != NULL)
				{
					findVideo = true;
				}
				else
				{
					// auto lock the buffer
					if (dmaPageRootAutoLock(pDmaParams->pPageRoot))
					{
						dmaPageRootPrune(deviceNumber,
										 pDmaParams->pPageRoot,
										 videoCardBytes);
						dmaPageRootAdd(deviceNumber,
									   pDmaParams->pPageRoot,
									   pDmaParams->pVidUserVa,
									   videoCardBytes,
									   false,
									   dmaPageRootAutoMap(pDmaParams->pPageRoot));
						pVideoPageBuffer = dmaPageRootFind(deviceNumber,
														   pDmaParams->pPageRoot,
														   pDmaParams->pVidUserVa,
														   videoCardBytes);
						if (pVideoPageBuffer != NULL)
						{
							findVideo = true;
						}
						else
						{
							pVideoPageBuffer = &pDmaContext->videoPageBuffer;
						}
					}
					else
					{
						pVideoPageBuffer = &pDmaContext->videoPageBuffer;
					}
				}
				lockVideo = !dmaPageLocked(pVideoPageBuffer);
				mapVideo = !dmaSgMapped(pVideoPageBuffer);

				if (lockVideo)
				{
					// lock pages for dma
					dmaPageLock(deviceNumber,
								pVideoPageBuffer,
								pDmaParams->pVidUserVa,
								videoCardBytes,
								direction);
				}

				if (mapVideo)
				{
					dmaSgMap(deviceNumber, pVideoPageBuffer);
				}
				else
				{
					dmaSgDevice(deviceNumber, pVideoPageBuffer);
				}

				if (!dmaSgMapped(pVideoPageBuffer))
				{
					hasVideo = false;
					errorCount++;
					NTV2_MSG_ERROR("%s%d:%s%d:%s%d: dmaTransfer video transfer failed\n", 
								   DMA_MSG_CONTEXT);
				}
			}
			else if ((pDmaParams->videoBusAddress != 0) && (pDmaParams->videoBusSize != 0))
			{
				if (videoCardBytes <= pDmaParams->videoBusSize)
				{
					// use prealloc page buffer
					pVideoPageBuffer = &pDmaContext->videoPageBuffer;

					// map the physical target
					status = dmaBusMap(deviceNumber, pVideoPageBuffer,
									   pDmaParams->videoBusAddress, videoCardBytes);
					lockVideo = true;
					if (status != 0)
					{
						hasVideo = false;
						lockVideo = false;
						errorCount++;
						NTV2_MSG_ERROR("%s%d:%s%d:%s%d: dmaTransfer video dmaBusMap failed\n", 
									   DMA_MSG_CONTEXT);
					}
				}
				else
				{
					hasVideo = false;
					errorCount++;
					NTV2_MSG_ERROR("%s%d:%s%d:%s%d: dmaTransfer p2p target aperture %d < transfer size %d\n", 
								   DMA_MSG_CONTEXT, pDmaParams->videoBusSize, videoCardBytes);
				}

				if (hasVideo && (pDmaParams->messageBusAddress != 0))
				{
					// compute message location
					messageCardAddress = (pDmaParams->videoFrame + 1) * frameBufferSize - 4;  // last 4 bytes of frame

					if ((pNTV2Params->_FrameApertureBaseAddress != 0) &&
						(messageCardAddress < pNTV2Params->_FrameApertureBaseSize))
					{
						hasMessage = true;
					}
					else
					{
						hasVideo = false;
						errorCount++;
						NTV2_MSG_ERROR("%s%d:%s%d:%s%d: dmaTransfer bad p2p frame aperture  address %08lx  size %d\n", 
									   DMA_MSG_CONTEXT,
									   pNTV2Params->_FrameApertureBaseAddress,
									   pNTV2Params->_FrameApertureBaseSize);
					}
				}
			}
			else
			{
				hasVideo = false;
			}

			// configure the dma transfer
			if (hasVideo)
			{
				if (dmaVideoSegmentConfig(pDmaContext,
										  &pDmaContext->dmaVideoSegment,
										  videoCardAddress,
										  videoCardBytes,
										  videoFramePitch,
										  videoUserPitch,
										  videoNumBytes,
										  videoNumSegments,
										  false))
				{
					doVideo = true;
					if (hasMessage)
					{
						// do p2p message
						pDmaContext->messageBusAddress = pDmaParams->messageBusAddress;
						pDmaContext->messageCardAddress = messageCardAddress;
						doMessage = true;
					}
				}
			}
		}

		// get audio info
		if(NTV2DeviceCanDoStackedAudio(pNTV2Params->_DeviceID))
		{
			audioRingAddress = memorySize - (NTV2_AUDIO_BUFFEROFFSET_BIG * (pDmaParams->audioSystem + 1));
		}
		else
		{
			audioRingAddress = GetAudioFrameBufferNumber(deviceNumber,
														 (getNTV2Params(deviceNumber)->_DeviceID),
														 pDmaParams->audioSystem) * frameBufferSize;
		}
		if (dmaC2H)
		{
			audioRingAddress += GetAudioReadOffset(deviceNumber, pDmaParams->audioSystem);
		}
		audioRingSize = GetAudioWrapAddress(deviceNumber, pDmaParams->audioSystem);

		// look for audio to dma
		hasAudio = true;

		dmaAudioSegmentInit(pDmaContext, &pDmaContext->dmaAudioSegment);

		// enforce 4 byte alignment
		if ((pDmaParams->audioSystemCount > 0) ||
			(pDmaParams->audioSystemCount <= MAX_NUM_AUDIO_LINKS))
		{
			audioRingOffset = pDmaParams->audOffset & 0xfffffffc;
            audioCardBytes = pDmaParams->audNumBytes & 0xfffffffc;
			
			// verify audio buffer
			if((pDmaParams->pAudUserVa == NULL) || (audioCardBytes == 0))
			{
				hasAudio = false;
			}
		}
		else
		{
			hasAudio = false;
		}

		if(hasAudio)
		{
			// check buffer cache
			pAudioPageBuffer = dmaPageRootFind(deviceNumber,
											   pDmaParams->pPageRoot,
											   pDmaParams->pAudUserVa,
											   audioCardBytes);
			if (pAudioPageBuffer != NULL)
			{
				findAudio = true;
			}
			else
			{
				// auto lock the buffer
				if (dmaPageRootAutoLock(pDmaParams->pPageRoot))
				{
					dmaPageRootPrune(deviceNumber,
									 pDmaParams->pPageRoot,
									 audioCardBytes);
					dmaPageRootAdd(deviceNumber,
								   pDmaParams->pPageRoot,
								   pDmaParams->pAudUserVa,
								   audioCardBytes,
								   false,
								   dmaPageRootAutoMap(pDmaParams->pPageRoot));
					pAudioPageBuffer = dmaPageRootFind(deviceNumber,
													   pDmaParams->pPageRoot,
													   pDmaParams->pAudUserVa,
													   audioCardBytes);
					if (pAudioPageBuffer != NULL)
					{
						findAudio = true;
					}
					else
					{
						pAudioPageBuffer = &pDmaContext->audioPageBuffer;
					}
				}
				else
				{
					pAudioPageBuffer = &pDmaContext->audioPageBuffer;
				}
			}
			lockAudio = !dmaPageLocked(pAudioPageBuffer);
			mapAudio = !dmaSgMapped(pAudioPageBuffer);

			if (lockAudio)
			{
				// lock pages for dma
				dmaPageLock(deviceNumber,
							pAudioPageBuffer,
									 pDmaParams->pAudUserVa,
							audioCardBytes,
							direction);
			}

			if (mapAudio)
			{
				dmaSgMap(deviceNumber, pAudioPageBuffer);
			}
			else
			{
				dmaSgDevice(deviceNumber, pAudioPageBuffer);
			}

			if (!dmaSgMapped(pAudioPageBuffer))
			{
				hasAudio = false;
				errorCount++;
				NTV2_MSG_ERROR("%s%d:%s%d:%s%d: dmaTransfer audio transfer failed\n", 
							   DMA_MSG_CONTEXT);
			}

			// configure the dma transfer
			if (hasAudio)
			{
				ULWord pAddress[MAX_NUM_AUDIO_LINKS];
				ULWord i = 0;
				for (i = 0; i < MAX_NUM_AUDIO_LINKS; i++)
				{
					if (i < pDmaParams->audioSystemCount)
						pAddress[i] = audioRingAddress - (i * NTV2_AUDIO_BUFFEROFFSET_BIG);
					else
						pAddress[i] = 0;
				}
				if(dmaAudioSegmentConfig(pDmaContext,
										 &pDmaContext->dmaAudioSegment,
										 pAudioPageBuffer->userSize,
										 audioCardBytes,
										 pAddress,
										 audioRingSize,
										 audioRingOffset))
				{
					doAudio = true;
				}
			}
		}

		// look for anc field 1 to dma
		hasAncF1 = true;

		dmaAncSegmentInit(pDmaContext, &pDmaContext->dmaAncF1Segment);

		// enforce 4 byte alignment
		ancF1FrameOffset = pDmaParams->ancF1Offset & 0xfffffffc;
		ancF1CardBytes = pDmaParams->ancF1NumBytes & 0xfffffffc;

		// verify ancillary buffer
		if((pDmaParams->pAncF1UserVa == NULL) || (ancF1CardBytes == 0))
		{
			hasAncF1 = false;
		}

		// compute card address and size
		ancF1CardAddress = pDmaParams->ancF1Frame * frameBufferSize + ancF1FrameOffset;

		// setup ancillary dma
		if(hasAncF1)
		{
			// check buffer cache
			pAncF1PageBuffer = dmaPageRootFind(deviceNumber,
											   pDmaParams->pPageRoot,
											   pDmaParams->pAncF1UserVa,
											   ancF1CardBytes);
			if (pAncF1PageBuffer != NULL)
			{
				findAncF1 = true;
			}
			else
			{
				// auto lock the buffer
				if (dmaPageRootAutoLock(pDmaParams->pPageRoot))
				{
					dmaPageRootPrune(deviceNumber,
									 pDmaParams->pPageRoot,
									 ancF1CardBytes);
					dmaPageRootAdd(deviceNumber,
								   pDmaParams->pPageRoot,
								   pDmaParams->pAncF1UserVa,
								   ancF1CardBytes,
								   false,
								   dmaPageRootAutoMap(pDmaParams->pPageRoot));
					pAncF1PageBuffer = dmaPageRootFind(deviceNumber,
													   pDmaParams->pPageRoot,
													   pDmaParams->pAncF1UserVa,
													   ancF1CardBytes);
					if (pAncF1PageBuffer != NULL)
					{
						findAncF1 = true;
					}
					else
					{
						pAncF1PageBuffer = &pDmaContext->ancF1PageBuffer;
					}
				}
				else
				{
					pAncF1PageBuffer = &pDmaContext->ancF1PageBuffer;
				}
			}
			lockAncF1 = !dmaPageLocked(pAncF1PageBuffer);
			mapAncF1 = !dmaSgMapped(pAncF1PageBuffer);

			if (lockAncF1)
			{
				// lock pages for dma
				dmaPageLock(deviceNumber,
							pAncF1PageBuffer,
							pDmaParams->pAncF1UserVa,
							ancF1CardBytes,
							direction);
			}
			
			if (mapAncF1)
			{
				dmaSgMap(deviceNumber, pAncF1PageBuffer);
			}
			else
			{
				dmaSgDevice(deviceNumber, pAncF1PageBuffer);
			}

			if (!dmaSgMapped(pAncF1PageBuffer))
			{
				hasAncF1 = false;
				errorCount++;
				NTV2_MSG_ERROR("%s%d:%s%d:%s%d: dmaTransfer anc field 1 transfer failed\n", 
							   DMA_MSG_CONTEXT);
			}

			// configure the dma transfer
			if (hasAncF1)
			{
				if(dmaAncSegmentConfig(pDmaContext,
									   &pDmaContext->dmaAncF1Segment,
									   ancF1CardAddress,
									   ancF1CardBytes))
				{
					doAncF1 = true;
				}
			}
		}

		// look for anc field 2 to dma
		hasAncF2 = true;

		dmaAncSegmentInit(pDmaContext, &pDmaContext->dmaAncF2Segment);

		// enforce 4 byte alignment
		ancF2FrameOffset = pDmaParams->ancF2Offset & 0xfffffffc;
		ancF2CardBytes = pDmaParams->ancF2NumBytes & 0xfffffffc;

		// verify ancillary buffer
		if((pDmaParams->pAncF2UserVa == NULL) || (ancF2CardBytes == 0))
		{
			hasAncF2 = false;
		}

		// compute card address and size
		ancF2CardAddress = pDmaParams->ancF2Frame * frameBufferSize + ancF2FrameOffset;

		// setup ancillary dma
		if(hasAncF2)
		{
			// check buffer cache
			pAncF2PageBuffer = dmaPageRootFind(deviceNumber,
											   pDmaParams->pPageRoot,
											   pDmaParams->pAncF2UserVa,
											   ancF2CardBytes);
			if (pAncF2PageBuffer != NULL)
			{
				findAncF2 = true;
			}
			else
			{
				// auto lock the buffer
				if (dmaPageRootAutoLock(pDmaParams->pPageRoot))
				{
					dmaPageRootPrune(deviceNumber,
									 pDmaParams->pPageRoot,
									 ancF2CardBytes);
					dmaPageRootAdd(deviceNumber,
								   pDmaParams->pPageRoot,
								   pDmaParams->pAncF2UserVa,
								   ancF2CardBytes,
								   false,
								   dmaPageRootAutoMap(pDmaParams->pPageRoot));
					pAncF2PageBuffer = dmaPageRootFind(deviceNumber,
													   pDmaParams->pPageRoot,
													   pDmaParams->pAncF2UserVa,
													   ancF2CardBytes);
					if (pAncF2PageBuffer != NULL)
					{
						findAncF2 = true;
					}
					else
					{
						pAncF2PageBuffer = &pDmaContext->ancF2PageBuffer;
					}
				}
				else
				{
					pAncF2PageBuffer = &pDmaContext->ancF2PageBuffer;
				}
			}

			lockAncF2 = !dmaPageLocked(pAncF2PageBuffer);
			mapAncF2 = !dmaSgMapped(pAncF2PageBuffer);

			if (lockAncF2)
			{
				// lock pages for dma
				dmaPageLock(deviceNumber,
							pAncF2PageBuffer,
							pDmaParams->pAncF2UserVa,
							ancF2CardBytes,
							direction);
			}
			
			if (mapAncF2)
			{
				dmaSgMap(deviceNumber, pAncF2PageBuffer);
			}
			else
			{
				dmaSgDevice(deviceNumber, pAncF2PageBuffer);
			}

			if (!dmaSgMapped(pAncF2PageBuffer))
			{
				hasAncF2 = false;
				errorCount++;
				NTV2_MSG_ERROR("%s%d:%s%d:%s%d: dmaTransfer anc field 2 transfer failed\n", 
							   DMA_MSG_CONTEXT);
			}

			// configure the dma transfer
			if (hasAncF2)
			{
				if(dmaAncSegmentConfig(pDmaContext,
									   &pDmaContext->dmaAncF2Segment,
									   ancF2CardAddress,
									   ancF2CardBytes))
				{
					doAncF2 = true;
				}
			}
		}

		if (!doVideo && !doAudio && !doAncF1 && !doAncF2)
		{
			errorCount++;
			dmaStatus = -EPERM;
			NTV2_MSG_ERROR("%s%d:%s%d:%s%d: dmaTransfer nothing to transfer\n",
						   DMA_MSG_CONTEXT);
		}

		if (dmaStatus == 0)
		{
			// record the wait start time
			softDmaWaitTime = ntv2Time100ns();
			
			// wait for dma hardware
			if (serialTransfer)
			{
				dmaStatus = dmaSerialAcquire(deviceNumber, timeoutJiffies);
				if (dmaStatus == 0)
				{
					serialAcquired = true;
					dmaStatus = dmaHardwareAcquire(pDmaEngine, timeoutJiffies);
				}
				else
				{
					errorCount++;
					NTV2_MSG_ERROR("%s%d:%s%d:%s%d: dmaTransfer acquire serial lock failed\n",
								   DMA_MSG_CONTEXT);
				}
			}
			else
			{
				dmaStatus = dmaHardwareAcquire(pDmaEngine, timeoutJiffies);
			}

			if (dmaStatus == 0)
			{
				engineAcquired = true;
			}
			else
			{
				errorCount++;
				NTV2_MSG_ERROR("%s%d:%s%d:%s%d: dmaTransfer acquire engine lock failed\n", DMA_MSG_CONTEXT);
			}

			if (dmaStatus == 0)
			{
				// check enabled
				if (!pDmaEngine->dmaEnable)
				{
					errorCount++;
					dmaStatus = -EPERM;
					NTV2_MSG_ERROR("%s%d:%s%d:%s%d: dmaTransfer engine not enabled\n", DMA_MSG_CONTEXT);
				}

				// check for correct engine state
				if (pDmaEngine->state != DmaStateIdle)
				{
					errorCount++;
					dmaStatus = -EPERM;
					NTV2_MSG_ERROR("%s%d:%s%d:%s%d: dmaTransfer engine state %d not idle\n",
								   DMA_MSG_CONTEXT, pDmaEngine->state);
				}
			}
		}

		if (dmaStatus == 0)
		{
			// record the transfer start time
			softDmaTime = ntv2Time100ns();

			// init hardware programming stats
			pDmaEngine->programStartCount = 0;
			pDmaEngine->programCompleteCount = 0;
			pDmaEngine->programDescriptorCount = 0;
			pDmaEngine->programErrorCount = 0;
			pDmaEngine->programBytes = 0;
			pDmaEngine->programTime = 0;

			NTV2_MSG_STATE("%s%d:%s%d:%s%d: dmaTransfer dma state setup\n", DMA_MSG_CONTEXT);
			pDmaEngine->state = DmaStateSetup;

			// set the program info
			pDmaContext->pVideoPageBuffer = pVideoPageBuffer;
			pDmaContext->pAudioPageBuffer = pAudioPageBuffer;
			pDmaContext->pAncF1PageBuffer = pAncF1PageBuffer;
			pDmaContext->pAncF2PageBuffer = pAncF2PageBuffer;
			pDmaContext->doVideo = doVideo;
			pDmaContext->doAudio = doAudio;
			pDmaContext->doAncF1 = doAncF1;
			pDmaContext->doAncF2 = doAncF2;
			pDmaContext->doMessage = doMessage;

			// look for an rdma transfer
			if (pVideoPageBuffer != NULL)
				rdma = pVideoPageBuffer->rdma;

			NTV2_MSG_STATE("%s%d:%s%d:%s%d: dmaTransfer dma state transfer\n", DMA_MSG_CONTEXT);
			pDmaEngine->state = DmaStateTransfer;

			// configure gma message
			if (doMessage)
			{
				// aperture must be 0 based
				WriteFrameApertureOffset(deviceNumber, 0);
				// write message data to frame buffer
				WriteFrameAperture(deviceNumber, messageCardAddress, pDmaParams->messageData);
			}
				
			// clear the transfer event
			clear_bit(0, &pDmaEngine->transferDone);

			// do the dma
			switch (pDmaEngine->dmaMethod)
			{
			case DmaMethodAja:
				dmaStatus = dmaAjaProgram(pDmaContext);
				break;
			case DmaMethodNwl:
				dmaStatus = dmaNwlProgram(pDmaContext);
				break;
			case DmaMethodXlnx:
				dmaStatus = dmaXlnxProgram(pDmaContext);
				break;
			default:
				errorCount++;
				dmaStatus = -EPERM;
				NTV2_MSG_ERROR("%s%d:%s%d:%s%d: dmaTransfer bad dma method %d\n",
							   DMA_MSG_CONTEXT, pDmaEngine->dmaMethod);
				break;
			}

			if (dmaStatus == 0)
			{
				status = wait_event_timeout(pDmaEngine->transferEvent,
											test_bit(0, &pDmaEngine->transferDone),
											timeoutJiffies);

				if (status == 0)
				{
					errorCount++;
					NTV2_MSG_ERROR("%s%d:%s%d:%s%d: dmaTransfer dma transfer timeout\n", 
								   DMA_MSG_CONTEXT);
					switch (pDmaEngine->dmaMethod)
					{
					case DmaMethodAja:
						dmaAjaAbort(pDmaEngine);
						break;
					case DmaMethodNwl:
						dmaNwlAbort(pDmaEngine);
						break;
					case DmaMethodXlnx:
						dmaXlnxAbort(pDmaEngine);
						break;
					default:
						break;
					}
					dmaStatus = -EPERM;
				}
			}
			else
			{
				NTV2_MSG_ERROR("%s%d:%s%d:%s%d: dmaTransfer program error %d\n",
							   DMA_MSG_CONTEXT, dmaStatus);
			}

			NTV2_MSG_STATE("%s%d:%s%d:%s%d: dmaTransfer dma state finish\n", DMA_MSG_CONTEXT);
			pDmaEngine->state = DmaStateFinish;

			// save dma programming statistics
			dmaEngineLock(pDmaEngine);
			if (dmaC2H)
			{
				pDmaEngine->csErrorCount += pDmaEngine->programErrorCount;
				pDmaEngine->csTransferBytes += pDmaEngine->programBytes;
				pDmaEngine->csDescriptorCount += pDmaEngine->programDescriptorCount;
				pDmaEngine->csHardTime += pDmaEngine->programTime;
			}
			else
			{
				pDmaEngine->scErrorCount += pDmaEngine->programErrorCount;
				pDmaEngine->scTransferBytes += pDmaEngine->programBytes;
				pDmaEngine->scDescriptorCount += pDmaEngine->programDescriptorCount;
				pDmaEngine->scHardTime += pDmaEngine->programTime;
			}
			dmaEngineUnlock(pDmaEngine);

			NTV2_MSG_STATE("%s%d:%s%d:%s%d: dmaTransfer dma state idle\n", DMA_MSG_CONTEXT);
			pDmaEngine->state = DmaStateIdle;
		}

		// release the dma engine
		if (engineAcquired)
		{
			dmaHardwareRelease(pDmaEngine);
		}
		if (serialAcquired)
		{
			dmaSerialRelease(deviceNumber);
		}

		softUnlockTime = ntv2Time100ns();

		if (mapVideo)
		{
			dmaSgUnmap(deviceNumber, pVideoPageBuffer);
		}
		else
		{
			dmaSgHost(deviceNumber, pVideoPageBuffer);
		}
		if (mapAudio)
		{
			dmaSgUnmap(deviceNumber, pAudioPageBuffer);
		}
		else
		{
			dmaSgHost(deviceNumber, pAudioPageBuffer);
		}
		if (mapAncF1)
		{
			dmaSgUnmap(deviceNumber, pAncF1PageBuffer);
		}
		else
		{
			dmaSgHost(deviceNumber, pAncF1PageBuffer);
		}
		if (mapAncF2)
		{
			dmaSgUnmap(deviceNumber, pAncF2PageBuffer);
		}
		else
		{
			dmaSgHost(deviceNumber, pAncF2PageBuffer);
		}

		if (lockVideo)
		{
			dmaPageUnlock(deviceNumber, pVideoPageBuffer);
		}
		if (lockAudio)
		{
			dmaPageUnlock(deviceNumber, pAudioPageBuffer);
		}
		if (lockAncF1)
		{
			dmaPageUnlock(deviceNumber, pAncF1PageBuffer);
		}
		if (lockAncF2)
		{
			dmaPageUnlock(deviceNumber, pAncF2PageBuffer);
		}
		
		if (findVideo)
		{
			dmaPageRootFree(deviceNumber, pVideoPageBuffer);
		}
		if (findAudio)
		{
			dmaPageRootFree(deviceNumber, pAudioPageBuffer);
		}
		if (findAncF1)
		{
			dmaPageRootFree(deviceNumber, pAncF1PageBuffer);
		}
		if (findAncF2)
		{
			dmaPageRootFree(deviceNumber, pAncF2PageBuffer);
		}

		// done
		pDmaContext->doVideo = false;
		pDmaContext->doAudio = false;
		pDmaContext->doAncF1 = false;
		pDmaContext->doAncF2 = false;
		pDmaContext->doMessage = false;
		
		// release the context
		dmaContextRelease(pDmaContext);
	}

	softDoneTime = ntv2Time100ns();

	dmaEngineLock(pDmaEngine);
	if (dmaC2H)
	{
		pDmaEngine->csTransferCount++;
		if (rdma)
			pDmaEngine->csRdmaCount++;
		pDmaEngine->csErrorCount += errorCount;
		pDmaEngine->csTransferTime += softDoneTime - softStartTime;
		pDmaEngine->csLockWaitTime += softLockTime - softStartTime;
		pDmaEngine->csLockTime += softDmaWaitTime - softLockTime;
		pDmaEngine->csDmaWaitTime += softDmaTime - softDmaWaitTime;
		pDmaEngine->csDmaTime += softUnlockTime - softDmaTime;
		pDmaEngine->csUnlockTime += softDoneTime - softUnlockTime;
	}
	else
	{
		pDmaEngine->scTransferCount++;
		if (rdma)
			pDmaEngine->scRdmaCount++;
		pDmaEngine->scErrorCount += errorCount;
		pDmaEngine->scTransferTime += softDoneTime - softStartTime;
		pDmaEngine->scLockWaitTime += softLockTime - softStartTime;
		pDmaEngine->scLockTime += softDmaWaitTime - softLockTime;
		pDmaEngine->scDmaWaitTime += softDmaTime - softDmaWaitTime;
		pDmaEngine->scDmaTime += softUnlockTime - softDmaTime;
		pDmaEngine->scUnlockTime += softDoneTime - softUnlockTime;
	}
	dmaEngineUnlock(pDmaEngine);

	dmaStatistics(pDmaEngine, dmaC2H);

	return dmaStatus;
}

int dmaTargetP2P(ULWord deviceNumber, NTV2_DMA_P2P_CONTROL_STRUCT* pParams)
{
	NTV2PrivateParams* pNTV2Params = getNTV2Params(deviceNumber);
	ULWord frameBufferSize;
	ULWord videoOffset;
	ULWord videoSize;
	Ntv2SystemContext systemContext;
	systemContext.devNum = deviceNumber;

	pParams->ullVideoBusAddress		= 0;
	pParams->ullMessageBusAddress	= 0;
	pParams->ulVideoBusSize			= 0;
	pParams->ulMessageData			= 0;

	frameBufferSize = GetFrameBufferSize(&systemContext, pParams->dmaChannel);
	videoOffset = pParams->ulFrameNumber * frameBufferSize + pParams->ulFrameOffset;
	videoSize = frameBufferSize - pParams->ulFrameOffset;

	if((pNTV2Params->_FrameAperturePhysicalAddress != 0) &&
	   ((videoOffset + videoSize) <= pNTV2Params->_FrameApertureBaseSize))
	{
		pParams->ullVideoBusAddress = pNTV2Params->_FrameAperturePhysicalAddress + videoOffset;
		pParams->ulVideoBusSize = videoSize;

		if(pParams->dmaChannel == NTV2_CHANNEL1)
		{
			pParams->ullMessageBusAddress = pNTV2Params->_pPhysicalOutputChannel1;
		}
		else if(pParams->dmaChannel == NTV2_CHANNEL2)
		{
			pParams->ullMessageBusAddress = pNTV2Params->_pPhysicalOutputChannel2;
		}
		else if(pParams->dmaChannel == NTV2_CHANNEL3)
		{
			pParams->ullMessageBusAddress = pNTV2Params->_pPhysicalOutputChannel3;
		}
		else if(pParams->dmaChannel == NTV2_CHANNEL4)
		{
			pParams->ullMessageBusAddress = pNTV2Params->_pPhysicalOutputChannel4;
		}
		else if(pParams->dmaChannel == NTV2_CHANNEL5)
		{
			pParams->ullMessageBusAddress = pNTV2Params->_pPhysicalOutputChannel5;
		}
		else if(pParams->dmaChannel == NTV2_CHANNEL6)
		{
			pParams->ullMessageBusAddress = pNTV2Params->_pPhysicalOutputChannel6;
		}
		else if(pParams->dmaChannel == NTV2_CHANNEL7)
		{
			pParams->ullMessageBusAddress = pNTV2Params->_pPhysicalOutputChannel7;
		}
		else if(pParams->dmaChannel == NTV2_CHANNEL8)
		{
			pParams->ullMessageBusAddress = pNTV2Params->_pPhysicalOutputChannel8;
		}

		if(pParams->ullMessageBusAddress != 0)
		{
			pParams->ulMessageData = pParams->ulFrameNumber;
		}

		NTV2_MSG_TRANSFER("%s%d: dmaTargetP2P  vidBusAddress %016llx  vidSize %08x  msgBusAddress %016llx  msgData %08x\n",
						  DMA_MSG_DEVICE,
						  pParams->ullVideoBusAddress, pParams->ulVideoBusSize,
						  pParams->ullMessageBusAddress, pParams->ulMessageData);
	}
	else
	{
		NTV2_MSG_ERROR("%s%d: dmaTargetP2P no p2p target memory aperture for frame %d\n",
					   DMA_MSG_DEVICE, pParams->ulFrameNumber);
		return -EINVAL;
	}

	return 0;
}

static void dmaStatistics(PDMA_ENGINE pDmaEngine, bool dmaC2H)
{
	LWord64 softStatTime;
	LWord64 statTransferCount;
	LWord64 statRdmaCount;
	LWord64 statDescriptorCount;
	LWord64 statTransferBytes;
	LWord64 statTransferTime;
	LWord64 statLockWaitTime;
	LWord64 statLockTime;
	LWord64 statDmaWaitTime;
	LWord64 statDmaTime;
	LWord64 statUnlockTime;
	LWord64 statHardTime;
	LWord64 statErrorCount;
	LWord64 statDisplayTime;
	ULWord hardwareRate;
	ULWord transferRate;

	softStatTime = ntv2Time100ns();

	dmaEngineLock(pDmaEngine);
	if (dmaC2H)
	{
		statTransferCount	= pDmaEngine->csTransferCount;
		statRdmaCount		= pDmaEngine->csRdmaCount;
		statErrorCount		= pDmaEngine->csErrorCount;
		statDescriptorCount	= pDmaEngine->csDescriptorCount;
		statTransferBytes	= pDmaEngine->csTransferBytes;
		statTransferTime	= pDmaEngine->csTransferTime;
		statLockWaitTime	= pDmaEngine->csLockWaitTime;
		statLockTime		= pDmaEngine->csLockTime;
		statDmaWaitTime		= pDmaEngine->csDmaWaitTime;
		statDmaTime			= pDmaEngine->csDmaTime;
		statUnlockTime		= pDmaEngine->csUnlockTime;
		statHardTime		= pDmaEngine->csHardTime;
		statDisplayTime		= softStatTime - pDmaEngine->csLastDisplayTime;
	}
	else
	{
		statTransferCount	= pDmaEngine->scTransferCount;
		statRdmaCount		= pDmaEngine->scRdmaCount;
		statErrorCount		= pDmaEngine->scErrorCount;
		statDescriptorCount	= pDmaEngine->scDescriptorCount;
		statTransferBytes	= pDmaEngine->scTransferBytes;
		statTransferTime	= pDmaEngine->scTransferTime;
		statLockWaitTime	= pDmaEngine->scLockWaitTime;
		statLockTime		= pDmaEngine->scLockTime;
		statDmaWaitTime		= pDmaEngine->scDmaWaitTime;
		statDmaTime			= pDmaEngine->scDmaTime;
		statUnlockTime		= pDmaEngine->scUnlockTime;
		statHardTime		= pDmaEngine->scHardTime;
		statDisplayTime		= softStatTime - pDmaEngine->scLastDisplayTime;
	}

	if (statDisplayTime > DMA_STATISTICS_INTERVAL)
	{
		if (dmaC2H)
		{
			pDmaEngine->csTransferCount		= 0;
			pDmaEngine->csRdmaCount			= 0;
			pDmaEngine->csErrorCount		= 0;
			pDmaEngine->csDescriptorCount	= 0;
			pDmaEngine->csTransferBytes		= 0;
			pDmaEngine->csTransferTime		= 0;
			pDmaEngine->csLockWaitTime		= 0;
			pDmaEngine->csLockTime			= 0;
			pDmaEngine->csDmaWaitTime		= 0;
			pDmaEngine->csDmaTime			= 0;
			pDmaEngine->csUnlockTime		= 0;
			pDmaEngine->csHardTime			= 0;
			pDmaEngine->csLastDisplayTime	= softStatTime;
		}
		else
		{
			pDmaEngine->scTransferCount		= 0;
			pDmaEngine->scRdmaCount			= 0;
			pDmaEngine->scErrorCount		= 0;
			pDmaEngine->scDescriptorCount	= 0;
			pDmaEngine->scTransferBytes		= 0;
			pDmaEngine->scTransferTime		= 0;
			pDmaEngine->scLockWaitTime		= 0;
			pDmaEngine->scLockTime			= 0;
			pDmaEngine->scDmaWaitTime		= 0;
			pDmaEngine->scDmaTime			= 0;
			pDmaEngine->scUnlockTime		= 0;
			pDmaEngine->scHardTime			= 0;
			pDmaEngine->scLastDisplayTime	= softStatTime;
		}
		dmaEngineUnlock(pDmaEngine);

		if (statTransferCount == 0) statTransferCount = 1;
		if (statTransferTime == 0) statTransferTime = 1;
		if (statHardTime == 0) statHardTime = 1;

		hardwareRate = (ULWord)(statTransferBytes * 10 / statHardTime);
		transferRate = (ULWord)(statTransferBytes * 10 / statTransferTime);

		if (dmaC2H)
		{
			WriteRegister(pDmaEngine->deviceNumber,
						  dmaHardwareRateC2HReg[pDmaEngine->engIndex],
						  hardwareRate, NO_MASK, NO_SHIFT);
			WriteRegister(pDmaEngine->deviceNumber,
						  dmaTransferRateC2HReg[pDmaEngine->engIndex],
						  transferRate, NO_MASK, NO_SHIFT);
		}
		else
		{
			WriteRegister(pDmaEngine->deviceNumber,
						  dmaHardwareRateH2CReg[pDmaEngine->engIndex],
						  hardwareRate, NO_MASK, NO_SHIFT);
			WriteRegister(pDmaEngine->deviceNumber,
						  dmaTransferRateH2CReg[pDmaEngine->engIndex],
						  transferRate, NO_MASK, NO_SHIFT);
		}
			
		NTV2_MSG_STATISTICS("%s%d:%s%d: dmaTransfer %s  trn %6d  rdma %6d  err %6d  time %6d us  size %6dk  desc %6d  perf %6d mb/s\n",
							DMA_MSG_ENGINE,
							dmaC2H? "c2h":"h2c",
							(ULWord)(statTransferCount),
							(ULWord)(statRdmaCount),
							(ULWord)(statErrorCount),
							(ULWord)(statTransferTime / statTransferCount / 10),
							(ULWord)(statTransferBytes / statTransferCount / 1000),
							(ULWord)(statDescriptorCount / statTransferCount),
							transferRate);
		NTV2_MSG_STATISTICS("%s%d:%s%d: dmaTransfer %s  lockw %6d  lock %6d  dmaw %6d  dma %6d  unlck %6d  hrd %6d  perf %6d\n",
							DMA_MSG_ENGINE,
							dmaC2H? "c2h":"h2c",
							(ULWord)(statLockWaitTime / statTransferCount / 10),
							(ULWord)(statLockTime / statTransferCount / 10),
							(ULWord)(statDmaWaitTime / statTransferCount / 10),
							(ULWord)(statDmaTime / statTransferCount / 10),
							(ULWord)(statUnlockTime / statTransferCount / 10),
							(ULWord)(statHardTime / statTransferCount / 10),
							hardwareRate);
	}
	else
	{
		dmaEngineUnlock(pDmaEngine);
	}
}
	
int dmaStreamStart(PDMA_PARAMS pDmaParams)
{
	ULWord deviceNumber = pDmaParams->deviceNumber;
	PDMA_ENGINE pDmaEngine = NULL;
	PDMA_CONTEXT pDmaContext = NULL;
	PDMA_PAGE_BUFFER pVideoPageBuffer = NULL;
	bool dmaC2H = false;
	ULWord direction = 0;
	ULWord timeoutJiffies = microsecondsToJiffies(DMA_TRANSFER_TIMEOUT/10);
	int dmaStatus = 0;
	ULWord errorCount = 0;
	ULWord videoFrameOffset = 0;
	ULWord videoNumBytes = 0;
	ULWord videoFramePitch = 0;
	ULWord videoUserPitch = 0;
	ULWord videoNumSegments = 0;
	ULWord videoCardAddress = 0;
	ULWord videoCardBytes = 0;
    ULWord frameBufferSize = 0;
	bool hasVideo = false;
	bool doVideo = false;
	bool engineAcquired = false;
    unsigned long flags;
	Ntv2SystemContext systemContext;
	systemContext.devNum = deviceNumber;

	if (pDmaParams == NULL)
		return -EPERM;
	
	if (pDmaParams->pPageRoot == NULL)
		return -EPERM;

	pDmaEngine = dmaMapEngine(deviceNumber, pDmaParams->dmaEngine, pDmaParams->toHost);
	if (pDmaEngine == NULL) 
	{
		NTV2_MSG_ERROR("%s%d: dmaStreamStart can not find dma engine to match  toHost %d  dmaEngine %d\n",
					   DMA_MSG_DEVICE, pDmaParams->toHost, pDmaParams->dmaEngine);
		return -EPERM;
	}

 	NTV2_MSG_TRANSFER("%s%d:%s%d: dmaStreamStart toHost %d  dmaEngine %d",
					  DMA_MSG_ENGINE, pDmaParams->toHost, pDmaParams->dmaEngine);
 	NTV2_MSG_TRANSFER("%s%d:%s%d: dmaStreamStart pVidUserVa %016llx  vidChannel %d  vidFrame %d  vidNumBytes %d\n",
					  DMA_MSG_ENGINE, (ULWord64)pDmaParams->pVidUserVa, pDmaParams->videoChannel, 
					  pDmaParams->videoFrame, pDmaParams->vidNumBytes);

	// check for no video
	if((pDmaParams->pVidUserVa == NULL) || (pDmaParams->vidNumBytes == 0))
	{
		return 0;
	}

	// check enabled
	if (!pDmaEngine->dmaEnable)
	{
		errorCount++;
		NTV2_MSG_ERROR("%s%d:%s%d: dmaStreamStart engine not enabled\n", DMA_MSG_ENGINE);
		return -EPERM;
	}

	if (!pDmaEngine->dmaStream)
	{
		errorCount++;
		NTV2_MSG_ERROR("%s%d:%s%d: dmaStreamStart engine is mapped\n", DMA_MSG_ENGINE);
		return -EPERM;
	}

	// wait for page lock resource
	pDmaContext = dmaContextAcquire(pDmaEngine, timeoutJiffies);
	if (pDmaContext != NULL)
	{
		// check enabled
		if (!pDmaEngine->dmaEnable)
		{
			errorCount++;
			NTV2_MSG_ERROR("%s%d:%s%d: dmaStreamStart engine not enabled\n", DMA_MSG_ENGINE);
			dmaStatus = -EPERM;
			dmaContextRelease(pDmaContext);
		}
	}
	else
	{
		errorCount++;
		NTV2_MSG_ERROR("%s%d:%s%d: dmaStreamStart acquire context timeout\n", DMA_MSG_ENGINE);
		dmaStatus = -EBUSY;
	}

	if (dmaStatus == 0)
	{
		dmaC2H = pDmaContext->dmaC2H;
		direction = dmaC2H ? DMA_FROM_DEVICE : DMA_TO_DEVICE;

		// do nothing by default
		pDmaContext->pVideoPageBuffer = NULL;
		pDmaContext->pAudioPageBuffer = NULL;
		pDmaContext->pAncF1PageBuffer = NULL;
		pDmaContext->pAncF2PageBuffer = NULL;
		pDmaContext->doVideo = false;
		pDmaContext->doAudio = false;
		pDmaContext->doAncF1 = false;
		pDmaContext->doAncF2 = false;
		pDmaContext->doMessage = false;

		// get video info
//		memorySize = NTV2DeviceGetActiveMemorySize(pNTV2Params->_DeviceID);
		frameBufferSize = GetFrameBufferSize(&systemContext, pDmaParams->videoChannel);

		// look for video to dma
		hasVideo = true;

		// enforce 4 byte alignment
		videoFrameOffset = pDmaParams->frameOffset;
		videoNumBytes = pDmaParams->vidNumBytes;
		videoFramePitch = pDmaParams->vidFramePitch;
		videoUserPitch = pDmaParams->vidUserPitch;
		videoNumSegments = pDmaParams->numSegments;

		// enforce alignment
		if (((ULWord)(ULWord64)pDmaParams->pVidUserVa & pDmaEngine->alignmentMask) != 0)
			hasVideo = false;
		if ((videoFrameOffset & pDmaEngine->alignmentMask) != 0)
			hasVideo = false;
		if ((videoFramePitch & pDmaEngine->alignmentMask) != 0)
			hasVideo = false;
		if ((videoUserPitch & pDmaEngine->alignmentMask) != 0)
			hasVideo = false;
		if ((videoNumBytes & pDmaEngine->granularityMask) != 0)
			hasVideo = false;

		// verify number of data bytes
		if(videoNumBytes == 0)
		{
			hasVideo = false;
		}

		// verify number of segments
		if(videoNumSegments == 0)
		{
			videoNumSegments = 1;
		}
		if(videoNumSegments != 1)
		{
			hasVideo = false;
			NTV2_MSG_ERROR("%s%d:%s%d:%s%d: dmaTransfer number of video segment to large %d\n", 
						   DMA_MSG_CONTEXT, videoNumSegments);
		}

		// compute card address and size
		videoCardAddress = pDmaParams->videoFrame * frameBufferSize + videoFrameOffset;
		videoCardBytes = videoUserPitch*(videoNumSegments - 1) + videoNumBytes;

		if(hasVideo)
		{
			if(pDmaParams->pVidUserVa != NULL)
			{
				// check buffer cache
				pVideoPageBuffer = dmaPageRootFind(deviceNumber,
												   pDmaParams->pPageRoot,
												   pDmaParams->pVidUserVa,
												   videoCardBytes);
				if (pVideoPageBuffer != NULL)
				{
					doVideo = true;
				}
			}
			else
			{
				hasVideo = false;
			}
		}

		if (!doVideo)
		{
			errorCount++;
			dmaStatus = -EPERM;
			NTV2_MSG_ERROR("%s%d:%s%d:%s%d: dmaStreamStart nothing to transfer\n",
						   DMA_MSG_CONTEXT);
		}

		if (dmaStatus == 0)
		{
			dmaStatus = dmaHardwareAcquire(pDmaEngine, timeoutJiffies);

			if (dmaStatus == 0)
			{
				engineAcquired = true;
			}
			else
			{
				errorCount++;
				NTV2_MSG_ERROR("%s%d:%s%d:%s%d: dmaStreamStart acquire engine lock failed\n", DMA_MSG_CONTEXT);
			}

			if (dmaStatus == 0)
			{
				// check enabled
				if (!pDmaEngine->dmaEnable)
				{
					errorCount++;
					dmaStatus = -EPERM;
					NTV2_MSG_ERROR("%s%d:%s%d:%s%d: dmaStreamStart engine not enabled\n", DMA_MSG_CONTEXT);
				}

				// check for correct engine state
				if (pDmaEngine->state != DmaStateIdle)
				{
					errorCount++;
					dmaStatus = -EPERM;
					NTV2_MSG_ERROR("%s%d:%s%d:%s%d: dmaStreamStart engine state %d not idle\n",
								   DMA_MSG_CONTEXT, pDmaEngine->state);
				}
			}
		}

		if (dmaStatus == 0)
		{
			// init hardware programming stats
			pDmaEngine->programStartCount = 0;
			pDmaEngine->programCompleteCount = 0;
			pDmaEngine->programDescriptorCount = 0;
			pDmaEngine->programErrorCount = 0;
			pDmaEngine->programBytes = 0;
			pDmaEngine->programTime = 0;

			NTV2_MSG_STATE("%s%d:%s%d:%s%d: dmaStreamStart dma state setup\n", DMA_MSG_CONTEXT);
			pDmaEngine->state = DmaStateSetup;

			// set the program info
			pDmaEngine->activeContext = pDmaContext->conIndex;
			pDmaContext->pVideoPageBuffer = pVideoPageBuffer;
			pDmaContext->doVideo = doVideo;

            // build the descriptor list
            dmaStatus = dmaXlnxStreamBuild(pDmaEngine, pVideoPageBuffer, 0);
            if (dmaStatus >= 0)
            {
                // link the last descriptor to the start
                dmaStatus = dmaXlnxStreamLink(pDmaEngine, pVideoPageBuffer->numSgs - 1, 0);
                if (dmaStatus != 0)
                {
                    errorCount++;
                    NTV2_MSG_ERROR("%s%d:%s%d:%s%d: dmaStreamStart dma stream link failed\n", DMA_MSG_CONTEXT);
                }
            }
            else
            {
                errorCount++;
                NTV2_MSG_ERROR("%s%d:%s%d:%s%d: dmaStreamStart dma stream build failed\n", DMA_MSG_CONTEXT);
            }

            if (dmaStatus == 0)
            {
                spin_lock_irqsave(&pDmaParams->pPageRoot->bufferLock, flags);
                dmaStatus = pDmaParams->pPageRoot->engineRef[pDmaEngine->engIndex];
                if (dmaStatus == 0)
                {
                    // start the dma engine
                    dmaStatus = dmaXlnxStreamStart(pDmaEngine, 0);
                    if (dmaStatus == 0)
                    {
                        // ref the resource
                        pDmaParams->pPageRoot->engineRef[pDmaEngine->engIndex]++;
                        spin_unlock_irqrestore(&pDmaParams->pPageRoot->bufferLock, flags);
                    }
                    else
                    {
                        spin_unlock_irqrestore(&pDmaParams->pPageRoot->bufferLock, flags);
                        NTV2_MSG_ERROR("%s%d:%s%d:%s%d: dmaStreamStart dma stream start failed\n", DMA_MSG_CONTEXT);
                    }
                }
                else
                {
                    spin_lock_irqsave(&pDmaParams->pPageRoot->bufferLock, flags);
                    NTV2_MSG_ERROR("%s%d:%s%d:%s%d: dmaStreamStart not engine owner\n", DMA_MSG_CONTEXT);
                }
            }

            if (dmaStatus == 0)
            {
                NTV2_MSG_STATE("%s%d:%s%d:%s%d: dmaStreamStart dma state transfer\n", DMA_MSG_CONTEXT);
                pDmaEngine->state = DmaStateTransfer;
            }
            else
            {
                errorCount++;
                NTV2_MSG_STATE("%s%d:%s%d:%s%d: dmaStreamStart dma state idle\n", DMA_MSG_CONTEXT);
                pDmaEngine->state = DmaStateIdle;
            }
		}

		// release the dma engine
		if (engineAcquired)
		{
			dmaHardwareRelease(pDmaEngine);
		}

        if (dmaStatus != 0)
        {
            if (pDmaContext != NULL)
            {
                pDmaContext->doVideo = false;
                dmaPageRootFree(deviceNumber, pVideoPageBuffer);
                dmaContextRelease(pDmaContext);
            }
        }
    }

    return 0;
}

int dmaStreamStop(PDMA_PARAMS pDmaParams)
{
	ULWord deviceNumber = pDmaParams->deviceNumber;
	PDMA_ENGINE pDmaEngine = NULL;
	PDMA_CONTEXT pDmaContext = NULL;
    unsigned long flags;
	int dmaStatus = 0;

	if (pDmaParams == NULL)
		return -EPERM;

    // get the dma engine
	pDmaEngine = dmaMapEngine(deviceNumber, pDmaParams->dmaEngine, pDmaParams->toHost);
	if (pDmaEngine == NULL) 
	{
		NTV2_MSG_ERROR("%s%d: dmaStreamStop can not find dma engine to match  toHost %d  dmaEngine %d\n",
					   DMA_MSG_DEVICE, pDmaParams->toHost, pDmaParams->dmaEngine);
		return -EPERM;
	}

 	NTV2_MSG_TRANSFER("%s%d:%s%d: dmaStreamStop toHost %d  dmaEngine %d",
					  DMA_MSG_ENGINE, pDmaParams->toHost, pDmaParams->dmaEngine);

	// check engine running
	if (pDmaEngine->state != DmaStateTransfer)
	{
		NTV2_MSG_ERROR("%s%d: dmaStreamStop dma state not transfer\n", DMA_MSG_DEVICE);
		return -EPERM;
	}
    
	// check if we own the engine
	if (pDmaParams->pPageRoot->engineRef[pDmaEngine->engIndex] == 0)
	{
		NTV2_MSG_ERROR("%s%d: dmaStreamStop not engine owner\n", DMA_MSG_DEVICE);
		return -EPERM;
	}

    // get the active context
	pDmaContext = &pDmaEngine->dmaContext[pDmaEngine->activeContext];

	spin_lock_irqsave(&pDmaParams->pPageRoot->bufferLock, flags);

    dmaStatus = 0;
    dmaXlnxStreamStop(pDmaEngine);
    if (dmaStatus == 0)
    {
		// deref the resource
		pDmaParams->pPageRoot->engineRef[pDmaEngine->engIndex]--;
        spin_unlock_irqrestore(&pDmaParams->pPageRoot->bufferLock, flags);
        NTV2_MSG_STATE("%s%d:%s%d:%s%d: dmaStreamStop dma state idle\n", DMA_MSG_CONTEXT);
        pDmaEngine->state = DmaStateIdle;
    }
    else
    {
        spin_unlock_irqrestore(&pDmaParams->pPageRoot->bufferLock, flags);
		NTV2_MSG_ERROR("%s%d: dmaStreamStop dma failed to stop\n", DMA_MSG_DEVICE);
    }

    // done
    if (dmaStatus == 0)
    {
        pDmaContext->doVideo = false;
        dmaPageRootFree(deviceNumber, pDmaContext->pVideoPageBuffer);
        dmaContextRelease(pDmaContext);
    }

    return 0;
}

void dmaInterrupt(ULWord deviceNumber, ULWord intStatus)
{
	NTV2PrivateParams* pNTV2Params = getNTV2Params(deviceNumber);
	PDMA_ENGINE pDmaEngine = NULL;
    struct ntv2_stream* pDmaStream = NULL;

	if (pNTV2Params->_dmaMethod == DmaMethodAja)
	{
		if (intStatus & kIntDMA1)
		{
			pDmaEngine = getDmaEngine(deviceNumber, 0);
			dmaAjaInterrupt(pDmaEngine);
		}
		if (intStatus & kIntDMA2)
		{
			pDmaEngine = getDmaEngine(deviceNumber, 1);
			dmaAjaInterrupt(pDmaEngine);
		}
		if (intStatus & kIntDMA3)
		{
			pDmaEngine = getDmaEngine(deviceNumber, 2);
			dmaAjaInterrupt(pDmaEngine);
		}
		if (intStatus & kIntDMA4)
		{
			pDmaEngine = getDmaEngine(deviceNumber, 3);
			dmaAjaInterrupt(pDmaEngine);
		}
	}
	else if (pNTV2Params->_dmaMethod == DmaMethodNwl)
	{
		if (intStatus & kRegMaskNwlCommonS2CInterruptStatus0)
		{
			pDmaEngine = getDmaEngine(deviceNumber, 0);
			dmaNwlInterrupt(pDmaEngine);
		}
		if (intStatus & kRegMaskNwlCommonC2SInterruptStatus0)
		{
			pDmaEngine = getDmaEngine(deviceNumber, 1);
			dmaNwlInterrupt(pDmaEngine);
		}
		if (intStatus & kRegMaskNwlCommonS2CInterruptStatus1)
		{
			pDmaEngine = getDmaEngine(deviceNumber, 2);
			dmaNwlInterrupt(pDmaEngine);
		}
		if (intStatus & kRegMaskNwlCommonC2SInterruptStatus1)
		{
			pDmaEngine = getDmaEngine(deviceNumber, 3);
			dmaNwlInterrupt(pDmaEngine);
		}
	}
	else if (pNTV2Params->_dmaMethod == DmaMethodXlnx)
	{
		if (IsXlnxDmaInterrupt(deviceNumber, false, 0, intStatus))
		{
            pDmaStream = getDmaStream(deviceNumber, 0);
            if (pDmaStream != NULL)
            {
                ntv2_stream_channel_advance(pDmaStream);
            }
            else
            {
                pDmaEngine = getDmaEngine(deviceNumber, 0);
                dmaXlnxInterrupt(pDmaEngine);
            }
		}
		if (IsXlnxDmaInterrupt(deviceNumber, true, 0, intStatus))
		{
            pDmaStream = getDmaStream(deviceNumber, 1);
            if (pDmaStream != NULL)
            {
                ntv2_stream_channel_advance(pDmaStream);
            }
            else
            {
                pDmaEngine = getDmaEngine(deviceNumber, 1);
                dmaXlnxInterrupt(pDmaEngine);
            }
		}
		if (IsXlnxDmaInterrupt(deviceNumber, false, 1, intStatus))
		{
            pDmaStream = getDmaStream(deviceNumber, 2);
            if (pDmaStream != NULL)
            {
                ntv2_stream_channel_advance(pDmaStream);
            }
            else
            {
                pDmaEngine = getDmaEngine(deviceNumber, 2);
                dmaXlnxInterrupt(pDmaEngine);
            }
		}
		if (IsXlnxDmaInterrupt(deviceNumber, true, 1, intStatus))
		{
            pDmaStream = getDmaStream(deviceNumber, 3);
            if (pDmaStream != NULL)
            {
                ntv2_stream_channel_advance(pDmaStream);
            }
            else
            {
                pDmaEngine = getDmaEngine(deviceNumber, 3);
                dmaXlnxInterrupt(pDmaEngine);
            }
		}
	}
	else
	{
		NTV2_MSG_INFO("%s%d: dmaInterrupt bad dma method %d\n",
					  DMA_MSG_DEVICE, pNTV2Params->_dmaMethod);
	}
}

static void dmaEngineLock(PDMA_ENGINE pDmaEngine)
{
	spin_lock_irqsave(&pDmaEngine->engineLock, pDmaEngine->engineFlags);
}

static void dmaEngineUnlock(PDMA_ENGINE pDmaEngine)
{
	spin_unlock_irqrestore(&pDmaEngine->engineLock, pDmaEngine->engineFlags);
}

static PDMA_CONTEXT dmaContextAcquire(PDMA_ENGINE pDmaEngine, ULWord timeout)
{
	PDMA_CONTEXT pDmaContext = NULL;
	int status;
	int iCon;

	// wait for context available
	status = down_timeout(&pDmaEngine->contextSemaphore, timeout);
	if (status != 0)
		return NULL;

	// acquire context
	dmaEngineLock(pDmaEngine);
	for (iCon = 0; iCon < DMA_NUM_CONTEXTS; iCon++)
	{
		pDmaContext = getDmaContext(pDmaEngine->deviceNumber, pDmaEngine->engIndex, iCon);
		if (!pDmaContext->inUse)
		{
			pDmaContext->inUse = true;
			break;
		}
	}
	dmaEngineUnlock(pDmaEngine);

	return pDmaContext;
}

static void dmaContextRelease(PDMA_CONTEXT pDmaContext)
{
	PDMA_ENGINE pDmaEngine = getDmaEngine(pDmaContext->deviceNumber, pDmaContext->engIndex);

	// release context
	dmaEngineLock(pDmaEngine);
	pDmaContext->inUse = false;
	dmaEngineUnlock(pDmaEngine);
	up(&pDmaEngine->contextSemaphore);
}

static int dmaHardwareAcquire(PDMA_ENGINE pDmaEngine, ULWord timeout)
{
	int status;

	// wait for hardware available
	status = down_timeout(&pDmaEngine->transferSemaphore, timeout);
	if (status != 0)
		return -ETIME;

	return 0;
}

static void dmaHardwareRelease(PDMA_ENGINE pDmaEngine)
{
	// release hardware
	up(&pDmaEngine->transferSemaphore);
}

static int dmaSerialAcquire(ULWord deviceNumber, ULWord timeout)
{
	NTV2PrivateParams *pNTV2Params = getNTV2Params(deviceNumber);
	int status;

	// wait for serial available
	status = down_timeout(&pNTV2Params->_dmaSerialSemaphore, timeout);
	if (status != 0)
		return -ETIME;

	return 0;
}

static void dmaSerialRelease(ULWord deviceNumber)
{
	NTV2PrivateParams *pNTV2Params = getNTV2Params(deviceNumber);

	// release hardware
	up(&pNTV2Params->_dmaSerialSemaphore);
}

int dmaPageRootInit(ULWord deviceNumber, PDMA_PAGE_ROOT pRoot)
{
	if (pRoot == NULL)
		return -EINVAL;
	
	memset(pRoot, 0, sizeof(DMA_PAGE_ROOT));
	INIT_LIST_HEAD(&pRoot->bufferHead);
	spin_lock_init(&pRoot->bufferLock);

	return 0;
}

void dmaPageRootRelease(ULWord deviceNumber, PDMA_PAGE_ROOT pRoot)
{
	PDMA_PAGE_BUFFER pBuffer = NULL;
	PDMA_PAGE_BUFFER pBufferLast = NULL;
	DMA_PARAMS dmaParams;
	unsigned long flags;
	LWord refCount;
	int timeout = DMA_TRANSFER_TIMEOUT / 20000;
	int out = 0;
    int i;

	if (pRoot == NULL)
		return;

	NTV2_MSG_PAGE_MAP("%s%d: dmaPageRootRelease  release %lld bytes\n",
					  DMA_MSG_DEVICE, pRoot->lockTotalSize);

	// stop all serial dma
	for (i = 0; i < DMA_NUM_ENGINES; i++)
	{
		if (pRoot->engineRef[i] > 0)
		{
			memset(&dmaParams, 0, sizeof(DMA_PARAMS));
            dmaParams.deviceNumber = deviceNumber;
			dmaParams.pPageRoot = pRoot;
			dmaParams.toHost = getDmaEngine(deviceNumber, i)->dmaC2H;
			dmaParams.dmaEngine = getDmaEngine(deviceNumber, i)->engIndex;
			dmaStreamStop(&dmaParams);
		}
	}

	// remove all locks
	spin_lock_irqsave(&pRoot->bufferLock, flags);
	while(!list_empty(&pRoot->bufferHead))
	{
		// get current ref count
		pBuffer = list_first_entry(&pRoot->bufferHead, DMA_PAGE_BUFFER, bufferEntry);
		if (pBuffer != pBufferLast)
		{
			pBufferLast = pBuffer;
			out = 0;
		}
		refCount = pBuffer->refCount;
		if (refCount <= 1)
		{
			// remove buffer from list
			pBuffer->refCount--;
			list_del_init(&pBuffer->bufferEntry);
			spin_unlock_irqrestore(&pRoot->bufferLock, flags);
			dmaPageBufferRelease(deviceNumber, pBuffer);
			kfree(pBuffer);
		}
		else
		{
			// wait for buffer not used
			spin_unlock_irqrestore(&pRoot->bufferLock, flags);
			out++;
			if (out >= timeout)
			{
				NTV2_MSG_ERROR("%s%d: dmaPageRootRelease  timeout waiting for %d buffer reference(s)\n",
							   DMA_MSG_DEVICE, (refCount - 1));
				out = 0;
			}
			msleep(5);
		}
		spin_lock_irqsave(&pRoot->bufferLock, flags);
	}

	pRoot->lockCounter = 0;
	pRoot->lockTotalSize = 0;
	spin_unlock_irqrestore(&pRoot->bufferLock, flags);

	return;
}

int dmaPageRootAdd(ULWord deviceNumber, PDMA_PAGE_ROOT pRoot,
				   PVOID pAddress, ULWord size, bool rdma, bool map)
{
	PDMA_PAGE_BUFFER pBuffer;
	unsigned long flags;
	int ret;
	
	if ((pRoot == NULL) || (pAddress == NULL) || (size == 0))
		return -EINVAL;

	NTV2_MSG_PAGE_MAP("%s%d: dmaPageRootAdd  addr %016llx  size %d  rdma %d  map %d\n",
					  DMA_MSG_DEVICE, (ULWord64)pAddress, size, rdma, map);

	// use current buffer if found
	pBuffer = dmaPageRootFind(deviceNumber, pRoot, pAddress, size);
	if (pBuffer != NULL)
	{
		dmaPageRootFree(deviceNumber, pBuffer);
		return 0;
	}

	// allocate and initialize new page buffer
	pBuffer = (PDMA_PAGE_BUFFER)kmalloc(sizeof(DMA_PAGE_BUFFER), GFP_ATOMIC);
	if (pBuffer == NULL)
	{
		NTV2_MSG_ERROR("%s%d: dmaPageRootAdd allocate page buffer object failed\n",
					   DMA_MSG_DEVICE);
		return -ENOMEM;
	}

	ret = dmaPageBufferInit(deviceNumber, pBuffer, (size / PAGE_SIZE + 2), rdma);
	if (ret < 0)
	{
		kfree(pBuffer);
		return ret;
	}
	
	// lock buffer 
	ret = dmaPageLock(deviceNumber, pBuffer, pAddress, size, DMA_BIDIRECTIONAL);
	if (ret < 0)
	{
		kfree(pBuffer);
		return ret;
	}

	// map buffer
	if (map)
	{
		ret = dmaSgMap(deviceNumber, pBuffer);
		if (ret < 0)
		{
			dmaPageUnlock(deviceNumber, pBuffer);
			kfree(pBuffer);
			return ret;
		}
		dmaSgHost(deviceNumber, pBuffer);
	}
	
	spin_lock_irqsave(&pRoot->bufferLock, flags);
    pBuffer->pPageRoot = pRoot;
	pBuffer->refCount = 1;
	pBuffer->lockCount = pRoot->lockCounter++;
	pBuffer->lockSize = pBuffer->numPages * PAGE_SIZE;
	pRoot->lockTotalSize += pBuffer->lockSize;
	list_add_tail(&pBuffer->bufferEntry, &pRoot->bufferHead);
	spin_unlock_irqrestore(&pRoot->bufferLock, flags);

	NTV2_MSG_PAGE_MAP("%s%d: dmaPageRootAdd  cnt %lld  addr %016llx  size %lld\n",
					  DMA_MSG_DEVICE, pBuffer->lockCount, (ULWord64)pBuffer->pUserAddress, pBuffer->lockSize);

	return 0;
}

int dmaPageRootRemove(ULWord deviceNumber, PDMA_PAGE_ROOT pRoot,
					  PVOID pAddress, ULWord size)
{
	PDMA_PAGE_BUFFER pBuffer;
	unsigned long flags;

	if ((pRoot == NULL) || (pAddress == NULL) || (size == 0))
		return -EINVAL;

	NTV2_MSG_PAGE_MAP("%s%d: dmaPageRootRemove  addr %016llx  size %d\n",
					  DMA_MSG_DEVICE, (ULWord64)pAddress, size);
	
	// look for buffer
	spin_lock_irqsave(&pRoot->bufferLock, flags);
	list_for_each_entry(pBuffer, &pRoot->bufferHead, bufferEntry)
	{
		if ((pBuffer->refCount > 1) ||
			(pAddress != pBuffer->pUserAddress) ||
			(size != pBuffer->userSize)) continue;

		// remove buffer from list
		pBuffer->refCount--;
		pRoot->lockTotalSize -= pBuffer->lockSize;
		list_del_init(&pBuffer->bufferEntry);
		spin_unlock_irqrestore(&pRoot->bufferLock, flags);

		NTV2_MSG_PAGE_MAP("%s%d: dmaPageRootRemove  cnt %lld  addr %016llx  size %lld\n",
						  DMA_MSG_DEVICE, pBuffer->lockCount, (ULWord64)pBuffer->pUserAddress, pBuffer->lockSize);

		dmaPageBufferRelease(deviceNumber, pBuffer);
		kfree(pBuffer);

		return 0;
	}

	// buffer not found
	spin_unlock_irqrestore(&pRoot->bufferLock, flags);

	NTV2_MSG_PAGE_MAP("%s%d: dmaPageRootRemove  addr %016llx  size %d  busy or not found\n",
					  DMA_MSG_DEVICE, (ULWord64)pAddress, size);
	return -ENOMEM;
}

int dmaPageRootPrune(ULWord deviceNumber, PDMA_PAGE_ROOT pRoot, ULWord size)
{
	PDMA_PAGE_BUFFER pBuffer;
	PDMA_PAGE_BUFFER pPrune;
	unsigned long flags;
	LWord64 lockCount;

	if (pRoot == NULL)
		return -EINVAL;

	NTV2_MSG_PAGE_MAP("%s%d: dmaPageRootPrune  size %d  cur %lld  max %lld\n",
					  DMA_MSG_DEVICE, size, pRoot->lockTotalSize, pRoot->lockMaxSize);

	if (size > pRoot->lockMaxSize)
		size = pRoot->lockMaxSize;

	spin_lock_irqsave(&pRoot->bufferLock, flags);
	while ((pRoot->lockTotalSize + size) > pRoot->lockMaxSize)
	{
		// look for buffer with oldest lock count
		pBuffer = NULL;
		lockCount = 0x7fffffffffffffff;
		list_for_each_entry(pPrune, &pRoot->bufferHead, bufferEntry)
		{
			if (pPrune->refCount > 1) continue;
			if (pPrune->lockCount < lockCount)
			{
				pBuffer = pPrune;
				lockCount = pPrune->lockCount;
			}
		}

		// no buffers available
		if (pBuffer == NULL)
		{
			spin_unlock_irqrestore(&pRoot->bufferLock, flags);
			NTV2_MSG_PAGE_MAP("%s%d: dmaPageRootPrune failed\n", DMA_MSG_DEVICE);
			return -ENOMEM;
		}
		
		// remove buffer from list
		pBuffer->refCount--;
		pRoot->lockTotalSize -= pBuffer->lockSize;
		list_del_init(&pBuffer->bufferEntry);
		spin_unlock_irqrestore(&pRoot->bufferLock, flags);

		NTV2_MSG_PAGE_MAP("%s%d: dmaPageRootPrune  cnt %lld  addr %016llx  size %lld \n",
						  DMA_MSG_DEVICE, pBuffer->lockCount,  (ULWord64)pBuffer->pUserAddress, pBuffer->lockSize);

		dmaPageBufferRelease(deviceNumber, pBuffer);
		kfree(pBuffer);
		spin_lock_irqsave(&pRoot->bufferLock, flags);
	}
	spin_unlock_irqrestore(&pRoot->bufferLock, flags);
	
	return 0;
}

void dmaPageRootAuto(ULWord deviceNumber, PDMA_PAGE_ROOT pRoot,
					 bool lockAuto, bool lockMap, ULWord64 maxSize)
{
	unsigned long flags;

	if (pRoot == NULL)
		return;

	NTV2_MSG_PAGE_MAP("%s%d: dmaPageRootAuto  auto %d  map %d  size %lld\n",
					  DMA_MSG_DEVICE, lockAuto, lockMap, maxSize);

	spin_lock_irqsave(&pRoot->bufferLock, flags);
	pRoot->lockAuto = lockAuto;
	pRoot->lockMap = lockMap;
	pRoot->lockMaxSize = maxSize;
	spin_unlock_irqrestore(&pRoot->bufferLock, flags);
}

static inline bool dmaPageRootAutoLock(PDMA_PAGE_ROOT pRoot)
{
	return pRoot->lockAuto;
}

static inline bool dmaPageRootAutoMap(PDMA_PAGE_ROOT pRoot)
{
	return pRoot->lockMap;
}

PDMA_PAGE_BUFFER dmaPageRootFind(ULWord deviceNumber, PDMA_PAGE_ROOT pRoot,
								 PVOID pAddress, ULWord size)
{
	PDMA_PAGE_BUFFER pBuffer;
	unsigned long flags;

	if ((pRoot == NULL) || (pAddress == NULL) || (size == 0))
		return NULL;

	// look for buffer
	spin_lock_irqsave(&pRoot->bufferLock, flags);
	list_for_each_entry(pBuffer, &pRoot->bufferHead, bufferEntry)
	{
		if ((pBuffer->refCount <= 0) || !pBuffer->pageLock ||
			(pAddress != pBuffer->pUserAddress) ||
			(size > pBuffer->userSize)) continue;

		// found buffer
		pBuffer->refCount++;
		pBuffer->lockCount = pRoot->lockCounter++; 
		spin_unlock_irqrestore(&pRoot->bufferLock, flags);

		NTV2_MSG_PAGE_MAP("%s%d: dmaPageRootFind  addr %016llx  size %d  found  addr %016llx  size %d\n",
						  DMA_MSG_DEVICE, (ULWord64)pAddress, size,
						  (ULWord64)pBuffer->pUserAddress, pBuffer->userSize);
		return pBuffer;
	}
	spin_unlock_irqrestore(&pRoot->bufferLock, flags);

	NTV2_MSG_PAGE_MAP("%s%d: dmaPageRootFind  addr %016llx  size %d  not found\n",
					  DMA_MSG_DEVICE, (ULWord64)pAddress, size);
	return NULL;
}

void dmaPageRootFree(ULWord deviceNumber, PDMA_PAGE_BUFFER pBuffer)
{
	unsigned long flags;
    PDMA_PAGE_ROOT pRoot;
    
	if (pBuffer == NULL)
		return;

    pRoot = pBuffer->pPageRoot;
	if (pRoot == NULL)
		return;

	NTV2_MSG_PAGE_MAP("%s%d: dmaPageRootFree  addr %016llx  size %d\n",
					  DMA_MSG_DEVICE, (ULWord64)pBuffer->pUserAddress, pBuffer->userSize);

	// decrement reference
	spin_lock_irqsave(&pRoot->bufferLock, flags);
	pBuffer->refCount--;
	spin_unlock_irqrestore(&pRoot->bufferLock, flags);
}

static int dmaPageBufferInit(ULWord deviceNumber, PDMA_PAGE_BUFFER pBuffer,
							 ULWord numPages, bool rdma)
{
	if ((pBuffer == NULL) || (numPages == 0))
		return -EINVAL;
	
	memset(pBuffer, 0, sizeof(DMA_PAGE_BUFFER));
	INIT_LIST_HEAD(&pBuffer->bufferEntry);

#ifdef AJA_RDMA
	if (rdma)
	{
		pBuffer->rdma = true;
		return 0;
	}
#endif
	if (rdma)
	{
		NTV2_MSG_ERROR("%s%d: dmaPageLock driver does not support rdma\n", DMA_MSG_DEVICE); 
		return -EINVAL;
	}
	
	// alloc page list
	pBuffer->pPageList = kmalloc(numPages * sizeof(struct page*), GFP_KERNEL);
	if (pBuffer->pPageList == NULL)
	{
		NTV2_MSG_ERROR("%s%d: dmaPageBufferInit allocate page buffer failed  numPages %d\n",
					   DMA_MSG_DEVICE, numPages);
		return -ENOMEM;
	}
	memset(pBuffer->pPageList, 0, sizeof(struct page *) * numPages);
	pBuffer->pageListSize = numPages;
	
	// alloc scatter list
	pBuffer->pSgList = vmalloc(numPages * sizeof(struct scatterlist));
	if (pBuffer->pSgList == NULL)
	{
		NTV2_MSG_ERROR("%s%d: dmaPageBufferInit allocate scatter buffer failed  numPages %d\n",
					   DMA_MSG_DEVICE, numPages);
		return -ENOMEM;
	}
	pBuffer->sgListSize = numPages;
	
	return 0;
}

static void dmaPageBufferRelease(ULWord deviceNumber, PDMA_PAGE_BUFFER pBuffer)
{
	if (pBuffer == NULL)
		return;

	dmaPageUnlock(deviceNumber, pBuffer);

	if (pBuffer->pSgList != 0)
		vfree(pBuffer->pSgList);
	pBuffer->pSgList = NULL;
 	pBuffer->sgListSize = 0;

	if (pBuffer->pPageList != NULL)
		kfree(pBuffer->pPageList);
	pBuffer->pPageList = NULL;
	pBuffer->pageListSize = 0;
}

static int dmaPageLock(ULWord deviceNumber, PDMA_PAGE_BUFFER pBuffer,
					   PVOID pAddress, ULWord size, ULWord direction)
{
	unsigned long address = (unsigned long)pAddress;
	bool write;
	int numPages;
	int numPinned;
	int pageOffset;
	int count;
	int i;

	if ((pBuffer == NULL) || (pAddress == NULL) || (size == 0) || pBuffer->busMap)
		return -EINVAL;

	if (pBuffer->pageLock)
	{
		if ((pAddress == pBuffer->pUserAddress) &&
			(size == pBuffer->userSize) &&
			(direction == pBuffer->direction))
			return 0;
		dmaPageBufferRelease(deviceNumber, pBuffer);
	}

#ifdef AJA_RDMA
	if (pBuffer->rdma)
	{
		ULWord64 rdmaAddress = address & GPU_PAGE_MASK;
		ULWord64 rdmaOffset = address & GPU_PAGE_OFFSET;
		ULWord64 rdmaLen = size;
#ifdef AJA_IGPU		
		ULWord64 rdmaAlignedLen = (rdmaOffset + rdmaLen + GPU_PAGE_SIZE - 1) & GPU_PAGE_MASK;
#else
		ULWord64 rdmaAlignedLen = address + size - rdmaAddress;
#endif		
		struct nvidia_p2p_page_table* rdmaPage = NULL;
		int ret;

		ret = nvidia_p2p_get_pages(
#ifndef AJA_IGPU				
			0, 0,
#endif				
			rdmaAddress,
			rdmaAlignedLen,
			&rdmaPage,
			rdmaFreeCallback,
			pBuffer);
		if (ret < 0)
		{
			NTV2_MSG_ERROR("%s%d: dmaPageLock rdma lock failed %d  addr %016llx  len %08llx\n",
						   DMA_MSG_DEVICE, ret, rdmaAddress, rdmaAlignedLen);
			return ret;
		}

		pBuffer->pUserAddress = pAddress;
		pBuffer->userSize = size;
		pBuffer->direction = direction;
		pBuffer->rdmaAddress = rdmaAddress;
		pBuffer->rdmaOffset = rdmaOffset;
		pBuffer->rdmaLen = rdmaLen;
		pBuffer->rdmaAlignedLen = rdmaAlignedLen;
		pBuffer->rdmaPage = rdmaPage;
		pBuffer->numPages = rdmaPage->entries;
		pBuffer->pageLock = true;
		
		NTV2_MSG_PAGE_MAP("%s%d: dmaPageLock rdma locked %d pages\n", DMA_MSG_DEVICE, pBuffer->numPages);		
		return 0;
	}
#endif

	if (pBuffer->rdma || (pBuffer->pPageList == NULL) || (pBuffer->pSgList == NULL))
		return -EINVAL;

	// clear page list
	memset(pBuffer->pPageList, 0, sizeof(struct page*) * pBuffer->pageListSize);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4,0,0))
	flush_write_buffers();
#endif	

	// compute number of pages to lock
	numPages = (int)(((address & ~PAGE_MASK) + size + ~PAGE_MASK) >> PAGE_SHIFT);

	// test number of pages
	if (numPages > (int)pBuffer->pageListSize) {
		NTV2_MSG_ERROR("%s%d: dmaPageLock page list too small %d  need %d pages\n",
					   DMA_MSG_DEVICE, pBuffer->pageListSize, numPages); 
		return -ENOMEM;
	}
	if (numPages > pBuffer->sgListSize)
	{
		NTV2_MSG_ERROR("%s%d: dmaPageLock scatter list too small %d  needed %d pages\n",
					   DMA_MSG_DEVICE, pBuffer->sgListSize, numPages);
		return -ENOMEM;
	}

	// determine if buffer will be written
	write = (direction == DMA_BIDIRECTIONAL) || (direction == DMA_FROM_DEVICE);

	// get the map semaphore
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,8,0))
	mmap_read_lock(current->mm);
#else	
	down_read(&current->mm->mmap_sem);
#endif	
	// page in and lock the user buffer
#if defined(KERNEL_6_5_0_GET_USER_PAGES)
	numPinned = get_user_pages(address, numPages, write ? FOLL_WRITE : 0, pBuffer->pPageList);
#else
	numPinned = get_user_pages(
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4,6,0))
		current,
		current->mm,
#endif
		address,
		numPages,
#if ((LINUX_VERSION_CODE < KERNEL_VERSION(4,9,0)) && \
	 ((LINUX_VERSION_CODE < KERNEL_VERSION(4,4,168)) ||	\
	  (LINUX_VERSION_CODE >= KERNEL_VERSION(4,5,0))))
		write,
		0,
#else
		write? FOLL_WRITE : 0,
#endif
		pBuffer->pPageList,
		NULL);
#endif
	
	// release the map semaphore
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,8,0))
	mmap_read_unlock(current->mm);
#else	
	up_read(&current->mm->mmap_sem);
#endif	

	// check that all pages are mapped
	if (numPinned != numPages)
	{
		NTV2_MSG_ERROR("%s%d: dmaPageLock get_user_pages failed request %d  pinned %d pages\n",
					   DMA_MSG_DEVICE, numPages, numPinned); 
		goto out_unmap;
	}

	// clear segment list
	NTV2_LINUX_SG_INIT_TABLE_FUNC(pBuffer->pSgList, pBuffer->sgListSize);

	// offset on first page
	pageOffset = (int)(address & ~PAGE_MASK);

	// build scatter list
	count = size;
	if (numPages > 1)
	{
		sg_set_page(&pBuffer->pSgList[0], pBuffer->pPageList[0], PAGE_SIZE - pageOffset, pageOffset);
		count -= sg_dma_len(&pBuffer->pSgList[0]);

		for (i = 1; i < numPages; i++)
		{
			sg_set_page(&pBuffer->pSgList[i], pBuffer->pPageList[i], count < PAGE_SIZE ? count : PAGE_SIZE, 0);
			count -= PAGE_SIZE;
		}
	}
	else
	{
		sg_set_page(&pBuffer->pSgList[0], pBuffer->pPageList[0], count, pageOffset);
	}

	// save parameters
	pBuffer->pUserAddress = pAddress;
	pBuffer->userSize = size;
	pBuffer->direction = direction;
	pBuffer->numPages = numPages;
	pBuffer->numSgs = numPages;
	pBuffer->pageLock = true;

	NTV2_MSG_PAGE_MAP("%s%d: dmaPageLock lock %d pages\n", DMA_MSG_DEVICE, numPages);
	
	return 0;

out_unmap:
	for (i = 0; i < numPinned; i++)
	{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,5,0))
		put_page(pBuffer->pPageList[i]);
#else
		page_cache_release(pBuffer->pPageList[i]);
#endif
	}
	
	return -EPERM;
}

static void dmaPageUnlock(ULWord deviceNumber, PDMA_PAGE_BUFFER pBuffer)
{
	int i;

	dmaSgUnmap(deviceNumber, pBuffer);
	
	if (pBuffer == NULL)
		return;

	if (pBuffer->pageLock)
	{
#ifdef AJA_RDMA
		if (pBuffer->rdma)
		{
			if ((pBuffer->rdmaAddress == 0) || (pBuffer->rdmaPage == NULL))
				return;
			
			NTV2_MSG_PAGE_MAP("%s%d: dmaPageUnlock rdma unlock %d pages\n", 
							  DMA_MSG_DEVICE, pBuffer->numPages); 

			nvidia_p2p_put_pages(
#ifndef AJA_IGPU				
				0, 0,
				pBuffer->rdmaAddress,
#endif								
				pBuffer->rdmaPage);
			rdmaFreeCallback(pBuffer);			
			return;
		}
#endif
		if (pBuffer->rdma || (pBuffer->pPageList == NULL))
			return;

		NTV2_MSG_PAGE_MAP("%s%d: dmaPageUnlock unlock %d pages\n", 
						  DMA_MSG_DEVICE, pBuffer->numPages); 

		// release the locked pages
		for (i = 0; i < pBuffer->numPages; i++)
		{
			if ((pBuffer->direction == DMA_FROM_DEVICE) &&
				!PageReserved(pBuffer->pPageList[i]))
			{
				set_page_dirty(pBuffer->pPageList[i]);
			}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,5,0))
			put_page(pBuffer->pPageList[i]);
#else
			page_cache_release(pBuffer->pPageList[i]);
#endif
		}
	}
		
	// clear parameters
	pBuffer->pUserAddress = NULL;
	pBuffer->userSize = 0;
	pBuffer->direction = 0;
	pBuffer->pageLock = false;
	pBuffer->busMap = false;
	pBuffer->numPages = 0;
	pBuffer->numSgs = 0;
}

#ifdef AJA_RDMA
static void rdmaFreeCallback(void* data)
{
	PDMA_PAGE_BUFFER pBuffer = (PDMA_PAGE_BUFFER)data;
	struct nvidia_p2p_page_table* rdmaPage;

//	ntv2Message("rdmaFreeCallback %llx\n", (long long)data);

	rdmaPage = xchg(&pBuffer->rdmaPage, NULL);
	if (rdmaPage != NULL)
		nvidia_p2p_free_page_table(rdmaPage);

	pBuffer->rdmaAddress = 0;
	pBuffer->rdmaOffset = 0;
	pBuffer->rdmaLen = 0;
	pBuffer->rdmaAlignedLen = 0;
	pBuffer->rdmaPage = NULL;
	pBuffer->pageLock = false;
}

static void dmaSgSetRdmaPage(struct scatterlist* pSg, struct nvidia_p2p_dma_mapping	*rdmaMap,
							 int index, ULWord64 length, ULWord64 offset)
{
	if ((pSg == NULL) || (rdmaMap == NULL) || (index >= rdmaMap->entries))
		return;

	pSg->offset = (unsigned int)offset;
#ifdef AJA_IGPU
	(void)length;
	pSg->dma_address = (dma_addr_t)rdmaMap->hw_address[index];
	pSg->length = (unsigned int)rdmaMap->hw_len[index];
#else
	pSg->dma_address = (dma_addr_t)rdmaMap->dma_addresses[index];
	pSg->length = (unsigned int)length;
#endif	
#ifdef CONFIG_NEED_SG_DMA_LENGTH
	pSg->dma_length = pSg->length;
#endif
}
#endif

static int dmaBusMap(ULWord deviceNumber, PDMA_PAGE_BUFFER pBuffer,
					 ULWord64 videoBusAddress, ULWord videoBusSize)
{
	ULWord numSgs = 0;
	
	if ((pBuffer == NULL) || (pBuffer->pSgList == NULL) || pBuffer->pageLock)
		return -EINVAL;

	if ((videoBusAddress == 0) || (videoBusSize == 0))
	{
		NTV2_MSG_ERROR("%s%d: dmaBusMap nothing to map\n",
					   DMA_MSG_DEVICE);
		return -EPERM;
	}

	// clear segment list
	NTV2_LINUX_SG_INIT_TABLE_FUNC(pBuffer->pSgList, pBuffer->sgListSize);

	// make bus target segment
	pBuffer->pSgList[numSgs].page_link = 0;
	pBuffer->pSgList[numSgs].offset = 0;
	pBuffer->pSgList[numSgs].length = videoBusSize;
	pBuffer->pSgList[numSgs].dma_address = videoBusAddress;
#ifdef CONFIG_NEED_SG_DMA_LENGTH
	pBuffer->pSgList[numSgs].dma_length = videoBusSize;
#endif

	pBuffer->numSgs = 1;
	pBuffer->busMap = true;

	NTV2_MSG_PAGE_MAP("%s%d: dmaBusMap map %d segment(s)\n",
					  DMA_MSG_DEVICE, pBuffer->numSgs);
	
	return 0;
}

static int dmaSgMap(ULWord deviceNumber, PDMA_PAGE_BUFFER pBuffer)
{
	NTV2PrivateParams *pNTV2Params = getNTV2Params(deviceNumber);
	int count;
	
	if (pBuffer == NULL)
		return -EINVAL;

	if (pBuffer->pageLock && !pBuffer->sgMap)
	{
#ifdef AJA_RDMA
		if (pBuffer->rdma)
		{
			ULWord numEntries;
			ULWord64 pageOffset;
			ULWord64 count;
			int i;
			int ret;
#ifdef AJA_IGPU
			ret = nvidia_p2p_dma_map_pages(&(pNTV2Params->pci_dev)->dev,
										   pBuffer->rdmaPage,
										   &pBuffer->rdmaMap,
										   (pBuffer->direction == PCI_DMA_TODEVICE)? DMA_TO_DEVICE : DMA_FROM_DEVICE);
#else
			ret = nvidia_p2p_dma_map_pages(pNTV2Params->pci_dev,
										   pBuffer->rdmaPage,
										   &pBuffer->rdmaMap);
#endif
			if (ret < 0)
			{
				NTV2_MSG_ERROR("%s%d: dmaSgMap rdma map failed %d\n",
							   DMA_MSG_DEVICE, ret); 
				return ret;
			}

			if ((pBuffer->rdmaMap == NULL) || (pBuffer->rdmaMap->entries == 0))
			{
				NTV2_MSG_ERROR("%s%d: dmaSgMap rdma map failed - no map\n",
							   DMA_MSG_DEVICE); 
				return -EPERM;
			}
				
			// alloc scatter list
			numEntries = pBuffer->rdmaMap->entries;
			pBuffer->pSgList = vmalloc(numEntries * sizeof(struct scatterlist));
			if (pBuffer->pSgList == NULL)
			{
				NTV2_MSG_ERROR("%s%d: dmaSgMap allocate rdma scatter buffer failed - entries %d\n",
							   DMA_MSG_DEVICE, numEntries);
				return -ENOMEM;
			}
			pBuffer->sgListSize = numEntries;

			// clear segment list
			NTV2_LINUX_SG_INIT_TABLE_FUNC(pBuffer->pSgList, pBuffer->sgListSize);

			// offset on first page
			pageOffset = pBuffer->rdmaOffset;

			// build scatter list
			count = pBuffer->rdmaLen;
			for (i = 0; i < numEntries; i++)
			{
				if (count > 0)
				{
					dmaSgSetRdmaPage(&pBuffer->pSgList[i],
									 pBuffer->rdmaMap,
									 i,
									 count < (GPU_PAGE_SIZE - pageOffset)? count : (GPU_PAGE_SIZE - pageOffset),
									 pageOffset);
				}
				count = (count < GPU_PAGE_SIZE)? 0 : (count - GPU_PAGE_SIZE);
				pageOffset = 0;
			}

			NTV2_MSG_PAGE_MAP("%s%d: dmaSgMap rdma mapped %d segment(s)\n", 
							  DMA_MSG_DEVICE, numEntries);

			pBuffer->numSgs = (ULWord)numEntries;
			pBuffer->sgMap = true;
			pBuffer->sgHost = false;
			
			return 0;
		}
#endif
		if (pBuffer->pSgList == NULL)
			return -EINVAL;
			
		// map scatter list
		count = dma_map_sg(&(pNTV2Params->pci_dev)->dev,
						   pBuffer->pSgList,
						   pBuffer->numSgs,
						   pBuffer->direction);

		if (count == 0)
		{
			NTV2_MSG_ERROR("%s%d: dmaSgMap map %d segment(s) failed\n",
						   DMA_MSG_DEVICE, pBuffer->numSgs); 
			return -EPERM;
		}
		pBuffer->sgMap = true;
		pBuffer->sgHost = false;

		NTV2_MSG_PAGE_MAP("%s%d: dmaSgMap mapped %d segment(s)\n", 
					  DMA_MSG_DEVICE, pBuffer->numSgs); 
	}

	return 0;
}

static void dmaSgUnmap(ULWord deviceNumber, PDMA_PAGE_BUFFER pBuffer)
{
	NTV2PrivateParams *pNTV2Params = getNTV2Params(deviceNumber);

	if ((pBuffer == NULL) || (pBuffer->pSgList == NULL))
		return;

	if (pBuffer->sgMap)
	{
#ifdef AJA_RDMA
		if (pBuffer->rdma)
		{
			if ((pBuffer->rdmaPage != NULL) && (pBuffer->rdmaMap != NULL))
			{
				NTV2_MSG_PAGE_MAP("%s%d: dmaSgUnmap rdma unmap %d segments\n", 
								  DMA_MSG_DEVICE, pBuffer->numSgs);
				
#ifdef AJA_IGPU
				nvidia_p2p_dma_unmap_pages(pBuffer->rdmaMap);
#else
				nvidia_p2p_dma_unmap_pages(pNTV2Params->pci_dev,
										   pBuffer->rdmaPage,
										   pBuffer->rdmaMap);
#endif
			}
			if (pBuffer->pSgList != NULL)
				vfree(pBuffer->pSgList);
			pBuffer->pSgList = NULL;
			pBuffer->sgListSize = 0;
			pBuffer->numSgs = 0;
			pBuffer->sgMap = false;
			pBuffer->sgHost = false;
			return;
		}
#endif

		NTV2_MSG_PAGE_MAP("%s%d: dmaSgUnmap unmap %d segments\n", 
						  DMA_MSG_DEVICE, pBuffer->numSgs); 

		// unmap the scatter list
		dma_unmap_sg(&(pNTV2Params->pci_dev)->dev,
					 pBuffer->pSgList,
					 pBuffer->numSgs,
					 pBuffer->direction);
	}

	// clear parameters
	pBuffer->sgMap = false;
	pBuffer->sgHost = false;
}

static void dmaSgDevice(ULWord deviceNumber, PDMA_PAGE_BUFFER pBuffer)
{
	NTV2PrivateParams *pNTV2Params = getNTV2Params(deviceNumber);

	if ((pBuffer == NULL) ||
        (pBuffer->pSgList == NULL) ||
        (pBuffer->rdma))
		return;

	if (pBuffer->sgMap)
	{
		if (pBuffer->sgHost)
		{
			// sync scatter list for device access
			dma_sync_sg_for_device(&(pNTV2Params->pci_dev)->dev,
								   pBuffer->pSgList,
								   pBuffer->numSgs,
								   pBuffer->direction);
			pBuffer->sgHost = false;
	
			NTV2_MSG_PAGE_MAP("%s%d: dmaSgDevice sync %d pages\n",
							  DMA_MSG_DEVICE, pBuffer->numSgs);
		}
	}
}

static void dmaSgHost(ULWord deviceNumber, PDMA_PAGE_BUFFER pBuffer)
{
	NTV2PrivateParams *pNTV2Params = getNTV2Params(deviceNumber);

	if ((pBuffer == NULL) ||
        (pBuffer->pSgList == NULL) ||
        (pBuffer->rdma))
		return;

	if (pBuffer->sgMap)
	{
		if (!pBuffer->sgHost)
		{
			// sync scatter list for cpu access
			dma_sync_sg_for_cpu(&(pNTV2Params->pci_dev)->dev,
								pBuffer->pSgList,
								pBuffer->numSgs,
								pBuffer->direction);
			pBuffer->sgHost = true;
	
			NTV2_MSG_PAGE_MAP("%s%d: dmaSgHost sync %d pages\n",
							  DMA_MSG_DEVICE, pBuffer->numSgs);
		}
	}
}

static inline bool dmaPageLocked(PDMA_PAGE_BUFFER pBuffer)
{
	return pBuffer->pageLock;
}

static inline bool dmaSgMapped(PDMA_PAGE_BUFFER pBuffer)
{
	return pBuffer->sgMap;
}

static inline ULWord dmaSgSize(PDMA_PAGE_BUFFER pBuffer)
{
	return pBuffer->numSgs;
}

static inline ULWord64 dmaSgAddress(PDMA_PAGE_BUFFER pBuffer, ULWord index)
{
	return (ULWord64)sg_dma_address(pBuffer->pSgList + index);
}

static inline ULWord dmaSgLength(PDMA_PAGE_BUFFER pBuffer, ULWord index)
{
	return (ULWord)sg_dma_len(pBuffer->pSgList + index);
}

static int dmaAjaProgram(PDMA_CONTEXT pDmaContext)
{
	ULWord 					deviceNumber = pDmaContext->deviceNumber;
	DMA_ENGINE				*pDmaEngine = getDmaEngine(deviceNumber, pDmaContext->engIndex);
	ULWord					ajaIndex = pDmaContext->dmaIndex;
	bool					ajaC2H = pDmaContext->dmaC2H;
	ULWord					valControl;
	PDMA_DESCRIPTOR64		pDescriptor;
	PDMA_DESCRIPTOR64		pDescriptorLast;
	ULWord64				physDescriptor;
	ULWord					descriptorCount;
	ULWord					sgIndex;
	ULWord64				descSystemAddress = 0;
	ULWord					descCardAddress = 0;
	ULWord					descTransferSize = 0;
	ULWord					programBytes;
	ULWord					dpIndex;
	ULWord					dpPageCount;
	ULWord					dpNumPerPage;
	bool					done;

	NTV2_MSG_PROGRAM("%s%d:%s%d:%s%d: dmaAjaProgram program %s %s %s %s\n", 
					 DMA_MSG_CONTEXT, 
					 pDmaContext->doVideo? "video":"",
					 pDmaContext->doAudio? "audio":"",
					 pDmaContext->doAncF1? "ancF1":"",
					 pDmaContext->doAncF2? "ancF2":"");

	// check engine state
	if (pDmaEngine->state != DmaStateTransfer)
	{
		NTV2_MSG_ERROR("%s%d:%s%d:%s%d: dmaAjaProgram dma state %d not transfer\n", 
					   DMA_MSG_CONTEXT, pDmaEngine->state);
		pDmaEngine->programErrorCount++;
		return -EPERM;
	}
	
	// read status register
	valControl = ReadDMAControlStatus(deviceNumber);
	if (valControl == 0xffffffff)
	{
		NTV2_MSG_ERROR("%s%d:%s%d:%s%d: dmaAjaProgram dma hardware not present\n", 
					   DMA_MSG_CONTEXT);
		pDmaEngine->programErrorCount++;
		return -EPERM;
	}
	
	// setup for descriptor loop
	valControl = 0;
	dpIndex = 0;
	dpPageCount = 0;
	dpNumPerPage = PAGE_SIZE / DMA_DESCRIPTOR_SIZE;
	pDescriptor = (PDMA_DESCRIPTOR64)pDmaEngine->pDescriptorVirtual[dpIndex];
	pDescriptorLast = pDescriptor;
	physDescriptor = pDmaEngine->descriptorPhysical[dpIndex];
	descriptorCount = 0;
	programBytes = 0;
	sgIndex = 0;
	done = false;

	// iterate through the mapped descriptor lists
	while (!done)
	{
		// setup descriptor generator
		while (!done)
		{
			if (pDmaContext->doVideo)
			{
				if ((sgIndex < dmaSgSize(pDmaContext->pVideoPageBuffer)) && 
					dmaVideoSegmentTransfer(pDmaContext,
											&pDmaContext->dmaVideoSegment,
											dmaSgAddress(pDmaContext->pVideoPageBuffer, sgIndex),
											dmaSgLength(pDmaContext->pVideoPageBuffer, sgIndex)))
				{
					sgIndex++;
					break;
				}
				pDmaContext->doVideo = false;
				sgIndex = 0;
			}
		
			if (pDmaContext->doAudio)
			{
				if ((sgIndex < dmaSgSize(pDmaContext->pAudioPageBuffer)) && 
					dmaAudioSegmentTransfer(pDmaContext,
											&pDmaContext->dmaAudioSegment,
											dmaSgAddress(pDmaContext->pAudioPageBuffer, sgIndex),
											dmaSgLength(pDmaContext->pAudioPageBuffer, sgIndex)))
				{
					sgIndex++;
					break;
				}
				pDmaContext->doAudio = false;
				sgIndex = 0;
			}
		
			if (pDmaContext->doAncF1)
			{
				if ((sgIndex < dmaSgSize(pDmaContext->pAncF1PageBuffer)) && 
					dmaAncSegmentTransfer(pDmaContext,
										  &pDmaContext->dmaAncF1Segment,
										  dmaSgAddress(pDmaContext->pAncF1PageBuffer, sgIndex),
										  dmaSgLength(pDmaContext->pAncF1PageBuffer, sgIndex)))
				{
					sgIndex++;
					break;
				}
				pDmaContext->doAncF1 = false;
				sgIndex = 0;
			}
		
			if (pDmaContext->doAncF2)
			{
				if ((sgIndex < dmaSgSize(pDmaContext->pAncF2PageBuffer)) && 
					dmaAncSegmentTransfer(pDmaContext,
										  &pDmaContext->dmaAncF2Segment,
										  dmaSgAddress(pDmaContext->pAncF2PageBuffer, sgIndex),
										  dmaSgLength(pDmaContext->pAncF2PageBuffer, sgIndex)))
				{
					sgIndex++;
					break;
				}
				pDmaContext->doAncF2 = false;
				sgIndex = 0;
			}
		
			if (pDmaContext->doMessage)
			{
				if ((sgIndex < 1) &&
					(pDmaContext->messageBusAddress != 0) &&
					(pDmaContext->messageCardAddress != 0))
				{
					sgIndex++;
					break;
				}
				pDmaContext->doMessage = false;
				sgIndex = 0;
			}

			done = true;
		}

		// generate the segment descriptors for each mapped descriptor
		while (!done)
		{
			descSystemAddress = 0;
			descCardAddress = 0;
			descTransferSize = 0;

			// get the next segment descriptor
			if (pDmaContext->doVideo)
			{
				if (!dmaVideoSegmentDescriptor(pDmaContext,
											   &pDmaContext->dmaVideoSegment,
											   &descSystemAddress,
											   &descCardAddress,
											   &descTransferSize))
				{
					break;
				}
			}
			else if (pDmaContext->doAudio)
			{
				if (!dmaAudioSegmentDescriptor(pDmaContext,
											   &pDmaContext->dmaAudioSegment,
											   &descSystemAddress,
											   &descCardAddress,
											   &descTransferSize))
				{
					break;
				}
			}
			else if (pDmaContext->doAncF1)
			{
				if (!dmaAncSegmentDescriptor(pDmaContext,
											 &pDmaContext->dmaAncF1Segment,
											 &descSystemAddress,
											 &descCardAddress,
											 &descTransferSize))
				{
					break;
				}
			}
			else if (pDmaContext->doAncF2)
			{
				if (!dmaAncSegmentDescriptor(pDmaContext,
											 &pDmaContext->dmaAncF2Segment,
											 &descSystemAddress,
											 &descCardAddress,
											 &descTransferSize))
				{
					break;
				}
			}
			else if (pDmaContext->doMessage)
			{
				if (pDmaContext->messageCardAddress == 0)
				{
					break;
				}

				descSystemAddress = pDmaContext->messageBusAddress;
				descCardAddress = pDmaContext->messageCardAddress;
				descTransferSize = 4;

				pDmaContext->messageCardAddress = 0;
			}
			else
			{
				break;
			}

			// write descriptor
			pDescriptor->ulHostAddressLow = (ULWord)descSystemAddress;
			pDescriptor->ulHostAddressHigh = (ULWord)(descSystemAddress>>32);
			pDescriptor->ulLocalAddress = (ULWord)descCardAddress;
			pDescriptor->ulTransferCount = (descTransferSize / DMA_TRANSFERCOUNT_BYTES) | DMA_TRANSFERCOUNT_64;
			if(ajaC2H)
			{
				pDescriptor->ulTransferCount |= DMA_TRANSFERCOUNT_TOHOST;
			}

			// setup for next segment descriptor
			pDescriptorLast = pDescriptor;

			dpPageCount++;
			if (dpPageCount < dpNumPerPage)
			{
				pDescriptor++;
				physDescriptor += sizeof(DMA_DESCRIPTOR64);
			}
			else
			{
				dpPageCount = 0;
				dpIndex++;
				if (dpIndex >= pDmaEngine->numDescriptorPages)
				{
					NTV2_MSG_DESCRIPTOR("%s%d:%s%d:%s%d: dmaAjaProgram too many descriptor pages\n",
										DMA_MSG_CONTEXT);
					return -EPERM;
				}
				pDescriptor = (PDMA_DESCRIPTOR64)pDmaEngine->pDescriptorVirtual[dpIndex];
				physDescriptor = pDmaEngine->descriptorPhysical[dpIndex];
			}
			pDescriptorLast->ulNextAddressLow = (ULWord)physDescriptor;
			pDescriptorLast->ulNextAddressHigh = (ULWord)(physDescriptor>>32);

			NTV2_MSG_DESCRIPTOR("%s%d:%s%d:%s%d: dmaAjaProgram cnt %08x sys %08x:%08x crd %08x nxt %08x:%08x\n",
								DMA_MSG_CONTEXT,
								pDescriptorLast->ulTransferCount, pDescriptorLast->ulHostAddressHigh,
								pDescriptorLast->ulHostAddressLow, pDescriptorLast->ulLocalAddress,
								pDescriptorLast->ulNextAddressHigh, pDescriptorLast->ulNextAddressLow);

			programBytes += descTransferSize;
			descriptorCount++;
			if (descriptorCount > pDmaEngine->maxDescriptors)
			{
				pDmaEngine->programErrorCount++;
				NTV2_MSG_ERROR("%s%d:%s%d:%s%d: dmaAjaProgram exceeded max descriptor count %d\n",
							   DMA_MSG_CONTEXT, descriptorCount);
				return -EPERM;
			}
		}
	}

	// no next descriptor
	pDescriptorLast->ulNextAddressLow = 0;
	pDescriptorLast->ulNextAddressHigh = 0;

	// need at least one descriptor
	if (descriptorCount == 0)
	{
		pDmaEngine->programErrorCount++;
		NTV2_MSG_ERROR("%s%d:%s%d:%s%d: dmaAjaProgram no descriptors generated\n", DMA_MSG_CONTEXT);
		return -EPERM;
	}

	// make sure that engine is not running
	valControl = ReadDMAControlStatus(deviceNumber);
	if((valControl & (0x1 << ajaIndex)) != 0)
	{
		pDmaEngine->programErrorCount++;
		NTV2_MSG_ERROR("%s%d:%s%d:%s%d: dmaAjaProgram dma still running\n", DMA_MSG_CONTEXT);
		dmaAjaAbort(pDmaEngine);
		pDmaEngine->programErrorCount++;
		return -EPERM;
	}

	// load first descriptor
	pDescriptor = (PDMA_DESCRIPTOR64)pDmaEngine->pDescriptorVirtual[0];
	WriteDMAHostAddressLow(deviceNumber, ajaIndex, pDescriptor->ulHostAddressLow);
	WriteDMAHostAddressHigh(deviceNumber, ajaIndex, pDescriptor->ulHostAddressHigh);
	WriteDMALocalAddress(deviceNumber, ajaIndex, pDescriptor->ulLocalAddress);
	WriteDMATransferCount(deviceNumber, ajaIndex, pDescriptor->ulTransferCount);
	WriteDMANextDescriptorLow(deviceNumber, ajaIndex, pDescriptor->ulNextAddressLow);
	WriteDMANextDescriptorHigh(deviceNumber, ajaIndex, pDescriptor->ulNextAddressHigh);

	NTV2_MSG_PROGRAM("%s%d:%s%d: dmaAjaProgram dma engine start  size %d bytes\n",
					 DMA_MSG_ENGINE, programBytes);

	// count the program starts and descriptors
	pDmaEngine->programStartCount++;
	pDmaEngine->programDescriptorCount += descriptorCount;
	pDmaEngine->programBytes += programBytes;
	pDmaEngine->programStartTime = ntv2Time100ns();

	// clear interrupt
	ClearDMAInterrupt(deviceNumber, dmaAjaIntClear[ajaIndex]);

	// start dma
	SetDMAEngineStatus(deviceNumber, ajaIndex, true);

	return 0;
}

static void dmaAjaAbort(PDMA_ENGINE pDmaEngine)
{
	ULWord 		deviceNumber = pDmaEngine->deviceNumber;
	ULWord		ajaIndex = pDmaEngine->dmaIndex;
	ULWord		regValue;

	regValue = ReadDMAControlStatus(deviceNumber);
	NTV2_MSG_PROGRAM("%s%d:%s%d: dmaAjaAbort dma engine abort  control status %08x\n",
					 DMA_MSG_ENGINE, regValue);

	// stop the dma engine
	SetDMAEngineStatus(deviceNumber, ajaIndex, false);
}

static void dmaAjaInterrupt(PDMA_ENGINE pDmaEngine)
{
	ULWord 		deviceNumber = pDmaEngine->deviceNumber;
	ULWord		ajaIndex = pDmaEngine->dmaIndex;
	ULWord		valDmaStatus;

	// clear interrupt
	ClearDMAInterrupt(deviceNumber, dmaAjaIntClear[ajaIndex]);

	// check enabled
	if (!pDmaEngine->dmaEnable)
	{
		pDmaEngine->programErrorCount++;
		NTV2_MSG_ERROR("%s%d:%s%d: dmaAjaInterrupt engine not enabled\n", DMA_MSG_ENGINE);
	}

	// check engine state
	if (pDmaEngine->state != DmaStateTransfer)
	{
		pDmaEngine->programErrorCount++;
		NTV2_MSG_ERROR("%s%d:%s%d: dmaAjaInterrupt dma state not transfer %d\n",
					   DMA_MSG_ENGINE, pDmaEngine->state);
		return;
	}

	pDmaEngine->interruptCount++;

	// save statistics
	pDmaEngine->programStopTime = ntv2Time100ns();
	pDmaEngine->programCompleteCount++;
	pDmaEngine->programTime = pDmaEngine->programStopTime - pDmaEngine->programStartTime;
	pDmaEngine->programBytes = pDmaEngine->programBytes;

	// check for dma still active or hardware error
	valDmaStatus = ReadDMAControlStatus(deviceNumber);
	if (((valDmaStatus & (0x1 << ajaIndex)) != 0) || ((valDmaStatus & 0x80000000) != 0))
	{
		// stop the current transaction
		pDmaEngine->programErrorCount++;
		NTV2_MSG_ERROR("%s%d:%s%d: dmaDmaInterrupt dma hardware not done or error %08x\n",
					   DMA_MSG_ENGINE, valDmaStatus);
		dmaAjaAbort(pDmaEngine);
		return;
	}

	NTV2_MSG_PROGRAM("%s%d:%s%d: dmaDmaInterrupt dma complete  size %lld bytes  time sys %lld us\n",
					 DMA_MSG_ENGINE, pDmaEngine->programBytes, 
					 (pDmaEngine->programStopTime - pDmaEngine->programStartTime)/10);

	set_bit(0, &pDmaEngine->transferDone);
	wake_up(&pDmaEngine->transferEvent);
}

static int dmaNwlProgram(PDMA_CONTEXT pDmaContext)
{
	ULWord 					deviceNumber = pDmaContext->deviceNumber;
	DMA_ENGINE				*pDmaEngine = getDmaEngine(deviceNumber, pDmaContext->engIndex);
	ULWord					nwlIndex = pDmaContext->dmaIndex;
	bool					nwlC2H = pDmaContext->dmaC2H;
	ULWord					valControl;
	PNWL_DESCRIPTOR			pDescriptor;
	PNWL_DESCRIPTOR			pDescriptorLast;
	ULWord64				physDescriptor;
	ULWord					descriptorCount;
	ULWord					sgIndex;
	ULWord64				descSystemAddress;
	ULWord					descCardAddress;
	ULWord					descTransferSize;
	ULWord					programBytes;
	ULWord					dpIndex;
	ULWord					dpPageCount;
	ULWord					dpNumPerPage;
	bool					done;

	NTV2_MSG_PROGRAM("%s%d:%s%d:%s%d: dmaNwlProgram program %s %s %s %s\n", 
					 DMA_MSG_CONTEXT, 
					 pDmaContext->doVideo? "video":"",
					 pDmaContext->doAudio? "audio":"",
					 pDmaContext->doAncF1? "ancF1":"",
					 pDmaContext->doAncF2? "ancF2":"");

	// check engine state
	if (pDmaEngine->state != DmaStateTransfer)
	{
		NTV2_MSG_ERROR("%s%d:%s%d:%s%d: dmaNwlProgram dma state %d not transfer\n", 
					   DMA_MSG_CONTEXT, pDmaEngine->state);
		pDmaEngine->programErrorCount++;
		return -EPERM;
	}
	
	// read status register
	valControl = ReadNwlControlStatus(deviceNumber, nwlC2H, nwlIndex);
	if (valControl == 0xffffffff)
	{
		NTV2_MSG_ERROR("%s%d:%s%d:%s%d: dmaNwlProgram dma hardware not present\n", 
					   DMA_MSG_CONTEXT);
		pDmaEngine->programErrorCount++;
		return -EPERM;
	}
	
	// setup for descriptor loop
	valControl = 0;
	dpIndex = 0;
	dpPageCount = 0;
	dpNumPerPage = PAGE_SIZE / DMA_DESCRIPTOR_SIZE;
	pDescriptor = (PNWL_DESCRIPTOR)pDmaEngine->pDescriptorVirtual[dpIndex];
	pDescriptorLast = pDescriptor;
	physDescriptor = pDmaEngine->descriptorPhysical[dpIndex];
	descriptorCount = 0;
	programBytes = 0;
	sgIndex = 0;
	done = false;

	// iterate through the mapped descriptor lists
	while (!done)
	{
		// setup descriptor generator
		while (!done)
		{
			if (pDmaContext->doVideo)
			{
				if ((sgIndex < dmaSgSize(pDmaContext->pVideoPageBuffer)) && 
					dmaVideoSegmentTransfer(pDmaContext,
											&pDmaContext->dmaVideoSegment,
											dmaSgAddress(pDmaContext->pVideoPageBuffer, sgIndex),
											dmaSgLength(pDmaContext->pVideoPageBuffer, sgIndex)))
				{
					sgIndex++;
					break;
				}
				pDmaContext->doVideo = false;
				sgIndex = 0;
			}
		
			if (pDmaContext->doAudio)
			{
				if ((sgIndex < dmaSgSize(pDmaContext->pAudioPageBuffer)) && 
					dmaAudioSegmentTransfer(pDmaContext,
											&pDmaContext->dmaAudioSegment,
											dmaSgAddress(pDmaContext->pAudioPageBuffer, sgIndex),
											dmaSgLength(pDmaContext->pAudioPageBuffer, sgIndex)))
				{
					sgIndex++;
					break;
				}
				pDmaContext->doAudio = false;
				sgIndex = 0;
			}
		
			if (pDmaContext->doAncF1)
			{
				if ((sgIndex < dmaSgSize(pDmaContext->pAncF1PageBuffer)) && 
					dmaAncSegmentTransfer(pDmaContext,
										  &pDmaContext->dmaAncF1Segment,
										  dmaSgAddress(pDmaContext->pAncF1PageBuffer, sgIndex),
										  dmaSgLength(pDmaContext->pAncF1PageBuffer, sgIndex)))
				{
					sgIndex++;
					break;
				}
				pDmaContext->doAncF1 = false;
				sgIndex = 0;
			}
		
			if (pDmaContext->doAncF2)
			{
				if ((sgIndex < dmaSgSize(pDmaContext->pAncF2PageBuffer)) && 
					dmaAncSegmentTransfer(pDmaContext,
										  &pDmaContext->dmaAncF2Segment,
										  dmaSgAddress(pDmaContext->pAncF2PageBuffer, sgIndex),
										  dmaSgLength(pDmaContext->pAncF2PageBuffer, sgIndex)))
				{
					sgIndex++;
					break;
				}
				pDmaContext->doAncF2 = false;
				sgIndex = 0;
			}
		
			if (pDmaContext->doMessage)
			{
				if ((sgIndex < 1) &&
					(pDmaContext->messageBusAddress != 0) &&
					(pDmaContext->messageCardAddress != 0))
				{
					sgIndex++;
					break;
				}
				pDmaContext->doMessage = false;
				sgIndex = 0;
			}

			done = true;
		}

		// generate the segment descriptors for each mapped descriptor
		while (!done)
		{
			descSystemAddress = 0;
			descCardAddress = 0;
			descTransferSize = 0;

			// get the next segment descriptor
			if (pDmaContext->doVideo)
			{
				if (!dmaVideoSegmentDescriptor(pDmaContext,
											   &pDmaContext->dmaVideoSegment,
											   &descSystemAddress,
											   &descCardAddress,
											   &descTransferSize))
				{
					break;
				}
			}
			else if (pDmaContext->doAudio)
			{
				if (!dmaAudioSegmentDescriptor(pDmaContext,
											   &pDmaContext->dmaAudioSegment,
											   &descSystemAddress,
											   &descCardAddress,
											   &descTransferSize))
				{
					break;
				}
			}
			else if (pDmaContext->doAncF1)
			{
				if (!dmaAncSegmentDescriptor(pDmaContext,
											 &pDmaContext->dmaAncF1Segment,
											 &descSystemAddress,
											 &descCardAddress,
											 &descTransferSize))
				{
					break;
				}
			}
			else if (pDmaContext->doAncF2)
			{
				if (!dmaAncSegmentDescriptor(pDmaContext,
											 &pDmaContext->dmaAncF2Segment,
											 &descSystemAddress,
											 &descCardAddress,
											 &descTransferSize))
				{
					break;
				}
			}
			else if (pDmaContext->doMessage)
			{
				if (pDmaContext->messageCardAddress == 0)
				{
					break;
				}

				descSystemAddress = pDmaContext->messageBusAddress;
				descCardAddress = pDmaContext->messageCardAddress;
				descTransferSize = 4;

				pDmaContext->messageCardAddress = 0;
			}
			else
			{
				break;
			}

			// write descriptor
			pDescriptor->ulControl = valControl;
			pDescriptor->ulTransferCount = descTransferSize;
			pDescriptor->llHostAddress = descSystemAddress;
			pDescriptor->llLocalAddress = descCardAddress;

			// setup for next segment descriptor
			pDescriptorLast = pDescriptor;

			dpPageCount++;
			if (dpPageCount < dpNumPerPage)
			{
				pDescriptor++;
				physDescriptor += sizeof(DMA_DESCRIPTOR64);
			}
			else
			{
				dpPageCount = 0;
				dpIndex++;
				if (dpIndex >= pDmaEngine->numDescriptorPages)
				{
					NTV2_MSG_DESCRIPTOR("%s%d:%s%d:%s%d: dmaNwlProgram too many descriptor pages\n",
										DMA_MSG_CONTEXT);
					return -EPERM;
				}
				pDescriptor = (PNWL_DESCRIPTOR)pDmaEngine->pDescriptorVirtual[dpIndex];
				physDescriptor = pDmaEngine->descriptorPhysical[dpIndex];
			}
			pDescriptorLast->llNextAddress = physDescriptor;

			NTV2_MSG_DESCRIPTOR("%s%d:%s%d:%s%d: dmaNwlProgram con %08x cnt %08x sys %016llx crd %016llx nxt %016llx\n",
								DMA_MSG_CONTEXT, pDescriptorLast->ulControl, pDescriptorLast->ulTransferCount,
								pDescriptorLast->llHostAddress, pDescriptorLast->llLocalAddress, pDescriptorLast->llNextAddress);

			programBytes += descTransferSize;
			descriptorCount++;
			if (descriptorCount > pDmaEngine->maxDescriptors)
			{
				pDmaEngine->programErrorCount++;
				NTV2_MSG_ERROR("%s%d:%s%d:%s%d: dmaNwlProgram exceeded max descriptor count %d\n",
							   DMA_MSG_CONTEXT, descriptorCount);
				return -EPERM;
			}
		}
	}

	// last descriptor generates interrupt
	pDescriptorLast->ulControl = (ULWord)(NWL_CONTROL_IRQ_ON_SHORT_ERR |
										  NWL_CONTROL_IRQ_ON_SHORT_SW |
										  NWL_CONTROL_IRQ_ON_SHORT_HW |
										  NWL_CONTROL_IRQ_ON_COMPLETION);
	// no next descriptor
	pDescriptorLast->llNextAddress = 0;

	NTV2_MSG_DESCRIPTOR("%s%d:%s%d:%s%d: dmaNwlProgram con %08x cnt %08x sys %016llx crd %016llx nxt %016llx\n",
						DMA_MSG_CONTEXT, pDescriptorLast->ulControl, pDescriptorLast->ulTransferCount,
						pDescriptorLast->llHostAddress, pDescriptorLast->llLocalAddress, pDescriptorLast->llNextAddress);
	
	// need at least one descriptor
	if (descriptorCount == 0)
	{
		pDmaEngine->programErrorCount++;
		NTV2_MSG_ERROR("%s%d:%s%d:%s%d: dmaNwlProgram no descriptors generated\n", DMA_MSG_CONTEXT);
		return -EPERM;
	}

	// make sure that engine is not running
	valControl = ReadNwlControlStatus(deviceNumber, nwlC2H, nwlIndex);
	if ((valControl & kRegMaskNwlControlStatusChainRunning) != 0)
	{
		pDmaEngine->programErrorCount++;
		NTV2_MSG_ERROR("%s%d:%s%d:%s%d: dmaNwlProgram dma still running\n", DMA_MSG_CONTEXT);
		dmaNwlAbort(pDmaEngine);
		pDmaEngine->programErrorCount++;
		return -EPERM;
	}

	// reset the dma engine
	ResetNwlHardware(deviceNumber, nwlC2H, nwlIndex);

	// write descriptor start
	WriteNwlChainStartAddressLow(deviceNumber, nwlC2H, nwlIndex, (ULWord)(pDmaEngine->descriptorPhysical[0] & 0xffffffff));
	WriteNwlChainStartAddressHigh(deviceNumber, nwlC2H, nwlIndex, (ULWord)(pDmaEngine->descriptorPhysical[0] >> 32));

	NTV2_MSG_PROGRAM("%s%d:%s%d: dmaNwlProgram dma engine start  size %d bytes\n",
					 DMA_MSG_ENGINE, programBytes);

	// count the program starts and descriptors
	pDmaEngine->programStartCount++;
	pDmaEngine->programDescriptorCount += descriptorCount;
	pDmaEngine->programBytes += programBytes;
	pDmaEngine->programStartTime = ntv2Time100ns();

	// clear interrupt
	WriteNwlControlStatus(deviceNumber, nwlC2H, nwlIndex, kRegMaskNwlControlStatusInterruptActive);

	// start dma
	valControl = (ULWord)(kRegMaskNwlControlStatusInterruptEnable | 
						  kRegMaskNwlControlStatusChainStart |
						  kRegMaskNwlControlStatusChainComplete);
	WriteNwlControlStatus(deviceNumber, nwlC2H, nwlIndex, valControl);

	return 0;
}

static void dmaNwlAbort(PDMA_ENGINE pDmaEngine)
{
	ULWord 					deviceNumber = pDmaEngine->deviceNumber;
	ULWord					nwlIndex = pDmaEngine->dmaIndex;
	bool					nwlC2H = pDmaEngine->dmaC2H;
	ULWord					regValue;

	regValue = ReadNwlControlStatus(deviceNumber, nwlC2H, nwlIndex);
	NTV2_MSG_PROGRAM("%s%d:%s%d: dmaNwlAbort dma engine abort  control status %08x\n",
					 DMA_MSG_ENGINE, regValue);

	// reset the dma engine
	ResetNwlHardware(deviceNumber, nwlC2H, nwlIndex);
}

static void dmaNwlInterrupt(PDMA_ENGINE pDmaEngine)
{
	ULWord 		deviceNumber = pDmaEngine->deviceNumber;
	ULWord		nwlIndex = pDmaEngine->dmaIndex;
	bool		nwlC2H = pDmaEngine->dmaC2H;
	ULWord 		valHardwareTime;
	ULWord		valByteCount;
	ULWord		valDmaStatus;

	// clear interrupt
	WriteNwlControlStatus(deviceNumber, nwlC2H, nwlIndex, kRegMaskNwlControlStatusInterruptActive);

	// check enabled
	if (!pDmaEngine->dmaEnable)
	{
		pDmaEngine->programErrorCount++;
		NTV2_MSG_ERROR("%s%d:%s%d: dmaNwlInterrupt engine not enabled\n", DMA_MSG_ENGINE);
	}

	// check engine state
	if (pDmaEngine->state != DmaStateTransfer)
	{
		NTV2_MSG_ERROR("%s%d:%s%d: dmaNwlInterrupt dma state not transfer %d\n",
					   DMA_MSG_ENGINE, pDmaEngine->state);
		return;
	}

	pDmaEngine->interruptCount++;

	// get the hardware time in nanoseconds
	valHardwareTime = ReadNwlHardwareTime(deviceNumber, nwlC2H, nwlIndex);

	// get the bytes transferred
	valByteCount = ReadNwlChainCompleteByteCount(deviceNumber, nwlC2H, nwlIndex);

	// save statistics
	pDmaEngine->programStopTime = ntv2Time100ns();
	pDmaEngine->programCompleteCount++;
	pDmaEngine->programTime = pDmaEngine->programStopTime - pDmaEngine->programStartTime;
//	pDmaEngine->programTime = valHardwareTime/100;
	pDmaEngine->programBytes = pDmaEngine->programBytes;
//	pDmaEngine->programBytes = valByteCount;

	// check for dma still active or hardware error
	valDmaStatus = ReadNwlControlStatus(deviceNumber, nwlC2H, nwlIndex);
	if ((valDmaStatus & kRegMaskNwlControlStatusChainComplete) == 0)
	{
		// stop the current transaction
		pDmaEngine->programErrorCount++;
		NTV2_MSG_ERROR("%s%d:%s%d: dmaNwlInterrupt dma hardware not done or error %08x\n",
					   DMA_MSG_ENGINE, valDmaStatus);
		dmaNwlAbort(pDmaEngine);
		return;
	}

	NTV2_MSG_PROGRAM("%s%d:%s%d: dmaNwlInterrupt dma complete  size %lld bytes  time sys %lld  hrd %d us\n",
					 DMA_MSG_ENGINE, pDmaEngine->programBytes, 
					 (pDmaEngine->programStopTime - pDmaEngine->programStartTime)/10,
					 valHardwareTime/1000);

	set_bit(0, &pDmaEngine->transferDone);
	wake_up(&pDmaEngine->transferEvent);
}

static int dmaXlnxProgram(PDMA_CONTEXT pDmaContext)
{
	ULWord 					deviceNumber = pDmaContext->deviceNumber;
	DMA_ENGINE				*pDmaEngine = getDmaEngine(deviceNumber, pDmaContext->engIndex);
	ULWord					xlnxIndex = pDmaContext->dmaIndex;
	bool					xlnxC2H = pDmaContext->dmaC2H;
	ULWord					valControl;
	PXLNX_DESCRIPTOR		pDescriptor;
	PXLNX_DESCRIPTOR		pDescriptorLast;
	ULWord64				physDescriptor;
	ULWord					descriptorCount;
	ULWord					sgIndex;
	ULWord64				descSystemAddress;
	ULWord					descCardAddress;
	ULWord					descTransferSize;
	ULWord					descSystemLast;
	ULWord					descTransferLast;
	ULWord					programBytes;
	ULWord					contigCount;
	ULWord					dpIndex;
	ULWord					dpPageMask;
	ULWord					dpPageCount;
	ULWord					dpNumPerPage;
	ULWord					dsIndex;
	bool					done;

	NTV2_MSG_PROGRAM("%s%d:%s%d:%s%d: dmaXlnxProgram program %s %s %s %s\n", 
					 DMA_MSG_CONTEXT, 
					 pDmaContext->doVideo? "video":"",
					 pDmaContext->doAudio? "audio":"",
					 pDmaContext->doAncF1? "ancF1":"",
					 pDmaContext->doAncF2? "ancF2":"");

	// check engine state
	if (pDmaEngine->state != DmaStateTransfer)
	{
		NTV2_MSG_ERROR("%s%d:%s%d:%s%d: dmaXlnxProgram dma state %d not transfer\n", 
					   DMA_MSG_CONTEXT, pDmaEngine->state);
		pDmaEngine->programErrorCount++;
		return -EPERM;
	}
	
	// read status register
	valControl = ReadXlnxDmaStatus(deviceNumber, xlnxC2H, xlnxIndex);
	if (valControl == 0xffffffff)
	{
		NTV2_MSG_ERROR("%s%d:%s%d:%s%d: dmaXlnxProgram dma hardware not present\n", 
					   DMA_MSG_CONTEXT);
		pDmaEngine->programErrorCount++;
		return -EPERM;
	}
	
	// setup for descriptor loop
	valControl = XLNX_CONTROL_DESC_MAGIC;
	dpIndex = 0;
	dpPageCount = 0;
	dpPageMask = PAGE_SIZE - 1;
	dpNumPerPage = PAGE_SIZE / DMA_DESCRIPTOR_SIZE;
	pDescriptor = (PXLNX_DESCRIPTOR)pDmaEngine->pDescriptorVirtual[dpIndex];
	pDescriptorLast = pDescriptor;
	physDescriptor = pDmaEngine->descriptorPhysical[dpIndex];
	descriptorCount = 0;
	programBytes = 0;
	sgIndex = 0;
	done = false;
	descSystemLast = 0;
	descTransferLast = 0;

	// iterate through the mapped descriptor lists
	while (!done)
	{
		// setup descriptor generator
		while (!done)
		{
			if (pDmaContext->doVideo)
			{
				if ((sgIndex < dmaSgSize(pDmaContext->pVideoPageBuffer)) && 
					dmaVideoSegmentTransfer(pDmaContext,
											&pDmaContext->dmaVideoSegment,
											dmaSgAddress(pDmaContext->pVideoPageBuffer, sgIndex),
											dmaSgLength(pDmaContext->pVideoPageBuffer, sgIndex)))
				{
					sgIndex++;
					break;
				}
				pDmaContext->doVideo = false;
				sgIndex = 0;
			}
		
			if (pDmaContext->doAudio)
			{
				if ((sgIndex < dmaSgSize(pDmaContext->pAudioPageBuffer)) && 
					dmaAudioSegmentTransfer(pDmaContext,
											&pDmaContext->dmaAudioSegment,
											dmaSgAddress(pDmaContext->pAudioPageBuffer, sgIndex),
											dmaSgLength(pDmaContext->pAudioPageBuffer, sgIndex)))
				{
					sgIndex++;
					break;
				}
				pDmaContext->doAudio = false;
				sgIndex = 0;
			}
		
			if (pDmaContext->doAncF1)
			{
				if ((sgIndex < dmaSgSize(pDmaContext->pAncF1PageBuffer)) && 
					dmaAncSegmentTransfer(pDmaContext,
										  &pDmaContext->dmaAncF1Segment,
										  dmaSgAddress(pDmaContext->pAncF1PageBuffer, sgIndex),
										  dmaSgLength(pDmaContext->pAncF1PageBuffer, sgIndex)))
				{
					sgIndex++;
					break;
				}
				pDmaContext->doAncF1 = false;
				sgIndex = 0;
			}
		
			if (pDmaContext->doAncF2)
			{
				if ((sgIndex < dmaSgSize(pDmaContext->pAncF2PageBuffer)) && 
					dmaAncSegmentTransfer(pDmaContext,
										  &pDmaContext->dmaAncF2Segment,
										  dmaSgAddress(pDmaContext->pAncF2PageBuffer, sgIndex),
										  dmaSgLength(pDmaContext->pAncF2PageBuffer, sgIndex)))
				{
					sgIndex++;
					break;
				}
				pDmaContext->doAncF2 = false;
				sgIndex = 0;
			}
		
			if (pDmaContext->doMessage)
			{
				if ((sgIndex < 1) &&
					(pDmaContext->messageBusAddress != 0) &&
					(pDmaContext->messageCardAddress != 0))
				{
					sgIndex++;
					break;
				}
				pDmaContext->doMessage = false;
				sgIndex = 0;
			}

			done = true;
		}

		// generate the segment descriptors for each mapped descriptor
		while (!done)
		{
			descSystemAddress = 0;
			descCardAddress = 0;
			descTransferSize = 0;

			// get the next segment descriptor
			if (pDmaContext->doVideo)
			{
				if (!dmaVideoSegmentDescriptor(pDmaContext,
											   &pDmaContext->dmaVideoSegment,
											   &descSystemAddress,
											   &descCardAddress,
											   &descTransferSize))
				{
					break;
				}
			}
			else if (pDmaContext->doAudio)
			{
				if (!dmaAudioSegmentDescriptor(pDmaContext,
											   &pDmaContext->dmaAudioSegment,
											   &descSystemAddress,
											   &descCardAddress,
											   &descTransferSize))
				{
					break;
				}
			}
			else if (pDmaContext->doAncF1)
			{
				if (!dmaAncSegmentDescriptor(pDmaContext,
											 &pDmaContext->dmaAncF1Segment,
											 &descSystemAddress,
											 &descCardAddress,
											 &descTransferSize))
				{
					break;
				}
			}
			else if (pDmaContext->doAncF2)
			{
				if (!dmaAncSegmentDescriptor(pDmaContext,
											 &pDmaContext->dmaAncF2Segment,
											 &descSystemAddress,
											 &descCardAddress,
											 &descTransferSize))
				{
					break;
				}
			}
			else if (pDmaContext->doMessage)
			{
				if (pDmaContext->messageCardAddress == 0)
				{
					break;
				}

				descSystemAddress = pDmaContext->messageBusAddress;
				descCardAddress = pDmaContext->messageCardAddress;
				descTransferSize = 4;

				pDmaContext->messageCardAddress = 0;
			}
			else
			{
				break;
			}
#if 0
			// coalesce adjacent addresses
			if ((descTransferLast != 0) &&
				(descSystemAddress == (descSystemLast + descTransferLast)))
			{
				pDescriptorLast->ulTransferCount += descTransferSize;
				descTransferLast += descTransferSize;
				programBytes += descTransferSize;
				continue;
			}
#endif
			// xlnx can fetch up to 16 descriptors at once if they are contiguous and do not span pages
			contigCount = dpNumPerPage - dpPageCount - 1;
			if (contigCount > 0)
			{
				contigCount--;
			}
			if (contigCount > XLNX_MAX_ADJACENT_COUNT)
			{
				contigCount = XLNX_MAX_ADJACENT_COUNT;
			}

			// write descriptor
			pDescriptor->ulControl = valControl | (contigCount << 8);
			pDescriptor->ulTransferCount = descTransferSize;
			if (xlnxC2H)
			{
				pDescriptor->llDstAddress = descSystemAddress;
				pDescriptor->llSrcAddress = descCardAddress;
			}
			else
			{
				pDescriptor->llSrcAddress = descSystemAddress;
				pDescriptor->llDstAddress = descCardAddress;
			}

			// setup for next segment descriptor
			pDescriptorLast = pDescriptor;
			descSystemLast = descSystemAddress;
			descTransferLast = descTransferSize;

			dpPageCount++;
			if (dpPageCount < dpNumPerPage)
			{
				pDescriptor++;
				physDescriptor += sizeof(DMA_DESCRIPTOR64);
			}
			else
			{
				dpPageCount = 0;
				dpIndex++;
				if (dpIndex >= pDmaEngine->numDescriptorPages)
				{
					NTV2_MSG_DESCRIPTOR("%s%d:%s%d:%s%d: dmaXlnxProgram too many descriptor pages\n",
										DMA_MSG_CONTEXT);
					return -EPERM;
				}
				pDescriptor = (PXLNX_DESCRIPTOR)pDmaEngine->pDescriptorVirtual[dpIndex];
				physDescriptor = pDmaEngine->descriptorPhysical[dpIndex];
			}
			pDescriptorLast->llNextAddress = physDescriptor;

			NTV2_MSG_DESCRIPTOR("%s%d:%s%d:%s%d: dmaXlnxProgram con %08x cnt %08x src %016llx dst %016llx nxt %016llx\n",
								DMA_MSG_CONTEXT, pDescriptorLast->ulControl, pDescriptorLast->ulTransferCount,
								pDescriptorLast->llSrcAddress, pDescriptorLast->llDstAddress, pDescriptorLast->llNextAddress);

			programBytes += descTransferSize;
			descriptorCount++;
			if (descriptorCount > pDmaEngine->maxDescriptors)
			{
				pDmaEngine->programErrorCount++;
				NTV2_MSG_ERROR("%s%d:%s%d:%s%d: dmaXlnxProgram exceeded max descriptor count %d\n",
							   DMA_MSG_CONTEXT, descriptorCount);
				return -EPERM;
			}
		}
	}

	// reset final contig counts
	if (((unsigned long)pDescriptorLast & (unsigned long)dpPageMask) != 0)
	{
		pDescriptor = pDescriptorLast - 1;
		for (dsIndex = 0; dsIndex < XLNX_MAX_ADJACENT_COUNT; dsIndex++)
		{
			pDescriptor->ulControl = valControl;
			NTV2_MSG_DESCRIPTOR("%s%d:%s%d:%s%d: dmaXlnxProgram con %08x cnt %08x src %016llx dst %016llx nxt %016llx clear adjacent\n",
								DMA_MSG_CONTEXT, pDescriptor->ulControl, pDescriptor->ulTransferCount,
								pDescriptor->llSrcAddress, pDescriptor->llDstAddress, pDescriptor->llNextAddress);
			if (((unsigned long)pDescriptor & (unsigned long)dpPageMask) == 0)
				break;
			pDescriptor--;
		}
	}

	// last descriptor generates interrupt
	pDescriptorLast->ulControl = XLNX_CONTROL_DESC_STOP | XLNX_CONTROL_DESC_COMPLETION | XLNX_CONTROL_DESC_MAGIC;
	// no next descriptor
	pDescriptorLast->llNextAddress = 0;

	NTV2_MSG_DESCRIPTOR("%s%d:%s%d:%s%d: dmaXlnxProgram con %08x cnt %08x src %016llx dst %016llx nxt %016llx repeat last\n",
						DMA_MSG_CONTEXT, pDescriptorLast->ulControl, pDescriptorLast->ulTransferCount,
						pDescriptorLast->llSrcAddress, pDescriptorLast->llDstAddress, pDescriptorLast->llNextAddress);

	// need at least one descriptor
	if (descriptorCount == 0)
	{
		pDmaEngine->programErrorCount++;
		NTV2_MSG_ERROR("%s%d:%s%d:%s%d: dmaXlnxProgram no descriptors generated\n", DMA_MSG_CONTEXT);
		return -EPERM;
	}

	// make sure that engine is not running
	if (IsXlnxDmaActive(valControl))
	{
		pDmaEngine->programErrorCount++;
		NTV2_MSG_ERROR("%s%d:%s%d:%s%d: dmaXlnxProgram dma still running\n", DMA_MSG_CONTEXT);
		dmaXlnxAbort(pDmaEngine);
		pDmaEngine->programErrorCount++;
		return -EPERM;
	}

	// write descriptor start
	WriteXlnxDmaEngineStartLow(deviceNumber, xlnxC2H, xlnxIndex, (ULWord)(pDmaEngine->descriptorPhysical[0] & 0xffffffff));
	WriteXlnxDmaEngineStartHigh(deviceNumber, xlnxC2H, xlnxIndex, (ULWord)(pDmaEngine->descriptorPhysical[0] >> 32));
	WriteXlnxDmaEngineStartAdjacent(deviceNumber, xlnxC2H, xlnxIndex,
									(descriptorCount > XLNX_MAX_ADJACENT_COUNT) ? XLNX_MAX_ADJACENT_COUNT : 0);

	NTV2_MSG_PROGRAM("%s%d:%s%d: dmaXlnxProgram dma engine start  size %d bytes\n",
					 DMA_MSG_ENGINE, programBytes);

	// count the program starts and descriptors
	pDmaEngine->programStartCount++;
	pDmaEngine->programDescriptorCount += descriptorCount;
	pDmaEngine->programBytes += programBytes;
	pDmaEngine->programStartTime = ntv2Time100ns();

	// start dma
	ClearXlnxDmaStatus(deviceNumber, xlnxC2H, xlnxIndex);
	EnableXlnxDmaInterrupt(deviceNumber, xlnxC2H, xlnxIndex);
	StartXlnxDma(deviceNumber,xlnxC2H, xlnxIndex);

	return 0;
}

static void dmaXlnxAbort(PDMA_ENGINE pDmaEngine)
{
	ULWord 					deviceNumber = pDmaEngine->deviceNumber;
	ULWord					xlnxIndex = pDmaEngine->dmaIndex;
	bool					xlnxC2H = pDmaEngine->dmaC2H;
	ULWord					regValue;

	regValue = ReadXlnxDmaStatus(deviceNumber, xlnxC2H, xlnxIndex);
	NTV2_MSG_PROGRAM("%s%d:%s%d: dmaXlnxAbort dma engine abort  control status %08x\n",
					 DMA_MSG_ENGINE, regValue);

	// disable the interrupt
	DisableXlnxDmaInterrupt(deviceNumber, xlnxC2H, xlnxIndex);
	// stop the dma (should be reset)
	StopXlnxDma(deviceNumber, xlnxC2H, xlnxIndex);
	// wait for engine stop (can take a couple of register reads)
	WaitXlnxDmaActive(deviceNumber, xlnxC2H, xlnxIndex, false);
}

static void dmaXlnxInterrupt(PDMA_ENGINE pDmaEngine)
{
	ULWord 		deviceNumber = pDmaEngine->deviceNumber;
	ULWord		xlnxIndex = pDmaEngine->dmaIndex;
	bool		xlnxC2H = pDmaEngine->dmaC2H;
	ULWord 		valHardwareTime;
	ULWord		valByteCount;
	ULWord		valReadRequestSize;
	ULWord		valDmaStatus;
	bool		done;

	// disable interrupt and stop dma engine
	if (!pDmaEngine->dmaStream)
    {
        DisableXlnxDmaInterrupt(deviceNumber, xlnxC2H, xlnxIndex);
        StopXlnxDma(deviceNumber, xlnxC2H, xlnxIndex);
    }

	// check enabled
	if (!pDmaEngine->dmaEnable)
	{
		pDmaEngine->programErrorCount++;
		NTV2_MSG_ERROR("%s%d:%s%d: dmaXlnxInterrupt engine not enabled\n", DMA_MSG_ENGINE);
	}

	// check engine state
	if (pDmaEngine->state != DmaStateTransfer)
	{
		NTV2_MSG_ERROR("%s%d:%s%d: dmaXlnxInterrupt dma state not transfer %d\n",
					   DMA_MSG_ENGINE, pDmaEngine->state);
		return;
	}

	pDmaEngine->interruptCount++;

	if (pDmaEngine->dmaStream)
	{
		valDmaStatus = ReadXlnxDmaStatus(deviceNumber, xlnxC2H, xlnxIndex);
		if ((pDmaEngine->interruptCount < 10) || ((pDmaEngine->interruptCount % 60) == 0) || IsXlnxDmaError(valDmaStatus))
		{
			NTV2_MSG_PROGRAM("%s%d:%s%d: dmaXlnxInterrupt dma streaming count %lld  status %08x\n",
                             DMA_MSG_ENGINE, pDmaEngine->interruptCount, valDmaStatus);
		}
		return;
	}

	// wait for engine stop (can take a couple of register reads)
	done = WaitXlnxDmaActive(deviceNumber, xlnxC2H, xlnxIndex, false);

	// get the hardware time in clocks
	valHardwareTime = ReadXlnxPerformanceCycleCount(deviceNumber, xlnxC2H, xlnxIndex);

	// get the bytes transferred
	valByteCount = ReadXlnxPerformanceDataCount(deviceNumber, xlnxC2H, xlnxIndex);

	// get the read request size
	valReadRequestSize = ReadXlnxMaxReadRequestSize(deviceNumber);

	// save statistics
	pDmaEngine->programStopTime = ntv2Time100ns();
	pDmaEngine->programCompleteCount++;
	pDmaEngine->programTime = pDmaEngine->programStopTime - pDmaEngine->programStartTime;
//	pDmaEngine->programTime = valHardwareTime / 25;  // 250 MHz clock
	pDmaEngine->programBytes = pDmaEngine->programBytes;

	// check for dma still active or hardware error
	valDmaStatus = ReadXlnxDmaStatus(deviceNumber, xlnxC2H, xlnxIndex);
	if (!done || IsXlnxDmaError(valDmaStatus))
	{
		// stop the current transaction
		pDmaEngine->programErrorCount++;
		NTV2_MSG_ERROR("%s%d:%s%d: dmaXlnxInterrupt dma hardware not done or error %08x\n",
					   DMA_MSG_ENGINE, valDmaStatus);
		dmaXlnxAbort(pDmaEngine);
		return;
	}

	NTV2_MSG_PROGRAM("%s%d:%s%d: dmaXlnxInterrupt dma complete  size %lld bytes  time sys %lld  hrd %d us  rq pgm %1d  eff %1d\n",
					 DMA_MSG_ENGINE, pDmaEngine->programBytes, 
					 (pDmaEngine->programStopTime - pDmaEngine->programStartTime)/10,
					 valHardwareTime / 250,
					 (valReadRequestSize & kRegMaskXlnxUserMaxReadRequestPgm) >> kRegShiftXlnxUserMaxReadRequestPgm,
					 (valReadRequestSize & kRegMaskXlnxUserMaxReadRequestEff) >> kRegShiftXlnxUserMaxReadRequestEff);

	set_bit(0, &pDmaEngine->transferDone);
	wake_up(&pDmaEngine->transferEvent);
}

int dmaOpsStreamInitialize(struct ntv2_stream *stream)
{
    PDMA_ENGINE pDmaEngine = (PDMA_ENGINE)stream->dma_engine;
//    int status = 0;

    // lock the engine
    dmaEngineLock(pDmaEngine);

    // stop the dma engine synchonously
    NTV2_MSG_STATE("%s%d:%s%d: dmaOpsStreamInitialize stop the dma\n", DMA_MSG_ENGINE);
#if 0    
    status = dmaXlnxStreamStop(pDmaEngine);
    if (status < 0)
    {
        stream->engine_state = ntv2_stream_state_error;
        dmaEngineUnlock(pDmaEngine);        
        return NTV2_STREAM_OPS_FAIL;
    }        
#endif
    // reset counters

    // set state
    stream->stream_state = ntv2_stream_state_initialized;
    stream->engine_state = ntv2_stream_state_initialized;

    // unlock the engine
    dmaEngineUnlock(pDmaEngine);
    
    NTV2_MSG_STATE("%s%d:%s%d: dmaOpsStreamInitialize stream/engine state initialized\n", DMA_MSG_ENGINE);    
    return NTV2_STREAM_OPS_SUCCESS;
}

int dmaOpsStreamRelease(struct ntv2_stream *stream)
{    
    PDMA_ENGINE pDmaEngine = (PDMA_ENGINE)stream->dma_engine;
//    int status = 0;

    // lock the engine
    dmaEngineLock(pDmaEngine);

    // stop the dma engine synchonously
    NTV2_MSG_STATE("%s%d:%s%d: dmaOpsStreamRelease stop the dma\n", DMA_MSG_ENGINE);
#if 0
    status = dmaXlnxStreamStop(pDmaEngine);
    if (status < 0)
    {
        stream->engine_state = ntv2_stream_state_error;
        dmaEngineUnlock(pDmaEngine);        
        return NTV2_STREAM_OPS_FAIL;
    }        
#endif
    // reset counters

    // set state
    stream->stream_state = ntv2_stream_state_released;
    stream->engine_state = ntv2_stream_state_released;

    // unlock the engine
    dmaEngineUnlock(pDmaEngine);
    
    NTV2_MSG_STATE("%s%d:%s%d: dmaOpsStreamRelease stream/engine state released\n", DMA_MSG_ENGINE);    
    return NTV2_STREAM_OPS_SUCCESS;
}

int dmaOpsStreamStart(struct ntv2_stream *stream)
{
    PDMA_ENGINE pDmaEngine = (PDMA_ENGINE)stream->dma_engine;
//    int start_index = 0;
//    int status = 0;
    
    // lock the engine
    dmaEngineLock(pDmaEngine);

    if (stream->engine_state == ntv2_stream_state_error)
    {
        dmaEngineUnlock(pDmaEngine);
        return NTV2_STREAM_OPS_FAIL;
    }

    if(stream->stream_state == ntv2_stream_state_initialized)
    {
        // start the engine
        NTV2_MSG_STATE("%s%d:%s%d: dmaOpsStreamStart start the dma\n", DMA_MSG_ENGINE);
#if 0        
        status = dmaXlnxStreamStart(pDmaEngine, start_index);
        if (status < 0)
        {
            stream->engine_state = ntv2_stream_state_error;
            dmaEngineUnlock(pDmaEngine);
            return NTV2_STREAM_OPS_FAIL;
        }
#endif        
    }
    
    // set state
    stream->stream_state = ntv2_stream_state_active;

    // unlock the engine
    dmaEngineUnlock(pDmaEngine);

    NTV2_MSG_STATE("%s%d:%s%d: dmaOpsStreamStart stream state active\n", DMA_MSG_ENGINE);
    return NTV2_STREAM_OPS_SUCCESS;
}

int dmaOpsStreamStop(struct ntv2_stream *stream)
{    
    PDMA_ENGINE pDmaEngine = (PDMA_ENGINE)stream->dma_engine;

    // sync with the engine
    dmaEngineLock(pDmaEngine);
    
    if (stream->engine_state == ntv2_stream_state_error)
    {
        dmaEngineUnlock(pDmaEngine);
        return NTV2_STREAM_OPS_FAIL;
    }

    // set state
    stream->stream_state = ntv2_stream_state_idle;

    dmaEngineUnlock(pDmaEngine);

    NTV2_MSG_STATE("%s%d:%s%d: dmaOpsStreamStop stream state idle\n", DMA_MSG_ENGINE);
    return NTV2_STREAM_OPS_SUCCESS;
}

int dmaOpsStreamAdvance(struct ntv2_stream *stream)
{
    PDMA_ENGINE pDmaEngine = (PDMA_ENGINE)stream->dma_engine;

    // lock the engine
    dmaEngineLock(pDmaEngine);

    if ((stream->engine_state != ntv2_stream_state_idle) &&
        (stream->engine_state != ntv2_stream_state_active))
    {
        dmaEngineUnlock(pDmaEngine);
        return NTV2_STREAM_OPS_FAIL;
    }
    
    if (stream->stream_state == ntv2_stream_state_active)
    {
        if (stream->engine_state != ntv2_stream_state_active)
        {
            NTV2_MSG_STATE("%s%d:%s%d: dmaOpsStreamAdvance engine state active\n", DMA_MSG_ENGINE);
        }
        
        // update engine state
        stream->engine_state = ntv2_stream_state_active;
    }
    else
    {
        if (stream->engine_state != ntv2_stream_state_idle)
        {
            NTV2_MSG_STATE("%s%d:%s%d: dmaOpsStreamAdvance engine state idle\n", DMA_MSG_ENGINE);
        }

        // update engine state
        stream->engine_state = ntv2_stream_state_idle;
    }
        
    // unlock the engine
    dmaEngineUnlock(pDmaEngine);

    return NTV2_STREAM_OPS_SUCCESS;
}

int dmaOpsBufferPrepare(struct ntv2_stream *stream, int index)
{
    PDMA_ENGINE pDmaEngine = (PDMA_ENGINE)stream->dma_engine;
    ULWord deviceNumber = stream->system_context->devNum;
    struct ntv2_stream_buffer* buffer = &stream->stream_buffers[index];
    PFILE_DATA file = (PFILE_DATA)stream->owner;
    PDMA_PAGE_ROOT page_root = NULL;
    PDMA_PAGE_BUFFER page_buffer = NULL;

    // check prepared
    if (buffer->prepared)
    {
        return NTV2_STREAM_OPS_SUCCESS;
    }

    // lock the engine to reset the flags
    dmaEngineLock(pDmaEngine);

    buffer->prepared = false;
    buffer->linked = false;
    buffer->completed = false;
    buffer->flushed = false;
    buffer->released = false;
    buffer->dma_buffer = NULL;

    dmaEngineUnlock(pDmaEngine);

    // check file pointer
    if (file == NULL)
    {
        NTV2_MSG_ERROR("%s%d:%s%d: dmaOpsBufferPrepare null file pointer\n", DMA_MSG_ENGINE);
        return NTV2_STREAM_OPS_FAIL;
    }

    // check page root
    if (page_root == NULL)
    {
        NTV2_MSG_ERROR("%s%d:%s%d: dmaOpsBufferPrepare null page root\n", DMA_MSG_ENGINE);
        return NTV2_STREAM_OPS_FAIL;
    }

    // check for no video
    if(((PVOID)(buffer->user_buffer.mBuffer.fUserSpacePtr) == NULL) || (buffer->user_buffer.mBuffer.fByteCount == 0))
    {
        NTV2_MSG_ERROR("%s%d:%s%d: dmaOpsBufferPrepare no buffer\n", DMA_MSG_ENGINE);
        return NTV2_STREAM_OPS_FAIL;
    }

    // check buffer cache
    page_buffer = dmaPageRootFind(deviceNumber,
                                  page_root,
                                  (PVOID)(buffer->user_buffer.mBuffer.fUserSpacePtr),
                                  buffer->user_buffer.mBuffer.fByteCount);
    if (page_buffer == NULL)
    {
        NTV2_MSG_ERROR("%s%d:%s%d: dmaOpsBufferPrepare buffer not found\n", DMA_MSG_ENGINE);
        return NTV2_STREAM_OPS_FAIL;
    }

    if (!dmaPageLocked(page_buffer))
    {
        NTV2_MSG_ERROR("%s%d:%s%d: dmaOpsBufferPrepare buffer not locked\n", DMA_MSG_ENGINE);
        return NTV2_STREAM_OPS_FAIL;
    }

    if (!dmaSgMapped(page_buffer))
    {
        NTV2_MSG_ERROR("%s%d:%s%d: dmaOpsBufferPrepare buffer not mapped\n", DMA_MSG_ENGINE);
        return NTV2_STREAM_OPS_FAIL;
    }

    // lock the engine to set the flags
    dmaEngineLock(pDmaEngine);

    buffer->prepared = true;
    buffer->linked = false;
    buffer->completed = false;
    buffer->flushed = false;
    buffer->released = false;
    buffer->dma_buffer = (void*)page_buffer;
    buffer->ds_index = 0;
    buffer->ds_count = 0;
    
    dmaEngineUnlock(pDmaEngine);

    NTV2_MSG_PROGRAM("%s%d:%s%d: dmaOpsBufferPrepare address %016llx\n",
                     DMA_MSG_ENGINE, (ULWord64)buffer->user_buffer.mBuffer.fUserSpacePtr);

    return NTV2_STREAM_OPS_SUCCESS;
}

int dmaOpsBufferLink(struct ntv2_stream *stream, int from_index, int to_index)
{
    PDMA_ENGINE pDmaEngine = (PDMA_ENGINE)stream->dma_engine;
    struct ntv2_stream_buffer* buffer_from;
    struct ntv2_stream_buffer* buffer_to;
    PDMA_PAGE_BUFFER  page_from;
    PDMA_PAGE_BUFFER  page_to;
    uint32_t desc_index;
    uint32_t next_index;
//    int status;

    // lock the engine
    dmaEngineLock(pDmaEngine);

    if (stream->engine_state == ntv2_stream_state_error)
    {
        dmaEngineUnlock(pDmaEngine);
        return NTV2_STREAM_OPS_FAIL;
    }

    // get from page buffer
    buffer_from = &stream->stream_buffers[from_index];
    page_from = (PDMA_PAGE_BUFFER)buffer_from->dma_buffer;
    if (!buffer_from->prepared ||
        buffer_from->flushed ||
        buffer_from->completed ||
        buffer_from->released ||
        (page_from == NULL))
    {
        dmaEngineUnlock(pDmaEngine);
        return NTV2_STREAM_OPS_FAIL;
    }

    // link the from buffer if necessary
    if (!buffer_from->linked)
    {
        // build active buffer descriptors (stream starting)
        buffer_from->ds_index = 0;
#if 0        
        status = dmaXlnxStreamBuild(pDmaEngine, page_from, buffer_from->ds_index);
        if (status <= 0)
        {
            stream->engine_state = ntv2_stream_state_error;
            dmaEngineUnlock(pDmaEngine);
            return NTV2_STREAM_OPS_FAIL;
        }
        buffer_from->ds_count = (uint32_t)status;

        // link from buffer to itself
        desc_index = (buffer_from->ds_index + buffer_from->ds_count - 1) % pDmaEngine->maxDescriptors;
        next_index = buffer_from->ds_index;
        status = dmaXlnxStreamLink(pDmaEngine, desc_index, next_index);
        if (status < 0)
        {
            stream->engine_state = ntv2_stream_state_error;
            dmaEngineUnlock(pDmaEngine);
            return NTV2_STREAM_OPS_FAIL;
        }
#endif       
        buffer_from->linked = true;
    }

    // get to page buffer
    buffer_to = &stream->stream_buffers[to_index];
    page_to = (PDMA_PAGE_BUFFER)buffer_to->dma_buffer;
    
    if (!buffer_to->prepared ||
        buffer_to->flushed ||
        buffer_to->completed ||
        buffer_to->released ||
        (page_to == NULL))
    {
        dmaEngineUnlock(pDmaEngine);
        return NTV2_STREAM_OPS_FAIL;
    }
    
    // build to buffer descriptors
    if (!buffer_to->linked)
    {
        buffer_to->ds_index = buffer_from->ds_index + buffer_from->ds_count;
#if 0        
        status = dmaXlnxStreamBuild(pDmaEngine, page_to, buffer_to->ds_index);
        if (status <= 0)
        {
            stream->engine_state = ntv2_stream_state_error;
            dmaEngineUnlock(pDmaEngine);
            return NTV2_STREAM_OPS_FAIL;
        }
        buffer_to->ds_count = (uint32_t)status;

        // link to buffer to itself
        desc_index = (buffer_to->ds_index + buffer_to->ds_count - 1) % pDmaEngine->maxDescriptors;
        next_index = buffer_to->ds_index;
        status = dmaXlnxStreamLink(pDmaEngine, desc_index, next_index);
        if (status < 0)
        {
            stream->engine_state = ntv2_stream_state_error;
            dmaEngineUnlock(pDmaEngine);
            return NTV2_STREAM_OPS_FAIL;
        }
#endif       
        buffer_to->linked = true;
    }
    
    // link descriptor chain
    if (from_index != to_index)
    {
        desc_index = (buffer_from->ds_index + buffer_from->ds_count - 1) % pDmaEngine->maxDescriptors;
        next_index = buffer_to->ds_index;
#if 0        
        status = dmaXlnxStreamLink(pDmaEngine, desc_index, next_index);
        if (status < 0)
        {
            stream->engine_state = ntv2_stream_state_error;
            dmaEngineUnlock(pDmaEngine);
            return NTV2_STREAM_OPS_FAIL;
        }
#endif        
    }

    dmaEngineUnlock(pDmaEngine);

    NTV2_MSG_PROGRAM("%s%d:%s%d: dmaOpsBufferLink address %016llx to %016llx\n",
                     DMA_MSG_ENGINE,
                     (ULWord64)buffer_from->user_buffer.mBuffer.fUserSpacePtr,
                     (ULWord64)buffer_to->user_buffer.mBuffer.fUserSpacePtr);
   
    return NTV2_STREAM_OPS_SUCCESS;
}

int dmaOpsBufferComplete(struct ntv2_stream *stream, int index)
{
    PDMA_ENGINE pDmaEngine = (PDMA_ENGINE)stream->dma_engine;
//    ULWord deviceNumber = stream->system_context->devNum;
    struct ntv2_stream_buffer* buffer = &stream->stream_buffers[index];

    // lock the engine
    dmaEngineLock(pDmaEngine);

    if ((buffer->released) || (buffer->flushed))
    {
        dmaEngineUnlock(pDmaEngine);
        return NTV2_STREAM_OPS_SUCCESS;
    }

    if ((!buffer->prepared) || (buffer->flushed))
    {
        dmaEngineUnlock(pDmaEngine);
        return NTV2_STREAM_OPS_FAIL;
    }

    buffer->completed = true;

    dmaEngineUnlock(pDmaEngine);
    
    NTV2_MSG_PROGRAM("%s%d:%s%d: dmaOpsBufferComplete address %016llx\n",
                     DMA_MSG_ENGINE, (ULWord64)buffer->user_buffer.mBuffer.fUserSpacePtr);
    
    return NTV2_STREAM_OPS_SUCCESS;
}

int dmaOpsBufferFlush(struct ntv2_stream *stream, int index)
{
    PDMA_ENGINE pDmaEngine = (PDMA_ENGINE)stream->dma_engine;
//    ULWord deviceNumber = stream->system_context->devNum;
    struct ntv2_stream_buffer* buffer = &stream->stream_buffers[index];

    // lock the engine
    dmaEngineLock(pDmaEngine);

    if ((buffer->released) || (buffer->flushed))
    {
        dmaEngineUnlock(pDmaEngine);
        return NTV2_STREAM_OPS_SUCCESS;
    }

    if (!buffer->prepared)
    {
        dmaEngineUnlock(pDmaEngine);
        return NTV2_STREAM_OPS_FAIL;
    }

    if ((stream->engine_state == ntv2_stream_state_active) ||
        (stream->engine_state == ntv2_stream_state_idle))
    {    
        if (buffer->linked)
        {
            dmaEngineUnlock(pDmaEngine);
            return NTV2_STREAM_OPS_FAIL;
        }
    }
    buffer->flushed = true;

    dmaEngineUnlock(pDmaEngine);
    
    NTV2_MSG_PROGRAM("%s%d:%s%d: dmaOpsBufferFlush address %016llx\n",
                     DMA_MSG_ENGINE, (ULWord64)buffer->user_buffer.mBuffer.fUserSpacePtr);
    
    return NTV2_STREAM_OPS_SUCCESS;
}

int dmaOpsBufferRelease(struct ntv2_stream *stream, int index)
{
    PDMA_ENGINE pDmaEngine = (PDMA_ENGINE)stream->dma_engine;
    ULWord deviceNumber = stream->system_context->devNum;
    struct ntv2_stream_buffer* buffer = &stream->stream_buffers[index];
    PDMA_PAGE_BUFFER pPageBuffer = (PDMA_PAGE_BUFFER)buffer->dma_buffer;

    // lock the engine
    dmaEngineLock(pDmaEngine);

    if (buffer->released)
    {
        return NTV2_STREAM_OPS_SUCCESS;
    }
    
    if ((stream->engine_state == ntv2_stream_state_active) ||
        (stream->engine_state == ntv2_stream_state_idle))
    {    
        if (buffer->linked && !buffer->completed)
        {
            dmaEngineUnlock(pDmaEngine);
            return NTV2_STREAM_OPS_FAIL;
        }
    }

    buffer->released = true;
    buffer->dma_buffer = NULL;

    dmaEngineUnlock(pDmaEngine);

    dmaPageRootFree(deviceNumber, pPageBuffer);
    
    NTV2_MSG_PROGRAM("%s%d:%s%d: dmaOpsBufferRelease address %016llx\n",
                     DMA_MSG_ENGINE, (ULWord64)buffer->user_buffer.mBuffer.fUserSpacePtr);
    
    return NTV2_STREAM_OPS_SUCCESS;
}

static int dmaXlnxStreamBuild(PDMA_ENGINE pDmaEngine, PDMA_PAGE_BUFFER pPageBuffer, uint32_t index)
{
//	ULWord 					deviceNumber = pDmaEngine->deviceNumber;
//	ULWord					xlnxIndex = pDmaEngine->dmaIndex;
	bool					xlnxC2H = pDmaEngine->dmaC2H;
	ULWord					valControl;
	PXLNX_DESCRIPTOR		pDescriptor;
	PXLNX_DESCRIPTOR		pDescriptorLast;
	ULWord64				physDescriptor;
	ULWord					descriptorCount;
	ULWord					sgIndex;
    ULWord                  sgCount;
	ULWord64				descSystemAddress;
	ULWord					descCardAddress;
	ULWord					descTransferSize;
	ULWord					descSystemLast;
	ULWord					descTransferLast;
	ULWord					programBytes;
	ULWord					contigCount;
	ULWord					dpPage;
	ULWord					dpPageMask;
	ULWord					dpIndex;
	ULWord					dpNumPerPage;
	ULWord					dsIndex;
	bool					done;

	NTV2_MSG_PROGRAM("%s%d:%s%d: dmaXlnxStreamBuild index %d\n", 
					 DMA_MSG_ENGINE, index);

    if (!dmaSgMapped(pPageBuffer))
    {
		NTV2_MSG_ERROR("%s%d:%s%d: dmaXlnxStreamBuild buffer not segment mapped\n", 
					   DMA_MSG_ENGINE);
		pDmaEngine->programErrorCount++;
		return -EPERM;
	}
	
	// get first descriptor
    if (dmaXlnxDescIndexToPage(index, &dpPage, &dpIndex) != 0)
    {
        return -EPERM;
    }

	// setup for descriptor loop
	valControl = XLNX_CONTROL_DESC_MAGIC;
	dpPageMask = PAGE_SIZE - 1;
	dpNumPerPage = PAGE_SIZE / DMA_DESCRIPTOR_SIZE;
	pDescriptor = (PXLNX_DESCRIPTOR)pDmaEngine->pDescriptorVirtual[dpPage];
	pDescriptorLast = pDescriptor;
	physDescriptor = pDmaEngine->descriptorPhysical[dpPage];
	descriptorCount = 0;
	programBytes = 0;
	sgIndex = 0;
    sgCount = dmaSgSize(pPageBuffer);
	done = false;
	descSystemLast = 0;
	descTransferLast = 0;

    if (sgCount == 0)
    {
		NTV2_MSG_ERROR("%s%d:%s%d: dmaXlnxStreamBuild buffer segment count is zero\n", 
					   DMA_MSG_ENGINE);
		pDmaEngine->programErrorCount++;
		return -EPERM;
    }
    
    if (sgCount > (pDmaEngine->numDescriptorPages * dpNumPerPage))
    {
		NTV2_MSG_ERROR("%s%d:%s%d: dmaXlnxStreamBuild more segments %d than descriptors %d\n", 
					   DMA_MSG_ENGINE, sgCount, pDmaEngine->numDescriptorPages * dpNumPerPage);
		pDmaEngine->programErrorCount++;
		return -EPERM;
    }

    for (sgIndex = 0; sgIndex < sgCount; sgIndex++)
    {
        descSystemAddress = dmaSgAddress(pPageBuffer, sgIndex);
        descCardAddress = 0;
        descTransferSize = dmaSgLength(pPageBuffer, sgIndex);
        
        // xlnx can fetch up to 16 descriptors at once if they are contiguous and do not span pages
        contigCount = dpNumPerPage - dpIndex - 1;
        if (contigCount > 0)
        {
            contigCount--;
        }
        if (contigCount > XLNX_MAX_ADJACENT_COUNT)
        {
            contigCount = XLNX_MAX_ADJACENT_COUNT;
        }

        // write descriptor
        pDescriptor->ulControl = valControl | (contigCount << 8);
        pDescriptor->ulTransferCount = descTransferSize;
        if (xlnxC2H)
        {
            pDescriptor->llDstAddress = descSystemAddress;
            pDescriptor->llSrcAddress = descCardAddress;
        }
        else
        {
            pDescriptor->llSrcAddress = descSystemAddress;
            pDescriptor->llDstAddress = descCardAddress;
        }

        // setup for next segment descriptor
        pDescriptorLast = pDescriptor;
        descSystemLast = descSystemAddress;
        descTransferLast = descTransferSize;

        dpIndex++;
        if (dpIndex < dpNumPerPage)
        {
            pDescriptor++;
            physDescriptor += sizeof(DMA_DESCRIPTOR64);
        }
        else
        {
            dpIndex = 0;
            dpPage++;
            if (dpPage >= pDmaEngine->numDescriptorPages)
            {
                dpPage = 0;
                NTV2_MSG_DESCRIPTOR("%s%d:%s%d: dmaXlnxStreamBuild descriptor list wrap\n",
                                    DMA_MSG_ENGINE);
            }
            pDescriptor = (PXLNX_DESCRIPTOR)pDmaEngine->pDescriptorVirtual[dpPage];
            physDescriptor = pDmaEngine->descriptorPhysical[dpPage];
        }
        pDescriptorLast->llNextAddress = physDescriptor;

        NTV2_MSG_DESCRIPTOR("%s%d:%s%d: dmaXlnxStreamBuild idx %05d con %08x cnt %08x src %016llx dst %016llx nxt %016llx\n",
                            DMA_MSG_ENGINE, index + descriptorCount, pDescriptorLast->ulControl, pDescriptorLast->ulTransferCount,
                            pDescriptorLast->llSrcAddress, pDescriptorLast->llDstAddress, pDescriptorLast->llNextAddress);

        programBytes += descTransferSize;
        descriptorCount++;
        if (descriptorCount > pDmaEngine->maxDescriptors)
        {
            pDmaEngine->programErrorCount++;
            NTV2_MSG_ERROR("%s%d:%s%d: dmaXlnxStreamBuild exceeded max descriptor count %d\n",
                           DMA_MSG_ENGINE, descriptorCount);
            return -EPERM;
        }
    }

	// reset final contig counts
	if (((unsigned long)pDescriptorLast & (unsigned long)dpPageMask) != 0)
	{
		pDescriptor = pDescriptorLast - 1;
		for (dsIndex = 0; dsIndex < XLNX_MAX_ADJACENT_COUNT; dsIndex++)
		{
			pDescriptor->ulControl = valControl;
			NTV2_MSG_DESCRIPTOR("%s%d:%s%d: dmaXlnxStreamBuild con %08x cnt %08x src %016llx dst %016llx nxt %016llx clear adjacent\n",
								DMA_MSG_ENGINE, pDescriptor->ulControl, pDescriptor->ulTransferCount,
								pDescriptor->llSrcAddress, pDescriptor->llDstAddress, pDescriptor->llNextAddress);
			if (((unsigned long)pDescriptor & (unsigned long)dpPageMask) == 0)
				break;
			pDescriptor--;
		}
	}

	// last descriptor generates interrupt
	pDescriptorLast->ulControl = XLNX_CONTROL_DESC_COMPLETION | XLNX_CONTROL_DESC_MAGIC;
	// no next descriptor
	pDescriptorLast->llNextAddress = 0;

	NTV2_MSG_DESCRIPTOR("%s%d:%s%d: dmaXlnxStreamBuild con %08x cnt %08x src %016llx dst %016llx nxt %016llx repeat last\n",
						DMA_MSG_ENGINE, pDescriptorLast->ulControl, pDescriptorLast->ulTransferCount,
						pDescriptorLast->llSrcAddress, pDescriptorLast->llDstAddress, pDescriptorLast->llNextAddress);

	// need at least one descriptor
	if (descriptorCount == 0)
	{
		pDmaEngine->programErrorCount++;
		NTV2_MSG_ERROR("%s%d:%s%d: dmaXlnxStreamBuild no descriptors generated\n", DMA_MSG_ENGINE);
		return -EPERM;
	}

    return (int)descriptorCount;
}

static int dmaXlnxStreamLink(PDMA_ENGINE pDmaEngine, uint32_t descIndex, uint32_t nextIndex)
{
    PXLNX_DESCRIPTOR pDescriptor = NULL;
    ULWord srcPage = 0;
    ULWord srcIndex = 0;
    ULWord dstPage = 0;
    ULWord dstIndex = 0;
    
    NTV2_MSG_PROGRAM("%s%d:%s%d: dmaXlnxStreamLink() descriptor index %d  next index %d\n",
                     DMA_MSG_ENGINE, descIndex, nextIndex);

	if (!pDmaEngine->dmaStream)
	{
		NTV2_MSG_PROGRAM("%s%d:%s%d: dmaXlnxStreamLink() not a streaming engine\n", DMA_MSG_ENGINE);
		return -EPERM;
	}

    if (dmaXlnxDescIndexToPage(descIndex, &srcPage, &srcIndex) != 0)
        return -EPERM;

    if (dmaXlnxDescIndexToPage(nextIndex, &dstPage, &dstIndex) != 0)
        return -EPERM;

    pDescriptor = ((PXLNX_DESCRIPTOR)pDmaEngine->pDescriptorVirtual[srcPage]) + srcIndex;
    pDescriptor->llNextAddress = pDmaEngine->descriptorPhysical[dstPage] + (dstIndex * sizeof(XLNX_DESCRIPTOR));

    NTV2_MSG_DESCRIPTOR("%s%d:%s%d: dmaXlnxStreamLink con %08x cnt %08x src %016llx dst %016llx nxt %016llx\n",
                        DMA_MSG_ENGINE, pDescriptor->ulControl, pDescriptor->ulTransferCount,
                        pDescriptor->llSrcAddress, pDescriptor->llDstAddress, pDescriptor->llNextAddress);
    return 0;
}

static int dmaXlnxStreamStart(PDMA_ENGINE pDmaEngine, uint32_t startIndex)
{
	ULWord 	deviceNumber = pDmaEngine->deviceNumber;
	ULWord	xlnxIndex = pDmaEngine->dmaIndex;
	bool	xlnxC2H = pDmaEngine->dmaC2H;
    ULWord  startPage = 0;
    ULWord  startCount = 0;
    ULWord64 startAddress = 0;

    NTV2_MSG_PROGRAM("%s%d:%s%d: dmaXlnxStreamStart() start index %d\n", DMA_MSG_ENGINE, startIndex);

	if (!pDmaEngine->dmaStream)
	{
		NTV2_MSG_PROGRAM("%s%d:%s%d: dmaXlnxStreamStart() not a streaming engine\n", DMA_MSG_ENGINE);
		return -EPERM;
	}

    if (dmaXlnxDescIndexToPage(startIndex, &startPage, &startCount) != 0)
        return -EPERM;
    startAddress = pDmaEngine->descriptorPhysical[startPage] + (startCount * sizeof(XLNX_DESCRIPTOR));

	// write descriptor start
	WriteXlnxDmaEngineStartLow(deviceNumber, xlnxC2H, xlnxIndex, (ULWord)(startAddress & 0xffffffff));
	WriteXlnxDmaEngineStartHigh(deviceNumber, xlnxC2H, xlnxIndex, (ULWord)(startAddress >> 32));
	WriteXlnxDmaEngineStartAdjacent(deviceNumber, xlnxC2H, xlnxIndex, 0);

	// count the program starts
	pDmaEngine->programStartCount++;
	pDmaEngine->programStartTime = ntv2Time100ns();

	// start dma
	ClearXlnxDmaStatus(deviceNumber, xlnxC2H, xlnxIndex);
	EnableXlnxDmaInterrupt(deviceNumber, xlnxC2H, xlnxIndex);
	StartXlnxDma(deviceNumber,xlnxC2H, xlnxIndex);

	return 0;
}

int dmaXlnxStreamStop(PDMA_ENGINE pDmaEngine)
{
	ULWord 	deviceNumber = pDmaEngine->deviceNumber;
	ULWord	xlnxIndex = pDmaEngine->dmaIndex;
	bool	xlnxC2H = pDmaEngine->dmaC2H;

    NTV2_MSG_PROGRAM("%s%d:%s%d: dmaXlnxStreamStop()\n", DMA_MSG_ENGINE);

	if (!pDmaEngine->dmaStream)
	{
		NTV2_MSG_PROGRAM("%s%d:%s%d: dmaXlnxStreamStop() not a streaming engine\n", DMA_MSG_ENGINE);
		return -EPERM;
	}

	// disable the interrupt
	DisableXlnxDmaInterrupt(deviceNumber, xlnxC2H, xlnxIndex);
	// stop the dma (should be reset)
	StopXlnxDma(deviceNumber, xlnxC2H, xlnxIndex);
	// wait for engine stop (can take a couple of register reads)
	WaitXlnxDmaActive(deviceNumber, xlnxC2H, xlnxIndex, false);

	return 0;
}

static int dmaXlnxDescIndexToPage(uint32_t index, uint32_t* pageIndex, uint32_t* descIndex)
{
    uint32_t descPage = PAGE_SIZE / sizeof(XLNX_DESCRIPTOR);
    uint32_t numDesc = DMA_DESCRIPTOR_PAGES_MAX * descPage;
    
    if (index >= numDesc)
    {
        *pageIndex = 0;
        *descIndex = 0;
        return -ENOMEM;
    }

    *pageIndex = index / descPage;
    *descIndex = index % descPage;

    return 0;
}

static bool dmaVideoSegmentInit(PDMA_CONTEXT pDmaContext, PDMA_VIDEO_SEGMENT pDmaSegment)
{
	if (pDmaSegment == NULL) return false;

	memset(pDmaSegment, 0, sizeof(DMA_VIDEO_SEGMENT));

	NTV2_MSG_VIDEO_SEGMENT("%s%d:%s%d:%s%d: dmaVideoSegmentInit initialize\n", DMA_MSG_CONTEXT);
	return true;
}

static bool dmaVideoSegmentConfig(PDMA_CONTEXT pDmaContext,
								  PDMA_VIDEO_SEGMENT pDmaSegment,
								  ULWord cardAddress,
								  ULWord cardSize,
								  ULWord cardPitch,
								  ULWord systemPitch,
								  ULWord segmentSize,
								  ULWord segmentCount,
								  bool invertImage)
{
	if (pDmaSegment == NULL) return false;

	pDmaSegment->cardAddress = cardAddress;
	pDmaSegment->cardSize = cardSize;
	pDmaSegment->cardPitch = cardPitch;
	pDmaSegment->systemPitch = systemPitch;
	pDmaSegment->segmentSize = segmentSize;
	pDmaSegment->segmentCount = segmentCount;
	pDmaSegment->invertImage = invertImage;
	pDmaSegment->transferAddress = 0;
	pDmaSegment->transferSize = 0;
	pDmaSegment->systemOffset = 0;
	pDmaSegment->segmentIndex = 0;

	NTV2_MSG_VIDEO_SEGMENT("%s%d:%s%d:%s%d: dmaVideoSegmentConfig cardAdd %08x  cardSize %d sysPitch %d  cardPitch %d  segSize %d  segCount %d  invertImage %d\n",
						   DMA_MSG_CONTEXT,
						   cardAddress, cardSize, systemPitch, cardPitch, 
						   segmentSize, segmentCount, invertImage);

	return true;
}

static inline bool dmaVideoSegmentTransfer(PDMA_CONTEXT pDmaContext,
										   PDMA_VIDEO_SEGMENT pDmaSegment,
										   ULWord64 transferAddress, 
										   ULWord transferSize)
{
	if (pDmaSegment == NULL) return false;

	NTV2_MSG_VIDEO_SEGMENT("%s%d:%s%d:%s%d: dmaVideoSegmentTransfer address %016llx  size %d\n",
						   DMA_MSG_CONTEXT, transferAddress, transferSize);
	
	if (transferSize > pDmaSegment->cardSize)
		transferSize = pDmaSegment->cardSize;

	// track system transfer offset
	pDmaSegment->systemOffset += pDmaSegment->transferSize;
	if (pDmaSegment->systemOffset >= pDmaSegment->cardSize) return false;

	// limit transfer to system buffer size
	if ((pDmaSegment->systemOffset + transferSize) > pDmaSegment->cardSize)
	{
		transferSize = pDmaSegment->cardSize - pDmaSegment->systemOffset;
	}

	// record mapped segment info
	pDmaSegment->transferAddress = transferAddress;
	pDmaSegment->transferSize = transferSize;
	pDmaSegment->segmentIndex = 0;

	// compute min overlap transfer segment index (optimization)
	if ((pDmaSegment->systemOffset > pDmaSegment->segmentSize) && (pDmaSegment->systemPitch > 0))
	{
		pDmaSegment->segmentIndex = (pDmaSegment->systemOffset - pDmaSegment->segmentSize)/pDmaSegment->systemPitch;
	}

//	NTV2_MSG_VIDEO_SEGMENT("%s%d:%s%d:%s%d: dmaVideoSegmentTransfer address %016llx  size %d\n",
//						   DMA_MSG_CONTEXT, transferAddress, transferSize);
	return true;
}

static inline bool dmaVideoSegmentDescriptor(PDMA_CONTEXT pDmaContext,
											 PDMA_VIDEO_SEGMENT pDmaSegment,
											 ULWord64* pSystemAddress, 
											 ULWord* pCardAddress, 
											 ULWord* pDescriptorSize)
{
	ULWord cardOffset = 0;
	ULWord segOffset = 0;
	ULWord segSize = 0;
	ULWord trnOffset = 0;
	ULWord trnSize = 0;
	ULWord64 systemAddress = 0;
	ULWord cardAddress = 0;
	ULWord descSize = 0;

	if ((pDmaSegment == NULL) ||
		(pSystemAddress == NULL) ||
		(pCardAddress == NULL) ||
		(pDescriptorSize == NULL)) return false;

	// optimization for 1 segment
	if (pDmaSegment->segmentCount == 1)
	{
		if (pDmaSegment->segmentIndex > 0)
			return false;

		pDmaSegment->segmentIndex++;
		*pSystemAddress = pDmaSegment->transferAddress;
		*pCardAddress = pDmaSegment->cardAddress + pDmaSegment->systemOffset;
		*pDescriptorSize = pDmaSegment->transferSize;
		NTV2_MSG_VIDEO_SEGMENT("%s%d:%s%d:%s%d: dmaVideoSegmentDescriptor sys %016llx  card %08x  size %d\n",
							   DMA_MSG_CONTEXT,
							   pDmaSegment->transferAddress,
							   pDmaSegment->systemOffset,
							   pDmaSegment->transferSize);
		return true;
	}

	// check for all segments transferred
	while (pDmaSegment->segmentIndex < pDmaSegment->segmentCount)
	{
		// calculate segment card offset
		if (pDmaSegment->invertImage)
		{
			cardOffset = (pDmaSegment->segmentCount - pDmaSegment->segmentIndex - 1)*pDmaSegment->cardPitch;
		}
		else
		{
			cardOffset = pDmaSegment->segmentIndex*pDmaSegment->cardPitch;
		}

		// calculate segment system offset
		segOffset = pDmaSegment->segmentIndex*pDmaSegment->systemPitch;

		// get segment size
		segSize = pDmaSegment->segmentSize;

		// get current transfer offset in system buffer
		trnOffset = pDmaSegment->systemOffset;

		// get current transfer size
		trnSize = pDmaSegment->transferSize;

		// generate descriptor if system segment overlaps with current transfer
		if (segOffset < trnOffset)
		{
			if (trnOffset < (segOffset + segSize))
			{
				// transfer starts at system transfer address
				systemAddress = pDmaSegment->transferAddress;  
				// calculate card address at transfer offset in segment
				cardAddress = pDmaSegment->cardAddress + cardOffset + (trnOffset - segOffset);  
				// assume descriptor size to end of segment
				descSize = segSize - (trnOffset - segOffset);
				// if segment size to big use transfer size
				if (descSize > trnSize)
				{
					descSize = trnSize;                              
				}
			}
		}
		else
		{
			if (segOffset < (trnOffset + trnSize))
			{
				// calculate system address at segment start
				systemAddress = pDmaSegment->transferAddress + (segOffset - trnOffset);
				// transfer starts at card segment address
				cardAddress =  pDmaSegment->cardAddress + cardOffset;
				// assume descriptor size to end of transfer
				descSize = trnSize - (segOffset - trnOffset);
				// if transfer size to big use segment size
				if (descSize > segSize)
				{
					descSize = segSize;
				}
			}
			else
			{
				// segment index is beyond the transfer buffer (optimization)
				break;                                              
			}
		}

		// move to next segment
		pDmaSegment->segmentIndex++;

		// transfer overlap
		if (descSize > 0)
		{
			*pSystemAddress = systemAddress;
			*pCardAddress = cardAddress;
			*pDescriptorSize = descSize;
			NTV2_MSG_VIDEO_SEGMENT("%s%d:%s%d:%s%d: dmaVideoSegmentDescriptor sys %016llx  card %08x  size %d\n",
								   DMA_MSG_CONTEXT, systemAddress, cardAddress, descSize);
			return true;
		}
	}

	NTV2_MSG_VIDEO_SEGMENT("%s%d:%s%d:%s%d: dmaVideoSegmentDescriptor segment complete\n",
						   DMA_MSG_CONTEXT);

	// need another transfer
	return false;
}

static bool dmaAudioSegmentInit(PDMA_CONTEXT pDmaContext, PDMA_AUDIO_SEGMENT pDmaSegment)
{
	if (pDmaSegment == NULL) return false;

	memset(pDmaSegment, 0, sizeof(DMA_AUDIO_SEGMENT));

	NTV2_MSG_AUDIO_SEGMENT("%s%d:%s%d:%s%d: dmaAudioSegmentInit initialize\n", DMA_MSG_CONTEXT);
	return true;
}

static bool dmaAudioSegmentConfig(PDMA_CONTEXT pDmaContext,
								  PDMA_AUDIO_SEGMENT pDmaSegment,
								  ULWord systemSize,
								  ULWord transferSize,
								  ULWord* pRingAddress,
								  ULWord ringSize,
								  ULWord audioStart)
{
	ULWord i = 0;
	if (pDmaSegment == NULL) return false;
	if (audioStart > ringSize)
	{
		NTV2_MSG_AUDIO_SEGMENT("%s%d:%s%d:%s%d: dmaAudioSegmentConfig audioStart(%08x) > ringSize(%08x)\n",
							   DMA_MSG_CONTEXT, audioStart, ringSize);
		return false;
	}
	
	pDmaSegment->systemSize = systemSize;
	pDmaSegment->transferSize = transferSize;
	pDmaSegment->ringCount = 0;
	for (i = 0; i < MAX_NUM_AUDIO_LINKS; i++)
	{
		if (pRingAddress[i] == 0)
			break;
		pDmaSegment->ringAddress[i] = pRingAddress[i];
		pDmaSegment->ringCount++;
	}

	pDmaSegment->ringSize = ringSize;
	pDmaSegment->audioStart = audioStart;
	pDmaSegment->audioSize = transferSize /  pDmaSegment->ringCount;
	pDmaSegment->pageAddress = 0;
	pDmaSegment->pageSize = 0;
	pDmaSegment->ringIndex = 0;
	pDmaSegment->systemOffset = 0;
	pDmaSegment->pageOffset = 0;
	pDmaSegment->audioOffset = 0;

	NTV2_MSG_AUDIO_SEGMENT("%s%d:%s%d:%s%d: dmaAudioSegmentConfig ring address %08x  count %d size %d  audio offset %08x  size %d\n", 
						   DMA_MSG_CONTEXT, pRingAddress[0], pDmaSegment->ringCount, pDmaSegment->ringSize = ringSize, pDmaSegment->audioStart, pDmaSegment->audioSize);
	return true;
}

static inline bool dmaAudioSegmentTransfer(PDMA_CONTEXT pDmaContext,
										   PDMA_AUDIO_SEGMENT pDmaSegment,
										   ULWord64 pageAddress, 
										   ULWord pageSize)
{
	if (pDmaSegment == NULL) return false;
	if (pageSize >= pDmaSegment->ringSize) return false;

	// track system transfer offset
	pDmaSegment->systemOffset += pDmaSegment->pageSize;
	if (pDmaSegment->systemOffset >= pDmaSegment->systemSize) return false;

	// record new transfer info
	pDmaSegment->pageAddress = pageAddress;
	pDmaSegment->pageSize = pageSize;
	pDmaSegment->pageOffset = 0;

	NTV2_MSG_AUDIO_SEGMENT("%s%d:%s%d:%s%d: dmaAudioSegmentTransfer address %016llx  size %d\n",
						   DMA_MSG_CONTEXT, pageAddress, pageSize);
	return true;
}

static inline bool dmaAudioSegmentDescriptor(PDMA_CONTEXT pDmaContext,
											 PDMA_AUDIO_SEGMENT pDmaSegment,
											 ULWord64* pSystemAddress, 
											 ULWord* pCardAddress, 
											 ULWord* pDescriptorSize)
{
	ULWord64 systemAddress = 0;
	ULWord cardAddress = 0;
	ULWord cardOffset = 0;
	ULWord descSize = 0;

	if ((pDmaSegment == NULL) ||
		(pSystemAddress == NULL) ||
		(pCardAddress == NULL) ||
		(pDescriptorSize == NULL)) return false;

	// check for done with this transfer
	if (pDmaSegment->pageOffset >= pDmaSegment->pageSize) 
	{
		NTV2_MSG_AUDIO_SEGMENT("%s%d:%s%d:%s%d: dmaAudioSegmentDescriptor segment complete\n",
							   DMA_MSG_CONTEXT);
		return false;
	}
	
	// check for done with this ring
	if (pDmaSegment->audioOffset >= pDmaSegment->audioSize)
	{
		NTV2_MSG_AUDIO_SEGMENT("%s%d:%s%d:%s%d: audio complete\n",
							   DMA_MSG_CONTEXT);
		pDmaSegment->ringIndex++;
		if (pDmaSegment->ringIndex >= pDmaSegment->ringCount)
			return false;
		pDmaSegment->audioOffset = 0;
	}

	// compute system address for this descriptor
	systemAddress = pDmaSegment->pageAddress + pDmaSegment->pageOffset;
	
	// compute card offset into ring buffer
	cardOffset = pDmaSegment->audioStart + pDmaSegment->audioOffset;
	
	// compute descriptor size for transfer
	descSize = pDmaSegment->pageSize - pDmaSegment->pageOffset;
	
	// limit descriptor size to transfer size for this ring
	if ((pDmaSegment->audioOffset + descSize) > pDmaSegment->audioSize)
		descSize = pDmaSegment->audioSize - pDmaSegment->audioOffset;
		
	// check for ring wrap
	if (cardOffset >= pDmaSegment->ringSize)
	{
		cardAddress = pDmaSegment->ringAddress[pDmaSegment->ringIndex] + (cardOffset - pDmaSegment->ringSize);
	}
	else if ((cardOffset + descSize) >= pDmaSegment->ringSize)
	{
		cardAddress = pDmaSegment->ringAddress[pDmaSegment->ringIndex] + cardOffset;
		descSize = pDmaSegment->ringSize - cardOffset;
	}
	else
	{
		cardAddress = pDmaSegment->ringAddress[pDmaSegment->ringIndex] + cardOffset;
	}
	
	// this should not happen
	if (descSize == 0) return false;

	// increment the card offset
	pDmaSegment->pageOffset += descSize;
	pDmaSegment->audioOffset += descSize;

	// return a descriptor
	*pSystemAddress = systemAddress;
	*pCardAddress = cardAddress;
	*pDescriptorSize = descSize;

	NTV2_MSG_AUDIO_SEGMENT("%s%d:%s%d:%s%d: dmaAudioSegmentDescriptor sys %016llx  card %08x  size %d\n",
						   DMA_MSG_CONTEXT, systemAddress, cardAddress, descSize);
	return true;
}

static bool dmaAncSegmentInit(PDMA_CONTEXT pDmaContext, PDMA_ANC_SEGMENT pDmaSegment)
{
	if (pDmaSegment == NULL) return false;

	memset(pDmaSegment, 0, sizeof(DMA_ANC_SEGMENT));

	NTV2_MSG_ANC_SEGMENT("%s%d:%s%d:%s%d: dmaAncSegmentInit initialize\n", DMA_MSG_CONTEXT);

	return true;
}

static bool dmaAncSegmentConfig(PDMA_CONTEXT pDmaContext,
								PDMA_ANC_SEGMENT pDmaSegment,
								ULWord ancAddress,
								ULWord ancSize)
{
	if (pDmaSegment == NULL) return false;

	pDmaSegment->ancAddress = ancAddress;
	pDmaSegment->ancSize = ancSize;
	pDmaSegment->transferAddress = 0;
	pDmaSegment->transferSize = 0;
	pDmaSegment->systemOffset = 0;
	pDmaSegment->transferOffset = 0;

	NTV2_MSG_ANC_SEGMENT("%s%d:%s%d:%s%d: dmaAncSegmentConfig cardAddress %08x  cardSize %d\n", 
						 DMA_MSG_CONTEXT, ancAddress, ancSize);
	return true;
}

static inline bool dmaAncSegmentTransfer(PDMA_CONTEXT pDmaContext,
										 PDMA_ANC_SEGMENT pDmaSegment,
										 ULWord64 transferAddress, 
										 ULWord transferSize)
{
	if (pDmaSegment == NULL) return false;

	// track system transfer offset
	pDmaSegment->systemOffset += pDmaSegment->transferSize;
	if (pDmaSegment->systemOffset >= pDmaSegment->ancSize) return false;

	// limit transfer to system buffer size
	if ((pDmaSegment->systemOffset + transferSize) > pDmaSegment->ancSize)
	{
		transferSize = pDmaSegment->ancSize - pDmaSegment->systemOffset;
	}

	// record mapped segment info
	pDmaSegment->transferAddress = transferAddress;
	pDmaSegment->transferSize = transferSize;
	pDmaSegment->transferOffset = 0;

	NTV2_MSG_ANC_SEGMENT("%s%d:%s%d:%s%d: dmaAncSegmentTransfer address %016llx  size %d\n",
						 DMA_MSG_CONTEXT, transferAddress, transferSize);
	return true;
}

static inline bool dmaAncSegmentDescriptor(PDMA_CONTEXT pDmaContext,
										   PDMA_ANC_SEGMENT pDmaSegment,
										   ULWord64* pSystemAddress, 
										   ULWord* pCardAddress, 
										   ULWord* pDescriptorSize)
{
	ULWord64 systemAddress = 0;
	ULWord cardAddress = 0;
	ULWord descSize = 0;

	if ((pDmaSegment == NULL) ||
		(pSystemAddress == NULL) ||
		(pCardAddress == NULL) ||
		(pDescriptorSize == NULL)) return false;

	// check for done with this transfer
	if (pDmaSegment->transferOffset >= pDmaSegment->transferSize) 
	{
		NTV2_MSG_ANC_SEGMENT("%s%d:%s%d:%s%d: dmaAncSegmentDescriptor segment complete\n",
							 DMA_MSG_CONTEXT);
		return false;
	}

	// compute system address for this descriptor
	systemAddress = pDmaSegment->transferAddress + pDmaSegment->transferOffset;
	// compute card address
	cardAddress = pDmaSegment->ancAddress + pDmaSegment->systemOffset + pDmaSegment->transferOffset;
	// compute descriptor size
	descSize = pDmaSegment->transferSize - pDmaSegment->transferOffset;

	// this should not happen
	if (descSize == 0) return false;

	// increment the card offset
	pDmaSegment->transferOffset += descSize;

	// return a descriptor
	*pSystemAddress = systemAddress;
	*pCardAddress = cardAddress;
	*pDescriptorSize = descSize;

	NTV2_MSG_ANC_SEGMENT("%s%d:%s%d:%s%d: dmaAncSegmentDescriptor sys %016llx  card %08x  size %d\n",
						 DMA_MSG_CONTEXT, systemAddress, cardAddress, descSize);
	return true;
}

static uint32_t microsecondsToJiffies(int64_t timeout)
{
	return (uint32_t)((timeout + (1000000/HZ - 1)) * HZ / 1000000);
}

