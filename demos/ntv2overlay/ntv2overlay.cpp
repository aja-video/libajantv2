/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2overlay.cpp
	@brief		Implementation of NTV2Overlay demonstration class.
	@copyright	(C) 2012-2022 AJA Video Systems, Inc.  All rights reserved.
**/

#include "ntv2overlay.h"
#include "ntv2devicescanner.h"
#include "ntv2testpatterngen.h"	//	Needed for --novideo
#include "ntv2vpid.h"
#include "ajabase/common/types.h"
#include "ajabase/system/process.h"
#include "ajabase/system/systemtime.h"
#include "ajaanc/includes/ancillarylist.h"
#include <iostream>

using namespace std;

#define	OVRLFAIL	BURNFAIL
#define	OVRLWARN	BURNWARN
#define	OVRLDBG		BURNDBG
#define	OVRLNOTE	BURNNOTE
#define	OVRLINFO	BURNINFO

//#define NTV2_BUFFER_LOCKING		//	Define this to pre-lock video/audio buffers in kernel
#define HasWidgetsAnyOf(_w1,_w2,_w3)	(mDevice.features().CanDoWidget(_w1) || mDevice.features().CanDoWidget(_w2) || mDevice.features().CanDoWidget(_w3))
#if defined(_DEBUG)
	static const bool sShowConfig(true);
#else
	static const bool sShowConfig(false);
#endif
static const uint32_t	kAppSignature	(NTV2_FOURCC('O','v','r','l'));


//////////////////////	IMPLEMENTATION


NTV2Overlay::NTV2Overlay (const OverlayConfig & inConfig)
	:	mConfig			(inConfig),
		mPlayThread		(AJAThread()),
		mCaptureThread	(AJAThread()),
		mVideoFormat	(NTV2_FORMAT_UNKNOWN),
		mHDMIColorSpace	(NTV2_LHIHDMIColorSpaceYCbCr),
		mSavedTaskMode	(NTV2_DISABLE_TASKS),
		mGlobalQuit		(false)
{
}	//	constructor


NTV2Overlay::~NTV2Overlay ()
{
	Quit();	//	Stop my capture and playout threads, then destroy them
	mDevice.UnsubscribeInputVerticalEvent(mConfig.fInputChannel);	//	Unsubscribe from input VBI event
}	//	destructor


void NTV2Overlay::Quit (void)
{
	//	Set the global 'quit' flag, and wait for the threads to go inactive...
	mGlobalQuit = true;

	while (mPlayThread.Active())
		AJATime::Sleep(10);

	while (mCaptureThread.Active())
		AJATime::Sleep(10);

	if (!mConfig.fDoMultiFormat)
	{	//	Release the device...
		mDevice.ReleaseStreamForApplication (kAppSignature, int32_t(AJAProcess::GetPid()));
		if (NTV2_IS_VALID_TASK_MODE(mSavedTaskMode))
			mDevice.SetEveryFrameServices(mSavedTaskMode);	//	Restore prior task mode
	}
}	//	Quit


AJAStatus NTV2Overlay::Init (void)
{
	AJAStatus status (AJA_STATUS_SUCCESS);

	//	Open the device...
	if (!CNTV2DeviceScanner::GetFirstDeviceFromArgument (mConfig.fDeviceSpec, mDevice))
		{cerr << "## ERROR:  Device '" << mConfig.fDeviceSpec << "' not found" << endl;  return AJA_STATUS_OPEN;}
    if (!mDevice.IsDeviceReady(false))
		{cerr << "## ERROR:  Device '" << mConfig.fDeviceSpec << "' not ready" << endl;  return AJA_STATUS_INITIALIZE;}

	ULWord	appSig(0);
	int32_t	appPID(0);
	mDevice.GetStreamingApplication (appSig, appPID);	//	Who currently "owns" the device?
	mDevice.GetEveryFrameServices(mSavedTaskMode);		//	Save the current device state

	if (!mDevice.AcquireStreamForApplication (kAppSignature, int32_t(AJAProcess::GetPid())))
	{
		cerr << "## ERROR:  Cannot acquire device because another app (pid " << appPID << ") owns it" << endl;
		return AJA_STATUS_BUSY;		//	Some other app is using the device
	}
	mDevice.ClearRouting();	//	Clear current device routing (since I "own" the device)
	
	mDevice.SetEveryFrameServices(NTV2_OEM_TASKS);	//	Force OEM tasks

	//	Set up the overlay image...
	status = SetupOverlayBug();
	if (AJA_FAILURE(status))
		return status;

	//	Set up the video and audio...
	status = SetupVideo();
	if (AJA_FAILURE(status))
		return status;
	status = SetupAudio();
	if (AJA_FAILURE(status))
		return status;

	//	Ready to go...
	if (mConfig.IsVerbose() || sShowConfig)
	{	cerr << mConfig;
		if (mDevice.IsRemote())
			cerr	<< "Device Description:  " << mDevice.GetDescription() << endl;
		cerr << endl;
	}
	OVRLINFO("Configuration: " << mConfig);
	return AJA_STATUS_SUCCESS;

}	//	Init


