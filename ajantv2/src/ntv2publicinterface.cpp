/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2publicinterface.cpp
	@brief		Implementations of methods declared in 'ntv2publicinterface.h'.
	@copyright	(C) 2016-2022 AJA Video Systems, Inc.
**/
#include "ntv2publicinterface.h"
#include "ntv2devicefeatures.h"
#include "ntv2utils.h"
#include "ntv2endian.h"
#include "ajabase/system/memory.h"
#include "ajabase/system/debug.h"
#include "ajabase/common/common.h"
#include "ntv2registerexpert.h"
#include "ntv2nubtypes.h"
#include "ntv2version.h"
#include "ntv2debug.h"
#include <iomanip>
#include <locale>		//	For std::locale, std::numpunct, std::use_facet
#include <string.h>		//	For memset, et al.
#include <algorithm>	//	For set_difference
#include <iterator>		//	For std::inserter
#include "ntv2rp188.h"
#if !defined(MSWindows)
	#include <unistd.h>
#endif
using namespace std;

//#define NTV2BUFFER_NO_MEMCMP


/////////// Stream Operators

ostream & operator << (ostream & inOutStr, const NTV2AudioChannelPairs & inSet)
{
	if (inSet.empty())
		inOutStr << "(none)";
	else
		for (NTV2AudioChannelPairsConstIter iter (inSet.begin ());	iter != inSet.end ();  ++iter)
			inOutStr << (iter != inSet.begin() ? ", " : "") << ::NTV2AudioChannelPairToString (*iter, true);
	return inOutStr;
}

ostream &	operator << (ostream & inOutStr, const NTV2AudioChannelQuads & inSet)
{
	for (NTV2AudioChannelQuadsConstIter iter (inSet.begin ());	iter != inSet.end ();  ++iter)
		inOutStr << (iter != inSet.begin () ? ", " : "") << ::NTV2AudioChannelQuadToString (*iter, true);
	return inOutStr;
}

ostream &	operator << (ostream & inOutStr, const NTV2AudioChannelOctets & inSet)
{
	for (NTV2AudioChannelOctetsConstIter iter (inSet.begin ());	 iter != inSet.end ();	++iter)
		inOutStr << (iter != inSet.begin () ? ", " : "") << ::NTV2AudioChannelOctetToString (*iter, true);
	return inOutStr;
}

ostream &	operator << (ostream & inOutStr, const NTV2DoubleArray & inVector)
{
	for (NTV2DoubleArrayConstIter iter (inVector.begin ());	 iter != inVector.end ();  ++iter)
		inOutStr << *iter << endl;
	return inOutStr;
}

ostream &	operator << (ostream & inOutStr, const NTV2DIDSet & inDIDs)
{
	for (NTV2DIDSetConstIter it (inDIDs.begin());  it != inDIDs.end();	)
	{
		inOutStr << xHEX0N(uint16_t(*it),2);
		if (++it != inDIDs.end())
			inOutStr << ", ";
	}
	return inOutStr;
}

ostream & operator << (ostream & inOutStream, const UWordSequence & inData)
{
	inOutStream << DEC(inData.size()) << " UWords: ";
	for (UWordSequenceConstIter iter(inData.begin());  iter != inData.end();  )
	{
		inOutStream << HEX0N(*iter,4);
		if (++iter != inData.end())
			inOutStream << " ";
	}
	return inOutStream;
}

ostream & operator << (ostream & inOutStream, const ULWordSequence & inData)
{
	inOutStream << DEC(inData.size()) << " ULWords: ";
	for (ULWordSequenceConstIter iter(inData.begin());	iter != inData.end();  )
	{
		inOutStream << HEX0N(*iter,8);
		if (++iter != inData.end())
			inOutStream << " ";
	}
	return inOutStream;
}

ostream & operator << (ostream & inOutStream, const ULWord64Sequence & inData)
{
	inOutStream << DEC(inData.size()) << " ULWord64s: ";
	for (ULWord64SequenceConstIter iter(inData.begin());  iter != inData.end();	 )
	{
		inOutStream << HEX0N(*iter,16);
		if (++iter != inData.end())
			inOutStream << " ";
	}
	return inOutStream;
}


NTV2SDIInputStatus::NTV2SDIInputStatus ()
{
	Clear ();
}


void NTV2SDIInputStatus::Clear (void)
{
	mCRCTallyA			= 0;
	mCRCTallyB			= 0;
	mUnlockTally		= 0;
	mFrameRefClockCount = 0;
	mGlobalClockCount	= 0;
	mFrameTRSError		= false;
	mLocked				= false;
	mVPIDValidA			= false;
	mVPIDValidB			= false;
}


ostream & NTV2SDIInputStatus::Print (ostream & inOutStream) const
{
	inOutStream << "[CRCA="			<< DEC(mCRCTallyA)
				<< " CRCB="			<< DEC(mCRCTallyB)
				<< " unlk="			<< xHEX0N(mUnlockTally,8)
				<< " frmRefClkCnt=" << xHEX0N(mFrameRefClockCount,16)
				<< " globalClkCnt=" << xHEX0N(mGlobalClockCount,16)
				<< " frmTRS="		<< YesNo(mFrameTRSError)
				<< " locked="		<< YesNo(mLocked)
				<< " VPIDA="		<< YesNo(mVPIDValidA)
				<< " VPIDB="		<< YesNo(mVPIDValidB)
				<< "]";
	return inOutStream;
}


void NTV2HDMIOutputStatus::Clear (void)
{
	mEnabled		= false;
	mPixel420		= false;
	mColorSpace		= NTV2_INVALID_HDMI_COLORSPACE;
	mRGBRange		= NTV2_INVALID_HDMI_RANGE;
	mProtocol		= NTV2_INVALID_HDMI_PROTOCOL;
	mVideoStandard	= NTV2_STANDARD_INVALID;
	mVideoRate		= NTV2_FRAMERATE_UNKNOWN;
	mVideoBitDepth	= NTV2_INVALID_HDMIBitDepth;
	mAudioFormat	= NTV2_AUDIO_FORMAT_INVALID;
	mAudioRate		= NTV2_AUDIO_RATE_INVALID;
	mAudioChannels	= NTV2_INVALID_HDMI_AUDIO_CHANNELS;
}

bool NTV2HDMIOutputStatus::SetFromRegValue (const ULWord inData)
{
	Clear();
	mVideoRate		= NTV2FrameRate((inData & kVRegMaskHDMOutVideoFrameRate) >> kVRegShiftHDMOutVideoFrameRate);
	if (mVideoRate == NTV2_FRAMERATE_UNKNOWN)
		return true;	//	Not enabled -- success
	mEnabled		= true;
	mPixel420		= ((inData & kVRegMaskHDMOutPixel420) >> kVRegShiftHDMOutPixel420) == 1;
	mColorSpace		= NTV2HDMIColorSpace(((inData & kVRegMaskHDMOutColorRGB) >> kVRegShiftHDMOutColorRGB) ? NTV2_HDMIColorSpaceRGB : NTV2_HDMIColorSpaceYCbCr);
	mRGBRange		= NTV2HDMIRange((inData & kVRegMaskHDMOutRangeFull) >> kVRegShiftHDMOutRangeFull);
	mProtocol		= NTV2HDMIProtocol((inData & kVRegMaskHDMOutProtocol) >> kVRegShiftHDMOutProtocol);
	mVideoStandard	= NTV2Standard((inData & kVRegMaskHDMOutVideoStandard) >> kVRegShiftHDMOutVideoStandard);
	mVideoBitDepth	= NTV2HDMIBitDepth((inData & kVRegMaskHDMOutBitDepth) >> kVRegShiftHDMOutBitDepth);
	mAudioFormat	= NTV2AudioFormat((inData & kVRegMaskHDMOutAudioFormat) >> kVRegShiftHDMOutAudioFormat);
	mAudioRate		= NTV2AudioRate((inData & kVRegMaskHDMOutAudioRate) >> kVRegShiftHDMOutAudioRate);
	mAudioChannels	= NTV2HDMIAudioChannels((inData & kVRegMaskHDMOutAudioChannels) >> kVRegShiftHDMOutAudioChannels);
	return true;
}

ostream & NTV2HDMIOutputStatus::Print (ostream & inOutStream) const
{
	inOutStream << "Enabled: "			<< YesNo(mEnabled);
	if (mEnabled)
		inOutStream << endl
					<< "Is 4:2:0: "			<< YesNo(mPixel420)									<< endl
					<< "Color Space: "		<< ::NTV2HDMIColorSpaceToString(mColorSpace,true)	<< endl;
	if (mColorSpace == NTV2_HDMIColorSpaceRGB)
		inOutStream << "RGB Range: "		<< ::NTV2HDMIRangeToString(mRGBRange,true)			<< endl;
	inOutStream		<< "Protocol: "			<< ::NTV2HDMIProtocolToString(mProtocol,true)		<< endl
					<< "Video Standard: "	<< ::NTV2StandardToString(mVideoStandard,true)		<< endl
					<< "Frame Rate: "		<< ::NTV2FrameRateToString(mVideoRate,true)			<< endl
					<< "Bit Depth: "		<< ::NTV2HDMIBitDepthToString(mVideoBitDepth,true)	<< endl
					<< "Audio Format: "		<< ::NTV2AudioFormatToString(mAudioFormat,true)		<< endl
					<< "Audio Rate: "		<< ::NTV2AudioRateToString(mAudioRate,true)			<< endl
					<< "Audio Channels: "	<< ::NTV2HDMIAudioChannelsToString(mAudioChannels,true);
	return inOutStream;
}


AutoCircVidProcInfo::AutoCircVidProcInfo ()
	:	mode						(AUTOCIRCVIDPROCMODE_MIX),
		foregroundVideoCrosspoint	(NTV2CROSSPOINT_CHANNEL1),
		backgroundVideoCrosspoint	(NTV2CROSSPOINT_CHANNEL1),
		foregroundKeyCrosspoint		(NTV2CROSSPOINT_CHANNEL1),
		backgroundKeyCrosspoint		(NTV2CROSSPOINT_CHANNEL1),
		transitionCoefficient		(0),
		transitionSoftness			(0)
{
}


AUTOCIRCULATE_DATA::AUTOCIRCULATE_DATA (const AUTO_CIRC_COMMAND inCommand, const NTV2Crosspoint inCrosspoint)
	:	eCommand	(inCommand),
		channelSpec (inCrosspoint),
		lVal1		(0),
		lVal2		(0),
		lVal3		(0),
		lVal4		(0),
		lVal5		(0),
		lVal6		(0),
		bVal1		(false),
		bVal2		(false),
		bVal3		(false),
		bVal4		(false),
		bVal5		(false),
		bVal6		(false),
		bVal7		(false),
		bVal8		(false),
		pvVal1		(AJA_NULL),
		pvVal2		(AJA_NULL),
		pvVal3		(AJA_NULL),
		pvVal4		(AJA_NULL)
{
}

ostream & AUTOCIRCULATE_DATA::Print (ostream & oss) const
{
	static const string sCmds [] = {"ACInit","ACStart","ACStop","ACPause","GetAC","ACFrmStmp",
									"ACFlush","ACPreRoll","ACXfer","ACAbort","ACStartAt",
									"ACXfer1","ACXfer2","ACFrmStmp2","ACTask","ACSetActFrm"};
	if (size_t(eCommand) < sizeof(sCmds))
	{	oss << sCmds[eCommand];
		switch (eCommand)
		{
			case eInitAutoCirc:
				oss << " " << ::NTV2CrosspointToString(channelSpec);
				oss	<< " frms " << DEC(lVal1) << "-" << DEC(lVal2);
				if (lVal4 > 1)				oss << " +" << DEC(lVal4) << " chls";
				if (lVal6 & AUTOCIRCULATE_WITH_FIELDS)		oss << " +FieldMode";
				if (lVal6 & AUTOCIRCULATE_WITH_HDMIAUX)		oss << " +HDMIAux";
				if (bVal2)									oss << " +RP188";
				if (bVal3)									oss << " +FBFChg";
				if (bVal4)									oss << " +FBOChg";
				if (bVal5)									oss << " +ColCorr";
				if (bVal6)									oss << " +VidProc";
				if (bVal7)									oss << " +Anc";
				if (bVal8)									oss << " +LTC";
				oss	<< " " << ::NTV2AudioSystemToString(NTV2AudioSystem(lVal3 & 0xF), true);
				if (!bVal1 && ((lVal3 & 0xF) == NTV2_MAX_NUM_AudioSystemEnums))	oss << " +AudCtrl";
				if (lVal3 & NTV2_AUDIOSYSTEM_Plus1)			oss << " +MLAud1";
				if (lVal3 & NTV2_AUDIOSYSTEM_Plus2)			oss << " +MLAud2";
				if (lVal3 & NTV2_AUDIOSYSTEM_Plus3)			oss << " +MLAud3";
				break;
			case eStartAutoCirc:
				break;
			case eStartAutoCircAtTime:
				if (lVal1 || lVal2)
					oss << " at " << xHEX0N((uint64_t(lVal1) << 32) | uint64_t(lVal2),16);
				break;
			case eStopAutoCirc:
				oss << " " << ::NTV2CrosspointToString(channelSpec);
				break;
			case eAbortAutoCirc:
				oss << " " << ::NTV2CrosspointToString(channelSpec);
				break;
			case ePauseAutoCirc:
				oss << " " << ::NTV2CrosspointToString(channelSpec);
				if (!bVal1  &&  lVal6)	oss << " at frame " << DEC(lVal6);
				if (bVal1)	oss << " +resume";
				if (bVal1  &&  bVal2)	oss << " +clearDropCount";
				break;
			case eFlushAutoCirculate:
				oss << " " << ::NTV2CrosspointToString(channelSpec);
				if (bVal1)	oss << " +clearDropCount";
				break;
			case ePrerollAutoCirculate:
				oss << " " << ::NTV2CrosspointToString(channelSpec) << " " << DEC(ULWord(lVal1)) << " frame(s)";
				break;
			case eGetAutoCirc:
				oss << " " << ::NTV2CrosspointToString(channelSpec);
				break;
			case eSetActiveFrame:
			case eGetFrameStamp:
				oss << " " << ::NTV2CrosspointToString(channelSpec) << " frm " << DEC(ULWord(lVal1));
				break;
			default:
				break;
		}
	}
	return oss;
}


NTV2_HEADER::NTV2_HEADER (const ULWord inStructureType, const ULWord inStructSizeInBytes)
	:	fHeaderTag		(NTV2_HEADER_TAG),
		fType			(inStructureType),
		fHeaderVersion	(NTV2_CURRENT_HEADER_VERSION),
		fVersion		(AUTOCIRCULATE_STRUCT_VERSION),
		fSizeInBytes	(inStructSizeInBytes),
		fPointerSize	(sizeof(int*)),
		fOperation		(0),
		fResultStatus	(0)
{
}


ostream & operator << (ostream & inOutStream, const NTV2_HEADER & inObj)
{
	return inObj.Print (inOutStream);
}


ostream & NTV2_HEADER::Print (ostream & inOutStream) const
{
	inOutStream << "[";
	if (NTV2_IS_VALID_HEADER_TAG (fHeaderTag))
		inOutStream << NTV2_4CC_AS_STRING (fHeaderTag);
	else
		inOutStream << "BAD-" << HEX0N(fHeaderTag,8);
	if (NTV2_IS_VALID_STRUCT_TYPE (fType))
		inOutStream << NTV2_4CC_AS_STRING (fType);
	else
		inOutStream << "|BAD-" << HEX0N(fType,8);
	inOutStream << " v" << fHeaderVersion << " vers=" << fVersion << " sz=" << fSizeInBytes;
	return inOutStream << "]";
}

string NTV2_HEADER::FourCCToString (const ULWord in4CC)
{
	const char * pU32 (reinterpret_cast<const char *>(&in4CC));
	ostringstream result;
	size_t badTally(0);
	result << "'";
	for (size_t charPos(0);  charPos < 4;  charPos++)
	{
		#if AJATargetBigEndian
		const char ch(pU32[charPos]);
		#else	//	little-endian:
			const char ch(pU32[3-charPos]);
		#endif
		if (ch < ' '  ||  ch > 126)
			{result << '.';  badTally++;}	//	not printable
		else
			result << ch;
	}
	result << "'";
	if (badTally)
		result << " (" << xHEX0N(in4CC,8) << ")";
	return result.str();
}


ostream & operator << (ostream & inOutStream, const NTV2_TRAILER & inObj)
{
	inOutStream << "[";
	if (NTV2_IS_VALID_TRAILER_TAG(inObj.fTrailerTag))
		inOutStream << NTV2_4CC_AS_STRING(inObj.fTrailerTag);
	else
		inOutStream << "BAD-" << HEX0N(inObj.fTrailerTag,8);
	return inOutStream << " rawVers=" << xHEX0N(inObj.fTrailerVersion,8) << " clientSDK="
				<< DEC(NTV2SDKVersionDecode_Major(inObj.fTrailerVersion))
				<< "." << DEC(NTV2SDKVersionDecode_Minor(inObj.fTrailerVersion))
				<< "." << DEC(NTV2SDKVersionDecode_Point(inObj.fTrailerVersion))
				<< "." << DEC(NTV2SDKVersionDecode_Build(inObj.fTrailerVersion)) << "]";
}


ostream & operator << (ostream & inOutStream, const NTV2Buffer & inObj)
{
	return inObj.Print (inOutStream);
}


ostream & NTV2Buffer::Print (ostream & inOutStream) const
{
	inOutStream << (IsAllocatedBySDK() ? "0X" : "0x") << HEX0N(GetRawHostPointer(),16) << "/" << DECN(GetByteCount(),10);
	return inOutStream;
}


string NTV2Buffer::AsString (UWord inDumpMaxBytes) const
{
	ostringstream oss;
	oss << xHEX0N(GetRawHostPointer(),16) << ":" << DEC(GetByteCount()) << " bytes";
	if (inDumpMaxBytes	&&	GetHostPointer())
	{
		oss << ":";
		if (inDumpMaxBytes > 256)
			inDumpMaxBytes = 256;
		if (ULWord(inDumpMaxBytes) > GetByteCount())
			inDumpMaxBytes = UWord(GetByteCount());
		const UByte *	pBytes	(reinterpret_cast<const UByte *>(GetHostPointer()));
		for (UWord ndx(0);	ndx < inDumpMaxBytes;  ndx++)
			oss << HEX0N(uint16_t(pBytes[ndx]),2);
	}
	return oss.str();
}

string NTV2Buffer::AsCode (const size_t inBytesPerWord, const std::string & inVarName, const bool inUseSTL, const bool inByteSwap) const
{
	ostringstream oss;
	if (inBytesPerWord != 1 && inBytesPerWord != 2 && inBytesPerWord != 4 && inBytesPerWord != 8) return string();
	NTV2Buffer tmp;
	if (inBytesPerWord > 1)
	{	//	Use a copy for U16s, U32s, or U64s...
		tmp = *this;
		if (!tmp)
			return string();
	}
	const string cType (inBytesPerWord == 1 ? "uint8_t" : (inBytesPerWord == 2 ? "uint16_t" : (inBytesPerWord == 4 ? "uint32_t" : "uint64_t")));
	const size_t numWords (GetByteCount() / inBytesPerWord);
	const string vecType = "std::vector<" + cType + ">";
	const string varName (inVarName.empty() ? (inUseSTL ? "tmpVector" : "tmpArray") : inVarName);
	oss << "const " << (inUseSTL ? vecType : cType) << " " << varName << (inUseSTL ? "" : "[]") << " = {" << endl;
	if (inByteSwap && inBytesPerWord > 1)
		switch (inBytesPerWord)
		{
			case 2:	tmp.ByteSwap16();  break;
			case 4:	tmp.ByteSwap32();  break;
			case 8:	tmp.ByteSwap64();  break;
		}
	for (size_t ndx(0);  ndx < numWords;  )
	{
		switch (inBytesPerWord)
		{
			case 1:	oss << xHEX0N(UWord(U8(int(ndx))),2);	break;
			case 2:	oss << xHEX0N(tmp.U16(int(ndx)),4);		break;
			case 4:	oss << xHEX0N(tmp.U32(int(ndx)),8);		break;
			case 8:	oss << xHEX0N(tmp.U64(int(ndx)),16);		break;
		}
		if (++ndx < numWords)
			oss << ",";
		if (ndx % 128 == 0)
			oss << endl;
	}
	oss << "};" << endl;
	return oss.str();
}

bool NTV2Buffer::toHexString (std::string & outStr, const size_t inLineBreakInterval) const
{
	outStr.clear();
	ostringstream oss;
	if (GetHostPointer() && GetByteCount())
		for (int ndx(0);  ndx < int(GetByteCount());  )
		{
			oss << HEX0N(uint16_t(U8(ndx++)),2);
			if (inLineBreakInterval  &&  ndx < int(GetByteCount())  &&  ((size_t(ndx) % inLineBreakInterval) == 0))
				oss << endl;
		}
	outStr = oss.str();
	return !outStr.empty();
}

static string print_address_offset (const size_t inRadix, const ULWord64 inOffset)
{
	const streamsize maxAddrWidth (sizeof(ULWord64) * 2);
	ostringstream oss;
	if (inRadix == 8)
		oss << OCT0N(inOffset,maxAddrWidth) << ": ";
	else if (inRadix == 10)
		oss << DEC0N(inOffset,maxAddrWidth) << ": ";
	else
		oss << xHEX0N(inOffset,maxAddrWidth) << ": ";
	return oss.str();
}

ostream & NTV2Buffer::Dump (ostream &		inOStream,
							const size_t	inStartOffset,
							const size_t	inByteCount,
							const size_t	inRadix,
							const size_t	inBytesPerGroup,
							const size_t	inGroupsPerRow,
							const size_t	inAddressRadix,
							const bool		inShowAscii,
							const size_t	inAddrOffset) const
{
	if (IsNULL())
		return inOStream;
	if (inRadix != 8 && inRadix != 10 && inRadix != 16 && inRadix != 2)
		return inOStream;
	if (inAddressRadix != 0 && inAddressRadix != 8 && inAddressRadix != 10 && inAddressRadix != 16)
		return inOStream;
	if (inBytesPerGroup == 0)	//	|| inGroupsPerRow == 0)
		return inOStream;

	{
		const void *	pInStartAddress		(GetHostAddress(ULWord(inStartOffset)));
		size_t			bytesRemaining		(inByteCount ? inByteCount : GetByteCount());
		size_t			bytesInThisGroup	(0);
		size_t			groupsInThisRow		(0);
		const unsigned	maxByteWidth		(inRadix == 8 ? 4 : (inRadix == 10 ? 3 : (inRadix == 2 ? 8 : 2)));
		const UByte *	pBuffer				(reinterpret_cast <const UByte *> (pInStartAddress));
		const size_t	asciiBufferSize		(inShowAscii && inGroupsPerRow ? (inBytesPerGroup * inGroupsPerRow + 1) * sizeof (UByte) : 0);	//	Size in bytes, not chars
		UByte *			pAsciiBuffer		(asciiBufferSize ? new UByte[asciiBufferSize / sizeof(UByte)] : AJA_NULL);

		if (!pInStartAddress)
			return inOStream;

		if (pAsciiBuffer)
			::memset (pAsciiBuffer, 0, asciiBufferSize);

		if (inGroupsPerRow && inAddressRadix)
			inOStream << print_address_offset (inAddressRadix, ULWord64(pBuffer) - ULWord64(pInStartAddress) + ULWord64(inAddrOffset));
		while (bytesRemaining)
		{
			if (inRadix == 2)
				inOStream << BIN08(*pBuffer);
			else if (inRadix == 8)
				inOStream << oOCT(uint16_t(*pBuffer));
			else if (inRadix == 10)
				inOStream << DEC0N(uint16_t(*pBuffer),maxByteWidth);
			else if (inRadix == 16)
				inOStream << HEX0N(uint16_t(*pBuffer),2);

			if (pAsciiBuffer)
				pAsciiBuffer[groupsInThisRow * inBytesPerGroup + bytesInThisGroup] = isprint(*pBuffer) ? *pBuffer : '.';
			pBuffer++;
			bytesRemaining--;

			bytesInThisGroup++;
			if (bytesInThisGroup >= inBytesPerGroup)
			{
				groupsInThisRow++;
				if (inGroupsPerRow && groupsInThisRow >= inGroupsPerRow)
				{
					if (pAsciiBuffer)
					{
						inOStream << " " << pAsciiBuffer;
						::memset (pAsciiBuffer, 0, asciiBufferSize);
					}
					inOStream << endl;
					if (inAddressRadix && bytesRemaining)
						inOStream << print_address_offset (inAddressRadix, reinterpret_cast <ULWord64> (pBuffer) - reinterpret_cast <ULWord64> (pInStartAddress) + ULWord64 (inAddrOffset));
					groupsInThisRow = 0;
				}	//	if time for new row
				else
					inOStream << " ";
				bytesInThisGroup = 0;
			}	//	if time for new group
		}	//	loop til no bytes remaining

		if (bytesInThisGroup && bytesInThisGroup < inBytesPerGroup && pAsciiBuffer)
		{
			groupsInThisRow++;
			inOStream << string ((inBytesPerGroup - bytesInThisGroup) * maxByteWidth + 1, ' ');
		}

		if (groupsInThisRow)
		{
			if (groupsInThisRow < inGroupsPerRow && pAsciiBuffer)
				inOStream << string (((inGroupsPerRow - groupsInThisRow) * inBytesPerGroup * maxByteWidth + (inGroupsPerRow - groupsInThisRow)), ' ');
			if (pAsciiBuffer)
				inOStream << pAsciiBuffer;
			inOStream << endl;
		}
		else if (bytesInThisGroup && bytesInThisGroup < inBytesPerGroup)
			inOStream << endl;

		if (pAsciiBuffer)
			delete [] pAsciiBuffer;
	}	//	else radix is 16, 10, 8 or 2

	return inOStream;
}	//	Dump

string & NTV2Buffer::Dump (	string &		inOutputString,
							const size_t	inStartOffset,
							const size_t	inByteCount,
							const size_t	inRadix,
							const size_t	inBytesPerGroup,
							const size_t	inGroupsPerRow,
							const size_t	inAddressRadix,
							const bool		inShowAscii,
							const size_t	inAddrOffset) const
{
	ostringstream oss;
	Dump (oss, inStartOffset, inByteCount, inRadix, inBytesPerGroup, inGroupsPerRow, inAddressRadix, inShowAscii, inAddrOffset);
	inOutputString = oss.str();
	return inOutputString;
}

NTV2Buffer & NTV2Buffer::Segment (NTV2Buffer & outPtr, const ULWord inByteOffset, const ULWord inByteCount) const
{
	outPtr.Set(AJA_NULL, 0);	//	Make invalid
	if (inByteOffset >= GetByteCount())
		return outPtr;	//	Offset past end
	if (inByteOffset+inByteCount > GetByteCount())
		return outPtr;	//	Segment too long
	outPtr.Set(GetHostAddress(inByteOffset), inByteCount);
	return outPtr;
}


bool NTV2Buffer::GetU64s (ULWord64Sequence & outUint64s, const size_t inU64Offset, const size_t inMaxSize, const bool inByteSwap) const
{
	outUint64s.clear();
	if (IsNULL())
		return false;

	size_t				maxSize (size_t(GetByteCount()) / sizeof(uint64_t));
	if (maxSize < inU64Offset)
		return false;	//	Past end
	maxSize -= inU64Offset; //	Remove starting offset

	const uint64_t *	pU64	(reinterpret_cast <const uint64_t *> (GetHostAddress(ULWord(inU64Offset * sizeof(uint64_t)))));
	if (!pU64)
		return false;	//	Past end

	if (inMaxSize  &&  inMaxSize < maxSize)
		maxSize = inMaxSize;

	try
	{
		outUint64s.reserve(maxSize);
		for (size_t ndx(0);	 ndx < maxSize;	 ndx++)
		{
			const uint64_t	u64 (*pU64++);
			outUint64s.push_back(inByteSwap ? NTV2EndianSwap64(u64) : u64);
		}
	}
	catch (...)
	{
		outUint64s.clear();
		outUint64s.reserve(0);
		return false;
	}
	return true;
}


bool NTV2Buffer::GetU32s (ULWordSequence & outUint32s, const size_t inU32Offset, const size_t inMaxSize, const bool inByteSwap) const
{
	outUint32s.clear();
	if (IsNULL())
		return false;

	size_t	maxNumU32s	(size_t(GetByteCount()) / sizeof(uint32_t));
	if (maxNumU32s < inU32Offset)
		return false;	//	Past end
	maxNumU32s -= inU32Offset;	//	Remove starting offset

	const uint32_t * pU32 (reinterpret_cast<const uint32_t*>(GetHostAddress(ULWord(inU32Offset * sizeof(uint32_t)))));
	if (!pU32)
		return false;	//	Past end

	if (inMaxSize  &&  inMaxSize < maxNumU32s)
		maxNumU32s = inMaxSize;

	try
	{
		outUint32s.reserve(maxNumU32s);
		for (size_t ndx(0);	 ndx < maxNumU32s;	ndx++)
		{
			const uint32_t	u32 (*pU32++);
			outUint32s.push_back(inByteSwap ? NTV2EndianSwap32(u32) : u32);
		}
	}
	catch (...)
	{
		outUint32s.clear();
		outUint32s.reserve(0);
		return false;
	}
	return true;
}


