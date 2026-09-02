/**
	@file		ntv2captiondecoder608.h
	@brief		Declares the CNTV2CaptionDecoder608 class.
	@copyright	(C) 2006-2022 AJA Video Systems, Inc. All rights reserved.
**/

#ifndef __NTV2_CEA608_DECODER_
#define __NTV2_CEA608_DECODER_

#include "ntv2captiondecodechannel608.h"
#include "ntv2xdscaptiondecodechannel608.h"
#include "ntv2formatdescriptor.h"
#include "ajabase/common/ajarefptr.h"
#include <string>
#include <vector>

#ifdef AJAMac
	#define	odprintf	printf
#endif	//	AJAMac

#ifdef MSWindows
	#include "windows.h"
	#include "stdio.h"
#endif


/**
	@brief	I am a full-featured CEA-608 ("Line 21") captioning decoder. I accept caption data (2 bytes/field),
			parse it to the appropriate channel (CC1, CC2, etc.), where the current state and buffer status is
			maintained. I currently handle parsing for captioning channels CC1-CC4, Text1-Text4, and XDS in
			parallel -- i.e. I'm decoding all channels simultaneously.

			For display purposes, I provide CNTV2CaptionDecoder608::SetDisplayChannel to select the current displayed channel.
			However, since all channels are being decoded in parallel, you may change the displayed channel at
			any time and immediately see the state of the newly selected channel. You don't have to wait for the
			channel to receive enough new data to get in-sync.

			I also have methods to "burn-in" caption glyphs into video for the selected display channel.
			CNTV2CaptionDecoder608::BurnCaptions will do the entire burn-in (for a limited number of frame buffer
			sizes and formats), or you can call lower-level methods to extract the current state of the displayed channel.

			To implement a basic decoder:
			-	Call CNTV2CaptionDecoder608::Create to create a ::CNTV2CaptionDecoder608 instance;
			-	Call CNTV2CaptionDecoder608::SetDisplayChannel methods to select the current displayed channel (if any);
			-	Call CNTV2CaptionDecoder608::ProcessNew608FrameData every frame, passing it the four bytes of CEA-608 data
				that arrive every frame (2 bytes per field for SD/interlaced video).

			CNTV2CaptionDecoder608::ProcessNew608FrameData should be called every frame, even if the captioning data is "null"
			(zeros). CEA-608 caption rules call for certain commands to be sent twice on adjacent frames, which means there's
			a difference between "Command|Command|Command" and "Command|null|Command", and if the ::CNTV2CaptionDecoder608
			instance never sees the intervening NULL frames, it can mistakenly think two commands come from adjacent frames
			and misinterpret them.

			To display captions, call CNTV2CaptionDecoder608::SetDisplayChannel to select the desired caption channel to
			display, then at every frame, passing it the host NTV2Buffer containing the current video frame, call
			CNTV2CaptionDecoder608::BurnCaptions. This function contains logic for blinking characters with the
			"flashing" attribute, and for smoothly performing roll-ups. The modified video NTV2Buffer can then be
			transferred to the device for playback.

			Clients can implement their own caption display, if needed, by calling CNTV2CaptionDecoder608::GetOnAirCharacter
			to obtain the character (and its attributes) at each row/column position from the decoder's caption
			channel buffer.
**/
class CNTV2CaptionDecoder608;
typedef AJARefPtr <CNTV2CaptionDecoder608>	CNTV2CaptionDecoder608Ptr;

class AJAExport CNTV2CaptionDecoder608 : public CNTV2CaptionLogConfig
{
	//	Class Methods
	public:
		/**
			@brief		Creates a new CNTV2CaptionEncoder608 instance.
			@param[out]	outEncoder	Receives the newly-created encoder instance.
			@return		True if successful; otherwise False.
		**/
		static bool				Create (CNTV2CaptionDecoder608Ptr & outEncoder);

		/**
			@brief		Determines the flash/blink rate for characters having the "flash" attribute.
						It's the number of frames required to complete a full cycle from "on" thru "off".
						Defaults to 30 frames, or about a second at 29.97 fps.
		**/
		static const int		CharacterFlashCycleFrames		= 30;

	//	Instance Methods
	public:
		/**
			@name	Decoder Operation
		**/
		///@{
		/**
			@brief		Flushes me, clearing all in-progress data.
			@note		This is NOT thread-safe!
		**/
		virtual void						Reset (void);

		/**
			@brief		Changes the caption channel that I'm focused on (or that I'm currently "burning" into video).
			@param[in]	inChannel	Specifies the new caption channel that I am to focus on.
			@return		True if successful; otherwise False.
		**/
		virtual	bool						SetDisplayChannel (const NTV2Line21Channel inChannel);

