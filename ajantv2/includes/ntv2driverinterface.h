/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2driverinterface.h
	@brief		Declares the CNTV2DriverInterface base class.
	@copyright	(C) 2004-2022 AJA Video Systems, Inc.
**/

#ifndef NTV2DRIVERINTERFACE_H
#define NTV2DRIVERINTERFACE_H

#include "ajatypes.h"
#include "ntv2enums.h"
#include "ntv2nubtypes.h"
#include "ntv2nubaccess.h"
#include "ntv2publicinterface.h"
#include "ntv2utils.h"
#include "ntv2devicefeatures.h"
#if defined(NTV2_WRITEREG_PROFILING)	//	Register Write Profiling
	#include "ajabase/system/lock.h"
#endif	//	NTV2_WRITEREG_PROFILING		Register Write Profiling
#include <string>

//	Check consistent use of AJA_USE_CPLUSPLUS11 and NTV2_USE_CPLUSPLUS11
#ifdef AJA_USE_CPLUSPLUS11
	#ifndef NTV2_USE_CPLUSPLUS11
		#error "AJA_USE_CPLUSPLUS11 && !NTV2_USE_CPLUSPLUS11"
	#else
		//#warning "AJA_USE_CPLUSPLUS11 && NTV2_USE_CPLUSPLUS11"
	#endif
#else
	#ifdef NTV2_USE_CPLUSPLUS11
		#error "!AJA_USE_CPLUSPLUS11 && NTV2_USE_CPLUSPLUS11"
	#else
		//#warning "!AJA_USE_CPLUSPLUS11 && !NTV2_USE_CPLUSPLUS11"
	#endif
#endif

#if defined(AJALinux ) || defined(AJAMac)
//	#include <sys/types.h>	//	** MrBill **	Not needed for AJALinux, needed for AJAMac?
//	#include <netinet/in.h>	//	** MrBill **	Not needed for AJALinux, needed for AJAMac?
	#include <unistd.h>	//	for usleep
#elif defined(MSWindows)
	#include <WinSock2.h>
	#include <assert.h>
#endif


#define	AsNTV2DriverInterfaceRef(_x_)	reinterpret_cast<CNTV2DriverInterface&>(_x_)
#define	AsNTV2DriverInterfacePtr(_x_)	reinterpret_cast<CNTV2DriverInterface*>(_x_)


typedef struct
{
	std::string		buildNumber;
	std::string		packageNumber;
	std::string		date;
	std::string		time;
} PACKAGE_INFO_STRUCT, *PPACKAGE_INFO_STRUCT;


/**
	@brief	I'm the base class that undergirds the platform-specific derived classes (from which ::CNTV2Card is ultimately derived).
**/
class AJAExport CNTV2DriverInterface
{
	//	STATIC (CLASS) METHODS
	public:
		static NTV2StringList	GetLegalSchemeNames (void);	///< @return	A list of legal scheme names that can be used to open remote or non-physical devices. (New in SDK 16.0)
		static inline UWord		MaxNumDevices (void)	{return 32;}	///< @return	Maximum number of local/physical device connections. (New in SDK 16.0)

		/**
			@brief		Specifies if subsequent Open calls should open the device in shared mode or not.
			@note		On some platforms, this function may have no effect.
			@param[in]	inSharedMode	Specify true for shared mode;  otherwise use false.
		**/
		static void				SetShareMode (const bool inSharedMode);
		static bool				GetShareMode (void);	///< @return	True if local devices will be opened in shared mode; otherwise false. (New in SDK 16.0)

		/**
			@brief		Specifies if the next Open call should try to open the device in shared mode or not.
			@note		On some platforms,  this function may have no effect.
			@param[in]	inOverlapMode	Specify true for overlapped mode;  otherwise use false.
		**/
		static void				SetOverlappedMode (const bool inOverlapMode);
		static bool				GetOverlappedMode (void);	///< @return	True if local devices will be opened in overlapped mode; otherwise false. (New in SDK 16.0)

	/**
		@name	Construction, destruction, assignment
	**/
	///@{
	public:
				CNTV2DriverInterface();		///< @brief	My default constructor.
		virtual	~CNTV2DriverInterface();	///< @brief	My destructor.

	private:
		/**
			@brief	My assignment operator.
			@note	We have intentionally disabled this capability because historically it was never implemented properly.
			@param[in]	inRHS	The rvalue to be assigned to the lvalue.
			@return	A non-constant reference to the lvalue.
		**/
		AJA_VIRTUAL CNTV2DriverInterface & operator = (const CNTV2DriverInterface & inRHS);

		/**
			@brief	My copy constructor.
			@note	We have intentionally disabled this capability because historically it was never implemented properly.
			@param[in]	inObjToCopy		The object to be copied.
		**/
		CNTV2DriverInterface (const CNTV2DriverInterface & inObjToCopy);
	///@}

	public:
	/**
		@name	Inquiry
	**/
	///@{
		AJA_VIRTUAL NTV2DeviceID	GetDeviceID (void);	///< @return	The 4-byte value that identifies the kind of AJA device this is.
		AJA_VIRTUAL inline UWord	GetIndexNumber (void) const	{return _boardNumber;}	///< @return	My zero-based index number (relative to other devices attached to the host).
		AJA_VIRTUAL inline bool		IsOpen (void) const			{return _boardOpened;}	///< @return	True if I'm able to communicate with the device I represent;  otherwise false.

