/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2autocirculate.cpp
	@brief		Implements the CNTV2Card AutoCirculate API functions.
	@copyright	(C) 2004-2022 AJA Video Systems, Inc.
**/

#include "ntv2card.h"
#include "ntv2utils.h"
#include "ntv2rp188.h"
#include "ntv2endian.h"
#include "ajabase/system/lock.h"
#include "ajabase/system/debug.h"
#include "ajaanc/includes/ancillarylist.h"
#include "ajaanc/includes/ancillarydata_timecode_atc.h"
#include "ajabase/common/timecode.h"
#include "ajabase/common/common.h"
#include <iomanip>
#include <assert.h>
#include <algorithm>


using namespace std;

#if defined(MSWindows)
	#undef min
#endif

#if 0
	//	Debug builds can clear Anc buffers during A/C capture
	#define AJA_NTV2_CLEAR_DEVICE_ANC_BUFFER_AFTER_CAPTURE_XFER		//	Requires non-zero kVRegZeroDeviceAncPostCapture
	#define AJA_NTV2_CLEAR_HOST_ANC_BUFFER_TAIL_AFTER_CAPTURE_XFER	//	Requires non-zero kVRegZeroHostAncPostCapture
#endif	//	_DEBUG

//	Logging Macros
#define ACINSTP(_p_)		" " << HEX0N(uint64_t(_p_),8)
#define ACTHIS				ACINSTP(this)

#define ACFAIL(__x__)		AJA_sERROR	(AJA_DebugUnit_AutoCirculate,	ACTHIS << "::" << AJAFUNC << ": " << __x__)
#define ACWARN(__x__)		AJA_sWARNING(AJA_DebugUnit_AutoCirculate,	ACTHIS << "::" << AJAFUNC << ": " << __x__)
#define ACNOTE(__x__)		AJA_sNOTICE (AJA_DebugUnit_AutoCirculate,	ACTHIS << "::" << AJAFUNC << ": " << __x__)
#define ACINFO(__x__)		AJA_sINFO	(AJA_DebugUnit_AutoCirculate,	ACTHIS << "::" << AJAFUNC << ": " << __x__)
#define ACDBG(__x__)		AJA_sDEBUG	(AJA_DebugUnit_AutoCirculate,	ACTHIS << "::" << AJAFUNC << ": " << __x__)

#define RCVFAIL(__x__)		AJA_sERROR	(AJA_DebugUnit_Anc2110Rcv,		ACTHIS << "::" << AJAFUNC << ": " << __x__)
#define RCVWARN(__x__)		AJA_sWARNING(AJA_DebugUnit_Anc2110Rcv,		ACTHIS << "::" << AJAFUNC << ": " << __x__)
#define RCVNOTE(__x__)		AJA_sNOTICE (AJA_DebugUnit_Anc2110Rcv,		ACTHIS << "::" << AJAFUNC << ": " << __x__)
#define RCVINFO(__x__)		AJA_sINFO	(AJA_DebugUnit_Anc2110Rcv,		ACTHIS << "::" << AJAFUNC << ": " << __x__)
#define RCVDBG(__x__)		AJA_sDEBUG	(AJA_DebugUnit_Anc2110Rcv,		ACTHIS << "::" << AJAFUNC << ": " << __x__)

#define XMTFAIL(__x__)		AJA_sERROR	(AJA_DebugUnit_Anc2110Xmit,		ACTHIS << "::" << AJAFUNC << ": " << __x__)
#define XMTWARN(__x__)		AJA_sWARNING(AJA_DebugUnit_Anc2110Xmit,		ACTHIS << "::" << AJAFUNC << ": " << __x__)
#define XMTNOTE(__x__)		AJA_sNOTICE (AJA_DebugUnit_Anc2110Xmit,		ACTHIS << "::" << AJAFUNC << ": " << __x__)
#define XMTINFO(__x__)		AJA_sINFO	(AJA_DebugUnit_Anc2110Xmit,		ACTHIS << "::" << AJAFUNC << ": " << __x__)
#define XMTDBG(__x__)		AJA_sDEBUG	(AJA_DebugUnit_Anc2110Xmit,		ACTHIS << "::" << AJAFUNC << ": " << __x__)


static const char	gFBAllocLockName[]	=	"com.aja.ntv2.mutex.FBAlloc";
static AJALock		gFBAllocLock(gFBAllocLockName); //	New in SDK 15:	Global mutex to avoid device frame buffer allocation race condition


//GetFrameStamp(NTV2Crosspoint channelSpec, ULONG frameNum, FRAME_STAMP_STRUCT* pFrameStamp)
//When a channelSpec is autocirculating, the ISR or DPC will continously fill in a
// FRAME_STAMP_STRUCT for the frame it is working on.
// The framestamp structure is intended to give enough information to determine if frames
// have been dropped either on input or output. It also allows for synchronization of 
// audio and video by stamping the audioinputaddress at the start and end of a video frame.
bool   CNTV2Card::GetFrameStamp (NTV2Crosspoint channelSpec, ULWord frameNum, FRAME_STAMP_STRUCT* pFrameStamp)
{
	// Insure that the CNTV2Card has been 'open'ed
	if (! _boardOpened )	return false;

	// Fill in our OS independent data structure 
	AUTOCIRCULATE_DATA autoCircData (eGetFrameStamp, channelSpec);
	autoCircData.lVal1		 = LWord(frameNum);
	autoCircData.pvVal1		 = PVOID(pFrameStamp);

	// The following is ignored by Windows; it looks at the 
	// channel spec and frame num in the autoCircData instead.
	pFrameStamp->channelSpec = channelSpec;
	pFrameStamp->frame = frameNum;

	// Call the OS specific method
	return AutoCirculate (autoCircData);
}

// GetAutoCirculate(NTV2Crosspoint channelSpec,AUTOCIRCULATE_STATUS_STRUCT* autoCirculateStatus )
// Returns true if communication with the driver was successful.
// Passes back: whether associated channelSpec is currently autocirculating;
//				Frame Range (Start and End); and Current Active Frame.
//
//				Note that Current Active Frame is reliable,
//				whereas reading, for example, the Ch1OutputFrame register is not reliable,
//				because the latest-written value may or may-not have been clocked-in to the hardware.
//				Note also that this value is valid only if bIsCirculating is true.
bool   CNTV2Card::GetAutoCirculate(NTV2Crosspoint channelSpec, AUTOCIRCULATE_STATUS_STRUCT* autoCirculateStatus )
{
	// Insure that the CNTV2Card has been 'open'ed
	if (!_boardOpened)	return false;

	// The following is ignored by Windows.
	autoCirculateStatus -> channelSpec = channelSpec;
	
	// Fill in our OS independent data structure 
	AUTOCIRCULATE_DATA autoCircData (eGetAutoCirc, channelSpec);
	autoCircData.pvVal1		 = PVOID(autoCirculateStatus);

	// Call the OS specific method
	return AutoCirculate (autoCircData);
}


bool CNTV2Card::FindUnallocatedFrames (const UWord inFrameCount, LWord & outStartFrame, LWord & outEndFrame, const NTV2Channel inFrameStore)
{
	outStartFrame = outEndFrame = -1;
	if (!IsOpen())
		{ACFAIL(GetDescription() << ": Not open");  return false;}
	if (!inFrameCount)
		{ACFAIL(GetDescription() << ": Must request at least one frame");  return false;}

	//	Before inventorying SDRAM, disable the framestore(s) of interest (if enabled),
	//	to maximize contiguous available free memory (since those frames will be re-utilized anyway)...
	NTV2VideoFormat vFmt(NTV2_FORMAT_UNKNOWN);
	NTV2ChannelSet enabledFrameStores;
	bool isQuad(false), isQuadQuad(false), wasEnabled(false);
	if (inFrameStore != NTV2_CHANNEL_INVALID)	//	Caller wants to utilize a specific FrameStore?
	{
		NTV2Channel ch(inFrameStore);
		while (IsChannelEnabled(ch, wasEnabled)  &&  wasEnabled)	//	Is it currently enabled?
		{
			enabledFrameStores.insert(ch);
			if (enabledFrameStores.size() == 1)
			{
				GetVideoFormat(vFmt, inFrameStore);				//	Get its video format
				isQuad = NTV2_IS_QUAD_FRAME_FORMAT(vFmt);		//	Is quad format?
				isQuadQuad = NTV2_IS_QUAD_QUAD_FORMAT(vFmt);	//	Is quad-quad format?
				if (!isQuad && !isQuadQuad)
					break;
			}
			ch = NTV2Channel(ch+1);
			if (UWord(ch) > (UWord(inFrameStore)+1))
				break;
		}
		DisableChannels(enabledFrameStores);	//	Temporarily disable it/them, to preclude it/them from fragmenting free memory
	}

	//	Inventory device SDRAM utilization...
	SDRAMAuditor	auditor(*this);	//	Using this constructor automatically excludes audio buffers from consideration, by default
	ULWordSequence	freeRgns8MB, freeRgns;
	auditor.GetFreeRegions (freeRgns8MB);
	if (!enabledFrameStores.empty())
		EnableChannels(enabledFrameStores);	//	Restore FrameStore enabled state

	if (freeRgns8MB.empty())
		{ACFAIL(GetDescription() << ": No free regions");  return false;}
	if (!auditor.TranslateRegions (freeRgns, freeRgns8MB, isQuad, isQuadQuad))
		{ACFAIL(GetDescription() << ": TranslateRegions failed");  return false;}
	if (freeRgns.empty())
		{ACFAIL(GetDescription() << ": No free regions after translation");  return false;}

	//	Look for first free region whose length >= inFrameCount...
	for (size_t ndx(0);  ndx < freeRgns.size();  ndx++)
	{	const ULWord val(freeRgns.at(ndx));
		UWord startFrame(val >> 16), lengthFrames(UWord(val & 0x0000FFFF));
		if (inFrameCount > lengthFrames)	//	Will it fit?
			continue;	//	No, skip it
		//	Yes -- use it
		outStartFrame = LWord(startFrame);
		outEndFrame   = LWord(startFrame + inFrameCount - 1);
		break;	//	Done, found!
	}
	const string qstr (isQuad ? " quad" : (isQuadQuad ? " quad-quad" : ""));
	if (outStartFrame < 0  ||  outEndFrame < 0)
	{
	#if defined(_DEBUG)
		auditor.DumpBlocks(cerr);
	#endif	//	_DEBUG
		ostringstream dump;		dump << DEC(freeRgns.size()) << " free region(s):" << endl;
		if (!freeRgns.empty())
			dump << "    Tgt Frms   8MB Frms" << endl;
		for (size_t ndx(0);  ndx < freeRgns.size();  ndx++)
		{	ULWord rgn(freeRgns.at(ndx)), rgn8(freeRgns8MB.at(ndx));
			UWord startBlk(rgn >> 16), numBlks(UWord(rgn & 0x0000FFFF));
			UWord startBlk8(rgn8 >> 16), numBlks8(UWord(rgn8 & 0x0000FFFF));
			if (numBlks > 1)
				dump << "Frms " << DEC0N(startBlk,3) << "-" << DEC0N(startBlk+numBlks-1,3) << "   ";
			else
				dump << "Frm  " << DEC0N(startBlk,3) << "       ";
			if (numBlks8 > 1)
				dump << DEC0N(startBlk8,3) << "-" << DEC0N(startBlk8+numBlks8-1,3) << endl;
			else
				dump << DEC0N(startBlk8,3) << endl;
		}
		ACFAIL(GetDescription() << ": Cannot find " << DEC(inFrameCount) << " contiguous" << qstr << " frames in these " << dump.str());
		return false;
	}
	ACINFO(GetDescription() << ": Found requested " << DEC(inFrameCount) << " contiguous" << qstr << " frames (" << DEC(outStartFrame) << "-" << DEC(outEndFrame) << ")");
	return true;

}	//	FindUnallocatedFrames


//	Handy function to fetch the NTV2Crosspoint for a given NTV2Channel that works with both pre & post 12.3 drivers.
//	NOTE:  This relies on the channel's NTV2Mode being correct and aligned with the driver's NTV2Crosspoint!
static bool GetCurrentACChannelCrosspoint (CNTV2Card & inDevice, const NTV2Channel inChannel, NTV2Crosspoint & outCrosspoint)
{
	NTV2Mode	mode	(NTV2_MODE_DISPLAY);
	outCrosspoint = NTV2CROSSPOINT_INVALID;
	if (!inDevice.IsOpen ())
		return false;
	if (!NTV2_IS_VALID_CHANNEL (inChannel))
		return false;

	if (!inDevice.GetMode (inChannel, mode))
		return false;
	outCrosspoint = (mode == NTV2_MODE_DISPLAY) ? ::NTV2ChannelToOutputCrosspoint (inChannel) : ::NTV2ChannelToInputCrosspoint (inChannel);
	return true;
}


