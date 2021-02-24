/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2democommon.cpp
	@brief		Common implementation code used by many of the demo applications.
	@copyright	(C) 2013-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#include "ntv2democommon.h"
#include "ntv2devicescanner.h"
#include "ntv2devicefeatures.h"
#include "ntv2debug.h"
#include "ntv2utils.h"
#include "ntv2bft.h"
#include "ajabase/common/common.h"
#include <algorithm>
#include <map>
#include <iomanip>
#if defined (AJAMac) || defined (AJALinux)
	#include <unistd.h>
	#include <termios.h>
#endif

using namespace std;

typedef NTV2TCIndexes							NTV2TCIndexSet;				///< @brief	An alias to NTV2TCIndexes.
typedef NTV2TCIndexesConstIter					NTV2TCIndexSetConstIter;	///< @brief	An alias to NTV2TCIndexesConstIter.

typedef	map <string, NTV2VideoFormat>			String2VideoFormatMap;
typedef	String2VideoFormatMap::const_iterator	String2VideoFormatMapConstIter;

typedef	map <string, NTV2FrameBufferFormat>		String2PixelFormatMap;
typedef	String2PixelFormatMap::const_iterator	String2PixelFormatMapConstIter;

typedef	map <string, NTV2AudioSystem>			String2AudioSystemMap;
typedef	String2AudioSystemMap::const_iterator	String2AudioSystemMapConstIter;

typedef	map <string, NTV2InputSource>			String2InputSourceMap;
typedef	String2InputSourceMap::const_iterator	String2InputSourceMapConstIter;

typedef	map <string, NTV2OutputDestination>		String2OutputDestMap;
typedef	String2OutputDestMap::const_iterator	String2OutputDestMapConstIter;

typedef	map <string, NTV2TCIndex>				String2TCIndexMap;
typedef	pair <string, NTV2TCIndex>				String2TCIndexPair;
typedef	String2TCIndexMap::const_iterator		String2TCIndexMapConstIter;

typedef	map <string, string>					String2TPNamesMap;
typedef	pair <string, string>					String2TPNamePair;
typedef	String2TPNamesMap::const_iterator		String2TPNamesMapConstIter;


static const string				gGlobalMutexName	("com.aja.ntv2.mutex.demo");
static NTV2VideoFormatSet		gAllFormats;
static NTV2VideoFormatSet		gNon4KFormats;
static NTV2VideoFormatSet		g4KFormats;
static NTV2VideoFormatSet		g8KFormats;
static NTV2FrameBufferFormatSet	gPixelFormats;
static NTV2FrameBufferFormatSet	gFBFsRGB;
static NTV2FrameBufferFormatSet	gFBFsPlanar;
static NTV2FrameBufferFormatSet	gFBFsRaw;
static NTV2FrameBufferFormatSet	gFBFsPacked;
static NTV2FrameBufferFormatSet	gFBFsAlpha;
static NTV2FrameBufferFormatSet	gFBFsProRes;
static NTV2InputSourceSet		gInputSources;
static NTV2InputSourceSet		gInputSourcesSDI;
static NTV2InputSourceSet		gInputSourcesHDMI;
static NTV2InputSourceSet		gInputSourcesAnalog;
static NTV2OutputDestinations	gOutputDestinations;
static String2VideoFormatMap	gString2VideoFormatMap;
static String2PixelFormatMap	gString2PixelFormatMap;
static String2AudioSystemMap	gString2AudioSystemMap;
static String2InputSourceMap	gString2InputSourceMap;
static String2OutputDestMap		gString2OutputDestMap;
static NTV2TCIndexSet			gTCIndexes;
static NTV2TCIndexSet			gTCIndexesSDI;
static NTV2TCIndexSet			gTCIndexesHDMI;
static NTV2TCIndexSet			gTCIndexesAnalog;
static NTV2TCIndexSet			gTCIndexesATCLTC;
static NTV2TCIndexSet			gTCIndexesVITC1;
static NTV2TCIndexSet			gTCIndexesVITC2;
static String2TCIndexMap		gString2TCIndexMap;
static String2TPNamesMap		gString2TPNamesMap;
static NTV2StringList			gTestPatternNames;


