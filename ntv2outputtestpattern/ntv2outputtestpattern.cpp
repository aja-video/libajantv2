/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2outputtestpattern.cpp
	@brief		Implementation of NTV2OutputTestPattern demonstration class.
	@copyright	(C) 2013-2022 AJA Video Systems, Inc.  All rights reserved.
**/


//	Includes
#include "ntv2outputtestpattern.h"
#include "ntv2devicescanner.h"
#include "ntv2democommon.h"	//	Also includes ntv2testpatterngen.h
#include "ajabase/system/process.h"
#include "ajabase/common/options_popt.h"
#include <signal.h>
#include <iostream>
#include <iomanip>

#define	AsULWordPtr(_p_)	reinterpret_cast<const ULWord*>	(_p_)


using namespace std;


const uint32_t	kAppSignature	(NTV2_FOURCC('T','e','s','t'));


NTV2OutputTestPattern::NTV2OutputTestPattern (	const std::string &		inDeviceSpecifier,
												const std::string &		inTestPatternSpec,
												const NTV2VideoFormat	inVideoFormat,
												const NTV2PixelFormat	inPixelFormat,
												const NTV2Channel		inChannel,
												const NTV2VANCMode		inVancMode)
	:	mDeviceID			(DEVICE_ID_NOTFOUND),
		mDeviceSpecifier	(inDeviceSpecifier),
		mTestPatternSpec	(inTestPatternSpec.empty() ? "100% ColorBars" : inTestPatternSpec),
		mOutputChannel		(inChannel),
		mPixelFormat		(inPixelFormat),
		mVideoFormat		(inVideoFormat),
		mSavedTaskMode		(NTV2_TASK_MODE_INVALID),
		mSavedConnections	(),
		mVancMode			(inVancMode)
{
}	//	constructor


NTV2OutputTestPattern::~NTV2OutputTestPattern ()
{
	//	Restore prior widget routing...
	mDevice.ApplySignalRoute(mSavedConnections, /*replace?*/true);

	//	Restore the prior service level, and release the device...
	mDevice.SetEveryFrameServices(mSavedTaskMode);
	mDevice.ReleaseStreamForApplication (kAppSignature, static_cast<int32_t>(AJAProcess::GetPid()));

}	//	destructor


