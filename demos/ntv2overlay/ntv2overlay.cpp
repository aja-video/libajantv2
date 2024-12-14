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
static const uint32_t	kAppSignature	(NTV2_FOURCC('B','u','r','n'));


//////////////////////	IMPLEMENTATION


NTV2Overlay::NTV2Overlay (const OverlayConfig & inConfig)
	:	mConfig			(inConfig),
		mPlayThread		(AJAThread()),
		mCaptureThread	(AJAThread()),
		mDeviceID		(DEVICE_ID_NOTFOUND),
		mVideoFormat	(NTV2_FORMAT_UNKNOWN),
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

	mDeviceID = mDevice.GetDeviceID();	//	Keep the device ID handy since it will be used frequently
	if (!mDevice.features().CanDoCapture())
		{cerr << "## ERROR:  Device '" << mDeviceID << "' cannot capture" << endl;  return AJA_STATUS_FEATURE;}
	if (!mDevice.features().CanDoPlayback())
		{cerr << "## ERROR:  Device '" << mDeviceID << "' cannot playout" << endl;  return AJA_STATUS_FEATURE;}

	ULWord	appSignature	(0);
	int32_t	appPID			(0);
	mDevice.GetStreamingApplication (appSignature, appPID);	//	Who currently "owns" the device?
	mDevice.GetEveryFrameServices(mSavedTaskMode);			//	Save the current device state
	if (!mConfig.fDoMultiFormat)
	{
		if (!mDevice.AcquireStreamForApplication (kAppSignature, int32_t(AJAProcess::GetPid())))
		{
			cerr << "## ERROR:  Unable to acquire device because another app (pid " << appPID << ") owns it" << endl;
			return AJA_STATUS_BUSY;		//	Some other app is using the device
		}
		mDevice.ClearRouting();									//	Clear the current device routing (since I "own" the device)
	}
	mDevice.SetEveryFrameServices(NTV2_OEM_TASKS);			//	Force OEM tasks

	//	Configure the SDI relays if present
	if (mDevice.features().HasSDIRelays())
	{
		//	Note that if the board's jumpers are not set in the position
		//	to enable the watchdog timer, these calls will have no effect.
		mDevice.SetSDIWatchdogEnable(true, 0);	//	SDI 1/2
		mDevice.SetSDIWatchdogEnable(true, 1);	//	SDI 3/4

		//	Set timeout delay to 2 seconds expressed in multiples of 8 ns
		//	and take the relays out of bypass...
		mDevice.SetSDIWatchdogTimeout(2 * 125000000);
		mDevice.KickSDIWatchdog();

		//	Give the mechanical relays some time to switch...
		AJATime::Sleep(500);
	}

	//	Make sure the device actually supports custom anc before using it...
	if (mConfig.WithAnc())
		mConfig.fWithAnc = mDevice.features().CanDoCustomAnc();

	//	Set up the video and audio...
	status = SetupVideo();
	if (AJA_FAILURE(status))
		return status;
	status = mConfig.WithAudio() ? SetupAudio() : AJA_STATUS_SUCCESS;
	if (AJA_FAILURE(status))
		return status;

	//	Set up the signal routing...
	NTV2XptConnections connections;
	GetInputSignalRouting(connections);
	GetOutputSignalRouting(connections);
	if (!mDevice.ApplySignalRoute(connections, /*replaceAllRoutes?*/!mConfig.fDoMultiFormat))
		cerr << "## WARN: ApplySignalRoute failed for:" << endl << connections << endl;

	//	Set up Mixer to "mix" mode, FG raster "unshaped", BG raster "full raster", pass thru VANC from background video...
	const UWord	mixerNum (mConfig.fInputChannel);
	mDevice.SetMixerMode (mixerNum, NTV2MIXERMODE_FOREGROUND_ON);
	mDevice.SetMixerFGInputControl (mixerNum, NTV2MIXERINPUTCONTROL_UNSHAPED);
	mDevice.SetMixerBGInputControl (mixerNum, NTV2MIXERINPUTCONTROL_FULLRASTER);
	mDevice.SetMixerVancOutputFromForeground (mixerNum, false);	//	false means "use BG VANC, not FG"

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
	//	If no input source specified, choose a default...
	if (!NTV2_IS_VALID_INPUT_SOURCE(mConfig.fInputSource))
	{
		mConfig.fInputSource = ::NTV2ChannelToInputSource(NTV2_CHANNEL1, mDeviceID == DEVICE_ID_KONAHDMI ? NTV2_IOKINDS_HDMI : NTV2_IOKINDS_SDI);
		if (mConfig.IsVerbose())
			cout << "## NOTE:  Input source was not specified, will use " << mConfig.ISrcStr() << endl;
	}

	//	Does this device have the requested input source?
	if (!mDevice.features().CanDoInputSource(mConfig.fInputSource))
		{cerr << "## ERROR:  Device does not have input source " << mConfig.ISrcStr() << endl;  return AJA_STATUS_BAD_PARAM;}

	//	Enable/subscribe interrupts...
	mConfig.fInputChannel = ::NTV2InputSourceToChannel(mConfig.fInputSource);
	mDevice.EnableInputInterrupt(mConfig.fInputChannel);
	mDevice.SubscribeInputVerticalEvent(mConfig.fInputChannel);
	mDevice.EnableOutputInterrupt(NTV2_CHANNEL1);
	mDevice.SubscribeOutputVerticalEvent(NTV2_CHANNEL1);

	//	Flip the input spigot to "receive" if necessary...
	bool isXmit (false);
	if (mDevice.features().HasBiDirectionalSDI()							//	If device has bidirectional SDI connectors...
		&& mConfig.ISrcIsSDI()												//	...and desired input source is SDI...
		&& mDevice.GetSDITransmitEnable (mConfig.fInputChannel, isXmit)		//	...and GetSDITransmitEnable succeeds...
		&& isXmit)															//	...and input is set to "transmit"...
		{
			mDevice.SetSDITransmitEnable (mConfig.fInputChannel, false);		//	...then disable transmit mode...
			mDevice.WaitForOutputVerticalInterrupt (NTV2_CHANNEL1, 10);			//	...and give device time to lock to input
		}	//	if input SDI connector needs to switch from transmit mode

	//	Since this demo runs in E-to-E mode (thru Mixer/Keyer), reference must be tied to the input...
	mDevice.SetReference(::NTV2InputSourceToReferenceSource(mConfig.fInputSource));

	//	Transparent overlays will need an ARGB buffer format...
	mConfig.fPixelFormat = NTV2_FBF_ARGB;
	if (!mDevice.features().CanDoFrameBufferFormat(mConfig.fPixelFormat))
		{cerr << "## ERROR: " << ::NTV2FrameBufferFormatToString(mConfig.fPixelFormat) << " unsupported" << endl;  return AJA_STATUS_UNSUPPORTED;}

	//	If the device supports different per-channel video formats, configure it as requested...
	if (mDevice.features().CanDoMultiFormat())
		mDevice.SetMultiFormatMode(mConfig.fDoMultiFormat);

	//	Choose an output channel/FrameStore...
	const UWord	numFrameStores (mDevice.features().GetNumFrameStores());
	switch (mConfig.fInputSource)
	{
		case NTV2_INPUTSOURCE_SDI1:		mConfig.fOutputChannel = numFrameStores > 4 ? NTV2_CHANNEL2 : (numFrameStores == 2 ? NTV2_CHANNEL2 : NTV2_CHANNEL3);
										break;

		case NTV2_INPUTSOURCE_HDMI2:
		case NTV2_INPUTSOURCE_SDI2:		mConfig.fOutputChannel = numFrameStores > 4 ? NTV2_CHANNEL3 : NTV2_CHANNEL4;
										break;

		case NTV2_INPUTSOURCE_HDMI3:
		case NTV2_INPUTSOURCE_SDI3:		mConfig.fOutputChannel = NTV2_CHANNEL4;
										break;

		case NTV2_INPUTSOURCE_HDMI4:
		case NTV2_INPUTSOURCE_SDI4:		mConfig.fOutputChannel = numFrameStores > 4 ? NTV2_CHANNEL5 : NTV2_CHANNEL3;
										break;

		case NTV2_INPUTSOURCE_SDI5: 	mConfig.fOutputChannel = NTV2_CHANNEL6;		break;
		case NTV2_INPUTSOURCE_SDI6:		mConfig.fOutputChannel = NTV2_CHANNEL7;		break;
		case NTV2_INPUTSOURCE_SDI7:		mConfig.fOutputChannel = NTV2_CHANNEL8;		break;
		case NTV2_INPUTSOURCE_SDI8:		mConfig.fOutputChannel = NTV2_CHANNEL7;		break;

		case NTV2_INPUTSOURCE_ANALOG1:
		case NTV2_INPUTSOURCE_HDMI1:	mConfig.fOutputChannel = numFrameStores < 3 ? NTV2_CHANNEL2 : NTV2_CHANNEL3;
										break;
		default:
		case NTV2_INPUTSOURCE_INVALID:	cerr << "## ERROR:  Bad input source" << endl;  return AJA_STATUS_BAD_PARAM;
	}
	if (mConfig.IsVerbose())
		cout << "## NOTE:  Output FrameStore chosen to be " << mConfig.OChStr() << endl;
	if (NTV2_IS_VALID_OUTPUT_DEST(mConfig.fOutputDest))
	{
		if (NTV2_OUTPUT_DEST_IS_SDI(mConfig.fOutputDest))
			if (NTV2_IS_VALID_CHANNEL(mConfig.ODstCh()))
				if (mConfig.ODstCh() != mConfig.fOutputChannel)
				{
					const string oldChStr(mConfig.OChStr());
					mConfig.fOutputChannel = mConfig.ODstCh();
					if (mConfig.IsVerbose())
						cout << "## NOTE:  Output " << mConfig.ODstStr() << " with Anc forced FrameStore change to "
								<< mConfig.OChStr() << " from " << oldChStr << endl;
				}
	}
	else	//	else output destination not specified
	{	//	Pick an appropriate output spigot based on the output channel...
		mConfig.fOutputDest	= ::NTV2ChannelToOutputDestination(mConfig.fOutputChannel);
		if (!HasWidgetsAnyOf(NTV2_Wgt12GSDIOut2, NTV2_Wgt3GSDIOut2, NTV2_WgtSDIOut2))
		{	//	If device has only one SDI output
			mConfig.fOutputDest = NTV2_OUTPUTDESTINATION_SDI1;
			if (mConfig.IsVerbose())
				cout << "## NOTE:  Output destination was not specified, will use " << mConfig.ODstStr() << endl;
		}
	}
	if (mDevice.features().HasBiDirectionalSDI()					//	If device has bidirectional SDI connectors...
		&& mConfig.ODstIsSDI())										//	...and output destination is SDI...
			mDevice.SetSDITransmitEnable (mConfig.ODstCh(), true);	//	...then enable transmit mode
	if (mConfig.fInputChannel == mConfig.fOutputChannel)
		{cerr << "## ERROR: Input " << mConfig.IChStr() << " & output " << mConfig.OChStr() << " conflict" << endl;  return AJA_STATUS_BAD_PARAM;}
	if (mDevice.features().HasBiDirectionalSDI() && mConfig.ISrcIsSDI() && mConfig.ODstIsSDI() && mConfig.ISrcCh() == mConfig.ODstCh())
		{cerr << "## ERROR: SDI conflict:  input " << mConfig.ISrcStr() << " & output " << mConfig.ODstStr() << " are same connector" << endl;  return AJA_STATUS_BAD_PARAM;}
	return AJA_STATUS_SUCCESS;

}	//	SetupVideo


