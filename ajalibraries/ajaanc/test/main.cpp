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
#include <iomanip>

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


class CNTV2AncDataTester
{
	public:
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
			SHOULD_BE_EQUAL (defaultPkt.GetLocationVideoStream(), AJAAncillaryDataVideoStream_Y);
			SHOULD_BE_EQUAL (defaultPkt.GetLocationVideoSpace(), AJAAncillaryDataSpace_VANC);
			SHOULD_BE_EQUAL (defaultPkt.GetLocationLineNumber(), 9);
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
			SHOULD_BE_EQUAL (defaultPkt.GetLocationVideoStream(), AJAAncillaryDataVideoStream_Y);
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
			SHOULD_BE_TRUE (pLine21Packet != NULL);
			SHOULD_SUCCEED (pLine21Packet->GeneratePayloadData());
			SHOULD_SUCCEED (pLine21Packet->SetDataLocation (AJAAncillaryDataLocation(AJAAncillaryDataLink_A, AJAAncillaryDataVideoStream_Y, AJAAncillaryDataSpace_VANC, 21)));
			SHOULD_SUCCEED (pLine21Packet->SetCEA608Bytes (AJAAncillaryData_Cea608::AddOddParity('A'), AJAAncillaryData_Cea608::AddOddParity('b')));
			SHOULD_SUCCEED (pLine21Packet->SetDID(AJAAncillaryData_Cea608_Vanc_DID));
			SHOULD_SUCCEED (pLine21Packet->SetSID(AJAAncillaryData_Cea608_Vanc_SID));
			//pLine21Packet->Print (cerr, true);
			SHOULD_BE_EQUAL (AJAAncillaryDataFactory::GuessAncillaryDataType(pLine21Packet), AJAAncillaryDataType_Cea608_Line21);

