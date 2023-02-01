/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
////////////////////////////////////////////////////////////
//
// NTV2 Linux v2.6 Device Driver for AJA OEM boards.
//
// Filename: registerio.c
// Purpose:	 Read/write real and virtual registers
// Notes:
//
///////////////////////////////////////////////////////////////

/*needed by kernel 2.6.18*/
#ifndef CONFIG_HZ
#include <linux/autoconf.h>
#endif

#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/pci.h>
#include <asm/io.h>
#include <asm/div64.h>
#include <linux/interrupt.h>
#include <linux/sched.h>

#include "ajatypes.h"
#include "ntv2enums.h"
#include "ntv2videodefines.h"
#include "ntv2publicinterface.h"

#include "ntv2driver.h"
#include "ntv2linuxpublicinterface.h"
#include "ntv2driverprocamp.h"
#include "registerio.h"
#include "ntv2devicefeatures.h"
#include "../ntv2kona.h"
#include "../ntv2pciconfig.h"

#include "driverdbg.h"

/***************************/
/* Local defines and types */
/***************************/

//#define DEBUGAUDIO
//#define DEBUG_UART

/*********************************************/
/* Prototypes for private utility functions. */
/*********************************************/

/********************/
/* Static variables */
/********************/
static const ULWord	gChannelToGlobalControlRegNum []	= {	kRegGlobalControl, kRegGlobalControlCh2, kRegGlobalControlCh3, kRegGlobalControlCh4,
															kRegGlobalControlCh5, kRegGlobalControlCh6, kRegGlobalControlCh7, kRegGlobalControlCh8, 0};
static const ULWord	gChannelToSmpte372RegisterNum []		= {	kRegGlobalControl,			kRegGlobalControl,			kRegGlobalControl2,			kRegGlobalControl2,
																kRegGlobalControl2,			kRegGlobalControl2,			kRegGlobalControl2,			kRegGlobalControl2,			0};
static const ULWord	gChannelToSmpte372Masks []				= {	kRegMaskSmpte372Enable,		kRegMaskSmpte372Enable,		kRegMaskSmpte372Enable4,	kRegMaskSmpte372Enable4,
																kRegMaskSmpte372Enable6,	kRegMaskSmpte372Enable6,	kRegMaskSmpte372Enable8,	kRegMaskSmpte372Enable8,	0};
static const ULWord	gChannelToSmpte372Shifts []				= {	kRegShiftSmpte372,			kRegShiftSmpte372,		kRegShiftSmpte372Enable4,	kRegShiftSmpte372Enable4,
																kRegShiftSmpte372Enable6,	kRegShiftSmpte372Enable6,	kRegShiftSmpte372Enable8,	kRegShiftSmpte372Enable8,	0};
static const ULWord	gChannelToSDIOutControlRegNum []	= {	kRegSDIOut1Control, kRegSDIOut2Control, kRegSDIOut3Control, kRegSDIOut4Control,
															kRegSDIOut5Control, kRegSDIOut6Control, kRegSDIOut7Control, kRegSDIOut8Control, 0};
static const ULWord	gChannelToSDIIn6GModeMask []	= {	kRegMaskSDIIn16GbpsMode,		kRegMaskSDIIn26GbpsMode,	kRegMaskSDIIn36GbpsMode,	kRegMaskSDIIn46GbpsMode,
														kRegMaskSDIIn56GbpsMode,	kRegMaskSDIIn66GbpsMode,	kRegMaskSDIIn76GbpsMode,	kRegMaskSDIIn86GbpsMode,	0};

static const ULWord	gChannelToSDIIn6GModeShift []	= {	kRegShiftSDIIn16GbpsMode,	kRegShiftSDIIn26GbpsMode,	kRegShiftSDIIn36GbpsMode,	kRegShiftSDIIn46GbpsMode,
														kRegShiftSDIIn56GbpsMode,	kRegShiftSDIIn66GbpsMode,	kRegShiftSDIIn76GbpsMode,	kRegShiftSDIIn86GbpsMode,	0};

static const ULWord	gChannelToSDIIn12GModeMask []	= {	kRegMaskSDIIn112GbpsMode,		kRegMaskSDIIn212GbpsMode,	kRegMaskSDIIn312GbpsMode,	kRegMaskSDIIn412GbpsMode,
														kRegMaskSDIIn512GbpsMode,	kRegMaskSDIIn612GbpsMode,	kRegMaskSDIIn712GbpsMode,	kRegMaskSDIIn812GbpsMode,	0};

static const ULWord	gChannelToSDIIn12GModeShift []	= {	kRegShiftSDIIn112GbpsMode,	kRegShiftSDIIn212GbpsMode,	kRegShiftSDIIn312GbpsMode,	kRegShiftSDIIn412GbpsMode,
														kRegShiftSDIIn512GbpsMode,	kRegShiftSDIIn612GbpsMode,	kRegShiftSDIIn712GbpsMode,	kRegShiftSDIIn812GbpsMode,	0};
static const ULWord	gChannelToSDIInput3GStatusRegNum []	= {	kRegSDIInput3GStatus,		kRegSDIInput3GStatus,		kRegSDIInput3GStatus2,		kRegSDIInput3GStatus2,
															kRegSDI5678Input3GStatus,	kRegSDI5678Input3GStatus,	kRegSDI5678Input3GStatus,	kRegSDI5678Input3GStatus,	0};

/*************/
/* Functions */
/*************/

static unsigned long
GetRegisterAddress(	ULWord deviceNumber,
					ULWord registerNumber)
{
	NTV2PrivateParams *pNTV2Params;
	NTV2ModulePrivateParams *pNTV2ModuleParams;
	unsigned long address = 0;

	pNTV2Params = getNTV2Params(deviceNumber);
	pNTV2ModuleParams = getNTV2ModuleParams();

	address = pNTV2Params->_pGlobalControl+(registerNumber*4);

	return address;
}


ULWord READ_REGISTER_ULWord( unsigned long address)
{
	ULWord value = 0xffffffff;
	NTV2PrivateParams* pNTV2Params;
	pNTV2Params = getNTV2Params(0);

	if(address == (pNTV2Params->_VideoAddress+0x118) ) {
#ifdef DEBUG_UART
		MSG("READ_REGISTER_ULWord(TX)\n");
#endif
		return 0;
	}

	//debug spin read
	if(address == pNTV2Params->_pGlobalControl+(228*4))
	{
		ULWord count = 0;
		while(count < 1000)
		{
			count++;
			value = readl((void *)address);
			if(value == 0xffffffff)
			{
				printk("AJA Driver Error: checked register read error count %d\n", count);
			}
		}
		return value;
	}

	value = readl((void *)address);
	if(value == 0xffffffff && (address == pNTV2Params->_pDMAControlStatus || address == pNTV2Params->_pMessageInterruptControl))
	{
		ULWord count = 1;
		printk("AJA Driver Error: checked %s register read 0xffffffff\n", address == pNTV2Params->_pDMAControlStatus ? "DMA" : "Interrupt");
		while(value == 0xffffffff)
		{
			if(count%100 == 0)
				printk("AJA Driver Error: checked register read 0xffffffff\n");
			value = readl((void *)address);
			count++;
		}
		printk("AJA Driver Error: checked register read error count %d\n", count);
	}
	// printk("RRul_: r = %lx\n", readl((void *)address));

	return value;
}

ULWord READ_REGISTER_UWord( unsigned long address)
{
	// The 16 bit registers read by this function are not on the PCI bus so we assume
	// they are memory mapped and do not use readw().
	ULWord value = (ULWord)*((UWord *)address);

	// printk("RRuw_: r = %lx\n", value);

	return value;
}


void WRITE_REGISTER_ULWord( unsigned long address, ULWord regValue)
{
	NTV2PrivateParams* pNTV2Params;
	pNTV2Params = getNTV2Params(0);

	//	printk("WR_: r(%lx) v(%x)\n", (address-pNTV2Params->_BAR0Address)/4, regValue);
	writel(regValue, (void *)address);
}

void WRITE_REGISTER_UWord( unsigned long address, ULWord regValue)
{
	UWord shortRegValue = regValue & 0xFFFF;
	// The 16 bit registers read by this function are not on the PCI bus so we assume
	// they are memory mapped and do not use writew().
	*(UWord *)address = shortRegValue;
}

void WRITE_REGISTER_UByte( unsigned long address, ULWord regValue)
{
	UByte byteRegValue = regValue & 0xFF;
	// The 8 bit registers read by this function are not on the PCI bus so we assume
	// they are memory mapped and do not use writew().
	*(UByte *)address = byteRegValue;
}

void GetActiveFrameBufferSize(ULWord deviceNumber, NTV2FrameDimensions * frameBufferSize)
{
	Ntv2SystemContext systemContext;
	NTV2Standard  standard;
	systemContext.devNum = deviceNumber;
	standard = GetStandard(&systemContext, NTV2_CHANNEL1);

	switch ( standard )
	{
	case NTV2_STANDARD_1080:
	case NTV2_STANDARD_1080p:
		frameBufferSize->mWidth = HD_NUMCOMPONENTPIXELS_1080;
		frameBufferSize->mHeight = HD_NUMACTIVELINES_1080;
		break;
	case NTV2_STANDARD_720:
		frameBufferSize->mWidth = HD_NUMCOMPONENTPIXELS_720;
		frameBufferSize->mHeight = HD_NUMACTIVELINES_720;
		break;
	case NTV2_STANDARD_525:
		frameBufferSize->mWidth = NUMCOMPONENTPIXELS;
		frameBufferSize->mHeight = NUMACTIVELINES_525;
		break;
	case NTV2_STANDARD_625:
		frameBufferSize->mWidth = NUMCOMPONENTPIXELS;
		frameBufferSize->mHeight = NUMACTIVELINES_625;
		break;
	case NTV2_STANDARD_2K:
		frameBufferSize->mWidth = HD_NUMCOMPONENTPIXELS_2K;
		frameBufferSize->mHeight = HD_NUMACTIVELINES_2K;
		break;

	default:
		MSG("GetActiveFrameBufferSize(): Unknown video standard %d\n", standard );
		frameBufferSize->mWidth = -1;
		frameBufferSize->mHeight = -1;
		break;
	}
}

static bool
IsRegisterNumValid(NTV2PrivateParams* pNTV2Params, ULWord registerNumber, unsigned long address)
{
	bool result = true;

	if ( address >= (pNTV2Params->_pGlobalControl + pNTV2Params->_VideoMemorySize) )
	{
		result = false;
	}

	return result;
}


// Write a register by number.
// TODO: We need to either add an UpdateRegister() or make all read-update-modify operations
//       explicit somehow

void WriteRegister(	ULWord deviceNumber,
					ULWord registerNumber,
					ULWord registerValue,
					ULWord registerMask,
					ULWord registerShift)
{
	unsigned long flags = 0;
	NTV2PrivateParams *pNTV2Params;
	NTV2ModulePrivateParams *pNTV2ModuleParams;
	unsigned long address = 0;
	ULWord oldValue = 0;

	//	printk("WR(%d): r(%x) v(%x)\n", deviceNumber, registerNumber, registerValue );

	pNTV2Params = getNTV2Params(deviceNumber);
	pNTV2ModuleParams = getNTV2ModuleParams();

	if (!pNTV2Params->registerEnable &&
		!((registerNumber >= VIRTUALREG_START) && (registerNumber <= kVRegLast)))
	{
		return;
	}
	
	address = GetRegisterAddress(deviceNumber, registerNumber);

	// handle "normal", real hardware registers
	if ((registerNumber < VIRTUALREG_START) ||
		((registerNumber > kVRegLast) &&
		 ((registerNumber * 4) < pNTV2Params->_VideoMemorySize)))
	{
		if ( !IsRegisterNumValid(pNTV2Params, registerNumber, address))
		{
//			printk("%s: attempt to write unsupported hardware register %d address 0x%08lx\n", __FUNCTION__, registerNumber, address);
			return;
		}
		if (registerNumber <= pNTV2Params->_numberOfHWRegisters)
		{
# ifdef SOFTWARE_UART_FIFO		/* this is here to prevent a deadlock in the software UART.  Seems that the register access in the ISR and driver initialization deadlock when probe()ing */
			if (registerNumber != kRegRS422Control
				&& registerNumber != kRegRS422Transmit
				&& registerNumber != kRegRS422Transmit)
# endif
			ntv2_spin_lock_irqsave(&(pNTV2Params->_registerSpinLock), flags);
		}

		// Doesn't make sense to have a shift with no mask, so just check mask
		if (registerMask != NO_MASK)
		{
			oldValue = READ_REGISTER_ULWord(address);
			oldValue &= ~registerMask;
			registerValue <<= registerShift;
			registerValue |= oldValue;
		}

#ifdef SOFTWARE_UART_FIFO
		switch(registerNumber) {

		case kRegRS422Control:
		{
			unsigned long flags;

# ifdef UARTRXFIFOSIZE
			ntv2_spin_lock_irqsave(&pNTV2Params->uartRxFifoLock, flags);

			if(!(registerValue & kRegMaskRS422RXEnable)) {
				pNTV2Params->uartRxFifoSize = 0;
			}

			if(registerValue & BIT_7) {
				pNTV2Params->uartRxFifoOverrun = 0;
			}

			ntv2_spin_unlock_irqrestore(&pNTV2Params->uartRxFifoLock, flags);
# endif	// UARTRXFIFOSIZE

# ifdef UARTTXFIFOSIZE
			ntv2_spin_lock_irqsave(&pNTV2Params->uartTxFifoLock, flags);

			if(!(registerValue & kRegMaskRS422TXEnable)) {
				pNTV2Params->uartTxFifoSize = 0;
			}

			ntv2_spin_unlock_irqrestore(&pNTV2Params->uartTxFifoLock, flags);
# endif	// UARTTXFIFOSIZE

			WRITE_REGISTER_ULWord(address,registerValue);
		}
		break;

		case kRegRS4222Control:
		{
			unsigned long flags;

# ifdef UARTRXFIFOSIZE
			ntv2_spin_lock_irqsave(&pNTV2Params->uartRxFifoLock2, flags);

			if(!(registerValue & kRegMaskRS422RXEnable)) {
				pNTV2Params->uartRxFifoSize2 = 0;
			}

			if(registerValue & BIT_7) {
				pNTV2Params->uartRxFifoOverrun2 = 0;
			}

			ntv2_spin_unlock_irqrestore(&pNTV2Params->uartRxFifoLock2, flags);
# endif	// UARTRXFIFOSIZE

# ifdef UARTTXFIFOSIZE
			ntv2_spin_lock_irqsave(&pNTV2Params->uartTxFifoLock2, flags);

			if(!(registerValue & kRegMaskRS422TXEnable)) {
				pNTV2Params->uartTxFifoSize2 = 0;
			}

			ntv2_spin_unlock_irqrestore(&pNTV2Params->uartTxFifoLock2, flags);
# endif	// UARTTXFIFOSIZE

			WRITE_REGISTER_ULWord(address,registerValue);
		}
		break;

# ifdef UARTTXFIFOSIZE
		case kRegRS422Transmit:
		{
			unsigned long flags;

			ntv2_spin_lock_irqsave(&pNTV2Params->uartTxFifoLock, flags);

			if(pNTV2Params->uartTxFifoSize < UARTTXFIFOSIZE) {
				pNTV2Params->uartTxFifo[pNTV2Params->uartTxFifoSize++] =
					registerValue;
			}

			if(registerMask == NO_MASK) {
				unsigned i, j;

				for(i = 0; i < pNTV2Params->uartTxFifoSize; ++i) {
					ULWord control = ReadUARTControl(deviceNumber);
					if(control & BIT_2) break;
					registerValue = pNTV2Params->uartTxFifo[i];
					WriteUARTTransmitData(deviceNumber, registerValue);
				}

				for(j = 0; j < i; ++j) {
					pNTV2Params->uartTxFifo[j] = pNTV2Params->uartTxFifo[i+j];
				}

				pNTV2Params->uartTxFifoSize -= i;
			}

			ntv2_spin_unlock_irqrestore(&pNTV2Params->uartTxFifoLock, flags);
		}
		break;

		case kRegRS4222Transmit:
		{
			unsigned long flags;

			ntv2_spin_lock_irqsave(&pNTV2Params->uartTxFifoLock2, flags);

			if(pNTV2Params->uartTxFifoSize2 < UARTTXFIFOSIZE) {
				pNTV2Params->uartTxFifo2[pNTV2Params->uartTxFifoSize2++] =
					registerValue;
			}

			if(registerMask == NO_MASK) {
				unsigned i, j;

				for(i = 0; i < pNTV2Params->uartTxFifoSize2; ++i) {
					ULWord control = ReadUARTControl2(deviceNumber);
					if(control & BIT_2) break;
					registerValue = pNTV2Params->uartTxFifo2[i];
					WriteUARTTransmitData(deviceNumber, registerValue);
				}

				for(j = 0; j < i; ++j) {
					pNTV2Params->uartTxFifo2[j] = pNTV2Params->uartTxFifo2[i+j];
				}

				pNTV2Params->uartTxFifoSize2 -= i;
			}

			ntv2_spin_unlock_irqrestore(&pNTV2Params->uartTxFifoLock2, flags);
		}
		break;
# endif	// UARTTXFIFOSIZE

		default:
			WRITE_REGISTER_ULWord(address,registerValue);

			if (registerNumber <= pNTV2Params->_numberOfHWRegisters)
#  ifdef SOFTWARE_UART_FIFO
				if (registerNumber != kRegRS422Control
					&& registerNumber != kRegRS422Transmit
					&& registerNumber != kRegRS422Transmit)
#  endif
				ntv2_spin_unlock_irqrestore(&(pNTV2Params->_registerSpinLock), flags);
			break;
		} // switch

#else // !defined(SOFTWARE_UART_FIFO)

		WRITE_REGISTER_ULWord(address,registerValue);

		if (registerNumber <= pNTV2Params->_numberOfHWRegisters)
			ntv2_spin_unlock_irqrestore(&(pNTV2Params->_registerSpinLock), flags);
#endif	// SOFTWARE_UART_FIFO

		return;
	}

	// Handle virtual registers
	if ( (registerNumber >= VIRTUALREG_START) && (registerNumber <= kVRegLast) )
	{
		ntv2_spin_lock_irqsave(&(pNTV2Params->_virtualRegisterLock), flags);

		switch(registerNumber)
		{
		case kVRegGlobalAudioPlaybackMode:

			// Doesn't make sense to have a shift with no mask, so just check mask
			if (registerMask != NO_MASK)
			{
				oldValue = 	GetAudioPlaybackMode(deviceNumber);
				oldValue &= ~registerMask;
				registerValue <<= registerShift;
				registerValue |= oldValue;
			}
			SetAudioPlaybackMode(deviceNumber, registerValue);
			break;

		case kVRegProcAmpStandardDefBrightness:
		case kVRegProcAmpStandardDefContrast:
		case kVRegProcAmpStandardDefSaturation:
		case kVRegProcAmpStandardDefHue:
		case kVRegProcAmpStandardDefCbOffset:
		case kVRegProcAmpStandardDefCrOffset:
		case kVRegProcAmpHighDefBrightness:
		case kVRegProcAmpHighDefContrast:
		case kVRegProcAmpHighDefSaturationCb:
		case kVRegProcAmpHighDefSaturationCr:
		case kVRegProcAmpHighDefHue:
		case kVRegProcAmpHighDefCbOffset:
		case kVRegProcAmpHighDefCrOffset:
		case kVRegProcAmpSDRegsInitialized:
		case kVRegProcAmpHDRegsInitialized:
			if (!SetVirtualProcampRegister(registerNumber,
										   registerValue,
										   &pNTV2Params->_virtualProcAmpRegisters))
			{
				MSG("%s: Attempt to write 0x%08x to virtual register %d failed\n",
					pNTV2Params->name, registerValue, registerNumber );
			}

			// Exclude virtual procamp regs for which there is no hardware register
			if (registerNumber != kVRegProcAmpSDRegsInitialized && registerNumber != kVRegProcAmpHDRegsInitialized)
			{
				if (!WriteHardwareProcampRegister(deviceNumber,
												  pNTV2Params->_DeviceID,
												  registerNumber,
												  registerValue,
												  &pNTV2Params->_hwProcAmpRegisterImage)
					)
				{
					MSG("%s: Attempt to update hardware register during write of value 0x%08x to virtual register %d failed\n",
						pNTV2Params->name, registerValue, registerNumber );
				}
			}
			break;

		case kVRegAudioSyncTolerance:
			pNTV2Params->_audioSyncTolerance = registerValue;
			break;

		case kVRegDmaSerialize:
			pNTV2Params->_dmaSerialize = registerValue;
			break;

		case kVRegSyncChannels:
			pNTV2Params->_syncChannels = registerValue;
			break;

		// Control panel
		case kVRegApplicationPID:
			if( pNTV2Params->_ApplicationPID == 0 )
				pNTV2Params->_ApplicationPID = registerValue;
			break;

		case kVRegApplicationCode:
			pNTV2Params->_ApplicationCode = registerValue;
			break;

		case kVRegReleaseApplication:
			if( pNTV2Params->_ApplicationPID == registerValue )
			{
				pNTV2Params->_ApplicationPID = 0;
				pNTV2Params->_ApplicationCode = 0;
			}
			break;

		case kVRegForceApplicationPID:
			pNTV2Params->_ApplicationPID = registerValue;
			break;

		case kVRegForceApplicationCode:
			pNTV2Params->_ApplicationCode = registerValue;
			break;

		case kVRegAcquireLinuxReferenceCount:
			if( registerValue == 0 )
				pNTV2Params->_ApplicationReferenceCount = 0;
			else
				pNTV2Params->_ApplicationReferenceCount++;
			break;

		case kVRegReleaseLinuxReferenceCount:
			if( pNTV2Params->_ApplicationReferenceCount > 0 )
				pNTV2Params->_ApplicationReferenceCount--;
			break;

		case kVRegMailBoxTimeoutNS:
			pNTV2Params->_VirtualMailBoxTimeoutNS = registerValue;
			break;

		case kVRegPCIMaxReadRequestSize:
			ntv2WritePciMaxReadRequestSize(&pNTV2Params->systemContext, registerValue);
			break;

		default:
			// store virtual reg
			pNTV2Params->_virtualRegisterMem[registerNumber - VIRTUALREG_START] = registerValue;
			break;

		} // switch

		ntv2_spin_unlock_irqrestore(&(pNTV2Params->_virtualRegisterLock), flags);

		return;
	}

	MSG("%s: BUG BUG %s: Attempt to write register %u with value 0x%x was not handled\n",
		pNTV2Params->name, __FUNCTION__, registerNumber, registerValue);
}