AJAStatus NTV2Overlay::SetupAudio (void)
{
	NTV2AudioSystem audioSystem = NTV2_AUDIOSYSTEM_1;	//	the audio system that's associated with the input spigot

	//	Set up the output audio embedders...
	if (mDevice.features().GetNumAudioSystems() > 1)
	{
		//	Some devices, like the Kona1, have 2 FrameStores but only 1 SDI output,
		//	which makes mConfig.fOutputChannel == NTV2_CHANNEL2, but need SDIoutput to be NTV2_CHANNEL1...
		UWord	SDIoutput(mConfig.fOutputChannel);
		if (SDIoutput >= mDevice.features().GetNumVideoOutputs())
			SDIoutput = mDevice.features().GetNumVideoOutputs() - 1;
		mDevice.SetSDIOutputAudioSystem (NTV2Channel(SDIoutput), audioSystem);

		if (mDevice.features().GetNumHDMIVideoOutputs() > 0)
			mDevice.SetHDMIOutAudioSource2Channel(NTV2_AudioChannel1_2, audioSystem);
	}

	//
	//	Loopback mode plays whatever audio appears in the input signal when it's
	//	connected directly to an output (i.e., "end-to-end" mode). If loopback is
	//	left enabled, the video will lag the audio as video frames get briefly delayed
	//	in our ring buffer. Audio, therefore, needs to come out of the (buffered) frame
	//	data being played, so loopback must be turned off...
	//
	mDevice.SetAudioLoopBack (NTV2_AUDIO_LOOPBACK_ON, audioSystem);
	return AJA_STATUS_SUCCESS;

}	//	SetupAudio


