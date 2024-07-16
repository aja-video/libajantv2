/*****************************************************************************
 * main.c
 * July 10, 2015
 *
 * Copyright 2015 - AJA Video Systems, Inc
 * Proprietary and confidential information
 *
 *****************************************************************************/

#include <linux/version.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/kobject.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/kernel.h>       /* printk() */
#include <linux/slab.h>         /* kmalloc() */
#include <linux/errno.h>        /* error codes */
#include <linux/types.h>        /* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>        /* O_ACCMODE */
#include <linux/aio.h>
#include <asm/uaccess.h>
#include <linux/device.h>
#include <linux/poll.h>
#include <linux/sched.h>

#ifdef AJA_RDMA
#include <nv-p2p.h>
#include "ajatypes.h"
#include "buildenv.h"
#include "ntv2enums.h"
#include "ntv2videodefines.h"
#include "ntv2audiodefines.h"
#include "ntv2publicinterface.h"
#include "ntv2linuxpublicinterface.h"
#include "registerio.h"
#include "ntv2dma.h"

#ifdef AJA_IGPU
#define GPU_PAGE_SHIFT	12
#else
#define GPU_PAGE_SHIFT	16
#endif

#define GPU_PAGE_SIZE	(((ULWord64)1) << GPU_PAGE_SHIFT)
#define GPU_PAGE_OFFSET	(GPU_PAGE_SIZE - 1)
#define GPU_PAGE_MASK	(~GPU_PAGE_OFFSET)

typedef struct rdma_page_buffer {
	ULWord64				address;		// rdma gpu aligned buffer address
	ULWord64				offset;			// rdma gpu aligned offset
	ULWord64				len;			// rdma buffer length
	ULWord64				alignedLen;		// rdma gpu aligned buffer length
	struct nvidia_p2p_page_table*	page;
	struct nvidia_p2p_dma_mapping*	map;
} RDMA_PAGE_BUFFER, *PRDMA_PAGE_BUFFER;

extern void ntv2_set_rdma_fops(struct ntv2_page_fops* fops);

static void rdmaFreeCallback(void* data);
static void dmaSgSetRdmaPage(struct scatterlist* pSg, struct nvidia_p2p_dma_mapping	*rdmaMap,
							 int index, ULWord64 length, ULWord64 offset);

int ntv2_rdma_get_pages(PDMA_PAGE_BUFFER pBuffer,
                        PVOID pAddress, ULWord size, ULWord direction)
{
    PRDMA_PAGE_BUFFER pRdmaBuffer = NULL;
	ULWord64 address = (unsigned long)pAddress;
	ULWord64 rdmaAddress = address & GPU_PAGE_MASK;
	ULWord64 rdmaOffset = address & GPU_PAGE_OFFSET;
	ULWord64 rdmaLen = size;
#ifdef AJA_IGPU		
	ULWord64 rdmaAlignedLen = (rdmaOffset + rdmaLen + GPU_PAGE_SIZE - 1) & GPU_PAGE_MASK;
#else
	ULWord64 rdmaAlignedLen = address + size - rdmaAddress;
#endif		
	struct nvidia_p2p_page_table* rdmaPage = NULL;
	int ret = -1;

    pRdmaBuffer = vmalloc(sizeof(struct rdma_page_buffer));
    if (pRdmaBuffer == NULL)
    {
        return -ENOMEM;
    }
    pBuffer->rdmaContext = (void*)pRdmaBuffer;

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
        return ret;
    }

    pBuffer->pUserAddress = pAddress;
    pBuffer->userSize = size;
    pBuffer->direction = direction;
    pRdmaBuffer->address = rdmaAddress;
    pRdmaBuffer->offset = rdmaOffset;
    pRdmaBuffer->len = rdmaLen;
    pRdmaBuffer->alignedLen = rdmaAlignedLen;
    pRdmaBuffer->page = rdmaPage;
    pBuffer->numPages = rdmaPage->entries;
    pBuffer->pageLock = true;
		
    return 0;
}

void ntv2_rdma_put_pages(PDMA_PAGE_BUFFER pBuffer)
{
    PRDMA_PAGE_BUFFER pRdmaBuffer = (PRDMA_PAGE_BUFFER)pBuffer->rdmaContext;

    if (pRdmaBuffer == NULL) return;
    
    if ((pRdmaBuffer->address == 0) || (pRdmaBuffer->page == NULL))
        return;
			
    nvidia_p2p_put_pages(
#ifndef AJA_IGPU				
        0, 0,
        pRdmaBuffer->address,
#endif								
        pRdmaBuffer->page);

    rdmaFreeCallback(pBuffer);
    return;
}

