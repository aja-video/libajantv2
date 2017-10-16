/**
	@file		main.cpp
	@brief		Basic Functionality Tests for the AJA Anc Library.
	@copyright	Copyright (c) 2013-2015 AJA Video Systems, Inc. All rights reserved.
**/
//#define	DEBUG_BREAK_AFTER_FAILURE	(true)
#include "ntv2bft.h"
#include "ancillarydata_cea608_line21.h"
#include "ancillarydata_cea608_vanc.h"
#include "ancillarydata_cea708.h"
#include "ancillarylist.h"
#include "ntv2smpteancdata.h"
#include "ntv2captionlogging.h"
#include <iomanip>
#if defined(AJA_LINUX)
	#include <string.h>
#endif

using namespace std;


	#define	SHOULD_SUCCEED(_x_)			do																															\
										{																															\
											const AJAStatus __err__ (_x_);																							\
											if (AJA_FAILURE (__err__))																								\
											{																														\
												STDERR	<< "## ERROR:  '" << __FUNCTION__ << "' failed at line " << __LINE__ << " of " << __FILE__ << ":" << ENDL	\
														<< "           Expected 'AJA_STATUS_SUCCESS' or 'AJA_STATUS_TRUE' from '" << #_x_ << "'" << ENDL			\
														<< "           Instead got '" << ::AJAStatusToString (__err__) << "'"  << ENDL;								\
												if (STOP_AFTER_FAILURE)																								\
												{																													\
													if (DEBUG_BREAK_AFTER_FAILURE)																					\
														DEBUG_BREAK ();																								\
													return false;																									\
												}																													\
											}																														\
											else if (SHOW_PASSED)																									\
												STDOUT	<< "## NOTE:  '" << #_x_ << "' in '" << __FUNCTION__ << "' returned '" << ::AJAStatusToString (__err__)		\
														<< "'" << ENDL;																								\
										} while (false)


	#define	SHOULD_FAIL(_x_)			do																															\
										{																															\
											const AJAStatus __err__ (_x_);																							\
											if (AJA_SUCCESS (__err__))																								\
											{																														\
												STDERR	<< "## ERROR:  '" << __FUNCTION__ << "' failed at line " << __LINE__ << " of " << __FILE__ << ":" << ENDL	\
														<< "           Expected 'AJA_FAILURE' from '" << #_x_ << "'" << ENDL										\
														<< "           Instead got '" << ::AJAStatusToString (__err__) << "'" << ENDL;								\
												if (STOP_AFTER_FAILURE)																								\
												{																													\
													if (DEBUG_BREAK_AFTER_FAILURE)																					\
														DEBUG_BREAK ();																								\
													return false;																									\
												}																													\
											}																														\
											else if (SHOW_PASSED)																									\
												STDOUT	<< "## NOTE:  '" << #_x_ << "' in '" << __FUNCTION__ << "' returned '" << ::AJAStatusToString (__err__)		\
														<< "'" << ENDL;																								\
										} while (false)


static string AJAStatusToString (const AJAStatus inStatus)
{
	switch (inStatus)
	{
		case AJA_STATUS_SUCCESS:			return "AJA_STATUS_SUCCESS";
		case AJA_STATUS_TRUE:				return "AJA_STATUS_TRUE";
		case AJA_STATUS_UNKNOWN:			return "AJA_STATUS_UNKNOWN";
		case AJA_STATUS_FAIL:				return "AJA_STATUS_FAIL";
		case AJA_STATUS_TIMEOUT:			return "AJA_STATUS_TIMEOUT";
		case AJA_STATUS_RANGE:				return "AJA_STATUS_RANGE";
		case AJA_STATUS_INITIALIZE:			return "AJA_STATUS_INITIALIZE";
		case AJA_STATUS_NULL:				return "AJA_STATUS_NULL";
		case AJA_STATUS_OPEN:				return "AJA_STATUS_OPEN";
		case AJA_STATUS_IO:					return "AJA_STATUS_IO";
		case AJA_STATUS_DISABLED:			return "AJA_STATUS_DISABLED";
		case AJA_STATUS_BUSY:				return "AJA_STATUS_BUSY";
		case AJA_STATUS_BAD_PARAM:			return "AJA_STATUS_BAD_PARAM";
		case AJA_STATUS_FEATURE:			return "AJA_STATUS_FEATURE";
		case AJA_STATUS_UNSUPPORTED:		return "AJA_STATUS_UNSUPPORTED";
		case AJA_STATUS_READONLY:			return "AJA_STATUS_READONLY";
		case AJA_STATUS_WRITEONLY:			return "AJA_STATUS_WRITEONLY";
		case AJA_STATUS_MEMORY:				return "AJA_STATUS_MEMORY";
		case AJA_STATUS_ALIGN:				return "AJA_STATUS_ALIGN";
		case AJA_STATUS_FLUSH:				return "AJA_STATUS_FLUSH";
		case AJA_STATUS_NOINPUT:			return "AJA_STATUS_NOINPUT";
		case AJA_STATUS_SURPRISE_REMOVAL:	return "AJA_STATUS_SURPRISE_REMOVAL";
		case AJA_STATUS_NOBUFFER:			return "AJA_STATUS_NOBUFFER";
		case AJA_STATUS_INVALID_TIME:		return "AJA_STATUS_INVALID_TIME";
		case AJA_STATUS_NOSTREAM:			return "AJA_STATUS_NOSTREAM";
		case AJA_STATUS_TIMEEXPIRED:		return "AJA_STATUS_TIMEEXPIRED";
		case AJA_STATUS_BADBUFFERCOUNT:		return "AJA_STATUS_BADBUFFERCOUNT";
		case AJA_STATUS_BADBUFFERSIZE:		return "AJA_STATUS_BADBUFFERSIZE";
		case AJA_STATUS_STREAMCONFLICT:		return "AJA_STATUS_STREAMCONFLICT";
		case AJA_STATUS_NOTINITIALIZED:		return "AJA_STATUS_NOTINITIALIZED";
		case AJA_STATUS_STREAMRUNNING:		return "AJA_STATUS_STREAMRUNNING";
        case AJA_STATUS_REBOOT:             return "AJA_STATUS_REBOOT";
	}
	return "<bad AJAStatus>";
}

/**
	@brief		Packs up to one raster line of uint16_t YUV components into an NTV2_FBF_10BIT_YCBCR frame buffer.
	@param[in]	inYCbCrLine		The YUV components to be packed into the frame buffer. This must contain at least 12 values.
	@param		inFrameBuffer	The frame buffer in host memory that is to be modified.
	@param[in]	inDescriptor	The NTV2FormatDescriptor that describes the frame buffer.
	@param[in]	inLineOffset	The zero-based line offset into the frame buffer where the packed components will be written.
	@return		True if successful;  otherwise false.
	@note		Neighboring components in the packed output will be corrupted if input component values exceed 0x3FF.
	@note		This is a safer version of the ::PackLine_UWordSequenceTo10BitYUV function.
**/
AJAExport bool YUVComponentsTo10BitYUVPackedBuffer (const std::vector<uint16_t> & inYCbCrLine, NTV2_POINTER & inFrameBuffer,
													const NTV2FormatDescriptor & inDescriptor, const UWord inLineOffset);


bool YUVComponentsTo10BitYUVPackedBuffer (const vector<uint16_t> & inYCbCrLine,  NTV2_POINTER & inFrameBuffer,
											const NTV2FormatDescriptor & inDescriptor,  const UWord inLineOffset)
{
	if (inYCbCrLine.size() < 12)
		return false;	//	Input vector needs at least 12 components
	if (inFrameBuffer.IsNULL())
		return false;	//	NULL frame buffer
	if (!inDescriptor.IsValid())
		return false;	//	Bad format descriptor
	if (ULWord(inLineOffset) >= inDescriptor.GetFullRasterHeight())
		return false;	//	Illegal line offset

	const uint32_t	pixPerLineX2	(inDescriptor.GetRasterWidth() * 2);
	uint32_t *		pOutPackedLine	(NULL);
	if (inFrameBuffer.GetByteCount() < inDescriptor.GetBytesPerRow() * ULWord(inLineOffset+1))
		return false;	//	Buffer too small

	pOutPackedLine = (uint32_t*) inDescriptor.GetRowAddress(inFrameBuffer.GetHostAddress(0), inLineOffset);
	if (pOutPackedLine == NULL)
		return false;	//	Buffer too small

	for (uint32_t inputCount = 0, outputCount = 0;   inputCount < pixPerLineX2;   outputCount += 4, inputCount += 12)
	{
		if ((inputCount+11) >= uint32_t(inYCbCrLine.size()))
			break;	//	Early exit (not fatal)
	#if defined(_DEBUG)	//	'at' throws upon bad index values
		pOutPackedLine[outputCount]   = uint32_t(inYCbCrLine.at(inputCount+0)) | uint32_t(inYCbCrLine.at(inputCount+ 1)<<10) | uint32_t(inYCbCrLine.at(inputCount+ 2)<<20);
		pOutPackedLine[outputCount+1] = uint32_t(inYCbCrLine.at(inputCount+3)) | uint32_t(inYCbCrLine.at(inputCount+ 4)<<10) | uint32_t(inYCbCrLine.at(inputCount+ 5)<<20);
		pOutPackedLine[outputCount+2] = uint32_t(inYCbCrLine.at(inputCount+6)) | uint32_t(inYCbCrLine.at(inputCount+ 7)<<10) | uint32_t(inYCbCrLine.at(inputCount+ 8)<<20);
		pOutPackedLine[outputCount+3] = uint32_t(inYCbCrLine.at(inputCount+9)) | uint32_t(inYCbCrLine.at(inputCount+10)<<10) | uint32_t(inYCbCrLine.at(inputCount+11)<<20);
	#else				//	'operator[]' doesn't throw
		pOutPackedLine[outputCount]   = uint32_t(inYCbCrLine[inputCount+0]) | uint32_t(inYCbCrLine[inputCount+ 1]<<10) | uint32_t(inYCbCrLine[inputCount+ 2]<<20);
		pOutPackedLine[outputCount+1] = uint32_t(inYCbCrLine[inputCount+3]) | uint32_t(inYCbCrLine[inputCount+ 4]<<10) | uint32_t(inYCbCrLine[inputCount+ 5]<<20);
		pOutPackedLine[outputCount+2] = uint32_t(inYCbCrLine[inputCount+6]) | uint32_t(inYCbCrLine[inputCount+ 7]<<10) | uint32_t(inYCbCrLine[inputCount+ 8]<<20);
		pOutPackedLine[outputCount+3] = uint32_t(inYCbCrLine[inputCount+9]) | uint32_t(inYCbCrLine[inputCount+10]<<10) | uint32_t(inYCbCrLine[inputCount+11]<<20);
	#endif
	}
	return true;
}