bool NTV2Buffer::GetU16s (UWordSequence & outUint16s, const size_t inU16Offset, const size_t inMaxSize, const bool inByteSwap) const
{
	outUint16s.clear();
	if (IsNULL())
		return false;

	size_t				maxSize (size_t(GetByteCount()) / sizeof(uint16_t));
	if (maxSize < inU16Offset)
		return false;	//	Past end
	maxSize -= inU16Offset; //	Remove starting offset

	const uint16_t *	pU16	(reinterpret_cast <const uint16_t *> (GetHostAddress(ULWord(inU16Offset * sizeof(uint16_t)))));
	if (!pU16)
		return false;	//	Past end

	if (inMaxSize  &&  inMaxSize < maxSize)
		maxSize = inMaxSize;

	try
	{
		outUint16s.reserve(maxSize);
		for (size_t ndx(0);	 ndx < maxSize;	 ndx++)
		{
			const uint16_t	u16 (*pU16++);
			outUint16s.push_back(inByteSwap ? NTV2EndianSwap16(u16) : u16);
		}
	}
	catch (...)
	{
		outUint16s.clear();
		outUint16s.reserve(0);
		return false;
	}
	return true;
}


bool NTV2Buffer::GetU8s (UByteSequence & outUint8s, const size_t inU8Offset, const size_t inMaxSize) const
{
	outUint8s.clear();
	if (IsNULL())
		return false;

	size_t	maxSize (GetByteCount());
	if (maxSize < inU8Offset)
		return false;	//	Past end
	maxSize -= inU8Offset;	//	Remove starting offset

	const uint8_t * pU8 (reinterpret_cast <const uint8_t *> (GetHostAddress(ULWord(inU8Offset))));
	if (!pU8)
		return false;	//	Past end

	if (inMaxSize  &&  inMaxSize < maxSize)
		maxSize = inMaxSize;

	try
	{
		outUint8s.reserve(maxSize);
		for (size_t ndx(0);	 ndx < maxSize;	 ndx++)
			outUint8s.push_back(*pU8++);
	}
	catch (...)
	{
		outUint8s.clear();
		outUint8s.reserve(0);
		return false;
	}
	return true;
}

bool NTV2Buffer::AppendU8s (UByteSequence & outU8s) const
{
	const uint8_t * pU8 (reinterpret_cast<const uint8_t*> (GetHostPointer()));
	if (!pU8)
		return false;	//	Past end
	const size_t maxSize (GetByteCount());
	try
	{
		outU8s.reserve(outU8s.size() + maxSize);
		outU8s.insert(outU8s.end(),pU8, pU8 + maxSize);
	}
	catch (...)
	{
		return false;
	}
	return true;
}


bool NTV2Buffer::GetString (std::string & outString, const size_t inU8Offset, const size_t inMaxSize) const
{
	outString.clear();
	if (IsNULL())
		return false;

	size_t maxSize(GetByteCount());
	if (maxSize < inU8Offset)
		return false;		//	Past end
	maxSize -= inU8Offset;	//	Remove starting offset

	const uint8_t * pU8 (reinterpret_cast <const uint8_t *> (GetHostAddress(ULWord(inU8Offset))));
	if (!pU8)
		return false;	//	Past end

	if (inMaxSize  &&  inMaxSize < maxSize)
		maxSize = inMaxSize;

	try
	{
		outString.reserve(maxSize);
		for (size_t ndx(0);	 ndx < maxSize;	 ndx++)
		{
			const char c = *pU8++;
			if (c)
				outString += c;
			else
				break;
		}
	}
	catch (...)
	{
		outString.clear();
		outString.reserve(0);
		return false;
	}
	return true;
}


bool NTV2Buffer::PutU64s (const ULWord64Sequence & inU64s, const size_t inU64Offset, const bool inByteSwap)
{
	if (IsNULL())
		return false;	//	No buffer or space
	if (inU64s.empty())
		return true;	//	Nothing to copy

	size_t		maxU64s (GetByteCount() / sizeof(uint64_t));
	uint64_t *	pU64	(reinterpret_cast<uint64_t*>(GetHostAddress(ULWord(inU64Offset * sizeof(uint64_t)))));
	if (!pU64)
		return false;	//	Start offset is past end
	if (maxU64s > inU64Offset)
		maxU64s -= inU64Offset; //	Don't go past end
	if (maxU64s > inU64s.size())
		maxU64s = inU64s.size();	//	Truncate incoming vector to not go past my end
	if (inU64s.size() > maxU64s)
		return false;	//	Will write past end

	for (unsigned ndx(0);  ndx < maxU64s;  ndx++)
#if defined(_DEBUG)
		*pU64++ = inByteSwap ? NTV2EndianSwap64(inU64s.at(ndx)) : inU64s.at(ndx);
#else
		*pU64++ = inByteSwap ? NTV2EndianSwap64(inU64s[ndx]) : inU64s[ndx];
#endif
	return true;
}


bool NTV2Buffer::PutU32s (const ULWordSequence & inU32s, const size_t inU32Offset, const bool inByteSwap)
{
	if (IsNULL())
		return false;	//	No buffer or space
	if (inU32s.empty())
		return true;	//	Nothing to copy

	size_t		maxU32s (GetByteCount() / sizeof(uint32_t));
	uint32_t *	pU32	(reinterpret_cast<uint32_t*>(GetHostAddress(ULWord(inU32Offset * sizeof(uint32_t)))));
	if (!pU32)
		return false;	//	Start offset is past end
	if (maxU32s > inU32Offset)
		maxU32s -= inU32Offset; //	Don't go past end
	if (maxU32s > inU32s.size())
		maxU32s = inU32s.size();	//	Truncate incoming vector to not go past my end
	if (inU32s.size() > maxU32s)
		return false;	//	Will write past end

	for (unsigned ndx(0);  ndx < maxU32s;  ndx++)
#if defined(_DEBUG)
		*pU32++ = inByteSwap ? NTV2EndianSwap32(inU32s.at(ndx)) : inU32s.at(ndx);
#else
		*pU32++ = inByteSwap ? NTV2EndianSwap32(inU32s[ndx]) : inU32s[ndx];
#endif
	return true;
}


bool NTV2Buffer::PutU16s (const UWordSequence & inU16s, const size_t inU16Offset, const bool inByteSwap)
{
	if (IsNULL())
		return false;	//	No buffer or space
	if (inU16s.empty())
		return true;	//	Nothing to copy

	size_t		maxU16s (GetByteCount() / sizeof(uint16_t));
	uint16_t *	pU16	(reinterpret_cast<uint16_t*>(GetHostAddress(ULWord(inU16Offset * sizeof(uint16_t)))));
	if (!pU16)
		return false;	//	Start offset is past end
	if (maxU16s > inU16Offset)
		maxU16s -= inU16Offset; //	Don't go past end
	if (maxU16s > inU16s.size())
		maxU16s = inU16s.size();	//	Truncate incoming vector to not go past my end
	if (inU16s.size() > maxU16s)
		return false;	//	Will write past end

	for (unsigned ndx(0);  ndx < maxU16s;  ndx++)
#if defined(_DEBUG)
		*pU16++ = inByteSwap ? NTV2EndianSwap16(inU16s.at(ndx)) : inU16s.at(ndx);
#else
		*pU16++ = inByteSwap ? NTV2EndianSwap16(inU16s[ndx]) : inU16s[ndx];
#endif
	return true;
}


bool NTV2Buffer::PutU8s (const UByteSequence & inU8s, const size_t inU8Offset)
{
	if (IsNULL())
		return false;	//	No buffer or space
	if (inU8s.empty())
		return true;	//	Nothing to copy

	size_t		maxU8s	(GetByteCount());
	uint8_t *	pU8		(reinterpret_cast<uint8_t*>(GetHostAddress(ULWord(inU8Offset))));
	if (!pU8)
		return false;	//	Start offset is past end
	if (maxU8s > inU8Offset)
		maxU8s -= inU8Offset;	//	Don't go past end
	if (maxU8s > inU8s.size())
		maxU8s = inU8s.size();	//	Truncate incoming vector to not go past end
	if (inU8s.size() > maxU8s)
		return false;	//	Will write past end
#if 1
	::memcpy(pU8, &inU8s[0], maxU8s);
#else
	for (unsigned ndx(0);  ndx < maxU8s;  ndx++)
	#if defined(_DEBUG)
		*pU8++ = inU8s.at(ndx);
	#else
		*pU8++ = inU8s[ndx];
	#endif
#endif
	return true;
}


ostream & operator << (ostream & inOutStream, const NTV2_RP188 & inObj)
{
	if (inObj.IsValid ())
		return inOutStream << "{Dx" << HEX0N(inObj.fDBB,8) << "|Lx" << HEX0N(inObj.fLo,8) << "|Hx" << HEX0N(inObj.fHi,8) << "}";
	else
		return inOutStream << "{invalid}";
}


NTV2TimeCodeList & operator << (NTV2TimeCodeList & inOutList, const NTV2_RP188 & inRP188)
{
	inOutList.push_back (inRP188);
	return inOutList;
}


ostream & operator << (ostream & inOutStream, const NTV2TimeCodeList & inObj)
{
	inOutStream << inObj.size () << ":[";
	for (NTV2TimeCodeListConstIter iter (inObj.begin ());  iter != inObj.end ();  )
	{
		inOutStream << *iter;
		if (++iter != inObj.end ())
			inOutStream << ", ";
	}
	return inOutStream << "]";
}


ostream & operator << (std::ostream & inOutStream, const NTV2TimeCodes & inObj)
{
	inOutStream << inObj.size () << ":[";
	for (NTV2TimeCodesConstIter iter (inObj.begin ());	iter != inObj.end ();  )
	{
		inOutStream << ::NTV2TCIndexToString (iter->first,true) << "=" << iter->second;
		if (++iter != inObj.end ())
			inOutStream << ", ";
	}
	return inOutStream << "]";
}


ostream & operator << (std::ostream & inOutStream, const NTV2TCIndexes & inObj)
{
	for (NTV2TCIndexesConstIter iter (inObj.begin ());	iter != inObj.end ();  )
	{
		inOutStream << ::NTV2TCIndexToString (*iter);
		if (++iter != inObj.end ())
			inOutStream << ", ";
	}
	return inOutStream;
}


NTV2TCIndexes & operator += (NTV2TCIndexes & inOutSet, const NTV2TCIndexes & inSet)
{
	for (NTV2TCIndexesConstIter iter (inSet.begin ());	iter != inSet.end ();  ++iter)
		inOutSet.insert (*iter);
	return inOutSet;
}


ostream & operator << (ostream & inOutStream, const FRAME_STAMP & inObj)
{
	return inOutStream	<< inObj.acHeader
						<< " frmTime="			<< inObj.acFrameTime
						<< " reqFrm="			<< inObj.acRequestedFrame
						<< " audClkTS="			<< inObj.acAudioClockTimeStamp
						<< " audExpAdr="		<< hex << inObj.acAudioExpectedAddress << dec
						<< " audInStrtAdr="		<< hex << inObj.acAudioInStartAddress << dec
						<< " audInStopAdr="		<< hex << inObj.acAudioInStopAddress << dec
						<< " audOutStrtAdr="	<< hex << inObj.acAudioOutStartAddress << dec
						<< " audOutStopAdr="	<< hex << inObj.acAudioOutStopAddress << dec
						<< " totBytes="			<< inObj.acTotalBytesTransferred
						<< " strtSamp="			<< inObj.acStartSample
						<< " curTime="			<< inObj.acCurrentTime
						<< " curFrm="			<< inObj.acCurrentFrame
						<< " curFrmTime="		<< inObj.acCurrentFrameTime
						<< " audClkCurTime="	<< inObj.acAudioClockCurrentTime
						<< " curAudExpAdr="		<< hex << inObj.acCurrentAudioExpectedAddress << dec
						<< " curAudStrtAdr="	<< hex << inObj.acCurrentAudioStartAddress << dec
						<< " curFldCnt="		<< inObj.acCurrentFieldCount
						<< " curLnCnt="			<< inObj.acCurrentLineCount
						<< " curReps="			<< inObj.acCurrentReps
						<< " curUsrCookie="		<< hex << inObj.acCurrentUserCookie << dec
						<< " acFrame="			<< inObj.acFrame
						<< " acRP188="			<< inObj.acRP188	//	deprecated
						<< " "					<< inObj.acTrailer;
}


ostream & operator << (ostream & inOutStream, const NTV2SegmentedDMAInfo & inObj)
{
	if (inObj.acNumSegments > 1)
		inOutStream << "segs=" << inObj.acNumSegments << " numActBPR=" << inObj.acNumActiveBytesPerRow
					<< " segHostPitc=" << inObj.acSegmentHostPitch << " segDevPitc=" << inObj.acSegmentDevicePitch;
	else
		inOutStream << "n/a";
	return inOutStream;
}


ostream & operator << (ostream & inOutStream, const AUTOCIRCULATE_TRANSFER & inObj)
{
	#if defined (_DEBUG)
		NTV2_ASSERT (inObj.NTV2_IS_STRUCT_VALID ());
	#endif
	string	str (::NTV2FrameBufferFormatToString (inObj.acFrameBufferFormat, true));
	while (str.find (' ') != string::npos)
		str.erase (str.find (' '), 1);
	inOutStream << inObj.acHeader << " vid=" << inObj.acVideoBuffer
				<< " aud=" << inObj.acAudioBuffer
				<< " ancF1=" << inObj.acANCBuffer
				<< " ancF2=" << inObj.acANCField2Buffer
				<< " outTC(" << inObj.acOutputTimeCodes << ")"
				<< " cookie=" << inObj.acInUserCookie
				<< " vidDMAoff=" << inObj.acInVideoDMAOffset
				<< " segDMA=" << inObj.acInSegmentedDMAInfo
				<< " colcor=" << inObj.acColorCorrection
				<< " fbf=" << str
				<< " fbo=" << (inObj.acFrameBufferOrientation == NTV2_FRAMEBUFFER_ORIENTATION_BOTTOMUP ? "flip" : "norm")
				<< " vidProc=" << inObj.acVidProcInfo
				<< " quartsz=" << inObj.acVideoQuarterSizeExpand
				<< " p2p=" << inObj.acPeerToPeerFlags
				<< " repCnt=" << inObj.acFrameRepeatCount
				<< " desFrm=" << inObj.acDesiredFrame
				<< " rp188=" << inObj.acRP188		//	deprecated
				<< " xpt=" << inObj.acCrosspoint
				<< " status{" << inObj.acTransferStatus << "}"
				<< " " << inObj.acTrailer;
	return inOutStream;
}


ostream & operator << (ostream & inOutStream, const AUTOCIRCULATE_TRANSFER_STATUS & inObj)
{
	inOutStream << inObj.acHeader << " state=" << ::NTV2AutoCirculateStateToString (inObj.acState)
				<< " xferFrm=" << inObj.acTransferFrame
				<< " bufLvl=" << inObj.acBufferLevel
				<< " frms=" << inObj.acFramesProcessed
				<< " drops=" << inObj.acFramesDropped
				<< " " << inObj.acFrameStamp
				<< " audXfrSz=" << inObj.acAudioTransferSize
				<< " audStrtSamp=" << inObj.acAudioStartSample
				<< " ancF1Siz=" << inObj.acAncTransferSize
				<< " ancF2Siz=" << inObj.acAncField2TransferSize
				<< " " << inObj.acTrailer;
	return inOutStream;
}


ostream & operator << (ostream & inOutStream, const NTV2RegisterValueMap & inObj)
{
	NTV2RegValueMapConstIter	iter	(inObj.begin ());
	inOutStream << "RegValues:" << inObj.size () << "[";
	while (iter != inObj.end ())
	{
		const NTV2RegisterNumber	registerNumber	(static_cast <NTV2RegisterNumber> (iter->first));
		const ULWord				registerValue	(iter->second);
		inOutStream << ::NTV2RegisterNumberToString (registerNumber) << "=0x" << hex << registerValue << dec;
		if (++iter != inObj.end ())
			inOutStream << ",";
	}
	return inOutStream << "]";
}


ostream & operator << (ostream & inOutStream, const AutoCircVidProcInfo & inObj)
{
	return inOutStream	<< "{mode=" << ::AutoCircVidProcModeToString (inObj.mode, true)
						<< ", FGvid=" << ::NTV2CrosspointToString (inObj.foregroundVideoCrosspoint)
						<< ", BGvid=" << ::NTV2CrosspointToString (inObj.backgroundVideoCrosspoint)
						<< ", FGkey=" << ::NTV2CrosspointToString (inObj.foregroundKeyCrosspoint)
						<< ", BGkey=" << ::NTV2CrosspointToString (inObj.backgroundKeyCrosspoint)
						<< ", transCoeff=" << inObj.transitionCoefficient
						<< ", transSoftn=" << inObj.transitionSoftness << "}";
}


ostream & operator << (ostream & inOutStream, const NTV2ColorCorrectionData & inObj)
{
	return inOutStream	<< "{ccMode=" << ::NTV2ColorCorrectionModeToString (inObj.ccMode)
						<< ", ccSatVal=" << inObj.ccSaturationValue
						<< ", ccTables=" << inObj.ccLookupTables << "}";
}


NTV2VideoFormatSet & operator += (NTV2VideoFormatSet & inOutSet, const NTV2VideoFormatSet inSet)
{
	for (NTV2VideoFormatSetConstIter iter(inSet.begin());  iter != inSet.end();  ++iter)
		if (inOutSet.find(*iter) == inOutSet.end())
			inOutSet.insert(*iter);
	return inOutSet;
}


//	Implementation of NTV2VideoFormatSet's ostream writer...
ostream & operator << (ostream & inOStream, const NTV2VideoFormatSet & inFormats)
{
	NTV2VideoFormatSet::const_iterator	iter	(inFormats.begin ());

	inOStream	<< inFormats.size ()
				<< (inFormats.size () == 1 ? " video format:  " : " video format(s):  ");

	while (iter != inFormats.end ())
	{
		inOStream << std::string (::NTV2VideoFormatToString (*iter));
		inOStream << (++iter == inFormats.end () ? "" : ", ");
	}

	return inOStream;

}	//	operator <<


//	Implementation of NTV2FrameBufferFormatSet's ostream writer...
ostream & operator << (ostream & inOStream, const NTV2PixelFormats & inFormats)
{
	NTV2PixelFormatsConstIter iter(inFormats.begin());
	inOStream	<< inFormats.size()
				<< (inFormats.size() == 1 ? " pixel format:  " : " pixel formats:	");

	while (iter != inFormats.end())
	{
		inOStream << ::NTV2FrameBufferFormatToString(*iter);
		inOStream << (++iter == inFormats.end()  ?	 ""	 :	", ");
	}
	return inOStream;

}	//	operator <<


NTV2PixelFormats & operator += (NTV2PixelFormats & inOutSet, const NTV2PixelFormats inFBFs)
{
	for (NTV2PixelFormatsConstIter iter(inFBFs.begin());  iter != inFBFs.end();  ++iter)
		inOutSet.insert(*iter);
	return inOutSet;
}


//	Implementation of NTV2StandardSet's ostream writer...
ostream & operator << (ostream & inOStream, const NTV2StandardSet & inStandards)
{
	NTV2StandardSetConstIter	iter	(inStandards.begin ());

	inOStream	<< inStandards.size ()
				<< (inStandards.size () == 1 ? " standard:	" : " standards:  ");

	while (iter != inStandards.end ())
	{
		inOStream << ::NTV2StandardToString(*iter);
		inOStream << (++iter == inStandards.end ()	?  ""  :  ", ");
	}

	return inOStream;
}


NTV2StandardSet & operator += (NTV2StandardSet & inOutSet, const NTV2StandardSet inSet)
{
	for (NTV2StandardSetConstIter iter(inSet.begin ());	 iter != inSet.end();  ++iter)
		inOutSet.insert(*iter);
	return inOutSet;
}


//	Implementation of NTV2GeometrySet's ostream writer...
ostream & operator << (ostream & inOStream, const NTV2GeometrySet & inGeometries)
{
	NTV2GeometrySetConstIter	iter	(inGeometries.begin ());
	inOStream	<< inGeometries.size ()
				<< (inGeometries.size () == 1 ? " geometry:	 " : " geometries:	");
	while (iter != inGeometries.end ())
	{
		inOStream << ::NTV2FrameGeometryToString(*iter);
		inOStream << (++iter == inGeometries.end ()	 ?	""	:  ", ");
	}
	return inOStream;
}


NTV2GeometrySet & operator += (NTV2GeometrySet & inOutSet, const NTV2GeometrySet inSet)
{
	for (NTV2GeometrySetConstIter iter(inSet.begin ());	 iter != inSet.end();  ++iter)
		inOutSet.insert(*iter);
	return inOutSet;
}


//	Implementation of NTV2FrameBufferFormatSet's ostream writer...
ostream & operator << (ostream & inOStream, const NTV2InputSourceSet & inSet)
{
	NTV2InputSourceSetConstIter iter(inSet.begin());
	inOStream	<< inSet.size()
				<< (inSet.size() == 1 ? " input:  " : " inputs:	 ");
	while (iter != inSet.end())
	{
		inOStream << ::NTV2InputSourceToString (*iter);
		inOStream << (++iter == inSet.end()	 ?	""	:  ", ");
	}
	return inOStream;
}	//	operator <<


NTV2InputSourceSet & operator += (NTV2InputSourceSet & inOutSet, const NTV2InputSourceSet & inSet)
{
	for (NTV2InputSourceSetConstIter iter (inSet.begin ());	 iter != inSet.end ();	++iter)
		inOutSet.insert (*iter);
	return inOutSet;
}


ostream & operator << (ostream & inOStream, const NTV2OutputDestinations & inSet)
{
	NTV2OutputDestinationsConstIter iter(inSet.begin());
	inOStream	<< inSet.size()
				<< (inSet.size() == 1 ? " output:  " : " outputs:  ");
	while (iter != inSet.end())
	{
		inOStream << ::NTV2OutputDestinationToString(*iter);
		inOStream << (++iter == inSet.end()	 ?	""	:  ", ");
	}
	return inOStream;
}


NTV2OutputDestinations & operator += (NTV2OutputDestinations & inOutSet, const NTV2OutputDestinations & inSet)
{
	for (NTV2OutputDestinationsConstIter iter(inSet.begin());  iter != inSet.end();	 ++iter)
		inOutSet.insert(*iter);
	return inOutSet;
}

bool NTV2GetSupportedPixelFormats (NTV2PixelFormats & outFormats)
{
	outFormats.clear();
	const NTV2DeviceIDSet devIDs (::NTV2GetSupportedDevices());
	for (NTV2DeviceIDSetConstIter it(devIDs.begin());  it != devIDs.end();  ++it)
	{
		NTV2PixelFormats fmts;
		::NTV2DeviceGetSupportedPixelFormats(*it, fmts);
		for (NTV2PixelFormatsConstIter fit(fmts.begin());  fit != fmts.end();  ++fit)
			if (outFormats.find(*fit) == outFormats.end())
				outFormats.insert(*fit);
	}
	return true;
}

bool NTV2GetUnsupportedPixelFormats (NTV2PixelFormats & outFormats)
{
	NTV2PixelFormats usedFormats;
	::NTV2GetSupportedPixelFormats(usedFormats);
	for (NTV2PixelFormat pf(NTV2_FBF_FIRST);  pf < NTV2_FBF_LAST;  pf = NTV2PixelFormat(pf+1))
		if (usedFormats.find(pf) == usedFormats.end())	//	if unused
			outFormats.insert(pf);
	return true;
}

bool NTV2GetSupportedStandards (NTV2StandardSet & outStandards)
{
	outStandards.clear();
	const NTV2DeviceIDSet devIDs (::NTV2GetSupportedDevices());
	for (NTV2DeviceIDSetConstIter it(devIDs.begin());  it != devIDs.end();  ++it)
	{
		NTV2StandardSet stds;
		::NTV2DeviceGetSupportedStandards(*it, stds);
		for (NTV2StandardSetConstIter sit(stds.begin());  sit != stds.end();  ++sit)
			if (outStandards.find(*sit) == outStandards.end())
				outStandards.insert(*sit);
	}
	return true;
}

bool NTV2GetUnsupportedStandards (NTV2StandardSet & outStandards)
{
	NTV2StandardSet usedStandards;
	::NTV2GetSupportedStandards(usedStandards);
	for (NTV2Standard st(NTV2_STANDARD_1080);  st < NTV2_NUM_STANDARDS;  st = NTV2Standard(st+1))
		if (usedStandards.find(st) == usedStandards.end())	//	if unused
			outStandards.insert(st);
	return true;
}


//	This needs to be moved into a C++ compatible "device features" module:
bool NTV2DeviceGetSupportedVideoFormats (const NTV2DeviceID inDeviceID, NTV2VideoFormatSet & outFormats)
{
	bool isOkay(true);
	outFormats.clear();

    for (NTV2VideoFormat vf(NTV2_FORMAT_UNKNOWN);  vf < NTV2_MAX_NUM_VIDEO_FORMATS;  vf = NTV2VideoFormat(vf+1))
	{
		if (inDeviceID != DEVICE_ID_INVALID  &&  !::NTV2DeviceCanDoVideoFormat(inDeviceID, vf))
			continue;	//	Valid devID specified and VF not supported on that device
		if (inDeviceID == DEVICE_ID_INVALID  &&  !NTV2_IS_VALID_VIDEO_FORMAT(vf))
			continue;	//	Invalid devID specified and invalid VF
		try
		{
			outFormats.insert(vf);
		}
		catch (const std::bad_alloc &)
		{
			isOkay = false;
			outFormats.clear();
			break;
		}
	}	//	for each video format

	NTV2_ASSERT ((isOkay && !outFormats.empty())  ||  (!isOkay && outFormats.empty()));
	return isOkay;

}	//	NTV2DeviceGetSupportedVideoFormats

//	This needs to be moved into a C++ compatible "device features" module:
bool NTV2DeviceGetSupportedPixelFormats (const NTV2DeviceID inDeviceID, NTV2PixelFormats & outFormats)
{
	bool isOkay(true);
	outFormats.clear();

	for (NTV2PixelFormat pixelFormat(NTV2_FBF_FIRST);  pixelFormat < NTV2_FBF_LAST;  pixelFormat = NTV2PixelFormat(pixelFormat+1))
		if (::NTV2DeviceCanDoFrameBufferFormat (inDeviceID, pixelFormat))
			try
			{
				outFormats.insert(pixelFormat);
			}
			catch (const std::bad_alloc &)
			{
				isOkay = false;
				outFormats.clear();
				break;
			}

	NTV2_ASSERT ((isOkay && !outFormats.empty() ) || (!isOkay && outFormats.empty() ));
	return isOkay;

}	//	NTV2DeviceGetSupportedPixelFormats

//	This needs to be moved into a C++ compatible "device features" module:
bool NTV2DeviceGetSupportedStandards (const NTV2DeviceID inDeviceID, NTV2StandardSet & outStandards)
{
	NTV2VideoFormatSet	videoFormats;
	outStandards.clear();
	if (!::NTV2DeviceGetSupportedVideoFormats(inDeviceID, videoFormats))
		return false;
	for (NTV2VideoFormatSetConstIter it(videoFormats.begin());  it != videoFormats.end();  ++it)
	{
		const NTV2Standard	std	(::GetNTV2StandardFromVideoFormat(*it));
		if (NTV2_IS_VALID_STANDARD(std)  &&  outStandards.find(std) == outStandards.end())
			outStandards.insert(std);
	}
	return true;
}

//	This needs to be moved into a C++ compatible "device features" module:
bool NTV2DeviceGetSupportedGeometries (const NTV2DeviceID inDeviceID, NTV2GeometrySet & outGeometries)
{
	NTV2VideoFormatSet	videoFormats;
	outGeometries.clear();
	if (!::NTV2DeviceGetSupportedVideoFormats(inDeviceID, videoFormats))
		return false;
	for (NTV2VideoFormatSetConstIter it(videoFormats.begin());  it != videoFormats.end();  ++it)
	{
		const NTV2FrameGeometry	fg	(::GetNTV2FrameGeometryFromVideoFormat(*it));
		if (NTV2_IS_VALID_NTV2FrameGeometry(fg))
			outGeometries += ::GetRelatedGeometries(fg);
	}
	return true;
}

