/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2legacyserver.cpp
	@brief		Implements NTV2LegacyNubServer.
	@copyright	(C) 2019-2022 AJA Video Systems, Inc.
**/

#include "ntv2utils.h"
#include "ajabase/system/debug.h"
#include "ntv2nubtypes.h"
#include "ntv2devicescanner.h"
#include "ntv2legacycommon.h"
#ifdef MSWindows
	#include "windows.h"
	#include "winsock.h"
	#define snprintf _snprintf
	#define vsnprintf _vsnprintf
	#define close closesocket
#else
	#include <ctype.h>
	#include <unistd.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <sys/wait.h>
	#include <signal.h>
	#include <pthread.h>
	#include <fcntl.h>
#endif

using namespace std;

// if this is #defined, then we use a thread for the discovery
// queries, otherwise we fork
#define USING_DISCOVERY_THREAD
#define N_QUERY_RESPONSES       1
#define DEBUG_RESPONSE_LOG      0

#define	NBFAIL(__x__)	AJA_sERROR  (AJA_DebugUnit_RPCServer, AJAFUNC << ": " << __x__)
#define	NBWARN(__x__)	AJA_sWARNING(AJA_DebugUnit_RPCServer, AJAFUNC << ": " << __x__)
#define	NBNOTE(__x__)	AJA_sNOTICE (AJA_DebugUnit_RPCServer, AJAFUNC << ": " << __x__)
#define	NBINFO(__x__)	AJA_sINFO   (AJA_DebugUnit_RPCServer, AJAFUNC << ": " << __x__)
#define	NBDBG(__x__)	AJA_sDEBUG  (AJA_DebugUnit_RPCServer, AJAFUNC << ": " << __x__)

#define NTV2MAXOPENBOARDFDS 32

class openBoardEntry
{
	public:
		openBoardEntry (const ULWord inCookie, const ULWord inBoardNum, const int inFD);
		~openBoardEntry();
		inline ULWord					cookie(void) const		{return fCookie;}
		inline ULWord					boardNum(void) const	{return fBoardNum;}
		inline ULWord					refCount(void) const	{return fRefCount;}
		inline ULWord &					refCount(void)			{return fRefCount;}
		inline CNTV2Card *				card(void)				{return fpCard;}
		inline const std::vector<int> &	FDs(void) const			{return fFDs;}
		inline std::vector<int> &		FDs(void)				{return fFDs;}
		bool							removeFD (const int inFD);
	private:
		ULWord		fCookie;			// Key
		ULWord		fBoardNum;
		ULWord		fBoardType;
		CNTV2Card*	fpCard;
		ULWord		fRefCount;
		// TODO: The file descriptors per board should be a linked
		// list too but I just wanna get this thing working for now
		std::vector<int>	fFDs;	// FD of socket to remote end
};


bool isDiscoverQueryPacket (NTV2NubPkt *pPkt)
{
	return isNTV2NubPacketType(pPkt, eDiscoverQueryPkt);
}

bool isNubOpenQueryPacket (NTV2NubPkt *pPkt)
{
	return isNTV2NubPacketType(pPkt, eNubOpenQueryPkt);
}

bool isNubReadSingleRegisterQueryPacket (NTV2NubPkt *pPkt)
{
	return isNTV2NubPacketType(pPkt, eNubReadRegisterSingleQueryPkt);
}

bool isNubWriteRegisterQueryPacket (NTV2NubPkt *pPkt)
{
	return isNTV2NubPacketType(pPkt, eNubWriteRegisterQueryPkt);
}

bool isNubGetAutocirculateQueryPacket (NTV2NubPkt *pPkt)
{
	return isNTV2NubPacketType(pPkt, eNubGetAutoCirculateQueryPkt);
}

bool isNubControlAutocirculateQueryPacket (NTV2NubPkt *pPkt)
{
	// Note: also handles eNubV1ControlAutoCirculateQueryPkt
	return isNTV2NubPacketType(pPkt, eNubV2ControlAutoCirculateQueryPkt);
}

bool isNubWaitForInterruptQueryPacket (NTV2NubPkt *pPkt)
{
	return isNTV2NubPacketType(pPkt, eNubWaitForInterruptQueryPkt);
}

bool isNubDriverGetBitFileInformationQueryPacket (NTV2NubPkt *pPkt)
{
	return isNTV2NubPacketType(pPkt, eNubDriverGetBitFileInformationQueryPkt);
}

bool isNubDownloadTestPatternQueryPacket (NTV2NubPkt *pPkt)
{
	return isNTV2NubPacketType(pPkt, eNubDownloadTestPatternQueryPkt);
}

bool isNubReadMultiRegisterQueryPacket (NTV2NubPkt *pPkt)
{
	return isNTV2NubPacketType(pPkt, eNubReadRegisterMultiQueryPkt);
}

bool isNubGetDriverVersionQueryPacket (NTV2NubPkt *pPkt)
{
	return isNTV2NubPacketType(pPkt, eNubGetDriverVersionQueryPkt);
}

bool isNubDriverGetBuildInformationQueryPacket (NTV2NubPkt *pPkt)
{
	return isNTV2NubPacketType(pPkt, eNubDriverGetBuildInformationQueryPkt);
}



void sigchld_handler(int s)
{
	(void) s;
	#ifndef MSWindows
		while (waitpid(-1, NULL, WNOHANG) > 0)
			;
	#endif
}


//////////////////////////////////////////////////////////////////////////////
static void InventoryBoards (NTV2DiscoverRespPayload *boardInventory)
{
	CNTV2Card	device;
	UWord		deviceNdx(0);
	while (CNTV2DeviceScanner::GetDeviceAtIndex(deviceNdx, device))
	{
		const ULWord ndx(boardInventory->numBoards);
		boardInventory->discoverBoardInfo[ndx].boardNumber	= deviceNdx;
		boardInventory->discoverBoardInfo[ndx].boardType	= 0;	//	obsolete
		boardInventory->discoverBoardInfo[ndx].boardID		= ULWord(device.GetDeviceID());
		::strncpy(boardInventory->discoverBoardInfo[ndx].description, device.GetDisplayName().c_str(), NTV2_DISCOVER_BOARDINFO_DESC_STRMAX);
		boardInventory->discoverBoardInfo[ndx].description[NTV2_DISCOVER_BOARDINFO_DESC_STRMAX-1] = 0;
		++boardInventory->numBoards;
		deviceNdx++;
	}
}

NTV2NubPkt * BuildDiscoverRespPacket (NTV2NubProtocolVersion protocolVersion)
{
	char * p(AJA_NULL);
	NTV2NubPkt * pPkt (BuildNubBasePacket (protocolVersion,  eDiscoverRespPkt,  sizeof(NTV2DiscoverRespPayload),  &p));
	if (!pPkt)
		return AJA_NULL;

	NTV2DiscoverRespPayload boardInventory;
	::memset (&boardInventory, 0, sizeof(boardInventory));
	InventoryBoards(&boardInventory);

	// Convert to network byte order
	for (ULWord boardNum(0);  boardNum < boardInventory.numBoards;  boardNum++)
	{
		NTV2DiscoverBoardInfo * dbi(&boardInventory.discoverBoardInfo[boardNum]);

		dbi->boardNumber = htonl(dbi->boardNumber);
		dbi->boardType = htonl(dbi->boardType);
		dbi->boardID = htonl(dbi->boardID);
		// Description is ASCII string, okay as-is.
	}
	// Done with numBoard member so we can convert it now.
	boardInventory.numBoards		= htonl(boardInventory.numBoards);

	// Add device table.  Note, could be unaligned.
	memcpy(p, &boardInventory, sizeof(NTV2DiscoverRespPayload));
	return pPkt;
}

