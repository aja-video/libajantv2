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
	if (!mDevice.features().CanDoCapture())
		{cerr << "## ERROR:  '" << mDevice.GetDisplayName() << "' cannot capture" << endl;  return AJA_STATUS_FEATURE;}
	if (!mDevice.features().CanDoPlayback())
		{cerr << "## ERROR:  '" << mDevice.GetDisplayName() << "' cannot playout" << endl;  return AJA_STATUS_FEATURE;}
	const UWord	numMixers(mDevice.features().GetNumMixers());
	if (!numMixers)
		{cerr << "## ERROR:  '" << mDevice.GetDisplayName() << "' has no Mixer/Keyer widget" << endl;  return AJA_STATUS_FEATURE;}

	ULWord	appSig(0);
	int32_t	appPID(0);
	mDevice.GetStreamingApplication (appSig, appPID);	//	Who currently "owns" the device?
	mDevice.GetEveryFrameServices(mSavedTaskMode);		//	Save the current device state
	if (!mConfig.fDoMultiFormat)
	{
		if (!mDevice.AcquireStreamForApplication (kAppSignature, int32_t(AJAProcess::GetPid())))
		{
			cerr << "## ERROR:  Cannot acquire device because another app (pid " << appPID << ") owns it" << endl;
			return AJA_STATUS_BUSY;		//	Some other app is using the device
		}
		mDevice.ClearRouting();	//	Clear current device routing (since I "own" the device)
	}
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

	//	Set up Mixer...
	const UWord mixerNum (MixerNum());
	if (mixerNum >= numMixers)
		{cerr << "## ERROR:  '" << mDevice.GetDisplayName() << "' has no Mixer" << DEC(mixerNum+1) << ", has " << DEC(numMixers) << " mixers" << endl;  return AJA_STATUS_FEATURE;}
	mDevice.SetMixerMode (mixerNum, NTV2MIXERMODE_MIX);	//	"mix" mode
	mDevice.SetMixerFGMatteEnabled (mixerNum, false);	//	no FG matte
	mDevice.SetMixerBGMatteEnabled (mixerNum, false);	//	no BG matte
	mDevice.SetMixerCoefficient (mixerNum, 0x10000);	//	FG "bug" overlay full strength (initially)
	mDevice.SetMixerFGInputControl (mixerNum, NTV2MIXERINPUTCONTROL_SHAPED);		//	respect FG alpha channel
	mDevice.SetMixerBGInputControl (mixerNum, NTV2MIXERINPUTCONTROL_FULLRASTER);	//	BG uses full raster
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
		mConfig.fInputSource = ::NTV2ChannelToInputSource(NTV2_CHANNEL1,
															mDevice.GetDeviceID() == DEVICE_ID_KONAHDMI
																? NTV2_IOKINDS_HDMI
																: NTV2_IOKINDS_SDI);
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
		case NTV2_INPUTSOURCE_SDI1:
		case NTV2_INPUTSOURCE_ANALOG1:
		case NTV2_INPUTSOURCE_HDMI1:	mConfig.fOutputChannel = NTV2_CHANNEL2;
										break;

		case NTV2_INPUTSOURCE_HDMI2:
		case NTV2_INPUTSOURCE_SDI2:		mConfig.fOutputChannel = numFrameStores > 4 ? NTV2_CHANNEL3 : NTV2_CHANNEL1;
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

		default:
		case NTV2_INPUTSOURCE_INVALID:	cerr << "## ERROR:  Bad input source " << DEC(mConfig.fInputSource) << endl;  return AJA_STATUS_BAD_PARAM;
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

	//	Enable loopback for E-E mode (i.e. output whatever audio is in input signal)...
	mDevice.SetAudioLoopBack (NTV2_AUDIO_LOOPBACK_ON, audioSystem);
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