		/**
			@return		True if the device is ready to be fully operable;  otherwise false.
			@param[in]	inCheckValid	If true, additionally checks CNTV2Card::IsMBSystemValid. Defaults to false.
			@note		Some devices have processors that require a lot of time (~30 to ~90 seconds) to start up after a PCIe bus reset,
						power-up or wake from sleep. Calls to CNTV2Card::IsOpen, CNTV2Card::ReadRegister and CNTV2Card::WriteRegister
						will all succeed, but the device won't be capable of either ingesting or playing video or performing DMA operations.
		**/
		AJA_VIRTUAL bool		IsDeviceReady (const bool inCheckValid = false);
		AJA_VIRTUAL bool		IsMBSystemValid (void);	///< @return	True if microblaze system exists (is valid); otherwise false.
		AJA_VIRTUAL bool		IsMBSystemReady (void);	///< @return	True if microblaze system is in ready state; otherwise false.
		AJA_VIRTUAL inline bool	IsIPDevice (void)	{return ::NTV2DeviceCanDoIP(GetDeviceID());}	///< @return	True if I am an IP device; otherwise false.
	///@}

	/**
		@name	Open/Close, Connect/Disconnect
	**/
	///@{
		/**
			@brief		Opens a local/physical AJA device so it can be monitored/controlled.
			@param[in]	inDeviceIndex	Specifies a zero-based index number of the AJA device to open.
			@result		True if successful; otherwise false.
			@note		Before attempting the Open, if I'm currently open, Close will be called first.
		**/
		AJA_VIRTUAL bool		Open (const UWord inDeviceIndex);

		/**
			@brief		Opens the specified local, remote or software device.
			@param[in]	inURLSpec	Specifies the local, remote or software device to be opened.
			@result		True if successful; otherwise false.
			@note		Before attempting the Open, if I'm currently open, Close will be called first.
		**/
		AJA_VIRTUAL bool		Open (const std::string & inURLSpec);

		/**
			@brief		Closes me, releasing host resources that may have been allocated in a previous Open call.
			@result		True if successful; otherwise false.
			@details	This function closes any onnection to an AJA device.
						Once closed, the device can no longer be queried or controlled by this instance.
		**/
		AJA_VIRTUAL bool		Close (void);
	///@}

	/**
		@name	Register Read/Write
	**/
	///@{

		/**
			@brief		Updates or replaces all or part of the 32-bit contents of a specific register (real or virtual)
						on the AJA device. Using the optional mask and shift parameters, it's possible to set or clear
						any number of specific bits in a real register without altering any of the register's other bits.
			@result		True if successful; otherwise false.
			@param[in]	inRegNum	Specifies the register number of interest.
			@param[in]	inValue		Specifies the desired new register value. If the "inShift" parameter is non-zero,
									this value is shifted left by the designated number of bit positions before being
									masked and applied to the real register contents.
			@param[in]	inMask		Optionally specifies a bit mask to be applied to the new (shifted) value before
									updating the register. Defaults to 0xFFFFFFFF, which does not perform any masking.
									On Windows and MacOS, a zero mask is treated the same as 0xFFFFFFFF.
			@param[in]	inShift		Optionally specifies the number of bits to left-shift the specified value before applying
									it to the register. Defaults to zero, which does not perform any shifting.
									On MacOS, this parameter is ignored.
									On Windows, a shift value of 0xFFFFFFFF is treated the same as a zero shift value.
			@note		This function should be used only when there is no higher-level function available to accomplish the desired task.
			@note		The mask and shift parameters are ignored when setting a virtual register.
			@bug		On MacOS, "holes" in the mask (i.e., one or more runs of 0-bits lying between more-significant and
						less-significant 1-bits) were not handled correctly.
		**/
		AJA_VIRTUAL bool	WriteRegister (const ULWord inRegNum,  const ULWord inValue,  const ULWord inMask = 0xFFFFFFFF,  const ULWord inShift = 0);

		/**
			@brief		Reads all or part of the 32-bit contents of a specific register (real or virtual) on the AJA device.
						Using the optional mask and shift parameters, it's possible to read any number of specific bits in a register
						while ignoring the register's other bits.
			@result		True if successful; otherwise false.
			@param[in]	inRegNum	Specifies the register number of interest.
			@param[out]	outValue	Receives the register value obtained from the device.
			@param[in]	inMask		Optionally specifies a bit mask to be applied after reading the device register.
									Zero and 0xFFFFFFFF masks are ignored. Defaults to 0xFFFFFFFF (no masking).
			@param[in]	inShift		Optionally specifies the number of bits to right-shift the value obtained
									from the device register after the mask has been applied. Defaults to zero (no shift).
			@note		This function should be used only when there is no higher-level function available to accomplish the desired task.
			@note		The mask and shift parameters are ignored when reading a virtual register.
		**/
		AJA_VIRTUAL bool	ReadRegister (const ULWord inRegNum,  ULWord & outValue,  const ULWord inMask = 0xFFFFFFFF,  const ULWord inShift = 0);

