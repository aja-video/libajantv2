/**
	@file		ntv2captiondecoder708.cpp
	@brief		Implementation of the CNTV2CaptionDecoder708 class.
	@copyright	(C) 2006-2022 AJA Video Systems, Inc. All rights reserved.
**/


#include "ntv2captiondecoder708.h"
#include "ntv2smpteancdata.h"
#include "ajabase/system/debug.h"
#include <sstream>
//#include <string.h>	//	for memset

using namespace std;


/////////////////////////////////////////////////////////////////////////////
// CaptionEncoder708 definition
/////////////////////////////////////////////////////////////////////////////

#define	LOGMYERROR(__xpr__)		AJA_sREPORT(AJA_DebugUnit_CC708Decode, AJA_DebugSeverity_Error,		GetLogLabel() << ":  " << AJAFUNC << ":  " << __xpr__)
#define	LOGMYWARN(__xpr__)		AJA_sREPORT(AJA_DebugUnit_CC708Decode, AJA_DebugSeverity_Warning,	GetLogLabel() << ":  " << AJAFUNC << ":  " << __xpr__)
#define	LOGMYNOTE(__xpr__)		AJA_sREPORT(AJA_DebugUnit_CC708Decode, AJA_DebugSeverity_Notice,	GetLogLabel() << ":  " << AJAFUNC << ":  " << __xpr__)
#define	LOGMYINFO(__xpr__)		AJA_sREPORT(AJA_DebugUnit_CC708Decode, AJA_DebugSeverity_Info,		GetLogLabel() << ":  " << AJAFUNC << ":  " << __xpr__)
#define	LOGMYDEBUG(__xpr__)		AJA_sREPORT(AJA_DebugUnit_CC708Decode, AJA_DebugSeverity_Debug,		GetLogLabel() << ":  " << AJAFUNC << ":  " << __xpr__)
#define	YESNO(__x__)			((__x__)?"Y":"N")
#define HX		":\t" << xHEX0N(UWord(mRawVancDataBuffer[index]),2) << " " << xHEX0N(UWord(mRawVancDataBuffer[index+1]),2) << " " << xHEX0N(UWord(mRawVancDataBuffer[index+2]),2) << " ..."

static uint32_t	gInstanceTally	(0);


bool CNTV2CaptionDecoder708::Create (CNTV2CaptionDecoder708Ptr & outDecoder)
{
	outDecoder = NULL;

	try
	{
		outDecoder = new CNTV2CaptionDecoder708;
	}
	catch (const std::bad_alloc &)
	{
	}
	return outDecoder;

}	//	Create



/////////////////////////////////////////////////////////////////////////////
// Constructor
CNTV2CaptionDecoder708::CNTV2CaptionDecoder708 (void)
{
	AJAAtomic::Increment(&gInstanceTally);
	ostringstream	oss;	oss << "CaptionDecoder708-" << gInstanceTally;
	SetLogLabel(oss.str());
	mRawVancByteCount			= 0;
	mCC708FrameDataByteCount	= 0;
	mPreviousSequenceCount		= 0x8000;	//	Pick a number that is unlikely to match (+1) the first frame's sequence count

	mCC608QueueField1.SetField(NTV2_CC608_Field1);
	mCC608QueueField2.SetField(NTV2_CC608_Field2);
	Reset();

}	//	constructor


CNTV2CaptionDecoder708::~CNTV2CaptionDecoder708 ()
{
}	//	destructor



// Reset()
//
//	This can be called to "flush" the system of any in-progress data.
//	Note: this is NOT guaranteed to be thread-safe.
//
void CNTV2CaptionDecoder708::Reset (void)
{
	mCC608QueueField1.Flush();
	mCC608QueueField2.Flush();

	for (unsigned ndx(0);  ndx < unsigned(NTV2_CC708MaxNumServices);  ndx++)
		mAvailableServices [ndx].Reset();

	ClearAllCaptionChannelPacketInfo();
	mParsedCDPData.Zero();
	mCurrentServiceInfo.InitAllServiceInfo();
	mNewServiceInfo.InitAllServiceInfo();
	UpdateServiceInfo();
	LOGMYNOTE("Decoder reset");

}	//	Reset



// SetDisplayChannel()
//		Sets the current Line 21 Captioning mode (CC1, CC2, Text1, etc). This tells the decoder
//		software which captioning channel to display.
bool CNTV2CaptionDecoder708::SetDisplayChannel (const NTV2Line21Channel inChan)
{
	if (!IsValidLine21Channel(inChan))
		return false;
	mLine21DisplayChannel = inChan;
	return true;
}



// Note: there are two ways to get new SMPTE-334 Anc data into the system, and three corresponding methods to handle it:
//
//	1) If you already have the Anc data, call SetSMPTE334AncData() with a pointer and size.
//	2) If you have a video frame (with VANC area) in memory and you want it parsed for a SMPTE-334 Anc packet,
//	   call FindSMPTE334AncPacketInVideoFrame().
//
//	In either event, after one of the above calls the SMPTE-334 Anc data (if available) will be resident in the
//	m_708Decoder (CNTV2CaptionDecoder708 object) and ready for parsing, etc.


// SetSMPTE334AncData()
//		Just like method below except takes ptr to 8-bit data
//
bool CNTV2CaptionDecoder708::SetSMPTE334AncData (const UByte * pInAncData, const size_t inByteCount)
{
	if (!pInAncData)
		{LOGMYERROR("NULL buffer pointer"); return false;}	//	NULL buffer pointer!
	if (inByteCount >= 256)
		{LOGMYERROR(inByteCount << " exceeds 255");  return false;}	//	Too big!

	::memcpy (mRawVancDataBuffer, pInAncData, inByteCount);
	mRawVancByteCount = inByteCount;
	if (AJADebug::IsActive(AJA_DebugUnit_CC708Decode))
		LOGMYDEBUG("New Anc data: " << mRawVancByteCount << " byte(s): " << HexDump32Bytes(mRawVancDataBuffer, mRawVancByteCount));
	return true;

}	//	SetSMPTE334AncData


// SetSMPTE334AncData()
//		Just like method above except takes ptr to 16-bit data (ancSize is number of 16-bit words)
//
bool CNTV2CaptionDecoder708::SetSMPTE334AncData (const UWord * pInAncData, const size_t inWordCount)
{
	if (!pInAncData)
		{LOGMYERROR("NULL buffer pointer"); return false;}	//	NULL buffer pointer!
	if (inWordCount >= 256)
		{LOGMYERROR(inWordCount << " exceeds 255");  return false;}	//	Too big!

	for (size_t ndx(0);  ndx < inWordCount;  ndx++)
		mRawVancDataBuffer[ndx] = pInAncData[ndx] & 0xFF;	//	only the least significant byte

	mRawVancByteCount = inWordCount;
	if (AJADebug::IsActive(AJA_DebugUnit_CC708Decode))
		LOGMYDEBUG("New Anc data: " << mRawVancByteCount << " byte(s): " << HexDump32Bytes(mRawVancDataBuffer, mRawVancByteCount));
	return true;

}	//	SetSMPTE334AncData


// FindSMPTE334AncPacketInVideoFrame()
//
bool CNTV2CaptionDecoder708::FindSMPTE334AncPacketInVideoFrame (const UByte * pInVideoFrame, const NTV2VideoFormat inVideoFormat, const NTV2FrameBufferFormat inPixelFormat, bool & outHasParityErrors)
{
	UWord	ancBuff [kMaxAncPacketSize];
	ULWord	ancWordCount;

	outHasParityErrors = false;
	mRawVancByteCount = 0;
	::memset(mRawVancDataBuffer, 0, sizeof(mRawVancDataBuffer));

	if (!CNTV2SMPTEAncData::FindAnc (NTV2_SMPTEAncRP334DID,
									NTV2_SMPTEAncRP334SDID,
									reinterpret_cast <const ULWord *> (pInVideoFrame),
									kNTV2SMPTEAncChannel_Y,
									inVideoFormat,
									inPixelFormat,
									ancBuff,
									ancWordCount,
									kMaxAncPacketSize,
									outHasParityErrors))
		{LOGMYERROR("FindAnc failed, vf=" << ::NTV2VideoFormatToString(inVideoFormat) << " fbf=" << ::NTV2FrameBufferFormatToString(inPixelFormat));  return false;}

	//	Got the ANC packet -- copy the data into mRawVancDataBuffer (payload only)...
	mRawVancByteCount = ancWordCount - 7;

	for (size_t ndx(0);  ndx < mRawVancByteCount;  ndx++)
		mRawVancDataBuffer[ndx] = ancBuff[6+ndx] & 0xFF;		//	Strip parity bits

	if (AJADebug::IsActive(AJA_DebugUnit_CC708Decode))
		LOGMYDEBUG("New Anc data: " << mRawVancByteCount << " byte(s): " << HexDump32Bytes(mRawVancDataBuffer, mRawVancByteCount));
	return true;

}	//	FindSMPTE334AncPacketInVideoFrame


bool CNTV2CaptionDecoder708::FindSMPTE334AncPacketInVideoFrame (const UByte * pInVideoFrame, const NTV2VideoFormat inVideoFormat, const NTV2FrameBufferFormat inPixelFormat)
{
	bool	hasParityErrors	(false);

	return FindSMPTE334AncPacketInVideoFrame (pInVideoFrame, inVideoFormat, inPixelFormat, hasParityErrors);

}	//	FindSMPTE334AncPacketInVideoFrame


// GetSMPTE334AncData()
//
bool CNTV2CaptionDecoder708::GetSMPTE334AncData (const UByte **ppAncData, size_t & outAncSize) const
{
	*ppAncData = mRawVancDataBuffer;
	outAncSize  = mRawVancByteCount;

	return true;

}	//	GetSMPTE334AncData


// ParseSMPTE334AncPacket()
//
//		Parse a SMPTE 334 Ancillary packet and extract the 608 and 708 caption data from it
//	This assumes that the "payload" of a SMPTE 334 Ancillary Packet has already been loaded
//	into mRawVancDataBuffer[], and that mRawVancByteCount is set to the number of bytes in the payload
//	(i.e. the "Data Count" of the Ancillary Packet. This means that the first word of the data
//	is the first word of a "Caption Data Packet", as defined in CEA-708-B (soon to be moved to
//	an updated version of SMPTE 334). In addition, we assume that the parity bits have already
//	been stripped and all we have is the 8-bit raw data (if you want to do parity checks, you'll
//	need to do it before now).
//
//		The purpose of this routine is to: a) extract the "608" (aka Line 21) data from the
//	payload and put it into mCC608FrameData; and b) extract the 708 "Caption Channel Packet" data
//	from the Ancillary data and put it into mCC708FrameData[], with the word count in mCC708FrameDataByteCount.
//	Any further parsing of the extracted 608 or 708 data is left to other routines.
//
bool CNTV2CaptionDecoder708::ParseSMPTE334AncPacket (bool & outHasParityErrors)
{
	(void)outHasParityErrors;
	//Log()	<< "ParseSMPTE334AncPacket():" << endl;

	//	Some basic sanity checks --
	//	The first two words should be the cdp_identifier...
	if (mRawVancDataBuffer[0] != 0x96  ||  mRawVancDataBuffer[1] != 0x69)
	{
		LOGMYERROR("Bad ccsvcinfo_id: " << UHEX2(mRawVancDataBuffer[0])
					<< " " << UHEX2(mRawVancDataBuffer[1]) << " -- ignoring frame");
		return false;
	}

	//	Get the CDP sequence number for the new frame from both the cdp_header and cdp_footer.
	//	If they don't match, this new CDP is bogus...
	const UWord newHdrSeqCnt	((mRawVancDataBuffer[5] << 8) + mRawVancDataBuffer[6]);
	const UWord newFtrSeqCnt	((mRawVancDataBuffer[mRawVancByteCount-3] << 8) + mRawVancDataBuffer[mRawVancByteCount-2]);

	if (newHdrSeqCnt != newFtrSeqCnt)
	{
		//	The premise here is that "something happened" (a video switch?) between the time the packet started
		//	and the time it ended. Since we don't/can't know what happened to the data in-between, we simply throw
		//	out the entire packet instead of risking parsing bad data...
		LOGMYERROR("Header and Footer sequence numbers ("
					<< newHdrSeqCnt << ", " << newFtrSeqCnt << ") don't match: "
					<< UHEX2 (mRawVancDataBuffer [0]) << " " << UHEX2 (mRawVancDataBuffer [1])
					<< " :: " << UHEX2 (mRawVancDataBuffer [mRawVancByteCount - 3])
					<< " " << UHEX2 (mRawVancDataBuffer [mRawVancByteCount - 2])
					<< " -- ignoring frame");
		return false;
	}

	//	Some data is sent over the span of multiple packets. If the sequence number of this new frame
	//	is any OTHER than one count past the previous frame, we need to dump any multi-packet CCPs and
	//	Service_Info updates that are currently in-progress!
	mIsNonContiguousFrame = (newHdrSeqCnt != (UWord)(mPreviousSequenceCount + 1));
	if (mIsNonContiguousFrame)
	{
		//	If this frame is NOT contiguous with previous frames, throw away any accumulated caption channel packet and service_info "work in progress"...
		ClearAllCaptionChannelPacketInfo ();
		mNewServiceInfo.InitAllServiceInfo ();
		LOGMYWARN("New frame " << newHdrSeqCnt << " discontiguous with prior frame " << mPreviousSequenceCount);
	}

	bool	bResult	(true);
	size_t	index	(0);

	#if defined (_DEBUG)
		//	Call DebugParseSMPTE334AncPacket() to see if there are any errors in the incoming data.
		bResult = DebugParseSMPTE334AncPacket(outHasParityErrors);
	#endif	//	_DEBUG

	//	Skip past cdp_header...
	index += 7;

	//	We might now be pointing at the beginning of the (optional) timecode_section...
	bResult = ParseSMPTE334CCTimeCodeSection(index);
	if (!bResult)
		goto end;

	//	We should now be pointing at the beginning of the cc_data_section...
	bResult = ParseSMPTE334CCDataSection(index);
	if (!bResult)
		goto end;

	//	We might now be pointing at the beginning of the (optional) svc_info_section...
	bResult = ParseSMPTE334CCServiceInfoSection(index);
	if (!bResult)
		goto end;

	// future_section...?

	//	ParseSMPTE334CCDataSection() splits the CDP cc_data into zero or more CaptionChannelPackets.
	//	This steps through the COMPLETED CCPs and parses them down to the Service Block level and
	//	adds them to the separate Service Block Queues...
	bResult = ParseAllCaptionChannelPackets();

	//	Save the sequence count for next frame...
	mPreviousSequenceCount = newHdrSeqCnt;

end:
	return bResult;

}	//	ParseSMPTE334AncPacket


