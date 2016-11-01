#if defined(AJA_WINDOWS)
#include <windows.h>
#else
#include "sys/types.h"
#include "sys/stat.h"
//#include "sys/dirent.h"
#endif

#include <string>
#include <sstream>

#include "ajastuff/common/dpx_hdr.h"
#include "ajastuff/common/dpxfileio.h"
#include "ajastuff/system/file_io.h"
#include "ajastuff/system/systemtime.h"

#include "ajapreviewwidget.h"
#include "ntv2capture.h"
#include "ntv2card.h"
#include "ntv2debug.h"
#include "ntv2playcorder.h"
#include "ntv2player.h"
#include "ntv2utils.h"
#include "ntv2videoformataja.h"

using std::string;
using std::vector;

QString findNextSequenceDirectory(QString currentDirectory);

static NTV2Playcorder *	sSingleton = NULL;


IPlaycorder * IPlaycorder::GetIPlaycorder ()
{
	return NTV2Playcorder::GetIPlaycorder ();
}


NTV2Playcorder::NTV2Playcorder ()
	:	mBoardIndex(0),
		mPlayback(NULL),
		mIsPlaying(false),
		mRecord(NULL),
		mIsRecording(false),
		mIsWritingToStorage(false),
		mNTV2VideoFormatAJA(NULL),
		mAjaDpxFileIO(NULL),
		mAjaFileIO(NULL),
		mSequenceNumber(0),
		mInputFormat(NTV2_FORMAT_UNKNOWN),
		mPlayFormat(NTV2_FORMAT_UNKNOWN),
		mPreviewCircularBufferAbort(false),
		mPreviewWidth(0),
		mPreviewHeight(0)
{
}


IPlaycorder * NTV2Playcorder::GetIPlaycorder ()
{
	if (!sSingleton)
	{
		sSingleton = new NTV2Playcorder ();
		return (IPlaycorder*) sSingleton;
	}

	// Allow ony one parent to have an instance
	return NULL;
}


NTV2Playcorder::~NTV2Playcorder ()
{
	StopRecording ();
	StopPlaying ();

	delete mPlayback;
	delete mRecord;

	delete mNTV2VideoFormatAJA;
	delete mAjaDpxFileIO;
	delete mAjaFileIO;

	sSingleton = NULL;
}


//////////////////////////////////////////////
//  Start of the IPlaycorder methods
//////////////////////////////////////////////


AJAStatus NTV2Playcorder::Init (const uint32_t previewWidth, const uint32_t previewHeight)
{
	mPreviewWidth  = previewWidth;
	mPreviewHeight = previewHeight;

	mAjaFileIO		= new AJAFileIO();
	mAjaDpxFileIO	= new AJADPXFileIO;
	mNTV2VideoFormatAJA	= new NTV2VideoFormatAJA;

	SetupDeviceNames ();
	SetupPreviewCircularBuffer ();

	return AJA_STATUS_SUCCESS;
}


uint32_t NTV2Playcorder::GetDeviceCount () const
{
	return mDeviceNames.size () - 1;
}	//	GetDeviceCount


const string &	NTV2Playcorder::GetDeviceString (const uint32_t index)
{
	if ((size_t)index < mDeviceNames.size () - 1)
		return mDeviceNames [index + 1];
	else
		return mDeviceNames [0];
}	//	GetDeviceString


AJAStatus NTV2Playcorder::SetDevice (const uint32_t index)
{
	if (index != mBoardIndex)
	{
		mBoardIndex = index;

		StopAll ();

		delete mPlayback;
		mPlayback = NULL;
		delete mRecord;
		mRecord = NULL;
	}

	return AJA_STATUS_SUCCESS;
}


AJA_VideoFormat	NTV2Playcorder::GetInputFormat () const
{
	NTV2VideoFormat ntv2VideoFormat = mRecord->GetVideoFormat ();

	return mNTV2VideoFormatAJA->GetAJAVideoFormat (ntv2VideoFormat);
}	//	GetInputFormat


const string &	NTV2Playcorder::GetInputFormatString ()
{
	NTV2VideoFormat currentFormat = mInputFormat;
	mInputFormat = mRecord->GetVideoFormat ();

	if (currentFormat != mInputFormat)
	{
		if (mIsRecording)
		{
			StopRecording ();
			delete mRecord;

			ClearPreviewBuffers ();

			// Create a new instance that will configure itself for the new video format
			mRecord	= new NTV2Capture (mBoardIndex, true, NTV2_CHANNEL1, NTV2_FBF_10BIT_DPX_LITTLEENDIAN);
			StartRecording ();
		}
		// Things like number of pixels and lines need to change in the header
		InitializeDpxHdr ();
	}

	mInputFormatString = NTV2VideoFormatStrings [mInputFormat];

	return mInputFormatString;
}	//	GetInputFormatString


