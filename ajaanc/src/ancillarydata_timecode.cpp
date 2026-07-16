/* SPDX-License-Identifier: MIT */
/**
	@file		ancillarydata_timecode.cpp
	@brief		Implements the AJAAncillaryData_Timecode class.
	@copyright	(C) 2010-2022 AJA Video Systems, Inc.
**/

#include "ancillarydata_timecode.h"
#include <iomanip>


using namespace std;


AJAAncillaryData_Timecode::AJAAncillaryData_Timecode ()
	:	AJAAncillaryData ()
{
	Init();
}


AJAAncillaryData_Timecode::AJAAncillaryData_Timecode (const AJAAncillaryData_Timecode & clone)
	:	AJAAncillaryData ()
{
	Init();
	*this = clone;
}


AJAAncillaryData_Timecode::AJAAncillaryData_Timecode (const AJAAncillaryData_Timecode * pClone)
	:	AJAAncillaryData ()
{
	Init();
	if (pClone)
		*this = *pClone;
}


AJAAncillaryData_Timecode::AJAAncillaryData_Timecode (const AJAAncillaryData * pData)
	:	AJAAncillaryData	(pData)
{
	Init();
}


void AJAAncillaryData_Timecode::Init()
{
	uint8_t i;

	for (i = 0; i < kNumTimeDigits; i++)
		m_timeDigits[i] = 0;

	for (i = 0; i < kNumBinaryGroups; i++)
		m_binaryGroup[i] = 0;
}


AJAAncillaryData_Timecode & AJAAncillaryData_Timecode::operator = (const AJAAncillaryData_Timecode & inRHS)
{
	if (this != &inRHS)		// ignore self-assignment
	{
		AJAAncillaryData::operator = (inRHS);		// copy the base class stuff

		// copy the local stuff
		uint8_t i;
		for (i = 0; i < kNumTimeDigits; i++)
			m_timeDigits[i] = inRHS.m_timeDigits[i];

		for (i = 0; i < kNumBinaryGroups; i++)
			m_binaryGroup[i] = inRHS.m_binaryGroup[i];
	}
	return *this;
}


void AJAAncillaryData_Timecode::Clear (void)
{
	AJAAncillaryData::Clear();
	Init();
}


AJAStatus AJAAncillaryData_Timecode::ParsePayloadData (void)
{
	// Since I have no "concrete" transport of my own, this must be done by my derived classes.
	m_rcvDataValid = false;
	return AJA_STATUS_SUCCESS;
}


AJAAncDataType AJAAncillaryData_Timecode::RecognizeThisAncillaryData (const AJAAncillaryData * pAncData)
{
	(void) pAncData;
	// Since I have no "concrete" transport of my own, this must be done by my derived classes.
	return AJAAncDataType_Unknown;
}


//----------------------

// sets "raw" hex value, with no attempt to second-guess what the value is used for
// Note: "digitNum" is in transmission order, i.e. 0 = ls value, 7 = ms value)
AJAStatus AJAAncillaryData_Timecode::SetTimeHexValue (const uint8_t inDigitNum, const uint8_t inHexValue, const uint8_t inMask)
{
	if (inDigitNum >= kNumTimeDigits)
		return AJA_STATUS_RANGE;

	m_timeDigits[inDigitNum] = (m_timeDigits[inDigitNum] & ~inMask) | (inHexValue & inMask);
	return AJA_STATUS_SUCCESS;
}


// gets "raw" hex value, with no attempt to second-guess what the value is used for
// Note: "digitNum" is in transmission order, i.e. 0 = ls value, 7 = ms value)
AJAStatus AJAAncillaryData_Timecode::GetTimeHexValue (uint8_t inDigitNum, uint8_t & outHexValue, uint8_t inMask) const
{
	if (inDigitNum >= kNumTimeDigits)
		return AJA_STATUS_RANGE;

	outHexValue = (m_timeDigits[inDigitNum] & inMask);
	return AJA_STATUS_SUCCESS;
}


// Sets time digits assuming "time" values. Masks each digit according to the number of bits it actually uses in normal timecode applications.
// Note that the parameter order is the REVERSE of that used by SetTimeHexValue(): this order proceeds from ms digit (hour tens) to ls digit (frame units).
AJAStatus AJAAncillaryData_Timecode::SetTimeDigits (uint8_t hourTens, uint8_t hourUnits, uint8_t minuteTens, uint8_t minuteUnits, uint8_t secondTens, uint8_t secondUnits, uint8_t frameTens, uint8_t frameUnits)
{
	// Note: each digit is masked to keep only the number of bits allotted to it in the ancillary payload,
	// however no "time" limit-checking is done at this point, so it is possible to pass in (and get back!)
	// hex values that are out of BCD range and/or not appropriate for the digit (e.g. "38 hours...").
	// This allows "clever" users to use the bits for other nefarious purposes, but also allows for "bad"
	// timecode. In other words, if you (the Caller) care about correct range-checking of time values, do it yourself!
	SetTimeHexValue(kTcHourTens,	hourTens,	0x03);		// retain 2 ls bits
	SetTimeHexValue(kTcHourUnits,	hourUnits		);		// retain 4 ls bits
	SetTimeHexValue(kTcMinuteTens,	minuteTens, 0x07);		// retain 3 ls bits
	SetTimeHexValue(kTcMinuteUnits, minuteUnits		);		// retain 4 ls bits
	SetTimeHexValue(kTcSecondTens,	secondTens, 0x07);		// retain 3 ls bits
	SetTimeHexValue(kTcSecondUnits, secondUnits		);		// retain 4 ls bits
	SetTimeHexValue(kTcFrameTens,	frameTens,	0x03);		// retain 2 ls bits
	SetTimeHexValue(kTcFrameUnits,	frameUnits		);		// retain 4 ls bits
	return AJA_STATUS_SUCCESS;
}


