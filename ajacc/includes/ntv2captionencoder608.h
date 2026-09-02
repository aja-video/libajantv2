/**
	@file		ntv2captionencoder608.h
	@brief		Declares the CNTV2CaptionEncoder608 class.
	@copyright	(C) 2006-2022 AJA Video Systems, Inc. All rights reserved.
**/

#ifndef __NTV2_CEA608_ENCODER_
#define __NTV2_CEA608_ENCODER_

#include "ntv2line21captioner.h"
#include "ntv2caption608types.h"
#include "ntv2caption608messagequeue.h"
#include "ajabase/common/ajarefptr.h"
#include <string>
#include <deque>


#ifdef MSWindows
	#include "windows.h"
	#include "stdio.h"
#endif

#ifdef AJALinux
	#include <stdio.h>
#endif


/**
	@brief	I am used to create CEA-608 ("Line 21") caption messages, queue the data for every-frame processing,
			and generate the Line 21 waveform for blitting (keying) into an SD frame buffer.

			Simply call my CNTV2CaptionEncoder608::Create method to instantiate an instance of me, then use one of
			my Enqueue methods to enqueue a message. Then call one of my dequeueing methods
			(e.g., CNTV2CaptionEncoder608::EncodeNextCaptionBytesIntoLine21) once for each outgoing field
			(or twice per frame).

			There are three sets of public methods. The first set can be used to enqueue simple CEA-608 messages.
			These take a character string and some attributes (color, format, starting location on screen, etc.),
			format the CEA-608 message, and add it to my queue for the designated captioning channel.

			The second set of methods is usually called twice per output frame (once per field). This pulls two
			bytes from the designated field's current message and calls a CNTV2CaptionEncoder608::CNTV2Line21Captioner
			to generate the appropriate Line 21 waveform.

			The third set of methods are for status inquiry.

	@note	I utilize thread-safe queues, so clients can use a thread for enqueuing messages, and another thread
			to grab the next CEA-608 bytes or encode each field's line 21 waveform.
**/
class CNTV2CaptionEncoder608;
typedef AJARefPtr <CNTV2CaptionEncoder608>	CNTV2CaptionEncoder608Ptr;

class AJAExport CNTV2CaptionEncoder608 : public CNTV2CaptionLogConfig
{
	//	CLASS METHODS
	public:
		/**
			@brief		Creates a new CNTV2CaptionEncoder608 instance.
			@param[out]	outEncoder	Receives the newly-created encoder instance.
			@return		True if successful; otherwise False.
			@note		This method will catch any "bad_alloc" exception and return False
						if there's insufficient free memory to allocate the new encoder.
		**/
		static bool				Create (CNTV2CaptionEncoder608Ptr & outEncoder);


	//	INSTANCE METHODS
	public:
		/**
			@name	Enqueueing
		**/
		///@{

		/**
			@brief		Enqueues the given message for eventual "pop-on" display.
			@param[in]	inMessageStr		Specifies the message to be enqueued for eventual dequeuing/encoding/display.
											Must not be empty.
											May contain line-breaks, but only the first four lines will be utilized.
											Must not utilize any Unicode encoding (e.g., UTF-8).
											May contain two-byte CEA-608 command sequences for encoding special characters,
											but these are caption-channel specific, and thus, should match what's specified
											in the "inChannel" parameter. The caller is also responsible for abiding by the
											CEA-608 standard practice of immediately preceding each special character with
											its ASCII equivalent (as downstream decoders that support special characters are
											expected to "backspace" over the ASCII equivalent with the correct glyph).
			@param[in]	inChannel			Specifies the CEA-608 caption channel to use, which must be CC1, CC2, CC3 or CC4.
			@param[in]	inRowNumber			Specifies the row number at which the caption should be displayed.
											Defaults to zero, which centers it vertically.
			@param[in]	inColumnNumber		Specifies the column number at which the caption should be displayed.
											Defaults to zero, which centers it horizontally.
			@param[in]	inDisplayAttribs	Specifies a valid, non-NULL pointer to the display attributes to be used.
											Defaults to the default display attributes.
			@return		True if enqueued successfully; otherwise False.
		**/
		virtual bool			EnqueuePopOnMessage		(const std::string &	inMessageStr,
														const NTV2Line21Channel	inChannel			= NTV2_CC608_CC1,
														const UWord				inRowNumber			= 0,
														const UWord				inColumnNumber		= 0,
														const NTV2Line21Attrs &	inDisplayAttribs	= NTV2Line21Attrs());