bool NTV2Playcorder::IsPlaying () const
{
	return mIsPlaying;
}	//	IsPlaying


AJAStatus NTV2Playcorder::StartPlaying (uint32_t & numberOfFrames)
{
	//	Read the first frame of the file to determine the video format so the
	//	player can be told how to configure the board
	mAjaDpxFileIO->SetPath (mPlayPath);

	GetVideoFormatFromFile ();

	if (!mPlayback)
	{
		mPlayback	= new NTV2Player (mBoardIndex,
									  true,
									  NTV2_CHANNEL2,
									  NTV2_FBF_10BIT_DPX_LITTLEENDIAN,
									  NTV2_OUTPUTDESTINATION_SDI2,
									  mPlayFormat);
	}

	if (!mPlayback)
		return AJA_STATUS_MEMORY;

	numberOfFrames = mAjaDpxFileIO->GetFileCount ();
	ClearPreviewBuffers ();

	mPlayback->SetCallback (this, ProcessPlaybackFrame);
	mPlayback->Init ();
	mPlayback->Run ();

	mIsPlaying = true;

	return AJA_STATUS_SUCCESS;
}	//	StartPlaying


bool NTV2Playcorder::GetPlayPaused () const
{
	return mAjaDpxFileIO->GetPauseMode ();
}	//	GetPlayPaused


AJAStatus NTV2Playcorder::SetPlayPaused (const bool pause)
{
	if (pause)
	{
		mAjaDpxFileIO->SetPauseMode (true);
	}
	else
	{
		mAjaDpxFileIO->SetPauseMode (false);
	}

	return AJA_STATUS_SUCCESS;
}	//	SetPlayPaused


uint32_t NTV2Playcorder::GetPlayFrame () const
{
	return mAjaDpxFileIO->GetIndex ();
}	//	GetPlayFrame


AJAStatus NTV2Playcorder::SetPlayFrame (const uint32_t number)
{
	mAjaDpxFileIO->SetIndex (number);
	mSequenceNumber = number;

	return AJA_STATUS_SUCCESS;
}	//	SetPlayFrame


AJAStatus NTV2Playcorder::StopPlaying ()
{
	if (!mPlayback)
		return AJA_STATUS_SUCCESS;

	mPlayback->Quit ();
	mPlayback->SetCallback (NULL, NULL);

	delete mPlayback;
	mPlayback = NULL;

	mIsPlaying = false;

	return AJA_STATUS_SUCCESS;
}	//	StopPlaying


AJAStatus NTV2Playcorder::SetPlayPath (const string path)
{
	mPlayPath = path;
	return AJA_STATUS_SUCCESS;
}	//	SetPlayPath


uint32_t NTV2Playcorder::GetPlayDropCount () const
{
	AUTOCIRCULATE_STATUS_STRUCT acInfo;

	mPlayback->GetACStatus (acInfo);

	return acInfo.framesDropped;
}	//	GetPlayDropCount


bool NTV2Playcorder::IsRecording () const
{
	return mIsRecording;
}	//	IsRecording


bool NTV2Playcorder::IsWritingToStorage () const
{
	return mIsWritingToStorage;
}	//	IsWritingToStorage


AJAStatus NTV2Playcorder::StartRecording ()
{
	if (!mRecord)
	{
		mRecord	= new NTV2Capture (mBoardIndex, true, NTV2_CHANNEL1, NTV2_FBF_10BIT_DPX_LITTLEENDIAN);
	}

	if (!mRecord)
		return AJA_STATUS_MEMORY;

	mSequenceNumber = 0;
	InitializeDpxHdr ();

	mAjaDpxFileIO->SetPath (mRecordPath);
	mAjaDpxFileIO->SetLoopMode (true);

	mRecord->Init ();
	mRecord->SetCallback( this, ProcessCaptureFrame );
	mRecord->Run ();

	mIsRecording = true;

	return AJA_STATUS_SUCCESS;
}	//	StartRecording


bool NTV2Playcorder::GetRecordPaused () const
{
	printf("GetRecordPaused unimplemented\n");
	return false;
}	//	GetRecordPaused


AJAStatus NTV2Playcorder::SetRecordPaused (const bool pause)
{
	printf("SetRecordPaused to %d is unimplemented\n", pause);
	return AJA_STATUS_SUCCESS;
}	//	SetRecordPaused