// Write a group of registers as a block
void WriteRegisterBufferULWord(	ULWord deviceNumber,
								ULWord registerNumber,
								ULWord* sourceData,
								ULWord sourceDataSizeULWords)
{
	NTV2PrivateParams *pNTV2Params;
	NTV2ModulePrivateParams *pNTV2ModuleParams;
	void* address;

	pNTV2ModuleParams = getNTV2ModuleParams();
	pNTV2Params = getNTV2Params(deviceNumber);

	if (!pNTV2Params->registerEnable)
		return;

	if (registerNumber < VIRTUALREG_START)
	{
		address = (void *)GetRegisterAddress(deviceNumber, registerNumber);

		memcpy_toio(	address,
			(const void *)sourceData,
			sourceDataSizeULWords);
	}
	else if ( (registerNumber >= VIRTUALREG_START) && (registerNumber < (VIRTUALREG_START+MAX_NUM_VIRTUAL_REGISTERS)) ) // It is a virtual register
	{
	   switch(registerNumber)
	   {
			case kVRegGlobalAudioPlaybackMode:
				SetAudioPlaybackMode(deviceNumber, *sourceData);
			break;

			default:
				MSG("%s: WriteRegisterBufferULWord(): Attempt to write to readonly or unsupported virtual register 0x%08x\n",
					pNTV2Params->name, registerNumber );
			return;
	   }
	}
}

// Read a register by number.  Reading of virual registers is done here too.
//
ULWord ReadRegister(ULWord deviceNumber, ULWord registerNumber, ULWord registerMask, ULWord registerShift)
{
	ULWord value;
	NTV2PrivateParams* pNTV2Params;
	NTV2ModulePrivateParams *pNTV2ModuleParams;
	unsigned long address;
	unsigned long flags = 0;

	pNTV2ModuleParams = getNTV2ModuleParams();
	pNTV2Params = getNTV2Params(deviceNumber);

	if (!pNTV2Params->registerEnable &&
		!((registerNumber >= VIRTUALREG_START) && (registerNumber <= kVRegLast)))
	{
		if (registerNumber == kRegBoardID)
			return pNTV2Params->_DeviceID;
		return 0;
	}

	address = GetRegisterAddress( deviceNumber, registerNumber);


	// If the register number corresponds to a real on-board register,
	// read it.
	if ((registerNumber < VIRTUALREG_START) ||
		((registerNumber > kVRegLast) &&
		 ((registerNumber * 4) < pNTV2Params->_VideoMemorySize)))
	{
		if ( !IsRegisterNumValid(pNTV2Params, registerNumber, address))
		{
//			MSG("%s: attempt to read unsupported hardware register %u address 0x%08lx\n",
//				pNTV2Params->name, registerNumber, address);
//			return 0xFFFFFFFF;
			return 0;
		}

#ifdef SOFTWARE_UART_FIFO
		switch(registerNumber) {
		case kRegRS422Control:
		{
			unsigned long flags;

			value = ReadUARTControl(deviceNumber);

# ifdef UARTRXFIFOSIZE
			// Clear bits for software Rx FIFO. Leave overrun bit (bit 7) alone.
			// If hardward FIFO has overrun, then the software FIFO needs to
			// be considered as overrun as well.

			value &= ~(BIT_4 | BIT_5);

			ntv2_spin_lock_irqsave(&pNTV2Params->uartRxFifoLock, flags);

			if(pNTV2Params->uartRxFifoSize) value |= BIT_4;
			if(pNTV2Params->uartRxFifoSize == UARTRXFIFOSIZE) value |= BIT_5;
			if(pNTV2Params->uartRxFifoOverrun) value |= BIT_7;

			ntv2_spin_unlock_irqrestore(&pNTV2Params->uartRxFifoLock, flags);
# endif // UARTRXFIFOSIZE

# ifdef UARTTXFIFOSIZE
			value &= ~(BIT_1 | BIT_2);

			ntv2_spin_lock_irqsave(&pNTV2Params->uartTxFifoLock, flags);

			if(pNTV2Params->uartTxFifoSize == 0) value |= BIT_1;
			if(pNTV2Params->uartTxFifoSize == UARTTXFIFOSIZE) value |= BIT_2;

			ntv2_spin_unlock_irqrestore(&pNTV2Params->uartTxFifoLock, flags);
# endif // UARTTXFIFOSIZE
		}
		break;

		case kRegRS4222Control:
		{
			unsigned long flags;

			value = ReadUARTControl2(deviceNumber);

# ifdef UARTRXFIFOSIZE
			// Clear bits for software Rx FIFO. Leave overrun bit (bit 7) alone.
			// If hardward FIFO has overrun, then the software FIFO needs to
			// be considered as overrun as well.

			value &= ~(BIT_4 | BIT_5);

			ntv2_spin_lock_irqsave(&pNTV2Params->uartRxFifoLock2, flags);

			if(pNTV2Params->uartRxFifoSize2) value |= BIT_4;
			if(pNTV2Params->uartRxFifoSize2 == UARTRXFIFOSIZE) value |= BIT_5;
			if(pNTV2Params->uartRxFifoOverrun2) value |= BIT_7;

			ntv2_spin_unlock_irqrestore(&pNTV2Params->uartRxFifoLock2, flags);
# endif // UARTRXFIFOSIZE

# ifdef UARTTXFIFOSIZE
			value &= ~(BIT_1 | BIT_2);

			ntv2_spin_lock_irqsave(&pNTV2Params->uartTxFifoLock2, flags);

			if(pNTV2Params->uartTxFifoSize2 == 0) value |= BIT_1;
			if(pNTV2Params->uartTxFifoSize2 == UARTTXFIFOSIZE) value |= BIT_2;

			ntv2_spin_unlock_irqrestore(&pNTV2Params->uartTxFifoLock2, flags);
# endif // UARTTXFIFOSIZE
		}
		break;

# ifdef UARTRXFIFOSIZE
		case kRegRS422Receive:
		{
			unsigned long flags;

			ntv2_spin_lock_irqsave(&pNTV2Params->uartRxFifoLock, flags);

			if(pNTV2Params->uartRxFifoSize) {
				unsigned i, n = --pNTV2Params->uartRxFifoSize;
				value = pNTV2Params->uartRxFifo[0];
				for(i = 0; i < n; ++i) {
					pNTV2Params->uartRxFifo[i] = pNTV2Params->uartRxFifo[i+1];
				}
			} else {
				value = 0;
			}

			ntv2_spin_unlock_irqrestore(&pNTV2Params->uartRxFifoLock, flags);
		}
		break;
# endif // UARTRXFIFOSIZE

# ifdef UARTRXFIFOSIZE
		case kRegRS4222Receive:
		{
			unsigned long flags;

			ntv2_spin_lock_irqsave(&pNTV2Params->uartRxFifoLock2, flags);

			if(pNTV2Params->uartRxFifoSize2) {
				unsigned i, n = --pNTV2Params->uartRxFifoSize2;
				value = pNTV2Params->uartRxFifo2[0];
				for(i = 0; i < n; ++i) {
					pNTV2Params->uartRxFifo2[i] = pNTV2Params->uartRxFifo2[i+1];
				}
			} else {
				value = 0;
			}

			ntv2_spin_unlock_irqrestore(&pNTV2Params->uartRxFifoLock2, flags);
		}
		break;
# endif // UARTRXFIFOSIZE
		default:
#endif // SOFTWARE_UART_FIFO

		if (registerNumber <= pNTV2Params->_numberOfHWRegisters)
			ntv2_spin_lock_irqsave(&(pNTV2Params->_registerSpinLock), flags);

		value = READ_REGISTER_ULWord(address);

		if (registerNumber <= pNTV2Params->_numberOfHWRegisters)
			ntv2_spin_unlock_irqrestore(&(pNTV2Params->_registerSpinLock), flags);

#ifdef SOFTWARE_UART_FIFO
			break;
		}
#endif // SOFTWARE_UART_FIFO

		// Mask and shift is only applied to real registers.
		value &= registerMask;
		value >>= registerShift;
		return value;
	}

	// It is a virtual register, return the right value here.
	if ( (registerNumber >= VIRTUALREG_START) && (registerNumber <= kVRegLast) )
	{
		switch(registerNumber)
		{
		case kVRegBA0MemorySize:
			return pNTV2Params->_BAR0MemorySize;

		case kVRegBA1MemorySize:
			return pNTV2Params->_BAR1MemorySize;

		case kVRegBA2MemorySize:
			return pNTV2Params->_BAR2MemorySize;

		case kVRegBA4MemorySize:
			return 0;

			//			case kRegBA4MemoryBase:
			//				return pNTV2Params->_BA4MemorySize;

			//			case kRegBA4MappedAddress:
			//				return pNTV2Params->_BAR4Address;

		case kVRegDMADriverBufferPhysicalAddress:
			// If value is 0, failure or bigphysarea patch not applied
			return pNTV2Params->_dmaBuffer ?  : -1;

		case kVRegNumDmaDriverBuffers:
			return 0;

		case kVRegGlobalAudioPlaybackMode:
			// If value is 0, failure or bigphysarea patch not applied
			return pNTV2Params->_globalAudioPlaybackMode;

		case kVRegProcAmpSDRegsInitialized:
		case kVRegProcAmpStandardDefBrightness:
		case kVRegProcAmpStandardDefContrast:
		case kVRegProcAmpStandardDefSaturation:
		case kVRegProcAmpStandardDefHue:
		case kVRegProcAmpStandardDefCbOffset:
		case kVRegProcAmpStandardDefCrOffset:
		case kVRegProcAmpHDRegsInitialized:
		case kVRegProcAmpHighDefBrightness:
		case kVRegProcAmpHighDefContrast:
		case kVRegProcAmpHighDefSaturationCb:
		case kVRegProcAmpHighDefSaturationCr:
		case kVRegProcAmpHighDefHue:
		case kVRegProcAmpHighDefCbOffset:
		case kVRegProcAmpHighDefCrOffset:
			if (!GetVirtualProcampRegister(registerNumber,
										   &value,
										   &pNTV2Params->_virtualProcAmpRegisters))
			{
//				MSG("%s: Attempt to read virtual procamp register %u failed\n",
//					pNTV2Params->name, registerNumber );
				return -1;
			}
			else
				return value;

		case kVRegAudioSyncTolerance:
			return pNTV2Params->_audioSyncTolerance;

		case kVRegDmaSerialize:
			return pNTV2Params->_dmaSerialize;

		case kVRegSyncChannels:
			return pNTV2Params->_syncChannels;

		// Control panel
		case kVRegApplicationPID:
			return pNTV2Params->_ApplicationPID;
			break;

		case kVRegApplicationCode:
			return pNTV2Params->_ApplicationCode;
			break;

		case kVRegAcquireLinuxReferenceCount:
			return pNTV2Params->_ApplicationReferenceCount;
			break;

		case kVRegMailBoxAcquire:
			{
				int		status;
				//	Virtual register in units of 100 ns, so convert to ms first
				ULWord	timeout = ntv2_getRoundedUpTimeoutJiffies(pNTV2Params->_VirtualMailBoxTimeoutNS / 10000);

				status = down_timeout(&pNTV2Params->_mailBoxSemaphore, timeout);
				return (status < 0) ? false : true;
			}
			break;

		case kVRegMailBoxRelease:
			up(&pNTV2Params->_mailBoxSemaphore);
			return true;
			break;

		case kVRegMailBoxAbort:
			up(&pNTV2Params->_mailBoxSemaphore);
			return true;
			break;

		case kVRegMailBoxTimeoutNS:
			return pNTV2Params->_VirtualMailBoxTimeoutNS;
			break;

		case kVRegPCIMaxReadRequestSize:
			return ntv2ReadPciMaxReadRequestSize(&pNTV2Params->systemContext);
			break;
			
		default:
			// return virtual reg
			return pNTV2Params->_virtualRegisterMem[registerNumber - VIRTUALREG_START];
			break;
		} // switch
	}

	MSG("%s: BUG BUG: Attempt to read register %u was not handled\n",
		pNTV2Params->name, registerNumber );

	return 0xFFFFFFFF;
}

void WriteVideoProcessingControl(ULWord deviceNumber,ULWord value)
{
	WRITE_REGISTER_ULWord(getNTV2Params(deviceNumber)->_pVideoProcessingControl,value);
}

ULWord ReadVideoProcessingControl(ULWord deviceNumber)
{
	return  READ_REGISTER_ULWord(getNTV2Params(deviceNumber)->_pVideoProcessingControl);
}

void WriteVideoProcessingControlCrosspoint(ULWord deviceNumber,ULWord value)
{
	WRITE_REGISTER_ULWord(getNTV2Params(deviceNumber)->_pVideoProcessingCrossPointControl,value);
}

ULWord ReadVideoProcessingControlCrosspoint(ULWord deviceNumber)
{
	return  READ_REGISTER_ULWord(getNTV2Params(deviceNumber)->_pVideoProcessingCrossPointControl);
}

void SetForegroundVideoCrosspoint(ULWord deviceNumber, NTV2Crosspoint crosspoint)
{
	ULWord regValue;

	regValue = ReadVideoProcessingControlCrosspoint(deviceNumber);
	regValue &= ~(FGVCROSSPOINTMASK);
	regValue |= (crosspoint<<FGVCROSSPOINTSHIFT);
	WriteVideoProcessingControlCrosspoint(deviceNumber, regValue);
}

void SetForegroundKeyCrosspoint(ULWord deviceNumber, NTV2Crosspoint crosspoint)
{
	ULWord regValue;

	regValue = ReadVideoProcessingControlCrosspoint(deviceNumber);
	regValue &= ~(FGKCROSSPOINTMASK);
	regValue |= (crosspoint<<FGKCROSSPOINTSHIFT);
	WriteVideoProcessingControlCrosspoint(deviceNumber, regValue);

}

void SetBackgroundVideoCrosspoint(ULWord deviceNumber, NTV2Crosspoint crosspoint)
{
	ULWord regValue;

	regValue = ReadVideoProcessingControlCrosspoint(deviceNumber);
	regValue &= ~(BGVCROSSPOINTMASK);
	regValue |= (crosspoint<<BGVCROSSPOINTSHIFT);
	WriteVideoProcessingControlCrosspoint(deviceNumber, regValue);

}

void SetBackgroundKeyCrosspoint(ULWord deviceNumber, NTV2Crosspoint crosspoint)
{
	ULWord regValue;

	regValue = ReadVideoProcessingControlCrosspoint(deviceNumber);
	regValue &= ~(BGKCROSSPOINTMASK);
	regValue |= (crosspoint<<BGKCROSSPOINTSHIFT);
	WriteVideoProcessingControlCrosspoint(deviceNumber, regValue);

}

void WriteInterruptRegister(ULWord deviceNumber ,ULWord value)
{
	WRITE_REGISTER_ULWord(getNTV2Params(deviceNumber)->_pInterruptControl,value);
}

ULWord ReadInterruptRegister(ULWord deviceNumber)
{
	return READ_REGISTER_ULWord(getNTV2Params(deviceNumber)->_pInterruptControl);
}

ULWord ReadStatusRegister(ULWord deviceNumber)
{
	return  READ_REGISTER_ULWord(getNTV2Params(deviceNumber)->_pStatus);
}

void WriteInterrupt2Register(ULWord deviceNumber ,ULWord value)
{
	WRITE_REGISTER_ULWord(getNTV2Params(deviceNumber)->_pInterruptControl2,value);
}

ULWord ReadInterrupt2Register(ULWord deviceNumber)
{
	return READ_REGISTER_ULWord(getNTV2Params(deviceNumber)->_pInterruptControl2);
}

ULWord ReadStatus2Register(ULWord deviceNumber)
{
	return  READ_REGISTER_ULWord(getNTV2Params(deviceNumber)->_pStatus2);
}

void SetRegisterWriteMode(ULWord deviceNumber, NTV2Channel channel, NTV2RegisterWriteMode value)
{
	Ntv2SystemContext systemContext;
	ULWord regNum = 0;
	systemContext.devNum = deviceNumber;
	
	if (!IsMultiFormatActive(&systemContext))
			channel = NTV2_CHANNEL1;

	regNum = gChannelToGlobalControlRegNum[channel];
	WriteRegister(	deviceNumber,
					regNum,
					value,
					NTV2REGWRITEMODEMASK,
					NTV2REGWRITEMODESHIFT);
}

NTV2RegisterWriteMode GetRegisterWriteMode(ULWord deviceNumber, NTV2Channel channel)
{
	Ntv2SystemContext systemContext;
	ULWord regNum = 0;
	systemContext.devNum = deviceNumber;
	
	if (!IsMultiFormatActive(&systemContext))
			channel = NTV2_CHANNEL1;

	regNum = gChannelToGlobalControlRegNum[channel];

	return (NTV2RegisterWriteMode) ReadRegister(deviceNumber, regNum, NTV2REGWRITEMODEMASK, NTV2REGWRITEMODESHIFT);
}

void SetLEDState(ULWord deviceNumber,ULWord value)
{
	WriteRegister(	deviceNumber,
					kRegGlobalControl,
					value,
					NTV2LEDSTATEMASK,
					NTV2LEDSTATESHIFT);
}

