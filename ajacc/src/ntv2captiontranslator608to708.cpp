/**
	@file		ntv2captiontranslator608to708.cpp
	@brief		Implementation of the CNTV2CaptionTranslator608to708 class.
	@copyright	(C) 2006-2022 AJA Video Systems, Inc. All rights reserved.
**/


/*
		This module is the top-level module for a device that translates CEA-608 ("Line 21")
	captions into CEA-708 ("DTVCC") captions. It accepts 608 caption data (4 bytes/frame) and
	parses it to the appropriate channel (CC1, CC2, etc.) where the current state and buffer
	status is maintained. The 608 data and commands for each channel are then translated into
	CEA-708 "Service Blocks" which are queued according to channel priorities.

		This module instantiates eight CNTV2CaptionTranslatorChannel608to708 class objects (1 each for
	CC1-CC4 and Text1-Text4), and one CNTV2XDSDecodeChannel608 class object for XDS data. Users should
	not need to access the individual "channel" objects directly - all calls should funnel through this
	top level class. As the top-level object in the decoder, this code only worries about how to route
	incoming data (and user method calls) to the appropriate channel(s), and how to coalesce the output
	data. All other decoding logic is carried in the channel objects. A CNTV2CaptionEncoder708 class
	object is also instantiated to provide aid in encoding the 708 data.

	Implementation Note: This module is VERY similar to CNTV2CaptionDecoder608 in its decoding/parsing
	logic (the Translator doesn't require the burn-in methods). If you make a change here, take a look
	at CNTV2CaptionDecoder608 to see if the same logic shouldn't also be changed there...

		To implement a basic translator, instantiate this object and use the SetDisplayChannel() and
	SetDebugMode() methods to select the debug modes. You should then call Translate608CCDataToSMPTE334Anc()
	with the four bytes of CEA-608 data that arrive every frame (if you are decoding Standard Def video,
	you may be getting the data (2 bytes) each field. The best thing to do is wait until the end of the
	frame and package both fields' data into the one call).

		It is best to call Translate608CCDataToSMPTE334Anc() every frame, even if the 608 captioning data
	for that frame is "null" (zeros). CEA-608 caption rules call for certain commands to be sent twice on
	adjacent frames. This means	that there is a difference between <command> <command> and <command> <null>
	<command>, and if this decoder never "sees" the intervening null frames it may mistakenly think two
	commands come from adjacent	frames and misinterpret them.

		Translate608CCDataToSMPTE334Anc() returns a pointer and length to an array of SMPTE 334 Ancillary
	data. This data is the entire ancillary packet, including the SMPTE Ancillary header and checksum.

 */


#include "ntv2captiontranslator608to708.h"
#include "ccfont.h"

using namespace std;


/////////////////////////////////////////////////////////////////////////////
// CaptionTranslator608to708 definition
/////////////////////////////////////////////////////////////////////////////
#if defined (MSWindows)
	#pragma warning(disable: 4800)
#endif


static unsigned	gInstanceTally	(0);



// Create
//		Factory method.																								//	static
//
bool CNTV2CaptionTranslator608to708::Create (CNTV2CaptionTranslator608to708Ptr & outObj)
{
	outObj = NULL;

	try
	{
		outObj = new CNTV2CaptionTranslator608to708;
	}
	catch (const std::bad_alloc &)
	{
	}
	return outObj;

}	//	Create



/////////////////////////////////////////////////////////////////////////////
// Constructor
//
CNTV2CaptionTranslator608to708::CNTV2CaptionTranslator608to708 (void)
	:	mChannelDecoders (NTV2_CC608_ChannelMax)
{
	bool	badAlloc	(false);

	gInstanceTally++;
	mDisplayChannel		= NTV2_CC608_CC1;
	mCurrXmitChannel[0]	= NTV2_CC608_CC1;
	mCurrXmitChannel[1]	= NTV2_CC608_CC3;

	//	Set up each of the translate channels...
	for (UWord i (NTV2_CC608_CC1);  i <= NTV2_CC608_Text4;  i++)
	{
		CNTV2CaptionTranslatorChannel608to708Ptr	p;
		CNTV2CaptionTranslatorChannel608to708::Create (p);
		mChannelDecoders [i] = p;
		badAlloc = !p;
		if (badAlloc)
			break;

		//	Tell it which 608 channel it is...
		const NTV2Line21Channel	channel	(static_cast <NTV2Line21Channel> (i));
		mChannelDecoders [i]->SetChannel (channel);

		//	Tell it which 708 Service Number to use...
		Set708ServiceNumber (channel, (NTV2_CC708PrimaryCaptionServiceNum + i - NTV2_CC608_CC1));	//	Assume channels 1 - 8
		//Set708ServiceNumber (channel, (NTV2_CC708PrimaryCaptionServiceNum + 1 + i - (int) NTV2_CC608_CC1));	//	DEBUG: start with "Service 2"
																		// This is because Evertz decoders always interpret "Service 1" as embedded 608
		//	Tell it whether or not it's contributing to the 708 output...
		Set708TranslateEnable (channel, true);		//	Assume all channels enabled until told otherwise
	}

	CNTV2XDSDecodeChannel608::Create (mXDSDecoder);
	CNTV2CaptionEncoder708::Create (m708Encoder);

	if (badAlloc || !mXDSDecoder || !m708Encoder)
	{
		std::bad_alloc	exception;
		throw exception;
	}

	mLastControlCode [0] = 0;
	mLastControlCode [1] = 0;

	mEnableEmbedded608Output = true;	//	Assume 608 output is always enabled

}	//	constructor