AJAStatus NTV2Overlay::SetupVideo (void)
{
	//	Enable/subscribe interrupts...
	mConfig.fInputChannel = NTV2_CHANNEL1;
	mConfig.fOutputChannel = NTV2_CHANNEL2;
	mDevice.SubscribeInputVerticalEvent(NTV2_CHANNEL1);
	mDevice.SubscribeOutputVerticalEvent(NTV2_CHANNEL1);
	mDevice.SubscribeOutputVerticalEvent(NTV2_CHANNEL2);

	//	Flip the input spigot to "receive" if necessary...
	mDevice.SetSDITransmitEnable (NTV2_CHANNEL1, false);
	mDevice.SetSDITransmitEnable (NTV2_CHANNEL2, true);
	
	mDevice.WaitForOutputVerticalInterrupt (NTV2_CHANNEL1, 10);			//	...and give device time to lock to input

	//	Since this demo runs in E-to-E mode (thru Mixer/Keyer), reference must be tied to the input...
	mDevice.SetReference(NTV2_REFERENCE_INPUT1);

	//	Transparent overlays will need an ARGB buffer format...
	mConfig.fPixelFormat = NTV2_FBF_ARGB;

	//	If the device supports different per-channel video formats, configure it as requested...
	mDevice.SetMultiFormatMode(true);
	
	mDevice.ClearRouting();
	RouteInputSignal();
	RouteOutputSignal();
	
	//	Set up Mixer...
	const UWord mixerNum (0);
	mDevice.SetMixerMode (mixerNum, NTV2MIXERMODE_MIX);	//	"mix" mode
	mDevice.SetMixerFGMatteEnabled (mixerNum, false);	//	no FG matte
	mDevice.SetMixerBGMatteEnabled (mixerNum, false);	//	no BG matte
	mDevice.SetMixerCoefficient (mixerNum, 0x10000);	//	FG "bug" overlay full strength (initially)
	mDevice.SetMixerFGInputControl (mixerNum, NTV2MIXERINPUTCONTROL_SHAPED);		//	respect FG alpha channel
	mDevice.SetMixerBGInputControl (mixerNum, NTV2MIXERINPUTCONTROL_FULLRASTER);	//	BG uses full raster
	mDevice.SetMixerVancOutputFromForeground (mixerNum, false);	//	false means "use BG VANC, not FG"
	
	return AJA_STATUS_SUCCESS;

}	//	SetupVideo