AJAStatus NTV2OutputTestPattern::Init (void)
{
	//	Open the board...
	if (!CNTV2DeviceScanner::GetFirstDeviceFromArgument (mDeviceSpecifier, mDevice))
		{cerr << "## ERROR:  Device '" << mDeviceSpecifier << "' not found" << endl;  return AJA_STATUS_OPEN;}

    if (!mDevice.IsDeviceReady(false))
		{cerr << "## ERROR:  Device '" << mDeviceSpecifier << "' not ready" << endl;  return AJA_STATUS_INITIALIZE;}

	if (!mDevice.AcquireStreamForApplication (kAppSignature, static_cast<int32_t>(AJAProcess::GetPid())))
	{
		cerr << "## ERROR:  Unable to acquire device because another app owns it" << endl;
		return AJA_STATUS_BUSY;		//	Some other app is using the device
	}

	mDeviceID = mDevice.GetDeviceID();				//	Keep this handy, since it will be used frequently
	mDevice.GetEveryFrameServices(mSavedTaskMode);	//	Save current task mode, so it can be restored later
	mDevice.SetEveryFrameServices(NTV2_OEM_TASKS);	//	Since this is an OEM demo, so use the OEM tasks mode
	mDevice.GetConnections(mSavedConnections);		//	Save current routing, so it can be restored later

	if (mVideoFormat != NTV2_FORMAT_UNKNOWN)
	{
		//	User specified a video format -- is it legal for this device?
		if (!::NTV2DeviceCanDoVideoFormat(mDeviceID, mVideoFormat))
		{	cerr << "## ERROR: '" << mDevice.GetDisplayName() << "' cannot do " << ::NTV2VideoFormatToString(mVideoFormat) << endl;
			return AJA_STATUS_UNSUPPORTED;
		}

		//	Set the video format -- is it legal for this device?
		if (!mDevice.SetVideoFormat (mVideoFormat, /*retail?*/false, /*keepVANC*/false, mOutputChannel))
		{	cerr << "## ERROR: SetVideoFormat '" << ::NTV2VideoFormatToString(mVideoFormat) << "' failed" << endl;
			return AJA_STATUS_FAIL;
		}

		//	Set the VANC mode
		if (!mDevice.SetEnableVANCData (NTV2_IS_VANCMODE_ON(mVancMode), NTV2_IS_VANCMODE_TALLER(mVancMode), mOutputChannel))
		{	cerr << "## ERROR: SetEnableVANCData '" << ::NTV2VANCModeToString(mVancMode,true) << "' failed" << endl;
			return AJA_STATUS_FAIL;
		}
	}
	else
	{
		//	User didn't specify a video format.
		//  Get the device's current video format and use it to create the test pattern...
		if (!mDevice.GetVideoFormat (mVideoFormat, mOutputChannel))
			return AJA_STATUS_FAIL;

		//  Read the current VANC mode, as this can affect the NTV2FormatDescriptor and host frame buffer size...
		if (!mDevice.GetVANCMode (mVancMode, mOutputChannel))
			return AJA_STATUS_FAIL;
	}

	//	SD/HD/2K only -- no 4K or 8K...
	if (NTV2_IS_4K_VIDEO_FORMAT(mVideoFormat) ||  NTV2_IS_8K_VIDEO_FORMAT(mVideoFormat))
	{	cerr << "## ERROR: This demo only supports SD/HD/2K1080, not '" << ::NTV2VideoFormatToString(mVideoFormat) << "'" << endl;
		return AJA_STATUS_UNSUPPORTED;
	}

	return AJA_STATUS_SUCCESS;

}	//	Init


AJAStatus NTV2OutputTestPattern::SetUpVideo (void)
{
	//	This is a "playback" application, so set the board reference to free run...
	if (!mDevice.SetReference(NTV2_REFERENCE_FREERUN))
		return AJA_STATUS_FAIL;

	//	Set the FrameStore's pixel format...
	if (!mDevice.SetFrameBufferFormat (mOutputChannel, mPixelFormat))
		return AJA_STATUS_FAIL;

	//	Enable the FrameStore (if currently disabled)...
	mDevice.EnableChannel(mOutputChannel);

	//	Set the FrameStore mode to "playout" (not capture)...
	if (!mDevice.SetMode (mOutputChannel, NTV2_MODE_DISPLAY))
		return AJA_STATUS_FAIL;

	//	Enable SDI output from the channel being used, but only if the device supports bi-directional SDI...
	if (::NTV2DeviceHasBiDirectionalSDI(mDeviceID))
		mDevice.SetSDITransmitEnable (mOutputChannel, true);

	return AJA_STATUS_SUCCESS;

}	//	SetUpVideo


