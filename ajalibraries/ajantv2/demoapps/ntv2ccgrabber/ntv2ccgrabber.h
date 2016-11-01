/**
	@file		ntv2ccgrabber.h
	@brief		Header file for NTV2CCGrabber demonstration class
	@copyright	Copyright (C) 2013-2016 AJA Video Systems, Inc.  All rights reserved.
**/


#ifndef _NTV2CCGRABBER_H
#define _NTV2CCGRABBER_H

#include "ntv2enums.h"
#include "ntv2devicefeatures.h"
#include "ntv2devicescanner.h"
#include "ntv2democommon.h"
#include "ntv2task.h"
#include "ntv2utils.h"
#include "ajastuff/common/types.h"
#include "ajastuff/common/videotypes.h"
#include "ajastuff/common/timecode.h"
#include "ajastuff/common/timecodeburn.h"
#include "ajastuff/common/circularbuffer.h"
#include "ajastuff/system/thread.h"
#include "ajastuff/system/systemtime.h"
#include "ajastuff/system/process.h"
#include "ajacc/includes/ntv2captiondecoder608.h"
#include "ajacc/includes/ntv2captiondecoder708.h"


/**
	@brief		I can decode captions from frames captured from an AJA device in real time. I can optionally play the
				captured video with captions "burned" in on-screen using the device's mixer.

	@details	This app demonstrates how to properly react to input signal changes using AutoCirculate, resuming the
				capture stream once a stable video signal is restored. It also shows how to use the CEA-608 and CEA-708
				decoders provided in the 'ajacclib' caption library. It also shows how to use the mixer/keyer widget to
				superimpose graphics (with alpha transparency) over live video.
**/

class NTV2CCGrabber
{
	//	Public Class Methods
	public:
		/**
			@brief	Returns a string containing a concatenation of human-readable names of every
					available NTV2Line21Channel value.
			@param[in]	inDelimiter		The string to be used to separate each name. Defaults to ", ".
		**/
		static std::string		GetLine21ChannelNames (std::string inDelimiter = ", ");


	//	Public Instance Methods
	public:
		/**
			@brief	Constructs me using the given configuration settings.
			@note	I'm not completely initialized and ready for use until after my Init method has been called.
			@param[in]	inDeviceSpecifier	Specifies the AJA device to use.
											Defaults to "0" (first device found).
			@param[in]	in608Channel		Specifies the CEA-608 caption channel to "tune" to. Defaults to CC1.
			@param[in]	inBurnCaptions		If true, plays out the captured video with captions "burned" in; otherwise only captures.
											Defaults to false (capture only -- no playout or caption burn-in).
			@param[in]	inMultiFormat		If true, enables multiformat/multichannel mode if the device supports it, and won't acquire
											or release the device. If false (the default), acquires/releases exclusive use of the device.
			@param[in]	inForceVanc			If true, forces the use of Vanc, even if the device supports hardware Anc extraction.
											Defaults to false.
			@param[in]	inWithAudio			If true, enables audio capture, even though it's not needed for caption decoding.
											Defaults to false.
			@param[in]	inInputChannel		Specifies which input channel (and SDI input) to capture from. Defaults to NTV2_CHANNEL1 (SDI in 1).
		**/
								NTV2CCGrabber (	const std::string		inDeviceSpecifier	= "0",
												const NTV2Line21Channel	in608Channel		= NTV2_CC608_CC1,
												const bool				inBurnCaptions		= false,
												const bool				inMultiFormat		= false,
												const bool				inForceVanc			= false,
												const bool				inWithAudio			= false,
												const NTV2Channel		inInputChannel		= NTV2_CHANNEL1);

		virtual					~NTV2CCGrabber ();

		/**
			@brief	Initializes me and prepares me to Run.
		**/
		virtual AJAStatus		Init (void);

		/**
			@brief	Runs me.
			@note	Do not call this method without first calling my Init method.
		**/
		virtual AJAStatus		Run (void);

		/**
			@brief	Gracefully stops me from running.
		**/
		virtual void			Quit (void);

