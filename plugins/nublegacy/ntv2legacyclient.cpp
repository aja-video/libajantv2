/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2legacyclient.cpp
	@brief		Legacy nub implementation of NTV2RPCClientAPI.
	@copyright	(C) 2019-2022 AJA Video Systems, Inc.
**/
#include "ajatypes.h"
#include "ntv2nubaccess.h"
#include "ntv2endian.h"
#include "ntv2publicinterface.h"
#include "ntv2utils.h"
#include "ntv2legacycommon.h"
#include "ajabase/system/debug.h"
#include "ajabase/common/common.h"
#include "ajabase/system/atomic.h"
#include "ajabase/system/memory.h"
#include "ajabase/system/lock.h"
#include "ajabase/system/info.h"
#include <fstream>
#include <iomanip>
#if defined(AJAMac)
	#include <CoreFoundation/CoreFoundation.h>
	#include <dlfcn.h>
#endif
#if defined(AJALinux ) || defined(AJAMac)
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <netdb.h>
	#include <unistd.h>
#elif defined(MSWindows)
	//#include <WinSock.h>
	#include <WinSock2.h>
#endif
#if defined(AJAMac)
	#include <CoreFoundation/CoreFoundation.h>
	#include <dlfcn.h>
#endif

using namespace std;

#define INSTP(_p_)			xHEX0N(uint64_t(_p_),16)
#define	NBFAIL(__x__)		AJA_sERROR  (AJA_DebugUnit_RPCClient, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define	NBWARN(__x__)		AJA_sWARNING(AJA_DebugUnit_RPCClient, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define	NBNOTE(__x__)		AJA_sNOTICE (AJA_DebugUnit_RPCClient, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define	NBINFO(__x__)		AJA_sINFO   (AJA_DebugUnit_RPCClient, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define	NBDBG(__x__)		AJA_sDEBUG  (AJA_DebugUnit_RPCClient, INSTP(this) << "::" << AJAFUNC << ": " << __x__)

#define	AsIOMemoryDescriptor(_f_)		(reinterpret_cast <IOMemoryDescriptor *> (_f_))
#define	AsIOMemoryMap(_f_)				(reinterpret_cast <IOMemoryMap *> (_f_))
#define	AsAUTOCIRCULATE_STATUS(_p_)		(reinterpret_cast <AUTOCIRCULATE_STATUS *> (_p_))
#define	AsAUTOCIRCULATE_TRANSFER(_p_)	(reinterpret_cast <AUTOCIRCULATE_TRANSFER *> (_p_))
#define	AsNTV2GetRegisters(_p_)			(reinterpret_cast <NTV2GetRegisters *> (_p_))
#define	AsNTV2SetRegisters(_p_)			(reinterpret_cast <NTV2SetRegisters *> (_p_))
#define	AsNTV2BankSelGetSetRegs(_p_)	(reinterpret_cast <NTV2BankSelGetSetRegs *> (_p_))
#define	AsNTV2NTV2VirtualData(_p_)      (reinterpret_cast <NTV2VirtualData *> (_p_))
#define	AsFRAME_STAMP(_p_)				(reinterpret_cast <FRAME_STAMP *> (_p_))
#define	AsNTV2BufferLock(_p_)      		(reinterpret_cast <NTV2BufferLock *> (_p_))

static ULWord gIgnoredNTV2pkts(0);





static bool isNubOpenRespPacket (NTV2NubPkt *pPkt)
{
	return isNTV2NubPacketType(pPkt, eNubOpenRespPkt);
}

static void * getNubPktPayload (NTV2NubPkt *pPkt)
{
	NTV2NubPktType pktType(pPkt->hdr.pktType);
	NTV2NubProtocolVersion protocolVersion(pPkt->hdr.protocolVersion);
	const char *queryRespStr (nubQueryRespStr(protocolVersion, pktType));
	return reinterpret_cast<char*>(pPkt->data) + strlen(queryRespStr) + 1;
}

static NTV2NubPkt * BuildOpenQueryPacket (ULWord inDeviceIndex)
{
	char *p(AJA_NULL);
	NTV2NubPkt *pPkt(BuildNubBasePacket (maxKnownProtocolVersion, eNubOpenQueryPkt, sizeof(NTV2BoardOpenInfo), &p));
	if (!pPkt)
		return AJA_NULL;

	NTV2BoardOpenInfo *pBOI (reinterpret_cast<NTV2BoardOpenInfo*>(p));
	pBOI->boardNumber = htonl(inDeviceIndex);
	pBOI->boardType = 0;
	return pPkt;
}

static bool isNubReadRegisterRespPacket (NTV2NubPkt *pPkt)
{
	return isNTV2NubPacketType(pPkt, eNubReadRegisterSingleRespPkt);
}

static bool isNubWriteRegisterRespPacket (NTV2NubPkt *pPkt)
{
	return isNTV2NubPacketType(pPkt, eNubWriteRegisterRespPkt);
}

static bool isNubGetAutoCirculateRespPacket (NTV2NubPkt *pPkt)
{
	return isNTV2NubPacketType(pPkt, eNubGetAutoCirculateRespPkt);
}

static bool isNubControlAutoCirculateRespPacket (NTV2NubPkt *pPkt)
{	// Also handles eNubV1ControlAutoCirculateRespPkt
	return isNTV2NubPacketType(pPkt, eNubV2ControlAutoCirculateRespPkt);
}

static bool isNubWaitForInterruptRespPacket (NTV2NubPkt *pPkt)
{
	return isNTV2NubPacketType(pPkt, eNubWaitForInterruptRespPkt);
}

static bool isNubDriverGetBitFileInformationRespPacket (NTV2NubPkt *pPkt)
{
	return isNTV2NubPacketType(pPkt,  eNubDriverGetBitFileInformationRespPkt);
}

static bool isNubDriverGetBuildInformationRespPacket (NTV2NubPkt *pPkt)
{
	return isNTV2NubPacketType(pPkt,  eNubDriverGetBuildInformationRespPkt);
}

static bool isNubDownloadTestPatternRespPacket (NTV2NubPkt *pPkt)
{
	return isNTV2NubPacketType(pPkt, eNubDownloadTestPatternRespPkt);
}

static bool isNubReadRegisterMultiRespPacket (NTV2NubPkt *pPkt)
{
	return isNTV2NubPacketType(pPkt, eNubReadRegisterMultiRespPkt);
}


static NTV2NubPkt * BuildReadRegisterQueryPacket (	LWord handle,
													NTV2NubProtocolVersion nubProtocolVersion,
													ULWord registerNumber,
													ULWord registerMask,
													ULWord registerShift)
{
	char *p(AJA_NULL);
	NTV2NubPkt *pPkt(BuildNubBasePacket (nubProtocolVersion, eNubReadRegisterSingleQueryPkt,
										sizeof(NTV2ReadWriteRegisterPayload), &p));
	if (!pPkt)
		return AJA_NULL;
	
	NTV2ReadWriteRegisterPayload *pRWRP = reinterpret_cast<NTV2ReadWriteRegisterPayload*>(p);
	pRWRP->handle			= LWord(htonl(handle));
	pRWRP->registerNumber	= htonl(registerNumber);
	pRWRP->registerValue	= htonl(0);	// Value will be here on response 
	pRWRP->registerMask		= htonl(registerMask);
	pRWRP->registerShift	= htonl(registerShift);
	return pPkt;
}	//	BuildReadRegisterQueryPacket


static NTV2NubPkt * BuildWriteRegisterQueryPacket (	LWord handle,
													NTV2NubProtocolVersion nubProtocolVersion,
													ULWord registerNumber,
													ULWord registerValue,
													ULWord registerMask,
													ULWord registerShift)
{
	char *p(AJA_NULL);
	NTV2NubPkt *pPkt(BuildNubBasePacket (nubProtocolVersion, eNubWriteRegisterQueryPkt,
										sizeof(NTV2ReadWriteRegisterPayload), &p));
	if (!pPkt)
		return AJA_NULL;

	NTV2ReadWriteRegisterPayload *pRWRP = reinterpret_cast<NTV2ReadWriteRegisterPayload*>(p);
	pRWRP->handle			= LWord(htonl(handle));
	pRWRP->registerNumber	= htonl(registerNumber);
	pRWRP->registerValue	= htonl(registerValue);
	pRWRP->registerMask		= htonl(registerMask);
	pRWRP->registerShift	= htonl(registerShift);
	return pPkt;
}	//	BuildWriteRegisterQueryPacket


