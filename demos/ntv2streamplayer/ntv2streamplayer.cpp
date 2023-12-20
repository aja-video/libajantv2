/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2streamplayer.cpp
	@brief		Implementation of NTV2StreamPlayer class.
	@copyright	(C) 2013-2022 AJA Video Systems, Inc.  All rights reserved.
**/

#include "ntv2streamplayer.h"
#include "ntv2debug.h"
#include "ntv2devicescanner.h"
#include "ntv2testpatterngen.h"
#include "ajabase/common/timebase.h"
#include "ajabase/system/process.h"
#include "ajaanc/includes/ancillarydata_hdr_sdr.h"
#include "ajaanc/includes/ancillarydata_hdr_hdr10.h"
#include "ajaanc/includes/ancillarydata_hdr_hlg.h"
#include <fstream>	//	For ifstream

using namespace std;

//	Convenience macros for EZ logging:
#define	TCFAIL(_expr_)	AJA_sERROR  (AJA_DebugUnit_TimecodeGeneric, AJAFUNC << ": " << _expr_)
#define	TCWARN(_expr_)	AJA_sWARNING(AJA_DebugUnit_TimecodeGeneric, AJAFUNC << ": " << _expr_)
#define	TCNOTE(_expr_)	AJA_sNOTICE	(AJA_DebugUnit_TimecodeGeneric, AJAFUNC << ": " << _expr_)
#define	TCINFO(_expr_)	AJA_sINFO	(AJA_DebugUnit_TimecodeGeneric, AJAFUNC << ": " << _expr_)
#define	TCDBG(_expr_)	AJA_sDEBUG	(AJA_DebugUnit_TimecodeGeneric, AJAFUNC << ": " << _expr_)

static const bool		BUFFER_PAGE_ALIGNED	(true);


NTV2StreamPlayer::NTV2StreamPlayer (const PlayerConfig & inConfig)
	:	mConfig				(inConfig),
		mConsumerThread		(),
		mProducerThread		(),
		mDevice				(),
		mDeviceID			(DEVICE_ID_INVALID),
		mSavedTaskMode		(NTV2_TASK_MODE_INVALID),
		mCurrentFrame		(0),
		mFormatDesc			(),
		mGlobalQuit			(false),
		mTCBurner			(),
		mHostBuffers		(),
		mTestPatRasters		()
{
}


NTV2StreamPlayer::~NTV2StreamPlayer (void)
{
	//	Stop my playout and producer threads, then destroy them...
	Quit();
}	//	destructor


void NTV2StreamPlayer::Quit (void)
{
	//	Set the global 'quit' flag, and wait for the threads to go inactive...
	mGlobalQuit = true;

	while (mProducerThread.Active())
		AJATime::Sleep(10);

	while (mConsumerThread.Active())
		AJATime::Sleep(10);

	if (!mConfig.fDoMultiFormat  &&  mDevice.IsOpen())
	{
		mDevice.ReleaseStreamForApplication (kDemoAppSignature, int32_t(AJAProcess::GetPid()));
		if (NTV2_IS_VALID_TASK_MODE(mSavedTaskMode))
			mDevice.SetEveryFrameServices(mSavedTaskMode);		//	Restore prior task mode
	}
}	//	Quit


