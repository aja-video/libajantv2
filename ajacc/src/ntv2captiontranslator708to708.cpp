/**
	@file		ntv2captiontranslator708to708.cpp
	@brief		Implementation of the CNTV2CaptionTranslator708to708 class.
	@copyright	(C) 2007-2022 AJA Video Systems, Inc. All rights reserved.
**/


/*
	This module is the top-level module for a device that translates/transforms CEA-708
	("DTVCC") captions from one frame rate to another.
*/

#include "ntv2captiontranslator708to708.h"
#include "ccfont.h"

using namespace std;



/////////////////////////////////////////////////////////////////////////////
// CaptionTranslator708to708 definition
/////////////////////////////////////////////////////////////////////////////
#if defined (MSWindows)
	#pragma warning(disable: 4800)
#endif


static unsigned	gInstanceTally	(0);


bool CNTV2CaptionTranslator708to708::Create (CNTV2CaptionTranslator708to708Ptr & outTranslator)
{
	outTranslator = NULL;

	try
	{
		outTranslator = new CNTV2CaptionTranslator708to708;
	}
	catch (const std::bad_alloc &)
	{
	}
	return outTranslator;

}	//	Create



/////////////////////////////////////////////////////////////////////////////
// Constructor
//
CNTV2CaptionTranslator708to708::CNTV2CaptionTranslator708to708 (void)
{
	gInstanceTally++;
	if (!CNTV2CaptionDecoder708::Create(m708Decoder) || !CNTV2CaptionEncoder708::Create(m708Encoder))
	{
		std::bad_alloc	exception;
		throw exception;
	}

	Reset();

}	//	constructor


CNTV2CaptionTranslator708to708::~CNTV2CaptionTranslator708to708 ()
{
}	//	destructor


//------------------------------------------------------------------------------------

// Reset()
//
//	This can be called to "flush" the system of any in-progress data.
//	Note: this is NOT guaranteed to be thread-safe.
//
void CNTV2CaptionTranslator708to708::Reset (void)
{
	m708Decoder->Reset ();
	m708Encoder->Reset ();

}	//	Reset


// GrabInputSmpte334AndParse()
//		Call this as many times as you think you might get new SMPTE-334 packets (e.g. every field or every frame).
//	This method will input the Anc packet, then parse it down to elementary commands and place them into queues.
//	Call the complementary method, AssembleAandOutputSmpte334() to pull data from the queues, reassemble a SMPTE-334
//	packet, and output.
//
bool CNTV2CaptionTranslator708to708::GrabInputSmpte334AndParse (const UByte * pVideo, const NTV2VideoFormat inVideoFormat, const NTV2FrameBufferFormat inPixelFormat, bool & outHasParityErrors)
{
	bool	bGotAnc	(false);

	if (pVideo)
	{
		//	Get the SMPTE-334 Anc data from an in-host-memory frame buffer...
		bGotAnc = m708Decoder->FindSMPTE334AncPacketInVideoFrame (pVideo, inVideoFormat, inPixelFormat, outHasParityErrors);
	}

	if (bGotAnc)
		m708Decoder->ParseSMPTE334AncPacket (outHasParityErrors);		//	Break it down into elementary commands, etc...

	return bGotAnc;

}	//	GrabInputSmpte334AndParse


// CreateSMPTE334Anc()
//		This method should be called once per (output) frame. It returns a CEA-708 caption message wrapped in a SMPTE 334 Ancillary Packet.
//
bool CNTV2CaptionTranslator708to708::CreateSMPTE334Anc (NTV2FrameRate outputFrameRate, NTV2Line21Field field, UWordPtr & outAncPacketData, size_t & outSize)
{
	return m708Encoder->MakeSMPTE334AncPacket (outputFrameRate, field, outAncPacketData, outSize);

}	//	CreateSMPTE334Anc


bool CNTV2CaptionTranslator708to708::CreateSMPTE334Anc (NTV2FrameRate inOutputFrameRate, NTV2Line21Field inField)
{
	UWordPtr	pAncPacketData	(NULL);
	size_t		ancPacketSize	(0);

	return m708Encoder->MakeSMPTE334AncPacket (inOutputFrameRate, inField, pAncPacketData, ancPacketSize);

}	//	CreateSMPTE334Anc


// OutputSMPTE334Anc()
//
bool CNTV2CaptionTranslator708to708::OutputSMPTE334Anc (void * pFrameBuffer, const NTV2VideoFormat inVideoFormat, const NTV2FrameBufferFormat inPixelFormat, const ULWord inLineNumber)
{
	return m708Encoder->InsertSMPTE334AncPacketInVideoFrame (pFrameBuffer, inVideoFormat, inPixelFormat, inLineNumber);

}	//	OutputSMPTE334Anc


