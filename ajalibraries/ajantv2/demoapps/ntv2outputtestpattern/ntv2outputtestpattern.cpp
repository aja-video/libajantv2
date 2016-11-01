/**
	@file		ntv2outputtestpattern.cpp
	@brief		Implementation of NTV2OutputTestPattern demonstration class.
	@copyright	Copyright (C) 2013-2016 AJA Video Systems, Inc.  All rights reserved.
**/


//	Includes
#include "ajastuff/common/types.h"
#include "ajastuff/system/process.h"
#include "ajastuff/common/testpatterngen.h"
#include "ajastuff/common/options_popt.h"
#include "ntv2card.h"
#include "ntv2outputtestpattern.h"
#include "ntv2utils.h"
#include "ntv2democommon.h"
#include <signal.h>
#include <iostream>
#include <iomanip>


using namespace std;


const uint32_t	kAppSignature	(AJA_FOURCC ('T','e','s','t'));


NTV2OutputTestPattern::NTV2OutputTestPattern (const string &	inDeviceSpecifier,
											  const NTV2Channel	channel)
	:	mDeviceID			(DEVICE_ID_NOTFOUND),
		mDeviceSpecifier	(inDeviceSpecifier),
		mOutputChannel		(channel),
		mVideoFormat		(NTV2_FORMAT_UNKNOWN),
		mPixelFormat		(NTV2_FBF_8BIT_YCBCR),
		mVancEnabled		(false),
		mWideVanc			(false)
{
}	//	constructor


NTV2OutputTestPattern::~NTV2OutputTestPattern ()
{
	//	Restore the prior service level...
	mDevice.SetEveryFrameServices (mSavedTaskMode);													//	Restore prior service level
	mDevice.ReleaseStreamForApplication (kAppSignature, static_cast <uint32_t> (AJAProcess::GetPid ()));	//	Release the device

}	//	destructor


AJAStatus NTV2OutputTestPattern::Init ()
{
	//	Open the board...
	if (!CNTV2DeviceScanner::GetFirstDeviceFromArgument (mDeviceSpecifier, mDevice))
		{cerr << "## ERROR:  Device '" << mDeviceSpecifier << "' not found" << endl;  return AJA_STATUS_OPEN;}

	if (!mDevice.IsDeviceReady ())
		{cerr << "## ERROR:  Device '" << mDeviceSpecifier << "' not ready" << endl;  return AJA_STATUS_INITIALIZE;}

	if (!mDevice.AcquireStreamForApplication (kAppSignature, static_cast <uint32_t> (AJAProcess::GetPid ())))
	{
		cerr << "## ERROR:  Unable to acquire device because another app owns it" << endl;
		return AJA_STATUS_BUSY;		//	Some other app is using the device
	}

	mDevice.GetEveryFrameServices (mSavedTaskMode);		//	Save the current state before changing it
	mDevice.SetEveryFrameServices (NTV2_OEM_TASKS);		//	Since this is an OEM demo, use the OEM service level

	mDeviceID = mDevice.GetDeviceID ();		//	Keep this handy since it will be used frequently...

	//	At this point, the device can be used without fear of messing up another application.
	//	The only way to relinquish the device to another application is to manually set the
	//	task mode back to "Standard Tasks" using the 'cables' utility (in the "Control" tab).

	//  Get the board's current video format and use it later for writing the test pattern...
	if (!mDevice.GetVideoFormat (mVideoFormat, mOutputChannel))
		return AJA_STATUS_FAIL;

	//  Read the current VANC settings so an NTV2FormatDescriptor can be constructed...
	if (!mDevice.GetEnableVANCData (mVancEnabled, mWideVanc, mOutputChannel))
		return AJA_STATUS_FAIL;

	return AJA_STATUS_SUCCESS;

}	//	Init


AJAStatus NTV2OutputTestPattern::SetUpVideo (void)
{
	//	This is a "playback" application, so set the board reference to free run...
	if (!mDevice.SetReference (NTV2_REFERENCE_FREERUN))
		return AJA_STATUS_FAIL;

	//	Set the video format for the channel's Frame Store to 8-bit YCbCr...
	if (!mDevice.SetFrameBufferFormat (mOutputChannel, mPixelFormat))
		return AJA_STATUS_FAIL;

	//	Enable the Frame Buffer, just in case it's not currently enabled...
	mDevice.EnableChannel (mOutputChannel);

	//	Set the channel mode to "playout" (not capture)...
	if (!mDevice.SetMode (mOutputChannel, NTV2_MODE_DISPLAY))
		return AJA_STATUS_FAIL;

	//	Enable SDI output from the channel being used, but only if the device supports bi-directional SDI...
	if (::NTV2DeviceHasBiDirectionalSDI (mDeviceID))
		mDevice.SetSDITransmitEnable (mOutputChannel, true);

	return AJA_STATUS_SUCCESS;

}	//	SetUpVideo


