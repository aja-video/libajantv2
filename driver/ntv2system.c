/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
//////////////////////////////////////////////////////////////
//
// NTV2 Linux v2.6+ Device Driver for AJA OEM boards.
//
// Filename: ntv2system.c
// Purpose:	 NTV2 system function abstraction
//
///////////////////////////////////////////////////////////////

#include "ntv2system.h"

extern uint32_t ntv2ReadRegCon32(Ntv2SystemContext* context, uint32_t regNum);
extern bool ntv2ReadRegMSCon32(Ntv2SystemContext* context, uint32_t regNum, uint32_t* regValue, uint32_t regMask, uint32_t regShift);
extern bool ntv2WriteRegCon32(Ntv2SystemContext* context, uint32_t regNum, uint32_t regValue);
extern bool ntv2WriteRegMSCon32(Ntv2SystemContext* context, uint32_t regNum, uint32_t regValue, uint32_t regMask, uint32_t regShift);
extern uint32_t ntv2ReadVirtRegCon32(Ntv2SystemContext* context, uint32_t regNum);
extern bool ntv2WriteVirtRegCon32(Ntv2SystemContext* context, uint32_t regNum, uint32_t data);

uint32_t ntv2ReadRegister(Ntv2SystemContext* context, uint32_t regnum)
{
	return ntv2ReadRegCon32(context, regnum);
}

bool ntv2ReadRegisterMS(Ntv2SystemContext* context, uint32_t regNum, uint32_t* regValue, uint32_t regMask, uint32_t regShift)
{
	return ntv2ReadRegMSCon32(context, regNum, regValue, regMask, regShift);
}

bool ntv2WriteRegister(Ntv2SystemContext* context, uint32_t regnum, uint32_t data)
{
	return ntv2WriteRegCon32(context, regnum, data);
}

bool ntv2WriteRegisterMS(Ntv2SystemContext* context, uint32_t regNum, uint32_t data, uint32_t regMask, uint32_t regShift)
{
	return ntv2WriteRegMSCon32(context, regNum, data, regMask, regShift);
}

uint32_t ntv2ReadVirtualRegister(Ntv2SystemContext* context, uint32_t regNum)
{
	return ntv2ReadVirtRegCon32(context, regNum);
}

bool ntv2WriteVirtualRegister(Ntv2SystemContext* context, uint32_t regNum, uint32_t data)
{
	return ntv2WriteVirtRegCon32(context, regNum, data);
}


#if defined (AJAVirtual)

#ifdef AJALinux
#include <unistd.h>
#endif

// virtual spinlock functions

bool ntv2SpinLockOpen(Ntv2SpinLock* pSpinLock, Ntv2SystemContext* pSysCon)
{
	if((pSpinLock == NULL) ||
	   (pSysCon == NULL)) return false;

	return false;
}

void ntv2SpinLockClose(Ntv2SpinLock* pSpinLock)
{
	if(pSpinLock == NULL) return;
}

void ntv2SpinLockAcquire(Ntv2SpinLock* pSpinLock)
{
	if(pSpinLock == NULL) return;
}

void ntv2SpinLockRelease(Ntv2SpinLock* pSpinLock)
{
	if(pSpinLock == NULL) return;
}

// virtual interrupt lock fucntions

bool ntv2InterruptLockOpen(Ntv2InterruptLock* pInterruptLock, Ntv2SystemContext* pSysCon)
{
	if((pInterruptLock == NULL) ||
	   (pSysCon == NULL)) return false;

	return false;
}

void ntv2InterruptLockClose(Ntv2InterruptLock* pInterruptLock)
{
	if(pInterruptLock == NULL) return;
}

void ntv2InterruptLockAcquire(Ntv2InterruptLock* pInterruptLock)
{
	if(pInterruptLock == NULL) return;
}

void ntv2InterruptLockRelease(Ntv2InterruptLock* pInterruptLock)
{
	if(pInterruptLock == NULL) return;
}

// virtual memory functions

void* ntv2MemoryAlloc(uint32_t size)
{
	if(size == 0) return NULL;

	return malloc(size);
}

void ntv2MemoryFree(void* pAddress, uint32_t size)
{
	if(pAddress == NULL) return;

    free(pAddress);
}

bool ntv2DmaMemoryAlloc(Ntv2DmaMemory* pDmaMemory, Ntv2SystemContext* pSysCon, uint32_t size)
{
	void* pAddress = NULL;
	Ntv2DmaAddress dmaAddress = 0;

	if((pDmaMemory == NULL) ||
	   (pSysCon == NULL) ||
	   (size == 0)) return false;

	return false;
}

void ntv2DmaMemoryFree(Ntv2DmaMemory* pDmaMemory)
{
	if((pDmaMemory == NULL) ||
	   (pDmaMemory->pAddress == NULL) ||
	   (pDmaMemory->dmaAddress == 0) ||
	   (pDmaMemory->size == 0)) return;
}

void* ntv2DmaMemoryVirtual(Ntv2DmaMemory* pDmaMemory)
{
	if(pDmaMemory == NULL) return NULL;

	return NULL;
}

Ntv2DmaAddress ntv2DmaMemoryPhysical(Ntv2DmaMemory* pDmaMemory)
{
	if(pDmaMemory == NULL) return 0;

	return (Ntv2DmaAddress)0;
}

uint32_t ntv2DmaMemorySize(Ntv2DmaMemory* pDmaMemory)
{
	if(pDmaMemory == NULL) return 0;

	return 0;
}

// virtual user buffer functions

bool ntv2UserBufferPrepare(Ntv2UserBuffer* pUserBuffer, Ntv2SystemContext* pSysCon,
						   uint64_t address, uint32_t size, bool write)
{
	if((pUserBuffer == NULL) ||
	   (pSysCon == NULL) ||
	   (address == 0) ||
	   (size == 0)) return false;

	memset(pUserBuffer, 0, sizeof(Ntv2UserBuffer));

	pUserBuffer->pAddress = (void*)((uintptr_t)(address));
	pUserBuffer->size = size;
	pUserBuffer->write = write;

	return true;
}

void ntv2UserBufferRelease(Ntv2UserBuffer* pUserBuffer)
{
	if(pUserBuffer == NULL) return;

	memset(pUserBuffer, 0, sizeof(Ntv2UserBuffer));
}

bool ntv2UserBufferCopyTo(Ntv2UserBuffer* pDstBuffer, uint32_t dstOffset, void* pSrcAddress, uint32_t size)
{
	uint8_t* pDst;
	int result;

	if((pDstBuffer == NULL) ||
	   (pSrcAddress == NULL) ||
	   (size == 0)) return false;

	if(!pDstBuffer->write) return false;

	if((dstOffset+size) > pDstBuffer->size) return false;

	pDst = (uint8_t*)pDstBuffer->pAddress;

	memcpy(pDst+dstOffset, pSrcAddress, size);

	return true;
}

bool ntv2UserBufferCopyFrom(Ntv2UserBuffer* pSrcBuffer, uint32_t srcOffset, void* pDstAddress, uint32_t size)
{
	uint8_t* pSrc;
	int result;

	if((pSrcBuffer == NULL) ||
	   (pDstAddress == NULL) ||
	   (size == 0)) return false;

	if((srcOffset+size) > pSrcBuffer->size) return false;

	pSrc = (uint8_t*)pSrcBuffer->pAddress;

	memcpy(pDstAddress, pSrc+srcOffset, size);

	return true;
}

// virtual dpc functions

bool ntv2DpcOpen(Ntv2Dpc* pDpc, Ntv2SystemContext* pSysCon, Ntv2DpcTask* pDpcTask, Ntv2DpcData dpcData)
{
	if((pDpc == NULL) ||
	   (pSysCon == NULL) ||
	   (pDpcTask == NULL)) return false;

	// initialize dpc data structure
	memset(pDpc, 0, sizeof(Ntv2Dpc));

	return false;
}

