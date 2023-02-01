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
// Filename: hevcregister.c
// Purpose:	 HEVC register access functions
//
///////////////////////////////////////////////////////////////

#include "hevcregister.h"

// determine which pci bar the register is in
static enum fmb_pci_bar_type hevcGetBarType(uint32_t devNum, uint32_t address);

// get the system address of a register
static void hevcSetRemapAddress(uint32_t devNum, uint32_t regAddress,
								enum fmb_pci_bar_type barType, Ntv2Register* pPCIAddress);

// reset the bar 4 register window
static void hevcResetRemapAddress(uint32_t devNum);

// reset all continuity counters
static void hevcResetContinuity(uint32_t devNum);

// increment the specified continuity counter
static uint32_t hevcUpdateContinuityCount(uint32_t devNum, enum fmb_pci_continuity_cnt_type cntType);

// does address use the high bits
static bool hevcIs64bitAddress(Ntv2DmaAddress address);

// read firmware array
static void readFirmwareArray(uint32_t devNum, uint32_t* pArray, uint32_t arReg, uint32_t arSize);

// read firmware string
static void readFirmwareString(uint32_t devNum, char* pString, uint32_t strReg, uint32_t strSize);

// convert unsigned 32 bit integer to byte array
static void copyU32ToU8Array(uint32_t src, uint8_t* pDest);

// cleanup string
static void cleanString(char* pString, uint32_t strSize);

// is good character
static bool isGoodChar(char ch);


void hevcRegisterInitialize(uint32_t devNum)
{
	HevcModuleParams* pModParams = hevcGetModuleParams();
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	int i;

	if(pDevParams == NULL) return;

	HEVC_MSG_REGISTER_INFO("%s(%d): hevcRegisterInitialize()\n",
						   pModParams->pModuleName, devNum);

	// initialize the register access lock
	ntv2InterruptLockOpen(&pDevParams->regAccessLock, &pDevParams->systemContext);

	// initialize the continuity locks
	for(i = 0; i < FMB_PCI_CONT_TYPE_MAX; i++)
	{
		ntv2SpinLockOpen(&pDevParams->regContinuityLock[i], &pDevParams->systemContext);
	}

	// reset bar 4 remap
	hevcResetRemapAddress(devNum);

	// reset the continuity counts
	hevcResetContinuity(devNum);
}

void hevcRegisterRelease(uint32_t devNum)
{
	HevcModuleParams* pModParams = hevcGetModuleParams();
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	int i;

	// close the continuity locks
	for(i = 0; i < FMB_PCI_CONT_TYPE_MAX; i++)
	{
		ntv2SpinLockClose(&pDevParams->regContinuityLock[i]);
	}

	// close the register access lock
	ntv2InterruptLockClose(&pDevParams->regAccessLock);

	HEVC_MSG_REGISTER_INFO("%s(%d): hevcRegisterRelease()\n",
						   pModParams->pModuleName, devNum);
}

bool hevcIsDeviceAlive(uint32_t devNum)
{
	uint32_t mainState;

	// read main state and verify
	mainState = hevcRegRead(devNum, FMB_REG_MAIN_STATE);
	if((mainState != FMB_MAIN_STATE_BOOT) &&
	   (mainState != FMB_MAIN_STATE_INIT) &&
	   (mainState != FMB_MAIN_STATE_ENCODE)) return false;

	return true;
}

static enum fmb_pci_bar_type hevcGetBarType(uint32_t devNum, uint32_t address)
{
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	enum fmb_pci_bar_type barType = FMB_PCI_BAR4;

	// check address against bar ranges
	if((address >= FMB_PCI_BAR0_ADDR) && (address < (FMB_PCI_BAR0_ADDR + pDevParams->bar0Size)))
	{
		barType = FMB_PCI_BAR0;
	}
	else if((address >= FMB_PCI_BAR2_ADDR) && (address < (FMB_PCI_BAR2_ADDR + pDevParams->bar2Size)))
	{
		barType = FMB_PCI_BAR2;
	}
	else if((address >= FMB_PCI_BAR5_ADDR) && (address < (FMB_PCI_BAR5_ADDR + pDevParams->bar5Size)))
	{
		barType = FMB_PCI_BAR5;
	}
	else
	{
		barType = FMB_PCI_BAR4;
	}

	return barType;
}

static void hevcSetRemapAddress(uint32_t devNum, uint32_t regAddress,
								enum fmb_pci_bar_type barType, Ntv2Register* pPCIAddress)
{
	HevcModuleParams* pModParams = hevcGetModuleParams();
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	int i;
	uint32_t bar4RemapAddress;
	uint32_t curRemapAddress;
	uint32_t bar0Mask = pDevParams->bar0Size - 1;
	uint32_t bar2Mask = pDevParams->bar2Size - 1;
	uint32_t bar4Mask = pDevParams->bar4Size - 1;
	uint32_t bar5Mask = pDevParams->bar5Size - 1;
	Ntv2Register remapRegAddress;

	if(pPCIAddress == NULL) return;
	*pPCIAddress = NULL;

