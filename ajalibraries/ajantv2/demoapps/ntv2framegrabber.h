/**
	@file		ntv2framegrabber.h
	@brief		Header file for the NTV2FrameGrabber class.
	@copyright	Copyright (C) 2013-2014 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef NTV2FRAMEGRABBER_H
#define NTV2FRAMEGRABBER_H

#include <QBasicTimer>
#include <QtCore>
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
	#include <QtWidgets>
#else
	#include <QtGui>
#endif
#include <QThread>
#include <QtMultimedia>
#include "ntv2card.h"
#include "ntv2enums.h"
#include "ntv2task.h"
#include "ntv2rp188.h"
#include "ajastuff/common/types.h"
#include "ajastuff/system/process.h"
#if defined (INCLUDE_AJACC)
	#include "ajacc/includes/ntv2captiondecoder608.h"
	#include "ajacc/includes/ntv2captiondecoder708.h"
#endif	//	INCLUDE_AJACC

#define QTPREVIEW_WIDGET_X (960)
#define QTPREVIEW_WIDGET_Y (540)



/**
	@brief	A QThread that captures audio/video from NTV2-compatible AJA devices and uses Qt signals to emit ARGB video frames.
			To simplify things, I assume the input signal is YCbCr, and the input goes through a color space converter (CSC) to an
			ARGB FrameStore. I can also output 2 channels of Audio to the host audio system using Qt's QAudioOutput device.
**/

class NTV2FrameGrabber : public QThread
{
	Q_OBJECT

	//	Instance Methods
	public:
		/**
			@brief	Constructs me.
			@param[in]	pInParentObject		Optionally specifies my parent object. Defaults to NULL (no parent).
		**/
						NTV2FrameGrabber (QObject * pInParentObject = NULL);

		virtual			~NTV2FrameGrabber ();		///< @brief	My destructor.

		/**
			@brief	Sets the input to be used for capture on the AJA device being used.
			@param[in]	inInputSource	Specifies the input source to be used.
		**/
		void			SetInputSource (const NTV2InputSource inInputSource);

		/**
			@brief	Enables or disables host audio playback.
			@param[in]	inWithAudio		If true, enables host audio playback;  otherwise disables it.
		**/
		inline void		SetWithAudio (const bool inWithAudio)							{mbWithAudio = inWithAudio;	mRestart = true;}

		/**
			@brief	Sets the AJA device to be used for capture.
			@param[in]	inDeviceIndex	Specifies the zero-based index number of the device to be used.
		**/
		void			SetDeviceIndex (const UWord inDeviceIndex);

		void			SetTimeCodeSource (const NTV2TCIndex inTCSource);

		UWord			GetDeviceIndex (void) const;

		/**
			@brief	Enables or disables checking for 4K/UHD video (on devices that supported 4K/UHD).
			@param[in]	inCheckFor4K	If true, enables checking for 4K/UHD video;  otherwise disables it.
		**/
		inline void		CheckFor4kInput (const bool inCheckFor4K)						{mCheckFor4K = inCheckFor4K;}

		/**
			@brief	Enables or disables deinterlacing of non-progressive video.
			@param[in]	inDeinterlace	If true, enables deinterlacing of non-progressive video;  otherwise disables it.
		**/
		inline void		SetDeinterlaceNonProgressiveVideo (const bool inDeinterlace)	{mDeinterlace = inDeinterlace;}

		inline bool		GetDeinterlaceNonProgressiveVideo (void) const					{return mDeinterlace;}	///< @return	True if deinterlacing is enabled;  otherwise false.

	protected:
		void			ClearCaptionBuffer (const bool inSignalClients = false);
		void			GrabCaptions (void);		///< @brief	Performs caption data extraction & decoding


	signals:
		/**
			@brief	This is signaled (called) when a new frame has been captured and is available for display.
			@param[in]	inImage		A QImage that contains the frame image.
			@param[in]	inClear		True if a redraw should take place -- i.e., if the frame is the first of a valid video stream,
									or if there is currently no valid video.
		**/
		void			newFrame (const QImage & inImage, const bool inClear);

		/**
			@brief	This is signaled (called) when my status string changes.
			@param[in]	inStatus	The QString containing the status message.
		**/
		void			newStatusString (const QString & inStatus);

		/**
			@brief	This is signaled (called) when my caption screen buffer changes.
			@param[in]	pInScreen	Points to the (screen) array of Utf16 characters.
		**/
		void			captionScreenChanged (const ushort * pInScreen);

