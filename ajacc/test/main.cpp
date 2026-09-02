/**
	@file		main.cpp
	@brief		Basic Functionality Tests for the AJA Closed Captioning Library.
	@copyright	(C) 2013-2022 AJA Video Systems, Inc. All rights reserved.
**/

#include <doctest.h>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
// need to define this so will work with compilers that don't support thread_local
// ie xcode 6, 7
#define DOCTEST_THREAD_LOCAL

#include "ntv2captiondecodechannel608.h"
#include "ntv2captionencoder608.h"
#include "ntv2captiondecoder608.h"
#include "ntv2captionencoder708.h"
#include "ntv2captiondecoder708.h"
#include "ntv2smpteancdata.h"
#include "ntv2srt.h"
#include "ajaanc/includes/ancillarydata_cea708.h"
#include "ajaanc/includes/ancillarylist.h"
#include "ntv2captionrenderer.h"
#include "ntv2transcode.h"
#include "ntv2testpatterngen.h"
#include "ajabase/system/thread.h"
#include "ajabase/system/systemtime.h"
#include "ajabase/common/timer.h"
#include "ajabase/common/common.h"	//	for aja::join
#include "ajabase/system/debug.h"
#include "ancillarydatafactory.h"
#include <cstring>
#include <iomanip>
#include <utility>	//	std::rel_ops

using namespace std;
using namespace std::rel_ops;

//#define	BURNCAPTIONS_TIMING_TEST

//#define	TEST_WITH_HARDWARE
#if defined(TEST_WITH_HARDWARE)
	#include "ntv2devicescanner.h"
	#include "ajabase/system/process.h"
#endif

#define	LOGMYERROR(__x__)	AJA_sREPORT(AJA_DebugUnit_Testing, AJA_DebugSeverity_Error,		AJAFUNC << ":  " << __x__)
#define	LOGMYWARN(__x__)	AJA_sREPORT(AJA_DebugUnit_Testing, AJA_DebugSeverity_Warning,	AJAFUNC << ":  " << __x__)
#define	LOGMYNOTE(__x__)	AJA_sREPORT(AJA_DebugUnit_Testing, AJA_DebugSeverity_Notice,	AJAFUNC << ":  " << __x__)
#define	LOGMYINFO(__x__)	AJA_sREPORT(AJA_DebugUnit_Testing, AJA_DebugSeverity_Info,		AJAFUNC << ":  " << __x__)
#define	LOGMYDEBUG(__x__)	AJA_sREPORT(AJA_DebugUnit_Testing, AJA_DebugSeverity_Debug,		AJAFUNC << ":  " << __x__)

static const string gTestRows_StandardNorthAmerican [] = {	"ABCDEFGHIJKLMNOPQRSTUVWXYZ",
															"abcdefghijklmnopqrstuvwxyz",
															"0123456789",
															"Exclamation point ! ! !",
															"Inch mark \" \" \"",
															"Sharp # # #",
															"Dollar $ $ $",
															"Percent % % %",
															"Ampersand & & &",
															"Foot mark ' ' '",
															"Open parenthesis ( ( (",
															"Closed parenthesis ) ) )",
															"Plus + + +",
															"Hyphen - - -",
															"Comma , , ,",
															"Period . . .",
															"Forward slash / / /",
															"Colon : : :",
															"Semicolon ; ; ;",
															"Less < < <",
															"Equal = = =",
															"Greater > > >",
															"Question mark ? ? ?",
															"At @ @ @",
															"Open bracket [ [ [",
															"Closed bracket ] ] ]",
															"lower a acute \xC3\xA1 \xC3\xA1 \xC3\xA1",
															"lower e acute \xC3\xA9 \xC3\xA9 \xC3\xA9",
															"lower i acute \xC3\xAD \xC3\xAD \xC3\xAD",
															"lower o acute \xC3\xB3 \xC3\xB3 \xC3\xB3",
															"lower u acute \xC3\xBA \xC3\xBA \xC3\xBA",
															"lower c cedilla \xC3\xA7 \xC3\xA7 \xC3\xA7",
															"Division \xC3\xB7 \xC3\xB7 \xC3\xB7",
															"upper N tilde \xC3\x91 \xC3\x91 \xC3\x91",
															"lower n tilde \xC3\xB1 \xC3\xB1 \xC3\xB1",
															"full block \xE2\x96\x88 \xE2\x96\x88 \xE2\x96\x88",
															""	};

static const string gTestRows_SpecialNorthAmerican [] = {	"registered \xC2\xAE \xC2\xAE \xC2\xAE",
															"degree \xC2\xB0 \xC2\xB0 \xC2\xB0",
															"half \xC2\xBD \xC2\xBD \xC2\xBD",
															"inverted question \xC2\xBF \xC2\xBF \xC2\xBF",
															"trademark \xE2\x84\xA2 \xE2\x84\xA2 \xE2\x84\xA2",
															"cents \xC2\xA2 \xC2\xA2 \xC2\xA2",
															"pound sterling \xC2\xA3 \xC2\xA3 \xC2\xA3",
															"eighth note \xE2\x99\xAA \xE2\x99\xAA \xE2\x99\xAA",
															"lower a grave \xC3\xA0 \xC3\xA0 \xC3\xA0",
															"transparent space \xC2\xA0 \xC2\xA0 \xC2\xA0",
															"lower e grave \xC3\xA8 \xC3\xA8 \xC3\xA8",
															"lower a circumflex \xC3\xA2 \xC3\xA2 \xC3\xA2",
															"lower e circumflex \xC3\xAA \xC3\xAA \xC3\xAA",
															"lower i circumflex \xC3\xAE \xC3\xAE \xC3\xAE",
															"lower o circumflex \xC3\xB4 \xC3\xB4 \xC3\xB4",
															"lower u circumflex \xC3\xBB \xC3\xBB \xC3\xBB",
															""	};

static const string gTestRows_SpanishMisc [] = {			"upper A acute \xC3\x81 \xC3\x81 \xC3\x81",
															"upper E acute \xC3\x89 \xC3\x89 \xC3\x89",
															"upper O acute \xC3\x93 \xC3\x93 \xC3\x93",
															"upper U acute \xC3\x9A \xC3\x9A \xC3\x9A",
															"upper U umlaut \xC3\x9C \xC3\x9C \xC3\x9C",
															"lower u umlaut \xC3\xBC \xC3\xBC \xC3\xBC",
															"opening single quote \xE2\x80\x98 \xE2\x80\x98 \xE2\x80\x98",
															"closing single quote \xE2\x80\x99 \xE2\x80\x99 \xE2\x80\x99",
															"opening double quotes \xE2\x80\x9C \xE2\x80\x9C \xE2\x80\x9C",
															"closing double quotes \xE2\x80\x9D \xE2\x80\x9D \xE2\x80\x9D",
															"inverted exclamation \xC2\xA1 \xC2\xA1 \xC2\xA1",
															"asterisk * * *",
															"em dash \xE2\x80\x94 \xE2\x80\x94 \xE2\x80\x94",
															"copyright \xC2\xA9 \xC2\xA9 \xC2\xA9",
															"service mark \xC2\xAE \xC2\xAE \xC2\xAE",
															"round bullet \xE2\x80\xA2 \xE2\x80\xA2 \xE2\x80\xA2",
															""	};

static const string gTestRows_French [] = {					"upper A grave \xC3\x80 \xC3\x80 \xC3\x80",
															"upper A circumflex \xC3\x82 \xC3\x82 \xC3\x82",
															"upper C cedilla \xC3\x87 \xC3\x87 \xC3\x87",
															"upper E grave \xC3\x88 \xC3\x88 \xC3\x88",
															"upper E circumflex \xC3\x8A \xC3\x8A \xC3\x8A",
															"upper E umlaut \xC3\x8B \xC3\x8B \xC3\x8B",
															"lower e umlaut \xC3\xAB \xC3\xAB \xC3\xAB",
															"upper I circumflex \xC3\x8E \xC3\x8E \xC3\x8E",
															"upper I umlaut \xC3\x8F \xC3\x8F \xC3\x8F",
															"lower i umlaut \xC3\xAF \xC3\xAF \xC3\xAF",
															"upper O circumflex \xC3\x94 \xC3\x94 \xC3\x94",
															"upper U grave \xC3\x99 \xC3\x99 \xC3\x99",
															"lower u grave \xC3\xB9 \xC3\xB9 \xC3\xB9",
															"upper U circumflex \xC3\x9B \xC3\x9B \xC3\x9B",
															"opening guillemet \xC2\xAB \xC2\xAB \xC2\xAB",
															"closing guillemet \xC2\xBB \xC2\xBB \xC2\xBB",
															""	};

static const string gTestRows_Portuguese [] = {				"upper A tilde \xC3\x83 \xC3\x83 \xC3\x83",
															"lower a tilde \xC3\xA3 \xC3\xA3 \xC3\xA3",
															"upper I acute \xC3\x8D \xC3\x8D \xC3\x8D",
															"upper I grave \xC3\x8C \xC3\x8C \xC3\x8C",
															"lower i grave \xC3\xAC \xC3\xAC \xC3\xAC",
															"upper O grave \xC3\x92 \xC3\x92 \xC3\x92",
															"lower o grave \xC3\xB2 \xC3\xB2 \xC3\xB2",
															"upper O tilde \xC3\x95 \xC3\x95 \xC3\x95",
															"lower o tilde \xC3\xB5 \xC3\xB5 \xC3\xB5",
															"opening brace { { {",
															"closing brace } } }",
															"backslash \\ \\ \\",
															"caret ^ ^ ^",
															"underbar _ _ _",
															"pipe | | |",
															"tilde ~ ~ ~",
															""	};

static const string gTestRows_GermanDanish [] = {			"upper A umlaut \xC3\x84 \xC3\x84 \xC3\x84",
															"lower a umlaut \xC3\xA4 \xC3\xA4 \xC3\xA4",
															"upper O umlaut \xC3\x96 \xC3\x96 \xC3\x96",
															"lower o umlaut \xC3\xB6 \xC3\xB6 \xC3\xB6",
															"small sharp s \xC3\x9F \xC3\x9F \xC3\x9F",
															"yen \xC2\xA5 \xC2\xA5 \xC2\xA5",
															"non-spec currency \xC2\xA4 \xC2\xA4 \xC2\xA4",
															"vertical bar | | |",
															"upper A ring \xC3\x85 \xC3\x85 \xC3\x85",
															"lower a ring \xC3\xA5 \xC3\xA5 \xC3\xA5",
															"upper O stroke \xC3\x98 \xC3\x98 \xC3\x98",
															"lower o stroke \xC3\xB8 \xC3\xB8 \xC3\xB8",
															"UL corner \xE2\x94\x8C \xE2\x94\x8C \xE2\x94\x8C",
															"UR corner \xE2\x94\x90 \xE2\x94\x90 \xE2\x94\x90",
															"LL corner \xE2\x94\x94 \xE2\x94\x94 \xE2\x94\x94",
															"LR corner \xE2\x94\x98 \xE2\x94\x98 \xE2\x94\x98",
															""	};

static const string gTestRows_StandardPlainText [] = {		"ABCDEFGHIJKLMNOPQRSTUVWXYZ",
															"abcdefghijklmnopqrstuvwxyz",
															"0123456789",
															"Exclamation point ! ! !",
															"Inch mark \" \" \"",
															"Sharp # # #",
															"Dollar $ $ $",
															"Percent % % %",
															"Ampersand & & &",
															"Foot mark ' ' '",
															"Open parenthesis ( ( (",
															"Closed parenthesis ) ) )",
															"Asterisk * * *",
															"Plus + + +",
															"Comma , , ,",
															"Hyphen - - -",
															"Period . . .",
															"Forward slash / / /",
															"Colon : : :",
															"Semicolon ; ; ;",
															"Less < < <",
															"Equal = = =",
															"Greater > > >",
															"Question mark ? ? ?",
															"At @ @ @",
															"Open bracket [ [ [",
															"Backslash \\ \\ \\",
															"Closed bracket ] ] ]",
															"Caret ^ ^ ^",
															"Underscore _ _ _",
															//"Back Tick ` ` `",
															"Open Brace { { {",
															"Vertical Bar | | |",
															"Closed Brace } } }",
															"Tilde ~ ~ ~",
															""	};

static const string		gTestNames []	=	{	"Standard North American", "Special North American", "Spanish/Misc", "French", "Portuguese", "German/Danish", "" };
static const string *	gTests []		=	{	gTestRows_StandardNorthAmerican, gTestRows_SpecialNorthAmerican, gTestRows_SpanishMisc,
												gTestRows_French, gTestRows_Portuguese, gTestRows_GermanDanish, NULL};

class MyTestThread
{
	private:
		CNTV2CaptionDecoder608Ptr	mDecoder;
		AJAThread *					mThread;
		unsigned					mCounter;
		unsigned					mMyIndexNum;
	public:
		explicit	MyTestThread (CNTV2CaptionDecoder608Ptr inDecoder, const unsigned inIndexNum)
			:	mDecoder (inDecoder), mThread (NULL), mCounter (0), mMyIndexNum (inIndexNum)
			{
				mThread = new AJAThread;
				mThread->Attach (TestThreadStatic, this);
				mThread->SetPriority (AJA_ThreadPriority_Normal);
				mThread->Start ();
			}
		virtual		~MyTestThread ()
		{
			if (mThread)
			{
				while (mThread->Active ())
					AJATime::Sleep (10);
				delete mThread;
				mThread = NULL;
			}
		}
		virtual bool	IsActive (void) const	{if (mThread) return mThread->Active (); else return false;}
		virtual bool	RunMe (void)
		{
			mCounter = 0;
			do
			{
				mDecoder->Reset ();
				AJATime::Sleep ((int32_t (mMyIndexNum) + 1) * 20);
			} while (++mCounter < 60000);
			return true;
		}
	protected:
		static void	TestThreadStatic (AJAThread * pThread, void * pContext)
		{
			(void) pThread;
			MyTestThread *	pApp	(reinterpret_cast <MyTestThread *> (pContext));
			pApp->RunMe ();
		}
};	//	MyTestThread


////////////////////	BEGIN	Deprecated functions copied from ntv2captiondecoder608.cpp	/////////////////////
	static bool DecodeLine21 (UByte & outCC1, UByte & outCC2, const void * pInLine, const NTV2PixelFormat inFormat)
	{
		if (!pInLine)
			return false;	//	NULL line buffer pointer

		bool bResult(false);
		if (inFormat == NTV2_FBF_10BIT_YCBCR)
		{	//	Convert 10-bit line to 8-bit...
			NTV2Buffer line8BitYCbCr(720 * 16 / 8);
			if (line8BitYCbCr)
			{
				bResult = ::ConvertLine_v210_to_2vuy (reinterpret_cast<const ULWord*>(pInLine), line8BitYCbCr, 720);
				if (bResult)
					bResult = CNTV2Line21Captioner::DecodeLine (line8BitYCbCr, outCC1, outCC2);
			}
		}	//	if 10-bit YCbCr
		else if (inFormat == NTV2_FBF_8BIT_YCBCR)
			bResult = CNTV2Line21Captioner::DecodeLine (reinterpret_cast<const UByte*>(pInLine), outCC1, outCC2);

		return bResult;

	}	//	DecodeLine21

	static CaptionData DecodeLine21CaptionData (const UByte *			pInFrameData,
												const NTV2PixelFormat	inPixelFormat,
												const NTV2VideoFormat	inVideoFormat,
												const NTV2FrameGeometry	inFrameGeometry = NTV2_FG_720x486)
	{
		CaptionData	result;
		ULWord lineOffsetF1(1);

		if (!pInFrameData)
			return result;

		if (inFrameGeometry == NTV2_FG_720x486)			lineOffsetF1 = 1;
		else if (inFrameGeometry == NTV2_FG_720x508)	lineOffsetF1 = 23;
		else if (inFrameGeometry == NTV2_FG_720x514)	lineOffsetF1 = 29;
		else return result;

		const ULWord	lineOffsetF2	(lineOffsetF1 + 1);
		const ULWord	bytesPerRow		(::CalcRowBytesForFormat (inPixelFormat, ::GetDisplayWidth(inVideoFormat)));
		const UByte *	pLine21			(pInFrameData + (lineOffsetF1 * bytesPerRow));	//	F1 (CC1 & CC2)
		const UByte *	pLine284		(pInFrameData + (lineOffsetF2 * bytesPerRow));	//	F2 (CC3 & CC4)
		UByte			char1 (0), char2 (0);

		//	Decode field1...
		result.bGotField1Data = DecodeLine21 (char1, char2, pLine21, inPixelFormat);
		result.f1_char1 = char1;
		result.f1_char2 = char2;

		//	Decode field2...
		result.bGotField2Data = DecodeLine21 (char1, char2, pLine284, inPixelFormat);
		result.f2_char1 = char1;
		result.f2_char2 = char2;

		//cerr << "CNTV2CaptionDecoder608::DecodeCaptionData:  " << result << endl;
		return result;

	}	//	DecodeLine21CaptionData
////////////////////	END	Deprecated functions copied from ntv2captiondecoder608.cpp	/////////////////////