static NTV2NubPkt * BuildAutoCirculateQueryPacket (LWord handle,
													NTV2NubProtocolVersion nubProtocolVersion,
													AUTOCIRCULATE_DATA &autoCircData)
{
	NTV2NubPktType pktType = eNumNTV2NubPktTypes;
	ULWord payloadSize = 0;

	switch (autoCircData.eCommand)
	{
		case eStartAutoCirc:
		case eStopAutoCirc:
		case eAbortAutoCirc:
		case ePauseAutoCirc:
		case eFlushAutoCirculate:
			pktType = eNubV2ControlAutoCirculateQueryPkt;
			payloadSize = sizeof(NTV2ControlAutoCircPayload);
			break;

		case eGetAutoCirc:
			// queryStr = NTV2NUB_GET_AUTOCIRCULATE_QUERY;
			pktType = eNubGetAutoCirculateQueryPkt;
			payloadSize = sizeof(NTV2GetAutoCircPayload);
			break;

		default: // Not handled yet.
			return AJA_NULL;
	}

	char *p(AJA_NULL);
	NTV2NubPkt *pPkt(BuildNubBasePacket (nubProtocolVersion, pktType, payloadSize, &p));
	if (!pPkt)
		return AJA_NULL;

	switch (autoCircData.eCommand)
	{
		case eStartAutoCirc:
		case eStopAutoCirc:
		case eAbortAutoCirc:
		case ePauseAutoCirc:
		case eFlushAutoCirculate:
			{
				NTV2ControlAutoCircPayload *pCACP = reinterpret_cast<NTV2ControlAutoCircPayload*>(p);
				pCACP->handle		= htonl(handle);
				pCACP->eCommand		= htonl(autoCircData.eCommand);
				pCACP->channelSpec	= htonl(autoCircData.channelSpec);
				if (autoCircData.eCommand == ePauseAutoCirc)
					pCACP->bVal1	= htonl(ULWord(autoCircData.bVal1)); // bool bPlayToPause
			}
			break;

		case eGetAutoCirc:
			{
				NTV2GetAutoCircPayload *pGACP = reinterpret_cast<NTV2GetAutoCircPayload*>(p);
				pGACP->handle		= LWord(htonl(handle));
				pGACP->eCommand		= htonl(autoCircData.eCommand);
				pGACP->channelSpec	= htonl(autoCircData.channelSpec);
			}
			break;

		default: // Not handled yet.
			return AJA_NULL;
	}
	return pPkt;
}	//	BuildAutoCirculateQueryPacket


static NTV2NubPkt * BuildWaitForInterruptQueryPacket (	LWord handle,
														NTV2NubProtocolVersion nubProtocolVersion,
														INTERRUPT_ENUMS eInterrupt,
														ULWord timeOutMs)
{
	char *p(AJA_NULL);
	NTV2NubPkt *pPkt(BuildNubBasePacket (nubProtocolVersion, eNubWaitForInterruptQueryPkt,
										sizeof(NTV2WaitForInterruptPayload), &p));
	if (!pPkt)
		return AJA_NULL;

	NTV2WaitForInterruptPayload *pWFIP = reinterpret_cast<NTV2WaitForInterruptPayload*>(p);
	pWFIP->handle		= LWord(htonl(handle));
	pWFIP->eInterrupt	= htonl(ULWord(eInterrupt));
	pWFIP->timeOutMs	= htonl(timeOutMs);
	return pPkt;
}	//	BuildWaitForInterruptQueryPacket


static NTV2NubPkt * BuildDriverGetBitFileInformationQueryPacket (LWord handle,
																NTV2NubProtocolVersion nubProtocolVersion,
																BITFILE_INFO_STRUCT &bitFileInfo,
																NTV2BitFileType bitFileType)
{
	char *p(AJA_NULL);
	NTV2NubPkt *pPkt(BuildNubBasePacket (nubProtocolVersion,  eNubDriverGetBitFileInformationQueryPkt,
										sizeof(NTV2DriverGetBitFileInformationPayload), &p));
	if (!pPkt)
		return AJA_NULL;

	NTV2DriverGetBitFileInformationPayload *pDGBFIP = reinterpret_cast<NTV2DriverGetBitFileInformationPayload*>(p);
	pDGBFIP->handle					= LWord(htonl(handle));
	pDGBFIP->bitFileType			= htonl(ULWord(bitFileType));
	pDGBFIP->bitFileInfo.whichFPGA	= NTV2XilinxFPGA(htonl(ULWord(bitFileInfo.whichFPGA)));
	return pPkt;
}	//	BuildDriverGetBitFileInformationQueryPacket


static NTV2NubPkt * BuildDriverGetBuildInformationQueryPacket (LWord  handle,
																NTV2NubProtocolVersion nubProtocolVersion)
{
	char *p(AJA_NULL);
	NTV2NubPkt *pPkt(BuildNubBasePacket (nubProtocolVersion, eNubDriverGetBuildInformationQueryPkt,
											sizeof(NTV2DriverGetBuildInformationPayload), &p));
	if (!pPkt)
		return AJA_NULL;

	NTV2DriverGetBuildInformationPayload *pDGBFIP = reinterpret_cast<NTV2DriverGetBuildInformationPayload*>(p);
	pDGBFIP->handle = LWord(htonl(handle));
	return pPkt;
}	//	BuildDriverGetBuildInformationQueryPacket


static NTV2NubPkt * BuildDownloadTestPatternQueryPacket (LWord handle,
														NTV2NubProtocolVersion nubProtocolVersion,
														NTV2Channel channel,
														NTV2FrameBufferFormat testPatternFrameBufferFormat,
														UWord signalMask,
														bool testPatternDMAEnable,
														ULWord testPatternNumber)
{
	char *p(AJA_NULL);
	NTV2NubPkt *pPkt(BuildNubBasePacket (nubProtocolVersion, eNubDownloadTestPatternQueryPkt,
										sizeof(NTV2DownloadTestPatternPayload), &p));
	if (!pPkt)
		return AJA_NULL;

	NTV2DownloadTestPatternPayload *pDTPP = reinterpret_cast<NTV2DownloadTestPatternPayload*>(p);
	pDTPP->handle						= LWord(htonl(handle));
	pDTPP->channel						= htonl(channel);
	pDTPP->testPatternFrameBufferFormat	= htonl(testPatternFrameBufferFormat);
	pDTPP->signalMask					= htonl(signalMask);
	pDTPP->testPatternDMAEnable			= htonl(testPatternDMAEnable);
	pDTPP->testPatternNumber			= htonl(testPatternNumber);
	return pPkt;
}	//	BuildDownloadTestPatternQueryPacket


static NTV2NubPkt * BuildReadRegisterMultiQueryPacket (	LWord handle,
														NTV2NubProtocolVersion nubProtocolVersion,
														ULWord numRegs,
														NTV2ReadWriteRegisterSingle aRegs[])
{
	char *p(AJA_NULL);
	NTV2NubPkt *pPkt(BuildNubBasePacket (nubProtocolVersion, eNubReadRegisterMultiQueryPkt,
										sizeof(NTV2ReadWriteMultiRegisterPayloadHeader) + numRegs * sizeof(NTV2ReadWriteRegisterSingle), &p));
	if (!pPkt)
		return AJA_NULL;

	// TODO: Move this into BuildNubBasePacket for this pkt type
	// memset(pPkt, 0xab, sizeof(NTV2NubPkt));

	NTV2ReadWriteMultiRegisterPayload *pRWMRP (reinterpret_cast<NTV2ReadWriteMultiRegisterPayload*>(p));
	pRWMRP->payloadHeader.handle = LWord(htonl(handle));
	pRWMRP->payloadHeader.numRegs = htonl(numRegs);
	for (ULWord i = 0; i < numRegs; i++)
	{
		pRWMRP->aRegs[i].registerNumber = htonl(aRegs[i].registerNumber);
		pRWMRP->aRegs[i].registerValue = htonl(0);	// Value will be here on response 
		pRWMRP->aRegs[i].registerMask = htonl(aRegs[i].registerMask);
		pRWMRP->aRegs[i].registerShift = htonl(aRegs[i].registerShift);
	}
	return pPkt;
}	//	BuildReadRegisterMultiQueryPacket