class DemoCommonInitializer
{
	public:
		DemoCommonInitializer ()
		{
			typedef	pair <string, NTV2VideoFormat>			String2VideoFormatPair;
			typedef	pair <string, NTV2FrameBufferFormat>	String2PixelFormatPair;
			typedef	pair <string, NTV2AudioSystem>			String2AudioSystemPair;
			typedef	pair <string, NTV2InputSource>			String2InputSourcePair;
			typedef	pair <string, NTV2OutputDestination>	String2OutputDestPair;

			NTV2_ASSERT (gNon4KFormats.empty ());
			for (NTV2VideoFormat legalFormat (NTV2_FORMAT_UNKNOWN);  legalFormat < NTV2_MAX_NUM_VIDEO_FORMATS;  legalFormat = NTV2VideoFormat (legalFormat + 1))
			{
				string	str;
				if (!NTV2_IS_VALID_VIDEO_FORMAT (legalFormat))
					continue;

				if (NTV2_IS_QUAD_QUAD_FORMAT(legalFormat))
					g8KFormats.insert (legalFormat);
				else if (NTV2_IS_4K_VIDEO_FORMAT (legalFormat))
					g4KFormats.insert (legalFormat);
				else
					gNon4KFormats.insert (legalFormat);
				gAllFormats.insert (legalFormat);

				if		(legalFormat == NTV2_FORMAT_525_5994)	str = "525i2997";
				else if	(legalFormat == NTV2_FORMAT_625_5000)	str = "625i25";
				else if	(legalFormat == NTV2_FORMAT_525_2398)	str = "525i2398";
				else if	(legalFormat == NTV2_FORMAT_525_2400)	str = "525i24";
				else
				{
					str = ::NTV2VideoFormatToString (legalFormat);
					if (str.at (str.length () - 1) == 'a')	//	If it ends in "a"...
						str.erase (str.length () - 1);		//	...lop off the "a"

					if (str.find (".00") != string::npos)	//	If it ends in ".00"...
						str.erase (str.find (".00"), 3);	//	...lop off the ".00" (but keep the "b", if any)

					while (str.find (" ") != string::npos)
						str.erase (str.find (" "), 1);		//	Remove all spaces

					if (str.find (".") != string::npos)
						str.erase (str.find ("."), 1);		//	Remove "."

					str = CNTV2DemoCommon::ToLower (str);	//	Fold to lower case
				}
				gString2VideoFormatMap.insert (String2VideoFormatPair (str, legalFormat));
				gString2VideoFormatMap.insert (String2VideoFormatPair (ULWordToString (legalFormat), legalFormat));
			}	//	for each video format supported in demo apps

			//	Add popular format names...
			gString2VideoFormatMap.insert (String2VideoFormatPair ("sd",			NTV2_FORMAT_525_5994));
			gString2VideoFormatMap.insert (String2VideoFormatPair ("525i",			NTV2_FORMAT_525_5994));
			gString2VideoFormatMap.insert (String2VideoFormatPair ("625i",			NTV2_FORMAT_625_5000));
			gString2VideoFormatMap.insert (String2VideoFormatPair ("720p",			NTV2_FORMAT_720p_5994));
			gString2VideoFormatMap.insert (String2VideoFormatPair ("hd",			NTV2_FORMAT_1080i_5994));
            gString2VideoFormatMap.insert (String2VideoFormatPair ("1080i",			NTV2_FORMAT_1080i_5994));
            gString2VideoFormatMap.insert (String2VideoFormatPair ("1080i50",	    NTV2_FORMAT_1080i_5000));
            gString2VideoFormatMap.insert (String2VideoFormatPair ("1080p",	        NTV2_FORMAT_1080p_5994_B));
            gString2VideoFormatMap.insert (String2VideoFormatPair ("1080p50",	    NTV2_FORMAT_1080p_5000_B));

            gString2VideoFormatMap.insert (String2VideoFormatPair ("uhd",			NTV2_FORMAT_4x1920x1080p_6000));
			gString2VideoFormatMap.insert (String2VideoFormatPair ("uhd2398",		NTV2_FORMAT_4x1920x1080p_2398));
			gString2VideoFormatMap.insert (String2VideoFormatPair ("uhd24",			NTV2_FORMAT_4x1920x1080p_2400));
			gString2VideoFormatMap.insert (String2VideoFormatPair ("uhd25",			NTV2_FORMAT_4x1920x1080p_2500));
			gString2VideoFormatMap.insert (String2VideoFormatPair ("uhd50",			NTV2_FORMAT_4x1920x1080p_5000));
			gString2VideoFormatMap.insert (String2VideoFormatPair ("uhd5994",		NTV2_FORMAT_4x1920x1080p_5994));
			gString2VideoFormatMap.insert (String2VideoFormatPair ("uhd60",			NTV2_FORMAT_4x1920x1080p_6000));

			gString2VideoFormatMap.insert (String2VideoFormatPair ("4k",			NTV2_FORMAT_4x2048x1080p_6000));
			gString2VideoFormatMap.insert (String2VideoFormatPair ("4k2398",		NTV2_FORMAT_4x2048x1080p_2398));
			gString2VideoFormatMap.insert (String2VideoFormatPair ("4k24",			NTV2_FORMAT_4x2048x1080p_2400));
			gString2VideoFormatMap.insert (String2VideoFormatPair ("4k25",			NTV2_FORMAT_4x2048x1080p_2500));
			gString2VideoFormatMap.insert (String2VideoFormatPair ("4k4795",		NTV2_FORMAT_4x2048x1080p_4795));
			gString2VideoFormatMap.insert (String2VideoFormatPair ("4k48",			NTV2_FORMAT_4x2048x1080p_4800));
			gString2VideoFormatMap.insert (String2VideoFormatPair ("4k50",			NTV2_FORMAT_4x2048x1080p_5000));
			gString2VideoFormatMap.insert (String2VideoFormatPair ("4k5994",		NTV2_FORMAT_4x2048x1080p_5994));
			gString2VideoFormatMap.insert (String2VideoFormatPair ("4k60",			NTV2_FORMAT_4x2048x1080p_6000));
			gString2VideoFormatMap.insert (String2VideoFormatPair ("4k11988",		NTV2_FORMAT_4x2048x1080p_11988));
			gString2VideoFormatMap.insert (String2VideoFormatPair ("4k120",			NTV2_FORMAT_4x2048x1080p_12000));

			gString2VideoFormatMap.insert (String2VideoFormatPair ("uhd2",			NTV2_FORMAT_4x3840x2160p_2398));
			gString2VideoFormatMap.insert (String2VideoFormatPair ("uhd22398",		NTV2_FORMAT_4x3840x2160p_2398));
			gString2VideoFormatMap.insert (String2VideoFormatPair ("uhd224",		NTV2_FORMAT_4x3840x2160p_2400));
			gString2VideoFormatMap.insert (String2VideoFormatPair ("uhd225",		NTV2_FORMAT_4x3840x2160p_2500));
			gString2VideoFormatMap.insert (String2VideoFormatPair ("uhd250",		NTV2_FORMAT_4x3840x2160p_5000));
			gString2VideoFormatMap.insert (String2VideoFormatPair ("uhd25994",		NTV2_FORMAT_4x3840x2160p_5994));
			gString2VideoFormatMap.insert (String2VideoFormatPair ("uhd260",		NTV2_FORMAT_4x3840x2160p_6000));

			gString2VideoFormatMap.insert (String2VideoFormatPair ("8k",			NTV2_FORMAT_4x4096x2160p_6000));
			gString2VideoFormatMap.insert (String2VideoFormatPair ("8k2398",		NTV2_FORMAT_4x4096x2160p_2398));
			gString2VideoFormatMap.insert (String2VideoFormatPair ("8k24",			NTV2_FORMAT_4x4096x2160p_2400));
			gString2VideoFormatMap.insert (String2VideoFormatPair ("8k25",			NTV2_FORMAT_4x4096x2160p_2500));
			gString2VideoFormatMap.insert (String2VideoFormatPair ("8k4795",		NTV2_FORMAT_4x4096x2160p_4795));
			gString2VideoFormatMap.insert (String2VideoFormatPair ("8k48",			NTV2_FORMAT_4x4096x2160p_4800));
			gString2VideoFormatMap.insert (String2VideoFormatPair ("8k50",			NTV2_FORMAT_4x4096x2160p_5000));
			gString2VideoFormatMap.insert (String2VideoFormatPair ("8k5994",		NTV2_FORMAT_4x4096x2160p_5994));
			gString2VideoFormatMap.insert (String2VideoFormatPair ("8k60",			NTV2_FORMAT_4x4096x2160p_6000));
			
			NTV2_ASSERT (gPixelFormats.empty ());
			for (NTV2FrameBufferFormat legalFormat (NTV2_FBF_10BIT_YCBCR);  legalFormat < NTV2_FBF_NUMFRAMEBUFFERFORMATS;  legalFormat = NTV2FrameBufferFormat (legalFormat + 1))
			{
				string	str;
				if (!NTV2_IS_VALID_FRAME_BUFFER_FORMAT (legalFormat))
					continue;

				gPixelFormats.insert (legalFormat);
				if (NTV2_IS_FBF_PLANAR (legalFormat))
					gFBFsPlanar.insert (legalFormat);
				if (NTV2_IS_FBF_RGB (legalFormat))
					gFBFsRGB.insert (legalFormat);
				if (NTV2_IS_FBF_PRORES (legalFormat))
					gFBFsProRes.insert (legalFormat);
				if (NTV2_FBF_HAS_ALPHA (legalFormat))
					gFBFsAlpha.insert (legalFormat);
				if (NTV2_FBF_IS_RAW (legalFormat))
					gFBFsRaw.insert (legalFormat);

				str = ::NTV2FrameBufferFormatToString (legalFormat, true);
				while (str.find (" ") != string::npos)
					str.erase (str.find (" "), 1);		//	Remove all spaces

				while (str.find ("-") != string::npos)
					str.erase (str.find ("-"), 1);		//	Remove all "-"

				if (str.find ("compatible") != string::npos)
					str.erase (str.find ("compatible"), 10);	//	Remove "compatible"

				if (str.find ("ittle") != string::npos)
					str.erase (str.find ("ittle"), 5);	//	Remove "ittle"

				if (str.find ("ndian") != string::npos)
					str.erase (str.find ("ndian"), 5);	//	Remove "ndian"

				str = CNTV2DemoCommon::ToLower (str);	//	Fold to lower case

				gString2PixelFormatMap.insert (String2PixelFormatPair (str, legalFormat));

				str = ::NTV2FrameBufferFormatToString (legalFormat, false);
				if (str.find ("NTV2_FBF_") == 0)		//	If it starts with "NTV2_FBF_"...
					str.erase (0, 9);					//	...lop it off

				while (str.find (" ") != string::npos)
					str.erase (str.find (" "), 1);		//	Remove all spaces

				while (str.find ("_") != string::npos)
					str.erase (str.find ("_"), 1);		//	Remove all "_"

				str = CNTV2DemoCommon::ToLower (str);	//	Fold to lower case

				gString2PixelFormatMap.insert (String2PixelFormatPair (str, legalFormat));
				gString2PixelFormatMap.insert (String2PixelFormatPair (::NTV2FrameBufferFormatToString (legalFormat, false), legalFormat));
				gString2PixelFormatMap.insert (String2PixelFormatPair (ULWordToString (legalFormat), legalFormat));
			}	//	for each pixel format supported in demo apps

			//	Add popular pixel format names...
			gString2PixelFormatMap.insert (String2PixelFormatPair ("v210",			NTV2_FBF_10BIT_YCBCR));
			gString2PixelFormatMap.insert (String2PixelFormatPair ("yuv10",			NTV2_FBF_10BIT_YCBCR));
			gString2PixelFormatMap.insert (String2PixelFormatPair ("2vuy",			NTV2_FBF_8BIT_YCBCR));
			gString2PixelFormatMap.insert (String2PixelFormatPair ("uyvy",			NTV2_FBF_8BIT_YCBCR));
			gString2PixelFormatMap.insert (String2PixelFormatPair ("argb",			NTV2_FBF_ARGB));
			gString2PixelFormatMap.insert (String2PixelFormatPair ("argb8",			NTV2_FBF_ARGB));
			gString2PixelFormatMap.insert (String2PixelFormatPair ("bgra",			NTV2_FBF_RGBA));
			gString2PixelFormatMap.insert (String2PixelFormatPair ("bgra8",			NTV2_FBF_RGBA));
			gString2PixelFormatMap.insert (String2PixelFormatPair ("rgba",			NTV2_FBF_RGBA));
			gString2PixelFormatMap.insert (String2PixelFormatPair ("rgba8",			NTV2_FBF_RGBA));
			gString2PixelFormatMap.insert (String2PixelFormatPair ("rgb10",			NTV2_FBF_10BIT_RGB));
			gString2PixelFormatMap.insert (String2PixelFormatPair ("yuy2",			NTV2_FBF_8BIT_YCBCR_YUY2));
			gString2PixelFormatMap.insert (String2PixelFormatPair ("abgr",			NTV2_FBF_ABGR));
			gString2PixelFormatMap.insert (String2PixelFormatPair ("abgr8",			NTV2_FBF_ABGR));
			gString2PixelFormatMap.insert (String2PixelFormatPair ("rgb10dpx",		NTV2_FBF_10BIT_DPX));
			gString2PixelFormatMap.insert (String2PixelFormatPair ("yuv10dpx",		NTV2_FBF_10BIT_YCBCR_DPX));
			gString2PixelFormatMap.insert (String2PixelFormatPair ("ycbcr10dpx",	NTV2_FBF_10BIT_YCBCR_DPX));
			gString2PixelFormatMap.insert (String2PixelFormatPair ("dvcpro8",		NTV2_FBF_8BIT_DVCPRO));
			gString2PixelFormatMap.insert (String2PixelFormatPair ("i420",			NTV2_FBF_8BIT_YCBCR_420PL3));
			gString2PixelFormatMap.insert (String2PixelFormatPair ("hdv",			NTV2_FBF_8BIT_HDV));
			gString2PixelFormatMap.insert (String2PixelFormatPair ("hdv8",			NTV2_FBF_8BIT_HDV));
			gString2PixelFormatMap.insert (String2PixelFormatPair ("rgb24",			NTV2_FBF_24BIT_RGB));
			gString2PixelFormatMap.insert (String2PixelFormatPair ("bgr24",			NTV2_FBF_24BIT_BGR));
			gString2PixelFormatMap.insert (String2PixelFormatPair ("ycbcra10",		NTV2_FBF_10BIT_YCBCRA));
            gString2PixelFormatMap.insert (String2PixelFormatPair ("rgb10dpxle",	NTV2_FBF_10BIT_DPX_LE));
			gString2PixelFormatMap.insert (String2PixelFormatPair ("proresdvcpro",	NTV2_FBF_PRORES_DVCPRO));
			gString2PixelFormatMap.insert (String2PixelFormatPair ("proreshdv",		NTV2_FBF_PRORES_HDV));
			gString2PixelFormatMap.insert (String2PixelFormatPair ("rgb10packed",	NTV2_FBF_10BIT_RGB_PACKED));
			gString2PixelFormatMap.insert (String2PixelFormatPair ("argb10",		NTV2_FBF_10BIT_ARGB));
			gString2PixelFormatMap.insert (String2PixelFormatPair ("argb16",		NTV2_FBF_16BIT_ARGB));
			gString2PixelFormatMap.insert (String2PixelFormatPair ("rgb10raw",		NTV2_FBF_10BIT_RAW_RGB));
			gString2PixelFormatMap.insert (String2PixelFormatPair ("ycbcr10raw",	NTV2_FBF_10BIT_RAW_YCBCR));
			gString2PixelFormatMap.insert (String2PixelFormatPair ("yuv10raw",		NTV2_FBF_10BIT_RAW_YCBCR));

			//	Audio systems...
			for (uint8_t ndx (0);  ndx < 8;  ndx++)
				gString2AudioSystemMap.insert (String2AudioSystemPair (ULWordToString (ndx + 1), NTV2AudioSystem (ndx)));

			//	Input Sources...
			for (NTV2InputSource inputSource(NTV2_INPUTSOURCE_ANALOG1);  inputSource < NTV2_NUM_INPUTSOURCES;  inputSource = NTV2InputSource(inputSource+1))
			{
				gInputSources.insert(inputSource);
				if (NTV2_INPUT_SOURCE_IS_SDI(inputSource))
				{
					gInputSourcesSDI.insert(inputSource);
					gString2InputSourceMap.insert(String2InputSourcePair(ULWordToString(inputSource - NTV2_INPUTSOURCE_SDI1 + 1), inputSource));
				}
				else if (NTV2_INPUT_SOURCE_IS_HDMI(inputSource))
					gInputSourcesHDMI.insert(inputSource);
				else if (NTV2_INPUT_SOURCE_IS_ANALOG(inputSource))
					gInputSourcesAnalog.insert(inputSource);
				else
					continue;
				gString2InputSourceMap.insert(String2InputSourcePair(::NTV2InputSourceToString(inputSource, false), inputSource));
				gString2InputSourceMap.insert(String2InputSourcePair(::NTV2InputSourceToString(inputSource, true), inputSource));
				gString2InputSourceMap.insert(String2InputSourcePair(CNTV2DemoCommon::ToLower(::NTV2InputSourceToString (inputSource, true)), inputSource));
			}	//	for each input source
			gString2InputSourceMap.insert(String2InputSourcePair(string("hdmi"),NTV2_INPUTSOURCE_HDMI1));

			//	Output Destinations...
			for (NTV2OutputDestination outputDest(NTV2_OUTPUTDESTINATION_ANALOG);  outputDest < NTV2_OUTPUTDESTINATION_INVALID;  outputDest = NTV2OutputDestination(outputDest+1))
			{
				gOutputDestinations.insert(outputDest);
				gString2OutputDestMap.insert(String2OutputDestPair(::NTV2OutputDestinationToString(outputDest,false), outputDest));
				gString2OutputDestMap.insert(String2OutputDestPair(::NTV2OutputDestinationToString(outputDest,true), outputDest));
				gString2OutputDestMap.insert(String2OutputDestPair(CNTV2DemoCommon::ToLower(::NTV2OutputDestinationToString(outputDest, true)), outputDest));
				if (NTV2_OUTPUT_DEST_IS_SDI(outputDest))
				{	ostringstream oss;  oss << DEC(UWord(::NTV2OutputDestinationToChannel(outputDest)+1));
					gString2OutputDestMap.insert(String2OutputDestPair(oss.str(), outputDest));
				}
			}	//	for each output dest
			gString2OutputDestMap.insert(String2OutputDestPair(string("hdmi1"),NTV2_OUTPUTDESTINATION_HDMI));

			//	TCIndexes...
			for (uint16_t ndx (0);  ndx < NTV2_MAX_NUM_TIMECODE_INDEXES;  ndx++)
			{
				const NTV2TCIndex	tcIndex	(static_cast<NTV2TCIndex>(ndx));
				gTCIndexes.insert (tcIndex);
				gString2TCIndexMap.insert (String2TCIndexPair(ULWordToString(ndx), tcIndex));
				gString2TCIndexMap.insert (String2TCIndexPair(::NTV2TCIndexToString(tcIndex, false), tcIndex));
				gString2TCIndexMap.insert (String2TCIndexPair(CNTV2DemoCommon::ToLower(::NTV2TCIndexToString(tcIndex, true)), tcIndex));
				if (NTV2_IS_ANALOG_TIMECODE_INDEX(tcIndex))
					gTCIndexesAnalog.insert (tcIndex);
				else
					gTCIndexesSDI.insert (tcIndex);
				if (NTV2_IS_ATC_LTC_TIMECODE_INDEX(tcIndex))
					gTCIndexesATCLTC.insert (tcIndex);
				if (NTV2_IS_ATC_VITC1_TIMECODE_INDEX(tcIndex))
					gTCIndexesVITC1.insert (tcIndex);
				if (NTV2_IS_ATC_VITC2_TIMECODE_INDEX(tcIndex))
					gTCIndexesVITC2.insert (tcIndex);
			}

			{	//	Test Patterns...
				const NTV2StringList & tpNames(NTV2TestPatternGen::getTestPatternNames());
				const NTV2StringList colorNames(NTV2TestPatternGen::getColorNames());
				for (NTV2TestPatternSelect tp(NTV2_TestPatt_ColorBars100);  tp < NTV2_TestPatt_All;  tp = NTV2TestPatternSelect(tp+1))
				{
					string tpName(tpNames.at(tp));
					aja::replace(aja::replace(aja::replace(tpName, " ", ""), "%", ""), "_", "");
					gString2TPNamesMap.insert(String2TPNamePair(aja::lower(tpName), tpNames.at(tp)));
					ostringstream oss; oss << DEC(tp);
					gString2TPNamesMap.insert(String2TPNamePair(oss.str(), tpNames.at(tp)));
				}
				for (NTV2StringListConstIter it(colorNames.begin());  it != colorNames.end();  ++it)
				{
					string colorName(*it);
					aja::replace(aja::replace(aja::replace(colorName, " ", ""), "%", ""), "_", "");
					gString2TPNamesMap.insert(String2TPNamePair(aja::lower(colorName), *it));
				}
			}
		}	//	constructor
	private:
		string ULWordToString (const ULWord inNum)
		{
			ostringstream	oss;
			oss << inNum;
			return oss.str ();
		}
};	//	constructor

