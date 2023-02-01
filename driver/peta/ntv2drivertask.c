/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
///////////////////////////////////////////////////////////////
//
// NTV2 Linux v2.6 Device Driver for AJA devices.
//
// Filename:	ntv2drivertask.c
// Purpose: 	Implementation file for task methods.
// Notes:	
//
///////////////////////////////////////////////////////////////

/*needed by kernel 2.6.18*/
#ifndef CONFIG_HZ
#include <linux/autoconf.h>
#endif

#include <linux/kernel.h> // for printk() prototype
#include <linux/delay.h>
#include <linux/pci.h>
#include <asm/uaccess.h>
#include <asm/div64.h>
#include <linux/compiler.h>

#include "ajatypes.h"
#include "ntv2enums.h"

#include "ntv2publicinterface.h"
#include "ntv2linuxpublicinterface.h"

#include "registerio.h"
#include "driverdbg.h"
#include "ntv2driverdbgmsgctl.h"
#include "../ntv2kona.h"

//#define TASKDUMP

bool InitTaskArray(AutoCircGenericTask* pTaskArray, ULWord numTasks)
{
	if((pTaskArray == NULL) || (numTasks > AUTOCIRCULATE_TASK_MAX_TASKS))
	{
		return false;
	}

	memset(pTaskArray, 0, numTasks * sizeof(AutoCircGenericTask));

	return true;
}


ULWord CopyTaskArray(AutoCircGenericTask* pDstArray, ULWord dstSize, ULWord dstMax,
							const AutoCircGenericTask* pSrcArray, ULWord srcSize, ULWord srcNum)
{
	ULWord i;
	ULWord transferSize = 0;
	ULWord transferNum = 0;
	UByte* pSrc = NULL;
	UByte* pDst = NULL;

	// copy src to dst with support for changes in sizeof(AutoCircGenericTask)

	if((pSrcArray == NULL) || (pDstArray == NULL))
	{
		return false;
	}

	transferSize = srcSize;
	if(transferSize > dstSize)
	{
		transferSize = dstSize;
	}

	transferNum = srcNum;
	if(transferNum > dstMax)
	{
		transferNum = dstMax;
	}

	pSrc = (UByte*)pSrcArray;
	pDst = (UByte*)pDstArray;

	if(MsgsEnabled(NTV2_DRIVER_AUTOCIRCULATE_DEBUG_MESSAGES))
	{
		MSG("copy %d tasks\n", transferNum);
	}

	for(i = 0; i < transferNum; i++)
	{
		memcpy(pDst, pSrc, transferSize);
		pSrc += srcSize;
		pDst += dstSize;
	}

	return transferNum;
}

