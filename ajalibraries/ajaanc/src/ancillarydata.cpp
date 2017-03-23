/**
	@file		ancillarydata.cpp
	@brief		Implementation of the AJAAncillaryData class.
	@copyright	(C) 2010-2017 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#include "ntv2publicinterface.h"
#include "ancillarydata.h"
#if defined(AJA_LINUX)
	#include <string.h>				// For memcpy
	#include <stdlib.h>				// For realloc
#endif
#include <ios>

using namespace std;


const uint32_t AJAAncillaryDataWrapperSize = 7;		// 3 bytes header + DID + SID + DC + Checksum: i.e. everything EXCEPT the payload

//const uint8_t  AJAAncillaryDataAnalogDID = 0x00;		// used in header DID field when ancillary data is "analog"
//const uint8_t  AJAAncillaryDataAnalogSID = 0x00;		// used in header SID field when ancillary data is "analog"



AJAAncillaryData::AJAAncillaryData()
{
	m_pPayload = NULL;
	Init();
}


AJAAncillaryData::AJAAncillaryData (const AJAAncillaryData & inClone)
{
	m_pPayload = NULL;
	Init();
	*this = inClone;
}


AJAAncillaryData::AJAAncillaryData (const AJAAncillaryData * pClone)
{
	m_pPayload = NULL;
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
	m_DC  = 0;

	m_checksum = 0;

	m_coding = AJAAncillaryDataCoding_Digital;

	m_location.link		= AJAAncillaryDataLink_A;
	m_location.stream   = AJAAncillaryDataVideoStream_Y;
	m_location.ancSpace = AJAAncillaryDataSpace_VANC;
	m_location.lineNum  = 0;

	m_ancType	   = AJAAncillaryDataType_Unknown;
	m_rcvDataValid = false;
}


void AJAAncillaryData::Clear (void)
{
	Init();
}


AJAAncillaryData & AJAAncillaryData::operator= (const AJAAncillaryData & rhs)
{
	if (this != &rhs)		// ignore self-assignment
	{
		m_DID		= rhs.m_DID;
		m_SID		= rhs.m_SID;
		m_checksum	= rhs.m_checksum;
		m_location	= rhs.m_location;
		m_coding	= rhs.m_coding;

		m_ancType	= rhs.m_ancType;
		m_rcvDataValid = rhs.m_rcvDataValid;

		if (rhs.m_DC > 0)
		{
			AJAStatus status = AllocDataMemory(rhs.m_DC);

			if (status == AJA_STATUS_SUCCESS && m_pPayload != NULL)
				rhs.GetPayloadData(m_pPayload, m_DC);
		}
	}

	return *this;
}


AJAAncillaryData * AJAAncillaryData::Clone (void) const
{
	return new AJAAncillaryData (this);
}


AJAStatus AJAAncillaryData::AllocDataMemory(uint32_t numBytes)
{
	AJAStatus status;

	if (m_pPayload)
		FreeDataMemory ();

	uint8_t *	pPayload = NULL;
	if (numBytes > 0)
		pPayload = new uint8_t[numBytes];

	if (pPayload)
	{
		m_pPayload = pPayload;
		m_DC = numBytes;
		status = AJA_STATUS_SUCCESS;
	}
	else
	{
		m_pPayload = NULL;
		m_DC = 0;
		status = AJA_STATUS_FAIL;
	}

	return status;
}


AJAStatus AJAAncillaryData::FreeDataMemory (void)
{
	if (m_pPayload)
	{
		delete[] m_pPayload;
		m_pPayload = NULL;
	}

	m_DC = 0;

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
	// note: this is NOT the "real" 9-bit checksum used in SMPTE 299 Ancillary packets,
	//       but it's calculated the same way and should be the same as the ls 8-bits
	//       of the 9-bit checksum.
	uint8_t sum = 0;

	sum += m_DID;
	sum += m_SID;
	sum += m_DC;

	if (m_pPayload  &&  m_DC > 0)
		for (uint32_t i = 0;  i < m_DC;  i++)
			sum += m_pPayload[i];

	return sum;
}


//**********
// Set anc data location parameters
//
AJAStatus AJAAncillaryData::SetDataLocation (const AJAAncillaryDataLocation & loc)
{
	AJAStatus	status	(SetLocationVideoLink (loc.link));
	if (AJA_SUCCESS (status))
		status = SetLocationVideoStream (loc.stream);
	if (AJA_SUCCESS (status))
		status = SetLocationVideoSpace (loc.ancSpace);
	if (AJA_SUCCESS (status))
		status = SetLocationLineNumber (loc.lineNum);
	return status;
}


AJAStatus AJAAncillaryData::GetDataLocation (AJAAncillaryDataLink &			outLink,
											AJAAncillaryDataVideoStream &	outStream,
											AJAAncillaryDataSpace &			outAncSpace,
											uint16_t &						outLineNum)
{
	outLink		= m_location.link;
	outStream	= m_location.stream;
	outAncSpace	= m_location.ancSpace;
	outLineNum	= m_location.lineNum;
	return AJA_STATUS_SUCCESS;
}

//-------------
//
AJAStatus AJAAncillaryData::SetDataLocation (const AJAAncillaryDataLink link, const AJAAncillaryDataVideoStream stream, const AJAAncillaryDataSpace ancSpace, const uint16_t lineNum)
{
	AJAStatus	status	(SetLocationVideoLink (link));
	if (AJA_SUCCESS (status))
		status = SetLocationVideoStream (stream);
	if (AJA_SUCCESS (status))
		status = SetLocationVideoSpace (ancSpace);
	if (AJA_SUCCESS (status))
		status = SetLocationLineNumber (lineNum);
	return status;
}


//-------------
//
AJAStatus AJAAncillaryData::SetLocationVideoLink (const AJAAncillaryDataLink inLinkValue)
{
	if (!IS_VALID_AJAAncillaryDataLink (inLinkValue))
		return AJA_STATUS_RANGE;

	m_location.link = inLinkValue;
	return AJA_STATUS_SUCCESS;
}


//-------------
//
AJAStatus AJAAncillaryData::SetLocationVideoStream (const AJAAncillaryDataVideoStream stream)
{
	if (stream != AJAAncillaryDataVideoStream_C  &&  stream != AJAAncillaryDataVideoStream_Y)
		return AJA_STATUS_RANGE;

	m_location.stream = stream;
	return AJA_STATUS_SUCCESS;
}


//-------------
//
AJAStatus AJAAncillaryData::SetLocationVideoSpace (const AJAAncillaryDataSpace space)
{
	if (space != AJAAncillaryDataSpace_VANC  &&  space != AJAAncillaryDataSpace_HANC)
		return AJA_STATUS_RANGE;

	m_location.ancSpace = space;
	return AJA_STATUS_SUCCESS;
}


//-------------
//
AJAStatus AJAAncillaryData::SetLocationLineNumber (const uint16_t lineNum)
{
	//	No range checking here because we don't know how big the frame is
	m_location.lineNum = lineNum;
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

	//	Free any existing data...
	FreeDataMemory ();

	//	Allocate new memory...
	AJAStatus status = AllocDataMemory (inNumBytes);

	//	Copy payload into new memory...
	if (status == AJA_STATUS_SUCCESS  &&  m_pPayload)
		::memcpy (m_pPayload, pInData, inNumBytes);
	
	return status;
}


AJAStatus AJAAncillaryData::SetFromSMPTE334 (const uint16_t * pInData, const uint32_t inNumWords, const AJAAncillaryDataLocation & inLocInfo)
{
	if (!pInData)
		return AJA_STATUS_NULL;
	if (inNumWords < 7)
		return AJA_STATUS_RANGE;

	const uint32_t	payloadByteCount	(uint32_t (pInData [5] & 0x00FF));
	if ((inNumWords - 7) > payloadByteCount)
		return AJA_STATUS_RANGE;

	//	Free any existing data...
	FreeDataMemory ();

	//	Allocate new memory...
	const AJAStatus status	(AllocDataMemory (payloadByteCount));
	if (AJA_FAILURE (status))
		return status;

	if (!m_pPayload)
		return AJA_STATUS_MEMORY;

	//	Copy payload into new memory...
	for (uint32_t numWord (0);  numWord < payloadByteCount;  numWord++)
		m_pPayload [numWord] = UByte (pInData [6 + numWord] & 0x00FF);

	SetDataCoding (AJAAncillaryDataCoding_Digital);
	SetDataLocation (inLocInfo);
	SetChecksum (UByte (pInData [payloadByteCount + 6] & 0x00FF));
	SetDID (UByte (pInData[3] & 0x00FF));
	SetSID (UByte (pInData[4] & 0x00FF));

	return AJA_STATUS_SUCCESS;
}


//**********
// Append payload data from external source to existing internal storage. Realloc's current
// payload memory (if any) or allocates new memory

AJAStatus AJAAncillaryData::AppendPayloadData (const uint8_t * pInData, const uint32_t inNumBytes)
{
	if (pInData == NULL || inNumBytes == 0)
		return AJA_STATUS_NULL;

	//	See if we can grow (or make) the existing payload buffer...
	uint8_t * pNewPtr = (uint8_t *) ::realloc (m_pPayload, m_DC + inNumBytes);

	//	If realloc fails, return an error but leave the original data intact...
	if (pNewPtr == NULL)
		return AJA_STATUS_FAIL;

	//	We now have our original data in a larger payload buffer
	m_pPayload = pNewPtr;
	::memcpy ((m_pPayload + m_DC), pInData, inNumBytes);	//	Append the new stuff
	m_DC += inNumBytes; 
	
	return AJA_STATUS_SUCCESS;
}


//**********
// Append payload data from an external AJAAncillaryData object to existing internal storage.

AJAStatus AJAAncillaryData::AppendPayload (const AJAAncillaryData & inAnc)
{
	//	See if we can grow (or make) the existing payload buffer...
	uint8_t *	pNewPtr	= (uint8_t *) ::realloc (m_pPayload, (m_DC + inAnc.m_DC));

	//	If realloc fails, return an error but leave the original data intact...
	if (pNewPtr == NULL)
		return AJA_STATUS_FAIL;

	m_pPayload = pNewPtr;	//	Original data is now in a larger payload buffer
	AJAStatus	status	= inAnc.GetPayloadData ((m_pPayload + m_DC), inAnc.m_DC);	//	Copy new data from append source
	if (status != AJA_STATUS_SUCCESS)
		return status;

	m_DC += inAnc.m_DC; 
	return AJA_STATUS_SUCCESS;
}


//**********
// Copy payload data from internal storage to external destination.

AJAStatus AJAAncillaryData::GetPayloadData (uint8_t * pOutData, const uint32_t inNumBytes) const
{
	if (pOutData == NULL)
		return AJA_STATUS_NULL;

	if (inNumBytes > m_DC)
		return AJA_STATUS_RANGE;

	if (m_pPayload == NULL)
		return AJA_STATUS_NULL;

	::memcpy (pOutData, m_pPayload, inNumBytes);
	return AJA_STATUS_SUCCESS;
}


uint8_t AJAAncillaryData::GetPayloadByteAtIndex (const uint32_t inIndex0) const
{
	if (m_pPayload && inIndex0 < m_DC)
		return m_pPayload [inIndex0];
	else
		return 0;
}


AJAStatus AJAAncillaryData::ParsePayloadData (void)
{
	// should be overridden by derived classes to parse payload data to local data
	m_rcvDataValid = false;

	return AJA_STATUS_SUCCESS;
}


//**********
// Initializes the AJAAncillaryData object from "raw" ancillary data received from hardware (ingest)
AJAStatus AJAAncillaryData::InitWithReceivedData (const uint8_t * pInData, const uint32_t inMaxBytes, const AJAAncillaryDataLocation & inLocationInfo, uint32_t & outPacketByteCount)
{
	AJAStatus status = AJA_STATUS_SUCCESS;

	// if all is well, pInData points to the beginning of a packet that is laid out like this:
	//
	//	pInData ->	0:	0xFF			// 1st byte is always FF
	//				1:  Hdr data1		// location data byte #1
	//				2:  Hdr data2		// location data byte #2
	//				3:  DID				// ancillary packet Data ID
	//				4:  SID				// ancillary packet Secondary ID (or DBN)
	//				5:  DC				// ancillary packet Data Count (size of payload: 0 - 255)
	//				6:  Payload[0]		// 1st byte of payload
	//				7:  Payload[1]		// 2nd byte of payload
	//             ...    ...
	//		 (5 + DC):  Payload[DC-1]	// last byte of payload
	//		 (6 + DC):	checksum		// 8-bit sum of (DID + SID + DC + Payload[0] + ... + Payload[DC-1])
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
	// except the line number. We decode it 
	//
	// When we have extracted the useful data from the packet, we return the size of the packet so the
	// caller can find the beginning of the next packet.

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
		m_location.stream   = ((pInData[1] & 0x20) == 0) ? AJAAncillaryDataVideoStream_C : AJAAncillaryDataVideoStream_Y;	// byte 1, bit 5
		m_location.ancSpace = ((pInData[1] & 0x10) == 0) ? AJAAncillaryDataSpace_VANC : AJAAncillaryDataSpace_HANC;			// byte 1, bit 4
		m_location.lineNum  = ((pInData[1] & 0x0F) << 7) + (pInData[2] & 0x7F);												// byte 1, bits 3:0 + byte 2, bits 6:0
	}

	//	Allocate space for the payload and copy it in...
	uint32_t payloadSize = pInData[5];	// SMPTE-291 Data Count
	if (payloadSize)
	{
		status = AllocDataMemory (payloadSize);					// note: this also sets our local "DC" value
		if (status == AJA_STATUS_SUCCESS && m_pPayload)
			::memcpy (m_pPayload, &pInData[6], payloadSize);
	}

	outPacketByteCount = totalSize;

	return status;
}


AJAStatus AJAAncillaryData::InitWithReceivedData (const vector<uint8_t> & inData, const AJAAncillaryDataLocation & inLocationInfo)
{
	AJAStatus status = AJA_STATUS_SUCCESS;

	// if all is well, inData contains a packet that's laid out like this:
	//
	//	pInData ->	0:	0xFF			// 1st byte is always FF
	//				1:  Hdr data1		// location data byte #1
	//				2:  Hdr data2		// location data byte #2
	//				3:  DID				// ancillary packet Data ID
	//				4:  SID				// ancillary packet Secondary ID (or DBN)
	//				5:  DC				// ancillary packet Data Count (size of payload: 0 - 255)
	//				6:  Payload[0]		// 1st byte of payload
	//				7:  Payload[1]		// 2nd byte of payload
	//             ...    ...
	//		 (5 + DC):  Payload[DC-1]	// last byte of payload
	//		 (6 + DC):	checksum		// 8-bit sum of (DID + SID + DC + Payload[0] + ... + Payload[DC-1])
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
	// except the line number. We decode it 
	//
	// When we have extracted the useful data from the packet, we return the size of the packet so the
	// caller can find the beginning of the next packet.

	if (inData.empty())
		return AJA_STATUS_NULL;

	//	The minimum size for a packet (i.e. no payload) is 7 bytes
	if (inData.size() < size_t(AJAAncillaryDataWrapperSize))
		return AJA_STATUS_RANGE;

	//	The first byte should be 0xFF. If it's not, then the Anc data stream may be broken...
	if (inData[0] != 0xFF)
		return AJA_STATUS_BAD_PARAM;

	//	So we have at least enough bytes for a minimum packet, and the first byte is what we expect.
	//	Let's see what size this packet actually reports...
	uint32_t totalSize = inData[5] + AJAAncillaryDataWrapperSize;

	//	If the reported packet size extends beyond the end of the buffer, we're toast...
	if (size_t(totalSize) > inData.size())
		return AJA_STATUS_RANGE;

	//	OK... we have enough data in the buffer to contain the packet, and everything else checks out,
	//	so go ahead and parse the data...

	m_DID	   = inData[3];				// SMPTE-291 Data ID
	m_SID	   = inData[4];				// SMPTE-291 Secondary ID (or DBN)
//	DC		   = inData[5];
	m_checksum = inData[totalSize-1];	// reported checksum

	//	Caller provides all of the "location" information as a default. If the packet header info is "real", overwrite it...
	m_location = inLocationInfo;

	if ((inData[1] & 0x80) != 0)
	{
		m_coding            = ((inData[1] & 0x40) == 0) ? AJAAncillaryDataCoding_Digital : AJAAncillaryDataCoding_Analog;	// byte 1, bit 6
		m_location.stream   = ((inData[1] & 0x20) == 0) ? AJAAncillaryDataVideoStream_C : AJAAncillaryDataVideoStream_Y;	// byte 1, bit 5
		m_location.ancSpace = ((inData[1] & 0x10) == 0) ? AJAAncillaryDataSpace_VANC : AJAAncillaryDataSpace_HANC;			// byte 1, bit 4
		m_location.lineNum  = ((inData[1] & 0x0F) << 7) + (inData[2] & 0x7F);												// byte 1, bits 3:0 + byte 2, bits 6:0
	}

	//	Allocate space for the payload and copy it in...
	uint32_t payloadSize = inData[5];	// SMPTE-291 Data Count
	if (payloadSize)
	{
		status = AllocDataMemory (payloadSize);					// note: this also sets our local "DC" value
		if (status == AJA_STATUS_SUCCESS && m_pPayload)
			for (uint32_t ndx(0);  ndx < payloadSize;  ndx++)
				m_pPayload[ndx] = inData[ndx+6];
	}

	return status;
}


//**********
// This returns the number of bytes that will be returned by GenerateTransmitData(). This is usually
// called first so the caller can allocate a buffer large enough to hold the results.

AJAStatus AJAAncillaryData::GetRawPacketSize (uint32_t & packetSize)
{
	AJAStatus status = AJA_STATUS_SUCCESS;

	// if this is digital ancillary data (i.e. SMPTE-291 based), then the total size will be
	// seven bytes (3 bytes header + DID + SID + DC + Checksum) plus the payload size.
	if (m_coding == AJAAncillaryDataCoding_Digital)
	{
		if (m_DC <= 255)
			packetSize = m_DC + AJAAncillaryDataWrapperSize;

		else		// this is an error condition: if we have more than 255 bytes of payload in a "digital" packet, truncate the payload
			packetSize = 255 + AJAAncillaryDataWrapperSize;
	}
	else if (m_coding == AJAAncillaryDataCoding_Raw)
	{
		// if this is analog/raw ancillary data then we need to figure out how many "packets" we need
		// to generate in order to pass all of the payload data (max 255 bytes per packet)
		// special case: if DC is zero, there's no reason to generate ANY output!
		if (m_DC == 0)
			packetSize = 0;
		else	// DC > 0
		{
			uint32_t numPackets = (m_DC + 254) / 255;

			// all packets will have a 255-byte payload except the last one
			uint32_t lastPacketDC = m_DC % 255;

			// each packet has a 7-byte "wrapper" + the payload size
			packetSize =  ((numPackets - 1) * (255 + AJAAncillaryDataWrapperSize))	// all packets except the last one have a payload of 255 bytes
						+ (lastPacketDC + AJAAncillaryDataWrapperSize);			// the last packet carries the remainder
		}
	}
	else	// huh? (coding not set)
	{
		packetSize = 0;
		status = AJA_STATUS_FAIL;
	}
	
	return status;
}


//**********
// Generates "raw" ancillary data from the internal ancillary data (playback)

AJAStatus AJAAncillaryData::GenerateTransmitData (uint8_t * pData, const uint32_t maxData, uint32_t & packetSize)
{
	AJAStatus status = AJA_STATUS_SUCCESS;

	// make sure the caller has allocated enough space to hold what we're going to generate
	uint32_t myPacketSize;
	GetRawPacketSize(myPacketSize);

	if (myPacketSize > maxData || myPacketSize == 0)
	{
		packetSize = 0;		// it won't fit: generate no data and return zero size
		return AJA_STATUS_FAIL;
	}

	if (m_coding == AJAAncillaryDataCoding_Digital)
	{
		pData[0] = GetHeaderByte1();
		pData[1] = GetHeaderByte2();
		pData[2] = GetHeaderByte3();

		pData[3] = m_DID;
		pData[4] = m_SID;

		uint8_t payloadSize = (m_DC > 255) ? 255 : m_DC;	// truncate payload to max 255 bytes
		pData[5] = payloadSize;	

		// copy payload data to raw stream
		status = GetPayloadData(&pData[6], payloadSize);

		// note: the hardware automatically recalculates the checksum anyway, so this byte
		// is ignored. However, if the original source had a checksum we'll send it back...
//		pData[6+payloadSize] = checksum;
		pData[6+payloadSize] = Calculate8BitChecksum();

		// all done!
		packetSize = myPacketSize;
	}
	else if (m_coding == AJAAncillaryDataCoding_Raw)
	{
		// "analog" or "raw" ancillary data is special in that it may generate multiple output packets,
		// depending on the length of the payload data.
		// NOTE: we're assuming that zero-length payloads have already been screened out (above)!
		uint32_t numPackets = (m_DC + 254) / 255;

		// all packets will have a 255-byte payload except the last one
		//Note: Unused --  uint32_t lastPacketDC = m_DC % 255;

		uint8_t *payloadPtr = m_pPayload;
		uint32_t remainingPayloadData = m_DC;

		for (uint32_t i = 0; i < numPackets; i++)
		{
			pData[0] = GetHeaderByte1();
			pData[1] = GetHeaderByte2();
			pData[2] = GetHeaderByte3();

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
		packetSize = myPacketSize;
	}
	else	// huh? coding param not set - don't generate anything
	{
		packetSize = 0;
		status = AJA_STATUS_FAIL;
	}

	return status;
}


uint8_t AJAAncillaryData::GetHeaderByte2 (void) const
{
	uint8_t result = 0;

	if (true)
		result |= 0x80;		// LE bit is always active

	if (m_coding == AJAAncillaryDataCoding_Raw)
		result |= 0x40;		// analog/raw (1) or digital (0) ancillary data

	if (m_location.stream == AJAAncillaryDataVideoStream_Y)
		result |= 0x20;		// carried in Y (1) or C stream

	if (m_location.ancSpace == AJAAncillaryDataSpace_HANC)
		result |= 0x10;

	// ms 4 bits of line number
	result |= (m_location.lineNum >> 7) & 0x0F;	// ms 4 bits [10:7] of line number

	return result;
}


uint8_t AJAAncillaryData::GetHeaderByte3 (void) const
{
	uint8_t result = m_location.lineNum & 0x7F;	// ls 7 bits [6:0] of line number
	return result;
}


static const string		gEmptyString;


const string & AJAAncillaryDataLinkToString (const AJAAncillaryDataLink inValue, const bool inCompact)
{
	static const string		gAncDataLinkToStr []			= {"A", "B", "?"};
	static const string		gDAncDataLinkToStr []			= {"AJAAncillaryDataLink_A", "AJAAncillaryDataLink_B", "AJAAncillaryDataLink_Unknown"};

	return IS_VALID_AJAAncillaryDataLink (inValue) ? (inCompact ? gAncDataLinkToStr [inValue] : gDAncDataLinkToStr [inValue]) : gEmptyString;
}


const string & AJAAncillaryDataVideoStreamToString (const AJAAncillaryDataVideoStream inValue, const bool inCompact)
{
	static const string		gAncDataVideoStreamToStr []		= {"Chroma", "Luma", ""};
	static const string		gDAncDataVideoStreamToStr []	= {"AJAAncillaryDataVideoStream_C", "AJAAncillaryDataVideoStream_Y", "AJAAncillaryDataVideoStream_Unknown"};

	return IS_VALID_AJAAncillaryDataVideoStream (inValue) ? (inCompact ? gAncDataVideoStreamToStr [inValue] : gDAncDataVideoStreamToStr [inValue]) : gEmptyString;
}


const string & AJAAncillaryDataSpaceToString (const AJAAncillaryDataSpace inValue, const bool inCompact)
{
	static const string		gAncDataSpaceToStr []			= {"VANC", "HANC", "?"};
	static const string		gDAncDataSpaceToStr []			= {"AJAAncillaryDataSpace_VANC", "AJAAncillaryDataSpace_HANC", "AJAAncillaryDataSpace_Unknown"};

	return IS_VALID_AJAAncillaryDataSpace (inValue) ? (inCompact ? gAncDataSpaceToStr [inValue] : gDAncDataSpaceToStr [inValue]) : gEmptyString;
}


string AJAAncillaryDataLocationToString (const AJAAncillaryDataLocation & inValue, const bool inCompact)
{
	ostringstream	oss;
	oss	<< ::AJAAncillaryDataLinkToString (inValue.link, inCompact) << "|" << ::AJAAncillaryDataVideoStreamToString (inValue.stream, inCompact)
		<< "|" << ::AJAAncillaryDataSpaceToString (inValue.ancSpace, inCompact) << "|" << inValue.lineNum;
	return oss.str ();
}

ostream & operator << (ostream & inOutStream, const AJAAncillaryDataLocation & inValue)
{
	inOutStream	<< "Data Link:\t" << ::AJAAncillaryDataLinkToString (inValue.link) << endl
				<< "Video Stream:\t" << ::AJAAncillaryDataVideoStreamToString (inValue.stream) << endl
				<< "Data Space:\t" << ::AJAAncillaryDataSpaceToString (inValue.ancSpace) << endl
				<< "Line Number:\t" << inValue.lineNum;
	return inOutStream;
}


const string & AJAAncillaryDataCodingToString (const AJAAncillaryDataCoding inValue, const bool inCompact)
{
	static const string		gAncDataCodingToStr []			= {"Digital", "Analog/Raw", ""};
	static const string		gDAncDataCodingToStr []			= {"AJAAncillaryDataCoding_Digital", "AJAAncillaryDataCoding_Raw", "AJAAncillaryDataCoding_Unknown"};

	return IS_VALID_AJAAncillaryDataCoding (inValue) ? (inCompact ? gAncDataCodingToStr [inValue] : gDAncDataCodingToStr [inValue]) : gEmptyString;
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

	return inValue < AJAAncillaryDataType_Size ? (inCompact ? gAncDataTypeToStr [inValue] : gDAncDataTypeToStr [inValue]) : gEmptyString;
}


ostream & AJAAncillaryData::Print (ostream & inOutStream, const bool inDumpPayload) const
{
	inOutStream << "Type:\t\t"	<< AJAAncillaryData::DIDSIDToString (m_DID, m_SID) << endl	//	::AJAAncillaryDataTypeToString (GetAncillaryDataType ()) << endl
				<< "DID:\t\t0x"	<< hex << setfill ('0') << setw (2) << uint32_t (m_DID) << endl
				<< "SID:\t\t0x"	<< hex << setfill ('0') << setw (2) << uint32_t (m_SID) << endl
				<< "DC:\t\t"	<< dec << m_DC << endl
				<< "CS:\t\t0x"	<< hex << setfill ('0') << setw (2) << uint32_t (m_checksum) << dec << endl
				<< m_location	<< endl
				<< "Coding:\t\t"<< ::AJAAncillaryDataCodingToString (m_coding) << endl
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
	if (inDumpMaxBytes  &&  m_pPayload)
	{
		uint16_t	byteCount	= uint16_t(GetPayloadByteCount());
		oss << byteCount << " bytes: ";
		if (inDumpMaxBytes > byteCount)
			inDumpMaxBytes = byteCount;
		for (uint16_t ndx(0);  ndx < inDumpMaxBytes;  ndx++)
			oss << HEX0N(uint16_t(m_pPayload[ndx]),2);
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
	if (m_pPayload)
	{
		const int32_t	kBytesPerLine	(32);
		int32_t			count			(m_DC);
		uint8_t *		pData			(m_pPayload);

		while (count > 0)
		{
			const uint8_t	numBytes	((count >= kBytesPerLine) ? kBytesPerLine : count);
			inOutStream << ((pData == m_pPayload) ? "Payload:  " : "          ");
			for (uint8_t num (0);  num < numBytes;  num++)
			{
				inOutStream << " " << hex << setfill ('0') << setw (2) << uint32_t (pData [num]);
				if (num % 4 == 3)
					inOutStream << " ";		// an extra space every four bytes for readability
			}
			inOutStream << endl;
			pData += numBytes;
			count -= numBytes;
		}	//	loop til break
	}
	else
		inOutStream	<< "(NULL payload)" << endl;
	return inOutStream;
}


bool AJAAncillaryData::operator == (const AJAAncillaryData & inRHS) const
{
	if (GetDID() == inRHS.GetDID()
		&&  GetSID() == inRHS.GetSID()
		&&  GetPayloadByteCount() == inRHS.GetPayloadByteCount()
		&&  GetChecksum() == inRHS.GetChecksum()
		&&  GetDataLocation() == inRHS.GetDataLocation()
		&&  GetDataCoding() == inRHS.GetDataCoding())
		{
			if (m_pPayload  &&  inRHS.m_pPayload  &&  ::memcmp (m_pPayload, inRHS.m_pPayload, GetPayloadByteCount()) == 0)
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
		case 0x46:	if (inSID == 0x01)
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