void ParseNubOpenQueryPacket (NTV2NubPkt * pPkt, ULWord & outDeviceIndex)
{
	NTV2BoardOpenInfo *boardOpenInfo = (NTV2BoardOpenInfo *)GetNubPktPayloadPtr(pPkt);
	outDeviceIndex = ntohl(boardOpenInfo->boardNumber);
}

void ParseNubReadRegisterSingleQueryPacket(NTV2NubPkt *pPkt, LWord *cookie, ULWord *registerNumber, ULWord *registerMask, ULWord *registerShift)
{
	NTV2ReadWriteRegisterPayload *pRWRP = 
		(NTV2ReadWriteRegisterPayload *) GetNubPktPayloadPtr(pPkt);
	*cookie = ntohl(pRWRP->handle);
	*registerNumber= ntohl(pRWRP->registerNumber);
	*registerMask= ntohl(pRWRP->registerMask);
	*registerShift= ntohl(pRWRP->registerShift);
}

void ParseNubDiscoverQueryPacket(NTV2NubPkt *pPkt, UWord *boardMask)
{
	NTV2DiscoverQueryPayload *pDQP =
		(NTV2DiscoverQueryPayload *)GetNubPktPayloadPtr(pPkt);
	*boardMask = (UWord)ntohl(pDQP->boardMask);
	// printf("ParseNubDiscoverQueryPacket(): Received Boardmask of 0x%x\n", *boardMask);
}


void ParseNubWriteRegisterQueryPacket(NTV2NubPkt *pPkt, LWord *cookie, ULWord *registerNumber, ULWord *registerValue, ULWord *registerMask, ULWord *registerShift)
{
	NTV2ReadWriteRegisterPayload *pRWRP =
		(NTV2ReadWriteRegisterPayload *)GetNubPktPayloadPtr(pPkt);

	*cookie = ntohl(pRWRP->handle);
	*registerNumber = ntohl(pRWRP->registerNumber);
	*registerValue = ntohl(pRWRP->registerValue);
	*registerMask = ntohl(pRWRP->registerMask);
	*registerShift = ntohl(pRWRP->registerShift);
}

void ParseNubGetAutoCirculateQueryPacket(NTV2NubPkt *pPkt, LWord *cookie, NTV2Crosspoint *channelSpec)
{
	NTV2GetAutoCircPayload *pGACP = (NTV2GetAutoCircPayload *)GetNubPktPayloadPtr(pPkt);
	*cookie = ntohl(pGACP->handle);
	*channelSpec = (NTV2Crosspoint)ntohl(pGACP->channelSpec);
}

void ParseNubControlAutoCirculateQueryPacket(NTV2NubPkt *pPkt, 
									LWord *cookie, 
									AUTO_CIRC_COMMAND *eCommand, 
									NTV2Crosspoint *channelSpec,
									bool *bPlayToPause)
{
	NTV2ControlAutoCircPayload *pCACP;
	if (pPkt->hdr.protocolVersion == ntv2NubProtocolVersion1)
	{
		// Fix duplicate packet type
		NTV2NubPkt adjustedPkt;

		adjustedPkt = *pPkt;
		adjustedPkt.hdr.pktType = eNubV2ControlAutoCirculateQueryPkt;
		pCACP = (NTV2ControlAutoCircPayload *)GetNubPktPayloadPtr(&adjustedPkt);
	}
	else
		pCACP = (NTV2ControlAutoCircPayload *)GetNubPktPayloadPtr(pPkt);
	
	*cookie = ntohl(pCACP->handle);
	*eCommand = (AUTO_CIRC_COMMAND)ntohl(pCACP->eCommand);
	*channelSpec = (NTV2Crosspoint)ntohl(pCACP->channelSpec);
	*bPlayToPause = ntohl(pCACP->bVal1) ? true : false;	// Note: only valid if eCommand == ePauseAutoCirc:
}

void ParseNubWaitForInterruptQueryPacket(NTV2NubPkt *pPkt, LWord *cookie, INTERRUPT_ENUMS *eInterrupt, ULWord *timeOutMs)
{
	NTV2WaitForInterruptPayload *pWFIP =
		(NTV2WaitForInterruptPayload *)GetNubPktPayloadPtr(pPkt);

	*cookie = ntohl(pWFIP->handle);
	*eInterrupt= (INTERRUPT_ENUMS)ntohl(pWFIP->eInterrupt);
	*timeOutMs= ntohl(pWFIP->timeOutMs);
}

void ParseNubDriverGetBitFileInformationQueryPacket (NTV2NubPkt *pPkt, 
													LWord *cookie, 
													NTV2BitFileType *bitFileType,
													NTV2XilinxFPGA *whichFPGA)
{
	NTV2DriverGetBitFileInformationPayload *pDGBFIP =
		(NTV2DriverGetBitFileInformationPayload *)GetNubPktPayloadPtr(pPkt);

	*cookie = ntohl(pDGBFIP->handle);
	*bitFileType = (NTV2BitFileType)ntohl(pDGBFIP->bitFileType);
	*whichFPGA = (NTV2XilinxFPGA)ntohl(pDGBFIP->bitFileInfo.whichFPGA);
}

void ParseNubDriverGetBuildInformationQueryPacket (NTV2NubPkt *pPkt, 
													LWord *cookie,
													BUILD_INFO_STRUCT *pBuildInfo)
{
	NTV2DriverGetBuildInformationPayload *pDGBIP =
		(NTV2DriverGetBuildInformationPayload *)GetNubPktPayloadPtr(pPkt);

	*cookie = ntohl(pDGBIP->handle);
	pBuildInfo->structVersion = ntohl(pDGBIP->buildInfo.structVersion);
	pBuildInfo->structSize = ntohl(pDGBIP->buildInfo.structSize);
}


void ParseNubDownloadTestPatternQueryPacket (NTV2NubPkt *pPkt, 
											LWord *cookie, 
											NTV2Channel *channel,
											NTV2FrameBufferFormat *testPatternFrameBufferFormat,
											UWord *signalMask,
											bool *testPatternDMAEnable,
											ULWord *testPatternNumber)
{
	NTV2DownloadTestPatternPayload *pDTPP =
		(NTV2DownloadTestPatternPayload *)GetNubPktPayloadPtr(pPkt);

	*cookie = ntohl(pDTPP->handle);
	*channel = (NTV2Channel)ntohl(pDTPP->channel);
	*testPatternFrameBufferFormat = (NTV2FrameBufferFormat)ntohl(pDTPP->testPatternFrameBufferFormat);
	*signalMask = (UWord)ntohl(pDTPP->signalMask);
	*testPatternDMAEnable = ntohl(pDTPP->testPatternDMAEnable) ? true : false;
	*testPatternNumber = ntohl(pDTPP->testPatternNumber);
}