		/**
			@brief		This template function reads all or part of the 32-bit contents of a specific register (real or virtual)
						from the AJA device, and if successful, returns its value automatically casted to the scalar type of the
						"outValue" parameter.
			@result		True if successful; otherwise false.
			@param[in]	inRegNum	Specifies the register number of interest.
			@param[out]	outValue	Receives the register value obtained from the device, automatically casted to the parameter's type.
									Its type must be statically castable from ULWord (i.e. it must be a scalar).
			@param[in]	inMask		Optionally specifies a bit mask to be applied after reading the device register.
									Zero and 0xFFFFFFFF masks are ignored. Defaults to 0xFFFFFFFF (no masking).
			@param[in]	inShift		Optionally specifies the number of bits to right-shift the value obtained
									from the device register after the mask has been applied. Defaults to zero (no shift).
			@note		This function should be used only when there is no higher-level function available to accomplish the desired task.
			@note		The mask and shift parameters are ignored when reading a virtual register.
		**/
		template<typename T> bool	ReadRegister(const ULWord inRegNum,  T & outValue,  const ULWord inMask = 0xFFFFFFFF,  const ULWord inShift = 0)
									{
										ULWord regValue(0);
										bool result (ReadRegister(inRegNum, regValue, inMask, inShift));
										if (result)
											outValue = T(regValue);
										return result;
									}

#if !defined(READREGMULTICHANGE)
		/**
			@brief		Reads the register(s) specified by the given ::NTV2RegInfo sequence.
			@param[in]	inOutValues		Specifies the register(s) to be read, and upon return, contains their values.
			@return		True only if all registers were read successfully;  otherwise false.
			@note		This operation is not guaranteed to be performed atomically.
		**/
		AJA_VIRTUAL bool	ReadRegisters (NTV2RegisterReads & inOutValues);
#endif	//	!defined(READREGMULTICHANGE)
		AJA_VIRTUAL bool	RestoreHardwareProcampRegisters() = 0;
	///@}

	/**
		@name	DMA Transfer
	**/
	///@{
		/**
			@brief		Transfers data between the AJA device and the host. This function will block and not return to the caller until
						the transfer has finished or failed.
			@param[in]	inDMAEngine			Specifies the device DMA engine to use. Use NTV2_DMA_FIRST_AVAILABLE for most applications.
											(Use the ::NTV2DeviceGetNumDMAEngines function to determine how many are available.)
			@param[in]	inIsRead			Specifies the transfer direction. Use 'true' for reading (device-to-host).
											Use 'false' for writing (host-to-device).
			@param[in]	inFrameNumber		Specifies the zero-based frame number of the starting frame to be transferred to/from the device.
			@param		pFrameBuffer		Specifies a valid, non-NULL address of the host buffer. If reading (device-to-host), this memory
											will be written into. If writing (host-to-device), this memory will be read from.
			@param[in]	inCardOffsetBytes	Specifies the byte offset into the device frame buffer where the data transfer will start.
			@param[in]	inTotalByteCount	Specifies the total number of bytes to be transferred.
			@param[in]	inSynchronous		This parameter is obsolete, and ignored.
			@return		True if successful; otherwise false.
			@note		The host buffer must be at least inByteCount + inOffsetBytes in size; otherwise, host memory will be corrupted,
						or a bus error or other runtime exception may occur.
		**/
		AJA_VIRTUAL bool	DmaTransfer (const NTV2DMAEngine	inDMAEngine,
										const bool				inIsRead,
										const ULWord			inFrameNumber,
										ULWord *				pFrameBuffer,
										const ULWord			inCardOffsetBytes,
										const ULWord			inTotalByteCount,
										const bool				inSynchronous = true);

		/**
			@brief		Transfers data in segments between the AJA device and the host. This function will block
						and not return to the caller until the transfer has finished or failed.
			@param[in]	inDMAEngine			Specifies the device DMA engine to use. Use ::NTV2_DMA_FIRST_AVAILABLE
											for most applications. (Use the ::NTV2DeviceGetNumDMAEngines function
											to determine how many are available.)
			@param[in]	inIsRead			Specifies the transfer direction. Use 'true' for reading (device-to-host).
											Use 'false' for writing (host-to-device).
			@param[in]	inFrameNumber		Specifies the zero-based frame number of the starting frame to be
											transferred to/from the device.
			@param		pFrameBuffer		Specifies a valid, non-NULL address of the host buffer. If reading
											(device-to-host), this memory will be written into. If writing
											(host-to-device), this memory will be read from.
			@param[in]	inCardOffsetBytes	Specifies the byte offset into the device frame buffer where the data
											transfer will start.
			@param[in]	inTotalByteCount	Specifies the total number of bytes to be transferred.
			@param[in]	inNumSegments		Specifies the number of segments to transfer. Note that this determines
											the number of bytes per segment (by dividing into <i>inTotalByteCount</i>).
			@param[in]	inHostPitchPerSeg	Specifies the number of bytes to increment the host memory pointer
											after each segment is transferred.
			@param[in]	inCardPitchPerSeg	Specifies the number of bytes to increment the on-device memory pointer
											after each segment is transferred.
			@param[in]	inSynchronous		This parameter is obsolete, and ignored.
			@return		True if successful; otherwise false.
			@note		The host buffer must be at least inByteCount + inOffsetBytes in size; otherwise, host memory will be corrupted,
						or a bus error or other runtime exception may occur.
		**/
		AJA_VIRTUAL bool	DmaTransfer (const NTV2DMAEngine	inDMAEngine,
										const bool				inIsRead,
										const ULWord			inFrameNumber,
										ULWord *				pFrameBuffer,
										const ULWord			inCardOffsetBytes,
										const ULWord			inTotalByteCount,
										const ULWord			inNumSegments,
										const ULWord			inHostPitchPerSeg,
										const ULWord			inCardPitchPerSeg,
										const bool				inSynchronous = true)	= 0;