bool NTV2Overlay::RouteInputSignal (void)
{
	mInputConnections.clear();
	const NTV2OutputXptID inputOutputXpt (::GetInputSourceOutputXpt(mConfig.fInputSource));
	const NTV2InputXptID mixBGVidInputXpt(::GetMixerBGInputXpt(mConfig.fInputChannel));
	const NTV2InputXptID mixBGKeyInputXpt(::GetMixerBGInputXpt(mConfig.fInputChannel, /*key*/true));
	const NTV2InputXptID fsInputXpt (::GetFrameStoreInputXptFromChannel(mConfig.fInputChannel));
	
	if (mConfig.fInputSource == NTV2_INPUTSOURCE_HDMI1
		&&  mDevice.GetHDMIInputColor(mHDMIColorSpace, mConfig.fInputChannel)
		&&  mHDMIColorSpace == NTV2_LHIHDMIColorSpaceRGB)
	{
		//	The HDMI input video is RGB, but the Mixer only accepts YCbCr video or key.
		//	The HDMI video signal must go thru a CSC on its way to the Mixer...
		const NTV2InputXptID cscVidInputXpt(::GetCSCInputXptFromChannel(mConfig.fInputChannel));
		const NTV2OutputXptID cscVidOutputXpt(::GetCSCOutputXptFromChannel(mConfig.fInputChannel, false/*isKey*/));
		const NTV2OutputXptID cscKeyOutputXpt(::GetCSCOutputXptFromChannel(mConfig.fInputChannel, true/*isKey*/));
		
		mInputConnections.insert(NTV2XptConnection(cscVidInputXpt, inputOutputXpt));	//	CSC vid input from HDMI input
		mInputConnections.insert(NTV2XptConnection(mixBGVidInputXpt, cscVidOutputXpt));	//	Mixer BG vid input from CSC vid output
		mInputConnections.insert(NTV2XptConnection(mixBGKeyInputXpt, cscKeyOutputXpt));	//	Mixer BG key input from CSC key output
		mInputConnections.insert(NTV2XptConnection(fsInputXpt, cscVidOutputXpt));		//	Capture FrameStore's input from CSC vid output
	}
	else
	{
		mInputConnections.insert(NTV2XptConnection(mixBGVidInputXpt, inputOutputXpt));	//	Mixer BG vid input from input spigot
		mInputConnections.insert(NTV2XptConnection(mixBGKeyInputXpt, inputOutputXpt));	//	Mixer BG key input from input spigot
		mInputConnections.insert(NTV2XptConnection(fsInputXpt, inputOutputXpt));		//	Capture FrameStore's input from input spigot
	}
	return mDevice.ApplySignalRoute(mInputConnections);
	
}	//	RouteInputSignal


bool NTV2Overlay::RouteOutputSignal (void)
{
	{	//	FrameStore to CSC to Mixer FG routing...
		const NTV2OutputXptID fsRGBOutputXpt (::GetFrameStoreOutputXptFromChannel(mConfig.fOutputChannel, /*rgb*/true));
		const NTV2InputXptID cscVidInputXpt(::GetCSCInputXptFromChannel(mConfig.fOutputChannel));
		const NTV2OutputXptID cscVidOutputXpt(::GetCSCOutputXptFromChannel(mConfig.fOutputChannel, /*key*/false));
		const NTV2OutputXptID cscKeyOutputXpt(::GetCSCOutputXptFromChannel(mConfig.fOutputChannel, /*key*/true));
		const NTV2InputXptID mixFGVidInputXpt(::GetMixerFGInputXpt(mConfig.fInputChannel));
		const NTV2InputXptID mixFGKeyInputXpt(::GetMixerFGInputXpt(mConfig.fInputChannel, /*key*/true));
		
		mOutputConnections.insert(NTV2XptConnection(cscVidInputXpt, fsRGBOutputXpt));		//	CSC vid input from FrameStore RGB output
		mOutputConnections.insert(NTV2XptConnection(mixFGVidInputXpt, cscVidOutputXpt));	//	Mixer FG vid input from CSC vid output
		mOutputConnections.insert(NTV2XptConnection(mixFGKeyInputXpt, cscKeyOutputXpt));	//	Mixer FG key input from CSC key output
	}
	{	//	Mixer to Output
		const NTV2InputXptID outputInputXpt (::GetOutputDestInputXpt(mConfig.fOutputDest));
		const NTV2OutputXptID mixOutputXpt (::GetMixerOutputXptFromChannel(mConfig.fInputChannel));
		mOutputConnections.insert(NTV2XptConnection(outputInputXpt, mixOutputXpt));	//	SDIOut from Mixer output
		mOutputConnections.insert(NTV2XptConnection(NTV2_XptHDMIOutQ1Input, mixOutputXpt));	//	HDMIOut from Mixer output
		
	}
	return mDevice.ApplySignalRoute(mOutputConnections);
	
}	//	RouteOutputSignal


AJAStatus NTV2Overlay::SetupAudio (void)
{
	//	Set up the output audio embedders...
	NTV2AudioSystem audioSystem = NTV2_AUDIOSYSTEM_1;
	mDevice.SetSDIOutputAudioSystem (NTV2_CHANNEL2, audioSystem);
	mDevice.SetHDMIOutAudioSource2Channel(NTV2_AudioChannel1_2, audioSystem);

	//	Enable loopback for E-E mode (i.e. output whatever audio is in input signal)...
	mDevice.SetAudioLoopBack (NTV2_AUDIO_LOOPBACK_OFF, audioSystem); // Don't turn loopback on until input signal is stable
	return AJA_STATUS_SUCCESS;

}	//	SetupAudio