AJAStatus NTV2StreamPlayer::Init (void)
{
	AJAStatus	status	(AJA_STATUS_SUCCESS);

	//	Open the device...
	if (!CNTV2DeviceScanner::GetFirstDeviceFromArgument (mConfig.fDeviceSpec, mDevice))
		{cerr << "## ERROR:  Device '" << mConfig.fDeviceSpec << "' not found" << endl;  return AJA_STATUS_OPEN;}
	mDeviceID = mDevice.GetDeviceID();	//	Keep this ID handy -- it's used frequently

    if (!mDevice.IsDeviceReady(false))
		{cerr << "## ERROR:  Device '" << mDevice.GetDisplayName() << "' not ready" << endl;  return AJA_STATUS_INITIALIZE;}
	if (!mDevice.features().CanDoPlayback())
		{cerr << "## ERROR:  '" << mDevice.GetDisplayName() << "' is capture-only" << endl;  return AJA_STATUS_FEATURE;}

	const UWord maxNumChannels (mDevice.features().GetNumFrameStores());

	//	Beware -- some older devices (e.g. Corvid1) can only output from FrameStore 2...
	if ((mConfig.fOutputChannel == NTV2_CHANNEL1) && (!mDevice.features().CanDoFrameStore1Display()))
		mConfig.fOutputChannel = NTV2_CHANNEL2;
	if (UWord(mConfig.fOutputChannel) >= maxNumChannels)
	{
		cerr	<< "## ERROR:  '" << mDevice.GetDisplayName() << "' can't use Ch" << DEC(mConfig.fOutputChannel+1)
				<< " -- only supports Ch1" << (maxNumChannels > 1  ?  string("-Ch") + string(1, char(maxNumChannels+'0'))  :  "") << endl;
		return AJA_STATUS_UNSUPPORTED;
	}

	if (!mConfig.fDoMultiFormat)
	{
		mDevice.GetEveryFrameServices(mSavedTaskMode);		//	Save the current task mode
		if (!mDevice.AcquireStreamForApplication (kDemoAppSignature, int32_t(AJAProcess::GetPid())))
			return AJA_STATUS_BUSY;		//	Device is in use by another app -- fail
	}
	mDevice.SetEveryFrameServices(NTV2_OEM_TASKS);			//	Set OEM service level

	if (mDevice.features().CanDoMultiFormat())
		mDevice.SetMultiFormatMode(mConfig.fDoMultiFormat);
	else
		mConfig.fDoMultiFormat = false;

	//	Set up the video and audio...
	status = SetUpVideo();
	if (AJA_FAILURE(status))
		return status;

	//	Set up the circular buffers, and the test pattern buffers...
	status = SetUpHostBuffers();
	if (AJA_FAILURE(status))
		return status;
	status = SetUpTestPatternBuffers();
	if (AJA_FAILURE(status))
		return status;

	//	Set up the device signal routing...
	if (!RouteOutputSignal())
		return AJA_STATUS_FAIL;

	//	Lastly, prepare my AJATimeCodeBurn instance...
	if (!mTCBurner.RenderTimeCodeFont (CNTV2DemoCommon::GetAJAPixelFormat(mConfig.fPixelFormat), mFormatDesc.numPixels, mFormatDesc.numLines))
		{cerr << "## ERROR:  RenderTimeCodeFont failed for:  " << mFormatDesc << endl;  return AJA_STATUS_UNSUPPORTED;}

	//	Ready to go...
	#if defined(_DEBUG)
		cerr << mConfig << endl;
	#endif	//	defined(_DEBUG)
	return AJA_STATUS_SUCCESS;

}	//	Init