		/**
			@brief	Changes the caption channel I'm displaying.
			@param	inNewChannel	Specifies the new caption channel of interest.
		**/
		virtual void			SetCaptionDisplayChannel (const NTV2Line21Channel inNewChannel);


		/**
			@brief	Returns true if my play thread is currently running.
		**/
		virtual inline bool		IsPlayThreadRunning (void) const						{return mPlayoutThread && mPlayoutThread->Active ();}

		/**
			@brief	Starts my playout thread.
		**/
		virtual void			StartPlayThread (void);

		/**
			@brief	Toggles the Head-Up-Display.
			@note	This only affects caption burn-in.
		**/
		virtual inline void		ToggleHUD (void)										{mHeadUpDisplayOn = !mHeadUpDisplayOn;}

		virtual void			ToggleVANC (void);			///< @brief	Toggles the use of VANC. (Debug, experimental)


	//	Protected Instance Methods
	protected:
		/**
			@brief	Sets up everything I need for capturing video.
		**/
		virtual AJAStatus		SetupInputVideo (void);

		/**
			@brief	Sets up audio for capture (and playout, if burning captions).
		**/
		virtual AJAStatus		SetupAudio (void);

		/**
			@brief	Sets up board routing for capture (and sets the output standard based on the given video format).
		**/
		virtual void			RouteInputSignal (void);

		/**
			@brief	Sets the device output standard based on the given video format.
			@param[in]	inVideoFormat	Specifies the video format.
		**/
		virtual void			SetOutputStandards (const NTV2VideoFormat inVideoFormat);

		/**
			@brief	Starts my capture thread.
		**/
		virtual void			StartCaptureThread (void);

		/**
			@brief	Repeatedly captures frames using AutoCirculate (until global quit flag set).
		**/
		virtual void			CaptureFrames (void);

		/**
			@brief		Sets up my circular buffers.
			@param[in]	inVideoFormat	Specifies the video format.
			@return		AJA_STATUS_SUCCESS if successful; otherwise a relevant AJAStatus value.
		**/
		virtual AJAStatus		SetupHostBuffers (const NTV2VideoFormat inVideoFormat);

		/**
			@brief	Releases my circular buffers.
		**/
		virtual void			ReleaseHostBuffers (void);

		/**
			@brief	Extracts closed-caption data, if present, and emits it on a character-by-character basis to
					the standard output stream.
			@param[in]	inVideoFormat	Specifies the current video format of the captured video data that contains the
										captions to be decoded.
			@return	Zero if successful.
		**/
		virtual unsigned		ExtractClosedCaptionData (const NTV2VideoFormat inVideoFormat);


		/**
			@brief	Returns the address of the first line of captured video data in the host frame buffer.
		**/
		virtual const UByte *	GetVideoData (void) const;


		/**
			@brief	Emits the given character to standard output.
			@param[in]	inCharacter		Specifies the character to be emitted to the standard output stream.
			@note	This function only serves to eliminate long whitespace runs.
		**/
		virtual void			EmitCharacter (const string & inCharacter);


		/**
			@brief	Sets up everything I need for capturing video.
			@param[in]	inVideoFormat	Specifies the desired output video format.
			@return		AJA_STATUS_SUCCESS if successful;  otherwise the AJAStatus failure code.
		**/
		virtual AJAStatus		SetupOutputVideo (const NTV2VideoFormat inVideoFormat);

		/**
			@brief	Sets up board routing for playout.
			@param[in]	inVideoFormat	Specifies the desired output video format.
		**/
		virtual void			RouteOutputSignal (const NTV2VideoFormat inVideoFormat);

		/**
			@brief	Repeatedly updates captions (until global quit flag set).
		**/
		virtual void			PlayFrames (void);


		/**
			@brief	This function gets called whenever caption changes occur.
			@param[in]	inChangeInfo	The caption change that occurred.
		**/
		virtual void			CaptioningChanged (const NTV2Caption608ChangeInfo & inChangeInfo);

		/**
			@return	True if the device supports ancillary data extraction, and the driver supports the new AutoCirculate calls.
		**/
		virtual bool			DeviceAncExtractorIsAvailable (void);