		/**
			@brief		Enqueues the given message for eventual "paint-on" display.
			@param[in]	inMessageStr		Specifies the message to be enqueued for eventual dequeuing/encoding/display.
											Must not be empty.
											May contain line-breaks, but only the first four lines will be utilized.
											Must not utilize any Unicode encoding (e.g., UTF-8).
											May contain two-byte CEA-608 command sequences for encoding special characters,
											but these are caption-channel specific, and thus, should match what's specified
											in the "inChannel" parameter. The caller is also responsible for abiding by the
											CEA-608 standard practice of immediately preceding each special character with
											its ASCII equivalent (as downstream decoders that support special characters are
											expected to "backspace" over the ASCII equivalent with the correct glyph).
			@param[in]	inEraseFirst		If true, specifies that the caption area should be erased before displaying the message;
											otherwise the message will simply be painted on top of what is already in the caption
											area (the default).
			@param[in]	inChannel			Specifies the CEA-608 caption channel to use, which must be CC1, CC2, CC3 or CC4.
			@param[in]	inRowNumber			Specifies the row number at which the caption should be displayed. Defaults to zero.
			@param[in]	inColumnNumber		Specifies the column number at which the caption should be displayed. Defaults to zero.
			@param[in]	inDisplayAttribs	Specifies a valid, non-NULL pointer to the display attributes to be used.
											Defaults to the default display attributes.
			@return		True if enqueued successfully; otherwise False.
		**/
		virtual bool			EnqueuePaintOnMessage	(const std::string &	inMessageStr,
														const bool				inEraseFirst		= false,
														const NTV2Line21Channel	inChannel			= NTV2_CC608_CC1,
														const UWord				inRowNumber			= 0,
														const UWord				inColumnNumber		= 0,
														const NTV2Line21Attrs &	inDisplayAttribs	= NTV2Line21Attrs());

		/**
			@brief		Enqueues the given message for eventual "roll up" display.
			@param[in]	inMessageStr		Specifies the message to be enqueued for eventual dequeuing/encoding/display.
											Must not exceed NTV2_CC608_MaxCol characters in length.
											Must not be empty.
											Must not contain any line-break characters.
											Must not utilize any Unicode encoding (e.g., UTF-8).
											May contain two-byte CEA-608 command sequences for encoding special characters,
											but these are caption-channel specific, and thus, should match what's specified
											in the "inChannel" parameter. The caller is also responsible for abiding by the
											CEA-608 standard practice of immediately preceding each special character with
											its ASCII equivalent (as downstream decoders that support special characters are
											expected to "backspace" over the ASCII equivalent with the correct glyph).
			@param[in]	inRollMode			Specifies the "roll-up" behavior to use -- 2, 3 or 4-row roll-up.
											Defaults to NTV2_CC608_CapModeRollUp4 (4-line roll-up).
			@param[in]	inChannel			Specifies the CEA-608 caption channel to use, which must be CC1, CC2, CC3 or CC4.
											Defaults to NTV2_CC608_CC1.
			@param[in]	inRowNumber			Specifies the row number at which the caption should be displayed.
											Defaults to the most appropriate bottom-most row.
			@param[in]	inColumnNumber		Specifies the column number at which the caption should be displayed.
											Defaults to the most appropriate left-most column.
			@param[in]	inDisplayAttribs	Specifies a valid, non-NULL pointer to the display attributes to be used.
											Defaults to the default display attributes.
			@return		True if enqueued successfully; otherwise False.
		**/
		virtual bool			EnqueueRollUpMessage	(const std::string &	inMessageStr,
														const NTV2Line21Mode	inRollMode			= NTV2_CC608_CapModeRollUp4,
														const NTV2Line21Channel	inChannel			= NTV2_CC608_CC1,
														const UWord				inRowNumber			= NTV2_CC608_MaxRow,
														const UWord				inColumnNumber		= NTV2_CC608_MinCol,
														const NTV2Line21Attrs &	inDisplayAttribs	= NTV2Line21Attrs());

