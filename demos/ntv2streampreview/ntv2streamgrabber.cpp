/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2streamgrabber.cpp
	@brief		Contains the implementation of the NTV2StreamGrabber class.
	@copyright	(C) 2013-2022 AJA Video Systems, Inc.  All rights reserved.
**/
#include <ostream>
#include "ntv2streamgrabber.h"
#include "ntv2democommon.h"
#include "ntv2devicefeatures.h"
#include "ntv2devicescanner.h"
#include "ntv2utils.h"
#if defined (INCLUDE_AJACC)
	#include "ajaanc/includes/ancillarylist.h"
	#include "ajaanc/includes/ancillarydata_cea608_line21.h"
	using namespace std;
#endif

#define NTV2_NUM_IMAGES		(10)

//	Convenience macros for EZ logging:
#define	FGFAIL(_expr_)		AJA_sERROR  (AJA_DebugUnit_Application, AJAFUNC << ": " << _expr_)
#define	FGWARN(_expr_)		AJA_sWARNING(AJA_DebugUnit_Application, AJAFUNC << ": " << _expr_)
#define	FGDBG(_expr_)		AJA_sDEBUG	(AJA_DebugUnit_Application, AJAFUNC << ": " << _expr_)
#define	FGNOTE(_expr_)		AJA_sNOTICE	(AJA_DebugUnit_Application, AJAFUNC << ": " << _expr_)
#define	FGINFO(_expr_)		AJA_sINFO	(AJA_DebugUnit_Application, AJAFUNC << ": " << _expr_)

static QMutex	gMutex;


NTV2StreamGrabber::NTV2StreamGrabber (QObject * parent)
	:	QThread					(parent),
		mRestart				(true),
		mAbort					(false),
    	mbFixedReference        (false),
		mBoardNumber			(0),
		mDeviceID				(DEVICE_ID_NOTFOUND),
		mChannel				(NTV2_CHANNEL2),
		mCurrentVideoFormat		(NTV2_FORMAT_UNKNOWN),
        mCurrentColorSpace      (NTV2_LHIHDMIColorSpaceYCbCr),
		mLastVideoFormat		(NTV2_FORMAT_UNKNOWN),
		mDebounceCounter		(0),
		mFormatIsProgressive	(true),
		mInputSource			(NTV2_NUM_INPUTSOURCES),
		mFrameBufferFormat		(NTV2_FBF_ARGB),
		mDoMultiChannel			(false)
{

}	//	constructor


NTV2StreamGrabber::~NTV2StreamGrabber ()
{
	mAbort = true;
	while (isRunning ())
		;	//	Wait for grabber thread to finish

	StopStream ();

}	//	destructor


void NTV2StreamGrabber::SetInputSource (const NTV2InputSource inInputSource)
{
	if (inInputSource != mInputSource)
	{
		qDebug() << "## DEBUG:  NTV2StreamGrabber::SetInputSource" << ::NTV2InputSourceToString (inInputSource).c_str ();
		gMutex.lock ();
			mInputSource = inInputSource;
			mRestart = true;
		gMutex.unlock ();
	}

}	//	SetInputSource


void NTV2StreamGrabber::SetDeviceIndex (const UWord inDeviceIndex)
{
	if (inDeviceIndex != mBoardNumber)
	{
		gMutex.lock ();
			mBoardNumber = inDeviceIndex;
			mRestart = true;
		gMutex.unlock ();
	}
}	//	SetDeviceIndex


UWord NTV2StreamGrabber::GetDeviceIndex (void) const
{
	gMutex.lock ();
	const UWord result (mBoardNumber);
	gMutex.unlock ();
	return result;
}


