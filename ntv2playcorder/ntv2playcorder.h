/**
		@file			ntv2playcorder.h  
		@brief			Header file for NTV2Playcorder class.
						This class supplies an NTV2 implementation of the IPlaycorder class
						for the NTV2QtPlaycorder UI.
		@copyright		Copyright 2013 AJA Video Systems, Inc. All rights reserved. 
**/

#ifndef NTV2PLAYCORDER_H
#define NTV2PLAYCORDER_H


#include <string>
#include <vector>

#include "ajatypes.h"
#include "iplaycorder.h"
#include "ntv2capture.h"			// Needed for the static callback function below
#include "ntv2player.h"				// Needed for the static callback function below

class NTV2VideoFormatAJA;
class AJADPXFileIO;
class AJAFileIO;
class NTV2Capture;
class NTV2Player;

 
class NTV2Playcorder : public IPlaycorder
{

public:

	virtual ~NTV2Playcorder	();

	// Required IPlaycorder implementations
	virtual			AJAStatus			Init (const uint32_t previewWidth,
											  const uint32_t previewHeight);

	virtual			uint32_t			GetDeviceCount () const;
	virtual const	std::string &		GetDeviceString (const uint32_t index);
	virtual			AJAStatus			SetDevice (const uint32_t index);

	virtual			AJA_VideoFormat		GetInputFormat () const;
	virtual const	std::string &		GetInputFormatString ();

	virtual			bool				IsPlaying () const;
	virtual			AJAStatus			StartPlaying (uint32_t & clipLength);
	virtual			bool				GetPlayPaused () const;
	virtual			AJAStatus			SetPlayPaused (const bool pause);
	virtual			uint32_t			GetPlayFrame () const;
	virtual			AJAStatus			SetPlayFrame (const uint32_t number);
	virtual			AJAStatus			StopPlaying ();
	virtual			AJAStatus			SetPlayPath (const std::string path);
	virtual			uint32_t			GetPlayDropCount () const;

	virtual			bool				IsRecording () const;
	virtual			bool				IsWritingToStorage () const;
	virtual			AJAStatus			StartRecording ();
	virtual			bool				GetRecordPaused () const;
	virtual			AJAStatus			SetRecordPaused (const bool pause);
	virtual			AJAStatus			StopRecording ();
	virtual			AJAStatus			SetRecordPath (const std::string path);
	virtual			AJAStatus			WriteToStorage (const bool enable);
	virtual			uint32_t			GetRecordDropCount () const;

	virtual			AJAStatus			StopAll ();

	virtual			AJACircularBuffer<IPlaycorder::PreviewFrameInfo *> *
										GetPreviewCircularBuffer ();

	// End of required IPlaycorder implementations

	static			IPlaycorder *		GetIPlaycorder ();

protected:

	void			InitializeDpxHdr();
	AJAStatus		WriteDpxFrame( uint8_t* pBuffer, uint32_t bufferSize );

private:

	NTV2Playcorder();

	void			SetupDeviceNames ();
	void			SetupPreviewCircularBuffer ();

	void			HDToPreview (const uint32_t* pSrcBuff,
								 uint8_t* pPreviewBuff,
								 const uint32_t width,
								 const uint32_t height);
	void			SDToPreview (const uint32_t* pSrcBuff,
								 uint8_t* pPreviewBuff,
								 const uint32_t width,
								 const uint32_t height);
	void			ClearPreviewBuffers ();

	AJAStatus		GetVideoFormatFromFile ();

	uint32_t											mBoardIndex;
	std::vector<std::string>							mDeviceNames;

	NTV2Player*											mPlayback;
	bool												mIsPlaying;
	NTV2Capture*										mRecord;
	bool												mIsRecording;
	bool												mIsWritingToStorage;

	NTV2VideoFormatAJA*									mNTV2VideoFormatAJA;
	AJADPXFileIO*										mAjaDpxFileIO;
	AJAFileIO*											mAjaFileIO;
	uint32_t											mSequenceNumber;

	NTV2VideoFormat										mInputFormat;
	NTV2VideoFormat										mPlayFormat;
	std::string											mInputFormatString;
	std::string											mPlayPath;
	std::string											mRecordPath;

	IPlaycorder::PreviewFrameInfo						mPreviewFrames [CIRCULAR_BUFFER_SIZE];
	AJACircularBuffer<IPlaycorder::PreviewFrameInfo *> 	mPreviewCircularBuffer;
	bool												mPreviewCircularBufferAbort;
	uint32_t											mPreviewWidth;
	uint32_t											mPreviewHeight;

	static	NTV2Capture::NTV2CaptureCallback 			ProcessCaptureFrame;
	static	NTV2Player::NTV2PlayerCallback 				ProcessPlaybackFrame;
};

#endif // NTV2PLAYCORDER_H