		/**
			@brief		Answers with the caption channel that I'm currently focused on (or that I'm currently "burning" into video).
			@return		My current NTV2Line21Channel of interest.
		**/
		virtual	inline NTV2Line21Channel	GetDisplayChannel (void) const		{return mDisplayChannel;}


		/**
			@brief		Call this once per video frame when I'm doing display "burn-in", whether there is
						new caption data to parse or not. This keeps "flash" mode regular during "burn-in".
			@note		Starting in SDK 17.0, if AutoCallIdleFrame is set to 'true', this function will be
						called automatically when ProcessNew608FrameData is called.
		**/
		virtual void						IdleFrame (void);


		/**
			@brief		Notifies me that new frame data has arrived. Clients should call this method
						once per video frame with the four bytes (2 per field) of new captioning data.
			@param[in]	inCC608Data		Specifies the caption data that arrived for the lastest frame.
			@return		True if successful;  otherwise False.
			@note		Starting in SDK 17.0, if AutoCallIdleFrame is set to 'true', this function will
						automatically call IdleFrame.
		**/
		virtual bool						ProcessNew608FrameData (const CaptionData & inCC608Data);
		///@}

		/**
			@name	Caption Screen Access
		**/
		///@{
		/**
			@brief		Retrieves the "on-air" character and its attributes at the given on-screen row
						and column position.
			@param[in]	inRow			Specifies the row number of interest. Must be 1-15.
			@param[in]	inCol			Specifies the column number of interest. Must be 1-32.
			@param[out]	outAttrs		Receives the attributes of the "on-air" character of interest.
			@note		Starting in SDK 17.0, if AutoFlashOnAirChars is set to 'true', and my current flash cycle
						is "off", and the attributes of the character at that position includes "flashing",
						the returned string will be a non-underlined space.
			@return		The "on-air" character if successful;  otherwise zero.
		**/
		virtual UByte						GetOnAirCharacterWithAttributes (const UWord inRow,
																			const UWord inCol,
																			NTV2Line21Attrs & outAttrs) const;

		/**
			@brief		Returns the UTF-16 character that best represents the caption character at the given screen position.
			@param[in]	inRow			The row number of interest (1-15).
			@param[in]	inCol			The column number of interest (1-32).
			@param[out]	outAttrs		Receives the NTV2Line21Attributes of the on-screen character.
			@note		Starting in SDK 17.0, if AutoFlashOnAirChars is set to 'true', and my current flash cycle
						is "off", and the attributes of the character at that position includes "flashing",
						the returned string will be a non-underlined space.
			@return		The UTF-16 character that best represents the on-screen caption character,
						if successful, or zero upon failure.
		**/
		virtual UWord						GetOnAirUTF16CharacterWithAttributes (const UWord inRow,
																					const UWord inCol,
																					NTV2Line21Attrs & outAttrs) const;


		/**
			@brief		Retrieves the "on-air" character and its attributes at the given row and column position.
			@param[in]	inRow			Specifies the row number of interest (1-15).
			@param[in]	inCol			Specifies the column number of interest (1-32).
			@param[out]	outAttrs		Receives the attributes of the "on-air" character of interest.
			@return		A UTF8-encoded string that contains the "on-air" character, if successful;
						otherwise an empty string.
			@note		Starting in SDK 17.0, if AutoFlashOnAirChars is set to 'true', and my current flash cycle
						is "off", and the attributes of the character at that position includes "flashing",
						the returned string will be a non-underlined space.
		**/
		virtual std::string					GetOnAirCharacter (const UWord inRow,
																const UWord inCol,
																NTV2Line21Attrs & outAttrs) const;

		/**
			@brief		Retrieves the "on-air" character at the given on-screen row and column position.
			@param[in]	inRow	Specifies the row number of interest (1-15).
			@param[in]	inCol	Specifies the column number of interest (1-32).
			@return		A UTF-8 encoded string that contains the "on-air" character, if successful;
						otherwise an empty string.
			@note		Starting in SDK 17.0, if AutoFlashOnAirChars is set to 'true', and my current flash cycle
						is "off", and the attributes of the character at that position includes "flashing",
						the returned string will be a non-underlined space.
		**/
		virtual inline std::string			GetOnAirCharacter (const UWord inRow, const UWord inCol) const
													{NTV2Line21Attrs atrs; return GetOnAirCharacter(inRow, inCol, atrs);}

		/**
			@brief		Retrieves all "on-air" characters either for all rows, or a specific row.
			@param[in]	inRowNumber		Optionally specifies the row number of interest (1 thru 15).
										Specify zero (the default) to return all rows.
			@return		A UTF-8 encoded string that contains all "on-air" characters, if successful;
						otherwise an empty string.
						Multiple rows will include newline character(s) between successive rows.
			@note		Starting in SDK 17.0, if AutoFlashOnAirChars is set to 'true',, and my current flash cycle
						is "off", and the attributes of the character at that position includes "flashing",
						the returned string will be a non-underlined space.
		**/
		virtual std::string					GetOnAirCharacters (const UWord inRowNumber = 0) const;
		///@}