ULWord GetLEDState(ULWord deviceNumber)
{
	ULWord regValue =  READ_REGISTER_ULWord(getNTV2Params(deviceNumber)->_pGlobalControl);
	regValue &= NTV2LEDSTATEMASK;
	regValue >>= NTV2LEDSTATESHIFT;
	return (ULWord)regValue;
}

void SetSingleLED(ULWord deviceNumber,ULWord bitNum)
{
	ULWord origValue = GetLEDState(deviceNumber);
	ULWord newValue;

	newValue = origValue | BIT(bitNum);
	SetLEDState(deviceNumber, newValue);
}

void ClearSingleLED(ULWord deviceNumber,ULWord bitNum)
{
	ULWord origValue = GetLEDState(deviceNumber);
	ULWord newValue;

	newValue = origValue & ~BIT(bitNum);
	SetLEDState(deviceNumber, newValue);
}

ULWord ReadAudioLastOut(ULWord deviceNumber)
{
	return READ_REGISTER_ULWord(getNTV2Params(deviceNumber)->_pAudioLastOut);
}

ULWord ReadAudioLastOut2(ULWord deviceNumber)
{
	return READ_REGISTER_ULWord(getNTV2Params(deviceNumber)->_pAudio2LastOut);
}

ULWord ReadAudioLastOut3(ULWord deviceNumber)
{
	return READ_REGISTER_ULWord(getNTV2Params(deviceNumber)->_pAudio3LastOut);
}

ULWord ReadAudioLastOut4(ULWord deviceNumber)
{
	return READ_REGISTER_ULWord(getNTV2Params(deviceNumber)->_pAudio4LastOut);
}

ULWord ReadAudioLastOut5(ULWord deviceNumber)
{
	return READ_REGISTER_ULWord(getNTV2Params(deviceNumber)->_pAudio5LastOut);
}

ULWord ReadAudioLastOut6(ULWord deviceNumber)
{
	return READ_REGISTER_ULWord(getNTV2Params(deviceNumber)->_pAudio6LastOut);
}

ULWord ReadAudioLastOut7(ULWord deviceNumber)
{
	return READ_REGISTER_ULWord(getNTV2Params(deviceNumber)->_pAudio7LastOut);
}

ULWord ReadAudioLastOut8(ULWord deviceNumber)
{
	return READ_REGISTER_ULWord(getNTV2Params(deviceNumber)->_pAudio8LastOut);
}

// Method: ReadAudioLastIn
	// Input:  NONE
	// Output: Audio last in address
ULWord ReadAudioLastIn(ULWord deviceNumber)
{
	return READ_REGISTER_ULWord(getNTV2Params(deviceNumber)->_pAudioLastIn);
}

ULWord ReadAudioLastIn2(ULWord deviceNumber)
{
	return READ_REGISTER_ULWord(getNTV2Params(deviceNumber)->_pAudio2LastIn);
}

ULWord ReadAudioLastIn3(ULWord deviceNumber)
{
	return READ_REGISTER_ULWord(getNTV2Params(deviceNumber)->_pAudio3LastIn);
}

ULWord ReadAudioLastIn4(ULWord deviceNumber)
{
	return READ_REGISTER_ULWord(getNTV2Params(deviceNumber)->_pAudio4LastIn);
}

ULWord ReadAudioLastIn5(ULWord deviceNumber)
{
	return READ_REGISTER_ULWord(getNTV2Params(deviceNumber)->_pAudio5LastIn);
}

ULWord ReadAudioLastIn6(ULWord deviceNumber)
{
	return READ_REGISTER_ULWord(getNTV2Params(deviceNumber)->_pAudio6LastIn);
}

ULWord ReadAudioLastIn7(ULWord deviceNumber)
{
	return READ_REGISTER_ULWord(getNTV2Params(deviceNumber)->_pAudio7LastIn);
}

ULWord ReadAudioLastIn8(ULWord deviceNumber)
{
	return READ_REGISTER_ULWord(getNTV2Params(deviceNumber)->_pAudio8LastIn);
}

//
// Method: ReadAudioSampleCounter
	// Input:  NONE
	// Output: Value of Audio Sample Counter
ULWord ReadAudioSampleCounter(ULWord deviceNumber)
{
	return  READ_REGISTER_ULWord(getNTV2Params(deviceNumber)->_pAudioSampleCounter);
}

void AvInterruptControl(ULWord deviceNumber,
						INTERRUPT_ENUMS	eInterruptType,	// Which interrupt
						ULWord			enable)			// 0: disable, nonzero: enable
{
	ULWord mask;
	ULWord bit = getNTV2ModuleParams()->intrBitLut[eInterruptType];

	NTV2PrivateParams *pNTV2Params = getNTV2Params(deviceNumber);
	unsigned long flags;
	ntv2_spin_lock_irqsave(&(pNTV2Params->_registerSpinLock), flags);
	if( eInterruptType == eInput3 || eInterruptType == eInput4 ||
		eInterruptType == eInput5 || eInterruptType == eInput6 ||
		eInterruptType == eInput7 || eInterruptType == eInput8 ||
		eInterruptType == eOutput5 || eInterruptType == eOutput6 ||
		eInterruptType == eOutput7 || eInterruptType == eOutput8 )
	{
		mask = ReadInterrupt2Register(deviceNumber);
	}
	else
	{
		mask = ReadInterruptRegister(deviceNumber);
	}

	if (enable)
	{
		mask |= bit;
	}
	else // Disable
	{
		mask &= ~bit;
	}

	if( eInterruptType == eInput3 || eInterruptType == eInput4 ||
		eInterruptType == eInput5 || eInterruptType == eInput6 ||
		eInterruptType == eInput7 || eInterruptType == eInput8 ||
		eInterruptType == eOutput5 || eInterruptType == eOutput6 ||
		eInterruptType == eOutput7 || eInterruptType == eOutput8 )
	{
		WriteInterrupt2Register(deviceNumber,mask);
	}
	else
	{
		WriteInterruptRegister(deviceNumber,mask);
	}
	ntv2_spin_unlock_irqrestore(&(pNTV2Params->_registerSpinLock), flags);
}

void ClearOutputVerticalInterrupt(ULWord deviceNumber)
{
	ULWord mask = ReadInterruptRegister(deviceNumber);
	mask = (NTV2InterruptMask)((ULWord)mask |  NTV2_OUTPUTVERTICAL_CLEAR);
	WriteInterruptRegister(deviceNumber,mask);
}

void ClearInput1VerticalInterrupt(ULWord deviceNumber)
{
	ULWord mask = ReadInterruptRegister(deviceNumber);
	mask = (NTV2InterruptMask)((ULWord)mask |  NTV2_INPUT1VERTICAL_CLEAR);
	WriteInterruptRegister(deviceNumber,mask);
}

void ClearInput2VerticalInterrupt(ULWord deviceNumber)
{
	ULWord mask = ReadInterruptRegister(deviceNumber);
	mask = (NTV2InterruptMask)((ULWord)mask |  NTV2_INPUT2VERTICAL_CLEAR);
	WriteInterruptRegister(deviceNumber,mask);
}

void ClearInput3VerticalInterrupt(ULWord deviceNumber)
{
	ULWord mask = ReadInterrupt2Register(deviceNumber);
	mask = (NTV2InterruptMask)((ULWord)mask |  NTV2_INPUT3VERTICAL_CLEAR);
	WriteInterrupt2Register(deviceNumber,mask);
}

void ClearInput4VerticalInterrupt(ULWord deviceNumber)
{
	ULWord mask = ReadInterrupt2Register(deviceNumber);
	mask = (NTV2InterruptMask)((ULWord)mask |  NTV2_INPUT4VERTICAL_CLEAR);
	WriteInterrupt2Register(deviceNumber,mask);
}

void ClearInput5VerticalInterrupt(ULWord deviceNumber)
{
	ULWord mask = ReadInterrupt2Register(deviceNumber);
	mask = (NTV2InterruptMask)((ULWord)mask |  NTV2_INPUT5VERTICAL_CLEAR);
	WriteInterrupt2Register(deviceNumber,mask);
}

void ClearInput6VerticalInterrupt(ULWord deviceNumber)
{
	ULWord mask = ReadInterrupt2Register(deviceNumber);
	mask = (NTV2InterruptMask)((ULWord)mask |  NTV2_INPUT6VERTICAL_CLEAR);
	WriteInterrupt2Register(deviceNumber,mask);
}

void ClearInput7VerticalInterrupt(ULWord deviceNumber)
{
	ULWord mask = ReadInterrupt2Register(deviceNumber);
	mask = (NTV2InterruptMask)((ULWord)mask |  NTV2_INPUT7VERTICAL_CLEAR);
	WriteInterrupt2Register(deviceNumber,mask);
}

void ClearInput8VerticalInterrupt(ULWord deviceNumber)
{
	ULWord mask = ReadInterrupt2Register(deviceNumber);
	mask = (NTV2InterruptMask)((ULWord)mask |  NTV2_INPUT8VERTICAL_CLEAR);
	WriteInterrupt2Register(deviceNumber,mask);
}

void ClearOutput2VerticalInterrupt(ULWord deviceNumber)
{
	ULWord mask = ReadInterruptRegister(deviceNumber);
	mask = (NTV2InterruptMask)((ULWord)mask | NTV2_OUTPUT2VERTICAL_CLEAR);
	WriteInterruptRegister(deviceNumber,mask);
}

void ClearOutput3VerticalInterrupt(ULWord deviceNumber)
{
	ULWord mask = ReadInterruptRegister(deviceNumber);
	mask = (NTV2InterruptMask)((ULWord)mask | NTV2_OUTPUT3VERTICAL_CLEAR);
	WriteInterruptRegister(deviceNumber,mask);
}

void ClearOutput4VerticalInterrupt(ULWord deviceNumber)
{
	ULWord mask = ReadInterruptRegister(deviceNumber);
	mask = (NTV2InterruptMask)((ULWord)mask | NTV2_OUTPUT4VERTICAL_CLEAR);
	WriteInterruptRegister(deviceNumber,mask);
}

void ClearOutput5VerticalInterrupt(ULWord deviceNumber)
{
	ULWord mask = ReadInterrupt2Register(deviceNumber);
	mask = (NTV2InterruptMask)((ULWord)mask | NTV2_OUTPUT5VERTICAL_CLEAR);
	WriteInterrupt2Register(deviceNumber,mask);
}

void ClearOutput6VerticalInterrupt(ULWord deviceNumber)
{
	ULWord mask = ReadInterrupt2Register(deviceNumber);
	mask = (NTV2InterruptMask)((ULWord)mask | NTV2_OUTPUT6VERTICAL_CLEAR);
	WriteInterrupt2Register(deviceNumber,mask);
}

void ClearOutput7VerticalInterrupt(ULWord deviceNumber)
{
	ULWord mask = ReadInterrupt2Register(deviceNumber);
	mask = (NTV2InterruptMask)((ULWord)mask | NTV2_OUTPUT7VERTICAL_CLEAR);
	WriteInterrupt2Register(deviceNumber,mask);
}

void ClearOutput8VerticalInterrupt(ULWord deviceNumber)
{
	ULWord mask = ReadInterrupt2Register(deviceNumber);
	mask = (NTV2InterruptMask)((ULWord)mask | NTV2_OUTPUT8VERTICAL_CLEAR);
	WriteInterrupt2Register(deviceNumber,mask);
}

void ClearAudioInterrupt(ULWord deviceNumber)
{
	ULWord mask = ReadInterruptRegister(deviceNumber);
	mask = (NTV2InterruptMask)((ULWord)mask |  0x10000000);
	WriteInterruptRegister(deviceNumber,mask);
}

void ClearUartRxInterrupt(ULWord deviceNumber)
{
	ULWord mask = ReadInterruptRegister(deviceNumber);
	mask = (NTV2InterruptMask)((ULWord)mask |  BIT_15);
	WriteInterruptRegister(deviceNumber,mask);
}

void ClearUartTxInterrupt(ULWord deviceNumber)
{
	ULWord mask = ReadInterruptRegister(deviceNumber);
	mask = (NTV2InterruptMask)((ULWord)mask |  BIT_24);
	WriteInterruptRegister(deviceNumber,mask);
}

void ClearUartTxInterrupt2(ULWord deviceNumber)
{
	ULWord mask = ReadInterruptRegister(deviceNumber);
	mask = (NTV2InterruptMask)((ULWord)mask |  BIT_26);
	WriteInterruptRegister(deviceNumber,mask);
}

// Method: ReadDeviceID
	// Input:  NONE
	// Output: ULWord or equivalent(i.e. ULWord).
ULWord ReadDeviceIDRegister(ULWord deviceNumber)
{
	if (getNTV2Params(deviceNumber)->pci_device == NTV2_DEVICE_ID_IO4KPLUS)
		return DEVICE_ID_IO4KPLUS;

	return  READ_REGISTER_ULWord(getNTV2Params(deviceNumber)->_pDeviceID);
}

// NTV2 DMA functions

ULWord ReadDMARegister(ULWord deviceNumber, ULWord registerNumber)
{
	unsigned long address;

	address = GetRegisterAddress(deviceNumber, registerNumber);
	return READ_REGISTER_ULWord(address);
}

void WriteDMARegister(ULWord deviceNumber, ULWord registerNumber, ULWord value)
{
	unsigned long address;

	address = GetRegisterAddress(deviceNumber, registerNumber);
	WRITE_REGISTER_ULWord(address, value);
}

bool ConfigureDMAChannels(ULWord deviceNumber)
{
	NTV2PrivateParams *pNTV2Params = getNTV2Params(deviceNumber);

	WRITE_REGISTER_ULWord(pNTV2Params->_pDMA1HostAddressHigh, 0);
	WRITE_REGISTER_ULWord(pNTV2Params->_pDMA1NextDescriptorHigh, 0);
	WRITE_REGISTER_ULWord(pNTV2Params->_pDMA2HostAddressHigh, 0);
	WRITE_REGISTER_ULWord(pNTV2Params->_pDMA2NextDescriptorHigh, 0);
	WRITE_REGISTER_ULWord(pNTV2Params->_pDMA3HostAddressHigh, 0);
	WRITE_REGISTER_ULWord(pNTV2Params->_pDMA3NextDescriptorHigh, 0);
	WRITE_REGISTER_ULWord(pNTV2Params->_pDMA4HostAddressHigh, 0);
	WRITE_REGISTER_ULWord(pNTV2Params->_pDMA4NextDescriptorHigh, 0);

	MSG("%s: configure ntv dma engines\n", pNTV2Params->name);

	return true;
}

void WriteDMAHostAddressLow(ULWord deviceNumber, ULWord index, ULWord value)
{
	const ULWord reg[] = { kRegDMA1HostAddr,
						   kRegDMA2HostAddr,
						   kRegDMA3HostAddr,
						   kRegDMA4HostAddr };

	if (index >= sizeof(reg)/sizeof(ULWord)) return;

	WriteDMARegister(deviceNumber, reg[index], value);
}

void WriteDMAHostAddressHigh(ULWord deviceNumber, ULWord index, ULWord value)
{
	const ULWord reg[] = { kRegDMA1HostAddrHigh,
						   kRegDMA2HostAddrHigh,
						   kRegDMA3HostAddrHigh,
						   kRegDMA4HostAddrHigh };

	if (index >= sizeof(reg)/sizeof(ULWord)) return;

	WriteDMARegister(deviceNumber, reg[index], value);
}

void WriteDMALocalAddress(ULWord deviceNumber, ULWord index, ULWord value)
{
	const ULWord reg[] = { kRegDMA1LocalAddr,
						   kRegDMA2LocalAddr,
						   kRegDMA3LocalAddr,
						   kRegDMA4LocalAddr };

	if (index >= sizeof(reg)/sizeof(ULWord)) return;

	WriteDMARegister(deviceNumber, reg[index], value);
}

void WriteDMATransferCount(ULWord deviceNumber, ULWord index, ULWord value)
{
	const ULWord reg[] = { kRegDMA1XferCount,
						   kRegDMA2XferCount,
						   kRegDMA3XferCount,
						   kRegDMA4XferCount };

	if (index >= sizeof(reg)/sizeof(ULWord)) return;

	WriteDMARegister(deviceNumber, reg[index], value);
}

void WriteDMANextDescriptorLow(ULWord deviceNumber, ULWord index, ULWord value)
{
	const ULWord reg[] = { kRegDMA1NextDesc,
						   kRegDMA2NextDesc,
						   kRegDMA3NextDesc,
						   kRegDMA4NextDesc };

	if (index >= sizeof(reg)/sizeof(ULWord)) return;

	WriteDMARegister(deviceNumber, reg[index], value);
}

void WriteDMANextDescriptorHigh(ULWord deviceNumber, ULWord index, ULWord value)
{
	const ULWord reg[] = { kRegDMA1NextDescHigh,
						   kRegDMA2NextDescHigh,
						   kRegDMA3NextDescHigh,
						   kRegDMA4NextDescHigh };

	if (index >= sizeof(reg)/sizeof(ULWord)) return;

	WriteDMARegister(deviceNumber, reg[index], value);
}

ULWord ReadDMAControlStatus(ULWord deviceNumber)
{
	return READ_REGISTER_ULWord(getNTV2Params(deviceNumber)->_pDMAControlStatus);
}

void WriteDMAControlStatus(ULWord deviceNumber,ULWord value)
{
	WRITE_REGISTER_ULWord(getNTV2Params(deviceNumber)->_pDMAControlStatus,value);
}

void SetDMAEngineStatus(ULWord deviceNumber, int index, bool enable)
{
	NTV2PrivateParams *pNTV2Params = getNTV2Params(deviceNumber);
	int engineBit = (1 << index);
	ULWord regValue;
	unsigned long flags;

	ntv2_spin_lock_irqsave(&(pNTV2Params->_registerSpinLock), flags);

	regValue =  READ_REGISTER_ULWord(pNTV2Params->_pDMAControlStatus);
	if (enable)
	{
		regValue |= engineBit;
	}
	else
	{
		regValue &= ~engineBit;
	}
	WRITE_REGISTER_ULWord(getNTV2Params(deviceNumber)->_pDMAControlStatus, regValue);

	ntv2_spin_unlock_irqrestore(&(pNTV2Params->_registerSpinLock), flags);
}

int GetDMAEngineStatus(ULWord deviceNumber, int index)
{
	NTV2PrivateParams *pNTV2Params = getNTV2Params(deviceNumber);
	ULWord regValue =  READ_REGISTER_ULWord(pNTV2Params->_pDMAControlStatus);
	int engineBit = (regValue >> index) & 0x1;

	return engineBit;
}

ULWord ReadDMAInterruptControl(ULWord deviceNumber)
{
	return  READ_REGISTER_ULWord(getNTV2Params(deviceNumber)->_pDMAInterruptControl);
}

void WriteDMAInterruptControl(ULWord deviceNumber,ULWord value)
{
	WRITE_REGISTER_ULWord(getNTV2Params(deviceNumber)->_pDMAInterruptControl, value);
}

void EnableDMAInterrupt(ULWord deviceNumber, NTV2DMAInterruptMask interruptMask)
{
	ULWord mask;
	NTV2PrivateParams *pNTV2Params = getNTV2Params(deviceNumber);
	unsigned long flags;

	ntv2_spin_lock_irqsave(&(pNTV2Params->_registerSpinLock), flags);
	mask = ReadDMAInterruptControl(deviceNumber);
	mask = (NTV2InterruptMask) ((ULWord)mask | (ULWord)interruptMask);
	WriteDMAInterruptControl(deviceNumber,mask);
	ntv2_spin_unlock_irqrestore(&(pNTV2Params->_registerSpinLock), flags);
}