// CopyDecoderDataToEncoder()
//
bool CNTV2CaptionTranslator708to708::CopyDecoderDataToEncoder(NTV2FrameRate outputFrameRate, NTV2Line21Field field)
{
	bool	bResult	(true);

	//	Copy current timecode data...?

	//	Copy current ServiceInfo database...
	m708Encoder->CopyAllServiceInfo (m708Decoder->GetAllServiceInfoPtr ());

	//	Copy 608 data to encoder...
	const CaptionData	captionData	(m708Decoder->GetCC608CaptionData ());
	if (field == NTV2_CC608_Field1)
		m708Encoder->Set608CaptionData (NTV2_CC608_Field1, captionData.f1_char1, captionData.f1_char2, captionData.bGotField1Data);
	else if (field == NTV2_CC608_Field2)
		m708Encoder->Set608CaptionData (NTV2_CC608_Field2, captionData.f2_char1, captionData.f2_char2, captionData.bGotField2Data);
	else	// both fields
		m708Encoder->Set608CaptionData (captionData);

	//	Combine and copy the decoder 708 Service Block commands to the encoder CCP...
	Combine708CaptionServiceData (outputFrameRate);

	return bResult;

}	//	CopyDecoderDataToEncoder


// Combine708CaptionServiceData()
//		Gather the translated 708 captioning data from each of the decoder services into a single Captioning Data Packet.
//
bool CNTV2CaptionTranslator708to708::Combine708CaptionServiceData (NTV2FrameRate frameRate)
{
	bool	bResult	(true);

	//	We're going to build the Captioning Data Packet in our output m708Encoder
	m708Encoder->InitCaptionChannelPacket();

	//	Get a pointer to the encoder's caption data buffer
	UByte *	pEncodeData	(m708Encoder->GetCaptionChannelPacket ());
	if (pEncodeData == NULL)
		return false;

	//	Skip the Caption Channel Packet header for now (we'll come back and insert it
	//	after we know the final packet size)
	size_t	index		(1);
	size_t	packetSize	(0);

	//	How much data we can collect depends on the frame rate
	size_t maxIndex = MaxCaptionChannelDataForFrameRate(frameRate) - 1;	// be sure to leave room for at least one Null Service Block

	//	Walk through the following channels and add any Service Blocks they may have queued up.
	//	We'll stop when we either: a) run out of room in this CDP; an/or b) have no more 708 data to add.
	//
	//	Note:	The order in which we process the Services determines their relative priorities:
	//			i.e. we'll take all of Service 1's data first, then if there's enough room we'll take
	//			Service 2, Service 3, etc. (Max size for Caption Channel Packets is 128 bytes, including header).
	//			For translation purposes, we're assuming tha the aggregate data rate of the input and output
	//			match, so over the long run (several frames) we won't have any issues with queue overflow.
	//
	for (size_t svcIndex = 1; svcIndex < NTV2_CC708MaxNumServices; svcIndex++)
	{
		if (!AddServiceServiceBlockData (svcIndex, pEncodeData, index, maxIndex, index))
			goto bail;
	}

	//	Did we get any data?
	if (index > 1)
	{
		//	Insert a Null Service Block to terminate this packet
		if (!m708Encoder->MakeNullServiceBlockHeader (index, index))
			goto bail;

		//	Caption Channel Packets need to contain an even number of bytes - if we have
		//	an odd number insert another Null Service Block to round it up
		if (index % 2)
		{
			if (!m708Encoder->MakeNullServiceBlockHeader(index, index))
				goto bail;
		}

		//	Now that we know the size (the final "index" value is the total packet size),
		//	go back and insert the Caption Channel Packet header at the beginning of the packet.
		packetSize = index;
		size_t	tmpNdx	(0);
		if (!m708Encoder->MakeCaptionChannelPacketHeader (0, packetSize - 1, tmpNdx))
			goto bail;
	}
	else
		packetSize = 0;		// no 708 captioning data

	//	If we made it this far we must be OK
	m708Encoder->SetCaptionChannelPacketSize (packetSize);
	bResult = true;

bail:
	return bResult;

}	//	Combine708CaptionServiceData


// MaxCaptionChannelDataForFrameRate
//		Returns the max number of 708 Caption Channel Packet Bytes available for a given NTV2FrameRate.
//		This is generally the number of 708 "triplets" per frame times 2 (two data bytes per triplet).

unsigned CNTV2CaptionTranslator708to708::MaxCaptionChannelDataForFrameRate(NTV2FrameRate ntv2Rate)
{
	unsigned result	(0);

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

		default:					result = 0;		break;	// that's all that CEA-708B defines - so the rest are an error...?
	}

	return result;
}


