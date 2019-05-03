/**
	@file		ntv2macdriverinterface.h
	@brief		Declares the MacOS-specific flavor of CNTV2DriverInterface.
	@copyright	(C) 2007-2019 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#ifndef NTV2MACDRIVERINTERFACE_H
#define NTV2MACDRIVERINTERFACE_H


#include "ntv2publicinterface.h"
#include "ntv2macpublicinterface.h"
#include "ntv2driverinterface.h"


typedef struct MDIStats
{
	inline MDIStats ()	: fConstructCount(0), fDestructCount(0), fOpenCount(0), fCloseCount(0)	{}
	~MDIStats();
	uint32_t	fConstructCount;	///< @brief	Number of constructor calls made
	uint32_t	fDestructCount;		///< @brief	Number of destructor calls made
	uint32_t	fOpenCount;			///< @brief	Number of local Open calls made
	uint32_t	fCloseCount;		///< @brief	Number of local Close calls made
} MDIStats;


//--------------------------------------------------------------------------------------------------------------------
//	class CNTV2MacDriverInterface
//--------------------------------------------------------------------------------------------------------------------
class CNTV2MacDriverInterface : public CNTV2DriverInterface
{
public:
						CNTV2MacDriverInterface( void );
	AJA_VIRTUAL			~CNTV2MacDriverInterface( void );

#if !defined(NTV2_DEPRECATE_14_3)
	virtual bool		Open (UWord inDeviceIndex,
							  bool displayError,
							  NTV2DeviceType eDeviceType,
							  const char * hostName);
#endif	//	!defined(NTV2_DEPRECATE_14_3)
	AJA_VIRTUAL bool	Open (const UWord inDeviceIndex = 0,
							  const std::string & inHostName = std::string());
	AJA_VIRTUAL bool	TestOpen();
	AJA_VIRTUAL bool	Close (void);
	AJA_VIRTUAL ULWord	GetPCISlotNumber (void) const;
	AJA_VIRTUAL bool	MapFrameBuffers( void );
	AJA_VIRTUAL bool	UnmapFrameBuffers( void );
	AJA_VIRTUAL bool	MapRegisters( void );
	AJA_VIRTUAL bool	UnmapRegisters( void );
	AJA_VIRTUAL bool	MapXena2Flash( void );
	AJA_VIRTUAL bool	UnmapXena2Flash( void );
	AJA_VIRTUAL bool	MapMemory( MemoryType memType, void **memPtr );

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

	AJA_VIRTUAL bool	SetStreamingApplication( ULWord appType, int32_t pid );

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
	AJA_VIRTUAL bool	LockFormat( void );
	AJA_VIRTUAL bool	WaitForInterrupt( INTERRUPT_ENUMS type,  ULWord timeout = 50 );
	AJA_VIRTUAL bool	GetInterruptCount( INTERRUPT_ENUMS eInterrupt, ULWord *pCount );
	AJA_VIRTUAL bool	WaitForChangeEvent( UInt32 timeout = 0 );
	AJA_VIRTUAL bool    GetQuickTimeTime( UInt32 *time, UInt32 *scale );	//	Formerly called "GetTime" which shadowed CNTV2KonaFlashProgram::GetTime
	AJA_VIRTUAL bool	DmaTransfer (	const NTV2DMAEngine	inDMAEngine,
										const bool			inIsRead,
										const ULWord		inFrameNumber,
										ULWord *			pFrameBuffer,
										const ULWord		inOffsetBytes,
										const ULWord		inByteCount,
										const bool			inSynchronous = true);

	AJA_VIRTUAL bool	DmaTransfer ( NTV2DMAEngine DMAEngine,
									  bool bRead,
									  ULWord frameNumber,
									  ULWord * pFrameBuffer,
									  ULWord offsetBytes,
									  ULWord bytes,
									  ULWord videoNumSegments,
									  ULWord videoSegmentHostPitch,
									  ULWord videoSegmentCardPitch,
									  bool bSync );

	AJA_VIRTUAL bool	DmaTransfer (NTV2DMAEngine DMAEngine,
									NTV2Channel DMAChannel,
									bool bTarget,
									ULWord frameNumber,
									ULWord frameOffset,
									ULWord videoSize,
									ULWord videoNumSegments,
									ULWord videoSegmentHostPitch,
									ULWord videoSegmentCardPitch,
									PCHANNEL_P2P_STRUCT pP2PData);

	AJA_VIRTUAL bool	AutoCirculate( AUTOCIRCULATE_DATA &autoCircData );
	AJA_VIRTUAL bool	NTV2Message (NTV2_HEADER * pInMessage);
	AJA_VIRTUAL bool	ControlDriverDebugMessages( NTV2_DriverDebugMessageSet /*msgSet*/, bool /*enable*/ ) {return false;}
	AJA_VIRTUAL bool	RestoreHardwareProcampRegisters( void );

	AJA_VIRTUAL	Word	SleepMs( LWord msec ) const;
	AJA_VIRTUAL void	Sleep( int /*milliseconds*/ ) {}

	AJA_VIRTUAL bool	SetUserModeDebugLevel( ULWord  level );
	AJA_VIRTUAL bool	GetUserModeDebugLevel( ULWord* level );
	AJA_VIRTUAL bool	SetKernelModeDebugLevel( ULWord  level );
	AJA_VIRTUAL bool	GetKernelModeDebugLevel( ULWord* level );

	AJA_VIRTUAL bool	SetUserModePingLevel( ULWord  level );
	AJA_VIRTUAL bool	GetUserModePingLevel( ULWord* level );
	AJA_VIRTUAL bool	SetKernelModePingLevel( ULWord  level );
	AJA_VIRTUAL bool	GetKernelModePingLevel( ULWord* level );

	AJA_VIRTUAL bool	SetLatencyTimerValue( ULWord value );
	AJA_VIRTUAL bool	GetLatencyTimerValue( ULWord* value );

	AJA_VIRTUAL bool	SetDebugFilterStrings( const char* includeString,const char* excludeString );
	AJA_VIRTUAL bool	GetDebugFilterStrings( char* includeString,char* excludeString );

	AJA_VIRTUAL bool	ReadRP188Registers( NTV2Channel channel, RP188_STRUCT* pRP188Data );

