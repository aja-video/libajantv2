/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
//////////////////////////////////////////////////////////////
//
// HEVC Linux Device Driver for AJA HEVC boards.
//
// Boards supported include:
//
// OEM-HEVC
//
// Filename: hevcdriver.c
// Purpose:	 Main module file.  Load, unload, fops, ioctls.
//
///////////////////////////////////////////////////////////////

#include "hevcdriver.h"
#include "hevcpublic.h"
#include "hevcparams.h"
#include "hevcinterrupt.h"

#if defined (MSWindows)

#elif defined (AJAMac)

// tbd

#elif defined (AJALinux)

// pci bar configuration and release
static int hevcPciConfigure(uint32_t devNum);
static int hevcPciRelease(uint32_t devNum);

// pci interrupt configuration and release
static int hevcIrqConfigure(uint32_t devNum);
static int hevcIrqRelease(uint32_t devNum);
static irqreturn_t hevcIrqInterrupt(int irq, void* dev_id);

// hevc driver module intialization
int hevc_module_init(const char* pModuleName)
{
	HEVC_MSG_INFO("%s: initialize hevc module\n", pModuleName);

	// initialize the module and device parameters
	hevcParamsInitialize(pModuleName);

	{
		// initialize the debug output mask
		HevcModuleParams* pModParams = hevcGetModuleParams();
		pModParams->debugMask =
			HEVC_DEBUG_MASK_INFO |
			HEVC_DEBUG_MASK_WARNING |
			HEVC_DEBUG_MASK_ERROR |
			HEVC_DEBUG_MASK_INT_ERROR |
			HEVC_DEBUG_MASK_REGISTER_ERROR |
			HEVC_DEBUG_MASK_COMMAND_ERROR |
			HEVC_DEBUG_MASK_STREAM_ERROR |
			HEVC_DEBUG_MASK_MEMORY_ERROR |
			HEVC_DEBUG_MASK_DMA_ERROR;
	}	

	return 0;
}

int hevc_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	HevcModuleParams* pModParams = hevcGetModuleParams();
	uint32_t devNum = hevcGetMaxDevices();
	HevcDeviceParams* pDevParams = NULL;
	const struct pci_bus *pbus = pdev->bus;
	int result;

	// enable the hevc device
	result = pci_enable_device(pdev);
	if(result < 0)
	{
		HEVC_MSG_ERROR("%s: *error* hevc_probe can not enable hevc device\n", pModParams->pModuleName);
		return -EPERM;
	}

	// get a new device number
	devNum = hevcNewDevice();
	if(devNum >= hevcGetMaxDevices())
	{
		HEVC_MSG_ERROR("%s: *error* hevc_probe only %d hevc devices supported\n",
					   pModParams->pModuleName, hevcGetMaxDevices());
		return -EPERM;
	}

	// get the device parameter pointer and save the system pointer
	pDevParams = hevcGetDeviceParams(devNum);
	pDevParams->systemContext.devNum = devNum;
	pDevParams->systemContext.pDevice = pdev;
	pDevParams->pciId.vendor = id->vendor & 0x0000ffff;
	pDevParams->pciId.device = id->device & 0x0000ffff;
	pDevParams->pciId.subVendor = id->subvendor & 0x0000ffff;
	pDevParams->pciId.subDevice = id->subdevice & 0x0000ffff;

	if (pbus != NULL)
	{
		pDevParams->systemContext.busNumber = (uint32_t)pbus->number;
	}
	else
	{
		pDevParams->systemContext.busNumber = 0xffffffff;
	}

	// determine device mode
	pDevParams->deviceMode = Hevc_DeviceMode_Codec;
	if((pDevParams->pciId.subVendor == 0) ||
	   (pDevParams->pciId.subDevice == 0))
	{
		pDevParams->deviceMode = Hevc_DeviceMode_Maintenance;
	}

	// configure the pci bars and dma
	result = hevcPciConfigure(devNum);
	if(result < 0) return result;

	pci_set_drvdata(pdev, pDevParams);

	// initialize device
	hevcDeviceOpen(devNum);

	if(pDevParams->deviceMode == Hevc_DeviceMode_Codec)
	{
		// enable interrupts
		result = hevcIrqConfigure(devNum);
		if(result < 0) return result;
	}

	HEVC_MSG_INFO("%s(%d): hevc pci bus number %d\n", pModParams->pModuleName, devNum, pDevParams->systemContext.busNumber);

	HEVC_MSG_INFO("%s(%d): hevc device initialization complete\n", pModParams->pModuleName, devNum);

	return 0;
}