static void deNBOifyAndCopyGetAutoCirculateData (AUTOCIRCULATE_STATUS_STRUCT & outACStatus,  NTV2GetAutoCircPayload * pGACP)
{
	// Some 32 bit quantities
	outACStatus.channelSpec	= NTV2Crosspoint(ntohl(pGACP->channelSpec));
	outACStatus.state		= NTV2AutoCirculateState(ntohl(pGACP->state));
	outACStatus.startFrame	= LWord(ntohl(pGACP->startFrame));
	outACStatus.endFrame	= LWord(ntohl(pGACP->endFrame));
	outACStatus.activeFrame	= LWord(ntohl(pGACP->activeFrame));

	// Note: the following four items are 64-bit quantities!
	outACStatus.rdtscStartTime			= ntohll(pGACP->rdtscStartTime);
	outACStatus.audioClockStartTime		= ntohll(pGACP->audioClockStartTime);
	outACStatus.rdtscCurrentTime		= ntohll(pGACP->rdtscCurrentTime);
	outACStatus.audioClockCurrentTime	= ntohll(pGACP->audioClockCurrentTime);

	// Back to 32 bit quantities.
	outACStatus.framesProcessed	= ntohl(pGACP->framesProcessed);
	outACStatus.framesDropped	= ntohl(pGACP->framesDropped);
	outACStatus.bufferLevel		= ntohl(pGACP->bufferLevel);

	// These are bools, which natively on some systems (Linux) are 1 byte and others 4 (MacOSX)
	// So the portable structures makes them all ULWords.
#ifdef MSWindows
#pragma warning(disable: 4800) 
#endif	
	outACStatus.bWithAudio				= bool(ntohl(pGACP->bWithAudio));
	outACStatus.bWithRP188				= bool(ntohl(pGACP->bWithRP188));
	outACStatus.bFbfChange				= bool(ntohl(pGACP->bFboChange));
	outACStatus.bWithColorCorrection	= bool(ntohl(pGACP->bWithColorCorrection));
	outACStatus.bWithVidProc			= bool(ntohl(pGACP->bWithVidProc));
	outACStatus.bWithCustomAncData		= bool(ntohl(pGACP->bWithCustomAncData));
#ifdef MSWindows
#pragma warning(default: 4800)
#endif
}

static void deNBOifyAndCopyGetDriverBitFileInformation (BITFILE_INFO_STRUCT &localBitFileInfo,
														BITFILE_INFO_STRUCT &remoteBitFileInfo)
{
	localBitFileInfo.checksum = ntohl(remoteBitFileInfo.checksum);
	localBitFileInfo.structVersion = ntohl(remoteBitFileInfo.structVersion);
	localBitFileInfo.structSize = ntohl(remoteBitFileInfo.structSize);

	localBitFileInfo.numBytes = ntohl(remoteBitFileInfo.numBytes);

	::memcpy(localBitFileInfo.dateStr, remoteBitFileInfo.dateStr, NTV2_BITFILE_DATETIME_STRINGLENGTH); 
	::memcpy(localBitFileInfo.timeStr, remoteBitFileInfo.timeStr, NTV2_BITFILE_DATETIME_STRINGLENGTH); 
	::memcpy(localBitFileInfo.designNameStr , remoteBitFileInfo.designNameStr, NTV2_BITFILE_DESIGNNAME_STRINGLENGTH); 

	localBitFileInfo.bitFileType = ntohl(remoteBitFileInfo.bitFileType);
	localBitFileInfo.whichFPGA = (NTV2XilinxFPGA)ntohl(remoteBitFileInfo.whichFPGA);
}

static void deNBOifyAndCopyGetDriverBuildInformation( BUILD_INFO_STRUCT &localBuildInfo,
													  BUILD_INFO_STRUCT &remoteBuildInfo)
{
	localBuildInfo.structVersion = ntohl(remoteBuildInfo.structVersion);
	localBuildInfo.structSize = ntohl(remoteBuildInfo.structSize);

	memcpy(localBuildInfo.buildStr, remoteBuildInfo.buildStr, NTV2_BUILD_STRINGLENGTH); 
}



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/**
	@brief	Specific NTV2RPCClientAPI implementation to control NTV2 device on remote host via TCP/IP using legacy nub code.
**/
class AJAExport NTV2LegacyNubClient : public NTV2RPCClientAPI
{
	//	Instance Methods
	public:
								NTV2LegacyNubClient (const NTV2ConnectParams & inConnectParams)
													:	NTV2RPCClientAPI(inConnectParams),
														_sockfd				(-1),
														_remoteHandle		(INVALID_NUB_HANDLE),
														_nubProtocolVersion	(ntv2NubProtocolVersionNone),
														_remoteIndex		(0)								{}
		AJA_VIRTUAL									~NTV2LegacyNubClient()			{NTV2CloseRemote();}
		AJA_VIRTUAL	inline bool						IsConnected	(void) const		{return SocketValid()  &&  HandleValid();}
		AJA_VIRTUAL inline AJASocket				Socket (void) const				{return _sockfd;}
		AJA_VIRTUAL	inline bool						SocketValid (void) const		{return Socket() != AJASocket(-1);}
		AJA_VIRTUAL inline LWord					Handle (void) const				{return _remoteHandle;}
		AJA_VIRTUAL	bool							HandleValid (void) const		{return Handle() != INVALID_NUB_HANDLE;}
		AJA_VIRTUAL	string							Name (void) const				{return _name;}
		AJA_VIRTUAL inline NTV2NubProtocolVersion	ProtocolVersion (void) const	{return _nubProtocolVersion;}
		AJA_VIRTUAL	bool		NTV2ReadRegisterRemote	(const ULWord regNum, ULWord & outRegValue, const ULWord regMask = 0xFFFFFFFF, const ULWord regShift = 0);
		AJA_VIRTUAL	bool		NTV2WriteRegisterRemote	(const ULWord regNum, const ULWord regValue, const ULWord regMask = 0xFFFFFFFF, const ULWord regShift = 0);
		AJA_VIRTUAL	bool		NTV2AutoCirculateRemote	(AUTOCIRCULATE_DATA & autoCircData);
		AJA_VIRTUAL	bool		NTV2WaitForInterruptRemote	(const INTERRUPT_ENUMS eInterrupt, const ULWord timeOutMs);
		AJA_VIRTUAL	bool		NTV2DriverGetBitFileInformationRemote	(BITFILE_INFO_STRUCT & outInfo, const NTV2BitFileType inType);
		AJA_VIRTUAL	bool		NTV2DriverGetBuildInformationRemote	(BUILD_INFO_STRUCT & outBuildInfo);
		AJA_VIRTUAL	bool		NTV2DownloadTestPatternRemote	(const NTV2Channel channel, const NTV2PixelFormat testPatternFBF,
															const UWord signalMask, const bool testPatDMAEnb, const ULWord testPatNum);
		AJA_VIRTUAL	bool		NTV2ReadRegisterMultiRemote	(const ULWord numRegs, ULWord & outFailedRegNum,  NTV2RegInfo aRegs[]);
		AJA_VIRTUAL	bool		NTV2GetDriverVersionRemote	(ULWord & outDriverVersion)		{(void) outDriverVersion;	return false;}
		AJA_VIRTUAL	bool		NTV2DMATransferRemote		(const NTV2DMAEngine inDMAEngine,	const bool inIsRead,
															const ULWord inFrameNumber,			NTV2Buffer & inOutBuffer,
															const ULWord inCardOffsetBytes,		const ULWord inNumSegments,
															const ULWord inSegmentHostPitch,	const ULWord inSegmentCardPitch,
															const bool inSynchronous);
		AJA_VIRTUAL	bool		NTV2MessageRemote	(NTV2_HEADER *	pInMessage);
		AJA_VIRTUAL ostream &	Print (ostream & oss) const;

		AJA_VIRTUAL bool		NTV2QueryDevices (NTV2StringList & outDeviceInfos);

	protected:
		AJA_VIRTUAL	bool		NTV2OpenRemote (void);
		AJA_VIRTUAL int			OpenRemoteDeviceWithIndex (const UWord inDeviceIndex);
		AJA_VIRTUAL	bool		NTV2CloseRemote (void);
		AJA_VIRTUAL	inline		ULWord	ConnectionID (void) const		{return 0;}

	//	Instance Data
	private:
		AJASocket				_sockfd;				///< @brief	Socket descriptor
		LWord					_remoteHandle;			///< @brief	Remote host handle
		NTV2NubProtocolVersion	_nubProtocolVersion;	///< @brief	Protocol version
		UWord					_remoteIndex;			///< @brief	Remote device index number
		string					_name;					///< @brief	User-friendly name
};	//	NTV2LegacyNubClient