static const DemoCommonInitializer	gInitializer;


bool NTV2FrameData::LockAll (CNTV2Card & inDevice)
{
	size_t errorCount(0);
	if (fVideoBuffer)
		if (!inDevice.DMABufferLock(fVideoBuffer, true))
			errorCount++;
	if (fVideoBuffer2)
		if (!inDevice.DMABufferLock(fVideoBuffer2, true))
			errorCount++;
	if (fAudioBuffer)
		if (!inDevice.DMABufferLock(fAudioBuffer, true))
			errorCount++;
	if (fAncBuffer)
		if (!inDevice.DMABufferLock(fAncBuffer, true))
			errorCount++;
	if (fAncBuffer2)
		if (!inDevice.DMABufferLock(fAncBuffer2, true))
			errorCount++;
	return !errorCount;
}

bool NTV2FrameData::UnlockAll (CNTV2Card & inDevice)
{
	size_t errorCount(0);
	if (fVideoBuffer)
		if (!inDevice.DMABufferUnlock(fVideoBuffer))
			errorCount++;
	if (fVideoBuffer2)
		if (!inDevice.DMABufferUnlock(fVideoBuffer2))
			errorCount++;
	if (fAudioBuffer)
		if (!inDevice.DMABufferUnlock(fAudioBuffer))
			errorCount++;
	if (fAncBuffer)
		if (!inDevice.DMABufferUnlock(fAncBuffer))
			errorCount++;
	if (fAncBuffer2)
		if (!inDevice.DMABufferUnlock(fAncBuffer2))
			errorCount++;
	return !errorCount;
}


bool CNTV2DemoCommon::IsValidDevice (const string & inDeviceSpec)
{
	CNTV2Card	device;
	const string	deviceSpec	(inDeviceSpec.empty() ? "0" : inDeviceSpec);
	return CNTV2DeviceScanner::GetFirstDeviceFromArgument (deviceSpec, device);
}


static string DeviceFilterString (const NTV2DeviceKinds inKinds)
{
	if (inKinds == NTV2_DEVICEKIND_ALL)
		return "any device";
	else if (inKinds == NTV2_DEVICEKIND_NONE)
		return "no device";

	vector<string>	strs;
	if (inKinds & NTV2_DEVICEKIND_INPUT)
		strs.push_back("capture");
	if (inKinds & NTV2_DEVICEKIND_OUTPUT)
		strs.push_back("playout");
	if (inKinds & NTV2_DEVICEKIND_SDI)
		strs.push_back("SDI");
	if (inKinds & NTV2_DEVICEKIND_HDMI)
		strs.push_back("HDMI");
	if (inKinds & NTV2_DEVICEKIND_ANALOG)
		strs.push_back("analog video");
	if (inKinds & NTV2_DEVICEKIND_SFP)
		strs.push_back("IP/SFPs");
	if (inKinds & NTV2_DEVICEKIND_EXTERNAL)
		strs.push_back("Thunderbolt/PCMCIA");
	if (inKinds & NTV2_DEVICEKIND_4K)
		strs.push_back("4K");
	if (inKinds & NTV2_DEVICEKIND_12G)
		strs.push_back("12G SDI");
	if (inKinds & NTV2_DEVICEKIND_6G)
		strs.push_back("6G SDI");
	if (inKinds & NTV2_DEVICEKIND_CUSTOM_ANC)
		strs.push_back("custom Anc");
	if (inKinds & NTV2_DEVICEKIND_RELAYS)
		strs.push_back("SDI relays");
	if (strs.empty())
		return "??";

	ostringstream	oss;
	for (vector<string>::const_iterator it(strs.begin());  it != strs.end();  )
	{
		oss << *it;
		if (++it != strs.end())
			oss << " | ";
	}
	return oss.str();
}


string CNTV2DemoCommon::GetDeviceStrings (const NTV2DeviceKinds inKinds)
{
	ostringstream			oss, hdr;
	CNTV2Card				device;
	ULWord					ndx(0);
	const NTV2DeviceIDSet	supportedDevices(::NTV2GetSupportedDevices(inKinds));
	const string			filterString(DeviceFilterString(inKinds));

	typedef map<NTV2DeviceID,UWord>	DeviceTallies;
	DeviceTallies	tallies;

	for (ndx = 0;  CNTV2DeviceScanner::GetDeviceAtIndex(ndx, device);  ndx++)
	{
		const NTV2DeviceID	deviceID(device.GetDeviceID());
		string	serialNum;
		oss	<< endl
			<< DEC(ndx);
		if (tallies.find(deviceID) == tallies.end())
		{
			tallies[deviceID] = 1;
			oss << endl
				<< ToLower(::NTV2DeviceIDToString(deviceID));
		}
		else
		{
			const UWord num(tallies[deviceID]);
			tallies.erase(deviceID);
			tallies[deviceID] = num+1;
		}
		if (device.GetSerialNumberString(serialNum))
			oss << endl
				<< serialNum;
		if (inKinds != NTV2_DEVICEKIND_ALL  &&  inKinds != NTV2_DEVICEKIND_NONE)
			if (supportedDevices.find(deviceID) == supportedDevices.end())
				oss << "\t## Doesn't support one of " << filterString;
		oss << endl;
	}

	if (!ndx)
		return string("No devices\n");
	hdr << DEC(ndx) << (ndx == 1 ? " device" : " devices") << " found:" << endl
		<< setw(16) << left << "Legal -d Values" << endl
		<< setw(16) << left << "----------------";
	hdr << oss.str();
	return hdr.str();
}


const NTV2VideoFormatSet &	CNTV2DemoCommon::GetSupportedVideoFormats (const NTV2VideoFormatKinds inKinds)
{
	switch(inKinds)
	{
		case VIDEO_FORMATS_ALL:			return gAllFormats;
		case VIDEO_FORMATS_4KUHD:		return g4KFormats;
		case VIDEO_FORMATS_8KUHD2:		return g8KFormats;
		default:						return gNon4KFormats;
	}
}