CNTV2CaptionTranslator608to708::~CNTV2CaptionTranslator608to708 ()
{
}	//	destructor


// Reset()
//
//	This can be called to "flush" the system of any in-progress data.
//	Note: this is NOT guaranteed to be thread-safe.
//
void CNTV2CaptionTranslator608to708::Reset (void)
{
	for (UWord i (0);  i < NTV2_CC608_ChannelMax - 1;  i++)
		mChannelDecoders [i]->Reset ();

	mXDSDecoder->Reset ();
	m708Encoder->Reset ();

	mLastControlCode [0] = 0;
	mLastControlCode [1] = 0;

}	//	Reset


// Set708ServiceNumber()
//		Sets the 708 Service Number that a caption channel (CC1, CC2, etc.) should use for output
//
bool CNTV2CaptionTranslator608to708::Set708ServiceNumber (const NTV2Line21Channel chan, const int serviceNum)
{
	bool	bResult	(true);

	if (chan >= 0 && chan < (NTV2_CC608_ChannelMax - 1))
		mChannelDecoders [chan]->Set708ServiceNumber (serviceNum);
	else
		bResult = false;

	return bResult;

}	//	Set708ServiceNumber


// Get708ServiceNumber()
//		Returns the 708 Service Number that a caption channel (CC1, CC2, etc.) is using
//
int CNTV2CaptionTranslator608to708::Get708ServiceNumber (const NTV2Line21Channel chan) const
{
	if (chan >= 0 && chan < (NTV2_CC608_ChannelMax - 1))
		return mChannelDecoders [chan]->Get708ServiceNumber ();

	return 0;

}	//	Get708ServiceNumber


// Set708TranslateEnable()
//		Enable a translate channel (CC1, CC2, etc.) for 708 output
//		Note: all channels are decoded, but only enabled channels are output to SMPTE 334
//
bool CNTV2CaptionTranslator608to708::Set708TranslateEnable (const NTV2Line21Channel chan, const bool bEnable)
{
	bool	bResult	(true);

	if (chan >= 0 && chan < (NTV2_CC608_ChannelMax - 1))
		mChannelDecoders [chan]->Set708TranslateEnable (bEnable);
	else
		bResult = false;

	return bResult;

}	//	Set708TranslateEnable


// Get708TranslateEnable()
//		Returns a caption channel's (CC1, CC2, etc.) enable status
//		Note: all channels are decoded, but only enabled channels are output to SMPTE 334
//
bool CNTV2CaptionTranslator608to708::Get708TranslateEnable (const NTV2Line21Channel chan) const
{
	if (chan >= 0 && chan < (NTV2_CC608_ChannelMax - 1))
		return mChannelDecoders [chan]->Get708TranslateEnable ();

	return false;

}	//	Get708TranslateEnable



// GetCaptionChannelPacket
//		Returns pointer to Caption Channel Packet buffer and current size (in bytes). Returns 'false' if error.
//		NOTE: we're just returning a pointer to the current local storage - it's up to the
//			  caller to make a copy of this data if they want "double buffering" or plan
//			  to make more changes before using the data.
//
bool CNTV2CaptionTranslator608to708::GetCaptionChannelPacket (UBytePtr & outDataPtr, size_t & outSize)
{
	outDataPtr = m708Encoder->GetCaptionChannelPacket ();
	outSize = m708Encoder->GetCaptionChannelPacketSize ();
	return true;

}	//	GetCaptionChannelPacket



