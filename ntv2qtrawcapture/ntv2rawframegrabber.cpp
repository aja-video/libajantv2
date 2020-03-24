/**
	@file		ntv2rawframegrabber.cpp
	@brief		Contains the implementation of the NTV2RawFrameGrabber class.
	@copyright	(C) 2013-2020 AJA Video Systems, Inc.  All rights reserved.
**/


#include <ostream>
#include "ajabase/system/lock.h"
#include "ajabase/system/memory.h"
#include "ajabase/system/process.h"
#include "ajabase/system/thread.h"
#include "ajapreviewwidget.h"
#include "ntv2devicefeatures.h"
#include "ntv2devicescanner.h"
#include "ntv2rawdngwriter.h"
#include "ntv2rawframegrabber.h"
#include "ntv2rawpreview.h"
#include "ntv2utils.h"


//	Globals
static AJALock	gMutex;


NTV2RawFrameGrabber::NTV2RawFrameGrabber (QObject * parent)
	:	QThread						(parent),
		mDeviceIndex				(0),
		mDeviceID					(DEVICE_ID_NOTFOUND),
		mPrimaryChannel				(NTV2_CHANNEL1),
		mSecondaryChannel			(NTV2_CHANNEL1),
		mCurrentVideoFormat			(NTV2_FORMAT_UNKNOWN),
		mLastVideoFormat			(NTV2_FORMAT_UNKNOWN),
		mVideoFormatDebounceCounter	(0),
		mInputSource				(NTV2_INPUTSOURCE_SDI1),
		mFrameBufferFormat			(NTV2_FBF_10BIT_RAW_RGB),
		mRecording					(false),
		mPreviewWhenIdle			(true),
		mIncrementSequence			(true),
		mRecordPath					(""),
		mInputVideoWidth			(3840),
		mInputLinkCount				(0),
		mAbort						(false),
		mGlobalQuit					(false),
		mRestart					(true),
		mDngWriterThread			(NULL),
		mPreviewThread				(NULL)
{
	::memset (mDngHostBuffer,		0, sizeof (mDngHostBuffer));
	::memset (mPreviewHostBuffer,	0, sizeof (mPreviewHostBuffer));

}	//	constructor


NTV2RawFrameGrabber::~NTV2RawFrameGrabber ()
{
	//	Stop capturing raw frames from the device
	StopAutoCirculate ();

	//	Stop the file writer and UI preview threads
	StopWorkerThreads ();

	//	Set flags instructing any looping threads to exit
	mGlobalQuit	= true;
	mAbort		= true;

	//	Wait for the thread in ::run to exit
	while (isRunning ())
		;

	//	Free all my buffers...
	for (unsigned bufferNdx = 0; bufferNdx < CIRCULAR_BUFFER_SIZE; bufferNdx++)
	{
		if (mDngHostBuffer[bufferNdx].fVideoBuffer)
		{
            AJAMemory::FreeAligned (mDngHostBuffer[bufferNdx].fVideoBuffer);
			mDngHostBuffer[bufferNdx].fVideoBuffer = NULL;
		}
	}	//	for each buffer in the ring

	//	Note that the buffers in the preview ring sre not deleted.
	//	This is because they shared the same buffer addresses as the DNG writer ring.
	//	The buffers the ring pointed to have already been freed by the above.

}	//	destructor


UWord NTV2RawFrameGrabber::GetDeviceIndex (void) const
{
	gMutex.Lock ();
		const UWord result (mDeviceIndex);
	gMutex.Unlock ();

	return result;

}	//	GetDeviceIndex


void NTV2RawFrameGrabber::SetDeviceIndex (const UWord inDeviceIndex)
{
	if (inDeviceIndex != mDeviceIndex)
	{
		gMutex.Lock ();
			mDeviceIndex = inDeviceIndex;
			mRestart = true;
		gMutex.Unlock ();
	}

}	//	SetDeviceIndex


void NTV2RawFrameGrabber::SetIncrementSequence (const bool inIncrementSequence)
{
	gMutex.Lock ();
		mIncrementSequence = inIncrementSequence;
		if (mDngWriterThread)
		{
			mDngWriterThread->SetIncrementSequence (mIncrementSequence);
		}
	gMutex.Unlock ();

}	//	SetIncrementSequence


void NTV2RawFrameGrabber::SetNewFrame (const uint8_t * inBytes, uint32_t inSize)
{
	QImage qImage (inBytes, (int) inSize, mFrameDimensions.Height(), QImage::Format_RGB32);

	emit NewFrame (qImage, false);

}	//	SetNewFrame