// Gets time digits assuming "time" values. Masks each digit according to the number of bits it actually uses in normal timecode applications.
// Note that the parameter order is the REVERSE of that used by GetTimeHexValue(): this order proceeds from ms digit (hour tens) to ls digit (frame units).
AJAStatus AJAAncillaryData_Timecode::GetTimeDigits (uint8_t& hourTens, uint8_t& hourUnits, uint8_t& minuteTens, uint8_t& minuteUnits, uint8_t& secondTens, uint8_t& secondUnits, uint8_t& frameTens, uint8_t& frameUnits) const
{
	GetTimeHexValue(kTcHourTens,	hourTens,	 0x03);		// retain 2 ls bits
	GetTimeHexValue(kTcHourUnits,	hourUnits		 );		// retain 4 ls bits
	GetTimeHexValue(kTcMinuteTens,	minuteTens,	 0x07);		// retain 3 ls bits
	GetTimeHexValue(kTcMinuteUnits, minuteUnits		 );		// retain 4 ls bits
	GetTimeHexValue(kTcSecondTens,	secondTens,	 0x07);		// retain 3 ls bits
	GetTimeHexValue(kTcSecondUnits, secondUnits		 );		// retain 4 ls bits
	GetTimeHexValue(kTcFrameTens,	frameTens,	 0x03);		// retain 2 ls bits
	GetTimeHexValue(kTcFrameUnits,	frameUnits		 );		// retain 4 ls bits
	return AJA_STATUS_SUCCESS;
}


AJAStatus AJAAncillaryData_Timecode::SetTime (AJAAncillaryData_Timecode_Format tcFmt, uint32_t hours, uint32_t minutes, uint32_t seconds, uint32_t frames)
{
	if ( (hours >= 24) || (minutes >= 60) || (seconds >= 60) )
		return AJA_STATUS_RANGE;

	uint32_t framesPerSecond = 0;
	switch (tcFmt)
	{
		case AJAAncillaryData_Timecode_Format_60fps:	framesPerSecond = 60;	break;
		case AJAAncillaryData_Timecode_Format_50fps:	framesPerSecond = 50;	break;
		case AJAAncillaryData_Timecode_Format_48fps:	framesPerSecond = 48;	break;
		case AJAAncillaryData_Timecode_Format_30fps:	framesPerSecond = 30;	break;
		case AJAAncillaryData_Timecode_Format_25fps:	framesPerSecond = 25;	break;
		case AJAAncillaryData_Timecode_Format_24fps:	framesPerSecond = 24;	break;
		default:										framesPerSecond = 0;	break;		// triggers "range" error
	}

	if (frames >= framesPerSecond)
		return AJA_STATUS_RANGE;


	if (tcFmt == AJAAncillaryData_Timecode_Format_60fps || tcFmt == AJAAncillaryData_Timecode_Format_50fps || tcFmt == AJAAncillaryData_Timecode_Format_48fps)
	{
			// for "high" frame rates we need to use the "field ID" bit as an added ls bit to extend the frame digit range
		bool bOddFrame = (frames % 2 == 1 ? true : false);
		frames = frames / 2;

		SetFieldIdFlag(bOddFrame, tcFmt);		// note that FieldID does NOT get changed for "low" frame rates!
	}

	SetTimeDigits(	(uint8_t)(hours	  / 10), (uint8_t)(hours   % 10),		// hour digits
					(uint8_t)(minutes / 10), (uint8_t)(minutes % 10),		// minute digits
					(uint8_t)(seconds / 10), (uint8_t)(seconds % 10),		// second digits
					(uint8_t)(frames  / 10), (uint8_t)(frames  % 10) );		// frame digits

	return AJA_STATUS_SUCCESS;
}


AJAStatus AJAAncillaryData_Timecode::GetTime (AJAAncillaryData_Timecode_Format tcFmt, uint32_t& hours, uint32_t& minutes, uint32_t& seconds, uint32_t& frames) const
{
	uint8_t hourTens, hourUnits, minuteTens, minuteUnits, secondTens, secondUnits, frameTens, frameUnits;
	GetTimeDigits(hourTens, hourUnits, minuteTens, minuteUnits, secondTens, secondUnits, frameTens, frameUnits);

	hours	= (hourTens	  * 10) + hourUnits;
	minutes = (minuteTens * 10) + minuteUnits;
	seconds = (secondTens * 10) + secondUnits;
	frames	= (frameTens  * 10) + frameUnits;

	// for "high" frame rates we need to include the "field ID" bit as an added ls bit to extend the frame digit range
	if (tcFmt == AJAAncillaryData_Timecode_Format_60fps || tcFmt == AJAAncillaryData_Timecode_Format_50fps || tcFmt == AJAAncillaryData_Timecode_Format_48fps)
	{
		bool bFieldID = false;
		GetFieldIdFlag(bFieldID, tcFmt);

		frames = (frames * 2) + (bFieldID ? 1 : 0);
	}

	return AJA_STATUS_SUCCESS;
}


AJAStatus AJAAncillaryData_Timecode::SetTimecode (const AJATimeCode & inTimecode, const AJATimeBase & inTimeBase, const bool inIsDropFrame)
{
	uint32_t hours, minutes, seconds, frames;
	inTimecode.QueryHmsf (hours, minutes, seconds, frames, inTimeBase, inIsDropFrame);
	const AJAAncillaryData_Timecode_Format	tcFmt	(GetTimecodeFormatFromTimeBase(inTimeBase));
	SetTime (tcFmt, hours, minutes, seconds, frames);
	SetDropFrameFlag (inIsDropFrame, tcFmt);
	return AJA_STATUS_SUCCESS;
}