void ntv2DpcClose(Ntv2Dpc* pDpc)
{
	if(pDpc == NULL) return;

	// initialize dpc data structure
	memset(pDpc, 0, sizeof(Ntv2Dpc));
}

void ntv2DpcSchedule(Ntv2Dpc* pDpc)
{
	if(pDpc == NULL) return;
}

// virtual event functions

bool ntv2EventOpen(Ntv2Event* pEvent, Ntv2SystemContext* pSysCon)
{
	if((pEvent == NULL) ||
	   (pSysCon == NULL)) return false;

	// initialize event data structure
	memset(pEvent, 0, sizeof(Ntv2Event));

	return false;
}

void ntv2EventClose(Ntv2Event* pEvent)
{
	if(pEvent == NULL) return;
}

void ntv2EventSignal(Ntv2Event* pEvent)
{
	if(pEvent == NULL) return;
}

void ntv2EventClear(Ntv2Event* pEvent)
{
	if(pEvent == NULL) return;
}

bool ntv2EventWaitForSignal(Ntv2Event* pEvent, int64_t timeout, bool alert)
{
	int result = 0;
	uint32_t jifout;

	if(pEvent == NULL) return false;

#if defined (MSWindows)
    Sleep(timeout/1000);
#elif defined (AJALinux)
    usleep(timeout);
#endif

	return false;
}

// virtual semaphore functions

bool ntv2SemaphoreOpen(Ntv2Semaphore* pSemaphore, Ntv2SystemContext* pSysCon, uint32_t count)
{
	if((pSemaphore == NULL) ||
	   (pSysCon == NULL) ||
	   (count == 0)) return false;

	// initialize semaphore data structure
	memset(pSemaphore, 0, sizeof(Ntv2Semaphore));

	return false;
}

void ntv2SemaphoreClose(Ntv2Semaphore* pSemaphore)
{
	if(pSemaphore == NULL) return;

	// initialize semaphore data structure
	memset(pSemaphore, 0, sizeof(Ntv2Semaphore));
}

bool ntv2SemaphoreDown(Ntv2Semaphore* pSemaphore, int64_t timeout)
{
	uint32_t jifout;
	int result;

	if(pSemaphore == NULL) return false;

	return false;
}

void ntv2SemaphoreUp(Ntv2Semaphore* pSemaphore)
{
	if(pSemaphore == NULL) return;
}

int64_t ntv2TimeCounter(void)
{
	return 0;
}

int64_t ntv2TimeFrequency(void)
{
	return (int64_t)1000000;
}

int64_t ntv2Time100ns(void)
{
	return 0;
}

void ntv2TimeSleep(int64_t microseconds)
{
	if(microseconds == 0) return;

#ifdef AJALinux
    usleep(microseconds);
#endif    
}

bool ntv2ThreadOpen(Ntv2Thread* pThread, Ntv2SystemContext* pSysCon, const char* pName)
{
	if((pThread == NULL) ||
	   (pSysCon == NULL)) return false;

	// initialize semaphore data structure
	memset(pThread, 0, sizeof(Ntv2Thread));

	pThread->pName = pName;
	if(pThread->pName == NULL)
	{
		pThread->pName = "aja worker";
	}

	return true;
}

void ntv2ThreadClose(Ntv2Thread* pThread)
{
	if(pThread == NULL) return;
	
	ntv2ThreadStop(pThread);

	memset(pThread, 0, sizeof(Ntv2Thread));
}

int ntv2ThreadFunc(void* pData)
{
	Ntv2Thread* pThread = (Ntv2Thread*)pData;

	(*pThread->pFunc)(pThread->pContext);

	return 0;
}

bool ntv2ThreadRun(Ntv2Thread* pThread, Ntv2ThreadTask* pTask, void* pContext)
{
	if((pThread == NULL) ||
	   (pTask == NULL)) return false;

	if(pThread->run) return false;

	pThread->pFunc = pTask;
	pThread->pContext = pContext;
	pThread->run = true;

    (pTask)(pContext);

	return false;
}

void ntv2ThreadStop(Ntv2Thread* pThread)
{
	if(pThread == NULL) return;
	
	if(!pThread->run) return;
	pThread->run = false;

	pThread->pFunc = NULL;
	pThread->pContext = NULL;
}

void ntv2ThreadExit(Ntv2Thread* pThread)
{
}

const char* ntv2ThreadGetName(Ntv2Thread* pThread)
{
	if(pThread == NULL) return NULL;
	return pThread->pName;
}

bool ntv2ThreadShouldStop(Ntv2Thread* pThread)
{
	return false;
}

Ntv2Status ntv2ReadPciConfig(Ntv2SystemContext* pSysCon, void* pData, int32_t offset, int32_t size)
{
	int status;
	
	if ((pSysCon == NULL) || (pData == NULL))
		return NTV2_STATUS_BAD_PARAMETER;
	
	return NTV2_STATUS_FAIL;
}

Ntv2Status ntv2WritePciConfig(Ntv2SystemContext* pSysCon, void* pData, int32_t offset, int32_t size)
{
	int status;
	
	if ((pSysCon == NULL)  || (pData == NULL))
		return NTV2_STATUS_BAD_PARAMETER;
	
	return NTV2_STATUS_FAIL;
}

#elif defined (MSWindows)

// windows spinlock functions

bool ntv2SpinLockOpen(Ntv2SpinLock* pSpinLock, Ntv2SystemContext* pSysCon)
{
	if((pSpinLock == NULL) ||
	   (pSysCon == NULL)) return false;

	memset(pSpinLock, 0, sizeof(Ntv2SpinLock));

	KeInitializeSpinLock(&pSpinLock->lock);

	return true;
}

void ntv2SpinLockClose(Ntv2SpinLock* pSpinLock)
{
	if(pSpinLock == NULL) return;

	memset(pSpinLock, 0, sizeof(Ntv2SpinLock));
}

void ntv2SpinLockAcquire(Ntv2SpinLock* pSpinLock)
{
	if(pSpinLock == NULL) return;

	KeAcquireSpinLock(&pSpinLock->lock, &pSpinLock->irql);
}

void ntv2SpinLockRelease(Ntv2SpinLock* pSpinLock)
{
	if(pSpinLock == NULL) return;

	KeReleaseSpinLock(&pSpinLock->lock, pSpinLock->irql);
}

// windows interrupt lock fucntions

bool ntv2InterruptLockOpen(Ntv2InterruptLock* pInterruptLock, Ntv2SystemContext* pSysCon)
{
	if((pInterruptLock == NULL) ||
	   (pSysCon == NULL)) return false;

	memset(pInterruptLock, 0, sizeof(Ntv2InterruptLock));

	pInterruptLock->wdfInterrupt = pSysCon->wdfInterrupt;
	pInterruptLock->locked = false;

	return true;
}

void ntv2InterruptLockClose(Ntv2InterruptLock* pInterruptLock)
{
	if(pInterruptLock == NULL) return;

	memset(pInterruptLock, 0, sizeof(Ntv2InterruptLock));
}

void ntv2InterruptLockAcquire(Ntv2InterruptLock* pInterruptLock)
{
	if((pInterruptLock == NULL) || (pInterruptLock->wdfInterrupt == 0)) return;

	if(KeGetCurrentIrql() <= DISPATCH_LEVEL)
	{
		WdfInterruptAcquireLock(pInterruptLock->wdfInterrupt);
		pInterruptLock->locked = true;
	}
}

void ntv2InterruptLockRelease(Ntv2InterruptLock* pInterruptLock)
{
	if((pInterruptLock == NULL) || (pInterruptLock->wdfInterrupt == 0)) return;

	if(pInterruptLock->locked)
	{
		pInterruptLock->locked = false;
		WdfInterruptReleaseLock(pInterruptLock->wdfInterrupt);
	}
}