class CNTV2AncDataTester
{
	public:
		static bool BFT_AncEnums (void)
		{
			//	AJAAncillaryDataType
			for (unsigned ordinal(0);  ordinal < AJAAncillaryDataType_Size;  ordinal++)
			{
				const AJAAncillaryDataType	ancType = AJAAncillaryDataType(ordinal);
				//cerr << DEC(ordinal) << " " << xHEX0N(uint16_t(ordinal),2) << "\t" << ::AJAAncillaryDataTypeToString(ancType,false) << "\t" << ::AJAAncillaryDataTypeToString(ancType,true) << endl;
				SHOULD_BE_TRUE(IS_VALID_AJAAncillaryDataType(ancType));
			}

			//	AJAAncillaryDataLink
			for (unsigned ordinal(0);  ordinal < AJAAncillaryDataLink_Size;  ordinal++)
			{
				const AJAAncillaryDataLink	link = AJAAncillaryDataLink(ordinal);
				cerr << DEC(ordinal) << " " << xHEX0N(uint16_t(ordinal),2) << "\t" << ::AJAAncillaryDataLinkToString(link,false) << "\t" << ::AJAAncillaryDataLinkToString(link,true) << endl;
				if (link == AJAAncillaryDataLink_Unknown)
					SHOULD_BE_FALSE(IS_VALID_AJAAncillaryDataLink(link));
				else
					SHOULD_BE_TRUE(IS_VALID_AJAAncillaryDataLink(link));
			}

			//	AJAAncillaryDataStream
			for (unsigned ordinal(0);  ordinal < AJAAncillaryDataStream_Size;  ordinal++)
			{
				const AJAAncillaryDataStream	stream = AJAAncillaryDataStream(ordinal);
				cerr << DEC(ordinal) << " " << xHEX0N(uint16_t(ordinal),2) << "\t" << ::AJAAncillaryDataStreamToString(stream,false) << "\t" << ::AJAAncillaryDataStreamToString(stream,true) << endl;
				if (stream == AJAAncillaryDataStream_Unknown)
					SHOULD_BE_FALSE(IS_VALID_AJAAncillaryDataStream(stream));
				else
					SHOULD_BE_TRUE(IS_VALID_AJAAncillaryDataStream(stream));
			}

			//	AJAAncillaryDataChannel
			for (unsigned ordinal(0);  ordinal < AJAAncillaryDataChannel_Size;  ordinal++)
			{
				const AJAAncillaryDataChannel	chan = AJAAncillaryDataChannel(ordinal);
				cerr << DEC(ordinal) << " " << xHEX0N(uint16_t(ordinal),2) << "\t" << ::AJAAncillaryDataChannelToString(chan,false) << "\t" << ::AJAAncillaryDataChannelToString(chan,true) << endl;
				if (chan == AJAAncillaryDataChannel_Unknown)
					SHOULD_BE_FALSE(IS_VALID_AJAAncillaryDataChannel(chan));
				else
					SHOULD_BE_TRUE(IS_VALID_AJAAncillaryDataChannel(chan));
			}

			//	AJAAncillaryDataSpace
			for (unsigned ordinal(0);  ordinal < AJAAncillaryDataSpace_Size;  ordinal++)
			{
				const AJAAncillaryDataSpace	space = AJAAncillaryDataSpace(ordinal);
				cerr << DEC(ordinal) << " " << xHEX0N(uint16_t(ordinal),2) << "\t" << ::AJAAncillaryDataSpaceToString(space,false) << "\t" << ::AJAAncillaryDataSpaceToString(space,true) << endl;
				if (space == AJAAncillaryDataSpace_Unknown)
					SHOULD_BE_FALSE(IS_VALID_AJAAncillaryDataSpace(space));
				else
					SHOULD_BE_TRUE(IS_VALID_AJAAncillaryDataSpace(space));
			}
			return true;
		}

		static bool BFT_DataLocation (void)
		{
			typedef	std::set<AJAAncillaryDataLocation>	AncLocationSet;
			typedef	AncLocationSet::const_iterator		AncLocationSetConstIter;
			AncLocationSet	ancLocations;
			static const uint16_t					lines[]		=	{9,		16,		220,	285,	1910,	2320};
			static const uint16_t					hOffsets[]	=	{AJAAncillaryDataLocation::AJAAncDataHorizOffset_Default,	AJAAncillaryDataLocation::AJAAncDataHorizOffset_Anywhere,	AJAAncillaryDataLocation::AJAAncDataHorizOffset_AnyHanc,	127,	898,	1321};
			static const AJAAncillaryDataSpace		spaces[]	=	{AJAAncillaryDataSpace_VANC,	AJAAncillaryDataSpace_HANC};
			static const AJAAncillaryDataChannel	channels[]	=	{AJAAncillaryDataChannel_C,		AJAAncillaryDataChannel_Y};
			static const AJAAncillaryDataStream		streams[]	=	{AJAAncillaryDataStream_1,		AJAAncillaryDataStream_2,	AJAAncillaryDataStream_3,	AJAAncillaryDataStream_4};
			static const AJAAncillaryDataLink		links[]		=	{AJAAncillaryDataLink_A,		AJAAncillaryDataLink_B};
			AJAAncillaryDataLocation	nullLoc;
			AJAAncillaryDataLocation	a, toSetAllAtOnce, toBeSet;

			SHOULD_BE_FALSE(nullLoc.IsValid());	//	Invalid, because default constructor makes everything "unknown"
			SHOULD_BE_TRUE(nullLoc == a);

			for (unsigned lineNdx(0);  lineNdx < sizeof(lines)/sizeof(lines[0]);  lineNdx++)
			{
				const uint16_t lineNum(lines[lineNdx]);
				toBeSet.SetLineNumber(lineNum);
				for (unsigned spaceNdx(0);  spaceNdx < sizeof(spaces)/sizeof(spaces[0]);  spaceNdx++)
				{
					toBeSet.SetDataSpace(spaces[spaceNdx]);
					for (unsigned chanNdx(0);  chanNdx < sizeof(channels)/sizeof(channels[0]);  chanNdx++)
					{
						toBeSet.SetDataChannel(channels[chanNdx]);
						for (unsigned streamNdx(0);  streamNdx < sizeof(streams)/sizeof(streams[0]);  streamNdx++)
						{
							toBeSet.SetDataStream(streams[streamNdx]);
							for (unsigned hOffNdx(0);  hOffNdx < sizeof(hOffsets)/sizeof(hOffsets[0]);  hOffNdx++)
							{
								const uint16_t hOffset(hOffsets[hOffNdx]);
								toBeSet.SetHorizontalOffset(hOffset);
								for (unsigned linkNdx(0);  linkNdx < sizeof(links)/sizeof(links[0]);  linkNdx++)
								{
									const AJAAncillaryDataLocation	b(links[linkNdx], channels[chanNdx], spaces[spaceNdx], lineNum, hOffset, streams[streamNdx]);
									toBeSet.SetDataLink(links[linkNdx]);
									toSetAllAtOnce.Set(links[linkNdx], channels[chanNdx], spaces[spaceNdx], lineNum, hOffset, streams[streamNdx]);
									a.SetLineNumber(lineNum).SetHorizontalOffset(hOffset).SetDataSpace(spaces[spaceNdx]).SetDataChannel(channels[chanNdx]).SetDataStream(streams[streamNdx]).SetDataLink(links[linkNdx]);
									SHOULD_BE_TRUE(b == toBeSet);
									SHOULD_BE_TRUE(a == b);
									SHOULD_BE_TRUE(a == toSetAllAtOnce);
									SHOULD_BE_TRUE(a.IsValid());
									SHOULD_BE_EQUAL(a.GetDataLink(), links[linkNdx]);
									SHOULD_BE_EQUAL(b.GetDataStream(), streams[streamNdx]);
									SHOULD_BE_EQUAL(toBeSet.GetDataChannel(), channels[chanNdx]);
									SHOULD_BE_EQUAL(toSetAllAtOnce.GetDataSpace(), spaces[spaceNdx]);
									SHOULD_BE_EQUAL(a.GetLineNumber(), lineNum);
									SHOULD_BE_EQUAL(a.GetHorizontalOffset(), hOffset);
									a.Reset();
									SHOULD_BE_FALSE(a == b);
									SHOULD_BE_FALSE(a.IsValid());
									ancLocations.insert(b);
								}	//	for each AJAAncillaryDataLink
							}	//	for each horizOffset
						}	//	for each AJAAncillaryDataStream
					}	//	for each AJAAncillaryDataChannel
				}	//	for each AJAAncillaryDataSpace
			}	//	for each line number
			//cerr << endl << endl;		for (AncLocationSetConstIter it(ancLocations.begin());  it != ancLocations.end();  ++it)	cerr << *it << endl;	cerr << endl << endl;

			//	Ordering Tests
			return true;
		}

		static bool	BFT_AncDataCEA608Vanc (void)
		{
			static const uint8_t			pGump608Vanc[]	=	{	0xFF,	0xA0,	0x09,	0x61,	0x02,	0x03,
																	0x09,	0xC1,	0xC2,	0xF2,	//	end of packet
																	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10};
			uint32_t						packetByteCount	(0);
			AJAAncillaryData_Cea608_Vanc	pktRX, pktTX;
			AJAAncillaryData				defaultPkt;

			//	Test AJAAncillaryData_Cea608_Vanc  and GUMP decoding of it
			SHOULD_SUCCEED (defaultPkt.InitWithReceivedData (pGump608Vanc, sizeof(pGump608Vanc), AJAAncillaryDataLocation(), packetByteCount));
			SHOULD_BE_EQUAL (defaultPkt.GetDC(), 3);
			SHOULD_BE_EQUAL (defaultPkt.GetDID(), 0x61);
			SHOULD_BE_EQUAL (defaultPkt.GetSID(), 0x02);
			SHOULD_BE_EQUAL (defaultPkt.GetChecksum(), 0xF2);
			SHOULD_BE_EQUAL (defaultPkt.GetLocationVideoLink(), AJAAncillaryDataLink_Unknown);
			SHOULD_BE_EQUAL (defaultPkt.GetLocationDataChannel(), AJAAncillaryDataChannel_Y);
			SHOULD_BE_EQUAL (defaultPkt.GetLocationVideoSpace(), AJAAncillaryDataSpace_VANC);
			SHOULD_BE_EQUAL (defaultPkt.GetLocationLineNumber(), 9);
			SHOULD_BE_EQUAL (defaultPkt.GetLocationHorizOffset(), 0);
			SHOULD_BE_EQUAL (defaultPkt.GetDataCoding(), AJAAncillaryDataCoding_Digital);
			SHOULD_BE_FALSE (defaultPkt.GotValidReceiveData());		//	False, because wasn't vetted by specific subclass
			SHOULD_BE_EQUAL (defaultPkt.GetAncillaryDataType(), AJAAncillaryDataType_Unknown);
			SHOULD_BE_UNEQUAL (defaultPkt.GetAncillaryDataType(), AJAAncillaryDataType_Cea608_Vanc);
			SHOULD_BE_EQUAL (AJAAncillaryData_Cea608_Vanc::RecognizeThisAncillaryData(&defaultPkt), AJAAncillaryDataType_Cea608_Vanc);
			SHOULD_BE_EQUAL (AJAAncillaryData_Cea608_Line21::RecognizeThisAncillaryData(&defaultPkt), AJAAncillaryDataType_Unknown);
			SHOULD_BE_EQUAL (AJAAncillaryData_Cea708::RecognizeThisAncillaryData(&defaultPkt), AJAAncillaryDataType_Unknown);
			SHOULD_SUCCEED (pktRX.InitWithReceivedData (pGump608Vanc, sizeof(pGump608Vanc), AJAAncillaryDataLocation(), packetByteCount));
			SHOULD_SUCCEED (pktRX.ParsePayloadData());
			SHOULD_BE_TRUE (pktRX.GotValidReceiveData());
			uint8_t	fieldNumber(0), lineNumber(0), char1(0), char2(0);	bool isValid(false);
			SHOULD_SUCCEED (pktRX.GetLine(fieldNumber, lineNumber));
			SHOULD_BE_EQUAL (fieldNumber, 0);	//	F1
			SHOULD_BE_EQUAL (lineNumber, 9);	//	SMPTE line 9
			SHOULD_SUCCEED (pktRX.GetCEA608Characters(char1, char2, isValid));
			SHOULD_BE_EQUAL (char1, 'A');
			SHOULD_BE_EQUAL (char2, 'B');
			SHOULD_BE_TRUE (isValid);
			return true;
		}

