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
#ifdef MSWindows
	#include <WinSock2.h>
#endif
#include "ntv2discover.h"
#include "ntv2nubpktcom.h"
#include "ntv2nubaccess.h"
#include "ntv2endian.h"

// max number of bytes we can get at once 
#define MAXDATASIZE (sizeof(NTV2NubPktHeader) + NTV2_NUBPKT_MAX_DATASIZE)

#if defined (NTV2_NUB_CLIENT_SUPPORT)

static bool 
isNubOpenRespPacket(NTV2NubPkt *pPkt)
{
	return isNTV2NubPacketType(pPkt, eNubOpenRespPkt);
}


static void * getNubPktPayload(NTV2NubPkt *pPkt) 
{
	NTV2NubPktType pktType = pPkt->hdr.pktType;
	NTV2NubProtocolVersion protocolVersion = pPkt->hdr.protocolVersion;
	const char *queryRespStr = nubQueryRespStr(protocolVersion, pktType);

	return (((char *)pPkt->data) + strlen(queryRespStr) + 1);
}


static NTV2NubPkt *
BuildOpenQueryPacket(ULWord boardNumber, ULWord boardType)
{
	NTV2NubPkt *pPkt;
	char *p;
	
	pPkt = BuildNubBasePacket(	maxKnownProtocolVersion,
								eNubOpenQueryPkt,
								sizeof(NTV2BoardOpenInfo),
								&p);
	if (pPkt == 0)
		return 0;
	
	NTV2BoardOpenInfo *pBOI = (NTV2BoardOpenInfo *)p;
	pBOI->boardNumber = htonl(boardNumber);
	pBOI->boardType = htonl(boardType);

	return pPkt;
}

static bool 
isNubReadRegisterRespPacket(NTV2NubPkt *pPkt)
{
	return isNTV2NubPacketType(pPkt, eNubReadRegisterSingleRespPkt);
}

static bool 
isNubWriteRegisterRespPacket(NTV2NubPkt *pPkt)
{
	return isNTV2NubPacketType(pPkt, eNubWriteRegisterRespPkt);
}

static bool 
isNubGetAutoCirculateRespPacket(NTV2NubPkt *pPkt)
{
	return isNTV2NubPacketType(pPkt, eNubGetAutoCirculateRespPkt);
}

static bool 
isNubControlAutoCirculateRespPacket(NTV2NubPkt *pPkt)
{
	// Also handles eNubV1ControlAutoCirculateRespPkt
	return isNTV2NubPacketType(pPkt, eNubV2ControlAutoCirculateRespPkt);
}

static bool 
isNubWaitForInterruptRespPacket(NTV2NubPkt *pPkt)
{
	return isNTV2NubPacketType(pPkt, eNubWaitForInterruptRespPkt);
}

static bool 
isNubDriverGetBitFileInformationRespPacket(NTV2NubPkt *pPkt)
{
	return isNTV2NubPacketType(pPkt,  eNubDriverGetBitFileInformationRespPkt);
}

static bool 
isNubDriverGetBuildInformationRespPacket(NTV2NubPkt *pPkt)
{
	return isNTV2NubPacketType(pPkt,  eNubDriverGetBuildInformationRespPkt);
}

static bool 
isNubDownloadTestPatternRespPacket(NTV2NubPkt *pPkt)
{
	return isNTV2NubPacketType(pPkt, eNubDownloadTestPatternRespPkt);
}

static bool 
isNubReadRegisterMultiRespPacket(NTV2NubPkt *pPkt)
{
	return isNTV2NubPacketType(pPkt, eNubReadRegisterMultiRespPkt);
}

static NTV2NubPkt *
BuildReadRegisterQueryPacket(	LWord  handle,
								NTV2NubProtocolVersion nubProtocolVersion,
								ULWord registerNumber,
								ULWord registerMask,
								ULWord registerShift)
{
	NTV2NubPkt *pPkt;
	char *p;
	
	pPkt = BuildNubBasePacket(	nubProtocolVersion,
								eNubReadRegisterSingleQueryPkt,
								sizeof(NTV2ReadWriteRegisterPayload),
								&p);
	if (pPkt == 0)
		return 0;
	
	NTV2ReadWriteRegisterPayload *pRWRP = (NTV2ReadWriteRegisterPayload *)p;
	pRWRP->handle = htonl(handle);
	pRWRP->registerNumber = htonl(registerNumber);
	pRWRP->registerValue = htonl(0);	// Value will be here on response 
	pRWRP->registerMask = htonl(registerMask);
	pRWRP->registerShift = htonl(registerShift);

	return pPkt;
}

static NTV2NubPkt *
BuildWriteRegisterQueryPacket(	LWord  handle,
								NTV2NubProtocolVersion nubProtocolVersion,
								ULWord registerNumber,
								ULWord registerValue,
								ULWord registerMask,
								ULWord registerShift)
{
	NTV2NubPkt *pPkt;
	char *p;
	
	pPkt = BuildNubBasePacket(	nubProtocolVersion,
								eNubWriteRegisterQueryPkt,
								sizeof(NTV2ReadWriteRegisterPayload),
								&p);
	if (pPkt == 0)
		return 0;

	NTV2ReadWriteRegisterPayload *pRWRP = (NTV2ReadWriteRegisterPayload *)p;
	pRWRP->handle = htonl(handle);
	pRWRP->registerNumber = htonl(registerNumber);
	pRWRP->registerValue = htonl(registerValue);
	pRWRP->registerMask = htonl(registerMask);
	pRWRP->registerShift = htonl(registerShift);

	return pPkt;
}

