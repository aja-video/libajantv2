/**
	@file		ntv2captiontranslator608to708.h
	@brief		Declares the CNTV2CaptionTranslator608to708 class.
	@copyright	(C) 2006-2022 AJA Video Systems, Inc. All rights reserved.
**/

#ifndef __NTV2_CEA608to708_TRANSLATOR_
#define __NTV2_CEA608to708_TRANSLATOR_

#include "ntv2caption608types.h"
#include "ntv2captiontranslatorchannel608to708.h"
#include "ntv2xdscaptiondecodechannel608.h"
#include <vector>


#ifdef MSWindows
	#include "windows.h"
	#include "stdio.h"
#endif


/**
	@brief	I translate CEA-608 ("Line 21") captions into CEA-708 ("DTVCC") captions.
			I accept 608 caption data (4 bytes/frame) and parse it to the appropriate channel (CC1, CC2, etc.)
			where the current state and buffer status is maintained. The 608 data and commands for each channel
			are then translated into CEA-708 "Service Blocks" which are queued according to channel priorities.

			To implement a basic translator, instantiate me (using Create) then call Translate608CCData every frame,
			passing it the four bytes of CEA-608 "line 21" data decoded from the host frame buffer (or from the Anc
			buffer, which is supported by newer AJA hardware.)

			I provide three ways to make use of the translated CEA-708 caption data:
			1)	Call GetCaptionChannelPacket to get direct access to my encoder's ancillary data buffer.
			2)	Call CreateSMPTE334Anc to generate and copy the SMPTE334 data into a local buffer.
			3)	Call InsertSMPTE334AncPacketInVideoFrame to insert the SMPTE334 data into a local host frame buffer.

			I also provide methods for customizing which CEA-608 channels get translated, and their mapping to
			CEA-708 "services". By default, all CEA-608 channels NTV2_CC608_CC1 thru NTV2_CC608_Text4 are translated,
			and by default, they're mapped to service numbers 1 thru 8. Call Set708ServiceNumber to change the mapping.
			Call Set708TranslateEnable to disable or enable the translation of a particular CEA-608 caption channel.
**/
class CNTV2CaptionTranslator608to708;
typedef AJARefPtr <CNTV2CaptionTranslator608to708>	CNTV2CaptionTranslator608to708Ptr;

class AJAExport CNTV2CaptionTranslator608to708 : public CNTV2CaptionLogConfig
{
	//	CLASS METHODS
	public:
		/**
			@brief	Creates an instance of me.
			@param[out]	outInstance		Receives the newly-created NTV2CaptionTranslator608to708 instance.
			@return		True if successful; otherwise false.
			@note		This method catches std::bad_alloc. If a new translator cannot be allocated, false
						will be returned, and outInstance will contain a NULL pointer.
		**/
		static bool		Create (CNTV2CaptionTranslator608to708Ptr & outInstance);


	//	INSTANCE METHODS
	public:
		/**
			@brief	Resets me, clearing all of my channel decoders, and flushing any/all in-progress data.
			@note	This method is not thread-safe.
		**/
		virtual void	Reset (void);

		/**
			@brief	Specifies the CEA-608 channel to focus debugging on.
			@param[in]	chan	Specifies the "line 21" channel to debug. Must be one of NTV2_CC608_CC1...NTV2_CC608_CC4,
								NTV2_CC608_Text1...NTV2_CC608_Text4, or NTV2_CC608_XDS.
		**/
		virtual	bool	SetDisplayChannel (const NTV2Line21Channel chan);

		/**
			@brief	Maps the specified CEA-608 caption channel to the specified CEA-708 service number.
			@param[in]	chan			Specifies the CEA-608 caption channel to be re-mapped. This must be in the
										range of NTV2_CC608_CC1 thru NTV2_CC608_Text4 (inclusive).
			@param[in]	serviceNum		Specifies the CEA-708 caption "service number", which must be in the range
										0 thru 64 (inclusive).
			@return		True if successful; otherwise false.
		**/
		virtual bool	Set708ServiceNumber (const NTV2Line21Channel chan, const int serviceNum);