		static bool	BFT_AncDataCEA608Raw (void)
		{
			static const uint8_t			pGump608Raw[]	=	{	0xFF,	0xE0,	0x00,	0x00,	0x00,	0xFF,
																	0x24,	0x47,	0x6a,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x11,	0x16,	0x1d,	0x26,	0x31,	0x3d,	0x4a,	0x56,	0x62,	0x6c,	0x74,
																	0x7a,	0x7d,	0x7d,	0x7a,	0x74,	0x6c,	0x62,	0x56,	0x4a,	0x3d,	0x31,	0x26,	0x1d,	0x16,	0x11,	0x10,	0x11,	0x16,	0x1d,	0x26,	0x31,	0x3d,	0x4a,	0x56,	0x62,	0x6c,	0x74,	0x7a,	0x7d,	0x7d,	0x7a,	0x74,
																	0x6c,	0x62,	0x56,	0x4a,	0x3d,	0x31,	0x26,	0x1d,	0x16,	0x11,	0x10,	0x11,	0x16,	0x1d,	0x26,	0x31,	0x3d,	0x4a,	0x56,	0x62,	0x6c,	0x74,	0x7a,	0x7d,	0x7d,	0x7a,	0x74,	0x6c,	0x62,	0x56,	0x4a,	0x3d,
																	0x31,	0x26,	0x1d,	0x16,	0x11,	0x10,	0x11,	0x16,	0x1d,	0x26,	0x31,	0x3d,	0x4a,	0x56,	0x62,	0x6c,	0x74,	0x7a,	0x7d,	0x7d,	0x7a,	0x74,	0x6c,	0x62,	0x56,	0x4a,	0x3d,	0x31,	0x26,	0x1d,	0x16,	0x11,
																	0x10,	0x11,	0x16,	0x1d,	0x26,	0x31,	0x3d,	0x4a,	0x56,	0x62,	0x6c,	0x74,	0x7a,	0x7d,	0x7d,	0x7a,	0x74,	0x6c,	0x62,	0x56,	0x4a,	0x3d,	0x31,	0x26,	0x1d,	0x16,	0x11,	0x10,	0x11,	0x16,	0x1d,	0x26,
																	0x31,	0x3d,	0x4a,	0x56,	0x62,	0x6c,	0x74,	0x7a,	0x7d,	0x7d,	0x7a,	0x74,	0x6c,	0x62,	0x56,	0x4a,	0x3d,	0x31,	0x26,	0x1d,	0x16,	0x11,	0x10,	0x11,	0x16,	0x1d,	0x26,	0x31,	0x3d,	0x4a,	0x56,	0x62,
																	0x6c,	0x74,	0x7a,	0x7d,	0x7d,	0x7a,	0x74,	0x6c,	0x62,	0x56,	0x4a,	0x3d,	0x31,	0x26,	0x1d,	0x16,	0x11,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,
																	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x7e,	0x7e,
																	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x00,	0xff,	0xe0,	0x00,	0x00,	0x00,	0xff,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x10,
																	0x10,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x6a,	0x47,	0x24,	0x10,
																	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,
																	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,
																	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,
																	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,
																	0x10,	0x10,	0x10,	0x24,	0x47,	0x6a,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,
																	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x6a,	0x47,	0x24,	0x10,	0x10,	0x10,	0x10,
																	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x00,	0xff,	0xe0,	0x00,	0x00,	0x00,	0xd2,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x24,	0x47,	0x6a,	0x7e,	0x7e,
																	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,
																	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,
																	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x6a,	0x47,	0x24,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,
																	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x24,	0x47,	0x6a,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,
																	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x7e,	0x6a,	0x47,	0x24,
																	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,
																	0x10,	0x10,	0x10,	0x10,	0x00};
			uint32_t						packetByteCount	(0);
			AJAAncillaryData_Cea608_Line21	pktRX, pktTX;
			AJAAncillaryData				defaultPkt;
			uint8_t							char1(0), char2(0);
			bool							isValid(false);

			//	Test AJAAncillaryData_Cea608_Line21  and GUMP decoding of it
			SHOULD_SUCCEED (defaultPkt.InitWithReceivedData (pGump608Raw, sizeof(pGump608Raw), AJAAncillaryDataLocation(), packetByteCount));
			SHOULD_BE_EQUAL (defaultPkt.GetDC(), 720);
			SHOULD_BE_EQUAL (defaultPkt.GetDID(), 0);
			SHOULD_BE_EQUAL (defaultPkt.GetSID(), 0);
			SHOULD_BE_EQUAL (defaultPkt.GetChecksum(), 0);
			SHOULD_BE_EQUAL (defaultPkt.GetLocationVideoLink(), AJAAncillaryDataLink_Unknown);
			SHOULD_BE_EQUAL (defaultPkt.GetLocationDataChannel(), AJAAncillaryDataChannel_Y);
			SHOULD_BE_EQUAL (defaultPkt.GetLocationVideoSpace(), AJAAncillaryDataSpace_VANC);
			SHOULD_BE_EQUAL (defaultPkt.GetLocationLineNumber(), 9);
			SHOULD_BE_EQUAL (defaultPkt.GetDataCoding(), AJAAncillaryDataCoding_Raw);
			SHOULD_BE_FALSE (defaultPkt.GotValidReceiveData());		//	False, because wasn't vetted by specific subclass
			SHOULD_BE_EQUAL (defaultPkt.GetAncillaryDataType(), AJAAncillaryDataType_Unknown);
			SHOULD_BE_UNEQUAL (defaultPkt.GetAncillaryDataType(), AJAAncillaryDataType_Cea608_Line21);
			SHOULD_BE_EQUAL (AJAAncillaryData_Cea608_Vanc::RecognizeThisAncillaryData(&defaultPkt), AJAAncillaryDataType_Unknown);
			SHOULD_BE_EQUAL (AJAAncillaryData_Cea608_Line21::RecognizeThisAncillaryData(&defaultPkt), AJAAncillaryDataType_Cea608_Line21);
			SHOULD_BE_EQUAL (AJAAncillaryData_Cea708::RecognizeThisAncillaryData(&defaultPkt), AJAAncillaryDataType_Unknown);
			SHOULD_SUCCEED (pktRX.InitWithReceivedData (pGump608Raw, sizeof(pGump608Raw), AJAAncillaryDataLocation(), packetByteCount));
			SHOULD_SUCCEED (pktRX.ParsePayloadData());
			SHOULD_BE_TRUE (pktRX.GotValidReceiveData());
			SHOULD_SUCCEED (pktRX.GetCEA608Characters(char1, char2, isValid));
			SHOULD_BE_EQUAL (char1, 'A');
			SHOULD_BE_EQUAL (char2, 'n');
			SHOULD_BE_TRUE (isValid);

			AJAAncillaryList	packetList;
			SHOULD_SUCCEED (packetList.AddReceivedAncillaryData (pGump608Raw, sizeof(pGump608Raw)));
			cerr << endl << packetList << endl;


			AJAAncillaryData_Cea608_Line21 * pLine21Packet = reinterpret_cast <AJAAncillaryData_Cea608_Line21 *> (AJAAncillaryDataFactory::Create(AJAAncillaryDataType_Cea608_Line21));
			SHOULD_BE_NON_NULL (pLine21Packet);
			SHOULD_SUCCEED (pLine21Packet->GeneratePayloadData());
			SHOULD_SUCCEED (pLine21Packet->SetDataLocation (AJAAncillaryDataLocation(AJAAncillaryDataLink_A, AJAAncillaryDataChannel_Y, AJAAncillaryDataSpace_VANC, 21)));
			SHOULD_SUCCEED (pLine21Packet->SetCEA608Bytes (AJAAncillaryData_Cea608::AddOddParity('A'), AJAAncillaryData_Cea608::AddOddParity('b')));
			SHOULD_SUCCEED (pLine21Packet->SetDID(AJAAncillaryData_Cea608_Vanc_DID));
			SHOULD_SUCCEED (pLine21Packet->SetSID(AJAAncillaryData_Cea608_Vanc_SID));
			//pLine21Packet->Print (cerr, true);
			SHOULD_BE_EQUAL (AJAAncillaryDataFactory::GuessAncillaryDataType(pLine21Packet), AJAAncillaryDataType_Cea608_Line21);

			return true;
		}	//	BFT_AncDataCEA608Raw


