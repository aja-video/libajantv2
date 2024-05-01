////////////////////////////////////////////////////////////
//
// Filename: ntv2rdma.h
// Purpose:	 ntv2 driver rdma defines
//
///////////////////////////////////////////////////////////////

#ifndef NTV2RDMA_HEADER
#define NTV2RDMA_HEADER

#ifdef AJA_RDMA
#include <nv-p2p.h>

typedef int (*rdma_get_pages)(
    uint64_t p2p_token, uint32_t va_space,
#ifndef AJA_IGPU
    uint64_t virtual_address, uint64_t length,
#endif                              
    struct nvidia_p2p_page_table **page_table,
    void (*free_callback)(void *data), void *data);

typedef int (*rdma_put_pages)(
#ifndef AJA_IGPU
    uint64_t p2p_token,
    uint32_t va_space, uint64_t virtual_address,
#endif    
    struct nvidia_p2p_page_table *page_table);

typedef int (*rdma_free_page_table)(struct nvidia_p2p_page_table *page_table);

#ifdef AJA_IGPU

typedef int (*rdma_dma_map_pages)(
    struct device *dev,
    struct nvidia_p2p_page_table *page_table,
    struct nvidia_p2p_dma_mapping **dma_mapping,
    enum dma_data_direction dir);

typedef int (*rdma_dma_unmap_pages)(
    struct nvidia_p2p_dma_mapping *dma_mapping);

#else

typedef int (*rdma_dma_map_pages)(
    struct pci_dev *peer,
    struct nvidia_p2p_page_table *page_table,
    struct nvidia_p2p_dma_mapping **dma_mapping);

typedef int (*rdma_dma_unmap_pages)(
    struct pci_dev *peer,
    struct nvidia_p2p_page_table *page_table,
    struct nvidia_p2p_dma_mapping *dma_mapping);

#endif

struct ntv2_rdma_fops
{
    rdma_get_pages        get_pages;
    rdma_put_pages        put_pages;
    rdma_free_page_table  free_page_table;
    rdma_dma_map_pages    dma_map_pages;
    rdma_dma_unmap_pages  dma_unmap_pages;
};

void ntv2_set_rdma_fops(struct ntv2_rdma_fops* fops);

#endif

#endif