bool NTV2DeviceGetSupportedInputSources (const NTV2DeviceID inDeviceID, NTV2InputSourceSet & outInputSources, const NTV2IOKinds inKinds)
{
	outInputSources.clear();
	if (!NTV2_IS_VALID_IOKINDS(inKinds))
		return false;
	for (NTV2InputSource src(NTV2_INPUTSOURCE_ANALOG1);  src < NTV2_NUM_INPUTSOURCES;  src = NTV2InputSource(src+1))
	{	const bool ok (inDeviceID == DEVICE_ID_INVALID  ?  true  :  ::NTV2DeviceCanDoInputSource(inDeviceID, src));
		if (ok)
			if (	(NTV2_INPUT_SOURCE_IS_SDI(src)		&&  (inKinds & NTV2_IOKINDS_SDI))
				||	(NTV2_INPUT_SOURCE_IS_HDMI(src)		&&  (inKinds & NTV2_IOKINDS_HDMI))
				||	(NTV2_INPUT_SOURCE_IS_ANALOG(src)	&&  (inKinds & NTV2_IOKINDS_ANALOG))	)
					outInputSources.insert(src);
	}
	return true;
}

bool NTV2DeviceGetSupportedOutputDests (const NTV2DeviceID inDeviceID, NTV2OutputDestinations & outOutputDests, const NTV2IOKinds inKinds)
{
	static const NTV2OutputDest sDsts[] = {	NTV2_OUTPUTDESTINATION_ANALOG1,	NTV2_OUTPUTDESTINATION_HDMI1,
											NTV2_OUTPUTDESTINATION_SDI1,	NTV2_OUTPUTDESTINATION_SDI2,	NTV2_OUTPUTDESTINATION_SDI3,	NTV2_OUTPUTDESTINATION_SDI4,
											NTV2_OUTPUTDESTINATION_SDI5,	NTV2_OUTPUTDESTINATION_SDI6,	NTV2_OUTPUTDESTINATION_SDI7,	NTV2_OUTPUTDESTINATION_SDI8};
	outOutputDests.clear();
	if (!NTV2_IS_VALID_IOKINDS(inKinds))
		return false;
	for (size_t ndx(0);  ndx < 10;  ndx++)
	{	const NTV2OutputDest dst(sDsts[ndx]);
		const bool ok (inDeviceID == DEVICE_ID_INVALID  ?  true  :  ::NTV2DeviceCanDoOutputDestination(inDeviceID, dst));
		if (ok)
			if (	(NTV2_OUTPUT_DEST_IS_SDI(dst)		&&  (inKinds & NTV2_IOKINDS_SDI))
				||	(NTV2_OUTPUT_DEST_IS_HDMI(dst)		&&  (inKinds & NTV2_IOKINDS_HDMI))
				||	(NTV2_OUTPUT_DEST_IS_ANALOG(dst)	&&  (inKinds & NTV2_IOKINDS_ANALOG))	)
					outOutputDests.insert(dst);
	}
	return true;
}

ostream & operator << (ostream & oss, const NTV2FrameRateSet & inSet)
{
	NTV2FrameRateSetConstIter it(inSet.begin());
	oss	<< inSet.size()
		<< (inSet.size() == 1 ? " rate:  " : " rates:  ");
	while (it != inSet.end())
	{
		oss << ::NTV2FrameRateToString(*it);
		oss << (++it == inSet.end()	 ?	""	:  ", ");
	}
	return oss;
}

NTV2FrameRateSet & operator += (NTV2FrameRateSet & inOutSet, const NTV2FrameRateSet & inSet)
{
	for (NTV2FrameRateSetConstIter it(inSet.begin());  it != inSet.end();  ++it)
		if (inOutSet.find(*it) == inOutSet.end())
			inOutSet.insert(*it);
	return inOutSet;
}

bool NTV2DeviceGetSupportedFrameRates (const NTV2DeviceID inDeviceID, NTV2FrameRateSet & outRates)
{
	outRates.clear();
	NTV2VideoFormatSet vfs;
	if (!::NTV2DeviceGetSupportedVideoFormats (inDeviceID, vfs))
		return false;
	for (NTV2VideoFormatSetConstIter it(vfs.begin());  it != vfs.end();  ++it)
	{	const NTV2FrameRate fr (::GetNTV2FrameRateFromVideoFormat(*it));
		if (NTV2_IS_VALID_NTV2FrameRate(fr))
			outRates.insert(fr);
	}
	return true;
}


ostream & operator << (ostream & inOutStrm, const NTV2SegmentedXferInfo & inRun)
{
	return inRun.Print(inOutStrm);
}


//	Implementation of NTV2AutoCirculateStateToString...
string NTV2AutoCirculateStateToString (const NTV2AutoCirculateState inState)
{
	static const char * sStateStrings []	= { "Disabled", "Initializing", "Starting", "Paused", "Stopping", "Running", "StartingAtTime", AJA_NULL};
	if (inState >= NTV2_AUTOCIRCULATE_DISABLED && inState <= NTV2_AUTOCIRCULATE_STARTING_AT_TIME)
		return string (sStateStrings [inState]);
	else
		return "<invalid>";
}



NTV2_TRAILER::NTV2_TRAILER ()
	:	fTrailerVersion		(NTV2SDKVersionEncode(AJA_NTV2_SDK_VERSION_MAJOR, AJA_NTV2_SDK_VERSION_MINOR, AJA_NTV2_SDK_VERSION_POINT, AJA_NTV2_SDK_BUILD_NUMBER)),
		fTrailerTag			(NTV2_TRAILER_TAG)
{
}


NTV2FrameSize::operator NTV2FrameGeometry() const		//	Cast to NTV2FrameGeometry
{
	return isValid() ? ::GetGeometryFromFrameDimensions(*this) : NTV2_FG_INVALID;
}

ULWord NTV2FrameSize::FGWidth (const NTV2FrameGeometry fg)
{	//	Requires C++11:
	static const FGSizesMap sFGWdths = {	{NTV2_FG_720x486,	720},
											{NTV2_FG_720x508,	720},
											{NTV2_FG_720x514,	720},
											{NTV2_FG_720x576,	720},
											{NTV2_FG_720x598,	720},
											{NTV2_FG_720x612,	720},
											{NTV2_FG_1280x720,	1280},
											{NTV2_FG_1280x740,	1280},
											{NTV2_FG_1920x1080,	1920},
											{NTV2_FG_1920x1114,	1920},
											{NTV2_FG_1920x1112,	1920},
											{NTV2_FG_2048x1080,	2048},
											{NTV2_FG_2048x1112,	2048},
											{NTV2_FG_2048x1114,	2048},
											{NTV2_FG_2048x1556,	2048},
											{NTV2_FG_2048x1588,	2048},
											{NTV2_FG_3840x2160,	3840},
											{NTV2_FG_4096x2160,	4096},
											{NTV2_FG_7680x4320,	7680},
											{NTV2_FG_8192x4320,	8192}	};
	FGSizesMapCI it (sFGWdths.find(fg));
	return it != sFGWdths.end() ? it->second : 0;
}

ULWord NTV2FrameSize::FGHeight (const NTV2FrameGeometry fg)
{	//	Requires C++11:
	static const FGSizesMap sFGHghts = {	{NTV2_FG_720x486,	486},
											{NTV2_FG_720x508,	508},
											{NTV2_FG_720x514,	514},
											{NTV2_FG_720x576,	576},
											{NTV2_FG_720x598,	598},
											{NTV2_FG_720x612,	612},
											{NTV2_FG_1280x720,	720},
											{NTV2_FG_1280x740,	740},
											{NTV2_FG_2048x1556,	1556},
											{NTV2_FG_2048x1588,	1588},
											{NTV2_FG_1920x1080,	1080},
											{NTV2_FG_2048x1080,	1080},
											{NTV2_FG_1920x1114,	1114},
											{NTV2_FG_2048x1114,	1114},
											{NTV2_FG_1920x1112,	1112},
											{NTV2_FG_2048x1112,	1112},
											{NTV2_FG_3840x2160,	2160},
											{NTV2_FG_4096x2160,	2160},
											{NTV2_FG_7680x4320,	4320},
											{NTV2_FG_8192x4320,	4320}	};
	FGSizesMapCI it (sFGHghts.find(fg));
	return it != sFGHghts.end() ? it->second : 0;
}


static const string sSegXferUnits[] = {"", " U8", " U16", "", " U32", "", "", "", " U64", ""};

ostream & NTV2SegmentedXferInfo::Print (ostream & inStrm, const bool inDumpSegments) const
{
	if (!isValid())
		return inStrm << "(invalid)";
	if (inDumpSegments)
	{
		//	TBD
	}
	else
	{
		inStrm	<< DEC(getSegmentCount()) << " x " << DEC(getSegmentLength())
				<< sSegXferUnits[getElementLength()] << " segs";
		if (getSourceOffset())
			inStrm	<< " srcOff=" << xHEX0N(getSourceOffset(),8);
		if (getSegmentCount() > 1)
			inStrm << " srcSpan=" << xHEX0N(getSourcePitch(),8) << (isSourceBottomUp()?" VF":"");
		if (getDestOffset())
			inStrm	<< " dstOff=" << xHEX0N(getDestOffset(),8);
		if (getSegmentCount() > 1)
			inStrm << " dstSpan=" << xHEX0N(getDestPitch(),8) << (isDestBottomUp()?" VF":"");
		inStrm << " totElm=" << DEC(getTotalElements()) << " totByt=" << xHEX0N(getTotalBytes(),8);
	}
	return inStrm;
}

string NTV2SegmentedXferInfo::getSourceCode (const bool inInclDecl) const
{
	static string var("segInfo");
	ostringstream oss;
	string units("\t// bytes");
	if (!isValid())
		return "";
	if (inInclDecl)
		oss << "NTV2SegmentedXferInfo " << var << ";" << endl;
	if (getElementLength() > 1)
	{
		units = "\t// " + sSegXferUnits[getElementLength()] + "s";
		oss << var << ".setElementLength(" << getElementLength() << ");" << endl;
	}
	oss << var << ".setSegmentCount(" << DEC(getSegmentCount()) << ");" << endl;
	oss << var << ".setSegmentLength(" << DEC(getSegmentLength()) << ");" << units << endl;
	if (getSourceOffset())
		oss << var << ".setSourceOffset(" << DEC(getSourceOffset()) << ");" << units << endl;
	oss << var << ".setSourcePitch(" << DEC(getSourcePitch()) << ");" << units << endl;
	if (isSourceBottomUp())
		oss << var << ".setSourceDirection(false);" << endl;
	if (getDestOffset())
		oss << var << ".setDestOffset(" << DEC(getDestOffset()) << ");" << units << endl;
	if (getDestPitch())
		oss << var << ".setDestPitch(" << DEC(getDestPitch()) << ");" << units << endl;
	if (isDestBottomUp())
		oss << var << ".setDestDirection(false);" << endl;
	return oss.str();
}

bool NTV2SegmentedXferInfo::containsElementAtOffset (const ULWord inElementOffset) const
{
	if (!isValid())
		return false;
	if (getSegmentCount() == 1)
	{
		if (inElementOffset >= getSourceOffset())
			if (inElementOffset < getSourceOffset()+getSegmentLength())
				return true;
		return false;
	}
	ULWord offset(getSourceOffset());
	for (ULWord seg(0);	 seg < getSegmentCount();  seg++)
	{
		if (inElementOffset < offset)
			return false;	//	past element of interest already
		if (inElementOffset < offset+getSegmentLength())
			return true;	//	must be within this segment
		offset += getSourcePitch(); //	skip to next segment
	}
	return false;
}

bool NTV2SegmentedXferInfo::operator != (const NTV2SegmentedXferInfo & inRHS) const
{
	if (getElementLength() != inRHS.getElementLength())
		//	FUTURE TBD:	 Need to transform RHS to match ElementLength so as to make apples-to-apples comparison
		return true;	//	For now, fail
	if (getSegmentCount() != inRHS.getSegmentCount())
		return true;
	if (getSegmentLength() != inRHS.getSegmentLength())
		return true;
	if (getSourceOffset() != inRHS.getSourceOffset())
		return true;
	if (getSourcePitch() != inRHS.getSourcePitch())
		return true;
	if (getDestOffset() != inRHS.getDestOffset())
		return true;
	if (getDestPitch() != inRHS.getDestPitch())
		return true;
	return false;
}

NTV2SegmentedXferInfo & NTV2SegmentedXferInfo::reset (void)
{
	mFlags				= 0;
	mNumSegments		= 0;
	mElementsPerSegment = 0;
	mInitialSrcOffset	= 0;
	mInitialDstOffset	= 0;
	mSrcElementsPerRow	= 0;
	mDstElementsPerRow	= 0;
	setElementLength(1);	//	elements == bytes
	return *this;
}

NTV2SegmentedXferInfo & NTV2SegmentedXferInfo::swapSourceAndDestination (void)
{
	std::swap(mSrcElementsPerRow, mDstElementsPerRow);
	std::swap(mInitialSrcOffset, mInitialDstOffset);
	const bool srcNormal(this->isSourceTopDown()), dstNormal(this->isDestTopDown());
	setSourceDirection(dstNormal).setDestDirection(srcNormal);
	return *this;
}


NTV2Buffer::NTV2Buffer (const void * pInUserPointer, const size_t inByteCount)
	:	fUserSpacePtr		(inByteCount ? NTV2Buffer_TO_ULWORD64(pInUserPointer) : 0),
		fByteCount			(ULWord(pInUserPointer ? inByteCount : 0)),
		fFlags				(0),
	#if defined (AJAMac)
		fKernelSpacePtr		(0),
		fIOMemoryDesc		(0),
		fIOMemoryMap		(0)
	#else
		//fKernelSpacePtr		(0),
		fKernelHandle		(0)
	#endif
{
}


NTV2Buffer::NTV2Buffer (const size_t inByteCount)
	:	fUserSpacePtr		(0),
		fByteCount			(0),
		fFlags				(0),
	#if defined (AJAMac)
		fKernelSpacePtr		(0),
		fIOMemoryDesc		(0),
		fIOMemoryMap		(0)
	#else
		//fKernelSpacePtr		(0),
		fKernelHandle		(0)
	#endif
{
	if (inByteCount)
		Allocate(inByteCount);
}


NTV2Buffer::NTV2Buffer (const NTV2Buffer & inObj)
	:	fUserSpacePtr		(0),
		fByteCount			(0),
		fFlags				(0),
	#if defined (AJAMac)
		fKernelSpacePtr		(0),
		fIOMemoryDesc		(0),
		fIOMemoryMap		(0)
	#else
		//fKernelSpacePtr		(0),
		fKernelHandle		(0)
	#endif
{
	if (Allocate(inObj.GetByteCount()))
		SetFrom(inObj);
}

bool NTV2Buffer::Truncate (const size_t inNewByteCount)
{
	if (inNewByteCount == GetByteCount())
		return true;	//	Same size -- done!
	if (inNewByteCount > GetByteCount())
		return false;	//	Cannot enlarge -- i.e. can't be greater than my current size
	if (!inNewByteCount  &&  IsAllocatedBySDK())
		return Deallocate();	//	A newByteCount of zero calls Deallocate
	fByteCount = ULWord(inNewByteCount);
	return true;
}

NTV2Buffer & NTV2Buffer::operator = (const NTV2Buffer & inRHS)
{
	if (&inRHS != this)
	{
		if (inRHS.IsNULL())
			Set (AJA_NULL, 0);
		else if (GetByteCount() == inRHS.GetByteCount())
			SetFrom(inRHS);
		else if (Allocate(inRHS.GetByteCount()))
			SetFrom(inRHS);
		//else; //	Error
	}
	return *this;
}


NTV2Buffer::~NTV2Buffer ()
{
	Set (AJA_NULL, 0);	//	Call 'Set' to delete the array (if I allocated it)
}


bool NTV2Buffer::ByteSwap64 (void)
{
	uint64_t *	pU64s(reinterpret_cast<uint64_t*>(GetHostPointer()));
	const size_t loopCount(GetByteCount() / sizeof(uint64_t));
	if (IsNULL())
		return false;
	for (size_t ndx(0);	 ndx < loopCount;  ndx++)
		pU64s[ndx] = NTV2EndianSwap64(pU64s[ndx]);
	return true;
}


bool NTV2Buffer::ByteSwap32 (void)
{
	uint32_t *	pU32s(reinterpret_cast<uint32_t*>(GetHostPointer()));
	const size_t loopCount(GetByteCount() / sizeof(uint32_t));
	if (IsNULL())
		return false;
	for (size_t ndx(0);	 ndx < loopCount;  ndx++)
		pU32s[ndx] = NTV2EndianSwap32(pU32s[ndx]);
	return true;
}


bool NTV2Buffer::ByteSwap16 (void)
{
	uint16_t *	pU16s(reinterpret_cast<uint16_t*>(GetHostPointer()));
	const size_t loopCount(GetByteCount() / sizeof(uint16_t));
	if (IsNULL())
		return false;
	for (size_t ndx(0);	 ndx < loopCount;  ndx++)
		pU16s[ndx] = NTV2EndianSwap16(pU16s[ndx]);
	return true;
}


bool NTV2Buffer::Set (const void * pInUserPointer, const size_t inByteCount)
{
	Deallocate();
	fUserSpacePtr = inByteCount ? NTV2Buffer_TO_ULWORD64(pInUserPointer) : 0;
	fByteCount = ULWord(pInUserPointer ? inByteCount : 0);
	//	Return true only if both UserPointer and ByteCount are non-zero, or both are zero.
	return (pInUserPointer && inByteCount)	||	(!pInUserPointer && !inByteCount);
}


bool NTV2Buffer::SetAndFill (const void * pInUserPointer, const size_t inByteCount, const UByte inValue)
{
	return Set(pInUserPointer, inByteCount)	 &&	 Fill(inValue);
}


bool NTV2Buffer::Allocate (const size_t inByteCount, const bool inPageAligned)
{
	if (GetByteCount()	&&	fFlags & NTV2Buffer_ALLOCATED)	//	If already was Allocated
		if (inByteCount == GetByteCount())					//	If same byte count
		{
			Fill(UByte(0));		//	Zero it...
			return true;	//	...and return true
		}

	bool result(Set(AJA_NULL, 0));	//	Jettison existing buffer (if any)
	if (inByteCount)
	{	//	Allocate the byte array, and call Set...
		UByte * pBuffer(AJA_NULL);
		result = false;
		if (inPageAligned)
			pBuffer = reinterpret_cast<UByte*>(AJAMemory::AllocateAligned(inByteCount, DefaultPageSize()));
		else
			try
				{pBuffer = new UByte[inByteCount];}
			catch (const std::bad_alloc &)
				{pBuffer = AJA_NULL;}
		if (pBuffer	 &&	 Set(pBuffer, inByteCount))
		{	//	SDK owns this memory -- set NTV2Buffer_ALLOCATED bit -- I'm responsible for deleting
			result = true;
			fFlags |= NTV2Buffer_ALLOCATED;
			if (inPageAligned)
				fFlags |= NTV2Buffer_PAGE_ALIGNED;	//	Set "page aligned" flag
			Fill(UByte(0));	//	Zero it
		}
	}	//	if requested size is non-zero
	return result;
}


bool NTV2Buffer::Deallocate (void)
{
	if (IsAllocatedBySDK())
	{
		if (!IsNULL())
		{
			if (IsPageAligned())
			{
				AJAMemory::FreeAligned(GetHostPointer());
				fFlags &= ~NTV2Buffer_PAGE_ALIGNED;
			}
			else
				delete [] reinterpret_cast<UByte*>(GetHostPointer());
		}
		fUserSpacePtr = 0;
		fByteCount = 0;
		fFlags &= ~NTV2Buffer_ALLOCATED;
	}
	return true;
}


void * NTV2Buffer::GetHostAddress (const ULWord inByteOffset, const bool inFromEnd) const
{
	if (IsNULL())
		return AJA_NULL;
	if (inByteOffset >= GetByteCount())
		return AJA_NULL;
	UByte * pBytes	(reinterpret_cast<UByte*>(GetHostPointer()));
	if (inFromEnd)
		pBytes += GetByteCount() - inByteOffset;
	else
		pBytes += inByteOffset;
	return pBytes;
}


bool NTV2Buffer::SetFrom (const NTV2Buffer & inBuffer)
{
	if (inBuffer.IsNULL())
		return false;	//	NULL or empty
	if (IsNULL())
		return false;	//	I am NULL or empty
	if (inBuffer.GetByteCount() == GetByteCount()  &&  inBuffer.GetHostPointer() == GetHostPointer())
		return true;	//	Same buffer

	size_t bytesToCopy(inBuffer.GetByteCount());
	if (bytesToCopy > GetByteCount())
		bytesToCopy = GetByteCount();
	::memcpy (GetHostPointer(), inBuffer.GetHostPointer(), bytesToCopy);
	return true;
}


bool NTV2Buffer::CopyFrom (const void * pInSrcBuffer, const ULWord inByteCount)
{
	if (!inByteCount)
		return Set (AJA_NULL, 0);	//	Zero bytes
	if (!pInSrcBuffer)
		return false;	//	NULL src ptr
	if (!Allocate (inByteCount))
		return false;	//	Resize failed
	::memcpy (GetHostPointer(), pInSrcBuffer, inByteCount);
	return true;
}


bool NTV2Buffer::CopyFrom (const NTV2Buffer & inBuffer,
							const ULWord inSrcByteOffset, const ULWord inDstByteOffset, const ULWord inByteCount)
{
	if (inBuffer.IsNULL() || IsNULL())
		return false;	//	NULL or empty
	if (inSrcByteOffset + inByteCount > inBuffer.GetByteCount())
		return false;	//	Past end of src
	if (inDstByteOffset + inByteCount > GetByteCount())
		return false;	//	Past end of me

	const UByte * pSrc (inBuffer);
	pSrc += inSrcByteOffset;

	UByte * pDst (*this);
	pDst += inDstByteOffset;

	::memcpy (pDst, pSrc, inByteCount);
	return true;
}


bool NTV2Buffer::CopyFrom (const NTV2Buffer & inSrcBuffer, const NTV2SegmentedXferInfo & inXferInfo)
{
	if (!inXferInfo.isValid()  ||  inSrcBuffer.IsNULL()	 ||	 IsNULL())
		return false;

	//	Copy every segment...
	LWord	srcOffset	(LWord(inXferInfo.getSourceOffset() * inXferInfo.getElementLength()));
	LWord	dstOffset	(LWord(inXferInfo.getDestOffset() * inXferInfo.getElementLength()));
	LWord	srcPitch	(LWord(inXferInfo.getSourcePitch() * inXferInfo.getElementLength()));
	LWord	dstPitch	(LWord(inXferInfo.getDestPitch() * inXferInfo.getElementLength()));
	const LWord	bytesPerSeg (inXferInfo.getSegmentLength() * inXferInfo.getElementLength());
	if (inXferInfo.isSourceBottomUp())
		srcPitch = 0 - srcPitch;
	if (inXferInfo.isDestBottomUp())
		dstPitch = 0 - dstPitch;
	for (ULWord segNdx(0);	segNdx < inXferInfo.getSegmentCount();	segNdx++)
	{
		const void *	pSrc (inSrcBuffer.GetHostAddress(srcOffset));
		void *			pDst (GetHostAddress(dstOffset));
		if (!pSrc)	return false;
		if (!pDst)	return false;
		if (srcOffset + bytesPerSeg > LWord(inSrcBuffer.GetByteCount()))
			return false;	//	memcpy will read past end of srcBuffer
		if (dstOffset + bytesPerSeg > LWord(GetByteCount()))
			return false;	//	memcpy will write past end of me
		::memcpy (pDst,	 pSrc,	size_t(bytesPerSeg));
		srcOffset += srcPitch;	//	Bump src offset
		dstOffset += dstPitch;	//	Bump dst offset
	}	//	for each segment
	return true;
}

bool NTV2Buffer::SetFromHexString (const string & inStr)
{
	string str(inStr);

	//	Remove all whitespace...
	const string newline("\n"), tab("\t");
	aja::replace(str, newline, string());
	aja::replace(str, tab, string());
	aja::upper(str);

	//	Fail if any non-hex found...
	for (size_t ndx(0);  ndx < str.size();  ndx++)
		if (!aja::is_hex_digit(str.at(ndx)))
			return false;

	if (str.size() & 1)
		return false;	//	Remaining length must be even
	if (!Allocate(str.size() / 2))
		return false;	//	Resize failed

	//	Decode and copy in the data...
	for (size_t srcNdx(0), dstNdx(0);  srcNdx < str.size();  srcNdx += 2)
		U8(int(dstNdx++)) = uint8_t(aja::stoul (str.substr(srcNdx,2), AJA_NULL, 16));

	return true;
}

bool NTV2Buffer::SwapWith (NTV2Buffer & inBuffer)
{
	if (inBuffer.IsNULL ())
		return false;	//	NULL or empty
	if (IsNULL ())
		return false;	//	I am NULL or empty
	if (inBuffer.GetByteCount () != GetByteCount ())
		return false;	//	Different sizes
	if (fFlags != inBuffer.fFlags)
		return false;	//	Flags mismatch
	if (inBuffer.GetHostPointer () == GetHostPointer ())
		return true;	//	Same buffer

	ULWord64	tmp			= fUserSpacePtr;
	fUserSpacePtr			= inBuffer.fUserSpacePtr;
	inBuffer.fUserSpacePtr	= tmp;
	return true;
}

set<ULWord> & NTV2Buffer::FindAll (set<ULWord> & outOffsets, const NTV2Buffer & inValue) const
{
	outOffsets.clear();
	if (IsNULL())
		return outOffsets;	//	NULL buffer, return "no matches"
	if (inValue.IsNULL())
		return outOffsets;	//	NULL buffer, return "no matches"
	const ULWord srchByteCount(inValue.GetByteCount());
	if (GetByteCount() < srchByteCount)
		return outOffsets;	//	I'm smaller than the search data, return "no matches"

	const ULWord maxOffset(GetByteCount() - srchByteCount);	//	Don't search past here
	const uint8_t * pSrchData (inValue);	//	Pointer to search data
	const uint8_t * pMyData (*this);		//	Pointer to where search starts in me
	ULWord offset(0);						//	Search starts at this byte offset
	do
	{
		if (!::memcmp(pMyData, pSrchData, srchByteCount))
			outOffsets.insert(offset);	//	Record byte offset of match
		pMyData++;	//	Bump search pointer
		offset++;	//	Bump search byte offset
	} while (offset < maxOffset);
	return outOffsets;
}

bool NTV2Buffer::IsContentEqual (const NTV2Buffer & inBuffer, const ULWord inByteOffset, const ULWord inByteCount) const
{
	if (IsNULL() || inBuffer.IsNULL())
		return false;	//	Buffer(s) are NULL/empty
	if (inBuffer.GetByteCount() != GetByteCount())
		return false;	//	Buffers are different sizes

	ULWord	totalBytes(GetByteCount());
	if (inByteOffset >= totalBytes)
		return false;	//	Bad offset

	totalBytes -= inByteOffset;

	ULWord	byteCount(inByteCount);
	if (byteCount > totalBytes)
		byteCount = totalBytes;

	if (inBuffer.GetHostPointer() == GetHostPointer())
		return true;	//	Same buffer

	const UByte *	pByte1 (*this);
	const UByte *	pByte2 (inBuffer);
	pByte1 += inByteOffset;
	pByte2 += inByteOffset;
	#if !defined(NTV2BUFFER_NO_MEMCMP)
	return ::memcmp (pByte1, pByte2, byteCount) == 0;
	#else	//	NTV2BUFFER_NO_MEMCMP
		ULWord offset(inByteOffset);
		while (byteCount)
		{
			if (*pByte1 != *pByte2)
			{
				cerr << "## ERROR: IsContentEqual: miscompare at offset " << xHEX0N(offset,8)
					<< " (" << DEC(offset) << "): " << xHEX0N(UWord(*pByte1),2) << " != "
					<< xHEX0N(UWord(*pByte2),2) << ", " << xHEX0N(byteCount,8) << " ("
					<< DEC(byteCount) << ") bytes left to compare" << endl;
				return false;
			}
			pByte1++;  pByte2++;
			byteCount--;
			offset++;
		}
		return true;
	#endif	//	NTV2BUFFER_NO_MEMCMP
}

bool NTV2Buffer::NextDifference (const NTV2Buffer & inBuffer, ULWord & byteOffset) const
{
	if (byteOffset == 0xFFFFFFFF)
		return false;	//	bad offset
	if (IsNULL() || inBuffer.IsNULL())
		return false;	//	NULL or empty buffers
	if (inBuffer.GetByteCount() != GetByteCount())
		return false;	//	Different byte counts
	if (inBuffer.GetHostPointer() == GetHostPointer())
		{byteOffset = 0xFFFFFFFF;  return true;}	//	Same buffer

	ULWord	totalBytesToCompare(GetByteCount());
	if (byteOffset >= totalBytesToCompare)
		return false;	//	Bad offset
	totalBytesToCompare -= byteOffset;

	const UByte * pByte1 (*this);
	const UByte * pByte2 (inBuffer);
	while (totalBytesToCompare)
	{
		if (pByte1[byteOffset] != pByte2[byteOffset])
			return true;
		totalBytesToCompare--;
		byteOffset++;
	}
	byteOffset = 0xFFFFFFFF;
	return true;
}