void ParseNubReadRegisterMultiQueryPacket (NTV2NubPkt *pPkt, LWord *cookie, ULWord *numRegs, NTV2ReadWriteRegisterSingle aRegs[])
{
	NTV2ReadWriteMultiRegisterPayload *pRWMRP =
		(NTV2ReadWriteMultiRegisterPayload *)GetNubPktPayloadPtr(pPkt);

	*cookie = ntohl(pRWMRP->payloadHeader.handle);
	*numRegs = ntohl(pRWMRP->payloadHeader.numRegs);
	if (*numRegs > NTV2_NUB_NUM_MULTI_REGS)
	{
		NBWARN("numRegs=" << *numRegs << ", clipped to " << NTV2_NUB_NUM_MULTI_REGS);
		*numRegs = NTV2_NUB_NUM_MULTI_REGS;
	}

	for (ULWord i = 0; i < *numRegs; i++)
	{
		aRegs[i].registerNumber = ntohl(pRWMRP->aRegs[i].registerNumber);
		aRegs[i].registerMask = ntohl(pRWMRP->aRegs[i].registerMask);
		aRegs[i].registerShift = ntohl(pRWMRP->aRegs[i].registerShift);
	}
}

NTV2NubPkt * BuildNubOpenRespPacket (NTV2NubProtocolVersion otherSideProtocolVersion, ULWord boardNumber, LWord handle)
{
	char *p(AJA_NULL);
	NTV2NubProtocolVersion negotiatedProtocolVersion (NTV2_CPP_MIN(otherSideProtocolVersion, maxKnownProtocolVersion));
	NTV2NubPkt *pPkt(BuildNubBasePacket (negotiatedProtocolVersion,  eNubOpenRespPkt,  sizeof(NTV2BoardOpenInfo),  &p));
	if (!pPkt)
		return AJA_NULL;

	NTV2BoardOpenInfo *pBOI = (NTV2BoardOpenInfo *)p;
	pBOI->boardNumber = htonl(boardNumber);
	pBOI->boardType = 0;
	pBOI->handle = htonl(handle);
	return pPkt;
}

NTV2NubPkt *BuildNubReadSingleRegisterRespPacket(	NTV2NubProtocolVersion protocolVersion,
													LWord cookie, 
													ULWord registerNumber, 
													ULWord registerValue, 
													UWord result)
{
	NTV2NubPkt *pPkt;
	char *p;
	
	pPkt = BuildNubBasePacket(	protocolVersion,
								eNubReadRegisterSingleRespPkt,
								sizeof(NTV2ReadWriteRegisterPayload),
								&p);
	if (pPkt == 0)
		return 0;
	NTV2ReadWriteRegisterPayload *pReadRegisterPayload = (NTV2ReadWriteRegisterPayload *)p;

	// Convert to network byte order
	pReadRegisterPayload->handle = htonl(cookie);
	pReadRegisterPayload->registerNumber= htonl(registerNumber);
	pReadRegisterPayload->registerValue= htonl(registerValue);
	pReadRegisterPayload->result= htonl(result);

	return pPkt;
}

NTV2NubPkt *BuildNubReadMultiRegisterRespPacket( NTV2NubProtocolVersion protocolVersion,
												LWord cookie, 
												ULWord numRegs, 
												NTV2ReadWriteRegisterSingle aRegs[], 
												UWord result, 
												ULWord whichRegisterFailed)
{
	NTV2NubPkt *pPkt;
	char *p;
	// Payload size is length of response string plus board open info
	ULWord payloadSize = sizeof(NTV2ReadWriteMultiRegisterPayloadHeader) + numRegs * sizeof(NTV2ReadWriteRegisterSingle);

	if (numRegs > NTV2_NUB_NUM_MULTI_REGS)
	{
		printf("BuildNubReadMultiRegisterRespPacket(): numRegs was %d, clipped to %d\n", numRegs, NTV2_NUB_NUM_MULTI_REGS);
		numRegs = NTV2_NUB_NUM_MULTI_REGS;
	}
	
	pPkt = BuildNubBasePacket(	protocolVersion,
								eNubReadRegisterMultiRespPkt,
								payloadSize,
								&p);
	if (pPkt == 0)
		return AJA_NULL;

	NTV2ReadWriteMultiRegisterPayload *pReadRegisterMultiPayload = 
		(NTV2ReadWriteMultiRegisterPayload *)p;

	// Convert to network byte order
	pReadRegisterMultiPayload->payloadHeader.handle = htonl(cookie);
	pReadRegisterMultiPayload->payloadHeader.result = htonl(result);
	pReadRegisterMultiPayload->payloadHeader.numRegs = htonl(numRegs);
	pReadRegisterMultiPayload->payloadHeader.whichRegisterFailed = htonl(whichRegisterFailed);
	for (ULWord i = 0; i < numRegs; i++)
	{
		pReadRegisterMultiPayload->aRegs[i].registerNumber = htonl(aRegs[i].registerNumber);
		pReadRegisterMultiPayload->aRegs[i].registerValue = htonl(aRegs[i].registerValue);
	}

	return pPkt;
}

NTV2NubPkt * BuildNubWriteRegisterRespPacket (NTV2NubProtocolVersion protocolVersion,
											LWord cookie, 
											ULWord registerNumber, 
											ULWord registerValue, 
											UWord result)
{
	NTV2NubPkt *pPkt;
	char *p;
	
	pPkt = BuildNubBasePacket(	protocolVersion,
								eNubWriteRegisterRespPkt,
								sizeof(NTV2ReadWriteRegisterPayload),
								&p);
	if (pPkt == 0)
		return 0;

	NTV2ReadWriteRegisterPayload *pWriteRegisterPayload = 
		(NTV2ReadWriteRegisterPayload *)p;

	// Convert to network byte order
	pWriteRegisterPayload->handle = htonl(cookie);
	pWriteRegisterPayload->registerNumber = htonl(registerNumber);
	pWriteRegisterPayload->registerValue = htonl(registerValue);
	pWriteRegisterPayload->result = htonl(result);

	return pPkt;
}

NTV2NubPkt * BuildNubGetAutoCirculateRespPacket( NTV2NubProtocolVersion protocolVersion,
												LWord cookie, 
												ULWord channelSpec, 
												AUTOCIRCULATE_STATUS_STRUCT *acStatus, 
												UWord result)
{
	NTV2NubPkt *pPkt;
	char *p;
	
	pPkt = BuildNubBasePacket(	protocolVersion,
								eNubGetAutoCirculateRespPkt,
								sizeof(NTV2GetAutoCircPayload),
								&p);
	if (pPkt == 0)
		return 0;

	NTV2GetAutoCircPayload *pGetAutoCircPayload =
		(NTV2GetAutoCircPayload *)p;

	// Convert to network byte order
	pGetAutoCircPayload->handle = htonl(cookie);
	pGetAutoCircPayload->result = htonl(result);

	pGetAutoCircPayload->channelSpec = htonl(channelSpec);

	// Some 32 bit quantities
	pGetAutoCircPayload->channelSpec = htonl((ULWord)(acStatus->channelSpec));
	pGetAutoCircPayload->state = htonl((ULWord)(acStatus->state));
	pGetAutoCircPayload->startFrame = htonl((ULWord)(acStatus->startFrame));
	pGetAutoCircPayload->endFrame = htonl((ULWord)(acStatus->endFrame));
	pGetAutoCircPayload->activeFrame = htonl((ULWord)(acStatus->activeFrame));

	// Note: the following four items are 64-bit quantities!
	pGetAutoCircPayload->rdtscStartTime = htonll(acStatus->rdtscStartTime);
	pGetAutoCircPayload->audioClockStartTime = htonll(acStatus->audioClockStartTime);
	pGetAutoCircPayload->rdtscCurrentTime = htonll(acStatus->rdtscCurrentTime);
	pGetAutoCircPayload->audioClockCurrentTime = htonll(acStatus->audioClockCurrentTime);

	// Back to 32 bit quantities.
	pGetAutoCircPayload->framesProcessed = htonl((ULWord)(acStatus->framesProcessed));
	pGetAutoCircPayload->framesDropped = htonl((ULWord)(acStatus->framesDropped));
	pGetAutoCircPayload->bufferLevel = htonl((ULWord)(acStatus->bufferLevel));

	// These are bools, which natively on some systems (Linux) are 1 byte and others 4 (MacOSX)
	// So the portable structures makes them all ULWords.
	// They probably should have been bitfields... 
	
	pGetAutoCircPayload->bWithAudio = htonl((ULWord)(acStatus->bWithAudio));
	pGetAutoCircPayload->bWithRP188 = htonl((ULWord)(acStatus->bWithRP188));
	pGetAutoCircPayload->bFbfChange = htonl((ULWord)(acStatus->bFbfChange));
	pGetAutoCircPayload->bFboChange = htonl((ULWord)(acStatus->bFboChange));
	pGetAutoCircPayload->bWithColorCorrection = htonl((ULWord)(acStatus->bWithColorCorrection));
	pGetAutoCircPayload->bWithVidProc = htonl((ULWord)(acStatus->bWithVidProc));
	pGetAutoCircPayload->bWithCustomAncData = htonl((ULWord)(acStatus->bWithCustomAncData));
	
	return pPkt;
}

