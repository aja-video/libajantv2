/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
///////////////////////////////////////////////////////////////
//
// Device Driver for AJA OEM devices.
//
////////////////////////////////////////////////////////////
//
// Filename: ntv2system.h
// Purpose:	 NTV2 system function abstraction
// Notes:	 
//
///////////////////////////////////////////////////////////////

#ifndef NTV2SYSTEM_H
#define NTV2SYSTEM_H

#define NTV2_MEMORY_ALIGN_DEFAULT	64
#define NTV2_MEMORY_ALIGN_MAX		4096

#if defined(AJAVirtual)

	#include <stdbool.h>
	#include <stddef.h>
	#include <stdint.h>
	#include <string.h>
	#include <stdio.h>
	#include <stdlib.h>

	#include "ajatypes.h"

	// virtual return codes

	typedef int				Ntv2Status;

	#define NTV2_STATUS_SUCCESS			(0)
	#define NTV2_STATUS_FAIL			(-1)
	#define NTV2_STATUS_NO_DEVICE		(-2)
	#define NTV2_STATUS_BAD_STATE		(-3)
	#define NTV2_STATUS_BAD_PARAMETER	(-4)
	#define NTV2_STATUS_NO_MEMORY		(-5)
	#define NTV2_STATUS_BUSY			(-6)
	#define NTV2_STATUS_IO_ERROR		(-7)
	#define NTV2_STATUS_TIMEOUT			(-8)
	#define NTV2_STATUS_NO_RESOURCES	(-9)

	// virtual try/catch

	#define NTV2_TRY	if (true)
	#define NTV2_CATCH	if (false)

	// virtual system context

	typedef struct ntv2_system_context
	{
	} Ntv2SystemContext;

	// virtual register abstraction

	typedef void*	Ntv2Register;	

	//MRBILL	#define ntv2WriteRegister32(reg, value)		
	//MRBILL	#define ntv2ReadRegister32(reg)				

	// virtual message abstraction

	#define ntv2Message(string, ...) 			printf(string, __VA_ARGS__)

	// virtual spinlock abstraction

	typedef struct ntv2_spinlock
	{
	} Ntv2SpinLock;

	// virtual interrupt lock abstraction

	typedef struct ntv2_interrupt_lock
	{
	} Ntv2InterruptLock;

	// virtual memory abstraction

	typedef void* 		Ntv2DmaAddress;
	typedef struct ntv2_dma_memory
	{
		void*				pAddress;
		Ntv2DmaAddress		dmaAddress;
		uint32_t			size;
	} Ntv2DmaMemory;

	// virtual user buffer abstraction

	typedef struct ntv2_user_buffer
	{
		void*				pAddress;
		uint32_t			size;
		bool				write;
	} Ntv2UserBuffer;

	// virtual dpc task abstraction

	typedef unsigned long 	Ntv2DpcData;
	typedef void Ntv2DpcTask(Ntv2DpcData data);

	typedef struct ntv2_dpc
	{
	} Ntv2Dpc;

	// virtual event abstraction

	typedef struct ntv2_event
	{
	} Ntv2Event;

	// virtual semaphore abstraction

	typedef struct ntv2_semaphore
	{
	} Ntv2Semaphore;

	// virtual thread abstraction

	typedef void Ntv2ThreadTask(void* pContext);
	typedef struct ntv2_thread
	{
		const char*			pName;
		Ntv2ThreadTask*		pFunc;				
		void*				pContext;
		bool				run;
	} Ntv2Thread;

