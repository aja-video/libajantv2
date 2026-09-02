/**
	@file		ntv2caption608message.h
	@brief		Declares the CNTV2Caption608Message class.
	@copyright	(C) 2006-2022 AJA Video Systems, Inc. All rights reserved.
**/

#ifndef __NTV2_CAPTION608MESSAGE__
#define __NTV2_CAPTION608MESSAGE__

#include "ntv2captionlogging.h"
#include "ntv2caption608types.h"
#include "ajabase/common/ajarefptr.h"
#include <string>
#include <iostream>


typedef enum NTV2_CC608_CaptionMessageType
{
	NTV2_CC608_CaptionMsgType_Data	= 0,	// message contains data to be transmitted
	NTV2_CC608_CaptionMsgType_Delay	= 1		// message contains frame count

} NTV2_CC608_CaptionMessageType;


/**
	@brief	I am a caption message that can accommodate up to 256 bytes of caption message data.
			I have a suite of "Reader" methods for reading my data bytes one-at-a-time.
			I also have a two "Writer" methods -- CNTV2Caption608Message::Add608Command
			and CNTV2Caption608Message::Add608String -- for adding data to me.
	@note	This class is not intended for external AJACCLib client use.
**/
class CNTV2Caption608Message;
typedef AJARefPtr<CNTV2Caption608Message>	CNTV2Caption608MessagePtr;
typedef CNTV2Caption608MessagePtr	CNTV2608MsgPtr;

class AJAExport CNTV2Caption608Message : public CNTV2CaptionLogConfig
{
	friend class CNTV2CaptionEncoder608;		///	For internal use by NTV2CaptionEncoder608 only
	friend class CNTV2Caption608MessageQueue;

	//	Class Data
	protected:
		static const UWord NTV2_CC608_CaptionMsgMaxBytes = 256;		///	The maximum number of bytes we can handle in one "message"


	//	Class Methods
	protected:
		static bool										Create (CNTV2Caption608MessagePtr &			outNewInstance,
																const NTV2Line21Channel				inChannel,
																const NTV2_CC608_CaptionMessageType	inType		= NTV2_CC608_CaptionMsgType_Data);


	//	Instance Methods
	protected:
		//	Construction & Destruction
		explicit										CNTV2Caption608Message (const NTV2Line21Channel				inChannel,
																				const NTV2_CC608_CaptionMessageType	inType	= NTV2_CC608_CaptionMsgType_Data);
		public: virtual inline							~CNTV2Caption608Message ()				{}

	public:
		/**
			@brief		Prints a human-readable representation of me to the given output stream.
			@param[in]	inOutStream		Specifies the output stream to receive my human-readable representation.
			@return		The output stream that received my human-readable representation.
		**/
		virtual std::ostream &							Print (std::ostream & inOutStream) const;

	protected:
		//	Inquiry
		virtual inline NTV2_CC608_CaptionMessageType	GetType (void) const					{return mType;}
		virtual inline NTV2Line21Channel				GetChannel (void) const					{return mChannel;}
		virtual inline bool								HasData (void) const					{return GetLength () > 0;}
		virtual inline bool								IsData (void) const						{return GetType () == NTV2_CC608_CaptionMsgType_Data;}
		virtual inline bool								IsDelay (void) const					{return GetType () == NTV2_CC608_CaptionMsgType_Delay;}
		virtual inline bool								IsLowPriority (void) const				{return !IsLine21CaptionChannel (GetChannel ());}
		virtual inline bool								IsHighPriority (void) const				{return IsLine21CaptionChannel (GetChannel ());}
		virtual inline UWord							GetLength (void) const					{return mLength;}
		virtual inline UWord							GetBytesRemaining (void) const			{return IsPastEnd () ? 0 : GetLength () - GetReadPosition ();}
		virtual inline UWord							GetRemainingByteCapacity (void) const	{return NTV2_CC608_CaptionMsgMaxBytes - GetLength ();}


		//	Readers

		/**
			@brief		Returns the next data byte from my buffer.
						If my read position is past the end, I return zero.
		**/
		virtual UByte									ReadNext (void);


		/**
			@brief		Increments my internal read position (if not already past end).
		**/
		virtual inline void								SkipNext (void)						{if (mReadPosition < mLength) mReadPosition++;}


		/**
			@brief		Returns my internal read position.
		**/
		virtual inline UWord							GetReadPosition (void) const		{return mReadPosition;}


		/**
			@brief		Returns "true" if my internal read position is past the end;  otherwise returns "false".
		**/
		virtual inline bool								IsPastEnd (void) const				{return GetReadPosition () >= GetLength ();}


		//	Writers

		/**
			@brief		Adds the given CEA-608 command to my internal buffer.
			@param[in]	inCommand	Specifies the CEA-608 command to be added to my internal buffer.
			@return		True if successful;  otherwise False.
		**/
		virtual bool									Add608Command (const UWord inCommand);

		/**
			@brief		Adds the contents of the given message string to my internal buffer.
			@param[in]	inMessageStr	Specifies the message character data to be added to my internal buffer.
			@return		True if successful;  otherwise False.
			@note		If the entire message string won't fit into my buffer, none of its characters will be added.
		**/
		virtual bool									Add608String (const std::string & inMessageStr);

		/**
			@brief		Adds the given pair of bytes into my internal buffer.
			@param[in]	inByte1		Specifies the first byte to add to my internal buffer.
			@param[in]	inByte2		Specifies the second byte to add to my internal buffer.
			@return		True if successful;  otherwise False.
		**/
		virtual bool									AddBytePair (const UByte inByte1, const UByte inByte2);


	private:
		virtual CNTV2Caption608Message &				operator = (const CNTV2Caption608Message & inRHS);


	//	Instance Data
	private:
		NTV2_CC608_CaptionMessageType	mType;									///< @brief	Type of message
		UWord							mLength;								///< @brief	Total message length, in bytes (or total delay, in frames)
		UWord							mReadPosition;							///< @brief	Index to next available byte (or current delay count)
		UByte							mData [NTV2_CC608_CaptionMsgMaxBytes];	///< @brief	Up to 256 bytes of message data
		const NTV2Line21Channel			mChannel;								///< @brief	Target channel for the message

};	//	CNTV2Caption608Message


//	Output stream operators:
inline std::ostream & operator << (std::ostream & inOutStream, const CNTV2Caption608Message & inMsg)	{return inMsg.Print (inOutStream);}
std::ostream & operator << (std::ostream & inOutStream, CNTV2Caption608MessagePtr inMsgPtr);


//	Backward compatibility
typedef CNTV2Caption608Message		NTV2CaptionMessage;
typedef CNTV2Caption608MessagePtr	NTV2CaptionMessagePtr;


#endif	//	__NTV2_CAPTION608MESSAGE__