		/**
			@brief		Enqueues the given message for eventual reception and possible display on a receiver capable of displaying Tx data.
			@param[in]	inMessageStr		Specifies the message to be enqueued for eventual dequeuing/encoding/display.
											The string must not be empty, nor contain line breaks, nor contain any characters not in
											the ISO 8859-1 (Latin 1) character set.
			@param[in]	inEraseFirst		If true, specifies that the caption area should be erased before displaying the message;
											otherwise the message will simply be painted on top of what is already in the caption
											area (the default).
			@param[in]	inChannel			Specifies the CEA-608 caption channel to use, which must be Text1, Text2, Text3 or Text4.
			@return		True if enqueued successfully; otherwise False.
			@note		Text messages are lower priority than normal captions, and won't get dequeued until all pending CC1/CC2/CC3/CC4
						messages have been dequeued.
		**/
		virtual bool			EnqueueTextMessage		(const std::string &		inMessageStr,
														const bool					inEraseFirst	= false,
														const NTV2Line21Channel		inChannel		= NTV2_CC608_Text1);

		/**
			@brief		This is a low-level method that enqueues the given CaptionData for eventual transmission.
			@param[in]	inCaptionData		Specifies the CaptionData to be enqueued for eventual transmission.
											Field 1 data bytes are queued for NTV2_CC608_CC1, NTV2_CC608_CC2,
											NTV2_CC608_Text1, or NTV2_CC608_Text2 transmission.
											Field 2 data bytes are queued for NTV2_CC608_CC3, NTV2_CC608_CC4,
											NTV2_CC608_Text3, or NTV2_CC608_Text4 transmission.
			@note		Successful use of this function requires a thorough knowledge of the CEA-608 specification.
			@return		True if enqueued successfully; otherwise False.
		**/
		virtual bool			EnqueueCaptionData		(const CaptionData &		inCaptionData);

		/**
			@brief		Adds a delay to the given channel queue.
			@param[in]	inFrameCount		Specifies the delay, in frames.
			@param[in]	inChannel			Specifies the CEA-608 caption channel to use. Defaults to NTV2_CC608_CC1.
			@note		Successful use of this function requires a thorough knowledge of the CEA-608 specification.
			@return		True if enqueued successfully; otherwise False.
		**/
		virtual bool			EnqueueDelay		(const uint32_t inFrameCount, const NTV2Line21Channel inChannel = NTV2_CC608_CC1);
		///@}

		/**
			@name	Dequeueing
		**/
		///@{

		/**
			@brief		For the given field, pops the next two pending caption bytes from the current message being transmitted,
						and encodes them into a valid "line 21" waveform in the given frame buffer. If there is no current message
						being transmitted, the next pending caption message is dequeued and is started transmitting. If there's
						nothing being transmitted and nothing in my queue, the frame buffer is not modified.
			@param		pFrameBuffer	A valid, non-NULL pointer to the frame buffer whose "line 21" will be replaced
										if there are caption bytes to transmit.
			@param[in]	inPixelFormat	Specifies the format of the given frame buffer. (Must be 8 or 10 bit YUV.)
			@param[in]	inVideoFormat	Specifies the video format. (Currently only NTV2_FORMAT_525_5994 is supported.)
			@param[in]	inFieldNum		Specifies the field of interest -- NTV2_CC608_Field1 or NTV2_CC608_Field2.
			@return		True if successful; otherwise False.
		**/
		virtual bool			EncodeNextCaptionBytesIntoLine21 (void *					pFrameBuffer,
																	const NTV2PixelFormat	inPixelFormat,
																	const NTV2VideoFormat	inVideoFormat,
																	const NTV2Line21Field	inFieldNum);