AJAStatus NTV2Playcorder::StopRecording ()
{
	if (!mRecord)
		return AJA_STATUS_SUCCESS;

	mPreviewCircularBufferAbort = true;

	mRecord->Quit ();
	mRecord->SetCallback( NULL, NULL );

	delete mRecord;
	mRecord = NULL;

	mIsRecording = false;
	return AJA_STATUS_SUCCESS;
}	//	StopRecording


AJAStatus NTV2Playcorder::SetRecordPath (const string path)
{
	mRecordPath = path;
	mAjaDpxFileIO->SetPath (mRecordPath);

	return AJA_STATUS_SUCCESS;
}	//	SetRecordPath

AJAStatus NTV2Playcorder::WriteToStorage (const bool enable)
{
	mIsWritingToStorage = enable;
	return AJA_STATUS_SUCCESS;
}	//	WriteToStorage


uint32_t NTV2Playcorder::GetRecordDropCount () const
{
	AUTOCIRCULATE_STATUS_STRUCT acInfo;

	mRecord->GetACStatus (acInfo);

	return acInfo.framesDropped;
}	//	GetRecordDropCount


AJAStatus NTV2Playcorder::StopAll ()
{
	mIsPlaying = false;
	mIsRecording = false;

	return AJA_STATUS_SUCCESS;
}	//	StopAll


AJACircularBuffer<IPlaycorder::PreviewFrameInfo *> * NTV2Playcorder::GetPreviewCircularBuffer ()
{
	return &mPreviewCircularBuffer;
}


//////////////////////////////////////////////
//  End of the IPlaycorder methods
//////////////////////////////////////////////


void NTV2Playcorder::SetupDeviceNames ()
{
	mDeviceNames.clear ();
	mDeviceNames.push_back("");

	CNTV2BoardScan boardScan;
	if (boardScan.GetNumBoards () != 0)
	{
		BoardInfoList & boardList (boardScan.GetBoardList ());

		for (BoardInfoList::const_iterator iter = boardList.begin (); iter != boardList.end (); ++iter)
		{
			mDeviceNames.push_back ((*iter).boardIdentifier);
		}
	}
}


void NTV2Playcorder::SetupPreviewCircularBuffer ()
{
	uint32_t previewBufferSize = mPreviewWidth * mPreviewHeight * 3;
	mPreviewCircularBuffer.SetAbortFlag (&mPreviewCircularBufferAbort);

	//	Allocate and add each in-host PreviewFrameInfo to my circular buffer member variable...
	for (unsigned bufferNdx = 0; bufferNdx < CIRCULAR_BUFFER_SIZE; bufferNdx++ )
	{
		mPreviewFrames [bufferNdx].pPreviewFrame = new uint8_t [previewBufferSize];
		mPreviewFrames [bufferNdx].previewFrameSize = previewBufferSize;

		mPreviewCircularBuffer.Add (& mPreviewFrames [bufferNdx]);
	}	//	for each PreviewFrameInfo
}


AJAStatus NTV2Playcorder::ProcessCaptureFrame(void * instance, const AVDataBuffer * const playData)
{
	NTV2Playcorder * pInstance = (NTV2Playcorder*) instance;

	if ( !pInstance->mPreviewWidth || !pInstance->mPreviewHeight)
		return AJA_STATUS_INITIALIZE;

	// Get an available preview buffer
	IPlaycorder::PreviewFrameInfo * currentFrame = pInstance->mPreviewCircularBuffer.StartProduceNextBuffer ();
	if( currentFrame )
	{
		if (NTV2_IS_HD_VIDEO_FORMAT (pInstance->mInputFormat))
		{
			pInstance->HDToPreview (playData->fVideoBuffer,
									currentFrame->pPreviewFrame,
									pInstance->mPreviewWidth,
									pInstance->mPreviewHeight);
		}
		else if (NTV2_IS_SD_VIDEO_FORMAT (pInstance->mInputFormat))
		{
			pInstance->SDToPreview (playData->fVideoBuffer,
									currentFrame->pPreviewFrame,
									pInstance->mPreviewWidth,
									pInstance->mPreviewHeight);
		}
		else
		{
			pInstance->mPreviewCircularBuffer.EndProduceNextBuffer ();
			return AJA_STATUS_SUCCESS;
		}

		if (pInstance->mIsWritingToStorage)
		{
			pInstance->WriteDpxFrame( (uint8_t*)playData->fVideoBuffer, playData->fVideoBufferSize );
		}

		pInstance->mPreviewCircularBuffer.EndProduceNextBuffer ();
	}

	return AJA_STATUS_SUCCESS;
}


