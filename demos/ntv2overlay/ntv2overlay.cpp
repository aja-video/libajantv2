/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2overlay.cpp
	@brief		Implementation of NTV2Overlay demonstration class.
	@copyright	(C) 2012-2022 AJA Video Systems, Inc.  All rights reserved.
**/

#include "ntv2overlay.h"
#include "ntv2devicescanner.h"
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
	mDevice.UnsubscribeInputVerticalEvent(NTV2_CHANNEL1);
	mDevice.UnsubscribeOutputVerticalEvent(NTV2_CHANNEL1);
	mDevice.UnsubscribeOutputVerticalEvent(NTV2_CHANNEL2);
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
	
	mDevice.SetEveryFrameServices(NTV2_OEM_TASKS);	//	Force OEM tasks
	
	mDevice.EnableChannel(NTV2_CHANNEL1);
	mDevice.EnableChannel(NTV2_CHANNEL2);
	
	//	Enable/subscribe interrupts...
	mConfig.fInputChannel = NTV2_CHANNEL1;
	mConfig.fOutputChannel = NTV2_CHANNEL2;
	mDevice.SubscribeInputVerticalEvent(NTV2_CHANNEL1);
	mDevice.SubscribeOutputVerticalEvent(NTV2_CHANNEL1);
	mDevice.SubscribeOutputVerticalEvent(NTV2_CHANNEL2);

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
	cerr << mConfig;
	if (mDevice.IsRemote())
		cerr	<< "Device Description:  " << mDevice.GetDescription() << endl;
	cerr << endl;

	OVRLINFO("Configuration: " << mConfig);
	return AJA_STATUS_SUCCESS;

}	//	Init


AJAStatus NTV2Overlay::SetupVideo (void)
{
	//	Flip the input spigot to "receive" if necessary...
	mDevice.SetSDITransmitEnable (NTV2_CHANNEL1, false);
	mDevice.SetSDITransmitEnable (NTV2_CHANNEL2, true);
	
	mDevice.WaitForOutputVerticalInterrupt (NTV2_CHANNEL1, 10);	

	//	Since this demo runs in E-to-E mode (thru Mixer/Keyer), reference must be tied to the input...
	mDevice.SetReference(NTV2_REFERENCE_INPUT1);

	mDevice.SetMultiFormatMode(true);
	
	mDevice.ClearRouting();
	RouteInputSignal();
	RouteOverlaySignal();
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

void NTV2Overlay::RouteInputSignal (void)
{
	mDevice.Connect(NTV2_XptFrameBuffer1Input, NTV2_XptSDIIn1, false);
	mDevice.Connect(NTV2_XptMixer1BGVidInput, NTV2_XptSDIIn1, false);
	mDevice.Connect(NTV2_XptMixer1BGKeyInput, NTV2_XptSDIIn1, false);
}	//	RouteInputSignal

void NTV2Overlay::RouteOverlaySignal (void)
{
	mDevice.Connect(NTV2_XptCSC2VidInput, NTV2_XptFrameBuffer2RGB, false);
	mDevice.Connect(NTV2_XptMixer1FGVidInput, NTV2_XptCSC2VidYUV, false);
	mDevice.Connect(NTV2_XptMixer1FGKeyInput, NTV2_XptCSC2KeyYUV, false);
}	//	RouteOverlaySignal


void NTV2Overlay::RouteOutputSignal (void)
{
	mDevice.Connect(NTV2_XptSDIOut2Input, NTV2_XptMixer1VidYUV, false);
	mDevice.Connect(NTV2_XptHDMIOutInput, NTV2_XptMixer1VidYUV, false);
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
		mDevice.SetMode (mConfig.fInputChannel, NTV2_MODE_CAPTURE);
		mDevice.SetFrameBufferFormat (mConfig.fInputChannel, NTV2_FBF_10BIT_YCBCR);
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
		}	//	inner loop -- until signal change
	}	//	loop til quit signaled
	mDevice.AutoCirculateStop(mConfig.fInputChannel);
	OVRLNOTE("Completed: " << DEC(vfChgTally) << " signal changes, " << DEC(waitTally) << " good waits, " << DEC(badWaits) << " bad waits");

}	//	InputThread

