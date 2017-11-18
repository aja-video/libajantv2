/**
	@file		ancillarydata.cpp
	@brief		Implementation of the AJAAncillaryData class.
	@copyright	(C) 2010-2017 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#include "ntv2publicinterface.h"
#include "ancillarydata.h"
#include "ajabase/system/debug.h"				//	This makes 'ajaanc' dependent upon 'ajabase'
#include "ajacc/includes/ntv2smpteancdata.h"	//	This makes 'ajaanc' dependent upon 'ajacc':
												//	CNTV2SMPTEAncData::AddEvenParity
#if defined(AJA_LINUX)
	#include <string.h>				// For memcpy
	#include <stdlib.h>				// For realloc
#endif
#include <ios>

using namespace std;

#define	LOGMYERROR(__x__)	AJA_sREPORT(AJA_DebugUnit_AJAAncData, AJA_DebugSeverity_Error,		__FUNCTION__ << ":  " << __x__)
#define	LOGMYWARN(__x__)	AJA_sREPORT(AJA_DebugUnit_AJAAncData, AJA_DebugSeverity_Warning,	__FUNCTION__ << ":  " << __x__)
#define	LOGMYNOTE(__x__)	AJA_sREPORT(AJA_DebugUnit_AJAAncData, AJA_DebugSeverity_Notice,		__FUNCTION__ << ":  " << __x__)
#define	LOGMYINFO(__x__)	AJA_sREPORT(AJA_DebugUnit_AJAAncData, AJA_DebugSeverity_Info,		__FUNCTION__ << ":  " << __x__)
#define	LOGMYDEBUG(__x__)	AJA_sREPORT(AJA_DebugUnit_AJAAncData, AJA_DebugSeverity_Debug,		__FUNCTION__ << ":  " << __x__)


const uint32_t AJAAncillaryDataWrapperSize = 7;		// 3 bytes header + DID + SID + DC + Checksum: i.e. everything EXCEPT the payload

//const uint8_t  AJAAncillaryDataAnalogDID = 0x00;		// used in header DID field when ancillary data is "analog"
//const uint8_t  AJAAncillaryDataAnalogSID = 0x00;		// used in header SID field when ancillary data is "analog"



AJAAncillaryData::AJAAncillaryData()
{
	Init();
}


AJAAncillaryData::AJAAncillaryData (const AJAAncillaryData & inClone)
{
	Init();
	*this = inClone;
}


AJAAncillaryData::AJAAncillaryData (const AJAAncillaryData * pClone)
{
	Init();
	if (pClone)
		*this = *pClone;
}


AJAAncillaryData::~AJAAncillaryData ()
{
	FreeDataMemory();
}


void AJAAncillaryData::Init()
{
	FreeDataMemory();	// reset all internal data to defaults

	m_DID = 0x00;
	m_SID = 0x00;

	m_checksum = 0;

	m_coding = AJAAncillaryDataCoding_Digital;

	m_location.SetDataLink(AJAAncillaryDataLink_A);
	m_location.SetDataStream(AJAAncillaryDataStream_1);
	m_location.SetDataChannel(AJAAncillaryDataChannel_Y);
	m_location.SetDataSpace(AJAAncillaryDataSpace_VANC);
	m_location.SetLineNumber(0);
	m_location.SetHorizontalOffset(AJAAncillaryDataLocation::AJAAncDataHorizOffset_Default);

	m_ancType	   = AJAAncillaryDataType_Unknown;
	m_rcvDataValid = false;
}


void AJAAncillaryData::Clear (void)
{
	Init();
}


AJAAncillaryData * AJAAncillaryData::Clone (void) const
{
	return new AJAAncillaryData (this);
}


AJAStatus AJAAncillaryData::AllocDataMemory(uint32_t numBytes)
{
	AJAStatus status;
	FreeDataMemory();
	try
	{
		m_payload.reserve(numBytes);
		for (uint32_t ndx(0);  ndx < numBytes;  ndx++)
			m_payload.push_back(0);
assert(m_payload.size() == numBytes);
		status = AJA_STATUS_SUCCESS;
	}
	catch(bad_alloc)
	{
		m_payload.clear();
		status = AJA_STATUS_FAIL;
	}
	return status;
}


AJAStatus AJAAncillaryData::FreeDataMemory (void)
{
	m_payload.clear();
	return AJA_STATUS_SUCCESS;
}


AJAStatus AJAAncillaryData::SetDID (const uint8_t inDID)
{
	m_DID = inDID;
	return AJA_STATUS_SUCCESS;
}


AJAStatus AJAAncillaryData::SetSID (const uint8_t inSID)
{
	m_SID = inSID;
	return AJA_STATUS_SUCCESS;
}


uint8_t AJAAncillaryData::Calculate8BitChecksum (void) const
{
	//	NOTE:	This is NOT the "real" 9-bit checksum used in SMPTE 299 Ancillary packets,
	//			but it's calculated the same way and should be the same as the LS 8-bits
	//			of the 9-bit checksum...
	uint8_t	sum	(m_DID);
	sum += m_SID;
	sum += m_payload.size();
	if (!m_payload.empty())
		for (ByteVector::size_type ndx(0);  ndx < m_payload.size();  ndx++)
			sum += m_payload[ndx];
	return sum;
}


//**********
// Set anc data location parameters
//
AJAStatus AJAAncillaryData::SetDataLocation (const AJAAncillaryDataLocation & loc)
{
	AJAStatus	status	(SetLocationVideoLink(loc.GetDataLink()));
	if (AJA_SUCCESS(status))
		status = SetLocationDataStream(loc.GetDataStream());
	if (AJA_SUCCESS(status))
		status = SetLocationDataChannel(loc.GetDataChannel());
	if (AJA_SUCCESS(status))
		status = SetLocationVideoSpace(loc.GetDataSpace());
	if (AJA_SUCCESS(status))
		status = SetLocationLineNumber(loc.GetLineNumber());
	return status;
}


//	DEPRECATED
AJAStatus AJAAncillaryData::GetDataLocation (AJAAncillaryDataLink &			outLink,
											AJAAncillaryDataVideoStream &	outStream,
											AJAAncillaryDataSpace &			outAncSpace,
											uint16_t &						outLineNum)
{
	outLink		= m_location.GetDataLink();
	outStream	= m_location.GetDataChannel();
	outAncSpace	= m_location.GetDataSpace();
	outLineNum	= m_location.GetLineNumber();
	return AJA_STATUS_SUCCESS;
}

//-------------
//
AJAStatus AJAAncillaryData::SetDataLocation (const AJAAncillaryDataLink inLink, const AJAAncillaryDataChannel inChannel, const AJAAncillaryDataSpace inAncSpace, const uint16_t inLineNum, const AJAAncillaryDataStream inStream)
{
	AJAStatus	status	(SetLocationVideoLink(inLink));
	if (AJA_SUCCESS(status))
		status = SetLocationDataStream(inStream);
	if (AJA_SUCCESS(status))
		status = SetLocationDataChannel(inChannel);
	if (AJA_SUCCESS(status))
		status = SetLocationVideoSpace(inAncSpace);
	if (AJA_SUCCESS(status))
		status = SetLocationLineNumber(inLineNum);
	return status;
}


//-------------
//
AJAStatus AJAAncillaryData::SetLocationVideoLink (const AJAAncillaryDataLink inLinkValue)
{
	if (!IS_VALID_AJAAncillaryDataLink(inLinkValue))
		return AJA_STATUS_RANGE;

	m_location.SetDataLink(inLinkValue);
	return AJA_STATUS_SUCCESS;
}


//-------------
//
AJAStatus AJAAncillaryData::SetLocationDataStream (const AJAAncillaryDataStream inStream)
{
	if (!IS_VALID_AJAAncillaryDataStream(inStream))
		return AJA_STATUS_RANGE;

	m_location.SetDataStream(inStream);
	return AJA_STATUS_SUCCESS;
}


//-------------
//
AJAStatus AJAAncillaryData::SetLocationDataChannel (const AJAAncillaryDataChannel inChannel)
{
	if (!IS_VALID_AJAAncillaryDataChannel(inChannel))
		return AJA_STATUS_RANGE;

	m_location.SetDataChannel(inChannel);
	return AJA_STATUS_SUCCESS;
}


//-------------
//
AJAStatus AJAAncillaryData::SetLocationVideoSpace (const AJAAncillaryDataSpace inSpace)
{
	if (!IS_VALID_AJAAncillaryDataSpace(inSpace))
		return AJA_STATUS_RANGE;

	m_location.SetDataSpace(inSpace);
	return AJA_STATUS_SUCCESS;
}


//-------------
//
AJAStatus AJAAncillaryData::SetLocationLineNumber (const uint16_t inLineNum)
{
	//	No range checking here because we don't know how big the frame is
	m_location.SetLineNumber(inLineNum);
	return AJA_STATUS_SUCCESS;
}


//-------------
//
AJAStatus AJAAncillaryData::SetLocationHorizOffset (const uint16_t inOffset)
{
	//	No range checking here because we don't know how wide the frame is
	m_location.SetHorizontalOffset(inOffset);
	return AJA_STATUS_SUCCESS;
}

//-------------
//
AJAStatus AJAAncillaryData::SetDataCoding (const AJAAncillaryDataCoding inCodingType)
{
	if (m_coding != AJAAncillaryDataCoding_Digital  &&  m_coding != AJAAncillaryDataCoding_Raw)
		return AJA_STATUS_RANGE;

	m_coding = inCodingType;
	return AJA_STATUS_SUCCESS;
}


//**********
// Copy payload data from external source to internal storage. Erases current payload memory
// (if any) and allocates new memory

AJAStatus AJAAncillaryData::SetPayloadData (const uint8_t * pInData, const uint32_t inNumBytes)
{
	if (pInData == NULL || inNumBytes == 0)
		return AJA_STATUS_NULL;

	//	[Re]Allocate...
	AJAStatus status (AllocDataMemory(inNumBytes));
	if (AJA_FAILURE(status))
		return status;

	//	Copy payload into memory...
	::memcpy (&m_payload[0], pInData, inNumBytes);
	return AJA_STATUS_SUCCESS;
}


AJAStatus AJAAncillaryData::SetFromSMPTE334 (const uint16_t * pInData, const uint32_t inNumWords, const AJAAncillaryDataLocation & inLocInfo)
{
	if (!pInData)
		return AJA_STATUS_NULL;
	if (inNumWords < 7)
		return AJA_STATUS_RANGE;

	const uint32_t	payloadByteCount	(uint32_t(pInData[5] & 0x00FF));
	if ((inNumWords - 7) > payloadByteCount)
		return AJA_STATUS_RANGE;

	//	[Re]Allocate...
	const AJAStatus status	(AllocDataMemory(payloadByteCount));
	if (AJA_FAILURE(status))
		return status;

	//	Copy payload into new memory...
	for (uint32_t numWord (0);  numWord < payloadByteCount;  numWord++)
		m_payload[numWord] = UByte(pInData[6+numWord] & 0x00FF);

	SetDataCoding(AJAAncillaryDataCoding_Digital);
	SetDataLocation(inLocInfo);
	SetChecksum(UByte(pInData[6+payloadByteCount] & 0x00FF));
	SetDID(UByte(pInData[3] & 0x00FF));
	SetSID(UByte(pInData[4] & 0x00FF));

	return AJA_STATUS_SUCCESS;
}


//**********
// Append payload data from external source to existing internal storage. Realloc's current
// payload memory (if any) or allocates new memory

AJAStatus AJAAncillaryData::AppendPayloadData (const uint8_t * pInData, const uint32_t inNumBytes)
{
	if (pInData == NULL || inNumBytes == 0)
		return AJA_STATUS_NULL;

	try
	{
		for (uint32_t ndx(0);  ndx < inNumBytes;  ndx++)
			m_payload.push_back(pInData[ndx]);
	}
	catch(bad_alloc)
	{
		return AJA_STATUS_MEMORY;
	}
	
	return AJA_STATUS_SUCCESS;
}


//**********
// Append payload data from an external AJAAncillaryData object to existing internal storage.

AJAStatus AJAAncillaryData::AppendPayload (const AJAAncillaryData & inAnc)
{
	try
	{
		const uint8_t *	pInData		(inAnc.GetPayloadData());
		const uint32_t	numBytes	(uint32_t(inAnc.GetPayloadByteCount()));
		for (uint32_t ndx(0);  ndx < numBytes;  ndx++)
			m_payload.push_back(pInData[ndx]);
	}
	catch(bad_alloc)
	{
		return AJA_STATUS_MEMORY;
	}

	return AJA_STATUS_SUCCESS;
}


//**********
// Copy payload data from internal storage to external destination.

AJAStatus AJAAncillaryData::GetPayloadData (uint8_t * pOutData, const uint32_t inNumBytes) const
{
	if (pOutData == NULL)
		return AJA_STATUS_NULL;

	if (ByteVectorIndex(inNumBytes) > m_payload.size())
		return AJA_STATUS_RANGE;

	::memcpy (pOutData, GetPayloadData(), inNumBytes);
	return AJA_STATUS_SUCCESS;
}


AJAStatus AJAAncillaryData::GetPayloadData (vector<uint16_t> & outUDWs, const bool inAddParity) const
{
	AJAStatus	status	(AJA_STATUS_SUCCESS);
	const vector<uint16_t>::size_type	origSize	(outUDWs.size());
	for (ByteVectorConstIter iter(m_payload.begin());  iter != m_payload.end()  &&  AJA_SUCCESS(status);  ++iter)
	{
		const uint16_t	UDW	(inAddParity ? CNTV2SMPTEAncData::AddEvenParity(*iter) : *iter);
		try
		{
			outUDWs.push_back(UDW);	//	Copy 8-bit data into LS 8 bits, add even parity to bit 8, and ~bit 8 to bit 9
		}
		catch(...)
		{
			status = AJA_STATUS_MEMORY;
		}
	}	//	for each packet byte
	if (AJA_FAILURE(status))
		outUDWs.resize(origSize);
	return status;
}


uint8_t AJAAncillaryData::GetPayloadByteAtIndex (const uint32_t inIndex0) const
{
	if (ByteVectorIndex(inIndex0) < m_payload.size())
		return m_payload[inIndex0];
	else
		return 0;
}


AJAStatus AJAAncillaryData::SetPayloadByteAtIndex (const uint8_t inDataByte, const uint32_t inIndex0)
{
	if (inIndex0 >= GetDC())
		return AJA_STATUS_RANGE;

	m_payload[inIndex0] = inDataByte;
	return AJA_STATUS_SUCCESS;
}


AJAStatus AJAAncillaryData::ParsePayloadData (void)
{
	// should be overridden by derived classes to parse payload data to local data
	m_rcvDataValid = false;

	return AJA_STATUS_SUCCESS;
}


//**********
// Initializes the AJAAncillaryData object from "raw" ancillary data received from hardware (ingest)
AJAStatus AJAAncillaryData::InitWithReceivedData (const uint8_t *					pInData,
													const uint32_t					inMaxBytes,
													const AJAAncillaryDataLocation & inLocationInfo,
													uint32_t &						outPacketByteCount)
{
	AJAStatus status = AJA_STATUS_SUCCESS;

	// if all is well, pInData points to the beginning of a "GUMP" packet:
	//
	//	pInData ->	0:	0xFF			// 1st byte is always FF
	//				1:  Hdr data1		// location data byte #1
	//				2:  Hdr data2		// location data byte #2
	//				3:  DID				// ancillary packet Data ID
	//				4:  SID				// ancillary packet Secondary ID (or DBN)
	//				5:  DC				// ancillary packet Data Count (size of payload: 0 - 255)
	//				6:  Payload[0]		// 1st byte of payload UDW
	//				7:  Payload[1]		// 2nd byte of payload UDW
	//             ...    ...
	//		 (5 + DC):  Payload[DC-1]	// last byte of payload UDW
	//		 (6 + DC):	CS (checksum)	// 8-bit sum of (DID + SID + DC + Payload[0] + ... + Payload[DC-1])
	//
	//		 (7 + DC):  (start of next packet, if any...) returned in packetSize.
	//
	// Note that this is the layout of the data as returned from the ANCExtractor hardware, and
	// is NOT exactly the same as SMPTE-291.
	//
	// The inMaxBytes input gives us an indication of how many "valid" bytes remain in the caller's TOTAL
	// ANC Data buffer. We use this as a sanity check to make sure we don't try to parse past the end
	// of the captured data.
	//
	// The caller provides an AJAAncillaryDataLocation struct with all of the information filled in
	// except the line number.
	//
	// When we have extracted the useful data from the packet, we return the packet size, in bytes, so the
	// caller can find the start of the next packet (if any).

	if (pInData == NULL)
	{
		outPacketByteCount = 0;
		return AJA_STATUS_NULL;
	}

	//	The minimum size for a packet (i.e. no payload) is 7 bytes
	if (inMaxBytes < AJAAncillaryDataWrapperSize)
	{
		outPacketByteCount = inMaxBytes;
		return AJA_STATUS_RANGE;
	}

	//	The first byte should be 0xFF. If it's not, then the Anc data stream may be broken...
	if (pInData[0] != 0xFF)
	{
		//	Let the caller try to resynchronize...
		outPacketByteCount = 0;
		return AJA_STATUS_BAD_PARAM;
	}

	//	So we have at least enough bytes for a minimum packet, and the first byte is what we expect.
	//	Let's see what size this packet actually reports...
	uint32_t totalSize = pInData[5] + AJAAncillaryDataWrapperSize;

	//	If the reported packet size extends beyond the end of the buffer, we're toast...
	if (totalSize > inMaxBytes)
	{
		outPacketByteCount = inMaxBytes;
		return AJA_STATUS_RANGE;
	}

	//	OK... we have enough data in the buffer to contain the packet, and everything else checks out,
	//	so go ahead and parse the data...

	m_DID	   = pInData[3];				// SMPTE-291 Data ID
	m_SID	   = pInData[4];				// SMPTE-291 Secondary ID (or DBN)
//	DC		   = pInData[5];
	m_checksum = pInData[totalSize-1];	// reported checksum

	//	Caller provides all of the "location" information as a default. If the packet header info is "real", overwrite it...
	m_location = inLocationInfo;

	if ((pInData[1] & 0x80) != 0)
	{
		m_coding            = ((pInData[1] & 0x40) == 0) ? AJAAncillaryDataCoding_Digital : AJAAncillaryDataCoding_Analog;	// byte 1, bit 6
		m_location.SetDataStream(AJAAncillaryDataStream_1);	//	???	GUMP doesn't tell us the data stream it came from
		m_location.SetDataChannel(((pInData[1] & 0x20) == 0) ? AJAAncillaryDataChannel_C : AJAAncillaryDataChannel_Y);		// byte 1, bit 5
		m_location.SetDataSpace(((pInData[1] & 0x10) == 0) ? AJAAncillaryDataSpace_VANC : AJAAncillaryDataSpace_HANC);		// byte 1, bit 4
		m_location.SetLineNumber(((pInData[1] & 0x0F) << 7) + (pInData[2] & 0x7F));											// byte 1, bits 3:0 + byte 2, bits 6:0
		//m_location.SetHorizontalOffset(hOffset);	//	??? GUMP doesn't tell us the horiz offset of where the packet started in the raster line
	}

	//	Allocate space for the payload and copy it in...
	uint32_t payloadSize = pInData[5];	// SMPTE-291 Data Count
	if (payloadSize)
	{
		status = AllocDataMemory(payloadSize);					// note: this also sets our local "DC" value
		if (AJA_SUCCESS(status))
			for (uint32_t ndx(0);  ndx < payloadSize;  ndx++)
				m_payload[ndx] = pInData[ndx+6];
	}

	outPacketByteCount = totalSize;

	return status;
}


AJAStatus AJAAncillaryData::InitWithReceivedData (const ByteVector & inData, const AJAAncillaryDataLocation & inLocationInfo)
{
	uint32_t	pktByteCount(0);
	if (inData.empty())
		return AJA_STATUS_NULL;
	return InitWithReceivedData (&inData[0], uint32_t(inData.size()), inLocationInfo, pktByteCount);
}


//**********
// This returns the number of bytes that will be returned by GenerateTransmitData(). This is usually
// called first so the caller can allocate a buffer large enough to hold the results.

AJAStatus AJAAncillaryData::GetRawPacketSize (uint32_t & outPacketSize) const
{
	AJAStatus status = AJA_STATUS_SUCCESS;

	// if this is digital ancillary data (i.e. SMPTE-291 based), then the total size will be
	// seven bytes (3 bytes header + DID + SID + DC + Checksum) plus the payload size.
	if (m_coding == AJAAncillaryDataCoding_Digital)
	{
		if (GetDC() <= 255)
			outPacketSize = GetDC() + AJAAncillaryDataWrapperSize;

		else		// this is an error condition: if we have more than 255 bytes of payload in a "digital" packet, truncate the payload
			outPacketSize = 255 + AJAAncillaryDataWrapperSize;
	}
	else if (m_coding == AJAAncillaryDataCoding_Raw)
	{
		// if this is analog/raw ancillary data then we need to figure out how many "packets" we need
		// to generate in order to pass all of the payload data (max 255 bytes per packet)
		// special case: if DC is zero, there's no reason to generate ANY output!
		if (IsEmpty())
			outPacketSize = 0;
		else	// DC > 0
		{
			uint32_t numPackets = (GetDC() + 254) / 255;

			// all packets will have a 255-byte payload except the last one
			uint32_t lastPacketDC = GetDC() % 255;

			// each packet has a 7-byte "wrapper" + the payload size
			outPacketSize =  ((numPackets - 1) * (255 + AJAAncillaryDataWrapperSize))	// all packets except the last one have a payload of 255 bytes
							+ (lastPacketDC + AJAAncillaryDataWrapperSize);				// the last packet carries the remainder
		}
	}
	else	// huh? (coding not set)
	{
		outPacketSize = 0;
		status = AJA_STATUS_FAIL;
	}
	
	return status;
}


//**********
// Generates "raw" ancillary data from the internal ancillary data (playback)

AJAStatus AJAAncillaryData::GenerateTransmitData (uint8_t * pData, const uint32_t inMaxBytes, uint32_t & outPacketSize) const
{
	AJAStatus status = AJA_STATUS_SUCCESS;

	// make sure the caller has allocated enough space to hold what we're going to generate
	uint32_t myPacketSize;
	GetRawPacketSize(myPacketSize);

	if (myPacketSize > inMaxBytes  ||  myPacketSize == 0)
	{
		outPacketSize = 0;		// it won't fit: generate no data and return zero size
		return AJA_STATUS_FAIL;
	}

	if (IsDigital())
	{
		pData[0] = GetGUMPHeaderByte1();
		pData[1] = GetGUMPHeaderByte2();
		pData[2] = GetGUMPHeaderByte3();

		pData[3] = m_DID;
		pData[4] = m_SID;

		uint8_t payloadSize = (GetDC() > 255) ? 255 : GetDC();	// truncate payload to max 255 bytes
		pData[5] = payloadSize;	

		// copy payload data to raw stream
		status = GetPayloadData(&pData[6], payloadSize);

		// note: the hardware automatically recalculates the checksum anyway, so this byte
		// is ignored. However, if the original source had a checksum we'll send it back...
//		pData[6+payloadSize] = checksum;
		pData[6+payloadSize] = Calculate8BitChecksum();

		// all done!
		outPacketSize = myPacketSize;
	}
	else if (IsRaw())
	{
		// "analog" or "raw" ancillary data is special in that it may generate multiple output packets,
		// depending on the length of the payload data.
		// NOTE: we're assuming that zero-length payloads have already been screened out (above)!
		uint32_t numPackets = (GetDC() + 254) / 255;

		// all packets will have a 255-byte payload except the last one
		//Note: Unused --  uint32_t lastPacketDC = m_DC % 255;

		const uint8_t *	payloadPtr = GetPayloadData();
		uint32_t remainingPayloadData = GetDC();

		for (uint32_t i = 0;  i < numPackets;  i++)
		{
			pData[0] = GetGUMPHeaderByte1();
			pData[1] = GetGUMPHeaderByte2();
			pData[2] = GetGUMPHeaderByte3();

			pData[3] = m_DID;
			pData[4] = m_SID;

			uint8_t payloadSize = (remainingPayloadData > 255) ? 255 : remainingPayloadData;	// truncate payload to max 255 bytes
			pData[5] = payloadSize;		// DC

			// copy payload data to raw stream
			::memcpy(&pData[6], payloadPtr, payloadSize);

			// note: the hardware automatically recalculates the checksum anyway, so this byte
			// is ignored. However, if the original source had a checksum we'll send it back...
			pData[6+payloadSize] = m_checksum;

			// advance the payloadPtr to the beginning of the next bunch of payload data
			payloadPtr += payloadSize;

			// decrease the remaining data count
			remainingPayloadData -= payloadSize;

			// advance the external data stream pointer to the beginning of the next packet
			pData += (payloadSize + AJAAncillaryDataWrapperSize);
		}

		// all done!
		outPacketSize = myPacketSize;
	}
	else	// huh? coding param not set - don't generate anything
	{
		outPacketSize = 0;
		status = AJA_STATUS_FAIL;
	}

	return status;
}


uint8_t AJAAncillaryData::GetGUMPHeaderByte2 (void) const
{
	uint8_t result	(0x80);	// LE bit is always active

	if (m_coding == AJAAncillaryDataCoding_Raw)
		result |= 0x40;		// analog/raw (1) or digital (0) ancillary data

	if (m_location.IsLumaChannel())
		result |= 0x20;		// carried in Y (1) or C stream

	if (m_location.IsHanc())
		result |= 0x10;

	// ms 4 bits of line number
	result |= (m_location.GetLineNumber() >> 7) & 0x0F;	// ms 4 bits [10:7] of line number

	return result;
}


AJAStatus AJAAncillaryData::GenerateTransmitData (vector<uint16_t> & outRawComponents) const
{
	AJAStatus							status		(AJA_STATUS_SUCCESS);
	const vector<uint16_t>::size_type	origSize	(outRawComponents.size());

	if (IsDigital())
	{
		try
		{
			const uint8_t	dataCount	((GetDC() > 255) ? 255 : GetDC());	//	Truncate payload to max 255 bytes
			outRawComponents.push_back(0x000);														//	000
			outRawComponents.push_back(0x3FF);														//	3FF
			outRawComponents.push_back(0x3FF);														//	3FF
			outRawComponents.push_back(CNTV2SMPTEAncData::AddEvenParity(GetDID()));					//	DID
			outRawComponents.push_back(CNTV2SMPTEAncData::AddEvenParity(GetSID()));					//	SDID
			outRawComponents.push_back(CNTV2SMPTEAncData::AddEvenParity(dataCount));				//	DC
		}
		catch(...)
		{
			outRawComponents.resize(origSize);
			status = AJA_STATUS_MEMORY;
		}
	}

	//	Copy payload data into output vector...
	if (AJA_SUCCESS(status))
		status = GetPayloadData(outRawComponents, IsDigital() /* Add parity for Digital only */);	//	UDWs

	//	The hardware automatically recalcs the CS, but still needs to be there...
	if (AJA_SUCCESS(status) && IsDigital())
		outRawComponents.push_back(CNTV2SMPTEAncData::AddEvenParity(Calculate8BitChecksum()));	//	CS

	if (AJA_SUCCESS(status))
		LOGMYDEBUG("Appended " << (outRawComponents.size()-origSize) << " elements: " << AsString(16));
	else
		LOGMYERROR("Failed: " << ::AJAStatusToString(status) << ": origSize=" << origSize << ", " << AsString(16));
	return status;
}


