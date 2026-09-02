/**
	@file		ntv2captionencoder608.cpp
	@brief		Implementation of the CNTV2CaptionEncoder608 class.
	@copyright	(C) 2006-2022 AJA Video Systems, Inc. All rights reserved.
**/


#include "ntv2captionencoder608.h"
#include "ntv2transcode.h"
#include "ntv2utils.h"
#include "ajabase/system/debug.h"
#include <sstream>
#include <iomanip>

using namespace std;

//	A lovely hack to keep the AJACCLIB headers separated from 'ajalibraries/ajabase'...
#define	MyQueueLock		reinterpret_cast <AJALock *> (mpQueueLock)

#define	LOGMYERROR(__xpr__)		AJA_sERROR(AJA_DebugUnit_CC608Encode,	GetLogLabel() << ": " << AJAFUNC << ": " << __xpr__)
#define	LOGMYWARN(__xpr__)		AJA_sWARNING(AJA_DebugUnit_CC608Encode,	GetLogLabel() << ": " << AJAFUNC << ": " << __xpr__)
#define	LOGMYNOTE(__xpr__)		AJA_sNOTICE(AJA_DebugUnit_CC608Encode,	GetLogLabel() << ": " << AJAFUNC << ": " << __xpr__)
#define	LOGMYINFO(__xpr__)		AJA_sINFO(AJA_DebugUnit_CC608Encode,	GetLogLabel() << ": " << AJAFUNC << ": " << __xpr__)
#define	LOGMYDEBUG(__xpr__)		AJA_sDEBUG(AJA_DebugUnit_CC608Encode,	GetLogLabel() << ": " << AJAFUNC << ": " << __xpr__)


/////////////////////////////////////////////////////////////////////////////
// CaptionEncoder608 definition
/////////////////////////////////////////////////////////////////////////////
#if defined (MSWindows)
	#include "ntv2debug.h"
#endif


static uint32_t	gInstanceTally(0);



/////////////////////////////////////////////////////////////////////////////
// Constructor
CNTV2CaptionEncoder608::CNTV2CaptionEncoder608 (void)
	:	mpXmitCurrentF1Msg	(NULL),
		mpXmitCurrentF2Msg	(NULL)
{
	AJAAtomic::Increment(&gInstanceTally);
	ostringstream oss; oss << "CaptionEncoder608-" << gInstanceTally;
	SetLogLabel(oss.str());
	mXmitMsgQueueF1.SetLogLabel(GetLogLabel() + "-MsgQueF1");
	mXmitMsgQueueF2.SetLogLabel(GetLogLabel() + "-MsgQueF2");

}	//	constructor


CNTV2CaptionEncoder608::~CNTV2CaptionEncoder608 ()
{
}	//	destructor


bool CNTV2CaptionEncoder608::Create (CNTV2CaptionEncoder608Ptr & outEncoder)
{
	outEncoder = NULL;

	try
	{
		outEncoder = new CNTV2CaptionEncoder608;
	}
	catch (const std::bad_alloc &)
	{
	}
	return outEncoder;

}	//	Create


bool CNTV2CaptionEncoder608::EncodeNextCaptionBytesIntoLine21 (void *					pVideoFrameBuffer,
																const NTV2PixelFormat	inFrameBufferFormat,
																const NTV2VideoFormat	inVideoFormat,
																const NTV2Line21Field	inFieldNum)
{
	bool result (true);
	if (!pVideoFrameBuffer)
	{
		LOGMYERROR ("Called with NULL frame buffer");
		return false;
	}
	if (!NTV2_IS_SD_VIDEO_FORMAT (inVideoFormat))
	{
		LOGMYERROR ("Called with video format '" << ::NTV2VideoFormatToString(inVideoFormat)
					<< "', expected '" << ::NTV2VideoFormatToString(NTV2_FORMAT_525_5994) << "'");
		return false;
	}
	if (inFrameBufferFormat != NTV2_FBF_8BIT_YCBCR && inFrameBufferFormat != NTV2_FBF_10BIT_YCBCR)
	{
		LOGMYERROR ("Called with FB format '" << ::NTV2FrameBufferFormatToString(inFrameBufferFormat)
					<< "', expected 8/10-bit YUV");
		return false;
	}
	if (inFieldNum != NTV2_CC608_Field1 && inFieldNum != NTV2_CC608_Field2)
	{
		LOGMYERROR ("Called with field '" << ::NTV2Line21FieldToStr(inFieldNum) << "', expected 'F1' or 'F2'");
		return false;
	}

	/**
		(See page 1 of "VideoVerticalIntervals.pdf" document, for layout of SD 525i)

		"pVideoData" points to the start of the host frame buffer, which mirrors the on-device frame buffer.

		For standard NTV2_FORMAT_525_5994 "Broadcast" format, the buffer is 720 horizontal pixels wide x 486 lines tall.

		"bytesPerRow" is the number of bytes in a single row of video in the buffer, which is used to compute a line-by-line
		byte offset into the host buffer.

		The first line -- offset zero -- line 283 -- is line 20 of NTV2_CC608_Field2 (an "analog half line") -- is skipped.
		The 2nd line is line 21 of NTV2_CC608_Field1. This should receive caption F1.
		The 3rd line is line 284 -- line 21 of NTV2_CC608_Field2. This should receive caption F2.

		"pLine21InBuffer" will point to the start of Line 21 in the host buffer -- either field 1's Line 21, or Field 2's.
	**/

	//	Pop any queued captions waiting to go out, and encode them as a line of video...
	const UByte *	pEncodedYUV8Line	(GetNextLine21TransmitCaptions (inFieldNum));
	const ULWord	bytesPerRow			(::CalcRowBytesForFormat (inFrameBufferFormat, ::GetDisplayWidth (inVideoFormat)));
	UByte *			pVideoData			(reinterpret_cast <UByte *> (pVideoFrameBuffer));
	UByte *			pLine21InBuffer		(pVideoData + ((inFieldNum == NTV2_CC608_Field1 ? 1 : 2) * bytesPerRow));

	if (inFrameBufferFormat == NTV2_FBF_8BIT_YCBCR)
		::memcpy (pLine21InBuffer, pEncodedYUV8Line, bytesPerRow);		//	Replace line 21 with the line handed to us by GetNextLine21TransmitCaptions...
	else if (inFrameBufferFormat == NTV2_FBF_10BIT_YCBCR)
		result = ::ConvertLine_2vuy_to_v210 (pEncodedYUV8Line, reinterpret_cast <ULWord *> (pLine21InBuffer), 720);	//	Convert to 10-bit YUV in-place

	return result;

}	//	EncodeNextCaptionBytesIntoLine21



//******************************************************************************
//
//	Methods to manipulate caption message queues
//
//	The basic "unit" is an CNTV2Caption608Message. This object contains the message
//	data, the total length (in bytes) of the message, and the current output index.
//	Since it will typically take many frames to transmit one message (we can only
//	transmit 2 bytes per frame), a given message will be the "current" message for
//	many frames until all of its data has been transmitted (i.e. its index == length).


// GetNextCaptionMessage()
//
//	Pop the next CNTV2608MsgPtr from the designated queue - return null Ptr if none available.

CNTV2608MsgPtr CNTV2CaptionEncoder608::GetNextCaptionMessage (const NTV2Line21Field inFieldNum)
{
	CNTV2608MsgPtr	result;
	switch (inFieldNum)
	{
		case NTV2_CC608_Field1:	result = mXmitMsgQueueF1.GetNextCaptionMessage ();	break;
		case NTV2_CC608_Field2:	result = mXmitMsgQueueF2.GetNextCaptionMessage ();	break;
		default:				AJACC_ASSERT (false && "invalid Line21Field");		return CNTV2608MsgPtr();
	}
	if (result)
		LOGMYNOTE(::NTV2Line21FieldToStr(inFieldNum) << ",  " << result);
	else
		LOGMYDEBUG(::NTV2Line21FieldToStr(inFieldNum) << ",  " << result);
	return result;
}


// EnqueueCaptionMessage()
//
//	Push the given CNTV2608MsgPtr on the queue.
bool CNTV2CaptionEncoder608::EnqueueCaptionMessage (const NTV2Line21Field inFieldNum, CNTV2608MsgPtr pMsg)
{
	LOGMYNOTE(::NTV2Line21FieldToStr(inFieldNum) << ",  " << pMsg);
	switch (inFieldNum)
	{
		case NTV2_CC608_Field1:	return mXmitMsgQueueF1.EnqueueCaptionMessage (pMsg);
		case NTV2_CC608_Field2:	return mXmitMsgQueueF2.EnqueueCaptionMessage (pMsg);
		default:				AJACC_ASSERT (false && "invalid Line21Field");	return false;
	}
}


// Flush()
//
//	Pop and dispose all CNTV2Caption608Message elements on the queue, including in-progress
void CNTV2CaptionEncoder608::Flush (const NTV2Line21Field inFieldNum, const bool inAlsoInProgress)
{
	switch (inFieldNum)
	{
		case NTV2_CC608_Field1:
			mXmitMsgQueueF1.Flush();
			if (inAlsoInProgress)
				mpXmitCurrentF1Msg = NULL;
			break;
		case NTV2_CC608_Field2:
			mXmitMsgQueueF2.Flush();
			if (inAlsoInProgress)
				mpXmitCurrentF2Msg = NULL;
			break;
		default:
			AJACC_ASSERT (false && "invalid Line21Field");
	}
	LOGMYNOTE("Flushed " << ::NTV2Line21FieldToStr(inFieldNum) << " (" << (inAlsoInProgress ? "" : "not ") << "including in-progress)");
}


void CNTV2CaptionEncoder608::FlushChannel (const NTV2Line21Channel inChannel, const bool inAlsoInProgress)
{
	const NTV2Line21Field	fieldNum	(IsField1Line21CaptionChannel(inChannel)  ?  NTV2_CC608_Field1  :  NTV2_CC608_Field2);
	switch (fieldNum)
	{
		case NTV2_CC608_Field1:
			mXmitMsgQueueF1.Flush(inChannel);
			if (inAlsoInProgress  &&  mpXmitCurrentF1Msg  &&  mpXmitCurrentF1Msg->GetChannel() == inChannel)
				mpXmitCurrentF1Msg = NULL;
			break;
		case NTV2_CC608_Field2:
			mXmitMsgQueueF2.Flush(inChannel);
			if (inAlsoInProgress  &&  mpXmitCurrentF2Msg  &&  mpXmitCurrentF2Msg->GetChannel() == inChannel)
				mpXmitCurrentF2Msg = NULL;
			break;
		default:
			AJACC_ASSERT (false && "invalid Line21Field");
	}
	LOGMYNOTE("Flushed " << ::NTV2Line21FieldToStr(fieldNum) << " (" << (inAlsoInProgress ? "" : "not ") << "including in-progress)");
}


// Erase()
//
//	Issues ENM and EOC control codes to erase all captions
bool CNTV2CaptionEncoder608::Erase (const NTV2Line21Channel	inChannel)
{
	NTV2Line21Field	field	(NTV2_CC608_Field_Invalid);
	UWord			EDMCmd	(0);
	CNTV2608MsgPtr	pCapMsg;

	switch (inChannel)
	{
		//	NTV2Line21Channel		Erase Displayed Memory		Field to use
		default:
		case NTV2_CC608_CC1:		EDMCmd = 0x142c;			field = NTV2_CC608_Field1;	break;
		case NTV2_CC608_CC2:		EDMCmd = 0x1c2c;			field = NTV2_CC608_Field1;	break;
		case NTV2_CC608_CC3:		EDMCmd = 0x152c;			field = NTV2_CC608_Field2;	break;
		case NTV2_CC608_CC4:		EDMCmd = 0x1d2c;			field = NTV2_CC608_Field2;	break;
	}

	return CNTV2Caption608Message::Create (pCapMsg, inChannel)
			&& pCapMsg
				&& pCapMsg->Add608Command(EDMCmd)
					&& EnqueueCaptionMessage (field, pCapMsg);
}