bool CNTV2CaptionDecoder708::ParseSMPTE334AncPacket (void)
{
	bool	hasParityErrors	(false);

	return ParseSMPTE334AncPacket(hasParityErrors);

}	//	ParseSMPTE334AncPacket




// ParseSMPTE334CCTimeCodeSection()
//
//	Place-holder: we currently don't do much with timecode (if it's there), but this would be
//	the place to add any specific parsing to pull the timecode data out of the incoming SMPTE 334
//	packet and store it in some discoverable place for later processing.
//
bool CNTV2CaptionDecoder708::ParseSMPTE334CCTimeCodeSection (size_t & index)
{
	bool	bResult	(true);

	if (index < mRawVancByteCount && mRawVancDataBuffer [index] == NTV2_CC708_CDPTimecodeId)
		index += 5;		//	We got a timecode data section: skip past it
	else
	{
		// note: lack of a timecode_section does NOT constitute an error
	}

	return bResult;

}	//	ParseSMPTE334CCTimeCodeSection



// ParseSMPTE334CCDataSection()
//
//	This method pulls the "cc_data" bytes out of an incoming SMPTE-334 Caption Distribution Packet
//	(CDP) and separates the "608" (SD) caption data from the "708" (HD) Caption Channel Packet data.
//
//	The 608 data (if present) is relatively easy to deal with: there can be up to 3 video fields of
//	608 data present, depending	on the incoming video frame rate: 29.97/30 fps video typically has
//	2 fields of caption data; 59.94/60 fps video typically has 1 field of caption data; and 23.98/24 fps
//	video can have an "extra" 3rd field of data to go along with 3:2 pulldown. All of the "608" data
//	that is present in the incoming SMPTE-334 packet is copied into a local struct, mCC608FrameData.
//
//	"708" caption data is a little trickier. The 708 spec defines a "Caption Channel Packet" (CCP),
//	which is a variable-length pile of data (2 - 128 bytes, even bytecounts only) which carries
//	the raw caption data for a given caption service. Caption Channel Packets may be "asynchronous"
//	with respect to Caption Distribution Packets. That is, a Caption Channel Packet may start in one
//	CDP, but "spill over" into additional CDPs. Or, a given CDP may contain multiple CCPs. All we can
//	count on is that there can only be one "open" (in-progress) CCP at a time. This means that we
//	accumulate data bytes into a CCP buffer until we see the flag for a new CCP, then accumulate
//	following data bytes into another CCP buffer, etc. etc. until the end of the CDP data. At that
//	point we will have zero or more "complete" CCPs, and 0 or 1 "in-progress" CCPs to be resumed in
//	the next CDP.
//
//	Note that this method does NOT attempt to parse the individual CCPs - it only demuxes and accumulates
//	the data to separate CCP buffers. Call ParseAllCaptionChannelPackets() to break the CCPs down to
//	more elementary caption service commands.
//
bool CNTV2CaptionDecoder708::ParseSMPTE334CCDataSection (size_t & index)
{
	static const string	cc_types[]	=	{"608F1", "608F2", "Cont708CCP", "New708CCP", "4", "5", "6", "7", "8"};
	ostringstream		oss;

	//	Init the CaptionChannelPacketInfo array to receive a new CDP...
	ResetCaptionChannelPacketInfoForNewCDP();
	if (index >= mRawVancByteCount)
		{LOGMYWARN("No CDP Data -- index " << index << " past mRawVancByteCount " << mRawVancByteCount);  return true;}	//	No cdp_data? (note: not an error - technically it is an option...)
	if (mRawVancDataBuffer[index] != NTV2_CC708_CDPDataId)
		{LOGMYWARN("No CDP Data -- " << UHEX2(mRawVancDataBuffer[index]) << " != " << UHEX2(NTV2_CC708_CDPDataId) << "NTV2_CC708_CDPDataId");  return true;}	//	No cdp_data? (note: not an error - technically it is an option...)

//cout << "[" << DEC0N(index,2) << "] ccDataID=" << HEX0N(UWord(mRawVancDataBuffer[index]),2) << HX << endl;
	index += 1;		//	Skip past ccdata_id

	//	We don't know how large the cc_data_section is until we parse it - but make sure there is at least one additional word available...
	if (index >= mRawVancByteCount)
		{LOGMYERROR(mRawVancByteCount << "-byte packet too small -- expected at least " << index << " bytes");  return false;}

	//	Get the number of "cc_data" triplets there are in this packet...
	const UByte	ccCount	(mRawVancDataBuffer[index] & 0x1F);
//cout << "[" << DEC0N(index,2) << "] ccCount=" << xHEX0N(UWord(ccCount),2) << HX << endl;
	index += 1;		// skip past cc_count

	//LOGMYDEBUG(ccCount << " cc_data triplets found");

	//	Now we know how large the rest of the ccdata_section SHOULD be -- make sure we have that much data...
	const size_t	size	(3 * ccCount);

	if (ccCount == 0)
		{LOGMYWARN("Zero cc_data triplets");  return true;}
	if (index+size > mRawVancByteCount)
		{LOGMYERROR(mRawVancByteCount << "-byte packet too small, need " << DEC(index+size) << " to process " << uint16_t(ccCount) << " cc_data triplets");  return false;}

	//	Init 608 data struct...
	Clear608CaptionData();

	//	Init 708 data...
	mCC708FrameDataByteCount = 0;

	//	Get cdp_hdr_sequence_ctr...
	oss << uint16_t(ccCount) << " triplets processed: ";
	for (int ccTriplet(0);  ccTriplet < ccCount;  ccTriplet++)
	{
		//	The first cc_data triplet should be the Field 1 608 data...
		UByte cc_typeValid	(mRawVancDataBuffer[index    ]);
		UByte cc_data_1		(mRawVancDataBuffer[index + 1]);
		UByte cc_data_2		(mRawVancDataBuffer[index + 2]);

		UByte cc_type		(cc_typeValid & 0x03);			//	bits 1,0
		UByte cc_valid		((cc_typeValid & 0x04) >> 2);	//	bit 2
		oss	<< "[" << ccTriplet << ": " << (cc_valid?"valid":"invalid");
//cout << "[" << DEC0N(index,2) << "] ccType=" << xHEX0N(UWord(cc_type),2) << HX << endl;
		if (cc_valid)
		{
			oss << " cc_type=" << cc_types[cc_type];
			switch (cc_type)
			{
				//	CEA-608 "Field 1" data
				case 0:		//bDidPrint = DebugParse608CaptionData (0, cc_data_1, cc_data_2);
							mCC608FrameData.f1_char1 = cc_data_1;
							mCC608FrameData.f1_char2 = cc_data_2;			//	Field 1 608 data (CC1, CC2, TEXT1, TEXT2)
							mCC608FrameData.bGotField1Data = true;

							//	Add to data queue if the data is anything OTHER than Nulls...
							if (cc_data_1 != 0x80 || cc_data_2 != 0x80)
								mCC608QueueField1.Push608Data (cc_data_1, cc_data_2, true);
							break;

				//	CEA-608 "Field 2" data
				case 1:		//bDidPrint = DebugParse608CaptionData (1, cc_data_1, cc_data_2);
							mCC608FrameData.f2_char1 = cc_data_1;
							mCC608FrameData.f2_char2 = cc_data_2;			//	Field 2 608 data (CC3, CC4, TEXT3, TEXT4)
							mCC608FrameData.bGotField2Data = true;

							//	Add to data queue if the data is anything OTHER than Nulls...
							if (cc_data_1 != 0x80 || cc_data_2 != 0x80)
								mCC608QueueField2.Push608Data (cc_data_1, cc_data_2, true);
							break;

				//	CEA-708 data: start of new Caption Channel Packet
				case 3:		mCC708FrameDataByteCount = 0;								//	New Caption Channel Packet
							{
								int seqNum		= (cc_data_1 & 0xC0) >> 6;
								int pktSizeCode = (cc_data_1 & 0x3F);
								int pktSize	= (pktSizeCode == 0) ? 128 : (pktSizeCode * 2);
								StartNewCaptionChannelPacketInfo(seqNum, pktSize);		//	Close out any previously open CCP and open a new one
								//printf ("New CCP: 0x%02x 0x%02x\n", cc_data_1, cc_data_2);
							}
							// FALLTHRU...!

				//	CEA-708 data: continuation of existing Caption Channel Packet
				case 2:		mCC708FrameData[mCC708FrameDataByteCount++] = cc_data_1;	//	Accumulate all 708 cc_data to an array (no longer used)
							mCC708FrameData[mCC708FrameDataByteCount++] = cc_data_2;

							AddCaptionChannelPacketInfoData(cc_data_1);	//	Accumulate 708 cc_data to the currently "open" CCP buffer
							AddCaptionChannelPacketInfoData(cc_data_2);
							break;
			}	//	switch on cc_type
		}	//	if cc_valid
		oss << "] ";
		index += 3;		// point to next triplet
	}	//	for (int ccTriplet = 0; ccTriplet < ccCount; ccTriplet++)
	LOGMYINFO(oss.str());
	return true;

}	//	ParseSMPTE334CCDataSection


// ParseSMPTE334CCServiceInfoSection()
//
//	This method parses the "cc_serviceinfo" section of a SMPTE-334 Caption Data Packet and saves
//	the information in a CNTV2Caption708ServiceInfo class. Because a complete set of cc_serviceinfo
//	can be sent over a period of many frames (i.e. multiple CDPs, or SMPTE-334 packets), the data
//	is double-buffered. New cc_serviceinfo data is saved in a "new" CNTV2Caption708ServiceInfo, and
//	only after an entire set of cc_serviceinfo data is received do we copy the "new" data into the
//	"current" data (the cc_serviceinfo data is also copied out to the specific CNTV2Caption708Service
//	objects at this time).
//
bool CNTV2CaptionDecoder708::ParseSMPTE334CCServiceInfoSection (size_t & index)
{
	bool	bResult	(true);

	if (index < mRawVancByteCount && mRawVancDataBuffer[index] == NTV2_CC708_CDPServiceInfoId)
	{
		//	We got a svc_info section: we don't know how long it is until we parse the next word
		index += 1;							//	Skip past ccsvcinfo_id
		if (index < mRawVancByteCount)		//	Make sure there IS another word...
		{
			const UByte	svcInfoStatus	(mRawVancDataBuffer[index]);
			index += 1;

			const size_t	svcCount	(svcInfoStatus & 0x0f);

			//	There are 7 bytes of status data per service - make sure we have that much data...
			if (index + (7 * svcCount) < mRawVancByteCount)
			{
				//	See if this batch of service_info data is the "start" (i.e. the first of a batch of new data)
				//	if so, we need to init the mNewServiceInfo to start with a clean slate...
				if ((svcInfoStatus & NTV2_CC708CDPSvcInfo_SvcInfoStart) != 0)
				{
					mNewServiceInfo.InitAllServiceInfo();

					//	Remember the state of the accompanying "Change" flag...
					bool bChange = ((svcInfoStatus & NTV2_CC708CDPSvcInfo_SvcInfoChange) != 0);
					mNewServiceInfo.SetServiceInfoChangeFlag(bChange);
				}

				//	Loop through all of the services and save the data to our temp database...
				for (size_t serviceNdx(0);  serviceNdx < svcCount;  serviceNdx++)
				{
					//	The 1st service byte contains the caption_service_number, which is either 5 bits or 6 bits depending on bit 6...
					int caption_service_number;
					if ((mRawVancDataBuffer[index] & 0x40) == 0)
						caption_service_number = mRawVancDataBuffer[index] & 0x3f;		// if bit 6 is 0, it's a 6-bit number
					else
						caption_service_number = mRawVancDataBuffer[index] & 0x1f;		// if bit 6 is 1, it's a 5-bit number
					index += 1;

					//	Clear any existing info for this service...
					mNewServiceInfo.InitCCServiceInfo(caption_service_number);

					//	The 2nd-4th bytes contain three ascii characters defining the language of the service...
					NTV2_CC708ServiceLanguage lang;
					lang.langID[0] = char(mRawVancDataBuffer[index++]);
					lang.langID[1] = char(mRawVancDataBuffer[index++]);
					lang.langID[2] = char(mRawVancDataBuffer[index++]);
					mNewServiceInfo.SetServiceInfoLanguage (caption_service_number, lang);

					//	The 5th byte contains the "digital_cc" flag and a copy (at least we HOPE it's the same...) of the caption_service_number...
					bool bDigitalCC = ( (mRawVancDataBuffer[index] & NTV2_CC708CDPSvcInfo_DigitalCC) != 0);
					mNewServiceInfo.SetServiceInfoDigitalCC (caption_service_number, bDigitalCC);
					index += 1;

					//	The 6th byte contains the "easy_reader" and "wide_aspect" flags...
					bool bEasyReader = ( (mRawVancDataBuffer[index] & NTV2_CC708CDPSvcInfo_EasyReader) != 0);
					mNewServiceInfo.SetServiceInfoEasyReader (caption_service_number, bEasyReader);

					bool bWideAspect = ( (mRawVancDataBuffer[index] & NTV2_CC708CDPSvcInfo_WideAspectRatio) != 0);
					mNewServiceInfo.SetServiceInfoWideAspect (caption_service_number, bWideAspect);
					index += 1;

					//	The 7th byte is unused...
					index += 1;

					//	By definition, a service is "active" if we have received service_info for it...
					mNewServiceInfo.SetServiceInfoActive (caption_service_number, true);

				}	// for each service

				//	If this batch of service_info was the last of a series, then we know we're not going to get any more
				//	and we should copy the contents of the "new" service_info database to the "current" database AND the
				//	individual CNTV2Caption708Service objects...
				if ((svcInfoStatus & NTV2_CC708CDPSvcInfo_SvcInfoComplete) != 0)
				{
					mCurrentServiceInfo.CopyAllServiceInfo (mNewServiceInfo.GetAllServiceInfoPtr());
					UpdateServiceInfo();
				}

			}	// if (index + (7 * svcCount) < mRawVancByteCount)
			else
				bResult = false;	//	Not enough data for the advertised number of services

		}	// if (index < mRawVancByteCount)
		else
			bResult = false;	//	svc_info section with no status word? bad...

	}	// if (index < mRawVancByteCount && mRawVancDataBuffer [index] == NTV2_CC708_CDPServiceInfoId)
//	else
//		;	//	Note: lack of a svc_info section does NOT constitute an error

	return bResult;

}	//	ParseSMPTE334CCServiceInfoSection


