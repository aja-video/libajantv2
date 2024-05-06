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
#ifdef NVIDIA_PROPRIETARY
#include <linux/ntv2rdma.h>

extern void ntv2_set_rdma_fops(struct ntv2_rdma_fops* fops);

static struct ntv2_rdma_fops rdma_fops = 
{
    .get_pages = nvidia_p2p_get_pages,
    .put_pages = nvidia_p2p_put_pages,
    .free_page_table = nvidia_p2p_free_page_table,
    .dma_map_pages = nvidia_p2p_dma_map_pages,
    .dma_unmap_pages = nvidia_p2p_dma_unmap_pages
};

#endif
#endif

int ntv2_rdma_init(void)
{
#ifdef AJA_RDMA
#ifdef NVIDIA_PROPRIETARY
    printk(KERN_INFO "ntv2_rmda_init: configuring RDMA\n");
    ntv2_set_rdma_fops(&rdma_fops);
#else
    printk(KERN_INFO "ntv2_rmda_init: RDMA open source\n");
#endif    
#else
    printk(KERN_INFO "ntv2_rmda_init: RDMA not supported\n");
#endif
    return 0;
}	

void ntv2_rdma_exit(void)
{
    printk(KERN_INFO "ntv2_rmda_exit\n");
    return;
}

module_init(ntv2_rdma_init);
module_exit(ntv2_rdma_exit);

MODULE_LICENSE("Proprietary");
MODULE_AUTHOR("AJA");
MODULE_DESCRIPTION("Interface between AJA NTV2 driver and NVidia RDMA");

