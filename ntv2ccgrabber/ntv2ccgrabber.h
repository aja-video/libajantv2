/**
	@file		ntv2ccgrabber.h
	@brief		Header file for NTV2CCGrabber demonstration class
	@copyright	(C) 2013-2020 AJA Video Systems, Inc.  All rights reserved.
**/


#ifndef _NTV2CCGRABBER_H
#define _NTV2CCGRABBER_H

#include "ntv2enums.h"
#include "ntv2devicefeatures.h"
#include "ntv2devicescanner.h"
#include "ntv2democommon.h"
#include "ntv2task.h"
#include "ntv2utils.h"
#include "ntv2vpid.h"
#include "ajabase/common/types.h"
#include "ajabase/common/videotypes.h"
#include "ajabase/common/timecode.h"
#include "ajabase/common/timecodeburn.h"
#include "ajabase/common/circularbuffer.h"
#include "ajabase/system/thread.h"
#include "ajabase/system/info.h"
#include "ajabase/system/systemtime.h"
#include "ajabase/system/process.h"
#include "ajacc/includes/ntv2captiondecoder608.h"
#include "ajacc/includes/ntv2captiondecoder708.h"


typedef enum _OutputMode
{
	kOutputMode_CaptionStream,
	kOutputMode_CaptionScreen,
	kOutputMode_CaptionFileSCC,
	kOutputMode_CaptionFileMCC,
	kOutputMode_Stats,
	kOutputMode_INVALID
} OutputMode;

#define	IS_VALID_OutputMode(_x_)	((_x_) >= kOutputMode_CaptionStream  &&  (_x_) < kOutputMode_INVALID)


typedef enum _CaptionDataSrc
{
	kCaptionDataSrc_Default,	//	Best available
	kCaptionDataSrc_Line21,
	kCaptionDataSrc_608FBVanc,
	kCaptionDataSrc_708FBVanc,
	kCaptionDataSrc_608Anc,
	kCaptionDataSrc_708Anc,
	kCaptionDataSrc_INVALID
} CaptionDataSrc;

#define	IS_VALID_CaptionDataSrc(_x_)	((_x_) >= kCaptionDataSrc_Default  &&  (_x_) < kCaptionDataSrc_INVALID)


/**
	@brief	This class is used to configure an NTV2CCGrabber instance.
**/
typedef struct CCGrabberConfig
{
	public:
		std::string						fDeviceSpecifier;	///< @brief	The AJA device to use
		NTV2Channel						fInputChannel;		///< @brief	The device channel to use
		NTV2InputSource					fInputSource;		///< @brief	The device input connector to use
		CNTV2DemoCommon::ACFrameRange	fFrames;			///< @brief	AutoCirculate frame count or range
		OutputMode						fOutputMode;		///< @brief	Desired output (captionStream, Screen etc)
		CaptionDataSrc					fCaptionSrc;		///< @brief	Caption data source (Line21? 608 VANC? 608 Anc?  etc)
		NTV2TCIndex						fTimecodeSrc;		///< @brief	Timecode source to use (if any)
		NTV2FrameBufferFormat			fPixelFormat;		///< @brief	Pixel format to use
		NTV2Line21Channel				fCaptionChannel;	///< @brief	Caption channel to monitor (defaults to CC1)
		bool							fBurnCaptions;		///< @brief	If true, burn-in captions on 2nd channel
		bool							fDoMultiFormat;		///< @brief	If true, use multi-format/multi-channel mode, if device supports it; otherwise normal mode
		bool							fUseVanc;			///< @brief	If true, use Vanc, even if the device supports Anc insertion
		bool							fCaptureAudio;		///< @brief	If true, also capture audio

		/**
			@brief	Constructs a default CCPlayer configuration.
		**/
		inline explicit	CCGrabberConfig (const std::string & inDeviceSpecifier	= "0")
			:	fDeviceSpecifier	(inDeviceSpecifier),
				fInputChannel		(NTV2_CHANNEL_INVALID),
				fInputSource		(NTV2_INPUTSOURCE_INVALID),
				fFrames				(7),
				fOutputMode			(kOutputMode_CaptionStream),
				fCaptionSrc			(kCaptionDataSrc_Default),
				fTimecodeSrc		(NTV2_TCINDEX_INVALID),
				fPixelFormat		(NTV2_FBF_10BIT_YCBCR),
				fCaptionChannel		(NTV2_CC608_CC1),
				fBurnCaptions		(false),
				fDoMultiFormat		(false),
				fUseVanc			(false),
				fCaptureAudio		(false)
		{
		}
		unsigned GetNumSourceSpecs(void)const {return	(NTV2_IS_VALID_CHANNEL(fInputChannel)?1:0)
													+	(NTV2_IS_VALID_INPUT_SOURCE(fInputSource)?1:0);}
		AJALabelValuePairs Get(const bool inCompact = false)const;
		static std::string	GetLegalOutputModes (void);
		static std::string	OutputModeToString (const OutputMode inMode);
		static OutputMode	StringToOutputMode (const std::string & inModeStr);
		static std::string	GetLegalCaptionDataSources (void);
		static std::string	CaptionDataSrcToString (const CaptionDataSrc inDataSrc);
		static CaptionDataSrc	StringToCaptionDataSrc (const std::string & inDataSrcStr);
} CCGrabberConfig;

