/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2outputtestpattern.cpp
	@brief		Implementation of NTV2OutputTestPattern demonstration class.
	@copyright	(C) 2013-2022 AJA Video Systems, Inc.  All rights reserved.
**/


//	Includes
#include "ntv2outputtestpattern.h"
#include "ntv2devicescanner.h"
#include "ntv2testpatterngen.h"
#include "ajabase/system/process.h"

#define	AsULWordPtr(_p_)	reinterpret_cast<const ULWord*>	(_p_)


using namespace std;


const uint32_t	kAppSignature	(NTV2_FOURCC('T','e','s','t'));


AJALabelValuePairs TestPatConfig::Get (const bool inCompact) const
{
	AJALabelValuePairs result (PlayerConfig::Get (inCompact));
	AJASystemInfo::append(result, "Background Pattern",	fTestPatternName);
	return result;
}

std::ostream & operator << (std::ostream & ioStrm,  const TestPatConfig & inObj)
{
	return ioStrm << AJASystemInfo::ToString(inObj.Get());
}


NTV2OutputTestPattern::NTV2OutputTestPattern (const TestPatConfig & inConfig)
	:	mConfig				(inConfig),
		mDeviceID			(DEVICE_ID_NOTFOUND),
		mSavedTaskMode		(NTV2_TASK_MODE_INVALID),
		mSavedConnections	()
{
}	//	constructor


NTV2OutputTestPattern::~NTV2OutputTestPattern ()
{
	if (!mConfig.fDoMultiFormat)
		mDevice.ApplySignalRoute(mSavedConnections, /*replace?*/true);	//	Restore prior widget routing

	//	Restore the prior service level, and release the device...
	mDevice.SetEveryFrameServices(mSavedTaskMode);
	mDevice.ReleaseStreamForApplication (kAppSignature, static_cast<int32_t>(AJAProcess::GetPid()));

}	//	destructor


AJAStatus NTV2OutputTestPattern::Init (void)
{
	//	Open the device...
	if (!CNTV2DeviceScanner::GetFirstDeviceFromArgument (mConfig.fDeviceSpec, mDevice))
		{cerr << "## ERROR:  Device '" << mConfig.fDeviceSpec << "' not found" << endl;  return AJA_STATUS_OPEN;}
	mDeviceID = mDevice.GetDeviceID();	//	Keep this ID handy -- it's used frequently

    if (!mDevice.IsDeviceReady(false))
		{cerr << "## ERROR:  Device '" << mDevice.GetDisplayName() << "' not ready" << endl;  return AJA_STATUS_INITIALIZE;}
	if (!mDevice.features().CanDoPlayback())
		{cerr << "## ERROR:  '" << mDevice.GetDisplayName() << "' is capture-only" << endl;  return AJA_STATUS_FEATURE;}

	const UWord maxNumChannels (mDevice.features().GetNumFrameStores());

	if ((mConfig.fOutputChannel == NTV2_CHANNEL1) && (!mDevice.features().CanDoFrameStore1Display()))
	{	//	Some older devices (e.g. Corvid1) can only output from FrameStore 2...
		mConfig.fOutputChannel = NTV2_CHANNEL2;
		cerr << "## WARNING:  '" << mDevice.GetDisplayName() << "' switched to Ch2 (Ch1 is input-only)" << endl;
	}
	if (UWord(mConfig.fOutputChannel) >= maxNumChannels)
	{
		cerr	<< "## ERROR:  '" << mDevice.GetDisplayName() << "' can't use Ch" << DEC(mConfig.fOutputChannel+1)
				<< " -- only supports Ch1" << (maxNumChannels > 1  ?  string("-Ch") + string(1, char(maxNumChannels+'0'))  :  "") << endl;
		return AJA_STATUS_UNSUPPORTED;
	}

	mDevice.GetEveryFrameServices(mSavedTaskMode);		//	Save current task mode
	if (!mConfig.fDoMultiFormat)
	{
		mDevice.GetConnections(mSavedConnections);		//	Save current routing, so it can be restored later
		if (!mDevice.AcquireStreamForApplication (kDemoAppSignature, int32_t(AJAProcess::GetPid())))
			return AJA_STATUS_BUSY;		//	Device is in use by another app -- fail
	}
	mDevice.SetEveryFrameServices(NTV2_OEM_TASKS);		//	Set OEM service level

	if (mDevice.features().CanDoMultiFormat())
		mDevice.SetMultiFormatMode(mConfig.fDoMultiFormat);
	else
		mConfig.fDoMultiFormat = false;

	//  Set up the desired video configuration...
	AJAStatus status (SetUpVideo());
	if (AJA_FAILURE(status))
		return status;

	#if defined(_DEBUG)
		AJALabelValuePairs info(mConfig.Get());
		if (!mDevice.GetDescription().empty())
			for (AJALabelValuePairsConstIter it(info.begin());  it != info.end();  ++it)
				if (it->first.find("Device Specifier") == 0)
					{info.insert(++it, AJALabelValuePair("Device Description", mDevice.GetDescription())); break;}
		cerr << AJASystemInfo::ToString(info);
	#endif	//	defined(_DEBUG)
	return AJA_STATUS_SUCCESS;

}	//	Init


