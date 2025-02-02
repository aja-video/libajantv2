/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
///////////////////////////////////////////////////////////////
//
// NTV2 Linux v2.6 Device Driver for AJA devices.
//
////////////////////////////////////////////////////////////
//
// Filename: registerio.h
// Purpose:	 Read/write real and virtual registers header file
// Notes:
//
///////////////////////////////////////////////////////////////

#ifndef REGISTERIO_H
#define REGISTERIO_H

#include <linux/wait.h>

//As of 2.6.26 semaphore is in include/linux
#include <linux/version.h>
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,25))
# include <linux/semaphore.h>
#else
# include <asm/semaphore.h>
#endif

// Some kernel version sensitive macro-rama
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22)
#define NTV2_LINUX_IRQ_SHARED_FLAG IRQF_SHARED
#define NTV2_LINUX_IRQ_SAMPLE_RANDOM_FLAG IRQF_SAMPLE_RANDOM
#else
#define NTV2_LINUX_IRQ_SHARED_FLAG SA_SHIRQ
#define NTV2_LINUX_IRQ_SAMPLE_RANDOM_FLAG SA_SAMPLE_RANDOM
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18)
#define NTV2_LINUX_PCI_REG_DRIVER_FUNC(x) pci_register_driver(x)
#else
#define NTV2_LINUX_PCI_REG_DRIVER_FUNC(x) pci_module_init(x)
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)
# define NTV2_LINUX_SG_INIT_TABLE_FUNC(x,y) sg_init_table((x),(y))
#else
# define NTV2_LINUX_SG_INIT_TABLE_FUNC(x,y) memset((x),0,sizeof(struct scatterlist) * (y));
#endif

#include "ntv2driverautocirculate.h"
#include "../ntv2hdmiin.h"
#include "../ntv2hdmiin4.h"
#include "../ntv2kona.h"
#include "../ntv2setup.h"
#include "../ntv2genlock2.h"
#include "../ntv2videoraster.h"

// clean old stuff
#define FGVCROSSPOINTMASK (BIT_0+BIT_1+BIT_2+BIT_3)
#define FGVCROSSPOINTSHIFT (0)
#define BGVCROSSPOINTMASK (BIT_4+BIT_5+BIT_6+BIT_7)
#define BGVCROSSPOINTSHIFT (4)
#define FGKCROSSPOINTMASK (BIT_8+BIT_9+BIT_10+BIT_11)
#define FGKCROSSPOINTSHIFT (8)
#define BGKCROSSPOINTMASK (BIT_12+BIT_13+BIT_14+BIT_15)
#define BGKCROSSPOINTSHIFT (12)

#define VIDPROCMUX1MASK (BIT_0+BIT_1)
#define VIDPROCMUX1SHIFT (0)
#define VIDPROCMUX2MASK (BIT_2+BIT_3)
#define VIDPROCMUX2SHIFT (2)
#define VIDPROCMUX3MASK (BIT_4+BIT_5)
#define VIDPROCMUX3SHIFT (4)
#define VIDPROCMUX4MASK (BIT_6+BIT_7)
#define VIDPROCMUX4SHIFT (6)
#define VIDPROCMUX5MASK (BIT_8+BIT_9)
#define VIDPROCMUX5SHIFT (8)

// Comment out SOFTWARE_UART_FIFO define to turn off software FIFO completely.
//# define SOFTWARE_UART_FIFO
# ifdef SOFTWARE_UART_FIFO
// Comment out either UARTRXFIFOSIZE or UARTTXFIFOSIZE to disable
// Rx or Tx software FIFO.
# define UARTRXFIFOSIZE	20
# define UARTTXFIFOSIZE	20
# endif	// SOFTWARE_UART_FIFO

//#define FPGA_REQ_FOR_DMA	(eFPGAVideoProc)
//#define NTV2_MAX_FPGA		(eFPGAVideoProc)

#define NTV2_MAX_HDMI_MONITOR	4
#define NTV2_MAX_DMA_STREAMS    8

// module driver mode
typedef enum _NTV2DriveMode
{
    eDriverModeAll = 0,     // support all driver functions
    eDriverModeRegister,    // only support register access
    eDriverModeGenlock      // register access with genlock initialization
} NTV2DriverMode;

// Singleton module params
typedef struct ntv2_module_private
{
	int		NTV2Major;
	UWord	numNTV2Devices;
	char *	name;
	char *	driverName;
	ULWord	intrBitLut[eNumInterruptTypes];
	struct class *class;
    NTV2DriverMode driverMode;

	// uart driver
	struct uart_driver 			*uart_driver;
	u32							uart_max;
	atomic_t					uart_index;

} NTV2ModulePrivateParams;