static const string		gEmptyString;


const string & AJAAncillaryDataLinkToString (const AJAAncillaryDataLink inValue, const bool inCompact)
{
	static const string		gAncDataLinkToStr []			= {"A", "B", "?"};
	static const string		gDAncDataLinkToStr []			= {"AJAAncillaryDataLink_A", "AJAAncillaryDataLink_B", "AJAAncillaryDataLink_Unknown"};

	return IS_VALID_AJAAncillaryDataLink(inValue) ? (inCompact ? gAncDataLinkToStr[inValue] : gDAncDataLinkToStr[inValue]) : gAncDataLinkToStr[2];
}


const string &	AJAAncillaryDataStreamToString (const AJAAncillaryDataStream inValue, const bool inCompact)
{
	static const string		gAncDataStreamToStr []			= {"DS1", "DS2", "DS3", "DS4", "?"};
	static const string		gDAncDataStreamToStr []			= {"AJAAncillaryDataStream_1", "AJAAncillaryDataStream_2",
																"AJAAncillaryDataStream_3", "AJAAncillaryDataStream_4", "AJAAncillaryDataStream_Unknown"};

	return IS_VALID_AJAAncillaryDataStream(inValue) ? (inCompact ? gAncDataStreamToStr[inValue] : gDAncDataStreamToStr[inValue]) : gEmptyString;
}