// windows memory functions

void* ntv2MemoryAlloc(uint32_t size)
{
	if(size == 0) return NULL;

	return (void*)ExAllocatePoolWithTag(NonPagedPool, (SIZE_T)size, '2vtn');
}

void ntv2MemoryFree(void* pAddress, uint32_t size)
{
	UNREFERENCED_PARAMETER(size);

	if(pAddress == NULL) return;

	ExFreePoolWithTag(pAddress, '2vtn');
}

bool ntv2DmaMemoryAlloc(Ntv2DmaMemory* pDmaMemory, Ntv2SystemContext* pSysCon, uint32_t size)
{
	WDFCOMMONBUFFER wdfCommonBuffer;
	PHYSICAL_ADDRESS physAddress;
	Ntv2Status status;

	if((pDmaMemory == NULL) ||
	   (pSysCon == NULL) ||
	   (size == 0)) return false;

	memset(pDmaMemory, 0, sizeof(Ntv2DmaMemory));

	status = WdfCommonBufferCreate(pSysCon->wdfDmaEnabler,
								   size,
								   WDF_NO_OBJECT_ATTRIBUTES,
								   &wdfCommonBuffer);
	if(!NT_SUCCESS(status)) return false;

	pDmaMemory->wdfCommonBuffer = wdfCommonBuffer;
	pDmaMemory->pAddress = WdfCommonBufferGetAlignedVirtualAddress(wdfCommonBuffer);
	physAddress = WdfCommonBufferGetAlignedLogicalAddress(wdfCommonBuffer);
	pDmaMemory->dmaAddress = (int64_t)physAddress.QuadPart;
	pDmaMemory->size = size;

	return true;
}

void ntv2DmaMemoryFree(Ntv2DmaMemory* pDmaMemory)
{
	if (pDmaMemory == NULL) return;

	memset(pDmaMemory, 0, sizeof(Ntv2DmaMemory));
}

void* ntv2DmaMemoryVirtual(Ntv2DmaMemory* pDmaMemory)
{
	if(pDmaMemory == NULL) return NULL;

	return pDmaMemory->pAddress;
}

Ntv2DmaAddress ntv2DmaMemoryPhysical(Ntv2DmaMemory* pDmaMemory)
{
	if(pDmaMemory == NULL) return 0;

	return pDmaMemory->dmaAddress;
}

uint32_t ntv2DmaMemorySize(Ntv2DmaMemory* pDmaMemory)
{
	if(pDmaMemory == NULL) return 0;

	return pDmaMemory->size;
}

// windows user buffer functions

bool ntv2UserBufferPrepare(Ntv2UserBuffer* pUserBuffer, Ntv2SystemContext* pSysCon, uint64_t address, uint32_t size, bool write)
{
	PVOID pAddress = NULL;

	if((pUserBuffer == NULL) ||
	   (pSysCon == NULL) ||
	   (address == 0) ||
	   (size == 0)) return false;

	// initialize data structure
	memset(pUserBuffer, 0, sizeof(Ntv2UserBuffer));

	// get the address
	pAddress = (PVOID)((UINT_PTR)address);
	if(pAddress == NULL) return false;

	__try
	{
		if(write)
		{
			ProbeForWrite(pAddress, size, 4);
		}
		else
		{
			ProbeForRead(pAddress, size, 4);
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return false;
	}

	// save user buffer info
	pUserBuffer->pAddress = pAddress;
	pUserBuffer->size = size;
	pUserBuffer->write = write;

	return true;
}

void ntv2UserBufferRelease(Ntv2UserBuffer* pUserBuffer)
{
	if(pUserBuffer == NULL) return;

	memset(pUserBuffer, 0, sizeof(Ntv2UserBuffer));
}

bool ntv2UserBufferCopyTo(Ntv2UserBuffer* pDstBuffer, uint32_t dstOffset, void* pSrcAddress, uint32_t size)
{
	uint8_t* pDst;

	if((pDstBuffer == NULL) ||
	   (pSrcAddress == NULL) ||
	   (size == 0)) return false;

	// check prepared for write
	if(!pDstBuffer->write) return false;

	// check buffer sizes
	if((dstOffset+size) > pDstBuffer->size) return false;

	// convert buffer address for offset math
	pDst = (uint8_t*)pDstBuffer->pAddress;

	// copy the buffer
	__try
	{
		RtlMoveMemory(pDst+dstOffset, pSrcAddress, size);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return false;
	}

	return true;
}

bool ntv2UserBufferCopyFrom(Ntv2UserBuffer* pSrcBuffer, uint32_t srcOffset, void* pDstAddress, uint32_t size)
{
	uint8_t* pSrc;

	if((pSrcBuffer == NULL) ||
	   (pDstAddress == NULL) ||
	   (size == 0)) return false;

	// check buffer sizes
	if((srcOffset+size) > pSrcBuffer->size) return false;

	// convert buffer address for offset math
	pSrc = (uint8_t*)pSrcBuffer->pAddress;

	// copy the buffer
	__try
	{
		RtlMoveMemory(pDstAddress, pSrc+srcOffset, size);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return false;
	}

	return true;
}

// windows dpc functions

typedef struct ntv2_dpc_context
{
	Ntv2DpcTask*		pDpcTask;
   	Ntv2DpcData			dpcData;
	Ntv2SpinLock		serialLock;
	int64_t				serialSchedule;
	int64_t				serialProcess;
	bool				serialActive;
} Ntv2DpcContext;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(Ntv2DpcContext, ntv2DpcGetContext);

VOID ntv2DpcFunc(WDFDPC wdfDpc)
{
	Ntv2DpcContext* pContext = NULL;

	if(wdfDpc == 0) return;

	pContext = ntv2DpcGetContext(wdfDpc);
	if(pContext == NULL) return;
	if(pContext->pDpcTask == NULL) return;

	// serialize dpc like a linux tasklet
	ntv2SpinLockAcquire(&pContext->serialLock);

	// check for active dpc or nothing to do
	if((pContext->serialActive) ||
	   (pContext->serialProcess >= pContext->serialSchedule))
	{
		// nothing todo
		ntv2SpinLockRelease(&pContext->serialLock);
		return;
	}

	// we are the active dpc
	pContext->serialActive = true;

	ntv2SpinLockRelease(&pContext->serialLock);

	for(;;)
	{
		// execute the dpc task once for each schedule
		(*pContext->pDpcTask)(pContext->dpcData);
		pContext->serialProcess++;

		ntv2SpinLockAcquire(&pContext->serialLock);

		// check for repeat
		if(pContext->serialProcess >= pContext->serialSchedule)
		{
			// no more todo
			pContext->serialActive = false;
			ntv2SpinLockRelease(&pContext->serialLock);
			return;
		}

		ntv2SpinLockRelease(&pContext->serialLock);
	}
}

bool ntv2DpcOpen(Ntv2Dpc* pDpc, Ntv2SystemContext* pSysCon, Ntv2DpcTask* pDpcTask, Ntv2DpcData dpcData)
{
	WDF_DPC_CONFIG dpcConfig;
	WDF_OBJECT_ATTRIBUTES dpcAttributes;
	WDFDPC wdfDpc;
	Ntv2DpcContext* pContext = NULL;
	NTSTATUS status;

	if((pDpc == NULL) ||
	   (pSysCon == NULL) ||
	   (pDpcTask == NULL)) return false;

	// initialize dpc data structure
	memset(pDpc, 0, sizeof(Ntv2Dpc));

	// initialize dpc object
	WDF_DPC_CONFIG_INIT(&dpcConfig,	ntv2DpcFunc);
	dpcConfig.AutomaticSerialization = TRUE;
	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&dpcAttributes, Ntv2DpcContext);
	dpcAttributes.ParentObject = pSysCon->wdfDevice;
	status = WdfDpcCreate(&dpcConfig,
						  &dpcAttributes,
						  &wdfDpc);
	if(!NT_SUCCESS(status)) return false;

	pContext = ntv2DpcGetContext(wdfDpc);
	if(pContext == NULL) return false;

	// setup dpc data
	pDpc->wdfDpc = wdfDpc;
	pContext->pDpcTask = pDpcTask;
	pContext->dpcData = dpcData;
	ntv2SpinLockOpen(&pContext->serialLock, pSysCon);
	pContext->serialSchedule = 0;
	pContext->serialProcess = 0;
	pContext->serialActive = false;

	return true;
}