size_t CNTV2CaptionEncoder608::GetQueuedMessageCount (const NTV2Line21Field inFieldNum) const
{
	switch (inFieldNum)
	{
		case NTV2_CC608_Field1:	return mXmitMsgQueueF1.GetQueuedMessageCount();	break;
		case NTV2_CC608_Field2:	return mXmitMsgQueueF2.GetQueuedMessageCount();	break;
		default:				AJACC_ASSERT (false && "invalid Line21Field");		break;
	}
	return 0;
}


size_t CNTV2CaptionEncoder608::GetQueuedByteCount (const NTV2Line21Field inFieldNum) const
{
	switch (inFieldNum)
	{
		case NTV2_CC608_Field1:	return mXmitMsgQueueF1.GetQueuedByteCount() + (mpXmitCurrentF1Msg ? mpXmitCurrentF1Msg->GetBytesRemaining() : 0);	break;
		case NTV2_CC608_Field2:	return mXmitMsgQueueF2.GetQueuedByteCount() + (mpXmitCurrentF2Msg ? mpXmitCurrentF2Msg->GetBytesRemaining() : 0);	break;
		default:				AJACC_ASSERT (false && "invalid Line21Field");	break;
	}
	return 0;
}


void CNTV2CaptionEncoder608::GetQueueInfoForChannel (const NTV2Line21Channel inChannel, size_t & outBytesQueued, size_t & outMessagesQueued) const
{
	const NTV2Line21Field	fieldNum	(IsField1Line21CaptionChannel (inChannel) ? NTV2_CC608_Field1 : NTV2_CC608_Field2);
	outBytesQueued = outMessagesQueued = 0;
	switch (fieldNum)
	{
		case NTV2_CC608_Field1:
			outMessagesQueued = mXmitMsgQueueF1.GetQueuedMessageCount (inChannel);
			outBytesQueued = mXmitMsgQueueF1.GetQueuedByteCount (inChannel);
			if (mpXmitCurrentF1Msg && mpXmitCurrentF1Msg->GetChannel () == inChannel)
				outBytesQueued += mpXmitCurrentF1Msg->GetBytesRemaining ();
			break;
		case NTV2_CC608_Field2:
			outMessagesQueued = mXmitMsgQueueF2.GetQueuedMessageCount (inChannel);
			outBytesQueued = mXmitMsgQueueF2.GetQueuedByteCount (inChannel);
			if (mpXmitCurrentF2Msg && mpXmitCurrentF2Msg->GetChannel () == inChannel)
				outBytesQueued += mpXmitCurrentF2Msg->GetBytesRemaining ();
			break;
		default:			AJACC_ASSERT (false && "invalid Line21Field");			break;
	}
}


void CNTV2CaptionEncoder608::GetQueueStatsForChannel (const NTV2Line21Channel inChannel,	size_t & outEnqueueBytes,	size_t & outEnqueueMsgs,
																							size_t & outDequeueBytes,	size_t & outDequeueMsgs) const
{
	const NTV2Line21Field	fieldNum	(IsField1Line21CaptionChannel (inChannel) ? NTV2_CC608_Field1 : NTV2_CC608_Field2);
	outEnqueueBytes = outEnqueueMsgs = outDequeueBytes = outDequeueMsgs = 0;

	outEnqueueBytes	= IsLine21Field1 (fieldNum)	? mXmitMsgQueueF1.GetEnqueueByteTally (inChannel)		: mXmitMsgQueueF2.GetEnqueueByteTally (inChannel);
	outEnqueueMsgs	= IsLine21Field1 (fieldNum)	? mXmitMsgQueueF1.GetEnqueueMessageTally (inChannel)	: mXmitMsgQueueF2.GetEnqueueMessageTally (inChannel);
	outDequeueBytes	= IsLine21Field1 (fieldNum)	? mXmitMsgQueueF1.GetDequeueByteTally (inChannel)		: mXmitMsgQueueF2.GetDequeueByteTally (inChannel);
	outDequeueMsgs	= IsLine21Field1 (fieldNum)	? mXmitMsgQueueF1.GetDequeueMessageTally (inChannel)	: mXmitMsgQueueF2.GetDequeueMessageTally (inChannel);
}


//******************************************************************************
//
//	Public methods to empty a caption message queue and generate Line 21 Waveforms for output
//


// GetNextLine21TransmitCaptions()
//
//	Typically called twice per frame (once per field). If there is a pending message in
//	the queue for the designated field, it pulls the next two bytes out of the message,
//	encodes them into a "Line 21" video waveform and returns a pointer to the waveform buffer.
//	If the two bytes complete the current message, the message is automatically "popped" from
//	the queue.
//
//	Returns pointer to Line 21 video data, or NULL if error.

UByte * CNTV2CaptionEncoder608::GetNextLine21TransmitCaptions (const NTV2Line21Field inFieldNum)
{
	UByte	ch1 (0), ch2 (0);

	GetNextTransmitCaptionBytes (inFieldNum, ch1, ch2);
	//	DEBUG	cerr << "GetNextLine21TransmitCaptions " << ::NTV2Line21FieldToStr (inFieldNum) << " 0x" << UHEX2 (ch1) << " 0x" << UHEX2 (ch2) << " (0x" << UHEX2 (ch1 & 0x7F) << " 0x" << UHEX2 (ch2 & 0x7F) << ")" << endl;

	//	Encode...
	return mLine21Encoder.EncodeLine (ch1, ch2);

}	//	GetNextLine21TransmitCaptions


bool CNTV2CaptionEncoder608::GetNextTransmitCaptionBytes (const NTV2Line21Field inFieldNum, UByte & outChar1, UByte & outChar2)
{
	CNTV2608MsgPtr			pCurrMsg;
	const UByte	NullData	(CC608OddParity(0));

	//	I'm going to transmit SOMETHING, even if it's only nulls!
	outChar1 = 0;
	outChar2 = 0;

	//	Is there a caption message in progress?
	if (inFieldNum == NTV2_CC608_Field2)
	{
		if (!mpXmitCurrentF2Msg)
			mpXmitCurrentF2Msg = GetNextCaptionMessage (inFieldNum);	//	No - see if there is one in the queue waiting to start
		pCurrMsg = mpXmitCurrentF2Msg;
	}
	else
	{
		if (!mpXmitCurrentF1Msg)
			mpXmitCurrentF1Msg = GetNextCaptionMessage (inFieldNum);	//	No - see if there is one in the queue waiting to start
		pCurrMsg = mpXmitCurrentF1Msg;
	}

	if (pCurrMsg)
	{
		//	I now have a caption message to process -- what kind is it?
		if (pCurrMsg->IsData ())
		{
			//	Transmitting data -- get next character(s)...
			outChar1 = pCurrMsg->ReadNext ();
			if (outChar1)
				outChar2 = pCurrMsg->ReadNext ();

			//	DEBUG	cerr	<< "GetNextTransmitCaptionBytes " << ::NTV2Line21FieldToStr (inFieldNum) << " 0x" << UHEX2 (outChar1) << " 0x" << UHEX2 (outChar2)
			//	DEBUG			<< " (0x" << UHEX2 (outChar1 & 0x7F) << " 0x" << UHEX2 (outChar2 & 0x7F) << ")  " << pCurrMsg << endl;
		}
		else if (pCurrMsg->IsDelay ())
			pCurrMsg->SkipNext ();			//	Delay message -- increment frame count and send nulls

		//	Are we finished transmitting this message? If so, dispose it...
		if (pCurrMsg->IsPastEnd ())
		{
			if (inFieldNum == NTV2_CC608_Field2)
				mpXmitCurrentF2Msg = NULL;	//	NOTE:  I'll pick up the next message from my queue the next time this method is called
			else
				mpXmitCurrentF1Msg = NULL;	
		}
	}	//	if pCurrMsg

	//	Add parity...
	outChar1 = CC608OddParity (outChar1);
	outChar2 = CC608OddParity (outChar2);

	return outChar1 != NullData || outChar2 != NullData;	//	Return true if either byte was non-zero

}	//	GetNextTransmitCaptionBytes


bool CNTV2CaptionEncoder608::GetNextCaptionData (CaptionData & outCaptionData)
{
	UByte	c1 (0), c2 (0);

	outCaptionData.bGotField1Data = GetNextTransmitCaptionBytes (NTV2_CC608_Field1, c1, c2);
	outCaptionData.f1_char1 = c1;
	outCaptionData.f1_char2 = c2;

	outCaptionData.bGotField2Data = GetNextTransmitCaptionBytes (NTV2_CC608_Field2, c1, c2);
	outCaptionData.f2_char1 = c1;
	outCaptionData.f2_char2 = c2;

	outCaptionData.bGotField3Data = false;
	outCaptionData.f3_char1 = outCaptionData.f3_char2 = CC608OddParity (0);

	return outCaptionData.bGotField1Data || outCaptionData.bGotField2Data;

}	//	GetNextCaptionData



//******************************************************************************
//
//	Public methods to format and add caption messages to the queue
//