	// translate register address to system address
	switch(barType)
	{
	case FMB_PCI_BAR0:
		if(pDevParams->bar0Base != NULL)
		{
			*pPCIAddress = pDevParams->bar0Base + (regAddress & bar0Mask);
		}
		break;

	case FMB_PCI_BAR2:
		if(pDevParams->bar2Base != NULL)
		{
			*pPCIAddress = pDevParams->bar2Base + (regAddress & bar2Mask);
		}
		break;

	case FMB_PCI_BAR4:
		if(pDevParams->bar4Base != NULL)
		{
			*pPCIAddress = pDevParams->bar4Base + (regAddress & bar4Mask);

			// compute the bar4 window address
			bar4RemapAddress = (regAddress & ~bar4Mask);
			if(pDevParams->bar5Base != NULL)
			{
				remapRegAddress = (pDevParams->bar5Base + FMB_PCI_REG_AXI_MASTER_REMAP_ADDR_OFFSET);
			}
			else
			{
				remapRegAddress = (pDevParams->bar0Base + FMB_PCI_REG_AXI_MASTER_REMAP_ADDR_OFFSET);
			}

			// read current remap address
			curRemapAddress = ntv2ReadRegister32(remapRegAddress);

			// check if remap update required
			if(bar4RemapAddress != curRemapAddress)
			{
				HEVC_MSG_REGISTER_STATE("%s(%d): hevcSetRemapAddress()  BAR4 remap address 0x%08x for address 0x%08x\n",
										pModParams->pModuleName, devNum, bar4RemapAddress, regAddress);

				// set the remap
				ntv2WriteRegister32(remapRegAddress, bar4RemapAddress);

				// wait until the write is complete
				for(i = 0; i < FMB_PCI_POLL_COUNT_REG_WRITE_COMPLETE; i++)
				{
					if(ntv2ReadRegister32(remapRegAddress) == bar4RemapAddress) break;
				}
		
				if(i >= FMB_PCI_POLL_COUNT_REG_WRITE_COMPLETE)
				{
					HEVC_MSG_REGISTER_ERROR("%s(%d): hevcSetRemapAddress() *error*  remap address write timeout\n",
											pModParams->pModuleName, devNum);
				}
			}
		}
		break;

	case FMB_PCI_BAR5:
		if(pDevParams->bar5Base != NULL)
		{
			*pPCIAddress = pDevParams->bar5Base + (regAddress & bar5Mask);
		}
		break;

	default:
		HEVC_MSG_REGISTER_ERROR("%s(%d): hevcSetRemapAddress() *error*   bad pci bar type %d\n",
								pModParams->pModuleName, devNum, barType);
		break;
	}

	return;
}

static void hevcResetRemapAddress(uint32_t devNum)
{
	HevcModuleParams* pModParams = hevcGetModuleParams();
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	int i;
	uint32_t bar4Mask = pDevParams->bar4Size - 1;
	uint32_t bar4RemapAddress;
	Ntv2Register remapRegAddress;

	// compute default remap address
	bar4RemapAddress = (FMB_PCI_BAR4_ADDR & ~bar4Mask);
	if(pDevParams->bar5Base != NULL)
	{
		remapRegAddress = (pDevParams->bar5Base + FMB_PCI_REG_AXI_MASTER_REMAP_ADDR_OFFSET);
	}
	else
	{
		remapRegAddress = (pDevParams->bar0Base + FMB_PCI_REG_AXI_MASTER_REMAP_ADDR_OFFSET);
	}

	HEVC_MSG_REGISTER_STATE("%s(%d): hevcResetRemapAddress()  BAR4 remap address reset to 0x%08x\n",
							pModParams->pModuleName, devNum, bar4RemapAddress);

	// restore the default remap address
	ntv2WriteRegister32(remapRegAddress, bar4RemapAddress);

	// wait until the write is complete
	for(i = 0; i < FMB_PCI_POLL_COUNT_REG_WRITE_COMPLETE; i++)
	{
		if(ntv2ReadRegister32(remapRegAddress) == bar4RemapAddress) break;
	}
	if(i >= FMB_PCI_POLL_COUNT_REG_WRITE_COMPLETE)
	{
		HEVC_MSG_REGISTER_ERROR("%s(%d): hevcResetRemapAddress() *error*  remap address write timeout\n",
								pModParams->pModuleName, devNum);
	}

	return;
}

static void hevcResetContinuity(uint32_t devNum)
{
	// zero the continuity counters
	hevcRegWrite(devNum, FMB_REG_COMMAND_CONTINUITY_CNT, 0);
	hevcRegWrite(devNum, FMB_REG_COMMAND_ACK_CONTINUITY_CNT, 0);
	hevcRegWrite(devNum, FMB_REG_COMMAND_RESULT_CONTINUITY_CNT, 0);

	hevcRegWrite(devNum, FMB_REG_DMA_VEI_CONTINUITY_CNT, 0);
	hevcRegWrite(devNum, FMB_REG_DMA_VEI_ACK_CONTINUITY_CNT, 0);
	hevcRegWrite(devNum, FMB_REG_DMA_VEI_COMPLETE_CONTINUITY_CNT, 0);

	hevcRegWrite(devNum, FMB_REG_DMA_SEO_CONTINUITY_CNT, 0);
	hevcRegWrite(devNum, FMB_REG_DMA_SEO_ACK_CONTINUITY_CNT, 0);
	hevcRegWrite(devNum, FMB_REG_DMA_SEO_COMPLETE_CONTINUITY_CNT, 0);
}