void NTV2StreamGrabber::run (void)
{
	//	Set up 2 images to ping-pong between capture and display...
	QImage *	images [NTV2_NUM_IMAGES];
	ULWord		framesCaptured	(0);
	FGNOTE("Thread started");

	// initialize images
	for (int i = 0; i < NTV2_NUM_IMAGES; i++)
	{
		images[i] = new QImage(STREAMPREVIEW_WIDGET_X, STREAMPREVIEW_WIDGET_Y, QImage::Format_RGB32);
	}

	// Make sure all Bidirectionals are set to Inputs.
	if (mNTV2Card.Open(mBoardNumber))
	{
		if (!mDoMultiChannel && !mNTV2Card.AcquireStreamForApplicationWithReference (kDemoAppSignature, int32_t(AJAProcess::GetPid())))
		{
			//	We have not acquired the board continue until something changes...
			qDebug ("Could not acquire board number %d", GetDeviceIndex());
			mNTV2Card.Close ();
			mDeviceID = DEVICE_ID_NOTFOUND;
		}
		else
		{
			mNTV2Card.GetEveryFrameServices (mSavedTaskMode);	//	Save the current state before we change it
			mNTV2Card.SetEveryFrameServices (NTV2_OEM_TASKS);	//	Since this is an OEM demo we will set the OEM service level
		}
	}

	while (true)
	{
		if (mAbort)
			break;

		if (!NTV2_IS_VALID_INPUT_SOURCE (mInputSource))
		{
			//	No input chosen, so display banner...
			QImage *	currentImage = images [0];
			currentImage->load (":/resources/splash.png");
			emit newStatusString ("");
			emit newFrame (*currentImage, true);
			if (::NTV2DeviceHasBiDirectionalSDI (mNTV2Card.GetDeviceID()))		//	If device has bidirectional SDI connectors...
			{
				bool waitForInput = false;
				UWord numInputs = NTV2DeviceGetNumVideoInputs(mNTV2Card.GetDeviceID());
				for (UWord offset = 0; offset < numInputs; offset++)
				{
					bool outputEnabled;
					mNTV2Card.GetSDITransmitEnable (NTV2Channel(offset), outputEnabled);
					if (outputEnabled)
					{
						waitForInput = true;
						mNTV2Card.SetSDITransmitEnable (NTV2Channel(offset), false);
					}
				}
				// Only if we had to change an output to input do we need to wait.
				if (waitForInput)
					msleep(500);
			}
			msleep(200);
			continue;
		}

		if (mRestart)
		{
			gMutex.lock ();
			if (mNTV2Card.IsOpen ())
			{
				StopStream();
				if (!mDoMultiChannel)
				{
					mNTV2Card.ReleaseStreamForApplicationWithReference (kDemoAppSignature, AJAProcess::GetPid ());
					mNTV2Card.SetEveryFrameServices (mSavedTaskMode);
				}
				mNTV2Card.Close ();
				mDeviceID = DEVICE_ID_NOTFOUND;
			}
			gMutex.unlock ();

			if (mNTV2Card.Open(mBoardNumber))
			{
				if (!mDoMultiChannel && !mNTV2Card.AcquireStreamForApplicationWithReference (kDemoAppSignature, int32_t(AJAProcess::GetPid())))
				{
					//We have not acquired the board continue until something changes
					qDebug() << "Could not acquire board number " << GetDeviceIndex();
					msleep (1000);
					mNTV2Card.Close ();
					mDeviceID = DEVICE_ID_NOTFOUND;
					continue;
				}

				mNTV2Card.GetEveryFrameServices (mSavedTaskMode);	//	Save the current state before we change it
				mNTV2Card.SetEveryFrameServices (NTV2_OEM_TASKS);	//	Since this is an OEM demo we will set the OEM service level

				mDeviceID = mNTV2Card.GetDeviceID ();
				if (::NTV2DeviceCanDoMultiFormat (mDeviceID))
					mNTV2Card.SetMultiFormatMode (false);

                if (!mNTV2Card.IsDeviceReady (false))
				{
					qDebug ("Device not ready");
					msleep (1000);	//	Keep the UI responsive while waiting for device to become ready
				}
				else if (SetupInput ())
				{
					NTV2StreamChannel	strStatus;
					NTV2StreamBuffer	bfrStatus;
					ULWord				status = 0;

					gMutex.lock ();
					if (!mNTV2Card.StreamChannelInitialize (mChannel))
					{
						qDebug() << "## WARNING:  Cannot acquire stream channel " << (int)(mChannel + 1);
						continue;
					}
					gMutex.unlock ();

					//	Unlock the old buffers
					mNTV2Card.DMABufferUnlockAll();

					//	Configure new buffers for streaming
					for (ULWord i = 0; i < NTV2_NUM_IMAGES; i++)
					{
						//	Delete the current image buffer
						delete images[i];

						//	Create a new buffer of the correct size
						images[i] = new QImage (mFrameDimensions.Width (), mFrameDimensions.Height (), QImage::Format_RGB32);

						//	Prelock and map the buffer
						if (!mNTV2Card.DMABufferLock(reinterpret_cast<PULWord>(images[i]->bits()), ULWord(images[i]->sizeInBytes()), true))
						{
							qDebug() << "## WARNING:  Cannot DMA lock input buffer";
							continue;
						}

						//	Queue the buffer for streaming capture
						NTV2Buffer buffer(reinterpret_cast<PULWord>(images[i]->bits()), ULWord(images[i]->sizeInBytes()));
						status = mNTV2Card.StreamBufferQueue(mChannel,
															buffer,
															i,
															bfrStatus);
						if (status == NTV2_STREAM_STATUS_SUCCESS)
						{
							qDebug() << "## WARNING:  Cannot add buffer to stream: " << bfrStatus.mStatus;
						}
					}

					//  Now start the stream capture
					status = mNTV2Card.StreamChannelStart(mChannel, strStatus);
					if (status != NTV2_STREAM_STATUS_SUCCESS)
					{
						qDebug() << "## WARNING:  Stream start failed: " << bfrStatus.mStatus;
					}

					framesCaptured = 0;
					mRestart = false;
				}	//	if board set up ok
				else
				{
					msleep (1000);	//	This keeps the UI responsive if/while this channel has no input
				}
			}	//	if board opened ok
			else
			{
				qDebug() << "## WARNING:  Open failed for device " << GetDeviceIndex ();
				msleep (200);
				continue;
			}

			emit newStatusString (::NTV2InputSourceToString (mInputSource, true).c_str());
		}	//	if mRestart

		if (CheckForValidInput () == false && NTV2_IS_VALID_INPUT_SOURCE (mInputSource))
		{
			QImage *	currentImage	(images [0]);
			currentImage->fill (qRgba (40, 40, 40, 255));

			QString	status	(QString("%1: No Detected Input").arg(::NTV2InputSourceToString(mInputSource, true).c_str()));
			emit newStatusString (status);
			emit newFrame (*currentImage, true);
			msleep (200);
			continue;
		}

		if (mNTV2Card.IsOpen())
		{
			NTV2StreamChannel	strStatus;
			NTV2StreamBuffer	bfrStatus;
			ULWord				status = 0;

			//  Look for captured buffers
        	status = mNTV2Card.StreamBufferRelease(mChannel, bfrStatus);
			if ((status == NTV2_STREAM_STATUS_SUCCESS) && (bfrStatus.mBufferCookie < NTV2_NUM_IMAGES))
			{
				ULWord index = bfrStatus.mBufferCookie;

				//	Update the status string
				QString outString (::NTV2InputSourceToString (mInputSource, true).c_str());
				outString.append ("  ");
				outString.append (::NTV2VideoFormatToString (mCurrentVideoFormat).c_str ());
				emit newStatusString (outString);

				//	Output the new video
				emit newFrame (*images[index], (framesCaptured == 0) ? true : false);

				//	Queue the buffer back to the stream
				NTV2Buffer buffer(reinterpret_cast<PULWord>(images[index]->bits()), ULWord(images[index]->sizeInBytes()));
				status = mNTV2Card.StreamBufferQueue(mChannel,
													buffer,
													index,
													bfrStatus);
				if (status == NTV2_STREAM_STATUS_SUCCESS)
				{
					qDebug() << "## WARNING:  Cannot add buffer to stream";
				}

				framesCaptured++;
			}
			else
			{
				//	Wait for one or more buffers to become available on the device, which should occur at next VBI...
				mNTV2Card.StreamChannelWait(mChannel, strStatus);
			}
		}
	}	//	loop til break

	if (mNTV2Card.IsOpen ())
	{
		gMutex.lock ();
		msleep (1000);
		if (!mDoMultiChannel)
		{
			mNTV2Card.ReleaseStreamForApplicationWithReference (kDemoAppSignature, int32_t(AJAProcess::GetPid()));	//	Release the device
			mNTV2Card.SetEveryFrameServices (mSavedTaskMode);	//	Restore prior task mode
		}
		mNTV2Card.Close ();
		mDeviceID = DEVICE_ID_NOTFOUND;
		gMutex.unlock ();
	}

	for (int i = 0;  i < NTV2_NUM_IMAGES;  i++)
		delete images[i];

	FGNOTE("Thread completed, will exit for device" << mNTV2Card.GetDisplayName() << " input source " << ::NTV2InputSourceToString(mInputSource));

}	//	run