// UpdateServiceInfo()
//		Copy the contents of mCurrentServiceInfo out to the individual "service" objects
//
bool CNTV2CaptionDecoder708::UpdateServiceInfo (void)
{
	bool	bResult	(true);

	for (int i = 0; i < NTV2_CC708MaxNumServices; i++)
		mAvailableServices [i].SetServiceInfo (mCurrentServiceInfo.GetOneServiceInfoPtr (i));

	return bResult;

}	//	UpdateServiceInfo


// Clear608CaptionData
//		Initialize 608 caption local memory
bool CNTV2CaptionDecoder708::Clear608CaptionData (void)
{
	mCC608FrameData.Clear();
	return true;

}	//	Clear608CaptionData


// GetCC608CaptionData()
//
CaptionData CNTV2CaptionDecoder708::GetCC608CaptionData (void)
{
	CaptionData	result;
	result.Zero();
	mCC608QueueField1.Pop608Data (result.f1_char1, result.f1_char2, result.bGotField1Data);
	mCC608QueueField2.Pop608Data (result.f2_char1, result.f2_char2, result.bGotField2Data);
	return result;

}	//	GetCC608CaptionData



// GetCaptionChannelPacket()
//
bool CNTV2CaptionDecoder708::GetCaptionChannelPacket (UBytePtr & outDataPtr, size_t & outSize)
{
	outDataPtr = mCC708FrameData;
	outSize  = mCC708FrameDataByteCount;
	return true;

}	//	GetCaptionChannelPacket


#if defined (_DEBUG)

// DebugParseSMPTE334AncPacket()
//
//		Print the contents of a SMPTE 334 Ancillary packet.	This assumes that the "payload" of
//	a SMPTE 334 Ancillary Packet has already been loaded into mRawVancDataBuffer[], and that mRawVancByteCount
//	is set to the number of bytes in the payload (i.e. the "Data Count" of the Ancillary Packet.
//	This means that the first word of the data is the first word of a "Caption Data Packet", as
//	defined in CEA-708-B (soon to be moved to an updated version of SMPTE 334). In addition, we
//	assume that the parity bits have already been stripped and all we have is the 8-bit raw data
//	(if you want to do parity checks, you'll need to do it before now).
//
bool CNTV2CaptionDecoder708::DebugParseSMPTE334AncPacket (bool & outHasParityErrors)
{
	bool	bResult	(true);
	size_t	index	(0);

	//	If this frame had to be cleared because it was not contiguous, note it here...
	if (mIsNonContiguousFrame)
		LOGMYWARN("Current frame discontiguous with prior frame -- accumulated data and service info flushed");

	if (TestLogMask(kCaptionLog_DecodeVANC))
	{
		//	Print hex dump of SMPTE-334 data...
		cerr << endl << "   SMPTE334AncPacket:" << endl;
		DumpMemory (mRawVancDataBuffer, mRawVancByteCount, cerr,  /*inRadix*/ 16,  /*inBytesPerGroup*/ 1,  /*inGroupsPerLine*/ 32,  /*inAddressRadix*/ 0);
	}

	//	Parse CDP...
	//LogIf (kCaptionLog_DecodeCDP) << endl << "   Caption Distribution Packet (CDP):" << endl;

	//	Parse the CDP Header (bytes 0 - 6)...
	if (index < mRawVancByteCount)
	{
		if (!DebugParseCDPHeader (mRawVancDataBuffer, index, mRawVancByteCount, &index, &mParsedCDPData.cdp_header, outHasParityErrors))
			return false;
	}

	//	Index now points to the first word past the header. This could either be
	//	the start of an (optional) cdp_timecode_section, or the start of a cdp_data section...
	if (index < mRawVancByteCount && mRawVancDataBuffer [index] == NTV2_CC708_CDPTimecodeId)
	{
		//	We got a timecode data section: parse it...
		if (!DebugParseCDPTimecode (mRawVancDataBuffer, index, mRawVancByteCount, &index, &mParsedCDPData.timecode_section, outHasParityErrors))
			return false;
	}

	//	Index should now point to the beginning of a cc_data_section...
	if (index < mRawVancByteCount && mRawVancDataBuffer [index] == NTV2_CC708_CDPDataId)
	{
		if (!DebugParseCDPData (mRawVancDataBuffer, index, mRawVancByteCount, &index, &mParsedCDPData.ccdata_section, outHasParityErrors))
			return false;
	}

	//	The index now points to the next word following the cc_data_section, which could
	//	be either: a) the beginning of a cdp_footer; b) the beginning of an (optional)
	//	ccsvcinfo_section; or c) some "future section" (to be defined)...
	if (index < mRawVancByteCount && mRawVancDataBuffer [index] == NTV2_CC708_CDPServiceInfoId)
	{
		if (!DebugParseCDPServiceInfo (mRawVancDataBuffer, index, mRawVancByteCount, &index, &mParsedCDPData.ccsvcinfo_section, outHasParityErrors))
			return false;
	}

	//	Is the next section a "future_section", i.e. not defined by today's standards? (if so, skip over it)
	//	Note: there could be multiple future_sections - keep skipping until we're past all of them...
	while (index < mRawVancByteCount && mRawVancDataBuffer [index] >= 0x75 && mRawVancDataBuffer [index] <= 0xEF)
	{
		if (!DebugParseCDPFutureSection (mRawVancDataBuffer, index, mRawVancByteCount, &index, outHasParityErrors))
			return false;
	}

	//	The index now points to the beginning of a cdp_footer...
	if (index < mRawVancByteCount && mRawVancDataBuffer [index] == NTV2_CC708_CDPFooterId)
	{
		if (!DebugParseCDPFooter (mRawVancDataBuffer, index, mRawVancByteCount, &index, &mParsedCDPData.cdp_footer, outHasParityErrors))
			return false;
	}

	return bResult;

}	//	DebugParseSMPTE334AncPacket



// DebugParseCDPHeader()
//
//		Parse 'n print useful stuff about a cdp_header section (see CEA-708-B, pg 71).
//	pInData points to the first word of a Caption Data Packet, and pktIndex is the position
//	of the first word of the cdp_header (usually 0). maxPacketSize is the total length
//	of the Caption Data Packet.
//		This method returns the index of the next word following the section.
//
bool CNTV2CaptionDecoder708::DebugParseCDPHeader (const UByte * pInData, size_t pktIndex, size_t maxPacketSize, size_t *pNewIndex, NTV2_CC708CDPHeaderPtr pHdr, bool & outHasErrors)
{
	bool	bResult	(false);

	NTV2_CC708CDPHeader hdr;
	hdr.cdp_identifier = 0;

	size_t	index	(pktIndex);
	size_t	size	(7);			//	There are 7 words in a cdp_header

	//	Make sure there is enough data left...
	if ( (index + size) > maxPacketSize)
	{
		LOGMYERROR("Not enough data in CDP (" << maxPacketSize << ") to parse cdp_header!");
		outHasErrors = true;
	}
	else
	{
		//	The first two bytes should be 0x96 0x69 - the CDP Identifier words...
		if (pInData[index] != NTV2_CC708_CDPHeaderId1  ||  pInData[index+1] != NTV2_CC708_CDPHeaderId2)
		{
			LOGMYERROR("bad cdp_identifier 0x" << UHEX2(pInData[index]) << "|0x" << UHEX2(pInData[index+1]) << ", expected 0x" << UHEX2(NTV2_CC708_CDPHeaderId1) << "|0x" << UHEX2(NTV2_CC708_CDPHeaderId2));
			outHasErrors = true;
		}
		else
		{
			//	Header ID
			hdr.cdp_identifier = (NTV2_CC708_CDPHeaderId1 << 8) + NTV2_CC708_CDPHeaderId2;
//cout << "[" << DEC0N(index,2) << "] cdpHdrID=" << xHEX0N(UWord(mRawVancDataBuffer[index]),2) << " " << xHEX0N(UWord(mRawVancDataBuffer[index+1]),2) << endl;
			index += 2;

			//	The next word is the packet length - which should be the same as the SMPTE 334 Anc Data Count...
//cout << "[" << DEC0N(index,2) << "] cdpLen=" << xHEX0N(UWord(mRawVancDataBuffer[index]),2) << HX << endl;
			hdr.cdp_length = pInData[index++];
			if (hdr.cdp_length != maxPacketSize)
			{
				LOGMYERROR("cdp_length " << UWord(hdr.cdp_length) << " doesn't match SMPTE334 DC Size " << maxPacketSize);
				outHasErrors = true;
			}

			//	The next word carries the video frame rate, which affects the "cc_count", or number of data triplets
			//	in the packet (however, this is also carried in the 2nd word in the cc_data section, too)...
			hdr.cdp_frame_rate = (pInData[index] & 0xF0) >> 4;
//cout << "[" << DEC0N(index,2) << "] cdpFR=" << xHEX0N(hdr.cdp_frame_rate,2) << HX << endl;
			index++;

			//	The next word carries a bunch of misc status bits...
			hdr.cdp_flags = pInData [index];
//cout << "[" << DEC0N(index,2) << "] flags=" << xHEX0N(hdr.cdp_flags,2) << HX << endl;
			index++;

			//	The next two words carry the "sequence counter". This is an error checking mechanism: the SAME two-byte
			//	sequence count should also be inserted in the cdp_footer. If it'S NOT the same, it means something
			//	happened mid-packet (like a switch) and the whole packet should be considered wrecked...
			hdr.cdp_hdr_sequence_cntr = (int(pInData[index]) * 256) + int(pInData[index+1]);
			mDebugFrameNumber = hdr.cdp_hdr_sequence_cntr;		//	save as global for other debug purposes
//cout << "[" << DEC0N(index,2) << "] hdrSeq=" << xHEX0N(mDebugFrameNumber,2) << HX << endl;
			index += 2;
			bResult = true;

			//	If desired, print results to console...
			LOGMYDEBUG("cdp_header: " << " cdp_identifier=" << xHEX0N(hdr.cdp_identifier,4)
						<< " cdp_length=" << UWord(hdr.cdp_length)
						<< " cdp_frame_rate=" << hdr.cdp_frame_rate
						<< " cdp_misc_bits=0x" << UHEX2(hdr.cdp_flags) << ":"	<< (hdr.cdp_flags & 0x80 ? " +time_code_present" : "")
																				<< (hdr.cdp_flags & 0x40 ? " +ccdata_present" : "")
																				<< (hdr.cdp_flags & 0x20 ? " +svcinfo_present" : "")
																				<< (hdr.cdp_flags & 0x10 ? " +svc_info_start" : "")
																				<< (hdr.cdp_flags & 0x08 ? " +svc_info_change" : "")
																				<< (hdr.cdp_flags & 0x04 ? " +svc_info_complete" : "")
																				<< (hdr.cdp_flags & 0x02 ? " +caption_service_active" : "")
						<< " cdp_hdr_sequence_cntr=" << xHEX0N(hdr.cdp_hdr_sequence_cntr,4));
		}
	}

	if (pNewIndex)
		*pNewIndex = index;

	if (bResult && pHdr)
		*pHdr = hdr;		// struct copy

	return bResult;

}	//	DebugParseCDPHeader



