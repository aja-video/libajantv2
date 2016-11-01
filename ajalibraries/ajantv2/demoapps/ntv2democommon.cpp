/**
	@file		ntv2democommon.cpp
	@brief		Common implementation code used by many of the demo applications.
	@copyright	Copyright (C) 2013-2016 AJA Video Systems, Inc.  All rights reserved.
**/

#include "ntv2democommon.h"
#include "ntv2devicescanner.h"
#include "ntv2devicefeatures.h"
#include "ntv2utils.h"
#include <algorithm>
#include <map>
#include <iomanip>
#if defined (AJAMac) || defined (AJALinux)
	#include <unistd.h>
	#include <termios.h>
#endif

using namespace std;


typedef	map <string, NTV2VideoFormat>			String2VideoFormatMap;
typedef	String2VideoFormatMap::const_iterator	String2VideoFormatMapConstIter;

typedef	map <string, NTV2FrameBufferFormat>		String2PixelFormatMap;
typedef	String2PixelFormatMap::const_iterator	String2PixelFormatMapConstIter;

typedef	map <string, NTV2AudioSystem>			String2AudioSystemMap;
typedef	String2AudioSystemMap::const_iterator	String2AudioSystemMapConstIter;

typedef	map <string, NTV2InputSource>			String2InputSourceMap;
typedef	String2InputSourceMap::const_iterator	String2InputSourceMapConstIter;

static const string				gGlobalMutexName	("com.aja.ntv2.mutex.demo");
static NTV2VideoFormatSet		gAllFormats;
static NTV2VideoFormatSet		gNon4KFormats;
static NTV2VideoFormatSet		g4KFormats;
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
static String2VideoFormatMap	gString2VideoFormatMap;
static String2PixelFormatMap	gString2PixelFormatMap;
static String2AudioSystemMap	gString2AudioSystemMap;
static String2InputSourceMap	gString2InputSourceMap;