typedef struct _fileData
{
	DMA_PAGE_ROOT dmaRoot;
} FILE_DATA, *PFILE_DATA;

typedef enum
{
	eIrqFpga = 0,
	eNumNTV2IRQDevices
} ntv2_irq_device_t;

// The ntv2_irq_device_t enums must match up with the array of function
// pointers below.

// Params that are per-device
typedef struct ntv2_private
{
	struct pci_dev *pci_dev;
	ULWord deviceNumber;
	ULWord busNumber;
	ULWord pci_device;
	char name[16];
	struct cdev cdev;

	// Base Address Values
	unsigned long _unmappedBAR0Address;
	unsigned long _mappedBAR0Address;
	ULWord _BAR0MemorySize;

	unsigned long _unmappedBAR1Address;
	unsigned long _mappedBAR1Address;
	ULWord _BAR1MemorySize;

	unsigned long _unmappedBAR2Address;
	unsigned long _mappedBAR2Address;
	ULWord _BAR2MemorySize;

	// Holds the number of hardware registers this device supports
	// Obtained from ntv2devicefeatures.cpp
	ULWord _numberOfHWRegisters;

	// Reserved HighMem DMA Buffer if needed.
	// used when setting boot option mem = totalmem-16Megs for SD or 64 Megs for HD.
	// this is NumDmaDriverBuffers frames worth.

	// These are the video registers
	unsigned long _VideoAddress;
	ULWord _VideoMemorySize;

	// The NWL dma registers actually live in BAR0, but usually BAR0 is used to map
	// the video control registers, so use a different set of variables for NWL to avoid confusion.
	unsigned long _NwlAddress;
	ULWord _NwlMemorySize;

	// The Xlnx dma registers live in 64K BAR1.
	unsigned long _XlnxAddress;
	ULWord _XlnxMemorySize;

	// The bigphysarea kernel patch returns a virtual address to contiguous LowMem
	unsigned long _dmaBuffer;

	caddr_t		_virtBuffer;
	ULWord 		_bigphysBufferSize;	// # of bytes allocated.

	// Pointers to Channel 1 and Channel 2 Frame Buffers
	// This pointer along with the value of _pCh1Control or _pCh1Control
	// register to address a given frame in the frame buffer.
//	unsigned long* _Ch1BaseAddress;
//	unsigned long* _Ch2BaseAddress;

	// Registers - See hdntv.pdf or Ntv2.pdf for details.

	unsigned long _pGlobalControl;
	unsigned long _pGlobalControl2;

	// clean old stuff
	unsigned long _pVideoProcessingControl;
	unsigned long _pVideoProcessingCrossPointControl;

	// interrupt
	unsigned long _pInterruptControl;
	unsigned long _pInterruptControl2;

	unsigned long _pStatus;
	unsigned long _pStatus2;

	// audio control
	unsigned long _pAudioControl;
	unsigned long _pAudioSource;
	unsigned long _pAudioLastOut;
	unsigned long _pAudioLastIn;

	unsigned long _pAudio2Control;
	unsigned long _pAudio2Source;
	unsigned long _pAudio2LastOut;
	unsigned long _pAudio2LastIn;

	unsigned long _pAudio3Control;
	unsigned long _pAudio3Source;
	unsigned long _pAudio3LastOut;
	unsigned long _pAudio3LastIn;

	unsigned long _pAudio4Control;
	unsigned long _pAudio4Source;
	unsigned long _pAudio4LastOut;
	unsigned long _pAudio4LastIn;

	unsigned long _pAudio5Control;
	unsigned long _pAudio5Source;
	unsigned long _pAudio5LastOut;
	unsigned long _pAudio5LastIn;

	unsigned long _pAudio6Control;
	unsigned long _pAudio6Source;
	unsigned long _pAudio6LastOut;
	unsigned long _pAudio6LastIn;

	unsigned long _pAudio7Control;
	unsigned long _pAudio7Source;
	unsigned long _pAudio7LastOut;
	unsigned long _pAudio7LastIn;

	unsigned long _pAudio8Control;
	unsigned long _pAudio8Source;
	unsigned long _pAudio8LastOut;
	unsigned long _pAudio8LastIn;

	unsigned long _pAudioSampleCounter;

	// aja dma registers
	unsigned long _pDMA1HostAddress;
	unsigned long _pDMA1LocalAddress;
	unsigned long _pDMA1TransferCount;
	unsigned long _pDMA1NextDescriptor;
	unsigned long _pDMA2HostAddress;
	unsigned long _pDMA2LocalAddress;
	unsigned long _pDMA2TransferCount;
	unsigned long _pDMA2NextDescriptor;
	unsigned long _pDMA3HostAddress;
	unsigned long _pDMA3LocalAddress;
	unsigned long _pDMA3TransferCount;
	unsigned long _pDMA3NextDescriptor;
	unsigned long _pDMA4HostAddress;
	unsigned long _pDMA4LocalAddress;
	unsigned long _pDMA4TransferCount;
	unsigned long _pDMA4NextDescriptor;

	unsigned long _pDMA1HostAddressHigh;
	unsigned long _pDMA1NextDescriptorHigh;
	unsigned long _pDMA2HostAddressHigh;
	unsigned long _pDMA2NextDescriptorHigh;
	unsigned long _pDMA3HostAddressHigh;
	unsigned long _pDMA3NextDescriptorHigh;
	unsigned long _pDMA4HostAddressHigh;
	unsigned long _pDMA4NextDescriptorHigh;

	unsigned long _pDMAControlStatus;
	unsigned long _pDMAInterruptControl;

	bool		  _bMultiChannel;

	unsigned long _pDeviceID;			// device ID register

# ifdef SOFTWARE_UART_FIFO
#  ifdef UARTTXFIFOSIZE
	unsigned long _pUARTTransmitData;
#  endif
#  ifdef UARTRXFIFOSIZE
	unsigned long _pUARTReceiveData;
#  endif
	unsigned long _pUARTControl;

#  ifdef UARTTXFIFOSIZE
	unsigned long _pUARTTransmitData2;
#  endif
#  ifdef UARTRXFIFOSIZE
	unsigned long _pUARTReceiveData2;
#  endif
	unsigned long _pUARTControl2;
# endif	// SOFTWARE_UART_FIFO
	NTV2DeviceID _DeviceID;		// device ID value

	wait_queue_head_t       _interruptWait[eNumInterruptTypes];

	ULWord64 				_interruptCount[eNumInterruptTypes];
	unsigned long			_interruptHappened[eNumInterruptTypes];

	struct semaphore        _I2CMutex;

	// dma engines
	NTV2DmaMethod 			_dmaMethod;
	DMA_ENGINE				_dmaEngine[DMA_NUM_ENGINES];
	ULWord					_dmaNumEngines;
	struct semaphore        _dmaSerialSemaphore;
	int						_numXlnxH2CEngines;
	int						_numXlnxC2HEngines;

	// Autocirculate stuff.

	INTERNAL_AUTOCIRCULATE_STRUCT _AutoCirculate[NUM_CIRCULATORS];
	NTV2Crosspoint _LkUpAcChanSpecGivenEngine[NUM_CIRCULATORS];

	//audio clock!
	ULWord _ulNumberOfWrapsOfClockSampleCounter;//The software sample counter is 64 bit...
												// So this keeps track of the number of wraps
												// around of the 32-bit HW counter.
	ULWord _ulLastClockSampleCounter; //value the register had last time it was read;

	spinlock_t  _registerSpinLock;
	spinlock_t  _autoCirculateLock;
	spinlock_t  _virtualRegisterLock;
	spinlock_t  _nwlRegisterLock[NUM_NWL_REGS];
	spinlock_t  _p2pInterruptControlRegisterLock;
	spinlock_t  _audioClockLock;
	spinlock_t	_bankAndRegisterAccessLock;
	struct semaphore	_mailBoxSemaphore;

	NTV2_GlobalAudioPlaybackMode _globalAudioPlaybackMode;

	ULWord      _videoBitfileProgramming;
	bool        _startAudioNextFrame;

	bool        _bitFileInfoSet[eFPGA_NUM_FPGAs];
	BITFILE_INFO_STRUCT bitFileInfo[eFPGA_NUM_FPGAs];

	VirtualProcAmpRegisters			_virtualProcAmpRegisters;
	HardwareProcAmpRegisterImage	_hwProcAmpRegisterImage;

# ifdef SOFTWARE_UART_FIFO
#  ifdef UARTRXFIFOSIZE
	spinlock_t uartRxFifoLock;
	UByte uartRxFifo[UARTRXFIFOSIZE];
	ULWord uartRxFifoSize;
	bool uartRxFifoOverrun;

	spinlock_t uartRxFifoLock2;
	UByte uartRxFifo2[UARTRXFIFOSIZE];
	ULWord uartRxFifoSize2;
	bool uartRxFifoOverrun2;
#  endif

#  ifdef UARTTXFIFOSIZE
	spinlock_t uartTxFifoLock;
	UByte uartTxFifo[UARTTXFIFOSIZE];
	ULWord uartTxFifoSize;

	spinlock_t uartTxFifoLock2;
	UByte uartTxFifo2[UARTTXFIFOSIZE];
	ULWord uartTxFifoSize2;
#  endif
# endif	// SOFTWARE_UART_FIFO

    spinlock_t ioLock;        // lock for io reference count
    uint32_t   ioCount;       // io reference count for device removal
    bool       ioRemove;      // device has been removed
    bool       hotplug;       // device is hotpluggable
    
	unsigned int _ntv2IRQ[eNumNTV2IRQDevices];

	ULWord _audioSyncTolerance;
	ULWord _dmaSerialize;
	ULWord _syncChannels;

	NTV2Crosspoint _syncChannel1;
	NTV2Crosspoint _syncChannel2;

	// Virtual registers
	ULWord  _virtualRegisterMem[MAX_NUM_VIRTUAL_REGISTERS];

	//
	// Control panel additions
	//
	ULWord _ApplicationPID;						// 10184
	ULWord _ApplicationCode;					// 10185

	ULWord _ApplicationReferenceCount;			// 10326 and 10327

	ULWord _VirtualMailBoxTimeoutNS;			// 10478	//	Units are 100 ns, not nanoseconds!

	// P2P  -  Peer to peer messaging
	unsigned long _pMessageChannel1;			// control register kerenel address
	unsigned long _pMessageChannel2;
	unsigned long _pMessageChannel3;
	unsigned long _pMessageChannel4;
	unsigned long _pMessageChannel5;
	unsigned long _pMessageChannel6;
	unsigned long _pMessageChannel7;
	unsigned long _pMessageChannel8;

	unsigned long _pPhysicalMessageChannel1;	// control registere bus address
	unsigned long _pPhysicalMessageChannel2;
	unsigned long _pPhysicalMessageChannel3;
	unsigned long _pPhysicalMessageChannel4;
	unsigned long _pPhysicalMessageChannel5;
	unsigned long _pPhysicalMessageChannel6;
	unsigned long _pPhysicalMessageChannel7;
	unsigned long _pPhysicalMessageChannel8;

	unsigned long _pMessageInterruptStatus;		// kerenel address
	unsigned long _pMessageInterruptControl;

	unsigned long _pPhysicalOutputChannel1;		// bus address
	unsigned long _pPhysicalOutputChannel2;
	unsigned long _pPhysicalOutputChannel3;
	unsigned long _pPhysicalOutputChannel4;
	unsigned long _pPhysicalOutputChannel5;
	unsigned long _pPhysicalOutputChannel6;
	unsigned long _pPhysicalOutputChannel7;
	unsigned long _pPhysicalOutputChannel8;

	unsigned long _FrameAperturePhysicalAddress;	// bus
	unsigned long _FrameApertureBaseAddress;		// kernel
	ULWord		  _FrameApertureBaseSize;
	unsigned long _pFrameApertureOffset;

	ULWord _PCIDeviceControlOffset;

	Ntv2SystemContext		systemContext;
    struct ntv2_genlock		*m_pGenlockMonitor;
    struct ntv2_genlock2	*m_pGenlock2Monitor;
    struct ntv2_videoraster *m_pRasterMonitor;
	struct ntv2_hdmiin		*m_pHDMIInputMonitor[NTV2_MAX_HDMI_MONITOR];
	struct ntv2_hdmiin4		*m_pHDMIIn4Monitor[NTV2_MAX_HDMI_MONITOR];
	struct ntv2_hdmiout4	*m_pHDMIOut4Monitor[NTV2_MAX_HDMI_MONITOR];
	struct ntv2_serial		*m_pSerialPort;
	struct ntv2_mcap		*m_pBitstream;
	struct ntv2_setup		*m_pSetupMonitor;
    struct ntv2_stream  	*m_pDmaStream[NTV2_MAX_DMA_STREAMS];

    DMA_ENGINE              *m_pMapEngine[DMA_NUM_ENGINES];
    DMA_ENGINE              *m_pMapStream[DMA_NUM_ENGINES];
    
	bool registerEnable;
	bool serialActive;
	
#if defined(AJA_HEVC)
	unsigned long _hevcDevNum;
#endif

	ULWord _AncF2StartMemory[NTV2_MAX_NUM_CHANNELS];
	ULWord _AncF2StopMemory[NTV2_MAX_NUM_CHANNELS];
	ULWord _AncF2Size[NTV2_MAX_NUM_CHANNELS];
} NTV2PrivateParams;