//endif	//	defined(AJAVirtual)
#elif defined(MSWindows)

	#define NO_STRICT
	#include <ntifs.h>
	#include <ntddk.h>
	#include <wmilib.h>

	#pragma warning(disable:4201)
	#include <wdf.h>
	#pragma warning(default:4201)

	//#define NTSTRSAFE_LIB
	#include <ntstrsafe.h>
	#include <ntddser.h>

	#if !defined (__cplusplus)
		typedef unsigned char bool;
		#define true 1
		#define false 0
	#endif

	#include "ajatypes.h"
	#include "ntv2publicinterface.h"

	// windows return codes

	typedef NTSTATUS			Ntv2Status;

	#define NTV2_STATUS_SUCCESS			(STATUS_SUCCESS)
	#define NTV2_STATUS_FAIL			(STATUS_UNSUCCESSFUL)
	#define NTV2_STATUS_NO_DEVICE		(STATUS_NO_SUCH_DEVICE)
	#define NTV2_STATUS_BAD_STATE		(STATUS_INVALID_DEVICE_STATE)
	#define NTV2_STATUS_BAD_PARAMETER	(STATUS_INVALID_PARAMETER)
	#define NTV2_STATUS_NO_MEMORY		(STATUS_NO_MEMORY)
	#define NTV2_STATUS_BUSY			(STATUS_DEVICE_BUSY)
	#define NTV2_STATUS_IO_ERROR		(STATUS_IO_DEVICE_ERROR)
	#define NTV2_STATUS_TIMEOUT			(STATUS_IO_TIMEOUT)
	#define NTV2_STATUS_NO_RESOURCES	(STATUS_INSUFFICIENT_RESOURCES)

	// windows system context
	typedef struct ntv2_system_context
	{
		uint32_t					devNum;				// device number
		WDFDRIVER					wdfDriver;			// wdf driver
		WDFDEVICE					wdfDevice;			// wdf device
		WDFINTERRUPT				wdfInterrupt;		// wdf interrupt
		WDFDMAENABLER				wdfDmaEnabler;		// wdf dma enabler
		ULONG						busNumber;			// device pci bus number
		BUS_INTERFACE_STANDARD		BusInterface;		// windows bus interface
		PBUS_INTERFACE_STANDARD		pBusInterface;		// bus interface pointer
	} Ntv2SystemContext;

	// windows register abstraction

	typedef uint8_t*		Ntv2Register;

	#define ntv2WriteRegister32(reg, value)		WRITE_REGISTER_ULONG((ULONG*)(reg), (ULONG)(value))
	#define ntv2ReadRegister32(reg)				READ_REGISTER_ULONG((ULONG*)(reg))

	// windows message abstraction

	#define ntv2Message(string, ...) 			DbgPrint(string, __VA_ARGS__)

	// windows spinlock abstraction

	typedef struct ntv2_spinlock
	{
		KSPIN_LOCK			lock;
		KIRQL				irql;
	} Ntv2SpinLock;

	// windows interrupt lock abstraction

	typedef struct ntv2_interrupt_lock
	{
		WDFINTERRUPT		wdfInterrupt;
		bool				locked;
	} Ntv2InterruptLock;

	// windows memory abstraction

	typedef int64_t		Ntv2DmaAddress;
	typedef struct ntv2_dma_memory
	{
		WDFCOMMONBUFFER		wdfCommonBuffer;
		PVOID				pAddress;
		Ntv2DmaAddress		dmaAddress;
		uint32_t			size;
	} Ntv2DmaMemory;

	// windows user buffer abstraction

	typedef struct ntv2_user_buffer
	{
		PVOID				pAddress;
		uint32_t			size;
		bool				write;
	} Ntv2UserBuffer;

	// windows dpc task abstraction

	typedef uint64_t	Ntv2DpcData;
	typedef void Ntv2DpcTask(Ntv2DpcData data);

	typedef struct ntv2_dpc
	{
		WDFDPC				wdfDpc;
	} Ntv2Dpc;

	// windows event abstraction

	typedef struct ntv2_event
	{
		KEVENT 				event;
	} Ntv2Event;

	// windows semaphore abstraction

	typedef struct ntv2_semaphore
	{
		KSEMAPHORE			semaphore;
	} Ntv2Semaphore;

	// windows thread abstraction

	typedef void Ntv2ThreadTask(void* pContext);
	typedef struct ntv2_thread
	{
		PKTHREAD			pTask;
		const char			*pName;
		Ntv2ThreadTask*		pFunc;				
		void				*pContext;
		bool				run;
	} Ntv2Thread;

