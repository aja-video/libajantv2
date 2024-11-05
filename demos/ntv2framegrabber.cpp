/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2framegrabber.cpp
	@brief		Contains the implementation of the NTV2FrameGrabber class.
	@copyright	(C) 2013-2022 AJA Video Systems, Inc.  All rights reserved.
**/
#include <ostream>
#include "ntv2framegrabber.h"
#include "ntv2democommon.h"
#include "ntv2utils.h"
#if defined (INCLUDE_AJACC)
	#include "ajaanc/includes/ancillarylist.h"
	#include "ajaanc/includes/ancillarydata_cea608_line21.h"
	using namespace std;
#endif

#define NTV2_NUM_IMAGES		(10)

//	Convenience macros for EZ logging:
#define	FGFAIL(_expr_)		AJA_sERROR  (AJA_DebugUnit_DemoCapture, AJAFUNC << ": " << _expr_)
#define	FGWARN(_expr_)		AJA_sWARNING(AJA_DebugUnit_DemoCapture, AJAFUNC << ": " << _expr_)
#define	FGDBG(_expr_)		AJA_sDEBUG	(AJA_DebugUnit_DemoCapture, AJAFUNC << ": " << _expr_)
#define	FGNOTE(_expr_)		AJA_sNOTICE	(AJA_DebugUnit_DemoCapture, AJAFUNC << ": " << _expr_)
#define	FGINFO(_expr_)		AJA_sINFO	(AJA_DebugUnit_DemoCapture, AJAFUNC << ": " << _expr_)

static QMutex	gMutex;


NTV2FrameGrabber::NTV2FrameGrabber (QObject * parent)
	:	QThread					(parent),
		mRestart				(true),
		mAbort					(false),
		mCheckFor4K				(false),
		mDeinterlace			(true),
		mbFixedReference		(false),
		mBoardNumber			(0),
		mDeviceID				(DEVICE_ID_NOTFOUND),
		mChannel				(NTV2_MAX_NUM_CHANNELS),
		mNumChannels			(0),
		mTsi					(false),
		mCurrentVideoFormat		(NTV2_FORMAT_UNKNOWN),
		mCurrentColorSpace		(NTV2_LHIHDMIColorSpaceYCbCr),
		mLastVideoFormat		(NTV2_FORMAT_UNKNOWN),
		mDebounceCounter		(0),
		mFormatIsProgressive	(true),
		mInputSource			(NTV2_NUM_INPUTSOURCES),
		mFrameBufferFormat		(NTV2_FBF_ARGB),
		mDoMultiChannel			(false),
		mbWithAudio				(false),
		mAudioOutput			(AJA_NULL),
		mAudioDevice			(AJA_NULL),
		mNumAudioChannels		(0),
		mAudioSystem			(NTV2_AUDIOSYSTEM_1),
		mTimeCodeSource			(NTV2_TCINDEX_INVALID)
{
	ClearCaptionBuffer();
}	//	constructor


NTV2FrameGrabber::~NTV2FrameGrabber ()
{
	mAbort = true;
	while (isRunning())
		;	//	Wait for grabber thread to finish

	StopAutoCirculate();

	if (mAudioOutput)
		delete mAudioOutput;

}	//	destructor


void NTV2FrameGrabber::SetInputSource (const NTV2InputSource inInputSource)
{
	if (inInputSource != mInputSource)
	{
		FGDBG("From " << ::NTV2InputSourceToString (mInputSource) << " to " << ::NTV2InputSourceToString (inInputSource));
		gMutex.lock();
			mInputSource = inInputSource;
			mRestart = true;
		gMutex.unlock();
	}

}	//	SetInputSource


void NTV2FrameGrabber::SetDeviceIndex (const UWord inDeviceIndex)
{
	if (inDeviceIndex != mBoardNumber)
	{
		gMutex.lock();
			mBoardNumber = inDeviceIndex;
			mRestart = true;
		gMutex.unlock();
	}
}	//	SetDeviceIndex


void NTV2FrameGrabber::SetTimeCodeSource (const NTV2TCIndex inTCIndex)
{
	if (inTCIndex != mTimeCodeSource)
	{
		gMutex.lock();
		mTimeCodeSource = inTCIndex;
		mRestart = true;
		gMutex.unlock();
	}
}	//	SetTimeCodeSource


UWord NTV2FrameGrabber::GetDeviceIndex (void) const
{
	gMutex.lock();
	const UWord result (mBoardNumber);
	gMutex.unlock();
	return result;
}