		AJA_VIRTUAL bool	DmaTransfer (const NTV2DMAEngine		inDMAEngine,
										const NTV2Channel			inDMAChannel,
										const bool					inTarget,
										const ULWord				inFrameNumber,
										const ULWord				inCardOffsetBytes,
										const ULWord				inByteCount,
										const ULWord				inNumSegments,
										const ULWord				inSegmentHostPitch,
										const ULWord				inSegmentCardPitch,
										const PCHANNEL_P2P_STRUCT &	inP2PData)	= 0;
	///@}

	/**
		@name	Interrupts
	**/
	///@{
		AJA_VIRTUAL bool	ConfigureSubscription (const bool bSubscribe, const INTERRUPT_ENUMS inInterruptType, PULWord & outSubcriptionHdl);
		AJA_VIRTUAL bool	ConfigureInterrupt (const bool bEnable,  const INTERRUPT_ENUMS eInterruptType) = 0;
		/**
			@brief	Answers with the number of interrupts of the given type processed by the driver.
			@param[in]	eInterrupt	The interrupt type of interest.
			@param[out]	outCount	Receives the count value.
			@return	True if successful;  otherwise false.
		**/
		AJA_VIRTUAL bool	GetInterruptCount (const INTERRUPT_ENUMS eInterrupt,  ULWord & outCount) = 0;

		AJA_VIRTUAL bool	WaitForInterrupt (const INTERRUPT_ENUMS eInterrupt, const ULWord timeOutMs = 68);

		AJA_VIRTUAL HANDLE	GetInterruptEvent (const INTERRUPT_ENUMS eInterruptType);
		/**
			@brief		Answers with the number of interrupt events that I successfully waited for.
			@param[in]	inEventCode		Specifies the interrupt of interest.
			@param[out]	outCount		Receives the number of interrupt events that I successfully waited for.
			@return		True if successful;  otherwise false.
			@see		CNTV2DriverInterface::SetInterruptEventCount, \ref fieldframeinterrupts
		**/
		AJA_VIRTUAL bool	GetInterruptEventCount (const INTERRUPT_ENUMS inEventCode, ULWord & outCount);

		/**
			@brief		Resets my interrupt event tally for the given interrupt type. (This is my count of the number of successful event waits.)
			@param[in]	inEventCode		Specifies the interrupt type.
			@param[in]	inCount			Specifies the new count value. Use zero to reset the tally.
			@return		True if successful;  otherwise false.
			@see		CNTV2DriverInterface::GetInterruptEventCount, \ref fieldframeinterrupts
		**/
		AJA_VIRTUAL bool	SetInterruptEventCount (const INTERRUPT_ENUMS inEventCode, const ULWord inCount);
	///@}

	/**
		@name	Control/Messaging
	**/
	///@{
		/**
			@brief	Sends an AutoCirculate command to the NTV2 driver.
			@param	pAutoCircData	Points to the AUTOCIRCULATE_DATA that contains the AutoCirculate API command and parameters.
			@return	True if successful;  otherwise false.
		**/
		AJA_VIRTUAL bool		AutoCirculate (AUTOCIRCULATE_DATA & pAutoCircData);

		/**
			@brief	Sends a message to the NTV2 driver (the new, improved, preferred way).
			@param	pInMessage	Points to the message to pass to the driver.
								Valid messages start with an NTV2_HEADER and end with an NTV2_TRAILER.
			@return	True if successful;  otherwise false.
		**/
		AJA_VIRTUAL bool		NTV2Message (NTV2_HEADER * pInMessage);

		/**
			@brief	Sends an HEVC message to the NTV2 driver.
			@param	pMessage	Points to the HevcMessageHeader that contains the HEVC message.
			@return	False. This must be implemented by the platform-specific subclass.
		**/
		AJA_VIRTUAL inline bool	HevcSendMessage (HevcMessageHeader * pMessage)		{(void) pMessage; return false;}

		AJA_VIRTUAL bool	ControlDriverDebugMessages (NTV2_DriverDebugMessageSet msgSet,  bool enable) = 0;
	///@}

		AJA_VIRTUAL inline ULWord	GetNumFrameBuffers (void) const				{return _ulNumFrameBuffers;}
		AJA_VIRTUAL inline ULWord	GetFrameBufferSize (void) const				{return _ulFrameBufferSize;}

		/**
			@brief		Answers with the currently-installed bitfile information.
			@param[out]	outBitFileInfo	Receives the bitfile info.
			@param[in]	inBitFileType	Optionally specifies the bitfile type of interest. Defaults to NTV2_VideoProcBitFile.
			@return		True if successful;  otherwise false.
		**/
		AJA_VIRTUAL bool DriverGetBitFileInformation (BITFILE_INFO_STRUCT & outBitFileInfo,  const NTV2BitFileType inBitFileType = NTV2_VideoProcBitFile);