const string & AJAAncillaryDataChannelToString (const AJAAncillaryDataChannel inValue, const bool inCompact)
{
	static const string		gAncDataChannelToStr []		= {"C", "Y", "?"};
	static const string		gDAncDataChannelToStr []	= {"AJAAncillaryDataChannel_C", "AJAAncillaryDataChannel_Y", "AJAAncillaryDataChannel_Unknown"};

	return IS_VALID_AJAAncillaryDataChannel(inValue) ? (inCompact ? gAncDataChannelToStr[inValue] : gDAncDataChannelToStr[inValue]) : gEmptyString;
}


const string & AJAAncillaryDataSpaceToString (const AJAAncillaryDataSpace inValue, const bool inCompact)
{
	static const string		gAncDataSpaceToStr []			= {"VANC", "HANC", "????"};
	static const string		gDAncDataSpaceToStr []			= {"AJAAncillaryDataSpace_VANC", "AJAAncillaryDataSpace_HANC", "AJAAncillaryDataSpace_Unknown"};

	return IS_VALID_AJAAncillaryDataSpace(inValue) ? (inCompact ? gAncDataSpaceToStr[inValue] : gDAncDataSpaceToStr[inValue]) : gEmptyString;
}


string AJAAncillaryDataLocationToString (const AJAAncillaryDataLocation & inValue, const bool inCompact)
{
	ostringstream	oss;
	oss	<< ::AJAAncillaryDataLinkToString(inValue.GetDataLink(), inCompact)
		<< "|" << ::AJAAncillaryDataStreamToString(inValue.GetDataStream(), inCompact)
		<< "|" << ::AJAAncillaryDataChannelToString(inValue.GetDataChannel(), inCompact)
		<< "|" << ::AJAAncillaryDataSpaceToString(inValue.GetDataSpace(), inCompact)
		<< "|L" << DEC(inValue.GetLineNumber());
	if (inValue.GetHorizontalOffset() != AJAAncillaryDataLocation::AJAAncDataHorizOffset_Default)
	{
		oss	<< "|";
		if (inValue.GetHorizontalOffset() == AJAAncillaryDataLocation::AJAAncDataHorizOffset_AnyHanc)
			oss	<< "EAV*";
		else if (inValue.GetHorizontalOffset() == AJAAncillaryDataLocation::AJAAncDataHorizOffset_Anywhere)
			oss	<< "SAV*";
		else
			oss << "SAV+" << DEC(inValue.GetHorizontalOffset());
	}
	//else oss << DEC(inValue.GetHorizontalOffset());	//	DON'T SHOW ANYTHING FOR ZERO (DEFAULT)
	return oss.str ();
}

