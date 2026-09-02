/**
	@file		ntv2xdscaptiondecodechannel608.cpp
	@brief		Implementation of the CNTV2XDSDecodeChannel608 classes.
	@details	This file contains code for CNTV2XDSDecodeChannel608 that decodes CEA-608 "XDS" (eXtended Data Service)
				data. See below for detailed descriptions of each.
	@copyright	(C) 2007-2022 AJA Video Systems, Inc. All rights reserved.
**/


#include "ntv2xdscaptiondecodechannel608.h"
#include "ccfont.h"
#include <sstream>
#include <map>
#include <iomanip>
#include <string.h>	//	for memset

using namespace std;


/////////////////////////////////////////////////////////////////////////////
// Line21Decoder definition
/////////////////////////////////////////////////////////////////////////////
#if defined (MSWindows)
	#pragma warning(disable: 4800) 
#endif

#define	DEC02(__x__)		dec << setw (2) << setfill ('0') << (__x__)


static unsigned			gXDSInstanceTally	(0);



//****************************************************************************************************************
//
//	CNTV2XDSDecodeChannel608
//
//****************************************************************************************************************

/*
		This module implements a decoder for a CEA-608 XDS ("eXtended Data Service") data.
	In its current state, it does little more than decode and store the XDS parameters, and is
	mostly used as a debug/learning exercise. However, methods could be added to "get" the
	received data if that would be useful.

		Outside code should call NewData() each video frame when it has XDS data to decode
	(it is the reponsibility of the outside code to determine when CEA-608 ("Line 21") data
	is XDS data).


		One improvement that could be added if someone cares: the current implementation only
	single-buffers the received data, so previous data is overwritten immediately by new data.
	If there is an error in reception, the "old" data has already been overwritten, so you have
	garbage until the next good data is received. A more robust implementation would double-buffer
	the received data so that a complete transmision can be checked for correctness before
	"committing" it to permanent storage. This would also eliminate the contention problem
	that occurs when outside code tries to read a parameter while a packet is being received
	(right now we would only return the partial data that has been received to-date).

 */

#define XDS_DEBUG	TestLogMask(kCaptionLog_DecodeXDS)


bool CNTV2XDSDecodeChannel608::Create (CNTV2XDSDecodeChannel608Ptr & outObj)
{
	outObj = NULL;

	try
	{
		outObj = new CNTV2XDSDecodeChannel608;
	}
	catch (const std::bad_alloc &)
	{
	}
	return outObj;

}	//	Create



/////////////////////////////////////////////////////////////////////////////
// Constructor
//
CNTV2XDSDecodeChannel608::CNTV2XDSDecodeChannel608 (void)
{
	++gXDSInstanceTally;
	ostringstream	oss;	oss << "XDSDecodeChannel608-" << gXDSInstanceTally;
	SetLogLabel (oss.str ());
	Init ();

}	//	constructor


CNTV2XDSDecodeChannel608::CNTV2XDSDecodeChannel608 (const CNTV2XDSDecodeChannel608 & inObj)
	: CNTV2CaptionLogConfig ()
{
	(void) inObj;
	AJACC_ASSERT (false && "hidden copy constructor");

}	//	copy constructor


CNTV2XDSDecodeChannel608::~CNTV2XDSDecodeChannel608 ()
{
}	//	destructor


CNTV2XDSDecodeChannel608 &	CNTV2XDSDecodeChannel608::operator = (const CNTV2XDSDecodeChannel608 & inObj)
{
	(void) inObj;
	AJACC_ASSERT (false && "hidden assignment operator");
	return *this;

}	//	assignment operator


// Init()
//		Set all internal params to something "normal"
//
void CNTV2XDSDecodeChannel608::Init (void)
{
	mCurrClass = NTV2_CC608_XDSUnknownClass;
	mCurrType  = NTV2_CC608_XDSUnknownType;

	//	"Current Class" data
	mProgramIDNumberCount	 = 0;
	mLengthTimeInShowCount	 = 0;
	mProgramNameStr[0]		 = '\0';
	mProgramTypeCount		 = 0;
	mContentAdvisoryCount	 = 0;
	mAudioServicesCount		 = 0;
	mCaptioningServicesCount = 0;
	mCopyRedistributionCount = 0;
	mCompositePacket1Count	 = 0;
	mCompositePacket2Count	 = 0;

	mProgramDescriptionRow1Str[0] = '\0';
	mProgramDescriptionRow2Str[0] = '\0';
	mProgramDescriptionRow3Str[0] = '\0';
	mProgramDescriptionRow4Str[0] = '\0';
	mProgramDescriptionRow5Str[0] = '\0';
	mProgramDescriptionRow6Str[0] = '\0';
	mProgramDescriptionRow7Str[0] = '\0';
	mProgramDescriptionRow8Str[0] = '\0';

	//	"Channel Class" data
	mProgramNameStr[0]			= '\0';
	mCallLettersCount			= 0;
	mTapeDelayCount				= 0;
	mTransmissionSignalIDCount	= 0;

	//	"Miscellaneous Class" data
	mTimeOfDayCount					= 0;
	mImpulseCaptureIDCount			= 0;
	mSupplementalDataLocationCount	= 0;
	mLocalTimeZoneCount				= 0;
	mOutOfBandChannelNumberCount	= 0;
	mChannelMapPointerCount			= 0;
	mChannelMapHeaderCount			= 0;
	mChannelMapCount				= 0;

}	//	Init


// Reset()
//
//	This can be called to "flush" the system of any in-progress data.
//	Note: this is NOT guaranteed to be thread-safe.
//
void CNTV2XDSDecodeChannel608::Reset (void)
{
	Init ();

}	//	Reset


// GetCurrentChannel()												(CLASS METHOD)
//		Check the new field caption data bytes to see if a new channel has been selected.
//		Returns the new (or remains on the current) captioning channel for the designated field.
//
NTV2Line21Channel CNTV2XDSDecodeChannel608::GetCurrentChannel (UByte char608_1, UByte char608_2, const NTV2Line21Field inField)
{
	NTV2Line21Channel newChannel = NTV2_CC608_XDS;	//	Assume no change
	bool bTextChan = false;

	//	See if the new input is a command that switches between Caption Mode and Text Mode...
	if (char608_1 == 0x14 || char608_1 == 0x1c || char608_1 == 0x15 || char608_1 == 0x1d)
	{
		switch (char608_2)
		{
			case 0x2a:							// [TR] - Text Restart
			case 0x2b:	bTextChan = true;		// [RTD] - Resume Text Display
						break;

			case 0x20:							// [RCL] - Resume Caption Loading
			case 0x25:							// [RU2] - Roll-Up Captions (2 Rows)
			case 0x26:							// [RU3] - Roll-Up Captions (3 Rows)
			case 0x27:							// [RU4] - Roll-Up Captions (4 Rows)
			case 0x29:							// [RDC] - Resume Direct Captioning
			case 0x2f:	bTextChan = false;		// [EOC] - End of Captions
						break;
		}
	}
		
	//	If the first byte is between 0x10 - 0x17, it is a command word that selects the 1st caption channel...
	if (char608_1 >= 0x10 && char608_1 <= 0x17)
	{
		if (inField == NTV2_CC608_Field1)
			newChannel = (bTextChan ? NTV2_CC608_Text1 : NTV2_CC608_CC1);
		else
			newChannel = (bTextChan ? NTV2_CC608_Text3 : NTV2_CC608_CC3);
	}
	//	If the first byte is between 0x18 - 0x1f, it is a command word that selects the 2nd caption channel...
	else if (char608_1 >= 0x18 && char608_1 <= 0x1f)
	{
		if (inField == NTV2_CC608_Field1)
			newChannel = (bTextChan ? NTV2_CC608_Text2 : NTV2_CC608_CC2);
		else
			newChannel = (bTextChan ? NTV2_CC608_Text4 : NTV2_CC608_CC4);
	}
	//	If the first byte is between 0x01 - 0x0f, it is a command word that selects XDS data (Field 2 only)...
	else if ( (inField == NTV2_CC608_Field2) && char608_1 >= 0x01 && char608_1 <= 0x0f)
		newChannel = NTV2_CC608_XDS;
	//else... no change

	return newChannel;

}	//	GetCurrentChannel