		/**
			@brief		This high-level method is used to dequeue caption data for an entire frame.
			@param[out]	outCaptionData	Receives the dequeued caption data for an entire frame.
			@return		True if any caption bytes are available in the returned CaptionData structure; otherwise false.
		**/
		virtual bool			GetNextCaptionData (CaptionData & outCaptionData);

		/**
			@brief		This calls my lower level "GetNextTransmitCaptionBytes" method (below), and is typically
						called twice per frame (once per field). If there is a pending message in my queue for the
						designated field, it pulls the next two bytes out of the message, encodes them into a "Line
						21" video waveform and returns a pointer to the waveform buffer. If the two bytes complete
						the current message, the message is automatically "popped" from the queue and the next
						message (if any) is rippled into place for the next frame.
			@param[in]	inFieldNum		Specifies the field of interest -- NTV2_CC608_Field1 or NTV2_CC608_Field2.
			@return		Pointer to the properly encoded Line 21 video data, or NULL if error.
		**/
		virtual UByte *			GetNextLine21TransmitCaptions (const NTV2Line21Field inFieldNum);


		/**
			@brief		This low-level function is called by the higher level "GetNextLine21TransmitCaptions" function.
						Typically, this is called twice per frame (once per field). If there is a pending message in
						my queue for the designated field, it pulls the next two bytes out of the message, and returns
						them via the "outChar1" and "outChar2" output parameters. If the two bytes complete the current
						message, the message is automatically "popped" from my queue and the next message (if any) is
						rippled into place for the next frame.
			@param[in]	inFieldNum	Specifies the field -- NTV2_CC608_Field1 or NTV2_CC608_Field2.
			@param[out]	outChar1	Receives the first byte code, with the appropriate parity applied.
			@param[out]	outChar2	Receives the second byte code, with the appropriate parity applied.
			@return		True if any data was available; otherwise false.
		**/
		virtual bool			GetNextTransmitCaptionBytes (const NTV2Line21Field inFieldNum, UByte & outChar1, UByte & outChar2);
		///@}


		/**
			@name	Inquiry
		**/
		///@{

		/**
			@brief		Answers with the current depth of my queue (for the given CC608 field).
			@param[in]	inFieldNum		Specifies the c608 caption field of interest. Defaults to NTV2_CC608_Field1.
			@return		The current number of queued messages.
		**/
		virtual size_t			GetQueuedMessageCount (const NTV2Line21Field inFieldNum = NTV2_CC608_Field1) const;

		/**
			@brief		Answers with the total number of bytes (including command bytes) that are
						currently queued (for the given CC608 field).
			@param[in]	inFieldNum		Specifies the c608 caption field of interest. Defaults to NTV2_CC608_Field1.
			@return		The current number of queued bytes.
		**/
		virtual size_t			GetQueuedByteCount (const NTV2Line21Field inFieldNum = NTV2_CC608_Field1) const;

		/**
			@brief		Flushes all queued command bytes for the given CC608 field.
			@param[in]	inFieldNum			Specifies the c608 caption field of interest. Defaults to NTV2_CC608_Field1.
			@param[in]	inAlsoInProgress	If true, also flush any in-progress messages;  otherwise leave in-progress messages intact.
											Defaults to true.
		**/
		virtual void			Flush (const NTV2Line21Field inFieldNum = NTV2_CC608_Field1, const bool inAlsoInProgress = true);

