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

static const uint32_t	kAppSignature	(NTV2_FOURCC('O','v','r','l'));

//////////////////////	IMPLEMENTATION


NTV2Overlay::NTV2Overlay (const OverlayConfig & inConfig)
	:	mConfig			(inConfig),
		mPlayThread		(AJAThread()),
		mCaptureThread	(AJAThread()),
		mVideoFormat	(NTV2_FORMAT_UNKNOWN),
		mSavedTaskMode	(NTV2_DISABLE_TASKS),
		mInputPixFormat	(inConfig.fPixelFormat),
		mOutputPixFormat(NTV2_FBF_ARGB),			//	fixed to 8-bit ARGB
		mGlobalQuit		(false)
{
	mAVCircularBuffer.SetAbortFlag(&mGlobalQuit);	//	Ring buffer abandons waits when mGlobalQuit goes true
}	//	constructor


NTV2Overlay::~NTV2Overlay ()
{
	Quit();	//	Stop my capture and playout threads, then destroy them
	mDevice.UnsubscribeInputVerticalEvent(NTV2_CHANNEL1);
	mDevice.UnsubscribeOutputVerticalEvent(NTV2_CHANNEL1);
	mDevice.UnsubscribeOutputVerticalEvent(NTV2_CHANNEL2);
	ReleaseCaptureBuffers();
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
		{cerr << "## ERROR:  Device '" << mDevice.GetDescription() << "' not ready" << endl;  return AJA_STATUS_INITIALIZE;}
    if (mDevice.GetDeviceID() != DEVICE_ID_KONAX)
		cerr << "## WARNING:  Device '" << mDevice.GetDescription() << "' is not KONA X" << endl;

	ULWord	appSig(0);
	int32_t	appPID(0);
	mDevice.GetStreamingApplication (appSig, appPID);	//	Who currently "owns" the device?
	mDevice.GetEveryFrameServices(mSavedTaskMode);		//	Save the current device state

	if (!mDevice.AcquireStreamForApplication (kAppSignature, int32_t(AJAProcess::GetPid())))
	{
		cerr << "## ERROR:  Cannot acquire '" << mDevice.GetDescription() << "' because another app (pid " << appPID << ") owns it" << endl;
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

	CAPINFO("Configuration: " << mConfig);  PLINFO("Configuration: " << mConfig);
	return AJA_STATUS_SUCCESS;
}	//	Init


AJAStatus NTV2Overlay::SetupVideo (void)
{
	//	Flip the input spigot to "receive" if necessary...
	mDevice.SetSDITransmitEnable (NTV2_CHANNEL1, false);
	mDevice.SetSDITransmitEnable (NTV2_CHANNEL2, true);

	mDevice.WaitForOutputVerticalInterrupt (NTV2_CHANNEL1, 10);	

	//	Since this demo runs in E-to-E mode (thru Mixer/Keyer), reference must be tied to the input...
	mDevice.SetReference(::NTV2InputSourceToReferenceSource(mConfig.fInputSource));

	mDevice.SetMultiFormatMode(true);

	mDevice.ClearRouting();
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

bool NTV2Overlay::IsInputSignalRGB (void)
{
	if (NTV2_INPUT_SOURCE_IS_HDMI(mConfig.fInputSource))
	{
		NTV2LHIHDMIColorSpace cs2;
		mDevice.GetHDMIInputColor (cs2, ::NTV2InputSourceToChannel(mConfig.fInputSource));
		return cs2 == NTV2_LHIHDMIColorSpaceRGB;
	}
	return false;
}

void NTV2Overlay::RouteInputSignal (void)
{
	const bool isCaptureBufferRGB(NTV2_IS_FBF_RGB(mConfig.fPixelFormat)), isSignalRGB(IsInputSignalRGB());
	NTV2OutputXptID oXptInput (::GetInputSourceOutputXpt (mConfig.fInputSource, /*DS2*/false, isSignalRGB));

	//	Connect input to Mixer1 background...
	if (isSignalRGB)
	{	//	Must convert to YUV...
		mDevice.Connect(NTV2_XptCSC1VidInput, oXptInput, false);	//	input => CSC1
		mDevice.Connect(NTV2_XptMixer1BGVidInput, NTV2_XptCSC1VidYUV, false);	//	CSC1Vid => Mix1BGVid
		mDevice.Connect(NTV2_XptMixer1BGKeyInput, NTV2_XptCSC1KeyYUV, false);	//	CSC1Key => Mix1BGKey
	}
	else
	{
		mDevice.Connect(NTV2_XptMixer1BGVidInput, oXptInput, false);		//	input => Mix1BGVid
		mDevice.Connect(NTV2_XptMixer1BGKeyInput, oXptInput, false);		//	input => Mix1BGKey
	}

	//	Connect input to FrameStore1...
	if (isCaptureBufferRGB  &&  !isSignalRGB)
	{
		mDevice.Connect(NTV2_XptCSC1VidInput, oXptInput, false);				//	input => CSC1
		mDevice.Connect(NTV2_XptFrameBuffer1Input, NTV2_XptCSC1VidRGB, false);	//	CSC1RGB => FS1
	}
	else if (!isCaptureBufferRGB && isSignalRGB)
		mDevice.Connect(NTV2_XptFrameBuffer1Input, NTV2_XptCSC1VidYUV, false);	//	CSC1YUV => FS1
	else	//	isCaptureBufferRGB == isSignalRGB
		mDevice.Connect(NTV2_XptFrameBuffer1Input, oXptInput, false);			//	input => FS1
}	//	RouteInputSignal

void NTV2Overlay::RouteOverlaySignal (void)
{
	//	Connect FrameStore2 RGB output through CSC2 to Mixer1 foreground...
	mDevice.Connect(NTV2_XptCSC2VidInput, NTV2_XptFrameBuffer2RGB, false);		//	FS2RGB => CSC2
	mDevice.Connect(NTV2_XptMixer1FGVidInput, NTV2_XptCSC2VidYUV, false);		//	CSC2Vid => Mix1FGVid
	mDevice.Connect(NTV2_XptMixer1FGKeyInput, NTV2_XptCSC2KeyYUV, false);		//	CSC2Key => Mix1FGKey
}	//	RouteOverlaySignal


void NTV2Overlay::RouteOutputSignal (void)
{
	//	Connect Mixer1 output to SDI & HDMI outputs...
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


AJAStatus NTV2Overlay::SetupCaptureBuffers (void)
{
	if (!NTV2_IS_VALID_VIDEO_FORMAT(mVideoFormat))
		return AJA_STATUS_NOINPUT;

	ReleaseCaptureBuffers();

	//	Allocate and add each NTV2FrameData to my circular buffer member variable...
	const ULWord captureBufferSize (::GetVideoWriteSize (mVideoFormat, mInputPixFormat));
	mBuffers.reserve(CIRCULAR_BUFFER_SIZE);
	while (mBuffers.size() < CIRCULAR_BUFFER_SIZE)
	{
		mBuffers.push_back(NTV2FrameData());		//	Make a new NTV2FrameData...
		NTV2FrameData & frameData(mBuffers.back());	//	...and get a reference to it
		//	Allocate a page-aligned video buffer
		if (!frameData.fVideoBuffer.Allocate(captureBufferSize, /*pageAlign?*/true))
			return AJA_STATUS_MEMORY;
		mAVCircularBuffer.Add(&frameData);	//	Add to my circular buffer
	}	//	for each NTV2FrameData
	return AJA_STATUS_SUCCESS;
}	//	SetupCaptureBuffers


void NTV2Overlay::ReleaseCaptureBuffers (void)
{
	mBuffers.clear();
	mAVCircularBuffer.Clear();
	return;
}	//	ReleaseCaptureBuffers

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
	static const ULWord sOpaqueRed(0xFFFF0000), sOpaqueBlue(0xFF0000FF), sOpaqueGreen(0xFF008000),
						sOpaqueWhite(0xFFFFFFFF), sOpaqueBlack(0xFF000000);

	//	The overlay "bug" is a 256x256 pixel raster image composed of four opaque rectangles of
	//	varying color and diminishing size, each set inside each other, centered in the raster.
	//	All space between them is transparent to reveal the background video. The line thickness
	//	of each rectangle is the same:  1/16th the width or height of the raster. The pixel format
	//	is NTV2_FBF_ARGB, which all devices handle, and easy to traverse at 4-bytes-per-pixel.
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
			mDevice.WaitForOutputVerticalInterrupt(NTV2_CHANNEL1);	//	Just delay til next output VBI
			if (mGlobalQuit)
				return NTV2_FORMAT_UNKNOWN;	//	Terminate if asked to do so

			const NTV2VideoFormat currVF (mDevice.GetInputVideoFormat(mConfig.fInputSource));
			if (currVF == NTV2_FORMAT_UNKNOWN)
			{	//	Wait for video signal to appear
				if (++loopCount % 500 == 0)	//	Log message every minute or so at ~50ms
					CAPDBG("Waiting for valid video signal to appear at " << ::NTV2InputSourceToString(mConfig.fInputSource,true));
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

		CAPNOTE(::NTV2InputSourceToString(mConfig.fInputSource,true) << " video format: "
				<< ::NTV2VideoFormatToString(result));
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

//	This is where the play thread is started
void NTV2Overlay::StartOutputThread (void)
{
	//	Create and start the playout thread...
	mPlayThread.Attach(OutputThreadStatic, this);
	mPlayThread.SetPriority(AJA_ThreadPriority_High);
	mPlayThread.Start();
}	//	StartOutputThread

//	The static output thread function
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
	ULWord thrdNum(gPlayEnterCount), fbNum(10), loops(0);
	ULWord goodWaits(0), badWaits(0), goodBlits(0), badBlits(0), goodXfers(0), badXfers(0), pingPong(0);
	AUTOCIRCULATE_STATUS acStatus;
	NTV2Buffer hostBuffer;

	//	Configure the output FrameStore...
	mDevice.SetMode (mConfig.fOutputChannel, NTV2_MODE_DISPLAY);
	mDevice.SetFrameBufferFormat (mConfig.fOutputChannel, mOutputPixFormat);
	mDevice.SetVideoFormat (mVideoFormat, /*retail*/false,  /*keepVanc*/false, mConfig.fOutputChannel);
	mDevice.SetVANCMode (NTV2_VANCMODE_OFF, mConfig.fOutputChannel);
	NTV2RasterInfo rasterInfo (mVideoFormat, mOutputPixFormat);
	if (!hostBuffer.Allocate(rasterInfo.GetTotalRasterBytes(), /*pageAligned*/true))
		{cerr << "## ERROR:  Failed to allocate " << rasterInfo.GetTotalRasterBytes() << "-byte vid buffer" << endl;	return;}

	mDevice.AutoCirculateStop(mConfig.fOutputChannel);
	if (mDevice.AutoCirculateInitForOutput(mConfig.fOutputChannel, 2) && mDevice.AutoCirculateGetStatus(mConfig.fOutputChannel, acStatus))	//	Find out which buffers we got
		fbNum = ULWord(acStatus.acStartFrame);	//	Use them
	else
		{cerr << "## NOTE:  Allocate 2-frame AC" << DEC(mConfig.fOutputChannel+1) << " range failed" << endl;	return;}

	PLNOTE(DEC(thrdNum) << " started, " << ::NTV2VideoFormatToString(mVideoFormat)
			<< " raster:" << rasterInfo << ", bug:" << mBugRasterInfo);
	Bouncer<ULWord> mixPct (/*max*/400, /*min*/100, /*start*/100),
					xPos (/*max*/rasterInfo.GetRasterWidth() - mBugRasterInfo.GetRasterWidth()),
					yPos (/*max*/rasterInfo.GetVisibleRasterHeight() - mBugRasterInfo.GetVisibleRasterHeight() - 1);

	//	Loop til Quit or my AC channel stops...
	while (!mGlobalQuit)
	{
		//	Terminate this output thread when video format changes...
		if (mDevice.GetInputVideoFormat(mConfig.fInputSource) != mVideoFormat)
			break;	//	Stopped, exit thread

		NTV2FrameData *	pFrameData (mAVCircularBuffer.StartConsumeNextBuffer());
		if (pFrameData)
		{
			//	Consume captured frame ... it's simply dropped on the floor.
			//	(The frame can't be used here anyway, since it's NTV2_FBF_10BIT_YCBCR,
			//	and the output pixel format is NTV2_FBF_ARGB.)
			mAVCircularBuffer.EndConsumeNextBuffer();	//	Signal "done with this frame"
		}

		//	Wait for the next output vertical interrupt event to get signaled...
		if (mDevice.WaitForOutputVerticalInterrupt(mConfig.fOutputChannel))
		{
			++goodWaits;
			//	Clear host buffer, blit the "bug" into it, then transfer it to device...
			hostBuffer.Fill(ULWord(0));
			if (::CopyRaster (mOutputPixFormat,							//	src & dst pixel format
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

		if (++loops % 500 == 0)	//	Log message every minute or so
			PLDBG(DEC(thrdNum) << ": " << DEC(goodXfers) << " xfers (" << DEC(badXfers) << " failed), " << DEC(goodBlits)
					<< " blits (" << DEC(badBlits) << " failed), " << DEC(goodWaits) << " waits ("
					<< DEC(badWaits) << " failed)");
	}	//	loop til quit or A/C stopped

	mDevice.AutoCirculateStop(mConfig.fOutputChannel);
	AJAAtomic::Increment(&gPlayExitCount);
	PLNOTE(DEC(thrdNum) << " done: " << DEC(goodXfers) << " xfers (" << DEC(badXfers) << " failed), "
			<< DEC(goodBlits) << " blits (" << DEC(badBlits) << " failed), " << DEC(goodWaits) << " waits (" << DEC(badWaits) << " failed)");
	NTV2_ASSERT(gPlayEnterCount == gPlayExitCount);

}	//	OutputThread


//////////////////////////////////////////////


//	This is where the input thread gets started
void NTV2Overlay::StartInputThread (void)
{
	//	Create and start the capture thread...
	mCaptureThread.Attach(InputThreadStatic, this);
	mCaptureThread.SetPriority(AJA_ThreadPriority_High);
	mCaptureThread.Start();
}	//	StartInputThread

//	The static input thread function
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
	ULWord	loops(0), bails(0), vfChgTally(0), badWaits(0), waitTally(0), goodXfers(0), badXfers(0);
	AUTOCIRCULATE_TRANSFER xferInfo;
	CAPNOTE("Started");

	//	Loop until time to quit...
	while (!mGlobalQuit)
	{
		mDevice.AutoCirculateStop(mConfig.fInputChannel);
		mVideoFormat = WaitForStableInputSignal();
		if (mVideoFormat == NTV2_FORMAT_UNKNOWN)
			break;	//	Quit

		//	At this point, the input video format is stable.
		//	Configure the input FrameStore...
		const bool isRGBSignal (IsInputSignalRGB());
		mDevice.SetMode (mConfig.fInputChannel, NTV2_MODE_CAPTURE);
		mDevice.SetFrameBufferFormat (mConfig.fInputChannel, mInputPixFormat);
		mDevice.SetVideoFormat (mVideoFormat, /*retail*/false,  /*keepVanc*/false, mConfig.fInputChannel);
		mDevice.SetVANCMode (NTV2_VANCMODE_OFF, mConfig.fInputChannel);
		RouteInputSignal();
		if (AJA_FAILURE(SetupCaptureBuffers()))
		{	cerr << "## NOTE:  Failed to [re]allocate host buffers" << endl;
			mGlobalQuit = true;
			continue;
		}

		AUTOCIRCULATE_STATUS acStatus;
		if (!mDevice.AutoCirculateInitForInput(mConfig.fInputChannel, 7))
		{	cerr << "## NOTE:  Input A/C allocate device frame buffer range failed" << endl;
			mGlobalQuit = true;
			continue;
		}
		
		mDevice.AutoCirculateStart(mConfig.fInputChannel);	//	Start capturing
		StartOutputThread();								//	Start a new playout thread

		//	Capture frames until signal format changes...
		while (!mGlobalQuit)
		{
			AUTOCIRCULATE_STATUS acStatus;
			mDevice.AutoCirculateGetStatus (mConfig.fInputChannel, acStatus);
			if (acStatus.IsRunning()  &&  acStatus.HasAvailableInputFrame())
			{
				//	At least one fully-formed frame is available in device frame buffer
				//	memory that can be transferred to the host. Reserve an NTV2FrameData
				//	to "produce", and use it to store the next frame to be transferred...
				NTV2FrameData *	pCaptureData (mAVCircularBuffer.StartProduceNextBuffer());
				if (pCaptureData)
				{	//	Transfer frame from device...
					xferInfo.SetVideoBuffer (pCaptureData->VideoBuffer(), pCaptureData->VideoBufferSize());
					if (mDevice.AutoCirculateTransfer (mConfig.fInputChannel, xferInfo))  ++goodXfers;
					else  ++badXfers;
					mAVCircularBuffer.EndProduceNextBuffer();	//	Signal "done with this frame"
				}	//	if pCaptureData != NULL
				else ++bails;
			}	//	if A/C running and frame(s) are available for transfer
			else
			{	//	Wait for next input VBI...
				if (mDevice.WaitForInputVerticalInterrupt(mConfig.fInputChannel))  ++waitTally;
				else  ++badWaits;
			}
			if (++loops % 500 == 0)	//	Log message every minute or so
				CAPDBG(DEC(vfChgTally) << " sigChgs, " << DEC(bails) << " bails, " << DEC(waitTally)
						<< " waits (" << DEC(badWaits) << " failed), " << DEC(goodXfers) << " xfers ("
						<< DEC(badXfers) << " failed)");

			//	Check input signal format...
			if (mDevice.GetInputVideoFormat(mConfig.fInputSource) != mVideoFormat
				||	isRGBSignal != IsInputSignalRGB())
			{	//	Input signal format changed!
				++vfChgTally;
				mAVCircularBuffer.StartProduceNextBuffer();	//	Unblock output thread
				mAVCircularBuffer.EndProduceNextBuffer();
				mDevice.WaitForOutputVerticalInterrupt(mConfig.fOutputChannel);
				break;	//	Break to outer loop to wait for stable input signal
			}
		}	//	inner loop -- until signal change
	}	//	loop til quit signaled

	mDevice.AutoCirculateStop(mConfig.fInputChannel);	//	This will signal OutputThread to exit
	CAPNOTE("Done: " << DEC(vfChgTally) << " sigChgs, " << DEC(bails) << " bails, " << DEC(waitTally)
			<< " waits (" << DEC(badWaits) << " failed), " << DEC(goodXfers) << " xfers ("
			<< DEC(badXfers) << " failed)");
}	//	InputThread