#if defined (INCLUDE_AJACC)
	#if 0
		static const UWord gTest [15][32] =
		//           01	     02	     03	     04	     05	     06	     07	     08	     09	     10	     11	     12	     13	     14	     15	     16
		//           17	     18	     19	     20	     21	     22      23	     24	     25	     26      27	     28	     29	     30	     31	     32
		/*01*/ {{0x250C, 0x2014, 0x2014, 0x2014, 0x2014, 0x2014, 0x2014, 0x2014, 0x2014, 0x2014, 0x2014, 0x2014, 0x2014, 0x2014, 0x2014, 0x2014,
				 0x2014, 0x2014, 0x2014, 0x2014, 0x2014, 0x2014, 0x2014, 0x2014, 0x2014, 0x2014, 0x2014, 0x2014, 0x2014, 0x2014, 0x2014, 0x2510},
		/*02*/	{0x007C, 0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E,
				 0x003F, 0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x007C},
		/*03*/	{0x007C, 0x004E, 0x004F, 0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005A, 0x005B, 0x005D,
				 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F, 0x007C},
		/*04*/	{0x007C, 0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 0x0078, 0x0079, 0x007A, 0x0020, 0x00E1, 0x00E9, 0x00ED,
				 0x00F3, 0x00FA, 0x00E7, 0x00F7, 0x00D1, 0x00E1, 0x00F1, 0x2588, 0x00AE, 0x00B0, 0x00BD, 0x00BF, 0x2122, 0x00A2, 0x00A3, 0x007C},
		/*05*/	{0x007C, 0x266A, 0x00E0, 0x00A0, 0x00E8, 0x00E2, 0x00EA, 0x00EE, 0x00F4, 0x00FB, 0x00C1, 0x00C9, 0x00D3, 0x00DA, 0x00DC, 0x00FC,
				 0x2018, 0x00A1, 0x002A, 0x2019, 0x2014, 0x00A9, 0x2120, 0x2022, 0x201C, 0x201D, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x007C},
		/*06*/	{0x007C, 0x00C0, 0x00C2, 0x00C7, 0x00C8, 0x00CA, 0x00CB, 0x00EB, 0x00CE, 0x00CF, 0x00EF, 0x00D4, 0x00D9, 0x00F9, 0x00DB, 0x00AB,
				 0x00BB, 0x0020, 0x0020, 0x00C3, 0x00E3, 0x00CD, 0x00CC, 0x00EC, 0x00D2, 0x00F2, 0x00D5, 0x00F5, 0x007B, 0x007D, 0x005C, 0x007C},
		/*07*/	{0x007C, 0x005E, 0x005F, 0x007C, 0x007E, 0x0020, 0x0020, 0x00C4, 0x00E4, 0x00D6, 0x00F6, 0x00DF, 0x00A5, 0x00A4, 0x007C, 0x00C5,
				 0x00E5, 0x00D8, 0x00F8, 0x00C4, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x007C},
		/*08*/	{0x007C, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020,
				 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x007C},
		/*09*/	{0x007C, 0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E,
				 0x003F, 0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x007C},
		/*10*/	{0x007C, 0x004E, 0x004F, 0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005A, 0x005B, 0x005D,
				 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F, 0x007C},
		/*11*/	{0x007C, 0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 0x0078, 0x0079, 0x007A, 0x0020, 0x00E1, 0x00E9, 0x00ED,
				 0x00F3, 0x00FA, 0x00E7, 0x00F7, 0x00D1, 0x00E1, 0x00F1, 0x2588, 0x00AE, 0x00B0, 0x00BD, 0x00BF, 0x2122, 0x00A2, 0x00A3, 0x007C},
		/*12*/	{0x007C, 0x266A, 0x00E0, 0x00A0, 0x00E8, 0x00E2, 0x00EA, 0x00EE, 0x00F4, 0x00FB, 0x00C1, 0x00C9, 0x00D3, 0x00DA, 0x00DC, 0x00FC,
				 0x2018, 0x00A1, 0x002A, 0x2019, 0x2014, 0x00A9, 0x2120, 0x2022, 0x201C, 0x201D, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x007C},
		/*13*/	{0x007C, 0x00C0, 0x00C2, 0x00C7, 0x00C8, 0x00CA, 0x00CB, 0x00EB, 0x00CE, 0x00CF, 0x00EF, 0x00D4, 0x00D9, 0x00F9, 0x00DB, 0x00AB,
				 0x00BB, 0x0020, 0x0020, 0x00C3, 0x00E3, 0x00CD, 0x00CC, 0x00EC, 0x00D2, 0x00F2, 0x00D5, 0x00F5, 0x007B, 0x007D, 0x005C, 0x007C},
		/*14*/	{0x007C, 0x005E, 0x005F, 0x007C, 0x007E, 0x0020, 0x0020, 0x00C4, 0x00E4, 0x00D6, 0x00F6, 0x00DF, 0x00A5, 0x00A4, 0x007C, 0x00C5,
				 0x00E5, 0x00D8, 0x00F8, 0x00C4, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x007C},
		/*15*/	{0x2514, 0x2014, 0x2014, 0x2014, 0x2014, 0x2014, 0x2014, 0x2014, 0x2014, 0x2014, 0x2014, 0x2014, 0x2014, 0x2014, 0x2014, 0x2014,
				 0x2014, 0x2014, 0x2014, 0x2014, 0x2014, 0x2014, 0x2014, 0x2014, 0x2014, 0x2014, 0x2014, 0x2014, 0x2014, 0x2014, 0x2014, 0x2518}};
	#endif	//	0
#endif	//	defined (INCLUDE_AJACC)


void NTV2FrameGrabber::ClearCaptionBuffer (const bool inNotifyClients)
{
	#if defined (INCLUDE_AJACC)
		for (UWord row (NTV2_CC608_MinRow);  row <= NTV2_CC608_MaxRow;  row++)
			for (UWord col (NTV2_CC608_MinCol);  col <= NTV2_CC608_MaxCol;  col++)
				mScreenBuffer [row-1][col-1] = 0x0020;	//	gTest[row-1][col-1];
		if (inNotifyClients)
			emit captionScreenChanged (&mScreenBuffer[0][0]);
	#else
		(void) inNotifyClients;
	#endif
}


void NTV2FrameGrabber::GrabCaptions (void)
{
	#if defined (INCLUDE_AJACC)
		NTV2_ASSERT (mNTV2Card.IsOpen ());
		NTV2_ASSERT (mDeviceID != DEVICE_ID_NOTFOUND);

		if (mNTV2Card.features().CanDoCustomAnc())
		{
			bool			gotCaptionPacket	(false);
			CaptionData		captionData;		//	The two byte pairs (one pair per field) our 608 decoder is looking for
			ostringstream	oss;				//	DEBUG

			//	See what's in the AUTOCIRCULATE_TRANSFER's F1 & F2 anc buffers...
			AJAAncillaryList	ancPacketsF1, ancPacketsF2;
			ancPacketsF1.AddReceivedAncillaryData ((uint8_t *) mTransferStruct.acANCBuffer.GetHostPointer (), mTransferStruct.acANCBuffer.GetByteCount ());
			ancPacketsF2.AddReceivedAncillaryData ((uint8_t *) mTransferStruct.acANCField2Buffer.GetHostPointer (), mTransferStruct.acANCField2Buffer.GetByteCount ());
			ancPacketsF1.ParseAllAncillaryData ();
			ancPacketsF2.ParseAllAncillaryData ();

			if (ancPacketsF1.CountAncillaryDataWithType (AJAAncillaryDataType_Cea708))
			{
				AJAAncillaryData	ancCEA708DataIn	(ancPacketsF1.GetAncillaryDataWithType (AJAAncillaryDataType_Cea708));
				if (ancCEA708DataIn.GetPayloadData () && ancCEA708DataIn.GetPayloadByteCount ())
					gotCaptionPacket = m708Decoder ? m708Decoder->SetSMPTE334AncData (ancCEA708DataIn.GetPayloadData (), ancCEA708DataIn.GetPayloadByteCount ()) : false;
				//if (gotCaptionPacket)		oss << "Using F1 AJAAnc CEA708 packet(s)";
			}
			if (ancPacketsF1.CountAncillaryDataWithType (AJAAncillaryDataType_Cea608_Vanc))
			{
				AJAAncillaryData	ancCEA608DataIn	(ancPacketsF1.GetAncillaryDataWithType (AJAAncillaryDataType_Cea608_Vanc));
				if (ancCEA608DataIn.GetPayloadData () && ancCEA608DataIn.GetPayloadByteCount ())
					gotCaptionPacket = m708Decoder ? m708Decoder->SetSMPTE334AncData (ancCEA608DataIn.GetPayloadData (), ancCEA608DataIn.GetPayloadByteCount ()) : false;
				//if (gotCaptionPacket)		oss << "Using F1 AJAAnc CEA608 packet(s)";
			}
			if (ancPacketsF1.CountAncillaryDataWithType (AJAAncillaryDataType_Cea608_Line21))
			{
				AJAAncillaryData_Cea608_Line21	ancEIA608DataIn	(ancPacketsF1.GetAncillaryDataWithType (AJAAncillaryDataType_Cea608_Line21));
				if (AJA_SUCCESS (ancEIA608DataIn.ParsePayloadData ()))
					if (AJA_SUCCESS (ancEIA608DataIn.GetCEA608Bytes (captionData.f1_char1, captionData.f1_char2, captionData.bGotField1Data)))
						gotCaptionPacket = true;
				//if (gotCaptionPacket)		oss << "Using F1 AJAAnc EIA608 packet(s)";
			}
			if (ancPacketsF2.CountAncillaryDataWithType (AJAAncillaryDataType_Cea608_Line21))
			{
				AJAAncillaryData_Cea608_Line21	ancEIA608DataIn	(ancPacketsF2.GetAncillaryDataWithType (AJAAncillaryDataType_Cea608_Line21));
				if (AJA_SUCCESS (ancEIA608DataIn.ParsePayloadData ()))
					if (AJA_SUCCESS (ancEIA608DataIn.GetCEA608Bytes (captionData.f2_char1, captionData.f2_char2, captionData.bGotField2Data)))
						gotCaptionPacket = true;
				//if (gotCaptionPacket)		oss << "Using F2 AJAAnc EIA608 packet(s)";
			}

			if (NTV2_IS_HD_VIDEO_FORMAT (mCurrentVideoFormat))
			{
				if (gotCaptionPacket  &&  m708Decoder  &&  m708Decoder->ParseSMPTE334AncPacket ())
					captionData = m708Decoder->GetCC608CaptionData ();	//	Extract the 608 caption byte pairs
			}	//	else if HD

			//	Sorry, we only handle 608 captions -- nothing else.
			//	The 608 decoder does the rest. It expects to be called once per frame (to implement flashing characters and roll-up).
			//	Pass the caption byte pairs to it for processing (even if captionData.HasData returns false)...
			if (m608Decoder)
				m608Decoder->ProcessNew608FrameData (captionData);
			//if (!oss.str ().empty ())	cerr << oss.str () << endl;	//	DEBUG DEBUG DEBUG DEBUG
			//if (captionData.HasData ())	cerr << captionData << endl;
		}	//	if able to use Anc buffers
	#endif	//	defined (INCLUDE_AJACC)
}	//	GrabCaptions