bool CNTV2Card::AutoCirculateInitForInput ( const NTV2Channel		inChannel,
											const UWord				inFrameCount,
											const NTV2AudioSystem	inAudioSystem,
											const ULWord			inOptionFlags,
											const UByte				inNumChannels,
											const UWord				inStartFrameNumber,
											const UWord				inEndFrameNumber)
{
	if (!NTV2_IS_VALID_CHANNEL(inChannel))
		{ACFAIL(GetDescription() << ": Ch" << DEC(inChannel+1) << " is illegal channel value");  return false;}	//	Must be valid channel
	if (!inNumChannels  ||  inNumChannels > 8)
		{ACFAIL(GetDescription() << ": Input Ch" << DEC(inChannel+1) << ": illegal 'inNumChannels' value '" << DEC(inNumChannels) << "' -- must be 1-8");  return false;}	//	At least one channel
	if (!gFBAllocLock.IsValid())
		{ACFAIL(GetDescription() << ": Input Ch" << DEC(inChannel+1) << ": FBAllocLock mutex not ready");  return false;}	//	Mutex not ready

	AJAAutoLock autoLock (&gFBAllocLock);	//	Avoid AutoCirculate buffer collisions
	LWord	startFrameNumber(LWord(inStartFrameNumber+0));
	LWord	endFrameNumber	(LWord(inEndFrameNumber+0));
	if (!endFrameNumber	 &&	 !startFrameNumber)
	{
		if (!inFrameCount)
			{ACFAIL(GetDescription() << ": Input Ch" << DEC(inChannel+1) << ": Zero frames requested");  return false;}
		if (!FindUnallocatedFrames (inFrameCount, startFrameNumber, endFrameNumber, inChannel))
			return false;
	}
	else if (inFrameCount)
		ACWARN (GetDescription() << ": Input Ch" << DEC(inChannel+1) << ": FrameCount " << DEC(inFrameCount) << " ignored -- using start/end " << DEC(inStartFrameNumber)
				<< "/" << DEC(inEndFrameNumber) << " frame numbers");
	if (endFrameNumber < startFrameNumber)	//	endFrame must be > startFrame
		{ACFAIL(GetDescription() << ": Input Ch" << DEC(inChannel+1) << ": EndFrame(" << DEC(endFrameNumber) << ") precedes StartFrame(" << DEC(startFrameNumber) << ")");  return false;}
	if ((endFrameNumber - startFrameNumber + 1) < 2)	//	must be at least 2 frames
		{ACFAIL(GetDescription() << ": Input Ch" << DEC(inChannel+1) << ": Frames " << DEC(startFrameNumber) << "-" << DEC(endFrameNumber) << " < 2 frames"); return false;}
	if (startFrameNumber >= MAX_FRAMEBUFFERS)
		{ACFAIL(GetDescription() << ": Output Ch" << DEC(inChannel+1) << ": Start frame " << DEC(startFrameNumber) << " exceeds max " << DEC(MAX_FRAMEBUFFERS-1)); return false;}
	if (endFrameNumber >= MAX_FRAMEBUFFERS)
		{ACFAIL(GetDescription() << ": Output Ch" << DEC(inChannel+1) << ": End frame " << DEC(endFrameNumber) << " exceeds max " << DEC(MAX_FRAMEBUFFERS-1)); return false;}
	if (inOptionFlags & (AUTOCIRCULATE_WITH_MULTILINK_AUDIO1 | AUTOCIRCULATE_WITH_MULTILINK_AUDIO2 | AUTOCIRCULATE_WITH_MULTILINK_AUDIO3)  &&  !IsSupported(kDeviceCanDoMultiLinkAudio))
		ACWARN(GetDescription() << ": Input Ch" << DEC(inChannel+1) << ": MultiLink Audio requested, but device doesn't support it");
	const UWord numAudSystems(UWord(GetNumSupported(kDeviceGetNumAudioSystems)));	//	AutoCirc cannot use AudioMixer or HostAudio
	if (inAudioSystem != NTV2_AUDIOSYSTEM_INVALID)
	{
		if (numAudSystems  &&  UWord(inAudioSystem) >= numAudSystems)
			{ACFAIL(GetDescription() << ": Invalid audio system specified: AudSys" << DEC(inAudioSystem+1) << " -- exceeds max legal AudSys" << DEC(numAudSystems)); return false;}
	}

	//	Fill in our OS independent data structure...
	AUTOCIRCULATE_DATA	autoCircData (eInitAutoCirc, ::NTV2ChannelToInputChannelSpec(inChannel));
	autoCircData.lVal1 = startFrameNumber;
	autoCircData.lVal2 = endFrameNumber;
	autoCircData.lVal3 = inAudioSystem;
	if (inOptionFlags & AUTOCIRCULATE_WITH_MULTILINK_AUDIO1)
		autoCircData.lVal3 |= NTV2_AUDIOSYSTEM_Plus1;
	if (inOptionFlags & AUTOCIRCULATE_WITH_MULTILINK_AUDIO2)
		autoCircData.lVal3 |= NTV2_AUDIOSYSTEM_Plus2;
	if (inOptionFlags & AUTOCIRCULATE_WITH_MULTILINK_AUDIO3)
		autoCircData.lVal3 |= NTV2_AUDIOSYSTEM_Plus3;
	autoCircData.lVal4 = inNumChannels;
	if (inOptionFlags & AUTOCIRCULATE_WITH_FIELDS)
		autoCircData.lVal6 |= AUTOCIRCULATE_WITH_FIELDS;
	if (inOptionFlags & AUTOCIRCULATE_WITH_HDMIAUX)
		autoCircData.lVal6 |= AUTOCIRCULATE_WITH_HDMIAUX;
	if (inOptionFlags & AUTOCIRCULATE_WITH_AUDIO_CONTROL)
		autoCircData.bVal1 = false;
	else
		autoCircData.bVal1 = NTV2_IS_VALID_AUDIO_SYSTEM(inAudioSystem) ? true : false;
	autoCircData.bVal2 = inOptionFlags & AUTOCIRCULATE_WITH_RP188			? true : false;
	autoCircData.bVal3 = inOptionFlags & AUTOCIRCULATE_WITH_FBFCHANGE		? true : false;
	autoCircData.bVal4 = inOptionFlags & AUTOCIRCULATE_WITH_FBOCHANGE		? true : false;
	autoCircData.bVal5 = inOptionFlags & AUTOCIRCULATE_WITH_COLORCORRECT	? true : false;
	autoCircData.bVal6 = inOptionFlags & AUTOCIRCULATE_WITH_VIDPROC			? true : false;
	autoCircData.bVal7 = inOptionFlags & AUTOCIRCULATE_WITH_ANC				? true : false;
	autoCircData.bVal8 = inOptionFlags & AUTOCIRCULATE_WITH_LTC				? true : false;

	const bool result (AutoCirculate(autoCircData));	//	Call the OS-specific method
	if (result)
	{	//	Success!
		#if 1
			//	Warn about interference from other channels...
			ULWordSequence badRgns;
			SDRAMAuditor auditor(*this);
			auditor.GetBadRegions(badRgns);
			for (size_t ndx(0);  ndx < badRgns.size();  ndx++)
			{	const ULWord rgnInfo(badRgns.at(ndx));
				const UWord startBlk(rgnInfo >> 16), numBlks(UWord(rgnInfo & 0x0000FFFF));
				NTV2StringSet tags;
				auditor.GetTagsForFrameIndex (startBlk, tags);
				const string infoStr (aja::join(tags, ", "));
				ostringstream acLabel;  acLabel << "AC" << DEC(inChannel+1);	//	Search for label e.g. "AC2"
				if (infoStr.find(acLabel.str()) != string::npos)
				{	ostringstream warning;
					if (numBlks > 1)
						warning << "Frms " << DEC0N(startBlk,3) << "-" << DEC0N(startBlk+numBlks-1,3);
					else
						warning << "Frm  " << DEC0N(startBlk,3);
					ACWARN(GetDescription() << ": Input Ch" << DEC(inChannel+1) << ": memory overlap/interference: " << warning.str() << ": " << infoStr);
				}
			}	//	for each "bad" region
		#endif
		#if 1
		{	AUTOCIRCULATE_STATUS stat;
			if (AutoCirculateGetStatus (inChannel, stat)  &&  !stat.IsStopped()  &&  stat.WithAudio())
			{	//	Not stopped and AutoCirculating audio -- check if audio buffer capacity will be exceeded...
				ULWord audChlsPerSample(0);
				NTV2FrameRate fr(NTV2_FRAMERATE_INVALID);
				NTV2AudioRate ar(NTV2_AUDIO_RATE_INVALID);
				GetNumberAudioChannels (audChlsPerSample, stat.GetAudioSystem());
				if (GetFrameRate (fr, inChannel)  &&  NTV2_IS_SUPPORTED_NTV2FrameRate(fr))
					if (GetAudioRate (ar, stat.GetAudioSystem())  &&  NTV2_IS_VALID_AUDIO_RATE(ar))
					{
						const double framesPerSecond (double(::GetScaleFromFrameRate(fr)) / 100.00);
						const double samplesPerSecond (double(::GetAudioSamplesPerSecond(ar)));
						const double bytesPerChannel (4.0);
						const double channelsPerSample (double(audChlsPerSample+0));
						const double bytesPerFrame (samplesPerSecond * bytesPerChannel * channelsPerSample / framesPerSecond);
						const ULWord maxVideoFrames (4UL * 1024UL * 1024UL / ULWord(bytesPerFrame));
						if (stat.GetFrameCount() > maxVideoFrames)
							ACWARN(GetDescription() << ": Input Ch" << DEC(inChannel+1) << ":  " << DEC(stat.GetFrameCount()) << " frames ("
									<< DEC(stat.GetStartFrame()) << "-" << DEC(stat.GetEndFrame()) << ") exceeds "
									<< DEC(maxVideoFrames) << "-frame max buffer capacity of AudSys" << DEC(stat.GetAudioSystem()+1));
					}
			}
		}
		#endif
		ACINFO(GetDescription() << ": Input Ch" << DEC(inChannel+1) << " initialized using frames " << DEC(startFrameNumber) << "-" << DEC(endFrameNumber));
	}
	else
		ACFAIL(GetDescription() << ": Input Ch" << DEC(inChannel+1) << " initialization failed");
	return result;

}	//	AutoCirculateInitForInput

bool CNTV2Card::AutoCirculateInitForInput ( const NTV2Channel			inChannel,
											const NTV2ACFrameRange &	inFrameRange,
											const NTV2AudioSystem		inAudioSystem,
											const ULWord				inOptionFlags,
											const UByte					inNumChannels)
{
	return inFrameRange ? AutoCirculateInitForInput (inChannel, inFrameRange.count(), inAudioSystem, inOptionFlags,
													inNumChannels, inFrameRange.firstFrame(), inFrameRange.lastFrame())
						: false;
}