// NewData()
//		Add new incoming XDS data
//
bool CNTV2XDSDecodeChannel608::NewData (const UByte inByte1, const UByte inByte2, const NTV2Line21Field inField)
{
	bool bResult = true;

	switch (inByte1)
	{
		case 0x01:
		case 0x02:	mCurrClass = NTV2_CC608_XDSCurrentClass;		break;

		case 0x03:
		case 0x04:	mCurrClass = NTV2_CC608_XDSFutureClass;			break;

		case 0x05:
		case 0x06:	mCurrClass = NTV2_CC608_XDSChannelClass;		break;

		case 0x07:	
		case 0x08:	mCurrClass = NTV2_CC608_XDSMiscClass;			break;

		case 0x09:
		case 0x0a:	mCurrClass = NTV2_CC608_XDSPublicServiceClass;	break;

		case 0x0b:
		case 0x0c:	mCurrClass = NTV2_CC608_XDSReservedClass;		break;

		case 0x0d:	
		case 0x0e:	mCurrClass = NTV2_CC608_XDSPrivateDataClass;	break;

		case 0x0f:	//	Don't change the Class (yet) - the "End" code needs to be sent to the individual class handlers to let them finish up
			if (XDS_DEBUG)
				Log() << GetLogLabel () << ":   Field " << inField << " [XDS]: 0x" << UHEX2(inByte1) << "  0x" << UHEX2(inByte2) << " - " << GetClassString (mCurrClass) << " Class End, Type = " << GetTypeString (mCurrType) << endl;
			break;
	}

	switch (mCurrClass)
	{
		case NTV2_CC608_XDSCurrentClass:		NewCurrentClassData			(inByte1, inByte2, inField);	break;
		case NTV2_CC608_XDSFutureClass:			NewFutureClassData			(inByte1, inByte2, inField);	break;
		case NTV2_CC608_XDSChannelClass:		NewChannelClassData			(inByte1, inByte2, inField);	break;
		case NTV2_CC608_XDSMiscClass:			NewMiscClassData			(inByte1, inByte2, inField);	break;
		case NTV2_CC608_XDSPublicServiceClass:	NewPublicServiceClassData	(inByte1, inByte2, inField);	break;
		case NTV2_CC608_XDSReservedClass:		NewReservedClassData		(inByte1, inByte2, inField);	break;
		case NTV2_CC608_XDSPrivateDataClass:	NewPrivateDataClassData		(inByte1, inByte2, inField);	break;
		default:								break;
	}

	if (XDS_DEBUG)
	{
		if (inByte1 > 0x00 && inByte1 < 0x0f)
			Log()	<< "## " << GetLogLabel () << ":   Field " << inField << " [XDS]: 0x" << UHEX2(inByte1) << "  0x" << UHEX2(inByte2) << " - " << GetClassString (mCurrClass)
							<< " Class " << ((inByte1 & 0x01) ? "Start" : "Continue") << ", Type = " << GetTypeString (mCurrType) << endl;
		else if (inByte1 >= 0x20 && inByte2 >= 0x20)
			Log()	<< "## " << GetLogLabel () << ":   Field " << inField << " [XDS]: 0x" << UHEX2(inByte1) << "  0x" << UHEX2(inByte2) << " : " << inByte1 << " " << inByte2 << endl;
		else if (inByte1 >= 0x20 && inByte2 == 0)
			Log()	<< "## " << GetLogLabel () << ":   Field " << inField << " [XDS]: 0x" << UHEX2(inByte1) << "  0x" << UHEX2(inByte2) << " : " << inByte1 << endl;
		else if (inByte1 != 0x0f && !(inByte1 == 0 && inByte2 == 0) )
			Log()	<< "## " << GetLogLabel () << ":   Field " << inField << " [XDS]: 0x" << UHEX2(inByte1) << "  0x" << UHEX2(inByte2) << endl;
	}

	return bResult;

}	//	NewData