AJAStatus NTV2Overlay::SetupHostBuffers (void)
{
	CNTV2DemoCommon::SetDefaultPageSize();	//	Set host-specific page size
	
	//	Let my circular buffer know when it's time to quit...
	mFrameDataRing.SetAbortFlag (&mGlobalQuit);
	
	//  Determine video buffer size...
	const ULWord vidBuffSizeBytes (mFormatDesc.GetVideoWriteSize(ULWord(NTV2Buffer::DefaultPageSize())));
	
	//	Determine per-field max Anc buffer size...
	ULWord ancBuffSizeBytes (0);
	if (!mDevice.GetAncRegionOffsetFromBottom (ancBuffSizeBytes, NTV2_AncRgn_Field2))
		ancBuffSizeBytes = NTV2_ANCSIZE_MAX;
	
	//	Allocate and add each in-host NTV2FrameData to my mFrameDataRing...
	mHostBuffers.reserve(CIRCULAR_BUFFER_SIZE);
	while (mHostBuffers.size() < CIRCULAR_BUFFER_SIZE)
	{
		mHostBuffers.push_back(NTV2FrameData());			//	Make a new NTV2FrameData...
		NTV2FrameData & frameData (mHostBuffers.back());	//	...and get a reference to it
		
		//	Allocate a page-aligned video buffer (if handling video)...
		if (!mConfig.fSuppressVideo)
			if (!frameData.fVideoBuffer.Allocate (vidBuffSizeBytes, /*pageAligned*/true))
			{
				BURNFAIL("Failed to allocate " << xHEX0N(vidBuffSizeBytes,8) << "-byte video buffer");
				return AJA_STATUS_MEMORY;
			}
#ifdef NTV2_BUFFER_LOCKING
		if (frameData.fVideoBuffer)
			mDevice.DMABufferLock(frameData.fVideoBuffer, true);
#endif
#if 0
		//	Allocate a page-aligned audio buffer (if handling audio)...
		if (NTV2_IS_VALID_AUDIO_SYSTEM(mAudioSystem)  &&  mConfig.WithAudio())
			if (!frameData.fAudioBuffer.Allocate (NTV2_AUDIOSIZE_MAX, /*pageAligned*/true))
			{
				BURNFAIL("Failed to allocate " << xHEX0N(NTV2_AUDIOSIZE_MAX,8) << "-byte audio buffer");
				return AJA_STATUS_MEMORY;
			}
		if (frameData.AudioBuffer())
			frameData.fAudioBuffer.Fill(ULWord(0));
		
		if (mConfig.WithAnc() && !mConfig.WithTallVANC())
		{	//	Allocate page-aligned anc buffers...
			if (!frameData.fAncBuffer.Allocate (ancBuffSizeBytes, /*pageAligned*/true))
			{
				BURNFAIL("Failed to allocate " << xHEX0N(ancBuffSizeBytes,8) << "-byte anc buffer");
				return AJA_STATUS_MEMORY;
			}
			if (!::IsProgressivePicture(mVideoFormat))
				if (!frameData.fAncBuffer2.Allocate(ancBuffSizeBytes, /*pageAligned*/true))
				{
					BURNFAIL("Failed to allocate " << xHEX0N(ancBuffSizeBytes,8) << "-byte F2 anc buffer");
					return AJA_STATUS_MEMORY;
				}
		}
		if (frameData.AncBuffer())
			frameData.AncBuffer().Fill(ULWord(0));
		if (frameData.AncBuffer2())
			frameData.AncBuffer2().Fill(ULWord(0));
#endif
		//	Add this NTV2FrameData to the ring...
		mFrameDataRing.Add(&frameData);
	}	//	for each NTV2FrameData
	
	return AJA_STATUS_SUCCESS;
	
}	//	SetupHostBuffers


//	4-byte-per-pixel DrawVLine -- draws a vertical line using the given pixel value and line thickness
static bool DrawVLine (NTV2Buffer & buf, const NTV2RasterInfo & info, const ULWord argbPixValue, const ULWord pixThickness,
						const ULWord xPos, const ULWord yTop, const ULWord height)
{
	for (ULWord y(yTop);  y < (yTop + height);  y++)
		for (ULWord w(0);  w < pixThickness;  w++)
			buf.U32(y * info.linePitch  +  xPos + w) = argbPixValue;
	return true;
}

