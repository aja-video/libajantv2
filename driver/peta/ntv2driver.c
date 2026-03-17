/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
//////////////////////////////////////////////////////////////
//
// NTV2 Linux v2.6+ Device Driver for AJA OEM boards.
//
// Boards supported include:
//
// Filename: ntv2driver.c
// Purpose:	 Main module file.  Load, unload, fops, ioctls.
//
///////////////////////////////////////////////////////////////

#if defined(CONFIG_SMP)
#define __SMP__
#endif

/*needed by kernel 2.6.18*/
#ifndef CONFIG_HZ
#include <linux/autoconf.h>
#endif


// Note1: Device driver version lives in NTV2_LINUX_DRIVER_VERSION in ntv2linuxpublicinterface.h

#include <linux/module.h>
#include <linux/irq.h>		// For set_irq_type() etc.
#include <linux/sched.h>
#include <asm/uaccess.h>
#include <linux/pci.h>
#include <linux/platform_device.h>
#include <linux/of.h>

#include <linux/moduleparam.h>

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <asm/delay.h>
#include <asm/page.h>
#include <linux/ioport.h>
#include <asm/io.h>
#include <asm/pgtable.h>
#include <linux/vmalloc.h>
#include <linux/proc_fs.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,9,0))
#include <linux/seq_file.h>
#endif
#include <linux/reboot.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,20,0))
#include <linux/pci-p2pdma.h>
#endif

#include "ajatypes.h"
#include "ntv2enums.h"
#include "ntv2videodefines.h"
#include "ntv2audiodefines.h"
#include "ntv2publicinterface.h"
#include "ntv2linuxpublicinterface.h"
#include "ntv2devicefeatures.h"
#include "ntv2driverprocamp.h"
#include "ntv2driver.h"
#include "driverdbg.h"
#include "registerio.h"
#include "ntv2dma.h"

#include "ntv2driverdbgmsgctl.h"
#include "ntv2driverstatus.h"
#include "ntv2kona2.h"

#include "ntv2hdmiin.h"
#include "ntv2serial.h"
#include "ntv2hdmiout4.h"
#include "ntv2genlock.h"
#include "../ntv2kona.h"

#include "buildenv.h"

#if  !defined(x86_64) && !defined(aarch64)
#error "*** AJA driver must be built 64 bit ***"
#endif

//#define RHEL4
//#undef RHEL4

#define AJA_CREATE_DEVICE_NODES

#define NTV2_MODULE_NAME "ntv2mod"
#define NTV2_DRIVER_NAME "ajantv2"

/*******************************/
/* Module macros, params, etc. */
/*******************************/
MODULE_AUTHOR("Bill Bowen and Shaun Case and Jeff Coffin");

MODULE_LICENSE("Dual MIT/GPL");

// For boards that support a serial port
// -1 = never make a serial port
// 0 = make serial port for linux versions that work correctly
// 1 = always make a serial port
int MakeSerial = 0;

#if defined RHEL4 || (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,9))
module_param(MakeSerial, int, S_IRUGO);
#else
MODULE_PARM(MakeSerial, "i");
#endif

// If the following is set to 0, frame buffers will not be mapped by
// the driver.  This means no PIO access (DMA only) but also allows
// more RAM to be used in the system.
int MapFrameBuffers = 1;

#if defined RHEL4 || (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,9))
module_param(MapFrameBuffers, int, S_IRUGO);
#else
MODULE_PARM(MapFrameBuffers, "i");
#endif

// DriverMode - configure the driver initialization mode
//   all         = support all driver functions (default)
//   register    = only support register access
//   genlock     = register access with genlock initialization
static char* DriverMode = "all";
module_param(DriverMode, charp, S_IRUGO);

/***************************/
/* Local defines and types */
/***************************/

#define STRMAX 81			// max size for string arrays ....
//#define INTERRUPT_WAITFOR_BIT  // restore interrupt wait using single bit

// Declare a type for a pointer to an IRQ function
typedef irqreturn_t (*fp_irq_t)(int, void *);

irqreturn_t ntv2_fpga_irq(int irq, void *dev_id);

typedef struct
{
	fp_irq_t irq_func;
	unsigned long flags;
	unsigned long irq_type;
} ntv2_irq_desc_t;

ntv2_irq_desc_t ntv2_irq_arr[] =
{
	{ ntv2_fpga_irq, NTV2_LINUX_IRQ_SHARED_FLAG, IRQ_TYPE_NONE},
	{ NULL, 0 }
};

#define ANC_EXT_1_OFFSET 0x1000
#define ANC_EXT_2_OFFSET 0x1040
#define ANC_EXT_3_OFFSET 0x1080
#define ANC_EXT_4_OFFSET 0x10c0
#define ANC_EXT_5_OFFSET 0x1100
#define ANC_EXT_6_OFFSET 0x1140
#define ANC_EXT_7_OFFSET 0x1180
#define ANC_EXT_8_OFFSET 0x11c0

static const ULWord	gChannelToAncExtOffset[] = { ANC_EXT_1_OFFSET, ANC_EXT_2_OFFSET, ANC_EXT_3_OFFSET, ANC_EXT_4_OFFSET,
	ANC_EXT_5_OFFSET, ANC_EXT_6_OFFSET, ANC_EXT_7_OFFSET, ANC_EXT_8_OFFSET, 0 };
	
static const ULWord	gChannelToSDIOutVPIDTransferCharacteristics[] = { kVRegNTV2VPIDTransferCharacteristics, kVRegNTV2VPIDTransferCharacteristics2, kVRegNTV2VPIDTransferCharacteristics3, kVRegNTV2VPIDTransferCharacteristics4,
	kVRegNTV2VPIDTransferCharacteristics5, kVRegNTV2VPIDTransferCharacteristics6, kVRegNTV2VPIDTransferCharacteristics7, kVRegNTV2VPIDTransferCharacteristics8, 0 };

static const ULWord	gChannelToSDIOutVPIDColorimetry[] = { kVRegNTV2VPIDColorimetry, kVRegNTV2VPIDColorimetry2, kVRegNTV2VPIDColorimetry3, kVRegNTV2VPIDColorimetry4,
	kVRegNTV2VPIDColorimetry5, kVRegNTV2VPIDColorimetry6, kVRegNTV2VPIDColorimetry7, kVRegNTV2VPIDColorimetry8, 0 };

static const ULWord	gChannelToSDIOutVPIDLuminance[] = { kVRegNTV2VPIDLuminance, kVRegNTV2VPIDLuminance, kVRegNTV2VPIDLuminance, kVRegNTV2VPIDLuminance,
	kVRegNTV2VPIDLuminance, kVRegNTV2VPIDLuminance, kVRegNTV2VPIDLuminance, kVRegNTV2VPIDLuminance, 0 };

	
typedef struct VirtualDataNode VirtualDataNode;

struct VirtualDataNode
{
    VirtualDataNode *next;
    VirtualDataNode *prev;
    ULWord          tag;
    ULWord          size;
    ULWord          data;
};

/*********************************************/
/* Prototypes for private utility functions. */
/*********************************************/

static void SetupBoard(ULWord deviceNumber);

static int ValidateAjaNTV2Message(NTV2_HEADER * pHeaderIn);
static int DoMessageSDIInStatictics(ULWord deviceNumber, NTV2Buffer * pInStatistics, void * pOutBuff);
static int DoMessageBankAndRegisterWrite(ULWord deviceNumber, NTV2RegInfo * pInReg, NTV2RegInfo * pInBank);
static int DoMessageBankAndRegisterRead(ULWord deviceNumber, NTV2RegInfo * pInReg, NTV2RegInfo * pInBank);
static int DoMessageAutoCircFrame(ULWord deviceNumber, FRAME_STAMP * pInOutFrameStamp, NTV2_RP188 * pTimecodeArray);
static int DoMessageBufferLock(ULWord deviceNumber, PDMA_PAGE_ROOT pRoot, NTV2BufferLock* pBufferLock);

static int platform_probe(struct platform_device *pd);
	
// platform configuration
static int platform_resources_config(ULWord deviceNumber);
static int platform_resources_release(ULWord deviceNumber);
static int platform_add_register(ULWord deviceNumber, struct device_node *node);
static int platform_remove_register(ULWord deviceNumber);
static int platform_add_fb_memory(ULWord deviceNumber, struct device_node *node);
static int platform_add_fs_memory(ULWord deviceNumber, struct device_node *node);
static int platform_remove_memory(ULWord deviceNumber);
static int platform_add_interrupt(ULWord deviceNumber, struct device_node *node);

static void initializeRegisterNames(ULWord deviceNumber, unsigned long mappedAddress);

static int readVirtualData(ULWord tag, UByte *buf, ULWord size);
static int writeVirtualData(ULWord tag, UByte *buf, ULWord size);
void deleteVirtualDataNode(VirtualDataNode *node);
static void deleteAllVirtualDataNodes(void);

static void suspend(ULWord deviceNumber);
static void resume(ULWord deviceNumber);

void InitInterruptBitLUT(void);

/********************/
/* Static variables */
/********************/

/* Define the the install/module paramaters */

static NTV2PrivateParams * NTV2Params[NTV2_MAXBOARDS];
static NTV2ModulePrivateParams NTV2ModuleParams;

// the uart driver
static struct uart_driver ntv2_uart_driver;

static int reboot_handler(struct notifier_block *this, unsigned long code, void *x);
static struct notifier_block reboot_notifier =
{
	.notifier_call	= reboot_handler,
	.next			= NULL,
	.priority		= 0
};

VirtualDataNode *gVirtualDataHead = NULL;

/*************/
/* Functions */
/*************/

NTV2PrivateParams *
getNTV2Params(unsigned int deviceNumber)
{
	if ( deviceNumber >= getNTV2ModuleParams()->numNTV2Devices )
	{
	    if ( getNTV2ModuleParams()->name == NULL)
			getNTV2ModuleParams()->name = "Unknown OEM board type";

		MSG("%s: bad board number %d\n", getNTV2ModuleParams()->name, deviceNumber);

		return NULL;
	}

	return NTV2Params[deviceNumber];
}

NTV2ModulePrivateParams *
getNTV2ModuleParams(void)
{
	return &NTV2ModuleParams;
}

/* The fops to tell the kernel where to go.  See linux/fs.h */
static struct file_operations ntv2_fops =
{
	.owner   = THIS_MODULE,
	.llseek  = ntv2_lseek,
	.read    = ntv2_read,
	.write   = ntv2_write,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36))
	.unlocked_ioctl   = ntv2_ioctl,
#else
	.ioctl   = ntv2_ioctl,
#endif
	.mmap    = ntv2_mmap,
	.open    = ntv2_open,
	.release = ntv2_release
};


// Seek
loff_t ntv2_lseek(struct file *file, loff_t off, int whence)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,19,0))
	UWord deviceNumber = MINOR(file->f_path.dentry->d_inode->i_rdev);
#else
	UWord deviceNumber = MINOR(file->f_dentry->d_inode->i_rdev);
#endif
	NTV2PrivateParams* pNTV2Params;
	loff_t newpos;

	if ( !(pNTV2Params = getNTV2Params(deviceNumber)) )
		return -ENODEV;

	// If board doesn't support PIO, refuse.
	if (NTV2DeviceCanDoPIO(pNTV2Params->_DeviceID) == false)
	{
		MSG("%s%d: lseek: Board type does not support PIO.\n",
				getNTV2ModuleParams()->name,
			  	deviceNumber);
		return -ENOSYS;
	}

	/* Check the seek type and set the memlocation pointer */
	switch (whence)
	{

    case 0: 	/* Absolute Seek */
		newpos = off;
		break;
    case 1: 	/* Seek from current position */
		newpos = file->f_pos + off;
		break;
	case 2:    /* go to end of 4th frame  */
		newpos = pNTV2Params->_BAR1MemorySize + off;
		break;
    default:	/* Any other value return An invalid value error */
		return -EINVAL;
	}

	if ( newpos < 0 )
	{
		newpos = 0;
		file->f_pos = 0;
		return -EINVAL;
	}

	// If framebuffers not mapped, _BA1MemorySize will be 0
	if ( newpos >= (pNTV2Params->_BAR1MemorySize) )
		newpos = 0;

	file->f_pos = newpos;
	return (newpos);
}

// Read
// Uses PIO mode to read from framebuffer
// This routine is only supplied cuz its easy.
// Best to either use API in ntv2card class(ntv2card.h,cpp)
ssize_t ntv2_read(struct file *file, char *buf, size_t count, loff_t *f_pos)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,19,0))
	UWord deviceNumber = MINOR(file->f_path.dentry->d_inode->i_rdev);
#else
	UWord deviceNumber = MINOR(file->f_dentry->d_inode->i_rdev);
#endif
	NTV2PrivateParams* pNTV2Params;
	unsigned long address;

	if ( !(pNTV2Params = getNTV2Params(deviceNumber)) )
		return -ENODEV;

	// If board doesn't support PIO, refuse.
	if (NTV2DeviceCanDoPIO(pNTV2Params->_DeviceID) == false)
	{
		MSG("%s%d: read: Board type does not support PIO.\n",
				getNTV2ModuleParams()->name,
			  	deviceNumber);
		return -ENOSYS;
	}

	// If framebuffers not mapped, _BA1MemorySize will be 0
	if (*f_pos >= pNTV2Params->_BAR1MemorySize)
	{
	   // Attempt to read beyond end of file
	   return 0;
	}


	address = pNTV2Params->_mappedBAR1Address;
	address += (*f_pos);

	if (*f_pos + count >= pNTV2Params->_BAR1MemorySize)
	{
	   count = pNTV2Params->_BAR1MemorySize - *f_pos;
	}

	if(copy_to_user(buf,(void*)address,count))
		return -EFAULT;

	*f_pos += count;

	return count;
}


// Write
// Uses PIO mode to write to framebuffer
// This routine is only supplied cuz its easy.
// Best to either use API in ntv2card class(ntv2card.h,cpp)
ssize_t ntv2_write(struct file *file, const char *buf, size_t count, loff_t *f_pos)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,19,0))
	UWord deviceNumber = MINOR(file->f_path.dentry->d_inode->i_rdev);
#else
	UWord deviceNumber = MINOR(file->f_dentry->d_inode->i_rdev);
#endif
	NTV2PrivateParams* pNTV2Params;
	unsigned long address;

	if ( !(pNTV2Params = getNTV2Params(deviceNumber)) )
		return -ENODEV;

	// If board doesn't support PIO, refuse.
	if (NTV2DeviceCanDoPIO(pNTV2Params->_DeviceID) == false)
	{
		MSG("%s%d: write: Board type does not support PIO.\n",
				getNTV2ModuleParams()->name,
			  	deviceNumber);
		return -ENOSYS;
	}

	// If framebuffers not mapped, _BA1MemorySize will be 0
	if (*f_pos >= pNTV2Params->_BAR1MemorySize)
	{
	   // Attempt to read beyond end of file
	   return 0;
	}

	address = pNTV2Params->_mappedBAR1Address;
	address += (*f_pos);

	if (*f_pos + count >= pNTV2Params->_BAR1MemorySize)
	{
	   count = pNTV2Params->_BAR1MemorySize - *f_pos;
	}

	if(copy_from_user((void*)address,buf,count))
		return -EFAULT;

	*f_pos += count;

	return count;
}



/* Function to handle IOCTL calls to device */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36))
long ntv2_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
#else
int ntv2_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
#endif
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,19,0))
	UWord deviceNumber = MINOR(file->f_path.dentry->d_inode->i_rdev);
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36))
	ULWord deviceNumber = iminor(file->f_dentry->d_inode);
#else
	ULWord deviceNumber = MINOR(inode->i_rdev);
#endif
	NTV2PrivateParams* pNTV2Params;
//#define IOCTL_DBG_MSG
#ifdef IOCTL_DBG_MSG
	unsigned long j;
#endif

	PFILE_DATA pFileData = (PFILE_DATA)file->private_data;

	// Find out which board
	if ( !(pNTV2Params = getNTV2Params(deviceNumber)) )
		return -ENODEV;

	if (_IOC_TYPE(cmd) != NTV2_DEVICE_TYPE)
	{
		MSG("%s: ntv2_ioctl ioctl not for NTV2[%d]\n", pNTV2Params->name, cmd);
		return -ENOTTY;
	}

#ifdef IOCTL_DBG_MSG
	j = jiffies + HZ/5;
	MSG("%s: ntv2_ioctl Cmd[%d]\n", pNTV2Params->name, _IOC_NR(cmd));
	while (jiffies < j)
		schedule();
