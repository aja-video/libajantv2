/**
	@file		ntv2caption708serviceinfo.cpp
	@brief		Implementation of the CNTV2Caption708ServiceInfo class.
	@copyright	(C) 2007-2022 AJA Video Systems, Inc. All rights reserved.
**/

#include "ntv2caption708serviceinfo.h"
#include "ajabase/system/debug.h"
#include <iomanip>
#include <string.h>	//	for memset


using namespace std;


static unsigned	gInstanceTally	(0);


NTV2_CC708ServiceLanguage::NTV2_CC708ServiceLanguage (const char inChar1, const char inChar2, const char inChar3)
{
	langID[0] = inChar1;	langID[1] = inChar2;	langID[2] = inChar3;
}


bool NTV2_CC708ServiceLanguage::IsEqual (const NTV2_CC708ServiceLanguage & inServiceLanguage) const
{
	return langID[0] == inServiceLanguage.langID[0] && langID[1] == inServiceLanguage.langID[1] && langID[2] == inServiceLanguage.langID[2];
}


ostream & NTV2_CC708ServiceLanguage::Print (ostream & inOutStream) const
{
	char	str [4]	=	{langID[0], langID[1], langID[2], 0};
	return inOutStream << "'" << str << "'";
}


void NTV2_CC708ServiceInfo::Zero (void)
{
	bSvcActive = digitalCC = easyReader = wideAspect = false;
	captionSvcNumber = 0;
	language = NTV2_CC708SvcLang_English;
	csn_line21field = 0;
}

bool NTV2_CC708ServiceInfo::IsEqual (const NTV2_CC708ServiceInfo & inCompareInfo) const
{
	return	bSvcActive			== inCompareInfo.bSvcActive			&&
			captionSvcNumber	== inCompareInfo.captionSvcNumber	&&
			csn_line21field		== inCompareInfo.csn_line21field	&&
			digitalCC			== inCompareInfo.digitalCC			&&
			easyReader			== inCompareInfo.easyReader			&&
			wideAspect			== inCompareInfo.wideAspect			&&
			language.IsEqual (inCompareInfo.language);
}


ostream & NTV2_CC708ServiceInfo::Print (ostream & inOutStream) const
{
	inOutStream	<< "       bSvcActive="	<< (bSvcActive ? "Y" : "N")	<< endl
				<<	"captionSvcNumber="	<< captionSvcNumber			<< endl
				<<	" csn_line21field="	<< csn_line21field			<< endl
				<<	"       digitalCC="	<< (digitalCC ? "Y" : "N")	<< endl
				<<	"      easyReader="	<< (easyReader ? "Y" : "N")	<< endl
				<<	"        language="	<< language					<< endl
				<<	"      wideAspect="	<< (wideAspect ? "Y" : "N");
	return inOutStream;
}


ostream & NTV2_CC708ServiceData::Print (ostream & inOutStream) const
{
	inOutStream	<< "bChange="	<< (bChange ? "Y" : "N")	<< endl;
	for (int svcNdx (0);  svcNdx < NTV2_CC708MaxNumServices;  svcNdx++)
		inOutStream	<< "Service [" << svcNdx << "]:" << endl << serviceInfo[svcNdx];
	return inOutStream;
}



/////////////////////////////////////////////////////////////////////////////
// Constructor
//
CNTV2Caption708ServiceInfo::CNTV2Caption708ServiceInfo (void)
	:	m_startIndex	(0)
{
	gInstanceTally++;
	InitAllServiceInfo();

}	//	constructor


CNTV2Caption708ServiceInfo::~CNTV2Caption708ServiceInfo ()
{
}	//	destructor


// InitAllServiceInfo()
//
bool CNTV2Caption708ServiceInfo::InitAllServiceInfo (void)
{
	m_serviceData.bChange = true;

	for (int i(0);  i < NTV2_CC708MaxNumServices;   i++)
		InitCCServiceInfo(i);

	ostringstream	oss;
	oss << "Caption708ServiceInfo-" << gInstanceTally;
	SetLogLabel(oss.str());
	return true;

}	//	InitAllServiceInfo