bool NTV2StreamGrabber::SetupInput (void)
{
	bool	validInput	(false);

	NTV2_ASSERT (mNTV2Card.IsOpen ());
	NTV2_ASSERT (mDeviceID != DEVICE_ID_NOTFOUND);

	NTV2Channel inputChannel = ::NTV2InputSourceToChannel (mInputSource);
	if(inputChannel == NTV2_CHANNEL_INVALID)
		inputChannel = NTV2_CHANNEL1;

	mCurrentVideoFormat = GetVideoFormatFromInputSource ();
    mCurrentColorSpace = GetColorSpaceFromInputSource ();
    mFrameDimensions.Set (STREAMPREVIEW_WIDGET_X, STREAMPREVIEW_WIDGET_Y);

	if (NTV2_IS_VALID_VIDEO_FORMAT (mCurrentVideoFormat))
	{
		validInput = true;
		switch (mCurrentVideoFormat)
		{
		   case NTV2_FORMAT_1080p_5000_B:
				mCurrentVideoFormat = NTV2_FORMAT_1080p_5000_A;
				break;
			case NTV2_FORMAT_1080p_5994_B:
				mCurrentVideoFormat = NTV2_FORMAT_1080p_5994_A;
				break;
			case NTV2_FORMAT_1080p_6000_B:
				mCurrentVideoFormat = NTV2_FORMAT_1080p_6000_A;
				break;
			default:
				break;
		}
		mNTV2Card.SetVideoFormat (mCurrentVideoFormat, false, false, mChannel);
		NTV2VANCMode vm(NTV2_VANCMODE_INVALID);
		mNTV2Card.GetVANCMode(vm, mChannel);
		const NTV2FormatDescriptor fd(mCurrentVideoFormat, mFrameBufferFormat, vm);
		mFrameDimensions.Set (fd.GetRasterWidth(), fd.GetRasterHeight());
		const QString vfString (::NTV2VideoFormatToString (mCurrentVideoFormat).c_str ());
		qDebug() << "## DEBUG:  mInputSource=" << mChannel << ", mCurrentVideoFormat=" << vfString << ", width=" << mFrameDimensions.Width() << ", height=" << mFrameDimensions.Height();

 		mFormatIsProgressive = IsProgressivePicture (mCurrentVideoFormat);
        if (!mbFixedReference)
            mNTV2Card.SetReference (NTV2_REFERENCE_FREERUN);

		if (NTV2_INPUT_SOURCE_IS_SDI (mInputSource))
		{
			if (::NTV2DeviceGetNumCSCs (mDeviceID) > (UWord)mChannel)
			{
				mNTV2Card.Connect (::GetCSCInputXptFromChannel (inputChannel), ::GetSDIInputOutputXptFromChannel (inputChannel));
				mNTV2Card.Connect (::GetFrameBufferInputXptFromChannel (mChannel), ::GetCSCOutputXptFromChannel ((inputChannel), false/*isKey*/, true/*isRGB*/));
				mNTV2Card.SetFrameBufferFormat (mChannel, mFrameBufferFormat);
			}
			else
			{
				mNTV2Card.Connect (::GetFrameBufferInputXptFromChannel (mChannel), ::GetSDIInputOutputXptFromChannel (inputChannel));
				mNTV2Card.SetFrameBufferFormat (mChannel, NTV2_FBF_8BIT_YCBCR);
			}
			mNTV2Card.EnableChannel (mChannel);
			mNTV2Card.SetMode (mChannel, NTV2_MODE_CAPTURE);
			mNTV2Card.SetSDIInLevelBtoLevelAConversion (inputChannel, IsInput3Gb (mInputSource) ? true : false);
		}
		else if (NTV2_INPUT_SOURCE_IS_HDMI (mInputSource))
		{
            if (!mbFixedReference)
				mNTV2Card.SetReference (::NTV2InputSourceToReferenceSource(mInputSource));

			{
				mNTV2Card.EnableChannel (mChannel);
				mNTV2Card.SetMode (mChannel, NTV2_MODE_CAPTURE);
				mNTV2Card.SetFrameBufferFormat (mChannel, mFrameBufferFormat);
                if (mCurrentColorSpace == NTV2_LHIHDMIColorSpaceYCbCr)
				{
					mNTV2Card.Connect (::GetCSCInputXptFromChannel (mChannel),
										::GetInputSourceOutputXpt (mInputSource, false/*isSDI_DS2*/, false/*isHDMI_RGB*/, 0/*hdmiQuadrant*/));
					mNTV2Card.Connect (::GetFrameBufferInputXptFromChannel (mChannel),
										::GetCSCOutputXptFromChannel (mChannel, false/*isKey*/, true/*isRGB*/));
				}
				else
				{
					mNTV2Card.Connect (::GetFrameBufferInputXptFromChannel (mChannel),
										::GetInputSourceOutputXpt (mInputSource, false/*isSDI_DS2*/, true/*isHDMI_RGB*/, 0/*hdmiQuadrant*/));
				}
			}
		}
		else
			qDebug () << "## DEBUG:  NTV2StreamGrabber::SetupInput:  Bad mInputSource switch value " << ::NTV2InputSourceToChannelSpec (mInputSource);
	}	//	if video format not unknown

	return validInput;

}	//	SetupInput