bool CNTV2Card::AutoCirculateInitForOutput (const NTV2Channel		inChannel,
											const UWord				inFrameCount,
											const NTV2AudioSystem	inAudioSystem,
											const ULWord			inOptionFlags,
											const UByte				inNumChannels,
											const UWord				inStartFrameNumber,
											const UWord				inEndFrameNumber)
{
	if (!NTV2_IS_VALID_CHANNEL(inChannel))
		{ACFAIL(GetDescription() << ": Ch" << DEC(inChannel+1) << " is illegal channel value");  return false;}	//	Must be valid channel
	if (!inNumChannels  ||  inNumChannels > 8)
		{ACFAIL(GetDescription() << ": Output Ch" << DEC(inChannel+1) << ": illegal 'inNumChannels' value '" << DEC(inNumChannels) << "' -- must be 1-8");  return false;}	//	At least one channel
	if (!gFBAllocLock.IsValid())
		{ACFAIL(GetDescription() << ": Output Ch" << DEC(inChannel+1) << ": FBAllocLock mutex not ready");  return false;}	//	Mutex not ready

	AJAAutoLock autoLock (&gFBAllocLock);	//	Avoid AutoCirculate buffer collisions
	LWord	startFrameNumber(LWord(inStartFrameNumber+0));
	LWord	endFrameNumber	(LWord(inEndFrameNumber+0));
	if (!endFrameNumber	 &&	 !startFrameNumber)
	{
		if (!inFrameCount)
			{ACFAIL(GetDescription() << ": Output Ch" << DEC(inChannel+1) << ": Zero frames requested");  return false;}
		if (!FindUnallocatedFrames (inFrameCount, startFrameNumber, endFrameNumber, inChannel))
			return false;
	}
	else if (inFrameCount)
		ACWARN (GetDescription() << ": Output Ch" << DEC(inChannel+1) << ": FrameCount " << DEC(inFrameCount) << " ignored -- using start/end "
				<< DEC(inStartFrameNumber) << "/" << DEC(inEndFrameNumber) << " frame numbers");
	if (endFrameNumber < startFrameNumber)	//	endFrame must be > startFrame
		{ACFAIL(GetDescription() << ": Output Ch" << DEC(inChannel+1) << ": EndFrame(" << DEC(endFrameNumber) << ") precedes StartFrame("
				<< DEC(startFrameNumber) << ")");  return false;}
	if ((endFrameNumber - startFrameNumber + 1) < 2)	//	must be at least 2 frames
		{ACFAIL(GetDescription() << ": Output Ch" << DEC(inChannel+1) << ": Frames " << DEC(startFrameNumber) << "-" << DEC(endFrameNumber) << " < 2 frames"); return false;}
	if (startFrameNumber >= MAX_FRAMEBUFFERS)
		{ACFAIL(GetDescription() << ": Output Ch" << DEC(inChannel+1) << ": Start frame " << DEC(startFrameNumber) << " exceeds max " << DEC(MAX_FRAMEBUFFERS-1)); return false;}
	if (endFrameNumber >= MAX_FRAMEBUFFERS)
		{ACFAIL(GetDescription() << ": Output Ch" << DEC(inChannel+1) << ": End frame " << DEC(endFrameNumber) << " exceeds max " << DEC(MAX_FRAMEBUFFERS-1)); return false;}
	if (inOptionFlags & (AUTOCIRCULATE_WITH_MULTILINK_AUDIO1 | AUTOCIRCULATE_WITH_MULTILINK_AUDIO2 | AUTOCIRCULATE_WITH_MULTILINK_AUDIO3)  &&  !IsSupported(kDeviceCanDoMultiLinkAudio))
		ACWARN(GetDescription() << ": Output Ch" << DEC(inChannel+1) << ": MultiLink Audio requested, but device doesn't support it");
	const UWord numAudSystems(UWord(GetNumSupported(kDeviceGetNumAudioSystems)));	//	AutoCirc cannot use AudioMixer or HostAudio
	if (inAudioSystem != NTV2_AUDIOSYSTEM_INVALID)
	{
		if (numAudSystems  &&  UWord(inAudioSystem) >= numAudSystems)
			{ACFAIL(GetDescription() << ": Invalid audio system specified: AudSys" << DEC(inAudioSystem+1) << " -- exceeds max legal AudSys" << DEC(numAudSystems)); return false;}
	}

	//	Warn about "with anc" and VANC mode...
	if (inOptionFlags & AUTOCIRCULATE_WITH_ANC)
	{
		NTV2VANCMode vancMode(NTV2_VANCMODE_INVALID);
		if (GetVANCMode(vancMode, inChannel)  &&  NTV2_IS_VANCMODE_ON(vancMode))
			ACWARN(GetDescription() << ": Output Ch" << DEC(inChannel+1) << "AUTOCIRCULATE_WITH_ANC set, but also has "
					<< ::NTV2VANCModeToString(vancMode) << " set -- this may cause anc insertion problems");
	}

	//	Fill in our OS independent data structure...
	AUTOCIRCULATE_DATA	autoCircData (eInitAutoCirc, ::NTV2ChannelToOutputChannelSpec(inChannel));
	autoCircData.lVal1 = startFrameNumber;
	autoCircData.lVal2 = endFrameNumber;
	autoCircData.lVal3 = inAudioSystem;
	if (inOptionFlags & AUTOCIRCULATE_WITH_MULTILINK_AUDIO1)
		autoCircData.lVal3 |= NTV2_AUDIOSYSTEM_Plus1;
	if (inOptionFlags & AUTOCIRCULATE_WITH_MULTILINK_AUDIO2)
		autoCircData.lVal3 |= NTV2_AUDIOSYSTEM_Plus2;
	if (inOptionFlags & AUTOCIRCULATE_WITH_MULTILINK_AUDIO3)
		autoCircData.lVal3 |= NTV2_AUDIOSYSTEM_Plus3;
	autoCircData.lVal4 = inNumChannels;
	if (inOptionFlags & AUTOCIRCULATE_WITH_FIELDS)
		autoCircData.lVal6 |= AUTOCIRCULATE_WITH_FIELDS;
	if (inOptionFlags & AUTOCIRCULATE_WITH_HDMIAUX)
		autoCircData.lVal6 |= AUTOCIRCULATE_WITH_HDMIAUX;
	if (inOptionFlags & AUTOCIRCULATE_WITH_AUDIO_CONTROL)
		autoCircData.bVal1 = false;
	else
		autoCircData.bVal1 = NTV2_IS_VALID_AUDIO_SYSTEM(inAudioSystem)		? true : false;
	autoCircData.bVal2 = (inOptionFlags & AUTOCIRCULATE_WITH_RP188)			? true : false;
	autoCircData.bVal3 = (inOptionFlags & AUTOCIRCULATE_WITH_FBFCHANGE)		? true : false;
	autoCircData.bVal4 = (inOptionFlags & AUTOCIRCULATE_WITH_FBOCHANGE)		? true : false;
	autoCircData.bVal5 = (inOptionFlags & AUTOCIRCULATE_WITH_COLORCORRECT)	? true : false;
	autoCircData.bVal6 = (inOptionFlags & AUTOCIRCULATE_WITH_VIDPROC)		? true : false;
	autoCircData.bVal7 = (inOptionFlags & AUTOCIRCULATE_WITH_ANC)			? true : false;
	autoCircData.bVal8 = (inOptionFlags & AUTOCIRCULATE_WITH_LTC)			? true : false;
	if (IsSupported(kDeviceCanDo2110))					//	If S2110 IP device...
		if (inOptionFlags & AUTOCIRCULATE_WITH_RP188)		//	and caller wants RP188
			if (!(inOptionFlags & AUTOCIRCULATE_WITH_ANC))	//	but caller failed to enable Anc playout
			{
				autoCircData.bVal7 = true;					//	Enable Anc insertion anyway
				ACWARN(GetDescription() << ": Output Ch" << DEC(inChannel+1)
						<< ": AUTOCIRCULATE_WITH_RP188 requested without AUTOCIRCULATE_WITH_ANC -- enabled AUTOCIRCULATE_WITH_ANC anyway");
			}

	const bool result (AutoCirculate(autoCircData));	//	Call the OS-specific method
	if (result)
	{	//	Success!
		#if 1
			//	Warn about interference from other channels...
			ULWordSequence badRgns;
			SDRAMAuditor auditor(*this);
			auditor.GetBadRegions(badRgns);
			for (size_t ndx(0);  ndx < badRgns.size();  ndx++)
			{	const ULWord rgnInfo(badRgns.at(ndx));
				const UWord startBlk(rgnInfo >> 16), numBlks(UWord(rgnInfo & 0x0000FFFF));
				NTV2StringSet tags;
				auditor.GetTagsForFrameIndex (startBlk, tags);
				const string infoStr (aja::join(tags, ", "));
				ostringstream acLabel;  acLabel << "AC" << DEC(inChannel+1);	//	Search for label e.g. "AC2"
				if (infoStr.find(acLabel.str()) != string::npos)
				{	ostringstream warning;
					if (numBlks > 1)
						warning << "Frms " << DEC0N(startBlk,3) << "-" << DEC0N(startBlk+numBlks-1,3);
					else
						warning << "Frm  " << DEC0N(startBlk,3);
					ACWARN(GetDescription() << ": Output Ch" << DEC(inChannel+1) << ": memory overlap/interference: " << warning.str() << ": " << infoStr);
				}
			}	//	for each "bad" region
		#endif
		#if 1
		{	AUTOCIRCULATE_STATUS stat;
			if (AutoCirculateGetStatus (inChannel, stat)  &&  !stat.IsStopped()  &&  stat.WithAudio())
			{	//	Not stopped and AutoCirculating audio -- check if audio buffer capacity will be exceeded...
				ULWord audChlsPerSample(0);
				NTV2FrameRate fr(NTV2_FRAMERATE_INVALID);
				NTV2AudioRate ar(NTV2_AUDIO_RATE_INVALID);
				GetNumberAudioChannels (audChlsPerSample, stat.GetAudioSystem());
				if (GetFrameRate (fr, inChannel)  &&  NTV2_IS_SUPPORTED_NTV2FrameRate(fr))
					if (GetAudioRate (ar, stat.GetAudioSystem())  &&  NTV2_IS_VALID_AUDIO_RATE(ar))
					{
						const double framesPerSecond (double(::GetScaleFromFrameRate(fr)) / 100.00);
						const double samplesPerSecond (double(::GetAudioSamplesPerSecond(ar)));
						const double bytesPerChannel (4.0);
						const double channelsPerSample (double(audChlsPerSample+0));
						const double bytesPerFrame (samplesPerSecond * bytesPerChannel * channelsPerSample / framesPerSecond);
						const ULWord maxVideoFrames (4UL * 1024UL * 1024UL / ULWord(bytesPerFrame));
						if (stat.GetFrameCount() > maxVideoFrames)
							ACWARN(GetDescription() << ": Output Ch" << DEC(inChannel+1) << ":  " << DEC(stat.GetFrameCount()) << " frames ("
									<< DEC(stat.GetStartFrame()) << "-" << DEC(stat.GetEndFrame()) << ") exceeds "
									<< DEC(maxVideoFrames) << "-frame max buffer capacity of AudSys" << DEC(stat.GetAudioSystem()+1));
					}
			}
		}
		#endif
		ACINFO(GetDescription() << ": Output Ch" << DEC(inChannel+1) << " initialized using frames " << DEC(startFrameNumber) << "-" << DEC(endFrameNumber));
	}
	else
		ACFAIL(GetDescription() << ": Output Ch" << DEC(inChannel+1) << " initialization failed");
	return result;

}	//	AutoCirculateInitForOutput

bool CNTV2Card::AutoCirculateInitForOutput ( const NTV2Channel			inChannel,
											const NTV2ACFrameRange &	inFrameRange,
											const NTV2AudioSystem		inAudioSystem,
											const ULWord				inOptionFlags,
											const UByte					inNumChannels)
{
	return inFrameRange ? AutoCirculateInitForOutput (inChannel, inFrameRange.count(), inAudioSystem, inOptionFlags,
														inNumChannels, inFrameRange.firstFrame(), inFrameRange.lastFrame())
						: false;
}


bool CNTV2Card::AutoCirculateStart (const NTV2Channel inChannel, const ULWord64 inStartTime)
{
	AUTOCIRCULATE_DATA autoCircData (inStartTime ? eStartAutoCircAtTime : eStartAutoCirc);
	autoCircData.lVal1 = LWord(inStartTime >> 32);
	autoCircData.lVal2 = LWord(inStartTime & 0xFFFFFFFF);
	if (!GetCurrentACChannelCrosspoint (*this, inChannel, autoCircData.channelSpec))
		return false;
	const bool result (AutoCirculate(autoCircData));
	if (result)
		ACINFO(GetDescription() << ": Started Ch" << DEC(inChannel+1));
	else
		ACFAIL(GetDescription() << ": Failed to start Ch" << DEC(inChannel+1));
	return result;
}


bool CNTV2Card::AutoCirculateStop (const NTV2Channel inChannel, const bool inAbort)
{
	if (!NTV2_IS_VALID_CHANNEL (inChannel))
		return false;

	const AUTO_CIRC_COMMAND acCommand	(inAbort ? eAbortAutoCirc : eStopAutoCirc);
	AUTOCIRCULATE_DATA		stopInput	(acCommand, ::NTV2ChannelToInputCrosspoint (inChannel));
	AUTOCIRCULATE_DATA		stopOutput	(acCommand, ::NTV2ChannelToOutputCrosspoint (inChannel));
	NTV2Mode				mode		(NTV2_MODE_INVALID);
	AUTOCIRCULATE_STATUS	acStatus;

	//	Stop input or output A/C using the old driver call...
	const bool	stopInputFailed		(!AutoCirculate (stopInput));
	const bool	stopOutputFailed	(!AutoCirculate (stopOutput));
	if (stopInputFailed && stopOutputFailed)
	{
		ACFAIL(GetDescription() << ": Failed to stop Ch" << DEC(inChannel+1));
		return false;	//	Both failed
	}
	if (inAbort)
	{
		ACINFO(GetDescription() << ": Aborted Ch" << DEC(inChannel+1));
		return true;	//	In abort case, no more to do!
	}

	//	Wait until driver changes AC state to DISABLED...
	bool result (GetMode(inChannel, mode));
	if (NTV2_IS_INPUT_MODE(mode))
		WaitForInputFieldID(NTV2_FIELD0, inChannel);
	if (NTV2_IS_OUTPUT_MODE(mode))
		WaitForOutputFieldID(NTV2_FIELD0, inChannel);
	if (AutoCirculateGetStatus(inChannel, acStatus)	 &&	 acStatus.acState != NTV2_AUTOCIRCULATE_DISABLED)
	{
		ACWARN(GetDescription() << ": Failed to stop Ch" << DEC(inChannel+1) << " -- retrying with ABORT");
		return AutoCirculateStop(inChannel, true);	//	something's wrong -- abort (WARNING: RECURSIVE CALL!)
	}
	ACINFO(GetDescription() << ": Stopped Ch" << DEC(inChannel+1));
	return result;

}	//	AutoCirculateStop