// InitCCServiceInfo
//		Clear a single CNTV2ServiceInfo struct	
//
bool CNTV2Caption708ServiceInfo::InitCCServiceInfo (const int inServiceIndex)
{
	if (inServiceIndex < 0 || inServiceIndex >= NTV2_CC708MaxNumServices)
		return false;

	NTV2_CC708ServiceInfo &	pSvcInfo	(m_serviceData.serviceInfo[inServiceIndex]);

	pSvcInfo.bSvcActive			= false;					// default = not active
	pSvcInfo.captionSvcNumber	= inServiceIndex;			// service number (Line 21 data = 0; 708 data = 1 - 63)

	// from caption service descriptor: ATSC A/65 pg 71
	pSvcInfo.language   = NTV2_CC708SvcLang_English;		// default = English
	pSvcInfo.digitalCC  = (inServiceIndex == 0 ? false : true);	// use index 0 for Line 21 captions, all the rest are 708
	pSvcInfo.easyReader = false;
	pSvcInfo.wideAspect = false;
	return true;

}	//	InitCCServiceInfo



//---------------- Set/Get All Services -----------------

// CopyAllServiceInfo()
//	Copies an entire m_serviceInfo array from another CNTV2Caption708ServiceInfo object to this one.
//	Returns 'true' if there has been a change in the service info data
//
bool CNTV2Caption708ServiceInfo::CopyAllServiceInfo (const NTV2_CC708ServiceData & inSrcSvcData)
{
	m_serviceData = inSrcSvcData;	//	::memcpy (&m_serviceData, &inSrcSvcData, sizeof (NTV2_CC708ServiceData));

	//	If there has been a change to the database, reset the startIndex and set the "change" flag...
	const bool	changed	(CompareAllServiceInfo(inSrcSvcData));
	if (changed)
	{
		ResetStartIndex();
		m_serviceData.bChange = true;
	}
	return changed;

}	//	CopyAllServiceInfo


// CompareAllServiceInfo()
//		Compare contents of given NTV2_CC708ServiceData with local database - return true if different
//
bool CNTV2Caption708ServiceInfo::CompareAllServiceInfo (const NTV2_CC708ServiceData & inSrcSvcData) const
{
	//	NOTE:	Theoretically "bChange" is supposed to tell us this, but I don't believe it...
	for (int svcIndex(0);  svcIndex < NTV2_CC708MaxNumServices;  svcIndex++)
		if (CompareOneServiceInfo (svcIndex, inSrcSvcData.serviceInfo[svcIndex]))
			return true;		//	Once we have one deviant, we know the final answer...

	return false;

}	//	CompareAllServiceInfo



//---------------- Set/Get One Service -----------------


// CopyOneServiceInfo()
//	Copies an entire m_serviceInfo array from another CNTV2Caption708ServiceInfo object to this one
//
bool CNTV2Caption708ServiceInfo::CopyOneServiceInfo (const int inServiceIndex, const NTV2_CC708ServiceInfo & inSrcSvcInfo)
{
	if (inServiceIndex < 0  ||  inServiceIndex >= NTV2_CC708MaxNumServices)
		return false;

	m_serviceData.serviceInfo[inServiceIndex] = inSrcSvcInfo;	//	Struct copy
	return true;

}	//	CopyOneServiceInfo


// GetOneServiceInfoPtr()
//
const NTV2_CC708ServiceInfo & CNTV2Caption708ServiceInfo::GetOneServiceInfoPtr (const int inServiceIndex) const
{
	static const NTV2_CC708ServiceInfo nullResult;

	//	Sanity check...
	if (inServiceIndex < 0  ||  inServiceIndex >= NTV2_CC708MaxNumServices)
		return nullResult;

	return m_serviceData.serviceInfo[inServiceIndex];

}	//	GetOneServiceInfoPtr