bool NTV2Overlay::GetInputSignalRouting (NTV2XptConnections & connections)
{
	const size_t origSize(connections.size());
	const NTV2OutputXptID inputOutputXpt (::GetInputSourceOutputXpt(mConfig.fInputSource));
	const NTV2InputXptID mixBGVidInputXpt(::GetMixerBGInputXpt(mConfig.fInputChannel));
	const NTV2InputXptID mixBGKeyInputXpt(::GetMixerBGInputXpt(mConfig.fInputChannel, /*key*/true));
	NTV2LHIHDMIColorSpace colorSpace (NTV2_LHIHDMIColorSpaceYCbCr);

	if (NTV2IOKind(::GetNTV2InputSourceKind(mConfig.fInputSource)) == NTV2_IOKINDS_HDMI
		&&  mDevice.GetHDMIInputColor(colorSpace, mConfig.fInputChannel)
		&&  colorSpace == NTV2_LHIHDMIColorSpaceRGB)
		{
			//	The HDMI input video is RGB, but the Mixer only accepts YCbCr video or key.
			//	The HDMI video signal must go thru a CSC on its way to the Mixer...
			const NTV2InputXptID cscVidInputXpt(::GetCSCInputXptFromChannel(mConfig.fInputChannel));
			const NTV2OutputXptID cscVidOutputXpt(::GetCSCOutputXptFromChannel(mConfig.fInputChannel, false/*isKey*/));
			const NTV2OutputXptID cscKeyOutputXpt(::GetCSCOutputXptFromChannel(mConfig.fInputChannel, true/*isKey*/));

			connections.insert(NTV2XptConnection(cscVidInputXpt, inputOutputXpt));		//	CSC vid input from HDMI input
			connections.insert(NTV2XptConnection(mixBGVidInputXpt, cscVidOutputXpt));	//	Mixer BG vid input from CSC vid output
			connections.insert(NTV2XptConnection(mixBGKeyInputXpt, cscVidOutputXpt));	//	Mixer BG key input from CSC key output
		}
	else
	{
		connections.insert(NTV2XptConnection(mixBGVidInputXpt, inputOutputXpt));	//	Mixer BG vid input from input spigot
		connections.insert(NTV2XptConnection(mixBGKeyInputXpt, inputOutputXpt));	//	Mixer BG key input from input spigot
	}
	return (connections.size() - origSize) > 0;	//	return true (success) if any connections added

}	//	GetInputSignalRouting