void NTV2StreamGrabber::StopStream (void)
{
	if (mNTV2Card.IsOpen ())
	{
    	//  Release stream ownership
		mNTV2Card.StreamChannelRelease(mChannel);
	}
}	//	StopStream


bool NTV2StreamGrabber::CheckForValidInput (void)
{
	NTV2VideoFormat	videoFormat	(GetVideoFormatFromInputSource ());
    NTV2LHIHDMIColorSpace colorSpace (GetColorSpaceFromInputSource ());

	switch (videoFormat)
	{
	   case NTV2_FORMAT_1080p_5000_B:
			videoFormat = NTV2_FORMAT_1080p_5000_A;
			break;
		case NTV2_FORMAT_1080p_5994_B:
			videoFormat = NTV2_FORMAT_1080p_5994_A;
			break;
		case NTV2_FORMAT_1080p_6000_B:
			videoFormat = NTV2_FORMAT_1080p_6000_A;
			break;
		default:
			break;
	}
	if (videoFormat == NTV2_FORMAT_UNKNOWN)
	{
		mCurrentVideoFormat = videoFormat;
		return false;
	}	//	if no video or unknown format
    else if ((mCurrentVideoFormat != videoFormat) ||
             (mCurrentColorSpace != colorSpace))
	{
		if (mDebounceCounter == 0)
		{
			//	Check to see if the video input has stabilized...
			mLastVideoFormat = videoFormat;
			mDebounceCounter++;
		}
		else if (mDebounceCounter == 6)
		{
			//	The new format is stable -- restart autocirculate...
			mRestart = true;
			mCurrentVideoFormat = videoFormat;
			mDebounceCounter = 0;
		}
		else
		{
			if (mLastVideoFormat == videoFormat)
				mDebounceCounter++;		//	New format still stable -- keep counting
			else
				mDebounceCounter = 0;	//	Input changed again -- start over
		}

		return true;
	}	//	else if video format changed
	else
		return true;

}	//	CheckForValidInput