// CompareOneServiceInfo()
//		Compare contents of given NTV2_CC708ServiceInfo with local database - return true if different
//
bool CNTV2Caption708ServiceInfo::CompareOneServiceInfo (const int inServiceIndex, const NTV2_CC708ServiceInfo & inSrcSvcInfo) const
{
	if (inServiceIndex < 0  ||  inServiceIndex >= NTV2_CC708MaxNumServices)
		return true;	//	return "different" for invalid svc index

	return !m_serviceData.serviceInfo[inServiceIndex].IsEqual(inSrcSvcInfo);

}	//	CompareOneServiceInfo



//---------------- Service Enumeration Methods ---------------

// NumActiveCDPServiceInfo
//		Returns the number of services whose "active" flags are set, starting with (and including) service #startIndex.
//	Set startIndex to '0' (or use default) to get a count of all active services.
//
int CNTV2Caption708ServiceInfo::NumActiveCDPServiceInfo (int startIndex)
{
	int	result (0);
	if (startIndex >= 0  &&  startIndex < NTV2_CC708MaxNumServices)
		for (int i(startIndex);  i < NTV2_CC708MaxNumServices;  i++)
			if (m_serviceData.serviceInfo[i].bSvcActive)
				result++;
	return result;

}	//	NumActiveCDPServiceInfo


// ResetStartIndex()
//		Resets m_startIndex to '0'. Usually done after a change has been made to the
//	Service Info database so we can start rescanning from the beginning again.
//
bool CNTV2Caption708ServiceInfo::ResetStartIndex (void)
{
	m_startIndex = 0;
	return true;

}	//	ResetStartIndex


// AdvanceToNextStartIndex()
//		Advance m_startIndex to the next active Service in the database. If bIncludeCurrentIndex
//	is true, the search will begin with the current value of m_startIndex (usually used at the 
//	beginning of an enumeration). If bIncludeCurrentIndex is false, the search will begin with
//	with m_startIndex + 1.
//		If there are no more active services to be found, this method returns "-1" and m_startIndex
//	is reset to 0. The caller needs to test for this and take evasive action!
//
int	CNTV2Caption708ServiceInfo::AdvanceToNextStartIndex (const bool bIncludeCurrentIndex)
{
	int			result	(-1);
	const int	start	(bIncludeCurrentIndex  ?  m_startIndex  :  m_startIndex + 1);

	for (int svcIndex(start);  svcIndex < NTV2_CC708MaxNumServices;  svcIndex++)
		if (m_serviceData.serviceInfo[svcIndex].bSvcActive)
		{
			result = svcIndex;
			break;
		}

	m_startIndex = (result < 0)  ?  0  :  result;
	return result;

}	//	AdvanceToNextStartIndex



//---------------- Set/Get Data within each ServiceInfo -----------------


// SetServiceInfoActive
//		Set the "bActive" flag for the designated Service Info
//
bool CNTV2Caption708ServiceInfo::SetServiceInfoActive (const int inServiceIndex, bool bActive)
{
	if (inServiceIndex < 0  ||  inServiceIndex >= NTV2_CC708MaxNumServices)
		return false;

	m_serviceData.serviceInfo[inServiceIndex].bSvcActive = bActive;
	return true;

}	//	SetServiceInfoActive


// SetServiceInfoLanguage
//		Set the language type for the designated Service Info
//
bool CNTV2Caption708ServiceInfo::SetServiceInfoLanguage (const int inServiceIndex, const NTV2_CC708ServiceLanguage & inNewLang)
{
	if (inServiceIndex < 0  ||  inServiceIndex >= NTV2_CC708MaxNumServices)
		return false;

	m_serviceData.serviceInfo[inServiceIndex].language = inNewLang;	//	Struct copy
	return true;

}	//	SetServiceInfoLanguage