// EnqueuePopOnMessage()
//		Format a Caption Message with the designated string and put in transmit queue.
//	Strings may contain up to four "lines" separated by '\n' characters to denote line breaks.
//	Only the first four lines are utilized -- any additional lines are discarded.
//	Strings may contain two-byte character encodings (e.g, 0x1b 0x33 -- lower-case o with umlaut)
//	but caller is responsible for using the correct byte codes for the given NTV2Line21Channel.
//	Caller is also responsible for immediately preceding the 2-byte sequence with the character's "ascii" equivalent (e.g., 'o').
//
bool CNTV2CaptionEncoder608::EnqueuePopOnMessage (const string &			inMessageStr,
													const NTV2Line21Channel	inChannel,
													const UWord				inRowNumber,
													const UWord				inColumnNumber,
													const NTV2Line21Attrs &	inAttr)
{
	if (!IsLine21CaptionChannel(inChannel))
		return false;	//	Bad caption channel
	if (inMessageStr.empty())
		return false;	//	Empty string

	//	Break original string into separate lines at Carriage Returns...
	UWord	msgLength	(UWord(inMessageStr.length()));
	UWord	charIndex	(0);
	UWord	lineIndex	(0);
	UByte	lineArray[4][NTV2_CC608_MaxCol+1];	//	Pop-on captions can have up to four lines

	for (UWord n (0);  n < 4;  n++)
		lineArray[n][0] = 0;

	for (UWord i(0);  i < msgLength;  i++)
	{
		UByte ch (inMessageStr[i]);
		if (ch != '\n')
		{
			if (ch >= 0x20)
			{
				//	Normal character -- copy one byte to array...
				lineArray [lineIndex] [charIndex++]	= ch;
				lineArray [lineIndex] [charIndex]	= 0;		//	Keep current line null-terminated
			}
			else
			{
				//	Two-byte command -- we need to:
				//	a) make sure we're on an even byte boundary (so both bytes will get sent in the same frame
				//	b) copy both bytes to the array
				//	c) duplicate the pair to act like a redundant command

				//	If we're on an odd character count, pad it to the next even count by adding a NULL
				//	(use x80 to avoid looking like the end of the string)
				if ((charIndex % 2) == 1)
					lineArray [lineIndex][charIndex++] = 0x80;

				//	Copy both command bytes...
				lineArray[lineIndex][charIndex++] = ch;
				if (++i < msgLength)
					lineArray[lineIndex][charIndex++] = inMessageStr[i];

				//	Copy both command bytes again to look like normal command redundancy...
				lineArray[lineIndex][charIndex++] = ch;
				lineArray[lineIndex][charIndex++] = inMessageStr[i];

				//	Keep current line null-terminated...
				lineArray[lineIndex][charIndex] = '\0';
			}
		}	//	not newline
		else
		{
			lineArray[lineIndex][charIndex] = '\0';	//	Null-terminate current line

			//	Get ready for a new line...
			if (lineIndex == 3)
				break;				//	Max out at 4 lines
			else
			{
				lineIndex += 1;		//	Start a new line
				charIndex = 0;
			}
		}	//	else newline
	}	//	for each character in the message string

	UWord numLines = lineIndex + 1;

	//	Handle vertical centering...
	UWord row (inRowNumber);
	if (row == 0)
		row = (NTV2_CC608_MaxRow - numLines) / 2;	//	Auto-center vertically
	//	Clamp row to legal 1 thru 15...
	if (row < NTV2_CC608_MinRow)  row = NTV2_CC608_MinRow;
	if (row > NTV2_CC608_MaxRow)  row = NTV2_CC608_MaxRow;

	//	Alloc a CaptionMessage...
	CNTV2608MsgPtr	pCapMsg;
	CNTV2Caption608Message::Create (pCapMsg, inChannel);
	if (!pCapMsg)
		return false;

	//	Start with a RCL command...
	UWord RCLCmd(0);
	switch (inChannel)
	{
		case NTV2_CC608_CC1:	RCLCmd = 0x1420;	break;
		case NTV2_CC608_CC2:	RCLCmd = 0x1c20;	break;
		case NTV2_CC608_CC3:	RCLCmd = 0x1520;	break;
		case NTV2_CC608_CC4:	RCLCmd = 0x1d20;	break;
		default:									break;
	}

	bool bResult (pCapMsg->Add608Command (RCLCmd));

	//	Iterate for each line in message...
	for (UWord capLine(0);  capLine < numLines;  capLine++)
	{
		UWord strLength (UWord(::strlen(reinterpret_cast<const char*>(lineArray[capLine]))));
		if (!strLength)
			continue;

		//	Calculate starting column...
		UWord startCol (inColumnNumber);
		if (!inColumnNumber)
			startCol = (NTV2_CC608_MaxCol - strLength) / 2;	//	Auto-center
		//	Clamp to legal 1-32...
		if (startCol < NTV2_CC608_MinCol)	startCol = NTV2_CC608_MinCol;
		if (startCol > NTV2_CC608_MaxCol)	startCol = NTV2_CC608_MaxCol;

		//	Indent to given row and closest column...
		if (bResult)
			bResult = InsertPACAndPenModeCommands (inChannel, (row + capLine), startCol, inAttr, pCapMsg);

		//	Add the ASCII data...
		if (bResult)
			bResult = pCapMsg->Add608String(reinterpret_cast<const char*>(lineArray[capLine]));
	}	//	for each caption line

	//	Erase the on-air message...
	UWord EDMCmd(0);
	switch (inChannel)
	{
		case NTV2_CC608_CC1:	EDMCmd = 0x142c;	break;
		case NTV2_CC608_CC2:	EDMCmd = 0x1c2c;	break;
		case NTV2_CC608_CC3:	EDMCmd = 0x152c;	break;
		case NTV2_CC608_CC4:	EDMCmd = 0x1d2c;	break;
		default:									break;
	}
	if (bResult)
		bResult = pCapMsg->Add608Command (EDMCmd);

	//	...and flip the new message to on-air...
	UWord EOCCmd(0);
	switch (inChannel)
	{
		case NTV2_CC608_CC1:	EOCCmd = 0x142f;	break;
		case NTV2_CC608_CC2:	EOCCmd = 0x1c2f;	break;
		case NTV2_CC608_CC3:	EOCCmd = 0x152f;	break;
		case NTV2_CC608_CC4:	EOCCmd = 0x1d2f;	break;
		default:									break;
	}
	if (bResult)
		bResult = pCapMsg->Add608Command (EOCCmd);

	//	Stick the message on the queue, using the correct video field for the given channel...
	if (bResult)
		bResult = EnqueueCaptionMessage (IsField2Line21CaptionChannel(inChannel) ? NTV2_CC608_Field2 : NTV2_CC608_Field1,
										pCapMsg);
	return bResult;

}	//	EnqueuePopOnMessage


// EnqueuePaintOnMessage()
//		Format a Caption Message with the designated string and put in transmit queue
//	Strings may contain up to four "lines" separated by '\n' characters to denote line breaks.
//	Only the first four lines are utilized -- any additional lines are discarded.
//	Strings may contain two-byte character encodings (e.g, 0x1b 0x33 -- lower-case o with umlaut)
//	but caller is responsible for using the correct byte codes for the given NTV2Line21Channel.
//	Caller is also responsible for immediately preceding the 2-byte sequence with the character's "ascii" equivalent (e.g., 'o').
//
bool CNTV2CaptionEncoder608::EnqueuePaintOnMessage (const string &			inMessageStr,
													const bool				inEraseFirst,
													const NTV2Line21Channel	inChannel,
													const UWord				inRowNumber,
													const UWord				inColumnNumber,
													const NTV2Line21Attrs &	inDisplayAttribs)
{
	bool				bResult			(false);
	UWord				row				(inRowNumber);
	UWord				col				(inColumnNumber);
	CNTV2608MsgPtr		pCapMsg;

	if (!IsLine21CaptionChannel(inChannel))
		return false;
	if (inMessageStr.empty())
		return false;


	//	Break original string into separate lines at newlines...
	UWord	msgLength	(UWord(inMessageStr.length()));
	UWord	charIndex	(0);
	UWord	lineIndex	(0);
	UByte	lineArray[4][NTV2_CC608_MaxCol + 1];	//	Paint-on captions can have up to four lines

	for (UWord n(0);  n < 4;  n++)
		lineArray[n][0] = 0;

	for (UWord i(0);  i < msgLength;  i++)
	{
		UByte ch(inMessageStr [i]);
		if (ch != '\n')
		{
			if (ch >= 0x20)
			{
				//	Normal character - copy one byte to array...
				lineArray[lineIndex][charIndex++] = ch;
				lineArray[lineIndex][charIndex] = '\0';	// keep current line null-terminated
			}
			else
			{
				//	Two-byte command -- we need to:
				//	a) make sure we're on an even byte boundary (so both bytes will get sent in the same frame
				//	b) copy both bytes to the array
				//	c) duplicate the pair to act like a redundant command

				//	If we're on an odd character count, pad it to the next even count by adding a NULL
				//	(use x80 to avoid looking like the end of the string)...
				if ((charIndex % 2) == 1)
					lineArray[lineIndex][charIndex++] = 0x80;

				//	Copy both command bytes...
				lineArray[lineIndex][charIndex++] = ch;
				i++;
				if (i < msgLength)
					lineArray[lineIndex][charIndex++] = inMessageStr [i];

				//	Copy both command bytes again to look like normal command redundancy...
				lineArray[lineIndex][charIndex++] = ch;
				lineArray[lineIndex][charIndex++] = inMessageStr [i];

				//	Keep current line null-terminated...
				lineArray[lineIndex][charIndex] = '\0';
			}
		}	//	if not newline
		else
		{
			lineArray[lineIndex][charIndex] = '\0';	//	Null-terminate current line

			//	Get ready for a new line...
			if (lineIndex == 3)
				break;				//	Max out at 4 lines
			else
			{
				lineIndex += 1;		//	Start a new line
				charIndex = 0;
			}
		}	//	else newline
	}	//	for each character in the message string

	UWord numLines = lineIndex + 1;

	//	Handle vertical centering...
	if (row == 0)
		row = (NTV2_CC608_MaxRow - numLines) / 2;

	if (row < NTV2_CC608_MinRow)  row = NTV2_CC608_MinRow;
	if (row > NTV2_CC608_MaxRow)  row = NTV2_CC608_MaxRow;


	//	Alloc a CaptionMessage...
	CNTV2Caption608Message::Create (pCapMsg, inChannel);
	if (!pCapMsg)
		return false;

	//	Optional: erase the on-air display...
	if (inEraseFirst)
	{
		UWord EDMCmd(0x1d2c);
		switch (inChannel)
		{
			case NTV2_CC608_CC1:	EDMCmd = 0x142c;	break;
			case NTV2_CC608_CC2:	EDMCmd = 0x1c2c;	break;
			case NTV2_CC608_CC3:	EDMCmd = 0x152c;	break;
			case NTV2_CC608_CC4:	EDMCmd = 0x1d2c;	break;
			default:									break;
		}
		bResult = pCapMsg->Add608Command (EDMCmd);
	}


	//	Send RDC command...
	UWord RDCCmd(0);
	switch (inChannel)
	{
		case NTV2_CC608_CC1:	RDCCmd = 0x1429;	break;
		case NTV2_CC608_CC2:	RDCCmd = 0x1c29;	break;
		case NTV2_CC608_CC3:	RDCCmd = 0x1529;	break;
		case NTV2_CC608_CC4:	RDCCmd = 0x1d29;	break;
		default:									break;
	}
	bResult = pCapMsg->Add608Command (RDCCmd);


	for (UWord capLine(0);  capLine < numLines;  capLine++)
	{
		const UWord	strLength (UWord(::strlen(reinterpret_cast<const char *>(lineArray[capLine]))));

		if (strLength > 0)
		{
			//	Calculate starting column...
			UWord startCol(col);

			//	Auto-center horizontally...
			if (col == 0)
				startCol = (NTV2_CC608_MaxCol - strLength) / 2;

			//	Sanity check...
			if (startCol < NTV2_CC608_MinCol)	startCol = NTV2_CC608_MinCol;
			if (startCol > NTV2_CC608_MaxCol)	startCol = NTV2_CC608_MaxCol;


			//	Indent to given row and closet column...
			bResult = InsertPACAndPenModeCommands (inChannel, (row + capLine), startCol, inDisplayAttribs, pCapMsg);


			//	Add the ASCII data...
			bResult = pCapMsg->Add608String (reinterpret_cast<const char *>(lineArray[capLine]));
		}
	}	//	for each line in the message string


	//	Stick it on the queue to eventually get transmitted, using the correct video field for the given channel...
	if (bResult)
		bResult = EnqueueCaptionMessage (IsField2Line21CaptionChannel(inChannel) ? NTV2_CC608_Field2 : NTV2_CC608_Field1, pCapMsg);
	
	return bResult;

}	//	EnqueuePaintOnMessage