AJAStatus AJAAncillaryData_Timecode::GetTimecode (AJATimeCode & outTimecode, const AJATimeBase & inTimeBase) const
{
	AJAAncillaryData_Timecode_Format tcFmt = GetTimecodeFormatFromTimeBase(inTimeBase);

	uint32_t hours, minutes, seconds, frames;
	GetTime(tcFmt, hours, minutes, seconds, frames);

	bool bDropFrame = false;
	GetDropFrameFlag(bDropFrame, tcFmt);

	outTimecode.SetHmsf(hours, minutes, seconds, frames, inTimeBase, bDropFrame);

	return AJA_STATUS_SUCCESS;
}


//----------------------

// sets "raw" hex value, with no attempt to second-guess what the value is used for
// Note: "digitNum" is in transmission order, i.e. 0 = ls (BG1) value, 7 = ms (BG8) value)
AJAStatus AJAAncillaryData_Timecode::SetBinaryGroupHexValue(uint8_t digitNum, uint8_t hexValue, uint8_t mask)
{
	if (digitNum >= kNumBinaryGroups)
		return AJA_STATUS_RANGE;

	m_binaryGroup[digitNum] = (m_binaryGroup[digitNum] & ~mask) | (hexValue & mask);
	return AJA_STATUS_SUCCESS;
}


// gets "raw" hex value, with no attempt to second-guess what the value is used for
// Note: "digitNum" is in transmission order, i.e. 0 = ls (BG1) value, 7 = ms (BG8) value)
AJAStatus AJAAncillaryData_Timecode::GetBinaryGroupHexValue (uint8_t digitNum, uint8_t& hexValue, uint8_t mask) const
{
	if (digitNum >= kNumBinaryGroups)
		return AJA_STATUS_RANGE;

	hexValue = (m_binaryGroup[digitNum] & mask);
	return AJA_STATUS_SUCCESS;
}


AJAStatus AJAAncillaryData_Timecode::SetBinaryGroups (uint8_t bg8, uint8_t bg7, uint8_t bg6, uint8_t bg5, uint8_t bg4, uint8_t bg3, uint8_t bg2, uint8_t bg1)
{
	SetBinaryGroupHexValue(kBg1, bg1);
	SetBinaryGroupHexValue(kBg2, bg2);
	SetBinaryGroupHexValue(kBg3, bg3);
	SetBinaryGroupHexValue(kBg4, bg4);
	SetBinaryGroupHexValue(kBg5, bg5);
	SetBinaryGroupHexValue(kBg6, bg6);
	SetBinaryGroupHexValue(kBg7, bg7);
	SetBinaryGroupHexValue(kBg8, bg8);
	return AJA_STATUS_SUCCESS;
}


AJAStatus AJAAncillaryData_Timecode::GetBinaryGroups (uint8_t& bg8, uint8_t& bg7, uint8_t& bg6, uint8_t& bg5, uint8_t& bg4, uint8_t& bg3, uint8_t& bg2, uint8_t& bg1) const
{
	GetBinaryGroupHexValue(kBg1, bg1);
	GetBinaryGroupHexValue(kBg2, bg2);
	GetBinaryGroupHexValue(kBg3, bg3);
	GetBinaryGroupHexValue(kBg4, bg4);
	GetBinaryGroupHexValue(kBg5, bg5);
	GetBinaryGroupHexValue(kBg6, bg6);
	GetBinaryGroupHexValue(kBg7, bg7);
	GetBinaryGroupHexValue(kBg8, bg8);
	return AJA_STATUS_SUCCESS;
}


AJAStatus AJAAncillaryData_Timecode::SetFieldIdFlag (bool bFlag, AJAAncillaryData_Timecode_Format tcFmt)
{
	// the Field ID flag bit assignment varies depending on the video format
	switch (tcFmt)
	{
		case AJAAncillaryData_Timecode_Format_25fps:	// 25 fps ("PAL") format
		case AJAAncillaryData_Timecode_Format_50fps:	// 50 fps
			if (bFlag)
				m_timeDigits[kTcHourTens] |= 0x08;			// bit 3 of Hour Tens digit
			else 
				m_timeDigits[kTcHourTens] &= 0xF7;
			break;

		case AJAAncillaryData_Timecode_Format_24fps:	// 24/23.98 fps format
		case AJAAncillaryData_Timecode_Format_48fps:	// 48/47.95 fps
			if (bFlag)
				m_timeDigits[kTcSecondTens] |= 0x08;		// bit 3 of Second Tens digit
			else 
				m_timeDigits[kTcSecondTens] &= 0xF7;
			break;

		case AJAAncillaryData_Timecode_Format_30fps:	// 30/29.97 fps ("NTSC") format
		case AJAAncillaryData_Timecode_Format_60fps:	// 60/59.94 fps
		case AJAAncillaryData_Timecode_Format_Unknown:
			if (bFlag)
				m_timeDigits[kTcSecondTens] |= 0x08;		// bit 3 of Second Tens digit
			else 
				m_timeDigits[kTcSecondTens] &= 0xF7;
			break;

		default:
			return AJA_STATUS_RANGE;
	}

	return AJA_STATUS_SUCCESS;
}