static NTV2NubPkt *
BuildAutoCirculateQueryPacket(	LWord  handle,
								NTV2NubProtocolVersion nubProtocolVersion,
								AUTOCIRCULATE_DATA &autoCircData)
{
	// char *queryStr = 0;

	NTV2NubPktType pktType = eNumNTV2NubPktTypes;
	ULWord payloadSize = 0;

	switch(autoCircData.eCommand)
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
			return 0;
	}

	char *p;
	NTV2NubPkt *pPkt;

	pPkt = BuildNubBasePacket(	nubProtocolVersion,
								pktType,
								payloadSize,
								&p);
	if (pPkt == 0)
		return 0;
	
	switch(autoCircData.eCommand)
	{
		case eStartAutoCirc:
		case eStopAutoCirc:
		case eAbortAutoCirc:
		case ePauseAutoCirc:
		case eFlushAutoCirculate:
			{
				NTV2ControlAutoCircPayload *pCACP = (NTV2ControlAutoCircPayload *)p;
				pCACP->handle = htonl(handle);
				pCACP->eCommand = htonl(autoCircData.eCommand);
				pCACP->channelSpec = htonl(autoCircData.channelSpec);
				if (autoCircData.eCommand == ePauseAutoCirc)
				{
					pCACP->bVal1 = htonl((ULWord)autoCircData.bVal1); // bool bPlayToPause
				}
			}
			break;

		case eGetAutoCirc:
			{
				NTV2GetAutoCircPayload *pGACP = (NTV2GetAutoCircPayload *)p;
				pGACP->handle = htonl(handle);
				pGACP->eCommand = htonl(autoCircData.eCommand);
				pGACP->channelSpec = htonl(autoCircData.channelSpec);
			}
			break;

		default: // Not handled yet.
			return 0;
	}

	return pPkt;
}

static NTV2NubPkt *
BuildWaitForInterruptQueryPacket(	LWord  handle,
									NTV2NubProtocolVersion nubProtocolVersion,
									INTERRUPT_ENUMS eInterrupt,
									ULWord timeOutMs)
{
	NTV2NubPkt *pPkt;
	char *p;

	pPkt = BuildNubBasePacket(	nubProtocolVersion,
								eNubWaitForInterruptQueryPkt,
								sizeof(NTV2WaitForInterruptPayload),
								&p);
	if (pPkt == 0)
		return 0;

	NTV2WaitForInterruptPayload *pWFIP = (NTV2WaitForInterruptPayload *)p;
	pWFIP->handle = htonl(handle);
	pWFIP->eInterrupt = htonl((ULWord)eInterrupt);
	pWFIP->timeOutMs = htonl(timeOutMs);

	return pPkt;
}

static NTV2NubPkt *
BuildDriverGetBitFileInformationQueryPacket(LWord  handle,
											NTV2NubProtocolVersion nubProtocolVersion,
											BITFILE_INFO_STRUCT &bitFileInfo,
											NTV2BitFileType bitFileType)
{
	NTV2NubPkt *pPkt;
	char *p;
	
	pPkt = BuildNubBasePacket(	nubProtocolVersion,
								eNubDriverGetBitFileInformationQueryPkt,
								sizeof(NTV2DriverGetBitFileInformationPayload),
								&p);
	if (pPkt == 0)
		return 0;
	
	NTV2DriverGetBitFileInformationPayload *pDGBFIP = (NTV2DriverGetBitFileInformationPayload *)p;
	pDGBFIP->handle = htonl(handle);
	pDGBFIP->bitFileType = htonl((ULWord)bitFileType);
	pDGBFIP->bitFileInfo.whichFPGA = (NTV2XilinxFPGA)htonl((ULWord)bitFileInfo.whichFPGA);

	return pPkt;
}

static NTV2NubPkt *
BuildDriverGetBuildInformationQueryPacket(LWord  handle,
										  NTV2NubProtocolVersion nubProtocolVersion)
										  
{
	NTV2NubPkt *pPkt;
	char *p;
	
	pPkt = BuildNubBasePacket(	nubProtocolVersion,
								eNubDriverGetBuildInformationQueryPkt,
								sizeof(NTV2DriverGetBuildInformationPayload),
								&p);
	if (pPkt == 0)
		return 0;
	
	NTV2DriverGetBuildInformationPayload *pDGBFIP = (NTV2DriverGetBuildInformationPayload *)p;
	pDGBFIP->handle = htonl(handle);

	return pPkt;
}


static NTV2NubPkt *
BuildDownloadTestPatternQueryPacket(LWord  handle,
									NTV2NubProtocolVersion nubProtocolVersion,
									NTV2Channel channel,
									NTV2FrameBufferFormat testPatternFrameBufferFormat,
									UWord signalMask,
									bool testPatternDMAEnable,
									ULWord testPatternNumber)
{
	NTV2NubPkt *pPkt;
	char *p;
	
	pPkt = BuildNubBasePacket(	nubProtocolVersion,
								eNubDownloadTestPatternQueryPkt,
								sizeof(NTV2DownloadTestPatternPayload),
								&p);
	if (pPkt == 0)
		return 0;
	
	NTV2DownloadTestPatternPayload *pDTPP = (NTV2DownloadTestPatternPayload *)p;
	pDTPP->handle = htonl(handle);
	pDTPP->channel = htonl(channel);
	pDTPP->testPatternFrameBufferFormat = htonl(testPatternFrameBufferFormat);
	pDTPP->signalMask = htonl(signalMask);
	pDTPP->testPatternDMAEnable = htonl(testPatternDMAEnable);
	pDTPP->testPatternNumber = htonl(testPatternNumber);

	return pPkt;
}