bool NTV2Overlay::RouteInputSignal (void)
{
	if (!mInputConnections.empty())
		mDevice.RemoveConnections(mInputConnections);
	mInputConnections.clear();
	const NTV2OutputXptID inputOutputXpt (::GetInputSourceOutputXpt(mConfig.fInputSource));
	const NTV2InputXptID mixBGVidInputXpt(::GetMixerBGInputXpt(mConfig.fInputChannel));
	const NTV2InputXptID mixBGKeyInputXpt(::GetMixerBGInputXpt(mConfig.fInputChannel, /*key*/true));
	const NTV2InputXptID fsInputXpt (::GetFrameStoreInputXptFromChannel(mConfig.fInputChannel));

	if (NTV2IOKind(::GetNTV2InputSourceKind(mConfig.fInputSource)) == NTV2_IOKINDS_HDMI
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

}	//	DoInputSignalRouting


bool NTV2Overlay::RouteOutputSignal (void)
{
	if (!mOutputConnections.empty())
		mDevice.RemoveConnections(mOutputConnections);
	mOutputConnections.clear();

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
		if (!mConfig.fDoMultiFormat)
		{
			//	Also route HDMIOut, SDIMonOut and/or AnalogOut (if available)...
			if (mDevice.features().GetNumHDMIVideoOutputs() > 0)
				mOutputConnections.insert(NTV2XptConnection(NTV2_XptHDMIOutQ1Input, mixOutputXpt));	//	HDMIOut from Mixer output
			if (mDevice.features().CanDoWidget(NTV2_WgtAnalogOut1))
				mOutputConnections.insert(NTV2XptConnection(NTV2_XptAnalogOutInput, mixOutputXpt));	//	AnalogOut from Mixer output
			if (mDevice.features().CanDoWidget(NTV2_WgtSDIMonOut1))
				mOutputConnections.insert(NTV2XptConnection(::GetSDIOutputInputXpt(NTV2_CHANNEL5), mixOutputXpt));	//	SDIMonOut from Mixer output
		}	//	if not multiChannel
	}
	return mDevice.ApplySignalRoute(mOutputConnections);

}	//	DoOutputSignalRouting

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
//			-	routes my (output) widgets
//			-	initializes (but doesn't start) AutoCirculate (to allocate 2 device buffers)
//		-	while running:
//			-	blits the overlay image into the host overlay raster buffer
//			-	DMAs the overlay raster buffer to the device for mixing
//		-	terminates when AutoCirculate channel is stopped or when mGlobalQuit goes true
//			-	ensures my AutoCirculate channel is stopped
//			-	removes my (output) widget routing
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

	RouteOutputSignal();

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
		if (mDevice.AutoCirculateGetStatus(mConfig.fOutputChannel, acStatus)  &&  acStatus.IsStopped())
			break;	//	Stopped, exit thread

		//	Wait for the next input vertical interrupt event to get signaled...
		if (mDevice.WaitForInputVerticalInterrupt(mConfig.fInputChannel))
		{	++goodWaits;

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
//			-	routes all input widgets
//			-	initializes AutoCirculate to reserve a couple of device frame buffers
//			-	starts a new output thread
//			-	waits indefinitely to see if the input signal is lost or changes, and if it does:
//				-	tells the output thread to exit
//				-	stops input AutoCirculate, and disables my monitor/capture FrameStore
//		-	upon termination:
//			-	ensures my monitor/capture AutoCirculate channel is stopped
//			-	clears my input widget routing
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
		RouteInputSignal();

		AUTOCIRCULATE_STATUS acStatus;
		ULWord fbNum (10);
		mDevice.AutoCirculateStop(mConfig.fInputChannel);
		if (mDevice.AutoCirculateInitForInput(mConfig.fInputChannel, 2) && mDevice.AutoCirculateGetStatus(mConfig.fInputChannel, acStatus))	//	Which buffers did we get?
			fbNum = ULWord(acStatus.acStartFrame);	//	Use them
		else
			{cerr << "## NOTE:  Input A/C allocate device frame buffer range failed" << endl;	return;}
		mDevice.SetInputFrame (mConfig.fInputChannel, fbNum);

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
			NTV2LHIHDMIColorSpace colorSpace (NTV2_LHIHDMIColorSpaceYCbCr);
			const NTV2VideoFormat vf (mDevice.GetInputVideoFormat(mConfig.fInputSource));
			const bool vfChanged(vf != mVideoFormat);
			const bool csChanged(NTV2IOKind(::GetNTV2InputSourceKind(mConfig.fInputSource)) == NTV2_IOKINDS_HDMI
								&& mDevice.GetHDMIInputColor(colorSpace, mConfig.fInputChannel)
								&& colorSpace != mHDMIColorSpace);
			if (vfChanged)
			{
				++vfChgTally;
				OVRLWARN("Signal format at " << ::NTV2InputSourceToString(mConfig.fInputSource) << " changed from '"
							<< ::NTV2VideoFormatToString(mVideoFormat) << "' to '" << ::NTV2VideoFormatToString(vf) << "'");
				cout << "## NOTE: Signal format at " << ::NTV2InputSourceToString(mConfig.fInputSource) << " changed from "
							<< ::NTV2VideoFormatToString(mVideoFormat) << " to " << ::NTV2VideoFormatToString(vf) << endl;
			}
			if (csChanged)
			{
				OVRLWARN("HDMI colorspace at " << ::NTV2InputSourceToString(mConfig.fInputSource) << " changed from "
							<< (mHDMIColorSpace?"RGB":"YUV") << " to " << (colorSpace?"RGB":"YUV"));
				cout << "## NOTE: " << ::NTV2InputSourceToString(mConfig.fInputSource) << " colorspace changed from "
							<< (mHDMIColorSpace?"RGB":"YUV") << " to " << (colorSpace?"RGB":"YUV");
			}
			if (vfChanged || csChanged)
			{
				mDevice.AutoCirculateStop(mConfig.fOutputChannel);	//	This signals output thread to exit
				mDevice.AutoCirculateStop(mConfig.fInputChannel);	//	Stop input A/C
				mDevice.DisableChannel(mConfig.fInputChannel);		//	Disable input FrameStore
				break;	//	exit inner loop
			}	//	if incoming video format changed
		}	//	inner loop -- until signal change
	}	//	loop til quit signaled
	mDevice.AutoCirculateStop(mConfig.fInputChannel);
	mDevice.RemoveConnections(mInputConnections);
	OVRLNOTE("Completed: " << DEC(vfChgTally) << " signal changes, " << DEC(waitTally) << " good waits, " << DEC(badWaits) << " bad waits");

}	//	InputThread