void ntv2DpcClose(Ntv2Dpc* pDpc)
{
	Ntv2DpcContext* pContext = NULL;

	if(pDpc == NULL) return;
	if(pDpc->wdfDpc == 0) return;

	pContext = ntv2DpcGetContext(pDpc->wdfDpc);
	if(pContext == NULL) return;

	WdfDpcCancel(pDpc->wdfDpc, TRUE);

	ntv2SpinLockClose(&pContext->serialLock);

	// initialize dpc data structure
	memset(pDpc, 0, sizeof(Ntv2Dpc));
}

void ntv2DpcSchedule(Ntv2Dpc* pDpc)
{
	Ntv2DpcContext* pContext = NULL;
	if(pDpc == NULL) return;
	if(pDpc->wdfDpc == 0) return;

	pContext = ntv2DpcGetContext(pDpc->wdfDpc);
	if(pContext == NULL) return;

	pContext->serialSchedule++;
	WdfDpcEnqueue(pDpc->wdfDpc);
}


// windows event functions

bool ntv2EventOpen(Ntv2Event* pEvent, Ntv2SystemContext* pSysCon)
{
	if((pEvent == NULL) ||
	   (pSysCon == NULL)) return false;

	// initialize event data structure
	memset(pEvent, 0, sizeof(Ntv2Event));

	KeInitializeEvent(&pEvent->event, NotificationEvent, FALSE);

	return true;
}

void ntv2EventClose(Ntv2Event* pEvent)
{
	if(pEvent == NULL) return;

	// initialize event data structure
	memset(pEvent, 0, sizeof(Ntv2Event));
}

void ntv2EventSignal(Ntv2Event* pEvent)
{
	if(pEvent == NULL) return;

	KeSetEvent(&pEvent->event, 0, FALSE);
}

void ntv2EventClear(Ntv2Event* pEvent)
{
	if(pEvent == NULL) return;

	KeClearEvent(&pEvent->event);
}

bool ntv2EventWaitForSignal(Ntv2Event* pEvent, int64_t timeout, bool alert)
{
	LARGE_INTEGER liTimeout;
	NTSTATUS status;

	if(pEvent == NULL) return false;

	liTimeout.QuadPart = -(10 * (int32_t)timeout);

	status =  KeWaitForSingleObject(&pEvent->event,
									Executive,
									KernelMode,
									(alert? TRUE:FALSE),
									&liTimeout);
	if(!NT_SUCCESS(status) || (status == STATUS_TIMEOUT)) return false;

	return true;
}


// windows semaphore functions

bool ntv2SemaphoreOpen(Ntv2Semaphore* pSemaphore, Ntv2SystemContext* pSysCon, uint32_t count)
{
	if((pSemaphore == NULL) ||
	   (pSysCon == NULL) ||
	   (count == 0)) return false;

	// initialize semaphore data structure
	memset(pSemaphore, 0, sizeof(Ntv2Semaphore));

	KeInitializeSemaphore(&pSemaphore->semaphore, (LONG)count, (LONG)count);

	return true;
}

void ntv2SemaphoreClose(Ntv2Semaphore* pSemaphore)
{
	if(pSemaphore == NULL) return;

	// initialize semaphore data structure
	memset(pSemaphore, 0, sizeof(Ntv2Semaphore));
}

bool ntv2SemaphoreDown(Ntv2Semaphore* pSemaphore, int64_t timeout)
{
	LARGE_INTEGER liTimeout;
	NTSTATUS status;

	if(pSemaphore == NULL) return false;

	liTimeout.QuadPart = -(10 * (int32_t)timeout);

	status =  KeWaitForSingleObject(&pSemaphore->semaphore,
									Executive,
									KernelMode,
									FALSE,
									&liTimeout);
	if(!NT_SUCCESS(status) || (status == STATUS_TIMEOUT)) return false;

	return true;
}

void ntv2SemaphoreUp(Ntv2Semaphore* pSemaphore)
{
	if(pSemaphore == NULL) return;

	KeReleaseSemaphore(&pSemaphore->semaphore, 0, 1, FALSE);
}

#define NTV2_PERFORMANCE_FREQUENCY_DEFAULT 1
static int64_t sNtv2PerformanceFrequency = NTV2_PERFORMANCE_FREQUENCY_DEFAULT;

int64_t ntv2TimeCounter()
{
	LARGE_INTEGER time, performanceFrequency;

	time = KeQueryPerformanceCounter(&performanceFrequency);

	return (int64_t)(time.QuadPart);
}

int64_t ntv2TimeFrequency()
{
	LARGE_INTEGER performanceFrequency;

	if (sNtv2PerformanceFrequency == NTV2_PERFORMANCE_FREQUENCY_DEFAULT)
	{
		KeQueryPerformanceCounter(&performanceFrequency);
		sNtv2PerformanceFrequency = (int64_t)(performanceFrequency.QuadPart);
	}

	return sNtv2PerformanceFrequency;
}

int64_t ntv2Time100ns(void)
{
	int64_t time;
	int64_t freq;
	int64_t adjust;

	time = ntv2TimeCounter();
	freq = ntv2TimeFrequency();

	adjust = time % freq;
	time /= freq;
	time *= 10000000;
	adjust *= 10000000;
	adjust /= freq;
	time += adjust;

	return time;
}

void ntv2TimeSleep(int64_t microseconds)
{
	LARGE_INTEGER largeMicroseconds;

	if(microseconds == 0) return;

	// KeDelayExecutionThread take in a large integer in units of 100 nanaseconds
	largeMicroseconds.QuadPart = -microseconds * 10;

	KeDelayExecutionThread(KernelMode, FALSE, &largeMicroseconds);
}

bool ntv2ThreadOpen(Ntv2Thread* pThread, Ntv2SystemContext* pSysCon, const char* pName)
{
	if((pThread == NULL) ||
	   (pSysCon == NULL))
	{
		return false;
	}
	
	// initialize semaphore data structure
	memset(pThread, 0, sizeof(Ntv2Thread));

	pThread->pName = pName;
	if(pThread->pName == NULL)
	{
		pThread->pName = "aja worker";
	}

	return false;
}

void ntv2ThreadClose(Ntv2Thread* pThread)
{
	if(pThread == NULL) return;
	
	ntv2ThreadStop(pThread);

	memset(pThread, 0, sizeof(Ntv2Thread));
}

void ntv2ThreadExit(Ntv2Thread* pThread)
{
	UNREFERENCED_PARAMETER(pThread);
}

int ntv2ThreadFunc(void* pData)
{
	Ntv2Thread* pThread = (Ntv2Thread*)pData;

	(*pThread->pFunc)(pThread->pContext);

	PsTerminateSystemThread(STATUS_SUCCESS);
	return 0;
}

