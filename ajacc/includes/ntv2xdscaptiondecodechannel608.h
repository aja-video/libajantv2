/**
	@file		ntv2xdscaptiondecodechannel608.h
	@brief		Declares the CNTV2XDSDecodeChannel608 class.
	@copyright	(C) 2006-2022 AJA Video Systems, Inc. All rights reserved.
**/

#ifndef __NTV2_CEA608_DECODEXDSCHANNEL_
#define __NTV2_CEA608_DECODEXDSCHANNEL_

#include "ntv2captionlogging.h"
#include "ntv2caption608types.h"
#include "ajabase/common/ajarefptr.h"
#include <string>

#ifdef MSWindows
	#include "windows.h"
	#include "stdio.h"
#elif defined (AJAMac)
	#pragma GCC diagnostic ignored "-Wunused-private-field"
#endif


//	CEA-608 Extended Data Service (XDS) Classes
typedef enum NTV2_CC608_XDSClass
{
	NTV2_CC608_XDSUnknownClass,
	NTV2_CC608_XDSCurrentClass,
	NTV2_CC608_XDSFutureClass,
	NTV2_CC608_XDSChannelClass,
	NTV2_CC608_XDSMiscClass,
	NTV2_CC608_XDSPublicServiceClass,
	NTV2_CC608_XDSReservedClass,
	NTV2_CC608_XDSPrivateDataClass,

	NTV2_CC608_XDSNumClasses

} NTV2_CC608_XDSClass;


//	CEA-608 Extended Data Service (XDS) Types
typedef enum NTV2_CC608_XDSType
{
	NTV2_CC608_XDSUnknownType,
		
	//	"Current Class" Packet Types
	NTV2_CC608_XDSProgramIDNumberType,
	NTV2_CC608_XDSLengthTimeInShowType,
	NTV2_CC608_XDSProgramNameType,
	NTV2_CC608_XDSProgramTypeType,
	NTV2_CC608_XDSContentAdvisoryType,
	NTV2_CC608_XDSAudioServicesType,
	NTV2_CC608_XDSCaptionServicesType,
	NTV2_CC608_XDSCopyRedistributionType,
	NTV2_CC608_XDSCompositePacket1Type,
	NTV2_CC608_XDSCompositePacket2Type,
	NTV2_CC608_XDSProgramDescRow1Type,
	NTV2_CC608_XDSProgramDescRow2Type,
	NTV2_CC608_XDSProgramDescRow3Type,
	NTV2_CC608_XDSProgramDescRow4Type,
	NTV2_CC608_XDSProgramDescRow5Type,
	NTV2_CC608_XDSProgramDescRow6Type,
	NTV2_CC608_XDSProgramDescRow7Type,
	NTV2_CC608_XDSProgramDescRow8Type,

	//	"Channel Class" Packet Types
	NTV2_CC608_XDSNetworkNameType,
	NTV2_CC608_XDSCallLettersType,
	NTV2_CC608_XDSTapeDelayType,
	NTV2_CC608_XDSTransmissionSignalIDType,

	//	"Miscellaneous Class" Packet Types
	NTV2_CC608_XDSTimeOfDayType,
	NTV2_CC608_XDSImpulseCaptureIDType,
	NTV2_CC608_XDSSupplementalDataLocationType,
	NTV2_CC608_XDSLocalTimeZoneType,
	NTV2_CC608_XDSOutOfBandChannelType,
	NTV2_CC608_XDSChannelMapPointerType,
	NTV2_CC608_XDSChannelMapHeaderType,
	NTV2_CC608_XDSChannelMapType,

	//	"Public Service Class" Types
	NTV2_CC608_XDSNWSCode,
	NTV2_CC608_XDSNWSMessage,

	NTV2_CC608_XDSNumTypes

} NTV2_CC608_XDSType;