void hevc_module_cleanup(void)
{
	HevcModuleParams* pModParams = hevcGetModuleParams();
	uint32_t devNum;

	HEVC_MSG_INFO("%s: cleanup hevc module\n", pModParams->pModuleName);

	// cleanup devices
	for(devNum = 0; devNum < HEVC_DEVICE_MAX; devNum++)
	{
		HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
		if((pDevParams != NULL) && (pDevParams->systemContext.pDevice != NULL))
		{
			if(pDevParams->deviceMode == Hevc_DeviceMode_Codec)
			{
				hevcIrqRelease(devNum);
			}

			hevcDeviceClose(devNum);
			hevcPciRelease(devNum);
			pci_disable_device(pDevParams->systemContext.pDevice);
			pDevParams->systemContext.pDevice = NULL;
		}
	}

	// cleanup module
	hevcParamsRelease();
}

int hevc_reboot_handler(struct notifier_block *this, unsigned long code, void *x)
{
	return NOTIFY_DONE;
}

static int hevcPciConfigure(uint32_t devNum)
{
	HevcModuleParams* pModParams = hevcGetModuleParams();
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	struct pci_dev* pDevice;
	struct resource* resource = NULL;
	int result;

	if(pDevParams == NULL) return -EPERM;
	pDevice = pDevParams->systemContext.pDevice;

	HEVC_MSG_INFO("%s(%d): configure PCI resources\n", pModParams->pModuleName, devNum);

	// map bar 0
	resource = request_mem_region(pci_resource_start(pDevice, FMB_PCI_BAR0),
								  pci_resource_len(pDevice, FMB_PCI_BAR0),
								  pModParams->pModuleName);
	if(resource == NULL)
	{
		HEVC_MSG_ERROR("%s(%d): *error* pci bar0 request_mem_region failed\n", pModParams->pModuleName, devNum);
		return -EPERM;
	}

	pDevParams->bar0Base = ioremap(pci_resource_start(pDevice, FMB_PCI_BAR0),
								   pci_resource_len(pDevice, FMB_PCI_BAR0));
	if(pDevParams->bar0Base == 0)
	{
		HEVC_MSG_ERROR("%s(%d): *error* pci bar0 ioremap failed\n", pModParams->pModuleName, devNum);
		release_mem_region(pci_resource_start(pDevice, FMB_PCI_BAR0),
						   pci_resource_len(pDevice, FMB_PCI_BAR0));
		return -EPERM;
	}

	pDevParams->bar0Size = pci_resource_len(pDevice, FMB_PCI_BAR0);

	// map bar 2
	resource = request_mem_region(pci_resource_start(pDevice, FMB_PCI_BAR2),
								  pci_resource_len(pDevice, FMB_PCI_BAR2),
								  pModParams->pModuleName);
	if(resource == NULL)
	{
		HEVC_MSG_ERROR("%s(%d): *error* pci bar2 request_mem_region failed\n", pModParams->pModuleName, devNum);
		return -EPERM;
	}

	pDevParams->bar2Base = ioremap(pci_resource_start(pDevice, FMB_PCI_BAR2),
								   pci_resource_len(pDevice, FMB_PCI_BAR2));
	if(pDevParams->bar2Base == NULL)
	{
		HEVC_MSG_ERROR("%s(%d): *error* pci bar2 ioremap failed\n", pModParams->pModuleName, devNum);
		release_mem_region(pci_resource_start(pDevice, FMB_PCI_BAR2),
						   pci_resource_len(pDevice, FMB_PCI_BAR2));
		return -EPERM;
	}

	pDevParams->bar2Size = pci_resource_len(pDevice, FMB_PCI_BAR2);

	// map bar 4
	resource = request_mem_region(pci_resource_start(pDevice, FMB_PCI_BAR4),
								  pci_resource_len(pDevice, FMB_PCI_BAR4),
								  pModParams->pModuleName);
	if(resource == NULL)
	{
		HEVC_MSG_ERROR("%s(%d): *error* pci bar4 request_mem_region failed\n", pModParams->pModuleName, devNum);
		return -EPERM;
	}

	pDevParams->bar4Base = ioremap(pci_resource_start(pDevice, FMB_PCI_BAR4),
								   pci_resource_len(pDevice, FMB_PCI_BAR4));
	if(pDevParams->bar4Base == NULL)
	{
		HEVC_MSG_ERROR("%s(%d): *error* pci bar4 ioremap failed\n", pModParams->pModuleName, devNum);
		release_mem_region(pci_resource_start(pDevice, FMB_PCI_BAR4),
						   pci_resource_len(pDevice, FMB_PCI_BAR4));
		return -EPERM;
	}

	pDevParams->bar4Size = pci_resource_len(pDevice, FMB_PCI_BAR4);

	if(pDevParams->deviceMode == Hevc_DeviceMode_Codec)
	{
		// map bar 5
		resource = request_mem_region(pci_resource_start(pDevice, FMB_PCI_BAR5),
									  pci_resource_len(pDevice, FMB_PCI_BAR5),
									  pModParams->pModuleName);
		if(resource != NULL)
		{
			pDevParams->bar5Base = ioremap(pci_resource_start(pDevice, FMB_PCI_BAR5),
										   pci_resource_len(pDevice, FMB_PCI_BAR5));
			if(pDevParams->bar5Base != NULL)
			{
				pDevParams->bar5Size = pci_resource_len(pDevice, FMB_PCI_BAR5);
			}
			else
			{
				HEVC_MSG_WARNING("%s(%d): *warning* pci bar5 ioremap failed\n", pModParams->pModuleName, devNum);
				release_mem_region(pci_resource_start(pDevice, FMB_PCI_BAR5),
								   pci_resource_len(pDevice, FMB_PCI_BAR5));
				pDevParams->deviceMode = Hevc_DeviceMode_Maintenance;
			}
		}
		else
		{
			HEVC_MSG_WARNING("%s(%d): *warning* pci bar5 request_mem_region failed\n", pModParams->pModuleName, devNum);
			pDevParams->deviceMode = Hevc_DeviceMode_Maintenance;
		}
	}

	HEVC_MSG_INFO("%s(%d): map pci bar0  phys 0x%08x  address 0x%px  size 0x%08x\n", pModParams->pModuleName, devNum,
				  (uint32_t)pci_resource_start(pDevice, FMB_PCI_BAR0), pDevParams->bar0Base, 
				  (uint32_t)pci_resource_len(pDevice, FMB_PCI_BAR0));
	HEVC_MSG_INFO("%s(%d): map pci bar2  phys 0x%08x  address 0x%px  size 0x%08x\n", pModParams->pModuleName, devNum,
				  (uint32_t)pci_resource_start(pDevice, FMB_PCI_BAR2), pDevParams->bar2Base, 
				  (uint32_t)pci_resource_len(pDevice, FMB_PCI_BAR2));
	HEVC_MSG_INFO("%s(%d): map pci bar4  phys 0x%08x  address 0x%px  size 0x%08x\n", pModParams->pModuleName, devNum,
				  (uint32_t)pci_resource_start(pDevice, FMB_PCI_BAR4), pDevParams->bar4Base, 
				  (uint32_t)pci_resource_len(pDevice, FMB_PCI_BAR4));
	HEVC_MSG_INFO("%s(%d): map pci bar5  phys 0x%08x  address 0x%px  size 0x%08x\n", pModParams->pModuleName, devNum,
				  (uint32_t)pci_resource_start(pDevice, FMB_PCI_BAR5), pDevParams->bar5Base, 
				  (uint32_t)pci_resource_len(pDevice, FMB_PCI_BAR5));

	if(pDevParams->deviceMode == Hevc_DeviceMode_Codec)
	{
		// we are a pci dma master
		pci_set_master(pDevice);

		result = pci_set_dma_mask(pDevice, DMA_BIT_MASK(64));
		if(result == 0)
		{
			result = pci_set_consistent_dma_mask(pDevice, DMA_BIT_MASK(64));
			if(result != 0)
			{
				HEVC_MSG_ERROR("%s(%d): could not set consistent dma mask to 64 bit (%08x)\n",
							   pModParams->pModuleName, devNum, result);
				return result;
			}

			HEVC_MSG_INFO("%s(%d): pci dma mask = 64 bit\n", pModParams->pModuleName, devNum);
		}
		else
		{
			result = pci_set_dma_mask(pDevice, DMA_BIT_MASK(32));
			if(result != 0)
			{
				HEVC_MSG_ERROR("%s(%d): could not set pci dma mask to 32 bit (%08x)\n",
							   pModParams->pModuleName, devNum, result);
				return result;
			}

			result = pci_set_consistent_dma_mask(pDevice, DMA_BIT_MASK(32));
			if(result != 0)
			{
				HEVC_MSG_ERROR("%s(%d): could not set consistent dma mask to 32 bit (%08x)\n",
							   pModParams->pModuleName, devNum, result);
				return result;
			}

			HEVC_MSG_INFO("%s(%d): pci dma mask = 32 bit\n", pModParams->pModuleName, devNum);
		}
	}

	return 0;
}

