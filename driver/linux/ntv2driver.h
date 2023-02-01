/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
///////////////////////////////////////////////////////////////
//
// NTV2 Linux v2.6 Device Driver for AJA OEM boards.
//
////////////////////////////////////////////////////////////
//
// Filename: ntv2driver.h
// Purpose:	 Main headerfile for driver.
// Notes:	 PCI Device IDs, memory sizes, fops prototypes
//
///////////////////////////////////////////////////////////////
#ifndef NTV2_DRIVER_HEADER
#define NTV2_DRIVER_HEADER

#include <linux/fs.h>

// Defines
#define NTV2_MAJOR 0

#define ntv2_spin_lock_irqsave(l, f) spin_lock_irqsave((l), (f))
#define ntv2_spin_unlock_irqrestore(l, f) spin_unlock_irqrestore((l), (f))

// the VendorID/DeviceID for the hardware
#define NTV2_VENDOR_ID                      0xF1D0
#define NTV2_DEVICE_ID_CORVID1              0xDAFE
#define NTV2_DEVICE_ID_LHI					0xDAFF
#define NTV2_DEVICE_ID_IOEXPRESS			0xDB00
#define NTV2_DEVICE_ID_CORVID22				0xDB01
#define NTV2_DEVICE_ID_KONA3G				0xDB02
#define NTV2_DEVICE_ID_CORVID3G				0xDB03
#define NTV2_DEVICE_ID_KONA3G_QUAD			0xDB04
#define NTV2_DEVICE_ID_LHE_PLUS				0xDB05
#define NTV2_DEVICE_ID_IOXT					0xDB06
#define NTV2_DEVICE_ID_KONA3G_P2P			0xDB07
#define NTV2_DEVICE_ID_KONA3G_QUAD_P2P		0xDB08
#define NTV2_DEVICE_ID_CORVID24				0xDB09
#define NTV2_DEVICE_ID_TTAP					0xDB11 
#define NTV2_DEVICE_ID_KONA4				0xEB0B
#define NTV2_DEVICE_ID_KONA4_UFC			0xEB0C
#define NTV2_DEVICE_ID_CORVID88				0xEB0D
#define NTV2_DEVICE_ID_CORVID44				0xEB0E
#define NTV2_DEVICE_ID_CORVIDHEVC_MB31      0xEB15
#define NTV2_DEVICE_ID_CORVIDHEVC_K7        0xEB16
#define NTV2_DEVICE_ID_CORVIDHDBT			0xEB18
#define NTV2_DEVICE_ID_CORVID446			0xEB19
#define NTV2_DEVICE_ID_KONAIP_CH1SFP		0xEB1A
#define NTV2_DEVICE_ID_KONAIP_PHANTOM		0xEB1B 
#define NTV2_DEVICE_ID_KONAIP_CH2SFP		0xEB1C
#define NTV2_DEVICE_ID_IO4KPLUS				0xEB1D
#define NTV2_DEVICE_ID_IOIP                 0xEB1E
#define NTV2_DEVICE_ID_KONA5    			0xEB1F
#define NTV2_DEVICE_ID_KONA5IP				0xEB20
#define NTV2_DEVICE_ID_KONA1				0xEB23
#define NTV2_DEVICE_ID_KONAHDMI				0xEB24
#define NTV2_DEVICE_ID_CORVID44_12g			0xEB25
#define NTV2_DEVICE_ID_TTAPPRO				0xEB26 
#define NTV2_DEVICE_ID_IOX3					0xEB27 
#define PRIVATIZE(name)						AJANTV2_ ## name


// The kernel has only one namespace, so make the NTV2Params global
// structure have a unique name per boardtype.  Access function for
// this per-board data is provided also.
#define NTV2Params PRIVATIZE(NTV2Params)
#define NTV2ModuleParams PRIVATIZE(NTV2ModuleParams)

#define MapFrameBuffers PRIVATIZE(MapFrameBuffers)
#define NumDmaDriverBuffers PRIVATIZE(NumDmaDriverBuffers)

int init_module(void);
void cleanup_module(void);

loff_t      ntv2_lseek(struct file *file, loff_t off, int whence);
ssize_t     ntv2_read(struct file *file, char *buf, size_t count, loff_t *f_pos);
ssize_t     ntv2_write(struct file *file, const char *buf, size_t count, loff_t *f_pos);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36))
long        ntv2_ioctl(struct file *file, unsigned int cmd,  unsigned long arg);
#else
int         ntv2_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg);
#endif
int         ntv2_mmap(struct file *file,struct vm_area_struct* vma);
int         ntv2_open(struct inode *minode, struct file *mfile);
int         ntv2_release(struct inode *minode, struct file *mfile);


#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,18,0))
#define smp_mb__before_clear_bit smp_mb__before_atomic
#define smp_mb__after_clear_bit  smp_mb__after_atomic
#endif


#endif	// NTV2_DRIVER_HEADER

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  tab-width: 4
 * End:
 */
