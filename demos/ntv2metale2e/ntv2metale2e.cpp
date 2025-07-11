/* SPDX-License-Identifier: MIT */
/**
    @file		ntv2metale2e.cpp
	@brief		Implementation of NTV2OutputTestPattern demonstration class.
	@copyright	(C) 2013-2022 AJA Video Systems, Inc.  All rights reserved.
**/


//	Includes
#include "ntv2metale2e.h"

NTV2MetalE2E::NTV2MetalE2E ()
{
}	//	constructor


NTV2MetalE2E::~NTV2MetalE2E ()
{
}	//	destructor


AJAStatus NTV2MetalE2E::DoSomething (void)
{
    mDevice.Open(0);
	NTV2DeviceID mDeviceID = mDevice.GetDeviceID();	//	Keep this ID handy -- it's used frequently

    //if (mDeviceID != DEVICE_ID_KONAX)
        //return AJA_STATUS_FAIL;
	
	//	Set up the Genlock circuit
    AJAStatus status (SetUpGenlock());
		if (AJA_FAILURE(status))
            return status;
	
	//	Set up the E2E routing
	RouteE2ESignal();
	
	//  Set up the desired video configuration...
	status = SetUpVideo();
	if (AJA_FAILURE(status))
		return status;


	return AJA_STATUS_SUCCESS;

}	//	Init

AJAStatus NTV2MetalE2E::SetUpGenlock (void)
{
	//  Port driver genlock
    return AJA_STATUS_SUCCESS;
	
}

AJAStatus NTV2MetalE2E::SetUpVideo (void)
{
	mDevice.SetReference(NTV2_REFERENCE_INPUT1);
	mDevice.EnableChannel(NTV2_CHANNEL1);
	mDevice.EnableChannel(NTV2_CHANNEL2);
	
	NTV2VideoFormat videoFormat = mDevice.GetInputVideoFormat (NTV2_INPUTSOURCE_SDI1);
	mDevice.SetVideoFormat (videoFormat, false, false, NTV2_CHANNEL1);
	mDevice.SetVideoFormat (videoFormat, false, false, NTV2_CHANNEL2);
	
	NTV2Standard videoStandard = GetNTV2StandardFromVideoFormat(videoFormat);
	mDevice.SetSDIOutputStandard(NTV2_CHANNEL2, videoStandard);
	
    bool isValidVPID (mDevice.GetVPIDValidA(NTV2_CHANNEL1));
	if (isValidVPID)
	{
		CNTV2VPID inputVPID;
		ULWord vpidDS1(0), vpidDS2(0);
		mDevice.ReadSDIInVPID(NTV2_CHANNEL1, vpidDS1, vpidDS2);
		inputVPID.SetVPID(vpidDS1);
		isValidVPID = inputVPID.IsValid();
		mDevice.SetSDIOutVPID(vpidDS1, vpidDS2, NTV2_CHANNEL2);
	}
	
	bool is3G = false, is6G = false, is12G = false;
	mDevice.GetSDIInput3GPresent(is3G, NTV2_CHANNEL1);
	mDevice.GetSDIInput6GPresent(is6G, NTV2_CHANNEL1);
	mDevice.GetSDIInput12GPresent(is12G, NTV2_CHANNEL1);
	
	mDevice.SetSDIOut3GEnable(NTV2_CHANNEL2, is3G);
	mDevice.SetSDIOut6GEnable(NTV2_CHANNEL2, is6G);
	mDevice.SetSDIOut12GEnable(NTV2_CHANNEL2, is12G);
	
	return AJA_STATUS_SUCCESS;
	
}	//	SetUpVideo

void NTV2MetalE2E::RouteE2ESignal (void)
{
    mDevice.SetSDITransmitEnable (NTV2_CHANNEL1, false);
    mDevice.SetSDITransmitEnable (NTV2_CHANNEL2, true);
    mDevice.ClearRouting();
    mDevice.Connect(NTV2_XptSDIOut2Input, NTV2_XptSDIIn1);

}	//	RouteOutputSignal