		/**
			@brief	Returns the CEA-708 caption "service number" that's currently mapped to the given CEA-608 caption channel.
			@param[in]	chan	Specifies the CEA-608 caption channel of interest. This must be in the
								range of NTV2_CC608_CC1 thru NTV2_CC608_Text4 (inclusive).
			@return	If successful, the CEA-708 caption "service number" that's currently mapped to the given caption channel;
					otherwise zero.
		**/
		virtual int		Get708ServiceNumber (const NTV2Line21Channel chan) const;

		/**
			@brief	Controls whether or not the given CEA-608 caption channel will be translated into an equivalent CEA-708 caption service.
			@param[in]	chan		Specifies the CEA-608 caption channel to be enabled or disabled. This must be in the
									range of NTV2_CC608_CC1 thru NTV2_CC608_Text4 (inclusive).
			@param[in]	bEnable		Enables translation if true; otherwise disables translation.
			@return		True if successful; otherwise false.
		**/
		virtual bool	Set708TranslateEnable (const NTV2Line21Channel chan, bool bEnable);


		/**
			@brief	Returns true if the given CEA-608 caption channel is currently being translated to an equivalent
					CEA-708 caption service.
			@param[in]	chan	Specifies the CEA-608 caption channel of interest. This must be in the range of
								NTV2_CC608_CC1 thru NTV2_CC608_Text4 (inclusive).
			@return	True if the given caption channel is currently being translated; otherwise false.
		**/
		virtual bool	Get708TranslateEnable (const NTV2Line21Channel chan) const;

		/**
			@brief	Returns a non-const pointer to my CEA-708 caption encoder's data packet buffer and also returns the number
					of data bytes it currently contains. This gives clients the ability to interrogate the Ancillary data and/or
					modify it, if needed.
			@param[out]	outDataPtr		Receives the address of my 708 encoder's ancillary data buffer.
			@param[out]	outByteCount	Receives the number of data bytes currently held in my 708 encoder's ancillary data buffer.
										The returned value will never exceed NTV2_CC708MaxPktSize.
			@return		True if successful; otherwise false.
		**/
		virtual bool	GetCaptionChannelPacket (UBytePtr & outDataPtr, size_t & outByteCount);

		/**
			@brief	Translates the given CEA-608 caption data bytes for a full frame of video.
			@param[in]	inCC608Data		Specifies an entire frame's worth of CEA-608 caption data bytes.
			@return		True if successful; otherwise false.
			@note		This method should be called once per frame, even if no caption data was present in the frame.
						CEA-608 caption rules call for certain commands to be sent twice on adjacent frames, which means
						that if there's a difference between |command|command| and |command|null|command|, if I never
						see the intervening null frames, I may mistakenly think two commands arrived from adjacent frames
						and misinterpret them.
		**/
		virtual bool	Translate608CCData (const CaptionData & inCC608Data);

		/**
			@brief		Builds and returns a SMPTE 334 ancillary data packet that contains the latest translated CEA-708
						caption message.
			@param[in]	frameRate			Specifies the output frame rate.
			@param[in]	field				Specifies the "line 21" field (if interlaced).
			@param[out]	outAncPacketData	Specifies the buffer that is to receive the ancillary data packet.
			@param[out]	outSize				On input, specifies the capacity, in bytes, of the specified buffer.
											On output, receives the number of bytes that were copied into the buffer.
			@return		True if successful; false if unsuccessful.
			@note		This method should be called once per (output) frame.
		**/
		virtual bool	CreateSMPTE334Anc (const NTV2FrameRate frameRate, const NTV2Line21Field field, UWordPtr & outAncPacketData, size_t & outSize);

