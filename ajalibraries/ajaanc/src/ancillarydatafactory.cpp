/**
	@file		ancillarydatafactory.cpp
	@brief		Implementation of the AJAAncillaryDataFactory class.
	@copyright	(C) 2010-2016 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#include "ancillarydatafactory.h"
#include "ancillarydata.h"
#include "ancillarydata_timecode_atc.h"
#include "ancillarydata_timecode_vitc.h"
#include "ancillarydata_cea708.h"
#include "ancillarydata_cea608_vanc.h"
#include "ancillarydata_cea608_line21.h"
#include "ancillarydata_framestatusinfo524D.h"
#include "ancillarydata_framestatusinfo5251.h"
//#include "ancillarydata_smpte352.h"
//#include "ancillarydata_smpte2016-3.h"
//#include "ancillarydata_smpte2051.h"



AJAAncillaryData *
AJAAncillaryDataFactory::Create(AJAAncillaryDataType ancType, AJAAncillaryData *pAncData)
{
	switch (ancType)
	{
		case AJAAncillaryDataType_Unknown:				// if we don't recognize a specific derived type, instantiate the base class
			return new AJAAncillaryData(pAncData);
			break;

// 		case AJAAncillaryDataType_Smpte2016_3:	
// 			return new AJAAncillaryData_Smpte2016_3(pAncData);
// 			break;
// 
		case AJAAncillaryDataType_Timecode_ATC:	
			return new AJAAncillaryData_Timecode_ATC(pAncData);
			break;

		case AJAAncillaryDataType_Timecode_VITC:	
			return new AJAAncillaryData_Timecode_VITC(pAncData);
			break;

		case AJAAncillaryDataType_Cea708:
			return new AJAAncillaryData_Cea708(pAncData);
			break;

		case AJAAncillaryDataType_Cea608_Vanc:
			return new AJAAncillaryData_Cea608_Vanc(pAncData);
			break;

		case AJAAncillaryDataType_Cea608_Line21:
			return new AJAAncillaryData_Cea608_Line21(pAncData);
			break;

// 		case AJAAncillaryDataType_Smpte352:
// 			return new AJAAncillaryData_Smpte352(pAncData);
// 			break;
// 
// 		case AJAAncillaryDataType_Smpte2051:
// 			return new AJAAncillaryData_Smpte2051(pAncData);
// 			break;
// 
		case AJAAncillaryDataType_FrameStatusInfo524D:
			return new AJAAncillaryData_FrameStatusInfo524D(pAncData);

		case AJAAncillaryDataType_FrameStatusInfo5251:
			return new AJAAncillaryData_FrameStatusInfo5251(pAncData);

		default:
			return NULL;
			break;
	}
}


//---------------------------------
// Given a "raw" (unparsed) AJAAncillaryData object, see if we can identify it based on its contents (e.g. DID, SID, location, etc.)
// (Architectural note: while this isn't a "factory" method, per se, it's convenient to be here because the factory is the one thing
// that knows about ALL classes, and you're often calling this as a prelude to creating a new object of the appropriate type using
// the factory.)
//

AJAAncillaryDataType
AJAAncillaryDataFactory::GuessAncillaryDataType(AJAAncillaryData *pAncData)
{
	AJAAncillaryDataType result = AJAAncillaryDataType_Unknown;

		// walk through each of the known derived AncillaryData derived classes
		// and ask them if they recognize this data

// 	if ( (result = AJAAncillaryData_Smpte2016_3::RecognizeThisAncillaryData(pAncData)) != AJAAncillaryDataType_Unknown)
// 		return result;
// 
	if ( (result = AJAAncillaryData_Timecode_ATC::RecognizeThisAncillaryData(pAncData)) != AJAAncillaryDataType_Unknown)
		return result;

	if ( (result = AJAAncillaryData_Timecode_VITC::RecognizeThisAncillaryData(pAncData)) != AJAAncillaryDataType_Unknown)
		return result;

	if ( (result = AJAAncillaryData_Cea708::RecognizeThisAncillaryData(pAncData)) != AJAAncillaryDataType_Unknown)
		return result;

	if ( (result = AJAAncillaryData_Cea608_Vanc::RecognizeThisAncillaryData(pAncData)) != AJAAncillaryDataType_Unknown)
		return result;

	if ( (result = AJAAncillaryData_Cea608_Line21::RecognizeThisAncillaryData(pAncData)) != AJAAncillaryDataType_Unknown)
		return result;

// 	if ( (result = AJAAncillaryData_Smpte352::RecognizeThisAncillaryData(pAncData)) != AJAAncillaryDataType_Unknown)
// 		return result;

// 	if ( (result = AJAAncillaryData_Smpte2051::RecognizeThisAncillaryData(pAncData)) != AJAAncillaryDataType_Unknown)
// 		return result;
// 
	if ( (result = AJAAncillaryData_FrameStatusInfo524D::RecognizeThisAncillaryData(pAncData)) != AJAAncillaryDataType_Unknown)
		return result;

	if ( (result = AJAAncillaryData_FrameStatusInfo5251::RecognizeThisAncillaryData(pAncData)) != AJAAncillaryDataType_Unknown)
		return result;

	return result;
}