// AddServiceServiceBlockData()
//		If enabled, add as much of the 708 Service Block data (if any) from the designated 708 Service to the Caption Channel buffer as will fit
//
bool CNTV2CaptionTranslator708to708::AddServiceServiceBlockData (const size_t svcIndex, UByte * pEncodeData, size_t index, const size_t maxIndex, size_t & outEndIndex)
{
	bool	bResult	(true);

	//	Sanity checks...
	if (index >= maxIndex)
		return true;			// not an error - just filled up

	if (index >= NTV2_CC708_MaxCaptionChannelPacketSize)
		return false;

	int		serviceNum		(0);
	bool	bExtended		(false);
	size_t	svcDataSize		(0);
	size_t	svcBlockSize	(0);

	//	See if the designated channel has any new 708 commands to transmit (each command, if present, is packaged in a 708 "Service Block")...
	bool bMoreSvcBlocks = m708Decoder->GetNextServiceBlockInfoFromQueue (svcIndex, svcBlockSize, svcDataSize, serviceNum, bExtended);

	//	If there is a command (Service Block), and it will fit within the remaining space in the Caption Channel Packet, add it...
	if (bMoreSvcBlocks && ((index + svcBlockSize) < NTV2_CC708_MaxCaptionChannelPacketSize) && ((index + svcBlockSize) <= maxIndex) )
	{
		//	We have (at least) one Service Block and it will fit...
		const size_t	svcBlockStart			(index);
		size_t			totalSvcBlockDataSize	(0);		//	Measure payload size only (don't include the header)

		//	Skip over the concatenated Service Block header until we know how much data we're going to be adding...
		size_t svcBlockHdrSize = ServiceBlockHeaderSize(serviceNum);
		index += svcBlockHdrSize;

		//	Note:	The 708 Decoder Services package their 708 commands into separate Service Blocks, each with a Service Block header and data.
		//			However, since (we assume) that all Service Blocks coming from a specific service are targeted at the same 708 "Service
		//			Number", we can concatenate multiple Service Blocks into one combined Service Block.
		while (bMoreSvcBlocks)
		{
			//	Make sure the new data fits: both into the remaining Caption Channel Packet space AND within a single Service Block...
			if ( ((index + svcBlockSize) <= maxIndex) && ((index + svcDataSize) < NTV2_CC708_MaxCaptionChannelPacketSize) && ((totalSvcBlockDataSize + svcDataSize) <= NTV2_CC708_MaxServiceBlockSize) )
			{
				//	AJA_PRINT ("AddChannelServiceBlockData(): index = %d, svcDataSize = %d, totalSvcBlockDataSize = %d\n", index, svcDataSize, totalSvcBlockDataSize);

				//	Copy the data (ONLY) from the channel's next Service Block...
				m708Decoder->GetNextServiceBlockDataFromQueue (svcIndex, &pEncodeData [index]);
				index += svcDataSize;
				totalSvcBlockDataSize += svcDataSize;

				//	Any more Service Blocks (commands) from this channel...?
				bMoreSvcBlocks = m708Decoder->GetNextServiceBlockInfoFromQueue (svcIndex, svcBlockSize, svcDataSize, serviceNum, bExtended);
			}
			else
			{
				//	Caption Channel Packet (or concatenated Service Block) is full then stop - if there are any more
				//	Service Blocks in the channel we will pick them up next time
				bMoreSvcBlocks = false;
				//	AJA_PRINT ("AddChannelServiceBlockData(): END!  index = %d, svcDataSize = %d, totalSvcBlockDataSize = %d\n", index, svcDataSize, totalSvcBlockDataSize);
			}
		}	//	while bMoreSvcBlocks

		//	Now go back and insert the original service block header.
		//	Now that we know how big the Service Block is, insert the header at the beginning...
		bResult = m708Encoder->MakeServiceBlockHeader (svcBlockStart, serviceNum, totalSvcBlockDataSize);
	}

	outEndIndex = index;

	return bResult;

}	//	AddServiceServiceBlockData


// ------------ Debug -------------


// Set608TestIDMode()
//	For test/ID purposes: flip the case of 608 characters only
//
void CNTV2CaptionTranslator708to708::Set608TestIDMode (bool bTest)
{
	m708Encoder->Set608TestIDMode (bTest);

}	//	Set608TestIDMode


NTV2CaptionLogMask CNTV2CaptionTranslator708to708::SetLogMask (const NTV2CaptionLogMask inLogMask)
{
	m708Decoder->SetLogMask (inLogMask);
	m708Encoder->SetLogMask (inLogMask);
	return CNTV2CaptionLogConfig::SetLogMask (inLogMask);
}


CNTV2CaptionTranslator708to708::CNTV2CaptionTranslator708to708 (const CNTV2CaptionTranslator708to708 & inTranslatorToCopy)
	:	CNTV2CaptionLogConfig ()
{
	(void) inTranslatorToCopy;
	gInstanceTally++;
	AJACC_ASSERT (false);

}	//	copy constructor


CNTV2CaptionTranslator708to708 & CNTV2CaptionTranslator708to708::operator = (const CNTV2CaptionTranslator708to708 & inTranslatorToCopy)
{
	(void) inTranslatorToCopy;
	AJACC_ASSERT (false);
	return *this;

}	//	assignment operator
