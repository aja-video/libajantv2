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
#include <linux/dmaengine.h>
#include <linux/pagemap.h>
#include <linux/of_device.h>

#include "ntv2enums.h"
#include "ntv2audiodefines.h"
#include "ntv2videodefines.h"

#include "ntv2publicinterface.h"
#include "ntv2linuxpublicinterface.h"
#include "ntv2devicefeatures.h"

#include "registerio.h"
#include "ntv2dma.h"

/* Register Offsets */
#define ZYNQMP_ERR_CTRL         0x000
#define ZYNQMP_DMA_ISR			0x100
#define ZYNQMP_DMA_IMR			0x104
#define ZYNQMP_DMA_IER			0x108
#define ZYNQMP_DMA_IDS			0x10C
#define ZYNQMP_DMA_CTRL0		0x110
#define ZYNQMP_DMA_CTRL1		0x114
#define ZYNQMP_DMA_CH_FC1       0x118
#define ZYNQMP_DMA_CH_STATUS	0x11C
#define ZYNQMP_DMA_DATA_ATTR		0x120
#define ZYNQMP_DMA_DSCR_ATTR		0x124
#define ZYNQMP_DMA_SRC_DSCR_WRD0	0x128
#define ZYNQMP_DMA_SRC_DSCR_WRD1	0x12C
#define ZYNQMP_DMA_SRC_DSCR_WRD2	0x130
#define ZYNQMP_DMA_SRC_DSCR_WRD3	0x134
#define ZYNQMP_DMA_DST_DSCR_WRD0	0x138
#define ZYNQMP_DMA_DST_DSCR_WRD1	0x13C
#define ZYNQMP_DMA_DST_DSCR_WRD2	0x140
#define ZYNQMP_DMA_DST_DSCR_WRD3	0x144
#define ZYNQMP_DMA_SRC_START_LSB	0x158
#define ZYNQMP_DMA_SRC_START_MSB	0x15C
#define ZYNQMP_DMA_DST_START_LSB	0x160
#define ZYNQMP_DMA_DST_START_MSB	0x164
#define ZYNQMP_DMA_TOTAL_BYTE		0x188
#define ZYNQMP_DMA_RATE_CTRL		0x18C
#define ZYNQMP_DMA_IRQ_SRC_ACCT		0x190
#define ZYNQMP_DMA_IRQ_DST_ACCT		0x194
#define ZYNQMP_DMA_CTRL2		0x200

#define ZDMA_NOSHIFT    0x0
#define ZDMA_NOMASK     0xffffffff

/* rate control constants */
#define ZDMA_RATE_CONTROL_DEFAULT       0x000
#define ZDMA_RATE_CONTROL_OFFSET        0x18C
#define ZDMA_RATE_CONTROL_SHIFT         0
#define ZDMA_RATE_CONTROL_MASK          0xFFF

#define ZDMA_CH_CTRL0_OFFSET            0x110
#define ZDMA_CH_CTRL0_SHIFT             3
#define ZDMA_CH_CTRL0_MASK              0x1

/* echo 32 > /sys/devices/platform/ntv2device0/zynq/zdma_rate_control */

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
//	NTV2_DEBUG_TRANSFER |
//	NTV2_DEBUG_PAGE_MAP |
//	NTV2_DEBUG_PROGRAM |
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

#define DMA_STATISTICS_INTERVAL			5000000	// us
#define DMA_TRANSFER_TIMEOUT			5000000

// hack this function back into the zynqmp_dma.c
extern struct dma_async_tx_descriptor *zynqmp_dma_prep_sg(
	struct dma_chan *dchan, struct scatterlist *dst_sg,
	unsigned int dst_sg_len, struct scatterlist *src_sg,
	unsigned int src_sg_len, unsigned long flags);

PDMA_ENGINE getDmaEngine(ULWord deviceNumber, ULWord engIndex);
static PDMA_CONTEXT getDmaContext(ULWord deviceNumber, ULWord engIndex, ULWord conIndex);

static bool dmaHardwareOpen(PDMA_ENGINE pDmaEngine);
static void dmaHardwareClose(PDMA_ENGINE pDmaEngine);

static void dmaFreeEngine(PDMA_ENGINE pDmaEngine);
static PDMA_ENGINE dmaMapEngine(ULWord deviceNumber, NTV2DMAEngine eDMAEngine, bool bToHost);
static void dmaStatistics(PDMA_ENGINE pDmaEngine, bool dmaC2H);

static void dmaEngineLock(PDMA_ENGINE pDmaEngine);
static void dmaEngineUnlock(PDMA_ENGINE pDmaEngine);
static PDMA_CONTEXT dmaContextAcquire(PDMA_ENGINE pDmaEngine, ULWord timeout);
static void dmaContextRelease(PDMA_CONTEXT pDmaContext);
int dmaHardwareAcquire(PDMA_ENGINE pDmaEngine, ULWord timeout);
void dmaHardwareRelease(PDMA_ENGINE pDmaEngine);
static int dmaSerialAcquire(ULWord deviceNumber, ULWord timeout);
static void dmaSerialRelease(ULWord deviceNumber);

static inline bool dmaPageRootAutoLock(PDMA_PAGE_ROOT pRoot);
static inline bool dmaPageRootAutoMap(PDMA_PAGE_ROOT pRoot);

static int dmaPageBufferInit(ULWord deviceNumber, PDMA_PAGE_BUFFER pBuffer, ULWord numPages);
static void dmaPageBufferRelease(ULWord deviceNumber, PDMA_PAGE_BUFFER pBuffer);
static int dmaPageLock(ULWord deviceNumber, PDMA_PAGE_BUFFER pBuffer,
					   PVOID pAddress, ULWord size, ULWord direction);
static void dmaPageUnlock(ULWord deviceNumber, PDMA_PAGE_BUFFER pBuffer);
static int dmaBusMap(ULWord deviceNumber, PDMA_PAGE_BUFFER pBuffer,
					 ULWord64 videoBusAddress, ULWord videoBusSize);
static int dmaSgMap(ULWord deviceNumber, PDMA_PAGE_BUFFER pBuffer, struct device* pSgDevice);
static void dmaSgUnmap(ULWord deviceNumber, PDMA_PAGE_BUFFER pBuffer);
static void dmaSgDevice(ULWord deviceNumber, PDMA_PAGE_BUFFER pBuffer);
static void dmaSgHost(ULWord deviceNumber, PDMA_PAGE_BUFFER pBuffer);

static inline bool dmaPageLocked(PDMA_PAGE_BUFFER pBuffer);
static inline bool dmaSgMapped(PDMA_PAGE_BUFFER pBuffer);
static inline struct scatterlist* dmaSgList(PDMA_PAGE_BUFFER pBuffer);
static inline ULWord dmaSgCount(PDMA_PAGE_BUFFER pBuffer);

static int dmaZynqProgram(PDMA_CONTEXT pDmaContext);
static void dmaZynqAbort(PDMA_ENGINE pDmaEngine);
static void dmaZynqVideoCallback(void* arg);
static void dmaZynqAudioCallback(void* arg);
static void dmaZynqAncF1Callback(void* arg);
static void dmaZynqAncF2Callback(void* arg);

static inline uint32_t microsecondsToJiffies(int64_t timeout);
static int64_t timeMicro(void);

static uint32_t ofpd_ioread32(uint64_t addr, uint32_t shift, uint32_t mask)
{
    void *ioaddr = ioremap(addr, sizeof(uint32_t));
    uint32_t reg = (ioread32(ioaddr) >> shift) & mask;
    iounmap(ioaddr);
    return reg;
}

static uint32_t ofpd_iormw32(uint64_t addr, uint32_t value, uint32_t shift, uint32_t mask)
{
    void *ioaddr = ioremap(addr, sizeof(uint32_t));
    uint32_t ret = ioread32(ioaddr);
    ret &= ~(mask << shift);
    ret |= ((value & mask) << shift);
    iowrite32(ret, ioaddr);
    iounmap(ioaddr);
    return ret;
}

static uint32_t ofpd_zdma_read_rate_control(PDMA_ENGINE pDmaEngine)
{
    if(pDmaEngine == NULL) return 0;
    if(!pDmaEngine->zdma_addr) return 0;
    return ofpd_ioread32(pDmaEngine->zdma_addr + ZDMA_RATE_CONTROL_OFFSET, ZDMA_RATE_CONTROL_SHIFT, ZDMA_RATE_CONTROL_MASK);
}

static void ofpd_zdma_write_rate_control(PDMA_ENGINE pDmaEngine, uint32_t value)
{
    if(pDmaEngine == NULL) return;
    if(!pDmaEngine->zdma_addr) return;
    ofpd_iormw32(pDmaEngine->zdma_addr + ZDMA_RATE_CONTROL_OFFSET, value, ZDMA_RATE_CONTROL_SHIFT, ZDMA_RATE_CONTROL_MASK);
    NTV2_MSG_INFO("%s%d:%s%d: zynq dma rate control %d\n", DMA_MSG_ENGINE, value);
    return;
}