//	4-byte-per-pixel DrawHLine -- draws a horizontal line using the given pixel value and line thickness
static bool DrawHLine (NTV2Buffer & buf, const NTV2RasterInfo & info, const ULWord argbPixValue, const ULWord vThickness,
						const ULWord yPos, const ULWord xLeft, const ULWord width)
{
	for (ULWord v(0);  v < vThickness;  v++)
		for (ULWord x(xLeft);  x < (xLeft + width);  x++)
			buf.U32((yPos + v) * info.linePitch  +  x) = argbPixValue;
	return true;
}

//	4-byte-per-pixel DrawBox -- draws a rectangle using the given pixel value and line thickness
static bool DrawBox (NTV2Buffer & buf, const NTV2RasterInfo & info, const ULWord argbPixValue, const UWord pixThickness,
					const UWord topLeftX, const UWord topLeftY, const ULWord width, const ULWord height)
{
	/*T*/DrawVLine (buf, info, argbPixValue, pixThickness, /*xPos*/topLeftX,                         /*yTop */topLeftY, /*hght*/height);
	/*B*/DrawVLine (buf, info, argbPixValue, pixThickness, /*xPos*/topLeftX + width - pixThickness,  /*yTop */topLeftY, /*hght*/height);
	/*L*/DrawHLine (buf, info, argbPixValue, pixThickness, /*yPos*/topLeftY,                         /*xLeft*/topLeftX, /*wdth*/width);
	/*R*/DrawHLine (buf, info, argbPixValue, pixThickness, /*yPos*/topLeftY + height - pixThickness, /*xLeft*/topLeftX, /*wdth*/width);
	return true;
}

AJAStatus NTV2Overlay::SetupOverlayBug (void)
{
	static const ULWord sOpaqueRed(0xFFFF0000), sOpaqueBlue(0xFF0000FF), sOpaqueGreen(0xFF008000), sOpaqueWhite(0xFFFFFFFF), sOpaqueBlack(0xFF000000);

	//	The overlay "bug" is a 256x256 pixel raster image composed of four opaque rectangles of varying color and
	//	diminishing size, each set inside each other, centered in the raster. All space between them is transparent to
	//	reveal the background video.  The line thickness of each rectangle is the same:  1/16th the width or height of
	//	the raster. The pixel format is NTV2_FBF_ARGB, which all devices handle, and easy to traverse at 4-bytes-per-pixel.
	static const vector<ULWord> sColors = {sOpaqueWhite, sOpaqueRed, sOpaqueBlue, sOpaqueGreen};
	const ULWord hght(256), wdth(256), thickness(wdth/16);

	//	The overlay bug's NTV2RasterInfo must be hacked together manually:
	mBugRasterInfo = NTV2RasterInfo (hght, wdth, /*U32s/line*/wdth, /*1stAct*/0, /*lumaBits*/0, /*chromaBits*/8, /*alphaBits*/8);

	//	Allocate the overlay bug's host buffer:
	if (!mBug.Allocate(mBugRasterInfo.GetTotalBytes()))
		return AJA_STATUS_MEMORY;

	//	Draw the rectangles into the mBug buffer...
	for (size_t n(0);  n < sColors.size();  n++)
		DrawBox (mBug, mBugRasterInfo, sColors.at(n), thickness, /*topLeftX*/n*2*thickness, /*topLeftY*/n*2*thickness, wdth-2*n*2*thickness, hght-2*n*2*thickness);

	//	Draw a hatch mark in the middle of it...
	/*V*/DrawVLine (mBug, mBugRasterInfo, sOpaqueBlack, /*thickness*/1, /*xPos*/wdth/2, /*yTop */hght/2-thickness, /*hght*/2*thickness);
	/*H*/DrawHLine (mBug, mBugRasterInfo, sOpaqueBlack, /*thickness*/1, /*yPos*/hght/2, /*xLeft*/wdth/2-thickness, /*wdth*/2*thickness);
	return AJA_STATUS_SUCCESS;

}	//	SetupOverlayBug