		static bool	BFT_AncDataCEA708 (void)
		{
			static const uint8_t		pGump708	[]=	{	0xFF,	0xA0,	0x09,	0x61,	0x01,	0x52,	0x96,	0x69,	0x52,	0x4F,	0x67,	0xA7,	0x9A,	0x72,	0xF4,	0xFC,	0x80,
															0x80,	0xF9,	0x00,	0x00,	0xFB,	0x00,	0x00,	0xFA,	0x00,	0x00,	0xFA,	0x00,	0x00,	0xFA,	0x00,	0x00,
															0xFA,	0x00,	0x00,	0xFA,	0x00,	0x00,	0xFA,	0x00,	0x00,	0xFA,	0x00,	0x00,	0xFA,	0x00,	0x00,	0xFA,
															0x00,	0x00,	0xFA,	0x00,	0x00,	0xFA,	0x00,	0x00,	0xFA,	0x00,	0x00,	0xFA,	0x00,	0x00,	0xFA,	0x00,
															0x00,	0xFA,	0x00,	0x00,	0xFA,	0x00,	0x00,	0xFA,	0x00,	0x00,	0x73,	0x91,	0xE1,	0x00,	0x00,	0x00,
															0xC1,	0x3F,	0xFF,	0x74,	0xA7,	0x9A,	0x2F,	0xB4,	//	end of packet
															0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10};
			uint32_t					packetByteCount	(0);
			AJAAncillaryData_Cea708		pktRX, pktTX;
			AJAAncillaryData			defaultPkt;

			//	Test AJAAncillaryData_Cea708  and GUMP decoding of it
			SHOULD_SUCCEED (defaultPkt.InitWithReceivedData (pGump708, sizeof(pGump708), AJAAncillaryDataLocation(), packetByteCount));
			SHOULD_BE_EQUAL (defaultPkt.GetDC(), 82);
			SHOULD_BE_EQUAL (defaultPkt.GetDID(), 0x61);
			SHOULD_BE_EQUAL (defaultPkt.GetSID(), 0x01);
			SHOULD_BE_EQUAL (defaultPkt.GetChecksum(), 0xB4);
			SHOULD_BE_EQUAL (defaultPkt.GetLocationVideoLink(), AJAAncillaryDataLink_Unknown);
			SHOULD_BE_EQUAL (defaultPkt.GetLocationDataChannel(), AJAAncillaryDataChannel_Y);
			SHOULD_BE_EQUAL (defaultPkt.GetLocationVideoSpace(), AJAAncillaryDataSpace_VANC);
			SHOULD_BE_EQUAL (defaultPkt.GetLocationLineNumber(), 9);
			SHOULD_BE_EQUAL (defaultPkt.GetDataCoding(), AJAAncillaryDataCoding_Digital);
			SHOULD_BE_FALSE (defaultPkt.GotValidReceiveData());		//	False, because wasn't vetted by specific subclass
			SHOULD_BE_EQUAL (defaultPkt.GetAncillaryDataType(), AJAAncillaryDataType_Unknown);
			SHOULD_BE_UNEQUAL (defaultPkt.GetAncillaryDataType(), AJAAncillaryDataType_Cea608_Vanc);
			SHOULD_BE_EQUAL (AJAAncillaryData_Cea608_Vanc::RecognizeThisAncillaryData(&defaultPkt), AJAAncillaryDataType_Unknown);
			SHOULD_BE_EQUAL (AJAAncillaryData_Cea608_Line21::RecognizeThisAncillaryData(&defaultPkt), AJAAncillaryDataType_Unknown);
			SHOULD_BE_EQUAL (AJAAncillaryData_Cea708::RecognizeThisAncillaryData(&defaultPkt), AJAAncillaryDataType_Cea708);
			SHOULD_SUCCEED (pktRX.InitWithReceivedData (pGump708, sizeof(pGump708), AJAAncillaryDataLocation(), packetByteCount));
			SHOULD_SUCCEED (pktRX.ParsePayloadData());
			SHOULD_BE_TRUE (pktRX.GotValidReceiveData());

			//	Test GUMP encoding...
			SHOULD_SUCCEED (pktTX.SetPayloadData(pktRX.GetPayloadData(), pktRX.GetPayloadByteCount()));
			SHOULD_BE_EQUAL(::memcmp(pktTX.GetPayloadData(), pktRX.GetPayloadData(), pktTX.GetDC()), 0);
			//SHOULD_BE_EQUAL(pktRX, pktTX);
			return true;
		}	//	BFT_AncDataCEA708


		static bool BFT_SMPTEAncData (const vector<uint16_t> & in10BitYUVReferenceLine)
		{
			static const uint16_t	pv210YSamples [] =	{	0x000,	0x3FF,	0x3FF,	0x161,	0x101,	0x152,	0x296,	0x269,	0x152,	0x14F,	0x167,	0x2A9,	0x27E,	0x272,	0x1F4,	0x2FC,
															0x120,	0x173,	0x2F9,	0x200,	0x200,	0x2FF,	0x146,	0x228,	0x1FE,	0x173,	0x265,	0x1FE,	0x16D,	0x269,	0x1FE,	0x16E,
															0x161,	0x1FE,	0x272,	0x179,	0x1FE,	0x200,	0x200,	0x2FA,	0x200,	0x200,	0x2FA,	0x200,	0x200,	0x2FA,	0x200,	0x200,
															0x2FA,	0x200,	0x200,	0x2FA,	0x200,	0x200,	0x2FA,	0x200,	0x200,	0x2FA,	0x200,	0x200,	0x2FA,	0x200,	0x200,	0x2FA,
															0x200,	0x200,	0x2FA,	0x200,	0x200,	0x2FA,	0x200,	0x200,	0x2FA,	0x200,	0x200,	0x173,	0x191,	0x2E1,	0x200,	0x200,
															0x200,	0x1C1,	0x23F,	0x2FF,	0x274,	0x2A9,	0x27E,	0x2E2,	0x2B4,	//	end of packet
															0x040,	0x040,	0x040,	0x040,	0x040,	0x040,	0x040,	0x040,	0x040,	0x040,	0x040,	0x040,	0x040,	0x040,	0x040,	0x040,	0x040,	0x040,	0x040};
			UWordSequence	v210VancLine;
			for (unsigned ndx(0);  ndx < sizeof(pv210YSamples);  ndx++)
			{
				v210VancLine.push_back(0x040);				//	Chroma
				v210VancLine.push_back(pv210YSamples[ndx]);	//	Luma
			}
			UWordVANCPacketList	u16Pkts;
			AJAAncillaryList	pktList;
			SHOULD_BE_FALSE (CNTV2SMPTEAncData::GetAncPacketsFromVANCLine (UWordSequence(), kNTV2SMPTEAncChannel_Y, u16Pkts));
			SHOULD_BE_TRUE (u16Pkts.empty());
			SHOULD_BE_TRUE (CNTV2SMPTEAncData::GetAncPacketsFromVANCLine (v210VancLine, kNTV2SMPTEAncChannel_C, u16Pkts));
			SHOULD_BE_TRUE (u16Pkts.empty());
			SHOULD_BE_TRUE (CNTV2SMPTEAncData::GetAncPacketsFromVANCLine (v210VancLine, kNTV2SMPTEAncChannel_Y, u16Pkts));
			SHOULD_BE_FALSE (u16Pkts.empty());
			SHOULD_BE_EQUAL (u16Pkts.size(), 1);
			SHOULD_SUCCEED (pktList.AddVANCData(u16Pkts.front(), 9, AJAAncillaryDataChannel_Y));
			cerr << pktList << endl;

			if (true)
			{
				//	Test CNTV2SMPTEAncData::GetAncPacketsFromVANCLine...
				UWordVANCPacketList	packets;
				UWordSequence		hOffsets;
				::SetDefaultCaptionLogOutputStream(cerr);
				//::SetDefaultCaptionLogMask(kCaptionLog_SMPTEAncErrors | kCaptionLog_SMPTEAncSuccess | kCaptionLog_SMPTEAncDebug);
				SHOULD_BE_TRUE(CNTV2SMPTEAncData::GetAncPacketsFromVANCLine (in10BitYUVReferenceLine, kNTV2SMPTEAncChannel_Both, packets, hOffsets));
				SHOULD_BE_EQUAL(packets.size(), 2);
				SHOULD_BE_EQUAL(packets.size(), hOffsets.size());
				const UWordSequence	rawPktComponents1	(packets.at(0));
				const UWordSequence	rawPktComponents2	(packets.at(1));
				SHOULD_BE_EQUAL(rawPktComponents1.size(), 223);
				SHOULD_BE_EQUAL(rawPktComponents2.size(), 9);
				cerr << "BFT_SMPTEAncData:  PACKETS:  " << packets << endl
					 << "HOFFSETS:  " << hOffsets << endl;
				//	TBD:	Validate hOffsets
				//	TBD:	Validate packets
				SHOULD_BE_TRUE(false);	//	TBD
			}

			return true;
		}	//	BFT_SMPTEAncData