#endif

	switch (cmd)
	{	
	case IOCTL_NTV2_WRITE_REGISTER:
		{
			REGISTER_ACCESS param;
            int status = 0;
			if(copy_from_user((void*)&param,(const void*) arg,sizeof(REGISTER_ACCESS)))
				return -EFAULT;

			status = WriteReg(  deviceNumber,
				  				param.RegisterNumber,
								param.RegisterValue,
								param.RegisterMask,
								param.RegisterShift);
            if (status != 0)
                return status;
		}
		break;

	case IOCTL_NTV2_READ_REGISTER:
		{
			REGISTER_ACCESS param;
            int status = 0;
			if(copy_from_user((void*)&param,(const void*) arg,sizeof(REGISTER_ACCESS)))
				return -EFAULT;

			status = ReadReg(deviceNumber,
                             param.RegisterNumber,
                             &param.RegisterValue,
                             param.RegisterMask,
                             param.RegisterShift);
			if(copy_to_user((void*)arg,(const void*) &param,sizeof(REGISTER_ACCESS)))
				return -EFAULT;
            if(status != 0)
                return status;
		}
		break;

	case IOCTL_NTV2_DMA_READ_FRAME:
		{
			NTV2_DMA_CONTROL_STRUCT param;
			DMA_PARAMS dmaParams;
			int status;

			if(copy_from_user((void*)&param,(const void*) arg,sizeof(NTV2_DMA_CONTROL_STRUCT)))
				return -EFAULT;
			//SHOW_DMA_IOCTL(IOCTL_NTV2_DMA_READ_FRAME);

			// If driver asked to choose engine, put assigned engine into dma struct
			if (param.engine == NTV2_DMA_FIRST_AVAILABLE)
			   param.engine = NTV2_DMA1;

			memset(&dmaParams, 0, sizeof(DMA_PARAMS));
			dmaParams.deviceNumber = deviceNumber;
			dmaParams.pPageRoot = &pFileData->dmaRoot;
			dmaParams.toHost = true;
			dmaParams.dmaEngine = param.engine;
			dmaParams.videoChannel = param.dmaChannel;
			dmaParams.pVidUserVa = (PVOID)param.frameBuffer;
			dmaParams.videoFrame = param.frameNumber;
			dmaParams.vidNumBytes = param.numBytes;
			dmaParams.frameOffset = param.frameOffsetSrc;

			status = dmaTransfer(&dmaParams);
			if (status)
			{
				MSG("%s: IOCTL_NTV2_DMA_READ_FRAME: DMA failed, err %d\n",
					pNTV2Params->name, status);
			}
			return status;
		}
		break;

	case IOCTL_NTV2_DMA_WRITE_FRAME:
		{
			NTV2_DMA_CONTROL_STRUCT param;
			DMA_PARAMS dmaParams;
			int status;

			if(copy_from_user((void*)&param,(const void*) arg,sizeof(NTV2_DMA_CONTROL_STRUCT)))
				return -EFAULT;
			//SHOW_DMA_IOCTL(IOCTL_NTV2_DMA_READ_FRAME);

			// If driver asked to choose engine, put assigned engine into dma struct
			if (param.engine == NTV2_DMA_FIRST_AVAILABLE)
			   param.engine = NTV2_DMA1;

			memset(&dmaParams, 0, sizeof(DMA_PARAMS));
			dmaParams.deviceNumber = deviceNumber;
			dmaParams.pPageRoot = &pFileData->dmaRoot;
			dmaParams.toHost = false;
			dmaParams.dmaEngine = param.engine;
			dmaParams.videoChannel = param.dmaChannel;
			dmaParams.pVidUserVa = (PVOID)param.frameBuffer;
			dmaParams.videoFrame = param.frameNumber;
			dmaParams.vidNumBytes = param.numBytes;
			dmaParams.frameOffset = param.frameOffsetDest;

			status = dmaTransfer(&dmaParams);
			if (status)
			{
				MSG("%s: IOCTL_NTV2_DMA_WRITE_FRAME: DMA failed, err %d\n",
					pNTV2Params->name, status);
			}
			return status;
		}
		break;

	case IOCTL_NTV2_DMA_READ:
		{
			NTV2_DMA_CONTROL_STRUCT param;
			DMA_PARAMS dmaParams;
			int status;

			if(copy_from_user((void*)&param,(const void*) arg,sizeof(NTV2_DMA_CONTROL_STRUCT)))
				return -EFAULT;
			//SHOW_DMA_IOCTL(IOCTL_NTV2_DMA_READ_FRAME);

			// If driver asked to choose engine, put assigned engine into dma struct
			if (param.engine == NTV2_DMA_FIRST_AVAILABLE)
			   param.engine = NTV2_DMA1;

			memset(&dmaParams, 0, sizeof(DMA_PARAMS));
			dmaParams.deviceNumber = deviceNumber;
			dmaParams.pPageRoot = &pFileData->dmaRoot;
			dmaParams.toHost = true;
			dmaParams.dmaEngine = param.engine;
			dmaParams.videoChannel = param.dmaChannel;
			dmaParams.pVidUserVa = (PVOID)param.frameBuffer;
			dmaParams.videoFrame = param.frameNumber;
			dmaParams.vidNumBytes = param.numBytes;
			dmaParams.frameOffset = param.frameOffsetSrc;

			status = dmaTransfer(&dmaParams);
			if (status)
			{
				MSG("%s: IOCTL_NTV2_DMA_READ: DMA failed, err %d\n",
					pNTV2Params->name, status);
			}
			return status;
		}
		break;

	case IOCTL_NTV2_DMA_WRITE:
		{
			NTV2_DMA_CONTROL_STRUCT param;
			DMA_PARAMS dmaParams;
			int status;

			if(copy_from_user((void*)&param,(const void*) arg,sizeof(NTV2_DMA_CONTROL_STRUCT)))
				return -EFAULT;
			//SHOW_DMA_IOCTL(IOCTL_NTV2_DMA_READ_FRAME);

			// If driver asked to choose engine, put assigned engine into dma struct
			if (param.engine == NTV2_DMA_FIRST_AVAILABLE)
			   param.engine = NTV2_DMA1;
			
			memset(&dmaParams, 0, sizeof(DMA_PARAMS));
			dmaParams.deviceNumber = deviceNumber;
			dmaParams.pPageRoot = &pFileData->dmaRoot;
			dmaParams.toHost = false;
			dmaParams.dmaEngine = param.engine;
			dmaParams.videoChannel = param.dmaChannel;
			dmaParams.pVidUserVa = (PVOID)param.frameBuffer;
			dmaParams.videoFrame = param.frameNumber;
			dmaParams.vidNumBytes = param.numBytes;
			dmaParams.frameOffset = param.frameOffsetDest;

			status = dmaTransfer(&dmaParams);
			if (status)
			{
				MSG("%s: IOCTL_NTV2_DMA_WRITE: DMA failed, err %d\n",
					pNTV2Params->name, status);
			}
			return status;
		}
		break;

/* Herein begins the segment stuff */

	case IOCTL_NTV2_DMA_READ_FRAME_SEGMENT:
		{
			NTV2_DMA_SEGMENT_CONTROL_STRUCT param;
			DMA_PARAMS dmaParams;
			int status;

			if(copy_from_user((void*)&param,(const void*) arg,sizeof(NTV2_DMA_SEGMENT_CONTROL_STRUCT)))
				return -EFAULT;
//			SHOW_DMA_SEGMENT_IOCTL(IOCTL_NTV2_DMA_READ_FRAME_SEGMENT);

			// If driver asked to choose engine, put assigned engine into dma struct
			if (param.engine == NTV2_DMA_FIRST_AVAILABLE)
			   param.engine = NTV2_DMA1;

			memset(&dmaParams, 0, sizeof(DMA_PARAMS));
			dmaParams.deviceNumber = deviceNumber;
			dmaParams.pPageRoot = &pFileData->dmaRoot;
			dmaParams.toHost = true;
			dmaParams.dmaEngine = param.engine;
			dmaParams.videoChannel = param.dmaChannel;
			dmaParams.pVidUserVa = (PVOID)param.frameBuffer;
			dmaParams.videoFrame = param.frameNumber;
			dmaParams.vidNumBytes = param.numBytes;
			dmaParams.frameOffset = param.frameOffsetSrc;
			dmaParams.vidUserPitch = param.videoSegmentHostPitch;
			dmaParams.vidFramePitch = param.videoSegmentCardPitch;
			dmaParams.numSegments = param.videoNumSegments;

			status = dmaTransfer(&dmaParams);
			if (status)
			{
				MSG("%s: IOCTL_NTV2_DMA_READ_FRAME_SEGMENT: DMA failed, err %d\n",
					pNTV2Params->name, status);
			}
			return status;
		}
		break;

	case IOCTL_NTV2_DMA_WRITE_FRAME_SEGMENT:
		{
			NTV2_DMA_SEGMENT_CONTROL_STRUCT param;
			DMA_PARAMS dmaParams;
			int status;

			if(copy_from_user((void*)&param,(const void*) arg,sizeof(NTV2_DMA_SEGMENT_CONTROL_STRUCT)))
				return -EFAULT;
//			SHOW_DMA_SEGMENT_IOCTL(IOCTL_NTV2_DMA_WRITE_FRAME_SEGMENT);

			// If driver asked to choose engine, put assigned engine into dma struct
			if (param.engine == NTV2_DMA_FIRST_AVAILABLE)
			   param.engine = NTV2_DMA1;

			memset(&dmaParams, 0, sizeof(DMA_PARAMS));
			dmaParams.deviceNumber = deviceNumber;
			dmaParams.pPageRoot = &pFileData->dmaRoot;
			dmaParams.toHost = false;
			dmaParams.dmaEngine = param.engine;
			dmaParams.videoChannel = param.dmaChannel;
			dmaParams.pVidUserVa = (PVOID)param.frameBuffer;
			dmaParams.videoFrame = param.frameNumber;
			dmaParams.vidNumBytes = param.numBytes;
			dmaParams.frameOffset = param.frameOffsetDest;
			dmaParams.vidUserPitch = param.videoSegmentHostPitch;
			dmaParams.vidFramePitch = param.videoSegmentCardPitch;
			dmaParams.numSegments = param.videoNumSegments;

			status = dmaTransfer(&dmaParams);
			if (status)
			{
				MSG("%s: IOCTL_NTV2_DMA_WRITE_FRAME_SEGMENT: DMA failed, err %d\n",
					pNTV2Params->name, status);
				return status;
			}
			return status;
		}
		break;

	case IOCTL_NTV2_DMA_READ_SEGMENT:
		{
			NTV2_DMA_SEGMENT_CONTROL_STRUCT param;
			DMA_PARAMS dmaParams;
			int status;

			if(copy_from_user((void*)&param,(const void*) arg,sizeof(NTV2_DMA_SEGMENT_CONTROL_STRUCT)))
				return -EFAULT;
//			SHOW_DMA_SEGMENT_IOCTL(IOCTL_NTV2_DMA_READ_SEGMENT);

			// If driver asked to choose engine, put assigned engine into dma struct
			if (param.engine == NTV2_DMA_FIRST_AVAILABLE)
				param.engine = NTV2_DMA1;

			memset(&dmaParams, 0, sizeof(DMA_PARAMS));
			dmaParams.deviceNumber = deviceNumber;
			dmaParams.pPageRoot = &pFileData->dmaRoot;
			dmaParams.toHost = true;
			dmaParams.dmaEngine = param.engine;
			dmaParams.videoChannel = param.dmaChannel;
			dmaParams.pVidUserVa = (PVOID)param.frameBuffer;
			dmaParams.videoFrame = param.frameNumber;
			dmaParams.vidNumBytes = param.numBytes;
			dmaParams.frameOffset = param.frameOffsetSrc;
			dmaParams.vidUserPitch = param.videoSegmentHostPitch;
			dmaParams.vidFramePitch = param.videoSegmentCardPitch;
			dmaParams.numSegments = param.videoNumSegments;

			status = dmaTransfer(&dmaParams);
			if (status)
			{
				MSG("%s: IOCTL_NTV2_DMA_READ_SEGMENT: DMA failed, err %d\n",
					pNTV2Params->name, status);
			}
			return status;
		}
		break;

	case IOCTL_NTV2_DMA_WRITE_SEGMENT:
		{
			NTV2_DMA_SEGMENT_CONTROL_STRUCT param;
			DMA_PARAMS dmaParams;
			int status;

			if(copy_from_user((void*)&param,(const void*) arg,sizeof(NTV2_DMA_SEGMENT_CONTROL_STRUCT)))
				return -EFAULT;
//			SHOW_DMA_SEGMENT_IOCTL(IOCTL_NTV2_DMA_WRITE_SEGMENT);

			// If driver asked to choose engine, put assigned engine into dma struct
			if (param.engine == NTV2_DMA_FIRST_AVAILABLE)
			   param.engine = NTV2_DMA1;

			memset(&dmaParams, 0, sizeof(DMA_PARAMS));
			dmaParams.deviceNumber = deviceNumber;
			dmaParams.pPageRoot = &pFileData->dmaRoot;
			dmaParams.toHost = false;
			dmaParams.dmaEngine = param.engine;
			dmaParams.videoChannel = param.dmaChannel;
			dmaParams.pVidUserVa = (PVOID)param.frameBuffer;
			dmaParams.videoFrame = param.frameNumber;
			dmaParams.vidNumBytes = param.numBytes;
			dmaParams.frameOffset = param.frameOffsetDest;
			dmaParams.vidUserPitch = param.videoSegmentHostPitch;
			dmaParams.vidFramePitch = param.videoSegmentCardPitch;
			dmaParams.numSegments = param.videoNumSegments;

			status = dmaTransfer(&dmaParams);
			if (status)
			{
				MSG("%s: IOCTL_NTV2_DMA_WRITE_SEGMENT: DMA failed, err %d\n",
					pNTV2Params->name, status);
			}
			return status;
		}
		break;

/* End of the Segmented DMAs */
#if 0
	case IOCTL_NTV2_DMA_P2P:
		{
			NTV2_DMA_P2P_CONTROL_STRUCT param;
			DMA_PARAMS dmaParams;
			int status;

			if(copy_from_user((void*)&param,(const void*) arg,sizeof(NTV2_DMA_P2P_CONTROL_STRUCT)))
				return -EFAULT;

			if (param.bRead && pNTV2Params->_FrameApertureBaseAddress)
			{
				status = dmaTargetP2P(deviceNumber, &param);
				if (status)
					return status;

				if(copy_to_user((void*)arg,(const void*) &param,sizeof(NTV2_DMA_P2P_CONTROL_STRUCT)))
					return -EFAULT;
			}
			else if (!param.bRead && pNTV2Params->_FrameApertureBaseAddress)
			{
				if(param.dmaEngine == NTV2_PIO)
					return -EPERM;

				// If driver asked to choose engine, put assigned engine into dma struct
				if (param.dmaEngine == NTV2_DMA_FIRST_AVAILABLE)
				   param.dmaEngine = NTV2_DMA1;

				// configure dma params
				memset(&dmaParams, 0, sizeof(DMA_PARAMS));
				dmaParams.deviceNumber = deviceNumber;
				dmaParams.toHost = true;
				dmaParams.dmaEngine = param.dmaEngine;
				dmaParams.videoChannel = param.dmaChannel;
				dmaParams.videoBusAddress = param.ullVideoBusAddress;
				dmaParams.videoBusSize = param.ulVideoBusSize;
				dmaParams.messageBusAddress = param.ullMessageBusAddress;
				dmaParams.messageData = param.ulMessageData;
				dmaParams.videoFrame = param.ulFrameNumber;
				dmaParams.vidNumBytes = param.ulVidNumBytes;
				dmaParams.frameOffset = param.ulFrameOffset;
				dmaParams.vidUserPitch = param.ulVidSegmentHostPitch;
				dmaParams.vidFramePitch = param.ulVidSegmentCardPitch;
				dmaParams.numSegments = param.ulVidNumSegments;

				status = dmaTransfer(&dmaParams);
				if (status)
					return status;
			}
			else
				return -EINVAL;
		}
		break;
#endif
	case IOCTL_NTV2_INTERRUPT_CONTROL:
		{
			NTV2_INTERRUPT_CONTROL_STRUCT param;
			ULWord interruptCount = 0;
			if(copy_from_user((void*)&param,(const void*) arg,sizeof(NTV2_INTERRUPT_CONTROL_STRUCT)))
				return -EFAULT;
			switch(param.eInterruptType)
			{
				case eVerticalInterrupt:
				case eInput1:
				case eInput2:
				case eInput3:
				case eInput4:
				case eInput5:
				case eInput6:
				case eInput7:
				case eInput8:
				case eOutput2:
				case eOutput3:
				case eOutput4:
				case eOutput5:
				case eOutput6:
				case eOutput7:
				case eOutput8:
				case eAudio:
				case eAudioInWrap:
				case eAudioOutWrap:
				case eWrapRate:
				case eUartTx:
				case eUartTx2:
				case eUartRx:
				case eAuxVerticalInterrupt:
				   AvInterruptControl(deviceNumber, param.eInterruptType, param.enable);
				   break;

				case eDMA1:
				case eDMA2:
				case eDMA3:
				case eDMA4:
				   if (param.enable)
					  	EnableDMAInterrupt(deviceNumber,
							getNTV2ModuleParams()->intrBitLut[param.eInterruptType]);
				   else
						DisableDMAInterrupt(deviceNumber,
							getNTV2ModuleParams()->intrBitLut[param.eInterruptType]);
				   break;

				case eInterruptMask:
				   break;

				case eChangeEvent:
				   break;

				case eGetIntCount:
					// No mutex but should not be an issue unless you're too close to the edge

					switch(param.interruptCount)
					{
						case eVerticalInterrupt:
						case eInput1:
						case eInput2:
						case eInput3:
						case eInput4:
						case eInput5:
						case eInput6:
						case eInput7:
						case eInput8:
						case eOutput2:
						case eOutput3:
						case eOutput4:
						case eOutput5:
						case eOutput6:
						case eOutput7:
						case eOutput8:
						case eUartRx:
						case eUartTx:
						case eUartTx2:
						case eAuxVerticalInterrupt:
							interruptCount = pNTV2Params->_interruptCount[param.interruptCount];
							break;
						default:
							break;
					}
					param.interruptCount = interruptCount;
					if(copy_to_user((void*)arg,(const void*) &param,sizeof(NTV2_INTERRUPT_CONTROL_STRUCT)))
						return -EFAULT;
				   break;

				default:
				   break;
			};
		}
		break;

	case IOCTL_NTV2_WAITFOR_INTERRUPT:
		{
			NTV2_WAITFOR_INTERRUPT_STRUCT param;
			ULWord timeoutJiffies;
			int result;
#ifndef INTERRUPT_WAITFOR_BIT
			ULWord count;
#endif
			if(copy_from_user((void*)&param,(const void*) arg,sizeof(NTV2_WAITFOR_INTERRUPT_STRUCT)))
				return -EFAULT;

			if(param.eInterruptType >= eNumInterruptTypes)
				return -EINVAL;

			// Round up to granularity of HZ
			timeoutJiffies = ntv2_getRoundedUpTimeoutJiffies(param.timeOutMs);
//#define WAITFOR_INTERRUPT_DEBUG_MSGS
#ifdef WAITFOR_INTERRUPT_DEBUG_MSGS
			MSG("%s: IOCTL_NTV2_WAITFOR_INTERRUPT: timeout = %d, timeOutJiffies = %d, eInterruptType = %d\n",
				pNTV2Params->name, param.timeOutMs, timeoutJiffies, param.eInterruptType);
#endif
#ifdef INTERRUPT_WAITFOR_BIT
			// Avoid race condition
			// When clear_bit is used for locking purposes, asm-i386/bitops.h
			// advises doing smp_mb__before_clear_bit()	before and
			// smp_mb__after_clear_bit() afterwards to ensure changes are
			// visible on other processors.  Not sure why this isn't required for
			// set_bit() ???
			smp_mb__before_clear_bit();
			clear_bit(0, (volatile unsigned long *)&pNTV2Params->_interruptHappened[param.eInterruptType]);
			smp_mb__after_clear_bit();
#else			
			count = *((volatile ULWord *)&pNTV2Params->_interruptCount[param.eInterruptType]);
#endif
			// if it's a uart TX/RX we don't want to interrupt the
			// process as we might lose data
			if (param.eInterruptType == eUartTx
				|| param.eInterruptType == eUartTx2
				|| param.eInterruptType == eUartRx)
			{
#ifdef INTERRUPT_WAITFOR_BIT
				result = wait_event_timeout((pNTV2Params->_interruptWait[param.eInterruptType]),
											test_bit(0, (volatile unsigned long *)&pNTV2Params->_interruptHappened[param.eInterruptType]),
											timeoutJiffies);
#else				
				result = wait_event_timeout((pNTV2Params->_interruptWait[param.eInterruptType]),
											count != *((volatile ULWord *)&pNTV2Params->_interruptCount[param.eInterruptType]),
											timeoutJiffies);
#endif				
			}
			else
			{
#ifdef INTERRUPT_WAITFOR_BIT
				result = wait_event_interruptible_timeout((pNTV2Params->_interruptWait[param.eInterruptType]),
														  test_bit(0, (volatile unsigned long *)&pNTV2Params->_interruptHappened[param.eInterruptType]),
														  timeoutJiffies);
#else				
				result = wait_event_interruptible_timeout((pNTV2Params->_interruptWait[param.eInterruptType]),
														  count != *((volatile ULWord *)&pNTV2Params->_interruptCount[param.eInterruptType]),
														  timeoutJiffies);
#endif				
			}


#ifdef WAITFOR_INTERRUPT_DEBUG_MSGS
			if (result <= 0)
				MSG("%s: IOCTL_NTV2_WAITFOR_INTERRUPT: wait complete, result = %d\n",
					pNTV2Params->name, result);
#endif
			if (result < 0)
			{
				// Signal
				// return -ERESTARTSYS;
				return result;
			}
			if (result == 0)
			{
				// timed out
				return -ETIMEDOUT;
			}
			param.success = result * 1000 / HZ; // Success == true, we return the number of msec remaining
#ifdef WAITFOR_INTERRUPT_DEBUG_MSGS
			MSG("%s: IOCTL_NTV2_WAITFOR_INTERRUPT: wait complete, success, remaining time = %d ms\n",
				pNTV2Params->name, param.success);
#endif
			if(copy_to_user((void*)arg,(const void*) &param,sizeof(NTV2_WAITFOR_INTERRUPT_STRUCT)))
				return -EFAULT;
		}
		break;

	//
	// Autocirculate IOCTLs
	//
	case IOCTL_NTV2_AUTOCIRCULATE_CONTROL:
		{
			AUTOCIRCULATE_DATA param;
			if(copy_from_user((void*)&param, (const void*) arg, sizeof(AUTOCIRCULATE_DATA)))
				return -EFAULT;
			return AutoCirculateControl(deviceNumber, &param);
		}
		break;

	case IOCTL_NTV2_AUTOCIRCULATE_STATUS:
		{
			int returnCode;

			AUTOCIRCULATE_STATUS_STRUCT param;
			if(copy_from_user((void*)&param, (const void*) arg, sizeof(AUTOCIRCULATE_STATUS_STRUCT)))
				return -EFAULT;
			returnCode = AutoCirculateStatus(deviceNumber, &param);
			if(copy_to_user((void*)arg, (const void*) &param, sizeof(AUTOCIRCULATE_STATUS_STRUCT)))
				return -EFAULT;
			return returnCode;
		}
		break;

	case IOCTL_NTV2_AUTOCIRCULATE_FRAMESTAMP:
		{
			int returnCode;
			AUTOCIRCULATE_FRAME_STAMP_COMBO_STRUCT param;
			if(copy_from_user((void*)&param, (const void*) arg, sizeof(AUTOCIRCULATE_FRAME_STAMP_COMBO_STRUCT)))
				return -EFAULT;
			returnCode = AutoCirculateFrameStamp(deviceNumber, &param);
			if(copy_to_user((void*)arg, (const void*) &param, sizeof(AUTOCIRCULATE_FRAME_STAMP_COMBO_STRUCT)))
				return -EFAULT;
			return returnCode;
		}
		break;

	case IOCTL_NTV2_AUTOCIRCULATE_CAPTURETASK:
		{
			int returnCode;
			AUTOCIRCULATE_FRAME_STAMP_COMBO_STRUCT param;
			if(copy_from_user((void*)&param, (const void*) arg, sizeof(AUTOCIRCULATE_FRAME_STAMP_COMBO_STRUCT)))
				return -EFAULT;
			returnCode = AutoCirculateCaptureTask(deviceNumber, &param);
			if(copy_to_user((void*)arg, (const void*) &param, sizeof(AUTOCIRCULATE_FRAME_STAMP_COMBO_STRUCT)))
				return -EFAULT;
			return returnCode;
		}
		break;

	case IOCTL_NTV2_AUTOCIRCULATE_TRANSFER:
		{
			int returnCode;
			AUTOCIRCULATE_TRANSFER_COMBO_STRUCT *param;

			param = (AUTOCIRCULATE_TRANSFER_COMBO_STRUCT *)kmalloc(sizeof (AUTOCIRCULATE_TRANSFER_COMBO_STRUCT), GFP_ATOMIC);

			if (param)
			{
				if(copy_from_user((void*)param, (const void*) arg, sizeof(AUTOCIRCULATE_TRANSFER_COMBO_STRUCT)))
				{
					kfree(param);
					return -EFAULT;
				}
				returnCode = AutoCirculateTransfer(deviceNumber, param);
				if(copy_to_user((void*)arg, (const void*) param, sizeof(AUTOCIRCULATE_TRANSFER_COMBO_STRUCT)))
				{
					kfree(param);
					return -EFAULT;
				}

				kfree(param);
				return returnCode;
			}
			return -ENOMEM;
		}
		break;
	case IOCTL_NTV2_CONTROL_DRIVER_DEBUG_MESSAGES:
		{
			NTV2_CONTROL_DRIVER_DEBUG_MESSAGES_STRUCT param;
			if(copy_from_user((void*)&param,(const void*) arg,sizeof(NTV2_CONTROL_DRIVER_DEBUG_MESSAGES_STRUCT)))
				return -EFAULT;
			param.success = ControlDebugMessages(param.msgSet, param.enable);
			if(copy_to_user((void*)arg,(const void*) &param,sizeof(NTV2_CONTROL_DRIVER_DEBUG_MESSAGES_STRUCT)))
				return -EFAULT;
			if (MsgsEnabled(NTV2_DRIVER_DEBUG_DEBUG_MESSAGES))
				ShowDebugMessageControl(param.msgSet);
		}
		break;

	case IOCTL_NTV2_SETUP_BOARD:
		{
		   SetupBoard(deviceNumber);
		   return 0;
		}
		break;

	case IOCTL_NTV2_RESTORE_HARDWARE_PROCAMP_REGISTERS:
		{
		   return RestoreHardwareProcampRegisters(deviceNumber,
					pNTV2Params->_DeviceID,
					&pNTV2Params->_virtualProcAmpRegisters,
					&pNTV2Params->_hwProcAmpRegisterImage);
		}
		break;

	case IOCTL_NTV2_SET_BITFILE_INFO:
		return -EPERM;

	case IOCTL_NTV2_GET_BITFILE_INFO:
		return -EPERM;

	case IOCTL_AJANTV2_MESSAGE:
		{
			NTV2_HEADER *	pMessage	= NULL;
			void *			pInBuff		= NULL;
			void *			pOutBuff	= NULL;
			int				returnCode	= 0;
            void *          pVirtBuf[3] = { NULL, NULL, NULL };

			// This limits the message size to one page, which should not be a problem, nor is sleeping on the alloc
			pMessage = (NTV2_HEADER *) get_zeroed_page(GFP_KERNEL);
			if (!pMessage)
				return -ENOMEM;

			if(copy_from_user((void*)pMessage, (const void*) arg, sizeof(NTV2_HEADER)))
			{
				returnCode = -EFAULT;
				goto messageError;
			}

			// Check for buffer overrun
			if (pMessage->fSizeInBytes > PAGE_SIZE)
			{
				returnCode = -ENOMEM;
				goto messageError;
			}

			// Go get the whole thing
			if(copy_from_user((void*)pMessage, (const void*) arg, pMessage->fSizeInBytes))
			{
				returnCode = -EFAULT;
				goto messageError;
			}

			returnCode = ValidateAjaNTV2Message (pMessage);
			if(returnCode)
			{
				MSG("%s: NTV2_AJAMESSAGE: validate fail\n", pNTV2Params->name);
				goto messageError;
			}

			// Scratch area for input parameters
			pInBuff = (void *) get_zeroed_page(GFP_KERNEL);
			if (!pInBuff)
			{
				returnCode = -ENOMEM;
				goto messageError;
			}

			// Scratch areas for returned data
			pOutBuff = (void *) get_zeroed_page(GFP_KERNEL);
			if (!pOutBuff)
			{
				returnCode = -ENOMEM;
				goto messageError;
			}

			// Determine what kind of message it is
			switch(pMessage->fType)
			{
#if 0                
			case NTV2_TYPE_ACSTATUS:
				{
					returnCode = AutoCirculateStatus_Ex(deviceNumber, (AUTOCIRCULATE_STATUS *) pMessage);
					if(returnCode)
						goto messageError;

					if(copy_to_user((void*)arg, (const void*)pMessage, sizeof(AUTOCIRCULATE_STATUS)))
					{
						returnCode = -EFAULT;
						goto messageError;
					}
				}
				break;

			case NTV2_TYPE_ACXFER:
				{
					returnCode = AutoCirculateTransfer_Ex(deviceNumber, &pFileData->dmaRoot, (AUTOCIRCULATE_TRANSFER *) pMessage);
					if(returnCode)
						goto messageError;

					if(copy_to_user((void*)arg, (const void*)pMessage, sizeof(AUTOCIRCULATE_TRANSFER)))
					{
						returnCode = -EFAULT;
						goto messageError;
					}
				}
				break;
#endif
			case NTV2_TYPE_SDISTATS:
				{
					NTV2Buffer * pInStatistics = &((NTV2SDIInStatistics*)pMessage)->mInStatistics;
					if(copy_from_user((void*) pOutBuff,
									  (const void*)(pInStatistics->fUserSpacePtr),
									  pInStatistics->fByteCount))
					{
						returnCode = -EFAULT;
						goto messageError;
					}

					returnCode = DoMessageSDIInStatictics(deviceNumber, pInStatistics, pOutBuff);
					if(returnCode)
						goto messageError;

					if(copy_to_user((void*)(pInStatistics->fUserSpacePtr), (const void*)pOutBuff, pInStatistics->fByteCount))
					{
						returnCode = -EFAULT;
						goto messageError;
					}
				}
				break;

			case NTV2_TYPE_GETREGS:
				{
					ULWord  		mInNumRegisters		=  ((NTV2GetRegisters*)pMessage)->mInNumRegisters;
					NTV2Buffer *	pInRegisters		= &((NTV2GetRegisters*)pMessage)->mInRegisters;
					NTV2Buffer *	pOutGoodRegisters	= &((NTV2GetRegisters*)pMessage)->mOutGoodRegisters;
					NTV2Buffer *	pOutValues			= &((NTV2GetRegisters*)pMessage)->mOutValues;
					ULWord *		pInRegArray			= NULL;
					ULWord *		pOutRegArray		= NULL;
					ULWord *		pOutValuesArray		= NULL;
					ULWord			i;

					//	Check for buffer overrun
					if((pInRegisters->fByteCount > (PAGE_SIZE * 1000)) ||
					   (pOutGoodRegisters->fByteCount > (PAGE_SIZE * 1000)) ||
					   (pOutValues->fByteCount > (PAGE_SIZE * 1000)))
					{
						returnCode = -ENOMEM;
						goto messageError;
					}
					if((pInRegisters->fByteCount < (mInNumRegisters * 4)) ||
					   (pOutGoodRegisters->fByteCount < (mInNumRegisters * 4)) ||
					   (pOutValues->fByteCount < (mInNumRegisters * 4)))
					{
						returnCode = -ENOMEM;
						goto messageError;
					}

                    //  Allocate register buffers
                    pVirtBuf[0] = vmalloc(pInRegisters->fByteCount);
                    if (pVirtBuf[0] == NULL)
					{
						returnCode = -ENOMEM;
						goto messageError;
					}
                    pInRegArray = (ULWord*)pVirtBuf[0];
                    
                    pVirtBuf[1] = vmalloc(pOutGoodRegisters->fByteCount);
                    if (pVirtBuf[1] == NULL)
					{
						returnCode = -ENOMEM;
						goto messageError;
					}
                    pOutRegArray = (ULWord*)pVirtBuf[1];
                    
                    pVirtBuf[2] = vmalloc(pOutValues->fByteCount);
                    if (pVirtBuf[2] == NULL)
					{
						returnCode = -ENOMEM;
						goto messageError;
					}
                    pOutValuesArray = (ULWord*)pVirtBuf[2];

					//	List of registers to read
					if(copy_from_user((void*) pInRegArray,
									  (const void*)(pInRegisters->fUserSpacePtr),
									  pInRegisters->fByteCount))
					{
						returnCode = -EFAULT;
						goto messageError;
					}

					//	List of registers read to return
					if(copy_from_user((void*) pOutRegArray,
									  (const void*)(pOutGoodRegisters->fUserSpacePtr),
									  pOutGoodRegisters->fByteCount))
					{
						returnCode = -EFAULT;
						goto messageError;
					}

					//	List of register values to return
					if(copy_from_user((void*) pOutValuesArray,
									  (const void*)(pOutValues->fUserSpacePtr),
									  pOutValues->fByteCount))
					{
						returnCode = -EFAULT;
						goto messageError;
					}

					for (i = 0; i < mInNumRegisters; i++)
					{
						pOutRegArray[i] = pInRegArray[i];
						if (pInRegArray[i] != kRegXenaxFlashDOUT)	//	Prevent firmware erase/program/verify failures
							pOutValuesArray[i] = ReadRegister (deviceNumber, pInRegArray[i], NO_MASK, NO_SHIFT);
						else
							pOutValuesArray[i] = 0;
					}

					//	Send back the list of registers read
					if(copy_to_user((void*)(pOutGoodRegisters->fUserSpacePtr), (const void*)pOutRegArray, pOutGoodRegisters->fByteCount))
					{
						returnCode = -EFAULT;
						goto messageError;
					}

					//	Send back the list of values read
					if(copy_to_user((void*)(pOutValues->fUserSpacePtr), (const void*)pOutValuesArray, pOutValues->fByteCount))
					{
						returnCode = -EFAULT;
						goto messageError;
					}

					((NTV2GetRegisters*)pMessage)->mOutNumRegisters = mInNumRegisters;

					// Pass message back to user so updated fields can be read
					if(copy_to_user((void*)arg, (const void*) pMessage, pMessage->fSizeInBytes))
					{
						returnCode = -EFAULT;
						goto messageError;
					}
				}
				break;

			case NTV2_TYPE_SETREGS:
				{
					ULWord  		mInNumRegisters		=  ((NTV2SetRegisters*)pMessage)->mInNumRegisters;
					NTV2Buffer *	pInRegisters		= &((NTV2SetRegisters*)pMessage)->mInRegInfos;
					NTV2RegInfo  *	pInRegInfos			= (NTV2RegInfo*) pInBuff;
					ULWord			i;

					//	Check for buffer overrun
					if(pInRegisters->fByteCount > PAGE_SIZE)
					{
						returnCode = -ENOMEM;
						goto messageError;
					}

					//	List of registers to read
					if(copy_from_user((void*) pInBuff,
									  (const void*)(pInRegisters->fUserSpacePtr),
									  pInRegisters->fByteCount))
					{
						returnCode = -EFAULT;
						goto messageError;
					}

					for (i = 0; i < mInNumRegisters; i++)
					{
						NTV2RegInfo regInfo = pInRegInfos [i];
						WriteRegister (deviceNumber, regInfo.registerNumber, regInfo.registerValue, regInfo.registerMask, regInfo.registerShift);
					}

					((NTV2SetRegisters*)pMessage)->mOutNumFailures = 0;
				}
				break;
			case NTV2_TYPE_BANKGETSET:
				{
					ULWord  		mIsWriting		=  ((NTV2BankSelGetSetRegs*)pMessage)->mIsWriting;
					NTV2Buffer *	pInBankInfos	= &((NTV2BankSelGetSetRegs*)pMessage)->mInBankInfos;
					NTV2Buffer *	pInRegInfos		= &((NTV2BankSelGetSetRegs*)pMessage)->mInRegInfos;


					//	Check for buffer overrun
					if((pInBankInfos->fByteCount > PAGE_SIZE) || (pInRegInfos->fByteCount > PAGE_SIZE))
					{
						returnCode = -ENOMEM;
						goto messageError;
					}

					if(copy_from_user((void*) pInBuff,
									  (const void*)(pInBankInfos->fUserSpacePtr),
									  pInBankInfos->fByteCount))
					{
						returnCode = -EFAULT;
						goto messageError;
					}

					if(copy_from_user((void*) pOutBuff,
									  (const void*)(pInRegInfos->fUserSpacePtr),
									  pInRegInfos->fByteCount))
					{
						returnCode = -EFAULT;
						goto messageError;
					}

					if( mIsWriting )
						returnCode = DoMessageBankAndRegisterWrite(deviceNumber, (NTV2RegInfo*) pOutBuff, (NTV2RegInfo*) pInBuff);
					else
						returnCode = DoMessageBankAndRegisterRead(deviceNumber, (NTV2RegInfo*) pOutBuff, (NTV2RegInfo*) pInBuff);

					if(returnCode)
						goto messageError;

					if( ! mIsWriting )
					{
						if(copy_to_user((void*)(pInRegInfos->fUserSpacePtr), (const void*)pOutBuff, pInRegInfos->fByteCount))
						{
							returnCode = -EFAULT;
							goto messageError;
						}
					}
				}
				break;
#if 0                
			case NTV2_TYPE_ACFRAMESTAMP:
				{
					NTV2Buffer *	pAcTimeCodes	= &((FRAME_STAMP*)pMessage)->acTimeCodes;

					//	Check for buffer overrun
					if (pAcTimeCodes->fByteCount > PAGE_SIZE)
					{
						returnCode = -ENOMEM;
						goto messageError;
					}

					returnCode = DoMessageAutoCircFrame (deviceNumber, (FRAME_STAMP*)pMessage, (NTV2_RP188*)pOutBuff);
					if (returnCode)
					{
						goto messageError;
					}

					//	Pass back timecode array
					if(copy_to_user((void*)(pAcTimeCodes->fUserSpacePtr), (const void*)pOutBuff, pAcTimeCodes->fByteCount))
					{
						returnCode = -EFAULT;
						goto messageError;
					}

					// Pass message back to user so updated fields can be read
					if(copy_to_user((void*)arg, (const void*)pMessage, pMessage->fSizeInBytes))
					{
						returnCode = -EFAULT;
						goto messageError;
					}
				}
				break;
#endif                
			case NTV2_TYPE_AJABUFFERLOCK:
				{
					if (pFileData == NULL)
					{
						goto messageError;
					}
					
					returnCode = DoMessageBufferLock (deviceNumber, &pFileData->dmaRoot, (NTV2BufferLock*)pMessage);
					if (returnCode)
					{
						goto messageError;
					}

					if(copy_to_user((void*)arg, (const void*)pMessage, sizeof(NTV2BufferLock)))
					{
						returnCode = -EFAULT;
						goto messageError;
					}
				}
				break;
			case NTV2_TYPE_VIRTUAL_DATA_RW:
				{
                    NTV2VirtualData *msg = (NTV2VirtualData *)pMessage;


                    // Can't use 0 as a tag
                    if (msg->mTag == 0)
                    {
                        returnCode = -EINVAL;
                        goto messageError;
                    }
                    // check for null ptr
                    if (msg->mVirtualData.fUserSpacePtr == 0)
                    {
                        returnCode = -EINVAL;
                        goto messageError;
                    }
                    // Check for buffer overrun
					if (msg->mVirtualData.fByteCount > PAGE_SIZE)
					{
						returnCode = -ENOMEM;
						goto messageError;
					}

                    // if we are writing, copy data from user space to buffer
                    // and write to list
                    if (msg->mIsWriting)
                    {
                        if (copy_from_user((void *)pInBuff,
                                  (const void *)msg->mVirtualData.fUserSpacePtr,
                                  msg->mVirtualData.fByteCount))
                        {
                            returnCode = -EFAULT;
                            goto messageError;
                        }
                        returnCode = writeVirtualData(msg->mTag, pInBuff,
                                            msg->mVirtualData.fByteCount);
                    }
                    // if we are reading, read from list and copy back to
                    // user space
                    else
                    {
                        returnCode = readVirtualData(msg->mTag, pOutBuff,
                                                msg->mVirtualData.fByteCount);
                        if (returnCode != 0)
                            goto messageError;
                        // copy result back to user space
						if (copy_to_user((void *)msg->mVirtualData.fUserSpacePtr,
                                    (const void*)pOutBuff,
                                    msg->mVirtualData.fByteCount))
						{
							returnCode = -EFAULT;
							goto messageError;
						}
                    }
                    if (returnCode != 0)
                        goto messageError;
				}
				break;

			default:
				returnCode = -EPERM;
				break;
			}

messageError:

            {
                int i;
                for (i = 0; i < 3; i++)
                {
                    if (pVirtBuf[i] != NULL)
                        vfree(pVirtBuf[i]);
                }
                free_page((unsigned long) pOutBuff);
                free_page((unsigned long) pInBuff);
                free_page((unsigned long) pMessage);
            }

			return returnCode;
		}
		break;
	}

	return 0;
}