bool NTV2Buffer::GetRingChangedByteRange (const NTV2Buffer & inBuffer, ULWord & outByteOffsetFirst, ULWord & outByteOffsetLast) const
{
	outByteOffsetFirst = outByteOffsetLast = GetByteCount ();
	if (IsNULL () || inBuffer.IsNULL ())
		return false;	//	NULL or empty
	if (inBuffer.GetByteCount () != GetByteCount ())
		return false;	//	Different byte counts
	if (inBuffer.GetHostPointer () == GetHostPointer ())
		return true;	//	Same buffer
	if (GetByteCount() < 3)
		return false;	//	Too small

	const UByte *	pByte1 (reinterpret_cast <const UByte *> (GetHostPointer()));
	const UByte *	pByte2 (reinterpret_cast <const UByte *> (inBuffer.GetHostPointer()));

	outByteOffsetFirst = 0;
	while (outByteOffsetFirst < GetByteCount())
	{
		if (*pByte1 != *pByte2)
			break;
		pByte1++;
		pByte2++;
		outByteOffsetFirst++;
	}
	if (outByteOffsetFirst == 0)
	{
		//	Wrap case -- look for first match...
		while (outByteOffsetFirst < GetByteCount())
		{
			if (*pByte1 == *pByte2)
				break;
			pByte1++;
			pByte2++;
			outByteOffsetFirst++;
		}
		if (outByteOffsetFirst < GetByteCount())
			outByteOffsetFirst--;
	}
	if (outByteOffsetFirst == GetByteCount())
		return true;	//	Identical --- outByteOffsetFirst == outByteOffsetLast == GetByteCount()

	//	Now scan from the end...
	pByte1 = reinterpret_cast <const UByte *> (GetHostPointer());
	pByte2 = reinterpret_cast <const UByte *> (inBuffer.GetHostPointer());
	pByte1 += GetByteCount () - 1;	//	Point to last byte
	pByte2 += GetByteCount () - 1;
	while (--outByteOffsetLast)
	{
		if (*pByte1 != *pByte2)
			break;
		pByte1--;
		pByte2--;
	}
	if (outByteOffsetLast == (GetByteCount() - 1))
	{
		//	Wrap case -- look for first match...
		while (outByteOffsetLast)
		{
			if (*pByte1 == *pByte2)
				break;
			pByte1--;
			pByte2--;
			outByteOffsetLast--;
		}
		if (outByteOffsetLast < GetByteCount())
			outByteOffsetLast++;
		if (outByteOffsetLast <= outByteOffsetFirst)
			cerr << "## WARNING:  GetRingChangedByteRange:	last " << outByteOffsetLast << " <= first " << outByteOffsetFirst << " in wrap condition" << endl;
		const ULWord	tmp (outByteOffsetLast);
		outByteOffsetLast = outByteOffsetFirst;
		outByteOffsetFirst = tmp;
		if (outByteOffsetLast >= outByteOffsetFirst)
			cerr << "## WARNING:  GetRingChangedByteRange:	last " << outByteOffsetLast << " >= first " << outByteOffsetFirst << " in wrap condition" << endl;
	}
	return true;

}	//	GetRingChangedByteRange


static size_t	gDefaultPageSize	(AJA_PAGE_SIZE);

size_t NTV2Buffer::DefaultPageSize (void)
{
	return gDefaultPageSize;
}

bool NTV2Buffer::SetDefaultPageSize (const size_t inNewSize)
{
	const bool result (inNewSize  &&  (!(inNewSize & (inNewSize - 1))));
	if (result)
		gDefaultPageSize = inNewSize;
	return result;
}

size_t NTV2Buffer::HostPageSize (void)
{
#if defined(MSWindows) || defined(AJABareMetal)
	return AJA_PAGE_SIZE;
#else
	return size_t(::getpagesize());
#endif
}


FRAME_STAMP::FRAME_STAMP ()
	:	acHeader						(NTV2_TYPE_ACFRAMESTAMP, sizeof(FRAME_STAMP)),
		acFrameTime						(0),
		acRequestedFrame				(0),
		acAudioClockTimeStamp			(0),
		acAudioExpectedAddress			(0),
		acAudioInStartAddress			(0),
		acAudioInStopAddress			(0),
		acAudioOutStopAddress			(0),
		acAudioOutStartAddress			(0),
		acTotalBytesTransferred			(0),
		acStartSample					(0),
		acTimeCodes						(NTV2_MAX_NUM_TIMECODE_INDEXES * sizeof (NTV2_RP188)),
		acCurrentTime					(0),
		acCurrentFrame					(0),
		acCurrentFrameTime				(0),
		acAudioClockCurrentTime			(0),
		acCurrentAudioExpectedAddress	(0),
		acCurrentAudioStartAddress		(0),
		acCurrentFieldCount				(0),
		acCurrentLineCount				(0),
		acCurrentReps					(0),
		acCurrentUserCookie				(0),
		acFrame							(0),
		acRP188							()
{
	NTV2_ASSERT_STRUCT_VALID;
}


FRAME_STAMP::FRAME_STAMP (const FRAME_STAMP & inObj)
	:	acHeader						(inObj.acHeader),
		acFrameTime						(inObj.acFrameTime),
		acRequestedFrame				(inObj.acRequestedFrame),
		acAudioClockTimeStamp			(inObj.acAudioClockTimeStamp),
		acAudioExpectedAddress			(inObj.acAudioExpectedAddress),
		acAudioInStartAddress			(inObj.acAudioInStartAddress),
		acAudioInStopAddress			(inObj.acAudioInStopAddress),
		acAudioOutStopAddress			(inObj.acAudioOutStopAddress),
		acAudioOutStartAddress			(inObj.acAudioOutStartAddress),
		acTotalBytesTransferred			(inObj.acTotalBytesTransferred),
		acStartSample					(inObj.acStartSample),
		acCurrentTime					(inObj.acCurrentTime),
		acCurrentFrame					(inObj.acCurrentFrame),
		acCurrentFrameTime				(inObj.acCurrentFrameTime),
		acAudioClockCurrentTime			(inObj.acAudioClockCurrentTime),
		acCurrentAudioExpectedAddress	(inObj.acCurrentAudioExpectedAddress),
		acCurrentAudioStartAddress		(inObj.acCurrentAudioStartAddress),
		acCurrentFieldCount				(inObj.acCurrentFieldCount),
		acCurrentLineCount				(inObj.acCurrentLineCount),
		acCurrentReps					(inObj.acCurrentReps),
		acCurrentUserCookie				(inObj.acCurrentUserCookie),
		acFrame							(inObj.acFrame),
		acRP188							(inObj.acRP188),
		acTrailer						(inObj.acTrailer)
{
	acTimeCodes.SetFrom (inObj.acTimeCodes);
}


FRAME_STAMP::~FRAME_STAMP ()
{
}


bool FRAME_STAMP::GetInputTimeCodes (NTV2TimeCodeList & outValues) const
{
	NTV2_ASSERT_STRUCT_VALID;
	ULWord				numRP188s	(acTimeCodes.GetByteCount () / sizeof (NTV2_RP188));
	const NTV2_RP188 *	pArray		(reinterpret_cast <const NTV2_RP188 *> (acTimeCodes.GetHostPointer ()));
	outValues.clear ();
	if (!pArray)
		return false;		//	No 'acTimeCodes' array!

	if (numRP188s > NTV2_MAX_NUM_TIMECODE_INDEXES)
		numRP188s = NTV2_MAX_NUM_TIMECODE_INDEXES;	//	clamp to this max number

	for (ULWord ndx (0);  ndx < numRP188s;	ndx++)
		outValues << pArray [ndx];

	return true;
}


bool FRAME_STAMP::GetInputTimeCode (NTV2_RP188 & outTimeCode, const NTV2TCIndex inTCIndex) const
{
	NTV2_ASSERT_STRUCT_VALID;
	ULWord				numRP188s	(acTimeCodes.GetByteCount () / sizeof (NTV2_RP188));
	const NTV2_RP188 *	pArray		(reinterpret_cast <const NTV2_RP188 *> (acTimeCodes.GetHostPointer ()));
	outTimeCode.Set (); //	invalidate
	if (!pArray)
		return false;		//	No 'acTimeCodes' array!
	if (numRP188s > NTV2_MAX_NUM_TIMECODE_INDEXES)
		numRP188s = NTV2_MAX_NUM_TIMECODE_INDEXES;	//	clamp to this max number
	if (!NTV2_IS_VALID_TIMECODE_INDEX (inTCIndex))
		return false;

	outTimeCode = pArray [inTCIndex];
	return true;
}


bool FRAME_STAMP::GetInputTimeCodes (NTV2TimeCodes & outTimeCodes, const NTV2Channel inSDIInput, const bool inValidOnly) const
{
	NTV2_ASSERT_STRUCT_VALID;
	outTimeCodes.clear();

	if (!NTV2_IS_VALID_CHANNEL(inSDIInput))
		return false;	//	Bad SDI input

	NTV2TimeCodeList	allTCs;
	if (!GetInputTimeCodes(allTCs))
		return false;	//	GetInputTimeCodes failed

	const NTV2TCIndexes tcIndexes (GetTCIndexesForSDIInput(inSDIInput));
	for (NTV2TCIndexesConstIter iter(tcIndexes.begin());  iter != tcIndexes.end();	++iter)
	{
		const NTV2TCIndex tcIndex(*iter);
		NTV2_ASSERT(NTV2_IS_VALID_TIMECODE_INDEX(tcIndex));
		const NTV2_RP188 tc(allTCs.at(tcIndex));
		if (!inValidOnly)
			outTimeCodes[tcIndex] = tc;
		else if (tc.IsValid())
			outTimeCodes[tcIndex] = tc;
	}
	return true;
}


bool FRAME_STAMP::GetSDIInputStatus(NTV2SDIInputStatus & outStatus, const UWord inSDIInputIndex0) const
{
	NTV2_ASSERT_STRUCT_VALID;
	(void)outStatus;
	(void)inSDIInputIndex0;
	return true;
}

bool FRAME_STAMP::SetInputTimecode (const NTV2TCIndex inTCNdx, const NTV2_RP188 & inTimecode)
{
	ULWord			numRP188s	(acTimeCodes.GetByteCount() / sizeof(NTV2_RP188));
	NTV2_RP188 *	pArray		(reinterpret_cast<NTV2_RP188*>(acTimeCodes.GetHostPointer()));
	if (!pArray	 ||	 !numRP188s)
		return false;		//	No 'acTimeCodes' array!

	if (numRP188s > NTV2_MAX_NUM_TIMECODE_INDEXES)
		numRP188s = NTV2_MAX_NUM_TIMECODE_INDEXES;	//	clamp to this max number
	if (ULWord(inTCNdx) >= numRP188s)
		return false;	//	Past end

	pArray[inTCNdx] = inTimecode;	//	Write the new value
	return true;	//	Success!
}


FRAME_STAMP & FRAME_STAMP::operator = (const FRAME_STAMP & inRHS)
{
	if (this != &inRHS)
	{
		acTimeCodes						= inRHS.acTimeCodes;
		acHeader						= inRHS.acHeader;
		acFrameTime						= inRHS.acFrameTime;
		acRequestedFrame				= inRHS.acRequestedFrame;
		acAudioClockTimeStamp			= inRHS.acAudioClockTimeStamp;
		acAudioExpectedAddress			= inRHS.acAudioExpectedAddress;
		acAudioInStartAddress			= inRHS.acAudioInStartAddress;
		acAudioInStopAddress			= inRHS.acAudioInStopAddress;
		acAudioOutStopAddress			= inRHS.acAudioOutStopAddress;
		acAudioOutStartAddress			= inRHS.acAudioOutStartAddress;
		acTotalBytesTransferred			= inRHS.acTotalBytesTransferred;
		acStartSample					= inRHS.acStartSample;
		acCurrentTime					= inRHS.acCurrentTime;
		acCurrentFrame					= inRHS.acCurrentFrame;
		acCurrentFrameTime				= inRHS.acCurrentFrameTime;
		acAudioClockCurrentTime			= inRHS.acAudioClockCurrentTime;
		acCurrentAudioExpectedAddress	= inRHS.acCurrentAudioExpectedAddress;
		acCurrentAudioStartAddress		= inRHS.acCurrentAudioStartAddress;
		acCurrentFieldCount				= inRHS.acCurrentFieldCount;
		acCurrentLineCount				= inRHS.acCurrentLineCount;
		acCurrentReps					= inRHS.acCurrentReps;
		acCurrentUserCookie				= inRHS.acCurrentUserCookie;
		acFrame							= inRHS.acFrame;
		acRP188							= inRHS.acRP188;
		acTrailer						= inRHS.acTrailer;
	}
	return *this;
}


bool FRAME_STAMP::SetFrom (const FRAME_STAMP_STRUCT & inOldStruct)
{
	NTV2_ASSERT_STRUCT_VALID;
	//acCrosspoint					= inOldStruct.channelSpec;
	acFrameTime						= inOldStruct.frameTime;
	acRequestedFrame				= inOldStruct.frame;
	acAudioClockTimeStamp			= inOldStruct.audioClockTimeStamp;
	acAudioExpectedAddress			= inOldStruct.audioExpectedAddress;
	acAudioInStartAddress			= inOldStruct.audioInStartAddress;
	acAudioInStopAddress			= inOldStruct.audioInStopAddress;
	acAudioOutStopAddress			= inOldStruct.audioOutStopAddress;
	acAudioOutStartAddress			= inOldStruct.audioOutStartAddress;
	acTotalBytesTransferred			= inOldStruct.bytesRead;
	acStartSample					= inOldStruct.startSample;
	acCurrentTime					= inOldStruct.currentTime;
	acCurrentFrame					= inOldStruct.currentFrame;
	acCurrentFrameTime				= inOldStruct.currentFrameTime;
	acAudioClockCurrentTime			= inOldStruct.audioClockCurrentTime;
	acCurrentAudioExpectedAddress	= inOldStruct.currentAudioExpectedAddress;
	acCurrentAudioStartAddress		= inOldStruct.currentAudioStartAddress;
	acCurrentFieldCount				= inOldStruct.currentFieldCount;
	acCurrentLineCount				= inOldStruct.currentLineCount;
	acCurrentReps					= inOldStruct.currentReps;
	acCurrentUserCookie				= inOldStruct.currenthUser;
	acRP188							= NTV2_RP188 (inOldStruct.currentRP188);
	if (!acTimeCodes.IsNULL() && acTimeCodes.GetByteCount () >= sizeof (NTV2_RP188))
	{
		NTV2_RP188 *	pTimecodes	(reinterpret_cast <NTV2_RP188 *> (acTimeCodes.GetHostPointer ()));
		NTV2_ASSERT (pTimecodes);
		NTV2_ASSERT (NTV2_TCINDEX_DEFAULT == 0);
		pTimecodes [NTV2_TCINDEX_DEFAULT] = acRP188;
	}
	return true;
}


bool FRAME_STAMP::CopyTo (FRAME_STAMP_STRUCT & outOldStruct) const
{
	NTV2_ASSERT_STRUCT_VALID;
	outOldStruct.frameTime						= acFrameTime;
	outOldStruct.frame							= acRequestedFrame;
	outOldStruct.audioClockTimeStamp			= acAudioClockTimeStamp;
	outOldStruct.audioExpectedAddress			= acAudioExpectedAddress;
	outOldStruct.audioInStartAddress			= acAudioInStartAddress;
	outOldStruct.audioInStopAddress				= acAudioInStopAddress;
	outOldStruct.audioOutStopAddress			= acAudioOutStopAddress;
	outOldStruct.audioOutStartAddress			= acAudioOutStartAddress;
	outOldStruct.bytesRead						= acTotalBytesTransferred;
	outOldStruct.startSample					= acStartSample;
	outOldStruct.currentTime					= acCurrentTime;
	outOldStruct.currentFrame					= acCurrentFrame;
	outOldStruct.currentFrameTime				= acCurrentFrameTime;
	outOldStruct.audioClockCurrentTime			= acAudioClockCurrentTime;
	outOldStruct.currentAudioExpectedAddress	= acCurrentAudioExpectedAddress;
	outOldStruct.currentAudioStartAddress		= acCurrentAudioStartAddress;
	outOldStruct.currentFieldCount				= acCurrentFieldCount;
	outOldStruct.currentLineCount				= acCurrentLineCount;
	outOldStruct.currentReps					= acCurrentReps;
	outOldStruct.currenthUser					= ULWord(acCurrentUserCookie);
	outOldStruct.currentRP188					= acRP188;
	//	Ticket 3367 -- Mark Gilbert of Gallery UK reports that after updating from AJA Retail Software 10.5 to 14.0,
	//	their QuickTime app stopped receiving timecode during capture. Turns out the QuickTime components use the new
	//	AutoCirculate APIs, but internally still use the old FRAME_STAMP_STRUCT for frame info, including timecode...
	//	...and only use the "currentRP188" field for the "retail" timecode.
	//	Sadly, this FRAME_STAMP-to-FRAME_STAMP_STRUCT function historically only set "currentRP188" from the deprecated
	//	(and completely unused) "acRP188" field, when it really should've been using the acTimeCodes[NTV2_TCINDEX_DEFAULT]
	//	value all along...
	if (!acTimeCodes.IsNULL())									//	If there's an acTimeCodes buffer...
		if (acTimeCodes.GetByteCount() >= sizeof(NTV2_RP188))	//	...and it has at least one timecode value...
		{
			const NTV2_RP188 *	pDefaultTC	(reinterpret_cast<const NTV2_RP188*>(acTimeCodes.GetHostPointer()));
			if (pDefaultTC)
				outOldStruct.currentRP188		= pDefaultTC[NTV2_TCINDEX_DEFAULT]; //	Stuff the "default" (retail) timecode into "currentRP188".
		}
	return true;
}


string FRAME_STAMP::operator [] (const unsigned inIndexNum) const
{
	ostringstream	oss;
	NTV2_RP188		rp188;
	if (GetInputTimeCode (rp188, NTV2TimecodeIndex (inIndexNum)))
	{
		if (rp188.IsValid())
		{
			CRP188	foo (rp188);
			oss << foo;
		}
		else
			oss << "---";
	}
	else if (NTV2_IS_VALID_TIMECODE_INDEX (inIndexNum))
		oss << "---";
	return oss.str();
}


NTV2SDIInStatistics::NTV2SDIInStatistics()
	:	mHeader(NTV2_TYPE_SDISTATS, sizeof(NTV2SDIInStatistics)),
		mInStatistics(NTV2_MAX_NUM_CHANNELS * sizeof(NTV2SDIInputStatus))
{
	Clear();
	NTV2_ASSERT_STRUCT_VALID;
}

void NTV2SDIInStatistics::Clear(void)
{
	NTV2_ASSERT_STRUCT_VALID;
	if (mInStatistics.IsNULL())
		return;
	NTV2SDIInputStatus * pArray(reinterpret_cast <NTV2SDIInputStatus *> (mInStatistics.GetHostPointer()));
	for (int i = 0; i < NTV2_MAX_NUM_CHANNELS; i++)
		pArray[i].Clear();
}

bool NTV2SDIInStatistics::GetSDIInputStatus(NTV2SDIInputStatus & outStatus, const UWord inSDIInputIndex0)
{
	NTV2_ASSERT_STRUCT_VALID;
	const ULWord		numElements(mInStatistics.GetByteCount() / sizeof(NTV2SDIInputStatus));
	const NTV2SDIInputStatus *	pArray(reinterpret_cast <const NTV2SDIInputStatus *> (mInStatistics.GetHostPointer()));
	outStatus.Clear();
	if (!pArray)
		return false;
	if (numElements != 8)
		return false;
	if (inSDIInputIndex0 >= numElements)
		return false;
	outStatus = pArray[inSDIInputIndex0];
	return true;
}

NTV2SDIInputStatus &	NTV2SDIInStatistics::operator [] (const size_t inSDIInputIndex0)
{
	NTV2_ASSERT_STRUCT_VALID;
	static NTV2SDIInputStatus dummy;
	const ULWord	numElements(mInStatistics.GetByteCount() / sizeof(NTV2SDIInputStatus));
	NTV2SDIInputStatus * pArray(reinterpret_cast<NTV2SDIInputStatus*>(mInStatistics.GetHostPointer()));
	if (!pArray)
		return dummy;
	if (numElements != 8)
		return dummy;
	if (inSDIInputIndex0 >= numElements)
		return dummy;
	return pArray[inSDIInputIndex0];
}

std::ostream &	NTV2SDIInStatistics::Print(std::ostream & inOutStream) const
{
	NTV2_ASSERT_STRUCT_VALID;
	inOutStream << mHeader << ", " << mInStatistics << ", " << mTrailer; return inOutStream;
}


AUTOCIRCULATE_TRANSFER_STATUS::AUTOCIRCULATE_TRANSFER_STATUS ()
	:	acHeader					(NTV2_TYPE_ACXFERSTATUS, sizeof (AUTOCIRCULATE_TRANSFER_STATUS)),
		acState						(NTV2_AUTOCIRCULATE_DISABLED),
		acTransferFrame				(0),
		acBufferLevel				(0),
		acFramesProcessed			(0),
		acFramesDropped				(0),
		acFrameStamp				(),
		acAudioTransferSize			(0),
		acAudioStartSample			(0),
		acAncTransferSize			(0),
		acAncField2TransferSize		(0)
		//acTrailer					()
{
	NTV2_ASSERT_STRUCT_VALID;
}


AUTOCIRCULATE_STATUS::AUTOCIRCULATE_STATUS (const NTV2Crosspoint inCrosspoint)
	:	acHeader				(NTV2_TYPE_ACSTATUS, sizeof (AUTOCIRCULATE_STATUS)),
		acCrosspoint			(inCrosspoint),
		acState					(NTV2_AUTOCIRCULATE_DISABLED),
		acStartFrame			(0),
		acEndFrame				(0),
		acActiveFrame			(0),
		acRDTSCStartTime		(0),
		acAudioClockStartTime	(0),
		acRDTSCCurrentTime		(0),
		acAudioClockCurrentTime (0),
		acFramesProcessed		(0),
		acFramesDropped			(0),
		acBufferLevel			(0),
		acOptionFlags			(0),
		acAudioSystem			(NTV2_AUDIOSYSTEM_INVALID)
{
	NTV2_ASSERT_STRUCT_VALID;
}


bool AUTOCIRCULATE_STATUS::CopyTo (AUTOCIRCULATE_STATUS_STRUCT & outOldStruct)
{
	NTV2_ASSERT_STRUCT_VALID;
	outOldStruct.channelSpec			= acCrosspoint;
	outOldStruct.state					= acState;
	outOldStruct.startFrame				= acStartFrame;
	outOldStruct.endFrame				= acEndFrame;
	outOldStruct.activeFrame			= acActiveFrame;
	outOldStruct.rdtscStartTime			= acRDTSCStartTime;
	outOldStruct.audioClockStartTime	= acAudioClockStartTime;
	outOldStruct.rdtscCurrentTime		= acRDTSCCurrentTime;
	outOldStruct.audioClockCurrentTime	= acAudioClockCurrentTime;
	outOldStruct.framesProcessed		= acFramesProcessed;
	outOldStruct.framesDropped			= acFramesDropped;
	outOldStruct.bufferLevel			= acBufferLevel;
	outOldStruct.bWithAudio				= NTV2_IS_VALID_AUDIO_SYSTEM (acAudioSystem);
	outOldStruct.bWithRP188				= acOptionFlags & AUTOCIRCULATE_WITH_RP188 ? 1 : 0;
	outOldStruct.bFbfChange				= acOptionFlags & AUTOCIRCULATE_WITH_FBFCHANGE ? 1 : 0;
	outOldStruct.bFboChange				= acOptionFlags & AUTOCIRCULATE_WITH_FBOCHANGE ? 1 : 0;
	outOldStruct.bWithColorCorrection	= acOptionFlags & AUTOCIRCULATE_WITH_COLORCORRECT ? 1 : 0;
	outOldStruct.bWithVidProc			= acOptionFlags & AUTOCIRCULATE_WITH_VIDPROC ? 1 : 0;
	outOldStruct.bWithCustomAncData		= acOptionFlags & AUTOCIRCULATE_WITH_ANC ? 1 : 0;
	return true;
}


bool AUTOCIRCULATE_STATUS::CopyFrom (const AUTOCIRCULATE_STATUS_STRUCT & inOldStruct)
{
	NTV2_ASSERT_STRUCT_VALID;
	acCrosspoint			= inOldStruct.channelSpec;
	acState					= inOldStruct.state;
	acStartFrame			= inOldStruct.startFrame;
	acEndFrame				= inOldStruct.endFrame;
	acActiveFrame			= inOldStruct.activeFrame;
	acRDTSCStartTime		= inOldStruct.rdtscStartTime;
	acAudioClockStartTime	= inOldStruct.audioClockStartTime;
	acRDTSCCurrentTime		= inOldStruct.rdtscCurrentTime;
	acAudioClockCurrentTime = inOldStruct.audioClockCurrentTime;
	acFramesProcessed		= inOldStruct.framesProcessed;
	acFramesDropped			= inOldStruct.framesDropped;
	acBufferLevel			= inOldStruct.bufferLevel;
	acAudioSystem			= NTV2_AUDIOSYSTEM_INVALID; //	NTV2_AUDIOSYSTEM_1;
	acOptionFlags			=	(inOldStruct.bWithRP188				? AUTOCIRCULATE_WITH_RP188			: 0) |
								(inOldStruct.bFbfChange				? AUTOCIRCULATE_WITH_FBFCHANGE		: 0) |
								(inOldStruct.bFboChange				? AUTOCIRCULATE_WITH_FBOCHANGE		: 0) |
								(inOldStruct.bWithColorCorrection	? AUTOCIRCULATE_WITH_COLORCORRECT	: 0) |
								(inOldStruct.bWithVidProc			? AUTOCIRCULATE_WITH_VIDPROC		: 0) |
								(inOldStruct.bWithCustomAncData		? AUTOCIRCULATE_WITH_ANC			: 0);
	return true;
}


void AUTOCIRCULATE_STATUS::Clear (void)
{
	NTV2_ASSERT_STRUCT_VALID;
	acCrosspoint			= NTV2CROSSPOINT_INVALID;
	acState					= NTV2_AUTOCIRCULATE_DISABLED;
	acStartFrame			= 0;
	acEndFrame				= 0;
	acActiveFrame			= 0;
	acRDTSCStartTime		= 0;
	acAudioClockStartTime	= 0;
	acRDTSCCurrentTime		= 0;
	acAudioClockCurrentTime = 0;
	acFramesProcessed		= 0;
	acFramesDropped			= 0;
	acBufferLevel			= 0;
	acOptionFlags			= 0;
	acAudioSystem			= NTV2_AUDIOSYSTEM_INVALID;
}


NTV2Channel AUTOCIRCULATE_STATUS::GetChannel (void) const
{
	return ::NTV2CrosspointToNTV2Channel(acCrosspoint);
}


struct ThousandsSeparator : std::numpunct <char>
{
	virtual inline	char			do_thousands_sep() const	{return ',';}
	virtual inline	std::string		do_grouping() const			{return "\03";}
};


template <class T> string CommaStr (const T & inNum)
{
	ostringstream	oss;
	const locale	loc (oss.getloc(), new ThousandsSeparator);
	oss.imbue (loc);
	oss << inNum;
	return oss.str();
}	//	CommaStr


string AUTOCIRCULATE_STATUS::operator [] (const unsigned inIndexNum) const
{
	ostringstream	oss;
	if (inIndexNum == 0)
		oss << ::NTV2AutoCirculateStateToString(acState);
	else if (!IsStopped())
		switch (inIndexNum)
		{
			case 1:		oss << DEC(GetStartFrame());							break;
			case 2:		oss << DEC(GetEndFrame());								break;
			case 3:		oss << DEC(GetFrameCount());							break;
			case 4:		oss << DEC(GetActiveFrame());							break;
			case 5:		oss << xHEX0N(acRDTSCStartTime,16);						break;
			case 6:		oss << xHEX0N(acAudioClockStartTime,16);				break;
			case 7:		oss << DEC(acRDTSCCurrentTime);							break;
			case 8:		oss << DEC(acAudioClockCurrentTime);					break;
			case 9:		oss << CommaStr(GetProcessedFrameCount());				break;
			case 10:	oss << CommaStr(GetDroppedFrameCount());				break;
			case 11:	oss << DEC(GetBufferLevel());							break;
			case 12:	oss << ::NTV2AudioSystemToString(acAudioSystem, true);	break;
			case 13:	oss << (WithRP188()			? "Yes" : "No");			break;
			case 14:	oss << (WithLTC()			? "Yes" : "No");			break;
			case 15:	oss << (WithFBFChange()		? "Yes" : "No");			break;
			case 16:	oss << (WithFBOChange()		? "Yes" : "No");			break;
			case 17:	oss << (WithColorCorrect()	? "Yes" : "No");			break;
			case 18:	oss << (WithVidProc()		? "Yes" : "No");			break;
			case 19:	oss << (WithCustomAnc()		? "Yes" : "No");			break;
			case 20:	oss << (WithHDMIAuxData()	? "Yes" : "No");			break;
			case 21:	oss << (IsFieldMode()		? "Yes" : "No");			break;
			default:	break;
		}
	else if (inIndexNum < 22)
		oss << "---";
	return oss.str();
}