// EnqueueRollUpMessage()
//		Format a Caption Message with the designated string and put in transmit queue
//		Note: this method assumes one row per string - no multiple-line strings allowed!
//
bool CNTV2CaptionEncoder608::EnqueueRollUpMessage (const string &			inMessageStr,
													const NTV2Line21Mode	inRollMode,
													const NTV2Line21Channel	inChannel,
													const UWord				inRowNumber,
													const UWord				inColumnNumber,
													const NTV2Line21Attrs &	inDisplayAttribs)
{
	UWord				row				(inRowNumber);
	UWord				col				(inColumnNumber);
	bool				bResult			(false);
	CNTV2608MsgPtr		pCapMsg;

	if (!IsLine21CaptionChannel(inChannel))
		return false;

	if (inMessageStr.empty() || inMessageStr.find('\n') != string::npos)
		return false;
	//	Can't preflight byte length, because the number of 2-byte letters is unpredictable...
	//if (inMessageStr.length () > NTV2_CC608_MaxCol)
	//	{cerr << "## DEBUG:  EnqueueRollUpMessage:  '" << inMessageStr << "' " << inMessageStr.length () << " bytes) > " << NTV2_CC608_MaxCol << " chars" << endl; return false;}

	if (!IsLine21RollUpMode (inRollMode))
		return false;

	//	Alloc a CaptionMessage...
	CNTV2Caption608Message::Create (pCapMsg, inChannel);
	if (!pCapMsg)
		return false;

	//	Start with a RU2, RU3, or RU4 command...
	UWord RollUpCmd(0);
	switch (inChannel)
	{						// NTV2_CC608_CapModeRollUp4
		case NTV2_CC608_CC1:	RollUpCmd = 0x1427;		break;
		case NTV2_CC608_CC2:	RollUpCmd = 0x1c27;		break;
		case NTV2_CC608_CC3:	RollUpCmd = 0x1527;		break;
		case NTV2_CC608_CC4:	RollUpCmd = 0x1d27;		break;
		default:										break;
	}
	if (inRollMode == NTV2_CC608_CapModeRollUp2)
		RollUpCmd -= 2;	//	2-Row
	else if (inRollMode == NTV2_CC608_CapModeRollUp3)
		RollUpCmd -= 1;	//	3-Row

	bResult = pCapMsg->Add608Command (RollUpCmd);


	//	Insert a Carriage Return command...
	UWord CRCmd(0);
	switch (inChannel)
	{
		case NTV2_CC608_CC1:	CRCmd = 0x142d;		break;
		case NTV2_CC608_CC2:	CRCmd = 0x1c2d;		break;
		case NTV2_CC608_CC3:	CRCmd = 0x152d;		break;
		case NTV2_CC608_CC4:	CRCmd = 0x1d2d;		break;
		default:									break;
	}
	bResult = pCapMsg->Add608Command (CRCmd);

	//	Indent to given row and closest column...
	bResult = InsertPACAndPenModeCommands (inChannel, row, col, inDisplayAttribs, pCapMsg);

	UWord	msgLength	(UWord(inMessageStr.length()));
	UWord	charIndex	(0);
	UByte	msgStr [4 * NTV2_CC608_MaxCol];	//	Use 4X MaxCol just in case each character is two-byte cmd
	msgStr [0] = 0;

	for (UWord i(0);  i < msgLength;  i++)
	{
		UByte	ch	(inMessageStr[i]);
		if (ch >= 0x20)
		{
			//	Normal character - copy one byte to array...
			msgStr[charIndex++] = ch;
			msgStr[charIndex] = '\0';	// keep null-terminated
		}
		else
		{
			//	Two-byte command -- we need to:
			//	a) make sure we're on an even byte boundary (so both bytes will get sent in the same frame
			//	b) copy both bytes to the array
			//	c) duplicate the pair to act like a redundant command

			//	If we're on an odd character count, pad it to the next even count by adding a NULL
			//	(use x80 to avoid looking like the end of the string)...
			if ((charIndex % 2) == 1)
				msgStr[charIndex++] = 0x80;

			//	Copy both command bytes...
			msgStr[charIndex++] = ch;
			i++;
			if (i < msgLength)
				msgStr[charIndex++] = inMessageStr[i];

			//	Copy both command bytes again to look like normal command redundancy...
			msgStr[charIndex++] = ch;
			msgStr[charIndex++] = inMessageStr[i];
			msgStr[charIndex] = '\0';			//	Keep null-terminated...
		}
	}	//	for each character in the message string

	//	Add the string data...
	bResult = pCapMsg->Add608String (reinterpret_cast<const char *>(msgStr));

	//	Stick the message on the queue using the correct video field for the given channel to start transmission at the next VBI...
	if (bResult)
		bResult = EnqueueCaptionMessage (IsField2Line21CaptionChannel(inChannel) ? NTV2_CC608_Field2 : NTV2_CC608_Field1, pCapMsg);

	return bResult;

}	//	EnqueueRollUpMessage


// EnqueueTextMessage()
//		Format a Text Message with the designated string and put in transmit queue
//		Note: no multi-line strings! ISO 8859-1 (Latin 1) character set only!
bool CNTV2CaptionEncoder608::EnqueueTextMessage (const string &			inMessageStr,
												const bool				inEraseFirst,
												const NTV2Line21Channel	inChannel)
{
	NTV2Line21Attrs	attr;
	CNTV2608MsgPtr	pCapMsg;
	//										CC1		CC2		CC3		CC4		TX1		TX2		TX3		TX4		XDS
	static const UWord	TR_commands[]	= { 0x0000,	0x0000,	0x0000,	0x0000,	0x142a,	0x1c2a,	0x152a,	0x1d2a, 0x0000};
	static const UWord	RTD_commands[]	= { 0x0000,	0x0000,	0x0000,	0x0000,	0x142b,	0x1c2b,	0x152b,	0x1d2b, 0x0000};
	static const UWord	CR_commands[]	= { 0x0000,	0x0000,	0x0000,	0x0000,	0x142d,	0x1c2d,	0x152d,	0x1d2d, 0x0000};

	if (!IsLine21TextChannel(inChannel))
		return false;	//	Bad caption channel
	if (inMessageStr.empty())
		return true;	//	Nothing to enqueue/transmit -- not an error

	for (unsigned ndx(0);  ndx < inMessageStr.length();  ndx++)
		if (UByte(inMessageStr.at(ndx)) < 0x20 || UByte(inMessageStr.at(ndx)) > 0x7E)
		{
			LOGMYERROR ("Message string contains 1 or more non-ISO-8859-1 characters or control codes");
			return false;	//	Fail -- ISO 8859-1 only, and no control codes
		}

	//	Alloc a CaptionMessage thingie...
	if (!CNTV2Caption608Message::Create (pCapMsg, inChannel))
		return false;	//	Fail -- couldn't create caption message

	//	Start with a TR or RTD command...
	if (!pCapMsg->Add608Command (inEraseFirst ? TR_commands[inChannel] : RTD_commands[inChannel]))
		return false;

	if (!inEraseFirst)
	{
		//	Insert a Carriage Return command...
		if (!pCapMsg->Add608Command (CR_commands[inChannel]))
			return false;
	}

	//	Add the ASCII data.
	//	If it's too long to fit in one message, slice it up, and enqueue the remaining slices in separate RTD/CR messages...
	string	messageStr(inMessageStr);
	while (messageStr.length() > size_t (pCapMsg->GetRemainingByteCapacity()))
	{
		const string chunkStr (messageStr.substr (0, pCapMsg->GetRemainingByteCapacity()));
		messageStr.erase (0, pCapMsg->GetRemainingByteCapacity());
		if (!pCapMsg->Add608String (chunkStr))
			return false;	//	Fail -- too big?
		if (!EnqueueCaptionMessage (IsField1Line21CaptionChannel(inChannel) ? NTV2_CC608_Field1 : NTV2_CC608_Field2, pCapMsg))
			return false;	//	Fail -- couldn't enqueue

		//	Start a new RTD caption message...
		if (!CNTV2Caption608Message::Create (pCapMsg, inChannel))
			return false;	//	Fail -- no memory
		if (!pCapMsg->Add608Command (RTD_commands[inChannel]))
			return false;	//	Fail -- couldn't add RTD
	}	//	loop til messageStr fits into pCapMsg
	if (messageStr.empty ())
		return true;	//	Skip Add & enqueue if empty -- not an error

	if (!pCapMsg->Add608String (messageStr))
		return false;

	//	Enqueue the message...
	return EnqueueCaptionMessage (IsField1Line21CaptionChannel(inChannel) ? NTV2_CC608_Field1 : NTV2_CC608_Field2, pCapMsg);

}	//	EnqueueTextMessage


bool CNTV2CaptionEncoder608::EnqueueCaptionData (const CaptionData & inCaptionData)
{
	if (inCaptionData.bGotField1Data)
	{
		CNTV2608MsgPtr	pMsgF1;
		if (!CNTV2Caption608Message::Create (pMsgF1, NTV2_CC608_CC1))
			return false;	//	Fail -- couldn't create caption message
		pMsgF1->AddBytePair (inCaptionData.f1_char1, inCaptionData.f1_char2);
		EnqueueCaptionMessage (NTV2_CC608_Field1, pMsgF1);
	}
	if (inCaptionData.bGotField2Data)
	{
		CNTV2608MsgPtr	pMsgF2;
		if (!CNTV2Caption608Message::Create (pMsgF2, NTV2_CC608_CC3))
			return false;	//	Fail -- couldn't create caption message
		pMsgF2->AddBytePair (inCaptionData.f2_char1, inCaptionData.f2_char2);
		EnqueueCaptionMessage (NTV2_CC608_Field2, pMsgF2);
	}
	return true;

}	//	EnqueueCaptionData


bool CNTV2CaptionEncoder608::EnqueueDelay (const uint32_t inFrameCount, const NTV2Line21Channel inChannel)
{
	const NTV2Line21Field	field(IsField1Line21CaptionChannel(inChannel) ? NTV2_CC608_Field1 : NTV2_CC608_Field2);
	CNTV2608MsgPtr	pMsg;
	uint32_t	frames(inFrameCount);
	while (frames)
	{
		if (!CNTV2Caption608Message::Create(pMsg, inChannel, NTV2_CC608_CaptionMsgType_Delay))
			return false;	//	Fail -- couldn't create caption message
		while (frames  &&  pMsg->AddBytePair(0x00, 0x00))
			--frames;
		if (!EnqueueCaptionMessage(field, pMsg))
			return false;	//	Failed to enqueue message
	}
	return true;

}	//	EnqueueCaptionData


//******************************************************************************
//
//	Private methods to format caption messages
//

