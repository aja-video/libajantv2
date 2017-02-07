/*
	ntv2linuxdriverinterface.cpp

	Copyright (C) 2003, 2004, 2005, 2006, 2007, 2009, 2011 AJA Video Systems, Inc.
	Proprietary and Confidential information.

	Purpose:

	Platform dependent implementations for NTV2 Driver Interface
	on Linux.

 */

#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <iomanip>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <iostream>

#include "ntv2linuxdriverinterface.h"
#include <ntv2linuxpublicinterface.h>

#include <ntv2devicefeatures.h> 			// For multiple bitfile support for XenaHS
#include <ntv2nubtypes.h>
#include <ntv2utils.h>

using namespace std;

/////////////////////////////////////////////////////////////////////////////////////
// Board Open / Close methods
/////////////////////////////////////////////////////////////////////////////////////

// Method: Open
// Input:  UWord boardNumber(starts at 0)
//         bool displayErrorMessage
//         NTV2DeviceType eBoardType e.g. KSD, KHD, XENA2
// Output: bool - true if opened ok.
bool
CNTV2LinuxDriverInterface::Open(
	  UWord			boardNumber,
	  bool			displayError,
	  NTV2DeviceType	eBoardType,
	  const char 	*hostname	// Non-null: card on remote host
	  )
{
	// Check if already opened
	if (IsOpen())
	{
		// Don't do anything if the requested board is the same as last opened, and
		// the requested or last opened board aren't remote boards
		if ( ( _boardNumber == boardNumber ) &&
				((hostname == NULL) || (hostname[0] == '\0')) &&
				 (_remoteHandle == INVALID_NUB_HANDLE ) )
			return true;

		Close();   // Close current board and open desired board
	}

#if 0
    _boardType = eBoardType;
	if (_boardType == DEVICETYPE_UNKNOWN){
		_boardType = GetCompileFlag ();
	}
#else
    // All the old boards are now depricated. Open only with the ajantv2 driver.
	_boardType = DEVICETYPE_NTV2;
#endif

	_boardNumber = boardNumber;
	_displayErrorMessage = displayError;

	// Got rid of stringstream stuff as it caused heaps of warnings under
	// Redhat 7.2

#define BOARDSTRMAX	32
	char boardStr[BOARDSTRMAX];

	const char *s = NULL;
	switch  ( _boardType )
	{
	#if !defined (NTV2_DEPRECATE)
	case BOARDTYPE_KHD:
		s = "khd";
		break;
	case BOARDTYPE_KSD:
		s = "ksd";
		break;
	case BOARDTYPE_AJAXENA2:
        s = "ajantv2";
		break;
	case BOARDTYPE_FS1:
		s = "fs1";
		break;
	case BOARDTYPE_BORG:
		s = "borg";			// TODO: External product name
		break;
	case BOARDTYPE_HDNTV:	// HDNTV is no longer supported.
		if ( _displayErrorMessage )
		{
			DisplayNTV2Error("Tried to call open with BOARDTYPE_HDNTV");
		}
		return false;

	// TODO: We may want to support KONA2 and KONAX here someday.
	case BOARDTYPE_KONA2:
		if ( _displayErrorMessage )
		{
			DisplayNTV2Error("Tried to call open with BOARDTYPE_KONA2");
		}
		return false;
	case BOARDTYPE_KONAX:
		if ( _displayErrorMessage )
		{
			DisplayNTV2Error("Tried to call open with BOARDTYPE_KONAX");
		}
		return false;
	#endif	//	!defined (NTV2_DEPRECATE)
	case DEVICETYPE_NTV2:
        s = "ajantv2";
		break;
	case DEVICETYPE_UNKNOWN:
		if ( _displayErrorMessage )
		{
			DisplayNTV2Error("Tried to call open with BOARDTYPE_UNKNOWN");
		}
		return false;
	}

	if (hostname && hostname[0] != '\0')	// Non-null: card on remote host
	{
		#if !defined (NTV2_DEPRECATE)
		if(_boardType == BOARDTYPE_FS1 || _boardType == BOARDTYPE_BORG)
		{
			snprintf(boardStr, BOARDSTRMAX - 1, "%s:%s", hostname, s);
		}
		else
		#endif	//	!defined (NTV2_DEPRECATE)
		{
			snprintf(boardStr, BOARDSTRMAX - 1, "%s:%s%d", hostname, s, _boardNumber);
		}

		if ( !OpenRemote(boardNumber, displayError, eBoardType, hostname))
		{
			DisplayNTV2Error("Failed to open board on remote host.");
		}
	}
	else
	{
		#if !defined (NTV2_DEPRECATE)
		if(_boardType == BOARDTYPE_FS1)
		{
			snprintf(boardStr, BOARDSTRMAX - 1, "/dev/fs1");
		}
		else if(_boardType == BOARDTYPE_BORG)
		{
#ifdef BORG
			snprintf(boardStr, BOARDSTRMAX - 1, "/dev/borg");
#else
			// Hack to prevent boardscan from thinking every host is a borg
			// until we have a driver.
			snprintf(boardStr, BOARDSTRMAX - 1, "/dev/%s", s);
#endif
		}
		else
		#endif	//	!defined (NTV2_DEPRECATE)
		{
			snprintf(boardStr, BOARDSTRMAX - 1, "/dev/%s%d", s, _boardNumber);
		}

		_hDevice = open(boardStr,O_RDWR);
	}

	if ( _hDevice == INVALID_HANDLE_VALUE && _remoteHandle == INVALID_NUB_HANDLE)
	{
#define ERRSTRMAX 80
		char buf[ERRSTRMAX];

		snprintf(buf, ERRSTRMAX - 1, "Couldn't open %s\n", boardStr);
		DisplayNTV2Error(buf);
		return false;
	}

	_boardOpened = true;

	// Fail if running with an old driver
	ULWord driverVersionMajor;
	GetDriverVersion(&driverVersionMajor);
	driverVersionMajor = (driverVersionMajor >> 4) & 0xF;
	if (driverVersionMajor < AJA_NTV2_SDK_VERSION_MAJOR)
	{
		printf("Failing open as driver version %d is older than SDK version %d\n",
				driverVersionMajor, AJA_NTV2_SDK_VERSION_MAJOR);
		Close();
		return false;
	}

	ReadRegister(kRegBoardID, reinterpret_cast<ULWord*>(&_boardID));
	//Kludge Warning.....
	//InitMemberVariablesOnOpen needs frame geometry to determine frame buffer
	// size and number....
	// We cannot read the registers unless the xilinx is programmed, so check first
	NTV2FrameGeometry fg =  NTV2_FG_720x486;
	NTV2FrameBufferFormat format = NTV2_FBF_10BIT_YCBCR;

	ULWord programFlashValue;
	if(ReadRegister(kRegFlashProgramReg, &programFlashValue))
	{
		if ((programFlashValue & BIT(9)) == BIT(9))
		{
			ReadRegister (kRegGlobalControl,
						  (ULWord*)&fg,
						  kRegMaskGeometry,
						  kRegShiftGeometry);

			// More of the same Kludge... (from ntv2Register.cpp)
			ULWord returnVal1,returnVal2;
			ReadRegister (kRegCh1Control,&returnVal1,kRegMaskFrameFormat,kRegShiftFrameFormat);
			ReadRegister (kRegCh1Control,&returnVal2,kRegMaskFrameFormatHiBit,kRegShiftFrameFormatHiBit);
			format = (NTV2FrameBufferFormat)((returnVal1&0x0f) | ((returnVal2&0x1)<<4));

		}
		else						// what's the right thing do do here?
		{							// Guess  :-)
			switch(_boardID)
			{
			#if !defined (NTV2_DEPRECATE)
			case BOARD_ID_XENA_SD:
			case BOARD_ID_XENA_SD_MM:
			case BOARD_ID_XENA_SD22:
			case BOARD_ID_KSD22:
			case BOARD_ID_KONA_SD:
			case BOARD_ID_KONALS:
				fg = NTV2_FG_720x486; // SD boards default
				break;
			#endif	//	!defined (NTV2_DEPRECATE)
			default:
				fg = NTV2_FG_1920x1080;	// we usually load the bitfiles for HD, so assume 1080
				break;
			}
		}
	}

	InitMemberVariablesOnOpen(fg, format);

	return true;
}

// Method:	SetOverlappedMode
// Input:	bool mode
bool
CNTV2LinuxDriverInterface::SetOverlappedMode (bool bOverlapped)
{
	// MOP on Linux
	return true;
}

// Method:	SetShareMode
// Input:	bool mode
bool
CNTV2LinuxDriverInterface::SetShareMode (bool bShared)
{
	_bOpenShared = bShared;
	return true;
}

// Method: Close
// Input:  NONE
// Output: NONE
bool
CNTV2LinuxDriverInterface::Close()
{
	if (_remoteHandle != INVALID_NUB_HANDLE)
	{
		return CloseRemote();
	}

	if( !_boardOpened )
		return true;

	assert( _hDevice );

	// oem additions
	UnmapFrameBuffers ();
	DmaUnlock ();
	UnmapXena2Flash();
	UnmapRegisters();
	UnmapDMADriverBuffer();

	close(_hDevice);
	_hDevice = INVALID_HANDLE_VALUE;
	_boardOpened = false;

	return true;
}


///////////////////////////////////////////////////////////////////////////////////
// Read and Write Register methods
///////////////////////////////////////////////////////////////////////////////////


bool
CNTV2LinuxDriverInterface::ReadRegister(
	ULWord registerNumber,
	ULWord *registerValue,
	ULWord registerMask,
	ULWord registerShift)
{

	if (_remoteHandle != INVALID_NUB_HANDLE)
	{
		if (!CNTV2DriverInterface::ReadRegister(
					registerNumber,
					registerValue,
					registerMask,
					registerShift))
		{
			DisplayNTV2Error("NTV2ReadRegisterRemote failed");
			return false;
		}
	}
#ifdef BORG
#ifndef KUMO //TODO: Resolve correct call to use for KUMO
	else if ((_boardType == BOARDTYPE_BORG) && (registerNumber == 54)) // serial number low - see NTV2Status::GetSerialNumberString()
	{
		string serialNumber = SerialNumber_Borg::GetFormattedSerialNumber();
		//string serialNumber = SerialNumber_Borg::FormatSerialNumber( SerialNumber_Borg::GetSerialNumber() );
		char sn[8] = {0,0,0,0,0,0,0,0};
		strncpy(sn,serialNumber.c_str(),8);
		*registerValue = sn[0] | (sn[1]<<8) | (sn[2]<<16) | (sn[3]<<24);
		*registerValue &= registerMask;
		*registerValue >>= registerShift;
	}

	else if ((_boardType == BOARDTYPE_BORG) && (registerNumber == 55)) // serial number high - see NTV2Status::GetSerialNumberString()
	{
		string serialNumber = SerialNumber_Borg::GetFormattedSerialNumber();
		//string serialNumber = SerialNumber_Borg::FormatSerialNumber( SerialNumber_Borg::GetSerialNumber() );
		char sn[8] = {0,0,0,0,0,0,0,0};
		strncpy(sn,serialNumber.c_str(),8);
		*registerValue = sn[4] | (sn[5]<<8) | (sn[6]<<16) | (sn[7]<<24);
		*registerValue &= registerMask;
		*registerValue >>= registerShift;
	}
#endif
#endif

	else
	{
		assert( (_hDevice != INVALID_HANDLE_VALUE) && (_hDevice != 0));
		NTV2_ASSERT (registerShift < 32);

		REGISTER_ACCESS ra;

		ra.RegisterNumber = registerNumber;
		ra.RegisterMask	  = registerMask;
		ra.RegisterShift  = registerShift;
		ra.RegisterValue  = 0xDEADBEEF;

		if (ioctl( _hDevice, IOCTL_NTV2_READ_REGISTER, &ra))
		{
			DisplayNTV2Error("IOCTL_NTV2_READ_REGISTER failed");
			return false;
		}

		*registerValue = ra.RegisterValue;
	}

	return true;
}