static ssize_t ofpd_zdma_rate_control_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    PDMA_ENGINE pDmaEngine = (PDMA_ENGINE)dev_get_drvdata(dev);
    return (pDmaEngine == NULL) ?
        scnprintf(buf, PAGE_SIZE, "-1\n") :
        scnprintf(buf, PAGE_SIZE, "%u\n", (unsigned int)ofpd_zdma_read_rate_control(pDmaEngine));
}

static ssize_t ofpd_zdma_rate_control_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    PDMA_ENGINE pDmaEngine = (PDMA_ENGINE)dev_get_drvdata(dev);
    u32 value;
    int ret = kstrtou32(buf, 0, &value);
    if(ret) return ret;
    ofpd_zdma_write_rate_control(pDmaEngine, value);
    return size;
}

#define ZDMA_DUMP_REG(REG) NTV2_MSG_INFO("%s%d: reg %30s %8x  %08x\n", DMA_MSG_DEVICE, #REG, REG, \
                                         ofpd_ioread32(regAddress + REG, ZDMA_NOSHIFT, ZDMA_NOMASK));

static void ofpd_zdma_dump_registers(PDMA_ENGINE pDmaEngine)
{
    uint32_t deviceNumber;
    uint64_t regAddress;
    
    if (pDmaEngine == NULL)
        return;

    deviceNumber = pDmaEngine->deviceNumber;
    regAddress = pDmaEngine->zdma_addr;

    if (regAddress == 0)
        return;
    
    ZDMA_DUMP_REG(ZYNQMP_ERR_CTRL);
    ZDMA_DUMP_REG(ZYNQMP_DMA_ISR);
    ZDMA_DUMP_REG(ZYNQMP_DMA_IMR);
    ZDMA_DUMP_REG(ZYNQMP_DMA_IER);
    ZDMA_DUMP_REG(ZYNQMP_DMA_IDS);
    ZDMA_DUMP_REG(ZYNQMP_DMA_CTRL0);
    ZDMA_DUMP_REG(ZYNQMP_DMA_CTRL1);
    ZDMA_DUMP_REG(ZYNQMP_DMA_CH_FC1);
    ZDMA_DUMP_REG(ZYNQMP_DMA_CH_STATUS);
    ZDMA_DUMP_REG(ZYNQMP_DMA_DATA_ATTR);
    ZDMA_DUMP_REG(ZYNQMP_DMA_DSCR_ATTR);
    ZDMA_DUMP_REG(ZYNQMP_DMA_SRC_DSCR_WRD0);
    ZDMA_DUMP_REG(ZYNQMP_DMA_SRC_DSCR_WRD1);
    ZDMA_DUMP_REG(ZYNQMP_DMA_SRC_DSCR_WRD2);
    ZDMA_DUMP_REG(ZYNQMP_DMA_SRC_DSCR_WRD3);
    ZDMA_DUMP_REG(ZYNQMP_DMA_DST_DSCR_WRD0);
    ZDMA_DUMP_REG(ZYNQMP_DMA_DST_DSCR_WRD1);
    ZDMA_DUMP_REG(ZYNQMP_DMA_DST_DSCR_WRD2);
    ZDMA_DUMP_REG(ZYNQMP_DMA_DST_DSCR_WRD3);
    ZDMA_DUMP_REG(ZYNQMP_DMA_SRC_START_LSB);
    ZDMA_DUMP_REG(ZYNQMP_DMA_SRC_START_MSB);
    ZDMA_DUMP_REG(ZYNQMP_DMA_DST_START_LSB);
    ZDMA_DUMP_REG(ZYNQMP_DMA_DST_START_MSB);
    ZDMA_DUMP_REG(ZYNQMP_DMA_TOTAL_BYTE);
    ZDMA_DUMP_REG(ZYNQMP_DMA_RATE_CTRL);
    ZDMA_DUMP_REG(ZYNQMP_DMA_IRQ_SRC_ACCT);
    ZDMA_DUMP_REG(ZYNQMP_DMA_IRQ_DST_ACCT);
    ZDMA_DUMP_REG(ZYNQMP_DMA_CTRL2);
}

static DEVICE_ATTR(zdma_rate_control, S_IWUSR | S_IRUGO, ofpd_zdma_rate_control_show, ofpd_zdma_rate_control_store);