static uint32_t hevcUpdateContinuityCount(uint32_t devNum, enum fmb_pci_continuity_cnt_type  cntType)
{
	HevcModuleParams* pModParams = hevcGetModuleParams();
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	uint32_t continuityRegAddress;
	uint32_t continuityCount;
	int i;

	// determine which counter to increment
	switch(cntType)
	{
	case FMB_PCI_CONT_COMMAND:
		continuityRegAddress = FMB_REG_COMMAND_CONTINUITY_CNT;
		break;

	case FMB_PCI_CONT_DMA_VEI:
		continuityRegAddress = FMB_REG_DMA_VEI_CONTINUITY_CNT;
		break;

	case FMB_PCI_CONT_DMA_SEO:
		continuityRegAddress = FMB_REG_DMA_SEO_CONTINUITY_CNT;
		break;

	default:
		return 0;
	}

	ntv2SpinLockAcquire(&pDevParams->regContinuityLock[cntType]);

	// do a read, increment and write
	continuityCount = hevcRegRead(devNum, continuityRegAddress) + 1;
	hevcRegWrite(devNum, continuityRegAddress, continuityCount);

	// wait until the write is complete
	for(i = 0; i < FMB_PCI_POLL_COUNT_REG_WRITE_COMPLETE; i++)
	{
		if(hevcRegRead(devNum, continuityRegAddress) == continuityCount) break;
	}

	ntv2SpinLockRelease(&pDevParams->regContinuityLock[cntType]);

	if(i >= FMB_PCI_POLL_COUNT_REG_WRITE_COMPLETE)
	{
		HEVC_MSG_REGISTER_ERROR("%s(%d): hevcUpdateContinuityCount() *error*  count type %d increment timeout\n",
								pModParams->pModuleName, devNum, cntType);
	}

	return continuityCount;
}

uint32_t hevcRegRead(uint32_t devNum, uint32_t regAddress)
{
	HevcModuleParams* pModParams = hevcGetModuleParams();
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	enum fmb_pci_bar_type barType;
	Ntv2Register pciAddress;
	uint32_t regData = 0;
	bool success = false;

	// dword align
	regAddress &= 0xfffffffc;

	if(regAddress == HEVC_DEBUG_DRIVER_REGISTER)
	{
		// our special debug mask register
		regData = pModParams->debugMask;
		success = true;
	}
	else
	{
		ntv2InterruptLockAcquire(&pDevParams->regAccessLock);

		// get which bar contains the register
		barType = hevcGetBarType(devNum, regAddress);

		// get the system address of the register
		hevcSetRemapAddress(devNum, regAddress, barType, &pciAddress);

		if(pciAddress != NULL)
		{
			// read the pci register
			regData = ntv2ReadRegister32(pciAddress);
			success = true;
		}

		ntv2InterruptLockRelease(&pDevParams->regAccessLock);
	}

	if(success)
	{
		HEVC_MSG_REGISTER_INFO("%s(%d): hevcRegRead()   address 0x%08x  data 0x%08x\n",
							   pModParams->pModuleName, devNum, regAddress, regData);
	}
	else
	{
		HEVC_MSG_REGISTER_ERROR("%s(%d): hevcRegRead()   *failed*  address 0x%08x\n",
								pModParams->pModuleName, devNum, regAddress);
	}

	return regData;
}

void hevcRegWrite(uint32_t devNum, uint32_t regAddress, uint32_t regData)
{
	HevcModuleParams* pModParams = hevcGetModuleParams();
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	enum fmb_pci_bar_type barType;
	Ntv2Register pciAddress;
	bool success = false;

	// dword align
	regAddress &= 0xfffffffc;

	if(regAddress == HEVC_DEBUG_DRIVER_REGISTER)
	{
		pModParams->debugMask = regData;
		success = true;
	}
	else
	{
		ntv2InterruptLockAcquire(&pDevParams->regAccessLock);

		// get which bar contains the register
		barType = hevcGetBarType(devNum, regAddress);

		// get the system address of the register
		hevcSetRemapAddress(devNum, regAddress, barType, &pciAddress);

		if(pciAddress != NULL)
		{
			// write the pci register
			ntv2WriteRegister32(pciAddress, regData);
			success = true;
		}

		ntv2InterruptLockRelease(&pDevParams->regAccessLock);
	}

	if(success)
	{
		HEVC_MSG_REGISTER_INFO("%s(%d): hevcRegWrite()  address 0x%08x  data 0x%08x\n",
							   pModParams->pModuleName, devNum, regAddress, regData);
	}
	else
	{
		HEVC_MSG_REGISTER_ERROR("%s(%d): hevcRegWrite()  *failed*  address 0x%08x  data 0x%08x\n",
								pModParams->pModuleName, devNum, regAddress, regData);
	}

	return;
}

void hevcRegReadMultiple(uint32_t devNum, uint32_t address, uint32_t size, uint32_t* pData)
{
	int i;

	if(pData == NULL) return;
	if(size == 0) return;

	for(i = 0; i < (int)size; i++)
	{
		pData[i] = hevcRegRead(devNum, (address + (i * 4)));
	}

	return;
}

void hevcRegWriteMultiple(uint32_t devNum, uint32_t address, uint32_t size, uint32_t* pData)
{
	int i;

	if(pData == NULL) return;
	if(size == 0) return;

	for(i = 0; i < (int)size; i++)
	{
		hevcRegWrite(devNum, (address + (i * 4)), pData[i]);
	}

	return;
}