void NTV2FrameGrabber::changeCaptionChannel (int inNewCaptionChannelId)
{
	#if defined (INCLUDE_AJACC)
		const NTV2Line21Channel	chosenCaptionChannel	(static_cast <NTV2Line21Channel> (inNewCaptionChannelId));

		if (!m608Decoder && IsValidLine21Channel (chosenCaptionChannel))
		{
			NTV2_ASSERT (!m708Decoder);
			if (CNTV2CaptionDecoder608::Create (m608Decoder) && CNTV2CaptionDecoder708::Create (m708Decoder))
			{
				m608Decoder->SubscribeChangeNotification (Caption608Changed, this);
				m608Decoder->SetDisplayChannel (chosenCaptionChannel);
				ClearCaptionBuffer (true);
			}
			else
				{m608Decoder = AJA_NULL;	m708Decoder = AJA_NULL;}
		}
		else if (m608Decoder)
		{
			NTV2_ASSERT (m708Decoder);
			if (!IsValidLine21Channel (chosenCaptionChannel))
				{m608Decoder = AJA_NULL;	m708Decoder = AJA_NULL;}
			else if (m608Decoder->GetDisplayChannel () != chosenCaptionChannel)
				m608Decoder->SetDisplayChannel (chosenCaptionChannel);
			ClearCaptionBuffer (true);
		}
	#else
		(void) inNewCaptionChannelId;
	#endif	//	defined (INCLUDE_AJACC)
}	//	changeCaptionChannel