ostream & operator << (ostream & inOutStream, const AJAAncillaryDataLocation & inValue)
{
	inOutStream	<< ::AJAAncillaryDataLocationToString(inValue, true);
	return inOutStream;
}


const string & AJAAncillaryDataCodingToString (const AJAAncillaryDataCoding inValue, const bool inCompact)
{
	static const string		gAncDataCodingToStr []			= {"Dig", "Ana", "???"};
	static const string		gDAncDataCodingToStr []			= {"AJAAncillaryDataCoding_Digital", "AJAAncillaryDataCoding_Raw", "AJAAncillaryDataCoding_Unknown"};

	return IS_VALID_AJAAncillaryDataCoding (inValue) ? (inCompact ? gAncDataCodingToStr[inValue] : gDAncDataCodingToStr[inValue]) : gEmptyString;
}


const string & AJAAncillaryDataTypeToString (const AJAAncillaryDataType inValue, const bool inCompact)
{
	static const string		gAncDataTypeToStr []			= {	"Unknown", "SMPTE 2016-3 AFD", "SMPTE 12-M RP188", "SMPTE 12-M VITC",
																"SMPTE 334 CEA708", "SMPTE 334 CEA608", "CEA608 Line21", "SMPTE 352 VPID",
																"SMPTE 2051 2 Frame Marker", "524D Frame Status", "5251 Frame Status", "?"};
	static const string		gDAncDataTypeToStr []			= {	"AJAAncillaryDataType_Unknown", "AJAAncillaryDataType_Smpte2016_3", "AJAAncillaryDataType_Timecode_ATC",
																"AJAAncillaryDataType_Timecode_VITC", "AJAAncillaryDataType_Cea708", "AJAAncillaryDataType_Cea608_Vanc",
																"AJAAncillaryDataType_Cea608_Line21", "AJAAncillaryDataType_Smpte352", "AJAAncillaryDataType_Smpte2051",
																"AJAAncillaryDataType_FrameStatusInfo524D", "AJAAncillaryDataType_FrameStatusInfo5251", "?"};

	return inValue < AJAAncillaryDataType_Size ? (inCompact ? gAncDataTypeToStr[inValue] : gDAncDataTypeToStr[inValue]) : gEmptyString;
}