bool ntv2ThreadRun(Ntv2Thread* pThread, Ntv2ThreadTask* pTask, void* pContext)
{
	OBJECT_ATTRIBUTES ObjectAttributes;
	HANDLE hTask;
	NTSTATUS status;

	if((pThread == NULL) ||
	   (pTask == NULL)) return false;

	if(pThread->run) return false;

	pThread->pFunc = pTask;
	pThread->pContext = pContext;
	pThread->run = true;

	InitializeObjectAttributes(&ObjectAttributes, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);
	
	status = PsCreateSystemThread(
		&hTask, 
		THREAD_ALL_ACCESS,
		&ObjectAttributes,
		NULL, 
		NULL, 
		(PKSTART_ROUTINE)ntv2ThreadFunc,
		(PVOID)pThread);
	if (!NT_SUCCESS(status))
	{
		pThread->pTask = NULL;
		pThread->pFunc = NULL;
		pThread->pContext = NULL;
		pThread->run = false;
		return false;
	}

	ObReferenceObjectByHandle(
		hTask, 
		THREAD_ALL_ACCESS, 
		NULL,
		KernelMode, 
		(PVOID*)&pThread->pTask,
		NULL);
	ZwClose(hTask);

	return true;
}

void ntv2ThreadStop(Ntv2Thread* pThread)
{
	if(pThread == NULL) return;
	
	if(!pThread->run) return;
	pThread->run = false;

	KeWaitForSingleObject(pThread->pTask, Executive, KernelMode, FALSE, NULL);
	ObDereferenceObject(pThread->pTask);

	pThread->pTask = NULL;
	pThread->pFunc = NULL;
	pThread->pContext = NULL;
}

const char* ntv2ThreadGetName(Ntv2Thread* pThread)
{
	if(pThread == NULL) return NULL;
	return pThread->pName;
}

bool ntv2ThreadShouldStop(Ntv2Thread* pThread)
{
	return !pThread->run;
}

Ntv2Status ntv2ReadPciConfig(Ntv2SystemContext* pSysCon, void* pData, int32_t offset, int32_t size)
{
	ULONG bytes;

	if (pSysCon->pBusInterface == NULL)
		return NTV2_STATUS_FAIL;

	bytes = pSysCon->pBusInterface->GetBusData(
		pSysCon->pBusInterface->Context,
		PCI_WHICHSPACE_CONFIG,
		pData,
		offset,
		size);

	if (bytes != (ULONG)size)
		return NTV2_STATUS_FAIL;

	return NTV2_STATUS_SUCCESS;
}

Ntv2Status ntv2WritePciConfig(Ntv2SystemContext* pSysCon, void* pData, int32_t offset, int32_t size)
{
	ULONG bytes;

	if (pSysCon->pBusInterface == NULL)
		return NTV2_STATUS_FAIL;

	bytes = pSysCon->pBusInterface->SetBusData(
		pSysCon->pBusInterface->Context,
		PCI_WHICHSPACE_CONFIG,
		pData,
		offset,
		size);

	if (bytes != (ULONG)size)
		return NTV2_STATUS_FAIL;

	return NTV2_STATUS_SUCCESS;
}


#elif defined (AJAMac)

// Mac spinlock functions
bool ntv2SpinLockOpen(Ntv2SpinLock* pSpinLock, Ntv2SystemContext* pSysCon)
{
	if((pSpinLock == NULL) ||
	   (pSysCon == NULL)) return false;
	
#if defined(AJAMacDext)
	atomic_flag_clear(pSpinLock->lock);
#else
	memset(pSpinLock, 0, sizeof(Ntv2SpinLock));
	pSpinLock->lock = IOSimpleLockAlloc();
	IOSimpleLockInit(pSpinLock->lock);
#endif

	return true;
}

void ntv2SpinLockClose(Ntv2SpinLock* pSpinLock)
{
	if(pSpinLock == NULL) return;
	
#if defined(AJAMacDext)
	atomic_flag_clear(pSpinLock->lock);
#else
	IOSimpleLockFree(pSpinLock->lock);
#endif
	memset(pSpinLock, 0, sizeof(Ntv2SpinLock));
}

void ntv2SpinLockAcquire(Ntv2SpinLock* pSpinLock)
{
	if(pSpinLock == NULL) return;
	
#if defined(AJAMacDext)
	while(atomic_flag_test_and_set(pSpinLock->lock)) {;}
#else
	IOSimpleLockLock(pSpinLock->lock);
#endif
}

void ntv2SpinLockRelease(Ntv2SpinLock* pSpinLock)
{
	if(pSpinLock == NULL) return;
	
#if defined(AJAMacDext)
	atomic_flag_clear(pSpinLock->lock);
#else
	IOSimpleLockUnlock(pSpinLock->lock);
#endif
}

bool ntv2InterruptLockOpen(Ntv2InterruptLock* pInterruptLock, Ntv2SystemContext* pSysCon)
{
	if((pInterruptLock == NULL) ||
	   (pSysCon == NULL)) return false;
#if defined(AJAMacDext)
	atomic_flag_clear(pInterruptLock->lock);
#else
	memset(pInterruptLock, 0, sizeof(Ntv2InterruptLock));
	pInterruptLock->lock = IOSimpleLockAlloc();
	IOSimpleLockInit(pInterruptLock->lock);
#endif

	return true;
}

void ntv2InterruptLockClose(Ntv2InterruptLock* pInterruptLock)
{
	if(pInterruptLock == NULL || pInterruptLock->lock == NULL) return;
#if defined(AJAMacDext)
	atomic_flag_clear(pInterruptLock->lock);
#else
	IOSimpleLockFree(pInterruptLock->lock);
	memset(pInterruptLock, 0, sizeof(Ntv2InterruptLock));
#endif
}

void ntv2InterruptLockAcquire(Ntv2InterruptLock* pInterruptLock)
{
	if(pInterruptLock == NULL || pInterruptLock->lock == NULL) return;
#if defined(AJAMacDext)
	while(atomic_flag_test_and_set(pInterruptLock->lock)) {;}
#else
	IOSimpleLockLock(pInterruptLock->lock);
	pInterruptLock->locked = true;
#endif
}

void ntv2InterruptLockRelease(Ntv2InterruptLock* pInterruptLock)
{
	if(pInterruptLock == NULL || pInterruptLock->lock == NULL	) return;
#if defined(AJAMacDext)
	atomic_flag_clear(pInterruptLock->lock);
#else
	IOSimpleLockUnlock(pInterruptLock->lock);
	pInterruptLock->locked = false;
#endif
}

// Mac memory functions

void* ntv2MemoryAlloc(uint32_t size)
{
	if(size == 0) return NULL;
	
	return (void*)IOMalloc(size);
}

void ntv2MemoryFree(void* pAddress, uint32_t size)
{
	if(pAddress == NULL) return;
	
	IOFree(pAddress, size);
}

// Mac event functions

bool ntv2EventOpen(Ntv2Event* pEvent, Ntv2SystemContext* pSysCon)
{
	if((pEvent == NULL) ||
	   (pSysCon == NULL)) return false;
	
	// initialize event data structure
	memset(pEvent, 0, sizeof(Ntv2Event));
	
	pEvent->pRecursiveLock = IORecursiveLockAlloc();

	return true;
}

void ntv2EventClose(Ntv2Event* pEvent)
{
	if(pEvent == NULL) return;
	
	IORecursiveLockFree(pEvent->pRecursiveLock);

	// initialize event data structure
	memset(pEvent, 0, sizeof(Ntv2Event));
}

void ntv2EventSignal(Ntv2Event* pEvent)
{
	Ntv2Event* pEventMac = pEvent;

	if(pEventMac == NULL) return;
	
	pEventMac->flag = true;
	
#if defined(AJAMacDext)
	//I do not see a replacement for this in DriverKit
#else
	IORecursiveLockWakeup(pEventMac->pRecursiveLock, pEventMac->pRecursiveLock, false);
#endif
}

void ntv2EventClear(Ntv2Event* pEvent)
{
	Ntv2Event* pEventMac = pEvent;

	if(pEventMac == NULL) return;

	pEventMac->flag = false;
}