int dmaInit(ULWord deviceNumber)
{
	NTV2PrivateParams *pNTV2Params = getNTV2Params(deviceNumber);
	ULWord deviceID = pNTV2Params->_DeviceID;
	ULWord maxVideoSize;
	ULWord maxVideoPages;
	ULWord iEng;
	ULWord iCon;
	
	if (pNTV2Params->_dmaNumEngines != 0)
	{
		NTV2_MSG_INFO("%s%d: dmaInit called again (ignored)\n", DMA_MSG_DEVICE);
		return 0;
	}

	NTV2_MSG_INFO("%s%d: dmaInit\n", DMA_MSG_DEVICE);

	for (iEng = 0; iEng < DMA_NUM_ENGINES; iEng++)
	{
		memset(&pNTV2Params->_dmaEngine[iEng], 0, sizeof(DMA_ENGINE));
	}

	pNTV2Params->_dmaNumEngines = NTV2DeviceGetNumDMAEngines(deviceID);
	if (pNTV2Params->_dmaNumEngines > DMA_NUM_ENGINES)
		pNTV2Params->_dmaNumEngines = DMA_NUM_ENGINES;

	sema_init(&pNTV2Params->_dmaSerialSemaphore, 1);

	maxVideoSize = HD_MAX_FRAME_SIZE;
	maxVideoPages = HD_MAX_PAGES;
	if (NTV2DeviceCanDo4KVideo(pNTV2Params->_DeviceID))
	{
		maxVideoSize = UHD_MAX_FRAME_SIZE;
		maxVideoPages = UHD_MAX_PAGES;
	}
	if (NTV2DeviceCanDo8KVideo(pNTV2Params->_DeviceID))
	{
		maxVideoSize = UHD2_MAX_FRAME_SIZE;
		maxVideoPages = UHD2_MAX_PAGES;
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
		case DmaMethodZynq:
			pDmaEngine->dmaIndex = pDmaEngine->engIndex;
			pDmaEngine->dmaC2H = false; // not used
			pDmaEngine->engName = "zynq";
			break;
		default:
			pDmaEngine->dmaIndex = 0;
			pDmaEngine->dmaC2H = false; // not used
			pDmaEngine->engName = "unknown";
			NTV2_MSG_ERROR("%s%d:%s%d: dmaInit unsupported dma method %d\n",
						   DMA_MSG_ENGINE, pDmaEngine->dmaMethod);
			pDmaEngine->state = DmaStateDead;
			continue;
		}
		
		// max resources
		pDmaEngine->maxVideoSize = maxVideoSize;
		pDmaEngine->maxVideoPages = maxVideoPages;
		pDmaEngine->maxAudioSize = AUDIO_MAX_SIZE;
		pDmaEngine->maxAudioPages = AUDIO_MAX_PAGES;
		pDmaEngine->maxAncSize = ANC_MAX_SIZE;
		pDmaEngine->maxAncPages = ANC_MAX_PAGES;

		// engine is initialized
		pDmaEngine->engInit = true;
		
		NTV2_MSG_INFO("%s%d:%s%d: dmaInit configure max  vid %d  aud %d  anc %d\n",
					  DMA_MSG_ENGINE, pDmaEngine->maxVideoSize,
					  pDmaEngine->maxAudioSize, pDmaEngine->maxAncSize);
		
		// init dma hardware dependent
		if (pDmaEngine->state == DmaStateConfigure)
		{
			if (dmaHardwareOpen(pDmaEngine))
			{
				NTV2_MSG_INFO("%s%d:%s%d: dmaInit hardware configured\n", DMA_MSG_ENGINE);
			}
			else
			{
				NTV2_MSG_ERROR("%s%d:%s%d: dmaInit hardware configure failed\n", DMA_MSG_ENGINE);
				pDmaEngine->state = DmaStateDead;
			}
		}

		// configure dma context
		if (pDmaEngine->state == DmaStateConfigure)
		{
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
				if (dmaPageBufferInit(deviceNumber, &pDmaContext->videoPageBuffer, pDmaEngine->maxVideoPages) != 0)
				{
					NTV2_MSG_ERROR("%s%d:%s%d:%s%d: dmaInit allocate video page buffer failed\n",
								   DMA_MSG_CONTEXT);
					pDmaEngine->state = DmaStateDead;
				}

				if (dmaPageBufferInit(deviceNumber, &pDmaContext->audioPageBuffer, pDmaEngine->maxAudioPages) != 0)
				{
					NTV2_MSG_ERROR("%s%d:%s%d:%s%d: dmaInit allocate audio page buffer failed\n",
								   DMA_MSG_CONTEXT);
					pDmaEngine->state = DmaStateDead;
				}

				if (dmaPageBufferInit(deviceNumber, &pDmaContext->ancF1PageBuffer, pDmaEngine->maxAncPages) != 0)
				{
					NTV2_MSG_ERROR("%s%d:%s%d:%s%d: dmaInit allocate anc field 1 page buffer failed\n",
								   DMA_MSG_CONTEXT);
					pDmaEngine->state = DmaStateDead;
				}

				if (dmaPageBufferInit(deviceNumber, &pDmaContext->ancF2PageBuffer, pDmaEngine->maxAncPages) != 0)
				{
					NTV2_MSG_ERROR("%s%d:%s%d:%s%d: dmaInit allocate anc field 2 page buffer failed\n",
								   DMA_MSG_CONTEXT);
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
		dmaHardwareAcquire(pDmaEngine, DMA_TRANSFER_TIMEOUT);
		dmaHardwareRelease(pDmaEngine);
	}

	NTV2_MSG_INFO("%s%d: dmaDisable end\n", DMA_MSG_DEVICE);
}

static void dmaDeviceRelease(struct device *dev)
{
	PDMA_ENGINE pDmaEngine = (PDMA_ENGINE)dev_get_drvdata(dev);

	NTV2_MSG_INFO("%s%d:%s%d: dmaDeviceRelease\n", DMA_MSG_ENGINE);
	return;
}

static bool dmaHardwareOpen(PDMA_ENGINE pDmaEngine)
{
	ULWord deviceNumber = pDmaEngine->deviceNumber;
	NTV2PrivateParams *pNTV2Params = getNTV2Params(deviceNumber);
    struct dma_device* dma_dev = NULL;
    struct device* dma_sysdev = NULL;
	dma_cap_mask_t mask;
    uint32_t values[4];
	int ret;
	
	switch (pDmaEngine->dmaMethod)
	{
	case DmaMethodZynq:
		break;
	default:
		NTV2_MSG_ERROR("%s%d:%s%d: dmaHardwareOpen unsupported dma hardware\n", DMA_MSG_ENGINE);
		return false;
	}

	pDmaEngine->dmaDevice.parent = &pNTV2Params->platform_dev->dev;
	pDmaEngine->dmaDevice.release = dmaDeviceRelease;
	dev_set_name(&pDmaEngine->dmaDevice, "%s", pDmaEngine->engName);
	dev_set_drvdata(&pDmaEngine->dmaDevice, pDmaEngine);
	ret = device_register(&pDmaEngine->dmaDevice);
	if(ret)
	{
		NTV2_MSG_ERROR("%s%d:%s%d: dmaHardwareOpen zynq dma device register failed\n", DMA_MSG_ENGINE);
		return false;
	}

	dma_cap_zero(mask);
	pDmaEngine->dmaChannel = dma_request_channel(mask, NULL, NULL);
	if(pDmaEngine->dmaChannel == NULL)
	{
		NTV2_MSG_ERROR("%s%d:%s%d: dmaHardwareOpen dma_request_channel failed\n", DMA_MSG_ENGINE);
		goto fail;
	}

    dma_dev = pDmaEngine->dmaChannel->device;
    if(dma_dev != NULL)
    {
        dma_sysdev = dma_dev->dev;
        if(dma_sysdev != NULL)
        {
            if(dma_sysdev->of_node != NULL)
            {
                if(of_device_is_compatible(dma_sysdev->of_node, "xlnx,zynqmp-dma-1.0"))
                {
                    NTV2_MSG_INFO("%s%d:%s%d: dmaHardwareOpen zynq dma engine can be throttled\n", DMA_MSG_ENGINE);
                    ret = of_property_read_u32_array(dma_sysdev->of_node, "reg", values, 4);
                    pDmaEngine->zdma_addr = ((uint64_t)values[0] << 32) | (uint64_t)values[1];
                    ofpd_zdma_write_rate_control(pDmaEngine, ZDMA_RATE_CONTROL_DEFAULT);
                    device_create_file(&pDmaEngine->dmaDevice, &dev_attr_zdma_rate_control);
                }
            }
        }
    }
                    
	NTV2_MSG_INFO("%s%d:%s%d: dmaHardwareOpen zynq dma engine configured\n", DMA_MSG_ENGINE);
	return true;

fail:
	NTV2_MSG_INFO("%s%d:%s%d: dmaHardwareOpen zynq dma engine configuration failed\n", DMA_MSG_ENGINE);
	device_unregister(&pDmaEngine->dmaDevice);
	return false;
}

static void dmaHardwareClose(PDMA_ENGINE pDmaEngine)
{
	if (pDmaEngine == NULL)
		return;

    if(pDmaEngine->zdma_addr)
    {
        device_remove_file(&pDmaEngine->dmaDevice, &dev_attr_zdma_rate_control);
        pDmaEngine->zdma_addr = 0;
    }
    
	device_unregister(&pDmaEngine->dmaDevice);
	if(pDmaEngine->dmaChannel != NULL)
	{
		dmaengine_terminate_all(pDmaEngine->dmaChannel);
		dma_release_channel(pDmaEngine->dmaChannel);
		pDmaEngine->dmaChannel = NULL;
	}

	NTV2_MSG_INFO("%s%d:%s%d: dmaHardwareClose\n", DMA_MSG_ENGINE);
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
	int dmaStatus = 0;
	int status = 0;
	ULWord errorCount = 0;
	bool dmaC2H = false;
	ULWord direction = 0;
	ULWord timeoutJiffies = microsecondsToJiffies(DMA_TRANSFER_TIMEOUT);
	bool serialTransfer = false;
	bool serialAcquired = false;
	bool engineAcquired = false;
	bool hasVideo = false;
	bool hasAudio = false;
	bool hasAncF1 = false;
	bool hasAncF2 = false;
	ULWord memorySize = 0;
	ULWord frameBufferSize = 0;
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
	ULWord videoFrameOffset = 0;
	ULWord videoNumBytes = 0;
	ULWord videoFramePitch = 0;
	ULWord videoUserPitch = 0;
	ULWord videoNumSegments = 0;
	ULWord videoCardAddress = 0;
	ULWord videoCardBytes = 0;
	ULWord audioFrameOffset = 0;
	ULWord audioCardBytes = 0;
	ULWord audioRingAddress = 0;
	ULWord audioRingSize = 0;
	ULWord ancF1FrameOffset = 0;
	ULWord ancF1CardAddress = 0;
	ULWord ancF1CardBytes = 0;
	ULWord ancF2FrameOffset = 0;
	ULWord ancF2CardAddress = 0;
	ULWord ancF2CardBytes = 0;
	LWord64 softStartTime = 0;
	LWord64 softLockTime = 0;
	LWord64 softDmaWaitTime = 0;
	LWord64 softDmaTime = 0;
	LWord64 softUnlockTime = 0;
	LWord64 softDoneTime = 0;
	Ntv2SystemContext systemContext;
	
    memset(&systemContext, 0, sizeof(Ntv2SystemContext));
	systemContext.devNum = deviceNumber;

	if (NTV2_DEBUG_ACTIVE(NTV2_DEBUG_STATISTICS))
	{
		softStartTime = timeMicro();
		softLockTime = softStartTime;
		softDmaWaitTime = softStartTime;
		softDmaTime = softStartTime;
		softUnlockTime = softStartTime;
		softDoneTime = softStartTime;
	}

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
 	NTV2_MSG_TRANSFER("%s%d:%s%d: dmaTransfer pAudUserVa %016llx  audioSystem %d  audNumBytes %d  audOffset %d\n",
					  DMA_MSG_ENGINE, (ULWord64)pDmaParams->pAudUserVa, pDmaParams->audioSystem, 
					  pDmaParams->audNumBytes, pDmaParams->audOffset);
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

	// direction
	dmaC2H = pDmaParams->toHost;

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
		if (NTV2_DEBUG_ACTIVE(NTV2_DEBUG_STATISTICS))
		{
			softLockTime = timeMicro();
		}

		// zynq dma engines are bidirectional
		if (pDmaEngine->dmaMethod == DmaMethodZynq)
		{
			pDmaContext->dmaC2H = pDmaParams->toHost;
		}
		dmaC2H = pDmaContext->dmaC2H;
		direction = dmaC2H? DMA_FROM_DEVICE : DMA_TO_DEVICE;
		
		// do nothing by default
		pDmaContext->pVideoPageBuffer = NULL;
		pDmaContext->pAudioPageBuffer = NULL;
		pDmaContext->pAncF1PageBuffer = NULL;
		pDmaContext->pAncF2PageBuffer = NULL;
		pDmaContext->videoCardAddress = 0;
		pDmaContext->videoCardSize = 0;
		pDmaContext->audioCardAddress[0] = 0;
		pDmaContext->audioCardSize[0] = 0;
		pDmaContext->audioCardAddress[1] = 0;
		pDmaContext->audioCardSize[1] = 0;
		pDmaContext->ancF1CardAddress = 0;
		pDmaContext->ancF1CardSize = 0;
		pDmaContext->ancF2CardAddress = 0;
		pDmaContext->ancF2CardSize = 0;
		pDmaContext->doVideo = false;
		pDmaContext->doAudio = false;
		pDmaContext->doAncF1 = false;
		pDmaContext->doAncF2 = false;

		// get video info
		memorySize = NTV2DeviceGetActiveMemorySize(pNTV2Params->_DeviceID);
		frameBufferSize = GetFrameBufferSize(&systemContext, pDmaParams->videoChannel);

		// look for video to dma
		hasVideo = true;

		// enforce 4 byte alignment
		videoFrameOffset = pDmaParams->frameOffset & 0xfffffffc;
		videoNumBytes = pDmaParams->vidNumBytes & 0xfffffffc;
		videoFramePitch = pDmaParams->vidFramePitch & 0xfffffffc;
		videoUserPitch = pDmaParams->vidUserPitch & 0xfffffffc;
		videoNumSegments = pDmaParams->numSegments;

		if(videoNumBytes == 0)
		{
			hasVideo = false;
		}
	
		if(videoNumSegments == 0)
		{
			videoNumSegments = 1;
		}
		if (videoNumSegments != 1)
		{
			errorCount++;
			NTV2_MSG_ERROR("%s%d:%s%d: dmaTransfer segmented transfer not supported\n", DMA_MSG_ENGINE);
			return -EPERM;
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
					dmaSgMap(deviceNumber, pVideoPageBuffer, pDmaEngine->dmaChannel->device->dev);
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
			else
			{
				hasVideo = false;
			}
		}

		if (hasVideo)
		{
			pDmaContext->pVideoPageBuffer = pVideoPageBuffer;
			pDmaContext->videoCardAddress = videoCardAddress;
			pDmaContext->videoCardSize = videoCardBytes;
			pDmaContext->doVideo = true;
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

		// enforce 4 byte alignment
		audioFrameOffset = pDmaParams->audOffset & 0xfffffffc;
		audioCardBytes = pDmaParams->audNumBytes & 0xfffffffc;

		// verify audio buffer
		if((pDmaParams->pAudUserVa == NULL) ||
		   (audioCardBytes == 0) ||
		   (audioCardBytes >= audioRingSize))
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
				dmaSgMap(deviceNumber, pAudioPageBuffer, pDmaEngine->dmaChannel->device->dev);
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
		}

		if (hasAudio)
		{
			pDmaContext->pAudioPageBuffer = pAudioPageBuffer;
			if (audioFrameOffset >= audioRingSize)
			{
				audioFrameOffset = 0;
			}
			pDmaContext->audioCardAddress[0] = audioRingAddress + audioFrameOffset;
			pDmaContext->audioCardSize[0] = audioCardBytes;
			pDmaContext->audioCardAddress[1] = 0;
			pDmaContext->audioCardSize[1] = 0;
			if ((audioFrameOffset + audioCardBytes) >= audioRingSize)
			{
				pDmaContext->audioCardSize[0] = audioRingSize - audioFrameOffset;
				pDmaContext->audioCardAddress[1] = audioRingAddress;
				pDmaContext->audioCardSize[1] = audioCardBytes - pDmaContext->audioCardSize[0];
			}
			pDmaContext->doAudio = true;
		}
		
		// look for anc field 1 to dma
		hasAncF1 = true;

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
				dmaSgMap(deviceNumber, pAncF1PageBuffer, pDmaEngine->dmaChannel->device->dev);
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
		}

		if (hasAncF1)
		{
			pDmaContext->pAncF1PageBuffer = pAncF1PageBuffer;
			pDmaContext->ancF1CardAddress = ancF1CardAddress;
			pDmaContext->ancF1CardSize = ancF1CardBytes;
			pDmaContext->doAncF1 = true;
		}

		// look for anc field 2 to dma
		hasAncF2 = true;

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
				dmaSgMap(deviceNumber, pAncF2PageBuffer, pDmaEngine->dmaChannel->device->dev);
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
		}		

		if (hasAncF2)
		{
			pDmaContext->pAncF2PageBuffer = pAncF2PageBuffer;
			pDmaContext->ancF2CardAddress = ancF2CardAddress;
			pDmaContext->ancF2CardSize = ancF2CardBytes;
			pDmaContext->doAncF2 = true;
		}

		if (!pDmaContext->doVideo &&
			!pDmaContext->doAudio &&
			!pDmaContext->doAncF1 &&
			!pDmaContext->doAncF2)
		{
			errorCount++;
			dmaStatus = -EPERM;
			NTV2_MSG_ERROR("%s%d:%s%d:%s%d: dmaTransfer nothing to transfer\n",
						   DMA_MSG_CONTEXT);
		}

		// wait for the hardware
		if (dmaStatus == 0)
		{
			// record the wait start time
			if (NTV2_DEBUG_ACTIVE(NTV2_DEBUG_STATISTICS))
			{
				softDmaWaitTime = timeMicro();
			}

			if (serialTransfer)
			{
				dmaStatus = dmaSerialAcquire(deviceNumber, DMA_TRANSFER_TIMEOUT);
				if (dmaStatus == 0)
				{
					serialAcquired = true;
					dmaStatus = dmaHardwareAcquire(pDmaEngine, DMA_TRANSFER_TIMEOUT);
				}
				else
				{
					errorCount++;
					NTV2_MSG_ERROR("%s%d:%s%d: dmaTransfer acquire serial lock failed\n",
								   DMA_MSG_ENGINE);
				}
			}
			else
			{
				dmaStatus = dmaHardwareAcquire(pDmaEngine, DMA_TRANSFER_TIMEOUT);
			}

			if (dmaStatus == 0)
			{
				engineAcquired = true;
			}
			else
			{
				errorCount++;
				NTV2_MSG_ERROR("%s%d:%s%d: dmaTransfer acquire engine lock failed\n", DMA_MSG_ENGINE);
			}

			if (dmaStatus == 0)
			{
				// check enabled
				if (!pDmaEngine->dmaEnable)
				{
					errorCount++;
					dmaStatus = -EPERM;
					NTV2_MSG_ERROR("%s%d:%s%d: dmaTransfer engine not enabled\n", DMA_MSG_ENGINE);
				}
			
				// check for correct engine state
				if (pDmaEngine->state != DmaStateIdle)
				{
					errorCount++;
					dmaStatus = -EPERM;
					NTV2_MSG_ERROR("%s%d:%s%d: dmaTransfer engine state %d not idle\n",
								   DMA_MSG_ENGINE, pDmaEngine->state);
				}
			}
		}
	
		if (dmaStatus == 0)
		{
			// record the transfer start time
			if (NTV2_DEBUG_ACTIVE(NTV2_DEBUG_STATISTICS))
			{
				softDmaTime = timeMicro();
			}

			// do dma
			NTV2_MSG_STATE("%s%d:%s%d: dmaTransfer dma state transfer\n", DMA_MSG_ENGINE);
			pDmaEngine->state = DmaStateTransfer;
		
			// clear the transfer event
			clear_bit(0, &pDmaEngine->transferDone);

			// do the dma
			switch (pDmaEngine->dmaMethod)
			{
			case DmaMethodZynq:
				dmaStatus = dmaZynqProgram(pDmaContext);
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
					NTV2_MSG_ERROR("%s%d:%s%d:%s%d: dmaTransfer dma transfer timeout address %016llx  frame %d  size %d\n", 
								   DMA_MSG_CONTEXT, (ULWord64)pDmaParams->pVidUserVa,
                                   pDmaParams->videoFrame, pDmaParams->vidNumBytes);

                    switch (pDmaEngine->dmaMethod)
					{
					case DmaMethodZynq:
						dmaZynqAbort(pDmaEngine);
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

			// finish dma
			NTV2_MSG_STATE("%s%d:%s%d: dmaTransfer dma state finish\n", DMA_MSG_ENGINE);
			pDmaEngine->state = DmaStateFinish;

			// save dma programming statistics
			if (NTV2_DEBUG_ACTIVE(NTV2_DEBUG_STATISTICS))
			{
				dmaEngineLock(pDmaEngine);
				if (dmaC2H)
				{
					pDmaEngine->csErrorCount += errorCount;
					pDmaEngine->csTransferBytes += pDmaParams->vidNumBytes;
				}
				else
				{
					pDmaEngine->scErrorCount += errorCount;
					pDmaEngine->scTransferBytes += pDmaParams->vidNumBytes;
				}
				dmaEngineUnlock(pDmaEngine);
			}
		
			NTV2_MSG_STATE("%s%d:%s%d: dmaTransfer dma state idle\n", DMA_MSG_ENGINE);
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

		if(NTV2_DEBUG_ACTIVE(NTV2_DEBUG_STATISTICS))
		{
			softUnlockTime = timeMicro();
		}

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
			dmaPageRootFree(deviceNumber, pDmaParams->pPageRoot, pVideoPageBuffer);
		}
		if (findAudio)
		{
			dmaPageRootFree(deviceNumber, pDmaParams->pPageRoot, pAudioPageBuffer);
		}
		if (findAncF1)
		{
			dmaPageRootFree(deviceNumber, pDmaParams->pPageRoot, pAncF1PageBuffer);
		}
		if (findAncF2)
		{
			dmaPageRootFree(deviceNumber, pDmaParams->pPageRoot, pAncF2PageBuffer);
		}

		// done
		pDmaContext->doVideo = false;
		pDmaContext->doAudio = false;
		pDmaContext->doAncF1 = false;
		pDmaContext->doAncF2 = false;
		
		// release the context
		dmaContextRelease(pDmaContext);
	}

	// do stats
	if(NTV2_DEBUG_ACTIVE(NTV2_DEBUG_STATISTICS))
	{
		softDoneTime = timeMicro();
		
		dmaEngineLock(pDmaEngine);
		if (dmaC2H)
		{
			pDmaEngine->csTransferCount++;
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
			pDmaEngine->scErrorCount += errorCount;
			pDmaEngine->scTransferTime += softDoneTime - softStartTime;
			pDmaEngine->scLockWaitTime += softLockTime - softStartTime;
			pDmaEngine->scLockTime += softDmaWaitTime - softLockTime;
			pDmaEngine->scDmaWaitTime += softDmaTime - softDmaWaitTime;
			pDmaEngine->scDmaTime += softUnlockTime - softDmaTime;
			pDmaEngine->scUnlockTime += softDoneTime - softUnlockTime;
		}
		dmaEngineUnlock(pDmaEngine);
	}

	dmaStatistics(pDmaEngine, dmaC2H);

	return dmaStatus;
}

