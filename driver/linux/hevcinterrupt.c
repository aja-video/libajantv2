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
// Filename: hevcinterrupt.c
// Purpose:	 HEVC interrupt service routines
//
///////////////////////////////////////////////////////////////

#include "hevcinterrupt.h"
#include "hevcregister.h"
#include "hevccommand.h"
#include "hevcstream.h"


// interrupt dpc tasks
static void hevcCmdAckDpc(unsigned long data);
static void hevcCmdResultDpc(unsigned long data);
static void hevcCodecMessageDpc(unsigned long data);
static void hevcVeiAckDpc(unsigned long data);
static void hevcVeiCompleteDpc(unsigned long data);
static void hevcSeoAckDpc(unsigned long data);
static void hevcSeoCompleteDpc(unsigned long data);
static void hevcCodecMsg(uint32_t devNum, HevcCodecMsgInfo* pMsgInfo);


void hevcInterruptInitialize(uint32_t devNum)
{
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);

	if(pDevParams == NULL) return;

	// get interrupt registers
	pDevParams->intRegStatus = pDevParams->bar0Base + (FMB_REG_HSIO_PCIE1_INT_REG_STATUS & FMB_PCI_BAR0_OFFSET_ADDR_MASK);
	pDevParams->intRegClear = pDevParams->bar0Base + (FMB_REG_HSIO_PCIE1_INT_REG_CLR & FMB_PCI_BAR0_OFFSET_ADDR_MASK);

	// initialize dpc for each interrupt type
	ntv2DpcOpen(&pDevParams->irqDpc[FMB_FACT_ID_CMD_ACK], &pDevParams->systemContext, hevcCmdAckDpc, devNum);
	ntv2DpcOpen(&pDevParams->irqDpc[FMB_FACT_ID_CMD_RESULT], &pDevParams->systemContext, hevcCmdResultDpc, devNum);
	ntv2DpcOpen(&pDevParams->irqDpc[FMB_FACT_ID_MESSAGE], &pDevParams->systemContext, hevcCodecMessageDpc, devNum);
	ntv2DpcOpen(&pDevParams->irqDpc[FMB_FACT_ID_DMA_VEI_ACK], &pDevParams->systemContext, hevcVeiAckDpc, devNum);
	ntv2DpcOpen(&pDevParams->irqDpc[FMB_FACT_ID_DMA_VEI_COMPLETE], &pDevParams->systemContext, hevcVeiCompleteDpc, devNum);
	ntv2DpcOpen(&pDevParams->irqDpc[FMB_FACT_ID_DMA_SEO_ACK], &pDevParams->systemContext, hevcSeoAckDpc, devNum);
	ntv2DpcOpen(&pDevParams->irqDpc[FMB_FACT_ID_DMA_SEO_COMPLETE], &pDevParams->systemContext, hevcSeoCompleteDpc, devNum);
}

void hevcInterruptRelease(uint32_t devNum)
{
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);

	if(pDevParams == NULL) return;

	// kill all dpcs
	ntv2DpcClose(&pDevParams->irqDpc[FMB_FACT_ID_CMD_ACK]);
	ntv2DpcClose(&pDevParams->irqDpc[FMB_FACT_ID_CMD_RESULT]);
	ntv2DpcClose(&pDevParams->irqDpc[FMB_FACT_ID_MESSAGE]);
	ntv2DpcClose(&pDevParams->irqDpc[FMB_FACT_ID_DMA_VEI_ACK]);
	ntv2DpcClose(&pDevParams->irqDpc[FMB_FACT_ID_DMA_VEI_COMPLETE]);
	ntv2DpcClose(&pDevParams->irqDpc[FMB_FACT_ID_DMA_SEO_ACK]);
	ntv2DpcClose(&pDevParams->irqDpc[FMB_FACT_ID_DMA_SEO_COMPLETE]);
}

