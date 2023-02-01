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
// Filename: hevcommand.c
// Purpose:	 Main module file.  Load, unload, fops, ioctls.
//
///////////////////////////////////////////////////////////////


#include "hevccommand.h"
#include "hevcregister.h"

#define HEVC_COMMAND_TIMEOUT	2000000  // 2 sec


// command queue send dpc
static void hevcCommandQueueDpc(Ntv2DpcData data);

// determine command queue level
static uint32_t hevcCommandQueueLevel(uint32_t devNum);


void hevcCommandInitialize(uint32_t devNum)
{
	HevcModuleParams* pModParams = hevcGetModuleParams();
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	uint32_t it;

	if(pDevParams == NULL) return;

	HEVC_MSG_COMMAND_INFO("%s(%d): hevcCommandInitialize()  max queue depth %d\n",
						  pModParams->pModuleName, devNum, HEVC_COMMAND_TASK_MAX);

	// clear tasks and queue pointers
	memset(pDevParams->cmdQueue, 0, sizeof(pDevParams->cmdQueue));
	ntv2SpinLockOpen(&pDevParams->cmdQueueLock, &pDevParams->systemContext);
	pDevParams->cmdQueueReady = 0;
	pDevParams->cmdQueueSend = 0;

	// initialize command queue
	for(it = 0; it < HEVC_COMMAND_TASK_MAX; it++)
	{
		pDevParams->cmdQueue[it].taskNum = it;
		ntv2EventOpen(&pDevParams->cmdQueue[it].ackEvent, &pDevParams->systemContext);
		ntv2EventOpen(&pDevParams->cmdQueue[it].msgEvent, &pDevParams->systemContext);
	}

	// initialize command queue send dpc
	ntv2DpcOpen(&pDevParams->cmdQueueDpc,
				&pDevParams->systemContext,
				hevcCommandQueueDpc,
				devNum);
}

void hevcCommandRelease(uint32_t devNum)
{
	HevcModuleParams* pModParams = hevcGetModuleParams();
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	uint32_t it;

	if(pDevParams == NULL) return;

	HEVC_MSG_COMMAND_INFO("%s(%d): hevcCommandRelease()\n",
						  pModParams->pModuleName, devNum);

	// kill command queue send dpc
	ntv2DpcClose(&pDevParams->cmdQueueDpc);

	// wake up all waiting threads
	for(it = 0; it < HEVC_COMMAND_TASK_MAX; it++)
	{
		pDevParams->cmdQueue[it].cmdReady = false;
		pDevParams->cmdQueue[it].ackReceived = true;
		pDevParams->cmdQueue[it].msgReceived = true;
		ntv2EventClose(&pDevParams->cmdQueue[it].ackEvent);
		ntv2EventClose(&pDevParams->cmdQueue[it].msgEvent);
	}

	ntv2SpinLockClose(&pDevParams->cmdQueueLock);
}

void hevcCommandReset(uint32_t devNum)
{
	HevcModuleParams* pModParams = hevcGetModuleParams();
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	uint32_t it;

	if(pDevParams == NULL) return;

	HEVC_MSG_COMMAND_INFO("%s(%d): hevcCommandReset() reset command queue\n",
						  pModParams->pModuleName, devNum);

	ntv2SpinLockAcquire(&pDevParams->cmdQueueLock);

	// reset all the command tasks
	for(it = 0; it < HEVC_COMMAND_TASK_MAX; it++)
	{
		pDevParams->cmdQueue[it].cmdReady = false;
		pDevParams->cmdQueue[it].cmdSent = true;
		pDevParams->cmdQueue[it].ackReceived = true;
		pDevParams->cmdQueue[it].msgReceived = true;
	}

	// reset queue pointers and stats
	pDevParams->cmdQueueReady = 0;
	pDevParams->cmdQueueSend = 0;
	pDevParams->cmdQueueCount = 0;
	pDevParams->cmdQueueLevel = 0;

	ntv2SpinLockRelease(&pDevParams->cmdQueueLock);
}