// NewCurrentClassData()
//		Parse new incoming XDS "Current Programming" Class data 
//
bool CNTV2XDSDecodeChannel608::NewCurrentClassData (const UByte inByte1, const UByte inByte2, const NTV2Line21Field inField)
{
	bool bResult = true;

	(void) inField;
	//char *str = "";

	//	Start (or continuation) of mode?
	if (inByte1 == 0x01 || inByte1 == 0x02)
	{
		mCurrClass = NTV2_CC608_XDSCurrentClass;
		bool bStart = (inByte1 == 0x01);

		switch (inByte2)
		{
			case 0x01:	mCurrType = NTV2_CC608_XDSProgramIDNumberType;
						if (bStart)
						{
							mProgramIDNumberCount = 0;
							for (int i = 0; i < 4; i++)
								mProgramIDNumberData[i] = 0;
						}
						break;

			case 0x02:	mCurrType = NTV2_CC608_XDSLengthTimeInShowType;
						if (bStart)
						{
							mLengthTimeInShowCount = 0;
							for (int i = 0; i < 6; i++)
								mLengthTimeInShowData[i] = 0;
						}
						break;

			case 0x03:	mCurrType = NTV2_CC608_XDSProgramNameType;
						if (bStart)
							mProgramNameStr[0] = '\0';
						break;

			case 0x04:	mCurrType = NTV2_CC608_XDSProgramTypeType;
						if (bStart)
						{
							mProgramTypeCount = 0;
							for (int i = 0; i < 2; i++)
								mProgramTypeData[i] = 0;
						}
						break;

			case 0x05:	mCurrType = NTV2_CC608_XDSContentAdvisoryType;
						if (bStart)
						{
							mContentAdvisoryCount = 0;
							for (int i = 0; i < 2; i++)
								mContentAdvisoryData[i] = 0;
						}
						break;

			case 0x06:	mCurrType = NTV2_CC608_XDSAudioServicesType;
						if (bStart)
						{
							mAudioServicesCount = 0;
							for (int i = 0; i < 2; i++)
								mAudioServicesData[i] = 0;
						}
						break;

			case 0x07:	mCurrType = NTV2_CC608_XDSCaptionServicesType;
						if (bStart)
						{
							mCaptioningServicesCount = 0;
							for (int i = 0; i < 8; i++)
								mCaptioningServicesData[i] = 0;
						}
						break;

			case 0x08:	mCurrType = NTV2_CC608_XDSCopyRedistributionType;
						if (bStart)
						{
							mCopyRedistributionCount = 0;
							for (int i = 0; i < 2; i++)
								mCopyRedistributionData[i] = 0;
						}
						break;

			case 0x0c:	mCurrType = NTV2_CC608_XDSCompositePacket1Type;
						if (bStart)
						{
							mCompositePacket1Count = 0;
							for (int i = 0; i < 33; i++)
								mCompositePacket1Data[i] = 0;
						}
						break;

			case 0x0d:	mCurrType = NTV2_CC608_XDSCompositePacket2Type;
						if (bStart)
						{
							mCompositePacket2Count = 0;
							for (int i = 0; i < 33; i++)
								mCompositePacket2Data[i] = 0;
						}
						break;

			case 0x10:	mCurrType = NTV2_CC608_XDSProgramDescRow1Type;
						if (bStart)
							mProgramDescriptionRow1Str[0] = '\0';
						break;

			case 0x11:	mCurrType = NTV2_CC608_XDSProgramDescRow2Type;
						if (bStart)
							mProgramDescriptionRow2Str[0] = '\0';
						break;

			case 0x12:	mCurrType = NTV2_CC608_XDSProgramDescRow3Type;
						if (bStart)
							mProgramDescriptionRow3Str[0] = '\0';
						break;

			case 0x13:	mCurrType = NTV2_CC608_XDSProgramDescRow4Type;
						if (bStart)
							mProgramDescriptionRow4Str[0] = '\0';
						break;

			case 0x14:	mCurrType = NTV2_CC608_XDSProgramDescRow5Type;
						if (bStart)
							mProgramDescriptionRow5Str[0] = '\0';
						break;

			case 0x15:	mCurrType = NTV2_CC608_XDSProgramDescRow6Type;
						if (bStart)
							mProgramDescriptionRow6Str[0] = '\0';
						break;

			case 0x16:	mCurrType = NTV2_CC608_XDSProgramDescRow7Type;
						if (bStart)
							mProgramDescriptionRow7Str[0] = '\0';
						break;

			case 0x17:	mCurrType = NTV2_CC608_XDSProgramDescRow8Type;
						if (bStart)
							mProgramDescriptionRow8Str[0] = '\0';
						break;

			default:	mCurrType = NTV2_CC608_XDSUnknownType;
						break;
		}
	}
	else if (inByte1 == 0x0f)	//	End of packet?
	{
		switch (mCurrType)
		{
			case NTV2_CC608_XDSProgramIDNumberType:
					if (XDS_DEBUG)
					{
						if (mProgramIDNumberCount == 4)
							Log()	<< "## " << GetLogLabel () << ":      Program ID = " << (mProgramIDNumberData[3] & 0x0f) << "/" << (mProgramIDNumberData[2] & 0x1f)
											<< " " << UHEX2(mProgramIDNumberData[1] & 0x1f) << ":" << UHEX2(mProgramIDNumberData[0] & 0x3f) << endl;
						else
							Log()	<< "## " << GetLogLabel () << ":      Program ID data is incomplete" << endl;
					}
					break;

			case NTV2_CC608_XDSLengthTimeInShowType:
					if (XDS_DEBUG)
					{
						if (mLengthTimeInShowCount >= 2)
						{
							Log()	<< "## " << GetLogLabel () << ":      Program Length = " << DEC02 (mLengthTimeInShowData[1] & 0x3f) << ":" << DEC02 (mLengthTimeInShowData[0] & 0x3f) << endl;
							if (mLengthTimeInShowCount >= 5)
								Log()	<< "      Elapsed Time = " << DEC02(mLengthTimeInShowData[3] & 0x3f) << ":" << DEC02(mLengthTimeInShowData[2] & 0x3f) << ":" << DEC02(mLengthTimeInShowData[4] & 0x3f) << endl;
							if (mLengthTimeInShowCount == 4)
								Log() << "      Elapsed Time = " << DEC02(mLengthTimeInShowData[3] & 0x3f) << ":" << DEC02(mLengthTimeInShowData[2] & 0x3f) << endl;
						}
						else
							Log()	<< "## " << GetLogLabel () << ":      Length/Time-in-Show data is incomplete" << endl;
					}
					break;
		
			case NTV2_CC608_XDSProgramNameType:
					if (XDS_DEBUG)
						Log()	<< "## " << GetLogLabel () << ":      Program Name = " << mProgramNameStr << endl;
					break;
		
			case NTV2_CC608_XDSProgramTypeType:		
					if (XDS_DEBUG)
					{
						if (mProgramTypeCount > 0)
						{
							for (int i = 0; i < mProgramTypeCount; i++)
							{
								if (mProgramTypeData[i] > 0)
									Log()	<< "## " << GetLogLabel () << ":      Program Type[" << i << "] = 0x" << UHEX2(mProgramTypeData[i]) << endl;
							}
						}
						else
							Log()	<< "## " << GetLogLabel () << ":      Program Type data is incomplete" << endl;
					}
					break;
		
			case NTV2_CC608_XDSContentAdvisoryType:		
					if (XDS_DEBUG)
					{
						if (mContentAdvisoryCount == 2)
						{
							const char *sSystem = "";
							const char *sRating = "";

							//	The Content Advisory data can be described in one of several "systems" (MPA, US TV Parental Guidance, Canadian, etc.)
							int system = ((mContentAdvisoryData[0] & 0x38) >> 3) + (mContentAdvisoryData[1] & 0x08);	//	Word 1, bit 3; word 0, bits 5-3
							switch (system)
							{
								case 0x00:
								case 0x02:
								case 0x04:
								case 0x06:
								case 0x08:
								case 0x0a:
								case 0x0c:
								case 0x0e:		// System = MPA ("System 0" or "System 2")
									{
										sSystem = "MPA";
										int rating = (mContentAdvisoryData[0] & 0x07);	//	bits r2, r1, r0
										switch (rating)
										{
											case 0:	sRating = "N/A";		break;
											case 1:	sRating = "G";			break;
											case 2:	sRating = "PG";			break;
											case 3:	sRating = "PG-13";		break;
											case 4:	sRating = "R";			break;
											case 5:	sRating = "NC-17";		break;
											case 6:	sRating = "X";			break;
											case 7:	sRating = "Not Rated";	break;
										}
										Log()	<< "## " << GetLogLabel () << ":      Content Advisory = " << sSystem << ": " << sRating << endl;
									}
									break;

								case 0x01:
								case 0x05:
								case 0x09:
								case 0x0d:		// System = "U.S. TV Parental Guidelines"
									{
										sSystem = "US TV Parental Guidelines";
										#if defined (AJA_DEBUG)
											bool bViolence			((mContentAdvisoryData[1] & 0x20) != 0);
											bool bFantasy			(false);
											bool bSexualSituations	((mContentAdvisoryData[1] & 0x10) != 0);
											bool bLanguage			((mContentAdvisoryData[1] & 0x08) != 0);
											bool bDialog			((mContentAdvisoryData[0] & 0x20) != 0);
										#endif	//	AJA_DEBUG

										int rating = (mContentAdvisoryData[1] & 0x07);	//	bits g2, g1, g0
										switch (rating)
										{
											case 0:	sRating = "None";		break;
											case 1:	sRating = "TV-Y";		break;
											case 2:	sRating = "TV-Y7";
													#if defined (AJA_DEBUG)
													bFantasy = bViolence;		//	In "TV-Y7", Violence is not "Violence" - it's "Fantasy Violence"...
													bViolence = false;
													#endif	//	AJA_DEBUG
													break;
											case 3:	sRating = "TV-G";		break;
											case 4:	sRating = "TV-PG";		break;
											case 5:	sRating = "TV-14";		break;
											case 6:	sRating = "TV-MA";		break;
											case 7:	sRating = "None";		break;
										}
										Log()	<< "## " << GetLogLabel () << ":      Content Advisory = " << sSystem << ": " << sRating << " ("
													#if defined (AJA_DEBUG)
														<< (bFantasy ? "FantasyViolence " : "") << (bViolence ? "Violence " : "")
														<< (bSexualSituations ? "SexualSituations " : "") << (bLanguage ? "Language " : "") << (bDialog ? "SexuallySuggestiveDialog " : "")
													#endif	//	defined (AJA_DEBUG)
														<< ")" << endl;
									}
									break;

								case 0x03:
									{
										sSystem = "Canadian English Language Rating";

										int rating = (mContentAdvisoryData[1] & 0x07);	//	bits g2, g1, g0
										switch (rating)
										{
											case 0:	sRating = "E - Exempt";							break;
											case 1:	sRating = "C - Children";						break;
											case 2:	sRating = "C8+ - Children 8 years and older";	break;
											case 3:	sRating = "G - General";						break;
											case 4:	sRating = "PG - Parental Guidance";				break;
											case 5:	sRating = "14+ - Viewers 14 years and older";	break;
											case 6:	sRating = "18+ - Adult Programming";			break;
											case 7:	sRating = "Undefined";							break;
										}
										Log()	<< "## " << GetLogLabel () << ":      Content Advisory = " << sSystem << ": " << sRating << endl;
									}
									break;

								case 0x07:
									{
										sSystem = "Canadian French Language Rating";

										int rating = (mContentAdvisoryData[1] & 0x07);	//	bits g2, g1, g0
										switch (rating)
										{
											case 0:	sRating = "E";			break;
											case 1:	sRating = "G";			break;
											case 2:	sRating = "8 ans+";		break;
											case 3:	sRating = "13 ans+";	break;
											case 4:	sRating = "16 ans+";	break;
											case 5:	sRating = "18 ans+";	break;
											case 6:
											case 7:	sRating = "Undefined";	break;
										}
										Log()	<< "## " << GetLogLabel () << ":      Content Advisory = " << sSystem << ": " << sRating << endl;
									}
									break;

								case 0x0b:
								case 0x0f:
									{
										sSystem = "Reserved";
										sRating = "Not defined";

										Log()	<< "## " << GetLogLabel () << ":      Content Advisory = " << sSystem << ": " << sRating << endl;
									}
									break;

							}
						}
						else
							Log()	<< "## " << GetLogLabel () << ":      Content Advisory data is incomplete" << endl;
					}
					break;
		
			case NTV2_CC608_XDSAudioServicesType:		
					if (XDS_DEBUG)
					{
						if (mAudioServicesCount == 2)
							Log()	<< "## " << GetLogLabel () << ":      Audio Services = 0x" << UHEX2(mAudioServicesData[0] & 0x3f) << " 0x" << UHEX2(mAudioServicesData[1] & 0x3f) << endl;
						else
							Log()	<< "## " << GetLogLabel () << ":      Audio Services data is incomplete" << endl;
					}
					break;
		
			case NTV2_CC608_XDSCaptionServicesType:		
					if (XDS_DEBUG)
					{
						if (mCaptioningServicesCount > 0)
						{
							for (int i = 0; i < mCaptioningServicesCount; i++)
								Log()	<< "## " << GetLogLabel () << ":      Captioning Service[" << i << "] = 0x" << UHEX2(mCaptioningServicesData[i] & 0x3f) << endl;
						}
						else
							Log()	<< "## " << GetLogLabel () << ":      Captioning Services data is incomplete" << endl;
					}
					break;
		
			case NTV2_CC608_XDSCopyRedistributionType:		
					if (XDS_DEBUG)
					{
						if (mCopyRedistributionCount == 2)
							Log()	<< "## " << GetLogLabel () << ":      Copy and Redistribution = 0x" << UHEX2(mCopyRedistributionData[0] & 0x3f) << " 0x" << UHEX2(mCopyRedistributionData[1] & 0x3f) << endl;
						else
							Log()	<< "## " << GetLogLabel () << ":      Copy and Redistribution data is incomplete" << endl;
					}
					break;
		
			case NTV2_CC608_XDSCompositePacket1Type:		
					if (XDS_DEBUG)
					{
						if (mCompositePacket1Count >=10)
							Log()	<< "## " << GetLogLabel () << ":      Composite Packet #1 = " << endl
											<< "      Program Type:     0x" << UHEX2(mCompositePacket1Data[0] & 0x3f) << " " << UHEX2(mCompositePacket1Data[1] & 0x3f)
																			<< " " << UHEX2(mCompositePacket1Data[2] & 0x3f) << " " << UHEX2(mCompositePacket1Data[3] & 0x3f)
																			<< " " << UHEX2(mCompositePacket1Data[4] & 0x3f) << endl
											<< "      Content Advisory: 0x" << UHEX2(mCompositePacket1Data[5] & 0x3f) << endl
											<< "      Length:  " << (mCompositePacket1Data[7] & 0x3f) << ":" << (mCompositePacket1Data[6] & 0x3f) << endl
											<< "      Elapsed: " << (mCompositePacket1Data[9] & 0x3f) << ":" << (mCompositePacket1Data[8] & 0x3f) << endl
											<< "      Title: " << string (reinterpret_cast <char *> (&mCompositePacket1Data[10])) << endl;
						else
							Log()	<< GetLogLabel () << ":      Composite Packet #1 data is incomplete" << endl;
					}
					break;
		
			case NTV2_CC608_XDSCompositePacket2Type:		
					if (XDS_DEBUG)
					{
						if (mCompositePacket2Count >=14)
							Log()	<< "## " << GetLogLabel () << ":      Composite Packet #2 = " << endl
											<< "      Program Start:    " << (mCompositePacket2Data[3] & 0x3f) << "/" << (mCompositePacket2Data[2] & 0x3f) << "  "
																			<< (mCompositePacket2Data[1] & 0x3f) << ":" << (mCompositePacket2Data[0] & 0x3f) << endl
											<< "      Audio Services:   0x" << UHEX2(mCompositePacket2Data[4] & 0x3f) << " " << UHEX2(mCompositePacket2Data[5] & 0x3f) << endl
											<< "      Caption Services: 0x" << UHEX2(mCompositePacket2Data[6] & 0x3f) << " " << UHEX2(mCompositePacket2Data[7] & 0x3f) << endl
											<< "      Call Letters:     %c%c%c%c" << mCompositePacket2Data[8] << mCompositePacket2Data[9] << mCompositePacket2Data[10] << mCompositePacket2Data[11] << endl
											<< "      Native Channel:   0x" << UHEX2(mCompositePacket2Data[12] & 0x3f) << " " << UHEX2(mCompositePacket2Data[13] & 0x3f) << endl
											<< "      Network Name:     %s" << string (reinterpret_cast <char *> (&mCompositePacket2Data[14])) << endl;
						else
							Log()	<< "## " << GetLogLabel () << ":      Composite Packet #2 data is incomplete" << endl;
					}
					break;
		
			case NTV2_CC608_XDSProgramDescRow1Type:		
					if (XDS_DEBUG)
						Log()	<< "## " << GetLogLabel () << ":      Program Desription Row #1 = " << mProgramDescriptionRow1Str << endl;
					break;
		
			case NTV2_CC608_XDSProgramDescRow2Type:		
					if (XDS_DEBUG)
						Log()	<< "## " << GetLogLabel () << ":      Program Desription Row #2 = " << mProgramDescriptionRow2Str << endl;
					break;
		
			case NTV2_CC608_XDSProgramDescRow3Type:		
					if (XDS_DEBUG)
						Log()	<< "## " << GetLogLabel () << ":      Program Desription Row #3 = " << mProgramDescriptionRow3Str << endl;
					break;
		
			case NTV2_CC608_XDSProgramDescRow4Type:		
					if (XDS_DEBUG)
						Log()	<< "## " << GetLogLabel () << ":      Program Desription Row #4 = " << mProgramDescriptionRow4Str << endl;
					break;
		
			case NTV2_CC608_XDSProgramDescRow5Type:		
					if (XDS_DEBUG)
						Log()	<< "## " << GetLogLabel () << ":      Program Desription Row #5 = " << mProgramDescriptionRow5Str << endl;
					break;
		
			case NTV2_CC608_XDSProgramDescRow6Type:		
					if (XDS_DEBUG)
						Log()	<< "## " << GetLogLabel () << ":      Program Desription Row #6 = " << mProgramDescriptionRow6Str << endl;
					break;
		
			case NTV2_CC608_XDSProgramDescRow7Type:		
					if (XDS_DEBUG)
						Log()	<< "## " << GetLogLabel () << ":      Program Desription Row #7 = " << mProgramDescriptionRow7Str << endl;
					break;
		
			case NTV2_CC608_XDSProgramDescRow8Type:		
					if (XDS_DEBUG)
						Log()	<< "## " << GetLogLabel () << ":      Program Desription Row #8 = " << mProgramDescriptionRow8Str << endl;
					break;
		
			case NTV2_CC608_XDSUnknownType:
			default:
					if (XDS_DEBUG)
						Log()	<< "## " << GetLogLabel () << ":      Unknown Type!" << endl;
					break;
		}
		mCurrType = NTV2_CC608_XDSUnknownType;
	}
	else	//	Got packet data
	{
		//	For string-based data, build a string out of our two new characters
		UByte dataStr[3];
		dataStr[0] = inByte1;
		dataStr[1] = inByte2;
		dataStr[2] = 0;

		switch (mCurrType)
		{
			case NTV2_CC608_XDSProgramIDNumberType:
					if (mProgramIDNumberCount < 4)
						mProgramIDNumberData[mProgramIDNumberCount++] = inByte1;
					if (mProgramIDNumberCount < 4)
						mProgramIDNumberData[mProgramIDNumberCount++] = inByte2;
					break;
		
			case NTV2_CC608_XDSLengthTimeInShowType:
					if (mLengthTimeInShowCount < 6)
						mLengthTimeInShowData[mLengthTimeInShowCount++] = inByte1;
					if (mLengthTimeInShowCount < 6)
						mLengthTimeInShowData[mLengthTimeInShowCount++] = inByte2;
					break;
		
			case NTV2_CC608_XDSProgramNameType:	
					if (strlen(mProgramNameStr) <= 30)
						strcat(mProgramNameStr, reinterpret_cast <char *> (dataStr));
					break;
		
			case NTV2_CC608_XDSProgramTypeType:		
					if (mProgramTypeCount < 32)
						mProgramTypeData[mProgramTypeCount++] = inByte1;
					if (mProgramTypeCount < 32)
						mProgramTypeData[mProgramTypeCount++] = inByte2;
					break;
		
			case NTV2_CC608_XDSContentAdvisoryType:		
					if (mContentAdvisoryCount < 2)
						mContentAdvisoryData[mContentAdvisoryCount++] = inByte1;
					if (mContentAdvisoryCount < 2)
						mContentAdvisoryData[mContentAdvisoryCount++] = inByte2;
					break;
		
			case NTV2_CC608_XDSAudioServicesType:		
					if (mAudioServicesCount < 2)
						mAudioServicesData[mAudioServicesCount++] = inByte1;
					if (mAudioServicesCount < 2)
						mAudioServicesData[mAudioServicesCount++] = inByte2;
					break;
		
			case NTV2_CC608_XDSCaptionServicesType:		
					if (mCaptioningServicesCount < 8)
						mCaptioningServicesData[mCaptioningServicesCount++] = inByte1;
					if (mCaptioningServicesCount < 8)
						mCaptioningServicesData[mCaptioningServicesCount++] = inByte2;
					break;
		
			case NTV2_CC608_XDSCopyRedistributionType:		
					if (mCopyRedistributionCount < 2)
						mCopyRedistributionData[mCopyRedistributionCount++] = inByte1;
					if (mCopyRedistributionCount < 2)
						mCopyRedistributionData[mCopyRedistributionCount++] = inByte2;
					break;
		
			case NTV2_CC608_XDSCompositePacket1Type:		
					if (mCompositePacket1Count < 32)
						mCompositePacket1Data[mCompositePacket1Count++] = inByte1;
					if (mCompositePacket1Count < 32)
						mCompositePacket1Data[mCompositePacket1Count++] = inByte2;
					break;
		
			case NTV2_CC608_XDSCompositePacket2Type:		
					if (mCompositePacket2Count < 32)
						mCompositePacket2Data[mCompositePacket2Count++] = inByte1;
					if (mCompositePacket2Count < 32)
						mCompositePacket2Data[mCompositePacket2Count++] = inByte2;
					break;
		
			case NTV2_CC608_XDSProgramDescRow1Type:		
					if (strlen(mProgramDescriptionRow1Str) <= 30)
						strcat(mProgramDescriptionRow1Str, reinterpret_cast <char *> (dataStr));
					break;
		
			case NTV2_CC608_XDSProgramDescRow2Type:		
					if (strlen(mProgramDescriptionRow2Str) <= 30)
						strcat(mProgramDescriptionRow2Str, reinterpret_cast <char *> (dataStr));
					break;
		
			case NTV2_CC608_XDSProgramDescRow3Type:		
					if (strlen(mProgramDescriptionRow3Str) <= 30)
						strcat(mProgramDescriptionRow3Str, reinterpret_cast <char *> (dataStr));
					break;
		
			case NTV2_CC608_XDSProgramDescRow4Type:		
					if (strlen(mProgramDescriptionRow4Str) <= 30)
						strcat(mProgramDescriptionRow4Str, reinterpret_cast <char *> (dataStr));
					break;
		
			case NTV2_CC608_XDSProgramDescRow5Type:		
					if (strlen(mProgramDescriptionRow5Str) <= 30)
						strcat(mProgramDescriptionRow5Str, reinterpret_cast <char *> (dataStr));
					break;
		
			case NTV2_CC608_XDSProgramDescRow6Type:		
					if (strlen(mProgramDescriptionRow6Str) <= 30)
						strcat(mProgramDescriptionRow6Str, reinterpret_cast <char *> (dataStr));
					break;
		
			case NTV2_CC608_XDSProgramDescRow7Type:		
					if (strlen(mProgramDescriptionRow7Str) <= 30)
						strcat(mProgramDescriptionRow7Str, reinterpret_cast <char *> (dataStr));
					break;
		
			case NTV2_CC608_XDSProgramDescRow8Type:		
					if (strlen(mProgramDescriptionRow8Str) <= 30)
						strcat(mProgramDescriptionRow8Str, reinterpret_cast <char *> (dataStr));
					break;
		
			case NTV2_CC608_XDSUnknownType:
			default:
					break;
		}	//	switch on mCurrType
	}	//	else got packet data

	return bResult;

}	//	NewCurrentClassData


