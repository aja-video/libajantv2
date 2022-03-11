/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2nubtypes.h
	@brief		Declares data types and structures used in NTV2 "nub" packets.
	@copyright	(C) 2006-2022 AJA Video Systems, Inc.
**/

#ifndef __NTV2NUBTYPES_H
#define __NTV2NUBTYPES_H

#include "ntv2publicinterface.h"

#define NTV2DISCOVERYPORT	7777	// the port users will be connecting to

#define NTV2NUBPORT			7474	// port we're listening on

typedef ULWord NTV2NubProtocolVersion;

#if defined(NTV2_RPC_SUPPORT)
	#include "ntv2endian.h"

	#define	PUSHU16(__v__,__b__)	{	const uint16_t _u16(NTV2HostIsBigEndian ? (__v__) : NTV2EndianSwap16HtoB(__v__));	\
										const UByte * _pU16(reinterpret_cast<const UByte*>(&_u16));							\
										__b__.push_back(_pU16[0]); __b__.push_back(_pU16[1]);								\
									}

	#define	PUSHU32(__v__,__b__)	{	const uint32_t _u32(NTV2HostIsBigEndian ? (__v__) : NTV2EndianSwap32HtoB(__v__));	\
										const UByte * _pU32(reinterpret_cast<const UByte*>(&_u32));							\
										__b__.push_back(_pU32[0]); __b__.push_back(_pU32[1]);								\
										__b__.push_back(_pU32[2]); __b__.push_back(_pU32[3]);								\
									}

	#define	PUSHU64(__v__,__b__)	{	const uint64_t _u64(NTV2HostIsBigEndian ? (__v__) : NTV2EndianSwap64HtoB(__v__));	\
										const UByte * _pU64(reinterpret_cast<const UByte*>(&_u64));							\
										__b__.push_back(_pU64[0]); __b__.push_back(_pU64[1]);								\
										__b__.push_back(_pU64[2]); __b__.push_back(_pU64[3]);								\
										__b__.push_back(_pU64[4]); __b__.push_back(_pU64[5]);								\
										__b__.push_back(_pU64[6]); __b__.push_back(_pU64[7]);								\
									}

	#define	POPU16(__v__,__b__,__n__)	{	uint16_t _u16(0);	UByte * _pU8(reinterpret_cast<UByte*>(&_u16));				\
											_pU8[0] = (__b__).at((__n__)++); _pU8[1] = (__b__).at((__n__)++);				\
											(__v__) = NTV2HostIsBigEndian ? _u16 : NTV2EndianSwap16BtoH(_u16);				\
										}

	#define	POPU32(__v__,__b__,__n__)	{	uint32_t _u32(0);	UByte * _pU8(reinterpret_cast<UByte*>(&_u32));				\
											_pU8[0] = (__b__).at((__n__)++); _pU8[1] = (__b__).at((__n__)++);				\
											_pU8[2] = (__b__).at((__n__)++); _pU8[3] = (__b__).at((__n__)++);				\
											(__v__) = NTV2HostIsBigEndian ? _u32 : NTV2EndianSwap32BtoH(_u32);				\
										}

	#define	POPU64(__v__,__b__,__n__)	{	uint64_t _u64(0);	UByte * _pU8(reinterpret_cast<UByte*>(&_u64));				\
											_pU8[0] = (__b__).at((__n__)++); _pU8[1] = (__b__).at((__n__)++);				\
											_pU8[2] = (__b__).at((__n__)++); _pU8[3] = (__b__).at((__n__)++);				\
											_pU8[4] = (__b__).at((__n__)++); _pU8[5] = (__b__).at((__n__)++);				\
											_pU8[6] = (__b__).at((__n__)++); _pU8[7] = (__b__).at((__n__)++);				\
											(__v__) = NTV2HostIsBigEndian ? _u64 : NTV2EndianSwap64BtoH(_u64);				\
										}

#else	//	!defined(NTV2_RPC_SUPPORT)

#define INVALID_NUB_HANDLE (-1)

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
#endif	//	!defined(NTV2_RPC_SUPPORT)

#endif	//	__NTV2NUBTYPES_H