bool NTV2Overlay::GetOutputSignalRouting (NTV2XptConnections & connections)
{
	const size_t origSize(connections.size());
	const NTV2InputXptID outputInputXpt (::GetOutputDestInputXpt(mConfig.fOutputDest));
	const NTV2OutputXptID mixOutputXpt (::GetMixerOutputXptFromChannel(mConfig.fOutputChannel));

	{	//	FrameStore to CSC to Mixer FG routing...
		const NTV2OutputXptID fsRGBOutputXpt (::GetFrameStoreOutputXptFromChannel(mConfig.fOutputChannel, /*rgb*/true));
		const NTV2InputXptID cscVidInputXpt(::GetCSCInputXptFromChannel(mConfig.fOutputChannel));
		const NTV2OutputXptID cscVidOutputXpt(::GetCSCOutputXptFromChannel(mConfig.fOutputChannel, false/*isKey*/));
		const NTV2OutputXptID cscKeyOutputXpt(::GetCSCOutputXptFromChannel(mConfig.fOutputChannel, true/*isKey*/));
		const NTV2InputXptID mixFGVidInputXpt(::GetMixerFGInputXpt(mConfig.fInputChannel));
		const NTV2InputXptID mixFGKeyInputXpt(::GetMixerFGInputXpt(mConfig.fInputChannel, /*key*/true));

		connections.insert(NTV2XptConnection(cscVidInputXpt, fsRGBOutputXpt));		//	CSC vid input from FrameStore RGB output
		connections.insert(NTV2XptConnection(mixFGVidInputXpt, cscVidOutputXpt));	//	Mixer FG vid input from CSC vid output
		connections.insert(NTV2XptConnection(mixFGKeyInputXpt, cscKeyOutputXpt));	//	Mixer FG key input from CSC key output
	}

	//	Mixer output to primary SDI output...
	connections.insert(NTV2XptConnection(outputInputXpt, mixOutputXpt));	//	SDIOut from Mixer output

	if (!mConfig.fDoMultiFormat)
	{
		//	Also route HDMIOut, SDIMonOut and/or AnalogOut (if available)...
		if (mDevice.features().GetNumHDMIVideoOutputs() > 0)
			connections.insert(NTV2XptConnection(NTV2_XptHDMIOutQ1Input, mixOutputXpt));	//	HDMIOut from Mixer output
		if (mDevice.features().CanDoWidget(NTV2_WgtAnalogOut1))
			connections.insert(NTV2XptConnection(NTV2_XptAnalogOutInput, mixOutputXpt));	//	AnalogOut from Mixer output
		if (mDevice.features().CanDoWidget(NTV2_WgtSDIMonOut1))
			connections.insert(NTV2XptConnection(::GetSDIOutputInputXpt(NTV2_CHANNEL5), mixOutputXpt));	//	SDIMonOut from Mixer output
	}	//	if not multiChannel
	return (connections.size() - origSize) > 0;	//	return true (success) if any connections added

}	//	GetOutputSignalRouting

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
//
void NTV2Overlay::OutputThread (void)
{
	AJAAtomic::Increment(&gPlayEnterCount);
	const ULWord threadTally(gPlayEnterCount);

	OVRLNOTE(DEC(threadTally) << " started, video format: " << ::NTV2VideoFormatToString(mVideoFormat));
	ULWord goodWaits(0), badWaits(0), goodXfers(0), badXfers(0), starves(0), noRoomWaits(0);
	const NTV2VideoFormat vf (mVideoFormat);

	//	Configure the output FrameStore...
	mDevice.EnableChannel(mConfig.fOutputChannel);
	mDevice.SetMode (mConfig.fOutputChannel, NTV2_MODE_DISPLAY);
	mDevice.SetFrameBufferFormat (mConfig.fOutputChannel, mConfig.fPixelFormat);
	mDevice.SetVideoFormat (mVideoFormat, /*retail*/false,  /*keepVanc*/false, mConfig.fOutputChannel);
	mDevice.SetVANCMode (NTV2_VANCMODE_OFF, mConfig.fOutputChannel);

	AUTOCIRCULATE_STATUS acStatus;
	ULWord fbNum (10);
	mDevice.AutoCirculateStop(mConfig.fOutputChannel);
	if (mDevice.AutoCirculateInitForOutput(mConfig.fOutputChannel, 2) && mDevice.AutoCirculateGetStatus(mConfig.fOutputChannel, acStatus))	//	Find out which buffers we got
		fbNum = ULWord(acStatus.acStartFrame);	//	Use them
	else
		{cerr << "## NOTE:  A/C allocate device frame buffer range failed" << endl;	return;}

	const NTV2FormatDesc formatDesc (vf, mConfig.fPixelFormat);
	const ULWord bufferSizeBytes (formatDesc.GetTotalRasterBytes());
	ULWord pingPong(0);	//	Bounce between 0 and 1

	//	Allocate overlay host frame buffer...
	NTV2Buffer hostBuffer;
	if (!hostBuffer.Allocate(bufferSizeBytes, /*pageAligned*/true))
		{cerr << "## NOTE:  Unable to allocate " << bufferSizeBytes << "-byte caption video buffer" << endl;	return;}

	//	Clear both device ping/pong buffers to fully transparent, all black...
	hostBuffer.Fill(ULWord(0));
	mDevice.DMAWriteFrame (fbNum + 0, hostBuffer, bufferSizeBytes);
	mDevice.DMAWriteFrame (fbNum + 1, hostBuffer, bufferSizeBytes);
	mDevice.SetOutputFrame (mConfig.fOutputChannel, fbNum + pingPong);
	pingPong = pingPong ? 0 : 1;

	//	Do forever (until Quit)...
	while (!mGlobalQuit)
	{
		//	Input thread will signal us to terminate by stopping A/C...
		if (mDevice.AutoCirculateGetStatus(mConfig.fOutputChannel, acStatus)  &&  acStatus.IsStopped())
			{OVRLDBG("AC" << DEC(mConfig.fOutputChannel+1) << " stopped");	break;}

		//	Wait for the next input vertical interrupt event to get signaled...
		if (mDevice.WaitForInputVerticalInterrupt(mConfig.fInputChannel))
			++goodWaits;
		else
			++badWaits;
		{
			//	Transfer the overlay to the device, then ping-pong it into the mixer foreground...
			mDevice.DMAWriteFrame (fbNum + pingPong, hostBuffer, hostBuffer.GetByteCount());
			mDevice.SetOutputFrame (mConfig.fOutputChannel, fbNum + pingPong);	//	Toggle device frame buffer
			pingPong = pingPong ? 0 : 1;	//	Switch device frame numbers for next time
			hostBuffer.Fill(ULWord(0));		//	Clear host buffer to fully-transparent, all-black again

			//	Check for format change...
			if (vf != mVideoFormat)
				{cerr << "## NOTE:  Format change, " << ::NTV2VideoFormatToString(vf) << " to " << ::NTV2VideoFormatToString(mVideoFormat) << endl; break;}

			//	Check mixer sync...
			bool mixerSyncOK (false);
			if (mDevice.GetMixerSyncStatus(UWord(mConfig.fOutputChannel), mixerSyncOK)  &&  !mixerSyncOK)
				{cerr << "## NOTE:  Lost mixer " << DEC(mConfig.fOutputChannel+1) << " sync" << endl;	break;}
		}	//	if pPlayData
	}	//	loop til quit signaled

	if (mDevice.AutoCirculateGetStatus (mConfig.fOutputChannel, acStatus)  &&  !acStatus.IsStopped())
		mDevice.AutoCirculateStop(mConfig.fOutputChannel);

	AJAAtomic::Increment(&gPlayExitCount);
	OVRLNOTE(DEC(threadTally) << " completed: " << DEC(goodXfers) << " xfers, " << DEC(badXfers) << " failed, "
			<< DEC(starves) << " ring starves, " << DEC(noRoomWaits) << " device starves");
	if (gPlayEnterCount != gPlayExitCount)
		OVRLFAIL("Enter/Exit counts differ: " << DEC(gPlayEnterCount) << "/" << DEC(gPlayExitCount));

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
//		-	monitors the input signal
//		-	terminates the output thread upon signal loss or change
//		-	starts a new output thread if/when the input signal is recovered
//
void NTV2Overlay::InputThread (void)
{
	ULWord	noVidTally(0), badWaits(0), waitTally(0);
	OVRLNOTE("Started");

	//	Loop until time to quit...
	while (!mGlobalQuit)
	{
		mVideoFormat = WaitForStableInputSignal();
		if (mVideoFormat == NTV2_FORMAT_UNKNOWN)
			break;	//	Quit

		//	At this point, the input video format is stable.

		StartOutputThread();	//	Start a new playout thread

		//	Spin until signal format changes...
		while (!mGlobalQuit)
		{
			//	Wait for the next input vertical interrupt event to get signaled...
			if (mDevice.WaitForInputVerticalInterrupt(mConfig.fInputChannel))
				++waitTally;
			else
				++badWaits;

			//	Input signal format change?
			const NTV2VideoFormat vf (mDevice.GetInputVideoFormat(mConfig.fInputSource));
			const bool vfChanged(vf != mVideoFormat);
			if (vfChanged)
			{
				++noVidTally;
				if (vfChanged)
				{
					OVRLWARN("Signal format at " << ::NTV2InputSourceToString(mConfig.fInputSource) << " changed from '"
								<< ::NTV2VideoFormatToString(mVideoFormat) << "' to '" << ::NTV2VideoFormatToString(vf) << "'");
					cout << "## NOTE: Signal format at " << ::NTV2InputSourceToString(mConfig.fInputSource) << " changed from "
								<< ::NTV2VideoFormatToString(mVideoFormat) << " to " << ::NTV2VideoFormatToString(vf) << endl;
				}

				//	Terminate the output thread...
				mDevice.AutoCirculateStop(mConfig.fOutputChannel);
				break;	//	exit inner loop
			}	//	if incoming video format changed
		}	//	inner loop -- until signal change

	}	//	loop til quit signaled

	OVRLNOTE("Completed: " << DEC(noVidTally) << " no video, " << DEC(waitTally) << " good waits, " << DEC(badWaits) << " bad waits");

}	//	InputThread

