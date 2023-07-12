/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2legacycommon.h
	@brief		Common defines & typedefs shared between NTV2RPCLibClient and NTV2RPCLibServer for legacy nub.
	@copyright	(C) 2021-2022 AJA Video Systems, Inc.
**/

#ifndef __NTV2LEGACYCOMMON_H
#define __NTV2LEGACYCOMMON_H

#include "ntv2publicinterface.h"
#ifdef MSWindows
	#include <WinSock2.h>
	typedef int socklen_t ;
#else
	#include <sys/socket.h>
#endif

#define NTV2DISCOVERYPORT	7777	// the port users will be connecting to

#define NTV2LEGACYNUBPORT	7474	// legacy port we listen on

#define INVALID_NUB_HANDLE (-1)

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


typedef ULWord NTV2NubProtocolVersion;

const ULWord ntv2NubProtocolVersionNone = 0;
const ULWord ntv2NubProtocolVersion1 = 1;
const ULWord ntv2NubProtocolVersion2 = 2;
const ULWord ntv2NubProtocolVersion3 = 3;	// Added get buildinfo
const ULWord maxKnownProtocolVersion = 3;

typedef enum
{
	eDiscoverQueryPkt						= 0,
	eDiscoverRespPkt						= 1,
	eNubOpenQueryPkt						= 2,
	eNubOpenRespPkt							= 3,
	eNubReadRegisterSingleQueryPkt			= 4,
	eNubReadRegisterSingleRespPkt			= 5,
	eNubWriteRegisterQueryPkt				= 6,
	eNubWriteRegisterRespPkt				= 7,
	eNubGetAutoCirculateQueryPkt			= 8,
	eNubGetAutoCirculateRespPkt				= 9,
	eNubV1ControlAutoCirculateQueryPkt		= 8,	// Dupe #, maintained for bkwd compat
	eNubV1ControlAutoCirculateRespPkt		= 9,	// Dupe #, maintained for bkwd compat
	eNubWaitForInterruptQueryPkt			= 10,
	eNubWaitForInterruptRespPkt				= 11,
	eNubDriverGetBitFileInformationQueryPkt = 12,
	eNubDriverGetBitFileInformationRespPkt	= 13,
	eNubDownloadTestPatternQueryPkt			= 14,
	eNubDownloadTestPatternRespPkt			= 15,
	eNubReadRegisterMultiQueryPkt			= 16,
	eNubReadRegisterMultiRespPkt			= 17,
	eNubGetDriverVersionQueryPkt			= 18,
	eNubGetDriverVersionRespPkt				= 19,
	eNubV2ControlAutoCirculateQueryPkt		= 20,	// Replaces eNubV1ControlAutoCirculateQueryPkt
	eNubV2ControlAutoCirculateRespPkt		= 21,	// Replaces eNubV1ControlAutoCirculateRespPkt
	eNubDriverGetBuildInformationQueryPkt	= 22,
	eNubDriverGetBuildInformationRespPkt	= 23,
	eNubDriverDmaTransferQueryPkt			= 24,
	eNubDriverDmaTransferRespPkt			= 25,
	eNubDriverMessageQueryPkt				= 26,
	eNubDriverMessageRespPkt				= 27,
	eNumNTV2NubPktTypes
} NTV2NubPktType;


typedef struct
{
	NTV2NubProtocolVersion	protocolVersion;
	NTV2NubPktType			pktType;
	ULWord					dataLength;						// Length of payload in bytes	
	ULWord					reserved[13];					// Future use
} NTV2NubPktHeader;

#define NTV2_NUBPKT_MAX_DATASIZE	8192	// ISO C++ forbids zero-size arrays

typedef struct
{
	NTV2NubPktHeader		hdr;
	unsigned char			data[NTV2_NUBPKT_MAX_DATASIZE]; // Variable-length payload
} NTV2NubPkt;

#define NTV2_DISCOVER_BOARDINFO_DESC_STRMAX 32