	private slots:
		void			changeCaptionChannel (int id);	///< @brief	This gets called when a different NTV2Line21Channel is requested.


	protected:
		virtual void	run (void);					///< @brief	My thread function.

		bool			SetupInput (void);			///< @brief	Configures my AJA device for capture
		void			StopAutoCirculate (void);	///< @brief	Stops capturing
		void			SetupAudio (void);			///< @brief	Performs audio configuration

		/**
			@brief	Writes audio samples for channels 1 and 2 that are in the given audio buffer to my QAudioOutput device,
					which should play out on the host.

			@param	pInOutAudioBuffer	Specifies a valid, non-NULL pointer to a buffer containing the audio samples
										that were captured from my AJA device for a single frame. This is not a 'const'
										pointer because the buffer content will be changed.
										On entry, the buffer will contain 6, 8 or 16 channels of 4-byte (32-bit) audio
										samples:  0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF...
										On exit, the buffer will contain unsigned 2-byte (16-bit) samples in LRLRLRLR order,
										which is what Qt's QAudioOutput object is expecting. (The least significant 16 bits
										of each sample from the AJA device are discarded.)

			@param	inNumValidBytes		Specifies the number of valid bytes that are in the audio buffer.
		**/
		void OutputAudio (ULWord * pInOutAudioBuffer, const ULWord inNumValidBytes);

		bool CheckForValidInput (void);

		NTV2VideoFormat GetVideoFormatFromInputSource (void);

		bool IsInput3Gb (const NTV2InputSource inputSource);

	//	Instance Data
	private:
		bool						mRestart;				///< @brief	Set true to reconfigure me and restart AutoCirculate
		bool						mAbort;					///< @brief	Used in my destructor to immediately cause me to exit
		bool						mCheckFor4K;			///< @brief	Check for 4K/UHD video?
		bool						mDeinterlace;			///< @brief	De-interlace non-progressive video?

		CNTV2Card					mNTV2Card;				///< @brief	Used to talk to monitor & control the device
		UWord						mBoardNumber;			///< @brief	Index number of the device I'm using
		NTV2DeviceID				mDeviceID;				///< @brief	Device ID of the device I'm using
		NTV2Channel					mChannel;				///< @brief	AutoCirculate capture channel
		NTV2VideoFormat				mCurrentVideoFormat;	///< @brief	Current video format seen on selected device input
		NTV2VideoFormat				mLastVideoFormat;		///< @brief	Used to detect input video format changes
		ULWord						mDebounceCounter;		///< @brief	Used for detecting stable input video
		bool						mFormatIsProgressive;	///< @brief	True if input video format is progressive (not interlaced)
		NTV2InputSource				mInputSource;			///< @brief	User-selected input source
		NTV2FrameDimensions			mFrameDimensions;		///< @brief	Frame dimensions, pixels X lines
		NTV2FrameBufferFormat		mFrameBufferFormat;		///< @brief	My frame buffer format
		AUTOCIRCULATE_TRANSFER		mTransferStruct;		///< @brief	AutoCirculate transfer object
		NTV2EveryFrameTaskMode		mSavedTaskMode;			///< @brief	Used to restore the previous task mode

		bool						mbWithAudio;			///< @brief	Capture audio?
		QAudioOutput *				mAudioOutput;			///< @brief	Used to play captured audio on host audio system
		QAudioFormat				mFormat;				///< @brief	Output audio stream format information
		QIODevice *					mAudioDevice;			///< @brief	Host audio device
		ULWord						mNumAudioChannels;		///< @brief	Number of audio channels being captured on the AJA device
		NTV2AudioSystem				mAudioSystem;			///< @brief	Audio subsystem to use

		std::string					mTimeCode;				///< @brief	Currently displayed timecode
		NTV2TCIndex					mTimeCodeSource;		///< @brief	Timecode source
		#if defined (INCLUDE_AJACC)
			CNTV2CaptionDecoder608Ptr	m608Decoder;		///< @brief	My 608 closed-caption decoder
			CNTV2CaptionDecoder708Ptr	m708Decoder;		///< @brief	My 708 closed-caption decoder
			ushort						mScreenBuffer [15][32];	///< @brief	My caption buffer

			static void					Caption608Changed (void * pInstance, const NTV2Caption608ChangeInfo & inChangeInfo);
			void						caption608Changed (const NTV2Caption608ChangeInfo & inChangeInfo);
		#endif	//	defined (INCLUDE_AJACC)

};	//	class NTV2FrameGrabber


#endif	//	NTV2FRAMEGRABBER_H
