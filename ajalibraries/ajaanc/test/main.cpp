/**
	@file		main.cpp
	@brief		Basic Functionality Tests for the AJA Anc Library.
	@copyright	Copyright (c) 2013-2015 AJA Video Systems, Inc. All rights reserved.
**/
//#define	DEBUG_BREAK_AFTER_FAILURE	(true)
#include "ntv2bft.h"
#include "ancillarydata_cea608_line21.h"
#include "ancillarydata_cea608_vanc.h"

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
		case AJA_STATUS_FALSE:				return "AJA_STATUS_FALSE";
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
	}
	return "<bad AJAStatus>";
}


class CNTV2AncDataTester
{
	public:
		static bool BFT (void)
		{
		#if 1
			{
				AJAAncillaryData *					pDefaultPacket (NULL);
				AJAAncillaryData_Cea608_Line21 *	pLine21Packet (NULL);
				AJAAncillaryDataLocation	nullLocation;
				const UByte					pPayloadBytes []	=	{	0x10,	0x11,	0x12,	0x13,	0x14,	0x15,	0x16,	0x17,	0x18,	0x19,	0x1A,	0x1B,	0x1C,	0x1D,	0x1E,	0x1F,
																		0x20,	0x21,	0x22,	0x23,	0x24,	0x25,	0x26,	0x27,	0x28,	0x29,	0x2A,	0x2B,	0x2C,	0x2D,	0x2E,	0x2F	};
				SHOULD_BE_FALSE (pDefaultPacket);
				pDefaultPacket = AJAAncillaryDataFactory::Create (AJAAncillaryDataType_Unknown);
				SHOULD_BE_TRUE (pDefaultPacket != NULL);
				SHOULD_BE_TRUE (pDefaultPacket);
				SHOULD_BE_FALSE (pDefaultPacket->GotValidReceiveData ());
				SHOULD_BE_EQUAL (pDefaultPacket->GetDID (), 0x00);
				SHOULD_BE_EQUAL (pDefaultPacket->GetSID (), 0x00);
				SHOULD_BE_EQUAL (pDefaultPacket->GetPayloadByteCount (), 0);
				SHOULD_BE_FALSE (pDefaultPacket->GetDataLocation () == nullLocation);
				SHOULD_BE_TRUE (pDefaultPacket->GetDataLocation ().IsValid ());
				SHOULD_BE_TRUE (pDefaultPacket->IsLumaChannel ());
				SHOULD_BE_FALSE (pDefaultPacket->IsChromaChannel ());
				SHOULD_BE_TRUE (pDefaultPacket->IsDigital ());
				SHOULD_BE_FALSE (pDefaultPacket->IsRaw ());
				SHOULD_BE_TRUE (pDefaultPacket->IsVanc ());
				SHOULD_BE_FALSE (pDefaultPacket->IsHanc ());
				SHOULD_BE_EQUAL (pDefaultPacket->GetLocationLineNumber (), 0);
				pDefaultPacket->Print (cerr, true);

				SHOULD_FAIL (pDefaultPacket->SetPayloadData (NULL, sizeof (pPayloadBytes)));
				SHOULD_FAIL (pDefaultPacket->SetPayloadData (pPayloadBytes, 0));
				SHOULD_SUCCEED (pDefaultPacket->SetPayloadData (pPayloadBytes, sizeof (pPayloadBytes)));
				SHOULD_BE_EQUAL (pDefaultPacket->GetPayloadByteCount (), sizeof (pPayloadBytes));
				SHOULD_SUCCEED (pDefaultPacket->SetChecksum (pDefaultPacket->Calculate8BitChecksum ()));
				pDefaultPacket->Print (cerr, true);
				//for (uint32_t n (0);  n < pDefaultPacket->GetPayloadByteCount ();  n++)
				//	SHOULD_BE_EQUAL (pDefaultPacket->GetPayloadByteAtIndex (n), pPayloadBytes [n]);
				SHOULD_BE_EQUAL (AJAAncillaryDataFactory::GuessAncillaryDataType (pDefaultPacket), AJAAncillaryDataType_Unknown);

				pLine21Packet = reinterpret_cast <AJAAncillaryData_Cea608_Line21 *> (AJAAncillaryDataFactory::Create (AJAAncillaryDataType_Cea608_Line21));
				SHOULD_BE_TRUE (pLine21Packet != NULL);
				SHOULD_SUCCEED (pLine21Packet->GeneratePayloadData ());
				SHOULD_SUCCEED (pLine21Packet->SetDataLocation (AJAAncillaryDataLocation (AJAAncillaryDataLink_A, AJAAncillaryDataVideoStream_Y, AJAAncillaryDataSpace_VANC, 21)));
				SHOULD_SUCCEED (pLine21Packet->SetCEA608Bytes (AJAAncillaryData_Cea608::AddOddParity ('A'), AJAAncillaryData_Cea608::AddOddParity ('b')));
				SHOULD_SUCCEED (pLine21Packet->SetDID (AJAAncillaryData_Cea608_Vanc_DID));
				SHOULD_SUCCEED (pLine21Packet->SetSID (AJAAncillaryData_Cea608_Vanc_SID));
				pLine21Packet->Print (cerr, true);
				SHOULD_BE_EQUAL (AJAAncillaryDataFactory::GuessAncillaryDataType (pLine21Packet), AJAAncillaryDataType_Cea608_Line21);
			}
		#endif
			return true;
		}

};	//	CNTV2AncDataTester


static bool RunAllTests (void)
{
	SHOULD_BE_TRUE	(CNTV2AncDataTester::BFT ());
	return true;
}


int main (int argc, const char * pArgs [])
{
	(void) argc;
	(void) pArgs;
	return RunAllTests () ? 0 : 501;

}	//	main