void NTV2RawFrameGrabber::SetNewStatusString (const std::string inStatusString)
{
	QString outString (NTV2InputSourceToString (mInputSource).c_str ());
	outString.append ("  ");
	outString.append (::NTV2VideoFormatToString (mCurrentVideoFormat).c_str ());
	outString.append ("  ");
	outString.append (inStatusString.c_str ());

	emit NewStatusString (outString);

}	//	SetNewStatusString


void NTV2RawFrameGrabber::SetPreviewWhenIdle (const bool inPreviewWhenIdle)
{
	gMutex.Lock ();
		mPreviewWhenIdle = inPreviewWhenIdle;
	gMutex.Unlock ();

}	//	SetPreviewWhenIdle


void NTV2RawFrameGrabber::SetRecordPath (const std::string inRecordPath)
{
	gMutex.Lock ();
		mRecordPath = inRecordPath;
		if (mDngWriterThread)
		{
			mDngWriterThread->SetFileNameBase (mRecordPath);
		}
	gMutex.Unlock ();

}	//	SetRecordPath


void NTV2RawFrameGrabber::SetRecording (const bool inRecording)
{
	gMutex.Lock ();
		mRecording = inRecording;
		if (mDngWriterThread)
		{
			mDngWriterThread->SetRecordState (mRecording);
		}
	gMutex.Unlock ();

}	//	SetRecording