void hevcRegRMW(uint32_t devNum, uint32_t regAddress,
				uint32_t writeMask, uint32_t writeShift, uint32_t writeData)
{
	HevcModuleParams* pModParams = hevcGetModuleParams();
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	enum fmb_pci_bar_type barType;
	Ntv2Register pciAddress;
	uint32_t shiftData;
	uint32_t shiftMask;
	uint32_t regRead = 0;
	uint32_t regWrite = 0;
	bool success = false;

	if(writeShift >= 32) return;

	// dword align
	regAddress &= 0xfffffffc;

	// apply shift
	shiftData = writeData << writeShift;
	shiftMask = writeMask << writeShift;

	ntv2InterruptLockAcquire(&pDevParams->regAccessLock);

	// get which bar contains the register
	barType = hevcGetBarType(devNum, regAddress);

	// get the system address of the register
	hevcSetRemapAddress(devNum, regAddress, barType, &pciAddress);

	if(pciAddress != NULL)
	{
		// read the pci register
		regRead = ntv2ReadRegister32(pciAddress);

		// modify the register contents
		regWrite = ((regRead & ~shiftMask) | (shiftData & shiftMask));

		// write the pci register
		ntv2WriteRegister32(pciAddress, regWrite);

		success = true;
	}

	ntv2InterruptLockRelease(&pDevParams->regAccessLock);

	if(success)
	{
		HEVC_MSG_REGISTER_INFO("%s(%d): hevcRegRMW()  address 0x%08x  data 0x%08x  mask 0x%08x  shift 0x%08x  read 0x%08x  write 0x%08x\n",
							   pModParams->pModuleName, devNum, regAddress,
							   writeData, writeMask, writeShift, regRead, regWrite);
	}
	else
	{
		HEVC_MSG_REGISTER_ERROR("%s(%d): hevcRegRMW()  *failed*  address 0x%08x  data 0x%08x  mask 0x%08x  shift 0x%08x\n",
								pModParams->pModuleName, devNum, regAddress,
								writeData, writeMask, writeShift);
	}

	return;
}

uint32_t hevcRegReadBar4(uint32_t devNum, uint32_t address)
{
	HevcModuleParams* pModParams = hevcGetModuleParams();
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	Ntv2Register pciAddress;
	uint32_t data = 0;
	bool success = false;

	// dword align
	address &= 0xfffffffc;

	ntv2InterruptLockAcquire(&pDevParams->regAccessLock);

	// get the system address of the register
	hevcSetRemapAddress(devNum, address, FMB_PCI_BAR4, &pciAddress);

	if(pciAddress != NULL)
	{
		// read the pci register
		data = ntv2ReadRegister32(pciAddress);
		success = true;
	}

	ntv2InterruptLockRelease(&pDevParams->regAccessLock);

	if(success)
	{
		HEVC_MSG_REGISTER_INFO("%s(%d): hevcRegReadBar4()   address 0x%08x  data 0x%08x\n",
							   pModParams->pModuleName, devNum, address, data);
	}
	else
	{
		HEVC_MSG_REGISTER_ERROR("%s(%d): hevcRegReadBar4()   *failed*  address 0x%08x\n",
								pModParams->pModuleName, devNum, address);
	}

	return data;
}

void hevcRegWriteBar4(uint32_t devNum, uint32_t address, uint32_t data)
{
	HevcModuleParams* pModParams = hevcGetModuleParams();
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	Ntv2Register pciAddress;
	bool success = false;

	// dword align
	address &= 0xfffffffc;

	ntv2InterruptLockAcquire(&pDevParams->regAccessLock);

	// get the system address of the register
	hevcSetRemapAddress(devNum, address, FMB_PCI_BAR4, &pciAddress);

	if(pciAddress != NULL)
	{
		// write the pci register
		ntv2WriteRegister32(pciAddress, data);
		success = true;
	}

	ntv2InterruptLockRelease(&pDevParams->regAccessLock);

	if(success)
	{
		HEVC_MSG_REGISTER_INFO("%s(%d): hevcRegWriteBar4()  address 0x%08x  data 0x%08x\n",
							   pModParams->pModuleName, devNum, address, data);
	}
	else
	{
		HEVC_MSG_REGISTER_ERROR("%s(%d): hevcRegWriteBar4()  *failed*  address 0x%08x  data 0x%08x\n",
								pModParams->pModuleName, devNum, address, data);
	}

	return;
}

bool hevcRegWait(uint32_t devNum, uint32_t address, uint32_t data,
				 uint32_t pollCount, uint32_t pollTime)
{
	uint32_t value;
	uint32_t count;

	for(count = 0; count < pollCount; count++)
	{
		value = hevcRegRead(devNum, address);
		if(value == data)
		{
			return true;
		}
		ntv2TimeSleep(pollTime);
	}

	return false;
}

bool hevcSetupCommand(uint32_t devNum, HevcCommandInfo* cmdInfo, uint32_t* pCntCount)
{
	if(pCntCount == NULL) return false;

	if(cmdInfo == NULL) 
	{
		*pCntCount = hevcUpdateContinuityCount(devNum, FMB_PCI_CONT_COMMAND);
		return false;
	}

	// write the command info to the codec registers
	hevcRegWrite(devNum, FMB_REG_COMMAND_TARGET, cmdInfo->target);
	hevcRegWrite(devNum, FMB_REG_COMMAND_ID, cmdInfo->id);
	hevcRegWriteMultiple(devNum, FMB_REG_COMMAND_PARAM_BASE, FMB_COMMAND_PARAM_MAX, cmdInfo->param);

	*pCntCount = hevcUpdateContinuityCount(devNum, FMB_PCI_CONT_COMMAND);

	return true;
}