void DisableDMAInterrupt(ULWord deviceNumber, NTV2DMAInterruptMask interruptMask)
{

	unsigned long flags;
	NTV2PrivateParams *pNTV2Params;
	ULWord mask;

	pNTV2Params = getNTV2Params(deviceNumber);
	ntv2_spin_lock_irqsave(&(pNTV2Params->_registerSpinLock), flags);
	mask = ReadDMAInterruptControl(deviceNumber);
	mask = (NTV2InterruptMask) ((ULWord)mask & (~(ULWord)interruptMask));
	WriteDMAInterruptControl(deviceNumber,mask);
	ntv2_spin_unlock_irqrestore(&(pNTV2Params->_registerSpinLock), flags);
}

void EnableDMAInterrupts(ULWord deviceNumber)
{
	NTV2PrivateParams *pNTV2Params = getNTV2Params(deviceNumber);
	ULWord boardID = pNTV2Params->_DeviceID;
	ULWord numDMAEngines = NTV2GetNumDMAEngines(boardID);

	if (numDMAEngines >= 1)
	{
		EnableDMAInterrupt(deviceNumber, NTV2_DMA1_ENABLE);
	}
	if (numDMAEngines >= 2)
	{
		EnableDMAInterrupt(deviceNumber, NTV2_DMA2_ENABLE);
	}
	if (numDMAEngines >= 3)
	{
		EnableDMAInterrupt(deviceNumber, NTV2_DMA3_ENABLE);
	}
	if (numDMAEngines >= 4)
	{
		EnableDMAInterrupt(deviceNumber, NTV2_DMA4_ENABLE);
	}
}

void DisableDMAInterrupts(ULWord deviceNumber)
{
	NTV2PrivateParams *pNTV2Params = getNTV2Params(deviceNumber);
	ULWord boardID = pNTV2Params->_DeviceID;
	ULWord numDMAEngines = NTV2GetNumDMAEngines(boardID);

	if (numDMAEngines >= 1)
	{
		DisableDMAInterrupt(deviceNumber, NTV2_DMA1_ENABLE);
	}
	if (numDMAEngines >= 2)
	{
		DisableDMAInterrupt(deviceNumber, NTV2_DMA2_ENABLE);
	}
	if (numDMAEngines >= 3)
	{
		DisableDMAInterrupt(deviceNumber, NTV2_DMA3_ENABLE);
	}
	if (numDMAEngines >= 4)
	{
		DisableDMAInterrupt(deviceNumber, NTV2_DMA4_ENABLE);
	}

	DisableDMAInterrupt(deviceNumber,NTV2_DMA_BUS_ERROR);
}

void ClearDMAInterrupt(ULWord deviceNumber, NTV2DMAStatusBits clearBit)
{
	unsigned long flags;
	NTV2PrivateParams *pNTV2Params;
	ULWord regValue = ReadDMAInterruptControl(deviceNumber);

	pNTV2Params = getNTV2Params(deviceNumber);
	ntv2_spin_lock_irqsave(&(pNTV2Params->_registerSpinLock), flags);
	regValue = ReadDMAInterruptControl(deviceNumber);
	regValue |= clearBit;
	regValue = (NTV2InterruptMask)(ULWord)regValue;
	WriteDMAInterruptControl(deviceNumber,regValue);
	ntv2_spin_unlock_irqrestore(&(pNTV2Params->_registerSpinLock), flags);
}

void ClearDMAInterrupts(ULWord deviceNumber)
{
	unsigned long flags;
	NTV2PrivateParams *pNTV2Params;
	ULWord regValue = ReadDMAInterruptControl(deviceNumber);

	pNTV2Params = getNTV2Params(deviceNumber);
	ntv2_spin_lock_irqsave(&(pNTV2Params->_registerSpinLock), flags);
	regValue = ReadDMAInterruptControl(deviceNumber);
	regValue |= (NTV2_DMA1_CLEAR + NTV2_DMA2_CLEAR + NTV2_DMA3_CLEAR + NTV2_DMA4_CLEAR);
	regValue = (NTV2InterruptMask)((ULWord)regValue);
	WriteDMAInterruptControl(deviceNumber,regValue);
	ntv2_spin_unlock_irqrestore(&(pNTV2Params->_registerSpinLock), flags);
}

// NWL DMA functions

ULWord ReadNwlRegister(ULWord deviceNumber, ULWord regNumber)
{
	NTV2PrivateParams *pNTV2Params = getNTV2Params(deviceNumber);
	unsigned long baseAddress = (unsigned long)pNTV2Params->_NwlAddress;
	ULWord memSize = (ULWord)pNTV2Params->_NwlMemorySize;
	ULWord regOffset = regNumber * 4;
	uint32_t value;

	if ((baseAddress == 0) || (regOffset >= memSize))
	{
		return 0;
	}

	value = READ_REGISTER_ULWord(baseAddress + regOffset);

	return value;
}

void WriteNwlRegister(ULWord deviceNumber, ULWord regNumber, ULWord value)
{
	NTV2PrivateParams *pNTV2Params = getNTV2Params(deviceNumber);
	unsigned long baseAddress = (unsigned long)pNTV2Params->_NwlAddress;
	ULWord memSize = (ULWord)pNTV2Params->_NwlMemorySize;
	ULWord regOffset = regNumber * 4;

	if ((baseAddress == 0) || (regOffset >= memSize))
	{
		return;
	}

	WRITE_REGISTER_ULWord(baseAddress + regOffset, value);
}

bool ConfigureNwlChannels(ULWord deviceNumber)
{
	NTV2PrivateParams *pNTV2Params = getNTV2Params(deviceNumber);
	ULWord i;

	for (i = 0; i < 2; i++)
	{
		WriteNwlChainStartAddressLow(deviceNumber, false, i, 0);
		WriteNwlChainStartAddressHigh(deviceNumber, false, i, 0);
		WriteNwlChainStartAddressLow(deviceNumber, true, i, 0);
		WriteNwlChainStartAddressHigh(deviceNumber, true, i, 0);
	}

	MSG("%s: configure nwl dma engines\n", pNTV2Params->name);

	return true;
}

bool IsNwlChannel(ULWord deviceNumber, bool bC2H, int index)
{
	ULWord cap = ReadNwlCapabilities(deviceNumber, bC2H, index);
	if ((cap & kRegMaskNwlCapabilitiesPresent) == kRegMaskNwlCapabilitiesPresent)
	{
		return true;
	}

	return false;
}

void WriteNwlCommonControlStatus(ULWord deviceNumber, ULWord value)
{
	WriteNwlRegister(deviceNumber, kRegNwlCommonControlStatus, value);
}

void WriteNwlControlStatus(ULWord deviceNumber, bool bC2H, ULWord index, ULWord value)
{
	const ULWord reg[] = { kRegNwlS2C1ControlStatus,
						   kRegNwlS2C2ControlStatus,
						   kRegNwlC2S1ControlStatus,
						   kRegNwlC2S2ControlStatus };

	if (bC2H) index += 2;
	if (index >= sizeof(reg)/sizeof(ULWord)) return;

	WriteNwlRegister(deviceNumber, reg[index], value);
}

void WriteNwlChainStartAddressLow(ULWord deviceNumber, bool bC2H, ULWord index, ULWord value)
{
	const ULWord reg[] = { kRegNwlS2C1ChainStartAddressLow,
						   kRegNwlS2C2ChainStartAddressLow,
						   kRegNwlC2S1ChainStartAddressLow,
						   kRegNwlC2S2ChainStartAddressLow };

	if (bC2H) index += 2;
	if (index >= sizeof(reg)/sizeof(ULWord)) return;

	WriteNwlRegister(deviceNumber, reg[index], value);
}

void WriteNwlChainStartAddressHigh(ULWord deviceNumber, bool bC2H, ULWord index, ULWord value)
{
	const ULWord reg[] = { kRegNwlS2C1ChainStartAddressHigh,
						   kRegNwlS2C2ChainStartAddressHigh,
						   kRegNwlC2S1ChainStartAddressHigh,
						   kRegNwlC2S2ChainStartAddressHigh };

	if (bC2H) index += 2;
	if (index >= sizeof(reg)/sizeof(ULWord)) return;

	WriteNwlRegister(deviceNumber, reg[index], value);
}

ULWord ReadNwlCommonControlStatus(ULWord deviceNumber)
{
	return ReadNwlRegister(deviceNumber, kRegNwlCommonControlStatus);
}

ULWord ReadNwlCapabilities(ULWord deviceNumber, bool bC2H, ULWord index)
{
	const ULWord reg[] = { kRegNwlS2C1Capabilities,
						   kRegNwlS2C2Capabilities,
						   kRegNwlC2S1Capabilities,
						   kRegNwlC2S2Capabilities };

	if (bC2H) index += 2;
	if (index >= sizeof(reg)/sizeof(ULWord)) return 0;

	return ReadNwlRegister(deviceNumber, reg[index]);
}

ULWord ReadNwlControlStatus(ULWord deviceNumber, bool bC2H, ULWord index)
{
	const ULWord reg[] = { kRegNwlS2C1ControlStatus,
						   kRegNwlS2C2ControlStatus,
						   kRegNwlC2S1ControlStatus,
						   kRegNwlC2S2ControlStatus };

	if (bC2H) index += 2;
	if (index >= sizeof(reg)/sizeof(ULWord)) return 0;

	return ReadNwlRegister(deviceNumber, reg[index]);
}

ULWord ReadNwlHardwareTime(ULWord deviceNumber, bool bC2H, ULWord index)
{
	const ULWord reg[] = { kRegNwlS2C1HardwareTime,
						   kRegNwlS2C2HardwareTime,
						   kRegNwlC2S1HardwareTime,
						   kRegNwlC2S2HardwareTime };

	if (bC2H) index += 2;
	if (index >= sizeof(reg)/sizeof(ULWord)) return 0;

	return ReadNwlRegister(deviceNumber, reg[index]);
}

ULWord ReadNwlChainCompleteByteCount(ULWord deviceNumber, bool bC2H, ULWord index)
{
	const ULWord reg[] = { kRegNwlS2C1ChainCompleteByteCount,
						   kRegNwlS2C2ChainCompleteByteCount,
						   kRegNwlC2S1ChainCompleteByteCount,
						   kRegNwlC2S2ChainCompleteByteCount };

	if (bC2H) index += 2;
	if (index >= sizeof(reg)/sizeof(ULWord)) return 0;

	return ReadNwlRegister(deviceNumber, reg[index]);
}

void ResetNwlHardware(ULWord deviceNumber, bool bC2H, ULWord index)
{
	int i;

	WriteNwlControlStatus(deviceNumber, bC2H, index, kRegMaskNwlControlStatusDmaResetRequest);
	for (i = 0; i < 1000; i++)
	{
		if ((ReadNwlControlStatus(deviceNumber, bC2H, index) &
			 (kRegMaskNwlControlStatusDmaResetRequest | kRegMaskNwlControlStatusChainRunning)) == 0) break;
	}
	WriteNwlControlStatus(deviceNumber, bC2H, index, kRegMaskNwlControlStatusDmaReset);
	for (i = 0; i < 1000; i++)
	{
		if ((ReadNwlControlStatus(deviceNumber, bC2H, index) & kRegMaskNwlControlStatusDmaReset) == 0) break;
	}
}

void EnableNwlUserInterrupt(ULWord deviceNumber)
{
	NTV2PrivateParams *pNTV2Params;
	ULWord regValue;
	unsigned long flags;

	pNTV2Params = getNTV2Params(deviceNumber);
	ntv2_spin_lock_irqsave(&(pNTV2Params->_nwlRegisterLock[kRegNwlCommonControlStatusIndex]), flags);

	regValue = ReadNwlRegister(deviceNumber, kRegNwlCommonControlStatus);
	regValue |= kRegMaskNwlCommonUserInterruptEnable;
	WriteNwlRegister(deviceNumber, kRegNwlCommonControlStatus, regValue);

	ntv2_spin_unlock_irqrestore(&(pNTV2Params->_nwlRegisterLock[kRegNwlCommonControlStatusIndex]), flags);
}

void DisableNwlUserInterrupt(ULWord deviceNumber)
{
	NTV2PrivateParams *pNTV2Params;
	ULWord regValue;
	unsigned long flags;

	pNTV2Params = getNTV2Params(deviceNumber);
	ntv2_spin_lock_irqsave(&(pNTV2Params->_nwlRegisterLock[kRegNwlCommonControlStatusIndex]), flags);

	regValue = ReadNwlRegister(deviceNumber, kRegNwlCommonControlStatus);
	regValue &= ~kRegMaskNwlCommonUserInterruptEnable;
	WriteNwlRegister(deviceNumber, kRegNwlCommonControlStatus, regValue);

	ntv2_spin_unlock_irqrestore(&(pNTV2Params->_nwlRegisterLock[kRegNwlCommonControlStatusIndex]), flags);
}

void EnableNwlDmaInterrupt(ULWord deviceNumber)
{
	NTV2PrivateParams *pNTV2Params;
	ULWord regValue;
	unsigned long flags;

	pNTV2Params = getNTV2Params(deviceNumber);
	ntv2_spin_lock_irqsave(&(pNTV2Params->_nwlRegisterLock[kRegNwlCommonControlStatusIndex]), flags);

	regValue = ReadNwlRegister(deviceNumber, kRegNwlCommonControlStatus);
	regValue |= kRegMaskNwlCommonDmaInterruptEnable;
	WriteNwlRegister(deviceNumber, kRegNwlCommonControlStatus, regValue);

	ntv2_spin_unlock_irqrestore(&(pNTV2Params->_nwlRegisterLock[kRegNwlCommonControlStatusIndex]), flags);
}

void DisableNwlDmaInterrupt(ULWord deviceNumber)
{
	NTV2PrivateParams *pNTV2Params;
	ULWord regValue;
	unsigned long flags;

	pNTV2Params = getNTV2Params(deviceNumber);
	ntv2_spin_lock_irqsave(&(pNTV2Params->_nwlRegisterLock[kRegNwlCommonControlStatusIndex]), flags);

	regValue = ReadNwlRegister(deviceNumber, kRegNwlCommonControlStatus);
	regValue &= ~kRegMaskNwlCommonDmaInterruptEnable;
	WriteNwlRegister(deviceNumber, kRegNwlCommonControlStatus, regValue);

	ntv2_spin_unlock_irqrestore(&(pNTV2Params->_nwlRegisterLock[kRegNwlCommonControlStatusIndex]), flags);
}

void ClearNwlUserInterrupt(ULWord deviceNumber)
{
	NTV2PrivateParams *pNTV2Params;
	ULWord regValue;
	unsigned long flags;

	pNTV2Params = getNTV2Params(deviceNumber);
	ntv2_spin_lock_irqsave(&(pNTV2Params->_nwlRegisterLock[kRegNwlCommonControlStatusIndex]), flags);

	regValue = ReadNwlRegister(deviceNumber, kRegNwlCommonControlStatus);
	regValue |= kRegMaskNwlCommonUserInterruptActive;
	WriteNwlRegister(deviceNumber, kRegNwlCommonControlStatus, regValue);

	ntv2_spin_unlock_irqrestore(&(pNTV2Params->_nwlRegisterLock[kRegNwlCommonControlStatusIndex]), flags);
}

void ClearNwlS2C0Interrupt(ULWord deviceNumber)
{
	NTV2PrivateParams *pNTV2Params;
	ULWord regValue;
	unsigned long flags;

	pNTV2Params = getNTV2Params(deviceNumber);
	ntv2_spin_lock_irqsave(&(pNTV2Params->_nwlRegisterLock[kRegNwlS2C1ControlStatusIndex]), flags);

	regValue =  ReadNwlRegister(deviceNumber, kRegNwlS2C1ControlStatus);
	regValue |= kRegMaskNwlControlStatusInterruptActive;
	WriteNwlRegister(deviceNumber, kRegNwlS2C1ControlStatus, regValue);

	ntv2_spin_unlock_irqrestore(&(pNTV2Params->_nwlRegisterLock[kRegNwlS2C1ControlStatusIndex]), flags);
}

void ClearNwlC2S0Interrupt(ULWord deviceNumber)
{
	NTV2PrivateParams *pNTV2Params;
	ULWord regValue;
	unsigned long flags;

	pNTV2Params = getNTV2Params(deviceNumber);
	ntv2_spin_lock_irqsave(&(pNTV2Params->_nwlRegisterLock[kRegNwlC2S1ControlStatusIndex]), flags);

	regValue =  ReadNwlRegister(deviceNumber, kRegNwlC2S1ControlStatus);
	regValue |= kRegMaskNwlControlStatusInterruptActive;
	WriteNwlRegister(deviceNumber, kRegNwlC2S1ControlStatus, regValue);

	ntv2_spin_unlock_irqrestore(&(pNTV2Params->_nwlRegisterLock[kRegNwlC2S1ControlStatusIndex]), flags);
}

void ClearNwlS2C1Interrupt(ULWord deviceNumber)
{
	NTV2PrivateParams *pNTV2Params;
	ULWord regValue;
	unsigned long flags;

	pNTV2Params = getNTV2Params(deviceNumber);
	ntv2_spin_lock_irqsave(&(pNTV2Params->_nwlRegisterLock[kRegNwlS2C2ControlStatusIndex]), flags);

	regValue =  ReadNwlRegister(deviceNumber, kRegNwlS2C2ControlStatus);
	regValue |= kRegMaskNwlControlStatusInterruptActive;
	WriteNwlRegister(deviceNumber, kRegNwlS2C2ControlStatus, regValue);

	ntv2_spin_unlock_irqrestore(&(pNTV2Params->_nwlRegisterLock[kRegNwlS2C2ControlStatusIndex]), flags);
}

void ClearNwlC2S1Interrupt(ULWord deviceNumber)
{
	NTV2PrivateParams *pNTV2Params;
	ULWord regValue;
	unsigned long flags;

	pNTV2Params = getNTV2Params(deviceNumber);
	ntv2_spin_lock_irqsave(&(pNTV2Params->_nwlRegisterLock[kRegNwlC2S2ControlStatusIndex]), flags);

	regValue =  ReadNwlRegister(deviceNumber, kRegNwlC2S2ControlStatus);
	regValue |= kRegMaskNwlControlStatusInterruptActive;
	WriteNwlRegister(deviceNumber, kRegNwlC2S2ControlStatus, regValue);

	ntv2_spin_unlock_irqrestore(&(pNTV2Params->_nwlRegisterLock[kRegNwlC2S2ControlStatusIndex]), flags);
}

// Xilinx DMA functions

ULWord ReadXlnxRegister(ULWord deviceNumber, ULWord regNum)
{
	NTV2PrivateParams *pNTV2Params = getNTV2Params(deviceNumber);
	uint32_t offset = regNum * 4;
	uint32_t value;

	if ((pNTV2Params->_XlnxAddress == 0) || (offset >= pNTV2Params->_XlnxMemorySize))
	{
		return 0;
	}

	value = READ_REGISTER_ULWord(pNTV2Params->_XlnxAddress + offset);

	return value;
}

void WriteXlnxRegister(ULWord deviceNumber, ULWord registerNumber, ULWord value)
{
	NTV2PrivateParams *pNTV2Params = getNTV2Params(deviceNumber);
	uint32_t offset = registerNumber * 4;

	if ((pNTV2Params->_XlnxAddress == 0) || (offset >= pNTV2Params->_XlnxMemorySize))
	{
		return;
	}

	WRITE_REGISTER_ULWord(pNTV2Params->_XlnxAddress + offset, value);
}

