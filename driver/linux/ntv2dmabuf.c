/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2026 Ariel Molina <ariel@edis.mx>
 */
//==========================================================================
//
//  ntv2dmabuf.c
//
//  Host memory backend that takes its scatter list from an imported
//  dma-buf instead of get_user_pages(). The importer never touches cpu
//  cache maintenance: coherency belongs to the exporter and userspace
//  brackets its own access with DMA_BUF_IOCTL_SYNC.
//
//==========================================================================

#include <linux/version.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/errno.h>
#include <linux/pci.h>
#include <linux/scatterlist.h>
#include <linux/dma-buf.h>

#include "buildenv.h"
#include "ntv2enums.h"
#include "ntv2audiodefines.h"
#include "ntv2videodefines.h"

#include "ntv2publicinterface.h"
#include "ntv2linuxpublicinterface.h"

#include "registerio.h"
#include "ntv2stream.h"
#include "ntv2dma.h"
#include "ntv2dmabuf.h"

#if IS_ENABLED(CONFIG_DMA_SHARED_BUFFER)

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6,13,0))
MODULE_IMPORT_NS("DMA_BUF");
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(5,16,0))
MODULE_IMPORT_NS(DMA_BUF);
#endif

typedef struct ntv2_dmabuf_context
{
	struct dma_buf*				dmabuf;
	struct dma_buf_attachment*	attach;
	struct sg_table*			sgt;
} NTV2_DMABUF_CONTEXT, *PNTV2_DMABUF_CONTEXT;

static struct sg_table* ntv2_dmabuf_map(struct dma_buf_attachment* attach,
										enum dma_data_direction direction)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6,2,0))
	return dma_buf_map_attachment_unlocked(attach, direction);
#else
	return dma_buf_map_attachment(attach, direction);
#endif
}

static void ntv2_dmabuf_unmap(struct dma_buf_attachment* attach,
							  struct sg_table* sgt,
							  enum dma_data_direction direction)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6,2,0))
	dma_buf_unmap_attachment_unlocked(attach, sgt, direction);
#else
	dma_buf_unmap_attachment(attach, sgt, direction);
#endif
}

int ntv2_dmabuf_get_pages(PDMA_PAGE_BUFFER pBuffer, PVOID pAddress,
						  ULWord size, ULWord direction)
{
	PNTV2_DMABUF_CONTEXT pDmabufContext;
	struct dma_buf* dmabuf;

	if ((pBuffer == NULL) || (size == 0))
		return -EINVAL;

	if (pBuffer->dmabufFd < 0)
		return -EINVAL;

	dmabuf = dma_buf_get(pBuffer->dmabufFd);
	if (IS_ERR(dmabuf))
	{
		pr_err("ntv2dmabuf: get fd %d failed %ld\n",
			   pBuffer->dmabufFd, PTR_ERR(dmabuf));
		return (int)PTR_ERR(dmabuf);
	}

	if (dmabuf->size < (size_t)size)
	{
		pr_err("ntv2dmabuf: dma-buf size %zu smaller than requested %d\n",
			   dmabuf->size, size);
		dma_buf_put(dmabuf);
		return -EINVAL;
	}

	pDmabufContext = vmalloc(sizeof(NTV2_DMABUF_CONTEXT));
	if (pDmabufContext == NULL)
	{
		dma_buf_put(dmabuf);
		return -ENOMEM;
	}
	memset(pDmabufContext, 0, sizeof(NTV2_DMABUF_CONTEXT));
	pDmabufContext->dmabuf = dmabuf;
	pBuffer->dmabufContext = (void*)pDmabufContext;

	pBuffer->pUserAddress = pAddress;
	pBuffer->userSize = size;
	pBuffer->direction = direction;
	pBuffer->numPages = (ULWord)((size + PAGE_SIZE - 1) / PAGE_SIZE);
	pBuffer->pageLock = true;

	return 0;
}

void ntv2_dmabuf_put_pages(PDMA_PAGE_BUFFER pBuffer)
{
	PNTV2_DMABUF_CONTEXT pDmabufContext;

	if (pBuffer == NULL)
		return;

	pDmabufContext = (PNTV2_DMABUF_CONTEXT)pBuffer->dmabufContext;
	if (pDmabufContext == NULL)
		return;

	if (pDmabufContext->dmabuf != NULL)
		dma_buf_put(pDmabufContext->dmabuf);

	pBuffer->dmabufContext = NULL;
	vfree(pDmabufContext);

	pBuffer->pageLock = false;
}

