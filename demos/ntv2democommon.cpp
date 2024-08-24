/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2democommon.cpp
	@brief		Common implementation code used by many of the demo applications.
	@copyright	(C) 2013-2022 AJA Video Systems, Inc.  All rights reserved.
**/

#include "ntv2democommon.h"
#include "ntv2devicescanner.h"
#include "ntv2devicefeatures.h"
#include "ntv2testpatterngen.h"
#include "ntv2debug.h"
#include "ntv2utils.h"
#include "ajabase/common/common.h"
#include <algorithm>
#include <map>
#include <iomanip>
#if defined (AJAMac) || defined (AJALinux)
	#include <unistd.h>
	#include <termios.h>
#endif

using namespace std;

typedef	multimap <string, NTV2VideoFormat>		String2VideoFormatMMap;
typedef	String2VideoFormatMMap::const_iterator	String2VideoFormatMMapCI;

typedef	map <string, NTV2FrameBufferFormat>		String2PixelFormatMap;
typedef	String2PixelFormatMap::const_iterator	String2PixelFormatMapConstIter;

typedef	map <string, NTV2AudioSystem>			String2AudioSystemMap;
typedef	String2AudioSystemMap::const_iterator	String2AudioSystemMapConstIter;

typedef	map <string, NTV2VANCMode>				String2VANCModeMap;
typedef	String2VANCModeMap::const_iterator		String2VANCModeMapConstIter;

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
static NTV2VideoFormatSet		gSDHDFormats;
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
static String2VideoFormatMMap	gString2VideoFormatMMap;
static String2PixelFormatMap	gString2PixelFormatMap;
static String2AudioSystemMap	gString2AudioSystemMap;
static String2VANCModeMap		gString2VANCModeMap;
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
			typedef pair <string, NTV2VANCMode>				String2VANCModePair;
			typedef	pair <string, NTV2InputSource>			String2InputSourcePair;
			typedef	pair <string, NTV2OutputDestination>	String2OutputDestPair;

			NTV2_ASSERT (gSDHDFormats.empty());
			for (NTV2VideoFormat legalFormat(NTV2_FORMAT_UNKNOWN);  legalFormat < NTV2_MAX_NUM_VIDEO_FORMATS;  legalFormat = NTV2VideoFormat(legalFormat + 1))
			{
				if (!NTV2_IS_VALID_VIDEO_FORMAT(legalFormat))
					continue;
				string str(::NTV2VideoFormatToString (legalFormat));

				if (NTV2_IS_QUAD_QUAD_FORMAT(legalFormat))
					g8KFormats.insert(legalFormat);
				else if (NTV2_IS_4K_VIDEO_FORMAT(legalFormat))
					g4KFormats.insert(legalFormat);
				else
					gSDHDFormats.insert(legalFormat);
				gAllFormats.insert(legalFormat);

				if		(legalFormat == NTV2_FORMAT_525_5994)	str = "525i2997";
				else if	(legalFormat == NTV2_FORMAT_625_5000)	str = "625i25";
				else if	(legalFormat == NTV2_FORMAT_525_2398)	str = "525i2398";
				else if	(legalFormat == NTV2_FORMAT_525_2400)	str = "525i24";
				else
				{
					if (str.at (str.length() - 1) == 'a')	//	If it ends in "a"...
						str.erase (str.length() - 1);		//	...lop off the "a"

					if (str.find(".00") != string::npos)	//	If it ends in ".00"...
						str.erase (str.find(".00"), 3);	//	...lop off the ".00" (but keep the "b", if any)

					while (str.find(" ") != string::npos)
						str.erase (str.find(" "), 1);		//	Remove all spaces

					if (str.find(".") != string::npos)
						str.erase (str.find("."), 1);		//	Remove "."

					str = CNTV2DemoCommon::ToLower(str);	//	Fold to lower case
				}
				gString2VideoFormatMMap.insert (String2VideoFormatPair (str, legalFormat));
				gString2VideoFormatMMap.insert (String2VideoFormatPair (ULWordToString(legalFormat), legalFormat));
			}	//	for each video format supported in demo apps

			//	Add popular format names...
			gString2VideoFormatMMap.insert (String2VideoFormatPair ("sd",			NTV2_FORMAT_525_5994));
			gString2VideoFormatMMap.insert (String2VideoFormatPair ("525i",			NTV2_FORMAT_525_5994));
			gString2VideoFormatMMap.insert (String2VideoFormatPair ("625i",			NTV2_FORMAT_625_5000));
			gString2VideoFormatMMap.insert (String2VideoFormatPair ("720p",			NTV2_FORMAT_720p_5994));
			gString2VideoFormatMMap.insert (String2VideoFormatPair ("hd",			NTV2_FORMAT_1080i_5994));
			gString2VideoFormatMMap.insert (String2VideoFormatPair ("1080i",		NTV2_FORMAT_1080i_5994));
			gString2VideoFormatMMap.insert (String2VideoFormatPair ("1080i50",		NTV2_FORMAT_1080i_5000));
			gString2VideoFormatMMap.insert (String2VideoFormatPair ("1080p",		NTV2_FORMAT_1080p_5994_B));
			gString2VideoFormatMMap.insert (String2VideoFormatPair ("1080p50",		NTV2_FORMAT_1080p_5000_B));

			gString2VideoFormatMMap.insert (String2VideoFormatPair ("uhd",			NTV2_FORMAT_4x1920x1080p_6000));
			gString2VideoFormatMMap.insert (String2VideoFormatPair ("uhd",			NTV2_FORMAT_3840x2160p_6000));
			gString2VideoFormatMMap.insert (String2VideoFormatPair ("uhd2398",		NTV2_FORMAT_4x1920x1080p_2398));
			gString2VideoFormatMMap.insert (String2VideoFormatPair ("uhd2398",		NTV2_FORMAT_3840x2160p_2398));
			gString2VideoFormatMMap.insert (String2VideoFormatPair ("uhd24",		NTV2_FORMAT_4x1920x1080p_2400));
			gString2VideoFormatMMap.insert (String2VideoFormatPair ("uhd24",		NTV2_FORMAT_3840x2160p_2400));
			gString2VideoFormatMMap.insert (String2VideoFormatPair ("uhd25",		NTV2_FORMAT_4x1920x1080p_2500));
			gString2VideoFormatMMap.insert (String2VideoFormatPair ("uhd25",		NTV2_FORMAT_3840x2160p_2500));
			gString2VideoFormatMMap.insert (String2VideoFormatPair ("uhd50",		NTV2_FORMAT_4x1920x1080p_5000));
			gString2VideoFormatMMap.insert (String2VideoFormatPair ("uhd50",		NTV2_FORMAT_3840x2160p_5000));
			gString2VideoFormatMMap.insert (String2VideoFormatPair ("uhd5994",		NTV2_FORMAT_4x1920x1080p_5994));
			gString2VideoFormatMMap.insert (String2VideoFormatPair ("uhd5994",		NTV2_FORMAT_3840x2160p_5994));
			gString2VideoFormatMMap.insert (String2VideoFormatPair ("uhd60",		NTV2_FORMAT_4x1920x1080p_6000));
			gString2VideoFormatMMap.insert (String2VideoFormatPair ("uhd60",		NTV2_FORMAT_3840x2160p_6000));

			gString2VideoFormatMMap.insert (String2VideoFormatPair ("4k",			NTV2_FORMAT_4x2048x1080p_6000));
			gString2VideoFormatMMap.insert (String2VideoFormatPair ("4k",			NTV2_FORMAT_4096x2160p_6000));
			gString2VideoFormatMMap.insert (String2VideoFormatPair ("4k2398",		NTV2_FORMAT_4x2048x1080p_2398));
			gString2VideoFormatMMap.insert (String2VideoFormatPair ("4k2398",		NTV2_FORMAT_4096x2160p_2398));
			gString2VideoFormatMMap.insert (String2VideoFormatPair ("4k24",			NTV2_FORMAT_4x2048x1080p_2400));
			gString2VideoFormatMMap.insert (String2VideoFormatPair ("4k24",			NTV2_FORMAT_4096x2160p_2400));
			gString2VideoFormatMMap.insert (String2VideoFormatPair ("4k25",			NTV2_FORMAT_4x2048x1080p_2500));
			gString2VideoFormatMMap.insert (String2VideoFormatPair ("4k25",			NTV2_FORMAT_4096x2160p_2500));
			gString2VideoFormatMMap.insert (String2VideoFormatPair ("4k4795",		NTV2_FORMAT_4x2048x1080p_4795));
			gString2VideoFormatMMap.insert (String2VideoFormatPair ("4k4795",		NTV2_FORMAT_4096x2160p_4795));
			gString2VideoFormatMMap.insert (String2VideoFormatPair ("4k48",			NTV2_FORMAT_4x2048x1080p_4800));
			gString2VideoFormatMMap.insert (String2VideoFormatPair ("4k48",			NTV2_FORMAT_4096x2160p_4800));
			gString2VideoFormatMMap.insert (String2VideoFormatPair ("4k50",			NTV2_FORMAT_4x2048x1080p_5000));
			gString2VideoFormatMMap.insert (String2VideoFormatPair ("4k50",			NTV2_FORMAT_4096x2160p_5000));
			gString2VideoFormatMMap.insert (String2VideoFormatPair ("4k5994",		NTV2_FORMAT_4x2048x1080p_5994));
			gString2VideoFormatMMap.insert (String2VideoFormatPair ("4k5994",		NTV2_FORMAT_4096x2160p_5994));
			gString2VideoFormatMMap.insert (String2VideoFormatPair ("4k60",			NTV2_FORMAT_4x2048x1080p_6000));
			gString2VideoFormatMMap.insert (String2VideoFormatPair ("4k60",			NTV2_FORMAT_4096x2160p_6000));
			gString2VideoFormatMMap.insert (String2VideoFormatPair ("4k11988",		NTV2_FORMAT_4x2048x1080p_11988));
			gString2VideoFormatMMap.insert (String2VideoFormatPair ("4k11988",		NTV2_FORMAT_4096x2160p_11988));
			gString2VideoFormatMMap.insert (String2VideoFormatPair ("4k120",		NTV2_FORMAT_4x2048x1080p_12000));
			gString2VideoFormatMMap.insert (String2VideoFormatPair ("4k120",		NTV2_FORMAT_4096x2160p_12000));

			gString2VideoFormatMMap.insert (String2VideoFormatPair ("uhd2",			NTV2_FORMAT_4x3840x2160p_2398));
			gString2VideoFormatMMap.insert (String2VideoFormatPair ("uhd22398",		NTV2_FORMAT_4x3840x2160p_2398));
			gString2VideoFormatMMap.insert (String2VideoFormatPair ("uhd224",		NTV2_FORMAT_4x3840x2160p_2400));
			gString2VideoFormatMMap.insert (String2VideoFormatPair ("uhd225",		NTV2_FORMAT_4x3840x2160p_2500));
			gString2VideoFormatMMap.insert (String2VideoFormatPair ("uhd250",		NTV2_FORMAT_4x3840x2160p_5000));
			gString2VideoFormatMMap.insert (String2VideoFormatPair ("uhd25994",		NTV2_FORMAT_4x3840x2160p_5994));
			gString2VideoFormatMMap.insert (String2VideoFormatPair ("uhd260",		NTV2_FORMAT_4x3840x2160p_6000));

			gString2VideoFormatMMap.insert (String2VideoFormatPair ("8k",			NTV2_FORMAT_4x4096x2160p_6000));
			gString2VideoFormatMMap.insert (String2VideoFormatPair ("8k2398",		NTV2_FORMAT_4x4096x2160p_2398));
			gString2VideoFormatMMap.insert (String2VideoFormatPair ("8k24",			NTV2_FORMAT_4x4096x2160p_2400));
			gString2VideoFormatMMap.insert (String2VideoFormatPair ("8k25",			NTV2_FORMAT_4x4096x2160p_2500));
			gString2VideoFormatMMap.insert (String2VideoFormatPair ("8k4795",		NTV2_FORMAT_4x4096x2160p_4795));
			gString2VideoFormatMMap.insert (String2VideoFormatPair ("8k48",			NTV2_FORMAT_4x4096x2160p_4800));
			gString2VideoFormatMMap.insert (String2VideoFormatPair ("8k50",			NTV2_FORMAT_4x4096x2160p_5000));
			gString2VideoFormatMMap.insert (String2VideoFormatPair ("8k5994",		NTV2_FORMAT_4x4096x2160p_5994));
			gString2VideoFormatMMap.insert (String2VideoFormatPair ("8k60",			NTV2_FORMAT_4x4096x2160p_6000));
			
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
				gString2AudioSystemMap.insert (String2AudioSystemPair(ULWordToString (ndx + 1), NTV2AudioSystem(ndx)));

			//	VANC Modes...
			for (NTV2VANCMode vm(NTV2_VANCMODE_OFF);  vm < NTV2_VANCMODE_INVALID;  vm = NTV2VANCMode(vm+1))
			{
				gString2VANCModeMap.insert (String2VANCModePair(aja::to_string(vm), vm));
//				gString2VANCModeMap.insert (String2VANCModePair(::NTV2VANCModeToString(vm, false), vm));
				gString2VANCModeMap.insert (String2VANCModePair(::NTV2VANCModeToString(vm, true), vm));
			}

			//	Input Sources...
			::NTV2DeviceGetSupportedInputSources(DEVICE_ID_INVALID, gInputSources);
			::NTV2DeviceGetSupportedInputSources(DEVICE_ID_INVALID, gInputSourcesSDI, NTV2_IOKINDS_SDI);
			::NTV2DeviceGetSupportedInputSources(DEVICE_ID_INVALID, gInputSourcesHDMI, NTV2_IOKINDS_HDMI);
			::NTV2DeviceGetSupportedInputSources(DEVICE_ID_INVALID, gInputSourcesAnalog, NTV2_IOKINDS_ANALOG);
			for (NTV2InputSourceSetConstIter it(gInputSources.begin());  it != gInputSources.end();  ++it)
			{
				const NTV2InputSource src(*it);
				string sSmall(::NTV2InputSourceToString(src, true)), sBig(::NTV2InputSourceToString(src, false));
				gString2InputSourceMap.insert(String2InputSourcePair(sBig, src));
				gString2InputSourceMap.insert(String2InputSourcePair(sSmall, src));
				gString2InputSourceMap.insert(String2InputSourcePair(CNTV2DemoCommon::ToLower(sSmall), src));
				if (NTV2_INPUT_SOURCE_IS_SDI(src))
				{
					sSmall.erase(0, 3);	//	Erase first 3 chars to leave just the number (e.g. "SDI3" becomes "3")
					gString2InputSourceMap.insert(String2InputSourcePair(sSmall, src));
				}
			}	//	for each input source
			gString2InputSourceMap.insert(String2InputSourcePair(string("hdmi"),NTV2_INPUTSOURCE_HDMI1));

			//	Output Destinations...
			::NTV2DeviceGetSupportedOutputDests (DEVICE_ID_INVALID, gOutputDestinations);
			for (NTV2OutputDestinationsConstIter it(gOutputDestinations.begin());  it != gOutputDestinations.end();  ++it)
			{
				const NTV2OutputDest dst(*it);
				string sSmall(::NTV2OutputDestinationToString(dst, true)), sBig(::NTV2OutputDestinationToString(dst, false));
				gString2OutputDestMap.insert(String2OutputDestPair(sBig, dst));
				gString2OutputDestMap.insert(String2OutputDestPair(sSmall, dst));
				gString2OutputDestMap.insert(String2OutputDestPair(CNTV2DemoCommon::ToLower(sSmall), dst));
				if (NTV2_OUTPUT_DEST_IS_SDI(dst))
				{
					const string str(aja::to_string(UWord(::NTV2OutputDestinationToChannel(dst)+1)));
					gString2OutputDestMap.insert(String2OutputDestPair(str, dst));
				}
			}	//	for each output dest
			gString2OutputDestMap.insert(String2OutputDestPair(string("hdmi1"),NTV2_OUTPUTDESTINATION_HDMI));

			//	TCIndexes...
			for (uint16_t ndx (0);  ndx < NTV2_MAX_NUM_TIMECODE_INDEXES;  ndx++)
			{
				const NTV2TCIndex	tcIndex	(NTV2TCIndex(ndx+0));
				if (tcIndex == NTV2_TCINDEX_DEFAULT)
					continue;	//	Skip NTV2_TCINDEX_DEFAULT
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


NTV2_RP188 NTV2FrameData::Timecode (const NTV2TCIndex inTCNdx) const
{
	NTV2TimeCodesConstIter it(fTimecodes.find(inTCNdx));
	if (it != fTimecodes.end())
		return it->second;
	return NTV2_RP188();
}

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
	CNTV2Card device;
	const string deviceSpec	(inDeviceSpec.empty() ? "0" : inDeviceSpec);
	if (! CNTV2DeviceScanner::GetFirstDeviceFromArgument (deviceSpec, device))
	{
		if (deviceSpec != "LIST" && deviceSpec != "list" && deviceSpec != "?")
			cerr << "## ERROR: Failed to open device spec '" << deviceSpec << "'" << endl;
		return false;
	}
	return true;
}


