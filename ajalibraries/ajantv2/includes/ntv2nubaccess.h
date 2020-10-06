/**
	@file		ntv2nubaccess.h
	@brief		Declares functions to connect/open/close/send/receive data via the NTV2 "nub".
	@copyright	(C) 2006-2020 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#ifndef NTV2NUBACCESS_H
#define NTV2NUBACCESS_H

#include "ajaexport.h"
#include "ntv2nubtypes.h"

#define NTV2_REMOTE_ACCESS_SUCCESS						  	 0
#define NTV2_REMOTE_ACCESS_NOT_CONNECTED  				 	-1
#define NTV2_REMOTE_ACCESS_OUT_OF_MEMORY				 	-2
#define NTV2_REMOTE_ACCESS_SEND_ERR						 	-3
#define NTV2_REMOTE_ACCESS_CONNECTION_CLOSED 			 	-4
#define NTV2_REMOTE_ACCESS_RECV_ERR						 	-5
#define NTV2_REMOTE_ACCESS_TIMEDOUT				  		 	-6
#define NTV2_REMOTE_ACCESS_NO_CARD						 	-7
#define NTV2_REMOTE_ACCESS_NOT_OPEN_RESP				 	-8
#define NTV2_REMOTE_ACCESS_NON_NUB_PKT					 	-9
#define NTV2_REMOTE_ACCESS_NOT_READ_REGISTER_RESP			-10
#define NTV2_REMOTE_ACCESS_NOT_WRITE_REGISTER_RESP			-11
#define NTV2_REMOTE_ACCESS_NOT_AUTOCIRC_RESP				-12
#define NTV2_REMOTE_ACCESS_NOT_WAIT_FOR_INTERRUPT_RESP		-13
#define NTV2_REMOTE_ACCESS_WAIT_FOR_INTERRUPT_FAILED		-14
#define NTV2_REMOTE_AUTOCIRC_FAILED							-15
#define NTV2_REMOTE_ACCESS_DRIVER_GET_BITFILE_INFO_FAILED	-16
#define NTV2_REMOTE_ACCESS_NOT_DRIVER_GET_BITFILE_INFO		-17
#define NTV2_REMOTE_ACCESS_NOT_DOWNLOAD_TEST_PATTERN		-18
#define NTV2_REMOTE_ACCESS_DOWNLOAD_TEST_PATTERN_FAILED		-19
#define NTV2_REMOTE_ACCESS_READ_REG_MULTI_FAILED			-20
#define NTV2_REMOTE_ACCESS_NOT_READ_REG_MULTI				-21
#define NTV2_REMOTE_ACCESS_GET_DRIVER_VERSION_FAILED		-22
#define NTV2_REMOTE_ACCESS_NOT_GET_DRIVER_VERSION_RESP		-23
#define NTV2_REMOTE_ACCESS_READ_REG_FAILED					-24
#define NTV2_REMOTE_ACCESS_DRIVER_GET_BUILD_INFO_FAILED		-25
#define NTV2_REMOTE_ACCESS_NOT_DRIVER_GET_BUILD_INFO		-26
#define NTV2_REMOTE_ACCESS_UNIMPLEMENTED					-27

/**
	@brief	Interface to remote or fake devices.
**/
class AJAExport NTV2RPCAPI
{
	public:
		static NTV2RPCAPI *		MakeNTV2NubRPCAPI (const std::string & inSpec, const std::string & inPort = "");
		static NTV2RPCAPI *		MakeNTV2SoftwareDevice (const std::string & inSpec, const std::string & inPort = "");

	public:
						NTV2RPCAPI ()	:	_hostname()	{}
		AJA_VIRTUAL		~NTV2RPCAPI()	{}

		//	Inquiry
		AJA_VIRTUAL bool			IsConnected	(void) const			{return false;}
		AJA_VIRTUAL std::string		Name (void) const					{return _hostname;}
		AJA_VIRTUAL std::ostream &	Print (std::ostream & oss) const
									{	oss << (IsConnected()?"Connected":"Disconnected");
										if (IsConnected() && !Name().empty())
											oss << " to '" << Name() << "'";
										return oss;
									}
		AJA_VIRTUAL NTV2NubProtocolVersion	NubProtocolVersion (void) const	{return ntv2NubProtocolVersionNone;}

//		AJA_VIRTUAL int	NTV2Connect		(const std::string & inHostname, const UWord inDeviceIndex)
//																		{(void) inDeviceIndex; _hostname = inHostname;	return 0;}
		AJA_VIRTUAL int	NTV2Disconnect	(void)							{return NTV2CloseRemote();}