bool CNTV2Card::AutoCirculateStop (const NTV2ChannelSet & inChannels, const bool inAbort)
{	UWord failures(0);
	for (NTV2ChannelSetConstIter it(inChannels.begin());  it != inChannels.end();  ++it)
		if (!AutoCirculateStop(*it, inAbort))
			failures++;
	return !failures;
}


bool CNTV2Card::AutoCirculatePause (const NTV2Channel inChannel,  const UWord inAtFrameNum)
{	(void) inAtFrameNum;
	//	Use the old A/C driver call...
	AUTOCIRCULATE_DATA	autoCircData (ePauseAutoCirc);
	autoCircData.bVal1	= false;
	if (!GetCurrentACChannelCrosspoint (*this, inChannel, autoCircData.channelSpec))
		return false;

	//	FUTURE:  pass inAtFrameNum
	//	if (inAtFrameNum != 0xFFFF)
	//		autoCircData.lVal6	= LWord(inAtFrameNum);
	const bool result(AutoCirculate(autoCircData));
	if (result)
		ACINFO(GetDescription() << ": Paused Ch" << DEC(inChannel+1));
	else
		ACFAIL(GetDescription() << ": Failed to pause Ch" << DEC(inChannel+1));
	return result;

}	//	AutoCirculatePause


bool CNTV2Card::AutoCirculateResume (const NTV2Channel inChannel, const bool inClearDropCount)
{
	//	Use the old A/C driver call...
	AUTOCIRCULATE_DATA	autoCircData (ePauseAutoCirc);
	autoCircData.bVal1 = true;
	autoCircData.bVal2 = inClearDropCount;
	if (!GetCurrentACChannelCrosspoint (*this, inChannel, autoCircData.channelSpec))
		return false;

	const bool result(AutoCirculate(autoCircData));
	if (result)
		ACINFO(GetDescription() << ": Resumed Ch" << DEC(inChannel+1));
	else
		ACFAIL(GetDescription() << ": Failed to resume Ch" << DEC(inChannel+1));
	return result;

}	//	AutoCirculateResume


bool CNTV2Card::AutoCirculateFlush (const NTV2Channel inChannel, const bool inClearDropCount)
{
	//	Use the old A/C driver call...
	AUTOCIRCULATE_DATA	autoCircData	(eFlushAutoCirculate);
	autoCircData.bVal1 = inClearDropCount;
	if (!GetCurrentACChannelCrosspoint (*this, inChannel, autoCircData.channelSpec))
		return false;

	const bool result(AutoCirculate(autoCircData));
	if (result)
		ACINFO(GetDescription() << ": Flushed Ch" << DEC(inChannel+1) << ", " << (inClearDropCount?"cleared":"retained") << " drop count");
	else
		ACFAIL(GetDescription() << ": Failed to flush Ch" << DEC(inChannel+1));
	return result;

}	//	AutoCirculateFlush


bool CNTV2Card::AutoCirculatePreRoll (const NTV2Channel inChannel, const ULWord inPreRollFrames)
{
	//	Use the old A/C driver call...
	AUTOCIRCULATE_DATA	autoCircData	(ePrerollAutoCirculate);
	autoCircData.lVal1 = LWord(inPreRollFrames);
	if (!GetCurrentACChannelCrosspoint (*this, inChannel, autoCircData.channelSpec))
		return false;

	const bool result(AutoCirculate(autoCircData));
	if (result)
		ACINFO(GetDescription() << ": Prerolled " << DEC(inPreRollFrames) << " frame(s) on Ch" << DEC(inChannel+1));
	else
		ACFAIL(GetDescription() << ": Failed to preroll " << DEC(inPreRollFrames) << " frame(s) on Ch" << DEC(inChannel+1));
	return result;

}	//	AutoCirculatePreRoll


bool CNTV2Card::AutoCirculateGetStatus (const NTV2Channel inChannel, AUTOCIRCULATE_STATUS & outStatus)
{
	outStatus.Clear ();
	if (!GetCurrentACChannelCrosspoint (*this, inChannel, outStatus.acCrosspoint))
		return false;

	if (!NTV2_IS_VALID_NTV2CROSSPOINT (outStatus.acCrosspoint))
	{
		const AUTOCIRCULATE_STATUS notRunningStatus (::NTV2ChannelToOutputCrosspoint (inChannel));
		outStatus = notRunningStatus;
		return true;	//	AutoCirculate not running on this channel
	}

	const bool result(NTV2Message(outStatus));
	if (!result)
		ACFAIL(GetDescription() << ": Failed to get status on Ch" << DEC(inChannel+1));
	return result;

}	//	AutoCirculateGetStatus


bool CNTV2Card::AutoCirculateGetFrameStamp (const NTV2Channel inChannel, const ULWord inFrameNum, FRAME_STAMP & outFrameStamp)
{
	//	Use the new driver call...
	outFrameStamp.acFrameTime = LWord64 (inChannel);
	outFrameStamp.acRequestedFrame = inFrameNum;
	return NTV2Message(outFrameStamp);

}	//	AutoCirculateGetFrameStamp


bool CNTV2Card::AutoCirculateSetActiveFrame (const NTV2Channel inChannel, const ULWord inNewActiveFrame)
{
	//	Use the old A/C driver call...
	AUTOCIRCULATE_DATA	autoCircData	(eSetActiveFrame);
	autoCircData.lVal1 = LWord(inNewActiveFrame);
	if (!GetCurrentACChannelCrosspoint (*this, inChannel, autoCircData.channelSpec))
		return false;

	const bool result(AutoCirculate(autoCircData));
	if (result)
		ACINFO(GetDescription() << ": Set active frame to " << DEC(inNewActiveFrame) << " on Ch" << DEC(inChannel+1));
	else
		ACFAIL(GetDescription() << ": Failed to set active frame to " << DEC(inNewActiveFrame) << " on Ch" << DEC(inChannel+1));
	return result;

}	//	AutoCirculateSetActiveFrame