NTV2NubPkt * BuildNubControlAutoCirculateRespPacket (NTV2NubProtocolVersion protocolVersion,
													LWord cookie, 
													ULWord channelSpec, 
													UWord result)
{
	NTV2NubPkt *pPkt;
	char *p;
	
	pPkt = BuildNubBasePacket(	protocolVersion,
								eNubV2ControlAutoCirculateRespPkt,
								sizeof(NTV2ControlAutoCircPayload),
								&p);
	if (pPkt == 0)
		return 0;

	NTV2ControlAutoCircPayload *pControlAutoCircPayload =
		(NTV2ControlAutoCircPayload *)p;

	// Convert to network byte order
	pControlAutoCircPayload->handle = htonl(cookie);
	pControlAutoCircPayload->result = htonl(result);
	pControlAutoCircPayload->channelSpec = htonl(channelSpec);

	return pPkt;
}

NTV2NubPkt * BuildNubWaitForInterruptRespPacket (NTV2NubProtocolVersion protocolVersion,
												LWord cookie, 
												UWord result)
{
	NTV2NubPkt *pPkt;
	char *p;
	
	pPkt = BuildNubBasePacket(	protocolVersion,
								eNubWaitForInterruptRespPkt,
								sizeof(NTV2WaitForInterruptPayload),
								&p);
	if (pPkt == 0)
		return 0;

	NTV2WaitForInterruptPayload *pWaitForInterruptPayload =
		(NTV2WaitForInterruptPayload *)p;

	// Convert to network byte order
	pWaitForInterruptPayload->handle = htonl(cookie);
	pWaitForInterruptPayload->result = htonl(result);

	return pPkt;
}

NTV2NubPkt * BuildNubDriverGetBitFileInformationRespPacket (NTV2NubProtocolVersion protocolVersion,
															LWord cookie, 
															BITFILE_INFO_STRUCT &bitFileInfo, 
															NTV2BitFileType bitFileType,
															UWord result)
{
	NTV2NubPkt *pPkt;
	char *p;
	
	pPkt = BuildNubBasePacket(	protocolVersion,
								eNubDriverGetBitFileInformationRespPkt,
								sizeof(NTV2DriverGetBitFileInformationPayload),
								&p);
	if (pPkt == 0)
		return 0;

	NTV2DriverGetBitFileInformationPayload *pDriverGetBitFileInformationPayload =
		(NTV2DriverGetBitFileInformationPayload *)p;

	// Convert to network byte order
	pDriverGetBitFileInformationPayload->handle = htonl(cookie);
	pDriverGetBitFileInformationPayload->result = htonl(result);
	pDriverGetBitFileInformationPayload->bitFileType = htonl(bitFileType);

	pDriverGetBitFileInformationPayload->bitFileInfo.checksum = htonl(bitFileInfo.checksum);
	pDriverGetBitFileInformationPayload->bitFileInfo.structVersion = htonl(bitFileInfo.structVersion);
	pDriverGetBitFileInformationPayload->bitFileInfo.structSize = htonl(bitFileInfo.structSize);
	pDriverGetBitFileInformationPayload->bitFileInfo.numBytes = htonl(bitFileInfo.numBytes);

	// "strings" (may not be null-terminated)
	memcpy(pDriverGetBitFileInformationPayload->bitFileInfo.dateStr, bitFileInfo.dateStr, NTV2_BITFILE_DATETIME_STRINGLENGTH);
	memcpy(pDriverGetBitFileInformationPayload->bitFileInfo.timeStr, bitFileInfo.timeStr, NTV2_BITFILE_DATETIME_STRINGLENGTH);
	memcpy(pDriverGetBitFileInformationPayload->bitFileInfo.designNameStr, bitFileInfo.designNameStr, NTV2_BITFILE_DESIGNNAME_STRINGLENGTH);

	pDriverGetBitFileInformationPayload->bitFileInfo.bitFileType = htonl(bitFileInfo.bitFileType);
	pDriverGetBitFileInformationPayload->bitFileInfo.whichFPGA = (NTV2XilinxFPGA)htonl(bitFileInfo.whichFPGA);
	
	return pPkt;
}

NTV2NubPkt * BuildNubDownloadTestPatternRespPacket (NTV2NubProtocolVersion protocolVersion,
													LWord cookie, 
													UWord result)
{
	NTV2NubPkt *pPkt;
	char *p;
	
	pPkt = BuildNubBasePacket(	protocolVersion,
								eNubDownloadTestPatternRespPkt,
								sizeof(NTV2DownloadTestPatternPayload),
								&p);
	if (pPkt == 0)
		return 0;

	NTV2DownloadTestPatternPayload *pDownloadTestPatternPayload =
		(NTV2DownloadTestPatternPayload *)p;

	// Convert to network byte order
	pDownloadTestPatternPayload->handle = htonl(cookie);
	pDownloadTestPatternPayload->result = htonl(result);

	return pPkt;
}

NTV2NubPkt * BuildNubDriverGetBuildInfoRespPacket (NTV2NubProtocolVersion protocolVersion,
													LWord cookie, 
													UWord result, 
													BUILD_INFO_STRUCT *pBuildInfo)
{
	NTV2NubPkt *pPkt;
	char *p;
	
	pPkt = BuildNubBasePacket(	protocolVersion,
								eNubDriverGetBuildInformationRespPkt,
								sizeof(NTV2DriverGetBuildInformationPayload),
								&p);
	if (pPkt == 0)
		return 0;

	NTV2DriverGetBuildInformationPayload *pDriverGetBuildInformationPayload =
	 (NTV2DriverGetBuildInformationPayload *)p;

	// Convert to network byte order
	pDriverGetBuildInformationPayload->handle = htonl(cookie);
	pDriverGetBuildInformationPayload->result = htonl(result);
	pDriverGetBuildInformationPayload->buildInfo.structVersion = htonl(1);
	pDriverGetBuildInformationPayload->buildInfo.structSize = htonl(sizeof(BUILD_INFO_STRUCT));
	memcpy(pDriverGetBuildInformationPayload->buildInfo.buildStr, pBuildInfo->buildStr, NTV2_BUILD_STRINGLENGTH); 

	return pPkt;
}