void hevcSubmitCommand(uint32_t devNum)
{
	// execute command
	hevcRegWrite(devNum, FMB_REG_HSIO_PCIE0_INT_REG_SET, FMB_REG_HSIO_PCIE0_INT_REG_CMD);
}

bool hevcSetupVeiTransfer(uint32_t devNum, HevcStreamInfo* pStrInfo, uint32_t* pCntCount)
{
	HevcModuleParams* pModParams = hevcGetModuleParams();
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	HevcStreamBufferGroup* pBufferGroup;
	HevcStreamBuffer* pBuffer;
	uint32_t remainSize;
	uint32_t validSize;
	uint32_t copySize;
	uint32_t ib;
	uint32_t addrL;
	uint32_t addrH;
	bool videoTransfer = false;
	bool infoTransfer = false;

	if(pCntCount == NULL) return false;

	// verify video transfer data
	if((pDevParams == NULL) ||
	   (pStrInfo == NULL)) 
	{
		*pCntCount = hevcUpdateContinuityCount(devNum, FMB_PCI_CONT_DMA_VEI);
		return false;
	}

	pBufferGroup = pStrInfo->pBufferGroup;
	if(pBufferGroup == NULL) 
	{
		*pCntCount = hevcUpdateContinuityCount(devNum, FMB_PCI_CONT_DMA_VEI);
		return false;
	}

	remainSize = pBufferGroup->videoDataSize;
	validSize = 0;

	HEVC_MSG_DMA_INFO("%s(%d): hevcSetupVeiTransfer()  dma video to codec  type %d  id %d  buf %d, vid_size %d  pic_size %d  last %d\n",
					  pModParams->pModuleName, devNum, pBufferGroup->streamType,
					  pBufferGroup->streamId, pBufferGroup->bufNum,
					  pBufferGroup->videoDataSize, pBufferGroup->infoDataSize, pStrInfo->isLastFrame);

	// for each transfer buffers
	ib = 0;
	while(remainSize > 0)
	{
		if(ib >= HEVC_STREAM_DESCRIPTOR_MAX)
		{
			HEVC_MSG_DMA_ERROR("%s(%d): hevcSetupVeiTransfer()  *error* video descriptor overrun  type %d  id %d  buf %d\n",
							   pModParams->pModuleName, devNum, pBufferGroup->streamType,
							   pBufferGroup->streamId, pBufferGroup->bufNum);
			break;
		}

		pBuffer = &pBufferGroup->videoBuffers[ib];

		if((pBuffer->dmaAddress == 0) || (pBuffer->bufferSize == 0))
		{
			HEVC_MSG_DMA_ERROR("%s(%d): hevcSetupVeiTransfer()  *error* video buffer empty  type %d  id %d  buf %d\n",
							   pModParams->pModuleName, devNum, pBufferGroup->streamType,
							   pBufferGroup->streamId, pBufferGroup->bufNum);
			break;
		}

		// determine address and size of this buffer
		copySize = min(remainSize, pBuffer->bufferSize);
		if(hevcIs64bitAddress(pBuffer->dmaAddress))
		{
			addrL = (uint32_t)(pBuffer->dmaAddress & 0xFFFFFFFFU);
			addrH = (uint32_t)(pBuffer->dmaAddress >> 32);
		}
		else
		{
			addrL = (uint32_t)pBuffer->dmaAddress;
			addrH = 0x00000000U;
		}
		// write the transfer descriptor registers
		hevcRegWrite(devNum, FMB_REG_DMA_VEI_DESCRIPTOR_ADDRESS_L(ib), addrL);
		hevcRegWrite(devNum, FMB_REG_DMA_VEI_DESCRIPTOR_ADDRESS_H(ib), addrH);
		hevcRegWrite(devNum, FMB_REG_DMA_VEI_DESCRIPTOR_DMA_SIZE(ib), copySize);

		HEVC_MSG_DMA_DESCRIPTOR("%s(%d): hevcSetupVeiTransfer()  dma video descriptor  type %d  id %d  num %d addr %08x:%08x size %d\n",
								pModParams->pModuleName, devNum, pBufferGroup->streamType,
								pBufferGroup->streamId, ib, addrH, addrL, copySize);

		// track remaining buffer size and number of bytes transferred
		remainSize -= copySize;
		validSize += copySize;
		ib++;
		videoTransfer = true;
	}

	// write transfer data to codec registers
	hevcRegWrite(devNum, FMB_REG_DMA_VEI_DATA_ID, pBufferGroup->streamId);
	hevcRegWrite(devNum, FMB_REG_DMA_VEI_NUMBER_OF_DESCRIPTORS, ib);
	hevcRegWrite(devNum, FMB_REG_DMA_VEI_TOTAL_SIZE, validSize);
	hevcRegWrite(devNum, FMB_REG_DMA_VEI_LAST_FRAME_MARKER,
				 pStrInfo->isLastFrame? FMB_REG_DMA_VEI_LAST_FRAME : FMB_REG_DMA_VEI_NOT_LAST_FRAME);

	// transfer pic data
	if(pBufferGroup->infoDataSize > 0)
	{
		if((pBufferGroup->infoDataSize >= FMB_TRANSFER_VEI_PIC_INFO_MIN_SIZE) &&
		   (pBufferGroup->infoDataSize <= pBufferGroup->infoBufferSize))
		{
			// determine address and size of this buffer
			pBuffer = &pBufferGroup->infoBuffer;
			copySize = pBufferGroup->infoDataSize;
			if(hevcIs64bitAddress(pBuffer->dmaAddress))
			{
				addrL = (uint32_t)(pBuffer->dmaAddress & 0xFFFFFFFFU);
				addrH = (uint32_t)(pBuffer->dmaAddress >> 32);
			}
			else
			{
				addrL = (uint32_t)pBuffer->dmaAddress;
				addrH = 0x00000000U;
			}
			// write the transfer descriptor registers
			hevcRegWrite(devNum, FMB_REG_DMA_VEI_PICTURE_INFO_ADDRESS_L, addrL);
			hevcRegWrite(devNum, FMB_REG_DMA_VEI_PICTURE_INFO_ADDRESS_H, addrH);
			hevcRegWrite(devNum, FMB_REG_DMA_VEI_PICTURE_INFO_SIZE, copySize);
			infoTransfer = true;

			HEVC_MSG_DMA_DESCRIPTOR("%s(%d): hevcSetupVeiTransfer()  dma pic descriptor  type %d  id %d  addr %08x:%08x size %d\n",
									pModParams->pModuleName, devNum, pBufferGroup->streamType,
									pBufferGroup->streamId, addrH, addrL, copySize);
		}
		else
		{
			HEVC_MSG_DMA_ERROR("%s(%d): hevcSetupVeiTransfer()  *error* bad pic info size  type %d  id %d  buf %d  size %d\n",
							   pModParams->pModuleName, devNum, pBufferGroup->streamType,
							   pBufferGroup->streamId, pBufferGroup->bufNum, pBufferGroup->infoDataSize);

			hevcRegWrite(devNum, FMB_REG_DMA_VEI_PICTURE_INFO_ADDRESS_L, 0x0);
			hevcRegWrite(devNum, FMB_REG_DMA_VEI_PICTURE_INFO_ADDRESS_H, 0x0);
			hevcRegWrite(devNum, FMB_REG_DMA_VEI_PICTURE_INFO_SIZE, 0x0);
		}
	}
	else
	{
		// nothing to transfer
		hevcRegWrite(devNum, FMB_REG_DMA_VEI_PICTURE_INFO_ADDRESS_L, 0x0);
		hevcRegWrite(devNum, FMB_REG_DMA_VEI_PICTURE_INFO_ADDRESS_H, 0x0);
		hevcRegWrite(devNum, FMB_REG_DMA_VEI_PICTURE_INFO_SIZE, 0x0);
	}

	// increment continuity count
	*pCntCount = hevcUpdateContinuityCount(devNum, FMB_PCI_CONT_DMA_VEI);

	if(!videoTransfer && !infoTransfer) return false;

	return true;
}