bool NTV2LegacyNubClient::NTV2OpenRemote (void)
{	//	PARAMS:	const string & inHostName, const UWord inDeviceIndexNum
	const string inHostName(HostName());
	if (inHostName.empty())
		return false;	//	Empty host IP or name

	UWord portNumber(NTV2LEGACYNUBPORT);
	if (HasConnectParam(kConnectParamPort))
	{
		const string portStr(ConnectParam(kConnectParamPort));
		if (!portStr.empty())
			portNumber = UWord(aja::stoul(portStr));	//	Override default legacy port number
	}

	UWord inDeviceIndexNum(0);
	if (HasConnectParam(kConnectParamDevIndex))
	{
		const string devNumStr(ConnectParam(kConnectParamDevIndex));
		if (!devNumStr.empty())
			inDeviceIndexNum = UWord(aja::stoul(devNumStr));	//	Override default device index number
	}

	//	Get the host info
	struct hostent * he(::gethostbyname(inHostName.c_str()));
	if (!he)
	{
#ifndef MSWindows
		herror("gethostbyname");
#endif
		return false;
	}

	_sockfd = ::socket(PF_INET, SOCK_STREAM, 0);
	if (!SocketValid()) 
	{
		NBFAIL("'socket' failed, socket=" << Socket() << ": " << ::strerror(errno));
		return false;
	}

	struct sockaddr_in their_addr;
	their_addr.sin_family = AF_INET;	// host byte order 
	their_addr.sin_port = htons(NTV2LEGACYNUBPORT);  // short, network byte order 
	their_addr.sin_addr = *((struct in_addr *)he->h_addr);
	::memset(&(their_addr.sin_zero), '\0', 8);  // zero the rest of the struct 

	int retval (::connect(Socket(), reinterpret_cast<struct sockaddr*>(&their_addr), sizeof(struct sockaddr)));
	if (retval == -1) 
	{
		NBFAIL("'connect' failed: " << ::strerror(errno) << ", socket=" << Socket() << ", hostName='" << inHostName << "'");
		NTV2Disconnect();
	}
	if (retval < 0)
		return false;

	retval = OpenRemoteDeviceWithIndex(inDeviceIndexNum);
	switch (retval)
	{
		case NTV2_REMOTE_ACCESS_SUCCESS:
			NBNOTE("OpenRemote succeeded, handle=" << _remoteHandle);
			return 0;

		case NTV2_REMOTE_ACCESS_CONNECTION_CLOSED:
			NTV2Disconnect();
			NBFAIL("OpenRemote failed 'connection closed', handle=" << xHEX0N(_remoteHandle,8));
			_remoteHandle = LWord(INVALID_NUB_HANDLE);
			break;

		case NTV2_REMOTE_ACCESS_NOT_CONNECTED:
		case NTV2_REMOTE_ACCESS_OUT_OF_MEMORY:
		case NTV2_REMOTE_ACCESS_SEND_ERR:
		case NTV2_REMOTE_ACCESS_RECV_ERR:
		case NTV2_REMOTE_ACCESS_TIMEDOUT:
		case NTV2_REMOTE_ACCESS_NO_CARD:
		case NTV2_REMOTE_ACCESS_NOT_OPEN_RESP:
		case NTV2_REMOTE_ACCESS_NON_NUB_PKT:
		default:
			// Failed, but don't close connection, can try with another card on same connection.
			NBWARN("OpenRemote failed, _remoteHandle came back as " << _remoteHandle);
			_remoteHandle = LWord(INVALID_NUB_HANDLE);
			break;
	}

	if (retval >= 0)		//	Set my user-friendly name...
	{	ostringstream oss;  oss << "Device " << inDeviceIndexNum << " on host '" << HostName();
		if (portNumber != NTV2LEGACYNUBPORT)
			oss << ":" << portNumber;
		_name = oss.str();
	}
	return retval < 0 ? false : true;

}	//	NTV2OpenRemote


int NTV2LegacyNubClient::OpenRemoteDeviceWithIndex (const UWord inDeviceIndex)
{
	// Connected?
	if (!SocketValid())
		return NTV2_REMOTE_ACCESS_NOT_CONNECTED; 

	// Construct open query
	NTV2NubPkt *pPkt = BuildOpenQueryPacket(inDeviceIndex);
	if (!pPkt)
		return NTV2_REMOTE_ACCESS_OUT_OF_MEMORY;

	int retcode (NTV2_REMOTE_ACCESS_SUCCESS);
	int len (pPkt  ?  int(sizeof(NTV2NubPktHeader) + pPkt->hdr.dataLength)  :  0);
	// Send it
	if (NBOifyNTV2NubPkt(pPkt)) 
	{
		if (-1 == ::sendall(Socket(), reinterpret_cast<char*>(pPkt), &len))
			{NBFAIL("'sendall' failed, socket=" << Socket() << ", len=" << len << ": " << ::strerror(errno));  retcode = NTV2_REMOTE_ACCESS_SEND_ERR;}
		else
		{
			// Wait for response
			int numbytes (::recvtimeout_sec(Socket(), reinterpret_cast<char*>(pPkt), sizeof(NTV2NubPkt), 2)); // 2 second timeout

			switch (numbytes)
			{
				case  0:	// Remote side closed connection
							retcode = NTV2_REMOTE_ACCESS_CONNECTION_CLOSED;
							NBFAIL("'recvtimeout_sec' returned zero bytes:  remote access connection closed");
							break;

				case -1: // error occurred
						NBFAIL("'recvtimeout_sec' failed on socket " << Socket() << ": " << ::strerror(errno));
						retcode = NTV2_REMOTE_ACCESS_RECV_ERR;
						break;
			
				case -2: // timeout occurred
						retcode = NTV2_REMOTE_ACCESS_TIMEDOUT;
						NBFAIL("'recvtimeout_sec' timed out on socket " << Socket());
						break;
			
				default: // got some data.  Open response packet?
						if (deNBOifyNTV2NubPkt(pPkt, ULWord(numbytes))) 
						{
							if (isNubOpenRespPacket(pPkt)) 
							{
								// printf("Got an open response packet\n");
								NTV2BoardOpenInfo * pBoardOpenInfo;
								pBoardOpenInfo = reinterpret_cast<NTV2BoardOpenInfo*>(getNubPktPayload(pPkt));
								_remoteHandle = LWord(ntohl(pBoardOpenInfo->handle));
								// printf("Handle = %d\n", _remoteHandle);
								if (Handle() == LWord(INVALID_NUB_HANDLE))
								{
									NBFAIL("Got invalid handle on 'open' response");
									retcode = NTV2_REMOTE_ACCESS_NO_CARD;
								}
								_nubProtocolVersion = pPkt->hdr.protocolVersion;
								NBDBG("Got protocol version " << _nubProtocolVersion << " from 'open' response");
							}
							else // Not an open response packet, count it and discard it.
							{
								++gIgnoredNTV2pkts;
								retcode = NTV2_REMOTE_ACCESS_NOT_OPEN_RESP;
							}
						}
						else // Non ntv2 packet on our port.
						{
							// NOTE: Defragmentation of jumbo packets would probably go here.
							retcode = NTV2_REMOTE_ACCESS_NON_NUB_PKT;
							NBFAIL("Non-nub packet on NTV2 port, socket=" << Socket());
						}
			}	//	switch on numbytes
		}	//	sendall succeeded
	}	//	if marshalled OK
	delete pPkt;
	return retcode;
}	//	OpenRemoteDeviceWithIndex


bool NTV2LegacyNubClient::NTV2CloseRemote (void)
{
	if (SocketValid())
	{
		#ifdef MSWindows
			closesocket(_sockfd);
		#else
			close(_sockfd);
		#endif
		_sockfd = -1;
		_remoteHandle = INVALID_NUB_HANDLE;
	}
	return NTV2RPCClientAPI::NTV2CloseRemote();
}


bool NTV2LegacyNubClient::NTV2ReadRegisterRemote (const ULWord regNum, ULWord & outRegValue, const ULWord regMask, const ULWord regShift)
{
	outRegValue = 0;
	if (!IsConnected())
		return false;	//	No connection

	// Construct open query
	NTV2NubPkt *pPkt (BuildReadRegisterQueryPacket (Handle(), ProtocolVersion(),  regNum, regMask, regShift));
	if (!pPkt)
		return false;	//	NTV2_REMOTE_ACCESS_OUT_OF_MEMORY;

	int retcode (NTV2_REMOTE_ACCESS_SUCCESS);
	int len (pPkt  ?  int(sizeof(NTV2NubPktHeader) + pPkt->hdr.dataLength)  :  0);
	// Send it
	if (NBOifyNTV2NubPkt(pPkt)) 
	{
		if (-1 == ::sendall(Socket(), reinterpret_cast<char*>(pPkt), &len))
			{NBFAIL("'sendall' failed, socket=" << Socket() << ", len=" << len << ": " << strerror(errno));  retcode = NTV2_REMOTE_ACCESS_SEND_ERR;}
		else
		{
			// Wait for response
			int numbytes (::recvtimeout_sec(Socket(), reinterpret_cast<char*>(pPkt), sizeof(NTV2NubPkt), 2)); // 2 second timeout

			switch (numbytes)
			{
				case  0:	// Remote side closed connection
							retcode = NTV2_REMOTE_ACCESS_CONNECTION_CLOSED;
							NBFAIL("'recvtimeout_sec' returned zero bytes:  remote access connection closed");
							break;

				case -1:	// error occurred
							NBFAIL("'recvtimeout_sec' failed on socket " << Socket() << ": " << strerror(errno));
							retcode = NTV2_REMOTE_ACCESS_RECV_ERR;
							break;

				case -2:	// timeout occurred
							retcode = NTV2_REMOTE_ACCESS_TIMEDOUT;
							NBFAIL("'recvtimeout_sec' timed out on socket " << Socket());
							break;

				default: // got some data.  Open response packet?
						if (deNBOifyNTV2NubPkt(pPkt, ULWord(numbytes))) 
						{
							if (isNubReadRegisterRespPacket(pPkt)) 
							{
								// printf("Got a read register response packet\n");
								NTV2ReadWriteRegisterPayload * pRWRP;
								pRWRP = (NTV2ReadWriteRegisterPayload *)getNubPktPayload(pPkt);
								// Did card go away?
								LWord hdl (LWord(ntohl(pRWRP->handle)));
								// printf("hdl = %d\n", hdl);
								if (hdl == LWord(INVALID_NUB_HANDLE))
								{
									printf("Got invalid nub handle back from register read.\n");
									retcode = NTV2_REMOTE_ACCESS_NO_CARD;
								}
								ULWord result (ntohl (pRWRP->result));
								if (result)
								{
									outRegValue = ntohl(pRWRP->registerValue);
									// printf("Register #%d value = 0x%08x\n", registerNumber, *registerValue);
								}
								else // Read register failed on remote side
								{
									// printf("Read Register %d failed on remote side.\n", registerNumber);
									retcode = NTV2_REMOTE_ACCESS_READ_REG_FAILED;
								}
							}
							else // Not an read register response packet, count it and discard it.
							{
								++gIgnoredNTV2pkts;
								retcode = NTV2_REMOTE_ACCESS_NOT_READ_REGISTER_RESP;
							}
						}
						else // Non ntv2 packet on our port.
						{
							// NOTE: Defragmentation of jumbo packets would probably go here.
							retcode = NTV2_REMOTE_ACCESS_NON_NUB_PKT;
							NBFAIL("Non-nub packet on NTV2 port, socket=" << Socket());
						}
			}
		}
	}
	delete pPkt;
	return retcode < 0 ? false : true;
}