		/**
			@brief		Answers with the driver's build information.
			@param[out]	outBuildInfo	Receives the build information.
			@return		True if successful;  otherwise false.
		**/
		AJA_VIRTUAL bool DriverGetBuildInformation (BUILD_INFO_STRUCT & outBuildInfo);

		/**
			@brief		Answers with the IP device's package information.
			@param[out]	outPkgInfo	Receives the package information.
			@return		True if successful;  otherwise false.
		**/
		AJA_VIRTUAL bool GetPackageInformation (PACKAGE_INFO_STRUCT & outPkgInfo);

		AJA_VIRTUAL bool BitstreamWrite (const NTV2Buffer & inBuffer, const bool inFragment, const bool inSwap);
		AJA_VIRTUAL bool BitstreamReset (const bool inConfiguration, const bool inInterface);
		AJA_VIRTUAL bool BitstreamStatus (NTV2ULWordVector & outRegValues);
		AJA_VIRTUAL bool BitstreamLoad (const bool inSuspend, const bool inResume);

	/**
		@name	Device Features
	**/
	///@{
		/**
			@return		True if the requested device feature is supported.
			@param[in]	inParamID	The NTV2BoolParamID of interest.
		**/
		AJA_VIRTUAL bool		IsSupported (const NTV2BoolParamID inParamID)	//	New in SDK 17.0
									{	ULWord value(0);
										GetBoolParam (ULWord(inParamID), value);
										return bool(value);
									}
		/**
			@return		The requested quantity for the given device feature.
			@param[in]	inParamID	The NTV2NumericParamID of interest.
		**/
		AJA_VIRTUAL ULWord		GetNumSupported (const NTV2NumericParamID inParamID)	//	New in SDK 17.0
									{	ULWord value(0);
										GetNumericParam (ULWord(inParamID), value);
										return value;
									}

		/**
			@param[in]	inEnumsID	The NTV2EnumsID of interest.
			@return		The supported items.
		**/
		AJA_VIRTUAL ULWordSet	GetSupportedItems (const NTV2EnumsID inEnumsID);	//	New in SDK 17.0
	///@}

		// stream channel operations
		AJA_VIRTUAL bool	StreamChannelOps (const NTV2Channel inChannel,
												ULWord flags,
												NTV2StreamChannel& status);

		// stream buffer operations
		AJA_VIRTUAL bool	StreamBufferOps (const NTV2Channel inChannel,
												NTV2_POINTER inBuffer,
												ULWord64 bufferCookie,
												ULWord flags,
												NTV2StreamBuffer& status);
    
	/**
		@name	Device Ownership
	**/
	///@{
		/**
			@brief		A reference-counted version of CNTV2DriverInterface::AcquireStreamForApplication useful for
						process groups.
			@result		True if successful; otherwise false.
			@param[in]	inAppType		A 32-bit "four CC" value that helps identify the calling application that is
										requesting exclusive use of the device.
			@param[in]	inProcessID		Specifies the OS-specific process identifier that uniquely identifies the running
										process on the host machine that is requesting exclusive use of the device
										(see AJAProcess::GetPid).
			@details	This method reserves exclusive use of the AJA device by the given running host process,
						recording both the "process ID" and 4-byte "application type" specified by the caller, and if
						already reserved for the given application, increments a reference counter. If a different host
						process has already reserved the device, the function will fail.
			@note		Each call to CNTV2DriverInterface::AcquireStreamForApplicationWithReference should be balanced by
						the same number of calls to CNTV2DriverInterface::ReleaseStreamForApplicationWithReference.
			@see		CNTV2DriverInterface::ReleaseStreamForApplicationWithReference, \ref devicesharing
		**/
		AJA_VIRTUAL bool AcquireStreamForApplicationWithReference (const ULWord inAppType, const int32_t inProcessID);

		/**
			@brief		A reference-counted version of CNTV2DriverInterface::ReleaseStreamForApplication useful for
						process groups.
			@result		True if successful; otherwise false.
			@param[in]	inAppType		A 32-bit "four CC" value that helps identify the calling application that is
										releasing exclusive use of the device.
			@param[in]	inProcessID		Specifies the OS-specific process identifier that uniquely identifies the running
										process on the host machine that is releasing exclusive use of the device
										(see AJAProcess::GetPid).
			@details	This method releases exclusive use of the AJA device by the given host process once the reference
						count goes to zero. This method will fail if the specified application type or process ID values
						don't match those used in the prior call to CNTV2DriverInterface::AcquireStreamForApplication.
			@note		Each call to CNTV2DriverInterface::AcquireStreamForApplicationWithReference should be balanced by
						the same number of calls to CNTV2DriverInterface::ReleaseStreamForApplicationWithReference.
			@see		CNTV2DriverInterface::AcquireStreamForApplicationWithReference, \ref devicesharing
		**/
		AJA_VIRTUAL bool ReleaseStreamForApplicationWithReference (const ULWord inAppType, const int32_t inProcessID);