void hevcSubmitVeiTransfer(uint32_t devNum)
{
	// execute vei transfer
	hevcRegWrite(devNum, FMB_REG_HSIO_PCIE0_INT_REG_SET, FMB_REG_HSIO_PCIE0_INT_REG_DMA_VEI_REQ);
}

bool hevcSetupSeoTransfer(uint32_t devNum, HevcStreamInfo* pStrInfo, uint32_t* pCntCount)
{
	HevcModuleParams* pModParams = hevcGetModuleParams();
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	HevcStreamBufferGroup* pBufferGroup;
	HevcStreamBuffer* pBuffer;
	uint32_t remainSize;
	uint32_t validSize;
	uint32_t copySize;
	uint32_t addrL;
	uint32_t addrH;
	uint32_t ib;
	bool videoTransfer = false;
	bool infoTransfer = false;

	if(pCntCount == NULL) return false;

	// verify video transfer data
	if((pDevParams == NULL) ||
	   (pStrInfo == NULL)) 
	{
		*pCntCount = hevcUpdateContinuityCount(devNum, FMB_PCI_CONT_DMA_SEO);
		return false;
	}

	pBufferGroup = pStrInfo->pBufferGroup;
	if(pBufferGroup == NULL)
	{
		*pCntCount = hevcUpdateContinuityCount(devNum, FMB_PCI_CONT_DMA_SEO);
		return false;
	}

	remainSize = pBufferGroup->videoBufferSize;
	validSize = 0;

	HEVC_MSG_DMA_INFO("%s(%d): hevcSetupSeoTransfer()  dma hevc from codec  type %d  id %d  buf %d  vid_size %d  es_size %d\n",
					  pModParams->pModuleName, devNum, pBufferGroup->streamType,
					  pBufferGroup->streamId, pBufferGroup->bufNum,
					  pBufferGroup->videoBufferSize, pBufferGroup->infoBufferSize);

	// for each transfer buffers
	ib = 0;
	while(remainSize > 0)
	{
		if(ib >= HEVC_STREAM_DESCRIPTOR_MAX)
		{
			HEVC_MSG_DMA_ERROR("%s(%d): hevcSetupSeoTransfer()  *error* hevc descriptor overrun  type %d  id %d  buf %d\n",
							   pModParams->pModuleName, devNum, pBufferGroup->streamType,
							   pBufferGroup->streamId, pBufferGroup->bufNum);
			break;
		}

		pBuffer = &pBufferGroup->videoBuffers[ib];

		if((pBuffer->dmaAddress == 0) || (pBuffer->bufferSize == 0))
		{
			HEVC_MSG_DMA_ERROR("%s(%d): hevcSetupSeoTransfer()  *error* hevc buffer empty  type %d  id %d  buf %d\n",
							   pModParams->pModuleName, devNum, pBufferGroup->streamType,
							   pBufferGroup->streamId, pBufferGroup->bufNum);
			break;
		}

		// determine address and size of this buffer
		copySize = min(remainSize, pBuffer->bufferSize);
		if(hevcIs64bitAddress(pBuffer->dmaAddress))
		{
			addrL = (uint32_t)(pBuffer->dmaAddress & 0xFFFFFFFFU);
			addrH = (uint32_t)(pBuffer->dmaAddress >> 32);
		}
		else
		{
			addrL = (uint32_t)pBuffer->dmaAddress;
			addrH = 0x00000000U;
		}
		// write the transfer descriptor registers
		hevcRegWrite(devNum, FMB_REG_DMA_SEO_DESCRIPTOR_ADDRESS_L(ib), addrL);
		hevcRegWrite(devNum, FMB_REG_DMA_SEO_DESCRIPTOR_ADDRESS_H(ib), addrH);
		hevcRegWrite(devNum, FMB_REG_DMA_SEO_DESCRIPTOR_DMA_SIZE(ib), copySize);

		HEVC_MSG_DMA_DESCRIPTOR("%s(%d): hevcSetupSeoTransfer()  dma hevc descriptor  type %d  id %d  num %d addr %08x:%08x size %d\n",
								pModParams->pModuleName, devNum, pBufferGroup->streamType,
								pBufferGroup->streamId, ib, addrH, addrL, copySize);

		// track remaining buffer size and number of bytes transferred
		remainSize -= copySize;
		validSize += copySize;
		ib++;
		videoTransfer = true;
	}

	// write transfer data to codec registers
	hevcRegWrite(devNum, FMB_REG_DMA_SEO_DATA_ID, pBufferGroup->streamId);
	hevcRegWrite(devNum, FMB_REG_DMA_SEO_NUMBER_OF_DESCRIPTORS, ib);
	hevcRegWrite(devNum, FMB_REG_DMA_SEO_TOTAL_SIZE, validSize);

	// transfer es data
	if(pBufferGroup->infoBufferSize > 0)
	{
		// determine address and size of this buffer
		pBuffer = &pBufferGroup->infoBuffer;
		copySize = pBufferGroup->infoBufferSize;
		if(hevcIs64bitAddress(pBuffer->dmaAddress))
		{
			addrL = (uint32_t)(pBuffer->dmaAddress & 0xFFFFFFFFU);
			addrH = (uint32_t)(pBuffer->dmaAddress >> 32);
		}
		else
		{
			addrL = (uint32_t)pBuffer->dmaAddress;
			addrH = 0x00000000U;
		}
		// write the transfer descriptor registers
		hevcRegWrite(devNum, FMB_REG_DMA_SEO_ES_INFO_ADDRESS_L, addrL);
		hevcRegWrite(devNum, FMB_REG_DMA_SEO_ES_INFO_ADDRESS_H, addrH);
		hevcRegWrite(devNum, FMB_REG_DMA_SEO_ES_INFO_SIZE, copySize);
		infoTransfer = true;

		HEVC_MSG_DMA_DESCRIPTOR("%s(%d): hevcSetupSeoTransfer()  dma es descriptor  type %d  id %d  addr %08x:%08x size %d\n",
								pModParams->pModuleName, devNum, pBufferGroup->streamType,
								pBufferGroup->streamId, addrH, addrL, copySize);
	}
	else
	{
		hevcRegWrite(devNum, FMB_REG_DMA_SEO_ES_INFO_ADDRESS_L, 0x0);
		hevcRegWrite(devNum, FMB_REG_DMA_SEO_ES_INFO_ADDRESS_H, 0x0);
		hevcRegWrite(devNum, FMB_REG_DMA_SEO_ES_INFO_SIZE, 0x0);
	}

	// increment continuity count
	*pCntCount = hevcUpdateContinuityCount(devNum, FMB_PCI_CONT_DMA_SEO);

	if(!videoTransfer && !infoTransfer) return false;

	return true;
}