AJAStatus NTV2StreamPlayer::SetUpVideo (void)
{
	//	Configure the device to output the requested video format...
 	if (mConfig.fVideoFormat == NTV2_FORMAT_UNKNOWN)
		return AJA_STATUS_BAD_PARAM;
	if (!mDevice.features().CanDoVideoFormat(mConfig.fVideoFormat))
	{	cerr	<< "## ERROR:  '" << mDevice.GetDisplayName() << "' doesn't support "
				<< ::NTV2VideoFormatToString(mConfig.fVideoFormat) << endl;
		return AJA_STATUS_UNSUPPORTED;
	}
	if (!mDevice.features().CanDoFrameBufferFormat(mConfig.fPixelFormat))
	{	cerr	<< "## ERROR: '" << mDevice.GetDisplayName() << "' doesn't support "
				<< ::NTV2FrameBufferFormatString(mConfig.fPixelFormat) << endl;
		return AJA_STATUS_UNSUPPORTED;
	}

	//	This demo doesn't playout dual-link RGB over SDI -- only YCbCr.
	//	Check that this device has a CSC to convert RGB to YUV...
	if (::IsRGBFormat(mConfig.fPixelFormat))	//	If RGB FBF...
		if (UWord(mConfig.fOutputChannel) > mDevice.features().GetNumCSCs())	//	No CSC for this channel?
			{cerr << "## ERROR: No CSC for channel " << DEC(mConfig.fOutputChannel+1) << " to convert RGB pixel format" << endl;
				return AJA_STATUS_UNSUPPORTED;}

	if (!mDevice.features().CanDo3GLevelConversion() && mConfig.fDoABConversion && ::IsVideoFormatA(mConfig.fVideoFormat))
		mConfig.fDoABConversion = false;
	if (mConfig.fDoABConversion)
		mDevice.SetSDIOutLevelAtoLevelBConversion (mConfig.fOutputChannel, mConfig.fDoABConversion);

	//	Keep the raster description handy...
	mFormatDesc = NTV2FormatDescriptor(mConfig.fVideoFormat, mConfig.fPixelFormat);
	if (!mFormatDesc.IsValid())
		return AJA_STATUS_FAIL;

	//	Turn on the FrameStore (to read frame buffer memory and transmit video)...
	mDevice.EnableChannel(mConfig.fOutputChannel);
	mDevice.SetMode(mConfig.fOutputChannel, NTV2_MODE_DISPLAY);

	//	This demo assumes VANC is disabled...
	mDevice.SetVANCMode(NTV2_VANCMODE_OFF, mConfig.fOutputChannel);
	mDevice.SetVANCShiftMode(mConfig.fOutputChannel, NTV2_VANCDATA_NORMAL);

	//	Set the FrameStore video format...
	mDevice.SetVideoFormat (mConfig.fVideoFormat, false, false, mConfig.fOutputChannel);
//	mDevice.SetTsiFrameEnable(true, mConfig.fOutputChannel);

	//	Set the frame buffer pixel format for the device FrameStore...
	mDevice.SetFrameBufferFormat (mConfig.fOutputChannel, mConfig.fPixelFormat);

	//	Check if HDR anc is permissible...
	if (IS_KNOWN_AJAAncDataType(mConfig.fTransmitHDRType)  &&  !mDevice.features().CanDoCustomAnc())
		{cerr << "## WARNING:  HDR Anc requested, but device can't do custom anc" << endl;
			mConfig.fTransmitHDRType = AJAAncDataType_Unknown;}

	//	Set output clock reference...
	mDevice.SetReference(mDevice.features().CanDo2110() ? NTV2_REFERENCE_SFP1_PTP : NTV2_REFERENCE_FREERUN);

	//	At this point, video setup is complete (except for widget signal routing).
	return AJA_STATUS_SUCCESS;

}	//	SetUpVideo


AJAStatus NTV2StreamPlayer::SetUpHostBuffers (void)
{
	CNTV2DemoCommon::SetDefaultPageSize();	//	Set host-specific page size

	//	Allocate and add each in-host NTV2FrameData to my circular buffer member variable...
	mHostBuffers.reserve(CIRCULAR_BUFFER_SIZE);
	while (mHostBuffers.size() < CIRCULAR_BUFFER_SIZE)
	{
		mHostBuffers.push_back(FrameData());		//	Make a new NTV2FrameData...
		FrameData & frameData(mHostBuffers.back());	//	...and get a reference to it

		//	Allocate a page-aligned video buffer
		if (mConfig.WithVideo())
		{
			if (!frameData.fVideoBuffer.Allocate (mFormatDesc.GetTotalBytes(), BUFFER_PAGE_ALIGNED))
			{
				PLFAIL("Failed to allocate " << xHEX0N(mFormatDesc.GetTotalBytes(),8) << "-byte video buffer");
				return AJA_STATUS_MEMORY;
			}
		}
		frameData.fDataReady = false;
	}	//	for each NTV2FrameData

	mProducerIndex = 0;
	mConsumerIndex = 0;

	return AJA_STATUS_SUCCESS;

}	//	SetUpHostBuffers