AJAStatus AJAAncillaryData_Timecode::GetFieldIdFlag(bool& bFlag, AJAAncillaryData_Timecode_Format tcFmt) const
{
	// the Field ID flag bit assignment varies depending on the video format
	switch (tcFmt)
	{
		case AJAAncillaryData_Timecode_Format_25fps:			// 25 fps ("PAL") format
		case AJAAncillaryData_Timecode_Format_50fps:			// 50 fps
			bFlag = ((m_timeDigits[kTcHourTens] & 0x08) != 0);		// bit 3 of Hour Tens digit
			break;

		case AJAAncillaryData_Timecode_Format_24fps:			// 24/23.98 fps format
		case AJAAncillaryData_Timecode_Format_48fps:			// 48/47.95 fps
			bFlag = ((m_timeDigits[kTcSecondTens] & 0x08) != 0);	// bit 3 of Second Tens digit
			break;

		case AJAAncillaryData_Timecode_Format_30fps:			// 30/29.97 fps ("NTSC") format
		case AJAAncillaryData_Timecode_Format_60fps:			// 60/59.94 fps
		case AJAAncillaryData_Timecode_Format_Unknown:
			bFlag = ((m_timeDigits[kTcSecondTens] & 0x08) != 0);	// bit 3 of Second Tens digit
			break;

		default:
			return AJA_STATUS_RANGE;
	}

	return AJA_STATUS_SUCCESS;
}


AJAStatus AJAAncillaryData_Timecode::SetDropFrameFlag (bool bFlag, AJAAncillaryData_Timecode_Format tcFmt)
{
	switch (tcFmt)
	{
		// note: "Drop Frame" is not a valid flag for 25 fps ("PAL") video and should always be set to 0.
		// However, we're not going to stand in the way of someone who really wants to set it...
		case AJAAncillaryData_Timecode_Format_25fps:	// 25 fps ("PAL") format
		case AJAAncillaryData_Timecode_Format_50fps:	// 50 fps
			if (bFlag)
				m_timeDigits[kTcFrameTens] |= 0x04;			// bit 2 of Frame Tens digit
			else 
				m_timeDigits[kTcFrameTens] &= 0xFB;
			break;

		// note: "Drop Frame" is not a valid flag for 23.98/24 fps video and should always be set to 0.
		// However, we're not going to stand in the way of someone who really wants to set it...
		case AJAAncillaryData_Timecode_Format_24fps:	// 24/23.98 fps format
		case AJAAncillaryData_Timecode_Format_48fps:	// 48/47.95 fps
			if (bFlag)
				m_timeDigits[kTcFrameTens] |= 0x04;			// bit 2 of Frame Tens digit
			else 
				m_timeDigits[kTcFrameTens] &= 0xFB;
			break;

		case AJAAncillaryData_Timecode_Format_30fps:	// 30/29.97 fps ("NTSC") format
		case AJAAncillaryData_Timecode_Format_60fps:	// 60/59.94 fps
		case AJAAncillaryData_Timecode_Format_Unknown:
			if (bFlag)
				m_timeDigits[kTcFrameTens] |= 0x04;			// bit 2 of Frame Tens digit
			else 
				m_timeDigits[kTcFrameTens] &= 0xFB;
			break;

		default:
			return AJA_STATUS_RANGE;
	}

	return AJA_STATUS_SUCCESS;
}


AJAStatus AJAAncillaryData_Timecode::GetDropFrameFlag (bool& bFlag, AJAAncillaryData_Timecode_Format tcFmt) const
{
	switch (tcFmt)
	{
		case AJAAncillaryData_Timecode_Format_25fps:			// 25 fps ("PAL") format
		case AJAAncillaryData_Timecode_Format_50fps:			// 50 fps
			bFlag = ((m_timeDigits[kTcFrameTens] & 0x04) != 0);		// bit 2 of Frame Tens digit
			break;

		case AJAAncillaryData_Timecode_Format_24fps:			// 24/23.98 fps format
		case AJAAncillaryData_Timecode_Format_48fps:			// 48/47.95 fps
			bFlag = ((m_timeDigits[kTcFrameTens] & 0x04) != 0);		// bit 2 of Frame Tens digit
			break;

		case AJAAncillaryData_Timecode_Format_30fps:			// 30/29.97 fps ("NTSC") format
		case AJAAncillaryData_Timecode_Format_60fps:			// 60/59.94 fps
		case AJAAncillaryData_Timecode_Format_Unknown:
			bFlag = ((m_timeDigits[kTcFrameTens] & 0x04) != 0);		// bit 2 of Frame Tens digit
			break;

		default:
			return AJA_STATUS_RANGE;
	}

	return AJA_STATUS_SUCCESS;
}


AJAStatus AJAAncillaryData_Timecode::SetColorFrameFlag(bool bFlag, AJAAncillaryData_Timecode_Format tcFmt)
{
	switch (tcFmt)
	{
		case AJAAncillaryData_Timecode_Format_25fps:	// 25 fps ("PAL") format
		case AJAAncillaryData_Timecode_Format_50fps:	// 50 fps
			if (bFlag)
				m_timeDigits[kTcFrameTens] |= 0x08;			// bit 3 of Frame Tens digit
			else 
				m_timeDigits[kTcFrameTens] &= 0xF7;
			break;

		// note: "Color Frame" is not a valid flag for 23.98/24 fps video and should always be set to 0.
		// However, we're not going to stand in the way of someone who really wants to set it...
		case AJAAncillaryData_Timecode_Format_24fps:	// 24/23.98 fps format
		case AJAAncillaryData_Timecode_Format_48fps:	// 48/47.95 fps
			if (bFlag)
				m_timeDigits[kTcFrameTens] |= 0x08;			// bit 3 of Frame Tens digit
			else 
				m_timeDigits[kTcFrameTens] &= 0xF7;
			break;

		case AJAAncillaryData_Timecode_Format_30fps:	// 30/29.97 fps ("NTSC") format
		case AJAAncillaryData_Timecode_Format_60fps:	// 60/59.94 fps
		case AJAAncillaryData_Timecode_Format_Unknown:
			if (bFlag)
				m_timeDigits[kTcFrameTens] |= 0x08;			// bit 3 of Frame Tens digit
			else 
				m_timeDigits[kTcFrameTens] &= 0xF7;
			break;

		default:
			return AJA_STATUS_RANGE;
	}

	return AJA_STATUS_SUCCESS;
}