AJAStatus NTV2OutputTestPattern::SetUpVideo (void)
{
 	if (mConfig.fVideoFormat == NTV2_FORMAT_UNKNOWN)
	{
		//	User didn't specify a video format.
		//  Get the device's current video format and use it to create the test pattern...
		bool enabled(false);
		mDevice.IsChannelEnabled(mConfig.fOutputChannel, enabled);
		if (!enabled)
		if (!mDevice.GetVideoFormat (mConfig.fVideoFormat, mConfig.fOutputChannel))
			return AJA_STATUS_FAIL;

		//  Read the current VANC mode, as this can affect the NTV2FormatDescriptor and host frame buffer size...
		if (!mDevice.GetVANCMode (mConfig.fVancMode, mConfig.fOutputChannel))
			return AJA_STATUS_FAIL;
	}

	if (mConfig.fVideoFormat != NTV2_FORMAT_UNKNOWN)
	{
		//	User specified a video format -- is it legal for this device?
		if (!mDevice.features().CanDoVideoFormat(mConfig.fVideoFormat))
		{	cerr << "## ERROR: '" << mDevice.GetDisplayName() << "' cannot do " << ::NTV2VideoFormatToString(mConfig.fVideoFormat) << endl;
			return AJA_STATUS_UNSUPPORTED;
		}

		//	Set the video format -- is it legal for this device?
		if (!mDevice.SetVideoFormat (mConfig.fVideoFormat, /*retail?*/false, /*keepVANC*/false, mConfig.fOutputChannel))
		{	cerr << "## ERROR: SetVideoFormat '" << ::NTV2VideoFormatToString(mConfig.fVideoFormat) << "' failed" << endl;
			return AJA_STATUS_FAIL;
		}

		//	Set the VANC mode
		if (!mDevice.SetEnableVANCData (NTV2_IS_VANCMODE_ON(mConfig.fVancMode), NTV2_IS_VANCMODE_TALLER(mConfig.fVancMode), mConfig.fOutputChannel))
		{	cerr << "## ERROR: SetEnableVANCData '" << ::NTV2VANCModeToString(mConfig.fVancMode,true) << "' failed" << endl;
			return AJA_STATUS_FAIL;
		}
	}

	//	4K/UHD is allowed, but only for devices that can do 12G routing...
	if (NTV2_IS_4K_VIDEO_FORMAT(mConfig.fVideoFormat) && !mDevice.features().CanDo12gRouting())
	{	cerr << "## ERROR: '" << ::NTV2VideoFormatToString(mConfig.fVideoFormat) << "' requires 12G routing, but '" << mDevice.GetDisplayName() << "' doesn't support it" << endl;
		return AJA_STATUS_UNSUPPORTED;
	}
	//	This demo won't do 8K/UHD2
	if (NTV2_IS_8K_VIDEO_FORMAT(mConfig.fVideoFormat))
	{	cerr << "## ERROR: This demo only supports SD/HD/2K1080, not '" << ::NTV2VideoFormatToString(mConfig.fVideoFormat) << "'" << endl;
		return AJA_STATUS_UNSUPPORTED;
	}

	//	This is a "playback" application, so set the board reference to free run...
	if (!mDevice.SetReference(NTV2_REFERENCE_FREERUN))
		return AJA_STATUS_FAIL;

	//	Set the FrameStore's pixel format...
	if (!mDevice.SetFrameBufferFormat (mConfig.fOutputChannel, mConfig.fPixelFormat))
		return AJA_STATUS_FAIL;

	//	Enable the FrameStore (if currently disabled)...
	mDevice.EnableChannel(mConfig.fOutputChannel);
	if (!mConfig.fDoMultiFormat)
	{	//	Disable all other FrameStores...
		NTV2ChannelSet frmStores (::NTV2MakeChannelSet (NTV2_CHANNEL1, mDevice.features().GetNumFrameStores()));
		frmStores.erase(frmStores.find(mConfig.fOutputChannel));
		mDevice.DisableChannels(frmStores);
	}

	//	Set the FrameStore mode to "playout" (not capture)...
	if (!mDevice.SetMode (mConfig.fOutputChannel, NTV2_MODE_DISPLAY))
		return AJA_STATUS_FAIL;

	//	Enable SDI output from the channel being used, but only if the device supports bi-directional SDI...
	if (mDevice.features().HasBiDirectionalSDI())
		mDevice.SetSDITransmitEnable (mConfig.fOutputChannel, true);

	return AJA_STATUS_SUCCESS;

}	//	SetUpVideo