// mmap.
// Kind of cheating.but it works.
// This function maps
// BAR1(framebuffer) if
// vm_pgoff = 0
// BAR0(registers) if
// vm_pgoff = 1
// DMA Buffer
// vm_pgoff = 2
// PCI Flash Buffer
// vm_pgoff = 4

int ntv2_mmap(struct file *file,struct vm_area_struct* vma)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,19,0))
	UWord deviceNumber = MINOR(file->f_path.dentry->d_inode->i_rdev);
#else
	UWord deviceNumber = MINOR(file->f_dentry->d_inode->i_rdev);
#endif
	NTV2PrivateParams* pNTV2Params;
	ULWord size = vma->vm_end-vma->vm_start;
	// MSG("%s%d: ntv2_mmap() %lx\n", getNTV2ModuleParams()->name,	deviceNumber, vma->vm_pgoff);

	if ( !(pNTV2Params = getNTV2Params(deviceNumber)) )
		return -ENODEV;

	// Don't try to swap out physical pages
#if defined(KERNEL_6_3_0_VM_FLAGS)
    vm_flags_set(vma, VM_IO);
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0))
	vma->vm_flags |= VM_IO;
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(3,7,0))
	vma->vm_flags |= (VM_DONTEXPAND | VM_DONTDUMP );
#else
	vma->vm_flags |= VM_RESERVED;
#endif

	//	MSG("%s: ntv2_mmap() vma->start 0x%016lX, size 0x%08X\n", pNTV2Params->name, vma->vm_start, size);

	switch ( vma->vm_pgoff )
	{
	case 0:
		if (!MapFrameBuffers)	// If frame buffers not mapped
			return -EADDRNOTAVAIL;
#ifndef RHEL4
		if ( remap_pfn_range(vma, vma->vm_start,pNTV2Params->_unmappedBAR1Address >> PAGE_SHIFT,size,vma->vm_page_prot))
#else
		if ( remap_page_range( vma, vma->vm_start, pNTV2Params->_unmappedBAR1Address, size, vma->vm_page_prot))
#endif
			return -EAGAIN;
		break;

	case 1:
#if !defined(RHEL4)
		if ( remap_pfn_range(vma, vma->vm_start,pNTV2Params->_unmappedBAR0Address >> PAGE_SHIFT,size,vma->vm_page_prot))
#else
		if ( remap_page_range( vma, vma->vm_start, pNTV2Params->_unmappedBAR0Address, size, vma->vm_page_prot))
#endif
			return -EAGAIN;
		break;

	default:
		return -EAGAIN;
		break;
	}
	return 0;
}


// Initialize lookup table that translates from an INTERRUPT_ENUM to
// a single-bit mask in either the Audio/Video Interrupt Control register
// (register 20) or the DMA Interrupt Control register (register 49).
// Entries in the table for enums with no associated bit are set to all ones.
void InitInterruptBitLUT(void)
{
	ULWord * intrBitLut = getNTV2ModuleParams()->intrBitLut;

	intrBitLut[eVerticalInterrupt]		= NTV2_OUTPUTVERTICAL;			// AV Interrupt reg
	intrBitLut[eInput1] 				= NTV2_INPUT1VERTICAL;			// AV Interrupt reg
	intrBitLut[eInput2] 				= NTV2_INPUT2VERTICAL;			// AV Interrupt reg
	intrBitLut[eInput3] 				= NTV2_INPUT3VERTICAL;			// AV Interrupt reg (different reg, same bit as input1)
	intrBitLut[eInput4] 				= NTV2_INPUT4VERTICAL;			// AV Interrupt reg
	intrBitLut[eInput5] 				= NTV2_INPUT5VERTICAL;			// AV Interrupt reg
	intrBitLut[eInput6] 				= NTV2_INPUT6VERTICAL;			// AV Interrupt reg
	intrBitLut[eInput7] 				= NTV2_INPUT7VERTICAL;			// AV Interrupt reg
	intrBitLut[eInput8] 				= NTV2_INPUT8VERTICAL;			// AV Interrupt reg

	intrBitLut[eOutput2] 				= NTV2_OUTPUT2VERTICAL;			// AV Interrupt reg
	intrBitLut[eOutput3] 				= NTV2_OUTPUT3VERTICAL;			// AV Interrupt reg
	intrBitLut[eOutput4] 				= NTV2_OUTPUT4VERTICAL;			// AV Interrupt reg
	intrBitLut[eOutput5] 				= NTV2_OUTPUT5VERTICAL;			// AV Interrupt reg
	intrBitLut[eOutput6] 				= NTV2_OUTPUT6VERTICAL;			// AV Interrupt reg
	intrBitLut[eOutput7] 				= NTV2_OUTPUT7VERTICAL;			// AV Interrupt reg
	intrBitLut[eOutput8] 				= NTV2_OUTPUT8VERTICAL;			// AV Interrupt reg
	intrBitLut[eAudio]					= NTV2_AUDIOINTERRUPT; 			// AV Interrupt reg
	intrBitLut[eAudioInWrap] 			= NTV2_AUDIOINWRAPINTERRUPT;	// AV Interrupt reg
	intrBitLut[eAudioOutWrap] 			= NTV2_AUDIOOUTWRAPINTERRUPT; 	// AV Interrupt reg
	intrBitLut[eWrapRate]				= NTV2_AUDIOWRAPRATEINTERRUPT; 	// AV Interrupt reg
	intrBitLut[eUartTx]					= NTV2_UART_TX_INTERRUPT; 		// AV Interrupt reg
	intrBitLut[eUartTx2]				= NTV2_UART_TX_INTERRUPT2; 		// AV Interrupt reg
	intrBitLut[eUartRx]					= NTV2_UART_RX_INTERRUPT; 		// AV Interrupt reg
	intrBitLut[eAuxVerticalInterrupt]	= NTV2_AUX_VERTICAL_INTERRUPT; 	// AV Interrupt reg
}

/* Function to open device */
int ntv2_open(struct inode *minode, struct file *mfile)
{

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,19,0))
	UWord deviceNumber = MINOR(mfile->f_path.dentry->d_inode->i_rdev);
#else
	UWord deviceNumber = MINOR(mfile->f_dentry->d_inode->i_rdev);
#endif
	PFILE_DATA pFileData;

	if (deviceNumber >= getNTV2ModuleParams()->numNTV2Devices)
	{
		// Remove this message since scanning for boards will eventually try to open one that doesn't exist
		// Also, we now have apps that scan for boards in their timer routines, causing a message flood
#if 0
		MSG("%s: open: device %d not present (num devices %d)\n",
			getNTV2ModuleParams()->name, deviceNumber, getNTV2ModuleParams()->numNTV2Devices);
#endif
		return -ENODEV;
	}

	pFileData = (PFILE_DATA)kmalloc(sizeof (FILE_DATA), GFP_ATOMIC);
	if (pFileData != NULL)
	{
		if (dmaPageRootInit(deviceNumber, &pFileData->dmaRoot) == 0)
		{
			mfile->private_data = pFileData;
		}
		else
		{
			kfree(pFileData);
		}
	}

	return 0;
}


/* Function to close device */
int ntv2_release(struct inode *minode, struct file *mfile) {

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,19,0))
	UWord deviceNumber = MINOR(mfile->f_path.dentry->d_inode->i_rdev);
#else
	UWord deviceNumber = MINOR(mfile->f_dentry->d_inode->i_rdev);
#endif
	PFILE_DATA pFileData;

	if (deviceNumber >= getNTV2ModuleParams()->numNTV2Devices)
	{
		MSG("%s: release: device %d not present (num devices %d)\n",
				getNTV2ModuleParams()->name, deviceNumber, getNTV2ModuleParams()->numNTV2Devices);
		return -ENODEV;
	}

	pFileData = (PFILE_DATA)mfile->private_data;
	if (pFileData != NULL)
	{
		dmaPageRootRelease(deviceNumber, &pFileData->dmaRoot);
		kfree(pFileData);
		mfile->private_data = NULL;
	}

	return 0;
}

inline void
interruptHousekeeping(NTV2PrivateParams* pNTV2Params, INTERRUPT_ENUMS interrupt)
{
	set_bit(0, (volatile unsigned long *)&pNTV2Params->_interruptHappened[interrupt]);
	pNTV2Params->_interruptCount[interrupt]++;
	wake_up(&pNTV2Params->_interruptWait[interrupt]);
}