static NTV2NubPkt *
BuildReadRegisterMultiQueryPacket(	LWord  handle,
									NTV2NubProtocolVersion nubProtocolVersion,
									ULWord numRegs,
									NTV2ReadWriteRegisterSingle aRegs[])
{
	NTV2NubPkt *pPkt;
	char *p;
	
	pPkt = BuildNubBasePacket(	nubProtocolVersion,
								eNubReadRegisterMultiQueryPkt,
								sizeof(NTV2ReadWriteMultiRegisterPayloadHeader) + numRegs * sizeof(NTV2ReadWriteRegisterSingle),
								&p);
	if (pPkt == 0)
		return 0;
	
	// TODO: Move this into BuildNubBasePacket for this pkt type
	// memset(pPkt, 0xab, sizeof(NTV2NubPkt));

	NTV2ReadWriteMultiRegisterPayload *pRWMRP = (NTV2ReadWriteMultiRegisterPayload *)p;
	pRWMRP->payloadHeader.handle = htonl(handle);
	pRWMRP->payloadHeader.numRegs = htonl(numRegs);
	for (ULWord i = 0; i < numRegs; i++)
	{
		pRWMRP->aRegs[i].registerNumber = htonl(aRegs[i].registerNumber);
		pRWMRP->aRegs[i].registerValue = htonl(0);	// Value will be here on response 
		pRWMRP->aRegs[i].registerMask = htonl(aRegs[i].registerMask);
		pRWMRP->aRegs[i].registerShift = htonl(aRegs[i].registerShift);
	}

	return pPkt;
}

