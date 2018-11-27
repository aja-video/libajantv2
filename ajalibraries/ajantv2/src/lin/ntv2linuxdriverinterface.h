////////////////////////////////////////////////////////////
//
//	Copyright (C) 2003, 2004, 2005, 2006, 2007, 2009, 2011 AJA Video Systems, Inc.
//	Proprietary and Confidential information.
//
////////////////////////////////////////////////////////////
#ifndef NTV2LINUXDRIVERINTERFACE_H
#define NTV2LINUXDRIVERINTERFACE_H

#include "ntv2driverinterface.h"
#include "ntv2linuxpublicinterface.h"
#include "ntv2devicefeatures.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <string>

#define CopyMemory(a,b,c) memcpy((a),(b),(c))

// oem dma: save locked pages in a stl::vector
#include <vector>
using namespace std;
typedef vector<ULWord *> DMA_LOCKED_VEC;


class CNTV2LinuxDriverInterface : public CNTV2DriverInterface
{
public:
	CNTV2LinuxDriverInterface();
	virtual ~CNTV2LinuxDriverInterface();

public:
	// Interfaces
	// Call this method to set the Open share mode
	bool SetShareMode (bool bShared);

	bool SetOverlappedMode (bool bOverlapped);

	bool Open (const UWord inDeviceIndex = 0,
			  const std::string & inHostName = std::string());
#if !defined(NTV2_DEPRECATE_14_3)
	bool Open(	UWord			boardNumber,
		  		bool			displayError,
				NTV2DeviceType	eBoardType,
				const char 		*hostname);   // Non-null: card on remote host
#endif	//	!defined(NTV2_DEPRECATE_14_3)


	bool Close();

	bool WriteRegister(ULWord registerNumber,
					   ULWord registerValue,
					   ULWord registerMask = 0xFFFFFFFF,
					   ULWord registerShift = 0);

	virtual bool		ReadRegister (const ULWord inRegisterNumber,
									  ULWord & outValue,
									  const ULWord inRegisterMask = 0xFFFFFFFF,
									  const ULWord inRegisterShift = 0x0);
#if !defined(NTV2_DEPRECATE_14_3)
	virtual inline NTV2_DEPRECATED_f(bool	ReadRegister (const ULWord inRegNum, ULWord * pOutValue, const ULWord inRegMask = 0xFFFFFFFF, const ULWord inRegShift = 0))
	{
		return pOutValue ? ReadRegister(inRegNum, *pOutValue, inRegMask, inRegShift) : false;
	}
#endif	//	!defined(NTV2_DEPRECATE_14_3)

	bool RestoreHardwareProcampRegisters();

    bool DmaTransfer (NTV2DMAEngine	DMAEngine,
					  bool			bRead,
					  ULWord		frameNumber,
					  ULWord		* pFrameBuffer,
					  ULWord		offsetBytes,
					  ULWord		bytes,
					  bool			bSync = true);

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

	virtual bool DmaUnlock   (void)
		{ return false; }

	virtual bool CompleteMemoryForDMA (ULWord * pFrameBuffer)
		{ (void) pFrameBuffer; return false; }

	virtual bool PrepareMemoryForDMA (ULWord * pFrameBuffer, ULWord ulNumBytes)
		{ (void) pFrameBuffer; (void) ulNumBytes; return false; }



	bool ConfigureInterrupt (bool bEnable,
							 INTERRUPT_ENUMS eInterruptType);

	bool MapFrameBuffers (void);

	bool UnmapFrameBuffers (void);

	bool MapRegisters (void);

	bool UnmapRegisters (void);

	bool MapXena2Flash (void);

	bool UnmapXena2Flash (void);

	bool MapDNXRegisters (void);

	bool UnmapDNXRegisters (void);

	bool ConfigureSubscription (bool			bSubscribe,
								INTERRUPT_ENUMS	eInterruptType,
								PULWord			& hSubcription)
		{ (void) bSubscribe; (void) eInterruptType; (void) hSubcription; return true; /* No subscriptions on Linux */ }

	bool GetInterruptCount (INTERRUPT_ENUMS eInterrupt,
							ULWord			*pCount);

	// default of 68 ms timeout is enough time for 2K at 14.98 HZ
	bool WaitForInterrupt (INTERRUPT_ENUMS eInterrupt, ULWord timeOutMs = 68);

	bool AutoCirculate (AUTOCIRCULATE_DATA &autoCircData);

	virtual bool NTV2Message (NTV2_HEADER * pInOutMessage);

	bool ControlDriverDebugMessages(NTV2_DriverDebugMessageSet msgSet,
		  							bool enable);

	bool SetupBoard(void);

	// Driver allocated buffer (DMA performance enhancement, requires
	// bigphysarea patch to kernel)

	bool MapDMADriverBuffer();