irqreturn_t ntv2_fpga_irq(int irq, void *dev_id)
{
	NTV2PrivateParams* pNTV2Params = (NTV2PrivateParams*)dev_id;
	ULWord deviceNumber = pNTV2Params->deviceNumber;
//    ULWord statusRegister;
	ULWord status2Register;
    ULWord64 audioClock;
	int handled = 0;

	Ntv2SystemContext systemContext;
    memset(&systemContext, 0, sizeof(Ntv2SystemContext));
	systemContext.devNum = deviceNumber;

	if( NTV2DeviceGetNumVideoChannels(pNTV2Params->_DeviceID) > 2)
	{
		status2Register = ReadRegister(deviceNumber, kRegStatus2, NO_MASK, NO_SHIFT);
	}
	else
	{
		status2Register = 0;
	}

#if 0
	{
		statusRegister = ReadStatusRegister(deviceNumber);

		// check serial port interrupt
		ntv2_serial_interrupt(pNTV2Params->m_pSerialPort);

		statusRegister = ReadStatusRegister(deviceNumber);

		// UART Rx
		if ( statusRegister & BIT_15 )
		{
			ClearUartRxInterrupt(deviceNumber);
			interruptHousekeeping(pNTV2Params, eUartRx);
# ifdef UARTRXFIFOSIZE
			{
			   ULWord uartControl = ReadUARTControl(deviceNumber);
			   unsigned long flags;

			   while(uartControl & BIT_4) {
				  ULWord uartData = ReadUARTReceiveData(deviceNumber);
				  UByte byte = uartData & 0xff;

				  spin_lock_irqsave(&pNTV2Params->uartRxFifoLock, flags);
				  if(pNTV2Params->uartRxFifoSize < UARTRXFIFOSIZE) {
					 pNTV2Params->uartRxFifo[pNTV2Params->uartRxFifoSize++] = byte;
				  } else {
					  MSG("%s: Virtual UART Fifo 1 overrun; lost %02x\n",
						  pNTV2Params->name, byte);
					  pNTV2Params->uartRxFifoOverrun = 1;
				  }
				  spin_unlock_irqrestore(&pNTV2Params->uartRxFifoLock, flags);
				  uartControl = ReadUARTControl(deviceNumber);
			   }
			}

			{
			   ULWord uartControl = ReadUARTControl2(deviceNumber);
			   unsigned long flags;

			   while(uartControl & BIT_4) {
				  ULWord uartData = ReadUARTReceiveData2(deviceNumber);
				  UByte byte = uartData & 0xff;

				  spin_lock_irqsave(&pNTV2Params->uartRxFifoLock2, flags);
				  if(pNTV2Params->uartRxFifoSize2 < UARTRXFIFOSIZE) {
					 pNTV2Params->uartRxFifo2[pNTV2Params->uartRxFifoSize2++] = byte;
				  } else {
					 MSG("%s: Virtual UART Fifo 2 overrun; lost %02x\n",
						 pNTV2Params->name, byte);
					 pNTV2Params->uartRxFifoOverrun2 = 1;
				  }
				  spin_unlock_irqrestore(&pNTV2Params->uartRxFifoLock2, flags);
				  uartControl = ReadUARTControl2(deviceNumber);
			   }
			}
# endif	// UARTRXFIFOSIZE
		}

		// UART Tx
		if ( statusRegister & BIT_24 )
		{
			ClearUartTxInterrupt(deviceNumber);
			interruptHousekeeping(pNTV2Params, eUartTx);
# ifdef UARTTXFIFOSIZE
			{
			   ULWord value;
			   unsigned long flags;
			   unsigned i, j;

			   spin_lock_irqsave(&pNTV2Params->uartTxFifoLock, flags);

			   for(i = 0; i < pNTV2Params->uartTxFifoSize; ++i) {
				  ULWord control = ReadUARTControl(deviceNumber);
				  if(control & BIT_2) break;
				  value = pNTV2Params->uartTxFifo[i];
				  WriteUARTTransmitData(deviceNumber, value);
			   }

			   for(j = 0; j < i; ++j) {
				  pNTV2Params->uartTxFifo[j] = pNTV2Params->uartTxFifo[i+j];
			   }

			   pNTV2Params->uartTxFifoSize -= i;

			   spin_unlock_irqrestore(&pNTV2Params->uartTxFifoLock, flags);
			}
# endif	// UARTTXFIFOSIZE
		}

		// UART Tx2
		if ( statusRegister & BIT_26 )
		{
			ClearUartTxInterrupt2(deviceNumber);
			interruptHousekeeping(pNTV2Params, eUartTx2);
# ifdef UARTTXFIFOSIZE
			{
			   ULWord value;
			   unsigned long flags;
			   unsigned i, j;

			   spin_lock_irqsave(&pNTV2Params->uartTxFifoLock2, flags);

			   for(i = 0; i < pNTV2Params->uartTxFifoSize2; ++i) {
				  ULWord control = ReadUARTControl2(deviceNumber);
				  if(control & BIT_2) break;
				  value = pNTV2Params->uartTxFifo2[i];
				  WriteUARTTransmitData2(deviceNumber, value);
			   }

			   for(j = 0; j < i; ++j) {
				  pNTV2Params->uartTxFifo2[j] = pNTV2Params->uartTxFifo2[i+j];
			   }

			   pNTV2Params->uartTxFifoSize2 -= i;

			   spin_unlock_irqrestore(&pNTV2Params->uartTxFifoLock2, flags);
			}
# endif	// UARTTXFIFOSIZE
		}
		++handled;
	}
#endif
	{
		ULWord statusRegister = ReadStatusRegister(deviceNumber);
		bool autoCirculateLocked = false;
		unsigned long flags = 0;

		// Test for each interrupt and send a wakeup for each one that's active

		if ( statusRegister & kIntAuxVerticalActive )
		{
			interruptHousekeeping(pNTV2Params,eAuxVerticalInterrupt);
		}
#if 0
		if( statusRegister & kIntAudioInWrapActive )
		{
			// We don't do anything with audio in wrap
		}
		if( statusRegister & kIntAudioOutWrapActive )
		{
			// Nor do we do anything with audio out wrap
		}
#endif
		if( statusRegister & kIntAudioWrapActive )
		{
			interruptHousekeeping(pNTV2Params, eAudio);
		}

        if ( statusRegister & kIntInput1VBLActive )
        {
            ClearInput1VerticalInterrupt(deviceNumber);

            // save the interrupt time
            audioClock = GetAudioClock(deviceNumber);
            WriteRegister(deviceNumber, kVRegTimeStampLastInput1VerticalLo, audioClock & 0xFFFF, NO_MASK, NO_SHIFT);
            WriteRegister(deviceNumber, kVRegTimeStampLastInput1VerticalHi, audioClock >> 32, NO_MASK, NO_SHIFT);
#if 0
            if(!autoCirculateLocked)
            {
                ntv2_spin_lock_irqsave(&pNTV2Params->_autoCirculateLock, flags);
                autoCirculateLocked = true;
            }
            OemAutoCirculate(deviceNumber, NTV2CROSSPOINT_INPUT1);
#endif			
            interruptHousekeeping(pNTV2Params, eInput1);
        }

		if ( statusRegister & kIntInput2VBLActive )
		{
			ClearInput2VerticalInterrupt(deviceNumber);

			// save the interrupt time
            audioClock = GetAudioClock(deviceNumber);
            WriteRegister(deviceNumber, kVRegTimeStampLastInput2VerticalLo, audioClock & 0xFFFF, NO_MASK, NO_SHIFT);
            WriteRegister(deviceNumber, kVRegTimeStampLastInput2VerticalHi, audioClock >> 32, NO_MASK, NO_SHIFT);
#if 0
			if(!autoCirculateLocked)
			{
				ntv2_spin_lock_irqsave(&pNTV2Params->_autoCirculateLock, flags);
				autoCirculateLocked = true;
			}
			OemAutoCirculate(deviceNumber, NTV2CROSSPOINT_INPUT2);
#endif			
			interruptHousekeeping(pNTV2Params, eInput2);
		}

		if ( status2Register & kIntInput3VBLActive )
		{
			ClearInput3VerticalInterrupt(deviceNumber);

            // save the interrupt time
            audioClock = GetAudioClock(deviceNumber);
            WriteRegister(deviceNumber, kVRegTimeStampLastInput3VerticalLo, audioClock & 0xFFFF, NO_MASK, NO_SHIFT);
            WriteRegister(deviceNumber, kVRegTimeStampLastInput3VerticalHi, audioClock >> 32, NO_MASK, NO_SHIFT);
#if 0
			if(!autoCirculateLocked)
			{
				ntv2_spin_lock_irqsave(&pNTV2Params->_autoCirculateLock, flags);
				autoCirculateLocked = true;
			}
			OemAutoCirculate(deviceNumber, NTV2CROSSPOINT_INPUT3);
#endif			
			interruptHousekeeping(pNTV2Params, eInput3);
		}
        
		if ( status2Register & kIntInput4VBLActive )
		{
			ClearInput4VerticalInterrupt(deviceNumber);

            // save the interrupt time
            audioClock = GetAudioClock(deviceNumber);
            WriteRegister(deviceNumber, kVRegTimeStampLastInput4VerticalLo, audioClock & 0xFFFF, NO_MASK, NO_SHIFT);
            WriteRegister(deviceNumber, kVRegTimeStampLastInput4VerticalHi, audioClock >> 32, NO_MASK, NO_SHIFT);
#if 0
			if(!autoCirculateLocked)
			{
				ntv2_spin_lock_irqsave(&pNTV2Params->_autoCirculateLock, flags);
				autoCirculateLocked = true;
			}
			OemAutoCirculate(deviceNumber, NTV2CROSSPOINT_INPUT4);
#endif			
			interruptHousekeeping(pNTV2Params, eInput4);
		}
        
		if ( status2Register & kIntInput5VBLActive )
		{
			ClearInput5VerticalInterrupt(deviceNumber);

            // save the interrupt time
            audioClock = GetAudioClock(deviceNumber);
            WriteRegister(deviceNumber, kVRegTimeStampLastInput5VerticalLo, audioClock & 0xFFFF, NO_MASK, NO_SHIFT);
            WriteRegister(deviceNumber, kVRegTimeStampLastInput5VerticalHi, audioClock >> 32, NO_MASK, NO_SHIFT);
#if 0
			if(!autoCirculateLocked)
			{
				ntv2_spin_lock_irqsave(&pNTV2Params->_autoCirculateLock, flags);
				autoCirculateLocked = true;
			}
			OemAutoCirculate(deviceNumber, NTV2CROSSPOINT_INPUT5);
#endif			
			interruptHousekeeping(pNTV2Params, eInput5);
		}
		if ( status2Register & kIntInput6VBLActive )
		{
			ClearInput6VerticalInterrupt(deviceNumber);

            // save the interrupt time
            audioClock = GetAudioClock(deviceNumber);
            WriteRegister(deviceNumber, kVRegTimeStampLastInput6VerticalLo, audioClock & 0xFFFF, NO_MASK, NO_SHIFT);
            WriteRegister(deviceNumber, kVRegTimeStampLastInput6VerticalHi, audioClock >> 32, NO_MASK, NO_SHIFT);
#if 0
			if(!autoCirculateLocked)
			{
				ntv2_spin_lock_irqsave(&pNTV2Params->_autoCirculateLock, flags);
				autoCirculateLocked = true;
			}
			OemAutoCirculate(deviceNumber, NTV2CROSSPOINT_INPUT6);
#endif			
			interruptHousekeeping(pNTV2Params, eInput6);
		}
		if ( status2Register & kIntInput7VBLActive )
		{
			ClearInput7VerticalInterrupt(deviceNumber);

            // save the interrupt time
            audioClock = GetAudioClock(deviceNumber);
            WriteRegister(deviceNumber, kVRegTimeStampLastInput7VerticalLo, audioClock & 0xFFFF, NO_MASK, NO_SHIFT);
            WriteRegister(deviceNumber, kVRegTimeStampLastInput7VerticalHi, audioClock >> 32, NO_MASK, NO_SHIFT);
#if 0
			if(!autoCirculateLocked)
			{
				ntv2_spin_lock_irqsave(&pNTV2Params->_autoCirculateLock, flags);
				autoCirculateLocked = true;
			}
			OemAutoCirculate(deviceNumber, NTV2CROSSPOINT_INPUT7);
#endif			
			interruptHousekeeping(pNTV2Params, eInput7);
		}
		if ( status2Register & kIntInput8VBLActive )
		{
			ClearInput8VerticalInterrupt(deviceNumber);

            // save the interrupt time
            audioClock = GetAudioClock(deviceNumber);
            WriteRegister(deviceNumber, kVRegTimeStampLastInput8VerticalLo, audioClock & 0xFFFF, NO_MASK, NO_SHIFT);
            WriteRegister(deviceNumber, kVRegTimeStampLastInput8VerticalHi, audioClock >> 32, NO_MASK, NO_SHIFT);
#if 0
			if(!autoCirculateLocked)
			{
				ntv2_spin_lock_irqsave(&pNTV2Params->_autoCirculateLock, flags);
				autoCirculateLocked = true;
			}
			OemAutoCirculate(deviceNumber, NTV2CROSSPOINT_INPUT8);
#endif			
			interruptHousekeeping(pNTV2Params, eInput8);
		}
		
		if ( statusRegister & kIntOutput1VBLActive )
		{
			ClearOutputVerticalInterrupt(deviceNumber);

            // save the interrupt time
            audioClock = GetAudioClock(deviceNumber);
            WriteRegister(deviceNumber, kVRegTimeStampLastOutputVerticalLo, audioClock & 0xFFFF, NO_MASK, NO_SHIFT);
            WriteRegister(deviceNumber, kVRegTimeStampLastOutputVerticalHi, audioClock >> 32, NO_MASK, NO_SHIFT);
#if 0
            // Advance the Output1 frame if auto-circulate is enabled
			if(!autoCirculateLocked)
			{
				ntv2_spin_lock_irqsave(&pNTV2Params->_autoCirculateLock, flags);
				autoCirculateLocked = true;
			}
			OemAutoCirculate(deviceNumber, NTV2CROSSPOINT_CHANNEL1);
#endif			
			interruptHousekeeping(pNTV2Params, eVerticalInterrupt);
#if 0
			if( !NTV2DeviceCanDoMultiFormat(pNTV2Params->_DeviceID) )
			{
				OemAutoCirculate(deviceNumber, NTV2CROSSPOINT_CHANNEL2);
				if( NTV2DeviceGetNumVideoChannels(pNTV2Params->_DeviceID) > 2)
				{
					OemAutoCirculate(deviceNumber, NTV2CROSSPOINT_CHANNEL3);
					OemAutoCirculate(deviceNumber, NTV2CROSSPOINT_CHANNEL4);
				}
			}
#endif			
		}

		if ( statusRegister & kIntOutput2VBLActive )
		{
			ClearOutput2VerticalInterrupt(deviceNumber);

            // save the interrupt time
            audioClock = GetAudioClock(deviceNumber);
            WriteRegister(deviceNumber, kVRegTimeStampLastOutput2VerticalLo, audioClock & 0xFFFF, NO_MASK, NO_SHIFT);
            WriteRegister(deviceNumber, kVRegTimeStampLastOutput2VerticalHi, audioClock >> 32, NO_MASK, NO_SHIFT);
#if 0
			if(!autoCirculateLocked)
			{
				ntv2_spin_lock_irqsave(&pNTV2Params->_autoCirculateLock, flags);
				autoCirculateLocked = true;
			}
			OemAutoCirculate(deviceNumber, NTV2CROSSPOINT_CHANNEL2);
#endif			
			interruptHousekeeping(pNTV2Params, eOutput2);
		}
		if ( statusRegister & kIntOutput3VBLActive )
		{
			ClearOutput3VerticalInterrupt(deviceNumber);

            // save the interrupt time
            audioClock = GetAudioClock(deviceNumber);
            WriteRegister(deviceNumber, kVRegTimeStampLastOutput3VerticalLo, audioClock & 0xFFFF, NO_MASK, NO_SHIFT);
            WriteRegister(deviceNumber, kVRegTimeStampLastOutput3VerticalHi, audioClock >> 32, NO_MASK, NO_SHIFT);
#if 0
			if(!autoCirculateLocked)
			{
				ntv2_spin_lock_irqsave(&pNTV2Params->_autoCirculateLock, flags);
				autoCirculateLocked = true;
			}
			OemAutoCirculate(deviceNumber, NTV2CROSSPOINT_CHANNEL3);
#endif			
			interruptHousekeeping(pNTV2Params, eOutput3);
		}
		if ( statusRegister & kIntOutput4VBLActive )
		{
			ClearOutput4VerticalInterrupt(deviceNumber);

            // save the interrupt time
            audioClock = GetAudioClock(deviceNumber);
            WriteRegister(deviceNumber, kVRegTimeStampLastOutput4VerticalLo, audioClock & 0xFFFF, NO_MASK, NO_SHIFT);
            WriteRegister(deviceNumber, kVRegTimeStampLastOutput4VerticalHi, audioClock >> 32, NO_MASK, NO_SHIFT);
#if 0
			if(!autoCirculateLocked)
			{
				ntv2_spin_lock_irqsave(&pNTV2Params->_autoCirculateLock, flags);
				autoCirculateLocked = true;
			}
			OemAutoCirculate(deviceNumber, NTV2CROSSPOINT_CHANNEL4);
#endif			
			interruptHousekeeping(pNTV2Params, eOutput4);
		}

		if ( status2Register & kIntOutput5VBLActive )
		{
			ClearOutput5VerticalInterrupt(deviceNumber);

            // save the interrupt time
            audioClock = GetAudioClock(deviceNumber);
            WriteRegister(deviceNumber, kVRegTimeStampLastOutput5VerticalLo, audioClock & 0xFFFF, NO_MASK, NO_SHIFT);
            WriteRegister(deviceNumber, kVRegTimeStampLastOutput5VerticalHi, audioClock >> 32, NO_MASK, NO_SHIFT);
#if 0		
			if(!autoCirculateLocked)
			{
				ntv2_spin_lock_irqsave(&pNTV2Params->_autoCirculateLock, flags);
				autoCirculateLocked = true;
			}
			OemAutoCirculate(deviceNumber, NTV2CROSSPOINT_CHANNEL5);
#endif			
			interruptHousekeeping(pNTV2Params, eOutput5);
		}
		if ( status2Register & kIntOutput6VBLActive )
		{
			ClearOutput6VerticalInterrupt(deviceNumber);

            // save the interrupt time
            audioClock = GetAudioClock(deviceNumber);
            WriteRegister(deviceNumber, kVRegTimeStampLastOutput6VerticalLo, audioClock & 0xFFFF, NO_MASK, NO_SHIFT);
            WriteRegister(deviceNumber, kVRegTimeStampLastOutput6VerticalHi, audioClock >> 32, NO_MASK, NO_SHIFT);
#if 0
			if(!autoCirculateLocked)
			{
				ntv2_spin_lock_irqsave(&pNTV2Params->_autoCirculateLock, flags);
				autoCirculateLocked = true;
			}
			OemAutoCirculate(deviceNumber, NTV2CROSSPOINT_CHANNEL6);
#endif			
			interruptHousekeeping(pNTV2Params, eOutput6);
		}
		if ( status2Register & kIntOutput7VBLActive )
		{
			ClearOutput7VerticalInterrupt(deviceNumber);

            // save the interrupt time
            audioClock = GetAudioClock(deviceNumber);
            WriteRegister(deviceNumber, kVRegTimeStampLastOutput7VerticalLo, audioClock & 0xFFFF, NO_MASK, NO_SHIFT);
            WriteRegister(deviceNumber, kVRegTimeStampLastOutput7VerticalHi, audioClock >> 32, NO_MASK, NO_SHIFT);
#if 0
			if(!autoCirculateLocked)
			{
				ntv2_spin_lock_irqsave(&pNTV2Params->_autoCirculateLock, flags);
				autoCirculateLocked = true;
			}
			OemAutoCirculate(deviceNumber, NTV2CROSSPOINT_CHANNEL7);
#endif			
			interruptHousekeeping(pNTV2Params, eOutput7);
		}
		if ( status2Register & kIntOutput8VBLActive )
		{
			ClearOutput8VerticalInterrupt(deviceNumber);

            // save the interrupt time
            audioClock = GetAudioClock(deviceNumber);
            WriteRegister(deviceNumber, kVRegTimeStampLastOutput8VerticalLo, audioClock & 0xFFFF, NO_MASK, NO_SHIFT);
            WriteRegister(deviceNumber, kVRegTimeStampLastOutput8VerticalHi, audioClock >> 32, NO_MASK, NO_SHIFT);
#if 0
			if(!autoCirculateLocked)
			{
				ntv2_spin_lock_irqsave(&pNTV2Params->_autoCirculateLock, flags);
				autoCirculateLocked = true;
			}
			OemAutoCirculate(deviceNumber, NTV2CROSSPOINT_CHANNEL8);
#endif			
			interruptHousekeeping(pNTV2Params, eOutput8);
		}

		if(autoCirculateLocked)
		{
			ntv2_spin_unlock_irqrestore(&pNTV2Params->_autoCirculateLock, flags);
		}
		
		++handled;
	}

	return IRQ_RETVAL(handled);
}


/* Function to load the module */

// Define version independent print prefix and print suffix macros
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0))
#define P_PRE seq_printf( m,
#else
#define P_PRE len += snprintf( buf + len, STRMAX,
#endif
#define P_SUFF );

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0))
static int aja_read_procmem_output( struct seq_file *m, void *v )
#else
static int aja_read_procmem( char *buf, char **start, off_t offset,
		int count, int *eof, void *data )
#endif
{
	int  numberOfBoards =0;
	int  channelCount;
	char tmpBuf[STRMAX];
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0))
	int  len =0;
#endif
	int  i;
	NTV2PrivateParams* pNTV2Params;

//	len += snprintf( buf + len, STRMAX, "AJA Driver Name: %s\n", getNTV2ModuleParams()->name);
	P_PRE "AJA Driver Name: %s\n", getNTV2ModuleParams()->name P_SUFF
	getDriverVersionString( tmpBuf, STRMAX );
	P_PRE "Version: %s\n", tmpBuf P_SUFF

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdate-time"        
    P_PRE "Date: %s %s\n", __DATE__, __TIME__ P_SUFF