		/**
			@brief		Reserves exclusive use of the AJA device for a given process, preventing other processes on the host
						from acquiring it until subsequently released.
			@result		True if successful; otherwise false.
			@param[in]	inAppType		A 32-bit "four CC" value that helps identify the calling application that is
										requesting exclusive use of the device.
			@param[in]	inProcessID		Specifies the OS-specific process identifier that uniquely identifies the running
										process on the host machine that is requesting exclusive use of the device
										(see AJAProcess::GetPid).
			@details	This method reserves exclusive use of the AJA device by the given running host process.
						The AJA device records both the "process ID" and "four CC". If another host process has already
						reserved the device, this function will fail.
			@note		AJA recommends saving the device's ::NTV2EveryFrameTaskMode when this function is called, and
						restoring it after CNTV2DriverInterface::ReleaseStreamForApplication is called.
			@note		A call to CNTV2DriverInterface::AcquireStreamForApplication should always be balanced by a call to
						CNTV2DriverInterface::ReleaseStreamForApplication.
			@see		CNTV2DriverInterface::ReleaseStreamForApplication, \ref devicesharing
		**/
		AJA_VIRTUAL bool AcquireStreamForApplication (const ULWord inAppType, const int32_t inProcessID);

		/**
			@brief		Releases exclusive use of the AJA device for the given process, permitting other processes to
						acquire it.
			@result		True if successful; otherwise false.
			@param[in]	inAppType		A 32-bit "four CC" value that helps identify the application that is releasing
										its exclusive use of the device.
			@param[in]	inProcessID		Specifies the OS-specific process identifier that uniquely identifies the running
										process on the host machine that is releasing its exclusive use of the device
										(see AJAProcess::GetPid).
			@details	This method will fail if the specified application type or process ID values don't match those used
						in the previous call to CNTV2DriverInterface::AcquireStreamForApplication.
			@note		AJA recommends saving the device's ::NTV2EveryFrameTaskMode at the time
						CNTV2DriverInterface::AcquireStreamForApplication is called, and restoring it after releasing
						the device.
			@see		CNTV2DriverInterface::AcquireStreamForApplication, \ref devicesharing
		**/
		AJA_VIRTUAL bool ReleaseStreamForApplication (const ULWord inAppType, const int32_t inProcessID);

		/**
			@brief		Sets the four-CC type and process ID of the application that should "own" the AJA device
						(i.e. reserve it for exclusive use).
			@result		True if successful; otherwise false.
			@param[in]	inAppType		Specifies the 32-bit "four CC" value that helps identify the application that will
										"own" the device.
			@param[in]	inProcessID		Specifies the OS-specific process identifier of the host application process that
										will "own" the device (see AJAProcess::GetPid).
			@note		This function should only be used to forcibly set (or reset) the owning application after a crash or
						other abnormal termination. In normal use, AJA recommends calling CNTV2DriverInterface::AcquireStreamForApplication
						and CNTV2DriverInterface::ReleaseStreamForApplication (or their reference-counted equivalents).
			@see		CNTV2DriverInterface::GetStreamingApplication, \ref devicesharing
		**/
		AJA_VIRTUAL bool SetStreamingApplication (const ULWord inAppType, const int32_t inProcessID);

		/**
			@brief		Answers with the four-CC type and process ID of the application that currently "owns" the AJA device
						(i.e. reserved it for exclusive use).
			@result		True if successful; otherwise false.
			@param[out]	outAppType		Receives the 32-bit "four CC" value of the application that currently "owns" the device.
			@param[out]	outProcessID	Receives the OS-specific process identifier that uniquely identifies the running
										application process on the host machine that currently "owns" the device
										(see AJAProcess::GetPid).
			@details	If the AJA retail agent (service) is controlling the device, the returned process ID will be zero,
						and the returned app type will be <tt>NTV2_FOURCC('A','j','a','A')</tt>.
			@see		CNTV2DriverInterface::SetStreamingApplication, \ref devicesharing
		**/
		AJA_VIRTUAL bool GetStreamingApplication (ULWord & outAppType, int32_t & outProcessID);
	///@}

		AJA_VIRTUAL bool				ReadRP188Registers (const NTV2Channel inChannel, RP188_STRUCT * pRP188Data);
		AJA_VIRTUAL inline std::string	GetHostName (void) const	{return IsRemote() ? _pRPCAPI->Name() : "";}	///< @return	String containing the remote device host name (if any).
		AJA_VIRTUAL inline bool			IsRemote (void) const		{return _pRPCAPI ? true : false;}	///< @return	True if I'm connected to a non-local or non-physical device;  otherwise false.
		/**
			@return		String containing remote device description.
		**/
		AJA_VIRTUAL inline std::string	GetDescription (void) const	{return IsRemote() ? _pRPCAPI->Description() : "";}	//	New in SDK 17.0
#if defined(NTV2_NUB_CLIENT_SUPPORT)  &&  !defined(NTV2_DEPRECATE_16_0)
		AJA_VIRTUAL inline NTV2NubProtocolVersion	GetNubProtocolVersion (void) const	{return 0;}	///< @return	My nub protocol version.
#endif

