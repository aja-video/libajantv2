#include "gtest.h"
#include "ntv2endian.h"
#include "ntv2formatdescriptor.h"
#include "ajabase/common/types.h"
#include "ajabase/system/systemtime.h"
#include "ajaanc/includes/ancillarylist.h"
#include <iostream>

extern bool	gGlobalQuit;

using namespace std;

#define AsULWordPtr(_p_)		reinterpret_cast<ULWord*>(_p_)
#define AsCU8Ptr(_p_)			reinterpret_cast<const uint8_t*>(_p_)

GTest::GTest (const GTestConfig & inConfig)
	:	mConfig					(inConfig),
		mDeviceID				(DEVICE_ID_NOTFOUND),
		mVideoFormat			(NTV2_FORMAT_UNKNOWN),
		mSavedTaskMode			(NTV2_DISABLE_TASKS),
		mOutputDest				(NTV2_OUTPUTDESTINATION_INVALID)
{
}	//	constructor


GTest::~GTest ()
{
  mDevice.SetEveryFrameServices (mSavedTaskMode);										//	Restore prior service level
}	//	destructor


AJAStatus GTest::Init (void)
{
	AJAStatus	status	(AJA_STATUS_SUCCESS);

  printf("init called\n");
	//	Open the device...
	if (!CNTV2DeviceScanner::GetFirstDeviceFromArgument (mConfig.fDeviceSpec, mDevice))
		{cerr << "## ERROR:  Device '" << mConfig.fDeviceSpec << "' not found" << endl;  return AJA_STATUS_OPEN;}

    if (!mDevice.IsDeviceReady (false))
		{cerr << "## ERROR:  Device '" << mConfig.fDeviceSpec << "' not ready" << endl;  return AJA_STATUS_INITIALIZE;}

	mDeviceID = mDevice.GetDeviceID ();		//	Keep the device ID handy since it will be used frequently

	mDevice.GetEveryFrameServices (mSavedTaskMode);				//	Save the current device state
//  mDevice.SetEveryFrameServices (NTV2_OEM_TASKS);			//	Set the OEM service level
  mDevice.ClearRouting ();								//	Clear the current device routing (since I "own" the device)

	//	Set up the video
	status = SetupVideo();
	if (AJA_FAILURE (status))
		return status;

	mFormatDesc = NTV2FormatDescriptor (mVideoFormat, mConfig.fPixelFormat);

	//	Set up the buffer
	CNTV2DemoCommon::SetDefaultPageSize();	//	Set host-specific page size

	mpHostVideoBuffer.Allocate(mFormatDesc.GetVideoWriteSize(ULWord(NTV2Buffer::DefaultPageSize())), true);

	if (!mpHostVideoBuffer)
	{
		cerr << "## ERROR:  Unable to allocate host buffer(s)" << endl;
		return AJA_STATUS_MEMORY;
	}

	mDevice.SetOutputFrame	(mConfig.fOutputChannel, 2);

  while (!gGlobalQuit) {
    memset(mpHostVideoBuffer.GetHostPointer(), 0xff, 1000000);
    mDevice.DMAWriteFrame (2, mpHostVideoBuffer, mpHostVideoBuffer.GetByteCount());
    AJATime::Sleep(1000);
    memset(mpHostVideoBuffer.GetHostPointer(), 0x00, 1000000);
    mDevice.DMAWriteFrame (2, mpHostVideoBuffer, mpHostVideoBuffer.GetByteCount());
    AJATime::Sleep(1000);
    printf("looping\n");
    printf("%08lX %08lX %08lX %08lX\n", *(uint32_t *)(0x80000000+4*0x3400),
        *(uint32_t *)(0x80000000+4*0x3401), *(uint32_t *)(0x80000000+4*0x3402),
        *(uint32_t *)(0x80000000+4*0x3403));
    printf("%08lX %08lX %08lX %08lX\n", *(uint32_t *)(0x80000000+4*0x3440),
        *(uint32_t *)(0x80000000+4*0x3441), *(uint32_t *)(0x80000000+4*0x3442),
        *(uint32_t *)(0x80000000+4*0x3443));
  }
  mDevice.SetSDITransmitEnable (mConfig.fOutputChannel, false);

	return AJA_STATUS_SUCCESS;

}	//	Init


AJAStatus GTest::SetupVideo (void)
{
	const uint16_t	numFrameStores	(::NTV2DeviceGetNumFrameStores (mDeviceID));
	const uint16_t	numSDIOutputs	(::NTV2DeviceGetNumVideoOutputs(mDeviceID));

  printf("%d framestores, %d sdi outputs\n", numFrameStores, numSDIOutputs);

	mVideoFormat = NTV2_FORMAT_1080p_6000_A;

	mOutputDest		= ::NTV2ChannelToOutputDestination (mConfig.fOutputChannel);

	if (::NTV2DeviceHasBiDirectionalSDI (mDeviceID)					//	If device has bidirectional SDI connectors...
		&& NTV2_OUTPUT_DEST_IS_SDI (mOutputDest))			//	...and output destination is SDI...
			mDevice.SetSDITransmitEnable (mConfig.fOutputChannel, true);	//	...then enable transmit mode

	mDevice.EnableChannel (mConfig.fOutputChannel);		//	Enable the output frame buffer

  mDevice.SetMultiFormatMode (false);

	if (::NTV2DeviceCanDoMultiFormat (mDeviceID))									//	If device supports multiple formats per-channel...
		mDevice.SetVideoFormat (mVideoFormat, false, false, mConfig.fOutputChannel);		//	...then also set the output channel format to the detected input format

	//	Set the frame buffer pixel format for both channels on the device, assuming it
	//	supports that pixel format . . . otherwise default to 8-bit YCbCr...
	if (!::NTV2DeviceCanDoFrameBufferFormat (mDeviceID, mConfig.fPixelFormat))
		mConfig.fPixelFormat = NTV2_FBF_8BIT_YCBCR;

	//	Set the pixel format for both device frame buffers...
	mDevice.SetFrameBufferFormat (mConfig.fOutputChannel, mConfig.fPixelFormat);

	mDevice.SetMode (mConfig.fOutputChannel, NTV2_MODE_DISPLAY);

	//	Set up the device signal routing, and both playout and capture AutoCirculate...
	const NTV2InputCrosspointID		outputInputXpt	(::GetOutputDestInputXpt (mOutputDest));
	const NTV2OutputCrosspointID	fbOutputXpt		(::GetFrameBufferOutputXptFromChannel (mConfig.fOutputChannel, ::IsRGBFormat (mConfig.fPixelFormat)));
	NTV2OutputCrosspointID			outputXpt		(fbOutputXpt);

  mDevice.Connect (outputInputXpt, outputXpt);

	return AJA_STATUS_SUCCESS;

}	//	SetupVideo