		AJA_VIRTUAL int	NTV2ReadRegisterRemote	(const ULWord regNum, ULWord & outRegValue, const ULWord regMask, const ULWord regShift)
												{(void) regNum;  (void) outRegValue; (void) regMask; (void) regShift; return -1;}
	
		AJA_VIRTUAL int	NTV2WriteRegisterRemote	(const ULWord regNum, const ULWord regValue, const ULWord regMask, const ULWord regShift)
												{(void) regNum; (void) regValue; (void) regMask; (void) regShift;  return -1;}
	
		AJA_VIRTUAL int	NTV2AutoCirculateRemote	(AUTOCIRCULATE_DATA & autoCircData)
												{(void) autoCircData;	return -1;}
	
		AJA_VIRTUAL int	NTV2WaitForInterruptRemote	(const INTERRUPT_ENUMS eInterrupt, const ULWord timeOutMs)
													{(void) eInterrupt; (void) timeOutMs;  return -1;}
	
		AJA_VIRTUAL int	NTV2DriverGetBitFileInformationRemote	(BITFILE_INFO_STRUCT & bitFileInfo, const NTV2BitFileType bitFileType)
																{(void) bitFileInfo; (void) bitFileType; return -1;}
	
		AJA_VIRTUAL int	NTV2DriverGetBuildInformationRemote	(BUILD_INFO_STRUCT & buildInfo)
															{(void) buildInfo;  return -1;}
	
		AJA_VIRTUAL int	NTV2DownloadTestPatternRemote	(const NTV2Channel channel, const NTV2PixelFormat testPatternFBF,
														const UWord signalMask, const bool testPatDMAEnb, const ULWord testPatNum)
														{(void) channel;  (void) testPatternFBF; (void) signalMask; (void) testPatDMAEnb; (void) testPatNum;
														return -1;}
	
		AJA_VIRTUAL int	NTV2ReadRegisterMultiRemote	(const ULWord numRegs, ULWord & outFailedRegNum, NTV2RegInfo outRegs[])
													{(void) numRegs; (void) outFailedRegNum; (void) outRegs;
													return -1;}
	
		AJA_VIRTUAL int	NTV2GetDriverVersionRemote	(ULWord & outDriverVersion)
													{outDriverVersion = 0xFFFFFFFF; return -1;}
	
		AJA_VIRTUAL int	NTV2DMATransferRemote	(const NTV2DMAEngine inDMAEngine,	const bool inIsRead,
												const ULWord inFrameNumber,			ULWord * pFrameBuffer,
												const ULWord inCardOffsetBytes,		const ULWord inTotalByteCount,
												const ULWord inNumSegments,			const ULWord inSegmentHostPitch,
												const ULWord inSegmentCardPitch,	const bool inSynchronous)
												{(void) inDMAEngine; (void) inIsRead;	(void) inFrameNumber; (void) pFrameBuffer;
												(void) inCardOffsetBytes; (void) inTotalByteCount; (void) inNumSegments; (void) inSegmentHostPitch;
												(void) inSegmentCardPitch; (void) inSynchronous;
												return -1;}
	
		AJA_VIRTUAL int	NTV2MessageRemote	(NTV2_HEADER *	pInMessage)		{(void) pInMessage;  return -1;}

	protected:
		AJA_VIRTUAL int	NTV2OpenRemote	(const UWord inDeviceIndex)		{(void) inDeviceIndex;  return -1;}
		AJA_VIRTUAL int	NTV2CloseRemote	(void)							{_hostname.clear();  return 0;}

	protected:
		std::string	_hostname;
};	//	NTV2RPCAPI

inline std::ostream & operator << (std::ostream & oss, const NTV2RPCAPI & inObj)	{return inObj.Print(oss);}

#endif	//	NTV2NUBACCESS_H