// Translate608CCData()
//		This method should be called once per (input) frame with CEA-608 caption data from one frame.
//		It translates the 608 data into 708 Caption Channel Data, which is held in queues in each of the channels.
//
bool CNTV2CaptionTranslator608to708::Translate608CCData (const CaptionData & inCC608Data)
{
	//	Parse the incoming 608 data and distribute it to the individual 608 channels (CC1, CC2, etc.)...
	return New608FrameData (inCC608Data);

}	//	Translate608CCData



// CreateSMPTE334Anc()
//		This method should be called once per (output) frame. It returns a CEA-708 caption message wrapped in a SMPTE 334 Ancillary Packet.
//
bool CNTV2CaptionTranslator608to708::CreateSMPTE334Anc (const NTV2FrameRate frameRate, const NTV2Line21Field field, UWordPtr & outAncPacketData, size_t & outSize)
{
	//	Bookkeeping:  map captioning channels into "service numbers" and set enables appropriately...
	bool isOkay	(MapChannelServiceNumbers ());

	//	Combine the translated 708 Service Blocks from each of the captioning channels...
	if (isOkay)
		isOkay = Combine708CaptionChannelData (frameRate);

	//	If we successfully translated into 708, build a SMPTE 334 Ancillary packet...
	if (isOkay)
		m708Encoder->MakeSMPTE334AncPacket (frameRate, field, outAncPacketData, outSize);

	return isOkay;

}	//	CreateSMPTE334Anc


// InsertSMPTE334AncPacketInVideoFrame()
//		Passthrough method to the 708 encoder to tell it to insert the SMPTE334 Anc packet it created in MakeSMPTE334AncPacket into the given host frame buffer.
//
bool CNTV2CaptionTranslator608to708::InsertSMPTE334AncPacketInVideoFrame (void * pFrameBuffer, const NTV2VideoFormat inVideoFormat,
																		  const NTV2FrameBufferFormat inPixelFormat, const ULWord inLineNumber) const
{
	return m708Encoder->InsertSMPTE334AncPacketInVideoFrame (pFrameBuffer, inVideoFormat, inPixelFormat, inLineNumber);

}	//	InsertSMPTE334AncPacketInVideoFrame



// New608FrameData()
//		This method is called once per frame with new CEA-608 data. It creates an "equivalent" CEA-708 caption message.
//
bool CNTV2CaptionTranslator608to708::New608FrameData (const CaptionData & inCC608Data)
{
	bool	bResult	(true);

	//	Send 608 data to 708 encoder - this will be used as the "embedded 608" portion of the final output...
	m708Encoder->Set608CaptionData (inCC608Data);

	//	Clear buffer and init size...
	m708Encoder->InitCaptionChannelPacket ();

	//	Parse Field 1...
	if (inCC608Data.bGotField1Data)
		bResult = New608FieldData (inCC608Data.f1_char1, inCC608Data.f1_char2, NTV2_CC608_Field1);

	//	Parse Field 2...
	if (inCC608Data.bGotField2Data)
		bResult = New608FieldData (inCC608Data.f2_char1, inCC608Data.f2_char2, NTV2_CC608_Field2);

	return bResult;

}	//	New608FrameData



// New608FieldData()
//		Usually called twice per video frame with the two bytes for each field of captioning or XDS data
//
bool CNTV2CaptionTranslator608to708::New608FieldData (UByte charP1, UByte charP2, NTV2Line21Field field)
{
	bool	bResult	(true);

	//	Which captioning channels are we talking about here?
	NTV2Line21Channel	currChannel	(GetCaptionChannel (charP1, charP2, field));

	if (currChannel == NTV2_CC608_XDS)
		bResult = ParseXDSData (charP1, charP2, field);
	else
		bResult = ParseCaptionData (charP1, charP2, field, currChannel);

	return bResult;

}	//	New608FieldData