NTV2ModulePrivateParams * getNTV2ModuleParams(void);
NTV2PrivateParams * getNTV2Params(unsigned int deviceNumber);

// Null mask and null shift for ReadRegister and WriteRegister
//
#define NO_MASK	(0xFFFFFFFF)
#define NO_SHIFT	(0)

#define NTV2REGWRITEMODEMASK (BIT_20+BIT_21)
#define NTV2REGWRITEMODESHIFT (20)
#define NTV2LEDSTATEMASK (BIT_16+BIT_17+BIT_18+BIT_19)
#define NTV2LEDSTATESHIFT (16)

// Billions and billions of prototypes for reading and writing registers
//
ULWord READ_REGISTER_ULWord( ULWord deviceNumber, unsigned long address);
ULWord READ_REGISTER_UWord( ULWord deviceNumber, unsigned long address);
ULWord READ_REGISTER_UByte( ULWord deviceNumber, unsigned long address);

void WRITE_REGISTER_ULWord( ULWord deviceNumber, unsigned long address, ULWord regValue);
void WRITE_REGISTER_UWord( ULWord deviceNumber, unsigned long address, ULWord regValue);
void WRITE_REGISTER_UByte( ULWord deviceNumber, unsigned long address, ULWord regValue);

void GetActiveFrameBufferSize(ULWord deviceNumber,NTV2FrameDimensions * frameBufferSize);