// NewFutureClassData()
//		Parse new incoming XDS "Future Programming" Class data 
//
bool CNTV2XDSDecodeChannel608::NewFutureClassData (const UByte inByte1, const UByte inByte2, const NTV2Line21Field inField)
{
	bool bResult = true;

	(void) inField;
	//char *str = "";

	//	Start (or continuation) of mode?
	if (inByte1 == 0x03 || inByte1 == 0x04)
	{
		mCurrClass = NTV2_CC608_XDSFutureClass;
		//bool bStart = (inByte1 == 0x03);

		switch (inByte2)
		{
			case 0:
			default:	mCurrType = NTV2_CC608_XDSUnknownType;
						break;
		}
	}
	else if (inByte1 == 0x0f)	//	End of packet?
	{
		switch (mCurrType)
		{
			case NTV2_CC608_XDSUnknownType:
			default:
					if (XDS_DEBUG)
						Log()	<< "## " << GetLogLabel () << ":      Unknown Type!" << endl;
					break;
		}
		mCurrType = NTV2_CC608_XDSUnknownType;
	}
	else	//	Got packet data
	{
		switch (mCurrType)
		{
			case NTV2_CC608_XDSUnknownType:
			default:
					break;
		}
	}

	return bResult;

}	//	NewFutureClassData