class CNTV2608CodecTester
{
	public:
		static bool ThreadTest (void)
		{
			SetDefaultCaptionLogMask (0);
			CNTV2CaptionDecoder608Ptr	decoder608;
			CHECK	(CNTV2CaptionDecoder608::Create (decoder608));
			MyTestThread *	threads []	= {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
			for (unsigned num (0);  num < 10;  num++)
				threads [num] = new MyTestThread (decoder608, num);

			unsigned numActive (0);
			do
			{
				AJATime::Sleep (10000);
				for (unsigned n (0);  n < 10;  n++)
					if (threads [n]->IsActive ())
						numActive++;
			} while (numActive);
			return true;
		}

		static bool BFT (void)
		{
			//	Try driving a 608 decoder from a 608 encoder...
			CaptionData					captionData;
			CNTV2CaptionEncoder608Ptr	enc608;
			CNTV2CaptionDecoder608Ptr	dec608;
			CHECK	(CNTV2CaptionEncoder608::Create(enc608));
			CHECK	(CNTV2CaptionDecoder608::Create(dec608));
			CHECK	(enc608);
			CHECK	(dec608);
			CHECK_EQ (enc608->GetQueuedMessageCount (NTV2_CC608_Field1), 0);
			CHECK_EQ (enc608->GetQueuedMessageCount (NTV2_CC608_Field2), 0);
			CHECK_EQ (dec608->GetDisplayChannel (), NTV2_CC608_CC1);

			const NTV2Line21Channel	testChannels []	=	{	NTV2_CC608_CC1,	NTV2_CC608_CC2,	NTV2_CC608_CC3,	NTV2_CC608_CC4,	NTV2_CC608_ChannelMax	};
			uint8_t *				pYUV10VideoBuffer	(new uint8_t [::GetVideoWriteSize (NTV2_FORMAT_525_5994, NTV2_FBF_10BIT_YCBCR)]);
			uint8_t *				pYUV8VideoBuffer	(new uint8_t [::GetVideoWriteSize (NTV2_FORMAT_525_5994, NTV2_FBF_8BIT_YCBCR)]);
			::memset (pYUV10VideoBuffer, 0, ::GetVideoWriteSize (NTV2_FORMAT_525_5994, NTV2_FBF_10BIT_YCBCR));
			::memset (pYUV8VideoBuffer, 0, ::GetVideoWriteSize (NTV2_FORMAT_525_5994, NTV2_FBF_8BIT_YCBCR));

			if (true)
			{
				NTV2Line21Attrs	normal;
				NTV2Line21Attrs	yellowOnBlackFlashing	(NTV2_CC608_Yellow, NTV2_CC608_Black, NTV2_CC608_Opaque, false, false, true);
				NTV2Line21Attrs	semiTranspBlueOnGreenUnderline	(NTV2_CC608_Blue, NTV2_CC608_Green, NTV2_CC608_SemiTransparent, false, true);
				NTV2Line21Attrs	transpMagentaOnCyanItalic	(NTV2_CC608_Magenta, NTV2_CC608_Cyan, NTV2_CC608_SemiTransparent, true);
				NTV2Line21Attrs	testAttribs []	= {semiTranspBlueOnGreenUnderline, transpMagentaOnCyanItalic, normal, yellowOnBlackFlashing};
				const string	testMessages []	= {"This is channel one", "This is channel two", "This is channel three", "This is channel four"};
				const int		testRows []		= {NTV2_CC608_MaxRow, NTV2_CC608_MaxRow - 1, NTV2_CC608_MaxRow - 2, NTV2_CC608_MaxRow - 3};
				SetDefaultCaptionLogMask (kCaptionLog_DecodeXDS);
				for (unsigned chl (NTV2_CC608_CC1);  chl <= NTV2_CC608_CC4;  chl++)
					CHECK	(enc608->EnqueuePopOnMessage (testMessages[chl], NTV2Line21Channel(chl), testRows[chl], 0, testAttribs[chl]));
				CHECK_EQ (enc608->GetQueuedMessageCount (NTV2_CC608_Field1), 2);
				CHECK_EQ (enc608->GetQueuedMessageCount (NTV2_CC608_Field2), 2);
				CHECK_EQ (enc608->GetQueuedByteCount (NTV2_CC608_Field1), 92);
				CHECK_EQ (enc608->GetQueuedByteCount (NTV2_CC608_Field2), 82);

				CHECK	(enc608->GetNextCaptionData (captionData));
				CHECK_EQ (enc608->GetQueuedMessageCount (NTV2_CC608_Field1), 1);
				CHECK_EQ (enc608->GetQueuedMessageCount (NTV2_CC608_Field2), 1);
					CHECK	(captionData.bGotField1Data);
					CHECK_EQ (captionData.f1_char1, 0x94);
					CHECK_EQ (captionData.f1_char2, ' ');
					CHECK (captionData.bGotField2Data);
					CHECK_EQ (captionData.f2_char1, 0x15);
					CHECK_EQ (captionData.f2_char2, ' ');
					CHECK_FALSE (captionData.bGotField3Data);
					CHECK_EQ (captionData.f3_char1, 0x80);
					CHECK_EQ (captionData.f3_char2, 0x80);
				for (unsigned foo (0);  foo < 20;  foo++)
				{
					CHECK	(dec608->ProcessNew608FrameData (captionData));
					CHECK	(enc608->GetNextCaptionData (captionData));
				}
				CHECK	(dec608->ProcessNew608FrameData (captionData));
				dec608->DebugPrintCurrentScreen (true);
				for (unsigned chan (NTV2_CC608_CC1);  chan <= NTV2_CC608_CC4;  chan++)
				{
					const int firstNonEmptyScreenRow (GetFirstNonEmptyScreenRow (dec608, NTV2Line21Channel(chan)));
					const string rowText (Trim (GetScreenTextAtRow (dec608, testRows[chan], NTV2Line21Channel(chan))));
					if (chan == NTV2_CC608_CC2 || chan == NTV2_CC608_CC4)
						continue;
					CHECK_EQ(testRows[chan], firstNonEmptyScreenRow);
					CHECK_EQ(testMessages[chan], rowText);
				}

				//	Verify CNTV2CaptionEncoder608::EncodeNextCaptionBytesIntoLine21 failure modes...
				CHECK_FALSE (enc608->EncodeNextCaptionBytesIntoLine21 (NULL, NTV2_FBF_8BIT_YCBCR, NTV2_FORMAT_525_5994, NTV2_CC608_Field1));
				CHECK_FALSE (enc608->EncodeNextCaptionBytesIntoLine21 (pYUV8VideoBuffer, NTV2_FBF_ARGB, NTV2_FORMAT_525_5994, NTV2_CC608_Field1));
				CHECK_FALSE (enc608->EncodeNextCaptionBytesIntoLine21 (pYUV8VideoBuffer, NTV2_FBF_8BIT_YCBCR, NTV2_FORMAT_1080i_5994, NTV2_CC608_Field1));
				CHECK_FALSE (enc608->EncodeNextCaptionBytesIntoLine21 (pYUV8VideoBuffer, NTV2_FBF_8BIT_YCBCR, NTV2_FORMAT_525_5994, NTV2_CC608_Field_Invalid));
			}

			//	Test Roll-up Round-Trip Using CC1, CC2, CC3, CC4...
			dec608->SetLogMask (kCaptionLog_608ShowAllScreens | kCaptionLog_608ShowScreen);
			for (unsigned testChannelNdx (0);  testChannels [testChannelNdx] != NTV2_CC608_ChannelMax;  testChannelNdx++)
			{
				const NTV2Line21Channel	chan	(testChannels [testChannelNdx]);

				//	Round-Trip test, roll-up mode:
				cerr << endl << "######### " << ::NTV2Line21ChannelToStr (chan) << " ######### ROLL-UP MODE #########" << endl;
				enc608->Flush (NTV2_CC608_Field1);	enc608->Flush (NTV2_CC608_Field2);	dec608->Reset ();	dec608->SetDisplayChannel (chan);
				CHECK_FALSE (enc608->GetNextCaptionData (captionData));

				for (unsigned testNum (0);  gTests [testNum];  testNum++)
				{
					const string *	pCharSetTestLines	(gTests [testNum]);
					cerr << endl << "--------- " << gTestNames [testNum] << " ---------" << endl;
					for (unsigned lineNum (0);  !pCharSetTestLines [lineNum].empty ();  lineNum++)
					{
						CHECK (enc608->EnqueueRollUpMessage (CUtf8Helpers::Utf8ToCEA608String(pCharSetTestLines[lineNum], chan), NTV2_CC608_CapModeRollUp4, chan));
						while (enc608->GetNextCaptionData (captionData))
							CHECK	(dec608->ProcessNew608FrameData (captionData));
						const string rowTxt (Trim (GetScreenTextAtRow (dec608, 15, chan)));
						CHECK_EQ (rowTxt, pCharSetTestLines [lineNum]);
					}	//	for each non-empty line in the gTestRows array
				}	//	for each character set test

				//	Round-Trip test, roll-up mode, including Line21 waveform encoding/decoding:
				//SetDefaultCaptionLogMask (kCaptionLog_Line21DetectSuccess | kCaptionLog_Line21DetectFail | kCaptionLog_Line21DecodeSuccess | kCaptionLog_Line21DecodeFail);
				//dec608->SetLogMask (kCaptionLog_608ShowScreen | 0x000000000000F000);
				cerr << endl << "######### " << ::NTV2Line21ChannelToStr (chan) << " ######### ROLL-UP MODE #########   W I T H   W A V E F O R M   C O D E C   #########" << endl;
				enc608->Flush (NTV2_CC608_Field1);	enc608->Flush (NTV2_CC608_Field2);	dec608->Reset ();	dec608->SetDisplayChannel (chan);
				CHECK_FALSE (enc608->GetNextCaptionData (captionData));

				for (unsigned testNum (0);  gTests [testNum];  testNum++)
				{
					const string *	pCharSetTestLines	(gTests [testNum]);
					cerr << endl << "--------- " << gTestNames [testNum] << " ---------" << endl;
					for (unsigned lineNum (0);  !pCharSetTestLines [lineNum].empty ();  lineNum++)
					{
						CHECK (enc608->EnqueueRollUpMessage (CUtf8Helpers::Utf8ToCEA608String (pCharSetTestLines [lineNum], chan), NTV2_CC608_CapModeRollUp4, chan));
						while (enc608->GetQueuedByteCount (NTV2_CC608_Field1) || enc608->GetQueuedByteCount (NTV2_CC608_Field2))
						{
							CHECK (enc608->EncodeNextCaptionBytesIntoLine21 (pYUV10VideoBuffer, NTV2_FBF_10BIT_YCBCR, NTV2_FORMAT_525_5994, NTV2_CC608_Field1));
							CHECK (enc608->EncodeNextCaptionBytesIntoLine21 (pYUV10VideoBuffer, NTV2_FBF_10BIT_YCBCR, NTV2_FORMAT_525_5994, NTV2_CC608_Field2));
							captionData = ::DecodeLine21CaptionData (pYUV10VideoBuffer, NTV2_FBF_10BIT_YCBCR, NTV2_FORMAT_525_5994);
							CHECK	(dec608->ProcessNew608FrameData (captionData));
						}	//	loop til encoder queue empty
						const string rowTxt (Trim (GetScreenTextAtRow (dec608, 15, chan)));
						CHECK_EQ (rowTxt, pCharSetTestLines [lineNum]);
					}	//	for each non-empty line in the gTestRows array
				}	//	for each character set test

				//	Round-Trip test, paint-on mode:
				cerr << endl << "######### " << ::NTV2Line21ChannelToStr (chan) << " ######### PAINT-ON MODE #########" << endl;
				enc608->Flush (NTV2_CC608_Field1);	enc608->Flush (NTV2_CC608_Field2);	dec608->Reset ();	dec608->SetDisplayChannel (chan);
				CHECK_FALSE (enc608->GetNextCaptionData (captionData));

				for (unsigned testNum (0);  gTests [testNum];  testNum++)
				{
					const string *	pCharSetTestLines	(gTests [testNum]);
					cerr << endl << "--------- " << gTestNames [testNum] << " ---------" << endl;
					for (unsigned lineNum (0);  !pCharSetTestLines [lineNum].empty ();  lineNum++)
					{
						const string	testMsgStr	(pCharSetTestLines [lineNum]);
						const size_t	numChars	(CUtf8Helpers::Utf8LengthInChars (testMsgStr));
						for (unsigned rowNum (NTV2_CC608_MinRow);  rowNum <= NTV2_CC608_MaxRow;  rowNum++)
						{
							unsigned		colNum		(NTV2_CC608_MinCol);
							CHECK (enc608->EnqueuePaintOnMessage (CUtf8Helpers::Utf8ToCEA608String (testMsgStr, chan), true/*eraseFirst*/, chan, rowNum, colNum));
							while (enc608->GetNextCaptionData (captionData))
								CHECK	(dec608->ProcessNew608FrameData (captionData));
							for (unsigned ndx (0);  ndx < numChars;  ndx++)
							{
								const string	thisChar	(dec608->GetOnAirCharacter (rowNum, colNum + ndx));
								const string	testChar	(CUtf8Helpers::Utf8GetCharacter (testMsgStr, ndx));
								CHECK_EQ (thisChar, testChar);
							}	//	for char index
						}	//	for rowNum
					}	//	for each non-empty line in the gTestRows array
				}	//	for each character set test

				//	Round-Trip test, pop-on mode:
				cerr << endl << "######### " << ::NTV2Line21ChannelToStr (chan) << " ######### POP-ON MODE #########" << endl;
				enc608->Flush (NTV2_CC608_Field1);	enc608->Flush (NTV2_CC608_Field2);	dec608->Reset ();	dec608->SetDisplayChannel (chan);
				CHECK_FALSE (enc608->GetNextCaptionData (captionData));

				for (unsigned testNum (0);  gTests [testNum];  testNum++)
				{
					const string *	pCharSetTestLines	(gTests [testNum]);
					cerr << endl << "--------- " << gTestNames [testNum] << " ---------" << endl;
					for (unsigned lineNum (0);  !pCharSetTestLines [lineNum].empty ();  lineNum++)
					{
						const string	testMsgStr	(pCharSetTestLines [lineNum]);
						const size_t	numChars	(CUtf8Helpers::Utf8LengthInChars (testMsgStr));
						for (unsigned rowNum (NTV2_CC608_MinRow);  rowNum <= NTV2_CC608_MaxRow;  rowNum++)
						{
							unsigned		colNum		(NTV2_CC608_MinCol);
							CHECK (enc608->EnqueuePopOnMessage (CUtf8Helpers::Utf8ToCEA608String (testMsgStr, chan), chan, rowNum, colNum));
							while (enc608->GetNextCaptionData (captionData))
								CHECK	(dec608->ProcessNew608FrameData (captionData));
							for (unsigned ndx (0);  ndx < numChars;  ndx++)
							{
								const string	thisChar	(dec608->GetOnAirCharacter (rowNum, colNum + ndx));
								const string	testChar	(CUtf8Helpers::Utf8GetCharacter (testMsgStr, ndx));
								CHECK_EQ (thisChar, testChar);
							}	//	for char index
						}	//	for rowNum
					}	//	for each non-empty line in the gTestRows array
				}	//	for each character set test
			}	//	for each CC channel

			//	Test Round-Trip Using Tx1, Tx2, Tx3, Tx4...
			const NTV2Line21Channel	textChannels []	=	{	NTV2_CC608_Text1,	NTV2_CC608_Text2,	NTV2_CC608_Text3,	NTV2_CC608_Text4,	NTV2_CC608_ChannelMax	};
			dec608->SetLogMask (kCaptionLog_608ShowAllScreens | kCaptionLog_608ShowScreen);
			const bool	bEraseFirst	(true);
			const bool	bDontErase	(false);
			for (unsigned testChannelNdx (0);  textChannels [testChannelNdx] != NTV2_CC608_ChannelMax;  testChannelNdx++)
			{
				const NTV2Line21Channel	chan	(textChannels [testChannelNdx]);

				//	Round-Trip test, text mode:
				cerr << endl << "######### " << ::NTV2Line21ChannelToStr (chan) << " ######### TEXT MODE #########" << endl;
				enc608->Flush (NTV2_CC608_Field1);	enc608->Flush (NTV2_CC608_Field2);	dec608->Reset ();	dec608->SetDisplayChannel (chan);
				CHECK_FALSE (enc608->GetNextCaptionData (captionData));

				CHECK_FALSE (enc608->EnqueueTextMessage ("This line should fail because of the \t tab character", bDontErase, chan));
				CHECK_FALSE (enc608->EnqueueTextMessage ("This line should fail because of the \n newline character", bDontErase, chan));
				CHECK_FALSE (enc608->EnqueueTextMessage ("This line should fail because of the \r carriage return character", bDontErase, chan));
				const string *	pCharSetTestLines	(gTestRows_StandardPlainText);
				cerr << endl << "--------- Standard Plain Text ---------" << endl;
				for (unsigned lineNum (0);  !pCharSetTestLines [lineNum].empty ();  lineNum++)
				{
					CHECK (enc608->EnqueueTextMessage (pCharSetTestLines [lineNum], lineNum ? bDontErase : bEraseFirst, chan));
					while (enc608->GetNextCaptionData (captionData))
						CHECK	(dec608->ProcessNew608FrameData (captionData));
					const string rowTxt (Trim (GetScreenTextAtRow (dec608, lineNum > 14 ? 15 : lineNum + 1, chan)));
					CHECK_EQ (rowTxt, pCharSetTestLines [lineNum]);
				}	//	for each non-empty line in the gTestRows array
			}	//	for each text channel

			/**
			//	Round-trip test, paint-on mode, every channel, every attribute permutation, row-by-row, column-by-column...
			//	NOTE: THE CEA-608 STANDARD DOESN'T SUPPORT EVERY ATTRIBUTE PERMUTATION, SO THIS TEST NEEDS TO CHANGE TO WHAT THE STANDARD ACTUALLY SUPPORTS.
			//dec608->SetLogMask (kCaptionLog_608ShowScreenAttrs);
			cerr << endl << "#########   P A I N T - O N   M O D E   A N D   A T T R I B U T E   T E S T S   #########" << endl;
			for (unsigned testChannel (0);  testChannel < NTV2_CC608_TextChannelOffset;  testChannel++)
			{
				const NTV2Line21Channel		chan	(static_cast <NTV2Line21Channel> (testChannel));
				enc608->Flush (NTV2_CC608_Field1);	enc608->Flush (NTV2_CC608_Field2);
				dec608->Reset ();
				dec608->SetDisplayChannel (chan);
//dec608->SetLogMask (kCaptionLog_608ShowScreen);
				CHECK_FALSE (enc608->GetNextCaptionData (captionData));
				cerr << endl << endl << endl << endl << endl << endl << endl << "####################### " << ::NTV2Line21ChannelToStr (chan) << endl;
				for (unsigned isFlashing (0);  isFlashing < 2;  isFlashing++)
				{
					cerr << endl << endl << endl << endl << endl << endl << "======================= FLASH " << (isFlashing ? "ON" : "OFF") << endl;
					for (unsigned opacity (NTV2_CC608_Opaque);  opacity < NTV2_CC608_NumOpacities;  opacity++)
					{
						cerr << endl << endl << endl << endl << endl << "~~~~~~~~~~~~~~~~~~~~~~~ OPACITY " << ::NTV2Line21OpacityToStr (NTV2Line21Opacity (opacity)) << endl;
						for (unsigned isUnderline (0);  isUnderline < 2;  isUnderline++)
						{
							cerr << endl << endl << endl << endl << "----------------------- UNDERLINE " << (isUnderline ? "ON" : "OFF") << endl;
							for (unsigned bgColor (NTV2_CC608_White);  bgColor < NTV2_CC608_NumColors;  bgColor++)
							{
								cerr << endl << endl << endl << "....................... BACKGROUND " << ::NTV2Line21ColorToStr (NTV2Line21Color (bgColor)) << endl;
								for (unsigned isItalic (0);  isItalic < 2;  isItalic++)
								{
									cerr << endl << endl << ". . . . . . . . . . . . ITALICS " << (isItalic ? "ON" : "OFF") << endl;
									for (unsigned fgColor (NTV2_CC608_White);  fgColor < NTV2_CC608_NumColors;  fgColor++)
									{
										if (fgColor != bgColor)
										{
											cerr << endl << ".   .   .   .   .   .   FOREGROUND " << ::NTV2Line21ColorToStr (NTV2Line21Color (fgColor)) << endl;
											for (unsigned rowNum (NTV2_CC608_MinRow);  rowNum <= NTV2_CC608_MaxRow;  rowNum++)
											{
												for (unsigned colNum (NTV2_CC608_MinCol);  colNum <= (NTV2_CC608_MaxCol - 20);  colNum++)
												{
													NTV2Line21Attributes	attr	(NTV2Line21Color (fgColor), NTV2Line21Color (bgColor), NTV2Line21Opacity (opacity), isItalic, isUnderline, isFlashing);
													ostringstream			oss;
													oss << attr;
													const string			tstMsgStr	(::NTV2Line21ChannelToStr (chan) + oss.str ());
													CHECK (enc608->EnqueuePaintOnMessage (CUtf8Helpers::Utf8ToCEA608String (tstMsgStr, chan), true , chan, rowNum, colNum, attr));
													while (enc608->GetNextCaptionData (captionData))
														CHECK	(dec608->ProcessNew608FrameData (captionData));
													AJACC_ASSERT (tstMsgStr.length () == 20);
													for (unsigned ndx (0);  ndx < 20;  ndx++)
													{
														NTV2Line21Attributes	thisAttr;
														const string			thisChar (dec608->GetOnAirCharacter (rowNum, colNum + ndx, thisAttr));
														CHECK_EQ (thisChar, tstMsgStr.substr (ndx, 1));
														HOULD_BE_EQUAL (attr, thisAttr);
													}	//	for char index
												}	//	for colNum
											}	//	for rowNum
										}	//	if fgColor != bgColor
									}	//	for fgColor
								}	//	for isItalic
							}	//	for bgColor
						}	//	for isUnderline
					}	//	for opacity
				}	//	for isFlashing
			}	//	for CC1 thru CC4
			cerr << endl << "#########   E N D   P A I N T - O N   M O D E   A N D   A T T R I B U T E   T E S T S   #########" << endl;
			**/

			delete [] pYUV8VideoBuffer;
			delete [] pYUV10VideoBuffer;
			return true;
		}	//	BFT

	private:
		static string	GetScreenTextAtRow (CNTV2CaptionDecoder608Ptr inDecoder, const UWord inRow, const NTV2Line21Channel inChannel = NTV2_CC608_CC1)
		{
			string	result;
			if (inDecoder && IsValidLine21Channel (inChannel) && IsValidLine21Row (inRow))
			{
				const NTV2Line21Channel	oldChannel (inDecoder->GetDisplayChannel ());
				inDecoder->SetDisplayChannel (inChannel);
				for (UWord col (NTV2_CC608_MinCol);  col <= NTV2_CC608_MaxCol;  col++)
				{
					const string utf8Char (inDecoder->GetOnAirCharacter (inRow, col));
					if (utf8Char.empty ())
						result += " ";
					else
						result += utf8Char;
				}
				inDecoder->SetDisplayChannel (oldChannel);
			}
			return result;
		}

		static int	GetFirstNonEmptyScreenRow (CNTV2CaptionDecoder608Ptr inDecoder, const NTV2Line21Channel inChannel)
		{
			const string	kBlankRow	(NTV2_CC608_MaxCol, ' ');
			if (inDecoder && IsValidLine21Channel (inChannel))
			{
				const NTV2Line21Channel	oldChannel (inDecoder->GetDisplayChannel ());
				inDecoder->SetDisplayChannel (inChannel);
				for (UWord row (NTV2_CC608_MinRow);  row <= NTV2_CC608_MaxRow;  row++)
				{
					const string rowText (GetScreenTextAtRow (inDecoder, row, inChannel));
					if (rowText != kBlankRow)
						return row;
				}
				inDecoder->SetDisplayChannel (oldChannel);
			}
			return 0;
		}

		static string Trim (const string & inStr)
		{
			string	result;
			if (!inStr.empty ())
			{
				size_t	pos (0);
				while (pos < inStr.length ())
					if (inStr [pos] != ' ')
						break;
					else
						pos++;
				while (pos < inStr.length ())
					result += inStr [pos++];
				while (!result.empty () && result [result.length () - 1] == ' ')
					result.erase (result.length () - 1);
			}
			return result;
		}

};	//	CNTV2608CodecTester

#define FOO_EIGHT		std::cerr, 16/*radix*/, 1/*bytesPerGroup*/, 128/*groupsPerLine*/, 0/*addressRadix*/, false/*showAscii*/
#define FOO_TWO			std::cerr, 16/*radix*/, 2/*bytesPerGroup*/,  32/*groupsPerLine*/, 0/*addressRadix*/, false/*showAscii*/

class CNTV2708CodecTester
{
	public:
		static bool RoundTripTest608In708 (void)
		{
			//	Round-trip 608-in-708 encode, decode 708-to-608, and compare...
			typedef	vector<CaptionData>	CaptionDataList;
			typedef CaptionDataList::const_iterator	CaptionDataListConstIter;
			CNTV2CaptionEncoder608Ptr	enc608;
			CNTV2CaptionEncoder708Ptr	enc708;
			CNTV2CaptionDecoder708Ptr	dec708;
			const string captionText("Hello, world!");
			CHECK(CNTV2CaptionEncoder608::Create(enc608));
			CHECK(CNTV2CaptionEncoder708::Create(enc708));
			CHECK(CNTV2CaptionDecoder708::Create(dec708));
			CHECK(enc708);
			CHECK(dec708);
			typedef vector<NTV2FrameRate>	NTV2FrameRates;
			typedef NTV2FrameRates::const_iterator	NTV2FRConstIter;
			const NTV2FrameRates AllFRs = {NTV2_FRAMERATE_1498, NTV2_FRAMERATE_1500, NTV2_FRAMERATE_2398,
										NTV2_FRAMERATE_2400, NTV2_FRAMERATE_2500, NTV2_FRAMERATE_2997,
										NTV2_FRAMERATE_3000, NTV2_FRAMERATE_4795, NTV2_FRAMERATE_4800,
										NTV2_FRAMERATE_5000, NTV2_FRAMERATE_5994, NTV2_FRAMERATE_6000,
										NTV2_FRAMERATE_11988, NTV2_FRAMERATE_12000};
			const NTV2FrameRates FRs = {NTV2_FRAMERATE_2398, NTV2_FRAMERATE_2400, //NTV2_FRAMERATE_2500,	** PAL FRAME RATES DON'T ENCODE CEA608 CAPTION DATA BYTES INTO 708 CDP!
										NTV2_FRAMERATE_2997, NTV2_FRAMERATE_3000, //NTV2_FRAMERATE_5000,	** SEE TICKET 4129
										NTV2_FRAMERATE_5994, NTV2_FRAMERATE_6000};
			const uint16_t					kF1PktLineNumCEA708(9);
			const AJAAncDataLoc	kCEA708LocF1(AJAAncDataLink_A,  AJAAncDataChannel_Y,
											 AJAAncDataSpace_VANC,  kF1PktLineNumCEA708,
											 AJAAncDataHorizOffset_AnyVanc);
			LOGMYNOTE("Started");

			for (NTV2FRConstIter pFR(FRs.begin());  pFR != FRs.end();  ++pFR)
			{
				const NTV2FrameRate	frameRate(*pFR);
				CaptionDataList	inputs, outputs;
				CaptionData	captionData;

				enc608->Flush(NTV2_CC608_Field1);  enc608->Flush(NTV2_CC608_Field2);
				enc708->Reset();
				dec708->Reset();

				//	Encode a roll-up "Hello world" caption message...
				LOGMYINFO("Checking " << ::NTV2FrameRateToString(frameRate));
				CHECK(enc608->EnqueueRollUpMessage(CUtf8Helpers::Utf8ToCEA608String(captionText)));

				//	Grab all CaptionData inputs from that caption message...
				while (enc608->GetNextCaptionData(captionData))
					inputs.push_back(captionData);

				//	Generate CEA708 packets for that 608 message...
				AJAAncillaryList	pkts;
				for (CaptionDataListConstIter pCD(inputs.begin());  pCD != inputs.end();  ++pCD)
				{
					AJAAncillaryData_Cea708	pkt708;
					CHECK(enc708->Set608CaptionData(*pCD));
					CHECK(enc708->MakeSMPTE334AncPacket(frameRate, NTV2_CC608_Field1));
					CHECK(AJA_SUCCESS(pkt708.SetFromSMPTE334(enc708->GetSMPTE334Data(),
																		uint32_t(enc708->GetSMPTE334Size()),
																		kCEA708LocF1)));
					CHECK(AJA_SUCCESS(pkts.AddAncillaryData(pkt708)));
				}

				//	Decode the 708 packets to build CaptionData outputs...
				for (uint32_t pktNdx(0);  pktNdx < pkts.CountAncillaryData();  pktNdx++)
				{
					AJAAncillaryData *	pPkt(pkts.GetAncillaryDataAtIndex(pktNdx));
					CHECK(pPkt != NULL);
					bool hasParityErrors(false);
					CHECK(dec708->SetSMPTE334AncData(pPkt->GetPayloadData(), pPkt->GetPayloadByteCount()));
					CHECK(dec708->ParseSMPTE334AncPacket(hasParityErrors));
					CHECK_FALSE(hasParityErrors);
					outputs.push_back(dec708->GetCC608CaptionData());
				}

				//	Compare 708 encoder inputs & 708 decoder outputs...
				CHECK_EQ(inputs.size(), outputs.size());
				for (size_t ndx(0);  ndx < inputs.size();  ndx++)
				{
					const CaptionData input(inputs.at(ndx));
					const CaptionData output(outputs.at(ndx));
					CHECK(input.bGotField1Data);
					CHECK(output.bGotField1Data);
					CHECK_EQ(input.f1_char1, output.f1_char1);
					CHECK_EQ(input.f1_char2, output.f1_char2);
				}	//	for each CaptionData
			}	//	for each supported frame rate

			LOGMYNOTE("Passed");
			return true;
		}	//	RoundTripTest608In708