		/**
			@brief		Flushes all queued command bytes for the given caption channel.
			@param[in]	inChannel			Specifies the caption channel of interest.
			@param[in]	inAlsoInProgress	If true, also flush any in-progress messages for the channel;
											otherwise leave in-progress messages intact. Defaults to true.
		**/
		virtual void			FlushChannel (const NTV2Line21Channel inChannel, const bool inAlsoInProgress = true);


		/**
			@brief		Clears captions on the given channel by emitting ENM (Erase Non-displayed Memory) and EOC (End Of
						Caption) control codes.
			@param[in]	inChannel		Specifies the CEA-608 caption channel to use, which must be CC1, CC2, CC3 or CC4.
										Illegal values are overridden to use the default CC1 value (NTV2_CC608_CC1).
			@return		True if erase control codes were enqueued successfully; otherwise False.
		**/
		virtual bool			Erase (const NTV2Line21Channel	inChannel	= NTV2_CC608_CC1);


		/**
			@brief		Returns queue information for a given caption channel.
			@param[in]	inChannel			Specifies the CEA608 caption channel of interest.
			@param[out]	outBytesQueued		Receives the total number of message bytes queued for the given channel.
			@param[out]	outMessagesQueued	Receives the total number of messages queued for the given channel.
		**/
		virtual void			GetQueueInfoForChannel (const NTV2Line21Channel inChannel, size_t & outBytesQueued, size_t & outMessagesQueued) const;


		//	Stats

		/**
			@brief		Returns the total number of bytes (including command bytes) that have been
						enqueued onto me since I was instantiated.
		**/
		virtual inline size_t	GetEnqueueByteTally (void) const			{return mXmitMsgQueueF1.GetEnqueueByteTally () + mXmitMsgQueueF2.GetEnqueueByteTally ();}

		/**
			@brief		Returns the total number of bytes (including command bytes) that have been
						dequeued from me since I was instantiated.
		**/
		virtual inline size_t	GetDequeueByteTally (void) const			{return mXmitMsgQueueF1.GetDequeueByteTally () + mXmitMsgQueueF2.GetDequeueByteTally ();}

		/**
			@brief		Returns the total number of messages that have been enqueued onto me since I was instantiated.
		**/
		virtual inline size_t	GetEnqueueMessageTally (void) const			{return mXmitMsgQueueF1.GetEnqueueMessageTally () + mXmitMsgQueueF2.GetEnqueueMessageTally ();}

		/**
			@brief		Returns the total number of messages that have been dequeued from me since I was instantiated.
		**/
		virtual inline size_t	GetDequeueMessageTally (void) const			{return mXmitMsgQueueF1.GetDequeueMessageTally () + mXmitMsgQueueF2.GetDequeueMessageTally ();}

		/**
			@brief		Returns the highest queue depth -- i.e., the maximum number of messages I held in either of
						my queues -- since I was instantiated.
		**/
		virtual inline size_t	GetHighestQueueDepth (void) const			{return mXmitMsgQueueF1.GetHighestQueueDepth () + mXmitMsgQueueF2.GetHighestQueueDepth ();}


		/**
			@brief		Returns stats for a given caption channel.
			@param[in]	inChannel			Specifies the CEA608 caption channel of interest.
			@param[out]	outEnqueueBytes		Receives the total number of message bytes enqueued for the given channel.
			@param[out]	outEnqueueMsgs		Receives the total number of messages enqueued for the given channel.
			@param[out]	outDequeueBytes		Receives the total number of message bytes ever dequeued for the given channel.
			@param[out]	outDequeueMsgs		Receives the total number of messages dequeued for the given channel.
		**/
		virtual void			GetQueueStatsForChannel (const NTV2Line21Channel inChannel,
														size_t & outEnqueueBytes,	size_t & outEnqueueMsgs,
														size_t & outDequeueBytes,	size_t & outDequeueMsgs) const;
		///@}