static string DeviceFilterString (const NTV2DeviceKinds inKinds)
{
	if (inKinds == NTV2_DEVICEKIND_ALL)
		return "any device";
	else if (inKinds == NTV2_DEVICEKIND_NONE)
		return "no device";

	NTV2StringList strs;
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
	return aja::join(strs, " | ");
}


NTV2VideoFormatSet CNTV2DemoCommon::GetSupportedVideoFormats (const NTV2VideoFormatKinds inKinds)
{
	if ((inKinds & VIDEO_FORMATS_ALL) == VIDEO_FORMATS_ALL)
		return gAllFormats;

	NTV2VideoFormatSet result;
	if (inKinds & VIDEO_FORMATS_SDHD)
		result += gSDHDFormats;
	if (inKinds & VIDEO_FORMATS_4KUHD)
		result += g4KFormats;
	if (inKinds & VIDEO_FORMATS_8KUHD2)
		result += g8KFormats;
	return result;
}


string CNTV2DemoCommon::GetVideoFormatStrings (const NTV2VideoFormatKinds inKinds, const string inDevSpec)
{
	const NTV2VideoFormatSet & formatSet(GetSupportedVideoFormats(inKinds));
	ostringstream oss;
	CNTV2Card dev;
	if (!inDevSpec.empty())
		dev.Open(inDevSpec);

	oss	<< setw(25) << left << "Supported Video Format"		<< "\t" << setw(16) << left << "Legal -v Values" << endl
		<< setw(25) << left << "------------------------"	<< "\t" << setw(16) << left << "----------------" << endl;
	for (NTV2VideoFormatSetConstIter iter(formatSet.begin());  iter != formatSet.end();  ++iter)
	{	const NTV2VideoFormat vf(*iter);
		const string vfName (::NTV2VideoFormatToString(vf, true));
		if (vfName == "Unknown")
			continue;
		NTV2StringList vfNames;
		for (String2VideoFormatMMapCI it(gString2VideoFormatMMap.begin());  it != gString2VideoFormatMMap.end();  ++it)
			if (vf == it->second)
			{
				if (!inDevSpec.empty()  &&  dev.IsOpen()  &&  !dev.features().CanDoVideoFormat(vf))
					continue;
				vfNames.push_back(it->first);
			}
		if (!vfNames.empty())
			oss << setw(25) << left << vfName << "\t" << aja::join(vfNames,", ") << endl;
	}
	return oss.str();
}