// ParseCaptionData()
//		Usually called twice per video frame with the two bytes for each field of captioning data
//
bool CNTV2CaptionTranslator608to708::ParseCaptionData (UByte charP1, UByte charP2, NTV2Line21Field field, NTV2Line21Channel currChannel)
{
	bool	bResult	(true);
	string	parityStr, str;		//	Check incoming parity

	const bool bParityOK	(Check608Parity (charP1, charP2, parityStr));	//	NOTE:  Doesn't matter which channel we use to check parity

	//	Strip the parity bits...
	UByte char1 = charP1 & 0x7f;
	UByte char2 = charP2 & 0x7f;

	//	Does the new data look like a control code?
	//	Control codes are two-byte commands, and both bytes are guaranteed to arrive in the same field...
	bool	bControlCode		(false);
	bool	bSecondControlCode	(false);
	unsigned short newControlCode	(0);

	if (char1 >= 0x10 && char1 < 0x20)
	{
		//	Yep... if the first byte isn't character data (0x20 - 0x7f), assume we have a command...
		bControlCode = true;

		newControlCode = ((char1 << 8) + char2);

		//	Is this a duplicate control code?
		//	(many caption control codes are sent twice to avoid being missed - we only want to respond to the FIRST one)
		UWord lastControlCode = (field == NTV2_CC608_Field1 ? mLastControlCode [0] : mLastControlCode [1]);

		if (newControlCode == lastControlCode)
			bSecondControlCode = true;
	}

	//	Parse for specific caption commands or character data.
	//	For the most part, we just send the data to the current channel for parsing. However, there are a couple of exceptions...
	NTV2Line21Channel theChannel = currChannel;

	//	Erase XXX Memory (EDM and ENM) commands always are applied to Caption channels (i.e. NOT Text channels).
	//	If the "current" channel is Text, send it to the corresponding Caption channel.
	if ( (char1 == 0x14 || char1 == 0x1c || char1 == 0x15 || char1 == 0x1d) && (char2 == 0x2c || char2 == 0x2e) )
	{
		if (mChannelDecoders [currChannel]->IsTextChannel ())
			theChannel = (NTV2Line21Channel) (currChannel - NTV2_CC608_TextChannelOffset);
	}

	//	Send the data to the designated channel for parsing...
	if (!bSecondControlCode)
		mChannelDecoders [theChannel]->Parse608Data (char1, char2, str);

	//	More special cases: if this is an CR or EOC command, we may want to start a RollUp and/or display the screen (debug)...
	if (!bSecondControlCode)
	{
		//	More special cases: Carriage Return  (0x142d, 0x152d, 0x1c2d, 0x1d2d)
		if ( (char1 == 0x14 || char1 == 0x1c || char1 == 0x15 || char1 == 0x1d) && (char2 == 0x2d) )
		{
			//	Carriage Return: if this is the selected channel, start the RollUp animation process...
			if (currChannel == mDisplayChannel)
			{
				//	Push the entire display DOWN one row and start scroll animation...
				//mRollOffset = NTV2_CCFont_CharDotHeight;
				if (TestLogMask(kCaptionLog_Decode608))
					DebugPrintCurrentScreen ();
			}
		}

		//	More special cases: End of Captions  (0x142f, 0x152f, 0x1c2f, 0x1d2f)...
		if ( (char1 == 0x14 || char1 == 0x1c || char1 == 0x15 || char1 == 0x1d) && (char2 == 0x2f) )
		{
			//	Debug...
			if ((currChannel == mDisplayChannel) && TestLogMask(kCaptionLog_Decode608))
				DebugPrintCurrentScreen();
		}
	}

	//	Save the control code so we can check for duplicates on the next frame.
	//	If this frame's data was NOT a control code, or was already a duplicate control code, reset to "last" to zero...
	UWord lastControlCode = bControlCode ? newControlCode : 0;
	if (bSecondControlCode)
		lastControlCode = 0;

	if (field == NTV2_CC608_Field1)
		mLastControlCode[0] = lastControlCode;
	else
		mLastControlCode[1] = lastControlCode;

	//if (!bParityOK)
	//	AJA_PRINT ("   Field %d [%s]: 0x%02x  0x%02x  - Parity Bad!\n",  (int)field, ::NTV2Line21ChannelToStr (currChannel).c_str (), charP1, charP2);


	if (TestLogMask(kCaptionLog_Decode608))
	{
		const bool	bPACCommand	(char1 >= 0x10 && char1 <= 0x1f && char2 >= 0x40 && char2 <= 0x7f);		//	Print different stuff for PAC commands

		if (!bParityOK)
			Log() << "   Field " << field << " [" << ::NTV2Line21ChannelToStr(currChannel) << "]: 0x" << UHEX2(charP1) << "  0x" << UHEX2(charP2) << "  - Parity Bad!" << endl;
		else if (bPACCommand)
			Log()	<< "Field " << field << " [" << ::NTV2Line21ChannelToStr(currChannel) << "]: 0x" << UHEX2(char1) << "  0x" << UHEX2(char2) << "  " << str << " Row "
							<< mChannelDecoders [currChannel]->GetRow () << " Col " << mChannelDecoders [currChannel]->GetColumn () << " " << ((char2 % 2) ? "(underline)" : "") << endl;
		else if (char1 >= 0x20 && char2 >= 0x20)
			Log() << "Field " << field << " [" << ::NTV2Line21ChannelToStr(currChannel) << "]: '" << char1 << "'  '" << char2 << "'" << endl;
		else if (char1 >= 0x20 && char2 == 0)
			Log() << "Field " << field << " [" << ::NTV2Line21ChannelToStr(currChannel) << "]: '" << char1 << "'  0x" << UHEX2(char2) << endl;
		else if (char1 > 0 || char2 > 0)
			Log() << "Field " << field << " [" << ::NTV2Line21ChannelToStr(currChannel) << "]: 0x" << UHEX2(char1) << "  0x" << UHEX2(char2) << "  " << str << endl;

		if (bSecondControlCode)
			Log() << "   Duplicate 608 command..." << endl;
	}

	return bResult;

}	//	ParseCaptionData


