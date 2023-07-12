/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2macdriverinterface.h
	@brief		Declares the MacOS-specific flavor of CNTV2DriverInterface.
	@copyright	(C) 2007-2022 AJA Video Systems, Inc.
**/

#ifndef NTV2MACDRIVERINTERFACE_H
#define NTV2MACDRIVERINTERFACE_H


#include "ntv2publicinterface.h"
#include "ntv2macpublicinterface.h"
#include "ntv2driverinterface.h"

//#define USE_DEVICE_MAP			//	Define to use one io_connect_t for all CNTV2Card instances (per device)
//#define OPEN_UNSUPPORTED_DEVICES	//	Define to open device, even if it's not support (see NTV2GetSupportedDevices)


/**
	@brief	A Mac-specific implementation of CNTV2DriverInterface.
**/
class CNTV2MacDriverInterface : public CNTV2DriverInterface
{
	/**
		@name	Construction, destruction, assignment
	**/
	///@{
	public:
						CNTV2MacDriverInterface();	///< @brief	My default constructor.
	AJA_VIRTUAL			~CNTV2MacDriverInterface();	///< @brief	My destructor.
	///@}

	/**
		@name	Overloaded Methods
	**/
	///@{
	AJA_VIRTUAL bool	WriteRegister (const ULWord inRegNum,  const ULWord inValue,  const ULWord inMask = 0xFFFFFFFF,  const ULWord inShift = 0);
	AJA_VIRTUAL bool	ReadRegister (const ULWord inRegNum,  ULWord & outValue,  const ULWord inMask = 0xFFFFFFFF,  const ULWord inShift = 0);

	AJA_VIRTUAL bool	AcquireStreamForApplication (ULWord inApplicationType, int32_t inProcessID);
	AJA_VIRTUAL bool	ReleaseStreamForApplication (ULWord inApplicationType, int32_t inProcessID);
	AJA_VIRTUAL bool	AcquireStreamForApplicationWithReference (ULWord inApplicationType, int32_t inProcessID);
	AJA_VIRTUAL bool	ReleaseStreamForApplicationWithReference (ULWord inApplicationType, int32_t inProcessID);
	AJA_VIRTUAL bool	SetStreamingApplication (const ULWord appType, const int32_t pid);
	AJA_VIRTUAL bool	GetStreamingApplication (ULWord & outAppType, int32_t & outProcessID);

	AJA_VIRTUAL bool	WaitForInterrupt (const INTERRUPT_ENUMS type,  const ULWord timeout = 50);
	AJA_VIRTUAL bool	GetInterruptCount (const INTERRUPT_ENUMS eInterrupt, ULWord & outCount);
	AJA_VIRTUAL bool	WaitForChangeEvent (UInt32 timeout = 0);
	AJA_VIRTUAL bool	DmaTransfer (const NTV2DMAEngine inDMAEngine,
									const bool		inIsRead,
									const ULWord	inFrameNumber,
									ULWord *		pFrameBuffer,
									const ULWord	inCardOffsetBytes,
									const ULWord	inByteCount,
									const bool		inSynchronous = true);

	AJA_VIRTUAL bool	DmaTransfer (const NTV2DMAEngine inDMAEngine,
									const bool inIsRead,
									const ULWord inFrameNumber,
									ULWord * pFrameBuffer,
									const ULWord inCardOffsetBytes,
									const ULWord inByteCount,
									const ULWord inNumSegments,
									const ULWord inSegmentHostPitch,
									const ULWord inSegmentCardPitch,
									const bool inSynchronous = true);

	AJA_VIRTUAL bool	DmaTransfer (const NTV2DMAEngine inDMAEngine,
									const NTV2Channel inDMAChannel,
									const bool inIsTarget,
									const ULWord inFrameNumber,
									const ULWord inCardOffsetBytes,
									const ULWord inByteCount,
									const ULWord inNumSegments,
									const ULWord inSegmentHostPitch,
									const ULWord inSegmentCardPitch,
									const PCHANNEL_P2P_STRUCT & inP2PData);