		/**
			@brief		My destructor.
		**/
		virtual					~CNTV2CaptionEncoder608 ();

		virtual NTV2CaptionLogMask	SetLogMask (const NTV2CaptionLogMask inLogMask);

#if !defined(NTV2_DEPRECATE_17_0)
		virtual inline NTV2_DEPRECATED_17_0(bool EnqueuePopOnMessage (const std::string & str, const NTV2Line21Channel ch, const UWord row, const UWord col, const NTV2Line21AttrsPtr pAttrs))	{NTV2Line21Attrs tmp; return EnqueuePopOnMessage(str, ch, row, col, pAttrs ? *pAttrs : tmp);}	///< @deprecated	Call EnqueuePopOnMessage(..., const NTV2Line21Attrs &) instead.
		virtual inline NTV2_DEPRECATED_17_0(bool EnqueuePaintOnMessage (const std::string & str, const bool erase, const NTV2Line21Channel ch, const UWord row, const UWord col, const NTV2Line21AttrsPtr pAttrs))	{NTV2Line21Attrs tmp; return EnqueuePaintOnMessage(str, erase, ch, row, col, pAttrs ? *pAttrs : tmp);}	///< @deprecated	Call EnqueuePaintOnMessage(..., const NTV2Line21Attrs &) instead.
		virtual inline NTV2_DEPRECATED_17_0(bool EnqueueRollUpMessage (const std::string & str, const NTV2Line21Mode mode, const NTV2Line21Channel ch, const UWord row, const UWord col, const NTV2Line21AttrsPtr pAttrs))	{NTV2Line21Attrs tmp; return EnqueueRollUpMessage(str, mode, ch, row, col, pAttrs ? *pAttrs : tmp);}	///< @deprecated	Call EnqueueRollUpMessage(..., const NTV2Line21Attrs &) instead.
#endif	//	defined(NTV2_DEPRECATE_17_0)


	//	Private Instance Methods
	private:
		virtual CNTV2608MsgPtr	GetNextCaptionMessage (const NTV2Line21Field inFieldNum);
		virtual bool			EnqueueCaptionMessage (const NTV2Line21Field inFieldNum, CNTV2608MsgPtr pInMsg);

		virtual bool			InsertPACAndPenModeCommands (const NTV2Line21Channel	inChannel,
															const UWord				inRowNumber,
															const UWord				inColumnNumber,
															const NTV2Line21Attrs &	inDisplayAttribs,
															CNTV2608MsgPtr			pCapMsg);

		virtual UWord			GetWhitePACCommand (const NTV2Line21Channel inChannel,
													const UWord				inRowNumber,
													const UWord				inColumnNumber,
													const NTV2Line21Attrs &	inDisplayAttribs,
													UWord &					outExtraColumn);

		virtual UWord			GetColorPACCommand (const NTV2Line21Channel	inChannel,
													const UWord				inRowNumber,
													const NTV2Line21Attrs &	inDisplayAttribs);

		virtual UWord			GetMidRowCommand (const NTV2Line21Channel	inChannel,
													const NTV2Line21Attrs &	inDisplayAttribs);

		//	Hidden constructors & assignment operators
		explicit				CNTV2CaptionEncoder608 ();
		explicit inline			CNTV2CaptionEncoder608 (const CNTV2CaptionEncoder608 & inEncoderToCopy)	: 	CNTV2CaptionLogConfig()	{(void) inEncoderToCopy;}
		virtual inline CNTV2CaptionEncoder608 &	operator = (const CNTV2CaptionEncoder608 & inEncoderToCopy)					{(void) inEncoderToCopy; return *this;}


	//	Private Class Methods
	private:
		/**
			@brief		Returns the given byte value after applying the appropriate CEA-608 parity information.
			@param[in]	inByte	Specifies the byte value to be encoded.
			@return		The CEA-608-parity-encoded byte value.
		**/
		static UByte		CC608OddParity (const UByte inByte);


