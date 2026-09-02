/**
	@file		ntv2caption708serviceinfo.h
	@brief		Declares the CNTV2Caption708ServiceInfo class.
	@copyright	(C) 2007-2022 AJA Video Systems, Inc. All rights reserved.
**/

#ifndef __NTV2_CEA708_SERVICEINFO_
#define __NTV2_CEA708_SERVICEINFO_

#include "ntv2captionlogging.h"
#ifdef MSWindows
	#include "windows.h"
	#include "stdio.h"
#endif


// CEA-708 Services
const int NTV2_CC708PrimaryCaptionServiceNum = 1;	// service number for standard caption service

const int NTV2_CC708MaxCDPServices = 15;		// the max number of services that can be updated in a single CDP (by CEA-708 law)
const int NTV2_CC708MaxNumServices = 64;		// the max number of services that can be addressed


typedef struct AJAExport NTV2_CC708ServiceLanguage
{
	char	langID[3];		// caption language (e.g. 'ENG')

	explicit NTV2_CC708ServiceLanguage (const char inChar1 = 'e', const char inChar2 = 'n', const char inChar3 = 'g');
	bool	IsEqual (const NTV2_CC708ServiceLanguage & inServiceLanguage) const;
	std::ostream &	Print (std::ostream & inOutStream) const;

} NTV2_CC708ServiceLanguage;

inline std::ostream & operator << (std::ostream & inOutStream, const NTV2_CC708ServiceLanguage & inLanguage)	{return inLanguage.Print (inOutStream);}


// common language codes (see ISO 639.2: www.loc.gov/standards/iso639-2/php/English_list.php
const NTV2_CC708ServiceLanguage NTV2_CC708SvcLang_English	(NTV2_CC708ServiceLanguage ('e', 'n', 'g'));
const NTV2_CC708ServiceLanguage NTV2_CC708SvcLang_French	(NTV2_CC708ServiceLanguage ('f', 'r', 'e'));
const NTV2_CC708ServiceLanguage NTV2_CC708SvcLang_Spanish	(NTV2_CC708ServiceLanguage ('s', 'p', 'a'));


//	CEA-708B CDP Service Info flags  (see CEA-708B, pp 76)
//  NOTE: these bit positions are NOT the same as the svc_info_start/change/complete bits in the CDP Header!
enum
{
	NTV2_CC708CDPSvcInfo_SvcInfoStart	 = (1 << 6),
	NTV2_CC708CDPSvcInfo_SvcInfoChange	 = (1 << 5),
	NTV2_CC708CDPSvcInfo_SvcInfoComplete = (1 << 4)
};

//	CEA-708B CDP Service Info flags  (see ATSC A/65C, pg 71)
enum
{
	NTV2_CC708CDPSvcInfo_DigitalCC	 = (1 << 7),
	NTV2_CC708CDPSvcInfo_Line21Field = (1 << 0)		// note: obsolete
};

//	CEA-708B CDP Service Info flags  (see ATSC A/65C, pg 71)
enum
{
	NTV2_CC708CDPSvcInfo_EasyReader		 = (1 << 7),
	NTV2_CC708CDPSvcInfo_WideAspectRatio = (1 << 6)
};


typedef struct NTV2_CC708ServiceInfo
{
	bool						bSvcActive;				///< @brief	True if service is active and should be included in ccsvcinfo_section
	int							captionSvcNumber;		///< @brief	Service number (Line 21 data = 0; 708 data = 1 - 16)

	//	From caption service descriptor: ATSC A/65 pg 71...
	NTV2_CC708ServiceLanguage	language;				///< @brief	Caption language (e.g. 'ENG')
	bool						digitalCC;				///< @brief	True if 708 captions, false if Line 21 (608)
	UByte						csn_line21field;		///< @brief	The remainder of that byte
	bool						easyReader;
	bool						wideAspect;

	explicit inline	NTV2_CC708ServiceInfo ()		{Zero();}
	void			Zero (void);
	bool			IsEqual (const NTV2_CC708ServiceInfo & inCompareInfo) const;
	std::ostream &	Print (std::ostream & inOutStream) const;

} NTV2_CC708ServiceInfo;


inline std::ostream & operator << (std::ostream & inOutStream, const NTV2_CC708ServiceInfo & inInfo)	{return inInfo.Print (inOutStream);}


typedef struct NTV2_CC708ServiceData
{
	bool					bChange;
	NTV2_CC708ServiceInfo	serviceInfo [NTV2_CC708MaxNumServices];

	std::ostream &	Print (std::ostream & inOutStream) const;

} NTV2_CC708ServiceData;


inline std::ostream & operator << (std::ostream & inOutStream, const NTV2_CC708ServiceData & inData)	{return inData.Print (inOutStream);}