// Write a single register with mask and shift
void WriteRegister(	ULWord deviceNumber,
					ULWord registerNumber,
					ULWord registerValue,
					ULWord registerMask,
					ULWord registerShift);

int WriteReg(	ULWord deviceNumber,
				ULWord registerNumber,
				ULWord registerValue,
				ULWord registerMask,
				ULWord registerShift);

// Write a group of registers as a block
void WriteRegisterBufferULWord(	ULWord deviceNumber,
								ULWord registerNumber,
								ULWord* sourceData,
								ULWord sourceDataSizeULWords);

ULWord ReadRegister(    ULWord deviceNumber,
                        ULWord registerNumber,
                        ULWord registerMask,
                        ULWord registerShift);

int ReadReg(    ULWord deviceNumber,
                ULWord registerNumber,
                ULWord* registerValue,
                ULWord registerMask,
                ULWord registerShift);

// old stuff
void WriteVideoProcessingControl(ULWord deviceNumber,ULWord value);
ULWord ReadVideoProcessingControl(ULWord deviceNumber);
void WriteVideoProcessingControlCrosspoint(ULWord deviceNumber,ULWord value);
ULWord ReadVideoProcessingControlCrosspoint(ULWord deviceNumber);

void WriteInterruptRegister(ULWord deviceNumber ,ULWord value);
ULWord ReadInterruptRegister(ULWord deviceNumber);
ULWord ReadStatusRegister(ULWord deviceNumber);