NTV2VideoFormat NTV2Overlay::WaitForStableInputSignal (void)
{
	NTV2VideoFormat	result(NTV2_FORMAT_UNKNOWN), lastVF(NTV2_FORMAT_UNKNOWN);
	ULWord	numConsecutiveFrames(0), MIN_NUM_CONSECUTIVE_FRAMES(6);

	//	Detection loop:
	while (result == NTV2_FORMAT_UNKNOWN)
	{
		//	Determine the input video signal format...
		//	Warning:  if there's no input signal, this loop won't exit until mGlobalQuit goes true!
		UWord loopCount(0);
		while (result == NTV2_FORMAT_UNKNOWN)
		{
			mDevice.WaitForInputVerticalInterrupt(mConfig.fInputChannel);
			if (mGlobalQuit)
				return NTV2_FORMAT_UNKNOWN;	//	Terminate if asked to do so

			const NTV2VideoFormat currVF (mDevice.GetInputVideoFormat(mConfig.fInputSource));
			if (currVF == NTV2_FORMAT_UNKNOWN)
			{	//	Wait for video signal to appear
				if (++loopCount % 500 == 0)	//	Log message every minute or so at ~50ms
					OVRLDBG("Waiting for valid video signal to appear at " << ::NTV2InputSourceToString(mConfig.fInputSource,true));
			}
			else if (numConsecutiveFrames == 0)
			{
				lastVF = currVF;		//	First valid video format to appear
				numConsecutiveFrames++;	//	Start debounce counter
			}
			else if (numConsecutiveFrames == MIN_NUM_CONSECUTIVE_FRAMES)
			{
				numConsecutiveFrames = 0;	//	Reset for next signal outage
				result = currVF;			//	Set official video format to use
			}
			else
				numConsecutiveFrames = (lastVF == currVF) ? numConsecutiveFrames + 1 : 0;
		}	//	loop while input video format is unstable

		//	At this point, the video format is stable and valid.
		if (!mDevice.features().CanDoVideoFormat(result))	//	Can this device handle this video format?
		{
			OVRLWARN(mDevice.GetModelName() << " can't handle " << ::NTV2VideoFormatToString(result));
			cerr << endl << "## NOTE:  " << mDevice.GetModelName() << " can't handle " << ::NTV2VideoFormatToString(result) << endl;
			result = NTV2_FORMAT_UNKNOWN;
			mDevice.WaitForInputVerticalInterrupt(mConfig.fInputChannel, 30);	//	Wait 30 frames
			continue;	//	Retry
		}
		OVRLNOTE(::NTV2InputSourceToString(mConfig.fInputSource,true) << " video format: " << ::NTV2VideoFormatToString(result));
		cerr << endl << "## NOTE:  " << ::NTV2InputSourceToString(mConfig.fInputSource,true)
						<< " video format is " << ::NTV2VideoFormatToString(result) << endl;
		break;	//	Done!
	}	//	loop 

	NTV2_ASSERT(result != NTV2_FORMAT_UNKNOWN);
	return result;
}	//	WaitForStableInputSignal


AJAStatus NTV2Overlay::Run (void)
{
	//	Start the input thread...
	StartInputThread();
	return AJA_STATUS_SUCCESS;

}	//	Run



//////////////////////////////////////////////

//	This is where we will start the play thread
void NTV2Overlay::StartOutputThread (void)
{
	//	Create and start the playout thread...
	mPlayThread.Attach(OutputThreadStatic, this);
	mPlayThread.SetPriority(AJA_ThreadPriority_High);
	mPlayThread.Start();

}	//	StartOutputThread


//	The playout thread function
void NTV2Overlay::OutputThreadStatic (AJAThread * pThread, void * pContext)		//	static
{	(void) pThread;
	//	Grab the NTV2Overlay instance pointer from the pContext parameter,
	//	then call its OutputThread method...
	NTV2Overlay * pApp (reinterpret_cast<NTV2Overlay*>(pContext));
	pApp->OutputThread();

}	//	OutputThreadStatic

static ULWord gPlayEnterCount(0), gPlayExitCount(0);

