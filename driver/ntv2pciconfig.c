/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
///////////////////////////////////////////////////////////////
//
// NTV2 Device Driver for AJA OEM boards.
//
// Filename: ntv2pciconfig.c
// Purpose:	 PCI configuration space utility
//
///////////////////////////////////////////////////////////////

#include "ntv2pciconfig.h"

// PCI find capability constants
#define NTV2_PCI_CONFIG_SPACE_SIZE		256
#define NTV2_PCI_CONFIG_SPACE_EXP_SIZE	4096
#define NTV2_PCI_CONFIG_CAP_MAX			48
#define NTV2_PCI_CONFIG_EXT_CAP_MAX		(NTV2_PCI_CONFIG_SPACE_EXP_SIZE - NTV2_PCI_CONFIG_SPACE_SIZE) / 8

#define NTV2_PCI_CAP_POINTER_OFFSET		0x34
#define NTV2_PCI_CAP_POINTER_SIZE		1

#define NTV2_PCI_CAP_HEADER_SIZE			2
#define NTV2_PCI_CAP_ID(header)			(header & 0xff)
#define NTV2_PCI_CAP_NEXT(header)		((header >> 8) & 0xff)

#define NTV2_PCI_EXT_CAP_OFFSET			NTV2_PCI_CONFIG_SPACE_SIZE

#define NTV2_PCI_EXT_CAP_HEADER_SIZE		4
#define NTV2_PCI_EXT_CAP_ID(header)		(header & 0xffff)
#define NTV2_PCI_EXT_CAP_VERISON(header)	((header >> 16) & 0xf)
#define NTV2_PCI_EXT_CAP_NEXT(header)	((header >> 20) & 0xffc)

int32_t ntv2PciFindCapability(Ntv2SystemContext* pSysCon, uint32_t cap_id)
{
	uint32_t buffer;
	Ntv2Status status;
	int32_t offset = 0;
	int32_t count = NTV2_PCI_CONFIG_CAP_MAX;
	uint32_t id = 0;

	if (pSysCon == NULL) return 0;

	// get the capability offset
	buffer = 0;
	status = ntv2ReadPciConfig(pSysCon, &buffer, NTV2_PCI_CAP_POINTER_OFFSET, NTV2_PCI_CAP_POINTER_SIZE);
//	KdPrint(("CNTV2Device::ntv2PciFindCapability pci cap offset 0x%08x  status %08x\n", buffer, status));
	if (status != NTV2_STATUS_SUCCESS) return 0;
	offset = buffer;

	// find the capability id offset
	while (offset != 0)
	{
		buffer = 0;
		status = ntv2ReadPciConfig(pSysCon, &buffer, offset, NTV2_PCI_CAP_HEADER_SIZE);
//		KdPrint(("CNTV2Device::ntv2PciFindCapability pci cap list 0x%08x  status %08x\n", buffer, status));
		if (status != NTV2_STATUS_SUCCESS) return 0;
		id = NTV2_PCI_CAP_ID(buffer);
		if (id == cap_id) break;
		offset = NTV2_PCI_CAP_NEXT(buffer);
		if (--count <= 0) return 0;
	}

	return offset;
}

int32_t ntv2PciFindExtCapability(Ntv2SystemContext* pSysCon, uint32_t ext_id)
{
	uint32_t buffer;
	Ntv2Status status;
	int32_t offset = NTV2_PCI_EXT_CAP_OFFSET;
	int32_t count = NTV2_PCI_CONFIG_EXT_CAP_MAX;
	uint32_t id = 0;

	if (pSysCon == NULL) return 0;

	// find the extended capability id offset
	while (offset != 0)
	{
		buffer = 0;
		status = ntv2ReadPciConfig(pSysCon, &buffer, offset, NTV2_PCI_EXT_CAP_HEADER_SIZE);
//		KdPrint(("CNTV2Device::ntv2PciExtFindCapability pci cap list 0x%08x  status %08x\n", buffer, status));
		if (status != NTV2_STATUS_SUCCESS) return 0;
		id = NTV2_PCI_EXT_CAP_ID(buffer);
		if (id == ext_id) break;
		offset = NTV2_PCI_EXT_CAP_NEXT(buffer);
		if (--count <= 0) return 0;
	}

	return offset;
}