void NTV2RawFrameGrabber::run (void)
{
	//	Set up 2 images to ping-pong between capture and display...
	QImage					imageEven					(AJAPREVIEW_WIDGET_X, AJAPREVIEW_WIDGET_Y, QImage::Format_RGB32);
	QImage					imageOdd					(AJAPREVIEW_WIDGET_X, AJAPREVIEW_WIDGET_Y, QImage::Format_RGB32);
	QImage *				images [2]					=	{	&imageEven,		&imageOdd	};
	ULWord					framesCaptured				(0);
	ULWord					mPrimaryDropsRequired		(0);		//	Frame time synchronization requires primary drop frame
	ULWord					mPrimaryDropsCompleted		(0);		//	Frame time synchronization primary drop count
	ULWord					mSecondaryDropsRequired		(0);		//	Frame time synchronization requires secondary drop frame
	ULWord					mSecondaryDropsCompleted	(0);		//	Frame time synchronization secondary drop count
	AUTOCIRCULATE_TRANSFER	mPrimaryXfer;
	AUTOCIRCULATE_TRANSFER	mSecondaryXfer;

	while (true)
	{
		if (mAbort)
			break;

		if (!mRecording && !mPreviewWhenIdle)
		{
			//	No input chosen, so display banner...
			QImage *	currentImage = images [framesCaptured & 0x1];
			currentImage->load (":/resources/splash.png");
			emit NewStatusString ("");
			emit NewFrame (*currentImage, true);

			//	Allow other threads to run for a while, then try again
			msleep (200);
			continue;
		}

		if (mRestart)
		{
			gMutex.Lock ();
				StopAutoCirculate ();
				StopWorkerThreads ();
				if (mDevice.IsOpen ())
				{
					mDevice.SetEveryFrameServices (mPreviousFrameServices);
					mDevice.ReleaseStreamForApplicationWithReference (NTV2_FOURCC('D','E','M','O'), AJAProcess::GetPid ());
					mDevice.Close ();
				}
			gMutex.Unlock ();

			if (mDevice.Open (mDeviceIndex, false))
			{
				if (!mDevice.AcquireStreamForApplicationWithReference (NTV2_FOURCC('D','E','M','O'), (uint32_t) AJAProcess::GetPid ()))
				{
					//We have not acquired the board continue until something changes
					qDebug ("Could not acquire device index %d", GetDeviceIndex ());
					mDevice.Close ();
					msleep (500);
					continue;
				}
				else
				{
					//	Save the current state before we change it
					mDevice.GetEveryFrameServices (&mPreviousFrameServices);
					//	Since this is an OEM demo we will set the OEM service level
					mDevice.SetEveryFrameServices (NTV2_OEM_TASKS);
				}

				mDeviceID = mDevice.GetDeviceID ();
				if (::NTV2DeviceCanDoMultiFormat (mDeviceID))
					mDevice.SetMultiFormatMode (false);	// This app needs all channels configured alike

				if (SetupInput ())
				{
					SetupHostBuffers ();
					SetupAutoCirculate ();

					//	Start autocirculate on channel 1
					//	If more than one input link is in use, channels 2 and 3 will start synchronously
					mDevice.AutoCirculateStart (mPrimaryChannel);

					StartWorkerThreads ();
					imageEven		= imageEven.scaled (mFrameDimensions.Width(), mFrameDimensions.Height());	//	make image correct size
					imageOdd		= imageOdd.scaled (mFrameDimensions.Width(), mFrameDimensions.Height());	//	make image correct size
					framesCaptured	= 0;
					mRestart		= false;
				}	//	if board set up ok
				else
				{
					msleep (1000);	//	This keeps the UI responsive if/while this channel has no input
				}
			}	//	if board opened ok
			else
			{
				qDebug ("Open failed for device index %d", GetDeviceIndex ());
				msleep (200);
				continue;
			}
		}	//	if mRestart

		//	Use the input VPID to determine the width of the frame
		ULWord vpidA = 0;
		ULWord vpidB = 0;
		if (mDevice.ReadSDIInVPID (mPrimaryChannel, vpidA, vpidB))
			mInputVideoWidth = vpidA & BIT(22) ? 4096 : 3840;
		else
			qDebug () << "Loop read VPID failed";

		// setup for frame alignment monitoring when capturing 2 channels
		// the transferred frames must have the same frameTime stamp
		bool primaryTransfer = false;
		bool secondaryTransfer = false;
//		char debugStr[256];

		//	Handle the autocirculate process
		RP188_STRUCT				acTimecode;
		AUTOCIRCULATE_STATUS		status;
		mDevice.AutoCirculateGetStatus (mPrimaryChannel, status);
		if (status.IsRunning() && status.HasAvailableInputFrame())
		{
			// check for required frameTime alignment drop
			if (mPrimaryDropsCompleted >= mPrimaryDropsRequired)
			{
				//	At this point, there's at least one fully-formed frame available in the device's
				//	frame buffer to transfer to the host. Reserve an AVDataBuffer to "produce", and
				//	use it in the next transfer from the device...
				AVDataBuffer *	captureData	(mDngCircularBuffer.StartProduceNextBuffer ());

				//	It's possible no buffer will be produced if the app is shutting down
				if (!captureData)
					continue;

				mPrimaryXfer.SetVideoBuffer (captureData->fVideoBuffer, captureData->fVideoBufferSize);

				primaryTransfer = mDevice.AutoCirculateTransfer (mPrimaryChannel, mPrimaryXfer);

				//	Save the timecode that was captured at the same time as the frame
				NTV2_RP188	timeCode;
				mPrimaryXfer.GetInputTimeCode (timeCode);
				captureData->fRP188Data	= timeCode;
				acTimecode				= captureData->fRP188Data;

				//  Signal that we're done "producing" the frame, making it available for future "consumption"...
				mDngCircularBuffer.EndProduceNextBuffer ();

				//	Now send the same frame to be previewed if there's room in the queue
				if ((mRecording || mPreviewWhenIdle) &&
					(mPreviewCircularBuffer.GetCircBufferCount () < mPreviewCircularBuffer.GetNumFrames ()))
				{
					AVDataBuffer *	previewData (mPreviewCircularBuffer.StartProduceNextBuffer ());

					//	Pass the same raw buffer to the preview thread for display
					previewData->fVideoBuffer			= (uint32_t *) mPrimaryXfer.GetVideoBuffer().GetHostPointer();
					previewData->fVideoBufferSize		= mPrimaryXfer.GetVideoBuffer().GetByteCount();
					previewData->fRP188Data				= timeCode;
					previewData->fVideoBufferUnaligned	= (uint8_t *) mInputVideoWidth;	//	Repurpose this member to hold the input video width

					mPreviewCircularBuffer.EndProduceNextBuffer ();
				}
			}
			else
			{
				// drop this frame to align with secondary
				mPrimaryDropsCompleted++;
//				sprintf(debugStr, "Primary drop required %d  completed %d\n", mPrimaryDropsRequired, mPrimaryDropsCompleted);
//				OutputDebugStr(debugStr);
			}

			//	Perform a second transfer for high frame rate captures
			if ((mInputLinkCount == 2) || (mInputLinkCount == 4))
			{
				// check for required frameTime alignment drop
				if (mSecondaryDropsCompleted >= mSecondaryDropsRequired)
				{
					AVDataBuffer *	captureData2 (mDngCircularBuffer.StartProduceNextBuffer ());

					mSecondaryXfer.SetVideoBuffer (captureData2->fVideoBuffer, captureData2->fVideoBufferSize);

					secondaryTransfer = mDevice.AutoCirculateTransfer (mSecondaryChannel, mSecondaryXfer);

					// Timecode is received only on channel 1
					captureData2->fRP188Data = acTimecode;

					mDngCircularBuffer.EndProduceNextBuffer ();

					//	And queue the preview frame as above
					if ((mRecording || mPreviewWhenIdle) &&
						(mPreviewCircularBuffer.GetCircBufferCount () < mPreviewCircularBuffer.GetNumFrames ()))
					{
						AVDataBuffer *	previewData (mPreviewCircularBuffer.StartProduceNextBuffer ());

						previewData->fVideoBuffer			= (uint32_t *) mSecondaryXfer.GetVideoBuffer().GetHostPointer();
						previewData->fVideoBufferSize		= mSecondaryXfer.GetVideoBuffer().GetByteCount();
						// Timecode is received only on channel 1
						previewData->fRP188Data				= acTimecode;
						previewData->fVideoBufferUnaligned	= (uint8_t *) mInputVideoWidth;	//	Repurpose this member to hold the input video width

						mPreviewCircularBuffer.EndProduceNextBuffer ();
					}
				}
				else
				{
					// drop this frame to align with primary
					mSecondaryDropsCompleted++;
//					sprintf(debugStr, "Secondary drop required %d  completed %d\n", mSecondaryDropsRequired, mSecondaryDropsCompleted);
//					OutputDebugStr(debugStr);
				}
			}

			framesCaptured++;

			// if we did primary and secondary transfers, check frameTime alignment
			if (primaryTransfer && secondaryTransfer)
			{
				// compute time between primary and secondary frame captures
				LWord delta = (LWord)(mPrimaryXfer.GetFrameInfo().acFrameTime  -  mSecondaryXfer.GetFrameInfo().acFrameTime);
				// check for primary ahead of secondary
				if (delta > 50000) 
				{
					mPrimaryDropsRequired++;
//					sprintf(debugStr, "Primary drop required %d  completed %d\n", mPrimaryDropsRequired, mPrimaryDropsCompleted);
//					OutputDebugStr(debugStr);
				}
				// check for secondary ahead of primary
				if ((-delta) > 50000) 
				{
					mSecondaryDropsRequired++;
//					sprintf(debugStr, "Secondary drop required %d  completed %d\n", mSecondaryDropsRequired, mSecondaryDropsCompleted);
//					OutputDebugStr(debugStr);
				}
			}
		}	//	if running and at least one frame ready to transfer
		else
		{
			//	Suspend processing until a new frame is received
			mDevice.WaitForInputVerticalInterrupt (mPrimaryChannel);
		}

		if (CheckForValidInput () == false)
		{
			//	If no valud input, display a message over a gray frame
			QImage *	currentImage	(images [framesCaptured & 0x1]);
			currentImage->fill (qRgba (40, 40, 40, 255));

			QString		status			(QString ("No Input Detected"));
			emit NewStatusString (status);
			emit NewFrame (*currentImage, true);
		}
	}	//	loop til break

	if (mDevice.IsOpen ())
	{
		gMutex.Lock ();
			StopAutoCirculate ();
			StopWorkerThreads ();
			//	Restore the service level that was before the acquire
			mDevice.SetEveryFrameServices (mPreviousFrameServices);
			//	Release the board
			mDevice.ReleaseStreamForApplicationWithReference (NTV2_FOURCC('D','E','M','O'), (uint32_t) AJAProcess::GetPid ());		
		gMutex.Unlock ();
	}

}	//	run