bool NTV2LegacyNubClient::NTV2WriteRegisterRemote (const ULWord regNum, const ULWord regValue, const ULWord regMask, const ULWord regShift)
{
	if (!IsConnected())
		return false;	//	No connection

	// Construct open query
	NTV2NubPkt *pPkt (BuildWriteRegisterQueryPacket (Handle(),  _nubProtocolVersion,  regNum, regValue, regMask, regShift));
	if (!pPkt)
		return false;	//	NTV2_REMOTE_ACCESS_OUT_OF_MEMORY;

	int retcode (NTV2_REMOTE_ACCESS_SUCCESS);
	int len (pPkt  ?  int(sizeof(NTV2NubPktHeader) + pPkt->hdr.dataLength)  :  0);
	// Send it
	if (NBOifyNTV2NubPkt(pPkt)) 
	{
		if (-1 == ::sendall(Socket(), reinterpret_cast<char*>(pPkt), &len))
			{NBFAIL("'sendall' failed, socket=" << Socket() << ", len=" << len << ": " << strerror(errno));  retcode = NTV2_REMOTE_ACCESS_SEND_ERR;}
		else
		{
			// Wait for response
			int numbytes (::recvtimeout_sec(Socket(), reinterpret_cast<char*>(pPkt), sizeof(NTV2NubPkt), 2)); // 2 second timeout

			switch (numbytes)
			{
				case  0:	// Remote side closed connection
							retcode = NTV2_REMOTE_ACCESS_CONNECTION_CLOSED;
							NBFAIL("'recvtimeout_sec' returned zero bytes:  remote access connection closed");
							break;

				case -1:	// error occurred
							NBFAIL("'recvtimeout_sec' failed on socket " << Socket() << ": " << strerror(errno));
							retcode = NTV2_REMOTE_ACCESS_RECV_ERR;
							break;

				case -2:	// timeout occurred
							retcode = NTV2_REMOTE_ACCESS_TIMEDOUT;
							NBFAIL("'recvtimeout_sec' timed out on socket " << Socket());
							break;

				default: // got some data.  Open response packet?
						if (deNBOifyNTV2NubPkt(pPkt, ULWord(numbytes))) 
						{
							if (isNubWriteRegisterRespPacket(pPkt)) 
							{
								// printf("Got a write register response packet\n");
								NTV2ReadWriteRegisterPayload * pRWRP(reinterpret_cast<NTV2ReadWriteRegisterPayload*>(getNubPktPayload(pPkt)));
								// Did card go away?
								LWord hdl (LWord(ntohl(pRWRP->handle)));
								// printf("hdl = %d\n", hdl);
								if (hdl == LWord(INVALID_NUB_HANDLE))
								{
									printf("Got invalid nub handle back from register write.\n");
									retcode = NTV2_REMOTE_ACCESS_NO_CARD;
								}
								ULWord result = ntohl (pRWRP->result);
								if (result)
								{
									// ULWord registerValue = ntohl(pRWRP->registerValue);
									// printf("Write succeeded. Register #%d value = 0x%08x\n", registerNumber, registerValue);
								}
								else // write register failed on remote side
								{
									printf("Write Register %d failed on remote side.\n", regNum);
								}
							}
							else // Not a write register response packet, count it and discard it.
							{
								++gIgnoredNTV2pkts;
								retcode = NTV2_REMOTE_ACCESS_NOT_WRITE_REGISTER_RESP;
							}
						}
						else // Non ntv2 packet on our port.
						{
							// NOTE: Defragmentation of jumbo packets would probably go here.
							retcode = NTV2_REMOTE_ACCESS_NON_NUB_PKT;
							NBFAIL("Non-nub packet on NTV2 port, socket=" << Socket());
						}
			}	//	switch
		}	//	else
	}	//	NBOify OK
	delete pPkt;
	return retcode < 0 ? false : true;
}

bool NTV2LegacyNubClient::NTV2AutoCirculateRemote (AUTOCIRCULATE_DATA & autoCircData)
{
	if (!IsConnected())
		return false;

	// Construct autocirculate query packet.
	NTV2NubPkt *pPkt (BuildAutoCirculateQueryPacket (Handle(),  _nubProtocolVersion,  autoCircData));
	if (!pPkt)
		return false;	//	NTV2_REMOTE_ACCESS_OUT_OF_MEMORY;

	int retcode (NTV2_REMOTE_ACCESS_SUCCESS);
	int len (pPkt  ?  int(sizeof(NTV2NubPktHeader) + pPkt->hdr.dataLength)  :  0);
	// Send it
	if(NBOifyNTV2NubPkt(pPkt)) 
	{
		if (-1 == ::sendall(Socket(), reinterpret_cast<char*>(pPkt), &len))
			{NBFAIL("'sendall' failed, socket=" << Socket() << ", len=" << len << ": " << strerror(errno));  retcode = NTV2_REMOTE_ACCESS_SEND_ERR;}
		else
		{
			// Wait for response
			int numbytes (::recvtimeout_sec(Socket(), reinterpret_cast<char*>(pPkt), sizeof(NTV2NubPkt), 2)); // 2 second timeout

			switch (numbytes)
			{
				case  0:	// Remote side closed connection
							retcode = NTV2_REMOTE_ACCESS_CONNECTION_CLOSED;
							NBFAIL("'recvtimeout_sec' returned zero bytes:  remote access connection closed");
							break;

				case -1:	// error occurred
							NBFAIL("'recvtimeout_sec' failed on socket " << Socket() << ": " << strerror(errno));
							retcode = NTV2_REMOTE_ACCESS_RECV_ERR;
							break;

				case -2:	// timeout occurred
							retcode = NTV2_REMOTE_ACCESS_TIMEDOUT;
							NBFAIL("'recvtimeout_sec' timed out on socket " << Socket());
							break;

				default: // got some data.  Autocirculate response packet?
						if (deNBOifyNTV2NubPkt(pPkt, ULWord(numbytes))) 
						{
							if (isNubGetAutoCirculateRespPacket(pPkt)) 
							{
								// printf("Got an autocirculate response packet\n");
								NTV2GetAutoCircPayload * pGACP (reinterpret_cast<NTV2GetAutoCircPayload*>(getNubPktPayload(pPkt)));
								// Did card go away?
								LWord hdl (LWord(ntohl(pGACP->handle)));
								// printf("hdl = %d\n", hdl);
								if (hdl == LWord(INVALID_NUB_HANDLE))
								{
									NBFAIL("Got invalid nub handle back");
									retcode = NTV2_REMOTE_ACCESS_NO_CARD;
								}
								ULWord result (ntohl(pGACP->result));
								if (result)
								{	// Success
									AUTOCIRCULATE_STATUS_STRUCT * pStatus (reinterpret_cast<AUTOCIRCULATE_STATUS_STRUCT*>(autoCircData.pvVal1));
									deNBOifyAndCopyGetAutoCirculateData(*pStatus, pGACP);
									NBDBG("Success");
								}
								else // GetAutocirculate failed on remote side
								{
									NBFAIL("AutoCirculate GET failed on remote side");
								}
							}
							else if (isNubControlAutoCirculateRespPacket(pPkt)) 
							{
								// printf("Got an autocirculate response packet\n");
								NTV2ControlAutoCircPayload * pCACP(reinterpret_cast<NTV2ControlAutoCircPayload*>(getNubPktPayload(pPkt)));
								// Did card go away?
								LWord hdl (ntohl(pCACP->handle));
								// printf("hdl = %d\n", hdl);
								if (hdl == LWord(INVALID_NUB_HANDLE))
								{
									NBFAIL("Got invalid nub handle back");
									retcode = NTV2_REMOTE_ACCESS_NO_CARD;
								}
								ULWord result (ntohl(pCACP->result));
								if (result)
								{
									// Success
								}
								else // GetAutocirculate failed on remote side
								{
									// printf("Autocirculate CONTROL failed on remote side.\n");
									retcode = NTV2_REMOTE_AUTOCIRC_FAILED;
								}
							}
							// Other autocirculate responses go here.
							else // Not an autocirculate response packet, count it and discard it.
							{
								++gIgnoredNTV2pkts;
								retcode = NTV2_REMOTE_ACCESS_NOT_AUTOCIRC_RESP;
							}
						}	//	if deNBOifyNTV2NubPkt
						else // Non ntv2 packet on our port.
						{
							// NOTE: Defragmentation of jumbo packets would probably go here.
							retcode = NTV2_REMOTE_ACCESS_NON_NUB_PKT;
							NBFAIL("Non-nub packet on NTV2 port, socket=" << Socket());
						}	//	else non-ntv2 pkt
			}	//	switch on numbytes
		}	//	else sendall successful
	}	//	if NBOifyNTV2NubPkt OK
	delete pPkt;
	return retcode < 0 ? false : true;
}