// InsertPACAndPenModeCommands()
//		Insert whatever PAC, Tab Offset, Transparent Space and/or Mid Row Codes  
//		to get us to the designated cursor position and pen mode
//
bool CNTV2CaptionEncoder608::InsertPACAndPenModeCommands (const NTV2Line21Channel	inChannel,
															const UWord				inRowNumber,
															const UWord				inColumnNumber,
															const NTV2Line21Attrs &	inAttr,
															CNTV2608MsgPtr			pCapMsg)
{
	NTV2Line21Channel	channel	(inChannel);
	bool				bResult	(true);
	UWord				row		(inRowNumber);
	UWord				col		(inColumnNumber);

	if (inAttr.GetColor() == NTV2_CC608_White  &&  !inAttr.IsItalicized())
	{
		//	"White" or "white underline":
		//	Choose the PAC code that moves to the closest indent point...
		UWord	extraCol	(0);
		UWord	PACCmd		(GetWhitePACCommand (channel, row, col, inAttr, extraCol));
		bResult = pCapMsg->Add608Command (PACCmd);

		//	If that didn't get us to the designated column, add a TabOffset...
		UWord	TOxCmd	(0);
		if (extraCol == 1)
			TOxCmd = (channel == NTV2_CC608_CC1 || channel == NTV2_CC608_CC3) ? 0x1721 : 0x1F21;
		else if (extraCol == 2)
			TOxCmd = (channel == NTV2_CC608_CC1 || channel == NTV2_CC608_CC3) ? 0x1722 : 0x1F22;
		else if (extraCol == 3)
			TOxCmd = (channel == NTV2_CC608_CC1 || channel == NTV2_CC608_CC3) ? 0x1723 : 0x1F23;

		if (TOxCmd)
			bResult = pCapMsg->Add608Command (TOxCmd);
	}	//	if white + non-italic
	else
	{
		//	Non-white (or white + italic) color:
		//	The closest PAC code is to the beginning of a row.
		//	After that, add however many Tab Offsets are needed to arrive at the designated column...
		UWord PACCmd = GetColorPACCommand (channel, row, inAttr);
		bResult = pCapMsg->Add608Command (PACCmd);

		//	Clamp the column...
		if (col < NTV2_CC608_MinCol)
			col = NTV2_CC608_MinCol;
		if (col > NTV2_CC608_MaxCol)
			col = NTV2_CC608_MaxCol;

		//	Insert enough Tab Offset commands to get us to the proper column...
		UWord	xCol	(1);
		while (xCol < col)
		{
			int		diff	(col - xCol);
			UWord	TOxCmd	(0);
			if (diff == 1)
			{
				TOxCmd = (channel == NTV2_CC608_CC1 || channel == NTV2_CC608_CC3) ? 0x1721 : 0x1F21;
				xCol += 1;
			}
			else if (diff == 2)
			{
				TOxCmd = (channel == NTV2_CC608_CC1 || channel == NTV2_CC608_CC3) ? 0x1722 : 0x1F22;
				xCol += 2;
			}
			else	// (diff >= 3)
			{
				TOxCmd = (channel == NTV2_CC608_CC1 || channel == NTV2_CC608_CC3) ? 0x1723 : 0x1F23;
				xCol += 3;
			}

			if (TOxCmd)
				bResult = pCapMsg->Add608Command (TOxCmd);
		}

		//	For colored+italic pen mode, add an "Italic" mid-row command (except for White+Italic, which has its own PAC code).
		//	NOTE:	Mid-row commands get decoded as a "space", so the final result will have an extra space added to the left of the text.
		if (inAttr.IsItalicized()  &&  inAttr.GetColor() != NTV2_CC608_White)
		{
			//	NOTE:	Even though we're passing the "white" Italic or Italic+Underline pen modes to GetMidRowCommand,
			//	technically the mid-row code it returns will change the pen mode WITHOUT changing the pen color.
			UWord	MidRowCmd	(GetMidRowCommand (channel, inAttr));
			if (MidRowCmd)
				bResult = pCapMsg->Add608Command (MidRowCmd);
		}
	}	//	else non-white or white+italic

	return bResult;

}	//	InsertPACAndPenModeCommands



// GetWhitePACCommand()
//		Determine the closest Preamble Address Code to use to get to the given
//		row and column. Also returns count of "extra" columns that the caller
//		will have to account for
//
UWord CNTV2CaptionEncoder608::GetWhitePACCommand (const NTV2Line21Channel	inChannel,
													const UWord				inRowNumber,
													const UWord				inColumnNumber,
													const NTV2Line21Attrs &	inDisplayAttribs,
													UWord &					outExtraColumn)
{
	UWord	row		(inRowNumber);
	UWord	col		(inColumnNumber);
	UWord	result	(0);

	//	Limit row/col to allowable values (don't return errors, just clip)...
	if (row < NTV2_CC608_MinRow)	row = NTV2_CC608_MinRow;
	if (row > NTV2_CC608_MaxRow)	row = NTV2_CC608_MaxRow;
	if (col < NTV2_CC608_MinCol)	col = NTV2_CC608_MinCol;
	if (col > NTV2_CC608_MaxCol)	col = NTV2_CC608_MaxCol;

	
	// get 1st PAC byte and a "base" 2nd byte based on the row
	UWord	byte1	(0),	byte2	(0);

	switch (row)
	{
		case 1:	 byte1 = 0x11;	byte2 = 0x50;	break;
		case 2:	 byte1 = 0x11;	byte2 = 0x70;	break;
		case 3:	 byte1 = 0x12;	byte2 = 0x50;	break;
		case 4:	 byte1 = 0x12;	byte2 = 0x70;	break;
		case 5:	 byte1 = 0x15;	byte2 = 0x50;	break;
		case 6:	 byte1 = 0x15;	byte2 = 0x70;	break;
		case 7:	 byte1 = 0x16;	byte2 = 0x50;	break;
		case 8:	 byte1 = 0x16;	byte2 = 0x70;	break;
		case 9:	 byte1 = 0x17;	byte2 = 0x50;	break;
		case 10: byte1 = 0x17;	byte2 = 0x70;	break;
		case 11: byte1 = 0x10;	byte2 = 0x50;	break;
		case 12: byte1 = 0x13;	byte2 = 0x50;	break;
		case 13: byte1 = 0x13;	byte2 = 0x70;	break;
		case 14: byte1 = 0x14;	byte2 = 0x50;	break;
		case 15: byte1 = 0x14;	byte2 = 0x70;	break;
	}

	// if this is a "second channel" code, add a fixed offset to the first byte...
	if (inChannel == NTV2_CC608_CC2 || inChannel == NTV2_CC608_CC4)
		byte1 += 0x08;

	//	Add an offset to byte 2 based on the column.
	//	CEA-608 "indents" are every 4 columns, with the remainder being our "extra" columns.
	const UWord	indent	((col - 1) / 4);
	const UWord	extra	((col - 1) % 4);

	//	Every other code is a new indent...
	byte2 += static_cast <UWord> (2 * indent);

	//	If this is an "underline" pen mode, add one to the second byte...
	if (inDisplayAttribs.IsUnderlined())
		byte2 += 0x01;

	//	Concatenate the two words...
	result = (byte1 << 8) + byte2;

	outExtraColumn = extra;

	return result;

}	//	GetWhitePACCommand


// GetColorPACCommand()
//		Determine the closest Preamble Address Code to use to get to the beginning
//		of the given row for the designated color.
//
UWord CNTV2CaptionEncoder608::GetColorPACCommand (const NTV2Line21Channel	inChannel,
													const UWord				inRowNumber,
													const NTV2Line21Attrs &	inDisplayAttribs)
{
	UWord	row	(inRowNumber);

	//	Limit row to allowable values (don't return errors, just clip)
	if (row < NTV2_CC608_MinRow)	row = NTV2_CC608_MinRow;
	if (row > NTV2_CC608_MaxRow)	row = NTV2_CC608_MaxRow;

	//	Get 1st PAC byte and a "base" 2nd byte based on the row...
	UWord byte1(0),	byte2(0);

	switch (row)
	{
		case 1:	 byte1 = 0x11;	byte2 = 0x40;	break;
		case 2:	 byte1 = 0x11;	byte2 = 0x60;	break;
		case 3:	 byte1 = 0x12;	byte2 = 0x40;	break;
		case 4:	 byte1 = 0x12;	byte2 = 0x60;	break;
		case 5:	 byte1 = 0x15;	byte2 = 0x40;	break;
		case 6:	 byte1 = 0x15;	byte2 = 0x60;	break;
		case 7:	 byte1 = 0x16;	byte2 = 0x40;	break;
		case 8:	 byte1 = 0x16;	byte2 = 0x60;	break;
		case 9:	 byte1 = 0x17;	byte2 = 0x40;	break;
		case 10: byte1 = 0x17;	byte2 = 0x60;	break;
		case 11: byte1 = 0x10;	byte2 = 0x40;	break;
		case 12: byte1 = 0x13;	byte2 = 0x40;	break;
		case 13: byte1 = 0x13;	byte2 = 0x60;	break;
		case 14: byte1 = 0x14;	byte2 = 0x40;	break;
		case 15: byte1 = 0x14;	byte2 = 0x60;	break;
		default:								break;
	}

	//	If this is a "second channel" code, add a fixed offset to the first byte...
	if (inChannel == NTV2_CC608_CC2 || inChannel == NTV2_CC608_CC4)
		byte1 += 0x08;

	//	Add an offset to the second byte depending on the color...
	switch (inDisplayAttribs.GetColor())
	{
		case NTV2_CC608_White:		byte2 += (inDisplayAttribs.IsItalicized()) ? 0x0e : 0x00;	break;
		case NTV2_CC608_Green:		byte2 += 0x02;												break;
		case NTV2_CC608_Blue:		byte2 += 0x04;												break;
		case NTV2_CC608_Cyan:		byte2 += 0x06;												break;
		case NTV2_CC608_Red:		byte2 += 0x08;												break;
		case NTV2_CC608_Yellow:		byte2 += 0x0a;												break;
		case NTV2_CC608_Magenta:	byte2 += 0x0c;												break;
		default:																				break;
	}

	if (inDisplayAttribs.IsUnderlined())
		byte2 += 0x01;

	//	Concatenate the two words...
	return (byte1 << 8) + byte2;

}	//	GetColorPACCommand


// GetMidRowCommand()
//
//
UWord CNTV2CaptionEncoder608::GetMidRowCommand (const NTV2Line21Channel	inChannel,
												const NTV2Line21Attrs &	inDisplayAttribs)
{
	UWord result(0);

	switch (inDisplayAttribs.GetColor())
	{
		case NTV2_CC608_White:		result = (inDisplayAttribs.IsItalicized()) ? 0x112e : 0x1120;	break;
		case NTV2_CC608_Green:		result = 0x1122;												break;
		case NTV2_CC608_Blue:		result = 0x1124;												break;
		case NTV2_CC608_Cyan:		result = 0x1126;												break;
		case NTV2_CC608_Red:		result = 0x1128;												break;
		case NTV2_CC608_Yellow:		result = 0x112a;												break;
		case NTV2_CC608_Magenta:	result = 0x112c;												break;
		default:																					break;
	}

	if (inDisplayAttribs.IsUnderlined())
		result += 0x0001;

	//	If this is a "second channel" code, add a fixed offset to the first byte...
	if (inChannel == NTV2_CC608_CC2 || inChannel == NTV2_CC608_CC4)
		result += 0x0800;

	return result;

}	//	GetMidRowCommand


// CC608OddParity()
//		Add odd parity to 608 character
//
UByte CNTV2CaptionEncoder608::CC608OddParity (const UByte inCharacter)
{
	UByte	ch		(inCharacter);
	UByte	result	(inCharacter);
	UWord	ones	(0);

	//	Count the number of ones in the first 7 bits...
	for (int bitNum = 0; bitNum < 7; bitNum++)
	{
		if (ch & 0x01)
			ones++;

		ch = ch >> 1;
	}

	//	If there are an even number of ones, add another in bit 7...
	if (ones % 2 == 0)
		result |= 0x80;
	else
		result &= 0x7F;

	return result;

}	//	CC608OddParity


NTV2CaptionLogMask CNTV2CaptionEncoder608::SetLogMask (const NTV2CaptionLogMask inLogMask)
{
	mLine21Encoder.SetLogMask (inLogMask);
	mXmitMsgQueueF1.SetLogMask (inLogMask);
	mXmitMsgQueueF2.SetLogMask (inLogMask);
	if (mpXmitCurrentF1Msg)
		mpXmitCurrentF1Msg->SetLogMask (inLogMask);
	if (mpXmitCurrentF2Msg)
		mpXmitCurrentF2Msg->SetLogMask (inLogMask);
	return CNTV2CaptionLogConfig::SetLogMask (inLogMask);

}	//	SetDebugLevel


#define	IsChannel1Or3(__chl__)		((__chl__) == NTV2_CC608_CC1 || (__chl__) == NTV2_CC608_CC3)
#define	IsChannel2Or4(__chl__)		((__chl__) == NTV2_CC608_CC2 || (__chl__) == NTV2_CC608_CC4)


