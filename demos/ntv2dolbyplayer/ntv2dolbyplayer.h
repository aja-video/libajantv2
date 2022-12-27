/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2dolbyplayer.h
	@brief		Header file for NTV2DolbyPlayer demonstration class
	@copyright	(C) 2013-2022 AJA Video Systems, Inc.  All rights reserved.
**/


#ifndef _NTV2DOLBYPLAYER_H
#define _NTV2DOLBYPLAYER_H

#include "ntv2enums.h"
#include "ntv2devicefeatures.h"
#include "ntv2devicescanner.h"
#include "ntv2democommon.h"
#include "ajabase/common/circularbuffer.h"
#include "ajabase/system/thread.h"
#include "ajabase/system/file_io.h"
#include "ajaanc/includes/ancillarydata.h"


#define DOLBY_FULL_PARSER	//	If defined, parse EC3 files with multiple sync frames per HDMI burst;  otherwise parse with single sync frame per HDMI burst.

/**
	@brief	I am an object that can play out a test pattern (with timecode) to an output of an AJA device
			with or without audio tone in real time. I make use of the AJACircularBuffer, which simplifies
			implementing a producer/consumer model, in which a "producer" thread generates the test pattern
			frames, and a "consumer" thread (i.e., the "play" thread) sends those frames to the AJA device.
			I demonstrate how to embed timecode into an SDI output signal using AutoCirculate during playout.
**/

class NTV2DolbyPlayer
{
	//	Public Instance Methods
	public:
		/**
			@brief	Constructs me using the given configuration settings.
			@note	I'm not completely initialized and ready for use until after my Init method has been called.
			@param[in]	inDeviceSpecifier	Specifies the AJA device to use. Defaults to "0", the first device found.
			@param[in]	inWithAudio			If true, include audio tone in the output signal;  otherwise, omit it.
											Defaults to "true".
			@param[in]	inChannel			Specifies the channel to use. Defaults to NTV2_CHANNEL1.
			@param[in]	inPixelFormat		Specifies the pixel format to use for the device's frame buffers.
											Defaults to 8-bit YUV.
			@param[in]	inVideoFormat		Specifies the video format to use. Defaults to 1080i5994.
			@param[in]	inDoMultiFormat		If true, use multi-format mode; otherwise use uniformat mode. Defaults to false (uniformat mode).
		**/
								NTV2DolbyPlayer (const std::string &			inDeviceSpecifier	= "0",
                                                 const bool                     inWithAudio			= true,
                                                 const NTV2Channel              inChannel			= NTV2_CHANNEL1,
												 const NTV2FrameBufferFormat	inPixelFormat		= NTV2_FBF_8BIT_YCBCR,
                                                 const NTV2VideoFormat          inVideoFormat		= NTV2_FORMAT_1080i_5994,
                                                 const bool                     inDoMultiFormat		= false,
                                                 AJAFileIO*                     inDolbyFile         = NULL);

		virtual					~NTV2DolbyPlayer (void);

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
			@return	True if I'm running;  otherwise false.
		**/
		virtual bool			IsRunning (void) const				{return !mGlobalQuit;}

		/**
			@brief	Provides status information about my output (playout) process.
			@param[out]	outGoodFrames		Receives the number of successfully played frames.
			@param[out]	outDroppedFrames	Receives the number of dropped frames.
			@param[out]	outBufferLevel		Receives the driver's current buffer level.
		**/
		virtual void			GetACStatus (ULWord & outGoodFrames, ULWord & outDroppedFrames, ULWord & outBufferLevel);