bool NTV2LegacyNubClient::NTV2WaitForInterruptRemote (const INTERRUPT_ENUMS eInterrupt, const ULWord timeOutMs)
{
	if (!IsConnected())
		return false;	//	No connection

	// Construct open query
	NTV2NubPkt *pPkt(BuildWaitForInterruptQueryPacket(Handle(), _nubProtocolVersion, eInterrupt, timeOutMs));
	if (!pPkt)
		return false;	//	NTV2_REMOTE_ACCESS_OUT_OF_MEMORY;

	int retcode (NTV2_REMOTE_ACCESS_SUCCESS);
	int len (pPkt  ?  int(sizeof(NTV2NubPktHeader) + pPkt->hdr.dataLength)  :  0);
	// Send it
	if (NBOifyNTV2NubPkt(pPkt)) 
	{
		if (-1 == ::sendall(Socket(), reinterpret_cast<char*>(pPkt), &len))
			{NBFAIL("'sendall' failed, socket=" << Socket() << ", len=" << len << ": " << strerror(errno));  retcode = NTV2_REMOTE_ACCESS_SEND_ERR;}
		else
		{
			// Wait for response
			int numbytes = ::recvtimeout_sec(Socket(), (char *)pPkt, sizeof(NTV2NubPkt), 2); // 2 second timeout
			switch (numbytes)
			{
				case  0:	// Remote side closed connection
							retcode = NTV2_REMOTE_ACCESS_CONNECTION_CLOSED;
							NBFAIL("'recvtimeout_sec' returned zero bytes:  remote access connection closed");
							break;

				case -1:	// error occurred
							NBFAIL("'recvtimeout_sec' failed on sockfd " << Socket() << ": " << ::strerror(errno));
							retcode = NTV2_REMOTE_ACCESS_RECV_ERR;
							break;
			
				case -2:	// timeout occurred
							NBFAIL("'recvtimeout_sec' timed out after 2 seconds");
							retcode = NTV2_REMOTE_ACCESS_TIMEDOUT;
							break;
			
				default:	// got some data.  Open response packet?
							if (deNBOifyNTV2NubPkt(pPkt, ULWord(numbytes))) 
							{
								if (isNubWaitForInterruptRespPacket(pPkt)) 
								{
									// printf("Got a write register response packet\n");
									NTV2WaitForInterruptPayload * pWFIP (reinterpret_cast<NTV2WaitForInterruptPayload*>(getNubPktPayload(pPkt)));
									// Did card go away?
									LWord hdl (LWord(ntohl(pWFIP->handle)));
									// printf("hdl = %d\n", hdl);
									if (hdl == LWord(INVALID_NUB_HANDLE))
									{
										NBWARN("Got invalid nub handle back");
										retcode = NTV2_REMOTE_ACCESS_NO_CARD;
									}
									ULWord result(ntohl(pWFIP->result));
									if (result)
									{
										// printf("Waitfor interrupt %d succeeded.\n", eInterrupt);
									}
									else // write register failed on remote side
									{
										// printf("Waitfor interrupt %d failed on remote side.\n", eInterrupt);
										retcode = NTV2_REMOTE_ACCESS_WAIT_FOR_INTERRUPT_FAILED;
									}
								}
								else // Not a write register response packet, count it and discard it.
								{
									++gIgnoredNTV2pkts;
									retcode = NTV2_REMOTE_ACCESS_NOT_WAIT_FOR_INTERRUPT_RESP;
								}
							}
							else // Non ntv2 packet on our port.
							{
								// NOTE: Defragmentation of jumbo packets would probably go here.
								retcode = NTV2_REMOTE_ACCESS_NON_NUB_PKT;
								NBFAIL("Non-nub packet on NTV2 port, socket=" << Socket());
							}
			}	//	switch
		}	//	else
	}
	delete pPkt;
	return retcode < 0 ? false : true;
}

bool NTV2LegacyNubClient::NTV2DriverGetBitFileInformationRemote (BITFILE_INFO_STRUCT & outInfo,  const NTV2BitFileType inType)
{
	if (!IsConnected())
		return false;

	// Construct open query
	NTV2NubPkt *pPkt (BuildDriverGetBitFileInformationQueryPacket (Handle(),  _nubProtocolVersion,  outInfo,  inType));
	if (!pPkt)
		return false;	//	NTV2_REMOTE_ACCESS_OUT_OF_MEMORY;

	int retcode (NTV2_REMOTE_ACCESS_SUCCESS);
	int len (pPkt  ?  int(sizeof(NTV2NubPktHeader) + pPkt->hdr.dataLength)  :  0);
	// Send it
	if (NBOifyNTV2NubPkt(pPkt)) 
	{
		if (-1 == ::sendall(Socket(), reinterpret_cast<char*>(pPkt), &len))
			{NBFAIL("'sendall' failed, socket=" << Socket() << ", len=" << len << ": " << strerror(errno));  retcode = NTV2_REMOTE_ACCESS_SEND_ERR;}
		else
		{
			// Wait for response
			int numbytes = ::recvtimeout_sec(Socket(), (char *)pPkt, sizeof(NTV2NubPkt), 2); // 2 second timeout

			switch (numbytes)
			{
				case  0:	// Remote side closed connection
							retcode = NTV2_REMOTE_ACCESS_CONNECTION_CLOSED;
							NBFAIL("'recvtimeout_sec' returned zero bytes:  remote access connection closed");
							break;

				case -1:	// error occurred
							NBFAIL("'recvtimeout_sec' failed on socket " << Socket() << ": " << strerror(errno));
							retcode = NTV2_REMOTE_ACCESS_RECV_ERR;
							break;
			
				case -2:	// timeout occurred
							retcode = NTV2_REMOTE_ACCESS_TIMEDOUT;
							NBFAIL("'recvtimeout_sec' timed out on socket " << Socket());
							break;
			
				default:	// got some data.  Open response packet?
							if (deNBOifyNTV2NubPkt(pPkt, ULWord(numbytes))) 
							{
								if (isNubDriverGetBitFileInformationRespPacket(pPkt)) 
								{
									// printf("Got a driver get bitfile info response packet\n");
									NTV2DriverGetBitFileInformationPayload * pDGBFIP;
									pDGBFIP = (NTV2DriverGetBitFileInformationPayload *)getNubPktPayload(pPkt);
									// Did card go away?
									LWord hdl (LWord(ntohl(pDGBFIP->handle)));
									// printf("hdl = %d\n", hdl);
									if (hdl == LWord(INVALID_NUB_HANDLE))
									{
										printf("Got invalid nub handle back from get bitfile info.\n");
										retcode = NTV2_REMOTE_ACCESS_NO_CARD;
									}
									ULWord result (ntohl(pDGBFIP->result));
									if (result)
									{
										deNBOifyAndCopyGetDriverBitFileInformation(outInfo, pDGBFIP->bitFileInfo);
										NBINFO("Success, socket=" << Socket() << ", bitFileType=" << inType);
										// printf("Driver get bitfileinfo for type %d succeeded.\n", bitFileType);
									}
									else // failed on remote side
									{
										// printf("Driver get bitfileinfo for type %d failed on remote side.\n", bitFileType);
										retcode = NTV2_REMOTE_ACCESS_DRIVER_GET_BITFILE_INFO_FAILED;
									}
								}
								else // Not a write register response packet, count it and discard it.
								{
									++gIgnoredNTV2pkts;
									retcode = NTV2_REMOTE_ACCESS_NOT_DRIVER_GET_BITFILE_INFO;
								}
							}
							else // Non ntv2 packet on our port.
							{
								// NOTE: Defragmentation of jumbo packets would probably go here.
								retcode = NTV2_REMOTE_ACCESS_NON_NUB_PKT;
								NBFAIL("Non-nub packet on NTV2 port, socket=" << Socket());
							}
			}	//	switch
		}	//	else sendall OK
	}	//	if NBOifyNTV2NubPkt
	delete pPkt;
	return retcode;
}