string CNTV2DemoCommon::GetVideoFormatStrings (const NTV2VideoFormatKinds inKinds, const string inDeviceSpecifier)
{
	const NTV2VideoFormatSet &	formatSet	(GetSupportedVideoFormats(inKinds));
	ostringstream				oss;
	CNTV2Card					theDevice;
	if (!inDeviceSpecifier.empty())
		CNTV2DeviceScanner::GetFirstDeviceFromArgument (inDeviceSpecifier, theDevice);

	oss	<< setw(25) << left << "Video Format"				<< "\t" << setw(16) << left << "Legal -v Values" << endl
		<< setw(25) << left << "------------------------"	<< "\t" << setw(16) << left << "----------------" << endl;
	for (NTV2VideoFormatSetConstIter iter(formatSet.begin());  iter != formatSet.end();  ++iter)
	{
		string	formatName	(::NTV2VideoFormatToString (*iter));
		for (String2VideoFormatMapConstIter it(gString2VideoFormatMap.begin());  it != gString2VideoFormatMap.end();  ++it)
			if (*iter == it->second)
			{
				oss << setw(25) << left << formatName << "\t" << setw(16) << left << it->first;
				if (!inDeviceSpecifier.empty()  &&  theDevice.IsOpen()  &&  !::NTV2DeviceCanDoVideoFormat(theDevice.GetDeviceID(), *iter))
					oss << "\t## Incompatible with " << theDevice.GetDisplayName();
				oss << endl;
				formatName.clear();
			}
		oss << endl;
	}
	return oss.str();
}


NTV2FrameBufferFormatSet CNTV2DemoCommon::GetSupportedPixelFormats (const NTV2PixelFormatKinds inKinds)
{
	if (inKinds == PIXEL_FORMATS_ALL)
		return gPixelFormats;

	NTV2FrameBufferFormatSet	result;

	if (inKinds & PIXEL_FORMATS_RGB)
		result += gFBFsRGB;
	if (inKinds & PIXEL_FORMATS_PLANAR)
		result += gFBFsPlanar;
	if (inKinds & PIXEL_FORMATS_RAW)
		result += gFBFsRaw;
	if (inKinds & PIXEL_FORMATS_PACKED)
		result += gFBFsPacked;
	if (inKinds & PIXEL_FORMATS_ALPHA)
		result += gFBFsAlpha;

	return result;
}


string CNTV2DemoCommon::GetPixelFormatStrings (const NTV2PixelFormatKinds inKinds, const string inDeviceSpecifier)
{
	const NTV2FrameBufferFormatSet &	formatSet	(GetSupportedPixelFormats (inKinds));
	NTV2DeviceID						deviceID	(DEVICE_ID_NOTFOUND);
	string								displayName;
	ostringstream						oss;

	if (!inDeviceSpecifier.empty ())
	{
		CNTV2Card	device;
		CNTV2DeviceScanner::GetFirstDeviceFromArgument (inDeviceSpecifier, device);
		if (device.IsOpen ())
		{
			deviceID = device.GetDeviceID ();
			displayName = device.GetDisplayName ();
		}
	}


	oss << setw (34) << left << "Frame Buffer Format"					<< "\t" << setw (32) << left << "Legal -p Values" << endl
		<< setw (34) << left << "----------------------------------"	<< "\t" << setw (32) << left << "--------------------------------" << endl;
	for (NTV2FrameBufferFormatSetConstIter iter (formatSet.begin ());  iter != formatSet.end ();  ++iter)
	{
		string	formatName	(::NTV2FrameBufferFormatToString (*iter, true));
		for (String2PixelFormatMapConstIter it (gString2PixelFormatMap.begin ());  it != gString2PixelFormatMap.end ();  ++it)
			if (*iter == it->second)
			{
				oss << setw (35) << left << formatName << "\t" << setw (25) << left << it->first;
				if (!displayName.empty ()  &&  !::NTV2DeviceCanDoFrameBufferFormat (deviceID, *iter))
					oss << "\t## Incompatible with " << displayName;
				oss << endl;
				formatName.clear ();
			}
		oss << endl;
	}
	return oss.str ();
}


NTV2VideoFormat CNTV2DemoCommon::GetVideoFormatFromString (const string & inStr, const NTV2VideoFormatKinds inKinds)
{
	String2VideoFormatMapConstIter	iter	(gString2VideoFormatMap.find(inStr));
	if (iter == gString2VideoFormatMap.end())
		return NTV2_FORMAT_UNKNOWN;
	const NTV2VideoFormat	format	(iter->second);
	if (inKinds == VIDEO_FORMATS_ALL)
		return format;
	if (inKinds == VIDEO_FORMATS_4KUHD && NTV2_IS_4K_VIDEO_FORMAT(format))
		return format;
	if (inKinds == VIDEO_FORMATS_8KUHD2 && NTV2_IS_QUAD_QUAD_FORMAT(format))
		return format;
	if (inKinds == VIDEO_FORMATS_NON_4KUHD && !NTV2_IS_4K_VIDEO_FORMAT(format))
		return format;
	return NTV2_FORMAT_UNKNOWN;
}


NTV2FrameBufferFormat CNTV2DemoCommon::GetPixelFormatFromString (const string & inStr)
{
	String2PixelFormatMapConstIter	iter	(gString2PixelFormatMap.find (inStr));
	return  iter != gString2PixelFormatMap.end ()  ?  iter->second  :  NTV2_FBF_INVALID;
}


const NTV2InputSourceSet CNTV2DemoCommon::GetSupportedInputSources (const NTV2InputSourceKinds inKinds)
{
	if (inKinds == NTV2_INPUTSOURCES_ALL)
		return gInputSources;

	NTV2InputSourceSet	result;

	if (inKinds & NTV2_INPUTSOURCES_SDI)
		result += gInputSourcesSDI;
	if (inKinds & NTV2_INPUTSOURCES_HDMI)
		result += gInputSourcesHDMI;
	if (inKinds & NTV2_INPUTSOURCES_ANALOG)
		result += gInputSourcesAnalog;

	return result;
}


string CNTV2DemoCommon::GetInputSourceStrings (const NTV2InputSourceKinds inKinds,  const string inDeviceSpecifier)
{
	const NTV2InputSourceSet &	sourceSet	(GetSupportedInputSources (inKinds));
	ostringstream				oss;
	CNTV2Card					theDevice;
	if (!inDeviceSpecifier.empty ())
		CNTV2DeviceScanner::GetFirstDeviceFromArgument (inDeviceSpecifier, theDevice);

	oss	<< setw (25) << left << "Input Source"				<< "\t" << setw (16) << left << "Legal -i Values" << endl
		<< setw (25) << left << "------------------------"	<< "\t" << setw (16) << left << "----------------" << endl;
	for (NTV2InputSourceSetConstIter iter (sourceSet.begin ());  iter != sourceSet.end ();  ++iter)
	{
		string	sourceName	(::NTV2InputSourceToString (*iter));
		for (String2InputSourceMapConstIter it (gString2InputSourceMap.begin ());  it != gString2InputSourceMap.end ();  ++it)
			if (*iter == it->second)
			{
				oss << setw (25) << left << sourceName << "\t" << setw (16) << left << it->first;
				if (!inDeviceSpecifier.empty ()  &&  theDevice.IsOpen ()  &&  !::NTV2DeviceCanDoInputSource (theDevice.GetDeviceID (), *iter))
					oss << "\t## Incompatible with " << theDevice.GetDisplayName ();
				oss << endl;
				sourceName.clear ();
			}
		oss << endl;
	}
	return oss.str ();
}


NTV2InputSource CNTV2DemoCommon::GetInputSourceFromString (const string & inStr)
{
	String2InputSourceMapConstIter	iter	(gString2InputSourceMap.find (inStr));
	if (iter == gString2InputSourceMap.end ())
		return NTV2_INPUTSOURCE_INVALID;
	return iter->second;
}


string CNTV2DemoCommon::GetOutputDestinationStrings (const string inDeviceSpecifier)
{
	const NTV2OutputDestinations &	dests (gOutputDestinations);
	ostringstream					oss;
	CNTV2Card						theDevice;
	if (!inDeviceSpecifier.empty())
		CNTV2DeviceScanner::GetFirstDeviceFromArgument(inDeviceSpecifier, theDevice);

	oss	<< setw (25) << left << "Output Destination"		<< "\t" << setw(16) << left << "Legal -o Values" << endl
		<< setw (25) << left << "------------------------"	<< "\t" << setw(16) << left << "----------------" << endl;
	for (NTV2OutputDestinationsConstIter iter(dests.begin());  iter != dests.end();  ++iter)
	{
		string	destName(::NTV2OutputDestinationToString(*iter));
		for (String2OutputDestMapConstIter it(gString2OutputDestMap.begin ());  it != gString2OutputDestMap.end ();  ++it)
			if (*iter == it->second)
			{
				oss << setw(25) << left << destName << "\t" << setw(16) << left << it->first;
				if (!inDeviceSpecifier.empty()  &&  theDevice.IsOpen()  &&  !::NTV2DeviceCanDoOutputDestination(theDevice.GetDeviceID(), *iter))
					oss << "\t## Incompatible with " << theDevice.GetDisplayName();
				oss << endl;
				destName.clear();
			}
		oss << endl;
	}
	return oss.str ();
}


NTV2OutputDestination CNTV2DemoCommon::GetOutputDestinationFromString (const string & inStr)
{
	String2OutputDestMapConstIter iter(gString2OutputDestMap.find(inStr));
	if (iter == gString2OutputDestMap.end())
		return NTV2_OUTPUTDESTINATION_INVALID;
	return iter->second;
}


const NTV2TCIndexes CNTV2DemoCommon::GetSupportedTCIndexes (const NTV2TCIndexKinds inKinds)
{
	if (inKinds == TC_INDEXES_ALL)
		return gTCIndexes;

	NTV2TCIndexes	result;

	if (inKinds & TC_INDEXES_SDI)
		result += gTCIndexesSDI;
	if (inKinds & TC_INDEXES_ANALOG)
		result += gTCIndexesAnalog;
	if (inKinds & TC_INDEXES_ATCLTC)
		result += gTCIndexesATCLTC;
	if (inKinds & TC_INDEXES_VITC1)
		result += gTCIndexesVITC1;
	if (inKinds & TC_INDEXES_VITC2)
		result += gTCIndexesVITC2;

	return result;
}