void NTV2FrameGrabber::run (void)
{
	//	Set up 2 images to ping-pong between capture and display...
	QImage *	images [NTV2_NUM_IMAGES];
	ULWord		framesCaptured	(0);
	FGNOTE("Thread started");

	//	Initialize images
	for (int i(0);  i < NTV2_NUM_IMAGES;  i++)
		images[i] = new QImage(QTPREVIEW_WIDGET_X, QTPREVIEW_WIDGET_Y, QImage::Format_RGB32);

	// Make sure all Bidirectionals are set to Inputs.
	if (mNTV2Card.Open(mBoardNumber))
	{
		if (!mDoMultiChannel && !mNTV2Card.AcquireStreamForApplicationWithReference (kDemoAppSignature, int32_t(AJAProcess::GetPid())))
		{
			//	We have not acquired the board continue until something changes...
			FGWARN("Could not acquire " << mNTV2Card.GetDescription());
			mNTV2Card.Close ();
			mDeviceID = DEVICE_ID_NOTFOUND;
		}
		else
		{
			mNTV2Card.GetEveryFrameServices (mSavedTaskMode);	//	Save the current state before we change it
			mNTV2Card.SetEveryFrameServices (NTV2_OEM_TASKS);	//	Since this is an OEM demo we will set the OEM service level

			if (mNTV2Card.features().HasBiDirectionalSDI())		//	If device has bidirectional SDI connectors...
			{
				bool waitForInput = false;
				for (unsigned offset(0);  offset < 8;  offset++)
				{
					mNTV2Card.EnableChannel (NTV2Channel(offset));
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
					mNTV2Card.WaitForInputVerticalInterrupt (mChannel, 10);	//	...and give the device some time to lock to a signal
			}
		}
	}

	while (true)
	{
		if (mAbort)
			break;

		if (!NTV2_IS_VALID_INPUT_SOURCE(mInputSource))
		{
			//	No input chosen, so display banner...
			if (NTV2_IS_VALID_CHANNEL(mChannel))
				StopAutoCirculate();
			QImage * currentImage (images[framesCaptured % NTV2_NUM_IMAGES]);
			currentImage->load (":/resources/splash.png");
			emit newStatusString ("");
			emit newFrame (*currentImage, true);
			AJATime::Sleep(200);
			continue;
		}

		if (mRestart)
		{
			gMutex.lock();
				StopAutoCirculate();
				if (mNTV2Card.IsOpen())
				{
					if (!mDoMultiChannel)
					{
						mNTV2Card.ReleaseStreamForApplicationWithReference (kDemoAppSignature, AJAProcess::GetPid ());
						mNTV2Card.SetEveryFrameServices (mSavedTaskMode);
					}
					mNTV2Card.Close();
					mDeviceID = DEVICE_ID_NOTFOUND;
				}
			gMutex.unlock();

			if (mNTV2Card.Open(mBoardNumber))
			{
				if (!mDoMultiChannel  &&  !mNTV2Card.AcquireStreamForApplicationWithReference (kDemoAppSignature, int32_t(AJAProcess::GetPid())))
				{
					//	Haven't acquired the device --- continue til other app releases it...
					FGWARN(mNTV2Card.GetDisplayName() << " is busy");
					QImage * currentImage (images[framesCaptured % NTV2_NUM_IMAGES]);
					currentImage->fill(Qt::lightGray);
					QString	status (QString("%1 is busy").arg(mNTV2Card.GetDisplayName().c_str()));
					emit newStatusString(status);
					emit newFrame(*currentImage, true);
					mNTV2Card.Close();
					mDeviceID = DEVICE_ID_NOTFOUND;
					AJATime::Sleep(500);
					continue;
				}

				mNTV2Card.GetEveryFrameServices(mSavedTaskMode);	//	Save the current state before we change it
				mNTV2Card.SetEveryFrameServices(NTV2_OEM_TASKS);	//	Since this is an OEM demo we will set the OEM service level

				mDeviceID = mNTV2Card.GetDeviceID();
				if (mNTV2Card.features().CanDoMultiFormat())
					mNTV2Card.SetMultiFormatMode(true);

                if (!mNTV2Card.IsDeviceReady(false))
				{
					FGWARN(mNTV2Card.GetDisplayName() << " not ready");
					QImage * currentImage (images[framesCaptured % NTV2_NUM_IMAGES]);
					currentImage->fill(Qt::lightGray);
					QString	status (QString("%1 is not ready").arg(mNTV2Card.GetDisplayName().c_str()));
					emit newStatusString(status);
					emit newFrame(*currentImage, true);
					AJATime::Sleep(1000);	//	Keep the UI responsive while waiting for device to become ready
				}
				else if (!mNTV2Card.features().CanDoCapture())
				{
					FGWARN(mNTV2Card.GetDisplayName() << " is output-only device");
					QImage * currentImage (images[framesCaptured % NTV2_NUM_IMAGES]);
					currentImage->fill(qRgba(40, 40, 40, 255));
					QString	status (QString("%1 is output-only device").arg(mNTV2Card.GetDisplayName().c_str()));
					emit newStatusString(status);
					emit newFrame(*currentImage, true);
					AJATime::Sleep(1000);	//	Keep the UI responsive
				}
				else if (!mNTV2Card.features().GetNumCSCs())
				{
					FGWARN(mNTV2Card.GetDisplayName() << " has no CSCs, won't work with NTV2FrameGrabber");
					QImage * currentImage (images[framesCaptured % NTV2_NUM_IMAGES]);
					currentImage->fill(qRgba(40, 40, 40, 255));
					QString	status (QString("%1 has no CSCs, won't work for this demo").arg(mNTV2Card.GetDisplayName().c_str()));
					emit newStatusString(status);
					emit newFrame(*currentImage, true);
					AJATime::Sleep(1000);	//	Keep the UI responsive
				}
				else if (SetupInput())
				{
					mTransferStruct.acANCBuffer.Allocate (mNTV2Card.features().CanDoCustomAnc() ? NTV2_ANCSIZE_MAX : 0);		//	Reserve space for anc data
					mTransferStruct.acANCField2Buffer.Allocate(mNTV2Card.features().CanDoCustomAnc() ? NTV2_ANCSIZE_MAX : 0);	//	Reserve space for anc data
					const ULWord acOptions(AUTOCIRCULATE_WITH_RP188  |  (mTransferStruct.acANCBuffer.IsNULL() && mTransferStruct.acANCField2Buffer.IsNULL() ? 0 : AUTOCIRCULATE_WITH_ANC));
					gMutex.lock();
						StopAutoCirculate();
						ULWord numFrameBuffersAvailable = (mNTV2Card.DeviceGetNumberFrameBuffers() - mNTV2Card.features().GetNumAudioSystems());
						ULWord startFrameBuffer = (numFrameBuffersAvailable / mNTV2Card.features().GetNumFrameStores()) * ULWord(mChannel);
						mNTV2Card.AutoCirculateInitForInput (mChannel, 0, ::NTV2ChannelToAudioSystem(mChannel),
															acOptions, 1, startFrameBuffer, startFrameBuffer+7);
					gMutex.unlock();
					SetupAudio();
					if (mAudioOutput)
						mAudioDevice = mAudioOutput->start();

					//	Start AutoCirculate...
					mNTV2Card.SetLTCInputEnable(true);
					mNTV2Card.SetRP188SourceFilter (mChannel, 0);	//	0=LTC 1=VITC1 2=VITC2
					mNTV2Card.AutoCirculateStart(mChannel);

					for (int i(0);  i < NTV2_NUM_IMAGES;  i++)
					{
						delete images[i];
						images[i] = new QImage (mFrameDimensions.Width(), mFrameDimensions.Height(), QImage::Format_RGB32);
					}

					framesCaptured = 0;
					mRestart = false;
				}	//	if board set up ok
				else
					AJATime::Sleep(1000);	//	This keeps the UI responsive if/while this channel has no input
			}	//	if board opened ok
			else
			{
				FGWARN("Open failed for device " << DEC(GetDeviceIndex()));
				AJATime::Sleep(200);
				continue;
			}

			emit newStatusString(::NTV2InputSourceToString (mInputSource, true).c_str());
		}	//	if mRestart

		if (!CheckForValidInput()  &&  NTV2_IS_VALID_INPUT_SOURCE(mInputSource))
		{
			QImage * currentImage (images[framesCaptured % NTV2_NUM_IMAGES]);
			currentImage->fill(qRgba(40, 40, 40, 255));
			QString	status (QString("%1: No Detected Input").arg(::NTV2InputSourceToString(mInputSource, true).c_str()));
			emit newStatusString(status);
			emit newFrame(*currentImage, true);
			AJATime::Sleep(200);
			continue;
		}

		AUTOCIRCULATE_STATUS acStatus;
		mNTV2Card.AutoCirculateGetStatus (mChannel, acStatus);
		if (acStatus.acState == NTV2_AUTOCIRCULATE_RUNNING && acStatus.acBufferLevel > 1)
		{
			QImage *			currentImage	(images [framesCaptured % NTV2_NUM_IMAGES]);
			NTV2TimeCodeList	tcValues;

			#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
				mTransferStruct.SetVideoBuffer (reinterpret_cast<PULWord>(currentImage->bits()), ULWord(currentImage->sizeInBytes()));
			#else
				mTransferStruct.SetVideoBuffer (reinterpret_cast<PULWord>(currentImage->bits()), ULWord(currentImage->byteCount()));
			#endif
			mNTV2Card.AutoCirculateTransfer (mChannel, mTransferStruct);
			if (!mFormatIsProgressive && mDeinterlace)
			{
				//	Eliminate field flicker by copying even lines to odd lines...
				//	(a very lame de-interlace technique)
				if (currentImage->height() == int(mFrameDimensions.Height())  &&  currentImage->width() == int(mFrameDimensions.Width()))
					for (ULWord line (0);  line < mFrameDimensions.Height();  line += 2)
						::memcpy (currentImage->scanLine (int(line) + 1),  currentImage->scanLine(int(line)),  mFrameDimensions.Width() * 4);
			}
			GrabCaptions();
			mTimeCode.clear ();
			if (mTransferStruct.acTransferStatus.acFrameStamp.GetInputTimeCodes(tcValues) && size_t(mTimeCodeSource) < tcValues.size())
			{
				CRP188	tc	(tcValues.at(mTimeCodeSource));
				if (!tcValues.at (mTimeCodeSource).IsValid())
				{
					//	If the requested timecode was invalid, check for embedded LTC
					if (NTV2_IS_VALID_INPUT_SOURCE(mInputSource))
						tc.SetRP188 (tcValues.at (::NTV2InputSourceToTimecodeIndex (mInputSource, true)));
				}
				tc.GetRP188Str (mTimeCode);
			}

			QString outString (::NTV2InputSourceToString (mInputSource, true).c_str());
			outString.append ("  ");
			outString.append (::NTV2VideoFormatToString (mCurrentVideoFormat).c_str ());
			outString.append ("  ");
			outString.append (mTimeCode.c_str());

			emit newStatusString (outString);

			emit newFrame (*currentImage, (framesCaptured == 0) ? true : false);
			if (mbWithAudio && mTransferStruct.acAudioBuffer)
				OutputAudio (mTransferStruct.acAudioBuffer, mTransferStruct.acTransferStatus.acAudioTransferSize);

			framesCaptured++;
		}	//	if running and at least one frame ready to transfer
		else
			mNTV2Card.WaitForInputVerticalInterrupt (mChannel);
	}	//	loop til break

	if (mNTV2Card.IsOpen ())
	{
		gMutex.lock();
		StopAutoCirculate();
		if (!mDoMultiChannel)
		{
			mNTV2Card.ReleaseStreamForApplicationWithReference (kDemoAppSignature, int32_t(AJAProcess::GetPid()));	//	Release the device
			mNTV2Card.SetEveryFrameServices(mSavedTaskMode);	//	Restore prior task mode
		}
		gMutex.unlock();
	}

	for (int i(0);  i < NTV2_NUM_IMAGES;  i++)
		delete images[i];

	FGNOTE("Thread completed, will exit for " << mNTV2Card.GetDisplayName() << " input " << ::NTV2InputSourceToString(mInputSource));

}	//	run


bool NTV2FrameGrabber::SetupInput (void)
{
	if (!mNTV2Card.IsOpen())
		return false;
	if (mDeviceID == DEVICE_ID_NOTFOUND)
		return false;

	mChannel = ::NTV2InputSourceToChannel(mInputSource);
	if (mChannel == NTV2_CHANNEL_INVALID)
		mChannel = NTV2_CHANNEL1;
	mTimeCodeSource = ::NTV2InputSourceToTimecodeIndex(mInputSource);

	bool waitForInput(false);
	if (mNTV2Card.features().HasBiDirectionalSDI())	//	If device has bidirectional SDIs...
		for (unsigned offset(0);  offset < 4;  offset++)
		{
			mNTV2Card.EnableChannel(NTV2Channel(mChannel + offset));
			bool outputEnabled;
			mNTV2Card.GetSDITransmitEnable (NTV2Channel(mChannel + offset), outputEnabled);
			if (outputEnabled)
			{
				waitForInput = true;
				mNTV2Card.SetSDITransmitEnable (NTV2Channel(mChannel + offset), false);
			}
		}

	//	Only if we had to change an output to input do we need to wait.
	if (waitForInput)
		mNTV2Card.WaitForInputVerticalInterrupt (mChannel, 10);	//	Give the device ~10 fields/frames to lock to a signal

	mCurrentVideoFormat = GetVideoFormatFromInputSource();
	mCurrentColorSpace = GetColorSpaceFromInputSource();
	mFrameDimensions.Set (QTPREVIEW_WIDGET_X, QTPREVIEW_WIDGET_Y);
	if (!NTV2_IS_VALID_VIDEO_FORMAT(mCurrentVideoFormat))
		return false;	//	No video!

	mNTV2Card.SetVideoFormat (mCurrentVideoFormat, false, false, mChannel);
	NTV2VANCMode vm(NTV2_VANCMODE_INVALID);
	mNTV2Card.GetVANCMode (vm, mChannel);
	const NTV2FormatDescriptor fd (mCurrentVideoFormat, mFrameBufferFormat, vm);
	mFrameDimensions.Set (fd.GetRasterWidth(), fd.GetRasterHeight());
	const QString vfString (::NTV2VideoFormatToString(mCurrentVideoFormat).c_str());
	FGDBG("mInputSource=" << mChannel << " mCurrentVideoFormat=" << vfString.toStdString()
			<< " wdth=" << mFrameDimensions.Width() << " hght=" << mFrameDimensions.Height());

	mFormatIsProgressive = IsProgressivePicture(mCurrentVideoFormat);
	if (!mbFixedReference)
		mNTV2Card.SetReference(NTV2_REFERENCE_FREERUN);

	if (NTV2_INPUT_SOURCE_IS_SDI(mInputSource))
	{
		mNumChannels = 0;
		mTsi = false;

		bool is6g(false), is12g(false);
		mNTV2Card.GetSDIInput6GPresent(is6g, mChannel);
		mNTV2Card.GetSDIInput12GPresent(is12g, mChannel);

		if (mNTV2Card.features().CanDo12gRouting() && (is6g || is12g))
		{
			mNTV2Card.Connect (::GetCSCInputXptFromChannel(mChannel), ::GetSDIInputOutputXptFromChannel(mChannel));
			mNTV2Card.Connect (::GetFrameBufferInputXptFromChannel(mChannel), ::GetCSCOutputXptFromChannel (mChannel, false/*isKey*/, true/*isRGB*/));
			mNTV2Card.SetFrameBufferFormat (mChannel, mFrameBufferFormat);
			mNTV2Card.EnableChannel(mChannel);
			mNTV2Card.SetSDIInLevelBtoLevelAConversion (mChannel, IsInput3Gb(mInputSource) ? true : false);
		}
		else for (unsigned offset(0);  offset < 4;  offset++)
		{
			mNumChannels++;
			mNTV2Card.Connect (::GetCSCInputXptFromChannel (NTV2Channel(mChannel + offset)), ::GetSDIInputOutputXptFromChannel(NTV2Channel(mChannel + offset)));
			mNTV2Card.Connect (::GetFrameBufferInputXptFromChannel (NTV2Channel(mChannel + offset)), ::GetCSCOutputXptFromChannel(NTV2Channel(mChannel + offset), false/*isKey*/, true/*isRGB*/));
			mNTV2Card.SetFrameBufferFormat (NTV2Channel(mChannel + offset), mFrameBufferFormat);
			mNTV2Card.EnableChannel (NTV2Channel(mChannel + offset));
			mNTV2Card.SetSDIInLevelBtoLevelAConversion (mChannel + offset, IsInput3Gb(mInputSource) ? true : false);
			if (!NTV2_IS_4K_VIDEO_FORMAT(mCurrentVideoFormat))
				break;
			mNTV2Card.Set4kSquaresEnable(true, NTV2_CHANNEL1);
		}
	}
		else if (mInputSource == NTV2_INPUTSOURCE_ANALOG1)
		{
			mNumChannels = 0;
			mTsi = false;
			//mNTV2Card.SetTsiFrameEnable(false, NTV2_CHANNEL1);

			mNTV2Card.Connect (::GetCSCInputXptFromChannel(NTV2_CHANNEL1), NTV2_XptAnalogIn);
			mNTV2Card.Connect (::GetFrameBufferInputXptFromChannel(NTV2_CHANNEL1), ::GetCSCOutputXptFromChannel(NTV2_CHANNEL1, false/*isKey*/, true/*isRGB*/));
			mNTV2Card.SetFrameBufferFormat(NTV2_CHANNEL1, mFrameBufferFormat);
			if (!mbFixedReference)
				mNTV2Card.SetReference(NTV2_REFERENCE_ANALOG_INPUT1);
			mChannel = NTV2_CHANNEL1;
			mNumChannels = 1;
		}
		else if (NTV2_INPUT_SOURCE_IS_HDMI(mInputSource))
		{
			mNumChannels = 0;
			mTsi = false;

			if (!mbFixedReference)
				mNTV2Card.SetReference(::NTV2InputSourceToReferenceSource(mInputSource));

			// configure hdmi with 2.0 support
			if (mNTV2Card.features().CanDo12gRouting())
			{
				mNumChannels = 1;
				mNTV2Card.EnableChannel(mChannel);
				mNTV2Card.SetMode (mChannel, NTV2_MODE_CAPTURE);
				mNTV2Card.SetFrameBufferFormat (mChannel, mFrameBufferFormat);
				if (mCurrentColorSpace == NTV2_LHIHDMIColorSpaceYCbCr)
				{
					mNTV2Card.Connect (::GetCSCInputXptFromChannel(mChannel),
										::GetInputSourceOutputXpt (mInputSource, false/*isSDI_DS2*/, false/*isHDMI_RGB*/, 0/*hdmiQuadrant*/));
					mNTV2Card.Connect (::GetFrameBufferInputXptFromChannel(mChannel),
										::GetCSCOutputXptFromChannel (mChannel, false/*isKey*/, true/*isRGB*/));
				}
				else
					mNTV2Card.Connect (::GetFrameBufferInputXptFromChannel(mChannel),
										::GetInputSourceOutputXpt (mInputSource, false/*isSDI_DS2*/, true/*isHDMI_RGB*/, 0/*hdmiQuadrant*/));
			}
			else if (NTV2_IS_4K_VIDEO_FORMAT(mCurrentVideoFormat) && !mNTV2Card.features().CanDoHDMIQuadRasterConversion())
			{
				//	Set two sample interleave
				mChannel = NTV2_CHANNEL1;
				mNTV2Card.SetTsiFrameEnable(true, NTV2_CHANNEL1);

				for (NTV2Channel channel (NTV2_CHANNEL1);  channel < NTV2_CHANNEL3;  channel = NTV2Channel(channel+1))
				{
					mNTV2Card.EnableChannel(channel);
					mNTV2Card.SetMode (channel, NTV2_MODE_CAPTURE);
					mNTV2Card.SetFrameBufferFormat (channel, mFrameBufferFormat);
				}

				if (mCurrentColorSpace == NTV2_LHIHDMIColorSpaceYCbCr)
				{
					mNTV2Card.Connect (NTV2_XptCSC1VidInput,
										::GetInputSourceOutputXpt (mInputSource, false/*isSDI_DS2*/, false/*isHDMI_RGB*/, NTV2_CHANNEL1/*hdmiQuadrant*/));
					mNTV2Card.Connect (NTV2_XptCSC2VidInput,
										::GetInputSourceOutputXpt (mInputSource, false/*isSDI_DS2*/, false/*isHDMI_RGB*/, NTV2_CHANNEL2/*hdmiQuadrant*/));
					mNTV2Card.Connect (NTV2_XptCSC3VidInput,
										::GetInputSourceOutputXpt (mInputSource, false/*isSDI_DS2*/, false/*isHDMI_RGB*/, NTV2_CHANNEL3/*hdmiQuadrant*/));
					mNTV2Card.Connect (NTV2_XptCSC4VidInput,
										::GetInputSourceOutputXpt (mInputSource, false/*isSDI_DS2*/, false/*isHDMI_RGB*/, NTV2_CHANNEL4/*hdmiQuadrant*/));

					mNTV2Card.Connect (NTV2_Xpt425Mux1AInput, NTV2_XptCSC1VidRGB);
					mNTV2Card.Connect (NTV2_Xpt425Mux1BInput, NTV2_XptCSC2VidRGB);
					mNTV2Card.Connect (NTV2_Xpt425Mux2AInput, NTV2_XptCSC3VidRGB);
					mNTV2Card.Connect (NTV2_Xpt425Mux2BInput, NTV2_XptCSC4VidRGB);
				}
				else
				{
					mNTV2Card.Connect (NTV2_Xpt425Mux1AInput, ::GetInputSourceOutputXpt (mInputSource, false/*isSDI_DS2*/, true/*isHDMI_RGB*/, NTV2_CHANNEL1/*hdmiQuadrant*/));
					mNTV2Card.Connect (NTV2_Xpt425Mux1BInput, ::GetInputSourceOutputXpt (mInputSource, false/*isSDI_DS2*/, true/*isHDMI_RGB*/, NTV2_CHANNEL2/*hdmiQuadrant*/));
					mNTV2Card.Connect (NTV2_Xpt425Mux2AInput, ::GetInputSourceOutputXpt (mInputSource, false/*isSDI_DS2*/, true/*isHDMI_RGB*/, NTV2_CHANNEL3/*hdmiQuadrant*/));
					mNTV2Card.Connect (NTV2_Xpt425Mux2BInput, ::GetInputSourceOutputXpt (mInputSource, false/*isSDI_DS2*/, true/*isHDMI_RGB*/, NTV2_CHANNEL4/*hdmiQuadrant*/));
				}

				mNTV2Card.Connect (NTV2_XptFrameBuffer1Input, NTV2_Xpt425Mux1AYUV);
				mNTV2Card.Connect (NTV2_XptFrameBuffer1DS2Input, NTV2_Xpt425Mux1BYUV);
				mNTV2Card.Connect (NTV2_XptFrameBuffer2Input, NTV2_Xpt425Mux2AYUV);
				mNTV2Card.Connect (NTV2_XptFrameBuffer2DS2Input, NTV2_Xpt425Mux2BYUV);

				mNumChannels = 2;
				mTsi = true;
			}	//	if 4K and can't do HDMI quad raster conversion
			else if (NTV2_IS_4K_VIDEO_FORMAT(mCurrentVideoFormat) && mNTV2Card.features().CanDoHDMIQuadRasterConversion())
			{
				mNumChannels = 0;
				mNTV2Card.SetTsiFrameEnable(false, NTV2_CHANNEL1);
				for (NTV2Channel channel(NTV2_CHANNEL1);  channel < NTV2_CHANNEL5;  channel = NTV2Channel(channel+1))
				{
					mNumChannels++;
					mNTV2Card.EnableChannel(channel);
					mNTV2Card.SetMode (channel, NTV2_MODE_CAPTURE);
					mNTV2Card.SetFrameBufferFormat (channel, mFrameBufferFormat);
					if (mCurrentColorSpace == NTV2_LHIHDMIColorSpaceYCbCr)
					{
						mNTV2Card.Connect (::GetCSCInputXptFromChannel(channel),
											::GetInputSourceOutputXpt (mInputSource, false/*isSDI_DS2*/, false/*isHDMI_RGB*/, channel/*hdmiQuadrant*/));
						mNTV2Card.Connect (::GetFrameBufferInputXptFromChannel(channel),
											::GetCSCOutputXptFromChannel (channel, false/*isKey*/, true/*isRGB*/));
					}
					else
						mNTV2Card.Connect (::GetFrameBufferInputXptFromChannel(channel),
											::GetInputSourceOutputXpt (mInputSource, false/*isSDI_DS2*/, true/*isHDMI_RGB*/, channel/*hdmiQuadrant*/));
				}	//	loop once for each channel (4 times for 4K/UHD)
			}	//	else if 4K and can do HDMI quad raster conversion
			else
			{
				mNumChannels = 1;
				mNTV2Card.EnableChannel(mChannel);
				mNTV2Card.SetMode (mChannel, NTV2_MODE_CAPTURE);
				mNTV2Card.SetFrameBufferFormat (mChannel, mFrameBufferFormat);
				if (mCurrentColorSpace == NTV2_LHIHDMIColorSpaceYCbCr)
				{
					mNTV2Card.Connect (::GetCSCInputXptFromChannel(mChannel),
										::GetInputSourceOutputXpt (mInputSource, false/*isSDI_DS2*/, false/*isHDMI_RGB*/, 0/*hdmiQuadrant*/));
					mNTV2Card.Connect (::GetFrameBufferInputXptFromChannel(mChannel),
										::GetCSCOutputXptFromChannel (mChannel, false/*isKey*/, true/*isRGB*/));
				}
				else
					mNTV2Card.Connect (::GetFrameBufferInputXptFromChannel(mChannel),
										::GetInputSourceOutputXpt (mInputSource, false/*isSDI_DS2*/, true/*isHDMI_RGB*/, 0/*hdmiQuadrant*/));
			}	//	else not 4K

			// configure the qrc if present
			if (mNTV2Card.features().GetHDMIVersion() == 2)
				mNTV2Card.SetHDMIV2Mode (NTV2_IS_4K_VIDEO_FORMAT(mCurrentVideoFormat) ? NTV2_HDMI_V2_4K_CAPTURE : NTV2_HDMI_V2_HDSD_BIDIRECTIONAL);
		}
		else
			FGWARN("Bad mInputSource value " << DEC(mInputSource));

	return true;

}	//	SetupInput


void NTV2FrameGrabber::StopAutoCirculate (void)
{
	if (mNTV2Card.IsOpen())
	{
		mNTV2Card.AutoCirculateStop(mChannel);

		bool tsiEnable(false);
		mNTV2Card.GetTsiFrameEnable (tsiEnable, NTV2_CHANNEL1);
		for (ULWord i(0);  i < (tsiEnable ? 2 : mNumChannels);  i++)
			mNTV2Card.SetMode (NTV2Channel(mChannel + i), NTV2_MODE_DISPLAY);
	}
	ClearCaptionBuffer(true);

}	//	StopAutoCirculate


bool NTV2FrameGrabber::CheckForValidInput (void)
{
	NTV2VideoFormat	videoFormat	(GetVideoFormatFromInputSource());
	NTV2LHIHDMIColorSpace colorSpace (GetColorSpaceFromInputSource());

	if (videoFormat == NTV2_FORMAT_UNKNOWN)
	{
		mCurrentVideoFormat = videoFormat;
		return false;
	}	//	if no video or unknown format

	if ((mCurrentVideoFormat != videoFormat) || (mCurrentColorSpace != colorSpace))
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
	}	//	if video format changed

	return true;

}	//	CheckForValidInput