ostream & AJAAncillaryData::Print (ostream & inOutStream, const bool inDumpPayload) const
{
	inOutStream << "Type:\t\t"	<< AJAAncillaryData::DIDSIDToString(m_DID, m_SID)	<< endl	//	::AJAAncillaryDataTypeToString (GetAncillaryDataType ()) << endl
				<< "DID:\t\t"	<< xHEX0N(uint32_t(m_DID),2)						<< endl
				<< "SID:\t\t"	<< xHEX0N(uint32_t(m_SID),2)						<< endl
				<< "DC:\t\t"	<< DEC(GetDC())										<< endl
				<< "CS:\t\t"	<< xHEX0N(uint32_t(m_checksum),2)					<< endl
				<< "Loc:\t\t"	<< m_location										<< endl
				<< "Coding:\t\t"<< ::AJAAncillaryDataCodingToString(m_coding)		<< endl
				<< "Valid:\t\t"	<< (m_rcvDataValid ? "Yes" : "No");
	if (inDumpPayload)
		{inOutStream << endl;  DumpPayload (inOutStream);}
	return inOutStream;
}


string AJAAncillaryData::AsString (uint16_t inDumpMaxBytes) const
{
	ostringstream	oss;
	oss	<< "[" << ::AJAAncillaryDataCodingToString(GetDataCoding())
		<< "|" << ::AJAAncillaryDataLocationToString(GetDataLocation()) << "|" << GetDIDSIDPair() << "]";
	if (inDumpMaxBytes  &&  GetDC())
	{
		uint16_t	byteCount	= uint16_t(GetPayloadByteCount());
		oss << byteCount << " bytes: ";
		if (inDumpMaxBytes > byteCount)
			inDumpMaxBytes = byteCount;
		for (uint16_t ndx(0);  ndx < inDumpMaxBytes;  ndx++)
			oss << HEX0N(uint16_t(m_payload[ndx]),2);
	}
	return oss.str();
}