bool NTV2LegacyNubClient::NTV2DriverGetBuildInformationRemote (BUILD_INFO_STRUCT & outBuildInfo)
{
	if (!IsConnected())
		return false;

	// Construct open query
	NTV2NubPkt *pPkt (BuildDriverGetBuildInformationQueryPacket(Handle(),  ProtocolVersion()));
	if (!pPkt)
		return false;	//	NTV2_REMOTE_ACCESS_OUT_OF_MEMORY;

	int retcode (NTV2_REMOTE_ACCESS_SUCCESS);
	int len (pPkt  ?  int(sizeof(NTV2NubPktHeader) + pPkt->hdr.dataLength)  :  0);
	// Send it
	if (NBOifyNTV2NubPkt(pPkt)) 
	{
		if (-1 == ::sendall(Socket(), reinterpret_cast<char*>(pPkt), &len))
			{NBFAIL("'sendall' failed, socket=" << Socket() << ", len=" << len << ": " << strerror(errno));  retcode = NTV2_REMOTE_ACCESS_SEND_ERR;}
		else
		{
			// Wait for response
			int numbytes (::recvtimeout_sec(Socket(), reinterpret_cast<char*>(pPkt), sizeof(NTV2NubPkt), 2)); // 2 second timeout

			switch (numbytes)
			{
				case  0:	// Remote side closed connection
							retcode = NTV2_REMOTE_ACCESS_CONNECTION_CLOSED;
							NBFAIL("'recvtimeout_sec' returned zero bytes:  remote access connection closed");
							break;

				case -1:	// error occurred
							NBFAIL("'recvtimeout_sec' failed on socket " << Socket() << ": " << strerror(errno));
							retcode = NTV2_REMOTE_ACCESS_RECV_ERR;
							break;
			
				case -2:	// timeout occurred
							retcode = NTV2_REMOTE_ACCESS_TIMEDOUT;
							NBFAIL("'recvtimeout_sec' timed out on socket " << Socket());
							break;
			
				default:	// got some data.  Open response packet?
							if (deNBOifyNTV2NubPkt(pPkt, ULWord(numbytes))) 
							{
								if (isNubDriverGetBuildInformationRespPacket((NTV2NubPkt *)pPkt)) 
								{
									// printf("Got a driver get build info response packet\n");
									NTV2DriverGetBuildInformationPayload * pDGBIP (reinterpret_cast<NTV2DriverGetBuildInformationPayload*>(getNubPktPayload(pPkt)));
									// Did card go away?
									LWord hdl (LWord(ntohl(pDGBIP->handle)));
									// printf("hdl = %d\n", hdl);
									if (hdl == LWord(INVALID_NUB_HANDLE))
									{
										printf("Got invalid nub handle back from get build info.\n");
										retcode = NTV2_REMOTE_ACCESS_NO_CARD;
									}
									ULWord result (ntohl(pDGBIP->result));
									if (result)
									{
										deNBOifyAndCopyGetDriverBuildInformation (outBuildInfo, pDGBIP->buildInfo);
										// printf("Driver get buildinfo succeeded.\n");
									}
									else // failed on remote side
									{
										// printf("Driver get buildinfo failed on remote side.\n");
										retcode = NTV2_REMOTE_ACCESS_DRIVER_GET_BUILD_INFO_FAILED;
									}
								}
								else // Not a write register response packet, count it and discard it.
								{
									++gIgnoredNTV2pkts;
									retcode = NTV2_REMOTE_ACCESS_NOT_DRIVER_GET_BUILD_INFO;
								}
							}
							else // Non ntv2 packet on our port.
							{
								// NOTE: Defragmentation of jumbo packets would probably go here.
								retcode = NTV2_REMOTE_ACCESS_NON_NUB_PKT;
								NBFAIL("Non-nub packet on NTV2 port, socket=" << Socket());
							}
			}	//	switch
		}	//	else
	}	//	if NBOify OK
	delete pPkt;
	return retcode;
}

bool NTV2LegacyNubClient::NTV2DownloadTestPatternRemote	(const NTV2Channel channel, const NTV2PixelFormat testPatternFBF,
													const UWord signalMask, const bool testPatDMAEnb, const ULWord testPatNum)
{
	if (!IsConnected())
		return false;

	// Construct open query
	NTV2NubPkt *pPkt (BuildDownloadTestPatternQueryPacket(	Handle(),  _nubProtocolVersion,  channel,  testPatternFBF,
															signalMask,  testPatDMAEnb,  testPatNum));
	if (!pPkt)
		return false;   //  NTV2_REMOTE_ACCESS_OUT_OF_MEMORY

	int retcode (NTV2_REMOTE_ACCESS_SUCCESS);
	int len (pPkt  ?  int(sizeof(NTV2NubPktHeader) + pPkt->hdr.dataLength)  :  0);
	// Send it
	if (NBOifyNTV2NubPkt(pPkt)) 
	{
		if (-1 == ::sendall(Socket(), reinterpret_cast<char*>(pPkt), &len))
			{NBFAIL("'sendall' failed, socket=" << Socket() << ", len=" << len << ": " << strerror(errno));  retcode = NTV2_REMOTE_ACCESS_SEND_ERR;}
		else
		{
			// Wait for response
			int numbytes (::recvtimeout_sec(Socket(), reinterpret_cast<char*>(pPkt), sizeof(NTV2NubPkt), 2)); // 2 second timeout

			switch (numbytes)
			{
				case  0:	// Remote side closed connection
							retcode = NTV2_REMOTE_ACCESS_CONNECTION_CLOSED;
							NBFAIL("'recvtimeout_sec' returned zero bytes:  remote access connection closed");
							break;

				case -1:	// error occurred
							NBFAIL("'recvtimeout_sec' failed on socket " << Socket() << ": " << strerror(errno));
							retcode = NTV2_REMOTE_ACCESS_RECV_ERR;
							break;
			
				case -2:	// timeout occurred
							retcode = NTV2_REMOTE_ACCESS_TIMEDOUT;
							NBFAIL("'recvtimeout_sec' timed out on socket " << Socket());
							break;
			
				default:	// got some data.  Open response packet?
							if (deNBOifyNTV2NubPkt(pPkt, ULWord(numbytes))) 
							{
								if (isNubDownloadTestPatternRespPacket(reinterpret_cast<NTV2NubPkt*>(pPkt))) 
								{
									// printf("Got a download test pattern response packet\n");
									NTV2DownloadTestPatternPayload * pDTPP(reinterpret_cast<NTV2DownloadTestPatternPayload*>(getNubPktPayload(pPkt)));
									// Did card go away?
									LWord hdl (LWord(ntohl(pDTPP->handle)));
									// printf("hdl = %d\n", hdl);
									if (hdl == LWord(INVALID_NUB_HANDLE))
									{
										printf("Got invalid nub handle back from download test pattern.\n");
										retcode = NTV2_REMOTE_ACCESS_NO_CARD;
									}
									ULWord result (ntohl(pDTPP->result));
									if (result)
									{
										// printf("Download test pattern remote succeeded.\n");
									}
									else // write register failed on remote side
									{
										printf("Download test pattern failed on remote side.\n");
										retcode = NTV2_REMOTE_ACCESS_DOWNLOAD_TEST_PATTERN_FAILED;
									}
								}
								else // Not a write register response packet, count it and discard it.
								{
									++gIgnoredNTV2pkts;
									retcode = NTV2_REMOTE_ACCESS_NOT_DOWNLOAD_TEST_PATTERN;
								}
							}
							else // Non ntv2 packet on our port.
							{
								// NOTE: Defragmentation of jumbo packets would probably go here.
								retcode = NTV2_REMOTE_ACCESS_NON_NUB_PKT;
								NBFAIL("Non-nub packet on NTV2 port, socket=" << Socket());
							}
			}	//	switch
		}	//	else
	}	//	if NBOify OK
	delete pPkt;
	return retcode == NTV2_REMOTE_ACCESS_SUCCESS;
}