// DebugParseCDPTimecode()
//
//		Parse 'n print useful stuff about a CDP time_code_section (see CEA-708-B, pg 73).
//	pInData points to the first word of a Caption Data Packet, and pktIndex is the position
//	of the first word of the time_code_section. maxPacketSize is the total length of the
//	Caption Data Packet.
//		This method returns the index of the next word following the section.
//
bool CNTV2CaptionDecoder708::DebugParseCDPTimecode (const UByte * pInData, size_t pktIndex, size_t maxPacketSize, size_t * pNewIndex, NTV2_CC708CDPTimecodeSectionPtr pTC, bool & outHasErrors)
{
	bool	bResult	(false);

	NTV2_CC708CDPTimecodeSection tc;
	tc.time_code_section_id = 0;

	size_t	index	(pktIndex);
	size_t	size	(5);			//	There are 5 words in a time_code_section

	//	Make sure there is enough data left...
	if ( (index + size) > maxPacketSize)
	{
		LOGMYERROR("Not enough data in CDP (" << maxPacketSize << ") to parse cctimecode_section!");
		outHasErrors = true;
	}
	else
	{
		//	The first word should be 0x71 - the time_code_section ID...
		if (pInData [index] != NTV2_CC708_CDPTimecodeId)
		{
			LOGMYERROR("Bad time_code_section_id 0x" << UHEX2(pInData[index]) << ", expected 0x" << UHEX2(NTV2_CC708_CDPTimecodeId));
			outHasErrors = true;
		}
		else
		{
			//	Get section ID...
			tc.time_code_section_id = pInData [index++];

			//	Unpack the timecode...
			tc.tc_10hrs = (pInData [index    ] & 0x30) >> 4;
			tc.tc_1hrs  = (pInData [index    ] & 0x0F);
			tc.tc_10min = (pInData [index + 1] & 0x70) >> 4;
			tc.tc_1min  = (pInData [index + 1] & 0x0F);
			tc.tc_10sec = (pInData [index + 2] & 0x70) >> 4;
			tc.tc_1sec  = (pInData [index + 2] & 0x0F);
			tc.tc_10fr  = (pInData [index + 3] & 0x70) >> 4;
			tc.tc_1fr   = (pInData [index + 3] & 0x0F);

			tc.tc_field_flag   = (pInData [index + 2] & 0x80);
			tc.drop_frame_flag = (pInData [index + 3] & 0x80);

			index += 4;

			bResult = true;

			//	Print results to console...
			LOGMYDEBUG("time_code_section: time_code_section_id=0x" << UHEX2(tc.time_code_section_id)
						<< " tc=" << tc.tc_10hrs << tc.tc_1hrs << ":" << tc.tc_10min << tc.tc_1min << ":" << tc.tc_10sec << tc.tc_1sec << ":" << tc.tc_10fr << tc.tc_1fr
						<< " field=" << (tc.tc_field_flag?"Y":"N") << " DF=" << (tc.drop_frame_flag?"Y":"N"));
		}
	}

	if (pNewIndex)
		*pNewIndex = index;

	if (bResult && pTC)
		*pTC = tc;		// struct copy

	return bResult;

}	//	DebugParseCDPTimecode



// DebugParseCDPData()
//
//		Parse 'n print useful stuff about a CDP ccdata_section (see CEA-708-B, pg 74).
//	pInData points to the first word of a Caption Data Packet, and pktIndex is the position
//	of the first word of the ccdata_section. maxPacketSize is the total length of the
//	Caption Data Packet.
//		This method returns the index of the next word following the section.
//
bool CNTV2CaptionDecoder708::DebugParseCDPData (const UByte * pInData, size_t pktIndex, size_t maxPacketSize, size_t * pNewIndex, NTV2_CC708CDPDataSectionPtr pCC, bool & outHasErrors)
{
	bool	bResult	(false);
	size_t	index	(pktIndex);
	size_t	size	(2);			//	We don't know how many words are in this section until we parse it, but make sure there are at least two!
	NTV2_CC708CDPDataSection cc;
	::memset (&cc, 0, sizeof(cc));	//	cc.ccdata_id = 0;

	//	Make sure there is enough data left...
	if ((index + size) > maxPacketSize)
	{
		LOGMYERROR("Not enough data in CDP (" << maxPacketSize << ") to parse ccdata_section!");
		outHasErrors = true;
	}
	else
	{
		//	The first word should be 0x72 - the ccdata_section ID...
		if (pInData[index] != NTV2_CC708_CDPDataId)
		{
			LOGMYERROR("bad ccdata_id 0x" << UHEX2(pInData[index]) << ", expected 0x" << UHEX2(NTV2_CC708_CDPDataId));
			outHasErrors = true;
		}
		else
		{
			//	Get section ID...
			cc.ccdata_id = pInData[index++];

			//	The next word is the cc_count, which is the number of "cc constructs" (or data triplets) in the data section...
			cc.cc_count = pInData[index++] & 0x1F;

			//	Now we know how big the rest of the ccdata_section is...
			size = 3 * size_t(cc.cc_count);
			if ((index + size) > maxPacketSize)
			{
				LOGMYERROR("Not enough data remaining in CDP (" << maxPacketSize << ") to parse ccdata_section!");
				outHasErrors = true;
			}
			else
			{
				for (int i = 0;  i < cc.cc_count;  i++)
				{
					cc.cc_data[i].cc_valid  = pInData[index    ] & 0x04;		// (bool)
					cc.cc_data[i].cc_type   = pInData[index    ] & 0x03;
					cc.cc_data[i].cc_data_1 = pInData[index + 1];
					cc.cc_data[i].cc_data_2 = pInData[index + 2];
					index += 3;
				}

				bResult = true;

				//	If desired, print results to console...
				LOGMYDEBUG("ccdata_section: ccdata_id=0x" << UHEX2(cc.ccdata_id) << " cc_count=" << cc.cc_count);
				#if 0
				for (int i(0);  i < cc.cc_count;  i++)
					cerr	<< "[" << setw(2) << i << "]: type = " << cc.cc_data[i].cc_type << ", valid = " << cc.cc_data[i].cc_valid
							<< ", data1 = 0x" << UHEX2(cc.cc_data[i].cc_data_1) << ", data2 = 0x" << UHEX2(cc.cc_data[i].cc_data_2) << endl;
				#endif
			}	//	else ccdata_section exists
		}	//	else first word is NTV2_CC708_CDPDataId
	}	//	else (index + size) <= maxPacketSize

	if (pNewIndex)
		*pNewIndex = index;

	if (bResult && pCC)
		*pCC = cc;		// struct copy

	return bResult;

}	//	DebugParseCDPData



// DebugParseCDPServiceInfo()
//
//	Parse 'n print useful stuff about a CDP ccsvcinfo_section (see CEA-708-B, pg 74).
//	pInData points to the first word of a Caption Data Packet, and pktIndex is the position
//	of the first word of the ccsvcinfo_section. maxPacketSize is the total length of the
//	Caption Data Packet.
//		This method returns the index of the next word following the section.
//
bool CNTV2CaptionDecoder708::DebugParseCDPServiceInfo (const UByte * pInData, size_t pktIndex, size_t maxPacketSize, size_t * pNewIndex, NTV2_CC708CDPServiceInfoSectionPtr pSvc, bool & outHasErrors)
{
	bool	bResult	(false);

	NTV2_CC708CDPServiceInfoSection svc;
	svc.ccsvcinfo_id = 0;

	size_t	index	(pktIndex);
	size_t	size	(2);			//	We don't know how many words are in this section until we parse it, but make sure there are at least two!

	//	Make sure there is enough data left...
	if ((index + size) > maxPacketSize)
	{
		LOGMYERROR("Not enough data in CDP (" << maxPacketSize << ") to parse ccsvcinfo_section!");
		outHasErrors = true;
	}
	else
	{
		//	The first word should be 0x73 - the ccsvcinfo_section ID...
		if (pInData[index] != NTV2_CC708_CDPServiceInfoId)
		{
			LOGMYERROR("Bad ccsvcinfo_id 0x" << UHEX2(pInData[index]) << ", expected 0x" << UHEX2(NTV2_CC708_CDPServiceInfoId));
			outHasErrors = true;
		}
		else
		{
			//	Get section ID...
			svc.ccsvcinfo_id = pInData [index++];

			//	The next word contains the svc_info_start/change/complete flags, as well as a count
			//	of how many services are being reported...
			svc.svc_info_start		= (pInData [index] & 0x40) >> 6;
			svc.svc_info_change		= (pInData [index] & 0x20) >> 5;
			svc.svc_info_complete	= (pInData [index] & 0x10) >> 4;
			svc.svc_count			=  pInData [index] & 0x0F;
			index += 1;

			//	Now we know how big the entire ccsvcinfo_section is
			size = 2 + (7 * size_t (svc.svc_count));
			if ((index + size) > maxPacketSize)
			{
				LOGMYERROR("Not enough data remaining in CDP (" << maxPacketSize << ") to parse ccsvcinfo_section!");
				outHasErrors = true;
			}
			else
			{
				for (int i(0);  i < svc.svc_count;  i++)
				{
					svc.svc_info[i].captionSvcNumber	= pInData [index] & 0x01F;
					svc.svc_info[i].language.langID [0]	= char (pInData [index + 1]);
					svc.svc_info[i].language.langID [1]	= char (pInData [index + 2]);
					svc.svc_info[i].language.langID [2]	= char (pInData [index + 3]);
					svc.svc_info[i].digitalCC			= pInData [index + 4] & 0x80;		//	bool
					svc.svc_info[i].csn_line21field		= pInData [index + 4] & 0x7f;		//	the rest of that byte
					svc.svc_info[i].easyReader			= pInData [index + 5] & 0x80;		//	bool
					svc.svc_info[i].wideAspect			= pInData [index + 5] & 0x40;		//	bool
														//	note: the rest of byte 5 and all of byte 6 is '1's
					index += 7;
				}

				bResult = true;

				//	Print results to console...
				LOGMYDEBUG("ccsvcinfo_section: ccsvcinfo_id=0x" << UHEX2(svc.ccsvcinfo_id) << " svc_info_start=" << svc.svc_info_start
						<< " svc_info_change=" << svc.svc_info_change << " svc_info_complete=" << svc.svc_info_complete << " svc_count=" << svc.svc_count);
				if (AJADebug::IsActive(AJA_DebugUnit_CC708Decode))
					for (int i(0);  i < svc.svc_count;  i++)
					{
						ostringstream	oss;
						oss	<< "[" << i << "]: caption_service_number=" << svc.svc_info[i].captionSvcNumber << " language=";
						if (svc.svc_info[i].language.langID[0] < 0x20 || svc.svc_info[i].language.langID[1] < 0x20 || svc.svc_info[i].language.langID[2] < 0x20)
							oss	<< "'0x" << UHEX2(svc.svc_info[i].language.langID[0]) << " " << UHEX2(svc.svc_info[i].language.langID[1]) << " " << UHEX2(svc.svc_info[i].language.langID[2]) << "'";
						else
							oss	<< "'" << svc.svc_info[i].language.langID[0] << svc.svc_info[i].language.langID[1] << svc.svc_info[i].language.langID[2] << "'";
						if (svc.svc_info[i].digitalCC)
							oss	<< " digital_cc=Y caption_service_number=" << (svc.svc_info[i].csn_line21field & 0x3F);
						else
							oss	<< " digital_cc=N line21_field=" << (svc.svc_info[i].csn_line21field & 0x01)
									<< " easy_reader=" << YESNO(svc.svc_info[i].easyReader)
									<< " wide_aspect_ratio=" << YESNO(svc.svc_info[i].wideAspect);
						LOGMYDEBUG(oss.str());
					}
			}	//	else CDP has ccsvcinfo_section
		}	//	else ccsvcinfo_section exists
	}	//	else (index + size) <= maxPacketSize

	if (pNewIndex)
		*pNewIndex = index;

	if (bResult && pSvc)
		*pSvc = svc;		// struct copy
#if defined(_DEBUG)
	LOGMYDEBUG("mNewServiceInfo Status: " << mNewServiceInfo << endl	<< "mCurrentServiceInfo Status: " << mCurrentServiceInfo);
#endif
	return bResult;

}	//	DebugParseCDPServiceInfo



// DebugParseCDPFutureSection()
//
//		This is a placeholder for any as-yet-undefined CDP sections. According to SMPTE 334-2,
//	all "future" sections will start with a section_id and a length, which allows current code
//	to skip jump over them without throwing off the parsing of subsequent sections.
//	pInData points to the first word of a Caption Data Packet, and pktIndex is the position
//	of the first word of the time_code_section. maxPacketSize is the total length of the
//	Caption Data Packet.
//		This method returns the index of the next word following the section.
//
bool CNTV2CaptionDecoder708::DebugParseCDPFutureSection (const UByte * pInData, size_t pktIndex, size_t maxPacketSize, size_t * pNewIndex, bool & outHasErrors)
{
	bool	bResult	(false);
	size_t	index	(pktIndex);
	size_t	size	(2);			//	"future_sections" have AT LEAST two bytes: a section_id and a length

	UByte	sectionID		(0);
	size_t	sectionLength	(0);

	//	Make sure there is enough data left...
	if ( (index + size) > maxPacketSize)
	{
		LOGMYERROR("Not enough data in CDP (" << maxPacketSize << ") to parse future_section!");
		outHasErrors = true;
	}
	else
	{
		//	The first word must be in the range 0x75 thru 0xEF (inclusive)...
		if (pInData[index] < 0x75 || pInData[index] > 0xef)
		{
			LOGMYERROR("Bad future_section_id 0x" << UHEX2(pInData[index]) << ", expected value between 0x75 and 0xEF");
			outHasErrors = true;
		}
		else
		{
			//	Get section ID...
			sectionID = pInData[index++];

			//	Get the "length" - which is the number of data bytes following the length param...
			sectionLength = pInData[index++];

			//	So the total packet length should be 2 + length...
			index += sectionLength;

			if (index > maxPacketSize)
			{
				LOGMYERROR("Not enough data in CDP (" << maxPacketSize << ") to hold future_section data (" << sectionLength << ")!");
				outHasErrors = true;
			}

			bResult = true;

			//	Print results to console...
			LOGMYDEBUG("future_section:  future_section_id=0x" << UHEX2(sectionID) << " length=" << sectionLength);
		}
	}

	if (pNewIndex != NULL)
		*pNewIndex = index;

	return bResult;

}	//	DebugParseCDPFutureSection