typedef struct
{
	ULWord	boardMask;	// One or more of NTV2DeviceType
} NTV2DiscoverQueryPayload;


typedef struct
{
	ULWord boardNumber;		// Card number, 0 .. 3
	ULWord boardType;		// e.g. BOARDTYPE_KHD
	ULWord boardID;			// From register 50
	char   description[NTV2_DISCOVER_BOARDINFO_DESC_STRMAX];	// "IPADDR: board identifier"
} NTV2DiscoverBoardInfo;


// Enough for 4 KSDs, 4 KHDs, 4 HDNTVs and 4 XENA2s.
// Which would imply a system with 16 PCI slots.
// Four would probably be enough... 
#define NTV2_NUB_MAXBOARDS_PER_HOST 16

typedef struct
{
	ULWord					numBoards;	// Number of entries in the table below
	NTV2DiscoverBoardInfo	discoverBoardInfo[NTV2_NUB_MAXBOARDS_PER_HOST];
} NTV2DiscoverRespPayload;

typedef struct
{
	ULWord	boardNumber;	// Card number, 0 .. 3
	ULWord	boardType;		// e.g. BOARDTYPE_KHD
	LWord	handle;			// A session cookie required for reg gets/sets and close
} NTV2BoardOpenInfo;

// Single read/writes
typedef struct
{
	LWord  handle;			// A session cookie required for reg gets/sets and close
	ULWord registerNumber; 
	ULWord registerValue;
	ULWord registerMask;
	ULWord registerShift;
	ULWord result;			// Actually a bool, returned from RegisterRead/RegisterWrite
} NTV2ReadWriteRegisterPayload;

// Multi reads.	 TODO: Support writes.

// This following number is enough for the watcher for OEM2K.
// If it is increased, increase the protocol version number.
// Be sure the number of registers fits into the maximum packet
// size.
#define NTV2_NUB_NUM_MULTI_REGS 200 
typedef struct
{
	LWord  handle;			// A session cookie required for reg gets/sets and close
	ULWord numRegs;			// In: number to read/write.  (Write not supported yet).
	ULWord result;			// Actually a bool, returned from RegisterRead
	ULWord whichRegisterFailed; // Only if result is false.	 Regs after that contain garbage. 
} NTV2ReadWriteMultiRegisterPayloadHeader;

typedef struct
{
	NTV2ReadWriteMultiRegisterPayloadHeader payloadHeader;
	NTV2ReadWriteRegisterSingle aRegs[NTV2_NUB_NUM_MULTI_REGS];
} NTV2ReadWriteMultiRegisterPayload;

typedef struct
{
	LWord  handle;			// A session cookie required for reg gets/sets and close
	ULWord result;			// Actually a bool, returned from RegisterRead/RegisterWrite
	ULWord eCommand;		// From AUTOCIRCULATE_DATA. 
	ULWord channelSpec;		
	ULWord state;
	ULWord startFrame;		// Acually LWORD
	ULWord endFrame;		// Acually LWORD
	ULWord activeFrame;		// Acually LWORD Current Frame# actually being output (or input), -1, if not active
	ULWord64				rdtscStartTime;			// Performance Counter at start
	ULWord64				audioClockStartTime;	// Register 28 with Wrap Logic
	ULWord64				rdtscCurrentTime;		// Performance Counter at time of call
	ULWord64				audioClockCurrentTime;	// Register 28 with Wrap Logic
	ULWord					framesProcessed;
	ULWord					framesDropped;
	ULWord					bufferLevel;	// how many buffers ready to record or playback

	ULWord					bWithAudio;
	ULWord					bWithRP188;
	ULWord					bFbfChange;
	ULWord					bFboChange ;
	ULWord					bWithColorCorrection;
	ULWord					bWithVidProc;		   
	ULWord					bWithCustomAncData;			 
} NTV2GetAutoCircPayload;

