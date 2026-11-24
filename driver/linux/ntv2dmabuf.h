/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2026 Ariel Molina <ariel@edis.mx>
 */
////////////////////////////////////////////////////////////
//
// Filename: ntv2dmabuf.h
// Purpose:	 ntv2 driver dma-buf host memory backend
//
///////////////////////////////////////////////////////////////

#ifndef NTV2DMABUF_HEADER
#define NTV2DMABUF_HEADER

struct pci_dev;

// import the dma-buf named by pBuffer->dmabufFd and take a reference to it
int ntv2_dmabuf_get_pages(PDMA_PAGE_BUFFER pBuffer, PVOID pAddress,
						  ULWord size, ULWord direction);

// release the dma-buf reference
void ntv2_dmabuf_put_pages(PDMA_PAGE_BUFFER pBuffer);

// attach to the device and map the dma-buf for dma
int ntv2_dmabuf_map_pages(struct pci_dev* pci_dev, PDMA_PAGE_BUFFER pBuffer);

// unmap and detach the dma-buf
void ntv2_dmabuf_unmap_pages(struct pci_dev* pci_dev, PDMA_PAGE_BUFFER pBuffer);

#endif