		static bool BFT_AncillaryData (void)
		{
			AJAAncillaryData					defaultPkt;
			AJAAncillaryData *					pDefaultPkt		(NULL);
			AJAAncillaryData *					pClone			(NULL);
			const AJAAncillaryDataLocation		nullLocation;
			vector<uint8_t>						buffer4K;
			buffer4K.resize(4096, 0x65);

			//	Verify default values...
			SHOULD_BE_FALSE (defaultPkt.GotValidReceiveData());
			SHOULD_BE_EQUAL (defaultPkt.GetDID(), 0x00);
			SHOULD_BE_EQUAL (defaultPkt.GetSID(), 0x00);
			SHOULD_BE_TRUE (defaultPkt.IsEmpty());
			SHOULD_BE_EQUAL (defaultPkt.GetPayloadByteCount(), 0);
			SHOULD_BE_NULL (defaultPkt.GetPayloadData());
			SHOULD_BE_UNEQUAL (defaultPkt.GetDataLocation(), nullLocation);
			SHOULD_BE_TRUE (defaultPkt.GetDataLocation().IsValid());
			SHOULD_BE_TRUE (defaultPkt.IsLumaChannel());
			SHOULD_BE_FALSE (defaultPkt.IsChromaChannel());
			SHOULD_BE_TRUE (defaultPkt.IsDigital());
			SHOULD_BE_FALSE (defaultPkt.IsRaw());
			SHOULD_BE_TRUE (defaultPkt.IsVanc());
			SHOULD_BE_FALSE (defaultPkt.IsHanc());
			SHOULD_BE_EQUAL (defaultPkt.GetLocationLineNumber(), 0);
			SHOULD_BE_EQUAL(defaultPkt.GetLocationVideoLink(), AJAAncillaryDataLink_A);
			SHOULD_BE_EQUAL(defaultPkt.GetLocationDataStream(), AJAAncillaryDataStream_1);
			SHOULD_BE_EQUAL(defaultPkt.GetLocationVideoSpace(), AJAAncillaryDataSpace_VANC);
			SHOULD_BE_EQUAL(defaultPkt.GetLocationDataChannel(), AJAAncillaryDataChannel_Y);

			//	Verify that GetDataLocation/SetDataLocation works...
			if (true)
			{
				const AJAAncillaryDataLocation savedLoc(defaultPkt.GetDataLocation());
					SHOULD_SUCCEED(defaultPkt.SetLocationVideoLink(AJAAncillaryDataLink_B));
					SHOULD_SUCCEED(defaultPkt.SetLocationDataStream(AJAAncillaryDataStream_4));
					SHOULD_SUCCEED(defaultPkt.SetLocationVideoSpace(AJAAncillaryDataSpace_HANC));
					SHOULD_SUCCEED(defaultPkt.SetLocationDataChannel(AJAAncillaryDataChannel_C));
					SHOULD_SUCCEED(defaultPkt.SetLocationLineNumber(225));
					SHOULD_BE_EQUAL(defaultPkt.GetLocationLineNumber(), 225);
					SHOULD_BE_EQUAL(defaultPkt.GetLocationVideoLink(), AJAAncillaryDataLink_B);
					SHOULD_BE_EQUAL(defaultPkt.GetLocationDataStream(), AJAAncillaryDataStream_4);
					SHOULD_BE_EQUAL(defaultPkt.GetLocationVideoSpace(), AJAAncillaryDataSpace_HANC);
					SHOULD_BE_EQUAL(defaultPkt.GetLocationDataChannel(), AJAAncillaryDataChannel_C);
				SHOULD_SUCCEED(defaultPkt.SetDataLocation(savedLoc));
			}

			//	Validate new "Unknown" packet from AJAAncillaryDataFactory::Create...
			SHOULD_BE_NULL (pDefaultPkt);
			pDefaultPkt = AJAAncillaryDataFactory::Create(AJAAncillaryDataType_Unknown);
			SHOULD_BE_NON_NULL (pDefaultPkt);
			SHOULD_BE_TRUE (pDefaultPkt);
			SHOULD_BE_FALSE (pDefaultPkt->GotValidReceiveData());
			SHOULD_BE_EQUAL (pDefaultPkt->GetDID(), 0x00);
			SHOULD_BE_EQUAL (pDefaultPkt->GetSID(), 0x00);
			SHOULD_BE_TRUE (pDefaultPkt->IsEmpty());
			SHOULD_BE_EQUAL (pDefaultPkt->GetPayloadByteCount(), 0);
			SHOULD_BE_NULL (pDefaultPkt->GetPayloadData());
			SHOULD_BE_UNEQUAL (pDefaultPkt->GetDataLocation(), nullLocation);
			SHOULD_BE_TRUE (pDefaultPkt->GetDataLocation().IsValid());
			SHOULD_BE_TRUE (pDefaultPkt->IsLumaChannel());
			SHOULD_BE_FALSE (pDefaultPkt->IsChromaChannel());
			SHOULD_BE_TRUE (pDefaultPkt->IsDigital());
			SHOULD_BE_FALSE (pDefaultPkt->IsRaw());
			SHOULD_BE_TRUE (pDefaultPkt->IsVanc());
			SHOULD_BE_FALSE (pDefaultPkt->IsHanc());
			SHOULD_BE_EQUAL (pDefaultPkt->GetLocationLineNumber(), 0);

			//	Validate AJAAncillaryData operator == comparing two NULL packets...
			SHOULD_BE_EQUAL	(*pDefaultPkt, defaultPkt);

			//	Validate AJAAncillaryData payload data modification...
			static const UByte		pTestBytes []	=	{	0x10,	0x11,	0x12,	0x13,	0x14,	0x15,	0x16,	0x17,	0x18,	0x19,	0x1A,	0x1B,	0x1C,	0x1D,	0x1E,	0x1F,
															0x20,	0x21,	0x22,	0x23,	0x24,	0x25,	0x26,	0x27,	0x28,	0x29,	0x2A,	0x2B,	0x2C,	0x2D,	0x2E,	0x2F	};
			SHOULD_FAIL (pDefaultPkt->SetPayloadData (NULL, sizeof(pTestBytes)));
			SHOULD_FAIL (pDefaultPkt->SetPayloadData (pTestBytes, 0));
			SHOULD_SUCCEED (pDefaultPkt->SetPayloadData (pTestBytes, sizeof(pTestBytes)));
			SHOULD_SUCCEED (pDefaultPkt->SetDID(0x40));
			SHOULD_SUCCEED (pDefaultPkt->SetSID(0x02));
			SHOULD_BE_EQUAL (pDefaultPkt->GetDID(), 0x40);
			SHOULD_BE_EQUAL (pDefaultPkt->GetSID(), 0x02);
			SHOULD_BE_FALSE (pDefaultPkt->IsEmpty());
			SHOULD_BE_EQUAL (pDefaultPkt->GetPayloadByteCount(), sizeof(pTestBytes));
			SHOULD_BE_NON_NULL (pDefaultPkt->GetPayloadData());
			SHOULD_BE_EQUAL (pDefaultPkt->GetDC(), sizeof(pTestBytes));
			SHOULD_BE_FALSE (pDefaultPkt->ChecksumOK());
			SHOULD_SUCCEED (pDefaultPkt->SetChecksum(pDefaultPkt->Calculate8BitChecksum()));
			SHOULD_BE_TRUE (pDefaultPkt->ChecksumOK());
			SHOULD_SUCCEED (pDefaultPkt->SetDID(0x45));
			SHOULD_BE_UNEQUAL (pDefaultPkt->GetChecksum(), pDefaultPkt->Calculate8BitChecksum());	//	Unequal, since DID changed
			SHOULD_BE_FALSE (pDefaultPkt->ChecksumOK());	//	Fail, since DID changed
			SHOULD_SUCCEED (pDefaultPkt->SetChecksum(pDefaultPkt->Calculate8BitChecksum()));	//	Fix CS
			SHOULD_BE_EQUAL (pDefaultPkt->GetPayloadByteAtIndex(4), pTestBytes[4]);
			SHOULD_BE_EQUAL (pDefaultPkt->GetPayloadByteAtIndex(pDefaultPkt->GetDC()), 0);

			//	Validate AJAAncillaryData operators...
			SHOULD_BE_UNEQUAL (*pDefaultPkt, defaultPkt);	//	Validate AJAAncillaryData operator !=
			defaultPkt = *pDefaultPkt;						//	Validate AJAAncillaryData operator =
			SHOULD_BE_EQUAL	(*pDefaultPkt, defaultPkt);		//	Validate AJAAncillaryData operator ==
			defaultPkt.SetPayloadByteAtIndex(0x1F, defaultPkt.GetDC()/2);
			SHOULD_BE_UNEQUAL (*pDefaultPkt, defaultPkt);

			if (true)	/////	Clone, GeneratePayloadData and GuessAncillaryDataType Tests
			{
				pClone = defaultPkt.Clone();
				SHOULD_BE_EQUAL(*pClone, defaultPkt);

				//	Validate that Clear works...
				pClone->Clear();
				SHOULD_BE_UNEQUAL(*pClone, defaultPkt);
				SHOULD_BE_EQUAL(*pClone, AJAAncillaryData());
				SHOULD_BE_EQUAL (pClone->GetDID(), 0);
				SHOULD_BE_EQUAL (pClone->GetSID(), 0);
				SHOULD_BE_TRUE (pClone->IsEmpty());
				SHOULD_BE_EQUAL(pClone->GetDataCoding(), AJAAncillaryDataCoding_Digital);
				SHOULD_SUCCEED(pClone->SetDataCoding(AJAAncillaryDataCoding_Raw));
				SHOULD_BE_EQUAL(pClone->GetDataCoding(), AJAAncillaryDataCoding_Raw);
				SHOULD_BE_EQUAL(pClone->GetLocationLineNumber(), 0);
				SHOULD_SUCCEED(pClone->SetLocationLineNumber(100));
				SHOULD_BE_EQUAL(pClone->GetLocationLineNumber(), 100);
				SHOULD_BE_EQUAL(pClone->GetLocationVideoSpace(), AJAAncillaryDataSpace_VANC);
				SHOULD_SUCCEED(pClone->SetLocationVideoSpace(AJAAncillaryDataSpace_HANC));
				SHOULD_BE_EQUAL(pClone->GetLocationVideoSpace(), AJAAncillaryDataSpace_HANC);
				SHOULD_BE_EQUAL(pClone->GetLocationDataChannel(), AJAAncillaryDataChannel_Y);
				SHOULD_SUCCEED(pClone->SetLocationVideoStream(AJAAncillaryDataChannel_C));
				SHOULD_BE_EQUAL(pClone->GetLocationDataChannel(), AJAAncillaryDataChannel_C);
				SHOULD_BE_EQUAL(pClone->GetLocationVideoLink(), AJAAncillaryDataLink_A);
				SHOULD_SUCCEED(pClone->SetLocationVideoLink(AJAAncillaryDataLink_B));
				SHOULD_BE_EQUAL(pClone->GetLocationVideoLink(), AJAAncillaryDataLink_B);

				//	Test generating default packet using AJAAncillaryDataFactory::Create for these AJAAncillaryDataTypes...
				//	...then GeneratePayloadData for it, then clone it, and test AJAAncillaryDataFactory::GuessAncillaryDataType using the clone.
				//	The detected data type should match the original's...
				static const AJAAncillaryDataType	gDataTypes[]	=	{	AJAAncillaryDataType_Unknown
																			//	,AJAAncillaryDataType_Smpte2016_3
																			,AJAAncillaryDataType_Timecode_ATC
																			,AJAAncillaryDataType_Timecode_VITC
																			,AJAAncillaryDataType_Cea708
																			,AJAAncillaryDataType_Cea608_Vanc
																			,AJAAncillaryDataType_Cea608_Line21
																			//	,AJAAncillaryDataType_Smpte352
																			//	,AJAAncillaryDataType_Smpte2051
																			//	,AJAAncillaryDataType_FrameStatusInfo524D
																			//	,AJAAncillaryDataType_FrameStatusInfo5251
																			//	,AJAAncillaryDataType_HDR_SDR
																			//	,AJAAncillaryDataType_HDR_HDR10
																			//	,AJAAncillaryDataType_HDR_HLG
																			};
				for (unsigned ndx(0);  ndx < sizeof(gDataTypes)/sizeof(AJAAncillaryDataType);  ndx++)
				{
					const AJAAncillaryDataType	dataType	(gDataTypes[ndx]);
					cerr << ::AJAAncillaryDataTypeToString(dataType) << endl;
					AJAAncillaryData *	pDefaultPkt	= AJAAncillaryDataFactory::Create(dataType);
					SHOULD_BE_NON_NULL(pDefaultPkt);
					SHOULD_SUCCEED(pDefaultPkt->GeneratePayloadData());
					AJAAncillaryData *	pClonePkt	= pDefaultPkt->Clone();
					SHOULD_BE_NON_NULL(pClonePkt);
					SHOULD_BE_UNEQUAL(pDefaultPkt, pClonePkt);
					SHOULD_BE_EQUAL(*pDefaultPkt, *pClonePkt);
					SHOULD_BE_EQUAL (AJAAncillaryDataFactory::GuessAncillaryDataType(pClonePkt), dataType);
				}
			}	//	Clone Tests

			//	Validate AJAAncillaryData payload append...
			SHOULD_SUCCEED(defaultPkt.SetPayloadData(buffer4K.data(), buffer4K.size()));
			SHOULD_BE_EQUAL (defaultPkt.GetDC(), 4096);
			SHOULD_SUCCEED(defaultPkt.AppendPayload(pDefaultPkt));
			SHOULD_BE_EQUAL (defaultPkt.GetDC(), 4096+sizeof(pTestBytes));
			SHOULD_FAIL(defaultPkt.AppendPayloadData(NULL, 50));
			SHOULD_FAIL(defaultPkt.AppendPayloadData(pTestBytes, 0));
			SHOULD_SUCCEED(defaultPkt.AppendPayloadData(pTestBytes, sizeof(pTestBytes)));
			SHOULD_BE_EQUAL (defaultPkt.GetDC(), 4096 + 2*sizeof(pTestBytes));
			SHOULD_SUCCEED(defaultPkt.AppendPayload(defaultPkt));
			SHOULD_BE_EQUAL (defaultPkt.GetDC(), 2*(4096 + 2*sizeof(pTestBytes)));

			SHOULD_SUCCEED(defaultPkt.SetDataCoding(AJAAncillaryDataCoding_Digital));
			SHOULD_SUCCEED(defaultPkt.SetLocationLineNumber(9));
			SHOULD_SUCCEED(defaultPkt.SetLocationVideoSpace(AJAAncillaryDataSpace_VANC));
			SHOULD_SUCCEED(defaultPkt.SetLocationDataChannel(AJAAncillaryDataChannel_Y));
			SHOULD_SUCCEED(defaultPkt.SetLocationDataStream(AJAAncillaryDataStream_1));
			SHOULD_SUCCEED(defaultPkt.SetLocationVideoLink(AJAAncillaryDataLink_A));
			SHOULD_SUCCEED(defaultPkt.SetDID(0x45));
			SHOULD_SUCCEED(defaultPkt.SetSID(0x01));

			if (true)
			{
				//	Test 10-bit UDW conversion (with parity)...
				static const uint8_t				p8_RDD6Pkt1 []	=	{	0x00,	0x01,	0x02,	0x03,	0x04,	0x05,	0x06,	0x07,	0x08,	0x09,	0x0A,	0x0B,	0x0C,	0x0D,	0x0E,	0x0F,
																			0x10,	0x11,	0x12,	0x13,	0x14,	0x15,	0x16,	0x17,	0x18,	0x19,	0x1A,	0x1B,	0x1C,	0x1D,	0x1E,	0x1F,
																			0x20,	0x21,	0x22,	0x23,	0x24,	0x25,	0x26,	0x27,	0x28,	0x29,	0x2A,	0x2B,	0x2C,	0x2D,	0x2E,	0x2F,
																			0x30,	0x31,	0x32,	0x33,	0x34,	0x35,	0x36,	0x37,	0x38,	0x39,	0x3A,	0x3B,	0x3C,	0x3D,	0x3E,	0x3F,
																			0x40,	0x41,	0x42,	0x43,	0x44,	0x45,	0x46,	0x47,	0x48,	0x49,	0x4A,	0x4B,	0x4C,	0x4D,	0x4E,	0x4F,
																			0x50,	0x51,	0x52,	0x53,	0x54,	0x55,	0x56,	0x57,	0x58,	0x59,	0x5A,	0x5B,	0x5C,	0x5D,	0x5E,	0x5F,
																			0x60,	0x61,	0x62,	0x63,	0x64,	0x65,	0x66,	0x67,	0x68,	0x69,	0x6A,	0x6B,	0x6C,	0x6D,	0x6E,	0x6F,
																			0x70,	0x71,	0x72,	0x73,	0x74,	0x75,	0x76,	0x77,	0x78,	0x79,	0x7A,	0x7B,	0x7C,	0x7D,	0x7E,	0x7F,
																			0x80,	0x81,	0x82,	0x83,	0x84,	0x85,	0x86,	0x87,	0x88,	0x89,	0x8A,	0x8B,	0x8C,	0x8D,	0x8E,	0x8F,
																			0x90,	0x91,	0x92,	0x93,	0x94,	0x95,	0x96,	0x97,	0x98,	0x99,	0x9A,	0x9B,	0x9C,	0x9D,	0x9E,	0x9F,
																			0xA0,	0xA1,	0xA2,	0xA3,	0xA4,	0xA5,	0xA6,	0xA7,	0xA8,	0xA9,	0xAA,	0xAB,	0xAC,	0xAD,	0xAE,	0xAF,
																			0xB0,	0xB1,	0xB2,	0xB3,	0xB4,	0xB5,	0xB6,	0xB7,	0xB8,	0xB9,	0xBA,	0xBB,	0xBC,	0xBD,	0xBE,	0xBF,
																			0xC0,	0xC1,	0xC2,	0xC3,	0xC4,	0xC5,	0xC6,	0xC7,	0xC8,	0xC9,	0xCA,	0xCB,	0xCC,	0xCD,	0xCE,	0xCF,
																			0xD0,	0xD1,	0xD2,	0xD3,	0xD4,	0xD5,	0xD6,	0xD7,	0xD8,	0xD9,	0xDA,	0xDB,	0xDC,	0xDD,	0xDE,	0xDF,
																			0xE0,	0xE1,	0xE2,	0xE3,	0xE4,	0xE5,	0xE6,	0xE7,	0xE8,	0xE9,	0xEA,	0xEB,	0xEC,	0xED,	0xEE,	0xEF,
																			0xF0,	0xF1,	0xF2,	0xF3,	0xF4,	0xF5,	0xF6,	0xF7,	0xF8,	0xF9,	0xFA,	0xFB,	0xFC,	0xFD,	0xFE	};
				static const uint16_t				p16_RDD6Pkt1 [] =	{	0x200,	0x101,	0x102,	0x203,	0x104,	0x205,	0x206,	0x107,	0x108,	0x209,	0x20A,	0x10B,	0x20C,	0x10D,	0x10E,	0x20F,
																			0x110,	0x211,	0x212,	0x113,	0x214,	0x115,	0x116,	0x217,	0x218,	0x119,	0x11A,	0x21B,	0x11C,	0x21D,	0x21E,	0x11F,
																			0x120,	0x221,	0x222,	0x123,	0x224,	0x125,	0x126,	0x227,	0x228,	0x129,	0x12A,	0x22B,	0x12C,	0x22D,	0x22E,	0x12F,
																			0x230,	0x131,	0x132,	0x233,	0x134,	0x235,	0x236,	0x137,	0x138,	0x239,	0x23A,	0x13B,	0x23C,	0x13D,	0x13E,	0x23F,
																			0x140,	0x241,	0x242,	0x143,	0x244,	0x145,	0x146,	0x247,	0x248,	0x149,	0x14A,	0x24B,	0x14C,	0x24D,	0x24E,	0x14F,
																			0x250,	0x151,	0x152,	0x253,	0x154,	0x255,	0x256,	0x157,	0x158,	0x259,	0x25A,	0x15B,	0x25C,	0x15D,	0x15E,	0x25F,
																			0x260,	0x161,	0x162,	0x263,	0x164,	0x265,	0x266,	0x167,	0x168,	0x269,	0x26A,	0x16B,	0x26C,	0x16D,	0x16E,	0x26F,
																			0x170,	0x271,	0x272,	0x173,	0x274,	0x175,	0x176,	0x277,	0x278,	0x179,	0x17A,	0x27B,	0x17C,	0x27D,	0x27E,	0x17F,
																			0x180,	0x281,	0x282,	0x183,	0x284,	0x185,	0x186,	0x287,	0x288,	0x189,	0x18A,	0x28B,	0x18C,	0x28D,	0x28E,	0x18F,
																			0x290,	0x191,	0x192,	0x293,	0x194,	0x295,	0x296,	0x197,	0x198,	0x299,	0x29A,	0x19B,	0x29C,	0x19D,	0x19E,	0x29F,
																			0x2A0,	0x1A1,	0x1A2,	0x2A3,	0x1A4,	0x2A5,	0x2A6,	0x1A7,	0x1A8,	0x2A9,	0x2AA,	0x1AB,	0x2AC,	0x1AD,	0x1AE,	0x2AF,
																			0x1B0,	0x2B1,	0x2B2,	0x1B3,	0x2B4,	0x1B5,	0x1B6,	0x2B7,	0x2B8,	0x1B9,	0x1BA,	0x2BB,	0x1BC,	0x2BD,	0x2BE,	0x1BF,
																			0x2C0,	0x1C1,	0x1C2,	0x2C3,	0x1C4,	0x2C5,	0x2C6,	0x1C7,	0x1C8,	0x2C9,	0x2CA,	0x1CB,	0x2CC,	0x1CD,	0x1CE,	0x2CF,
																			0x1D0,	0x2D1,	0x2D2,	0x1D3,	0x2D4,	0x1D5,	0x1D6,	0x2D7,	0x2D8,	0x1D9,	0x1DA,	0x2DB,	0x1DC,	0x2DD,	0x2DE,	0x1DF,
																			0x1E0,	0x2E1,	0x2E2,	0x1E3,	0x2E4,	0x1E5,	0x1E6,	0x2E7,	0x2E8,	0x1E9,	0x1EA,	0x2EB,	0x1EC,	0x2ED,	0x2EE,	0x1EF,
																			0x2F0,	0x1F1,	0x1F2,	0x2F3,	0x1F4,	0x2F5,	0x2F6,	0x1F7,	0x1F8,	0x2F9,	0x2FA,	0x1FB,	0x2FC,	0x1FD,	0x1FE	};
				vector<uint16_t>	UDWs, componentData;
				SHOULD_SUCCEED(defaultPkt.SetPayloadData(p8_RDD6Pkt1, sizeof(p8_RDD6Pkt1)/sizeof(uint8_t)));
				SHOULD_SUCCEED(defaultPkt.GetPayloadData(UDWs, true));
				//for (unsigned ndx(0);  ndx < UDWs.size();  )	{	cerr << xHEX0N(UDWs[ndx],3) << ",\t";	if (++ndx % 16 == 0) cerr << endl;	}
				SHOULD_BE_EQUAL(UDWs.size(), sizeof(p16_RDD6Pkt1)/sizeof(uint16_t));
				SHOULD_BE_EQUAL(::memcmp(UDWs.data(), p16_RDD6Pkt1, sizeof(p16_RDD6Pkt1)), 0);

				//	Test GenerateTransmitData...
				SHOULD_SUCCEED(defaultPkt.GenerateTransmitData(componentData));
				SHOULD_BE_EQUAL(componentData.size(), 262);
				SHOULD_BE_EQUAL(componentData.at(0), 0x000);
				SHOULD_BE_EQUAL(componentData.at(1), 0x3FF);
				SHOULD_BE_EQUAL(componentData.at(2), 0x3FF);
				SHOULD_BE_EQUAL(componentData.at(3)&0x0FF, defaultPkt.GetDID());
				SHOULD_BE_EQUAL(componentData.at(4)&0x0FF, defaultPkt.GetSID());
				SHOULD_BE_EQUAL(componentData.at(5)&0x0FF, defaultPkt.GetDC());
				SHOULD_BE_EQUAL(componentData.at(6), p16_RDD6Pkt1[0]);
				SHOULD_BE_EQUAL(::memcmp(&componentData[6], p16_RDD6Pkt1, sizeof(p16_RDD6Pkt1)), 0);
				SHOULD_BE_EQUAL(componentData.back()&0x0FF, defaultPkt.Calculate8BitChecksum());
			}

			return true;
		}	//	BFT_AncillaryData