#pragma GCC diagnostic pop        

	// get number of boards
	numberOfBoards = getNTV2ModuleParams()->numNTV2Devices;
	// for each board we get the BITFILE_INFO_STRUCT
	for ( i = 0; i < numberOfBoards; i++ )
	{
		ULWord boardID = (ULWord)getDeviceID(i);
		pNTV2Params = getNTV2Params(i);

		P_PRE "\nCard #: %d\n", i P_SUFF
		getDeviceVersionString(i, tmpBuf, STRMAX );
		P_PRE "Device Version: %s\n", tmpBuf P_SUFF
		// query the FPGA version and board id
		getPCIFPGAVersionString( i, tmpBuf, STRMAX );
		P_PRE "Firmware Version: %s\n", tmpBuf P_SUFF
		getDeviceIDString( i, tmpBuf, STRMAX );
		P_PRE "Board ID: %s\n", tmpBuf P_SUFF

		getDeviceSerialNumberString( i, tmpBuf, STRMAX );
		P_PRE "Serial Number: %s\n", tmpBuf P_SUFF

		P_PRE "Registered FPGA IRQ: 0x%x\n", pNTV2Params->_ntv2IRQ[eIrqFpga] P_SUFF

		itoa64(pNTV2Params->_interruptCount[eVerticalInterrupt], tmpBuf);
		channelCount = NTV2DeviceCanDoMultiFormat(boardID) ? NTV2DeviceGetNumVideoChannels(boardID) : 1;
		if (channelCount >= 1)
		{
			P_PRE "Output 1 vertical interrupts: %s\n", tmpBuf P_SUFF
			itoa64(pNTV2Params->_interruptCount[eOutput2], tmpBuf);
		}
		if (channelCount >= 2)
		{
			P_PRE "Output 2 vertical interrupts: %s\n", tmpBuf P_SUFF
			itoa64(pNTV2Params->_interruptCount[eOutput3], tmpBuf);
		}
		if (channelCount >= 3)
		{
			P_PRE "Output 3 vertical interrupts: %s\n", tmpBuf P_SUFF
			itoa64(pNTV2Params->_interruptCount[eOutput4], tmpBuf);
		}
		if (channelCount >= 4)
		{
			P_PRE "Output 4 vertical interrupts: %s\n", tmpBuf P_SUFF
			itoa64(pNTV2Params->_interruptCount[eOutput5], tmpBuf);
		}
		if (channelCount >= 5)
		{
			P_PRE "Output 5 vertical interrupts: %s\n", tmpBuf P_SUFF
			itoa64(pNTV2Params->_interruptCount[eOutput6], tmpBuf);
		}
		if (channelCount >= 6)
		{
			P_PRE "Output 6 vertical interrupts: %s\n", tmpBuf P_SUFF
			itoa64(pNTV2Params->_interruptCount[eOutput7], tmpBuf);
		}
		if (channelCount >= 7)
		{
			P_PRE "Output 7 vertical interrupts: %s\n", tmpBuf P_SUFF
			itoa64(pNTV2Params->_interruptCount[eOutput8], tmpBuf);
		}
		if (channelCount >= 8)
		{
			P_PRE "Output 8 vertical interrupts: %s\n", tmpBuf P_SUFF
		}

		channelCount = NTV2DeviceGetNumVideoInputs(boardID);
		itoa64(pNTV2Params->_interruptCount[eInput1], tmpBuf);
		if (channelCount >= 1)
		{
			P_PRE "Input 1 vertical interrupts: %s\n", tmpBuf P_SUFF
			itoa64(pNTV2Params->_interruptCount[eInput2], tmpBuf);
		}
		if (channelCount >= 2)
		{
			P_PRE "Input 2 vertical interrupts: %s\n", tmpBuf P_SUFF
			itoa64(pNTV2Params->_interruptCount[eInput3], tmpBuf);
		}
		if (channelCount >= 3)
		{
			P_PRE "Input 3 vertical interrupts: %s\n", tmpBuf P_SUFF
			itoa64(pNTV2Params->_interruptCount[eInput3], tmpBuf);
		}
		if (channelCount >= 4)
		{
			P_PRE "Input 4 vertical interrupts: %s\n", tmpBuf P_SUFF
			itoa64(pNTV2Params->_interruptCount[eInput5], tmpBuf);
		}
		if (channelCount >= 5)
		{
			P_PRE "Input 5 vertical interrupts: %s\n", tmpBuf P_SUFF
			itoa64(pNTV2Params->_interruptCount[eInput6], tmpBuf);
		}
		if (channelCount >= 6)
		{
			P_PRE "Input 6 vertical interrupts: %s\n", tmpBuf P_SUFF
			itoa64(pNTV2Params->_interruptCount[eInput7], tmpBuf);
		}
		if (channelCount >= 7)
		{
			P_PRE "Input 7 vertical interrupts: %s\n", tmpBuf P_SUFF
			itoa64(pNTV2Params->_interruptCount[eInput8], tmpBuf);
		}
		if (channelCount >= 8)
		{
			P_PRE "Input 8 vertical interrupts: %s\n", tmpBuf P_SUFF
		}
		itoa64(pNTV2Params->_interruptCount[eUartRx], tmpBuf);
		P_PRE "UART Rx interrupts: %s\n", tmpBuf P_SUFF
		itoa64(pNTV2Params->_interruptCount[eUartTx], tmpBuf);
		P_PRE "UART Tx interrupts: %s\n", tmpBuf P_SUFF
		itoa64(pNTV2Params->_interruptCount[eUartTx2], tmpBuf);
		P_PRE "UART Tx2 interrupts: %s\n", tmpBuf P_SUFF
		itoa64(pNTV2Params->_interruptCount[eAuxVerticalInterrupt], tmpBuf);
		P_PRE "Aux Vertical interrupts: %s\n", tmpBuf P_SUFF
	}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0))
	return 0;
#else
	*eof = 1;
	// return len contains how many bytes added to the proc page
	return len;
#endif
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0))
static int aja_read_procmem_open( struct inode *inode, struct file *file )
{
	return single_open( file, aja_read_procmem_output, NULL );
}

static const struct of_device_id ntv2_platform_table[] =
{
    { .compatible = "aja,ntv2" },
    { /* End of List */ }
};

static struct platform_driver ntv2_platform_driver =
{
    .driver = {
        .name = NTV2_DRIVER_NAME,
        .of_match_table = ntv2_platform_table,
        //.pm = &platform_ops
    },
    .probe = platform_probe,
//    .remove = platform_remove
};

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,6,0))
static struct proc_ops ntv2_proc_ops =
{
	.proc_lseek		= seq_lseek,
	.proc_read		= seq_read,
	.proc_open		= aja_read_procmem_open,
	.proc_release	= seq_release
};
#else
static struct file_operations ntv2_proc_fops =
{
	.owner		= THIS_MODULE,
	.llseek		= seq_lseek,
	.read		= seq_read,
	.open		= aja_read_procmem_open,
	.release	= seq_release
};
#endif
#endif

static int reboot_handler(struct notifier_block *this, unsigned long code, void *x)
{
	ULWord  i;

	// We don't want to generate interrupts if the system is rebooting
	// This could confuse the BIOS and cause the system to hang
	// Disable interrupts on all boards so we'll come up quietly
	for ( i=0; i< getNTV2ModuleParams()->numNTV2Devices; i++ )
	{
        if (getNTV2Params(i) != NULL)
            DisableAllInterrupts(i);
	}

	return NOTIFY_DONE;
}

#if defined(AJA_CREATE_DEVICE_NODES)
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6,2,0))
static int aja_ntv2_dev_uevent(const struct device *dev, struct kobj_uevent_env *env)
#else
static int aja_ntv2_dev_uevent(struct device *dev, struct kobj_uevent_env *env)
#endif
{
	add_uevent_var(env, "DEVMODE=%#o", 0666);
	return 0;
}
#endif

static UWord sDeviceNumber;

static int __init aja_ntv2_module_init(void)
{
	int res;
	int i;
	char versionString[STRMAX];
    char* driverModeString;
#if defined(AJA_CREATE_DEVICE_NODES)
	struct class *ntv2_class = NULL;
#endif

    sDeviceNumber = 0;
	for (i = 0; i < NTV2_MAXBOARDS; i++)
	{
		NTV2Params[i] = NULL;
	}

	if (getNTV2ModuleParams()->name == NULL)
	{
        getNTV2ModuleParams()->name = NTV2_MODULE_NAME;
	}
	if (getNTV2ModuleParams()->driverName == NULL)
	{
        getNTV2ModuleParams()->driverName = NTV2_DRIVER_NAME;
	}

	MSG("%s: module init begin\n", getNTV2ModuleParams()->name);
	getDriverVersionString(versionString, STRMAX);
	MSG("%s: driver version %s\n",
		getNTV2ModuleParams()->name, versionString);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdate-time"        
    MSG("%s: driver date %s %s\n", getNTV2ModuleParams()->name, __DATE__, __TIME__);
#pragma GCC diagnostic pop        

    // determine driver mode
    strncpy(versionString, DriverMode, STRMAX);
    versionString[STRMAX - 1] = '\0';
    driverModeString = strstrip(versionString);
    if (strcmp(driverModeString, "all") == 0)
    {
        getNTV2ModuleParams()->driverMode = eDriverModeAll;
        MSG("%s: driver mode: all functions\n", getNTV2ModuleParams()->name);;
    }
    else if (strcmp(driverModeString, "register") == 0)
    {
        getNTV2ModuleParams()->driverMode = eDriverModeRegister;
        MSG("%s: driver mode: register access only\n", getNTV2ModuleParams()->name);;
    }
    else if (strcmp(driverModeString, "genlock") == 0)
    {
        getNTV2ModuleParams()->driverMode = eDriverModeGenlock;
        MSG("%s: driver mode: register access with genlock\n", getNTV2ModuleParams()->name);;
    }
    else
    {
        getNTV2ModuleParams()->driverMode = eDriverModeAll;
        MSG("%s: unknown driver mode: %s, default to all functions\n",
            getNTV2ModuleParams()->name, driverModeString);
    }

	ntv2_uart_driver.owner			= THIS_MODULE;
	ntv2_uart_driver.driver_name	= getNTV2ModuleParams()->driverName;
	ntv2_uart_driver.dev_name		= NTV2_TTY_NAME;
	ntv2_uart_driver.nr				= NTV2_MAXBOARDS;

#if 0
	res = uart_register_driver(&ntv2_uart_driver);
	if (res < 0) {
		MSG("%s: *error* uart_register_driver failed code %d\n",
			getNTV2ModuleParams()->name, res);
		return res;
	}
	getNTV2ModuleParams()->uart_driver = &ntv2_uart_driver;
	getNTV2ModuleParams()->uart_max = NTV2_MAXBOARDS;
	atomic_set(&getNTV2ModuleParams()->uart_index, 0);
#endif

#if defined(AJA_CREATE_DEVICE_NODES)
	// Create device class
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6,4,0))
	ntv2_class = class_create(getNTV2ModuleParams()->driverName);
#else
	ntv2_class = class_create(THIS_MODULE, getNTV2ModuleParams()->driverName);
#endif
	if (IS_ERR(ntv2_class))
	{
		res = PTR_ERR(ntv2_class);
		MSG("%s: Failed to create device class; code %d\n",
			getNTV2ModuleParams()->name, res);
	}
    else
    {
        ntv2_class->dev_uevent = aja_ntv2_dev_uevent;
        getNTV2ModuleParams()->class = ntv2_class;
    }
#endif

	// register driver
	MSG("%s: register driver %s\n",
		getNTV2ModuleParams()->name, getNTV2ModuleParams()->driverName);

	// register device with kernel
	MSG("%s: register chrdev %s\n",	getNTV2ModuleParams()->name, getNTV2ModuleParams()->driverName);
	res = register_chrdev(NTV2_MAJOR,
						  getNTV2ModuleParams()->driverName,
						  &ntv2_fops);
	if (res < 0 )
	{
		MSG("%s: can't register device with kernel\n",
			getNTV2ModuleParams()->name);
		return res;
	}
	getNTV2ModuleParams()->NTV2Major = res;
	MSG("%s: chrdev major %d\n", getNTV2ModuleParams()->name, getNTV2ModuleParams()->NTV2Major);

	// register platform device (calls probe)
	platform_driver_register(&ntv2_platform_driver);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0))
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,6,0))
	proc_create( "driver/aja",
			0 /* default mode */,
			NULL /* parent dir */,
			&ntv2_proc_ops);
#else	
	proc_create( "driver/aja",
			0 /* default mode */,
			NULL /* parent dir */,
			&ntv2_proc_fops);
#endif	
#else
	create_proc_read_entry( "driver/aja",
			0 /* default mode */,
			NULL /* parent dir */,
			aja_read_procmem,
			NULL /* client data */ );
#endif

	// Ask to be notified before the system reboots
	register_reboot_notifier(&reboot_notifier);

	MSG("%s: module init end\n", getNTV2ModuleParams()->name);

	return 0;
}

/* Function to clean up and unload the module */
static void aja_ntv2_module_cleanup(void)
{
	ULWord  i;
	ULWord  j;
	int irqIndex;
#if defined(AJA_CREATE_DEVICE_NODES)
    dev_t dev;
#endif

	MSG("%s: module exit begin\n", getNTV2ModuleParams()->name);
	
	unregister_reboot_notifier(&reboot_notifier);

	for (i = 0; i < getNTV2ModuleParams()->numNTV2Devices; i++)
	{
		NTV2PrivateParams *ntv2pp = getNTV2Params(i);
        if (ntv2pp == NULL) continue;

		MSG("%s: device %d release\n", ntv2pp->name, i);

#if defined(AJA_CREATE_DEVICE_NODES)
        if (getNTV2ModuleParams()->class != NULL)
        {
            dev = MKDEV(getNTV2ModuleParams()->NTV2Major, i);
            device_destroy(getNTV2ModuleParams()->class, dev);
            cdev_del(&ntv2pp->cdev);
        }
#endif

        if (ntv2pp->registerEnable)
        {
            if (getNTV2ModuleParams()->driverMode == eDriverModeAll)
            {
                // stop all dma engines
                dmaRelease(i);

                // disable and unregister interrupts,
                DisableAllInterrupts(i);

                // disable autocirculate
                AutoCirculateInitialize(i);
            }

            // disable hdmi monitor
            for (j = 0; j < NTV2_MAX_HDMI_MONITOR; j++)
            {
                if (ntv2pp->m_pHDMIInputMonitor[j] != NULL)
                {
                    ntv2_hdmiin_disable(ntv2pp->m_pHDMIInputMonitor[j]);
                }
                if (ntv2pp->m_pHDMIIn4Monitor[j] != NULL)
                {
                    ntv2_hdmiin4_disable(ntv2pp->m_pHDMIIn4Monitor[j]);
                }
                if (ntv2pp->m_pHDMIOut4Monitor[j] != NULL)
                {
                    ntv2_hdmiout4_disable(ntv2pp->m_pHDMIOut4Monitor[j]);
                }	
            }
		
            // close hdmi monitor
            for (j = 0; j < NTV2_MAX_HDMI_MONITOR; j++)
            {
                if (ntv2pp->m_pHDMIInputMonitor[j] != NULL)
                {
                    ntv2_hdmiin_close(ntv2pp->m_pHDMIInputMonitor[j]);
                    ntv2pp->m_pHDMIInputMonitor[j] = NULL;
                }
                if (ntv2pp->m_pHDMIIn4Monitor[j] != NULL)
                {
                    ntv2_hdmiin4_close(ntv2pp->m_pHDMIIn4Monitor[j]);
                    ntv2pp->m_pHDMIIn4Monitor[j] = NULL;
                }
                if (ntv2pp->m_pHDMIOut4Monitor[j] != NULL)
                {
                    ntv2_hdmiout4_close(ntv2pp->m_pHDMIOut4Monitor[j]);
                    ntv2pp->m_pHDMIOut4Monitor[j] = NULL;
                }
            }

            if (ntv2pp->m_pSetupMonitor != NULL)
            {
                ntv2_setup_disable(ntv2pp->m_pSetupMonitor);
                ntv2_setup_close(ntv2pp->m_pSetupMonitor);
                ntv2pp->m_pSetupMonitor = NULL;
            }

            if (ntv2pp->m_pGenlock2Monitor != NULL)
            {
                ntv2_genlock2_disable(ntv2pp->m_pGenlock2Monitor);
                ntv2_genlock2_close(ntv2pp->m_pGenlock2Monitor);
                ntv2pp->m_pGenlock2Monitor = NULL;
            }

            // close the serial port
            if (ntv2pp->m_pSerialPort != NULL)
            {
                ntv2_serial_close(ntv2pp->m_pSerialPort);
                ntv2pp->m_pSerialPort = NULL;
            }

            if ((ntv2pp->platform_dev != NULL) &&
                (getNTV2ModuleParams()->driverMode == eDriverModeAll))
            {
                for(irqIndex = 0; irqIndex < eNumNTV2IRQDevices; ++irqIndex)
                {
                    if (ntv2pp->_ntv2IRQ[irqIndex] != 0)
                    {
                        MSG("%s: free irq 0x%x, dev_id %p\n",
                            ntv2pp->name, ntv2pp->_ntv2IRQ[irqIndex], (void *)ntv2pp);

                        devm_free_irq(&ntv2pp->platform_dev->dev,
                                      ntv2pp->_ntv2IRQ[irqIndex],
                                      (void *)ntv2pp);
                    }
                }
            }
        }

        platform_resources_release(i);
	}
	
	unregister_chrdev( getNTV2ModuleParams()->NTV2Major, getNTV2ModuleParams()->driverName);
	remove_proc_entry("driver/aja", NULL /* parent dir */);

#if defined(AJA_CREATE_DEVICE_NODES)
    if (getNTV2ModuleParams()->class != NULL)
    {
        class_destroy(getNTV2ModuleParams()->class);
    }
#endif

	for (i = 0; i < NTV2_MAXBOARDS; i++)
	{
		if (NTV2Params[i] != NULL)
		{
			vfree(NTV2Params[i]);
			NTV2Params[i] = NULL;
		}
	}

    platform_driver_unregister(&ntv2_platform_driver);

#if 0
	uart_unregister_driver(&ntv2_uart_driver);
#endif

    // clean up any VirtualData nodes that were allocated
    deleteAllVirtualDataNodes();

	MSG("%s: module exit end\n", getNTV2ModuleParams()->name);
}