bool
CNTV2LinuxDriverInterface::WriteRegister (
	ULWord registerNumber,
	ULWord registerValue,
	ULWord registerMask,
	ULWord registerShift)
{
	if (_remoteHandle != INVALID_NUB_HANDLE)
	{
		if (!CNTV2DriverInterface::WriteRegister(
					registerNumber,
					registerValue,
					registerMask,
					registerShift))
		{
			DisplayNTV2Error("NTV2WriteRegisterRemote failed");
			return false;
		}
	}
	#if !defined (NTV2_DEPRECATE)
	else if (_boardType == BOARDTYPE_FS1 && registerNumber != kRegFlashProgramReg)
	{
		if ( registerNumber < VIRTUALREG_START )
		{
			// Range check.  Disallow nonexistent registers to prevent segfault.
			if (registerNumber > NTV2BoardGetNumberRegisters(BOARD_ID_FS1))
			{
				fprintf(stderr, "WriteRegister failed due to out of bounds regnum: %d\n", registerNumber);
				return false;
			}

			if (registerMask != 0xFFFFFFFF)
			{
				ULWord oldValue = *(_pRegisterBaseAddress + registerNumber);
				oldValue &= ~registerMask;
				registerValue <<= registerShift;
				registerValue |= oldValue;
			}
			*(_pRegisterBaseAddress + registerNumber) = registerValue;
		}
		else
		{
			// Virtual registers not supported yet.
			return false;
		}
	}
	#endif	//	!defined (NTV2_DEPRECATE)
	else
	{
		assert( (_hDevice != INVALID_HANDLE_VALUE) && (_hDevice != 0) );
		NTV2_ASSERT (registerShift < 32);
		REGISTER_ACCESS ra;

		ra.RegisterNumber = registerNumber;
		ra.RegisterValue = registerValue;
		ra.RegisterMask = registerMask;
		ra.RegisterShift = registerShift;

		if (ioctl( _hDevice, IOCTL_NTV2_WRITE_REGISTER, &ra))
		{
			DisplayNTV2Error("IOCTL_NTV2_WRITE_REGISTER failed");
			return false;
		}
	}
	return true;

}

bool
CNTV2LinuxDriverInterface::RestoreHardwareProcampRegisters()
{
	bool result = false;

	assert( (_hDevice != INVALID_HANDLE_VALUE) && (_hDevice != 0) );

	if (ioctl( _hDevice, IOCTL_NTV2_RESTORE_HARDWARE_PROCAMP_REGISTERS))
	{
		DisplayNTV2Error("IOCTL_NTV2_RESTORE_HARDWARE_PROCAMP_REGISTERS failed");
	}
	else
	{
		result = true;
	}

	return result;
}

/////////////////////////////////////////////////////////////////////////////
// Interrupt enabling / disabling method
/////////////////////////////////////////////////////////////////////////////

// Method:	ConfigureInterrupt
// Input:	bool bEnable (turn on/off interrupt), INTERRUPT_ENUMS eInterruptType
// Output:	bool status
// Purpose:	Provides a 1 point connection to driver for interrupt calls
bool
CNTV2LinuxDriverInterface::ConfigureInterrupt (
	bool			bEnable,
	INTERRUPT_ENUMS	eInterruptType)
{
	assert( (_hDevice != INVALID_HANDLE_VALUE) && (_hDevice != 0) );

	NTV2_INTERRUPT_CONTROL_STRUCT intrControlStruct;
	memset(&intrControlStruct, 0, sizeof(NTV2_INTERRUPT_CONTROL_STRUCT));	// Suppress valgrind error
	intrControlStruct.eInterruptType = eInterruptType;
	intrControlStruct.enable = bEnable;

	if (ioctl( _hDevice, IOCTL_NTV2_INTERRUPT_CONTROL, &intrControlStruct))
	{
		DisplayNTV2Error("IOCTL_NTV2_INTERRUPT_CONTROL failed");
		return false;
	}

	return true;
}

// Method: getInterruptCount
// Input:  INTERRUPT_ENUMS	eInterruptType.  Currently only output vertical interrupts are supported.
// Output: ULWord or equivalent(i.e. ULWord).
bool
CNTV2LinuxDriverInterface::GetInterruptCount(
	INTERRUPT_ENUMS	eInterruptType,
	ULWord			*pCount)
{
	assert( (_hDevice != INVALID_HANDLE_VALUE) && (_hDevice != 0) );
	if (     eInterruptType != eVerticalInterrupt
		  && eInterruptType != eInput1
		  && eInterruptType != eInput2
		  && eInterruptType != eAuxVerticalInterrupt
		  )
	{
		DisplayNTV2Error("Unsupported interrupt count request.  Only vertical, input "

	"interrupts are counted.");
		return false;
	}

	NTV2_INTERRUPT_CONTROL_STRUCT intrControlStruct;
	memset(&intrControlStruct, 0, sizeof(NTV2_INTERRUPT_CONTROL_STRUCT));// Suppress valgrind error
	intrControlStruct.eInterruptType = eGetIntCount;
	intrControlStruct.interruptCount = eInterruptType;

	if (ioctl( _hDevice, IOCTL_NTV2_INTERRUPT_CONTROL, &intrControlStruct))
	{
		DisplayNTV2Error("IOCTL_NTV2_INTERRUPT_CONTROL failed");
		return false;
	}

    *pCount = intrControlStruct.interruptCount;
	return true;
}

// Method: WaitForInterrupt
// Output: True on successs, false on failure (ioctl failed or interrupt didn't happen)
bool
CNTV2LinuxDriverInterface::WaitForInterrupt (
	INTERRUPT_ENUMS	eInterrupt, 	// Which interrupt to wait for
	ULWord			timeOutMs		// Num of milliseconds to wait before timing out
	)
{
	if (_remoteHandle != INVALID_NUB_HANDLE)
	{
		return CNTV2DriverInterface::WaitForInterrupt(eInterrupt, timeOutMs);
	}

	assert( (_hDevice != INVALID_HANDLE_VALUE) && (_hDevice != 0) );

	NTV2_WAITFOR_INTERRUPT_STRUCT waitIntrStruct;
	waitIntrStruct.eInterruptType = eInterrupt;
	waitIntrStruct.timeOutMs = timeOutMs;
	waitIntrStruct.success = 0;	// Assume failure

	if (ioctl( _hDevice, IOCTL_NTV2_WAITFOR_INTERRUPT, &waitIntrStruct))
	{
		DisplayNTV2Error("IOCTL_NTV2_WAITFOR_INTERRUPT failed");
		return false;
	}
	BumpEventCount (eInterrupt);

	return waitIntrStruct.success != 0;
}

// Method: ControlDriverDebugMessages
// Output: True on successs, false on failure (ioctl failed or interrupt didn't happen)
bool
CNTV2LinuxDriverInterface::ControlDriverDebugMessages(
	NTV2_DriverDebugMessageSet msgSet,
	bool enable )
{
	assert( (_hDevice != INVALID_HANDLE_VALUE) && (_hDevice != 0) );

	NTV2_CONTROL_DRIVER_DEBUG_MESSAGES_STRUCT cddmStruct;
	cddmStruct.msgSet = msgSet;
	cddmStruct.enable = enable;

	if (ioctl(	_hDevice,
			 	IOCTL_NTV2_CONTROL_DRIVER_DEBUG_MESSAGES,
				&cddmStruct))
	{
		DisplayNTV2Error("IOCTL_NTV2_CONTROL_DRIVER_DEBUG_MESSAGES failed");
		return false;
	}

	return cddmStruct.success != 0;

}