bool hevcInterrupt(uint32_t devNum)
{
	HevcModuleParams* pModParams = hevcGetModuleParams();
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	uint32_t intStatus;
	uint32_t clrStatus;
	uint32_t intBit;
	enum fmb_factor_id factorID;
	int i;

	// hevc primary interrupt routine
	if(pDevParams == NULL) return false;

	// lock register access
	ntv2InterruptLockAcquire(&pDevParams->regAccessLock);

	// get interrupt status
	intStatus = ntv2ReadRegister32(pDevParams->intRegStatus);

	// clear interrupt
	ntv2WriteRegister32(pDevParams->intRegClear, intStatus);

	// wait until interrupt clear complete
	for(i = 0; i < FMB_PCI_POLL_COUNT_REG_WRITE_COMPLETE; i++) 
	{
		clrStatus = ntv2ReadRegister32(pDevParams->intRegStatus);
		if((clrStatus & intStatus) == 0) break;
	}

	// release register lock
	ntv2InterruptLockRelease(&pDevParams->regAccessLock);

	// check for timeout
	if(i >= FMB_PCI_POLL_COUNT_REG_WRITE_COMPLETE)
	{
		HEVC_MSG_INT_ERROR("%s(%d): hevcInterrupt() *error*  clear interrupt status timeout\n",
						   pModParams->pModuleName, devNum);
	}

	HEVC_MSG_INT_PRIMARY("%s(%d): hevcInterrupt()  pci interrupt status 0x%08x\n",
						 pModParams->pModuleName, devNum, intStatus);

	// schedule the dpc
	for(factorID = 0; factorID < FMB_FACT_IRQ_MAX; factorID++)
	{
		intBit = (1U << factorID);
		switch(factorID)
		{
		case FMB_FACT_ID_CMD_ACK:
		case FMB_FACT_ID_CMD_RESULT:
		case FMB_FACT_ID_MESSAGE:
		case FMB_FACT_ID_DMA_VEI_ACK:
		case FMB_FACT_ID_DMA_VEI_COMPLETE:
		case FMB_FACT_ID_DMA_SEO_ACK:
		case FMB_FACT_ID_DMA_SEO_COMPLETE:
			if(intStatus & intBit)
			{
				ntv2DpcSchedule(&pDevParams->irqDpc[factorID]);
			}
			break;

		case FMB_FACT_ID_FATAL_ERROR:
			if(intStatus & intBit)
			{
				HEVC_MSG_INT_ERROR("%s(%d): hevcInterrupt() *error*  pci interrupt hardware error\n",
								   pModParams->pModuleName, devNum);
			}
			break;
			
		default:
			if(intStatus & intBit)
			{
				HEVC_MSG_INT_ERROR("%s(%d): hevcInterrupt() *error*  pci interrupt unknown %08x\n",
								   pModParams->pModuleName, devNum, intBit);
			}
			break;
		}
	}

	return true;
}

static void hevcCmdAckDpc(unsigned long data)
{
	HevcModuleParams* pModParams = hevcGetModuleParams();
	uint32_t devNum = (uint32_t)data;
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	HevcCommandAckInfo ackInfo;

	if(pDevParams == NULL) return;

	// read ack results from codec
	ackInfo.cntCount	= hevcRegRead(devNum, FMB_REG_COMMAND_ACK_CONTINUITY_CNT);
	ackInfo.target		= hevcRegRead(devNum, FMB_REG_COMMAND_ACK_TARGET);
	ackInfo.id			= hevcRegRead(devNum, FMB_REG_COMMAND_ACK_ID);
	ackInfo.result0		= hevcRegRead(devNum, FMB_REG_COMMAND_ACK_RESULT);

	// record ack results
	hevcCommandAck(devNum, &ackInfo);

	HEVC_MSG_INT_COMMAND("%s(%d): command ack interrupt cnt %d\n",
						 pModParams->pModuleName, devNum, ackInfo.cntCount);

	return;
}