//
//	The output thread:
//		-	at startup:
//			-	configures the overlay FrameStore
//			-	initializes (but doesn't start) AutoCirculate (to allocate 2 device buffers)
//		-	while running:
//			-	blits the overlay image into the host overlay raster buffer
//			-	DMAs the overlay raster buffer to the device for mixing
//		-	terminates when AutoCirculate channel is stopped or when mGlobalQuit goes true
//			-	ensures my AutoCirculate channel is stopped
//
void NTV2Overlay::OutputThread (void)
{
	AJAAtomic::Increment(&gPlayEnterCount);
	const ULWord threadTally(gPlayEnterCount);
	ULWord goodWaits(0), badWaits(0), goodBlits(0), badBlits(0), goodXfers(0), badXfers(0), pingPong(0), fbNum(10);
	AUTOCIRCULATE_STATUS acStatus;
	NTV2Buffer hostBuffer;

	//	Configure the output FrameStore...
	mDevice.EnableChannel(mConfig.fOutputChannel);
	mDevice.SetMode (mConfig.fOutputChannel, NTV2_MODE_DISPLAY);
	mDevice.SetFrameBufferFormat (mConfig.fOutputChannel, mConfig.fPixelFormat);
	mDevice.SetVideoFormat (mVideoFormat, /*retail*/false,  /*keepVanc*/false, mConfig.fOutputChannel);
	mDevice.SetVANCMode (NTV2_VANCMODE_OFF, mConfig.fOutputChannel);
	NTV2RasterInfo rasterInfo (mVideoFormat, mConfig.fPixelFormat);
	if (!hostBuffer.Allocate(rasterInfo.GetTotalRasterBytes(), /*pageAligned*/true))
		{cerr << "## ERROR:  Failed to allocate " << rasterInfo.GetTotalRasterBytes() << "-byte vid buffer" << endl;	return;}

	mDevice.AutoCirculateStop(mConfig.fOutputChannel);
	if (mDevice.AutoCirculateInitForOutput(mConfig.fOutputChannel, 2) && mDevice.AutoCirculateGetStatus(mConfig.fOutputChannel, acStatus))	//	Find out which buffers we got
		fbNum = ULWord(acStatus.acStartFrame);	//	Use them
	else
		{cerr << "## NOTE:  Allocate 2-frame AC" << DEC(mConfig.fOutputChannel+1) << " range failed" << endl;	return;}

	OVRLNOTE(DEC(threadTally) << " started, " << ::NTV2VideoFormatToString(mVideoFormat) << " raster:" << rasterInfo << ", bug:" << mBugRasterInfo);
	Bouncer<ULWord> mixPct (/*max*/400, /*min*/100, /*start*/100),
					xPos (/*max*/rasterInfo.GetRasterWidth() - mBugRasterInfo.GetRasterWidth()),
					yPos (/*max*/rasterInfo.GetVisibleRasterHeight() - mBugRasterInfo.GetVisibleRasterHeight() - 1);

	//	Loop til Quit or my AC channel stops...
	while (!mGlobalQuit)
	{
		//	Input thread will signal us to terminate by stopping A/C...
		if (mDevice.AutoCirculateGetStatus(mConfig.fInputChannel, acStatus)  &&  acStatus.IsStopped())
			break;	//	Stopped, exit thread

		//	Wait for the next input vertical interrupt event to get signaled...
		if (mDevice.WaitForInputVerticalInterrupt(mConfig.fInputChannel))
		{
			++goodWaits;
			//	Clear host buffer, blit the "bug" into it, then transfer it to device...
			hostBuffer.Fill(ULWord(0));
			if (::CopyRaster (mConfig.fPixelFormat,
							hostBuffer,									//	dstBuffer
							rasterInfo.GetBytesPerRow(),				//	dstBytesPerLine
							rasterInfo.GetVisibleRasterHeight(),		//	dstTotalLines
							yPos.Next(),								//	dstVertLineOffset
							xPos.Next(),								//	dstHorzPixelOffset
							mBug,										//	srcBuffer
							mBugRasterInfo.GetBytesPerRow(),			//	srcBytesPerLine
							mBugRasterInfo.GetVisibleRasterHeight(),	//	srcTotalLines
							0,											//	srcVertLineOffset
							mBugRasterInfo.GetVisibleRasterHeight(),	//	srcVertLinesToCopy
							0,											//	srcHorzPixelOffset
							mBugRasterInfo.GetRasterWidth()))			//	srcHorzPixelsToCopy
				++goodBlits;
			else
				++badBlits;
			if (mDevice.DMAWriteFrame (fbNum + pingPong, hostBuffer, hostBuffer.GetByteCount()))
				++goodXfers;
			else
				++badXfers;
			mDevice.SetOutputFrame (mConfig.fOutputChannel, fbNum + pingPong);
			pingPong = pingPong ? 0 : 1;
			mDevice.SetMixerCoefficient (MixerNum(), mixPct.Next() * 0x10000 / 400);
		}
		else
			++badWaits;
	}	//	loop til quit or A/C stopped

	mDevice.AutoCirculateStop(mConfig.fOutputChannel);
	mDevice.RemoveConnections(mOutputConnections);

	AJAAtomic::Increment(&gPlayExitCount);
	OVRLNOTE(DEC(threadTally) << " completed: " << DEC(goodXfers) << " xfers (" << DEC(badXfers) << " failed), "
			<< DEC(goodBlits) << " blits (" << DEC(badBlits) << " failed), " << DEC(goodWaits) << " waits (" << DEC(badWaits) << " failed)");
	NTV2_ASSERT(gPlayEnterCount == gPlayExitCount);

}	//	OutputThread