/*
bool DoTaskArray(INTERNAL_AUTOCIRCULATE_STRUCT* pAuto, AutoCircGenericTask* pTaskArray, ULWord numTasks)
{
	NTV2PrivateParams* pNTV2Params = NULL;
	ULWord i = 0;
	ULWord deviceNumber = 0;

	if((pAuto == NULL) || (pTaskArray == NULL) || (numTasks > AUTOCIRCULATE_TASK_MAX_TASKS))
	{
		return false;
	}

	deviceNumber = pAuto->deviceNumber;

	if(!(pNTV2Params = getNTV2Params(deviceNumber)) )
	{
		return false;
	}

	if(MsgsEnabled(NTV2_DRIVER_AUTOCIRCULATE_DEBUG_MESSAGES))
	{
		MSG("device %d dump %d tasks\n", deviceNumber, numTasks);
	}

	for(i = 0; i < numTasks; i++)
	{
		AutoCircGenericTask* entry = pTaskArray + i;
		switch(entry->taskType)
		{
		case eAutoCircTaskRegisterWrite:
			if(entry->u.registerTask.regNum < pNTV2Params->_BA0MemorySize/4)
			{
				WriteRegister(deviceNumber,
							  entry->u.registerTask.regNum,
							  entry->u.registerTask.value,
							  entry->u.registerTask.mask,
							  entry->u.registerTask.shift);

				if(MsgsEnabled(NTV2_DRIVER_AUTOCIRCULATE_DEBUG_MESSAGES))
				{
					MSG("device %3d task %3d register write - reg %3d  value %08x  mask %08x  shift %3d\n",
						deviceNumber,
						i,
						entry->u.registerTask.regNum,
						entry->u.registerTask.value,
						entry->u.registerTask.mask,
						entry->u.registerTask.shift);
				}
			}
			break;
		case eAutoCircTaskRegisterRead:
			if(entry->u.registerTask.regNum < pNTV2Params->_BA0MemorySize/4)
			{
				entry->u.registerTask.value = ReadRegister(deviceNumber,
														   entry->u.registerTask.regNum,
														   entry->u.registerTask.mask,
														   entry->u.registerTask.shift);

				if(MsgsEnabled(NTV2_DRIVER_AUTOCIRCULATE_DEBUG_MESSAGES))
				{
					MSG("device %3d task %3d register read  - reg %3d  value %08x  mask %08x  shift %3d\n",
						deviceNumber,
						i,
						entry->u.registerTask.regNum,
						entry->u.registerTask.value,
						entry->u.registerTask.mask,
						entry->u.registerTask.shift);
				}
			}
			break;
		case eAutoCircTaskTimeCodeWrite:
			CopyRP188StructToQueue(pAuto,
								   &entry->u.timeCodeTask.TCInOut1,		&entry->u.timeCodeTask.LTCEmbedded,
								   &entry->u.timeCodeTask.TCInOut2,		&entry->u.timeCodeTask.LTCEmbedded2,
								   &entry->u.timeCodeTask.TCInOut3,		&entry->u.timeCodeTask.LTCEmbedded3,
								   &entry->u.timeCodeTask.TCInOut4,		&entry->u.timeCodeTask.LTCEmbedded4,
								   &entry->u.timeCodeTask.TCInOut5,		&entry->u.timeCodeTask.LTCEmbedded5,
								   &entry->u.timeCodeTask.TCInOut6,		&entry->u.timeCodeTask.LTCEmbedded6,
								   &entry->u.timeCodeTask.TCInOut7,		&entry->u.timeCodeTask.LTCEmbedded7,
								   &entry->u.timeCodeTask.TCInOut8,		&entry->u.timeCodeTask.LTCEmbedded8,
								   &entry->u.timeCodeTask.LTCAnalog,	&entry->u.timeCodeTask.LTCAnalog2);
				if(MsgsEnabled(NTV2_DRIVER_AUTOCIRCULATE_DEBUG_MESSAGES))
				{
					MSG("device %3d task %3d timecode write\n10( %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x\n",
						deviceNumber,
						i,
						entry->u.timeCodeTask.TCInOut1.DBB,
						entry->u.timeCodeTask.TCInOut1.Low,
						entry->u.timeCodeTask.TCInOut1.High,
						entry->u.timeCodeTask.TCInOut2.DBB,
						entry->u.timeCodeTask.TCInOut2.Low,
						entry->u.timeCodeTask.TCInOut2.High,
						entry->u.timeCodeTask.LTCEmbedded.Low,
						entry->u.timeCodeTask.LTCEmbedded.High,
						entry->u.timeCodeTask.LTCAnalog.Low,
						entry->u.timeCodeTask.LTCAnalog.High,
						entry->u.timeCodeTask.LTCEmbedded2.Low,
						entry->u.timeCodeTask.LTCEmbedded2.High,
						entry->u.timeCodeTask.LTCAnalog2.Low,
						entry->u.timeCodeTask.LTCAnalog2.High,
						entry->u.timeCodeTask.TCInOut3.DBB,
						entry->u.timeCodeTask.TCInOut3.Low,
						entry->u.timeCodeTask.TCInOut3.High,
						entry->u.timeCodeTask.TCInOut4.DBB,
						entry->u.timeCodeTask.TCInOut4.Low,
						entry->u.timeCodeTask.TCInOut4.High,
						entry->u.timeCodeTask.TCInOut5.DBB,
						entry->u.timeCodeTask.TCInOut5.Low,
						entry->u.timeCodeTask.TCInOut5.High,
						entry->u.timeCodeTask.TCInOut6.DBB,
						entry->u.timeCodeTask.TCInOut6.Low,
						entry->u.timeCodeTask.TCInOut6.High,
						entry->u.timeCodeTask.TCInOut7.DBB,
						entry->u.timeCodeTask.TCInOut7.Low,
						entry->u.timeCodeTask.TCInOut7.High,
						entry->u.timeCodeTask.TCInOut8.DBB,
						entry->u.timeCodeTask.TCInOut8.Low,
						entry->u.timeCodeTask.TCInOut8.High);
				}
			break;
		case eAutoCircTaskTimeCodeRead:
			CopyRP188QueueToStruct(pAuto,
								   &entry->u.timeCodeTask.TCInOut1,		&entry->u.timeCodeTask.LTCEmbedded,
								   &entry->u.timeCodeTask.TCInOut2,		&entry->u.timeCodeTask.LTCEmbedded2,
								   &entry->u.timeCodeTask.TCInOut3,		&entry->u.timeCodeTask.LTCEmbedded3,
								   &entry->u.timeCodeTask.TCInOut4,		&entry->u.timeCodeTask.LTCEmbedded4,
								   &entry->u.timeCodeTask.TCInOut5,		&entry->u.timeCodeTask.LTCEmbedded5,
								   &entry->u.timeCodeTask.TCInOut6,		&entry->u.timeCodeTask.LTCEmbedded6,
								   &entry->u.timeCodeTask.TCInOut7,		&entry->u.timeCodeTask.LTCEmbedded7,
								   &entry->u.timeCodeTask.TCInOut8,		&entry->u.timeCodeTask.LTCEmbedded8,
								   &entry->u.timeCodeTask.LTCAnalog,	&entry->u.timeCodeTask.LTCAnalog2);
				if(MsgsEnabled(NTV2_DRIVER_AUTOCIRCULATE_DEBUG_MESSAGES))
				{
					MSG("CNTV2: device %3d task %3d timecode read\n10( %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x\n",
						deviceNumber,
						i,
						entry->u.timeCodeTask.TCInOut1.DBB,
						entry->u.timeCodeTask.TCInOut1.Low,
						entry->u.timeCodeTask.TCInOut1.High,
						entry->u.timeCodeTask.TCInOut2.DBB,
						entry->u.timeCodeTask.TCInOut2.Low,
						entry->u.timeCodeTask.TCInOut2.High,
						entry->u.timeCodeTask.LTCEmbedded.Low,
						entry->u.timeCodeTask.LTCEmbedded.High,
						entry->u.timeCodeTask.LTCAnalog.Low,
						entry->u.timeCodeTask.LTCAnalog.High,
						entry->u.timeCodeTask.LTCEmbedded2.Low,
						entry->u.timeCodeTask.LTCEmbedded2.High,
						entry->u.timeCodeTask.LTCAnalog2.Low,
						entry->u.timeCodeTask.LTCAnalog2.High,
						entry->u.timeCodeTask.TCInOut3.DBB,
						entry->u.timeCodeTask.TCInOut3.Low,
						entry->u.timeCodeTask.TCInOut3.High,
						entry->u.timeCodeTask.TCInOut4.DBB,
						entry->u.timeCodeTask.TCInOut4.Low,
						entry->u.timeCodeTask.TCInOut4.High,
						entry->u.timeCodeTask.TCInOut5.DBB,
						entry->u.timeCodeTask.TCInOut5.Low,
						entry->u.timeCodeTask.TCInOut5.High,
						entry->u.timeCodeTask.TCInOut6.DBB,
						entry->u.timeCodeTask.TCInOut6.Low,
						entry->u.timeCodeTask.TCInOut6.High,
						entry->u.timeCodeTask.TCInOut7.DBB,
						entry->u.timeCodeTask.TCInOut7.Low,
						entry->u.timeCodeTask.TCInOut7.High,
						entry->u.timeCodeTask.TCInOut8.DBB,
						entry->u.timeCodeTask.TCInOut8.Low,
						entry->u.timeCodeTask.TCInOut8.High);
				}
			break;
		default:
			MSG("device %d task %d error - unknown task type: %d\n", deviceNumber, i, entry->taskType);
			break;
		}
	}

	return true;
}
*/