bool ConfigureXlnxChannels(ULWord deviceNumber)
{
	NTV2PrivateParams *pNTV2Params = getNTV2Params(deviceNumber);
	ULWord base;
	ULWord value;
	int i;

	if (pNTV2Params->_XlnxAddress == 0)
	{
		return false;
	}

	pNTV2Params->_numXlnxH2CEngines = 0;
	pNTV2Params->_numXlnxC2HEngines = 0;

	for (i = 0; i < XLNX_MAX_CHANNELS; i++)
	{
		base = (kRegXlnxTargetChannelH2C * XLNX_REG_TARGET_SIZE) + (i * XLNX_REG_CHANNEL_SIZE);
		value = ReadXlnxRegister(deviceNumber, base + kRegXlnxChannelIdentifier);

		if ((((value & kRegMaskXlnxSubsystemId) >> kRegShiftXlnxSubsystemId) != XLNX_SUBSYSTEM_ID) ||
			(((value & kRegMaskXlnxTarget) >> kRegShiftXlnxTarget) != kRegXlnxTargetChannelH2C))
		{
			break;
		}
		pNTV2Params->_numXlnxH2CEngines++;

		WriteXlnxRegister(deviceNumber, base + kRegXlnxChannelControl, 0x0);
	}

	for (i = 0; i < XLNX_MAX_CHANNELS; i++)
	{
		base = (kRegXlnxTargetChannelC2H * XLNX_REG_TARGET_SIZE) + (i * XLNX_REG_CHANNEL_SIZE);
		value = ReadXlnxRegister(deviceNumber, base + kRegXlnxChannelIdentifier);

		if ((((value & kRegMaskXlnxSubsystemId) >> kRegShiftXlnxSubsystemId) != XLNX_SUBSYSTEM_ID) ||
			(((value & kRegMaskXlnxTarget) >> kRegShiftXlnxTarget) != kRegXlnxTargetChannelC2H))
		{
			break;
		}
		pNTV2Params->_numXlnxC2HEngines++;

		WriteXlnxRegister(deviceNumber, base + kRegXlnxChannelControl, 0x0);
	}

	MSG("%s: configure xilinx dma engines  h2c %d  c2h %d\n",
		pNTV2Params->name, pNTV2Params->_numXlnxH2CEngines, pNTV2Params->_numXlnxC2HEngines);

	return true;
}

bool IsXlnxChannel(ULWord deviceNumber, bool bC2H, int index)
{
	NTV2PrivateParams *pNTV2Params = getNTV2Params(deviceNumber);

	if (bC2H)
	{
		if (index < pNTV2Params->_numXlnxC2HEngines)
		{
			return true;
		}
	}
	else
	{
		if (index < pNTV2Params->_numXlnxH2CEngines)
		{
			return true;
		}
	}

	return false;
}

ULWord XlnxChannelRegBase(ULWord deviceNumber, bool bC2H, int index)
{
	ULWord reg;

	if (!IsXlnxChannel(deviceNumber, index, bC2H))
	{
		index = 0;
	}

	reg = bC2H? kRegXlnxTargetChannelC2H : kRegXlnxTargetChannelH2C;
	reg *= XLNX_REG_TARGET_SIZE;
	reg += index*XLNX_REG_CHANNEL_SIZE;

	return reg;
}

ULWord XlnxSgdmaRegBase(ULWord deviceNumber, bool bC2H, int index)
{
	ULWord reg;

	if (!IsXlnxChannel(deviceNumber, bC2H, index))
	{
		index = 0;
	}

	reg = bC2H? kRegXlnxTargetSgdmaC2H : kRegXlnxTargetSgdmaH2C;
	reg *= XLNX_REG_TARGET_SIZE;
	reg += index*XLNX_REG_CHANNEL_SIZE;

	return reg;
}

ULWord XlnxConfigRegBase(ULWord deviceNumber)
{
	return kRegXlnxTargetConfig * XLNX_REG_TARGET_SIZE;;
}

ULWord XlnxIrqRegBase(ULWord deviceNumber)
{
	return kRegXlnxTargetIRQ * XLNX_REG_TARGET_SIZE;;
}

ULWord XlnxIrqBitMask(ULWord deviceNumber, bool bC2H, int index)
{
	NTV2PrivateParams *pNTV2Params = getNTV2Params(deviceNumber);
	ULWord bit;

	if (!IsXlnxChannel(deviceNumber, bC2H, index))
	{
		return 0;
	}

	bit = bC2H? pNTV2Params->_numXlnxH2CEngines + index : index;

	return ((ULWord)0x1) << bit;
}

void EnableXlnxUserInterrupt(ULWord deviceNumber, int index)
{
	WriteXlnxRegister(deviceNumber,
					  XlnxIrqRegBase(deviceNumber) + kRegXlnxIrqUserInterruptEnableW1S,
					  ((ULWord)0x1 << index));
}

void DisableXlnxUserInterrupt(ULWord deviceNumber, int index)
{
	WriteXlnxRegister(deviceNumber,
					  XlnxIrqRegBase(deviceNumber) + kRegXlnxIrqUserInterruptEnableW1C,
					  ((ULWord)0x1 << index));
}

ULWord ReadXlnxUserInterrupt(ULWord deviceNumber)
{
	ULWord value;

	value = ReadXlnxRegister(deviceNumber, XlnxIrqRegBase(deviceNumber) + kRegXlnxIrqUserInterruptRequest);
	return value & 0x1;
}

bool IsXlnxUserInterrupt(ULWord deviceNumber, int index, ULWord intReg)
{
	return ((intReg & ((ULWord)0x1 << index)) != 0)? true : false;
}

void EnableXlnxDmaInterrupt(ULWord deviceNumber, bool bC2H, int index)
{
	WriteXlnxRegister(deviceNumber,
					  XlnxIrqRegBase(deviceNumber) + kRegXlnxIrqChannelInterruptEnableW1S,
					  XlnxIrqBitMask(deviceNumber, bC2H, index));
}

void DisableXlnxDmaInterrupt(ULWord deviceNumber, bool bC2H, int index)
{
	WriteXlnxRegister(deviceNumber,
					  XlnxIrqRegBase(deviceNumber) + kRegXlnxIrqChannelInterruptEnableW1C,
					  XlnxIrqBitMask(deviceNumber, bC2H, index));
}

void EnableXlnxDmaInterrupts(ULWord deviceNumber)
{
	NTV2PrivateParams *pNTV2Params = getNTV2Params(deviceNumber);
	int i;

	for (i = 0; i < pNTV2Params->_numXlnxH2CEngines; i++)
	{
		EnableXlnxDmaInterrupt(deviceNumber, false, i);
	}

	for (i = 0; i < pNTV2Params->_numXlnxC2HEngines; i++)
	{
		EnableXlnxDmaInterrupt(deviceNumber, true, i);
	}
}

void DisableXlnxDmaInterrupts(ULWord deviceNumber)
{
	NTV2PrivateParams *pNTV2Params = getNTV2Params(deviceNumber);
	int i;

	for (i = 0; i < pNTV2Params->_numXlnxH2CEngines; i++)
	{
		DisableXlnxDmaInterrupt(deviceNumber, false, i);
	}

	for (i = 0; i < pNTV2Params->_numXlnxC2HEngines; i++)
	{
		DisableXlnxDmaInterrupt(deviceNumber, true, i);
	}
}

void DisableXlnxInterrupts(ULWord deviceNumber)
{
	WriteXlnxRegister(deviceNumber, XlnxIrqRegBase(deviceNumber) + kRegXlnxIrqUserInterruptEnableW1C, 0xffffffff);
	WriteXlnxRegister(deviceNumber, XlnxIrqRegBase(deviceNumber) + kRegXlnxIrqChannelInterruptEnableW1C, 0xffffffff);
}

ULWord ReadXlnxDmaInterrupt(ULWord deviceNumber)
{
	ULWord value;

	value = ReadXlnxRegister(deviceNumber, XlnxIrqRegBase(deviceNumber) + kRegXlnxIrqChannelInterruptRequest);
	return value;
}

bool IsXlnxDmaInterrupt(ULWord deviceNumber, bool bC2H, int index, ULWord intReg)
{
	if (!IsXlnxChannel(deviceNumber, bC2H, index))
	{
		return false;
	}

	return ((intReg & XlnxIrqBitMask(deviceNumber, bC2H, index)) != 0)? true : false;
}

bool StartXlnxDma(ULWord deviceNumber, bool bC2H, int index)
{
	ULWord base;
	ULWord value;

	if (!IsXlnxChannel(deviceNumber, bC2H, index))
	{
		return false;
	}

	base = XlnxChannelRegBase(deviceNumber, bC2H, index);

	// enable interrupts
	value = kRegMaskXlnxIntDescError;
	value |= kRegMaskXlnxIntReadError;
	value |= kRegMaskXlnxIntMagicStop;
	value |= kRegMaskXlnxIntAlignMismatch;
	value |= kRegMaskXlnxIntDescStop;
	WriteXlnxRegister(deviceNumber, base + kRegXlnxChannelInterruptEnable, value);

	// start dma
	value = kRegMaskXlnxRun;
	value |= kRegMaskXlnxIntDescError;
	value |= kRegMaskXlnxIntReadError;
	value |= kRegMaskXlnxIntMagicStop;
	value |= kRegMaskXlnxIntAlignMismatch;
	value |= kRegMaskXlnxIntDescStop;
	WriteXlnxRegister(deviceNumber, base + kRegXlnxChannelControl, value);

	return true;
}

bool StopXlnxDma(ULWord deviceNumber, bool bC2H, int index)
{
	ULWord base;

	if (!IsXlnxChannel(deviceNumber, bC2H, index))
	{
		return false;
	}

	base = XlnxChannelRegBase(deviceNumber, bC2H, index);

	// disable interrupts
	WriteXlnxRegister(deviceNumber, base + kRegXlnxChannelInterruptEnable, 0);

	// stop dma
	WriteXlnxRegister(deviceNumber, base + kRegXlnxChannelControl, 0);

	return true;
}

bool WaitXlnxDmaActive(ULWord deviceNumber, bool bC2H, int index, bool active)
{
	ULWord status;
	int i;

	if (!IsXlnxChannel(deviceNumber, bC2H, index))
	{
		return false;
	}

	for (i = 0; i < 1000; i++)
	{

		status = ReadXlnxDmaStatus(deviceNumber, bC2H, index);
		if (IsXlnxDmaActive(status) == active)
		{
			return true;
		}
	}

	return false;
}

void StopAllXlnxDma(ULWord deviceNumber)
{
	NTV2PrivateParams *pNTV2Params = getNTV2Params(deviceNumber);
	int i;

	for (i = 0; i < pNTV2Params->_numXlnxH2CEngines; i++)
	{
		StopXlnxDma(deviceNumber, false, i);
	}

	for (i = 0; i < pNTV2Params->_numXlnxC2HEngines; i++)
	{
		StopXlnxDma(deviceNumber, true, i);
	}
}

ULWord ReadXlnxDmaStatus(ULWord deviceNumber, bool bC2H, int index)
{
	return ReadXlnxRegister(deviceNumber,
							XlnxChannelRegBase(deviceNumber, bC2H, index) + kRegXlnxChannelStatus);
}

ULWord ClearXlnxDmaStatus(ULWord deviceNumber, bool bC2H, int index)
{
	return ReadXlnxRegister(deviceNumber,
							XlnxChannelRegBase(deviceNumber, bC2H, index) + kRegXlnxChannelStatusRC);
}

bool IsXlnxDmaActive(ULWord status)
{
	return (status & kRegMaskXlnxRun) != 0;
}

bool IsXlnxDmaError(ULWord status)
{
	ULWord value;

	value = kRegMaskXlnxIntDescError;
	value |= kRegMaskXlnxIntReadError;
	value |= kRegMaskXlnxIntMagicStop;
	value |= kRegMaskXlnxIntAlignMismatch;

	return (status & value) != 0;
}

void WriteXlnxDmaEngineStartLow(ULWord deviceNumber, bool bC2H, int index, ULWord addressLow)
{
	ULWord base;

	if (!IsXlnxChannel(deviceNumber, bC2H, index))
	{
		return;
	}

	base = XlnxSgdmaRegBase(deviceNumber, bC2H, index);

	WriteXlnxRegister(deviceNumber, base + kRegXlnxSgdmaDescAddressLow, addressLow);
}

void WriteXlnxDmaEngineStartHigh(ULWord deviceNumber, bool bC2H, int index, ULWord addressHigh)
{
	ULWord base;

	if (!IsXlnxChannel(deviceNumber, bC2H, index))
	{
		return;
	}

	base = XlnxSgdmaRegBase(deviceNumber, bC2H, index);

	WriteXlnxRegister(deviceNumber, base + kRegXlnxSgdmaDescAddressHigh, addressHigh);
}

void WriteXlnxDmaEngineStartAdjacent(ULWord deviceNumber, bool bC2H, int index, ULWord adjacent)
{
	ULWord base;

	if (!IsXlnxChannel(deviceNumber, bC2H, index))
	{
		return;
	}

	base = XlnxSgdmaRegBase(deviceNumber, bC2H, index);

	WriteXlnxRegister(deviceNumber, base + kRegXlnxSgdmaDescAdjacent, adjacent);
}

ULWord ReadXlnxPerformanceCycleCount(ULWord deviceNumber, bool bC2H, int index)
{
	ULWord base;

	if (!IsXlnxChannel(deviceNumber, bC2H, index))
	{
		return 0;
	}

	base = XlnxSgdmaRegBase(deviceNumber, bC2H, index);

	return ReadXlnxRegister(deviceNumber, base + kRegXlnxChannelPerfCycleCountLow);
}

ULWord ReadXlnxPerformanceDataCount(ULWord deviceNumber, bool bC2H, int index)
{
	ULWord base;

	if (!IsXlnxChannel(deviceNumber, bC2H, index))
	{
		return 0;
	}

	base = XlnxSgdmaRegBase(deviceNumber, bC2H, index);

	return ReadXlnxRegister(deviceNumber, base + kRegXlnxChannelPerfDataCountLow);
}

ULWord ReadXlnxMaxReadRequestSize(ULWord deviceNumber)
{
	return ReadXlnxRegister(deviceNumber,
							XlnxConfigRegBase(deviceNumber) + kRegXlnxChannelUserMaxReadRequestSize);
}

// Interrupt control

void EnableAllInterrupts(ULWord deviceNumber)
{
	NTV2PrivateParams *pNTV2Params = getNTV2Params(deviceNumber);
	ULWord boardID = pNTV2Params->_DeviceID;

	int inputChannelCount;
	int outputChannelCount = NTV2DeviceCanDoMultiFormat(boardID)
							 ? NTV2DeviceGetNumVideoChannels(boardID)
							 : 1;

	MSG("%s: enable video interrupts\n", pNTV2Params->name);

	switch( outputChannelCount )
	{
	// Fall through on purpose
	case 8:
		AvInterruptControl(deviceNumber, eOutput8, 1);
		// fall through
	case 7:
		AvInterruptControl(deviceNumber, eOutput7, 1);
		// fall through
	case 6:
		AvInterruptControl(deviceNumber, eOutput6, 1);
		// fall through
	case 5:
		AvInterruptControl(deviceNumber, eOutput5, 1);
		// fall through
	case 4:
		AvInterruptControl(deviceNumber, eOutput4, 1);
		// fall through
	case 3:
		AvInterruptControl(deviceNumber, eOutput3, 1);
		// fall through
	case 2:
		AvInterruptControl(deviceNumber, eOutput2, 1);
		// fall through
	case 1:
		// fall through
	default:
		AvInterruptControl(deviceNumber, eVerticalInterrupt, 1);
		break;
	}

	inputChannelCount = NTV2DeviceGetNumVideoChannels(boardID);

	switch( inputChannelCount )
	{
	// Fall through on purpose
	case 8:
		AvInterruptControl(deviceNumber, eInput8, 1);
		// fall through
	case 7:
		AvInterruptControl(deviceNumber, eInput7, 1);
		// fall through
	case 6:
		AvInterruptControl(deviceNumber, eInput6, 1);
		// fall through
	case 5:
		AvInterruptControl(deviceNumber, eInput5, 1);
		// fall through
	case 4:
		AvInterruptControl(deviceNumber, eInput4, 1);
		// fall through
	case 3:
		AvInterruptControl(deviceNumber, eInput3, 1);
		// fall through
	case 2:
		AvInterruptControl(deviceNumber, eInput2, 1);
		// fall through
	case 1:
		// fall through
	default:
		AvInterruptControl(deviceNumber, eInput1, 1);
	}

//	AvInterruptControl(deviceNumber, eAudio, 1);
//	AvInterruptControl(deviceNumber, eAudioInWrap, 1);
//	AvInterruptControl(deviceNumber, eAudioOutWrap, 1);
//	AvInterruptControl(deviceNumber, eWrapRate, 1);

	// Don't enable Xena's 422 UART interrupts in case
	// the user wants to poll
	//
	// AvInterruptControl(deviceNumber, eUartRx, 1);
	// AvInterruptControl(deviceNumber, eUartTx, 1);
	// AvInterruptControl(deviceNumber, eUartTx2, 1);

	// Enable DMA interrupts
	switch(pNTV2Params->_dmaMethod)
	{
	case DmaMethodAja:
		MSG("%s: enable aja dma interrupts\n", pNTV2Params->name);
		EnableDMAInterrupts(deviceNumber);
		break;
	case DmaMethodNwl:
		MSG("%s: enable nwl dma interrupts\n", pNTV2Params->name);
		EnableNwlUserInterrupt(deviceNumber);
		EnableNwlDmaInterrupt(deviceNumber);
		break;
	case DmaMethodXlnx:
		MSG("%s: enable xlnx user interrupt\n", pNTV2Params->name);
		EnableXlnxUserInterrupt(deviceNumber, 0);
		break;
	default:
		break;
	}

	// enable p2p message interrupts
	if(DeviceCanDoP2P(deviceNumber) && (getNTV2Params(deviceNumber)->_FrameApertureBaseAddress != 0))
	{
		ULWord numChannels = NTV2DeviceGetNumVideoChannels(getNTV2Params(deviceNumber)->_DeviceID);

		if(numChannels > 0)
		{
			EnableMessageChannel1Interrupt(deviceNumber);
		}
		if(numChannels > 1)
		{
			EnableMessageChannel2Interrupt(deviceNumber);
		}
		if(numChannels > 2)
		{
			EnableMessageChannel3Interrupt(deviceNumber);
		}
		if(numChannels > 3)
		{
			EnableMessageChannel4Interrupt(deviceNumber);
		}
	}
}

void DisableAllInterrupts(ULWord deviceNumber)
{
	NTV2PrivateParams *pNTV2Params = getNTV2Params(deviceNumber);
	ULWord boardID = pNTV2Params->_DeviceID;
	int inputChannelCount = NTV2DeviceGetNumVideoChannels(boardID);
	int outputChannelCount = NTV2DeviceCanDoMultiFormat(boardID)
							 ? NTV2DeviceGetNumVideoChannels(boardID)
							 : 1;

	switch( outputChannelCount )
	{
	// Fall through on purpose
	case 8:
		AvInterruptControl(deviceNumber, eOutput8, 0);
		// fall through
	case 7:
		AvInterruptControl(deviceNumber, eOutput7, 0);
		// fall through
	case 6:
		AvInterruptControl(deviceNumber, eOutput6, 0);
		// fall through
	case 5:
		AvInterruptControl(deviceNumber, eOutput5, 0);
		// fall through
	case 4:
		AvInterruptControl(deviceNumber, eOutput4, 0);
		// fall through
	case 3:
		AvInterruptControl(deviceNumber, eOutput3, 0);
		// fall through
	case 2:
		AvInterruptControl(deviceNumber, eOutput2, 0);
		// fall through
	case 1:
		// fall through
	default:
		AvInterruptControl(deviceNumber, eVerticalInterrupt, 0);
		break;
	}

	switch( inputChannelCount )
	{
	// Fall through on purpose
	case 8:
		AvInterruptControl(deviceNumber, eInput8, 0);
		// fall through
	case 7:
		AvInterruptControl(deviceNumber, eInput7, 0);
		// fall through
	case 6:
		AvInterruptControl(deviceNumber, eInput6, 0);
		// fall through
	case 5:
		AvInterruptControl(deviceNumber, eInput5, 0);
		// fall through
	case 4:
		AvInterruptControl(deviceNumber, eInput4, 0);
		// fall through
	case 3:
		AvInterruptControl(deviceNumber, eInput3, 0);
		// fall through
	case 2:
		AvInterruptControl(deviceNumber, eInput2, 0);
		// fall through
	case 1:
		// fall through
	default:
		AvInterruptControl(deviceNumber, eInput1, 0);
	}

	AvInterruptControl(deviceNumber, eAudio, 0);
	AvInterruptControl(deviceNumber, eAudioInWrap, 0);
	AvInterruptControl(deviceNumber, eAudioOutWrap, 0);
	AvInterruptControl(deviceNumber, eWrapRate, 0);
	AvInterruptControl(deviceNumber, eUartRx,  0);
	AvInterruptControl(deviceNumber, eUartTx,  0);
	AvInterruptControl(deviceNumber, eUartTx2, 0);

	if (pNTV2Params->_FrameApertureBaseAddress)
	{
		DisableMessageChannel1Interrupt(deviceNumber);
		DisableMessageChannel2Interrupt(deviceNumber);
		DisableMessageChannel3Interrupt(deviceNumber);
		DisableMessageChannel4Interrupt(deviceNumber);
	}

	// Disable DMA interrupts
	switch(pNTV2Params->_dmaMethod)
	{
	case DmaMethodAja:
		MSG("%s: disable aja dma interrupts\n", pNTV2Params->name);
		DisableDMAInterrupts(deviceNumber);
		break;
	case DmaMethodNwl:
		MSG("%s: disable nwl user and dma interrupts\n", pNTV2Params->name);
		DisableNwlUserInterrupt(deviceNumber);
		DisableNwlDmaInterrupt(deviceNumber);
		break;
	case DmaMethodXlnx:
		MSG("%s: disable xlnx user and dma interrupts\n", pNTV2Params->name);
		DisableXlnxUserInterrupt(deviceNumber, 0);
		DisableXlnxDmaInterrupts(deviceNumber);
		break;
	default:
		break;
	}
}