/**
	@brief	I decode CEA-608 XDS ("eXtended Data Service") data.
			I do little more than decode and store the XDS parameters, and am mostly used as a debug/learning exercise.
			However, methods could be added to "get" the received data if that would be useful.

			Clients should call NewData each video frame when it has XDS data to decode.
			(It is the client's reponsibility to determine when CEA-608 "Line 21" data is XDS data.)

	@todo	One improvement that could be added if someone cares:
				The current implementation only single-buffers the received data, so previous data is overwritten
				immediately by new data. If there's an error in reception, the "old" data has already been overwritten,
				so you have garbage until the next good data is received. A more robust implementation would double-buffer
				the received data so that a complete transmision can be checked for correctness before "committing" it to
				permanent storage. This would also eliminate the contention problem that occurs when outside code tries to
				read a parameter while a packet is being received (right now we would only return the partial data that has
				been received to-date.
	@note	There is one CNTV2XDSDecodeChannel608 -- on the assumption that there is only one XDS service.
**/
class CNTV2XDSDecodeChannel608;
typedef AJARefPtr <CNTV2XDSDecodeChannel608>	CNTV2XDSDecodeChannel608Ptr;

class AJAExport CNTV2XDSDecodeChannel608 : public CNTV2CaptionLogConfig
{
	//	CLASS METHODS
	public:
		static bool					Create (CNTV2XDSDecodeChannel608Ptr & outObj);
		static NTV2Line21Channel	GetCurrentChannel (UByte char608_1, UByte char608_2, NTV2Line21Field field);


	//	INSTANCE METHODS
	public:
		virtual				~CNTV2XDSDecodeChannel608 ();

		virtual void		Init (void);
		virtual void		Reset (void);

		virtual bool		NewData (const UByte inByte1, const UByte inByte2, const NTV2Line21Field inField);

		virtual bool		SubscribeChangeNotification (NTV2Caption608Changed * pInCallback, void * pInUserData = NULL);
		virtual bool		UnsubscribeChangeNotification (NTV2Caption608Changed *	pInCallback, void * pInUserData = NULL);


	//	PRIVATE INSTANCE METHODS
	private:
		virtual bool		NewCurrentClassData			(const UByte inByte1, const UByte inByte2, const NTV2Line21Field inField);
		virtual bool		NewFutureClassData			(const UByte inByte1, const UByte inByte2, const NTV2Line21Field inField);
		virtual bool		NewChannelClassData			(const UByte inByte1, const UByte inByte2, const NTV2Line21Field inField);
		virtual bool		NewMiscClassData			(const UByte inByte1, const UByte inByte2, const NTV2Line21Field inField);
		virtual bool		NewPublicServiceClassData	(const UByte inByte1, const UByte inByte2, const NTV2Line21Field inField);
		virtual bool		NewReservedClassData		(const UByte inByte1, const UByte inByte2, const NTV2Line21Field inField);
		virtual bool		NewPrivateDataClassData		(const UByte inByte1, const UByte inByte2, const NTV2Line21Field inField);

		//	Hidden constructors and assignment operator
		explicit			CNTV2XDSDecodeChannel608 ();
		explicit			CNTV2XDSDecodeChannel608 (const CNTV2XDSDecodeChannel608 & inObj);
		virtual CNTV2XDSDecodeChannel608 &	operator = (const CNTV2XDSDecodeChannel608 & inObj);


	//	PRIVATE CLASS METHODS
	private:
		static std::string	GetClassString (const NTV2_CC608_XDSClass theClass);
		static std::string	GetTypeString (const NTV2_CC608_XDSType theType);


	//	INSTANCE DATA
	private:
		NTV2_CC608_XDSClass	mCurrClass;
		NTV2_CC608_XDSType	mCurrType;

		UByte	mProgramIDNumberData [4];			///< @brief	Data for Current Class: Program Identification Number Packet
		int		mProgramIDNumberCount;

		UByte	mLengthTimeInShowData [6];			///< @brief	Data for Current Class: Length/Time-in-Show Packet
		int		mLengthTimeInShowCount;

		char	mProgramNameStr [33];				///< @brief	String for Current Class: Program Name Packet

		UByte	mProgramTypeData [2];				///< @brief	Data for Current Class: Program Type Packet
		int		mProgramTypeCount;

		UByte	mContentAdvisoryData [2];			///< @brief	Data for Current Class: Content Advisory Packet
		int		mContentAdvisoryCount;