// GetCaptionChannel()
//		Check the new field caption data bytes to see if a new channel has been selected.
//		Returns the current captioning channel for the designated field.
//
NTV2Line21Channel CNTV2CaptionTranslator608to708::GetCaptionChannel (UByte charP1, UByte charP2, NTV2Line21Field field)
{
	//	Strip the parity bits...
	UByte char1 = charP1 & 0x7f;
	UByte char2 = charP2 & 0x7f;

	//	Get the (previous) current channel for this field...
	NTV2Line21Channel prevChannel = (field == NTV2_CC608_Field1 ? mCurrXmitChannel[0] : mCurrXmitChannel[1]);

	//	Ask the current channel if it thinks it should remain the current channel for this field,
	//	or whether the new data is designating a new channel...
	NTV2Line21Channel currChannel = NTV2_CC608_CC1;
	if (prevChannel == NTV2_CC608_XDS)
		currChannel = mXDSDecoder->GetCurrentChannel (char1, char2, field);
	else
		currChannel = mChannelDecoders [prevChannel]->GetCurrentChannel (char1, char2, field);

	//	Save and return the new (or unchanged) channel...
	if (field == NTV2_CC608_Field1)
		mCurrXmitChannel [0] = currChannel;
	else
		mCurrXmitChannel [1] = currChannel;

	return currChannel;

}	//	GetCaptionChannel


// ParseXDSData()
//		Usually called once per video frame with the two bytes for each frame of XDS data
//
bool CNTV2CaptionTranslator608to708::ParseXDSData (UByte charP1, UByte charP2, NTV2Line21Field field)
{
	bool bResult = true;

	//	Strip the parity bits...
	UByte char1 = charP1 & 0x7f;
	UByte char2 = charP2 & 0x7f;

	bResult = mXDSDecoder->NewData (char1, char2, field);

	return bResult;

}	//	ParseXDSData


// Combine708CaptionChannelData()
//		Gather the translated 708 captioning data from each of the channels - in priority order -
//		into a single Captioning Data Packet.
//
bool CNTV2CaptionTranslator608to708::Combine708CaptionChannelData (const NTV2FrameRate frameRate)
{
	bool bResult = true;

	//	We're going to build the Captioning Data Packet in our m708Encoder...
	m708Encoder->InitCaptionChannelPacket();

	//	Get a pointer to the encoder's caption data buffer...
	UByte *	pEncodeData	(m708Encoder->GetCaptionChannelPacket ());
	if (pEncodeData == NULL)
		return false;

	//	Skip the Caption Channel Packet header for now (we'll come back and insert it
	//	after we know the final packet size)...
	size_t	index		(1);
	size_t	packetSize	(0);

	//	How much data we can collect depends on the frame rate...
	size_t	maxIndex = MaxCaptionChannelDataForFrameRate (frameRate) - 1;	//	Be sure to leave room for at least one Null Service Block

	//	Walk through the following channels and add any Service Blocks they may have queued up.
	//	We'll stop when we either: a) run out of room in this CDP; an/or b) have no more 708 data to add.
	//
	//	Note:	The order in which we process the channels determines their relative priorities:
	//  		i.e. we'll take all of CC1's data first, then if there's enough room we'll take
	//			CC3's, CC2's, etc. (Max size for Caption Channel Packets is 128 bytes, including header)
	if (!AddChannelServiceBlockData (NTV2_CC608_CC1,   pEncodeData, index, maxIndex, index))
		goto bail;

	if (!AddChannelServiceBlockData (NTV2_CC608_CC3,   pEncodeData, index, maxIndex, index))
		goto bail;

	if (!AddChannelServiceBlockData (NTV2_CC608_CC2,   pEncodeData, index, maxIndex, index))
		goto bail;

	if (!AddChannelServiceBlockData (NTV2_CC608_CC4,   pEncodeData, index, maxIndex, index))
		goto bail;

	if (!AddChannelServiceBlockData (NTV2_CC608_Text1, pEncodeData, index, maxIndex, index))
		goto bail;

	if (!AddChannelServiceBlockData (NTV2_CC608_Text3, pEncodeData, index, maxIndex, index))
		goto bail;

	if (!AddChannelServiceBlockData (NTV2_CC608_Text2, pEncodeData, index, maxIndex, index))
		goto bail;

	if (!AddChannelServiceBlockData (NTV2_CC608_Text4, pEncodeData, index, maxIndex, index))
		goto bail;

	//	Did we get any data?
	if (index > 1)
	{
		//	Insert a Null Service Block to terminate this packet...
		if (!m708Encoder->MakeNullServiceBlockHeader (index, index))
			goto bail;

		//	Caption Channel Packets need to contain an even number of bytes.
		//	If we have an odd number, insert another Null Service Block to round it up...
		if (index % 2)
		{
			if (!m708Encoder->MakeNullServiceBlockHeader (index, index))
				goto bail;
		}

		//	Now that we know the size (the final "index" value is the total packet size),
		//	go back and insert the Caption Channel Packet header at the beginning of the packet...
		packetSize = index;
		size_t	tmpNdx	(0);
		if (!m708Encoder->MakeCaptionChannelPacketHeader (0, packetSize - 1, tmpNdx))
			goto bail;
	}
	else
		packetSize = 0;		// no 708 captioning data

	//	If we made it this far we must be OK...
	m708Encoder->SetCaptionChannelPacketSize (packetSize);
	bResult = true;

bail:
	return bResult;

}	//	Combine708CaptionChannelData


