/**
	@file		ntv2publicinterface.cpp
	@brief		Implementations of methods declared in 'ntv2publicinterface.h'.
	@copyright	(C) 2016-2016 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#include "ntv2publicinterface.h"
#include "ntv2devicefeatures.h"
#include "ntv2utils.h"
#include <iomanip>
#include <assert.h>
#include <string.h>		//	For memset, et al.
#include "ntv2rp188.h"
using namespace std;


NTV2_RP188::NTV2_RP188 (const ULWord inDBB, const ULWord inLow, const ULWord inHigh)
	:	fDBB	(inDBB),
		fLo		(inLow),
		fHi		(inHigh)
{
}


NTV2_RP188::NTV2_RP188 (const RP188_STRUCT & inOldRP188)
	:	fDBB	(inOldRP188.DBB),
		fLo		(inOldRP188.Low),
		fHi		(inOldRP188.High)
{
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
	mFrameRefClockCount	= 0;
	mGlobalClockCount	= 0;
	mFrameTRSError		= false;
	mLocked				= false;
	mVPIDValidA			= false;
	mVPIDValidB			= false;
}


ostream & NTV2SDIInputStatus::Print (ostream & inOutStream) const
{
	inOutStream	<< "[CRCA="			<< mCRCTallyA
				<< " CRCB="			<< mCRCTallyB
				<< " unlk="			<< mUnlockTally
				<< " frmRefClkCnt="	<< mFrameRefClockCount
				<< " globalClkCnt="	<< mGlobalClockCount
				<< " frmTRS="		<< mFrameTRSError
				<< " locked="		<< mLocked
				<< " VPIDA="		<< mVPIDValidA
				<< " VPIDB="		<< mVPIDValidB
				<< "]";
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
		channelSpec	(inCrosspoint),
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
		pvVal1		(NULL),
		pvVal2		(NULL),
		pvVal3		(NULL),
		pvVal4		(NULL)
{
}


NTV2_HEADER::NTV2_HEADER (const ULWord inStructureType, const ULWord inStructSizeInBytes)
	:	fHeaderTag		(NTV2_HEADER_TAG),
		fType			(inStructureType),
		fHeaderVersion	(NTV2_CURRENT_HEADER_VERSION),
		fVersion		(AUTOCIRCULATE_STRUCT_VERSION),
		fSizeInBytes	(inStructSizeInBytes),
		fPointerSize	(sizeof (int *)),
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
		inOutStream << "BAD-" << hex << fHeaderTag << dec;
	if (NTV2_IS_VALID_STRUCT_TYPE (fType))
		inOutStream << NTV2_4CC_AS_STRING (fType);
	else
		inOutStream << "|BAD-" << hex << fType << dec;
	inOutStream << " v" << fHeaderVersion << " vers=" << fVersion << " sz=" << fSizeInBytes;
	return inOutStream << "]";
}


ostream & operator << (ostream & inOutStream, const NTV2_TRAILER & inObj)
{
	inOutStream << "[";
	if (NTV2_IS_VALID_TRAILER_TAG (inObj.fTrailerTag))
		inOutStream << NTV2_4CC_AS_STRING (inObj.fTrailerTag);
	else
		inOutStream << "BAD-" << hex << inObj.fTrailerTag << dec;
	return inOutStream << " v" << inObj.fTrailerVersion << "]";
}


ostream & operator << (ostream & inOutStream, const NTV2_POINTER & inObj)
{
	return inObj.Print (inOutStream);
}


ostream & NTV2_POINTER::Print (ostream & inOutStream) const
{
	inOutStream << (fFlags & NTV2_POINTER_ALLOCATED ? "0X" : "0x") << hex << GetRawHostPointer () << dec << "/" << GetByteCount ();
	return inOutStream;
}


ostream & operator << (ostream & inOutStream, const NTV2_RP188 & inObj)
{
	if (inObj.IsValid ())
		return inOutStream << "{DBx" << hex << inObj.fDBB << dec << "|LOx" << hex << inObj.fLo << dec << "|HIx" << hex << inObj.fHi << dec << "}";
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
	for (NTV2TimeCodesConstIter iter (inObj.begin ());  iter != inObj.end ();  )
	{
		inOutStream << ::NTV2TCIndexToString (iter->first) << " => " << iter->second;
		if (++iter != inObj.end ())
			inOutStream << ",  ";
	}
	return inOutStream << "]";
}


ostream & operator << (std::ostream & inOutStream, const NTV2TCIndexes & inObj)
{
	for (NTV2TCIndexesConstIter iter (inObj.begin ());  iter != inObj.end ();  )
	{
		inOutStream << ::NTV2TCIndexToString (*iter);
		if (++iter != inObj.end ())
			inOutStream << ", ";
	}
	return inOutStream;
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


ostream & operator << (ostream & inOutStream, const AUTOCIRCULATE_STATUS & inObj)
{
	string	str	(::NTV2CrosspointToString (inObj.acCrosspoint));
	while (str.find (' ') != string::npos)
		str.erase (str.find (' '), 1);
	inOutStream << inObj.acHeader << " " << str << " " << ::NTV2AutoCirculateStateToString (inObj.acState);
	if (inObj.acState != NTV2_AUTOCIRCULATE_DISABLED)
	{
		inOutStream	<< " startFrm=" << inObj.acStartFrame
					<< " endFrm=" << inObj.acEndFrame
					<< " actFrm=" << inObj.acActiveFrame
					<< " proc=" << inObj.acFramesProcessed
					<< " drop=" << inObj.acFramesDropped
					<< " bufLvl=" << inObj.acBufferLevel;
		if (NTV2_IS_VALID_AUDIO_SYSTEM (inObj.acAudioSystem))
			inOutStream	<< " +" << ::NTV2AudioSystemToString (inObj.acAudioSystem, true);
		inOutStream	<< (inObj.acOptionFlags & AUTOCIRCULATE_WITH_RP188			? " +RP188" : "")
					<< (inObj.acOptionFlags & AUTOCIRCULATE_WITH_LTC			? " +LTC" : "")
					<< (inObj.acOptionFlags & AUTOCIRCULATE_WITH_FBFCHANGE		? " +FBFCHG" : "")
					<< (inObj.acOptionFlags & AUTOCIRCULATE_WITH_FBOCHANGE		? " +FBOCHG" : "")
					<< (inObj.acOptionFlags & AUTOCIRCULATE_WITH_COLORCORRECT	? " +COLORCORR" : "")
					<< (inObj.acOptionFlags & AUTOCIRCULATE_WITH_VIDPROC		? " +VIDPROC" : "")
					<< (inObj.acOptionFlags & AUTOCIRCULATE_WITH_ANC			? " +ANC" : "");
					//	<< inObj.acRDTSCStartTime
					//	<< inObj.acAudioClockStartTime
					//	<< inObj.acRDTSCCurrentTime
					//	<< inObj.acAudioClockCurrentTime
	}
	return inOutStream << " " << inObj.acTrailer;
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
	string	str	(::NTV2FrameBufferFormatToString (inObj.acFrameBufferFormat, true));
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
	inOutStream	<< inObj.acHeader << " state=" << ::NTV2AutoCirculateStateToString (inObj.acState)
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
		const NTV2RegisterNumber	registerNumber	(static_cast <const NTV2RegisterNumber> (iter->first));
		const ULWord				registerValue	(iter->second);
		inOutStream	<< ::NTV2RegisterNumberToString (registerNumber) << "=0x" << hex << registerValue << dec;
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
ostream & operator << (ostream & inOStream, const NTV2FrameBufferFormatSet & inFormats)
{
	NTV2FrameBufferFormatSetConstIter	iter	(inFormats.begin ());

	inOStream	<< inFormats.size ()
				<< (inFormats.size () == 1 ? " pixel format:  " : " pixel formats:  ");

	while (iter != inFormats.end ())
	{
		inOStream << ::NTV2FrameBufferFormatToString (*iter);
		inOStream << (++iter == inFormats.end ()  ?  ""  :  ", ");
	}

	return inOStream;

}	//	operator <<


NTV2FrameBufferFormatSet & operator += (NTV2FrameBufferFormatSet & inOutSet, const NTV2FrameBufferFormatSet inFBFs)
{
	for (NTV2FrameBufferFormatSetConstIter iter (inFBFs.begin ());  iter != inFBFs.end ();  ++iter)
		inOutSet.insert (*iter);
	return inOutSet;
}


//	Implementation of NTV2FrameBufferFormatSet's ostream writer...
ostream & operator << (ostream & inOStream, const NTV2InputSourceSet & inSet)
{
	NTV2InputSourceSetConstIter	iter	(inSet.begin ());

	inOStream	<< inSet.size ()
				<< (inSet.size () == 1 ? " input source:  " : " input sources:  ");

	while (iter != inSet.end ())
	{
		inOStream << ::NTV2InputSourceToString (*iter);
		inOStream << (++iter == inSet.end ()  ?  ""  :  ", ");
	}

	return inOStream;

}	//	operator <<


NTV2InputSourceSet & operator += (NTV2InputSourceSet & inOutSet, const NTV2InputSourceSet inSet)
{
	for (NTV2InputSourceSetConstIter iter (inSet.begin ());  iter != inSet.end ();  ++iter)
		inOutSet.insert (*iter);
	return inOutSet;
}


//	Implementation of NTV2AutoCirculateStateToString...
string NTV2AutoCirculateStateToString (const NTV2AutoCirculateState inState)
{
	static const char *	sStateStrings []	= {	"Disabled", "Initializing", "Starting", "Paused", "Stopping", "Running", "StartingAtTime", NULL};
	if (inState >= NTV2_AUTOCIRCULATE_DISABLED && inState <= NTV2_AUTOCIRCULATE_STARTING_AT_TIME)
		return string (sStateStrings [inState]);
	else
		return "<invalid>";
}



NTV2_TRAILER::NTV2_TRAILER ()
	:	fTrailerVersion		(NTV2_CURRENT_TRAILER_VERSION),
		fTrailerTag			(NTV2_TRAILER_TAG)
{
}


NTV2_POINTER::NTV2_POINTER (const void * pInUserPointer, const size_t inByteCount)
	:	fUserSpacePtr		(NTV2_POINTER_TO_ULWORD64 (pInUserPointer)),
		fByteCount			(static_cast <ULWord> (inByteCount)),
		fFlags				(0),
	#if defined (AJAMac)
		fKernelSpacePtr		(0),
		fIOMemoryDesc		(0),
		fIOMemoryMap		(0)
	#else
		fKernelHandle		(0)
	#endif
{
}


NTV2_POINTER::NTV2_POINTER (const size_t inByteCount)
	:	fUserSpacePtr		(0),
		fByteCount			(0),
		fFlags				(0),
	#if defined (AJAMac)
		fKernelSpacePtr		(0),
		fIOMemoryDesc		(0),
		fIOMemoryMap		(0)
	#else
		fKernelHandle		(0)
	#endif
{
	if (inByteCount)
		if (Allocate (inByteCount))
			Fill (UByte (0));
}


NTV2_POINTER::NTV2_POINTER (const NTV2_POINTER & inObj)
	:	fUserSpacePtr		(0),
		fByteCount			(0),
		fFlags				(0),
	#if defined (AJAMac)
		fKernelSpacePtr		(0),
		fIOMemoryDesc		(0),
		fIOMemoryMap		(0)
	#else
		fKernelHandle		(0)
	#endif
{
	if (Allocate (inObj.GetByteCount ()))
		SetFrom (inObj);
}


NTV2_POINTER & NTV2_POINTER::operator = (const NTV2_POINTER & inRHS)
{
	if (&inRHS != this)
	{
		if (inRHS.IsNULL ())
			Set (NULL, 0);
		else
		{
			if (Allocate (inRHS.GetByteCount ()))
				SetFrom (inRHS);
		}
	}
	return *this;
}


NTV2_POINTER::~NTV2_POINTER ()
{
	Set (NULL, 0);	//	Call 'Set' to delete the array (if I allocated it)
}


bool NTV2_POINTER::Set (const void * pInUserPointer, const size_t inByteCount)
{
	if (fFlags & NTV2_POINTER_ALLOCATED)
	{
		if (GetHostPointer () && GetByteCount ())
			delete [] (UByte *) GetHostPointer ();
		fUserSpacePtr = 0;
		fByteCount = 0;
		fFlags &= ~NTV2_POINTER_ALLOCATED;
	}
	fUserSpacePtr = NTV2_POINTER_TO_ULWORD64 (pInUserPointer);
	fByteCount = static_cast <ULWord> (inByteCount);
	return true;
}


bool NTV2_POINTER::SetAndFill (const void * pInUserPointer, const size_t inByteCount, const UByte inValue)
{
	if (Set (pInUserPointer, inByteCount))
	{
		Fill (inValue);
		return true;
	}
	return false;
}


bool NTV2_POINTER::Allocate (const size_t inByteCount)
{
	if (GetByteCount ()  &&  fFlags & NTV2_POINTER_ALLOCATED)	//	If already was Allocated
		if (inByteCount == GetByteCount ())						//	If same byte count
		{
			::memset (GetHostPointer (), 0, GetByteCount ());	//	Just zero it
			return true;										//	And return true
		}

	bool	result	(false);
	if (inByteCount)
	{
		//	Allocate the byte array, and call Set...
		result = Set (new UByte [inByteCount], inByteCount);
		fFlags |= NTV2_POINTER_ALLOCATED;	//	Important:  set this flag so I can delete the array later
	}
	else
		result = Set (NULL, 0);
	return result;
}


void NTV2_POINTER::Fill (const UByte inValue)
{
	UByte *	pBytes	(reinterpret_cast <UByte *> (GetHostPointer ()));
	if (pBytes)
		::memset (pBytes, inValue, GetByteCount ());
}


void NTV2_POINTER::Fill (const UWord inValue)
{
	UWord *		pUWords		(reinterpret_cast <UWord *> (GetHostPointer ()));
	size_t		loopCount	(GetByteCount () / sizeof (inValue));
	if (pUWords)
		for (size_t n (0);  n < loopCount;  n++)
			pUWords [n] = inValue;
}


void NTV2_POINTER::Fill (const ULWord inValue)
{
	ULWord *	pULWords	(reinterpret_cast <ULWord *> (GetHostPointer ()));
	size_t		loopCount	(GetByteCount () / sizeof (inValue));
	if (pULWords)
		for (size_t n (0);  n < loopCount;  n++)
			pULWords [n] = inValue;
}


void NTV2_POINTER::Fill (const ULWord64 inValue)
{
	ULWord64 *	pULWord64s	(reinterpret_cast <ULWord64 *> (GetHostPointer ()));
	size_t		loopCount	(GetByteCount () / sizeof (inValue));
	if (pULWord64s)
		for (size_t n (0);  n < loopCount;  n++)
			pULWord64s [n] = inValue;
}


void * NTV2_POINTER::GetHostAddress (const ULWord inByteOffset, const bool inFromEnd) const
{
	if (IsNULL())
		return NULL;
	if (inByteOffset >= GetByteCount())
		return NULL;
	UByte *	pBytes	(reinterpret_cast <UByte *> (GetHostPointer ()));
	if (inFromEnd)
		pBytes += GetByteCount() - inByteOffset;
	else
		pBytes += inByteOffset;
	return pBytes;
}


bool NTV2_POINTER::SetFrom (const NTV2_POINTER & inBuffer)
{
	if (inBuffer.IsNULL ())
		return false;	//	NULL or empty
	if (IsNULL ())
		return false;	//	I am NULL or empty
	if (inBuffer.GetByteCount () == GetByteCount ()  &&  inBuffer.GetHostPointer () == GetHostPointer ())
		return true;	//	Same buffer

	size_t	bytesToCopy	(inBuffer.GetByteCount ());
	if (bytesToCopy > GetByteCount ())
		bytesToCopy = GetByteCount ();
	::memcpy (GetHostPointer (), inBuffer.GetHostPointer (), bytesToCopy);
	return true;
}


bool NTV2_POINTER::CopyFrom (const void * pInSrcBuffer, const ULWord inByteCount)
{
	if (!inByteCount)
		return Set (NULL, 0);	//	Zero bytes
	if (!pInSrcBuffer)
		return false;	//	NULL src ptr
	if (!Allocate (inByteCount))
		return false;	//	Resize failed
	::memcpy (GetHostPointer(), pInSrcBuffer, inByteCount);
	return true;
}


bool NTV2_POINTER::CopyFrom (const NTV2_POINTER & inBuffer,
							const ULWord inSrcByteOffset, const ULWord inDstByteOffset, const ULWord inByteCount)
{
	if (inBuffer.IsNULL () || IsNULL ())
		return false;	//	NULL or empty
	if (inSrcByteOffset + inByteCount > inBuffer.GetByteCount ())
		return false;	//	Past end of src
	if (inDstByteOffset + inByteCount > GetByteCount ())
		return false;	//	Past end of me

	const UByte *	pSrc	(reinterpret_cast <const UByte *> (inBuffer.GetHostPointer ()));
	pSrc += inSrcByteOffset;

	UByte *			pDst	(reinterpret_cast <UByte *> (GetHostPointer ()));
	pDst += inDstByteOffset;

	::memcpy (pDst, pSrc, inByteCount);
	return true;
}


bool NTV2_POINTER::SwapWith (NTV2_POINTER & inBuffer)
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


bool NTV2_POINTER::IsContentEqual (const NTV2_POINTER & inBuffer, const ULWord inByteOffset, const ULWord inByteCount) const
{
	if (IsNULL () || inBuffer.IsNULL ())
		return false;	//	NULL or empty
	if (inBuffer.GetByteCount () != GetByteCount ())
		return false;	//	Different byte counts
	if (inBuffer.GetHostPointer () == GetHostPointer ())
		return true;	//	Same buffer

	ULWord	totalBytes	(GetByteCount());
	if (inByteOffset >= totalBytes)
		return false;	//	Bad offset

	totalBytes -= inByteOffset;

	ULWord	byteCount	(inByteCount);
	if (byteCount > totalBytes)
		byteCount = totalBytes;

	const UByte *	pByte1 (reinterpret_cast <const UByte *> (GetHostPointer()));
	const UByte *	pByte2 (reinterpret_cast <const UByte *> (inBuffer.GetHostPointer()));
	pByte1 += inByteOffset;
	pByte2 += inByteOffset;
	return ::memcmp (pByte1, pByte2, byteCount) == 0;
}


bool NTV2_POINTER::GetRingChangedByteRange (const NTV2_POINTER & inBuffer, ULWord & outByteOffsetFirst, ULWord & outByteOffsetLast) const
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
			cerr << "## WARNING:  GetRingChangedByteRange:  last " << outByteOffsetLast << " <= first " << outByteOffsetFirst << " in wrap condition" << endl;
		const ULWord	tmp	(outByteOffsetLast);
		outByteOffsetLast = outByteOffsetFirst;
		outByteOffsetFirst = tmp;
		if (outByteOffsetLast >= outByteOffsetFirst)
			cerr << "## WARNING:  GetRingChangedByteRange:  last " << outByteOffsetLast << " >= first " << outByteOffsetFirst << " in wrap condition" << endl;
	}
	return true;

}	//	GetRingChangedByteRange


FRAME_STAMP::FRAME_STAMP ()
	:	acHeader						(AUTOCIRCULATE_TYPE_FRAMESTAMP, sizeof (FRAME_STAMP)),
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

	for (ULWord ndx (0);  ndx < numRP188s;  ndx++)
		outValues << pArray [ndx];

	return true;
}


bool FRAME_STAMP::GetInputTimeCode (NTV2_RP188 & outTimeCode, const NTV2TCIndex inTCIndex) const
{
	NTV2_ASSERT_STRUCT_VALID;
	ULWord				numRP188s	(acTimeCodes.GetByteCount () / sizeof (NTV2_RP188));
	const NTV2_RP188 *	pArray		(reinterpret_cast <const NTV2_RP188 *> (acTimeCodes.GetHostPointer ()));
	outTimeCode.Set ();	//	invalidate
	if (!pArray)
		return false;		//	No 'acTimeCodes' array!
	if (numRP188s > NTV2_MAX_NUM_TIMECODE_INDEXES)
		numRP188s = NTV2_MAX_NUM_TIMECODE_INDEXES;	//	clamp to this max number
	if (!NTV2_IS_VALID_TIMECODE_INDEX (inTCIndex))
		return false;

	outTimeCode = pArray [inTCIndex];
	return true;
}

bool FRAME_STAMP::GetSDIInputStatus(NTV2SDIInputStatus & outStatus, const UWord inSDIInputIndex0) const
{
	NTV2_ASSERT_STRUCT_VALID;
	(void)outStatus;
	(void)inSDIInputIndex0;
	return true;
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
    outOldStruct.currenthUser					= (ULWord)acCurrentUserCookie;
	outOldStruct.currentRP188					= acRP188;
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
	:	mHeader(AUTOCIRCULATE_TYPE_SDISTATS, sizeof(NTV2SDIInStatistics)),
		mInStatistics(NTV2_MAX_NUM_CHANNELS * sizeof(NTV2SDIInputStatus))
{
	NTV2SDIInputStatus * pArray(reinterpret_cast <NTV2SDIInputStatus *> (mInStatistics.GetHostPointer()));
	for (int i = 0; i < NTV2_MAX_NUM_CHANNELS; i++)
		pArray[i].Clear();
	NTV2_ASSERT_STRUCT_VALID;
}

void NTV2SDIInStatistics::Clear(void)
{
	NTV2_ASSERT_STRUCT_VALID;
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

std::ostream &	NTV2SDIInStatistics::Print(std::ostream & inOutStream) const
{
	NTV2_ASSERT_STRUCT_VALID;
	inOutStream << mHeader << ", " << mInStatistics << ", " << mTrailer; return inOutStream;
}


AUTOCIRCULATE_TRANSFER_STATUS::AUTOCIRCULATE_TRANSFER_STATUS ()
	:	acHeader					(AUTOCIRCULATE_TYPE_XFERSTATUS, sizeof (AUTOCIRCULATE_TRANSFER_STATUS)),
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
	:	acHeader				(AUTOCIRCULATE_TYPE_STATUS, sizeof (AUTOCIRCULATE_STATUS)),
		acCrosspoint			(inCrosspoint),
		acState					(NTV2_AUTOCIRCULATE_DISABLED),
		acStartFrame			(0),
		acEndFrame				(0),
		acActiveFrame			(0),
		acRDTSCStartTime		(0),
		acAudioClockStartTime	(0),
		acRDTSCCurrentTime		(0),
		acAudioClockCurrentTime	(0),
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
	acAudioClockCurrentTime	= inOldStruct.audioClockCurrentTime;
	acFramesProcessed		= inOldStruct.framesProcessed;
	acFramesDropped			= inOldStruct.framesDropped;
	acBufferLevel			= inOldStruct.bufferLevel;
	acAudioSystem			= NTV2_AUDIOSYSTEM_INVALID;	//	NTV2_AUDIOSYSTEM_1;
	acOptionFlags			=	inOldStruct.bWithRP188				? AUTOCIRCULATE_WITH_RP188			: 0		|
								inOldStruct.bFbfChange				? AUTOCIRCULATE_WITH_FBFCHANGE		: 0		|
								inOldStruct.bFboChange				? AUTOCIRCULATE_WITH_FBOCHANGE		: 0		|
								inOldStruct.bWithColorCorrection	? AUTOCIRCULATE_WITH_COLORCORRECT	: 0		|
								inOldStruct.bWithVidProc			? AUTOCIRCULATE_WITH_VIDPROC		: 0		|
								inOldStruct.bWithCustomAncData		? AUTOCIRCULATE_WITH_ANC			: 0;
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
	acAudioClockCurrentTime	= 0;
	acFramesProcessed		= 0;
	acFramesDropped			= 0;
	acBufferLevel			= 0;
	acOptionFlags			= 0;
	acAudioSystem			= NTV2_AUDIOSYSTEM_INVALID;
}


NTV2Channel AUTOCIRCULATE_STATUS::GetChannel (void) const
{
	return ::NTV2CrosspointToNTV2Channel (acCrosspoint);
}


string AUTOCIRCULATE_STATUS::operator [] (const unsigned inIndexNum) const
{
	ostringstream	oss;
	if (inIndexNum == 0)
		oss << ::NTV2AutoCirculateStateToString (acState);
	else if (!IsStopped())
		switch (inIndexNum)
		{
			case 1:		oss << dec << acStartFrame;					break;
			case 2:		oss << dec << acEndFrame;					break;
			case 3:		oss << dec << acActiveFrame;				break;
			case 4:		oss << dec << acRDTSCStartTime;				break;
			case 5:		oss << dec << acAudioClockStartTime;		break;
			case 6:		oss << dec << acRDTSCCurrentTime;			break;
			case 7:		oss << dec << acAudioClockCurrentTime;		break;
			case 8:		oss << dec << acFramesProcessed;			break;
			case 9:		oss << dec << acFramesDropped;				break;
			case 10:	oss << dec << acBufferLevel;				break;
			case 11:	if (NTV2_IS_VALID_AUDIO_SYSTEM (acAudioSystem))
							oss << ::NTV2AudioSystemToString (acAudioSystem, true);
						else
							oss << "NoAudio";
						break;
			case 12:	oss << dec << (acOptionFlags & AUTOCIRCULATE_WITH_RP188			? "Yes" : "No");	break;
			case 13:	oss << dec << (acOptionFlags & AUTOCIRCULATE_WITH_LTC			? "Yes" : "No");	break;
			case 14:	oss << dec << (acOptionFlags & AUTOCIRCULATE_WITH_FBFCHANGE		? "Yes" : "No");	break;
			case 15:	oss << dec << (acOptionFlags & AUTOCIRCULATE_WITH_FBOCHANGE		? "Yes" : "No");	break;
			case 16:	oss << dec << (acOptionFlags & AUTOCIRCULATE_WITH_COLORCORRECT	? "Yes" : "No");	break;
			case 17:	oss << dec << (acOptionFlags & AUTOCIRCULATE_WITH_VIDPROC		? "Yes" : "No");	break;
			case 18:	oss << dec << (acOptionFlags & AUTOCIRCULATE_WITH_ANC			? "Yes" : "No");	break;
			default:																						break;
		}
	else if (inIndexNum < 19)
		oss << "---";
	return oss.str();
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
		ccSaturationValue	(0),
		ccLookupTables		(NULL, 0)
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
	if (ccLookupTables.GetHostPointer ())
		delete [] (UByte *) ccLookupTables.GetHostPointer ();
	ccLookupTables.Set (NULL, 0);
}


bool NTV2ColorCorrectionData::Set (const NTV2ColorCorrectionMode inMode, const ULWord inSaturation, const void * pInTableData)
{
	Clear ();
	if (!NTV2_IS_VALID_COLOR_CORRECTION_MODE (inMode))
		return false;

	if (pInTableData)
	{
		if (!ccLookupTables.Set (new UByte [NTV2_COLORCORRECTOR_TABLESIZE], NTV2_COLORCORRECTOR_TABLESIZE))
			return false;
	}
	ccMode = inMode;
	ccSaturationValue = (inMode == NTV2_CCMODE_3WAY) ? inSaturation : 0;
	return true;
}


AUTOCIRCULATE_TRANSFER::AUTOCIRCULATE_TRANSFER ()
	:	acHeader					(AUTOCIRCULATE_TYPE_XFER, sizeof (AUTOCIRCULATE_TRANSFER)),
		acVideoBuffer				(NULL, 0),
		acAudioBuffer				(NULL, 0),
		acANCBuffer					(NULL, 0),
		acANCField2Buffer			(NULL, 0),
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
		acReserved001				(0),
		acPeerToPeerFlags			(0),
		acFrameRepeatCount			(1),
		acDesiredFrame				(0xFFFFFFFF),
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
	:	acHeader					(AUTOCIRCULATE_TYPE_XFER, sizeof (AUTOCIRCULATE_TRANSFER)),
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
		acReserved001				(0),
		acPeerToPeerFlags			(0),
		acFrameRepeatCount			(1),
		acDesiredFrame				(0xFFFFFFFF),
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


bool AUTOCIRCULATE_TRANSFER::SetOutputTimeCodes (const NTV2TimeCodes & inValues)
{
	NTV2_ASSERT_STRUCT_VALID;
	NTV2_RP188		invalidValue;
	ULWord			maxNumValues	(acOutputTimeCodes.GetByteCount () / sizeof (NTV2_RP188));
	NTV2_RP188 *	pArray	(reinterpret_cast <NTV2_RP188 *> (acOutputTimeCodes.GetHostPointer ()));
	if (!pArray)
		return false;
	if (maxNumValues > NTV2_MAX_NUM_TIMECODE_INDEXES)
		maxNumValues = NTV2_MAX_NUM_TIMECODE_INDEXES;

	for (UWord ndx (0);  ndx < maxNumValues;  ndx++)
	{
		const NTV2TCIndex		tcIndex	(static_cast <const NTV2TCIndex> (ndx));
		NTV2TimeCodesConstIter	iter	(inValues.find (tcIndex));
		pArray [ndx] = (iter != inValues.end ())  ?  iter->second  :  invalidValue;
	}	//	for each possible NTV2TCSource value
	return true;
}


bool AUTOCIRCULATE_TRANSFER::SetOutputTimeCode (const NTV2_RP188 & inTimeCode, const NTV2TCIndex inTCIndex)
{
	NTV2_ASSERT_STRUCT_VALID;
	ULWord			maxNumValues	(acOutputTimeCodes.GetByteCount () / sizeof (NTV2_RP188));
	NTV2_RP188 *	pArray			(reinterpret_cast <NTV2_RP188 *> (acOutputTimeCodes.GetHostPointer ()));
	if (!pArray)
		return false;
	if (maxNumValues > NTV2_MAX_NUM_TIMECODE_INDEXES)
		maxNumValues = NTV2_MAX_NUM_TIMECODE_INDEXES;
	if (!NTV2_IS_VALID_TIMECODE_INDEX (inTCIndex))
		return false;

	pArray [inTCIndex] = inTimeCode;
	return true;
}

bool AUTOCIRCULATE_TRANSFER::SetAllOutputTimeCodes (const NTV2_RP188 & inTimeCode)
{
	NTV2_ASSERT_STRUCT_VALID;
	ULWord			maxNumValues	(acOutputTimeCodes.GetByteCount () / sizeof (NTV2_RP188));
	NTV2_RP188 *	pArray			(reinterpret_cast <NTV2_RP188 *> (acOutputTimeCodes.GetHostPointer ()));
	if (!pArray)
		return false;
	if (maxNumValues > NTV2_MAX_NUM_TIMECODE_INDEXES)
		maxNumValues = NTV2_MAX_NUM_TIMECODE_INDEXES;

	for (ULWord tcIndex (0);  tcIndex < maxNumValues;  tcIndex++)
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
	SetBuffers (NULL, 0, NULL, 0, NULL, 0, NULL, 0);
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


NTV2GetRegisters::NTV2GetRegisters (const NTV2RegNumSet & inRegisterNumbers)
	:	mHeader				(AUTOCIRCULATE_TYPE_GETREGS, sizeof (NTV2GetRegisters)),
		mInNumRegisters		(ULWord (inRegisterNumbers.size ())),
		mInRegisters		(NULL, 0),
		mOutNumRegisters	(0),
		mOutGoodRegisters	(NULL, 0),
		mOutValues			(NULL, 0)
{
	NTV2_ASSERT_STRUCT_VALID;
	ResetUsing (inRegisterNumbers);
}


NTV2GetRegisters::NTV2GetRegisters (NTV2RegisterReads & inRegReads)
	:	mHeader				(AUTOCIRCULATE_TYPE_GETREGS, sizeof (NTV2GetRegisters)),
		mInNumRegisters		(ULWord (inRegReads.size ())),
		mInRegisters		(NULL, 0),
		mOutNumRegisters	(0),
		mOutGoodRegisters	(NULL, 0),
		mOutValues			(NULL, 0)
{
	NTV2_ASSERT_STRUCT_VALID;
	ResetUsing (inRegReads);
}


bool NTV2GetRegisters::ResetUsing (const NTV2RegisterReads & inRegReads)
{
	NTV2_ASSERT_STRUCT_VALID;
	mInNumRegisters = ULWord (inRegReads.size ());
	mOutNumRegisters = 0;
	const bool	result	(	mInRegisters.Allocate (mInNumRegisters * sizeof (ULWord))
						&&	mOutGoodRegisters.Allocate (mInNumRegisters * sizeof (ULWord))
						&&	mOutValues.Allocate (mInNumRegisters * sizeof (ULWord)));
	if (result)
	{
		ULWord		ndx			(0);
		ULWord *	pRegArray	(reinterpret_cast <ULWord *> (mInRegisters.GetHostPointer ()));
		assert (pRegArray);
		for (NTV2RegisterReadsConstIter iter (inRegReads.begin ());  iter != inRegReads.end ();  ++iter)
			pRegArray [ndx++] = iter->registerNumber;
		assert ((ndx * sizeof (ULWord)) == mInRegisters.GetByteCount ());
	}
	return result;
}


bool NTV2GetRegisters::ResetUsing (const NTV2RegNumSet & inRegisterNumbers)
{
	NTV2_ASSERT_STRUCT_VALID;
	mInNumRegisters = ULWord (inRegisterNumbers.size ());
	mOutNumRegisters = 0;
	const bool	result	(	mInRegisters.Allocate (mInNumRegisters * sizeof (ULWord))
						&&	mOutGoodRegisters.Allocate (mInNumRegisters * sizeof (ULWord))
						&&	mOutValues.Allocate (mInNumRegisters * sizeof (ULWord)));
	if (result)
	{
		ULWord		ndx			(0);
		ULWord *	pRegArray	(reinterpret_cast <ULWord *> (mInRegisters.GetHostPointer ()));
		assert (pRegArray);
		for (NTV2RegNumSetConstIter iter (inRegisterNumbers.begin ());  iter != inRegisterNumbers.end ();  ++iter)
			pRegArray [ndx++] = *iter;
		assert ((ndx * sizeof (ULWord)) == mInRegisters.GetByteCount ());
	}
	return result;
}


bool NTV2GetRegisters::GetGoodRegisters (NTV2RegNumSet & outGoodRegNums) const
{
	NTV2_ASSERT_STRUCT_VALID;
	outGoodRegNums.clear ();
	if (mOutGoodRegisters.GetHostPointer () == 0)
		return false;		//	No 'mOutGoodRegisters' array!
	if (mOutGoodRegisters.GetByteCount () == 0)
		return false;		//	No good registers!
	if (mOutNumRegisters == 0)
		return false;		//	No good registers!  (The driver sets this field.)
	if (mOutNumRegisters > mInNumRegisters)
		return false;		//	mOutNumRegisters must be less than or equal to mInNumRegisters!

	const ULWord *	pRegArray	(reinterpret_cast <const ULWord *> (mOutGoodRegisters.GetHostPointer ()));
	for (ULWord ndx (0);  ndx < mOutGoodRegisters.GetByteCount ();  ndx++)
		outGoodRegNums << pRegArray [ndx];

	return true;
}


bool NTV2GetRegisters::GetRegisterValues (NTV2RegisterValueMap & outValues) const
{
	NTV2_ASSERT_STRUCT_VALID;
	outValues.clear ();
	if (mOutGoodRegisters.GetHostPointer () == 0)
		return false;		//	No 'mOutGoodRegisters' array!
	if (mOutGoodRegisters.GetByteCount () == 0)
		return false;		//	No good registers!
	if (mOutNumRegisters == 0)
		return false;		//	No good registers!  (The driver sets this field.)
	if (mOutNumRegisters > mInNumRegisters)
		return false;		//	mOutNumRegisters must be less than or equal to mInNumRegisters!
	if (mOutValues.GetHostPointer () == 0)
		return false;		//	No 'mOutValues' array!
	if (mOutValues.GetByteCount () == 0)
		return false;		//	No values!
	if (mOutGoodRegisters.GetByteCount () != mOutValues.GetByteCount ())
		return false;		//	These sizes should match

	const ULWord *	pRegArray	(reinterpret_cast <const ULWord *> (mOutGoodRegisters.GetHostPointer ()));
	const ULWord *	pValArray	(reinterpret_cast <const ULWord *> (mOutValues.GetHostPointer ()));
	for (ULWord ndx (0);  ndx < mOutNumRegisters;  ndx++)
		outValues [pRegArray [ndx]] = pValArray [ndx];

	return true;
}


bool NTV2GetRegisters::GetRegisterValues (NTV2RegisterReads & outValues) const
{
	NTV2RegisterValueMap	regValues;
	if (!GetRegisterValues (regValues))
		return false;
	for (NTV2RegisterReadsIter it (outValues.begin());  it != outValues.end();  ++it)
	{
		NTV2RegValueMapConstIter	mapIter	(regValues.find (it->registerNumber));
		if (mapIter == regValues.end())
			return false;	//	Missing register
		it->registerValue = mapIter->second;
	}
	return true;
}


ostream & NTV2GetRegisters::Print (ostream & inOutStream) const
{
	NTV2_ASSERT_STRUCT_VALID;
	inOutStream	<< mHeader << ", numRegs=" << mInNumRegisters << ", inRegs=" << mInRegisters << ", outNumGoodRegs=" << mOutNumRegisters
				<< ", outGoodRegs=" << mOutGoodRegisters << ", outValues=" << mOutValues << ", " << mTrailer;
	return inOutStream;
}


NTV2SetRegisters::NTV2SetRegisters (const NTV2RegisterWrites & inRegWrites)
	:	mHeader				(AUTOCIRCULATE_TYPE_SETREGS, sizeof (NTV2SetRegisters)),
		mInNumRegisters		(ULWord (inRegWrites.size ())),
		mInRegInfos			(NULL, 0),
		mOutNumFailures		(0),
		mOutBadRegIndexes	(NULL, 0)
{
	ResetUsing (inRegWrites);
}


bool NTV2SetRegisters::ResetUsing (const NTV2RegisterWrites & inRegWrites)
{
	NTV2_ASSERT_STRUCT_VALID;
	mInNumRegisters	= ULWord (inRegWrites.size ());
	mOutNumFailures	= 0;
	const bool	result	(mInRegInfos.Allocate (mInNumRegisters * sizeof (NTV2RegInfo)) && mOutBadRegIndexes.Allocate (mInNumRegisters * sizeof (UWord)));
	if (result)
	{
		ULWord			ndx				(0);
		NTV2RegInfo *	pRegInfoArray	(reinterpret_cast <NTV2RegInfo *> (mInRegInfos.GetHostPointer ()));
		UWord *			pBadRegIndexes	(reinterpret_cast <UWord *> (mOutBadRegIndexes.GetHostPointer ()));

		for (NTV2RegisterWritesConstIter iter (inRegWrites.begin ());  iter != inRegWrites.end ();  ++iter)
		{
			if (pBadRegIndexes)
				pBadRegIndexes [ndx] = 0;
			if (pRegInfoArray)
				pRegInfoArray [ndx++] = *iter;
		}
		assert ((ndx * sizeof (NTV2RegInfo)) == mInRegInfos.GetByteCount ());
		assert ((ndx * sizeof (UWord)) == mOutBadRegIndexes.GetByteCount ());
	}
	return result;
}


bool NTV2SetRegisters::GetFailedRegisterWrites (NTV2RegisterWrites & outFailedRegWrites) const
{
	NTV2_ASSERT_STRUCT_VALID;
	outFailedRegWrites.clear ();
	return true;
}


ostream & NTV2SetRegisters::Print (ostream & inOutStream) const
{
	NTV2_ASSERT_STRUCT_VALID;
	inOutStream	<< mHeader << ", numRegs=" << mInNumRegisters << ", inRegInfos=" << mInRegInfos << ", outNumFailures=" << mOutNumFailures
				<< ", outBadRegIndexes=" << mOutBadRegIndexes << ", " << mTrailer;
	const UWord *		pBadRegIndexes		(reinterpret_cast <UWord *> (mOutBadRegIndexes.GetHostPointer ()));
	const UWord			numBadRegIndexes	(UWord (mOutBadRegIndexes.GetByteCount () / sizeof (UWord)));
	const NTV2RegInfo *	pRegInfoArray		(reinterpret_cast <NTV2RegInfo *> (mInRegInfos.GetHostPointer ()));
	const UWord			numRegInfos			(UWord (mInRegInfos.GetByteCount () / sizeof (NTV2RegInfo)));
	if (pBadRegIndexes && numBadRegIndexes && pRegInfoArray && numRegInfos)
	{
		inOutStream << endl;
		for (UWord num (0);  num < numBadRegIndexes;  num++)
		{
			const UWord	badRegIndex	(pBadRegIndexes [num]);
			if (badRegIndex < numRegInfos)
			{
				const NTV2RegInfo &	badRegInfo	(pRegInfoArray [badRegIndex]);
				inOutStream << "Bad " << num << ":  " << badRegInfo << endl;
			}
		}
	}
	return inOutStream;
}


bool NTV2RegInfo::operator < (const NTV2RegInfo & inRHS) const
{
	typedef std::pair <ULWord, ULWord>			ULWordPair;
	typedef std::pair <ULWordPair, ULWordPair>	ULWordPairs;
	const ULWordPairs	rhs	(ULWordPair (inRHS.registerNumber, inRHS.registerValue), ULWordPair (inRHS.registerMask, inRHS.registerShift));
	const ULWordPairs	mine(ULWordPair (registerNumber, registerValue), ULWordPair (registerMask, registerShift));
	return mine < rhs;
}


ostream & operator << (ostream & inOutStream, const NTV2RasterLineOffsets & inObj)
{
	NTV2StringList	pieces;
	NTV2RasterLineOffsetsConstIter	iter (inObj.begin());
	ULWord	current		(0xFFFFFFFF);
	ULWord	previous	(0xFFFFFFFF);
	ULWord	first		(0xFFFFFFFF);
	ULWord	last		(0xFFFFFFFF);

	#if 0
		//	Verify sorted ascending...
		current = 0;
		while (iter != inObj.end())
		{
			NTV2_ASSERT (current < previous);
			previous = current;
		}
		iter = inObj.begin();
	#endif	//	_DEBUG

	while (iter != inObj.end())
	{
		current = *iter;
		if (previous == 0xFFFFFFFF)
			previous = first = last = current;	//	First time -- always start new sequence
		else if (current == (previous + 1))
			last = previous = current;			//	Continue sequence
		else if (current == previous)
			;
		else
		{
			ostringstream	oss;
			if (first == last)
				oss << first;
			else
				oss << first << "-" << last;
			pieces.push_back (oss.str ());

			first = last = previous = current;	//	Start new sequence...
		}	//	else sequence break
		++iter;
	}

	if (first != 0xFFFFFFFF && last != 0xFFFFFFFF)
	{
		ostringstream	oss;
		if (first == last)
			oss << first;
		else
			oss << first << "-" << last;
		pieces.push_back (oss.str ());
	}

	for (NTV2StringListConstIter it (pieces.begin());  ; )
	{
		inOutStream << *it;
		if (++it != pieces.end())
			inOutStream << ",";
		else
			break;
	}
	return inOutStream;
}


ostream & operator << (std::ostream & inOutStream, const NTV2RegInfo & inObj)
{
	const string	regName	(::NTV2RegisterNumberToString (NTV2RegisterNumber (inObj.registerNumber)));
	inOutStream << "[" << regName << "(" << inObj.registerNumber << ") val=0x" << hex << inObj.registerValue << dec << "|" << inObj.registerValue
				<< " mask=0x" << hex << inObj.registerMask << dec << " shift=" << inObj.registerShift << "]";
	return inOutStream;
}


ostream & operator << (ostream & inOutStream, const NTV2RegisterWrites & inObj)
{
	inOutStream << inObj.size () << " regs:" << endl;
	for (NTV2RegisterWritesConstIter iter (inObj.begin ());  iter != inObj.end ();  ++iter)
		inOutStream << *iter << endl;
	return inOutStream;
}


NTV2BankSelGetSetRegs::NTV2BankSelGetSetRegs (const NTV2RegInfo & inBankSelect, const NTV2RegInfo & inOutRegInfo, const bool inDoWrite)
	:	mHeader			(NTV2_TYPE_BANKGETSET, sizeof (NTV2BankSelGetSetRegs)),
		mIsWriting		(inDoWrite),			//	Default to reading
		mInBankInfos	(sizeof (NTV2RegInfo)),	//	Room for one bank select
		mInRegInfos		(sizeof (NTV2RegInfo))	//	Room for one register read or write
{
	NTV2RegInfo *	pRegInfo		(reinterpret_cast <NTV2RegInfo *> (mInBankInfos.GetHostPointer ()));
	if (pRegInfo)
		*pRegInfo = inBankSelect;
	pRegInfo = reinterpret_cast <NTV2RegInfo *> (mInRegInfos.GetHostPointer ());
	if (pRegInfo)
		*pRegInfo = inOutRegInfo;
	NTV2_ASSERT_STRUCT_VALID;
}


NTV2RegInfo NTV2BankSelGetSetRegs::GetRegInfo (const UWord inIndex0) const
{
	NTV2_ASSERT_STRUCT_VALID;
	NTV2RegInfo	result;
	if (!mInRegInfos.IsNULL ())
	{
        const UWord	maxNum	(mInRegInfos.GetByteCount () / (UWord)sizeof (NTV2RegInfo));
		if (inIndex0 < maxNum)
		{
			const NTV2RegInfo *	pRegInfo	(reinterpret_cast <const NTV2RegInfo *> (mInRegInfos.GetHostPointer ()));
			result = pRegInfo [inIndex0];
		}
	}
	return result;
}


ostream & NTV2BankSelGetSetRegs::Print (ostream & inOutStream) const
{
	NTV2_ASSERT_STRUCT_VALID;
	const NTV2RegInfo *	pBankRegInfo	(reinterpret_cast <const NTV2RegInfo *> (mInBankInfos.GetHostPointer ()));
	const NTV2RegInfo *	pRegInfo		(reinterpret_cast <const NTV2RegInfo *> (mInRegInfos.GetHostPointer ()));
	inOutStream << mHeader << " " << (mIsWriting ? "W" : "R") << " bankRegInfo=";
	if (mInBankInfos.IsNULL ())
		inOutStream << "-";
	else
		inOutStream << *pBankRegInfo;
	inOutStream << " regInfo=";
	if (mInRegInfos.IsNULL ())
		inOutStream << "-";
	else
		inOutStream << *pRegInfo;
	return inOutStream;
}