static int hevcPciRelease(uint32_t devNum)
{
	HevcModuleParams* pModParams = hevcGetModuleParams();
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	struct pci_dev* pDevice;

	if(pDevParams == NULL) return -EPERM;
	pDevice = pDevParams->systemContext.pDevice;

	HEVC_MSG_INFO("%s(%d): release hevc device resources\n", pModParams->pModuleName, devNum);

	// unmap bar 0
	if(pDevParams->bar0Base != NULL)
	{
		HEVC_MSG_INFO("%s(%d): unmap pci bar0 address 0x%px\n", pModParams->pModuleName, devNum, pDevParams->bar0Base);
		iounmap(pDevParams->bar0Base);
		pDevParams->bar0Base = NULL;
		release_mem_region(pci_resource_start(pDevice, FMB_PCI_BAR0),
						   pci_resource_len(pDevice, FMB_PCI_BAR0));
	}

	// unmap bar 2
	if(pDevParams->bar2Base != NULL)
	{
		HEVC_MSG_INFO("%s(%d): unmap pci bar2 address 0x%px\n", pModParams->pModuleName, devNum, pDevParams->bar2Base);
		iounmap(pDevParams->bar2Base);
		pDevParams->bar2Base = NULL;
		release_mem_region(pci_resource_start(pDevice, FMB_PCI_BAR2),
						   pci_resource_len(pDevice, FMB_PCI_BAR2));
	}

	// unmap bar 4
	if(pDevParams->bar4Base != NULL)
	{
		HEVC_MSG_INFO("%s(%d): unmap pci bar4 address 0x%px\n", pModParams->pModuleName, devNum, pDevParams->bar4Base);
		iounmap(pDevParams->bar4Base);
		pDevParams->bar4Base = NULL;
		release_mem_region(pci_resource_start(pDevice, FMB_PCI_BAR4),
						   pci_resource_len(pDevice, FMB_PCI_BAR4));
	}

	// unmap bar 5
	if(pDevParams->bar5Base != NULL)
	{
		HEVC_MSG_INFO("%s(%d): unmap pci bar5 address 0x%px\n", pModParams->pModuleName, devNum, pDevParams->bar5Base);
		iounmap(pDevParams->bar5Base);
		pDevParams->bar5Base = NULL;
		release_mem_region(pci_resource_start(pDevice, FMB_PCI_BAR5),
						   pci_resource_len(pDevice, FMB_PCI_BAR5));
	}

	return 0;
}