	bool UnmapDMADriverBuffer();

	bool DmaReadFrameDriverBuffer(
		 	NTV2DMAEngine DMAEngine,
			ULWord frameNumber,
			unsigned long dmaBufferFrame,
			ULWord bytes,
			ULWord downSample,
			ULWord linePitch,
			ULWord poll);

	bool DmaReadFrameDriverBuffer(
		 	NTV2DMAEngine DMAEngine,
			ULWord frameNumber,
			unsigned long dmaBufferFrame,
			ULWord offsetSrc,
			ULWord offsetDest,
			ULWord bytes,
			ULWord downSample,
			ULWord linePitch,
			ULWord poll);

	bool DmaWriteFrameDriverBuffer(
		  	NTV2DMAEngine DMAEngine,
			ULWord frameNumber,
			unsigned long dmaBufferFrame,
			ULWord bytes,
			ULWord poll);

	bool DmaWriteFrameDriverBuffer(
		  	NTV2DMAEngine DMAEngine,
			ULWord frameNumber,
			unsigned long dmaBufferFrame,
			ULWord offsetSrc,
			ULWord offsetDest,
			ULWord bytes,
			ULWord poll);

	// User allocated buffer methods.  Not as fast as driverbuffer
	// methods but no kernel patch required.

	bool DmaWriteWithOffsets(
			NTV2DMAEngine DMAEngine,
			ULWord frameNumber,
			ULWord * pFrameBuffer,
			ULWord offsetSrc,
			ULWord offsetDest,
        	ULWord bytes);

    bool DmaReadWithOffsets(
			NTV2DMAEngine DMAEngine,
			ULWord frameNumber,
			ULWord * pFrameBuffer,
			ULWord offsetSrc,
			ULWord offsetDest,
        	ULWord bytes);

public:
//
//  virtual register access.
//
    bool SetRelativeVideoPlaybackDelay (ULWord frameDelay);		// Not supported
    bool GetRelativeVideoPlaybackDelay (ULWord* frameDelay);	// Not supported
    bool SetAudioPlaybackPinDelay (ULWord millisecondDelay);	// Not supported
    bool GetAudioPlaybackPinDelay (ULWord* millisecondDelay);	// Not supported
    bool SetAudioRecordPinDelay (ULWord millisecondDelay);		// Not supported
    bool GetAudioRecordPinDelay (ULWord* millisecondDelay);		// Not supported
	bool GetBA0MemorySize(ULWord* memSize);						// Supported!
	bool GetBA1MemorySize(ULWord* memSize);						// Supported!
	bool GetBA2MemorySize(ULWord* memSize);						// Supported!
	bool GetBA4MemorySize(ULWord* memSize);						// Supported! XENA2 PCI Flash
	bool GetDMADriverBufferPhysicalAddress(ULWord* physAddr);	// Supported!
	bool GetDMADriverBufferAddress(ULWord** pDMADriverBuffer);	// Supported!
	bool GetDMANumDriverBuffers(ULWord* pNumDmaDriverBuffers);	// Supported!
	bool SetAudioOutputMode(NTV2_GlobalAudioPlaybackMode mode);	// Supported!
	bool GetAudioOutputMode(NTV2_GlobalAudioPlaybackMode* mode);// Supported!

	bool DisplayNTV2Error (const char *str);

	Word SleepMs(LWord msec);

	// These are only implemented in Mac code but need to be here to satisfy Win/Linux build
	bool		SuspendAudio () { return true; }
	bool		ResumeAudio (ULWord frameBufferSize) { (void) frameBufferSize; return true; }

	// Control panel support
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
		@param[in]	appType			A 32-bit value that uniquely and globally identifies the calling application
									or process that is requesting exclusive use of the device.
		@param[in]	pid				Specifies the OS-specific process identifier that uniquely identifies the running
									process on the host machine that is requesting exclusive use of the device.
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

	bool GetLastOutputVerticalTimestamp( NTV2Channel channel, uint64_t* pTimestamp );
	bool GetLastInputVerticalTimestamp( NTV2Channel channel, uint64_t* pTimestamp );

	bool HevcSendMessage(HevcMessageHeader* pMessage);

protected:	//	INSTANCE DATA
 	std::string		_bitfileDirectory;

	HANDLE			_hDevice;
	bool			_bOpenShared;

	ULWord*			_pDMADriverBufferAddress;
	ULWord			_BA0MemorySize;

	// KiPro Mini only
	ULWord*			_pDNXRegisterBaseAddress;
	ULWord			_BA2MemorySize;

	ULWord*			_pXena2FlashBaseAddress; /* PCI Flash base */
	ULWord			_BA4MemorySize;	         /* XENA2 only */
};

#endif	//	NTV2LINUXDRIVERINTERFACE_H