string CUtf8Helpers::UnicodeCodePointToCEA608Sequence (const ULWord inUnicodeCodePoint, const NTV2Line21Channel inChannel)
{
	string	resultStr;
	UByte	theChar(0), byte1(0), byte2(0);

	switch (inUnicodeCodePoint)
	{//	UNICODE CP			THECHAR				BYTE1												BYTE2					//	DESCRIPTION

		//	NORTH AMERICAN CHARACTER SET		(these are single-byte, not double-byte)
		case 0x000000E1:	theChar = 0x2A;		byte1 = 0x00;										byte2 = 0x00;	break;	//	lower-case a, acute accent
		case 0x000000E9:	theChar = 0x5C;		byte1 = 0x00;										byte2 = 0x00;	break;	//	lower-case e, acute accent
		case 0x000000ED:	theChar = 0x5E;		byte1 = 0x00;										byte2 = 0x00;	break;	//	lower-case i, acute accent
		case 0x000000F3:	theChar = 0x5F;		byte1 = 0x00;										byte2 = 0x00;	break;	//	lower-case o, acute accent
		case 0x000000FA:	theChar = 0x60;		byte1 = 0x00;										byte2 = 0x00;	break;	//	lower-case u, acute accent
		case 0x000000E7:	theChar = 0x7B;		byte1 = 0x00;										byte2 = 0x00;	break;	//	lower-case c with cedilla
		case 0x000000F7:	theChar = 0x7C;		byte1 = 0x00;										byte2 = 0x00;	break;	//	division sign
		case 0x000000D1:	theChar = 0x7D;		byte1 = 0x00;										byte2 = 0x00;	break;	//	upper-case N with tilde
		case 0x000000F1:	theChar = 0x7E;		byte1 = 0x00;										byte2 = 0x00;	break;	//	lower-case n with tilde
		case 0x00002588:	theChar = 0x7F;		byte1 = 0x00;										byte2 = 0x00;	break;	//	full block

		//	Special North American channel 1 (and 3?) and channel 2 (and 4?)
		case 0x000000AE:						byte1 = IsChannel1Or3(inChannel) ? 0x11 : 0x19;		byte2 = 0x30;	break;	//	registered sign
		case 0x000000B0:						byte1 = IsChannel1Or3(inChannel) ? 0x11 : 0x19;		byte2 = 0x31;	break;	//	degree sign
		case 0x000000BD:						byte1 = IsChannel1Or3(inChannel) ? 0x11 : 0x19;		byte2 = 0x32;	break;	//	1/2 symbol
		case 0x000000BF:						byte1 = IsChannel1Or3(inChannel) ? 0x11 : 0x19;		byte2 = 0x33;	break;	//	inverted question mark
		case 0x00002122:						byte1 = IsChannel1Or3(inChannel) ? 0x11 : 0x19;		byte2 = 0x34;	break;	//	trade mark sign
		case 0x000000A2:						byte1 = IsChannel1Or3(inChannel) ? 0x11 : 0x19;		byte2 = 0x35;	break;	//	cents symbol
		case 0x000000A3:						byte1 = IsChannel1Or3(inChannel) ? 0x11 : 0x19;		byte2 = 0x36;	break;	//	pound sterling
		case 0x0000266A:						byte1 = IsChannel1Or3(inChannel) ? 0x11 : 0x19;		byte2 = 0x37;	break;	//	eighth note
		case 0x000000E0:						byte1 = IsChannel1Or3(inChannel) ? 0x11 : 0x19;		byte2 = 0x38;	break;	//	lower-case a, grave accent
		case 0x000000A0:						byte1 = IsChannel1Or3(inChannel) ? 0x11 : 0x19;		byte2 = 0x39;	break;	//	transparent space
		case 0x000000E8:						byte1 = IsChannel1Or3(inChannel) ? 0x11 : 0x19;		byte2 = 0x3A;	break;	//	lower-case e, grave accent
		case 0x000000E2:						byte1 = IsChannel1Or3(inChannel) ? 0x11 : 0x19;		byte2 = 0x3B;	break;	//	lower-case a, circumflex accent
		case 0x000000EA:						byte1 = IsChannel1Or3(inChannel) ? 0x11 : 0x19;		byte2 = 0x3C;	break;	//	lower-case e, circumflex accent
		case 0x000000EE:						byte1 = IsChannel1Or3(inChannel) ? 0x11 : 0x19;		byte2 = 0x3D;	break;	//	lower-case i, circumflex accent
		case 0x000000F4:						byte1 = IsChannel1Or3(inChannel) ? 0x11 : 0x19;		byte2 = 0x3E;	break;	//	lower-case o, circumflex accent
		case 0x000000FB:						byte1 = IsChannel1Or3(inChannel) ? 0x11 : 0x19;		byte2 = 0x3F;	break;	//	lower-case u, circumflex accent

		//	EXTENDED WESTERN EUROPEAN CHARACTER SET

		//	Extended Spanish/Misc, channels 1/3 and 2/4
		case 0x000000C1:	theChar = 'A';		byte1 = IsChannel1Or3(inChannel) ? 0x12 : 0x1A;		byte2 = 0x20;	break;	//	upper-case A, acute accent			'A'
		case 0x000000C9:	theChar = 'E';		byte1 = IsChannel1Or3(inChannel) ? 0x12 : 0x1A;		byte2 = 0x21;	break;	//	upper-case E, acute accent			'E'
		case 0x000000D3:	theChar = 'O';		byte1 = IsChannel1Or3(inChannel) ? 0x12 : 0x1A;		byte2 = 0x22;	break;	//	upper-case O, acute accent			'O'
		case 0x000000DA:	theChar = 'U';		byte1 = IsChannel1Or3(inChannel) ? 0x12 : 0x1A;		byte2 = 0x23;	break;	//	upper-case U, acute accent			'U'
		case 0x000000DC:	theChar = 'U';		byte1 = IsChannel1Or3(inChannel) ? 0x12 : 0x1A;		byte2 = 0x24;	break;	//	upper-case U, umlaut				'U'
		case 0x000000FC:	theChar = 'u';		byte1 = IsChannel1Or3(inChannel) ? 0x12 : 0x1A;		byte2 = 0x25;	break;	//	lower-case u, umlaut				'u'
		case 0x00002018:	theChar = '`';		byte1 = IsChannel1Or3(inChannel) ? 0x12 : 0x1A;		byte2 = 0x26;	break;	//	opening single quote				'`'
		case 0x000000A1:	theChar = '!';		byte1 = IsChannel1Or3(inChannel) ? 0x12 : 0x1A;		byte2 = 0x27;	break;	//	inverted exclamation				'!'
		case 0x0000002A:	theChar = 'o';		byte1 = IsChannel1Or3(inChannel) ? 0x12 : 0x1A;		byte2 = 0x28;	break;	//	asterisk							'*'
		case 0x00002019:	theChar = '\'';		byte1 = IsChannel1Or3(inChannel) ? 0x12 : 0x1A;		byte2 = 0x29;	break;	//	closing single quote				'\''
		case 0x00002014:	theChar = '-';		byte1 = IsChannel1Or3(inChannel) ? 0x12 : 0x1A;		byte2 = 0x2A;	break;	//	em dash								'_'
		case 0x000000A9:	theChar = 'c';		byte1 = IsChannel1Or3(inChannel) ? 0x12 : 0x1A;		byte2 = 0x2B;	break;	//	copyright							'(C)'
		case 0x00002120:	theChar = 'r';		byte1 = IsChannel1Or3(inChannel) ? 0x12 : 0x1A;		byte2 = 0x2C;	break;	//	service mark						'(R)'
		case 0x00002022:	theChar = 'o';		byte1 = IsChannel1Or3(inChannel) ? 0x12 : 0x1A;		byte2 = 0x2D;	break;	//	round bullet						'o'
		case 0x0000201C:	theChar = '"';		byte1 = IsChannel1Or3(inChannel) ? 0x12 : 0x1A;		byte2 = 0x2E;	break;	//	opening double quotes				'"'
		case 0x0000201D:	theChar = '"';		byte1 = IsChannel1Or3(inChannel) ? 0x12 : 0x1A;		byte2 = 0x2F;	break;	//	closing double quotes				'"'

		//	Extended French, channels 1/3 and 2/4
		case 0x000000C0:	theChar = 'A';		byte1 = IsChannel1Or3(inChannel) ? 0x12 : 0x1A;		byte2 = 0x30;	break;	//	upper-case A, grave accent			'A'
		case 0x000000C2:	theChar = 'A';		byte1 = IsChannel1Or3(inChannel) ? 0x12 : 0x1A;		byte2 = 0x31;	break;	//	upper-case A, circumflex accent		'A'
		case 0x000000C7:	theChar = 'C';		byte1 = IsChannel1Or3(inChannel) ? 0x12 : 0x1A;		byte2 = 0x32;	break;	//	upper-case C with cedilla			'C'
		case 0x000000C8:	theChar = 'E';		byte1 = IsChannel1Or3(inChannel) ? 0x12 : 0x1A;		byte2 = 0x33;	break;	//	upper-case E, grave accent			'E'
		case 0x000000CA:	theChar = 'E';		byte1 = IsChannel1Or3(inChannel) ? 0x12 : 0x1A;		byte2 = 0x34;	break;	//	upper-case E, circumflex accent		'E'
		case 0x000000CB:	theChar = 'E';		byte1 = IsChannel1Or3(inChannel) ? 0x12 : 0x1A;		byte2 = 0x35;	break;	//	upper-case E, umlaut				'E'
		case 0x000000EB:	theChar = 'e';		byte1 = IsChannel1Or3(inChannel) ? 0x12 : 0x1A;		byte2 = 0x36;	break;	//	lower-case e, umlaut				'e'
		case 0x000000CE:	theChar = 'I';		byte1 = IsChannel1Or3(inChannel) ? 0x12 : 0x1A;		byte2 = 0x37;	break;	//	upper-case I, circumflex accent		'I'
		case 0x000000CF:	theChar = 'I';		byte1 = IsChannel1Or3(inChannel) ? 0x12 : 0x1A;		byte2 = 0x38;	break;	//	upper-case I, umlaut				'I'
		case 0x000000EF:	theChar = 'i';		byte1 = IsChannel1Or3(inChannel) ? 0x12 : 0x1A;		byte2 = 0x39;	break;	//	lower-case i, umlaut				'i'
		case 0x000000D4:	theChar = 'O';		byte1 = IsChannel1Or3(inChannel) ? 0x12 : 0x1A;		byte2 = 0x3A;	break;	//	upper-case O, circumflex accent		'O'
		case 0x000000D9:	theChar = 'U';		byte1 = IsChannel1Or3(inChannel) ? 0x12 : 0x1A;		byte2 = 0x3B;	break;	//	upper-case U, grave accent			'U'
		case 0x000000F9:	theChar = 'u';		byte1 = IsChannel1Or3(inChannel) ? 0x12 : 0x1A;		byte2 = 0x3C;	break;	//	lower-case u, grave accent			'u'
		case 0x000000DB:	theChar = 'U';		byte1 = IsChannel1Or3(inChannel) ? 0x12 : 0x1A;		byte2 = 0x3D;	break;	//	upper-case U, circumflex accent		'U'
		case 0x000000AB:	theChar = '<';		byte1 = IsChannel1Or3(inChannel) ? 0x12 : 0x1A;		byte2 = 0x3E;	break;	//	opening guillemet					'<'
		case 0x000000BB:	theChar = '>';		byte1 = IsChannel1Or3(inChannel) ? 0x12 : 0x1A;		byte2 = 0x3F;	break;	//	closing guillemet					'>'

		//	Extended Portuguese, channels 1 or 3
		case 0x000000C3:	theChar = 'A';		byte1 = IsChannel1Or3(inChannel) ? 0x13 : 0x1B;		byte2 = 0x20;	break;	//	upper-case A with tilde				'A'
		case 0x000000E3:	theChar = 'a';		byte1 = IsChannel1Or3(inChannel) ? 0x13 : 0x1B;		byte2 = 0x21;	break;	//	lower-case a with tilde				'E'
		case 0x000000CD:	theChar = 'I';		byte1 = IsChannel1Or3(inChannel) ? 0x13 : 0x1B;		byte2 = 0x22;	break;	//	upper-case I, acute accent			'I'
		case 0x000000CC:	theChar = 'I';		byte1 = IsChannel1Or3(inChannel) ? 0x13 : 0x1B;		byte2 = 0x23;	break;	//	upper-case I, grave accent			'I'
		case 0x000000EC:	theChar = 'i';		byte1 = IsChannel1Or3(inChannel) ? 0x13 : 0x1B;		byte2 = 0x24;	break;	//	lower-case i, grave accent			'i'
		case 0x000000D2:	theChar = 'O';		byte1 = IsChannel1Or3(inChannel) ? 0x13 : 0x1B;		byte2 = 0x25;	break;	//	upper-case O, grave accent			'O'
		case 0x000000F2:	theChar = 'o';		byte1 = IsChannel1Or3(inChannel) ? 0x13 : 0x1B;		byte2 = 0x26;	break;	//	lower-case o, grave accent			'o'
		case 0x000000D5:	theChar = 'O';		byte1 = IsChannel1Or3(inChannel) ? 0x13 : 0x1B;		byte2 = 0x27;	break;	//	upper-case O with tilde				'O'
		case 0x000000F5:	theChar = 'o';		byte1 = IsChannel1Or3(inChannel) ? 0x13 : 0x1B;		byte2 = 0x28;	break;	//	lower-case o with tilde				'o'
		case 0x0000007B:	theChar = '{';		byte1 = IsChannel1Or3(inChannel) ? 0x13 : 0x1B;		byte2 = 0x29;	break;	//	opening brace						'{'
		case 0x0000007D:	theChar = '}';		byte1 = IsChannel1Or3(inChannel) ? 0x13 : 0x1B;		byte2 = 0x2A;	break;	//	closing brace						'}'
		case 0x0000005C:	theChar = '\\';		byte1 = IsChannel1Or3(inChannel) ? 0x13 : 0x1B;		byte2 = 0x2B;	break;	//	backslash							'\\'
		case 0x0000005E:	theChar = '^';		byte1 = IsChannel1Or3(inChannel) ? 0x13 : 0x1B;		byte2 = 0x2C;	break;	//	caret								'^'
		case 0x0000005F:	theChar = '_';		byte1 = IsChannel1Or3(inChannel) ? 0x13 : 0x1B;		byte2 = 0x2D;	break;	//	underbar							'_'
		case 0x0000007C:	theChar = '|';		byte1 = IsChannel1Or3(inChannel) ? 0x13 : 0x1B;		byte2 = 0x2E;	break;	//	pipe								'|'
		case 0x0000007E:	theChar = '~';		byte1 = IsChannel1Or3(inChannel) ? 0x13 : 0x1B;		byte2 = 0x2F;	break;	//	tilde								'~'

		//	Extended German/Danish, channels 1 or 3
		case 0x000000C4:	theChar = 'A';		byte1 = IsChannel1Or3(inChannel) ? 0x13 : 0x1B;		byte2 = 0x30;	break;	//	upper-case A, umlaut				'A'
		case 0x000000E4:	theChar = 'a';		byte1 = IsChannel1Or3(inChannel) ? 0x13 : 0x1B;		byte2 = 0x31;	break;	//	lower-case a, umlaut				'a'
		case 0x000000D6:	theChar = 'O';		byte1 = IsChannel1Or3(inChannel) ? 0x13 : 0x1B;		byte2 = 0x32;	break;	//	upper-case O, umlaut				'O'
		case 0x000000F6:	theChar = 'o';		byte1 = IsChannel1Or3(inChannel) ? 0x13 : 0x1B;		byte2 = 0x33;	break;	//	lower-case o, umlaut				'o'
		case 0x000000DF:	theChar = 's';		byte1 = IsChannel1Or3(inChannel) ? 0x13 : 0x1B;		byte2 = 0x34;	break;	//	small sharp s						's'
		case 0x000000A5:	theChar = 'Y';		byte1 = IsChannel1Or3(inChannel) ? 0x13 : 0x1B;		byte2 = 0x35;	break;	//	yen sign							'Y'
		case 0x000000A4:	theChar = '$';		byte1 = IsChannel1Or3(inChannel) ? 0x13 : 0x1B;		byte2 = 0x36;	break;	//	non-specific currency sign			'$'
		//case 0x0000007C:	theChar = '|';		byte1 = IsChannel1Or3(inChannel) ? 0x13 : 0x1B;		byte2 = 0x37;	break;	//	vertical bar						'|'
		case 0x000000C5:	theChar = 'A';		byte1 = IsChannel1Or3(inChannel) ? 0x13 : 0x1B;		byte2 = 0x38;	break;	//	upper-case A with ring				'A'
		case 0x000000E5:	theChar = 'a';		byte1 = IsChannel1Or3(inChannel) ? 0x13 : 0x1B;		byte2 = 0x39;	break;	//	lower-case a with ring				'a'
		case 0x000000D8:	theChar = 'O';		byte1 = IsChannel1Or3(inChannel) ? 0x13 : 0x1B;		byte2 = 0x3A;	break;	//	upper-case O with stroke			'O'
		case 0x000000F8:	theChar = 'o';		byte1 = IsChannel1Or3(inChannel) ? 0x13 : 0x1B;		byte2 = 0x3B;	break;	//	lower-case o with stroke			'o'
		case 0x0000250C:	theChar = 'F';		byte1 = IsChannel1Or3(inChannel) ? 0x13 : 0x1B;		byte2 = 0x3C;	break;	//	upper-left corner					'F'
		case 0x00002510:	theChar = 'T';		byte1 = IsChannel1Or3(inChannel) ? 0x13 : 0x1B;		byte2 = 0x3D;	break;	//	upper-right corner					'T'
		case 0x00002514:	theChar = 'L';		byte1 = IsChannel1Or3(inChannel) ? 0x13 : 0x1B;		byte2 = 0x3E;	break;	//	lower-left corner					'L'
		case 0x00002518:	theChar = 'J';		byte1 = IsChannel1Or3(inChannel) ? 0x13 : 0x1B;		byte2 = 0x3F;	break;	//	lower-right corner					'J'

		default:
			if (inUnicodeCodePoint > 0x0000001F  &&  inUnicodeCodePoint < 0x0000007F)
				theChar = uint8_t(inUnicodeCodePoint & 0x000000FF);	//	Basic printable ASCII
	}
	if (theChar)
	{
		resultStr += string(1, char(theChar));
		if (byte1)
		{
			resultStr += string(1, char(byte1));
			if (byte2)
				resultStr += string(1, char(byte2));
		}
	}
	else if (byte1 || byte2)
	{
		if (byte1)
		{
			resultStr += string(1, char(byte1));
			if (byte2)
				resultStr += string(1, char(byte2));
		}
	}
	return resultStr;

}	//	UnicodeCodePointToCEA608Sequence