NTV2VideoFormat CNTV2DemoCommon::GetVideoFormatFromString (const string & inStr, const NTV2VideoFormatKinds inKinds, const string & inDevSpec)
{
	String2VideoFormatMMapCI iter (gString2VideoFormatMMap.find(inStr));
	if (iter == gString2VideoFormatMMap.end())
		return NTV2_FORMAT_UNKNOWN;

	CNTV2Card dev;
	if (!inDevSpec.empty())
		dev.Open(inDevSpec);

	//	If a device was specifed, look for the first name-matching format it supports...
	NTV2VideoFormat vf(iter->second);
	while (dev.IsOpen()  &&  !dev.features().CanDoVideoFormat(vf))
	{
		if (++iter == gString2VideoFormatMMap.end())
			return NTV2_FORMAT_UNKNOWN;
		if (inStr != iter->first)
			return NTV2_FORMAT_UNKNOWN;
		vf = iter->second;
	}
	if ((inKinds & VIDEO_FORMATS_ALL) == VIDEO_FORMATS_ALL)
		return vf;
	if (inKinds & VIDEO_FORMATS_4KUHD  &&  NTV2_IS_4K_VIDEO_FORMAT(vf))
		return vf;
	if (inKinds & VIDEO_FORMATS_8KUHD2  &&  NTV2_IS_QUAD_QUAD_FORMAT(vf))
		return vf;
	if (inKinds & VIDEO_FORMATS_SDHD  &&  !NTV2_IS_4K_VIDEO_FORMAT(vf))
		return vf;
	return NTV2_FORMAT_UNKNOWN;
}