bool CNTV2Card::AutoCirculateTransfer (const NTV2Channel inChannel, AUTOCIRCULATE_TRANSFER & inOutXferInfo)
{
	if (!_boardOpened)
		return false;
	#if defined(_DEBUG)
		NTV2_ASSERT (inOutXferInfo.NTV2_IS_STRUCT_VALID ());
	#endif

	NTV2Crosspoint			crosspoint	(NTV2CROSSPOINT_INVALID);
	NTV2EveryFrameTaskMode	taskMode	(NTV2_OEM_TASKS);
	if (!GetCurrentACChannelCrosspoint (*this, inChannel, crosspoint))
		return false;
	if (!NTV2_IS_VALID_NTV2CROSSPOINT(crosspoint))
		return false;
	GetEveryFrameServices(taskMode);

	if (NTV2_IS_INPUT_CROSSPOINT(crosspoint))
		inOutXferInfo.acTransferStatus.acFrameStamp.acTimeCodes.Fill(ULWord(0xFFFFFFFF));	//	Invalidate old timecodes
	else if (NTV2_IS_OUTPUT_CROSSPOINT(crosspoint))
	{
		bool isProgressive (false);
		IsProgressiveStandard(isProgressive, inChannel);
		if (inOutXferInfo.acRP188.IsValid())
			inOutXferInfo.SetAllOutputTimeCodes(inOutXferInfo.acRP188, /*alsoSetF2*/!isProgressive);

		const NTV2_RP188 *	pArray	(reinterpret_cast <const NTV2_RP188*>(inOutXferInfo.acOutputTimeCodes.GetHostPointer()));
		if (pArray	&&	pArray[NTV2_TCINDEX_DEFAULT].IsValid())
			inOutXferInfo.SetAllOutputTimeCodes(pArray[NTV2_TCINDEX_DEFAULT], /*alsoSetF2*/!isProgressive);
	}

	bool		tmpLocalF1AncBuffer(false),	 tmpLocalF2AncBuffer(false);
	NTV2Buffer	savedAncF1,	 savedAncF2;
	if (IsSupported(kDeviceCanDo2110)  &&  NTV2_IS_OUTPUT_CROSSPOINT(crosspoint))
	{
		//	S2110 Playout:	So that most Retail & OEM playout apps "just work" with S2110 RTP Anc streams,
		//					our classic SDI Anc data that device firmware normally embeds into SDI output
		//					as derived from registers -- VPID & RP188 -- the SDK here automatically inserts
		//					these packets into the outgoing RTP streams, even if the client didn't provide
		//					Anc buffers in the AUTOCIRCULATE_TRANSFER object, or specify AUTOCIRCULATE_WITH_ANC.
		ULWord	F1OffsetFromBottom(0),	F2OffsetFromBottom(0);
		size_t	F1SizeInBytes(0), F2SizeInBytes(0);
		if (GetAncRegionOffsetFromBottom(F1OffsetFromBottom, NTV2_AncRgn_Field1)
			&&	GetAncRegionOffsetFromBottom(F2OffsetFromBottom, NTV2_AncRgn_Field2))
		{
			F2SizeInBytes = size_t(F2OffsetFromBottom);
			if (F2OffsetFromBottom < F1OffsetFromBottom)
				F1SizeInBytes = size_t(F1OffsetFromBottom - F2OffsetFromBottom);
			else
				F1SizeInBytes = size_t(F2OffsetFromBottom - F1OffsetFromBottom);
		}
		if ((_boardID == DEVICE_ID_IOIP_2110) || (_boardID == DEVICE_ID_IOIP_2110_RGB12))
		{	//	IoIP 2110 Playout requires room for RTP+GUMP per anc buffer, to also operate SDI5 Mon output
			ULWord	F1MonOffsetFromBottom(0),  F2MonOffsetFromBottom(0);
			const bool good (GetAncRegionOffsetFromBottom(F1MonOffsetFromBottom, NTV2_AncRgn_MonField1)
							 &&	 GetAncRegionOffsetFromBottom(F2MonOffsetFromBottom, NTV2_AncRgn_MonField2));
			if (good	//	Driver expects anc regions in this order (from bottom): F2Mon, F2, F1Mon, F1
				&&	F2MonOffsetFromBottom < F2OffsetFromBottom
				&&	F2OffsetFromBottom < F1MonOffsetFromBottom
				&&	F1MonOffsetFromBottom < F1OffsetFromBottom)
			{
				F1SizeInBytes = size_t(F1OffsetFromBottom - F2OffsetFromBottom);
				F2SizeInBytes = size_t(F2OffsetFromBottom);
			}
			else
			{	//	Anc regions out of order!
				XMTWARN(GetDescription() << ": IoIP 2110 playout anc rgns disordered (offsets from bottom): F2Mon=" << HEX0N(F2MonOffsetFromBottom,8)
						<< " F2=" << HEX0N(F2OffsetFromBottom,8) << " F1Mon=" << HEX0N(F1MonOffsetFromBottom,8)
						<< " F1=" << HEX0N(F1OffsetFromBottom,8));
				F1SizeInBytes = F2SizeInBytes = 0;	//	Out of order, don't do Anc
			}
			savedAncF1 = inOutXferInfo.acANCBuffer;			//	copy
			savedAncF2 = inOutXferInfo.acANCField2Buffer;	//	copy
			if (inOutXferInfo.acANCBuffer.GetByteCount() < F1SizeInBytes)
			{	//	Enlarge acANCBuffer, and copy everything from savedAncF1 into it...
				inOutXferInfo.acANCBuffer.Allocate(F1SizeInBytes);
				inOutXferInfo.acANCBuffer.Fill(uint64_t(0));
				inOutXferInfo.acANCBuffer.CopyFrom(savedAncF1, 0, 0, savedAncF1.GetByteCount());
			}
			if (inOutXferInfo.acANCField2Buffer.GetByteCount() < F2SizeInBytes)
			{	//	Enlarge acANCField2Buffer, and copy everything from savedAncF2 into it...
				inOutXferInfo.acANCField2Buffer.Allocate(F2SizeInBytes);
				inOutXferInfo.acANCField2Buffer.Fill(uint64_t(0));
				inOutXferInfo.acANCField2Buffer.CopyFrom(savedAncF2, 0, 0, savedAncF2.GetByteCount());
			}
		}	//	if IoIP 2110 playout
		else
		{	//	else KonaIP 2110 playout
			if (inOutXferInfo.acANCBuffer.IsNULL())
				tmpLocalF1AncBuffer = inOutXferInfo.acANCBuffer.Allocate(F1SizeInBytes);
			else
				savedAncF1 = inOutXferInfo.acANCBuffer; //	copy
			if (inOutXferInfo.acANCField2Buffer.IsNULL())
				tmpLocalF2AncBuffer = inOutXferInfo.acANCField2Buffer.Allocate(F2SizeInBytes);
			else
				savedAncF2 = inOutXferInfo.acANCField2Buffer;	//	copy
		}	//	else KonaIP 2110 playout
		S2110DeviceAncToXferBuffers(inChannel, inOutXferInfo);
	}	//	if SMPTE 2110 playout
	else if (IsSupported(kDeviceCanDo2110)  &&  NTV2_IS_INPUT_CROSSPOINT(crosspoint))
	{	//	Need local host buffers to receive 2110 Anc VPID & ATC
		if (inOutXferInfo.acANCBuffer.IsNULL())
			tmpLocalF1AncBuffer = inOutXferInfo.acANCBuffer.Allocate(2048);
		if (inOutXferInfo.acANCField2Buffer.IsNULL())
			tmpLocalF2AncBuffer = inOutXferInfo.acANCField2Buffer.Allocate(2048);
	}	//	if SMPTE 2110 capture

	/////////////////////////////////////////////////////////////////////////////
	//	Call the driver...
	inOutXferInfo.acCrosspoint = crosspoint;
	bool result = NTV2Message(inOutXferInfo);
	/////////////////////////////////////////////////////////////////////////////

	if (result	&&	NTV2_IS_INPUT_CROSSPOINT(crosspoint))
	{
		if (IsSupported(kDeviceCanDo2110))
		{	//	S2110:	decode VPID and timecode anc packets from RTP, and put into A/C Xfer and device regs
			S2110DeviceAncFromXferBuffers(inChannel, inOutXferInfo);
		}
		if (taskMode == NTV2_STANDARD_TASKS)
		{
			//	After 12.? shipped, we discovered problems with timecode capture in our classic retail stuff.
			//	The acTimeCodes[NTV2_TCINDEX_DEFAULT] was coming up empty.
			//	Rather than fix all three drivers -- the Right, but Difficult Thing To Do --
			//	we decided to do the Easy Thing, here, in user-space.

			//	First, determine the ControlPanel's current Input source (SDIIn1/HDMIIn1 or SDIIn2/HDMIIn2)...
			ULWord	inputSelect (NTV2_Input1Select);
			ReadRegister (kVRegInputSelect, inputSelect);
			const bool	bIsInput2	(inputSelect == NTV2_Input2Select);

			//	Next, determine the ControlPanel's current TimeCode source (LTC? VITC1? VITC2)...
			RP188SourceFilterSelect TimecodeSource(kRP188SourceEmbeddedLTC);
			CNTV2DriverInterface::ReadRegister(kVRegRP188SourceSelect, TimecodeSource);

			//	Now convert that into an NTV2TCIndex...
			NTV2TCIndex TimecodeIndex = NTV2_TCINDEX_DEFAULT;
			switch (TimecodeSource)
			{
				default:/*kRP188SourceEmbeddedLTC:*/TimecodeIndex = bIsInput2 ? NTV2_TCINDEX_SDI2_LTC : NTV2_TCINDEX_SDI1_LTC;	break;
				case kRP188SourceEmbeddedVITC1:		TimecodeIndex = bIsInput2 ? NTV2_TCINDEX_SDI2	  : NTV2_TCINDEX_SDI1;		break;
				case kRP188SourceEmbeddedVITC2:		TimecodeIndex = bIsInput2 ? NTV2_TCINDEX_SDI2_2	  : NTV2_TCINDEX_SDI1_2;	break;
				case kRP188SourceLTCPort:			TimecodeIndex = NTV2_TCINDEX_LTC1;											break;
			}

			//	Fetch the TimeCode value that's in that NTV2TCIndex slot...
			NTV2_RP188	tcValue;
			inOutXferInfo.GetInputTimeCode(tcValue, TimecodeIndex);
			if (TimecodeIndex == NTV2_TCINDEX_LTC1)
			{	//	Special case for external LTC:
				//	Our driver currently returns all-zero DBB values for external LTC.
				//	It should probably at least set DBB BIT(17) "selected RP188 received" if external LTC is present.
				//	Ticket 3367: Our QuickTime 'vdig' relies on DBB BIT(17) being set, or it assumes timecode is invalid
				if (tcValue.fLo	 &&	 tcValue.fHi  &&  tcValue.fLo != 0xFFFFFFFF	 &&	 tcValue.fHi != 0xFFFFFFFF)
					tcValue.fDBB |= 0x00020000;
			}

			//	Valid or not, stuff that TimeCode value into inOutXferInfo.acTransferStatus.acFrameStamp.acTimeCodes[NTV2_TCINDEX_DEFAULT]...
			NTV2_RP188 *	pArray	(reinterpret_cast <NTV2_RP188 *> (inOutXferInfo.acTransferStatus.acFrameStamp.acTimeCodes.GetHostPointer()));
			if (pArray)
				pArray [NTV2_TCINDEX_DEFAULT] = tcValue;
		}	//	if retail mode
	}	//	if NTV2Message OK && capturing
	if (result	&&	NTV2_IS_OUTPUT_CROSSPOINT(crosspoint))
	{
		if (savedAncF1)
			inOutXferInfo.acANCBuffer = savedAncF1;			//	restore
		if (savedAncF2)
			inOutXferInfo.acANCField2Buffer = savedAncF2;	//	restore
	}	//	if successful playout

	if (tmpLocalF1AncBuffer)
		inOutXferInfo.acANCBuffer.Deallocate();
	if (tmpLocalF2AncBuffer)
		inOutXferInfo.acANCField2Buffer.Deallocate();

	#if defined (AJA_NTV2_CLEAR_DEVICE_ANC_BUFFER_AFTER_CAPTURE_XFER)
		if (result	&&	NTV2_IS_INPUT_CROSSPOINT(crosspoint))
		{
			ULWord	doZeroing	(0);
			if (ReadRegister(kVRegZeroDeviceAncPostCapture, doZeroing)	&&	doZeroing)
			{	//	Zero out the Anc buffer on the device...
				static NTV2Buffer gClearDeviceAncBuffer;
				const LWord		xferFrame	(inOutXferInfo.GetTransferFrameNumber());
				ULWord			ancOffsetF1 (0);
				ULWord			ancOffsetF2 (0);
				NTV2Framesize	fbSize		(NTV2_FRAMESIZE_INVALID);
				ReadRegister(kVRegAncField1Offset, ancOffsetF1);
				ReadRegister(kVRegAncField2Offset, ancOffsetF2);
				GetFrameBufferSize(inChannel, fbSize);
				const ULWord	fbByteCount (::NTV2FramesizeToByteCount(fbSize));
				const ULWord	ancOffset	(ancOffsetF2 > ancOffsetF1	?  ancOffsetF2	:  ancOffsetF1);	//	Use whichever is larger
				NTV2_ASSERT (xferFrame != -1);
				if (gClearDeviceAncBuffer.IsNULL() || (gClearDeviceAncBuffer.GetByteCount() != ancOffset))
				{
					gClearDeviceAncBuffer.Allocate(ancOffset);	//	Allocate it
					gClearDeviceAncBuffer.Fill (ULWord(0));		//	Clear it
				}
				if (xferFrame != -1	 &&	 fbByteCount  &&  !gClearDeviceAncBuffer.IsNULL())
					DMAWriteSegments (ULWord(xferFrame),
									reinterpret_cast<ULWord*>(gClearDeviceAncBuffer.GetHostPointer()),	//	host buffer
									fbByteCount - ancOffset,				//	device memory offset, in bytes
									gClearDeviceAncBuffer.GetByteCount(),	//	total number of bytes to xfer
									1,										//	numSegments -- one chunk of 'ancOffset'
									gClearDeviceAncBuffer.GetByteCount(),	//	segmentHostPitch
									gClearDeviceAncBuffer.GetByteCount());	//	segmentCardPitch
			}
		}
	#endif	//	AJA_NTV2_CLEAR_DEVICE_ANC_BUFFER_AFTER_CAPTURE_XFER

	#if defined (AJA_NTV2_CLEAR_HOST_ANC_BUFFER_TAIL_AFTER_CAPTURE_XFER)
		if (result	&&	NTV2_IS_INPUT_CROSSPOINT(crosspoint))
		{
			ULWord	doZeroing	(0);
			if (ReadRegister(kVRegZeroHostAncPostCapture, doZeroing)  &&  doZeroing)
			{	//	Zero out everything past the last captured Anc byte in the client's host buffer(s)... 
				NTV2Buffer &	clientAncBufferF1	(inOutXferInfo.acANCBuffer);
				NTV2Buffer &	clientAncBufferF2	(inOutXferInfo.acANCField2Buffer);
				const ULWord	ancF1ByteCount		(inOutXferInfo.GetCapturedAncByteCount(false));
				const ULWord	ancF2ByteCount		(inOutXferInfo.GetCapturedAncByteCount(true));
				void *			pF1TailEnd			(clientAncBufferF1.GetHostAddress(ancF1ByteCount));
				void *			pF2TailEnd			(clientAncBufferF2.GetHostAddress(ancF2ByteCount));
				if (pF1TailEnd	&&	clientAncBufferF1.GetByteCount() > ancF1ByteCount)
					::memset (pF1TailEnd, 0, clientAncBufferF1.GetByteCount() - ancF1ByteCount);
				if (pF2TailEnd	&&	clientAncBufferF2.GetByteCount() > ancF2ByteCount)
					::memset (pF2TailEnd, 0, clientAncBufferF2.GetByteCount() - ancF2ByteCount);
			}
		}
	#endif	//	AJA_NTV2_CLEAR_HOST_ANC_BUFFER_TAIL_AFTER_CAPTURE_XFER

	if (result)
		ACDBG(GetDescription() << ": Transfer successful for Ch" << DEC(inChannel+1));
	else
		ACFAIL(GetDescription() << ": Transfer failed on Ch" << DEC(inChannel+1));
	return result;

}	//	AutoCirculateTransfer


static const AJA_FrameRate	sNTV2Rate2AJARate[] = { AJA_FrameRate_Unknown	//	NTV2_FRAMERATE_UNKNOWN	= 0,
													,AJA_FrameRate_6000		//	NTV2_FRAMERATE_6000		= 1,
													,AJA_FrameRate_5994		//	NTV2_FRAMERATE_5994		= 2,
													,AJA_FrameRate_3000		//	NTV2_FRAMERATE_3000		= 3,
													,AJA_FrameRate_2997		//	NTV2_FRAMERATE_2997		= 4,
													,AJA_FrameRate_2500		//	NTV2_FRAMERATE_2500		= 5,
													,AJA_FrameRate_2400		//	NTV2_FRAMERATE_2400		= 6,
													,AJA_FrameRate_2398		//	NTV2_FRAMERATE_2398		= 7,
													,AJA_FrameRate_5000		//	NTV2_FRAMERATE_5000		= 8,
													,AJA_FrameRate_4800		//	NTV2_FRAMERATE_4800		= 9,
													,AJA_FrameRate_4795		//	NTV2_FRAMERATE_4795		= 10,
													,AJA_FrameRate_12000	//	NTV2_FRAMERATE_12000	= 11,
													,AJA_FrameRate_11988	//	NTV2_FRAMERATE_11988	= 12,
													,AJA_FrameRate_1500		//	NTV2_FRAMERATE_1500		= 13,
													,AJA_FrameRate_1498		//	NTV2_FRAMERATE_1498		= 14,
#if !defined(NTV2_DEPRECATE_16_0)
													,AJA_FrameRate_1900		//	NTV2_FRAMERATE_1900		= 15,	// Formerly 09 in older SDKs
													,AJA_FrameRate_1898		//	NTV2_FRAMERATE_1898		= 16,	// Formerly 10 in older SDKs
													,AJA_FrameRate_1800		//	NTV2_FRAMERATE_1800		= 17,	// Formerly 11 in older SDKs
													,AJA_FrameRate_1798		//	NTV2_FRAMERATE_1798		= 18,	// Formerly 12 in older SDKs
#endif	//	!defined(NTV2_DEPRECATE_16_0)
													};