int ntv2_dmabuf_map_pages(struct pci_dev* pci_dev, PDMA_PAGE_BUFFER pBuffer)
{
	PNTV2_DMABUF_CONTEXT pDmabufContext;
	enum dma_data_direction direction;
	struct scatterlist* sgIn;
	struct scatterlist* pSgList;
	ULWord64 totalLength = 0;
	ULWord numEntries;
	int i;

	if ((pci_dev == NULL) || (pBuffer == NULL))
		return -EINVAL;

	pDmabufContext = (PNTV2_DMABUF_CONTEXT)pBuffer->dmabufContext;
	if ((pDmabufContext == NULL) || (pDmabufContext->dmabuf == NULL))
		return -EPERM;

	direction = (enum dma_data_direction)pBuffer->direction;

	// a static attachment pins the buffer for the life of the registration
	pDmabufContext->attach = dma_buf_attach(pDmabufContext->dmabuf, &pci_dev->dev);
	if (IS_ERR(pDmabufContext->attach))
	{
		int ret = (int)PTR_ERR(pDmabufContext->attach);
		pr_err("ntv2dmabuf: attach failed %d\n", ret);
		pDmabufContext->attach = NULL;
		return ret;
	}

	pDmabufContext->sgt = ntv2_dmabuf_map(pDmabufContext->attach, direction);
	if (IS_ERR(pDmabufContext->sgt))
	{
		int ret = (int)PTR_ERR(pDmabufContext->sgt);
		pr_err("ntv2dmabuf: map attachment failed %d\n", ret);
		pDmabufContext->sgt = NULL;
		dma_buf_detach(pDmabufContext->dmabuf, pDmabufContext->attach);
		pDmabufContext->attach = NULL;
		return ret;
	}

	numEntries = (ULWord)pDmabufContext->sgt->nents;
	if ((numEntries == 0) || (numEntries > DMA_DESCRIPTOR_PAGES_MAX))
	{
		pr_err("ntv2dmabuf: bad scatter entry count %d\n", numEntries);
		goto out_unmap;
	}

	// the descriptor generator indexes the scatter list linearly, so the
	// imported table is copied into a flat array: a table with more than
	// SG_MAX_SINGLE_ALLOC entries is chained and cannot be indexed
	pSgList = vmalloc(numEntries * sizeof(struct scatterlist));
	if (pSgList == NULL)
		goto out_unmap;

	NTV2_LINUX_SG_INIT_TABLE_FUNC(pSgList, numEntries);

	i = 0;
	for_each_sg(pDmabufContext->sgt->sgl, sgIn, pDmabufContext->sgt->nents, i)
	{
		dma_addr_t address = sg_dma_address(sgIn);
		unsigned int length = sg_dma_len(sgIn);

		if (length == 0)
		{
			pr_err("ntv2dmabuf: zero length scatter entry %d\n", i);
			vfree(pSgList);
			goto out_unmap;
		}

		pSgList[i].offset = 0;
		pSgList[i].dma_address = address;
		pSgList[i].length = length;
#ifdef CONFIG_NEED_SG_DMA_LENGTH
		pSgList[i].dma_length = length;
#endif
		totalLength += length;
	}

	if (totalLength < (ULWord64)pBuffer->userSize)
	{
		pr_err("ntv2dmabuf: mapped %lld bytes, buffer needs %d\n",
			   totalLength, pBuffer->userSize);
		vfree(pSgList);
		goto out_unmap;
	}

	pBuffer->pSgList = pSgList;
	pBuffer->sgListSize = numEntries;
	pBuffer->numSgs = numEntries;
	pBuffer->sgMap = true;
	pBuffer->sgHost = false;

	return 0;

out_unmap:
	ntv2_dmabuf_unmap(pDmabufContext->attach, pDmabufContext->sgt, direction);
	pDmabufContext->sgt = NULL;
	dma_buf_detach(pDmabufContext->dmabuf, pDmabufContext->attach);
	pDmabufContext->attach = NULL;
	return -EPERM;
}

void ntv2_dmabuf_unmap_pages(struct pci_dev* pci_dev, PDMA_PAGE_BUFFER pBuffer)
{
	PNTV2_DMABUF_CONTEXT pDmabufContext;

	(void)pci_dev;

	if (pBuffer == NULL)
		return;

	pDmabufContext = (PNTV2_DMABUF_CONTEXT)pBuffer->dmabufContext;
	if (pDmabufContext == NULL)
		return;

	if (pDmabufContext->sgt != NULL)
	{
		ntv2_dmabuf_unmap(pDmabufContext->attach,
						  pDmabufContext->sgt,
						  (enum dma_data_direction)pBuffer->direction);
		pDmabufContext->sgt = NULL;
	}

	if (pDmabufContext->attach != NULL)
	{
		dma_buf_detach(pDmabufContext->dmabuf, pDmabufContext->attach);
		pDmabufContext->attach = NULL;
	}

	if (pBuffer->pSgList != NULL)
		vfree(pBuffer->pSgList);
	pBuffer->pSgList = NULL;
	pBuffer->sgListSize = 0;
	pBuffer->numSgs = 0;
	pBuffer->sgMap = false;
	pBuffer->sgHost = false;
}

#else

int ntv2_dmabuf_get_pages(PDMA_PAGE_BUFFER pBuffer, PVOID pAddress,
						  ULWord size, ULWord direction)
{
	(void)pBuffer;
	(void)pAddress;
	(void)size;
	(void)direction;
	return -EOPNOTSUPP;
}

void ntv2_dmabuf_put_pages(PDMA_PAGE_BUFFER pBuffer)
{
	(void)pBuffer;
}

int ntv2_dmabuf_map_pages(struct pci_dev* pci_dev, PDMA_PAGE_BUFFER pBuffer)
{
	(void)pci_dev;
	(void)pBuffer;
	return -EOPNOTSUPP;
}

void ntv2_dmabuf_unmap_pages(struct pci_dev* pci_dev, PDMA_PAGE_BUFFER pBuffer)
{
	(void)pci_dev;
	(void)pBuffer;
}

#endif