string CNTV2DemoCommon::GetTCIndexStrings (const NTV2TCIndexKinds inKinds,
											const string inDeviceSpecifier,
											const bool inIsInputOnly)
{
	const NTV2TCIndexes &	tcIndexes	(GetSupportedTCIndexes (inKinds));
	ostringstream			oss;
	CNTV2Card				theDevice;
	if (!inDeviceSpecifier.empty ())
		CNTV2DeviceScanner::GetFirstDeviceFromArgument (inDeviceSpecifier, theDevice);

	oss	<< setw (25) << left << "Timecode Index"			<< "\t" << setw (16) << left << "Legal Values    " << endl
		<< setw (25) << left << "------------------------"	<< "\t" << setw (16) << left << "----------------" << endl;
	for (NTV2TCIndexesConstIter iter (tcIndexes.begin ());  iter != tcIndexes.end ();  ++iter)
	{
		string	tcNdxName	(::NTV2TCIndexToString (*iter));
		for (String2TCIndexMapConstIter it (gString2TCIndexMap.begin ());  it != gString2TCIndexMap.end ();  ++it)
			if (*iter == it->second)
			{
				oss << setw (25) << left << tcNdxName << "\t" << setw (16) << left << it->first;
				if (!inDeviceSpecifier.empty ()  &&  theDevice.IsOpen ())
				{
					const NTV2DeviceID	deviceID(theDevice.GetDeviceID());
					const bool canDoTCIndex	(inIsInputOnly	? ::NTV2DeviceCanDoInputTCIndex(deviceID, *iter)
															: ::NTV2DeviceCanDoTCIndex(deviceID, *iter));
					if (!canDoTCIndex)
						oss << "\t## Incompatible with " << theDevice.GetDisplayName();
				}
				oss << endl;
				tcNdxName.clear ();
			}
		oss << endl;
	}
	return oss.str ();
}


NTV2TCIndex CNTV2DemoCommon::GetTCIndexFromString (const string & inStr)
{
	String2TCIndexMapConstIter	iter	(gString2TCIndexMap.find (inStr));
	if (iter == gString2TCIndexMap.end ())
		return NTV2_TCINDEX_INVALID;
	return iter->second;
}


string CNTV2DemoCommon::GetAudioSystemStrings (const string inDeviceSpecifier)
{
	NTV2DeviceID	deviceID	(DEVICE_ID_NOTFOUND);
	string			displayName;
	ostringstream	oss;

	if (!inDeviceSpecifier.empty ())
	{
		CNTV2Card	device;
		CNTV2DeviceScanner::GetFirstDeviceFromArgument (inDeviceSpecifier, device);
		if (device.IsOpen ())
		{
			deviceID = device.GetDeviceID ();
			displayName = device.GetDisplayName ();
		}
	}

	const UWord		numAudioSystems	(::NTV2DeviceGetNumAudioSystems (deviceID));
	oss << setw(12) << left << "Audio System"	<< endl
		<< setw(12) << left << "------------"	<< endl;
	for (UWord ndx (0);  ndx < 8;  ndx++)
	{
		oss << setw(12) << left << (ndx+1);
		if (!displayName.empty ()  &&  ndx >= numAudioSystems)
			oss << "\t## Incompatible with " << displayName;
		oss << endl;
	}
	return oss.str();
}


NTV2AudioSystem CNTV2DemoCommon::GetAudioSystemFromString (const string & inStr)
{
	String2AudioSystemMapConstIter	iter	(gString2AudioSystemMap.find (inStr));
	return iter != gString2AudioSystemMap.end ()  ?  iter->second  :  NTV2_AUDIOSYSTEM_INVALID;
}


string CNTV2DemoCommon::GetTestPatternStrings (void)
{
	typedef map<string,string>	NTV2StringMap;
	NTV2StringSet keys;
	for (String2TPNamesMapConstIter it(gString2TPNamesMap.begin());  it != gString2TPNamesMap.end();  ++it)
		if (keys.find(it->second) == keys.end())
			keys.insert(it->second);

	NTV2StringMap legals;
	for (NTV2StringSet::const_iterator kit(keys.begin());  kit != keys.end();  ++kit)
	{
		const string & officialPatName(*kit);
		NTV2StringList legalValues;
		for (String2TPNamesMapConstIter it(gString2TPNamesMap.begin());  it != gString2TPNamesMap.end();  ++it)
			if (it->second == officialPatName)
				legalValues.push_back(it->first);
		legals[officialPatName] = aja::join(legalValues, ", ");
	}

	ostringstream oss;
	oss	<< setw(25) << left << "Test Pattern or Color   " << "\t" << setw(22) << left << "Legal --pattern Values" << endl
		<< setw(25) << left << "------------------------" << "\t" << setw(22) << left << "----------------------" << endl;
	for (NTV2StringMap::const_iterator it(legals.begin());  it != legals.end();  ++it)
		oss << setw(25) << left << it->first << "\t" << setw(22) << left << it->second << endl;
	return oss.str();
}


string CNTV2DemoCommon::GetTestPatternNameFromString (const string & inStr)
{
	string tpName(inStr);
	aja::lower(aja::strip(aja::replace(tpName, " ", "")));
	String2TPNamesMapConstIter it(gString2TPNamesMap.find(tpName));
	return (it != gString2TPNamesMap.end()) ? it->second : "";
}


string CNTV2DemoCommon::ToLower (const string & inStr)
{
	string	result(inStr);
	return aja::lower(result);
}


string CNTV2DemoCommon::StripFormatString (const std::string & inStr)
{
	string	result	(inStr);
	while (result.find (" ") != string::npos)
		result.erase (result.find (" "), 1);
	while (result.find ("00") != string::npos)
		result.erase (result.find ("00"), 2);
	while (result.find (".") != string::npos)
		result.erase (result.find ("."), 1);
	return result;
}


char CNTV2DemoCommon::ReadCharacterPress (void)
{
	char	result	(0);
	#if defined (AJAMac) || defined (AJALinux)
		struct termios	terminalStatus;
		::memset (&terminalStatus, 0, sizeof (terminalStatus));
		if (::tcgetattr (0, &terminalStatus) < 0)
			cerr << "tcsetattr()";
		terminalStatus.c_lflag &= ~uint32_t(ICANON);
		terminalStatus.c_lflag &= ~uint32_t(ECHO);
		terminalStatus.c_cc[VMIN] = 1;
		terminalStatus.c_cc[VTIME] = 0;
		if (::tcsetattr (0, TCSANOW, &terminalStatus) < 0)
			cerr << "tcsetattr ICANON";
		if (::read (0, &result, 1) < 0)
			cerr << "read()" << endl;
		terminalStatus.c_lflag |= ICANON;
		terminalStatus.c_lflag |= ECHO;
		if (::tcsetattr (0, TCSADRAIN, &terminalStatus) < 0)
			cerr << "tcsetattr ~ICANON" << endl;
	#elif defined (MSWindows) || defined (AJAWindows)
		HANDLE			hdl		(GetStdHandle (STD_INPUT_HANDLE));
		DWORD			nEvents	(0);
		INPUT_RECORD	buffer;
		PeekConsoleInput (hdl, &buffer, 1, &nEvents);
		if (nEvents > 0)
		{
			ReadConsoleInput (hdl, &buffer, 1, &nEvents);
			result = char (buffer.Event.KeyEvent.wVirtualKeyCode);
		}
	#endif
	return result;
}


void CNTV2DemoCommon::WaitForEnterKeyPress (void)
{
	cout << "## Press Enter/Return key to exit: ";
	cout.flush();
	cin.get();
}


TimecodeFormat CNTV2DemoCommon::NTV2FrameRate2TimecodeFormat (const NTV2FrameRate inFrameRate)
{
	TimecodeFormat	result	(kTCFormatUnknown);
	switch (inFrameRate)
	{
	case NTV2_FRAMERATE_6000:	result = kTCFormat60fps;	break;
	case NTV2_FRAMERATE_5994:	result = kTCFormat60fpsDF;	break;
	case NTV2_FRAMERATE_4800:	result = kTCFormat48fps;	break;
	case NTV2_FRAMERATE_4795:	result = kTCFormat48fps;	break;
	case NTV2_FRAMERATE_3000:	result = kTCFormat30fps;	break;
	case NTV2_FRAMERATE_2997:	result = kTCFormat30fpsDF;	break;
	case NTV2_FRAMERATE_2500:	result = kTCFormat25fps;	break;
	case NTV2_FRAMERATE_2400:	result = kTCFormat24fps;	break;
	case NTV2_FRAMERATE_2398:	result = kTCFormat24fps;	break;
	case NTV2_FRAMERATE_5000:	result = kTCFormat50fps;	break;
	default:					break;
	}
	return result;

}	//	NTV2FrameRate2TimecodeFormat


AJA_FrameRate CNTV2DemoCommon::GetAJAFrameRate (const NTV2FrameRate inFrameRate)
{
	switch (inFrameRate)
	{
		case NTV2_FRAMERATE_1498:		return AJA_FrameRate_1498;
		case NTV2_FRAMERATE_1500:		return AJA_FrameRate_1500;
#if !defined(NTV2_DEPRECATE_16_0)
		case NTV2_FRAMERATE_1798:		return AJA_FrameRate_1798;
		case NTV2_FRAMERATE_1800:		return AJA_FrameRate_1800;
		case NTV2_FRAMERATE_1898:		return AJA_FrameRate_1898;
		case NTV2_FRAMERATE_1900:		return AJA_FrameRate_1900;
#endif	//!defined(NTV2_DEPRECATE_16_0)
		case NTV2_FRAMERATE_5000:		return AJA_FrameRate_5000;
		case NTV2_FRAMERATE_2398:		return AJA_FrameRate_2398;
		case NTV2_FRAMERATE_2400:		return AJA_FrameRate_2400;
		case NTV2_FRAMERATE_2500:		return AJA_FrameRate_2500;
		case NTV2_FRAMERATE_2997:		return AJA_FrameRate_2997;
		case NTV2_FRAMERATE_3000:		return AJA_FrameRate_3000;
		case NTV2_FRAMERATE_4795:		return AJA_FrameRate_4795;
		case NTV2_FRAMERATE_4800:		return AJA_FrameRate_4800;
		case NTV2_FRAMERATE_5994:		return AJA_FrameRate_5994;
		case NTV2_FRAMERATE_6000:		return AJA_FrameRate_6000;
		case NTV2_FRAMERATE_12000:		return AJA_FrameRate_12000;
		case NTV2_FRAMERATE_11988:		return AJA_FrameRate_11988;

		case NTV2_NUM_FRAMERATES:
		case NTV2_FRAMERATE_UNKNOWN:	break;
	}
	return AJA_FrameRate_Unknown;
}	//	GetAJAFrameRate