// DebugParseCDPFooter()
//
//		Parse 'n print useful stuff about a CDP cdp_footer (see CEA-708-B, pg 77).
//	pInData points to the first word of a Caption Data Packet, and pktIndex is the position
//	of the first word of the cdp_footer. maxPacketSize is the total length of the
//	Caption Data Packet.
//		This method returns the index of the next word following the section.
//
bool CNTV2CaptionDecoder708::DebugParseCDPFooter (const UByte * pInData, size_t pktIndex, size_t maxPacketSize, size_t * pNewIndex, NTV2_CC708CDPFooterPtr pFtr, bool & outHasErrors)
{
	NTV2_CC708CDPFooter	ftr	= {0, 0};
	bool	bResult	(false);
	size_t	index	(pktIndex);
	size_t	size	(4);			//	There are 4 words in a cdp_footer

	//	Make sure there is enough data left...
	if ((index + size) > maxPacketSize)
	{
		LOGMYERROR("Not enough data in CDP (" << maxPacketSize << ") to parse cdp_footer!");
		outHasErrors = true;
	}
	else
	{
		//	The first word should be 0x74 - the cdp_footer ID...
		if (pInData[index] != NTV2_CC708_CDPFooterId)
		{
			LOGMYERROR("Bad cdp_footer_id 0x" << UHEX2(pInData[index]) << ", expected 0x" << UHEX2(NTV2_CC708_CDPFooterId));
			outHasErrors = true;
		}
		else
		{
			//	Skip past header byte...
			index += 1;

			//	The next two words carry the "sequence counter". This is an error checking mechanism: the SAME two-byte
			//	sequence count should also be inserted in the cdp_header. If it'S NOT the same, it means something
			//	happened mid-packet (like a switch) and the whole packet should be considered wrecked...
			ftr.cdp_ftr_sequence_cntr = (int(pInData[index]) * 256) + int(pInData[index + 1]);
			if (ftr.cdp_ftr_sequence_cntr != mDebugFrameNumber)
			{
				LOGMYERROR("Header Sequence Number " << xHEX0N(mDebugFrameNumber,4) << " doesn't match Footer Sequence Number " << xHEX0N(ftr.cdp_ftr_sequence_cntr,4));
				outHasErrors = true;
			}
			index += 2;

			//	The next byte holds the packet checksum. According to CEA-708, the checksum is "the 8-bit value necessary
			//	to make the arithmetic sum of the entire packet (first byte of cdp_identifier to packet_checksum, inclusive)
			//	modulo 256 equal zero."
			//	Assuming that pInData points to the first byte of the packet, let's do the math...
			UByte sum = 0;
			for (size_t i(0);  i < index;  i++)
				sum += pInData[i];			//	Add up to, but not including, the checksum

			ftr.packet_checksum = pInData[index++];
			if (((int(sum) + int(ftr.packet_checksum)) & 0xFF) != 0)
			{
				LOGMYERROR("packet_checksum error: sent 0x" << UHEX2(ftr.packet_checksum) << ", expected 0x" << UHEX2((256-sum) & 0xFF));
				outHasErrors = true;
			}

			bResult = true;

			//	Print results to console...
			LOGMYDEBUG("cdp_footer: cdp_footer_id=0x" << UHEX2(NTV2_CC708_CDPFooterId) << " cdp_ftr_sequence_cntr=" << xHEX0N(ftr.cdp_ftr_sequence_cntr,4)
						<< " packet_checksum=0x" << UHEX2(ftr.packet_checksum));
		}
	}

	if (pNewIndex)
		*pNewIndex = index;

	if (bResult && pFtr)
		*pFtr = ftr;		// struct copy

	return bResult;

}	//	DebugParseCDPFooter



// DebugParse608CaptionData()
//
// for debug purposes: print a description of the given data
// returns 'true' if something was printed, 'false' if there's nothing worth printing

bool CNTV2CaptionDecoder708::DebugParse608CaptionData (UByte inChar1, UByte inChar2, const int inFieldNum)
{
	//	If both characters are "nulls", don't bother...
	if (inChar1 != 0x80 || inChar2 != 0x80)
	{
		#if defined (_DEBUG)
			//	Remove parity bits...
			const char	ch1	(inChar1 & 0x7F);
			const char	ch2	(inChar2 & 0x7F);

			if (ch1 >= 0x20 && ch2 >= 0x20)		//	If both characters are alphanumeric, print as text
				LOGMYDEBUG("608 Field " << (inFieldNum+1) << " = " << ch1 << " " << ch2);
			else if (ch1 >= 0x20)				//	If only first character is alphanumeric...
				LOGMYDEBUG("608 Field " << (inFieldNum+1) << " = " << ch1 << "  (0x" << UHEX2(ch2) << ")");
			else								//	Print as hex values
				LOGMYDEBUG("608 Field " << (inFieldNum+1) << " = 0x" << UHEX2(ch1) << " 0x" << UHEX2(ch2));
		#endif	//	_DEBUG
		return true;
	}
	return false;

}	//	DebugParse608CaptionData



// DebugParse708CaptionData()
//
// for debug purposes: print a description of the given data
// returns 'true' if something was printed, 'false' if there's nothing worth printing

bool CNTV2CaptionDecoder708::DebugParse708CaptionData (const UByte * data, const size_t byteCount, const bool bHexDump) const
{
	bool	bResult	(true);

	if (byteCount > 0)
	{
		//	Print a single line "hex-dump" style string, e.g.: 0x41 0x42 0x43 0x00 : ABC...
		if (bHexDump && AJADebug::IsActive(AJA_DebugUnit_CC708Decode))
		{
			Log() << "   708 Data = ";
			DumpMemory (data, byteCount, Log(), /*inRadix*/ 16, /*inBytesPerGroup*/ 1, /*inGroupsPerLine*/ 32, /*inAddressRadix*/ 0) << endl;
		}

		//	Now go back and analyze the packet bytes...
		size_t	currIndex	(0);
		while (currIndex < byteCount)
		{
			currIndex = DebugParse708CaptionChannelPacket (data, byteCount, currIndex);

			if (currIndex == 0)		//	currIndex should now point past the end of the packet - if it's zero, something bad happened
				break;
		}
	}
//	else
//		Log () << "    708 byte count = 0?" << endl;

	return bResult;

}	//	DebugParse708CaptionData


// DebugParse708CaptionChannelPacket()
//
// Print useful info about a 708 Caption Channel Packet, which starts at <currIndex> into the data array at <data>.
// Return a new index which points to the byte following the last byte of this packet (i.e. the first byte of the NEXT packet)

size_t CNTV2CaptionDecoder708::DebugParse708CaptionChannelPacket (const UByte * data, const size_t byteCount, size_t currIndex) const
{
	//	Reality check...
	if (data == NULL || currIndex >= byteCount)
	{
		LOGMYERROR("Bad params");
		return 0;
	}

	//	Gather packet info from header (1st byte)...
	const UByte		sequence_number		((data [currIndex] & 0xC0) >> 6);	//	The ms 2 bits = a rotating sequence count (used to detect broken or missing sequences)
	const UByte		packet_size_id		(data [currIndex] & 0x3F);			//	The ls 6 bits are a packet size ID, which translates to bytes as: bytes = 2 * packet_size_id - 1
	const UByte		packetByteCount		((packet_size_id * 2) - 1);			//	Actual byte count (NOT including the Packet Header byte)
	const size_t	endOfPacketIndex	(currIndex + packetByteCount + 1);	//	Index of next byte following this packet

	//	Another reality check: make sure the packet size (including our header byte) doesn't exceed the total data size...
	if (endOfPacketIndex > byteCount)
	{
		LOGMYERROR("Channel packet size (" << packetByteCount << " bytes) exceeds remaining size of data buffer ("
					<< (byteCount - currIndex) << " bytes)");
		return 0;
	}

	//	Assume we have a good header -- print it...
	LOGMYDEBUG("   0x" << UHEX2(data[currIndex]) << ": Packet Hdr, Seq Count = " << sequence_number << ", Size ID = " << packet_size_id << ", Byte Count = " << packetByteCount);
	currIndex++;

	//	Caption channel packets consist of Service Blocks -- find and print all of them...
	while (currIndex < endOfPacketIndex)
	{
		currIndex = DebugParse708ServiceBlock (data, byteCount, currIndex);

		if (currIndex == 0)		//	currIndex should now point past the end of the Service Block -- if it's zero, something bad happened
			return 0;
	}

	return endOfPacketIndex;

}	//	DebugParse708CaptionChannelPacket


// DebugParse708ServiceBlock()
//
// Print useful info about a 708 Caption Service Block, which starts at <currIndex> into the data array at <data>.
// Return a new index which points to the byte following the last byte of this packet (i.e. the first byte of the NEXT Service Block or end of packet)

size_t CNTV2CaptionDecoder708::DebugParse708ServiceBlock (const UByte * data, const size_t byteCount, size_t currIndex) const
{
	//	Reality check...
	if (data == NULL || currIndex >= byteCount)
	{
		LOGMYERROR("Bad params");
		return 0;
	}

	//	Gather Service Block Info from header (1st byte)...
	UByte			service_number			((data [currIndex] & 0xE0) >> 5);	//	The ms 3 bits define a "service number"
	const UByte		block_size				(data [currIndex] & 0x1F);			//	The ls 5 bits are the block size (in bytes, NOT including the Service Block Header)
	const size_t	endOfServiceBlockIndex	(currIndex + block_size + 1);		//	Index of next byte following this Service Block

	//	Take care of the simple case first: NULL Service Block...
	if (service_number == 0 && block_size == 0)
	{
		LOGMYDEBUG("0x00: Null Service Block Header");
		currIndex++;
	}
	else if (service_number == 7)	//	else see if this is an "Extended Service Block" -- if so, the next byte holds the REAL service number
	{
		LOGMYDEBUG("0x" << UHEX2(data[currIndex]) << ": Extended Service Block Header, Block Size = " << block_size << " bytes");

		//	Advance to next byte to get extended service number...
		currIndex++;
		service_number = (data [currIndex] & 0x3F);		//	ls 6 bits of 2nd byte
		LOGMYDEBUG("0x" << UHEX2(data[currIndex]) << ": Extended Service Number = 0x" << UHEX2(service_number));
		currIndex++;
	}
	else	//	else assume it's a Standard Service Block
	{
		LOGMYDEBUG("0x" << UHEX2(data[currIndex]) << ": Standard Service Block, Service = 0x" << UHEX2(service_number) << ", Size = " << block_size);
		currIndex++;
	}

	//	Parse the data bytes within the Service Block...
	while (currIndex < endOfServiceBlockIndex)
	{
		#if 0
			currIndex = DebugParse708Data(data, byteCount, currIndex);
			if (currIndex == 0)		// currIndex should now point to the next data - if it's zero, something bad happened
				return 0;
		#else
			const size_t	cmdLength	(mAvailableServices[0].DebugParse708Command (&data[currIndex], byteCount - currIndex));
			if (cmdLength)
				currIndex += cmdLength;
			else
				return 0;
		#endif
	}

	return endOfServiceBlockIndex;

}	//	DebugParse708ServiceBlock



#if 0		// note: this method has been "pushed down" to the CNTV2Caption708Service level as DebugParse708Command()
// DebugParse708Data()
//
// Print useful info about one or more 708 Caption Data bytes, which start at <currIndex> into the data array at <data>.
// Return a new index which points to the byte following the last byte of this data

