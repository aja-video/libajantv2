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
// Filename: hevcparams.c
// Purpose:	 HEVC private device parameters
//
///////////////////////////////////////////////////////////////

#include "hevcparams.h"

// module and device data storage
static HevcModuleParams sHevcModuleParams;
static HevcDeviceParams* spHevcDeviceParams[HEVC_DEVICE_MAX];
static bool sHevcModuleInit = false;

void hevcParamsInitialize(const char* pModuleName)
{
	if(!sHevcModuleInit)
	{
		// clear module structure and save module name
		memset(&sHevcModuleParams, 0, sizeof(HevcModuleParams));
		sHevcModuleParams.pModuleName = pModuleName;

		// set module version ???
		sHevcModuleParams.driverVersion.major = HEVC_DRIVER_MAJOR;
		sHevcModuleParams.driverVersion.minor = HEVC_DRIVER_MINOR;
		sHevcModuleParams.driverVersion.point = HEVC_DRIVER_POINT;
		sHevcModuleParams.driverVersion.build = HEVC_DRIVER_BUILD;

		// initialize the device param pointers
		memset(spHevcDeviceParams, 0, sizeof(spHevcDeviceParams));

		sHevcModuleInit = true;
	}
}

void hevcParamsRelease(void)
{
	int i;

	for(i = 0; i < HEVC_DEVICE_MAX; i++)
	{
		hevcFreeDevice(i);
	}
}

uint32_t hevcGetMaxDevices(void)
{
	return HEVC_DEVICE_MAX;
}

HevcModuleParams* hevcGetModuleParams(void)
{
	return &sHevcModuleParams;
}

uint32_t hevcNewDevice(void)
{
	HevcDeviceParams* pDevParams = NULL;
	uint32_t i;

	for(i = 0; i < HEVC_DEVICE_MAX; i++)
	{
		if(spHevcDeviceParams[i] == NULL) break;
	}

	if(i >= HEVC_DEVICE_MAX)
	{
		return HEVC_DEVICE_MAX;
	}

	pDevParams = ntv2MemoryAlloc(sizeof(HevcDeviceParams));
	if(pDevParams == NULL) return HEVC_DEVICE_MAX;

	memset(pDevParams, 0, sizeof(HevcDeviceParams));

	pDevParams->devNum = i;

	spHevcDeviceParams[i] = pDevParams;

	return i;
}

void hevcFreeDevice(uint32_t devNum)
{
	if(spHevcDeviceParams[devNum] == NULL) return;

	ntv2MemoryFree(spHevcDeviceParams[devNum], sizeof(HevcDeviceParams));
	spHevcDeviceParams[devNum] = NULL;
}

HevcDeviceParams* hevcGetDeviceParams(uint32_t devNum)
{
	if(devNum >= HEVC_DEVICE_MAX) return NULL;

	return spHevcDeviceParams[devNum];
}