AJA_PixelFormat CNTV2DemoCommon::GetAJAPixelFormat (const NTV2FrameBufferFormat inFormat)
{
	switch (inFormat)
	{
		case NTV2_FBF_10BIT_YCBCR:				return AJA_PixelFormat_YCbCr10;
		case NTV2_FBF_8BIT_YCBCR:				return AJA_PixelFormat_YCbCr8;
		case NTV2_FBF_ARGB:						return AJA_PixelFormat_ARGB8;
		case NTV2_FBF_RGBA:						return AJA_PixelFormat_RGBA8;
		case NTV2_FBF_10BIT_RGB:				return AJA_PixelFormat_RGB10;
		case NTV2_FBF_8BIT_YCBCR_YUY2:			return AJA_PixelFormat_YUY28;
		case NTV2_FBF_ABGR:						return AJA_PixelFormat_ABGR8;
		case NTV2_FBF_10BIT_DPX:				return AJA_PixelFormat_RGB_DPX;
		case NTV2_FBF_10BIT_YCBCR_DPX:			return AJA_PixelFormat_YCbCr_DPX;
		case NTV2_FBF_8BIT_DVCPRO:				return AJA_PixelFormat_DVCPRO;
		case NTV2_FBF_8BIT_HDV:					return AJA_PixelFormat_HDV;
		case NTV2_FBF_24BIT_RGB:				return AJA_PixelFormat_RGB8_PACK;
		case NTV2_FBF_24BIT_BGR:				return AJA_PixelFormat_BGR8_PACK;
		case NTV2_FBF_10BIT_YCBCRA:				return AJA_PixelFormat_YCbCrA10;
        case NTV2_FBF_10BIT_DPX_LE:             return AJA_PixelFormat_RGB_DPX_LE;
		case NTV2_FBF_48BIT_RGB:				return AJA_PixelFormat_RGB16;
		case NTV2_FBF_12BIT_RGB_PACKED:			return AJA_PixelFormat_RGB12P;
		case NTV2_FBF_PRORES_DVCPRO:			return AJA_PixelFormat_PRORES_DVPRO;
		case NTV2_FBF_PRORES_HDV:				return AJA_PixelFormat_PRORES_HDV;
		case NTV2_FBF_10BIT_RGB_PACKED:			return AJA_PixelFormat_RGB10_PACK;

		case NTV2_FBF_8BIT_YCBCR_420PL2:		return AJA_PixelFormat_YCBCR8_420PL;
		case NTV2_FBF_8BIT_YCBCR_422PL2:		return AJA_PixelFormat_YCBCR8_422PL;
		case NTV2_FBF_10BIT_YCBCR_420PL2:		return AJA_PixelFormat_YCBCR10_420PL;
		case NTV2_FBF_10BIT_YCBCR_422PL2:		return AJA_PixelFormat_YCBCR10_422PL;

		case NTV2_FBF_8BIT_YCBCR_420PL3:		return AJA_PixelFormat_YCBCR8_420PL3;
		case NTV2_FBF_8BIT_YCBCR_422PL3:		return AJA_PixelFormat_YCBCR8_422PL3;
		case NTV2_FBF_10BIT_YCBCR_420PL3_LE:	return AJA_PixelFormat_YCBCR10_420PL3LE;
		case NTV2_FBF_10BIT_YCBCR_422PL3_LE:	return AJA_PixelFormat_YCBCR10_422PL3LE;

		case NTV2_FBF_10BIT_RAW_RGB:
		case NTV2_FBF_10BIT_RAW_YCBCR:
		case NTV2_FBF_10BIT_ARGB:
		case NTV2_FBF_16BIT_ARGB:
		case NTV2_FBF_INVALID:					break;
	}
	return AJA_PixelFormat_Unknown;
}	//	GetAJAPixelFormat


bool CNTV2DemoCommon::Get4KInputFormat (NTV2VideoFormat & inOutVideoFormat)
{
	static struct	VideoFormatPair
	{
		NTV2VideoFormat	vIn;
		NTV2VideoFormat	vOut;
	} VideoFormatPairs[] =	{	//			vIn								vOut
								{NTV2_FORMAT_1080psf_2398,		NTV2_FORMAT_4x1920x1080psf_2398},
								{NTV2_FORMAT_1080psf_2400,		NTV2_FORMAT_4x1920x1080psf_2400},
								{NTV2_FORMAT_1080p_2398,		NTV2_FORMAT_4x1920x1080p_2398},
								{NTV2_FORMAT_1080p_2400,		NTV2_FORMAT_4x1920x1080p_2400},
								{NTV2_FORMAT_1080p_2500,		NTV2_FORMAT_4x1920x1080p_2500},
								{NTV2_FORMAT_1080p_2997,		NTV2_FORMAT_4x1920x1080p_2997},
								{NTV2_FORMAT_1080p_3000,		NTV2_FORMAT_4x1920x1080p_3000},
								{NTV2_FORMAT_1080p_5000_B,		NTV2_FORMAT_4x1920x1080p_5000},
								{NTV2_FORMAT_1080p_5994_B,		NTV2_FORMAT_4x1920x1080p_5994},
								{NTV2_FORMAT_1080p_6000_B,		NTV2_FORMAT_4x1920x1080p_6000},
								{NTV2_FORMAT_1080p_2K_2398,		NTV2_FORMAT_4x2048x1080p_2398},
								{NTV2_FORMAT_1080p_2K_2400,		NTV2_FORMAT_4x2048x1080p_2400},
								{NTV2_FORMAT_1080p_2K_2500,		NTV2_FORMAT_4x2048x1080p_2500},
								{NTV2_FORMAT_1080p_2K_2997,		NTV2_FORMAT_4x2048x1080p_2997},
								{NTV2_FORMAT_1080p_2K_3000,		NTV2_FORMAT_4x2048x1080p_3000},
								{NTV2_FORMAT_1080p_2K_5000_A,	NTV2_FORMAT_4x2048x1080p_5000},
								{NTV2_FORMAT_1080p_2K_5994_A,	NTV2_FORMAT_4x2048x1080p_5994},
								{NTV2_FORMAT_1080p_2K_6000_A,	NTV2_FORMAT_4x2048x1080p_6000},

								{NTV2_FORMAT_1080p_5000_A,		NTV2_FORMAT_4x1920x1080p_5000},
								{NTV2_FORMAT_1080p_5994_A,		NTV2_FORMAT_4x1920x1080p_5994},
								{NTV2_FORMAT_1080p_6000_A,		NTV2_FORMAT_4x1920x1080p_6000},

								{NTV2_FORMAT_1080p_2K_5000_A,	NTV2_FORMAT_4x2048x1080p_5000},
								{NTV2_FORMAT_1080p_2K_5994_A,	NTV2_FORMAT_4x2048x1080p_5994},
								{NTV2_FORMAT_1080p_2K_6000_A,	NTV2_FORMAT_4x2048x1080p_6000}
	};
	for (size_t formatNdx(0);  formatNdx < sizeof(VideoFormatPairs) / sizeof(VideoFormatPair);  formatNdx++)
		if (VideoFormatPairs[formatNdx].vIn == inOutVideoFormat)
		{
			inOutVideoFormat = VideoFormatPairs[formatNdx].vOut;
			return true;
		}
	return false;

}	//	get4KInputFormat

bool CNTV2DemoCommon::Get8KInputFormat (NTV2VideoFormat & inOutVideoFormat)
{
	static struct	VideoFormatPair
	{
		NTV2VideoFormat	vIn;
		NTV2VideoFormat	vOut;
	} VideoFormatPairs[] =	{	//			vIn								vOut
								{NTV2_FORMAT_3840x2160p_2398,		NTV2_FORMAT_4x3840x2160p_2398},
								{NTV2_FORMAT_3840x2160p_2400,		NTV2_FORMAT_4x3840x2160p_2400},
								{NTV2_FORMAT_3840x2160p_2500,		NTV2_FORMAT_4x3840x2160p_2500},
								{NTV2_FORMAT_3840x2160p_2997,		NTV2_FORMAT_4x3840x2160p_2997},
								{NTV2_FORMAT_3840x2160p_3000,		NTV2_FORMAT_4x3840x2160p_3000},
								{NTV2_FORMAT_3840x2160p_5000,		NTV2_FORMAT_4x3840x2160p_5000},
								{NTV2_FORMAT_3840x2160p_5994,		NTV2_FORMAT_4x3840x2160p_5994},
								{NTV2_FORMAT_3840x2160p_6000,		NTV2_FORMAT_4x3840x2160p_6000},
								{NTV2_FORMAT_3840x2160p_5000_B,		NTV2_FORMAT_4x3840x2160p_5000_B},
								{NTV2_FORMAT_3840x2160p_5994_B,		NTV2_FORMAT_4x3840x2160p_5994_B},
								{NTV2_FORMAT_3840x2160p_6000_B,		NTV2_FORMAT_4x3840x2160p_6000_B},
								{NTV2_FORMAT_4096x2160p_2398,		NTV2_FORMAT_4x4096x2160p_2398},
								{NTV2_FORMAT_4096x2160p_2400,		NTV2_FORMAT_4x4096x2160p_2400},
								{NTV2_FORMAT_4096x2160p_2500,		NTV2_FORMAT_4x4096x2160p_2500},
								{NTV2_FORMAT_4096x2160p_2997,		NTV2_FORMAT_4x4096x2160p_2997},
								{NTV2_FORMAT_4096x2160p_3000,		NTV2_FORMAT_4x4096x2160p_3000},
								{NTV2_FORMAT_4096x2160p_4795,		NTV2_FORMAT_4x4096x2160p_4795},
								{NTV2_FORMAT_4096x2160p_4800,		NTV2_FORMAT_4x4096x2160p_4800},
								{NTV2_FORMAT_4096x2160p_5000,		NTV2_FORMAT_4x4096x2160p_5000},
								{NTV2_FORMAT_4096x2160p_5994,		NTV2_FORMAT_4x4096x2160p_5994},
								{NTV2_FORMAT_4096x2160p_6000,		NTV2_FORMAT_4x4096x2160p_6000},
								{NTV2_FORMAT_4096x2160p_4795_B,		NTV2_FORMAT_4x4096x2160p_4795_B},
								{NTV2_FORMAT_4096x2160p_4800_B,		NTV2_FORMAT_4x4096x2160p_4800_B},
								{NTV2_FORMAT_4096x2160p_5000_B,		NTV2_FORMAT_4x4096x2160p_5000_B},
								{NTV2_FORMAT_4096x2160p_5994_B,		NTV2_FORMAT_4x4096x2160p_5994_B},
								{NTV2_FORMAT_4096x2160p_6000_B,		NTV2_FORMAT_4x4096x2160p_6000_B}
	};
	for (size_t formatNdx(0);  formatNdx < sizeof(VideoFormatPairs) / sizeof(VideoFormatPair);  formatNdx++)
		if (VideoFormatPairs[formatNdx].vIn == inOutVideoFormat)
		{
			inOutVideoFormat = VideoFormatPairs[formatNdx].vOut;
			return true;
		}
	return false;

}	//	get8KInputFormat