ostream & operator << (ostream & oss, const AUTOCIRCULATE_STATUS & inObj)
{
	if (!inObj.IsStopped())
		oss << ::NTV2ChannelToString(inObj.GetChannel(), true) << ": "
			<< (inObj.IsInput() ? "Input " : (inObj.IsOutput() ? "Output" : "*BAD* "))
			<< setw(12) << ::NTV2AutoCirculateStateToString(inObj.acState) << "	 "
			<< setw( 5) << inObj.GetStartFrame()
			<< setw( 6) << inObj.GetEndFrame()
			<< setw( 6) << inObj.GetActiveFrame()
			<< setw( 8) << inObj.GetProcessedFrameCount()
			<< setw( 8) << inObj.GetDroppedFrameCount()
			<< setw( 7) << inObj.GetBufferLevel()
			<< setw(10) << ::NTV2AudioSystemToString(inObj.acAudioSystem, true)
			<< setw(10) << (inObj.WithRP188()			? "+RP188"		: "-RP188")
			<< setw(10) << (inObj.WithLTC()				? "+LTC"		: "-LTC")
			<< setw(10) << (inObj.WithFBFChange()		? "+FBFchg"		: "-FBFchg")
			<< setw(10) << (inObj.WithFBOChange()		? "+FBOchg"		: "-FBOchg")
			<< setw(10) << (inObj.WithColorCorrect()	? "+ColCor"		: "-ColCor")
			<< setw(10) << (inObj.WithVidProc()			? "+VidProc"	: "-VidProc")
			<< setw(10) << (inObj.WithCustomAnc()		? "+AncData"	: "-AncData")
			<< setw(10) << (inObj.WithHDMIAuxData()		? "+HDMIAux"	: "-HDMIAux")
			<< setw(10) << (inObj.IsFieldMode()			? "+FldMode"	: "-FldMode");
	return oss;
}


NTV2SegmentedDMAInfo::NTV2SegmentedDMAInfo ()
{
	Reset ();
}


NTV2SegmentedDMAInfo::NTV2SegmentedDMAInfo (const ULWord inNumSegments, const ULWord inNumActiveBytesPerRow, const ULWord inHostBytesPerRow, const ULWord inDeviceBytesPerRow)
{
	Set (inNumSegments, inNumActiveBytesPerRow, inHostBytesPerRow, inDeviceBytesPerRow);
}


void NTV2SegmentedDMAInfo::Set (const ULWord inNumSegments, const ULWord inNumActiveBytesPerRow, const ULWord inHostBytesPerRow, const ULWord inDeviceBytesPerRow)
{
	acNumSegments = inNumSegments;
	if (acNumSegments > 1)
	{
		acNumActiveBytesPerRow	= inNumActiveBytesPerRow;
		acSegmentHostPitch		= inHostBytesPerRow;
		acSegmentDevicePitch	= inDeviceBytesPerRow;
	}
	else
		Reset ();
}


void NTV2SegmentedDMAInfo::Reset (void)
{
	acNumSegments = acNumActiveBytesPerRow = acSegmentHostPitch = acSegmentDevicePitch = 0;
}


NTV2ColorCorrectionData::NTV2ColorCorrectionData ()
	:	ccMode				(NTV2_CCMODE_INVALID),
		ccSaturationValue	(0)
{
}


NTV2ColorCorrectionData::~NTV2ColorCorrectionData ()
{
	Clear ();
}


void NTV2ColorCorrectionData::Clear (void)
{
	ccMode = NTV2_CCMODE_INVALID;
	ccSaturationValue = 0;
	ccLookupTables.Deallocate();
}


bool NTV2ColorCorrectionData::Set (const NTV2ColorCorrectionMode inMode, const ULWord inSaturation, const void * pInTableData)
{
	Clear();
	if (!NTV2_IS_VALID_COLOR_CORRECTION_MODE (inMode))
		return false;

	if (pInTableData)
		if (!ccLookupTables.CopyFrom(pInTableData, ULWord(NTV2_COLORCORRECTOR_TABLESIZE)))
			return false;
	ccMode = inMode;
	ccSaturationValue = (inMode == NTV2_CCMODE_3WAY) ? inSaturation : 0;
	return true;
}


AUTOCIRCULATE_TRANSFER::AUTOCIRCULATE_TRANSFER ()
	:	acHeader					(NTV2_TYPE_ACXFER, sizeof(AUTOCIRCULATE_TRANSFER)),
		acOutputTimeCodes			(NTV2_MAX_NUM_TIMECODE_INDEXES * sizeof (NTV2_RP188)),
		acTransferStatus			(),
		acInUserCookie				(0),
		acInVideoDMAOffset			(0),
		acInSegmentedDMAInfo		(),
		acColorCorrection			(),
		acFrameBufferFormat			(NTV2_FBF_10BIT_YCBCR),
		acFrameBufferOrientation	(NTV2_FRAMEBUFFER_ORIENTATION_TOPDOWN),
		acVidProcInfo				(),
		acVideoQuarterSizeExpand	(NTV2_QuarterSizeExpandOff),
		acPeerToPeerFlags			(0),
		acFrameRepeatCount			(1),
		acDesiredFrame				(-1),
		acRP188						(),
		acCrosspoint				(NTV2CROSSPOINT_INVALID)
{
	if (acOutputTimeCodes.GetHostPointer ())
		::memset (acOutputTimeCodes.GetHostPointer (), 0xFF, acOutputTimeCodes.GetByteCount ());
	NTV2_ASSERT_STRUCT_VALID;
}


AUTOCIRCULATE_TRANSFER::AUTOCIRCULATE_TRANSFER (ULWord * pInVideoBuffer, const ULWord inVideoByteCount, ULWord * pInAudioBuffer,
												const ULWord inAudioByteCount, ULWord * pInANCBuffer, const ULWord inANCByteCount,
												ULWord * pInANCF2Buffer, const ULWord inANCF2ByteCount)
	:	acHeader					(NTV2_TYPE_ACXFER, sizeof(AUTOCIRCULATE_TRANSFER)),
		acVideoBuffer				(pInVideoBuffer, inVideoByteCount),
		acAudioBuffer				(pInAudioBuffer, inAudioByteCount),
		acANCBuffer					(pInANCBuffer, inANCByteCount),
		acANCField2Buffer			(pInANCF2Buffer, inANCF2ByteCount),
		acOutputTimeCodes			(NTV2_MAX_NUM_TIMECODE_INDEXES * sizeof (NTV2_RP188)),
		acTransferStatus			(),
		acInUserCookie				(0),
		acInVideoDMAOffset			(0),
		acInSegmentedDMAInfo		(),
		acColorCorrection			(),
		acFrameBufferFormat			(NTV2_FBF_10BIT_YCBCR),
		acFrameBufferOrientation	(NTV2_FRAMEBUFFER_ORIENTATION_TOPDOWN),
		acVidProcInfo				(),
		acVideoQuarterSizeExpand	(NTV2_QuarterSizeExpandOff),
		acPeerToPeerFlags			(0),
		acFrameRepeatCount			(1),
		acDesiredFrame				(-1),
		acRP188						(),
		acCrosspoint				(NTV2CROSSPOINT_INVALID)
{
	if (acOutputTimeCodes.GetHostPointer ())
		::memset (acOutputTimeCodes.GetHostPointer (), 0xFF, acOutputTimeCodes.GetByteCount ());
	NTV2_ASSERT_STRUCT_VALID;
}


AUTOCIRCULATE_TRANSFER::~AUTOCIRCULATE_TRANSFER ()
{
}


bool AUTOCIRCULATE_TRANSFER::SetBuffers (ULWord * pInVideoBuffer, const ULWord inVideoByteCount,
										ULWord * pInAudioBuffer, const ULWord inAudioByteCount,
										ULWord * pInANCBuffer, const ULWord inANCByteCount,
										ULWord * pInANCF2Buffer, const ULWord inANCF2ByteCount)
{
	NTV2_ASSERT_STRUCT_VALID;
	return SetVideoBuffer (pInVideoBuffer, inVideoByteCount)
			&& SetAudioBuffer (pInAudioBuffer, inAudioByteCount)
				&& SetAncBuffers (pInANCBuffer, inANCByteCount, pInANCF2Buffer, inANCF2ByteCount);
}


bool AUTOCIRCULATE_TRANSFER::SetVideoBuffer (ULWord * pInVideoBuffer, const ULWord inVideoByteCount)
{
	NTV2_ASSERT_STRUCT_VALID;
	acVideoBuffer.Set (pInVideoBuffer, inVideoByteCount);
	return true;
}


bool AUTOCIRCULATE_TRANSFER::SetAudioBuffer (ULWord * pInAudioBuffer, const ULWord inAudioByteCount)
{
	NTV2_ASSERT_STRUCT_VALID;
	acAudioBuffer.Set (pInAudioBuffer, inAudioByteCount);
	return true;
}


bool AUTOCIRCULATE_TRANSFER::SetAncBuffers (ULWord * pInANCBuffer, const ULWord inANCByteCount, ULWord * pInANCF2Buffer, const ULWord inANCF2ByteCount)
{
	NTV2_ASSERT_STRUCT_VALID;
	acANCBuffer.Set (pInANCBuffer, inANCByteCount);
	acANCField2Buffer.Set (pInANCF2Buffer, inANCF2ByteCount);
	return true;
}

static const NTV2_RP188		INVALID_TIMECODE_VALUE;


bool AUTOCIRCULATE_TRANSFER::SetOutputTimeCodes (const NTV2TimeCodes & inValues)
{
	NTV2_ASSERT_STRUCT_VALID;
	ULWord			maxNumValues (acOutputTimeCodes.GetByteCount() / sizeof(NTV2_RP188));
	NTV2_RP188 *	pArray (reinterpret_cast<NTV2_RP188*>(acOutputTimeCodes.GetHostPointer()));
	if (!pArray)
		return false;
	if (maxNumValues > NTV2_MAX_NUM_TIMECODE_INDEXES)
		maxNumValues = NTV2_MAX_NUM_TIMECODE_INDEXES;

	for (UWord ndx (0);	 ndx < UWord(maxNumValues);	 ndx++)
	{
		const NTV2TCIndex		tcIndex (static_cast<NTV2TCIndex>(ndx));
		NTV2TimeCodesConstIter	iter	(inValues.find(tcIndex));
		pArray[ndx] = (iter != inValues.end())	?  iter->second	 :	INVALID_TIMECODE_VALUE;
	}	//	for each possible NTV2TCSource value
	return true;
}


bool AUTOCIRCULATE_TRANSFER::SetOutputTimeCode (const NTV2_RP188 & inTimeCode, const NTV2TCIndex inTCIndex)
{
	NTV2_ASSERT_STRUCT_VALID;
	ULWord			maxNumValues (acOutputTimeCodes.GetByteCount() / sizeof(NTV2_RP188));
	NTV2_RP188 *	pArray (reinterpret_cast<NTV2_RP188*>(acOutputTimeCodes.GetHostPointer()));
	if (!pArray)
		return false;
	if (maxNumValues > NTV2_MAX_NUM_TIMECODE_INDEXES)
		maxNumValues = NTV2_MAX_NUM_TIMECODE_INDEXES;
	if (!NTV2_IS_VALID_TIMECODE_INDEX(inTCIndex))
		return false;

	pArray[inTCIndex] = inTimeCode;
	return true;
}

bool AUTOCIRCULATE_TRANSFER::SetAllOutputTimeCodes (const NTV2_RP188 & inTimeCode, const bool inIncludeF2)
{
	NTV2_ASSERT_STRUCT_VALID;
	ULWord			maxNumValues	(acOutputTimeCodes.GetByteCount() / sizeof(NTV2_RP188));
	NTV2_RP188 *	pArray			(reinterpret_cast<NTV2_RP188*>(acOutputTimeCodes.GetHostPointer()));
	if (!pArray)
		return false;
	if (maxNumValues > NTV2_MAX_NUM_TIMECODE_INDEXES)
		maxNumValues = NTV2_MAX_NUM_TIMECODE_INDEXES;

	for (ULWord tcIndex(0);	 tcIndex < maxNumValues;  tcIndex++)
		if (NTV2_IS_ATC_VITC2_TIMECODE_INDEX(tcIndex))
			pArray[tcIndex] = inIncludeF2 ? inTimeCode : INVALID_TIMECODE_VALUE;
		else
			pArray[tcIndex] = inTimeCode;
	return true;
}


bool AUTOCIRCULATE_TRANSFER::SetFrameBufferFormat (const NTV2FrameBufferFormat inNewFormat)
{
	NTV2_ASSERT_STRUCT_VALID;
	if (!NTV2_IS_VALID_FRAME_BUFFER_FORMAT (inNewFormat))
		return false;
	acFrameBufferFormat = inNewFormat;
	return true;
}


void AUTOCIRCULATE_TRANSFER::Clear (void)
{
	NTV2_ASSERT_STRUCT_VALID;
	SetBuffers (AJA_NULL, 0, AJA_NULL, 0, AJA_NULL, 0, AJA_NULL, 0);
}


bool AUTOCIRCULATE_TRANSFER::EnableSegmentedDMAs (const ULWord inNumSegments, const ULWord inNumActiveBytesPerRow,
													const ULWord inHostBytesPerRow, const ULWord inDeviceBytesPerRow)
{
	NTV2_ASSERT_STRUCT_VALID;
	//	Cannot allow segmented DMAs if video buffer was self-allocated by the SDK, since the video buffer size holds the segment size (in bytes)...
	if (acVideoBuffer.IsAllocatedBySDK ())
		return false;	//	Disallow
	acInSegmentedDMAInfo.Set (inNumSegments, inNumActiveBytesPerRow, inHostBytesPerRow, inDeviceBytesPerRow);
	return true;
}


bool AUTOCIRCULATE_TRANSFER::DisableSegmentedDMAs (void)
{
	NTV2_ASSERT_STRUCT_VALID;
	acInSegmentedDMAInfo.Reset ();
	return true;
}


bool AUTOCIRCULATE_TRANSFER::SegmentedDMAsEnabled (void) const
{
	NTV2_ASSERT_STRUCT_VALID;
	return acInSegmentedDMAInfo.acNumSegments > 1;
}


bool AUTOCIRCULATE_TRANSFER::GetInputTimeCodes (NTV2TimeCodeList & outValues) const
{
	NTV2_ASSERT_STRUCT_VALID;
	return acTransferStatus.acFrameStamp.GetInputTimeCodes (outValues);
}


bool AUTOCIRCULATE_TRANSFER::GetInputTimeCode (NTV2_RP188 & outTimeCode, const NTV2TCIndex inTCIndex) const
{
	NTV2_ASSERT_STRUCT_VALID;
	return acTransferStatus.acFrameStamp.GetInputTimeCode (outTimeCode, inTCIndex);
}


bool AUTOCIRCULATE_TRANSFER::GetInputTimeCodes (NTV2TimeCodes & outTimeCodes, const NTV2Channel inSDIInput, const bool inValidOnly) const
{
	NTV2_ASSERT_STRUCT_VALID;
	return acTransferStatus.acFrameStamp.GetInputTimeCodes (outTimeCodes, inSDIInput, inValidOnly);
}


NTV2DebugLogging::NTV2DebugLogging(const bool inEnable)
	:	mHeader				(NTV2_TYPE_AJADEBUGLOGGING, sizeof (NTV2DebugLogging)),
		mSharedMemory		(inEnable ? AJADebug::GetPrivateDataLoc() : AJA_NULL,  inEnable ? AJADebug::GetPrivateDataLen() : 0)
{
}


ostream & NTV2DebugLogging::Print (ostream & inOutStream) const
{
	NTV2_ASSERT_STRUCT_VALID;
	inOutStream << mHeader << " shMem=" << mSharedMemory << " " << mTrailer;
	return inOutStream;
}



NTV2BufferLock::NTV2BufferLock()
	:	mHeader (NTV2_TYPE_AJABUFFERLOCK, sizeof(NTV2BufferLock))
{
	NTV2_ASSERT_STRUCT_VALID;
	SetFlags(0);
	SetMaxLockSize(0);
}

NTV2BufferLock::NTV2BufferLock (const NTV2Buffer & inBuffer, const ULWord inFlags)
	:	mHeader (NTV2_TYPE_AJABUFFERLOCK, sizeof(NTV2BufferLock))
{
	NTV2_ASSERT_STRUCT_VALID;
	SetBuffer(inBuffer);
	SetFlags(inFlags);
	SetMaxLockSize(0);
}

NTV2BufferLock::NTV2BufferLock(const ULWord * pInBuffer, const ULWord inByteCount, const ULWord inFlags)
	:	mHeader (NTV2_TYPE_AJABUFFERLOCK, sizeof(NTV2BufferLock))
{
	NTV2_ASSERT_STRUCT_VALID;
	SetBuffer (NTV2Buffer(pInBuffer, inByteCount));
	SetFlags (inFlags);
	SetMaxLockSize(0);
}

NTV2BufferLock::NTV2BufferLock(const ULWord64 inMaxLockSize, const ULWord inFlags)
	:	mHeader (NTV2_TYPE_AJABUFFERLOCK, sizeof(NTV2BufferLock))
{
	NTV2_ASSERT_STRUCT_VALID;
	SetBuffer (NTV2Buffer());
	SetFlags (inFlags);
	SetMaxLockSize(inMaxLockSize);
}

bool NTV2BufferLock::SetBuffer (const NTV2Buffer & inBuffer)
{	//	Just use address & length (don't deep copy)...
	NTV2_ASSERT_STRUCT_VALID;
	return mBuffer.Set (inBuffer.GetHostPointer(), inBuffer.GetByteCount());
}

ostream & NTV2BufferLock::Print (ostream & inOutStream) const
{
	NTV2_ASSERT_STRUCT_VALID;
	inOutStream << mHeader << mBuffer << " flags=" << xHEX0N(mFlags,8) << " " << mTrailer;
	return inOutStream;
}


NTV2Bitstream::NTV2Bitstream()
	:	mHeader (NTV2_TYPE_AJABITSTREAM, sizeof(NTV2Bitstream))
{
	NTV2_ASSERT_STRUCT_VALID;
}

NTV2Bitstream::NTV2Bitstream (const NTV2Buffer & inBuffer, const ULWord inFlags)
	:	mHeader (NTV2_TYPE_AJABITSTREAM, sizeof(NTV2Bitstream))
{
	NTV2_ASSERT_STRUCT_VALID;
	SetBuffer(inBuffer);
	SetFlags(inFlags);
}

NTV2Bitstream::NTV2Bitstream(const ULWord * pInBuffer, const ULWord inByteCount, const ULWord inFlags)
	:	mHeader (NTV2_TYPE_AJABITSTREAM, sizeof(NTV2Bitstream))
{
	NTV2_ASSERT_STRUCT_VALID;
	SetBuffer (NTV2Buffer(pInBuffer, inByteCount));
	SetFlags (inFlags);
}

bool NTV2Bitstream::SetBuffer (const NTV2Buffer & inBuffer)
{	//	Just use address & length (don't deep copy)...
	NTV2_ASSERT_STRUCT_VALID;
	return mBuffer.Set (inBuffer.GetHostPointer(), inBuffer.GetByteCount());
}

ostream & NTV2Bitstream::Print (ostream & inOutStream) const
{
	NTV2_ASSERT_STRUCT_VALID;
	inOutStream << mHeader << mBuffer << " flags=" << xHEX0N(mFlags,8) << " " << mTrailer;
	return inOutStream;
}

NTV2StreamChannel::NTV2StreamChannel()
	:	mHeader (NTV2_TYPE_AJASTREAMCHANNEL, sizeof(NTV2StreamChannel))
{
	NTV2_ASSERT_STRUCT_VALID;
}

ostream & NTV2StreamChannel::Print (ostream & inOutStream) const
{
	NTV2_ASSERT_STRUCT_VALID;
	inOutStream << mHeader << mChannel << " flags=" << xHEX0N(mFlags,8) << xHEX0N(mStatus, 8) << " " << mTrailer;
	return inOutStream;
}

NTV2StreamBuffer::NTV2StreamBuffer()
	:	mHeader (NTV2_TYPE_AJASTREAMBUFFER, sizeof(NTV2StreamBuffer))
{
	NTV2_ASSERT_STRUCT_VALID;
}

ostream & NTV2StreamBuffer::Print (ostream & inOutStream) const
{
	NTV2_ASSERT_STRUCT_VALID;
	inOutStream << mHeader << mChannel << " flags=" << xHEX0N(mFlags,8) << xHEX0N(mStatus, 8) << " " << mTrailer;
	return inOutStream;
}

NTV2MailBuffer::NTV2MailBuffer()
	:	mHeader (NTV2_TYPE_AJAMAILBUFFER, sizeof(NTV2MailBuffer))
{
	NTV2_ASSERT_STRUCT_VALID;
}

ostream & NTV2MailBuffer::Print (ostream & inOutStream) const
{
	NTV2_ASSERT_STRUCT_VALID;
	inOutStream << mHeader << mChannel << " flags=" << xHEX0N(mFlags,8) << xHEX0N(mStatus, 8) << " " << mTrailer;
	return inOutStream;
}

NTV2GetRegisters::NTV2GetRegisters (const NTV2RegNumSet & inRegisterNumbers)
	:	mHeader				(NTV2_TYPE_GETREGS, sizeof(NTV2GetRegisters)),
		mInNumRegisters		(ULWord (inRegisterNumbers.size ())),
		mOutNumRegisters	(0)
{
	NTV2_ASSERT_STRUCT_VALID;
	ResetUsing (inRegisterNumbers);
}


NTV2GetRegisters::NTV2GetRegisters (NTV2RegisterReads & inRegReads)
	:	mHeader				(NTV2_TYPE_GETREGS, sizeof(NTV2GetRegisters)),
		mInNumRegisters		(ULWord (inRegReads.size ())),
		mOutNumRegisters	(0)
{
	NTV2_ASSERT_STRUCT_VALID;
	ResetUsing (inRegReads);
}


bool NTV2GetRegisters::ResetUsing (const NTV2RegNumSet & inRegisterNumbers)
{
	NTV2_ASSERT_STRUCT_VALID;
	mInNumRegisters = ULWord(inRegisterNumbers.size());
	mOutNumRegisters = 0;
	bool result	(	mInRegisters.Allocate(mInNumRegisters * sizeof(ULWord))
					&&	mOutGoodRegisters.Allocate(mInNumRegisters * sizeof(ULWord))
					&&	mOutValues.Allocate(mInNumRegisters * sizeof(ULWord)));
	if (!result)
		return false;
	mInRegisters.Fill(ULWord(0));	mOutGoodRegisters.Fill(ULWord(0));	mOutValues.Fill(ULWord(0));

	ULWord * pRegArray(mInRegisters);
	if (!pRegArray)
		return false;

	ULWord ndx(0);
	for (NTV2RegNumSetConstIter iter(inRegisterNumbers.begin());  iter != inRegisterNumbers.end();  ++iter)
		pRegArray[ndx++] = *iter;
	return (ndx * sizeof(ULWord)) == mInRegisters.GetByteCount();
}

bool NTV2GetRegisters::GetRequestedRegisterNumbers (NTV2RegNumSet & outRegNums) const
{
	outRegNums.clear();
	if (!mInNumRegisters)
		return true;	//	None requested
	if (!mInRegisters)
		return false;	//	Empty/NULL reg num buffer
	if (mInRegisters.GetByteCount()/4 < mInNumRegisters)
		return false;	//	Sanity check failed:  Reg num buffer too small

	const ULWord *	pRegNums(mInRegisters);
	for (ULWord ndx(0);  ndx < mInNumRegisters;  ndx++)
		if (outRegNums.find(pRegNums[ndx]) == outRegNums.end())
			outRegNums.insert(pRegNums[ndx]);
	return true;
}


bool NTV2GetRegisters::GetGoodRegisters (NTV2RegNumSet & outGoodRegNums) const
{
	NTV2_ASSERT_STRUCT_VALID;
	outGoodRegNums.clear();
	if (!mOutGoodRegisters)
		return false;		//	Empty/NULL 'mOutGoodRegisters' array!
	if (!mOutNumRegisters)
		return false;		//	The driver says zero successfully read!
	if (mOutNumRegisters > mInNumRegisters)
		return false;		//	Sanity check failed:  mOutNumRegisters must be less than or equal to mInNumRegisters!

	const ULWord *	pRegArray (mOutGoodRegisters);
	for (ULWord ndx(0);  ndx < mOutNumRegisters;  ndx++)
		outGoodRegNums.insert(pRegArray[ndx]);
	return true;
}

bool NTV2GetRegisters::GetBadRegisters (NTV2RegNumSet & outBadRegNums) const
{
	NTV2_ASSERT_STRUCT_VALID;
	outBadRegNums.clear();
	NTV2RegNumSet reqRegNums, goodRegNums;
	if (!GetRequestedRegisterNumbers(reqRegNums))
		return false;
	if (!GetGoodRegisters(goodRegNums))
		return false;
	if (reqRegNums == goodRegNums)
		return true;	//	Requested reg nums identical to those that were read successfully

	//	Subtract goodRegNums from reqRegNums...
	std::set_difference (reqRegNums.begin(), reqRegNums.end(),
						goodRegNums.begin(), goodRegNums.end(),
						std::inserter(outBadRegNums, outBadRegNums.begin()));
	return true;
}

bool NTV2GetRegisters::PatchRegister (const ULWord inRegNum, const ULWord inValue)
{
	if (!mOutGoodRegisters)
		return false;		//	Empty/null 'mOutGoodRegisters' array!
	if (!mOutNumRegisters)
		return false;		//	Driver says zero successfully read!
	if (mOutNumRegisters > mInNumRegisters)
		return false;		//	Sanity check failed:  mOutNumRegisters must be less than or equal to mInNumRegisters!
	if (!mOutValues)
		return false;		//	Empty/null 'mOutValues' array!
	if (mOutGoodRegisters.GetByteCount() != mOutValues.GetByteCount())
		return false;		//	Sanity check failed:  These sizes should match
	const ULWord *	pRegArray	(mOutGoodRegisters);
	ULWord *		pValArray	(mOutValues);
	for (ULWord ndx(0);  ndx < mOutNumRegisters;  ndx++)
		if (pRegArray[ndx] == inRegNum)
		{
			pValArray[ndx] = inValue;
			return true;
		}
	return false;	//	Not found
}


bool NTV2GetRegisters::GetRegisterValues (NTV2RegisterValueMap & outValues) const
{
	NTV2_ASSERT_STRUCT_VALID;
	outValues.clear ();
	if (!mOutGoodRegisters)
		return false;		//	Empty/null 'mOutGoodRegisters' array!
	if (!mOutNumRegisters)
		return false;		//	Driver says zero successfully read!
	if (mOutNumRegisters > mInNumRegisters)
		return false;		//	Sanity check failed:  mOutNumRegisters must be less than or equal to mInNumRegisters!
	if (!mOutValues)
		return false;		//	Empty/null 'mOutValues' array!
	if (mOutGoodRegisters.GetByteCount() != mOutValues.GetByteCount())
		return false;		//	Sanity check failed:  These sizes should match

	const ULWord *	pRegArray	(mOutGoodRegisters);
	const ULWord *	pValArray	(mOutValues);
	for (ULWord ndx(0);  ndx < mOutNumRegisters;  ndx++)
	{
		outValues [pRegArray[ndx]] = pValArray[ndx];
#if 0	//	Fake KONAIP25G from C4412G (see also CNTV2XXXXDriverInterface::ReadRegister):
	if (pRegArray[ndx] == kRegBoardID  &&  pValArray[ndx] == DEVICE_ID_CORVID44_8K)
		outValues [pRegArray[ndx]] = DEVICE_ID_KONAIP_25G;
	else if (pRegArray[ndx] == kRegReserved83  ||  pRegArray[ndx] == kRegLPRJ45IP)
		outValues [pRegArray[ndx]] = 0x0A03FAD9;	//	Local IPv4    10.3.250.217
#endif	//	0
	}
	return true;
}


