/**
	@file		ntv2macdriverinterface.h
	@brief		Declares the MacOS-specific flavor of CNTV2DriverInterface.
	@copyright	(C) 2007-2020 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#ifndef NTV2MACDRIVERINTERFACE_H
#define NTV2MACDRIVERINTERFACE_H


#include "ntv2publicinterface.h"
#include "ntv2macpublicinterface.h"
#include "ntv2driverinterface.h"


//--------------------------------------------------------------------------------------------------------------------
//	class CNTV2MacDriverInterface
//--------------------------------------------------------------------------------------------------------------------
class CNTV2MacDriverInterface : public CNTV2DriverInterface
{
	/**
		@name	Construction, destruction, assignment
	**/
	///@{
	public:
							CNTV2MacDriverInterface();	///< @brief	My default constructor.
		AJA_VIRTUAL			~CNTV2MacDriverInterface();	///< @brief	My destructor.

	// Driver calls
	AJA_VIRTUAL bool	ReadRegister (const ULWord inRegisterNumber,
									  ULWord & outValue,
									  const ULWord inRegisterMask = 0xFFFFFFFF,
									  const ULWord inRegisterShift = 0x0);
#if !defined(NTV2_DEPRECATE_14_3)
	virtual inline NTV2_DEPRECATED_f(bool	ReadRegister (const ULWord inRegNum, ULWord * pOutValue, const ULWord inRegMask = 0xFFFFFFFF, const ULWord inRegShift = 0x0))
	{
		return pOutValue ? ReadRegister(inRegNum, *pOutValue, inRegMask, inRegShift) : false;
	}
#endif	//	!defined(NTV2_DEPRECATE_14_3)
	AJA_VIRTUAL bool	WriteRegister( ULWord registerNumber,
									   ULWord registerValue,
									   ULWord registerMask = 0xFFFFFFFF,
									   ULWord registerShift = 0x0 );

	AJA_VIRTUAL bool	StartDriver( DriverStartPhase phase );

	/**
		@brief		Reserves exclusive use of the AJA device for a given process, preventing other processes on the host
					from acquiring it until subsequently released.
		@result		True if successful; otherwise false.
		@param[in]	inApplicationType	An unsigned 32-bit value that uniquely and globally identifies the calling
										application or process that is requesting exclusive use of the device.
		@param[in]	inProcessID			Specifies the OS-specific process identifier that uniquely identifies the
										running process on the host machine that is requesting exclusive use of the device.
		@details	This method asks the AJA device driver to reserve exclusive use of the AJA device by the given running host process.
					The AJA device driver records the "process ID" of the process granted this exclusive access,
					and also records the 4-byte "application type" that was specified by the caller. This "application
					type" is interpreted as a four-byte (ASCII) identifier, and is displayed by the AJA Control Panel
					application to show the device is being used exclusively by the given application.
					If another host process has already reserved the device, the function will fail and return false.
		@note		AJA recommends saving the device's NTV2EveryFrameTaskMode at the time AcquireStreamForApplication
					is called, and restoring it after releasing the device.
	**/
	AJA_VIRTUAL bool	AcquireStreamForApplication (ULWord inApplicationType, int32_t inProcessID);

	/**
		@brief		Releases exclusive use of the AJA device for a given process, permitting other processes to acquire it.
		@result		True if successful; otherwise false.
		@param[in]	inApplicationType	A 32-bit value that uniquely and globally identifies the calling application or
										process that is releasing exclusive use of the device. This value must match the
										appType that was specified in the prior call to AcquireStreamForApplication.
		@param[in]	inProcessID			Specifies the OS-specific process identifier that uniquely identifies the running
										process on the host machine that is releasing exclusive use of the device. This
										value must match the "process ID" that was specified in the prior call to AcquireStreamForApplication.
		@details	This method asks the AJA device driver to release exclusive use of the AJA device by the given running host process.
					The AJA device driver records the "process ID" of the process granted this exclusive access, and
					also records the 4-byte "application type" that was specified by the caller.
					This method will fail and return false if the specified application type or process ID values
					don't match those used in the prior call to AcquireStreamForApplication.
		@note		AJA recommends saving the device's NTV2EveryFrameTaskMode at the time AcquireStreamForApplication
					is called, and restoring it after releasing the device.
	**/
	AJA_VIRTUAL bool	ReleaseStreamForApplication (ULWord inApplicationType, int32_t inProcessID);

	AJA_VIRTUAL bool	AcquireStreamForApplicationWithReference (ULWord inApplicationType, int32_t inProcessID);
	AJA_VIRTUAL bool	ReleaseStreamForApplicationWithReference (ULWord inApplicationType, int32_t inProcessID);
	
	AJA_VIRTUAL bool	KernelLog( void* dataPtr, UInt32 dataSize );

	AJA_VIRTUAL bool	SetStreamingApplication (const ULWord appType, const int32_t pid);

	/**
		@brief		Answers whether or not an application is currently using the AJA device, and if so, reports the host process ID.
		@param[out]	pOutAppFourCC	If non-NULL, specifies a valid pointer to the variable that is to receive the "four CC"
									identifier of the application that's currently using the AJA device.
		@param[out]	pOutAppPID		If non-NULL, specifies a valid pointer to the variable that is to receive the host process
									identifier of the application that's currently using the AJA device.
		@result		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool	GetStreamingApplication (ULWord * pOutAppFourCC, int32_t * pOutAppPID);

	AJA_VIRTUAL bool	SetDefaultDeviceForPID( int32_t pid );
	AJA_VIRTUAL bool	IsDefaultDeviceForPID( int32_t pid );
	AJA_VIRTUAL bool	WaitForInterrupt (const INTERRUPT_ENUMS type,  const ULWord timeout = 50);
	AJA_VIRTUAL bool	GetInterruptCount (const INTERRUPT_ENUMS eInterrupt, ULWord & outCount);
	AJA_VIRTUAL bool	WaitForChangeEvent( UInt32 timeout = 0 );
	AJA_VIRTUAL bool	DmaTransfer (const NTV2DMAEngine inDMAEngine,
									const bool		inIsRead,
									const ULWord	inFrameNumber,
									ULWord *		pFrameBuffer,
									const ULWord	inOffsetBytes,
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

	AJA_VIRTUAL bool	AutoCirculate( AUTOCIRCULATE_DATA &autoCircData );
	AJA_VIRTUAL bool	NTV2Message (NTV2_HEADER * pInMessage);
	AJA_VIRTUAL bool	ControlDriverDebugMessages( NTV2_DriverDebugMessageSet /*msgSet*/, bool /*enable*/ ) {return false;}
	AJA_VIRTUAL bool	RestoreHardwareProcampRegisters( void );

	AJA_VIRTUAL bool	SetAudioOutputMode(NTV2_GlobalAudioPlaybackMode mode);
	AJA_VIRTUAL bool	GetAudioOutputMode(NTV2_GlobalAudioPlaybackMode* mode);

	AJA_VIRTUAL bool	SystemStatus( void* dataPtr, SystemStatusCode systemStatusCode );

	AJA_VIRTUAL bool	ConfigureInterrupt( bool /*bEnable*/, INTERRUPT_ENUMS /*eInterruptType*/ ) {return true;}