		/**
			@name	Caption Display Rendering
		**/
		///@{
		/**
			@brief		Blits all of my current caption channel's "on-air" captions into the given host buffer
						with the correct colors, display attributes and positioning.
			@param		inFB	Specifies a valid host buffer that is to be blitted into.
			@param[in]	inFD	Describes the raster and pixel format of the given host buffer.
			@return		True if successful;  otherwise False.
		**/
		virtual bool						BurnCaptions (NTV2Buffer & inFB, const NTV2FormatDesc & inFD);	//	New in SDK 16.0
		///@}

		/**
			@name	Text Mode Operation
		**/
		///@{
		/**
			@brief		Returns the number of rows used for displaying Text Mode captions for the given (TxN) caption channel.
			@param[in]	inChannel	Specifies the [Text] caption channel of interest.
			@return		True if successful;  otherwise false.
		**/
		virtual UWord						GetTextModeDisplayRowCount (const NTV2Line21Channel inChannel);

		/**
			@brief		Changes the number of rows used for displaying Text Mode captions for the given (TxN) caption channel.
			@param[in]	inChannel	Specifies the [Text] caption channel to be configured.
			@param[in]	inNumRows	Specifies the number of rows to use for displaying Text Mode captions.
									Must be at least 1 and no more than 32.
			@return		True if successful;  otherwise false.
		**/
		virtual bool						SetTextModeDisplayRowCount (const NTV2Line21Channel inChannel, const UWord inNumRows);

		/**
			@brief		Returns the display attributes that Text Mode captions are currently using (assuming my caption
						channel is Tx1/Tx2/Tx3/Tx4).
			@param[in]	inChannel		Specifies the [Text] caption channel of interest.
			@return		My current Text Mode caption display attributes.
		**/
		virtual const NTV2Line21Attrs &		GetTextModeDisplayAttributes (const NTV2Line21Channel inChannel) const;

		/**
			@brief		Sets the display attributes that Text Mode captions will use henceforth for the given (TxN) caption channel.
			@param[in]	inChannel	Specifies the [Text] caption channel to be configured.
			@param[in]	inAttrs		Specifies the new display attributes for Text Mode captions will have going forward.
			@return		True if successful;  otherwise false.
			@note		This has no effect on captions that have already been decoded (that may be currently displayed).
		**/
		virtual bool						SetTextModeDisplayAttributes (const NTV2Line21Channel inChannel,
																			const NTV2Line21Attrs & inAttrs);
		///@}


		/**
			@name	Change Notification
		**/
		///@{
		/**
			@brief		Subscribes to change notifications.
			@param[in]	pInCallback		Specifies a pointer to the callback function to be called if/when changes occur.
			@param[in]	pInUserData		Optionally specifies a data pointer to be passed to the callback function.
			@return		True if successful;  otherwise False.
			@note		In this implementation, each decoder instance only accommodates a single subscriber.
						Thus, each call to this function replaces the callback/userData used in prior calls to this function.
		**/
		virtual bool						SubscribeChangeNotification (NTV2Caption608Changed * pInCallback,
																		void * pInUserData = AJA_NULL);

		/**
			@brief		Unsubscribes a prior change notification subscription.
			@param[in]	pInCallback		Specifies a pointer to the callback function that was specified in the prior call to SubscribeChangeNotification.
			@param[in]	pInUserData		Specifies the userData pointer that was specified in the prior call to SubscribeChangeNotification.
			@return		True if successful;  otherwise False.
		**/
		virtual bool						UnsubscribeChangeNotification (NTV2Caption608Changed *	pInCallback,
																			void * pInUserData = AJA_NULL);
		///@}

		/**
			@return		True if current flash cycle is off (and a blank space should be displayed instead
						of the character itself).
		**/
		virtual inline bool					IsFlashCycleOff (void) const			{return mFlashCount > CharacterFlashCycleFrames / 2;}

		/**
			@brief	My destructor.
		**/
		virtual								~CNTV2CaptionDecoder608 ();

		virtual NTV2CaptionLogMask			SetLogMask (const NTV2CaptionLogMask inLogMask);


#if !defined(NTV2_DEPRECATE_16_0)
		virtual NTV2_DEPRECATED_16_0(bool BurnCaptions (UByte*	pBuf, const NTV2FrameDimensions	fd, const NTV2PixelFormat pf, const UWord rb));	///< @deprecated	Use BurnCaptions(NTV2Buffer &, const NTV2FormatDescriptor &)
#endif	//	!defined(NTV2_DEPRECATE_16_0)
#if !defined(NTV2_DEPRECATE_16_2)
		static NTV2_DEPRECATED_16_2(CaptionData DecodeCaptionData (const UByte* pFB, const NTV2PixelFormat	pf, const NTV2VideoFormat vf, const NTV2FrameGeometry fg = NTV2_FG_720x486));	///< @deprecated	This function is obsolete
#endif	//	!defined(NTV2_DEPRECATE_16_2)