int NTV2ConnectToNub(	const char *hostname, 
						AJASocket *sockfd)
{
	struct hostent *he;
	struct sockaddr_in their_addr;

	// get the host info 
    if ((he=gethostbyname(hostname)) == NULL) 
	{
#ifndef MSWindows
        herror("gethostbyname");
#endif
        return -1;
    }

    if ((*sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) 
	{
        perror("socket");
   		return -1;
	}

    their_addr.sin_family = AF_INET;    // host byte order 
    their_addr.sin_port = htons(NTV2NUBPORT);  // short, network byte order 
    their_addr.sin_addr = *((struct in_addr *)he->h_addr);
    memset(&(their_addr.sin_zero), '\0', 8);  // zero the rest of the struct 

	int retval  = connect(*sockfd, (struct sockaddr *)&their_addr, sizeof(struct sockaddr));

    if (retval == -1) 
	{
        perror("connect");
        return -1;
    }	
	return retval;

}


static void
deNBOifyAndCopyGetAutoCirculateData(AUTOCIRCULATE_STATUS_STRUCT *pACStatus, NTV2GetAutoCircPayload *pGACP)
{
	// Some 32 bit quantities
	pACStatus->channelSpec = (NTV2Crosspoint)ntohl(pGACP->channelSpec);
	pACStatus->state = (NTV2AutoCirculateState)ntohl(pGACP->state);
	pACStatus->startFrame = ntohl(pGACP->startFrame);
	pACStatus->endFrame =  ntohl(pGACP->endFrame);
	pACStatus->activeFrame =  ntohl(pGACP->activeFrame);

	// Note: the following four items are 64-bit quantities!
#if defined(AJAMac)	//	'ntohll' was introduced in MacOS 10.9 (Xcode9)
	pACStatus->rdtscStartTime = NTV2EndianSwap64BtoH(pGACP->rdtscStartTime);
	pACStatus->audioClockStartTime = NTV2EndianSwap64BtoH(pGACP->audioClockStartTime);
	pACStatus->rdtscCurrentTime = NTV2EndianSwap64BtoH(pGACP->rdtscCurrentTime);
	pACStatus->audioClockCurrentTime = NTV2EndianSwap64BtoH(pGACP->audioClockCurrentTime);
#else
	pACStatus->rdtscStartTime = ntohll(pGACP->rdtscStartTime);
	pACStatus->audioClockStartTime = ntohll(pGACP->audioClockStartTime);
	pACStatus->rdtscCurrentTime = ntohll(pGACP->rdtscCurrentTime);
	pACStatus->audioClockCurrentTime = ntohll(pGACP->audioClockCurrentTime);
#endif

	// Back to 32 bit quantities.
	pACStatus->framesProcessed = ntohl(pGACP->framesProcessed);
	pACStatus->framesDropped = ntohl(pGACP->framesDropped);
	pACStatus->bufferLevel = ntohl(pGACP->bufferLevel);

	// These are bools, which natively on some systems (Linux) are 1 byte and others 4 (MacOSX)
	// So the portable structures makes them all ULWords.
#ifdef MSWindows
#pragma warning(disable: 4800) 
#endif	
	pACStatus->bWithAudio = (bool)ntohl(pGACP->bWithAudio);
	pACStatus->bWithRP188 = (bool)ntohl(pGACP->bWithRP188);
	pACStatus->bFbfChange = (bool)ntohl(pGACP->bFboChange);
	pACStatus->bWithColorCorrection = (bool)ntohl(pGACP->bWithColorCorrection);
	pACStatus->bWithVidProc = (bool)ntohl(pGACP->bWithVidProc);
	pACStatus->bWithCustomAncData = (bool)ntohl(pGACP->bWithCustomAncData);
#ifdef MSWindows
#pragma warning(default: 4800)
#endif
}

static void
deNBOifyAndCopyGetDriverBitFileInformation( BITFILE_INFO_STRUCT &localBitFileInfo,
											BITFILE_INFO_STRUCT &remoteBitFileInfo)
{
	localBitFileInfo.checksum = ntohl(remoteBitFileInfo.checksum);
	localBitFileInfo.structVersion = ntohl(remoteBitFileInfo.structVersion);
	localBitFileInfo.structSize = ntohl(remoteBitFileInfo.structSize);

	localBitFileInfo.numBytes = ntohl(remoteBitFileInfo.numBytes);
	
	memcpy(localBitFileInfo.dateStr, remoteBitFileInfo.dateStr, NTV2_BITFILE_DATETIME_STRINGLENGTH); 
	memcpy(localBitFileInfo.timeStr, remoteBitFileInfo.timeStr, NTV2_BITFILE_DATETIME_STRINGLENGTH); 
	memcpy(localBitFileInfo.designNameStr , remoteBitFileInfo.designNameStr, NTV2_BITFILE_DESIGNNAME_STRINGLENGTH); 

	localBitFileInfo.bitFileType = ntohl(remoteBitFileInfo.bitFileType);
	localBitFileInfo.whichFPGA = (NTV2XilinxFPGA)ntohl(remoteBitFileInfo.whichFPGA);
}

static void
deNBOifyAndCopyGetDriverBuildInformation( BUILD_INFO_STRUCT &localBuildInfo,
										  BUILD_INFO_STRUCT &remoteBuildInfo)
{
	localBuildInfo.structVersion = ntohl(remoteBuildInfo.structVersion);
	localBuildInfo.structSize = ntohl(remoteBuildInfo.structSize);

	memcpy(localBuildInfo.buildStr, remoteBuildInfo.buildStr, NTV2_BUILD_STRINGLENGTH); 
}



int
NTV2OpenRemoteCard(	AJASocket sockfd, 
					UWord boardNumber, 
					UWord boardType, 
					LWord *handle, 
					NTV2NubProtocolVersion *nubProtocolVersion)
{
	// Connected?
	if (sockfd == -1)
		return NTV2_REMOTE_ACCESS_NOT_CONNECTED; 

	// Construct open query
	NTV2NubPkt *pPkt = BuildOpenQueryPacket(boardNumber, boardType);

	if (pPkt == NULL)
	{
		return NTV2_REMOTE_ACCESS_OUT_OF_MEMORY;
	}

	int retcode = NTV2_REMOTE_ACCESS_SUCCESS;

	int len =  pPkt == 0 ? 0 : sizeof(NTV2NubPktHeader) + pPkt->hdr.dataLength;
	// Send it
	if(NBOifyNTV2NubPkt(pPkt)) 
	{
		if (-1 == sendall(sockfd, (char *)pPkt, &len))
		{
			retcode = NTV2_REMOTE_ACCESS_SEND_ERR;
		}
		else
		{
			// Wait for response
			int numbytes = recvtimeout_sec(sockfd, (char *)pPkt, sizeof(NTV2NubPkt), 2); // 2 second timeout

			switch (numbytes)
			{
				case  0: // Remote side closed connection
						retcode = NTV2_REMOTE_ACCESS_CONNECTION_CLOSED;
						break;

				case -1: // error occurred
						perror("recvtimeout_sec");
						retcode = NTV2_REMOTE_ACCESS_RECV_ERR;
						break;
			
				case -2: // timeout occurred
						retcode = NTV2_REMOTE_ACCESS_TIMEDOUT;
						break;
			
				default: // got some data.  Open response packet?
						if (deNBOifyNTV2NubPkt(pPkt, numbytes)) 
						{
							if (isNubOpenRespPacket(pPkt)) 
							{
								// printf("Got an open response packet\n");
								NTV2BoardOpenInfo * pBoardOpenInfo;
								pBoardOpenInfo = (NTV2BoardOpenInfo *)getNubPktPayload(pPkt);
								*handle = ntohl(pBoardOpenInfo->handle);
								// printf("Handle = %d\n", *handle);
								if (*handle == (LWord)INVALID_NUB_HANDLE)
								{
									printf("Got invalid handle on open response.\n");
									retcode = NTV2_REMOTE_ACCESS_NO_CARD;
								}
								*nubProtocolVersion = pPkt->hdr.protocolVersion;
								 // printf("Got protocol version %d from open response.\n", *nubProtocolVersion);
							}
							else // Not an open response packet, count it and discard it.
							{
								static unsigned long ignoredNTV2pkts;
								++ignoredNTV2pkts;
								retcode = NTV2_REMOTE_ACCESS_NOT_OPEN_RESP;
							}
						}
						else // Non ntv2 packet on our port.
						{
							// NOTE: Defragmentation of jumbo packets would probably go here.
							retcode = NTV2_REMOTE_ACCESS_NON_NUB_PKT;
						}
			}
		}
	}
	delete pPkt;
	return retcode;
}

int
NTV2ReadRegisterRemote(	AJASocket sockfd,
						LWord  handle,
						NTV2NubProtocolVersion nubProtocolVersion,
						ULWord registerNumber,
						ULWord *registerValue,
						ULWord registerMask,
						ULWord registerShift)
{
	// Connected?
	if (sockfd == -1)
		return NTV2_REMOTE_ACCESS_NOT_CONNECTED; 

	// Construct open query
	NTV2NubPkt *pPkt = BuildReadRegisterQueryPacket(handle,
													nubProtocolVersion,
													registerNumber,
													registerMask,
													registerShift);
	if (pPkt == NULL)
	{
		return NTV2_REMOTE_ACCESS_OUT_OF_MEMORY;
	}

	int retcode = NTV2_REMOTE_ACCESS_SUCCESS;

	int len =  pPkt == 0 ? 0 : sizeof(NTV2NubPktHeader) + pPkt->hdr.dataLength;
	// Send it
	if(NBOifyNTV2NubPkt(pPkt)) 
	{
		if (-1 == sendall(sockfd, (char *)pPkt, &len))
		{
			retcode = NTV2_REMOTE_ACCESS_SEND_ERR;
		}
		else
		{
			// Wait for response
			int numbytes = recvtimeout_sec(sockfd, (char *)pPkt, sizeof(NTV2NubPkt), 2); // 2 second timeout

			switch (numbytes)
			{
				case  0: // Remote side closed connection
						retcode = NTV2_REMOTE_ACCESS_CONNECTION_CLOSED;
						break;

				case -1: // error occurred
						perror("recvtimeout_sec");
						retcode = NTV2_REMOTE_ACCESS_RECV_ERR;
						break;
			
				case -2: // timeout occurred
						retcode = NTV2_REMOTE_ACCESS_TIMEDOUT;
						break;
			
				default: // got some data.  Open response packet?
						if (deNBOifyNTV2NubPkt(pPkt, numbytes)) 
						{
							if (isNubReadRegisterRespPacket(pPkt)) 
							{
								// printf("Got a read register response packet\n");
								NTV2ReadWriteRegisterPayload * pRWRP;
								pRWRP = (NTV2ReadWriteRegisterPayload *)getNubPktPayload(pPkt);
								// Did card go away?
								LWord hdl = ntohl(pRWRP->handle);
								// printf("hdl = %d\n", hdl);
								if (hdl == (LWord)INVALID_NUB_HANDLE)
								{
									printf("Got invalid nub handle back from register read.\n");
									retcode = NTV2_REMOTE_ACCESS_NO_CARD;
								}
								ULWord result = ntohl (pRWRP->result);
								if (result)
								{
									*registerValue = ntohl(pRWRP->registerValue);
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
						}
			}
		}
	}
	delete pPkt;
	return retcode;
}

int
NTV2WriteRegisterRemote(AJASocket sockfd,
						LWord  handle,
						NTV2NubProtocolVersion nubProtocolVersion,
						ULWord registerNumber,
						ULWord registerValue,
						ULWord registerMask,
						ULWord registerShift)
{
	// Connected?
	if (sockfd == -1)
		return NTV2_REMOTE_ACCESS_NOT_CONNECTED; 

	// Construct open query
	NTV2NubPkt *pPkt = BuildWriteRegisterQueryPacket(handle,
													nubProtocolVersion,
													registerNumber,
													registerValue,
													registerMask,
													registerShift);
	if (pPkt == NULL)
	{
		return NTV2_REMOTE_ACCESS_OUT_OF_MEMORY;
	}

	int retcode = NTV2_REMOTE_ACCESS_SUCCESS;

	int len =  pPkt == 0 ? 0 : sizeof(NTV2NubPktHeader) + pPkt->hdr.dataLength;
	// Send it
	if(NBOifyNTV2NubPkt(pPkt)) 
	{
		if (-1 == sendall(sockfd, (char *)pPkt, &len))
		{
			retcode = NTV2_REMOTE_ACCESS_SEND_ERR;
		}
		else
		{
			// Wait for response
			int numbytes = recvtimeout_sec(sockfd, (char *)pPkt, sizeof(NTV2NubPkt), 2); // 2 second timeout

			switch (numbytes)
			{
				case  0: // Remote side closed connection
						retcode = NTV2_REMOTE_ACCESS_CONNECTION_CLOSED;
						break;

				case -1: // error occurred
						perror("recvtimeout_sec");
						retcode = NTV2_REMOTE_ACCESS_RECV_ERR;
						break;
			
				case -2: // timeout occurred
						retcode = NTV2_REMOTE_ACCESS_TIMEDOUT;
						break;
			
				default: // got some data.  Open response packet?
						if (deNBOifyNTV2NubPkt(pPkt, numbytes)) 
						{
							if (isNubWriteRegisterRespPacket(pPkt)) 
							{
								// printf("Got a write register response packet\n");
								NTV2ReadWriteRegisterPayload * pRWRP;
								pRWRP = (NTV2ReadWriteRegisterPayload *)getNubPktPayload(pPkt);
								// Did card go away?
								LWord hdl = ntohl(pRWRP->handle);
								// printf("hdl = %d\n", hdl);
								if (hdl == (LWord)INVALID_NUB_HANDLE)
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
									printf("Write Register %d failed on remote side.\n", registerNumber);
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
						}
			}
		}
	}
	delete pPkt;
	return retcode;
}

int 
NTV2AutoCirculateRemote(AJASocket sockfd,
						LWord  handle,
						NTV2NubProtocolVersion nubProtocolVersion,
						AUTOCIRCULATE_DATA &autoCircData)
{
	// Connected?
	if (sockfd == -1)
		return NTV2_REMOTE_ACCESS_NOT_CONNECTED; 

	// Construct autocirculate query packet.
	NTV2NubPkt *pPkt = BuildAutoCirculateQueryPacket(handle,
													nubProtocolVersion,
													autoCircData);
	if (pPkt == NULL)
	{
		return NTV2_REMOTE_ACCESS_OUT_OF_MEMORY;
	}

	int retcode = NTV2_REMOTE_ACCESS_SUCCESS;

	int len =  pPkt == 0 ? 0 : sizeof(NTV2NubPktHeader) + pPkt->hdr.dataLength;
	// Send it
	if(NBOifyNTV2NubPkt(pPkt)) 
	{
		if (-1 == sendall(sockfd, (char *)pPkt, &len))
		{
			retcode = NTV2_REMOTE_ACCESS_SEND_ERR;
		}
		else
		{
			// Wait for response
			int numbytes = recvtimeout_sec(sockfd, (char *)pPkt, sizeof(NTV2NubPkt), 2); // 2 second timeout

			switch (numbytes)
			{
				case  0: // Remote side closed connection
						retcode = NTV2_REMOTE_ACCESS_CONNECTION_CLOSED;
						break;

				case -1: // error occurred
						perror("recvtimeout_sec");
						retcode = NTV2_REMOTE_ACCESS_RECV_ERR;
						break;
			
				case -2: // timeout occurred
						retcode = NTV2_REMOTE_ACCESS_TIMEDOUT;
						break;
			
				default: // got some data.  Autocirculate response packet?
						if (deNBOifyNTV2NubPkt((NTV2NubPkt *)pPkt, numbytes)) 
						{
							if (isNubGetAutoCirculateRespPacket((NTV2NubPkt *)pPkt)) 
							{
								// printf("Got an autocirculate response packet\n");
								NTV2GetAutoCircPayload * pGACP;
								pGACP = (NTV2GetAutoCircPayload *)getNubPktPayload(pPkt);
								// Did card go away?
								LWord hdl = ntohl(pGACP->handle);
								// printf("hdl = %d\n", hdl);
								if (hdl == (LWord)INVALID_NUB_HANDLE)
								{
									printf("Got invalid nub handle back from register write.\n");
									retcode = NTV2_REMOTE_ACCESS_NO_CARD;
								}
								ULWord result = ntohl (pGACP->result);
								if (result)
								{
									// Success
									deNBOifyAndCopyGetAutoCirculateData((AUTOCIRCULATE_STATUS_STRUCT *)autoCircData.pvVal1, pGACP);
								}
								else // GetAutocirculate failed on remote side
								{
									printf("Autocirculate GET failed on remote side.\n");
								}
							}
							else if (isNubControlAutoCirculateRespPacket((NTV2NubPkt *)pPkt)) 
							{
								// printf("Got an autocirculate response packet\n");
								NTV2ControlAutoCircPayload * pCACP;
								pCACP = (NTV2ControlAutoCircPayload *)getNubPktPayload(pPkt);
								// Did card go away?
								LWord hdl = ntohl(pCACP->handle);
								// printf("hdl = %d\n", hdl);
								if (hdl == (LWord)INVALID_NUB_HANDLE)
								{
									printf("Got invalid nub handle back from register write.\n");
									retcode = NTV2_REMOTE_ACCESS_NO_CARD;
								}
								ULWord result = ntohl (pCACP->result);
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
						}
			}
		}
	}
	delete pPkt;
	return retcode;
}

int
NTV2WaitForInterruptRemote(AJASocket sockfd,
						LWord  handle,
						NTV2NubProtocolVersion nubProtocolVersion,
						INTERRUPT_ENUMS eInterrupt,
						ULWord timeOutMs)
{
	// Connected?
	if (sockfd == -1)
		return NTV2_REMOTE_ACCESS_NOT_CONNECTED; 

	// Construct open query
	NTV2NubPkt *pPkt = BuildWaitForInterruptQueryPacket(handle,
														nubProtocolVersion,
														eInterrupt,
														timeOutMs);
	if (pPkt == NULL)
	{
		return NTV2_REMOTE_ACCESS_OUT_OF_MEMORY;
	}

	int retcode = NTV2_REMOTE_ACCESS_SUCCESS;

	int len =  pPkt == 0 ? 0 : sizeof(NTV2NubPktHeader) + pPkt->hdr.dataLength;
	// Send it
	if(NBOifyNTV2NubPkt(pPkt)) 
	{
		if (-1 == sendall(sockfd, (char *)pPkt, &len))
		{
			retcode = NTV2_REMOTE_ACCESS_SEND_ERR;
		}
		else
		{
			// Wait for response
			int numbytes = recvtimeout_sec(sockfd, (char *)pPkt, sizeof(NTV2NubPkt), 2); // 2 second timeout

			switch (numbytes)
			{
				case  0: // Remote side closed connection
						retcode = NTV2_REMOTE_ACCESS_CONNECTION_CLOSED;
						break;

				case -1: // error occurred
						perror("recvtimeout_sec");
						retcode = NTV2_REMOTE_ACCESS_RECV_ERR;
						break;
			
				case -2: // timeout occurred
						retcode = NTV2_REMOTE_ACCESS_TIMEDOUT;
						break;
			
				default: // got some data.  Open response packet?
						if (deNBOifyNTV2NubPkt((NTV2NubPkt *)pPkt, numbytes)) 
						{
							if (isNubWaitForInterruptRespPacket((NTV2NubPkt *)pPkt)) 
							{
								// printf("Got a write register response packet\n");
								NTV2WaitForInterruptPayload * pWFIP;
								pWFIP = (NTV2WaitForInterruptPayload *)getNubPktPayload(pPkt);
								// Did card go away?
								LWord hdl = ntohl(pWFIP->handle);
								// printf("hdl = %d\n", hdl);
								if (hdl == (LWord)INVALID_NUB_HANDLE)
								{
									printf("Got invalid nub handle back from register write.\n");
									retcode = NTV2_REMOTE_ACCESS_NO_CARD;
								}
								ULWord result = ntohl (pWFIP->result);
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
						}
			}
		}
	}
	delete pPkt;
	return retcode;
}

int
NTV2DriverGetBitFileInformationRemote(	AJASocket sockfd,
										LWord  handle,
										NTV2NubProtocolVersion nubProtocolVersion,
										BITFILE_INFO_STRUCT &bitFileInfo,
										NTV2BitFileType bitFileType)
{
	// Connected?
	if (sockfd == -1)
		return NTV2_REMOTE_ACCESS_NOT_CONNECTED; 

	// Construct open query
	NTV2NubPkt *pPkt = BuildDriverGetBitFileInformationQueryPacket(handle,
															nubProtocolVersion,
															bitFileInfo,
															bitFileType);
	if (pPkt == NULL)
	{
		return NTV2_REMOTE_ACCESS_OUT_OF_MEMORY;
	}

	int retcode = NTV2_REMOTE_ACCESS_SUCCESS;

	int len =  pPkt == 0 ? 0 : sizeof(NTV2NubPktHeader) + pPkt->hdr.dataLength;
	// Send it
	if(NBOifyNTV2NubPkt(pPkt)) 
	{
		if (-1 == sendall(sockfd, (char *)pPkt, &len))
		{
			retcode = NTV2_REMOTE_ACCESS_SEND_ERR;
		}
		else
		{
			// Wait for response
			int numbytes = recvtimeout_sec(sockfd, (char *)pPkt, sizeof(NTV2NubPkt), 2); // 2 second timeout

			switch (numbytes)
			{
				case  0: // Remote side closed connection
						retcode = NTV2_REMOTE_ACCESS_CONNECTION_CLOSED;
						break;

				case -1: // error occurred
						perror("recvtimeout_sec");
						retcode = NTV2_REMOTE_ACCESS_RECV_ERR;
						break;
			
				case -2: // timeout occurred
						retcode = NTV2_REMOTE_ACCESS_TIMEDOUT;
						break;
			
				default: // got some data.  Open response packet?
						if (deNBOifyNTV2NubPkt((NTV2NubPkt *)pPkt, numbytes)) 
						{
							if (isNubDriverGetBitFileInformationRespPacket((NTV2NubPkt *)pPkt)) 
							{
								// printf("Got a driver get bitfile info response packet\n");
								NTV2DriverGetBitFileInformationPayload * pDGBFIP;
								pDGBFIP = (NTV2DriverGetBitFileInformationPayload *)getNubPktPayload(pPkt);
								// Did card go away?
								LWord hdl = ntohl(pDGBFIP->handle);
								// printf("hdl = %d\n", hdl);
								if (hdl == (LWord)INVALID_NUB_HANDLE)
								{
									printf("Got invalid nub handle back from get bitfile info.\n");
									retcode = NTV2_REMOTE_ACCESS_NO_CARD;
								}
								ULWord result = ntohl (pDGBFIP->result);
								if (result)
								{
									deNBOifyAndCopyGetDriverBitFileInformation(bitFileInfo, pDGBFIP->bitFileInfo);
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
						}
			}
		}
	}
	delete pPkt;
	return retcode;
}

int
NTV2DriverGetBuildInformationRemote(	AJASocket sockfd,
										LWord  handle,
										NTV2NubProtocolVersion nubProtocolVersion,
										BUILD_INFO_STRUCT &buildInfo)
{
	// Connected?
	if (sockfd == -1)
		return NTV2_REMOTE_ACCESS_NOT_CONNECTED; 

	// Construct open query
	NTV2NubPkt *pPkt = BuildDriverGetBuildInformationQueryPacket(handle,
																 nubProtocolVersion);
	if (pPkt == NULL)
	{
		return NTV2_REMOTE_ACCESS_OUT_OF_MEMORY;
	}

	int retcode = NTV2_REMOTE_ACCESS_SUCCESS;

	int len =  pPkt == 0 ? 0 : sizeof(NTV2NubPktHeader) + pPkt->hdr.dataLength;
	// Send it
	if(NBOifyNTV2NubPkt(pPkt)) 
	{
		if (-1 == sendall(sockfd, (char *)pPkt, &len))
		{
			retcode = NTV2_REMOTE_ACCESS_SEND_ERR;
		}
		else
		{
			// Wait for response
			int numbytes = recvtimeout_sec(sockfd, (char *)pPkt, sizeof(NTV2NubPkt), 2); // 2 second timeout

			switch (numbytes)
			{
				case  0: // Remote side closed connection
						retcode = NTV2_REMOTE_ACCESS_CONNECTION_CLOSED;
						break;

				case -1: // error occurred
						perror("recvtimeout_sec");
						retcode = NTV2_REMOTE_ACCESS_RECV_ERR;
						break;
			
				case -2: // timeout occurred
						retcode = NTV2_REMOTE_ACCESS_TIMEDOUT;
						break;
			
				default: // got some data.  Open response packet?
						if (deNBOifyNTV2NubPkt((NTV2NubPkt *)pPkt, numbytes)) 
						{
							if (isNubDriverGetBuildInformationRespPacket((NTV2NubPkt *)pPkt)) 
							{
								// printf("Got a driver get build info response packet\n");
								NTV2DriverGetBuildInformationPayload * pDGBIP;
								pDGBIP = (NTV2DriverGetBuildInformationPayload *)getNubPktPayload(pPkt);
								// Did card go away?
								LWord hdl = ntohl(pDGBIP->handle);
								// printf("hdl = %d\n", hdl);
								if (hdl == (LWord)INVALID_NUB_HANDLE)
								{
									printf("Got invalid nub handle back from get build info.\n");
									retcode = NTV2_REMOTE_ACCESS_NO_CARD;
								}
								ULWord result = ntohl (pDGBIP->result);
								if (result)
								{
									deNBOifyAndCopyGetDriverBuildInformation(buildInfo, pDGBIP->buildInfo);
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
						}
			}
		}
	}
	delete pPkt;
	return retcode;
}

int
NTV2DownloadTestPatternRemote(AJASocket sockfd,	
								LWord handle,
								NTV2NubProtocolVersion nubProtocolVersion,
								NTV2Channel channel,
								NTV2FrameBufferFormat testPatternFrameBufferFormat,
								UWord signalMask,
								bool testPatternDMAEnable,
								ULWord testPatternNumber)
{
	// Connected?
	if (sockfd == -1)
		return NTV2_REMOTE_ACCESS_NOT_CONNECTED; 

	// Construct open query
	NTV2NubPkt *pPkt = BuildDownloadTestPatternQueryPacket(	handle,
															nubProtocolVersion,
															channel,
															testPatternFrameBufferFormat,
															signalMask,
															testPatternDMAEnable,
															testPatternNumber);
	if (pPkt == NULL)
	{
		return NTV2_REMOTE_ACCESS_OUT_OF_MEMORY;
	}

	int retcode = NTV2_REMOTE_ACCESS_SUCCESS;

	int len =  pPkt == 0 ? 0 : sizeof(NTV2NubPktHeader) + pPkt->hdr.dataLength;
	// Send it
	if(NBOifyNTV2NubPkt(pPkt)) 
	{
		if (-1 == sendall(sockfd, (char *)pPkt, &len))
		{
			retcode = NTV2_REMOTE_ACCESS_SEND_ERR;
		}
		else
		{
			// Wait for response
			int numbytes = recvtimeout_sec(sockfd, (char *)pPkt, sizeof(NTV2NubPkt), 2); // 2 second timeout

			switch (numbytes)
			{
				case  0: // Remote side closed connection
						retcode = NTV2_REMOTE_ACCESS_CONNECTION_CLOSED;
						break;

				case -1: // error occurred
						perror("recvtimeout_sec");
						retcode = NTV2_REMOTE_ACCESS_RECV_ERR;
						break;
			
				case -2: // timeout occurred
						retcode = NTV2_REMOTE_ACCESS_TIMEDOUT;
						break;
			
				default: // got some data.  Open response packet?
						if (deNBOifyNTV2NubPkt((NTV2NubPkt *)pPkt, numbytes)) 
						{
							if (isNubDownloadTestPatternRespPacket((NTV2NubPkt *)pPkt)) 
							{
								// printf("Got a download test pattern response packet\n");
								NTV2DownloadTestPatternPayload * pDTPP;
								pDTPP = (NTV2DownloadTestPatternPayload *)getNubPktPayload(pPkt);
								// Did card go away?
								LWord hdl = ntohl(pDTPP->handle);
								// printf("hdl = %d\n", hdl);
								if (hdl == (LWord)INVALID_NUB_HANDLE)
								{
									printf("Got invalid nub handle back from download test pattern.\n");
									retcode = NTV2_REMOTE_ACCESS_NO_CARD;
								}
								ULWord result = ntohl (pDTPP->result);
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
						}
			}
		}
	}
	delete pPkt;
	return retcode;
}

int 
NTV2ReadRegisterMultiRemote(AJASocket sockfd,
							LWord handle,
							NTV2NubProtocolVersion nubProtocolVersion,
							ULWord numRegs,
							ULWord *whichRegisterFailed,
							NTV2ReadWriteRegisterSingle aRegs[])
{
	// Connected?
	if (sockfd == -1)
		return NTV2_REMOTE_ACCESS_NOT_CONNECTED; 

	// Construct open query
	NTV2NubPkt *pPkt = BuildReadRegisterMultiQueryPacket(	handle,
															nubProtocolVersion,
															numRegs,
															aRegs);
	if (pPkt == NULL)
	{
		return NTV2_REMOTE_ACCESS_OUT_OF_MEMORY;
	}

	int retcode = NTV2_REMOTE_ACCESS_SUCCESS;

	int len =  pPkt == 0 ? 0 : sizeof(NTV2NubPktHeader) + pPkt->hdr.dataLength;
	// Send it
	if(NBOifyNTV2NubPkt(pPkt)) 
	{
		if (-1 == sendall(sockfd, (char *)pPkt, &len))
		{
			retcode = NTV2_REMOTE_ACCESS_SEND_ERR;
		}
		else
		{
			// Wait for response

			LWord maxPktFetchsize = 0;
			maxPktFetchsize += sizeof(NTV2ReadWriteMultiRegisterPayloadHeader);
			maxPktFetchsize += sizeof(NTV2NubPktHeader);
			maxPktFetchsize += (LWord)strlen(nubQueryRespStr(nubProtocolVersion, eNubReadRegisterMultiRespPkt)) + 1;
			maxPktFetchsize += numRegs * sizeof(NTV2ReadWriteRegisterSingle);

			int numbytestotal = 0;
			int defragAttemptCount = 0;
			defrag:
			int numbytes = recvtimeout_usec(sockfd, ((char *)pPkt)+numbytestotal, maxPktFetchsize - numbytestotal, 250000L); // 500 msec timeout

			if (++defragAttemptCount > 3)
			{
				retcode = NTV2_REMOTE_ACCESS_TIMEDOUT;
			}
			else switch (numbytes)
			{
				case  0: // Remote side closed connection
						retcode = NTV2_REMOTE_ACCESS_CONNECTION_CLOSED;
						break;

				case -1: // error occurred
						perror("recvtimeout_sec");
						retcode = NTV2_REMOTE_ACCESS_RECV_ERR;
						break;
			
				case -2: // timeout occurred
						retcode = NTV2_REMOTE_ACCESS_TIMEDOUT;
						break;
			
				default: // got some data.  Open response packet?
						numbytestotal += numbytes;
						if (numbytestotal <  maxPktFetchsize)
							goto defrag;

						if (deNBOifyNTV2NubPkt((NTV2NubPkt *)pPkt, numbytestotal)) 
						{
							if (isNubReadRegisterMultiRespPacket((NTV2NubPkt *)pPkt)) 
							{
								// printf("Got a read register multi response packet\n");
								NTV2ReadWriteMultiRegisterPayload * pRWMRP;
								pRWMRP = (NTV2ReadWriteMultiRegisterPayload *)getNubPktPayload(pPkt);
								// Did card go away?
								LWord hdl = ntohl(pRWMRP->payloadHeader.handle);
								// printf("hdl = %d\n", hdl);
								if (hdl == (LWord)INVALID_NUB_HANDLE)
								{
									printf("Got invalid nub handle back from read reg multi.\n");
									retcode = NTV2_REMOTE_ACCESS_NO_CARD;
								}
								ULWord result = ntohl (pRWMRP->payloadHeader.result);
								*whichRegisterFailed = ntohl(pRWMRP->payloadHeader.whichRegisterFailed);

								if (result)
								{
									// printf("Read register multi remote succeeded.\n");
									for (ULWord i = 0; i < numRegs; i++)
									{
										aRegs[i].registerNumber = ntohl(pRWMRP->aRegs[i].registerNumber);
										aRegs[i].registerValue = ntohl(pRWMRP->aRegs[i].registerValue);
									}
								}
								else // Read register failed on remote side
								{
									// Guard against buffer overrun from remote side
									ULWord maxRegs = *whichRegisterFailed;
									if (maxRegs > numRegs)
									{
										maxRegs = numRegs;
									}
									
									printf("Read register multi failed on remote side on register number %d.\n", *whichRegisterFailed);
									retcode = NTV2_REMOTE_ACCESS_READ_REG_MULTI_FAILED;
									for (ULWord i = 0; i < maxRegs; i++)
									{
										aRegs[i].registerValue = ntohl(pRWMRP->aRegs[i].registerValue);
									}
								}
							}
							else // Not a write register response packet, count it and discard it.
							{
								static unsigned long ignoredNTV2pkts;
								++ignoredNTV2pkts;
								retcode = NTV2_REMOTE_ACCESS_NOT_READ_REG_MULTI;
							}
						}
						else // Non ntv2 packet on our port.
						{
							// NOTE: Defragmentation of jumbo packets would probably go here.
							retcode = NTV2_REMOTE_ACCESS_NON_NUB_PKT;
						}
			}
		}
	}
	delete pPkt;
	return retcode;
}

#endif	//	defined (NTV2_NUB_CLIENT_SUPPORT)