bool NTV2LegacyNubClient::NTV2ReadRegisterMultiRemote	(const ULWord numRegs, ULWord & outFailedRegNum,  NTV2RegInfo outRegs[])
{
	if (!IsConnected())	//	if (!SocketValid())
		return false;	//	NTV2_REMOTE_ACCESS_NOT_CONNECTED		//	No connection
	if (!numRegs)
		return true;	//	Nothing to do
	// Connected?

	// Construct open query
	NTV2NubPkt *pPkt (BuildReadRegisterMultiQueryPacket (Handle(),  _nubProtocolVersion,  numRegs,  outRegs));
	if (!pPkt)
		return false;	//	NTV2_REMOTE_ACCESS_OUT_OF_MEMORY;

	int retcode (NTV2_REMOTE_ACCESS_SUCCESS);
	int len (pPkt  ?  int(sizeof(NTV2NubPktHeader) + pPkt->hdr.dataLength)  :  0);
	// Send it
	if(NBOifyNTV2NubPkt(pPkt)) 
	{
		if (-1 == ::sendall(Socket(), reinterpret_cast<char*>(pPkt), &len))
			{NBFAIL("'sendall' failed, socket=" << Socket() << ", len=" << len << ": " << strerror(errno));  retcode = NTV2_REMOTE_ACCESS_SEND_ERR;}
		else
		{
			// Wait for response
			LWord maxPktFetchsize = 0;
			maxPktFetchsize += sizeof(NTV2ReadWriteMultiRegisterPayloadHeader);
			maxPktFetchsize += sizeof(NTV2NubPktHeader);
			maxPktFetchsize += (LWord)strlen(nubQueryRespStr(_nubProtocolVersion, eNubReadRegisterMultiRespPkt)) + 1;
			maxPktFetchsize += numRegs * sizeof(NTV2ReadWriteRegisterSingle);

			int numbytestotal(0), defragAttemptCount(0);
		defrag:
			int numbytes(::recvtimeout_usec(Socket(), ((char *)pPkt)+numbytestotal, maxPktFetchsize - numbytestotal, 250000L)); // 500 msec timeout

			if (++defragAttemptCount > 3)
			{
				NBFAIL("defrag timeout on socket " << Socket());
				retcode = NTV2_REMOTE_ACCESS_TIMEDOUT;
			}
			else switch (numbytes)
			{
				case  0:	// Remote side closed connection
							retcode = NTV2_REMOTE_ACCESS_CONNECTION_CLOSED;
							NBFAIL("'recvtimeout_sec' returned zero bytes:  remote access connection closed");
							break;

				case -1:	// error occurred
							NBFAIL("'recvtimeout_sec' failed on socket " << Socket() << ": " << strerror(errno));
							retcode = NTV2_REMOTE_ACCESS_RECV_ERR;
							break;
			
				case -2:	// timeout occurred
							retcode = NTV2_REMOTE_ACCESS_TIMEDOUT;
							NBFAIL("'recvtimeout_sec' timed out on socket " << Socket());
							break;
			
				default:	// got some data.  Open response packet?
							numbytestotal += numbytes;
							if (numbytestotal <  maxPktFetchsize)
								goto defrag;

							if (deNBOifyNTV2NubPkt(pPkt, ULWord(numbytestotal))) 
							{
								if (isNubReadRegisterMultiRespPacket((NTV2NubPkt *)pPkt)) 
								{
									// printf("Got a read register multi response packet\n");
									NTV2ReadWriteMultiRegisterPayload * pRWMRP;
									pRWMRP = reinterpret_cast<NTV2ReadWriteMultiRegisterPayload*>(getNubPktPayload(pPkt));
									// Did card go away?
									LWord hdl = ntohl(pRWMRP->payloadHeader.handle);
									// printf("hdl = %d\n", hdl);
									if (hdl == (LWord)INVALID_NUB_HANDLE)
									{
										NBWARN("Received invalid nub handle from ReadRegMulti");
										retcode = NTV2_REMOTE_ACCESS_NO_CARD;
									}
									ULWord result = ntohl (pRWMRP->payloadHeader.result);
									outFailedRegNum = ntohl(pRWMRP->payloadHeader.whichRegisterFailed);
	
									if (result)
									{
										NBINFO("ReadRegMulti succeeded, numRegs=" << numRegs);
										for (ULWord i(0);  i < numRegs;  i++)
										{
											outRegs[i].registerNumber = ntohl(pRWMRP->aRegs[i].registerNumber);
											outRegs[i].registerValue = ntohl(pRWMRP->aRegs[i].registerValue);
										}
									}
									else // Read register failed on remote side
									{
										// Guard against buffer overrun from remote side
										ULWord maxRegs (outFailedRegNum > numRegs ? numRegs : outFailedRegNum);
										NBFAIL("ReadRegMulti failed on remote side, regNum=" << outFailedRegNum);
										retcode = NTV2_REMOTE_ACCESS_READ_REG_MULTI_FAILED;
										for (ULWord i(0);  i < maxRegs;  i++)
											outRegs[i].registerValue = ntohl(pRWMRP->aRegs[i].registerValue);
									}
								}
								else // Not a write register response packet, count it and discard it.
								{
									++gIgnoredNTV2pkts;
									retcode = NTV2_REMOTE_ACCESS_NOT_READ_REG_MULTI;
									NBWARN("Received non-ReadRegMulti response pkt, " << gIgnoredNTV2pkts << " ignored pkts");
								}
							}
							else // Non ntv2 packet on our port.
							{
								// NOTE: Defragmentation of jumbo packets would probably go here.
								retcode = NTV2_REMOTE_ACCESS_NON_NUB_PKT;
								NBFAIL("Non-nub packet on NTV2 port, socket=" << Socket());
							}
			}	//	else switch
		}	//	else
	}	//	if NBOify OK
	delete pPkt;
	return retcode < 0 ? false : true;
}	//	NTV2ReadRegisterMultiRemote


#define AsNTV2GetRegs(_p_)		(reinterpret_cast<NTV2GetRegisters*>(_p_))
#define AsNTV2SetRegs(_p_)		(reinterpret_cast<NTV2SetRegisters*>(_p_))
#define AsNTV2BankGetSet(_p_)	(reinterpret_cast<NTV2BankSelGetSetRegs*>(_p_))
#define AsACStatus(_p_)			(reinterpret_cast<AUTOCIRCULATE_STATUS*>(_p_))
#define AsACFrameStamp(_p_)		(reinterpret_cast<FRAME_STAMP*>(_p_))
#define AsACTransfer(_p_)		(reinterpret_cast<AUTOCIRCULATE_TRANSFER*>(_p_))
#define AsNTV2Bitstream(_p_)	(reinterpret_cast<NTV2Bitstream*>(_p_))


bool NTV2LegacyNubClient::NTV2DMATransferRemote(	const NTV2DMAEngine inDMAEngine,	const bool inIsRead,
											const ULWord inFrameNumber,			NTV2Buffer & inOutBuffer,
											const ULWord inCardOffsetBytes,		const ULWord inNumSegments,
											const ULWord inSegmentHostPitch,	const ULWord inSegmentCardPitch,
											const bool inSynchronous)
{
	if (!IsConnected())
		return false;	//	No connection
	return false;
}

bool NTV2LegacyNubClient::NTV2MessageRemote (NTV2_HEADER *	pInMessage)
{
	if (!IsConnected())
		return false;	//	Not connected
	if (!pInMessage)
		return false;	//	NULL message pointer
	return false;	//	TBD
}

ostream & NTV2LegacyNubClient::Print (ostream & oss) const
{
	NTV2RPCAPI::Print(oss);
	oss << " devNdx=" << _remoteIndex << " sockfd=" << Socket()
		<< " handle=" << Handle() << " protocolVers=" << ProtocolVersion();
	return oss;
}


bool NTV2LegacyNubClient::NTV2QueryDevices (NTV2StringList & outDeviceInfos)
{
	outDeviceInfos.clear();
	return true;
}	//	NTV2QueryDevices

extern "C"
{
	/**
		@brief	Instantiates a new client instance to talk to a remote server.
				-	pDLLHandle:	A pointer to the DLL/dylib/so handle.
				-	inParams: A const reference to the NTV2ConnectParams that specify the "what" and "how" the new client should access the server device.
				-	inHostSDKVersion:	Specifies the NTV2 SDK version the caller was compiled with.
		@return	A pointer to the new client instance if successful, or nullptr (zero) upon failure.
	**/
	NTV2RPCClientAPI* CreateClient (void * pInDLLHandle, const NTV2ConnectParams & inParams, const uint32_t inHostSDKVersion)
	{	(void) pInDLLHandle; (void)inHostSDKVersion;
		return new NTV2LegacyNubClient(inParams);
	}
}