void NTV2OutputTestPattern::RouteOutputSignal (void)
{
	NTV2XptConnections	connections;

	//	Build a set of crosspoint connections (input-to-output)
	//	between the relevant signal processing widgets on the device.
	//	By default, the main output crosspoint that feeds the SDI output widgets is the FrameStore's video output:
	NTV2OutputXptID		outputXpt (::GetFrameBufferOutputXptFromChannel(mOutputChannel, ::IsRGBFormat(mPixelFormat)));
	if (::IsRGBFormat(mPixelFormat))
	{	//	Even though this demo defaults to using an 8-bit YUV frame buffer,
		//	this code block allows it to work with RGB frame buffers, which
		//	necessitate inserting a CSC between the FrameStore and the SDI output(s)...
		connections.insert(NTV2Connection(::GetCSCInputXptFromChannel(mOutputChannel), outputXpt));	//	CSC video input to FrameStore output
		outputXpt = ::GetCSCOutputXptFromChannel(mOutputChannel);	//	The CSC output feeds all SDIOut inputs
	}

	//	Route all SDI outputs to the outputXpt...
	const NTV2ChannelSet	sdiOutputs	(::NTV2MakeChannelSet(NTV2_CHANNEL1, ::NTV2DeviceGetNumVideoOutputs(mDeviceID)));
	const NTV2Standard		videoStd	(::GetNTV2StandardFromVideoFormat(mVideoFormat));
	const NTV2ChannelList	sdiOuts		(::NTV2MakeChannelList(sdiOutputs));

	//	Some devices have bi-directional SDI connectors...
	if (::NTV2DeviceHasBiDirectionalSDI(mDeviceID))
		mDevice.SetSDITransmitEnable (sdiOutputs, true);	//	Set to "transmit"

	//	For every SDI output, set video standard, and disable level A/B conversion...
	mDevice.SetSDIOutputStandard (sdiOutputs, videoStd);
	mDevice.SetSDIOutLevelAtoLevelBConversion(sdiOutputs, false);
	mDevice.SetSDIOutRGBLevelAConversion(sdiOutputs, false);

	//	Connect each SDI output to the main output crosspoint...
	for (size_t ndx(0);  ndx < sdiOutputs.size();  ndx++)
	{
		const NTV2Channel sdiOut(sdiOuts.at(ndx));
		connections.insert(NTV2Connection(::GetSDIOutputInputXpt(sdiOut), outputXpt));
	}

	//	And connect analog video output, if the device has one...
	if (::NTV2DeviceGetNumAnalogVideoOutputs(mDeviceID))
		connections.insert(NTV2Connection(::GetOutputDestInputXpt(NTV2_OUTPUTDESTINATION_ANALOG), outputXpt));

	//	And connect HDMI video output, if the device has one...
	if (::NTV2DeviceGetNumHDMIVideoOutputs(mDeviceID))
		connections.insert(NTV2Connection(::GetOutputDestInputXpt(NTV2_OUTPUTDESTINATION_HDMI), outputXpt));

	//	Apply all the accumulated connections...
	mDevice.ApplySignalRoute(connections, /*replaceExistingRoutes*/true);

}	//	RouteOutputSignal


AJAStatus NTV2OutputTestPattern::EmitPattern (void)
{
	//  Set up the desired video configuration...
	AJAStatus status (SetUpVideo());
	if (AJA_FAILURE(status))
		return status;

	//  Connect the FrameStore to the video output...
	RouteOutputSignal();

	//	Allocate a host video buffer that will hold our test pattern raster...
	NTV2FormatDescriptor fd	(mVideoFormat, mPixelFormat, mVancMode);
	NTV2_POINTER hostBuffer (fd.GetTotalBytes());
	if (hostBuffer.IsNULL())
		return AJA_STATUS_MEMORY;

	//	Write the requested test pattern into host buffer...
	NTV2TestPatternGen	testPatternGen;
	testPatternGen.setVANCToLegalBlack(fd.IsVANC());
	if (!testPatternGen.DrawTestPattern (mTestPatternSpec,	fd,	hostBuffer))
		return AJA_STATUS_FAIL;

	//	Find out which frame is currently being output from the frame store...
	uint32_t currentOutputFrame(0);
	if (!mDevice.GetOutputFrame (mOutputChannel, currentOutputFrame))
		return AJA_STATUS_FAIL;	//	ReadRegister failure?

	//	Now simply transfer the contents of the host buffer to the device's current output frame...
	if (!mDevice.DMAWriteFrame (currentOutputFrame,				//	Device frame number
								AsULWordPtr(fd.GetRowAddress(hostBuffer.GetHostPointer(), 0)),	//	Host buffer address
								hostBuffer.GetByteCount()))		//	# bytes to xfer
		return AJA_STATUS_FAIL;

	return AJA_STATUS_SUCCESS;

}	//	EmitPattern