int dmaTargetP2P(ULWord deviceNumber, NTV2_DMA_P2P_CONTROL_STRUCT* pParams)
{
	NTV2_MSG_INFO("%s%d: dmaTargetP2P\n", DMA_MSG_DEVICE);

	return -EINVAL;
}

void dmaInterrupt(ULWord deviceNumber, ULWord intStatus)
{
	NTV2_MSG_INFO("%s%d: dmaInterrupt\n", DMA_MSG_DEVICE);
}

PDMA_ENGINE getDmaEngine(ULWord deviceNumber, ULWord engIndex)
{
	NTV2PrivateParams *pNTV2Params = getNTV2Params(deviceNumber);
	PDMA_ENGINE pDmaEngine = &pNTV2Params->_dmaEngine[engIndex];
	return pDmaEngine;
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
	NTV2_MSG_INFO("%s%d:%s%d: dmaFreeEngine\n", DMA_MSG_ENGINE);

	if (!pDmaEngine->engInit)
	{
		return;
	}

	dmaHardwareClose(pDmaEngine);

	pDmaEngine->engInit = false;
	pDmaEngine->dmaEnable = false;
	pDmaEngine->state = DmaStateUnknown;
}

static PDMA_ENGINE dmaMapEngine(ULWord deviceNumber, NTV2DMAEngine eDMAEngine, bool bToHost)
{
	NTV2PrivateParams *pNTV2Params = getNTV2Params(deviceNumber);
	ULWord engIndex = DMA_NUM_ENGINES;

	// find the correct engine
	switch (eDMAEngine)
	{
	default:
	case NTV2_DMA1:
	case NTV2_DMA2:
	case NTV2_DMA3:
	case NTV2_DMA4:
		engIndex = 0;
		break;
	}

	if (engIndex > pNTV2Params->_dmaNumEngines) return NULL;
	
	return getDmaEngine(deviceNumber, engIndex);
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

int dmaHardwareAcquire(PDMA_ENGINE pDmaEngine, ULWord timeout)
{
	int status;

	// wait for hardware available
	timeout = microsecondsToJiffies(timeout);
	status = down_timeout(&pDmaEngine->transferSemaphore, timeout);
	if (status != 0)
		return -ETIME;

	return 0;
}

void dmaHardwareRelease(PDMA_ENGINE pDmaEngine)
{
	// release hardware
	up(&pDmaEngine->transferSemaphore);
}

static int dmaSerialAcquire(ULWord deviceNumber, ULWord timeout)
{
	NTV2PrivateParams *pNTV2Params = getNTV2Params(deviceNumber);
	int status;

	// wait for serial available
	timeout = microsecondsToJiffies(timeout);
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

static void dmaStatistics(PDMA_ENGINE pDmaEngine, bool dmaC2H)
{
	LWord64 softStatTime;
	LWord64 statTransferCount;
	LWord64 statTransferBytes;
	LWord64 statTransferTime;
	LWord64 statLockWaitTime;
	LWord64 statLockTime;
	LWord64 statDmaWaitTime;
	LWord64 statDmaTime;
	LWord64 statUnlockTime;
	LWord64 statErrorCount;
	LWord64 statDisplayTime;

	if(NTV2_DEBUG_ACTIVE(NTV2_DEBUG_STATISTICS))
	{
		softStatTime = timeMicro();

		dmaEngineLock(pDmaEngine);
		if (dmaC2H)
		{
			statTransferCount	= pDmaEngine->csTransferCount;
			statErrorCount		= pDmaEngine->csErrorCount;
			statTransferBytes	= pDmaEngine->csTransferBytes;
			statTransferTime	= pDmaEngine->csTransferTime;
			statLockWaitTime	= pDmaEngine->csLockWaitTime;
			statLockTime		= pDmaEngine->csLockTime;
			statDmaWaitTime		= pDmaEngine->csDmaWaitTime;
			statDmaTime			= pDmaEngine->csDmaTime;
			statUnlockTime		= pDmaEngine->csUnlockTime;
			statDisplayTime		= softStatTime - pDmaEngine->csLastDisplayTime;
		}
		else
		{
			statTransferCount	= pDmaEngine->scTransferCount;
			statErrorCount		= pDmaEngine->scErrorCount;
			statTransferBytes	= pDmaEngine->scTransferBytes;
			statTransferTime	= pDmaEngine->scTransferTime;
			statLockWaitTime	= pDmaEngine->scLockWaitTime;
			statLockTime		= pDmaEngine->scLockTime;
			statDmaWaitTime		= pDmaEngine->scDmaWaitTime;
			statDmaTime			= pDmaEngine->scDmaTime;
			statUnlockTime		= pDmaEngine->scUnlockTime;
			statDisplayTime		= softStatTime - pDmaEngine->scLastDisplayTime;
		}

		if (statDisplayTime > DMA_STATISTICS_INTERVAL)
		{
			LWord64 transferTime;
			LWord64 transferBytes;
			LWord64 transferPerf;
			LWord64 lockWaitTime;
			LWord64 lockTime;
			LWord64 dmaWaitTime;
			LWord64 dmaTime;
			LWord64 unlockTime;
			
			if (dmaC2H)
			{
				pDmaEngine->csTransferCount		= 0;
				pDmaEngine->csErrorCount		= 0;
				pDmaEngine->csTransferBytes		= 0;
				pDmaEngine->csTransferTime		= 0;
				pDmaEngine->csLockWaitTime		= 0;
				pDmaEngine->csLockTime			= 0;
				pDmaEngine->csDmaWaitTime		= 0;
				pDmaEngine->csDmaTime			= 0;
				pDmaEngine->csUnlockTime		= 0;
				pDmaEngine->csLastDisplayTime	= softStatTime;
			}
			else
			{
				pDmaEngine->scTransferCount		= 0;
				pDmaEngine->scErrorCount		= 0;
				pDmaEngine->scTransferBytes		= 0;
				pDmaEngine->scTransferTime		= 0;
				pDmaEngine->scLockWaitTime		= 0;
				pDmaEngine->scLockTime			= 0;
				pDmaEngine->scDmaWaitTime		= 0;
				pDmaEngine->scDmaTime			= 0;
				pDmaEngine->scUnlockTime		= 0;
				pDmaEngine->scLastDisplayTime	= softStatTime;
			}
			dmaEngineUnlock(pDmaEngine);

			if (statTransferCount == 0) statTransferCount = 1;
			if (statTransferTime == 0) statTransferTime = 1;

			transferTime = statTransferTime;
			do_div(transferTime, statTransferCount);
			transferBytes = statTransferBytes;
			do_div(transferBytes, statTransferCount);
			do_div(transferBytes, 1000);
			transferPerf = statTransferBytes;
			do_div(transferPerf, statTransferTime);
			NTV2_MSG_STATISTICS("%s%d:%s%d: dmaTransfer %s  trn %6d  err %6d  time %6d us  size %6dk  perf %6d mb/s\n",
								DMA_MSG_ENGINE,
								dmaC2H? "c2h":"h2c",
								(ULWord)statTransferCount,
								(ULWord)statErrorCount,
								(ULWord)transferTime,
								(ULWord)transferBytes,
								(ULWord)transferPerf);

			lockWaitTime = statLockWaitTime;
			do_div(lockWaitTime, statTransferCount);
			lockTime = statLockTime;
			do_div(lockTime, statTransferCount);
			dmaWaitTime = statDmaWaitTime;
			do_div(dmaWaitTime, statTransferCount);
			dmaTime = statDmaTime;
			do_div(dmaTime, statTransferCount);
			unlockTime = statUnlockTime;
			do_div(unlockTime, statTransferCount);
			NTV2_MSG_STATISTICS("%s%d:%s%d: dmaTransfer %s  lockw %6d  lock %6d  dmaw %6d  dma %6d  unlck %6d\n",
								DMA_MSG_ENGINE,
								dmaC2H? "c2h":"h2c",
								(ULWord)lockWaitTime,
								(ULWord)lockTime,
								(ULWord)dmaWaitTime,
								(ULWord)dmaTime,
								(ULWord)unlockTime);
		}
		else
		{
			dmaEngineUnlock(pDmaEngine);
		}
	}
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
	unsigned long flags;
	LWord refCount;
	int timeout = DMA_TRANSFER_TIMEOUT / 20000;
	int out = 0;

	if (pRoot == NULL)
		return;

	NTV2_MSG_PAGE_MAP("%s%d: dmaPageRootRelease  release %lld bytes\n",
					  DMA_MSG_DEVICE, pRoot->lockTotalSize);

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
				   PVOID pAddress, ULWord size, bool map)
{
	PDMA_PAGE_BUFFER pBuffer;
	unsigned long flags;
	int ret;
	
	if ((pRoot == NULL) || (pAddress == NULL) || (size == 0))
		return -EINVAL;

	NTV2_MSG_PAGE_MAP("%s%d: dmaPageRootAdd  addr %016llx  size %d  map %d\n",
					  DMA_MSG_DEVICE, (ULWord64)pAddress, size, map);

	// use current buffer if found
	pBuffer = dmaPageRootFind(deviceNumber, pRoot, pAddress, size);
	if (pBuffer != NULL)
	{
		dmaPageRootFree(deviceNumber, pRoot, pBuffer);
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

	ret = dmaPageBufferInit(deviceNumber, pBuffer, (size / PAGE_SIZE + 2));
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
#if 0
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
	}
#endif	
	spin_lock_irqsave(&pRoot->bufferLock, flags);
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

void dmaPageRootFree(ULWord deviceNumber, PDMA_PAGE_ROOT pRoot, PDMA_PAGE_BUFFER pBuffer)
{
	unsigned long flags;

	if ((pRoot == NULL) || (pBuffer == NULL))
		return;

	NTV2_MSG_PAGE_MAP("%s%d: dmaPageRootFree  addr %016llx  size %d\n",
					  DMA_MSG_DEVICE, (ULWord64)pBuffer->pUserAddress, pBuffer->userSize);

	// decrement reference
	spin_lock_irqsave(&pRoot->bufferLock, flags);
	pBuffer->refCount--;
	spin_unlock_irqrestore(&pRoot->bufferLock, flags);
}

static int dmaPageBufferInit(ULWord deviceNumber, PDMA_PAGE_BUFFER pBuffer, ULWord numPages)
{
	if ((pBuffer == NULL) || (numPages == 0))
		return -EINVAL;
	
	memset(pBuffer, 0, sizeof(DMA_PAGE_BUFFER));
	INIT_LIST_HEAD(&pBuffer->bufferEntry);

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

	if ((pBuffer == NULL) || (pBuffer->pPageList == NULL) || (pBuffer->pSgList == NULL) ||
		(pAddress == NULL) || (size == 0) || pBuffer->busMap)
		return -EINVAL;

	if (pBuffer->pageLock)
		return 0;

	// clear page list
	memset(pBuffer->pPageList, 0, sizeof(struct page*) * pBuffer->pageListSize);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4,16,0))
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

	// page in and lock the user buffer
	numPinned = get_user_pages_fast(
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
		pBuffer->pPageList);
	
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
        flush_dcache_page(pBuffer->pPageList[0]);
		sg_set_page(&pBuffer->pSgList[0], pBuffer->pPageList[0], PAGE_SIZE - pageOffset, pageOffset);
		count -= sg_dma_len(&pBuffer->pSgList[0]);

		for (i = 1; i < numPages; i++)
		{
			flush_dcache_page(pBuffer->pPageList[i]);
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
	
	if ((pBuffer == NULL) || (pBuffer->pPageList == NULL))
		return;

	if (pBuffer->pageLock)
	{
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

static int dmaSgMap(ULWord deviceNumber, PDMA_PAGE_BUFFER pBuffer, struct device* pSgDevice)
{
	int count;
	
	if ((pBuffer == NULL) || (pBuffer->pSgList == NULL))
		return 0;

	if (pBuffer->pageLock && !pBuffer->sgMap)
	{
		// map scatter list
		count = dma_map_sg(pSgDevice,
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
		pBuffer->pSgDevice = pSgDevice;

		NTV2_MSG_PAGE_MAP("%s%d: dmaSgMap mapped %d segment(s)\n", 
					  DMA_MSG_DEVICE, pBuffer->numSgs); 
	}

	return 0;
}

static void dmaSgUnmap(ULWord deviceNumber, PDMA_PAGE_BUFFER pBuffer)
{
	if ((pBuffer == NULL) || (pBuffer->pSgList == NULL))
		return;

	if (pBuffer->sgMap)
	{
		NTV2_MSG_PAGE_MAP("%s%d: dmaSgUnmap unmap %d segments\n", 
						  DMA_MSG_DEVICE, pBuffer->numSgs); 

		// unmap the scatter list
		dma_unmap_sg(pBuffer->pSgDevice,
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
	if ((pBuffer == NULL) || (pBuffer->pSgList == NULL))
		return;

	if (pBuffer->sgMap)
	{
		if (pBuffer->sgHost)
		{
			// sync scatter list for device access
			dma_sync_sg_for_device(pBuffer->pSgDevice,
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
	if ((pBuffer == NULL) || (pBuffer->pSgList == NULL))
		return;

	if (pBuffer->sgMap)
	{
		if (!pBuffer->sgHost)
		{
			// sync scatter list for cpu access
			dma_sync_sg_for_cpu(pBuffer->pSgDevice,
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

static inline struct scatterlist* dmaSgList(PDMA_PAGE_BUFFER pBuffer)
{
	return pBuffer->pSgList;
}

static inline ULWord dmaSgCount(PDMA_PAGE_BUFFER pBuffer)
{
	return pBuffer->numSgs;
}

static int dmaZynqProgram(PDMA_CONTEXT pDmaContext)
{
	ULWord 		deviceNumber = pDmaContext->deviceNumber;
	PDMA_ENGINE	pDmaEngine = getDmaEngine(deviceNumber, pDmaContext->engIndex);
	NTV2PrivateParams *pNTV2Params = getNTV2Params(deviceNumber);
#ifdef ZEFRAM
	unsigned long dmaFlags = DMA_CTRL_ACK | DMA_PREP_INTERRUPT;
#endif
	struct dma_async_tx_descriptor*	pDmaDesc;
	struct scatterlist sg[2];
	int sgCount;
	int res;
	bool fail = false;
	
	NTV2_MSG_PROGRAM("%s%d:%s%d:%s%d: dmaZynqProgram program %s %s %s %s\n", 
					 DMA_MSG_CONTEXT, 
					 pDmaContext->doVideo? "video":"",
					 pDmaContext->doAudio? "audio":"",
					 pDmaContext->doAncF1? "ancF1":"",
					 pDmaContext->doAncF2? "ancF2":"");

	pDmaEngine->sbVideo = false;
	pDmaEngine->sbAudio = false;
	pDmaEngine->sbAncF1 = false;
	pDmaEngine->sbAncF2 = false;

    // enable rate control
    if(pDmaEngine->zdma_addr)
    {
        ofpd_iormw32(pDmaEngine->zdma_addr + ZDMA_CH_CTRL0_OFFSET, 1, ZDMA_CH_CTRL0_SHIFT, ZDMA_CH_CTRL0_MASK);
    }
    
	if (pDmaContext->doVideo)
	{
		// build card sg list
		sgCount = 1;
		sg_init_table(sg, sgCount);
		sg_dma_address(sg) = pNTV2Params->_FrameMemoryAddress + pDmaContext->videoCardAddress;
		sg_dma_len(sg) = pDmaContext->videoCardSize;

		NTV2_MSG_PROGRAM("%s%d:%s%d:%s%d: dmaZynqProgram video address %016llx  size %d\n",
						 DMA_MSG_CONTEXT,
						 sg_dma_address(sg),
						 sg_dma_len(sg));
		
		// iniatlize dma descriptor
		pDmaDesc = NULL;
#ifdef ZEFRAM
		if (pDmaContext->dmaC2H)
		{
			pDmaDesc = zynqmp_dma_prep_sg(pDmaEngine->dmaChannel,
										  dmaSgList(pDmaContext->pVideoPageBuffer),
										  dmaSgCount(pDmaContext->pVideoPageBuffer),
										  sg,
										  sgCount,
										  dmaFlags);
		}
		else
		{
			pDmaDesc = zynqmp_dma_prep_sg(pDmaEngine->dmaChannel,
										  sg,
										  sgCount,
										  dmaSgList(pDmaContext->pVideoPageBuffer),
										  dmaSgCount(pDmaContext->pVideoPageBuffer),
										  dmaFlags);
		}
#endif        
		if (pDmaDesc != NULL)
		{
			pDmaDesc->callback = dmaZynqVideoCallback;
			pDmaDesc->callback_param = pDmaEngine;
			pDmaEngine->videoDmaCookie = dmaengine_submit(pDmaDesc);
			pDmaEngine->sbVideo = true;
			res = dma_submit_error(pDmaEngine->videoDmaCookie);
			if (res)
			{
				NTV2_MSG_ERROR("%s%d:%s%d: dmaZynqProgram video dma_submit_error %08x\n",
							   DMA_MSG_ENGINE, res);
				pDmaEngine->sbVideo = false;
				fail = true;
			}
		}
		else
		{
			NTV2_MSG_ERROR("%s%d:%s%d: dmaZynqProgram video dmaengine_prep_dma_sg returns NULL\n",
						   DMA_MSG_ENGINE);
			fail = true;
		}
	}

	if (pDmaContext->doAudio)
	{
		// build card sg list
		if ((pDmaContext->audioCardAddress[1] == 0) ||
			(pDmaContext->audioCardSize[1] == 0))
		{
			sgCount = 1;
			sg_init_table(sg, sgCount);
			sg_dma_address(&sg[0]) = pNTV2Params->_FrameMemoryAddress + pDmaContext->audioCardAddress[0];
			sg_dma_len(&sg[0]) = pDmaContext->audioCardSize[0];
		}
		else
		{
			sgCount = 2;
			sg_init_table(sg, sgCount);
			sg_dma_address(&sg[0]) = pNTV2Params->_FrameMemoryAddress + pDmaContext->audioCardAddress[0];
			sg_dma_len(&sg[0]) = pDmaContext->audioCardSize[0];
			sg_dma_address(&sg[1]) = pNTV2Params->_FrameMemoryAddress + pDmaContext->audioCardAddress[1];
			sg_dma_len(&sg[1]) = pDmaContext->audioCardSize[1];
		}

		// iniatlize dma descriptor
		pDmaDesc = NULL;
#ifdef ZEFRAM
		if (pDmaContext->dmaC2H)
		{
			pDmaDesc = zynqmp_dma_prep_sg(pDmaEngine->dmaChannel,
										  dmaSgList(pDmaContext->pAudioPageBuffer),
										  dmaSgCount(pDmaContext->pAudioPageBuffer),
										  sg,
										  sgCount,
										  dmaFlags);
		}
		else
		{
			pDmaDesc = zynqmp_dma_prep_sg(pDmaEngine->dmaChannel,
										  sg,
										  sgCount,
										  dmaSgList(pDmaContext->pAudioPageBuffer),
										  dmaSgCount(pDmaContext->pAudioPageBuffer),
										  dmaFlags);
		}
#endif        
		if (pDmaDesc != NULL)
		{
			pDmaDesc->callback = dmaZynqAudioCallback;
			pDmaDesc->callback_param = pDmaEngine;
			pDmaEngine->audioDmaCookie = dmaengine_submit(pDmaDesc);
			pDmaEngine->sbAudio = true;
			res = dma_submit_error(pDmaEngine->audioDmaCookie);
			if (res)
			{
				NTV2_MSG_ERROR("%s%d:%s%d: dmaZynqProgram audio dma_submit_error %08x\n",
							   DMA_MSG_ENGINE, res);
				pDmaEngine->sbAudio = false;
				fail = true;
			}
		}
		else
		{
			NTV2_MSG_ERROR("%s%d:%s%d: dmaZynqProgram audio dmaengine_prep_dma_sg returns NULL\n",
						   DMA_MSG_ENGINE);
			fail = true;
		}
	}

	if (pDmaContext->doAncF1)
	{
		// build card sg list
		sgCount = 1;
		sg_init_table(sg, sgCount);
		sg_dma_address(sg) = pNTV2Params->_FrameMemoryAddress + pDmaContext->ancF1CardAddress;
		sg_dma_len(sg) = pDmaContext->ancF1CardSize;

		// iniatlize dma descriptor
		pDmaDesc = NULL;
#ifdef ZEFRAM
		if (pDmaContext->dmaC2H)
		{
			pDmaDesc = zynqmp_dma_prep_sg(pDmaEngine->dmaChannel,
										  dmaSgList(pDmaContext->pAncF1PageBuffer),
										  dmaSgCount(pDmaContext->pAncF1PageBuffer),
										  sg,
										  sgCount,
										  dmaFlags);
		}
		else
		{
			pDmaDesc = zynqmp_dma_prep_sg(pDmaEngine->dmaChannel,
										  sg,
										  sgCount,
										  dmaSgList(pDmaContext->pAncF1PageBuffer),
										  dmaSgCount(pDmaContext->pAncF1PageBuffer),
										  dmaFlags);
		}
#endif        
		if (pDmaDesc != NULL)
		{
			pDmaDesc->callback = dmaZynqAncF1Callback;
			pDmaDesc->callback_param = pDmaEngine;
			pDmaEngine->ancF1DmaCookie = dmaengine_submit(pDmaDesc);
			pDmaEngine->sbAncF1 = true;
			res = dma_submit_error(pDmaEngine->ancF1DmaCookie);
			if (res)
			{
				NTV2_MSG_ERROR("%s%d:%s%d: dmaZynqProgram ancF1 dma_submit_error %08x\n",
							   DMA_MSG_ENGINE, res);
				pDmaEngine->sbAncF1 = false;
				fail = true;
			}
		}
		else
		{
			NTV2_MSG_ERROR("%s%d:%s%d: dmaZynqProgram ancF1 dmaengine_prep_dma_sg returns NULL\n",
						   DMA_MSG_ENGINE);
			fail = true;
		}
	}
	
	if (pDmaContext->doAncF2)
	{
		// build card sg list
		sgCount = 1;
		sg_init_table(sg, sgCount);
		sg_dma_address(sg) = pNTV2Params->_FrameMemoryAddress + pDmaContext->ancF2CardAddress;
		sg_dma_len(sg) = pDmaContext->ancF2CardSize;

		// iniatlize dma descriptor
		pDmaDesc = NULL;
#ifdef ZEFRAM
		if (pDmaContext->dmaC2H)
		{
			pDmaDesc = zynqmp_dma_prep_sg(pDmaEngine->dmaChannel,
										  dmaSgList(pDmaContext->pAncF2PageBuffer),
										  dmaSgCount(pDmaContext->pAncF2PageBuffer),
										  sg,
										  sgCount,
										  dmaFlags);
		}
		else
		{
			pDmaDesc = zynqmp_dma_prep_sg(pDmaEngine->dmaChannel,
										  sg,
										  sgCount,
										  dmaSgList(pDmaContext->pAncF2PageBuffer),
										  dmaSgCount(pDmaContext->pAncF2PageBuffer),
										  dmaFlags);
		}
#endif        
		if (pDmaDesc != NULL)
		{
			pDmaDesc->callback = dmaZynqAncF2Callback;
			pDmaDesc->callback_param = pDmaEngine;
			pDmaEngine->ancF2DmaCookie = dmaengine_submit(pDmaDesc);
			pDmaEngine->sbAncF2 = true;
			res = dma_submit_error(pDmaEngine->ancF2DmaCookie);
			if (res)
			{
				NTV2_MSG_ERROR("%s%d:%s%d: dmaZynqProgram ancF2 dma_submit_error %08x\n",
							   DMA_MSG_ENGINE, res);
				pDmaEngine->sbAncF2 = false;
				fail = true;
			}
		}
		else
		{
			NTV2_MSG_ERROR("%s%d:%s%d: dmaZynqProgram ancF2 dmaengine_prep_dma_sg returns NULL\n",
						   DMA_MSG_ENGINE);
			fail = true;
		}
	}

	if (pDmaEngine->sbVideo || pDmaEngine->sbAudio || pDmaEngine->sbAncF1 || pDmaEngine->sbAncF2)
		dma_async_issue_pending(pDmaEngine->dmaChannel);

	if (fail)
		return -EPERM;
	
	return 0;
}

static void dmaZynqAbort(PDMA_ENGINE pDmaEngine)
{
    int i;
    
	NTV2_MSG_PROGRAM("%s%d:%s%d: dmaZynqAbort dma engine abort\n", DMA_MSG_ENGINE);

    ofpd_zdma_dump_registers(pDmaEngine);

    for (i = 0; i < 500; i++)
    {
        if (dma_async_is_tx_complete(pDmaEngine->dmaChannel, pDmaEngine->videoDmaCookie, NULL, NULL) != DMA_IN_PROGRESS)
            break;
        msleep(10);
    }
    if (dma_async_is_tx_complete(pDmaEngine->dmaChannel, pDmaEngine->videoDmaCookie, NULL, NULL) == DMA_IN_PROGRESS)
    {
        int ret = dmaengine_terminate_all(pDmaEngine->dmaChannel);
        if(ret)
        {
			NTV2_MSG_ERROR("%s%d:%s%d: dmaZynqAbort engine terminate failed %d\n", DMA_MSG_ENGINE, ret);
        }
        else
        {
			NTV2_MSG_ERROR("%s%d:%s%d: dmaZynqAbort dma terminated and probably broken\n", DMA_MSG_ENGINE);
        }
    }
}

static void dmaZynqVideoCallback(void* arg)
{
	PDMA_ENGINE pDmaEngine = (PDMA_ENGINE)arg;
	int res;

	if (pDmaEngine->sbVideo)
	{
		res = dma_async_is_tx_complete(pDmaEngine->dmaChannel, pDmaEngine->videoDmaCookie, NULL, NULL);
		if(res == DMA_COMPLETE)
		{
			if (!pDmaEngine->sbAudio && !pDmaEngine->sbAncF1 && !pDmaEngine->sbAncF2)
			{
				NTV2_MSG_PROGRAM("%s%d:%s%d: dmaZynqVideoCallback dma complete\n", DMA_MSG_ENGINE);
				set_bit(0, &pDmaEngine->transferDone);
				wake_up(&pDmaEngine->transferEvent);
			}
		}
		else
		{
			NTV2_MSG_ERROR("%s%d:%s%d: dmaZynqVideoCallback dma error %08x\n",
						   DMA_MSG_ENGINE, res);
		}

	}
	else
	{
		NTV2_MSG_ERROR("%s%d:%s%d: dmaZynqVideoCallback no video dma submitted\n",
					   DMA_MSG_ENGINE);
	}
}

static void dmaZynqAudioCallback(void* arg)
{
	PDMA_ENGINE pDmaEngine = (PDMA_ENGINE)arg;
	int res;

	if (pDmaEngine->sbAudio)
	{
		res = dma_async_is_tx_complete(pDmaEngine->dmaChannel, pDmaEngine->audioDmaCookie, NULL, NULL);
		if(res == DMA_COMPLETE)
		{
			if (!pDmaEngine->sbAncF1 && !pDmaEngine->sbAncF2)
			{
				NTV2_MSG_PROGRAM("%s%d:%s%d: dmaZynqAudioCallback dma complete\n", DMA_MSG_ENGINE);
				set_bit(0, &pDmaEngine->transferDone);
				wake_up(&pDmaEngine->transferEvent);
			}
		}
		else
		{
			NTV2_MSG_ERROR("%s%d:%s%d: dmaZynqAudioCallback dma error %08x\n",
						   DMA_MSG_ENGINE, res);
		}

	}
	else
	{
		NTV2_MSG_ERROR("%s%d:%s%d: dmaZynqAudioCallback no audio dma submitted\n",
					   DMA_MSG_ENGINE);
	}
}

static void dmaZynqAncF1Callback(void* arg)
{
	PDMA_ENGINE pDmaEngine = (PDMA_ENGINE)arg;
	int res;

	if (pDmaEngine->sbAncF1)
	{
		res = dma_async_is_tx_complete(pDmaEngine->dmaChannel, pDmaEngine->ancF1DmaCookie, NULL, NULL);
		if(res == DMA_COMPLETE)
		{
			if (!pDmaEngine->sbAncF2)
			{
				NTV2_MSG_PROGRAM("%s%d:%s%d: dmaZynqAncF1Callback dma complete\n", DMA_MSG_ENGINE);
				set_bit(0, &pDmaEngine->transferDone);
				wake_up(&pDmaEngine->transferEvent);
			}
		}
		else
		{
			NTV2_MSG_ERROR("%s%d:%s%d: dmaZynqAncF1Callback dma error %08x\n",
						   DMA_MSG_ENGINE, res);
		}

	}
	else
	{
		NTV2_MSG_ERROR("%s%d:%s%d: dmaZynqAncF1Callback no ancF1 dma submitted\n",
					   DMA_MSG_ENGINE);
	}
}

static void dmaZynqAncF2Callback(void* arg)
{
	PDMA_ENGINE pDmaEngine = (PDMA_ENGINE)arg;
	int res;

	if (pDmaEngine->sbAncF2)
	{
		res = dma_async_is_tx_complete(pDmaEngine->dmaChannel, pDmaEngine->ancF2DmaCookie, NULL, NULL);
		if(res == DMA_COMPLETE)
		{
			NTV2_MSG_PROGRAM("%s%d:%s%d: dmaZynqAncF2Callback dma complete\n", DMA_MSG_ENGINE);
			set_bit(0, &pDmaEngine->transferDone);
			wake_up(&pDmaEngine->transferEvent);
		}
		else
		{
			NTV2_MSG_ERROR("%s%d:%s%d: dmaZynqAncF2Callback dma error %08x\n",
						   DMA_MSG_ENGINE, res);
		}

	}
	else
	{
		NTV2_MSG_ERROR("%s%d:%s%d: dmaZynqAncF2Callback no ancF2 dma submitted\n",
					   DMA_MSG_ENGINE);
	}
}

static uint32_t microsecondsToJiffies(int64_t timeout)
{
	return (uint32_t)((timeout + (1000000/HZ - 1)) * HZ / 1000000);
}

static int64_t timeMicro(void)
{
	struct timespec64 ts64;

	ktime_get_real_ts64(&ts64);
	return (((int64_t)ts64.tv_sec * 1000000) + (ts64.tv_nsec / 1000));
}

#if 0
/**
 * zynqmp_dma_prep_slave_sg - prepare descriptors for a memory sg transaction
 * @dchan: DMA channel
 * @dst_sg: Destination scatter list
 * @dst_sg_len: Number of entries in destination scatter list
 * @src_sg: Source scatter list
 * @src_sg_len: Number of entries in source scatter list
 * @flags: transfer ack flags
 *
 * Return: Async transaction descriptor on success and NULL on failure
 */

#define to_chan(chan) container_of(chan, struct zynqmp_dma_chan, common)

struct dma_async_tx_descriptor *zynqmp_dma_prep_sg(
	struct dma_chan *dchan, struct scatterlist *dst_sg,
	unsigned int dst_sg_len, struct scatterlist *src_sg,
	unsigned int src_sg_len, unsigned long flags)
{
	struct zynqmp_dma_desc_sw *new, *first = NULL;
	struct zynqmp_dma_chan *chan = to_chan(dchan);
	void *desc = NULL, *prev = NULL;
	size_t len, dst_avail, src_avail;
	dma_addr_t dma_dst, dma_src;
	u32 desc_cnt = 0, i;
	struct scatterlist *sg;

	for_each_sg(src_sg, sg, src_sg_len, i)
		desc_cnt += DIV_ROUND_UP(sg_dma_len(sg),
					 ZYNQMP_DMA_MAX_TRANS_LEN);

	spin_lock_bh(&chan->lock);
	if (desc_cnt > chan->desc_free_cnt) {
		spin_unlock_bh(&chan->lock);
		dev_dbg(chan->dev, "chan %p descs are not available\n", chan);
		return NULL;
	}
	chan->desc_free_cnt = chan->desc_free_cnt - desc_cnt;
	spin_unlock_bh(&chan->lock);

	dst_avail = sg_dma_len(dst_sg);
	src_avail = sg_dma_len(src_sg);

	/* Run until we are out of scatterlist entries */
	while (true) {
		/* Allocate and populate the descriptor */
		new = zynqmp_dma_get_descriptor(chan);
		desc = (struct zynqmp_dma_desc_ll *)new->src_v;
		len = min_t(size_t, src_avail, dst_avail);
		len = min_t(size_t, len, ZYNQMP_DMA_MAX_TRANS_LEN);
		if (len == 0)
			goto fetch;
		dma_dst = sg_dma_address(dst_sg) + sg_dma_len(dst_sg) -
			dst_avail;
		dma_src = sg_dma_address(src_sg) + sg_dma_len(src_sg) -
			src_avail;

		zynqmp_dma_config_sg_ll_desc(chan, desc, dma_src, dma_dst,
					     len, prev);
		prev = desc;
		dst_avail -= len;
		src_avail -= len;

		if (!first)
			first = new;
		else
			list_add_tail(&new->node, &first->tx_list);
fetch:
		/* Fetch the next dst scatterlist entry */
		if (dst_avail == 0) {
			if (dst_sg_len == 0)
				break;
			dst_sg = sg_next(dst_sg);
			if (dst_sg == NULL)
				break;
			dst_sg_len--;
			dst_avail = sg_dma_len(dst_sg);
		}
		/* Fetch the next src scatterlist entry */
		if (src_avail == 0) {
			if (src_sg_len == 0)
				break;
			src_sg = sg_next(src_sg);
			if (src_sg == NULL)
				break;
			src_sg_len--;
			src_avail = sg_dma_len(src_sg);
		}
	}

	zynqmp_dma_desc_config_eod(chan, desc);
	first->async_tx.flags = flags;
	return &first->async_tx;
}
#endif