// Method: SetupBoard
// Output: True on successs, false on failure (ioctl failed or interrupt didn't happen)
bool
CNTV2LinuxDriverInterface::SetupBoard()
{
	assert( (_hDevice != INVALID_HANDLE_VALUE) && (_hDevice != 0) );

	if (ioctl(	_hDevice,
			 	IOCTL_NTV2_SETUP_BOARD,
				0,		// Suppress valgrind error
				0		// Suppress valgrind error
				))
	{
		DisplayNTV2Error("IOCTL_NTV2_SETUP_BOARD failed");
		return false;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////
// OEM Mapping to Userspace Methods
//////////////////////////////////////////////////////////////////////////////

// Method:	MapFrameBuffers
// Input:	None
// Output:	bool, and sets member _pBaseFrameAddress
bool
CNTV2LinuxDriverInterface::MapFrameBuffers (void)
{
	assert( (_hDevice != INVALID_HANDLE_VALUE) && (_hDevice != 0) );

	if ( _pFrameBaseAddress == NULL )
	{
		// Get memory window size from driver
		ULWord BA1MemorySize;
		if (!GetBA1MemorySize(&BA1MemorySize))
		{
			DisplayNTV2Error ("MapFrameBuffers failed - couldn't get BA1MemorySize");
			return false;
		}

		if (BA1MemorySize == 0)
		{
			DisplayNTV2Error ("BA1MemorySize is 0 -- module loaded with MapFrameBuffers=0?");
			DisplayNTV2Error ("PIO mode not available, only driverbuffer DMA.");
			return false;
		}

		// If BA1MemorySize is 0, then the module was loaded with MapFrameBuffers=0
		// and PIO mode is not available.

		// Map the memory.  For Xena(da) boards, the window will be the same size as the amount of
		// memory on the Xena card.  For Xena(mm) cards, it will be a window which is selected using
		// SetPCIAccessFrame().
		//
		// the offset of 0 in the call to mmap tells mmap to map BAR1 which is the framebuffers.
		_pFrameBaseAddress = (ULWord *) mmap(NULL,BA1MemorySize,PROT_READ | PROT_WRITE,MAP_SHARED,_hDevice,0);
		if ( _pFrameBaseAddress == MAP_FAILED )
		{
			_pFrameBaseAddress = NULL;
			DisplayNTV2Error ("MapFrameBuffers failed in call to mmap()");
			return false;
		}

		// Set the CH1 and CH2 frame base addresses for cards that require them.
		ULWord boardIDRegister;
		ReadRegister(kRegBoardID, &boardIDRegister);	//unfortunately GetBoardID is in ntv2card...ooops.
		#if !defined (NTV2_DEPRECATE)
		if ( boardIDRegister == BOARD_ID_HDNTV )       // No longer supported
			return false;
		#endif	//	!defined (NTV2_DEPRECATE)
		if ( ! NTV2BoardIsDirectAddressable( (NTV2DeviceID)boardIDRegister ) )
			_pCh1FrameBaseAddress = _pFrameBaseAddress;
	}

	return true;
}

// Method:	UnmapFrameBuffers
// Input:	None
// Output:	bool status
bool
CNTV2LinuxDriverInterface::UnmapFrameBuffers (void)
{
	if (_pFrameBaseAddress == 0)
		return true;

	assert( (_hDevice != INVALID_HANDLE_VALUE) && (_hDevice != 0) );

	// Get memory window size from driver
	ULWord BA1MemorySize;
	if (!GetBA1MemorySize(&BA1MemorySize))
	{
		DisplayNTV2Error ("UnmapFrameBuffers failed - couldn't get BA1MemorySize");
		return false;
	}

	if ( _pFrameBaseAddress != NULL )
		munmap(_pFrameBaseAddress,BA1MemorySize);

	_pFrameBaseAddress = NULL;

	return true;
}

// Method:	MapRegisters
// Input:	None
// Output:	bool, and sets member _pBaseFrameAddress
bool
CNTV2LinuxDriverInterface::MapRegisters (void)
{
	assert( (_hDevice != INVALID_HANDLE_VALUE) && (_hDevice != 0));

	if ( _pRegisterBaseAddress == NULL )
	{
		// Get register window size from driver
		if (!GetBA0MemorySize(&_BA0MemorySize))
		{
			DisplayNTV2Error ("MapRegisters failed - couldn't get BA0MemorySize");
			_pRegisterBaseAddress = NULL;
			return false;
		}

		if (_BA0MemorySize == 0)
		{
			DisplayNTV2Error ("BA0MemorySize is 0, registers not mapped.");
			_pRegisterBaseAddress = NULL;
			return false;
		}

		// the offset of 0x1000 in the call to mmap tells mmap to map BAR0 which is the registers.
		// 2.4 kernel interprets offset as number of pages, so 0x1000 works. This won't work on a 2.2
		// kernel
		_pRegisterBaseAddress = (ULWord *) mmap(NULL,_BA0MemorySize,PROT_READ | PROT_WRITE,MAP_SHARED,_hDevice,0x1000);

		if (  _pRegisterBaseAddress == MAP_FAILED )
		{
			_pRegisterBaseAddress = NULL;
			return false;
		}
	}

	return true;

}


// Method:	UnmapRegisters
// Input:	None
// Output:	bool status
bool
CNTV2LinuxDriverInterface::UnmapRegisters (void)
{
	assert( (_hDevice != INVALID_HANDLE_VALUE) && (_hDevice != 0) );

	if (_pRegisterBaseAddress == 0)
		return true;

	if ( _pRegisterBaseAddress != NULL )
	{
		munmap(_pRegisterBaseAddress,_BA0MemorySize);
	}
	_pRegisterBaseAddress = NULL;

	return true;
}

// Method:	MapXena2Flash
// Input:	None
// Output:	bool status
bool
CNTV2LinuxDriverInterface::MapXena2Flash (void)
{
	assert( (_hDevice != INVALID_HANDLE_VALUE) && (_hDevice != 0) );

	ULWord BA4MemorySize;
	if ( _pXena2FlashBaseAddress == NULL )
	{
		if ( !GetBA4MemorySize(&BA4MemorySize) )
		{
			DisplayNTV2Error ("MapXena2Flash failed - couldn't get BA4MemorySize");
			_pXena2FlashBaseAddress = NULL;
			return false;
		}
		if ( BA4MemorySize == 0 )
		{
			DisplayNTV2Error ("MapXena2Flash failed - BA4MemorySize == 0");
			_pXena2FlashBaseAddress = NULL;
			return false;
		}

		_BA4MemorySize = BA4MemorySize;

		// 0x4000 is a page offset magic token passed into the driver mmap callback that ends up mapping the right stuff
		_pXena2FlashBaseAddress = (ULWord *) mmap(NULL,BA4MemorySize, PROT_READ | PROT_WRITE, MAP_SHARED, _hDevice, 0x4000);

		if (  _pXena2FlashBaseAddress == MAP_FAILED )
		{
			_pXena2FlashBaseAddress = NULL;
			DisplayNTV2Error ("MapXena2Flash(): mmap of BAR4 for PCI Flash failed");
			return false;
		}
	}
	return true;
}

// Method:	UnmapXena2Flash
// Input:	None
// Output:	bool status
bool
CNTV2LinuxDriverInterface::UnmapXena2Flash (void)
{
	if ( _pXena2FlashBaseAddress == 0 )
		return true;

	assert( (_hDevice != INVALID_HANDLE_VALUE) && (_hDevice != 0) );

	if ( _pXena2FlashBaseAddress != NULL )
	{
		munmap(_pXena2FlashBaseAddress, _BA4MemorySize);
		_BA4MemorySize = 0;
	}
	_pXena2FlashBaseAddress = NULL;

	return false;
}


// Method:	MapDNXRegisters
// Input:	None
// Output:	bool status
bool
CNTV2LinuxDriverInterface::MapDNXRegisters (void)
{
	ULWord BA2MemorySize;
	assert( (_hDevice != INVALID_HANDLE_VALUE) && (_hDevice != 0) );

	if ( _pDNXRegisterBaseAddress == NULL )
	{
		if ( !GetBA2MemorySize(&BA2MemorySize) )
		{
			DisplayNTV2Error ("MapDNXRegisters failed - couldn't get BA2MemorySize");
			return false;
		}
		if ( BA2MemorySize == 0 )
		{
			DisplayNTV2Error ("MapDNXRegisters failed - BA2MemorySize == 0");
			return false;
		}
		_BA2MemorySize = BA2MemorySize;

		// 0x8000 is a page offset magic token passed into the driver mmap callback
		// that ends up mapping the right stuff
		_pDNXRegisterBaseAddress =
			(ULWord*) mmap(NULL, BA2MemorySize, PROT_READ | PROT_WRITE, MAP_SHARED, _hDevice, 0x8000);

		if (  _pDNXRegisterBaseAddress == MAP_FAILED )
		{
			_pDNXRegisterBaseAddress = NULL;
			_BA2MemorySize           = 0;
			DisplayNTV2Error ("MapDNXRegisters failed - couldn't map BAR2");
			return false;
		}
	}
	return true;
}


// Method:	UnmapDNXRegisters
// Input:	None
// Output:	bool status
bool
CNTV2LinuxDriverInterface::UnmapDNXRegisters (void)
{
	if (_pDNXRegisterBaseAddress == 0 )
		return true;

	assert( (_hDevice != INVALID_HANDLE_VALUE) && (_hDevice != 0) );

	if ( _pDNXRegisterBaseAddress != NULL )
	{
		munmap(_pDNXRegisterBaseAddress, _BA2MemorySize);
		_BA2MemorySize = 0;
	}
	_pDNXRegisterBaseAddress = NULL;

	return false;
}


//////////////////////////////////////////////////////////////////////////////////////
// DMA
//
// Note: Asynchronous DMA only available with driver-allocated buffers.

bool
CNTV2LinuxDriverInterface::DmaTransfer (
	NTV2DMAEngine	DMAEngine,
	bool			bRead,
	ULWord			frameNumber,
	ULWord 			*pFrameBuffer,
	ULWord			offsetBytes,
	ULWord			bytes,
	bool			bSync)
{
	if (_remoteHandle != INVALID_NUB_HANDLE) {
		if (!CNTV2DriverInterface::DmaTransfer(DMAEngine,
					bRead,
					frameNumber,
					pFrameBuffer,
					offsetBytes,
					bytes, bSync))
		{
			DisplayNTV2Error("DmaTransfer with remote failed");
			return false;
		}
		return true;
	}

	assert( (_hDevice != INVALID_HANDLE_VALUE) && (_hDevice != 0) );

//	fprintf(stderr, "%s: FRM(%d) ENG(%d) NB(%d) DIR(%s)\n", __FUNCTION__, frameNumber, DMAEngine, bytes, (bRead==true?"R":"W"));
	// NOTE: Linux driver assumes driver buffers to be used if
	// pFrameBuffer < numDmaDriverBuffers

    NTV2_DMA_CONTROL_STRUCT dmaControlBuf;

    dmaControlBuf.engine = DMAEngine;
    dmaControlBuf.frameNumber = frameNumber;
	dmaControlBuf.frameBuffer = pFrameBuffer;
	if (bRead)
	{
		dmaControlBuf.frameOffsetSrc = offsetBytes;
		dmaControlBuf.frameOffsetDest = 0;
	}
	else // write
	{
		dmaControlBuf.frameOffsetSrc = 0;
		dmaControlBuf.frameOffsetDest = offsetBytes;
	}
    dmaControlBuf.numBytes = bytes;

	// The following are used only for driver-created buffers.
	// Set them to known values.

	dmaControlBuf.downSample = 0;	// Not applicable to this mode
	dmaControlBuf.linePitch = 1;	// Not applicable to this mode

	ULWord numDmaDriverBuffers;
	GetDMANumDriverBuffers(&numDmaDriverBuffers);

	if ((unsigned long)pFrameBuffer >= numDmaDriverBuffers)
	{
		// Can't poll with usermode allocated buffer
		if (bSync == false)
		{
			return false;
		}
		dmaControlBuf.poll = 0;
	}
	else
		dmaControlBuf.poll = bSync;		// True == app must wait for DMA completion interrupt

	int request;
	const char *errMsg = NULL;
#define ERRMSG(s) #s " failed"

	// Usermode buffer stuff
	if (bRead) // Reading?
	{
		if (offsetBytes == 0) // Frame ( or field 0? )
		{
			request = IOCTL_NTV2_DMA_READ_FRAME;
			errMsg = ERRMSG(IOCTL_NTV2_DMA_READ_FRAME);
	   }
	   else // Field 1
	   {
			request = IOCTL_NTV2_DMA_READ;
			errMsg = ERRMSG(IOCTL_NTV2_DMA_READ);
	   }
	}
	else // Writing
	{
		if (offsetBytes == 0) // Frame ( or field 0? )
		{
			request = IOCTL_NTV2_DMA_WRITE_FRAME;
			errMsg = ERRMSG(IOCTL_NTV2_DMA_WRITE_FRAME);
	   }
	   else // Field 1
	   {
			request = IOCTL_NTV2_DMA_WRITE;
			errMsg = ERRMSG(IOCTL_NTV2_DMA_WRITE);
	   }
	}

	// TODO: Stick the IOCTL code inside the dmaControlBuf and collapse
	// 4 IOCTLs into one.
	if (ioctl( _hDevice, request, &dmaControlBuf))
	{
		DisplayNTV2Error(errMsg);
		return false;
	}

	return true;
}

bool
CNTV2LinuxDriverInterface::DmaTransfer (
	NTV2DMAEngine DMAEngine,
	bool		  bRead,
	ULWord		  frameNumber,
	ULWord 		  *pFrameBuffer,
	ULWord		  offsetBytes,
	ULWord		  bytes,
	ULWord        videoNumSegments,
	ULWord        videoSegmentHostPitch,
	ULWord        videoSegmentCardPitch,
	bool		  bSync = true)
{
	assert( (_hDevice != INVALID_HANDLE_VALUE) && (_hDevice != 0) );

//	fprintf(stderr, "%s: FRM(%d) ENG(%d) NB(%d) DIR(%s)\n", __FUNCTION__, frameNumber, DMAEngine, bytes, (bRead==true?"R":"W"));

	// NOTE: Linux driver assumes driver buffers to be used if
	// pFrameBuffer < numDmaDriverBuffers

    NTV2_DMA_SEGMENT_CONTROL_STRUCT dmaControlBuf;

    dmaControlBuf.engine = DMAEngine;
    dmaControlBuf.frameNumber = frameNumber;
	dmaControlBuf.frameBuffer = pFrameBuffer;
	if (bRead)
	{
		dmaControlBuf.frameOffsetSrc = offsetBytes;
		dmaControlBuf.frameOffsetDest = 0;
	}
	else // write
	{
		dmaControlBuf.frameOffsetSrc = 0;
		dmaControlBuf.frameOffsetDest = offsetBytes;
	}
    dmaControlBuf.numBytes = bytes;
    dmaControlBuf.videoNumSegments = videoNumSegments;
    dmaControlBuf.videoSegmentHostPitch = videoSegmentHostPitch;
    dmaControlBuf.videoSegmentCardPitch = videoSegmentCardPitch;


	ULWord numDmaDriverBuffers;
	GetDMANumDriverBuffers(&numDmaDriverBuffers);

	if ((unsigned long)pFrameBuffer >= numDmaDriverBuffers)
	{
		// Can't poll with usermode allocated buffer
		if (bSync == false)
		{
			return false;
		}
		dmaControlBuf.poll = 0;
	}
	else
		dmaControlBuf.poll = bSync;		// True == app must wait for DMA completion interrupt

	int request;
	const char *errMsg = NULL;
#define ERRMSG(s) #s " failed"

	// Usermode buffer stuff
	if (bRead) // Reading?
	{
		if (offsetBytes == 0) // Frame ( or field 0? )
		{
			request = IOCTL_NTV2_DMA_READ_FRAME_SEGMENT;
			errMsg = ERRMSG(IOCTL_NTV2_DMA_READ_FRAME_SEGMENT);
	   }
	   else // Field 1
	   {
			request = IOCTL_NTV2_DMA_READ_SEGMENT;
			errMsg = ERRMSG(IOCTL_NTV2_DMA_READ_SEGMENT);
	   }
	}
	else // Writing
	{
		if (offsetBytes == 0) // Frame ( or field 0? )
		{
			request = IOCTL_NTV2_DMA_WRITE_FRAME_SEGMENT;
			errMsg = ERRMSG(IOCTL_NTV2_DMA_WRITE_FRAME_SEGMENT);
	   }
	   else // Field 1
	   {
			request = IOCTL_NTV2_DMA_WRITE_SEGMENT;
			errMsg = ERRMSG(IOCTL_NTV2_DMA_WRITE_SEGMENT);
	   }
	}

	// TODO: Stick the IOCTL code inside the dmaControlBuf and collapse
	// 4 IOCTLs into one.
	if (ioctl( _hDevice, request, &dmaControlBuf))
	{
		DisplayNTV2Error(errMsg);
		return false;
	}

	return true;

}


bool
CNTV2LinuxDriverInterface::DmaTransfer (NTV2DMAEngine DMAEngine,
										NTV2Channel DMAChannel,
										bool bTarget,
										ULWord frameNumber,
										ULWord frameOffset,
										ULWord videoSize,
										ULWord videoNumSegments,
										ULWord videoSegmentHostPitch,
										ULWord videoSegmentCardPitch,
										PCHANNEL_P2P_STRUCT pP2PData)
{
	assert( (_hDevice != INVALID_HANDLE_VALUE) && (_hDevice != 0) );

	if( pP2PData == NULL )
	{
		DisplayNTV2Error( "DmaTransfer failed: pP2PData == NULL\n" );
		return false;
	}

	// Information to be sent to the driver
	NTV2_DMA_P2P_CONTROL_STRUCT dmaP2PStruct;
	memset( (void*)&dmaP2PStruct, 0, sizeof(dmaP2PStruct) );

	if( bTarget )
	{
		// reset info to be passed back to the user
		memset( (void*)pP2PData, 0, sizeof(CHANNEL_P2P_STRUCT) );
		pP2PData->p2pSize = sizeof(CHANNEL_P2P_STRUCT);
	}
	else
	{
		// check for valid p2p struct
		if( pP2PData->p2pSize != sizeof(CHANNEL_P2P_STRUCT) )
		{
			DisplayNTV2Error( "DmaTransfer failed: pP2PData->p2pSize != sizeof(CHANNEL_P2P_STRUCT)\n" );
			return false;
		}
	}

	dmaP2PStruct.bRead					= bTarget;
	dmaP2PStruct.dmaEngine				= DMAEngine;
	dmaP2PStruct.dmaChannel				= DMAChannel;
	dmaP2PStruct.ulFrameNumber			= frameNumber;
	dmaP2PStruct.ulFrameOffset			= frameOffset;
	dmaP2PStruct.ulVidNumBytes			= videoSize;
	dmaP2PStruct.ulVidNumSegments		= videoNumSegments;
	dmaP2PStruct.ulVidSegmentHostPitch	= videoSegmentHostPitch;
	dmaP2PStruct.ulVidSegmentCardPitch	= videoSegmentCardPitch;
	dmaP2PStruct.ullVideoBusAddress		= pP2PData->videoBusAddress;
	dmaP2PStruct.ullMessageBusAddress	= pP2PData->messageBusAddress;
	dmaP2PStruct.ulVideoBusSize			= pP2PData->videoBusSize;
	dmaP2PStruct.ulMessageData			= pP2PData->messageData;

	if (ioctl( _hDevice, IOCTL_NTV2_DMA_P2P, &dmaP2PStruct))
	{
		DisplayNTV2Error( " DmaTransfer failed: IOCTL error\n");
		return false;
	}

	// fill in p2p data
	pP2PData->videoBusAddress	= dmaP2PStruct.ullVideoBusAddress;
	pP2PData->messageBusAddress	= dmaP2PStruct.ullMessageBusAddress;
	pP2PData->videoBusSize		= dmaP2PStruct.ulVideoBusSize;
	pP2PData->messageData		= dmaP2PStruct.ulMessageData;

	return true;
}


///////////////////////////////////////////////////////////////////////////
// AutoCirculate
bool
CNTV2LinuxDriverInterface::AutoCirculate (AUTOCIRCULATE_DATA &autoCircData)
{
	if (_remoteHandle != INVALID_NUB_HANDLE)
	{
		if (!CNTV2DriverInterface::AutoCirculate(autoCircData))
		{
			DisplayNTV2Error("NTV2AutoCirculateRemote failed");
			return false;
		}
		return true;
	}
	else
	{
		int result;
		assert( (_hDevice != INVALID_HANDLE_VALUE) && (_hDevice != 0) );

		switch (autoCircData.eCommand)
		{
			case eInitAutoCirc:
			case eStartAutoCirc:
			case eStopAutoCirc:
			case eAbortAutoCirc:
			case ePauseAutoCirc:
			case eFlushAutoCirculate:
			case ePrerollAutoCirculate:
				// Pass the autoCircData structure to the driver.
				// The driver knows the implicit meanings of the
				// members of the structure based on the the
				// command contained within it.
				result = ioctl(	_hDevice,
									IOCTL_NTV2_AUTOCIRCULATE_CONTROL,
									&autoCircData);
				if (result)
				{
					DisplayNTV2Error("IOCTL_NTV2_AUTOCIRCULATE_CONTROL failed");

					return false;
				}
				return true;

			case eGetAutoCirc:
				// Pass the autoCircStatus structure to the driver.
				// It will read the channel spec contained within and
				// fill out the status structure accordingly.
				if (ioctl(	_hDevice,
							IOCTL_NTV2_AUTOCIRCULATE_STATUS,
							(AUTOCIRCULATE_STATUS_STRUCT *)autoCircData.pvVal1))
				{
					DisplayNTV2Error("IOCTL_NTV2_AUTOCIRCULATE_STATUS, failed");
					return false;
				}
				return true;

			case eGetFrameStamp:
			{
				// Pass the frameStamp structure to the driver.
				// It will read the channel spec and frame number
				// contained within and fill out the status structure
				// accordingly.
				AUTOCIRCULATE_FRAME_STAMP_COMBO_STRUCT acFrameStampCombo;
				memset(&acFrameStampCombo, 0, sizeof acFrameStampCombo);
				FRAME_STAMP_STRUCT* pFrameStamp = (FRAME_STAMP_STRUCT *) autoCircData.pvVal1;
				acFrameStampCombo.acFrameStamp = *pFrameStamp;

				if (ioctl(	_hDevice,
							IOCTL_NTV2_AUTOCIRCULATE_FRAMESTAMP,
						    &acFrameStampCombo))
				{
					DisplayNTV2Error("IOCTL_NTV2_AUTOCIRCULATE_FRAMESTAMP failed");
					return false;
				}

				*pFrameStamp = acFrameStampCombo.acFrameStamp;
				return true;
			}
			case eGetFrameStampEx2:
			{
				// Pass the frameStamp structure to the driver.
				// It will read the channel spec and frame number
				// contained within and fill out the status structure
				// accordingly.
				AUTOCIRCULATE_FRAME_STAMP_COMBO_STRUCT acFrameStampCombo;
				memset(&acFrameStampCombo, 0, sizeof acFrameStampCombo);
				FRAME_STAMP_STRUCT* pFrameStamp = (FRAME_STAMP_STRUCT *) autoCircData.pvVal1;
				PAUTOCIRCULATE_TASK_STRUCT pTask = (PAUTOCIRCULATE_TASK_STRUCT)autoCircData.pvVal2;

				acFrameStampCombo.acFrameStamp = *pFrameStamp;
				if (pTask != NULL)
				{
					acFrameStampCombo.acTask = *pTask;
				}
				if (ioctl(	_hDevice,
							IOCTL_NTV2_AUTOCIRCULATE_FRAMESTAMP,
						    &acFrameStampCombo))
				{
					DisplayNTV2Error("IOCTL_NTV2_AUTOCIRCULATE_FRAMESTAMP failed");
					return false;
				}

				*pFrameStamp = acFrameStampCombo.acFrameStamp;
				if (pTask != NULL)
				{
					*pTask = acFrameStampCombo.acTask;
				}
				return true;
			}
			case eTransferAutoCirculate:
			{
				PAUTOCIRCULATE_TRANSFER_STRUCT acTransfer =
				   (PAUTOCIRCULATE_TRANSFER_STRUCT) autoCircData.pvVal1;

				// If doing audio, insure buffer alignment is OK
				if (acTransfer->audioBufferSize)
				{
					if (acTransfer->audioBufferSize % 4)
					{
						DisplayNTV2Error ("TransferAutoCirculate failed - audio buffer size not mod 4");
						return false;
					}

					ULWord numDmaDriverBuffers;
					GetDMANumDriverBuffers(&numDmaDriverBuffers);

					if (	 (unsigned long)acTransfer->audioBuffer >=  numDmaDriverBuffers
						  && (unsigned long)acTransfer->audioBuffer % 4
					   )
					{
						DisplayNTV2Error ("TransferAutoCirculate failed - audio buffer address not mod 4");
						return false;
					}
				}

				// Can't pass multiple pointers in a single ioctl, so combine
				// them into a single structure and include channel spec too.
				AUTOCIRCULATE_TRANSFER_COMBO_STRUCT acXferCombo;
				memset(&acXferCombo, 0, sizeof acXferCombo);
				PAUTOCIRCULATE_TRANSFER_STATUS_STRUCT acStatus =
					(PAUTOCIRCULATE_TRANSFER_STATUS_STRUCT)autoCircData.pvVal2;
				NTV2RoutingTable	*pXena2RoutingTable =
					(NTV2RoutingTable*)autoCircData.pvVal3;

				acXferCombo.channelSpec = autoCircData.channelSpec;

				acXferCombo.acTransfer = *acTransfer;
				acXferCombo.acStatus = *acStatus;
				if (pXena2RoutingTable == NULL)
				{
					memset(&acXferCombo.acXena2RoutingTable, 0, sizeof(acXferCombo.acXena2RoutingTable));
				}
				else
				{
					acXferCombo.acXena2RoutingTable = *pXena2RoutingTable;
				}

				// Do the transfer
				if (ioctl(	_hDevice,
							IOCTL_NTV2_AUTOCIRCULATE_TRANSFER,
							&acXferCombo))
				{
					DisplayNTV2Error("IOCTL_NTV2_AUTOCIRCULATE_TRANSFER failed");
					return false;
				}
				// Copy the results back into the status buffer we were
				// given
				*acStatus = acXferCombo.acStatus;
				return true;
			}
			case eTransferAutoCirculateEx:
			{
				PAUTOCIRCULATE_TRANSFER_STRUCT acTransfer =
				   (PAUTOCIRCULATE_TRANSFER_STRUCT) autoCircData.pvVal1;

				// If doing audio, insure buffer alignment is OK
				if (acTransfer->audioBufferSize)
				{
					if (acTransfer->audioBufferSize % 4)
					{
						DisplayNTV2Error ("TransferAutoCirculate failed - audio buffer size not mod 4");
						return false;
					}

					ULWord numDmaDriverBuffers;
					GetDMANumDriverBuffers(&numDmaDriverBuffers);

					if (	 (unsigned long)acTransfer->audioBuffer >=  numDmaDriverBuffers
						  && (unsigned long)acTransfer->audioBuffer % 4
					   )
					{
						DisplayNTV2Error ("TransferAutoCirculate failed - audio buffer address not mod 4");
						return false;
					}
				}

				// Can't pass multiple pointers in a single ioctl, so combine
				// them into a single structure and include channel spec too.
				AUTOCIRCULATE_TRANSFER_COMBO_STRUCT acXferCombo;
				memset(&acXferCombo, 0, sizeof acXferCombo);
				PAUTOCIRCULATE_TRANSFER_STATUS_STRUCT acStatus =
					(PAUTOCIRCULATE_TRANSFER_STATUS_STRUCT)autoCircData.pvVal2;
				NTV2RoutingTable	*pXena2RoutingTable =
					(NTV2RoutingTable*)autoCircData.pvVal3;

				acXferCombo.channelSpec = autoCircData.channelSpec;

				acXferCombo.acTransfer = *acTransfer;
				acXferCombo.acStatus = *acStatus;
				if (pXena2RoutingTable == NULL)
				{
					memset(&acXferCombo.acXena2RoutingTable, 0, sizeof(acXferCombo.acXena2RoutingTable));
				}
				else
				{
					acXferCombo.acXena2RoutingTable = *pXena2RoutingTable;
				}

				// Do the transfer
				if (ioctl(	_hDevice,
							IOCTL_NTV2_AUTOCIRCULATE_TRANSFER,
							&acXferCombo))
				{
					DisplayNTV2Error("IOCTL_NTV2_AUTOCIRCULATE_TRANSFER failed");
					return false;
				}
				// Copy the results back into the status buffer we were
				// given
				*acStatus = acXferCombo.acStatus;
				return true;
			}
			case eTransferAutoCirculateEx2:
			{
				PAUTOCIRCULATE_TRANSFER_STRUCT acTransfer =
				   (PAUTOCIRCULATE_TRANSFER_STRUCT) autoCircData.pvVal1;

				// If doing audio, insure buffer alignment is OK
				if (acTransfer->audioBufferSize)
				{
					if (acTransfer->audioBufferSize % 4)
					{
						DisplayNTV2Error ("TransferAutoCirculate failed - audio buffer size not mod 4");
						return false;
					}

					ULWord numDmaDriverBuffers;
					GetDMANumDriverBuffers(&numDmaDriverBuffers);

					if (	 (unsigned long)acTransfer->audioBuffer >=  numDmaDriverBuffers
						  && (unsigned long)acTransfer->audioBuffer % 4
					   )
					{
						DisplayNTV2Error ("TransferAutoCirculate failed - audio buffer address not mod 4");
						return false;
					}
				}

				// Can't pass multiple pointers in a single ioctl, so combine
				// them into a single structure and include channel spec too.
				AUTOCIRCULATE_TRANSFER_COMBO_STRUCT acXferCombo;
				memset(&acXferCombo, 0, sizeof acXferCombo);
				PAUTOCIRCULATE_TRANSFER_STATUS_STRUCT acStatus =
					(PAUTOCIRCULATE_TRANSFER_STATUS_STRUCT)autoCircData.pvVal2;
				NTV2RoutingTable	*pXena2RoutingTable =
					(NTV2RoutingTable*)autoCircData.pvVal3;
				PAUTOCIRCULATE_TASK_STRUCT pTask = (PAUTOCIRCULATE_TASK_STRUCT)autoCircData.pvVal4;

				acXferCombo.channelSpec = autoCircData.channelSpec;

				acXferCombo.acTransfer = *acTransfer;
				acXferCombo.acStatus = *acStatus;
				if (pXena2RoutingTable != NULL)
				{
					acXferCombo.acXena2RoutingTable = *pXena2RoutingTable;
				}
				if (pTask != NULL)
				{
					acXferCombo.acTask = *pTask;
				}

				// Do the transfer
				if (ioctl(	_hDevice,
							IOCTL_NTV2_AUTOCIRCULATE_TRANSFER,
							&acXferCombo))
				{
					DisplayNTV2Error("IOCTL_NTV2_AUTOCIRCULATE_TRANSFER failed");
					return false;
				}
				// Copy the results back into the status buffer we were
				// given
				*acStatus = acXferCombo.acStatus;
				return true;
			}
			case eSetCaptureTask:
			{
				AUTOCIRCULATE_FRAME_STAMP_COMBO_STRUCT acFrameStampCombo;
				memset(&acFrameStampCombo, 0, sizeof acFrameStampCombo);
				PAUTOCIRCULATE_TASK_STRUCT pTask = (PAUTOCIRCULATE_TASK_STRUCT)autoCircData.pvVal1;

				acFrameStampCombo.acFrameStamp.channelSpec = autoCircData.channelSpec;
				acFrameStampCombo.acTask = *pTask;
				if (ioctl(	_hDevice,
							IOCTL_NTV2_AUTOCIRCULATE_CAPTURETASK,
						    &acFrameStampCombo))
				{
					DisplayNTV2Error("IOCTL_NTV2_AUTOCIRCULATE_CAPTURETASK failed");
					return false;
				}

				return true;
			}
		default:
			DisplayNTV2Error("Unsupported AC command type in AutoCirculate()\n");
			return false;
		}
	}
}


	bool CNTV2LinuxDriverInterface::NTV2Message (NTV2_HEADER * pInMessage)
	{
		assert( (_hDevice != INVALID_HANDLE_VALUE) && (_hDevice != 0) );

		if( !pInMessage )
			return false;

		if( ioctl( _hDevice,
				   IOCTL_AJANTV2_MESSAGE,
				   pInMessage) )
		{
			DisplayNTV2Error("IOCTL_AJANTV2_MESSAGE failed\n");
			return false;
		}

		return true;
	}


bool CNTV2LinuxDriverInterface::SetRelativeVideoPlaybackDelay(ULWord frameDelay)
{
#if 0
	return WriteRegister (kVRegRelativeVideoPlaybackDelay,
						  frameDelay);
#endif
	return false;
}

bool CNTV2LinuxDriverInterface::GetRelativeVideoPlaybackDelay(ULWord* frameDelay)
{
#if 0
	return ReadRegister (kVRegRelativeVideoPlaybackDelay,
								frameDelay);
#endif
	return false;
}

bool CNTV2LinuxDriverInterface::SetAudioRecordPinDelay(ULWord millisecondDelay)
{
	return false;
}

bool CNTV2LinuxDriverInterface::GetAudioRecordPinDelay(ULWord* millisecondDelay)
{
	return false;
}

bool CNTV2LinuxDriverInterface::GetDriverVersion(ULWord* driverVersion)
{
	return ReadRegister (kVRegLinuxDriverVersion, driverVersion);
}

bool CNTV2LinuxDriverInterface::GetBA0MemorySize(ULWord* memSize)
{
	return ReadRegister (kVRegBA0MemorySize, memSize);
}

bool CNTV2LinuxDriverInterface::GetBA1MemorySize(ULWord* memSize)
{
	return ReadRegister (kVRegBA1MemorySize, memSize);
}

bool CNTV2LinuxDriverInterface::GetBA2MemorySize(ULWord* memSize)
{
	return ReadRegister (kVRegBA2MemorySize, memSize);
}

bool CNTV2LinuxDriverInterface::GetBA4MemorySize(ULWord* memSize)
{
	return ReadRegister (kVRegBA4MemorySize, memSize);
}

bool CNTV2LinuxDriverInterface::GetDMADriverBufferPhysicalAddress(ULWord* physAddr)
{
	return ReadRegister (kVRegDMADriverBufferPhysicalAddress, physAddr);
}

bool CNTV2LinuxDriverInterface::GetDMANumDriverBuffers(ULWord* pNumDmaDriverBuffers)
{
	return ReadRegister (kVRegNumDmaDriverBuffers, pNumDmaDriverBuffers);
}


void CNTV2LinuxDriverInterface::GetDriverVersionString( std::string& driverVersionString)
{
	ULWord versionInfo=0;
	GetDriverVersion(&versionInfo);
// Bit  15  Debug
// Bit  14-8  Beta Flag
// Bits 7-4 Major Version
// Bits 3-0 Minor Version
#define VERSTRMAX 32

	char versionString[VERSTRMAX];
	int soFar;

	soFar = snprintf(	versionString, VERSTRMAX - 1, "%d.%d",
						((versionInfo>>4)&0xF) , (versionInfo&0xF));
	if ( versionInfo & (BIT_14+ BIT_13+ BIT_12+ BIT_11+ BIT_10+ BIT_9+ BIT_8 ))
	{
		// Beta Version
		soFar += snprintf(	versionString + soFar, VERSTRMAX - soFar - 1, " Beta %d",
							((versionInfo>>8)&0x3F));
	}
	if ( versionInfo & 0xFFFF0000 )
	{
		// Build Version
		soFar += snprintf(	versionString + soFar, VERSTRMAX - soFar - 1, " Beta %d",
							((versionInfo>>16)&0xFFFF));
	}
	if ( versionInfo & BIT_15 )
	{
		// Debug Version
		soFar += snprintf(	versionString + soFar, VERSTRMAX - soFar - 1, " Debug");
	}

	driverVersionString = versionString;
}

bool CNTV2LinuxDriverInterface::SetAudioOutputMode(NTV2_GlobalAudioPlaybackMode mode)
{
	return WriteRegister(kVRegGlobalAudioPlaybackMode,mode);
}

bool CNTV2LinuxDriverInterface::GetAudioOutputMode(NTV2_GlobalAudioPlaybackMode* mode)
{
	return ReadRegister(kVRegGlobalAudioPlaybackMode,(ULWord*)mode);
}


bool CNTV2LinuxDriverInterface::DisplayNTV2Error (const char *str)
{
	if ( _displayErrorMessage )
	{
		fprintf(stderr, "CNTV2Card: %s\n", str);
		return true;
	}

	return false;
}

//
// Method: SleepMs
// Input:  Time to sleep in milliseconds
// Output: NONE
// Returns: 0 on success, -1 if interrupted by a signal
// Notes: Millisecond sleep function not available on Linux.  usleep() is obsolete.
//		  This method uses the POSIX.1b-compliant nanosleep() function.
//
//		  On most Linux systems (especially x86 ones), HZ is 100, which results
//		  in nanosleep() being accurate only to within about 10 ms.
//
//		  TODO: try using sched_setscheduler() to fix this.
//
Word
CNTV2LinuxDriverInterface::SleepMs(LWord milliseconds)
{
   timespec req;

   req.tv_sec = milliseconds / 1000;
   req.tv_nsec = 1000000UL *(milliseconds - (1000 * req.tv_sec));
   return nanosleep(&req, NULL); // NULL: don't care about remaining time if interrupted for now
}
// Method: MapDMADriverBuffer(Maps 8 Frames worth of memory from kernel space to user space.
// Input:
// Output:
bool CNTV2LinuxDriverInterface::MapDMADriverBuffer()
{
	if ( _pDMADriverBufferAddress == NULL )
	{
		ULWord numDmaDriverBuffers;
		if (!GetDMANumDriverBuffers(&numDmaDriverBuffers))
		{
			DisplayNTV2Error("CNTV2LinuxDriverInterface::MapDMADriverBuffer(): GetDMANumDriverBuffers() failed");
			return false;
		}

		if (numDmaDriverBuffers == 0)
		{
			DisplayNTV2Error("CNTV2LinuxDriverInterface::MapDMADriverBuffer(): numDmaDriverBuffers == 0");
			return false;
		}

		// the offset of 0x2000 in the call to mmap tells mmap to map the DMA Buffer into user space
		// 2.4 kernel interprets offset as number of pages, so 0x2000 works. This won't work on a 2.2
		// kernel
		_pDMADriverBufferAddress = (ULWord *) mmap(NULL,GetFrameBufferSize()*numDmaDriverBuffers,PROT_READ | PROT_WRITE,MAP_SHARED,_hDevice,0x2000);
		if ( _pDMADriverBufferAddress == MAP_FAILED )
		{
			_pDMADriverBufferAddress = NULL;
			return false;
		}
	}

	return true;

}

bool CNTV2LinuxDriverInterface::GetDMADriverBufferAddress(ULWord** pDMADriverBufferAddress)
{
	if ( _pDMADriverBufferAddress == NULL )
	{
		if ( MapDMADriverBuffer() == false )
			return false;
	}

	*pDMADriverBufferAddress = _pDMADriverBufferAddress;

	return true;
}

// Method: UnmapDMADriverBuffer
// Input:  NONE
// Output: NONE
bool CNTV2LinuxDriverInterface::UnmapDMADriverBuffer()
{

	if ( _pDMADriverBufferAddress != NULL )
	{
		ULWord numDmaDriverBuffers;
		if (!GetDMANumDriverBuffers(&numDmaDriverBuffers))
		{
			DisplayNTV2Error("CNTV2LinuxDriverInterface::UnmapDMADriverBuffer(): GetDMANumDriverBuffers() failed");
			return false;
		}

		if (numDmaDriverBuffers == 0)
		{

			DisplayNTV2Error("CNTV2LinuxDriverInterface::UnmapDMADriverBuffer(): numDmaDriverBuffers == 0");
			return false;
		}
		munmap(_pDMADriverBufferAddress, GetFrameBufferSize() * numDmaDriverBuffers);
	}
	_pDMADriverBufferAddress = NULL;

	return true;

}

///////
// Method: DmaBufferWriteFrameDriverBuffer
// NTV2DMAEngine - DMAEngine
// ULWord frameNumber(0 .. NUM_FRAMEBUFFERS-1)
// ULWord dmaBufferFrame(0 .. numDmaDriverBuffers-1)
// ULWord bytes - number of bytes to dma
// ULWord poll - 0=block 1=return immediately and poll
//              via register 48
// When the board is opened the driver allocates
// a user-definable number of frames for dmaing
// This allows dma's to be done without scatter/gather
// which should help performance.
bool CNTV2LinuxDriverInterface::DmaWriteFrameDriverBuffer(NTV2DMAEngine DMAEngine,
											ULWord frameNumber,
											unsigned long dmaBufferFrame ,
											ULWord bytes,
											ULWord poll)
{
	assert( (_hDevice != INVALID_HANDLE_VALUE) && (_hDevice != 0) );

    NTV2_DMA_CONTROL_STRUCT dmaControlBuf;

    dmaControlBuf.engine = DMAEngine;
    dmaControlBuf.frameNumber = frameNumber;
    dmaControlBuf.frameBuffer = (PULWord)dmaBufferFrame;
    dmaControlBuf.frameOffsetSrc = 0;
    dmaControlBuf.frameOffsetDest = 0;
    dmaControlBuf.numBytes = bytes;
	dmaControlBuf.downSample = 0;
	dmaControlBuf.linePitch = 0;
	dmaControlBuf.poll = poll;

	if (ioctl( _hDevice, IOCTL_NTV2_DMA_WRITE_FRAME, &dmaControlBuf))
	{
		DisplayNTV2Error("IOCTL_NTV2_DMA_WRITE_FRAME failed");
		return false;
	}

	return true;
}

//
///////
// Method: DmaBufferWriteFrameDriverBuffer
// NTV2DMAEngine - DMAEngine
// ULWord frameNumber(0-NUM_FRAMEBUFFERS-1)
// ULWord dmaBufferFrame(0 .. numDmaDriverBuffers-1)
// ULWord bytes - number of bytes to dma
// ULWord poll - 0=block 1=return immediately and poll
//              via register 48
// When the board is opened the driver allocates
// a user-definable number of frames for dmaing
// This allows dma's to be done without scatter/gather
// which should help performance.
bool CNTV2LinuxDriverInterface::DmaWriteFrameDriverBuffer(NTV2DMAEngine DMAEngine,
											ULWord frameNumber,
											unsigned long dmaBufferFrame,
											ULWord offsetSrc,
											ULWord offsetDest,
											ULWord bytes,
											ULWord poll)
{
	assert( (_hDevice != INVALID_HANDLE_VALUE) && (_hDevice != 0) );

    NTV2_DMA_CONTROL_STRUCT dmaControlBuf;

    dmaControlBuf.engine = DMAEngine;
    dmaControlBuf.frameNumber = frameNumber;
	dmaControlBuf.frameBuffer = (PULWord)dmaBufferFrame;
    dmaControlBuf.frameOffsetSrc = offsetSrc;
    dmaControlBuf.frameOffsetDest = offsetDest;
    dmaControlBuf.numBytes = bytes;
	dmaControlBuf.downSample = 0;
	dmaControlBuf.linePitch = 0;
	dmaControlBuf.poll = poll;

	if (ioctl( _hDevice, IOCTL_NTV2_DMA_WRITE_FRAME, &dmaControlBuf))
	{
		DisplayNTV2Error("IOCTL_NTV2_DMA_WRITE_FRAME failed");
		return false;
	}

	return true;

}


// Method: DmaBufferReadFrameDriverBuffer
// NTV2DMAEngine - DMAEngine
// ULWord frameNumber(0-NUM_FRAMEBUFFERS-1)
// ULWord dmaBufferFrame(0 .. numDmaDriverBuffers-1)
// ULWord bytes - number of bytes to dma
// ULWord poll - 0=block 1=return immediately and poll
//              via register 48
// When the board is opened the driver allocates
// a user-definable number of frames for dmaing
// This allows dma's to be done without scatter/gather
// which should help performance.
bool CNTV2LinuxDriverInterface::DmaReadFrameDriverBuffer(NTV2DMAEngine DMAEngine,
										   ULWord frameNumber,
										   unsigned long dmaBufferFrame,
										   ULWord bytes,
										   ULWord downSample,
										   ULWord linePitch,
										   ULWord poll)
{
	assert( (_hDevice != INVALID_HANDLE_VALUE) && (_hDevice != 0) );

    NTV2_DMA_CONTROL_STRUCT dmaControlBuf;

    dmaControlBuf.engine = DMAEngine;
    dmaControlBuf.frameNumber = frameNumber;
	dmaControlBuf.frameBuffer = (PULWord)dmaBufferFrame;
    dmaControlBuf.frameOffsetSrc = 0;
    dmaControlBuf.frameOffsetDest = 0;
    dmaControlBuf.numBytes = bytes;
	dmaControlBuf.downSample = downSample;
	if( linePitch == 0 ) linePitch = 1;
	dmaControlBuf.linePitch = linePitch;
	dmaControlBuf.poll = poll;

	static bool bPrintedDownsampleDeprecatedMsg = false;

	if (downSample && !bPrintedDownsampleDeprecatedMsg)
	{
		fprintf(stderr, "CNTV2LinuxDriverInterface::DmaReadFrameDriverBuffer(): downSample is deprecated.\n");
		bPrintedDownsampleDeprecatedMsg = true;
	}

	if (ioctl( _hDevice, IOCTL_NTV2_DMA_READ_FRAME, &dmaControlBuf))
	{
		DisplayNTV2Error("IOCTL_NTV2_DMA_READ_FRAME failed");
		return false;
	}

	return true;
}

// Method: DmaBufferReadFrameDriverBuffer
// NTV2DMAEngine - DMAEngine
// ULWord frameNumber(0-NUM_FRAMEBUFFERS-1)
// ULWord dmaBufferFrame(0 .. numDmaDriverBuffers-1)
// ULWord bytes - number of bytes to dma
// ULWord poll - 0=block 1=return immediately and poll
//              via register 48
// When the board is opened the driver allocates
// a user-definable number of frames for dmaing
// This allows dma's to be done without scatter/gather
// which should help performance.
bool CNTV2LinuxDriverInterface::DmaReadFrameDriverBuffer(NTV2DMAEngine DMAEngine,
										  	ULWord frameNumber,
											unsigned long dmaBufferFrame,
											ULWord offsetSrc,
											ULWord offsetDest,
											ULWord bytes,
											ULWord downSample,
											ULWord linePitch,
											ULWord poll)
{
	assert( (_hDevice != INVALID_HANDLE_VALUE) && (_hDevice != 0) );

    NTV2_DMA_CONTROL_STRUCT dmaControlBuf;

    dmaControlBuf.engine = DMAEngine;
    dmaControlBuf.frameNumber = frameNumber;
	dmaControlBuf.frameBuffer = (PULWord)dmaBufferFrame;
    dmaControlBuf.frameOffsetSrc = offsetSrc;
    dmaControlBuf.frameOffsetDest = offsetDest;
    dmaControlBuf.numBytes = bytes;
	dmaControlBuf.downSample = downSample;
	if( linePitch == 0 ) linePitch = 1;
	dmaControlBuf.linePitch = linePitch;
	dmaControlBuf.poll = poll;

	static bool bPrintedDownsampleDeprecatedMsg = false;

	if (downSample && !bPrintedDownsampleDeprecatedMsg)
	{
		fprintf(stderr, "CNTV2LinuxDriverInterface::DmaReadFrameDriverBuffer(): downSample is deprecated.\n");
		bPrintedDownsampleDeprecatedMsg = true;
	}

	if (ioctl( _hDevice, IOCTL_NTV2_DMA_READ_FRAME, &dmaControlBuf))
	{
		DisplayNTV2Error("IOCTL_NTV2_DMA_READ_FRAME failed");
		return false;
	}

	return true;
}

bool
CNTV2LinuxDriverInterface::DmaWriteWithOffsets(
	NTV2DMAEngine DMAEngine,
	ULWord frameNumber,
	ULWord * pFrameBuffer,
	ULWord offsetSrc,
	ULWord offsetDest,
    ULWord bytes)
{
	// return DmaTransfer (DMAEngine, false, frameNumber, pFrameBuffer, (ULWord) 0, bytes, bSync);
	assert( (_hDevice != INVALID_HANDLE_VALUE) && (_hDevice != 0) );
	// NOTE: Linux driver assumes driver buffers to be used if
	// pFrameBuffer < numDmaDriverBuffers

    NTV2_DMA_CONTROL_STRUCT dmaControlBuf;

    dmaControlBuf.engine = DMAEngine;
    dmaControlBuf.frameNumber = frameNumber;
	dmaControlBuf.frameBuffer = pFrameBuffer;
    dmaControlBuf.frameOffsetSrc = offsetSrc;
    dmaControlBuf.frameOffsetDest = offsetDest;
    dmaControlBuf.numBytes = bytes;

	// The following are used only for driver-created buffers.
	// Set them to known values.

	dmaControlBuf.downSample = 0;       // Not applicable to this mode
	dmaControlBuf.linePitch = 1;        // Not applicable to this mode
	dmaControlBuf.poll = 0;             // currently can't poll with a usermode allocated dma buffer

	int request;
	const char *errMsg = NULL;
#define ERRMSG(s) #s " failed"

	// Usermode buffer stuff
	if (offsetSrc == 0 && offsetDest == 0) // Frame ( or field 0? )
	{
		request = IOCTL_NTV2_DMA_WRITE_FRAME;
		errMsg = ERRMSG(IOCTL_NTV2_DMA_WRITE_FRAME);
	}
	else // Field 1 or audio
	{
		request = IOCTL_NTV2_DMA_WRITE;
		errMsg = ERRMSG(IOCTL_NTV2_DMA_WRITE);
	}

	if (ioctl( _hDevice, request, &dmaControlBuf))
	{
		DisplayNTV2Error(errMsg);
		return false;
	}

	return true;

}

bool
CNTV2LinuxDriverInterface::DmaReadWithOffsets(
	NTV2DMAEngine DMAEngine,
	ULWord frameNumber,
	ULWord * pFrameBuffer,
	ULWord offsetSrc,
	ULWord offsetDest,
    ULWord bytes)
{
	// return DmaTransfer (DMAEngine, false, frameNumber, pFrameBuffer, (ULWord) 0, bytes, bSync);
	assert( (_hDevice != INVALID_HANDLE_VALUE) && (_hDevice != 0) );

	// NOTE: Linux driver assumes driver buffers to be used if
	// pFrameBuffer < numDmaDriverBuffers

    NTV2_DMA_CONTROL_STRUCT dmaControlBuf;

    dmaControlBuf.engine = DMAEngine;
    dmaControlBuf.frameNumber = frameNumber;
	dmaControlBuf.frameBuffer = pFrameBuffer;
    dmaControlBuf.frameOffsetSrc = offsetSrc;
    dmaControlBuf.frameOffsetDest = offsetDest;
    dmaControlBuf.numBytes = bytes;

	// The following are used only for driver-created buffers.
	// Set them to known values.

	dmaControlBuf.downSample = 0;       // Not applicable to this mode
	dmaControlBuf.linePitch = 1;        // Not applicable to this mode
	dmaControlBuf.poll = 0;             // currently can't poll with a usermode allocated dma buffer

	int request;
	const char *errMsg = NULL;
#define ERRMSG(s) #s " failed"

	// Usermode buffer stuff
	if (offsetSrc == 0 && offsetDest == 0) // Frame ( or field 0? )
	{
		request = IOCTL_NTV2_DMA_READ_FRAME;
		errMsg = ERRMSG(IOCTL_NTV2_DMA_READ_FRAME);
	}
	else // Field 1 or audio
	{
		request = IOCTL_NTV2_DMA_READ;
		errMsg = ERRMSG(IOCTL_NTV2_DMA_READ);
	}

	if (ioctl( _hDevice, request, &dmaControlBuf))
	{
		DisplayNTV2Error(errMsg);
		return false;
	}

	return true;

#undef ERRMSG
}

//
// Management of downloaded Xilinx bitfile
//
//
bool
CNTV2LinuxDriverInterface::DriverGetBitFileInformation(
		BITFILE_INFO_STRUCT &bitFileInfo,
		NTV2BitFileType bitFileType)
{
	if (_remoteHandle != INVALID_NUB_HANDLE)
	{
		if (!CNTV2DriverInterface::DriverGetBitFileInformation(
				bitFileInfo,
				bitFileType))
		{
			DisplayNTV2Error("NTV2DriverGetBitFileInformationRemote failed");
			return false;
		}
	}
	else
	{
		if(NTV2DeviceHasSPIFlash(_boardID))
		{
			if (!CNTV2DriverInterface::DriverGetBitFileInformation(
					bitFileInfo,
					bitFileType))
			{
				DisplayNTV2Error("DriverGetBitFileInformation failed");
				return false;
			}
			return true;
		}
		else
		{
			int request;

			assert( (_hDevice != INVALID_HANDLE_VALUE) && (_hDevice != 0) );
			assert( bitFileType == NTV2_VideoProcBitFile); // Other types not implemented yet

			request = (int)IOCTL_NTV2_GET_BITFILE_INFO;

			if (ioctl( _hDevice, request, &bitFileInfo))
			{
				return false;
			}
		}
	}
	return true;
}

bool
CNTV2LinuxDriverInterface::DriverSetBitFileInformation(
		BITFILE_INFO_STRUCT &bitFileInfo)
{
	int request;
	const char *errMsg = NULL;

#define ERRMSG(s) #s " failed"

	assert( (_hDevice != INVALID_HANDLE_VALUE) && (_hDevice != 0) );

	request = (int)IOCTL_NTV2_SET_BITFILE_INFO;
	errMsg = ERRMSG(IOCTL_NTV2_SET_BITFILE_INFO);

	if (ioctl( _hDevice, request, &bitFileInfo))
	{
		DisplayNTV2Error(errMsg);
		return false;
	}

	return true;
#undef ERRMSG
}

bool
CNTV2LinuxDriverInterface::SwitchBitfile(NTV2DeviceID boardID, NTV2BitfileType bitfile)
{
	// Bitfiles are no longer dynamically loaded
	return true;
}

//
// Code for control panel support
//

bool CNTV2LinuxDriverInterface::ReadRP188Registers( NTV2Channel /*channel-not-used*/, RP188_STRUCT* pRP188Data )
{
	bool bSuccess = false;
	RP188_STRUCT rp188;
	NTV2DeviceID boardID = DEVICE_ID_NOTFOUND;
	RP188SourceSelect source = kRP188SourceEmbeddedLTC;
	ULWord dbbReg, msReg, lsReg;

	ReadRegister(kRegBoardID, (ULWord *)&boardID);
	ReadRegister(kVRegRP188SourceSelect, (ULWord *)&source);
	#if !defined (NTV2_DEPRECATE)
		bool bLTCPort = (source == kRP188SourceLTCPort && boardID != BOARD_ID_MOAB);
	#else	//	defined (NTV2_DEPRECATE)
		bool bLTCPort = (source == kRP188SourceLTCPort);
	#endif	//	defined (NTV2_DEPRECATE)
	// values come from LTC port registers
	if (bLTCPort)
	{
		ULWord ltcPresent;
		ReadRegister (kRegStatus, &ltcPresent, kRegMaskLTCInPresent, kRegShiftLTCInPresent);

		// there is no equivalent DBB for LTC port - we synthesize it here
		rp188.DBB = (ltcPresent) ? 0xFE000000 | NEW_SELECT_RP188_RCVD : 0xFE000000;

		// LTC port registers
		dbbReg = 0; // don't care - does not exist
		msReg = kRegLTCAnalogBits0_31;
		lsReg  = kRegLTCAnalogBits32_63;
	}

	// values come from RP188 registers
	else
	{
		NTV2Channel channel = NTV2_CHANNEL1;
		NTV2InputVideoSelect inputSelect = NTV2_Input1Select;

		switch (boardID)
		{
		#if !defined (NTV2_DEPRECATE)
		case BOARD_ID_KONA2:
		case BOARD_ID_XENA2:
		case BOARD_ID_MOAB:
		#endif	//	!defined (NTV2_DEPRECATE)
		case DEVICE_ID_LHI:
		case DEVICE_ID_IOXT:
		case DEVICE_ID_CORVID22:
		case DEVICE_ID_KONA3G:
		case DEVICE_ID_KONA3GQUAD:
			//			case BOARD_ID_CORVID24:			//	** MrBill **	What about CORVID24?!
			ReadRegister (kVRegInputSelect, (ULWord *)&inputSelect);
			channel = (inputSelect == NTV2_Input1Select) ? NTV2_CHANNEL1 : NTV2_CHANNEL2;
			break;
		#if !defined (NTV2_DEPRECATE)
		case BOARD_ID_KONALS:
		case BOARD_ID_XENALH:
		#endif	//	!defined (NTV2_DEPRECATE)
		case DEVICE_ID_IOEXPRESS:
		case DEVICE_ID_CORVID1:
		default:
			channel = NTV2_CHANNEL1;
			break;
		}

		// rp188 registers
		dbbReg = (channel == NTV2_CHANNEL1 ? kRegRP188InOut1DBB       : kRegRP188InOut2DBB      );
		msReg  = (channel == NTV2_CHANNEL1 ? kRegRP188InOut1Bits0_31  : kRegRP188InOut2Bits0_31 );
		lsReg  = (channel == NTV2_CHANNEL1 ? kRegRP188InOut1Bits32_63 : kRegRP188InOut2Bits32_63);
	}

	// initialize values
	if (!bLTCPort)
		ReadRegister (dbbReg, &rp188.DBB );
	ReadRegister (msReg,  &rp188.Low );
	ReadRegister (lsReg,  &rp188.High);

	// register stability filter
	do
	{
		// struct copy to result
		*pRP188Data = rp188;

		// read again into local struct
		if (!bLTCPort)
			ReadRegister (dbbReg, &rp188.DBB );
		ReadRegister (msReg,  &rp188.Low );
		ReadRegister (lsReg,  &rp188.High);

		// if the new read equals the previous read, consider it done
		if ( (rp188.DBB  == pRP188Data->DBB) &&
			(rp188.Low  == pRP188Data->Low) &&
			(rp188.High == pRP188Data->High) )
		{
			bSuccess = true;
		}

	} while (bSuccess == false);

	return true;
}

//--------------------------------------------------------------------------------------------------------------------
//	Get/Set Output Timecode settings
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2LinuxDriverInterface::SetOutputTimecodeOffset( ULWord frames )
{
	return WriteRegister(kVRegOutputTimecodeOffset, frames);
}

bool CNTV2LinuxDriverInterface::GetOutputTimecodeOffset( ULWord* pFrames )
{
	return ReadRegister(kVRegOutputTimecodeOffset, pFrames);
}

bool CNTV2LinuxDriverInterface::SetOutputTimecodeType( ULWord type )
{
	return WriteRegister( kVRegOutputTimecodeType, type );
}

bool CNTV2LinuxDriverInterface::GetOutputTimecodeType( ULWord* pType )
{
	return ReadRegister(kVRegOutputTimecodeType, pType);
}

//--------------------------------------------------------------------------------------------------------------------
//	LockFormat
//
//	For Kona this is currently a no-op
//	For IoHD this will for bitfile swaps / Isoch channel rebuilds based on vidoe mode / video format
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2LinuxDriverInterface::LockFormat( void )
{
	//stub
	return false;
}

//--------------------------------------------------------------------------------------------------------------------
//	StartDriver
//
//	For IoHD this is currently a no-op
//	For Kona this starts the driver after all of the bitFiles have been sent to the driver.
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2LinuxDriverInterface::StartDriver( DriverStartPhase phase )
{
	//stub
	return false;
}

bool CNTV2LinuxDriverInterface::GetQuicktimeUsingBoard( ULWord* qtApp )
{
	return ReadRegister(kVRegQuicktimeUsingBoard, qtApp);
}

bool CNTV2LinuxDriverInterface::SetDefaultVideoOutMode ( uint32_t mode,
													   uint32_t channel,
													   uint32_t frameNum,
													   uint32_t initializing )
{
	return WriteRegister(kVRegDefaultVideoOutMode, mode);
}

bool CNTV2LinuxDriverInterface::GetDefaultVideoOutMode( ULWord* pMode )
{
	return ReadRegister(kVRegDefaultVideoOutMode, pMode);
}

//--------------------------------------------------------------------------------------------------------------------
//	Get/Set Debug Levels
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2LinuxDriverInterface::SetUserModeDebugLevel( ULWord level )
{
	//stub
	return false;
}

bool CNTV2LinuxDriverInterface::GetUserModeDebugLevel( ULWord* level )
{
	//stub
	return false;
}

bool CNTV2LinuxDriverInterface::SetKernelModeDebugLevel( ULWord level )
{
	//stub
	return false;
}

bool CNTV2LinuxDriverInterface::GetKernelModeDebugLevel( ULWord* level )
{
	//stub
	return false;
}

//--------------------------------------------------------------------------------------------------------------------
//	Get/Set Ping Levels
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2LinuxDriverInterface::SetUserModePingLevel( ULWord level )
{
	//stub
	return false;
}

bool CNTV2LinuxDriverInterface::GetUserModePingLevel( ULWord* level )
{
	//stub
	return false;
}

bool CNTV2LinuxDriverInterface::SetKernelModePingLevel( ULWord level )
{
	//stub
	return false;
}

bool CNTV2LinuxDriverInterface::GetKernelModePingLevel( ULWord* level )
{
	//stub
	return false;
}

//--------------------------------------------------------------------------------------------------------------------
//	Get/Set Latency Timer
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2LinuxDriverInterface::SetLatencyTimerValue( ULWord value )
{
	//stub
	return false;
}

bool CNTV2LinuxDriverInterface::GetLatencyTimerValue( ULWord* value )
{
	//stub
	return false;
}

//--------------------------------------------------------------------------------------------------------------------
//	Get/Set include/exclude debug filter strings
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2LinuxDriverInterface::SetDebugFilterStrings( const char* includeString,const char* excludeString )
{
	//stub
	return false;
}

//--------------------------------------------------------------------------------------------------------------------
//	GetDebugFilterStrings
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2LinuxDriverInterface::GetDebugFilterStrings( char* includeString,char* excludeString )
{
	//stub
	return false;
}

bool CNTV2LinuxDriverInterface::ProgramXilinx( void* dataPtr, uint32_t dataSize )
{
	//stub
	return false;
}

bool CNTV2LinuxDriverInterface::LoadBitFile( void* dataPtr, uint32_t dataSize, NTV2BitfileType bitFileType )
{
	//stub
	return false;
}

//--------------------------------------------------------------------------------------------------------------------
//	Application acquire and release stuff
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2LinuxDriverInterface::AcquireStreamForApplicationWithReference( ULWord appCode, int32_t pid )
{
	ULWord currentCode = 0;
	ULWord currentPID  = 0;

	if(!ReadRegister(kVRegApplicationCode, &currentCode))
		return false;
	if(!ReadRegister(kVRegApplicationPID, &currentPID))
		return false;

	// Check if owner is deceased
	struct stat pidStatus;
	char pidName[16];

	snprintf( pidName, 16, "/proc/%d", currentPID );
	if( stat( pidName, &pidStatus ) == -1 && errno == ENOENT )
	{
		// Process doesn't exist, so make the board our own
		ReleaseStreamForApplication( currentCode, currentPID );
	}

	if(!ReadRegister(kVRegApplicationCode, &currentCode))
		return false;
	if(!ReadRegister(kVRegApplicationPID, &currentPID))
		return false;

	for( int count = 0; count < 20; count++ )
	{
		if( currentPID == 0 )
		{
			// Nothing has the board
			if( !WriteRegister(kVRegApplicationCode, appCode) )
			{
				return false;
			}
			else
			{
				// Just in case this is not zero
				WriteRegister(kVRegAcquireLinuxReferenceCount, 0);
				WriteRegister(kVRegAcquireLinuxReferenceCount, 1);
				return WriteRegister(kVRegApplicationPID, (ULWord) pid);
			}
		}
		else if( currentCode == appCode && currentPID == (ULWord) pid )
		{
			// This process has already acquired, so bump the count
			return WriteRegister(kVRegAcquireLinuxReferenceCount, 1);
		}
		else
		{
			// Someone else has the board, so wait and try again
			SleepMs(50);
		}
	}

	return false;
}

bool CNTV2LinuxDriverInterface::ReleaseStreamForApplicationWithReference( ULWord appCode, int32_t pid)
{
	ULWord currentCode = 0;
	ULWord currentPID = 0;
	ULWord currentCount = 0;

	if(!ReadRegister(kVRegApplicationCode, &currentCode))
		return false;
	if(!ReadRegister(kVRegApplicationPID, &currentPID))
		return false;
	if(!ReadRegister(kVRegAcquireLinuxReferenceCount, &currentCount))
		return false;

	if( currentCode == appCode && currentPID == (ULWord) pid )
	{
		if( currentCount > 1 )
		{
			return WriteRegister(kVRegReleaseLinuxReferenceCount, 1);
		}
		else if( currentCount == 1 )
		{
			return ReleaseStreamForApplication( appCode, pid );
		}
		else
		{
			return true;
		}
	}
	else
		return false;
}

bool CNTV2LinuxDriverInterface::AcquireStreamForApplication( ULWord appCode, int32_t pid )
{
	// Loop for a while trying to acquire the board
	for( int count = 0; count < 20; count++ )
	{
		if(!WriteRegister(kVRegApplicationCode, appCode))
			SleepMs(50);
		else
			return WriteRegister(kVRegApplicationPID, (ULWord)pid);
	}

	// Get data about current owner
	ULWord currentCode = 0;
	ULWord currentPID  = 0;
	if(!ReadRegister(kVRegApplicationCode, &currentCode))
		return false;
	if(!ReadRegister(kVRegApplicationPID, &currentPID))
		return false;

	// Check if owner is deceased
	struct stat pidStatus;
	char pidName[16];

	snprintf( pidName, 16, "/proc/%d", currentPID );
	if( stat( pidName, &pidStatus ) == -1 && errno == ENOENT )
	{
		// Process doesn't exist, so make the board our own
		ReleaseStreamForApplication( currentCode, currentPID );
		for( int count = 0; count < 20; count++ )
		{
			if( !WriteRegister( kVRegApplicationCode, appCode ) )
				SleepMs(50);
			else
				return WriteRegister( kVRegApplicationPID, (ULWord)pid );
		}
	}

	// Currect owner is alive, so don't interfere
	return false;
}

bool CNTV2LinuxDriverInterface::ReleaseStreamForApplication( ULWord appCode, int32_t pid )
{
	if(WriteRegister(kVRegReleaseApplication, (ULWord)pid))
	{
		WriteRegister(kVRegAcquireLinuxReferenceCount, 0);
		return true;	// We don't care if the above call failed
	}

	return false;
}

bool CNTV2LinuxDriverInterface::SetStreamingApplication( ULWord appCode, int32_t pid )
{
	if(!WriteRegister(kVRegForceApplicationCode, appCode))
		return false;
	else
		return WriteRegister(kVRegForceApplicationPID, (ULWord)pid);
}

bool CNTV2LinuxDriverInterface::GetStreamingApplication( ULWord *appCode, int32_t  *pid )
{
	if(!ReadRegister(kVRegApplicationCode, appCode))
		return false;
	else
		 return ReadRegister(kVRegApplicationPID, (ULWord*)pid);
}

bool CNTV2LinuxDriverInterface::SetDefaultDeviceForPID( int32_t pid )
{
	//stub
	return false;
}

bool CNTV2LinuxDriverInterface::IsDefaultDeviceForPID( int32_t pid )
{
	//stub
	return false;
}


//	Timestamp virtual register lookup tables
static const ULWord	gChannelToTSLastOutputVertLo []		= {	kVRegTimeStampLastOutputVerticalLo, kVRegTimeStampLastOutput2VerticalLo, kVRegTimeStampLastOutput3VerticalLo, kVRegTimeStampLastOutput4VerticalLo,
															kVRegTimeStampLastOutput5VerticalLo, kVRegTimeStampLastOutput6VerticalLo, kVRegTimeStampLastOutput7VerticalLo, kVRegTimeStampLastOutput8VerticalLo, 0};

static const ULWord	gChannelToTSLastOutputVertHi []		= {	kVRegTimeStampLastOutputVerticalHi, kVRegTimeStampLastOutput2VerticalHi, kVRegTimeStampLastOutput3VerticalHi, kVRegTimeStampLastOutput4VerticalHi,
															kVRegTimeStampLastOutput5VerticalHi, kVRegTimeStampLastOutput6VerticalHi, kVRegTimeStampLastOutput7VerticalHi, kVRegTimeStampLastOutput8VerticalHi, 0};

static const ULWord	gChannelToTSLastInputVertLo []		= {	kVRegTimeStampLastInput1VerticalLo, kVRegTimeStampLastInput2VerticalLo, kVRegTimeStampLastInput3VerticalLo, kVRegTimeStampLastInput4VerticalLo,
															kVRegTimeStampLastInput5VerticalLo, kVRegTimeStampLastInput6VerticalLo, kVRegTimeStampLastInput7VerticalLo, kVRegTimeStampLastInput8VerticalLo, 0};

static const ULWord	gChannelToTSLastInputVertHi []		= {	kVRegTimeStampLastInput1VerticalHi, kVRegTimeStampLastInput2VerticalHi, kVRegTimeStampLastInput3VerticalHi, kVRegTimeStampLastInput4VerticalHi,
															kVRegTimeStampLastInput5VerticalHi, kVRegTimeStampLastInput6VerticalHi, kVRegTimeStampLastInput7VerticalHi, kVRegTimeStampLastInput8VerticalHi, 0};


bool CNTV2LinuxDriverInterface::GetLastOutputVerticalTimestamp (NTV2Channel channel, uint64_t  *pTimeStamp )
{
	uint32_t highWord;
	uint32_t lowWord;

	if(!pTimeStamp)
		return false;

	if(!ReadRegister(gChannelToTSLastOutputVertHi[channel], &highWord))
		return false;

	if(!ReadRegister(gChannelToTSLastOutputVertLo[channel], &lowWord))
		return false;

	*pTimeStamp = (uint64_t)highWord | lowWord;

	return true;
}

bool CNTV2LinuxDriverInterface::GetLastInputVerticalTimestamp (NTV2Channel channel, uint64_t  *pTimeStamp )
{
	uint32_t highWord;
	uint32_t lowWord;

	if(!pTimeStamp)
		return false;

	if(!ReadRegister(gChannelToTSLastInputVertHi[channel], &highWord))
		return false;

	if(!ReadRegister(gChannelToTSLastInputVertLo[channel], &lowWord))
		return false;

	*pTimeStamp = (uint64_t)highWord | lowWord;

	return true;
}

bool CNTV2LinuxDriverInterface::HevcSendMessage(HevcMessageHeader* pMessage)
{
	int request;

	assert( (_hDevice != INVALID_HANDLE_VALUE) && (_hDevice != 0) );

	request = (int)IOCTL_HEVC_MESSAGE;

	if (ioctl( _hDevice, request, pMessage))
	{
		return false;
	}

	return true;
}

