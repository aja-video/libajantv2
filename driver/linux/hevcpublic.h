/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
///////////////////////////////////////////////////////////////
//
// HEVC Linux Device Driver for AJA HEVC devices.
//
////////////////////////////////////////////////////////////
//
// Filename: hevcpublic.h
// Purpose:	 HEVC module public interface
// Notes:	 
//
///////////////////////////////////////////////////////////////

#ifndef HEVCPUBLIC_H
#define HEVCPUBLIC_H

#include "ntv2publicinterface.h"
#include "ntv2system.h"

#define HEVC_VENDOR_ID  0x10cf
#define HEVC_DEVICE_ID  0x2049


#ifdef __cplusplus
extern "C"
{
#endif

// device initialization
Ntv2Status  hevcDeviceOpen(uint32_t devNum);
Ntv2Status  hevcDeviceClose(uint32_t devNum);

// hevc ioctl api
Ntv2Status	hevcGetDeviceInfo(uint32_t devNum, HevcMessageInfo* pMessage);
Ntv2Status	hevcRegister(uint32_t devNum, HevcMessageRegister* pMessage);
Ntv2Status	hevcSendCommand(uint32_t devNum, HevcMessageCommand* pMessage);
Ntv2Status	hevcVideoTransfer(uint32_t devNum, HevcMessageTransfer* pMessage);
Ntv2Status	hevcGetStatus(uint32_t devNum, HevcMessageStatus* pMessage);
Ntv2Status	hevcDebugInfo(uint32_t devNum, HevcMessageDebug* pMessage);

// find hevc device
uint32_t			hevcGetNumDevices(void);
Ntv2SystemContext*	hevcGetSystemContext(uint32_t devNum);
bool				hevcIsCodecMode(uint32_t devNum);
Ntv2Status			hevcSetGpio(uint32_t devNum, uint32_t gpioNum, bool output, bool high);
#ifdef __cplusplus
}
#endif

#endif