//endif	//	defined(MSWindows)
#elif defined(AJAMac)

	#include "ajatypes.h"
	#if defined(NTV2_BUILDING_DRIVER)
		#include <IOKit/IOLocks.h>
		#include <IOKit/IOLib.h>
		#include "MacLog.h"
	#endif

	// Mac return codes
	typedef IOReturn					Ntv2Status;
	#define NTV2_STATUS_SUCCESS			(kIOReturnSuccess)
	#define NTV2_STATUS_FAIL			(kIOReturnError)
	#define NTV2_STATUS_NO_DEVICE		(kIOReturnNoDevice)
	#define NTV2_STATUS_BAD_STATE		(kIOReturnInvalid)
	#define NTV2_STATUS_BAD_PARAMETER	(kIOReturnBadArgument)
	#define NTV2_STATUS_NO_MEMORY		(kIOReturnNoMemory)
	#define NTV2_STATUS_BUSY			(kIOReturnBusy)
	#define NTV2_STATUS_IO_ERROR		(kIOReturnIPCError)
	#define NTV2_STATUS_TIMEOUT			(kIOReturnTimeout)
	#define NTV2_STATUS_NO_RESOURCES	(kIOReturnNoResources)

	// windows message abstraction

	#define ntv2Message(string, ...) 			DebugLog(string, __VA_ARGS__)

	// Mac system context

	typedef void* ntv2_mac_driver_ref;

	typedef struct ntv2_system_context
	{
		ntv2_mac_driver_ref		macDriverRef;
	} Ntv2SystemContext;

	// Mac register abstraction

	typedef uint8_t*		Ntv2Register;

	// Mac spinlock abstraction
	//class IOSimpleLock;
	typedef struct ntv2_spinlock
	{
		IOSimpleLock*			lock;
	} Ntv2SpinLock;

	// Mac event abstraction
	//class IORecursiveLock;
	typedef struct ntv2_event
	{
		IORecursiveLock*	pRecursiveLock;
		bool				flag;
	} Ntv2Event;


	// Mac thread abstraction

	typedef void Ntv2ThreadTask(void* pContext);
	typedef struct ntv2_thread
	{
		thread_t			pTask;
		const char			*pName;
		Ntv2ThreadTask*		pFunc;
		void				*pContext;
		bool				run;
	} Ntv2Thread;