		static bool BFT_YUVComponentsTo10BitYUVPackedBuffer (const vector<uint16_t> & in10BitYUVReferenceLine)
		{
			NTV2FormatDescriptor	fd		(NTV2_STANDARD_525, NTV2_FBF_10BIT_YCBCR, NTV2_VANCMODE_TALLER);
			NTV2_POINTER			fb		(size_t(fd.GetTotalRasterBytes() - fd.GetVisibleRasterBytes()));	//	Just the VANC lines
			UWord					fbRow	(0);
			vector<uint16_t>		YUVLine;

			//	Validate "YUVLine too small" failures...
			fb.Fill(UByte(0x80));  YUVLine = in10BitYUVReferenceLine;  YUVLine.resize(5);
			SHOULD_BE_FALSE(::YUVComponentsTo10BitYUVPackedBuffer (YUVLine,  fb,  fd,  fbRow));	//	YUVLine too small
			fb.Fill(UByte(0x80));  YUVLine = in10BitYUVReferenceLine;  YUVLine.resize(10);
			SHOULD_BE_FALSE(::YUVComponentsTo10BitYUVPackedBuffer (YUVLine,  fb,  fd,  fbRow));	//	YUVLine too small
			fb.Fill(UByte(0x80));  YUVLine = in10BitYUVReferenceLine;  YUVLine.resize(15);
			SHOULD_BE_TRUE(::YUVComponentsTo10BitYUVPackedBuffer (YUVLine,  fb,  fd,  fbRow));

			//	Validate buffer size failures...
			YUVLine = in10BitYUVReferenceLine;		//	Valid, full input line
			SHOULD_BE_TRUE(fb.Allocate(0));		//	NULL buffer
			SHOULD_BE_FALSE(::YUVComponentsTo10BitYUVPackedBuffer (YUVLine,  fb,  fd,  fbRow));	//	Buffer is NULL
			SHOULD_BE_TRUE(fb.Allocate(256));	//	256-byte buffer
			SHOULD_BE_FALSE(::YUVComponentsTo10BitYUVPackedBuffer (YUVLine,  fb,  fd,  fbRow));	//	Buffer too small
			SHOULD_BE_TRUE(fb.Allocate(512));	//	512-byte buffer
			SHOULD_BE_FALSE(::YUVComponentsTo10BitYUVPackedBuffer (YUVLine,  fb,  fd,  fbRow));	//	Buffer too small
			SHOULD_BE_TRUE(fb.Allocate(fd.GetBytesPerRow()));	//	One-line buffer
			SHOULD_BE_TRUE(::YUVComponentsTo10BitYUVPackedBuffer (YUVLine,  fb,  fd,  fbRow));
			SHOULD_BE_TRUE(fb.Allocate(size_t(fd.GetTotalRasterBytes() - fd.GetVisibleRasterBytes())));	//	Back to a full-VANC buffer

			//	Validate line number failures...
			SHOULD_BE_FALSE(::YUVComponentsTo10BitYUVPackedBuffer (YUVLine,  fb,  fd,  fd.GetFirstActiveLine()+20));	//	Line out of bounds for given buffer
			SHOULD_BE_FALSE(::YUVComponentsTo10BitYUVPackedBuffer (YUVLine,  fb,  fd,  fd.GetFirstActiveLine()));		//	Line out of bounds for given buffer
			SHOULD_BE_TRUE(::YUVComponentsTo10BitYUVPackedBuffer (YUVLine,  fb,  fd,  fd.GetFirstActiveLine()/2));	//	OK: Line within VANC

			//	Validate format descriptor failures...
			SHOULD_BE_FALSE(::YUVComponentsTo10BitYUVPackedBuffer (YUVLine,  fb,  NTV2FormatDescriptor(),  fbRow));	//	Bad format descriptor

			//	Validate packed data...
			fb.Fill(UByte(0x80));
			SHOULD_BE_TRUE(::YUVComponentsTo10BitYUVPackedBuffer (YUVLine,  fb,  fd,  0));
			//	Uncomment the following 2 lines to generate the static kCompPackedData array from the contents of line 0 in "fd" buffer...
			//	const uint64_t *	pData	((const uint64_t *) fb.GetHostPointer());
			//	cerr << "\t\t\tstatic const uint64_t kCompPackedData64[] = {";  for (UWord ndx(0);  ndx < fd.GetBytesPerRow()/sizeof(uint64_t);  ndx++)	{cerr << "\t" << xHEX0N(pData[ndx],16) << ",";  if((ndx+1)%6==0)cerr << endl << "\t\t\t\t\t\t\t\t\t\t\t\t\t\t";}	cerr << "\t};" << endl;
			static const uint64_t kCompPackedData64[] = {	0x2D8405453FFFFC00,	0x1258912322288520,	0x22B4A92922889D26,	0x1318C12F22E8B52C,	0x1378DA351348CD32,	0x13D8F13B23A8E538,
															0x14390A411408FD3E,	0x1499224714651644,	0x14F93A4D14C92D4A,	0x2555525315254650,	0x15B96A5915855E56,	0x1619825F15E5765C,
															0x16799A6516498D62,	0x16D9B16B26A9A568,	0x1739CA711709BD6E,	0x1799E2771765D674,	0x17F9FA7D17C9ED7A,	0x185A1183282A0580,
															0x28B62989288A1D86,	0x191A418F28EA358C,	0x197A5A95194A4D92,	0x19DA719B29AA6598,	0x2A3689A12A0A7D9E,	0x2A96A1A72A6A95A4,
															0x2AF6B9AD2AC6AEAA,	0x1B5AD1B32B2AC5B0,	0x2BB6E9B92B8ADDB6,	0x1C1B01BF2BEAF5BC,	0x1C7B1AC51C4B0DC2,	0x1CDB31CB2CAB25C8,
															0x1D3B4AD11D0B3DCE,	0x1D9B62D71D6756D4,	0x1DFB7ADD1DCB6DDA,	0x1E5B91E32E2B85E0,	0x2EB7A9E92E8B9DE6,	0x1F1BC1EF2EEBB5EC,
															0x1F7BDAF51F4BCDF2,	0x102517FF3FF002D2,	0x0408016220A83D02,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,
															0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,
															0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,
															0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,
															0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,
															0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,
															0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,
															0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,
															0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,
															0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,
															0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,
															0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,
															0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,
															0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,
															0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,
															0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,
															0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,
															0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,
															0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,
															0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,
															0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,
															0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,
															0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,
															0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,
															0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,
															0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,
															0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,
															0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,
															0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,
															0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,
															0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,
															0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,
															0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,
															0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200,	0x0408004020010200	};
			SHOULD_BE_EQUAL(::memcmp(kCompPackedData64, fb.GetHostPointer(), fd.GetBytesPerRow()), 0);

			return true;
		}


