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
// Filename: ntv2drivertask.h
// Purpose:  Header file for autocirculate methods.
// Notes:
//
///////////////////////////////////////////////////////////////

#ifndef NTV2DRIVERTASK_H
#define NTV2DRIVERTASK_H


bool InitTaskArray(AutoCircGenericTask* pTaskArray, ULWord numTasks);

ULWord CopyTaskArray(AutoCircGenericTask* pDstArray, ULWord dstSize, ULWord dstMax,
					 const AutoCircGenericTask* pSrcArray, ULWord srcSize, ULWord srcNum);

//bool DoTaskArray(INTERNAL_AUTOCIRCULATE_STRUCT* pAuto, AutoCircGenericTask* pTaskArray, ULWord numTasks);

#endif // NTV2DRIVERTASK_H