/**
	@brief	I am a container for all of the CEA-708 "Service Information" that a decoder or
			encoder needs to keep track of. This typically means a database containing the current
			status of up 63 "services", as defined in CEA-708 and SMPTE-334.

	@note	By CEA-708 rules, the information for ALL services may be updated in a single CDP, or
			the updates can be spread out across multiple CDPs. This may require using two of these
			objects: one to hold the "current" service information status; and another to accumulate
			the updates as they trickle in. After the last CDP containing service info updates has
			been received, all of the info may be copied at once to update the "current" status.
**/

class AJAExport CNTV2Caption708ServiceInfo : public CNTV2CaptionLogConfig
{
	//	INSTANCE METHODS
	public:
												CNTV2Caption708ServiceInfo ();
		virtual									~CNTV2Caption708ServiceInfo ();

		virtual bool							InitAllServiceInfo (void);
		virtual bool							InitCCServiceInfo (const int inServiceIndex);

		virtual bool							CopyAllServiceInfo (const NTV2_CC708ServiceData & inSrcSvcInfo);
		virtual inline const NTV2_CC708ServiceData &	GetAllServiceInfoPtr (void) const							{return m_serviceData;}
		virtual bool							CompareAllServiceInfo (const NTV2_CC708ServiceData & inSrcSvcData) const;

		virtual bool							CopyOneServiceInfo (const int inServiceIndex, const NTV2_CC708ServiceInfo & inSrcSvcInfo);
		virtual const NTV2_CC708ServiceInfo &	GetOneServiceInfoPtr (const int inServiceIndex) const;
		virtual bool							CompareOneServiceInfo (const int inServiceIndex, const NTV2_CC708ServiceInfo & inSrcSvcInfo) const;

		virtual int								NumActiveCDPServiceInfo (const int startIndex = 0);
		virtual bool							ResetStartIndex (void);

		virtual inline int						GetStartIndex (void) const											{return m_startIndex;}

		virtual int								AdvanceToNextStartIndex (const bool bIncludeCurrentIndex);

		virtual bool							SetServiceInfoActive (const int inServiceIndex, const bool inIsActive);

		virtual inline bool						GetServiceInfoActive (const int inServiceIndex) const
												{
													return (inServiceIndex >= 0 && inServiceIndex < NTV2_CC708MaxNumServices) ? m_serviceData.serviceInfo [inServiceIndex].bSvcActive : false;
												}

		virtual inline int						GetCaptionServiceNumber (const int inServiceIndex) const
												{
													return (inServiceIndex >= 0 && inServiceIndex < NTV2_CC708MaxNumServices) ? m_serviceData.serviceInfo [inServiceIndex].captionSvcNumber : 0;
												}

		virtual bool							SetServiceInfoLanguage (const int inServiceIndex, const NTV2_CC708ServiceLanguage & inNewLang);

		virtual bool							GetServiceInfoLanguage (const int inServiceIndex, NTV2_CC708ServiceLanguage & outNewLang) const;

		virtual bool							SetServiceInfoEasyReader (const int inServiceIndex, const bool inIsEasyReader);

		virtual inline bool						GetServiceInfoEasyReader (const int inServiceIndex)
												{
													return (inServiceIndex >= 0 && inServiceIndex < NTV2_CC708MaxNumServices) ? m_serviceData.serviceInfo [inServiceIndex].easyReader : false;
												}

		virtual bool							SetServiceInfoWideAspect (const int inServiceIndex, const bool inIsWideAspect);

		virtual inline bool						GetServiceInfoWideAspect (const int inServiceIndex) const
												{
													return (inServiceIndex >= 0 && inServiceIndex < NTV2_CC708MaxNumServices) ? m_serviceData.serviceInfo [inServiceIndex].wideAspect : false;
												}

		virtual bool							SetServiceInfoDigitalCC (const int inServiceIndex, const bool inIsDigitalCC);

		virtual inline bool						GetServiceInfoDigitalCC (const int inServiceIndex) const
												{
													return (inServiceIndex >= 0 && inServiceIndex < NTV2_CC708MaxNumServices) ? m_serviceData.serviceInfo [inServiceIndex].digitalCC : false;
												}

		virtual bool							SetServiceInfoChangeFlag (const bool inChangeFlag);

		virtual inline bool						GetServiceInfoChangeFlag (void) const
												{
													return m_serviceData.bChange;
												}

		// Debug
		virtual std::ostream &					Print (std::ostream & inOutStream) const;


	//	INSTANCE DATA
	private:
		int							m_startIndex;	///< @brief	My starting index value
		NTV2_CC708ServiceData		m_serviceData;	///< @brief	My service data

};	//	CNTV2Caption708ServiceInfo


std::ostream & operator << (std::ostream & inOutStream, const CNTV2Caption708ServiceInfo & inInfo);


#endif	// __NTV2_CEA708_SERVICEINFO_