static const TimecodeFormat sNTV2Rate2TCFormat[] = {kTCFormatUnknown	//	NTV2_FRAMERATE_UNKNOWN	= 0,
													,kTCFormat60fps		//	NTV2_FRAMERATE_6000		= 1,
													,kTCFormat30fps		//	NTV2_FRAMERATE_5994		= 2,
													,kTCFormat30fps		//	NTV2_FRAMERATE_3000		= 3,
													,kTCFormat30fps		//	NTV2_FRAMERATE_2997		= 4,
													,kTCFormat25fps		//	NTV2_FRAMERATE_2500		= 5,
													,kTCFormat24fps		//	NTV2_FRAMERATE_2400		= 6,
													,kTCFormat24fps		//	NTV2_FRAMERATE_2398		= 7,
													,kTCFormat50fps		//	NTV2_FRAMERATE_5000		= 8,
													,kTCFormat48fps		//	NTV2_FRAMERATE_4800		= 9,
													,kTCFormat48fps		//	NTV2_FRAMERATE_4795		= 10,
													,kTCFormat60fps		//	NTV2_FRAMERATE_12000	= 11,
													,kTCFormat60fps		//	NTV2_FRAMERATE_11988	= 12,
													,kTCFormat30fps		//	NTV2_FRAMERATE_1500		= 13,
													,kTCFormat30fps		//	NTV2_FRAMERATE_1498		= 14,
#if !defined(NTV2_DEPRECATE_16_0)
													,kTCFormatUnknown	//	NTV2_FRAMERATE_1900		= 15,
													,kTCFormatUnknown	//	NTV2_FRAMERATE_1898		= 16,
													,kTCFormatUnknown	//	NTV2_FRAMERATE_1800		= 17,
													,kTCFormatUnknown	//	NTV2_FRAMERATE_1798		= 18,
#endif	//	!defined(NTV2_DEPRECATE_16_0)
													};

//	VPID Packet Insertion						1080	720		525		625		1080p	2K		2K1080p		2K1080i		UHD		4K		UHDHFR		4KHFR
static const uint16_t	sVPIDLineNumsF1[] = {	10,		10,		13,		9,		10,		10,		10,			10,			10,		10,		10,			10	};
static const uint16_t	sVPIDLineNumsF2[] = {	572,	0,		276,	322,	0,		0,		0,			572,		0,		0,		0,			0	};

//	SDI RX Status Registers (for setting/clearing "VPID Present" bits)
static const uint32_t	gSDIInRxStatusRegs[] = {kRegRXSDI1Status, kRegRXSDI2Status, kRegRXSDI3Status, kRegRXSDI4Status, kRegRXSDI5Status, kRegRXSDI6Status, kRegRXSDI7Status, kRegRXSDI8Status, 0};


bool CNTV2Card::S2110DeviceAncFromXferBuffers (const NTV2Channel inChannel, AUTOCIRCULATE_TRANSFER & inOutXferInfo)
{
	//	IP 2110 Capture:	Extract timecode(s) and put into inOutXferInfo.acTransferStatus.acFrameStamp.acTimeCodes...
	//						Extract VPID and put into SDIIn VPID regs
	NTV2FrameRate		ntv2Rate		(NTV2_FRAMERATE_UNKNOWN);
	bool				result			(GetFrameRate(ntv2Rate, inChannel));
	bool				isProgressive	(false);
	const bool			isMonitoring	(AJADebug::IsActive(AJA_DebugUnit_Anc2110Rcv));
	NTV2Standard		standard		(NTV2_STANDARD_INVALID);
	NTV2Buffer &		ancF1			(inOutXferInfo.acANCBuffer);
	NTV2Buffer &		ancF2			(inOutXferInfo.acANCField2Buffer);
	AJAAncillaryData *	pPkt			(AJA_NULL);
	uint32_t			vpidA(0), vpidB(0);
	AJAAncillaryList	pkts;

	if (!result)
		return false;	//	Can't get frame rate
	if (!NTV2_IS_VALID_NTV2FrameRate(ntv2Rate))
		return false;	//	Bad frame rate
	if (!GetStandard(standard, inChannel))
		return false;	//	Can't get standard
	if (!NTV2_IS_VALID_STANDARD(standard))
		return false;	//	Bad standard
	isProgressive = NTV2_IS_PROGRESSIVE_STANDARD(standard);
	if (!ancF1.IsNULL() || !ancF2.IsNULL())
		if (AJA_FAILURE(AJAAncillaryList::SetFromDeviceAncBuffers(ancF1, ancF2, pkts)))
			return false;	//	Packet import failed

	const NTV2SmpteLineNumber	smpteLineNumInfo	(::GetSmpteLineNumber(standard));
	const uint32_t				F2StartLine			(isProgressive ? 0 : smpteLineNumInfo.GetLastLine());	//	F2 VANC starts past last line of F1

	//	Look for ATC and VITC...
	for (uint32_t ndx(0);  ndx < pkts.CountAncillaryData();	 ndx++)
	{
		pPkt = pkts.GetAncillaryDataAtIndex(ndx);
		if (pPkt->GetDID() == 0x41	&&	pPkt->GetSID() == 0x01) //	VPID?
		{	//	VPID!
			if (pPkt->GetDC() != 4)
				continue;	//	Skip . . . expected DC == 4
			const uint32_t* pULWord		(reinterpret_cast<const uint32_t*>(pPkt->GetPayloadData()));
			uint32_t		vpidValue	(pULWord ? *pULWord : 0);
			if (!pPkt->GetDataLocation().IsHanc())
				continue;	//	Skip . . . expected IsHANC
			vpidValue = NTV2EndianSwap32BtoH(vpidValue);
			if (IS_LINKB_AJAAncDataStream(pPkt->GetDataLocation().GetDataStream()))
				vpidB = vpidValue;
			else
				vpidA = vpidValue;
			continue;	//	Done . . . on to next packet
		}

		const AJAAncDataType ancType (pPkt->GetAncillaryDataType());
		if (ancType != AJAAncDataType_Timecode_ATC)
		{
			if (ancType == AJAAncDataType_Timecode_VITC  &&  isMonitoring)
				RCVWARN(GetDescription() << ": Skipped Ch" << (inChannel+1) << " VITC packet: " << pPkt->AsString(16));
			continue;	//	Not timecode . . . skip
		}

		//	Got ATC packet!
		AJAAncillaryData_Timecode_ATC * pATCPkt(reinterpret_cast<AJAAncillaryData_Timecode_ATC*>(pPkt));
		if (!pATCPkt)
			continue;

		AJAAncillaryData_Timecode_ATC_DBB1PayloadType	payloadType (AJAAncillaryData_Timecode_ATC_DBB1PayloadType_Unknown);
		pATCPkt->GetDBB1PayloadType(payloadType);
		NTV2TCIndex tcNdx (NTV2_TCINDEX_INVALID);
		switch(payloadType)
		{
			case AJAAncillaryData_Timecode_ATC_DBB1PayloadType_LTC:
				tcNdx = ::NTV2ChannelToTimecodeIndex (inChannel, /*inEmbeddedLTC*/true, /*inIsF2*/false);
				break;
			case AJAAncillaryData_Timecode_ATC_DBB1PayloadType_VITC1:
				tcNdx = ::NTV2ChannelToTimecodeIndex (inChannel, /*inEmbeddedLTC*/false, /*inIsF2*/false);
				break;
			case AJAAncillaryData_Timecode_ATC_DBB1PayloadType_VITC2:
				tcNdx = ::NTV2ChannelToTimecodeIndex (inChannel, /*inEmbeddedLTC*/false, /*inIsF2*/true);
				break;
			default:
				break;
		}
		if (!NTV2_IS_VALID_TIMECODE_INDEX(tcNdx))
			continue;

		NTV2_RP188	ntv2rp188;	//	<== This is what we want to get from pATCPkt
		AJATimeCode ajaTC;		//	We can get an AJATimeCode from it via GetTimecode
		const AJA_FrameRate ajaRate (sNTV2Rate2AJARate[ntv2Rate]);
		AJATimeBase			ajaTB	(ajaRate);

		bool isDF = false;
		AJAAncillaryData_Timecode_Format tcFmt = pATCPkt->GetTimecodeFormatFromTimeBase(ajaTB);
		pATCPkt->GetDropFrameFlag(isDF, tcFmt);

		pATCPkt->GetTimecode(ajaTC, ajaTB);
								//	There is an AJATimeCode function to get an NTV2_RP188:
								//	ajaTC.QueryRP188(ntv2rp188.fDBB, ntv2rp188.fLo, ntv2rp188.fHi, ajaTB, isDF);
								//	But it's not implemented!  D'OH!!
								//	Let the hacking begin...

		string	tcStr;
		ajaTC.QueryString(tcStr, ajaTB, isDF);
		CRP188 rp188(tcStr, sNTV2Rate2TCFormat[ntv2Rate]);
		rp188.SetDropFrame(isDF);
		rp188.GetRP188Reg(ntv2rp188);
		//	Finally, poke the RP188 timecode into the Input Timecodes array...
		inOutXferInfo.acTransferStatus.acFrameStamp.SetInputTimecode(tcNdx, ntv2rp188);
	}	//	for each anc packet

	if (isMonitoring)
	{
		NTV2TimeCodes	timecodes;
		inOutXferInfo.acTransferStatus.GetFrameStamp().GetInputTimeCodes(timecodes, inChannel);
		if (!timecodes.empty()) RCVDBG("Channel" << DEC(inChannel+1) << " timecodes: " << timecodes);
	}
	if (vpidA || vpidB)
	{
		if (isMonitoring)
			RCVDBG(GetDescription() << ": WriteSDIInVPID Ch" << DEC(inChannel+1) << " VPIDa=" << xHEX0N(vpidA,4) << " VPIDb=" << xHEX0N(vpidB,4));
		WriteSDIInVPID(inChannel, vpidA, vpidB);
	}
	WriteRegister(gSDIInRxStatusRegs[inChannel], vpidA ? 1 : 0, BIT(20), 20);	//	Set RX VPID Valid LinkA bit if vpidA non-zero
	WriteRegister(gSDIInRxStatusRegs[inChannel], vpidB ? 1 : 0, BIT(21), 21);	//	Set RX VPID Valid LinkB bit if vpidB non-zero

	//	Normalize to SDI/GUMP...
	return AJA_SUCCESS(pkts.GetTransmitData(ancF1, ancF2, isProgressive, F2StartLine));

}	//	S2110DeviceAncFromXferBuffers


bool CNTV2Card::S2110DeviceAncFromBuffers (const NTV2Channel inChannel, NTV2Buffer & ancF1, NTV2Buffer & ancF2)
{
	//	IP 2110 Capture:	Extract timecode(s) and put into RP188 registers
	//						Extract VPID and put into SDIIn VPID registers
	AUTOCIRCULATE_TRANSFER	tmpXfer;	tmpXfer.acANCBuffer = ancF1;	tmpXfer.acANCField2Buffer = ancF2;
	if (!S2110DeviceAncFromXferBuffers (inChannel, tmpXfer))	//	<== This handles stuffing the VPID regs
		{RCVFAIL(GetDescription() << ": Ch" << (inChannel+1) << ": S2110DeviceAncFromXferBuffers failed");  return false;}

	NTV2TimeCodes	timecodes;
	if (!tmpXfer.acTransferStatus.GetFrameStamp().GetInputTimeCodes(timecodes, inChannel))
		{RCVFAIL(GetDescription() << ": Ch" << (inChannel+1) << ": GetInputTimeCodes failed");  return false;}

	for (NTV2TimeCodesConstIter iter(timecodes.begin());  iter != timecodes.end();	++iter)
	{
		//const bool		isLTC		(NTV2_IS_ATC_LTC_TIMECODE_INDEX(iter->first));
		const NTV2_RP188	ntv2rp188	(iter->second);
		SetRP188Data (inChannel, ntv2rp188);	//	Poke the timecode into the SDIIn timecode regs
	}	//	for each good timecode associated with "inChannel"
	//	No need to log timecodes, already done in S2110DeviceAncFromXferBuffers:	//ANCDBG(timecodes);

	return true;

}	//	S2110DeviceAncFromBuffers


static inline uint32_t EndianSwap32NtoH (const uint32_t inValue)	{return NTV2EndianSwap32BtoH(inValue);} //	Guaranteed no in-place byte-swap -- always copies