	//	INSTANCE DATA
	private:
		CNTV2Line21Captioner		mLine21Encoder;			///< @brief	Keep a line 21 captioner handy

		CNTV2Caption608MessageQueue	mXmitMsgQueueF1;		///< @brief	Field 1 (CC1/CC2) transmit caption message queue
		CNTV2Caption608MessageQueue	mXmitMsgQueueF2;		///< @brief	Field 2 (CC3/CC4) transmit caption message queue

		CNTV2608MsgPtr	mpXmitCurrentF1Msg;		///< @brief	Caption Message currently being transmitted on Field 1
		CNTV2608MsgPtr	mpXmitCurrentF2Msg;		///< @brief	Caption Message currently being transmitted on Field 2

};	//	CNTV2CaptionEncoder608


/**
	@brief	A small collection of UTF-8 utility functions.
**/
class AJAExport CUtf8Helpers
{
	public:
		/**
			@brief		Converts the given UTF8-encoded string into a string that can be used in the CNTV2CaptionEncoder608's
						Enqueue____Message calls. "Special" characters that are supported in the CEA-608 extended character sets
						will be replaced with a three-byte sequence -- the first byte being the ASCII equivalent (for compatibility
						with decoders that don't handle the extended character sets), and the subsequent two-byte CEA-608 byte
						sequence that's appropriate for the given caption channel.
			@param[in]	inUtf8Str	The string containing UTF8-encoded characters to be converted.
			@param[in]	inChannel	The caption channel to be used. Defaults to NTV2_CC608_CC1.
			@return		The CEA-608 equivalent string that can be passed into any of the Enqueue____Message functions.
		**/
		static std::string		Utf8ToCEA608String (const std::string & inUtf8Str, const NTV2Line21Channel inChannel = NTV2_CC608_CC1);

		/**
			@brief		Converts the given Unicode codepoint into a string containing its equivalent CEA-608 three-byte sequence,
						suitable for use in the CNTV2CaptionEncoder608's Enqueue____Message calls. "Special" characters that are
						supported in the CEA-608 extended character sets are converted into a three-byte sequence -- the first byte
						being the ASCII equivalent (for compatibility with decoders that don't handle the extended character sets),
						and the subsequent two-byte CEA-608 byte sequence that's appropriate for the given caption channel.
			@param[in]	inUnicodeCodePoint	The unicode codepoint to be converted.
			@param[in]	inChannel			The caption channel to be used. Defaults to NTV2_CC608_CC1.
			@return		The CEA-608 equivalent string that can be passed into any of the Enqueue____Message functions.
		**/
		static std::string		UnicodeCodePointToCEA608Sequence (const ULWord inUnicodeCodePoint, const NTV2Line21Channel inChannel = NTV2_CC608_CC1);

		/**
			@brief		Returns the length of the given UTF8-encoded string, in characters.
			@param[in]	inUtf8Str	The string containing UTF8-encoded characters to be measured.
			@return		The length of the string, in characters.
			@note		The returned length may not match the number of bytes contained in the string.
		**/
		static size_t			Utf8LengthInChars (const std::string & inUtf8Str);

		/**
			@brief		Returns the character at the given character offset in the given UTF8-encoded string.
			@param[in]	inUtf8Str		The string containing the UTF8-encoded character sequence.
			@param[in]	inCharOffset	The character offset. Note that this is not a byte offset.
										Must be less than the number of characters in the string.
			@return		A string containing the UTF-8 character at the given character position. This will be
						empty if the offset is out of bounds or another error occurs.
		**/
		static std::string		Utf8GetCharacter (const std::string & inUtf8Str, const size_t inCharOffset);

};	//	CUtf8Helper

#endif	// __NTV2_CEA608_ENCODER_
