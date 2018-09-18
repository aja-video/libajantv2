/**
	@file		ntv2macdriverinterface.h
	@brief		Declares the MacOS-specific flavor of CNTV2DriverInterface.
	@copyright	(C) 2007-2018 AJA Video Systems, Inc.	Proprietary and confidential information.
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
	virtual 			~CNTV2MacDriverInterface( void );

#if !defined(NTV2_DEPRECATE_14_3)
	virtual bool		Open (UWord inDeviceIndex,
							  bool displayError,
							  NTV2DeviceType eDeviceType,
							  const char * hostName);
#endif	//	!defined(NTV2_DEPRECATE_14_3)
	virtual bool		Open (const UWord inDeviceIndex = 0,
							  const std::string & inHostName = std::string());
	bool				TestOpen();
	virtual bool		Close (void);
	virtual ULWord		GetPCISlotNumber (void) const;
	virtual bool		MapFrameBuffers( void );
	virtual bool		UnmapFrameBuffers( void );
	virtual bool		MapRegisters( void );
	virtual bool		UnmapRegisters( void );
	virtual bool		MapXena2Flash( void );
	virtual bool		UnmapXena2Flash( void );
	bool				MapMemory( MemoryType memType, void **memPtr );

	// Driver calls
	virtual bool		ReadRegister (const ULWord inRegisterNumber,
									  ULWord & outValue,
									  const ULWord inRegisterMask = 0xFFFFFFFF,
									  const ULWord inRegisterShift = 0x0);
#if !defined(NTV2_DEPRECATE_14_3)
	virtual inline NTV2_DEPRECATED_f(bool	ReadRegister (const ULWord inRegNum, ULWord * pOutValue, const ULWord inRegMask = 0xFFFFFFFF, const ULWord inRegShift = 0x0))
	{
		return pOutValue ? ReadRegister(inRegNum, *pOutValue, inRegMask, inRegShift) : false;
	}
#endif	//	!defined(NTV2_DEPRECATE_14_3)
	virtual bool		WriteRegister( ULWord registerNumber,
									   ULWord registerValue,
									   ULWord registerMask = 0xFFFFFFFF,
									   ULWord registerShift = 0x0 );
	UInt32				GetDriverVersion( NumVersion *driverVers = 0 );
	bool				StartDriver( DriverStartPhase phase );

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
	bool				AcquireStreamForApplication (ULWord inApplicationType, int32_t inProcessID);

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
	bool				ReleaseStreamForApplication (ULWord inApplicationType, int32_t inProcessID);

	bool				AcquireStreamForApplicationWithReference (ULWord inApplicationType, int32_t inProcessID);
	bool				ReleaseStreamForApplicationWithReference (ULWord inApplicationType, int32_t inProcessID);
	
	bool				KernelLog( void* dataPtr, UInt32 dataSize );

	bool				SetStreamingApplication( ULWord appType, int32_t pid );

	/**
		@brief		Answers whether or not an application is currently using the AJA device, and if so, reports the host process ID.
		@param[out]	pOutAppFourCC	If non-NULL, specifies a valid pointer to the variable that is to receive the "four CC"
									identifier of the application that's currently using the AJA device.
		@param[out]	pOutAppPID		If non-NULL, specifies a valid pointer to the variable that is to receive the host process
									identifier of the application that's currently using the AJA device.
		@result		True if successful; otherwise false.
	**/
	bool				GetStreamingApplication (ULWord * pOutAppFourCC, int32_t * pOutAppPID);

	bool				SetDefaultDeviceForPID( int32_t pid );
	bool				IsDefaultDeviceForPID( int32_t pid );
	bool				LockFormat( void );
	bool				SetAVSyncPattern( void* dataPtr, UInt32 dataSize );
	bool				TriggerAVSync( NTV2Crosspoint channelSpec, UInt32 count );
	virtual bool		WaitForInterrupt( INTERRUPT_ENUMS type,  ULWord timeout = 50 );
	virtual bool		GetInterruptCount( INTERRUPT_ENUMS eInterrupt, ULWord *pCount );
	bool				WaitForChangeEvent( UInt32 timeout = 0 );
	bool                GetTime( UInt32 *time, UInt32 *scale );
	virtual bool		DmaTransfer (	const NTV2DMAEngine	inDMAEngine,
										const bool			inIsRead,
										const ULWord		inFrameNumber,
										ULWord *			pFrameBuffer,
										const ULWord		inOffsetBytes,
										const ULWord		inByteCount,
										const bool			inSynchronous = true);

	bool				DmaTransfer ( NTV2DMAEngine DMAEngine,
									  bool bRead,
									  ULWord frameNumber,
									  ULWord * pFrameBuffer,
									  ULWord offsetBytes,
									  ULWord bytes,
									  ULWord videoNumSegments,
									  ULWord videoSegmentHostPitch,
									  ULWord videoSegmentCardPitch,
									  bool bSync );

	bool				DmaTransfer (NTV2DMAEngine DMAEngine,
									NTV2Channel DMAChannel,
									bool bTarget,
									ULWord frameNumber,
									ULWord frameOffset,
									ULWord videoSize,
									ULWord videoNumSegments,
									ULWord videoSegmentHostPitch,
									ULWord videoSegmentCardPitch,
									PCHANNEL_P2P_STRUCT pP2PData);

	virtual bool		AutoCirculate( AUTOCIRCULATE_DATA &autoCircData );
	virtual bool		NTV2Message (NTV2_HEADER * pInMessage);
	virtual bool		ControlDriverDebugMessages( NTV2_DriverDebugMessageSet /*msgSet*/, bool /*enable*/ ) {return false;}
	virtual bool		GetDriverVersion( ULWord* driverVersion );
	virtual bool		RestoreHardwareProcampRegisters( void );

	virtual	Word		SleepMs( LWord msec ) const;
	void				Sleep( int /*milliseconds*/ ) {}

	bool				SetUserModeDebugLevel( ULWord  level );
	bool				GetUserModeDebugLevel( ULWord* level );
	bool				SetKernelModeDebugLevel( ULWord  level );
	bool				GetKernelModeDebugLevel( ULWord* level );

	bool				SetUserModePingLevel( ULWord  level );
	bool				GetUserModePingLevel( ULWord* level );
	bool				SetKernelModePingLevel( ULWord  level );
	bool				GetKernelModePingLevel( ULWord* level );

	bool				SetLatencyTimerValue( ULWord value );
	bool				GetLatencyTimerValue( ULWord* value );

	bool				SetDebugFilterStrings( const char* includeString,const char* excludeString );
	bool				GetDebugFilterStrings( char* includeString,char* excludeString );

	bool				SetAudioAVSyncEnable( bool enable );
	bool				GetAudioAVSyncEnable( bool* enable );

	bool				ReadRP188Registers( NTV2Channel channel, RP188_STRUCT* pRP188Data );

	bool				SetOutputTimecodeOffset( ULWord frames );
	bool				GetOutputTimecodeOffset( ULWord* pFrames );
	bool				SetOutputTimecodeType( ULWord type );
	bool				GetOutputTimecodeType( ULWord* pType );
	bool				SetAudioOutputMode(NTV2_GlobalAudioPlaybackMode mode);
	bool				GetAudioOutputMode(NTV2_GlobalAudioPlaybackMode* mode);

	bool				SystemStatus( void* dataPtr, SystemStatusCode systemStatusCode );
	bool				SystemControl( void* dataPtr, SystemControlCode systemControlCode );

	bool				SwitchBitfile( NTV2DeviceID boardID, NTV2BitfileType bitfile );		//	DEPRECATION_CANDIDATE

	// We don't do these
	virtual bool		DmaUnlock( void ) {return false;}
	virtual bool		CompleteMemoryForDMA( ULWord* /*pFrameBuffer*/ ) {return false;}
	virtual bool		PrepareMemoryForDMA( ULWord* /*pFrameBuffer*/, ULWord /*ulNumBytes*/ ) {return false;}
	virtual bool		ConfigureInterrupt( bool /*bEnable*/, INTERRUPT_ENUMS /*eInterruptType*/ ) {return true;}
	virtual bool		ConfigureSubscription (bool bSubscribe, INTERRUPT_ENUMS eInterruptType, PULWord & hSubscription);
	virtual bool		SetShareMode( bool /*bShared*/ ) {return false;}
	virtual bool		SetOverlappedMode( bool /*bOverlapped*/ ) {return false;}