bool NTV2GetRegisters::GetRegisterValues (NTV2RegisterReads & outValues) const
{
	NTV2RegisterValueMap regValMap;
	if (!GetRegisterValues(regValMap))
		return false;

	if (outValues.empty())
	{
		for (NTV2RegValueMapConstIter it(regValMap.begin());  it != regValMap.end();  ++it)
		{
			NTV2RegInfo regInfo(/*regNum*/it->first, /*regVal*/it->second);
#if 0		//	Fake KONAIP25G from C4412G (see also CNTV2XXXXDriverInterface::ReadRegister):
			if (regInfo.regNum() == kRegBoardID  &&  regInfo.value() == DEVICE_ID_CORVID44_8K)
				regInfo.setValue(DEVICE_ID_KONAIP_25G);
			else if (regInfo.regNum() == kRegReserved83  ||  regInfo.regNum() == kRegLPRJ45IP)
				regInfo.setValue(0x0A03FAD9);	//	Local IPv4    10.3.250.217
#endif	//	0
			outValues.push_back(regInfo);
		}
		return true;
	}
	else
	{
		uint32_t missingTally(0);
		for (NTV2RegisterReadsIter it (outValues.begin());	it != outValues.end();	++it)
		{
			NTV2RegValueMapConstIter mapIter(regValMap.find(it->registerNumber));
			if (mapIter != regValMap.end())
				it->registerValue = mapIter->second;
			else
				missingTally++; //	Missing register
				
#if 0		//	Fake KONAIP25G from C4412G (see also CNTV2XXXXDriverInterface::ReadRegister):
			if (it->registerNumber == kRegBoardID  &&  it->registerValue == DEVICE_ID_CORVID44_8K)
				it->registerValue = DEVICE_ID_KONAIP_25G;
			else if (it->registerNumber == kRegReserved83  ||  it->registerNumber == kRegLPRJ45IP)
				it->registerValue = 0x0A03FAD9;	//	Local IPv4    10.3.250.217
#endif	//	0
		}
		return !missingTally;
	}
}


ostream & NTV2GetRegisters::Print (ostream & inOutStream) const
{
	inOutStream << mHeader << ", numRegs=" << mInNumRegisters << ", inRegs=" << mInRegisters << ", outNumGoodRegs=" << mOutNumRegisters
				<< ", outGoodRegs=" << mOutGoodRegisters << ", outValues=" << mOutValues << ", " << mTrailer;
	return inOutStream;
}


NTV2SetRegisters::NTV2SetRegisters (const NTV2RegisterWrites & inRegWrites)
	:	mHeader				(NTV2_TYPE_SETREGS, sizeof(NTV2SetRegisters)),
		mInNumRegisters		(ULWord(inRegWrites.size())),
		mOutNumFailures		(0)
{
	ResetUsing(inRegWrites);
}


bool NTV2SetRegisters::ResetUsing (const NTV2RegisterWrites & inRegWrites)
{
	NTV2_ASSERT_STRUCT_VALID;
	mInNumRegisters = ULWord(inRegWrites.size());
	mOutNumFailures = 0;
	const bool	result	(mInRegInfos.Allocate (mInNumRegisters * sizeof(NTV2RegInfo))
						&& mOutBadRegIndexes.Allocate (mInNumRegisters * sizeof(UWord)));
	if (!result)
		return false;

	ULWord			ndx				(0);
	NTV2RegInfo *	pRegInfoArray	(mInRegInfos);
	UWord *			pBadRegIndexes	(mOutBadRegIndexes);

	for (NTV2RegisterWritesConstIter it(inRegWrites.begin());  it != inRegWrites.end();  ++it)
	{
		if (pBadRegIndexes)
			pBadRegIndexes[ndx] = 0;
		if (pRegInfoArray)
			pRegInfoArray[ndx++] = *it;
	}
	NTV2_ASSERT((ndx * sizeof(NTV2RegInfo)) == mInRegInfos.GetByteCount());
	NTV2_ASSERT((ndx * sizeof(UWord)) == mOutBadRegIndexes.GetByteCount());
	return result;
}


bool NTV2SetRegisters::GetFailedRegisterWrites (NTV2RegisterWrites & outFailedRegWrites) const
{
	NTV2_ASSERT_STRUCT_VALID;
	outFailedRegWrites.clear();
	return true;
}

bool NTV2SetRegisters::GetRequestedRegisterWrites	(NTV2RegWrites & outRegWrites) const
{
	outRegWrites.clear();
	if (!mInNumRegisters)
		return false;
	if (!mInRegInfos)
		return false;

	outRegWrites.reserve(size_t(mInNumRegisters));
	const NTV2RegInfo *	pRegInfos(mInRegInfos);
	for (ULWord ndx(0);  ndx < mInNumRegisters;  ndx++)
		outRegWrites.push_back(pRegInfos[ndx]);
	return true;
}

ostream & NTV2SetRegisters::Print (ostream & oss) const
{
	NTV2_ASSERT_STRUCT_VALID;
	oss << mHeader << ": numRegs=" << mInNumRegisters << " inRegInfos=" << mInRegInfos << " numFailures=" << DEC(mOutNumFailures)
		<< " outBadRegIndexes=" << mOutBadRegIndexes << ": " << mTrailer;
	const UWord *		pBadRegIndexes		(mOutBadRegIndexes);
	const UWord			maxNumBadRegIndexes	(UWord(mOutBadRegIndexes.GetByteCount() / sizeof(UWord)));
	const NTV2RegInfo * pRegInfoArray		(mInRegInfos);
	const UWord			maxNumRegInfos		(UWord(mInRegInfos.GetByteCount() / sizeof(NTV2RegInfo)));
	if (pBadRegIndexes  &&  maxNumBadRegIndexes  &&  pRegInfoArray  &&  maxNumRegInfos  &&  mOutNumFailures)
	{
		oss << endl;
		for (UWord num(0);  num < maxNumBadRegIndexes;  num++)
		{
			const UWord badRegIndex (pBadRegIndexes[num]);
			if (badRegIndex < maxNumRegInfos)
			{
				const NTV2RegInfo & badRegInfo (pRegInfoArray[badRegIndex]);
				oss << "Failure " << num << ":	" << badRegInfo << endl;
			}
		}
	}
	return oss;
}


bool NTV2RegInfo::operator < (const NTV2RegInfo & inRHS) const
{
	typedef std::pair <ULWord, ULWord>			ULWordPair;
	typedef std::pair <ULWordPair, ULWordPair>	ULWordPairs;
	const ULWordPairs	rhs (ULWordPair (inRHS.registerNumber, inRHS.registerValue), ULWordPair (inRHS.registerMask, inRHS.registerShift));
	const ULWordPairs	mine(ULWordPair (registerNumber, registerValue), ULWordPair (registerMask, registerShift));
	return mine < rhs;
}

ostream & NTV2RegInfo::Print (ostream & oss, const bool inAsCode) const
{
	if (inAsCode)
		return PrintCode(oss);
	const string regName (CNTV2RegisterExpert::GetDisplayName(NTV2RegisterNumber(registerNumber)));
	oss << "[" << regName << "|" << DEC(registerNumber) << ": val=" << xHEX0N(registerValue,8);
	if (registerMask != 0xFFFFFFFF)
		oss << " msk=" << xHEX0N(registerMask,8);
	if (registerShift)
		oss << " shf=" << DEC(registerShift);
	return oss << "]";
}

ostream & NTV2RegInfo::PrintCode (ostream & oss, const int inRadix, const NTV2DeviceID inDeviceID) const
{
	const string regName (CNTV2RegisterExpert::GetDisplayName(NTV2RegisterNumber(registerNumber)));
	const bool readOnly (CNTV2RegisterExpert::IsReadOnly(registerNumber));
	const bool badName (regName.find(' ') != string::npos);
	if (readOnly)
		oss << "//\t";
	oss << "theDevice.WriteRegister (";
	if (badName)
		oss << DEC(registerNumber);
	else
		oss << regName;
	switch (inRadix)
	{
		case 2:		oss << ", " << BIN032(registerValue);	break;
		case 8:		oss << ", " << OCT(registerValue);		break;
		case 10:	oss << ", " << DEC(registerValue);		break;
		default:	oss << ", " << xHEX0N(registerValue,8); break;
	}
	if (registerMask != 0xFFFFFFFF)
		switch (inRadix)
		{
			case 2:		oss << ", " << BIN032(registerMask);	break;
			case 8:		oss << ", " << OCT(registerMask);		break;
			case 10:	oss << ", " << DEC(registerMask);		break;
			default:	oss << ", " << xHEX0N(registerMask,8);	break;
		}
	if (registerShift)
		oss << ", " << DEC(registerShift);
	oss << ");	// ";
	if (badName)
		oss << regName;
	else
		oss << "Reg " << DEC(registerNumber);
	//	Decode the reg value...
	string info(CNTV2RegisterExpert::GetDisplayValue(registerNumber, registerValue, inDeviceID));
	if (!info.empty())	//	and add to end of comment
		oss << "  // " << aja::replace(info, "\n", ", ");
	return oss;
}


ostream & NTV2PrintULWordVector (const NTV2ULWordVector & inObj, ostream & inOutStream)
{
	for (NTV2ULWordVector::const_iterator it(inObj.begin());  it != inObj.end();  ++it)
		inOutStream << " " << HEX0N(*it,8);
	return inOutStream;
}


ostream & NTV2PrintChannelList (const NTV2ChannelList & inObj, const bool inCompact, ostream & inOutStream)
{
	inOutStream << (inCompact ? "Ch[" : "[");
	for (NTV2ChannelListConstIter it(inObj.begin());  it != inObj.end();  )
	{
		if (inCompact)
			inOutStream << DEC(*it+1);
		else
			inOutStream << ::NTV2ChannelToString(*it);
		if (++it != inObj.end())
			inOutStream << (inCompact ? "|" : ",");
	}
	return inOutStream << "]";
}

string NTV2ChannelListToStr (const NTV2ChannelList & inObj, const bool inCompact)
{	ostringstream oss;
	::NTV2PrintChannelList (inObj, inCompact, oss);
	return oss.str();
}

ostream & NTV2PrintChannelSet (const NTV2ChannelSet & inObj, const bool inCompact, ostream & inOutStream)
{
	inOutStream << (inCompact ? "Ch{" : "{");
	for (NTV2ChannelSetConstIter it(inObj.begin());	 it != inObj.end();	 )
	{
		if (inCompact)
			inOutStream << DEC(*it+1);
		else
			inOutStream << ::NTV2ChannelToString(*it);
		if (++it != inObj.end())
			inOutStream << (inCompact ? "|" : ",");
	}
	return inOutStream << "}";
}

string NTV2ChannelSetToStr (const NTV2ChannelSet & inObj, const bool inCompact)
{	ostringstream oss;
	::NTV2PrintChannelSet (inObj, inCompact, oss);
	return oss.str();
}

NTV2ChannelSet NTV2MakeChannelSet (const NTV2Channel inFirstChannel, const UWord inNumChannels)
{
	NTV2ChannelSet result;
	for (NTV2Channel ch(inFirstChannel);  ch < NTV2Channel(inFirstChannel+inNumChannels);  ch = NTV2Channel(ch+1))
		if (NTV2_IS_VALID_CHANNEL(ch))
			result.insert(ch);
	return result;
}

NTV2ChannelSet NTV2MakeChannelSet (const NTV2ChannelList inChannels)
{
	NTV2ChannelSet result;
	for (NTV2ChannelListConstIter it(inChannels.begin());  it != inChannels.end();	++it)
		result.insert(*it);
	return result;
}

NTV2ChannelList NTV2MakeChannelList (const NTV2Channel inFirstChannel, const UWord inNumChannels)
{
	NTV2ChannelList result;
	for (NTV2Channel ch(inFirstChannel);  ch < NTV2Channel(inFirstChannel+inNumChannels);  ch = NTV2Channel(ch+1))
		if (NTV2_IS_VALID_CHANNEL(ch))
			result.push_back(ch);
	return result;
}

NTV2ChannelList NTV2MakeChannelList (const NTV2ChannelSet inChannels)
{
	NTV2ChannelList result;
	for (NTV2ChannelSetConstIter it(inChannels.begin());  it != inChannels.end();  ++it)
		result.push_back(*it);
	return result;
}

ostream & NTV2PrintAudioSystemSet (const NTV2AudioSystemSet & inObj, const bool inCompact, std::ostream & inOutStream)
{
	inOutStream << (inCompact ? "AudSys{" : "{");
	for (NTV2AudioSystemSetConstIter it(inObj.begin());	 it != inObj.end();	 )
	{
		if (inCompact)
			inOutStream << DEC(*it+1);
		else
			inOutStream << ::NTV2AudioSystemToString(*it);
		if (++it != inObj.end())
			inOutStream << (inCompact ? "|" : ",");
	}
	return inOutStream << "}";
}

string NTV2AudioSystemSetToStr (const NTV2AudioSystemSet & inObj, const bool inCompact)
{	ostringstream oss;
	::NTV2PrintAudioSystemSet (inObj, inCompact, oss);
	return oss.str();
}

NTV2AudioSystemSet NTV2MakeAudioSystemSet (const NTV2AudioSystem inFirstAudioSystem, const UWord inCount)
{
	NTV2AudioSystemSet result;
	for (NTV2AudioSystem audSys(inFirstAudioSystem);  audSys < NTV2AudioSystem(inFirstAudioSystem+inCount);  audSys = NTV2AudioSystem(audSys+1))
		if (NTV2_IS_VALID_AUDIO_SYSTEM(audSys))
			result.insert(audSys);
	return result;
}

NTV2RegNumSet GetRegisterNumbers (const NTV2RegReads & inRegInfos)
{
	NTV2RegNumSet result;
	for (NTV2RegisterReadsConstIter it(inRegInfos.begin());  it != inRegInfos.end();  ++it)
		if (result.find(it->registerNumber) == result.end())
			result.insert(it->registerNumber);
	return result;
}

NTV2RegisterReadsConstIter FindFirstMatchingRegisterNumber (const uint32_t inRegNum, const NTV2RegisterReads & inRegInfos)
{
	for (NTV2RegisterReadsConstIter iter(inRegInfos.begin());  iter != inRegInfos.end();  ++iter)	//	Ugh -- linear search
		if (iter->registerNumber == inRegNum)
			return iter;
	return inRegInfos.end();
}


ostream & operator << (std::ostream & inOutStream, const NTV2RegInfo & inObj)
{
	return inObj.Print(inOutStream);
}


ostream & operator << (ostream & inOutStream, const NTV2RegisterWrites & inObj)
{
	inOutStream << inObj.size () << " regs:" << endl;
	for (NTV2RegisterWritesConstIter iter (inObj.begin ());	 iter != inObj.end ();	++iter)
		inOutStream << *iter << endl;
	return inOutStream;
}


NTV2BankSelGetSetRegs::NTV2BankSelGetSetRegs (const NTV2RegInfo & inBankSelect, const NTV2RegInfo & inOutRegInfo, const bool inDoWrite)
	:	mHeader			(NTV2_TYPE_BANKGETSET, sizeof (NTV2BankSelGetSetRegs)),
		mIsWriting		(inDoWrite),			//	Default to reading
		mInBankInfos	(sizeof(NTV2RegInfo)),	//	Room for one bank select
		mInRegInfos		(sizeof(NTV2RegInfo))	//	Room for one register read or write
{
	NTV2RegInfo * pRegInfo (mInBankInfos);
	if (pRegInfo)
		*pRegInfo = inBankSelect;	//	Store bank select regInfo
	pRegInfo = mInRegInfos;
	if (pRegInfo)
		*pRegInfo = inOutRegInfo;	//	Store regInfo
	NTV2_ASSERT_STRUCT_VALID;
}


NTV2RegInfo NTV2BankSelGetSetRegs::GetRegInfo (const UWord inIndex0) const
{
	NTV2_ASSERT_STRUCT_VALID;
	NTV2RegInfo result;
	if (mInRegInfos)
	{
		const NTV2RegInfo * pRegInfos (mInRegInfos);
		const ULWord maxNum (mInRegInfos.GetByteCount() / ULWord(sizeof(NTV2RegInfo)));
		if (ULWord(inIndex0) < maxNum)
			result = pRegInfos[inIndex0];
	}
	return result;
}


ostream & NTV2BankSelGetSetRegs::Print (ostream & oss) const
{
	NTV2_ASSERT_STRUCT_VALID;
	const NTV2RegInfo * pBankRegInfo (mInBankInfos);
	const NTV2RegInfo * pRegInfo (mInRegInfos);
	oss << mHeader << (mIsWriting ? " WRIT" : " READ") << " bankReg=";
	if (mInBankInfos) oss << *pBankRegInfo;	else oss << "-";
	oss << " regInfos=";
	if (mInRegInfos) oss << *pRegInfo;	else oss << "-";
	return oss;
}


NTV2VirtualData::NTV2VirtualData (const ULWord inTag, const void* inVirtualData, const size_t inVirtualDataSize, const bool inDoWrite)
	:	mHeader			(NTV2_TYPE_VIRTUAL_DATA_RW, sizeof (NTV2VirtualData)),
		mTag			(inTag),								//	setup tag
		mIsWriting		(inDoWrite),							//	setup write/read
		mVirtualData	(inVirtualData, inVirtualDataSize)		//	setup virtual data
{
	NTV2_ASSERT_STRUCT_VALID;
}


ostream & NTV2VirtualData::Print (ostream & inOutStream) const
{
	NTV2_ASSERT_STRUCT_VALID;
	inOutStream << mHeader << ", mTag=" << mTag << ", mIsWriting=" << mIsWriting;
	return inOutStream;
}

