/**
	@file		ancillarylist.cpp
	@brief		Implementation of the AJAAncillaryList class.
	@copyright	(C) 2010-2017 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#include "ancillarylist.h"
#include "ancillarydatafactory.h"
#include "ajacc/includes/ntv2smpteancdata.h"	//	This makes 'ajaanc' dependent upon 'ajacc':
												//	CNTV2SMPTEAncData::UnpackLine_8BitYUVtoUWordSequence
												//	CNTV2SMPTEAncData::GetAncPacketsFromVANCLine
using namespace std;


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


AJAAncillaryData * AJAAncillaryList::GetAncillaryDataAtIndex (const uint32_t index) const
{
	AJAAncillaryData *	pAncData	(NULL);

	if (m_ancList.size () > 0  &&  index < m_ancList.size ())
	{
		AJAAncDataListConstIter	it	(m_ancList.begin ());

		for (uint32_t i = 0;  i < index;  i++)	// the problem with lists is: no random access...
			++it;

		pAncData = *it;
	}

	return pAncData;
}


AJAStatus AJAAncillaryList::ParseAllAncillaryData (void)
{
	AJAStatus result = AJA_STATUS_SUCCESS;

	for (AJAAncDataListConstIter it (m_ancList.begin ());  it != m_ancList.end ();  ++it)
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

	for (AJAAncDataListConstIter it (m_ancList.begin ());  it != m_ancList.end ();  ++it)
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
	AJAAncillaryData *	pData = pInAncData->Clone();
	if (!pData)
		return AJA_STATUS_FAIL;

	// add it to the list
	if (pData)
		m_ancList.push_back(pData);

	return AJA_STATUS_SUCCESS;
}


AJAStatus AJAAncillaryList::Clear (void)
{
	for (AJAAncDataListConstIter it (m_ancList.begin());  it != m_ancList.end();  ++it)
	{
		AJAAncillaryData *	pAncData(*it);
		if (pAncData)
			delete (pAncData);
	}

	m_ancList.clear();
	return AJA_STATUS_SUCCESS;
}


AJAStatus AJAAncillaryList::RemoveAncillaryData (AJAAncillaryData * pAncData)
{
	if (!pAncData)
		return AJA_STATUS_NULL;
	else
		m_ancList.remove(pAncData);	//	note:	there's no feedback as to whether one or more elements existed or not
									//	note:	pAncData is NOT deleted!
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

		//	NOTE:	Right now we're just bailing if there is a detected error in the raw data stream.
		//			Theoretically one could try to get back "in sync" and recover any following data, but
		//			we'll save that for another day...
		if (AJA_FAILURE(status))
			break;
		else
		{
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
						pPrevData->AppendPayload (&newAncData);
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
		}
	}	// while (bMoreData)

	return status;

}	//	AddReceivedAncillaryData


AJAStatus AJAAncillaryList::AppendReceivedRTPAncillaryData (const std::vector<uint8_t> & inRTPPacketData)
{
	(void) inRTPPacketData;		//	TODO:	FINISH THIS
	return AJA_STATUS_UNSUPPORTED;
}


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
	if (false)	//	!inLoc.IsValid())		//	NOTE: Tough call. Decided not to validate the AJAAncillaryDataLocation here.
		return AJA_STATUS_BAD_PARAM;

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


AJAStatus AJAAncillaryList::AddVANCData (const vector<uint16_t> & inPacketWords, const uint16_t inLineNum, const AJAAncillaryDataChannel inChannel)
{
	return AddVANCData (inPacketWords, AJAAncillaryDataLocation (AJAAncillaryDataLink_A,  inChannel, AJAAncillaryDataSpace_VANC,  inLineNum));

}	//	AddVANCData


AJAStatus AJAAncillaryList::SetFromVANCData (const NTV2_POINTER & inFrameBuffer, const NTV2FormatDescriptor & inFormatDesc,
											AJAAncillaryList & outF1Packets, AJAAncillaryList & outF2Packets)
{
	outF1Packets.Clear();
	outF2Packets.Clear();

	if (inFrameBuffer.IsNULL())
		return AJA_STATUS_BAD_PARAM;
	if (!inFormatDesc.IsValid())
		return AJA_STATUS_BAD_PARAM;
	if (!inFormatDesc.IsVANC())
		return AJA_STATUS_BAD_PARAM;

	const ULWord			vancBytes	(inFormatDesc.GetTotalRasterBytes() - inFormatDesc.GetVisibleRasterBytes());
	const NTV2PixelFormat	fbf			(inFormatDesc.GetPixelFormat());
	const bool				isSD		(NTV2_IS_SD_STANDARD(inFormatDesc.GetVideoStandard()));
	if (inFrameBuffer.GetByteCount() < vancBytes)
		return AJA_STATUS_FAIL;
	if (fbf != NTV2_FBF_10BIT_YCBCR  &&  fbf != NTV2_FBF_8BIT_YCBCR)
		return AJA_STATUS_UNSUPPORTED;	//	Only 'v210' and '2vuy' currently supported

	for (ULWord line (0);  line < inFormatDesc.GetFirstActiveLine();  line++)
	{
		UWordSequence	uwords;
		bool			isF2			(false);
		ULWord			smpteLineNum	(0);
		unsigned		ndx				(0);

		inFormatDesc.GetSMPTELineNumber (line, smpteLineNum, isF2);
		if (fbf == NTV2_FBF_10BIT_YCBCR)
			::UnpackLine_10BitYUVtoUWordSequence (inFormatDesc.GetRowAddress(inFrameBuffer.GetHostAddress(0), line),
													inFormatDesc, uwords);
		else
			CNTV2SMPTEAncData::UnpackLine_8BitYUVtoUWordSequence (inFormatDesc.GetRowAddress(inFrameBuffer.GetHostAddress(0), line),
																	uwords,  inFormatDesc.GetRasterWidth());
		if (isSD)
		{
			UWordVANCPacketList			ycPackets;
			UWordSequence				ycHOffsets;
			AJAAncillaryDataLocation	loc	(AJAAncillaryDataLink_Unknown, AJAAncillaryDataChannel_Both, AJAAncillaryDataSpace_VANC, smpteLineNum);

			CNTV2SMPTEAncData::GetAncPacketsFromVANCLine (uwords, kNTV2SMPTEAncChannel_Both, ycPackets, ycHOffsets);
			NTV2_ASSERT(ycPackets.size() == ycHOffsets.size());

			for (UWordVANCPacketListConstIter it (ycPackets.begin());  it != ycPackets.end();  ++it, ndx++)
				if (isF2)
					outF2Packets.AddVANCData (*it, loc.SetHorizontalOffset(ycHOffsets[ndx]));
				else
					outF1Packets.AddVANCData (*it, loc.SetHorizontalOffset(ycHOffsets[ndx]));
		}
		else
		{
			UWordVANCPacketList			yPackets, cPackets;
			UWordSequence				yHOffsets, cHOffsets;
			AJAAncillaryDataLocation	yLoc	(AJAAncillaryDataLink_Unknown, AJAAncillaryDataChannel_Y, AJAAncillaryDataSpace_VANC, smpteLineNum);
			AJAAncillaryDataLocation	cLoc	(AJAAncillaryDataLink_Unknown, AJAAncillaryDataChannel_C, AJAAncillaryDataSpace_VANC, smpteLineNum);

			CNTV2SMPTEAncData::GetAncPacketsFromVANCLine (uwords, kNTV2SMPTEAncChannel_Y, yPackets, yHOffsets);
			CNTV2SMPTEAncData::GetAncPacketsFromVANCLine (uwords, kNTV2SMPTEAncChannel_C, cPackets, cHOffsets);
			NTV2_ASSERT(yPackets.size() == yHOffsets.size());
			NTV2_ASSERT(cPackets.size() == cHOffsets.size());

			unsigned	ndx(0);
			for (UWordVANCPacketListConstIter it (yPackets.begin());  it != yPackets.end();  ++it, ndx++)
				if (isF2)
					outF2Packets.AddVANCData (*it, yLoc.SetHorizontalOffset(yHOffsets[ndx]));
				else
					outF1Packets.AddVANCData (*it, yLoc.SetHorizontalOffset(yHOffsets[ndx]));

			ndx = 0;
			for (UWordVANCPacketListConstIter it (cPackets.begin());  it != cPackets.end();  ++it, ndx++)
				if (isF2)
					outF2Packets.AddVANCData (*it, cLoc.SetHorizontalOffset(cHOffsets[ndx]));
				else
					outF1Packets.AddVANCData (*it, cLoc.SetHorizontalOffset(cHOffsets[ndx]));
		}
	}	//	for each VANC line
	//cerr << "AJAAncillaryList::SetFromVANCData: returning " << DEC(outF1Packets.CountAncillaryData()) << "/" << DEC(outF2Packets.CountAncillaryData()) << " F1/F2 pkts:" << endl << outF1Packets << endl << outF2Packets << endl;
	return AJA_STATUS_SUCCESS;
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
	m_analogTypeMap.erase (inLineNum);				// in case someone has already set this line
	if (inAncType != AJAAncillaryDataType_Unknown)	// note: a non-entry is the same as AJAAncillaryDataType_Unknown
		m_analogTypeMap [inLineNum] = inAncType;
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



AJAStatus AJAAncillaryList::GetAncillaryDataTransmitSize (const bool bProgressive, const uint32_t f2StartLine, uint32_t & ancSizeF1, uint32_t & ancSizeF2) const
{
	AJAStatus	status	(AJA_STATUS_SUCCESS);
	uint32_t	f1Size	(0);
	uint32_t	f2Size	(0);

	for (AJAAncillaryDataList::const_iterator it (m_ancList.begin ());  it != m_ancList.end ();  ++it)
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


AJAStatus AJAAncillaryList::GetAncillaryDataTransmitData (const bool bProgressive, const uint32_t f2StartLine, uint8_t * pF1AncData, const uint32_t inMaxF1Data, uint8_t * pF2AncData, const uint32_t inMaxF2Data) const
{
	AJAStatus	status		(AJA_STATUS_SUCCESS);
	uint32_t	maxF1Data	(inMaxF1Data);
	uint32_t	maxF2Data	(inMaxF2Data);

	for (AJAAncDataListConstIter it (m_ancList.begin ());  it != m_ancList.end ();  ++it)
	{
		AJAAncillaryData *	pAncData = *it;

		uint32_t packetSize = 0;
		if (bProgressive || pAncData->GetLocationLineNumber() < f2StartLine)
		{
			if (pF1AncData != NULL && maxF1Data > 0)
			{
				status = pAncData->GenerateTransmitData(pF1AncData, maxF1Data, packetSize);
				if (status != AJA_STATUS_SUCCESS)
					break;

				pF1AncData += packetSize;
				maxF1Data  -= packetSize;
			}
		}
		else
		{
			if (pF2AncData != NULL && maxF2Data > 0)
			{
				status = pAncData->GenerateTransmitData(pF2AncData, maxF2Data, packetSize);
				if (status != AJA_STATUS_SUCCESS)
					break;

				pF2AncData += packetSize;
				maxF2Data  -= packetSize;
			}
		}
	}
	return status;
}


ostream & AJAAncillaryList::Print (ostream & inOutStream, const bool inDumpPayload) const
{
	unsigned	num	(0);
	inOutStream << "AJAAncillaryList: " << CountAncillaryData () << " pkts:" << endl;
	for (AJAAncDataListConstIter it (m_ancList.begin ());  it != m_ancList.end ();  )
	{
		AJAAncillaryData *	ancData	(*it);

		inOutStream << "## Packet " << ++num << ":  ";
		ancData->Print (inOutStream, inDumpPayload);
		++it;
		if (it != m_ancList.end())
			inOutStream << endl;
	}
	return inOutStream;
}