// Listen and respond to NTV2 nub discovery queries

void * handleDiscoveryQueries (void *)
{
	int sockfd(0), numbytes(0);
	struct sockaddr_in my_addr;	// my address information
	struct sockaddr_in their_addr; // connector's address information
	socklen_t addr_len;
	char buf[sizeof(NTV2NubPkt)];
	::memset(buf, 0, sizeof(NTV2NubPkt));

	if ((sockfd = int(socket(AF_INET, SOCK_DGRAM, 0))) == -1) 
		{cerr << "'socket' failed: " << ::strerror(errno) << endl;  exit(1);}

    // lose the pesky "address already in use" error message
    int yes=1;        // for setsockopt() SO_REUSEADDR, below
	if (::setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char*>(&yes), sizeof(int)) == -1) 
		{cerr << "'setsockopt' failed: " << ::strerror(errno) << endl;  exit(1);}

	my_addr.sin_family = AF_INET;		 // host byte order
	my_addr.sin_port = htons(NTV2DISCOVERYPORT);	 // short, network byte order
	my_addr.sin_addr.s_addr = INADDR_ANY; // automatically fill with my IP
	memset(&(my_addr.sin_zero), '\0', 8); // zero the rest of the struct

	if (::bind(sockfd, reinterpret_cast<struct sockaddr*>(&my_addr),  sizeof(struct sockaddr)) == -1) 
		{cerr << "'bind' failed: " << ::strerror(errno) << endl;  exit(1);}

#if DEBUG_RESPONSE_LOG
	int        fileDesc;
	time_t     rawtime;
	struct tm* timeinfo;
	char       logBuffer[256];
	size_t     startPos;

	string     theTime;
	string     outputTime;

	if (-1 != (fileDesc = open("/var/log/ntv2nub.log", O_CREAT | O_TRUNC | O_RDWR)))
	{
		fprintf(stderr, "Opened the ntv2nub log\n");
	}
	else
	{
		fprintf(stderr, "Unable to open the ntv2nub log\n");
	}
#endif

	addr_len = sizeof(struct sockaddr);

	while (true)
	{
		if ((numbytes = recvfrom(sockfd, buf, sizeof(NTV2NubPkt), 0,
			(struct sockaddr *)&their_addr, &addr_len)) == -1) 
		{
			cerr << "'recvfrom' failed: " << ::strerror(errno) << endl;
			exit(1);
			// Note: the nub can exit here with this message:
			// "recvfrom: Resource temporarily unavailable"
			// Due to this Linux kernel bug:
			// http://bugzilla.kernel.org/show_bug.cgi?id=5498
			// In production environments the nub should be monitored 
			// and automatically restarted.
		}

		// Discover packet?
		if (deNBOifyNTV2NubPkt((NTV2NubPkt *)buf, numbytes) && isDiscoverQueryPacket((NTV2NubPkt *)buf))
		{
			char ipAddrStr[16]; // "255.255.255.255\0".  Will need to be increased for IPV6.
			strncpy(ipAddrStr, inet_ntoa(their_addr.sin_addr), 15);
			ipAddrStr[15] = 0;
			// printf("Got a discover query packet on socket %d", sockfd);
			// printf(" from %s\n",ipAddrStr);
			// dumpDiscoveryPacket((NTV2NubPkt *)buf);
			UWord boardMask = 0;
			ParseNubDiscoverQueryPacket((NTV2NubPkt *)buf, &boardMask);	//	We ignore boardMask now

			NTV2NubPkt *pPkt = BuildDiscoverRespPacket(((NTV2NubPkt *)buf)->hdr.protocolVersion);
			int len =  pPkt == 0 ? 0 : sizeof(NTV2NubPktHeader) + pPkt->hdr.dataLength;

			if(NBOifyNTV2NubPkt(pPkt)) 
			{
#if DEBUG_RESPONSE_LOG
				time(&rawtime);
				timeinfo = localtime(&rawtime);
				theTime  = asctime(timeinfo);

				if (string::npos != (startPos = theTime.find(" ")))
				{
					startPos++;
					outputTime = theTime.substr(startPos, theTime.length() - (6 + startPos));
				}
#endif
	
#if 0
				if (sendall(sockfd, (char *)pPkt, &len) == -1) 
				{
					perror("sendall");
				}
#endif
				// Send out several responses since it is UDP and they could be lost.
				// Discoverer send out a bunch of queries so we just send a few responses.
				// Discoverer will have to handle duplicates.
				//
				// Updated Comment: The new discoveryd module does _not_ send out numerous
				// discovery requests. One response should be sufficient since, on a LAN
				// UDP will not get lost unless the network is being overwhelmed, in which
				// case, sending out multiple response will not help, but only make matters
				// worse.
				for (int i = 0; i < N_QUERY_RESPONSES; i++)
				{
					if ((numbytes=sendto(sockfd, (char *)pPkt, len, 0,
						 (struct sockaddr *)&their_addr, sizeof(struct sockaddr))) == -1) 
					{
#if 0
						perror("sendto");
#endif
					}

#if DEBUG_RESPONSE_LOG
					////////////////////////////////////////////
					// Logging the receive and response activity
					////////////////////////////////////////////
					if (-1 != fileDesc)
					{
						sprintf(logBuffer, "%s: From %s\n",
							outputTime.c_str(),
							inet_ntoa(their_addr.sin_addr));

						write(fileDesc, logBuffer, strlen(logBuffer));
					}
#endif
				}
			}

			if (pPkt)
			{
				delete pPkt;
			}
		} // No, unknown type, dump it. 
        else 
		{
			printf("got dgram packet from %s\n",inet_ntoa(their_addr.sin_addr));
			printf("packet is %d bytes long\n",numbytes);
			buf[numbytes] = '\0';
			printf("packet contains \"%s\"\n",buf);
		}
	}	//	while loop
	close(sockfd);

#if DEBUG_RESPONSE_LOG
	if (-1 != fileDesc)
	{
		close(fileDesc);
	}
#endif
}	//	handleDiscoveryQueries thread function


#ifdef MSWindows
DWORD WINAPI handleDiscoveryQueriesThread(LPVOID lpParameter)
{
	(void) lpParameter;
	handleDiscoveryQueries(NULL);
	return 2;
}
#endif


openBoardEntry::openBoardEntry (const ULWord inCookie, const ULWord inBoardNum, const int inFD)
	:	fCookie		(inCookie),
		fBoardNum	(inBoardNum),
		fBoardType	(256),
		fpCard		(new CNTV2Card),
		fRefCount	(0)
{
	if (CNTV2DeviceScanner::GetDeviceAtIndex(inBoardNum, *fpCard))
		fFDs.push_back(inFD);
}

openBoardEntry::~openBoardEntry()
{
}

bool openBoardEntry::removeFD (const int inFD)
{
	for (vector<int>::iterator it(fFDs.begin());  it != fFDs.end();  ++it)
		if (*it == inFD)
		{
			fFDs.erase(it);
			return true;
		}
	return false;
}