AJAStatus AJAAncillaryData_Timecode::GetColorFrameFlag (bool& bFlag, AJAAncillaryData_Timecode_Format tcFmt) const
{
	switch (tcFmt)
	{
		case AJAAncillaryData_Timecode_Format_25fps:			// 25 fps ("PAL") format
		case AJAAncillaryData_Timecode_Format_50fps:			// 50 fps
			bFlag = ((m_timeDigits[kTcFrameTens] & 0x08) != 0);		// bit 3 of Frame Tens digit
			break;

		case AJAAncillaryData_Timecode_Format_24fps:			// 24/23.98 fps format
		case AJAAncillaryData_Timecode_Format_48fps:			// 48/47.95 fps
			bFlag = ((m_timeDigits[kTcFrameTens] & 0x08) != 0);		// bit 3 of Frame Tens digit
			break;

		case AJAAncillaryData_Timecode_Format_30fps:			// 30/29.97 fps ("NTSC") format
		case AJAAncillaryData_Timecode_Format_60fps:			// 60/59.94 fps
		case AJAAncillaryData_Timecode_Format_Unknown:
			bFlag = ((m_timeDigits[kTcFrameTens] & 0x08) != 0);		// bit 3 of Frame Tens digit
			break;

		default:
			return AJA_STATUS_RANGE;
	}

	return AJA_STATUS_SUCCESS;
}


AJAStatus AJAAncillaryData_Timecode::SetBinaryGroupFlag(uint8_t bgFlag, AJAAncillaryData_Timecode_Format tcFmt)
{
	// the binary group flag bits vary depending on the video format
	switch (tcFmt)
	{
		case AJAAncillaryData_Timecode_Format_25fps:	// 25 fps ("PAL") format
		case AJAAncillaryData_Timecode_Format_50fps:	// 50 fps

			if ((bgFlag & 0x04) != 0)					// BG Flag, bit 2:
				m_timeDigits[kTcMinuteTens] |= 0x08;		// bit 3 of Minute Tens digit
			else 
				m_timeDigits[kTcMinuteTens] &= 0xF7;

			if ((bgFlag & 0x02) != 0)					// BG Flag, bit 1:
				m_timeDigits[kTcHourTens] |= 0x04;			// bit 2 of Hour Tens digit
			else 
				m_timeDigits[kTcHourTens] &= 0xFB;

			if ((bgFlag & 0x01) != 0)					// BG Flag, bit 0:
				m_timeDigits[kTcSecondTens] |= 0x08;		// bit 3 of Second Tens digit
			else 
				m_timeDigits[kTcSecondTens] &= 0xF7;

			break;


		case AJAAncillaryData_Timecode_Format_24fps:	// 24/23.98 fps format
		case AJAAncillaryData_Timecode_Format_48fps:	// 48/47.95 fps

			if ((bgFlag & 0x04) != 0)					// BG Flag, bit 2:
				m_timeDigits[kTcHourTens] |= 0x08;			// bit 3 of Hour Tens digit
			else 
				m_timeDigits[kTcHourTens] &= 0xF7;

			if ((bgFlag & 0x02) != 0)					// BG Flag, bit 1:
				m_timeDigits[kTcHourTens] |= 0x04;			// bit 2 of Hour Tens digit
			else 
				m_timeDigits[kTcHourTens] &= 0xFB;

			if ((bgFlag & 0x01) != 0)					// BG Flag, bit 0:
				m_timeDigits[kTcMinuteTens] |= 0x08;		// bit 3 of Minute Tens digit
			else 
				m_timeDigits[kTcMinuteTens] &= 0xF7;

			break;

		case AJAAncillaryData_Timecode_Format_30fps:	// 30/29.97 fps ("NTSC") format
		case AJAAncillaryData_Timecode_Format_60fps:	// 60/59.94 fps
		case AJAAncillaryData_Timecode_Format_Unknown:

			if ((bgFlag & 0x04) != 0)					// BG Flag, bit 2:
				m_timeDigits[kTcHourTens] |= 0x08;			// bit 3 of Hour Tens digit
			else 
				m_timeDigits[kTcHourTens] &= 0xF7;

			if ((bgFlag & 0x02) != 0)					// BG Flag, bit 1:
				m_timeDigits[kTcHourTens] |= 0x04;			// bit 2 of Hour Tens digit
			else 
				m_timeDigits[kTcHourTens] &= 0xFB;

			if ((bgFlag & 0x01) != 0)					// BG Flag, bit 0:
				m_timeDigits[kTcMinuteTens] |= 0x08;		// bit 3 of Minute Tens digit
			else 
				m_timeDigits[kTcMinuteTens] &= 0xF7;

			break;

		default:
			return AJA_STATUS_RANGE;
	}

	return AJA_STATUS_SUCCESS;
}