void WriteInterrupt2Register(ULWord deviceNumber ,ULWord value);
ULWord ReadInterrupt2Register(ULWord deviceNumber);
ULWord ReadStatus2Register(ULWord deviceNumber);

void SetRegisterWriteMode(ULWord deviceNumber, NTV2Channel channel, NTV2RegisterWriteMode value);
NTV2RegisterWriteMode GetRegisterWriteMode(ULWord deviceNumber, NTV2Channel channel);

void SetLEDState(ULWord deviceNumber,ULWord value);
ULWord GetLEDState(ULWord deviceNumber);

void SetSingleLED(ULWord deviceNumber,ULWord bitNum);
void ClearSingleLED(ULWord deviceNumber,ULWord bitNum);

// audio out last
ULWord ReadAudioLastOut(ULWord deviceNumber);
ULWord ReadAudioLastOut2(ULWord deviceNumber);
ULWord ReadAudioLastOut3(ULWord deviceNumber);
ULWord ReadAudioLastOut4(ULWord deviceNumber);
ULWord ReadAudioLastOut5(ULWord deviceNumber);
ULWord ReadAudioLastOut6(ULWord deviceNumber);
ULWord ReadAudioLastOut7(ULWord deviceNumber);
ULWord ReadAudioLastOut8(ULWord deviceNumber);

// audio in last
ULWord ReadAudioLastIn(ULWord deviceNumber);
ULWord ReadAudioLastIn2(ULWord deviceNumber);
ULWord ReadAudioLastIn3(ULWord deviceNumber);
ULWord ReadAudioLastIn4(ULWord deviceNumber);
ULWord ReadAudioLastIn5(ULWord deviceNumber);
ULWord ReadAudioLastIn6(ULWord deviceNumber);
ULWord ReadAudioLastIn7(ULWord deviceNumber);
ULWord ReadAudioLastIn8(ULWord deviceNumber);

ULWord ReadAudioSampleCounter(ULWord deviceNumber);

void AvInterruptControl(ULWord deviceNumber,
						INTERRUPT_ENUMS	eInterruptType,	// Which interrupt
						ULWord			enable);		// 0: disable, nonzero: enable