static int platform_probe(struct platform_device *pd)
{
	NTV2PrivateParams *ntv2pp = NULL;
    ULWord deviceNumber;
	char versionString[STRMAX];
	Ntv2Status status;
	int vpidIndex;
	int intrIndex;
	int i;
	int ret;
    bool irqGood;
    bool irqFail;
#if defined(AJA_CREATE_DEVICE_NODES)
	dev_t dev;
	struct device *device = NULL;
#endif

	MSG("%s: probe check\n", getNTV2ModuleParams()->name);
    if((pd == NULL) || (pd->dev.of_node == NULL))
	{
        MSG("%s: device has no of_node\n", getNTV2ModuleParams()->name);
        return -EINVAL;
    }

	if (sDeviceNumber >= NTV2_MAXBOARDS)
	{
		MSG("%s: only %d boards supported\n", getNTV2ModuleParams()->name, NTV2_MAXBOARDS);
		return -EPERM;
	}

	if (NTV2Params[sDeviceNumber])
	{
		MSG("%s: attempt to probe previously allocated device %d\n",
			getNTV2ModuleParams()->name, sDeviceNumber);
		return -EPERM;
	}

	MSG("ntv2dev: probe device state size %d\n", (int)sizeof(NTV2PrivateParams));

	// Allocate space for keeping track of the device state
	NTV2Params[sDeviceNumber] = vmalloc( sizeof(NTV2PrivateParams) );
	if( NTV2Params[sDeviceNumber] == NULL)
	{
		MSG("%s: allocation of device state failed for device %d\n",
			getNTV2ModuleParams()->name, sDeviceNumber);
		return -ENOMEM;
	}
	getNTV2ModuleParams()->numNTV2Devices = sDeviceNumber + 1;
    deviceNumber = sDeviceNumber;
	sDeviceNumber++;

	ntv2pp = NTV2Params[deviceNumber];
	memset(ntv2pp, 0, sizeof(NTV2PrivateParams));
	ntv2pp->deviceNumber = deviceNumber;
	snprintf(ntv2pp->name, STRMAX, "ntv2dev%u", deviceNumber);

	InitInterruptBitLUT();

	MSG("%s: probe begin\n", ntv2pp->name);

	ntv2pp->pci_dev = NULL;
	ntv2pp->platform_dev = pd;

	// get memory map and interrupt
	ret = platform_resources_config(deviceNumber);
    if(ret)
	{
        MSG("%s: failed to configure platform resources\n", ntv2pp->name);
        goto fail;
	}

    WriteRegister(deviceNumber, kVRegPCIDeviceID, ntv2pp->pci_device, NO_MASK, NO_SHIFT);

	// video registers
	ntv2pp->_VideoAddress = ntv2pp->_mappedBAR0Address;
	ntv2pp->_VideoMemorySize = ntv2pp->_BAR0MemorySize;
		
	// init video register pointers
	initializeRegisterNames(deviceNumber, ntv2pp->_VideoAddress);

	// enable register access
	ntv2pp->registerEnable = true;

	// configure system context
	ntv2pp->systemContext.devNum = deviceNumber;
	ntv2pp->systemContext.pDevice = NULL;
	ntv2pp->systemContext.busNumber = ntv2pp->busNumber;

#ifdef SOFTWARE_UART_FIFO
    WriteRegister(deviceNumber, kVRegSoftwareUartFifo, 1, NO_MASK, NO_SHIFT);
#else
    WriteRegister(deviceNumber, kVRegSoftwareUartFifo, 0, NO_MASK, NO_SHIFT);
#endif
#ifdef SOFTWARE_UART_FIFO
#ifdef UARTRXFIFOSIZE
	// Initialize UART Fifo.
	ntv2pp->uartRxFifoSize = 0;
	ntv2pp->uartRxFifoOverrun = 0;
	spin_lock_init(&ntv2pp->uartRxFifoLock);
#endif	// UARTRXFIFOSIZE
#ifdef UARTTXFIFOSIZE
	ntv2pp->uartTxFifoSize = 0;
	spin_lock_init(&ntv2pp->uartTxFifoLock);
#endif	// UARTTXFIFOSIZE
#endif	// SOFTWARE_UART_FIFO

	// Initialize wait queues and interrupt bookkeeping
	for ( intrIndex = 0; intrIndex < eNumInterruptTypes; intrIndex++)
	{
		ntv2pp->_interruptCount[intrIndex] = 0;
		ntv2pp->_interruptHappened[intrIndex] = 0;
		init_waitqueue_head(&ntv2pp->_interruptWait[intrIndex]);
	}

	// Initialize I2C semaphore
	sema_init(&ntv2pp->_I2CMutex,1);

	// Initialize spinlocks
    spin_lock_init(&(ntv2pp->_registerSpinLock));

    for (i = 0; i < NUM_NWL_REGS; i++)
        spin_lock_init(&(ntv2pp->_nwlRegisterLock[i]));

    spin_lock_init(&(ntv2pp->_p2pInterruptControlRegisterLock));
	spin_lock_init(&(ntv2pp->_audioClockLock));
	spin_lock_init(&(ntv2pp->_autoCirculateLock));
	spin_lock_init(&(ntv2pp->_virtualRegisterLock));

	spin_lock_init(&(ntv2pp->_bankAndRegisterAccessLock));
	sema_init(&ntv2pp->_mailBoxSemaphore, 1);

	// Get board ID.  This is a bit of a hack to get it early, before
	// we are sure the board is really functional to solve a chicken
	// and egg problem.
	ntv2pp->_DeviceID = ReadDeviceIDRegister(deviceNumber);

    if (ntv2pp->_DeviceID == DEVICE_ID_IP25_T)
    {
        ntv2pp->_FsMemoryAddress = 0x60000000000;
        ntv2pp->_FsMemorySize = 0x80000000;
    }

	ntv2pp->_numberOfHWRegisters = NTV2DeviceGetMaxRegisterNumber(ntv2pp->_DeviceID);

	ntv2pp->_globalAudioPlaybackMode = NTV2_AUDIOPLAYBACK_NORMALAUTOCIRCULATE;
	ntv2pp->_startAudioNextFrame = false;

	ntv2pp->_audioSyncTolerance = 10000;
	ntv2pp->_syncChannels = 0;

	ntv2pp->_syncChannel1 = NTV2CROSSPOINT_FGKEY;
	ntv2pp->_syncChannel2 = NTV2CROSSPOINT_FGKEY;

    // Initialize some virtural registers
	ntv2pp->_ApplicationPID = 0;
	ntv2pp->_ApplicationReferenceCount = 0;
	ntv2pp->_ApplicationCode = 0;

    WriteRegister(deviceNumber, kVRegDriverVersion, NTV2_LINUX_DRIVER_VERSION, NO_MASK, NO_SHIFT);
    WriteRegister(deviceNumber, kVRegDeviceOnline, 1, NO_MASK, NO_SHIFT);
    WriteRegister(deviceNumber, kVRegEveryFrameTaskFilter, 1, NO_MASK, NO_SHIFT);
    WriteRegister(deviceNumber, kVRegFlashState, kProgramStateFinished, NO_MASK, NO_SHIFT);

    WriteRegister(deviceNumber, kVRegMonAncField1Offset, 0x8000, NO_MASK, NO_SHIFT);
    WriteRegister(deviceNumber, kVRegMonAncField2Offset, 0x6000, NO_MASK, NO_SHIFT);
    WriteRegister(deviceNumber, kVRegAncField1Offset, 0x4000, NO_MASK, NO_SHIFT);	//	Formerly 0x24000! Why?!?!
    WriteRegister(deviceNumber, kVRegAncField2Offset, 0x2000, NO_MASK, NO_SHIFT);	//	Formerly 0x12000! Why?!?!

	ntv2pp->_VirtualMailBoxTimeoutNS = 100000;	// In units of 100 ns, so this is 10 ms

    WriteRegister(deviceNumber, kVRegUserDefinedDBB, 0x0, NO_MASK, NO_SHIFT);
    WriteRegister(deviceNumber, kVRegEnableBT2020, 0x0, NO_MASK, NO_SHIFT);
    WriteRegister(deviceNumber, kVRegDisableAutoVPID, 0x0, NO_MASK, NO_SHIFT);
	
	for(vpidIndex = 0; i < 8; i++)
	{
		WriteRegister(deviceNumber, gChannelToSDIOutVPIDTransferCharacteristics[vpidIndex] , 0x0, NO_MASK, NO_SHIFT);
		WriteRegister(deviceNumber, gChannelToSDIOutVPIDColorimetry[vpidIndex], 0x0, NO_MASK, NO_SHIFT);
		WriteRegister(deviceNumber, gChannelToSDIOutVPIDLuminance[vpidIndex], 0x0, NO_MASK, NO_SHIFT);
	}

	if (NTV2DeviceIsSupported(ntv2pp->_DeviceID))
	{
		MSG("%s: board id 0x%x\n", ntv2pp->name, (ULWord)ntv2pp->_DeviceID);
	}
	else
	{
		MSG("%s: board id 0x%x not supported!\n", ntv2pp->name, (ULWord)ntv2pp->_DeviceID);
	}

	// Zero out procamp register images.  This does not write anything to the
	// hardware registers.  These must be initialized by the SDK which knows
	// about ranges, default values, etc.
	memset(&ntv2pp->_virtualProcAmpRegisters, 0, sizeof(VirtualProcAmpRegisters));
	memset(&ntv2pp->_hwProcAmpRegisterImage,  0, sizeof(HardwareProcAmpRegisterImage));
    
    irqGood = false;
    irqFail = false;
    if (getNTV2ModuleParams()->driverMode == eDriverModeAll)
    {        
        if (ntv2pp->platform_dev != NULL)
        {
            for(intrIndex = 0; intrIndex < eNumNTV2IRQDevices; ++intrIndex)
            {
                if (ntv2pp->_ntv2IRQ[intrIndex] != 0)
                {
                    ret = devm_request_irq(&ntv2pp->platform_dev->dev,
                                           ntv2pp->_ntv2IRQ[intrIndex],
                                           ntv2_irq_arr[intrIndex].irq_func,
                                           ntv2_irq_arr[intrIndex].flags,
                                           ntv2pp->name,
                                           (void *)ntv2pp);
                    if(ret)
                    {
                        MSG("%s: failed to register interrupt %d\n", ntv2pp->name, ntv2pp->_ntv2IRQ[intrIndex]);
                        irqFail = true;
                    }    
                    else
                    {
                        MSG("%s: registered irq %d\n", ntv2pp->name, ntv2pp->_ntv2IRQ[intrIndex]);
                        
                        if (ntv2_irq_arr[intrIndex].irq_type != IRQ_TYPE_NONE)
                        {
                            irq_set_irq_type(ntv2pp->_ntv2IRQ[intrIndex], ntv2_irq_arr[intrIndex].irq_type);
                        }
                        irqGood = true;
                    }
                }
                else
                {
                    MSG("%s: no attempt to register interrupt %d\n", ntv2pp->name, ntv2pp->_ntv2IRQ[intrIndex]);
                }
            }
        }
        if (irqFail) irqGood = false;
        ntv2pp->canDoInterrupt = irqGood;
        
	    SetupBoard(deviceNumber);

	    getDeviceVersionString(deviceNumber, versionString, STRMAX);
	    MSG("%s: detected device %s\n", ntv2pp->name, versionString);
	    getDeviceSerialNumberString(deviceNumber, versionString, STRMAX);
	    MSG("%s: serial number %s\n", ntv2pp->name, versionString);
	    getPCIFPGAVersionString(deviceNumber, versionString, STRMAX);
	    MSG("%s: firmware version %s\n", ntv2pp->name, versionString);

	    // initialize dma
	    dmaInit(deviceNumber);

        // initialize autocirculate
        AutoCirculateInitialize(deviceNumber);

 	    // configure hdmi input monitor
	    for (i = 0; i < NTV2_MAX_HDMI_MONITOR; i++)
	    {
		    ntv2pp->m_pHDMIInputMonitor[i] = NULL;
		    ntv2pp->m_pHDMIIn4Monitor[i] = NULL;
		    ntv2pp->m_pHDMIOut4Monitor[i] = NULL;
	    }
        ntv2pp->m_pSetupMonitor = NULL;
        ntv2pp->m_pGenlock2Monitor = NULL;

        if (((ntv2pp->_DeviceID >= DEVICE_ID_KONA5_3DLUT) &&
            (ntv2pp->_DeviceID <= DEVICE_ID_KONA5_OE12)) ||
            ((ntv2pp->_DeviceID >= DEVICE_ID_SOJI_3DLUT) &&
            (ntv2pp->_DeviceID <= DEVICE_ID_SOJI_DIAGS)))
	    {
            ntv2pp->m_pHDMIOut4Monitor[0] = ntv2_hdmiout4_open(&ntv2pp->systemContext, "ntv2hdmiout4", 0);
            if (ntv2pp->m_pHDMIOut4Monitor[0] != NULL)
            {
                status = ntv2_hdmiout4_configure(ntv2pp->m_pHDMIOut4Monitor[0]);
                if (status != NTV2_STATUS_SUCCESS)
                {
                    ntv2_hdmiout4_close(ntv2pp->m_pHDMIOut4Monitor[0]);
                    ntv2pp->m_pHDMIOut4Monitor[0] = NULL;
                }
            }
	    }
	
        if (ntv2pp->_DeviceID == DEVICE_ID_IP25_R ||
                                      ntv2pp->_DeviceID == DEVICE_ID_IP25_T)
        {
            ntv2pp->m_pRasterMonitor = ntv2_videoraster_open(&ntv2pp->systemContext, "ntv2videoraster", 0);
            if (ntv2pp->m_pRasterMonitor != NULL)
            {
                status = ntv2_videoraster_configure(ntv2pp->m_pRasterMonitor, 0x3400, 64, 4);
                if (status != NTV2_STATUS_SUCCESS)
                {
                    ntv2_videoraster_close(ntv2pp->m_pRasterMonitor);
                    ntv2pp->m_pRasterMonitor = NULL;
                }
            }

            ntv2pp->m_pSetupMonitor = ntv2_setup_open(&ntv2pp->systemContext, "ntv2setup");
            if (ntv2pp->m_pSetupMonitor != NULL)
            {
                status = ntv2_setup_configure(ntv2pp->m_pSetupMonitor);
                if (status != NTV2_STATUS_SUCCESS)
                {
                    ntv2_setup_close(ntv2pp->m_pSetupMonitor);
                    ntv2pp->m_pSetupMonitor = NULL;
                }
            }

            ntv2pp->m_pHDMIOut4Monitor[0] = ntv2_hdmiout4_open(&ntv2pp->systemContext, "ntv2hdmiout4", 1);
            if (ntv2pp->m_pHDMIOut4Monitor[0] != NULL)
            {
                status = ntv2_hdmiout4_configure(ntv2pp->m_pHDMIOut4Monitor[0]);
                if (status != NTV2_STATUS_SUCCESS)
                {
                    ntv2_hdmiout4_close(ntv2pp->m_pHDMIOut4Monitor[0]);
                    ntv2pp->m_pHDMIOut4Monitor[0] = NULL;
                }
            }

            ntv2pp->m_pHDMIOut4Monitor[1] = ntv2_hdmiout4_open(&ntv2pp->systemContext, "ntv2hdmiout4", 2);
            if (ntv2pp->m_pHDMIOut4Monitor[1] != NULL)
            {
                status = ntv2_hdmiout4_configure(ntv2pp->m_pHDMIOut4Monitor[1]);
                if (status != NTV2_STATUS_SUCCESS)
                {
                    ntv2_hdmiout4_close(ntv2pp->m_pHDMIOut4Monitor[1]);
                    ntv2pp->m_pHDMIOut4Monitor[1] = NULL;
                }
            }
        }

	    for (i = 0; i < NTV2_MAX_HDMI_MONITOR; i++)
	    {
		    if (ntv2pp->m_pHDMIInputMonitor[i] != NULL)
		    {
			    ntv2_hdmiin_enable(ntv2pp->m_pHDMIInputMonitor[i]);
		    }
		    if (ntv2pp->m_pHDMIIn4Monitor[i] != NULL)
		    {
			    ntv2_hdmiin4_enable(ntv2pp->m_pHDMIIn4Monitor[i]);
		    }		
		    if (ntv2pp->m_pHDMIOut4Monitor[i] != NULL)
		    {
			    ntv2_hdmiout4_enable(ntv2pp->m_pHDMIOut4Monitor[i]);
            }
	    }

        if (ntv2pp->m_pRasterMonitor != NULL)
        {
            ntv2_videoraster_enable(ntv2pp->m_pRasterMonitor);
        }
    
        if (ntv2pp->m_pSetupMonitor != NULL)
        {
            ntv2_setup_enable(ntv2pp->m_pSetupMonitor);
        }
        
        // configure tty uart
        ntv2pp->m_pSerialPort = NULL;

	    //Make sure the anc extractor is shut off
	    if (NTV2DeviceCanDoCustomAnc (ntv2pp->_DeviceID))
	    {
		    int numChannels = NTV2DeviceGetNumVideoChannels (ntv2pp->_DeviceID);
		    for(i = 0; i < numChannels; i++)
		    {
			    WriteRegister(deviceNumber, gChannelToAncExtOffset[i], 0, maskEnableHancY, shiftEnableHancY);
			    WriteRegister(deviceNumber, gChannelToAncExtOffset[i], 0, maskEnableHancC, shiftEnableHancC);
			    WriteRegister(deviceNumber, gChannelToAncExtOffset[i], 0, maskEnableVancY, shiftEnableVancY);
			    WriteRegister(deviceNumber, gChannelToAncExtOffset[i], 0, maskEnableVancC, shiftEnableVancC);
		    }
	    }

	    // Enable interrupts
        if (irqGood)
        {
            EnableAllInterrupts(deviceNumber);
        }

	    // Enable DMA
	    dmaEnable(deviceNumber);
    }

#if defined(AJA_CREATE_DEVICE_NODES)
    if (getNTV2ModuleParams()->class != NULL)
    {
        // Create the device node
        dev = MKDEV(getNTV2ModuleParams()->NTV2Major, deviceNumber);
        cdev_init(&NTV2Params[deviceNumber]->cdev, &ntv2_fops);
        ret = cdev_add(&NTV2Params[deviceNumber]->cdev, dev, 1);
        if (ret < 0)
        {
            MSG("%s: Failed to add cdev to subsystem\n", getNTV2ModuleParams()->name);
        }
        else
        {
            device = device_create(getNTV2ModuleParams()->class, NULL, dev,
                                   NULL, "ajantv2%d", deviceNumber);
            if (IS_ERR(device))
            {
                MSG("%s: Failed to create device node\n", getNTV2ModuleParams()->name);
            }
            else
            {
                MSG("%s: Created device node /dev/ajantv2%d\n",
                    getNTV2ModuleParams()->name, deviceNumber);
            }
        }
    }
#endif

fail:    
	MSG("%s: probe end\n", ntv2pp->name);

	return 0;
}

static void initializeRegisterNames(ULWord deviceNumber, unsigned long mappedAddress)
{
	NTV2PrivateParams *ntv2pp = getNTV2Params(deviceNumber);

	ntv2pp->_pGlobalControl = mappedAddress;
	ntv2pp->_pGlobalControl2 = (unsigned long)mappedAddress+0x42C;
	
	ntv2pp->_pVideoProcessingControl= (unsigned long)mappedAddress+0x24;
	ntv2pp->_pVideoProcessingCrossPointControl= (unsigned long)mappedAddress+0x28;
	
	ntv2pp->_pInterruptControl= (unsigned long)mappedAddress+0x50;
	ntv2pp->_pStatus= (unsigned long)mappedAddress+0x54;
	ntv2pp->_pInterruptControl2 = (unsigned long)mappedAddress+0x428;
	ntv2pp->_pStatus2 = (unsigned long)mappedAddress+0x424;

	ntv2pp->_pDMA1HostAddress = (unsigned long)mappedAddress+0x80;
	ntv2pp->_pDMA1LocalAddress = (unsigned long)mappedAddress+0x84;
	ntv2pp->_pDMA1TransferCount = (unsigned long)mappedAddress+0x88;
	ntv2pp->_pDMA1NextDescriptor = (unsigned long)mappedAddress+0x8C;
	ntv2pp->_pDMA2HostAddress = (unsigned long)mappedAddress+0x90;
	ntv2pp->_pDMA2LocalAddress = (unsigned long)mappedAddress+0x94;
	ntv2pp->_pDMA2TransferCount = (unsigned long)mappedAddress+0x98;
	ntv2pp->_pDMA2NextDescriptor = (unsigned long)mappedAddress+0x9C;
	ntv2pp->_pDMA3HostAddress = (unsigned long)mappedAddress+0xA0;
	ntv2pp->_pDMA3LocalAddress = (unsigned long)mappedAddress+0xA4;
	ntv2pp->_pDMA3TransferCount = (unsigned long)mappedAddress+0xA8;
	ntv2pp->_pDMA3NextDescriptor = (unsigned long)mappedAddress+0xAC;
	ntv2pp->_pDMA4HostAddress = (unsigned long)mappedAddress+0xB0;
	ntv2pp->_pDMA4LocalAddress = (unsigned long)mappedAddress+0xB4;
	ntv2pp->_pDMA4TransferCount = (unsigned long)mappedAddress+0xB8;
	ntv2pp->_pDMA4NextDescriptor = (unsigned long)mappedAddress+0xBC;
	ntv2pp->_pDMA1HostAddressHigh = (unsigned long)mappedAddress+0x190;
	ntv2pp->_pDMA1NextDescriptorHigh = (unsigned long)mappedAddress+0x194;
	ntv2pp->_pDMA2HostAddressHigh = (unsigned long)mappedAddress+0x198;
	ntv2pp->_pDMA2NextDescriptorHigh = (unsigned long)mappedAddress+0x19C;
	ntv2pp->_pDMA3HostAddressHigh = (unsigned long)mappedAddress+0x1A0;
	ntv2pp->_pDMA3NextDescriptorHigh = (unsigned long)mappedAddress+0x1A4;
	ntv2pp->_pDMA4HostAddressHigh = (unsigned long)mappedAddress+0x1A8;
	ntv2pp->_pDMA4NextDescriptorHigh = (unsigned long)mappedAddress+0x1AC;

	ntv2pp->_pAudioControl = (unsigned long)mappedAddress+0x60;
	ntv2pp->_pAudioSource = (unsigned long)mappedAddress+0x64;
	ntv2pp->_pAudioLastOut = (unsigned long)mappedAddress+0x68;
	ntv2pp->_pAudioLastIn = (unsigned long)mappedAddress+0x6C;
	
	ntv2pp->_pAudio2Control = (unsigned long)(mappedAddress+0x3c0);
	ntv2pp->_pAudio2Source = (unsigned long)(mappedAddress+0x3c4);
	ntv2pp->_pAudio2LastOut = (unsigned long)(mappedAddress+0x3c8);
	ntv2pp->_pAudio2LastIn = (unsigned long)(mappedAddress+0x3cC);

	ntv2pp->_pAudio3Control = (unsigned long)mappedAddress+0x458;
	ntv2pp->_pAudio3Source = (unsigned long)mappedAddress+0x460;
	ntv2pp->_pAudio3LastOut = (unsigned long)mappedAddress+0x46C;
	ntv2pp->_pAudio3LastIn = (unsigned long)mappedAddress+0x470;

	ntv2pp->_pAudio4Control = (unsigned long)mappedAddress+0x45C;
	ntv2pp->_pAudio4Source = (unsigned long)mappedAddress+0x464;
	ntv2pp->_pAudio4LastOut = (unsigned long)mappedAddress+0x474;
	ntv2pp->_pAudio4LastIn = (unsigned long)mappedAddress+0x478;

	ntv2pp->_pAudio5Control = (unsigned long)mappedAddress+0x6E0;
	ntv2pp->_pAudio5Source = (unsigned long)mappedAddress+0x6E4;
	ntv2pp->_pAudio5LastOut = (unsigned long)mappedAddress+0x6E8;
	ntv2pp->_pAudio5LastIn = (unsigned long)mappedAddress+0x6EC;

	ntv2pp->_pAudio6Control = (unsigned long)mappedAddress+0x6F0;
	ntv2pp->_pAudio6Source = (unsigned long)mappedAddress+0x6F4;
	ntv2pp->_pAudio6LastOut = (unsigned long)mappedAddress+0x6F8;
	ntv2pp->_pAudio6LastIn = (unsigned long)mappedAddress+0x6FC;

	ntv2pp->_pAudio7Control = (unsigned long)mappedAddress+0x700;
	ntv2pp->_pAudio7Source = (unsigned long)mappedAddress+0x704;
	ntv2pp->_pAudio7LastOut = (unsigned long)mappedAddress+0x708;
	ntv2pp->_pAudio7LastIn = (unsigned long)mappedAddress+0x70C;

	ntv2pp->_pAudio8Control = (unsigned long)mappedAddress+0x710;
	ntv2pp->_pAudio8Source = (unsigned long)mappedAddress+0x714;
	ntv2pp->_pAudio8LastOut = (unsigned long)mappedAddress+0x718;
	ntv2pp->_pAudio8LastIn = (unsigned long)mappedAddress+0x71C;

	ntv2pp->_pAudioSampleCounter = (unsigned long)mappedAddress+0x70;

	// P2P
	ntv2pp->_pFrameApertureOffset = (unsigned long)mappedAddress+0x308;
	ntv2pp->_pMessageChannel1 = (unsigned long)mappedAddress+0x310;
	ntv2pp->_pMessageChannel2 = (unsigned long)mappedAddress+0x314;
	ntv2pp->_pMessageChannel3 = (unsigned long)mappedAddress+0x318;
	ntv2pp->_pMessageChannel4 = (unsigned long)mappedAddress+0x31C;
	ntv2pp->_pMessageChannel5 = (unsigned long)mappedAddress+0x31C;
	ntv2pp->_pMessageChannel6 = (unsigned long)mappedAddress+0x31C;
	ntv2pp->_pMessageChannel7 = (unsigned long)mappedAddress+0x31C;
	ntv2pp->_pMessageChannel8 = (unsigned long)mappedAddress+0x31C;

	ntv2pp->_pPhysicalMessageChannel1 = (unsigned long)ntv2pp->_unmappedBAR1Address+0x310;
	ntv2pp->_pPhysicalMessageChannel2 = (unsigned long)ntv2pp->_unmappedBAR1Address+0x314;
	ntv2pp->_pPhysicalMessageChannel3 = (unsigned long)ntv2pp->_unmappedBAR1Address+0x318;
	ntv2pp->_pPhysicalMessageChannel4 = (unsigned long)ntv2pp->_unmappedBAR1Address+0x31c;
	ntv2pp->_pPhysicalMessageChannel5 = (unsigned long)ntv2pp->_unmappedBAR1Address+0x31c;
	ntv2pp->_pPhysicalMessageChannel6 = (unsigned long)ntv2pp->_unmappedBAR1Address+0x31c;
	ntv2pp->_pPhysicalMessageChannel7 = (unsigned long)ntv2pp->_unmappedBAR1Address+0x31c;
	ntv2pp->_pPhysicalMessageChannel8 = (unsigned long)ntv2pp->_unmappedBAR1Address+0x31c;

	ntv2pp->_pMessageInterruptStatus  = (unsigned long)mappedAddress+0x320;
	ntv2pp->_pMessageInterruptControl = (unsigned long)mappedAddress+0x324;

	ntv2pp->_pPhysicalOutputChannel1 = (unsigned long)ntv2pp->_unmappedBAR1Address+0xC;
	ntv2pp->_pPhysicalOutputChannel2 = (unsigned long)ntv2pp->_unmappedBAR1Address+0x1C;
	ntv2pp->_pPhysicalOutputChannel3 = (unsigned long)ntv2pp->_unmappedBAR1Address+0x408;
	ntv2pp->_pPhysicalOutputChannel4 = (unsigned long)ntv2pp->_unmappedBAR1Address+0x414;
	ntv2pp->_pPhysicalOutputChannel5 = (unsigned long)ntv2pp->_unmappedBAR1Address+0x604;
	ntv2pp->_pPhysicalOutputChannel6 = (unsigned long)ntv2pp->_unmappedBAR1Address+0x614;
	ntv2pp->_pPhysicalOutputChannel7 = (unsigned long)ntv2pp->_unmappedBAR1Address+0x624;
	ntv2pp->_pPhysicalOutputChannel8 = (unsigned long)ntv2pp->_unmappedBAR1Address+0x634;

	ntv2pp->_pDMAInterruptControl = (unsigned long)mappedAddress+0xC4;
	ntv2pp->_pDMAControlStatus = (unsigned long)mappedAddress+0xC0;

	ntv2pp->_pDeviceID = (unsigned long)mappedAddress+0xC8;

#ifdef SOFTWARE_UART_FIFO
#ifdef UARTTXFIFOSIZE
	ntv2pp->_pUARTTransmitData = (unsigned long)mappedAddress+0x118;
	ntv2pp->_pUARTTransmitData2 = (unsigned long)mappedAddress+0x3D0;
#endif
#ifdef UARTRXFIFOSIZE
	ntv2pp->_pUARTReceiveData = (unsigned long)mappedAddress+0x11C;
	ntv2pp->_pUARTReceiveData2 = (unsigned long)mappedAddress+0x3D4;
#endif
	ntv2pp->_pUARTControl = (unsigned long)mappedAddress+0x120;
	ntv2pp->_pUARTControl2 = (unsigned long)mappedAddress+0x3D8;
#endif	// SOFTWARE_UART_FIFO
}