bool NTV2RawFrameGrabber::CheckForValidInput (void)
{
	NTV2VideoFormat	videoFormat	(GetInputVideoFormat ());
	if (videoFormat == NTV2_FORMAT_UNKNOWN)
	{
		mCurrentVideoFormat = videoFormat;
		return false;
	}	//	if no video or unknown format
	else if (mCurrentVideoFormat != videoFormat)
	{
		if (mVideoFormatDebounceCounter == 0)
		{
			//	Begin a check to see if the video input has stabilized
			mLastVideoFormat = videoFormat;
			mVideoFormatDebounceCounter++;
		}
		else if (mVideoFormatDebounceCounter == 6)
		{
			// The new format is stable, and it's ok to restart autocirculate
			mRestart = true;
			mCurrentVideoFormat = videoFormat;
			mVideoFormatDebounceCounter = 0;
		}
		else
		{
			if (mLastVideoFormat == videoFormat)
			{
				//	New format still stable.  Keep the count going...
				mVideoFormatDebounceCounter++;
			}
			else
			{
				//	Input changed again.  Restart the count...
				mVideoFormatDebounceCounter = 0;
			}
		}

		return true;
	}	//	else if video format changed
	else
		return true;

}	//	CheckForValidInput


NTV2VideoFormat NTV2RawFrameGrabber::GetInputVideoFormat (void)
{
	NTV2VideoFormat	videoFormat		(NTV2_FORMAT_UNKNOWN);
	uint64_t		newInputCount	(1);

	switch (mInputSource)
	{
		case NTV2_INPUTSOURCE_SDI1:
		{
			videoFormat = mDevice.GetInputVideoFormat (mInputSource);
			NTV2VideoFormat	videoFormatNext	(mDevice.GetInputVideoFormat (NTV2_INPUTSOURCE_SDI2));
			if (videoFormatNext == videoFormat)
			{
				//	The first two SDI inputs are the same, it could be 60 fps
				newInputCount = 2;
				videoFormatNext = mDevice.GetInputVideoFormat (NTV2_INPUTSOURCE_SDI3);
				if (videoFormatNext == videoFormat)
				{
					videoFormatNext = mDevice.GetInputVideoFormat (NTV2_INPUTSOURCE_SDI4);
					if (videoFormatNext == videoFormat)
					{
						//	The first four SDI inputs are the same, it could be 120 fps
						newInputCount = 4;
					}
				}
			}
			break;
		}
		default:
			videoFormat = mDevice.GetInputVideoFormat (mInputSource);
			break;
	}

	if (newInputCount != mInputLinkCount)
	{
		//	The number of SDI links in use has changed.
		//	Choose the correct pixel format and set the flag requesting a device reconfiguration
		mFrameBufferFormat = (newInputCount == 4) ? NTV2_FBF_10BIT_RAW_YCBCR : NTV2_FBF_10BIT_RAW_RGB;
		mInputLinkCount = newInputCount;
		mRestart = true;
	}

	return videoFormat;

}	//	GetInputVideoFormat