// NewChannelClassData()
//		Parse new incoming XDS "Channel" Class data 
//
bool CNTV2XDSDecodeChannel608::NewChannelClassData (const UByte inByte1, const UByte inByte2, const NTV2Line21Field inField)
{
	bool bResult = true;

	(void) inField;
	//char *str = "";

	//	Start (or continuation) of mode?
	if (inByte1 == 0x05 || inByte1 == 0x06)
	{
		mCurrClass = NTV2_CC608_XDSChannelClass;

		bool bStart = (inByte1 == 0x05);

		switch (inByte2)
		{
			case 0x01:	mCurrType = NTV2_CC608_XDSNetworkNameType;
						if (bStart)
							mNetworkNameStr[0] = '\0';
						break;

			case 0x02:	mCurrType = NTV2_CC608_XDSCallLettersType;
						if (bStart)
						{
							mCallLettersCount = 0;
							for (int i = 0; i < 6; i++)
								mCallLettersData[i] = 0;
						}
						break;

			case 0x03:	mCurrType = NTV2_CC608_XDSTapeDelayType;
						if (bStart)
						{
							mTapeDelayCount = 0;
							for (int i = 0; i < 2; i++)
								mTapeDelayData[i] = 0;
						}
						break;

			case 0x04:	mCurrType = NTV2_CC608_XDSTransmissionSignalIDType;
						if (bStart)
						{
							mTransmissionSignalIDCount = 0;
							for (int i = 0; i < 4; i++)
								mTransmissionSignalIDData[i] = 0;
						}
						break;

			default:	mCurrType = NTV2_CC608_XDSUnknownType;
						break;
		}
	}
	else if (inByte1 == 0x0f)	//	End of packet?
	{
		switch (mCurrType)
		{
			case NTV2_CC608_XDSNetworkNameType:
					if (XDS_DEBUG)
						Log()	<< "## " << GetLogLabel () << ":      Network Name = " << mNetworkNameStr << endl;
					break;

			case NTV2_CC608_XDSCallLettersType:
					if (XDS_DEBUG)
					{
						if (mCallLettersCount == 4)
							Log()	<< "## " << GetLogLabel () << ":      Call Letters = " << mCallLettersData[0] << mCallLettersData[1] << mCallLettersData[2] << mCallLettersData[3] << endl;
						else if (mCallLettersCount == 6)
							Log()	<< "## " << GetLogLabel () << ":      Call Letters = " << mCallLettersData[0] << mCallLettersData[1] << mCallLettersData[2] << mCallLettersData[3]
											<< "-" << (mCallLettersData[4] == 0 ? ' ' : mCallLettersData[4]) << (mCallLettersData[5] == 0 ? ' ' : mCallLettersData[5]) << endl;
						else
							Log()	<< "## " << GetLogLabel () << ":      Call Letters data is incomplete" << endl;
					}
					break;

			case NTV2_CC608_XDSTapeDelayType:
					if (XDS_DEBUG)
					{
						if (mTapeDelayCount == 2)
							Log()	<< "## " << GetLogLabel () << ":      Tape Delay = " << (mTapeDelayData[1] & 0x3f) << ":" << (mTapeDelayData[0] & 0x3f) << endl;
						else
							Log()	<< "## " << GetLogLabel () << ":      Tape Delay data is incomplete" << endl;
					}
					break;

			case NTV2_CC608_XDSTransmissionSignalIDType:
					if (XDS_DEBUG)
					{
						if (mTransmissionSignalIDCount == 4)
							Log()	<< "## " << GetLogLabel () << ":      Transmission Signal ID = 0x" << UHEX2(mTransmissionSignalIDData[3] & 0x0f) << " " << UHEX2(mTransmissionSignalIDData[2] & 0x0f)
											<< " " << UHEX2(mTransmissionSignalIDData[1] & 0x0f) << " " << UHEX2(mTransmissionSignalIDData[0] & 0x0f) << endl;
						else
							Log()	<< "## " << GetLogLabel () << ":      Transmission Signal ID data is incomplete" << endl;
					}
					break;

			case NTV2_CC608_XDSUnknownType:
			default:
					if (XDS_DEBUG)
						Log() << "## " << GetLogLabel () << ":      Unknown Type!" << endl;
					break;
		}
		
		mCurrType = NTV2_CC608_XDSUnknownType;
	}
	else	//	Got packet data
	{
		//	For string-based data, build a string out of our two new characters
		UByte dataStr[3];
		dataStr[0] = inByte1;
		dataStr[1] = inByte2;
		dataStr[2] = 0;

		switch (mCurrType)
		{
			case NTV2_CC608_XDSNetworkNameType:
					if (strlen(mNetworkNameStr) <= 30)
						strcat(mNetworkNameStr, (char *) dataStr);
					break;
		
			case NTV2_CC608_XDSCallLettersType:
					if (mCallLettersCount < 6)
						mCallLettersData[mCallLettersCount++] = inByte1;
					if (mCallLettersCount < 6)
						mCallLettersData[mCallLettersCount++] = inByte2;
					break;
		
			case NTV2_CC608_XDSTapeDelayType:
					if (mTapeDelayCount < 6)
						mTapeDelayData[mTapeDelayCount++] = inByte1;
					if (mTapeDelayCount < 6)
						mTapeDelayData[mTapeDelayCount++] = inByte2;
					break;
		
			case NTV2_CC608_XDSTransmissionSignalIDType:
					if (mTransmissionSignalIDCount < 4)
						mTransmissionSignalIDData[mTransmissionSignalIDCount++] = inByte1;
					if (mTransmissionSignalIDCount < 4)
						mTransmissionSignalIDData[mTransmissionSignalIDCount++] = inByte2;
					break;
		
			case NTV2_CC608_XDSUnknownType:
			default:
					break;
		}
	}

	return bResult;

}	//	NewChannelClassData