bool ntv2EventWaitForSignal(Ntv2Event* pEvent, int64_t timeout, bool alert)
{
	uint64_t currentTime;
	uint64_t timeoutNanos;
	
	Ntv2Event* pEventMac = pEvent;

	if(pEventMac == NULL) return false;
	
	// if flag is true, event has been "signaled", returns immediately until it is cleared
	if(pEventMac->flag) return true;

#if defined(AJAMacDext)
#else
	// Get the current time
	clock_get_uptime(&currentTime);
	nanoseconds_to_absolutetime(timeout*1000, &timeoutNanos);
	currentTime += timeoutNanos;
	
	// must have lock before sleeping
	IORecursiveLockLock(pEventMac->pRecursiveLock);

	// sleeping unlocks the lock, upon wakeup it reacquires the lock
	AbsoluteTime deadline = *((AbsoluteTime *) &currentTime);
	int result = IORecursiveLockSleepDeadline(pEventMac->pRecursiveLock, pEventMac->pRecursiveLock, deadline, THREAD_ABORTSAFE);
	
	// release lock
	IORecursiveLockUnlock(pEventMac->pRecursiveLock);
	
	// non zero value indicates failure or timeout
	if (result) return false;
#endif
	
	return true;
}

// Mac sleep function

void ntv2TimeSleep(int64_t microseconds)
{
	if(microseconds == 0) return;
	
	if(microseconds > 1000)
	{
		IOSleep(microseconds/1000);
	}
	else
	{
		IODelay(microseconds);
	}
}

// Mac thread functions

bool ntv2ThreadOpen(Ntv2Thread* pThread, Ntv2SystemContext* pSysCon, const char* pName)
{
	if((pThread == NULL) ||
	   (pSysCon == NULL)) return false;
	
	// initialize thread data structure
	memset(pThread, 0, sizeof(Ntv2Thread));
	
	pThread->pName = pName;
	if(pThread->pName == NULL)
	{
		pThread->pName = "aja worker";
	}
	
	return true;
}

void ntv2ThreadClose(Ntv2Thread* pThread)
{
	if(pThread == NULL) return;
	
	ntv2ThreadStop(pThread);
	
	memset(pThread, 0, sizeof(Ntv2Thread));
}

bool ntv2ThreadRun(Ntv2Thread* pThread, Ntv2ThreadTask* pTask, void* pContext)
{
	kern_return_t	result;

	if((pThread == NULL) ||
	   (pTask == NULL)) return false;
	
	if(pThread->run) return false;
	
	pThread->pFunc = pTask;
	pThread->pContext = pContext;
	pThread->run = true;
	
#if defined(AJAMacDext)
	result = kIOReturnUnsupported;
#else
	result = kernel_thread_start((thread_continue_t)pThread->pFunc, (void*)pContext, &pThread->pTask);
#endif
	if (result != KERN_SUCCESS)
	{
		pThread->pTask = NULL;
		pThread->pFunc = NULL;
		pThread->pContext = NULL;
		pThread->run = false;
		return false;
	}
	
	return true;
}

void ntv2ThreadStop(Ntv2Thread* pThread)
{
	int timeOutMs = 250;
	
	if(pThread == NULL) return;
	
	if(!pThread->run) return;
	pThread->run = false;
	
	// waiting for ntv2ThreadExit to be called
	while (timeOutMs > 0)
	{
		if (pThread->pTask == NULL)
			break;
		IOSleep(10);
		timeOutMs -= 10;
	}
	
	//if (timeOutMs <= 0 && pThread->pTask != NULL)
		//DebugLog("ntv2ThreadStop - fail to exit thread\n");
    
	return;
}

const char* ntv2ThreadGetName(Ntv2Thread* pThread)
{
	if(pThread == NULL) return NULL;
	return pThread->pName;
}

bool ntv2ThreadShouldStop(Ntv2Thread* pThread)
{
	return !pThread->run;
}

void ntv2ThreadExit(Ntv2Thread* pThread)
{
	pThread->pTask = NULL;
	pThread->pFunc = NULL;
	pThread->pContext = NULL;
	
	// no code is executed after this line
#if defined(AJAMacDext)
#else
	(void) thread_terminate(current_thread());
#endif
}
extern Ntv2Status ntv2ReadPCIConfigCon(Ntv2SystemContext* pSysCon, void* pData, int32_t offset, int32_t size);
extern Ntv2Status ntv2WritePCIConfigCon(Ntv2SystemContext* pSysCon, void* pData, int32_t offset, int32_t size);

Ntv2Status ntv2ReadPciConfig(Ntv2SystemContext* pSysCon, void* pData, int32_t offset, int32_t size)
{
	return ntv2ReadPCIConfigCon(pSysCon, pData, offset, size);
}

Ntv2Status ntv2WritePciConfig(Ntv2SystemContext* pSysCon, void* pData, int32_t offset, int32_t size)
{
	return ntv2WritePCIConfigCon(pSysCon, pData, offset, size);
}

int64_t ntv2Time100ns(void)
{
	uint64_t nanoTime = 0;
	mach_timebase_info_data_t timebaseInfo;
	uint64_t machTime = mach_absolute_time();
#if defined(AJAMacDext)
	mach_timebase_info(&timebaseInfo);
#else
	//mach_timebase_info(&timebaseInfo);
#endif
	nanoTime = machTime * timebaseInfo.numer / timebaseInfo.denom;
	return nanoTime;
}

bool ntv2UserBufferPrepare(Ntv2UserBuffer* pUserBuffer, Ntv2SystemContext* pSysCon,
						   uint64_t address, uint32_t size, bool write)
{
	if((pUserBuffer == NULL) ||
	   (pSysCon == NULL) ||
	   (address == 0) ||
	   (size == 0)) return false;

	bzero((void*)pUserBuffer, sizeof(Ntv2UserBuffer));

	pUserBuffer->pAddress = (void*)((uintptr_t)(address));
	pUserBuffer->size = size;
	pUserBuffer->write = write;

	return true;
}

void ntv2UserBufferRelease(Ntv2UserBuffer* pUserBuffer)
{
	if(pUserBuffer == NULL) return;

	memset(pUserBuffer, 0, sizeof(Ntv2UserBuffer));
}

bool ntv2UserBufferCopyTo(Ntv2UserBuffer* pDstBuffer, uint32_t dstOffset, void* pSrcAddress, uint32_t size)
{
	uint8_t* pDst;

	if((pDstBuffer == NULL) ||
	   (pSrcAddress == NULL) ||
	   (size == 0)) return false;

	if(!pDstBuffer->write) return false;

	if((dstOffset+size) > pDstBuffer->size) return false;

	pDst = (uint8_t*)pDstBuffer->pAddress;

	memcpy(pDst+dstOffset, pSrcAddress, size);

	return true;
}

bool ntv2UserBufferCopyFrom(Ntv2UserBuffer* pSrcBuffer, uint32_t srcOffset, void* pDstAddress, uint32_t size)
{
	uint8_t* pSrc;

	if((pSrcBuffer == NULL) ||
	   (pDstAddress == NULL) ||
	   (size == 0)) return false;

	if((srcOffset+size) > pSrcBuffer->size) return false;

	pSrc = (uint8_t*)pSrcBuffer->pAddress;

	memcpy(pDstAddress, pSrc+srcOffset, size);

	return true;
}

#elif defined (AJALinux)

#include <asm/div64.h>

static uint32_t microsecondsToJiffies(int64_t timeout);


// linux spinlock functions

bool ntv2SpinLockOpen(Ntv2SpinLock* pSpinLock, Ntv2SystemContext* pSysCon)
{
	if((pSpinLock == NULL) ||
	   (pSysCon == NULL)) return false;

	memset(pSpinLock, 0, sizeof(Ntv2SpinLock));

	spin_lock_init(&pSpinLock->lock);

	return true;
}

void ntv2SpinLockClose(Ntv2SpinLock* pSpinLock)
{
	if(pSpinLock == NULL) return;

	memset(pSpinLock, 0, sizeof(Ntv2SpinLock));
}