// GetServiceInfoLanguage
//		Set the language type for the designated Service Info
//
bool CNTV2Caption708ServiceInfo::GetServiceInfoLanguage (const int inServiceIndex, NTV2_CC708ServiceLanguage & outNewLang) const
{
	if (inServiceIndex < 0  ||  inServiceIndex >= NTV2_CC708MaxNumServices)
		return false;

	outNewLang = m_serviceData.serviceInfo[inServiceIndex].language;	// struct copy
	return true;

}	//	GetServiceInfoLanguage


// SetServiceInfoEasyReader
//		Sets the "easy reader" flag for the designated Service Info
//
bool CNTV2Caption708ServiceInfo::SetServiceInfoEasyReader (const int inServiceIndex, const bool inIsEasyReader)
{
	if (inServiceIndex < 0  ||  inServiceIndex >= NTV2_CC708MaxNumServices)
		return false;

	m_serviceData.serviceInfo[inServiceIndex].easyReader = inIsEasyReader;
	return true;

}	//	SetServiceInfoEasyReader


// SetServiceInfoWideAspect
//		Sets the "wide aspect" flag for the designated Service Info
//
bool CNTV2Caption708ServiceInfo::SetServiceInfoWideAspect (const int inServiceIndex, const bool inIsWideAspect)
{
	if (inServiceIndex < 0  ||  inServiceIndex >= NTV2_CC708MaxNumServices)
		return false;

	m_serviceData.serviceInfo[inServiceIndex].wideAspect = inIsWideAspect;
	return true;

}	//	SetServiceInfoWideAspect


// SetServiceInfoDigitalCC()
//		Sets the "DigitalCC" flag for the designated Service Info
//
bool CNTV2Caption708ServiceInfo::SetServiceInfoDigitalCC (const int inServiceIndex, const bool inIsDigitalCC)
{
	if (inServiceIndex < 0  ||  inServiceIndex >= NTV2_CC708MaxNumServices)
		return false;

	m_serviceData.serviceInfo[inServiceIndex].digitalCC = inIsDigitalCC;
	return true;

}	//	SetServiceInfoDigitalCC


// SetServiceInfoChangeFlag()
//		Sets the "Change" flag for the DataBase
//
bool CNTV2Caption708ServiceInfo::SetServiceInfoChangeFlag (const bool inChangeFlag)
{
	m_serviceData.bChange = inChangeFlag;
	return true;

}	//	SetServiceInfoChangeFlag


//-------------- Debug ----------------

ostream & CNTV2Caption708ServiceInfo::Print (ostream & inOutStream) const
{
	inOutStream	<< "      bChange: " << (m_serviceData.bChange ? "true" : "false") << endl
				<< "      Services:" << endl;
	for (int i(0);  i < NTV2_CC708MaxNumServices;  i++)
	{
		if (m_serviceData.serviceInfo[i].bSvcActive)
		{
			inOutStream	<< "         " << setw (2) << setfill ('0') << m_serviceData.serviceInfo[i].captionSvcNumber << dec << ": active  '"
						<< (m_serviceData.serviceInfo[i].language.langID[0] ? m_serviceData.serviceInfo[i].language.langID[0] : ' ')
						<< (m_serviceData.serviceInfo[i].language.langID[1] ? m_serviceData.serviceInfo[i].language.langID[1] : ' ')
						<< (m_serviceData.serviceInfo[i].language.langID[2] ? m_serviceData.serviceInfo[i].language.langID[2] : ' ')
						<< "'  " << (m_serviceData.serviceInfo[i].digitalCC  ? "digitalCC"  : "")
						<< " " << (m_serviceData.serviceInfo[i].easyReader ? "EasyReader" : "")
						<< " " << (m_serviceData.serviceInfo[i].wideAspect ? "WideAspect" : "") << endl;
		}
		else
			inOutStream	<< "         " << setw (2) << setfill ('0') << m_serviceData.serviceInfo[i].captionSvcNumber << dec << ": off" << endl;
	}
	return inOutStream;

}	//	DebugPrintServiceInfoStatus


ostream & operator << (ostream & inOutStream, const CNTV2Caption708ServiceInfo & inInfo)
{
	return inInfo.Print(inOutStream);
}