AJAStatus NTV2StreamPlayer::SetUpTestPatternBuffers (void)
{
	vector<NTV2TestPatternSelect>	testPatIDs;
		testPatIDs.push_back(NTV2_TestPatt_ColorBars100);
		testPatIDs.push_back(NTV2_TestPatt_ColorBars75);
		testPatIDs.push_back(NTV2_TestPatt_Ramp);
		testPatIDs.push_back(NTV2_TestPatt_MultiBurst);
		testPatIDs.push_back(NTV2_TestPatt_LineSweep);
		testPatIDs.push_back(NTV2_TestPatt_CheckField);
		testPatIDs.push_back(NTV2_TestPatt_FlatField);
		testPatIDs.push_back(NTV2_TestPatt_MultiPattern);
		testPatIDs.push_back(NTV2_TestPatt_Border);

	mTestPatRasters.clear();
	for (size_t tpNdx(0);  tpNdx < testPatIDs.size();  tpNdx++)
		mTestPatRasters.push_back(NTV2Buffer());

	if (!mFormatDesc.IsValid())
		{PLFAIL("Bad format descriptor");  return AJA_STATUS_FAIL;}
	if (mFormatDesc.IsVANC())
		{PLFAIL("VANC should have been disabled: " << mFormatDesc);  return AJA_STATUS_FAIL;}

	//	Set up one video buffer for each test pattern...
	for (size_t tpNdx(0);  tpNdx < testPatIDs.size();  tpNdx++)
	{
		//	Allocate the buffer memory...
		if (!mTestPatRasters.at(tpNdx).Allocate (mFormatDesc.GetTotalBytes(), BUFFER_PAGE_ALIGNED))
		{	PLFAIL("Test pattern buffer " << DEC(tpNdx+1) << " of " << DEC(testPatIDs.size()) << ": "
					<< xHEX0N(mFormatDesc.GetTotalBytes(),8) << "-byte page-aligned alloc failed");
			return AJA_STATUS_MEMORY;
		}

		//	Fill the buffer with test pattern...
		NTV2TestPatternGen	testPatternGen;
		if (!testPatternGen.DrawTestPattern (testPatIDs.at(tpNdx),  mFormatDesc,  mTestPatRasters.at(tpNdx)))
		{
			cerr << "## ERROR:  DrawTestPattern " << DEC(tpNdx) << " failed: " << mFormatDesc << endl;
			return AJA_STATUS_FAIL;
		}
	}	//	loop for each predefined pattern

	return AJA_STATUS_SUCCESS;

}	//	SetUpTestPatternBuffers


bool NTV2StreamPlayer::RouteOutputSignal (void)
{
	const NTV2Standard	outputStandard	(::GetNTV2StandardFromVideoFormat(mConfig.fVideoFormat));
	const UWord			numSDIOutputs	(mDevice.features().GetNumVideoOutputs());
	const bool			isRGB			(::IsRGBFormat(mConfig.fPixelFormat));
	const bool			canVerify		(mDevice.features().HasCrosspointConnectROM());
	UWord				connectFailures	(0);

	const NTV2OutputXptID	cscVidOutXpt(::GetCSCOutputXptFromChannel(mConfig.fOutputChannel,  false/*isKey*/,  !isRGB/*isRGB*/));
	const NTV2OutputXptID	fsVidOutXpt (::GetFrameBufferOutputXptFromChannel(mConfig.fOutputChannel,  isRGB/*isRGB*/,  false/*is425*/));
	const NTV2InputXptID	cscInputXpt (isRGB ? ::GetCSCInputXptFromChannel(mConfig.fOutputChannel, false/*isKeyInput*/) : NTV2_INPUT_CROSSPOINT_INVALID);
	if (isRGB)
		if (!mDevice.Connect (cscInputXpt,  fsVidOutXpt,  canVerify))
			connectFailures++;

	if (mConfig.fDoMultiFormat)
	{
		//	Multiformat --- We may be sharing the device with other processes, so route only one SDI output...
		if (mDevice.features().HasBiDirectionalSDI())
			mDevice.SetSDITransmitEnable(mConfig.fOutputChannel, true);

		if (!mDevice.Connect (::GetSDIOutputInputXpt (mConfig.fOutputChannel, false/*isDS2*/),  isRGB ? cscVidOutXpt : fsVidOutXpt,  canVerify))
			connectFailures++;
		//	NOTE: No need to send VITC2 with VITC1 (for "i" formats) -- firmware does this automatically
		mDevice.SetSDIOutputStandard (mConfig.fOutputChannel, outputStandard);
	}
	else
	{
		//	Not multiformat:  We own the whole device, so connect all possible SDI outputs...
		const UWord	numFrameStores(mDevice.features().GetNumFrameStores());
		mDevice.ClearRouting();		//	Start with clean slate

		if (isRGB)
			if (!mDevice.Connect (cscInputXpt,  fsVidOutXpt,  canVerify))
				connectFailures++;

		for (NTV2Channel chan(NTV2_CHANNEL1);  ULWord(chan) < numSDIOutputs;  chan = NTV2Channel(chan+1))
		{
			if (chan != mConfig.fOutputChannel  &&  chan < numFrameStores)
				mDevice.DisableChannel(chan);				//	Disable unused FrameStore
			if (mDevice.features().HasBiDirectionalSDI())
				mDevice.SetSDITransmitEnable (chan, true);	//	Make it an output

			if (!mDevice.Connect (::GetSDIOutputInputXpt (chan, false/*isDS2*/),  isRGB ? cscVidOutXpt : fsVidOutXpt,  canVerify))
				connectFailures++;
			mDevice.SetSDIOutputStandard (chan, outputStandard);
			//	NOTE: No need to send VITC2 with VITC1 (for "i" formats) -- firmware does this automatically
		}	//	for each SDI output spigot

		//	And connect analog video output, if the device has one...
		if (mDevice.features().GetNumAnalogVideoOutputs())
			if (!mDevice.Connect (::GetOutputDestInputXpt(NTV2_OUTPUTDESTINATION_ANALOG),  isRGB ? cscVidOutXpt : fsVidOutXpt,  canVerify))
				connectFailures++;

		//	And connect HDMI video output, if the device has one...
		if (mDevice.features().GetNumHDMIVideoOutputs())
			if (!mDevice.Connect (::GetOutputDestInputXpt(NTV2_OUTPUTDESTINATION_HDMI),  isRGB ? cscVidOutXpt : fsVidOutXpt,  canVerify))
				connectFailures++;
	}
	if (connectFailures)
		PLWARN(DEC(connectFailures) << " 'Connect' call(s) failed");
	return connectFailures == 0;

}	//	RouteOutputSignal