//endif	//	defined(AJAMac)
#elif defined(AJALinux)

	// linux system headers

	#if defined(CONFIG_SMP)
	#define __SMP__
	#endif

	#include <linux/version.h>
	#include <linux/module.h>
	#include <linux/fs.h>
	#include <linux/pci.h>
	#include <linux/cdev.h>
	#include <linux/sched.h>
	#include <linux/interrupt.h>
	#include <linux/delay.h>
	#include <linux/time.h>
	#include <linux/kthread.h>
	#include <linux/uaccess.h>
	#include <linux/vmalloc.h>
	#include <linux/serial.h>
	#include <linux/serial_core.h>
	#include <linux/tty.h>
	#include <linux/tty_flip.h>

	#include "ajatypes.h"

	// linux return codes

	typedef int				Ntv2Status;

	#define NTV2_STATUS_SUCCESS			(0)
	#define NTV2_STATUS_FAIL			(-EAGAIN)
	#define NTV2_STATUS_NO_DEVICE		(-ENODEV)
	#define NTV2_STATUS_BAD_STATE		(-EPERM)
	#define NTV2_STATUS_BAD_PARAMETER	(-EINVAL)
	#define NTV2_STATUS_NO_MEMORY		(-ENOMEM)
	#define NTV2_STATUS_BUSY			(-EBUSY)
	#define NTV2_STATUS_IO_ERROR		(-EIO)
	#define NTV2_STATUS_TIMEOUT			(-ETIME)
	#define NTV2_STATUS_NO_RESOURCES	(-ENOMEM)

	// linux system context

	typedef struct ntv2_system_context
	{
		uint32_t			devNum;				// device number
		struct pci_dev*		pDevice;			// linux pci device
		uint32_t			busNumber;			// pci bus number
	} Ntv2SystemContext;

	// linux register abstraction

	typedef void __iomem*	Ntv2Register;	

	#define ntv2WriteRegister32(reg, value)		iowrite32((uint32_t)(value), (void*)(reg))
	#define ntv2ReadRegister32(reg)				ioread32((void*)(reg))

	// linux message abstraction

	#define ntv2Message(string, ...) 			printk(KERN_ALERT string, __VA_ARGS__)

	// linux spinlock abstraction

	typedef struct ntv2_spinlock
	{
		spinlock_t			lock;
	} Ntv2SpinLock;

	// linux interrupt lock abstraction

	typedef struct ntv2_interrupt_lock
	{
		spinlock_t			lock;
		unsigned long		flags;
	} Ntv2InterruptLock;

	// linux memory abstraction

	typedef dma_addr_t 		Ntv2DmaAddress;
	typedef struct ntv2_dma_memory
	{
		struct pci_dev*		pDevice;
		void*				pAddress;
		Ntv2DmaAddress		dmaAddress;
		uint32_t			size;
	} Ntv2DmaMemory;

	// linux user buffer abstraction

	typedef struct ntv2_user_buffer
	{
		void*				pAddress;
		uint32_t			size;
		bool				write;
	} Ntv2UserBuffer;

	// linux dpc task abstraction

	typedef unsigned long 	Ntv2DpcData;
	typedef void Ntv2DpcTask(Ntv2DpcData data);

	typedef struct ntv2_dpc
	{
		struct tasklet_struct	tasklet;
	} Ntv2Dpc;

	// linux event abstraction

	typedef struct ntv2_event
	{
		wait_queue_head_t	event;
		bool				flag;
	} Ntv2Event;

	// linux semaphore abstraction

	typedef struct ntv2_semaphore
	{
		struct semaphore	semaphore;
	} Ntv2Semaphore;

	// linux thread abstraction

	typedef void Ntv2ThreadTask(void* pContext);
	typedef struct ntv2_thread
	{
		const char*			pName;
		struct task_struct*	pTask;
		Ntv2ThreadTask*		pFunc;				
		void*				pContext;
		bool				run;
	} Ntv2Thread;

#endif	//	defined(AJALinux)

#if defined (AJAMac)
	// Mac register read/write
	uint32_t	ntv2ReadRegister32(Ntv2SystemContext* context, uint32_t regNum);
	void		ntv2WriteRegister32(Ntv2SystemContext* context, uint32_t regNum, uint32_t regValue);
#endif

uint32_t ntv2ReadRegister(Ntv2SystemContext* context, uint32_t regnNum);
bool ntv2ReadRegisterMS(Ntv2SystemContext* context, uint32_t regnum, uint32_t* data, uint32_t regMask, uint32_t regShift);
bool ntv2WriteRegister(Ntv2SystemContext* context, uint32_t regnum, uint32_t data);
bool ntv2WriteRegisterMS(Ntv2SystemContext* context, uint32_t regnum, uint32_t data, uint32_t regMask, uint32_t regShift);
uint32_t ntv2ReadVirtualRegister(Ntv2SystemContext* context, uint32_t regNum);
bool ntv2WriteVirtualRegister(Ntv2SystemContext* context, uint32_t regNum, uint32_t data);

// spinlock functions

bool		ntv2SpinLockOpen(Ntv2SpinLock* pSpinLock, Ntv2SystemContext* pSysCon);
void		ntv2SpinLockClose(Ntv2SpinLock* pSpinLock);
void		ntv2SpinLockAcquire(Ntv2SpinLock* pSpinLock);
void		ntv2SpinLockRelease(Ntv2SpinLock* pSpinLock);

// memory functions

void*		ntv2MemoryAlloc(uint32_t size);
void		ntv2MemoryFree(void* pAddress, uint32_t size);

// event functions

bool		ntv2EventOpen(Ntv2Event* pEvent, Ntv2SystemContext* pSysCon);
void		ntv2EventClose(Ntv2Event* pEvent);
void		ntv2EventSignal(Ntv2Event* pEvent);
void		ntv2EventClear(Ntv2Event* pEvent);
bool		ntv2EventWaitForSignal(Ntv2Event* pEvent, int64_t timeout, bool alert);