static int hevcIrqConfigure(uint32_t devNum)
{
	HevcModuleParams* pModParams = hevcGetModuleParams();
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	struct pci_dev* pDevice;
	int result = 0;

	if(pDevParams == NULL) return -EPERM;
	pDevice = pDevParams->systemContext.pDevice;

	// codec is only msi
	result = pci_enable_msi(pDevice);
	if(result != 0)
	{
		HEVC_MSG_ERROR("%s(%d): *error* can not enable msi irq\n", pModParams->pModuleName, devNum);
		return -EPERM;
	}

	// connect interrupt routine to irq
	result = request_irq(pDevice->irq, hevcIrqInterrupt, 0, pDevParams->pDeviceName, (void*)pDevParams);
	if(result != 0)
	{
		HEVC_MSG_ERROR("%s(%d): *error* request irq %d failed\n", pModParams->pModuleName, devNum, pDevice->irq);
		return -EPERM;
	}

	HEVC_MSG_INFO("%s(%d): pci msi irq %d\n", pModParams->pModuleName, devNum, pDevice->irq);

	return 0;
}

static int hevcIrqRelease(uint32_t devNum)
{
	HevcModuleParams* pModParams = hevcGetModuleParams();
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	struct pci_dev* pDevice;

	if(pDevParams == NULL) return -EPERM;
	pDevice = pDevParams->systemContext.pDevice;

	HEVC_MSG_INFO("%s(%d): release hevc irq (%d) resources\n", pModParams->pModuleName, devNum, pDevice->irq);

	// disable irqs
	free_irq(pDevice->irq, (void*)pDevParams);

	// restore irq
	pci_disable_msi(pDevice);

	return 0;
}

static irqreturn_t hevcIrqInterrupt(int irq, void* dev_id)
{
	HevcDeviceParams* pDevParams = (HevcDeviceParams*)dev_id;
	bool success;

	if(pDevParams == NULL) return IRQ_NONE;

	success = hevcInterrupt(pDevParams->devNum);
	if(!success) return IRQ_NONE;

	return IRQ_HANDLED;
}

#endif