typedef vector<openBoardEntry>				OpenBoardEntries;
typedef OpenBoardEntries::iterator			OpenBoardEntriesIter;
typedef OpenBoardEntries::const_iterator	OpenBoardEntriesCIter;

static OpenBoardEntries	sOpenBoardEntries;

static bool Append (const ULWord cookie, const ULWord boardNumber, const int fd)
{
	sOpenBoardEntries.push_back(openBoardEntry(cookie, boardNumber, fd));
	if (!sOpenBoardEntries.back().card()->IsOpen())
	{
		sOpenBoardEntries.pop_back();
		NBFAIL("Append: Failed to open new board " << boardNumber << ": cookie=" << cookie << " refCount=" << sOpenBoardEntries.back().refCount() << " fd=" << fd);
		return false;
	}
	NBINFO("Append: Opened new board " << boardNumber << ": cookie=" << cookie << " refCount=" << sOpenBoardEntries.back().refCount() << " fd=" << fd);
	return true;
}

// Returns cookie.
static LWord OpenBoard (const ULWord boardNumber, const int fd)
{
	static ULWord nextCookie = 1;

	// Search for already opened board
	for (OpenBoardEntriesIter it(sOpenBoardEntries.begin());  it != sOpenBoardEntries.end();  ++it)
	{
		if (boardNumber == it->boardNum())
		{
			// Full?
			if (it->refCount() == NTV2MAXOPENBOARDFDS)
			{
				NBFAIL("fileDescriptor array full: boardNum=" << boardNumber);
				// Spill over into a new openBoardEntry for the same card and another NTV2MAXOPENBOARDFDS filedescriptors.
				continue; // Continue seeking for a spillover board
				// If none found a new one will be allocated below.
			}
			// Still room
			it->FDs().push_back(fd);
			it->refCount() += 1;
			NBINFO("Updated already open board: cookie=" << it->cookie() << " refCount=" << it->refCount() << " fd=" << fd);
			return it->cookie();
		}
	}
	
	// Not found, open and cache board.
	if (!Append(nextCookie, boardNumber, fd))
		return INVALID_NUB_HANDLE;
	const LWord result = LWord(nextCookie);
	++nextCookie;
	return result;
}

// Returns CNTV2Card pointer corresponding to cookie.
static CNTV2Card * FindOpenBoard (ULWord cookie)
{
	// Search for opened board
	for (OpenBoardEntriesIter it(sOpenBoardEntries.begin());  it != sOpenBoardEntries.end();  ++it)
	{
		if (cookie == it->cookie())
			return it->card();
		// printf("%s %d\n", iter->str, iter->val);
	}
	return AJA_NULL;
}

// Takes file descriptor, decrements refcount and if refcount falls to zero
// closes board so driver can possibly be unloaded.
static bool CloseBoard (int fileDescriptor)
{
	// Search for opened board
	for (OpenBoardEntriesIter it(sOpenBoardEntries.begin());  it != sOpenBoardEntries.end();  ++it)
	{
		openBoardEntry & entry(*it);
		if (entry.removeFD(fileDescriptor))
		{
			if (entry.refCount() == 0)
				sOpenBoardEntries.erase(it);
			return true;	//	found
		}
	}	//	for each opened board
	return false;	// Not found
}	//	CloseBoard


/**
	@brief	Nub server implementation that uses rpclib as the transport.
**/
class NTV2LegacyNubServer : public NTV2RPCServerAPI
{
	public:
		explicit		NTV2LegacyNubServer (const NTV2ConfigParams & inParams)
							:	NTV2RPCServerAPI(inParams)
						{
							mPortNumber = ConfigParams().u16ValueForKey(kConnectParamPort, NTV2LEGACYNUBPORT);
						}
		virtual void	RunServer (void);	///< @brief	Principal server thread function (override)

	protected:
		uint16_t	mPortNumber;	///< @brief	My port number that I'm listening on
};	//	NTV2RPCLibServer