NTV2VideoFormat NTV2FrameGrabber::GetVideoFormatFromInputSource (void)
{
	if (!mNTV2Card.IsOpen())
		return NTV2_FORMAT_UNKNOWN;
	if (mDeviceID == DEVICE_ID_NOTFOUND)
		return NTV2_FORMAT_UNKNOWN;

	NTV2VideoFormat	videoFormat(NTV2_FORMAT_UNKNOWN);
	if (NTV2_INPUT_SOURCE_IS_SDI(mInputSource))
	{
		bool is6g (false), is12g(false);
		NTV2Channel channel (::NTV2InputSourceToChannel(mInputSource));
		mNTV2Card.GetSDIInput6GPresent(is6g, channel);
		mNTV2Card.GetSDIInput12GPresent(is12g, channel);
		if (mNTV2Card.features().CanDo12gRouting() && (is6g || is12g))
		{
			videoFormat = mNTV2Card.GetInputVideoFormat(mInputSource);
			return videoFormat;
		}
	}

	switch (mInputSource)
	{
		case NTV2_INPUTSOURCE_SDI1:
		case NTV2_INPUTSOURCE_SDI5:
			{
				const ULWord ndx (::GetIndexForNTV2InputSource(mInputSource));
				videoFormat = mNTV2Card.GetInputVideoFormat(::GetNTV2InputSourceForIndex(ndx + 0));
				NTV2Standard videoStandard (::GetNTV2StandardFromVideoFormat(videoFormat));
				if (mCheckFor4K  &&  (videoStandard == NTV2_STANDARD_1080p))
				{
					NTV2VideoFormat	videoFormatNext	(mNTV2Card.GetInputVideoFormat(::GetNTV2InputSourceForIndex(ndx + 1)));
					if (videoFormatNext == videoFormat)
					{
						videoFormatNext = mNTV2Card.GetInputVideoFormat(::GetNTV2InputSourceForIndex(ndx + 2));
						if (videoFormatNext == videoFormat)
						{
							videoFormatNext = mNTV2Card.GetInputVideoFormat(::GetNTV2InputSourceForIndex(ndx + 3));
							if (videoFormatNext == videoFormat)
								CNTV2DemoCommon::Get4KInputFormat(videoFormat);
						}
					}
				}
			}
			break;

		case NTV2_INPUTSOURCE_INVALID:
			break;		//	indicates no source is currently selected

		default:
			videoFormat = mNTV2Card.GetInputVideoFormat(mInputSource);
			break;
	}
	return videoFormat;

}	//	GetVideoFormatFromInputSource