using namespace ntv2nub;

	/*********************************************************************************************************************
		RPC ENCODE/DECODE FUNCTIONS
	*********************************************************************************************************************/
	#define AsU8Ref(_x_)	reinterpret_cast<uint8_t&>(_x_)
	#define AsU16Ref(_x_)	reinterpret_cast<uint16_t&>(_x_)
	#define AsU32Ref(_x_)	reinterpret_cast<uint32_t&>(_x_)
	#define AsU64Ref(_x_)	reinterpret_cast<uint64_t&>(_x_)

	bool NTV2_HEADER::RPCEncode (UByteSequence & outBlob)
	{
		PUSHU32(fHeaderTag, outBlob);							//	ULWord		fHeaderTag
		PUSHU32(fType, outBlob);								//	ULWord		fType
		PUSHU32(fHeaderVersion, outBlob);						//	ULWord		fHeaderVersion
		PUSHU32(fVersion, outBlob);								//	ULWord		fVersion
		PUSHU32(fSizeInBytes, outBlob);							//	ULWord		fSizeInBytes
		PUSHU32(fPointerSize, outBlob);							//	ULWord		fPointerSize
		PUSHU32(fOperation, outBlob);							//	ULWord		fOperation
		PUSHU32(fResultStatus, outBlob);						//	ULWord		fResultStatus
		return true;
	}

	bool NTV2_HEADER::RPCDecode (const UByteSequence & inBlob, size_t & inOutIndex)
	{	uint32_t v32(0);
		POPU32(fHeaderTag, inBlob, inOutIndex);					//	ULWord		fHeaderTag
		POPU32(fType, inBlob, inOutIndex);						//	ULWord		fType
		POPU32(fHeaderVersion, inBlob, inOutIndex);				//	ULWord		fHeaderVersion
		POPU32(fVersion, inBlob, inOutIndex);					//	ULWord		fVersion
		// Do not decode fSizeInBytes, use native size because size can vary based on OS specific struct padding
		POPU32(v32, inBlob, inOutIndex);						//	ULWord		fSizeInBytes - dummy read
		POPU32(fPointerSize, inBlob, inOutIndex);				//	ULWord		fPointerSize
		POPU32(fOperation, inBlob, inOutIndex);					//	ULWord		fOperation
		POPU32(fResultStatus, inBlob, inOutIndex);				//	ULWord		fResultStatus
		return true;
	}

	bool NTV2_TRAILER::RPCEncode (UByteSequence & outBlob)
	{
		PUSHU32(fTrailerVersion, outBlob);						//	ULWord		fTrailerVersion
		PUSHU32(fTrailerTag, outBlob);							//	ULWord		fTrailerTag
		return true;
	}

	bool NTV2_TRAILER::RPCDecode (const UByteSequence & inBlob, size_t & inOutIndex)
	{
		POPU32(fTrailerVersion, inBlob, inOutIndex);			//	ULWord		fTrailerVersion
		POPU32(fTrailerTag, inBlob, inOutIndex);				//	ULWord		fTrailerTag
		return true;
	}


	bool NTV2Buffer::RPCEncode (UByteSequence & outBlob, bool fillBuffer)
	{
		PUSHU32(fByteCount, outBlob);							//	ULWord		fByteCount
		PUSHU32(fFlags, outBlob);								//	ULWord		fFlags
		if (!IsNULL() && fillBuffer)
			AppendU8s(outBlob);	//	NOTE: My buffer content should already have been made BigEndian, if necessary
		return true;
	}

	bool NTV2Buffer::RPCDecode (const UByteSequence & inBlob, size_t & inOutIndex, bool fillBuffer)
	{
		ULWord byteCount(0), flags(0);
		POPU32(byteCount, inBlob, inOutIndex);					//	ULWord		fByteCount
		POPU32(flags, inBlob, inOutIndex);						//	ULWord		fFlags
		if (!Allocate(byteCount, flags & NTV2Buffer_PAGE_ALIGNED))
			return false;
		if (fillBuffer)
		{
			if ((inOutIndex + byteCount) > inBlob.size())
				return false;	//	past end of inBlob
			::memcpy(GetHostPointer(), inBlob.data() + inOutIndex, byteCount); //	Caller is responsible for byte-swapping if needed
			inOutIndex += byteCount;
		}	
		return true;
	}

	// Created for DMATransfer, removed Allocate().
	bool NTV2Buffer::RPCDecodeNoAllocate (const UByteSequence & inBlob, size_t & inOutIndex)
	{
		ULWord byteCount(0), flags(0);
		POPU32(byteCount, inBlob, inOutIndex);					//	ULWord		fByteCount
		POPU32(flags, inBlob, inOutIndex);						//	ULWord		fFlags
		if ((inOutIndex + byteCount) > inBlob.size())
			return false;	//	past end of inBlob

		::memcpy(GetHostPointer(), inBlob.data() + inOutIndex, byteCount); //	Caller is responsible for byte-swapping if needed
		inOutIndex += byteCount;
		return true;
	}

	bool NTV2GetRegisters::RPCEncodeClient (UByteSequence & outBlob)
	{
		const size_t totBytes	(mHeader.GetSizeInBytes()	//	Header + natural size of all structs/fields inbetween + Trailer
								+ mInRegisters.GetByteCount() + mOutGoodRegisters.GetByteCount() + mOutValues.GetByteCount());	//	NTV2Buffer fields
		if (outBlob.capacity() < totBytes)
			outBlob.reserve(totBytes);
		if (!NTV2HostIsBigEndian)
		{	//	All of my NTV2Buffers store arrays of ULWords that must be BigEndian BEFORE encoding into outBlob...
			mInRegisters.ByteSwap32();
		}
		bool ok = mHeader.RPCEncode(outBlob);								//	NTV2_HEADER		mHeader
		PUSHU32(mInNumRegisters, outBlob);									//		ULWord			mInNumRegisters
		ok &= mInRegisters.RPCEncode(outBlob, /*fillbuffer=*/true);			//		NTV2Buffer		mInRegisters
		PUSHU32(mOutNumRegisters, outBlob);									//		ULWord			mOutNumRegisters
		ok &= mOutGoodRegisters.RPCEncode(outBlob, /*fillbuffer=*/false)	//		NTV2Buffer		mOutGoodRegisters
			&& mOutValues.RPCEncode(outBlob, /*fillbuffer=*/false)			//		NTV2Buffer		mOutValues
			&& mTrailer.RPCEncode(outBlob);									//	NTV2_TRAILER	mTrailer
		if (!NTV2HostIsBigEndian  &&  !ok)
		{	//	FAILED:  Un-byteswap NTV2Buffer data...
			mInRegisters.ByteSwap32();
		}
		return ok;
	}

	bool NTV2GetRegisters::RPCDecodeServer (const UByteSequence & inBlob, size_t & inOutIndex)
	{
		bool ok = mHeader.RPCDecode(inBlob, inOutIndex);								//	NTV2_HEADER		mHeader
		if (!ok) return false;
		POPU32(mInNumRegisters, inBlob, inOutIndex);									//		ULWord			mInNumRegisters
		ok &= mInRegisters.RPCDecode(inBlob, inOutIndex, /*fillbuffer=*/true);			//		NTV2Buffer		mInRegisters
		POPU32(mOutNumRegisters, inBlob, inOutIndex);									//		ULWord			mOutNumRegisters
		ok &= mOutGoodRegisters.RPCDecode(inBlob, inOutIndex, /*fillbuffer=*/false);	//		NTV2Buffer		mOutGoodRegisters
		ok &= mOutValues.RPCDecode(inBlob, inOutIndex, /*fillbuffer=*/false);			//		NTV2Buffer		mOutValues
		ok &= mTrailer.RPCDecode(inBlob, inOutIndex);									//	NTV2_TRAILER	mTrailer
		if (!NTV2HostIsBigEndian)
		{	//	Re-byteswap NTV2Buffer data after decoding...
			mInRegisters.ByteSwap32();
		}
		return ok;
	}

	bool NTV2GetRegisters::RPCEncodeServer (UByteSequence & outBlob)
	{
		const size_t totBytes	(mHeader.GetSizeInBytes()	//	Header + natural size of all structs/fields inbetween + Trailer
								+ mInRegisters.GetByteCount() + mOutGoodRegisters.GetByteCount() + mOutValues.GetByteCount());	//	NTV2Buffer fields
		if (outBlob.capacity() < totBytes)
			outBlob.reserve(totBytes);
		if (!NTV2HostIsBigEndian)
		{	//	All of my NTV2Buffers store arrays of ULWords that must be BigEndian BEFORE encoding into outBlob...
			mOutGoodRegisters.ByteSwap32();
			mOutValues.ByteSwap32();
		}
		bool ok = mHeader.RPCEncode(outBlob);							//	NTV2_HEADER		mHeader
		PUSHU32(mInNumRegisters, outBlob);								//		ULWord			mInNumRegisters
		ok &= mInRegisters.RPCEncode(outBlob, /*fillbuffer=*/false);	//		NTV2Buffer		mInRegisters
		PUSHU32(mOutNumRegisters, outBlob);								//		ULWord			mOutNumRegisters
		ok &= mOutGoodRegisters.RPCEncode(outBlob, /*fillbuffer=*/true)	//		NTV2Buffer		mOutGoodRegisters
			&& mOutValues.RPCEncode(outBlob, /*fillbuffer=*/true)		//		NTV2Buffer		mOutValues
			&& mTrailer.RPCEncode(outBlob);								//	NTV2_TRAILER	mTrailer
		if (!NTV2HostIsBigEndian  &&  !ok)
		{	//	FAILED:  Un-byteswap NTV2Buffer data...
			mOutGoodRegisters.ByteSwap32();
			mOutValues.ByteSwap32();
		}
		return ok;
	}

	bool NTV2GetRegisters::RPCDecodeClient (const UByteSequence & inBlob, size_t & inOutIndex)
	{
		bool ok = mHeader.RPCDecode(inBlob, inOutIndex);							//	NTV2_HEADER		mHeader
		if (!ok) return false;
		POPU32(mInNumRegisters, inBlob, inOutIndex);								//		ULWord			mInNumRegisters
		ok &= mInRegisters.RPCDecode(inBlob, inOutIndex, /*fillbuffer=*/false);		//		NTV2Buffer		mInRegisters
		POPU32(mOutNumRegisters, inBlob, inOutIndex);								//		ULWord			mOutNumRegisters
		ok &= mOutGoodRegisters.RPCDecode(inBlob, inOutIndex, /*fillbuffer=*/true);	//		NTV2Buffer		mOutGoodRegisters
		ok &= mOutValues.RPCDecode(inBlob, inOutIndex, /*fillbuffer=*/true);		//		NTV2Buffer		mOutValues
		ok &= mTrailer.RPCDecode(inBlob, inOutIndex);								//	NTV2_TRAILER	mTrailer
		if (!NTV2HostIsBigEndian)
		{	//	Re-byteswap NTV2Buffer data after decoding...
			mOutGoodRegisters.ByteSwap32();
			mOutValues.ByteSwap32();
		}
		return ok;
	}

	bool NTV2SetRegisters::RPCEncode (UByteSequence & outBlob)
	{
		const size_t totBytes	(mHeader.GetSizeInBytes()	//	Header + natural size of all structs/fields inbetween + Trailer
								+ mInRegInfos.GetByteCount() + mOutBadRegIndexes.GetByteCount());	//	NTV2Buffer fields
		if (outBlob.capacity() < totBytes)
			outBlob.reserve(totBytes);
		if (!NTV2HostIsBigEndian)
		{	//	All of my NTV2Buffers store arrays of ULWords that must be BigEndian BEFORE encoding into outBlob...
			mInRegInfos.ByteSwap32();
			mOutBadRegIndexes.ByteSwap32();
		}
		bool ok = mHeader.RPCEncode(outBlob);					//	NTV2_HEADER		mHeader
		PUSHU32(mInNumRegisters, outBlob);						//		ULWord			mInNumRegisters
		ok &= mInRegInfos.RPCEncode(outBlob);					//		NTV2Buffer		mInRegInfos
		PUSHU32(mOutNumFailures, outBlob);						//		ULWord			mOutNumFailures
		ok &= mOutBadRegIndexes.RPCEncode(outBlob)				//		NTV2Buffer		mOutBadRegIndexes
			&& mTrailer.RPCEncode(outBlob);						//	NTV2_TRAILER	mTrailer
		if (!NTV2HostIsBigEndian  &&  !ok)
		{	//	FAILED:  Un-byteswap NTV2Buffer data...
			mInRegInfos.ByteSwap32();
			mOutBadRegIndexes.ByteSwap16();
		}
		return ok;
	}

	bool NTV2SetRegisters::RPCDecode (const UByteSequence & inBlob, size_t & inOutIndex)
	{
		bool ok = mHeader.RPCDecode(inBlob, inOutIndex);		//	NTV2_HEADER		mHeader
		POPU32(mInNumRegisters, inBlob, inOutIndex);			//		ULWord			mInNumRegisters
		ok &= mInRegInfos.RPCDecode(inBlob, inOutIndex);		//		NTV2Buffer		mInRegInfos
		POPU32(mOutNumFailures, inBlob, inOutIndex);			//		ULWord			mOutNumFailures
		ok &= mOutBadRegIndexes.RPCDecode(inBlob, inOutIndex);	//		NTV2Buffer		mOutBadRegIndexes
		ok &= mTrailer.RPCDecode(inBlob, inOutIndex);			//	NTV2_TRAILER	mTrailer
		if (!NTV2HostIsBigEndian)
		{	//	Re-byteswap NTV2Buffer data after decoding...
			mInRegInfos.ByteSwap32();
			mOutBadRegIndexes.ByteSwap16();
		}
		return ok;
	}

	bool NTV2BankSelGetSetRegs::RPCEncode (UByteSequence & outBlob)
	{
		const size_t totBytes	(mHeader.GetSizeInBytes()	//	Header + natural size of all structs/fields inbetween + Trailer
								+ mInBankInfos.GetByteCount() + mInRegInfos.GetByteCount());	//	NTV2Buffer fields
		if (outBlob.capacity() < totBytes)
			outBlob.reserve(totBytes);
		if (!NTV2HostIsBigEndian)
		{	//	All of my NTV2Buffers store arrays of ULWords that must be BigEndian BEFORE encoding into outBlob...
			mInBankInfos.ByteSwap32();
			mInRegInfos.ByteSwap32();
		}
		bool ok = mHeader.RPCEncode(outBlob);					//	NTV2_HEADER		mHeader
		PUSHU32(mIsWriting, outBlob);							//		ULWord			mIsWriting
		ok &= mInBankInfos.RPCEncode(outBlob);					//		NTV2Buffer		mInBankInfos
		ok &= mInRegInfos.RPCEncode(outBlob)					//		NTV2Buffer		mInRegInfos
			&& mTrailer.RPCEncode(outBlob);						//	NTV2_TRAILER	mTrailer
		if (!NTV2HostIsBigEndian  &&  !ok)
		{	//	FAILED:  Un-byteswap NTV2Buffer data...
			mInBankInfos.ByteSwap32();
			mInRegInfos.ByteSwap32();
		}
		return ok;
	}

	bool NTV2BankSelGetSetRegs::RPCDecode (const UByteSequence & inBlob, size_t & inOutIndex)
	{
		bool ok = mHeader.RPCDecode(inBlob, inOutIndex);		//	NTV2_HEADER		mHeader
		POPU32(mIsWriting, inBlob, inOutIndex);					//		ULWord			mIsWriting
		ok &= mInBankInfos.RPCDecode(inBlob, inOutIndex);		//		NTV2Buffer		mInBankInfos
		ok &= mInRegInfos.RPCDecode(inBlob, inOutIndex);		//		NTV2Buffer		mInRegInfos
		ok &= mTrailer.RPCDecode(inBlob, inOutIndex);			//	NTV2_TRAILER	mTrailer
		if (!NTV2HostIsBigEndian)
		{	//	Re-byteswap NTV2Buffer data after decoding...
			mInBankInfos.ByteSwap32();
			mInRegInfos.ByteSwap32();
		}
		return ok;
	}

	bool AUTOCIRCULATE_STATUS::RPCEncode (UByteSequence & outBlob)
	{
		const size_t totBytes	(acHeader.GetSizeInBytes());	//	Header + natural size of all structs/fields inbetween + Trailer
		if (outBlob.capacity() < totBytes)
			outBlob.reserve(totBytes);
		bool ok = acHeader.RPCEncode(outBlob);					//	NTV2_HEADER				acHeader
		PUSHU16(UWord(acCrosspoint), outBlob);					//		NTV2Crosspoint			acCrosspoint
		PUSHU16(UWord(acState), outBlob);						//		NTV2AutoCirculateState	acState
		PUSHU32(ULWord(acStartFrame), outBlob);					//		LWord					acStartFrame
		PUSHU32(ULWord(acEndFrame), outBlob);					//		LWord					acEndFrame
		PUSHU32(ULWord(acActiveFrame), outBlob);				//		LWord					acActiveFrame
		PUSHU64(acRDTSCStartTime, outBlob);						//		ULWord64				acRDTSCStartTime
		PUSHU64(acAudioClockStartTime, outBlob);				//		ULWord64				acAudioClockStartTime
		PUSHU64(acRDTSCCurrentTime, outBlob);					//		ULWord64				acRDTSCCurrentTime
		PUSHU64(acAudioClockCurrentTime, outBlob);				//		ULWord64				acAudioClockCurrentTime
		PUSHU32(acFramesProcessed, outBlob);					//		ULWord					acFramesProcessed
		PUSHU32(acFramesDropped, outBlob);						//		ULWord					acFramesDeopped
		PUSHU32(acBufferLevel, outBlob);						//		ULWord					acBufferLevel
		PUSHU32(acOptionFlags, outBlob);						//		ULWord					acOptionFlags
		PUSHU16(UWord(acAudioSystem), outBlob);					//		NTV2AudioSystem			acAudioSystem
		ok &= acTrailer.RPCEncode(outBlob);						//	NTV2_TRAILER			acTrailer
		return ok;
	}

	bool AUTOCIRCULATE_STATUS::RPCDecode (const UByteSequence & inBlob, size_t & inOutIndex)
	{	uint16_t v16(0);  uint32_t v32(0);
		bool ok = acHeader.RPCDecode(inBlob, inOutIndex);		//	NTV2_HEADER				acHeader
		POPU16(v16, inBlob, inOutIndex);						//		NTV2Crosspoint			acCrosspoint
		acCrosspoint = NTV2Crosspoint(v16);
		POPU16(v16, inBlob, inOutIndex);						//		NTV2AutoCirculateState	acState
		acState = NTV2AutoCirculateState(v16);
		POPU32(v32, inBlob, inOutIndex);						//		LWord					acStartFrame
		acStartFrame = LWord(v32);
		POPU32(v32, inBlob, inOutIndex);						//		LWord					acEndFrame
		acEndFrame = LWord(v32);
		POPU32(v32, inBlob, inOutIndex);						//		LWord					acActiveFrame
		acActiveFrame = LWord(v32);
		POPU64(acRDTSCStartTime, inBlob, inOutIndex);			//		ULWord64				acRDTSCStartTime
		POPU64(acAudioClockStartTime, inBlob, inOutIndex);		//		ULWord64				acAudioClockStartTime
		POPU64(acRDTSCCurrentTime, inBlob, inOutIndex);			//		ULWord64				acRDTSCCurrentTime
		POPU64(acAudioClockCurrentTime, inBlob, inOutIndex);	//		ULWord64				acAudioClockCurrentTime
		POPU32(acFramesProcessed, inBlob, inOutIndex);			//		ULWord					acFramesProcessed
		POPU32(acFramesDropped, inBlob, inOutIndex);			//		ULWord					acFramesDropped
		POPU32(acBufferLevel, inBlob, inOutIndex);				//		ULWord					acBufferLevel
		POPU32(acOptionFlags, inBlob, inOutIndex);				//		ULWord					acOptionFlags
		POPU16(v16, inBlob, inOutIndex);						//		NTV2AudioSystem			acAudioSystem
		acAudioSystem = NTV2AudioSystem(v16);
		ok &= acTrailer.RPCDecode(inBlob, inOutIndex);			//	NTV2_TRAILER			acTrailer
		return ok;
	}

	bool FRAME_STAMP::RPCEncode (UByteSequence & outBlob)
	{
		const size_t totBytes	(acHeader.GetSizeInBytes());	//	Header + natural size of all structs/fields inbetween + Trailer
		if (outBlob.capacity() < totBytes)
			outBlob.reserve(totBytes);
		bool ok = acHeader.RPCEncode(outBlob);						//	NTV2_HEADER				acHeader
		PUSHU64(ULWord64(acFrameTime), outBlob);					//		LWord64					acFrameTime
		PUSHU32(acRequestedFrame, outBlob);							//		ULWord					acRequestedFrame
		PUSHU64(acAudioClockTimeStamp, outBlob);					//		ULWord64				acAudioClockTimeStamp
		PUSHU32(acAudioExpectedAddress, outBlob);					//		ULWord					acAudioExpectedAddress
		PUSHU32(acAudioInStartAddress, outBlob);					//		ULWord					acAudioInStartAddress
		PUSHU32(acAudioInStopAddress, outBlob);						//		ULWord					acAudioInStopAddress
		PUSHU32(acAudioOutStopAddress, outBlob);					//		ULWord					acAudioOutStopAddress
		PUSHU32(acAudioOutStartAddress, outBlob);					//		ULWord					acAudioOutStartAddress
		PUSHU32(acTotalBytesTransferred, outBlob);					//		ULWord					acTotalBytesTransferred
		PUSHU32(acStartSample, outBlob);							//		ULWord					acStartSample

		ok &= acTimeCodes.RPCEncode(outBlob);						//		NTV2Buffer				acTimeCodes
		PUSHU64(ULWord64(acCurrentTime), outBlob);					//		LWord64					acCurrentTime
		PUSHU32(acCurrentFrame, outBlob);							//		ULWord					acCurrentFrame
		PUSHU64(ULWord64(acCurrentFrameTime), outBlob);				//		LWord64					acCurrentFrameTime
		PUSHU64(acAudioClockCurrentTime, outBlob);					//		ULWord64				acAudioClockCurrentTime
		PUSHU32(acCurrentAudioExpectedAddress, outBlob);			//		ULWord					acCurrentAudioExpectedAddress
		PUSHU32(acCurrentAudioStartAddress, outBlob);				//		ULWord					acCurrentAudioStartAddress
		PUSHU32(acCurrentFieldCount, outBlob);						//		ULWord					acCurrentFieldCount
		PUSHU32(acCurrentLineCount, outBlob);						//		ULWord					acCurrentLineCount
		PUSHU32(acCurrentReps, outBlob);							//		ULWord					acCurrentReps
		PUSHU64(acCurrentUserCookie, outBlob);						//		ULWord64				acCurrentUserCookie
		PUSHU32(acFrame, outBlob);									//		ULWord					acFrame
		PUSHU32(acRP188.fDBB, outBlob);								//		ULWord					acRP188.fDBB
		PUSHU32(acRP188.fLo, outBlob);								//		ULWord					acRP188.fLo
		PUSHU32(acRP188.fHi, outBlob);								//		ULWord					acRP188.fHi
		ok &= acTrailer.RPCEncode(outBlob);							//	NTV2_TRAILER			acTrailer
		return ok;
	}

	bool FRAME_STAMP::RPCDecode (const UByteSequence & inBlob, size_t & inOutIndex)
	{	uint64_t v64(0);
		bool ok = acHeader.RPCDecode(inBlob, inOutIndex);			//	NTV2_HEADER				acHeader
		POPU64(v64, inBlob, inOutIndex);							//		LWord64					acFrameTime
		acFrameTime = LWord64(v64);
		POPU32(acRequestedFrame, inBlob, inOutIndex);				//		ULWord					acRequestedFrame
		POPU64(acAudioClockTimeStamp, inBlob, inOutIndex);			//		ULWord64				acAudioClockTimeStamp
		POPU32(acAudioExpectedAddress, inBlob, inOutIndex);			//		ULWord					acAudioExpectedAddress
		POPU32(acAudioInStartAddress, inBlob, inOutIndex);			//		ULWord					acAudioInStartAddress
		POPU32(acAudioInStopAddress, inBlob, inOutIndex);			//		ULWord					acAudioInStopAddress
		POPU32(acAudioOutStopAddress, inBlob, inOutIndex);			//		ULWord					acAudioOutStopAddress
		POPU32(acAudioOutStartAddress, inBlob, inOutIndex);			//		ULWord					acAudioOutStartAddress
		POPU32(acTotalBytesTransferred, inBlob, inOutIndex);		//		ULWord					acTotalBytesTransferred
		POPU32(acStartSample, inBlob, inOutIndex);					//		ULWord					acStartSample

		ok &= acTimeCodes.RPCDecode(inBlob, inOutIndex);			//		NTV2Buffer				acTimeCodes
		POPU64(v64, inBlob, inOutIndex);							//		LWord64					acCurrentTime
		acCurrentTime = LWord64(v64);
		POPU32(acCurrentFrame, inBlob, inOutIndex);					//		ULWord					acCurrentFrame
		POPU64(v64, inBlob, inOutIndex);							//		LWord64					acCurrentFrameTime
		acCurrentFrameTime = LWord64(v64);
		POPU64(acAudioClockCurrentTime, inBlob, inOutIndex);		//		ULWord64				acAudioClockCurrentTime
		POPU32(acCurrentAudioExpectedAddress, inBlob, inOutIndex);	//	ULWord					acCurrentAudioExpectedAddress
		POPU32(acCurrentAudioStartAddress, inBlob, inOutIndex);		//		ULWord					acCurrentAudioStartAddress
		POPU32(acCurrentFieldCount, inBlob, inOutIndex);			//		ULWord					acCurrentFieldCount
		POPU32(acCurrentLineCount, inBlob, inOutIndex);				//		ULWord					acCurrentLineCount
		POPU32(acCurrentReps, inBlob, inOutIndex);					//		ULWord					acCurrentReps
		POPU64(acCurrentUserCookie, inBlob, inOutIndex);			//		ULWord64				acCurrentUserCookie
		POPU32(acFrame, inBlob, inOutIndex);						//		ULWord					acFrame
		POPU32(acRP188.fDBB, inBlob, inOutIndex);					//		ULWord					acRP188.fDBB
		POPU32(acRP188.fLo, inBlob, inOutIndex);					//		ULWord					acRP188.fLo
		POPU32(acRP188.fHi, inBlob, inOutIndex);					//		ULWord					acRP188.fHi
		ok &= acTrailer.RPCDecode(inBlob, inOutIndex);				//	NTV2_TRAILER			acTrailer
		return ok;
	}

	bool AUTOCIRCULATE_TRANSFER_STATUS::RPCEncode (UByteSequence & outBlob)
	{
		const size_t totBytes (acHeader.GetSizeInBytes());		//	Header + natural size of all structs/fields inbetween + Trailer
		if (outBlob.capacity() < totBytes)
			outBlob.reserve(totBytes);
		bool ok = acHeader.RPCEncode(outBlob);					//	NTV2_HEADER				acHeader
		PUSHU16(acState, outBlob);								//		UWord					acState
		PUSHU32(ULWord(acTransferFrame), outBlob);				//		LWord					acTransferFrame
		PUSHU32(acBufferLevel, outBlob);						//		ULWord					acBufferLevel
		PUSHU32(acFramesProcessed, outBlob);					//		ULWord					acFramesProcessed
		PUSHU32(acFramesDropped, outBlob);						//		ULWord					acFramesDropped
		ok &= acFrameStamp.RPCEncode(outBlob);					//		FRAME_STAMP				acFrameStamp
		PUSHU32(acAudioTransferSize, outBlob);					//		ULWord					acAudioTransferSize
		PUSHU32(acAudioStartSample, outBlob);					//		ULWord					acAudioStartSample
		PUSHU32(acAncTransferSize, outBlob);					//		ULWord					acAncTransferSize
		PUSHU32(acAncField2TransferSize, outBlob);				//		ULWord					acAncField2TransferSize
		ok &= acTrailer.RPCEncode(outBlob);						//	NTV2_TRAILER			acTrailer
		return ok;
	}

	bool AUTOCIRCULATE_TRANSFER_STATUS::RPCDecode (const UByteSequence & inBlob, size_t & inOutIndex)
	{	uint16_t v16(0);  uint32_t v32(0);
		bool ok = acHeader.RPCDecode(inBlob, inOutIndex);		//	NTV2_HEADER				acHeader
		POPU16(v16, inBlob, inOutIndex);						//		NTV2AutoCirculateState	acState
		acState = NTV2AutoCirculateState(v16);
		POPU32(v32, inBlob, inOutIndex);						//		LWord					acTransferFrame
		acTransferFrame = LWord(v32);
		POPU32(acBufferLevel, inBlob, inOutIndex);				//		ULWord					acBufferLevel
		POPU32(acFramesProcessed, inBlob, inOutIndex);			//		ULWord					acFramesProcessed
		POPU32(acFramesDropped, inBlob, inOutIndex);			//		ULWord					acFramesDropped
		ok &= acFrameStamp.RPCDecode(inBlob, inOutIndex);		//		FRAME_STAMP				acFrameStamp
		POPU32(acAudioTransferSize, inBlob, inOutIndex);		//		ULWord					acAudioTransferSize
		POPU32(acAudioStartSample, inBlob, inOutIndex);			//		ULWord					acAudioStartSample
		POPU32(acAncTransferSize, inBlob, inOutIndex);			//		ULWord					acAncTransferSize
		POPU32(acAncField2TransferSize, inBlob, inOutIndex);	//		ULWord					acAncField2TransferSize
		ok &= acTrailer.RPCDecode(inBlob, inOutIndex);			//	NTV2_TRAILER			acTrailer
		return ok;
	}

	bool NTV2SegmentedDMAInfo::RPCEncode (UByteSequence & outBlob)
	{
		PUSHU32(acNumSegments, outBlob);						//	ULWord					acNumSegments
		PUSHU32(acNumActiveBytesPerRow, outBlob);				//	ULWord					acNumActiveBytesPerRow
		PUSHU32(acSegmentHostPitch, outBlob);					//	ULWord					acSegmentHostPitch
		PUSHU32(acSegmentDevicePitch, outBlob);					//	ULWord					acSegmentDevicePitch
		return true;
	}

	bool NTV2SegmentedDMAInfo::RPCDecode (const UByteSequence & inBlob, size_t & inOutIndex)
	{
		POPU32(acNumSegments, inBlob, inOutIndex);				//	ULWord					acNumSegments
		POPU32(acNumActiveBytesPerRow, inBlob, inOutIndex);		//	ULWord					acNumActiveBytesPerRow
		POPU32(acSegmentHostPitch, inBlob, inOutIndex);			//	ULWord					acSegmentHostPitch
		POPU32(acSegmentDevicePitch, inBlob, inOutIndex);		//	ULWord					acSegmentDevicePitch
		return true;
	}

	bool NTV2ColorCorrectionData::RPCEncode (UByteSequence & outBlob)
	{
		PUSHU16(ccMode, outBlob);								//	NTV2ColorCorrectionMode	ccMode
		PUSHU32(ccSaturationValue, outBlob);					//	ULWord					ccSaturationValue
		return ccLookupTables.RPCEncode(outBlob);				//	NTV2Buffer				ccLookupTables
	}

	bool NTV2ColorCorrectionData::RPCDecode (const UByteSequence & inBlob, size_t & inOutIndex)
	{	uint16_t u16(0);
		POPU16(u16, inBlob, inOutIndex);						//	NTV2ColorCorrectionMode	ccMode
		ccMode = NTV2ColorCorrectionMode(u16);
		POPU32(ccSaturationValue, inBlob, inOutIndex);			//	ULWord					ccSaturationValue
		return ccLookupTables.RPCDecode(inBlob, inOutIndex);	//	NTV2Buffer				ccLookupTables
	}

	bool AutoCircVidProcInfo::RPCEncode (UByteSequence & outBlob)
	{
		PUSHU16(mode, outBlob);									//	AutoCircVidProcMode		mode
		PUSHU16(foregroundVideoCrosspoint, outBlob);			//	NTV2Crosspoint			foregroundVideoCrosspoint
		PUSHU16(backgroundVideoCrosspoint, outBlob);			//	NTV2Crosspoint			backgroundVideoCrosspoint
		PUSHU16(foregroundKeyCrosspoint, outBlob);				//	NTV2Crosspoint			foregroundKeyCrosspoint
		PUSHU16(backgroundKeyCrosspoint, outBlob);				//	NTV2Crosspoint			backgroundKeyCrosspoint
		PUSHU32(ULWord(transitionCoefficient), outBlob);		//	Fixed_					transitionCoefficient
		PUSHU32(ULWord(transitionSoftness), outBlob);			//	Fixed_					transitionSoftness
		return true;
	}

	bool AutoCircVidProcInfo::RPCDecode (const UByteSequence & inBlob, size_t & inOutIndex)
	{	uint16_t v16(0);	uint32_t v32(0);
		POPU16(v16, inBlob, inOutIndex);						//	AutoCircVidProcMode		mode
		mode = AutoCircVidProcMode(v16);
		POPU16(v16, inBlob, inOutIndex);						//	NTV2Crosspoint			foregroundVideoCrosspoint
		foregroundVideoCrosspoint = NTV2Crosspoint(v16);
		POPU16(v16, inBlob, inOutIndex);						//	NTV2Crosspoint			backgroundVideoCrosspoint
		backgroundVideoCrosspoint = NTV2Crosspoint(v16);
		POPU16(v16, inBlob, inOutIndex);						//	NTV2Crosspoint			foregroundKeyCrosspoint
		foregroundKeyCrosspoint = NTV2Crosspoint(v16);
		POPU16(v16, inBlob, inOutIndex);						//	NTV2Crosspoint			backgroundKeyCrosspoint
		backgroundKeyCrosspoint = NTV2Crosspoint(v16);
		POPU32(v32, inBlob, inOutIndex);						//	Fixed_					transitionCoefficient
		transitionCoefficient = Fixed_(v32);
		POPU32(v32, inBlob, inOutIndex);						//	Fixed_					transitionSoftness
		transitionSoftness = Fixed_(v32);
		return true;
	}

	bool NTV2_RP188::RPCEncode (UByteSequence & outBlob)
	{
		PUSHU32(fDBB, outBlob);									//	ULWord					fDBB
		PUSHU32(fLo, outBlob);									//	ULWord					fLo
		PUSHU32(fHi, outBlob);									//	ULWord					fHi
		return true;
	}

	bool NTV2_RP188::RPCDecode (const UByteSequence & inBlob, size_t & inOutIndex)
	{
		POPU32(fDBB, inBlob, inOutIndex);						//	ULWord					fDBB
		POPU32(fLo, inBlob, inOutIndex);						//	ULWord					fLo
		POPU32(fHi, inBlob, inOutIndex);						//	ULWord					fHi
		return true;
	}

	bool AUTOCIRCULATE_TRANSFER::RPCEncode (UByteSequence & outBlob)
	{
		AJADebug::StatTimerStart(AJA_DebugStat_ACXferRPCEncode);
		const size_t totBytes (acHeader.GetSizeInBytes() + acVideoBuffer.GetByteCount() + acAudioBuffer.GetByteCount()
								+ acANCBuffer.GetByteCount() + acANCField2Buffer.GetByteCount() + acOutputTimeCodes.GetByteCount()
								+ acHDMIAuxData.GetByteCount() + 64);		//	Header + natural size of all structs/fields inbetween + Trailer
		if (outBlob.capacity() < totBytes)
			outBlob.reserve(totBytes);
		bool ok = acHeader.RPCEncode(outBlob);					//	NTV2_HEADER						acHeader
		ok &= acVideoBuffer.RPCEncode(outBlob);					//		NTV2Buffer						acVideoBuffer
		ok &= acAudioBuffer.RPCEncode(outBlob);					//		NTV2Buffer						acAudioBuffer
		ok &= acANCBuffer.RPCEncode(outBlob);					//		NTV2Buffer						acANCBuffer
		ok &= acANCField2Buffer.RPCEncode(outBlob);				//		NTV2Buffer						acANCField2Buffer
		ok &= acOutputTimeCodes.RPCEncode(outBlob);				//		NTV2Buffer						acOutputTimeCodes
		ok &= acTransferStatus.RPCEncode(outBlob);				//		AUTOCIRCULATE_TRANSFER_STATUS	acTransferStatus
		PUSHU64(acInUserCookie, outBlob);						//		ULWord64						acInUserCookie
		PUSHU32(acInVideoDMAOffset, outBlob);					//		ULWord							acInVideoDMAOffset
		ok &= acInSegmentedDMAInfo.RPCEncode(outBlob);			//		NTV2SegmentedDMAInfo			acInSegmentedDMAInfo
		ok &= acColorCorrection.RPCEncode(outBlob);				//		NTV2ColorCorrectionData			acColorCorrection
		PUSHU16(acFrameBufferFormat, outBlob);					//		NTV2PixelFormat					acFrameBufferFormat
		PUSHU16(acFrameBufferOrientation, outBlob);				//		NTV2FBOrientation				acFrameBufferOrientation
		ok &= acVidProcInfo.RPCEncode(outBlob);					//		AutoCircVidProcInfo				acVidProcInfo
		PUSHU16(acVideoQuarterSizeExpand, outBlob);				//		NTV2QtrSizeExpandMode			acVideoQuarterSizeExpand
		ok &= acHDMIAuxData.RPCEncode(outBlob);					//		NTV2Buffer						acHDMIAuxData
		PUSHU32(acPeerToPeerFlags, outBlob);					//		ULWord							acPeerToPeerFlags
		PUSHU32(acFrameRepeatCount, outBlob);					//		ULWord							acFrameRepeatCount
		PUSHU32(ULWord(acDesiredFrame), outBlob);				//		LWord							acDesiredFrame
		ok &= acRP188.RPCEncode(outBlob);						//		NTV2_RP188						acRP188
		PUSHU16(acCrosspoint, outBlob);							//		NTV2Crosspoint					acCrosspoint
		ok &= acTrailer.RPCEncode(outBlob);						//	NTV2_TRAILER					acTrailer
		AJADebug::StatTimerStop(AJA_DebugStat_ACXferRPCEncode);
		return ok;
	}

	bool AUTOCIRCULATE_TRANSFER::RPCDecode (const UByteSequence & inBlob, size_t & inOutIndex)
	{	uint16_t v16(0);  uint32_t v32(0);
		AJADebug::StatTimerStart(AJA_DebugStat_ACXferRPCDecode);
		bool ok = acHeader.RPCDecode(inBlob, inOutIndex);		//	NTV2_HEADER						acHeader
		ok &= acVideoBuffer.RPCDecode(inBlob, inOutIndex);		//		NTV2Buffer						acVideoBuffer
		ok &= acAudioBuffer.RPCDecode(inBlob, inOutIndex);		//		NTV2Buffer						acAudioBuffer
		ok &= acANCBuffer.RPCDecode(inBlob, inOutIndex);		//		NTV2Buffer						acANCBuffer
		ok &= acANCField2Buffer.RPCDecode(inBlob, inOutIndex);	//		NTV2Buffer						acANCField2Buffer
		ok &= acOutputTimeCodes.RPCDecode(inBlob, inOutIndex);	//		NTV2Buffer						acOutputTimeCodes
		ok &= acTransferStatus.RPCDecode(inBlob, inOutIndex);	//		AUTOCIRCULATE_TRANSFER_STATUS	acTransferStatus
		POPU64(acInUserCookie, inBlob, inOutIndex);				//		ULWord64						acInUserCookie
		POPU32(acInVideoDMAOffset, inBlob, inOutIndex);			//		ULWord							acInVideoDMAOffset
		ok &= acInSegmentedDMAInfo.RPCDecode(inBlob, inOutIndex);//		NTV2Buffer						acInSegmentedDMAInfo
		ok &= acColorCorrection.RPCDecode(inBlob, inOutIndex);	//		NTV2Buffer						acColorCorrection
		POPU16(v16, inBlob, inOutIndex);						//		NTV2PixelFormat					acFrameBufferFormat
		acFrameBufferFormat = NTV2FrameBufferFormat(v16);
		POPU16(v16, inBlob, inOutIndex);						//		NTV2FBOrientation				acFrameBufferOrientation
		acFrameBufferOrientation = NTV2FBOrientation(v16);
		ok &= acVidProcInfo.RPCDecode(inBlob, inOutIndex);		//		AutoCircVidProcInfo				acVidProcInfo
		POPU16(v16, inBlob, inOutIndex);						//		NTV2QtrSizeExpandMode			acVideoQuarterSizeExpand
		acVideoQuarterSizeExpand = NTV2QtrSizeExpandMode(v16);
		ok &= acHDMIAuxData.RPCDecode(inBlob, inOutIndex);		//		NTV2Buffer						acHDMIAuxData
		POPU32(acPeerToPeerFlags, inBlob, inOutIndex);			//		ULWord							acPeerToPeerFlags
		POPU32(acFrameRepeatCount, inBlob, inOutIndex);			//		ULWord							acFrameRepeatCount
		POPU32(v32, inBlob, inOutIndex);						//		LWord							acDesiredFrame
		acDesiredFrame = LWord(v32);
		ok &= acRP188.RPCDecode(inBlob, inOutIndex);			//		NTV2_RP188						acRP188
		POPU16(v16, inBlob, inOutIndex);						//		NTV2Crosspoint					acCrosspoint
		acCrosspoint = NTV2Crosspoint(v16);
		ok &= acTrailer.RPCDecode(inBlob, inOutIndex);			//	NTV2_TRAILER					acTrailer
		AJADebug::StatTimerStop(AJA_DebugStat_ACXferRPCDecode);
		return ok;
	}

	bool AUTOCIRCULATE_TRANSFER_STRUCT::RPCEncode (UByteSequence & outBlob)
	{
		NTV2Buffer buff;
		PUSHU16(UWord(channelSpec), outBlob);					//	NTV2Crosspoint			channelSpec
//		PUSHU64(ULWord64(videoBuffer), outBlob)					//	ULWord	*				videoBuffer
		PUSHU32(videoBufferSize, outBlob);						//	ULWord					videoBufferSize
		if (videoBuffer && videoBufferSize)
		{	buff.Set(videoBuffer, videoBufferSize);
			buff.AppendU8s(outBlob);
		}
		PUSHU32(videoDmaOffset, outBlob);						//	ULWord					videoDmaOffset
//		PUSHU64(ULWord64(audioBuffer), outBlob)					//	ULWord	*				audioBuffer
		PUSHU32(audioBufferSize, outBlob);						//	ULWord					audioBufferSize
		if (audioBuffer && audioBufferSize)
		{	buff.Set(audioBuffer, audioBufferSize);
			buff.AppendU8s(outBlob);
		}
		PUSHU32(audioStartSample, outBlob);						//	ULWord					audioStartSample
		PUSHU32(audioNumChannels, outBlob);						//	ULWord					audioNumChannels
		PUSHU32(frameRepeatCount, outBlob);						//	ULWord					frameRepeatCount
		rp188.RPCEncode(outBlob);								//	RP188_STRUCT			rp188
		PUSHU32(ULWord(desiredFrame), outBlob);					//	LWord					desiredFrame
		PUSHU32(hUser, outBlob);								//	ULWord					hUser
		PUSHU32(transferFlags, outBlob);						//	ULWord					transferFlags
		PUSHU8(bDisableExtraAudioInfo, outBlob);				//	BOOL_					bDisableExtraAudioInfo
		PUSHU16(UWord(frameBufferFormat), outBlob);				//	NTV2PixelFormat			frameBufferFormat
		PUSHU16(UWord(frameBufferOrientation), outBlob);		//	NTV2FBOrientation		frameBufferOrientation
		//	Skip color correction for now						//	NTV2ColorCorrectionInfo	colorCorrectionInfo
		vidProcInfo.RPCEncode(outBlob);							//	AutoCircVidProcInfo		vidProcInfo
		PUSHU32(customAncInfo.Group1, outBlob);					//	CUSTOM_ANC_STRUCT		customAncInfo
		PUSHU32(customAncInfo.Group2, outBlob);
		PUSHU32(customAncInfo.Group3, outBlob);
		PUSHU32(customAncInfo.Group4, outBlob);
		PUSHU32(videoNumSegments, outBlob);						//	ULWord					videoNumSegments
		PUSHU32(videoSegmentHostPitch, outBlob);				//	ULWord					videoSegmentHostPitch
		PUSHU32(videoSegmentCardPitch, outBlob);				//	ULWord					videoSegmentCardPitch
		PUSHU16(UWord(videoQuarterSizeExpand), outBlob);		//	NTV2QtrSizeExpandMode	videoQuarterSizeExpand
		return true;
	}

	bool AUTOCIRCULATE_TRANSFER_STRUCT::RPCDecode (const UByteSequence & inBlob, size_t & inOutIndex)
	{	UWord v16(0);  ULWord v32(0);
		POPU16(v16, inBlob, inOutIndex);						//	NTV2Crosspoint			channelSpec
		channelSpec = NTV2Crosspoint(v16);
//		POPU64(u64, inBlob, inOutIndex);						//	ULWord	*				videoBuffer
//		videoBuffer = reinterpret_cast<ULWord*>(u64);
		POPU32(videoBufferSize, inBlob, inOutIndex);			//	ULWord					videoBufferSize
		if (videoBufferSize  &&  !videoBuffer)
		{
			videoBuffer = reinterpret_cast<ULWord*>(AJAMemory::AllocateAligned(videoBufferSize, NTV2Buffer::DefaultPageSize()));
			if (!videoBuffer)
				return false;
			if ((inOutIndex + videoBufferSize) >= inBlob.size())
				return false;	//	past end of inBlob
			UByte* pBuffer = reinterpret_cast<UByte*>(videoBuffer);
			for (ULWord cnt(0);  cnt < videoBufferSize;  cnt++)
				pBuffer[cnt] = inBlob.at(inOutIndex++);	//	Caller is responsible for byte-swapping if needed
		}
		POPU32(videoDmaOffset, inBlob, inOutIndex);				//	ULWord					videoDmaOffset
//		POPU64(u64, inBlob, inOutIndex);						//	ULWord	*				audioBuffer
//		audioBuffer = reinterpret_cast<ULWord*>(u64);
		POPU32(audioBufferSize, inBlob, inOutIndex);			//	ULWord					audioBufferSize
		if (audioBufferSize  &&  !audioBuffer)
		{
			audioBuffer = reinterpret_cast<ULWord*>(AJAMemory::AllocateAligned(audioBufferSize, NTV2Buffer::DefaultPageSize()));
			if (!audioBuffer)
				return false;
			if ((inOutIndex + audioBufferSize) >= inBlob.size())
				return false;	//	past end of inBlob
			UByte* pBuffer = reinterpret_cast<UByte*>(audioBuffer);
			for (ULWord cnt(0);  cnt < audioBufferSize;  cnt++)
				pBuffer[cnt] = inBlob.at(inOutIndex++);	//	Caller is responsible for byte-swapping if needed
		}
		POPU32(audioStartSample, inBlob, inOutIndex);			//	ULWord					audioStartSample
		POPU32(audioNumChannels, inBlob, inOutIndex);			//	ULWord					audioNumChannels
		POPU32(frameRepeatCount, inBlob, inOutIndex);			//	ULWord					frameRepeatCount
		rp188.RPCDecode(inBlob, inOutIndex);					//	RP188_STRUCT			rp188
		POPU32(v32, inBlob, inOutIndex);						//	LWord					desiredFrame
		desiredFrame = LWord(v32);
		POPU32(hUser, inBlob, inOutIndex);						//	ULWord					hUser
		POPU32(transferFlags, inBlob, inOutIndex);				//	ULWord					transferFlags
		POPU8(AsU8Ref(bDisableExtraAudioInfo), inBlob, inOutIndex);	//	BOOL_					bDisableExtraAudioInfo
		POPU16(v16, inBlob, inOutIndex);						//	NTV2PixelFormat			frameBufferFormat
		frameBufferFormat = NTV2PixelFormat(v16);
		POPU16(v16, inBlob, inOutIndex);						//	NTV2FBOrientation		frameBufferOrientation
		frameBufferOrientation = NTV2FBOrientation(v16);
		//	Skip color correction for now						//	NTV2ColorCorrectionInfo	colorCorrectionInfo
		vidProcInfo.RPCDecode(inBlob, inOutIndex);				//	AutoCircVidProcInfo		vidProcInfo
		POPU32(customAncInfo.Group1, inBlob, inOutIndex);		//	CUSTOM_ANC_STRUCT		customAncInfo
		POPU32(customAncInfo.Group2, inBlob, inOutIndex);
		POPU32(customAncInfo.Group3, inBlob, inOutIndex);
		POPU32(customAncInfo.Group4, inBlob, inOutIndex);
		POPU32(videoNumSegments, inBlob, inOutIndex);			//	ULWord					videoNumSegments
		POPU32(videoSegmentHostPitch, inBlob, inOutIndex);		//	ULWord					videoSegmentHostPitch
		POPU32(videoSegmentCardPitch, inBlob, inOutIndex);		//	ULWord					videoSegmentCardPitch
		POPU16(v16, inBlob, inOutIndex);						//	NTV2QtrSizeExpandMode	videoQuarterSizeExpand
		videoQuarterSizeExpand = NTV2QtrSizeExpandMode(v16);
		return true;
	}

	bool NTV2Bitstream::RPCEncode (UByteSequence & outBlob)
	{
		const size_t totBytes (mHeader.GetSizeInBytes());		//	Header + natural size of all structs/fields inbetween + Trailer
		if (outBlob.capacity() < totBytes)
			outBlob.reserve(totBytes);
		bool ok = mHeader.RPCEncode(outBlob);					//	NTV2_HEADER				mHeader
		ok &= mBuffer.RPCEncode(outBlob);						//		NTV2Buffer				mBuffer
		PUSHU32(mFlags, outBlob);								//		ULWord					mFlags
		PUSHU32(mStatus, outBlob);								//		ULWord					mStatus
		for (size_t ndx(0);  ndx < 16;  ndx++)
			PUSHU32(mRegisters[ndx], outBlob);					//		ULWord					mRegisters[16]
		for (size_t ndx(0);  ndx < 32;  ndx++)
			PUSHU32(mReserved[ndx], outBlob);					//		ULWord					mReserved[32]
		ok &= mTrailer.RPCEncode(outBlob);						//	NTV2_TRAILER			mTrailer
		return ok;
	}

	bool NTV2Bitstream::RPCDecode (const UByteSequence & inBlob, size_t & inOutIndex)
	{
		bool ok = mHeader.RPCDecode(inBlob, inOutIndex);		//	NTV2_HEADER				acHeader
		ok &= mBuffer.RPCDecode(inBlob, inOutIndex);			//		NTV2Buffer				mBuffer
		POPU32(mFlags, inBlob, inOutIndex);						//		ULWord					mFlags
		POPU32(mStatus, inBlob, inOutIndex);					//		ULWord					mStatus
		for (size_t ndx(0);  ndx < 16;  ndx++)
			POPU32(mRegisters[ndx], inBlob, inOutIndex);		//		ULWord					mRegisters[16]
		for (size_t ndx(0);  ndx < 16;  ndx++)
			POPU32(mReserved[ndx], inBlob, inOutIndex);			//		ULWord					mReserved[16]
		ok &= mTrailer.RPCDecode(inBlob, inOutIndex);			//	NTV2_TRAILER			mTrailer
		return ok;
	}

	bool AUTOCIRCULATE_STATUS_STRUCT::RPCEncode (UByteSequence & outBlob)
	{
		PUSHU16(UWord(channelSpec), outBlob);					//	NTV2Crosspoint			channelSpec
		PUSHU16(UWord(state), outBlob);							//	NTV2AutoCirculateState	state
		PUSHU32(ULWord(startFrame), outBlob);					//	LWord					startFrame
		PUSHU32(ULWord(endFrame), outBlob);						//	LWord					endFrame
		PUSHU32(ULWord(activeFrame), outBlob);					//	LWord					activeFrame
		PUSHU64(rdtscStartTime, outBlob);						//	ULWord64				rdtscStartTime
		PUSHU64(audioClockStartTime, outBlob);					//	ULWord64				audioClockStartTime
		PUSHU64(rdtscCurrentTime, outBlob);						//	ULWord64				rdtscCurrentTime
		PUSHU64(audioClockCurrentTime, outBlob);				//	ULWord64				audioClockCurrentTime
		PUSHU32(framesProcessed, outBlob);						//	ULWord					framesProcessed
		PUSHU32(framesDropped, outBlob);						//	ULWord					framesDropped
		PUSHU32(bufferLevel, outBlob);							//	ULWord					bufferLevel
		PUSHU8(bWithAudio, outBlob);							//	BOOL_					bWithAudio
		PUSHU8(bWithRP188, outBlob);							//	BOOL_					bWithRP188
		PUSHU8(bFbfChange, outBlob);							//	BOOL_					bFbfChange
		PUSHU8(bFboChange, outBlob);							//	BOOL_					bFboChange
		PUSHU8(bWithColorCorrection, outBlob);					//	BOOL_					bWithColorCorrection
		PUSHU8(bWithVidProc, outBlob);							//	BOOL_					bWithVidProc
		PUSHU8(bWithCustomAncData, outBlob);					//	BOOL_					bWithCustomAncData
		return true;
	}

	bool AUTOCIRCULATE_STATUS_STRUCT::RPCDecode (const UByteSequence & inBlob, size_t & inOutIndex)
	{	uint16_t v16(0);  uint32_t v32(0);
		POPU16(v16, inBlob, inOutIndex);						//	NTV2Crosspoint			channelSpec
		channelSpec = NTV2Crosspoint(v16);
		POPU16(v16, inBlob, inOutIndex);						//	NTV2AutoCirculateState	state
		state = NTV2AutoCirculateState(v16);
		POPU32(v32, inBlob, inOutIndex);						//	LWord					startFrame
		startFrame = LWord(v32);
		POPU32(v32, inBlob, inOutIndex);						//	LWord					endFrame
		endFrame = LWord(v32);
		POPU32(v32, inBlob, inOutIndex);						//	LWord					activeFrame
		activeFrame = LWord(v32);
		POPU64(rdtscStartTime, inBlob, inOutIndex);				//	ULWord64				rdtscStartTime
		POPU64(audioClockStartTime, inBlob, inOutIndex);		//	ULWord64				audioClockStartTime
		POPU64(rdtscCurrentTime, inBlob, inOutIndex);			//	ULWord64				rdtscCurrentTime
		POPU64(audioClockCurrentTime, inBlob, inOutIndex);		//	ULWord64				audioClockCurrentTime
		POPU32(framesProcessed, inBlob, inOutIndex);			//	ULWord					framesProcessed
		POPU32(framesDropped, inBlob, inOutIndex);				//	ULWord					framesDropped
		POPU32(bufferLevel, inBlob, inOutIndex);				//	ULWord					bufferLevel
		POPU8(AsU8Ref(bWithAudio), inBlob, inOutIndex);			//	BOOL_					bWithAudio
		POPU8(AsU8Ref(bWithRP188), inBlob, inOutIndex);			//	BOOL_					bWithRP188
		POPU8(AsU8Ref(bFbfChange), inBlob, inOutIndex);			//	BOOL_					bFbfChange
		POPU8(AsU8Ref(bFboChange), inBlob, inOutIndex);			//	BOOL_					bFboChange
		POPU8(AsU8Ref(bWithColorCorrection), inBlob, inOutIndex);//	BOOL_					bWithColorCorrection
		POPU8(AsU8Ref(bWithVidProc), inBlob, inOutIndex);		//	BOOL_					bWithVidProc
		POPU8(AsU8Ref(bWithCustomAncData), inBlob, inOutIndex);	//	BOOL_					bWithCustomAncData
		return true;
	}

	bool RP188_STRUCT::RPCEncode (UByteSequence & outBlob)
	{
		PUSHU32(DBB, outBlob);				//	ULWord	DBB
		PUSHU32(Low, outBlob);				//	ULWord	Low
		PUSHU32(High, outBlob);				//	ULWord	High
		return true;
	}

	bool RP188_STRUCT::RPCDecode (const UByteSequence & inBlob, size_t & inOutIndex)
	{
		POPU32(DBB, inBlob, inOutIndex);	//	ULWord	DBB
		POPU32(Low, inBlob, inOutIndex);	//	ULWord	Low
		POPU32(High, inBlob, inOutIndex);	//	ULWord	High
		return true;
	}


	bool AUTOCIRCULATE_TASK_STRUCT::RPCEncode (UByteSequence & outBlob)
	{
		PUSHU32(taskVersion, outBlob);				//	ULWord	taskVersion
		PUSHU32(taskSize, outBlob);					//	ULWord	taskSize
		PUSHU32(numTasks, outBlob);					//	ULWord	numTasks
		PUSHU32(maxTasks, outBlob);					//	ULWord	maxTasks
		PUSHU64(ULWord64(taskArray), outBlob);		//	ULWord	taskArray
		if (taskArray && numTasks)
			for (ULWord num(0);  num < numTasks;  num++)
			{
				const AutoCircGenericTask &	task (taskArray[num]);
				PUSHU32(task.taskType, outBlob);	//	AutoCircTaskType	taskType
				const ULWord * pULWords = reinterpret_cast<const ULWord*>(&task.u);
				ULWord numWords(0);
				if (NTV2_IS_REGISTER_TASK(task.taskType))
					numWords = sizeof(AutoCircRegisterTask)/sizeof(ULWord);
				else if (NTV2_IS_TIMECODE_TASK(task.taskType))
					numWords = sizeof(AutoCircTimeCodeTask)/sizeof(ULWord);
				for (ULWord word(0);  word < numWords;  word++)
					PUSHU32(pULWords[word], outBlob);
			}
		return true;
	}

	bool AUTOCIRCULATE_TASK_STRUCT::RPCDecode (const UByteSequence & inBlob, size_t & inOutIndex)
	{	ULWord u32(0);  ULWord64 u64(0);
		POPU32(taskVersion, inBlob, inOutIndex);	//	ULWord	taskVersion
		POPU32(taskSize, inBlob, inOutIndex);		//	ULWord	taskSize
		POPU32(numTasks, inBlob, inOutIndex);		//	ULWord	numTasks
		POPU32(maxTasks, inBlob, inOutIndex);		//	ULWord	maxTasks
		POPU64(u64, inBlob, inOutIndex);			//	ULWord	taskArray
		taskArray = reinterpret_cast<AutoCircGenericTask*>(u64);
		if (taskArray && numTasks)
			for (ULWord num(0);  num < numTasks;  num++)
			{
				AutoCircGenericTask & task (taskArray[num]);
				POPU32(u32, inBlob, inOutIndex);	//	AutoCircTaskType	taskType
				task.taskType = AutoCircTaskType(u32);
				ULWord * pULWords = reinterpret_cast<ULWord*>(&task.u);
				ULWord numWords(0);
				if (NTV2_IS_REGISTER_TASK(task.taskType))
					numWords = sizeof(AutoCircRegisterTask)/sizeof(ULWord);
				else if (NTV2_IS_TIMECODE_TASK(task.taskType))
					numWords = sizeof(AutoCircTimeCodeTask)/sizeof(ULWord);
				for (ULWord word(0);  word < numWords;  word++)
					POPU32(pULWords[word], inBlob, inOutIndex);
			}
		return true;
	}

	bool FRAME_STAMP_STRUCT::RPCEncode (UByteSequence & outBlob)
	{
		PUSHU16(UWord(channelSpec), outBlob);					//	NTV2Crosspoint		channelSpec
		PUSHU64(ULWord64(frameTime), outBlob);					//	LWord64				frameTime
		PUSHU32(frame, outBlob);								//	ULWord				frame
		PUSHU64(audioClockTimeStamp, outBlob);					//	ULWord64			audioClockTimeStamp
		PUSHU32(audioExpectedAddress, outBlob);					//	ULWord				audioExpectedAddress
		PUSHU32(audioInStartAddress, outBlob);					//	ULWord				audioInStartAddress
		PUSHU32(audioInStopAddress, outBlob);					//	ULWord				audioInStopAddress
		PUSHU32(audioOutStopAddress, outBlob);					//	ULWord				audioOutStopAddress
		PUSHU32(audioOutStartAddress, outBlob);					//	ULWord				audioOutStartAddress
		PUSHU32(bytesRead, outBlob);							//	ULWord				bytesRead
		PUSHU32(startSample, outBlob);							//	ULWord				startSample
		PUSHU64(ULWord64(currentTime), outBlob);				//	LWord64				currentTime
		PUSHU32(currentFrame, outBlob);							//	ULWord				currentFrame
		currentRP188.RPCEncode(outBlob);						//	RP188_STRUCT		currentRP188
		PUSHU64(ULWord64(currentFrameTime), outBlob);			//	LWord64				currentFrameTime
		PUSHU64(audioClockCurrentTime, outBlob);				//	ULWord64			audioClockCurrentTime
		PUSHU32(currentAudioExpectedAddress, outBlob);			//	ULWord				currentAudioExpectedAddress
		PUSHU32(currentAudioStartAddress, outBlob);				//	ULWord				currentAudioStartAddress
		PUSHU32(currentFieldCount, outBlob);					//	ULWord				currentFieldCount
		PUSHU32(currentLineCount, outBlob);						//	ULWord				currentLineCount
		PUSHU32(currentReps, outBlob);							//	ULWord				currentReps
		PUSHU32(currenthUser, outBlob);							//	ULWord				currenthUser
		return true;
	}

	bool FRAME_STAMP_STRUCT::RPCDecode (const UByteSequence & inBlob, size_t & inOutIndex)
	{	uint16_t v16(0);  uint64_t v64(0);
		POPU16(v16, inBlob, inOutIndex);						//	NTV2Crosspoint		channelSpec
		channelSpec = NTV2Crosspoint(v16);
		POPU64(v64, inBlob, inOutIndex);						//	LWord64				frameTime
		frameTime = LWord64(v64);
		POPU32(frame, inBlob, inOutIndex);						//	ULWord				frame
		POPU64(audioClockTimeStamp, inBlob, inOutIndex);		//	ULWord64			audioClockTimeStamp
		POPU32(audioExpectedAddress, inBlob, inOutIndex);		//	ULWord				audioExpectedAddress
		POPU32(audioInStartAddress, inBlob, inOutIndex);		//	ULWord				audioInStartAddress
		POPU32(audioInStopAddress, inBlob, inOutIndex);			//	ULWord				audioInStopAddress
		POPU32(audioOutStopAddress, inBlob, inOutIndex);		//	ULWord				audioOutStopAddress
		POPU32(audioOutStartAddress, inBlob, inOutIndex);		//	ULWord				audioOutStartAddress
		POPU32(bytesRead, inBlob, inOutIndex);					//	ULWord				bytesRead
		POPU32(startSample, inBlob, inOutIndex);				//	ULWord				startSample
		POPU64(v64, inBlob, inOutIndex);						//	LWord64				currentTime
		currentTime = LWord64(v64);
		POPU32(currentFrame, inBlob, inOutIndex);				//	ULWord				currentFrame
		currentRP188.RPCDecode(inBlob, inOutIndex);				//	RP188_STRUCT		currentRP188
		POPU64(v64, inBlob, inOutIndex);						//	LWord64				currentFrameTime
		currentFrameTime = LWord64(v64);
		POPU64(audioClockCurrentTime, inBlob, inOutIndex);		//	ULWord64			audioClockCurrentTime
		POPU32(currentAudioExpectedAddress, inBlob, inOutIndex);//	ULWord				currentAudioExpectedAddress
		POPU32(currentAudioStartAddress, inBlob, inOutIndex);	//	ULWord				currentAudioStartAddress
		POPU32(currentFieldCount, inBlob, inOutIndex);			//	ULWord				currentFieldCount
		POPU32(currentLineCount, inBlob, inOutIndex);			//	ULWord				currentLineCount
		POPU32(currentReps, inBlob, inOutIndex);				//	ULWord				currentReps
		POPU32(currenthUser, inBlob, inOutIndex);				//	ULWord				currenthUser
		return true;
	}

	bool AUTOCIRCULATE_DATA::RPCEncode (UByteSequence & outBlob)
	{
		PUSHU16(UWord(eCommand), outBlob);						//	AUTO_CIRC_COMMAND		eCommand
		PUSHU16(UWord(channelSpec), outBlob);					//	NTV2Crosspoint			channelSpec
		PUSHU32(ULWord(lVal1), outBlob);						//	LWord					lVal1
		PUSHU32(ULWord(lVal2), outBlob);						//	LWord					lVal2
		PUSHU32(ULWord(lVal3), outBlob);						//	LWord					lVal3
		PUSHU32(ULWord(lVal4), outBlob);						//	LWord					lVal4
		PUSHU32(ULWord(lVal5), outBlob);						//	LWord					lVal5
		PUSHU32(ULWord(lVal6), outBlob);						//	LWord					lVal6
		PUSHU8(bVal1, outBlob);									//	BOOL_					bVal1
		PUSHU8(bVal2, outBlob);									//	BOOL_					bVal2
		PUSHU8(bVal3, outBlob);									//	BOOL_					bVal3
		PUSHU8(bVal4, outBlob);									//	BOOL_					bVal4
		PUSHU8(bVal5, outBlob);									//	BOOL_					bVal5
		PUSHU8(bVal6, outBlob);									//	BOOL_					bVal6
		PUSHU8(bVal7, outBlob);									//	BOOL_					bVal7
		PUSHU8(bVal8, outBlob);									//	BOOL_					bVal8
		PUSHU64(ULWord64(pvVal1), outBlob);						//	void*					pvVal1
		PUSHU64(ULWord64(pvVal2), outBlob);						//	void*					pvVal2
		PUSHU64(ULWord64(pvVal3), outBlob);						//	void*					pvVal3
		PUSHU64(ULWord64(pvVal4), outBlob);						//	void*					pvVal4
		if (eCommand == eGetAutoCirc  &&  pvVal1)
			reinterpret_cast<AUTOCIRCULATE_STATUS_STRUCT*>(pvVal1)->RPCEncode(outBlob);
		if ((eCommand == eGetFrameStamp  ||  eCommand == eGetFrameStampEx2)  &&  pvVal1)
			reinterpret_cast<FRAME_STAMP_STRUCT*>(pvVal1)->RPCEncode(outBlob);
		if (eCommand == eGetFrameStampEx2  &&  pvVal2)
			reinterpret_cast<AUTOCIRCULATE_TASK_STRUCT*>(pvVal2)->RPCEncode(outBlob);
		if (eCommand == eTransferAutoCirculate  &&  pvVal1)
			reinterpret_cast<AUTOCIRCULATE_TRANSFER_STRUCT*>(pvVal1)->RPCEncode(outBlob);
		return true;
	}

	bool AUTOCIRCULATE_DATA::RPCDecode (const UByteSequence & inBlob, size_t & inOutIndex)
	{
#if defined(AJA_LINUX)
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif
		uint16_t v16(0);	uint32_t v32(0);
		POPU16(v16, inBlob, inOutIndex);						//	AUTO_CIRC_COMMAND		eCommand
		eCommand = AUTO_CIRC_COMMAND(v16);
		POPU16(v16, inBlob, inOutIndex);						//	NTV2Crosspoint			channelSpec
		channelSpec = NTV2Crosspoint(v16);
		POPU32(v32, inBlob, inOutIndex);	lVal1 = LWord(v32);	//	LWord					lVal1
		POPU32(v32, inBlob, inOutIndex);	lVal2 = LWord(v32);	//	LWord					lVal2
		POPU32(v32, inBlob, inOutIndex);	lVal3 = LWord(v32);	//	LWord					lVal3
		POPU32(v32, inBlob, inOutIndex);	lVal4 = LWord(v32);	//	LWord					lVal4
		POPU32(v32, inBlob, inOutIndex);	lVal5 = LWord(v32);	//	LWord					lVal5
		POPU32(v32, inBlob, inOutIndex);	lVal6 = LWord(v32);	//	LWord					lVal6
		POPU8(AsU8Ref(bVal1), inBlob, inOutIndex);				//	BOOL_					bVal1
		POPU8(AsU8Ref(bVal2), inBlob, inOutIndex);				//	BOOL_					bVal2
		POPU8(AsU8Ref(bVal3), inBlob, inOutIndex);				//	BOOL_					bVal3
		POPU8(AsU8Ref(bVal4), inBlob, inOutIndex);				//	BOOL_					bVal4
		POPU8(AsU8Ref(bVal5), inBlob, inOutIndex);				//	BOOL_					bVal5
		POPU8(AsU8Ref(bVal6), inBlob, inOutIndex);				//	BOOL_					bVal6
		POPU8(AsU8Ref(bVal7), inBlob, inOutIndex);				//	BOOL_					bVal7
		POPU8(AsU8Ref(bVal8), inBlob, inOutIndex);				//	BOOL_					bVal8
		POPU64(AsU64Ref(pvVal1), inBlob, inOutIndex);			//	void*					pvVal1
		POPU64(AsU64Ref(pvVal2), inBlob, inOutIndex);			//	void*					pvVal2
		POPU64(AsU64Ref(pvVal3), inBlob, inOutIndex);			//	void*					pvVal3
		POPU64(AsU64Ref(pvVal4), inBlob, inOutIndex);			//	void*					pvVal4
#if defined(AJA_LINUX)
	#pragma GCC diagnostic pop
#endif
		if (eCommand == eGetAutoCirc  &&  pvVal1)
			reinterpret_cast<AUTOCIRCULATE_STATUS_STRUCT*>(pvVal1)->RPCDecode(inBlob, inOutIndex);
		if ((eCommand == eGetFrameStamp  ||  eCommand == eGetFrameStampEx2)  &&  pvVal1)
			reinterpret_cast<FRAME_STAMP_STRUCT*>(pvVal1)->RPCDecode(inBlob, inOutIndex);
		if (eCommand == eGetFrameStampEx2  &&  pvVal2)
			reinterpret_cast<AUTOCIRCULATE_TASK_STRUCT*>(pvVal2)->RPCDecode(inBlob, inOutIndex);
		if (eCommand == eTransferAutoCirculate  &&  pvVal1)
			reinterpret_cast<AUTOCIRCULATE_TRANSFER_STRUCT*>(pvVal1)->RPCDecode(inBlob, inOutIndex);
		return true;
	}