HevcCommandTask* hevcCommandEnqueue(uint32_t devNum, HevcCommandInfo* pCmdInfo)
{
	HevcModuleParams* pModParams = hevcGetModuleParams();
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	uint32_t taskNum;
	HevcCommandTask* pCmdTask = NULL;

	if((pDevParams == NULL) ||
	   (pCmdInfo == NULL)) return NULL;

	ntv2SpinLockAcquire(&pDevParams->cmdQueueLock);

	// queue the task to the next ready entry
	taskNum = pDevParams->cmdQueueReady;
	if(!pDevParams->cmdQueue[taskNum].cmdReady)
	{
		// copy command info and set status flags
		pCmdTask = &pDevParams->cmdQueue[taskNum];
		pCmdTask->cmdReady = true;
		pCmdTask->cmdSent = false;
		pCmdTask->ackReceived = false;
		pCmdTask->msgReceived = false;
		pCmdTask->cmdInfo = *pCmdInfo;
		memset(&pCmdTask->ackInfo, 0, sizeof(HevcCommandAckInfo));
		memset(&pCmdTask->msgInfo, 0, sizeof(HevcCommandMsgInfo));
		ntv2EventClear(&pCmdTask->ackEvent);
		ntv2EventClear(&pCmdTask->msgEvent);

		// increment the ready point and update stats
		pDevParams->cmdQueueReady = (pDevParams->cmdQueueReady + 1)%HEVC_COMMAND_TASK_MAX;
		pDevParams->cmdQueueCount++;
		pDevParams->cmdQueueLevel = hevcCommandQueueLevel(devNum);
	}

	ntv2SpinLockRelease(&pDevParams->cmdQueueLock);

	if(pCmdTask != NULL)
	{
		HEVC_MSG_COMMAND_STATE("%s(%d): hevcCommandEnqueue()  queue task %d  target 0x%08x  id 0x%08x  param 0x%08x 0x%08x\n",
							   pModParams->pModuleName, devNum, taskNum, pCmdInfo->target,
							   pCmdInfo->id, pCmdInfo->param[0], pCmdInfo->param[1]);
	}
	else
	{
		HEVC_MSG_COMMAND_ERROR("%s(%d): hevcCommandEnqueue()  *error* command queue is full\n",
							   pModParams->pModuleName, devNum);
	}

	// try to send queued commands
	ntv2DpcSchedule(&pDevParams->cmdQueueDpc);

	return pCmdTask;
}

bool hevcCommandAckWait(uint32_t devNum, HevcCommandTask* pCmdTask)
{
	HevcModuleParams* pModParams = hevcGetModuleParams();
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	bool success;

	if((pDevParams == NULL) ||
	   (pCmdTask == NULL)) return false;

	success = ntv2EventWaitForSignal(&pCmdTask->ackEvent, HEVC_COMMAND_TIMEOUT, true);
	if(!success)
	{
		HEVC_MSG_COMMAND_ERROR("%s(%d): hevcCommandAckWait()  *error* task %d ack wait timout\n",
							   pModParams->pModuleName, devNum, pCmdTask->taskNum);
	}

	return success;
}

bool hevcCommandMsgWait(uint32_t devNum, HevcCommandTask* pCmdTask)
{
	HevcModuleParams* pModParams = hevcGetModuleParams();
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	bool success;

	if((pDevParams == NULL) ||
	   (pCmdTask == NULL)) return false;

	success = ntv2EventWaitForSignal(&pCmdTask->msgEvent, HEVC_COMMAND_TIMEOUT, true);
	if(!success)
	{
		HEVC_MSG_COMMAND_ERROR("%s(%d): hevcCommandMsgWait()  *error* task %d msg wait timout\n",
							   pModParams->pModuleName, devNum, pCmdTask->taskNum);
	}

	return success;
}