void NTV2OutputTestPattern::RouteOutputSignal (void)
{
	const NTV2Standard	videoStandard	(::GetNTV2StandardFromVideoFormat (mVideoFormat));
	const NTV2InputCrosspointID		outputInputXpt	(::GetOutputDestInputXpt (::NTV2ChannelToOutputDestination (mOutputChannel)));
	const NTV2OutputCrosspointID	fbOutputXpt		(::GetFrameBufferOutputXptFromChannel (mOutputChannel, ::IsRGBFormat (mPixelFormat)));
	NTV2OutputCrosspointID			outputXpt		(fbOutputXpt);

	if (::IsRGBFormat (mPixelFormat))
	{
		const NTV2OutputCrosspointID	cscVidOutputXpt	(::GetCSCOutputXptFromChannel (mOutputChannel, false, true));
		const NTV2InputCrosspointID		cscVidInputXpt	(::GetCSCInputXptFromChannel (mOutputChannel));

		mDevice.Connect (cscVidInputXpt, fbOutputXpt);		//	Connect the CSC's video input to the frame store's output
		mDevice.Connect (outputInputXpt, cscVidOutputXpt);	//	Connect the SDI output's input to the CSC's video output
		outputXpt = cscVidOutputXpt;
	}
	else
		mDevice.Connect (outputInputXpt, outputXpt);

	//	Route all SDI outputs to the outputXpt...
	const NTV2Channel	startNum		(NTV2_CHANNEL1);
	const NTV2Channel	endNum			(NTV2Channel (::NTV2DeviceGetNumVideoChannels (mDeviceID)));
	NTV2WidgetID		outputWidgetID	(NTV2_WIDGET_INVALID);

	for (NTV2Channel chan (startNum);  chan < endNum;  chan = NTV2Channel (chan + 1))
	{
		mDevice.SetRP188Source (chan, 0);	//	Set all SDI spigots to capture embedded LTC (VITC could be an option)

		if (chan == mOutputChannel)
			continue;	//	Skip the input & output channel, already routed
		if (::NTV2DeviceHasBiDirectionalSDI (mDeviceID))
			mDevice.SetSDITransmitEnable (chan, true);
		if (CNTV2SignalRouter::GetWidgetForInput (::GetSDIOutputInputXpt (chan, ::NTV2DeviceCanDoDualLink (mDeviceID)), outputWidgetID))
			if (::NTV2DeviceCanDoWidget (mDeviceID, outputWidgetID))
				mDevice.Connect (::GetSDIOutputInputXpt (chan), outputXpt);
	}	//	for each output spigot

	//	If HDMI and/or analog video outputs are available, route them, too...
	if (::NTV2DeviceCanDoWidget (mDeviceID, NTV2_WgtHDMIOut1))
		mDevice.Connect (NTV2_XptHDMIOutInput, outputXpt);			//	Route the output signal to the HDMI output
	if (::NTV2DeviceCanDoWidget (mDeviceID, NTV2_WgtHDMIOut1v2))
		mDevice.Connect (NTV2_XptHDMIOutQ1Input, outputXpt);		//	Route the output signal to the HDMI output
	if (::NTV2DeviceCanDoWidget (mDeviceID, NTV2_WgtAnalogOut1))
		mDevice.Connect (NTV2_XptAnalogOutInput, outputXpt);		//	Route the output signal to the Analog output
	if (::NTV2DeviceCanDoWidget (mDeviceID, NTV2_WgtSDIMonOut1))
		mDevice.Connect (::GetSDIOutputInputXpt (NTV2_CHANNEL5), outputXpt);	//	Route the output signal to the SDI monitor output

	mDevice.SetSDIOutputStandard (mOutputChannel, videoStandard);

}	//	RouteOutputSignal


AJAStatus NTV2OutputTestPattern::EmitPattern (const UWord testPatternIndex)
{
	AJAStatus	status	(AJA_STATUS_SUCCESS);

	//  Set up the desired video configuration...
	status = SetUpVideo ();

	//  Connect the frame buffer to the video output...
	if (AJA_SUCCESS (status))
		RouteOutputSignal ();

	//	Get information about current video format...
	NTV2FormatDescriptor	fd	(GetFormatDescriptor (mVideoFormat, mPixelFormat, mVancEnabled, mWideVanc));

	//	Write the requested test pattern into host buffer...
	AJATestPatternGen		testPatternGen;
	AJATestPatternBuffer	testPatternBuffer;
	testPatternGen.DrawTestPattern ((AJATestPatternSelect) testPatternIndex,
									fd.numPixels,
									fd.numLines,
									CNTV2DemoCommon::GetAJAPixelFormat (mPixelFormat),
									testPatternBuffer);

	//	Find out which frame is currently being output from the frame store...
	uint32_t	currentOutputFrame	(0);
	if (AJA_SUCCESS (status))
		if (!mDevice.GetOutputFrame (mOutputChannel, currentOutputFrame))
			status = AJA_STATUS_FAIL;

	//	Now simply DMA the test pattern buffer to the currentOutputFrame...
	if (AJA_SUCCESS (status))
		if (!mDevice.DMAWriteFrame (currentOutputFrame,  reinterpret_cast <uint32_t *> (&testPatternBuffer[0]),  uint32_t (testPatternBuffer.size ())))
			status = AJA_STATUS_FAIL;

	return status;

}	//	EmitPattern
