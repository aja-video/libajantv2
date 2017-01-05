/**
	@file		ancillarylist.cpp
	@brief		Implementation of the AJAAncillaryList class.
	@copyright	(C) 2010-2017 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#include "ancillarylist.h"
#include "ancillarydatafactory.h"

using namespace std;


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


AJAStatus AJAAncillaryList::Clear (void)
{
	for (AJAAncDataListConstIter it (m_ancList.begin ());  it != m_ancList.end ();  ++it)
	{
		AJAAncillaryData *	pAncData (*it);
		if (pAncData)
			delete (pAncData);
	}

	m_ancList.clear ();
	return AJA_STATUS_SUCCESS;
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
		AJAAncillaryDataType	ancType		(pAncData->GetAncillaryDataType ());

		if (matchType == ancType)
			count++;
	}

	return count;
}


AJAAncillaryData * AJAAncillaryList::GetAncillaryDataWithType (const AJAAncillaryDataType matchType, const uint32_t index) const
{
	AJAAncillaryData *	pResult	(NULL);
	uint32_t count = 0;

	for (AJAAncDataListConstIter it (m_ancList.begin ());  it != m_ancList.end ();  ++it)
	{
		AJAAncillaryData *		pAncData (*it);
		AJAAncillaryDataType	ancType  (pAncData->GetAncillaryDataType ());

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

	for (AJAAncDataListConstIter it (m_ancList.begin ());  it != m_ancList.end ();  ++it)
	{
		AJAAncillaryData *	pAncData = *it;

		if (DID == AJAAncillaryDataWildcard_DID  ||  DID == pAncData->GetDID ())
		{
			if (SID == AJAAncillaryDataWildcard_SID  ||  SID == pAncData->GetSID ())
				count++;
		}
	}

	return count;
}


AJAAncillaryData * AJAAncillaryList::GetAncillaryDataWithID (const uint8_t DID, const uint8_t SID, const uint32_t index) const
{
	AJAAncillaryData *	pResult	(NULL);
	uint32_t count = 0;

	for (AJAAncDataListConstIter it (m_ancList.begin ());  it != m_ancList.end ();  ++it)
	{
		AJAAncillaryData *	pAncData = *it;

		// Note: Unused
		//AJAAncillaryDataType ancType = pAncData->GetAncillaryDataType();

		if (DID == AJAAncillaryDataWildcard_DID  ||  DID == pAncData->GetDID ())
		{
			if (SID == AJAAncillaryDataWildcard_SID  ||  SID == pAncData->GetSID ())
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
	AJAStatus status = AJA_STATUS_SUCCESS;

	// Note: this makes a clone of the incoming AJAAncillaryData object and
	// saves it in the List. The caller is responsible for deleting the original,
	// and the List will take ownership of the clone.
	AJAAncillaryData *	pData = pInAncData->Clone ();

	// add it to the list
	if (pData)
		m_ancList.push_back (pData);

	return status;
}



AJAStatus AJAAncillaryList::RemoveAncillaryData (AJAAncillaryData * pAncData)
{
	if (!pAncData)
		return AJA_STATUS_NULL;
	else
		m_ancList.remove (pAncData);	// note:	there's no feedback as to whether one or more elements existed or not
										// note:	pAncData is NOT deleted!
	return AJA_STATUS_SUCCESS;
}


AJAStatus AJAAncillaryList::DeleteAncillaryData (AJAAncillaryData * pAncData)
{
	if (pAncData == NULL)
		return AJA_STATUS_NULL;

	RemoveAncillaryData (pAncData);
	delete pAncData;

	return AJA_STATUS_SUCCESS;
}


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
			if ( (lhs->GetLocationVideoStream() == AJAAncillaryDataVideoStream_Y)  &&  (rhs->GetLocationVideoStream() == AJAAncillaryDataVideoStream_C))
				bResult = true;
		}
	}

	return bResult;
}

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
		&& (prevLoc.lineNum  == newLoc.lineNum)
		&& (prevLoc.ancSpace == newLoc.ancSpace)		// technically, these should ALWAYS be the same for "analog"...?
		&& (prevLoc.link     == newLoc.link)			//
		&& (prevLoc.stream   == newLoc.stream) )		//
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
	AJAAncillaryDataLocation	defaultLoc		(AJAAncillaryDataLink_A, AJAAncillaryDataVideoStream_Y, AJAAncillaryDataSpace_VANC, 9);
	int32_t						remainingSize	(static_cast <int32_t> (dataSize));
	const uint8_t *				pInputData		(pRcvData);
	bool						bMoreData		(true);
	AJAAncillaryDataFactory		factory;

	while (bMoreData)
	{
		newAncData.Clear();

		bool bInsertNew = false;	// this will be set 'true' when (if) we find a new AJAAncillaryData to insert to the list
		AJAAncillaryDataType newAncType = AJAAncillaryDataType_Unknown;

		//	Tell the Anc Data object to init itself from the next set of input data...
		uint32_t	packetSize	(0);		//	This is where the AncillaryData object returns the number of bytes that were "consumed" from the input stream

		status = newAncData.InitWithReceivedData (pInputData, remainingSize, &defaultLoc, packetSize);

		//	NOTE:	Right now we're just bailing if there is a detected error in the raw data stream.
		//			Theoretically one could try to get back "in sync" and recover any following data, but
		//			we'll save that for another day...
		if (AJA_FAILURE (status))
			break;
		else
		{
			//	Determine what type of anc data we have, and create an object of the appropriate class...
			if (newAncData.GetDataCoding () == AJAAncillaryDataCoding_Digital)
			{
				//	Digital anc packets are fairly easy to categorize: you just have to look at their DID/SID.
				//	Also, they are (by definition) independent packets which become independent AJAAncillaryData objects.
				newAncType = factory.GuessAncillaryDataType (&newAncData);
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
					m_ancList.push_back(pData);		//	Add it to the list
			}

			remainingSize -= packetSize;		//	Decrease the remaining data size by the amount we just "consumed"
			pInputData += packetSize;			//	Advance the input data pointer by the same amount
			if (remainingSize <= 0)				//	All of the input data consumed?
				bMoreData = false;
		}
	}	// while (bMoreData)

	return status;

}	//	AddReceivedAncillaryData


AJAStatus AJAAncillaryList::AddVANCData (const vector<uint16_t> & inPacketWords, const uint16_t inLineNum, const AJAAncillaryDataVideoStream inStream)
{
	AJAStatus	status	(AJA_STATUS_SUCCESS);

	if (inPacketWords.size () < 7)
		return AJA_STATUS_RANGE;

	//	Use this as an uninitialized template...
	AJAAncillaryDataLocation	defaultLoc		(AJAAncillaryDataLink_A, inStream, AJAAncillaryDataSpace_VANC, inLineNum);
	vector<uint16_t>::const_iterator	iter	(inPacketWords.begin());

	if (*iter != 0x0000)
		return AJA_STATUS_FAIL;
	++iter;
	if (*iter != 0x03FF)
		return AJA_STATUS_FAIL;
	++iter;
	if (*iter != 0x03FF)
		return AJA_STATUS_FAIL;
	++iter;	//	Now pointing at DID

	uint8_t *					pPacketBytes	(new uint8_t [inPacketWords.size()]);
	if (!pPacketBytes)
		return AJA_STATUS_MEMORY;

	pPacketBytes[0] = 0xFF;						//	First byte always 0xFF
	pPacketBytes[1] = 0x80;						//	Location data byte 1:	"location valid" bit always set
	pPacketBytes[1] |= (inLineNum >> 7) & 0x0F;	//	Location data byte 1:	LS 4 bits == MS 4 bits of 11-bit line number
	if (inStream == AJAAncillaryDataVideoStream_Y)
		pPacketBytes[1] |= 0x20;				//	Location data byte 1:	set Y/C bit for luma channel;  clear for chroma
	pPacketBytes[2] = inLineNum & 0x7F;			//	Location data byte 2:	MSB reserved; LS 7 bits == LS 7 bits of 11-bit line number

	unsigned	ndx	(3);	//	Start copying at the DID
	while (iter != inPacketWords.end())
	{
		pPacketBytes [ndx++] = *iter & 0xFF;	//	Mask off upper byte
		++iter;
	}

	uint32_t			pktByteCount	(0);
	AJAAncillaryData	newAncData;
	status = newAncData.InitWithReceivedData (pPacketBytes, inPacketWords.size(), defaultLoc, pktByteCount);
	delete [] pPacketBytes;
	if (AJA_SUCCESS (status))
	{
		AJAAncillaryDataFactory	factory;
		AJAAncillaryDataType	newAncType	(factory.GuessAncillaryDataType (&newAncData));
		AJAAncillaryData *		pData		(factory.Create (newAncType, &newAncData));
		if (pData)
			m_ancList.push_back (pData);	//	Add it to my list
		else
			status = AJA_STATUS_FAIL;
	}
	return status;

}	//	AddVANCData


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
	for (AJAAncDataListConstIter it (m_ancList.begin ());  it != m_ancList.end ();  ++it)
	{
		AJAAncillaryData *	ancData	(*it);

		inOutStream << "## Packet " << ++num << ":  ";
		ancData->Print (inOutStream, inDumpPayload);
	}
	return inOutStream;
}