AJAStatus NTV2StreamPlayer::Run (void)
{
	//	Start my consumer and producer threads...
	StartConsumerThread();
	StartProducerThread();
	return AJA_STATUS_SUCCESS;

}	//	Run



//////////////////////////////////////////////
//	This is where the play thread starts

void NTV2StreamPlayer::StartConsumerThread (void)
{
	//	Create and start the playout thread...
	mConsumerThread.Attach (ConsumerThreadStatic, this);
	mConsumerThread.SetPriority(AJA_ThreadPriority_High);
	mConsumerThread.Start();

}	//	StartConsumerThread


//	The playout thread function
void NTV2StreamPlayer::ConsumerThreadStatic (AJAThread * pThread, void * pContext)		//	static
{	(void) pThread;
	//	Grab the NTV2StreamPlayer instance pointer from the pContext parameter,
	//	then call its ConsumeFrames method...
	NTV2StreamPlayer * pApp (reinterpret_cast<NTV2StreamPlayer*>(pContext));
	if (pApp)
		pApp->ConsumeFrames();

}	//	ConsumerThreadStatic


void NTV2StreamPlayer::ConsumeFrames (void)
{
	NTV2StreamChannel	strStatus;
	NTV2StreamBuffer	bfrStatus;
	ULWord				goodQueue(0), badQueue(0), goodRelease(0), badRelease(0), starves(0), noRoomWaits(0);
	ULWord				status;

	// lock and map the buffers
	for (FrameDataArrayIter iterHost = mHostBuffers.begin(); iterHost != mHostBuffers.end(); iterHost++)
	{
		if (iterHost->fVideoBuffer)
		{
            mDevice.DMABufferLock(iterHost->fVideoBuffer, true);
		}
	}

	//	Initialize and claim ownership of the stream
	status = mDevice.StreamChannelInitialize(mConfig.fOutputChannel);
	if (status != NTV2_STREAM_STATUS_SUCCESS)
	{
		cerr << "## ERROR:  Stream initialize failed: " << status << endl;
		return;
	}
	PLNOTE("Thread started");

	while (!mGlobalQuit)
	{
        //  Get stream status
		status = mDevice.StreamChannelStatus(mConfig.fOutputChannel, strStatus);
		if (status != NTV2_STREAM_STATUS_SUCCESS)
		{
			cerr << "## ERROR:  Stream status failed: " << status << endl;
			return;
		}

		if (strStatus.GetQueueDepth() < 6)
		{
			FrameData *	pFrameData (&mHostBuffers[mConsumerIndex]);
			if (pFrameData->fDataReady)
            {
				//  Queue frame to stream
				NTV2Buffer buffer(pFrameData->fVideoBuffer.GetHostAddress(0), pFrameData->fVideoBuffer.GetByteCount());
				status = mDevice.StreamBufferQueue(mConfig.fOutputChannel,
													buffer,
													mConsumerIndex,
													bfrStatus);
				if (status == NTV2_STREAM_STATUS_SUCCESS)
				{
					goodQueue++;
					mConsumerIndex = (mConsumerIndex + 1) % CIRCULAR_BUFFER_SIZE;
				}
				else
				{
					badQueue++;
					cerr << "## ERROR:  Stream buffer add failed: " << status << endl;
				}

				if (goodQueue == 3)
				{
					//  Stop the stream (this will start streaming the first buffer to the output)
					status = mDevice.StreamChannelStop(mConfig.fOutputChannel, strStatus);
					if (status != NTV2_STREAM_STATUS_SUCCESS)
					{
						cerr << "## ERROR:  Stream stop failed: " << status << endl;
						return;
					}
					//	Wait for stream to startup
					mDevice.StreamChannelWait(mConfig.fOutputChannel, strStatus);
					while(!strStatus.IsIdle())
					{
						mDevice.StreamChannelWait(mConfig.fOutputChannel, strStatus);
					}
					//	Wait a few more frames for output to stabilize
					for (int i = 0; i < 30; i++)
					{
						mDevice.StreamChannelWait(mConfig.fOutputChannel, strStatus);
					}
					//  Now start the stream
					status = mDevice.StreamChannelStart(mConfig.fOutputChannel, strStatus);
					if (status != NTV2_STREAM_STATUS_SUCCESS)
					{
						cerr << "## ERROR:  Stream start failed: " << status << endl;
						return;
					}
				}
			}
			else
			{
                starves++;
            }
			continue;	//	Back to top of while loop
		}
		else
		{
			noRoomWaits++;
		}

		//  Look for buffers to release
        status = mDevice.StreamBufferRelease(mConfig.fOutputChannel, bfrStatus);
		while (status == NTV2_STREAM_STATUS_SUCCESS)
		{
			if (bfrStatus.mBufferCookie < CIRCULAR_BUFFER_SIZE)
			{
				mHostBuffers[bfrStatus.mBufferCookie].fDataReady = false;
				goodRelease++;
			}
			else
			{
				badRelease++;
			}
            status = mDevice.StreamBufferRelease(mConfig.fOutputChannel, bfrStatus);
		}

		//	Wait for one or more buffers to become available on the device, which should occur at next VBI...
		status = mDevice.StreamChannelWait(mConfig.fOutputChannel, strStatus);
		if (status != NTV2_STREAM_STATUS_SUCCESS)
		{
			break;
		}
	}	//	loop til quit signaled

	//	Stop streaming...
	status = mDevice.StreamChannelInitialize(mConfig.fOutputChannel);
	if (status != NTV2_STREAM_STATUS_SUCCESS)
	{
		cerr << "## ERROR:  Stream initialize failed: " << status << endl;
		return;
	}

    //  Release all buffers
    status = mDevice.StreamBufferRelease(mConfig.fOutputChannel, bfrStatus);
    while (status == NTV2_STREAM_STATUS_SUCCESS)
    {
		if (bfrStatus.mBufferCookie < CIRCULAR_BUFFER_SIZE)
		{
		//	Signal that I'm done consuming this FrameData, making it immediately available for more data...
			mHostBuffers[bfrStatus.mBufferCookie].fDataReady = false;
			goodRelease++;
		}
		else
		{
			badRelease++;
		}
        status = mDevice.StreamBufferRelease(mConfig.fOutputChannel, bfrStatus);
    }

    //  Release stream ownership
	status = mDevice.StreamChannelRelease(mConfig.fOutputChannel);
	if (status != NTV2_STREAM_STATUS_SUCCESS)
	{
		cerr << "## ERROR:  Stream release failed: " << status << endl;
		return;
	}    

	PLNOTE("Thread completed: " << DEC(goodQueue) << " queued, " << DEC(badQueue) << " failed, "
			<< DEC(starves) << " starves, " << DEC(noRoomWaits) << " VBI waits");

}	//	ConsumeFrames