public:
	static const char *	GetIOServiceName (void);	//	For internal use only
#if !defined(NTV2_DEPRECATE_14_3)
	static void			SetDebugLogging (const uint64_t inWhichUserClientCommands);
#endif
	static void			DumpDeviceMap (void);
	static UWord		GetConnectionCount (void);
	static ULWord		GetConnectionChecksum (void);
	static void			GetClientStats (MDIStats & outStats);

private:
	virtual io_connect_t		GetIOConnect (const bool inDoNotAllocate = false) const;	//	For internal use only

		// 64 bit thunking - only for structures that contain pointers
	void CopyTo_AUTOCIRCULATE_DATA_64 (AUTOCIRCULATE_DATA *p, AUTOCIRCULATE_DATA_64 *p64);
	void CopyTo_AUTOCIRCULATE_DATA (AUTOCIRCULATE_DATA_64 *p64, AUTOCIRCULATE_DATA *p);
	void CopyTo_AUTOCIRCULATE_TRANSFER_STRUCT_64 (AUTOCIRCULATE_TRANSFER_STRUCT *p, AUTOCIRCULATE_TRANSFER_STRUCT_64 *p64);
	void CopyTo_AUTOCIRCULATE_TASK_STRUCT_64 (AUTOCIRCULATE_TASK_STRUCT *p, AUTOCIRCULATE_TASK_STRUCT_64 *p64);
	void CopyTo_AUTOCIRCULATE_TRANSFER_STRUCT (AUTOCIRCULATE_TRANSFER_STRUCT_64 *p64, AUTOCIRCULATE_TRANSFER_STRUCT *p);

};	//	CNTV2MacDriverInterface

#endif	//	NTV2MACDRIVERINTERFACE_H