void NTV2RawFrameGrabber::SetupAutoCirculate (void)
{
	StopAutoCirculate ();

	//	Assign which frame buffer will be used by each channel
	const ULWord	kSecondaryStartFrameNumber	(8);
	const ULWord	kSecondaryEndFrameNumber	(15);

	ULWord autoCirculateChannelCount = 1;	//	Assume a single link is in use
	if ((mInputLinkCount == 2) || (mInputLinkCount == 4))
	{
		//	Two video streams need to be captured.  The secondary stream will come from channel 3.
		//	Channel 2 is also started so that channels 1, 2, and 3 can be ganged together and
		//	started in unison.  But channel 2 is a dummy and is otherwise ignored.
		mDevice.AutoCirculateInitForInput (	NTV2_CHANNEL3,				//	channel
											0,							//	frameCount -- using zero for custom frame numbers
											NTV2_AUDIOSYSTEM_INVALID,	//	audioSystem (invalid means noAudio)
											0,							//	no option flags (no RP188, no LTC, no Anc, etc.)
											1,							//	numChannels
											kSecondaryStartFrameNumber,	//	startFrameNumber
											kSecondaryEndFrameNumber);	//	endFrameNumber

		// This instance is a dummy
		mDevice.AutoCirculateInitForInput (	NTV2_CHANNEL2,				//	channel
											0,							//	frameCount -- using zero for custom frame numbers
											NTV2_AUDIOSYSTEM_INVALID,	//	audioSystem (invalid means noAudio)
											0,							//	no option flags (no RP188, no LTC, no Anc, etc.)
											1,							//	numChannels
											16,							//	startFrameNumber
											17);						//	endFrameNumber
		autoCirculateChannelCount = 3;
	}

	//	Setup autocirculate for the primary capture channel
	mDevice.AutoCirculateInitForInput (	NTV2_CHANNEL1,
										0,							//	frameCount -- using zero for custom frame numbers
										NTV2_AUDIOSYSTEM_INVALID,	//	audioSystem (invalid means noAudio)
										0,							//	no option flags (no RP188, no LTC, no Anc, etc.)
										1,							//	numChannels
										kSecondaryStartFrameNumber,	//	startFrameNumber
										kSecondaryEndFrameNumber);	//	endFrameNumber

}	//	SetupAutoCirculate