void ClearInput1VerticalInterrupt(ULWord deviceNumber);
void ClearInput2VerticalInterrupt(ULWord deviceNumber);
void ClearInput3VerticalInterrupt(ULWord deviceNumber);
void ClearInput4VerticalInterrupt(ULWord deviceNumber);
void ClearInput5VerticalInterrupt(ULWord deviceNumber);
void ClearInput6VerticalInterrupt(ULWord deviceNumber);
void ClearInput7VerticalInterrupt(ULWord deviceNumber);
void ClearInput8VerticalInterrupt(ULWord deviceNumber);

void ClearOutputVerticalInterrupt(ULWord deviceNumber);
void ClearOutput2VerticalInterrupt(ULWord deviceNumber);
void ClearOutput3VerticalInterrupt(ULWord deviceNumber);
void ClearOutput4VerticalInterrupt(ULWord deviceNumber);
void ClearOutput5VerticalInterrupt(ULWord deviceNumber);
void ClearOutput6VerticalInterrupt(ULWord deviceNumber);
void ClearOutput7VerticalInterrupt(ULWord deviceNumber);
void ClearOutput8VerticalInterrupt(ULWord deviceNumber);

void ClearAudioInterrupt(ULWord deviceNumber);

void ClearUartRxInterrupt(ULWord deviceNumber);

void ClearUartTxInterrupt(ULWord deviceNumber);
void ClearUartTxInterrupt2(ULWord deviceNumber);

ULWord ReadDeviceIDRegister(ULWord deviceNumber);

//////////////////////////////////////////////////////////////////
// Aja methods
//
ULWord ReadDMARegister(ULWord deviceNumber, ULWord regNum);
void WriteDMARegister(ULWord deviceNumber, ULWord regNum, ULWord value);
bool ConfigureDMAChannels(ULWord deviceNumber);
void WriteDMAHostAddressLow(ULWord deviceNumber, ULWord index, ULWord value);
void WriteDMAHostAddressHigh(ULWord deviceNumber, ULWord index, ULWord value);
void WriteDMALocalAddress(ULWord deviceNumber, ULWord index, ULWord value);
void WriteDMATransferCount(ULWord deviceNumber, ULWord index, ULWord value);
void WriteDMANextDescriptorLow(ULWord deviceNumber, ULWord index, ULWord value);
void WriteDMANextDescriptorHigh(ULWord deviceNumber, ULWord index, ULWord value);
ULWord ReadDMAControlStatus(ULWord deviceNumber);
void WriteDMAControlStatus(ULWord deviceNumber,ULWord value);
void SetDMAEngineStatus(ULWord deviceNumber, int index, bool enable);
int GetDMAEngineStatus(ULWord deviceNumber, int index);
ULWord ReadDMAInterruptControl(ULWord deviceNumber);
void WriteDMAInterruptControl(ULWord deviceNumber, ULWord value);
void EnableDMAInterrupt(ULWord deviceNumber, NTV2DMAInterruptMask interruptMask);
void DisableDMAInterrupt(ULWord deviceNumber, NTV2DMAInterruptMask interruptMask);
void ClearDMAInterrupt(ULWord deviceNumber, NTV2DMAStatusBits clearBit);
void ClearDMAInterrupts(ULWord deviceNumber);

//////////////////////////////////////////////////////////////////
// Nwl methods
//
ULWord ReadNwlRegister(ULWord deviceNumber, ULWord regNum);
void WriteNwlRegister(ULWord deviceNumber, ULWord regNum, ULWord value);
bool ConfigureNwlChannels(ULWord deviceNumber);
bool IsNwlChannel(ULWord deviceNumber, bool bC2H, int index);
void WriteNwlCommonControlStatus(ULWord deviceNumber, ULWord value);
void WriteNwlControlStatus(ULWord deviceNumber, bool bC2H, ULWord index, ULWord value);
void WriteNwlChainStartAddressLow(ULWord deviceNumber, bool bC2H, ULWord index, ULWord value);
void WriteNwlChainStartAddressHigh(ULWord deviceNumber, bool bC2H, ULWord index, ULWord value);
ULWord ReadNwlCommonControlStatus(ULWord deviceNumber);
ULWord ReadNwlCapabilities(ULWord deviceNumber, bool bC2H, ULWord index);
ULWord ReadNwlControlStatus(ULWord deviceNumber, bool bC2H, ULWord index);
ULWord ReadNwlHardwareTime(ULWord deviceNumber, bool bC2H, ULWord index);
ULWord ReadNwlChainCompleteByteCount(ULWord deviceNumber, bool bC2H, ULWord index);
void ResetNwlHardware(ULWord deviceNumber, bool bC2H, ULWord index);
void EnableNwlUserInterrupt(ULWord deviceNumber);
void DisableNwlUserInterrupt(ULWord deviceNumber);
void EnableNwlDmaInterrupt(ULWord deviceNumber);
void DisableNwlDmaInterrupt(ULWord deviceNumber);
void ClearNwlUserInterrupt(ULWord deviceNumber);
void ClearNwlS2C0Interrupt(ULWord deviceNumber);
void ClearNwlC2S0Interrupt(ULWord deviceNumber);
void ClearNwlS2C1Interrupt(ULWord deviceNumber);
void ClearNwlC2S1Interrupt(ULWord deviceNumber);