static void SetupBoard(ULWord deviceNumber)
{
	NTV2PrivateParams *ntv2pp = getNTV2Params(deviceNumber);
	int i = 0;
	Ntv2SystemContext systemContext;

    memset(&systemContext, 0, sizeof(Ntv2SystemContext));
	systemContext.devNum = deviceNumber;

	// Disable Xena's machine control UART and flush the FIFOs
	Init422Uart(deviceNumber);

	ClearSingleLED(deviceNumber, 3);
	ClearSingleLED(deviceNumber, 2);
	ClearSingleLED(deviceNumber, 1);

	// Setup the LUTs
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
	if ( NTV2DeviceCanDoColorCorrection( getNTV2Params(deviceNumber)->_DeviceID ) )
	{
		switch( NTV2DeviceGetNumLUTs( getNTV2Params(deviceNumber)->_DeviceID ) )
		{
		case 8:
			DownloadLinearLUTToHW (&systemContext, NTV2_CHANNEL8, 0);
			DownloadLinearLUTToHW (&systemContext, NTV2_CHANNEL8, 1);
			// fall through
		case 7:
			DownloadLinearLUTToHW (&systemContext, NTV2_CHANNEL7, 0);
			DownloadLinearLUTToHW (&systemContext, NTV2_CHANNEL7, 1);
			// fall through
		case 6:
			DownloadLinearLUTToHW (&systemContext, NTV2_CHANNEL6, 0);
			DownloadLinearLUTToHW (&systemContext, NTV2_CHANNEL6, 1);
			// fall through
		case 5:
			DownloadLinearLUTToHW (&systemContext, NTV2_CHANNEL5, 0);
			DownloadLinearLUTToHW (&systemContext, NTV2_CHANNEL5, 1);
			// fall through
		case 4:
			DownloadLinearLUTToHW (&systemContext, NTV2_CHANNEL4, 0);
			DownloadLinearLUTToHW (&systemContext, NTV2_CHANNEL4, 1);
			// fall through
		case 3:
			DownloadLinearLUTToHW (&systemContext, NTV2_CHANNEL3, 0);
			DownloadLinearLUTToHW (&systemContext, NTV2_CHANNEL3, 1);
			// fall through
		case 2:
			DownloadLinearLUTToHW (&systemContext, NTV2_CHANNEL2, 0);
			DownloadLinearLUTToHW (&systemContext, NTV2_CHANNEL2, 1);
			// fall through
		case 1:
			DownloadLinearLUTToHW (&systemContext, NTV2_CHANNEL1, 0);
			DownloadLinearLUTToHW (&systemContext, NTV2_CHANNEL1, 1);
			break;
		default:
			break;
		}
	}
#pragma GCC diagnostic pop

	switch( NTV2DeviceGetHDMIVersion(ntv2pp->_DeviceID) )
	{
	case 1:
		WriteRegister(deviceNumber, kRegHDMIOutControl, 0x00000400, NO_MASK, NO_SHIFT);
		msleep(200);
		WriteRegister(deviceNumber, kRegHDMIOutControl, 0x0c000400, NO_MASK, NO_SHIFT);
		msleep(200);
		WriteRegister(deviceNumber, kRegHDMIOutControl, 0x0d000400, NO_MASK, NO_SHIFT);
		msleep(200);
		WriteRegister(deviceNumber, kRegHDMIOutControl, 0x0c000400, NO_MASK, NO_SHIFT);
		break;
	case 2:
	case 3:
		{
			ULWord resetMask = BIT(24)+BIT(25)+BIT(26)+BIT(27);

			WriteRegister(deviceNumber, kRegHDMIOutControl, 0x0, BIT_25, 25);
			WriteRegister(deviceNumber, kRegHDMIOutControl, 0x0, BIT_24, 24);

			WriteRegister(deviceNumber, kRegHDMIOutControl, 0x0, resetMask, 24);
			msleep(200);
			WriteRegister(deviceNumber, kRegHDMIOutControl, 0xc, resetMask, 24);
		}
		break;
	default:
		break;
	}

	// Set default register clocking to match the video standard
	if( IsProgressiveStandard( &systemContext, NTV2_CHANNEL1 ) )
	{
		SetRegisterWriteMode( deviceNumber, NTV2_CHANNEL1, NTV2_REGWRITE_SYNCTOFIELD );
	}
	else
	{
		SetRegisterWriteMode( deviceNumber, NTV2_CHANNEL1, NTV2_REGWRITE_SYNCTOFRAME );
	}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
	if( NTV2DeviceCanDoMultiFormat( ntv2pp->_DeviceID ) )
	{
		switch( NTV2DeviceGetNumVideoChannels( ntv2pp->_DeviceID ) )
		{
		case 8:
			if( IsProgressiveStandard( &systemContext, NTV2_CHANNEL8 ) )
			{
				SetRegisterWriteMode( deviceNumber, NTV2_CHANNEL8, NTV2_REGWRITE_SYNCTOFIELD );
			}
			else
			{
				SetRegisterWriteMode( deviceNumber, NTV2_CHANNEL8, NTV2_REGWRITE_SYNCTOFRAME );
			}
			// Fall through
		case 7:
			if( IsProgressiveStandard( &systemContext, NTV2_CHANNEL7 ) )
			{
				SetRegisterWriteMode( deviceNumber, NTV2_CHANNEL7, NTV2_REGWRITE_SYNCTOFIELD );
			}
			else
			{
				SetRegisterWriteMode( deviceNumber, NTV2_CHANNEL7, NTV2_REGWRITE_SYNCTOFRAME );
			}
			// Fall through
		case 6:
			if( IsProgressiveStandard( &systemContext, NTV2_CHANNEL6 ) )
			{
				SetRegisterWriteMode( deviceNumber, NTV2_CHANNEL6, NTV2_REGWRITE_SYNCTOFIELD );
			}
			else
			{
				SetRegisterWriteMode( deviceNumber, NTV2_CHANNEL6, NTV2_REGWRITE_SYNCTOFRAME );
			}
			// Fall through
		case 5:
			if( IsProgressiveStandard( &systemContext, NTV2_CHANNEL5 ) )
			{
				SetRegisterWriteMode( deviceNumber, NTV2_CHANNEL5, NTV2_REGWRITE_SYNCTOFIELD );
			}
			else
			{
				SetRegisterWriteMode( deviceNumber, NTV2_CHANNEL5, NTV2_REGWRITE_SYNCTOFRAME );
			}
			// Fall through
		case 4:
			if( IsProgressiveStandard( &systemContext, NTV2_CHANNEL4 ) )
			{
				SetRegisterWriteMode( deviceNumber, NTV2_CHANNEL4, NTV2_REGWRITE_SYNCTOFIELD );
			}
			else
			{
				SetRegisterWriteMode( deviceNumber, NTV2_CHANNEL4, NTV2_REGWRITE_SYNCTOFRAME );
			}
			// Fall through
		case 3:
			if( IsProgressiveStandard( &systemContext, NTV2_CHANNEL3 ) )
			{
				SetRegisterWriteMode( deviceNumber, NTV2_CHANNEL3, NTV2_REGWRITE_SYNCTOFIELD );
			}
			else
			{
				SetRegisterWriteMode( deviceNumber, NTV2_CHANNEL3, NTV2_REGWRITE_SYNCTOFRAME );
			}
			// Fall through
		case 2:
			if( IsProgressiveStandard( &systemContext, NTV2_CHANNEL2 ) )
			{
				SetRegisterWriteMode( deviceNumber, NTV2_CHANNEL2, NTV2_REGWRITE_SYNCTOFIELD );
			}
			else
			{
				SetRegisterWriteMode( deviceNumber, NTV2_CHANNEL2, NTV2_REGWRITE_SYNCTOFRAME );
			}
			break;
		default:
			break;
		}
	}
#pragma GCC diagnostic pop

	//	Make an educated guess about the video formats of the channels
	for( i = 0; i < NTV2DeviceGetNumVideoChannels( ntv2pp->_DeviceID ); i++)
	{
		NTV2Standard standard = GetStandard(&systemContext, i);
		NTV2FrameRate frameRate = GetFrameRate(&systemContext, i);
		NTV2FrameGeometry frameGeometry = GetFrameGeometry(&systemContext, i);
		ULWord smpte372Enabled = GetSmpte372(&systemContext, i) ? 1 : 0;
		NTV2VideoFormat videoFormat = NTV2_FORMAT_UNKNOWN;

		NTV2DeviceGetVideoFormatFromState (&videoFormat,
											frameRate,
											frameGeometry,
											standard,
											smpte372Enabled);

        WriteRegister(deviceNumber, kVRegVideoFormatCh1+i, videoFormat, NO_MASK, NO_SHIFT);
    }

    if (NTV2DeviceGetNumVideoChannels(ntv2pp->_DeviceID) > 4)
        WriteRegister(deviceNumber, kRegGlobalControl2, 0, kRegMaskRefSource2, kRegShiftRefSource2);
    WriteRegister(deviceNumber, kRegGlobalControl, NTV2_REFERENCE_FREERUN, kRegMaskRefSource, kRegShiftRefSource);

    // wait for memory to configure
    {
        int waitCount = 10000;
        for( i = 0; i < waitCount; i++)
        {
            if (ReadRegister(deviceNumber, kRegDMAControl, BIT(24), 24) != 0)
                break;
        }
        if (i == waitCount)
            MSG("Wait for memory initialization timeout\n");
    }
}

int ValidateAjaNTV2Message(NTV2_HEADER * pHeaderIn)
{
//#define LOG_VALIDATE_ERRORS

#ifdef LOG_VALIDATE_ERRORS
	NTV2_TRAILER * pTrailerIn = NULL;
#endif

	//	Validation & sanity checks...
	if (!NTV2_IS_VALID_HEADER_TAG(pHeaderIn->fHeaderTag))
	{
#ifdef LOG_VALIDATE_ERRORS
		MSG("Bad NTV2_HEADER tag\n");
#endif
		return -EINVAL;
	}
	if (!NTV2_IS_VALID_STRUCT_TYPE(pHeaderIn->fType))
	{
#ifdef LOG_VALIDATE_ERRORS
		MSG("Bad or unknown NTV2 struct type\n");
#endif
		return -EINVAL;
	}
	if (pHeaderIn->fHeaderVersion != NTV2_CURRENT_HEADER_VERSION)
	{
#ifdef LOG_VALIDATE_ERRORS
		MSG("Bad or unsupported NTV2 header version\n");
#endif
		return -EINVAL;
	}
	if (pHeaderIn->fResultStatus)
	{
#ifdef LOG_VALIDATE_ERRORS
		MSG("fResultStatus non-zero\n");
#endif
		return -EINVAL;
	}
	if (pHeaderIn->fSizeInBytes < (sizeof(NTV2_HEADER) + sizeof(NTV2_TRAILER)))
	{
#ifdef LOG_VALIDATE_ERRORS
		MSG("Struct size unexpectedly small\n");
#endif
		return -EINVAL;
	}
	if (pHeaderIn->fPointerSize != 8)
	{
#ifdef LOG_VALIDATE_ERRORS
		MSG("Host pointer size not 8\n");
#endif
		return -EINVAL;
	}

#if 0
	//	Calculate where the NTV2_TRAILER should be, and verify it's there...
	pTrailerIn = (NTV2_TRAILER *)((UByte *)pHeaderIn + pHeaderIn->fSizeInBytes - sizeof(NTV2_TRAILER));
	if (!NTV2_IS_VALID_TRAILER_TAG(pTrailerIn->fTrailerTag))
	{
#ifdef LOG_VALIDATE_ERRORS
		MSG("Bad NTV2_TRAILER tag\n");
#endif
		return -EINVAL;
	}
#endif

#ifdef LOG_VALIDATE_ERRORS
		MSG("Validate ok\n");
#endif

	return 0;
}

int DoMessageSDIInStatictics(ULWord deviceNumber, NTV2Buffer * pInStatistics, void * pOutBuff)
{
	INTERNAL_SDI_STATUS_STRUCT internalSDIStruct;
	Ntv2SystemContext systemContext;
	int returnCode = 0;
	NTV2SDIInputStatus * pSDIInputStatus = NULL;
	NTV2PrivateParams * pNTV2Params = getNTV2Params(deviceNumber);

    memset(&systemContext, 0, sizeof(Ntv2SystemContext));
    systemContext.devNum = pNTV2Params->deviceNumber;

	CopySDIStatusHardwareToFrameStampSDIStatusArray(&systemContext, &internalSDIStruct);
	pSDIInputStatus = (NTV2SDIInputStatus *) pOutBuff;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
	switch(pInStatistics->fByteCount / sizeof(NTV2SDIInputStatus))
	{
	case 8:
		pSDIInputStatus[NTV2_CHANNEL8] = internalSDIStruct.SDIStatus8;
		// fall through
	case 7:
		pSDIInputStatus[NTV2_CHANNEL7] = internalSDIStruct.SDIStatus7;
		// fall through
	case 6:
		pSDIInputStatus[NTV2_CHANNEL6] = internalSDIStruct.SDIStatus6;
		// fall through
	case 5:
		pSDIInputStatus[NTV2_CHANNEL5] = internalSDIStruct.SDIStatus5;
		// fall through
	case 4:
		pSDIInputStatus[NTV2_CHANNEL4] = internalSDIStruct.SDIStatus4;
		// fall through
	case 3:
		pSDIInputStatus[NTV2_CHANNEL3] = internalSDIStruct.SDIStatus3;
		// fall through
	case 2:
		pSDIInputStatus[NTV2_CHANNEL2] = internalSDIStruct.SDIStatus2;
		// fall through
	case 1:
		pSDIInputStatus[NTV2_CHANNEL1] = internalSDIStruct.SDIStatus1;
	break;
	default:
		returnCode = -EINVAL;
		break;
	}
#pragma GCC diagnostic pop

	return returnCode;
}

int DoMessageBankAndRegisterWrite(ULWord deviceNumber, NTV2RegInfo * pInReg, NTV2RegInfo * pInBank)
{
	NTV2PrivateParams * pNTV2Params = getNTV2Params(deviceNumber);
	unsigned long flags;

	spin_lock_irqsave (&pNTV2Params->_bankAndRegisterAccessLock, flags);
	WriteRegister (deviceNumber, pInBank->registerNumber,	pInBank->registerValue,	pInBank->registerMask,	pInBank->registerShift);
	///NOTE: These two reads are a kludge.....still need to figure out why needed....without them you get undesired results
	ReadRegister(deviceNumber, pInBank->registerNumber,		pInBank->registerMask,	pInBank->registerShift);
	ReadRegister(deviceNumber, pInBank->registerNumber,		pInBank->registerMask,	pInBank->registerShift);
	WriteRegister (deviceNumber, pInReg->registerNumber,	pInReg->registerValue,	pInReg->registerMask,	pInReg->registerShift);
	spin_unlock_irqrestore (&pNTV2Params->_bankAndRegisterAccessLock, flags);

	return 0;
}


int DoMessageBankAndRegisterRead(ULWord deviceNumber, NTV2RegInfo * pInReg, NTV2RegInfo * pInBank)
{
	NTV2PrivateParams * pNTV2Params = getNTV2Params(deviceNumber);
	unsigned long	flags;

	spin_lock_irqsave (&pNTV2Params->_bankAndRegisterAccessLock, flags);
	WriteRegister (deviceNumber, pInBank->registerNumber,	pInBank->registerValue,	pInBank->registerMask,	pInBank->registerShift);
	///NOTE: These two reads are a kludge.....still need to figure out why needed....without them you get undesired results
	ReadRegister (deviceNumber, pInBank->registerNumber,		pInBank->registerMask,	pInBank->registerShift);
	ReadRegister (deviceNumber, pInBank->registerNumber,		pInBank->registerMask,	pInBank->registerShift);
	pInReg->registerValue = ReadRegister (deviceNumber, pInReg->registerNumber, pInReg->registerMask, pInReg->registerShift);
	spin_unlock_irqrestore (&pNTV2Params->_bankAndRegisterAccessLock, flags);

	return 0;
}


int DoMessageAutoCircFrame(ULWord deviceNumber, FRAME_STAMP * pInOutFrameStamp, NTV2_RP188 * pOutTimecodeArray)
{
	return AutoCirculateFrameStampImmediate(deviceNumber, pInOutFrameStamp, pOutTimecodeArray);
}


int DoMessageBufferLock(ULWord deviceNumber, PDMA_PAGE_ROOT pRoot, NTV2BufferLock* pBufferLock)
{
	if ((pRoot == NULL) || (pBufferLock == NULL))
		return -EINVAL;

	if ((pBufferLock->mFlags & DMABUFFERLOCK_LOCK) != 0)
	{
//		MSG("%s: lock flags %x\n", getNTV2ModuleParams()->name, pBufferLock->mFlags);
		return dmaPageRootAdd(deviceNumber, pRoot,
							  (PVOID)pBufferLock->mBuffer.fUserSpacePtr,
							  pBufferLock->mBuffer.fByteCount,
//							  ((pBufferLock->mFlags & DMABUFFERLOCK_RDMA) != 0),
							  ((pBufferLock->mFlags & DMABUFFERLOCK_MAP) != 0));
	}

	if ((pBufferLock->mFlags & DMABUFFERLOCK_UNLOCK) != 0)
	{
//		MSG("%s: unlock flags %x\n", getNTV2ModuleParams()->name, pBufferLock->mFlags);
		return dmaPageRootRemove(deviceNumber, pRoot,
								 (PVOID)pBufferLock->mBuffer.fUserSpacePtr,
								 pBufferLock->mBuffer.fByteCount);
	}

	if ((pBufferLock->mFlags & DMABUFFERLOCK_UNLOCK_ALL) != 0)
	{
//		MSG("%s: unlock all release root\n", getNTV2ModuleParams()->name);
		dmaPageRootRelease(deviceNumber, pRoot);
		return 0;
	}

	if ((pBufferLock->mFlags & DMABUFFERLOCK_AUTO) != 0)
	{
//		MSG("%s: auto lock flags %x\n", getNTV2ModuleParams()->name, pBufferLock->mFlags);
		dmaPageRootAuto(deviceNumber, pRoot,
						true,
						((pBufferLock->mFlags & DMABUFFERLOCK_MAP) != 0),
						pBufferLock->mMaxLockSize);
	}

	if ((pBufferLock->mFlags & DMABUFFERLOCK_MANUAL) != 0)
	{
//		MSG("%s: manual lock flags %x\n", getNTV2ModuleParams()->name, pBufferLock->mFlags);
		dmaPageRootAuto(deviceNumber, pRoot, false, false, 0);
	}

	return -EINVAL;
}