AJAStatus AJAAncillaryData_Timecode::GetBinaryGroupFlag (uint8_t & outBGFlag, const AJAAncillaryData_Timecode_Format inFormat) const
{
	uint8_t bf2, bf1, bf0;

	// the binary group flag bits vary depending on the video format
	switch (inFormat)
	{
		case AJAAncillaryData_Timecode_Format_25fps:			// 25 fps ("PAL") format
		case AJAAncillaryData_Timecode_Format_50fps:			// 50 fps
			bf2 = (m_timeDigits[kTcMinuteTens] & 0x08) >> 3;		// bit 3 of Hour Tens digit
			bf1 = (m_timeDigits[kTcHourTens]   & 0x04) >> 2;		// bit 2 of Hour Tens digit
			bf0 = (m_timeDigits[kTcSecondTens] & 0x08) >> 3;		// bit 3 of Second Tens digit
			break;

		case AJAAncillaryData_Timecode_Format_24fps:			// 24/23.98 fps format
		case AJAAncillaryData_Timecode_Format_48fps:			// 48/47.95 fps
			bf2 = (m_timeDigits[kTcHourTens]   & 0x08) >> 3;		// bit 3 of Hour Tens digit
			bf1 = (m_timeDigits[kTcHourTens]   & 0x04) >> 2;		// bit 2 of Hour Tens digit
			bf0 = (m_timeDigits[kTcMinuteTens] & 0x08) >> 3;		// bit 3 of Minute Tens digit
			break;

		case AJAAncillaryData_Timecode_Format_30fps:			// 30/29.97 fps ("NTSC") format
		case AJAAncillaryData_Timecode_Format_60fps:			// 60/59.94 fps
		case AJAAncillaryData_Timecode_Format_Unknown:
			bf2 = (m_timeDigits[kTcHourTens]   & 0x08) >> 3;		// bit 3 of Hour Tens digit
			bf1 = (m_timeDigits[kTcHourTens]   & 0x04) >> 2;		// bit 2 of Hour Tens digit
			bf0 = (m_timeDigits[kTcMinuteTens] & 0x08) >> 3;		// bit 3 of Minute Tens digit
			break;

		default:
			return AJA_STATUS_RANGE;
	}

	outBGFlag = (bf2 << 2) + (bf1 << 1) + bf0;
	return AJA_STATUS_SUCCESS;
}


//----------------------

// Bit layout of the two 32-bit "RP188 words" used to carry SMPTE 12M timecode over NTV2
// ancillary/register APIs. Scoped to this file only -- not part of the public API.
namespace
{
	enum
	{
		kFrameUnitsMask		= 0xf,	kFrameUnitsShift		= 0,
		kBinaryGroup1Mask	= 0xf,	kBinaryGroup1Shift		= 4,
		kFrameTensMask		= 0x3,	kFrameTensShift			= 8,
		kDropFrameMask		= 0x1,	kDropFrameShift			= 10,
		kColorFrameMask		= 0x1,	kColorFrameShift		= 11,
		kBinaryGroup2Mask	= 0xf,	kBinaryGroup2Shift		= 12,
		kSecondUnitsMask	= 0xf,	kSecondUnitsShift		= 16,
		kBinaryGroup3Mask	= 0xf,	kBinaryGroup3Shift		= 20,
		kSecondTensMask		= 0x7,	kSecondTensShift		= 24,
		kFieldIdLoMask		= 0x1,	kFieldIdLoShift			= 27,
		kBinaryGroup4Mask	= 0xf,	kBinaryGroup4Shift		= 28,

		kMinuteUnitsMask	= 0xf,	kMinuteUnitsShift		= 0,
		kBinaryGroup5Mask	= 0xf,	kBinaryGroup5Shift		= 4,
		kMinuteTensMask		= 0x7,	kMinuteTensShift		= 8,
		kFieldIdHiMask		= 0x1,	kFieldIdHiShift			= 11,
		kBinaryGroup6Mask	= 0xf,	kBinaryGroup6Shift		= 12,
		kHourUnitsMask		= 0xf,	kHourUnitsShift			= 16,
		kBinaryGroup7Mask	= 0xf,	kBinaryGroup7Shift		= 20,
		kHourTensMask		= 0x3,	kHourTensShift			= 24,
		kBinaryGroupFlagMask= 0x3,	kBinaryGroupFlagShift	= 26,	// only 2 of 3 BGF bits fit here
		kBinaryGroup8Mask	= 0xf,	kBinaryGroup8Shift		= 28
	};
}