int ntv2_rdma_map_pages(struct pci_dev* pci_dev, PDMA_PAGE_BUFFER pBuffer)
{
    PRDMA_PAGE_BUFFER pRdmaBuffer = (PRDMA_PAGE_BUFFER)pBuffer->rdmaContext;
    ULWord numEntries;
    ULWord64 pageOffset;
    ULWord64 count;
    int i;
    int ret = -1;

    if (pRdmaBuffer == NULL) return -EPERM;

#ifdef AJA_IGPU
    ret = nvidia_p2p_dma_map_pages(&pci_dev->dev,
                                   pRdmaBuffer->page,
                                   &pRdmaBuffer->map,
                                   (pBuffer->direction == PCI_DMA_TODEVICE)? DMA_TO_DEVICE : DMA_FROM_DEVICE);
#else
    ret = nvidia_p2p_dma_map_pages(pci_dev,
                                   pRdmaBuffer->page,
                                   &pRdmaBuffer->map);
#endif
    if (ret < 0)
    {
        return ret;
    }

    if ((pRdmaBuffer->map == NULL) || (pRdmaBuffer->map->entries == 0))
    {
        return -EPERM;
    }
				
    // alloc scatter list
    numEntries = pRdmaBuffer->map->entries;
    pBuffer->pSgList = vmalloc(numEntries * sizeof(struct scatterlist));
    if (pBuffer->pSgList == NULL)
    {
        return -ENOMEM;
    }
    pBuffer->sgListSize = numEntries;

    // clear segment list
    NTV2_LINUX_SG_INIT_TABLE_FUNC(pBuffer->pSgList, pBuffer->sgListSize);

    // offset on first page
    pageOffset = pRdmaBuffer->offset;

    // build scatter list
    count = pRdmaBuffer->len;
    for (i = 0; i < numEntries; i++)
    {
        if (count > 0)
        {
            dmaSgSetRdmaPage(&pBuffer->pSgList[i],
                             pRdmaBuffer->map,
                             i,
                             count < (GPU_PAGE_SIZE - pageOffset)? count : (GPU_PAGE_SIZE - pageOffset),
                             pageOffset);
        }
        count = (count < GPU_PAGE_SIZE)? 0 : (count - GPU_PAGE_SIZE);
        pageOffset = 0;
    }

    pBuffer->numSgs = (ULWord)numEntries;
    pBuffer->sgMap = true;
    pBuffer->sgHost = false;

    return 0;
}

void ntv2_rdma_unmap_pages(struct pci_dev* pci_dev, PDMA_PAGE_BUFFER pBuffer)
{
    PRDMA_PAGE_BUFFER pRdmaBuffer = (PRDMA_PAGE_BUFFER)pBuffer->rdmaContext;

    if (pRdmaBuffer == NULL) return;

    if ((pRdmaBuffer->page != NULL) && (pRdmaBuffer->map != NULL))
    {
#ifdef AJA_IGPU
        nvidia_p2p_dma_unmap_pages(pRdmaBuffer->map);
#else
        nvidia_p2p_dma_unmap_pages(pci_dev,
                                   pRdmaBuffer->page,
                                   pRdmaBuffer->map);
#endif
        if (pBuffer->pSgList != NULL)
            vfree(pBuffer->pSgList);
        pBuffer->pSgList = NULL;
        pBuffer->sgListSize = 0;
        pBuffer->numSgs = 0;
        pBuffer->sgMap = false;
        pBuffer->sgHost = false;
        return;
    }
}

static void rdmaFreeCallback(void* data)
{
	PDMA_PAGE_BUFFER pBuffer = (PDMA_PAGE_BUFFER)data;
    PRDMA_PAGE_BUFFER pRdmaBuffer = (PRDMA_PAGE_BUFFER)pBuffer->rdmaContext;
	struct nvidia_p2p_page_table* rdmaPage;

	rdmaPage = xchg(&pRdmaBuffer->page, NULL);
	if (rdmaPage != NULL)
    {
        nvidia_p2p_free_page_table(rdmaPage);
    }

    if (pBuffer->rdmaContext != NULL)
        vfree(pBuffer->rdmaContext);
    pBuffer->rdmaContext = NULL;    

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

int ntv2_rdma_init(void)
{
#ifdef AJA_RDMA
    struct ntv2_page_fops rdma_fops = 
    {
        ntv2_rdma_get_pages,
        ntv2_rdma_put_pages,
        ntv2_rdma_map_pages,
        ntv2_rdma_unmap_pages
    };

    // request the unified virtual memory driver
    request_module_nowait("nvidia-uvm");

    // set rdma functions in the ntv2 driver
    ntv2_set_rdma_fops(&rdma_fops);
    
#ifdef NVIDIA_PROPRIETARY
    printk(KERN_INFO "ntv2_rdma_init: RDMA proprietary\n");
#else
    printk(KERN_INFO "ntv2_rdma_init: RDMA open source\n");
#endif    
#else
    printk(KERN_INFO "ntv2_rdma_init: RDMA not supported\n");
#endif
    return 0;
}	

void ntv2_rdma_exit(void)
{
#ifdef AJA_RDMA
    // clear the rdma functions in the ntv2 driver
    ntv2_set_rdma_fops(NULL);
#endif    
    printk(KERN_INFO "ntv2_rdma_exit\n");
    return;
}

module_init(ntv2_rdma_init);
module_exit(ntv2_rdma_exit);

#ifdef NVIDIA_PROPRIETARY
MODULE_LICENSE("Proprietary");
#else
MODULE_LICENSE("Dual MIT/GPL");
#endif
MODULE_AUTHOR("AJA");
MODULE_DESCRIPTION("Interface between AJA NTV2 driver and NVidia RDMA");