#if !defined(NTV2_DEPRECATE_15_6)
	AJA_VIRTUAL NTV2_DEPRECATED_f(bool SetUserModeDebugLevel (ULWord  level));	///< @deprecated	Obsolete starting after SDK 15.5.
	AJA_VIRTUAL NTV2_DEPRECATED_f(bool GetUserModeDebugLevel (ULWord* level));	///< @deprecated	Obsolete starting after SDK 15.5.
	AJA_VIRTUAL NTV2_DEPRECATED_f(bool SetKernelModeDebugLevel (ULWord  level));///< @deprecated	Obsolete starting after SDK 15.5.
	AJA_VIRTUAL NTV2_DEPRECATED_f(bool GetKernelModeDebugLevel (ULWord* level));///< @deprecated	Obsolete starting after SDK 15.5.
	AJA_VIRTUAL NTV2_DEPRECATED_f(bool SetUserModePingLevel (ULWord  level));	///< @deprecated	Obsolete starting after SDK 15.5.
	AJA_VIRTUAL NTV2_DEPRECATED_f(bool GetUserModePingLevel (ULWord* level));	///< @deprecated	Obsolete starting after SDK 15.5.
	AJA_VIRTUAL NTV2_DEPRECATED_f(bool SetKernelModePingLevel (ULWord  level));	///< @deprecated	Obsolete starting after SDK 15.5.
	AJA_VIRTUAL NTV2_DEPRECATED_f(bool GetKernelModePingLevel (ULWord* level));	///< @deprecated	Obsolete starting after SDK 15.5.
	AJA_VIRTUAL NTV2_DEPRECATED_f(bool SetLatencyTimerValue (ULWord value));	///< @deprecated	Obsolete starting after SDK 15.5.
	AJA_VIRTUAL NTV2_DEPRECATED_f(bool GetLatencyTimerValue (ULWord* value));	///< @deprecated	Obsolete starting after SDK 15.5.
	AJA_VIRTUAL NTV2_DEPRECATED_f(bool SetDebugFilterStrings (const char* includeString,const char* excludeString));	///< @deprecated	Obsolete starting after SDK 15.5.
	AJA_VIRTUAL NTV2_DEPRECATED_f(bool GetDebugFilterStrings (char* includeString,char* excludeString));	///< @deprecated	Obsolete starting after SDK 15.5.
	AJA_VIRTUAL NTV2_DEPRECATED_f(bool LockFormat (void));		///< @deprecated	Obsolete after SDK 15.5.
	AJA_VIRTUAL NTV2_DEPRECATED_f(bool GetQuickTimeTime (UInt32 *time, UInt32 *scale));	//	Formerly called "GetTime" which shadowed CNTV2KonaFlashProgram::GetTime