		static bool RoundTripTest708Native (void)
		{
			//	Drive 708 decoder from native 708 encoder...
			CNTV2CaptionEncoder708Ptr	enc708;
			CNTV2CaptionDecoder708Ptr	decoder708;
			CHECK(CNTV2CaptionEncoder708::Create(enc708));
			CHECK(CNTV2CaptionDecoder708::Create(decoder708));
			CHECK(enc708);
			CHECK(decoder708);
			#if defined(_DEBUG)
				AJADebug::Enable(AJA_DebugUnit_CC708Decode);
				AJADebug::Enable(AJA_DebugUnit_CC708Encode);
			#endif	//	_DEBUG
			CHECK(enc708->GetSMPTE334Data() != nullptr);
			CHECK_EQ(enc708->GetSMPTE334Size(), 0);

			size_t	ndx	(0);
			int		windowID	(0);
			UByte	windowMap	(0x01);	//	Just window 0
			UByte	delayValue	(0x05);
			CC708WindowParms	defaultWindowParams;
			CC708WindowAttr		defaultWindowAttrs;
			CC708PenAttr		defaultPenAttrs;
			CC708PenColor		defaultPenColor;
			CC708PenLocation	defaultPenLocation;

			CHECK	(enc708->MakeNullServiceBlockHeader (ndx, ndx));
enc708->SetCaptionChannelPacketSize(ndx);	cout << "    MakeNullServiceBlockHeader:  "; CNTV2CaptionLogConfig::DumpMemory(enc708->GetCaptionChannelPacket(), enc708->GetCaptionChannelPacketSize(), FOO_EIGHT);

			CHECK	(enc708->MakeServiceBlockCharData (ndx, 'A', ndx));
enc708->SetCaptionChannelPacketSize(ndx);	cout << "      MakeServiceBlockCharData:  "; CNTV2CaptionLogConfig::DumpMemory(enc708->GetCaptionChannelPacket(), enc708->GetCaptionChannelPacketSize(), FOO_EIGHT);

			CHECK	(enc708->MakeDefineWindowCommand (ndx, windowID, defaultWindowParams, ndx));
enc708->SetCaptionChannelPacketSize(ndx);	cout << "       MakeDefineWindowCommand:  "; CNTV2CaptionLogConfig::DumpMemory(enc708->GetCaptionChannelPacket(), enc708->GetCaptionChannelPacketSize(), FOO_EIGHT);

			CHECK	(enc708->MakeClearWindowsCommand (ndx, windowMap, ndx));
enc708->SetCaptionChannelPacketSize(ndx);	cout << "       MakeClearWindowsCommand:  "; CNTV2CaptionLogConfig::DumpMemory(enc708->GetCaptionChannelPacket(), enc708->GetCaptionChannelPacketSize(), FOO_EIGHT);

			CHECK	(enc708->MakeDeleteWindowsCommand (ndx, windowMap, ndx));
enc708->SetCaptionChannelPacketSize(ndx);	cout << "      MakeDeleteWindowsCommand:  "; CNTV2CaptionLogConfig::DumpMemory(enc708->GetCaptionChannelPacket(), enc708->GetCaptionChannelPacketSize(), FOO_EIGHT);

			CHECK	(enc708->MakeDisplayWindowsCommand (ndx, windowMap, ndx));
enc708->SetCaptionChannelPacketSize(ndx);	cout << "     MakeDisplayWindowsCommand:  "; CNTV2CaptionLogConfig::DumpMemory(enc708->GetCaptionChannelPacket(), enc708->GetCaptionChannelPacketSize(), FOO_EIGHT);

			CHECK	(enc708->MakeHideWindowsCommand (ndx, windowMap, ndx));
enc708->SetCaptionChannelPacketSize(ndx);	cout << "        MakeHideWindowsCommand:  "; CNTV2CaptionLogConfig::DumpMemory(enc708->GetCaptionChannelPacket(), enc708->GetCaptionChannelPacketSize(), FOO_EIGHT);

			CHECK	(enc708->MakeToggleWindowsCommand (ndx, windowMap, ndx));
enc708->SetCaptionChannelPacketSize(ndx);	cout << "      MakeToggleWindowsCommand:  "; CNTV2CaptionLogConfig::DumpMemory(enc708->GetCaptionChannelPacket(), enc708->GetCaptionChannelPacketSize(), FOO_EIGHT);

			CHECK	(enc708->MakeSetCurrentWindowCommand (ndx, windowID, ndx));
enc708->SetCaptionChannelPacketSize(ndx);	cout << "   MakeSetCurrentWindowCommand:  "; CNTV2CaptionLogConfig::DumpMemory(enc708->GetCaptionChannelPacket(), enc708->GetCaptionChannelPacketSize(), FOO_EIGHT);

			CHECK	(enc708->MakeSetWindowAttributesCommand (ndx, defaultWindowAttrs, ndx));
enc708->SetCaptionChannelPacketSize(ndx);	cout << "MakeSetWindowAttributesCommand:  "; CNTV2CaptionLogConfig::DumpMemory(enc708->GetCaptionChannelPacket(), enc708->GetCaptionChannelPacketSize(), FOO_EIGHT);

			CHECK	(enc708->MakeSetPenAttributesCommand (ndx, defaultPenAttrs, ndx));
enc708->SetCaptionChannelPacketSize(ndx);	cout << "   MakeSetPenAttributesCommand:  "; CNTV2CaptionLogConfig::DumpMemory(enc708->GetCaptionChannelPacket(), enc708->GetCaptionChannelPacketSize(), FOO_EIGHT);

			CHECK	(enc708->MakeSetPenColorCommand (ndx, defaultPenColor, ndx));
enc708->SetCaptionChannelPacketSize(ndx);	cout << "        MakeSetPenColorCommand:  "; CNTV2CaptionLogConfig::DumpMemory(enc708->GetCaptionChannelPacket(), enc708->GetCaptionChannelPacketSize(), FOO_EIGHT);

			CHECK	(enc708->MakeSetPenLocationCommand (ndx, defaultPenLocation, ndx));
enc708->SetCaptionChannelPacketSize(ndx);	cout << "     MakeSetPenLocationCommand:  "; CNTV2CaptionLogConfig::DumpMemory(enc708->GetCaptionChannelPacket(), enc708->GetCaptionChannelPacketSize(), FOO_EIGHT);

			CHECK	(enc708->MakeDelayCommand (ndx, delayValue, ndx));
enc708->SetCaptionChannelPacketSize(ndx);	cout << "              MakeDelayCommand:  "; CNTV2CaptionLogConfig::DumpMemory(enc708->GetCaptionChannelPacket(), enc708->GetCaptionChannelPacketSize(), FOO_EIGHT);

			CHECK	(enc708->MakeResetCommand (ndx, ndx));
			CHECK	(enc708->SetCaptionChannelPacketSize (ndx));	//	Have to set this manually
enc708->SetCaptionChannelPacketSize(ndx);	cout << "              MakeResetCommand:  "; CNTV2CaptionLogConfig::DumpMemory(enc708->GetCaptionChannelPacket(), enc708->GetCaptionChannelPacketSize(), FOO_EIGHT);
			CHECK	(enc708->MakeSMPTE334AncPacket (NTV2_FRAMERATE_2997, NTV2_CC608_Field1));

			cerr << "mAnc334Data:" << endl;
			CNTV2CaptionLogConfig::DumpMemory (enc708->GetSMPTE334Data(), enc708->GetSMPTE334Size(), FOO_TWO);
			CHECK(decoder708->SetSMPTE334AncData(enc708->GetSMPTE334Data() + 6, enc708->GetSMPTE334Size () - 6));
			bool hasParityErrors (false);
			#if defined (_DEBUG)
				CHECK(decoder708->DebugParseSMPTE334AncPacket(hasParityErrors));
			#endif	//	defined (_DEBUG)
			CHECK(decoder708->ParseSMPTE334AncPacket());
			return true;
		}	//	RoundTripTest708Native

};	//	CNTV2708CodecTester


#if defined (AJA_DEBUG)
	#if defined (TEST_WITH_HARDWARE)
		static CNTV2Card	gDevice;
	#endif	//	defined (TEST_WITH_HARDWARE)