// MaxCaptionChannelDataForFrameRate
//		Returns the max number of 708 Caption Channel Packet Bytes available for a given NTV2FrameRate.
//		This is generally the number of 708 "triplets" per frame times 2 (two data bytes per triplet).

size_t CNTV2CaptionTranslator608to708::MaxCaptionChannelDataForFrameRate (NTV2FrameRate ntv2Rate)
{
	size_t result = 0;

	switch (ntv2Rate)
	{
		case NTV2_FRAMERATE_2398:
		case NTV2_FRAMERATE_2400:	result = 44;	break;

		case NTV2_FRAMERATE_2500:	result = 48;	break;

		case NTV2_FRAMERATE_2997:
		case NTV2_FRAMERATE_3000:	result = 36;	break;

		case NTV2_FRAMERATE_5000:	result = 24;	break;

		case NTV2_FRAMERATE_5994:
		case NTV2_FRAMERATE_6000:	result = 18;	break;

		default: result = 0;	// that's all that CEA-708B defines - so the rest are an error...?
				 break;
	}

	return result;

}	//	MaxCaptionChannelDataForFrameRate



// AddChannelServiceBlockData()
//		If enabled, add as much of the 708 Service Block data (if any) from the designated channel to the Caption Channel buffer as will fit
//
bool CNTV2CaptionTranslator608to708::AddChannelServiceBlockData (NTV2Line21Channel channel, UByte *pEncodeData, size_t index, size_t maxIndex, size_t & outEndIndex)
{
	bool bResult = true;

	//	Sanity checks...
	if (index >= maxIndex)
		return true;	//	Not an error - just filled up

	if (index >= NTV2_CC708_MaxCaptionChannelPacketSize)
		return false;

	if (channel < 0 || channel >= (NTV2_CC608_ChannelMax - 1) )
		return false;

	//	Is this channel enabled for translation? If not, just silently return (it's NOT an error)...
	if (mChannelDecoders [channel]->Get708TranslateEnable () == false)
		return true;

	int		serviceNum		(0);
	bool	bExtended		(false);
	size_t	svcDataSize		(0);
	size_t	svcBlockSize	(0);

	//	See if the designated channel has any new 708 commands to transmit (each command, if present, is packaged in a 708 "Service Block")...
	bool bMoreSvcBlocks = mChannelDecoders [channel]->GetNextServiceBlockInfoFromQueue (svcBlockSize, svcDataSize, serviceNum, bExtended);

	//	If there is a command (Service Block), and it will fit within the remaining space in the Caption Channel Packet, add it...
	if (bMoreSvcBlocks && ((index + svcBlockSize) < NTV2_CC708_MaxCaptionChannelPacketSize) && ((index + svcBlockSize) <= maxIndex) )
	{
		//	We have (at least) one Service Block and it will fit...
		size_t	svcBlockStart			(index);
		size_t	totalSvcBlockDataSize	(0);	//	Measure payload size only (don't include the header)

		//	Skip over the concatenated Service Block header until we know how much data we're going to be adding...
		size_t	svcBlockHdrSize	(ServiceBlockHeaderSize (serviceNum));
		index += svcBlockHdrSize;

		//	NOTE:	The channels package their 708 commands into separate Service Blocks, each with a Service Block header and data.
		//	However, since (we assume) that all Service Blocks coming from a specific channel are targeted at the same 708 "Service
		//	Number", we can concatenate multiple Service Blocks into one combined Service Block.
		//	If we change and allow channels to target multiple Service Numbers, then this code will have to change to avoid
		//	concatenating Service Blocks targeted to different Services...
		while (bMoreSvcBlocks)
		{
			//	Make sure the new data fits: both into the remaining Caption Channel Packet space AND within a single Service Block...
			if ( ((index + svcBlockSize) <= maxIndex)
				&& ((index + svcDataSize) < NTV2_CC708_MaxCaptionChannelPacketSize) && ((totalSvcBlockDataSize + svcDataSize) <= NTV2_CC708_MaxServiceBlockSize) )
			{
				int	tmpServiceNum	(0);
				//AJA_PRINT ("AddChannelServiceBlockData(): index = %d, svcDataSize = %d, totalSvcBlockDataSize = %d\n", index, svcDataSize, totalSvcBlockDataSize);

				//	Copy the data (ONLY) from the channel's next Service Block...
				mChannelDecoders [channel]->GetNextServiceBlockDataFromQueue (&pEncodeData [index]);
				index += svcDataSize;
				totalSvcBlockDataSize += svcDataSize;

				//	Any more Service Blocks (commands) from this channel...?
				bMoreSvcBlocks = mChannelDecoders [channel]->GetNextServiceBlockInfoFromQueue (svcBlockSize, svcDataSize, tmpServiceNum, bExtended);
			}
			else
			{
				//	Caption Channel Packet (or concatenated Service Block) is full then stop.
				//	If there are any more Service Blocks in the channel we will pick them up next time...
				bMoreSvcBlocks = false;
				//AJA_PRINT ("AddChannelServiceBlockData(): END!  index = %d, svcDataSize = %d, totalSvcBlockDataSize = %d\n", index, svcDataSize, totalSvcBlockDataSize);
			}
		}

		//	Now go back and insert the original service block header,
		//	now that we know how big the Service Block is, insert the header at the beginning...
		bResult = m708Encoder->MakeServiceBlockHeader (svcBlockStart, serviceNum, totalSvcBlockDataSize);
	}

	outEndIndex = index;

	return bResult;

}	//	AddChannelServiceBlockData