ostream & operator << (ostream & inOutStream, const AJAAncillaryDIDSIDPair & inData)
{
	inOutStream << "DID/SID " << xHEX0N(uint16_t(inData.first), 2) << "/" << xHEX0N(uint16_t(inData.second), 2);
	return inOutStream;
}


ostream & AJAAncillaryData::DumpPayload (ostream & inOutStream) const
{
	if (IsEmpty())
		inOutStream	<< "(NULL payload)" << endl;
	else
	{
		const int32_t	kBytesPerLine	(32);
		uint32_t		count			(GetDC());
		const uint8_t *	pData			(GetPayloadData());

		while (count > 0)
		{
			const uint32_t	numBytes	((count >= kBytesPerLine) ? kBytesPerLine : count);
			inOutStream << ((count == GetDC()) ? "Payload:  " : "          ");
			for (uint8_t num(0);  num < numBytes;  num++)
			{
				inOutStream << " " << HEX0N(uint32_t(pData[num]),2);
				if (num % 4 == 3)
					inOutStream << " ";		// an extra space every four bytes for readability
			}
			inOutStream << endl;
			pData += numBytes;
			count -= numBytes;
		}	//	loop til break
	}
	return inOutStream;
}


bool AJAAncillaryData::operator == (const AJAAncillaryData & inRHS) const
{
	if (GetDID() == inRHS.GetDID()
		&&  GetSID() == inRHS.GetSID()
		&&  GetDC() == inRHS.GetDC()
		&&  GetChecksum() == inRHS.GetChecksum()
		&&  GetDataLocation() == inRHS.GetDataLocation()
		&&  GetDataCoding() == inRHS.GetDataCoding())
		{
			if (IsEmpty())
				return true;
			if (::memcmp (GetPayloadData(), inRHS.GetPayloadData(), GetPayloadByteCount()) == 0)
				return true;
		}
	return false;
}