NTV2LHIHDMIColorSpace NTV2FrameGrabber::GetColorSpaceFromInputSource (void)
{
	if (NTV2_INPUT_SOURCE_IS_HDMI(mInputSource))
	{
		NTV2LHIHDMIColorSpace hdmiColor (NTV2_LHIHDMIColorSpaceRGB);
		mNTV2Card.GetHDMIInputColor (hdmiColor, mChannel);
		return hdmiColor;
	}
	return NTV2_LHIHDMIColorSpaceYCbCr;
}


bool NTV2FrameGrabber::IsInput3Gb (const NTV2InputSource inputSource)
{
	bool is3Gb(false);
	mNTV2Card.GetSDIInput3GbPresent (is3Gb, ::NTV2InputSourceToChannel(inputSource));
	return is3Gb;
}	//	IsInput3Gb


void NTV2FrameGrabber::SetupAudio (void)
{
	if (!mNTV2Card.IsOpen())
		return;
	if (mDeviceID == DEVICE_ID_NOTFOUND)
		return;

	mTransferStruct.acAudioBuffer.Allocate(NTV2_AUDIOSIZE_MAX);	//	Reserve space for largest audio packet/frame
	if (mAudioOutput)
	{
		delete mAudioOutput;
		mAudioOutput = AJA_NULL;
	}

	NTV2AudioSource	audioSource	(NTV2_AUDIO_EMBEDDED);
	if (NTV2_INPUT_SOURCE_IS_HDMI(mInputSource))
		audioSource = NTV2_AUDIO_HDMI;
	else if (NTV2_INPUT_SOURCE_IS_ANALOG(mInputSource))
		audioSource = NTV2_AUDIO_ANALOG;

	//	Set up AJA device audio...
	mNumAudioChannels = mNTV2Card.features().GetMaxAudioChannels();
	mAudioSystem = ::NTV2ChannelToAudioSystem(mChannel);
	mNTV2Card.SetAudioSystemInputSource (mAudioSystem, audioSource, ::NTV2InputSourceToEmbeddedAudioInput(mInputSource));
	mNTV2Card.SetNumberAudioChannels (mNumAudioChannels, mAudioSystem);
	mNTV2Card.SetAudioRate (NTV2_AUDIO_48K, mAudioSystem);
	mNTV2Card.SetAudioBufferSize (NTV2_AUDIO_BUFFER_BIG, mAudioSystem);

	//	Set up Qt's audio output...
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
	mFormat.setSampleRate(48000);
	mFormat.setChannelCount(2);
	mFormat.setSampleSize(16);
	mFormat.setCodec("audio/pcm");
	mFormat.setByteOrder(QAudioFormat::LittleEndian);
	mFormat.setSampleType(QAudioFormat::SignedInt);
#else
	mFormat.setSampleRate(48000);
	mFormat.setChannelCount(2);
	mFormat.setSampleFormat(QAudioFormat::Int16);
#endif

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
	QAudioDeviceInfo audioDeviceInfo (QAudioDeviceInfo::defaultOutputDevice());
	if (audioDeviceInfo.isFormatSupported(mFormat))
		mAudioOutput = new QAudioOutput (mFormat, AJA_NULL);
#else
	QAudioDevice deviceInfo (QMediaDevices::defaultAudioOutput());
	if (deviceInfo.isFormatSupported(mFormat))
		mAudioOutput = new QAudioSink(deviceInfo, mFormat, AJA_NULL);
#endif
}	//	SetupAudio