std::ostream &	operator << (std::ostream & ioStrm, const CCGrabberConfig & inObj);


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
			@param[in]	inConfigData	Specifies the grabber configuration.
		**/
								NTV2CCGrabber (const CCGrabberConfig & inConfigData);
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
		virtual inline bool		IsPlayThreadRunning (void) const		{return mPlayoutThread.Active();}

		/**
			@brief	Starts my playout thread.
		**/
		virtual void			StartPlayThread (void);

		/**
			@brief	Toggles the Head-Up-Display.
			@note	This only affects caption burn-in.
		**/
		virtual inline void		ToggleHUD (void)						{mHeadUpDisplayOn = !mHeadUpDisplayOn;}

		virtual void			ToggleVANC (void);			///< @brief	Toggles the use of VANC. (Debug, experimental)
		virtual void			SwitchOutput(void);			///< @brief	Switches/rotates --output mode.
		virtual void			Switch608Source(void);		///< @brief	Switches/rotates --608src.

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
			@brief		Wait for a stable input signal, and return it.
			@return		The input video format. Guaranteed to be valid unless the app is terminating.
		**/
		virtual NTV2VideoFormat	WaitForStableInputSignal (void);

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
			@param[in]	inFrameCount	Specifies the current frame number of the captured video.
			@param[in]	inVideoFormat	Specifies the current video format of the captured video.
		**/
		virtual void			ExtractClosedCaptionData (const uint32_t inFrameCount, const NTV2VideoFormat inVideoFormat);


		/**
			@brief	Returns the address of the first line of captured video data in the host frame buffer.
		**/
		virtual const UByte *	GetVideoData (void) const;


		/**
			@brief	Sets up everything I need for capturing video.
			@param[in]	inVideoFormat	Specifies the desired output video format.
			@return		AJA_STATUS_SUCCESS if successful;  otherwise the AJAStatus failure code.
		**/
		virtual AJAStatus		SetupOutputVideo (const NTV2VideoFormat inVideoFormat);

		/**
			@brief		Outputs CC data.
			@param[in]	inFrameNum		Specifies the current frame number.
			@param[in]	inCCData		Specifies the current CC data.
			@param[in]	inVideoFormat	Specifies the current video format.
		**/
		virtual void			DoCCOutput (const uint32_t inFrameNum, const CaptionData & inCCData, const NTV2VideoFormat inVideoFormat);

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
		CCGrabberConfig				mConfig;			///< @brief	My configuration
		AJAThread					mCaptureThread;		///< @brief	My capture thread object
		CNTV2Card					mDevice;			///< @brief	My CNTV2Card instance
		NTV2DeviceID				mDeviceID;			///< @brief	Keep device ID handy
		NTV2EveryFrameTaskMode		mSavedTaskMode;		///< @brief	Used to restore the previous task mode
		NTV2AudioSystem				mAudioSystem;		///< @brief	The audio system I'm using
		NTV2VANCMode				mVancMode;			///< @brief	Actual NTV2VANCMode in use
		bool						mGlobalQuit;		///< @brief	Set "true" to gracefully stop
		std::string					mLastOutStr;		///< @brief	Last thing I emitted to cout
		uint32_t					mFirstOutFrame;		///< @brief	First frame number for output
		uint32_t					mLastOutFrame;		///< @brief	Last frame number for output
		uint32_t					mCaptureBufferSize;	///< @brief	My capture video buffer size, in bytes
		ULWord						mErrorTally;		///< @brief	Number of caption detect/decode errors
		ULWord						mCaptionDataTally;	///< @brief	Number of caption detections
		NTV2Line21Channel			m608Channel;		///< @brief	Caption channel to "tune" to
		NTV2Line21Mode				m608Mode;			///< @brief	Current caption mode
		CNTV2CaptionDecoder608Ptr	m608Decoder;		///< @brief	My 608 closed-caption decoder
		CNTV2CaptionDecoder708Ptr	m708DecoderAnc;		///< @brief	My 708 closed-caption decoder (for anc extractor)
		CNTV2CaptionDecoder708Ptr	m708DecoderVanc;	///< @brief	My 708 closed-caption decoder (for VANC)
		CNTV2VPID					mVPIDInfoDS1;		///< @brief	Input DS1 VPID info
		CNTV2VPID					mVPIDInfoDS2;		///< @brief	Input DS2 VPID info
		NTV2ChannelSet				mAllFrameStores;	///< @brief	All device FrameStores
		NTV2ChannelSet				mAllCSCs;			///< @brief	All device CSCs
		NTV2ChannelSet				mInputFrameStores;	///< @brief	Active input FrameStores
		NTV2ChannelSet				mActiveSDIInputs;	///< @brief	Active SDI inputs
		NTV2ChannelSet				mActiveCSCs;		///< @brief	Active CSCs
		NTV2XptConnections			mInputConnections;	///< @brief	Input routing connections

		typedef	AJACircularBuffer <AVDataBuffer *>	MyCircularBuffer;
		AVDataBuffer				mAVHostBuffer [CIRCULAR_BUFFER_SIZE];	///< @brief	My host buffers
		MyCircularBuffer			mAVCircularBuffer;	///< @brief	My circular buffer
		AUTOCIRCULATE_TRANSFER		mInputXferInfo;		///< @brief	My input AutoCirculate transfer info

		//	Instance data only used for caption burn-in:
		bool						mHeadUpDisplayOn;	///< @brief	True if the HUD is being displayed
		NTV2Channel					mOutputChannel;		///< @brief	My playout channel -- determines SDI output spigot to use
		NTV2FrameBufferFormat		mPlayoutFBF;		///< @brief	My caption frame store's pixel format
		mutable AJAThread			mPlayoutThread;		///< @brief	My playout thread object
		NTV2ChannelSet				mOutputFrameStores;	///< @brief	My output FrameStores
		NTV2XptConnections			mOutputConnections;	///< @brief	Output routing connections

};	//	NTV2CCGrabber

#endif	//	_NTV2CCGRABBER_H