bool CNTV2Card::S2110DeviceAncToXferBuffers (const NTV2Channel inChannel, AUTOCIRCULATE_TRANSFER & inOutXferInfo)
{
	//	IP 2110 Playout:	Add relevant transmit timecodes and VPID to outgoing RTP Anc
	NTV2FrameRate		ntv2Rate		(NTV2_FRAMERATE_UNKNOWN);
	bool				result			(GetFrameRate(ntv2Rate, inChannel));
	bool				isProgressive	(false);
	bool				generateRTP		(false);
	const bool			isMonitoring	(AJADebug::IsActive(AJA_DebugUnit_Anc2110Xmit));
	const bool			isIoIP2110		((_boardID == DEVICE_ID_IOIP_2110) || (_boardID == DEVICE_ID_IOIP_2110_RGB12));
	NTV2Standard		standard		(NTV2_STANDARD_INVALID);
	NTV2Buffer &		ancF1			(inOutXferInfo.acANCBuffer);
	NTV2Buffer &		ancF2			(inOutXferInfo.acANCField2Buffer);
	NTV2TaskMode		taskMode		(NTV2_OEM_TASKS);
	ULWord				vpidA(0), vpidB(0);
	AJAAncillaryList	packetList;
	const NTV2Channel	SDISpigotChannel(GetEveryFrameServices(taskMode) && NTV2_IS_STANDARD_TASKS(taskMode)  ?	 NTV2_CHANNEL3	:  inChannel);
	ULWord				F1OffsetFromBottom(0),	F2OffsetFromBottom(0),	F1MonOffsetFromBottom(0),  F2MonOffsetFromBottom(0);
	if (!result)
		return false;	//	Can't get frame rate
	if (!NTV2_IS_VALID_NTV2FrameRate(ntv2Rate))
		return false;	//	Bad frame rate
	if (!GetStandard(standard, inChannel))
		return false;	//	Can't get standard
	if (!NTV2_IS_VALID_STANDARD(standard))
		return false;	//	Bad standard
	isProgressive = NTV2_IS_PROGRESSIVE_STANDARD(standard);
	const NTV2SmpteLineNumber	smpteLineNumInfo	(::GetSmpteLineNumber(standard));
	const uint32_t				F2StartLine			(smpteLineNumInfo.GetLastLine());	//	F2 VANC starts past last line of F1

	//	IoIP 2110 Playout requires RTP+GUMP per anc buffer to operate SDI5 Mon output...
	GetAncRegionOffsetFromBottom(F1OffsetFromBottom,	NTV2_AncRgn_Field1);
	GetAncRegionOffsetFromBottom(F2OffsetFromBottom,	NTV2_AncRgn_Field2);
	GetAncRegionOffsetFromBottom(F1MonOffsetFromBottom, NTV2_AncRgn_MonField1);
	GetAncRegionOffsetFromBottom(F2MonOffsetFromBottom, NTV2_AncRgn_MonField2);
	//	Define F1 & F2 GUMP sub-buffers from ancF1 & ancF2 (only used for IoIP 2110)...
	NTV2Buffer gumpF1(ancF1.GetHostAddress(F1OffsetFromBottom - F1MonOffsetFromBottom), // addr
						F1MonOffsetFromBottom - F2OffsetFromBottom);	// byteCount
	NTV2Buffer gumpF2(ancF2.GetHostAddress(F2OffsetFromBottom - F2MonOffsetFromBottom), // addr
						F2MonOffsetFromBottom); // byteCount

	if (ancF1 || ancF2)
	{
		//	Import anc packet list that AutoCirculateTransfer's caller put into Xfer struct's Anc buffers (GUMP or RTP).
		//	We're going to add VPID and timecode packets to the list.
		if (AJA_FAILURE(AJAAncillaryList::SetFromDeviceAncBuffers(ancF1, ancF2, packetList)))
			return false;	//	Packet import failed

		if (!packetList.IsEmpty())
		{
			const bool	isF1RTP (ancF1 ? AJARTPAncPayloadHeader::BufferStartsWithRTPHeader(ancF1) : false);
			const bool	isF2RTP (ancF2 ? AJARTPAncPayloadHeader::BufferStartsWithRTPHeader(ancF2) : false);
			if (isIoIP2110 && isF1RTP && isF2RTP)
			{	//	Generate F1 & F2 GUMP from F1 & F2 RTP...
				packetList.GetSDITransmitData(gumpF1, gumpF2, isProgressive, F2StartLine);
			}
			else
			{
				if (ancF1)
				{
					if (isF1RTP)
					{	//	Caller F1 buffer contains RTP
						if (isIoIP2110)
						{	//	Generate GUMP from packetList...
							NTV2Buffer skipF2Data;
							packetList.GetSDITransmitData(gumpF1, skipF2Data, isProgressive, F2StartLine);
						}
					}
					else
					{	//	Caller F1 buffer contains GUMP
						generateRTP = true; //	Force conversion to RTP
						if (isIoIP2110)
						{	//	Copy GUMP to where the driver expects it...
							const ULWord	gumpLength (std::min(F1MonOffsetFromBottom - F2OffsetFromBottom, gumpF1.GetByteCount()));
							gumpF1.CopyFrom(/*src=*/ancF1,	/*srcOffset=*/0,  /*dstOffset=*/0,	/*byteCount=*/gumpLength);
						}	//	if IoIP
					}	//	if F1 is GUMP
				}	//	if ancF1 non-NULL
				if (ancF2)
				{
					if (isF2RTP)
					{	//	Caller F2 buffer contains RTP
						if (isIoIP2110)
						{	//	Generate GUMP from packetList...
							NTV2Buffer skipF1Data;
							packetList.GetSDITransmitData(skipF1Data, gumpF2, isProgressive, F2StartLine);
						}
					}
					else
					{	//	Caller F2 buffer contains GUMP
						generateRTP = true; //	Force conversion to RTP
						if (isIoIP2110)
						{	//	Copy GUMP to where the driver expects it...
							const ULWord	gumpLength (std::min(F2MonOffsetFromBottom, gumpF2.GetByteCount()));
							gumpF2.CopyFrom(/*src=*/ancF2,	/*srcOffset=*/0,  /*dstOffset=*/0,	/*byteCount=*/gumpLength);
						}	//	if IoIP
					}	//	if F2 is GUMP
				}	//	if ancF2 non-NULL
			}	//	else not IoIP or not F1RTP or not F2RTP
		}	//	if caller supplied any anc
	}	//	if either buffer non-empty/NULL

	if (isMonitoring)	XMTDBG("ORIG: " << packetList); //	Original packet list from caller

	//	Callers can override our register-based VPID values...
	if (!packetList.CountAncillaryDataWithID(0x41,0x01))			//	If no VPID packets in buffer...
	{
		if (GetSDIOutVPID(vpidA, vpidB, UWord(SDISpigotChannel)))	//	...then we'll add them...
		{
			AJAAncillaryData	vpidPkt;
			vpidPkt.SetDID(0x41);
			vpidPkt.SetSID(0x01);
			vpidPkt.SetLocationVideoLink(AJAAncDataLink_A);
			vpidPkt.SetLocationDataStream(AJAAncDataStream_1);
			vpidPkt.SetLocationDataChannel(AJAAncDataChannel_Y);
			vpidPkt.SetLocationHorizOffset(AJAAncDataHorizOffset_AnyHanc);
			if (vpidA)
			{	//	LinkA/DS1:
				vpidA = ::EndianSwap32NtoH(vpidA);
				vpidPkt.SetPayloadData (reinterpret_cast<uint8_t*>(&vpidA), 4);
				vpidPkt.SetLocationLineNumber(sVPIDLineNumsF1[standard]);
				vpidPkt.GeneratePayloadData();
				packetList.AddAncillaryData(vpidPkt);	generateRTP = true;
				if (!isProgressive)
				{	//	Ditto for Field 2...
					vpidPkt.SetLocationLineNumber(sVPIDLineNumsF2[standard]);
					packetList.AddAncillaryData(vpidPkt);	generateRTP = true;
				}
			}
			if (vpidB)
			{	//	LinkB/DS2:
				vpidB = ::EndianSwap32NtoH(vpidB);
				vpidPkt.SetPayloadData (reinterpret_cast<uint8_t*>(&vpidB), 4);
				vpidPkt.SetLocationVideoLink(AJAAncDataLink_B);
				vpidPkt.SetLocationDataStream(AJAAncDataStream_2);
				vpidPkt.GeneratePayloadData();
				packetList.AddAncillaryData(vpidPkt);	generateRTP = true;
				if (!isProgressive)
				{	//	Ditto for Field 2...
					vpidPkt.SetLocationLineNumber(sVPIDLineNumsF2[standard]);
					packetList.AddAncillaryData(vpidPkt);	generateRTP = true;
				}
			}
		}	//	if user not inserting his own VPID
		else if (isMonitoring)	{XMTWARN("GetSDIOutVPID failed for SDI spigot " << ::NTV2ChannelToString(SDISpigotChannel,true));}
	}	//	if no VPID pkts in buffer
	else if (isMonitoring)	{XMTDBG(DEC(packetList.CountAncillaryDataWithID(0x41,0x01)) << " VPID packet(s) already provided, won't insert any here");}
	//	IoIP monitor GUMP VPID cannot be overridden -- SDI anc insert always inserts VPID via firmware

	//	Callers can override our register-based RP188 values...
	if (!packetList.CountAncillaryDataWithType(AJAAncDataType_Timecode_ATC)		//	if no caller-specified ATC timecodes...
		&& !packetList.CountAncillaryDataWithType(AJAAncDataType_Timecode_VITC))	//	...and no caller-specified VITC timecodes...
	{
		if (inOutXferInfo.acOutputTimeCodes)		//	...and if there's an output timecode array...
		{
			const AJA_FrameRate ajaRate		(sNTV2Rate2AJARate[ntv2Rate]);
			const AJATimeBase	ajaTB		(ajaRate);
			const NTV2TCIndexes tcIndexes	(::GetTCIndexesForSDIConnector(SDISpigotChannel));
			const size_t		maxNumTCs	(inOutXferInfo.acOutputTimeCodes.GetByteCount() / sizeof(NTV2_RP188));
			NTV2_RP188 *		pTimecodes	(reinterpret_cast<NTV2_RP188*>(inOutXferInfo.acOutputTimeCodes.GetHostPointer()));

			//	For each timecode index for this channel...
			for (NTV2TCIndexesConstIter it(tcIndexes.begin());	it != tcIndexes.end();	++it)
			{
				const NTV2TCIndex	tcNdx(*it);
				if (size_t(tcNdx) >= maxNumTCs)
					continue;	//	Skip -- not in the array
				if (!NTV2_IS_SDI_TIMECODE_INDEX(tcNdx))
					continue;	//	Skip -- analog or invalid

				const NTV2_RP188	regTC	(pTimecodes[tcNdx]);
				if (!regTC)
					continue;	//	Skip -- invalid timecode (all FFs)

				const bool isDF = AJATimeCode::QueryIsRP188DropFrame(regTC.fDBB, regTC.fLo, regTC.fHi);

				AJATimeCode						tc;		tc.SetRP188(regTC.fDBB, regTC.fLo, regTC.fHi, ajaTB);
				AJAAncillaryData_Timecode_ATC	atc;	atc.SetTimecode (tc, ajaTB, isDF);
				atc.SetDBB (uint8_t(regTC.fDBB & 0x000000FF), uint8_t(regTC.fDBB & 0x0000FF00 >> 8));
				if (NTV2_IS_ATC_VITC2_TIMECODE_INDEX(tcNdx))	//	VITC2?
				{
					atc.SetDBB1PayloadType(AJAAncillaryData_Timecode_ATC_DBB1PayloadType_VITC2);
					atc.SetLocationLineNumber(sVPIDLineNumsF2[standard] - 1);	//	Line 9 in F2
				}
				else
				{	//	F1 -- only consider LTC and VITC1 ... nothing else
					if (NTV2_IS_ATC_VITC1_TIMECODE_INDEX(tcNdx))	//	VITC1?
						atc.SetDBB1PayloadType(AJAAncillaryData_Timecode_ATC_DBB1PayloadType_VITC1);
					else if (NTV2_IS_ATC_LTC_TIMECODE_INDEX(tcNdx)) //	LTC?
						atc.SetDBB1PayloadType(AJAAncillaryData_Timecode_ATC_DBB1PayloadType_LTC);
					else
						continue;
				}
				atc.GeneratePayloadData();
				packetList.AddAncillaryData(atc);	generateRTP = true;
			}	//	for each timecode index value
		}	//	if user not inserting his own ATC/VITC
		else if (isMonitoring)	{XMTWARN("Cannot insert ATC/VITC -- Xfer struct has no acOutputTimeCodes array!");}
	}	//	if no ATC/VITC packets in buffer
	else if (isMonitoring)	{XMTDBG("ATC and/or VITC packet(s) already provided, won't insert any here");}
	//	IoIP monitor GUMP VPID cannot be overridden -- SDI anc inserter inserts RP188 via firmware

	if (generateRTP)	//	if anything added (or forced conversion from GUMP)
	{	//	Re-encode packets into the XferStruct buffers as RTP...
		//XMTDBG("CHGD: " << packetList);	//	DEBUG:	Changed packet list (to be converted to RTP)
		const bool		multiRTPPkt = inOutXferInfo.acTransferStatus.acState == NTV2_AUTOCIRCULATE_INVALID	?  true	 :	false;
		packetList.SetAllowMultiRTPTransmit(multiRTPPkt);
		NTV2Buffer rtpF1 (ancF1.GetHostAddress(0),  isIoIP2110  ?	 F1OffsetFromBottom - F1MonOffsetFromBottom	 :	ancF1.GetByteCount());
		NTV2Buffer rtpF2 (ancF2.GetHostAddress(0),  isIoIP2110  ?	 F2OffsetFromBottom - F2MonOffsetFromBottom	 :	ancF2.GetByteCount());
		result = AJA_SUCCESS(packetList.GetIPTransmitData (rtpF1, rtpF2, isProgressive, F2StartLine));
		//if (isIoIP2110)	XMTDBG("F1RTP: " << rtpF1 << " F2RTP: " << rtpF2 << " Xfer: " << inOutXferInfo);
#if 0	/// Development
		DMAWriteAnc(31, rtpF1, rtpF2, NTV2_CHANNEL_INVALID);	//	DEBUG: DMA RTP into frame 31
		if (result)
		{
			AJAAncillaryList	compareRTP; //	RTP into compareRTP
			NTV2_ASSERT(AJA_SUCCESS(AJAAncillaryList::SetFromDeviceAncBuffers(rtpF1, rtpF2, compareRTP)));
			//if (packetList.GetAncillaryDataWithID(0x61,0x02)->GetChecksum() != compareRTP.GetAncillaryDataWithID(0x61,0x02)->GetChecksum())
			//XMTDBG("COMPRTP608: " << packetList.GetAncillaryDataWithID(0x61,0x02)->AsString(8) << compareRTP.GetAncillaryDataWithID(0x61,0x02)->AsString(8));
			string compRTP (compareRTP.CompareWithInfo(packetList, /*ignoreLocation*/false, /*ignoreChecksum*/false));
			if (!compRTP.empty())
				XMTWARN("MISCOMPARE: " << compRTP);
		}
		if (isIoIP2110)
		{
			DMAWriteAnc(32, gumpF1, gumpF2, NTV2_CHANNEL_INVALID);	//	DEBUG: DMA GUMP into frame 32
			if (result)
			{
				AJAAncillaryList	compareGUMP;	//	GUMP into compareGUMP
				NTV2_ASSERT(AJA_SUCCESS(AJAAncillaryList::SetFromDeviceAncBuffers(gumpF1, gumpF2, compareGUMP)));
				//if (packetList.GetAncillaryDataWithID(0x61,0x02)->GetChecksum() != compareGUMP.GetAncillaryDataWithID(0x61,0x02)->GetChecksum())
				//XMTDBG("COMPGUMP608: " << packetList.GetAncillaryDataWithID(0x61,0x02)->AsString(8) << compareGUMP.GetAncillaryDataWithID(0x61,0x02)->AsString(8));
				string compGUMP (compareGUMP.CompareWithInfo(packetList, /*ignoreLocation*/false, /*ignoreChecksum*/false));
				if (!compGUMP.empty())
					XMTWARN("MISCOMPARE: " << compGUMP);
			}
		}	//	IoIP2110
#endif	///	Development
	}	//	if generateRTP
	return result;

}	//	S2110DeviceAncToXferBuffers