	//	PRIVATE INSTANCE METHODS
	private:
		virtual bool						New608FieldData (const UByte inCharP1, const UByte inCharP2, const NTV2Line21Field inField);
		virtual bool						ParseCaptionData (const UByte inCharP1, const UByte inCharP2, const NTV2Line21Field inField, const NTV2Line21Channel inChannel);
		virtual bool						ParseXDSData (const UByte inCharP1, const UByte inCharP2, const NTV2Line21Field inField, const NTV2Line21Channel inChannel);

		virtual NTV2Line21Channel			GetCaptionChannel (const UByte inCharP1, const UByte inCharP2, const NTV2Line21Field inField);

		//	Hidden constructors & assignment operators
		explicit							CNTV2CaptionDecoder608 ();
		explicit inline						CNTV2CaptionDecoder608 (const CNTV2CaptionDecoder608 & inDecoderToCopy);
		virtual CNTV2CaptionDecoder608 &	operator = (const CNTV2CaptionDecoder608 & inDecoderToCopy);
	public:
		/**
			@name	Decoder Debug Helpers
		**/
		///@{
		virtual void						DebugPrintCurrentScreen (const bool inAllChannels = false, const bool inShowChars = true, const bool inShowTextChannels = false);
		virtual void						SetDebugRowsOfInterest (const NTV2Line21Channel inChannel, const UWord inFromRow, const UWord inToRow, const bool inAdd = false);
		virtual void						SetDebugColumnsOfInterest (const NTV2Line21Channel inChannel, const UWord inFromCol, const UWord inToCol, const bool inAdd = false);
		///@}
		virtual CNTV2CaptionDecodeChannel608Ptr	Get608ChannelDecoder (const NTV2Line21Channel inChannel) const;

		/**
			@name	Global Decoder Operation Options
		**/
		///@{
		static bool							UseNewBurnCaptionsMethod;	///< @brief	Default is false. Reserved for future use.
		static bool							RenderPrePostSpaces;		///< @brief	Default is true. If true, BurnCaptions leads & follows character runs with blank space
		static bool							AutoCallIdleFrame;			///< @brief	Default is false. If true, ProcessNew608FrameData will automatically call IdleFrame
		static bool							AutoFlashOnAirChars;		///< @brief	Default is false. If true, GetOnAir... functions will automatically "flash" (periodically return spaces)
		///@}

	private:
		virtual void						Handle608ChangeNotification (const NTV2Caption608ChangeInfo & inChangeInfo) const;
		static void							NTV2Caption608ChangeHandler (void * pInstance, const NTV2Caption608ChangeInfo & inChangeInfo);


	//	INSTANCE DATA
	private:
		typedef std::vector <CNTV2CaptionDecodeChannel608Ptr>	ChannelDecoderArray;	/// @brief	An ordered sequence of CNTV2CaptionDecodeChannel608 instances

		NTV2Line21Channel			mDisplayChannel;		///< @brief	The captioning channel (CC1, CC2, Text1, etc.) I'm supposed to decode
		NTV2Line21Channel			mCurrXmitChannel [2];	///< @brief	The captioning channel currently being transmitted in each field (CC1 or CC2)

		ChannelDecoderArray			mChannelDecoders;		///< @brief	One of these per captioning channel (but not the XDS channel)
																///<		(i.e. I'm decoding all channels in parallel, but only displaying 1-at-a-time)

		CNTV2XDSDecodeChannel608Ptr	mXDSDecode;				///< @brief	A place to send the XDS data to

		unsigned short				mLastControlCode [2];	///< @brief	Used to remember last 16-bit control code for each field (to handle duplicate transmissions)
		UWord						mRollOffset;			///< @brief	Used to do dynamic "roll" at Carriage Return points
		UWord						mFlashCount;			///< @brief	Used to track when flash characters are displayed versus blanked
		NTV2Caption608Changed *		mpChangeSubscriber;		///< @brief	User callback for change notifications
		void *						mpSubscriberData;		///< @brief	User data for change notifications
	#if defined (AJA_DEBUG)
		public:
			static bool					ClassTest (void);
		protected:
			bool						InstanceTest (void);
	#endif	//	AJA_DEBUG

};	//	CNTV2CaptionDecoder608

#endif	// __NTV2_CEA608_DECODER_