	bool CNTV2CaptionDecoder608::ClassTest (void)
	{
		#if defined (TEST_WITH_HARDWARE)
			CNTV2CaptionDecoder608Ptr	decoder;
			CNTV2DeviceScanner::GetDeviceAtIndex (0, gDevice);
			if (Create (decoder))
				return decoder->InstanceTest ();
			return false;
		#else
			return true;
		#endif
	}

	#if defined (TEST_WITH_HARDWARE)
		static UByte * GetStartBlitAddress (UByte * pInFrameBuffer,
											const UWord inBytesPerVertLine, const UWord inVertLineOffset,
											const UWord inHorzPixelOffset, const UWord inBytesPerHorzPixel)
		{
			UByte *	pResult	(pInFrameBuffer);
			AJACC_ASSERT ((inHorzPixelOffset & 1) == 0);	//	For '2vuy', horizontal pixel offset must be even!!
			pResult += inBytesPerVertLine * inVertLineOffset;
			pResult += inBytesPerHorzPixel * inHorzPixelOffset;
			return pResult;
		}
	#endif	//	TEST_WITH_HARDWARE


	bool CNTV2CaptionDecoder608::InstanceTest (void)
	{
		#if defined (TEST_WITH_HARDWARE)
		{
			CNTV2Card &		device			(gDevice);
			const ULWord	kAppSignature	(NTV2_FOURCC('T','s','t','1'));
			NTV2VideoFormat	videoFormat		(NTV2_FORMAT_525_5994);
			NTV2PixelFormat	pixelFormat		(NTV2_FBF_8BIT_YCBCR);
			NTV2Standard	videoStandard	(::GetNTV2StandardFromVideoFormat (videoFormat));
			NTV2FormatDesc	formatDesc		(videoStandard, pixelFormat);
			ULWord			bytesPerLine	(formatDesc.GetBytesPerRow());
			NTV2Buffer		testPatternBuffer;

			//	Generate the test pattern data...
			NTV2TestPatternGen			testPatternGen;
			testPatternBuffer.Allocate(formatDesc.GetTotalBytes());
			testPatternGen.DrawTestPattern (NTV2_TestPatt_FlatField, formatDesc, testPatternBuffer);
			ULWord						testPatternSize	(testPatternBuffer.GetByteCount());

			//	Show it...
			NTV2Channel			channel			(NTV2_CHANNEL1);
			NTV2TaskMode		savedTaskMode	(NTV2_OEM_TASKS);
			if (!device.IsOpen ())
				return false;

			const NTV2DeviceID deviceID (device.GetDeviceID());
			device.AcquireStreamForApplication (kAppSignature, int32_t(AJAProcess::GetPid()));
			device.GetEveryFrameServices (savedTaskMode);	//	Save the current state before changing it
			device.SetEveryFrameServices (NTV2_OEM_TASKS);		//	Since this is an OEM demo, use the OEM service level
			device.SetReference (NTV2_REFERENCE_FREERUN);
			device.SetFrameBufferFormat (channel, pixelFormat);
			device.EnableChannel (channel);
			device.SetMode (channel, NTV2_MODE_DISPLAY);
			device.SetVideoFormat (videoFormat, false, false, channel);
			device.Connect (NTV2_XptSDIOut1Input, NTV2_XptFrameBuffer1YUV);
			device.SetSDIOutputStandard (channel, videoStandard);
			if (::NTV2DeviceCanDoWidget (deviceID, NTV2_Wgt3GSDIOut2) || ::NTV2DeviceCanDoWidget (deviceID, NTV2_WgtSDIOut2))
			{
				device.Connect (NTV2_XptSDIOut2Input, NTV2_XptFrameBuffer1YUV);
				if (::NTV2DeviceHasBiDirectionalSDI (deviceID))
					device.SetSDITransmitEnable (NTV2_CHANNEL2, true);
				device.SetSDIOutputStandard (NTV2_CHANNEL2, videoStandard);
			}
			if (::NTV2DeviceCanDoWidget (deviceID, NTV2_Wgt3GSDIOut3) || ::NTV2DeviceCanDoWidget (deviceID, NTV2_WgtSDIOut3))
			{
				device.Connect (NTV2_XptSDIOut3Input, NTV2_XptFrameBuffer1YUV);
				if (::NTV2DeviceHasBiDirectionalSDI (deviceID))
					device.SetSDITransmitEnable (NTV2_CHANNEL3, true);
				device.SetSDIOutputStandard (NTV2_CHANNEL3, videoStandard);
			}
			if (::NTV2DeviceCanDoWidget (deviceID, NTV2_Wgt3GSDIOut4) || ::NTV2DeviceCanDoWidget (deviceID, NTV2_WgtSDIOut4))
			{
				device.Connect (NTV2_XptSDIOut4Input, NTV2_XptFrameBuffer1YUV);
				if (::NTV2DeviceHasBiDirectionalSDI (deviceID))
					device.SetSDITransmitEnable (NTV2_CHANNEL4, true);
				device.SetSDIOutputStandard (NTV2_CHANNEL4, videoStandard);
			}
			ULWord	currentOutputFrame	(0);
			CHECK	(device.GetOutputFrame (channel, currentOutputFrame));
			CHECK	(device.DMAWriteFrame (currentOutputFrame, testPatternBuffer, testPatternSize));

			NTV2Line21AttributePermutations	testAttribs;
			unsigned ndx(0);
			CHECK_EQ (testAttribs.size(), 768);

			do
			{
				for (unsigned testNum(0);  gTests[testNum];  testNum++)
				{
					const string *	pCharSetTestLines	(gTests[testNum]);
					for (unsigned lineNum (0);  !pCharSetTestLines [lineNum].empty ();  lineNum++)
					{
						ostringstream	oss;	oss << testAttribs[ndx];
						const string	testStr	(oss.str() + /*testAttribs [ndx].GetHexString() + */ pCharSetTestLines[lineNum]);
						const int		rowNum	((lineNum % NTV2_CC608_MaxRow) + 1);
						if (lineNum % NTV2_CC608_MaxRow == 0)
						{
							CHECK (testPatternGen.DrawTestPattern (NTV2_TestPatt_FlatField, formatDesc, testPatternBuffer));
							CHECK (device.WaitForOutputVerticalInterrupt ());
							CHECK (device.DMAWriteFrame (currentOutputFrame, testPatternBuffer, testPatternSize));
						}
						CHECK (CNTV2CaptionRenderer::BurnString (testStr, testAttribs[ndx++], testPatternBuffer, formatDesc, rowNum, 1));
						CHECK (device.WaitForOutputVerticalInterrupt ());
						CHECK (device.DMAWriteFrame (currentOutputFrame, testPatternBuffer, testPatternSize));
						if (ndx >= 768)
							break;
					}	//	for each non-empty line in the gTestRows array
					CHECK (testPatternGen.DrawTestPattern (NTV2_TestPatt_FlatField, formatDesc, testPatternBuffer));
					//CHECK (device.WaitForOutputVerticalInterrupt ());
					CHECK (device.DMAWriteFrame (currentOutputFrame, testPatternBuffer, testPatternSize));
					if (ndx >= 768)
						break;
				}	//	for each character set test
			} while (ndx < 768);

			CHECK (testPatternGen.DrawTestPattern (NTV2_TestPatt_FlatField, formatDesc, testPatternBuffer));
			CHECK (device.DMAWriteFrame (currentOutputFrame, testPatternBuffer, testPatternSize));
			const ULWord	vLineOffset		(46);
			const ULWord	hPixelOffset	(54 + 18);
			const ULWord	bytesPerPixel	(2);
			ULWord			charRow			(0);
			ULWord			charCol			(0);
			ndx = 0;
			do
			{
				CHECK (NTV2CCFont::GetInstance().RenderGlyph8BitYCbCr (GetStartBlitAddress (testPatternBuffer, bytesPerLine, vLineOffset + 26 * charRow, hPixelOffset + 18 * charCol, bytesPerPixel),
																		bytesPerLine, NTV2GlyphIndex(0x0A), testAttribs[ndx++]));
				//CHECK (device.WaitForOutputVerticalInterrupt ());
				CHECK (device.DMAWriteFrame (currentOutputFrame, testPatternBuffer, testPatternSize));
				if (ndx >= 768)
					break;
				if (++charCol > 31)
				{
					charCol = 0;
					if (++charRow > 14)
					{
						charRow = 0;
						CHECK (testPatternGen.DrawTestPattern (NTV2_TestPatt_FlatField, formatDesc, testPatternBuffer));
						CHECK (device.WaitForOutputVerticalInterrupt ());
						CHECK (device.DMAWriteFrame (currentOutputFrame, testPatternBuffer, testPatternSize));
					}
				}
			} while (true);

			//	Test the CNTV2GlyphCache...
			CNTV2CaptionRendererPtr	renderer (CNTV2CaptionRenderer::GetRenderer(formatDesc));
			CHECK (renderer);
			CHECK_EQ (renderer->IsOpen(), true);
			CHECK (renderer->IsOpen());
			CHECK (renderer->Open());
			CHECK (renderer->IsOpen());
			CHECK_EQ (renderer->IsOpen(), true);
			CHECK (testPatternGen.DrawTestPattern (NTV2_TestPatt_FlatField, formatDesc, testPatternBuffer));
			CHECK (device.DMAWriteFrame (currentOutputFrame, testPatternBuffer, testPatternSize));
			charRow = 0;
			charCol = 0;
			ndx = 0;
			NTV2GlyphIndex			glyphNdx			(1);
			const NTV2GlyphIndex	maxGlyphNdx			(174);
			const UWord				glyphHeightLines	(renderer->GetGlyphHeightInLines());
			const UWord				glyphWidthPixels	(renderer->GetGlyphWidthInPixels());

			do
			{
				const UByte * pGlyphsRaster (renderer->GetPreloadedGlyphsRasterAddress(testAttribs[ndx++]));
				CHECK (::CopyRaster (pixelFormat,										//	inPixelFormat
									testPatternBuffer,									//	pDstBuffer
									bytesPerLine,										//	inDstBytesPerLine
									formatDesc.GetVisibleRasterHeight(),				//	inDstTotalLines
									vLineOffset + 26 * charRow,							//	inDstVertLineOffset
									hPixelOffset + 18 * charCol,						//	inDstHorzPixelOffset
									pGlyphsRaster,										//	pSrcBuffer
									renderer->GetPreloadedGlyphsRasterRowBytes(),		//	inSrcBytesPerLine
									renderer->GetPreloadedGlyphsRasterHeightInLines(),	//	inSrcTotalLines
									0,													//	inSrcVertLineOffset,
									glyphHeightLines,									//	inSrcVertLinesToCopy
									glyphWidthPixels * glyphNdx,						//	inSrcHorzPixelOffset
									glyphWidthPixels));									//	inSrcHorzPixelsToCopy
				//CHECK (device.WaitForOutputVerticalInterrupt ());
				CHECK (device.DMAWriteFrame (currentOutputFrame, testPatternBuffer, testPatternSize));
				if (ndx >= 768)
					break;
				if (++glyphNdx > maxGlyphNdx)
					glyphNdx = 1;
				if (++charCol > 31)
				{
					charCol = 0;
					if (++charRow > 14)
					{
						charRow = 0;
						CHECK (testPatternGen.DrawTestPattern (NTV2_TestPatt_FlatField, formatDesc, testPatternBuffer));
						CHECK (device.WaitForOutputVerticalInterrupt());
						CHECK (device.DMAWriteFrame (currentOutputFrame, testPatternBuffer, testPatternSize));
					}
				}
			} while (true);

			//	Caption full-row/full-column tests for 525/720/1080-sized frames...
			static const NTV2FrameBufferFormat	pixelFormats []	=	{	NTV2_FBF_8BIT_YCBCR,	NTV2_FBF_10BIT_YCBCR,	NTV2_FBF_ARGB,	NTV2_FBF_RGBA, NTV2_FBF_ABGR, NTV2_FBF_NUMFRAMEBUFFERFORMATS };
			static const NTV2VideoFormat		videoFormats []	=	{	NTV2_FORMAT_525_5994,	NTV2_FORMAT_720p_5994,	NTV2_FORMAT_1080i_5994,	/*NTV2_FORMAT_2K_2400, NTV2_FORMAT_4x1920x1080p_2400, NTV2_FORMAT_4x2048x1080p_2400,*/ NTV2_FORMAT_UNKNOWN	};
			static const string	rowText []		=	{	"01:45678901234567890123456789012",	"02:45678901234567890123456789012",	"03:45678901234567890123456789012",	"04:45678901234567890123456789012",
														"05:45678901234567890123456789012",	"06:45678901234567890123456789012",	"07:45678901234567890123456789012",	"08:45678901234567890123456789012",
														"09:45678901234567890123456789012",	"10:45678901234567890123456789012",	"11:45678901234567890123456789012",	"12:45678901234567890123456789012",
														"13:45678901234567890123456789012",	"14:45678901234567890123456789012",	"15:45678901234567890123456789012",	""	};
			static const NTV2Line21Attributes	WhiteOnBlackOpaqueNormal;
			static const NTV2Line21Attributes	YellowOnBlueTranspNormal	(NTV2_CC608_Yellow, NTV2_CC608_Blue, NTV2_CC608_Transparent);
			for (unsigned pfNdx (0);  pixelFormats [pfNdx] != NTV2_FBF_NUMFRAMEBUFFERFORMATS;  pfNdx++)
			{
				pixelFormat = pixelFormats [pfNdx];
				if (!::NTV2DeviceCanDoFrameBufferFormat (device.GetDeviceID (), pixelFormat))
					{cerr << "## WARNING:  Device '" << ::NTV2DeviceIDToString (device.GetDeviceID ()) << "' does not support pixel format '" << ::NTV2FrameBufferFormatToString (pixelFormat) << "'" << endl;  continue;}
				for (unsigned fmtNdx(0);  videoFormats[fmtNdx] != NTV2_FORMAT_UNKNOWN;  fmtNdx++)
				{
					videoFormat		= videoFormats [fmtNdx];
					videoStandard	= ::GetNTV2StandardFromVideoFormat(videoFormat);
					formatDesc		= NTV2FormatDescriptor (::GetNTV2StandardFromVideoFormat(videoFormat), pixelFormat);

					testPatternBuffer.Allocate(formatDesc.GetTotalBytes());
					testPatternGen.DrawTestPattern (NTV2_TestPatt_FlatField, formatDesc, testPatternBuffer);

					if (!::NTV2DeviceCanDoVideoFormat (device.GetDeviceID (), videoFormat))
						{cerr << "## WARNING:  Device '" << ::NTV2DeviceIDToString (device.GetDeviceID ()) << "' does not support video format '" << ::NTV2VideoFormatToString (videoFormat) << "'" << endl;  continue;}
					CHECK (device.SetFrameBufferFormat (channel, pixelFormat));
					CHECK (device.SetVideoFormat (videoFormat, false, false, channel));
					device.WaitForOutputVerticalInterrupt (NTV2_CHANNEL1, 10);		//	Let the device settle after format change

					CHECK (device.SetSDIOutputStandard (channel, videoStandard));
					if (::NTV2DeviceCanDoWidget (deviceID, NTV2_Wgt3GSDIOut2) || ::NTV2DeviceCanDoWidget (deviceID, NTV2_WgtSDIOut2))
						device.SetSDIOutputStandard (NTV2_CHANNEL2, videoStandard);
					if (::NTV2DeviceCanDoWidget (deviceID, NTV2_Wgt3GSDIOut3) || ::NTV2DeviceCanDoWidget (deviceID, NTV2_WgtSDIOut3))
						device.SetSDIOutputStandard (NTV2_CHANNEL3, videoStandard);
					if (::NTV2DeviceCanDoWidget (deviceID, NTV2_Wgt3GSDIOut4) || ::NTV2DeviceCanDoWidget (deviceID, NTV2_WgtSDIOut4))
						device.SetSDIOutputStandard (NTV2_CHANNEL4, videoStandard);

					CHECK (device.DMAWriteFrame (currentOutputFrame, testPatternBuffer, testPatternBuffer.GetByteCount()));
					for (unsigned rowNum (1);  rowNum <= 15;  rowNum++)
						CHECK (CNTV2CaptionRenderer::BurnString (rowText[rowNum-1], ::IsRGBFormat(pixelFormat) ? YellowOnBlueTranspNormal : WhiteOnBlackOpaqueNormal,  testPatternBuffer, formatDesc, rowNum, 1));
					CHECK (device.WaitForOutputVerticalInterrupt());
					CHECK (device.DMAWriteFrame (currentOutputFrame, testPatternBuffer, testPatternBuffer.GetByteCount()));
				}	//	for each video format to test
			}	//	for each pixel format to test

			//	Final test...
			const string burnStr	("1234567890123456789012345678901234567890");
			const string burnStr2	("ABCDEFGHIJKLMNOPQRSTUVWXYZ789012345abcde");
			CNTV2CaptionDecodeChannel608Ptr	decodeChl (mChannelDecoders[0]);
			CHECK(decodeChl);
			CHECK(decodeChl->InstanceTest());
			CHECK(testPatternGen.DrawTestPattern (NTV2_TestPatt_FlatField, formatDesc, testPatternBuffer));
			CHECK(device.DMAWriteFrame (currentOutputFrame, testPatternBuffer, testPatternSize));
			CHECK(BurnCaptions (testPatternBuffer, formatDesc));
			CHECK(device.DMAWriteFrame (currentOutputFrame, testPatternBuffer, testPatternSize));
			CHECK(CNTV2CaptionRenderer::BurnString (burnStr, NTV2Line21Attributes(), testPatternBuffer, formatDesc, 14, 1));
			CHECK(device.DMAWriteFrame (currentOutputFrame, testPatternBuffer, testPatternSize));
			CHECK(CNTV2CaptionRenderer::BurnString (burnStr2, NTV2Line21Attributes(), testPatternBuffer, formatDesc, 14, 1));
			CHECK(device.DMAWriteFrame (currentOutputFrame, testPatternBuffer, testPatternSize));
			CHECK(testPatternGen.DrawTestPattern (NTV2_TestPatt_FlatField, formatDesc, testPatternBuffer));
			CHECK(device.DMAWriteFrame (currentOutputFrame, testPatternBuffer, testPatternSize));
			device.ReleaseStreamForApplication (kAppSignature, int32_t(AJAProcess::GetPid()));
			device.SetEveryFrameServices (savedTaskMode);	//	Restore the previous state
		}
		#endif	//	TEST_WITH_HARDWARE

		return true;
	}