		static bool BFT (void)
		{
			//	This sequence of 10-bit YUV component values contains two SD ancillary data packets,
			//	as they would appear in a NTV2_FBF_10BIT_YCBCR frame buffer.
			//		DID=0x45 SDID=0x01 DC=216 CS=0xD2
			//		DID=0x45 SDID=0x02 DC=2   CS=0x62
			static const uint16_t	SD10BitYCbCrLine[]	=	{//	Cb		Y		Cr		Cb		Y		Cr		Cb		Y		Cr		Cb		Y		Cr			Cb		Y		Cr		Cb		Y		Cr		Cb		Y		Cr		Cb		Y		Cr
																0x000,	0x3FF,	0x3FF,	0x145,	0x101,	0x2D8,	0x120,	0x221,	0x222,	0x123,	0x224,	0x125,		0x126,	0x227,	0x228,	0x129,	0x12A,	0x22B,	0x12C,	0x22D,	0x22E,	0x12F,	0x230,	0x131,
																0x132,	0x233,	0x134,	0x235,	0x236,	0x137,	0x138,	0x239,	0x23A,	0x13B,	0x23C,	0x13D,		0x13E,	0x23F,	0x140,	0x241,	0x242,	0x143,	0x244,	0x145,	0x146,	0x247,	0x248,	0x149,
																0x14A,	0x24B,	0x14C,	0x24D,	0x24E,	0x14F,	0x250,	0x151,	0x152,	0x253,	0x154,	0x255,		0x256,	0x157,	0x158,	0x259,	0x25A,	0x15B,	0x25C,	0x15D,	0x15E,	0x25F,	0x260,	0x161,
																0x162,	0x263,	0x164,	0x265,	0x266,	0x167,	0x168,	0x269,	0x26A,	0x16B,	0x26C,	0x16D,		0x16E,	0x26F,	0x170,	0x271,	0x272,	0x173,	0x274,	0x175,	0x176,	0x277,	0x278,	0x179,
																0x17A,	0x27B,	0x17C,	0x27D,	0x27E,	0x17F,	0x180,	0x281,	0x282,	0x183,	0x284,	0x185,		0x186,	0x287,	0x288,	0x189,	0x18A,	0x28B,	0x18C,	0x28D,	0x28E,	0x18F,	0x290,	0x191,
																0x192,	0x293,	0x194,	0x295,	0x296,	0x197,	0x198,	0x299,	0x29A,	0x19B,	0x29C,	0x19D,		0x19E,	0x29F,	0x2A0,	0x1A1,	0x1A2,	0x2A3,	0x1A4,	0x2A5,	0x2A6,	0x1A7,	0x1A8,	0x2A9,
																0x2AA,	0x1AB,	0x2AC,	0x1AD,	0x1AE,	0x2AF,	0x1B0,	0x2B1,	0x2B2,	0x1B3,	0x2B4,	0x1B5,		0x1B6,	0x2B7,	0x2B8,	0x1B9,	0x1BA,	0x2BB,	0x1BC,	0x2BD,	0x2BE,	0x1BF,	0x2C0,	0x1C1,
																0x1C2,	0x2C3,	0x1C4,	0x2C5,	0x2C6,	0x1C7,	0x1C8,	0x2C9,	0x2CA,	0x1CB,	0x2CC,	0x1CD,		0x1CE,	0x2CF,	0x1D0,	0x2D1,	0x2D2,	0x1D3,	0x2D4,	0x1D5,	0x1D6,	0x2D7,	0x2D8,	0x1D9,
																0x1DA,	0x2DB,	0x1DC,	0x2DD,	0x2DE,	0x1DF,	0x1E0,	0x2E1,	0x2E2,	0x1E3,	0x2E4,	0x1E5,		0x1E6,	0x2E7,	0x2E8,	0x1E9,	0x1EA,	0x2EB,	0x1EC,	0x2ED,	0x2EE,	0x1EF,	0x2F0,	0x1F1,
																0x1F2,	0x2F3,	0x1F4,	0x2F5,	0x2F6,	0x1F7,	0x2D2,	0x000,	0x3FF,	0x3FF,	0x145,	0x102,		0x102,	0x20F,	0x20A,	0x162,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,
																0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,		0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,
																0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,		0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,
																0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,		0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,
																0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,		0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,
																0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,		0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,
																0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,		0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,
																0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,		0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,
																0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,		0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,
																0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,		0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,
																0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,		0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,
																0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,		0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,
																0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,		0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,
																0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,		0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,
																0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,		0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,
																0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,		0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,
																0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,		0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,
																0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,		0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,
																0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,		0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,
																0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,		0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,
																0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,		0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,
																0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,		0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,
																0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,		0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,
																0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,		0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,
																0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,		0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,
																0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,		0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,
																0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,		0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,
																0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,		0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,
																0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,		0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,
																0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,		0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,
																0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,		0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,
																0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,		0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,
																0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,		0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,
																0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,		0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,
																0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,		0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,
																0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,		0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,
																0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,		0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,
																0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,		0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,
																0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,		0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,
																0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,		0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,
																0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,		0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,
																0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,		0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,
																0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,		0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,
																0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,		0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,
																0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,		0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,
																0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,		0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,
																0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,		0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,
																0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,		0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,
																0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,		0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,
																0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,		0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,
																0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,		0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040,	0x200,	0x040	};
			vector<uint16_t>	SD10BitYUVComponents, YUVLine;
			for (unsigned ndx(0);  ndx < sizeof(SD10BitYCbCrLine)/sizeof(SD10BitYCbCrLine[0]);  ndx++)
				SD10BitYUVComponents.push_back(SD10BitYCbCrLine[ndx]);
			SHOULD_BE_TRUE(BFT_YUVComponentsTo10BitYUVPackedBuffer(SD10BitYUVComponents));
			YUVLine = SD10BitYUVComponents;

			if (true)
				SHOULD_BE_TRUE (BFT_AncEnums());

			if (true)
				SHOULD_BE_TRUE (BFT_DataLocation());

			if (true)
				SHOULD_BE_TRUE (BFT_AncillaryData());

			if (true)
				SHOULD_BE_TRUE (BFT_SMPTEAncData(SD10BitYUVComponents));

			if (true)
			{
				//	Test SetFromVANCData
				NTV2FormatDescriptor	fd	(NTV2_STANDARD_525, NTV2_FBF_10BIT_YCBCR, NTV2_VANCMODE_TALLER);
				NTV2_POINTER			fb (size_t(fd.GetTotalRasterBytes()));	//	Tall SD frame buffer, in which a single VANC line will receive the packed YUV component data

cerr << "WILL ITERATE OVER " << fd.GetFirstActiveLine() << " VANC LINES AND " << SD10BitYUVComponents.size() << " YUV COMPONENTS" << endl;
				//	For each VANC line...
				for (UWord fbRowOffset(0);  fbRowOffset < fd.GetFirstActiveLine();  fbRowOffset++)
				{
					//	This test repeatedly resets the frame buffer and packs an ever-shrinking number of YUV components
					//	into one of its tall VANC lines (fbRowOffset)... then AJAAncillaryList::SetFromVANCData is called
					//	to parse as many AJAAncillaryData packets from the VANC area as possible... then each packet's
					//	data and metadata are validated.
					cerr << endl << endl << "VANC LINE OFFSET " << fbRowOffset << endl;
					YUVLine = SD10BitYUVComponents;
					while (YUVLine.size() > 11)
					{
						ULWord				smpteLineNum	(0);
						bool				isField2		(false);
						AJAAncillaryList	pktList;	//	Receives decoded Anc packets
						SHOULD_BE_TRUE(fd.GetSMPTELineNumber (fbRowOffset, smpteLineNum, isField2));
						fb.Fill(UByte(0x80));			//	Reset the frame buffer contents to 0x80808080....
						SHOULD_BE_TRUE(::YUVComponentsTo10BitYUVPackedBuffer (YUVLine,  fb,  fd,  fbRowOffset));
						SHOULD_SUCCEED(AJAAncillaryList::SetFromVANCData (fb, fd, pktList));
						if (YUVLine.size() < 228)	//	1st packet disappears around here
						{
							if (pktList.CountAncillaryData() != 0)	cerr << "IN ITERATION YUVLine.size=" << YUVLine.size() << " IN fbRowOffset=" << fbRowOffset << "..." << endl;
							SHOULD_BE_EQUAL(pktList.CountAncillaryData(), 0);	//	Should have no packets
						}
						else
						{
							AJAAncillaryData * pPkt1	(NULL);
							if (pktList.CountAncillaryData() != ((YUVLine.size() < 240) ? 1 : 2))	cerr << "IN ITERATION YUVLine.size=" << YUVLine.size() << " IN fbRowOffset=" << fbRowOffset << "..." << endl;
							SHOULD_BE_EQUAL(pktList.CountAncillaryData(),  YUVLine.size() < 240  ?  1  :  2);
							pPkt1 = pktList.GetAncillaryDataWithID (0x45, 0x01);
							SHOULD_BE_NON_NULL(pPkt1);	//	Should have packet DID=0x41/SID=0x01
							SHOULD_BE_EQUAL(pPkt1->GetDID(), 0x45);	//	Should have DID=0x41
							SHOULD_BE_EQUAL(pPkt1->GetSID(), 0x01);	//	Should have SID=0x01
							SHOULD_BE_EQUAL(pPkt1->GetDC(), 216);	//	Should have DC=216
							SHOULD_BE_EQUAL(pPkt1->GetChecksum(), 0xD2);	//	Should have CS=0xD2
							SHOULD_BE_EQUAL(pPkt1->GetDataCoding(), AJAAncillaryDataCoding_Digital);		//	Should be Digital
							SHOULD_BE_EQUAL(pPkt1->GetLocationVideoLink(), AJAAncillaryDataLink_Unknown);	//	Should be unknown Link
							SHOULD_BE_EQUAL(pPkt1->GetLocationDataStream(), AJAAncillaryDataStream_1);		//	Should be DS1
							SHOULD_BE_EQUAL(pPkt1->GetLocationDataChannel(), AJAAncillaryDataChannel_Both);	//	Should be C/both
							SHOULD_BE_EQUAL(pPkt1->GetLocationVideoSpace(), AJAAncillaryDataSpace_VANC);	//	Should be VANC
							SHOULD_BE_EQUAL(pPkt1->GetLocationLineNumber(), uint16_t(smpteLineNum));		//	Should match smpteLineNum
							SHOULD_BE_EQUAL(pPkt1->GetLocationHorizOffset(), 0);							//	Should be zero
							if (YUVLine.size() >= 240)	//	2nd packet appears around here
							{
								AJAAncillaryData *	pPkt2		(NULL);
								const UWord			pkt2HOffset	(pPkt1->GetDC() + 7);	//	Pkt2 HOffset should be 7 components more than Pkt1's DC
								if (pktList.CountAncillaryData() != 2)	cerr << "IN ITERATION YUVLine.size=" << YUVLine.size() << " IN fbRowOffset=" << fbRowOffset << "..." << endl;
								SHOULD_BE_EQUAL(pktList.CountAncillaryData(), 2);	//	Should have two packets
								pPkt2 = pktList.GetAncillaryDataWithID (0x45, 0x02);
								SHOULD_BE_NON_NULL(pPkt2);	//	Should have packet DID=0x41/SID=0x02
								SHOULD_BE_EQUAL(pPkt2->GetDID(), 0x45);	//	Should have DID=0x41
								SHOULD_BE_EQUAL(pPkt2->GetSID(), 0x02);	//	Should have SID=0x02
								SHOULD_BE_EQUAL(pPkt2->GetDC(), 2);	//	Should have DC=2
								SHOULD_BE_EQUAL(pPkt2->GetChecksum(), 0x62);	//	Should have CS=0x62
								SHOULD_BE_EQUAL(pPkt2->GetDataCoding(), AJAAncillaryDataCoding_Digital);		//	Should be Digital
								SHOULD_BE_EQUAL(pPkt2->GetLocationVideoLink(), AJAAncillaryDataLink_Unknown);	//	Should be unknown Link
								SHOULD_BE_EQUAL(pPkt2->GetLocationDataStream(), AJAAncillaryDataStream_1);		//	Should be DS1
								SHOULD_BE_EQUAL(pPkt2->GetLocationDataChannel(), AJAAncillaryDataChannel_Both);	//	Should be C/both
								SHOULD_BE_EQUAL(pPkt2->GetLocationVideoSpace(), AJAAncillaryDataSpace_VANC);	//	Should be VANC
								SHOULD_BE_EQUAL(pPkt2->GetLocationLineNumber(), uint16_t(smpteLineNum));		//	Should match smpteLineNum
								if (pPkt2->GetLocationHorizOffset() != pkt2HOffset)	cerr << "IN ITERATION YUVLine.size=" << YUVLine.size() << " IN fbRowOffset=" << fbRowOffset << "..." << endl;
								SHOULD_BE_EQUAL(pPkt2->GetLocationHorizOffset(), pkt2HOffset);					//	Should match hOffset
							}
						}	//	else YUVLine.size() >= 228
						//cerr << "LINE OFFSET " << fbRowOffset << " (YUVLine " << YUVLine.size() << "):" << endl << pktList << endl;
						YUVLine.pop_back();	//	Lop off last component value
					}	//	while YUVLine has at least 12 components
				}	//	for each VANC line
			}	//	if true

			if (true)
				SHOULD_BE_TRUE (BFT_AncDataCEA608Vanc());

			if (false /* NOT YET READY FOR PRIME TIME */)
				SHOULD_BE_TRUE (BFT_AncDataCEA608Raw());

			if (true)
				SHOULD_BE_TRUE (BFT_AncDataCEA708());

			return true;
		}	//	BFT

};	//	CNTV2AncDataTester