const char * CNTV2DemoCommon::GetGlobalMutexName (void)
{
	return gGlobalMutexName.c_str();
}

static UWord GetNumTSIMuxers (const NTV2DeviceID inDeviceID)
{
	UWord result(0);
	static const NTV2WidgetID s425MuxerIDs[] = {NTV2_Wgt425Mux1, NTV2_Wgt425Mux2, NTV2_Wgt425Mux3, NTV2_Wgt425Mux4};
	for (size_t ndx(0);  ndx < sizeof(s425MuxerIDs)/sizeof(NTV2WidgetID);  ndx++)
		if (::NTV2DeviceCanDoWidget(inDeviceID, s425MuxerIDs[ndx]))
			result++;
	return result;
}

NTV2ChannelList CNTV2DemoCommon::GetTSIMuxesForFrameStore (const NTV2DeviceID inDeviceID, const NTV2Channel in1stFrameStore, const UWord inCount)
{
	UWord totFrameStores(::NTV2DeviceGetNumFrameStores(inDeviceID));
	UWord totTSIMuxers(::GetNumTSIMuxers(inDeviceID));
	UWord tsiMux(in1stFrameStore);
	NTV2ChannelList result;
	if (totFrameStores > totTSIMuxers)
		tsiMux = in1stFrameStore/2;
	else if (totFrameStores < totTSIMuxers)
		tsiMux = in1stFrameStore*2;
	for (UWord num(0);  num < inCount;  num++)
		result.push_back(NTV2Channel(tsiMux + num));
	return result;
}

string CNTV2DemoCommon::ACFrameRange::setFromString (const string & inStr)
{
	makeInvalid();
	if (inStr.empty())
		return "Frame count/range not specified";
	const bool hasCount(inStr.find('@') != string::npos);
	const bool hasRange(inStr.find('-') != string::npos);
	NTV2StringList	strs;
	if (hasCount && hasRange)
		return "'@' and '-' cannot both be specified";
	else if (hasCount)
		aja::split(inStr, '@', strs);
	else if (hasRange)
		aja::split(inStr, '-', strs);
	else
		strs.push_back(inStr);
	if (strs.empty())
		return "No frame count/range values parsed";
	if (strs.size() > 2)
		return "More than 2 frame count/range values parsed";
	if (hasCount || hasRange)
		if (strs.size() != 2)
			return "Expected exactly 2 frame count/range values";

	//	Check that all characters are decimal digits...
	for (size_t strNdx(0);  strNdx < strs.size();  strNdx++)
	{	string	str(strs.at(strNdx));
		if (aja::strip(str).empty())
			return "Expected unsigned decimal integer value";
		for (size_t chNdx(0);  chNdx < str.length();  chNdx++)
			if (!isdigit(str.at(chNdx)))
				return "Non-digit character encountered in '" + str + "'";
	}

	UWordSequence numbers;
	for (NTV2StringListConstIter it(strs.begin());  it != strs.end();  ++it)
	{
		string	str(*it);
		numbers.push_back(UWord(aja::stoul(aja::strip(str))));
	}
	if (hasCount)
		{mIsCountOnly = false;  mFrameCount = 0;  mFirstFrame = numbers[1];  mLastFrame = mFirstFrame + numbers[0] - 1;}
	else if (hasRange)
		{mIsCountOnly = false;  mFrameCount = 0;  mFirstFrame = numbers[0];  mLastFrame = numbers[1];}
	else
		{mIsCountOnly = true;  mFrameCount = numbers[0];  mFirstFrame = mLastFrame = 0;}
	if (!isCount()  &&  lastFrame() < firstFrame())
		{makeInvalid();  return "First frame past last frame";}
	return "";
}

string CNTV2DemoCommon::ACFrameRange::toString(void) const
{
	ostringstream oss;
	if (!valid())
		oss << "<invalid>";
	else if (isFrameRange())
		oss << "Frames " << DEC(firstFrame()) << "-" << DEC(lastFrame()) << " (" << DEC(lastFrame()-firstFrame()+1) << "@" << DEC(firstFrame()) << ")";
	else
		oss << DEC(count()) << " frames (auto-allocated)";
	return oss.str();
}

CNTV2DemoCommon::Popt::Popt (const int inArgc, const char ** pArgs, const PoptOpts * pInOptionsTable)
{
	mContext = ::poptGetContext(AJA_NULL, inArgc, pArgs, pInOptionsTable, 0);
	mResult = ::poptGetNextOpt(mContext);
	if (mResult < -1)
	{	ostringstream oss;
		oss << ::poptBadOption(mContext, 0) << ": " << ::poptStrerror(mResult);
		mError = oss.str();
	}
}

CNTV2DemoCommon::Popt::~Popt()
{
	mContext = ::poptFreeContext(mContext);
}