// MapChannelServiceNumbers()
//		608 data is transmitted in "channels" (CC1, CC2, Text1, etc.). 708 data is transmitted by "Service Numbers".
//	This method figures out which 608 channels are enabled for translation, gets their 708 Service Numbers, and
//	notifies the 708 Encoder that these Service Numbers are enabled (all other Service Numbers are disabled).
//	This process is necessary because:
//		A)	there are more 708 "services" defined than there are 608 channels, and...
//		B)	the mapping (theoretically) could be dynamic.
//
bool CNTV2CaptionTranslator608to708::MapChannelServiceNumbers (void)
{
	int	svc	(0);

	//	Start by disabling ALL services...
	for (svc = 0; svc < NTV2_CC708MaxNumServices; svc++)
		m708Encoder->SetServiceInfoActive (svc, false);

	//	"Service 0" is the embedded 608 data in the 708 stream...
	m708Encoder->SetServiceInfoActive (0, mEnableEmbedded608Output);

	//	Now walk through the channels.
	//	If they are enabled, find their corresponding Service Numbers, and tell the 708 Encoder that the service is enabled...
	for (UWord chan (0);  chan < (NTV2_CC608_ChannelMax - 1);  chan++)
	{
		if (mChannelDecoders [chan]->Get708TranslateEnable ())
		{
			svc = mChannelDecoders [chan]->Get708ServiceNumber ();
			if (svc > 0)
				m708Encoder->SetServiceInfoActive (svc, true);
		}
	}

	return true;

}	//	MapChannelServiceNumbers



//------------ Debug -----------------

// Set608TestIDMode()
//	For test/ID purposes: flip the case of 608 characters only
//
void CNTV2CaptionTranslator608to708::Set608TestIDMode (bool bTest)
{
	m708Encoder->Set608TestIDMode (bTest);

}	//	Set608TestIDMode