static int platform_resources_config(ULWord deviceNumber)
{
	NTV2PrivateParams *ntv2pp = NULL;
	struct platform_device* pd = NULL;
    struct device_node *node = NULL;
	uint32_t val32;
	int ret = 0;

	if ( !(ntv2pp = getNTV2Params(deviceNumber)) )
		return -ENODEV;

	pd = ntv2pp->platform_dev;

	// initialize mapping
	ntv2pp->_mappedBAR0Address = 0;
	ntv2pp->_unmappedBAR0Address = 0;
	ntv2pp->_BAR0MemorySize = 0;

	ntv2pp->_mappedBAR1Address = 0;
	ntv2pp->_unmappedBAR1Address = 0;
	ntv2pp->_BAR1MemorySize = 0;

	ntv2pp->_mappedBAR2Address = 0;
	ntv2pp->_unmappedBAR2Address = 0;
	ntv2pp->_BAR2MemorySize = 0;
    
    ret = of_property_read_u32(pd->dev.of_node, "deviceType", &val32);
    if(ret) {
        MSG("%s: failed to read deviceType property\n", ntv2pp->name);
        goto fail;
    }
    ntv2pp->deviceType = val32;

    ret = of_property_read_u32(pd->dev.of_node, "deviceID", &val32);
    if(ret) {
        MSG("%s: failed to read deviceID property\n", ntv2pp->name);
        goto fail;
    }
    ntv2pp->pci_device = val32;

    ret = of_property_read_u32(pd->dev.of_node, "vendorID", &val32);
    if(ret) {
        MSG("%s: failed to read vendorID property\n", ntv2pp->name);
        goto fail;
    }
    ntv2pp->pci_vendor = val32;
	ntv2pp->busNumber = 0xffffffff;
	
    MSG("%s: device type 0x%02x (%04x:%04x)\n",
		ntv2pp->name, ntv2pp->deviceType, ntv2pp->pci_vendor, ntv2pp->pci_device);
    ntv2pp->legacyInterrupt = of_property_read_bool(pd->dev.of_node, "legacyInterrupt");
    if(ntv2pp->legacyInterrupt)
		MSG("%s: using legacy interrupt\n", ntv2pp->name);

    // hack the registers
#if 0
    {
        unsigned long phyaddr = 0x00000000a8000000;
        unsigned long size = 0x0000000000040000;
        void *mapped = NULL;
        struct resource *resource = NULL;
        
        resource = request_mem_region(phyaddr, size, ntv2pp->name);
        if(resource == NULL)
        {
            MSG("%s: %s request_mem_region failed\n", ntv2pp->name, node->full_name);
            if(ret) goto fail;
        }
        mapped = ioremap(phyaddr, size);
        MSG("%s: video reg %p phyaddr\n", ntv2pp->name, (void*)phyaddr);
        MSG("%s: video reg %p mapped\n", ntv2pp->name, mapped);

        ntv2pp->_unmappedBAR0Address = phyaddr;
        ntv2pp->_mappedBAR0Address = (unsigned long)mapped;
        ntv2pp->_BAR0MemorySize = size;

        ntv2pp->_FrameMemoryAddress = 0x500000000;
        ntv2pp->_FrameMemorySize    = 0x40000000;
        MSG("%s: frame buffer addr 0x%lx  size 0x%zx bytes\n",
            ntv2pp->name, ntv2pp->_FrameMemoryAddress, ntv2pp->_FrameMemorySize);
    }
#endif    

    // Walk through the children nodes and setup the device
    for_each_child_of_node(pd->dev.of_node, node)
	{
        if(!strcmp("reg", node->name))
		{
			MSG("%s: found registers\n", ntv2pp->name);
			ret = platform_add_register(deviceNumber, node);
            if(ret)
				goto fail;
        }
        if(!strcmp("mem", node->name))
		{
			MSG("%s: found fb memory\n", ntv2pp->name);
			ret = platform_add_fb_memory(deviceNumber, node);
            if(ret)
				goto fail;
        }
#if 0        
        if(!strcmp("fs", node->name))
		{
			MSG("%s: found fs memory\n", ntv2pp->name);
			ret = platform_add_fs_memory(deviceNumber, node);
            if(ret)
				goto fail;
        }
#endif        
        if(!strcmp("fs1", node->name))
		{
			MSG("%s: found fs memory\n", ntv2pp->name);
			ret = platform_add_fs_memory(deviceNumber, node);
            if(ret)
				goto fail;
        }
#if 0
        if(!strcmp("fs2", node->name))
		{
			MSG("%s: found fs memory\n", ntv2pp->name);
			ret = platform_add_fs_memory(deviceNumber, node);
            if(ret)
				goto fail;
        }
#endif
		if(!strcmp("irq", node->name))
		{
			MSG("%s: found interrupt\n", ntv2pp->name);
			ret = platform_add_interrupt(deviceNumber, node);
            if(ret)
                goto fail;
        }
    }

	ntv2pp->_dmaMethod = DmaMethodZynq;
	ntv2pp->_dmaSerialize = true;

	return ret;

fail:
	return ret;
}

static int platform_resources_release(ULWord deviceNumber)
{
	NTV2PrivateParams *ntv2pp = NULL;

	if ( !(ntv2pp = getNTV2Params(deviceNumber)) )
		return -ENODEV;

	platform_remove_register(deviceNumber);
	platform_remove_memory(deviceNumber);

	return 0;
}

static int platform_add_register(ULWord deviceNumber, struct device_node *node)
{
	NTV2PrivateParams *ntv2pp = NULL;
    struct device_node *handle = of_parse_phandle(node, "node", 0);

	if ( !(ntv2pp = getNTV2Params(deviceNumber)) )
		return -ENODEV;

    if (ntv2pp->_BAR0MemorySize != 0)
	{
        MSG("%s: cannot add register %s\n", ntv2pp->name, node->full_name);
        return -ENOMEM;
    }
	
    if (handle != NULL)
	{
        if (handle->name != NULL)
		{
            uint32_t values[4];
            int ret;
            unsigned long phyaddr;
			unsigned long size;
            void *mapped = NULL;
            struct resource *resource = NULL;

            MSG("%s: %s is %s\n", ntv2pp->name, node->full_name, handle->name);
            ret = of_property_read_u32_array(handle, "reg", values, 4);
            if(ret)
			{
                MSG("%s: %s failed to read reg\n", ntv2pp->name, node->full_name);
                return -EINVAL;
            }
            phyaddr = values[0];
			phyaddr <<= 32;
			phyaddr |= values[1];
            size = values[2];
			size <<= 32;
			size |= values[3];

            MSG("%s: video reg (%s) addr 0x%08lx  size 0x%lx bytes\n",
                     ntv2pp->name, node->full_name, phyaddr, size);

            resource = request_mem_region(phyaddr, size, ntv2pp->name);
            if(resource == NULL)
			{
				MSG("%s: %s request_mem_region failed\n", ntv2pp->name, node->full_name);
                return -ENOMEM;
            }
            mapped = ioremap(phyaddr, size);
            MSG("%s: video reg %p mapped\n", ntv2pp->name, mapped);

			ntv2pp->_unmappedBAR0Address = phyaddr;
			ntv2pp->_mappedBAR0Address = (unsigned long)mapped;
			ntv2pp->_BAR0MemorySize = size;
			
            return 0;
        }
        
        MSG("%s: found register node no name\n", ntv2pp->name);
    }
	
    MSG("%s: node %s does not define a register block\n", ntv2pp->name, node->full_name);
    return -EINVAL;
}

static int platform_remove_register(ULWord deviceNumber)
{
	NTV2PrivateParams *ntv2pp = NULL;

	if ( !(ntv2pp = getNTV2Params(deviceNumber)) )
		return -ENODEV;

    if (ntv2pp->_BAR0MemorySize != 0)
	{
		iounmap((void*)ntv2pp->_mappedBAR0Address);
		release_mem_region(ntv2pp->_unmappedBAR0Address, ntv2pp->_BAR0MemorySize);
		MSG("%s: reg %p unmapped\n", ntv2pp->name, (void*)ntv2pp->_mappedBAR0Address);
    }

	ntv2pp->_unmappedBAR0Address = 0;
	ntv2pp->_mappedBAR0Address = 0;
	ntv2pp->_BAR0MemorySize = 0;

	return 0;
}

static int platform_add_fb_memory(ULWord deviceNumber, struct device_node *node)
{
	NTV2PrivateParams *ntv2pp = NULL;
    struct device_node *handle = of_parse_phandle(node, "node", 0);

	if ( !(ntv2pp = getNTV2Params(deviceNumber)) )
		return -ENODEV;

    if (ntv2pp->_FrameMemorySize != 0)
	{
        MSG("%s: cannot add fb memory %s\n", ntv2pp->name, node->full_name);
        return -ENOMEM;
    }
	
    if (handle != NULL)
	{
        if (handle->name != NULL)
		{
            uint32_t values[4];
            int ret;
            unsigned long phyaddr;
			unsigned long size;

            MSG("%s: %s is %s\n", ntv2pp->name, node->full_name, handle->name);
            ret = of_property_read_u32_array(handle, "reg", values, 4);
            if(ret)
			{
                MSG("%s: %s failed to read reg\n", ntv2pp->name, node->full_name);
                return -EINVAL;
            }
            phyaddr = values[0];
			phyaddr <<= 32;
			phyaddr |= values[1];
            size = values[2];
			size <<= 32;
			size |= values[3];

            MSG("%s: frame buffer memory (%s) addr 0x%016lx  size 0x%lx bytes\n",
                     ntv2pp->name, node->full_name, phyaddr, size);
#if 0
            void *mapped = NULL;
            struct resource *resource = NULL;
            resource = request_mem_region(phyaddr, size, ntv2pp->name);
            if(resource == NULL)
			{
				MSG("%s: %s request_mem_region failed\n", ntv2pp->name, node->full_name);
                return -ENOMEM;
            }
            mapped = ioremap(phyaddr, size);
            MSG("%s: video reg %p mapped\n", ntv2pp->name, mapped);
#endif
			ntv2pp->_FrameMemoryAddress = phyaddr;
			ntv2pp->_FrameMemorySize = size;
			
            return 0;
        }

        MSG("%s: found memory node no name\n", ntv2pp->name);
    }
	
    MSG("%s: node %s does not define a memory block\n", ntv2pp->name, node->full_name);
    return -EINVAL;
}

static int platform_add_fs_memory(ULWord deviceNumber, struct device_node *node)
{
	NTV2PrivateParams *ntv2pp = NULL;
    struct device_node *handle = of_parse_phandle(node, "node", 0);

	if ( !(ntv2pp = getNTV2Params(deviceNumber)) )
		return -ENODEV;

    if (ntv2pp->_FsMemorySize != 0)
	{
        MSG("%s: cannot add fs memory %s\n", ntv2pp->name, node->full_name);
        return -ENOMEM;
    }
	
    if (handle != NULL)
	{
        if (handle->name != NULL)
		{
            uint32_t values[4];
            int ret;
            unsigned long phyaddr;
			unsigned long size;

            MSG("%s: %s is %s\n", ntv2pp->name, node->full_name, handle->name);
            ret = of_property_read_u32_array(handle, "reg", values, 4);
            if(ret)
			{
                MSG("%s: %s failed to read reg\n", ntv2pp->name, node->full_name);
                return -EINVAL;
            }
            phyaddr = values[0];
			phyaddr <<= 32;
			phyaddr |= values[1];
            size = values[2];
			size <<= 32;
			size |= values[3];

            MSG("%s: frame sync memory (%s) addr 0x%016lx  size 0x%lx bytes\n",
                     ntv2pp->name, node->full_name, phyaddr, size);
#if 0
            void *mapped = NULL;
            struct resource *resource = NULL;
            resource = request_mem_region(phyaddr, size, ntv2pp->name);
            if(resource == NULL)
			{
				MSG("%s: %s request_mem_region failed\n", ntv2pp->name, node->full_name);
                return -ENOMEM;
            }
            mapped = ioremap(phyaddr, size);
            MSG("%s: video reg %p mapped\n", ntv2pp->name, mapped);
#endif
			ntv2pp->_FsMemoryAddress = phyaddr;
			ntv2pp->_FsMemorySize = size;
			
            return 0;
        }

        MSG("%s: found memory node no name\n", ntv2pp->name);
    }
	
    MSG("%s: node %s does not define a memory block\n", ntv2pp->name, node->full_name);
    return -EINVAL;
}

static int platform_remove_memory(ULWord deviceNumber)
{
	NTV2PrivateParams *ntv2pp = NULL;

	if ( !(ntv2pp = getNTV2Params(deviceNumber)) )
		return -ENODEV;
#if 0
    if (ntv2pp->_FrameMemorySize != 0)
	{
		iounmap((void*)ntv2pp->_mappedBAR0Address);
		release_mem_region(ntv2pp->_unmappedBAR0Address, ntv2pp->_BAR0MemorySize);
		MSG("%s: reg %p unmapped\n", ntv2pp->name, (void*)ntv2pp->_mappedBAR0Address);
    }
#endif
    ntv2pp->_FrameMemoryAddress = 0;
    ntv2pp->_FrameMemorySize = 0;

	return 0;
}

static int dev_node_match(struct device *dev, const void *data)
{
    return dev->of_node == data;
}

static int platform_add_interrupt(ULWord deviceNumber, struct device_node *node)
{
 	NTV2PrivateParams *ntv2pp = NULL;
    struct device_node *handle = of_parse_phandle(node, "node", 0);

	if ( !(ntv2pp = getNTV2Params(deviceNumber)) )
		return -ENODEV;

    if(ntv2pp->_ntv2IRQ[eIrqFpga])
	{
        MSG("%s: interrupt already %d\n", ntv2pp->name, ntv2pp->_ntv2IRQ[eIrqFpga]);
        return -EINVAL;
    }
	
    if(handle != NULL)
	{
        if(handle->name != NULL)
		{
            int ret;
            struct device *dev;
            struct platform_device *pd;
			
            dev = bus_find_device(&platform_bus_type, NULL, handle, dev_node_match);
            if(dev == NULL)
			{
                MSG("%s: %s failed to find device\n", ntv2pp->name, node->full_name);
                return -EINVAL;
            }
			
            pd = to_platform_device(dev);
            if(pd == NULL)
			{
                MSG("%s: %s failed to find platform_device\n", ntv2pp->name, node->full_name);
                return -EINVAL;
            }

            ret = platform_get_irq(pd, 0);
            if(ret < 1)
			{
                MSG("%s: %s failed to get interrupt: %d\n", ntv2pp->name, node->full_name, ret);
                return ret;
            }
			
            ntv2pp->_ntv2IRQ[eIrqFpga] = ret;
            MSG("%s: interrupt %d from %s (%s)\n",
				ntv2pp->name, ntv2pp->_ntv2IRQ[eIrqFpga], node->full_name, handle->name);
            return 0;
        }
        MSG("%s: found interrupt node no name\n", ntv2pp->name);
    }
	
    MSG("%s: node %s does not define an interrupt\n", ntv2pp->name, node->full_name);
    return -EINVAL;
}

static VirtualDataNode *findVirtualDataNode(ULWord tag)
{
    VirtualDataNode *node = gVirtualDataHead;
    // walk list looking for matching tag and return node
    while (node != NULL)
    {
        if (node->tag == tag)
            return node;
        node = node->next;
    }
    return NULL;
}

// add a new node to the head of the list
static int insertVirtualDataNode(ULWord tag, UByte *buf, ULWord size)
{
    UByte *bp;
    // allocate size of node plus additional data
    VirtualDataNode *node = (VirtualDataNode *)
            kmalloc(sizeof(VirtualDataNode)+size-sizeof(ULWord), GFP_KERNEL);
    if (node == NULL)
        return -ENOMEM;
    // initialize members and copy data
    node->tag = tag;
    node->size = size;
    bp = (UByte *)&node->data;
    while (size--)
        *bp++ = *buf++;
    // link at head of the list
    node->prev = NULL;
    node->next = gVirtualDataHead;
    // if list wasn't empty, change first item to point back at new node
    if (gVirtualDataHead != NULL)
        gVirtualDataHead->prev = node;
    gVirtualDataHead = node;
    return 0;
}

void deleteVirtualDataNode(VirtualDataNode *node)
{
    // check for bad node
    if (node == NULL)
        return;
    // check for empty list
    if (gVirtualDataHead == NULL)
        return;
    // handle special case of deleting first item in list
    if (gVirtualDataHead == node)
    {
        // if there are additional nodes
        if (node->next != NULL)
        {
            node->next->prev = NULL;
        }
        gVirtualDataHead = node->next;
        kfree(node);
    }
    // handle deleting item not at head
    else
    {
        // not first, so can use prev pointer
        node->prev->next = node->next;
        // if it isn't the last item, can use next
        if (node->next != NULL)
        {
            node->next->prev = node->prev;
        }
        kfree(node);
    }
}

static void deleteAllVirtualDataNodes(void)
{
    while(gVirtualDataHead != NULL)
        deleteVirtualDataNode(gVirtualDataHead);
}

static int readVirtualData(ULWord tag, UByte *buf, ULWord size)
{
    // see if we have one in list
    VirtualDataNode *node = findVirtualDataNode(tag);
    UByte *bp;
    // if no node or size doesn't match, return error
    if (node == NULL || size != node->size)
    {
        return -EINVAL;
    }
    // copy data from node
    bp = (UByte *)&node->data;
    while (size--)
        *buf++ = *bp++;
    return 0;
}

static int writeVirtualData(ULWord tag, UByte *buf, ULWord size)
{
    VirtualDataNode *node = findVirtualDataNode(tag);
    if (node != NULL)
    {
        // if the new size is zero and the node exists, just delete it and
        // return. This is how nodes get deleted.
        if (node->size == 0 )
        {
            deleteVirtualDataNode(node);
            return 0;
        }
        // if size is same, just copy over existing data
        if (node->size == size)
        {
            UByte *bp = (UByte *)&node->data;
            while (size--)
                *bp++ = *buf++;
            return 0;
        }
        // otherwise, delete the existing one and insert a new one
        deleteVirtualDataNode(node);
    }
    // make sure we have some data
    if (size > 0)
    {
        return insertVirtualDataNode(tag, buf, size);
    }
    return 0;
}

static void suspend(ULWord deviceNumber)
{
	NTV2PrivateParams *ntv2pp;
	int j;

    if ( !(ntv2pp = getNTV2Params(deviceNumber)) )
		return;

	MSG("%s: device suspend\n", ntv2pp->name);

	// disable hdmi monitor
	for (j = 0; j < NTV2_MAX_HDMI_MONITOR; j++)
	{
		if (ntv2pp->m_pHDMIInputMonitor[j] != NULL)
		{
			ntv2_hdmiin_disable(ntv2pp->m_pHDMIInputMonitor[j]);
		}
		if (ntv2pp->m_pHDMIIn4Monitor[j] != NULL)
		{
			ntv2_hdmiin4_disable(ntv2pp->m_pHDMIIn4Monitor[j]);
		}
		if (ntv2pp->m_pHDMIOut4Monitor[j] != NULL)
		{
			ntv2_hdmiout4_disable(ntv2pp->m_pHDMIOut4Monitor[j]);
		}	
	}

	if (ntv2pp->m_pSetupMonitor != NULL)
	{
		ntv2_setup_disable(ntv2pp->m_pSetupMonitor);
	}

    if (ntv2pp->m_pRasterMonitor != NULL)
    {
        ntv2_videoraster_disable(ntv2pp->m_pRasterMonitor);
    }
	
    if (ntv2pp->m_pGenlock2Monitor != NULL)
    {
        ntv2_genlock2_disable(ntv2pp->m_pGenlock2Monitor);
    }

	// disable the serial driver
	if (ntv2pp->m_pSerialPort)
	{
		ntv2pp->serialActive = ntv2_serial_active(ntv2pp->m_pSerialPort);
		ntv2_serial_disable(ntv2pp->m_pSerialPort);
	}
		
	// disable all dma engines
	dmaDisable(deviceNumber);

	// disable interrupts,
	DisableAllInterrupts(deviceNumber);

	// disable register access
	ntv2pp->registerEnable = false;
}

static void resume(ULWord deviceNumber)
{
	NTV2PrivateParams *ntv2pp;
	int intrIndex;
	char versionString[STRMAX];
	int i;

    if ( !(ntv2pp = getNTV2Params(deviceNumber)) )
		return;

	MSG("%s: device resume\n", ntv2pp->name);

	// enable register access
	ntv2pp->registerEnable = true;

	// Initialize wait queues and interrupt bookkeeping
	for (intrIndex = 0; intrIndex < eNumInterruptTypes; intrIndex++)
	{
		ntv2pp->_interruptCount[intrIndex] = 0;
		ntv2pp->_interruptHappened[intrIndex] = 0;
	}

	ntv2pp->_DeviceID = ReadDeviceIDRegister(deviceNumber);
	ntv2pp->_numberOfHWRegisters = NTV2DeviceGetMaxRegisterNumber(ntv2pp->_DeviceID);

	if (NTV2DeviceIsSupported(ntv2pp->_DeviceID))
	{
		MSG("%s: board id 0x%x\n", ntv2pp->name, (ULWord)ntv2pp->_DeviceID);
	}
	else
	{
		MSG("%s: board id 0x%x not supported!\n", ntv2pp->name, (ULWord)ntv2pp->_DeviceID);
	}

    if (getNTV2ModuleParams()->driverMode == eDriverModeAll)
    {
	    // configure board defaults
	    SetupBoard(deviceNumber);

	    getDeviceVersionString(deviceNumber, versionString, STRMAX);
	    MSG("%s: detected device %s\n", ntv2pp->name, versionString);
	    getDeviceSerialNumberString(deviceNumber, versionString, STRMAX);
	    MSG("%s: serial number %s\n", ntv2pp->name, versionString);
	    getPCIFPGAVersionString(deviceNumber, versionString, STRMAX);
	    MSG("%s: firmware version %s\n", ntv2pp->name, versionString);

	    //Make sure the anc extractor is shut off
	    if (NTV2DeviceCanDoCustomAnc (ntv2pp->_DeviceID))
	    {
		    int numChannels = NTV2DeviceGetNumVideoChannels (ntv2pp->_DeviceID);
		    for(i = 0; i < numChannels; i++)
		    {
			    WriteRegister(deviceNumber, gChannelToAncExtOffset[i], 0, maskEnableHancY, shiftEnableHancY);
			    WriteRegister(deviceNumber, gChannelToAncExtOffset[i], 0, maskEnableHancC, shiftEnableHancC);
			    WriteRegister(deviceNumber, gChannelToAncExtOffset[i], 0, maskEnableVancY, shiftEnableVancY);
			    WriteRegister(deviceNumber, gChannelToAncExtOffset[i], 0, maskEnableVancC, shiftEnableVancC);
		    }
	    }

	    // enable the serial driver
	    if (ntv2pp->m_pSerialPort && ntv2pp->serialActive)
	    {
		    ntv2_serial_enable(ntv2pp->m_pSerialPort);
	    }
		
	    // enable hdmi monitor
	    for (i = 0; i < NTV2_MAX_HDMI_MONITOR; i++)
	    {
		    if (ntv2pp->m_pHDMIInputMonitor[i] != NULL)
		    {
			    ntv2_hdmiin_enable(ntv2pp->m_pHDMIInputMonitor[i]);
		    }
		    if (ntv2pp->m_pHDMIIn4Monitor[i] != NULL)
		    {
			    ntv2_hdmiin4_enable(ntv2pp->m_pHDMIIn4Monitor[i]);
		    }
		    if (ntv2pp->m_pHDMIOut4Monitor[i] != NULL)
		    {
			    ntv2_hdmiout4_enable(ntv2pp->m_pHDMIOut4Monitor[i]);
		    }	
        }
	
        if (ntv2pp->m_pSetupMonitor != NULL)
        {
            ntv2_setup_enable(ntv2pp->m_pSetupMonitor);
        }

        if (ntv2pp->m_pRasterMonitor != NULL)
        {
            ntv2_videoraster_enable(ntv2pp->m_pRasterMonitor);
        }

        if (ntv2pp->m_pGenlock2Monitor != NULL)
        {
            ntv2_genlock2_enable(ntv2pp->m_pGenlock2Monitor);
        }

	    // Enable interrupts
        if (ntv2pp->canDoInterrupt)
        {
            EnableAllInterrupts(deviceNumber);
        }

	    // enable all dma engines
	    dmaEnable(deviceNumber);
    }
}

module_init(aja_ntv2_module_init);
module_exit(aja_ntv2_module_cleanup);