void NTV2OutputTestPattern::RouteOutputSignal (void)
{
	NTV2XptConnections connections;
	const bool isRGBPixFormat (::IsRGBFormat(mConfig.fPixelFormat));

	//	Build a set of crosspoint connections (input-to-output)
	//	between the relevant signal processing widgets on the device.
	//	By default, the main output crosspoint that feeds the output widget(s) is the FrameStore's video output:
	NTV2OutputXptID	sourceXpt (::GetFrameStoreOutputXptFromChannel(mConfig.fOutputChannel, isRGBPixFormat));
	if (isRGBPixFormat)
	{	//	This code block allows this demo to work with RGB frame buffers, which
		//	necessitate inserting a CSC between the FrameStore and the video output(s)...
		NTV2Connection csc_framestore (::GetCSCInputXptFromChannel(mConfig.fOutputChannel), sourceXpt);
		connections.insert(csc_framestore);	//	CSC video input to FrameStore output
		sourceXpt = ::GetCSCOutputXptFromChannel(mConfig.fOutputChannel);	//	CSC output will feed all output widget(s)
	}

	//	Route sourceXpt to SDI output(s)...
	const NTV2ChannelSet sdiOutputs (mConfig.fDoMultiFormat ? ::NTV2MakeChannelSet (mConfig.fOutputChannel, 1)
															: ::NTV2MakeChannelSet (NTV2_CHANNEL1,
																					mDevice.features().GetNumVideoOutputs()));
	const NTV2Standard videoStd (::GetNTV2StandardFromVideoFormat(mConfig.fVideoFormat));
	const NTV2ChannelList sdiOuts (::NTV2MakeChannelList(sdiOutputs));

	//	For every SDI output, set video standard, and disable level A/B conversion...
	mDevice.SetSDIOutputStandard (sdiOutputs, videoStd);
	mDevice.SetSDIOutLevelAtoLevelBConversion (sdiOutputs, false);
	mDevice.SetSDIOutRGBLevelAConversion (sdiOutputs, false);

	//	Connect each SDI output to the main output crosspoint...
	bool canCheck(mDevice.features().HasCrosspointConnectROM());
	for (size_t ndx(0);  ndx < sdiOutputs.size();  ndx++)
	{
		const NTV2Connection sdiConnection(::GetSDIOutputInputXpt(sdiOuts.at(ndx)), sourceXpt);
		bool canConnect(true);
		if (!canCheck  ||  (canCheck && mDevice.CanConnect(sdiConnection, canConnect) && canConnect))
			connections.insert(sdiConnection);
		if (mDevice.features().HasBiDirectionalSDI())			//	If device has bi-directional SDIs...
			mDevice.SetSDITransmitEnable (sdiOutputs, true);	//	... be sure it's set to "transmit"
	}

	//	Add analog video output, if the device has one...
	if (!mConfig.fDoMultiFormat  &&  mDevice.features().GetNumAnalogVideoOutputs())
		connections.insert(NTV2Connection(::GetOutputDestInputXpt(NTV2_OUTPUTDESTINATION_ANALOG1), sourceXpt));

	//	Add HDMI video output, if the device has one...
	if (!mConfig.fDoMultiFormat  &&  mDevice.features().GetNumHDMIVideoOutputs())
		connections.insert(NTV2Connection(::GetOutputDestInputXpt(NTV2_OUTPUTDESTINATION_HDMI1), sourceXpt));

	//	Apply all the accumulated connections...
	mDevice.ApplySignalRoute(connections, /*replaceExistingRoutes*/true);

}	//	RouteOutputSignal


AJAStatus NTV2OutputTestPattern::EmitPattern (void)
{
	//  Connect the FrameStore to the video output...
	RouteOutputSignal();

	//	Allocate a host video buffer that will hold our test pattern raster...
	NTV2FormatDescriptor fd	(mConfig.fVideoFormat, mConfig.fPixelFormat, mConfig.fVancMode);
	NTV2Buffer hostBuffer (fd.GetTotalBytes());
	if (hostBuffer.IsNULL())
		return AJA_STATUS_MEMORY;

	//	Write the requested test pattern into host buffer...
	NTV2TestPatternGen	testPatternGen;
	testPatternGen.setVANCToLegalBlack(fd.IsVANC());
	if (!testPatternGen.DrawTestPattern (mConfig.fTestPatternName,	fd,	hostBuffer))
		return AJA_STATUS_FAIL;

	//	Find out which frame is currently being output from the frame store...
	uint32_t currentOutputFrame(0);
	if (!mDevice.GetOutputFrame (mConfig.fOutputChannel, currentOutputFrame))
		return AJA_STATUS_FAIL;	//	ReadRegister failure?

	//	Now simply transfer the contents of the host buffer to the device's current output frame...
	if (!mDevice.DMAWriteFrame (currentOutputFrame,				//	Device frame number
								AsULWordPtr(fd.GetRowAddress(hostBuffer.GetHostPointer(), 0)),	//	Host buffer address
								hostBuffer.GetByteCount()))		//	# bytes to xfer
		return AJA_STATUS_FAIL;

	return AJA_STATUS_SUCCESS;

}	//	EmitPattern
