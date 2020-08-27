/**
	@file		ntv2nubaccess.cpp
	@brief		Implementation of NTV2 "nub" functions that connect/open/close/send/receive data.
	@copyright	(C) 2006-2020 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include "ajatypes.h"

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
#include "ntv2discover.h"
#include "ntv2nubpktcom.h"
#include "ntv2nubaccess.h"
#include "ntv2endian.h"
#include "ntv2publicinterface.h"
#include "ntv2testpatterngen.h"
#include "ajabase/system/debug.h"
#include "ajabase/common/common.h"	//	NoDevice
#include "ajabase/system/atomic.h"	//	NoDevice
#include "ajabase/system/memory.h"	//	NoDevice
#include "ajabase/system/lock.h"	//	NoDevice
#include <iomanip>

using namespace std;

// max number of bytes we can get at once 
#define MAXDATASIZE (sizeof(NTV2NubPktHeader) + NTV2_NUBPKT_MAX_DATASIZE)

#if defined(NTV2_NUB_CLIENT_SUPPORT)
#define INSTP(_p_)			xHEX0N(uint64_t(_p_),16)
#define	NBFAIL(__x__)		AJA_sERROR  (AJA_DebugUnit_RPCClient, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define	NBWARN(__x__)		AJA_sWARNING(AJA_DebugUnit_RPCClient, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define	NBNOTE(__x__)		AJA_sNOTICE (AJA_DebugUnit_RPCClient, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define	NBINFO(__x__)		AJA_sINFO   (AJA_DebugUnit_RPCClient, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define	NBDBG(__x__)		AJA_sDEBUG  (AJA_DebugUnit_RPCClient, INSTP(this) << "::" << AJAFUNC << ": " << __x__)

//	Specific NTV2RPCAPI implementation to talk to remote host
class AJAExport NTV2NubRPCAPI : public NTV2RPCAPI
{
	//	Instance Methods
	public:
		NTV2NubRPCAPI()
			:	_sockfd				(-1),
				_remoteHandle		(INVALID_NUB_HANDLE),
				_nubProtocolVersion	(ntv2NubProtocolVersionNone),
				_remoteIndex		(0)
		{
		}
		AJA_VIRTUAL									~NTV2NubRPCAPI()				{NTV2Disconnect();}
		AJA_VIRTUAL inline	bool					IsConnected	(void) const		{return SocketValid()  &&  HandleValid();}
		AJA_VIRTUAL inline	AJASocket				Socket (void) const				{return _sockfd;}
		AJA_VIRTUAL			bool					SocketValid (void) const;
		AJA_VIRTUAL inline	LWord					Handle (void) const				{return _remoteHandle;}
		AJA_VIRTUAL			bool					HandleValid (void) const;
		AJA_VIRTUAL inline	NTV2NubProtocolVersion	ProtocolVersion (void) const	{return _nubProtocolVersion;}
		AJA_VIRTUAL	inline	UWord					DeviceIndex (void) const		{return _remoteIndex;}
		AJA_VIRTUAL	int		NTV2Connect (const string & inHostname, const UWord inDeviceIndexNum);
		AJA_VIRTUAL	int		NTV2Disconnect (void);
		AJA_VIRTUAL	int		NTV2ReadRegisterRemote	(const ULWord regNum, ULWord & outRegValue, const ULWord regMask = 0xFFFFFFFF, const ULWord regShift = 0);
		AJA_VIRTUAL	int		NTV2WriteRegisterRemote	(const ULWord regNum, const ULWord regValue, const ULWord regMask = 0xFFFFFFFF, const ULWord regShift = 0);
		AJA_VIRTUAL	int		NTV2AutoCirculateRemote	(AUTOCIRCULATE_DATA & autoCircData);
		AJA_VIRTUAL	int		NTV2WaitForInterruptRemote	(const INTERRUPT_ENUMS eInterrupt, const ULWord timeOutMs);
		AJA_VIRTUAL	int		NTV2DriverGetBitFileInformationRemote	(BITFILE_INFO_STRUCT & outInfo, const NTV2BitFileType inType);
		AJA_VIRTUAL	int		NTV2DriverGetBuildInformationRemote	(BUILD_INFO_STRUCT & outBuildInfo);
		AJA_VIRTUAL	int		NTV2DownloadTestPatternRemote	(const NTV2Channel channel, const NTV2PixelFormat testPatternFBF,
															const UWord signalMask, const bool testPatDMAEnb, const ULWord testPatNum);
		AJA_VIRTUAL	int		NTV2ReadRegisterMultiRemote	(const ULWord numRegs, ULWord & outFailedRegNum,  NTV2RegInfo aRegs[]);
		AJA_VIRTUAL	int		NTV2GetDriverVersionRemote	(ULWord & outDriverVersion)		{(void) outDriverVersion;	return -1;}
		AJA_VIRTUAL	int		NTV2DMATransferRemote		(const NTV2DMAEngine inDMAEngine,	const bool inIsRead,
														const ULWord inFrameNumber,			ULWord * pFrameBuffer,
														const ULWord inCardOffsetBytes,		const ULWord inTotalByteCount,
														const ULWord inNumSegments,			const ULWord inSegmentHostPitch,
														const ULWord inSegmentCardPitch,	const bool inSynchronous);
		AJA_VIRTUAL	int		NTV2MessageRemote	(NTV2_HEADER *	pInMessage);
		AJA_VIRTUAL ostream &	Print (ostream & oss) const;
	protected:
		AJA_VIRTUAL	int		NTV2OpenRemote (const UWord inDeviceIndex);
		AJA_VIRTUAL	int		NTV2CloseRemote (void)	{return -1;}

	//	Instance Data
	private:
		AJASocket				_sockfd;				///< @brief	Socket descriptor
		LWord					_remoteHandle;			///< @brief	Remote host handle
		NTV2NubProtocolVersion	_nubProtocolVersion;	///< @brief	Protocol version
		UWord					_remoteIndex;			///< @brief	Remote device index number
};	//	NTV2NubRPCAPI

//	Factory method to create NTV2NubRPCAPI instance
NTV2RPCAPI * NTV2RPCAPI::MakeNTV2NubRPCAPI (const std::string & inSpec, const std::string & inPort)
{
	NTV2NubRPCAPI * pResult(new NTV2NubRPCAPI);
	if (!pResult)
		return pResult;
	//	Open the device on the remote system...
	pResult->NTV2Connect(inSpec, inPort.empty() ? 0 : UWord(aja::stoul(inPort)));
	return pResult;
}


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


int NTV2NubRPCAPI::NTV2Connect (const string & inHostName, const UWord inDeviceIndexNum)
{
	//	Get the host info
	struct hostent * he(::gethostbyname(inHostName.c_str()));
	if (!he)
	{
#ifndef MSWindows
		herror("gethostbyname");
#endif
		return -1;
	}

	_sockfd = ::socket(PF_INET, SOCK_STREAM, 0);
	if (!SocketValid()) 
	{
		NBFAIL("'socket' failed, socket=" << Socket() << ": " << ::strerror(errno));
		return -1;
	}

	struct sockaddr_in their_addr;
	their_addr.sin_family = AF_INET;    // host byte order 
	their_addr.sin_port = htons(NTV2NUBPORT);  // short, network byte order 
	their_addr.sin_addr = *((struct in_addr *)he->h_addr);
	::memset(&(their_addr.sin_zero), '\0', 8);  // zero the rest of the struct 

	int retval (::connect(Socket(), reinterpret_cast<struct sockaddr*>(&their_addr), sizeof(struct sockaddr)));
	if (retval == -1) 
	{
		NBFAIL("'connect' failed: " << ::strerror(errno) << ", socket=" << Socket() << ", hostName='" << inHostName << "'");
		NTV2Disconnect();
	}
	if (retval < 0)
		return retval;

	_hostname = inHostName;
	retval = NTV2OpenRemote(inDeviceIndexNum);
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
	return retval;

}	//	NTV2NubRPCAPI::NTV2Connect

bool NTV2NubRPCAPI::SocketValid (void) const
{
	return Socket() != AJASocket(-1);
}

bool NTV2NubRPCAPI::HandleValid (void) const
{
	return Handle() != INVALID_NUB_HANDLE;
}


int NTV2NubRPCAPI::NTV2Disconnect (void)
{
	NTV2CloseRemote();
	if (SocketValid())
	{
		#ifdef MSWindows
			closesocket(_sockfd);
		#else
			close(_sockfd);
		#endif
		_sockfd = -1;
	}
	return -1;

}	//	NTV2NubRPCAPI::NTV2Disconnect


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


int NTV2NubRPCAPI::NTV2OpenRemote (const UWord inDeviceIndex)
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
								static ULWord ignoredNTV2pkts;
								++ignoredNTV2pkts;
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
}	//	NTV2OpenRemote


int NTV2NubRPCAPI::NTV2ReadRegisterRemote (const ULWord regNum, ULWord & outRegValue, const ULWord regMask, const ULWord regShift)
{
	// Connected?
	if (!SocketValid())
		return NTV2_REMOTE_ACCESS_NOT_CONNECTED; 

	// Construct open query
	NTV2NubPkt *pPkt (BuildReadRegisterQueryPacket (Handle(), ProtocolVersion(),  regNum, regMask, regShift));
	if (!pPkt)
		return NTV2_REMOTE_ACCESS_OUT_OF_MEMORY;

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
								static unsigned long ignoredNTV2pkts;
								++ignoredNTV2pkts;
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
	return retcode;
}	//	NTV2NubRPCAPI::NTV2ReadRegisterRemote


int NTV2NubRPCAPI::NTV2WriteRegisterRemote (const ULWord regNum, const ULWord regValue, const ULWord regMask, const ULWord regShift)
{
	// Connected?
	if (!SocketValid())
		return NTV2_REMOTE_ACCESS_NOT_CONNECTED; 

	// Construct open query
	NTV2NubPkt *pPkt (BuildWriteRegisterQueryPacket (Handle(),  _nubProtocolVersion,  regNum, regValue, regMask, regShift));
	if (!pPkt)
		return NTV2_REMOTE_ACCESS_OUT_OF_MEMORY;

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
								static unsigned long ignoredNTV2pkts;
								++ignoredNTV2pkts;
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
	return retcode;
}	//	NTV2NubRPCAPI::NTV2WriteRegisterRemote


int NTV2NubRPCAPI::NTV2AutoCirculateRemote (AUTOCIRCULATE_DATA & autoCircData)
{
	// Connected?
	if (!SocketValid())
		return NTV2_REMOTE_ACCESS_NOT_CONNECTED; 

	// Construct autocirculate query packet.
	NTV2NubPkt *pPkt (BuildAutoCirculateQueryPacket (Handle(),  _nubProtocolVersion,  autoCircData));
	if (!pPkt)
		return NTV2_REMOTE_ACCESS_OUT_OF_MEMORY;

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
								static unsigned long ignoredNTV2pkts;
								++ignoredNTV2pkts;
								retcode = NTV2_REMOTE_ACCESS_NOT_AUTOCIRC_RESP;
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
	return retcode;
}	//	NTV2NubRPCAPI::NTV2AutoCirculateRemote


int NTV2NubRPCAPI::NTV2WaitForInterruptRemote (const INTERRUPT_ENUMS eInterrupt, const ULWord timeOutMs)
{
	// Connected?
	if (!SocketValid())
		return NTV2_REMOTE_ACCESS_NOT_CONNECTED; 

	// Construct open query
	NTV2NubPkt *pPkt(BuildWaitForInterruptQueryPacket(Handle(), _nubProtocolVersion, eInterrupt, timeOutMs));
	if (!pPkt)
		return NTV2_REMOTE_ACCESS_OUT_OF_MEMORY;

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
									static unsigned long ignoredNTV2pkts;
									++ignoredNTV2pkts;
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
	return retcode;
}	//	NTV2NubRPCAPI::NTV2WaitForInterruptRemote


int NTV2NubRPCAPI::NTV2DriverGetBitFileInformationRemote (BITFILE_INFO_STRUCT & outInfo,  const NTV2BitFileType inType)
{
	// Connected?
	if (!SocketValid())
		return NTV2_REMOTE_ACCESS_NOT_CONNECTED; 

	// Construct open query
	NTV2NubPkt *pPkt (BuildDriverGetBitFileInformationQueryPacket (Handle(),  _nubProtocolVersion,  outInfo,  inType));
	if (!pPkt)
		return NTV2_REMOTE_ACCESS_OUT_OF_MEMORY;

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
									static unsigned long ignoredNTV2pkts;
									++ignoredNTV2pkts;
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
}	//	NTV2NubRPCAPI::NTV2DriverGetBitFileInformationRemote


int NTV2NubRPCAPI::NTV2DriverGetBuildInformationRemote (BUILD_INFO_STRUCT & outBuildInfo)
{
	// Connected?
	if (!SocketValid())
		return NTV2_REMOTE_ACCESS_NOT_CONNECTED; 

	// Construct open query
	NTV2NubPkt *pPkt (BuildDriverGetBuildInformationQueryPacket(Handle(),  ProtocolVersion()));
	if (!pPkt)
		return NTV2_REMOTE_ACCESS_OUT_OF_MEMORY;

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
									static unsigned long ignoredNTV2pkts;
									++ignoredNTV2pkts;
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
}	//	NTV2DriverGetBuildInformationRemote


int NTV2NubRPCAPI::NTV2DownloadTestPatternRemote	(const NTV2Channel channel, const NTV2PixelFormat testPatternFBF,
													const UWord signalMask, const bool testPatDMAEnb, const ULWord testPatNum)
{
	// Connected?
	if (!SocketValid())
		return NTV2_REMOTE_ACCESS_NOT_CONNECTED; 

	// Construct open query
	NTV2NubPkt *pPkt (BuildDownloadTestPatternQueryPacket(	Handle(),  _nubProtocolVersion,  channel,  testPatternFBF,
															signalMask,  testPatDMAEnb,  testPatNum));
	if (!pPkt)
		return NTV2_REMOTE_ACCESS_OUT_OF_MEMORY;

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
									static unsigned long ignoredNTV2pkts;
									++ignoredNTV2pkts;
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
	return retcode;
}	//	NTV2NubRPCAPI::NTV2DownloadTestPatternRemote

int NTV2NubRPCAPI::NTV2ReadRegisterMultiRemote	(const ULWord numRegs, ULWord & outFailedRegNum,  NTV2RegInfo outRegs[])
{
	// Connected?
	if (!SocketValid())
		return NTV2_REMOTE_ACCESS_NOT_CONNECTED; 

	// Construct open query
	NTV2NubPkt *pPkt (BuildReadRegisterMultiQueryPacket (Handle(),  _nubProtocolVersion,  numRegs,  outRegs));
	if (!pPkt)
		return NTV2_REMOTE_ACCESS_OUT_OF_MEMORY;

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
									static unsigned long ignoredNTV2pkts;
									++ignoredNTV2pkts;
									retcode = NTV2_REMOTE_ACCESS_NOT_READ_REG_MULTI;
									NBWARN("Received non-ReadRegMulti response pkt, " << ignoredNTV2pkts << " ignored pkts");
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
	return retcode;
}	//	NTV2NubRPCAPI::NTV2ReadRegisterMultiRemote

int NTV2NubRPCAPI::NTV2DMATransferRemote (	const NTV2DMAEngine inDMAEngine,	const bool inIsRead,
											const ULWord inFrameNumber,			ULWord * pFrameBuffer,
											const ULWord inCardOffsetBytes,		const ULWord inTotalByteCount,
											const ULWord inNumSegments,			const ULWord inSegmentHostPitch,
											const ULWord inSegmentCardPitch,	const bool inSynchronous)
{	(void) inDMAEngine; (void) inIsRead;	(void) inFrameNumber; (void) pFrameBuffer; (void) inCardOffsetBytes;
	(void) inTotalByteCount; (void) inNumSegments; (void) inSegmentHostPitch; (void) inSegmentCardPitch; (void) inSynchronous;
	return -1;	//	TBD
}

int	NTV2NubRPCAPI::NTV2MessageRemote (NTV2_HEADER *	pInMessage)
{	(void) pInMessage;
	return -1;	//	TBD
}

ostream &	NTV2NubRPCAPI::Print (ostream & oss) const
{
	NTV2RPCAPI::Print(oss);
	oss << " devNdx=" << _remoteIndex << " sockfd=" << Socket()
		<< " handle=" << Handle() << " protocolVers=" << ProtocolVersion();
	return oss;
}




#define FAKE_DEVICE_SHARE_NAME		"ntv2shmdev"
//	Fake Device Shared Memory Layout:
//	[1K Hdr][---- 128MB Reg Memory ----][-------------------- 1024MB FrameBuffer Memory --------------------][2MB InternalAC]

typedef struct _AJANTV2FakeDevice
{	//								//	1024-byte header:
	uint32_t	fMagicID;			//	'0DEV'
	uint32_t	fVersion;			//	Version 1
	uint32_t	fNumRegBytes;		//	FIRST:	128MB of Register memory
	uint32_t	fNumFBBytes;		//	SECOND:	1024MB of FrameBuffer memory
	uint32_t	fNumACBytes;		//	THIRD:	2MB for INTERNAL_AUTOCIRCULATE_STRUCT
	uint32_t	fClientRefCount;	//	Initially 0
	uint32_t	fReserved[1000];	//	Filler to make 1024 byte header
} AJANTV2FakeDevice;

static const uint32_t		kNumRegBytes		(128ULL * 1024ULL * 1024ULL);	//	128MB
static const uint32_t		kNumFBBytes			(1024ULL * 1024ULL * 1024ULL);	//	1GB
static const uint32_t		kNumACBytes			(1024ULL * 1024ULL * 2ULL);		//	2MB
static const ULWord			kOffsetToRegBytes	(sizeof(AJANTV2FakeDevice));
static const size_t			kFakeDevTotalBytes	(sizeof(AJANTV2FakeDevice) + kNumRegBytes + kNumFBBytes + kNumACBytes);
static const uint32_t		kFakeDevCookie		(0xFACEDE00);
static AJANTV2FakeDevice *	spFakeDevice		(AJA_NULL);
static AJALock				sLock;

//	Specific NTV2RPCAPI implementation to talk to software device
class AJAExport NTV2SoftwareDevice : public NTV2RPCAPI
{
	//	Instance Methods
	public:
		NTV2SoftwareDevice()
			:	_remoteIndex(0)
		{
		}
		AJA_VIRTUAL										~NTV2SoftwareDevice()			{NTV2Disconnect();}
		AJA_VIRTUAL inline	bool						IsConnected	(void) const		{return spFakeDevice ? true : false;}
		AJA_VIRTUAL inline	AJASocket					Socket (void) const				{return -1;}
		AJA_VIRTUAL inline	LWord						Handle (void) const				{return 0;}
		AJA_VIRTUAL inline	NTV2NubProtocolVersion		ProtocolVersion (void) const	{return maxKnownProtocolVersion;}
		AJA_VIRTUAL	inline	UWord						DeviceIndex (void) const		{return _remoteIndex;}
		AJA_VIRTUAL	int		NTV2Connect					(const string & inName, const UWord inNum, const string & inQuery);
		AJA_VIRTUAL	int		NTV2Disconnect				(void);
		AJA_VIRTUAL	int		NTV2ReadRegisterRemote		(const ULWord regNum, ULWord & outRegValue, const ULWord regMask = 0xFFFFFFFF, const ULWord regShift = 0);
		AJA_VIRTUAL	int		NTV2WriteRegisterRemote		(const ULWord regNum, const ULWord regValue, const ULWord regMask = 0xFFFFFFFF, const ULWord regShift = 0);
		AJA_VIRTUAL	int		NTV2AutoCirculateRemote		(AUTOCIRCULATE_DATA & autoCircData);
		AJA_VIRTUAL	int		NTV2WaitForInterruptRemote	(const INTERRUPT_ENUMS eInterrupt, const ULWord timeOutMs);
		AJA_VIRTUAL	int		NTV2DriverGetBitFileInformationRemote	(BITFILE_INFO_STRUCT & outInfo, const NTV2BitFileType inType);
		AJA_VIRTUAL	int		NTV2DriverGetBuildInformationRemote		(BUILD_INFO_STRUCT & outBuildInfo);
		AJA_VIRTUAL	int		NTV2DownloadTestPatternRemote(const NTV2Channel channel, const NTV2FrameBufferFormat testPatternFBF,
														const UWord signalMask, const bool testPatDMAEnb, const ULWord testPatNum);
		AJA_VIRTUAL	int		NTV2ReadRegisterMultiRemote	(const ULWord numRegs, ULWord & outFailedRegNum,  NTV2RegInfo outRegs[]);
		AJA_VIRTUAL	int		NTV2GetDriverVersionRemote	(ULWord & outDriverVersion)		{(void) outDriverVersion;	return -1;}
		AJA_VIRTUAL	int		NTV2DMATransferRemote		(const NTV2DMAEngine inDMAEngine,	const bool inIsRead,
														const ULWord inFrameNumber,			ULWord * pFrameBuffer,
														const ULWord inCardOffsetBytes,		const ULWord inByteCount,
														const ULWord inNumSegments,			const ULWord inSegmentHostPitch,
														const ULWord inSegmentCardPitch,	const bool inSynchronous);
		AJA_VIRTUAL	int		NTV2MessageRemote			(NTV2_HEADER * pInMessage);

	//	Protected & Private Instance Methods
	protected:
		AJA_VIRTUAL	int		NTV2OpenRemote (const UWord inDeviceIndex);
		AJA_VIRTUAL	int		NTV2CloseRemote (void)	{return -1;}
		AJA_VIRTUAL bool	AllMemory					(NTV2_POINTER & outAllMemory) const;
		AJA_VIRTUAL bool	RegMemory					(NTV2_POINTER & outRegMemory) const;
		AJA_VIRTUAL bool	FBMemory					(NTV2_POINTER & outFBMemory) const;
		AJA_VIRTUAL bool	ACMemory					(NTV2_POINTER & outACMemory) const;
	private:
		AJA_VIRTUAL	void	InitRegs (void);

	//	Instance Data
	private:
		UWord	_remoteIndex;			///< @brief	Remote device index number
};	//	NTV2SoftwareDevice

NTV2RPCAPI * NTV2RPCAPI::MakeNTV2SoftwareDevice (const string & inSpec, const std::string & inPort)
{
	NTV2SoftwareDevice * pResult(new NTV2SoftwareDevice);
	if (!pResult)
		return pResult;
	//	Open the fake device...
	if (pResult->NTV2Connect(inSpec, inPort.empty() ? 0 : UWord(aja::stoul(inPort)), ""))
		{delete pResult; pResult = AJA_NULL;}	//	Failed
	return pResult;
}

int NTV2SoftwareDevice::NTV2Connect (const string & inName, const UWord inNum, const string & inQuery)
{	(void) inNum; (void) inQuery;
	if (inName != FAKE_DEVICE_SHARE_NAME)
		return -1;	//	Wrong name
	_hostname = inName;
	if (!sLock.IsValid())
		return -1;	//	No lock object
	AJAAutoLock lock(&sLock);

	try
	{	//	Allocate the shared memory region
		if (spFakeDevice == AJA_NULL)
		{
			//	Allocate the shared memory storage
			size_t sizeInBytes(kFakeDevTotalBytes);
			spFakeDevice = reinterpret_cast<AJANTV2FakeDevice*>(AJAMemory::AllocateShared (&sizeInBytes, FAKE_DEVICE_SHARE_NAME));
			if (spFakeDevice == AJA_NULL || spFakeDevice == (void*)(-1))
			{
				NBFAIL("AJAMemory::AllocateShared failed, name='" << FAKE_DEVICE_SHARE_NAME << "', size=" << sizeInBytes);
				spFakeDevice = AJA_NULL;
				NTV2Disconnect();
				return -1;
			}

			if (sizeInBytes < kFakeDevTotalBytes)
			{
				NBFAIL("'" << FAKE_DEVICE_SHARE_NAME << "' created, but size " << sizeInBytes << " < " << kFakeDevTotalBytes);
				NTV2Disconnect();
				return -1;
			}

			//	Check version
			if (spFakeDevice->fVersion == 0)
			{
				::memset(spFakeDevice, 0, kFakeDevTotalBytes);
				spFakeDevice->fMagicID			= kFakeDevCookie;
				spFakeDevice->fVersion			= 1;
				spFakeDevice->fNumRegBytes		= kNumRegBytes;
				spFakeDevice->fNumFBBytes		= kNumFBBytes;
				spFakeDevice->fNumACBytes		= kNumACBytes;
				spFakeDevice->fClientRefCount	= 0;
				InitRegs();
				NBNOTE("'" << FAKE_DEVICE_SHARE_NAME << "' created, initialized, size=" << sizeInBytes);
			}

			//	Must be correct version
			if (spFakeDevice->fVersion != 1)
			{
				NBFAIL("'" << FAKE_DEVICE_SHARE_NAME << "' created, but version " << spFakeDevice->fVersion << " != 1");
				NTV2Disconnect();
				return -1;
			}

            spFakeDevice->fClientRefCount++;
		}
	}
	catch(...)
	{
		NBFAIL("Exception during creation of '" << FAKE_DEVICE_SHARE_NAME << "'");
		NTV2Disconnect();
		return -1;
	}
	NBINFO("'" << Name() << "' ready, vers=" << spFakeDevice->fVersion
			<< " refCnt=" << spFakeDevice->fClientRefCount
			<< " reg=" << spFakeDevice->fNumRegBytes << " fb=" << spFakeDevice->fNumFBBytes
			<< " ac=" << spFakeDevice->fNumACBytes);
	return 0;
}

int NTV2SoftwareDevice::NTV2Disconnect (void)
{
	return 0;
}

int NTV2SoftwareDevice::NTV2OpenRemote (const UWord inDeviceIndex)
{	(void) inDeviceIndex;
	return 0;
}

bool NTV2SoftwareDevice::AllMemory (NTV2_POINTER & outAllMemory) const
{
	outAllMemory.Allocate(0);
	AJAAutoLock lock(&sLock);
	if (!IsConnected())
		return false;	//	Not connected
	if (spFakeDevice->fVersion != 1)
		return false;
	return outAllMemory.Set(spFakeDevice, kFakeDevTotalBytes);
}

bool NTV2SoftwareDevice::RegMemory (NTV2_POINTER & outRegMemory) const
{
	outRegMemory.Allocate(0);
	NTV2_POINTER allMemory;
	AJAAutoLock lock(&sLock);
	if (!AllMemory(allMemory))
		return false;
	return outRegMemory.Set(allMemory.GetHostAddress(kOffsetToRegBytes), spFakeDevice->fNumRegBytes);
}

bool NTV2SoftwareDevice::FBMemory (NTV2_POINTER & outFBMemory) const
{
	outFBMemory.Allocate(0);
	NTV2_POINTER allMemory;
	AJAAutoLock lock(&sLock);
	if (!AllMemory(allMemory))
		return false;
	return outFBMemory.Set(allMemory.GetHostAddress(kOffsetToRegBytes + spFakeDevice->fNumRegBytes),
													spFakeDevice->fNumFBBytes);
}

bool NTV2SoftwareDevice::ACMemory (NTV2_POINTER & outACMemory) const
{
	outACMemory.Allocate(0);
	NTV2_POINTER allMemory;
	AJAAutoLock lock(&sLock);
	if (!AllMemory(allMemory))
		return false;
	return outACMemory.Set(allMemory.GetHostAddress(kOffsetToRegBytes + spFakeDevice->fNumFBBytes),
													spFakeDevice->fNumACBytes);
}

int NTV2SoftwareDevice::NTV2ReadRegisterRemote (const ULWord inRegNum, ULWord & outRegValue, const ULWord inRegMask, const ULWord inRegShift)
{
	outRegValue = 0;
	if (inRegShift > 31)
		return -1;	//	Bad shift value

	NTV2_POINTER regMemory;
	AJAAutoLock lock(&sLock);
	if (!RegMemory(regMemory))
		return -1;
	const ULWord* pReg(reinterpret_cast<const ULWord*>(regMemory.GetHostAddress(sizeof(ULWord) * inRegNum)));
	if (!pReg)
		return -1;	//	Bad reg num

	ULWord value(*pReg & inRegMask);
	if (inRegShift)
		value >>= inRegShift;
	outRegValue = value;
	return 0;
}

int NTV2SoftwareDevice::NTV2WriteRegisterRemote (const ULWord inRegNum, const ULWord inRegVal, const ULWord inRegMask, const ULWord inRegShift)
{
	if (inRegShift > 31)
		return -1;

//	TODO:	Some registers aren't actually written until the next VBI.
//			These will need to be queued and processed as a group in a properly-timed VBI thread.
	NTV2_POINTER regMemory;
	AJAAutoLock lock(&sLock);
	if (!RegMemory(regMemory))
		return -1;
	ULWord* pReg(reinterpret_cast<ULWord*>(regMemory.GetHostAddress(sizeof(ULWord) * inRegNum)));
	if (!pReg)
		return -1;
//	ULWord oldValue(*pReg & inRegMask);
	ULWord newValue(inRegVal & inRegMask);
	if (inRegShift)
		newValue <<= inRegShift;
	*pReg = newValue;
	return 0;
}


int NTV2SoftwareDevice::NTV2AutoCirculateRemote (AUTOCIRCULATE_DATA & autoCircData)
{
	::memset(&autoCircData, 0, sizeof(autoCircData));
	AJAAutoLock lock(&sLock);
	if (!spFakeDevice)
		return -1;
	if (spFakeDevice->fVersion != 1)
		return -1;
	NTV2_POINTER fakeDevMem(spFakeDevice, kFakeDevTotalBytes);
	NTV2_POINTER fakeDevAutoCircMem(fakeDevMem.GetHostAddress(kOffsetToRegBytes + spFakeDevice->fNumFBBytes),
									spFakeDevice->fNumACBytes);
	//INTERNAL_AUTOCIRCULATE_STRUCT * pACStruct = reinterpret_cast<INTERNAL_AUTOCIRCULATE_STRUCT*>(fakeDevAutoCircMem.GetHostPointer());
	//	TBD
	return 0;
}

int NTV2SoftwareDevice::NTV2WaitForInterruptRemote (const INTERRUPT_ENUMS eInterrupt, const ULWord timeOutMs)
{	(void)eInterrupt; (void)timeOutMs;
	AJAAutoLock lock(&sLock);
	if (!spFakeDevice)
		return -1;
	if (spFakeDevice->fVersion != 1)
		return -1;
	//	TBD
	return 0;
}

int NTV2SoftwareDevice::NTV2DriverGetBitFileInformationRemote (BITFILE_INFO_STRUCT & outInfo, const NTV2BitFileType inType)
{	(void) inType;
	::memset(&outInfo, 0, sizeof(outInfo));
	return -1;
}

int NTV2SoftwareDevice::NTV2DriverGetBuildInformationRemote (BUILD_INFO_STRUCT & outInfo)
{
	::memset(&outInfo, 0, sizeof(outInfo));
	return -1;
}

int NTV2SoftwareDevice::NTV2DownloadTestPatternRemote (const NTV2Channel channel, const NTV2PixelFormat testPatternFBF,
														const UWord signalMask, const bool testPatDMAEnb, const ULWord testPatNum)
{	(void)channel; (void)testPatternFBF;(void)signalMask;(void)testPatDMAEnb;(void)testPatNum;
	NTV2TestPatternGen foo;
	return -1;
}

int NTV2SoftwareDevice::NTV2ReadRegisterMultiRemote (const ULWord numRegs, ULWord & outFailedRegister, NTV2RegInfo outRegs[])
{	(void)numRegs;(void)outFailedRegister;(void)outRegs;
	return -1;
}

int NTV2SoftwareDevice::NTV2DMATransferRemote (	const NTV2DMAEngine inDMAEngine,	const bool inIsRead,
												const ULWord inFrameNumber,			ULWord * pFrameBuffer,
												const ULWord inCardOffsetBytes,		const ULWord inTotalByteCount,
												const ULWord inNumSegments,			const ULWord inSegmentHostPitch,
												const ULWord inSegmentCardPitch,	const bool inSynchronous)
{
	NTV2_POINTER fbMemory;
	AJAAutoLock lock(&sLock);
	if (!FBMemory(fbMemory))
		return -1;
	if (inDMAEngine < NTV2_DMA1  ||  inDMAEngine > NTV2_DMA_FIRST_AVAILABLE)
		return -1;	//	Bad DMA engine
	if (!inSynchronous)
		return -1;	//	Must be synchronous

	if (inNumSegments)
	{
		NTV2SegmentedXferInfo	xferInfo;
		xferInfo.setDestOffset(inCardOffsetBytes).setSegmentCount(inNumSegments);
		xferInfo.setSegmentLength(inTotalByteCount / inNumSegments);
		xferInfo.setSourceOffset(0);
		xferInfo.setSourcePitch(inSegmentHostPitch);
		xferInfo.setDestOffset(inFrameNumber * 8ULL*1024ULL*1024ULL + inCardOffsetBytes);	//	!!! ASSUMES 8MB FRAMES!
		xferInfo.setDestPitch(inSegmentCardPitch);
		const ULWord totalBytes(xferInfo.getTotalBytes());
		if (inIsRead)
		{
			NTV2_POINTER destination(pFrameBuffer, totalBytes);
			return destination.CopyFrom(fbMemory, xferInfo) ? 0 : -1;
		}
		else
		{
			NTV2_POINTER source(pFrameBuffer, totalBytes);
			return fbMemory.CopyFrom(source, xferInfo) ? 0 : -1;
		}
	}
	else
	{
		if (inIsRead)
		{
			NTV2_POINTER destination(pFrameBuffer, inTotalByteCount);
			return destination.CopyFrom(fbMemory, inFrameNumber * 8UL*1024ULL*1024ULL,  0,  inTotalByteCount) ? 0 : -1;
		}
		else
		{
			NTV2_POINTER source(pFrameBuffer, inTotalByteCount);
			return fbMemory.CopyFrom(source, inFrameNumber * 8UL*1024ULL*1024ULL,  0,  inTotalByteCount) ? 0 : -1;
		}
	}
}

int NTV2SoftwareDevice::NTV2MessageRemote (NTV2_HEADER * pInMessage)
{
	if (!pInMessage)
		return -1;
	if (!pInMessage->IsValid())
		return -1;
	return -1;
}

void NTV2SoftwareDevice::InitRegs (void)
{
	NTV2WriteRegisterRemote (kRegGlobalControl, 0x30000202);  // Reg 0  // Frame Rate: 59.94, Frame Geometry: 1920x1080, Standard: 1080p, Reference Source: Reference In, Ch 2 link B 1080p 50/60: Off, LEDs ...., Register Clocking: Sync To Field, Ch 1 RP-188 output: Enabled, Ch 2 RP-188 output: Enabled, Color Correction: Channel: 1 Bank 0
	NTV2WriteRegisterRemote (kRegCh1Control, 0x00200080);  // Reg 1  // Mode: Display, Format: NTV2_FBF_10BIT_YCBCR, Channel: Disabled, Viper Squeeze: Normal, Flip Vertical: Normal, DRT Display: Off, Frame Buffer Mode: Frame, Dither: No dithering, Frame Size: 8 MB, RGB Range: Black = 0, VANC Data Shift: Normal 8 bit conversion
	NTV2WriteRegisterRemote (kRegCh1PCIAccessFrame, 0x00000001);  // Reg 2
	NTV2WriteRegisterRemote (kRegCh1OutputFrame, 0x0000000E);  // Reg 3
	NTV2WriteRegisterRemote (kRegCh1InputFrame, 0x00000003);  // Reg 4
	NTV2WriteRegisterRemote (kRegCh2Control, 0x00200080);  // Reg 5  // Mode: Display, Format: NTV2_FBF_10BIT_YCBCR, Channel: Disabled, Viper Squeeze: Normal, Flip Vertical: Normal, DRT Display: Off, Frame Buffer Mode: Frame, Dither: No dithering, Frame Size: 8 MB, RGB Range: Black = 0, VANC Data Shift: Normal 8 bit conversion
	NTV2WriteRegisterRemote (kRegCh2PCIAccessFrame, 0x00000005);  // Reg 6
	NTV2WriteRegisterRemote (kRegCh2OutputFrame, 0x00000004);  // Reg 7
	NTV2WriteRegisterRemote (kRegCh2InputFrame, 0x00000007);  // Reg 8
	NTV2WriteRegisterRemote (kRegMixer1Coefficient, 0x00010000);  // Reg 11
	NTV2WriteRegisterRemote (kRegFlatMatteValue, 0x20080200);  // Reg 13  // Flat Matte Cb: 200, Flat Matte Y: 1C0, Flat Matte Cr: 200
	NTV2WriteRegisterRemote (kRegOutputTimingControl, 0x08001000);  // Reg 14
	NTV2WriteRegisterRemote (kRegFlashProgramReg, 0x12345678);  // Reg 17
	NTV2WriteRegisterRemote (kRegLineCount, 0x00000142);  // Reg 18
	NTV2WriteRegisterRemote (kRegAud1Delay, 0x1FDF0000);  // Reg 19
	NTV2WriteRegisterRemote (kRegVidIntControl, 0x001C01C7);  // Reg 20  // Output 1 Vertical Enable: Y, Input 1 Vertical Enable: Y, Input 2 Vertical Enable: Y, Audio Out Wrap Interrupt Enable: N, Audio In Wrap Interrupt Enable: N, Wrap Rate Interrupt Enable: Y, UART Tx Interrupt EnableY, UART Rx Interrupt EnableY, UART Rx Interrupt ClearInactive, UART 2 Tx Interrupt EnableN, Output 2 Vertical Enable: Y, Output 3 Vertical Enable: Y, Output 4 Vertical Enable: Y, Output 4 Vertical Clear: Inactive, Output 3 Vertical Clear: Inactive, Output 2 Vertical Clear: Inactive, UART Tx Interrupt ClearInactive, Wrap Rate Interrupt ClearInactive, UART 2 Tx Interrupt ClearInactive, Audio Out Wrap Interrupt ClearInactive, Input 2 Vertical Clear: Inactive, Input 1 Vertical Clear: Inactive, Output 1 Vertical Clear: Inactive
	NTV2WriteRegisterRemote (kRegStatus, 0x00200000);  // Reg 21  // Input 1 Vertical Blank: Inactive, Input 1 Field ID: 1, Input 1 Vertical Interrupt: Inactive, Input 2 Vertical Blank: Inactive, Input 2 Field ID: 0, Input 2 Vertical Interrupt: Inactive, Output 1 Vertical Blank: Inactive, Output 1 Field ID: 0, Output 1 Vertical Interrupt: Inactive, Output 2 Vertical Blank: Inactive, Output 2 Field ID: 0, Output 2 Vertical Interrupt: Inactive, Aux Vertical Interrupt: Inactive, I2C 1 Interrupt: Inactive, I2C 2 Interrupt: Inactive, Chunk Rate Interrupt: Inactive, Wrap Rate Interrupt: Inactive, Audio Out Wrap Interrupt: Inactive, Audio 50Hz Interrupt: Inactive
	NTV2WriteRegisterRemote (kRegInputStatus, 0x0F000000);  // Reg 22  // Input 1 Frame Rate: Unknown, Input 1 Geometry: Unknown, Input 1 Scan Mode: Interlaced, Input 2 Frame Rate: Unknown, Input 2 Geometry: Unknown, Input 2 Scan Mode: Interlaced, Reference Frame Rate: Unknown, Reference Geometry: Unknown, Reference Scan Mode: Interlaced, AES Channel 1-2: Invalid, AES Channel 3-4: Invalid, AES Channel 5-6: Invalid, AES Channel 7-8: Invalid
	NTV2WriteRegisterRemote (kRegAud1Control, 0xA8F00300);  // Reg 24  // Audio Capture: Disabled, Audio Loopback: Disabled, Audio Input: Disabled, Audio Output: Disabled, Audio Embedder SDIOut1: Enabled, Audio Embedder SDIOut2: Enabled, A/V Sync Mode: Disabled, AES Rate Converter: Enabled, Audio Buffer Format: 16-Channel , 48kHz, 48kHz Support, Embedded Support, 8-Channel Support, K-box, Monitor: Ch 1/2, K-Box Input: BNC, K-Box: Present, Cable: BNC, Audio Buffer Size: 4 MB
	NTV2WriteRegisterRemote (kRegAud1SourceSelect, 0xF0004321);  // Reg 25  // Audio Source: Embedded Groups 1 and 2, Embedded Source Select: Video Input 1, AES Sync Mode bit (fib): Disabled, PCM disabled: N, Erase head enable: N, Embedded Clock Select: Board Reference, 3G audio source: Data stream 1
	NTV2WriteRegisterRemote (kRegAud1Counter, 0xE618F83A);  // Reg 28
	NTV2WriteRegisterRemote (kRegRP188InOut1DBB, 0xFF000000);  // Reg 29  // RP188: No RP-188 received, Bypass: Disabled, Filter: FF, DBB: 00 00
	NTV2WriteRegisterRemote (kRegDMA1HostAddr, 0x01D22F78);  // Reg 32
	NTV2WriteRegisterRemote (kRegDMA1LocalAddr, 0x01525840);  // Reg 33
	NTV2WriteRegisterRemote (kRegDMA2HostAddr, 0x0103E210);  // Reg 36
	NTV2WriteRegisterRemote (kRegDMA2LocalAddr, 0x007E9000);  // Reg 37
	NTV2WriteRegisterRemote (kRegDMAControl, 0x01243C00);  // Reg 48  // DMA 1 Int Active?: N, DMA 2 Int Active?: N, DMA 3 Int Active?: N, DMA 4 Int Active?: N, Bus Error Int Active?: N, DMA 1 Busy?: N, DMA 2 Busy?: N, DMA 3 Busy?: N, DMA 4 Busy?: N, Strap: Not Installed, Firmware Rev: 0x3C (60), Gen: 2, Lanes: 4
	NTV2WriteRegisterRemote (kRegBoardID, 0x10538200);  // Reg 50
	NTV2WriteRegisterRemote (kRegReserved54, 0x38543030);  // Reg 54
	NTV2WriteRegisterRemote (kRegReserved55, 0x32393735);  // Reg 55
	NTV2WriteRegisterRemote (kRegXenaxFlashControlStatus, 0x40009C0B);  // Reg 58
	NTV2WriteRegisterRemote (kRegXenaxFlashAddress, 0x000000FC);  // Reg 59
	NTV2WriteRegisterRemote (kRegXenaxFlashDOUT, 0x6B26B241);  // Reg 61
	NTV2WriteRegisterRemote (kRegCPLDVersion, 0x00000003);  // Reg 63  // CPLD Version: 3, Failsafe Bitfile Loaded: No, Force Reload: N
	NTV2WriteRegisterRemote (kRegRP188InOut2DBB, 0xFF0000FF);  // Reg 64  // RP188: No RP-188 received, Bypass: Disabled, Filter: FF, DBB: 00 FF
	NTV2WriteRegisterRemote (kRegCanDoStatus, 0x00000003);  // Reg 67  // Has CanConnect Xpt Route ROM: Y
	NTV2WriteRegisterRemote (kRegRS422Control, 0x0000010B);  // Reg 72
	NTV2WriteRegisterRemote (kRegBitfileDate, 0x20200325);  // Reg 88  // Bitfile Date: 03/25/2020
	NTV2WriteRegisterRemote (kRegBitfileTime, 0x00151600);  // Reg 89  // Bitfile Time: 15:16:00
	NTV2WriteRegisterRemote (kRegSysmonVccIntDieTemp, 0x53C1ABE1);  // Reg 119  // Die Temperature: 64.97 Celcius  (148.94 Fahrenheit, Core Voltage:  0.98 Volts DC
	NTV2WriteRegisterRemote (kRegInternalExternalVoltage, 0x000798A4);  // Reg 120
	NTV2WriteRegisterRemote (kRegHDMIOut3DControl, 0x00000080);  // Reg 124
	NTV2WriteRegisterRemote (kRegSDIOut1Control, 0x01040084);  // Reg 129  // Video Standard : 1080p, 2Kx1080 mode: 1920x1080, HBlank RGB Range: Black=0x40, 12G enable: N, 6G enalbe: N, 3G enable: Y, 3G mode: a, VPID insert enable: N, VPID overwrite enable: N, DS 1 audio source: Subsystem 5, DS 2 audio source: Subsystem 1
	NTV2WriteRegisterRemote (kRegSDIOut2Control, 0x01040084);  // Reg 130  // Video Standard : 1080p, 2Kx1080 mode: 1920x1080, HBlank RGB Range: Black=0x40, 12G enable: N, 6G enalbe: N, 3G enable: Y, 3G mode: a, VPID insert enable: N, VPID overwrite enable: N, DS 1 audio source: Subsystem 5, DS 2 audio source: Subsystem 1
	NTV2WriteRegisterRemote (kRegSDIOut3Control, 0x01040004);  // Reg 169  // Video Standard : 1080p, 2Kx1080 mode: 1920x1080, HBlank RGB Range: Black=0x04, 12G enable: N, 6G enalbe: N, 3G enable: Y, 3G mode: a, VPID insert enable: N, VPID overwrite enable: N, DS 1 audio source: Subsystem 5, DS 2 audio source: Subsystem 1
	NTV2WriteRegisterRemote (kRegSDIOut4Control, 0x01040004);  // Reg 170  // Video Standard : 1080p, 2Kx1080 mode: 1920x1080, HBlank RGB Range: Black=0x04, 12G enable: N, 6G enalbe: N, 3G enable: Y, 3G mode: a, VPID insert enable: N, VPID overwrite enable: N, DS 1 audio source: Subsystem 5, DS 2 audio source: Subsystem 1
	NTV2WriteRegisterRemote (kRegAudioOutputSourceMap, 0x00003210);  // Reg 190  // AES Outputs 1-4 Source: AudSys1, Audio Channels 1-4, AES Outputs 5-8 Source: AudSys1, Audio Channels 5-8, AES Outputs 9-12 Source: AudSys1, Audio Channels 9-12, AES Outputs 13-16 Source: AudSys1, Audio Channels 13-16, Analog Audio Monitor Output Source: AudSys1, Channels 1-2, HDMI 2-Chl Audio Output Source: AudSys1, Channels 1-2, or HDMI 8-Chl Audio Output 1-4 Source: AudSys1, Channels 1-4, or HDMI 8-Chl Audio Output 5-8 Source: AudSys1, Channels 1-4
	NTV2WriteRegisterRemote (kRegRP188InOut5Bits0_31_2, 0x63030608);  // Reg 210
	NTV2WriteRegisterRemote (kRegRP188InOut5Bits32_63_2, 0x00000004);  // Reg 211
	NTV2WriteRegisterRemote (kRegRP188InOut6Bits0_31_2, 0x63030608);  // Reg 212
	NTV2WriteRegisterRemote (kRegRP188InOut6Bits32_63_2, 0x00000004);  // Reg 213
	NTV2WriteRegisterRemote (kRegRP188InOut7Bits0_31_2, 0x63030608);  // Reg 214
	NTV2WriteRegisterRemote (kRegRP188InOut7Bits32_63_2, 0x00000004);  // Reg 215
	NTV2WriteRegisterRemote (kRegRP188InOut8Bits0_31_2, 0x63030608);  // Reg 216
	NTV2WriteRegisterRemote (kRegRP188InOut8Bits32_63_2, 0x00000004);  // Reg 217
	NTV2WriteRegisterRemote (kRegSDIInput3GStatus, 0x000C0000);  // Reg 232
	NTV2WriteRegisterRemote (kRegAud2Control, 0x80C00300);  // Reg 240  // Audio Capture: Disabled, Audio Loopback: Disabled, Audio Input: Disabled, Audio Output: Disabled, A/V Sync Mode: Disabled, AES Rate Converter: Enabled, Audio Buffer Format: 6-Channel , 48kHz, 48kHz Support, Embedded Support, 8-Channel Support, K-box, Monitor: Ch 1/2, K-Box Input: BNC, K-Box: Absent, Cable: BNC, Audio Buffer Size: 4 MB
	NTV2WriteRegisterRemote (kRegRS4222Control, 0x00000010);  // Reg 246
	NTV2WriteRegisterRemote (kRegFlatMatte2Value, 0x18080240);  // Reg 249  // Flat Matte Cb: 240, Flat Matte Y: 1C0, Flat Matte Cr: 180
	NTV2WriteRegisterRemote (kRegSDITransmitControl, 0xF0000000);  // Reg 256  // (Bi-directional SDI not supported)
	NTV2WriteRegisterRemote (kRegCh3Control, 0x00000080);  // Reg 257  // Mode: Display, Format: NTV2_FBF_10BIT_YCBCR, Channel: Disabled, Viper Squeeze: Normal, Flip Vertical: Normal, DRT Display: Off, Frame Buffer Mode: Frame, Dither: No dithering, Frame Size: 2 MB, RGB Range: Black = 0, VANC Data Shift: Normal 8 bit conversion
	NTV2WriteRegisterRemote (kRegCh4Control, 0x00000080);  // Reg 260  // Mode: Display, Format: NTV2_FBF_10BIT_YCBCR, Channel: Disabled, Viper Squeeze: Normal, Flip Vertical: Normal, DRT Display: Off, Frame Buffer Mode: Frame, Dither: No dithering, Frame Size: 2 MB, RGB Range: Black = 0, VANC Data Shift: Normal 8 bit conversion
	NTV2WriteRegisterRemote (kRegStatus2, 0x00015400);  // Reg 265  // Input 3 Vertical Blank: Inactive, Input 3 Field ID: 0, Input 3 Vertical Interrupt: Inactive, Input 4 Vertical Blank: Inactive, Input 4 Field ID: 0, Input 4 Vertical Interrupt: Inactive, Input 5 Vertical Blank: Active, Input 5 Field ID: 0, Input 5 Vertical Interrupt: Inactive, Input 6 Vertical Blank: Active, Input 6 Field ID: 0, Input 6 Vertical Interrupt: Inactive, Input 7 Vertical Blank: Active, Input 7 Field ID: 0, Input 7 Vertical Interrupt: Inactive, Input 8 Vertical Blank: Active, Input 8 Field ID: 0, Input 8 Vertical Interrupt: Inactive, Output 5 Vertical Blank: Inactive, Output 5 Field ID: 0, Output 5 Vertical Interrupt: Inactive, Output 6 Vertical Blank: Inactive, Output 6 Field ID: 0, Output 6 Vertical Interrupt: Inactive, Output 7 Vertical Blank: Inactive, Output 7 Field ID: 0, Output 7 Vertical Interrupt: Inactive, Output 8 Vertical Blank: Inactive, Output 8 Field ID: 0, Output 8 Vertical Interrupt: Inactive, HDMI In Hot-Plug Detect Interrupt: Inactive, HDMI In Chip Interrupt: Inactive
	NTV2WriteRegisterRemote (kRegVidIntControl2, 0x0000FF06);  // Reg 266  // Input 3 Vertical Enable: Y, Input 4 Vertical Enable: Y, Input 5 Vertical Enable: Y, Input 6 Vertical Enable: Y, Input 7 Vertical Enable: Y, Input 8 Vertical Enable: Y, Output 5 Vertical Enable: Y, Output 6 Vertical Enable: Y, Output 7 Vertical Enable: Y, Output 8 Vertical Enable: Y, Output 8 Vertical Clear: Inactive, Output 7 Vertical Clear: Inactive, Output 6 Vertical Clear: Inactive, Output 5 Vertical Clear: Inactive, Input 8 Vertical Clear: Inactive, Input 7 Vertical Clear: Inactive, Input 6 Vertical Clear: Inactive, Input 5 Vertical Clear: Inactive, Input 4 Vertical Clear: Inactive, Input 3 Vertical Clear: Inactive
	NTV2WriteRegisterRemote (kRegGlobalControl2, 0xBC021009);  // Reg 267  // Reference source bit 4: Set, Quad Mode Channel 1-4: Set, Quad Mode Channel 5-8: Set, Independent Channel Mode: Not Set, 2MB Frame Support: Supported, Audio Mixer: Not Present, Is DNXIV Product: N, Audio 1 Play/Capture Mode: Off, Audio 2 Play/Capture Mode: Off, Audio 3 Play/Capture Mode: Off, Audio 4 Play/Capture Mode: Off, Audio 5 Play/Capture Mode: Off, Audio 6 Play/Capture Mode: Off, Audio 7 Play/Capture Mode: Off, Audio 8 Play/Capture Mode: Off, Ch 3 RP188 Output: Enabled, Ch 4 RP188 Output: Enabled, Ch 5 RP188 Output: Disabled, Ch 6 RP188 Output: Enabled, Ch 7 RP188 Output: Enabled, Ch 8 RP188 Output: Enabled, Ch 4 1080p50/p60 Link-B Mode: Disabled, Ch 6 1080p50/p60 Link-B Mode: Disabled, Ch 8 1080p50/p60 Link-B Mode: Disabled, Ch 1/2 2SI Mode: Disabled, Ch 2/3 2SI Mode: Disabled, Ch 3/4 2SI Mode: Disabled, Ch 4/5 2SI Mode: Disabled, 2SI Min Align Delay 1-4: Disabled, 2SI Min Align Delay 5-8: Disabled
	NTV2WriteRegisterRemote (kRegRP188InOut3DBB, 0x020000FF);  // Reg 268  // RP188: No RP-188 received, Bypass: Disabled, Filter: 02, DBB: 00 FF
	NTV2WriteRegisterRemote (kRegRP188InOut4DBB, 0x020000FF);  // Reg 273  // RP188: No RP-188 received, Bypass: Disabled, Filter: 02, DBB: 00 FF
	NTV2WriteRegisterRemote (kRegAud3Control, 0x80000300);  // Reg 278  // Audio Capture: Disabled, Audio Loopback: Disabled, Audio Input: Disabled, Audio Output: Disabled, Audio Embedder SDIOut3: Enabled, Audio Embedder SDIOut4: Enabled, A/V Sync Mode: Disabled, AES Rate Converter: Enabled, Audio Buffer Format: 6-Channel , 48kHz, 48kHz Support, No Embedded Support, 6-Channel Support, K-box, Monitor: Ch 1/2, K-Box Input: BNC, K-Box: Absent, Cable: BNC, Audio Buffer Size: 4 MB
	NTV2WriteRegisterRemote (kRegAud4Control, 0x80000300);  // Reg 279  // Audio Capture: Disabled, Audio Loopback: Disabled, Audio Input: Disabled, Audio Output: Disabled, A/V Sync Mode: Disabled, AES Rate Converter: Enabled, Audio Buffer Format: 6-Channel , 48kHz, 48kHz Support, No Embedded Support, 6-Channel Support, K-box, Monitor: Ch 1/2, K-Box Input: BNC, K-Box: Absent, Cable: BNC, Audio Buffer Size: 4 MB
	NTV2WriteRegisterRemote (kRegSDIInput3GStatus2, 0x00000100);  // Reg 287
	NTV2WriteRegisterRemote (kRegInputStatus2, 0x00008000);  // Reg 288  // Input 3 Scan Mode: Interlaced, Input 3 Frame Rate: Unknown, Input 3 Geometry: Unknown, Input 4 Scan Mode: Progressive, Input 4 Frame Rate: Unknown, Input 4 Geometry: Unknown
	NTV2WriteRegisterRemote (kRegSDIOut5Control, 0x01000084);  // Reg 337  // Video Standard : 1080p, 2Kx1080 mode: 1920x1080, HBlank RGB Range: Black=0x40, 12G enable: N, 6G enalbe: N, 3G enable: Y, 3G mode: a, VPID insert enable: N, VPID overwrite enable: N, DS 1 audio source: Subsystem 1, DS 2 audio source: Subsystem 1
	NTV2WriteRegisterRemote (kRegRP188InOut5Bits0_31, 0x6B030607);  // Reg 340
	NTV2WriteRegisterRemote (kRegRP188InOut5Bits32_63, 0x00000004);  // Reg 341
	NTV2WriteRegisterRemote (kRegRP188InOut5DBB, 0x020D0002);  // Reg 342  // RP188: Unselected RP-188 received +LTC +VITC, Bypass: Disabled, Filter: 02, DBB: 00 02
	NTV2WriteRegisterRemote (kRegLTC5EmbeddedBits0_31, 0x63030608);  // Reg 344
	NTV2WriteRegisterRemote (kRegLTC5EmbeddedBits32_63, 0x00000004);  // Reg 345
	NTV2WriteRegisterRemote (kRegReserved353, 0x00000CA3);  // Reg 353
	NTV2WriteRegisterRemote (kRegLUTV2Control, 0x000F1F00);  // Reg 376  // (Register data relevant for V2 LUT, this device has V0LUT)
	NTV2WriteRegisterRemote (kRegGlobalControlCh2, 0x00000202);  // Reg 377  // Frame Rate: NTV2_FRAMERATE_5994, Frame Geometry: NTV2_FG_1920x1080, Standard: NTV2_STANDARD_1080p
	NTV2WriteRegisterRemote (kRegGlobalControlCh3, 0x00000202);  // Reg 378  // Frame Rate: NTV2_FRAMERATE_5994, Frame Geometry: NTV2_FG_1920x1080, Standard: NTV2_STANDARD_1080p
	NTV2WriteRegisterRemote (kRegGlobalControlCh4, 0x00000202);  // Reg 379  // Frame Rate: NTV2_FRAMERATE_5994, Frame Geometry: NTV2_FG_1920x1080, Standard: NTV2_STANDARD_1080p
	NTV2WriteRegisterRemote (kRegGlobalControlCh5, 0x00000202);  // Reg 380  // Frame Rate: NTV2_FRAMERATE_5994, Frame Geometry: NTV2_FG_1920x1080, Standard: NTV2_STANDARD_1080p
	NTV2WriteRegisterRemote (kRegGlobalControlCh6, 0x00000202);  // Reg 381  // Frame Rate: NTV2_FRAMERATE_5994, Frame Geometry: NTV2_FG_1920x1080, Standard: NTV2_STANDARD_1080p
	NTV2WriteRegisterRemote (kRegGlobalControlCh7, 0x00000202);  // Reg 382  // Frame Rate: NTV2_FRAMERATE_5994, Frame Geometry: NTV2_FG_1920x1080, Standard: NTV2_STANDARD_1080p
	NTV2WriteRegisterRemote (kRegGlobalControlCh8, 0x00000202);  // Reg 383  // Frame Rate: NTV2_FRAMERATE_5994, Frame Geometry: NTV2_FG_1920x1080, Standard: NTV2_STANDARD_1080p
	NTV2WriteRegisterRemote (kRegCh5Control, 0x00000001);  // Reg 384  // Mode: Capture, Format: NTV2_FBF_10BIT_YCBCR, Channel: Enabled, Viper Squeeze: Normal, Flip Vertical: Normal, DRT Display: Off, Frame Buffer Mode: Frame, Dither: No dithering, Frame Size: 2 MB, RGB Range: Black = 0, VANC Data Shift: Normal 8 bit conversion
	NTV2WriteRegisterRemote (kRegCh5InputFrame, 0x00000006);  // Reg 386
	NTV2WriteRegisterRemote (kRegCh6Control, 0x00000001);  // Reg 388  // Mode: Capture, Format: NTV2_FBF_10BIT_YCBCR, Channel: Enabled, Viper Squeeze: Normal, Flip Vertical: Normal, DRT Display: Off, Frame Buffer Mode: Frame, Dither: No dithering, Frame Size: 2 MB, RGB Range: Black = 0, VANC Data Shift: Normal 8 bit conversion
	NTV2WriteRegisterRemote (kRegCh6OutputFrame, 0x00000008);  // Reg 389
	NTV2WriteRegisterRemote (kRegCh7Control, 0x00000001);  // Reg 392  // Mode: Capture, Format: NTV2_FBF_10BIT_YCBCR, Channel: Enabled, Viper Squeeze: Normal, Flip Vertical: Normal, DRT Display: Off, Frame Buffer Mode: Frame, Dither: No dithering, Frame Size: 2 MB, RGB Range: Black = 0, VANC Data Shift: Normal 8 bit conversion
	NTV2WriteRegisterRemote (kRegCh8Control, 0x00000001);  // Reg 396  // Mode: Capture, Format: NTV2_FBF_10BIT_YCBCR, Channel: Enabled, Viper Squeeze: Normal, Flip Vertical: Normal, DRT Display: Off, Frame Buffer Mode: Frame, Dither: No dithering, Frame Size: 2 MB, RGB Range: Black = 0, VANC Data Shift: Normal 8 bit conversion
	NTV2WriteRegisterRemote (kRegSDIIn5VPIDA, 0x0180CA89);  // Reg 410  // Raw Value: 0x89CA8001, Version: 1, Standard: 1080 3G Level A, Video Format: 1080p59.94a, Progressive Transport: Yes, Progressive Picture: Yes, Picture Rate: 59.94, Aspect Ratio: 16x9, Sampling: YCbCr 4:2:2, Channel: 1, Bit Depth: 10, 3Ga: Yes, Two Sample Interleave: No, Xfer Characteristics: SDR, Colorimetry: Rec709, Luminance: YCbCr
	NTV2WriteRegisterRemote (kRegSDIIn5VPIDB, 0x4180CA89);  // Reg 411  // Raw Value: 0x89CA8041, Version: 1, Standard: 1080 3G Level A, Video Format: 1080p59.94a, Progressive Transport: Yes, Progressive Picture: Yes, Picture Rate: 59.94, Aspect Ratio: 16x9, Sampling: YCbCr 4:2:2, Channel: 2, Bit Depth: 10, 3Ga: Yes, Two Sample Interleave: No, Xfer Characteristics: SDR, Colorimetry: Rec709, Luminance: YCbCr
	NTV2WriteRegisterRemote (kRegSDIIn6VPIDA, 0x0180CA89);  // Reg 412  // Raw Value: 0x89CA8001, Version: 1, Standard: 1080 3G Level A, Video Format: 1080p59.94a, Progressive Transport: Yes, Progressive Picture: Yes, Picture Rate: 59.94, Aspect Ratio: 16x9, Sampling: YCbCr 4:2:2, Channel: 1, Bit Depth: 10, 3Ga: Yes, Two Sample Interleave: No, Xfer Characteristics: SDR, Colorimetry: Rec709, Luminance: YCbCr
	NTV2WriteRegisterRemote (kRegSDIIn6VPIDB, 0x4180CA89);  // Reg 413  // Raw Value: 0x89CA8041, Version: 1, Standard: 1080 3G Level A, Video Format: 1080p59.94a, Progressive Transport: Yes, Progressive Picture: Yes, Picture Rate: 59.94, Aspect Ratio: 16x9, Sampling: YCbCr 4:2:2, Channel: 2, Bit Depth: 10, 3Ga: Yes, Two Sample Interleave: No, Xfer Characteristics: SDR, Colorimetry: Rec709, Luminance: YCbCr
	NTV2WriteRegisterRemote (kRegRP188InOut6Bits0_31, 0x38000400);  // Reg 416
	NTV2WriteRegisterRemote (kRegRP188InOut6DBB, 0x020D0200);  // Reg 418  // RP188: Unselected RP-188 received +LTC +VITC, Bypass: Disabled, Filter: 02, DBB: 02 00
	NTV2WriteRegisterRemote (kRegLTC6EmbeddedBits0_31, 0x63030608);  // Reg 419
	NTV2WriteRegisterRemote (kRegLTC6EmbeddedBits32_63, 0x00000004);  // Reg 420
	NTV2WriteRegisterRemote (kRegSDIIn7VPIDA, 0x0180CA89);  // Reg 421  // Raw Value: 0x89CA8001, Version: 1, Standard: 1080 3G Level A, Video Format: 1080p59.94a, Progressive Transport: Yes, Progressive Picture: Yes, Picture Rate: 59.94, Aspect Ratio: 16x9, Sampling: YCbCr 4:2:2, Channel: 1, Bit Depth: 10, 3Ga: Yes, Two Sample Interleave: No, Xfer Characteristics: SDR, Colorimetry: Rec709, Luminance: YCbCr
	NTV2WriteRegisterRemote (kRegSDIIn7VPIDB, 0x4180CA89);  // Reg 422  // Raw Value: 0x89CA8041, Version: 1, Standard: 1080 3G Level A, Video Format: 1080p59.94a, Progressive Transport: Yes, Progressive Picture: Yes, Picture Rate: 59.94, Aspect Ratio: 16x9, Sampling: YCbCr 4:2:2, Channel: 2, Bit Depth: 10, 3Ga: Yes, Two Sample Interleave: No, Xfer Characteristics: SDR, Colorimetry: Rec709, Luminance: YCbCr
	NTV2WriteRegisterRemote (kRegRP188InOut7Bits0_31, 0x38000400);  // Reg 425
	NTV2WriteRegisterRemote (kRegRP188InOut7DBB, 0x020D00FF);  // Reg 427  // RP188: Unselected RP-188 received +LTC +VITC, Bypass: Disabled, Filter: 02, DBB: 00 FF
	NTV2WriteRegisterRemote (kRegLTC7EmbeddedBits0_31, 0x63030608);  // Reg 428
	NTV2WriteRegisterRemote (kRegLTC7EmbeddedBits32_63, 0x00000004);  // Reg 429
	NTV2WriteRegisterRemote (kRegSDIIn8VPIDA, 0x0180CA89);  // Reg 430  // Raw Value: 0x89CA8001, Version: 1, Standard: 1080 3G Level A, Video Format: 1080p59.94a, Progressive Transport: Yes, Progressive Picture: Yes, Picture Rate: 59.94, Aspect Ratio: 16x9, Sampling: YCbCr 4:2:2, Channel: 1, Bit Depth: 10, 3Ga: Yes, Two Sample Interleave: No, Xfer Characteristics: SDR, Colorimetry: Rec709, Luminance: YCbCr
	NTV2WriteRegisterRemote (kRegSDIIn8VPIDB, 0x4180CA89);  // Reg 431  // Raw Value: 0x89CA8041, Version: 1, Standard: 1080 3G Level A, Video Format: 1080p59.94a, Progressive Transport: Yes, Progressive Picture: Yes, Picture Rate: 59.94, Aspect Ratio: 16x9, Sampling: YCbCr 4:2:2, Channel: 2, Bit Depth: 10, 3Ga: Yes, Two Sample Interleave: No, Xfer Characteristics: SDR, Colorimetry: Rec709, Luminance: YCbCr
	NTV2WriteRegisterRemote (kRegRP188InOut8Bits0_31, 0x38000400);  // Reg 434
	NTV2WriteRegisterRemote (kRegRP188InOut8DBB, 0x020D0200);  // Reg 436  // RP188: Unselected RP-188 received +LTC +VITC, Bypass: Disabled, Filter: 02, DBB: 02 00
	NTV2WriteRegisterRemote (kRegLTC8EmbeddedBits0_31, 0x63030608);  // Reg 437
	NTV2WriteRegisterRemote (kRegLTC8EmbeddedBits32_63, 0x00000004);  // Reg 438
	NTV2WriteRegisterRemote (kRegAud5Control, 0x80100300);  // Reg 440  // Audio Capture: Disabled, Audio Loopback: Disabled, Audio Input: Disabled, Audio Output: Disabled, Audio Embedder SDIOut5: Enabled, Audio Embedder SDIOut6: Enabled, A/V Sync Mode: Disabled, AES Rate Converter: Enabled, Audio Buffer Format: 16-Channel , 48kHz, 48kHz Support, No Embedded Support, 6-Channel Support, K-box, Monitor: Ch 1/2, K-Box Input: BNC, K-Box: Absent, Cable: BNC, Audio Buffer Size: 4 MB
	NTV2WriteRegisterRemote (kRegAud5SourceSelect, 0x00400001);  // Reg 441  // Audio Source: Embedded Groups 1 and 2, Embedded Source Select: Video Input 1, AES Sync Mode bit (fib): Disabled, PCM disabled: N, Erase head enable: N, Embedded Clock Select: Video Input, 3G audio source: Data stream 1
	NTV2WriteRegisterRemote (kRegAud6Control, 0x00000300);  // Reg 444  // Audio Capture: Disabled, Audio Loopback: Disabled, Audio Input: Disabled, Audio Output: Disabled, A/V Sync Mode: Disabled, AES Rate Converter: Enabled, Audio Buffer Format: 6-Channel , 48kHz, 48kHz Support, No Embedded Support, 6-Channel Support, K-box, Monitor: Ch 1/2, K-Box Input: BNC, K-Box: Absent, Cable: BNC, Audio Buffer Size: 1 MB
	NTV2WriteRegisterRemote (kRegAud7Control, 0x00000300);  // Reg 448  // Audio Capture: Disabled, Audio Loopback: Disabled, Audio Input: Disabled, Audio Output: Disabled, Audio Embedder SDIOut7: Enabled, Audio Embedder SDIOut8: Enabled, A/V Sync Mode: Disabled, AES Rate Converter: Enabled, Audio Buffer Format: 6-Channel , 48kHz, 48kHz Support, No Embedded Support, 6-Channel Support, K-box, Monitor: Ch 1/2, K-Box Input: BNC, K-Box: Absent, Cable: BNC, Audio Buffer Size: 1 MB
	NTV2WriteRegisterRemote (kRegAud8Control, 0x00000300);  // Reg 452  // Audio Capture: Disabled, Audio Loopback: Disabled, Audio Input: Disabled, Audio Output: Disabled, A/V Sync Mode: Disabled, AES Rate Converter: Enabled, Audio Buffer Format: 6-Channel , 48kHz, 48kHz Support, No Embedded Support, 6-Channel Support, K-box, Monitor: Ch 1/2, K-Box Input: BNC, K-Box: Absent, Cable: BNC, Audio Buffer Size: 1 MB
	NTV2WriteRegisterRemote (kRegAudioDetect5678, 0xFFFFFFFF);  // Reg 456
	NTV2WriteRegisterRemote (kRegSDI5678Input3GStatus, 0x31313131);  // Reg 457
	NTV2WriteRegisterRemote (kRegInput56Status, 0x0000C2C2);  // Reg 458  // Input 5 Scan Mode: Progressive, Input 5 Frame Rate: 59.94, Input 5 Geometry: 1125, Input 6 Scan Mode: Progressive, Input 6 Frame Rate: 59.94, Input 6 Geometry: 1125
	NTV2WriteRegisterRemote (kRegInput78Status, 0x0000C2C2);  // Reg 459  // Input 7 Scan Mode: Progressive, Input 7 Frame Rate: 59.94, Input 7 Geometry: 1125, Input 8 Scan Mode: Progressive, Input 8 Frame Rate: 59.94, Input 8 Geometry: 1125
	NTV2WriteRegisterRemote (kRegSDIOut6Control, 0x01040004);  // Reg 475  // Video Standard : 1080p, 2Kx1080 mode: 1920x1080, HBlank RGB Range: Black=0x04, 12G enable: N, 6G enalbe: N, 3G enable: Y, 3G mode: a, VPID insert enable: N, VPID overwrite enable: N, DS 1 audio source: Subsystem 5, DS 2 audio source: Subsystem 1
	NTV2WriteRegisterRemote (kRegSDIOut7Control, 0x01040004);  // Reg 476  // Video Standard : 1080p, 2Kx1080 mode: 1920x1080, HBlank RGB Range: Black=0x04, 12G enable: N, 6G enalbe: N, 3G enable: Y, 3G mode: a, VPID insert enable: N, VPID overwrite enable: N, DS 1 audio source: Subsystem 5, DS 2 audio source: Subsystem 1
	NTV2WriteRegisterRemote (kRegSDIOut8Control, 0x01040004);  // Reg 477  // Video Standard : 1080p, 2Kx1080 mode: 1920x1080, HBlank RGB Range: Black=0x04, 12G enable: N, 6G enalbe: N, 3G enable: Y, 3G mode: a, VPID insert enable: N, VPID overwrite enable: N, DS 1 audio source: Subsystem 5, DS 2 audio source: Subsystem 1
	NTV2WriteRegisterRemote (kRegOutputTimingControlch2, 0x08001000);  // Reg 478
	NTV2WriteRegisterRemote (kRegOutputTimingControlch3, 0x08001000);  // Reg 479
	NTV2WriteRegisterRemote (kRegOutputTimingControlch4, 0x08001000);  // Reg 480
	NTV2WriteRegisterRemote (kRegOutputTimingControlch5, 0x08001000);  // Reg 481
	NTV2WriteRegisterRemote (kRegOutputTimingControlch6, 0x08001000);  // Reg 482
	NTV2WriteRegisterRemote (kRegOutputTimingControlch7, 0x08001000);  // Reg 483
	NTV2WriteRegisterRemote (kRegOutputTimingControlch8, 0x08001000);  // Reg 484
	NTV2WriteRegisterRemote (kRegVidProc3Control, 0x00200000);  // Reg 485  // Mode: Full Raster, FG Control: Unshaped, BG Control: Full Raster, VANC Pass-Thru: Foreground, FG Matte: Disabled, BG Matte: Disabled, Input Sync: in sync, Limiting: Legal SDI, Split Video Std: 1080i
	NTV2WriteRegisterRemote (kRegFlatMatte3Value, 0x18080240);  // Reg 487  // Flat Matte Cb: 240, Flat Matte Y: 1C0, Flat Matte Cr: 180
	NTV2WriteRegisterRemote (kRegFlatMatte4Value, 0x18080240);  // Reg 490  // Flat Matte Cb: 240, Flat Matte Y: 1C0, Flat Matte Cr: 180
	NTV2WriteRegisterRemote (kRegTRSErrorStatus, 0x00F00000);  // Reg 491
	NTV2WriteRegisterRemote (kRegRXSDI1FrameCountLow, 0x00D56B2B);  // Reg 2050
	NTV2WriteRegisterRemote (kRegRXSDI1FrameRefCountLow, 0x9C3966CF);  // Reg 2052
	NTV2WriteRegisterRemote (kRegRXSDI1FrameRefCountHigh, 0x00000003);  // Reg 2053
	NTV2WriteRegisterRemote (kRegRXSDI2FrameCountLow, 0x00A2E900);  // Reg 2058
	NTV2WriteRegisterRemote (kRegRXSDI2FrameRefCountLow, 0x1AB48C3B);  // Reg 2060
	NTV2WriteRegisterRemote (kRegRXSDI3FrameCountLow, 0x00F6602B);  // Reg 2066
	NTV2WriteRegisterRemote (kRegRXSDI3FrameRefCountLow, 0x1AF13D4F);  // Reg 2068
	NTV2WriteRegisterRemote (kRegRXSDI4FrameCountLow, 0x000480B4);  // Reg 2074
	NTV2WriteRegisterRemote (kRegRXSDI4FrameRefCountLow, 0x1AAD9B03);  // Reg 2076
	NTV2WriteRegisterRemote (kRegRXSDI5Status, 0x00310010);  // Reg 2080
	NTV2WriteRegisterRemote (kRegRXSDI5CRCErrorCount, 0x00000015);  // Reg 2081
	NTV2WriteRegisterRemote (kRegRXSDI5FrameCountLow, 0x00516EF4);  // Reg 2082
	NTV2WriteRegisterRemote (kRegRXSDI5FrameRefCountLow, 0xDABA55D6);  // Reg 2084
	NTV2WriteRegisterRemote (kRegRXSDI5FrameRefCountHigh, 0x000024C8);  // Reg 2085
	NTV2WriteRegisterRemote (kRegRXSDI6Status, 0x0031000C);  // Reg 2088
	NTV2WriteRegisterRemote (kRegRXSDI6CRCErrorCount, 0x0000CC70);  // Reg 2089
	NTV2WriteRegisterRemote (kRegRXSDI6FrameCountLow, 0x020AD1CE);  // Reg 2090
	NTV2WriteRegisterRemote (kRegRXSDI6FrameRefCountLow, 0xDABA55D7);  // Reg 2092
	NTV2WriteRegisterRemote (kRegRXSDI6FrameRefCountHigh, 0x000024C8);  // Reg 2093
	NTV2WriteRegisterRemote (kRegRXSDI7Status, 0x00310007);  // Reg 2096
	NTV2WriteRegisterRemote (kRegRXSDI7CRCErrorCount, 0x00003CB2);  // Reg 2097
	NTV2WriteRegisterRemote (kRegRXSDI7FrameCountLow, 0x00C00088);  // Reg 2098
	NTV2WriteRegisterRemote (kRegRXSDI7FrameRefCountLow, 0xDABA55D7);  // Reg 2100
	NTV2WriteRegisterRemote (kRegRXSDI7FrameRefCountHigh, 0x000024C8);  // Reg 2101
	NTV2WriteRegisterRemote (kRegRXSDI8Status, 0x00310007);  // Reg 2104
	NTV2WriteRegisterRemote (kRegRXSDI8CRCErrorCount, 0x000164D5);  // Reg 2105
	NTV2WriteRegisterRemote (kRegRXSDI8FrameCountLow, 0x00642D78);  // Reg 2106
	NTV2WriteRegisterRemote (kRegRXSDI8FrameRefCountLow, 0xDABA55D8);  // Reg 2108
	NTV2WriteRegisterRemote (kRegRXSDI8FrameRefCountHigh, 0x000024C8);  // Reg 2109
	NTV2WriteRegisterRemote (kRegRXSDIFreeRunningClockLow, 0x18188497);  // Reg 2112
	NTV2WriteRegisterRemote (kRegRXSDIFreeRunningClockHigh, 0x00002F12);  // Reg 2113
	NTV2WriteRegisterRemote (4096, 0x10011111);  // Extract 1 Control  // HANC Y enable: Y, VANC Y enable: Y, HANC C enable: Y, VANC C enable: Y, Progressive video: Y, Synchronize: field, Memory writes: Disabled, SD Y+C Demux: Disabled, Metadata from: MSBs
	NTV2WriteRegisterRemote (4097, 0x30000000);  // Extract 1 F1 Start Address
	NTV2WriteRegisterRemote (4098, 0x301FFFFF);  // Extract 1 F1 End Address
	NTV2WriteRegisterRemote (4099, 0x30200000);  // Extract 1 F2 Start Address
	NTV2WriteRegisterRemote (4100, 0x303FFFFF);  // Extract 1 F2 End Address
	NTV2WriteRegisterRemote (4101, 0x00640064);  // Extract 1 Field Cutoff Lines  // F1 cutoff line: 100, F2 cutoff line: 100
	NTV2WriteRegisterRemote (4105, 0x00000462);  // Extract 1 V Blank Lines  // F1 VBL start line: 1122, F2 VBL start line: 0
	NTV2WriteRegisterRemote (4106, 0x00000465);  // Extract 1 Lines Per Frame
	NTV2WriteRegisterRemote (4108, 0xE4E5E6E7);  // Extract 1 Ignore DID 1-4  // Ignoring DIDs E7, E6, E5, E4
	NTV2WriteRegisterRemote (4109, 0xE0E1E2E3);  // Extract 1 Ignore DID 5-8  // Ignoring DIDs E3, E2, E1, E0
	NTV2WriteRegisterRemote (4110, 0xA4A5A6A7);  // Extract 1 Ignore DID 9-12  // Ignoring DIDs A7, A6, A5, A4
	NTV2WriteRegisterRemote (4111, 0xA0A1A2A3);  // Extract 1 Ignore DID 13-16  // Ignoring DIDs A3, A2, A1, A0
	NTV2WriteRegisterRemote (4112, 0xE7E7E7E7);  // Extract 1 Ignore DID 17-20  // Ignoring DIDs E7, E7, E7, E7
	NTV2WriteRegisterRemote (4113, 0x010A0004);  // Extract 1 Analog Start Line  // F1 analog start line: 4, F2 analog start line: 266
	NTV2WriteRegisterRemote (4123, 0x00000780);  // Reg 0x101B
	NTV2WriteRegisterRemote (4160, 0x10011111);  // Extract 2 Control  // HANC Y enable: Y, VANC Y enable: Y, HANC C enable: Y, VANC C enable: Y, Progressive video: Y, Synchronize: field, Memory writes: Disabled, SD Y+C Demux: Disabled, Metadata from: MSBs
	NTV2WriteRegisterRemote (4161, 0x30800000);  // Extract 2 F1 Start Address
	NTV2WriteRegisterRemote (4162, 0x309FFFFF);  // Extract 2 F1 End Address
	NTV2WriteRegisterRemote (4163, 0x30A00000);  // Extract 2 F2 Start Address
	NTV2WriteRegisterRemote (4164, 0x30BFFFFF);  // Extract 2 F2 End Address
	NTV2WriteRegisterRemote (4165, 0x00640064);  // Extract 2 Field Cutoff Lines  // F1 cutoff line: 100, F2 cutoff line: 100
	NTV2WriteRegisterRemote (4169, 0x00000462);  // Extract 2 V Blank Lines  // F1 VBL start line: 1122, F2 VBL start line: 0
	NTV2WriteRegisterRemote (4170, 0x00000465);  // Extract 2 Lines Per Frame
	NTV2WriteRegisterRemote (4172, 0xE4E5E6E7);  // Extract 2 Ignore DID 1-4  // Ignoring DIDs E7, E6, E5, E4
	NTV2WriteRegisterRemote (4173, 0xE0E1E2E3);  // Extract 2 Ignore DID 5-8  // Ignoring DIDs E3, E2, E1, E0
	NTV2WriteRegisterRemote (4174, 0xA4A5A6A7);  // Extract 2 Ignore DID 9-12  // Ignoring DIDs A7, A6, A5, A4
	NTV2WriteRegisterRemote (4175, 0xA0A1A2A3);  // Extract 2 Ignore DID 13-16  // Ignoring DIDs A3, A2, A1, A0
	NTV2WriteRegisterRemote (4176, 0xE7E7E7E7);  // Extract 2 Ignore DID 17-20  // Ignoring DIDs E7, E7, E7, E7
	NTV2WriteRegisterRemote (4177, 0x010A0004);  // Extract 2 Analog Start Line  // F1 analog start line: 4, F2 analog start line: 266
	NTV2WriteRegisterRemote (4187, 0x00000780);  // Reg 0x105B
	NTV2WriteRegisterRemote (4224, 0x10011111);  // Extract 3 Control  // HANC Y enable: Y, VANC Y enable: Y, HANC C enable: Y, VANC C enable: Y, Progressive video: Y, Synchronize: field, Memory writes: Disabled, SD Y+C Demux: Disabled, Metadata from: MSBs
	NTV2WriteRegisterRemote (4225, 0x31000000);  // Extract 3 F1 Start Address
	NTV2WriteRegisterRemote (4226, 0x311FFFFF);  // Extract 3 F1 End Address
	NTV2WriteRegisterRemote (4227, 0x31200000);  // Extract 3 F2 Start Address
	NTV2WriteRegisterRemote (4228, 0x313FFFFF);  // Extract 3 F2 End Address
	NTV2WriteRegisterRemote (4229, 0x00640064);  // Extract 3 Field Cutoff Lines  // F1 cutoff line: 100, F2 cutoff line: 100
	NTV2WriteRegisterRemote (4233, 0x00000462);  // Extract 3 V Blank Lines  // F1 VBL start line: 1122, F2 VBL start line: 0
	NTV2WriteRegisterRemote (4234, 0x00000465);  // Extract 3 Lines Per Frame
	NTV2WriteRegisterRemote (4236, 0xE4E5E6E7);  // Extract 3 Ignore DID 1-4  // Ignoring DIDs E7, E6, E5, E4
	NTV2WriteRegisterRemote (4237, 0xE0E1E2E3);  // Extract 3 Ignore DID 5-8  // Ignoring DIDs E3, E2, E1, E0
	NTV2WriteRegisterRemote (4238, 0xA4A5A6A7);  // Extract 3 Ignore DID 9-12  // Ignoring DIDs A7, A6, A5, A4
	NTV2WriteRegisterRemote (4239, 0xA0A1A2A3);  // Extract 3 Ignore DID 13-16  // Ignoring DIDs A3, A2, A1, A0
	NTV2WriteRegisterRemote (4240, 0xE7E7E7E7);  // Extract 3 Ignore DID 17-20  // Ignoring DIDs E7, E7, E7, E7
	NTV2WriteRegisterRemote (4241, 0x010A0004);  // Extract 3 Analog Start Line  // F1 analog start line: 4, F2 analog start line: 266
	NTV2WriteRegisterRemote (4251, 0x00000780);  // Reg 0x109B
	NTV2WriteRegisterRemote (4288, 0x10011111);  // Extract 4 Control  // HANC Y enable: Y, VANC Y enable: Y, HANC C enable: Y, VANC C enable: Y, Progressive video: Y, Synchronize: field, Memory writes: Disabled, SD Y+C Demux: Disabled, Metadata from: MSBs
	NTV2WriteRegisterRemote (4289, 0x31800000);  // Extract 4 F1 Start Address
	NTV2WriteRegisterRemote (4290, 0x319FFFFF);  // Extract 4 F1 End Address
	NTV2WriteRegisterRemote (4291, 0x31A00000);  // Extract 4 F2 Start Address
	NTV2WriteRegisterRemote (4292, 0x31BFFFFF);  // Extract 4 F2 End Address
	NTV2WriteRegisterRemote (4293, 0x00640064);  // Extract 4 Field Cutoff Lines  // F1 cutoff line: 100, F2 cutoff line: 100
	NTV2WriteRegisterRemote (4297, 0x00000462);  // Extract 4 V Blank Lines  // F1 VBL start line: 1122, F2 VBL start line: 0
	NTV2WriteRegisterRemote (4298, 0x00000465);  // Extract 4 Lines Per Frame
	NTV2WriteRegisterRemote (4300, 0xE4E5E6E7);  // Extract 4 Ignore DID 1-4  // Ignoring DIDs E7, E6, E5, E4
	NTV2WriteRegisterRemote (4301, 0xE0E1E2E3);  // Extract 4 Ignore DID 5-8  // Ignoring DIDs E3, E2, E1, E0
	NTV2WriteRegisterRemote (4302, 0xA4A5A6A7);  // Extract 4 Ignore DID 9-12  // Ignoring DIDs A7, A6, A5, A4
	NTV2WriteRegisterRemote (4303, 0xA0A1A2A3);  // Extract 4 Ignore DID 13-16  // Ignoring DIDs A3, A2, A1, A0
	NTV2WriteRegisterRemote (4304, 0xE7E7E7E7);  // Extract 4 Ignore DID 17-20  // Ignoring DIDs E7, E7, E7, E7
	NTV2WriteRegisterRemote (4305, 0x010A0004);  // Extract 4 Analog Start Line  // F1 analog start line: 4, F2 analog start line: 266
	NTV2WriteRegisterRemote (4315, 0x00000780);  // Reg 0x10DB
	NTV2WriteRegisterRemote (4352, 0x11010000);  // Extract 5 Control  // HANC Y enable: N, VANC Y enable: N, HANC C enable: N, VANC C enable: N, Progressive video: Y, Synchronize: frame, Memory writes: Disabled, SD Y+C Demux: Disabled, Metadata from: MSBs
	NTV2WriteRegisterRemote (4353, 0x0DFFC000);  // Extract 5 F1 Start Address
	NTV2WriteRegisterRemote (4354, 0x0DFFDFFF);  // Extract 5 F1 End Address
	NTV2WriteRegisterRemote (4357, 0x00000465);  // Extract 5 Field Cutoff Lines  // F1 cutoff line: 1125, F2 cutoff line: 0
	NTV2WriteRegisterRemote (4360, 0x00000017);  // Extract 5 F2 Memory Usage  // Total F2 bytes: 23, Overrun: N
	NTV2WriteRegisterRemote (4361, 0x00000462);  // Extract 5 V Blank Lines  // F1 VBL start line: 1122, F2 VBL start line: 0
	NTV2WriteRegisterRemote (4362, 0x00000465);  // Extract 5 Lines Per Frame
	NTV2WriteRegisterRemote (4364, 0xE4E5E6E7);  // Extract 5 Ignore DID 1-4  // Ignoring DIDs E7, E6, E5, E4
	NTV2WriteRegisterRemote (4365, 0xE0E1E2E3);  // Extract 5 Ignore DID 5-8  // Ignoring DIDs E3, E2, E1, E0
	NTV2WriteRegisterRemote (4366, 0xA4A5A6A7);  // Extract 5 Ignore DID 9-12  // Ignoring DIDs A7, A6, A5, A4
	NTV2WriteRegisterRemote (4367, 0xA0A1A2A3);  // Extract 5 Ignore DID 13-16  // Ignoring DIDs A3, A2, A1, A0
	NTV2WriteRegisterRemote (4379, 0x00000780);  // Reg 0x111B
	NTV2WriteRegisterRemote (4416, 0x10011111);  // Extract 6 Control  // HANC Y enable: Y, VANC Y enable: Y, HANC C enable: Y, VANC C enable: Y, Progressive video: Y, Synchronize: field, Memory writes: Disabled, SD Y+C Demux: Disabled, Metadata from: MSBs
	NTV2WriteRegisterRemote (4417, 0x3A800000);  // Extract 6 F1 Start Address
	NTV2WriteRegisterRemote (4418, 0x3A9FFFFF);  // Extract 6 F1 End Address
	NTV2WriteRegisterRemote (4419, 0x3AA00000);  // Extract 6 F2 Start Address
	NTV2WriteRegisterRemote (4420, 0x3ABFFFFF);  // Extract 6 F2 End Address
	NTV2WriteRegisterRemote (4421, 0x00640064);  // Extract 6 Field Cutoff Lines  // F1 cutoff line: 100, F2 cutoff line: 100
	NTV2WriteRegisterRemote (4425, 0x00000462);  // Extract 6 V Blank Lines  // F1 VBL start line: 1122, F2 VBL start line: 0
	NTV2WriteRegisterRemote (4426, 0x00000465);  // Extract 6 Lines Per Frame
	NTV2WriteRegisterRemote (4428, 0xE4E5E6E7);  // Extract 6 Ignore DID 1-4  // Ignoring DIDs E7, E6, E5, E4
	NTV2WriteRegisterRemote (4429, 0xE0E1E2E3);  // Extract 6 Ignore DID 5-8  // Ignoring DIDs E3, E2, E1, E0
	NTV2WriteRegisterRemote (4430, 0xA4A5A6A7);  // Extract 6 Ignore DID 9-12  // Ignoring DIDs A7, A6, A5, A4
	NTV2WriteRegisterRemote (4431, 0xA0A1A2A3);  // Extract 6 Ignore DID 13-16  // Ignoring DIDs A3, A2, A1, A0
	NTV2WriteRegisterRemote (4432, 0xE7E7E7E7);  // Extract 6 Ignore DID 17-20  // Ignoring DIDs E7, E7, E7, E7
	NTV2WriteRegisterRemote (4433, 0x010A0004);  // Extract 6 Analog Start Line  // F1 analog start line: 4, F2 analog start line: 266
	NTV2WriteRegisterRemote (4443, 0x00000780);  // Reg 0x115B
	NTV2WriteRegisterRemote (4480, 0x10011111);  // Extract 7 Control  // HANC Y enable: Y, VANC Y enable: Y, HANC C enable: Y, VANC C enable: Y, Progressive video: Y, Synchronize: field, Memory writes: Disabled, SD Y+C Demux: Disabled, Metadata from: MSBs
	NTV2WriteRegisterRemote (4481, 0x3B000000);  // Extract 7 F1 Start Address
	NTV2WriteRegisterRemote (4482, 0x3B1FFFFF);  // Extract 7 F1 End Address
	NTV2WriteRegisterRemote (4483, 0x3B200000);  // Extract 7 F2 Start Address
	NTV2WriteRegisterRemote (4484, 0x3B3FFFFF);  // Extract 7 F2 End Address
	NTV2WriteRegisterRemote (4485, 0x00640064);  // Extract 7 Field Cutoff Lines  // F1 cutoff line: 100, F2 cutoff line: 100
	NTV2WriteRegisterRemote (4489, 0x00000462);  // Extract 7 V Blank Lines  // F1 VBL start line: 1122, F2 VBL start line: 0
	NTV2WriteRegisterRemote (4490, 0x00000465);  // Extract 7 Lines Per Frame
	NTV2WriteRegisterRemote (4492, 0xE4E5E6E7);  // Extract 7 Ignore DID 1-4  // Ignoring DIDs E7, E6, E5, E4
	NTV2WriteRegisterRemote (4493, 0xE0E1E2E3);  // Extract 7 Ignore DID 5-8  // Ignoring DIDs E3, E2, E1, E0
	NTV2WriteRegisterRemote (4494, 0xA4A5A6A7);  // Extract 7 Ignore DID 9-12  // Ignoring DIDs A7, A6, A5, A4
	NTV2WriteRegisterRemote (4495, 0xA0A1A2A3);  // Extract 7 Ignore DID 13-16  // Ignoring DIDs A3, A2, A1, A0
	NTV2WriteRegisterRemote (4496, 0xE7E7E7E7);  // Extract 7 Ignore DID 17-20  // Ignoring DIDs E7, E7, E7, E7
	NTV2WriteRegisterRemote (4497, 0x010A0004);  // Extract 7 Analog Start Line  // F1 analog start line: 4, F2 analog start line: 266
	NTV2WriteRegisterRemote (4507, 0x00000780);  // Reg 0x119B
	NTV2WriteRegisterRemote (4544, 0x10011111);  // Extract 8 Control  // HANC Y enable: Y, VANC Y enable: Y, HANC C enable: Y, VANC C enable: Y, Progressive video: Y, Synchronize: field, Memory writes: Disabled, SD Y+C Demux: Disabled, Metadata from: MSBs
	NTV2WriteRegisterRemote (4545, 0x3B800000);  // Extract 8 F1 Start Address
	NTV2WriteRegisterRemote (4546, 0x3B9FFFFF);  // Extract 8 F1 End Address
	NTV2WriteRegisterRemote (4547, 0x3BA00000);  // Extract 8 F2 Start Address
	NTV2WriteRegisterRemote (4548, 0x3BBFFFFF);  // Extract 8 F2 End Address
	NTV2WriteRegisterRemote (4549, 0x00640064);  // Extract 8 Field Cutoff Lines  // F1 cutoff line: 100, F2 cutoff line: 100
	NTV2WriteRegisterRemote (4553, 0x00000462);  // Extract 8 V Blank Lines  // F1 VBL start line: 1122, F2 VBL start line: 0
	NTV2WriteRegisterRemote (4554, 0x00000465);  // Extract 8 Lines Per Frame
	NTV2WriteRegisterRemote (4556, 0xE4E5E6E7);  // Extract 8 Ignore DID 1-4  // Ignoring DIDs E7, E6, E5, E4
	NTV2WriteRegisterRemote (4557, 0xE0E1E2E3);  // Extract 8 Ignore DID 5-8  // Ignoring DIDs E3, E2, E1, E0
	NTV2WriteRegisterRemote (4558, 0xA4A5A6A7);  // Extract 8 Ignore DID 9-12  // Ignoring DIDs A7, A6, A5, A4
	NTV2WriteRegisterRemote (4559, 0xA0A1A2A3);  // Extract 8 Ignore DID 13-16  // Ignoring DIDs A3, A2, A1, A0
	NTV2WriteRegisterRemote (4560, 0xE7E7E7E7);  // Extract 8 Ignore DID 17-20  // Ignoring DIDs E7, E7, E7, E7
	NTV2WriteRegisterRemote (4561, 0x010A0004);  // Extract 8 Analog Start Line  // F1 analog start line: 4, F2 analog start line: 266
	NTV2WriteRegisterRemote (4571, 0x00000780);  // Reg 0x11DB
	NTV2WriteRegisterRemote (4609, 0x11000000);  // Insert 1 Control  // HANC Y enable: N, VANC Y enable: N, HANC C enable: N, VANC C enable: N, Payload Y insert: N, Payload C insert: N, Payload F1 insert: N, Payload F2 insert: N, Progressive video: Y, Memory reads: Disabled, SD Packet Split: Disabled
	NTV2WriteRegisterRemote (4610, 0x30000000);  // Insert 1 F1 Start Address
	NTV2WriteRegisterRemote (4611, 0x30400000);  // Insert 1 F2 Start Address
	NTV2WriteRegisterRemote (4612, 0x00000008);  // Insert 1 Pixel Delay  // HANC pixel delay: 8, VANC pixel delay: 0
	NTV2WriteRegisterRemote (4613, 0x0000002A);  // Insert 1 Active Start  // F1 first active line: 42, F2 first active line: 0
	NTV2WriteRegisterRemote (4614, 0x08980780);  // Insert 1 Pixels Per Line  // Active line length: 1920, Total line length: 2200
	NTV2WriteRegisterRemote (4615, 0x00000465);  // Insert 1 Lines Per Frame
	NTV2WriteRegisterRemote (4617, 0x0000000A);  // Insert 1 Payload ID Control
	NTV2WriteRegisterRemote (4618, 0x0100CA59);  // Insert 1 Payload ID
	NTV2WriteRegisterRemote (4673, 0x11000000);  // Insert 2 Control  // HANC Y enable: N, VANC Y enable: N, HANC C enable: N, VANC C enable: N, Payload Y insert: N, Payload C insert: N, Payload F1 insert: N, Payload F2 insert: N, Progressive video: Y, Memory reads: Disabled, SD Packet Split: Disabled
	NTV2WriteRegisterRemote (4674, 0x30800000);  // Insert 2 F1 Start Address
	NTV2WriteRegisterRemote (4675, 0x30C00000);  // Insert 2 F2 Start Address
	NTV2WriteRegisterRemote (4676, 0x00000008);  // Insert 2 Pixel Delay  // HANC pixel delay: 8, VANC pixel delay: 0
	NTV2WriteRegisterRemote (4677, 0x0000002A);  // Insert 2 Active Start  // F1 first active line: 42, F2 first active line: 0
	NTV2WriteRegisterRemote (4678, 0x08980780);  // Insert 2 Pixels Per Line  // Active line length: 1920, Total line length: 2200
	NTV2WriteRegisterRemote (4679, 0x00000465);  // Insert 2 Lines Per Frame
	NTV2WriteRegisterRemote (4681, 0x0000000A);  // Insert 2 Payload ID Control
	NTV2WriteRegisterRemote (4682, 0x0100CA59);  // Insert 2 Payload ID
	NTV2WriteRegisterRemote (4737, 0x11000000);  // Insert 3 Control  // HANC Y enable: N, VANC Y enable: N, HANC C enable: N, VANC C enable: N, Payload Y insert: N, Payload C insert: N, Payload F1 insert: N, Payload F2 insert: N, Progressive video: Y, Memory reads: Disabled, SD Packet Split: Disabled
	NTV2WriteRegisterRemote (4738, 0x31000000);  // Insert 3 F1 Start Address
	NTV2WriteRegisterRemote (4739, 0x31400000);  // Insert 3 F2 Start Address
	NTV2WriteRegisterRemote (4740, 0x00000008);  // Insert 3 Pixel Delay  // HANC pixel delay: 8, VANC pixel delay: 0
	NTV2WriteRegisterRemote (4741, 0x0000002A);  // Insert 3 Active Start  // F1 first active line: 42, F2 first active line: 0
	NTV2WriteRegisterRemote (4742, 0x08980780);  // Insert 3 Pixels Per Line  // Active line length: 1920, Total line length: 2200
	NTV2WriteRegisterRemote (4743, 0x00000465);  // Insert 3 Lines Per Frame
	NTV2WriteRegisterRemote (4745, 0x0000000A);  // Insert 3 Payload ID Control
	NTV2WriteRegisterRemote (4746, 0x0100CA59);  // Insert 3 Payload ID
	NTV2WriteRegisterRemote (4801, 0x11000000);  // Insert 4 Control  // HANC Y enable: N, VANC Y enable: N, HANC C enable: N, VANC C enable: N, Payload Y insert: N, Payload C insert: N, Payload F1 insert: N, Payload F2 insert: N, Progressive video: Y, Memory reads: Disabled, SD Packet Split: Disabled
	NTV2WriteRegisterRemote (4802, 0x31800000);  // Insert 4 F1 Start Address
	NTV2WriteRegisterRemote (4803, 0x31C00000);  // Insert 4 F2 Start Address
	NTV2WriteRegisterRemote (4804, 0x00000008);  // Insert 4 Pixel Delay  // HANC pixel delay: 8, VANC pixel delay: 0
	NTV2WriteRegisterRemote (4805, 0x0000002A);  // Insert 4 Active Start  // F1 first active line: 42, F2 first active line: 0
	NTV2WriteRegisterRemote (4806, 0x08980780);  // Insert 4 Pixels Per Line  // Active line length: 1920, Total line length: 2200
	NTV2WriteRegisterRemote (4807, 0x00000465);  // Insert 4 Lines Per Frame
	NTV2WriteRegisterRemote (4809, 0x0000000A);  // Insert 4 Payload ID Control
	NTV2WriteRegisterRemote (4810, 0x0100CA59);  // Insert 4 Payload ID
	NTV2WriteRegisterRemote (4865, 0x11000000);  // Insert 5 Control  // HANC Y enable: N, VANC Y enable: N, HANC C enable: N, VANC C enable: N, Payload Y insert: N, Payload C insert: N, Payload F1 insert: N, Payload F2 insert: N, Progressive video: Y, Memory reads: Disabled, SD Packet Split: Disabled
	NTV2WriteRegisterRemote (4866, 0x30A00000);  // Insert 5 F1 Start Address
	NTV2WriteRegisterRemote (4867, 0x30E00000);  // Insert 5 F2 Start Address
	NTV2WriteRegisterRemote (4868, 0x00000008);  // Insert 5 Pixel Delay  // HANC pixel delay: 8, VANC pixel delay: 0
	NTV2WriteRegisterRemote (4869, 0x0000002A);  // Insert 5 Active Start  // F1 first active line: 42, F2 first active line: 0
	NTV2WriteRegisterRemote (4870, 0x08980780);  // Insert 5 Pixels Per Line  // Active line length: 1920, Total line length: 2200
	NTV2WriteRegisterRemote (4871, 0x00000465);  // Insert 5 Lines Per Frame
	NTV2WriteRegisterRemote (4873, 0x0000000A);  // Insert 5 Payload ID Control
	NTV2WriteRegisterRemote (4874, 0x0100CA59);  // Insert 5 Payload ID
	NTV2WriteRegisterRemote (4929, 0x11000000);  // Insert 6 Control  // HANC Y enable: N, VANC Y enable: N, HANC C enable: N, VANC C enable: N, Payload Y insert: N, Payload C insert: N, Payload F1 insert: N, Payload F2 insert: N, Progressive video: Y, Memory reads: Disabled, SD Packet Split: Disabled
	NTV2WriteRegisterRemote (4930, 0x31200000);  // Insert 6 F1 Start Address
	NTV2WriteRegisterRemote (4931, 0x31600000);  // Insert 6 F2 Start Address
	NTV2WriteRegisterRemote (4932, 0x00000008);  // Insert 6 Pixel Delay  // HANC pixel delay: 8, VANC pixel delay: 0
	NTV2WriteRegisterRemote (4933, 0x0000002A);  // Insert 6 Active Start  // F1 first active line: 42, F2 first active line: 0
	NTV2WriteRegisterRemote (4934, 0x08980780);  // Insert 6 Pixels Per Line  // Active line length: 1920, Total line length: 2200
	NTV2WriteRegisterRemote (4935, 0x00000465);  // Insert 6 Lines Per Frame
	NTV2WriteRegisterRemote (4937, 0x0000000A);  // Insert 6 Payload ID Control
	NTV2WriteRegisterRemote (4938, 0x0100CA59);  // Insert 6 Payload ID
	NTV2WriteRegisterRemote (4993, 0x11000000);  // Insert 7 Control  // HANC Y enable: N, VANC Y enable: N, HANC C enable: N, VANC C enable: N, Payload Y insert: N, Payload C insert: N, Payload F1 insert: N, Payload F2 insert: N, Progressive video: Y, Memory reads: Disabled, SD Packet Split: Disabled
	NTV2WriteRegisterRemote (4994, 0x31A00000);  // Insert 7 F1 Start Address
	NTV2WriteRegisterRemote (4995, 0x31E00000);  // Insert 7 F2 Start Address
	NTV2WriteRegisterRemote (4996, 0x00000008);  // Insert 7 Pixel Delay  // HANC pixel delay: 8, VANC pixel delay: 0
	NTV2WriteRegisterRemote (4997, 0x0000002A);  // Insert 7 Active Start  // F1 first active line: 42, F2 first active line: 0
	NTV2WriteRegisterRemote (4998, 0x08980780);  // Insert 7 Pixels Per Line  // Active line length: 1920, Total line length: 2200
	NTV2WriteRegisterRemote (4999, 0x00000465);  // Insert 7 Lines Per Frame
	NTV2WriteRegisterRemote (5001, 0x0000000A);  // Insert 7 Payload ID Control
	NTV2WriteRegisterRemote (5002, 0x0100CA59);  // Insert 7 Payload ID
	NTV2WriteRegisterRemote (5057, 0x11000000);  // Insert 8 Control  // HANC Y enable: N, VANC Y enable: N, HANC C enable: N, VANC C enable: N, Payload Y insert: N, Payload C insert: N, Payload F1 insert: N, Payload F2 insert: N, Progressive video: Y, Memory reads: Disabled, SD Packet Split: Disabled
	NTV2WriteRegisterRemote (5058, 0x32200000);  // Insert 8 F1 Start Address
	NTV2WriteRegisterRemote (5059, 0x32600000);  // Insert 8 F2 Start Address
	NTV2WriteRegisterRemote (5060, 0x00000008);  // Insert 8 Pixel Delay  // HANC pixel delay: 8, VANC pixel delay: 0
	NTV2WriteRegisterRemote (5061, 0x0000002A);  // Insert 8 Active Start  // F1 first active line: 42, F2 first active line: 0
	NTV2WriteRegisterRemote (5062, 0x08980780);  // Insert 8 Pixels Per Line  // Active line length: 1920, Total line length: 2200
	NTV2WriteRegisterRemote (5063, 0x00000465);  // Insert 8 Lines Per Frame
	NTV2WriteRegisterRemote (5065, 0x0000000A);  // Insert 8 Payload ID Control
	NTV2WriteRegisterRemote (5066, 0x0100CA59);  // Insert 8 Payload ID
	NTV2WriteRegisterRemote (kVRegDriverVersion, 0x03C50C08);  // Reg 10000
	NTV2WriteRegisterRemote (kVRegSecondaryFormatSelect, 0x00000002);  // Reg 10021
	NTV2WriteRegisterRemote (kVRegAnalogOutputType, 0x00000001);  // Reg 10025
	NTV2WriteRegisterRemote (kVRegGammaMode, 0x00000001);  // Reg 10043
	NTV2WriteRegisterRemote (kVRegLUTType, 0x00000002);  // Reg 10044
	NTV2WriteRegisterRemote (kVRegRGB10Range, 0x00000001);  // Reg 10045
	NTV2WriteRegisterRemote (kVRegRGB10Endian, 0x00000001);  // Reg 10046
	NTV2WriteRegisterRemote (kVRegAudioSyncTolerance, 0x00002710);  // Reg 10079
	NTV2WriteRegisterRemote (kVRegDSKAudioMode, 0x00000001);  // Reg 10125
	NTV2WriteRegisterRemote (kVRegCaptureReferenceSelect, 0x00000002);  // Reg 10128
	NTV2WriteRegisterRemote (kVRegSDIInput2RGBRange, 0x00000002);  // Reg 10135
	NTV2WriteRegisterRemote (kVRegSDIInput1Stereo3DMode, 0x00000001);  // Reg 10136
	NTV2WriteRegisterRemote (kVRegSDIInput2Stereo3DMode, 0x00000001);  // Reg 10137
	NTV2WriteRegisterRemote (kVRegFrameBuffer1RGBRange, 0x00000001);  // Reg 10138
	NTV2WriteRegisterRemote (kVRegFrameBuffer1Stereo3DMode, 0x00000001);  // Reg 10139
	NTV2WriteRegisterRemote (kVRegAnalogInputType, 0x00000001);  // Reg 10143
	NTV2WriteRegisterRemote (kVRegSDIInput2ColorSpaceMode, 0x00000001);  // Reg 10150
	NTV2WriteRegisterRemote (kVRegSDIOutput1Stereo3DMode, 0x00000001);  // Reg 10152
	NTV2WriteRegisterRemote (kVRegSDIOutput2Stereo3DMode, 0x00000001);  // Reg 10153
	NTV2WriteRegisterRemote (kVRegFrameBuffer2RGBRange, 0x00000001);  // Reg 10154
	NTV2WriteRegisterRemote (kVRegFrameBuffer2Stereo3DMode, 0x00000001);  // Reg 10155
	NTV2WriteRegisterRemote (kVRegAudioGainDisable, 0x00000001);  // Reg 10156
	NTV2WriteRegisterRemote (kVRegActiveVideoOutFilter, 0x0000007F);  // Reg 10158
	NTV2WriteRegisterRemote (kVRegDeviceOnline, 0x00000001);  // Reg 10168
	NTV2WriteRegisterRemote (kVRegTimelapseCaptureValue, 0x00000001);  // Reg 10175
	NTV2WriteRegisterRemote (kVRegTimelapseIntervalValue, 0x00000001);  // Reg 10177
	NTV2WriteRegisterRemote (kVRegTimelapseIntervalUnits, 0x00000001);  // Reg 10178
	NTV2WriteRegisterRemote (kVRegAnalogInStandard, 0x00000002);  // Reg 10180
	NTV2WriteRegisterRemote (kVRegAnalogIoSelect, 0x00000002);  // Reg 10193
	NTV2WriteRegisterRemote (kVRegProgressivePicture, 0x00000001);  // Reg 10260
	NTV2WriteRegisterRemote (kVRegLUT2Type, 0x00000002);  // Reg 10261
	NTV2WriteRegisterRemote (kVRegLUT3Type, 0x00000002);  // Reg 10262
	NTV2WriteRegisterRemote (kVRegLUT4Type, 0x00000002);  // Reg 10263
	NTV2WriteRegisterRemote (kVRegRGBRangeConverterLUTType, 0x00000002);  // Reg 10267
	NTV2WriteRegisterRemote (kVRegTestPatternChoice, 0x0000000C);  // Reg 10268
	NTV2WriteRegisterRemote (kVRegEveryFrameTaskFilter, 0x00000001);  // Reg 10270
	NTV2WriteRegisterRemote (kVRegDefaultInput, 0x00000001);  // Reg 10271
	NTV2WriteRegisterRemote (kVRegDefaultVideoOutMode, 0x00000004);  // Reg 10272
	NTV2WriteRegisterRemote (kVRegDefaultVideoFormat, 0x00000065);  // Reg 10273
	NTV2WriteRegisterRemote (kVRegDigitalOutput5Select, 0x0000000B);  // Reg 10274
	NTV2WriteRegisterRemote (kVRegLUT5Type, 0x00000002);  // Reg 10275
	NTV2WriteRegisterRemote (kVRegMacUserModeDebugLevel, 0x00000003);  // Reg 10300
	NTV2WriteRegisterRemote (kVRegMacUserModePingLevel, 0x00000003);  // Reg 10302
	NTV2WriteRegisterRemote (kVRegAudioInputSelect, 0x00000002);  // Reg 10306
	NTV2WriteRegisterRemote (kVRegStartupStatusFlags, 0x00000007);  // Reg 10311
	NTV2WriteRegisterRemote (kVRegRGBRangeMode, 0x00000001);  // Reg 10312
	NTV2WriteRegisterRemote (kVRegAdvancedIndexing, 0x00000001);  // Reg 10340
	NTV2WriteRegisterRemote (kVRegResetCycleCount, 0x00000001);  // Reg 10366
	NTV2WriteRegisterRemote (kVRegUseProgressive, 0x00000001);  // Reg 10367
	NTV2WriteRegisterRemote (kVRegEFTNeedsUpdating, 0x00000001);  // Reg 10373
	NTV2WriteRegisterRemote (kVRegServicesInitialized, 0x00000001);  // Reg 10378
	NTV2WriteRegisterRemote (kVRegChannelCrosspointFirst, 0x00000012);  // Reg 10380
	NTV2WriteRegisterRemote (VIRTUALREG_START+381, 0x00000012);  // Reg 10381
	NTV2WriteRegisterRemote (VIRTUALREG_START+382, 0x00000012);  // Reg 10382
	NTV2WriteRegisterRemote (VIRTUALREG_START+383, 0x00000012);  // Reg 10383
	NTV2WriteRegisterRemote (VIRTUALREG_START+384, 0x00000012);  // Reg 10384
	NTV2WriteRegisterRemote (VIRTUALREG_START+385, 0x00000012);  // Reg 10385
	NTV2WriteRegisterRemote (VIRTUALREG_START+386, 0x00000012);  // Reg 10386
	NTV2WriteRegisterRemote (kVRegChannelCrosspointLast, 0x00000012);  // Reg 10387
	NTV2WriteRegisterRemote (kVRegMonAncField1Offset, 0x00004000);  // Reg 10389
	NTV2WriteRegisterRemote (kVRegMonAncField2Offset, 0x00002000);  // Reg 10390
	NTV2WriteRegisterRemote (kVRegAncField1Offset, 0x00004000);  // Reg 10392
	NTV2WriteRegisterRemote (kVRegAncField2Offset, 0x00002000);  // Reg 10393
	NTV2WriteRegisterRemote (kVRegAgentCheck, 0x00000007);  // Reg 10394
	NTV2WriteRegisterRemote (kVReg4kOutputTransportSelection, 0x00000002);  // Reg 10396
	NTV2WriteRegisterRemote (kVRegVideoFormatCh1, 0x00000065);  // Reg 10401
	NTV2WriteRegisterRemote (kVRegVideoFormatCh2, 0x00000065);  // Reg 10402
	NTV2WriteRegisterRemote (kVRegVideoFormatCh3, 0x00000065);  // Reg 10403
	NTV2WriteRegisterRemote (kVRegVideoFormatCh4, 0x00000065);  // Reg 10404
	NTV2WriteRegisterRemote (kVRegVideoFormatCh5, 0x00000065);  // Reg 10405
	NTV2WriteRegisterRemote (kVRegVideoFormatCh6, 0x00000065);  // Reg 10406
	NTV2WriteRegisterRemote (kVRegVideoFormatCh7, 0x00000065);  // Reg 10407
	NTV2WriteRegisterRemote (kVRegVideoFormatCh8, 0x00000065);  // Reg 10408
	NTV2WriteRegisterRemote (kVRegMailBoxAcquire, 0x00000001);  // Reg 10459
	NTV2WriteRegisterRemote (kVRegMailBoxRelease, 0x00000001);  // Reg 10460
	NTV2WriteRegisterRemote (kVRegMailBoxAbort, 0x00000001);  // Reg 10461
	NTV2WriteRegisterRemote (kVRegAudioMixerSourceMainEnable, 0x00000001);  // Reg 10509
	NTV2WriteRegisterRemote (kVRegAudioMixerSourceAux1Enable, 0x00000001);  // Reg 10510
	NTV2WriteRegisterRemote (kVRegAudioMixerSourceMainGain, 0x00010000);  // Reg 10512
	NTV2WriteRegisterRemote (kVRegAudioMixerSourceAux1Gain, 0x00010000);  // Reg 10513
	NTV2WriteRegisterRemote (kVRegAudioMixerSourceAux2Gain, 0x00010000);  // Reg 10514
	NTV2WriteRegisterRemote (kVRegAudioCapMixerSourceMainEnable, 0x00000001);  // Reg 10515
	NTV2WriteRegisterRemote (kVRegAudioCapMixerSourceMainGain, 0x00010000);  // Reg 10518
	NTV2WriteRegisterRemote (kVRegAudioCapMixerSourceAux1Gain, 0x00010000);  // Reg 10519
	NTV2WriteRegisterRemote (kVRegAudioCapMixerSourceAux2Gain, 0x00010000);  // Reg 10520
	NTV2WriteRegisterRemote (kVRegSwizzle4kInput, 0x00000001);  // Reg 10521
	NTV2WriteRegisterRemote (kVRegHdrColorimetryCh1, 0x0000FFFF);  // Reg 10557
	NTV2WriteRegisterRemote (kVRegHdrTransferCh1, 0x0000FFFF);  // Reg 10558
	NTV2WriteRegisterRemote (kVRegHdrLuminanceCh1, 0x0000FFFF);  // Reg 10559
	NTV2WriteRegisterRemote (kVRegHdrGreenXCh1, 0x0000FFFF);  // Reg 10560
	NTV2WriteRegisterRemote (kVRegHdrGreenYCh1, 0x0000FFFF);  // Reg 10561
	NTV2WriteRegisterRemote (kVRegHdrBlueXCh1, 0x0000FFFF);  // Reg 10562
	NTV2WriteRegisterRemote (kVRegHdrBlueYCh1, 0x0000FFFF);  // Reg 10563
	NTV2WriteRegisterRemote (kVRegHdrRedXCh1, 0x0000FFFF);  // Reg 10564
	NTV2WriteRegisterRemote (kVRegHdrRedYCh1, 0x0000FFFF);  // Reg 10565
	NTV2WriteRegisterRemote (kVRegHdrWhiteXCh1, 0x0000FFFF);  // Reg 10566
	NTV2WriteRegisterRemote (kVRegHdrWhiteYCh1, 0x0000FFFF);  // Reg 10567
	NTV2WriteRegisterRemote (kVRegHdrMasterLumMaxCh1, 0x0000FFFF);  // Reg 10568
	NTV2WriteRegisterRemote (kVRegHdrMasterLumMinCh1, 0x0000FFFF);  // Reg 10569
	NTV2WriteRegisterRemote (kVRegHdrMaxCLLCh1, 0x0000FFFF);  // Reg 10570
	NTV2WriteRegisterRemote (kVRegHdrMaxFALLCh1, 0x0000FFFF);  // Reg 10571
}	//	InitRegs

#endif	//	defined (NTV2_NUB_CLIENT_SUPPORT)