AJAStatus AJAAncillaryData_Timecode::SetRP188Words (const uint32_t inLoWord, const uint32_t inHiWord, const AJATimeBase & inTimeBase)
{
	const AJAAncillaryData_Timecode_Format tcFmt (GetTimecodeFormatFromTimeBase(inTimeBase));

	const uint8_t frameUnits = (inLoWord >> kFrameUnitsShift)   & kFrameUnitsMask;
	const uint8_t bg1        = (inLoWord >> kBinaryGroup1Shift) & kBinaryGroup1Mask;
	const uint8_t frameTens  = (inLoWord >> kFrameTensShift)    & kFrameTensMask;
	const bool    dropFrame  = (inLoWord >> kDropFrameShift)    & kDropFrameMask;
	const bool    colorFrame = (inLoWord >> kColorFrameShift)   & kColorFrameMask;
	const uint8_t bg2        = (inLoWord >> kBinaryGroup2Shift) & kBinaryGroup2Mask;
	const uint8_t secUnits   = (inLoWord >> kSecondUnitsShift)  & kSecondUnitsMask;
	const uint8_t bg3        = (inLoWord >> kBinaryGroup3Shift) & kBinaryGroup3Mask;
	const uint8_t secTens    = (inLoWord >> kSecondTensShift)   & kSecondTensMask;
	const bool    fieldIdLo  = (inLoWord >> kFieldIdLoShift)    & kFieldIdLoMask;
	const uint8_t bg4        = (inLoWord >> kBinaryGroup4Shift) & kBinaryGroup4Mask;

	const uint8_t minUnits   = (inHiWord >> kMinuteUnitsShift)     & kMinuteUnitsMask;
	const uint8_t bg5        = (inHiWord >> kBinaryGroup5Shift)    & kBinaryGroup5Mask;
	const uint8_t minTens    = (inHiWord >> kMinuteTensShift)      & kMinuteTensMask;
	// fieldIdHi (hiWord bit 11) is a redundant mirror of fieldIdLo -- not an independent value
	// (the class models Field ID as a single flag), so it's read for completeness but not used.
	const uint8_t hourUnits  = (inHiWord >> kHourUnitsShift)       & kHourUnitsMask;
	const uint8_t bg7        = (inHiWord >> kBinaryGroup7Shift)    & kBinaryGroup7Mask;
	const uint8_t hourTens   = (inHiWord >> kHourTensShift)        & kHourTensMask;
	const uint8_t bgFlag     = (inHiWord >> kBinaryGroupFlagShift) & kBinaryGroupFlagMask;
	const uint8_t bg6        = (inHiWord >> kBinaryGroup6Shift)    & kBinaryGroup6Mask;
	const uint8_t bg8        = (inHiWord >> kBinaryGroup8Shift)    & kBinaryGroup8Mask;

	AJAStatus status = SetTimeDigits(hourTens, hourUnits, minTens, minUnits, secTens, secUnits, frameTens, frameUnits);
	if (AJA_FAILURE(status)) return status;
	status = SetBinaryGroups(bg8, bg7, bg6, bg5, bg4, bg3, bg2, bg1);
	if (AJA_FAILURE(status)) return status;
	status = SetDropFrameFlag(dropFrame, tcFmt);
	if (AJA_FAILURE(status)) return status;
	status = SetColorFrameFlag(colorFrame, tcFmt);
	if (AJA_FAILURE(status)) return status;
	status = SetFieldIdFlag(fieldIdLo, tcFmt);
	if (AJA_FAILURE(status)) return status;
	return SetBinaryGroupFlag(bgFlag, tcFmt);
}

AJAStatus AJAAncillaryData_Timecode::GetRP188Words (uint32_t & outLoWord, uint32_t & outHiWord, const AJATimeBase & inTimeBase) const
{
	const AJAAncillaryData_Timecode_Format tcFmt (GetTimecodeFormatFromTimeBase(inTimeBase));

	uint8_t hourTens, hourUnits, minTens, minUnits, secTens, secUnits, frameTens, frameUnits;
	AJAStatus status = GetTimeDigits(hourTens, hourUnits, minTens, minUnits, secTens, secUnits, frameTens, frameUnits);
	if (AJA_FAILURE(status)) return status;

	uint8_t bg1, bg2, bg3, bg4, bg5, bg6, bg7, bg8;
	status = GetBinaryGroups(bg8, bg7, bg6, bg5, bg4, bg3, bg2, bg1);
	if (AJA_FAILURE(status)) return status;

	bool dropFrame = false;
	status = GetDropFrameFlag(dropFrame, tcFmt);
	if (AJA_FAILURE(status)) return status;

	bool colorFrame = false;
	status = GetColorFrameFlag(colorFrame, tcFmt);
	if (AJA_FAILURE(status)) return status;

	bool fieldID = false;
	status = GetFieldIdFlag(fieldID, tcFmt);
	if (AJA_FAILURE(status)) return status;

	uint8_t bgFlag = 0;
	status = GetBinaryGroupFlag(bgFlag, tcFmt);
	if (AJA_FAILURE(status)) return status;

	outLoWord = 0;
	outLoWord |= (frameUnits & kFrameUnitsMask)		<< kFrameUnitsShift;
	outLoWord |= (bg1 & kBinaryGroup1Mask)				<< kBinaryGroup1Shift;
	outLoWord |= (frameTens & kFrameTensMask)			<< kFrameTensShift;
	outLoWord |= ((uint8_t)dropFrame & kDropFrameMask)	<< kDropFrameShift;
	outLoWord |= ((uint8_t)colorFrame & kColorFrameMask) << kColorFrameShift;
	outLoWord |= (bg2 & kBinaryGroup2Mask)				<< kBinaryGroup2Shift;
	outLoWord |= (secUnits & kSecondUnitsMask)			<< kSecondUnitsShift;
	outLoWord |= (bg3 & kBinaryGroup3Mask)				<< kBinaryGroup3Shift;
	outLoWord |= (secTens & kSecondTensMask)			<< kSecondTensShift;
	outLoWord |= ((uint8_t)fieldID & kFieldIdLoMask)	<< kFieldIdLoShift;
	outLoWord |= (bg4 & kBinaryGroup4Mask)				<< kBinaryGroup4Shift;

	outHiWord = 0;
	outHiWord |= (minUnits & kMinuteUnitsMask)			<< kMinuteUnitsShift;
	outHiWord |= (bg5 & kBinaryGroup5Mask)				<< kBinaryGroup5Shift;
	outHiWord |= (minTens & kMinuteTensMask)			<< kMinuteTensShift;
	outHiWord |= ((uint8_t)fieldID & kFieldIdHiMask)	<< kFieldIdHiShift;	// redundant mirror of loWord's bit
	outHiWord |= (bg6 & kBinaryGroup6Mask)				<< kBinaryGroup6Shift;
	outHiWord |= (hourUnits & kHourUnitsMask)			<< kHourUnitsShift;
	outHiWord |= (bg7 & kBinaryGroup7Mask)				<< kBinaryGroup7Shift;
	outHiWord |= (hourTens & kHourTensMask)				<< kHourTensShift;
	outHiWord |= (bgFlag & kBinaryGroupFlagMask)		<< kBinaryGroupFlagShift;	// only 2 of 3 bits fit
	outHiWord |= (bg8 & kBinaryGroup8Mask)				<< kBinaryGroup8Shift;

	return AJA_STATUS_SUCCESS;
}