bool hevcCommandDequeue(uint32_t devNum, HevcCommandTask* pCmdTask)
{
	HevcModuleParams* pModParams = hevcGetModuleParams();
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	bool result = false;
	uint32_t target;
	uint32_t id;
	uint32_t param0;
	uint32_t param1;
	
	if((pDevParams == NULL) ||
	   (pCmdTask == NULL)) return false;

	ntv2SpinLockAcquire(&pDevParams->cmdQueueLock);

	// is there queued command?
	result = pCmdTask->cmdReady;

	// clear state
	pCmdTask->cmdReady = false;
	pCmdTask->cmdSent = true;
	pCmdTask->ackReceived = true;
	pCmdTask->msgReceived = true;

	// compute stats
	pDevParams->cmdQueueLevel = hevcCommandQueueLevel(devNum);

	// save info for report
	target = pCmdTask->cmdInfo.target;
	id = pCmdTask->cmdInfo.id;
	param0 = pCmdTask->cmdInfo.param[0];
	param1 = pCmdTask->cmdInfo.param[1];

	ntv2SpinLockRelease(&pDevParams->cmdQueueLock);

	if(result)
	{
		HEVC_MSG_COMMAND_STATE("%s(%d): hevcCommandDequeue()  dequeue task %d  target 0x%08x  id 0x%08x  param 0x%08x 0x%08x\n",
							   pModParams->pModuleName, devNum, pCmdTask->taskNum, 
							   target, id, param0, param1);
	}
	else
	{
		HEVC_MSG_COMMAND_ERROR("%s(%d): hevcCommandDequeue()  *error* task %d not ready\n",
							   pModParams->pModuleName, devNum, pCmdTask->taskNum);
	}

	return result;
}

void hevcCommandAck(uint32_t devNum, HevcCommandAckInfo* pAckInfo)
{
	HevcModuleParams* pModParams = hevcGetModuleParams();
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	uint32_t it;

	if((pDevParams == NULL) ||
	   (pAckInfo == NULL)) return;

	ntv2SpinLockAcquire(&pDevParams->cmdQueueLock);

	// search for a matching command task
	for(it = 0; it < HEVC_COMMAND_TASK_MAX; it++)
	{
		if(pDevParams->cmdQueue[it].cntCount == pAckInfo->cntCount)
		{
			if(pDevParams->cmdQueue[it].cmdReady &&
			   pDevParams->cmdQueue[it].cmdSent)
			{
				// copy ack info to task and set received status
				pDevParams->cmdQueue[it].ackInfo = *pAckInfo;
				pDevParams->cmdQueue[it].ackReceived = true;
				ntv2EventSignal(&pDevParams->cmdQueue[it].ackEvent);
				break;
			}
		}
	}

	ntv2SpinLockRelease(&pDevParams->cmdQueueLock);

	if(it < HEVC_COMMAND_TASK_MAX)
	{ 
		// wake up those who wait
		HEVC_MSG_COMMAND_STATE("%s(%d): hevcCommandAck()  ack received task %d  cnt %d  target 0x%08x  id 0x%08x  result 0x%08x\n",
							   pModParams->pModuleName, devNum, it,
							   pAckInfo->cntCount, pAckInfo->target, pAckInfo->id,
							   pAckInfo->result0);
	}
	else
	{
		HEVC_MSG_COMMAND_ERROR("%s(%d): hevcCommandAck()  *error* task not found  cnt %d  target 0x%08x  id 0x%08x  result 0x%08x\n",
							   pModParams->pModuleName, devNum,
							   pAckInfo->cntCount, pAckInfo->target, pAckInfo->id,
							   pAckInfo->result0);
	}

	// try to send queued commands
	ntv2DpcSchedule(&pDevParams->cmdQueueDpc);
}