bool NTV2RawFrameGrabber::SetupHostBuffers (void)
{
	//	Let my circular buffers know when it's time to quit...
	mDngCircularBuffer.SetAbortFlag		(&mGlobalQuit);
	mPreviewCircularBuffer.SetAbortFlag	(&mGlobalQuit);

	//	Calculate the size of the largest raw frame
	uint32_t rawBufferSize = ((2048 * 1080 * 4) * 10) / 8;

	//	Allocate my buffers for the file writer...
	for (size_t ndx = 0; ndx < CIRCULAR_BUFFER_SIZE; ndx++)
	{
		mDngHostBuffer [ndx].fVideoBuffer		= reinterpret_cast <uint32_t *> (AJAMemory::AllocateAligned (rawBufferSize, 4096));
		mDngHostBuffer [ndx].fVideoBufferSize	= rawBufferSize;
		mDngHostBuffer [ndx].fAudioBuffer		= NULL;
		mDngHostBuffer [ndx].fAudioBufferSize	= 0;

		::memset (mDngHostBuffer [ndx].fVideoBuffer, 0x00, rawBufferSize);

		mDngCircularBuffer.Add (&mDngHostBuffer [ndx]);
	}	//	for each AV buffer in my circular buffer

	//	Allocate my buffers for the preview window...
	//	No video buffer addresses are assigned, as the capture thread will populate the address
	//	with the same one used by the file writer ring.
	for (size_t ndx = 0; ndx < CIRCULAR_BUFFER_SIZE; ndx++)
	{
		mPreviewHostBuffer [ndx].fVideoBuffer		= NULL;
		mPreviewHostBuffer [ndx].fVideoBufferSize	= rawBufferSize;
		mPreviewHostBuffer [ndx].fAudioBuffer		= NULL;
		mPreviewHostBuffer [ndx].fAudioBufferSize	= 0;

		mPreviewCircularBuffer.Add (&mPreviewHostBuffer [ndx]);
	}	//	for each AV buffer in my circular buffer

	return true;
}