AJAStatus NTV2Playcorder::ProcessPlaybackFrame(void * instance, const AVDataBuffer * const playData)
{
	NTV2Playcorder * pInstance = (NTV2Playcorder*) instance;

	if ( !pInstance->mPreviewWidth || !pInstance->mPreviewHeight)
		return AJA_STATUS_INITIALIZE;

	AJAStatus status = AJA_STATUS_SUCCESS;

	// To maintain low latency, block the producer thread until the board
	// is almost out of frames to display.
	static const uint32_t desiredBufferLevel = 3;	// If this is TOO small, frames could be dropped
	AUTOCIRCULATE_STATUS_STRUCT acInfo;

	while(1)
	{
		// Release thread if we are no longer playing
		if( !pInstance->IsPlaying () )
			return status;

		pInstance->mPlayback->GetACStatus (acInfo);
		if( acInfo.bufferLevel < desiredBufferLevel )
			break;

		AJATime::Sleep (10);
	}

	status = pInstance->mAjaDpxFileIO->Read( (uint8_t &) *playData->fVideoBuffer,
											 playData->fVideoBufferSize,
											 pInstance->mSequenceNumber);
	if (AJA_STATUS_SUCCESS != status)
	{
		printf("Error %d reading frame %d\n", status, pInstance->mSequenceNumber);
		return status;
	}

	// Get an available preview buffer
	IPlaycorder::PreviewFrameInfo * currentFrame = pInstance->mPreviewCircularBuffer.StartProduceNextBuffer ();
	if( currentFrame )
	{
		if (NTV2_IS_HD_VIDEO_FORMAT (pInstance->mPlayFormat))
		{
			pInstance->HDToPreview ((uint32_t*)playData->fVideoBuffer,
									currentFrame->pPreviewFrame,
									pInstance->mPreviewWidth,
									pInstance->mPreviewHeight);
		}
		else if (NTV2_IS_SD_VIDEO_FORMAT (pInstance->mPlayFormat))
		{
			pInstance->SDToPreview ((uint32_t*)playData->fVideoBuffer,
									currentFrame->pPreviewFrame,
									pInstance->mPreviewWidth,
									pInstance->mPreviewHeight);
		}
		else
		{
			pInstance->mPreviewCircularBuffer.EndProduceNextBuffer ();
			return AJA_STATUS_SUCCESS;
		}

		pInstance->mPreviewCircularBuffer.EndProduceNextBuffer ();
	}

	return status;
}


void NTV2Playcorder::InitializeDpxHdr()
{
	// Get info about input format in use
	NTV2FormatDescriptor fd = GetFormatDescriptor (GetNTV2StandardFromVideoFormat (mInputFormat),
												   NTV2_FBF_10BIT_DPX_LITTLEENDIAN,
												   false,						// No vanc enabled
												   Is2KFormat (mInputFormat),
												   false);						// No wide vanc

	// SMPTE 286 requires all unused header values to be set to 0xFF
	memset( &mAjaDpxFileIO->GetHdr (), 0xFF, mAjaDpxFileIO->GetHdrSize () );

	// The following fields are not optional and must be filled in

	// Magic number
	mAjaDpxFileIO->init (DPX_C_MAGIC);		// Force little endian for now (most dpx files are)

	// Offset to image data in bytes
	mAjaDpxFileIO->set_fi_image_offset (mAjaDpxFileIO->GetHdrSize ());

	// Version number of header format
	mAjaDpxFileIO->set_fi_version ("V2.0");

	// Total image file size in bytes
	mAjaDpxFileIO->set_fi_file_size (fd.numPixels * fd.numLines * 4 +
									 mAjaDpxFileIO->GetHdrSize ());

	// Image orientation
	mAjaDpxFileIO->set_ii_orientation (0);					// Left to right, top to bottom

	// Number of image elements
	mAjaDpxFileIO->set_ii_element_number (1);

	// Pixels per line
	mAjaDpxFileIO->set_ii_pixels (fd.numPixels);

	// Lines per image element
	mAjaDpxFileIO->set_ii_lines (fd.numLines);

	// Data sign
	mAjaDpxFileIO->set_ie_data_sign (0);

	// Descriptor
	mAjaDpxFileIO->set_ie_descriptor (50);					// RGB

	if (NTV2_IS_SD_VIDEO_FORMAT (mInputFormat))
	{
		NTV2FrameRate fr = GetNTV2FrameRateFromVideoFormat (mInputFormat);
		if ( (fr == NTV2_FRAMERATE_2500) || (fr == NTV2_FRAMERATE_5000) )
		{
			// Transfer characteristic
			mAjaDpxFileIO->set_ie_transfer		(7);		// ITU_R 601-5 (625)
			// Colorimetric specification
			mAjaDpxFileIO->set_ie_colorimetric	(7);		// ITU_R 601-5 (625)
		}
		else
		{
			// Transfer characteristic
			mAjaDpxFileIO->set_ie_transfer		(8);		// ITU_R 601-5 (525)
			// Colorimetric specification
			mAjaDpxFileIO->set_ie_colorimetric	(8);		// ITU_R 601-5 (525)
		}
	}
	else
	{
		// Transfer characteristic
		mAjaDpxFileIO->set_ie_transfer		(6);			// ITU_R 709-4
		// Colorimetric specification
		mAjaDpxFileIO->set_ie_colorimetric	(6);			// ITU_R 709-4
	}

	// Bit depth
	mAjaDpxFileIO->set_ie_bit_size (10);					// 10 bits per pixel

	// Packing
	mAjaDpxFileIO->set_ie_packing (1);						// 32 bit packing, method A

	// Encoding
	mAjaDpxFileIO->set_ie_encoding (0);						// No encoding

	// Offset to data
	mAjaDpxFileIO->set_ie_data_offset (mAjaDpxFileIO->GetHdrSize ());
}


