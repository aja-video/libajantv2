/* SPDX-License-Identifier: MIT */
/**
	@file		ancillarydata_hdr_hlg.cpp
	@brief		Implements the AJAAncillaryData_HDMI_Aux class.
	@copyright	(C) 2012-2022 AJA Video Systems, Inc.
**/

#include "ancillarydata_hdmi_aux.h"
//#include <ios>
//#include <iomanip>

using namespace std;


#define AJAAncillaryData_HDMI_Aux_MaxPayloadSize 28


AJAAncillaryData_HDMI_Aux::AJAAncillaryData_HDMI_Aux ()
	:	AJAAncillaryData ()
{
	Init();
}


AJAAncillaryData_HDMI_Aux::AJAAncillaryData_HDMI_Aux (const AJAAncillaryData_HDMI_Aux & inClone)
	:	AJAAncillaryData ()
{
	Init();
	*this = inClone;
}


AJAAncillaryData_HDMI_Aux::AJAAncillaryData_HDMI_Aux (const AJAAncillaryData_HDMI_Aux * pInClone)
	:	AJAAncillaryData ()
{
	Init();
	if (pInClone)
		*this = *pInClone;
}


AJAAncillaryData_HDMI_Aux::AJAAncillaryData_HDMI_Aux (const AJAAncillaryData * pInData)
	:	AJAAncillaryData (pInData)
{
	m_ancType = AJAAncDataType_HDMI_Aux;
}


AJAAncillaryData_HDMI_Aux::~AJAAncillaryData_HDMI_Aux ()
{
}


void AJAAncillaryData_HDMI_Aux::Init (void)
{
	m_ancType	   = AJAAncDataType_HDMI_Aux;
	m_coding	   = AJAAncDataCoding_Digital;
}


void AJAAncillaryData_HDMI_Aux::Clear (void)
{
	AJAAncillaryData::Clear();
	Init();
}


AJAAncillaryData_HDMI_Aux & AJAAncillaryData_HDMI_Aux::operator = (const AJAAncillaryData_HDMI_Aux & rhs)
{
	// Ignore self-assignment
	if (this != &rhs)
	{
		// Copy the base class members
		AJAAncillaryData::operator=(rhs);
	}
	return *this;
}
	

AJAStatus AJAAncillaryData_HDMI_Aux::ParsePayloadData (void)
{
	// The size is specific to Canon
	if (GetDC() > AJAAncillaryData_HDMI_Aux_MaxPayloadSize)
	{
		// Load default values
		Init();
		m_rcvDataValid = false;
		return AJA_STATUS_FAIL;
	}

	m_rcvDataValid = true;
	return AJA_STATUS_SUCCESS;
}


ostream & AJAAncillaryData_HDMI_Aux::Print (ostream & debugStream, const bool bShowDetail) const
{
	AJAAncillaryData::Print (debugStream, bShowDetail);
	debugStream << endl;
	return debugStream;
}

bool AJAAncillaryData_HDMI_Aux::isHDMIAuxInfoFrame(void) const
{
	return m_auxType & 0x80;
}