int CNTV2CaptionDecoder708::DebugParse708Data (UByte *data, int byteCount, int currIndex)
{
	bool	bErr	(false);

	// reality check...
	if (data == NULL || currIndex >= byteCount)
	{
		LogIf (kCaptionLog_InputErrors)	<< "ERROR: DebugParse708Data() - bad params" << endl;
		return 0;
	}

	// There are four standard "Code Groups" (CL, GL, CR, and GR), determined by ranges of the 8-bit data:
	//
	//		 Data Range  Code		Standard	Extended
	//					 Group		  Set		  Set
	//		0x00 - 0x1F:  CL		  C0		  C2
	//		0x20 - 0x7F:  GL		  G0		  G2
	//		0x80 - 0x9F:  CR		  C1		  C3
	//		0xA0 - 0xFF:  GR		  G1		  G3
	//
	// Each Code Group contains two "Code Sets", "Standard" and "Extended". The Extended Code Sets are accessed by first
	// transmitting an EXT1 byte (0x10). Without this byte, the Standard Code Sets are assumed.

	// See if this is an EXT1 byte
	UByte currByte = data[currIndex];	// shortcut
	bool bExtended = (currByte == 0x10);

	if (!bExtended)
	{
		// Standard Code Set
		if (/*currByte >= 0x00 &&*/ currByte <= 0x1F)
		{
			// Code Set C0
				// This is further broken into 3 sections:
				//		0x00 - 0x0F are 1-byte codes
				//		0x10 - 0x17 are 2-byte codes
				//		0x18 - 0x1F are 3-byte codes
			string	which = "";

			if (/*currByte >= 0x00 &&*/ currByte <= 0x0F)
			{
				// 1-byte codes
				switch (currByte)
				{
					case 0x00:	which = "NUL";		break;
					case 0x01:	which = "???";		break;
					case 0x02:	which = "???";		break;
					case 0x03:	which = "ETX";		break;
					case 0x04:	which = "???";		break;
					case 0x05:	which = "???";		break;
					case 0x06:	which = "???";		break;
					case 0x07:	which = "???";		break;
					case 0x08:	which = "BS";		break;
					case 0x09:	which = "???";		break;
					case 0x0A:	which = "???";		break;
					case 0x0B:	which = "???";		break;
					case 0x0C:	which = "FF";		break;
					case 0x0D:	which = "CR";		break;
					case 0x0E:	which = "HCR";		break;
					case 0x0F:	which = "???";		break;
				}

				LogIf (kCaptionLog_Input708)	<< "         0x" << UHEX2(currByte) << ": C0 Data - [" << which << "]" << endl;
				currIndex += 1;
			}

			else if (currByte >= 0x10 && currByte <= 0x17)
			{
				// 2-byte codes
				switch (currByte)
				{
					case 0x10:	which = "EXT1";		break;		// (note: EXT1 (0x10) is handled in "bExtended" code below)
					case 0x11:	which = "???";		break;
					case 0x12:	which = "???";		break;
					case 0x13:	which = "???";		break;
					case 0x14:	which = "???";		break;
					case 0x15:	which = "???";		break;
					case 0x16:	which = "???";		break;
					case 0x17:	which = "???";		break;
				}

				// is there a second byte?
				if ((currIndex+1) < byteCount)
				{
					UByte currByte2 = data[currIndex+1];		// get 2nd byte
					LogIf (kCaptionLog_Input708)	<< "         0x" << UHEX2(currByte) << " 0x" << UHEX2(currByte2) << ": C0 Data - [" << which << "]  2-byte command" << endl;
				}
				else
				{
					// got an initial 2-byte command but no second byte following - error!
					LogIf (kCaptionLog_InputErrors)	<< "         0x" << UHEX2(currByte) << ": C0 Data - [" << which << "]  Error: 2-byte code prefix with no following byte!" << endl;
					bErr = true;
				}
				currIndex += 2;
			}
			else	//	If (currByte >= 0x18 && currByte <= 0x1F)
			{
				// 3-byte codes
				switch (currByte)
				{
					case 0x18:	which = "P16";		break;
					case 0x19:	which = "???";		break;
					case 0x1A:	which = "???";		break;
					case 0x1B:	which = "???";		break;
					case 0x1C:	which = "???";		break;
					case 0x1D:	which = "???";		break;
					case 0x1E:	which = "???";		break;
					case 0x1F:	which = "???";		break;
				}

				// are there 2nd and 3rd bytes?
				if ((currIndex+2) < byteCount)
				{
					UByte currByte2 = data[currIndex+1];		// get 2nd byte
					UByte currByte3 = data[currIndex+2];		// get 3rd byte
					LogIf (kCaptionLog_Input708)	<< "         0x" << UHEX2(currByte) << " 0x" << UHEX2(currByte2) << " 0x" << UHEX2(currByte3) << ": C0 Data - [" << which << "]  3-byte command" << endl;
				}
				else
				{
					//	Got an initial 3-byte command but no 2nd or 3rd bytes following - error!
					LogIf (kCaptionLog_InputErrors)	<< "         0x" << UHEX2(currByte) << ": C0 Data - [" << which << "]  Error: 3-byte code prefix with no following bytes!" << endl;
					bErr = true;
				}
				currIndex += 3;
			}
		}	// Code Set C0
		else if (currByte >= 0x20 && currByte <= 0x7F)
		{
			// Code Set G0
			LogIf (kCaptionLog_Input708)	<< "         0x" << UHEX2(currByte) << ": G0 Data - '" << currByte << "'" << endl;
			currIndex += 1;

		}	// Code Set G0
		else if (currByte >= 0x80 && currByte <= 0x9F)
		{
				// Code Set C1
			if (currByte >= 0x80 && currByte <= 0x87)
			{
					// SetCurrentWindow command (no extra params)
				int windowID = currByte & 0x07;
				LogIf (kCaptionLog_Input708)	<< "         0x" << UHEX2(currByte) << ": C1 Data - [CW" << windowID << "], Set Current Window = " << windowID << endl;
				currIndex += 1;
			}
			else if (currByte == 0x88)
			{
					// ClearWindows command - next byte is bitmap of windows to clear
				UByte wmap = data[currIndex+1];
				LogIf (kCaptionLog_Input708)	<< "         0x" << UHEX2(currByte) << ": C1 Data - [CLW], ClearWindows 0x" << UHEX2(wmap) << " ("
									<< (wmap & 0x80 ? "7" : " ") << (wmap & 0x40 ? "6" : " ") << (wmap & 0x20 ? "5" : " ") << (wmap & 0x10 ? "4" : " ")
									<< (wmap & 0x08 ? "3" : " ") << (wmap & 0x04 ? "2" : " ") << (wmap & 0x02 ? "1" : " ") << (wmap & 0x01 ? "0" : " ") << ")" << endl;
				currIndex += 2;
			}
			else if (currByte == 0x89)
			{
				// DisplayWindows command - next byte is bitmap of windows to display
				UByte wmap = data[currIndex+1];
				LogIf (kCaptionLog_Input708)	<< "         0x" << UHEX2(currByte) << ": C1 Data - [DSW], DisplayWindows 0x" << UHEX2(wmap) << " ("
									<< (wmap & 0x80 ? "7" : " ") << (wmap & 0x40 ? "6" : " ") << (wmap & 0x20 ? "5" : " ") << (wmap & 0x10 ? "4" : " ")
									<< (wmap & 0x08 ? "3" : " ") << (wmap & 0x04 ? "2" : " ") << (wmap & 0x02 ? "1" : " ") << (wmap & 0x01 ? "0" : " ") << ")" << endl;
				currIndex += 2;
			}
			else if (currByte == 0x8A)
			{
				// HideWindows command - next byte is bitmap of windows to hide
				UByte wmap = data[currIndex+1];
				LogIf (kCaptionLog_Input708)	<< "         0x" << UHEX2(currByte) << ": C1 Data - [HDW], HideWindows 0x" << UHEX2(wmap) << " ("
									<< (wmap & 0x80 ? "7" : " ") << (wmap & 0x40 ? "6" : " ") << (wmap & 0x20 ? "5" : " ") << (wmap & 0x10 ? "4" : " ")
									<< (wmap & 0x08 ? "3" : " ") << (wmap & 0x04 ? "2" : " ") << (wmap & 0x02 ? "1" : " ") << (wmap & 0x01 ? "0" : " ") << ")" << endl;
				currIndex += 2;
			}
			else if (currByte == 0x8B)
			{
				// ToggleWindows command - next byte is bitmap of windows to toggle
				UByte wmap = data[currIndex+1];
				LogIf (kCaptionLog_Input708)	<< "         0x" << UHEX2(currByte) << ": C1 Data - [TGW], ToggleWindows 0x" << UHEX2(wmap) << " ("
									<< (wmap & 0x80 ? "7" : " ") << (wmap & 0x40 ? "6" : " ") << (wmap & 0x20 ? "5" : " ") << (wmap & 0x10 ? "4" : " ")
									<< (wmap & 0x08 ? "3" : " ") << (wmap & 0x04 ? "2" : " ") << (wmap & 0x02 ? "1" : " ") << (wmap & 0x01 ? "0" : " ") << ")" << endl;
				currIndex += 2;
			}
			else if (currByte == 0x8C)
			{
				// DeleteWindows command - next byte is bitmap of windows to delete
				UByte wmap = data[currIndex+1];
				LogIf (kCaptionLog_Input708)	<< "         0x" << UHEX2(currByte) << ": C1 Data - [DLW], DeleteWindows 0x" << UHEX2(wmap) << " ("
									<< (wmap & 0x80 ? "7" : " ") << (wmap & 0x40 ? "6" : " ") << (wmap & 0x20 ? "5" : " ") << (wmap & 0x10 ? "4" : " ")
									<< (wmap & 0x08 ? "3" : " ") << (wmap & 0x04 ? "2" : " ") << (wmap & 0x02 ? "1" : " ") << (wmap & 0x01 ? "0" : " ") << ")" << endl;
				currIndex += 2;
			}
			else if (currByte == 0x8D)
			{
				// Delay command - next byte is delay timeout (in tenths of seconds)
				UByte delay = data[currIndex+1];
				LogIf (kCaptionLog_Input708)	<< "         0x" << UHEX2(currByte) << ": C1 Data - [DLY], Delay = " << delay << "/10 seconds" << endl;
				currIndex += 2;
			}
			else if (currByte == 0x8E)
			{
				// DelayCancel command - no params
				LogIf (kCaptionLog_Input708)	<< "         0x" << UHEX2(currByte) << ": C1 Data - [DLC], DelayCancel" << endl;
				currIndex += 1;
			}
			else if (currByte == 0x8F)
			{
				// Reset command - no params
				LogIf (kCaptionLog_Input708)	<< "         0x" << UHEX2(currByte) << ": C1 Data - [RST], Reset" << endl;
				currIndex += 1;
			}
			else if (currByte == 0x90)
			{
				// SetPenAttributes command - next 2 bytes are params
				UByte parm1 = data[currIndex+1];
				UByte parm2 = data[currIndex+2];
				LogIf (kCaptionLog_Input708)	<< "         0x" << UHEX2(currByte) << ": C1 Data - [SPA], SetPenAttributes" << endl
									<< "                                  text tag   = " << ((parm1 & 0xF0) >> 4) << endl
									<< "                                  offset     = " << ((parm1 & 0x0C) >> 2) << endl
									<< "                                  pen size   = " << ((parm1 & 0x03)     ) << endl
									<< "                                  italics    = " << ((parm2 & 0x80) >> 7) << endl
									<< "                                  underline  = " << ((parm2 & 0x40) >> 6) << endl
									<< "                                  edge type  = " << ((parm2 & 0x38) >> 3) << endl
									<< "                                  font style = " << ((parm2 & 0x07)     ) << endl;
				currIndex += 3;
			}
			else if (currByte == 0x91)
			{
				// SetPenColor command - next 3 bytes are params
				UByte parm1 = data[currIndex+1];
				UByte parm2 = data[currIndex+2];
				UByte parm3 = data[currIndex+3];
				LogIf (kCaptionLog_Input708)	<< "         0x" << UHEX2(currByte) << ": C1 Data - [SPC], SetPenColor" << endl
									<< "                                  fg opacity = " << ((parm1 & 0xC0) >> 6) << endl
									<< "                                  fg r/g/b   = " << ((parm1 & 0x30) >> 4) << "/" << ((parm1 & 0x0C) >> 2) << "/" << (parm1 & 0x03) << endl
									<< "                                  bg opacity = " << ((parm2 & 0xC0) >> 6) << endl
									<< "                                  bg r/g/b   = " << ((parm2 & 0x30) >> 4) << "/" << ((parm2 & 0x0C) >> 2) << "/" << (parm2 & 0x03) << endl
									<< "                                  edge r/g/b = " << ((parm3 & 0x30) >> 4) << "/" << ((parm3 & 0x0C) >> 2) << "/" << (parm3 & 0x03) << endl;
				currIndex += 4;
			}
			else if (currByte == 0x92)
			{
				// SetPenLocation command - next 2 bytes are params
				UByte parm1 = data[currIndex+1];
				UByte parm2 = data[currIndex+2];
				LogIf (kCaptionLog_Input708)	<< "         0x" << UHEX2(currByte) << ": C1 Data - [SPL], SetPenLocation, row = " << (parm1 & 0x0F) << ", column = " << (parm2 & 0x3F) << endl;
				currIndex += 3;
			}
			else if (currByte == 0x97)
			{
				// SetWindowAttributes command - next 4 bytes are params
				UByte parm1 = data[currIndex+1];
				UByte parm2 = data[currIndex+2];
				UByte parm3 = data[currIndex+3];
				UByte parm4 = data[currIndex+4];
				LogIf (kCaptionLog_Input708)	<< "         0x" << UHEX2(currByte) << ": C1 Data - [SWA], SetWindowAttributes" << endl
									<< "                                  fill opacity = " << ((parm1 & 0xC0) >> 6) << endl
									<< "                                  fill r/g/b   = " << ((parm1 & 0x30) >> 4) << "/" << ((parm1 & 0x0C) >> 2) << "/" << (parm1 & 0x03) << endl
									<< "                                  border type  = " << (((parm3 & 0x80) >> 5) + ((parm2 & 0xC0) >> 6)) << endl
									<< "                                  border r/g/b = " << ((parm2 & 0x30) >> 4) << "/" << ((parm2 & 0x0C) >> 2) << "/" << (parm2 & 0x03) << endl
									<< "                                  word wrap    = " << ((parm3 & 0x40) >> 6) << endl
									<< "                                  print dir    = " << ((parm3 & 0x30) >> 4) << endl
									<< "                                  scroll dir   = " << ((parm3 & 0x0C) >> 2) << endl
									<< "                                  justify      = " << ((parm3 & 0x03)     ) << endl
									<< "                                  effect speed = " << ((parm4 & 0xF0) >> 4) << endl
									<< "                                  effect dir   = " << ((parm4 & 0x0C) >> 2) << endl
									<< "                                  disp effect  = " << ((parm4 & 0x03)     ) << endl;
				currIndex += 5;
			}
			else if (currByte >= 0x98 && currByte <= 0x9F)
			{
				// DefineWindow command - next 6 bytes are params
				int win = currByte & 0x07;
				UByte parm1 = data[currIndex+1];
				UByte parm2 = data[currIndex+2];
				UByte parm3 = data[currIndex+3];
				UByte parm4 = data[currIndex+4];
				UByte parm5 = data[currIndex+5];
				UByte parm6 = data[currIndex+6];
				LogIf (kCaptionLog_Input708)	<< "         0x" << UHEX2(currByte) << ": C1 Data - [DF" << win << "], DefineWindow" << win << endl
									<< "                                  visible   = " << ((parm1 & 0x20) >> 5) << endl
									<< "                                  row lock  = " << ((parm1 & 0x10) >> 4) << endl
									<< "                                  col lock  = " << ((parm1 & 0x08) >> 3) << endl
									<< "                                  priority  = " << ((parm1 & 0x07)     ) << endl
									<< "                                  rel pos   = " << ((parm2 & 0x80) >> 7) << endl
									<< "                                  anchor V  = " << ((parm2 & 0x7F)     ) << endl
									<< "                                  anchor H  = " << ((parm3 & 0xFF)     ) << endl
									<< "                                  anchor pt = " << ((parm4 & 0xF0) >> 4) << endl
									<< "                                  row count = " << ((parm4 & 0x0F)     ) << " (+1)" << endl
									<< "                                  col count = " << ((parm5 & 0x3F)     ) << " (+1)" << endl
									<< "                                  wnd style = " << ((parm6 & 0x38) >> 3) << endl
									<< "                                  pen style = " << ((parm6 & 0x07)     ) << endl;
				currIndex += 7;
			}
			else	// ??? undefined ???
			{
				LogIf (kCaptionLog_Input708)	<< "         0x" << UHEX2(currByte) << ": C1 Data - [???]" << endl;
				currIndex += 1;
			}
		}	// Code Set C1
		else  // (currByte >= 0xA0 && currByte <= 0xFF)
		{
			// Code Set G1 (consists of non-ASCII characters we can't print on the Console...)
			LogIf (kCaptionLog_Input708)	<< "         0x" << UHEX2(currByte) << ": G1 Data" << endl;
			currIndex += 1;

		}	// Code Set G1
	}	// if (!bExtended)
	else
	{
		// Extended Code Set (the first byte was 'EXT1' (0x10)
		// The specific command (and length) now depends on the second byte
		string	which = "EXT1";

		// Make sure there IS a second byte!
		if ((currIndex+1) < byteCount)
		{
			string	which2 = "???";
			UByte currByte2 = data[currIndex+1];		// get 2nd byte

			if (/*currByte2 >= 0x00 && */ currByte2 <= 0x1f)		// C2 Code Set - the length depends on the 2nd byte
			{
				if (/*currByte2 >= 0x00 && */ currByte2 <= 0x07)		// 2-byte codes (EXT1 + currByte2)
				{
					LogIf (kCaptionLog_Input708)	<< "         0x" << UHEX2(currByte) << " 0x" << UHEX2(currByte2) << ": C2 Data - [" << which << "], ???  (2-byte code)" << endl;
					currIndex += 2;
				}
				else if (currByte2 >= 0x08 && currByte2 <= 0x0f)		// 3-byte codes (EXT1 + currByte2 + 1 data byte)
				{
					// 3-byte command - make sure there is enough data
					if ((currIndex + 2) < byteCount)
						{LogIf (kCaptionLog_Input708)	<< "         0x" << UHEX2(currByte) << " 0x" << UHEX2(currByte2) << " 0x" << UHEX2(data[currIndex+2]) << ": C2 Data - [" << which << "], ??? ???  (3-byte code)" << endl;}
					else
						{LogIf (kCaptionLog_InputErrors)	<< "         0x" << UHEX2(currByte) << " 0x" << UHEX2(currByte2) << ": Error - EXT1 3-byte command received without enough data" << endl;}
					currIndex += 3;
				}
				else if (currByte2 >= 0x10 && currByte2 <= 0x17)		// 4-byte codes (EXT1 + currByte2 + 2 data bytes)
				{
					// 4-byte command - make sure there is enough data
					if ((currIndex + 3) < byteCount)
						{LogIf (kCaptionLog_Input708)	<< "         0x" << UHEX2(currByte) << " 0x" << UHEX2(currByte2) << " 0x" << UHEX2(data[currIndex+2]) << " 0x" << UHEX2(data[currIndex+3]) << ": C2 Data - [" << which << "], ??? ??? ??? (4-byte code)" << endl;}
					else
						{LogIf (kCaptionLog_InputErrors)	<< "         0x" << UHEX2(currByte) << " 0x" << UHEX2(currByte2) << ": Error - EXT1 4-byte command received without enough data" << endl;}
					currIndex += 4;
				}
				else /* (currByte2 >= 0x18 && currByte2 <= 0x1f) */		// 5-byte codes (EXT1 + currByte2 + 3 data bytes)
				{
					// 5-byte command - make sure there is enough data
					if ((currIndex + 4) < byteCount)
						{LogIf (kCaptionLog_Input708)	<< "         0x" << UHEX2(currByte) << " 0x" << UHEX2(currByte2) << " 0x" << UHEX2(data[currIndex+2]) << " 0x" << UHEX2(data[currIndex+3]) << " 0x" << UHEX2(data[currIndex+4]) << ": C2 Data - [" << which << "], ??? ??? ??? ??? (5-byte code)" << endl;}
					else
						{LogIf (kCaptionLog_InputErrors) << "         0x" << UHEX2(currByte) << " 0x" << UHEX2(currByte2) << ": Error - EXT1 5-byte command received without enough data" << endl;}
					currIndex += 5;
				}
			}
			else if (currByte2 >= 0x20 && currByte2 <= 0x7f)		// G2 Code Set - print the ones we know about...
			{
				switch (currByte2)
				{
					case 0x20:	which2 = "[TSP]";	break;
					case 0x21:	which2 = "[NBTSP]";	break;
					case 0x25:	which2 = "...";		break;
					case 0x2a:	which2 = "S";		break;
					case 0x2c:	which2 = "OE";		break;
					case 0x30:	which2 = "[Block]";	break;
					case 0x31:	which2 = "`";		break;
					case 0x32:	which2 = "'";		break;
					case 0x33:	which2 = "\"";		break;
					case 0x34:	which2 = "\"";		break;
					case 0x35:	which2 = ".";		break;
					case 0x39:	which2 = "TM";		break;
					case 0x3a:	which2 = "s";		break;
					case 0x3c:	which2 = "oe";		break;
					case 0x3d:	which2 = "SM";		break;
					case 0x3f:	which2 = "Y";		break;

					case 0x76:	which2 = "1/8";		break;
					case 0x77:	which2 = "3/8";		break;
					case 0x78:	which2 = "5/8";		break;
					case 0x79:	which2 = "7/8";		break;
					case 0x7a:	which2 = "|";		break;
					case 0x7b:	which2 = "-|";		break;
					case 0x7c:	which2 = "|_";		break;
					case 0x7d:	which2 = "_";		break;
					case 0x7e:	which2 = "_|";		break;
					case 0x7f:	which2 = "|-";		break;
				}

				LogIf (kCaptionLog_Input708)	<< "         0x" << UHEX2(currByte) << " 0x" << UHEX2(currByte2) << ": G2 Data - [" << which << "], " << which2 << "  (2-byte code)" << endl;
				currIndex += 2;
			}
			else if (currByte2 >= 0x80 && currByte2 <= 0x9f)		// C3 Code Set - the length depends on the 2nd byte
			{
				if (currByte2 >= 0x80 && currByte2 <= 0x87)		// 6-byte codes (EXT1 + currByte2 + 4 data bytes)
				{
					// 6-byte command - make sure there is enough data
					if ((currIndex + 5) < byteCount)
						{LogIf (kCaptionLog_Input708) << "         0x" << UHEX2(currByte) << " 0x" << UHEX2(currByte2) << " 0x" << UHEX2(data[currIndex+2]) << " 0x" << UHEX2(data[currIndex+3]) << " 0x" << UHEX2(data[currIndex+4]) << " 0x" << UHEX2(data[currIndex+5]) << ": C3 Data - [" << which << "], ??? ??? ??? ??? ??? (6-byte code)" << endl;}
					else
						{LogIf (kCaptionLog_InputErrors)	<< "         0x" << UHEX2(currByte) << " 0x" << UHEX2(currByte2) << ": Error - EXT1 6-byte command received without enough data" << endl;}
					currIndex += 6;
				}
				else if (currByte2 >= 0x88 && currByte2 <= 0x8f)		// 7-byte codes (EXT1 + currByte2 + 5 data bytes)
				{
					// 7-byte command - make sure there is enough data
					if ((currIndex + 6) < byteCount)
						{LogIf (kCaptionLog_Input708) << "         0x" << UHEX2(currByte) << " 0x" << UHEX2(currByte2) << " 0x" << UHEX2(data[currIndex+2]) << " 0x" << UHEX2(data[currIndex+3]) << " 0x" << UHEX2(data[currIndex+4]) << " 0x" << UHEX2(data[currIndex+5]) << " 0x" << UHEX2(data[currIndex+6]) << ": C2 Data - [" << which << "], ??? ??? ??? ??? ??? ??? (7-byte code)" << endl;}
					else
						{LogIf (kCaptionLog_InputErrors)	<< "         0x" << UHEX2(currByte) << " 0x" << UHEX2(currByte2) << ": Error - EXT1 7-byte command received without enough data" << endl;}
					currIndex += 7;
				}
				else /* (currByte2 >= 0x90 && currByte2 <= 0x9f) */		// variable-length commands
				{
					// what a nightmare...
					// the third byte contains the command header, which holds the "type" code and the length
					// make sure there is a third byte...
					int cmdLength = 2;		// we know we have at least 2 bytes so far...
					int cmdType = 0;		//

					if ((currIndex + 3) < byteCount)
					{
						cmdLength =  data[currIndex+2] & 0x1f;
						cmdType   = (data[currIndex+2] & 0xc0) >> 6;

						// see if we have the prescribed number of bytes left
						if ((currIndex + 3 + cmdLength) < byteCount)
							{LogIf (kCaptionLog_Input708)	<< "         0x" << UHEX2(currByte) << " 0x" << UHEX2(currByte2) << " 0x" << UHEX2(data[currIndex+2]) << ": C3 Data - [" << which << "], ??? ??? variable length (type " << cmdType << ": " << cmdLength << " data bytes) code" << endl;}
						else
							{LogIf (kCaptionLog_InputErrors)	<< "         0x" << UHEX2(currByte) << " 0x" << UHEX2(currByte2) << " 0x" << UHEX2(data[currIndex+2]) << ": Error - EXT1 variable-length (" << cmdLength << " data bytes) command received without enough data";}
					}
					else
						{LogIf (kCaptionLog_InputErrors)	<< "         0x" << UHEX2(currByte) << " 0x" << UHEX2(currByte2) << ": Error - EXT1 7-byte command received without enough data" << endl;}
					currIndex += cmdLength;
				}
			}
			else /* (currByte2 >= 0xa0 && currByte2 <= 0xff) */		// G3 Code Set - print the one(s) we know about...
			{
				switch (currByte2)
				{
					case 0xa0:	which2 = "CC";		break;
				}

				LogIf (kCaptionLog_Input708)	<< "         0x" << UHEX2(currByte) << " 0x" << UHEX2(currByte2) << ": G3 Data - [" << which << "], " << which2 << "  (2-byte code)" << endl;
				currIndex += 2;
			}
		}
		else
		{
			// got an EXT1 code but no second byte following - error!
			LogIf (kCaptionLog_InputErrors)	<< "         0x" << UHEX2(currByte) << ": Error - EXT1 received with no following byte!" << endl;
			currIndex += 1;
		}
	}

	return currIndex;
}