void ntv2SpinLockAcquire(Ntv2SpinLock* pSpinLock)
{
	if(pSpinLock == NULL) return;

	spin_lock_bh(&pSpinLock->lock);
}

void ntv2SpinLockRelease(Ntv2SpinLock* pSpinLock)
{
	if(pSpinLock == NULL) return;

	spin_unlock_bh(&pSpinLock->lock);
}

// linux interrupt lock fucntions

bool ntv2InterruptLockOpen(Ntv2InterruptLock* pInterruptLock, Ntv2SystemContext* pSysCon)
{
	if((pInterruptLock == NULL) ||
	   (pSysCon == NULL)) return false;

	memset(pInterruptLock, 0, sizeof(Ntv2InterruptLock));

	spin_lock_init(&pInterruptLock->lock);

	return true;
}

void ntv2InterruptLockClose(Ntv2InterruptLock* pInterruptLock)
{
	if(pInterruptLock == NULL) return;

	memset(pInterruptLock, 0, sizeof(Ntv2InterruptLock));
}

void ntv2InterruptLockAcquire(Ntv2InterruptLock* pInterruptLock)
{
	if(pInterruptLock == NULL) return;

	spin_lock_irqsave(&pInterruptLock->lock, pInterruptLock->flags);
}

void ntv2InterruptLockRelease(Ntv2InterruptLock* pInterruptLock)
{
	if(pInterruptLock == NULL) return;

	spin_unlock_irqrestore(&pInterruptLock->lock, pInterruptLock->flags);
}

// linux memory functions

void* ntv2MemoryAlloc(uint32_t size)
{
	if(size == 0) return NULL;

	return (void*)vmalloc(size);
}

void ntv2MemoryFree(void* pAddress, uint32_t size)
{
	if(pAddress == NULL) return;
	
	vfree(pAddress);
}

bool ntv2DmaMemoryAlloc(Ntv2DmaMemory* pDmaMemory, Ntv2SystemContext* pSysCon, uint32_t size)
{
	void* pAddress = NULL;
	Ntv2DmaAddress dmaAddress = 0;

	if((pDmaMemory == NULL) ||
	   (pSysCon == NULL) ||
	   (pSysCon->pDevice == NULL) ||
	   (size == 0)) return false;

	pAddress = dma_alloc_coherent(&pSysCon->pDevice->dev, size, &dmaAddress, GFP_ATOMIC);
	if((pAddress == NULL) || (dmaAddress == 0)) return false;

	// initialize memory data structure
	memset(pDmaMemory, 0, sizeof(Ntv2DmaMemory));

	pDmaMemory->pDevice = pSysCon->pDevice;
	pDmaMemory->pAddress = pAddress;
	pDmaMemory->dmaAddress = dmaAddress;
	pDmaMemory->size = size;

	return true;
}

void ntv2DmaMemoryFree(Ntv2DmaMemory* pDmaMemory)
{
	if((pDmaMemory == NULL) ||
	   (pDmaMemory->pAddress == NULL) ||
	   (pDmaMemory->dmaAddress == 0) ||
	   (pDmaMemory->size == 0)) return;

	dma_free_coherent(&pDmaMemory->pDevice->dev,
						pDmaMemory->size,
						pDmaMemory->pAddress,
						pDmaMemory->dmaAddress);

	memset(pDmaMemory, 0, sizeof(Ntv2DmaMemory));
}

void* ntv2DmaMemoryVirtual(Ntv2DmaMemory* pDmaMemory)
{
	if(pDmaMemory == NULL) return NULL;

	return pDmaMemory->pAddress;
}

Ntv2DmaAddress ntv2DmaMemoryPhysical(Ntv2DmaMemory* pDmaMemory)
{
	if(pDmaMemory == NULL) return 0;

	return pDmaMemory->dmaAddress;
}

uint32_t ntv2DmaMemorySize(Ntv2DmaMemory* pDmaMemory)
{
	if(pDmaMemory == NULL) return 0;

	return pDmaMemory->size;
}

// linux user buffer functions

bool ntv2UserBufferPrepare(Ntv2UserBuffer* pUserBuffer, Ntv2SystemContext* pSysCon,
						   uint64_t address, uint32_t size, bool write)
{
	if((pUserBuffer == NULL) ||
	   (pSysCon == NULL) ||
	   (address == 0) ||
	   (size == 0)) return false;

	memset(pUserBuffer, 0, sizeof(Ntv2UserBuffer));

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0))
	if(!access_ok((void*)((uintptr_t)(address)), size)) return false;
#elif defined(__aarch64__)
	if(!access_ok(write?VERIFY_WRITE:VERIFY_READ, (void*)((uintptr_t)(address)), size)) return false;
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(4,18,0))
	if(!access_ok((void*)((uintptr_t)(address)), size)) return false;
#else
	if(!access_ok(write?VERIFY_WRITE:VERIFY_READ, (void*)((uintptr_t)(address)), size)) return false;
#endif

	pUserBuffer->pAddress = (void*)((uintptr_t)(address));
	pUserBuffer->size = size;
	pUserBuffer->write = write;

	return true;
}

void ntv2UserBufferRelease(Ntv2UserBuffer* pUserBuffer)
{
	if(pUserBuffer == NULL) return;

	memset(pUserBuffer, 0, sizeof(Ntv2UserBuffer));
}

bool ntv2UserBufferCopyTo(Ntv2UserBuffer* pDstBuffer, uint32_t dstOffset, void* pSrcAddress, uint32_t size)
{
	uint8_t* pDst;
	int result;

	if((pDstBuffer == NULL) ||
	   (pSrcAddress == NULL) ||
	   (size == 0)) return false;

	if(!pDstBuffer->write) return false;

	if((dstOffset+size) > pDstBuffer->size) return false;

	pDst = (uint8_t*)pDstBuffer->pAddress;

	result = __copy_to_user(pDst+dstOffset, pSrcAddress, size);
	if(result != 0) return false;

	return true;
}

bool ntv2UserBufferCopyFrom(Ntv2UserBuffer* pSrcBuffer, uint32_t srcOffset, void* pDstAddress, uint32_t size)
{
	uint8_t* pSrc;
	int result;

	if((pSrcBuffer == NULL) ||
	   (pDstAddress == NULL) ||
	   (size == 0)) return false;

	if((srcOffset+size) > pSrcBuffer->size) return false;

	pSrc = (uint8_t*)pSrcBuffer->pAddress;

	result = __copy_from_user(pDstAddress, pSrc+srcOffset, size);
	if(result != 0) return false;

	return true;
}

// linux dpc functions

bool ntv2DpcOpen(Ntv2Dpc* pDpc, Ntv2SystemContext* pSysCon, Ntv2DpcTask* pDpcTask, Ntv2DpcData dpcData)
{
	if((pDpc == NULL) ||
	   (pSysCon == NULL) ||
	   (pDpcTask == NULL)) return false;

	// initialize dpc data structure
	memset(pDpc, 0, sizeof(Ntv2Dpc));

	tasklet_init(&pDpc->tasklet, pDpcTask, dpcData);

	return true;
}

void ntv2DpcClose(Ntv2Dpc* pDpc)
{
	if(pDpc == NULL) return;

	tasklet_kill(&pDpc->tasklet);

	// initialize dpc data structure
	memset(pDpc, 0, sizeof(Ntv2Dpc));
}

void ntv2DpcSchedule(Ntv2Dpc* pDpc)
{
	if(pDpc == NULL) return;

	tasklet_schedule(&pDpc->tasklet);
}

// linux event functions

bool ntv2EventOpen(Ntv2Event* pEvent, Ntv2SystemContext* pSysCon)
{
	if((pEvent == NULL) ||
	   (pSysCon == NULL)) return false;

	// initialize event data structure
	memset(pEvent, 0, sizeof(Ntv2Event));

	init_waitqueue_head(&pEvent->event);

	return true;
}

