/**
	@file		ntv2windriverinterface.h
	@brief		Declares the MSWindows-specific flavor of CNTV2DriverInterface.
	@copyright	(C) 2003-2019 AJA Video Systems, Inc.	Proprietary and confidential information.
**/
#ifndef NTV2WINDRIVERINTERFACE_H
#define NTV2WINDRIVERINTERFACE_H

#include "ajaexport.h"

#include <ajatypes.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <tchar.h>
#include <ks.h>
#include <ksmedia.h>
#include <Setupapi.h>
#include <cfgmgr32.h>
#include "psapi.h"

#include "ntv2driverinterface.h"
#include "ntv2winpublicinterface.h"
#include "ntv2devicefeatures.h"
#include <vector>


class AJAExport CNTV2WinDriverInterface : public CNTV2DriverInterface
{
public:
	CNTV2WinDriverInterface();
	virtual ~CNTV2WinDriverInterface();

public:
	// Interfaces
	// Call this method to set the Open share mode
	bool SetShareMode (bool bShared);

	// Call this method to set the Open overlapped mode
	bool SetOverlappedMode (bool bOverlapped);

	bool Open (const UWord inDeviceIndex = 0,
			  const std::string & inHostName = std::string());
#if !defined(NTV2_DEPRECATE_14_3)
	bool Open(UWord boardNumber, bool displayError,
        NTV2DeviceType eBoardType,
		const char *hostname);   // Non-null: card on remote host
#endif	//	!defined(NTV2_DEPRECATE_14_3)

	bool Close();

	//NTV2BoardType   GetCompileFlag (void);

	bool WriteRegister(ULWord registerNumber,
							   ULWord registerValue,
							   ULWord registerMask = 0xFFFFFFFF,
							   ULWord registerShift = 0x0);
	virtual bool		ReadRegister (const ULWord inRegisterNumber,
									  ULWord & outValue,
									  const ULWord inRegisterMask = 0xFFFFFFFF,
									  const ULWord inRegisterShift = 0x0);
#if !defined(NTV2_DEPRECATE_14_3)
	virtual inline NTV2_SHOULD_BE_DEPRECATED(bool	ReadRegister (const ULWord inRegNum, ULWord * pOutValue, const ULWord inRegMask = 0xFFFFFFFF, const ULWord inRegShift = 0))
	{
		return pOutValue ? ReadRegister(inRegNum, *pOutValue, inRegMask, inRegShift) : false;
	}
#endif	//	!defined(NTV2_DEPRECATE_14_3)

    bool DmaTransfer (	const NTV2DMAEngine	inDMAEngine,
                        const bool			inIsRead,
                        const ULWord		inFrameNumber,
                        ULWord *			pFrameBuffer,
                        const ULWord		inOffsetBytes,
                        const ULWord		inByteCount,
                        const bool			inSynchronous = true);

	bool DmaTransfer (NTV2DMAEngine DMAEngine,
						bool bRead,
						ULWord frameNumber,
						ULWord * pFrameBuffer,
						ULWord offsetBytes,
						ULWord bytes,
						ULWord videoNumSegments,
						ULWord videoSegmentHostPitch,
						ULWord videoSegmentCardPitch,
						bool bSync);

	bool DmaTransfer (NTV2DMAEngine DMAEngine,
						NTV2Channel DMAChannel,
						bool bTarget,
						ULWord frameNumber,
						ULWord frameOffset,
						ULWord videoSize,
						ULWord videoNumSegments,
						ULWord videoSegmentHostPitch,
						ULWord videoSegmentCardPitch,
						PCHANNEL_P2P_STRUCT pP2PData);

	bool MapMemory (PVOID pvUserVa, ULWord ulNumBytes, bool bMap, ULWord* ulUser = NULL);

	bool DmaUnlock   (void);

	bool CompleteMemoryForDMA (ULWord * pFrameBuffer);

	bool PrepareMemoryForDMA (ULWord * pFrameBuffer, ULWord ulNumBytes);

	bool ConfigureInterrupt (bool bEnable,
									 INTERRUPT_ENUMS eInterruptType);

	bool MapFrameBuffers (void);

	bool UnmapFrameBuffers (void);

	bool MapRegisters (void);

	bool UnmapRegisters (void);

	bool MapXena2Flash (void);

	bool UnmapXena2Flash (void);

	bool ConfigureSubscription (bool bSubscribe,
	                                    INTERRUPT_ENUMS eInterruptType,
				                        PULWord & hSubcription);

	bool GetInterruptCount (INTERRUPT_ENUMS eInterrupt,
								    ULWord *pCount);

	bool WaitForInterrupt (INTERRUPT_ENUMS eInterruptType, ULWord timeOutMs = 50);

	HANDLE GetInterruptEvent (INTERRUPT_ENUMS eInterruptType);

	bool AutoCirculate (AUTOCIRCULATE_DATA &autoCircData);
	
	virtual bool NTV2Message (NTV2_HEADER * pInMessage);

	bool HevcSendMessage (HevcMessageHeader* pMessage);
	
  	bool ControlDriverDebugMessages(NTV2_DriverDebugMessageSet msgSet,
									bool enable );