bool CNTV2Card::S2110DeviceAncToBuffers (const NTV2Channel inChannel, NTV2Buffer & ancF1, NTV2Buffer & ancF2)
{
	//	IP 2110 Playout:	Add relevant transmit timecodes and VPID to outgoing RTP Anc
	NTV2FrameRate		ntv2Rate		(NTV2_FRAMERATE_UNKNOWN);
	bool				result			(GetFrameRate(ntv2Rate, inChannel));
	bool				isProgressive	(false);
	bool				changed			(false);
	const bool			isMonitoring	(AJADebug::IsActive(AJA_DebugUnit_Anc2110Xmit));
	NTV2Standard		standard		(NTV2_STANDARD_INVALID);
	const NTV2Channel	SDISpigotChannel(inChannel);	//	DMAWriteAnc usually for OEM clients -- just use inChannel
	ULWord				vpidA(0), vpidB(0);
	AJAAncillaryList	pkts;

	if (!result)
		return false;	//	Can't get frame rate
	if (!NTV2_IS_VALID_NTV2FrameRate(ntv2Rate))
		return false;	//	Bad frame rate
	if (!GetStandard(standard, inChannel))
		return false;	//	Can't get standard
	if (!NTV2_IS_VALID_STANDARD(standard))
		return false;	//	Bad standard
	isProgressive = NTV2_IS_PROGRESSIVE_STANDARD(standard);
	if (!ancF1.IsNULL() || !ancF2.IsNULL())
		if (AJA_FAILURE(AJAAncillaryList::SetFromDeviceAncBuffers(ancF1, ancF2, pkts)))
			return false;	//	Packet import failed

	const NTV2SmpteLineNumber	smpteLineNumInfo	(::GetSmpteLineNumber(standard));
	const uint32_t				F2StartLine			(smpteLineNumInfo.GetLastLine());	//	F2 VANC starts past last line of F1

	//	Non-autocirculate users can transmit VPID two ways:
	//	1)	Insert a VPID packet into the Anc buffer(s) themselves, or
	//	2)	Set the SDI Out VPID register before calling DMAWriteAnc
	//	Extract VPID and place into our SDI In VPID register...
	if (pkts.CountAncillaryDataWithID(0x41,0x01))			//	If no VPID packets in buffer...
	{
		if (GetSDIOutVPID(vpidA, vpidB, UWord(SDISpigotChannel)))	//	...then we'll add them...
		{
			AJAAncillaryData	vpidPkt;
			vpidPkt.SetDID(0x41);
			vpidPkt.SetSID(0x01);
			vpidPkt.SetLocationVideoLink(AJAAncDataLink_A);
			vpidPkt.SetLocationDataStream(AJAAncDataStream_1);
			vpidPkt.SetLocationDataChannel(AJAAncDataChannel_Y);
			vpidPkt.SetLocationHorizOffset(AJAAncDataHorizOffset_AnyHanc);
			if (vpidA)
			{	//	LinkA/DS1:
				vpidA = ::EndianSwap32NtoH(vpidA);
				vpidPkt.SetPayloadData (reinterpret_cast<uint8_t*>(&vpidA), 4);
				vpidPkt.SetLocationLineNumber(sVPIDLineNumsF1[standard]);
				pkts.AddAncillaryData(vpidPkt);			changed = true;
				if (!isProgressive)
				{	//	Ditto for Field 2...
					vpidPkt.SetLocationLineNumber(sVPIDLineNumsF2[standard]);
					pkts.AddAncillaryData(vpidPkt); changed = true;
				}
			}
			if (vpidB)
			{	//	LinkB/DS2:
				vpidB = ::EndianSwap32NtoH(vpidB);
				vpidPkt.SetPayloadData (reinterpret_cast<uint8_t*>(&vpidB), 4);
				vpidPkt.SetLocationVideoLink(AJAAncDataLink_B);
				vpidPkt.SetLocationDataStream(AJAAncDataStream_2);
				vpidPkt.GeneratePayloadData();
				pkts.AddAncillaryData(vpidPkt); changed = true;
				if (!isProgressive)
				{	//	Ditto for Field 2...
					vpidPkt.SetLocationLineNumber(sVPIDLineNumsF2[standard]);
					pkts.AddAncillaryData(vpidPkt); changed = true;
				}
			}
		}	//	if client didn't insert their own VPID
	}	//	if no VPID pkts in buffer
	else if (isMonitoring)	{XMTDBG(DEC(pkts.CountAncillaryDataWithID(0x41,0x01)) << " VPID packet(s) already provided, won't insert any here");}

	//	Non-autocirculate users can transmit timecode two ways:
	//	1)	Insert ATC or VITC packets into the Anc buffers themselves, or
	//	2)	Set output timecode using RP188 registers using CNTV2Card::SetRP188Data,
	//		(but this will only work on newer boards with bidirectional SDI)
	if (!pkts.CountAncillaryDataWithType(AJAAncDataType_Timecode_ATC)		//	if no caller-specified ATC timecodes...
		&& !pkts.CountAncillaryDataWithType(AJAAncDataType_Timecode_VITC))	//	...and no caller-specified VITC timecodes...
	{
		if (IsSupported(kDeviceHasBiDirectionalSDI) && IsSupported(kDeviceCanDoStackedAudio))	//	if newer device with bidirectional SDI
		{
			const AJA_FrameRate ajaRate		(sNTV2Rate2AJARate[ntv2Rate]);
			const AJATimeBase	ajaTB		(ajaRate);
			const NTV2TCIndexes tcIndexes	(::GetTCIndexesForSDIConnector(SDISpigotChannel));
			NTV2_RP188			regTC;

			//	Supposedly, these newer devices support playout readback of their RP188 registers...
			GetRP188Data (inChannel, regTC);
			if (regTC)
			{
				//	For each timecode index for this channel...
				for (NTV2TCIndexesConstIter it(tcIndexes.begin());	it != tcIndexes.end();	++it)
				{
					const NTV2TCIndex	tcNdx(*it);
					if (!NTV2_IS_SDI_TIMECODE_INDEX(tcNdx))
						continue;	//	Skip -- analog or invalid

					//	TBD:  Does the DBB indicate which timecode it's intended for?
					//	i.e. VITC? LTC? VITC2?
					//	For now, transmit all three...		TBD

					const bool isDF = AJATimeCode::QueryIsRP188DropFrame(regTC.fDBB, regTC.fLo, regTC.fHi);

					AJATimeCode						tc;		tc.SetRP188(regTC.fDBB, regTC.fLo, regTC.fHi, ajaTB);
					AJAAncillaryData_Timecode_ATC	atc;	atc.SetTimecode (tc, ajaTB, isDF);
					atc.AJAAncillaryData_Timecode_ATC::SetDBB (uint8_t(regTC.fDBB & 0x000000FF), uint8_t(regTC.fDBB & 0x0000FF00 >> 8));
					if (NTV2_IS_ATC_VITC2_TIMECODE_INDEX(tcNdx))	//	VITC2?
					{
						if (isProgressive)
							continue;	//	Progressive -- skip VITC2
						atc.SetDBB1PayloadType(AJAAncillaryData_Timecode_ATC_DBB1PayloadType_VITC2);
						atc.SetLocationLineNumber(sVPIDLineNumsF2[standard] - 1);	//	Line 9 in F2
					}
					else
					{	//	F1 -- only consider LTC and VITC1 ... nothing else
						if (NTV2_IS_ATC_VITC1_TIMECODE_INDEX(tcNdx))	//	VITC1?
							atc.SetDBB1PayloadType(AJAAncillaryData_Timecode_ATC_DBB1PayloadType_VITC1);
						else if (NTV2_IS_ATC_LTC_TIMECODE_INDEX(tcNdx)) //	LTC?
							atc.SetDBB1PayloadType(AJAAncillaryData_Timecode_ATC_DBB1PayloadType_LTC);
						else
							continue;
					}
					atc.GeneratePayloadData();
					pkts.AddAncillaryData(atc);				changed = true;
				}	//	for each timecode index value
			}	//	if GetRP188Data returned valid timecode
		}	//	if newer device with bidirectional spigots
	}	//	if client didn't insert their own ATC/VITC
	else if (isMonitoring)	{XMTDBG("ATC and/or VITC packet(s) already provided, won't insert any here");}

	if (changed)
	{	//	We must re-encode packets into the RTP buffers only if anything new was added...
		ancF1.Fill(ULWord(0));	ancF2.Fill(ULWord(0));	//	Clear/reset anc RTP buffers
		//XMTDBG(pkts);
		result = AJA_SUCCESS(pkts.GetIPTransmitData (ancF1, ancF2, isProgressive, F2StartLine));
#if 0	///	Development
		if (result)
		{
			AJAAncillaryList	comparePkts;
			NTV2_ASSERT(AJA_SUCCESS(AJAAncillaryList::SetFromDeviceAncBuffers(ancF1, ancF2, comparePkts)));
			//XMTDBG(comparePkts);
			string compareResult (comparePkts.CompareWithInfo(pkts,false,false));
			if (!compareResult.empty())
				XMTWARN("MISCOMPARE: " << compareResult);
		}
#endif	///	Development
	}
	return result;

}	//	S2110DeviceAncToBuffers