static void hevcCmdResultDpc(unsigned long data)
{
	HevcModuleParams* pModParams = hevcGetModuleParams();
	uint32_t devNum = (uint32_t)data;
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	HevcCommandMsgInfo msgInfo;

	if(pDevParams == NULL) return;

	// read msg results from codec
	msgInfo.cntCount	= hevcRegRead(devNum, FMB_REG_COMMAND_RESULT_CONTINUITY_CNT);
	msgInfo.target		= hevcRegRead(devNum, FMB_REG_COMMAND_RESULT_TARGET);
	msgInfo.id			= hevcRegRead(devNum, FMB_REG_COMMAND_RESULT_ID);
	hevcRegReadMultiple(devNum, FMB_REG_COMMAND_RESULT_PARAM_BASE, FMB_REG_COMMAND_RESULT_PARAM_MAX, msgInfo.result);

	// ack msg
	hevcRegWrite(devNum, FMB_REG_HSIO_PCIE0_INT_REG_SET, FMB_REG_HSIO_PCIE0_INT_REG_CMD_RESULT_ACK);

	// record msg results
	hevcCommandMsg(devNum, &msgInfo);

	HEVC_MSG_INT_COMMAND("%s(%d): command msg interrupt cnt %d\n",
						 pModParams->pModuleName, devNum, msgInfo.cntCount);

	return;
}

static void hevcCodecMessageDpc(unsigned long data)
{
	int devNum = data;
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	HevcCodecMsgInfo msgInfo;

	if(pDevParams == NULL) return;

	// read message from codec
	msgInfo.target = hevcRegRead(devNum, FMB_REG_MESSAGE_RESULT_TARGET);
	msgInfo.id = hevcRegRead(devNum, FMB_REG_MESSAGE_RESULT_ID);
	hevcRegReadMultiple(devNum, FMB_REG_MESSAGE_RESULT_PARAM_BASE, FMB_REG_MESSAGE_RESULT_PARAM_MAX, msgInfo.params);

	// ack message
	hevcRegWrite(devNum, FMB_REG_HSIO_PCIE0_INT_REG_SET, FMB_REG_HSIO_PCIE0_INT_REG_MESSAGE_ACK);

	// record message
	hevcCodecMsg(devNum, &msgInfo);

	return;
}

static void hevcVeiAckDpc(unsigned long data)
{
	int devNum = data;
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	HevcStreamAckInfo ackInfo;

	if(pDevParams == NULL) return;

	// read ack results from codec
	ackInfo.streamType = FMB_STREAM_TYPE_VEI;
	ackInfo.cntCount = hevcRegRead(devNum, FMB_REG_DMA_VEI_ACK_CONTINUITY_CNT);
	ackInfo.streamId = hevcRegRead(devNum, FMB_REG_DMA_VEI_ACK_DATA_ID);
	ackInfo.result0 = hevcRegRead(devNum, FMB_REG_DMA_VEI_ACK_RESULT);

	// record ack results
	hevcStreamAck(devNum, &ackInfo);

	return;
}

static void hevcVeiCompleteDpc(unsigned long data)
{
	int devNum = data;
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	HevcStreamMsgInfo msgInfo;

	if(pDevParams == NULL) return;

	// read msg results from codec
	msgInfo.streamType = FMB_STREAM_TYPE_VEI;
	msgInfo.cntCount = hevcRegRead(devNum, FMB_REG_DMA_VEI_COMPLETE_CONTINUITY_CNT);
	msgInfo.streamId = hevcRegRead(devNum, FMB_REG_DMA_VEI_COMPLETE_DATA_ID);
	msgInfo.result0 = hevcRegRead(devNum, FMB_REG_DMA_VEI_COMPLETE_RESULT);
	msgInfo.videoDataSize = 0;
	msgInfo.infoDataSize = 0;
	if(msgInfo.result0 == FMB_DMA_RESULT_OK)
	{
		msgInfo.videoDataSize = hevcRegRead(devNum, FMB_REG_DMA_VEI_COMPLETE_TOTAL_SIZE);
		msgInfo.infoDataSize = 0; // no complete pic size available
	}
	msgInfo.lastFrame = 0;

	// ack msg
	hevcRegWrite(devNum, FMB_REG_HSIO_PCIE0_INT_REG_SET, FMB_REG_HSIO_PCIE0_INT_REG_DMA_VEI_COMPLETE_ACK);

	// record msg results
	hevcStreamMsg(devNum, &msgInfo);

	return;
}