void hevcSubmitSeoTransfer(uint32_t devNum)
{
	// execute seo transfer
	hevcRegWrite(devNum, FMB_REG_HSIO_PCIE0_INT_REG_SET, FMB_REG_HSIO_PCIE0_INT_REG_DMA_SEO_REQ);
}

bool hevcRestartFirmware(uint32_t devNum)
{
	HevcModuleParams* pModParams = hevcGetModuleParams();
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	int i;
	bool success;

	if(pDevParams == NULL) return false;

	HEVC_MSG_REGISTER_INFO("%s(%d): hevcRestartFirmware() begin\n", pModParams->pModuleName, devNum);

	// clear main state
	hevcRegWrite(devNum, FMB_REG_MAIN_STATE, FMB_MAIN_STATE_ERROR);

	// wait for write
	success = hevcRegWait(devNum, FMB_REG_MAIN_STATE, FMB_MAIN_STATE_ERROR,
						  FMB_CODEC_STATE_RESET_POLLING_COUNT,
						  FMB_CODEC_STATE_RESET_POLLING_TIME);
	if(!success)
	{
		HEVC_MSG_REGISTER_ERROR("%s(%d): hevcRestartFirmware()  main state write failed\n", pModParams->pModuleName, devNum);
	}

	// reset bar 4 remap
	hevcResetRemapAddress(devNum);

	// reset firmware
	hevcRegWrite(devNum, FMB_REG_MCPU_FIRMWARE_RESTART_REQUEST, FMB_MCPU_FIRMWARE_RESTART);

	// wait for restart
	success = hevcRegWait(devNum, FMB_REG_MAIN_STATE, FMB_MAIN_STATE_BOOT,
						  FMB_CODEC_STATE_RESET_POLLING_COUNT,
						  FMB_CODEC_STATE_RESET_POLLING_TIME);

	// wait a little longer to prevent initial state change error
	ntv2TimeSleep(100000);

	// reset bar 4 remap
	hevcResetRemapAddress(devNum);

	// reset continuity
	hevcResetContinuity(devNum);

	// mcpu does not clear these
	for(i = 0; i < FMB_STREAM_ID_MAX; i++)
	{
		hevcRegWrite(devNum, FMB_REG_VIN_STATE(i), 0);
		hevcRegWrite(devNum, FMB_REG_EH_STATE(i), 0);
	}

	if(!success)
	{
		HEVC_MSG_REGISTER_ERROR("%s(%d): hevcRestartFirmware()  mcpu restart failed\n", pModParams->pModuleName, devNum);
		return false;
	}

	HEVC_MSG_REGISTER_INFO("%s(%d): hevcRestartFirmware()  complete\n", pModParams->pModuleName, devNum);

	return true;
}