// NewMiscServiceClassData()
//		Parse new incoming XDS "Miscellaneous" Class data 
//
bool CNTV2XDSDecodeChannel608::NewMiscClassData (const UByte inByte1, const UByte inByte2, const NTV2Line21Field inField)
{
	bool bResult = true;

	(void) inField;
	//char *str = "";

	//	Start (or continuation) of mode?
	if (inByte1 == 0x07 || inByte1 == 0x08)
	{
		mCurrClass = NTV2_CC608_XDSMiscClass;
		bool bStart = (inByte1 == 0x07);

		switch (inByte2)
		{
			case 0x01:	mCurrType = NTV2_CC608_XDSTimeOfDayType;
						if (bStart)
						{
							mTimeOfDayCount = 0;
							for (int i = 0; i < 6; i++)
								mTimeOfDayData[i] = 0;
						}
						break;

			case 0x02:	mCurrType = NTV2_CC608_XDSImpulseCaptureIDType;
						if (bStart)
						{
							mImpulseCaptureIDCount = 0;
							for (int i = 0; i < 6; i++)
								mImpulseCaptureIDData[i] = 0;
						}
						break;

			case 0x03:	mCurrType = NTV2_CC608_XDSSupplementalDataLocationType;
						if (bStart)
						{
							mSupplementalDataLocationCount = 0;
							for (int i = 0; i < 32; i++)
								mSupplementalDataLocationData[i] = 0;
						}
						break;

			case 0x04:	mCurrType = NTV2_CC608_XDSLocalTimeZoneType;
						if (bStart)
						{
							mLocalTimeZoneCount = 0;
							for (int i = 0; i < 2; i++)
								mLocalTimeZoneData[i] = 0;
						}
						break;

			case 0x40:	mCurrType = NTV2_CC608_XDSOutOfBandChannelType;
						if (bStart)
						{
							mOutOfBandChannelNumberCount = 0;
							for (int i = 0; i < 2; i++)
								mOutOfBandChannelNumberData[i] = 0;
						}
						break;

			case 0x41:	mCurrType = NTV2_CC608_XDSChannelMapPointerType;
						if (bStart)
						{
							mChannelMapPointerCount = 0;
							for (int i = 0; i < 2; i++)
								mChannelMapPointerData[i] = 0;
						}
						break;

			case 0x42:	mCurrType = NTV2_CC608_XDSChannelMapHeaderType;
						if (bStart)
						{
							mChannelMapHeaderCount = 0;
							for (int i = 0; i < 4; i++)
								mChannelMapHeaderData[i] = 0;
						}
						break;

			case 0x43:	mCurrType = NTV2_CC608_XDSChannelMapType;
						if (bStart)
						{
							mChannelMapCount = 0;
							for (int i = 0; i < 10; i++)
								mChannelMapData[i] = 0;
						}
						break;

						
			default:	mCurrType = NTV2_CC608_XDSUnknownType;
						break;
		}
	}
	else if (inByte1 == 0x0f)	//	End of packet?
	{
		switch (mCurrType)
		{
			case NTV2_CC608_XDSTimeOfDayType:
					if (XDS_DEBUG)
					{
						if (mTimeOfDayCount == 6)
							Log()	<< "## " << GetLogLabel () << ":      Time of Day = " << (1990 + (mTimeOfDayData[5] & 0x3f)) << "/" << (mTimeOfDayData[4] & 0x07) << "/" << (mTimeOfDayData[3] & 0x0f)
											<< " (" << (mTimeOfDayData[2] & 0x1f) << ") " << DEC02(mTimeOfDayData[1] & 0x1f) << ":" << DEC02(mTimeOfDayData[0] & 0x3f) << endl;
						else
							Log()	<< "## " << GetLogLabel () << ":      Time of Day data is incomplete" << endl;
					}
					break;

			case NTV2_CC608_XDSImpulseCaptureIDType:
					if (XDS_DEBUG)
					{
						if (mImpulseCaptureIDCount == 6)
							Log()	<< "## " << GetLogLabel () << ":      Impulse Capture ID = " << (mImpulseCaptureIDData[3] & 0x0f) << "/" << (mImpulseCaptureIDData[2] & 0x1f) << " "
											<< DEC02(mImpulseCaptureIDData[1] & 0x1f) << ":" << DEC02(mImpulseCaptureIDData[0] & 0x3f) << "  Length = "
											<< DEC02(mImpulseCaptureIDData[5] & 0x3f) << ":" << DEC02(mImpulseCaptureIDData[4] & 0x3f) << endl;
						else
							Log()	<< "## " << GetLogLabel () << ":      Impulse Capture ID data is incomplete" << endl;
					}
					break;

			case NTV2_CC608_XDSSupplementalDataLocationType:
					if (XDS_DEBUG)
					{
						if (mSupplementalDataLocationCount > 0)
							for (int i (0);  i < mSupplementalDataLocationCount;  i++)
								Log()	<< "## " << GetLogLabel () << ":      Supplemental Data Location " << i << " = Field " << (((mSupplementalDataLocationData[i] & 0x20) == 0) ? 1 : 2)
												<< ", Line " << (mSupplementalDataLocationData[i] & 0x1f) << endl;
						else
							Log()	<< "## " << GetLogLabel () << ":      Supplemental Data Location data is incomplete" << endl;
					}
					break;

			case NTV2_CC608_XDSLocalTimeZoneType:
					if (XDS_DEBUG)
					{
						if (mLocalTimeZoneCount == 2)
							Log()	<< "## " << GetLogLabel () << ":      Local Time Zone = " << DEC02(mLocalTimeZoneData[0] & 0x1f) << ":00 " << ((mLocalTimeZoneData[0] & 0x20) ? "(DST)" : "") << endl;
						else
							Log()	<< "## " << GetLogLabel () << ":      Local Time Zone data is incomplete" << endl;
					}
					break;

			case NTV2_CC608_XDSOutOfBandChannelType:
					if (XDS_DEBUG)
					{
						if (mOutOfBandChannelNumberCount == 2)
							Log()	<< "## " << GetLogLabel () << ":      Out of Band Channel = " << (((mOutOfBandChannelNumberData[1] & 0x3f) << 6) + (mOutOfBandChannelNumberData[0] & 0x3f)) << endl;
						else
							Log()	<< "## " << GetLogLabel () << ":      Out of Band Channel data is incomplete" << endl;
					}
					break;

			case NTV2_CC608_XDSChannelMapPointerType:
					if (XDS_DEBUG)
					{
						if (mChannelMapPointerCount == 2)
							Log()	<< "## " << GetLogLabel () << ":      Channel Map Pointer = " << (((mChannelMapPointerData[1] & 0x0f) << 6) + (mChannelMapPointerData[0] & 0x3f)) << endl;
						else
							Log()	<< "## " << GetLogLabel () << ":      Channel Map Pointer data is incomplete" << endl;
					}
					break;

			case NTV2_CC608_XDSChannelMapHeaderType:
					if (XDS_DEBUG)
					{
						if (mChannelMapHeaderCount == 4)
							Log()	<< "## " << GetLogLabel () << ":      Channel = " << (((mChannelMapHeaderData[1] & 0x0f) << 6) + (mChannelMapHeaderData[0] & 0x3f)) << ", Version = "
											<< (mChannelMapHeaderData[2] & 0x3f) << endl;
						else
							Log()	<< "## " << GetLogLabel () << ":      Channel Map Header data is incomplete" << endl;
					}
					break;

			case NTV2_CC608_XDSChannelMapType:
					if (XDS_DEBUG)
					{
						if (mChannelMapCount >= 2)
						{
							bool bRemap = (mChannelMapHeaderData[1] & 0x20) != 0;
							int offset = 2;

							Log()	<< "## " << GetLogLabel () << ":      User Channel = " << (((mChannelMapData[1] & 0x0f) << 6) + (mChannelMapData[0] & 0x3f)) << "  " << (bRemap ? "(Remapped)" : "") << endl;
						
							if (bRemap && mChannelMapCount >= 4)
							{
								offset = 4;
								Log()	<< "## " << GetLogLabel () << ":      Tune Channel = " << (((mChannelMapData[3] & 0x0f) << 6) + (mChannelMapData[2] & 0x3f)) << endl;
							}

							if (mChannelMapCount > offset)
							{
								UByte sChanID[7];
								int IdCount = mChannelMapCount - offset;

								for (int i = 0; i < IdCount; i++)
									sChanID[i] = mChannelMapData[offset+i];
								sChanID[IdCount] = '\0';

								Log()	<< "## " << GetLogLabel () << ":      Channel ID = " << sChanID << endl;
							}
						}
						else
							Log()	<< "## " << GetLogLabel () << ":      Channel Map data is incomplete" << endl;
					}
					break;

					
			case NTV2_CC608_XDSUnknownType:
			default:
					if (XDS_DEBUG)
						Log()	<< "## " << GetLogLabel () << ":      Unknown Type!" << endl;
					break;
		}
		mCurrType = NTV2_CC608_XDSUnknownType;
	}
	else	//	Got packet data
	{
		switch (mCurrType)
		{
			case NTV2_CC608_XDSTimeOfDayType:
					if (mTimeOfDayCount < (int) sizeof(mTimeOfDayData) )
						mTimeOfDayData[mTimeOfDayCount++] = inByte1;
					if (mTimeOfDayCount < (int) sizeof(mTimeOfDayData) )
						mTimeOfDayData[mTimeOfDayCount++] = inByte2;
					break;

			case NTV2_CC608_XDSImpulseCaptureIDType:
					if (mImpulseCaptureIDCount < (int) sizeof(mImpulseCaptureIDData) )
						mImpulseCaptureIDData[mImpulseCaptureIDCount++] = inByte1;
					if (mImpulseCaptureIDCount < (int) sizeof(mImpulseCaptureIDData))
						mImpulseCaptureIDData[mImpulseCaptureIDCount++] = inByte2;
					break;

			case NTV2_CC608_XDSSupplementalDataLocationType:
					if (mSupplementalDataLocationCount < (int) sizeof(mSupplementalDataLocationData) )
						mSupplementalDataLocationData[mSupplementalDataLocationCount++] = inByte1;
					if (mSupplementalDataLocationCount < (int) sizeof(mSupplementalDataLocationData))
						mSupplementalDataLocationData[mSupplementalDataLocationCount++] = inByte2;
					break;

			case NTV2_CC608_XDSLocalTimeZoneType:
					if (mLocalTimeZoneCount < (int) sizeof(mLocalTimeZoneData) )
						mLocalTimeZoneData[mLocalTimeZoneCount++] = inByte1;
					if (mLocalTimeZoneCount < (int) sizeof(mLocalTimeZoneData))
						mLocalTimeZoneData[mLocalTimeZoneCount++] = inByte2;
					break;

			case NTV2_CC608_XDSOutOfBandChannelType:
					if (mOutOfBandChannelNumberCount < (int) sizeof(mOutOfBandChannelNumberData) )
						mOutOfBandChannelNumberData[mOutOfBandChannelNumberCount++] = inByte1;
					if (mOutOfBandChannelNumberCount < (int) sizeof(mOutOfBandChannelNumberData))
						mOutOfBandChannelNumberData[mOutOfBandChannelNumberCount++] = inByte2;
					break;

			case NTV2_CC608_XDSChannelMapPointerType:
					if (mChannelMapPointerCount < (int) sizeof(mChannelMapPointerData) )
						mChannelMapPointerData[mChannelMapPointerCount++] = inByte1;
					if (mChannelMapPointerCount < (int) sizeof(mChannelMapPointerData))
						mChannelMapPointerData[mChannelMapPointerCount++] = inByte2;
					break;

			case NTV2_CC608_XDSChannelMapHeaderType:
					if (mChannelMapHeaderCount < (int) sizeof(mChannelMapHeaderData) )
						mChannelMapHeaderData[mChannelMapHeaderCount++] = inByte1;
					if (mChannelMapHeaderCount < (int) sizeof(mChannelMapHeaderData))
						mChannelMapHeaderData[mChannelMapHeaderCount++] = inByte2;
					break;

			case NTV2_CC608_XDSChannelMapType:
					if (mChannelMapCount < (int) sizeof(mChannelMapData) )
						mChannelMapData[mChannelMapCount++] = inByte1;
					if (mChannelMapCount < (int) sizeof(mChannelMapData))
						mChannelMapData[mChannelMapCount++] = inByte2;
					break;

			case NTV2_CC608_XDSUnknownType:
			default:
					break;
		}
	}

	return bResult;

}	//	NewMiscClassData