NTV2FrameBufferFormatSet CNTV2DemoCommon::GetSupportedPixelFormats (const NTV2PixelFormatKinds inKinds)
{
	if (inKinds == PIXEL_FORMATS_ALL)
		return gPixelFormats;

	NTV2FrameBufferFormatSet result;
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


string CNTV2DemoCommon::GetPixelFormatStrings (const NTV2PixelFormatKinds inKinds, const string inDevSpec)
{
	const NTV2FrameBufferFormatSet & formatSet (GetSupportedPixelFormats(inKinds));
	CNTV2Card dev;
	ostringstream oss;

	if (!inDevSpec.empty())
		dev.Open(inDevSpec);

	oss << setw(34) << left << "Frame Buffer Format"				<< "\t" << setw(32) << left << "Legal -p Values" << endl
		<< setw(34) << left << "----------------------------------"	<< "\t" << setw(32) << left << "--------------------------------" << endl;
	for (NTV2FrameBufferFormatSetConstIter iter(formatSet.begin());  iter != formatSet.end();  ++iter)
	{
		const NTV2PixelFormat pf(*iter);
		const string pfName (::NTV2FrameBufferFormatToString (pf, true));
		if (pfName.empty())
			continue;
		NTV2StringList pfNames;
		for (String2PixelFormatMapConstIter it(gString2PixelFormatMap.begin());  it != gString2PixelFormatMap.end();  ++it)
			if (pf == it->second)
			{
				if (!inDevSpec.empty()  &&  dev.IsOpen()  &&  !dev.features().CanDoFrameBufferFormat(pf))
					continue;
				pfNames.push_back(it->first);
			}
		if (!pfNames.empty())
			oss << setw(35) << left << pfName << "\t" << aja::join(pfNames, ", ") << endl;
	}
	return oss.str();
}


NTV2PixelFormat CNTV2DemoCommon::GetPixelFormatFromString (const string & inStr, const NTV2PixelFormatKinds inKinds, const string inDevSpec)
{
	String2PixelFormatMapConstIter iter (gString2PixelFormatMap.find(inStr));
	if (iter == gString2PixelFormatMap.end())
		return NTV2_FBF_INVALID;

	CNTV2Card dev;
	if (!inDevSpec.empty())
		dev.Open(inDevSpec);

	//	If a device was specifed, look for the first name-matching format it supports...
	NTV2PixelFormat pf(iter->second);
	while (dev.IsOpen()  &&  !dev.features().CanDoPixelFormat(pf))
	{
		if (++iter == gString2PixelFormatMap.end())
			return NTV2_FBF_INVALID;
		if (inStr != iter->first)
			return NTV2_FBF_INVALID;
		pf = iter->second;
	}
	if ((inKinds & PIXEL_FORMATS_ALL) == PIXEL_FORMATS_ALL)
		return pf;
	if (inKinds & PIXEL_FORMATS_RGB  &&  NTV2_IS_FBF_RGB(pf))
		return pf;
	if (inKinds & PIXEL_FORMATS_PLANAR  &&  NTV2_IS_FBF_PLANAR(pf))
		return pf;
	if (inKinds & PIXEL_FORMATS_RAW  &&  !NTV2_FBF_IS_RAW(pf))
		return pf;
	if (inKinds & PIXEL_FORMATS_PACKED  &&  !NTV2_IS_FBF_PRORES(pf))
		return pf;
	if (inKinds & PIXEL_FORMATS_ALPHA  &&  !NTV2_FBF_HAS_ALPHA(pf))
		return pf;
	return NTV2_FBF_INVALID;
}


const NTV2InputSourceSet CNTV2DemoCommon::GetSupportedInputSources (const NTV2IOKinds inKinds)
{
	if (inKinds == NTV2_IOKINDS_ALL)
		return gInputSources;

	NTV2InputSourceSet result;
	if (inKinds & NTV2_IOKINDS_SDI)
		result += gInputSourcesSDI;
	if (inKinds & NTV2_IOKINDS_HDMI)
		result += gInputSourcesHDMI;
	if (inKinds & NTV2_IOKINDS_ANALOG)
		result += gInputSourcesAnalog;
	return result;
}


string CNTV2DemoCommon::GetInputSourceStrings (const NTV2IOKinds inKinds,  const string inDevSpec)
{
	const NTV2InputSourceSet & sourceSet (GetSupportedInputSources(inKinds));
	CNTV2Card dev;
	ostringstream oss;

	if (!inDevSpec.empty())
		dev.Open(inDevSpec);

	oss	<< setw (25) << left << "Input Source"				<< "\t" << setw (16) << left << "Legal -i Values" << endl
		<< setw (25) << left << "------------------------"	<< "\t" << setw (16) << left << "----------------" << endl;
	for (NTV2InputSourceSetConstIter iter(sourceSet.begin());  iter != sourceSet.end();  ++iter)
	{
		const NTV2InputSource src(*iter);
		string srcName (::NTV2InputSourceToString(src));
		if (srcName.empty())
			continue;
		NTV2StringList srcNames;
		for (String2InputSourceMapConstIter it(gString2InputSourceMap.begin());  it != gString2InputSourceMap.end();  ++it)
			if (src == it->second)
			{
				if (!inDevSpec.empty()  &&  dev.IsOpen()  &&  !dev.features().CanDoInputSource(src))
					continue;
				srcNames.push_back(it->first);
			}
		if (!srcNames.empty())
			oss << setw(25) << left << srcName << "\t" << aja::join(srcNames, ", ") << endl;
	}
	return oss.str ();
}


NTV2InputSource CNTV2DemoCommon::GetInputSourceFromString (const string & inStr, const NTV2IOKinds inKinds, const string inDevSpec)
{
	String2InputSourceMapConstIter iter (gString2InputSourceMap.find(inStr));
	if (iter == gString2InputSourceMap.end())
		return NTV2_INPUTSOURCE_INVALID;

	CNTV2Card dev;
	if (!inDevSpec.empty())
		dev.Open(inDevSpec);

	//	If a device was specifed, look for the first name-matching format it supports...
	NTV2InputSource src(iter->second);
	while (dev.IsOpen()  &&  !dev.features().CanDoInputSource(src))
	{
		if (++iter == gString2InputSourceMap.end())
			return NTV2_INPUTSOURCE_INVALID;
		if (inStr != iter->first)
			return NTV2_INPUTSOURCE_INVALID;
		src = iter->second;
	}
	if ((inKinds & NTV2_IOKINDS_ALL) == NTV2_IOKINDS_ALL)
		return src;
	if (inKinds & NTV2_IOKINDS_SDI  &&  NTV2_INPUT_SOURCE_IS_SDI(src))
		return src;
	if (inKinds & NTV2_IOKINDS_HDMI  &&  NTV2_INPUT_SOURCE_IS_HDMI(src))
		return src;
	if (inKinds & NTV2_IOKINDS_ANALOG  &&  NTV2_INPUT_SOURCE_IS_ANALOG(src))
		return src;
	return NTV2_INPUTSOURCE_INVALID;
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
		for (String2OutputDestMapConstIter it(gString2OutputDestMap.begin());  it != gString2OutputDestMap.end();  ++it)
			if (*iter == it->second)
			{
				oss << setw(25) << left << destName << "\t" << setw(16) << left << it->first;
				if (!inDeviceSpecifier.empty()  &&  theDevice.IsOpen()  &&  !theDevice.features().CanDoOutputDestination(*iter))
					oss << "\t## Incompatible with " << theDevice.GetDisplayName();
				oss << endl;
				destName.clear();
			}
		oss << endl;
	}
	return oss.str();
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
											const string inDevSpec,
											const bool inIsInputOnly)
{
	const NTV2TCIndexes & tcIndexes (GetSupportedTCIndexes(inKinds));
	CNTV2Card dev;
	ostringstream oss;

	if (!inDevSpec.empty())
		dev.Open(inDevSpec);

	oss	<< setw(25) << left << "Timecode Index"				<< "\t" << setw(16) << left << "Legal Values" << endl
		<< setw(25) << left << "------------------------"	<< "\t" << setw(16) << left << "----------------" << endl;
	for (NTV2TCIndexesConstIter iter (tcIndexes.begin());  iter != tcIndexes.end();  ++iter)
	{
		const NTV2TCIndex tcNdx(*iter);
		const string tcNdxName (::NTV2TCIndexToString(tcNdx));
		if (tcNdxName.empty())
			continue;
		NTV2StringList tcNdxNames;
		for (String2TCIndexMapConstIter it (gString2TCIndexMap.begin());  it != gString2TCIndexMap.end();  ++it)
			if (tcNdx == it->second)
			{
				if (!inDevSpec.empty()  &&  dev.IsOpen())
					if (!(inIsInputOnly ? dev.features().CanDoInputTCIndex(tcNdx) : dev.features().CanDoOutputTCIndex(tcNdx)))
						continue;
				tcNdxNames.push_back(it->first);
			}
		if (!tcNdxNames.empty())
			oss << setw(25) << left << tcNdxName << "\t" << aja::join(tcNdxNames, ", ") << endl;
	}
	return oss.str();
}