//////////////////////////////////////////////
//	This is where the producer thread starts

void NTV2StreamPlayer::StartProducerThread (void)
{
	//	Create and start the producer thread...
	mProducerThread.Attach(ProducerThreadStatic, this);
	mProducerThread.SetPriority(AJA_ThreadPriority_High);
	mProducerThread.Start();

}	//	StartProducerThread


void NTV2StreamPlayer::ProducerThreadStatic (AJAThread * pThread, void * pContext)	//	static
{	(void) pThread;
	NTV2StreamPlayer * pApp (reinterpret_cast<NTV2StreamPlayer*>(pContext));
	if (pApp)
		pApp->ProduceFrames();

}	//	ProducerThreadStatic


void NTV2StreamPlayer::ProduceFrames (void)
{
	ULWord	testPatNdx(0), badTally(0);
	double	timeOfLastSwitch	(0.0);

	const AJATimeBase		timeBase		(CNTV2DemoCommon::GetAJAFrameRate(::GetNTV2FrameRateFromVideoFormat(mConfig.fVideoFormat)));
	const NTV2StringList	tpNames			(NTV2TestPatternGen::getTestPatternNames());
	const bool				isInterlace		(!NTV2_VIDEO_FORMAT_HAS_PROGRESSIVE_PICTURE(mConfig.fVideoFormat));
	const bool				isPAL			(NTV2_IS_PAL_VIDEO_FORMAT(mConfig.fVideoFormat));
	const NTV2FrameRate		ntv2FrameRate	(::GetNTV2FrameRateFromVideoFormat(mConfig.fVideoFormat));
	const TimecodeFormat	tcFormat		(CNTV2DemoCommon::NTV2FrameRate2TimecodeFormat(ntv2FrameRate));

	PLNOTE("Thread started");

	while (!mGlobalQuit)
	{
		FrameData *	pFrameData (&mHostBuffers[mProducerIndex]);
		if (pFrameData->fDataReady)
		{	
			badTally++;			//	No frame available!
			AJATime::Sleep(10);	//	Wait a bit for the consumer thread to free one up for me...
			continue;			//	...then try again
		}

		//	Copy fresh, unmodified, pre-rendered test pattern into this frame's video buffer...
		pFrameData->fVideoBuffer.CopyFrom (mTestPatRasters.at(testPatNdx),
											/*srcOffset*/ 0,
											/*dstOffset*/ 0,
											/*byteCount*/ pFrameData->fVideoBuffer.GetByteCount());
	
		const	CRP188	rp188Info (mCurrentFrame++, 0, 0, 10, tcFormat);
		NTV2_RP188		tcF1, tcF2;
		string			tcString;

		rp188Info.GetRP188Reg(tcF1);
		rp188Info.GetRP188Str(tcString);

		//	Include timecode in output signal...
		tcF2 = tcF1;
		if (isInterlace)
		{	//	Set bit 27 of Hi word (PAL) or Lo word (NTSC)
			if (isPAL) tcF2.fHi |=  BIT(27);	else tcF2.fLo |=  BIT(27);
		}

		if (pFrameData->fVideoBuffer.GetHostAddress(0))
		{	//	Burn current timecode into the video buffer...
			mTCBurner.BurnTimeCode (pFrameData->fVideoBuffer.GetHostAddress(0), tcString.c_str(), 80);
		}
		TCDBG("F" << DEC0N(mCurrentFrame-1,6) << ": " << tcF1 << ": " << tcString);

		//	Every few seconds, change the test pattern and tone frequency...
		const double currentTime (timeBase.FramesToSeconds(mCurrentFrame));
		if (currentTime > timeOfLastSwitch + 4.0)
		{
			testPatNdx = (testPatNdx + 1) % ULWord(mTestPatRasters.size());
			timeOfLastSwitch = currentTime;
			PLINFO("F" << DEC0N(mCurrentFrame,6) << ": " << tcString << "pattern='" << tpNames.at(testPatNdx) << "'");
		}	//	if time to switch test pattern

		//	Signal that I'm done producing this FrameData, making it immediately available for transfer/playout...
		pFrameData->fDataReady = true;
		mProducerIndex = (mProducerIndex + 1) % CIRCULAR_BUFFER_SIZE;
	}	//	loop til mGlobalQuit goes true

	PLNOTE("Thread completed: " << DEC(mCurrentFrame) << " frame(s) produced, " << DEC(badTally) << " failed");
}	//	ProduceFrames


void NTV2StreamPlayer::GetStreamStatus (NTV2StreamChannel & outStatus)
{
	mDevice.StreamChannelStatus(mConfig.fOutputChannel, outStatus);
}