// NewPublicServiceClassData()
//		Parse new incoming XDS "Public Service" Class data 
//
bool CNTV2XDSDecodeChannel608::NewPublicServiceClassData (const UByte inByte1, const UByte inByte2, const NTV2Line21Field inField)
{
	bool bResult = true;

	(void) inField;
	//char *str = "";

	//	Start (or continuation) of mode?
	if (inByte1 == 0x09 || inByte1 == 0x0a)
	{
		mCurrClass = NTV2_CC608_XDSPublicServiceClass;
		bool bStart = (inByte1 == 0x09);

		switch (inByte2)
		{
			case 0x01:	mCurrType = NTV2_CC608_XDSNWSCode;
						if (bStart)
							mNWSCodeStr[0] = '\0';
						break;
						
			case 0x02:	mCurrType = NTV2_CC608_XDSNWSMessage;
						if (bStart)
							mNWSMessageStr[0] = '\0';
						break;

			default:	mCurrType = NTV2_CC608_XDSUnknownType;
						break;
		}
	}
	else if (inByte1 == 0x0f)	//	End of packet?
	{
		switch (mCurrType)
		{
			case NTV2_CC608_XDSNWSCode:
					if (XDS_DEBUG)
						Log()	<< "## " << GetLogLabel () << ":      National Weather Service Code = " << mNWSCodeStr << endl;
					break;
		
			case NTV2_CC608_XDSNWSMessage:
					if (XDS_DEBUG)
						Log()	<< "## " << GetLogLabel () << ":      National Weather Service Message = " << mNWSMessageStr << endl;
					break;
		
			case NTV2_CC608_XDSUnknownType:
			default:
					if (XDS_DEBUG)
						Log()	<< "## " << GetLogLabel () << ":      Unknown Type!" << endl;
					break;
		}
		
		mCurrType = NTV2_CC608_XDSUnknownType;
	}
	else	//	Got packet data
	{
		//	For string-based data, build a string out of our two new characters...
		UByte dataStr[3];
		dataStr[0] = inByte1;
		dataStr[1] = inByte2;
		dataStr[2] = '\0';

		switch (mCurrType)
		{
			case NTV2_CC608_XDSNWSCode:
					if (strlen(mNWSCodeStr) <= 30)
						strcat(mNWSCodeStr, (char *) dataStr);
					break;
		
			case NTV2_CC608_XDSNWSMessage:
					if (strlen(mNWSMessageStr) <= 30)
						strcat(mNWSMessageStr, (char *) dataStr);
					break;
		

			case NTV2_CC608_XDSUnknownType:
			default:
					break;
		}
	}

	return bResult;

}	//	NewPublicServiceClassData