static void hevcSeoAckDpc(unsigned long data)
{
	int devNum = data;
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	HevcStreamAckInfo ackInfo;

	if(pDevParams == NULL) return;

	// read ack results from codec
	ackInfo.streamType = FMB_STREAM_TYPE_SEO;
	ackInfo.cntCount = hevcRegRead(devNum, FMB_REG_DMA_SEO_ACK_CONTINUITY_CNT);
	ackInfo.streamId = hevcRegRead(devNum, FMB_REG_DMA_SEO_ACK_DATA_ID);
	ackInfo.result0 = hevcRegRead(devNum, FMB_REG_DMA_SEO_ACK_RESULT);

	// record ack results
	hevcStreamAck(devNum, &ackInfo);

	return;
}

static void hevcSeoCompleteDpc(unsigned long data)
{
	int devNum = data;
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	HevcStreamMsgInfo msgInfo;

	if(pDevParams == NULL) return;

	// read msg results from codec
	msgInfo.streamType = FMB_STREAM_TYPE_SEO;
	msgInfo.cntCount = hevcRegRead(devNum, FMB_REG_DMA_SEO_COMPLETE_CONTINUITY_CNT);
	msgInfo.streamId = hevcRegRead(devNum, FMB_REG_DMA_SEO_COMPLETE_DATA_ID);
	msgInfo.result0 = hevcRegRead(devNum, FMB_REG_DMA_SEO_COMPLETE_RESULT);
	msgInfo.videoDataSize = 0;
	msgInfo.infoDataSize = 0;
	if(msgInfo.result0 == FMB_DMA_RESULT_OK)
	{
		msgInfo.videoDataSize = hevcRegRead(devNum, FMB_REG_DMA_SEO_COMPLETE_TOTAL_SIZE);
		msgInfo.infoDataSize = hevcRegRead(devNum, FMB_REG_DMA_SEO_COMPLETE_ES_INFO_TOTAL_SIZE);
	}
	msgInfo.lastFrame = hevcRegRead(devNum, FMB_REG_DMA_SEO_COMPLETE_LAST_MARKER);

	// ack msg
	hevcRegWrite(devNum, FMB_REG_HSIO_PCIE0_INT_REG_SET, FMB_REG_HSIO_PCIE0_INT_REG_DMA_SEO_COMPLETE_ACK);

	// record msg results
	hevcStreamMsg(devNum, &msgInfo);

	return;
}

void hevcCodecMsg(uint32_t devNum, HevcCodecMsgInfo* pMsgInfo)
{
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	HevcFrameData frameData;
	uint32_t target;
	uint32_t id;
	uint32_t param0;

	if((pDevParams == NULL) ||
	   (pMsgInfo == NULL)) return;

	target = pMsgInfo->target;
	id = pMsgInfo->id;
	param0 = pMsgInfo->params[0];

	switch(target)
	{
	case FMB_MESSAGE_TARGET_VALUE_VI:
		switch(id)
		{
		case FMB_MESSAGE_ID_CAPTURE_START:
			if(param0 < HEVC_STREAM_MAX)
			{
				pDevParams->vifState[param0] = Hevc_VifState_Start;
				memset(&frameData, 0, sizeof(frameData));
				frameData.streamId = param0;
				frameData.syncCount = pMsgInfo->params[1];
				frameData.itcValueLow = pMsgInfo->params[2];
				frameData.itcValueHigh = pMsgInfo->params[3];
				frameData.itcExtension = pMsgInfo->params[4];
				frameData.encodeTime = ntv2Time100ns();
				hevcStreamFrameReady(devNum, &frameData);
				hevcStreamTransferStats(devNum, FMB_STREAM_TYPE_VEI, frameData.streamId, 0);
			}
			break;

		case FMB_MESSAGE_ID_VSYNC:
			break;

		default:
			break;
		}
		break;

	case FMB_MESSAGE_TARGET_VALUE_EH:
		switch(id)
		{
		case FMB_MESSAGE_ID_ENCODE_ADDITIONAL_INFO:
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
	
	return;
}