void NTV2FrameGrabber::OutputAudio (ULWord * pInOutBuffer, const ULWord inNumValidBytes)
{
	const ULWord	nBytesPerAJASample	(4);	//	AJA devices provide four bytes per channel per sample
	unsigned		channel				(0);	//	Current channel being processed
	UWord			qtSampleNdx			(0);	//	Which Qt audio sample is being processed
	const ULWord	totalSamples		(inNumValidBytes / mNumAudioChannels);		//	Total number of audio samples to process, regardless of channel count
	const ULWord	totalAjaSamples		(inNumValidBytes / nBytesPerAJASample);		//	Total number of AJA-device-provided audio samples, one sample per channel
	UWord *			pQtAudioBuffer		(reinterpret_cast<UWord*>(pInOutBuffer));	//	Qt-centric pointer to the audio buffer (2-bytes per channel per sample)

	//
	//	Walk through the audio buffer channel by channel:
	//	0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 0 1 2 3 4 5 ...etc...
	//	and copy samples from channels 0 and 1 up to the front of the buffer...
	//	0 1 0 1 0 1 0 1 0 1 0 1 ...etc..
	//
	for (unsigned ajaSampleNdx(0);  ajaSampleNdx < totalAjaSamples;  ajaSampleNdx++)
	{
		if (channel < 2)
		{
			pQtAudioBuffer [qtSampleNdx] = pInOutBuffer [ajaSampleNdx] >> 16;	//	Also discard the least significant 16 bits of each sample
			qtSampleNdx++;
		}	//	if channel 0 or 1

		channel++;
		if (channel == mNumAudioChannels)
			channel = 0;
	}	//	for each channel for each sample

	if (mAudioDevice)
		mAudioDevice->write (reinterpret_cast<const char*>(pQtAudioBuffer), totalSamples);

}	//	OutputAudio