	//	Protected Class Methods
	protected:
		/**
			@brief	This is the capture thread's static callback function that gets called when the capture thread runs.
					This function gets "Attached" to the AJAThread instance.
			@param[in]	pThread		Points to the AJAThread instance.
			@param[in]	pContext	Context information to pass to the thread.
									(For this application, this will be set to point to the NTV2Burn instance.)
		**/
		static void				CaptureThreadStatic (AJAThread * pThread, void * pContext);

		/**
			@brief	This is the playout thread's static callback function that gets called when the playout thread runs.
					This function gets "Attached" to the playout thread's AJAThread instance.
			@param[in]	pThread		A valid pointer to the playout thread's AJAThread instance.
			@param[in]	pContext	Context information to pass to the thread.
									(For this application, this will be set to point to the NTV2CCGrabber instance.)
		**/
		static void				PlayThreadStatic (AJAThread * pThread, void * pContext);

		/**
			@brief	This static function gets called whenever 608 captioning changes.
			@param[in]	pInstance		A valid pointer to the NTV2CCGrabber instance.
			@param[in]	inChangeInfo	The 608 captioning change.
		**/
		static void				Caption608ChangedStatic (void * pInstance, const NTV2Caption608ChangeInfo & inChangeInfo);


	//	Instance Data
	private:
		typedef	AJACircularBuffer <AVDataBuffer *>	MyCircularBuffer;

		AJAThread *					mCaptureThread;		///< @brief	My capture thread object
		AJALock *					mLock;				///< @brief	Global mutex to avoid device frame buffer allocation race condition
		CNTV2Card					mDevice;			///< @brief	My CNTV2Card instance
		NTV2DeviceID				mDeviceID;			///< @brief	My device identifier
		const std::string			mDeviceSpecifier;	///< @brief	The device specifier string
		NTV2Channel					mInputChannel;		///< @brief	The channel I'm using for input (capture)
		NTV2InputSource				mInputSource;		///< @brief	The input source I'm using
		NTV2FrameBufferFormat		mCaptureFBF;		///< @brief	My capture pixel format
		NTV2EveryFrameTaskMode		mSavedTaskMode;		///< @brief	Used to restore the previous state
		NTV2AudioSystem				mAudioSystem;		///< @brief	The audio system I'm using
		bool						mDoMultiFormat;		///< @brief	Multi-format enabled?
		bool						mForceVanc;			///< @brief	Force use of Vanc?
		bool						mGlobalQuit;		///< @brief	Set "true" to gracefully stop
		string						mLastChar;			///< @brief	Last character I emitted to cout
		uint32_t					mCaptureBufferSize;	///< @brief	My capture video buffer size, in bytes
		ULWord						mErrorTally;		///< @brief	Number of caption detect/decode errors
		ULWord						mCaptionDataTally;	///< @brief	Number of caption detections
		NTV2Line21Channel			m608Channel;		///< @brief	Caption channel to "tune" to
		NTV2Line21Mode				m608Mode;			///< @brief	Current caption mode
		CNTV2CaptionDecoder608Ptr	m608Decoder;		///< @brief	My 608 closed-caption decoder
		CNTV2CaptionDecoder708Ptr	m708Decoder;		///< @brief	My 708 closed-caption decoder

		AVDataBuffer				mAVHostBuffer [CIRCULAR_BUFFER_SIZE];	///< @brief	My host buffers
		MyCircularBuffer			mAVCircularBuffer;	///< @brief	My circular buffer
		AUTOCIRCULATE_TRANSFER		mInputXferInfo;		///< @brief	My input AutoCirculate transfer info

		//	Instance data only used for caption burn-in:
		bool						mBurnCaptions;		///< @brief	Burn captions into playout video?
		bool						mHeadUpDisplayOn;	///< @brief	True if the HUD is being displayed
		NTV2Channel					mOutputChannel;		///< @brief	My playout channel -- determines SDI output spigot to use
		NTV2FrameBufferFormat		mPlayoutFBF;		///< @brief	My caption frame store's pixel format
		AJAThread *					mPlayoutThread;		///< @brief	My playout thread object

};	//	NTV2CCGrabber

#endif	//	_NTV2CCGRABBER_H