//////////////////////////////////////////////////////////////////
// Xlnx methods
//
ULWord ReadXlnxRegister(ULWord deviceNumber, ULWord regNum);
void WriteXlnxRegister(ULWord deviceNumber, ULWord regNum, ULWord value);
bool ConfigureXlnxChannels(ULWord deviceNumber);
bool IsXlnxChannel(ULWord deviceNumber, bool bC2H, int index);
ULWord XlnxChannelRegBase(ULWord deviceNumber, bool bC2H, int index);
ULWord XlnxSgdmaRegBase(ULWord deviceNumber, bool bC2H, int index);
ULWord XlnxConfigRegBase(ULWord deviceNumber);
ULWord XlnxIrqRegBase(ULWord deviceNumber);
ULWord XlnxIrqBitMask(ULWord deviceNumber, bool bC2H, int index);
ULWord XlnxReadChannelIdentifier(ULWord deviceNumber, bool bC2H, ULWord index);
bool IsXlnxChannelStream(ULWord idReg);
bool IsXlnxChannelMapped(ULWord idReg);
ULWord XlnxReadChannelAlignments(ULWord deviceNumber, bool bC2H, ULWord index);
ULWord GetXlnxAddressAlignment(ULWord alReg);
ULWord GetXlnxTransferAlignment(ULWord alReg);
void EnableXlnxUserInterrupt(ULWord deviceNumber, int index);
void DisableXlnxUserInterrupt(ULWord deviceNumber, int index);
ULWord ReadXlnxUserInterrupt(ULWord deviceNumber);
bool IsXlnxUserInterrupt(ULWord deviceNumber, int index, ULWord intReg);
void EnableXlnxDmaInterrupt(ULWord deviceNumber, bool bC2H, int index);
void DisableXlnxDmaInterrupt(ULWord deviceNumber, bool bC2H, int index);
void EnableXlnxDmaInterrupts(ULWord deviceNumber);
void DisableXlnxDmaInterrupts(ULWord deviceNumber);
void DisableXlnxInterrupts(ULWord deviceNumber);
ULWord ReadXlnxDmaInterrupt(ULWord deviceNumber);
bool IsXlnxDmaInterrupt(ULWord deviceNumber, bool bC2H, int index, ULWord intReg);
bool StartXlnxDma(ULWord deviceNumber, bool bC2H, int index);
bool StopXlnxDma(ULWord deviceNumber, bool bC2H, int index);
void StopAllXlnxDma(ULWord deviceNumber);
ULWord ReadXlnxDmaStatus(ULWord deviceNumber, bool bC2H, int index);
ULWord ClearXlnxDmaStatus(ULWord deviceNumber, bool bC2H, int index);
bool IsXlnxDmaActive(ULWord status);
bool IsXlnxDmaError(ULWord status);
bool WaitXlnxDmaActive(ULWord deviceNumber, bool bC2H, int index, bool active);
void WriteXlnxDmaEngineStartLow(ULWord deviceNumber, bool bC2H, int index, ULWord addressLow);
void WriteXlnxDmaEngineStartHigh(ULWord deviceNumber, bool bC2H, int index, ULWord addressHigh);
void WriteXlnxDmaEngineStartAdjacent(ULWord deviceNumber, bool bC2H, int index, ULWord adjacent);
ULWord ReadXlnxPerformanceCycleCount(ULWord deviceNumber, bool bC2H, int index);
ULWord ReadXlnxPerformanceDataCount(ULWord deviceNumber, bool bC2H, int index);
ULWord ReadXlnxMaxReadRequestSize(ULWord deviceNumber);

//////////////////////////////////////////////////////////////////
// Interrupt methods
//
void EnableAllInterrupts(ULWord deviceNumber);
void DisableAllInterrupts(ULWord deviceNumber);

void StopAllDMAEngines(ULWord deviceNumber);

////////////////////////////////////////////////////////////////////////////////////////////
// LTC methods (reuses RP188 structure)

void SetLTCData (ULWord deviceNumber, NTV2Channel channel, RP188_STRUCT rp188Data);
void GetLTCData (ULWord deviceNumber, NTV2Channel channel, RP188_STRUCT* rp188Data);

////////////////////////////////////////////////////////////////////////////////////////////
// OEM RP188 methods