#endif	//	0

#endif	//	_DEBUG


// ClearAllCaptionChannelPacketInfo()
//		Clear ALL CaptionChannelPacketInfo structs in mCCPInfoArray
//
void CNTV2CaptionDecoder708::ClearAllCaptionChannelPacketInfo (void)
{
	for (int i = 0;  i < kMaxNumCaptionChannelPacketInfo;  i++)
		ClearCaptionChannelPacketInfo(&mCCPInfoArray[i]);

	mCurrentCCPIndex = 0;

}	//	ClearAllCaptionChannelPacketInfo


// ClearCaptionChannelPacketInfo()
//		Clear a designated CaptionChannelPacketInfo struct
//
void CNTV2CaptionDecoder708::ClearCaptionChannelPacketInfo (CaptionChannelPacketInfoPtr pCCPInfo)
{
	pCCPInfo->ccpStatus		 = kCaptionChannelPacketClear;
	pCCPInfo->ccpSize		 = 0;
	pCCPInfo->ccpSequenceNum = 0;
	pCCPInfo->ccpCurrentSize = 0;

	for (size_t ndx = 0;  ndx < NTV2_CC708_MaxCaptionChannelPacketSize;  ndx++)
		pCCPInfo->ccpData[ndx] = 0;

}	//	ClearCaptionChannelPacketInfo


// GetNumCompleteCaptionChannelPackets()
//	Count the number of "complete" CaptionChannelPacketInfo structs we have in our array.
//	Note: we're assuming the array is "clean" - that all complete structs are in the lowest numbered indices.
//
int CNTV2CaptionDecoder708::GetNumCompleteCaptionChannelPacketInfo (int * pCompleteCount, int * pStartedCount)
{
	int result = 0;

	for (int i = 0;  i < kMaxNumCaptionChannelPacketInfo;  i++)
	{
		if (mCCPInfoArray[i].ccpStatus == kCaptionChannelPacketComplete)
			result++;
		else
			break;		//	Once we get to a "non-complete" struct we can stop counting
	}

	if (pCompleteCount)
		*pCompleteCount = result;

	//	Report whether the CCP immediately following the last "Complete" struct has started but not completed...
	if (pStartedCount)
		*pStartedCount = (mCCPInfoArray[result].ccpStatus == kCaptionChannelPacketStarted) ? 1 : 0;

	return result;

}	//	GetNumCompleteCaptionChannelPacketInfo