// PCI max read request size constants
#define NTV2_PCI_CAPABILITY_EXPRESS_ID			0x10
#define NTV2_PCI_DEVICE_CONTROL_OFFSET			0x8
#define NTV2_PCI_DEVICE_CONTROL_LENGTH			2
#define NTV2_PCI_MAX_READ_REQUEST_SIZE_MASK		0x7000
#define NTV2_PCI_MAX_READ_REQUEST_SIZE_SHIFT		12
#define NTV2_PCI_MAX_READ_REQUEST_SIZE_128		0x0
#define NTV2_PCI_MAX_READ_REQUEST_SIZE_256		0x1
#define NTV2_PCI_MAX_READ_REQUEST_SIZE_512		0x2
#define NTV2_PCI_MAX_READ_REQUEST_SIZE_1024		0x3
#define NTV2_PCI_MAX_READ_REQUEST_SIZE_2048		0x4
#define NTV2_PCI_MAX_READ_REQUEST_SIZE_4096		0x5

uint32_t
ntv2ReadPciMaxReadRequestSize(Ntv2SystemContext* pSysCon)
{
	uint32_t buffer;
	int32_t offset;
	uint32_t req;
	Ntv2Status status;

	if (pSysCon == NULL) return 0xbad0;

	// compute the device control register offset
	offset = ntv2PciFindCapability(pSysCon, NTV2_PCI_CAPABILITY_EXPRESS_ID);
	if (offset == 0) return 0xbad1;
	offset += NTV2_PCI_DEVICE_CONTROL_OFFSET;

	// read the device control data
	buffer = 0;
	status = ntv2ReadPciConfig(pSysCon, &buffer, offset, NTV2_PCI_DEVICE_CONTROL_LENGTH);
	if (status != NTV2_STATUS_SUCCESS) return 0xbad2;
	req = (buffer & NTV2_PCI_MAX_READ_REQUEST_SIZE_MASK) >> NTV2_PCI_MAX_READ_REQUEST_SIZE_SHIFT;

	return req;
}

Ntv2Status 
ntv2WritePciMaxReadRequestSize(Ntv2SystemContext* pSysCon, uint32_t reqSize)
{
	uint32_t buffer;
	int32_t offset;
	Ntv2Status status;

	if ((pSysCon == NULL) || (reqSize > NTV2_PCI_MAX_READ_REQUEST_SIZE_4096))
	{
		return NTV2_STATUS_BAD_PARAMETER;
	}

	// compute the device control register offset
	offset = ntv2PciFindCapability(pSysCon, NTV2_PCI_CAPABILITY_EXPRESS_ID);
	if (offset == 0) return NTV2_STATUS_FAIL;
	offset += NTV2_PCI_DEVICE_CONTROL_OFFSET;

	// read/modify/write the device control data
	buffer = 0;
	status = ntv2ReadPciConfig(pSysCon, &buffer, offset, NTV2_PCI_DEVICE_CONTROL_LENGTH);
	if (status != NTV2_STATUS_SUCCESS) return status;

	buffer = (buffer & ~NTV2_PCI_MAX_READ_REQUEST_SIZE_MASK) |
		((reqSize <<  NTV2_PCI_MAX_READ_REQUEST_SIZE_SHIFT) & NTV2_PCI_MAX_READ_REQUEST_SIZE_MASK);

	status = ntv2WritePciConfig(pSysCon, &buffer, offset, NTV2_PCI_DEVICE_CONTROL_LENGTH);
	if (status != NTV2_STATUS_SUCCESS) return status;

	return NTV2_STATUS_SUCCESS;
}