static bool RunAllTests (void)
{
	SHOULD_BE_TRUE(CNTV2AncDataTester::BFT());
	return true;
}


int main (int argc, const char * pArgs [])
{
	(void) argc;
	(void) pArgs;
	return RunAllTests() ? 0 : 501;

}	//	main


#if 0	///////////////	BEGIN TAILING DUMP
	//AJAAncillaryData_Cea608_Line21		line21Packet;
	/*static const uint8_t				p2vuyLumaSamples[]=	{	0x00,	0xFF,	0xFF,	0x61,	0x01,	0x52,	0x96,	0x69,	0x52,	0x4F,	0x67,	0xA9,	0x7E,	0x72,	0xF4,	0xFC,
																0x20,	0x73,	0xF9,	0x00,	0x00,	0xFF,	0x46,	0x28,	0xFE,	0x73,	0x65,	0xFE,	0x6D,	0x69,	0xFE,	0x6E,
																0x61,	0xFE,	0x72,	0x79,	0xFE,	0x00,	0x00,	0xFA,	0x00,	0x00,	0xFA,	0x00,	0x00,	0xFA,	0x00,	0x00,
																0xFA,	0x00,	0x00,	0xFA,	0x00,	0x00,	0xFA,	0x00,	0x00,	0xFA,	0x00,	0x00,	0xFA,	0x00,	0x00,	0xFA,
																0x00,	0x00,	0xFA,	0x00,	0x00,	0xFA,	0x00,	0x00,	0xFA,	0x00,	0x00,	0x73,	0x91,	0xE1,	0x00,	0x00,
																0x00,	0xC1,	0x3F,	0xFF,	0x74,	0xA9,	0x7E,	0xE2,	0xB4,	//	end of packet
																0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10};	*/
	//static const uint8_t				p8_RDD6Pkt2 []	=	{	/*		0x00,	0xFF,	0xFF,	0x45,	0x01,	0x02,*/	0x0F,	0x0A	/*		0xXX	*/	};
	//static const uint16_t				p16_RDD6Pkt2 []	=	{	0x200,	0x20F,	0x20A	};
#endif	///////////////	END TAILING DUMP