	AJA_VIRTUAL bool	AutoCirculate (AUTOCIRCULATE_DATA & autoCircData);
	AJA_VIRTUAL bool	NTV2Message (NTV2_HEADER * pInMessage);
	AJA_VIRTUAL bool	ControlDriverDebugMessages (NTV2_DriverDebugMessageSet /*msgSet*/, bool /*enable*/) {return false;}
	AJA_VIRTUAL bool	RestoreHardwareProcampRegisters (void);
	///@}

#if !defined(NTV2_DEPRECATE_16_0)
	AJA_VIRTUAL NTV2_DEPRECATED_f(bool GetStreamingApplication(ULWord * pAppType, int32_t * pPID))	{return pAppType && pPID ? GetStreamingApplication(*pAppType,*pPID) : false;}	///< @deprecated	Deprecated starting in SDK 16.0.
	AJA_VIRTUAL NTV2_DEPRECATED_f(bool SystemControl(void* dataPtr, SystemControlCode systemControlCode));		///< @deprecated	Obsolete starting in SDK 16.0.
	AJA_VIRTUAL NTV2_DEPRECATED_f(void Sleep(int))		 	{}			///< @deprecated	Obsolete starting in SDK 16.0.
	AJA_VIRTUAL NTV2_DEPRECATED_f(bool MapFrameBuffers(void));			///< @deprecated	Obsolete starting in SDK 16.0.
	AJA_VIRTUAL NTV2_DEPRECATED_f(bool UnmapFrameBuffers(void));		///< @deprecated	Obsolete starting in SDK 16.0.
	AJA_VIRTUAL NTV2_DEPRECATED_f(bool MapRegisters(void));				///< @deprecated	Obsolete starting in SDK 16.0.
	AJA_VIRTUAL NTV2_DEPRECATED_f(bool UnmapRegisters(void));			///< @deprecated	Obsolete starting in SDK 16.0.
	AJA_VIRTUAL NTV2_DEPRECATED_f(bool MapXena2Flash(void));			///< @deprecated	Obsolete starting in SDK 16.0.
	AJA_VIRTUAL NTV2_DEPRECATED_f(bool UnmapXena2Flash(void));			///< @deprecated	Obsolete starting in SDK 16.0.
	AJA_VIRTUAL NTV2_SHOULD_BE_DEPRECATED(ULWord GetPCISlotNumber(void) const);	///< @deprecated	Obsolete starting in SDK 16.0.
	AJA_VIRTUAL NTV2_DEPRECATED_f(bool MapMemory(const MemoryType memType, void **memPtr));	///< @deprecated	Obsolete starting in SDK 16.0.
#endif	//	!defined(NTV2_DEPRECATE_16_0)

	AJA_VIRTUAL bool	SetAudioOutputMode(NTV2_GlobalAudioPlaybackMode mode);
	AJA_VIRTUAL bool	GetAudioOutputMode(NTV2_GlobalAudioPlaybackMode* mode);

	AJA_VIRTUAL bool	SystemStatus( void* dataPtr, SystemStatusCode systemStatusCode );
	AJA_VIRTUAL bool	KernelLog( void* dataPtr, UInt32 dataSize );
	AJA_VIRTUAL bool	ConfigureInterrupt( bool /*bEnable*/, INTERRUPT_ENUMS /*eInterruptType*/ ) {return true;}
	AJA_VIRTUAL std::string	GetConnectionType (void) const	{return IsOpen() && !IsRemote()  ?  (mIsDEXT ? "DEXT" : "KEXT")  :  "";}	//	New in SDK 17.0

#if !defined(NTV2_NULL_DEVICE)
	protected:
		AJA_VIRTUAL bool	OpenLocalPhysical (const UWord inDeviceIndex);
		AJA_VIRTUAL bool	CloseLocalPhysical (void);
#endif	//	!defined(NTV2_NULL_DEVICE)

private:
	bool			mIsDEXT;		//	Uses DEXT interface?
#if defined(USE_DEVICE_MAP)
	AJA_VIRTUAL io_connect_t	GetIOConnect (const bool inDoNotAllocate = false) const;	//	For internal use only
#else
	AJA_VIRTUAL inline io_connect_t	GetIOConnect (void) const	{return mConnection;}	//	For internal use only
	io_connect_t	mConnection;	//	My io_connect_t
#endif

	// 64 bit thunking - only for structures that contain pointers
	AJA_VIRTUAL void CopyTo_AUTOCIRCULATE_DATA_64 (AUTOCIRCULATE_DATA *p, AUTOCIRCULATE_DATA_64 *p64);
	AJA_VIRTUAL void CopyTo_AUTOCIRCULATE_DATA (AUTOCIRCULATE_DATA_64 *p64, AUTOCIRCULATE_DATA *p);
	AJA_VIRTUAL void CopyTo_AUTOCIRCULATE_TRANSFER_STRUCT_64 (AUTOCIRCULATE_TRANSFER_STRUCT *p, AUTOCIRCULATE_TRANSFER_STRUCT_64 *p64);
	AJA_VIRTUAL void CopyTo_AUTOCIRCULATE_TASK_STRUCT_64 (AUTOCIRCULATE_TASK_STRUCT *p, AUTOCIRCULATE_TASK_STRUCT_64 *p64);
	AJA_VIRTUAL void CopyTo_AUTOCIRCULATE_TRANSFER_STRUCT (AUTOCIRCULATE_TRANSFER_STRUCT_64 *p64, AUTOCIRCULATE_TRANSFER_STRUCT *p);

};	//	CNTV2MacDriverInterface

#endif	//	NTV2MACDRIVERINTERFACE_H