#endif	//	!defined(NTV2_DEPRECATE_15_6)
#if !defined(NTV2_DEPRECATE_16_0)
	AJA_VIRTUAL NTV2_SHOULD_BE_DEPRECATED(bool SystemControl(void* dataPtr, SystemControlCode systemControlCode));		///< @deprecated	Obsolete starting in SDK 16.0.
	AJA_VIRTUAL NTV2_SHOULD_BE_DEPRECATED(void Sleep(int))		 	{}			///< @deprecated	Obsolete starting in SDK 16.0.
	AJA_VIRTUAL NTV2_SHOULD_BE_DEPRECATED(bool MapFrameBuffers(void));			///< @deprecated	Obsolete starting in SDK 16.0.
	AJA_VIRTUAL NTV2_SHOULD_BE_DEPRECATED(bool UnmapFrameBuffers(void));		///< @deprecated	Obsolete starting in SDK 16.0.
	AJA_VIRTUAL NTV2_SHOULD_BE_DEPRECATED(bool MapRegisters(void));				///< @deprecated	Obsolete starting in SDK 16.0.
	AJA_VIRTUAL NTV2_SHOULD_BE_DEPRECATED(bool UnmapRegisters(void));			///< @deprecated	Obsolete starting in SDK 16.0.
	AJA_VIRTUAL NTV2_SHOULD_BE_DEPRECATED(bool MapXena2Flash(void));			///< @deprecated	Obsolete starting in SDK 16.0.
	AJA_VIRTUAL NTV2_SHOULD_BE_DEPRECATED(bool UnmapXena2Flash(void));			///< @deprecated	Obsolete starting in SDK 16.0.
	AJA_VIRTUAL NTV2_SHOULD_BE_DEPRECATED(ULWord GetPCISlotNumber(void) const);	///< @deprecated	Obsolete starting in SDK 16.0.
	AJA_VIRTUAL NTV2_SHOULD_BE_DEPRECATED(bool MapMemory(const MemoryType memType, void **memPtr));	///< @deprecated	Obsolete starting in SDK 16.0.
#endif	//	!defined(NTV2_DEPRECATE_16_0)

public:
	static const std::string &	GetIOServiceName (void);	//	For internal use only
#if !defined(NTV2_DEPRECATE_14_3)
	static void			SetDebugLogging (const uint64_t inWhichUserClientCommands);
#endif
	static void			DumpDeviceMap (void);
	static UWord		GetConnectionCount (void);
	static ULWord		GetConnectionChecksum (void);

protected:
		AJA_VIRTUAL bool	OpenLocalPhysical (const UWord inDeviceIndex);
		AJA_VIRTUAL bool	CloseLocalPhysical (void);

private:
	AJA_VIRTUAL io_connect_t	GetIOConnect (const bool inDoNotAllocate = false) const;	//	For internal use only

		// 64 bit thunking - only for structures that contain pointers
	AJA_VIRTUAL void CopyTo_AUTOCIRCULATE_DATA_64 (AUTOCIRCULATE_DATA *p, AUTOCIRCULATE_DATA_64 *p64);
	AJA_VIRTUAL void CopyTo_AUTOCIRCULATE_DATA (AUTOCIRCULATE_DATA_64 *p64, AUTOCIRCULATE_DATA *p);
	AJA_VIRTUAL void CopyTo_AUTOCIRCULATE_TRANSFER_STRUCT_64 (AUTOCIRCULATE_TRANSFER_STRUCT *p, AUTOCIRCULATE_TRANSFER_STRUCT_64 *p64);
	AJA_VIRTUAL void CopyTo_AUTOCIRCULATE_TASK_STRUCT_64 (AUTOCIRCULATE_TASK_STRUCT *p, AUTOCIRCULATE_TASK_STRUCT_64 *p64);
	AJA_VIRTUAL void CopyTo_AUTOCIRCULATE_TRANSFER_STRUCT (AUTOCIRCULATE_TRANSFER_STRUCT_64 *p64, AUTOCIRCULATE_TRANSFER_STRUCT *p);

};	//	CNTV2MacDriverInterface

#endif	//	NTV2MACDRIVERINTERFACE_H