#if	!defined(NTV2_DEPRECATE_15_1)
	AJA_VIRTUAL bool	SetOutputTimecodeOffset( ULWord frames );
	AJA_VIRTUAL bool	GetOutputTimecodeOffset( ULWord* pFrames );
#endif	//	!defined(NTV2_DEPRECATE_15_1)
	AJA_VIRTUAL bool	SetOutputTimecodeType( ULWord type );
	AJA_VIRTUAL bool	GetOutputTimecodeType( ULWord* pType );
	AJA_VIRTUAL bool	SetAudioOutputMode(NTV2_GlobalAudioPlaybackMode mode);
	AJA_VIRTUAL bool	GetAudioOutputMode(NTV2_GlobalAudioPlaybackMode* mode);

	AJA_VIRTUAL bool	SystemStatus( void* dataPtr, SystemStatusCode systemStatusCode );
	AJA_VIRTUAL bool	SystemControl( void* dataPtr, SystemControlCode systemControlCode );

	// Mac doesn't do these:
	AJA_VIRTUAL bool	DmaUnlock( void ) {return false;}
	AJA_VIRTUAL bool	CompleteMemoryForDMA( ULWord* /*pFrameBuffer*/ ) {return false;}
	AJA_VIRTUAL bool	PrepareMemoryForDMA( ULWord* /*pFrameBuffer*/, ULWord /*ulNumBytes*/ ) {return false;}
	AJA_VIRTUAL bool	ConfigureInterrupt( bool /*bEnable*/, INTERRUPT_ENUMS /*eInterruptType*/ ) {return true;}
	AJA_VIRTUAL bool	ConfigureSubscription (bool bSubscribe, INTERRUPT_ENUMS eInterruptType, PULWord & hSubscription);
	AJA_VIRTUAL bool	SetShareMode( bool /*bShared*/ ) {return false;}
	AJA_VIRTUAL bool	SetOverlappedMode( bool /*bOverlapped*/ ) {return false;}

public:
	static const char *	GetIOServiceName (void);	//	For internal use only
#if !defined(NTV2_DEPRECATE_14_3)
	static void			SetDebugLogging (const uint64_t inWhichUserClientCommands);
#endif
	static void			DumpDeviceMap (void);
	static UWord		GetConnectionCount (void);
	static ULWord		GetConnectionChecksum (void);
	static void			GetClientStats (MDIStats & outStats);
	static ULWord		GetMacRawDriverVersion (void);

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