void SetRP188Data (ULWord deviceNumber, NTV2Channel channel, RP188_STRUCT rp188Data);
void GetRP188Data (ULWord deviceNumber, NTV2Channel channel, RP188_STRUCT* rp188Data);

////////////////////////////////////////////////////////////////////////////////////////////
// OEM Custom Ancillary Data methods

void SetCustomAncillaryDataMode(ULWord deviceNumber, NTV2Channel channel, bool bEnable);

void SetCustomAncillaryData (ULWord deviceNumber, NTV2Channel channel, CUSTOM_ANC_STRUCT  *customAncInfo);

////////////////////////////////////////////////////////////////////////////////////////////
// OEM UART methods
void  Init422Uart(ULWord deviceNumber);
#ifdef SOFTWARE_UART_FIFO
#ifdef UARTRXFIFOSIZE
ULWord ReadUARTReceiveData(ULWord deviceNumber);
ULWord ReadUARTReceiveData2(ULWord deviceNumber);
#endif
#ifdef UARTTXFIFOSIZE
void WriteUARTTransmitData(ULWord deviceNumber,ULWord value);
void WriteUARTTransmitData2(ULWord deviceNumber,ULWord value);
#endif
ULWord ReadUARTControl(ULWord deviceNumber);
ULWord ReadUARTControl2(ULWord deviceNumber);
#endif // SOFTWARE_UART_FIFO

//////////////////////////////////////////////////////////////////
// Utility methods
//
bool IsSaveRecallRegister(ULWord deviceNumber, ULWord regNum);
void GetDeviceSerialNumberWords(ULWord deviceNumber, ULWord *low, ULWord *high);
void itoa64(ULWord64 i, char *buffer);
inline void interruptHousekeeping(NTV2PrivateParams* pNTV2Params, INTERRUPT_ENUMS interrupt);
void InitDNXAddressLUT(unsigned long address);

//////////////////////////////////////////////////////////////////
// P2P methods
//
ULWord ReadMessageChannel1(ULWord deviceNumber);
ULWord ReadMessageChannel2(ULWord deviceNumber);
ULWord ReadMessageChannel3(ULWord deviceNumber);
ULWord ReadMessageChannel4(ULWord deviceNumber);
ULWord ReadMessageChannel5(ULWord deviceNumber);
ULWord ReadMessageChannel6(ULWord deviceNumber);
ULWord ReadMessageChannel7(ULWord deviceNumber);
ULWord ReadMessageChannel8(ULWord deviceNumber);

ULWord ReadMessageInterruptStatus(ULWord deviceNumber);
ULWord ReadMessageInterruptControl(ULWord deviceNumber);

void EnableMessageChannel1Interrupt(ULWord deviceNumber);
void DisableMessageChannel1Interrupt(ULWord deviceNumber);
void ClearMessageChannel1Interrupt(ULWord deviceNumber);

void EnableMessageChannel2Interrupt(ULWord deviceNumber);
void DisableMessageChannel2Interrupt(ULWord deviceNumber);
void ClearMessageChannel2Interrupt(ULWord deviceNumber);

void EnableMessageChannel3Interrupt(ULWord deviceNumber);
void DisableMessageChannel3Interrupt(ULWord deviceNumber);
void ClearMessageChannel3Interrupt(ULWord deviceNumber);

void EnableMessageChannel4Interrupt(ULWord deviceNumber);
void DisableMessageChannel4Interrupt(ULWord deviceNumber);
void ClearMessageChannel4Interrupt(ULWord deviceNumber);

void EnableMessageChannel5Interrupt(ULWord deviceNumber);
void DisableMessageChannel5Interrupt(ULWord deviceNumber);
void ClearMessageChannel5Interrupt(ULWord deviceNumber);

void EnableMessageChannel6Interrupt(ULWord deviceNumber);
void DisableMessageChannel6Interrupt(ULWord deviceNumber);
void ClearMessageChannel6Interrupt(ULWord deviceNumber);

void EnableMessageChannel7Interrupt(ULWord deviceNumber);
void DisableMessageChannel7Interrupt(ULWord deviceNumber);
void ClearMessageChannel7Interrupt(ULWord deviceNumber);

void EnableMessageChannel8Interrupt(ULWord deviceNumber);
void DisableMessageChannel8Interrupt(ULWord deviceNumber);
void ClearMessageChannel8Interrupt(ULWord deviceNumber);

ULWord ReadFrameApertureOffset(ULWord deviceNumber);
void WriteFrameApertureOffset(ULWord deviceNumber, ULWord value);
void WriteFrameAperture(ULWord deviceNumber, ULWord offset, ULWord value);

bool DeviceCanDoP2P(ULWord deviceNumber);

ULWord ntv2_getRoundedUpTimeoutJiffies(ULWord timeOutMs);

#endif // !REGISTERIO_H