void hevcCommandMsg(uint32_t devNum, HevcCommandMsgInfo* pMsgInfo)
{
	HevcModuleParams* pModParams = hevcGetModuleParams();
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	uint32_t it;

	if((pDevParams == NULL) ||
	   (pMsgInfo == NULL)) return;

	ntv2SpinLockAcquire(&pDevParams->cmdQueueLock);

	// search for a matching command task
	for(it = 0; it < HEVC_COMMAND_TASK_MAX; it++)
	{
		if(pDevParams->cmdQueue[it].cntCount == pMsgInfo->cntCount)
		{
			if(pDevParams->cmdQueue[it].cmdReady &&
			   pDevParams->cmdQueue[it].cmdSent)
			{
				// copy msg info to task and set received status
				pDevParams->cmdQueue[it].msgInfo = *pMsgInfo;
				pDevParams->cmdQueue[it].msgReceived = true;
				ntv2EventSignal(&pDevParams->cmdQueue[it].msgEvent);
				break;
			}
		}
	}

	ntv2SpinLockRelease(&pDevParams->cmdQueueLock);

	if(it < HEVC_COMMAND_TASK_MAX)
	{
		// wake up those who wait
		HEVC_MSG_COMMAND_STATE("%s(%d): hevcCommandMsg()  msg received task %d  cnt %d  target 0x%08x  id 0x%08x  result 0x%08x 0x%08x 0x%08x 0x%08x\n",
							   pModParams->pModuleName, devNum, it,
							   pMsgInfo->cntCount, pMsgInfo->target, pMsgInfo->id,
							   pMsgInfo->result[0], pMsgInfo->result[1], pMsgInfo->result[2], pMsgInfo->result[3]);
	}
	else
	{
		HEVC_MSG_COMMAND_ERROR("%s(%d): hevcCommandMsg()  *error* task not found  cnt %d  target 0x%08x  id 0x%08x  result 0x%08x 0x%08x 0x%08x 0x%08x\n",
							   pModParams->pModuleName, devNum,
							   pMsgInfo->cntCount, pMsgInfo->target, pMsgInfo->id,
							   pMsgInfo->result[0], pMsgInfo->result[1], pMsgInfo->result[2], pMsgInfo->result[3]);
	}

	// try to send queued commands
	ntv2DpcSchedule(&pDevParams->cmdQueueDpc);
}

static void hevcCommandQueueDpc(unsigned long data)
{
	HevcModuleParams* pModParams = hevcGetModuleParams();
	uint32_t devNum = (uint32_t)data;
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	HevcCommandInfo cmdInfo;
	HevcCommandTask* pCmdTask;
	uint32_t taskNum;
	bool send = false;

	if(pDevParams == NULL) return;

	ntv2SpinLockAcquire(&pDevParams->cmdQueueLock);

	// skip commands already sent
	taskNum = pDevParams->cmdQueueSend;
	while(taskNum != pDevParams->cmdQueueReady)
	{
		if(!pDevParams->cmdQueue[taskNum].cmdSent ||
		   !pDevParams->cmdQueue[taskNum].msgReceived)
		{
			break;
		}
		taskNum = (taskNum + 1)%HEVC_COMMAND_TASK_MAX;
	}

	// check to see if command can be sent
	pCmdTask = &pDevParams->cmdQueue[taskNum];
	if(pCmdTask->cmdReady && !pCmdTask->cmdSent)
	{
		// copy command info
		cmdInfo = pCmdTask->cmdInfo;
		// mark task as command sent
		pCmdTask->cmdSent = true;
		// update send pointer
		pDevParams->cmdQueueSend = taskNum;
		// send this command
		send = true;
	}

	ntv2SpinLockRelease(&pDevParams->cmdQueueLock);

	if(send)
	{
		// setup command and save continuity count
		send = hevcSetupCommand(devNum, &cmdInfo, &pCmdTask->cntCount);
		if(send)
		{
			// submit command
			hevcSubmitCommand(devNum);

			HEVC_MSG_COMMAND_STATE("%s(%d): hevcCommandSend()  send command task %d  cnt %d  target 0x%08x  id 0x%08x  param 0x%08x 0x%08x %08x %08x %08x %08x\n",
								   pModParams->pModuleName, devNum, taskNum, pCmdTask->cntCount,
								   cmdInfo.target, cmdInfo.id,
								   cmdInfo.param[0], cmdInfo.param[1], cmdInfo.param[2],
								   cmdInfo.param[3], cmdInfo.param[4], cmdInfo.param[5]);
		}
		else
		{
			HEVC_MSG_COMMAND_ERROR("%s(%d): hevcCommandSend()  *error* send failed  command task %d  cnt %d  target 0x%08x  id 0x%08x  param 0x%08x 0x%08x\n",
								   pModParams->pModuleName, devNum, taskNum, pCmdTask->cntCount,
								   cmdInfo.target, cmdInfo.id, cmdInfo.param[0], cmdInfo.param[1]);
		}
	}

	return;
}

static uint32_t hevcCommandQueueLevel(uint32_t devNum)
{
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	uint32_t level = 0;
	uint32_t i;

	if(pDevParams == NULL) return 0;

	// count active commands in queue
	for(i = 0; i < HEVC_COMMAND_TASK_MAX; i++)
	{
		if(pDevParams->cmdQueue[i].cmdReady) level++;
	}

	return level;
}