NTV2TCIndex CNTV2DemoCommon::GetTCIndexFromString (const string & inStr, const NTV2TCIndexKinds inKinds, const string inDevSpec)
{
	String2TCIndexMapConstIter iter (gString2TCIndexMap.find(inStr));
	if (iter == gString2TCIndexMap.end())
		return NTV2_TCINDEX_INVALID;

	CNTV2Card dev;
	if (!inDevSpec.empty())
		dev.Open(inDevSpec);

	//	If a device was specifed, look for the first name-matching format it supports...
	NTV2TCIndex tcNdx(iter->second);
	NTV2InputSource tcInpSrc (::NTV2TimecodeIndexToInputSource(tcNdx));
	while (dev.IsOpen()  &&  !dev.features().CanDoInputSource(tcInpSrc))
	{
		if (++iter == gString2TCIndexMap.end())
			return NTV2_TCINDEX_INVALID;
		if (inStr != iter->first)
			return NTV2_TCINDEX_INVALID;
		tcNdx = iter->second;
		tcInpSrc = ::NTV2TimecodeIndexToInputSource(tcNdx);
	}
	if ((inKinds & TC_INDEXES_ALL) == TC_INDEXES_ALL)
		return tcNdx;
	if (inKinds & TC_INDEXES_SDI  &&  (NTV2_IS_ATC_VITC1_TIMECODE_INDEX(tcNdx) || NTV2_IS_ATC_VITC2_TIMECODE_INDEX(tcNdx) || NTV2_IS_ATC_LTC_TIMECODE_INDEX(tcNdx)))
		return tcNdx;
	if (inKinds & TC_INDEXES_ANALOG  &&  NTV2_IS_ANALOG_TIMECODE_INDEX(tcNdx))
		return tcNdx;
	if (inKinds & TC_INDEXES_ATCLTC  &&  NTV2_IS_ATC_LTC_TIMECODE_INDEX(tcNdx))
		return tcNdx;
	if (inKinds & TC_INDEXES_VITC1  &&  NTV2_IS_ATC_VITC1_TIMECODE_INDEX(tcNdx))
		return tcNdx;
	if (inKinds & TC_INDEXES_VITC2  &&  NTV2_IS_ATC_VITC2_TIMECODE_INDEX(tcNdx))
		return tcNdx;
	return NTV2_TCINDEX_INVALID;
}


string CNTV2DemoCommon::GetAudioSystemStrings (const string inDeviceSpecifier)
{
	CNTV2Card		device;
	string			displayName;
	ostringstream	oss;

	if (!inDeviceSpecifier.empty())
	{
		CNTV2DeviceScanner::GetFirstDeviceFromArgument (inDeviceSpecifier, device);
		if (device.IsOpen())
			displayName = device.GetDisplayName();
	}

	const UWord numAudioSystems	(device.features().GetNumAudioSystems());
	oss << setw(12) << left << "Audio System"	<< endl
		<< setw(12) << left << "------------"	<< endl;
	for (UWord ndx(0);  ndx < 8;  ndx++)
	{
		oss << setw(12) << left << (ndx+1);
		if (!displayName.empty()  &&  ndx >= numAudioSystems)
			oss << "\t## Incompatible with " << displayName;
		oss << endl;
	}
	return oss.str();
}


NTV2AudioSystem CNTV2DemoCommon::GetAudioSystemFromString (const string & inStr)
{
	String2AudioSystemMapConstIter iter(gString2AudioSystemMap.find(inStr));
	return iter != gString2AudioSystemMap.end()  ?  iter->second  :  NTV2_AUDIOSYSTEM_INVALID;
}

string CNTV2DemoCommon::GetVANCModeStrings (void)
{
	typedef map<string,string>	NTV2StringMap;
	NTV2StringSet keys;
	for (String2VANCModeMapConstIter it(gString2VANCModeMap.begin());  it != gString2VANCModeMap.end();  ++it)
	{
		const string val(aja::to_string(it->second));
		if (keys.find(val) == keys.end())
			keys.insert(val);
	}

	NTV2StringMap legals;
	for (NTV2StringSet::const_iterator kit(keys.begin());  kit != keys.end();  ++kit)
	{
		NTV2VANCMode officialVM(NTV2VANCMode(aja::stoul(*kit)));
		NTV2StringList legalValues;
		for (String2VANCModeMapConstIter it(gString2VANCModeMap.begin());  it != gString2VANCModeMap.end();  ++it)
			if (it->second == officialVM)
				legalValues.push_back(it->first);
		legals[aja::to_string(officialVM)] = aja::join(legalValues, ", ");
	}

	ostringstream oss;
	oss	<< setw(12) << left << "VANC Mode" << "\t" << setw(32) << left << "Legal --vanc Values             " << endl
		<< setw(12) << left << "---------" << "\t" << setw(32) << left << "--------------------------------" << endl;
	for (NTV2StringMap::const_iterator it(legals.begin());  it != legals.end();  ++it)
		oss << setw(12) << left << it->first << "\t" << setw(32) << left << it->second << endl;
	return oss.str();
}