	bool CNTV2CaptionDecodeChannel608::ClassTest (void)
	{
		CHECK_FALSE	(IsValidLine21Row (0));
		CHECK	(IsValidLine21Row (NTV2_CC608_MinRow));
		CHECK	(IsValidLine21Row (NTV2_CC608_MaxRow));
		CHECK_FALSE	(IsValidLine21Row (NTV2_CC608_MaxRow + 1));
		CHECK_FALSE	(IsValidLine21Column (0));
		CHECK	(IsValidLine21Column (NTV2_CC608_MinCol));
		CHECK	(IsValidLine21Column (NTV2_CC608_MaxCol));
		CHECK_FALSE	(IsValidLine21Column (NTV2_CC608_MaxCol + 1));

		CNTV2CaptionDecodeChannel608Ptr	decodeChl;
		CHECK	(CNTV2CaptionDecodeChannel608::Create (decodeChl));
		CHECK	(decodeChl);
		CHECK	(decodeChl->InstanceTest ());
		return true;
	}


	bool CNTV2CaptionDecodeChannel608::InstanceTest (void)
	{
		CHECK	(GetCurrentCharacterSet () == NTV2_CC608_DefaultCharacterSet);

		//	Check that reset worked...
		CHECK_EQ (GetRow (), NTV2_CC608_MaxRow);
		CHECK_EQ (GetColumn (), NTV2_CC608_MinCol);
		string	TestScreen [NTV2_CC608_MaxRow+1][NTV2_CC608_MaxCol+1];
		for (UWord r(NTV2_CC608_MinRow);  r <= NTV2_CC608_MaxRow;  r++)
			for (UWord c(NTV2_CC608_MinCol);  c <= NTV2_CC608_MaxCol;  c++)
				TestScreen[r][c] = " ";
		TestScreen[3][ 5] = "T";	TestScreen[3][ 6] = "h";	TestScreen[3][ 7] = "i";	TestScreen[3][ 8] = "s";
		TestScreen[4][10] = "i";	TestScreen[4][11] = "s";	TestScreen[4][12] = " ";	TestScreen[4][13] = "a";
		TestScreen[5][15] = "T";	TestScreen[5][16] = "e";	TestScreen[5][17] = "s";	TestScreen[5][18] = "t";
		TestScreen[6][20] = "o";	TestScreen[6][21] = "f";	TestScreen[6][22] = " ";	TestScreen[6][23] = "t";	TestScreen [6][24] = "h";	TestScreen [6][25] = "e";
		TestScreen[7][25] = "C";	TestScreen[7][26] = "N";	TestScreen[7][27] = "T";	TestScreen[7][28] = "V";	TestScreen [7][29] = "2";	TestScreen [7][30] = "C";
		TestScreen[7][31] = "a";	TestScreen[7][32] = "p";	TestScreen[7][32] = "t";	TestScreen[7][32] = "i";	TestScreen [7][32] = "o";	TestScreen [7][32] = "n";
		TestScreen[7][32] = "D";	TestScreen[7][32] = "e";	TestScreen[7][32] = "c";	TestScreen[7][32] = "o";	TestScreen [7][32] = "d";	TestScreen [7][32] = "e";
		TestScreen[7][32] = "C";	TestScreen[7][32] = "h";	TestScreen[7][32] = "a";	TestScreen[7][32] = "n";	TestScreen [7][32] = "n";	TestScreen [7][32] = "e";
		TestScreen[7][32] = "l";	TestScreen[7][32] = "6";	TestScreen[7][32] = "0";	TestScreen[7][32] = "8";	TestScreen [7][32] = " ";	TestScreen [7][32] = "c";
		TestScreen[7][32] = "l";	TestScreen[7][32] = "a";	TestScreen[7][32] = "s";	TestScreen[7][32] = "s";

		for (UWord row(NTV2_CC608_MinRow);  row <= NTV2_CC608_MaxRow;  row++)
		{
			for (UWord col(NTV2_CC608_MinCol);  col <= NTV2_CC608_MaxCol;  col++)
			{
				NTV2Line21Attrs	attribs;
				const UByte		theChar (GetOnAirCharacter (row, col, attribs));
				const string	utf8Char (GetOnAirCharacter (row, col));
				CHECK	(theChar == 0x00);
				CHECK_FALSE	(attribs.IsSet());
				CHECK_EQ (utf8Char, string(" "));
			}
		}	//	for each row

		const NTV2Line21Attrs	normal;
		const NTV2Line21Attrs	redOnYellow	(NTV2_CC608_Red,	NTV2_CC608_Yellow,	NTV2_CC608_SemiTransparent);
		const NTV2Line21Attrs	blueOnCyan	(NTV2_CC608_Blue,	NTV2_CC608_Cyan,	NTV2_CC608_Opaque,			true);
		//SetDebugRowsOfInterest (2, 6);	SetDebugColumnsOfInterest (8, 16);
		SetCaptionMode (NTV2_CC608_CapModePaintOn);
		SetRow (3);	SetColumn (5);	InsertCharacter ('T', 0, blueOnCyan);	InsertCharacter ('h', 0, blueOnCyan);	InsertCharacter ('i', 0, blueOnCyan);	InsertCharacter ('s', 0, blueOnCyan);	InsertCharacter (' ', 0, normal);
		SetRow (4);	SetColumn (10);	InsertCharacter ('i', 0);	InsertCharacter ('s', 0);	InsertCharacter (' ', 0);	InsertCharacter ('a', 0);
		SetRow (5);	SetColumn (15);	InsertCharacter ('T', 0, redOnYellow);	InsertCharacter ('e', 0, redOnYellow);	InsertCharacter ('s', 0, redOnYellow);	InsertCharacter ('t', 0, redOnYellow);
		SetRow (6);	SetColumn (20);	InsertCharacter ('o', 0);	InsertCharacter ('f', 0);	InsertCharacter (' ', 0);	InsertCharacter ('t', 0);	InsertCharacter ('h', 0);	InsertCharacter ('e', 0);
		SetRow (7);	SetColumn (25);	InsertCharacter ('C', 0);	InsertCharacter ('N', 0);	InsertCharacter ('T', 0);	InsertCharacter ('V', 0);	InsertCharacter ('2', 0);	InsertCharacter ('C', 0);
					InsertCharacter ('a', 0);	InsertCharacter ('p', 0);	InsertCharacter ('t', 0);	InsertCharacter ('i', 0);	InsertCharacter ('o', 0);
					InsertCharacter ('n', 0);	InsertCharacter ('D', 0);	InsertCharacter ('e', 0);	InsertCharacter ('c', 0);	InsertCharacter ('o', 0);
					InsertCharacter ('e', 0);	InsertCharacter ('C', 0);	InsertCharacter ('h', 0);	InsertCharacter ('a', 0);	InsertCharacter ('n', 0);
					InsertCharacter ('n', 0);	InsertCharacter ('e', 0);	InsertCharacter ('l', 0);	InsertCharacter ('6', 0);	InsertCharacter ('0', 0);
					InsertCharacter ('8', 0);	InsertCharacter (' ', 0);	InsertCharacter ('c', 0);	InsertCharacter ('l', 0);	InsertCharacter ('a', 0);
					InsertCharacter ('s', 0);	InsertCharacter ('s', 0);
		NTV2Line21Attrs	test;
		SetAttributes (5, 15, redOnYellow);
		SetAttributes (5, 17, blueOnCyan);
		cerr << GetDebugPrintScreen (false);
		cerr << GetDebugPrintScreen ();
		GetOnAirCharacter (5, 15, test);
		CHECK_EQ (test, redOnYellow);

		GetOnAirCharacter (5, 17, test);
		CHECK_EQ (test, blueOnCyan);

		for (UWord r(NTV2_CC608_MinRow);  r <= NTV2_CC608_MaxRow;  r++)
		{
			for (UWord c(NTV2_CC608_MinCol);  c <= NTV2_CC608_MaxCol;  c++)
				cerr << TestScreen[r][c];
			cerr << endl;
		}
		for (UWord r(NTV2_CC608_MinRow);  r <= NTV2_CC608_MaxRow;  r++)
			for (UWord c(NTV2_CC608_MinCol);  c <= NTV2_CC608_MaxCol;  c++)
				CHECK_EQ(GetOnAirCharacter (r, c), TestScreen[r][c]);
		return true;
	}
#endif	//	AJA_DEBUG

void caption_data_marker() {}
TEST_SUITE("CaptionData" * doctest::description("CaptionData tests"))
{
	TEST_CASE("Equality")
	{
		CaptionData	f1, f2;
		CHECK(f1 == f2);	//	Identical

		f1.f1_char1 = f1.f1_char2 = 0x9A; f1.bGotField1Data = true;
		f2.f2_char1 = f2.f2_char2 = 0x9A; f2.bGotField2Data = true;
		CHECK(f1 != f2);	//	Yes, different (one has F1 data, other has F2 data)

		f1.f1_char1 = 0x20; f1.f1_char2 = 0x21; f1.bGotField1Data = false;
		f2.f2_char1 = 0x20; f2.f2_char2 = 0x21; f2.bGotField2Data = true;
		CHECK(f1 != f2);	//	Yes, different (same byte pairs, but one says "no data")

		f1.f1_char1 = 0x20; f1.f1_char2 = 0x21; f1.bGotField1Data = true;
		f2.f2_char1 = 0x20; f2.f2_char2 = 0x21; f2.bGotField2Data = false;
		CHECK(f1 != f2);	//	Yes, different (same byte pairs, but one says "no data")

		f1.f1_char1 = 0x20; f1.f1_char2 = 0x21; f1.bGotField1Data = false;
		f2.f2_char1 = 0x22; f2.f2_char2 = 0x23; f2.bGotField2Data = false;
		CHECK_FALSE(f1 != f2);	//	No, essentially the same (different byte pairs, but both say "no data")

		f1.f1_char1 = 0x20; f1.f1_char2 = 0x21; f1.bGotField1Data = true;
		f2.f2_char1 = 0x22; f2.f2_char2 = 0x23; f2.bGotField2Data = true;
		CHECK(f1 != f2);	//	Yes, different (different both "no data")

		//	Need to be able to detect F1 "has signal" and F2 doesn't (or vice-versa)... 
		f1.f1_char1 = f1.f1_char2 = 0x80; f1.bGotField1Data = true;
		f2.f2_char1 = f2.f2_char2 = 0x80; f2.bGotField2Data = true;
		CHECK_FALSE(f1 == f2);
	}
}

void captions_line21_marker() {}
TEST_SUITE("CNTV2Line21Captioner" * doctest::description("Line 21 caption tests"))
{
	TEST_CASE("YUV Conversion")
	{
		CHECK_FALSE(::ConvertLine_2vuy_to_v210 (NULL, NULL, 0));
		CHECK_FALSE(::ConvertLine_v210_to_2vuy (NULL, NULL, 0));
	}
	TEST_CASE("NTV2Line21Attributes")
	{
		CHECK (IsValidLine21Color (NTV2_CC608_White));
		CHECK (IsValidLine21Color (NTV2_CC608_Green));
		CHECK (IsValidLine21Color (NTV2_CC608_Blue));
		CHECK (IsValidLine21Color (NTV2_CC608_Cyan));
		CHECK (IsValidLine21Color (NTV2_CC608_Red));
		CHECK (IsValidLine21Color (NTV2_CC608_Yellow));
		CHECK (IsValidLine21Color (NTV2_CC608_Magenta));
		CHECK (IsValidLine21Color (NTV2_CC608_Black));
		CHECK_FALSE (IsValidLine21Color (NTV2_CC608_NumColors));

		CHECK (IsLine21WhiteColor (NTV2_CC608_White));
		CHECK_FALSE (IsLine21WhiteColor (NTV2_CC608_Green));
		CHECK_FALSE (IsLine21WhiteColor (NTV2_CC608_Blue));
		CHECK_FALSE (IsLine21WhiteColor (NTV2_CC608_Cyan));
		CHECK_FALSE (IsLine21WhiteColor (NTV2_CC608_Red));
		CHECK_FALSE (IsLine21WhiteColor (NTV2_CC608_Yellow));
		CHECK_FALSE (IsLine21WhiteColor (NTV2_CC608_Magenta));
		CHECK_FALSE (IsLine21WhiteColor (NTV2_CC608_Black));
		CHECK_FALSE (IsLine21WhiteColor (NTV2_CC608_NumColors));

		CHECK_FALSE (IsLine21BlackColor (NTV2_CC608_White));
		CHECK_FALSE (IsLine21BlackColor (NTV2_CC608_Green));
		CHECK_FALSE (IsLine21BlackColor (NTV2_CC608_Blue));
		CHECK_FALSE (IsLine21BlackColor (NTV2_CC608_Cyan));
		CHECK_FALSE (IsLine21BlackColor (NTV2_CC608_Red));
		CHECK_FALSE (IsLine21BlackColor (NTV2_CC608_Yellow));
		CHECK_FALSE (IsLine21BlackColor (NTV2_CC608_Magenta));
		CHECK (IsLine21BlackColor (NTV2_CC608_Black));
		CHECK_FALSE (IsLine21BlackColor (NTV2_CC608_NumColors));

		CHECK (IsValidLine21Opacity (NTV2_CC608_Opaque));
		CHECK (IsValidLine21Opacity (NTV2_CC608_SemiTransparent));
		CHECK (IsValidLine21Opacity (NTV2_CC608_Transparent));
		CHECK_FALSE (IsValidLine21Opacity (NTV2_CC608_NumOpacities));

		CHECK_FALSE (IsLine21Transparent (NTV2_CC608_Opaque));
		CHECK_FALSE (IsLine21Transparent (NTV2_CC608_SemiTransparent));
		CHECK (IsLine21Transparent (NTV2_CC608_Transparent));
		CHECK_FALSE (IsLine21Transparent (NTV2_CC608_NumOpacities));

		CHECK_FALSE (IsLine21SemiTransparent (NTV2_CC608_Opaque));
		CHECK (IsLine21SemiTransparent (NTV2_CC608_SemiTransparent));
		CHECK_FALSE (IsLine21SemiTransparent (NTV2_CC608_Transparent));
		CHECK_FALSE (IsLine21SemiTransparent (NTV2_CC608_NumOpacities));

		CHECK (IsLine21Opaque (NTV2_CC608_Opaque));
		CHECK_FALSE (IsLine21Opaque (NTV2_CC608_SemiTransparent));
		CHECK_FALSE (IsLine21Opaque (NTV2_CC608_Transparent));
		CHECK_FALSE (IsLine21Opaque (NTV2_CC608_NumOpacities));

		for (unsigned isFlashing (0);  isFlashing < 2;  isFlashing++)
			for (unsigned opacity (NTV2_CC608_Opaque);  opacity < NTV2_CC608_NumOpacities;  opacity++)
				for (unsigned isUnderline (0);  isUnderline < 2;  isUnderline++)
					for (unsigned bgColor (NTV2_CC608_White);  bgColor < NTV2_CC608_NumColors;  bgColor++)
						for (unsigned isItalic (0);  isItalic < 2;  isItalic++)
							for (unsigned fgColor (NTV2_CC608_White);  fgColor < NTV2_CC608_NumColors;  fgColor++)
								if (true)//fgColor != bgColor)
								{
									NTV2Line21Attributes attr (NTV2Line21Color (fgColor), NTV2Line21Color (bgColor), NTV2Line21Opacity (opacity),
																isItalic ? true : false, isUnderline ? true : false, isFlashing ? true : false);
									ostringstream	oss;
									oss << attr;
									NTV2Line21Attributes	attr2	(::StrToNTV2Line21Attributes (oss.str ()));
									CHECK_EQ (attr, attr2);
								}

		NTV2Line21Attributes	normalA, normalB;

		CHECK (normalA == normalA);
		CHECK_FALSE (normalA != normalA);
		CHECK_FALSE (normalA.IsSet ());
		CHECK (normalA == normalB);
		CHECK_FALSE (normalA != normalB);
			CHECK_FALSE (normalA.IsItalicized ());
			CHECK_FALSE (normalA.IsUnderlined ());
			CHECK_FALSE (normalA.IsFlashing ());
			CHECK (IsLine21WhiteColor (normalA.GetColor ()));
			CHECK (IsLine21BlackColor (normalA.GetBGColor ()));
			CHECK (IsLine21Opaque (normalA.GetOpacity ()));
			CHECK_EQ (normalB.GetHexString (), "   ");
			CHECK_EQ (normalB.GetHexString (), normalA.GetHexString ());
		normalB.AddItalics ();
			CHECK_EQ (normalB.GetHexString (), "008");
			CHECK (normalB.IsSet ());
			CHECK_FALSE (normalA == normalB);
			CHECK (normalA != normalB);
			CHECK_FALSE (normalB == normalA);
			CHECK (normalB != normalA);
			CHECK (normalB.IsItalicized ());
			CHECK_FALSE (normalB.IsUnderlined ());
			CHECK_FALSE (normalB.IsFlashing ());
			CHECK (IsLine21WhiteColor (normalB.GetColor ()));
			CHECK (IsLine21BlackColor (normalB.GetBGColor ()));
			CHECK (IsLine21Opaque (normalB.GetOpacity ()));
			CHECK_EQ (::NTV2Line21AttributesToStr (normalB), "(wht-blk-opq-fIu)");
			CHECK_EQ (normalA.GetHexString (), "   ");
		normalA.AddItalics ();
			CHECK (normalA.IsSet ());
			CHECK (normalA == normalB);
			CHECK_FALSE (normalA != normalB);
			CHECK (normalB == normalA);
			CHECK_FALSE (normalB != normalA);
			CHECK (normalA.IsItalicized ());
			CHECK_FALSE (normalA.IsUnderlined ());
			CHECK_FALSE (normalA.IsFlashing ());
			CHECK (IsLine21WhiteColor (normalA.GetColor ()));
			CHECK (IsLine21BlackColor (normalA.GetBGColor ()));
			CHECK (IsLine21Opaque (normalA.GetOpacity ()));
			CHECK_EQ (::NTV2Line21AttributesToStr (normalA), "(wht-blk-opq-fIu)");
			CHECK_EQ (normalA.GetHexString (), "008");
		normalA.SetColor (NTV2_CC608_Red);
			CHECK_EQ (normalA.GetHexString (), "00C");
		normalA.SetBGColor (NTV2_CC608_Yellow);
			CHECK_EQ (normalA.GetHexString (), "05C");
			CHECK (normalA.IsSet ());
			CHECK_FALSE (normalA == normalB);
			CHECK (normalA != normalB);
			CHECK_FALSE (normalB == normalA);
			CHECK (normalB != normalA);
			CHECK (normalA.IsItalicized ());
			CHECK_FALSE (normalA.IsUnderlined ());
			CHECK_FALSE (normalA.IsFlashing ());
			CHECK_FALSE (IsLine21WhiteColor (normalA.GetColor ()));
			CHECK_FALSE (IsLine21BlackColor (normalA.GetBGColor ()));
			CHECK (IsLine21Opaque (normalA.GetOpacity ()));
			CHECK_EQ (::NTV2Line21AttributesToStr (normalA), "(red-yel-opq-fIu)");
		normalB.AddFlash ();
			CHECK_EQ (normalB.GetHexString (), "808");
		normalB.SetOpacity (NTV2_CC608_SemiTransparent);
			CHECK_EQ (normalB.GetHexString (), "908");
		normalB.AddUnderline ();
			CHECK_EQ (normalB.GetHexString (), "988");
		normalB.RemoveItalics ();
			CHECK_EQ (normalB.GetHexString (), "980");
			CHECK (normalB.IsSet ());
			CHECK_FALSE (normalA == normalB);
			CHECK (normalA != normalB);
			CHECK_FALSE (normalB == normalA);
			CHECK (normalB != normalA);
			CHECK_FALSE (normalB.IsItalicized ());
			CHECK (normalB.IsUnderlined ());
			CHECK (normalB.IsFlashing ());
			CHECK (IsLine21WhiteColor (normalB.GetColor ()));
			CHECK (IsLine21BlackColor (normalB.GetBGColor ()));
			CHECK (IsLine21SemiTransparent (normalB.GetOpacity ()));
			CHECK_EQ (::NTV2Line21AttributesToStr (normalB), "(wht-blk-tsl-FiU)");	
	}
	TEST_CASE("BasicStability")
	{
		UByte c1(0), c2(0);
		CHECK_FALSE(CNTV2Line21Captioner::DecodeLine(AJA_NULL, c1, c2));
		CHECK_EQ(CNTV2Line21Captioner::FindFirstDataBit_NTSC(AJA_NULL), AJA_NULL);
	}	//	TEST_CASE("BasicStability")
}	//	TEST_SUITE("CNTV2Line21Captioner")

void cea608_marker() {}
TEST_SUITE("Captions608" * doctest::description("608 captions tests"))
{
	TEST_CASE("CNTV2608CodecTester")
	{
		CHECK (CNTV2608CodecTester::BFT());
//**MrBill**	CHECK (CNTV2608CodecTester::ThreadTest());
//TODO(paulh): Remove this stuff?
#if defined (AJA_DEBUG)
 		CHECK (CNTV2CaptionDecodeChannel608::ClassTest());
 		CHECK (CNTV2CaptionDecoder608::ClassTest());
#endif	//	defined (AJA_DEBUG)
	}
}

void captions_708_marker() {}
TEST_SUITE("Captions708" * doctest::description("708 captions tests"))
{
	TEST_CASE("CNTV2708CodecTester")
	{
		CHECK (CNTV2708CodecTester::RoundTripTest608In708());
		// CHECK (CNTV2708CodecTester::BFT()); //TODO(paulh): Remove?
	}
}


static bool CheckAllRendererPermutations (void)
{
	static const NTV2PixelFormats pixFmtsToTest = {	NTV2_FBF_10BIT_YCBCR, NTV2_FBF_8BIT_YCBCR, NTV2_FBF_ARGB,
													NTV2_FBF_RGBA, NTV2_FBF_10BIT_RGB, NTV2_FBF_8BIT_YCBCR_YUY2,
													NTV2_FBF_ABGR,	//	NTV2_FBF_10BIT_DPX,   crashes!
																	//	NTV2_FBF_10BIT_YCBCR_DPX,	crashes!
													NTV2_FBF_10BIT_DPX_LE, NTV2_FBF_24BIT_RGB, NTV2_FBF_24BIT_BGR,
													NTV2_FBF_48BIT_RGB};
	static const NTV2StandardSet standardsToTest = {/*NTV2_STANDARD_1080,*/ NTV2_STANDARD_720, NTV2_STANDARD_525,
													NTV2_STANDARD_625, NTV2_STANDARD_1080p, NTV2_STANDARD_2Kx1080p,
													NTV2_STANDARD_3840x2160p, NTV2_STANDARD_4096x2160p,
													NTV2_STANDARD_7680, NTV2_STANDARD_8192};
	static const NTV2Line21AttributePermutations testAttrs;
	static const string tstStr ("AlphaBetaChiDeltaGammaIota");
	UWord rowNum(1), colNum(1);
	NTV2TestPatternGen tpGen;
	NTV2Buffer fb;
	for (NTV2StandardSetConstIter stIt(standardsToTest.begin());  stIt != standardsToTest.end();  ++stIt)
	{	const NTV2Standard st(*stIt);
		for (NTV2PixelFormatsConstIter pfIt(pixFmtsToTest.begin());  pfIt != pixFmtsToTest.end();  ++pfIt)
		{	const NTV2PixelFormat pf(*pfIt);
			NTV2FormatDesc fd (st, pf);

			CHECK(fb.Allocate(fd.GetTotalBytes()));
			CHECK(tpGen.DrawTestPattern (NTV2_TestPatt_FlatField, fd, fb));
			CNTV2CaptionRendererPtr	renderer (CNTV2CaptionRenderer::GetRenderer(fd));
			CHECK(renderer);
			CHECK(renderer->IsOpen());
			for (size_t attrNdx(0);  attrNdx < testAttrs.size();  attrNdx++)
			{	const NTV2Line21Attrs attrs (testAttrs.GetPermutation(attrNdx));
				AJATimer t1(AJATimerPrecisionNanoseconds); t1.Start();
				CHECK(renderer->BurnString (tstStr, attrs, fb, fd, rowNum + attrNdx%15, colNum + attrNdx%31));
				t1.Stop();
				cout << t1.ETSecs() << ": " << attrs << " " << renderer << endl;
			}
			CNTV2CaptionRenderer::FlushGlyphCaches();
		}
	}
	return true;
}

static bool CheckBurnStringSpeed (void)
{
	static const string tstStr ("AlphaBetaChiDeltaGammaIota");
	static const NTV2Line21Attrs defaultAttr;
	UWord rowNum(1), colNum(1);
	NTV2TestPatternGen tpGen;
	NTV2Buffer fb;

	//	Toughest case: 8K 48-bit RGB
	NTV2FormatDesc fd (NTV2_STANDARD_8192, NTV2_FBF_48BIT_RGB);
	CHECK(fb.Allocate(fd.GetTotalBytes()));
	CHECK(tpGen.DrawTestPattern (NTV2_TestPatt_FlatField, fd, fb));
	CNTV2CaptionRendererPtr	renderer (CNTV2CaptionRenderer::GetRenderer(fd));
	CHECK(renderer);
	CHECK(renderer->IsOpen());
	{
		AJATimer t1(AJATimerPrecisionNanoseconds); t1.Start();
		for (size_t ndx(0);  ndx < 16384;  ndx++)
			CHECK(renderer->BurnString (tstStr, defaultAttr, fb, fd, rowNum + ndx%15, colNum + ndx%31));
		t1.Stop();
		cout << (double(tstStr.length()) * double(16384) / t1.ETSecs() / 1000.0) << " glyphs/ms: " << renderer << endl;
	}

	//	Easiest case: SD 8-bit YUV
	fd = NTV2FormatDesc(NTV2_STANDARD_525, NTV2_FBF_8BIT_YCBCR);
	CHECK(fb.Allocate(fd.GetTotalBytes()));
	CHECK(tpGen.DrawTestPattern (NTV2_TestPatt_FlatField, fd, fb));
	renderer = CNTV2CaptionRenderer::GetRenderer(fd);
	CHECK(renderer);
	CHECK(renderer->IsOpen());
	{
		AJATimer t1(AJATimerPrecisionNanoseconds); t1.Start();
		for (size_t ndx(0);  ndx < 16384;  ndx++)
			CHECK(renderer->BurnString (tstStr, defaultAttr, fb, fd, rowNum + ndx%15, colNum + ndx%31));
		t1.Stop();
		cout << (double(tstStr.length()) * double(16384) / t1.ETSecs() / 1000.0) << " glyphs/ms: " << renderer << endl;
	}
	return true;
}

void caption_renderer_marker() {}
TEST_SUITE("CaptionRenderer" * doctest::description("caption renderer tests"))
{
	TEST_CASE("Permutations")
	{
		CHECK(CheckAllRendererPermutations());
	}
	TEST_CASE("BurnStringSpeed")
	{
		CHECK(CheckBurnStringSpeed());
	}
}


//	This sequence of 10-bit YUV component values contains two SD ancillary data packets,
//	as they would appear in a NTV2_FBF_10BIT_YCBCR frame buffer.
//		DID=0x45 SDID=0x01 DC=216 CS=0xD2
//		DID=0x45 SDID=0x02 DC=2   CS=0x62
//	It's used in BFT_YUVComponentsTo10BitYUVPackedBuffer,  BFT_SMPTEAncData
static const vector<uint16_t> SD10BitYUVComponents = {	//	Cb		Y		Cr		Cb		Y		Cr		Cb		Y		Cr		Cb		Y		Cr			Cb		Y		Cr		Cb		Y		Cr		Cb		Y		Cr		Cb		Y		Cr
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

#if defined(_DEBUG)
	#define	DBG_CHECK_EQ(__x__, __y__)		CHECK_EQ((__x__), (__y__))
#else
	#define	DBG_CHECK_EQ(__x__, __y__)	
#endif

void SMPTEAncData_marker() {}
TEST_SUITE("SMPTEAncData" * doctest::description("SMPTEAncData BFT"))
{
	TEST_CASE("BFT_SMPTEAncData")
	{
		const vector<uint16_t> & in10BitYUVReferenceLine(SD10BitYUVComponents);
		static const uint16_t	pv210YSamples [] =	{	0x000,	0x3FF,	0x3FF,	0x161,	0x101,	0x152,	0x296,	0x269,	0x152,	0x14F,	0x167,	0x2A9,	0x27E,	0x272,	0x1F4,	0x2FC,
														0x120,	0x173,	0x2F9,	0x200,	0x200,	0x2FF,	0x146,	0x228,	0x1FE,	0x173,	0x265,	0x1FE,	0x16D,	0x269,	0x1FE,	0x16E,
														0x161,	0x1FE,	0x272,	0x179,	0x1FE,	0x200,	0x200,	0x2FA,	0x200,	0x200,	0x2FA,	0x200,	0x200,	0x2FA,	0x200,	0x200,
														0x2FA,	0x200,	0x200,	0x2FA,	0x200,	0x200,	0x2FA,	0x200,	0x200,	0x2FA,	0x200,	0x200,	0x2FA,	0x200,	0x200,	0x2FA,
														0x200,	0x200,	0x2FA,	0x200,	0x200,	0x2FA,	0x200,	0x200,	0x2FA,	0x200,	0x200,	0x173,	0x191,	0x2E1,	0x200,	0x200,
														0x200,	0x1C1,	0x23F,	0x2FF,	0x274,	0x2A9,	0x27E,	0x2E2,	0x2B4,	//	end of packet
														0x040,	0x040,	0x040,	0x040,	0x040,	0x040,	0x040,	0x040,	0x040,	0x040,	0x040,	0x040,	0x040,	0x040,	0x040,	0x040,	0x040,	0x040,	0x040};
		UWordSequence				v210VancLine;
		UWordVANCPacketList			u16Pkts;
		AJAAncillaryList			pktList;
		AJAAncillaryData *			pPkt	(AJA_NULL);
		AJAAncillaryData_Cea708 *	p708Pkt	(AJA_NULL);
		UWordSequence				u16s;
		uint32_t numConst(0), numDest(0);
		AJAAncillaryData::ResetInstanceCounts();

		///////////////////////////////////////////////////////////////////////	BEGIN TEST SECTION 1
		//	The following 3 tests perform a round-trip validation of:
		//		UWordSequence (aka vector<uint16_t>)
		//			...into...
		//				CNTV2SMPTEAncData::GetAncPacketsFromVANCLine
		//					...resulting in...
		//						UWordVANCPacketList
		//							...into...
		//								AJAAncillaryList::AddVANCData
		//									...resulting in...
		//										AJAAncillaryList containing one AJAAncillaryData packet
		//											...into...
		//												AJAAncillaryDataFactory::GuessAncillaryDataType
		//												AJAAncillaryDataFactory::Create
		//													...resulting in...
		//														AJAAncillaryData_Cea708 instance
		//															...to call...
		//																AJAAncillaryData::GetPayloadData
		//																	...resulting in...
		//																		UWordSequence (aka vector<uint16_t>)

		//	TEST 1:		Y-Channel-only CEA708 PACKET
		for (unsigned ndx(0);  ndx < sizeof(pv210YSamples);  ndx++)
		{
			v210VancLine.push_back(0x040);				//	Chroma
			v210VancLine.push_back(pv210YSamples[ndx]);	//	Luma
		}
		UWordSequence	hOffsets;
		CHECK_FALSE (CNTV2SMPTEAncData::GetAncPacketsFromVANCLine (UWordSequence(), kNTV2SMPTEAncChannel_Y, u16Pkts, hOffsets));	//	This should fail (empty UWordSequence)
		CHECK (u16Pkts.empty());		//	Returned UWordSequence should be empty
		CHECK (CNTV2SMPTEAncData::GetAncPacketsFromVANCLine (v210VancLine, kNTV2SMPTEAncChannel_C, u16Pkts, hOffsets));	//	Should succeed, but no C-channel packets
		CHECK (u16Pkts.empty());		//	Expect no C-channel packets
		CHECK (CNTV2SMPTEAncData::GetAncPacketsFromVANCLine (v210VancLine, kNTV2SMPTEAncChannel_Y, u16Pkts, hOffsets));	//	Should succeed, 1 Y-channel packet
		CHECK_FALSE (u16Pkts.empty());		//	Expect 1 Y-channel packet
		CHECK_EQ (u16Pkts.size(), 1);	//	Expect 1 Y-channel packet
		CHECK(AJA_SUCCESS(pktList.AddVANCData(u16Pkts.front(), AJAAncDataLoc(AJAAncDataLink_A, AJAAncDataChannel_Y, AJAAncDataSpace_VANC, 9))));	//	Make a packet list from it
		CHECK_EQ(pktList.CountAncillaryData(), 1);	//	List should contain 1 packet
		pPkt = pktList.GetAncillaryDataAtIndex(0);			//	Get a pointer to the 1 and only packet
		CHECK(pPkt != AJA_NULL);							//	Pointer should be non-NULL
		CHECK_EQ(AJAAncDataType_Cea708, AJAAncillaryDataFactory::GuessAncillaryDataType(pPkt));	//	Guessed Anc type should be CEA708
		p708Pkt = reinterpret_cast <AJAAncillaryData_Cea708 *> (AJAAncillaryDataFactory::Create(AJAAncDataType_Cea708, pPkt));	//	Make a 708-specific packet instance
		CHECK(p708Pkt != AJA_NULL);								//	708-specific packet instance creation should work
		CHECK(AJA_SUCCESS(p708Pkt->GetPayloadData(u16s)));			//	Get its packet data as uint16_t vector (with parity)
		CHECK_EQ(uint32_t(u16s.size()), p708Pkt->GetDC());	//	Vector element count should match packet data count
		CHECK(size_t(u16s.size()) <= sizeof(pv210YSamples));//	Vector element count should be <= original pkt data count
		for (UWordSequence::size_type ndx(0);  ndx < u16s.size();  ndx++)
			CHECK_EQ(pv210YSamples[ndx+6], u16s.at(ndx));	//	Each element should match original

		//	Start over...
		v210VancLine.clear();	u16Pkts.clear();	u16s.clear();	pktList.Clear();
		delete p708Pkt;			p708Pkt = NULL;		pPkt = AJA_NULL;
		AJAAncillaryData::GetInstanceCounts(numConst, numDest);
		DBG_CHECK_EQ(numConst, 3);	DBG_CHECK_EQ(numDest, 3);

		//	TEST 2:		C-channel-only CEA708 PACKET
		for (unsigned ndx(0);  ndx < sizeof(pv210YSamples);  ndx++)
		{
			v210VancLine.push_back(pv210YSamples[ndx]);	//	Chroma
			v210VancLine.push_back(0x040);				//	Luma
		}
		CHECK (CNTV2SMPTEAncData::GetAncPacketsFromVANCLine (v210VancLine, kNTV2SMPTEAncChannel_Y, u16Pkts, hOffsets));	//	Should succeed, but no Y-channel packets
		CHECK (u16Pkts.empty());		//	Expect no Y-channel packets
		CHECK (CNTV2SMPTEAncData::GetAncPacketsFromVANCLine (v210VancLine, kNTV2SMPTEAncChannel_C, u16Pkts, hOffsets));	//	Should succeed, 1 C-channel packet
		CHECK_FALSE (u16Pkts.empty());		//	Expect 1 C-channel packet
		CHECK_EQ (u16Pkts.size(), 1);	//	Expect 1 C-channel packet
		CHECK(AJA_SUCCESS(pktList.AddVANCData(u16Pkts.front(), AJAAncDataLoc(AJAAncDataLink_A, AJAAncDataChannel_C, AJAAncDataSpace_VANC, 9))));	//	Make a packet list from it
		CHECK_EQ(pktList.CountAncillaryData(), 1);	//	List should contain 1 packet
		pPkt = pktList.GetAncillaryDataAtIndex(0);			//	Get a pointer to the 1 and only packet
		CHECK(pPkt != AJA_NULL);							//	Pointer should be non-NULL
		CHECK_EQ(AJAAncDataType_Cea708, AJAAncillaryDataFactory::GuessAncillaryDataType(pPkt));	//	This used to fail because it's not in Y channel, but changed in SDK 16.1 to succeed & log warning
		CHECK(AJA_SUCCESS(pPkt->GetPayloadData(u16s)));				//	Get its packet data as uint16_t vector (with parity)
		CHECK_EQ(uint32_t(u16s.size()), pPkt->GetDC());		//	Vector element count should match packet data count
		CHECK(size_t(u16s.size()) <= sizeof(pv210YSamples));//	Vector element count should be <= original pkt data count
		for (UWordSequence::size_type ndx(0);  ndx < u16s.size();  ndx++)
			CHECK_EQ(pv210YSamples[ndx+6], u16s.at(ndx));	//	Each element should match original

		//	Start over...
		v210VancLine.clear();	u16Pkts.clear();	u16s.clear();	pktList.Clear();	pPkt = AJA_NULL;
		AJAAncillaryData::GetInstanceCounts(numConst, numDest);
		DBG_CHECK_EQ(numConst, 5);	DBG_CHECK_EQ(numDest, 5);

		//	TEST 3:		Y&C-channel CEA708 PACKET
		for (unsigned ndx(0);  ndx < sizeof(pv210YSamples);  ndx++)
			v210VancLine.push_back(pv210YSamples[ndx]);	//	Both Chroma & Luma
		CHECK (CNTV2SMPTEAncData::GetAncPacketsFromVANCLine (v210VancLine, kNTV2SMPTEAncChannel_Y, u16Pkts, hOffsets));	//	Should succeed, but no Y-channel-only packets
		CHECK (u16Pkts.empty());		//	Expect no Y-channel-only packets
		CHECK (CNTV2SMPTEAncData::GetAncPacketsFromVANCLine (v210VancLine, kNTV2SMPTEAncChannel_C, u16Pkts, hOffsets));	//	Should succeed, but no C-channel-only packets
		CHECK (u16Pkts.empty());		//	Expect no C-channel-only packets
		CHECK (CNTV2SMPTEAncData::GetAncPacketsFromVANCLine (v210VancLine, kNTV2SMPTEAncChannel_Both, u16Pkts, hOffsets));	//	Should succeed, 1 Y&C-channel packet
		CHECK_EQ (u16Pkts.size(), 1);	//	Expect 1 Y&C-channel packet
		CHECK(AJA_SUCCESS(pktList.AddVANCData(u16Pkts.front(), AJAAncDataLoc(AJAAncDataLink_A, AJAAncDataChannel_Both, AJAAncDataSpace_VANC, 9))));	//	Make a packet list from it
		CHECK_EQ(pktList.CountAncillaryData(), 1);	//	List should contain 1 packet
		pPkt = pktList.GetAncillaryDataAtIndex(0);			//	Get a pointer to the 1 and only packet
		CHECK(pPkt != AJA_NULL);							//	Pointer should be non-NULL
		CHECK_EQ(AJAAncDataType_Cea708, AJAAncillaryDataFactory::GuessAncillaryDataType(pPkt));	//	This used to fail, but changed in SDK 16.1 to succeed as CEA708 starting to be carried in SD
		CHECK(AJA_SUCCESS(pPkt->GetPayloadData(u16s)));		//	Get its packet data as uint16_t vector (with parity)
		CHECK_EQ(uint32_t(u16s.size()), pPkt->GetDC());		//	Vector element count should match packet data count
		CHECK(size_t(u16s.size()) <= sizeof(pv210YSamples));//	Vector element count should be <= original pkt data count
		for (UWordSequence::size_type ndx(0);  ndx < u16s.size();  ndx++)
			CHECK_EQ(pv210YSamples[ndx+6], u16s.at(ndx));	//	Each element should match original
		//p708Pkt->Print(cerr, true);
		AJAAncillaryData::GetInstanceCounts(numConst, numDest);
		DBG_CHECK_EQ(numConst, 7);	DBG_CHECK_EQ(numDest, 6);
		///////////////////////////////////////////////////////////////////////	END TEST SECTION 1

		if (false)		//	** MrBill **	NOT QUITE READY FOR PRIME-TIME (SD NOT YET PASSING THIS TEST)
		{
			const uint8_t	TEST_DID	(0xAB);
			const uint8_t	TEST_SID	(0xCD);
			///////////////////////////////////////////////////////////////////////	BEGIN  CNTV2SMPTEAncData::FindAnc  TEST
			//	For each of a few NTV2VideoFormats (SD 525i, HD 720p, HD 1080i)
			//		For each of two NTV2FrameBufferFormats '2vuy' and 'v210' . . .
			//			For each VANC line in "Tall" mode . . .
			//				For each channel C then Y (unless SD, in which case C means "both")...
			//					Generate a test packet
			//					Stuff the test packet into the VANC line
			//					Call FindAnc at the start of the frame buffer...
			//						-	Once looking in the C channel (or "both" channels for SD)
			//						-	If HD, look again in the Y channel
			//					Confirm that FindAnc...
			//						-	found the packet in the channel it was supposed to;
			//							-	on the correct line
			//							-	at the correct pixel offset
			//						-	failed to find the packet in the channel it wasn't supposed to
			//
			const NTV2VideoFormat				VFs[]	=	{NTV2_FORMAT_525_5994, NTV2_FORMAT_720p_5994, NTV2_FORMAT_1080i_5994};
			const NTV2PixelFormat				FBFs[]	=	{NTV2_FBF_8BIT_YCBCR, NTV2_FBF_10BIT_YCBCR};
			const AJAAncDataChannel				CHLs[]	=	{AJAAncDataChannel_C, AJAAncDataChannel_Y};
			const NTV2_SMPTEAncChannelSelect	CHSs[]	=	{kNTV2SMPTEAncChannel_Both, kNTV2SMPTEAncChannel_Y, kNTV2SMPTEAncChannel_C};
			static const string					sCHSs[]	=	{"Y", "C", "Y+C", ""};
			for (unsigned VFndx(0);  VFndx < sizeof(VFs)/sizeof(VFs[0]);  VFndx++)
			{
				const NTV2VideoFormat	vf		(VFs[VFndx]);
				const bool				isSD	(NTV2_IS_SD_VIDEO_FORMAT(vf));
				for (unsigned FBFndx(0);  FBFndx < sizeof(FBFs)/sizeof(FBFs[0]);  FBFndx++)
				{
					const NTV2PixelFormat		fbf	(FBFs[FBFndx]);
					const NTV2FormatDescriptor	fd	(vf, fbf, NTV2_VANCMODE_TALL);
					if (!fd.IsValid()) cerr << ::NTV2FrameBufferFormatToString(fbf) << endl;
					CHECK(fd.IsValid());
					CHECK(fd.IsVANC());
					NTV2Buffer					fb	(size_t(fd.GetTotalRasterBytes() - fd.GetVisibleRasterBytes()));	//	Just VANC lines
					fb.Fill(UWord(0x8080));
cerr << endl << endl << "===========================================================================================================================================" << endl;
AJA_sREPORT(AJA_DebugUnit_SMPTEAnc, AJA_DebugSeverity_Notice,	__FUNCTION__ << ":  " << fd);
cerr << __FUNCTION__ << ":  " << fd << endl;
					for (UWord pktLineOffset(0);  pktLineOffset < fd.GetFirstActiveLine();  pktLineOffset++)
					{
						AJAAncillaryData pkt;
						AJAAncillaryList	pkts;
						ULWord				smpteLine	(0);
						bool				isF2		(false);
						for (unsigned CHLndx(0);  CHLndx < sizeof(CHLs)/sizeof(CHLs[0]);  CHLndx++)
						{
							const AJAAncDataChannel	chan	(CHLs[CHLndx]);
							CHECK(fd.GetSMPTELineNumber(pktLineOffset, smpteLine, isF2));
							CHECK(AJA_SUCCESS(pkt.SetDID(TEST_DID)));	CHECK(AJA_SUCCESS(pkt.SetSID(TEST_SID)));
							CHECK(AJA_SUCCESS(pkt.SetDataCoding(AJAAncDataCoding_Digital)));
							CHECK(AJA_SUCCESS(pkt.SetLocationVideoLink(AJAAncDataLink_A)));
							CHECK(AJA_SUCCESS(pkt.SetLocationHorizOffset(AJAAncDataHorizOffset_AnyVanc)));
							CHECK(AJA_SUCCESS(pkt.SetLocationDataChannel(chan)));
							CHECK(AJA_SUCCESS(pkt.SetLocationLineNumber(smpteLine)));
							const uint8_t		pTestData[]	=	{0xAA, 0xBB, uint8_t(vf), uint8_t(fbf), uint8_t(pkt.GetLocationVideoLink()), uint8_t(pkt.GetLocationDataChannel()), uint8_t(pkt.GetLocationVideoSpace()), uint8_t(smpteLine), uint8_t(pkt.GetDataCoding()), 0xBB, 0xAA};
							CHECK(AJA_SUCCESS(pkt.SetPayloadData (pTestData, sizeof(pTestData))));
							CHECK(AJA_SUCCESS(pkts.AddAncillaryData(pkt)));
//cerr << "Line offset " << pktLineOffset << "(" << smpteLine << ") BEFORE GetVANCTransmitData:" << endl;
//CNTV2CaptionLogConfig::DumpMemory(fd.GetRowAddress(fb.GetHostPointer(), pktLineOffset), fd.GetBytesPerRow(), cerr, 16/*inRadix*/, NTV2_IS_FBF_8BIT(fbf)?1:4/*bytesPerGroup*/,NTV2_IS_FBF_8BIT(fbf)?64:16/*groupsPerRow*/,0/*addressRadix*/,false/*ascii*/);
fb.Fill(UWord(0x8080));	//	** MrBill **	FOR NOW
							CHECK(AJA_SUCCESS(pkts.GetVANCTransmitData (fb,  fd)));
							//	At this point, the packet should be in the frame buffer's VANC area.
AJA_sDEBUG(AJA_DebugUnit_SMPTEAnc, "PKT SHOULD BE FOUND AT lineOffset=" << pktLineOffset << " SMPTELine=" << smpteLine << " chan=" << (isSD?(chan==AJAAncDataChannel_C?"Y+C":"ILLEGAL"):(chan==AJAAncDataChannel_C?"C":"Y")));
cerr << "Line offset " << pktLineOffset << "(" << smpteLine << ") AFTER GetVANCTransmitData:" << endl;
CNTV2CaptionLogConfig::DumpMemory(fd.GetRowAddress(fb.GetHostPointer(), pktLineOffset), fd.GetBytesPerRow(), cerr, 16/*inRadix*/, NTV2_IS_FBF_8BIT(fbf)?1:4/*bytesPerGroup*/,NTV2_IS_FBF_8BIT(fbf)?64:16/*groupsPerRow*/,0/*addressRadix*/,false/*ascii*/);
AJA_sDEBUG(AJA_DebugUnit_SMPTEAnc, ":  PKT SHOULD BE FOUND AT lineOffset=" << pktLineOffset << " SMPTELine=" << smpteLine << " chan=" << (isSD?(chan==AJAAncDataChannel_C?"Y+C":"ILLEGAL"):(chan==AJAAncDataChannel_C?"C":"Y")));
vector<uint16_t>	u16pktComponents;

CHECK(AJA_SUCCESS(pkt.GenerateTransmitData(u16pktComponents)));
cerr <<  "U16 Packet Components returned from GenerateTransmitData:" << endl;  for(unsigned n(0);  n < u16pktComponents.size();  n++)	cerr << " " << HEX0N(u16pktComponents[n],4); ;

//	Round-trip test:
{
	AJAAncillaryList	compPkts;
	UWordSequence		uwords;
	unsigned			ndx	(0);
	if (NTV2_IS_FBF_8BIT(fbf))
		CNTV2SMPTEAncData::UnpackLine_8BitYUVtoUWordSequence (fd.GetRowAddress(fb.GetHostPointer(), pktLineOffset), uwords, fd.GetRasterWidth());
	else
		::UnpackLine_10BitYUVtoUWordSequence (fd.GetRowAddress(fb.GetHostPointer(), pktLineOffset), uwords, fd.GetRasterWidth());
//cerr << "UWordSequence returned from " << (NTV2_IS_FBF_8BIT(fbf)?"CNTV2SMPTEAncData::UnpackLine_8BitYUVtoUWordSequence:":"UnpackLine_10BitYUVtoUWordSequence:") << endl << uwords << endl;

	UWordVANCPacketList	cPackets, yPackets;
	UWordSequence		cHOffsets, yHOffsets;
	CNTV2SMPTEAncData::GetAncPacketsFromVANCLine (uwords, isSD ? kNTV2SMPTEAncChannel_Both : kNTV2SMPTEAncChannel_C, cPackets, cHOffsets);
	if (!isSD)
		CNTV2SMPTEAncData::GetAncPacketsFromVANCLine (uwords, kNTV2SMPTEAncChannel_Y, yPackets, yHOffsets);
	AJAAncDataLoc loc	(AJAAncDataLink_Unknown, isSD ? AJAAncDataChannel_Both : AJAAncDataChannel_C,
						AJAAncDataSpace_VANC, uint16_t(smpteLine));
	for (UWordVANCPacketListConstIter it(cPackets.begin());  it != cPackets.end();  ++it, ndx++)
		compPkts.AddVANCData (*it, loc.SetHorizontalOffset(cHOffsets[ndx]));
	ndx = 0;
	loc.SetDataChannel(AJAAncDataChannel_Y);
	for (UWordVANCPacketListConstIter it(yPackets.begin());  it != yPackets.end();  ++it, ndx++)
		compPkts.AddVANCData (*it, loc.SetHorizontalOffset(yHOffsets[ndx]));
AJA_sDEBUG(AJA_DebugUnit_SMPTEAnc, "cPackets: " << cPackets);
AJA_sDEBUG(AJA_DebugUnit_SMPTEAnc, "yPackets: " << yPackets);
AJA_sDEBUG(AJA_DebugUnit_SMPTEAnc, "compPkts: " << compPkts);
	CHECK_EQ(compPkts.CountAncillaryDataWithID (TEST_DID, TEST_SID), 1);
	AJAAncillaryData *	pCompPkt	(compPkts.GetAncillaryDataWithID (TEST_DID, TEST_SID));
	CHECK(pCompPkt != AJA_NULL);
	CHECK_EQ(*pCompPkt, pkt);
}

							//	Search for the packet using FindAnc...
							if (false)	//	** MrBill **	FOR NOW
							for (UWord srchLineOffset(0);  srchLineOffset < fd.GetFirstActiveLine();  srchLineOffset++)
							{
								for (unsigned CHSndx(0);  CHSndx < sizeof(CHSs)/sizeof(CHSs[0]);  CHSndx++)
								{
									const NTV2_SMPTEAncChannelSelect	srchChan(CHSs[CHSndx]);
									//UWord	wordBuffer[256];
									UWordSequence	wordBuffer;
									ULWord	numWordsCopied	(0);
									UWord	lineOffset		(srchLineOffset);
									UWord	pixelOffset		(0);
									ULWord	foundSmpteLine	(0);
									bool	foundIsF2		(false);
									bool	hasParityErrors(false);
									bool	bFound	(CNTV2SMPTEAncData::FindAnc (pkt.GetDID(),				//	inAncDID
																				pkt.GetSID(),				//	inAncSID
																				fb,							//	inFrameBuffer
																				fd,							//	inFormatDesc
																				srchChan,					//	inAncChannel
																				wordBuffer,					//	outWords
																				hasParityErrors,			//	outHasParityErrors
																				1,							//	inLineIncrement
																				lineOffset,					//	inOutLineStart
																				pixelOffset));				//	inOutPixelStart
/*										bool	bFound	(CNTV2SMPTEAncData::FindAnc (pkt.GetDID(),				//	inAncDID
																				pkt.GetSID(),				//	inAncSID
																				reinterpret_cast<const ULWord*>(fb.GetHostPointer()),	//	pInFrameBuffer
																				srchChan,					//	inAncChannel
																				vf,							//	inVideoFormat
																				fbf,						//	inFBFormat
																				wordBuffer,					//	pOutBuff
																				numWordsCopied,				//	outWordCount
																				ULWord(sizeof(wordBuffer)),	//	inWordCountMax
																				hasParityErrors,			//	outHasParityErrors
																				1,							//	inLineIncrement
																				lineOffset,					//	inOutLineStart
																				pixelOffset));				//	inOutPixelStart
*/										CHECK(fd.GetSMPTELineNumber (lineOffset, foundSmpteLine, foundIsF2));
									bool	isAMatch	= foundSmpteLine == smpteLine
															&&	pktLineOffset == lineOffset
															&&	numWordsCopied == pkt.GetDC();
AJA_sREPORT(AJA_DebugUnit_SMPTEAnc, AJA_DebugSeverity_Notice,	__FUNCTION__ << ":  " << (bFound?"FOUND":"NOT FOUND") << ": srchCh=" << sCHSs[srchChan] << " srchLn=" << srchLineOffset << " ln=" << lineOffset << " px=" << pixelOffset << " words=" << numWordsCopied);
cerr << __FUNCTION__ << ": " << (bFound?"FOUND":"NOT FOUND") << ": srchCh=" << sCHSs[srchChan] << " srchLn=" << srchLineOffset << " ln=" << lineOffset << " px=" << pixelOffset << " words=" << numWordsCopied << endl;
									//CHECK_EQ(bFound, isAMatch);
									if (bFound && isAMatch)
										cerr << "HOORAY!" << endl;
								}	//	vary the search channel select
								//break;	//	** MrBill **	FOR NOW
							}	//	vary the starting search line offset
							pkts.Clear();
							pkt.Clear();
							if (NTV2_IS_SD_VIDEO_FORMAT(vf))
								break;	//	For SD, only one channel: AJAAncDataChannel_Both == AJAAncDataChannel_C
						}	//	permute Y+C/Y/C channel
					}	//	permute line offset
				}	//	permute FBF
			}	//	permute VF
			///////////////////////////////////////////////////////////////////////	END  CNTV2SMPTEAncData::FindAnc  TEST
		}	//	if test FindAnc

		if (true)
		{
			//	Start over:
			v210VancLine.clear();	u16Pkts.clear();	u16s.clear();	pktList.Clear();	pPkt = AJA_NULL;

			//	Test CNTV2SMPTEAncData::GetAncPacketsFromVANCLine...
			UWordSequence	hOffsets;
			AJAAncDataLoc	loc	(AJAAncDataLink_A, AJAAncDataChannel_Both, AJAAncDataSpace_VANC, 9);
			::SetDefaultCaptionLogOutputStream(cerr);
			//::SetDefaultCaptionLogMask(kCaptionLog_SMPTEAncErrors | kCaptionLog_SMPTEAncSuccess | kCaptionLog_SMPTEAncDebug);
			CHECK(CNTV2SMPTEAncData::GetAncPacketsFromVANCLine (in10BitYUVReferenceLine, kNTV2SMPTEAncChannel_Both, u16Pkts, hOffsets));
			CHECK_EQ(u16Pkts.size(), 2);
			CHECK_EQ(u16Pkts.size(), hOffsets.size());
			const UWordSequence	u16s_1	(u16Pkts.at(0));
			const UWordSequence	u16s_2	(u16Pkts.at(1));
			CHECK_EQ(u16s_1.size(), 223);
			CHECK_EQ(u16s_2.size(), 9);
			//cerr << "BFT_SMPTEAncData:  PACKETS:  " << u16Pkts << endl
			//	 << "HOFFSETS:  " << hOffsets << endl
			//	 << "RESULTING PACKET LIST:" << endl;
			AJAAncillaryData::GetInstanceCounts(numConst, numDest);
			DBG_CHECK_EQ(numConst, 7);	DBG_CHECK_EQ(numDest, 7);
			for (UWordVANCPacketList::size_type ndx(0);  ndx < u16Pkts.size();  ndx++)	//	+2 more pkt instances ==> 11
				CHECK(AJA_SUCCESS(pktList.AddVANCData(u16Pkts.at(ndx), loc.SetHorizontalOffset(hOffsets.at(ndx)))));	//	Add to packet list
			//pktList.Print(cerr, true) << endl;
			AJAAncillaryData::GetInstanceCounts(numConst, numDest);
			DBG_CHECK_EQ(numConst, 11);	DBG_CHECK_EQ(numDest, 9);
			pktList.Clear();
			AJAAncillaryData::GetInstanceCounts(numConst, numDest);
			DBG_CHECK_EQ(numDest, 11);
		}
	}	//	TEST_CASE("BFT_SMPTEAncData")
}	//	TEST_SUITE("SMPTEAnc")



void SubRipText_marker() {}
TEST_SUITE("SRT" * doctest::description("SubRipText Tests"))
{
	TEST_CASE("SubRipText")
	{
		SUBCASE("SRTWindow BFT")
		{
			SRTWindow	w1, w2;
			CHECK_FALSE(w1.isValid());
			CHECK(w1.isEmpty());
			CHECK(w1 == w1);	//	Same object
			CHECK(w1 == w2);	//	Identical
			w1.set(10, 5, 30, 20);
			CHECK(w1.isValid());
			CHECK_FALSE(w1.isEmpty());
			CHECK_EQ(w1.left(),10);
			CHECK_EQ(w1.top(),5);
			CHECK_EQ(w1.right(),30);
			CHECK_EQ(w1.bottom(),20);
			CHECK_EQ(w1.width(),20);
			CHECK_EQ(w1.height(),15);
			CHECK_NE(w1,w2);
			w2 = w1;
			CHECK_EQ(w1,w2);
			w2.setBottom(w2.bottom()-1);
			CHECK_NE(w1,w2);
		}	//	SRTWindow BFT

		SUBCASE("SRTCaptions Test 1")
		{
			const SRTCaptions::StringList tst1 =
			{
				"1",
				"00:00:03,400 --> 00:00:06,177",
				"In this lesson, we're going to",
				"be talking about finance. And",
				"",
				"2",
				"00:00:06,177 --> 00:00:10,009",
				"one of the most important aspects",
				"of finance is interest.",
				"",
				"3",
				"00:00:10,009 --> 00:00:13,655",
				"When I go to a bank or some",
				"other lending institution",
				"",
				"4",
				"00:00:13,655 --> 00:00:17,720",
				"to borrow money, the bank is happy",
				"to give me that money. But then I'm",
				"",
				"5",
				"00:00:17,900 --> 00:00:21,480",
				"going to be paying the bank for the",
				"privilege of using their money. And that",
				"",
				"6",
				"00:00:21,660 --> 00:00:26,440",
				"amount of money that I pay the bank is",
				"called interest. Likewise, if I put money",
				"",
				"7",
				"00:00:26,620 --> 00:00:31,220",
				"in a savings account or I purchase a",
				"certificate of deposit, the bank just",
				"",
				"8",
				"00:00:31,300 --> 00:00:35,800",
				"doesn't put my money in a little box",
				"and leave it there until later. They take",
				"",
				"9",
				"00:00:35,800 --> 00:00:40,822",
				"my money and lend it to someone",
				"else. So they are using my money.",
				"",
				"10",
				"00:00:40,822 --> 00:00:44,400",
				"The bank has to pay me for the privilege",
				"of using my money.",
				"",
				"11",
				"00:00:44,400 --> 00:00:48,700",
				"Now what makes banks",
				"profitable is the rate",
				"",
				"12",
				"00:00:48,700 --> 00:00:53,330",
				"that they charge people to use the bank's",
				"money is higher than the rate that they",
				"",
				"13",
				"00:00:53,510 --> 00:01:00,720",
				"pay people like me to use my money. The",
				"amount of interest that a person pays or",
				"",
				"14",
				"00:01:00,800 --> 00:01:06,640",
				"earns is dependent on three things. It's",
				"dependent on how much money is involved.",
				"",
				"15",
				"00:01:06,820 --> 00:01:11,300",
				"It's dependent upon the rate of interest",
				"being paid or the rate of interest being",
				"",
				"16",
				"00:01:11,480 --> 00:01:17,898",
				"charged. And it's also dependent upon",
				"how much time is involved. If I have",
				"",
				"17",
				"00:01:17,898 --> 00:01:22,730",
				"a loan and I want to decrease the amount",
				"of interest that I'm going to pay, then",
				"",
				"18",
				"00:01:22,800 --> 00:01:28,040",
				"I'm either going to have to decrease how",
				"much money I borrow, I'm going to have",
				"",
				"19",
				"00:01:28,220 --> 00:01:32,420",
				"to borrow the money over a shorter period",
				"of time, or I'm going to have to find a",
				"",
				"20",
				"00:01:32,600 --> 00:01:37,279",
				"lending institution that charges a lower",
				"interest rate. On the other hand, if I",
				"",
				"21",
				"00:01:37,279 --> 00:01:41,480",
				"want to earn more interest on my",
				"investment, I'm going to have to invest",
				"",
				"22",
				"00:01:41,480 --> 00:01:46,860",
				"more money, leave the money in the",
				"account for a longer period of time, or",
				"",
				"23",
				"00:01:46,860 --> 00:01:49,970",
				"find an institution that will pay",
				"me a higher interest rate."
			};
			SRTCaptions srtReader;
			CHECK(srtReader.reloadFrom(tst1));
			cout << "INFO 1:" << endl;  srtReader.printInfo(cout);
			CHECK_FALSE(srtReader.hasErrors());
			CHECK_FALSE(srtReader.hasWarnings());
	//		srtReader.printErrors(cerr);
			SRTTimestamp start(0), cmpStart(0);
			SRTCaptionInfo info;
			CHECK(srtReader.hasCaptionAtSeqNum(15));
			cmpStart = srtReader.startTimeAtSeqNum(15);
			CHECK_NE(cmpStart, 0);
			CHECK_FALSE(srtReader.hasCaptionAtSeqNum(0));
			CHECK_FALSE(srtReader.hasCaptionAtSeqNum(24));
			CHECK(srtReader.getTimestamp("00:01:06,820", start));
			CHECK(srtReader.hasCaptionAtStartTime(start));
			CHECK_FALSE(srtReader.hasCaptionAtStartTime(start+1));
			CHECK_EQ(start, cmpStart);
			const string cmp("It's dependent upon the rate of interest being paid or the rate of interest being");
			string caption (srtReader.captionAtStartTime(start));
			CHECK(srtReader.captionAtStartTime(start, info));
			cout << endl << "INFO ABOUT CAPTION AT " << SRTCaptions::timestampStr(start, true) << ":" << endl;
			info.printInfo(cout);
			CHECK_EQ(caption, cmp);
			cout << endl << "SRT:" << endl;
			srtReader.printSRT(cout);
		}	//	SUBCASE("SRTCaptions Test 1")

		SUBCASE("SRTCaptions Test 2")
		{
			SRTCaptions::StringList tst2 =
			{	/* 1*/	"",
				/* 2*/	"2", " ", "00:00:03,400 -> 00:00:03,899",	"In this lesson, we're going to be talking about finance. And",	"",
				/* 7*/	"3",	"00:00:06,177 --> 00:00:10,009",	"one of the most important aspects of finance is interest.",	"",
				/*11*/	"4",	"00:00:10,000 --> 00:00:13,655",	"When I go to a bank or some other lending institution",	"",
				/*15*/	"5",	"00:00:13,655 --> 00:00:17,720",	"to borrow money, the bank is happy to give me that money. But then I'm",	"",
				/*19*/	"6",	"00:00:17,900 --> 00:00:17,800",	"going to be paying the bank for the privilege of using their money. And that",	"",
				/*23*/	"7",	"00:00:21,660 --> 00:00:26,440",	"amount of money that I pay the bank is called interest. Likewise, if I put money",	"",
				/*27*/	"8",	"00:00:26,620 --> 00:00:31,220",	"in a savings account or I purchase a certificate of deposit, the bank just",	"",
				/*31*/	"9",	"00:00:31,300 --> 00:00:35,800",	"doesn't put my money in a little box and leave it there until later. They take",	"",
				/*35*/	"10",	"00:00:35,800 --> 00:00:40,822",	"my money and lend it to someone else. So they are using my money.",	"",
				/*39*/	"11",	"00:00:40,822 --> 00:00:44,400",	"The bank has to pay me for the privilege of using my money.",	"",
				/*43*/	"12",	"00:00:44,400 --> 00:00:48,700",	"Now what makes banks profitable is the rate",	"",
				/*47*/	"13",	"00:00:48,700 --> 00:00:53,330",	"that they charge people to use the bank's money is higher than the rate that they",	"",
				/*51*/	"14",	"00:00:53,510 --> 00:01:00,720",	"pay people like me to use my money. The amount of interest that a person pays or",	"",
				/*55*/	"15",	"00:01:00,800 --> 00:01:06,640",	"earns is dependent on three things. It's dependent on how much money is involved.",	"",
				/*59*/	"16",	"00:01:06,820 --> 00:01:11,300",	"It's dependent upon the rate of interest being paid or the rate of interest being",	"",
				/*63*/	"17",	"00:01:11,480 --> 00:01:17,898",	"charged. And it's also dependent upon how much time is involved. If I have",	"",
				/*67*/	"18",	"00:01:17,898 --> 00:01:22,730",	"a loan and I want to decrease the amount of interest that I'm going to pay, then",	"",
				/*71*/	"19",	"00:01:22,800 --> 00:01:28,040",	"I'm either going to have to decrease how much money I borrow, I'm going to have",	"",
				/*75*/	"20",	"00:01:28,220 --> 00:01:32,420",	"to borrow the money over a shorter period of time, or I'm going to have to find a",	"",
				/*79*/	"21",	"00:01:32,600 --> 00:01:37,279",	"lending institution that charges a lower interest rate. On the other hand, if I",	"",
				/*83*/	"22",	"00:01:37,279 --> 00:01:41,480",	"want to earn more interest on my investment, I'm going to have to invest",	"",
				/*87*/	"23",	"00:01:41,480 --> 00:01:46,860",	"more money, leave the money in the account for a longer period of time, or",	"",
				/*91*/	"24",	"00:01:46,860 --> 00:01:49,970",	"find an institution that will pay me a higher interest rate."
			};
			cout << "TEST 2:" << endl;
			SRTCaptions srtr;
			CHECK_FALSE(srtr.reloadFrom(tst2));
			cout << "INFO 2:" << endl;  srtr.printInfo(cout);
			CHECK_EQ(srtr.warningCount(),4);
			CHECK(srtr.hasErrors());
			CHECK(srtr.hasWarnings());
			srtr.printErrors(cerr);
			cout << endl << "SRT 2:" << endl;
			srtr.printSRT(cout);
			size_t lineNum(0);  string msg;
			CHECK(srtr.getWarning(0, lineNum, msg));
			CHECK(msg.find("got blank line") != string::npos);
			CHECK_EQ(lineNum, 0);
			CHECK(srtr.getWarning(1, lineNum, msg));
			CHECK(msg.find("Bad sequence number, expected '1'") != string::npos);
			CHECK_EQ(lineNum, 1);
			CHECK(srtr.getWarning(2, lineNum, msg));
			CHECK(msg.find("Expected timecode spec, instead got blank line") != string::npos);
			CHECK_EQ(lineNum, 2);
			CHECK(srtr.getWarning(3, lineNum, msg));
			CHECK(msg.find("instead got '->'") != string::npos);
			CHECK(msg.find("less than 500 msec") != string::npos);
			CHECK_EQ(lineNum, 3);
			CHECK(srtr.getError(lineNum, msg));
			CHECK(msg.find("happens after end time") != string::npos);
			CHECK_EQ(lineNum, 19);
			//	Fix the error and warnings, then reload:
			tst2.at(1) = "1";
			tst2.at(3) = "00:00:03,400 --> 00:00:04,959";
			tst2.at(19) = "00:00:17,800 --> 00:00:18,900";
			for (size_t ndx(2);  ndx < 24;  ndx++)
			{	ostringstream oss;  oss << DEC(ndx);
				tst2.at(4*(ndx-2)+6) = oss.str();
			}
			SRTCaptions::StringList::iterator it(tst2.begin());  ++it; ++it;  tst2.erase(it);  it = tst2.begin();  tst2.erase(it);
			cout << endl << endl << "POST-FIX INFO 2:" << endl
				<< aja::join(tst2,"\n") << endl;
			srtr.reloadFrom(tst2);
			srtr.printInfo(cout);
			srtr.printErrors(cerr);
		}	//	SUBCASE("SRTCaptions Test 2")

		SUBCASE("SRTCaptions Test 3")
		{
			SRTCaptions::StringList tst3 = 
			{	"1", "00:10:10,796 --> 00:10:33,800 X1:117 X2:619 Y1:042 Y2:428",
					"Windowed <b>Bold</b> text,",
					"along with <i>Italic</i>,",
					"and <b>Bold <i>OVERLAP</b> Italic</i>,",
					"plus <b><i>Bold & Italic</i></b>,",
					"and <i><b>Italic & Bold</b></i>.", "",
				"2", "00:10:10,796 --> 00:10:33,800",
					"{\\an4}Middle-Left",
					"anchor test with <b>bold</b> and <i>italic</i>", ""
			};
			SRTCaptions srt3;
			CHECK_FALSE(srt3.reloadFrom(tst3));
			cout << endl << "INFO 3a:" << endl;  srt3.printInfo(cout);  srt3.printSRT(cout);
			CHECK(srt3.hasErrors());
			size_t lineOff(0);  string msg;  SRTCaptionInfo info;
			CHECK(srt3.hasErrors());		srt3.printErrors(cerr);
			CHECK(srt3.getError(lineOff, msg));
			CHECK(msg.find("duplicated from caption") != string::npos);

			tst3.at(9) = "00:20:10,796 --> 00:20:33,800";	//	Fix duplicate caption error
			CHECK(srt3.reloadFrom(tst3));	//	Reparse
			cout << endl << "INFO 3b:" << endl;  srt3.printInfo(cout);  srt3.printSRT(cout);
			CHECK_EQ(srt3.captionCount(), 2);
			CHECK(srt3.hasCaptionAtSeqNum(2));
			CHECK(srt3.captionAtStartTime(srt3.startTimeAtSeqNum(2), info));
			cout << "INFO 3b CAPTION 2:" << endl;  info.printInfo(cout);
			CHECK(info.hasAnchor());
			CHECK_EQ(info.anchor(), 4);
			CHECK_FALSE(info.hasWindow());
			CHECK(info.hasLineNumbers());
			CHECK_EQ(info.startLineNumber(),9);
			CHECK_EQ(info.endLineNumber(),12);
			CHECK_EQ(info.caption(), "Middle-Left anchor test with bold and italic");

			tst3.push_back("3");
			tst3.push_back("00:11:11,700 --> 00:11:20,700 X1:117 X2:619 Y1:042 Y2:428");
			tst3.push_back("{\\an7}Upper-Left text");
			CHECK_FALSE(srt3.reloadFrom(tst3));
			cout << endl << "INFO 3c:" << endl;  srt3.printInfo(cout);  srt3.printSRT(cout);
			CHECK(srt3.hasErrors());
			CHECK(srt3.getError(lineOff, msg));
			CHECK(msg.find("conflicts with window spec") != string::npos);

			tst3.at(lineOff-1) = "00:11:11,700 --> 00:11:20,700";
			tst3.at(lineOff) = "Upper-left {\\an7} text";
			CHECK_FALSE(srt3.reloadFrom(tst3));
			cout << endl << "INFO 3d:" << endl;  srt3.printInfo(cout);  srt3.printSRT(cout);
			CHECK(srt3.hasErrors());		srt3.printErrors(cerr);
			CHECK(srt3.getError(lineOff, msg));
			CHECK(msg.find("expected at start of line") != string::npos);
			CHECK(msg.find("Anchor tag '{\\an7}' at offset 11") != string::npos);
			CHECK_EQ(lineOff, 15);

			tst3.at(lineOff) = "{\\an7}Upper-left text";
			tst3.push_back("{\\an6}Middle-right text");
			CHECK_FALSE(srt3.reloadFrom(tst3));
			cout << endl << "INFO 3e:" << endl << aja::join(tst3, "\n") << endl;  srt3.printInfo(cout);  srt3.printErrors(cerr);
			CHECK(srt3.hasErrors());
			CHECK(srt3.getError(lineOff, msg));
			CHECK(msg.find("must appear at start of caption") != string::npos);
			CHECK_EQ(lineOff, 16);
		}	//	SUBCASE("SRTCaptions Test 3")
	}	//	SubRipText
}	//	TEST_SUITE("SRT")