string CUtf8Helpers::Utf8ToCEA608String (const string & inUtf8Str, const NTV2Line21Channel inChannel)
{
	UByte			rawBytes [5]		= {0, 0, 0, 0, 0};
	bool			finished			(false);
	unsigned		charPos				(0);
	ULWord			unicodeCodePoint	(0);
	string			resultStr;
	ostringstream	errMsgs;

	while (!finished && charPos < inUtf8Str.length ())
	{
		rawBytes [0] = UByte (inUtf8Str.at (charPos++));
		if (charPos >= inUtf8Str.length ())
			finished = true;
		if (rawBytes [0] < 0x80)
		{
			unicodeCodePoint = ULWord (rawBytes [0]);
			resultStr += UnicodeCodePointToCEA608Sequence (unicodeCodePoint, inChannel);
			//resultStr += string (1, rawBytes [0]);
		}
		else if (rawBytes [0] < 0xC2)
			errMsgs	<< "## WARNING:  Utf8ToCEA608String:  Continuation or overlong 2-byte sequence at character position " << (charPos-1) << endl;
		else if (rawBytes [0] < 0xE0)
		{
			//	2-byte sequence
			if (finished)
				{errMsgs	<< "## WARNING:  Utf8ToCEA608String:  EOF before reading 2nd byte 2-byte UTF8 character" << endl;	break;}
			rawBytes [1] = UByte (inUtf8Str.at (charPos++));
			if (charPos >= inUtf8Str.length ())
				finished = true;
			if ((rawBytes [1] & 0xC0) != 0x80)
				{errMsgs	<< "## WARNING:  Utf8ToCEA608String:  Upper two bits of 2nd byte of 2-byte UTF8 character not 0x80, instead got "
							<< xHEX0N(uint16_t(rawBytes[1] & 0xC0),2) << ", at offset " << (charPos-1) << endl;	continue;}
			unicodeCodePoint = ULWord (rawBytes [0] << 6)  +  ULWord (rawBytes [1])  -  0x3080;
			resultStr += UnicodeCodePointToCEA608Sequence (unicodeCodePoint, inChannel);
		}
		else if (rawBytes [0] < 0xF0)
		{
			//	3-byte sequence
			if (finished)
				{errMsgs	<< "## WARNING:  Utf8ToCEA608String:  EOF before reading 2nd byte of 3-byte UTF8 character" << endl;	break;}
			rawBytes [1] = UByte (inUtf8Str.at (charPos++));
			if (charPos >= inUtf8Str.length ())
				{errMsgs	<< "## WARNING:  Utf8ToCEA608String:  EOF before reading 3rd byte of 3-byte UTF8 character" << endl;	break;}
			if ((rawBytes [1] & 0xC0) != 0x80)
				{errMsgs	<< "## WARNING:  Utf8ToCEA608String:  Upper two bits of 2nd byte of 3-byte UTF8 character not 0x80, instead got "
							<< xHEX0N(uint16_t(rawBytes[1] & 0xC0),2) << ", at offset " << (charPos-1) << endl;	continue;}
			if (rawBytes [0] == 0xE0 && rawBytes [1] < 0xA0)
				{errMsgs	<< "## WARNING:  Utf8ToCEA608String:  Overlong condition at offset " << (charPos-1) << ":  " << xHEX0N(uint16_t(rawBytes[0]),2)
							<< ", " << xHEX0N(uint16_t(rawBytes[1]),2) << endl;	continue;}
			rawBytes [2] = UByte (inUtf8Str.at (charPos++));
			if (charPos >= inUtf8Str.length ())
				finished = true;
			if ((rawBytes [2] & 0xC0) != 0x80)
				{errMsgs	<< "## WARNING:  Utf8ToCEA608String:  Upper two bits of 3rd byte of 3-byte UTF8 character not 0x80, instead got "
							<< xHEX0N(uint16_t(rawBytes[2] & 0xC0),2) << ", at offset " << (charPos-1) << endl;	continue;}
			unicodeCodePoint = ULWord (rawBytes [0] << 12)  +  ULWord (rawBytes [1] << 6)  +  ULWord (rawBytes [2])  -  0xE2080;
			resultStr += UnicodeCodePointToCEA608Sequence (unicodeCodePoint, inChannel);
		}
		else if (rawBytes [0] < 0xF5)
		{
			//	4-byte sequence */
			if (finished)
				{errMsgs	<< "## WARNING:  Utf8ToCEA608String:  EOF before reading 2nd byte of 4-byte UTF8 character" << endl;	break;}
			rawBytes [1] = UByte (inUtf8Str.at (charPos++));
			if (charPos >= inUtf8Str.length ())
				{errMsgs	<< "## WARNING:  Utf8ToCEA608String:  EOF before reading 3rd byte of 4-byte UTF8 character" << endl;	break;}
			if ((rawBytes [1] & 0xC0) != 0x80)
				{errMsgs	<< "## WARNING:  Utf8ToCEA608String:  Upper two bits of 2nd byte of 4-byte UTF8 character not 0x80, instead got "
							<< xHEX0N(uint16_t(rawBytes[1] & 0xC0),2) << ", at offset " << (charPos-1) << endl;	continue;}
			if (rawBytes [0] == 0xF0 && rawBytes [1] < 0x90)
				{errMsgs	<< "## WARNING:  Utf8ToCEA608String:  Overlong condition at offset " << (charPos-1) << ":  " << xHEX0N(uint16_t(rawBytes[0]),2)
							<< ", " << xHEX0N(uint16_t(rawBytes[1]),2) << endl;	continue;}
			if (rawBytes [0] == 0xF4 && rawBytes [1] >= 0x90)
				{errMsgs	<< "## WARNING:  Utf8ToCEA608String:  Unicode codepoint exceeds U+10FFFF at offset " << (charPos-1) << " in 4-byte sequence:  "
							<< xHEX0N(uint16_t(rawBytes[0]),2) << ", " << xHEX0N(uint16_t(rawBytes[1]),2) << endl;	continue;}
			rawBytes [2] = UByte (inUtf8Str.at (charPos++));
			if (charPos >= inUtf8Str.length ())
				{errMsgs << "## WARNING:  Utf8ToCEA608String:  EOF before reading 4th byte of 4-byte UTF8 character" << endl;	break;}
			if ((rawBytes [2] & 0xC0) != 0x80)
				{errMsgs	<< "## WARNING:  Utf8ToCEA608String:  Upper two bits of 3rd byte of 4-byte UTF8 character not 0x80, instead got "
							<< xHEX0N(uint16_t(rawBytes[2] & 0xC0),2) << ", at offset " << (charPos-1) << endl;	continue;}
			rawBytes [3] = UByte (inUtf8Str.at (charPos++));
			if (charPos >= inUtf8Str.length ())
				finished = true;
			if ((rawBytes [3] & 0xC0) != 0x80)
				{errMsgs	<< "## WARNING:  Utf8ToCEA608String:  Upper two bits of 4th byte of 4-byte UTF8 character not 0x80, instead got "
							<< xHEX0N(uint16_t(rawBytes[3] & 0xC0),2) << ", at offset " << (charPos-1) << endl;	continue;}
			unicodeCodePoint = ULWord (rawBytes [0] << 18)  +  ULWord (rawBytes [1] << 12)  +  ULWord (rawBytes [2] << 6)  +  ULWord (rawBytes [3])  -  0x3C82080;
			resultStr += UnicodeCodePointToCEA608Sequence (unicodeCodePoint, inChannel);
		}
		else
			{errMsgs	<< "## WARNING:  Utf8ToCEA608String:  Unicode codepoint exceeds U+10FFFF at offset " << (charPos-1) << ", "
						<< xHEX0N(uint16_t(rawBytes[0]),2) << endl;	continue;}
	}	//	loop til no more characters

	#if defined (_DEBUG)
		if (!errMsgs.str ().empty ())
			cerr << errMsgs.str ();
	#endif	//	defined (_DEBUG)

	return resultStr;

}	//	Utf8ToCEA608String