	//	Protected Instance Methods
	protected:
		struct NTV2DolbyBSI
		{
			uint32_t strmtyp;
			uint32_t substreamid;
			uint32_t frmsiz;
			uint32_t fscod;
			uint32_t numblkscod;
			uint32_t acmod;
			uint32_t lfeon;
			uint32_t bsid;
			uint32_t dialnorm;
			uint32_t compre;
			uint32_t compr;
			uint32_t dialnorm2;
			uint32_t compr2e;
			uint32_t compr2;
			uint32_t chanmape;
			uint32_t chanmap;
			uint32_t mixmdate;
			uint32_t dmixmod;
			uint32_t ltrtcmixlev;
			uint32_t lorocmixlev;
			uint32_t ltrtsurmixlev;
			uint32_t lorosurmixlev;
			uint32_t lfemixlevcode;
			uint32_t lfemixlevcod;
			uint32_t pgmscle;
			uint32_t pgmscl;
			uint32_t pgmscl2e;
			uint32_t pgmscl2;
			uint32_t extpgmscle;
			uint32_t extpgmscl;
			uint32_t mixdef;
			uint32_t premixcmpsel;
			uint32_t drcsrc;
			uint32_t premixcmpscl;
			uint32_t mixdata;
			uint32_t mixdeflen;
			uint32_t mixdata2e;
			uint32_t extpgmlscle;
			uint32_t extpgmlscl;
			uint32_t extpgmcscle;
			uint32_t extpgmcscl;
			uint32_t extpgmrscle;
			uint32_t extpgmrscl;
			uint32_t extpgmlsscle;
			uint32_t extpgmlsscl;
			uint32_t extpgmrsscle;
			uint32_t extpgmrsscl;
			uint32_t extpgmlfescle;
			uint32_t extpgmlfescl;
			uint32_t dmixscle;
			uint32_t dmixscl;
			uint32_t addche;
			uint32_t extpgmaux1scle;
			uint32_t extpgmaux1scl;
			uint32_t extpgmaux2scle;
			uint32_t extpgmaux2scl;
			uint32_t mixdata3e;
			uint32_t spchdat;
			uint32_t addspchdate;
			uint32_t spchdat1;
			uint32_t spchan1att;
			uint32_t addspchdat1e;
			uint32_t addspdat1e;
			uint32_t spchdat2;
			uint32_t spchan2att;
			uint8_t mixdatabuffer[64];
			uint32_t paninfoe;
			uint32_t panmean;
			uint32_t paninfo;
			uint32_t paninfo2e;
			uint32_t panmean2;
			uint32_t paninfo2;
			uint32_t frmmixcfginfoe;
			uint32_t blkmixcfginfo[6];
			uint32_t blkmixcfginfoe;
			uint32_t infomdate;
			uint32_t bsmod;
			uint32_t copyrightb;
			uint32_t origbs;
			uint32_t dsurmod;
			uint32_t dheadphonmod;
			uint32_t dsurexmod;
			uint32_t audprodie;
			uint32_t mixlevel;
			uint32_t roomtyp;
			uint32_t adconvtyp;
			uint32_t audprodi2e;
			uint32_t mixlevel2;
			uint32_t roomtyp2;
			uint32_t adconvtyp2;
			uint32_t sourcefscod;
			uint32_t convsync;
			uint32_t blkid;
			uint32_t frmsizecod;
			uint32_t addbsie;
			uint32_t addbsil;
			uint8_t addbsibuffer[64];
		};

		/**
			@brief	Sets up everything I need to play video.
		**/
		virtual AJAStatus		SetUpVideo (void);

		/**
			@brief	Sets up everything I need to play audio.
		**/
		virtual AJAStatus		SetUpAudio (void);

		/**
			@brief	Sets up device routing for playout.
		**/
		virtual void			RouteOutputSignal (void);

		/**
			@brief	Sets up my circular buffers.
		**/
		virtual void			SetUpHostBuffers (void);

		/**
			@brief	Creates my test pattern buffers.
		**/
		virtual AJAStatus		SetUpTestPatternVideoBuffers (void);

		/**
			@brief	Starts my playout thread.
		**/
		virtual void			StartConsumerThread (void);

		/**
			@brief	Repeatedly plays out frames using AutoCirculate (until quit).
		**/
		virtual void			PlayFrames (void);

		/**
			@brief	Starts my test pattern producer thread.
		**/
		virtual void			StartProducerThread (void);

		/**
			@brief	Repeatedly produces test pattern frames (until global quit flag set).
		**/
		virtual void			ProduceFrames (void);

		/**
			@brief	Inserts audio tone (based on my current tone frequency) into the given audio buffer.
			@param[out]	audioBuffer		Specifies a valid, non-NULL pointer to the buffer that is to receive
										the audio tone data.
			@return	Total number of bytes written into the buffer.
		**/
		virtual uint32_t		AddTone (ULWord * audioBuffer);

        /**
             @brief	Inserts dolby audio into the given audio buffer.
             @param[out]	audioBuffer		Specifies a valid, non-NULL pointer to the buffer that is to receive
                                            the audio tone data.
             @return	Total number of bytes written into the buffer.
		 **/
		virtual uint32_t		AddDolby (ULWord * audioBuffer);

#ifdef DOLBY_FULL_PARSER
		/**
			@brief	Get a dolby audio audio frame from the input file.
			@param[out]	pInDolbyBuffer		Specifies a valid, non-NULL pointer to the buffer that is to receive
												the dolby frame data.
			@param[out]	numSamples			Number of samples in the buffer.
			@return	True if valid sync frame in buffer.
		 **/
		virtual bool GetDolbyFrame (uint16_t * pInDolbyBuffer, uint32_t & numSamples);

		/**
			@brief	Parse the dolby audio bit stream information block.
			@param[out]	pInDolbyBuffer		Specifies a valid, non-NULL pointer to the buffer that is to receive
												the dolby frame data.
			@param[out]	numSamples			Number of samples in the buffer.
			@param[out]	pBsi				Parsed Dolby header data.
			@return	True if parser suceeded.
		 **/
		virtual bool ParseBSI (uint16_t * pInDolbyBuffer, uint32_t numSamples, NTV2DolbyBSI * pBsi);


		/**
			@brief	Set the bitstream buffer for bit retrieval
			@param[in]	pBuffer				Specifies a valid, non-NULL pointer to the bitstream buffer
			@param[in]	size				Bitstream buffer size
		 **/
		virtual void SetBitBuffer (uint8_t * pBuffer, uint32_t size);