// SetDisplayChannel()
//		For debug displays only: sets the current Line 21 Captioning mode (CC1, CC2, Text1, etc).
//		This tells the decoder software which captioning channel to display in debug messages.
//
bool CNTV2CaptionTranslator608to708::SetDisplayChannel (const NTV2Line21Channel chan)
{
	bool	bResult	(true);

	if (chan >= 0 && chan < (NTV2_CC608_ChannelMax - 1))
		mDisplayChannel = chan;
	else
		bResult = false;

	return bResult;

}	//	SetDisplayChannel


NTV2CaptionLogMask CNTV2CaptionTranslator608to708::SetLogMask (const NTV2CaptionLogMask inLogMask)
{
	mXDSDecoder->SetLogMask (inLogMask);
	m708Encoder->SetLogMask (inLogMask);
	for (UWord chan (0);  chan < NTV2_CC608_ChannelMax - 1;  chan++)
		mChannelDecoders [chan]->SetLogMask (inLogMask);
	return CNTV2CaptionLogConfig::SetLogMask (inLogMask);

}


// DebugPrintCurrentScreen()
//		A poor man's "printf" of the current on-air screen to the Console
//
void CNTV2CaptionTranslator608to708::DebugPrintCurrentScreen (void) const
{
	UByte	line[33];
	line[32] = 0;

	cerr << " -------------------------------- " << endl;

	for (UWord row (NTV2_CC608_MinRow);  row <= NTV2_CC608_MaxRow;  row++)
	{
		//	Convert row of characters to string...
		for (UWord col (NTV2_CC608_MinCol);  col <= NTV2_CC608_MaxCol;  col++)
		{
			UByte					ch	(' ');
			NTV2Line21Attributes	attr;
			if (mDisplayChannel >= NTV2_CC608_CC1 && mDisplayChannel <= NTV2_CC608_Text4)
				ch = mChannelDecoders [mDisplayChannel]->GetOnAirCharacter (row, col, attr);
			if (ch == 0)
				line [col - 1] = ' ';	//	Convert unfilled characters to spaces
			else if (ch >= 0x80 && ch < 0x90)
			{
				//	Handle two-byte characters
				//	The captioning two-byte characters don't have ASCII equivalents, so substitute something close...
				if (NTV2CCFont::GetInstance().GetNoUnderlineSpaceCharacterCode () + 0x20)
					ch = ' ';
				else if (NTV2CCFont::GetInstance().GetUnderlineCharacterCode())
					ch = '_';	//	We should never see this in the on-air display...
				else switch (ch)
				{
					case 0x80:	ch = 'R';	break;		// "Registered" mark
					case 0x81:	ch = 'o';	break;		// Degree sign
					case 0x82:	ch = '5';	break;		// 1/2
					case 0x83:	ch = '?';	break;		// Inverse query
					case 0x84:	ch = 'T';	break;		// "TradeMark"
					case 0x85:	ch = 'c';	break;		// Cents sign
					case 0x86:	ch = 'L';	break;		// Pounds Stirling
					case 0x87:	ch = 'b';	break;		// Musical note
					case 0x88:	ch = 'a';	break;		// Lower-case a with grave accent
					case 0x89:	ch = ' ';	break;		// Transparent space (shouldn't make it to buffer...)
					case 0x8a:	ch = 'e';	break;		// Lower-case e with grave accent
					case 0x8b:	ch = 'a';	break;		// Lower-case a with circumflex
					case 0x8c:	ch = 'e';	break;		// Lower-case e with circumflex
					case 0x8d:	ch = 'i';	break;		// Lower-case i with circumflex
					case 0x8e:	ch = 'o';	break;		// Lower-case o with circumflex
					case 0x8f:	ch = 'u';	break;		// Lower-case u with circumflex
				}
			}
			else
			{
				//	Ordinary ASCII chars...
				line [col - 1] = ch;
			}
		}
		cerr << "|" << line << "|" << endl;
	}

	cerr << " -------------------------------- " << endl;

}	//	DebugPrintCurrentScreen


CNTV2CaptionTranslator608to708::CNTV2CaptionTranslator608to708 (const CNTV2CaptionTranslator608to708 & inTranslatorToCopy)
	:	CNTV2CaptionLogConfig()
{
	(void) inTranslatorToCopy;
	gInstanceTally++;
	AJACC_ASSERT (false && "Hidden copy constructor");

}	//	copy constructor


CNTV2CaptionTranslator608to708 & CNTV2CaptionTranslator608to708::operator = (const CNTV2CaptionTranslator608to708 & inTranslatorToCopy)
{
	(void) inTranslatorToCopy;
	if (&inTranslatorToCopy != this)
		AJACC_ASSERT (false && "hidden assignment operator");
	return *this;

}	//	assignment operator
