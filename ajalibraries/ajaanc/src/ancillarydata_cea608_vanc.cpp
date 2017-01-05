/**
	@file		ancillarydata_cea608_vanc.cpp
	@brief		Implements the AJAAncillaryData_Cea608_Vanc class.
	@copyright	(C) 2010-2017 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#include "ancillarydata_cea608_vanc.h"
#include <ios>
#include <iomanip>

using namespace std;


AJAAncillaryData_Cea608_Vanc::AJAAncillaryData_Cea608_Vanc()
	:	AJAAncillaryData_Cea608()
{
	Init();
}


AJAAncillaryData_Cea608_Vanc::AJAAncillaryData_Cea608_Vanc (const AJAAncillaryData_Cea608_Vanc& clone)
	:	AJAAncillaryData_Cea608()
{
	Init();
	*this = clone;
}


AJAAncillaryData_Cea608_Vanc::AJAAncillaryData_Cea608_Vanc (const AJAAncillaryData_Cea608_Vanc *pClone)
	:	AJAAncillaryData_Cea608()
{
	Init();
	if (pClone != NULL_PTR)
		*this = *pClone;
}


AJAAncillaryData_Cea608_Vanc::AJAAncillaryData_Cea608_Vanc (const AJAAncillaryData *pData)
	:	AJAAncillaryData_Cea608 (pData)
{
	Init();
}


AJAAncillaryData_Cea608_Vanc::~AJAAncillaryData_Cea608_Vanc ()
{
}


void AJAAncillaryData_Cea608_Vanc::Init (void)
{
	m_ancType = AJAAncillaryDataType_Cea608_Vanc;
	m_coding  = AJAAncillaryDataCoding_Digital;
	m_DID	  = AJAAncillaryData_Cea608_Vanc_DID;
	m_SID	  = AJAAncillaryData_Cea608_Vanc_SID;

	m_fieldNum = 0;		// default to field 1
	m_lineNum  = 12;	// line 21 (0 = line 9 in 525i)
}


AJAAncillaryData_Cea608_Vanc & AJAAncillaryData_Cea608_Vanc::operator= (const AJAAncillaryData_Cea608_Vanc & rhs)
{
	if (this != &rhs)		// ignore self-assignment
	{
		AJAAncillaryData_Cea608::operator= (rhs);		// copy the base class stuff

		// copy the local stuff
		m_fieldNum = rhs.m_fieldNum;
		m_lineNum  = rhs.m_lineNum;
	}
	return *this;
}


void AJAAncillaryData_Cea608_Vanc::Clear (void)
{
	AJAAncillaryData_Cea608::Clear();
	Init();
}


AJAStatus AJAAncillaryData_Cea608_Vanc::SetLine (const uint8_t fieldNum, const uint8_t lineNum)
{
	m_fieldNum = fieldNum & 0x01;	// 0 = field 1, 1 = field 2
	m_lineNum  = lineNum  & 0x1F;	// valid range is 0 (= line 9) to 31 (= line 40)
	return AJA_STATUS_SUCCESS;
}


AJAStatus AJAAncillaryData_Cea608_Vanc::GetLine (uint8_t & fieldNum, uint8_t & lineNum) const
{
	fieldNum = m_fieldNum;
	lineNum  = m_lineNum;
	return AJA_STATUS_SUCCESS;
}


AJAStatus AJAAncillaryData_Cea608_Vanc::ParsePayloadData (void)
{
	if (m_pPayload == NULL_PTR  ||  m_DC < AJAAncillaryData_Cea608_Vanc_PayloadSize)
	{
		Init();						// load default values
		m_rcvDataValid = false;
		return AJA_STATUS_FAIL;
	}

	// we have some kind of payload data - try to parse it
	m_fieldNum = (m_pPayload[0] >> 7) & 0x01;	// the field number (flag) is bit 7 of the 1st payload word
	m_lineNum  = (m_pPayload[0] & 0x1F);		// the line number is bits [4:0] of the 1st payload word

	m_char1	   = m_pPayload[1];		// the 1st character
	m_char2    = m_pPayload[2];		// the 2nd character

	m_rcvDataValid = true;
	return AJA_STATUS_SUCCESS;
}


AJAStatus AJAAncillaryData_Cea608_Vanc::GeneratePayloadData (void)
{
	m_DID = AJAAncillaryData_Cea608_Vanc_DID;
	m_SID = AJAAncillaryData_Cea608_Vanc_SID;

	AJAStatus status = AllocDataMemory (AJAAncillaryData_Cea608_Vanc_PayloadSize);
	if (AJA_SUCCESS (status))
	{
		m_pPayload[0] = ((m_fieldNum & 0x01) << 7) | (m_lineNum & 0x1F);	// fieldNum goes in bit 7, line num goes in bits [4:0]
		m_pPayload[1] = m_char1;
		m_pPayload[2] = m_char2;
	}

	m_checksum = Calculate8BitChecksum();
	return status;
}

ostream & AJAAncillaryData_Cea608_Vanc::Print (ostream & debugStream, const bool bShowDetail) const
{
	debugStream << IDAsString() << "(" << ::AJAAncillaryDataCodingToString (m_coding) << ")" << endl;
	AJAAncillaryData_Cea608::Print (debugStream, bShowDetail);
	debugStream << endl
				<< "Field: " << (m_fieldNum ? "F2" : "F1") << endl
				<< "Line: " << dec << uint16_t(m_lineNum);
	return debugStream;
}


AJAAncillaryDataType AJAAncillaryData_Cea608_Vanc::RecognizeThisAncillaryData (const AJAAncillaryData * pInAncData)
{
	if (pInAncData->GetDataCoding() == AJAAncillaryDataCoding_Digital)
		if (pInAncData->GetDID() == AJAAncillaryData_Cea608_Vanc_DID)
			if (pInAncData->GetSID() == AJAAncillaryData_Cea608_Vanc_SID)
				if (pInAncData->GetDC() == AJAAncillaryData_Cea608_Vanc_PayloadSize)
					return AJAAncillaryDataType_Cea608_Vanc;
	return AJAAncillaryDataType_Unknown;
}