void ntv2EventClose(Ntv2Event* pEvent)
{
	if(pEvent == NULL) return;

	// initialize event data structure
	memset(pEvent, 0, sizeof(Ntv2Event));
}

void ntv2EventSignal(Ntv2Event* pEvent)
{
	if(pEvent == NULL) return;

	pEvent->flag = true;
	wake_up(&pEvent->event);
}

void ntv2EventClear(Ntv2Event* pEvent)
{
	if(pEvent == NULL) return;

	pEvent->flag = false;
}

bool ntv2EventWaitForSignal(Ntv2Event* pEvent, int64_t timeout, bool alert)
{
	int result = 0;
	uint32_t jifout;

	if(pEvent == NULL) return false;

	// convert to jiffies
	jifout = microsecondsToJiffies(timeout);
	if (jifout < 1) jifout = 1;

	// wait for signal
	if(alert)
	{
		result = wait_event_interruptible_timeout(pEvent->event,
												  pEvent->flag == true,
												  jifout);
	}
	else
	{
		result = wait_event_timeout(pEvent->event,
									pEvent->flag == true,
									jifout);
	}

	return result > 0;
}

// linux semaphore functions

bool ntv2SemaphoreOpen(Ntv2Semaphore* pSemaphore, Ntv2SystemContext* pSysCon, uint32_t count)
{
	if((pSemaphore == NULL) ||
	   (pSysCon == NULL) ||
	   (count == 0)) return false;

	// initialize semaphore data structure
	memset(pSemaphore, 0, sizeof(Ntv2Semaphore));

	sema_init(&pSemaphore->semaphore, count);

	return true;
}

void ntv2SemaphoreClose(Ntv2Semaphore* pSemaphore)
{
	if(pSemaphore == NULL) return;

	// initialize semaphore data structure
	memset(pSemaphore, 0, sizeof(Ntv2Semaphore));
}

bool ntv2SemaphoreDown(Ntv2Semaphore* pSemaphore, int64_t timeout)
{
	uint32_t jifout;
	int result;

	if(pSemaphore == NULL) return false;

	// convert to jiffies
	jifout = microsecondsToJiffies(timeout);

	// wait for our turn
	result = down_timeout(&pSemaphore->semaphore, jifout);

	return result == 0;
}

void ntv2SemaphoreUp(Ntv2Semaphore* pSemaphore)
{
	if(pSemaphore == NULL) return;

	up(&pSemaphore->semaphore);
}

int64_t ntv2TimeCounter(void)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,0,0))
	struct timespec64 ts64;

	ktime_get_real_ts64(&ts64);
	return (((int64_t)ts64.tv_sec * 1000000) + (ts64.tv_nsec / 1000));
#else
	struct timeval tv;
	do_gettimeofday(&tv);

	return ((int64_t)tv.tv_sec * 1000000 + tv.tv_usec);
#endif
}

int64_t ntv2TimeFrequency(void)
{
	return (int64_t)1000000;
}

int64_t ntv2Time100ns(void)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,0,0))
	struct timespec64 ts64;

	ktime_get_real_ts64(&ts64);
	return (((int64_t)ts64.tv_sec * 10000000) + (ts64.tv_nsec / 100));
#else	
	struct timeval tv;
	do_gettimeofday(&tv);

	return ((int64_t)tv.tv_sec * 1000000 + tv.tv_usec) * 10;
#endif
}

void ntv2TimeSleep(int64_t microseconds)
{
	if(microseconds == 0) return;

	if(microseconds > 1000)
	{
		msleep(microseconds >> 10); // approximate (/1000) with dividing by 1024
	}
	else
	{
		udelay(microseconds);
	}
}

static uint32_t microsecondsToJiffies(int64_t timeout)
{
	timeout = (timeout + (1000000/HZ - 1)) * HZ;
	do_div(timeout, 1000000);
	return (uint32_t)timeout;
}

bool ntv2ThreadOpen(Ntv2Thread* pThread, Ntv2SystemContext* pSysCon, const char* pName)
{
	if((pThread == NULL) ||
	   (pSysCon == NULL)) return false;

	// initialize semaphore data structure
	memset(pThread, 0, sizeof(Ntv2Thread));

	pThread->pName = pName;
	if(pThread->pName == NULL)
	{
		pThread->pName = "aja worker";
	}

	return true;
}

void ntv2ThreadClose(Ntv2Thread* pThread)
{
	if(pThread == NULL) return;
	
	ntv2ThreadStop(pThread);

	memset(pThread, 0, sizeof(Ntv2Thread));
}

int ntv2ThreadFunc(void* pData)
{
	Ntv2Thread* pThread = (Ntv2Thread*)pData;

	(*pThread->pFunc)(pThread->pContext);

	return 0;
}

bool ntv2ThreadRun(Ntv2Thread* pThread, Ntv2ThreadTask* pTask, void* pContext)
{
	if((pThread == NULL) ||
	   (pTask == NULL)) return false;

	if(pThread->run) return false;

	pThread->pFunc = pTask;
	pThread->pContext = pContext;
	pThread->run = true;

	pThread->pTask = kthread_run(ntv2ThreadFunc, (void*)pThread, pThread->pName);
	if(IS_ERR(pThread->pTask)) 
	{
		pThread->pTask = NULL;
		pThread->pFunc = NULL;
		pThread->pContext = NULL;
		pThread->run = false;
		return false;
	}

	return true;
}

void ntv2ThreadStop(Ntv2Thread* pThread)
{
	if(pThread == NULL) return;
	
	if(!pThread->run) return;
	pThread->run = false;

	kthread_stop(pThread->pTask);

	pThread->pTask = NULL;
	pThread->pFunc = NULL;
	pThread->pContext = NULL;

	return;
}

void ntv2ThreadExit(Ntv2Thread* pThread)
{
}

const char* ntv2ThreadGetName(Ntv2Thread* pThread)
{
	if(pThread == NULL) return NULL;
	return pThread->pName;
}

bool ntv2ThreadShouldStop(Ntv2Thread* pThread)
{
	return kthread_should_stop();
}

Ntv2Status ntv2ReadPciConfig(Ntv2SystemContext* pSysCon, void* pData, int32_t offset, int32_t size)
{
	int status;
	
	if ((pSysCon == NULL) || (pData == NULL))
		return NTV2_STATUS_BAD_PARAMETER;

	if (size == 4)
		status = pci_read_config_dword(pSysCon->pDevice, offset, (uint32_t*)pData);
	else if (size == 2)
		status = pci_read_config_word(pSysCon->pDevice, offset, (uint16_t*)pData);
	else if (size == 1)
		status = pci_read_config_byte(pSysCon->pDevice, offset, (uint8_t*)pData);
	else
		return NTV2_STATUS_BAD_PARAMETER;
		
	if (status != PCIBIOS_SUCCESSFUL)
		return NTV2_STATUS_IO_ERROR;
	
	return NTV2_STATUS_SUCCESS;
}

Ntv2Status ntv2WritePciConfig(Ntv2SystemContext* pSysCon, void* pData, int32_t offset, int32_t size)
{
	int status;
	
	if ((pSysCon == NULL)  || (pData == NULL))
		return NTV2_STATUS_BAD_PARAMETER;
	
	if (size == 4)
		status = pci_write_config_dword(pSysCon->pDevice, offset, *(uint32_t*)pData);
	else if (size == 2)
		status = pci_write_config_word(pSysCon->pDevice, offset, *(uint16_t*)pData);
	else if (size == 1)
		status = pci_write_config_byte(pSysCon->pDevice, offset, *(uint8_t*)pData);
	else
		return NTV2_STATUS_BAD_PARAMETER;
		
	if (status != PCIBIOS_SUCCESSFUL)
		return NTV2_STATUS_IO_ERROR;

	return NTV2_STATUS_SUCCESS;
}

#endif