		/**
			@brief	Inserts any translated CEA-708 caption data into the given host frame buffer's VANC area with the given line offset.
			@param		pFrameBuffer	Specifies a valid, non-NULL starting address of the host frame buffer into which the SMPTE334
										caption data will be written.
			@param[in]	inVideoFormat	Specifies the video format.
			@param[in]	inPixelFormat	Specifies the pixel format of the host frame buffer.
			@param[in]	inLineNumber	Specifies the line offset into the host frame buffer.
			@return		True if successful; otherwise false.
		**/
		virtual bool	InsertSMPTE334AncPacketInVideoFrame (void * pFrameBuffer, const NTV2VideoFormat inVideoFormat, const NTV2FrameBufferFormat inPixelFormat, const ULWord inLineNumber) const;


		//	DEBUG METHODS
		virtual NTV2CaptionLogMask	SetLogMask (const NTV2CaptionLogMask inLogMask);
		virtual void				Set608TestIDMode (bool bTest);

		virtual						~CNTV2CaptionTranslator608to708 ();


	//	PRIVATE INSTANCE METHODS
	private:
		virtual bool				New608FrameData (const CaptionData & inCC608Data);
		virtual bool				New608FieldData (UByte char608_1, UByte char608_2, NTV2Line21Field field);

		virtual bool				ParseCaptionData (UByte charP1, UByte charP2, NTV2Line21Field field, NTV2Line21Channel currChannel);
		virtual bool				ParseXDSData (UByte charP1, UByte charP2, NTV2Line21Field field);	//, NTV2Line21Channel currChannel);
		virtual NTV2Line21Channel	GetCaptionChannel (UByte charP1, UByte charP2, NTV2Line21Field field);

		virtual bool				Combine708CaptionChannelData (const NTV2FrameRate frameRate);
		virtual size_t				MaxCaptionChannelDataForFrameRate (NTV2FrameRate ntv2Rate);
		virtual bool				AddChannelServiceBlockData (NTV2Line21Channel channel, UByte * pEncodeData, size_t index, size_t maxIndex, size_t & outEndIndex);
		virtual bool				MapChannelServiceNumbers (void);

		// Debug
		virtual void				DebugPrintCurrentScreen (void) const;

		//	Hidden Constructors and Assignment Operator
		explicit									CNTV2CaptionTranslator608to708 ();
		explicit									CNTV2CaptionTranslator608to708 (const CNTV2CaptionTranslator608to708 & inObj);
		virtual CNTV2CaptionTranslator608to708 &	operator =	(const CNTV2CaptionTranslator608to708 & inObj);

		typedef std::vector <CNTV2CaptionTranslatorChannel608to708Ptr>	TranslatorArray;


	//	INSTANCE DATA
	private:
		NTV2Line21Channel			mDisplayChannel;			///< @brief	Captioning channel (CC1, CC2, Text1, etc.) we want to decode for debugging
		NTV2Line21Channel			mCurrXmitChannel [2];		///< @brief	Captioning channel currently being transmitted for each field (CC1 or CC2)
		TranslatorArray				mChannelDecoders;			///< @brief	One 608-to-708 caption translator per caption channel (but not XDS) -- all channels decoded in parallel
		bool						mEnableEmbedded608Output;	///< @brief	Set 'true' if embedded 608 data is to be output to SMPTE 334 (why would this ever NOT be the case???)
																///< @note	This ONLY enables the embedded 608 data output. The translated 708 channels are enabled in
																///<		their separate decode channels.
		CNTV2XDSDecodeChannel608Ptr	mXDSDecoder;				///< @brief	A place to send the XDS data to
		unsigned short				mLastControlCode [2];		///< @brief	Remembers the last 16-bit control code for each field (to handle duplicate transmissions)
		CNTV2CaptionEncoder708Ptr	m708Encoder;				///< @brief	CEA-708 encoder used to create 708 messages

};	//	CNTV2CaptionTranslator608to708

#endif	// __NTV2_CEA608to708_TRANSLATOR_