// sleep function

void		ntv2TimeSleep(int64_t microseconds);

// kernel thread

bool		ntv2ThreadOpen(Ntv2Thread* pThread, Ntv2SystemContext* pSysCon, const char* pName);
void		ntv2ThreadClose(Ntv2Thread* pThread);
bool		ntv2ThreadRun(Ntv2Thread* pThread, Ntv2ThreadTask* pTask, void* pContext);
void		ntv2ThreadStop(Ntv2Thread* pThread);
const char* ntv2ThreadGetName(Ntv2Thread* pThread);
bool		ntv2ThreadShouldStop(Ntv2Thread* pThread);
void		ntv2ThreadExit(Ntv2Thread* pThread);

// pci configuration space
Ntv2Status	ntv2ReadPciConfig(Ntv2SystemContext* pSysCon, void* pData, int32_t offset, int32_t size);
Ntv2Status	ntv2WritePciConfig(Ntv2SystemContext* pSysCon, void* pData, int32_t offset, int32_t size);

#if defined(MSWindows) || defined(AJALinux) || defined(AJAVirtual)

	// interrupt lock functions

	bool		ntv2InterruptLockOpen(Ntv2InterruptLock* pInterruptLock, Ntv2SystemContext* pSysCon);
	void		ntv2InterruptLockClose(Ntv2InterruptLock* pInterruptLock);
	void		ntv2InterruptLockAcquire(Ntv2InterruptLock* pInterruptLock);
	void		ntv2InterruptLockRelease(Ntv2InterruptLock* pInterruptLock);
	
	// dma functions
	bool		ntv2DmaMemoryAlloc(Ntv2DmaMemory* pDmaMemory, Ntv2SystemContext* pSysCon, uint32_t size);
	void		ntv2DmaMemoryFree(Ntv2DmaMemory* pDmaMemory);
	void*		ntv2DmaMemoryVirtual(Ntv2DmaMemory* pDmaMemory);
	Ntv2DmaAddress ntv2DmaMemoryPhysical(Ntv2DmaMemory* pDmaMemory);
	uint32_t	ntv2DmaMemorySize(Ntv2DmaMemory* pDmaMemory);

	// user buffer functions

	bool		ntv2UserBufferPrepare(Ntv2UserBuffer* pUserBuffer, Ntv2SystemContext* pSysCon, uint64_t address, uint32_t size, bool write);
	void		ntv2UserBufferRelease(Ntv2UserBuffer* pUserBuffer);
	bool		ntv2UserBufferCopyTo(Ntv2UserBuffer* pDstBuffer, uint32_t dstOffset, void* pSrcAddress, uint32_t size);
	bool		ntv2UserBufferCopyFrom(Ntv2UserBuffer* pSrcBuffer, uint32_t srcOffset, void* pDstAddress, uint32_t size);

	// dpc task functions

	bool		ntv2DpcOpen(Ntv2Dpc* pDpc, Ntv2SystemContext* pSysCon, Ntv2DpcTask* pDpcTask, Ntv2DpcData dpcData);
	void		ntv2DpcClose(Ntv2Dpc* pDpc);
	void		ntv2DpcSchedule(Ntv2Dpc* pDpc);

	// semaphore functions

	bool		ntv2SemaphoreOpen(Ntv2Semaphore* pSemaphore, Ntv2SystemContext* pSysCon, uint32_t count);
	void		ntv2SemaphoreClose(Ntv2Semaphore* pSemaphore);
	bool		ntv2SemaphoreDown(Ntv2Semaphore* pSemaphore, int64_t timeout);
	void		ntv2SemaphoreUp(Ntv2Semaphore* pSemaphore);

	// time functions

	int64_t		ntv2TimeCounter(void);
	int64_t		ntv2TimeFrequency(void);
	int64_t		ntv2Time100ns(void);
#endif	//	defined(MSWindows) || defined(AJALinux)

#endif	//	NTV2SYSTEM_H