		UByte	mAudioServicesData [2];				///< @brief	Data for Current Class: Audio Services Packet
		int		mAudioServicesCount;

		UByte	mCaptioningServicesData [8];		///< @brief	Data for Current Class: Captioning Services Packet
		int		mCaptioningServicesCount;

		UByte	mCopyRedistributionData [2];		///< @brief	Data for Current Class: Copy and Redistribution Packet
		int		mCopyRedistributionCount;

		UByte	mCompositePacket1Data [33];			///< @brief	Data for Current Class: Composite Packet #1
		int		mCompositePacket1Count;

		UByte	mCompositePacket2Data [33];			///< @brief	Data for Current Class: Composite Packet #2
		int		mCompositePacket2Count;

		char	mProgramDescriptionRow1Str [33];	///< @brief	String for Current Class: Program Description Row #1
		char	mProgramDescriptionRow2Str [33];	///< @brief	String for Current Class: Program Description Row #2
		char	mProgramDescriptionRow3Str [33];	///< @brief	String for Current Class: Program Description Row #3
		char	mProgramDescriptionRow4Str [33];	///< @brief	String for Current Class: Program Description Row #4
		char	mProgramDescriptionRow5Str [33];	///< @brief	String for Current Class: Program Description Row #5
		char	mProgramDescriptionRow6Str [33];	///< @brief	String for Current Class: Program Description Row #6
		char	mProgramDescriptionRow7Str [33];	///< @brief	String for Current Class: Program Description Row #7
		char	mProgramDescriptionRow8Str [33];	///< @brief	String for Current Class: Program Description Row #8


		char	mNetworkNameStr [33];				///< @brief	String for Channel Class: Network Name Packet

		UByte	mCallLettersData [6];				///< @brief	Data for Channel Class: Call Letters/Native Channel Packet
		int		mCallLettersCount;

		UByte	mTapeDelayData [2];					///< @brief	Data for Channel Class: Tape Delay Packet
		int		mTapeDelayCount;

		UByte	mTransmissionSignalIDData [4];		///< @brief	Data for Channel Class: Transmission Signal ID Packet
		int		mTransmissionSignalIDCount;


		UByte	mTimeOfDayData [6];					///< @brief	Data for Misc Class: Time-of-Day Packet
		int		mTimeOfDayCount;

		UByte	mImpulseCaptureIDData [6];			///< @brief	Data for Misc Class: Impulse Capture ID Packet
		int		mImpulseCaptureIDCount;

		UByte	mSupplementalDataLocationData [32];	///< @brief	Data for Misc Class: Supplemental Data Location Packet
		int		mSupplementalDataLocationCount;

		UByte	mLocalTimeZoneData [2];				///< @brief	Data for Misc Class: Local Time Zone Packet
		int		mLocalTimeZoneCount;

		UByte	mOutOfBandChannelNumberData [2];	///< @brief	Data for Misc Class: Out of Band Channel Number Packet
		int		mOutOfBandChannelNumberCount;

		UByte	mChannelMapPointerData [2];			///< @brief	Data for Misc Class: Channel Map Pointer Packet
		int		mChannelMapPointerCount;

		UByte	mChannelMapHeaderData [4];			///< @brief	Data for Misc Class: Channel Map Header Packet
		int		mChannelMapHeaderCount;

		UByte	mChannelMapData [10];				///< @brief	Data for Misc Class: Channel Map Packet
		int		mChannelMapCount;

		char	mNWSCodeStr [33];					///< @brief	String for Public Service Class: National Weather Service Code
		char	mNWSMessageStr [33];				///< @brief	String for Public Service Class: National Weather Service Message

		int		mDebugPrintOffset;					///< @brief	Offset added to debug levels (used in cases where there are multiple instances,

		NTV2Caption608Changed *	mpCallback;			///< @brief	Change notification callback, if any
		void *					mpSubscriberData;	///< @brief	Change notification user data, if any

};	//	CNTV2XDSDecodeChannel608

#endif	// __NTV2_CEA608_DECODEXDSCHANNEL_