		//	DEPRECATED FUNCTIONS
#if !defined(NTV2_DEPRECATE_16_0)
	// SuspendAudio/ResumeAudio were only implemented on MacOS
	AJA_VIRTUAL inline NTV2_SHOULD_BE_DEPRECATED(bool SuspendAudio(void))	{return true;}
	AJA_VIRTUAL inline NTV2_SHOULD_BE_DEPRECATED(bool ResumeAudio(const ULWord inFBSize))	{(void) inFBSize; return true;}
	//	Memory Mapping/Unmapping
	AJA_VIRTUAL inline NTV2_DEPRECATED_f(bool MapFrameBuffers(void))	{return false;}	///< @deprecated	Obsolete starting in SDK 16.0.
	AJA_VIRTUAL inline NTV2_DEPRECATED_f(bool UnmapFrameBuffers(void))	{return true;}	///< @deprecated	Obsolete starting in SDK 16.0.
	AJA_VIRTUAL inline NTV2_DEPRECATED_f(bool MapRegisters(void))		{return false;}	///< @deprecated	Obsolete starting in SDK 16.0.
	AJA_VIRTUAL inline NTV2_DEPRECATED_f(bool UnmapRegisters(void))		{return true;}	///< @deprecated	Obsolete starting in SDK 16.0.
	AJA_VIRTUAL inline NTV2_DEPRECATED_f(bool MapXena2Flash(void))		{return false;}	///< @deprecated	Obsolete starting in SDK 16.0.
	AJA_VIRTUAL inline NTV2_DEPRECATED_f(bool UnmapXena2Flash(void))	{return true;}	///< @deprecated	Obsolete starting in SDK 16.0.
	//	Others
	AJA_VIRTUAL inline NTV2_DEPRECATED_f(bool DmaUnlock(void))	{return false;}	///< @deprecated	Obsolete starting in SDK 16.0.
	AJA_VIRTUAL inline NTV2_DEPRECATED_f(bool CompleteMemoryForDMA(ULWord * pHostBuffer))	{(void)pHostBuffer; return false;}	///< @deprecated	Obsolete starting in SDK 16.0.
	AJA_VIRTUAL inline NTV2_DEPRECATED_f(bool PrepareMemoryForDMA(ULWord * pHostBuffer, const ULWord inNumBytes))	{(void)pHostBuffer; (void)inNumBytes; return false;}	///< @deprecated	Obsolete starting in SDK 16.0.
	AJA_VIRTUAL inline NTV2_SHOULD_BE_DEPRECATED(bool GetInterruptCount(const INTERRUPT_ENUMS eInt, ULWord *pCnt))	{return pCnt ? GetInterruptCount(eInt, *pCnt) : false;}	///< @deprecated	Use version of this function that accepts a non-const reference.
	AJA_VIRTUAL NTV2_SHOULD_BE_DEPRECATED(bool ReadRegisterMulti(const ULWord numRegs, ULWord * pOutWhichRegFailed, NTV2RegInfo aRegs[]));	///< @deprecated	Use CNTV2DriverInterface::ReadRegisters instead.
	AJA_VIRTUAL inline NTV2_SHOULD_BE_DEPRECATED(ULWord	GetPCISlotNumber(void) const)	{return _pciSlot;}			///< @deprecated	Obsolete starting in SDK 16.0.
	AJA_VIRTUAL NTV2_SHOULD_BE_DEPRECATED(Word SleepMs(const LWord msec));	///< @deprecated	Obsolete starting in SDK 16.0. Use AJATime::Sleep instead.
	AJA_VIRTUAL inline NTV2_SHOULD_BE_DEPRECATED(ULWord	GetAudioFrameBufferNumber(void) const)	{return GetNumFrameBuffers() - 1;}	///< @deprecated	Obsolete starting in SDK 16.0.
#endif	//	!defined(NTV2_DEPRECATE_16_0)
#if !defined(NTV2_DEPRECATE_16_3)
	AJA_VIRTUAL inline NTV2_DEPRECATED_f(bool SetDefaultDeviceForPID(const int32_t procID)) {(void)procID; return false;}	///< @deprecated	Obsolete, first deprecated in SDK 14.3 when classic Apple QuickTime support was dropped.
	AJA_VIRTUAL inline NTV2_DEPRECATED_f(bool IsDefaultDeviceForPID(const int32_t procID))  {(void)procID; return false;}	///< @deprecated	Obsolete, first deprecated in SDK 14.3 when classic Apple QuickTime support was dropped.
#endif	//	!defined(NTV2_DEPRECATE_16_3)

#if defined(NTV2_WRITEREG_PROFILING)	//	Register Write Profiling
		/**
			@name	WriteRegister Profiling
		**/
		///@{
		AJA_VIRTUAL bool	GetRecordedRegisterWrites (NTV2RegisterWrites & outRegWrites) const;	///< @brief	Answers with the recorded register writes.
		AJA_VIRTUAL bool	StartRecordRegisterWrites (const bool inSkipActualWrites = false);	///< @brief	Starts recording all WriteRegister calls.
		AJA_VIRTUAL bool	IsRecordingRegisterWrites (void) const;		///< @return	True if WriteRegister calls are currently being recorded (and not paused);  otherwise false.
		AJA_VIRTUAL bool	StopRecordRegisterWrites (void);			///< @brief		Stops recording all WriteRegister calls.
		AJA_VIRTUAL bool	PauseRecordRegisterWrites (void);			///< @brief		Pauses recording WriteRegister calls.
		AJA_VIRTUAL bool	ResumeRecordRegisterWrites (void);			///< @brief		Resumes recording WriteRegister calls (after a prior call to PauseRecordRegisterWrites).
		AJA_VIRTUAL ULWord	GetNumRecordedRegisterWrites (void) const;	///< @return	The number of recorded WriteRegister calls.
		///@}
#endif	//	NTV2_WRITEREG_PROFILING		//	Register Write Profiling