		/**
			@brief	Retreive the specified number of bits from the bitstream buffer
			@param		data				Bitstream data
			@param[in]	inBitCount			Number of bits to retrieve from the buffer
			@return	True if suceeded.
		 **/
		virtual bool GetBits (uint32_t & data, uint32_t inBitCount);
#endif

    //	Protected Class Methods
	protected:
		/**
			@brief	This is the consumer thread's static callback function that gets called when the consumer thread starts.
					This function gets "Attached" to the consumer thread's AJAThread instance.
			@param[in]	pThread		A valid pointer to the consumer thread's AJAThread instance.
			@param[in]	pContext	Context information to pass to the thread.
									(For this application, this will be set to point to the NTV2DolbyPlayer instance.)
		**/
		static void				ConsumerThreadStatic (AJAThread * pThread, void * pContext);

		/**
			@brief	This is the producer thread's static callback function that gets called when the producer thread starts.
					This function gets "Attached" to the producer thread's AJAThread instance.
			@param[in]	pThread		A valid pointer to the producer thread's AJAThread instance.
			@param[in]	pContext	Context information to pass to the thread.
									(For this application, this will be set to point to the NTV2DolbyPlayer instance.)
		**/
		static void				ProducerThreadStatic (AJAThread * pThread, void * pContext);

		/**
			@brief	Returns the RP188 DBB register number to use for the given ::NTV2OutputDestination.
			@param[in]	inOutputSource	Specifies the NTV2OutputDestination of interest.
			@return	The number of the RP188 DBB register to use for the given output destination.
		**/
		static ULWord			GetRP188RegisterForOutput (const NTV2OutputDestination inOutputSource);


	//	Private Member Data
	private:
		typedef AJACircularBuffer <AVDataBuffer *>		MyCirculateBuffer;

		AJAThread *					mConsumerThread;			///< @brief	My playout (consumer) thread object
		AJAThread *					mProducerThread;			///< @brief	My generator (producer) thread object

		uint32_t					mCurrentFrame;				///< @brief	My current frame number (used to generate timecode)
		ULWord						mCurrentSample;				///< @brief	My current audio sample (maintains audio tone generator state)
		double						mToneFrequency;				///< @brief	My current audio tone frequency [Hz]

		const std::string			mDeviceSpecifier;			///< @brief	Specifies the device I should use
		CNTV2Card					mDevice;					///< @brief	My CNTV2Card instance
		NTV2DeviceID				mDeviceID;					///< @brief	My device (model) identifier
		NTV2Channel					mOutputChannel;				///< @brief	The channel I'm using
		NTV2VideoFormat				mVideoFormat;				///< @brief	My video format
		NTV2FrameBufferFormat		mPixelFormat;				///< @brief	My pixel format
		NTV2EveryFrameTaskMode		mSavedTaskMode;				///< @brief	Used to restore the prior task mode
		NTV2AudioSystem				mAudioSystem;				///< @brief	The audio system I'm using
        NTV2AudioRate               mAudioRate;                 ///< @brief	My audio rate
		const bool					mWithAudio;					///< @brief	Playout audio?
		bool						mGlobalQuit;				///< @brief	Set "true" to gracefully stop
		bool						mDoMultiChannel;			///< @brief	Demonstrates how to configure the board for multi-format
		AJATimeCodeBurn				mTCBurner;					///< @brief	My timecode burner
		uint32_t					mVideoBufferSize;			///< @brief	My video buffer size, in bytes
		uint32_t					mAudioBufferSize;			///< @brief	My audio buffer size, in bytes

		uint8_t **					mTestPatternVideoBuffers;	///< @brief	My test pattern buffers
		uint32_t					mNumTestPatterns;			///< @brief	Number of test patterns to cycle through

		AVDataBuffer				mAVHostBuffer [CIRCULAR_BUFFER_SIZE];	///< @brief	My host buffers
		MyCirculateBuffer			mAVCircularBuffer;						///< @brief	My ring buffer

		uint32_t                    mBurstIndex;				///< @brief	HDMI burst sample index
		uint32_t                    mBurstSamples;				///< @brief	HDMI burst sample size

		uint16_t *                  mBurstBuffer;               ///< @brief	HDMI burst audio data buffer
		uint32_t                    mBurstSize;                 ///< @brief	HDMI burst audio data size
		uint32_t                    mBurstOffset;               ///< @brief	HDMI burst audio data offset
		uint32_t                    mBurstMax;			        ///< @brief	HDMI burst and dolby max size

		AJAFileIO *                 mDolbyFile;                 ///< @brief	Dolby audio source file
		uint16_t *                  mDolbyBuffer;               ///< @brief	Dolby audio data buffer
		uint32_t                    mDolbySize;                 ///< @brief	Dolby audio data size
		uint32_t					mDolbyBlocks;				///< @brief	Dolby audio block count

		uint8_t *					mBitBuffer;
		ULWord						mBitSize;
		ULWord						mBitIndex;
};	//	NTV2DolbyPlayer

#endif	//	_NTV2DOLBY_H