#if defined (INCLUDE_AJACC)
	void NTV2FrameGrabber::Caption608Changed (void * pInstance, const NTV2Caption608ChangeInfo & inChangeInfo)
	{
		NTV2FrameGrabber *	pFG	(reinterpret_cast <NTV2FrameGrabber *> (pInstance));
		if (pFG)
			pFG->caption608Changed (inChangeInfo);
	}

	void NTV2FrameGrabber::caption608Changed (const NTV2Caption608ChangeInfo & inChangeInfo)
	{
		NTV2Line21Attributes	attrs;
		bool					changed	(false);
		for (UWord row (NTV2_CC608_MinRow);  row <= NTV2_CC608_MaxRow;  row++)
			for (UWord col (NTV2_CC608_MinCol);  col <= NTV2_CC608_MaxCol;  col++)
			{
				ushort	utf16Char	(m608Decoder ? m608Decoder->GetOnAirUTF16CharacterWithAttributes (row, col, attrs) : 0x0020);
				if (!utf16Char)
					utf16Char = 0x0020;
				if (utf16Char != mScreenBuffer [row-1][col-1])
				{
					mScreenBuffer [row-1][col-1] = utf16Char;
					changed = true;
				}
			}
		if (changed)
			emit captionScreenChanged (&mScreenBuffer[0][0]);
	}
#endif	//	defined (INCLUDE_AJACC)