	//	PROTECTED METHODS
	protected:
		/**
			@brief		Peforms the housekeeping details of opening the specified local, remote or software device.
			@param[in]	inURLSpec	Specifies the local, remote or software device to be opened.
			@result		True if successful; otherwise false.
		**/
		AJA_VIRTUAL bool	OpenRemote (const std::string & inURLSpec);
		AJA_VIRTUAL bool	CloseRemote (void);	///< @brief	Releases host resources associated with the remote/special device connection.
		AJA_VIRTUAL bool	OpenLocalPhysical (const UWord inDeviceIndex);	///< @brief	Opens the local/physical device connection.
		AJA_VIRTUAL bool	CloseLocalPhysical (void);	///< @brief	Releases host resources associated with the local/physical device connection.
		AJA_VIRTUAL bool	ParseFlashHeader (BITFILE_INFO_STRUCT & outBitfileInfo);
		AJA_VIRTUAL bool	GetBoolParam (const ULWord inParamID,  ULWord & outValue);	//	New in SDK 17.0
		AJA_VIRTUAL bool	GetNumericParam (const ULWord inParamID,  ULWord & outValue);	//	New in SDK 17.0

		/**
			@brief		Answers with the NTV2RegInfo of the register associated with the given boolean (i.e., "Can Do") device feature.
			@param[in]	inParamID		Specifies the device features parameter of interest.
			@param[out] outRegInfo		Receives the associated NTV2RegInfo.
			@return		True if successful; otherwise false.
		**/
		AJA_VIRTUAL bool	GetRegInfoForBoolParam (const NTV2BoolParamID inParamID, NTV2RegInfo & outRegInfo);
		/**
			@brief		Answers with the NTV2RegInfo of the register associated with the given numeric (i.e., "Get Num") device feature.
			@param[in]	inParamID		Specifies the device features parameter of interest.
			@param[out] outRegInfo		Receives the associated NTV2RegInfo.
			@return		True if successful; otherwise false.
		**/
		AJA_VIRTUAL bool	GetRegInfoForNumericParam (const NTV2NumericParamID inParamID, NTV2RegInfo & outRegInfo);

		/**
			@brief		Atomically increments the event count tally for the given interrupt type.
			@param[in]	eInterruptType	Specifies the interrupt type of interest.
		**/
		AJA_VIRTUAL void	BumpEventCount (const INTERRUPT_ENUMS eInterruptType);

		/**
			@brief		Initializes my member variables after a successful Open.
		**/
		AJA_VIRTUAL void	FinishOpen (void);
		AJA_VIRTUAL bool	ReadFlashULWord (const ULWord inAddress, ULWord & outValue, const ULWord inRetryCount = 1000);


	//	PRIVATE TYPES
	protected:
		typedef std::vector<ULWord>		_EventCounts;
		typedef std::vector<PULWord>	_EventHandles;


	//	MEMBER DATA
	protected:
		UWord				_boardNumber;			///< @brief	My device index number.
		NTV2DeviceID		_boardID;				///< @brief	My cached device ID.
		bool				_boardOpened;			///< @brief	True if I'm open and connected to the device.
#if defined(NTV2_WRITEREG_PROFILING)
		bool				mRecordRegWrites;		///< @brief	True if recording; otherwise false when not recording
		bool				mSkipRegWrites;			///< @brief	True if actual register writes are skipped while recording
#endif	//	NTV2_WRITEREG_PROFILING
		ULWord				_programStatus;
		NTV2RPCAPI *		_pRPCAPI;				///< @brief	Points to remote or software device interface; otherwise NULL for local physical device.
		_EventHandles		mInterruptEventHandles;	///< @brief	For subscribing to each possible event, one for each interrupt type
		_EventCounts		mEventCounts;			///< @brief	My event tallies, one for each interrupt type. Note that these
#if defined(NTV2_WRITEREG_PROFILING)
		NTV2RegisterWrites	mRegWrites;				///< @brief	Stores WriteRegister data
		mutable AJALock		mRegWritesLock;			///< @brief	Guard mutex for mRegWrites
#endif	//	NTV2_WRITEREG_PROFILING
#if !defined(NTV2_DEPRECATE_16_0)
		ULWord *			_pFrameBaseAddress;			///< @deprecated	Obsolete starting in SDK 16.0.
		ULWord *			_pRegisterBaseAddress;		///< @deprecated	Obsolete starting in SDK 16.0.
		ULWord				_pRegisterBaseAddressLength;///< @deprecated	Obsolete starting in SDK 16.0.
		ULWord *			_pXena2FlashBaseAddress;	///< @deprecated	Obsolete starting in SDK 16.0.
		ULWord *			_pCh1FrameBaseAddress;		///< @deprecated	Obsolete starting in SDK 16.0.
		ULWord *			_pCh2FrameBaseAddress;		///< @deprecated	Obsolete starting in SDK 16.0.
#endif	//	!defined(NTV2_DEPRECATE_16_0)
		ULWord				_ulNumFrameBuffers;
		ULWord				_ulFrameBufferSize;
		ULWord				_pciSlot;					//	DEPRECATE!

};	//	CNTV2DriverInterface

#endif	//	NTV2DRIVERINTERFACE_H