typedef struct
{
	ULWord	handle;			// A session cookie 
	ULWord result;			// Actually a bool
	ULWord eCommand;		// From AUTOCIRCULATE_DATA. 
	ULWord channelSpec;		

	ULWord lVal1;
	ULWord lVal2;
	ULWord lVal3;
	ULWord lVal4;
	ULWord lVal5;
	ULWord lVal6;

	ULWord bVal1;
	ULWord bVal2;
	ULWord bVal3;
	ULWord bVal4;
	ULWord bVal5;
	ULWord bVal6;
	ULWord bVal7;
	ULWord bVal8;

	// Can't send pointers over network so pvVal1 etc do not appear here.

} NTV2ControlAutoCircPayload; 

typedef struct
{
	LWord  handle;			// A session cookie required for reg gets/sets and close
	ULWord result;			// Actually a bool
	ULWord eInterrupt;
	ULWord timeOutMs;
} NTV2WaitForInterruptPayload;

typedef struct
{
	LWord  handle;						// A session cookie required for reg gets/sets and close
	ULWord result;						// Actually a bool
	ULWord bitFileType;					// Actually enum: NTV2K2BitFileType
	BITFILE_INFO_STRUCT bitFileInfo;	// Joy: a portable proprietary type
} NTV2DriverGetBitFileInformationPayload;

typedef struct
{
	LWord  handle;						// A session cookie required for reg gets/sets and close
	ULWord result;						// Actually a bool
	BUILD_INFO_STRUCT buildInfo;		// Another portable proprietary type
} NTV2DriverGetBuildInformationPayload;


typedef struct
{
	LWord  handle;						// A session cookie required for reg gets/sets and close
	ULWord result;						// Actually a bool
	ULWord channel;						// Actually an enum: NTV2Channel 
	ULWord testPatternFrameBufferFormat;// Actually an enum: NTV2FrameBufferFormat 
	ULWord signalMask;
	ULWord testPatternDMAEnable;		// Actually a bool
	ULWord testPatternNumber;
} NTV2DownloadTestPatternPayload;




extern AJAExport const char *NTV2NubQueryRespStrProtVer1[eNumNTV2NubPktTypes];
extern AJAExport const char *NTV2NubQueryRespStrProtVer2[eNumNTV2NubPktTypes];

#if !defined (AJAMac) || !defined (ntohll)
	unsigned long long ntohll(unsigned long long n);
#endif
#if !defined (AJAMac) || !defined (htonll)
	unsigned long long htonll(unsigned long long n);
#endif

const AJAExport char * nubQueryRespStr (NTV2NubProtocolVersion	protocolVersion,
										NTV2NubPktType			pktType);

AJAExport void * GetNubPktPayloadPtr (NTV2NubPkt * pPkt);

AJAExport NTV2NubPkt * BuildNubBasePacket (NTV2NubProtocolVersion	protocolVersion,
											NTV2NubPktType			pktType,
											ULWord					payloadSize,
											char **					pPayload);

AJAExport bool NBOifyNTV2NubPkt (NTV2NubPkt * pPkt);
AJAExport bool deNBOifyNTV2NubPkt (NTV2NubPkt * pPkt, ULWord size);
AJAExport bool isNTV2NubPacketType (NTV2NubPkt * pPkt, NTV2NubPktType nubPktType);
AJAExport int sendall (AJASocket s, char * buf, int * len);
AJAExport int recvtimeout_sec (AJASocket s, char * buf, int len, int timeout_seconds);
AJAExport int recvtimeout_usec (AJASocket s, char * buf, int len, int timeout_uSecs);

#define RVCFROMTIMEOUT_ERR		(-1)
#define RVCFROMTIMEOUT_TIMEDOUT	(-2)

AJAExport int recvfromtimeout (AJASocket s, char * buf, int len, int timeout,
								struct sockaddr * their_addr, socklen_t * addr_len);

// Debug functions
AJAExport void dumpDiscoveryPacket (NTV2NubPkt * pPkt, NTV2DiscoverRespPayload * boardInventory = AJA_NULL);
AJAExport void dumpBoardInventory (NTV2DiscoverRespPayload * boardInventory);

#endif //	__NTV2LEGACYCOMMON_H