	virtual Word SleepMs(LWord msec) { Sleep(msec); return (Word)msec;}

public:
//
//  virtual register access.
//
    bool SetRelativeVideoPlaybackDelay (ULWord frameDelay);
    bool GetRelativeVideoPlaybackDelay (ULWord* frameDelay);
    bool SetAudioPlaybackPinDelay (ULWord millisecondDelay);
    bool GetAudioPlaybackPinDelay (ULWord* millisecondDelay);
    bool SetAudioRecordPinDelay (ULWord millisecondDelay);
    bool GetAudioRecordPinDelay (ULWord* millisecondDelay);
	bool SetAudioOutputMode(NTV2_GlobalAudioPlaybackMode mode);
	bool GetAudioOutputMode(NTV2_GlobalAudioPlaybackMode* mode);
    bool SetStrictTiming (ULWord strictTiming);
    bool GetStrictTiming (ULWord* strictTiming);

	// These are only implemented in Mac code but need to be here to satisfy Win/Linux build
	bool		SuspendAudio ();
	bool		ResumeAudio ();


	PerfCounterTimestampMode GetPerfCounterTimestampMode();
	void SetPerfCounterTimestampMode(PerfCounterTimestampMode mode);

	LWord64 GetLastOutputVerticalTimestamp();
	LWord64 GetLastInput1VerticalTimestamp();
	LWord64 GetLastInput2VerticalTimestamp();

	LWord64 GetLastOutputVerticalTimestamp(NTV2Channel channel);
	LWord64 GetLastInputVerticalTimestamp(NTV2Channel channel);

	bool DisplayNTV2Error (const char *str);

// Management of downloaded Xilinx bitfile
    bool DriverGetBitFileInformation(BITFILE_INFO_STRUCT &bitFileInfo,
		                             NTV2BitFileType bitFileType=NTV2_VideoProcBitFile);

    bool DriverSetBitFileInformation(BITFILE_INFO_STRUCT &bitFileInfo);

	// Functions for cards that support more than one bitfile

	bool RestoreHardwareProcampRegisters();

	bool NeedToLimitDMAToOneMegabyte();

	bool ReadRP188Registers( NTV2Channel channel, RP188_STRUCT* pRP188Data );

	bool SetOutputTimecodeOffset( ULWord frames );
	bool GetOutputTimecodeOffset( ULWord* pFrames );
	bool SetOutputTimecodeType( ULWord type );
	bool GetOutputTimecodeType( ULWord* pType );
	bool LockFormat( void );
	bool StartDriver( DriverStartPhase phase );

	bool AcquireStreamForApplicationWithReference( ULWord appType, int32_t pid );
	bool ReleaseStreamForApplicationWithReference( ULWord appType, int32_t pid );

	/**
		@brief		Reserves exclusive use of the AJA device for a given process, preventing other processes on the host
					from acquiring it until subsequently released.
		@result		True if successful; otherwise false.
		@param[in]	appType			An unsigned 32-bit value that uniquely and globally identifies the calling
									application or process that is requesting exclusive use of the device.
		@param[in]	pid				Specifies the OS-specific process identifier that uniquely identifies the
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
	bool AcquireStreamForApplication( ULWord appType, int32_t pid );

	/**
		@brief		Releases exclusive use of the AJA device for a given process, permitting other processes to acquire it.
		@result		True if successful; otherwise false.
		@param[in]	appType			A 32-bit value that uniquely and globally identifies the calling application or
									process that is releasing exclusive use of the device. This value must match the
									appType that was specified in the prior call to AcquireStreamForApplication.
		@param[in]	pid				Specifies the OS-specific process identifier that uniquely identifies the running
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
	bool ReleaseStreamForApplication( ULWord appType, int32_t pid );

	bool SetStreamingApplication( ULWord appType, int32_t pid );
	bool GetStreamingApplication( ULWord *appType, int32_t  *pid );
	bool SetDefaultDeviceForPID( int32_t pid );
	bool IsDefaultDeviceForPID( int32_t pid );

	bool SetUserModeDebugLevel( ULWord  level );
	bool GetUserModeDebugLevel( ULWord* level );
	bool SetKernelModeDebugLevel( ULWord  level );
	bool GetKernelModeDebugLevel( ULWord* level );

	bool SetUserModePingLevel( ULWord  level );
	bool GetUserModePingLevel( ULWord* level );
	bool SetKernelModePingLevel( ULWord  level );
	bool GetKernelModePingLevel( ULWord* level );

	bool SetLatencyTimerValue( ULWord value );
	bool GetLatencyTimerValue( ULWord* value );

	bool SetDebugFilterStrings( const char* includeString,const char* excludeString );
	bool GetDebugFilterStrings( char* includeString,char* excludeString );

	bool ProgramXilinx( void* dataPtr, uint32_t dataSize );
	bool LoadBitFile( void* dataPtr, uint32_t dataSize, NTV2BitfileType bitFileType );

protected:
	// oem dma: save locked pages in a stl::vector
	typedef std::vector<ULWord *>	DMA_LOCKED_VEC;

	PSP_DEVICE_INTERFACE_DETAIL_DATA _pspDevIFaceDetailData;
	SP_DEVINFO_DATA _spDevInfoData;
	HDEVINFO		_hDevInfoSet;
	HANDLE			_hDevice;
	bool			_bOpenShared;
	bool			_bOpenOverlapped;
	GUID			_GUID_PROPSET;
	ULWord			_previousAudioState;
	ULWord			_previousAudioSelection;

	// OEM save locked memory addresses in vector
	DMA_LOCKED_VEC _vecDmaLocked;
	DMA_LOCKED_VEC::iterator vecIter;

};

#endif	//	NTV2WINDRIVERINTERFACE_H