AJAAncillaryData_Timecode_Format AJAAncillaryData_Timecode::GetTimecodeFormatFromTimeBase (const AJATimeBase & timeBase)
{
	AJAAncillaryData_Timecode_Format tcFmt = AJAAncillaryData_Timecode_Format_Unknown;

	int64_t rate, duration;
	timeBase.GetFrameRate(rate, duration);
	double frameRate = (double)rate / (double)duration;

	// we're just going to coarsely (and arbitrarily) quantize the excruciatingly accurate frame rate...
	if (frameRate < 24.5)
		tcFmt = AJAAncillaryData_Timecode_Format_24fps;
	else if (frameRate < 28.0)
		tcFmt = AJAAncillaryData_Timecode_Format_25fps;
	else if (frameRate < 35.0)
		tcFmt = AJAAncillaryData_Timecode_Format_30fps;
	else if (frameRate < 49.0)
		tcFmt = AJAAncillaryData_Timecode_Format_48fps;
	else if (frameRate < 55.0)
		tcFmt = AJAAncillaryData_Timecode_Format_50fps;
	else
		tcFmt = AJAAncillaryData_Timecode_Format_60fps;
	return tcFmt;
}


ostream & AJAAncillaryData_Timecode::Print (ostream & debugStream, const bool bShowDetail) const
{
	debugStream << IDAsString() << "(" << ::AJAAncDataCodingToString (m_coding) << ")" << endl;
	AJAAncillaryData::Print (debugStream, bShowDetail);

	uint8_t		timeDigits[kNumTimeDigits];
	bool		bFieldIdFlag, bColorFrameFlag, bDropFrameFlag;
	uint8_t		binaryGroup [kNumBinaryGroups];
	uint8_t		binaryGroupFlag;

	GetTimeDigits (timeDigits[kTcHourTens], timeDigits[kTcHourUnits], timeDigits[kTcMinuteTens], timeDigits[kTcMinuteUnits], timeDigits[kTcSecondTens], timeDigits[kTcSecondUnits], timeDigits[kTcFrameTens], timeDigits[kTcFrameUnits]);
	GetFieldIdFlag (bFieldIdFlag);
	GetColorFrameFlag (bColorFrameFlag);
	GetDropFrameFlag (bDropFrameFlag);
	GetBinaryGroups (binaryGroup[kBg8], binaryGroup[kBg7], binaryGroup[kBg6], binaryGroup[kBg5], binaryGroup[kBg4], binaryGroup[kBg3], binaryGroup[kBg2], binaryGroup[kBg1]);
	GetBinaryGroupFlag (binaryGroupFlag);

	debugStream << endl
				<< "Base Timecode Info:" << endl
				<< "Time: " << dec << setw(1) << (uint32_t)timeDigits[kTcHourTens] << setw(1) << (uint32_t)timeDigits[kTcHourUnits]
				<< ":" << setw(1) << (uint32_t)timeDigits[kTcMinuteTens] << setw(1) << (uint32_t)timeDigits[kTcMinuteUnits]
				<< ":" << setw(1) << (uint32_t)timeDigits[kTcSecondTens] << setw(1) << (uint32_t)timeDigits[kTcSecondUnits]
				<< ":" << setw(1) << (uint32_t)timeDigits[kTcFrameTens]	 << setw(1) << (uint32_t)timeDigits[kTcFrameUnits] << endl
				<< "Field ID Flag: " << (bFieldIdFlag ? "f1" : "f0") << endl
				<< "Drop Frame Flag: " << (bDropFrameFlag ? "Drop" : "Non-drop") << endl
				<< "Color Frame: " << (bColorFrameFlag ? "On" : "Off") << endl
				<< "Binary Group: " << hex << setw(1) << uint16_t(binaryGroup[kBg8]) << setw(1) << uint16_t(binaryGroup[kBg7])
				<< ":" << setw(1) << uint16_t(binaryGroup[kBg6]) << setw(1) << uint16_t(binaryGroup[kBg5])
				<< ":" << setw(1) << uint16_t(binaryGroup[kBg4]) << setw(1) << uint16_t(binaryGroup[kBg3])
				<< ":" << setw(1) << uint16_t(binaryGroup[kBg2]) << setw(1) << uint16_t(binaryGroup[kBg1]) << endl
				<< "BG Flag: " << uint16_t(binaryGroupFlag);
	return debugStream;
}

string AJAAncillaryData_Timecode::TimecodeString (void) const
{
	ostringstream	oss;
	uint8_t			timeDigits[kNumTimeDigits];
	GetTimeDigits (timeDigits[kTcHourTens], timeDigits[kTcHourUnits], timeDigits[kTcMinuteTens], timeDigits[kTcMinuteUnits], timeDigits[kTcSecondTens], timeDigits[kTcSecondUnits], timeDigits[kTcFrameTens], timeDigits[kTcFrameUnits]);
	oss << dec << setw(1) << uint32_t(timeDigits[kTcHourTens])	 << setw(1) << uint32_t(timeDigits[kTcHourUnits])
		<< ":" << setw(1) << uint32_t(timeDigits[kTcMinuteTens]) << setw(1) << uint32_t(timeDigits[kTcMinuteUnits])
		<< ":" << setw(1) << uint32_t(timeDigits[kTcSecondTens]) << setw(1) << uint32_t(timeDigits[kTcSecondUnits])
		<< ":" << setw(1) << uint32_t(timeDigits[kTcFrameTens])	 << setw(1) << uint32_t(timeDigits[kTcFrameUnits]);
	return oss.str();
}