// NewReservedClassData()
//		Parse new incoming XDS "Reserved" Class data 
//
bool CNTV2XDSDecodeChannel608::NewReservedClassData (const UByte inByte1, const UByte inByte2, const NTV2Line21Field inField)
{
	bool bResult = true;

	(void) inField;
	//char *str = "";

	//	Start (or continuation) of mode?
	if (inByte1 == 0x0b || inByte1 == 0x0c)
	{
		mCurrClass = NTV2_CC608_XDSReservedClass;
		//bool bStart = (inByte1 == 0x0b);
		switch (inByte2)
		{
			case 0:
			default:	mCurrType = NTV2_CC608_XDSUnknownType;
						break;
		}
	}
	else if (inByte1 == 0x0f)	//	End of packet?
	{
		switch (mCurrType)
		{
			case NTV2_CC608_XDSUnknownType:
			default:
					if (XDS_DEBUG)
						Log()	<< "## " << GetLogLabel () << ":      Unknown Type!" << endl;
					break;
		}
		mCurrType = NTV2_CC608_XDSUnknownType;
	}
	else	//	Got packet data
	{
		switch (mCurrType)
		{
			case NTV2_CC608_XDSUnknownType:
			default:
					break;
		}
	}
	return bResult;

}	//	NewReservedClassData



// NewPrivateDataClassData()
//		Parse new incoming XDS "Private Data" Class data 
//
bool CNTV2XDSDecodeChannel608::NewPrivateDataClassData (const UByte inByte1, const UByte inByte2, const NTV2Line21Field inField)
{
	bool bResult = true;

	(void) inField;
	//char *str = "";

	//	Start (or continuation) of mode?
	if (inByte1 == 0x0d || inByte1 == 0x0e)
	{
		mCurrClass = NTV2_CC608_XDSPrivateDataClass;
		//bool bStart = (inByte1 == 0x0d);
		switch (inByte2)
		{
			case 0:
			default:	mCurrType = NTV2_CC608_XDSUnknownType;
						break;
		}
	}
	else if (inByte1 == 0x0f)	//	End of packet?
	{
		switch (mCurrType)
		{
			case NTV2_CC608_XDSUnknownType:
			default:
					if (XDS_DEBUG)
						Log()	<< "## " << GetLogLabel () << ":      Unknown Type!" << endl;
					break;
		}
		
		mCurrType = NTV2_CC608_XDSUnknownType;
	}
	else	//	Got packet data
	{
		switch (mCurrType)
		{
			case NTV2_CC608_XDSUnknownType:
			default:
					break;
		}
	}
	return bResult;

}	//	NewPrivateDataClassData


bool CNTV2XDSDecodeChannel608::SubscribeChangeNotification (NTV2Caption608Changed * pCallback, void * pUserData)
{
	mpCallback = pCallback;
	mpSubscriberData = pUserData;
	return true;
}


bool CNTV2XDSDecodeChannel608::UnsubscribeChangeNotification (NTV2Caption608Changed * pCallback, void * pUserData)
{
	if (mpCallback == pCallback && mpSubscriberData == pUserData)
	{
		mpCallback = NULL;
		mpSubscriberData = NULL;
		return true;
	}
	return false;
}


// GetClassString()
//		Returns a string (char *) with the name of the given XDS class
//
string CNTV2XDSDecodeChannel608::GetClassString (const NTV2_CC608_XDSClass theClass)
{
	switch (theClass)
	{
		case NTV2_CC608_XDSCurrentClass:		return "Current";
		case NTV2_CC608_XDSFutureClass:			return "Future";
		case NTV2_CC608_XDSChannelClass:		return "Channel";
		case NTV2_CC608_XDSMiscClass:			return "Misc";
		case NTV2_CC608_XDSPublicServiceClass:	return "Public Service";
		case NTV2_CC608_XDSReservedClass:		return "Reserved";
		case NTV2_CC608_XDSPrivateDataClass:	return "Private Data";
		default:								return "Unknown";
	}

}	//	GetClassString



// GetTypeString()
//		Returns a string (char *) with the name of the given XDS type
//
string CNTV2XDSDecodeChannel608::GetTypeString (const NTV2_CC608_XDSType theType)
{
	switch (theType)
	{
		// "Current Class" Types
		case NTV2_CC608_XDSProgramIDNumberType:				return "Program ID Number";
		case NTV2_CC608_XDSLengthTimeInShowType:			return "Length/Time-in-Show";
		case NTV2_CC608_XDSProgramNameType:					return "Program Name";
		case NTV2_CC608_XDSProgramTypeType:					return "Program Type";
		case NTV2_CC608_XDSContentAdvisoryType:				return "Content Advisory";
		case NTV2_CC608_XDSAudioServicesType:				return "Audio Services";
		case NTV2_CC608_XDSCaptionServicesType:				return "Caption Services";
		case NTV2_CC608_XDSCopyRedistributionType:			return "Copy and Redistribution Control";
		case NTV2_CC608_XDSCompositePacket1Type:			return "Composite Packet #1";
		case NTV2_CC608_XDSCompositePacket2Type:			return "Composite Packet #2";
		case NTV2_CC608_XDSProgramDescRow1Type:				return "Program Description Row 1";
		case NTV2_CC608_XDSProgramDescRow2Type:				return "Program Description Row 2";
		case NTV2_CC608_XDSProgramDescRow3Type:				return "Program Description Row 3";
		case NTV2_CC608_XDSProgramDescRow4Type:				return "Program Description Row 4";
		case NTV2_CC608_XDSProgramDescRow5Type:				return "Program Description Row 5";
		case NTV2_CC608_XDSProgramDescRow6Type:				return "Program Description Row 6";
		case NTV2_CC608_XDSProgramDescRow7Type:				return "Program Description Row 7";
		case NTV2_CC608_XDSProgramDescRow8Type:				return "Program Description Row 8";

		// "Channel Class" Types
		case NTV2_CC608_XDSNetworkNameType:					return "Network Name";
		case NTV2_CC608_XDSCallLettersType:					return "Call Letters";
		case NTV2_CC608_XDSTapeDelayType:					return "Tape Delay";
		case NTV2_CC608_XDSTransmissionSignalIDType:		return "Transmission Signal ID";

		// "Misc Class" Types
		case NTV2_CC608_XDSTimeOfDayType:					return "Time of Day";
		case NTV2_CC608_XDSImpulseCaptureIDType:			return "Impulse Capture ID";
		case NTV2_CC608_XDSSupplementalDataLocationType:	return "Supplemental Data Location";
		case NTV2_CC608_XDSLocalTimeZoneType:				return "Local Time Zone";
		case NTV2_CC608_XDSOutOfBandChannelType:			return "Out of Band Channel";
		case NTV2_CC608_XDSChannelMapPointerType:			return "Channel Map Pointer";
		case NTV2_CC608_XDSChannelMapHeaderType:			return "Channel Map Header";
		case NTV2_CC608_XDSChannelMapType:					return "Channel Map";

		// "Public Service Class" Types
		case NTV2_CC608_XDSNWSCode:							return "National Weather Service Code";
		case NTV2_CC608_XDSNWSMessage:						return "National Weather Service Message";

		default:											return "Unknown Type";
	}

}	//	GetTypeString