bool NTV2RawFrameGrabber::SetupInput (void)
{
	bool	validInput	(false);

	//	Bi-directional SDI connectors need to be set to capture...
	if (::NTV2DeviceHasBiDirectionalSDI (mDeviceID))
	{
		switch (::NTV2DeviceGetNumVideoChannels (mDeviceID))
		{
			case 8:		mDevice.SetSDITransmitEnable (NTV2_CHANNEL8, false);	// Fall through
			case 7:		mDevice.SetSDITransmitEnable (NTV2_CHANNEL7, false);	// Fall through
			case 6:		mDevice.SetSDITransmitEnable (NTV2_CHANNEL6, false);	// Fall through
			case 5:		mDevice.SetSDITransmitEnable (NTV2_CHANNEL5, false);	// Fall through
			case 4:		mDevice.SetSDITransmitEnable (NTV2_CHANNEL4, false);	// Fall through
			case 3:		mDevice.SetSDITransmitEnable (NTV2_CHANNEL3, false);	// Fall through
			case 2:		mDevice.SetSDITransmitEnable (NTV2_CHANNEL2, false);	// Fall through
			case 1:		mDevice.SetSDITransmitEnable (NTV2_CHANNEL1, false);
						break;
			default:	qDebug ("NTV2RawFrameGrabber::SetupInput bad mInputSource %d", ::NTV2DeviceGetNumVideoChannels (mDeviceID));
						break;
		}
	}

	//	Get the format of the current SDI input
	mCurrentVideoFormat	= GetInputVideoFormat ();
	mPrimaryChannel	= ::NTV2InputSourceToChannel (mInputSource);

	//	Set a default size for the preview windoe
	mFrameDimensions.Set (AJAPREVIEW_WIDGET_X, AJAPREVIEW_WIDGET_Y);

	//	Check if the input has valid video
	if (NTV2_IS_VALID_VIDEO_FORMAT (mCurrentVideoFormat) && mInputLinkCount)
	{
		validInput	= true;

		//	Set the device's video format to match that of the input
		mDevice.SetVideoFormat (mCurrentVideoFormat, false, false, mPrimaryChannel);

		//	Set the preview window's size to match that of the input
		mFrameDimensions = mDevice.GetActiveFrameDimensions (mPrimaryChannel);

		//	Route the input signals to the frame stores depending on the number of SDI links used
		switch (mInputSource)
		{
			case NTV2_INPUTSOURCE_SDI1:
				switch (mInputLinkCount)
				{
					case 1:	//	Input is single link 1080p30 level B
							mDevice.Connect (NTV2_XptDualLinkIn1Input,		NTV2_XptSDIIn1);
							mDevice.Connect (NTV2_XptDualLinkIn1DSInput,	NTV2_XptSDIIn1DS2);
							mDevice.Connect (NTV2_XptFrameBuffer1Input,		NTV2_XptDuallinkIn1);
							mDevice.Connect (NTV2_XptFrameBuffer2Input,		NTV2_XptDuallinkIn1);
							break;
					case 2:	//	Input is dual link 1080p60 level B
							mDevice.Connect (NTV2_XptDualLinkIn1Input,		NTV2_XptSDIIn1);
							mDevice.Connect (NTV2_XptDualLinkIn1DSInput,	NTV2_XptSDIIn1DS2);
							mDevice.Connect (NTV2_XptFrameBuffer1Input,		NTV2_XptDuallinkIn1);
							mDevice.Connect (NTV2_XptFrameBuffer2Input,		NTV2_XptDuallinkIn1);

							mDevice.Connect (NTV2_XptDualLinkIn2Input,		NTV2_XptSDIIn2);
							mDevice.Connect (NTV2_XptDualLinkIn2DSInput,	NTV2_XptSDIIn2DS2);
							mDevice.Connect (NTV2_XptFrameBuffer3Input,		NTV2_XptDuallinkIn2);
							mDevice.Connect (NTV2_XptFrameBuffer4Input,		NTV2_XptDuallinkIn2);
							break;
					case 4:	//	Input is quad link 1080p120 level A
							mDevice.Connect (NTV2_XptFrameBuffer1Input,		NTV2_XptSDIIn1);
							mDevice.Connect (NTV2_XptFrameBuffer2Input,		NTV2_XptSDIIn2);
							mDevice.Connect (NTV2_XptFrameBuffer3Input,		NTV2_XptSDIIn3);
							mDevice.Connect (NTV2_XptFrameBuffer4Input,		NTV2_XptSDIIn4);
							break;
				}
				break;

			default:
				qDebug ("NTV2RawFrameGrabber::SetupInput unsupported channel spec switch value %d", ::NTV2InputSourceToChannelSpec(mInputSource));
				break;
		}	//	switch on input source / channelSpec

		//	It's only necessary to genlock to the input when in E-E mode
		mDevice.SetReference (NTV2_REFERENCE_FREERUN);

		//	Set all the board's frame stores to capture and use the same format
		switch (::NTV2DeviceGetNumFrameStores (mDeviceID))
		{
			case 8:
				mDevice.SetMode (NTV2_CHANNEL8, NTV2_MODE_CAPTURE);
				mDevice.EnableChannel (NTV2_CHANNEL8);
				mDevice.SetFrameBufferFormat (NTV2_CHANNEL8, mFrameBufferFormat);		//	Fall through
			case 7:
				mDevice.SetMode (NTV2_CHANNEL7, NTV2_MODE_CAPTURE);
				mDevice.EnableChannel (NTV2_CHANNEL7);
				mDevice.SetFrameBufferFormat (NTV2_CHANNEL7, mFrameBufferFormat);		//	Fall through
			case 6:
				mDevice.SetMode (NTV2_CHANNEL6, NTV2_MODE_CAPTURE);
				mDevice.EnableChannel (NTV2_CHANNEL6);
				mDevice.SetFrameBufferFormat (NTV2_CHANNEL6, mFrameBufferFormat);		//	Fall through
			case 5:
				mDevice.SetMode (NTV2_CHANNEL5, NTV2_MODE_CAPTURE);
				mDevice.EnableChannel (NTV2_CHANNEL5);
				mDevice.SetFrameBufferFormat (NTV2_CHANNEL5, mFrameBufferFormat);		//	Fall through
			case 4:
				mDevice.SetMode (NTV2_CHANNEL4, NTV2_MODE_CAPTURE);
				mDevice.EnableChannel (NTV2_CHANNEL4);
				mDevice.SetFrameBufferFormat (NTV2_CHANNEL4, mFrameBufferFormat);		//	Fall through
			case 3:
				mDevice.SetMode (NTV2_CHANNEL3, NTV2_MODE_CAPTURE);
				mDevice.EnableChannel (NTV2_CHANNEL3);
				mDevice.SetFrameBufferFormat (NTV2_CHANNEL3, mFrameBufferFormat);		//	Fall through
			case 2:
				mDevice.SetMode (NTV2_CHANNEL2, NTV2_MODE_CAPTURE);
				mDevice.EnableChannel (NTV2_CHANNEL2);
				mDevice.SetFrameBufferFormat (NTV2_CHANNEL2, mFrameBufferFormat);		//	Fall through
			case 1:
				mDevice.SetMode (NTV2_CHANNEL1, NTV2_MODE_CAPTURE);
				mDevice.EnableChannel (NTV2_CHANNEL1);
				mDevice.SetFrameBufferFormat (NTV2_CHANNEL1, mFrameBufferFormat);		//	Fall through
				break;
		}
	}	//	if video format not unknown

	//	Configure the embedded timecode extractor to look for ATC_LTC
	mDevice.SetRP188Source (mPrimaryChannel, 0);

	//	Read the VPID now so the file writer will be passed the right raster size
	ULWord vpidA = 0;
	ULWord vpidB = 0;
	if (mDevice.ReadSDIInVPID (mPrimaryChannel, vpidA, vpidB))
		mInputVideoWidth = vpidA & BIT(22) ? 4096 : 3840;
	else
		qDebug () << "SetupInput read VPID failed";

	return validInput;

}	//	SetupInput


