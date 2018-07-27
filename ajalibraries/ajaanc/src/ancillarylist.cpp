/**
	@file		ancillarylist.cpp
	@brief		Implementation of the AJAAncillaryList class.
	@copyright	(C) 2010-2018 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#include "ancillarylist.h"
#include "ancillarydatafactory.h"
#include "ajabase/system/debug.h"	//	This makes 'ajaanc' dependent upon 'ajabase'
#include "ajantv2/includes/ntv2utils.h"	//	This makes 'ajaanc' dependent upon 'ajantv2'
#if defined (AJALinux)
	#include <string.h>		//	For memcpy
#endif	//	AJALinux

using namespace std;

#define	LOGMYERROR(__x__)	AJA_sREPORT(AJA_DebugUnit_AJAAncList, AJA_DebugSeverity_Error,		__FUNCTION__ << ":  " << __x__)
#define	LOGMYWARN(__x__)	AJA_sREPORT(AJA_DebugUnit_AJAAncList, AJA_DebugSeverity_Warning,	__FUNCTION__ << ":  " << __x__)
#define	LOGMYNOTE(__x__)	AJA_sREPORT(AJA_DebugUnit_AJAAncList, AJA_DebugSeverity_Notice,		__FUNCTION__ << ":  " << __x__)
#define	LOGMYINFO(__x__)	AJA_sREPORT(AJA_DebugUnit_AJAAncList, AJA_DebugSeverity_Info,		__FUNCTION__ << ":  " << __x__)
#define	LOGMYDEBUG(__x__)	AJA_sREPORT(AJA_DebugUnit_AJAAncList, AJA_DebugSeverity_Debug,		__FUNCTION__ << ":  " << __x__)

#if defined(AJAHostIsBigEndian)
	//	Host is BigEndian (BE)
	#define AJA_ENDIAN_16NtoH(__val__)		(__val__)
	#define AJA_ENDIAN_16HtoN(__val__)		(__val__)
	#define AJA_ENDIAN_32NtoH(__val__)		(__val__)
	#define AJA_ENDIAN_32HtoN(__val__)		(__val__)
	#define AJA_ENDIAN_64NtoH(__val__)		(__val__)
	#define AJA_ENDIAN_64HtoN(__val__)		(__val__)
#else
	//	Host is LittleEndian (LE)
	#define AJA_ENDIAN_16NtoH(__val__)		AJA_ENDIAN_SWAP16(__val__)
	#define AJA_ENDIAN_16HtoN(__val__)		AJA_ENDIAN_SWAP16(__val__)
	#define AJA_ENDIAN_32NtoH(__val__)		AJA_ENDIAN_SWAP32(__val__)
	#define AJA_ENDIAN_32HtoN(__val__)		AJA_ENDIAN_SWAP32(__val__)
	#define AJA_ENDIAN_64NtoH(__val__)		AJA_ENDIAN_SWAP64(__val__)
	#define AJA_ENDIAN_64HtoN(__val__)		AJA_ENDIAN_SWAP64(__val__)
#endif


static bool	gIncludeZeroLengthPackets	(false);


AJAAncillaryList::AJAAncillaryList ()
{
	Clear ();
	SetAnalogAncillaryDataTypeForLine (20, AJAAncillaryDataType_Cea608_Line21);
	SetAnalogAncillaryDataTypeForLine (21, AJAAncillaryDataType_Cea608_Line21);
	SetAnalogAncillaryDataTypeForLine (22, AJAAncillaryDataType_Cea608_Line21);
	SetAnalogAncillaryDataTypeForLine (283, AJAAncillaryDataType_Cea608_Line21);
	SetAnalogAncillaryDataTypeForLine (284, AJAAncillaryDataType_Cea608_Line21);
	SetAnalogAncillaryDataTypeForLine (285, AJAAncillaryDataType_Cea608_Line21);

}


AJAAncillaryList::~AJAAncillaryList ()
{
	Clear ();
}


AJAAncillaryList & AJAAncillaryList::operator = (const AJAAncillaryList & inRHS)
{
	if (this != &inRHS)
	{
		Clear ();
		for (AJAAncDataListConstIter it (inRHS.m_ancList.begin ());  it != inRHS.m_ancList.end ();  ++it)
			if (*it)
				AddAncillaryData ((*it)->Clone ());
	}
	return *this;
}


AJAAncillaryData * AJAAncillaryList::GetAncillaryDataAtIndex (const uint32_t inIndex) const
{
	AJAAncillaryData *	pAncData(NULL);

	if (!m_ancList.empty()  &&  inIndex < m_ancList.size())
	{
		AJAAncDataListConstIter	it	(m_ancList.begin());

		for (uint32_t i(0);  i < inIndex;  i++)	//	Dang, std::list has no random access
			++it;
		pAncData = *it;
	}

	return pAncData;
}


AJAStatus AJAAncillaryList::ParseAllAncillaryData (void)
{
	AJAStatus result = AJA_STATUS_SUCCESS;

	for (AJAAncDataListConstIter it(m_ancList.begin());  it != m_ancList.end();  ++it)
	{
		AJAAncillaryData *	pAncData = *it;
		AJAStatus status = pAncData->ParsePayloadData ();

		//	Parse ALL items (even if one fails), but only return success if ALL succeed...
		if (AJA_FAILURE (status))
			result = status;
	}

	return result;
}


uint32_t AJAAncillaryList::CountAncillaryDataWithType (const AJAAncillaryDataType matchType) const
{
	uint32_t count = 0;

	for (AJAAncDataListConstIter it(m_ancList.begin());  it != m_ancList.end();  ++it)
	{
		AJAAncillaryData *		pAncData	(*it);
		AJAAncillaryDataType	ancType		(pAncData->GetAncillaryDataType());

		if (matchType == ancType)
			count++;
	}

	return count;
}


AJAAncillaryData * AJAAncillaryList::GetAncillaryDataWithType (const AJAAncillaryDataType matchType, const uint32_t index) const
{
	AJAAncillaryData *	pResult	(NULL);
	uint32_t count = 0;

	for (AJAAncDataListConstIter it(m_ancList.begin());  it != m_ancList.end();  ++it)
	{
		AJAAncillaryData *		pAncData (*it);
		AJAAncillaryDataType	ancType  (pAncData->GetAncillaryDataType());

		if (matchType == ancType)
		{
			if (index == count)
			{
				pResult = pAncData;
				break;
			}
			else
				count++;
		}
	}
	return pResult;
}


uint32_t AJAAncillaryList::CountAncillaryDataWithID (const uint8_t DID, const uint8_t SID) const
{
	uint32_t count = 0;

	for (AJAAncDataListConstIter it(m_ancList.begin());  it != m_ancList.end();  ++it)
	{
		AJAAncillaryData *	pAncData = *it;

		if (DID == AJAAncillaryDataWildcard_DID  ||  DID == pAncData->GetDID())
		{
			if (SID == AJAAncillaryDataWildcard_SID  ||  SID == pAncData->GetSID())
				count++;
		}
	}

	return count;
}


AJAAncillaryData * AJAAncillaryList::GetAncillaryDataWithID (const uint8_t DID, const uint8_t SID, const uint32_t index) const
{
	AJAAncillaryData *	pResult	(NULL);
	uint32_t count = 0;

	for (AJAAncDataListConstIter it(m_ancList.begin());  it != m_ancList.end();  ++it)
	{
		AJAAncillaryData *	pAncData = *it;

		// Note: Unused
		//AJAAncillaryDataType ancType = pAncData->GetAncillaryDataType();

		if (DID == AJAAncillaryDataWildcard_DID  ||  DID == pAncData->GetDID())
		{
			if (SID == AJAAncillaryDataWildcard_SID  ||  SID == pAncData->GetSID())
			{
				if (index == count)
				{
					pResult = pAncData;
					break;
				}
				else
					count++;
			}
		}
	}

	return pResult;
}


AJAStatus AJAAncillaryList::AddAncillaryData (const AJAAncillaryData * pInAncData)
{
	if (!pInAncData)
		return AJA_STATUS_NULL;

	// Note: this makes a clone of the incoming AJAAncillaryData object and
	// saves it in the List. The caller is responsible for deleting the original,
	// and the List owns the clone.
	const bool			wasEmpty	(m_ancList.empty());
	AJAAncillaryData *	pData		(pInAncData->Clone());
	if (!pData)
		return AJA_STATUS_FAIL;

	// add it to the list
	if (pData)
		m_ancList.push_back(pData);

	LOGMYDEBUG(DEC(m_ancList.size()) << " packet(s) stored" << (wasEmpty ? " from" : " after appending") << " packet " << pData->AsString(32));
	return AJA_STATUS_SUCCESS;
}


AJAStatus AJAAncillaryList::Clear (void)
{
	uint32_t		numDeleted	(0);
	const uint32_t	oldSize		(uint32_t(m_ancList.size()));
	for (AJAAncDataListConstIter it (m_ancList.begin());  it != m_ancList.end();  ++it)
	{
		AJAAncillaryData *	pAncData(*it);
		if (pAncData)
		{
			delete (pAncData);
			numDeleted++;
		}
	}

	m_ancList.clear();
	if (oldSize || numDeleted)
		LOGMYDEBUG(numDeleted << " packet(s) deleted -- list emptied");
	return AJA_STATUS_SUCCESS;
}


AJAStatus AJAAncillaryList::RemoveAncillaryData (AJAAncillaryData * pAncData)
{
	if (!pAncData)
		return AJA_STATUS_NULL;

	m_ancList.remove(pAncData);	//	note:	there's no feedback as to whether one or more elements existed or not
								//	note:	pAncData is NOT deleted!
	LOGMYDEBUG(DEC(m_ancList.size()) << " packet(s) remain after removing packet " << pAncData->AsString(32));
	return AJA_STATUS_SUCCESS;
}


AJAStatus AJAAncillaryList::DeleteAncillaryData (AJAAncillaryData * pAncData)
{
	if (!pAncData)
		return AJA_STATUS_NULL;

	AJAStatus status (RemoveAncillaryData(pAncData));
	if (AJA_SUCCESS(status))
		delete pAncData;
	return status;
}


//	Sort Predicates

static bool SortByDID (AJAAncillaryData * lhs, AJAAncillaryData * rhs)
{
  return lhs->GetDID() < rhs->GetDID();
}


static bool SortBySID (AJAAncillaryData * lhs, AJAAncillaryData * rhs)
{
	return lhs->GetSID () < rhs->GetSID ();
}

static bool SortByLocation (AJAAncillaryData * lhs, AJAAncillaryData * rhs)
{
	bool bResult = false;

	//	Sort by line number...
	if (lhs->GetLocationLineNumber () < rhs->GetLocationLineNumber ())
		bResult = true;
	else if (lhs->GetLocationLineNumber () == rhs->GetLocationLineNumber ())
	{
		//	Same line number -- sort by HANC vs. VANC...
		if ( (lhs->GetLocationVideoSpace () == AJAAncillaryDataSpace_HANC)  &&  (rhs->GetLocationVideoSpace () == AJAAncillaryDataSpace_VANC))
			bResult = true;
		else if (lhs->GetLocationVideoSpace () == rhs->GetLocationVideoSpace ())
		{
			//	Same line, same ANC space -- let's do Y before C...
			if ( (lhs->GetLocationDataChannel() == AJAAncillaryDataChannel_Y)  &&  (rhs->GetLocationDataChannel() == AJAAncillaryDataChannel_C))
				bResult = true;
		}
	}
	return bResult;
}


//	Sort Methods

AJAStatus AJAAncillaryList::SortListByDID (void)
{
	m_ancList.sort (SortByDID);
	return AJA_STATUS_SUCCESS;
}


AJAStatus AJAAncillaryList::SortListBySID (void)
{
	m_ancList.sort (SortBySID);
	return AJA_STATUS_SUCCESS;
}

AJAStatus AJAAncillaryList::SortListByLocation (void)
{
	m_ancList.sort (SortByLocation);
	return AJA_STATUS_SUCCESS;
}


AJAStatus AJAAncillaryList::Compare (const AJAAncillaryList & inCompareList, const bool inIgnoreLocation, const bool inIgnoreChecksum) const
{
	if (inCompareList.CountAncillaryData() != CountAncillaryData())
		return AJA_STATUS_FAIL;
	for (uint32_t ndx (0);  ndx < CountAncillaryData();  ndx++)
	{
		AJAAncillaryData *	pPktA	(inCompareList.GetAncillaryDataAtIndex(ndx));
		AJAAncillaryData *	pPktB	(inCompareList.GetAncillaryDataAtIndex(ndx));
		if (AJA_FAILURE(pPktA->Compare(*pPktB, inIgnoreLocation, inIgnoreChecksum)))
			return AJA_STATUS_FAIL;
	}	//	for each packet
	return AJA_STATUS_SUCCESS;
}


string AJAAncillaryList::CompareWithInfo (const AJAAncillaryList & inCompareList, const bool inIgnoreLocation,  const bool inIgnoreChecksum) const
{
	ostringstream	oss;
	if (inCompareList.CountAncillaryData() != CountAncillaryData())
		{oss << "Packet count mismatch: " << DEC(CountAncillaryData()) << " vs " << DEC(inCompareList.CountAncillaryData());  return oss.str();}

	for (uint32_t ndx (0);  ndx < CountAncillaryData();  ndx++)
	{
		AJAAncillaryData *	pPktA	(inCompareList.GetAncillaryDataAtIndex(ndx));
		AJAAncillaryData *	pPktB	(inCompareList.GetAncillaryDataAtIndex(ndx));
		const string		info	(pPktA->CompareWithInfo(*pPktB, inIgnoreLocation, inIgnoreChecksum));
		if (!info.empty())
			return info;
	}	//	for each packet
	return string();
}


static bool TestForAnalogContinuation (AJAAncillaryData * pPrevData, AJAAncillaryData * pNewData)
{
	if (pPrevData == NULL || pNewData == NULL)
		return false;

	bool bResult = false;

	//	Get coding type and location for the previous ancillary packet...
	AJAAncillaryDataCoding		prevCoding	(pPrevData->GetDataCoding ());
	AJAAncillaryDataLocation	prevLoc		(pPrevData->GetDataLocation ());

	//	Get coding type and location for the new ancillary packet...
	AJAAncillaryDataCoding		newCoding	(pNewData->GetDataCoding ());
	AJAAncillaryDataLocation	newLoc		(pNewData->GetDataLocation ());

	//	Compare...
	if (   (prevCoding == AJAAncillaryDataCoding_Analog)
		&& (newCoding  == AJAAncillaryDataCoding_Analog)
		&& (prevLoc.GetLineNumber()  == newLoc.GetLineNumber())
		&& (prevLoc.GetHorizontalOffset()  == newLoc.GetHorizontalOffset())	// technically, these should ALWAYS be the same for "analog"...?
		&& (prevLoc.GetDataSpace()   == newLoc.GetDataSpace())		//
		&& (prevLoc.GetDataLink()    == newLoc.GetDataLink())		//
		&& (prevLoc.GetDataStream()  == newLoc.GetDataStream())		//
		&& (prevLoc.GetDataChannel() == newLoc.GetDataChannel()) )	//
	{
		bResult = true;
	}

	return bResult;
}


/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////	R E C E I V E
/////////////////////////////////////////////////////////////////

// Parse a stream of "raw" ancillary data as collected by an AJAAncExtractorWidget.
// Break the stream into separate AJAAncillaryData objects and add them to the list.
//
AJAStatus AJAAncillaryList::AddReceivedAncillaryData (const uint8_t * pRcvData, const uint32_t dataSize)
{
	AJAStatus	status	(AJA_STATUS_SUCCESS);

	if (pRcvData == NULL || dataSize == 0)
		return AJA_STATUS_NULL;

	//	Use this as an uninitialized template...
	AJAAncillaryData			newAncData;
	AJAAncillaryDataLocation	defaultLoc		(AJAAncillaryDataLink_A, AJAAncillaryDataChannel_Y, AJAAncillaryDataSpace_VANC, 9);
	int32_t						remainingSize	(static_cast <int32_t> (dataSize));
	const uint8_t *				pInputData		(pRcvData);
	bool						bMoreData		(true);
	AJAAncillaryDataFactory		factory;

	while (bMoreData)
	{
		bool					bInsertNew	(false);		//	We'll set this 'true' if/when we find a new Anc packet to insert
		AJAAncillaryDataType	newAncType	(AJAAncillaryDataType_Unknown);	//	We'll set this to the proper type once we know it
		uint32_t				packetSize	(0);			//	This is where the AncillaryData object returns the number of bytes that were "consumed" from the input stream

		//	Reset the AncData object, then load itself from the next GUMP packet...
		newAncData.Clear();
		status = newAncData.InitWithReceivedData (pInputData, remainingSize, defaultLoc, packetSize);
		if (AJA_FAILURE(status))
		{
			//	TODO:	Someday, let's try to recover and process subsequent packets.
			break;		//	NOTE:	For now, bail on errors in the GUMP stream
		}
		else if (packetSize == 0)
			break;		//	Nothing to do

		//	Determine what type of anc data we have, and create an object of the appropriate class...
		if (newAncData.GetDataCoding() == AJAAncillaryDataCoding_Digital)
		{
			//	Digital anc packets are fairly easy to categorize: you just have to look at their DID/SID.
			//	Also, they are (by definition) independent packets which become independent AJAAncillaryData objects.
			newAncType = factory.GuessAncillaryDataType(&newAncData);
			bInsertNew = true;		//	Add it to the list
		}	// digital anc data
		else if (newAncData.GetDataCoding() == AJAAncillaryDataCoding_Analog)
		{
			//	"Analog" packets are trickier... First, digitized analog lines are broken into multiple
			//	packets by the hardware, which need to be recombined into a single AJAAncillaryData object.
			//	Second, it is harder to classify the data type, since there is no single easy-to-read
			//	ID. Instead, we rely on the user to tell us what lines are expected to contain what kind
			//	of data (e.g. "expect line 21 to carry 608 captioning...").

			//	First, see if the LAST AJAAncillaryData object came from analog space and the same
			//	location as this new one. If so, we're going to assume that the new data is simply a
			//	continuation of the previous packet, and append the payload data accordingly.
			bool bAppendAnalog = false;

			if (!m_ancList.empty ())
			{
				AJAAncillaryData *	pPrevData		(m_ancList.back ());
				bool				bAnlgContinue	(TestForAnalogContinuation (pPrevData, &newAncData));

				if (bAnlgContinue)
				{
					//	The new data is just a continuation of the previous packet: simply append the
					//	new payload data to the previous payload...
					pPrevData->AppendPayload (newAncData);
					bAppendAnalog = true;
				}
			}

			if (!bAppendAnalog)
			{
				//	If this is NOT a "continuation" packet, then this is a new analog packet. see if the
				//	user has specified an expected Anc data type that matches the location of the new data...
				newAncType = GetAnalogAncillaryDataType (&newAncData);
				bInsertNew = true;
			}
		}	// analog anc data
//			else 
//				Anc Coding Unknown????	(ignore it)

		if (bInsertNew)
		{
			//	Create an AJAAncillaryData object of the appropriate type, and init it with our raw data...
			AJAAncillaryData *	pData	(factory.Create (newAncType, &newAncData));
			if (pData)
				m_ancList.push_back(pData);		//	Add it to my list
		}

		remainingSize -= packetSize;		//	Decrease the remaining data size by the amount we just "consumed"
		pInputData += packetSize;			//	Advance the input data pointer by the same amount
		if (remainingSize <= 0)				//	All of the input data consumed?
			bMoreData = false;
	}	// while (bMoreData)

	return status;

}	//	AddReceivedAncillaryData


//	Parse a "raw" RTP packet received from hardware (ingest) in network byte order into separate
//	AJAAncillaryData objects and append them to me.
AJAStatus AJAAncillaryList::AddReceivedAncillaryData (const vector<uint32_t> & inReceivedData)
{
	AJAStatus	status	(AJA_STATUS_SUCCESS);
	if (inReceivedData.empty())
		{LOGMYWARN("Empty RTP data vector");  return AJA_STATUS_SUCCESS;}

LOGMYDEBUG(PrintULWordsBE(inReceivedData));

	//	Crack open the RTP packet header...
	AJARTPAncPayloadHeader	RTPheader;
	if (!RTPheader.ReadULWordVector(inReceivedData))
		{LOGMYERROR("AJARTPAncPayloadHeader::ReadULWordVector failed, " << DEC(4*inReceivedData.size()) << " header bytes");  return AJA_STATUS_FAIL;}
	if (RTPheader.IsNULL())
		{LOGMYWARN("No anc packets added: NULL RTP header: " << RTPheader);  return AJA_STATUS_SUCCESS;}	//	Not an error
	if (!RTPheader.IsValid())
		{LOGMYWARN("RTP header invalid: " << RTPheader);  return AJA_STATUS_FAIL;}

	const size_t	numBytes		(RTPheader.GetPacketLength());
	const uint32_t	numPackets		(RTPheader.GetAncPacketCount());
	const size_t	actualBytes		(inReceivedData.size() * 4 - RTPheader.GetHeaderByteCount());

	if (actualBytes < numBytes)
		{LOGMYERROR("RTP header says " << DEC(numBytes) << "-byte pkt, but only given " << DEC(actualBytes) << " bytes: " << RTPheader);  return AJA_STATUS_BADBUFFERCOUNT;}
	//else if (actualBytes != numBytes) LOGMYWARN("RTP header says " << DEC(numBytes) << "-byte pkt, but given " << DEC(actualBytes) << " bytes: " << RTPheader);
	if (!numPackets)
		{LOGMYWARN("No Anc packets to append: " << RTPheader);  return AJA_STATUS_SUCCESS;}

LOGMYDEBUG(RTPheader);

	uint16_t	u32Ndx	(5);	//	First Anc packet starts at ULWord[5]
	unsigned	pktNum	(0);
	for (;  pktNum < numPackets  &&  AJA_SUCCESS(status);  pktNum++)
	{
		AJAAncillaryData	pkt;
		status = pkt.InitWithReceivedData(inReceivedData, u32Ndx);
		if (AJA_FAILURE(status))
			continue;
		status = AddAncillaryData(pkt);
	}	//	for each anc packet
	if (AJA_FAILURE(status))
		LOGMYERROR(::AJAStatusToString(status) << ": Failed at pkt " << DEC(pktNum) << " of " << DEC(numPackets));
	if (CountAncillaryData() < numPackets)
		LOGMYWARN(DEC(CountAncillaryData()) << " of " << DEC(numPackets) << " anc packet(s) decoded from RTP packet");
	else
		LOGMYINFO(DEC(numPackets) << " added from RTP packet: " << *this);
	return status;
}	//	AddReceivedAncillaryData


static AJAStatus AppendUWordPacketToGump (	vector<uint8_t> &				outGumpPkt,
											const vector<uint16_t> &		inPacketWords,
											const AJAAncillaryDataLocation	inLoc = AJAAncillaryDataLocation(AJAAncillaryDataLink_A,
																											AJAAncillaryDataChannel_Y,
																											AJAAncillaryDataSpace_VANC,
																											0))
{
	AJAStatus	status	(AJA_STATUS_SUCCESS);

	if (inPacketWords.size () < 7)
		return AJA_STATUS_RANGE;
//	NOTE: Tough call. Decided not to validate the AJAAncillaryDataLocation here:
//	if (!inLoc.IsValid())
//		return AJA_STATUS_BAD_PARAM;

	//	Use this as an uninitialized template...
	vector<uint16_t>::const_iterator	iter	(inPacketWords.begin());

	//	Anc packet must start with 0x000 0x3FF 0x3FF sequence...
	if (*iter != 0x0000)
		return AJA_STATUS_FAIL;
	++iter;
	if (*iter != 0x03FF)
		return AJA_STATUS_FAIL;
	++iter;
	if (*iter != 0x03FF)
		return AJA_STATUS_FAIL;
	++iter;	//	Now pointing at DID

	outGumpPkt.reserve (outGumpPkt.size() + inPacketWords.size());	//	Expand outGumpPkt's capacity in one step
	const vector<uint8_t>::size_type	dataByte1Ndx	(outGumpPkt.size()+1);	//	Index of byte[1] in outgoing Gump packet
	outGumpPkt.push_back(0xFF);											//	[0]	First byte always 0xFF
	outGumpPkt.push_back(0x80);											//	[1]	Location data byte 1:	"location valid" bit always set
	outGumpPkt[dataByte1Ndx] |= (inLoc.GetLineNumber() >> 7) & 0x0F;	//	[1]	Location data byte 1:	LS 4 bits == MS 4 bits of 11-bit line number
	if (inLoc.IsLumaChannel())
		outGumpPkt[dataByte1Ndx] |= 0x20;								//	[1]	Location data byte 1:	set Y/C bit for Y/luma channel
	if (inLoc.IsHanc())
		outGumpPkt[dataByte1Ndx] |= 0x10;								//	[1]	Location data byte 1:	set H/V bit for HANC
	outGumpPkt.push_back (inLoc.GetLineNumber() & 0x7F);				//	[2]	Location data byte 2:	MSB reserved; LS 7 bits == LS 7 bits of 11-bit line number

	while (iter != inPacketWords.end())
	{
		outGumpPkt.push_back(*iter & 0xFF);	//	Mask off upper byte
		++iter;
	}
	return status;
}


AJAStatus AJAAncillaryList::AddVANCData (const vector<uint16_t> & inPacketWords, const AJAAncillaryDataLocation & inLocation)
{
	vector<uint8_t>		gumpPacketData;
	AJAStatus	status	(AppendUWordPacketToGump (gumpPacketData,  inPacketWords, inLocation));
	if (AJA_FAILURE(status))
		return status;

	AJAAncillaryData	newAncData;
	status = newAncData.InitWithReceivedData (gumpPacketData, inLocation);
	if (AJA_FAILURE(status))
		return status;

	AJAAncillaryDataFactory	factory;
	AJAAncillaryDataType	newAncType	(factory.GuessAncillaryDataType(&newAncData));
	AJAAncillaryData *		pData		(factory.Create (newAncType, &newAncData));
	if (!pData)
		return AJA_STATUS_FAIL;

	if (gIncludeZeroLengthPackets  ||  pData->GetDC())
	try
	{
		m_ancList.push_back(pData);		//	Append to my list
	}
	catch (...)
	{
		return AJA_STATUS_FAIL;
	}

	return AJA_STATUS_SUCCESS;

}	//	AddVANCData


#if !defined(NTV2_DEPRECATE_14_2)
	AJAStatus AJAAncillaryList::AddVANCData (const vector<uint16_t> & inPacketWords, const uint16_t inLineNum, const AJAAncillaryDataChannel inChannel)
	{
		return AddVANCData (inPacketWords, AJAAncillaryDataLocation (AJAAncillaryDataLink_A,  inChannel, AJAAncillaryDataSpace_VANC,  inLineNum));
	}	//	AddVANCData
#endif	//	!defined(NTV2_DEPRECATE_14_2)


AJAStatus AJAAncillaryList::SetFromVANCData (const NTV2_POINTER &			inFrameBuffer,
											const NTV2FormatDescriptor &	inFormatDesc,
											AJAAncillaryList &				outPackets)
{
	outPackets.Clear();

	if (inFrameBuffer.IsNULL())
	{
		LOGMYERROR("AJA_STATUS_NULL: NULL frame buffer pointer");
		return AJA_STATUS_NULL;
	}
	if (!inFormatDesc.IsValid())
	{
		LOGMYERROR("AJA_STATUS_BAD_PARAM: bad NTV2FormatDescriptor");
		return AJA_STATUS_BAD_PARAM;
	}
	if (!inFormatDesc.IsVANC())
	{
		LOGMYERROR("AJA_STATUS_BAD_PARAM: format descriptor has no VANC lines");
		return AJA_STATUS_BAD_PARAM;
	}

	const ULWord			vancBytes	(inFormatDesc.GetTotalRasterBytes() - inFormatDesc.GetVisibleRasterBytes());
	const NTV2PixelFormat	fbf			(inFormatDesc.GetPixelFormat());
	const bool				isSD		(NTV2_IS_SD_STANDARD(inFormatDesc.GetVideoStandard()));
	if (inFrameBuffer.GetByteCount() < vancBytes)
	{
		LOGMYERROR("AJA_STATUS_FAIL: " << inFrameBuffer.GetByteCount() << "-byte frame buffer smaller than " << vancBytes << "-byte VANC region");
		return AJA_STATUS_FAIL;
	}
	if (fbf != NTV2_FBF_10BIT_YCBCR  &&  fbf != NTV2_FBF_8BIT_YCBCR)
	{
		LOGMYERROR("AJA_STATUS_UNSUPPORTED: frame buffer format " << ::NTV2FrameBufferFormatToString(fbf) << " not '2vuy' nor 'v210'");
		return AJA_STATUS_UNSUPPORTED;	//	Only 'v210' and '2vuy' currently supported
	}

	for (ULWord lineOffset (0);  lineOffset < inFormatDesc.GetFirstActiveLine();  lineOffset++)
	{
		UWordSequence	uwords;
		bool			isF2			(false);
		ULWord			smpteLineNum	(0);
		unsigned		ndx				(0);

		inFormatDesc.GetSMPTELineNumber (lineOffset, smpteLineNum, isF2);
		if (fbf == NTV2_FBF_10BIT_YCBCR)
			::UnpackLine_10BitYUVtoUWordSequence (inFormatDesc.GetRowAddress(inFrameBuffer.GetHostAddress(0), lineOffset),
													inFormatDesc, uwords);
		else
			AJAAncillaryData::Unpack8BitYCbCrToU16sVANCLine (inFormatDesc.GetRowAddress(inFrameBuffer.GetHostAddress(0), lineOffset),
															uwords,  inFormatDesc.GetRasterWidth());
		if (isSD)
		{
			AJAAncillaryData::U16Packets	ycPackets;
			UWordSequence					ycHOffsets;
			AJAAncillaryDataLocation		loc	(AJAAncillaryDataLink_Unknown, AJAAncillaryDataChannel_Both, AJAAncillaryDataSpace_VANC, uint16_t(smpteLineNum));

			AJAAncillaryData::GetAncPacketsFromVANCLine (uwords, AncChannelSearch_Both, ycPackets, ycHOffsets);
			NTV2_ASSERT(ycPackets.size() == ycHOffsets.size());

			for (AJAAncillaryData::U16Packets::const_iterator it(ycPackets.begin());  it != ycPackets.end();  ++it, ndx++)
				outPackets.AddVANCData (*it, loc.SetHorizontalOffset(ycHOffsets[ndx]));
		}
		else
		{
			AJAAncillaryData::U16Packets	yPackets, cPackets;
			UWordSequence					yHOffsets, cHOffsets;
			AJAAncillaryDataLocation		yLoc	(AJAAncillaryDataLink_Unknown, AJAAncillaryDataChannel_Y, AJAAncillaryDataSpace_VANC, uint16_t(smpteLineNum));
			AJAAncillaryDataLocation		cLoc	(AJAAncillaryDataLink_Unknown, AJAAncillaryDataChannel_C, AJAAncillaryDataSpace_VANC, uint16_t(smpteLineNum));
			//cerr << endl << "SetFromVANCData: +" << DEC0N(lineOffset,2) << ": ";  inFormatDesc.PrintSMPTELineNumber(cerr, lineOffset);  cerr << ":" << endl << uwords << endl;
			AJAAncillaryData::GetAncPacketsFromVANCLine (uwords, AncChannelSearch_Y, yPackets, yHOffsets);
			AJAAncillaryData::GetAncPacketsFromVANCLine (uwords, AncChannelSearch_C, cPackets, cHOffsets);
			NTV2_ASSERT(yPackets.size() == yHOffsets.size());
			NTV2_ASSERT(cPackets.size() == cHOffsets.size());

			unsigned	ndxx(0);
			for (AJAAncillaryData::U16Packets::const_iterator it(yPackets.begin());  it != yPackets.end();  ++it, ndxx++)
				outPackets.AddVANCData (*it, yLoc.SetHorizontalOffset(yHOffsets[ndxx]));

			ndxx = 0;
			for (AJAAncillaryData::U16Packets::const_iterator it(cPackets.begin());  it != cPackets.end();  ++it, ndxx++)
				outPackets.AddVANCData (*it, cLoc.SetHorizontalOffset(cHOffsets[ndxx]));
		}
	}	//	for each VANC line
	LOGMYDEBUG("returning " << outPackets);
	return AJA_STATUS_SUCCESS;
}


AJAStatus AJAAncillaryList::SetFromSDIAncData (const NTV2_POINTER & inF1AncBuffer,
											const NTV2_POINTER & inF2AncBuffer,
											AJAAncillaryList & outPackets)
{
	AJAStatus	result	(AJA_STATUS_SUCCESS);
	outPackets.Clear();
	if (inF1AncBuffer.IsNULL())
		LOGMYWARN("Empty/null F1 RTP anc buffer");
	if (!inF1AncBuffer.IsNULL()  &&  AJA_SUCCESS(result))
		result = outPackets.AddReceivedAncillaryData (reinterpret_cast <const uint8_t *> (inF1AncBuffer.GetHostPointer()), inF1AncBuffer.GetByteCount());
	if (!inF2AncBuffer.IsNULL()  &&  AJA_SUCCESS(result))
		result = outPackets.AddReceivedAncillaryData (reinterpret_cast <const uint8_t *> (inF2AncBuffer.GetHostPointer()), inF1AncBuffer.GetByteCount());
	return result;
}


AJAStatus AJAAncillaryList::SetFromIPAncData (const NTV2_POINTER & inF1AncBuffer,
											const NTV2_POINTER & inF2AncBuffer,
											AJAAncillaryList & outPackets)
{
	AJAStatus	result	(AJA_STATUS_SUCCESS);
	outPackets.Clear();
	{
		vector<uint32_t>	F1U32s;

		if (inF1AncBuffer.IsNULL())
			LOGMYWARN("Empty/null F1 RTP anc buffer");
		else
		{
			//	Peek into the F1 packet header and discover its true length...
			AJARTPAncPayloadHeader	F1PayloadHdr;
			if (!F1PayloadHdr.ReadBuffer(inF1AncBuffer))
				{result = AJA_STATUS_MEMORY;  LOGMYERROR("F1AncBuffer " << inF1AncBuffer.AsString() << ": failed reading IP payload hdr, AJA_STATUS_MEMORY");}	//	Fail

			const size_t	F1ULWordCnt	((F1PayloadHdr.GetHeaderByteCount() + F1PayloadHdr.GetPacketLength()) / sizeof(uint32_t));

			//	Read F1ULWordCnt x 32-bit words from the F1 buffer...
			if (AJA_SUCCESS(result)  &&  !inF1AncBuffer.GetU32s(F1U32s, 0, F1ULWordCnt))
				{result = AJA_STATUS_MEMORY;  LOGMYERROR("F1AncBuffer " << inF1AncBuffer.AsString() << ": failed reading " << DEC(F1ULWordCnt) << " 32-bit words, AJA_STATUS_MEMORY");}	//	Fail
		}
		if (AJA_SUCCESS(result))
			result = outPackets.AddReceivedAncillaryData(F1U32s);
	}

	if (AJA_SUCCESS(result)  &&  !inF2AncBuffer.IsNULL())
	{
		vector<uint32_t>		F2U32s;
		AJARTPAncPayloadHeader	F2PayloadHdr;

		//	Peek into the F2 packet header and discover its true length...
		if (!F2PayloadHdr.ReadBuffer(inF2AncBuffer))
			{result = AJA_STATUS_MEMORY;  LOGMYERROR("F2AncBuffer " << inF2AncBuffer.AsString() << ": failed reading IP payload hdr, AJA_STATUS_MEMORY");}	//	Fail

		const size_t	F2ULWordCnt	((F2PayloadHdr.GetHeaderByteCount() + F2PayloadHdr.GetPacketLength()) / sizeof(uint32_t));

		//	Read F2ULWordCnt x 32-bit words from the F2 buffer...
		if (AJA_SUCCESS(result)  &&  !inF2AncBuffer.GetU32s(F2U32s, 0, F2ULWordCnt))
			{result = AJA_STATUS_MEMORY;  LOGMYERROR("F2AncBuffer " << inF2AncBuffer.AsString() << ": failed reading " << DEC(F2ULWordCnt) << " 32-bit words, AJA_STATUS_MEMORY");}	//	Fail
		if (AJA_SUCCESS(result)  &&  !F2U32s.empty())
			result = outPackets.AddReceivedAncillaryData(F2U32s);
	}
	return result;
}


AJAAncillaryDataType AJAAncillaryList::GetAnalogAncillaryDataType (AJAAncillaryData * pData)
{
	return pData ? GetAnalogAncillaryDataTypeForLine (pData->GetLocationLineNumber ()) : AJAAncillaryDataType_Unknown;
}


AJAStatus AJAAncillaryList::ClearAnalogAncillaryDataTypeMap (void)
{
	m_analogTypeMap.clear ();
	return AJA_STATUS_SUCCESS;
}


AJAStatus AJAAncillaryList::SetAnalogAncillaryDataTypeMap (const AJAAncillaryAnalogTypeMap & inMap)
{
	m_analogTypeMap = inMap;
	return AJA_STATUS_SUCCESS;
}


AJAStatus AJAAncillaryList::GetAnalogAncillaryDataTypeMap (AJAAncillaryAnalogTypeMap & outMap) const
{
	outMap = m_analogTypeMap;
	return AJA_STATUS_SUCCESS;
}


AJAStatus AJAAncillaryList::SetAnalogAncillaryDataTypeForLine (const uint16_t inLineNum, const AJAAncillaryDataType inAncType)
{
	m_analogTypeMap.erase(inLineNum);				//	In case someone has already set this line
	if (inAncType == AJAAncillaryDataType_Unknown)
		return AJA_STATUS_SUCCESS;					//	A non-entry is the same as AJAAncillaryDataType_Unknown
	else if (IS_VALID_AJAAncillaryDataType(inAncType))
			m_analogTypeMap[inLineNum] = inAncType;
	else
		return AJA_STATUS_BAD_PARAM;
	return AJA_STATUS_SUCCESS;
}


AJAAncillaryDataType AJAAncillaryList::GetAnalogAncillaryDataTypeForLine (const uint16_t inLineNum) const
{
	AJAAncillaryDataType	ancType	(AJAAncillaryDataType_Unknown);
	if (!m_analogTypeMap.empty ())
	{
		AJAAncillaryAnalogTypeMap::const_iterator	it	(m_analogTypeMap.find (inLineNum));
		if (it != m_analogTypeMap.end ())
			ancType = it->second;
	}
	return ancType;
}


///////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////	T R A N S M I T
///////////////////////////////////////////////////////////////////


AJAStatus AJAAncillaryList::GetAncillaryDataTransmitSize (const bool bProgressive, const uint32_t f2StartLine, uint32_t & ancSizeF1, uint32_t & ancSizeF2)
{
	AJAStatus	status	(AJA_STATUS_SUCCESS);
	uint32_t	f1Size	(0);
	uint32_t	f2Size	(0);

	for (AJAAncDataListConstIter it(m_ancList.begin());  it != m_ancList.end();  ++it)
	{
		AJAAncillaryData *	pAncData	(*it);
		uint32_t			packetSize	(0);
		status = pAncData->GetRawPacketSize (packetSize);
		if (status != AJA_STATUS_SUCCESS)
			break;

		if (bProgressive || pAncData->GetLocationLineNumber () < f2StartLine)
			f1Size += packetSize;
		else
			f2Size += packetSize;
	}

	ancSizeF1 = f1Size;
	ancSizeF2 = f2Size;
	return status;
}


AJAStatus AJAAncillaryList::GetAncillaryDataTransmitData (const bool bProgressive, const uint32_t f2StartLine, uint8_t * pF1AncData, const uint32_t inMaxF1Data, uint8_t * pF2AncData, const uint32_t inMaxF2Data)
{
	NTV2_POINTER	F1Buffer(pF1AncData, inMaxF1Data), F2Buffer(pF2AncData, inMaxF2Data);
	if (!F1Buffer.IsNULL())
		NTV2_ASSERT(F1Buffer.IsProvidedByClient());
	if (!F2Buffer.IsNULL())
		NTV2_ASSERT(F2Buffer.IsProvidedByClient());
	return GetSDITransmitData(F1Buffer, F2Buffer, bProgressive, f2StartLine);
}


AJAStatus AJAAncillaryList::GetSDITransmitData (NTV2_POINTER & F1Buffer, NTV2_POINTER & F2Buffer,
												const bool inIsProgressive, const uint32_t inF2StartLine)
{
	AJAStatus	status		(AJA_STATUS_SUCCESS);
	uint32_t	maxF1Data	(F1Buffer.GetByteCount());
	uint32_t	maxF2Data	(F2Buffer.GetByteCount());
	uint8_t *	pF1AncData	(reinterpret_cast<uint8_t*>(F1Buffer.GetHostPointer()));
	uint8_t *	pF2AncData	(reinterpret_cast<uint8_t*>(F2Buffer.GetHostPointer()));

	//	Generate transmit data for each of my packets...
	for (AJAAncDataListConstIter it(m_ancList.begin());  it != m_ancList.end();  ++it)
	{
		uint32_t			pktSize(0);
		AJAAncillaryData *	pPkt(*it);
		if (!pPkt)
			{status = AJA_STATUS_NULL;	break;}	//	Fail

		if (inIsProgressive || pPkt->GetLocationLineNumber() < inF2StartLine)
		{
			if (pF1AncData  &&  maxF1Data)
			{
				status = pPkt->GenerateTransmitData(pF1AncData, maxF1Data, pktSize);
				if (AJA_FAILURE(status))
					break;

				pF1AncData += pktSize;
				maxF1Data  -= pktSize;
			}
		}
		else if (pF2AncData  &&  maxF2Data)
		{
			status = pPkt->GenerateTransmitData(pF2AncData, maxF2Data, pktSize);
			if (AJA_FAILURE(status))
				break;

			pF2AncData += pktSize;
			maxF2Data  -= pktSize;
		}
	}	//	for each of my anc packets
	return status;
}


AJAStatus AJAAncillaryList::GetVANCTransmitData (NTV2_POINTER & inFrameBuffer,  const NTV2FormatDescriptor & inFormatDesc)
{
	if (inFrameBuffer.IsNULL())
	{
		LOGMYERROR("AJA_STATUS_NULL: null frame buffer");
		return AJA_STATUS_NULL;
	}
	if (!inFormatDesc.IsValid())
	{
		LOGMYERROR("AJA_STATUS_BAD_PARAM: Invalid format descriptor");
		return AJA_STATUS_BAD_PARAM;
	}
	if (!inFormatDesc.IsVANC())
	{
		LOGMYERROR("AJA_STATUS_BAD_PARAM: Not a VANC geometry");
		return AJA_STATUS_BAD_PARAM;
	}
	if (inFormatDesc.GetPixelFormat() != NTV2_FBF_10BIT_YCBCR  &&  inFormatDesc.GetPixelFormat() != NTV2_FBF_8BIT_YCBCR)
	{
		LOGMYERROR("AJA_STATUS_UNSUPPORTED: unsupported pixel format: " << inFormatDesc);
		return AJA_STATUS_UNSUPPORTED;
	}
	if (!CountAncillaryData())
	{
		LOGMYWARN("List is empty");
		return AJA_STATUS_SUCCESS;
	}

	//	BRUTE-FORCE METHOD -- NOT VERY EFFICIENT
	const bool	isSD	(inFormatDesc.IsSDFormat());
	AJAAncillaryList	failures, successes;
	set <uint16_t>		lineOffsetsWritten;

	//	For each VANC line...
	for (UWord fbLineOffset(0);  fbLineOffset < inFormatDesc.GetFirstActiveLine();  fbLineOffset++)
	{
		ULWord smpteLine (0);	bool isF2 (false);
		inFormatDesc.GetSMPTELineNumber (fbLineOffset, smpteLine, isF2);

		//	Look for non-HANC packets destined for this line...
		for (AJAAncDataListConstIter iter(m_ancList.begin());   (iter != m_ancList.end()) && *iter;   ++iter)
		{
			bool								muxedOK	(false);
			AJAAncillaryData &					ancData (**iter);
			const AJAAncillaryDataLocation &	loc		(ancData.GetDataLocation());
			vector<uint16_t>					u16PktComponents;
			if (ancData.GetDataCoding() != AJAAncillaryDataCoding_Digital)	//	Ignore "Raw" or "Analog" or "Unknown" packets
				continue;
			if (loc.GetDataSpace() != AJAAncillaryDataSpace_VANC)			//	Ignore "HANC" or "Unknown" packets
			{
				//LOGMYWARN("Skipping non-VANC packet " << ancData);
				continue;
			}
			if (loc.GetLineNumber() != smpteLine)							//	Wrong line number
			{
				//LOGMYWARN("Skipping packet not destined for line " << DEC(smpteLine) << " " << loc);
				continue;
			}
			if (!IS_VALID_AJAAncillaryDataChannel(loc.GetDataChannel()))	//	Bad data channel
			{
				LOGMYWARN("Skipped packet with invalid data channel " << loc);
				continue;
			}

			//	For v210 buffers, generate 10-bit packet data and put into u16 vector...
			muxedOK = AJA_SUCCESS(ancData.GenerateTransmitData(u16PktComponents));
			if (muxedOK)
			{
				if (inFormatDesc.GetPixelFormat() == NTV2_FBF_8BIT_YCBCR)	//	2vuy buffers are simple -- just copy the data
				{
					uint8_t *	pLine	((uint8_t*)inFormatDesc.GetRowAddress(inFrameBuffer.GetHostPointer(), fbLineOffset));
					if (isSD)
					{	//	SD overwrites both Y & C channels in the frame buffer:
						for (unsigned ndx(0);  ndx < u16PktComponents.size();  ndx++)
							pLine[ndx] = uint8_t(u16PktComponents[ndx] & 0xFF);
					}
					else
					{	//	HD overwrites only the Y or C channel data in the frame buffer:
						unsigned	dstNdx(loc.IsLumaChannel() ? 1 : 0);
						for (unsigned srcNdx(0);  srcNdx < u16PktComponents.size();  srcNdx++)
							pLine[dstNdx + 2*srcNdx] = uint8_t(u16PktComponents[srcNdx] & 0x00FF);
					}
				}	//	if 2vuy
				else	/////////////	v210 FBF:
				{
					if (isSD)
					{	//	For SD, just pack the u16 components into the buffer...
						muxedOK = ::YUVComponentsTo10BitYUVPackedBuffer (u16PktComponents, inFrameBuffer, inFormatDesc, fbLineOffset);
					}
					else
					{	//	HD overwrites only the Y or C channel data:
						vector<uint16_t>	YUV16Line;
						unsigned			dstNdx	(loc.IsLumaChannel() ? 1 : 0);

						//	Read original Y+C components from FB...
						muxedOK = UnpackLine_10BitYUVtoU16s (YUV16Line, inFrameBuffer, inFormatDesc, fbLineOffset);
						//cerr << "WriteVANCData|orig: +" << DEC0N(fbLineOffset,2) << ": ";  inFormatDesc.PrintSMPTELineNumber(cerr, fbLineOffset);  cerr << ":" << endl << YUV16Line << endl;

						//	Patch the Y or C channel...
						if (muxedOK)
							for (unsigned srcNdx(0);  srcNdx < u16PktComponents.size();  srcNdx++)
								YUV16Line[dstNdx + 2*srcNdx] = u16PktComponents[srcNdx] & 0x03FF;
						//cerr << "WriteVANCData|modi: +" << DEC0N(fbLineOffset,2) << ": ";  inFormatDesc.PrintSMPTELineNumber(cerr, fbLineOffset);  cerr << ":" << endl << YUV16Line << endl;

						//	Repack the patched YUV16 line back into the FB...
						if (muxedOK)
							muxedOK = ::YUVComponentsTo10BitYUVPackedBuffer (YUV16Line, inFrameBuffer, inFormatDesc, fbLineOffset);
					}	//	else HD
				}	//	else v210
			}	//	if GenerateTransmitData OK

			//	TBD:	NEED TO TAKE INTO CONSIDERATION	loc.GetHorizontalOffset()

			if (muxedOK)
			{
				lineOffsetsWritten.insert(uint16_t(fbLineOffset));	//	Remember which FB line offsets we overwrote
				successes.AddAncillaryData(ancData);
			}
			else
				failures.AddAncillaryData(ancData);
		}	//	for each packet
	}	//	for each VANC line

	//	Any analog packets?
	for (AJAAncDataListConstIter iter(m_ancList.begin());   (iter != m_ancList.end()) && *iter;   ++iter)
	{
		bool								success		(inFormatDesc.IsSDFormat());
		AJAAncillaryData &					ancData		(**iter);
		const AJAAncillaryDataLocation &	loc			(ancData.GetDataLocation());
		ULWord								lineOffset	(0);
		if (!ancData.IsRaw())
			continue;	//	Ignore "Digital" or "Unknown" packets

		if (success)
			success = inFormatDesc.GetLineOffsetFromSMPTELine (loc.GetLineNumber(), lineOffset);
		if (success  &&  lineOffsetsWritten.find(uint16_t(lineOffset)) != lineOffsetsWritten.end())
			success = false;	//	Line already written -- "analog" data overwrites entire line
		if (success)
		{
			if (inFormatDesc.GetPixelFormat() == NTV2_FBF_8BIT_YCBCR)
				::memcpy((void*)inFormatDesc.GetRowAddress(inFrameBuffer.GetHostPointer(), lineOffset), ancData.GetPayloadData(), ancData.GetDC());	//	'2vuy' -- straight copy
			else
			{
				vector<uint16_t>	pktComponentData;
				success = AJA_SUCCESS(ancData.GenerateTransmitData(pktComponentData));
				if (success)
					success = ::YUVComponentsTo10BitYUVPackedBuffer (pktComponentData, inFrameBuffer, inFormatDesc, UWord(lineOffset));
			}
		}

		if (success)
		{
			lineOffsetsWritten.insert(uint16_t(lineOffset));	//	Remember which FB line offsets we overwrote
			successes.AddAncillaryData(ancData);
		}
		else
			failures.AddAncillaryData(ancData);
	}	//	for each Analog packet

	if (failures.CountAncillaryData())
		LOGMYWARN("FAILURES: " << failures);
	if (successes.CountAncillaryData())
		LOGMYDEBUG("SUCCESSES: " << successes);

	//	At least one success is considered SUCCESS.
	//	Zero successes and one or more failures is considered FAIL...
	return successes.CountAncillaryData() == 0  &&  failures.CountAncillaryData() > 0  ?  AJA_STATUS_FAIL  :  AJA_STATUS_SUCCESS;
}	//	WriteVANCData


AJAStatus AJAAncillaryList::GetIPTransmitData (NTV2_POINTER & F1Buffer, NTV2_POINTER & F2Buffer,
												const bool inIsProgressive, const uint32_t inF2StartLine)
{
	const size_t		maxPktLengthBytes		(0x0000FFFF);	//	65535 max
	const size_t		maxPktLengthWords		((maxPktLengthBytes+1) / sizeof(uint32_t) - 1);	//	16383 max
	const uint32_t		maxPacketCount			(0x000000FF);	//	255 max
	size_t				oldPktLengthWords		(0);
	uint32_t			actF1PktCnt				(0);
	uint32_t			actF2PktCnt				(0);
	unsigned			countOverflows			(0);
	size_t				overflowWords			(0);
	AJAStatus			result					(AJA_STATUS_SUCCESS);
	vector<uint32_t>	U32F1s, U32F2s;		//	32-bit network-byte-order data

	//	Reserve space in ULWord vectors...
	U32F1s.reserve(maxPktLengthWords);
	if (!inIsProgressive)
		U32F2s.reserve(maxPktLengthWords);

	//	Generate transmit data for each of my packets...
	for (uint32_t pktNdx(0);  pktNdx < CountAncillaryData();  pktNdx++)
	{
		AJAAncillaryData *	pAncData	(GetAncillaryDataAtIndex(pktNdx));
		if (!pAncData)
			return AJA_STATUS_NULL;	//	Fail

		AJAAncillaryData &	pkt	(*pAncData);
		if (pkt.GetDataCoding() != AJAAncillaryDataCoding_Digital)
		{
			LOGMYDEBUG("Skipped Pkt " << DEC(pktNdx+1) << " of " << DEC(CountAncillaryData()) << " -- Analog/Raw");
			continue;	//	Skip analog/raw packets
		}

		if (!inIsProgressive  &&  pkt.GetLocationLineNumber() >= inF2StartLine)
		{	//	Interlaced video && Field2
			if (actF2PktCnt >= maxPacketCount)
			{
				countOverflows++;
				LOGMYWARN("Skipped pkt " << DEC(pktNdx+1) << " of " << DEC(CountAncillaryData()) << " -- F2 RTP pkt count overflow");
				continue;
			}
			oldPktLengthWords = U32F2s.size();
			result = pkt.GenerateTransmitData(U32F2s);
			if (AJA_SUCCESS(result) && U32F2s.size() > maxPktLengthWords)
			{
				overflowWords += U32F2s.size() - oldPktLengthWords;
				LOGMYWARN("Skipped pkt " << DEC(pktNdx+1) << " of " << DEC(CountAncillaryData()) << " -- F2 RTP pkt length overflow");
				while (U32F2s.size() > oldPktLengthWords)
					U32F2s.pop_back();
				continue;
			}
			if (AJA_SUCCESS(result))
				actF2PktCnt++;
		}
		else
		{
			if (actF1PktCnt >= maxPacketCount)
			{
				countOverflows++;
				LOGMYWARN("Skipped pkt " << DEC(pktNdx+1) << " of " << DEC(CountAncillaryData()) << " -- F1 RTP pkt count overflow");
				continue;
			}
			oldPktLengthWords = U32F1s.size();
			result = pkt.GenerateTransmitData(U32F1s);
			if (AJA_SUCCESS(result) && U32F1s.size() > maxPktLengthWords)
			{
				overflowWords += U32F1s.size() - oldPktLengthWords;
				LOGMYWARN("Skipped pkt " << DEC(pktNdx+1) << " of " << DEC(CountAncillaryData()) << " -- F1 RTP pkt length overflow");
				while (U32F1s.size() > oldPktLengthWords)
					U32F1s.pop_back();
				continue;
			}
			if (AJA_SUCCESS(result))
				actF1PktCnt++;
		}
		if (AJA_FAILURE(result))
		{
			LOGMYERROR("Pkt " << DEC(pktNdx+1) << " of " << DEC(CountAncillaryData()) << " failed in GenerateTransmitData: " << ::AJAStatusToString(result));
			break;
		}
	}	//	for each packet

	size_t			RTP_pkt_length_bytes	(U32F1s.size() * sizeof(uint32_t));
	const size_t	F1BytesNeeded			(RTP_pkt_length_bytes + AJARTPAncPayloadHeader::GetHeaderByteCount());
	NTV2_ASSERT(RTP_pkt_length_bytes <= maxPktLengthBytes);
	NTV2_ASSERT(actF1PktCnt <= maxPacketCount);

	//	Resize F1 output buffer (if needed)...
	if (F1Buffer.GetByteCount() < F1BytesNeeded)
	{
		if (F1Buffer.IsProvidedByClient())
		{
			LOGMYERROR("AJA_STATUS_BADBUFFERSIZE: F1 buffer has fixed " << DEC(F1Buffer.GetByteCount())
						<< "-byte size -- needs " << DEC(F1BytesNeeded) << " for " << DEC(actF1PktCnt) << " anc pkt(s)");
			return AJA_STATUS_BADBUFFERSIZE;
		}
		//	Resize...
		if (!F1Buffer.Allocate(F1BytesNeeded))
		{
			LOGMYERROR("AJA_STATUS_MEMORY: F1 buffer resize failed, requested size=" << DEC(F1BytesNeeded) << " bytes");
			return AJA_STATUS_MEMORY;
		}
		LOGMYDEBUG("Resized F1 buffer to " << DEC(F1BytesNeeded) << " bytes for " << DEC(actF1PktCnt) << " pkt(s)");
	}

	//	Write F1 RTP header...
	AJARTPAncPayloadHeader	RTPHeaderF1, RTPHeaderF2;
	if (inIsProgressive)
		RTPHeaderF1.SetProgressive();
	else
		RTPHeaderF1.SetField1();
	RTPHeaderF1.SetAncPacketCount(uint8_t(actF1PktCnt));
	RTPHeaderF1.SetPacketLength(uint16_t(RTP_pkt_length_bytes));
		//	Playout:  JeffL needs full RTP pkt bytecount -- firmware looks for it in LS 16 bits of SequenceNumber in RTP header:
		RTPHeaderF1.SetSequenceNumber(uint32_t(F1BytesNeeded) & 0x0000FFFF);
	if (!RTPHeaderF1.WriteBuffer(F1Buffer))
		{LOGMYERROR("F1 RTP anc payload header WriteBuffer failed, " << F1Buffer);	return AJA_STATUS_FAIL;}
	//	Write F1 packed data...
	if (!U32F1s.empty())
	{
		if (!F1Buffer.PutU32s(U32F1s, 5))
		{
			LOGMYERROR("F1Buffer.PutU32s failed: F1Buffer=" << F1Buffer << ", U32F1s: " << ULWordSequence(U32F1s));
			return AJA_STATUS_FAIL;
		}
		LOGMYINFO("Put " << DEC(U32F1s.size()) << " ULWord(s) into F1 buffer starting at ULWord 5: " << F1Buffer << ": " << RTPHeaderF1);
		#if defined(_DEBUG)
			LOGMYDEBUG("F1 Output Buffer: " << F1Buffer.GetU32s(0, U32F1s.size()+5, true));	// True means Byte-swapped
		#endif
	}

	if (!inIsProgressive)
	{
		RTP_pkt_length_bytes = U32F2s.size() * sizeof(uint32_t);
		const size_t	F2BytesNeeded	(RTP_pkt_length_bytes + AJARTPAncPayloadHeader::GetHeaderByteCount());
		NTV2_ASSERT(RTP_pkt_length_bytes <= maxPktLengthBytes);
		NTV2_ASSERT(actF2PktCnt <= maxPacketCount);

		//	Resize F2 output buffer (if needed)...
		if (F2Buffer.GetByteCount() < F2BytesNeeded)
		{
			if (F2Buffer.IsProvidedByClient())
			{
				LOGMYERROR("AJA_STATUS_BADBUFFERSIZE: F2 buffer has fixed " << DEC(F2Buffer.GetByteCount())
							<< "-byte size -- needs " << DEC(F2BytesNeeded) << " for " << DEC(actF2PktCnt) << " anc pkt(s)");
				return AJA_STATUS_BADBUFFERSIZE;
			}
			//	Resize...
			if (!F2Buffer.Allocate(F2BytesNeeded))
			{
				LOGMYERROR("AJA_STATUS_MEMORY: F2 buffer resize failed, requested size=" << DEC(F2BytesNeeded) << " bytes");
				return AJA_STATUS_MEMORY;
			}
			LOGMYDEBUG("Resized F2 buffer to " << DEC(F2BytesNeeded) << " bytes for " << DEC(actF2PktCnt) << " pkt(s)");
		}

		//	Write F2 RTP header...
		RTPHeaderF2.SetField2();
		RTPHeaderF2.SetAncPacketCount(uint8_t(actF2PktCnt));
		RTPHeaderF2.SetPacketLength(uint16_t(RTP_pkt_length_bytes));
			//	Playout:  JeffL needs full RTP pkt bytecount -- firmware looks for it in LS 16 bits of SequenceNumber in RTP header:
			RTPHeaderF2.SetSequenceNumber(uint32_t(F2BytesNeeded) & 0x0000FFFF);
		if (!RTPHeaderF2.WriteBuffer(F2Buffer))
			{LOGMYERROR("F2 RTP anc payload header WriteBuffer failed, " << F2Buffer);	return AJA_STATUS_FAIL;}

		//	Write the packed data into the packet buffers...
		if (!U32F2s.empty())
		{
			if (!F2Buffer.PutU32s(U32F2s, 5))
			{
				LOGMYERROR("F2Buffer.PutU32s failed: F2Buffer=" << F2Buffer << ", U32F2s: " << ULWordSequence(U32F2s));
				return AJA_STATUS_FAIL;
			}
			LOGMYINFO("Put " << DEC(U32F2s.size()) << " ULWord(s) into F2 buffer starting at ULWord 5: " << F2Buffer << ": " << RTPHeaderF2);
			#if defined(_DEBUG)
				LOGMYDEBUG("F2 Output Buffer: " << F2Buffer.GetU32s(0, U32F2s.size()+5, true));	// True means Byte-swapped
			#endif
		}
	}	//	if interlaced video

	if (countOverflows)
		{LOGMYERROR(DEC(countOverflows) << " pkt(s) skipped due to RTP anc pkt count overflow (255 max capacity)");	return AJA_STATUS_RANGE;}
	if (overflowWords)
		{LOGMYERROR(DEC(overflowWords*4) << " bytes skipped due to RTP pkt length overflow (65535 max capacity)");	return AJA_STATUS_RANGE;}
	return AJA_STATUS_SUCCESS;
}	//	GetIPTransmitData


ostream & AJAAncillaryList::Print (ostream & inOutStream, const bool inDumpPayload) const
{
	unsigned	num	(0);
	inOutStream << "AJAAncillaryList: " << CountAncillaryData () << " pkts:" << endl;
	for (AJAAncDataListConstIter it (m_ancList.begin ());  it != m_ancList.end ();  )
	{
		AJAAncillaryData *	ancData	(*it);

		inOutStream << "## Pkt" << DEC0N(++num,3) << ":  " << ancData->AsString(inDumpPayload ? 64 : 0);
//		ancData->Print (inOutStream, inDumpPayload);
		++it;
		if (it != m_ancList.end())
			inOutStream << endl;
	}
	return inOutStream;
}