bool CNTV2DemoCommon::BFT(void)
{
	typedef struct {string fName; NTV2VideoFormat fFormat;} FormatNameDictionary;
	static const FormatNameDictionary sVFmtDict[] = {
								{"1080i50",				NTV2_FORMAT_1080i_5000},
								{"1080i",				NTV2_FORMAT_1080i_5994},
								{"1080i5994",			NTV2_FORMAT_1080i_5994},
								{"hd",					NTV2_FORMAT_1080i_5994},
								{"1080i60",				NTV2_FORMAT_1080i_6000},
								{"720p",				NTV2_FORMAT_720p_5994},
								{"720p5994",			NTV2_FORMAT_720p_5994},
								{"720p60",				NTV2_FORMAT_720p_6000},
								{"1080psf2398",			NTV2_FORMAT_1080psf_2398},
								{"1080psf24",			NTV2_FORMAT_1080psf_2400},
								{"1080p2997",			NTV2_FORMAT_1080p_2997},
								{"1080p30",				NTV2_FORMAT_1080p_3000},
								{"1080p25",				NTV2_FORMAT_1080p_2500},
								{"1080p2398",			NTV2_FORMAT_1080p_2398},
								{"1080p24",				NTV2_FORMAT_1080p_2400},
								{"2048x1080p2398",		NTV2_FORMAT_1080p_2K_2398},
								{"2048x1080p24",		NTV2_FORMAT_1080p_2K_2400},
								{"2048x1080psf2398",	NTV2_FORMAT_1080psf_2K_2398},
								{"2048x1080psf24",		NTV2_FORMAT_1080psf_2K_2400},
								{"720p50",				NTV2_FORMAT_720p_5000},
								{"1080p50b",			NTV2_FORMAT_1080p_5000_B},
								{"1080p",				NTV2_FORMAT_1080p_5994_B},
								{"1080p5994b",			NTV2_FORMAT_1080p_5994_B},
								{"1080p60b",			NTV2_FORMAT_1080p_6000_B},
								{"720p2398",			NTV2_FORMAT_720p_2398},
								{"720p25",				NTV2_FORMAT_720p_2500},
								{"1080p50",				NTV2_FORMAT_1080p_5000_A},
								{"1080p5994",			NTV2_FORMAT_1080p_5994_A},
								{"1080p60",				NTV2_FORMAT_1080p_6000_A},
								{"2048x1080p25",		NTV2_FORMAT_1080p_2K_2500},
								{"2048x1080psf25",		NTV2_FORMAT_1080psf_2K_2500},
								{"1080psf25",			NTV2_FORMAT_1080psf_2500_2},
								{"1080psf2997",			NTV2_FORMAT_1080psf_2997_2},
								{"1080psf30",			NTV2_FORMAT_1080psf_3000_2},
								{"525i",				NTV2_FORMAT_525_5994},
								{"525i2997",			NTV2_FORMAT_525_5994},
								{"sd",					NTV2_FORMAT_525_5994},
								{"625i",				NTV2_FORMAT_625_5000},
								{"625i25",				NTV2_FORMAT_625_5000},
								{"525i2398",			NTV2_FORMAT_525_2398},
								{"525i24",				NTV2_FORMAT_525_2400},
								{"525psf2997",			NTV2_FORMAT_525psf_2997},
								{"625psf25",			NTV2_FORMAT_625psf_2500},
								{"2048x1556psf1498",	NTV2_FORMAT_2K_1498},
								{"2048x1556psf15",		NTV2_FORMAT_2K_1500},
								{"2048x1556psf2398",	NTV2_FORMAT_2K_2398},
								{"2048x1556psf24",		NTV2_FORMAT_2K_2400},
								{"2048x1556psf25",		NTV2_FORMAT_2K_2500},
								{"4x1920x1080psf2398",	NTV2_FORMAT_4x1920x1080psf_2398},
								{"4x1920x1080psf24",	NTV2_FORMAT_4x1920x1080psf_2400},
								{"4x1920x1080psf25",	NTV2_FORMAT_4x1920x1080psf_2500},
								{"4x1920x1080p2398",	NTV2_FORMAT_4x1920x1080p_2398},
								{"uhd2398",				NTV2_FORMAT_4x1920x1080p_2398},
								{"4x1920x1080p24",		NTV2_FORMAT_4x1920x1080p_2400},
								{"uhd24",				NTV2_FORMAT_4x1920x1080p_2400},
								{"4x1920x1080p25",		NTV2_FORMAT_4x1920x1080p_2500},
								{"uhd25",				NTV2_FORMAT_4x1920x1080p_2500},
								{"4x2048x1080psf2398",	NTV2_FORMAT_4x2048x1080psf_2398},
								{"4x2048x1080psf24",	NTV2_FORMAT_4x2048x1080psf_2400},
								{"4x2048x1080psf25",	NTV2_FORMAT_4x2048x1080psf_2500},
								{"4k2398",				NTV2_FORMAT_4x2048x1080p_2398},
								{"4x2048x1080p2398",	NTV2_FORMAT_4x2048x1080p_2398},
								{"4k24",				NTV2_FORMAT_4x2048x1080p_2400},
								{"4x2048x1080p24",		NTV2_FORMAT_4x2048x1080p_2400},
								{"4k25",				NTV2_FORMAT_4x2048x1080p_2500},
								{"4x2048x1080p25",		NTV2_FORMAT_4x2048x1080p_2500},
								{"4x1920x1080p2997",	NTV2_FORMAT_4x1920x1080p_2997},
								{"4x1920x1080p30",		NTV2_FORMAT_4x1920x1080p_3000},
								{"4x1920x1080psf2997",	NTV2_FORMAT_4x1920x1080psf_2997},
								{"4x1920x1080psf30",	NTV2_FORMAT_4x1920x1080psf_3000},
								{"4x2048x1080p2997",	NTV2_FORMAT_4x2048x1080p_2997},
								{"4x2048x1080p30",		NTV2_FORMAT_4x2048x1080p_3000},
								{"4x2048x1080psf2997",	NTV2_FORMAT_4x2048x1080psf_2997},
								{"4x2048x1080psf30",	NTV2_FORMAT_4x2048x1080psf_3000},
								{"4x1920x1080p50",		NTV2_FORMAT_4x1920x1080p_5000},
								{"uhd50",				NTV2_FORMAT_4x1920x1080p_5000},
								{"4x1920x1080p5994",	NTV2_FORMAT_4x1920x1080p_5994},
								{"uhd5994",				NTV2_FORMAT_4x1920x1080p_5994},
								{"4x1920x1080p60",		NTV2_FORMAT_4x1920x1080p_6000},
								{"uhd",					NTV2_FORMAT_4x1920x1080p_6000},
								{"uhd60",				NTV2_FORMAT_4x1920x1080p_6000},
								{"4k50",				NTV2_FORMAT_4x2048x1080p_5000},
								{"4x2048x1080p50",		NTV2_FORMAT_4x2048x1080p_5000},
								{"4k5994",				NTV2_FORMAT_4x2048x1080p_5994},
								{"4x2048x1080p5994",	NTV2_FORMAT_4x2048x1080p_5994},
								{"4k",					NTV2_FORMAT_4x2048x1080p_6000},
								{"4k60",				NTV2_FORMAT_4x2048x1080p_6000},
								{"4x2048x1080p60",		NTV2_FORMAT_4x2048x1080p_6000},
								{"4k4795",				NTV2_FORMAT_4x2048x1080p_4795},
								{"4x2048x1080p4795",	NTV2_FORMAT_4x2048x1080p_4795},
								{"4k48",				NTV2_FORMAT_4x2048x1080p_4800},
								{"4x2048x1080p48",		NTV2_FORMAT_4x2048x1080p_4800},
								{"4k11988",				NTV2_FORMAT_4x2048x1080p_11988},
								{"4x2048x1080p11988",	NTV2_FORMAT_4x2048x1080p_11988},
								{"4k120",				NTV2_FORMAT_4x2048x1080p_12000},
								{"4x2048x1080p120",		NTV2_FORMAT_4x2048x1080p_12000},
								{"2048x1080p60",		NTV2_FORMAT_1080p_2K_6000_A},
								{"2048x1080p5994",		NTV2_FORMAT_1080p_2K_5994_A},
								{"2048x1080p2997",		NTV2_FORMAT_1080p_2K_2997},
								{"2048x1080p30",		NTV2_FORMAT_1080p_2K_3000},
								{"2048x1080p50",		NTV2_FORMAT_1080p_2K_5000_A},
								{"2048x1080p4795",		NTV2_FORMAT_1080p_2K_4795_A},
								{"2048x1080p48",		NTV2_FORMAT_1080p_2K_4800_A},
								{"2048x1080p60b",		NTV2_FORMAT_1080p_2K_6000_B},
								{"2048x1080p5994b",		NTV2_FORMAT_1080p_2K_5994_B},
								{"2048x1080p50b",		NTV2_FORMAT_1080p_2K_5000_B},
								{"2048x1080p48b",		NTV2_FORMAT_1080p_2K_4800_B},
								{"2048x1080p4795b",		NTV2_FORMAT_1080p_2K_4795_B},
								{"",					NTV2_FORMAT_UNKNOWN}	};
	if (true)
	{
		//	Dump the gString2VideoFormatMap map...
		for (String2VideoFormatMapConstIter it(gString2VideoFormatMap.begin());  it != gString2VideoFormatMap.end();  ++it)
		{
			cout << "'" << it->first << "'\t'" << ::NTV2VideoFormatToString(it->second) << "'\t" << ::NTV2VideoFormatString(it->second) << "\t" << DEC(it->second) << endl;
		}
	}
	cout << endl << endl;
	for (unsigned ndx(0);  !sVFmtDict[ndx].fName.empty();  ndx++)
	{
		const string &			str		(sVFmtDict[ndx].fName);
		const NTV2VideoFormat	vFormat	(sVFmtDict[ndx].fFormat);
		String2VideoFormatMapConstIter	it	(gString2VideoFormatMap.find(str));
		const NTV2VideoFormat	vFormat2	(it != gString2VideoFormatMap.end() ? it->second : NTV2_FORMAT_UNKNOWN);
		if (vFormat != vFormat2)
			cerr << "'" << str << "': '" << ::NTV2VideoFormatString(vFormat) << "' (" << DEC(vFormat) << ") != '" << ::NTV2VideoFormatString(vFormat2) << "' (" << DEC(vFormat2) << ")" << endl;
		//SHOULD_BE_EQUAL(vFormat, vFormat2);
	}
	if (true)
	{
		CNTV2DemoCommon::ACFrameRange fRange(0);
		SHOULD_BE_FALSE(fRange.valid());
		cerr << fRange.setFromString("") << endl;
		SHOULD_BE_FALSE(fRange.valid());	//	Nothing -- empty string
		cerr << fRange.setFromString("    \t    ") << endl;
		SHOULD_BE_FALSE(fRange.valid());	//	Nothing -- whitespace

		cerr << fRange.setFromString("10") << endl;
		SHOULD_BE_TRUE(fRange.valid());
		SHOULD_BE_TRUE(fRange.isCount());
		SHOULD_BE_FALSE(fRange.isFrameRange());
		SHOULD_BE_EQUAL(fRange.count(), 10);

		SHOULD_BE_FALSE(fRange.makeInvalid().valid());

		cerr << fRange.setFromString("   \t   15   \t   ") << endl;
		SHOULD_BE_TRUE(fRange.valid());
		SHOULD_BE_TRUE(fRange.isCount());
		SHOULD_BE_FALSE(fRange.isFrameRange());
		SHOULD_BE_EQUAL(fRange.count(), 15);

		cerr << fRange.setFromString("@") << endl;
		SHOULD_BE_FALSE(fRange.valid());	//	Missing integer values
		cerr << fRange.setFromString("20@") << endl;
		SHOULD_BE_FALSE(fRange.valid());	//	Missing 2nd integer value
		cerr << fRange.setFromString("@20") << endl;
		SHOULD_BE_FALSE(fRange.valid());	//	Missing 1st integer value

		cerr << fRange.setFromString("20@10") << endl;
		SHOULD_BE_TRUE(fRange.valid());
		SHOULD_BE_FALSE(fRange.isCount());
		SHOULD_BE_TRUE(fRange.isFrameRange());
		SHOULD_BE_EQUAL(fRange.count(), 0);
		SHOULD_BE_EQUAL(fRange.firstFrame(), 10);
		SHOULD_BE_EQUAL(fRange.lastFrame(), 29);

		cerr << fRange.setFromString("   \t   25   @   15   \t   ") << endl;
		SHOULD_BE_TRUE(fRange.valid());
		SHOULD_BE_FALSE(fRange.isCount());
		SHOULD_BE_TRUE(fRange.isFrameRange());
		SHOULD_BE_EQUAL(fRange.count(), 0);
		SHOULD_BE_EQUAL(fRange.firstFrame(), 15);
		SHOULD_BE_EQUAL(fRange.lastFrame(), 39);

		cerr << fRange.setFromString("   \t   2.5   @   1 $ 5   \t   ") << endl;
		SHOULD_BE_FALSE(fRange.valid());
		cerr << fRange.setFromString("~!@#$%^&*()_+{}|[]:;<>?/.,`") << endl;
		SHOULD_BE_FALSE(fRange.valid());
		cerr << fRange.setFromString("@@@@@@@@@--------") << endl;
		SHOULD_BE_FALSE(fRange.valid());
		cerr << fRange.setFromString("1@2@3@4@5@6@7@8@9@1") << endl;
		SHOULD_BE_FALSE(fRange.valid());

		cerr << fRange.setFromString("-") << endl;
		SHOULD_BE_FALSE(fRange.valid());	//	Missing integer values
		cerr << fRange.setFromString("10-") << endl;
		SHOULD_BE_FALSE(fRange.valid());	//	Missing 2nd integer value
		cerr << fRange.setFromString("-10") << endl;
		SHOULD_BE_FALSE(fRange.valid());	//	Missing 1st integer value
		cerr << fRange.setFromString("1-2-3-4-5-6-7-8-9-1") << endl;
		SHOULD_BE_FALSE(fRange.valid());
		cerr << fRange.setFromString("-1-2-3-4-5-6-7-8-9-") << endl;
		SHOULD_BE_FALSE(fRange.valid());

		cerr << fRange.setFromString("20-30") << endl;
		SHOULD_BE_TRUE(fRange.valid());
		SHOULD_BE_FALSE(fRange.isCount());
		SHOULD_BE_TRUE(fRange.isFrameRange());
		SHOULD_BE_EQUAL(fRange.count(), 0);
		SHOULD_BE_EQUAL(fRange.firstFrame(), 20);
		SHOULD_BE_EQUAL(fRange.lastFrame(), 30);

		cerr << fRange.setFromString("2.0-3#0") << endl;
		SHOULD_BE_FALSE(fRange.valid());

		cerr << fRange.setFromString("                   25            -                35         ") << endl;
		SHOULD_BE_TRUE(fRange.valid());
		SHOULD_BE_FALSE(fRange.isCount());
		SHOULD_BE_TRUE(fRange.isFrameRange());
		SHOULD_BE_EQUAL(fRange.count(), 0);
		SHOULD_BE_EQUAL(fRange.firstFrame(), 25);
		SHOULD_BE_EQUAL(fRange.lastFrame(), 35);

		cerr << fRange.setFromString("36-36") << endl;
		SHOULD_BE_TRUE(fRange.valid());
		SHOULD_BE_FALSE(fRange.isCount());
		SHOULD_BE_TRUE(fRange.isFrameRange());
		SHOULD_BE_EQUAL(fRange.count(), 0);
		SHOULD_BE_EQUAL(fRange.firstFrame(), 36);
		SHOULD_BE_EQUAL(fRange.lastFrame(), 36);

		cerr << fRange.setFromString("36-1") << endl;
		SHOULD_BE_FALSE(fRange.valid());
	}
	return true;
}