NTV2VideoFormat NTV2StreamGrabber::GetVideoFormatFromInputSource (void)
{
	NTV2VideoFormat	videoFormat	(NTV2_FORMAT_UNKNOWN);
	NTV2_ASSERT (mNTV2Card.IsOpen ());
	NTV2_ASSERT (mDeviceID != DEVICE_ID_NOTFOUND);

	switch (mInputSource)
	{
		case NTV2_NUM_INPUTSOURCES:
			break;			//	indicates no source is currently selected

		default:
			videoFormat = mNTV2Card.GetInputVideoFormat (mInputSource);
			break;
	}

	return videoFormat;

}	//	GetVideoFormatFromInputSource


NTV2LHIHDMIColorSpace NTV2StreamGrabber::GetColorSpaceFromInputSource (void)
{
    if (NTV2_INPUT_SOURCE_IS_HDMI (mInputSource))
    {
        NTV2LHIHDMIColorSpace	hdmiColor	(NTV2_LHIHDMIColorSpaceRGB);
        mNTV2Card.GetHDMIInputColor (hdmiColor, mChannel);
        return hdmiColor;
    }

    return NTV2_LHIHDMIColorSpaceYCbCr;
}


bool NTV2StreamGrabber::IsInput3Gb (const NTV2InputSource inputSource)
{
	bool	is3Gb	(false);

	mNTV2Card.GetSDIInput3GbPresent (is3Gb, ::NTV2InputSourceToChannel (inputSource));

	return is3Gb;
}	//	IsInput3Gb