void NTV2RawFrameGrabber::StopAutoCirculate (void)
{
	if (mDevice.IsOpen ())	//	Equivalent to: Open (GetBoardIndex (), false, BOARDTYPE_AJAKONA))
	{
		//	This will also stop the secondary channel autocirculate, if active, since the channel control is ganged
		mDevice.AutoCirculateStop (mPrimaryChannel);

		//	Switch all the frame stores back to display
		//	This is not strictly needed, but helps to avoid confusion
		switch (::NTV2DeviceGetNumFrameStores (mDeviceID))
		{
			case 8:	mDevice.SetMode (NTV2_CHANNEL8, NTV2_MODE_DISPLAY);	//	Fall through
			case 7:	mDevice.SetMode (NTV2_CHANNEL7, NTV2_MODE_DISPLAY);	//	Fall through
			case 6:	mDevice.SetMode (NTV2_CHANNEL6, NTV2_MODE_DISPLAY);	//	Fall through
			case 5:	mDevice.SetMode (NTV2_CHANNEL5, NTV2_MODE_DISPLAY);	//	Fall through
			case 4:	mDevice.SetMode (NTV2_CHANNEL4, NTV2_MODE_DISPLAY);	//	Fall through
			case 3:	mDevice.SetMode (NTV2_CHANNEL3, NTV2_MODE_DISPLAY);	//	Fall through
			case 2:	mDevice.SetMode (NTV2_CHANNEL2, NTV2_MODE_DISPLAY);	//	Fall through
			case 1:	mDevice.SetMode (NTV2_CHANNEL1, NTV2_MODE_DISPLAY);	//	Fall through
					break;
		}
	}

}	//	StopAutoCirculate


void NTV2RawFrameGrabber::StartWorkerThreads (void)
{
	//	Create a thread to manage writing raw frames to storage as DNG files
	mDngWriterThread = new NTV2RawDngWriter (mDngCircularBuffer);
	if (mDngWriterThread)
	{
		//	Configure the file writer with the user's choices
		mDngWriterThread->SetFileNameBase (mRecordPath);
		mDngWriterThread->SetFileNameSequence (0);
		mDngWriterThread->SetRasterDimensions (mInputVideoWidth, 2160);
		mDngWriterThread->SetIncrementSequence (mIncrementSequence);
		mDngWriterThread->SetRecordState (mRecording);
		switch (mInputLinkCount)
		{
			case 1:		mDngWriterThread->SetFrameRate (NTV2_FRAMERATE_3000);		break;
			case 2:		mDngWriterThread->SetFrameRate (NTV2_FRAMERATE_6000);		break;
			case 4:		mDngWriterThread->SetFrameRate (NTV2_FRAMERATE_12000);		break;
			default:	break;
		}

		//	The worket thread will run unitl the global quit flag is set
		mDngWriterThread->Start ();
	}

	//	Create a thread to manage displaying captured frames in the preview windoe
	mPreviewThread = new NTV2RawPreview (this, mPreviewCircularBuffer);
	if (mPreviewThread)
		mPreviewThread->Start ();

}	//	StartWorkerThreads


void NTV2RawFrameGrabber::StopWorkerThreads (void)
{
	//	Set flag so that the circular buffers the threads are using will terminate
	bool oldGlobalQuit	= mGlobalQuit;
	mGlobalQuit			= true;

	//	Tell the file writer thread to stop, then delete it
	if (mDngWriterThread)
	{
		AJAStatus status;

		status = mDngWriterThread->Stop (500);
		if (status != AJA_STATUS_SUCCESS)
			qDebug () << "Could not stop DNG thread";
	}
	delete mDngWriterThread;
	mDngWriterThread = NULL;

	//	Do the same for the preview window thread
	if (mPreviewThread)
	{
		AJAStatus status;

		status = mPreviewThread->Stop (500);
		if (status != AJA_STATUS_SUCCESS)
			qDebug () << "Could not stop preview thread";
	}
	delete mPreviewThread;
	mPreviewThread = NULL;

	//	Restore previous flag setting
	mGlobalQuit = oldGlobalQuit;
}