class DemoCommonInitializer
{
	public:
		DemoCommonInitializer ()
		{
			typedef	pair <string, NTV2VideoFormat>			String2VideoFormatPair;
			typedef	pair <string, NTV2FrameBufferFormat>	String2PixelFormatPair;
			typedef	pair <string, NTV2AudioSystem>			String2AudioSystemPair;
			typedef	pair <string, NTV2InputSource>			String2InputSourcePair;

			NTV2_ASSERT (gNon4KFormats.empty ());
			for (NTV2VideoFormat legalFormat (NTV2_FORMAT_UNKNOWN);  legalFormat < NTV2_MAX_NUM_VIDEO_FORMATS;  legalFormat = NTV2VideoFormat (legalFormat + 1))
			{
				string	str;
				if (!NTV2_IS_VALID_VIDEO_FORMAT (legalFormat))
					continue;

				if (NTV2_IS_4K_VIDEO_FORMAT (legalFormat))
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
						str.erase (str.find (".00"));		//	...lop off the ".00"

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
			gString2PixelFormatMap.insert (String2PixelFormatPair ("qrez",			NTV2_FBF_8BIT_QREZ));
			gString2PixelFormatMap.insert (String2PixelFormatPair ("qrez8",			NTV2_FBF_8BIT_QREZ));
			gString2PixelFormatMap.insert (String2PixelFormatPair ("hdv",			NTV2_FBF_8BIT_HDV));
			gString2PixelFormatMap.insert (String2PixelFormatPair ("hdv8",			NTV2_FBF_8BIT_HDV));
			gString2PixelFormatMap.insert (String2PixelFormatPair ("rgb24",			NTV2_FBF_24BIT_RGB));
			gString2PixelFormatMap.insert (String2PixelFormatPair ("bgr24",			NTV2_FBF_24BIT_BGR));
			gString2PixelFormatMap.insert (String2PixelFormatPair ("ycbcra10",		NTV2_FBF_10BIT_YCBCRA));
			gString2PixelFormatMap.insert (String2PixelFormatPair ("rgb10dpxle",	NTV2_FBF_10BIT_DPX_LITTLEENDIAN));
			gString2PixelFormatMap.insert (String2PixelFormatPair ("prores",		NTV2_FBF_PRORES));
			gString2PixelFormatMap.insert (String2PixelFormatPair ("ycbcr10prores",	NTV2_FBF_PRORES));
			gString2PixelFormatMap.insert (String2PixelFormatPair ("yuv10prores",	NTV2_FBF_PRORES));
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
			static const NTV2InputSource	gSDISources[]	=	{	NTV2_INPUTSOURCE_SDI1,	NTV2_INPUTSOURCE_SDI2,	NTV2_INPUTSOURCE_SDI3,	NTV2_INPUTSOURCE_SDI4,
																	NTV2_INPUTSOURCE_SDI5,	NTV2_INPUTSOURCE_SDI6,	NTV2_INPUTSOURCE_SDI7,	NTV2_INPUTSOURCE_SDI8,
																	NTV2_INPUTSOURCE_INVALID	};
			for (unsigned ndx (0);  gSDISources[ndx] != NTV2_INPUTSOURCE_INVALID;  ndx++)
			{
				const NTV2InputSource	inputSource	(gSDISources [ndx]);
				gInputSources.insert (inputSource);
				gInputSourcesSDI.insert (inputSource);
				string	str (::NTV2InputSourceToString (inputSource, true));
				while (str.find (" ") != string::npos)
					str.erase (str.find (" "), 1);		//	Remove all spaces
				gString2InputSourceMap.insert (String2InputSourcePair (str, inputSource));
				gString2InputSourceMap.insert (String2InputSourcePair (::NTV2InputSourceToString (inputSource, false), inputSource));
				gString2InputSourceMap.insert (String2InputSourcePair (::NTV2InputSourceToString (inputSource, true), inputSource));
				gString2InputSourceMap.insert (String2InputSourcePair (ULWordToString (ndx + 1), inputSource));
			}	//	for each SDI source
			gInputSources.insert (NTV2_INPUTSOURCE_HDMI);
			gInputSourcesHDMI.insert (NTV2_INPUTSOURCE_HDMI);
			gString2InputSourceMap.insert (String2InputSourcePair (::NTV2InputSourceToString (NTV2_INPUTSOURCE_HDMI, false), NTV2_INPUTSOURCE_HDMI));
			gString2InputSourceMap.insert (String2InputSourcePair (CNTV2DemoCommon::ToLower(::NTV2InputSourceToString (NTV2_INPUTSOURCE_HDMI, true)), NTV2_INPUTSOURCE_HDMI));
			gInputSources.insert (NTV2_INPUTSOURCE_ANALOG);
			gInputSourcesAnalog.insert (NTV2_INPUTSOURCE_ANALOG);
			gString2InputSourceMap.insert (String2InputSourcePair (::NTV2InputSourceToString (NTV2_INPUTSOURCE_ANALOG, false), NTV2_INPUTSOURCE_ANALOG));
			gString2InputSourceMap.insert (String2InputSourcePair (CNTV2DemoCommon::ToLower(::NTV2InputSourceToString (NTV2_INPUTSOURCE_ANALOG, true)), NTV2_INPUTSOURCE_ANALOG));
		}	//	constructor
	private:
		string ULWordToString (const ULWord inNum)
		{
			ostringstream	oss;
			oss << inNum;
			return oss.str ();
		}
};

static const DemoCommonInitializer	gInitializer;


const NTV2VideoFormatSet &	CNTV2DemoCommon::GetSupportedVideoFormats (const NTV2VideoFormatKinds inKinds)
{
	return inKinds == BOTH_VIDEO_FORMATS  ?  gAllFormats  :  (inKinds == UHD_VIDEO_FORMATS ? g4KFormats : gNon4KFormats);
}


string CNTV2DemoCommon::GetVideoFormatStrings (const NTV2VideoFormatKinds inKinds, const string inDeviceSpecifier)
{
	const NTV2VideoFormatSet &	formatSet	(GetSupportedVideoFormats (inKinds));
	ostringstream				oss;
	CNTV2Card					theDevice;
	if (!inDeviceSpecifier.empty ())
		CNTV2DeviceScanner::GetFirstDeviceFromArgument (inDeviceSpecifier, theDevice);

	oss	<< setw (25) << left << "Video Format"				<< "\t" << setw (16) << left << "Legal -v Values" << endl
		<< setw (25) << left << "------------------------"	<< "\t" << setw (16) << left << "----------------" << endl;
	for (NTV2VideoFormatSetConstIter iter (formatSet.begin ());  iter != formatSet.end ();  ++iter)
	{
		string	formatName	(::NTV2VideoFormatToString (*iter));
		for (String2VideoFormatMapConstIter it (gString2VideoFormatMap.begin ());  it != gString2VideoFormatMap.end ();  ++it)
			if (*iter == it->second)
			{
				oss << setw (25) << left << formatName << "\t" << setw (16) << left << it->first;
				if (!inDeviceSpecifier.empty ()  &&  theDevice.IsOpen ()  &&  !::NTV2DeviceCanDoVideoFormat (theDevice.GetDeviceID (), *iter))
					oss << "\t## Incompatible with " << theDevice.GetDisplayName ();
				oss << endl;
				formatName.clear ();
			}
		oss << endl;
	}
	return oss.str ();
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


NTV2VideoFormat CNTV2DemoCommon::GetVideoFormatFromString (const string & inStr, const bool in4K)
{
	String2VideoFormatMapConstIter	iter	(gString2VideoFormatMap.find (inStr));
	if (iter == gString2VideoFormatMap.end ())
		return NTV2_FORMAT_UNKNOWN;
	const NTV2VideoFormat	format	(iter->second);
	if (in4K && NTV2_IS_4K_VIDEO_FORMAT (format))
		return format;
	if (!in4K && !NTV2_IS_4K_VIDEO_FORMAT (format))
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
	if (inKinds == INPUT_SOURCES_ALL)
		return gInputSources;

	NTV2InputSourceSet	result;

	if (inKinds & INPUT_SOURCES_SDI)
		result += gInputSourcesSDI;
	if (inKinds & INPUT_SOURCES_HDMI)
		result += gInputSourcesHDMI;
	if (inKinds & INPUT_SOURCES_ANALOG)
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


string CNTV2DemoCommon::ToLower (const string & inStr)
{
	string	result (inStr);
	std::transform (result.begin (), result.end (), result.begin (), ::tolower);
	return result;
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
		terminalStatus.c_lflag &= ~ICANON;
		terminalStatus.c_lflag &= ~ECHO;
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
		case NTV2_FRAMERATE_1798:		return AJA_FrameRate_1798;
		case NTV2_FRAMERATE_1800:		return AJA_FrameRate_1800;
		case NTV2_FRAMERATE_1898:		return AJA_FrameRate_1898;
		case NTV2_FRAMERATE_1900:		return AJA_FrameRate_1900;
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
		case NTV2_FBF_8BIT_QREZ:				return AJA_PixelFormat_QREZ;
		case NTV2_FBF_8BIT_HDV:					return AJA_PixelFormat_HDV;
		case NTV2_FBF_24BIT_RGB:				return AJA_PixelFormat_RGB8_PACK;
		case NTV2_FBF_24BIT_BGR:				return AJA_PixelFormat_BGR8_PACK;
		case NTV2_FBF_10BIT_YCBCRA:				return AJA_PixelFormat_YCbCrA10;
		case NTV2_FBF_10BIT_DPX_LITTLEENDIAN:	return AJA_PixelFormat_RGB_DPX_LE;
		case NTV2_FBF_48BIT_RGB:				return AJA_PixelFormat_RGB16;
		case NTV2_FBF_PRORES:					return AJA_PixelFormat_PRORES;
		case NTV2_FBF_PRORES_DVCPRO:			return AJA_PixelFormat_PRORES_DVPRO;
		case NTV2_FBF_PRORES_HDV:				return AJA_PixelFormat_PRORES_HDV;
		case NTV2_FBF_10BIT_RGB_PACKED:			return AJA_PixelFormat_RGB10_PACK;

		case NTV2_FBF_10BIT_RAW_RGB:
		case NTV2_FBF_10BIT_RAW_YCBCR:
		case NTV2_FBF_10BIT_ARGB:
		case NTV2_FBF_16BIT_ARGB:
		case NTV2_FBF_8BIT_YCBCR_420PL:
		case NTV2_FBF_8BIT_YCBCR_422PL:
		case NTV2_FBF_10BIT_YCBCR_420PL:
		case NTV2_FBF_10BIT_YCBCR_422PL:		break;

		case NTV2_FBF_UNUSED_23:
		case NTV2_FBF_UNUSED_26:
		case NTV2_FBF_UNUSED_27:
		case NTV2_FBF_NUMFRAMEBUFFERFORMATS:	break;
	}
	return AJA_PixelFormat_Unknown;
}	//	GetAJAPixelFormat


const char * CNTV2DemoCommon::GetGlobalMutexName (void)
{
	return gGlobalMutexName.c_str ();
}
