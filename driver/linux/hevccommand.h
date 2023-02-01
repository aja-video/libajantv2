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
// Filename: hevccommand.h
// Purpose:	 HEVC command processing fuctions
// Notes:	 
//
///////////////////////////////////////////////////////////////

#ifndef HEVCCOMMAND_H
#define HEVCCOMMAND_H

#include "hevccommon.h"

// initialize and release command queue resources
void hevcCommandInitialize(uint32_t devNum);
void hevcCommandRelease(uint32_t devNum);

// reset command queue
void hevcCommandReset(uint32_t devNum);


// enqueue a command
HevcCommandTask* hevcCommandEnqueue(uint32_t devNum, HevcCommandInfo* pCmdInfo);

// wait for command ackknowledge
bool hevcCommandAckWait(uint32_t devNum, HevcCommandTask* pCmdTask);

// wait for command message (complete)
bool hevcCommandMsgWait(uint32_t devNum, HevcCommandTask* pCmdTask);

// dequeue command
bool hevcCommandDequeue(uint32_t devNum, HevcCommandTask* pCmdTask);


// record command acknowledge information to task
void hevcCommandAck(uint32_t devNum, HevcCommandAckInfo* pAckInfo);

// record command message information to task
void hevcCommandMsg(uint32_t devNum, HevcCommandMsgInfo* pMsgInfo);

#endif