NTV2VANCMode CNTV2DemoCommon::GetVANCModeFromString (const string & inStr)
{
	String2VANCModeMapConstIter iter(gString2VANCModeMap.find(inStr));
	return iter != gString2VANCModeMap.end()  ?  iter->second  :  NTV2_VANCMODE_INVALID;
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


AJA_PixelFormat CNTV2DemoCommon::GetAJAPixelFormat (const NTV2PixelFormat inFormat)
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

NTV2ChannelList CNTV2DemoCommon::GetTSIMuxesForFrameStore (CNTV2Card & inDevice, const NTV2Channel in1stFrameStore, const UWord inCount)
{
	UWord totFrameStores(inDevice.features().GetNumFrameStores());
	UWord totTSIMuxers(inDevice.features().GetNumTSIMuxers());
	UWord firstFramestoreIndex = UWord(::GetIndexForNTV2Channel(in1stFrameStore));
	UWord tsiMux(firstFramestoreIndex);
	NTV2ChannelList result;
	if (totFrameStores > totTSIMuxers)
		tsiMux = firstFramestoreIndex/2;
	else if (totFrameStores < totTSIMuxers)
		tsiMux = firstFramestoreIndex*2;
	for (UWord num(0);  num < inCount;  num++)
		result.push_back(NTV2Channel(tsiMux + num));
	return result;
}


bool CNTV2DemoCommon::GetInputRouting (	NTV2XptConnections & conns,
										const CaptureConfig & inConfig,
										const bool isInputRGB)
{
	const bool				isFrameRGB	(::IsRGBFormat(inConfig.fPixelFormat));
	const NTV2InputXptID	fbIXpt		(::GetFrameBufferInputXptFromChannel(inConfig.fInputChannel));
	const NTV2OutputXptID	inputOXpt	(::GetInputSourceOutputXpt(inConfig.fInputSource, false, isInputRGB));
	const NTV2InputXptID	cscVidIXpt	(::GetCSCInputXptFromChannel(inConfig.fInputChannel));
	NTV2OutputXptID			cscOXpt		(::GetCSCOutputXptFromChannel(inConfig.fInputChannel, /*key?*/false, /*RGB?*/isFrameRGB));

	conns.clear();
	if (isInputRGB && !isFrameRGB)
	{
		conns.insert(NTV2Connection(fbIXpt,		cscOXpt));		//	FB <== CSC
		conns.insert(NTV2Connection(cscVidIXpt,	inputOXpt));	//	CSC <== SDIIn/HDMIin
	}
	else if (!isInputRGB && isFrameRGB)
	{
		conns.insert(NTV2Connection(fbIXpt,		cscOXpt));		//	FB <== CSC
		conns.insert(NTV2Connection(cscVidIXpt,	inputOXpt));	//	CSC <== SDIIn/HDMIIn
	}
	else
		conns.insert(NTV2Connection(fbIXpt,		inputOXpt));	//	FB <== SDIIn/HDMIin

	return !conns.empty();

}	//	GetRoutingCapture


bool CNTV2DemoCommon::GetInputRouting4K (NTV2XptConnections & conns,
										const CaptureConfig & inConfig,
										const NTV2DeviceID devID,
										const bool isInputRGB)
{
	UWord sdi(0), mux(0), csc(0), fb(0), path(0);
	NTV2InputXptID in (NTV2_INPUT_CROSSPOINT_INVALID);
	NTV2OutputXptID out (NTV2_OUTPUT_CROSSPOINT_INVALID);
	const bool	isFrameRGB (::IsRGBFormat(inConfig.fPixelFormat));
	conns.clear();
	if (NTV2_INPUT_SOURCE_IS_HDMI(inConfig.fInputSource))
	{	//	HDMI
		if (inConfig.fInputChannel == NTV2_CHANNEL1)
		{	//	HDMI CH1234
			if (isInputRGB == isFrameRGB)
			{	//	HDMI CH1234 RGB SIGNAL AND RGB FBF  OR  YUV SIGNAL AND YUV FBF
				for (path = 0;  path < 4;  path++)
				{	//	MUX <== HDMIIn
					in = ::GetTSIMuxInputXptFromChannel(NTV2Channel(mux+path/2), /*LinkB*/path & 1);
					out = ::GetInputSourceOutputXpt(inConfig.fInputSource, /*DS2*/false, isInputRGB, /*quadrant*/path);
					conns.insert(NTV2Connection(in, out));
					//	FB <== MUX
					in = ::GetFrameBufferInputXptFromChannel(NTV2Channel(fb+path/2), /*Binput*/path & 1);
					out = ::GetTSIMuxOutputXptFromChannel(NTV2Channel(mux+path/2), /*LinkB*/path & 1, /*RGB*/isInputRGB);
					conns.insert(NTV2Connection(in, out));
				}
			}	//	HDMI CH1234 RGB SIGNAL AND RGB FBF
			else if (isInputRGB && !isFrameRGB)
			{	//	HDMI CH1234 RGB SIGNAL AND YUV FBF
				for (path = 0;  path < 4;  path++)
				{
					//	CSC <== HDMIIn
					in = ::GetCSCInputXptFromChannel(NTV2Channel(csc+path));
					out = ::GetInputSourceOutputXpt(inConfig.fInputSource, /*DS2*/false, isInputRGB, /*quadrant*/path);
					conns.insert(NTV2Connection(in, out));
					//	MUX <== CSC
					in = ::GetTSIMuxInputXptFromChannel(NTV2Channel(mux+path/2), /*LinkB*/path & 1);
					out = ::GetCSCOutputXptFromChannel(NTV2Channel(csc+path), /*key*/false, /*rgb*/isFrameRGB);
					conns.insert(NTV2Connection(in, out));
					//	FB <== MUX
					in = ::GetFrameBufferInputXptFromChannel(NTV2Channel(fb+path/2), /*DS2*/path & 1);
					out = ::GetTSIMuxOutputXptFromChannel(NTV2Channel(mux+path/2), /*LinkB*/path & 1, /*rgb*/isFrameRGB);
					conns.insert(NTV2Connection(in, out));
				}
			}	//	HDMI CH1234 RGB SIGNAL AND YUV FBF
			else	//	!isInputRGB && isFrameRGB
			{	//	HDMI CH1234 YUV SIGNAL AND RGB FBF
				for (path = 0;  path < 4;  path++)
				{
					//	CSC <== HDMIIn
					in = ::GetCSCInputXptFromChannel(NTV2Channel(csc+path));
					out = ::GetInputSourceOutputXpt(inConfig.fInputSource, /*DS2*/false, isInputRGB, /*quadrant*/path);
					conns.insert(NTV2Connection(in, out));
					//	MUX <== CSC
					in = ::GetTSIMuxInputXptFromChannel(NTV2Channel(mux+path/2), /*LinkB*/path & 1);
					out = ::GetCSCOutputXptFromChannel(NTV2Channel(csc+path), /*key*/false, /*rgb*/isFrameRGB);
					conns.insert(NTV2Connection(in, out));
					//	FB <== MUX
					in = ::GetFrameBufferInputXptFromChannel(NTV2Channel(fb+path/2), /*DS2*/path & 1);
					out = ::GetTSIMuxOutputXptFromChannel(NTV2Channel(mux+path/2), /*LinkB*/path & 1, /*rgb*/isFrameRGB);
					conns.insert(NTV2Connection(in, out));
				}
			}	//	HDMI CH1234 YUV SIGNAL AND RGB FBF
		}	//	HDMI CH1234
		else
		{	//	HDMI CH5678
			cerr << "## ERROR: Ch5678 must be for Corvid88, but no HDMI on that device" << endl;
		}	//	HDMI CH5678
	}	//	HDMI
	else
	{	//	SDI
		if (::NTV2DeviceCanDo12gRouting(devID))
		{	//	FB <== SDIIn
			in = ::GetFrameBufferInputXptFromChannel(inConfig.fInputChannel);
			out = ::GetInputSourceOutputXpt(inConfig.fInputSource);
			conns.insert(NTV2Connection(in, out));
		}
		else
		{	//	SDI CH1234 or CH5678
			if (inConfig.fInputChannel != NTV2_CHANNEL1)
				{fb = 4;  sdi = fb;  mux = fb / 2;  csc = fb;}
			if (isFrameRGB)
			{	//	RGB FB
				if (inConfig.fDoTSIRouting)
				{	//	SDI CH1234 RGB TSI
					for (path = 0;  path < 4;  path++)
					{
						//	CSC <== SDIIn
						in = ::GetCSCInputXptFromChannel(NTV2Channel(csc+path));
						out = ::GetInputSourceOutputXpt(::NTV2ChannelToInputSource(NTV2Channel(sdi+path)));
						conns.insert(NTV2Connection(in, out));
						//	MUX <== CSC
						in = ::GetTSIMuxInputXptFromChannel(NTV2Channel(mux+path/2), /*LinkB*/path & 1);
						out = ::GetCSCOutputXptFromChannel(NTV2Channel(csc+path), /*key*/false, /*rgb*/isFrameRGB);
						conns.insert(NTV2Connection(in, out));
						//	FB <== MUX
						in = ::GetFrameBufferInputXptFromChannel(NTV2Channel(fb+path/2), /*DS2*/path & 1);
						out = ::GetTSIMuxOutputXptFromChannel(NTV2Channel(mux+path/2), /*LinkB*/path & 1, /*rgb*/isFrameRGB);
						conns.insert(NTV2Connection(in, out));
					}	//	for each spigot
				}	//	SDI CH1234 RGB TSI
				else
				{	//	SDI CH1234 RGB SQUARES
					for (path = 0;  path < 4;  path++)
					{
						//	CSC <== SDIIn
						in = ::GetCSCInputXptFromChannel(NTV2Channel(csc+path));
						out = ::GetInputSourceOutputXpt(::NTV2ChannelToInputSource(NTV2Channel(sdi+path)));
						conns.insert(NTV2Connection(in, out));
						//	FB <== CSC
						in = ::GetFrameBufferInputXptFromChannel(NTV2Channel(fb+path));
						out = ::GetCSCOutputXptFromChannel(NTV2Channel(csc+path), /*key*/false, /*rgb*/isFrameRGB);
						conns.insert(NTV2Connection(in, out));
					}	//	for each spigot
				}	//	SDI CH1234 RGB SQUARES
			}	//	SDI CH1234 RGB FBF
			else	//	YUV FBF
			{
				if (inConfig.fDoTSIRouting)
				{	//	SDI CH1234 YUV TSI
					for (path = 0;  path < 4;  path++)
					{
						//	MUX <== SDIIn
						in = ::GetTSIMuxInputXptFromChannel(NTV2Channel(mux+path/2), /*LinkB*/path & 1);
						out = ::GetInputSourceOutputXpt(::NTV2ChannelToInputSource(NTV2Channel(sdi+path)));
						conns.insert(NTV2Connection(in, out));
						//	FB <== MUX
						in = ::GetFrameBufferInputXptFromChannel(NTV2Channel(fb+path/2), /*DS2*/path & 1);
						out = ::GetTSIMuxOutputXptFromChannel(NTV2Channel(mux+path/2), /*LinkB*/path & 1, /*rgb*/isFrameRGB);
						conns.insert(NTV2Connection(in, out));
					}	//	for each spigot
				}	//	SDI CH1234 YUV TSI
				else
				{
					for (path = 0;  path < 4;  path++)
					{	//	FB <== SDIIn
						in = ::GetFrameBufferInputXptFromChannel(NTV2Channel(fb+path));
						out = ::GetInputSourceOutputXpt(::NTV2ChannelToInputSource(NTV2Channel(sdi+path)));
						conns.insert(NTV2Connection(in, out));
					}	//	for each path
				}	//	SDI CH1234 YUV SQUARES
			}	//	YUV FBF
		}	//	3G SDI CH1234 or CH5678
	}	//	SDI
	return !conns.empty();
}	//	GetRoutingCapture4K


bool CNTV2DemoCommon::GetInputRouting8K (NTV2XptConnections & conns,
										const CaptureConfig & inConfig,
										const NTV2VideoFormat inVideoFormat,
										const NTV2DeviceID devID,
										const bool isInputRGB)
{	(void)isInputRGB;  (void) devID;
	UWord fb(0), path(0);
	NTV2InputXptID in (NTV2_INPUT_CROSSPOINT_INVALID);
	NTV2OutputXptID out (NTV2_OUTPUT_CROSSPOINT_INVALID);
	const bool	isFrameRGB (::IsRGBFormat(inConfig.fPixelFormat));
	const bool	isQuadQuadHFR (NTV2_IS_QUAD_QUAD_HFR_VIDEO_FORMAT(inVideoFormat));
	conns.clear();
	if (inConfig.fInputChannel % 2)
		return false;   //  Input channel cannot be Ch2/Ch4/Ch6/etc
	if (inConfig.fInputChannel > 3)
		return false;   //  Input channel cannot be Ch5 or greater
	if (::NTV2InputSourceToChannel(inConfig.fInputSource) % 2)
		return false;	//	Input source cannot be SDIIn2/4/6/etc
	if (::NTV2InputSourceToChannel(inConfig.fInputSource) > 3)
		return false;	//	Input source cannot be SDIIn5 or greater

	if (inConfig.fDoTSIRouting)
	{	//	TSI
		if (inConfig.fInputChannel)
			fb = 2;
		for (path = 0;  path < 4;  path++)
			if (isFrameRGB)	//	Uses 2 FBs, 4 SDIs
			{	//	RGB
				//	DLInDS1 <== SDIInDS1
				in = ::GetDLInInputXptFromChannel (NTV2Channel(path), /*B*/false);
				out = ::GetSDIInputOutputXptFromChannel (NTV2Channel(path),	/*DS2*/false);
				conns.insert(NTV2Connection(in, out));
				//	DLInDS2 <== SDIInDS2
				in = ::GetDLInInputXptFromChannel (NTV2Channel(path), /*B*/true);
				out = ::GetSDIInputOutputXptFromChannel (NTV2Channel(path),	/*DS2*/true);
				conns.insert(NTV2Connection(in, out));
				//	FB <== DLIn
				in = ::GetFrameBufferInputXptFromChannel (NTV2Channel(fb+path/2), /*B*/path & 1);
				out = ::GetDLInOutputXptFromChannel (NTV2Channel(path));
				conns.insert(NTV2Connection(in, out));
			}	//	if RGB
			else if (isQuadQuadHFR)	//	Uses 2 FBs, 4 SDIs
			{	//	FB <== SDIIn, FBDS2 <== SDIIn
				in = ::GetFrameBufferInputXptFromChannel (NTV2Channel(fb+path/2), /*DS2?*/path & 1);
				out = ::GetSDIInputOutputXptFromChannel (NTV2Channel(path));
				conns.insert(NTV2Connection(in, out));
			}	//	else if YUV QuadQuad
			else	//	Uses 2 FBs, 2 SDIs
			{	//	FB <== SDIIn, FBDS2 <== SDIInDS2
				in = ::GetFrameBufferInputXptFromChannel (NTV2Channel(fb+path/2), /*DS2?*/path & 1);
				out = ::GetSDIInputOutputXptFromChannel (NTV2Channel(fb+path/2), /*DS2?*/path & 1);
				conns.insert(NTV2Connection(in, out));
			}	//	else YUV non-QuadQuad
	}	//	if TSI
	else
	{	//	Square-division routing
		if (inConfig.fInputChannel)
			return false;   //  Sorry, Ch1 only
		if (inConfig.fInputSource != NTV2_INPUTSOURCE_SDI1)
			return false;   //  Sorry, SDI1 only (1st SDI of 4 links)
		for (path = 0;  path < 4;  path++)	//	4 FBs, 4 SDIs
			if (isFrameRGB)
			{	//	RGB
				//	DLInDS1 <== SDIInDS1
				in = ::GetDLInInputXptFromChannel (NTV2Channel(path), /*B*/false);
				out = ::GetSDIInputOutputXptFromChannel (NTV2Channel(path),	/*DS2*/false);
				conns.insert(NTV2Connection(in, out));
				//	DLInDS2 <== SDIInDS2
				in = ::GetDLInInputXptFromChannel (NTV2Channel(path), /*B*/true);
				out = ::GetSDIInputOutputXptFromChannel (NTV2Channel(path),	/*DS2*/true);
				conns.insert(NTV2Connection(in, out));
				//	FB <== DLIn
				in = ::GetFrameBufferInputXptFromChannel (NTV2Channel(fb+path));
				out = ::GetDLInOutputXptFromChannel (NTV2Channel(path));
				conns.insert(NTV2Connection(in, out));
			}	//	for each path
			else	//	YUV
			{	//	FB <== SDIIn
				in = ::GetFrameBufferInputXptFromChannel (NTV2Channel(fb+path));
				out = ::GetSDIInputOutputXptFromChannel (NTV2Channel(path));
				conns.insert(NTV2Connection(in, out));
			}	//	for each path
	}	//	else Squares
	return !conns.empty();
}   //  GetInputRouting8K


bool CNTV2DemoCommon::ConfigureAudioSystems (CNTV2Card & inDevice, const CaptureConfig & inConfig, const NTV2AudioSystemSet inAudioSystems)
{
    UWord failures(0);
    UWord numAudChannels(inDevice.features().GetMaxAudioChannels());
    for (NTV2AudioSystemSetConstIter it(inAudioSystems.begin());  it != inAudioSystems.end();  ++it)
    {	const NTV2AudioSystem audSys(*it);
		//	Have the audio system capture audio from the designated device input...
		if (!inDevice.SetAudioSystemInputSource (audSys, NTV2_AUDIO_EMBEDDED,
			::NTV2InputSourceToEmbeddedAudioInput(inConfig.fInputSource)))
				failures++;

		//	Configure for max available audio channels, 48KHz, 4MB buffers, and disable loopback...
		if (!inDevice.SetNumberAudioChannels (numAudChannels, audSys))			failures++;
		if (!inDevice.SetAudioRate (NTV2_AUDIO_48K, audSys))					failures++;
		if (!inDevice.SetAudioBufferSize (NTV2_AUDIO_BUFFER_SIZE_4MB, audSys))	failures++;
		if (!inDevice.SetAudioLoopBack(NTV2_AUDIO_LOOPBACK_OFF, audSys))		failures++;
	}
	return !failures;
}


size_t CNTV2DemoCommon::SetDefaultPageSize (void)
{
	const size_t hwPageSizeBytes (NTV2Buffer::HostPageSize());
	const size_t sdkPageSizeBytes (NTV2Buffer::DefaultPageSize());
	if (hwPageSizeBytes != sdkPageSizeBytes)
	{
		if (NTV2Buffer::SetDefaultPageSize(hwPageSizeBytes))
			cerr << "## NOTE:  Page size changed from " << DEC(sdkPageSizeBytes/1024) << "K to " << DEC(hwPageSizeBytes/1024) << "K" << endl;
		else
			cerr << "## WARNING:  Failed to change page size from " << DEC(sdkPageSizeBytes/1024) << "K to " << DEC(hwPageSizeBytes/1024) << "K" << endl;
	}
	return hwPageSizeBytes;
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
	else
	{
		const char * pStr (::poptGetArg(mContext));
		while (pStr)
		{
			mOtherArgs.push_back(string(pStr));	//	Append to file list
			pStr = ::poptGetArg(mContext);
		}	//	for each additional positional argument
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
		//	Dump the gString2VideoFormatMMap map...
		for (String2VideoFormatMMapCI it(gString2VideoFormatMMap.begin());  it != gString2VideoFormatMMap.end();  ++it)
		{
			cout << "'" << it->first << "'\t'" << ::NTV2VideoFormatToString(it->second) << "'\t" << ::NTV2VideoFormatString(it->second) << "\t" << DEC(it->second) << endl;
		}
	}
	cout << endl << endl;
	for (unsigned ndx(0);  !sVFmtDict[ndx].fName.empty();  ndx++)
	{
		const string &			str		(sVFmtDict[ndx].fName);
		const NTV2VideoFormat	vFormat	(sVFmtDict[ndx].fFormat);
		String2VideoFormatMMapCI	it	(gString2VideoFormatMMap.find(str));
		const NTV2VideoFormat	vFormat2	(it != gString2VideoFormatMMap.end() ? it->second : NTV2_FORMAT_UNKNOWN);
		if (vFormat != vFormat2)
			cerr << "'" << str << "': '" << ::NTV2VideoFormatString(vFormat) << "' (" << DEC(vFormat) << ") != '" << ::NTV2VideoFormatString(vFormat2) << "' (" << DEC(vFormat2) << ")" << endl;
		//SHOULD_BE_EQUAL(vFormat, vFormat2);
	}
	return true;
}


//////////////////////////////////////////////


AJALabelValuePairs CaptureConfig::Get (const bool inCompact) const
{
	AJALabelValuePairs result;
	AJASystemInfo::append (result,	"Capture Config");
	AJASystemInfo::append (result,		"Device Specifier",	fDeviceSpec);
	AJASystemInfo::append (result,		"Input Channel",	::NTV2ChannelToString(fInputChannel, inCompact));
	AJASystemInfo::append (result,		"Input Source",		::NTV2InputSourceToString(fInputSource, inCompact));
	AJASystemInfo::append (result,		"Pixel Format",		::NTV2FrameBufferFormatToString(fPixelFormat, inCompact));
	AJASystemInfo::append (result,		"AutoCirc Frames",	fFrames.toString());
	AJASystemInfo::append (result,		"A/B Conversion",	fDoABConversion ? "Y" : "N");
	AJASystemInfo::append (result,		"MultiFormat Mode",	fDoMultiFormat ? "Y" : "N");
	AJASystemInfo::append (result,		"Capture Anc",		fWithAnc ? "Y" : "N");
	AJASystemInfo::append (result,		"Anc Capture File",	fAncDataFilePath.empty() ? "---" : fAncDataFilePath);
	AJASystemInfo::append (result,		"Capture Audio",	fWithAudio ? "Y" : "N");
	AJASystemInfo::append (result,		"Num Audio Links",	aja::to_string(fNumAudioLinks));
	AJASystemInfo::append (result,		"TSI Routing",		fDoTSIRouting ? "Y" : "N");
	return result;
}


std::ostream & operator << (std::ostream & ioStrm,  const CaptureConfig & inObj)
{
	ioStrm	<< AJASystemInfo::ToString(inObj.Get());
	return ioStrm;
}


//////////////////////////////////////////////


AJALabelValuePairs PlayerConfig::Get (const bool inCompact) const
{
	AJALabelValuePairs result;
	AJASystemInfo::append (result,	"NTV2Player Config");
	AJASystemInfo::append (result,		"Device Specifier",		fDeviceSpec);
	AJASystemInfo::append (result,		"Video Format",			::NTV2VideoFormatToString(fVideoFormat));
	AJASystemInfo::append (result,		"Pixel Format",			::NTV2FrameBufferFormatToString(fPixelFormat, inCompact));
	AJASystemInfo::append (result,		"AutoCirc Frames",		fFrames.toString());
	AJASystemInfo::append (result,		"MultiFormat Mode",		fDoMultiFormat ? "Y" : "N");
	AJASystemInfo::append (result,		"VANC Mode",			::NTV2VANCModeToString(fVancMode));
	AJASystemInfo::append (result,		"HDR Anc Type",			::AJAAncDataTypeToString(fTransmitHDRType));
	AJASystemInfo::append (result,		"Output Channel",		::NTV2ChannelToString(fOutputChannel, inCompact));
	AJASystemInfo::append (result,		"Output Connector",		::NTV2OutputDestinationToString(fOutputDest, inCompact));
	AJASystemInfo::append (result,		"Anc Playback File",	fAncDataFilePath.empty() ? "---" : fAncDataFilePath);
	AJASystemInfo::append (result,		"Suppress Audio",		fSuppressAudio ? "Y" : "N");
	AJASystemInfo::append (result,		"Num Audio Links",		aja::to_string(fNumAudioLinks));
	AJASystemInfo::append (result,		"Suppress Video",		fSuppressVideo ? "Y" : "N");
	AJASystemInfo::append (result,		"Embedded Timecode",	fTransmitLTC ? "LTC" : "VITC");
	AJASystemInfo::append (result,		"Level Conversion",		fDoABConversion ? "Y" : "N");
	AJASystemInfo::append (result,		"HDMI Output",			fDoHDMIOutput ? "Yes" : "No");
	AJASystemInfo::append (result,		"RGB-On-SDI",			fDoRGBOnWire ? "Yes" : "No");
	AJASystemInfo::append (result,		"TSI Routing",			fDoTsiRouting ? "Yes" : "No");
	AJASystemInfo::append (result,		"6G/12G Output",		fDoLinkGrouping ? "Yes" : "No");
	return result;
}


std::ostream & operator << (std::ostream & ioStrm,  const PlayerConfig & inObj)
{
	ioStrm	<< AJASystemInfo::ToString(inObj.Get());
	return ioStrm;
}


//////////////////////////////////////////////


AJALabelValuePairs BurnConfig::Get (const bool inCompact) const
{
	AJALabelValuePairs result;
	AJASystemInfo::append(result, "NTV2Burn Config");
	if (fDeviceSpec2.empty())
		AJASystemInfo::append(result, "Device Specifier",	fDeviceSpec);
	else
	{
		AJASystemInfo::append(result, "Input Device",	fDeviceSpec);
		AJASystemInfo::append(result, "Output Device",	fDeviceSpec2);
	}
	AJASystemInfo::append(result, "Input Channel",		::NTV2ChannelToString(fInputChannel, inCompact));
	AJASystemInfo::append(result, "Output Channel",		::NTV2ChannelToString(fOutputChannel, inCompact));
	AJASystemInfo::append(result, "Input Source",		::NTV2InputSourceToString(fInputSource, inCompact));
	if (WithTimecode())
		AJASystemInfo::append(result, "Timecode Source",	::NTV2TCIndexToString(fTimecodeSource, inCompact));
	AJASystemInfo::append(result, "Pixel Format",		::NTV2FrameBufferFormatToString(fPixelFormat, inCompact));
	AJASystemInfo::append(result, "AC Input Frames",	fInputFrames.toString());
	AJASystemInfo::append(result, "AC Output Frames",	fOutputFrames.toString());
	AJASystemInfo::append(result, "Include Video",		WithVideo() ? "Y" : "N");
	AJASystemInfo::append(result, "Include Audio",		WithAudio() ? "Y" : "N");
	AJASystemInfo::append(result, "Include Anc",		WithAnc() ? "Y" : "N");
	AJASystemInfo::append(result, "Include HANC",		WithHanc() ? "Y" : "N");
	AJASystemInfo::append(result, "MultiFormat Mode",	fDoMultiFormat ? "Y" : "N");
	AJASystemInfo::append(result, "Field Mode",			FieldMode() ? "Y" : "N");
	return result;
}