void StopAllDMAEngines(ULWord deviceNumber)
{
	NTV2PrivateParams *pNTV2Params;
	int dmaEngineIdx;

	pNTV2Params = getNTV2Params(deviceNumber);

	for (	dmaEngineIdx = 0;
			dmaEngineIdx < NTV2GetNumDMAEngines(pNTV2Params->_DeviceID);
			dmaEngineIdx++)
	{
		// A stop will stop both directions
		SetDMAEngineStatus(deviceNumber, dmaEngineIdx, false);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////
// OEM RP188 methods


void SetLTCData (ULWord deviceNumber, NTV2Channel channel, RP188_STRUCT rp188Data)
{
	// ULWord flags;
	NTV2PrivateParams *pNTV2Params;
	pNTV2Params = getNTV2Params(deviceNumber);
	WriteRegister(deviceNumber, kRegLTCOutBits0_31, rp188Data.Low, NO_MASK, NO_SHIFT);
	WriteRegister(deviceNumber, kRegLTCOutBits32_63, rp188Data.High, NO_MASK, NO_SHIFT );
}

void GetLTCData (ULWord deviceNumber, NTV2Channel channel, RP188_STRUCT* rp188Data)
{
	// ULWord flags;
	NTV2PrivateParams *pNTV2Params;
	pNTV2Params = getNTV2Params(deviceNumber);
	rp188Data->Low  = ReadRegister(deviceNumber, kRegLTCInBits0_31, NO_MASK, NO_SHIFT);
	rp188Data->High = ReadRegister(deviceNumber, kRegLTCInBits32_63, NO_MASK, NO_SHIFT);
}

// Method: SetRP188Data
// Input:  RP188 mode
// Output: NONE
void SetRP188Data (ULWord deviceNumber, NTV2Channel channel, RP188_STRUCT rp188Data)
{
	// ULWord flags;
	NTV2PrivateParams *pNTV2Params;

	pNTV2Params = getNTV2Params(deviceNumber);
	// ntv2_spin_lock_irqsave(&(pNTV2Params->rp188Lock), flags);
	if (channel == NTV2_CHANNEL1)
	{
		WriteRegister(deviceNumber, kRegRP188InOut1DBB, 		rp188Data.DBB, NO_MASK, NO_SHIFT);
		WriteRegister(deviceNumber, kRegRP188InOut1Bits0_31, rp188Data.Low, NO_MASK, NO_SHIFT);
		WriteRegister(deviceNumber, kRegRP188InOut1Bits32_63,rp188Data.High, NO_MASK, NO_SHIFT );
	}
	else if (channel == NTV2_CHANNEL2)
	{
		WriteRegister(deviceNumber, kRegRP188InOut2DBB,		rp188Data.DBB, NO_MASK, NO_SHIFT );
		WriteRegister(deviceNumber, kRegRP188InOut2Bits0_31,	rp188Data.Low, NO_MASK, NO_SHIFT);
		WriteRegister(deviceNumber, kRegRP188InOut2Bits32_63,rp188Data.High, NO_MASK, NO_SHIFT );
	}
	// ntv2_spin_unlock_irqrestore(&(pNTV2Params->rp188Lock), flags);
}

// Method: GetRP188Data
// Input:  NONE
// Output: RP188 mode
void GetRP188Data (ULWord deviceNumber, NTV2Channel channel, RP188_STRUCT* rp188Data)
{
	// ULWord flags;
	NTV2PrivateParams *pNTV2Params;

	pNTV2Params = getNTV2Params(deviceNumber);
	// ntv2_spin_lock_irqsave(&(pNTV2Params->rp188Lock), flags);
	if (channel == NTV2_CHANNEL1)
	{
		rp188Data->DBB  = ReadRegister(deviceNumber, kRegRP188InOut1DBB, NO_MASK, NO_SHIFT);
		rp188Data->Low  = ReadRegister(deviceNumber, kRegRP188InOut1Bits0_31, NO_MASK, NO_SHIFT);
		rp188Data->High = ReadRegister(deviceNumber, kRegRP188InOut1Bits32_63, NO_MASK, NO_SHIFT);
	}
	else if (channel == NTV2_CHANNEL2)
	{
		rp188Data->DBB  = ReadRegister(deviceNumber, kRegRP188InOut2DBB, NO_MASK, NO_SHIFT);
		rp188Data->Low  = ReadRegister(deviceNumber, kRegRP188InOut2Bits0_31, NO_MASK, NO_SHIFT);
		rp188Data->High = ReadRegister(deviceNumber, kRegRP188InOut2Bits32_63, NO_MASK, NO_SHIFT);
	}
	// ntv2_spin_unlock_irqrestore(&(pNTV2Params->rp188Lock), flags);
}

////////////////////////////////////////////////////////////////////////////////////////////
// OEM Custom Ancillary data methods

// Method: SetCustomAncillaryDataMode
// Input:  NTV2Channel channel
// Output: NONE
void SetCustomAncillaryDataMode(ULWord deviceNumber, NTV2Channel channel, bool bEnable)
{
}

//////////////////////////////////////////////////////////////////
// OEM Color Correction Methods
//
void  SetColorCorrectionMode(ULWord deviceNumber, NTV2Channel channel, NTV2ColorCorrectionMode mode)
{
	WriteRegister(	deviceNumber,
					(channel == NTV2_CHANNEL1) ?
						kRegCh1ColorCorrectioncontrol : kRegCh2ColorCorrectioncontrol,
					mode,
					kRegMaskCCMode,
					kRegShiftCCMode);
}

ULWord
GetColorCorrectionMode(ULWord deviceNumber, NTV2Channel channel)
{
	return ReadRegister(deviceNumber,
						(channel == NTV2_CHANNEL1) ?
								kRegCh1ColorCorrectioncontrol : kRegCh2ColorCorrectioncontrol,
								kRegMaskCCMode,
								kRegShiftCCMode);
}


void
SetColorCorrectionOutputBank (ULWord deviceNumber, NTV2Channel channel, ULWord bank)
{
	if( NTV2DeviceGetLUTVersion(getNTV2Params(deviceNumber)->_DeviceID) == 2 )
	{
		return SetLUTV2OutputBank(deviceNumber, channel, bank);
	}

	switch( channel )
	{
	default:
	case NTV2_CHANNEL1:
		WriteRegister (	deviceNumber,
						kRegCh1ColorCorrectioncontrol,
						bank,
						kRegMaskCCOutputBankSelect,
						kRegShiftCCOutputBankSelect);
		break;

	case NTV2_CHANNEL2:
		WriteRegister (	deviceNumber,
						kRegCh2ColorCorrectioncontrol,
						bank,
						kRegMaskCCOutputBankSelect,
						kRegShiftCCOutputBankSelect);
		break;

	case NTV2_CHANNEL3:
		WriteRegister (	deviceNumber,
						kRegCh2ColorCorrectioncontrol,
						bank,
						kRegMaskCC3OutputBankSelect,
						kRegShiftCC3OutputBankSelect);
		break;

	case NTV2_CHANNEL4:
		WriteRegister (	deviceNumber,
						kRegCh2ColorCorrectioncontrol,
						bank,
						kRegMaskCC4OutputBankSelect,
						kRegShiftCC4OutputBankSelect);
		break;
	}
}

ULWord
GetColorCorrectionOutputBank (ULWord deviceNumber, NTV2Channel channel)
{
	if( NTV2DeviceGetLUTVersion(getNTV2Params(deviceNumber)->_DeviceID) == 2 )
	{
		return GetLUTV2OutputBank(deviceNumber, channel);
	}

	switch( channel )
	{
	default:
	case NTV2_CHANNEL1:
		return ReadRegister (	deviceNumber,
								kRegCh1ColorCorrectioncontrol,
								kRegMaskCCOutputBankSelect,
								kRegShiftCCOutputBankSelect);
		break;

	case NTV2_CHANNEL2:
		return ReadRegister (	deviceNumber,
								kRegCh2ColorCorrectioncontrol,
								kRegMaskCCOutputBankSelect,
								kRegShiftCCOutputBankSelect);
		break;

	case NTV2_CHANNEL3:
		return ReadRegister (	deviceNumber,
								kRegCh2ColorCorrectioncontrol,
								kRegMaskCC3OutputBankSelect,
								kRegShiftCC3OutputBankSelect);
		break;

	case NTV2_CHANNEL4:
		return ReadRegister (	deviceNumber,
								kRegCh2ColorCorrectioncontrol,
								kRegMaskCC4OutputBankSelect,
								kRegShiftCC4OutputBankSelect);
		break;
	}
}

void
SetColorCorrectionHostAccessBank (ULWord deviceNumber, NTV2ColorCorrectionHostAccessBank value)
{
	if( NTV2DeviceGetLUTVersion(getNTV2Params(deviceNumber)->_DeviceID) == 2 )
	{
		SetLUTV2HostAccessBank( deviceNumber, value );
	}
	else
	{
		switch( value )
		{
		case NTV2_CCHOSTACCESS_CH1BANK0:
		case NTV2_CCHOSTACCESS_CH1BANK1:
		case NTV2_CCHOSTACCESS_CH2BANK0:
		case NTV2_CCHOSTACCESS_CH2BANK1:
			{
				ULWord regValue = NTV2_LUTCONTROL_1_2 << kRegShiftLUTSelect;
				WriteRegister (	deviceNumber,
								kRegCh1ColorCorrectioncontrol,
								regValue,
								kRegMaskLUTSelect,
								kRegMaskLUTSelect);

				regValue = value << kRegShiftCCHostAccessBankSelect;
				WriteRegister (	deviceNumber,
								kRegGlobalControl,
								regValue,
								kRegMaskCCHostBankSelect,
								kRegShiftCCHostAccessBankSelect);
			}
			break;

		default:
			break;

		case NTV2_CCHOSTACCESS_CH3BANK0:
		case NTV2_CCHOSTACCESS_CH3BANK1:
		case NTV2_CCHOSTACCESS_CH4BANK0:
		case NTV2_CCHOSTACCESS_CH4BANK1:
			{
				ULWord regValue = NTV2_LUTCONTROL_3_4 << kRegShiftLUTSelect;
				WriteRegister (	deviceNumber,
								kRegCh1ColorCorrectioncontrol,
								regValue,
								kRegMaskLUTSelect,
								kRegMaskLUTSelect);

				regValue = (value-NTV2_CCHOSTACCESS_CH3BANK0) << kRegShiftCCHostAccessBankSelect;
				WriteRegister (	deviceNumber,
								kRegCh1ColorCorrectioncontrol,
								regValue,
								kRegMaskCCHostBankSelect,
								kRegShiftCCHostAccessBankSelect);
			}
			break;
		}
	}
}

NTV2ColorCorrectionHostAccessBank
GetColorCorrectionHostAccessBank (ULWord deviceNumber, NTV2Channel channel)
{
	if( NTV2DeviceGetLUTVersion(getNTV2Params(deviceNumber)->_DeviceID) == 1 )
	{
		switch(channel)
		{
		default:
		case NTV2_CHANNEL1:
		case NTV2_CHANNEL2:
			return (NTV2ColorCorrectionHostAccessBank) ReadRegister(
						deviceNumber,
						kRegGlobalControl,
						kRegMaskCCHostBankSelect,
						kRegShiftCCHostAccessBankSelect);
			break;
		case NTV2_CHANNEL3:
		case NTV2_CHANNEL4:
			return (NTV2ColorCorrectionHostAccessBank) (ReadRegister(
						deviceNumber,
						kRegCh1ColorCorrectioncontrol,
						kRegMaskCCHostBankSelect,
						kRegShiftCCHostAccessBankSelect) + NTV2_CCHOSTACCESS_CH3BANK0);
			break;
		}
	}
	else
	{
		switch(channel)
		{
		default:
		case NTV2_CHANNEL1:
			return (NTV2ColorCorrectionHostAccessBank) ReadRegister(
						deviceNumber,
						kRegLUTV2Control,
						kRegMaskLUT1HostAccessBankSelect,
						kRegShiftLUT1HostAccessBankSelect);
			break;
		case NTV2_CHANNEL2:
			return (NTV2ColorCorrectionHostAccessBank) ReadRegister(
						deviceNumber,
						kRegLUTV2Control,
						kRegMaskLUT2HostAccessBankSelect,
						kRegShiftLUT2HostAccessBankSelect);
			break;
		case NTV2_CHANNEL3:
			return (NTV2ColorCorrectionHostAccessBank) ReadRegister(
						deviceNumber,
						kRegLUTV2Control,
						kRegMaskLUT3HostAccessBankSelect,
						kRegShiftLUT3HostAccessBankSelect);
			break;
		case NTV2_CHANNEL4:
			return (NTV2ColorCorrectionHostAccessBank) ReadRegister(
						deviceNumber,
						kRegLUTV2Control,
						kRegMaskLUT4HostAccessBankSelect,
						kRegShiftLUT4HostAccessBankSelect);
			break;
		case NTV2_CHANNEL5:
			return (NTV2ColorCorrectionHostAccessBank) ReadRegister(
						deviceNumber,
						kRegLUTV2Control,
						kRegMaskLUT5HostAccessBankSelect,
						kRegShiftLUT5HostAccessBankSelect);
			break;
		case NTV2_CHANNEL6:
			return (NTV2ColorCorrectionHostAccessBank) ReadRegister(
						deviceNumber,
						kRegLUTV2Control,
						kRegMaskLUT6HostAccessBankSelect,
						kRegShiftLUT6HostAccessBankSelect);
			break;
		case NTV2_CHANNEL7:
			return (NTV2ColorCorrectionHostAccessBank) ReadRegister(
						deviceNumber,
						kRegLUTV2Control,
						kRegMaskLUT7HostAccessBankSelect,
						kRegShiftLUT7HostAccessBankSelect);
			break;
		case NTV2_CHANNEL8:
			return (NTV2ColorCorrectionHostAccessBank) ReadRegister(
						deviceNumber,
						kRegLUTV2Control,
						kRegMaskLUT8HostAccessBankSelect,
						kRegShiftLUT8HostAccessBankSelect);
			break;
		}
	}
}

void
SetColorCorrectionSaturation (ULWord deviceNumber, NTV2Channel channel, ULWord value)
{
	WriteRegister (	deviceNumber,
					(channel == NTV2_CHANNEL1) ?
						kRegCh1ColorCorrectioncontrol : kRegCh2ColorCorrectioncontrol,
					value,
					kRegMaskSaturationValue,
					kRegShiftSaturationValue);
}

ULWord
GetColorCorrectionSaturation (ULWord deviceNumber, NTV2Channel channel)
{
	return ReadRegister(deviceNumber,
						(channel == NTV2_CHANNEL1) ?
								kRegCh1ColorCorrectioncontrol : kRegCh2ColorCorrectioncontrol,
						kRegMaskSaturationValue,
						kRegShiftSaturationValue);
}


// Method: SetCustomAncillaryData
// Input:  Custom ancillary data struct
// Output: NONE
void SetCustomAncillaryData (ULWord deviceNumber, NTV2Channel channel, CUSTOM_ANC_STRUCT  *customAncInfo)
{
}

// Method: Init422Uart
// Input:  None
// Output: NONE
// NOTE:   Puts Xena Machine Control UART into known disabled state
void  Init422Uart(ULWord deviceNumber)
{
	// Disable UART TX and RX.
	WriteRegister(deviceNumber, kRegRS422Control,  0, NO_MASK, NO_SHIFT);
	WriteRegister(deviceNumber, kRegRS4222Control, 0, NO_MASK, NO_SHIFT);

	// Clear parity error and overrun
	WriteRegister(deviceNumber, kRegRS422Control,  BIT_6 | BIT_7, NO_MASK, NO_SHIFT);
	WriteRegister(deviceNumber, kRegRS4222Control, BIT_6 | BIT_7, NO_MASK, NO_SHIFT);
}

#ifdef SOFTWARE_UART_FIFO

#ifdef UARTRXFIFOSIZE
// Method: ReadUARTReceiveData
	// Input:  NONE
	// Output: ULWord or equivalent(i.e. ULWord).
ULWord ReadUARTReceiveData(ULWord deviceNumber)
{
	return  READ_REGISTER_ULWord(getNTV2Params(deviceNumber)->_pUARTReceiveData);
}

// Method: ReadUARTReceiveData2
	// Input:  NONE
	// Output: ULWord or equivalent(i.e. ULWord).
ULWord ReadUARTReceiveData2(ULWord deviceNumber)
{
	return  READ_REGISTER_ULWord(getNTV2Params(deviceNumber)->_pUARTReceiveData2);
}
#endif // UARTRXFIFOSIZE

#ifdef UARTTXFIFOSIZE
void WriteUARTTransmitData(ULWord deviceNumber, ULWord value)
{
	WRITE_REGISTER_ULWord(getNTV2Params(deviceNumber)->_pUARTTransmitData,value);
}

void WriteUARTTransmitData2(ULWord deviceNumber, ULWord value)
{
	WRITE_REGISTER_ULWord(getNTV2Params(deviceNumber)->_pUARTTransmitData2,value);
}
#endif // UARTTXFIFOSIZE

// Method: ReadUARTControl
	// Input:  NONE
	// Output: ULWord or equivalent(i.e. ULWord).
ULWord ReadUARTControl(ULWord deviceNumber)
{
	return  READ_REGISTER_ULWord(getNTV2Params(deviceNumber)->_pUARTControl);
}

// Method: ReadUARTControl2
	// Input:  NONE
	// Output: ULWord or equivalent(i.e. ULWord).
ULWord ReadUARTControl2(ULWord deviceNumber)
{
	return  READ_REGISTER_ULWord(getNTV2Params(deviceNumber)->_pUARTControl2);
}

#endif	// SOFTWARE_UART_FIFO

bool IsSaveRecallRegister(ULWord deviceNumber, ULWord regNum)
{
	switch ( regNum )
	{
		case kRegGlobalControl:
		case kRegGlobalControlCh2:
		case kRegGlobalControlCh3:
		case kRegGlobalControlCh4:
		case kRegGlobalControlCh5:
		case kRegGlobalControlCh6:
		case kRegGlobalControlCh7:
		case kRegGlobalControlCh8:
		case kRegCh1Control:
		case kRegCh1PCIAccessFrame:
		case kRegCh1OutputFrame:
		case kRegCh1InputFrame:
		case kRegCh2Control:
		case kRegCh2PCIAccessFrame:
		case kRegCh2OutputFrame:
		case kRegCh2InputFrame:
		case kRegCh3Control:
		case kRegCh3PCIAccessFrame:
		case kRegCh3OutputFrame:
		case kRegCh3InputFrame:
		case kRegCh4Control:
		case kRegCh4PCIAccessFrame:
		case kRegCh4OutputFrame:
		case kRegCh4InputFrame:
		case kRegCh5Control:
		case kRegCh5PCIAccessFrame:
		case kRegCh5OutputFrame:
		case kRegCh5InputFrame:
		case kRegCh6Control:
		case kRegCh6PCIAccessFrame:
		case kRegCh6OutputFrame:
		case kRegCh6InputFrame:
		case kRegCh7Control:
		case kRegCh7PCIAccessFrame:
		case kRegCh7OutputFrame:
		case kRegCh7InputFrame:
		case kRegCh8Control:
		case kRegCh8PCIAccessFrame:
		case kRegCh8OutputFrame:
		case kRegCh8InputFrame:
		case kRegVidProc1Control:
		case kRegVidProcXptControl:
		case kRegMixer1Coefficient:
		case kRegSplitControl:
		case kRegFlatMatteValue:
		case kRegOutputTimingControl:
		case kRegAud1Control:
		case kRegAud1SourceSelect:
		case kRegRP188InOut1DBB:
		case kRegRP188InOut1Bits0_31:
		case kRegRP188InOut1Bits32_63:
		case kRegRP188InOut2DBB:
		case kRegRP188InOut2Bits0_31:
		case kRegRP188InOut2Bits32_63:
		case kRegRP188InOut3DBB:
		case kRegRP188InOut3Bits0_31:
		case kRegRP188InOut3Bits32_63:
		case kRegRP188InOut4DBB:
		case kRegRP188InOut4Bits0_31:
		case kRegRP188InOut4Bits32_63:
		case kRegRP188InOut5DBB:
		case kRegRP188InOut5Bits0_31:
		case kRegRP188InOut5Bits32_63:
		case kRegRP188InOut6DBB:
		case kRegRP188InOut6Bits0_31:
		case kRegRP188InOut6Bits32_63:
		case kRegRP188InOut7DBB:
		case kRegRP188InOut7Bits0_31:
		case kRegRP188InOut7Bits32_63:
		case kRegRP188InOut8DBB:
		case kRegRP188InOut8Bits0_31:
		case kRegRP188InOut8Bits32_63:
		case kRegCh1ColorCorrectioncontrol:
		case kRegCh2ColorCorrectioncontrol:
		case kRegAnalogOutControl:
		case kRegSDIOut1Control:
		case kRegSDIOut2Control:
		case kRegConversionControl:
		case kRegFrameSync1Control:
		case kRegFrameSync2Control:
		case kRegCSCoefficients1_2:
		case kRegCSCoefficients3_4:
		case kRegCSCoefficients5_6:
		case kRegCSCoefficients7_8:
		case kRegCSCoefficients9_10:
		case kRegCS2Coefficients1_2:
		case kRegCS2Coefficients3_4:
		case kRegCS2Coefficients5_6:
		case kRegCS2Coefficients7_8:
		case kRegCS2Coefficients9_10:
		case kRegCS3Coefficients1_2:
		case kRegCS3Coefficients3_4:
		case kRegCS3Coefficients5_6:
		case kRegCS3Coefficients7_8:
		case kRegCS3Coefficients9_10:
		case kRegCS4Coefficients1_2:
		case kRegCS4Coefficients3_4:
		case kRegCS4Coefficients5_6:
		case kRegCS4Coefficients7_8:
		case kRegCS4Coefficients9_10:
		case kRegCS5Coefficients1_2:
		case kRegCS5Coefficients3_4:
		case kRegCS5Coefficients5_6:
		case kRegCS5Coefficients7_8:
		case kRegCS5Coefficients9_10:
		case kRegCS6Coefficients1_2:
		case kRegCS6Coefficients3_4:
		case kRegCS6Coefficients5_6:
		case kRegCS6Coefficients7_8:
		case kRegCS6Coefficients9_10:
		case kRegCS7Coefficients1_2:
		case kRegCS7Coefficients3_4:
		case kRegCS7Coefficients5_6:
		case kRegCS7Coefficients7_8:
		case kRegCS7Coefficients9_10:
		case kRegCS8Coefficients1_2:
		case kRegCS8Coefficients3_4:
		case kRegCS8Coefficients5_6:
		case kRegCS8Coefficients7_8:
		case kRegCS8Coefficients9_10:
		case kRegXptSelectGroup1:
		case kRegXptSelectGroup2:
		case kRegXptSelectGroup3:
		case kRegXptSelectGroup4:
		case kRegXptSelectGroup5:
		case kRegXptSelectGroup6:
		case kRegXptSelectGroup7:
		case kRegXptSelectGroup8:
		case kRegXptSelectGroup9:
		case kRegXptSelectGroup10:
		case kRegXptSelectGroup11:
		case kRegXptSelectGroup12:
		case kRegXptSelectGroup13:
		case kRegXptSelectGroup14:
		case kRegXptSelectGroup15:
		case kRegXptSelectGroup16:
		case kRegXptSelectGroup17:
		case kRegXptSelectGroup18:
		case kRegXptSelectGroup19:
		case kRegXptSelectGroup20:
		case kRegXptSelectGroup21:
		case kRegXptSelectGroup22:
		case kRegXptSelectGroup23:
		case kRegXptSelectGroup24:
		case kRegXptSelectGroup25:
		case kRegXptSelectGroup26:
		case kRegXptSelectGroup27:
		case kRegXptSelectGroup28:
		case kRegXptSelectGroup29:
		case kRegXptSelectGroup30:
		case kRegAudioOutputSourceMap:
			return true;
			break;
		default:
			return false;
			break;
	}
}

void
GetDeviceSerialNumberWords(ULWord deviceNumber, ULWord *low, ULWord *high)
{
  *low = ReadRegister(deviceNumber, kRegReserved54, NO_MASK, NO_SHIFT);
  *high = ReadRegister(deviceNumber, kRegReserved55, NO_MASK, NO_SHIFT);
}

void itoa64(ULWord64 i, char *buffer)
{
   char buf[128], *b = buf;

   if (i < 0)
   {
	   *(buffer++) = '-';
	   i = -i;
   }
   if (i == 0) *(buffer++) = '0';
   else
   {
	   while (i > 0)
	   {
		   char c = (char) do_div(i, 10);	// does i /= 10
		   *(b++) = '0' + c;
	   }
	   while (b > buf) *(buffer++) = *(--b);
   }
   *buffer = 0;
}

//
//  P2P methods
//
ULWord ReadMessageChannel1(ULWord deviceNumber)
{
	NTV2PrivateParams *pNTV2Params;
	pNTV2Params = getNTV2Params(deviceNumber);

	return READ_REGISTER_ULWord(pNTV2Params->_pMessageChannel1);
}

ULWord ReadMessageChannel2(ULWord deviceNumber)
{
	NTV2PrivateParams *pNTV2Params;
	pNTV2Params = getNTV2Params(deviceNumber);

	return READ_REGISTER_ULWord(pNTV2Params->_pMessageChannel2);
}

ULWord ReadMessageChannel3(ULWord deviceNumber)
{
	NTV2PrivateParams *pNTV2Params;
	pNTV2Params = getNTV2Params(deviceNumber);

	return READ_REGISTER_ULWord(pNTV2Params->_pMessageChannel3);
}

ULWord ReadMessageChannel4(ULWord deviceNumber)
{
	NTV2PrivateParams *pNTV2Params;
	pNTV2Params = getNTV2Params(deviceNumber);

	return READ_REGISTER_ULWord(pNTV2Params->_pMessageChannel4);
}

ULWord ReadMessageChannel5(ULWord deviceNumber)
{
	NTV2PrivateParams *pNTV2Params;
	pNTV2Params = getNTV2Params(deviceNumber);

	return READ_REGISTER_ULWord(pNTV2Params->_pMessageChannel5);
}

ULWord ReadMessageChannel6(ULWord deviceNumber)
{
	NTV2PrivateParams *pNTV2Params;
	pNTV2Params = getNTV2Params(deviceNumber);

	return READ_REGISTER_ULWord(pNTV2Params->_pMessageChannel6);
}

ULWord ReadMessageChannel7(ULWord deviceNumber)
{
	NTV2PrivateParams *pNTV2Params;
	pNTV2Params = getNTV2Params(deviceNumber);

	return READ_REGISTER_ULWord(pNTV2Params->_pMessageChannel7);
}

ULWord ReadMessageChannel8(ULWord deviceNumber)
{
	NTV2PrivateParams *pNTV2Params;
	pNTV2Params = getNTV2Params(deviceNumber);

	return READ_REGISTER_ULWord(pNTV2Params->_pMessageChannel8);
}

ULWord ReadMessageInterruptStatus(ULWord deviceNumber)
{
	NTV2PrivateParams *pNTV2Params;
	pNTV2Params = getNTV2Params(deviceNumber);

	return READ_REGISTER_ULWord(pNTV2Params->_pMessageInterruptStatus);
}

ULWord ReadMessageInterruptControl(ULWord deviceNumber)
{
	NTV2PrivateParams *pNTV2Params;
	pNTV2Params = getNTV2Params(deviceNumber);

	return READ_REGISTER_ULWord(pNTV2Params->_pMessageInterruptControl);
}

void EnableMessageChannel1Interrupt(ULWord deviceNumber)
{
	NTV2PrivateParams *pNTV2Params;
	ULWord regValue;
	unsigned long flags;

	pNTV2Params = getNTV2Params(deviceNumber);
	ntv2_spin_lock_irqsave(&pNTV2Params->_p2pInterruptControlRegisterLock, flags);

	regValue = READ_REGISTER_ULWord(pNTV2Params->_pMessageInterruptControl);
	regValue |= kRegMaskMessageInterruptControlEnable1;
	WRITE_REGISTER_ULWord(pNTV2Params->_pMessageInterruptControl, regValue);

	ntv2_spin_unlock_irqrestore(&pNTV2Params->_p2pInterruptControlRegisterLock, flags);
}

void DisableMessageChannel1Interrupt(ULWord deviceNumber)
{
	NTV2PrivateParams *pNTV2Params;
	ULWord regValue;
	unsigned long flags;

	pNTV2Params = getNTV2Params(deviceNumber);
	ntv2_spin_lock_irqsave(&pNTV2Params->_p2pInterruptControlRegisterLock, flags);

	regValue = READ_REGISTER_ULWord(pNTV2Params->_pMessageInterruptControl);
	regValue &= ~kRegMaskMessageInterruptControlEnable1;
	WRITE_REGISTER_ULWord(pNTV2Params->_pMessageInterruptControl, regValue);

	ntv2_spin_unlock_irqrestore(&pNTV2Params->_p2pInterruptControlRegisterLock, flags);
}

void ClearMessageChannel1Interrupt(ULWord deviceNumber)
{
	NTV2PrivateParams *pNTV2Params;
	ULWord regValue;
	unsigned long flags;

	pNTV2Params = getNTV2Params(deviceNumber);
	ntv2_spin_lock_irqsave(&pNTV2Params->_p2pInterruptControlRegisterLock, flags);

	regValue = READ_REGISTER_ULWord(pNTV2Params->_pMessageInterruptControl);
	regValue |= kRegMaskMessageInterruptControlClear1;
	WRITE_REGISTER_ULWord(pNTV2Params->_pMessageInterruptControl, regValue);

	ntv2_spin_unlock_irqrestore(&pNTV2Params->_p2pInterruptControlRegisterLock, flags);
}

void EnableMessageChannel2Interrupt(ULWord deviceNumber)
{
	NTV2PrivateParams *pNTV2Params;
	ULWord regValue;
	unsigned long flags;

	pNTV2Params = getNTV2Params(deviceNumber);
	ntv2_spin_lock_irqsave(&pNTV2Params->_p2pInterruptControlRegisterLock, flags);

	regValue = READ_REGISTER_ULWord(pNTV2Params->_pMessageInterruptControl);
	regValue |= kRegMaskMessageInterruptControlEnable2;
	WRITE_REGISTER_ULWord(pNTV2Params->_pMessageInterruptControl, regValue);

	ntv2_spin_unlock_irqrestore(&pNTV2Params->_p2pInterruptControlRegisterLock, flags);
}

void DisableMessageChannel2Interrupt(ULWord deviceNumber)
{
	NTV2PrivateParams *pNTV2Params;
	ULWord regValue;
	unsigned long flags;

	pNTV2Params = getNTV2Params(deviceNumber);
	ntv2_spin_lock_irqsave(&pNTV2Params->_p2pInterruptControlRegisterLock, flags);

	regValue = READ_REGISTER_ULWord(pNTV2Params->_pMessageInterruptControl);
	regValue &= ~kRegMaskMessageInterruptControlEnable2;
	WRITE_REGISTER_ULWord(pNTV2Params->_pMessageInterruptControl, regValue);

	ntv2_spin_unlock_irqrestore(&pNTV2Params->_p2pInterruptControlRegisterLock, flags);
}

void ClearMessageChannel2Interrupt(ULWord deviceNumber)
{
	NTV2PrivateParams *pNTV2Params;
	ULWord regValue;
	unsigned long flags;

	pNTV2Params = getNTV2Params(deviceNumber);
	ntv2_spin_lock_irqsave(&pNTV2Params->_p2pInterruptControlRegisterLock, flags);

	regValue = READ_REGISTER_ULWord(pNTV2Params->_pMessageInterruptControl);
	regValue |= kRegMaskMessageInterruptControlClear2;
	WRITE_REGISTER_ULWord(pNTV2Params->_pMessageInterruptControl, regValue);

	ntv2_spin_unlock_irqrestore(&pNTV2Params->_p2pInterruptControlRegisterLock, flags);
}

void EnableMessageChannel3Interrupt(ULWord deviceNumber)
{
	NTV2PrivateParams *pNTV2Params;
	ULWord regValue;
	unsigned long flags;

	pNTV2Params = getNTV2Params(deviceNumber);
	ntv2_spin_lock_irqsave(&pNTV2Params->_p2pInterruptControlRegisterLock, flags);

	regValue = READ_REGISTER_ULWord(pNTV2Params->_pMessageInterruptControl);
	regValue |= kRegMaskMessageInterruptControlEnable3;
	WRITE_REGISTER_ULWord(pNTV2Params->_pMessageInterruptControl, regValue);

	ntv2_spin_unlock_irqrestore(&pNTV2Params->_p2pInterruptControlRegisterLock, flags);
}

void DisableMessageChannel3Interrupt(ULWord deviceNumber)
{
	NTV2PrivateParams *pNTV2Params;
	ULWord regValue;
	unsigned long flags;

	pNTV2Params = getNTV2Params(deviceNumber);
	ntv2_spin_lock_irqsave(&pNTV2Params->_p2pInterruptControlRegisterLock, flags);

	regValue = READ_REGISTER_ULWord(pNTV2Params->_pMessageInterruptControl);
	regValue &= ~kRegMaskMessageInterruptControlEnable3;
	WRITE_REGISTER_ULWord(pNTV2Params->_pMessageInterruptControl, regValue);

	ntv2_spin_unlock_irqrestore(&pNTV2Params->_p2pInterruptControlRegisterLock, flags);
}

void ClearMessageChannel3Interrupt(ULWord deviceNumber)
{
	NTV2PrivateParams *pNTV2Params;
	ULWord regValue;
	unsigned long flags;

	pNTV2Params = getNTV2Params(deviceNumber);
	ntv2_spin_lock_irqsave(&pNTV2Params->_p2pInterruptControlRegisterLock, flags);

	regValue = READ_REGISTER_ULWord(pNTV2Params->_pMessageInterruptControl);
	regValue |= kRegMaskMessageInterruptControlClear3;
	WRITE_REGISTER_ULWord(pNTV2Params->_pMessageInterruptControl, regValue);

	ntv2_spin_unlock_irqrestore(&pNTV2Params->_p2pInterruptControlRegisterLock, flags);
}

void EnableMessageChannel4Interrupt(ULWord deviceNumber)
{
	NTV2PrivateParams *pNTV2Params;
	ULWord regValue;
	unsigned long flags;

	pNTV2Params = getNTV2Params(deviceNumber);
	ntv2_spin_lock_irqsave(&pNTV2Params->_p2pInterruptControlRegisterLock, flags);

	regValue = READ_REGISTER_ULWord(pNTV2Params->_pMessageInterruptControl);
	regValue |= kRegMaskMessageInterruptControlEnable4;
	WRITE_REGISTER_ULWord(pNTV2Params->_pMessageInterruptControl, regValue);

	ntv2_spin_unlock_irqrestore(&pNTV2Params->_p2pInterruptControlRegisterLock, flags);
}

void DisableMessageChannel4Interrupt(ULWord deviceNumber)
{
	NTV2PrivateParams *pNTV2Params;
	ULWord regValue;
	unsigned long flags;

	pNTV2Params = getNTV2Params(deviceNumber);
	ntv2_spin_lock_irqsave(&pNTV2Params->_p2pInterruptControlRegisterLock, flags);

	regValue = READ_REGISTER_ULWord(pNTV2Params->_pMessageInterruptControl);
	regValue &= ~kRegMaskMessageInterruptControlEnable4;
	WRITE_REGISTER_ULWord(pNTV2Params->_pMessageInterruptControl, regValue);

	ntv2_spin_unlock_irqrestore(&pNTV2Params->_p2pInterruptControlRegisterLock, flags);
}

void ClearMessageChannel4Interrupt(ULWord deviceNumber)
{
	NTV2PrivateParams *pNTV2Params;
	ULWord regValue;
	unsigned long flags;

	pNTV2Params = getNTV2Params(deviceNumber);
	ntv2_spin_lock_irqsave(&pNTV2Params->_p2pInterruptControlRegisterLock, flags);

	regValue = READ_REGISTER_ULWord(pNTV2Params->_pMessageInterruptControl);
	regValue |= kRegMaskMessageInterruptControlClear4;
	WRITE_REGISTER_ULWord(pNTV2Params->_pMessageInterruptControl, regValue);

	ntv2_spin_unlock_irqrestore(&pNTV2Params->_p2pInterruptControlRegisterLock, flags);
}

void EnableMessageChannel5Interrupt(ULWord deviceNumber)
{
	NTV2PrivateParams *pNTV2Params;
	ULWord regValue;
	unsigned long flags;

	pNTV2Params = getNTV2Params(deviceNumber);
	ntv2_spin_lock_irqsave(&pNTV2Params->_p2pInterruptControlRegisterLock, flags);

	regValue = READ_REGISTER_ULWord(pNTV2Params->_pMessageInterruptControl);
	regValue |= kRegMaskMessageInterruptControlEnable5;
	WRITE_REGISTER_ULWord(pNTV2Params->_pMessageInterruptControl, regValue);

	ntv2_spin_unlock_irqrestore(&pNTV2Params->_p2pInterruptControlRegisterLock, flags);
}

void DisableMessageChannel5Interrupt(ULWord deviceNumber)
{
	NTV2PrivateParams *pNTV2Params;
	ULWord regValue;
	unsigned long flags;

	pNTV2Params = getNTV2Params(deviceNumber);
	ntv2_spin_lock_irqsave(&pNTV2Params->_p2pInterruptControlRegisterLock, flags);

	regValue = READ_REGISTER_ULWord(pNTV2Params->_pMessageInterruptControl);
	regValue &= ~kRegMaskMessageInterruptControlEnable5;
	WRITE_REGISTER_ULWord(pNTV2Params->_pMessageInterruptControl, regValue);

	ntv2_spin_unlock_irqrestore(&pNTV2Params->_p2pInterruptControlRegisterLock, flags);
}

void ClearMessageChannel5Interrupt(ULWord deviceNumber)
{
	NTV2PrivateParams *pNTV2Params;
	ULWord regValue;
	unsigned long flags;

	pNTV2Params = getNTV2Params(deviceNumber);
	ntv2_spin_lock_irqsave(&pNTV2Params->_p2pInterruptControlRegisterLock, flags);

	regValue = READ_REGISTER_ULWord(pNTV2Params->_pMessageInterruptControl);
	regValue |= kRegMaskMessageInterruptControlClear5;
	WRITE_REGISTER_ULWord(pNTV2Params->_pMessageInterruptControl, regValue);

	ntv2_spin_unlock_irqrestore(&pNTV2Params->_p2pInterruptControlRegisterLock, flags);
}

void EnableMessageChannel6Interrupt(ULWord deviceNumber)
{
	NTV2PrivateParams *pNTV2Params;
	ULWord regValue;
	unsigned long flags;

	pNTV2Params = getNTV2Params(deviceNumber);
	ntv2_spin_lock_irqsave(&pNTV2Params->_p2pInterruptControlRegisterLock, flags);

	regValue = READ_REGISTER_ULWord(pNTV2Params->_pMessageInterruptControl);
	regValue |= kRegMaskMessageInterruptControlEnable6;
	WRITE_REGISTER_ULWord(pNTV2Params->_pMessageInterruptControl, regValue);

	ntv2_spin_unlock_irqrestore(&pNTV2Params->_p2pInterruptControlRegisterLock, flags);
}

void DisableMessageChannel6Interrupt(ULWord deviceNumber)
{
	NTV2PrivateParams *pNTV2Params;
	ULWord regValue;
	unsigned long flags;

	pNTV2Params = getNTV2Params(deviceNumber);
	ntv2_spin_lock_irqsave(&pNTV2Params->_p2pInterruptControlRegisterLock, flags);

	regValue = READ_REGISTER_ULWord(pNTV2Params->_pMessageInterruptControl);
	regValue &= ~kRegMaskMessageInterruptControlEnable6;
	WRITE_REGISTER_ULWord(pNTV2Params->_pMessageInterruptControl, regValue);

	ntv2_spin_unlock_irqrestore(&pNTV2Params->_p2pInterruptControlRegisterLock, flags);
}

void ClearMessageChannel6Interrupt(ULWord deviceNumber)
{
	NTV2PrivateParams *pNTV2Params;
	ULWord regValue;
	unsigned long flags;

	pNTV2Params = getNTV2Params(deviceNumber);
	ntv2_spin_lock_irqsave(&pNTV2Params->_p2pInterruptControlRegisterLock, flags);

	regValue = READ_REGISTER_ULWord(pNTV2Params->_pMessageInterruptControl);
	regValue |= kRegMaskMessageInterruptControlClear6;
	WRITE_REGISTER_ULWord(pNTV2Params->_pMessageInterruptControl, regValue);

	ntv2_spin_unlock_irqrestore(&pNTV2Params->_p2pInterruptControlRegisterLock, flags);
}

void EnableMessageChannel7Interrupt(ULWord deviceNumber)
{
	NTV2PrivateParams *pNTV2Params;
	ULWord regValue;
	unsigned long flags;

	pNTV2Params = getNTV2Params(deviceNumber);
	ntv2_spin_lock_irqsave(&pNTV2Params->_p2pInterruptControlRegisterLock, flags);

	regValue = READ_REGISTER_ULWord(pNTV2Params->_pMessageInterruptControl);
	regValue |= kRegMaskMessageInterruptControlEnable7;
	WRITE_REGISTER_ULWord(pNTV2Params->_pMessageInterruptControl, regValue);

	ntv2_spin_unlock_irqrestore(&pNTV2Params->_p2pInterruptControlRegisterLock, flags);
}

void DisableMessageChannel7Interrupt(ULWord deviceNumber)
{
	NTV2PrivateParams *pNTV2Params;
	ULWord regValue;
	unsigned long flags;

	pNTV2Params = getNTV2Params(deviceNumber);
	ntv2_spin_lock_irqsave(&pNTV2Params->_p2pInterruptControlRegisterLock, flags);

	regValue = READ_REGISTER_ULWord(pNTV2Params->_pMessageInterruptControl);
	regValue &= ~kRegMaskMessageInterruptControlEnable7;
	WRITE_REGISTER_ULWord(pNTV2Params->_pMessageInterruptControl, regValue);

	ntv2_spin_unlock_irqrestore(&pNTV2Params->_p2pInterruptControlRegisterLock, flags);
}

void ClearMessageChannel7Interrupt(ULWord deviceNumber)
{
	NTV2PrivateParams *pNTV2Params;
	ULWord regValue;
	unsigned long flags;

	pNTV2Params = getNTV2Params(deviceNumber);
	ntv2_spin_lock_irqsave(&pNTV2Params->_p2pInterruptControlRegisterLock, flags);

	regValue = READ_REGISTER_ULWord(pNTV2Params->_pMessageInterruptControl);
	regValue |= kRegMaskMessageInterruptControlClear7;
	WRITE_REGISTER_ULWord(pNTV2Params->_pMessageInterruptControl, regValue);

	ntv2_spin_unlock_irqrestore(&pNTV2Params->_p2pInterruptControlRegisterLock, flags);
}

void EnableMessageChannel8Interrupt(ULWord deviceNumber)
{
	NTV2PrivateParams *pNTV2Params;
	ULWord regValue;
	unsigned long flags;

	pNTV2Params = getNTV2Params(deviceNumber);
	ntv2_spin_lock_irqsave(&pNTV2Params->_p2pInterruptControlRegisterLock, flags);

	regValue = READ_REGISTER_ULWord(pNTV2Params->_pMessageInterruptControl);
	regValue |= kRegMaskMessageInterruptControlEnable8;
	WRITE_REGISTER_ULWord(pNTV2Params->_pMessageInterruptControl, regValue);

	ntv2_spin_unlock_irqrestore(&pNTV2Params->_p2pInterruptControlRegisterLock, flags);
}

void DisableMessageChannel8Interrupt(ULWord deviceNumber)
{
	NTV2PrivateParams *pNTV2Params;
	ULWord regValue;
	unsigned long flags;

	pNTV2Params = getNTV2Params(deviceNumber);
	ntv2_spin_lock_irqsave(&pNTV2Params->_p2pInterruptControlRegisterLock, flags);

	regValue = READ_REGISTER_ULWord(pNTV2Params->_pMessageInterruptControl);
	regValue &= ~kRegMaskMessageInterruptControlEnable8;
	WRITE_REGISTER_ULWord(pNTV2Params->_pMessageInterruptControl, regValue);

	ntv2_spin_unlock_irqrestore(&pNTV2Params->_p2pInterruptControlRegisterLock, flags);
}

void ClearMessageChannel8Interrupt(ULWord deviceNumber)
{
	NTV2PrivateParams *pNTV2Params;
	ULWord regValue;
	unsigned long flags;

	pNTV2Params = getNTV2Params(deviceNumber);
	ntv2_spin_lock_irqsave(&pNTV2Params->_p2pInterruptControlRegisterLock, flags);

	regValue = READ_REGISTER_ULWord(pNTV2Params->_pMessageInterruptControl);
	regValue |= kRegMaskMessageInterruptControlClear8;
	WRITE_REGISTER_ULWord(pNTV2Params->_pMessageInterruptControl, regValue);

	ntv2_spin_unlock_irqrestore(&pNTV2Params->_p2pInterruptControlRegisterLock, flags);
}

ULWord ReadFrameApertureOffset(ULWord deviceNumber)
{
	NTV2PrivateParams *pNTV2Params;
	pNTV2Params = getNTV2Params(deviceNumber);

	return READ_REGISTER_ULWord(pNTV2Params->_pFrameApertureOffset);
}

void WriteFrameApertureOffset(ULWord deviceNumber, ULWord value)
{
	WRITE_REGISTER_ULWord(getNTV2Params(deviceNumber)->_pFrameApertureOffset, value);
}

void WriteFrameAperture(ULWord deviceNumber, ULWord offset , ULWord value)
{
	NTV2PrivateParams *pNTV2Params;
	pNTV2Params = getNTV2Params(deviceNumber);

	if (pNTV2Params->_FrameApertureBaseAddress && (offset < pNTV2Params->_FrameApertureBaseSize))
	{
		WRITE_REGISTER_ULWord( (unsigned long)(pNTV2Params->_FrameApertureBaseAddress + offset), value );
	}
}

bool DeviceCanDoP2P(ULWord deviceNumber)
{
	NTV2PrivateParams *pNTV2Params;
	pNTV2Params = getNTV2Params(deviceNumber);

	switch(ReadRegister(deviceNumber, kVRegPCIDeviceID, NO_MASK, NO_SHIFT))
	{
		case NTV2_DEVICE_ID_KONA3G_P2P:
		case NTV2_DEVICE_ID_KONA3G_QUAD_P2P:
		case NTV2_DEVICE_ID_KONA4:
		case NTV2_DEVICE_ID_CORVID88:
		case NTV2_DEVICE_ID_CORVID44:
			return true;
		default:
			return false;
	}
}

void
SetLUTEnable(ULWord deviceNumber, NTV2Channel channel, ULWord value)
{
	if(NTV2DeviceGetLUTVersion(getNTV2Params(deviceNumber)->_DeviceID) == 2)
	{
		switch(channel)
		{
		case NTV2_CHANNEL1:
			WriteRegister(	deviceNumber,
							kRegLUTV2Control,
							(value ? 1 : 0),
							kRegMaskLUT1Enable,
							kRegShiftLUT1Enable);
			break;
		case NTV2_CHANNEL2:
			WriteRegister(	deviceNumber,
							kRegLUTV2Control,
							(value ? 1 : 0),
							kRegMaskLUT2Enable,
							kRegShiftLUT2Enable);
			break;
		case NTV2_CHANNEL3:
			WriteRegister(	deviceNumber,
							kRegLUTV2Control,
							(value ? 1 : 0),
							kRegMaskLUT3Enable,
							kRegShiftLUT3Enable);
			break;
		case NTV2_CHANNEL4:
			WriteRegister(	deviceNumber,
							kRegLUTV2Control,
							(value ? 1 : 0),
							kRegMaskLUT4Enable,
							kRegShiftLUT4Enable);
			break;
		case NTV2_CHANNEL5:
			WriteRegister(	deviceNumber,
							kRegLUTV2Control,
							(value ? 1 : 0),
							kRegMaskLUT5Enable,
							kRegShiftLUT5Enable);
			break;
		case NTV2_CHANNEL6:
			WriteRegister(	deviceNumber,
							kRegLUTV2Control,
							(value ? 1 : 0),
							kRegMaskLUT6Enable,
							kRegShiftLUT6Enable);
			break;
		case NTV2_CHANNEL7:
			WriteRegister(	deviceNumber,
							kRegLUTV2Control,
							(value ? 1 : 0),
							kRegMaskLUT7Enable,
							kRegShiftLUT7Enable);
			break;
		case NTV2_CHANNEL8:
			WriteRegister(	deviceNumber,
							kRegLUTV2Control,
							(value ? 1 : 0),
							kRegMaskLUT8Enable,
							kRegShiftLUT8Enable);
			break;
		default:
			return;
		}
	}
}

void
SetLUTV2HostAccessBank(ULWord deviceNumber, NTV2ColorCorrectionHostAccessBank value)
{
	ULWord numLUT = NTV2DeviceGetNumLUTs(getNTV2Params(deviceNumber)->_DeviceID);
	switch(value)
	{
	default:
	case NTV2_CCHOSTACCESS_CH1BANK0:
	case NTV2_CCHOSTACCESS_CH1BANK1:
		if(numLUT > 0)
			WriteRegister(	deviceNumber,
							kRegLUTV2Control,
							value - NTV2_CCHOSTACCESS_CH1BANK0,
							kRegMaskLUT1HostAccessBankSelect,
							kRegShiftLUT1HostAccessBankSelect);
		break;
	case NTV2_CCHOSTACCESS_CH2BANK0:
	case NTV2_CCHOSTACCESS_CH2BANK1:
		if(numLUT > 1)
			WriteRegister(	deviceNumber,
							kRegLUTV2Control,
							value - NTV2_CCHOSTACCESS_CH2BANK0,
							kRegMaskLUT2HostAccessBankSelect,
							kRegShiftLUT2HostAccessBankSelect);
		break;
	case NTV2_CCHOSTACCESS_CH3BANK0:
	case NTV2_CCHOSTACCESS_CH3BANK1:
		if(numLUT > 2)
			WriteRegister(	deviceNumber,
							kRegLUTV2Control,
							value - NTV2_CCHOSTACCESS_CH3BANK0,
							kRegMaskLUT3HostAccessBankSelect,
							kRegShiftLUT3HostAccessBankSelect);
		break;
	case NTV2_CCHOSTACCESS_CH4BANK0:
	case NTV2_CCHOSTACCESS_CH4BANK1:
		if(numLUT > 3)
			WriteRegister(	deviceNumber,
							kRegLUTV2Control,
							value - NTV2_CCHOSTACCESS_CH4BANK0,
							kRegMaskLUT4HostAccessBankSelect,
							kRegShiftLUT4HostAccessBankSelect);
		break;
	case NTV2_CCHOSTACCESS_CH5BANK0:
	case NTV2_CCHOSTACCESS_CH5BANK1:
		if(numLUT > 4)
			WriteRegister(	deviceNumber,
							kRegLUTV2Control,
							value - NTV2_CCHOSTACCESS_CH5BANK0,
							kRegMaskLUT5HostAccessBankSelect,
							kRegShiftLUT5HostAccessBankSelect);
		break;
	case NTV2_CCHOSTACCESS_CH6BANK0:
	case NTV2_CCHOSTACCESS_CH6BANK1:
		if(numLUT > 5)
			WriteRegister(	deviceNumber,
							kRegLUTV2Control,
							value - NTV2_CCHOSTACCESS_CH6BANK0,
							kRegMaskLUT6HostAccessBankSelect,
							kRegShiftLUT6HostAccessBankSelect);
		break;
	case NTV2_CCHOSTACCESS_CH7BANK0:
	case NTV2_CCHOSTACCESS_CH7BANK1:
		if(numLUT > 6)
			WriteRegister(	deviceNumber,
							kRegLUTV2Control,
							value - NTV2_CCHOSTACCESS_CH7BANK0,
							kRegMaskLUT7HostAccessBankSelect,
							kRegShiftLUT7HostAccessBankSelect);
		break;
	case NTV2_CCHOSTACCESS_CH8BANK0:
	case NTV2_CCHOSTACCESS_CH8BANK1:
		if(numLUT > 7)
			WriteRegister(	deviceNumber,
							kRegLUTV2Control,
							value - NTV2_CCHOSTACCESS_CH8BANK0,
							kRegMaskLUT8HostAccessBankSelect,
							kRegShiftLUT8HostAccessBankSelect);
		break;
	}
}

void
SetLUTV2OutputBank(ULWord deviceNumber, NTV2Channel channel, ULWord bank)
{
	ULWord numLUT = NTV2DeviceGetNumLUTs(getNTV2Params(deviceNumber)->_DeviceID);
	switch(channel)
	{
	case NTV2_CHANNEL1:
		if(numLUT > 0)
			WriteRegister(	deviceNumber,
							kRegLUTV2Control,
							bank,
							kRegMaskLUT1OutputBankSelect,
							kRegShiftLUT1OutputBankSelect);
		break;
	case NTV2_CHANNEL2:
		if(numLUT > 1)
			WriteRegister(	deviceNumber,
							kRegLUTV2Control,
							bank,
							kRegMaskLUT2OutputBankSelect,
							kRegShiftLUT2OutputBankSelect);
		break;
	case NTV2_CHANNEL3:
		if(numLUT > 2)
			WriteRegister(	deviceNumber,
							kRegLUTV2Control,
							bank,
							kRegMaskLUT3OutputBankSelect,
							kRegShiftLUT3OutputBankSelect);
		break;
	case NTV2_CHANNEL4:
		if(numLUT > 3)
			WriteRegister(	deviceNumber,
							kRegLUTV2Control,
							bank,
							kRegMaskLUT4OutputBankSelect,
							kRegShiftLUT4OutputBankSelect);
		break;
	case NTV2_CHANNEL5:
		if(numLUT > 4)
			WriteRegister(	deviceNumber,
							kRegLUTV2Control,
							bank,
							kRegMaskLUT5OutputBankSelect,
							kRegShiftLUT5OutputBankSelect);
		break;
	case NTV2_CHANNEL6:
		if(numLUT > 5)
			WriteRegister(	deviceNumber,
							kRegLUTV2Control,
							bank,
							kRegMaskLUT6OutputBankSelect,
							kRegShiftLUT6OutputBankSelect);
		break;
	case NTV2_CHANNEL7:
		if(numLUT > 6)
			WriteRegister(	deviceNumber,
							kRegLUTV2Control,
							bank,
							kRegMaskLUT7OutputBankSelect,
							kRegShiftLUT7OutputBankSelect);
		break;
	case NTV2_CHANNEL8:
		if(numLUT > 7)
			WriteRegister(	deviceNumber,
							kRegLUTV2Control,
							bank,
							kRegMaskLUT8OutputBankSelect,
							kRegShiftLUT8OutputBankSelect);
		break;
	default:
		break;
	}
}

ULWord
GetLUTV2OutputBank(ULWord deviceNumber, NTV2Channel channel)
{
	ULWord numLUT = NTV2DeviceGetNumLUTs(getNTV2Params(deviceNumber)->_DeviceID);
	ULWord bank = 0;

	switch(channel)
	{
	default:
	case NTV2_CHANNEL1:
		if(numLUT > 0)
			return  ReadRegister(deviceNumber, kRegLUTV2Control, kRegMaskLUT1OutputBankSelect, kRegShiftLUT1OutputBankSelect);
		break;
	case NTV2_CHANNEL2:
		if(numLUT > 1)
			return  ReadRegister(deviceNumber, kRegLUTV2Control, kRegMaskLUT2OutputBankSelect, kRegShiftLUT2OutputBankSelect);
		break;
	case NTV2_CHANNEL3:
		if(numLUT > 2)
			return  ReadRegister(deviceNumber, kRegLUTV2Control, kRegMaskLUT3OutputBankSelect, kRegShiftLUT3OutputBankSelect);
		break;
	case NTV2_CHANNEL4:
		if(numLUT > 3)
			return  ReadRegister(deviceNumber, kRegLUTV2Control, kRegMaskLUT4OutputBankSelect, kRegShiftLUT4OutputBankSelect);
		break;
	case NTV2_CHANNEL5:
		if(numLUT > 4)
			return  ReadRegister(deviceNumber, kRegLUTV2Control, kRegMaskLUT5OutputBankSelect, kRegShiftLUT5OutputBankSelect);
		break;
	case NTV2_CHANNEL6:
		if(numLUT > 5)
			return  ReadRegister(deviceNumber, kRegLUTV2Control, kRegMaskLUT6OutputBankSelect, kRegShiftLUT6OutputBankSelect);
		break;
	case NTV2_CHANNEL7:
		if(numLUT > 6)
			return  ReadRegister(deviceNumber, kRegLUTV2Control, kRegMaskLUT7OutputBankSelect, kRegShiftLUT7OutputBankSelect);
		break;
	case NTV2_CHANNEL8:
		if(numLUT > 7)
			return  ReadRegister(deviceNumber, kRegLUTV2Control, kRegMaskLUT8OutputBankSelect, kRegShiftLUT8OutputBankSelect);
		break;
	}

	return bank;
}

ULWord ntv2_getRoundedUpTimeoutJiffies(ULWord timeOutMs)
{
	ULWord roundedUpTimeout;

	// Round up to granularity of HZ
	if (timeOutMs % (1000/HZ) != 0)
	{
		roundedUpTimeout = timeOutMs - timeOutMs % (1000/HZ) + (1000/HZ);
	}
	else
	{
		roundedUpTimeout = timeOutMs;
	}

	return roundedUpTimeout * HZ / 1000;
}