			return true;
		}

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
			SHOULD_BE_EQUAL (defaultPkt.GetLocationVideoStream(), AJAAncillaryDataVideoStream_Y);
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
			return true;
		}

		static bool BFT_SMPTEAncData (void)
		{
			static const uint16_t				pv210YSamples [] =	{	0x000,	0x3FF,	0x3FF,	0x161,	0x101,	0x152,	0x296,	0x269,	0x152,	0x14F,	0x167,	0x2A9,	0x27E,	0x272,	0x1F4,	0x2FC,
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
			SHOULD_SUCCEED (pktList.AddVANCData(u16Pkts.front(), 9, AJAAncillaryDataVideoStream_Y));
			cerr << pktList << endl;
			return true;
		}

		static bool BFT (void)
		{
			if (true)
			{
				AJAAncillaryData					defaultPkt;
				AJAAncillaryData *					pDefaultPkt		(NULL);
				AJAAncillaryData *					pClone			(NULL);
				AJAAncillaryData_Cea608_Line21		line21Packet;
				AJAAncillaryDataLocation			nullLocation;
				static const UByte					pTestBytes []	=	{	0x10,	0x11,	0x12,	0x13,	0x14,	0x15,	0x16,	0x17,	0x18,	0x19,	0x1A,	0x1B,	0x1C,	0x1D,	0x1E,	0x1F,
																			0x20,	0x21,	0x22,	0x23,	0x24,	0x25,	0x26,	0x27,	0x28,	0x29,	0x2A,	0x2B,	0x2C,	0x2D,	0x2E,	0x2F	};
				static const uint8_t				p2vuyLumaSamples[]=	{	0x00,	0xFF,	0xFF,	0x61,	0x01,	0x52,	0x96,	0x69,	0x52,	0x4F,	0x67,	0xA9,	0x7E,	0x72,	0xF4,	0xFC,
																			0x20,	0x73,	0xF9,	0x00,	0x00,	0xFF,	0x46,	0x28,	0xFE,	0x73,	0x65,	0xFE,	0x6D,	0x69,	0xFE,	0x6E,
																			0x61,	0xFE,	0x72,	0x79,	0xFE,	0x00,	0x00,	0xFA,	0x00,	0x00,	0xFA,	0x00,	0x00,	0xFA,	0x00,	0x00,
																			0xFA,	0x00,	0x00,	0xFA,	0x00,	0x00,	0xFA,	0x00,	0x00,	0xFA,	0x00,	0x00,	0xFA,	0x00,	0x00,	0xFA,
																			0x00,	0x00,	0xFA,	0x00,	0x00,	0xFA,	0x00,	0x00,	0xFA,	0x00,	0x00,	0x73,	0x91,	0xE1,	0x00,	0x00,
																			0x00,	0xC1,	0x3F,	0xFF,	0x74,	0xA9,	0x7E,	0xE2,	0xB4,	//	end of packet
																			0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10};
				vector<uint8_t>						buffer4K;
				buffer4K.resize(4096, 0x65);
				SHOULD_BE_FALSE (defaultPkt.GotValidReceiveData());
				SHOULD_BE_EQUAL (defaultPkt.GetDID(), 0x00);
				SHOULD_BE_EQUAL (defaultPkt.GetSID(), 0x00);
				SHOULD_BE_TRUE (defaultPkt.IsEmpty());
				SHOULD_BE_EQUAL (defaultPkt.GetPayloadByteCount(), 0);
				SHOULD_BE_EQUAL (defaultPkt.GetPayloadData(), NULL);
				SHOULD_BE_UNEQUAL (defaultPkt.GetDataLocation(), nullLocation);
				SHOULD_BE_TRUE (defaultPkt.GetDataLocation().IsValid());
				SHOULD_BE_TRUE (defaultPkt.IsLumaChannel());
				SHOULD_BE_FALSE (defaultPkt.IsChromaChannel());
				SHOULD_BE_TRUE (defaultPkt.IsDigital());
				SHOULD_BE_FALSE (defaultPkt.IsRaw());
				SHOULD_BE_TRUE (defaultPkt.IsVanc());
				SHOULD_BE_FALSE (defaultPkt.IsHanc());
				SHOULD_BE_EQUAL (defaultPkt.GetLocationLineNumber(), 0);

				SHOULD_BE_EQUAL (pDefaultPkt, NULL);
				pDefaultPkt = AJAAncillaryDataFactory::Create (AJAAncillaryDataType_Unknown);
				SHOULD_BE_TRUE (pDefaultPkt != NULL);
				SHOULD_BE_TRUE (pDefaultPkt);
				SHOULD_BE_FALSE (pDefaultPkt->GotValidReceiveData());
				SHOULD_BE_EQUAL (pDefaultPkt->GetDID(), 0x00);
				SHOULD_BE_EQUAL (pDefaultPkt->GetSID(), 0x00);
				SHOULD_BE_TRUE (pDefaultPkt->IsEmpty());
				SHOULD_BE_EQUAL (pDefaultPkt->GetPayloadByteCount(), 0);
				SHOULD_BE_EQUAL (pDefaultPkt->GetPayloadData(), NULL);
				SHOULD_BE_UNEQUAL (pDefaultPkt->GetDataLocation(), nullLocation);
				SHOULD_BE_TRUE (pDefaultPkt->GetDataLocation().IsValid());
				SHOULD_BE_TRUE (pDefaultPkt->IsLumaChannel());
				SHOULD_BE_FALSE (pDefaultPkt->IsChromaChannel());
				SHOULD_BE_TRUE (pDefaultPkt->IsDigital());
				SHOULD_BE_FALSE (pDefaultPkt->IsRaw());
				SHOULD_BE_TRUE (pDefaultPkt->IsVanc());
				SHOULD_BE_FALSE (pDefaultPkt->IsHanc());
				SHOULD_BE_EQUAL (pDefaultPkt->GetLocationLineNumber(), 0);

				SHOULD_BE_EQUAL	(*pDefaultPkt, defaultPkt);

				SHOULD_FAIL (pDefaultPkt->SetPayloadData (NULL, sizeof(pTestBytes)));
				SHOULD_FAIL (pDefaultPkt->SetPayloadData (pTestBytes, 0));
				SHOULD_SUCCEED (pDefaultPkt->SetPayloadData (pTestBytes, sizeof(pTestBytes)));
				SHOULD_SUCCEED (pDefaultPkt->SetDID(0x40));
				SHOULD_SUCCEED (pDefaultPkt->SetSID(0x02));
				SHOULD_BE_EQUAL (pDefaultPkt->GetDID(), 0x40);
				SHOULD_BE_EQUAL (pDefaultPkt->GetSID(), 0x02);
				SHOULD_BE_FALSE (pDefaultPkt->IsEmpty());
				SHOULD_BE_EQUAL (pDefaultPkt->GetPayloadByteCount(), sizeof(pTestBytes));
				SHOULD_BE_TRUE (pDefaultPkt->GetPayloadData() != NULL);
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

				SHOULD_BE_UNEQUAL (*pDefaultPkt, defaultPkt);
				defaultPkt = *pDefaultPkt;
				SHOULD_BE_EQUAL	(*pDefaultPkt, defaultPkt);
				defaultPkt.SetPayloadByteAtIndex(0x1F, defaultPkt.GetDC()/2);
				SHOULD_BE_UNEQUAL (*pDefaultPkt, defaultPkt);

				pClone = defaultPkt.Clone();
				SHOULD_BE_EQUAL(*pClone, defaultPkt);

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
				SHOULD_BE_EQUAL(pClone->GetLocationVideoStream(), AJAAncillaryDataVideoStream_Y);
				SHOULD_SUCCEED(pClone->SetLocationVideoStream(AJAAncillaryDataVideoStream_C));
				SHOULD_BE_EQUAL(pClone->GetLocationVideoStream(), AJAAncillaryDataVideoStream_C);
				SHOULD_BE_EQUAL(pClone->GetLocationVideoLink(), AJAAncillaryDataLink_A);
				SHOULD_SUCCEED(pClone->SetLocationVideoLink(AJAAncillaryDataLink_B));
				SHOULD_BE_EQUAL(pClone->GetLocationVideoLink(), AJAAncillaryDataLink_B);

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

				//for (uint32_t n (0);  n < pDefaultPkt->GetPayloadByteCount ();  n++)
				//	SHOULD_BE_EQUAL (pDefaultPkt->GetPayloadByteAtIndex(n), pTestBytes[n]);
				SHOULD_BE_EQUAL (AJAAncillaryDataFactory::GuessAncillaryDataType(pDefaultPkt), AJAAncillaryDataType_Unknown);

				SHOULD_BE_TRUE (BFT_AncDataCEA608Vanc());
				//	NOT YET READY FOR PRIME-TIME:			SHOULD_BE_TRUE (BFT_AncDataCEA608Raw());
				SHOULD_BE_TRUE (BFT_AncDataCEA708());

				SHOULD_BE_TRUE (BFT_SMPTEAncData());
			}
			return true;
		}

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