size_t CUtf8Helpers::Utf8LengthInChars (const std::string & inUtf8Str)
{
	size_t		numChars	(0);
	UByte		rawByte		(0);
	bool		finished	(false);
	unsigned	charPos		(0);
	string		resultStr;

	while (!finished && charPos < inUtf8Str.length ())
	{
		rawByte = UByte (inUtf8Str.at (charPos++));
		if (charPos >= inUtf8Str.length ())
			finished = true;
		if (rawByte < 0x80)
			numChars++;
		else if (rawByte < 0xC2)
			continue;	//	## WARNING:  Continuation or overlong 2-byte sequence at character position (charPos-1)
		else if (rawByte < 0xE0)
		{
			//	2-byte sequence
			if (finished)
				break;	//	## WARNING:  EOF before reading 2nd byte 2-byte UTF8 character
			rawByte = UByte (inUtf8Str.at (charPos++));
			if (charPos >= inUtf8Str.length ())
				finished = true;
			if ((rawByte & 0xC0) != 0x80)
				continue;	//	## WARNING:  Upper two bits of 2nd byte of 2-byte UTF8 character not 0x80
			numChars++;
		}
		else if (rawByte < 0xF0)
		{
			//	3-byte sequence
			if (finished)
				break;	//	## WARNING:  EOF before reading 2nd byte of 3-byte UTF8 character
			rawByte = UByte (inUtf8Str.at (charPos++));
			if (charPos >= inUtf8Str.length ())
				break;	//	## WARNING:  EOF before reading 3rd byte of 3-byte UTF8 character
			if ((rawByte & 0xC0) != 0x80)
				continue;	//	## WARNING:  Upper two bits of 2nd byte of 3-byte UTF8 character not 0x80
			rawByte = UByte (inUtf8Str.at (charPos++));
			if (charPos >= inUtf8Str.length ())
				finished = true;
			if ((rawByte & 0xC0) != 0x80)
				continue;	//	## WARNING:  Upper two bits of 3rd byte of 3-byte UTF8 character not 0x80
			numChars++;
		}
		else if (rawByte < 0xF5)
		{
			//	4-byte sequence */
			if (finished)
				break;	//	## WARNING:  EOF before reading 2nd byte of 4-byte UTF8 character
			rawByte = UByte (inUtf8Str.at (charPos++));
			if (charPos >= inUtf8Str.length ())
				break;	//	## WARNING:  EOF before reading 3rd byte of 4-byte UTF8 character
			if ((rawByte & 0xC0) != 0x80)
				continue;	//	## WARNING:  Utf8ToCEA608String:  Upper two bits of 2nd byte of 4-byte UTF8 character not 0x80
			rawByte = UByte (inUtf8Str.at (charPos++));
			if (charPos >= inUtf8Str.length ())
				break;	//	## WARNING:  EOF before reading 4th byte of 4-byte UTF8 character
			if ((rawByte & 0xC0) != 0x80)
				continue;	//	## WARNING:  Upper two bits of 3rd byte of 4-byte UTF8 character not 0x80
			rawByte = UByte (inUtf8Str.at (charPos++));
			if (charPos >= inUtf8Str.length ())
				finished = true;
			if ((rawByte & 0xC0) != 0x80)
				continue;	//	## WARNING:  Upper two bits of 4th byte of 4-byte UTF8 character not 0x80
			numChars++;
		}
		else
			continue;	//	## WARNING:  Unicode codepoint exceeds U+10FFFF at offset (charPos-1)

	}	//	loop til no more characters

	return numChars;

}	//	Utf8LengthInChars


string CUtf8Helpers::Utf8GetCharacter (const std::string & inUtf8Str, const size_t inCharOffset)
{
	string		resultStr;
	size_t		numChars	(0);
	UByte		rawBytes [5] = {0, 0, 0, 0, 0};
	bool		finished	(false);
	unsigned	charPos		(0);

	while (!finished && charPos < inUtf8Str.length ())
	{
		rawBytes [0] = UByte (inUtf8Str.at (charPos++));
		if (charPos >= inUtf8Str.length ())
			finished = true;
		if (rawBytes [0] < 0x80)
		{
			numChars++;
			resultStr = string (1, char (rawBytes[0]));
		}
		else if (rawBytes [0] < 0xC2)
			continue;	//	## WARNING:  Continuation or overlong 2-byte sequence at character position (charPos-1)
		else if (rawBytes [0] < 0xE0)
		{
			//	2-byte sequence
			if (finished)
				break;	//	## WARNING:  EOF before reading 2nd byte 2-byte UTF8 character
			rawBytes [1] = UByte (inUtf8Str.at (charPos++));
			if (charPos >= inUtf8Str.length ())
				finished = true;
			if ((rawBytes [1] & 0xC0) != 0x80)
				continue;	//	## WARNING:  Upper two bits of 2nd byte of 2-byte UTF8 character not 0x80
			numChars++;
			resultStr = string (1, char (rawBytes[0])) + string (1, char (rawBytes[1]));
		}
		else if (rawBytes [0] < 0xF0)
		{
			//	3-byte sequence
			if (finished)
				break;	//	## WARNING:  EOF before reading 2nd byte of 3-byte UTF8 character
			rawBytes [1] = UByte (inUtf8Str.at (charPos++));
			if (charPos >= inUtf8Str.length ())
				break;	//	## WARNING:  EOF before reading 3rd byte of 3-byte UTF8 character
			if ((rawBytes [1] & 0xC0) != 0x80)
				continue;	//	## WARNING:  Upper two bits of 2nd byte of 3-byte UTF8 character not 0x80
			if (rawBytes [0] == 0xE0 && rawBytes [1] < 0xA0)
				continue;	//	## WARNING:  Overlong condition at offset (charPos-1)
			rawBytes [2] = UByte (inUtf8Str.at (charPos++));
			if (charPos >= inUtf8Str.length ())
				finished = true;
			if ((rawBytes [2] & 0xC0) != 0x80)
				continue;	//	## WARNING:  Upper two bits of 3rd byte of 3-byte UTF8 character not 0x80
			numChars++;
			resultStr = string (1, char (rawBytes[0])) + string (1, char (rawBytes[1])) + string (1, char (rawBytes[2]));
		}
		else if (rawBytes [0] < 0xF5)
		{
			//	4-byte sequence */
			if (finished)
				break;	//	## WARNING:  EOF before reading 2nd byte of 4-byte UTF8 character
			rawBytes [1] = UByte (inUtf8Str.at (charPos++));
			if (charPos >= inUtf8Str.length ())
				break;	//	## WARNING:  EOF before reading 3rd byte of 4-byte UTF8 character
			if ((rawBytes [1] & 0xC0) != 0x80)
				continue;	//	## WARNING:  Upper two bits of 2nd byte of 4-byte UTF8 character not 0x80
			if (rawBytes [0] == 0xF0 && rawBytes [1] < 0x90)
				continue;	//	## WARNING:  Overlong condition at offset (charPos-1)
			if (rawBytes [0] == 0xF4 && rawBytes [1] >= 0x90)
				continue;	//	## WARNING:  Unicode codepoint exceeds U+10FFFF at offset (charPos-1) in 4-byte sequence
			rawBytes [2] = UByte (inUtf8Str.at (charPos++));
			if (charPos >= inUtf8Str.length ())
				break;	//	## WARNING:  EOF before reading 4th byte of 4-byte UTF8 character
			if ((rawBytes [2] & 0xC0) != 0x80)
				continue;	//	## WARNING:  Upper two bits of 3rd byte of 4-byte UTF8 character not 0x80
			rawBytes [3] = UByte (inUtf8Str.at (charPos++));
			if (charPos >= inUtf8Str.length ())
				finished = true;
			if ((rawBytes [3] & 0xC0) != 0x80)
				continue;	//	## WARNING:  Upper two bits of 4th byte of 4-byte UTF8 character not 0x80
			numChars++;
			resultStr = string (1, char (rawBytes[0])) + string (1, char (rawBytes[1])) + string (1, char (rawBytes[2])) + string (1, char (rawBytes[3]));
		}
		else
			continue;	//	## WARNING:  Unicode codepoint exceeds U+10FFFF at offset (charPos-1)

		if (numChars > inCharOffset)
			break;
	}	//	loop til no more characters

	return resultStr;

}	//	Utf8GetCharacter