// ResetCaptionChannelPacketInfoForNewCDP()
//	Before start of a new CDP, delete any "complete" Caption Channel Packets left over from the last CDP,
//	and move (copy) any "started" (but incomplete) Caption Channel Packet to the first slot.
//	Note: assumes array is "clean", that is all non-clear CaptionChannelPacketInfo are down in the lowest indices.
//
void CNTV2CaptionDecoder708::ResetCaptionChannelPacketInfoForNewCDP (void)
{
	int startErase = 0;
	int endErase   = 0;

	//	How many CCPs are "complete"? they can be cleared...
	int numComplete = GetNumCompleteCaptionChannelPacketInfo();

	//	If we have a "started but not complete" CCP (and at most we'll only have one),
	//	then it will be immediately following the completed ones...
	if ((numComplete < kMaxNumCaptionChannelPacketInfo)  &&  (mCCPInfoArray[numComplete].ccpStatus == kCaptionChannelPacketStarted))
	{
		//	Yes, the last one is started but not complete. We'll want to copy this to the first slot
		//	so we can continue adding new data to it in the coming frame
		if (numComplete > 0)
		{
			mCCPInfoArray[0] = mCCPInfoArray [numComplete];		// struct copy

			startErase = 1;
			endErase   = numComplete + 1;	// be sure to clear the old copy of the incomplete CCP!
		}
	}
	else
	{
		//	No in-progress CCPs -- just erase the complete ones...
		startErase = 0;
		endErase   = numComplete;
	}

	//	Erase the "old" CCPs...
	for (int i(startErase);  i < endErase;  i++)
		ClearCaptionChannelPacketInfo(&mCCPInfoArray[i]);

	//	I'll always start using index 0 -- whether it's clear or in-progress...
	mCurrentCCPIndex = 0;

}	//	ResetCaptionChannelPacketInfoForNewCDP


// StartNewCaptionChannelPacketInfo()
//
void CNTV2CaptionDecoder708::StartNewCaptionChannelPacketInfo (int seqNum, int expectedSize)
{
	//	Sanity check...
	if (mCurrentCCPIndex < size_t(kMaxNumCaptionChannelPacketInfo))
	{
		//	If the current CCP is not clear -- close it and start a new one...
		CloseCurrentCaptionChannelPacketInfo();

		if (mCurrentCCPIndex < size_t(kMaxNumCaptionChannelPacketInfo))
		{
			mCCPInfoArray[mCurrentCCPIndex].ccpStatus		= kCaptionChannelPacketStarted;
			mCCPInfoArray[mCurrentCCPIndex].ccpSequenceNum	= seqNum;
			mCCPInfoArray[mCurrentCCPIndex].ccpSize			= expectedSize;
			mCCPInfoArray[mCurrentCCPIndex].ccpCurrentSize	= 0;
		}
		else
			{LOGMYERROR("out of CaptionChannelPacketInfo structs! " << mCurrentCCPIndex);}
	}
	else
		{LOGMYERROR("Invalid CaptionChannelPacketInfo index: " <<  mCurrentCCPIndex);}

}	//	StartNewCaptionChannelPacketInfo


// CloseCurrentCaptionChannelPacketInfo()
//
void CNTV2CaptionDecoder708::CloseCurrentCaptionChannelPacketInfo (void)
{
	//	Sanity check...
	if (mCurrentCCPIndex < size_t(kMaxNumCaptionChannelPacketInfo))
	{
		CaptionChannelPacketInfoPtr pCCPInfo = &mCCPInfoArray[mCurrentCCPIndex];

		//	If it was never open, don't bother...
		if (pCCPInfo->ccpStatus != kCaptionChannelPacketClear)
		{
			pCCPInfo->ccpStatus = kCaptionChannelPacketComplete;

			mCurrentCCPIndex++;		//	Bump index to next
		}
	}
	else
		{LOGMYERROR("Invalid CaptionChannelPacketInfo index: " << mCurrentCCPIndex);}

}	//	CloseCurrentCaptionChannelPacketInfo



// AddCaptionChannelPacketInfoData()
//
bool CNTV2CaptionDecoder708::AddCaptionChannelPacketInfoData (UByte newData)
{
	if (mCurrentCCPIndex < size_t(kMaxNumCaptionChannelPacketInfo))
	{
		CaptionChannelPacketInfoPtr pCCPInfo = &mCCPInfoArray[mCurrentCCPIndex];

		if (pCCPInfo->ccpStatus == kCaptionChannelPacketStarted)
		{
			if (pCCPInfo->ccpCurrentSize < int(NTV2_CC708_MaxCaptionChannelPacketSize))	//	** MrBill **	FIX
			{
				if (pCCPInfo->ccpCurrentSize < pCCPInfo->ccpSize)
				{
					pCCPInfo->ccpData[pCCPInfo->ccpCurrentSize] = newData;
					pCCPInfo->ccpCurrentSize++;

					//	Last byte?
					if (pCCPInfo->ccpCurrentSize >= pCCPInfo->ccpSize)
						CloseCurrentCaptionChannelPacketInfo();

					return true;	//	Success!
				}
				else
					{LOGMYERROR("CaptionChannelPacketInfo[" << mCurrentCCPIndex << "] has exceeded size " << pCCPInfo->ccpSize);}
			}
			else
				{LOGMYERROR("CaptionChannelPacketInfo[" << mCurrentCCPIndex << "] is full!");}
		}
		else
			{LOGMYERROR("CaptionChannelPacketInfo[" << mCurrentCCPIndex << "] is not started! (status = " << pCCPInfo->ccpStatus << ")");}
	}
	else
		{LOGMYERROR("Invalid CaptionChannelPacketInfo index: " << mCurrentCCPIndex);}

	return false;	//	Fail

}	//	AddCaptionChannelPacketInfoData


// GetCaptionChannelPacketInfoData()
//		Returns pointer to the Caption Channel Packet at [index] aong with the size of the data (in bytes)
//
bool CNTV2CaptionDecoder708::GetCaptionChannelPacketInfoData (int index, UByte **ppData, int *pSize)
{
	if (index >= 0  &&  index < kMaxNumCaptionChannelPacketInfo  &&  ppData  &&  pSize)
	{
		*ppData = mCCPInfoArray[index].ccpData;
		*pSize  = mCCPInfoArray[index].ccpCurrentSize;
		return true;
	}
	return false;

}	//	GetCaptionChannelPacketInfoData


// ParseAllCaptionChannelPackets()
//		Assuming that our array of Caption Channel Packets has been loaded from the incoming CDP ( ParseSMPTE334AncPacket() ),
//	break each CCP down to its constituent ServiceBlocks and send them to their respective services.
//
bool CNTV2CaptionDecoder708::ParseAllCaptionChannelPackets (void)
{
	//	Loop through the caption channel packets until we either: a) run out;
	//	or b) come across an incomplete CCP (assumes all complete CCPs live
	//	at the "bottom" of the array...
	for (int i(0);  i < kMaxNumCaptionChannelPacketInfo  &&  mCCPInfoArray[i].ccpStatus == kCaptionChannelPacketComplete;  i++)
		ParseCaptionChannelPacket(&mCCPInfoArray[i]);
	return true;

}	//	ParseAllCaptionChannelPackets


// ParseCaptionChannelPacket()
//
bool CNTV2CaptionDecoder708::ParseCaptionChannelPacket (CaptionChannelPacketInfoPtr pCCPInfo)
{
	bool	bResult	(true);

	//	Sanity check...
	if (pCCPInfo  &&  (pCCPInfo->ccpSize > 0))
	{
		size_t	ccpSize		= size_t(pCCPInfo->ccpSize);	//	Total number of bytes to parse
		size_t	index		= 1;							//	Start by skipping the CCP header byte
		bool	bDone		= false;
		UByte *	pCCPData	= pCCPInfo->ccpData;

		while (!bDone && (index < ccpSize))
		{
			//	Index should be pointing at a Service Block Header.
			//	This can be one of three types:
			//		- a NULL Header (which we throw away and move on)
			//		- a Standard Service Block Header (1 byte)
			//		- an Extended Service Block Header (2 bytes)
			//	If it's one of the latter two, we need to get the Service number and the
			//	Service Block length, and then call the appropriate Service object to parse it.

			int		serviceNum = (pCCPData[index] & 0xE0) >> 5;		//	The service number is in the ms 3 bits of the Service Block Header
			size_t	blockSize  = (pCCPData[index] & 0x1F);			//	The Block Size is in the ls 5 bits of the Service Block Header
			if (serviceNum == 0)
			{
				//	It's a NULL Service Block - discard
				//	According to CEA-708, a NULL Service Block signifies the end of a Caption Channel Packet
				blockSize = 0;
				index += 1;
				bDone = true;
			}
			else
			{
				//	It's either a Standard or an Extended
				if (serviceNum == 7)
				{
					//	It's an Extended Service Block: the REAL Service Number is in the 2nd byte
					index += 1;
					if (index < ccpSize)
						serviceNum = (pCCPData[index] & 0x3F);	//	The Extended Service Number is in the ls 6 bits
					else
						bDone = true;
				}

				index += 1;

				//	Index now points to the first data word ("command") of the Service Block.
				//	Make sure we have enough data left in the CCP to accommodate the (alleged) number of data bytes in the Service Block.
				if ((index + blockSize) <= ccpSize)
				{
					if (serviceNum < NTV2_CC708MaxNumServices)
						mAvailableServices[serviceNum].ParseInputServiceBlockToLocalQueue (&pCCPData[index], blockSize);
				}

				//	Index should now point to the first byte of the next Service Block...
				index += blockSize;
			}
		}
	}

	return bResult;

}	//	ParseCaptionChannelPacket


// GetNextServiceBlockInfoFromQueue()
//
//	Returns the Block Size, Data Size, and Service Number of the next Service Block on the queue (returns false if empty).
//	This is a "peek" operation - the service block is left on the queue.
//
bool CNTV2CaptionDecoder708::GetNextServiceBlockInfoFromQueue (const size_t svcIndex, size_t & outBlockSize, size_t & outDataSize, int & outServiceNum, bool & outIsExtended) const
{
	bool	bResult	(false);

	//	Sanity check...
	if (svcIndex < size_t(NTV2_CC708MaxNumServices))
		bResult = mAvailableServices[svcIndex].PeekNextServiceBlockInfo (outBlockSize, outDataSize, outServiceNum, outIsExtended);
	return bResult;

}	//	GetNextServiceBlockInfoFromQueue


// GetNextServiceBlockFromQueue()
//
//	Copies the next Service Block from the queue to the designated pointer (also "pops" queue element).
//	This method copies the entire Service Block - to copy only the data, call GetNextServiceBlockDataFromQueue().
//	Returns the size of the copied data.
//
size_t CNTV2CaptionDecoder708::GetNextServiceBlockFromQueue (const size_t svcIndex, vector<UByte> & outData)
{
	//	Sanity check...
	if (svcIndex < size_t(NTV2_CC708MaxNumServices))
		return mAvailableServices[svcIndex].PopServiceBlock(outData);

	return 0;

}	//	GetNextServiceBlockFromQueue


size_t CNTV2CaptionDecoder708::GetNextServiceBlockFromQueue (const size_t svcIndex, UByte * pOutDataBuffer)
{
	//	Sanity check...
	if (svcIndex < size_t(NTV2_CC708MaxNumServices))
		return mAvailableServices[svcIndex].PopServiceBlock(pOutDataBuffer);

	return 0;

}	//	GetNextServiceBlockFromQueue


// GetNextServiceBlockDataFromQueue()
//
//	Copies the next Service Block (data only) from the queue to the designated pointer (also "pops" queue element).
//	This method only copies the Service Block payload - to copy the entire Service Block, call GetNextServiceBlockFromQueue().
//	Returns the size of the copied data.
//
size_t CNTV2CaptionDecoder708::GetNextServiceBlockDataFromQueue (const size_t svcIndex, vector<UByte> & outData)
{
	//	Sanity check...
	outData.clear();
	if (svcIndex < size_t(NTV2_CC708MaxNumServices))
		return mAvailableServices[svcIndex].PopServiceBlockData(outData);
	return 0;

}	//	GetNextServiceBlockDataFromQueue

size_t CNTV2CaptionDecoder708::GetNextServiceBlockDataFromQueue (const size_t svcIndex, UByte * pOutDataBuffer)
{
	//	Sanity check...
	if (svcIndex < size_t(NTV2_CC708MaxNumServices))
		return mAvailableServices[svcIndex].PopServiceBlockData(pOutDataBuffer);
	return 0;

}	//	GetNextServiceBlockDataFromQueue



//-------- Debug --------

// SetDebugLevel()
//		Determines what kind of debug printf's get sent to console
//
NTV2CaptionLogMask CNTV2CaptionDecoder708::SetLogMask (const NTV2CaptionLogMask inLogMask)
{
	mCurrentServiceInfo.SetLogMask(inLogMask);
	mNewServiceInfo.SetLogMask(inLogMask);
	for (int i = 0;  i < NTV2_CC708MaxNumServices;  i++)
		mAvailableServices [i].SetLogMask(inLogMask);
	return CNTV2CaptionLogConfig::SetLogMask(inLogMask);
}


CNTV2CaptionDecoder708::CNTV2CaptionDecoder708 (const CNTV2CaptionDecoder708 & inDecoderToCopy)
	:	CNTV2CaptionLogConfig()
{
	(void) inDecoderToCopy;
	gInstanceTally++;
	AJACC_ASSERT(false);
}


CNTV2CaptionDecoder708 & CNTV2CaptionDecoder708::operator = (const CNTV2CaptionDecoder708 & inDecoderToCopy)
{
	(void) inDecoderToCopy;
	AJACC_ASSERT(false);
	return *this;
}