void NTV2LegacyNubServer::RunServer (void)
{
    fd_set master;					//	master file descriptor list
    fd_set read_fds;				//	temp file descriptor list for select()
    struct sockaddr_in myaddr;		//	server address
    struct sockaddr_in remoteaddr;	//	client address
    int fdmax;						//	maximum file descriptor number
    int listener;					//	listening socket descriptor
    int newfd;						//	newly accept()ed socket descriptor
    char buf[8192];					//	8K buffer for client data
    int nbytes;
    int yes(1);						//	for setsockopt() SO_REUSEADDR, below
    socklen_t addrlen;

	AJADebug::Open();
	cout << "AJA legacy nub running protocol " << maxKnownProtocolVersion << endl;
	NBINFO("AJA legacy nub started, protocol version " << maxKnownProtocolVersion);
#ifndef MSWindows
	bool stealthFlag = false;	// If true, don't even listen for discovery queries
#if defined(USING_DISCOVERY_THREAD)
	pthread_t discovery_thread;
	pthread_attr_t attr;
	int ptrc;
#endif	//	USING_DISCOVERY_THREAD

	struct sigaction sa;
	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (::sigaction(SIGCHLD, &sa, AJA_NULL) == -1)
		{NBFAIL("'sigaction' failed: " << ::strerror(errno));  exit(1);}

	//	Fork a child to respond to discovery datagrams.
	if (stealthFlag)
		NBFAIL("Stealth mode requested, not listening for discovery queries");
	else
	{
#if defined(USING_DISCOVERY_THREAD)
		pthread_attr_init(&attr);
    	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		ptrc = ::pthread_create(&discovery_thread, &attr, &handleDiscoveryQueries, AJA_NULL);
		if (ptrc)
			NBFAIL("Query-discovery thread creation failed: " << ptrc);
		else
			NBINFO("Created query-discovery thread: " << ptrc);
#else
	if (!stealthFlag)
		if (!fork())
		{
			// This is the child process
			handleDiscoveryQueries(NULL);
			exit(2);
		}
#endif // defined(USING_DISCOVERY_THREAD)
	}

#else	//	else ifndef MSWindows
	WSADATA wsaData;
	::WSAStartup(0x202,&wsaData);
	::CreateThread(NULL,  NULL, handleDiscoveryQueriesThread, NULL,   0,   NULL);
#endif	//	else ifndef MSWindows

    FD_ZERO(&master);    // clear the master and temp sets
    FD_ZERO(&read_fds);

	// get the listener
	if ((listener = int(::socket(AF_INET, SOCK_STREAM, 0))) == -1)
		{cerr << "'socket' failed: " << ::strerror(errno) << endl;  exit(1);}
	NBDBG("Socket " << listener << " created");

	// lose the pesky "address already in use" error message
	if (::setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char*>(&yes), sizeof(int)) == -1) 
		{NBFAIL("'setsockopt' failed: " << ::strerror(errno));  exit(1);}

	// bind
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = INADDR_ANY;
	myaddr.sin_port = htons(mPortNumber);
	::memset(&(myaddr.sin_zero), '\0', 8);
	if (::bind(listener, reinterpret_cast<struct sockaddr*>(&myaddr), sizeof(myaddr)) == -1) 
		{NBFAIL("'bind' failed: " << ::strerror(errno));  exit(1);}
	NBDBG("Socket " << listener << " bound to 'any' INET address on port " << mPortNumber);

	// listen
	if (::listen(listener, 10) == -1) 
		{NBFAIL("'listen' failed: " << ::strerror(errno));  exit(1);}
	NBDBG("Listening on socket " << listener);

    // add the listener to the master set
    FD_SET(listener, &master);

    // keep track of the biggest file descriptor
    fdmax = listener; // so far, it's this one

	//	Main loop
	while (true)
	{
		read_fds = master; // copy it
		if (::select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) 
			{NBFAIL("'select' failed: " << ::strerror(errno));  exit(1);}
		NBDBG("'select' " << (fdmax+1) << " OK");

        // run through the existing connections looking for data to read
        for (int ifd = 0;  ifd <= fdmax;  ifd++) 
		{
            if (!FD_ISSET(ifd, &read_fds))
				continue;
			// we got one!!
			if (ifd == listener) 
			{	// handle new connections
				addrlen = sizeof(remoteaddr);
				if ((newfd = int(accept(listener, (struct sockaddr *)&remoteaddr, &addrlen))) == -1) 
					NBFAIL("'accept' failed: " << ::strerror(errno));
				else
				{
					FD_SET(newfd, &master); // add to master set
					if (newfd > fdmax) 
						fdmax = newfd;    // keep track of the maximum
					cout << "New connection from " << inet_ntoa(remoteaddr.sin_addr) << " on socket " << newfd << endl;
					NBINFO("New connection from " << inet_ntoa(remoteaddr.sin_addr) << " on socket " << newfd);
				}
			} 
			else // handle data from a client
			{
				// TODO: if nbytes < pktsize, do multiple recvs 
				// to reassemble fragmented packets.
				// one buf, size of two maxpackets, per connection?
				if ((nbytes = recv(ifd, buf, sizeof(buf), 0)) <= 0) 
				{
					// got error or connection closed by client
					if (nbytes == 0) 
						NBFAIL("socket " << ifd << " hung up");	// connection closed
					else 
						NBFAIL("'recv' error: " << ::strerror(errno));
					// printf("Closing board on fd %d\n", i);
					CloseBoard(ifd);
					close(ifd); // bye!
					FD_CLR(ifd, &master); // remove from master set
				}
				else // we got some data from a client
				{
					// NTV2NubPacket?  (Except discovery packets handed in child on another port)
					if (deNBOifyNTV2NubPkt((NTV2NubPkt *)buf, nbytes))
					{
						NTV2NubPkt *recvdNubPkt = (NTV2NubPkt *)buf;
						NTV2NubProtocolVersion recvdProtoVer = recvdNubPkt->hdr.protocolVersion;
						NTV2NubPkt *pRespPkt = AJA_NULL;
						if (isNubOpenQueryPacket(recvdNubPkt))
						{
							ULWord boardNumber(0), cookie(0);
							ParseNubOpenQueryPacket(recvdNubPkt, boardNumber);
							// printf("Opening board on fd %d\n", i);
							cookie = OpenBoard(boardNumber, ifd);
							pRespPkt = BuildNubOpenRespPacket(	recvdProtoVer,  boardNumber,  cookie);
							// printf("Got open for boardNumber %d, boardType %d.  cookie = 0x%08x\n", boardNumber, boardType, cookie);
						}
						else if (isNubReadSingleRegisterQueryPacket(recvdNubPkt))
						{
							LWord cookie	(0);
							ULWord registerNumber(0), registerValue(0), registerMask(0), registerShift(0);
							ULWord result	(0);
	
							ParseNubReadRegisterSingleQueryPacket (recvdNubPkt, &cookie, &registerNumber, &registerMask, &registerShift);
							// 	printf("Got register read for cookie = 0x%08x, register number %d, registerMask 0x%X, registerShift %d\n", cookie, registerNumber, registerMask, registerShift);
	
							CNTV2Card * card =  FindOpenBoard(cookie);
							if (card)
								result = card->ReadRegister(registerNumber, registerValue, registerMask, registerShift);
							else // Card not found, return failure.
								cookie = INVALID_NUB_HANDLE;
							pRespPkt = BuildNubReadSingleRegisterRespPacket(recvdProtoVer, cookie, registerNumber, registerValue, result);
						}
						else if (isNubWriteRegisterQueryPacket(recvdNubPkt))
						{
							LWord cookie;
							ULWord registerNumber, registerValue, registerMask, registerShift;
							ULWord result = 0;
	
							ParseNubWriteRegisterQueryPacket (recvdNubPkt, &cookie, &registerNumber, &registerValue, &registerMask, &registerShift);
							// printf("Got register write for cookie = 0x%08x, register number %d, register value %d, registerMask 0x%X, registerShift %d\n", cookie, registerNumber, registerValue, registerMask, registerShift);
	
							CNTV2Card *card =  FindOpenBoard(cookie);
							if (card)
								result = card->WriteRegister(registerNumber, registerValue, registerMask, registerShift);
							else // Card not found, return failure.
								cookie = INVALID_NUB_HANDLE;
							pRespPkt = BuildNubWriteRegisterRespPacket(recvdProtoVer, cookie, registerNumber, registerValue, result);
						}
						else if (isNubGetAutocirculateQueryPacket(recvdNubPkt))
						{
							LWord cookie;
							NTV2Crosspoint		channelSpec;
							ULWord result = 0;
	
							ParseNubGetAutoCirculateQueryPacket (recvdNubPkt, &cookie, &channelSpec);
							// printf("Received autocirculate get for cookie = 0x%08x, channel spec %d\n", cookie, (ULWord)channelSpec);
	
							CNTV2Card *card =  FindOpenBoard(cookie);
							AUTOCIRCULATE_STATUS	acStatus;
							if (card)
								result = card->AutoCirculateGetStatus(::NTV2CrosspointToNTV2Channel(channelSpec), acStatus);
							else // Card not found, return failure.
								cookie = INVALID_NUB_HANDLE;
							AUTOCIRCULATE_STATUS_STRUCT oldStatus;
							memset(&oldStatus, 0, sizeof(oldStatus));
							acStatus.CopyTo(oldStatus);
							pRespPkt = BuildNubGetAutoCirculateRespPacket (recvdProtoVer, cookie, channelSpec, &oldStatus, result);
						}
						else if (isNubControlAutocirculateQueryPacket(recvdNubPkt))
						{
							LWord cookie;
							AUTO_CIRC_COMMAND eCommand;
							NTV2Crosspoint channelSpec;
							bool bPlayToPause = true;
							ULWord result = 0;
	
							ParseNubControlAutoCirculateQueryPacket (recvdNubPkt, &cookie, &eCommand, &channelSpec, &bPlayToPause);
							// printf("Received control autocirculate for cookie = 0x%08x, command %d, channel spec %d\n", cookie, (ULWord)eCommand, (ULWord)channelSpec);
	
							CNTV2Card *card =  FindOpenBoard(cookie);
							if (card)
							{
								const NTV2Channel chan (::NTV2CrosspointToNTV2Channel (channelSpec));
								switch (eCommand)
								{
									case eStartAutoCirc:		result = card->AutoCirculateStart(chan);					break;
									case eStopAutoCirc:			result = card->AutoCirculateStop(chan);						break;
									case eAbortAutoCirc:		result = card->AutoCirculateStop(chan, true);				break;
									case ePauseAutoCirc:		result = bPlayToPause	? card->AutoCirculateResume(chan)
																						: card->AutoCirculatePause(chan);	break;
									case eFlushAutoCirculate:	result = card->AutoCirculateFlush(chan);					break;
									default:					result = false;												break;	// Others not supported yet
								}
							}
							else // Card not found, return failure.
								cookie = INVALID_NUB_HANDLE;
							pRespPkt = BuildNubControlAutoCirculateRespPacket(recvdProtoVer, cookie, channelSpec, result);
						}
						else if (isNubWaitForInterruptQueryPacket(recvdNubPkt))
						{
							LWord cookie;
							INTERRUPT_ENUMS eInterrupt;
							ULWord timeOutMs;
							ULWord result = 0;

							ParseNubWaitForInterruptQueryPacket(recvdNubPkt, &cookie, &eInterrupt, &timeOutMs);
							// printf("Received wait for interrupt %d, timeout %d msec, for cookie = 0x%08x\n", eInterrupt, timeOutMs, cookie);
	
							CNTV2Card *card =  FindOpenBoard(cookie);
							if (card)
								result = card->WaitForInterrupt(eInterrupt, timeOutMs);
							else // Card not found, return failure.
								cookie = INVALID_NUB_HANDLE;
							pRespPkt = BuildNubWaitForInterruptRespPacket(recvdProtoVer, cookie, result);
						}
						else if (isNubDriverGetBitFileInformationQueryPacket(recvdNubPkt))
						{
							LWord cookie;
							NTV2BitFileType bitFileType;
							BITFILE_INFO_STRUCT bitFileInfo;
							NTV2XilinxFPGA whichFPGA;
							ULWord result = 0;
	
							memset(&bitFileInfo, 0, sizeof(BITFILE_INFO_STRUCT));
							ParseNubDriverGetBitFileInformationQueryPacket(recvdNubPkt, &cookie, &bitFileType, &whichFPGA);
							// printf("Received driver get bitfile info for type %d\n", bitFileType);
	
							CNTV2Card *card =  FindOpenBoard(cookie);
							if (card)
							{
								bitFileInfo.whichFPGA = whichFPGA;
								result = card->DriverGetBitFileInformation(bitFileInfo, bitFileType);
							}
							else // Card not found, return failure.
								cookie = INVALID_NUB_HANDLE;
							pRespPkt = BuildNubDriverGetBitFileInformationRespPacket(recvdProtoVer, cookie, bitFileInfo, bitFileType, result);
						}
						else if (isNubDownloadTestPatternQueryPacket(recvdNubPkt))
						{
							LWord cookie;
							ULWord result = 0;
							NTV2Channel channel;
							NTV2FrameBufferFormat testPatternFrameBufferFormat;
							UWord signalMask;
							bool testPatternDMAEnable;
							ULWord testPatternNumber;
	
							ParseNubDownloadTestPatternQueryPacket(recvdNubPkt, &cookie, &channel, &testPatternFrameBufferFormat, &signalMask, &testPatternDMAEnable, &testPatternNumber);
							// printf("Received download test pattern msg\n");
	
							CNTV2Card *card =  FindOpenBoard(cookie);
							if (card)
							{
								cerr << "Obsolete" << endl;
							}
							else // Card not found, return failure.
								cookie = INVALID_NUB_HANDLE;
							pRespPkt = BuildNubDownloadTestPatternRespPacket(recvdProtoVer, cookie, result);
						}
						else if (isNubReadMultiRegisterQueryPacket(recvdNubPkt))
						{
							LWord cookie;
							ULWord numRegs;
							ULWord result = 0;
							NTV2RegInfo aRegs[NTV2_NUB_NUM_MULTI_REGS];
	
							ParseNubReadRegisterMultiQueryPacket(recvdNubPkt, &cookie, &numRegs, aRegs);
							// printf("Got multi register read for cookie = 0x%08x, %d registers,\n", cookie, numRegs);
	
							CNTV2Card *card =  FindOpenBoard(cookie);
							ULWord whichRegisterFailed=999;
							if (card)
							{
#if defined(NTV2_DEPRECATE_16_0)
								NTV2RegisterReads regs;
								for (ULWord n(0);  n < numRegs;  n++)
									regs.push_back(aRegs[n]);
								result = card->ReadRegisters(regs);
								if (result)
									for (ULWord n(0);  n < numRegs;  n++)
										aRegs[n] = regs.at(n);
#else
								result = card->ReadRegisterMulti(numRegs, &whichRegisterFailed, aRegs);
#endif
								// printf("ReadRegister multi %s, whichRegisterFailed = %d\n", result ? "succeeded" : "failed", whichRegisterFailed); 
							}
							else // Card not found, return failure.
								cookie = INVALID_NUB_HANDLE;
							pRespPkt = BuildNubReadMultiRegisterRespPacket(recvdProtoVer, cookie, numRegs, aRegs, result, whichRegisterFailed);
						}
						else if (isNubDriverGetBuildInformationQueryPacket(recvdNubPkt))
						{
							LWord cookie;
							ULWord result = 0;
	
							BUILD_INFO_STRUCT buildInfo;
							ParseNubDriverGetBuildInformationQueryPacket(recvdNubPkt, &cookie, &buildInfo);
							// printf("Got driver get buildinfo for cookie = 0x%08x\n", cookie);
	
							CNTV2Card *card =  FindOpenBoard(cookie);
							if (card)
							{
								result = card->DriverGetBuildInformation(buildInfo);
								// printf("GetBuildInfo %s, buildInfo = %s\n", result ? "succeeded" : "failed", buildInfo.buildStr); 
							}
							else // Card not found, return failure.
								cookie = INVALID_NUB_HANDLE;
							pRespPkt = BuildNubDriverGetBuildInfoRespPacket(recvdProtoVer, cookie, result, &buildInfo);
						}
	
						// Send response
						int len (pRespPkt ? int(sizeof(NTV2NubPktHeader) + pRespPkt->hdr.dataLength) : 0);
						if (pRespPkt)
						{
							if (NBOifyNTV2NubPkt(pRespPkt)) 
							{
								if (::sendall(ifd, (char *)pRespPkt, &len) == -1) 
									NBFAIL("'sendall' failed: " << ::strerror(errno));
								else
									NBDBG("'sendall' " << len << " bytes on fd " << ifd);
							}
							delete pRespPkt;
						}
					}
					else
						for (int jfd(0);  jfd <= fdmax;  jfd++) // send to everyone!  They love us!
							if (FD_ISSET(jfd, &master)) 
								if (jfd != listener  &&  jfd != ifd)	//	except the listener and ourselves
								{
									if (::send(jfd, buf, nbytes, 0) == -1) 
										NBFAIL("'send' failed on fd " << jfd << ": " << ::strerror(errno));
									else
										NBDBG("'send' " << nbytes << " bytes on fd " << jfd);
								}
				}
			}	//	else handle data from a client
        }	//	for each existing connection (looking for data to read)
    }	//	loop forever
}	//	RunServer


extern "C"
{
	/**
		@brief	Instantiates a new server instance to talk to clients.
				-	pDLLHandle:	A pointer to the DLL/dylib/so handle.
				-	inParams: The NTV2ConfigParams that specify how to configure the server.
				-	inHostSDKVersion:	Specifies the NTV2 SDK version the caller was compiled with.
		@return	A pointer to the new server instance if successful, or nullptr (zero) upon failure.
	**/
	NTV2RPCServerAPI* CreateServer (void * pInDLLHandle, const NTV2ConfigParams & inParams, const uint32_t inHostSDKVersion)
	{	(void) pInDLLHandle; (void)inHostSDKVersion;
		return new NTV2LegacyNubServer(inParams);
	}
}
