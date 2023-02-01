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
// Filename: hevcregister.h
// Purpose:	 HEVC register access functions
// Notes:	 
//
///////////////////////////////////////////////////////////////

#ifndef HEVCREGISTER_H
#define HEVCREGISTER_H

#include "hevccommon.h"

// initialize and release register resources
void 		hevcRegisterInitialize(uint32_t devNum);
void 		hevcRegisterRelease(uint32_t devNum);


// is the device in a working state
bool 		hevcIsDeviceAlive(uint32_t devNum);

// read/write a codec registers
uint32_t	hevcRegRead(uint32_t devNum, uint32_t address);
void		hevcRegWrite(uint32_t devNum, uint32_t address, uint32_t data);

// read/write multiple contiguous registers
void		hevcRegReadMultiple(uint32_t devNum, uint32_t address, uint32_t size, uint32_t* pData);
void		hevcRegWriteMultiple(uint32_t devNum, uint32_t address, uint32_t size, uint32_t* pData);

// read modify write codec register
void		hevcRegRMW(uint32_t devNum, uint32_t address,
					   uint32_t writeMask, uint32_t writeShift, uint32_t writeData);

// read/write a bar 4 codec registers (force for maintenance mode)
uint32_t	hevcRegReadBar4(uint32_t devNum, uint32_t address);
void		hevcRegWriteBar4(uint32_t devNum, uint32_t address, uint32_t data);

// wait for register data
bool		hevcRegWait(uint32_t devNum, uint32_t address, uint32_t data,
						uint32_t pollCount, uint32_t pollTime);

// setup codec command registers
bool		hevcSetupCommand(uint32_t devNum, HevcCommandInfo* cmdInfo, uint32_t* pCntCount);

// submit codec command
void		hevcSubmitCommand(uint32_t devNum);

// setup vei dma transfer
bool		hevcSetupVeiTransfer(uint32_t devNum, HevcStreamInfo* pStrInfo, uint32_t* pCntCount);

// submit vei dma transfer
void		hevcSubmitVeiTransfer(uint32_t devNum);

// setup seo dma transfer
bool		hevcSetupSeoTransfer(uint32_t devNum, HevcStreamInfo* pStrInfo, uint32_t* pCntCount);

// submit seo dma transfer
void		hevcSubmitSeoTransfer(uint32_t devNum);

// restart codec
bool		hevcRestartFirmware(uint32_t devNum);


// get codec firmware version strings
void		hevcGetFirmwareVersion(uint32_t devNum,
								   uint32_t* pMCPUFirmware,
								   char* pSystemFirmware,
								   char* pStandardFirmwareSingle,
								   char* pStandardFirmwareMultiple,
								   char* pUserFirmwareSingle,
								   char* pUserFirmwareMultiple);
#endif