AJAStatus NTV2Playcorder::WriteDpxFrame( uint8_t* pBuffer, uint32_t bufferSize )
{
	AJAStatus status = AJA_STATUS_SUCCESS;

	status = mAjaDpxFileIO->Write( (const uint8_t &)	*pBuffer,
								   (const uint32_t &)	bufferSize,
														mSequenceNumber);
	if (AJA_STATUS_SUCCESS != status)
	{
		printf("Error %d writing frame %d\n", status, mSequenceNumber);
		return status;
	}

	mSequenceNumber++;

	return status;
}


void NTV2Playcorder::HDToPreview (const uint32_t* pSrcBuff,
								  uint8_t* pPreviewBuff,
								  const uint32_t width,
								  const uint32_t height)
{
	// Convert AJA 10 bit DPX little endian (10 R + 10 G + 10 B + 2 spare)
	// to QT RGB888 (8 R + 8 G + 8 B)
	for( uint32_t i = 0; i < height; i++ )
	{
		for( uint32_t j = 0; j < width; j++ )
		{
			uint32_t src = *pSrcBuff;

			*pPreviewBuff++ = src >> 24;	// Red
			*pPreviewBuff++ = src >> 14;	// Green
			*pPreviewBuff++ = src >>  4;	// Blue

			pSrcBuff += 2;
		}

		pSrcBuff += width * 2;
	}
}


void NTV2Playcorder::SDToPreview (const uint32_t* pSrcBuff,
								  uint8_t* pPreviewBuff,
								  const uint32_t width,
								  const uint32_t height)
{
	pPreviewBuff += ((((height - 486) / 2) * width) + 120) * 3;

	for( uint32_t i = 0; i < 486; i++ )
	{
		for( uint32_t j = 0; j < 720; j++ )
		{
			uint32_t src = *pSrcBuff++;

			*pPreviewBuff++ = src >> 24;	// Red
			*pPreviewBuff++ = src >> 14;	// Green
			*pPreviewBuff++ = src >>  4;	// Blue
		}

		pPreviewBuff += (width - 720) * 3;
	}
}


AJAStatus NTV2Playcorder::GetVideoFormatFromFile ()
{
	AJAStatus	returnStatus = AJA_STATUS_SUCCESS;

	uint32_t frameToRead = 0;
	returnStatus = mAjaDpxFileIO->Read (frameToRead);

	if (AJA_STATUS_SUCCESS != returnStatus)
	{
		printf("GetVideoFormatFromFile error %d reading frame 0\n", returnStatus);
		return returnStatus;
	}

	size_t lineCount = mAjaDpxFileIO->get_ii_lines ();
	if ((lineCount == 525) || (lineCount == 486) || (lineCount == 480))
	{
		mPlayFormat = NTV2_FORMAT_525_5994;
	}
	else if (lineCount == 625)
	{
		mPlayFormat = NTV2_FORMAT_625_5000;
	}
	else
	{
		mPlayFormat = NTV2_FORMAT_1080i_5994;
	}

	return returnStatus;	
}


void NTV2Playcorder::ClearPreviewBuffers ()
{
	//	Fill preview buffers with black in case the new format is smaller
	for (unsigned bufferNdx = 0; bufferNdx < CIRCULAR_BUFFER_SIZE; bufferNdx++ )
	{
		memset (mPreviewFrames [bufferNdx].pPreviewFrame,
				0x00,
				mPreviewFrames [bufferNdx].previewFrameSize);
	}
}

