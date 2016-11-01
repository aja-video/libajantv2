/**
	@file		ntv2status.cpp
	@deprecated	This module is obsolete.
	@copyright	(C) 2004-2014 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#include "ntv2status.h"
#include "ntv2utils.h"

using namespace std;
#include <sstream>

#if !defined(AJALinux) && !defined(AJAMac)
#pragma warning( disable : 4800 )  // Disable performance warning messages
#endif

#if !defined (NTV2_DEPRECATE)

	CNTV2Status::CNTV2Status (UWord boardNumber, bool displayErrorMessage, UWord ulBoardType, const char * hostname)
		:   CNTV2Card (boardNumber, displayErrorMessage, ulBoardType, hostname)
	{
	}


	CNTV2Status::~CNTV2Status ()
	{
	}


	void CNTV2Status::GetBoardString (string & outString)
	{
		::GetNTV2BoardString (GetBoardID (), outString);
	}


	void  CNTV2Status::GetFrameBufferVideoFormatString(std::string & outString)
	{
		NTV2VideoFormat	format	(NTV2_FORMAT_UNKNOWN);
		if (GetVideoFormat (&format))
			outString = ::NTV2VideoFormatToString (format);
		else
			outString = "Unsupported format";
	}


	void  CNTV2Status::GetInput1VideoFormatString (string & outString)
	{
		outString =  NTV2VideoFormatStrings [GetInputVideoFormat (NTV2_INPUTSOURCE_SDI1)];
	}

	void CNTV2Status::GetInput2VideoFormatString (string & outString)
	{
		outString =  NTV2VideoFormatStrings [GetInputVideoFormat (NTV2_INPUTSOURCE_SDI2)];
	}

	void CNTV2Status::GetInput3VideoFormatString (string & outString)
	{
		outString =  NTV2VideoFormatStrings [GetInputVideoFormat (NTV2_INPUTSOURCE_SDI3)];
	}

	void CNTV2Status::GetInput4VideoFormatString (string & outString)
	{
		outString =  NTV2VideoFormatStrings [GetInputVideoFormat (NTV2_INPUTSOURCE_SDI4)];
	}

	void CNTV2Status::GetInput5VideoFormatString (string & outString)
	{
		outString =  NTV2VideoFormatStrings [GetInputVideoFormat (NTV2_INPUTSOURCE_SDI5)];
	}

	void CNTV2Status::GetInput6VideoFormatString (string & outString)
	{
		outString =  NTV2VideoFormatStrings [GetInputVideoFormat (NTV2_INPUTSOURCE_SDI6)];
	}

	void CNTV2Status::GetInput7VideoFormatString (string & outString)
	{
		outString =  NTV2VideoFormatStrings [GetInputVideoFormat (NTV2_INPUTSOURCE_SDI7)];
	}

	void CNTV2Status::GetInput8VideoFormatString (string & outString)
	{
		outString =  NTV2VideoFormatStrings [GetInputVideoFormat (NTV2_INPUTSOURCE_SDI8)];
	}

	void CNTV2Status::GetHDMIInputVideoFormatString (string & outString)
	{
		outString =  NTV2VideoFormatStrings [GetHDMIInputVideoFormat ()];
	}

	void CNTV2Status::GetAnalogInputVideoFormatString (string & outString)
	{
		outString =  NTV2VideoFormatStrings [GetAnalogInputVideoFormat ()];
	}

	void CNTV2Status::GetInputVideoFormatString (int inputNum, string & outString)
	{
		outString =  NTV2VideoFormatStrings [GetInputVideoFormat (static_cast <NTV2InputSource> (inputNum))];
	}

	void CNTV2Status::GetReferenceVideoFormatString (string & outString)
	{
		outString =  NTV2VideoFormatStrings [GetReferenceVideoFormat()];
	}

	void CNTV2Status::GetVideoFormatString (NTV2VideoFormat format, std::string & outString)
	{
		if ((ULWord) format <= NTV2_FORMAT_END_HIGH_DEF_FORMATS2)
			outString = NTV2VideoFormatStrings [format];
		else
			outString.clear ();
	}

	void CNTV2Status::GetVideoStandardString (NTV2Standard standard, std::string & outString)
	{
		if ((ULWord) standard <= NTV2_NUM_STANDARDS)
			outString = NTV2VideoStandardStrings [standard];
		else
			outString.clear ();
	}

	void CNTV2Status::GetFrameRateString (NTV2FrameRate frameRate, std::string & outString)
	{
		if ((ULWord) frameRate <= NTV2_NUM_FRAMERATES)
			outString = NTV2FrameRateStrings [frameRate];
		else
			outString.clear ();
	}
#endif	//	!defined (NTV2_DEPRECATE)