void hevcGetFirmwareVersion(uint32_t devNum,
							uint32_t* pMCPUFirmware,
							char* pSystemFirmware,
							char* pStandardFirmwareSingle,
							char* pStandardFirmwareMultiple,
							char* pUserFirmwareSingle,
							char* pUserFirmwareMultiple)
{
	// get MCPU firmware version
	readFirmwareArray(devNum, pMCPUFirmware, FMB_REG_VERSION_MCPU, FMB_VERSION_SIZE_MCPU_FIRM);

	// get system firmware version
	readFirmwareString(devNum, pSystemFirmware, FMB_REG_VERSION_SYSTEM, FMB_VERSION_SIZE_SYSTEM_FIRM);

	// get standard hevc encoder firmware version
	readFirmwareString(devNum, pStandardFirmwareSingle,
					   FMB_REG_VERSION_HEVC_ENCODER_STD_SINGLE,
					   FMB_VERSION_SIZE_HEVC_ENCODER_FIRM);
	readFirmwareString(devNum, pStandardFirmwareMultiple,
					   FMB_REG_VERSION_HEVC_ENCODER_STD_MULTI,
					   FMB_VERSION_SIZE_HEVC_ENCODER_FIRM);

	// get user hevc encoder firmware version
	readFirmwareString(devNum, pUserFirmwareSingle,
					   FMB_REG_VERSION_HEVC_ENCODER_USER_SINGLE,
					   FMB_VERSION_SIZE_HEVC_ENCODER_FIRM);
	readFirmwareString(devNum, pUserFirmwareMultiple,
					   FMB_REG_VERSION_HEVC_ENCODER_USER_MULTI,
					   FMB_VERSION_SIZE_HEVC_ENCODER_FIRM);
}

static bool hevcIs64bitAddress(Ntv2DmaAddress address)
{
	if((address >> 32) != 0) return true;

	return false;
}

static void readFirmwareArray(uint32_t devNum, uint32_t* pArray, uint32_t arReg, uint32_t arSize)
{
	int cnt;

	if (pArray == NULL) return;

	for(cnt = 0; cnt < (int)arSize; cnt++)
	{
		pArray[cnt] = hevcRegRead(devNum, arReg + (cnt * sizeof(uint32_t)));
	}
}

static void readFirmwareString(uint32_t devNum, char* pString, uint32_t strReg, uint32_t strSize)
{
	uint32_t regValue;
	uint32_t cnt;

	if (pString == NULL) return;

	for(cnt = 0; cnt < strSize / sizeof(uint32_t); cnt++ )
	{
		regValue = hevcRegRead(devNum, strReg + (cnt * sizeof(uint32_t)));
		copyU32ToU8Array(regValue, (uint8_t*)&pString[cnt * sizeof(uint32_t)]);
	}

	cleanString(pString, strSize);
}

static void copyU32ToU8Array(uint32_t src, uint8_t* pDest)
{
	if(pDest == NULL) return;

	pDest[0] = src & 0x000000FFU;
	pDest[1] = (src >> 8) & 0x000000FFU;
	pDest[2] = (src >> 16) & 0x000000FFU;
	pDest[3] = (src >> 24) & 0x000000FFU;

	return;
}

static void cleanString(char* pString, uint32_t strSize)
{
	int cnt;

	pString[strSize] = '\0';
	for(cnt = 0; cnt < (int)strSize; cnt++ )
	{
		if(!isGoodChar(pString[cnt]))
		{
			pString[cnt] = '\0';
			break;
		}
	}
}

static bool isGoodChar(char ch)
{
	if((ch >= 0x20) && (ch <= 0x7e)) return true;

	return false;
}