string AJAAncillaryData::DIDSIDToString (const uint8_t inDID, const uint8_t inSID)
{
	switch (inDID)
	{
		case 0x00:	return "SMPTE-291 Control Packet";
		case 0x08:	if (inSID == 0x08) return "SMPTE-291 Control Packet";
					break;
		case 0x40:	switch (inSID)
					{
						case 0x01:	return "RP-305 SDTI Header Data";
						case 0x02:	return "RP-348 HD-SDTI Header Data";
						case 0x04:	return "SMPTE-427 Link Encryp Key Msg 1";
						case 0x05:	return "SMPTE-427 Link Encryp Key Msg 2";
						case 0x06:	return "SMPTE-427 Link Encryp MetaD";
					}
					break;
		case 0x41:	switch (inSID)
					{
						case 0x01:	return "SMPTE-352M Payload ID";
						case 0x05:	return "SMPTE-2016-3 ADF/Bar Data";
						case 0x06:	return "SMPTE-2016-4 Pan & Scan Data";
						case 0x07:	return "SMPTE-2010 ANSI/SCTE 104 Msgs";
						case 0x08:	return "SMPTE-2031 DVB/SCTE VBI Data";
					}
					break;
		case 0x43:	switch (inSID)
					{
						case 0x01:	return "BT.1685 Inter-Station Ctrl Data";
						case 0x02:	return "RDD08/OP-47 Teletext Subtitling";
						case 0x03:	return "RDD08/OP-47 VANC Multipacket";
						case 0x04:	return "ARIB TR-B29 AV Sig Error Mon MetaD";
						case 0x05:	return "RDD18 Camera Params";
					}
					break;
		case 0x44:	if (inSID == 0x04 || inSID == 0x14)	return "RP-214 KLV Encoded MetaD & Essence";
					else if (inSID == 0x44)				return "RP-223 UMID & Prog ID Label Data";
					break;
		case 0x45:	if (inSID > 0  &&  inSID < 0x0A)	return "RP-2020 Compr/Dolby Aud MetaD";
					break;
	//	case 0x46:	if (inSID == 0x01)
		case 0x50:	if (inSID == 0x01)		return "RDD08 WSS Data";
					else if (inSID == 0x51)	return "CineLink-2 Link Encryp MetaD";
					break;
		case 0x51:	if (inSID == 0x01)		return "RP-215 Film Transfer Info";
					else if (inSID == 0x02)	return "RDD-18 Cam Param MetaD Set Acq";
					break;
		case 0x5F:	if (inSID == 0xDF)		return "ARIB STD-B37 HD Captions";
					else if (inSID == 0xDE)	return "ARIB STD-B37 SD Captions";
					else if (inSID == 0xDD)	return "ARIB STD-B37 Analog Captions";
					else if (inSID == 0xDC)	return "ARIB STD-B37 Mobile Captions";
					else if ((inSID & 0xF0) == 0xD0)	return "ARIB STD-B37 ??? Captions";
					else					return "ARIB STD-B37 ???";
					break;
		case 0x60:	if (inSID == 0x60)		return "SMPTE-12M ATC Timecode";
					break;
		case 0x61:	if (inSID == 0x01)		return "SMPTE-334 HD CEA-708 CC";
					else if (inSID == 0x02)	return "SMPTE-334 SD CEA-608 CC";
					break;
		case 0x62:	if (inSID == 0x01)		return "RP-207 DTV Program Desc";
					else if (inSID == 0x02)	return "SMPTE-334 Data Broadcast";
					else if (inSID == 0x03)	return "RP-208 VBI Data";
					break;
		case 0x64:	if (inSID == 0x64)		return "RP-196 LTC in HANC (Obs)";
					else if (inSID == 0x7F)	return "RP-196 VITC in HANC (Obs)";
					break;
		case 0x80:	return "SMPTE-291 Ctrl Pkt 'Marked for Deletion'";
		case 0x84:	return "SMPTE-291 Ctrl Pkt 'End Marker'";
		case 0x88:	return "SMPTE-291 Ctrl Pkt 'Start Marker'";
		case 0xA0:	return "SMPTE-299M 3G HD Aud Ctrl 8";
		case 0xA1:	return "SMPTE-299M 3G HD Aud Ctrl 7";
		case 0xA2:	return "SMPTE-299M 3G HD Aud Ctrl 6";
		case 0xA3:	return "SMPTE-299M 3G HD Aud Ctrl 5";
		case 0xA4:	return "SMPTE-299M 3G HD Aud Data 8";
		case 0xA5:	return "SMPTE-299M 3G HD Aud Data 7";
		case 0xA6:	return "SMPTE-299M 3G HD Aud Data 6";
		case 0xA7:	return "SMPTE-299M 3G HD Aud Data 5";
		case 0xD1:
		case 0xD2:	return "AJA QA F1 Test Packet";
		case 0xD3:	return "AJA QA F2 Test Packet";
		case 0xE0:	return "SMPTE-299M HD Aud Ctrl 4";
		case 0xE1:	return "SMPTE-299M HD Aud Ctrl 3";
		case 0xE2:	return "SMPTE-299M HD Aud Ctrl 2";
		case 0xE3:	return "SMPTE-299M HD Aud Ctrl 1";
		case 0xE4:	return "SMPTE-299M HD Aud Data 4";
		case 0xE5:	return "SMPTE-299M HD Aud Data 3";
		case 0xE6:	return "SMPTE-299M HD Aud Data 2";
		case 0xE7:	return "SMPTE-299M HD Aud Data 1";
		case 0xEC:	return "SMPTE-272M SD Aud Ctrl 4";
		case 0xED:	return "SMPTE-272M SD Aud Ctrl 3";
		case 0xEE:	return "SMPTE-272M SD Aud Ctrl 2";
		case 0xEF:	return "SMPTE-272M SD Aud Ctrl 1";
		case 0xF0:	return "SMPTE-315 Camera Position";
		case 0xF4:	return "RP-165 Error Detect/Checkwords";
		case 0xF8:	return "SMPTE-272M SD Aud Ext Data 4";
		case 0xF9:	return "SMPTE-272M SD Aud Data 4";
		case 0xFA:	return "SMPTE-272M SD Aud Ext Data 3";
		case 0xFB:	return "SMPTE-272M SD Aud Data 3";
		case 0xFC:	return "SMPTE-272M SD Aud Ext Data 2";
		case 0xFD:	return "SMPTE-272M SD Aud Data 2";
		case 0xFE:	return "SMPTE-272M SD Aud Ext Data 1";
		case 0xFF:	return "SMPTE-272M SD Aud Data 1";
	}
	return "";
}	//	DIDSID2String