//////////////////////////////////////////////



//////////////////////////////////////////////
//
//	This is where the input thread gets started
//
void NTV2Overlay::StartInputThread (void)
{
	//	Create and start the capture thread...
	mCaptureThread.Attach(InputThreadStatic, this);
	mCaptureThread.SetPriority(AJA_ThreadPriority_High);
	mCaptureThread.Start();

}	//	StartInputThread


//
//	The static input thread function
//
void NTV2Overlay::InputThreadStatic (AJAThread * pThread, void * pContext)		//	static
{	(void) pThread;
	//	Grab the NTV2Overlay instance pointer from the pContext parameter,
	//	then call its InputThread method...
	NTV2Overlay * pApp (reinterpret_cast<NTV2Overlay*>(pContext));
	pApp->InputThread();
}	//	InputThreadStatic


//
//	The input thread:
//		-	waits indefinitely for a stable input signal, and if one is found:
//			-	configures an input FrameStore (if needed for capturing frames to host)
//			-	initializes AutoCirculate to reserve a couple of device frame buffers
//			-	starts a new output thread
//			-	waits indefinitely to see if the input signal is lost or changes, and if it does:
//				-	stops input AutoCirculate which signals the output thread to exit
//		-	upon termination:
//			-	ensures my monitor/capture AutoCirculate channel is stopped
//
void NTV2Overlay::InputThread (void)
{
	ULWord	vfChgTally(0), badWaits(0), waitTally(0);
	OVRLNOTE("Started");

	//	Loop until time to quit...
	while (!mGlobalQuit)
	{
		mVideoFormat = WaitForStableInputSignal();
		if (mVideoFormat == NTV2_FORMAT_UNKNOWN)
			break;	//	Quit

		//	At this point, the input video format is stable.

		//	Configure the input FrameStore...
		mDevice.EnableChannel(mConfig.fInputChannel);
		mDevice.SetMode (mConfig.fInputChannel, NTV2_MODE_CAPTURE);
		mDevice.SetFrameBufferFormat (mConfig.fInputChannel, NTV2_FBF_8BIT_YCBCR);
		mDevice.SetVideoFormat (mVideoFormat, /*retail*/false,  /*keepVanc*/false, mConfig.fInputChannel);
		mDevice.SetVANCMode (NTV2_VANCMODE_OFF, mConfig.fInputChannel);

		AUTOCIRCULATE_STATUS acStatus;
		ULWord fbNum (10);
		mDevice.AutoCirculateStop(mConfig.fInputChannel);
		if (mDevice.AutoCirculateInitForInput(mConfig.fInputChannel, 2) && mDevice.AutoCirculateGetStatus(mConfig.fInputChannel, acStatus))	//	Which buffers did we get?
			fbNum = ULWord(acStatus.acStartFrame);	//	Use them
		else
			{cerr << "## NOTE:  Input A/C allocate device frame buffer range failed" << endl;	return;}
		mDevice.SetInputFrame (mConfig.fInputChannel, fbNum);
		
		mDevice.AutoCirculateStart(mConfig.fInputChannel);

		StartOutputThread();	//	Start a new playout thread

		//	Spin until signal format changes...
		while (!mGlobalQuit)
		{
			//	Wait for the next input vertical interrupt event to get signaled...
			if (mDevice.WaitForInputVerticalInterrupt(mConfig.fInputChannel))
				++waitTally;
			else
				++badWaits;
			
			mDevice.AutoCirculateTransfer(mConfig.fInputChannel, );
			
		}	//	inner loop -- until signal change
	}	//	loop til quit signaled
	mDevice.AutoCirculateStop(mConfig.fInputChannel);
	OVRLNOTE("Completed: " << DEC(vfChgTally) << " signal changes, " << DEC(waitTally) << " good waits, " << DEC(badWaits) << " bad waits");

}	//	InputThread

