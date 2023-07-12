/* SPDX-License-Identifier: MIT */
/**
	@file		main.cpp
	@brief		Unittests for the AJA NTV2 Library (using doctest).
	@copyright	(C) 2019-2022 AJA Video Systems, Inc. All rights reserved.
**/
// for doctest usage see: https://github.com/onqtam/doctest/blob/1.1.4/doc/markdown/tutorial.md

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
// need to define this so will work with compilers that don't support thread_local
// ie xcode 6, 7
#define DOCTEST_THREAD_LOCAL
#include "doctest.h"
#include "ntv2bitfile.h"
#include "ntv2card.h"
#include "ntv2debug.h"
#include "ntv2endian.h"
#include "ntv2signalrouter.h"
#include "ntv2routingexpert.h"
#include "ntv2transcode.h"
#include "ntv2utils.h"
#include "ntv2vpid.h"
#include "ntv2version.h"
#include "ntv2testpatterngen.h"
#include "ajabase/system/debug.h"
#include <vector>
#include <algorithm>
#include <iomanip>
#include <iterator>    //      For std::inserter

using namespace std;

static bool gVerboseOutput = false;

#define	LOGERR(__x__)	AJA_sREPORT(AJA_DebugUnit_Testing, AJA_DebugSeverity_Error,		AJAFUNC << ":  " << __x__)
#define	LOGWARN(__x__)	AJA_sREPORT(AJA_DebugUnit_Testing, AJA_DebugSeverity_Warning,	AJAFUNC << ":  " << __x__)
#define	LOGNOTE(__x__)	AJA_sREPORT(AJA_DebugUnit_Testing, AJA_DebugSeverity_Notice,	AJAFUNC << ":  " << __x__)
#define	LOGINFO(__x__)	AJA_sREPORT(AJA_DebugUnit_Testing, AJA_DebugSeverity_Info,		AJAFUNC << ":  " << __x__)
#define	LOGDBG(__x__)	AJA_sREPORT(AJA_DebugUnit_Testing, AJA_DebugSeverity_Debug,		AJAFUNC << ":  " << __x__)

#if 0
template
void filename_marker() {} //this is used to easily just around in a GUI with a symbols list
TEST_SUITE("filename" * doctest::description("functions in streams/common/filename.h")) {

	TEST_CASE("constructor")
	{
	}

} //filename
#endif

typedef	vector<NTV2VANCMode>	NTV2VANCModes;


void ntv2debug_marker() {}
TEST_SUITE("ntv2debug" * doctest::description("ntv2 debug string functions")) {

	TEST_CASE("DeviceIDString")
	{
		// spot check a few
		CHECK(std::string(NTV2DeviceIDString(DEVICE_ID_NOTFOUND)) == "DEVICE_ID_NOTFOUND");
		CHECK(std::string(NTV2DeviceIDString(DEVICE_ID_CORVID1)) == "DEVICE_ID_CORVID1");
		CHECK(std::string(NTV2DeviceIDString(DEVICE_ID_KONALHI)) == "DEVICE_ID_KONALHI");
		CHECK(std::string(NTV2DeviceIDString(DEVICE_ID_KONA3GQUAD)) == "DEVICE_ID_KONA3GQUAD");
		CHECK(std::string(NTV2DeviceIDString(DEVICE_ID_KONA4)) == "DEVICE_ID_KONA4");
		CHECK(std::string(NTV2DeviceIDString(DEVICE_ID_KONA5)) == "DEVICE_ID_KONA5");
		CHECK(std::string(NTV2DeviceIDString(DEVICE_ID_CORVID44)) == "DEVICE_ID_CORVID44");
		CHECK(std::string(NTV2DeviceIDString(DEVICE_ID_CORVID88)) == "DEVICE_ID_CORVID88");
		CHECK(std::string(NTV2DeviceIDString(DEVICE_ID_CORVID44_8KMK)) == "DEVICE_ID_CORVID44_8KMK");
		CHECK(std::string(NTV2DeviceIDString(DEVICE_ID_KONAIP_2110)) == "DEVICE_ID_KONAIP_2110");
		CHECK(std::string(NTV2DeviceIDString(DEVICE_ID_IOIP_2110)) == "DEVICE_ID_IOIP_2110");
	}

	TEST_CASE("DeviceString")
	{
		// spot check a few
		CHECK(std::string(NTV2DeviceString(DEVICE_ID_NOTFOUND)) == "Unknown");
		CHECK(std::string(NTV2DeviceString(DEVICE_ID_CORVID1)) == "Corvid1");
		CHECK(std::string(NTV2DeviceString(DEVICE_ID_KONALHI)) == "KonaLHi");
		CHECK(std::string(NTV2DeviceString(DEVICE_ID_KONA3GQUAD)) == "Kona3GQuad");
		CHECK(std::string(NTV2DeviceString(DEVICE_ID_KONA4)) == "Kona4");
		CHECK(std::string(NTV2DeviceString(DEVICE_ID_KONA5)) == "Kona5");
		CHECK(std::string(NTV2DeviceString(DEVICE_ID_CORVID44)) == "Corvid44");
		CHECK(std::string(NTV2DeviceString(DEVICE_ID_CORVID88)) == "Corvid88");
		CHECK(std::string(NTV2DeviceString(DEVICE_ID_CORVID44_8KMK)) == "Corvid44_8KMK");
		CHECK(std::string(NTV2DeviceString(DEVICE_ID_KONAIP_2110)) == "KonaIP_2110");
		CHECK(std::string(NTV2DeviceString(DEVICE_ID_IOIP_2110)) == "DNxIP_2110");
	}

	TEST_CASE("StandardString")
	{
		// spot check a few
		CHECK(std::string(NTV2StandardString(NTV2_STANDARD_1080)) == "NTV2_STANDARD_1080");
		CHECK(std::string(NTV2StandardString(NTV2_STANDARD_4096i)) == "NTV2_STANDARD_4096i");
		CHECK(std::string(NTV2StandardString(NTV2_STANDARD_INVALID)) == "NTV2_STANDARD_INVALID");
		CHECK(std::string(NTV2StandardString(NTV2_STANDARD_UNDEFINED)) == "NTV2_STANDARD_INVALID");
		CHECK(std::string(NTV2StandardString(NTV2_NUM_STANDARDS)) == "NTV2_STANDARD_INVALID");
	}

	TEST_CASE("FrameBufferFormatString")
	{
		// spot check a few
		CHECK(std::string(NTV2FrameBufferFormatString(NTV2_FBF_10BIT_YCBCR)) == "NTV2_FBF_10BIT_YCBCR");
		CHECK(std::string(NTV2FrameBufferFormatString(NTV2_FBF_FIRST)) == "NTV2_FBF_10BIT_YCBCR");
		CHECK(std::string(NTV2FrameBufferFormatString(NTV2_FBF_8BIT_YCBCR_422PL2)) == "NTV2_FBF_8BIT_YCBCR_422PL2");
		CHECK(std::string(NTV2FrameBufferFormatString(NTV2_FBF_LAST)) == "NTV2_FBF_INVALID");
		CHECK(std::string(NTV2FrameBufferFormatString(NTV2_FBF_NUMFRAMEBUFFERFORMATS)) == "NTV2_FBF_INVALID");
		CHECK(std::string(NTV2FrameBufferFormatString(NTV2_FBF_INVALID)) == "NTV2_FBF_INVALID");
	}

	TEST_CASE("FrameGeometryString")
	{
		// spot check a few
		CHECK(std::string(NTV2FrameGeometryString(NTV2_FG_1920x1080)) == "NTV2_FG_1920x1080");
		CHECK(std::string(NTV2FrameGeometryString(NTV2_FG_4x4096x2160)) == "NTV2_FG_4x4096x2160");
		CHECK(std::string(NTV2FrameGeometryString(NTV2_FG_NUMFRAMEGEOMETRIES)) == "NTV2_FG_INVALID");
		CHECK(std::string(NTV2FrameGeometryString(NTV2_FG_INVALID)) == "NTV2_FG_INVALID");
	}

	TEST_CASE("FrameRateString")
	{
		// spot check a few
		CHECK(std::string(NTV2FrameRateString(NTV2_FRAMERATE_6000)) == "NTV2_FRAMERATE_6000");
		CHECK(std::string(NTV2FrameRateString(NTV2_FRAMERATE_1500)) == "NTV2_FRAMERATE_1500");
		CHECK(std::string(NTV2FrameRateString(NTV2_NUM_FRAMERATES)) == "");
		CHECK(std::string(NTV2FrameRateString(NTV2_FRAMERATE_UNKNOWN)) == "NTV2_FRAMERATE_INVALID");
		CHECK(std::string(NTV2FrameRateString(NTV2_FRAMERATE_INVALID)) == "NTV2_FRAMERATE_INVALID");
	}

	TEST_CASE("VideoFormatString")
	{
		// spot check a few
		CHECK(std::string(NTV2VideoFormatString(NTV2_FORMAT_1080i_5000)) == "NTV2_FORMAT_1080i_5000");
		CHECK(std::string(NTV2VideoFormatString(NTV2_FORMAT_525_5994)) == "NTV2_FORMAT_525_5994");
		CHECK(std::string(NTV2VideoFormatString(NTV2_FORMAT_2K_1498)) == "NTV2_FORMAT_2K_1498");
		CHECK(std::string(NTV2VideoFormatString(NTV2_FORMAT_4x1920x1080psf_2398)) == "NTV2_FORMAT_4x1920x1080psf_2398");
		CHECK(std::string(NTV2VideoFormatString(NTV2_FORMAT_1080p_2K_6000_A)) == "NTV2_FORMAT_1080p_2K_6000_A");
		CHECK(std::string(NTV2VideoFormatString(NTV2_FORMAT_3840x2160psf_2398)) == "NTV2_FORMAT_3840x2160psf_2398");
		CHECK(std::string(NTV2VideoFormatString(NTV2_FORMAT_4096x2160psf_2398)) == "NTV2_FORMAT_4096x2160psf_2398");
		CHECK(std::string(NTV2VideoFormatString(NTV2_FORMAT_4x1920x1080p_5000_B)) == "NTV2_FORMAT_4x1920x1080p_5000_B");
		CHECK(std::string(NTV2VideoFormatString(NTV2_FORMAT_4x3840x2160p_2398)) == "NTV2_FORMAT_4x3840x2160p_2398");
		CHECK(std::string(NTV2VideoFormatString(NTV2_FORMAT_4x4096x2160p_2398)) == "NTV2_FORMAT_4x4096x2160p_2398");
		CHECK(std::string(NTV2VideoFormatString(NTV2_MAX_NUM_VIDEO_FORMATS)) == "");
		CHECK(std::string(NTV2VideoFormatString(NTV2_FORMAT_END_HIGH_DEF_FORMATS)) == "");
		CHECK(std::string(NTV2VideoFormatString(NTV2_FORMAT_END_STANDARD_DEF_FORMATS)) == "");
		CHECK(std::string(NTV2VideoFormatString(NTV2_FORMAT_END_2K_DEF_FORMATS)) == "");
		CHECK(std::string(NTV2VideoFormatString(NTV2_FORMAT_END_HIGH_DEF_FORMATS2)) == "");
		CHECK(std::string(NTV2VideoFormatString(NTV2_FORMAT_END_4K_TSI_DEF_FORMATS)) == "");
		CHECK(std::string(NTV2VideoFormatString(NTV2_FORMAT_END_4K_DEF_FORMATS2)) == "");
		CHECK(std::string(NTV2VideoFormatString(NTV2_FORMAT_END_UHD2_DEF_FORMATS)) == "");
		CHECK(std::string(NTV2VideoFormatString(NTV2_FORMAT_END_UHD2_FULL_DEF_FORMATS)) == "");
		CHECK(std::string(NTV2VideoFormatString(NTV2_FORMAT_UNKNOWN)) == "");
	}

	TEST_CASE("RegisterNameString")
	{
		// spot check a few
		CHECK(std::string(NTV2RegisterNameString(kRegGlobalControl)) == "kRegGlobalControl");
		CHECK(std::string(NTV2RegisterNameString(kRegReserved511)) == "kRegReserved511");
		CHECK(std::string(NTV2RegisterNameString(kRegNumRegisters)) == "");
	}

	TEST_CASE("InterruptEnumString")
	{
		// spot check a few
		CHECK(std::string(NTV2InterruptEnumString(eVerticalInterrupt)) == "eOutput1");
		CHECK(std::string(NTV2InterruptEnumString(eOutput1)) == "eOutput1");
		CHECK(std::string(NTV2InterruptEnumString(eUartTx)) == "eUart1Tx");
		CHECK(std::string(NTV2InterruptEnumString(eUart1Tx)) == "eUart1Tx");
		CHECK(std::string(NTV2InterruptEnumString(eUartRx)) == "eUart1Rx");
		CHECK(std::string(NTV2InterruptEnumString(eUart1Rx)) == "eUart1Rx");
		CHECK(std::string(NTV2InterruptEnumString(eUartTx2)) == "eUart2Tx");
		CHECK(std::string(NTV2InterruptEnumString(eUart2Tx)) == "eUart2Tx");
		CHECK(std::string(NTV2InterruptEnumString(eUartRx2)) == "eUart2Rx");
		CHECK(std::string(NTV2InterruptEnumString(eUart2Rx)) == "eUart2Rx");
		CHECK(std::string(NTV2InterruptEnumString(eNumInterruptTypes)) == "");

	}

} //ntv2debug


void ntv2utils_marker() {}

void check_fmts_are_std(std::vector<NTV2VideoFormat> formats, NTV2Standard expected)
{
	std::string msgBase("expected: ");
	msgBase += NTV2StandardString(expected);
	msgBase += " = " + std::to_string(int(expected));
	msgBase += ", but got: ";

	for (auto &fmt : formats)
	{
		NTV2Standard have = GetNTV2StandardFromVideoFormat(fmt);
		std::string msg = msgBase;
		msg += NTV2StandardString(have);
		msg += " = " + std::to_string(int(have));
		CHECK_MESSAGE(expected == have, msg);
	}
}

void check_fmts_are_geom(std::vector<NTV2VideoFormat> formats, NTV2FrameGeometry expected)
{
	std::string msgBase("expected: ");
	msgBase += NTV2FrameGeometryString(expected);
	msgBase += " = " + std::to_string(int(expected));
	msgBase += ", but got: ";

	for (auto &fmt : formats)
	{
		NTV2FrameGeometry have = GetNTV2FrameGeometryFromVideoFormat(fmt);
		std::string msg = msgBase;
		msg += NTV2FrameGeometryString(have);
		msg += " = " + std::to_string(int(have));
		CHECK_MESSAGE(expected == have, msg);
	}
}

TEST_SUITE("ntv2utils" * doctest::description("ntv2 utils functions")) {

	TEST_CASE("GetNTV2StandardFromVideoFormat")
	{
		check_fmts_are_std({NTV2_FORMAT_1080i_5000,
							NTV2_FORMAT_1080i_5994,
							NTV2_FORMAT_1080i_6000,
							NTV2_FORMAT_1080psf_2398,
							NTV2_FORMAT_1080psf_2400,
							NTV2_FORMAT_1080psf_2500_2,
							NTV2_FORMAT_1080psf_2997_2,
							NTV2_FORMAT_1080psf_3000_2,
							NTV2_FORMAT_1080p_5000_B,
							NTV2_FORMAT_1080p_5994_B,
							NTV2_FORMAT_1080p_6000_B,
							NTV2_FORMAT_1080p_2K_4795_B,
							NTV2_FORMAT_1080p_2K_4800_B,
							NTV2_FORMAT_1080p_2K_5000_B,
							NTV2_FORMAT_1080p_2K_5994_B,
							NTV2_FORMAT_1080p_2K_6000_B}, NTV2_STANDARD_1080);

		check_fmts_are_std({NTV2_FORMAT_1080p_2500,
							NTV2_FORMAT_1080p_2997,
							NTV2_FORMAT_1080p_3000,
							NTV2_FORMAT_1080p_2398,
							NTV2_FORMAT_1080p_2400,
							NTV2_FORMAT_1080p_5000_A,
							NTV2_FORMAT_1080p_5994_A,
							NTV2_FORMAT_1080p_6000_A}, NTV2_STANDARD_1080p);

		check_fmts_are_std({NTV2_FORMAT_1080p_2K_2398,
							NTV2_FORMAT_1080p_2K_2400,
							NTV2_FORMAT_1080p_2K_2500,
							NTV2_FORMAT_1080p_2K_2997,
							NTV2_FORMAT_1080p_2K_3000,
							NTV2_FORMAT_1080p_2K_4795_A,
							NTV2_FORMAT_1080p_2K_4800_A,
							NTV2_FORMAT_1080p_2K_5000_A,
							NTV2_FORMAT_1080p_2K_5994_A,
							NTV2_FORMAT_1080p_2K_6000_A}, NTV2_STANDARD_2Kx1080p);

		check_fmts_are_std({NTV2_FORMAT_1080psf_2K_2398,
							NTV2_FORMAT_1080psf_2K_2400,
							NTV2_FORMAT_1080psf_2K_2500}, NTV2_STANDARD_2Kx1080i);

		check_fmts_are_std({NTV2_FORMAT_720p_2398,
							NTV2_FORMAT_720p_5000,
							NTV2_FORMAT_720p_5994,
							NTV2_FORMAT_720p_6000,
							NTV2_FORMAT_720p_2500}, NTV2_STANDARD_720);

		check_fmts_are_std({NTV2_FORMAT_525_5994,
							NTV2_FORMAT_525_2398,
							NTV2_FORMAT_525_2400,
							NTV2_FORMAT_525psf_2997}, NTV2_STANDARD_525);

		check_fmts_are_std({NTV2_FORMAT_625_5000,
							NTV2_FORMAT_625psf_2500}, NTV2_STANDARD_625);

		check_fmts_are_std({NTV2_FORMAT_2K_1498,
							NTV2_FORMAT_2K_1500,
							NTV2_FORMAT_2K_2398,
							NTV2_FORMAT_2K_2400,
							NTV2_FORMAT_2K_2500}, NTV2_STANDARD_2K);

		check_fmts_are_std({NTV2_FORMAT_4x1920x1080p_2398,
							NTV2_FORMAT_4x1920x1080p_2400,
							NTV2_FORMAT_4x1920x1080p_2500,
							NTV2_FORMAT_4x1920x1080p_2997,
							NTV2_FORMAT_4x1920x1080p_3000,
							NTV2_FORMAT_3840x2160p_2398,
							NTV2_FORMAT_3840x2160p_2400,
							NTV2_FORMAT_3840x2160p_2500,
							NTV2_FORMAT_3840x2160p_2997,
							NTV2_FORMAT_3840x2160p_3000}, NTV2_STANDARD_3840x2160p);

		check_fmts_are_std({NTV2_FORMAT_4x1920x1080psf_2398,
							NTV2_FORMAT_4x1920x1080psf_2400,
							NTV2_FORMAT_4x1920x1080psf_2500,
							NTV2_FORMAT_4x1920x1080psf_2997,
							NTV2_FORMAT_4x1920x1080psf_3000,
							NTV2_FORMAT_3840x2160psf_2398,
							NTV2_FORMAT_3840x2160psf_2400,
							NTV2_FORMAT_3840x2160psf_2500,
							NTV2_FORMAT_3840x2160psf_2997,
							NTV2_FORMAT_3840x2160psf_3000}, NTV2_STANDARD_3840x2160p);

		check_fmts_are_std({NTV2_FORMAT_4x1920x1080p_5000,
							NTV2_FORMAT_4x1920x1080p_5994,
							NTV2_FORMAT_4x1920x1080p_6000,
							NTV2_FORMAT_3840x2160p_5000,
							NTV2_FORMAT_3840x2160p_5994,
							NTV2_FORMAT_3840x2160p_6000}, NTV2_STANDARD_3840HFR);

		check_fmts_are_std({NTV2_FORMAT_4x2048x1080p_2398,
							NTV2_FORMAT_4x2048x1080p_2400,
							NTV2_FORMAT_4x2048x1080p_2500,
							NTV2_FORMAT_4x2048x1080p_2997,
							NTV2_FORMAT_4x2048x1080p_3000,
							NTV2_FORMAT_4x2048x1080p_4795,
							NTV2_FORMAT_4x2048x1080p_4800,
							NTV2_FORMAT_4096x2160p_2398,
							NTV2_FORMAT_4096x2160p_2400,
							NTV2_FORMAT_4096x2160p_2500,
							NTV2_FORMAT_4096x2160p_2997,
							NTV2_FORMAT_4096x2160p_3000,
							NTV2_FORMAT_4096x2160p_4795,
							NTV2_FORMAT_4096x2160p_4800}, NTV2_STANDARD_4096x2160p);

		check_fmts_are_std({NTV2_FORMAT_4x2048x1080psf_2398,
							NTV2_FORMAT_4x2048x1080psf_2400,
							NTV2_FORMAT_4x2048x1080psf_2500,
							NTV2_FORMAT_4x2048x1080psf_2997,
							NTV2_FORMAT_4x2048x1080psf_3000,
							NTV2_FORMAT_4096x2160psf_2398,
							NTV2_FORMAT_4096x2160psf_2400,
							NTV2_FORMAT_4096x2160psf_2500,
							NTV2_FORMAT_4096x2160psf_2997,
							NTV2_FORMAT_4096x2160psf_3000}, NTV2_STANDARD_4096x2160p);

		check_fmts_are_std({NTV2_FORMAT_4x2048x1080p_5000,
							NTV2_FORMAT_4x2048x1080p_5994,
							NTV2_FORMAT_4x2048x1080p_6000,
							NTV2_FORMAT_4x2048x1080p_11988,
							NTV2_FORMAT_4x2048x1080p_12000,
							NTV2_FORMAT_4096x2160p_5000,
							NTV2_FORMAT_4096x2160p_5994,
							NTV2_FORMAT_4096x2160p_6000,
							NTV2_FORMAT_4096x2160p_11988,
							NTV2_FORMAT_4096x2160p_12000}, NTV2_STANDARD_4096HFR);

		check_fmts_are_std({NTV2_FORMAT_4x3840x2160p_2398,
							NTV2_FORMAT_4x3840x2160p_2400,
							NTV2_FORMAT_4x3840x2160p_2500,
							NTV2_FORMAT_4x3840x2160p_2997,
							NTV2_FORMAT_4x3840x2160p_3000,
							NTV2_FORMAT_4x3840x2160p_5000,
							NTV2_FORMAT_4x3840x2160p_5994,
							NTV2_FORMAT_4x3840x2160p_6000}, NTV2_STANDARD_7680);

		check_fmts_are_std({NTV2_FORMAT_4x4096x2160p_2398,
							NTV2_FORMAT_4x4096x2160p_2400,
							NTV2_FORMAT_4x4096x2160p_2500,
							NTV2_FORMAT_4x4096x2160p_2997,
							NTV2_FORMAT_4x4096x2160p_3000,
							NTV2_FORMAT_4x4096x2160p_4795,
							NTV2_FORMAT_4x4096x2160p_4800,
							NTV2_FORMAT_4x4096x2160p_5000,
							NTV2_FORMAT_4x4096x2160p_5994,
							NTV2_FORMAT_4x4096x2160p_6000}, NTV2_STANDARD_8192);
	}

	TEST_CASE("GetNTV2FrameGeometryFromVideoFormat")
	{
		check_fmts_are_geom({NTV2_FORMAT_4x3840x2160p_2398,
							 NTV2_FORMAT_4x3840x2160p_2400,
							 NTV2_FORMAT_4x3840x2160p_2500,
							 NTV2_FORMAT_4x3840x2160p_2997,
							 NTV2_FORMAT_4x3840x2160p_3000,
							 NTV2_FORMAT_4x3840x2160p_5000,
							 NTV2_FORMAT_4x3840x2160p_5994,
							 NTV2_FORMAT_4x3840x2160p_6000,
							 NTV2_FORMAT_4x3840x2160p_5000_B,
							 NTV2_FORMAT_4x3840x2160p_5994_B,
							 NTV2_FORMAT_4x3840x2160p_6000_B}, NTV2_FG_4x3840x2160);

		check_fmts_are_geom({NTV2_FORMAT_4x4096x2160p_2398,
							 NTV2_FORMAT_4x4096x2160p_2400,
							 NTV2_FORMAT_4x4096x2160p_2500,
							 NTV2_FORMAT_4x4096x2160p_2997,
							 NTV2_FORMAT_4x4096x2160p_3000,
							 NTV2_FORMAT_4x4096x2160p_4795,
							 NTV2_FORMAT_4x4096x2160p_4800,
							 NTV2_FORMAT_4x4096x2160p_5000,
							 NTV2_FORMAT_4x4096x2160p_5994,
							 NTV2_FORMAT_4x4096x2160p_6000,
							 NTV2_FORMAT_4x4096x2160p_4795_B,
							 NTV2_FORMAT_4x4096x2160p_4800_B,
							 NTV2_FORMAT_4x4096x2160p_5000_B,
							 NTV2_FORMAT_4x4096x2160p_5994_B,
							 NTV2_FORMAT_4x4096x2160p_6000_B}, NTV2_FG_4x4096x2160);

		check_fmts_are_geom({NTV2_FORMAT_4x1920x1080psf_2398,
							 NTV2_FORMAT_4x1920x1080psf_2400,
							 NTV2_FORMAT_4x1920x1080psf_2500,
							 NTV2_FORMAT_4x1920x1080psf_2997,
							 NTV2_FORMAT_4x1920x1080psf_3000,
							 NTV2_FORMAT_4x1920x1080p_2398,
							 NTV2_FORMAT_4x1920x1080p_2400,
							 NTV2_FORMAT_4x1920x1080p_2500,
							 NTV2_FORMAT_4x1920x1080p_2997,
							 NTV2_FORMAT_4x1920x1080p_3000,
							 NTV2_FORMAT_4x1920x1080p_5000,
							 NTV2_FORMAT_4x1920x1080p_5994,
							 NTV2_FORMAT_4x1920x1080p_6000,
							 NTV2_FORMAT_3840x2160psf_2398,
							 NTV2_FORMAT_3840x2160psf_2400,
							 NTV2_FORMAT_3840x2160psf_2500,
							 NTV2_FORMAT_3840x2160p_2398,
							 NTV2_FORMAT_3840x2160p_2400,
							 NTV2_FORMAT_3840x2160p_2500,
							 NTV2_FORMAT_3840x2160p_2997,
							 NTV2_FORMAT_3840x2160p_3000,
							 NTV2_FORMAT_3840x2160psf_2997,
							 NTV2_FORMAT_3840x2160psf_3000,
							 NTV2_FORMAT_3840x2160p_5000,
							 NTV2_FORMAT_3840x2160p_5994,
							 NTV2_FORMAT_3840x2160p_6000,
							 NTV2_FORMAT_4x1920x1080p_5000_B,
							 NTV2_FORMAT_4x1920x1080p_5994_B,
							 NTV2_FORMAT_4x1920x1080p_6000_B,
							 NTV2_FORMAT_3840x2160p_5000_B,
							 NTV2_FORMAT_3840x2160p_5994_B,
							 NTV2_FORMAT_3840x2160p_6000_B}, NTV2_FG_4x1920x1080);

		check_fmts_are_geom({NTV2_FORMAT_4x2048x1080psf_2398,
							 NTV2_FORMAT_4x2048x1080psf_2400,
							 NTV2_FORMAT_4x2048x1080psf_2500,
							 NTV2_FORMAT_4x2048x1080p_2398,
							 NTV2_FORMAT_4x2048x1080p_2400,
							 NTV2_FORMAT_4x2048x1080p_2500,
							 NTV2_FORMAT_4x2048x1080p_2997,
							 NTV2_FORMAT_4x2048x1080p_3000,
							 NTV2_FORMAT_4x2048x1080psf_2997,
							 NTV2_FORMAT_4x2048x1080psf_3000,
							 NTV2_FORMAT_4x2048x1080p_4795,
							 NTV2_FORMAT_4x2048x1080p_4800,
							 NTV2_FORMAT_4x2048x1080p_5000,
							 NTV2_FORMAT_4x2048x1080p_5994,
							 NTV2_FORMAT_4x2048x1080p_6000,
							 NTV2_FORMAT_4x2048x1080p_11988,
							 NTV2_FORMAT_4x2048x1080p_12000,
							 NTV2_FORMAT_4096x2160psf_2398,
							 NTV2_FORMAT_4096x2160psf_2400,
							 NTV2_FORMAT_4096x2160psf_2500,
							 NTV2_FORMAT_4096x2160p_2398,
							 NTV2_FORMAT_4096x2160p_2400,
							 NTV2_FORMAT_4096x2160p_2500,
							 NTV2_FORMAT_4096x2160p_2997,
							 NTV2_FORMAT_4096x2160p_3000,
							 NTV2_FORMAT_4096x2160psf_2997,
							 NTV2_FORMAT_4096x2160psf_3000,
							 NTV2_FORMAT_4096x2160p_4795,
							 NTV2_FORMAT_4096x2160p_4800,
							 NTV2_FORMAT_4096x2160p_5000,
							 NTV2_FORMAT_4096x2160p_5994,
							 NTV2_FORMAT_4096x2160p_6000,
							 NTV2_FORMAT_4096x2160p_11988,
							 NTV2_FORMAT_4096x2160p_12000,
							 NTV2_FORMAT_4x2048x1080p_4795_B,
							 NTV2_FORMAT_4x2048x1080p_4800_B,
							 NTV2_FORMAT_4x2048x1080p_5000_B,
							 NTV2_FORMAT_4x2048x1080p_5994_B,
							 NTV2_FORMAT_4x2048x1080p_6000_B,
							 NTV2_FORMAT_4096x2160p_4795_B,
							 NTV2_FORMAT_4096x2160p_4800_B,
							 NTV2_FORMAT_4096x2160p_5000_B,
							 NTV2_FORMAT_4096x2160p_5994_B,
							 NTV2_FORMAT_4096x2160p_6000_B}, NTV2_FG_4x2048x1080);

		check_fmts_are_geom({NTV2_FORMAT_2K_1498,
							 NTV2_FORMAT_2K_1500,
							 NTV2_FORMAT_2K_2398,
							 NTV2_FORMAT_2K_2400,
							 NTV2_FORMAT_2K_2500}, NTV2_FG_2048x1556);

		check_fmts_are_geom({NTV2_FORMAT_1080i_5000,
							 NTV2_FORMAT_1080i_5994,
							 NTV2_FORMAT_1080i_6000,
							 NTV2_FORMAT_1080psf_2500_2,
							 NTV2_FORMAT_1080psf_2997_2,
							 NTV2_FORMAT_1080psf_3000_2,
							 NTV2_FORMAT_1080psf_2398,
							 NTV2_FORMAT_1080psf_2400,
							 NTV2_FORMAT_1080p_2997,
							 NTV2_FORMAT_1080p_3000,
							 NTV2_FORMAT_1080p_2398,
							 NTV2_FORMAT_1080p_2400,
							 NTV2_FORMAT_1080p_2500,
							 NTV2_FORMAT_1080p_5000_B,
							 NTV2_FORMAT_1080p_5994_B,
							 NTV2_FORMAT_1080p_6000_B,
							 NTV2_FORMAT_1080p_5000_A,
							 NTV2_FORMAT_1080p_5994_A,
							 NTV2_FORMAT_1080p_6000_A}, NTV2_FG_1920x1080);

		check_fmts_are_geom({NTV2_FORMAT_1080p_2K_2398,
							 NTV2_FORMAT_1080p_2K_2400,
							 NTV2_FORMAT_1080p_2K_2500,
							 NTV2_FORMAT_1080psf_2K_2398,
							 NTV2_FORMAT_1080psf_2K_2400,
							 NTV2_FORMAT_1080psf_2K_2500,
							 NTV2_FORMAT_1080p_2K_2997,
							 NTV2_FORMAT_1080p_2K_3000,
							 NTV2_FORMAT_1080p_2K_4795_A,
							 NTV2_FORMAT_1080p_2K_4795_B,
							 NTV2_FORMAT_1080p_2K_4800_A,
							 NTV2_FORMAT_1080p_2K_4800_B,
							 NTV2_FORMAT_1080p_2K_5000_A,
							 NTV2_FORMAT_1080p_2K_5000_B,
							 NTV2_FORMAT_1080p_2K_5994_A,
							 NTV2_FORMAT_1080p_2K_5994_B,
							 NTV2_FORMAT_1080p_2K_6000_A,
							 NTV2_FORMAT_1080p_2K_6000_B}, NTV2_FG_2048x1080);

		check_fmts_are_geom({NTV2_FORMAT_720p_2398,
							 NTV2_FORMAT_720p_2500,
							 NTV2_FORMAT_720p_5994,
							 NTV2_FORMAT_720p_6000,
							 NTV2_FORMAT_720p_5000}, NTV2_FG_1280x720);

		check_fmts_are_geom({NTV2_FORMAT_525_2398,
							 NTV2_FORMAT_525_2400,
							 NTV2_FORMAT_525_5994,
							 NTV2_FORMAT_525psf_2997}, NTV2_FG_720x486);

		check_fmts_are_geom({NTV2_FORMAT_625_5000,
							 NTV2_FORMAT_625psf_2500}, NTV2_FG_720x576);
	}

	TEST_CASE("GetVideoActiveSize")
	{
		CHECK(GetVideoActiveSize(NTV2_FORMAT_2K_2398, NTV2_FBF_10BIT_YCBCR, NTV2_VANCMODE_OFF) == 8564224);
		CHECK(GetVideoActiveSize(NTV2_FORMAT_2K_2398, NTV2_FBF_8BIT_YCBCR, NTV2_VANCMODE_OFF) == 6373376);
		CHECK(GetVideoActiveSize(NTV2_FORMAT_2K_2398, NTV2_FBF_ARGB, NTV2_VANCMODE_OFF) == 12746752);
		CHECK(GetVideoActiveSize(NTV2_FORMAT_2K_2398, NTV2_FBF_10BIT_RGB, NTV2_VANCMODE_OFF) == 12746752);
		CHECK(GetVideoActiveSize(NTV2_FORMAT_2K_2398, NTV2_FBF_48BIT_RGB, NTV2_VANCMODE_OFF) == 19120128);

		CHECK(GetVideoActiveSize(NTV2_FORMAT_1080i_5994, NTV2_FBF_10BIT_YCBCR, NTV2_VANCMODE_OFF) == 5529600);
		CHECK(GetVideoActiveSize(NTV2_FORMAT_1080i_5994, NTV2_FBF_8BIT_YCBCR, NTV2_VANCMODE_OFF) == 4147200);
		CHECK(GetVideoActiveSize(NTV2_FORMAT_1080i_5994, NTV2_FBF_ARGB, NTV2_VANCMODE_OFF) == 8294400);
		CHECK(GetVideoActiveSize(NTV2_FORMAT_1080i_5994, NTV2_FBF_10BIT_RGB, NTV2_VANCMODE_OFF) == 8294400);
		CHECK(GetVideoActiveSize(NTV2_FORMAT_1080i_5994, NTV2_FBF_48BIT_RGB, NTV2_VANCMODE_OFF) == 12441600);

		CHECK(GetVideoActiveSize(NTV2_FORMAT_720p_5994, NTV2_FBF_10BIT_YCBCR, NTV2_VANCMODE_OFF) == 2488320);
		CHECK(GetVideoActiveSize(NTV2_FORMAT_720p_5994, NTV2_FBF_8BIT_YCBCR, NTV2_VANCMODE_OFF) == 1843200);
		CHECK(GetVideoActiveSize(NTV2_FORMAT_720p_5994, NTV2_FBF_ARGB, NTV2_VANCMODE_OFF) == 3686400);
		CHECK(GetVideoActiveSize(NTV2_FORMAT_720p_5994, NTV2_FBF_10BIT_RGB, NTV2_VANCMODE_OFF) == 3686400);
		CHECK(GetVideoActiveSize(NTV2_FORMAT_720p_5994, NTV2_FBF_48BIT_RGB, NTV2_VANCMODE_OFF) == 5529600);

		CHECK(GetVideoActiveSize(NTV2_FORMAT_525_5994, NTV2_FBF_10BIT_YCBCR, NTV2_VANCMODE_OFF) == 933120);
		CHECK(GetVideoActiveSize(NTV2_FORMAT_525_5994, NTV2_FBF_8BIT_YCBCR, NTV2_VANCMODE_OFF) == 699840);
		CHECK(GetVideoActiveSize(NTV2_FORMAT_525_5994, NTV2_FBF_ARGB, NTV2_VANCMODE_OFF) == 1399680);
		CHECK(GetVideoActiveSize(NTV2_FORMAT_525_5994, NTV2_FBF_10BIT_RGB, NTV2_VANCMODE_OFF) == 1399680);
		CHECK(GetVideoActiveSize(NTV2_FORMAT_525_5994, NTV2_FBF_48BIT_RGB, NTV2_VANCMODE_OFF) == 2099520);
	}

} // ntv2utils

void ntv2devicescanner_marker() {}
TEST_SUITE("ntv2devicescanner" * doctest::description("ntv2 device scanner functions")) {

	// NOTE: these are implemented in ntv2devicescanner.cpp but declared in ntv2publicinterface.hs

	TEST_CASE("NTV2DeviceGetSupportedPixelFormats")
	{
		// Make sure that ticket# 4220 doesn't happen again
		// spot check with Kona4
		NTV2FrameBufferFormatSet fbf_set;
		CHECK(NTV2DeviceGetSupportedPixelFormats(DEVICE_ID_KONA4, fbf_set));
		CHECK(fbf_set.count(NTV2_FBF_10BIT_YCBCR) > 0);
	}

	TEST_CASE("NTV2DeviceGetSupportedVideoFormats")
	{
		// spot check with Kona4
		NTV2VideoFormatSet vf_set;
		CHECK(NTV2DeviceGetSupportedVideoFormats(DEVICE_ID_KONA4, vf_set));
		// supported
		CHECK(vf_set.count(NTV2_FORMAT_1080i_5000) > 0);
		CHECK(vf_set.count(NTV2_FORMAT_1080psf_3000_2) > 0);
		CHECK(vf_set.count(NTV2_FORMAT_525_5994) > 0);
		CHECK(vf_set.count(NTV2_FORMAT_4x1920x1080psf_2398) > 0);
		CHECK(vf_set.count(NTV2_FORMAT_4x2048x1080p_4800) > 0);
		CHECK(vf_set.count(NTV2_FORMAT_1080p_2K_6000_A) > 0);
		CHECK(vf_set.count(NTV2_FORMAT_1080p_2K_4800_A) > 0);
		// not supported
		CHECK(vf_set.count(NTV2_FORMAT_4x2048x1080p_11988) == 0);
		CHECK(vf_set.count(NTV2_FORMAT_4x2048x1080p_12000) == 0);
		CHECK(vf_set.count(NTV2_FORMAT_1080p_2K_4795_B) == 0);
		CHECK(vf_set.count(NTV2_FORMAT_1080p_2K_6000_B) == 0);
		CHECK(vf_set.count(NTV2_FORMAT_3840x2160psf_2398) == 0);
		CHECK(vf_set.count(NTV2_FORMAT_3840x2160p_6000_B) == 0);
		CHECK(vf_set.count(NTV2_FORMAT_4096x2160psf_2398) == 0);
		CHECK(vf_set.count(NTV2_FORMAT_4096x2160p_6000_B) == 0);
		CHECK(vf_set.count(NTV2_FORMAT_4x1920x1080p_5000_B) == 0);
		CHECK(vf_set.count(NTV2_FORMAT_4x2048x1080p_4800_B) == 0);
		CHECK(vf_set.count(NTV2_FORMAT_4x3840x2160p_2398) == 0);
		CHECK(vf_set.count(NTV2_FORMAT_4x3840x2160p_6000_B) == 0);
		CHECK(vf_set.count(NTV2_FORMAT_4x4096x2160p_2398) == 0);
		CHECK(vf_set.count(NTV2_FORMAT_4x4096x2160p_6000_B) == 0);
	}

	TEST_CASE("NTV2DeviceGetSupportedStandards")
	{
		// spot check with Kona4
		NTV2StandardSet std_set;
		CHECK(NTV2DeviceGetSupportedStandards(DEVICE_ID_KONA4, std_set));
		// supported
		CHECK(std_set.count(NTV2_STANDARD_1080) > 0);
		CHECK(std_set.count(NTV2_STANDARD_720) > 0);
		CHECK(std_set.count(NTV2_STANDARD_525) > 0);
		CHECK(std_set.count(NTV2_STANDARD_625) > 0);
		CHECK(std_set.count(NTV2_STANDARD_1080p) > 0);
		CHECK(std_set.count(NTV2_STANDARD_2Kx1080p) > 0);
		CHECK(std_set.count(NTV2_STANDARD_2Kx1080i) > 0);
		CHECK(std_set.count(NTV2_STANDARD_3840x2160p) > 0);
		CHECK(std_set.count(NTV2_STANDARD_4096x2160p) > 0);
		CHECK(std_set.count(NTV2_STANDARD_3840HFR) > 0);
		CHECK(std_set.count(NTV2_STANDARD_4096HFR) > 0);
		// not supported
		CHECK(std_set.count(NTV2_STANDARD_2K) == 0);
		CHECK(std_set.count(NTV2_STANDARD_7680) == 0);
		CHECK(std_set.count(NTV2_STANDARD_8192) == 0);
		CHECK(std_set.count(NTV2_STANDARD_3840i) == 0);
		CHECK(std_set.count(NTV2_STANDARD_4096i) == 0);
	}

	TEST_CASE("NTV2DeviceGetSupportedGeometries")
	{
		NTV2GeometrySet geom_set;
		CHECK(NTV2DeviceGetSupportedGeometries(DEVICE_ID_KONA4, geom_set));
		// supported
		CHECK(geom_set.count(NTV2_FG_1920x1080) > 0);
		CHECK(geom_set.count(NTV2_FG_1280x720) > 0);
		CHECK(geom_set.count(NTV2_FG_720x486) > 0);
		CHECK(geom_set.count(NTV2_FG_720x576) > 0);
		CHECK(geom_set.count(NTV2_FG_1920x1114) > 0);
		CHECK(geom_set.count(NTV2_FG_2048x1114) > 0);
		CHECK(geom_set.count(NTV2_FG_720x508) > 0);
		CHECK(geom_set.count(NTV2_FG_720x598) > 0);
		CHECK(geom_set.count(NTV2_FG_1920x1112) > 0);
		CHECK(geom_set.count(NTV2_FG_1280x740) > 0);
		CHECK(geom_set.count(NTV2_FG_2048x1080) > 0);
		CHECK(geom_set.count(NTV2_FG_2048x1112) > 0);
		CHECK(geom_set.count(NTV2_FG_720x514) > 0);
		CHECK(geom_set.count(NTV2_FG_720x612) > 0);
		CHECK(geom_set.count(NTV2_FG_4x1920x1080) > 0);
		CHECK(geom_set.count(NTV2_FG_4x2048x1080) > 0);
		// not supported
		CHECK(geom_set.count(NTV2_FG_2048x1556) == 0);
		CHECK(geom_set.count(NTV2_FG_2048x1588) == 0);
		CHECK(geom_set.count(NTV2_FG_4x3840x2160) == 0);
		CHECK(geom_set.count(NTV2_FG_4x4096x2160) == 0);
	}

} // ntv2devicescanner

void ntv2vpid_marker() {}

TEST_SUITE("CNTV2VPID" * doctest::description("CNTV2VPID functions")) {
	TEST_CASE("CNTV2VPID::GetVPID")
	{
		const ULWord data = 0x85c62000;
		const CNTV2VPID vpid(data);
		CHECK(vpid.GetVPID() == data);
	}

	TEST_CASE("CNTV2VPID::GetBitDepth")
	{
		{ // 8-bit
			const ULWord data = 0x85c62000;
			const CNTV2VPID vpid(data);
			const VPIDBitDepth& bit_depth = vpid.GetBitDepth();
			CHECK(bit_depth == VPIDBitDepth_10_Full);
		}
		{ // 10-bit
			const ULWord data = 0x85c62001;
			const CNTV2VPID vpid(data);
			const auto& bit_depth = vpid.GetBitDepth();
			CHECK(bit_depth == VPIDBitDepth_10);
		}
		{ // 12-bit
			const ULWord data = 0x85c62002;
			const CNTV2VPID vpid(data);
			const auto& bit_depth = vpid.GetBitDepth();
			CHECK(bit_depth == VPIDBitDepth_12);
		}
		{ // Reserved
			const ULWord data = 0x85c62003;
			const CNTV2VPID vpid(data);
			const auto& bit_depth = vpid.GetBitDepth();
			CHECK(bit_depth == VPIDBitDepth_12_Full);
		}
	}

	TEST_CASE("CNTV2VPID::SetBitDepth")
	{
		{ // 8-bit
			const ULWord data = 0x8a052201;
			const VPIDBitDepth bd = VPIDBitDepth_10_Full;
			CNTV2VPID vpid(data);
			vpid.SetBitDepth(bd);
			CHECK(vpid.GetBitDepth() == bd);
		}
		{ // 10-bit
			const ULWord data = 0x8a052200;
			const VPIDBitDepth bd = VPIDBitDepth_10;
			CNTV2VPID vpid(data);
			vpid.SetBitDepth(bd);
			CHECK(vpid.GetBitDepth() == bd);
		}
		{ // 12-bit
			const ULWord data = 0x8a052200;
			const VPIDBitDepth bd = VPIDBitDepth_12;
			CNTV2VPID vpid(data);
			vpid.SetBitDepth(bd);
			CHECK(vpid.GetBitDepth() == bd);
		}
		{ // Reserved
			const ULWord data = 0x8a052200;
			const VPIDBitDepth bd = VPIDBitDepth_12_Full;
			CNTV2VPID vpid(data);
			vpid.SetBitDepth(bd);
			CHECK(vpid.GetBitDepth() == bd);
		}
	}

	TEST_CASE("CNTV2VPID::GetChannel/GetDualLinkChannel")
	{
		{ // Channel 1
			const ULWord data = 0x85c62001;
			const CNTV2VPID vpid(data);
			const auto& chan = vpid.GetChannel();
			CHECK(chan == VPIDChannel_1);
		}
		{ // Channel 2
			const ULWord data = 0x85c62041;
			const CNTV2VPID vpid(data);
			const auto& chan = vpid.GetChannel();
			CHECK(chan == VPIDChannel_2);
		}
		{ // Channel 3
			const ULWord data = 0x85c62081;
			const CNTV2VPID vpid(data);
			const auto& chan = vpid.GetChannel();
			CHECK(chan == VPIDChannel_3);
		}
		{ // Channel 4
			const ULWord data = 0x85c620c1;
			const CNTV2VPID vpid(data);
			const auto& chan = vpid.GetChannel();
			CHECK(chan == VPIDChannel_4);
		}
		{ // Channel 5
			const ULWord data = 0x85c62081;
			const CNTV2VPID vpid(data);
			const auto& chan = vpid.GetDualLinkChannel();
			CHECK(chan == VPIDChannel_5);
		}
		{ // Channel 6
			const ULWord data = 0x85c620a1;
			const CNTV2VPID vpid(data);
			const auto& chan = vpid.GetDualLinkChannel();
			CHECK(chan == VPIDChannel_6);
		}
		{ // Channel 7
			const ULWord data = 0x85c620c1;
			const CNTV2VPID vpid(data);
			const auto& chan = vpid.GetDualLinkChannel();
			CHECK(chan == VPIDChannel_7);
		}
		{ // Channel 8
			const ULWord data = 0x85c620e1;
			const CNTV2VPID vpid(data);
			const auto& chan = vpid.GetDualLinkChannel();
			CHECK(chan == VPIDChannel_8);
		}
	}

	TEST_CASE("CNTV2VPID::SetChannel/SetDualLinkChannel")
	{
		{ // Get/SetChannel 1-4
			for (int i = 0; i < 4; i++)
			{
				const ULWord data = 0x85c62001;
				CNTV2VPID vpid(data);
				vpid.SetChannel((VPIDChannel)i);
				CHECK(vpid.GetChannel() == (VPIDChannel)i);
			}
		}
		{ // Get/SetDualLinkChannel
			for (int i = 4; i < 8; i++)
			{
				const ULWord data = 0x85c62001;
				CNTV2VPID vpid(data);
				vpid.SetDualLinkChannel((VPIDChannel)i);
				CHECK(vpid.GetDualLinkChannel() == (VPIDChannel)i);
			}
		}
	}

	TEST_CASE("CNTV2VPID GetVideoFormat")
	{
		//TODO(paulh): add more checks
		const ULWord data = 0x85c62001;
		const CNTV2VPID vpid(data);
		const auto& fmt = vpid.GetVideoFormat();
		CHECK(fmt == NTV2_FORMAT_1080p_2997);
	}

	TEST_CASE("CNTV2VPID::GetStandard")
	{
		//TODO(paulh): add more checks
		const size_t num_stds = 6;
		const VPIDStandard standards[num_stds] = {
			VPIDStandard_483_576,
			VPIDStandard_720,
			VPIDStandard_1080,
			VPIDStandard_1080_DualLink_3Gb,
			VPIDStandard_2160_QuadLink_3Ga,
			VPIDStandard_2160_Single_12Gb
		};

		const ULWord vpid_other_bytes = 0xc62001;

		for (size_t i = 0; i < num_stds; i++)
		{
			const uint8_t new_std = (uint8_t)standards[i];
			ULWord data = (new_std << 24) | vpid_other_bytes;
			CNTV2VPID vpid(data);
			CHECK(vpid.GetStandard() == (VPIDStandard)new_std);
		}
	}
} // ntv2vpid

void bft_marker() {}
TEST_SUITE("bft" * doctest::description("ajantv2 basic functionality tests")) {
	TEST_CASE("NTV2SegmentedXferInfo")
	{
		SUBCASE("NTV2SegmentedXferInfo Basic Test")
		{
			const NTV2SegmentedXferInfo nfo;
			CHECK(nfo.isValid() == false);
			CHECK(nfo.getElementLength() == 1);
			CHECK(nfo.getSegmentCount() == 0);
			CHECK(nfo.getSegmentLength() == 0);
			CHECK(nfo.getTotalElements() == 0);
			CHECK(nfo.getTotalBytes() == 0);
			CHECK(nfo.getSourceOffset() == 0);
			CHECK(nfo.getDestOffset() == 0);
			CHECK(nfo.isSourceTopDown() == true);
			CHECK(nfo.isSourceBottomUp() == false);
			CHECK(nfo.isDestTopDown() == true);
			CHECK(nfo.isDestBottomUp() == false);
		}

		// TEST_CASE("NTV2SegmentedXfrInfo 1920x1080 YUV8")
		// {
		//	NTV2SegmentedXferInfo nfo;
		//	nfo.setSegmentCount(64);
		//	nfo.setSegmentLength(1024); // bytes
		//	nfo.setSourcePitch(3840);	// bytes
		// }

		SUBCASE("NTV2SegmentedXfrInfo 1920x1080 NTV2_FBF_RGBA")
		{
			NTV2SegmentedXferInfo nfo;
			nfo.setSegmentCount(64);
			nfo.setSegmentLength(1024); // bytes
			nfo.setSourcePitch(7680);	// 0x1E00 bytes
			CHECK(nfo.isValid() == true);
			CHECK(nfo.getTotalBytes() == 0x00010000);	//	64K
		}
		SUBCASE("1920x1080 ARGB8 src buffer, 256x64 selection at upper-left corner 0,0")
		{	//	1920x1080 ARGB8 src buffer, 256x64 selection at upper-right corner 1664,0
			NTV2SegmentedXferInfo segInfo;
			segInfo.setSegmentCount(64);
			segInfo.setSegmentLength(1024); // bytes
			segInfo.setSourceOffset(6656);	// 0x1A00 bytes
			segInfo.setSourcePitch(7680);	// 0x1E00 bytes
			CHECK(segInfo.isValid() == true);
			CHECK(segInfo.getTotalBytes() == 0x00010000);	//	64K
		}
		SUBCASE("1920x1080 ARGB8 src buffer, 256x64 selection at upper-right corner 1664,0	")
		{
			NTV2SegmentedXferInfo segInfo;
			segInfo.setSegmentCount(64);
			segInfo.setSegmentLength(1024);		// bytes
			segInfo.setSourceOffset(1957376);	// 0x1DDE00 bytes
			segInfo.setSourcePitch(7680);		// 0x1E00 bytes
			CHECK(segInfo.isValid() == true);
			CHECK(segInfo.getTotalBytes() == 0x00010000);	//	64K
		}
		SUBCASE("1920x1080 ARGB8 src buffer, 256x64 selection at bottom-left corner 0,1016")
		{
			NTV2SegmentedXferInfo segInfo;
			segInfo.setSegmentCount(64);
			segInfo.setSegmentLength(1024);		// bytes
			segInfo.setSourceOffset(1950720);	// 0x1DC400 bytes
			segInfo.setSourcePitch(7680);		// 0x1E00 bytes
			CHECK(segInfo.isValid() == true);
			CHECK(segInfo.getTotalBytes() == 0x00010000);	//	64K
		}
		SUBCASE("1920x1080 ARGB8 src buffer, 256x64 selection at dead-center 832,508")
		{
			NTV2SegmentedXferInfo segInfo;
			segInfo.setSegmentCount(64);
			segInfo.setSegmentLength(1024);		// bytes
			segInfo.setSourceOffset(978688);	// 0xEEF00 bytes
			segInfo.setSourcePitch(7680);		// 0x1E00 bytes
			CHECK(segInfo.isValid() == true);
			CHECK(segInfo.getTotalBytes() == 0x00010000);	//	64K
		}
	}

	TEST_CASE("NTV2Buffer")
	{
		//							              1         2         3         4
		//							    01234567890123456789012345678901234567890123
		static const std::string str1 ("The rain in Spain stays mainly on the plain.");
		static const std::string str2 ("The rain in Japan stays mainly on the plain.");
		static const std::string str3 ("APWRAPWR in Spain stays mainly on WRAPWRAPWR");

		LOGNOTE("Started");
		NTV2Buffer a(AJA_NULL, 0), b(str1.c_str(), 0), c(AJA_NULL, str1.length()), d(str1.c_str(),str1.length());
		CHECK_FALSE(a);
		CHECK_FALSE(b);
		CHECK_FALSE(c);
		CHECK((bool)d);
		CHECK(a.Set(AJA_NULL, 0));
		CHECK_FALSE(b.Set(str1.c_str(), 0));
		CHECK_FALSE(c.Set(AJA_NULL, str1.length()));
		CHECK(d.Set(str1.c_str(),str1.length()));

		NTV2Buffer	spain	(str1.c_str(), str1.length());
		NTV2Buffer	japan	(str2.c_str(), str2.length());
		NTV2Buffer	wrap	(str3.c_str(), str3.length());
		ULWord		firstDiff	(0);
		ULWord		lastDiff	(0);
		CHECK(spain.GetRingChangedByteRange (japan, firstDiff, lastDiff));
		CHECK(firstDiff < lastDiff);
		CHECK_EQ(firstDiff, 12);
		CHECK_EQ(lastDiff, 15);
		CHECK(spain.GetRingChangedByteRange (wrap, firstDiff, lastDiff));
		CHECK_FALSE(firstDiff < lastDiff);
		CHECK_EQ(firstDiff, 34);
		CHECK_EQ(lastDiff, 7);
		std::string			stringOutput[5];	//	= {"", "", "", "", ""};
		std::ostringstream	ostreamOutput[5];
		//								offset	length	radix	bytes/group	groups/line	addrRadix	ascii	addrOffset
		spain.Dump(ostreamOutput[0],	0,		0,		16,		8,			8,			0,			false,	0);
		spain.Dump(ostreamOutput[1],	0,		0,		16,		4,			16,			0,			false,	0);
		spain.Dump(ostreamOutput[2],	0,		0,		16,		2,			32,			10,			true,	0);
		spain.Dump(ostreamOutput[3],	0,		0,		16,		1,			64,			16,			true,	0x10000);
		japan.Dump(ostreamOutput[4]);
		spain.Dump(stringOutput[0],		0,		0,		16,		8,			8,			0,			false,	0);
		spain.Dump(stringOutput[1],		0,		0,		16,		4,			16,			0,			false,	0);
		spain.Dump(stringOutput[2],		0,		0,		16,		2,			32,			10,			true,	0);
		spain.Dump(stringOutput[3],		0,		0,		16,		1,			64,			16,			true,	0x10000);
		japan.Dump(stringOutput[4]);

		auto CheckSizeT = [=](size_t x) {
			return x ? true : false;
		};
		// Test cast operators
		CHECK(CheckSizeT(spain));
		CHECK_FALSE(CheckSizeT(NTV2Buffer()));
		size_t sz(spain);
		CHECK_EQ(sz, spain.GetByteCount());
		sz = japan;
		CHECK_EQ(sz, japan.GetByteCount());
		sz = size_t(wrap) + 0;
		CHECK_EQ(sz, wrap.GetByteCount());
		sz = NTV2Buffer();
		CHECK_EQ(sz, 0);

		for (unsigned ndx(0);  ndx < 5;  ndx++)
			CHECK_EQ(ostreamOutput[ndx].str(),  stringOutput[ndx]);

		if (gVerboseOutput) {
			for (unsigned len(7);  len < str1.length()+2;  len+=5)
				for (unsigned n(0);  n <= str1.length()+2;  n++)
					std::cerr << spain.GetString(n, len) << std::endl;
		}

		std::vector<uint64_t> u64s;
		std::vector<uint32_t> u32s;
		std::vector<uint16_t> u16s;
		std::vector<uint8_t> u8s;
		NTV2Buffer spainCmp(spain.GetByteCount());
		CHECK(spain.GetU64s(u64s, 0, 0, true));
		CHECK_EQ(u64s.size(), 5);
		std::cerr << ULWord64Sequence(u64s) << std::endl;
		spain.Dump(std::cerr, 0, spain.GetByteCount(), 16, 8, 16, 0, false, 0);
		CHECK(spainCmp.PutU64s(u64s, 0, true));
		std::cerr << "spain: " << spain << std::endl;
		spain.Dump(std::cerr);
		std::cerr << "spainCmp: " << spainCmp << std::endl;
		spainCmp.Dump(std::cerr);
		CHECK(spainCmp.IsContentEqual(spain, 0, ULWord(u64s.size()*sizeof(uint64_t))));
		CHECK(spain.GetU64s(u64s, 0, 1));
		CHECK_EQ(u64s.size(), 1);
		CHECK(spainCmp.PutU64s(u64s, 0, true));

		CHECK(spain.GetU32s(u32s, 0, 0, true));
		CHECK_EQ(u32s.size(), 11);
		std::cerr << ULWordSequence(u32s) << std::endl;
		spain.Dump(std::cerr, 0, spain.GetByteCount(), 16, 4, 32, 0, false, 0);
		CHECK(spain.GetU32s(u32s, 0, 2));
		CHECK_EQ(u32s.size(), 2);

		CHECK(spain.GetU16s(u16s, 0, 0, true));
		CHECK_EQ(u16s.size(), 22);
		std::cerr << UWordSequence(u16s) << std::endl;
		spain.Dump(std::cerr, 0, spain.GetByteCount(), 16, 2, 64, 0, false, 0);
		CHECK(spain.GetU16s(u16s, 0, 3));
		CHECK_EQ(u16s.size(), 3);

		//	NextDifference
		ULWord byteOffset(0);
		CHECK(spain.NextDifference(japan, byteOffset));
		CHECK_EQ(byteOffset, 12);
		CHECK_EQ(spain.U8(byteOffset), 'S');
		CHECK_EQ(japan.U8(byteOffset), 'J');
		byteOffset += 4;
		CHECK(spain.NextDifference(japan, byteOffset));
		CHECK_EQ(byteOffset, 0xFFFFFFFF);	//	No other diffs

		//	Test cast-to-pointer & cast-to-size_t operators:
		const char * pConstChars = spain;
		std::string s;
		for (size_t ndx(0); ndx < size_t(spain); ndx++)
			s += pConstChars[ndx];
		CHECK_EQ(s, str1);
		const ULWord * pConstULWord = spain;
		for (size_t num(0); num < size_t(spain)/sizeof(ULWord); num++)
			CHECK_EQ(pConstULWord[num], spain.U32(int(num)));
		::memcpy(japan, spain, spain);

		//	Test scalar accessors:
		uint32_t saved(spain.U32(3));
		spain.U32(3) = 0x12345678;
		CHECK_EQ(0x12345678, spain.U32(3));
		spain.U32(3) = saved;
		CHECK_EQ(saved, spain.U32(3));
		//	Access from end:
		for (int nd(-1);  nd > (-int(str1.length()+1));  nd--)
			std::cout << char(spain.U8(nd));
		std::cout << std::endl;

		//           1         2         3         4
		// 01234567890123456789012345678901234567890123
		// The rain in Spain stays mainly on the plain.
		NTV2Buffer foo(50);
		NTV2SegmentedXferInfo segInfo;
		segInfo.setSegmentCount(4).setSegmentLength(3).setSourceOffset(2).setSourcePitch(5);
		CHECK(segInfo.isValid());
		CHECK(foo.CopyFrom(spain, segInfo));
		std::cerr << segInfo << foo << ":" << std::endl;
		foo.Dump(std::cerr, 0, 0, 16, 1, 64, 16, true, 0);

		// Check PutU64s..
		u64s.clear();
		// 16 x U64s is 128 bytes
		for (uint64_t u64(0); u64 < 16; u64++)
			u64s.push_back(u64 | 0xFEDCBA9800000000);
		foo.Allocate(128);
		foo.Fill(uint64_t(0));
		CHECK_FALSE(foo.PutU64s(u64s, 1));
		CHECK(foo.PutU64s(u64s, 0));
		foo.Allocate(256);  foo.Fill(uint64_t(0)); // 256 bytes permits up to 32 U64s
		for (size_t u64offset(0); u64offset < 17; u64offset++)
			CHECK(foo.PutU64s(u64s, u64offset));
		CHECK(foo.PutU64s(u64s, 16)); // Last U64 offset that will work
		CHECK_FALSE(foo.PutU64s(u64s, 17)); // First U64 offset that will write past end

		// Check PutU32s...
		u32s.clear();
		// 32 x U32s is 128 bytes
		for (uint32_t u32(0); u32 < 32; u32++)
			u32s.push_back(u32 | 0xFEDC0000);
		foo.Allocate(128);
		foo.Fill(uint32_t(0));
		CHECK_FALSE(foo.PutU32s(u32s, 1));
		CHECK(foo.PutU32s(u32s, 0));
		foo.Allocate(256);  foo.Fill(uint32_t(0)); // 256 bytes permits up to 64 U32s
		CHECK(foo.PutU32s(u32s, 32)); // Last U32 offset that will work
		CHECK_FALSE(foo.PutU32s(u32s, 33)); // First U32 offset that will write past end

		// Check PutU16s...
		u16s.clear();
		// 64 x U16s is 128 bytes
		for (uint16_t u16(0); u16 < 64; u16++)
			u16s.push_back(u16);
		foo.Allocate(128);
		foo.Fill(uint16_t(0));
		CHECK_FALSE(foo.PutU16s(u16s, 1));
		CHECK(foo.PutU16s(u16s, 0));
		foo.Allocate(256);  foo.Fill(uint16_t(0)); // 256 bytes permits up to 128 U16s
		CHECK(foo.PutU16s(u16s, 64)); // Last U16 offset that will work
		CHECK_FALSE(foo.PutU16s(u16s, 65)); // First U16 offset that will write past end

		// Check PutU8s...
		u8s.clear();
		// 128 x U8s is 128 bytes
		for (uint8_t u8(0);  u8 < 128;  u8++)
			u8s.push_back(u8);
		foo.Allocate(128);
		foo.Fill(uint8_t(0));
		CHECK_FALSE(foo.PutU8s(u8s, 1));
		CHECK(foo.PutU8s(u8s, 0));
		foo.Allocate(256);
		foo.Fill(uint8_t(0)); // 256 bytes permits up to 256 U8s
		CHECK(foo.PutU8s(u8s, 128)); // Last U8 offset that will work
		CHECK_FALSE(foo.PutU8s(u8s, 129)); // First U8 offset that will write past end

		// Make a "raster" of character strings of '.'s, 64 rows tall x 256 chars wide
		NTV2Buffer A(16*1024);
		A.Fill('.');
		for (ULWord row(1); row < A.GetByteCount() / 256; row++)
			CHECK(A.PutU8s(UByteSequence{'\n'}, row * 256));
		CHECK(A.PutU8s(UByteSequence{0x0}, A.GetByteCount() - 1)); // NUL terminate

		const NTV2Buffer aOrig(A); // Keep copy of original
		CHECK(A.IsContentEqual(aOrig)); // A == aOrig

		// "Blit" 5Hx128W box of X's into the "raster" of '.'s...
		NTV2Buffer X(16*1024);
		X.Fill('X');
		segInfo.reset().setSegmentInfo(5/*5 tall*/, 128 /*128 wide*/); // Xfer 128x5 box of X's
		segInfo.setSourceOffset(0).setSourcePitch(256); // From upper-left corner of X...
		segInfo.setDestInfo(8*256+64/*8 lines down, 64 chars in*/, 256/*charsPerLine*/);// ...into A @R8C64
		CHECK(segInfo.isValid());
		CHECK_EQ(segInfo.getTotalBytes(), segInfo.getTotalElements());
		CHECK_EQ(segInfo.getTotalBytes(), 128*5);
		if (gVerboseOutput) {
			std::cout << std::endl << A.GetString(0, 16*1024) << std::endl;
		}
		CHECK(A.CopyFrom (X, segInfo)); // Do the CopyBlit
		CHECK_FALSE(A.IsContentEqual(aOrig)); // A != aOrig
		CHECK(A.IsContentEqual (aOrig, /*offset*/ 0, /*byteCount*/segInfo.getSourceOffset())); // 1st part up to srcOffset same
		CHECK(A.IsContentEqual(aOrig, /*offset*/ segInfo.getDestEndOffset(), /*byteCount*/A.GetByteCount()-segInfo.getDestEndOffset()));
		std::cout << std::endl << A.GetString(0, 16*1024) << std::endl;
	}

	// TODO(paulh): Ask Mr. Bill how to test this...
	// TEST_CASE("NTV2Card Remote Device")
	// {
	// 	CNTV2Card card;
	// 	CNTV2DriverInterface & device(AsNTV2DriverInterfaceRef(card));
	// 	LOGNOTE("Legal schemes: " << aja::join(device.GetLegalSchemeNames(),", "));
	// 	device.Open("ntv2local://blabber");
	// 	device.Open("ntv2local://blabber.foo.bar");
	// 	device.Open("ntv2local://blabber/");
	// 	device.Open("ntv2local://blabber.foo.bar/");
	// 	device.Open("ntv2local://127.0.0.1");
	// 	device.Open("ntv2local://localhost");
	// 	device.Open("ntv2local://127.0.0.1/");
	// 	device.Open("ntv2local://localhost/");
	// 	device.Open("badscheme://localhost/");
	// 	device.Open("127.0.0.1");
	// 	device.Open("localhost");
	// 	device.Open("127.0.0.1:54321");
	// 	device.Open("localhost:6221");
	// 	device.Open("corvid24");
	// 	device.Open("ntv2nub://127.0.0.1");
	// 	device.Open("ntv2nub://localhost");
	// 	device.Open("ntv2nub://127.0.0.1/");
	// 	device.Open("ntv2nub://localhost/");
	// 	device.Open("ntv2local://corvid24;");
	// 	device.Open("ntv2local://corvid24:/");
	// 	device.Open("ntv2local://corvid24:4/");
	// 	device.Open("ntv2local://corvid24:4goof");
	// 	device.Open("ntv2local://corvid24:4;goof");
	// 	device.Open("ntv2local://corvid24:4/goof");
	// 	device.Open("ntv2local://kona4");
	// 	device.Open("ntv2local://0x10402100");
	// 	device.Open("ntv2local://0x12345678");
	// 	device.Open("ntv2local://0x1234567890");
	// 	device.Open("ntv2local://0x12345678:");
	// 	device.Open("ntv2local://0x12345678;");
	// 	device.Open("ntv2local://0x12345678:/");
	// 	device.Open("ntv2local://0x12345678:goof/");
	// 	device.Open("ntv2local://0x12345678:goof/query");
	// 	device.Open("ntv2://foobar:4/query");
	// 	device.Open("ntv2://ntv2-shm-dev/");
	// 	device.Open("ntv2://ntv2shmdev/");
	// 	/*
	// 	CHECK_FALSE(device.Open(""));
	// 	CHECK_FALSE(device.Open("blabber"));
	// 	CHECK(device.Open("0"));
	// 	CHECK_FALSE(device.Open("6"));
	// 	CHECK(device.Open("corvid24"));
	// 	CHECK_FALSE(device.Open("kona4"));
	// 	CHECK_FALSE(device.Open("0x10402100"));
	// 	CHECK_FALSE(device.Open("0x12345678"));
	// 	CHECK(device.Open("ntv2local://0"));
	// 	CHECK(device.Open("ntv2local://corvid24"));
	// 	CHECK_FALSE(device.Open("ntv2local://kona4"));
	// 	CHECK_FALSE(device.Open("ntv2local://0x10402100"));
	// 	CHECK_FALSE(device.Open("ntv2local://0x12345678"));

	// 	CHECK(device.Open("ntv2local://0:"));
	// 	CHECK(device.Open("ntv2local://0:25"));
	// 	CHECK(device.Open("ntv2local://0:25/"));
	// 	CHECK(device.Open("ntv2local://0:25/query"));
	// 	CHECK_FALSE(device.Open("ntv2local://0:25goof"));
	// 	CHECK_FALSE(device.Open("ntv2local://0:goof"));
	// 	CHECK_FALSE(device.Open("ntv2local://0goof"));

	// 	CHECK_FALSE(device.Open("ntv2local://corvid26"));
	// 	CHECK_FALSE(device.Open("ntv2local://corvid24;"));
	// 	CHECK_FALSE(device.Open("ntv2local://corvid24:/"));
	// 	CHECK(device.Open("ntv2local://corvid24:4/"));
	// 	CHECK_FALSE(device.Open("ntv2local://corvid24:4goof"));
	// 	CHECK_FALSE(device.Open("ntv2local://corvid24:4;goof"));
	// 	CHECK(device.Open("ntv2local://corvid24:4/goof"));
	// 	CHECK_FALSE(device.Open("ntv2local://kona4"));
	// 	CHECK_FALSE(device.Open("ntv2local://0x10402100"));
	// 	CHECK(device.Open("ntv2local://0x12345678"));
	// 	CHECK_FALSE(device.Open("ntv2local://0x1234567890"));
	// 	CHECK_FALSE(device.Open("ntv2local://0x12345678:"));
	// 	CHECK_FALSE(device.Open("ntv2local://0x12345678;"));
	// 	CHECK_FALSE(device.Open("ntv2local://0x12345678:/"));
	// 	CHECK_FALSE(device.Open("ntv2local://0x12345678:goof/"));
	// 	CHECK_FALSE(device.Open("ntv2local://0x12345678:goof/query"));
	// 	*/
	// }

	TEST_CASE("Copy Raster")
	{
		LOGNOTE("Started");

		//	Test CopyRaster
		const UWord		nDstWidthPixels		(32);
		const UWord		nDstHeightLines		(32);
		const UWord		nDstBytesPerLine	(nDstWidthPixels * 2);
		const UWord		nDstBytes			(nDstBytesPerLine * nDstHeightLines);
		NTV2Buffer		dstRaster			(nDstBytes);
		UByte *			pDstRaster			(reinterpret_cast<UByte*>(dstRaster.GetHostPointer()));
		dstRaster.Fill(uint8_t(0xAA));

		const UWord		nSrcWidthPixels		(16);
		const UWord		nSrcHeightLines		(16);
		const UWord		nSrcBytesPerLine	(nSrcWidthPixels * 2);
		const UWord		nSrcBytes			(nSrcBytesPerLine * nSrcHeightLines);
		NTV2Buffer		srcRaster			(nSrcBytes);
		UByte *			pSrcRaster			(reinterpret_cast<UByte*>(srcRaster.GetHostPointer()));
		srcRaster.Fill(UByte(0xBB));
		if (gVerboseOutput)	{std::cerr << "SrcRaster:" << std::endl;  srcRaster.Dump(std::cerr, 0/*byteOffset*/, 0/*byteCount*/, 16/*radix*/, 2/*bytes/group*/, nSrcWidthPixels/*groups/line*/, 16/*addrRadix*/);}
		if (gVerboseOutput)	{std::cerr << "DstRaster:" << std::endl;  dstRaster.Dump(std::cerr, 0/*byteOffset*/, 0/*byteCount*/, 16/*radix*/, 2/*bytes/group*/, nDstWidthPixels/*groups/line*/, 16/*addrRadix*/);}
		//				CopyRaster (NTV2FrameBufferFormat, pDstRaster, dstBytesPerLine,  dstTotalLines,  dstVertLineOffset, dstHorzPixelOffset, pSrcRaster, srcBytesPerLine,  srcTotalLines,  srcVertLineOffset, srcVertLinesToCopy, srcHorzPixelOffset, srcHorzPixelsToCopy)
		CHECK_FALSE(CopyRaster (NTV2_FBF_10BIT_YCBCR, pDstRaster, nDstBytesPerLine, nDstHeightLines,                2,                  4, pSrcRaster, nSrcBytesPerLine, nSrcHeightLines,                0,    nSrcHeightLines,                  0, nSrcWidthPixels));
		CHECK_FALSE(CopyRaster ( NTV2_FBF_8BIT_YCBCR, pDstRaster, nDstBytesPerLine, nDstHeightLines,                2,                  4, pDstRaster, nSrcBytesPerLine, nSrcHeightLines,                0,    nSrcHeightLines,                  0, nSrcWidthPixels));
		CHECK_FALSE(CopyRaster ( NTV2_FBF_8BIT_YCBCR, pDstRaster, nDstBytesPerLine, nDstHeightLines,                2,                  4, NULL,       nSrcBytesPerLine, nSrcHeightLines,                0,    nSrcHeightLines,                  0, nSrcWidthPixels));
		CHECK_FALSE(CopyRaster ( NTV2_FBF_8BIT_YCBCR,       NULL, nDstBytesPerLine, nDstHeightLines,                2,                  4, pSrcRaster, nSrcBytesPerLine, nSrcHeightLines,                0,    nSrcHeightLines,                  0, nSrcWidthPixels));

		//	Blit src into dst at vert offset 2, horz offset 4
		CHECK(CopyRaster ( NTV2_FBF_8BIT_YCBCR, pDstRaster, nDstBytesPerLine, nDstHeightLines,                2,                  4, pSrcRaster, nSrcBytesPerLine, nSrcHeightLines,                0,    nSrcHeightLines,                  0, nSrcWidthPixels));
		if (gVerboseOutput)	{std::cerr << "DstRaster:" << std::endl;  dstRaster.Dump(std::cerr, 0/*byteOffset*/, 0/*byteCount*/, 16/*radix*/, 2/*bytes/group*/, nDstWidthPixels/*groups/line*/, 16/*addrRadix*/);}
		dstRaster.Fill(uint8_t(0xAA));	//	::memset (pDstRaster, 0xAA, nDstBytes);

		//	Blit lines 1,2,3,4 from src into dst at vert offset 2, horz offset 4
		::memset (pSrcRaster + 0 * nSrcBytesPerLine, 0x00, nSrcBytesPerLine);
		::memset (pSrcRaster + 1 * nSrcBytesPerLine, 0x11, nSrcBytesPerLine);
		::memset (pSrcRaster + 2 * nSrcBytesPerLine, 0x22, nSrcBytesPerLine);
		::memset (pSrcRaster + 3 * nSrcBytesPerLine, 0x33, nSrcBytesPerLine);
		::memset (pSrcRaster + 4 * nSrcBytesPerLine, 0x44, nSrcBytesPerLine);
		if (gVerboseOutput)	{std::cerr << "SrcRaster:" << std::endl;  srcRaster.Dump(std::cerr, 0/*byteOffset*/, 0/*byteCount*/, 16/*radix*/, 2/*bytes/group*/, nSrcWidthPixels/*groups/line*/, 16/*addrRadix*/);}
		CHECK(CopyRaster ( NTV2_FBF_8BIT_YCBCR, pDstRaster, nDstBytesPerLine, nDstHeightLines,                2,                  4, pSrcRaster, nSrcBytesPerLine, nSrcHeightLines,                1,                  4,                  0, nSrcWidthPixels));
		if (gVerboseOutput)	{std::cerr << "DstRaster:" << std::endl;  dstRaster.Dump(std::cerr, 0/*byteOffset*/, 0/*byteCount*/, 16/*radix*/, 2/*bytes/group*/, nDstWidthPixels/*groups/line*/, 16/*addrRadix*/);}
		dstRaster.Fill(uint8_t(0xAA));	//	::memset (pDstRaster, 0xAA, nDstBytes);

		//	Ask to grab pixels past right edge of src raster -- Blit lines 1,2,3,4 from src into dst at vert offset 2, horz offset 4
		CHECK(CopyRaster ( NTV2_FBF_8BIT_YCBCR, pDstRaster, nDstBytesPerLine, nDstHeightLines,                2,                  4, pSrcRaster, nSrcBytesPerLine, nSrcHeightLines,                1,                  4,                  2, nSrcWidthPixels));
		if (gVerboseOutput)	{std::cerr << "DstRaster:" << std::endl;  dstRaster.Dump(std::cerr, 0/*byteOffset*/, 0/*byteCount*/, 16/*radix*/, 2/*bytes/group*/, nDstWidthPixels/*groups/line*/, 16/*addrRadix*/);}
		dstRaster.Fill(uint8_t(0xAA));	//	::memset (pDstRaster, 0xAA, nDstBytes);
		//	Ask to grab 14 pixels past right edge of src raster -- Blit lines 1,2,3,4 from src into dst at vert offset 2, horz offset 4
		CHECK(CopyRaster ( NTV2_FBF_8BIT_YCBCR, pDstRaster, nDstBytesPerLine, nDstHeightLines,                2,                  4, pSrcRaster, nSrcBytesPerLine, nSrcHeightLines,                1,                  4,                 14, nSrcWidthPixels));
		if (gVerboseOutput)	{std::cerr << "DstRaster:" << std::endl;  dstRaster.Dump(std::cerr, 0/*byteOffset*/, 0/*byteCount*/, 16/*radix*/, 2/*bytes/group*/, nDstWidthPixels/*groups/line*/, 16/*addrRadix*/);}
		dstRaster.Fill(uint8_t(0xAA));	//	::memset (pDstRaster, 0xAA, nDstBytes);
		//	Request all pixels past right edge of src raster -- fail
		CHECK_FALSE (CopyRaster ( NTV2_FBF_8BIT_YCBCR, pDstRaster, nDstBytesPerLine, nDstHeightLines,                2,                  4, pSrcRaster, nSrcBytesPerLine, nSrcHeightLines,                1,                  4,                 16, nSrcWidthPixels));
		if (gVerboseOutput)	{std::cerr << "DstRaster:" << std::endl;  dstRaster.Dump(std::cerr, 0/*byteOffset*/, 0/*byteCount*/, 16/*radix*/, 2/*bytes/group*/, nDstWidthPixels/*groups/line*/, 16/*addrRadix*/);}
		dstRaster.Fill(uint8_t(0xAA));	//	::memset (pDstRaster, 0xAA, nDstBytes);

		//	Ask to grab lines past bottom edge of src raster -- Blit last 4 lines from src into dst at vert offset 2, horz offset 4
		srcRaster.Fill(UByte(0xBB));	//	::memset (pSrcRaster, 0xBB, nSrcBytes);
		::memset (pSrcRaster + 11 * nSrcBytesPerLine, 0x11, nSrcBytesPerLine);
		::memset (pSrcRaster + 12 * nSrcBytesPerLine, 0x22, nSrcBytesPerLine);
		::memset (pSrcRaster + 13 * nSrcBytesPerLine, 0x33, nSrcBytesPerLine);
		::memset (pSrcRaster + 14 * nSrcBytesPerLine, 0x44, nSrcBytesPerLine);
		::memset (pSrcRaster + 15 * nSrcBytesPerLine, 0x55, nSrcBytesPerLine);
		if (gVerboseOutput)	{std::cerr << "SrcRaster:" << std::endl;  srcRaster.Dump(std::cerr, 0/*byteOffset*/, 0/*byteCount*/, 16/*radix*/, 2/*bytes/group*/, nSrcWidthPixels/*groups/line*/, 16/*addrRadix*/);}
		CHECK(CopyRaster ( NTV2_FBF_8BIT_YCBCR, pDstRaster, nDstBytesPerLine, nDstHeightLines,                2,                  4, pSrcRaster, nSrcBytesPerLine, nSrcHeightLines,               12,                  4,                  0, nSrcWidthPixels));
		if (gVerboseOutput)	{std::cerr << "DstRaster:" << std::endl;  dstRaster.Dump(std::cerr, 0/*byteOffset*/, 0/*byteCount*/, 16/*radix*/, 2/*bytes/group*/, nDstWidthPixels/*groups/line*/, 16/*addrRadix*/);}
		dstRaster.Fill(uint8_t(0xAA));	//	::memset (pDstRaster, 0xAA, nDstBytes);
		//	Ask to grab lines past bottom edge of src raster -- Blit last 4 lines from src into dst at vert offset 2, horz offset 4
		CHECK(CopyRaster ( NTV2_FBF_8BIT_YCBCR, pDstRaster, nDstBytesPerLine, nDstHeightLines,                2,                  4, pSrcRaster, nSrcBytesPerLine, nSrcHeightLines,               12,    nSrcHeightLines,                  0, nSrcWidthPixels));
		if (gVerboseOutput)	{std::cerr << "DstRaster:" << std::endl;  dstRaster.Dump(std::cerr, 0/*byteOffset*/, 0/*byteCount*/, 16/*radix*/, 2/*bytes/group*/, nDstWidthPixels/*groups/line*/, 16/*addrRadix*/);}
		dstRaster.Fill(uint8_t(0xAA));	//	::memset (pDstRaster, 0xAA, nDstBytes);
		//	Blit past bottom of dst raster -- Blit src into dst at vert offset 30, horz offset 4
		CHECK(CopyRaster ( NTV2_FBF_8BIT_YCBCR, pDstRaster, nDstBytesPerLine, nDstHeightLines,               30,                  4, pSrcRaster, nSrcBytesPerLine, nSrcHeightLines,               12,    nSrcHeightLines,                  0, nSrcWidthPixels));
		if (gVerboseOutput)	{std::cerr << "DstRaster:" << std::endl;  dstRaster.Dump(std::cerr, 0/*byteOffset*/, 0/*byteCount*/, 16/*radix*/, 2/*bytes/group*/, nDstWidthPixels/*groups/line*/, 16/*addrRadix*/);}
		dstRaster.Fill(uint8_t(0xAA));	//	::memset (pDstRaster, 0xAA, nDstBytes);
	}

	TEST_CASE("NTV2Debug")
	{
		{
			const NTV2DeviceID		deviceIDs []	= {	DEVICE_ID_NOTFOUND,
														DEVICE_ID_CORVID1,					DEVICE_ID_CORVID22,				DEVICE_ID_CORVID24,			DEVICE_ID_CORVID3G,
														DEVICE_ID_CORVID44,					DEVICE_ID_CORVID88,				DEVICE_ID_CORVIDHBR,		DEVICE_ID_CORVIDHEVC,
														DEVICE_ID_IO4K,						DEVICE_ID_IO4KPLUS,				DEVICE_ID_IO4KUFC,			DEVICE_ID_IOEXPRESS,
														DEVICE_ID_IOIP_2022,				DEVICE_ID_IOIP_2110,			DEVICE_ID_IOXT,
														DEVICE_ID_KONA1,					DEVICE_ID_KONA3G,				DEVICE_ID_KONA3GQUAD,		DEVICE_ID_KONA4,
														DEVICE_ID_KONA4UFC,					DEVICE_ID_KONAHDMI,
														DEVICE_ID_KONAIP_1RX_1TX_1SFP_J2K,	DEVICE_ID_KONAIP_1RX_1TX_2110,	DEVICE_ID_KONAIP_2022,		DEVICE_ID_KONAIP_2110,
														DEVICE_ID_KONAIP_2TX_1SFP_J2K,		DEVICE_ID_KONAIP_4CH_2SFP,
														DEVICE_ID_KONALHEPLUS,				DEVICE_ID_KONALHI,				DEVICE_ID_KONALHIDVI,		DEVICE_ID_TTAP,
														NTV2DeviceID(1234567)	};
			const std::string			devIDStrs []	= {	"DEVICE_ID_NOTFOUND",
														"DEVICE_ID_CORVID1",				"DEVICE_ID_CORVID22",			"DEVICE_ID_CORVID24",		"DEVICE_ID_CORVID3G",
														"DEVICE_ID_CORVID44",				"DEVICE_ID_CORVID88",			"DEVICE_ID_CORVIDHBR",		"DEVICE_ID_CORVIDHEVC",
														"DEVICE_ID_IO4K",					"DEVICE_ID_IO4KPLUS",			"DEVICE_ID_IO4KUFC",		"DEVICE_ID_IOEXPRESS",
														"DEVICE_ID_IOIP_2022",				"DEVICE_ID_IOIP_2110",			"DEVICE_ID_IOXT",
														"DEVICE_ID_KONA1",					"DEVICE_ID_KONA3G",				"DEVICE_ID_KONA3GQUAD",		"DEVICE_ID_KONA4",
														"DEVICE_ID_KONA4UFC",				"DEVICE_ID_KONAHDMI",
														"DEVICE_ID_KONAIP_1RX_1TX_1SFP_J2K","DEVICE_ID_KONAIP_1RX_1TX_2110","DEVICE_ID_KONAIP_2022",	"DEVICE_ID_KONAIP_2110",
														"DEVICE_ID_KONAIP_2TX_1SFP_J2K",	"DEVICE_ID_KONAIP_4CH_2SFP",
														"DEVICE_ID_KONALHEPLUS",			"DEVICE_ID_KONALHI",			"DEVICE_ID_KONALHIDVI",		"DEVICE_ID_TTAP",
														""						};
			const std::string			deviceStrs []	= {	"Unknown",
														"Corvid1",							"Corvid22",						"Corvid24",					"Corvid3G",
														"Corvid44",							"Corvid88",						"CorvidHBR",				"CorvidHEVC",
														"Io4K",								"DNxIV",						"Io4KUfc",					"IoExpress",
														"DNxIP_2022",						"DNxIP_2110",					"IoXT",
														"Kona1",							"Kona3G",						"Kona3GQuad",				"Kona4",
														"Kona4Ufc",							"KonaHDMI",
														"KonaIP_1Rx1Tx1SFPJ2K",				"KonaIP_1Rx1Tx2110",			"KonaIP_2022",				"KonaIP_2110",
														"KonaIP_2Tx1SFPJ2K",				"KonaIP_4ch2SFP",
														"KonaLHePlus",						"KonaLHi",						"KonaLHiDVI",				"TTap",
														""						};
			for (unsigned ndx (0);  ndx < sizeof (deviceIDs) / sizeof (NTV2DeviceID);  ndx++)
			{
				CHECK(::NTV2DeviceIDString (deviceIDs [ndx]) != NULL);	//	never NULL!
				CHECK_EQ(std::string (::NTV2DeviceIDString (deviceIDs [ndx])), devIDStrs [ndx]);
				CHECK_EQ(std::string (::NTV2DeviceString (deviceIDs [ndx])), deviceStrs [ndx]);
			}
		}
		{
			const NTV2Standard		standards []	= {	NTV2_STANDARD_1080,			NTV2_STANDARD_720,		NTV2_STANDARD_525,			NTV2_STANDARD_625,
														NTV2_STANDARD_1080p,		NTV2_STANDARD_2K,		NTV2_NUM_STANDARDS,			NTV2_STANDARD_UNDEFINED,
														NTV2_STANDARD_INVALID		};
			const std::string			stdStrs []		= {	"NTV2_STANDARD_1080",		"NTV2_STANDARD_720",	"NTV2_STANDARD_525",		"NTV2_STANDARD_625",
														"NTV2_STANDARD_1080p",		"NTV2_STANDARD_2K",		"NTV2_STANDARD_INVALID",	"NTV2_STANDARD_INVALID",
														"NTV2_STANDARD_INVALID"	};
			const bool				valid []	= 	{	true,						true,					true,						true,
														true,						true,					false,						false,
														false					};
			const bool				isProg []	= 	{	false,						true,					false,						false,
														true,						false,					false,						false,
														false					};
			for (unsigned ndx (0);  ndx < sizeof (standards) / sizeof (NTV2Standard);  ndx++)
			{
				CHECK_EQ(NTV2_IS_VALID_STANDARD (standards [ndx]), valid [ndx]);
				CHECK(::NTV2StandardString (standards [ndx]) != NULL);	//	never NULL!
				CHECK_EQ(std::string (::NTV2StandardString (standards [ndx])), stdStrs [ndx]);
				CHECK_EQ(NTV2_IS_PROGRESSIVE_STANDARD (standards [ndx]), isProg [ndx]);
			}
		}
		{
			const NTV2FrameBufferFormat	fbfs []	= {	NTV2_FBF_10BIT_YCBCR,		NTV2_FBF_8BIT_YCBCR,			NTV2_FBF_ARGB,					NTV2_FBF_RGBA,
													NTV2_FBF_10BIT_RGB,			NTV2_FBF_8BIT_YCBCR_YUY2,		NTV2_FBF_ABGR,					NTV2_FBF_10BIT_DPX,
													NTV2_FBF_10BIT_YCBCR_DPX,	NTV2_FBF_8BIT_DVCPRO,			NTV2_FBF_8BIT_YCBCR_420PL3,		NTV2_FBF_8BIT_HDV,
													NTV2_FBF_24BIT_RGB,			NTV2_FBF_24BIT_BGR,				NTV2_FBF_10BIT_YCBCRA,			NTV2_FBF_10BIT_DPX_LE,
													NTV2_FBF_48BIT_RGB,			NTV2_FBF_12BIT_RGB_PACKED,		NTV2_FBF_PRORES_DVCPRO,			NTV2_FBF_PRORES_HDV,
													NTV2_FBF_10BIT_RGB_PACKED,	NTV2_FBF_10BIT_ARGB,			NTV2_FBF_16BIT_ARGB,			NTV2_FBF_10BIT_RAW_RGB,
													NTV2_FBF_10BIT_RAW_YCBCR,	NTV2_FBF_10BIT_YCBCR_420PL2,	NTV2_FBF_10BIT_YCBCR_422PL2,	NTV2_FBF_8BIT_YCBCR_420PL2,
													NTV2_FBF_8BIT_YCBCR_422PL2,	NTV2_FBF_8BIT_YCBCR_422PL3,		NTV2_FBF_10BIT_YCBCR_420PL3_LE,	NTV2_FBF_10BIT_YCBCR_422PL3_LE,
													NTV2_FBF_INVALID	};
			const std::string			fbfStrs []	= {	"NTV2_FBF_10BIT_YCBCR",			"NTV2_FBF_8BIT_YCBCR",			"NTV2_FBF_ARGB",					"NTV2_FBF_RGBA",
													"NTV2_FBF_10BIT_RGB",			"NTV2_FBF_8BIT_YCBCR_YUY2",		"NTV2_FBF_ABGR",					"NTV2_FBF_10BIT_DPX",
													"NTV2_FBF_10BIT_YCBCR_DPX",		"NTV2_FBF_8BIT_DVCPRO",			"NTV2_FBF_8BIT_YCBCR_420PL3",		"NTV2_FBF_8BIT_HDV",
													"NTV2_FBF_24BIT_RGB",			"NTV2_FBF_24BIT_BGR",			"NTV2_FBF_10BIT_YCBCRA",			"NTV2_FBF_10BIT_DPX_LE",
													"NTV2_FBF_48BIT_RGB",			"NTV2_FBF_12BIT_RGB_PACKED",	"NTV2_FBF_PRORES_DVCPRO",			"NTV2_FBF_PRORES_HDV",
													"NTV2_FBF_10BIT_RGB_PACKED",	"NTV2_FBF_10BIT_ARGB",			"NTV2_FBF_16BIT_ARGB",				"NTV2_FBF_10BIT_RAW_RGB",
													"NTV2_FBF_10BIT_RAW_YCBCR",		"NTV2_FBF_10BIT_YCBCR_420PL2",	"NTV2_FBF_10BIT_YCBCR_422PL2",		"NTV2_FBF_8BIT_YCBCR_420PL2",
													"NTV2_FBF_8BIT_YCBCR_422PL2",	"NTV2_FBF_8BIT_YCBCR_422PL3",	"NTV2_FBF_10BIT_YCBCR_420PL3_LE",	"NTV2_FBF_10BIT_YCBCR_422PL3_LE",
													"NTV2_FBF_INVALID"	};
			const bool				valid []	= {	true,							true,							true,							true,
													true,							true,							true,							true,
													true,							true,							true,							true,
													true,							true,							true,							true,
													true,							true,							true,							true,
													true,							true,							true,							true,
													true,							true,							true,							true,
													true,							true,							true,							true,
													false		};
			const bool				isPlanar []	= {	false,							false,							false,							false,
													false,							false,							false,							false,
													false,							false,							true,							false,
													false,							false,							false,							false,
													false,							false,							false,							false,
													false,							false,							false,							false,
													false,							true,							true,							true,
													true,							true,							true,							true,
													false		};
	//		const UWord				numPlanes [] = {	0,								0,								0,								0,
	//													0,								0,								0,								0,
	//													0,								0,								3,								0,
	//													0,								0,								0,								0,
	//													0,								0,								0,								0,
	//													0,								0,								0,								0,
	//													0,								2,								2,								2,
	//													2,								3,								3,								3,
	//												0		};
			const bool				isProRes []	= {	false,							false,							false,							false,
													false,							false,							false,							false,
													false,							false,							false,							false,
													false,							false,							false,							false,
													false,							false,							true,							true,
													false,							false,							false,							false,
													false,							false,							false,							false,
													false,							false,							false,							false,
													false		};
			const bool				isRGB []	= {	false,							false,							true,							true,//3
													true,							false,							true,							true,//7
													false,							false,							false,							false,//11
													true,							true,							false,							true,//15
													true,							true,							false,							false,//19
													true,							true,							true,							true,
													false,							false,							false,							false,
													false,							false,							false,							false,
													false		};
			const bool				hasAlpha []	= {	false,							false,							true,							true,//3
													false,							false,							true,							false,//7
													false,							false,							false,							false,//11
													false,							false,							true,							false,//15
													false,							false,							false,							false,//19
													false,							true,							true,							false,
													false,							false,							false,							false,
													false,							false,							false,							false,
													false		};
			const bool				isRaw []	= {	false,							false,							false,							false,//3
													false,							false,							false,							false,//7
													false,							false,							false,							false,//11
													false,							false,							false,							false,//15
													false,							false,							false,							false,//19
													false,							false,							false,							true,
													true,							false,							false,							false,
													false,							false,							false,							false,
													false		};
			for (unsigned ndx(0);  ndx < sizeof(fbfs) / sizeof(NTV2FrameBufferFormat);  ndx++)
			{	const NTV2PixelFormat pf(fbfs[ndx]);
				std::cerr << ndx << ": " << ::NTV2FrameBufferFormatToString(pf) << std::endl;
				CHECK_EQ(NTV2_IS_VALID_FRAME_BUFFER_FORMAT(pf), valid[ndx]);
				CHECK(::NTV2FrameBufferFormatString(pf) != NULL);	//	never NULL!
				CHECK_EQ(std::string(::NTV2FrameBufferFormatString(pf)), fbfStrs[ndx]);
				CHECK_EQ(NTV2_IS_FBF_PLANAR(pf), isPlanar[ndx]);
				CHECK_EQ(NTV2_IS_FBF_PRORES(pf), isProRes[ndx]);
				CHECK_EQ(NTV2_IS_FBF_RGB(pf), isRGB[ndx]);
				CHECK_EQ(NTV2_FBF_HAS_ALPHA(pf), hasAlpha[ndx]);
				CHECK_EQ(NTV2_FBF_IS_RAW(pf), isRaw[ndx]);
			}
		}
		{
			const NTV2FrameGeometry	geometries []	= {	NTV2_FG_1920x1080,		NTV2_FG_1280x720,		NTV2_FG_720x486,	NTV2_FG_720x576,
														NTV2_FG_1920x1114,		NTV2_FG_2048x1114,		NTV2_FG_720x508,	NTV2_FG_720x598,
														NTV2_FG_1920x1112,		NTV2_FG_1280x740,		NTV2_FG_2048x1080,	NTV2_FG_2048x1556,
														NTV2_FG_2048x1588,		NTV2_FG_2048x1112,		NTV2_FG_720x514,	NTV2_FG_720x612,
														NTV2_FG_4x1920x1080,	NTV2_FG_4x2048x1080,	NTV2_FG_INVALID		};
			const std::string			geomStrs []		= {	"NTV2_FG_1920x1080",	"NTV2_FG_1280x720",		"NTV2_FG_720x486",		"NTV2_FG_720x576",
														"NTV2_FG_1920x1114",	"NTV2_FG_2048x1114",	"NTV2_FG_720x508",		"NTV2_FG_720x598",
														"NTV2_FG_1920x1112",	"NTV2_FG_1280x740",		"NTV2_FG_2048x1080",	"NTV2_FG_2048x1556",
														"NTV2_FG_2048x1588",	"NTV2_FG_2048x1112",	"NTV2_FG_720x514",		"NTV2_FG_720x612",
														"NTV2_FG_4x1920x1080",	"NTV2_FG_4x2048x1080",	"NTV2_FG_INVALID"		};
			const bool				valid []	= 	{	true,					true,					true,		true,
														true,					true,					true,		true,
														true,					true,					true,		true,
														true,					true,					true,		true,
														true,					true,					false				};
			const bool				isQuad []	= 	{	false,					false,					false,		false,
														false,					false,					false,		false,
														false,					false,					false,		false,
														false,					false,					false,		false,
														true,					true,					false				};
			const bool				is2K1080 []	= 	{	false,					false,					false,		false,
														false,					true,					false,		false,
														false,					false,					true,		false,
														false,					true,					false,		false,
														false,					false,					false				};
			for (unsigned ndx (0);  ndx < sizeof (geometries) / sizeof (NTV2FrameGeometry);  ndx++)
			{
				CHECK_EQ(NTV2_IS_VALID_NTV2FrameGeometry (geometries [ndx]), valid [ndx]);
				CHECK(::NTV2FrameGeometryString (geometries [ndx]) != NULL);	//	never NULL!
				CHECK_EQ(std::string (::NTV2FrameGeometryString (geometries [ndx])), geomStrs [ndx]);
				CHECK_EQ(NTV2_IS_QUAD_FRAME_GEOMETRY (geometries [ndx]), isQuad [ndx]);
				CHECK_EQ(NTV2_IS_2K_1080_FRAME_GEOMETRY (geometries [ndx]), is2K1080 [ndx]);
			}
		}
		{
			const NTV2FrameRate		rates []		= {	NTV2_FRAMERATE_12000,		NTV2_FRAMERATE_11988,		NTV2_FRAMERATE_6000,		NTV2_FRAMERATE_5994,
														NTV2_FRAMERATE_5000,		NTV2_FRAMERATE_4800,		NTV2_FRAMERATE_4795,		NTV2_FRAMERATE_3000,
														NTV2_FRAMERATE_2997,		NTV2_FRAMERATE_2500,		NTV2_FRAMERATE_2400,		NTV2_FRAMERATE_2398,
														NTV2_FRAMERATE_1500,		NTV2_FRAMERATE_1498,		NTV2_FRAMERATE_INVALID		};
			const std::string			rateStrs []		= {	"NTV2_FRAMERATE_12000",		"NTV2_FRAMERATE_11988",		"NTV2_FRAMERATE_6000",		"NTV2_FRAMERATE_5994",
														"NTV2_FRAMERATE_5000",		"NTV2_FRAMERATE_4800",		"NTV2_FRAMERATE_4795",		"NTV2_FRAMERATE_3000",
														"NTV2_FRAMERATE_2997",		"NTV2_FRAMERATE_2500",		"NTV2_FRAMERATE_2400",		"NTV2_FRAMERATE_2398",
														"NTV2_FRAMERATE_1500",		"NTV2_FRAMERATE_1498",		"NTV2_FRAMERATE_INVALID"	};
			const bool				valid []	= 	{	true,						true,						true,						true,
														true,						true,						true,						true,
														true,						true,						true,						true,
														true,						true,						false						};
			for (unsigned ndx (0);  ndx < sizeof (rates) / sizeof (NTV2FrameRate);  ndx++)
			{
				CHECK_EQ(NTV2_IS_VALID_NTV2FrameRate (rates [ndx]), valid [ndx]);
				CHECK(::NTV2FrameRateString (rates [ndx]) != NULL);	//	never NULL!
				CHECK_EQ(std::string (::NTV2FrameRateString (rates [ndx])), rateStrs [ndx]);
			}
		}
		{
			const NTV2VideoFormat	formats []		= {	NTV2_FORMAT_1080i_5000,											NTV2_FORMAT_1080i_5994,											NTV2_FORMAT_1080i_6000,
														NTV2_FORMAT_720p_5994,			NTV2_FORMAT_720p_6000,			NTV2_FORMAT_1080psf_2398,		NTV2_FORMAT_1080psf_2400,
														NTV2_FORMAT_1080p_2997,			NTV2_FORMAT_1080p_3000,			NTV2_FORMAT_1080p_2500,			NTV2_FORMAT_1080p_2398,
														NTV2_FORMAT_1080p_2400,			NTV2_FORMAT_1080p_2K_2398,		NTV2_FORMAT_1080p_2K_2400,		NTV2_FORMAT_1080p_2K_2500,
														NTV2_FORMAT_1080p_2K_2997,		NTV2_FORMAT_1080p_2K_3000,
														NTV2_FORMAT_1080p_2K_4795_A,	NTV2_FORMAT_1080p_2K_4800_A,	NTV2_FORMAT_1080p_2K_5000_A,	NTV2_FORMAT_1080p_2K_5994_A,	NTV2_FORMAT_1080p_2K_6000_A,
														NTV2_FORMAT_1080p_2K_4795_B,	NTV2_FORMAT_1080p_2K_4800_B,	NTV2_FORMAT_1080p_2K_5000_B,	NTV2_FORMAT_1080p_2K_5994_B,	NTV2_FORMAT_1080p_2K_6000_B,
														NTV2_FORMAT_1080psf_2K_2398,	NTV2_FORMAT_1080psf_2K_2400,	NTV2_FORMAT_1080psf_2K_2500,	NTV2_FORMAT_1080psf_2500_2,		NTV2_FORMAT_1080psf_2997_2,
														NTV2_FORMAT_1080psf_3000_2,		NTV2_FORMAT_720p_5000,			NTV2_FORMAT_1080p_5000_B,		NTV2_FORMAT_1080p_5000_A,		NTV2_FORMAT_1080p_5994_B,//
														NTV2_FORMAT_1080p_6000_B,		NTV2_FORMAT_1080p_6000_B,		NTV2_FORMAT_1080p_5000_A,		NTV2_FORMAT_1080p_5994_A,		NTV2_FORMAT_1080p_6000_A,
														NTV2_FORMAT_END_HIGH_DEF_FORMATS,
														NTV2_FORMAT_720p_2398,			NTV2_FORMAT_720p_2500,			NTV2_FORMAT_525_5994,			NTV2_FORMAT_625_5000,
														NTV2_FORMAT_525_2398,			NTV2_FORMAT_525_2400,			NTV2_FORMAT_525psf_2997,		NTV2_FORMAT_625psf_2500,
														NTV2_FORMAT_END_STANDARD_DEF_FORMATS,
														NTV2_FORMAT_2K_1498,			NTV2_FORMAT_2K_1500,			NTV2_FORMAT_2K_2398,			NTV2_FORMAT_2K_2400,			NTV2_FORMAT_2K_2500,
														NTV2_FORMAT_END_2K_DEF_FORMATS,
														NTV2_FORMAT_4x1920x1080psf_2398,	NTV2_FORMAT_4x1920x1080psf_2400,	NTV2_FORMAT_4x1920x1080psf_2500,	NTV2_FORMAT_4x1920x1080psf_2997,
														NTV2_FORMAT_4x1920x1080psf_3000,	NTV2_FORMAT_4x1920x1080p_2398,		NTV2_FORMAT_4x1920x1080p_2400,		NTV2_FORMAT_4x1920x1080p_2500,
														NTV2_FORMAT_4x1920x1080p_2997,		NTV2_FORMAT_4x1920x1080p_3000,		NTV2_FORMAT_4x2048x1080psf_2398,	NTV2_FORMAT_4x2048x1080psf_2400,
														NTV2_FORMAT_4x2048x1080psf_2500,	NTV2_FORMAT_4x2048x1080psf_2997,	NTV2_FORMAT_4x2048x1080psf_3000,	NTV2_FORMAT_4x2048x1080p_2398,
														NTV2_FORMAT_4x2048x1080p_2400,		NTV2_FORMAT_4x2048x1080p_2500,		NTV2_FORMAT_4x2048x1080p_2997,		NTV2_FORMAT_4x2048x1080p_3000,
														NTV2_FORMAT_4x2048x1080p_4795,		NTV2_FORMAT_4x2048x1080p_4800,		NTV2_FORMAT_4x1920x1080p_5000,		NTV2_FORMAT_4x1920x1080p_5994,
														NTV2_FORMAT_4x1920x1080p_6000,		NTV2_FORMAT_4x2048x1080p_5000,		NTV2_FORMAT_4x2048x1080p_5994,		NTV2_FORMAT_4x2048x1080p_6000,
														NTV2_FORMAT_4x2048x1080p_11988,		NTV2_FORMAT_4x2048x1080p_12000,
														NTV2_FORMAT_END_HIGH_DEF_FORMATS2,
														NTV2_FORMAT_UNKNOWN	};
			const std::string			fmtStrs []		= {	"NTV2_FORMAT_1080i_5000",				"NTV2_FORMAT_1080i_5994",				"NTV2_FORMAT_1080i_6000",
														"NTV2_FORMAT_720p_5994",		"NTV2_FORMAT_720p_6000",		"NTV2_FORMAT_1080psf_2398",		"NTV2_FORMAT_1080psf_2400",
														"NTV2_FORMAT_1080p_2997",		"NTV2_FORMAT_1080p_3000",		"NTV2_FORMAT_1080p_2500",		"NTV2_FORMAT_1080p_2398",
														"NTV2_FORMAT_1080p_2400",		"NTV2_FORMAT_1080p_2K_2398",	"NTV2_FORMAT_1080p_2K_2400",	"NTV2_FORMAT_1080p_2K_2500",
														"NTV2_FORMAT_1080p_2K_2997",	"NTV2_FORMAT_1080p_2K_3000",
														"NTV2_FORMAT_1080p_2K_4795_A",	"NTV2_FORMAT_1080p_2K_4800_A",	"NTV2_FORMAT_1080p_2K_5000_A",	"NTV2_FORMAT_1080p_2K_5994_A",	"NTV2_FORMAT_1080p_2K_6000_A",
														"NTV2_FORMAT_1080p_2K_4795_B",	"NTV2_FORMAT_1080p_2K_4800_B",	"NTV2_FORMAT_1080p_2K_5000_B",	"NTV2_FORMAT_1080p_2K_5994_B",	"NTV2_FORMAT_1080p_2K_6000_B",
														"NTV2_FORMAT_1080psf_2K_2398",	"NTV2_FORMAT_1080psf_2K_2400",	"NTV2_FORMAT_1080psf_2K_2500",	"NTV2_FORMAT_1080psf_2500_2",	"NTV2_FORMAT_1080psf_2997_2",
														"NTV2_FORMAT_1080psf_3000_2",	"NTV2_FORMAT_720p_5000",		"NTV2_FORMAT_1080p_5000_B",		"NTV2_FORMAT_1080p_5000_A",		"NTV2_FORMAT_1080p_5994_B",//
														"NTV2_FORMAT_1080p_6000_B",		"NTV2_FORMAT_1080p_6000_B",		"NTV2_FORMAT_1080p_5000_A",		"NTV2_FORMAT_1080p_5994_A",		"NTV2_FORMAT_1080p_6000_A",
														"",
														"NTV2_FORMAT_720p_2398",		"NTV2_FORMAT_720p_2500",		"NTV2_FORMAT_525_5994",			"NTV2_FORMAT_625_5000",
														"NTV2_FORMAT_525_2398",			"NTV2_FORMAT_525_2400",			"NTV2_FORMAT_525psf_2997",		"NTV2_FORMAT_625psf_2500",
														"",
														"NTV2_FORMAT_2K_1498",			"NTV2_FORMAT_2K_1500",			"NTV2_FORMAT_2K_2398",		"NTV2_FORMAT_2K_2400",				"NTV2_FORMAT_2K_2500",
														"",
														"NTV2_FORMAT_4x1920x1080psf_2398",	"NTV2_FORMAT_4x1920x1080psf_2400",	"NTV2_FORMAT_4x1920x1080psf_2500",	"NTV2_FORMAT_4x1920x1080psf_2997",
														"NTV2_FORMAT_4x1920x1080psf_3000",	"NTV2_FORMAT_4x1920x1080p_2398",	"NTV2_FORMAT_4x1920x1080p_2400",	"NTV2_FORMAT_4x1920x1080p_2500",
														"NTV2_FORMAT_4x1920x1080p_2997",	"NTV2_FORMAT_4x1920x1080p_3000",	"NTV2_FORMAT_4x2048x1080psf_2398",	"NTV2_FORMAT_4x2048x1080psf_2400",
														"NTV2_FORMAT_4x2048x1080psf_2500",	"NTV2_FORMAT_4x2048x1080psf_2997",	"NTV2_FORMAT_4x2048x1080psf_3000",	"NTV2_FORMAT_4x2048x1080p_2398",
														"NTV2_FORMAT_4x2048x1080p_2400",	"NTV2_FORMAT_4x2048x1080p_2500",	"NTV2_FORMAT_4x2048x1080p_2997",	"NTV2_FORMAT_4x2048x1080p_3000",
														"NTV2_FORMAT_4x2048x1080p_4795",	"NTV2_FORMAT_4x2048x1080p_4800",	"NTV2_FORMAT_4x1920x1080p_5000",	"NTV2_FORMAT_4x1920x1080p_5994",
														"NTV2_FORMAT_4x1920x1080p_6000",	"NTV2_FORMAT_4x2048x1080p_5000",	"NTV2_FORMAT_4x2048x1080p_5994",	"NTV2_FORMAT_4x2048x1080p_6000",
														"NTV2_FORMAT_4x2048x1080p_11988",	"NTV2_FORMAT_4x2048x1080p_12000",
														"",
														""	};
			const bool				valid []		= {	true,															true,															true,
														true,							true,							true,							true,
														true,							true,							true,							true,
														true,							true,							true,							true,
														true,							true,
														true,							true,							true,							true,							true,
														true,							true,							true,							true,							true,
														true,							true,							true,							true,							true,
														true,							true,							true,							true,							true,//
														true,							true,							true,							true,							true,
														false,
														true,							true,							true,							true,
														true,							true,							true,							true,
														false,
														true,							true,							true,							true,							true,
														false,
														true,								true,								true,								true,
														true,								true,								true,								true,
														true,								true,								true,								true,
														true,								true,								true,								true,
														true,								true,								true,								true,
														true,								true,								true,								true,
														true,								true,								true,								true,
														true,								true,
														false,
														false	};
	//	TODO:	Validate IS_HD, IS_SD, IS_720P, IS_2K, IS_2K_1080, IS_4K, IS_4K_HFR, IS_QUAD_FRAME, IS_4K_4096, IS_4K_QUADHD, IS_372_DL, IS_525, IS_625, IS_INTERMEDIATE, IS_3G, IS_3Gb, IS_WIRE, IS_PSF, IS_PROGRESSIVE, IS_A
			CHECK_EQ (sizeof(formats) / sizeof(NTV2VideoFormat),  sizeof (fmtStrs) / sizeof (std::string));
			for (unsigned ndx(0);  ndx < sizeof(formats) / sizeof(NTV2VideoFormat);  ndx++)
			{
				//cerr << " " << ndx << " " << ::NTV2VideoFormatString(formats[ndx]) << " == " << fmtStrs[ndx] << endl;
				CHECK_EQ (NTV2_IS_VALID_VIDEO_FORMAT(formats[ndx]), valid[ndx]);
				CHECK(::NTV2VideoFormatString (formats[ndx]) != NULL);	//	never NULL!
				CHECK_EQ (std::string(::NTV2VideoFormatString(formats[ndx])), fmtStrs[ndx]);
			}
		}

	//	TODO:	Validate ::NTV2RegisterNameString
	}

	TEST_CASE("NTV2AudioChannelPairs Quad Octet")
	{
		{
			NTV2AudioChannelPairs	nonPcmPairs, oldNonPcmPairs;
			NTV2AudioChannelPairs	whatsNew, whatsGone;
			CHECK(nonPcmPairs.empty ());
			CHECK(equal (oldNonPcmPairs.begin(), oldNonPcmPairs.end(), nonPcmPairs.begin()));
			oldNonPcmPairs.insert (NTV2_AudioChannel7_8);
			oldNonPcmPairs.insert (NTV2_AudioChannel11_12);
			oldNonPcmPairs.insert (NTV2_AudioChannel57_58);
			nonPcmPairs.insert (NTV2_AudioChannel3_4);

			std::set_difference (oldNonPcmPairs.begin(), oldNonPcmPairs.end(), nonPcmPairs.begin(), nonPcmPairs.end(),  std::inserter (whatsGone, whatsGone.begin()));
			std::set_difference (nonPcmPairs.begin(), nonPcmPairs.end(),  oldNonPcmPairs.begin(), oldNonPcmPairs.end(),  std::inserter (whatsNew, whatsNew.begin()));
			if (gVerboseOutput  &&  !whatsNew.empty ())
				std::cerr << "Whats new:  " << whatsNew << std::endl;
			CHECK_FALSE(whatsNew.empty ());
			CHECK_EQ(whatsNew.size (), 1);
			CHECK_EQ(*whatsNew.begin (), NTV2_AudioChannel3_4);
			if (gVerboseOutput  &&  !whatsGone.empty ())
				std::cerr << "Whats gone:  " << whatsGone.size() << ":  " << whatsGone << std::endl;
			CHECK_FALSE (whatsGone.empty ());
			CHECK_EQ (whatsGone.size (), 3);
			CHECK_EQ (*whatsGone.begin (), NTV2_AudioChannel7_8);
			CHECK_EQ (*whatsGone.rbegin (), NTV2_AudioChannel57_58);
		}
		{
			NTV2AudioChannelQuads	nonPcmPairs, oldNonPcmPairs;
			NTV2AudioChannelQuads	whatsNew, whatsGone;
			CHECK(nonPcmPairs.empty ());
			CHECK(equal (oldNonPcmPairs.begin(), oldNonPcmPairs.end(), nonPcmPairs.begin()));
			oldNonPcmPairs.insert (NTV2_AudioChannel5_8);
			oldNonPcmPairs.insert (NTV2_AudioChannel37_40);
			oldNonPcmPairs.insert (NTV2_AudioChannel125_128);
			nonPcmPairs.insert (NTV2_AudioChannel1_4);

			std::set_difference (oldNonPcmPairs.begin(), oldNonPcmPairs.end(), nonPcmPairs.begin(), nonPcmPairs.end(),  std::inserter (whatsGone, whatsGone.begin()));
			std::set_difference (nonPcmPairs.begin(), nonPcmPairs.end(),  oldNonPcmPairs.begin(), oldNonPcmPairs.end(),  std::inserter (whatsNew, whatsNew.begin()));
			if (gVerboseOutput  &&  !whatsNew.empty ())
				std::cerr << "Whats new:  " << whatsNew << std::endl;
			CHECK_FALSE (whatsNew.empty ());
			CHECK_EQ (whatsNew.size (), 1);
			CHECK_EQ (*whatsNew.begin (), NTV2_AudioChannel1_4);
			if (gVerboseOutput  &&  !whatsGone.empty ())
				std::cerr << "Whats gone:  " << whatsGone << std::endl;
			CHECK_FALSE (whatsGone.empty ());
			CHECK_EQ(whatsGone.size (), 3);
			CHECK_EQ(*whatsGone.begin (), NTV2_AudioChannel5_8);
			CHECK_EQ(*whatsGone.rbegin (), NTV2_AudioChannel125_128);
		}
		{
			NTV2AudioChannelOctets	nonPcmPairs, oldNonPcmPairs;
			NTV2AudioChannelOctets	whatsNew, whatsGone;
			CHECK (nonPcmPairs.empty ());
			CHECK (equal (oldNonPcmPairs.begin(), oldNonPcmPairs.end(), nonPcmPairs.begin()));
			oldNonPcmPairs.insert (NTV2_AudioChannel9_16);
			oldNonPcmPairs.insert (NTV2_AudioChannel73_80);
			oldNonPcmPairs.insert (NTV2_AudioChannel121_128);
			nonPcmPairs.insert (NTV2_AudioChannel1_8);

			std::set_difference (oldNonPcmPairs.begin(), oldNonPcmPairs.end(), nonPcmPairs.begin(), nonPcmPairs.end(),  std::inserter (whatsGone, whatsGone.begin()));
			std::set_difference (nonPcmPairs.begin(), nonPcmPairs.end(),  oldNonPcmPairs.begin(), oldNonPcmPairs.end(),  std::inserter (whatsNew, whatsNew.begin()));
			if (gVerboseOutput && !whatsNew.empty())
				std::cerr << "Whats new:  " << whatsNew << std::endl;
			CHECK_FALSE (whatsNew.empty ());
			CHECK_EQ (whatsNew.size (), 1);
			CHECK_EQ (*whatsNew.begin (), NTV2_AudioChannel1_8);
			if (gVerboseOutput  &&  !whatsGone.empty())
				std::cerr << "Whats gone:  " << whatsGone << std::endl;
			CHECK_FALSE (whatsGone.empty ());
			CHECK_EQ(whatsGone.size (), 3);
			CHECK_EQ(*whatsGone.begin (), NTV2_AudioChannel9_16);
			CHECK_EQ(*whatsGone.rbegin (), NTV2_AudioChannel121_128);
		}

		//	Spot-check the Pair/Quad/Octet-To-String conversions...
		CHECK_EQ(::NTV2AudioChannelPairToString (NTV2_AudioChannel57_58), "NTV2_AudioChannel57_58");
		CHECK_EQ(::NTV2AudioChannelQuadToString (NTV2_AudioChannel37_40), "NTV2_AudioChannel37_40");
		CHECK_EQ(::NTV2AudioChannelOctetToString (NTV2_AudioChannel121_128), "NTV2_AudioChannel121_128");
	}

	// TEST_CASE("NTV2RegisterExpert")
	// {
	// 	const NTV2RegNumSet	audioRegs	(CNTV2RegisterExpert::GetRegistersForClass(kRegClass_Audio));
	// 	const NTV2RegNumSet	deviceRegs	(CNTV2RegisterExpert::GetRegistersForDevice(DEVICE_ID_KONA4));
	// 	//cerr << endl << endl << "AUDIO REGS:" << endl << audioRegs << endl;
	// 	//cerr << endl << endl << "DEVICE REGS:" << endl << deviceRegs << endl;
	// 	CHECK(audioRegs.find(kRegAudioMixerInputSelects) != audioRegs.end());
	// 	CHECK(deviceRegs.find(kRegAudioMixerInputSelects) != audioRegs.end());
	// }

	TEST_CASE("NTV2FormatDescriptorBFT")
	{
		////////////////////////////////////////////////////	BEGIN TEST GetSMPTELineNumber
		NTV2FormatDescriptor	fd;
		ULWord					smpteLineNum	(0);
		ULWord					lineOffset		(0);
		bool					isF2			(false);
		const NTV2PixelFormat	fbf				(NTV2_FBF_10BIT_YCBCR);
		CHECK_FALSE(fd.GetSMPTELineNumber(0, smpteLineNum, isF2));
		CHECK_EQ(smpteLineNum, 0);
		CHECK_FALSE(isF2);

		//	525i	NO VANC
		fd = NTV2FormatDescriptor(NTV2_STANDARD_525, fbf, NTV2_VANCMODE_OFF);
		CHECK(fd.GetSMPTELineNumber(fd.GetFullRasterHeight()-2, smpteLineNum, isF2));
		CHECK_EQ(smpteLineNum, 525);
		CHECK(isF2);
		CHECK(fd.GetSMPTELineNumber(fd.GetFullRasterHeight()-1, smpteLineNum, isF2));
		CHECK_EQ(smpteLineNum, 263);
		CHECK_FALSE(isF2);
		CHECK_FALSE(fd.GetSMPTELineNumber(fd.GetFullRasterHeight(), smpteLineNum, isF2));
		CHECK_EQ(smpteLineNum, 0);
		CHECK_FALSE(isF2);
		CHECK(fd.GetSMPTELineNumber(0, smpteLineNum, isF2));
		CHECK_EQ(smpteLineNum, 283);
		CHECK(isF2);
		CHECK(fd.GetSMPTELineNumber(1, smpteLineNum, isF2));
		CHECK_EQ(smpteLineNum, 21);
		CHECK_FALSE(isF2);

		//	525i	NTV2_VANCMODE_OFF
		fd = NTV2FormatDescriptor(NTV2_STANDARD_525, fbf, NTV2_VANCMODE_OFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(268,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(6,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(269,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(7,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(271,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(9,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(272,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(10,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(282,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(20,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK (fd.GetLineOffsetFromSMPTELine(283,	lineOffset));	CHECK_EQ(lineOffset, 0);
		CHECK (fd.GetLineOffsetFromSMPTELine(21,	lineOffset));	CHECK_EQ(lineOffset, 1);
		CHECK (fd.GetLineOffsetFromSMPTELine(525,	lineOffset));	CHECK_EQ(lineOffset, 484);
		CHECK (fd.GetLineOffsetFromSMPTELine(263,	lineOffset));	CHECK_EQ(lineOffset, 485);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(600,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);

		//	525i	NTV2_VANCMODE_TALL
		fd = NTV2FormatDescriptor(NTV2_STANDARD_525, fbf, NTV2_VANCMODE_TALL);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(268,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(6,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(269,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(7,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(271,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(9,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK (fd.GetLineOffsetFromSMPTELine(272,	lineOffset));	CHECK_EQ(lineOffset, 0);
		CHECK (fd.GetLineOffsetFromSMPTELine(10,	lineOffset));	CHECK_EQ(lineOffset, 1);
		CHECK (fd.GetLineOffsetFromSMPTELine(282,	lineOffset));	CHECK_EQ(lineOffset, 20);
		CHECK (fd.GetLineOffsetFromSMPTELine(20,	lineOffset));	CHECK_EQ(lineOffset, 21);
		CHECK (fd.GetLineOffsetFromSMPTELine(283,	lineOffset));	CHECK_EQ(lineOffset, 22);
		CHECK (fd.GetLineOffsetFromSMPTELine(21,	lineOffset));	CHECK_EQ(lineOffset, 23);
		CHECK (fd.GetLineOffsetFromSMPTELine(525,	lineOffset));	CHECK_EQ(lineOffset, 506);
		CHECK (fd.GetLineOffsetFromSMPTELine(263,	lineOffset));	CHECK_EQ(lineOffset, 507);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(600,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);

		//	525i	NTV2_VANCMODE_TALLER
		fd = NTV2FormatDescriptor(NTV2_STANDARD_525, fbf, NTV2_VANCMODE_TALLER);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(268,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(6,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK (fd.GetLineOffsetFromSMPTELine(269,	lineOffset));	CHECK_EQ(lineOffset, 0);
		CHECK (fd.GetLineOffsetFromSMPTELine(7,	lineOffset));	CHECK_EQ(lineOffset, 1);
		CHECK (fd.GetLineOffsetFromSMPTELine(271,	lineOffset));	CHECK_EQ(lineOffset, 4);
		CHECK (fd.GetLineOffsetFromSMPTELine(9,	lineOffset));	CHECK_EQ(lineOffset, 5);
		CHECK (fd.GetLineOffsetFromSMPTELine(272,	lineOffset));	CHECK_EQ(lineOffset, 6);
		CHECK (fd.GetLineOffsetFromSMPTELine(10,	lineOffset));	CHECK_EQ(lineOffset, 7);
		CHECK (fd.GetLineOffsetFromSMPTELine(282,	lineOffset));	CHECK_EQ(lineOffset, 26);
		CHECK (fd.GetLineOffsetFromSMPTELine(20,	lineOffset));	CHECK_EQ(lineOffset, 27);
		CHECK (fd.GetLineOffsetFromSMPTELine(283,	lineOffset));	CHECK_EQ(lineOffset, 28);
		CHECK (fd.GetLineOffsetFromSMPTELine(21,	lineOffset));	CHECK_EQ(lineOffset, 29);
		CHECK (fd.GetLineOffsetFromSMPTELine(525,	lineOffset));	CHECK_EQ(lineOffset, 512);
		CHECK (fd.GetLineOffsetFromSMPTELine(263,	lineOffset));	CHECK_EQ(lineOffset, 513);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(600,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);

		//	625i	NTV2_VANCMODE_OFF
		fd = NTV2FormatDescriptor(NTV2_STANDARD_625, fbf, NTV2_VANCMODE_OFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(4,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(317,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(5,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(318,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(11,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(324,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(12,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(325,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(22,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(335,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK (fd.GetLineOffsetFromSMPTELine(23,	lineOffset));	CHECK_EQ(lineOffset, 0);
		CHECK (fd.GetLineOffsetFromSMPTELine(336,	lineOffset));	CHECK_EQ(lineOffset, 1);
		CHECK (fd.GetLineOffsetFromSMPTELine(310,	lineOffset));	CHECK_EQ(lineOffset, 574);
		CHECK (fd.GetLineOffsetFromSMPTELine(623,	lineOffset));	CHECK_EQ(lineOffset, 575);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(311,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(624,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);

		//	625i	NTV2_VANCMODE_TALL
		fd = NTV2FormatDescriptor(NTV2_STANDARD_625, fbf, NTV2_VANCMODE_TALL);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(4,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(317,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(5,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(318,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(11,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(324,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK (fd.GetLineOffsetFromSMPTELine(12,	lineOffset));	CHECK_EQ(lineOffset, 0);
		CHECK (fd.GetLineOffsetFromSMPTELine(325,	lineOffset));	CHECK_EQ(lineOffset, 1);
		CHECK (fd.GetLineOffsetFromSMPTELine(22,	lineOffset));	CHECK_EQ(lineOffset, 20);
		CHECK (fd.GetLineOffsetFromSMPTELine(335,	lineOffset));	CHECK_EQ(lineOffset, 21);
		CHECK (fd.GetLineOffsetFromSMPTELine(23,	lineOffset));	CHECK_EQ(lineOffset, 22);
		CHECK (fd.GetLineOffsetFromSMPTELine(336,	lineOffset));	CHECK_EQ(lineOffset, 23);
		CHECK (fd.GetLineOffsetFromSMPTELine(310,	lineOffset));	CHECK_EQ(lineOffset, 596);
		CHECK (fd.GetLineOffsetFromSMPTELine(623,	lineOffset));	CHECK_EQ(lineOffset, 597);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(311,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(624,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);

		//	625i	NTV2_VANCMODE_TALLER
		fd = NTV2FormatDescriptor(NTV2_STANDARD_625, fbf, NTV2_VANCMODE_TALLER);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(4,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(317,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK (fd.GetLineOffsetFromSMPTELine(5,	lineOffset));	CHECK_EQ(lineOffset, 0);
		CHECK (fd.GetLineOffsetFromSMPTELine(318,	lineOffset));	CHECK_EQ(lineOffset, 1);
		CHECK (fd.GetLineOffsetFromSMPTELine(11,	lineOffset));	CHECK_EQ(lineOffset, 12);
		CHECK (fd.GetLineOffsetFromSMPTELine(324,	lineOffset));	CHECK_EQ(lineOffset, 13);
		CHECK (fd.GetLineOffsetFromSMPTELine(12,	lineOffset));	CHECK_EQ(lineOffset, 14);
		CHECK (fd.GetLineOffsetFromSMPTELine(325,	lineOffset));	CHECK_EQ(lineOffset, 15);
		CHECK (fd.GetLineOffsetFromSMPTELine(22,	lineOffset));	CHECK_EQ(lineOffset, 34);
		CHECK (fd.GetLineOffsetFromSMPTELine(335,	lineOffset));	CHECK_EQ(lineOffset, 35);
		CHECK (fd.GetLineOffsetFromSMPTELine(23,	lineOffset));	CHECK_EQ(lineOffset, 36);
		CHECK (fd.GetLineOffsetFromSMPTELine(336,	lineOffset));	CHECK_EQ(lineOffset, 37);
		CHECK (fd.GetLineOffsetFromSMPTELine(310,	lineOffset));	CHECK_EQ(lineOffset, 610);
		CHECK (fd.GetLineOffsetFromSMPTELine(623,	lineOffset));	CHECK_EQ(lineOffset, 611);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(311,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(624,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);

		//	720p	NTV2_VANCMODE_OFF
		fd = NTV2FormatDescriptor(NTV2_STANDARD_720, fbf, NTV2_VANCMODE_OFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(5,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(6,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(25,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK (fd.GetLineOffsetFromSMPTELine(26,	lineOffset));	CHECK_EQ(lineOffset, 0);
		CHECK (fd.GetLineOffsetFromSMPTELine(386,	lineOffset));	CHECK_EQ(lineOffset, 360);
		CHECK (fd.GetLineOffsetFromSMPTELine(745,	lineOffset));	CHECK_EQ(lineOffset, 719);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(746,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(1200,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);

		//	720p	NTV2_VANCMODE_TALL
		fd = NTV2FormatDescriptor(NTV2_STANDARD_720, fbf, NTV2_VANCMODE_TALL);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(5,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK (fd.GetLineOffsetFromSMPTELine(6,	lineOffset));	CHECK_EQ(lineOffset, 0);
		CHECK (fd.GetLineOffsetFromSMPTELine(25,	lineOffset));	CHECK_EQ(lineOffset, 19);
		CHECK (fd.GetLineOffsetFromSMPTELine(26,	lineOffset));	CHECK_EQ(lineOffset, 20);
		CHECK (fd.GetLineOffsetFromSMPTELine(386,	lineOffset));	CHECK_EQ(lineOffset, 380);
		CHECK (fd.GetLineOffsetFromSMPTELine(745,	lineOffset));	CHECK_EQ(lineOffset, 739);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(746,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(1200,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);

		//	1080i	NTV2_VANCMODE_OFF
		fd = NTV2FormatDescriptor(NTV2_STANDARD_1080, fbf, NTV2_VANCMODE_OFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(3,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(566,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(4,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(567,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(5,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(568,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(20,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(583,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK (fd.GetLineOffsetFromSMPTELine(21,	lineOffset));	CHECK_EQ(lineOffset, 0);
		CHECK (fd.GetLineOffsetFromSMPTELine(584,	lineOffset));	CHECK_EQ(lineOffset, 1);
		CHECK (fd.GetLineOffsetFromSMPTELine(291,	lineOffset));	CHECK_EQ(lineOffset, 540);
		CHECK (fd.GetLineOffsetFromSMPTELine(854,	lineOffset));	CHECK_EQ(lineOffset, 541);
		CHECK (fd.GetLineOffsetFromSMPTELine(560,	lineOffset));	CHECK_EQ(lineOffset, 1078);
		CHECK (fd.GetLineOffsetFromSMPTELine(1123,	lineOffset));	CHECK_EQ(lineOffset, 1079);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(561,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(1124,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(1600,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);

		//	1080i	NTV2_VANCMODE_TALL
		fd = NTV2FormatDescriptor(NTV2_STANDARD_1080, fbf, NTV2_VANCMODE_TALL);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(3,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(566,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(4,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(567,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK (fd.GetLineOffsetFromSMPTELine(5,	lineOffset));	CHECK_EQ(lineOffset, 0);
		CHECK (fd.GetLineOffsetFromSMPTELine(568,	lineOffset));	CHECK_EQ(lineOffset, 1);
		CHECK (fd.GetLineOffsetFromSMPTELine(20,	lineOffset));	CHECK_EQ(lineOffset, 30);
		CHECK (fd.GetLineOffsetFromSMPTELine(583,	lineOffset));	CHECK_EQ(lineOffset, 31);
		CHECK (fd.GetLineOffsetFromSMPTELine(21,	lineOffset));	CHECK_EQ(lineOffset, 32);
		CHECK (fd.GetLineOffsetFromSMPTELine(584,	lineOffset));	CHECK_EQ(lineOffset, 33);
		CHECK (fd.GetLineOffsetFromSMPTELine(291,	lineOffset));	CHECK_EQ(lineOffset, 572);
		CHECK (fd.GetLineOffsetFromSMPTELine(854,	lineOffset));	CHECK_EQ(lineOffset, 573);
		CHECK (fd.GetLineOffsetFromSMPTELine(560,	lineOffset));	CHECK_EQ(lineOffset, 1110);
		CHECK (fd.GetLineOffsetFromSMPTELine(1123,	lineOffset));	CHECK_EQ(lineOffset, 1111);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(561,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(1124,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(1600,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);

		//	1080i	NTV2_VANCMODE_TALLER
		fd = NTV2FormatDescriptor(NTV2_STANDARD_1080, fbf, NTV2_VANCMODE_TALLER);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(3,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(566,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK (fd.GetLineOffsetFromSMPTELine(4,	lineOffset));	CHECK_EQ(lineOffset, 0);
		CHECK (fd.GetLineOffsetFromSMPTELine(567,	lineOffset));	CHECK_EQ(lineOffset, 1);
		CHECK (fd.GetLineOffsetFromSMPTELine(5,	lineOffset));	CHECK_EQ(lineOffset, 2);
		CHECK (fd.GetLineOffsetFromSMPTELine(568,	lineOffset));	CHECK_EQ(lineOffset, 3);
		CHECK (fd.GetLineOffsetFromSMPTELine(20,	lineOffset));	CHECK_EQ(lineOffset, 32);
		CHECK (fd.GetLineOffsetFromSMPTELine(583,	lineOffset));	CHECK_EQ(lineOffset, 33);
		CHECK (fd.GetLineOffsetFromSMPTELine(21,	lineOffset));	CHECK_EQ(lineOffset, 34);
		CHECK (fd.GetLineOffsetFromSMPTELine(584,	lineOffset));	CHECK_EQ(lineOffset, 35);
		CHECK (fd.GetLineOffsetFromSMPTELine(291,	lineOffset));	CHECK_EQ(lineOffset, 574);
		CHECK (fd.GetLineOffsetFromSMPTELine(854,	lineOffset));	CHECK_EQ(lineOffset, 575);
		CHECK (fd.GetLineOffsetFromSMPTELine(560,	lineOffset));	CHECK_EQ(lineOffset, 1112);
		CHECK (fd.GetLineOffsetFromSMPTELine(1123,	lineOffset));	CHECK_EQ(lineOffset, 1113);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(561,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(1124,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(1600,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);

		//	1080p	NTV2_VANCMODE_OFF
		fd = NTV2FormatDescriptor(NTV2_STANDARD_1080p, fbf, NTV2_VANCMODE_OFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(7,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(8,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(9,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(10,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(41,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK (fd.GetLineOffsetFromSMPTELine(42,	lineOffset));	CHECK_EQ(lineOffset, 0);
		CHECK (fd.GetLineOffsetFromSMPTELine(582,	lineOffset));	CHECK_EQ(lineOffset, 540);
		CHECK (fd.GetLineOffsetFromSMPTELine(1121,	lineOffset));	CHECK_EQ(lineOffset, 1079);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(1122,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(1600,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);

		//	1080p	NTV2_VANCMODE_TALL
		fd = NTV2FormatDescriptor(NTV2_STANDARD_1080p, fbf, NTV2_VANCMODE_TALL);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(7,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(8,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(9,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK (fd.GetLineOffsetFromSMPTELine(10,	lineOffset));	CHECK_EQ(lineOffset, 0);
		CHECK (fd.GetLineOffsetFromSMPTELine(41,	lineOffset));	CHECK_EQ(lineOffset, 31);
		CHECK (fd.GetLineOffsetFromSMPTELine(42,	lineOffset));	CHECK_EQ(lineOffset, 32);
		CHECK (fd.GetLineOffsetFromSMPTELine(582,	lineOffset));	CHECK_EQ(lineOffset, 572);
		CHECK (fd.GetLineOffsetFromSMPTELine(1121,	lineOffset));	CHECK_EQ(lineOffset, 1111);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(1122,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(1600,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);

		//	1080p	NTV2_VANCMODE_TALLER
		fd = NTV2FormatDescriptor(NTV2_STANDARD_1080p, fbf, NTV2_VANCMODE_TALLER);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(7,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK (fd.GetLineOffsetFromSMPTELine(8,	lineOffset));	CHECK_EQ(lineOffset, 0);
		CHECK (fd.GetLineOffsetFromSMPTELine(9,	lineOffset));	CHECK_EQ(lineOffset, 1);
		CHECK (fd.GetLineOffsetFromSMPTELine(10,	lineOffset));	CHECK_EQ(lineOffset, 2);
		CHECK (fd.GetLineOffsetFromSMPTELine(41,	lineOffset));	CHECK_EQ(lineOffset, 33);
		CHECK (fd.GetLineOffsetFromSMPTELine(42,	lineOffset));	CHECK_EQ(lineOffset, 34);
		CHECK (fd.GetLineOffsetFromSMPTELine(582,	lineOffset));	CHECK_EQ(lineOffset, 574);
		CHECK (fd.GetLineOffsetFromSMPTELine(1121,	lineOffset));	CHECK_EQ(lineOffset, 1113);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(1122,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(1600,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);

		//	2K1080p		NTV2_VANCMODE_OFF
		fd = NTV2FormatDescriptor(NTV2_STANDARD_2Kx1080p, fbf, NTV2_VANCMODE_OFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(7,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(8,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(9,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(10,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(41,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK (fd.GetLineOffsetFromSMPTELine(42,	lineOffset));	CHECK_EQ(lineOffset, 0);
		CHECK (fd.GetLineOffsetFromSMPTELine(582,	lineOffset));	CHECK_EQ(lineOffset, 540);
		CHECK (fd.GetLineOffsetFromSMPTELine(1121,	lineOffset));	CHECK_EQ(lineOffset, 1079);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(1122,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(1600,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);

		//	2K1080p		NTV2_VANCMODE_TALL
		fd = NTV2FormatDescriptor(NTV2_STANDARD_2Kx1080p, fbf, NTV2_VANCMODE_TALL);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(7,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(8,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(9,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK (fd.GetLineOffsetFromSMPTELine(10,	lineOffset));	CHECK_EQ(lineOffset, 0);
		CHECK (fd.GetLineOffsetFromSMPTELine(41,	lineOffset));	CHECK_EQ(lineOffset, 31);
		CHECK (fd.GetLineOffsetFromSMPTELine(42,	lineOffset));	CHECK_EQ(lineOffset, 32);
		CHECK (fd.GetLineOffsetFromSMPTELine(582,	lineOffset));	CHECK_EQ(lineOffset, 572);
		CHECK (fd.GetLineOffsetFromSMPTELine(1121,	lineOffset));	CHECK_EQ(lineOffset, 1111);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(1122,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(1600,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);

		//	2K1080p		NTV2_VANCMODE_TALLER
		fd = NTV2FormatDescriptor(NTV2_STANDARD_2Kx1080p, fbf, NTV2_VANCMODE_TALLER);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(7,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK (fd.GetLineOffsetFromSMPTELine(8,	lineOffset));	CHECK_EQ(lineOffset, 0);
		CHECK (fd.GetLineOffsetFromSMPTELine(9,	lineOffset));	CHECK_EQ(lineOffset, 1);
		CHECK (fd.GetLineOffsetFromSMPTELine(10,	lineOffset));	CHECK_EQ(lineOffset, 2);
		CHECK (fd.GetLineOffsetFromSMPTELine(41,	lineOffset));	CHECK_EQ(lineOffset, 33);
		CHECK (fd.GetLineOffsetFromSMPTELine(42,	lineOffset));	CHECK_EQ(lineOffset, 34);
		CHECK (fd.GetLineOffsetFromSMPTELine(582,	lineOffset));	CHECK_EQ(lineOffset, 574);
		CHECK (fd.GetLineOffsetFromSMPTELine(1121,	lineOffset));	CHECK_EQ(lineOffset, 1113);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(1122,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(1600,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);

		if (AJA_NTV2_SDK_VERSION_MAJOR > 0  &&  AJA_NTV2_SDK_VERSION_MAJOR < 15)	//	Film formats officially abandoned in SDK 15
		{
			//	2Kx1556		NTV2_VANCMODE_OFF
			fd = NTV2FormatDescriptor(NTV2_STANDARD_2K, fbf, NTV2_VANCMODE_OFF);
			CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(7,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
			CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(194,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
			CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(195,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
			CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(210,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
			CHECK (fd.GetLineOffsetFromSMPTELine(211,	lineOffset));	CHECK_EQ(lineOffset, 0);
			CHECK (fd.GetLineOffsetFromSMPTELine(988,	lineOffset));	CHECK_EQ(lineOffset, 1554);
			CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(1000,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);

			//	2Kx1556		NTV2_VANCMODE_TALL
			fd = NTV2FormatDescriptor(NTV2_STANDARD_2K, fbf, NTV2_VANCMODE_TALL);
			CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(7,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
			CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(194,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
			CHECK (fd.GetLineOffsetFromSMPTELine(195,	lineOffset));	CHECK_EQ(lineOffset, 0);
			CHECK (fd.GetLineOffsetFromSMPTELine(210,	lineOffset));	CHECK_EQ(lineOffset, 30);
			CHECK (fd.GetLineOffsetFromSMPTELine(211,	lineOffset));	CHECK_EQ(lineOffset, 31);
			CHECK (fd.GetLineOffsetFromSMPTELine(988,	lineOffset));	CHECK_EQ(lineOffset, 1586);
			CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(1000,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		}	//	AJA_NTV2_SDK_VERSION_MAJOR > 0  &&  AJA_NTV2_SDK_VERSION_MAJOR < 15

		//	UHD		NTV2_STANDARD_3840x2160p
		fd = NTV2FormatDescriptor(NTV2_STANDARD_3840x2160p, fbf);
		CHECK(fd.IsValid());
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(7,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(41,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK (fd.GetLineOffsetFromSMPTELine(42,	lineOffset));	CHECK_EQ(lineOffset, 0);
		CHECK (fd.GetLineOffsetFromSMPTELine(582,	lineOffset));	CHECK_EQ(lineOffset, 540);
		CHECK (fd.GetLineOffsetFromSMPTELine(1121,	lineOffset));	CHECK_EQ(lineOffset, 1079);
		CHECK (fd.GetLineOffsetFromSMPTELine(1122,	lineOffset));	CHECK_EQ(lineOffset, 1080);
		CHECK (fd.GetLineOffsetFromSMPTELine(1600,	lineOffset));	CHECK_EQ(lineOffset, 1558);
		CHECK (fd.GetLineOffsetFromSMPTELine(2048,	lineOffset));	CHECK_EQ(lineOffset, 2006);
		CHECK (fd.GetLineOffsetFromSMPTELine(2159,	lineOffset));	CHECK_EQ(lineOffset, 2117);
		CHECK (fd.GetLineOffsetFromSMPTELine(2160,	lineOffset));	CHECK_EQ(lineOffset, 2118);
		CHECK (fd.GetLineOffsetFromSMPTELine(2201,	lineOffset));	CHECK_EQ(lineOffset, 2159);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(2202,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(3840,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);

		//	4K		NTV2_STANDARD_4096x2160p
		fd = NTV2FormatDescriptor(NTV2_STANDARD_4096x2160p, fbf);
		CHECK(fd.IsValid());
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(7,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(41,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK (fd.GetLineOffsetFromSMPTELine(42,	lineOffset));	CHECK_EQ(lineOffset, 0);
		CHECK (fd.GetLineOffsetFromSMPTELine(582,	lineOffset));	CHECK_EQ(lineOffset, 540);
		CHECK (fd.GetLineOffsetFromSMPTELine(1121,	lineOffset));	CHECK_EQ(lineOffset, 1079);
		CHECK (fd.GetLineOffsetFromSMPTELine(1122,	lineOffset));	CHECK_EQ(lineOffset, 1080);
		CHECK (fd.GetLineOffsetFromSMPTELine(1600,	lineOffset));	CHECK_EQ(lineOffset, 1558);
		CHECK (fd.GetLineOffsetFromSMPTELine(2048,	lineOffset));	CHECK_EQ(lineOffset, 2006);
		CHECK (fd.GetLineOffsetFromSMPTELine(2159,	lineOffset));	CHECK_EQ(lineOffset, 2117);
		CHECK (fd.GetLineOffsetFromSMPTELine(2160,	lineOffset));	CHECK_EQ(lineOffset, 2118);
		CHECK (fd.GetLineOffsetFromSMPTELine(2201,	lineOffset));	CHECK_EQ(lineOffset, 2159);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(2202,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(3840,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);

		//	UHD HFR	NTV2_STANDARD_3840HFR -- 3840x2160
		fd = NTV2FormatDescriptor(NTV2_STANDARD_3840HFR, fbf);
		CHECK(fd.IsValid());
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(7,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(41,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK (fd.GetLineOffsetFromSMPTELine(42,	lineOffset));	CHECK_EQ(lineOffset, 0);
		CHECK (fd.GetLineOffsetFromSMPTELine(582,	lineOffset));	CHECK_EQ(lineOffset, 540);
		CHECK (fd.GetLineOffsetFromSMPTELine(1121,	lineOffset));	CHECK_EQ(lineOffset, 1079);
		CHECK (fd.GetLineOffsetFromSMPTELine(1122,	lineOffset));	CHECK_EQ(lineOffset, 1080);
		CHECK (fd.GetLineOffsetFromSMPTELine(1600,	lineOffset));	CHECK_EQ(lineOffset, 1558);
		CHECK (fd.GetLineOffsetFromSMPTELine(2048,	lineOffset));	CHECK_EQ(lineOffset, 2006);
		CHECK (fd.GetLineOffsetFromSMPTELine(2159,	lineOffset));	CHECK_EQ(lineOffset, 2117);
		CHECK (fd.GetLineOffsetFromSMPTELine(2160,	lineOffset));	CHECK_EQ(lineOffset, 2118);
		CHECK (fd.GetLineOffsetFromSMPTELine(2201,	lineOffset));	CHECK_EQ(lineOffset, 2159);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(2202,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(3840,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);

		//	4K HFR	NTV2_STANDARD_4096HFR -- 4096x2160
		fd = NTV2FormatDescriptor(NTV2_STANDARD_4096HFR, fbf);
		CHECK(fd.IsValid());
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(7,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(41,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK (fd.GetLineOffsetFromSMPTELine(42,	lineOffset));	CHECK_EQ(lineOffset, 0);
		CHECK (fd.GetLineOffsetFromSMPTELine(582,	lineOffset));	CHECK_EQ(lineOffset, 540);
		CHECK (fd.GetLineOffsetFromSMPTELine(1121,	lineOffset));	CHECK_EQ(lineOffset, 1079);
		CHECK (fd.GetLineOffsetFromSMPTELine(1122,	lineOffset));	CHECK_EQ(lineOffset, 1080);
		CHECK (fd.GetLineOffsetFromSMPTELine(1600,	lineOffset));	CHECK_EQ(lineOffset, 1558);
		CHECK (fd.GetLineOffsetFromSMPTELine(2048,	lineOffset));	CHECK_EQ(lineOffset, 2006);
		CHECK (fd.GetLineOffsetFromSMPTELine(2159,	lineOffset));	CHECK_EQ(lineOffset, 2117);
		CHECK (fd.GetLineOffsetFromSMPTELine(2160,	lineOffset));	CHECK_EQ(lineOffset, 2118);
		CHECK (fd.GetLineOffsetFromSMPTELine(2201,	lineOffset));	CHECK_EQ(lineOffset, 2159);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(2202,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(3840,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);

		//	UHD2	NTV2_STANDARD_7680 -- 4x3840x2160 or 7680x4320
		fd = NTV2FormatDescriptor(NTV2_STANDARD_7680, fbf);
		CHECK(fd.IsValid());
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(7,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(41,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK (fd.GetLineOffsetFromSMPTELine(42,	lineOffset));	CHECK_EQ(lineOffset, 0);
		CHECK (fd.GetLineOffsetFromSMPTELine(582,	lineOffset));	CHECK_EQ(lineOffset, 540);
		CHECK (fd.GetLineOffsetFromSMPTELine(1121,	lineOffset));	CHECK_EQ(lineOffset, 1079);
		CHECK (fd.GetLineOffsetFromSMPTELine(1122,	lineOffset));	CHECK_EQ(lineOffset, 1080);
		CHECK (fd.GetLineOffsetFromSMPTELine(1600,	lineOffset));	CHECK_EQ(lineOffset, 1558);
		CHECK (fd.GetLineOffsetFromSMPTELine(2048,	lineOffset));	CHECK_EQ(lineOffset, 2006);
		CHECK (fd.GetLineOffsetFromSMPTELine(2159,	lineOffset));	CHECK_EQ(lineOffset, 2117);
		CHECK (fd.GetLineOffsetFromSMPTELine(2160,	lineOffset));	CHECK_EQ(lineOffset, 2118);
		CHECK (fd.GetLineOffsetFromSMPTELine(2201,	lineOffset));	CHECK_EQ(lineOffset, 2159);
		CHECK (fd.GetLineOffsetFromSMPTELine(2202,	lineOffset));	CHECK_EQ(lineOffset, 2160);
		CHECK (fd.GetLineOffsetFromSMPTELine(3840,	lineOffset));	CHECK_EQ(lineOffset, 3798);
		CHECK (fd.GetLineOffsetFromSMPTELine(4319,	lineOffset));	CHECK_EQ(lineOffset, 4277);
		CHECK (fd.GetLineOffsetFromSMPTELine(4320,	lineOffset));	CHECK_EQ(lineOffset, 4278);
		CHECK (fd.GetLineOffsetFromSMPTELine(4361,	lineOffset));	CHECK_EQ(lineOffset, 4319);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(4362,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(5000,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);

		//	8K		NTV2_STANDARD_8192 -- 4x4096x2160 or 8192x4320
		fd = NTV2FormatDescriptor(NTV2_STANDARD_8192, fbf);
		CHECK(fd.IsValid());
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(7,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(41,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK (fd.GetLineOffsetFromSMPTELine(42,	lineOffset));	CHECK_EQ(lineOffset, 0);
		CHECK (fd.GetLineOffsetFromSMPTELine(582,	lineOffset));	CHECK_EQ(lineOffset, 540);
		CHECK (fd.GetLineOffsetFromSMPTELine(1121,	lineOffset));	CHECK_EQ(lineOffset, 1079);
		CHECK (fd.GetLineOffsetFromSMPTELine(1122,	lineOffset));	CHECK_EQ(lineOffset, 1080);
		CHECK (fd.GetLineOffsetFromSMPTELine(1600,	lineOffset));	CHECK_EQ(lineOffset, 1558);
		CHECK (fd.GetLineOffsetFromSMPTELine(2048,	lineOffset));	CHECK_EQ(lineOffset, 2006);
		CHECK (fd.GetLineOffsetFromSMPTELine(2159,	lineOffset));	CHECK_EQ(lineOffset, 2117);
		CHECK (fd.GetLineOffsetFromSMPTELine(2160,	lineOffset));	CHECK_EQ(lineOffset, 2118);
		CHECK (fd.GetLineOffsetFromSMPTELine(2201,	lineOffset));	CHECK_EQ(lineOffset, 2159);
		CHECK (fd.GetLineOffsetFromSMPTELine(2202,	lineOffset));	CHECK_EQ(lineOffset, 2160);
		CHECK (fd.GetLineOffsetFromSMPTELine(3840,	lineOffset));	CHECK_EQ(lineOffset, 3798);
		CHECK (fd.GetLineOffsetFromSMPTELine(4319,	lineOffset));	CHECK_EQ(lineOffset, 4277);
		CHECK (fd.GetLineOffsetFromSMPTELine(4320,	lineOffset));	CHECK_EQ(lineOffset, 4278);
		CHECK (fd.GetLineOffsetFromSMPTELine(4361,	lineOffset));	CHECK_EQ(lineOffset, 4319);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(4362,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
		CHECK_FALSE(fd.GetLineOffsetFromSMPTELine(5000,	lineOffset));	CHECK_EQ(lineOffset, 0xFFFFFFFF);
	}

	// TEST_CASE("NTV2 Driver Version")
	// {
	// 	const ULWord	maxMajorNum	(0x00000080);	//	Major Version:	0 thru 127
	// 	const ULWord	maxMinorNum	(0x00000040);	//	Minor Version:	0 thru 63
	// 	const ULWord	maxPointNum	(0x00000040);	//	Point Number:	0 thru 63
	// 	const ULWord	maxBuildNum	(0x00000400);	//	Build Number:	0 thru 1023
	// 	for (ULWord major=0;  major < maxMajorNum;  major++)
	// 		for (ULWord minor=0;  minor < maxMinorNum;  minor++)
	// 			for (ULWord point(0);  point < maxPointNum;  point++)
	// 				for (ULWord build(0);  build < maxBuildNum;  build++)
	// 				{
	// 					const ULWord	encodedVersion	(NTV2DriverVersionEncode(major, minor, point, build));
	// 					const ULWord	compMajor		(NTV2DriverVersionDecode_Major(encodedVersion));
	// 					const ULWord	compMinor		(NTV2DriverVersionDecode_Minor(encodedVersion));
	// 					const ULWord	compPoint		(NTV2DriverVersionDecode_Point(encodedVersion));
	// 					const ULWord	compBuild		(NTV2DriverVersionDecode_Build(encodedVersion));
	// 					//if (major != compMajor || minor != compMinor || point != compPoint || build != compBuild)
	// 					//	cerr << DEC(compMajor) << "." << DEC(compMinor) << "." << DEC(compPoint) << "." << DEC(compBuild) << endl;
	// 					CHECK_EQ(major, compMajor);
	// 					CHECK_EQ(minor, compMinor);
	// 					CHECK_EQ(point, compPoint);
	// 					CHECK_EQ(build, compBuild);
	// 					const ULWord	reEncodedVers	(NTV2DriverVersionEncode(compMajor, compMinor, compPoint, compBuild));
	// 					CHECK_EQ(encodedVersion, reEncodedVers);
	// 				}
	// }

	/*
		NTV2AncCollisionBFT
		This detects if the Anc area will run into the visible raster.
		It uses the default Anc offsets.
		If these change in the drivers, then it needs to be changed here too:
											NTV2_AncRgn_Field1	NTV2_AncRgn_Field2	NTV2_AncRgn_MonField1	NTV2_AncRgn_MonField2
		defaultAncRgnOffsetsFromBottom = {	0x4000,				0x2000,				0x8000,					0x6000};
	*/
	TEST_CASE("NTV2AncCollision")
	{
		typedef	std::vector<NTV2Standard>			NTV2Stds;
		typedef	NTV2Stds::const_iterator		NTV2StdsConstIter;
		const NTV2Stds	standards = {	NTV2_STANDARD_1080, NTV2_STANDARD_720, NTV2_STANDARD_525, NTV2_STANDARD_625,
											NTV2_STANDARD_1080p, NTV2_STANDARD_2K, NTV2_STANDARD_2Kx1080p, NTV2_STANDARD_2Kx1080i,
											NTV2_STANDARD_3840x2160p, NTV2_STANDARD_4096x2160p, NTV2_STANDARD_3840HFR,
											NTV2_STANDARD_4096HFR, NTV2_STANDARD_7680, NTV2_STANDARD_3840i, NTV2_STANDARD_4096i};
		const NTV2VANCModes	vancModes =	{	NTV2_VANCMODE_OFF, NTV2_VANCMODE_TALL, NTV2_VANCMODE_TALLER };
		const ULWordSequence defaultAncRgnOffsetsFromBottom = {0x4000, 0x2000, 0x8000, 0x6000};
		ULWord minAncGap(0xFFFFFFFF), minMonGap(0xFFFFFFFF);
		NTV2FrameGeometry minAncGeo(NTV2_FG_INVALID), minMonGeo(NTV2_FG_INVALID);
		NTV2PixelFormat minAncFBF(NTV2_FBF_INVALID), minMonFBF(NTV2_FBF_INVALID);

		LOGDBG("Standard" << "@" << "Geometry" << "@" << "PixelFormat" << "@" << "Phys" << "@" << "Start Avail Space" << "@" << "Start F1 Anc" << "@" << "Gap");
		for (NTV2StdsConstIter it(standards.begin());  it != standards.end();  ++it)
		{
			const NTV2Standard		standard	(*it);
			const NTV2FrameGeometry	stdGeometry	(::GetGeometryFromStandard(standard));
			const NTV2GeometrySet	geometries	(::GetRelatedGeometries(stdGeometry));
			for (NTV2PixelFormat fbf(NTV2_FBF_FIRST);  fbf < NTV2_FBF_LAST;  fbf = NTV2PixelFormat(fbf+1))
			{
				if (!NTV2_IS_VALID_FBF(fbf) || NTV2_IS_FBF_PLANAR(fbf) || NTV2_IS_FBF_PRORES(fbf) || NTV2_FBF_IS_RAW(fbf))
					continue;
				if (fbf == NTV2_FBF_8BIT_DVCPRO  ||  fbf == NTV2_FBF_8BIT_HDV)
					continue;

				for (size_t ndx(0);  ndx < geometries.size();  ndx++)
				{
					const NTV2FrameGeometry actualGeometry (::GetVANCFrameGeometry (stdGeometry, vancModes.at(ndx)));
					const ULWord	physBufferSize	(ULWord(::Get8MBFrameSizeFactor(actualGeometry, fbf)) * 8LL * 1024LL * 1024LL);
					const NTV2FormatDescriptor fd (standard, fbf, vancModes.at(ndx));
					ULWord	legalAncStartOffset(fd.GetTotalRasterBytes());
					NTV2_ASSERT(legalAncStartOffset>0);
					const ULWord ancF1Offset (physBufferSize - defaultAncRgnOffsetsFromBottom.at(NTV2_AncRgn_Field1));
					ULWord gap (ancF1Offset - legalAncStartOffset);
					LOGDBG(::NTV2StandardToString(standard) << "@" << ::NTV2FrameGeometryToString(actualGeometry) << "@" << ::NTV2FrameBufferFormatToString(fbf) << "@" << DEC(physBufferSize) << "@" << DEC(legalAncStartOffset) << "@" << DEC(ancF1Offset) << "@" << DEC(gap));
					if (gap < minAncGap)
						{minAncGap = gap;  minAncGeo = actualGeometry;  minAncFBF = fbf;}
					const ULWord monAncF1Offset (physBufferSize - defaultAncRgnOffsetsFromBottom.at(NTV2_AncRgn_MonField1));
					gap = monAncF1Offset - legalAncStartOffset;
					if (gap < minMonGap)
						{minMonGap = gap;  minMonGeo = actualGeometry;  minMonFBF = fbf;}
					CHECK(ancF1Offset >= legalAncStartOffset);
					CHECK(monAncF1Offset >= legalAncStartOffset);
				}	//	for each VANC mode
			}	//	for each valid FBF
		}	//	for every video standard

		LOGNOTE("Passed, minAncGap=" << DEC(minAncGap) << "|" << ::NTV2FrameGeometryToString(minAncGeo) << "|" << ::NTV2FrameBufferFormatToString(minAncFBF)
				<< ", minMonGap=" << DEC(minMonGap) << "|" << ::NTV2FrameGeometryToString(minMonGeo) << "|" << ::NTV2FrameBufferFormatToString(minMonFBF));
	}

	TEST_CASE("NTV2Utils")
	{
		static const NTV2FrameRate	fr1498[]	= {NTV2_FRAMERATE_1498, NTV2_FRAMERATE_2997, NTV2_FRAMERATE_5994, NTV2_FRAMERATE_11988};
		static const NTV2FrameRate	fr1500[]	= {NTV2_FRAMERATE_1500, NTV2_FRAMERATE_3000, NTV2_FRAMERATE_6000, NTV2_FRAMERATE_12000};
		static const NTV2FrameRate	fr2398[]	= {NTV2_FRAMERATE_2398, NTV2_FRAMERATE_4795};
		static const NTV2FrameRate	fr2400[]	= {NTV2_FRAMERATE_2400, NTV2_FRAMERATE_4800};
		static const NTV2FrameRate	fr2500[]	= {NTV2_FRAMERATE_2500, NTV2_FRAMERATE_5000};

	//	for (NTV2FrameRate fr(NTV2_FRAMERATE_6000);  fr <= NTV2_FRAMERATE_1498;  fr = NTV2FrameRate(fr+1))
	//		cerr << ::NTV2FrameRateToString(fr) << " family is " << ::NTV2FrameRateToString(::GetFrameRateFamily(fr)) << endl;

		//	Verify Related Rates
		CHECK(::IsMultiFormatCompatible(fr1498[0], fr1498[0]));
		CHECK(::IsMultiFormatCompatible(fr1498[0], fr1498[1]));
		CHECK(::IsMultiFormatCompatible(fr1498[0], fr1498[2]));
		CHECK(::IsMultiFormatCompatible(fr1498[0], fr1498[3]));
		CHECK(::IsMultiFormatCompatible(fr1498[1], fr1498[0]));
		CHECK(::IsMultiFormatCompatible(fr1498[1], fr1498[2]));
		CHECK(::IsMultiFormatCompatible(fr1498[1], fr1498[3]));
		CHECK(::IsMultiFormatCompatible(fr1498[2], fr1498[0]));
		CHECK(::IsMultiFormatCompatible(fr1498[2], fr1498[1]));
		CHECK(::IsMultiFormatCompatible(fr1498[2], fr1498[3]));
		CHECK(::IsMultiFormatCompatible(fr1498[3], fr1498[0]));
		CHECK(::IsMultiFormatCompatible(fr1498[3], fr1498[1]));
		CHECK(::IsMultiFormatCompatible(fr1498[3], fr1498[2]));
		CHECK(::IsMultiFormatCompatible(fr1500[0], fr1500[0]));
		CHECK(::IsMultiFormatCompatible(fr1500[0], fr1500[1]));
		CHECK(::IsMultiFormatCompatible(fr1500[0], fr1500[2]));
		CHECK(::IsMultiFormatCompatible(fr1500[0], fr1500[3]));
		CHECK(::IsMultiFormatCompatible(fr1500[1], fr1500[0]));
		CHECK(::IsMultiFormatCompatible(fr1500[1], fr1500[2]));
		CHECK(::IsMultiFormatCompatible(fr1500[1], fr1500[3]));
		CHECK(::IsMultiFormatCompatible(fr1500[2], fr1500[0]));
		CHECK(::IsMultiFormatCompatible(fr1500[2], fr1500[1]));
		CHECK(::IsMultiFormatCompatible(fr1500[2], fr1500[3]));
		CHECK(::IsMultiFormatCompatible(fr1500[3], fr1500[0]));
		CHECK(::IsMultiFormatCompatible(fr1500[3], fr1500[1]));
		CHECK(::IsMultiFormatCompatible(fr1500[3], fr1500[2]));
		CHECK(::IsMultiFormatCompatible(fr2398[0], fr2398[0]));
		CHECK(::IsMultiFormatCompatible(fr2398[0], fr2398[1]));
		CHECK(::IsMultiFormatCompatible(fr2398[1], fr2398[0]));
		CHECK(::IsMultiFormatCompatible(fr2400[0], fr2400[0]));
		CHECK(::IsMultiFormatCompatible(fr2400[0], fr2400[1]));
		CHECK(::IsMultiFormatCompatible(fr2400[1], fr2400[0]));
		CHECK(::IsMultiFormatCompatible(fr2500[0], fr2500[0]));
		CHECK(::IsMultiFormatCompatible(fr2500[0], fr2500[1]));
		CHECK(::IsMultiFormatCompatible(fr2500[1], fr2500[0]));

		//	Verify Unrelated Rates
		CHECK_FALSE(::IsMultiFormatCompatible(fr1498[0], fr1500[0]));
		CHECK_FALSE(::IsMultiFormatCompatible(fr1498[0], fr1500[1]));
		CHECK_FALSE(::IsMultiFormatCompatible(fr1498[0], fr1500[2]));
		CHECK_FALSE(::IsMultiFormatCompatible(fr1498[0], fr1500[3]));
		CHECK_FALSE(::IsMultiFormatCompatible(fr1498[0], fr2398[0]));
		CHECK_FALSE(::IsMultiFormatCompatible(fr1498[0], fr2398[1]));
		CHECK_FALSE(::IsMultiFormatCompatible(fr1498[0], fr2400[0]));
		CHECK_FALSE(::IsMultiFormatCompatible(fr1498[0], fr2400[1]));
		CHECK_FALSE(::IsMultiFormatCompatible(fr1498[0], fr2500[0]));
		CHECK_FALSE(::IsMultiFormatCompatible(fr1498[0], fr2500[1]));

		CHECK_FALSE(::IsMultiFormatCompatible(fr1500[0], fr1498[0]));
		CHECK_FALSE(::IsMultiFormatCompatible(fr1500[0], fr1498[1]));
		CHECK_FALSE(::IsMultiFormatCompatible(fr1500[0], fr1498[2]));
		CHECK_FALSE(::IsMultiFormatCompatible(fr1500[0], fr1498[3]));
		CHECK_FALSE(::IsMultiFormatCompatible(fr1500[0], fr2398[0]));
		CHECK_FALSE(::IsMultiFormatCompatible(fr1500[0], fr2398[1]));
		CHECK_FALSE(::IsMultiFormatCompatible(fr1500[0], fr2400[0]));
		CHECK_FALSE(::IsMultiFormatCompatible(fr1500[0], fr2400[1]));
		CHECK_FALSE(::IsMultiFormatCompatible(fr1500[0], fr2500[0]));
		CHECK_FALSE(::IsMultiFormatCompatible(fr1500[0], fr2500[1]));

		CHECK_FALSE(::IsMultiFormatCompatible(fr2398[0], fr1498[0]));
		CHECK_FALSE(::IsMultiFormatCompatible(fr2398[0], fr1498[1]));
		CHECK_FALSE(::IsMultiFormatCompatible(fr2398[0], fr1498[2]));
		CHECK_FALSE(::IsMultiFormatCompatible(fr2398[0], fr1498[3]));
		CHECK_FALSE(::IsMultiFormatCompatible(fr2398[0], fr1500[0]));
		CHECK_FALSE(::IsMultiFormatCompatible(fr2398[0], fr1500[1]));
		CHECK_FALSE(::IsMultiFormatCompatible(fr2398[0], fr1500[2]));
		CHECK_FALSE(::IsMultiFormatCompatible(fr2398[0], fr1500[3]));
		CHECK_FALSE(::IsMultiFormatCompatible(fr2398[0], fr2400[0]));
		CHECK_FALSE(::IsMultiFormatCompatible(fr2398[0], fr2400[1]));
		CHECK_FALSE(::IsMultiFormatCompatible(fr2398[0], fr2500[0]));
		CHECK_FALSE(::IsMultiFormatCompatible(fr2398[0], fr2500[1]));

		CHECK_FALSE(::IsMultiFormatCompatible(fr2400[0], fr1498[0]));
		CHECK_FALSE(::IsMultiFormatCompatible(fr2400[0], fr1498[1]));
		CHECK_FALSE(::IsMultiFormatCompatible(fr2400[0], fr1498[2]));
		CHECK_FALSE(::IsMultiFormatCompatible(fr2400[0], fr1498[3]));
		CHECK_FALSE(::IsMultiFormatCompatible(fr2400[0], fr1500[0]));
		CHECK_FALSE(::IsMultiFormatCompatible(fr2400[0], fr1500[1]));
		CHECK_FALSE(::IsMultiFormatCompatible(fr2400[0], fr1500[2]));
		CHECK_FALSE(::IsMultiFormatCompatible(fr2400[0], fr1500[3]));
		CHECK_FALSE(::IsMultiFormatCompatible(fr2400[0], fr2398[0]));
		CHECK_FALSE(::IsMultiFormatCompatible(fr2400[0], fr2398[1]));
		CHECK_FALSE(::IsMultiFormatCompatible(fr2400[0], fr2500[0]));
		CHECK_FALSE(::IsMultiFormatCompatible(fr2400[0], fr2500[1]));

		CHECK_FALSE(::IsMultiFormatCompatible(fr2500[0], fr1498[0]));
		CHECK_FALSE(::IsMultiFormatCompatible(fr2500[0], fr1498[1]));
		CHECK_FALSE(::IsMultiFormatCompatible(fr2500[0], fr1498[2]));
		CHECK_FALSE(::IsMultiFormatCompatible(fr2500[0], fr1498[3]));
		CHECK_FALSE(::IsMultiFormatCompatible(fr2500[0], fr1500[0]));
		CHECK_FALSE(::IsMultiFormatCompatible(fr2500[0], fr1500[1]));
		CHECK_FALSE(::IsMultiFormatCompatible(fr2500[0], fr1500[2]));
		CHECK_FALSE(::IsMultiFormatCompatible(fr2500[0], fr1500[3]));
		CHECK_FALSE(::IsMultiFormatCompatible(fr2500[0], fr2398[0]));
		CHECK_FALSE(::IsMultiFormatCompatible(fr2500[0], fr2398[1]));
		CHECK_FALSE(::IsMultiFormatCompatible(fr2500[0], fr2400[0]));
		CHECK_FALSE(::IsMultiFormatCompatible(fr2500[0], fr2400[1]));

		// CHECK(NTV2CopyRasterBFT ()); // TODO: BFTs called NTV2CopyRasterBFT as a function. Does it need to be here or can it be separate?
		for (NTV2FrameBufferFormat fbf(NTV2_FBF_10BIT_YCBCR);  fbf < NTV2_FBF_NUMFRAMEBUFFERFORMATS;  fbf = NTV2FrameBufferFormat(fbf+1))
			if (NTV2_IS_VALID_FRAME_BUFFER_FORMAT (fbf))
			{
				CHECK_EQ (::CalcRowBytesForFormat (fbf, 0), 0);
			}

		CHECK_FALSE(NTV2_IS_VALID_NTV2FrameGeometry(::GetGeometryFromStandard(NTV2_STANDARD_INVALID)));
		for (NTV2Standard std(NTV2_STANDARD_1080);  std < NTV2_NUM_STANDARDS;  std = NTV2Standard(std+1))
		{
			CHECK(NTV2_IS_VALID_STANDARD(std));
			const NTV2FrameGeometry fg(::GetGeometryFromStandard(std));
			LOGDBG(::NTV2StandardToString(std) << " ==> " << ::NTV2FrameGeometryToString(fg));
			CHECK(NTV2_IS_VALID_NTV2FrameGeometry(fg));
			const NTV2FrameGeometry fg_normalized(::GetNormalizedFrameGeometry(fg));
			CHECK(NTV2_IS_VALID_NTV2FrameGeometry(fg_normalized));
			for (NTV2VANCMode vancMode(NTV2_VANCMODE_TALL);  vancMode < NTV2_VANCMODE_INVALID;  vancMode = NTV2VANCMode(vancMode+1))
			{
				const NTV2FrameGeometry fg_vanc(::GetVANCFrameGeometry(fg, vancMode));
				CHECK(NTV2_IS_VALID_NTV2FrameGeometry(fg_vanc));
				if (!NTV2_IS_VANC_GEOMETRY(fg_vanc))
					{LOGDBG("No VANC geometry for " << ::NTV2FrameGeometryToString(fg_vanc)); continue;}
				CHECK(NTV2_IS_VALID_NTV2FrameGeometry(::GetNormalizedFrameGeometry(fg_vanc)));
			}
		}
		//	Check raster sizes
		for (NTV2FrameGeometry fg(NTV2_FG_1920x1080);  fg < NTV2_FG_NUMFRAMEGEOMETRIES;  fg = NTV2FrameGeometry(fg+1))
		{
			if (!NTV2_IS_VALID_NTV2FrameGeometry(fg))
				continue;	//	Skip invalid FGs
			for (NTV2PixelFormat fbf(NTV2_FBF_FIRST);  fbf < NTV2_FBF_NUMFRAMEBUFFERFORMATS;  fbf = NTV2PixelFormat(fbf+1))
			{
				if (!NTV2_IS_VALID_FRAME_BUFFER_FORMAT(fbf))
					continue;	//	Skip invalid FBFs
				if (NTV2_IS_FBF_PLANAR(fbf) || NTV2_FBF_IS_RAW(fbf) || NTV2_IS_FBF_PRORES(fbf))
					continue;	//	Skip planar, raw & ProRes formats
				if (fbf == NTV2_FBF_8BIT_DVCPRO  ||  fbf == NTV2_FBF_8BIT_HDV)
					continue;	//	Skip DVCPRO & HDV formats
				ULWord factor(ULWord(::Get8MBFrameSizeFactor(fg, fbf)));
				CHECK(factor != 0);	//	Factor must never be zero
				NTV2Standard	fs(::GetStandardFromGeometry(fg));
				CHECK(NTV2_IS_VALID_STANDARD(fs));	//	Standard from valid FG must be valid
				NTV2FormatDescriptor	fd (fs, fbf);
				CHECK(fd.IsValid());	//	FD from Standard & FBF must be valid
				ULWord	fdBytes (fd.GetTotalRasterBytes()),  factorBytes(ULWord(factor*8LL)*1024LL*1024LL);
				CHECK(fdBytes <= factorBytes);
			}
		}
	}

	TEST_CASE("NTV2Planar3Formats")
	{
		NTV2Buffer dummyBuffer(32*1024*1024);
		static const NTV2Standard	stds[]	=	{	NTV2_STANDARD_525,	NTV2_STANDARD_625,	NTV2_STANDARD_720,	NTV2_STANDARD_1080,	NTV2_STANDARD_1080p, NTV2_STANDARD_2K, NTV2_STANDARD_2Kx1080p, NTV2_STANDARD_2Kx1080i, NTV2_STANDARD_3840x2160p, NTV2_STANDARD_4096x2160p, NTV2_STANDARD_3840HFR, NTV2_STANDARD_4096HFR};
		for (unsigned stdNdx(0);  stdNdx < sizeof(stds)/sizeof(NTV2Standard);  stdNdx++)
		{
			const NTV2Standard		std			(stds[stdNdx]);
			const NTV2FormatDesc	fdv210		(std,	NTV2_FBF_10BIT_YCBCR,			NTV2_VANCMODE_OFF);
			const NTV2FormatDesc	fd8b420		(std,	NTV2_FBF_8BIT_YCBCR_420PL3,		NTV2_VANCMODE_OFF);
			const NTV2FormatDesc	fd8b422		(std,	NTV2_FBF_8BIT_YCBCR_422PL3,		NTV2_VANCMODE_OFF);
			const NTV2FormatDesc	fd10b420	(std,	NTV2_FBF_10BIT_YCBCR_420PL3_LE,	NTV2_VANCMODE_OFF);
			const NTV2FormatDesc	fd10b422	(std,	NTV2_FBF_10BIT_YCBCR_422PL3_LE,	NTV2_VANCMODE_OFF);
			const ULWord			hghtAdj		(std == NTV2_STANDARD_525  ?  6  :  0);	//	Only when comparing planar 525i with non-planar 525i formats

			CHECK(fd8b420.IsValid());		CHECK(fd8b422.IsValid());
			CHECK(fd10b420.IsValid());		CHECK(fd10b422.IsValid());
			CHECK(fd8b420.IsPlanar());		CHECK(fd8b422.IsPlanar());
			CHECK(fd10b420.IsPlanar());	CHECK(fd10b422.IsPlanar());
			CHECK_FALSE(fd8b420.IsVANC());		CHECK_FALSE(fd8b422.IsVANC());
			CHECK_FALSE(fd10b420.IsVANC());		CHECK_FALSE(fd10b422.IsVANC());
			CHECK_EQ(fd8b420.GetNumPlanes(), 3);		CHECK_EQ(fd8b422.GetNumPlanes(), 3);
			CHECK_EQ(fd10b420.GetNumPlanes(), 3);	CHECK_EQ(fd10b422.GetNumPlanes(), 3);

			CHECK_EQ(fd8b420.GetRasterHeight(),	fdv210.GetRasterHeight()-hghtAdj);
			CHECK_EQ(fd8b422.GetRasterHeight(),	fd8b420.GetRasterHeight());
			CHECK_EQ(fd10b420.GetRasterHeight(),	fd8b422.GetRasterHeight());
			CHECK_EQ(fd10b422.GetRasterHeight(),	fd10b420.GetRasterHeight());

			CHECK_EQ(fd8b420.GetRasterWidth(),	fdv210.GetRasterWidth());
			CHECK_EQ(fd8b422.GetRasterWidth(),	fd8b420.GetRasterWidth());
			CHECK_EQ(fd10b420.GetRasterWidth(),	fd8b422.GetRasterWidth());
			CHECK_EQ(fd10b422.GetRasterWidth(),	fd10b420.GetRasterWidth());

			CHECK_EQ(fd8b420.GetBytesPerRow(0),	fdv210.GetRasterWidth());
			CHECK_EQ(fd8b422.GetBytesPerRow(0),	fdv210.GetRasterWidth());
			CHECK_EQ(fd10b420.GetBytesPerRow(0),	sizeof(uint16_t)*fdv210.GetRasterWidth());
			CHECK_EQ(fd10b422.GetBytesPerRow(0),	sizeof(uint16_t)*fdv210.GetRasterWidth());

			CHECK_EQ(fd8b420.GetTotalRasterBytes(0),		fdv210.GetRasterWidth()*(fdv210.GetRasterHeight()-hghtAdj));
			CHECK_EQ(fd8b422.GetTotalRasterBytes(0),		fdv210.GetRasterWidth()*(fdv210.GetRasterHeight()-hghtAdj));
			CHECK_EQ(fd10b420.GetTotalRasterBytes(0),	sizeof(uint16_t)*fdv210.GetRasterWidth()*(fdv210.GetRasterHeight()-hghtAdj));
			CHECK_EQ(fd10b422.GetTotalRasterBytes(0),	sizeof(uint16_t)*fdv210.GetRasterWidth()*(fdv210.GetRasterHeight()-hghtAdj));

			//	Row bytes of Cb plane should be half the Y rowbytes in 3-plane formats:
			CHECK_EQ(fd8b420.GetBytesPerRow(1),	fd8b420.GetBytesPerRow()/2);
			CHECK_EQ(fd8b422.GetBytesPerRow(1),	fd8b422.GetBytesPerRow()/2);
			CHECK_EQ(fd10b420.GetBytesPerRow(1),	fd10b420.GetBytesPerRow()/2);
			CHECK_EQ(fd10b422.GetBytesPerRow(1),	fd10b422.GetBytesPerRow()/2);

			//	Row bytes of Cr plane should be same as Cb plane
			CHECK_EQ(fd8b420.GetBytesPerRow(2),	fd8b420.GetBytesPerRow(1));
			CHECK_EQ(fd8b422.GetBytesPerRow(2),	fd8b422.GetBytesPerRow(1));
			CHECK_EQ(fd10b420.GetBytesPerRow(2),	fd10b420.GetBytesPerRow(1));
			CHECK_EQ(fd10b422.GetBytesPerRow(2),	fd10b422.GetBytesPerRow(1));

			//	Size of Cb plane should be 1/4th the Y plane in 4:2:0, 1/2 in 4:2:2
			CHECK_EQ(fd8b420.GetTotalRasterBytes(1),		fd8b420.GetTotalRasterBytes(0)/4);
			CHECK_EQ(fd8b422.GetTotalRasterBytes(1),		fd8b422.GetTotalRasterBytes(0)/2);
			CHECK_EQ(fd10b420.GetTotalRasterBytes(1),	fd10b420.GetTotalRasterBytes(0)/4);
			CHECK_EQ(fd10b422.GetTotalRasterBytes(1),	fd10b422.GetTotalRasterBytes(0)/2);

			//	Size of Cr plane should be same as Cb plane
			CHECK_EQ(fd8b420.GetTotalRasterBytes(2),		fd8b420.GetTotalRasterBytes(1));
			CHECK_EQ(fd8b422.GetTotalRasterBytes(2),		fd8b422.GetTotalRasterBytes(1));
			CHECK_EQ(fd10b420.GetTotalRasterBytes(2),	fd10b420.GetTotalRasterBytes(1));
			CHECK_EQ(fd10b422.GetTotalRasterBytes(2),	fd10b422.GetTotalRasterBytes(1));

			//	Verify plane labels
			CHECK_EQ(fd8b420.PlaneToString(0),			fd8b422.PlaneToString(0));
			CHECK_EQ(fd8b420.PlaneToString(1),			fd8b422.PlaneToString(1));
			CHECK_EQ(fd8b420.PlaneToString(2),			fd8b422.PlaneToString(2));
			CHECK_EQ(fd10b420.PlaneToString(0),			fd10b422.PlaneToString(0));
			CHECK_EQ(fd10b420.PlaneToString(1),			fd10b422.PlaneToString(1));
			CHECK_EQ(fd10b420.PlaneToString(2),			fd10b422.PlaneToString(2));
			CHECK_EQ(fd8b420.PlaneToString(0),			fd10b420.PlaneToString(0));
			CHECK_EQ(fd8b420.PlaneToString(1),			fd10b420.PlaneToString(1));
			CHECK_EQ(fd8b420.PlaneToString(2),			fd10b420.PlaneToString(2));

			//	Spot-check address calculations
			CHECK(fd8b420.GetRowAddress(dummyBuffer,fd8b420.GetRasterHeight(),0) == NULL);	//	Past bottom
			CHECK(fd8b422.GetRowAddress(dummyBuffer,fd8b422.GetRasterHeight(),0) == NULL);	//	Past bottom
			CHECK(fd10b420.GetRowAddress(dummyBuffer,fd10b420.GetRasterHeight(),0) == NULL);	//	Past bottom
			CHECK(fd10b422.GetRowAddress(dummyBuffer,fd10b422.GetRasterHeight(),0) == NULL);	//	Past bottom
			//	Y plane line offsets should match between 4:2:0 and 4:2:2
			CHECK_EQ(fd8b420.GetRowAddress(dummyBuffer,15,0),		fd8b422.GetRowAddress(dummyBuffer,15,0));
			CHECK_EQ(fd10b420.GetRowAddress(dummyBuffer,15,0),		fd10b422.GetRowAddress(dummyBuffer,15,0));
			//	Distance, in bytes, between line 200 in Cb and Cr planes should match size of chroma plane
			CHECK_EQ(uint64_t(fd8b420.GetRowAddress(dummyBuffer,200,2)) - uint64_t(fd8b420.GetRowAddress(dummyBuffer,200,1)),	uint64_t(fd8b420.GetTotalRasterBytes(1)));
			CHECK_EQ(uint64_t(fd8b422.GetRowAddress(dummyBuffer,200,2)) - uint64_t(fd8b422.GetRowAddress(dummyBuffer,200,1)),	uint64_t(fd8b422.GetTotalRasterBytes(1)));
			CHECK_EQ(uint64_t(fd10b420.GetRowAddress(dummyBuffer,200,2)) - uint64_t(fd10b420.GetRowAddress(dummyBuffer,200,1)),	uint64_t(fd10b420.GetTotalRasterBytes(1)));
			CHECK_EQ(uint64_t(fd10b422.GetRowAddress(dummyBuffer,200,2)) - uint64_t(fd10b422.GetRowAddress(dummyBuffer,200,1)),	uint64_t(fd10b422.GetTotalRasterBytes(1)));
			//	Line offset checks
			CHECK_EQ(fd8b420.ByteOffsetToRasterLine(fd8b420.GetTotalRasterBytes(2)+fd8b420.GetTotalRasterBytes(1)+fd8b420.GetTotalRasterBytes(0)),	0xFFFF);
			CHECK_EQ(fd8b422.ByteOffsetToRasterLine(fd8b422.GetTotalRasterBytes(2)+fd8b422.GetTotalRasterBytes(1)+fd8b422.GetTotalRasterBytes(0)),	0xFFFF);
			CHECK_EQ(fd10b420.ByteOffsetToRasterLine(fd10b420.GetTotalRasterBytes(2)+fd10b420.GetTotalRasterBytes(1)+fd10b420.GetTotalRasterBytes(0)),	0xFFFF);
			CHECK_EQ(fd10b422.ByteOffsetToRasterLine(fd10b422.GetTotalRasterBytes(2)+fd10b422.GetTotalRasterBytes(1)+fd10b422.GetTotalRasterBytes(0)),	0xFFFF);
			CHECK_EQ(fd8b420.ByteOffsetToRasterLine(ULWord(uint64_t(fd8b420.GetRowAddress(dummyBuffer,200,0)) - uint64_t(dummyBuffer.GetHostAddress(0)))),	200);
			CHECK_EQ(fd8b420.ByteOffsetToRasterLine(ULWord(uint64_t(fd8b420.GetRowAddress(dummyBuffer,200,1)) - uint64_t(dummyBuffer.GetHostAddress(0)))),	200);
			CHECK_EQ(fd8b420.ByteOffsetToRasterLine(ULWord(uint64_t(fd8b420.GetRowAddress(dummyBuffer,200,2)) - uint64_t(dummyBuffer.GetHostAddress(0)))),	200);
			CHECK_EQ(fd8b422.ByteOffsetToRasterLine(ULWord(uint64_t(fd8b422.GetRowAddress(dummyBuffer,200,0)) - uint64_t(dummyBuffer.GetHostAddress(0)))),	200);
			CHECK_EQ(fd8b422.ByteOffsetToRasterLine(ULWord(uint64_t(fd8b422.GetRowAddress(dummyBuffer,200,1)) - uint64_t(dummyBuffer.GetHostAddress(0)))),	200);
			CHECK_EQ(fd8b422.ByteOffsetToRasterLine(ULWord(uint64_t(fd8b422.GetRowAddress(dummyBuffer,200,2)) - uint64_t(dummyBuffer.GetHostAddress(0)))),	200);
			CHECK_EQ(fd10b420.ByteOffsetToRasterLine(ULWord(uint64_t(fd10b420.GetRowAddress(dummyBuffer,200,0)) - uint64_t(dummyBuffer.GetHostAddress(0)))),	200);
			CHECK_EQ(fd10b420.ByteOffsetToRasterLine(ULWord(uint64_t(fd10b420.GetRowAddress(dummyBuffer,200,1)) - uint64_t(dummyBuffer.GetHostAddress(0)))),	200);
			CHECK_EQ(fd10b420.ByteOffsetToRasterLine(ULWord(uint64_t(fd10b420.GetRowAddress(dummyBuffer,200,2)) - uint64_t(dummyBuffer.GetHostAddress(0)))),	200);
			CHECK_EQ(fd10b422.ByteOffsetToRasterLine(ULWord(uint64_t(fd10b422.GetRowAddress(dummyBuffer,200,0)) - uint64_t(dummyBuffer.GetHostAddress(0)))),	200);
			CHECK_EQ(fd10b422.ByteOffsetToRasterLine(ULWord(uint64_t(fd10b422.GetRowAddress(dummyBuffer,200,1)) - uint64_t(dummyBuffer.GetHostAddress(0)))),	200);
			CHECK_EQ(fd10b422.ByteOffsetToRasterLine(ULWord(uint64_t(fd10b422.GetRowAddress(dummyBuffer,200,2)) - uint64_t(dummyBuffer.GetHostAddress(0)))),	200);
		}	//	for each NTV2Standard
		{
			//	Test line offsets for VANC FBFs
			const NTV2FormatDescriptor	fdNoVanc		(NTV2_STANDARD_1080p,	NTV2_FBF_10BIT_YCBCR,	NTV2_VANCMODE_OFF);
			const NTV2FormatDescriptor	fdVancTall		(NTV2_STANDARD_1080p,	NTV2_FBF_10BIT_YCBCR,	NTV2_VANCMODE_TALL);
			const NTV2FormatDescriptor	fdVancTaller	(NTV2_STANDARD_1080p,	NTV2_FBF_10BIT_YCBCR,	NTV2_VANCMODE_TALLER);
			CHECK_EQ(fdNoVanc.ByteOffsetToRasterLine(fdNoVanc.GetTotalRasterBytes()),	0xFFFF);
			CHECK_EQ(fdNoVanc.ByteOffsetToRasterLine(ULWord(uint64_t(fdNoVanc.GetTopVisibleRowAddress(dummyBuffer)) - uint64_t(dummyBuffer.GetHostAddress(0)))),	fdNoVanc.GetFirstActiveLine());
			CHECK_EQ(fdVancTall.ByteOffsetToRasterLine(fdVancTall.GetTotalRasterBytes()),	0xFFFF);
			CHECK_EQ(fdVancTall.ByteOffsetToRasterLine(ULWord(uint64_t(fdVancTall.GetTopVisibleRowAddress(dummyBuffer)) - uint64_t(dummyBuffer.GetHostAddress(0)))),	fdVancTall.GetFirstActiveLine());
			CHECK_EQ(fdVancTaller.ByteOffsetToRasterLine(fdVancTaller.GetTotalRasterBytes()),	0xFFFF);
			CHECK_EQ(fdVancTaller.ByteOffsetToRasterLine(ULWord(uint64_t(fdVancTaller.GetTopVisibleRowAddress(dummyBuffer)) - uint64_t(dummyBuffer.GetHostAddress(0)))),	fdVancTaller.GetFirstActiveLine());
		}
	}

	TEST_CASE("NTV2WidgetCount")
	{
		//	This test exposes contradictions between NTV2DeviceGetNumXXXXX and NTV2DeviceCanDoWidget...
		const NTV2DeviceIDSet	devices(::NTV2GetSupportedDevices());
		static const NTV2WidgetID	sWgtMixers[]		= {	NTV2_WgtMixer1, NTV2_WgtMixer2, NTV2_WgtMixer3, NTV2_WgtMixer4	};
		static const NTV2WidgetID	sWgtCSCs[]			= {	NTV2_WgtCSC1, NTV2_WgtCSC2, NTV2_WgtCSC3, NTV2_WgtCSC4, NTV2_WgtCSC5, NTV2_WgtCSC6, NTV2_WgtCSC7, NTV2_WgtCSC8	};
		static const NTV2WidgetID	sWgtLUTs[]			= {	NTV2_WgtLUT1, NTV2_WgtLUT2, NTV2_WgtLUT3, NTV2_WgtLUT4, NTV2_WgtLUT5, NTV2_WgtLUT6, NTV2_WgtLUT7, NTV2_WgtLUT8	};
		static const NTV2WidgetID	sWgtFrameStores[]	= {	NTV2_WgtFrameBuffer1, NTV2_WgtFrameBuffer2, NTV2_WgtFrameBuffer3, NTV2_WgtFrameBuffer4, NTV2_WgtFrameBuffer5, NTV2_WgtFrameBuffer6, NTV2_WgtFrameBuffer7, NTV2_WgtFrameBuffer8	};

		for (NTV2DeviceIDSetConstIter iter(devices.begin());  iter != devices.end();  ++iter)
		{
			const NTV2DeviceID	deviceID	(*iter);
			const UWord			numMixers(::NTV2DeviceGetNumMixers(deviceID));
			const UWord			numCSCs(::NTV2DeviceGetNumCSCs(deviceID));
			const UWord			numLUTs(::NTV2DeviceGetNumLUTs(deviceID));
			const UWord			numFrameStores(::NTV2DeviceGetNumFrameStores(deviceID));
			UWord				mixerTally(0), CSCTally(0), LUTTally(0), frameStoreTally(0);

			std::cerr << "Checking " << ::NTV2DeviceIDString(deviceID) << " (" << ::NTV2DeviceIDToString(deviceID) << ")..." << std::endl;
			//	Mixers
			for (unsigned ndx(0);  ndx < sizeof(sWgtMixers) / sizeof(NTV2WidgetID);  ndx++)
				if (::NTV2DeviceCanDoWidget(deviceID, sWgtMixers[ndx]))
					mixerTally++;
			CHECK_EQ(numMixers, mixerTally);

			//	CSCs
			for (unsigned ndx(0);  ndx < sizeof(sWgtCSCs) / sizeof(NTV2WidgetID);  ndx++)
				if (::NTV2DeviceCanDoWidget(deviceID, sWgtCSCs[ndx]))
					CSCTally++;
			CHECK_EQ(numCSCs, CSCTally);

			//	LUTs
			for (unsigned ndx(0);  ndx < sizeof(sWgtLUTs) / sizeof(NTV2WidgetID);  ndx++)
				if (::NTV2DeviceCanDoWidget(deviceID, sWgtLUTs[ndx]))
					LUTTally++;
			CHECK_EQ(numLUTs, LUTTally);

			//	FrameStores
			for (unsigned ndx(0);  ndx < sizeof(sWgtFrameStores) / sizeof(NTV2WidgetID);  ndx++)
				if (::NTV2DeviceCanDoWidget(deviceID, sWgtFrameStores[ndx]))
					frameStoreTally++;
			CHECK_EQ(numFrameStores, frameStoreTally);

		}	//	for each supported device
	}

	TEST_CASE("NTV2Transcode")
	{
		//	Make a '2vuy' line to test with...
		std::vector<UByte>	testLine2VUY;
		for (unsigned ndx(0);  ndx < 1440;  ndx++)
			testLine2VUY.push_back(UByte(ndx%256));
		NTV2Buffer	bufferV210(sizeof(UWord)*1440);
		NTV2Buffer	buffer2VUY(sizeof(UByte)*1440);
		std::vector<uint8_t>	compLine2VUY;
		CHECK_FALSE(::ConvertLine_2vuy_to_v210(NULL, reinterpret_cast<ULWord*>(bufferV210.GetHostPointer()), 720));	//	NULL src ptr
		CHECK_FALSE(::ConvertLine_2vuy_to_v210(&testLine2VUY[0], NULL, 720));	//	NULL dst ptr
		CHECK_FALSE(::ConvertLine_2vuy_to_v210(&testLine2VUY[0], reinterpret_cast<ULWord*>(bufferV210.GetHostPointer()), 0));	//	0 pixel count
		CHECK(::ConvertLine_2vuy_to_v210(&testLine2VUY[0], reinterpret_cast<ULWord*>(bufferV210.GetHostPointer()), 720));

		CHECK_FALSE(::ConvertLine_v210_to_2vuy(NULL, reinterpret_cast<UByte*>(buffer2VUY.GetHostPointer()), 720));	//	NULL src
		CHECK_FALSE(::ConvertLine_v210_to_2vuy(reinterpret_cast<const ULWord*>(bufferV210.GetHostPointer()), NULL, 720));	//	NULL dst
		CHECK_FALSE(::ConvertLine_v210_to_2vuy(reinterpret_cast<const ULWord*>(bufferV210.GetHostPointer()), reinterpret_cast<UByte*>(buffer2VUY.GetHostPointer()), 0));	//	0 pix count
		CHECK_FALSE(::ConvertLine_v210_to_2vuy(NULL, compLine2VUY, 720));	//	NULL src
		CHECK_FALSE(::ConvertLine_v210_to_2vuy(reinterpret_cast<const ULWord*>(bufferV210.GetHostPointer()), compLine2VUY, 0));	//	0 pix cnt

		CHECK(::ConvertLine_v210_to_2vuy(reinterpret_cast<const ULWord*>(bufferV210.GetHostPointer()), reinterpret_cast<UByte*>(buffer2VUY.GetHostPointer()), 720));
		CHECK(::ConvertLine_v210_to_2vuy(reinterpret_cast<const ULWord*>(bufferV210.GetHostPointer()), compLine2VUY, 720));
		CHECK_EQ(::memcmp(buffer2VUY.GetHostPointer(), &compLine2VUY[0], compLine2VUY.size()), 0);
	}

	TEST_CASE("NTV2Bitfile")
	{
		static unsigned char sTTapPro[] = { //	.............a.Et_tap_pro;COMPRESS=TRUE;UserID=0XFFFFFFFF;TANDEM=TRUE;Version=2019.1.b..xcku035-fbva676-1LV-i.c..2020/11/04.d..14:58:54.e..'......................................................................".D..........Uf ... ...0. .....0.......0......
			0x00, 0x09, 0x0f, 0xf0, 0x0f, 0xf0, 0x0f, 0xf0, 0x0f, 0xf0, 0x00, 0x00, 0x01, 0x61, 0x00, 0x45, 0x74, 0x5f, 0x74, 0x61, 0x70, 0x5f, 0x70, 0x72, 0x6f, 0x3b, 0x43, 0x4f, 0x4d, 0x50, 0x52, 0x45, 0x53, 0x53, 0x3d, 0x54, 0x52, 0x55, 0x45, 0x3b, 0x55, 0x73, 0x65, 0x72, 0x49, 0x44, 0x3d, 0x30, 0x58, 0x46, 0x46, 0x46, 0x46, 0x46, 0x46, 0x46, 0x46, 0x3b, 0x54, 0x41, 0x4e, 0x44, 0x45, 0x4d,
			0x3d, 0x54, 0x52, 0x55, 0x45, 0x3b, 0x56, 0x65, 0x72, 0x73, 0x69, 0x6f, 0x6e, 0x3d, 0x32, 0x30, 0x31, 0x39, 0x2e, 0x31, 0x00, 0x62, 0x00, 0x16, 0x78, 0x63, 0x6b, 0x75, 0x30, 0x33, 0x35, 0x2d, 0x66, 0x62, 0x76, 0x61, 0x36, 0x37, 0x36, 0x2d, 0x31, 0x4c, 0x56, 0x2d, 0x69, 0x00, 0x63, 0x00, 0x0b, 0x32, 0x30, 0x32, 0x30, 0x2f, 0x31, 0x31, 0x2f, 0x30, 0x34, 0x00, 0x64, 0x00, 0x09, 0x31,
			0x34, 0x3a, 0x35, 0x38, 0x3a, 0x35, 0x34, 0x00, 0x65, 0x00, 0xd2, 0x27, 0xd4, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
			0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0xbb, 0x11, 0x22, 0x00, 0x44, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xaa, 0x99, 0x55, 0x66, 0x20, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x30, 0x02, 0x20, 0x01, 0x00, 0x00, 0x00, 0x00, 0x30, 0x02, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x80, 0x01, 0x00, 0x00, 0x00	};

		const NTV2Buffer TTapPro(sTTapPro, sizeof(sTTapPro));
		NTV2Buffer sub(TTapPro.GetHostAddress(0), TTapPro.GetByteCount());
		CNTV2Bitfile bf;
		std::string err;

		SUBCASE("Bitfile Header Parsing")
		{

			while (true) {
				err = bf.ParseHeaderFromBuffer(sub);
				if (sub.GetByteCount() < 225) {
					CHECK_NE(err, "");
				}
				else {
					CHECK_EQ(err, "");
					CHECK_EQ(bf.GetDate(), "2020/11/04");
					CHECK_EQ(bf.GetTime(), "14:58:54");
				}

				if (!sub)
					break;

				sub.Set(sub.GetHostAddress(0), sub.GetByteCount()-1);
			}
		}

		SUBCASE("Bitfile Section Parsing")
		{
			uint16_t aSectionExpected(NTV2EndianSwap16BtoH(TTapPro.U16(7)));
			uint16_t bSectionExpected(NTV2EndianSwap16BtoH(TTapPro.U16(43)));
			uint16_t cSectionExpected(uint16_t(TTapPro.U8(86+16+9) << 8) | uint16_t(TTapPro.U8(86+16+10)));
			CHECK_EQ(cSectionExpected, 0x000B);
			uint16_t dSectionExpected(uint16_t(TTapPro.U8(125) << 8) | uint16_t(TTapPro.U8(126)));
			CHECK_EQ(dSectionExpected, 0x0009);

			NTV2Buffer aSectionBad(TTapPro);
			NTV2Buffer bSectionBad(TTapPro);
			NTV2Buffer cSectionBad(TTapPro);
			NTV2Buffer dSectionBad(TTapPro);
			for (uint16_t fnLen(0);	 fnLen < 0xFFFF;  fnLen++) {

				// A Section -- bad FileName length test
				aSectionBad.U16(7) = NTV2EndianSwap16HtoB(fnLen);
				err = bf.ParseHeaderFromBuffer(aSectionBad);
				if (fnLen != aSectionExpected) {
					CHECK_NE(err, "");
				}
				else {
					CHECK_EQ(err, "");
				}

				// B Section -- bad PartName length test
				bSectionBad.U16(43) = NTV2EndianSwap16HtoB(fnLen);
				err = bf.ParseHeaderFromBuffer(bSectionBad);
				if (fnLen != bSectionExpected) {
					CHECK_NE(err, "");
				}
				else {
					CHECK_EQ(err, "");
				}

				// C Section -- bad DateName length test
				cSectionBad.U8(86+16+9) = uint8_t(fnLen >> 8);
				cSectionBad.U8(86+16+10) = uint8_t(fnLen & 0x00FF);
				err = bf.ParseHeaderFromBuffer(cSectionBad);
				if (fnLen != cSectionExpected) {
					CHECK_NE(err, "");
				}
				else {
					CHECK_EQ(err, "");
				}

				// D Section -- bad TimeName length test
				dSectionBad.U8(125) = uint8_t(fnLen >> 8);
				dSectionBad.U8(126) = uint8_t(fnLen & 0x00FF);
				err = bf.ParseHeaderFromBuffer(dSectionBad);
				if (fnLen != dSectionExpected) {
					CHECK_NE(err, "");
				}
				else {
					CHECK_EQ(err, "");
				}
			}
		}
	}

	// TODO
	// static bool NTV2SignalRouterBFT (void)
	// {
	// 	LOGNOTE("Started");
	// 	if (true)
	// 	{	//	Test GetFrameBufferOutputXptFromChannel
	// 		const bool	kIsNotRGB	(false);
	// 		const bool	kIsRGB		(true);
	// 		const bool	kIsNot425	(false);
	// 		const bool	kIs425		(true);

	// 		SHOULD_BE_EQUAL(::GetFrameBufferOutputXptFromChannel(NTV2_CHANNEL1, kIsNotRGB, kIsNot425), NTV2_XptFrameBuffer1YUV);
	// 		SHOULD_BE_EQUAL(::GetFrameBufferOutputXptFromChannel(NTV2_CHANNEL2, kIsNotRGB, kIsNot425), NTV2_XptFrameBuffer2YUV);
	// 		SHOULD_BE_EQUAL(::GetFrameBufferOutputXptFromChannel(NTV2_CHANNEL3, kIsNotRGB, kIsNot425), NTV2_XptFrameBuffer3YUV);
	// 		SHOULD_BE_EQUAL(::GetFrameBufferOutputXptFromChannel(NTV2_CHANNEL4, kIsNotRGB, kIsNot425), NTV2_XptFrameBuffer4YUV);
	// 		SHOULD_BE_EQUAL(::GetFrameBufferOutputXptFromChannel(NTV2_CHANNEL5, kIsNotRGB, kIsNot425), NTV2_XptFrameBuffer5YUV);
	// 		SHOULD_BE_EQUAL(::GetFrameBufferOutputXptFromChannel(NTV2_CHANNEL6, kIsNotRGB, kIsNot425), NTV2_XptFrameBuffer6YUV);
	// 		SHOULD_BE_EQUAL(::GetFrameBufferOutputXptFromChannel(NTV2_CHANNEL7, kIsNotRGB, kIsNot425), NTV2_XptFrameBuffer7YUV);
	// 		SHOULD_BE_EQUAL(::GetFrameBufferOutputXptFromChannel(NTV2_CHANNEL8, kIsNotRGB, kIsNot425), NTV2_XptFrameBuffer8YUV);
	// 		SHOULD_BE_EQUAL(::GetFrameBufferOutputXptFromChannel(NTV2_CHANNEL1, kIsNotRGB, kIs425), NTV2_XptFrameBuffer1_DS2YUV);
	// 		SHOULD_BE_EQUAL(::GetFrameBufferOutputXptFromChannel(NTV2_CHANNEL2, kIsNotRGB, kIs425), NTV2_XptFrameBuffer2_DS2YUV);
	// 		SHOULD_BE_EQUAL(::GetFrameBufferOutputXptFromChannel(NTV2_CHANNEL3, kIsNotRGB, kIs425), NTV2_XptFrameBuffer3_DS2YUV);
	// 		SHOULD_BE_EQUAL(::GetFrameBufferOutputXptFromChannel(NTV2_CHANNEL4, kIsNotRGB, kIs425), NTV2_XptFrameBuffer4_DS2YUV);
	// 		SHOULD_BE_EQUAL(::GetFrameBufferOutputXptFromChannel(NTV2_CHANNEL5, kIsNotRGB, kIs425), NTV2_XptFrameBuffer5_DS2YUV);
	// 		SHOULD_BE_EQUAL(::GetFrameBufferOutputXptFromChannel(NTV2_CHANNEL6, kIsNotRGB, kIs425), NTV2_XptFrameBuffer6_DS2YUV);
	// 		SHOULD_BE_EQUAL(::GetFrameBufferOutputXptFromChannel(NTV2_CHANNEL7, kIsNotRGB, kIs425), NTV2_XptFrameBuffer7_DS2YUV);
	// 		SHOULD_BE_EQUAL(::GetFrameBufferOutputXptFromChannel(NTV2_CHANNEL8, kIsNotRGB, kIs425), NTV2_XptFrameBuffer8_DS2YUV);
	// 		SHOULD_BE_EQUAL(::GetFrameBufferOutputXptFromChannel(NTV2_CHANNEL1, kIsRGB, kIsNot425), NTV2_XptFrameBuffer1RGB);
	// 		SHOULD_BE_EQUAL(::GetFrameBufferOutputXptFromChannel(NTV2_CHANNEL2, kIsRGB, kIsNot425), NTV2_XptFrameBuffer2RGB);
	// 		SHOULD_BE_EQUAL(::GetFrameBufferOutputXptFromChannel(NTV2_CHANNEL3, kIsRGB, kIsNot425), NTV2_XptFrameBuffer3RGB);
	// 		SHOULD_BE_EQUAL(::GetFrameBufferOutputXptFromChannel(NTV2_CHANNEL4, kIsRGB, kIsNot425), NTV2_XptFrameBuffer4RGB);
	// 		SHOULD_BE_EQUAL(::GetFrameBufferOutputXptFromChannel(NTV2_CHANNEL5, kIsRGB, kIsNot425), NTV2_XptFrameBuffer5RGB);
	// 		SHOULD_BE_EQUAL(::GetFrameBufferOutputXptFromChannel(NTV2_CHANNEL6, kIsRGB, kIsNot425), NTV2_XptFrameBuffer6RGB);
	// 		SHOULD_BE_EQUAL(::GetFrameBufferOutputXptFromChannel(NTV2_CHANNEL7, kIsRGB, kIsNot425), NTV2_XptFrameBuffer7RGB);
	// 		SHOULD_BE_EQUAL(::GetFrameBufferOutputXptFromChannel(NTV2_CHANNEL8, kIsRGB, kIsNot425), NTV2_XptFrameBuffer8RGB);
	// 		SHOULD_BE_EQUAL(::GetFrameBufferOutputXptFromChannel(NTV2_CHANNEL1, kIsRGB, kIs425), NTV2_XptFrameBuffer1_DS2RGB);
	// 		SHOULD_BE_EQUAL(::GetFrameBufferOutputXptFromChannel(NTV2_CHANNEL2, kIsRGB, kIs425), NTV2_XptFrameBuffer2_DS2RGB);
	// 		SHOULD_BE_EQUAL(::GetFrameBufferOutputXptFromChannel(NTV2_CHANNEL3, kIsRGB, kIs425), NTV2_XptFrameBuffer3_DS2RGB);
	// 		SHOULD_BE_EQUAL(::GetFrameBufferOutputXptFromChannel(NTV2_CHANNEL4, kIsRGB, kIs425), NTV2_XptFrameBuffer4_DS2RGB);
	// 		SHOULD_BE_EQUAL(::GetFrameBufferOutputXptFromChannel(NTV2_CHANNEL5, kIsRGB, kIs425), NTV2_XptFrameBuffer5_DS2RGB);
	// 		SHOULD_BE_EQUAL(::GetFrameBufferOutputXptFromChannel(NTV2_CHANNEL6, kIsRGB, kIs425), NTV2_XptFrameBuffer6_DS2RGB);
	// 		SHOULD_BE_EQUAL(::GetFrameBufferOutputXptFromChannel(NTV2_CHANNEL7, kIsRGB, kIs425), NTV2_XptFrameBuffer7_DS2RGB);
	// 		SHOULD_BE_EQUAL(::GetFrameBufferOutputXptFromChannel(NTV2_CHANNEL8, kIsRGB, kIs425), NTV2_XptFrameBuffer8_DS2RGB);
	// 	}
	// 	if (false)
	// 	{	//	This generates the NTV2OutputCrosspointID enum declarations -- they should compare:
	// 		NTV2OutputXptIDSet legitOutputXpts;
	// 		for (UWord oxpt(NTV2_FIRST_OUTPUT_CROSSPOINT);  oxpt < UWord(NTV2_LAST_OUTPUT_CROSSPOINT);  oxpt++)
	// 		{
	// 			const string oxptName(::NTV2OutputCrosspointIDToString(NTV2OutputXptID(oxpt)));
	// 			if (!oxptName.empty())
	// 				legitOutputXpts.insert(NTV2OutputXptID(oxpt));
	// 		}
	// 		while (!legitOutputXpts.empty())
	// 		{
	// 			const NTV2OutputXptID yuvXpt(*legitOutputXpts.begin());
	// 			const string yuvName(::NTV2OutputCrosspointIDToString(yuvXpt));
	// 			SHOULD_BE_FALSE(yuvName.empty());
	// 			legitOutputXpts.erase(yuvXpt);
	// 			cout << yuvName << "= " << xHEX0N(yuvXpt,2) << "," << endl;

	// 			const NTV2OutputXptID rgbXpt(NTV2OutputXptID(yuvXpt | 0x80));
	// 			if (legitOutputXpts.find(rgbXpt) == legitOutputXpts.end())
	// 				continue;
	// 			const string rgbName(::NTV2OutputCrosspointIDToString(rgbXpt));
	// 			SHOULD_BE_FALSE(rgbName.empty());
	// 			legitOutputXpts.erase(rgbXpt);
	// 			cout << rgbName << "= " << yuvName << " | 0x80," << endl;
	// 		}
	// 	}
	// 	if (true)
	// 	{	//	Test CNTV2SignalRouter
	// 		CNTV2SignalRouter routerA, routerB;
	// 		NTV2XptConnections adds, chgs, dels;
	// 		SHOULD_BE_TRUE(routerA.IsEmpty());
	// 		SHOULD_BE_EQUAL(routerA.GetNumberOfConnections(), 0);
	// 		SHOULD_BE_FALSE(routerA.HasInput(NTV2_XptCSC3VidInput));
	// 		SHOULD_BE_FALSE(routerA.HasConnection(NTV2_XptCSC1VidInput, NTV2_XptFrameBuffer2YUV));
	// 		SHOULD_BE_TRUE(routerA.Compare(routerB, adds, chgs, dels));
	// 		SHOULD_BE_TRUE(adds.empty());	SHOULD_BE_TRUE(chgs.empty());	SHOULD_BE_TRUE(dels.empty());
	// 		SHOULD_BE_TRUE(routerB.IsEmpty());
	// 		SHOULD_BE_EQUAL(routerA.GetNumberOfConnections(), 0);
	// 		SHOULD_BE_TRUE(routerB.Compare(routerA, adds, chgs, dels));
	// 		SHOULD_BE_TRUE(adds.empty());	SHOULD_BE_TRUE(chgs.empty());	SHOULD_BE_TRUE(dels.empty());

	// 		//	routerA connects CSC1 <== FB1,  routerB empty
	// 		SHOULD_BE_TRUE(routerA.AddConnection(NTV2_XptCSC1VidInput, NTV2_XptFrameBuffer1YUV));
	// 		SHOULD_BE_FALSE(routerA.IsEmpty());
	// 		SHOULD_BE_EQUAL(routerA.GetNumberOfConnections(), 1);
	// 		SHOULD_BE_TRUE(routerA.HasInput(NTV2_XptCSC1VidInput));
	// 		SHOULD_BE_TRUE(routerA.HasConnection(NTV2_XptCSC1VidInput, NTV2_XptFrameBuffer1YUV));
	// 		SHOULD_BE_FALSE(routerA.HasInput(NTV2_XptCSC3VidInput));
	// 		SHOULD_BE_FALSE(routerA.HasConnection(NTV2_XptCSC1VidInput, NTV2_XptFrameBuffer2YUV));
	// 		SHOULD_BE_TRUE(routerA != routerB);
	// 		SHOULD_BE_FALSE(routerA == routerB);
	// 		SHOULD_BE_FALSE(routerA.Compare(routerB, adds, chgs, dels));
	// 		SHOULD_BE_FALSE(adds.empty());	SHOULD_BE_TRUE(chgs.empty());	SHOULD_BE_TRUE(dels.empty());
	// 		SHOULD_BE_TRUE(adds.size() == 1);
	// 		SHOULD_BE_TRUE(adds.begin()->first == NTV2_XptCSC1VidInput);
	// 		SHOULD_BE_TRUE(adds.begin()->second == NTV2_XptFrameBuffer1YUV);
	// 		SHOULD_BE_FALSE(routerB.Compare(routerA, adds, chgs, dels));
	// 		SHOULD_BE_TRUE(adds.empty());	SHOULD_BE_TRUE(chgs.empty());	SHOULD_BE_FALSE(dels.empty());
	// 		SHOULD_BE_TRUE(dels.size() == 1);
	// 		SHOULD_BE_TRUE(dels.begin()->first == NTV2_XptCSC1VidInput);
	// 		SHOULD_BE_TRUE(dels.begin()->second == NTV2_XptFrameBuffer1YUV);

	// 		//	routerA & routerB both connect CSC1 <== FB1
	// 		SHOULD_BE_TRUE(routerB.AddConnection(NTV2_XptCSC1VidInput, NTV2_XptFrameBuffer1YUV));
	// 		SHOULD_BE_EQUAL(routerA.IsEmpty(), routerB.IsEmpty());
	// 		SHOULD_BE_EQUAL(routerA.GetNumberOfConnections(), routerB.GetNumberOfConnections());
	// 		SHOULD_BE_TRUE(routerA == routerB);
	// 		SHOULD_BE_FALSE(routerA != routerB);
	// 		SHOULD_BE_TRUE(routerB.Compare(routerA, adds, chgs, dels));
	// 		SHOULD_BE_TRUE(adds.empty());	SHOULD_BE_TRUE(chgs.empty());	SHOULD_BE_TRUE(dels.empty());

	// 		//	routerA connects CSC1 <== FB1,  routerB connects CSC1 <= FB2
	// 		routerB.Reset();
	// 		SHOULD_BE_TRUE(routerB.AddConnection(NTV2_XptCSC1VidInput, NTV2_XptFrameBuffer2YUV));
	// 		SHOULD_BE_EQUAL(routerA.IsEmpty(), routerB.IsEmpty());
	// 		SHOULD_BE_EQUAL(routerA.GetNumberOfConnections(), routerB.GetNumberOfConnections());
	// 		SHOULD_BE_TRUE(routerA != routerB);
	// 		SHOULD_BE_FALSE(routerA == routerB);
	// 		string	code;
	// 		CNTV2SignalRouter::PrintCodeConfig	config;
	// 		SHOULD_BE_FALSE(routerA.Compare(routerB, config.mNew, config.mChanged, config.mMissing));
	// 		SHOULD_BE_TRUE(config.mNew.empty());	SHOULD_BE_FALSE(config.mChanged.empty());	SHOULD_BE_TRUE(config.mMissing.empty());
	// 		cerr << config.mChanged << endl;
	// 		routerA.PrintCode (code, config); cerr << code << endl;
	// 	}
	// 	if (true)
	// 	{	//	Test NTV2XptConnections, CompareConnections
	// 		NTV2XptConnections routingA, routingB, newConns, delConns;
	// 		//	A == B
	// 		routingA[NTV2_XptCSC1VidInput] = NTV2_XptFrameBuffer1YUV;
	// 		routingB[NTV2_XptCSC1VidInput] = NTV2_XptFrameBuffer1YUV;
	// 		SHOULD_BE_TRUE(CNTV2SignalRouter::CompareConnections(routingA, routingB, newConns, delConns));
	// 		SHOULD_BE_TRUE(newConns.empty());	//	Nothing added
	// 		SHOULD_BE_TRUE(delConns.empty());	//	Nothing removed

	// 		//	Change B's output xpt (same input xpt)
	// 		routingB[NTV2_XptCSC1VidInput] = NTV2_XptFrameBuffer1RGB;
	// 		SHOULD_BE_FALSE(CNTV2SignalRouter::CompareConnections(routingA, routingB, newConns, delConns));
	// 		SHOULD_BE_EQUAL(routingB, newConns);	//	B's connection added
	// 		SHOULD_BE_EQUAL(routingA, delConns);	//	A's connection removed

	// 		//	B empty, A still has 1 entry
	// 		routingB.clear();
	// 		SHOULD_BE_FALSE(CNTV2SignalRouter::CompareConnections(routingA, routingB, newConns, delConns));
	// 		SHOULD_BE_TRUE(newConns.empty());		//	Nothing added
	// 		SHOULD_BE_EQUAL(routingA, delConns);	//	A's connection removed

	// 		//	A empty, B has 1 entry
	// 		routingA.clear();
	// 		routingB[NTV2_XptCSC1VidInput] = NTV2_XptFrameBuffer1RGB;
	// 		SHOULD_BE_FALSE(CNTV2SignalRouter::CompareConnections(routingA, routingB, newConns, delConns));
	// 		SHOULD_BE_EQUAL(routingB, newConns);	//	B's connection added
	// 		SHOULD_BE_TRUE(delConns.empty());		//	Nothing removed

	// 		//	B == A, except one route removed
	// 		routingA[NTV2_XptCSC1VidInput]		= NTV2_XptLUT1Out;
	// 		routingA[NTV2_XptCSC2VidInput]		= NTV2_XptFrameBuffer2YUV;
	// 		routingA[NTV2_XptLUT1Input]			= NTV2_XptFrameBuffer1RGB;
	// 		routingA[NTV2_XptLUT2Input]			= NTV2_XptCSC2VidRGB;
	// 		routingA[NTV2_XptSDIOut3Input]		= NTV2_XptDuallinkOut3;
	// 		routingA[NTV2_XptSDIOut3InputDS2]	= NTV2_XptDuallinkOut3DS2;
	// 		routingA[NTV2_XptSDIOut4Input]		= NTV2_XptDuallinkOut4;
	// 		routingA[NTV2_XptSDIOut4InputDS2]	= NTV2_XptDuallinkOut4DS2;
	// 		routingA[NTV2_XptSDIOut5Input]		= NTV2_XptDuallinkOut5;
	// 		routingA[NTV2_XptSDIOut5InputDS2]	= NTV2_XptDuallinkOut5DS2;
	// 		routingA[NTV2_XptDualLinkOut3Input]	= NTV2_XptLUT1Out;
	// 		routingA[NTV2_XptDualLinkOut4Input]	= NTV2_XptLUT1Out;
	// 		routingA[NTV2_XptDualLinkOut5Input]	= NTV2_XptLUT1Out;
	// 		routingB = routingA;
	// 		NTV2XptConnection removedConnection(NTV2_XptSDIOut4InputDS2, NTV2_XptDuallinkOut4DS2);
	// 		routingB.erase(routingB.find(removedConnection.first));
	// 		cerr << "A:" << endl << routingA << endl << "B:" << endl << routingB << endl;
	// 		SHOULD_BE_FALSE(CNTV2SignalRouter::CompareConnections(routingA, routingB, newConns, delConns));
	// 		SHOULD_BE_TRUE(newConns.empty());
	// 		SHOULD_BE_EQUAL(delConns.size(), 1);
	// 		SHOULD_BE_EQUAL(delConns.begin()->first, removedConnection.first);
	// 		SHOULD_BE_EQUAL(delConns.begin()->second, removedConnection.second);

	// 		//	B == A, except one route removed, and one route added
	// 		NTV2XptConnection addedConnection(NTV2_XptFrameBuffer5Input, NTV2_XptSDIIn5);
	// 		routingB[addedConnection.first] = addedConnection.second;
	// 		SHOULD_BE_FALSE(CNTV2SignalRouter::CompareConnections(routingA, routingB, newConns, delConns));
	// 		SHOULD_BE_EQUAL(newConns.size(), 1);
	// 		SHOULD_BE_EQUAL(delConns.size(), 1);
	// 		SHOULD_BE_EQUAL(newConns.begin()->first, addedConnection.first);
	// 		SHOULD_BE_EQUAL(newConns.begin()->second, addedConnection.second);
	// 		SHOULD_BE_EQUAL(delConns.begin()->first, removedConnection.first);
	// 		SHOULD_BE_EQUAL(delConns.begin()->second, removedConnection.second);
	// 		cerr << "New:" << endl << newConns << endl << "Removed:" << endl << delConns << endl;
	// 	}
	// 	LOGNOTE("Passed");
	// 	return true;
	// }

} //bft

void signalrouter_marker() {}
TEST_SUITE("signal router" * doctest::description("CNTV2SignalRouter & RoutingExpert tests")) {
	TEST_CASE("RoutingExpert Instance") {
		RoutingExpertPtr pExpert1 = RoutingExpert::GetInstance();
		CHECK(pExpert1->NumInstances() == 1);
	}
	TEST_CASE("CNTV2SignalRouter String Conversions") {
		std::string inpXptStr = CNTV2SignalRouter::
						NTV2InputCrosspointIDToString(NTV2_FIRST_INPUT_CROSSPOINT);
		CHECK(inpXptStr == "FB1");

		std::string outXptStr = CNTV2SignalRouter::
						NTV2OutputCrosspointIDToString(NTV2_FIRST_OUTPUT_CROSSPOINT);
		CHECK(outXptStr == "Black");

		NTV2InputXptID inpXptID = CNTV2SignalRouter::
						StringToNTV2InputCrosspointID("DLIn1");
		CHECK(inpXptID == NTV2_XptDualLinkIn1Input);

		NTV2OutputXptID outXptID = CNTV2SignalRouter::
						StringToNTV2OutputCrosspointID("FB1RGBDS2");
		CHECK(outXptID == NTV2_XptFrameBuffer1_DS2RGB);
	}
	TEST_CASE("CNTV2SignalRouter WidgetID helpers") {
		NTV2WidgetIDSet widgetIDs;

		CNTV2SignalRouter::GetWidgetIDs(DEVICE_ID_KONA5, widgetIDs);
		CHECK(widgetIDs.size() == 39);

		CNTV2SignalRouter::GetWidgetsForInput(NTV2_XptMultiLinkOut1Input, widgetIDs);
		CHECK(widgetIDs.size() == 1);

		NTV2WidgetIDSet::const_iterator iter = widgetIDs.begin();
		CHECK(*iter == NTV2_WgtMultiLinkOut1);

		NTV2WidgetID widgetID = NTV2_WIDGET_INVALID;
		CNTV2SignalRouter::GetWidgetForInput(NTV2_XptDualLinkIn1Input,
						widgetID, DEVICE_ID_KONA4);
		CHECK(widgetID == NTV2_WgtDualLinkV2In1);

		CHECK(CNTV2SignalRouter::WidgetIDToChannel(NTV2_Wgt3GSDIIn3) == NTV2_CHANNEL3);
		CHECK(CNTV2SignalRouter::WidgetIDFromTypeAndChannel(NTV2WidgetType_CSC, NTV2_CHANNEL4) == NTV2_WgtCSC4);

		CHECK(CNTV2SignalRouter::WidgetIDToType(NTV2_WgtFrameBuffer1) == NTV2WidgetType_FrameStore);

		CHECK(CNTV2SignalRouter::IsSDIWidgetType(NTV2WidgetType_4KDownConverter) == false);
		CHECK(CNTV2SignalRouter::IsSDIWidgetType(NTV2WidgetType_SDIOut12G) == true);
		CHECK(CNTV2SignalRouter::IsSDIInputWidgetType(NTV2WidgetType_SDIIn3G) == true);
		CHECK(CNTV2SignalRouter::IsSDIInputWidgetType(NTV2WidgetType_SDIOut3G) == false);
		CHECK(CNTV2SignalRouter::IsSDIOutputWidgetType(NTV2WidgetType_SDIOut3G) == true);
		CHECK(CNTV2SignalRouter::IsSDIOutputWidgetType(NTV2WidgetType_SDIIn3G) == false);
		CHECK(CNTV2SignalRouter::Is3GSDIWidgetType(NTV2WidgetType_SDIOut3G) == true);
		CHECK(CNTV2SignalRouter::Is3GSDIWidgetType(NTV2WidgetType_SDIOut12G) == false);
		CHECK(CNTV2SignalRouter::Is12GSDIWidgetType(NTV2WidgetType_SDIOut12G) == true);
		CHECK(CNTV2SignalRouter::Is12GSDIWidgetType(NTV2WidgetType_SDIOut3G) == false);
		CHECK(CNTV2SignalRouter::IsDualLinkWidgetType(NTV2WidgetType_DualLinkV1Out) == true);
		CHECK(CNTV2SignalRouter::IsDualLinkWidgetType(NTV2WidgetType_LUT) == false);
		CHECK(CNTV2SignalRouter::IsDualLinkInWidgetType(NTV2WidgetType_DualLinkV2In) == true);
		CHECK(CNTV2SignalRouter::IsDualLinkInWidgetType(NTV2WidgetType_DualLinkV1Out) == false);
		CHECK(CNTV2SignalRouter::IsDualLinkOutWidgetType(NTV2WidgetType_DualLinkV1Out) == true);
		CHECK(CNTV2SignalRouter::IsDualLinkOutWidgetType(NTV2WidgetType_DualLinkV2In) == false);
		CHECK(CNTV2SignalRouter::IsHDMIWidgetType(NTV2WidgetType_HDMIOutV5) == true);
		CHECK(CNTV2SignalRouter::IsHDMIWidgetType(NTV2WidgetType_SDIIn) == false);
		CHECK(CNTV2SignalRouter::IsHDMIInWidgetType(NTV2WidgetType_HDMIInV3) == true);
		CHECK(CNTV2SignalRouter::IsHDMIInWidgetType(NTV2WidgetType_HDMIOutV5) == false);
		CHECK(CNTV2SignalRouter::IsHDMIOutWidgetType(NTV2WidgetType_HDMIOutV5) == true);
		CHECK(CNTV2SignalRouter::IsHDMIOutWidgetType(NTV2WidgetType_HDMIInV2) == false);
	}
}


void testpatterngenmarker() {}
TEST_SUITE("TestPatternGen" * doctest::description("NTV2TestPatternGen tests"))
{
	TEST_CASE("Permutations")
	{
		static const NTV2PixelFormats pixFmtsToTest = {	NTV2_FBF_10BIT_YCBCR, NTV2_FBF_8BIT_YCBCR, NTV2_FBF_ARGB,
														NTV2_FBF_RGBA, NTV2_FBF_10BIT_RGB, NTV2_FBF_8BIT_YCBCR_YUY2,
														NTV2_FBF_ABGR, NTV2_FBF_10BIT_DPX,
#if defined(_DEBUG)
														NTV2_FBF_10BIT_YCBCR_DPX,	//	crashes!
#endif	//	_DEBUG
														NTV2_FBF_10BIT_DPX_LE, NTV2_FBF_24BIT_RGB, NTV2_FBF_24BIT_BGR,
														NTV2_FBF_48BIT_RGB};
		static const NTV2StandardSet standardsToTest = {/*NTV2_STANDARD_1080,*/ NTV2_STANDARD_720, NTV2_STANDARD_525,
														NTV2_STANDARD_625, NTV2_STANDARD_1080p, /*NTV2_STANDARD_2K,*/
														NTV2_STANDARD_2Kx1080p, /*NTV2_STANDARD_2Kx1080i,*/
														NTV2_STANDARD_3840x2160p, NTV2_STANDARD_4096x2160p,
														NTV2_STANDARD_7680, NTV2_STANDARD_8192};
		static const NTV2TestPatternNames tpNames(NTV2TestPatternGen::getTestPatternNames());

		NTV2Buffer fb;
		//	For each possible raster dimension (via video standard)...
		for (NTV2StandardSetConstIter stIt(standardsToTest.begin());  stIt != standardsToTest.end();  ++stIt)
		{	const NTV2Standard st(*stIt);
			//	For this Standard, get a list of permissible VANC modes (off, tall, taller?)
			NTV2GeometrySet fgs (::GetRelatedGeometries(::GetGeometryFromStandard(st)));
			NTV2VANCModes vms;
			if (fgs.empty())
				vms.push_back(NTV2_VANCMODE_OFF);
			else for (NTV2GeometrySetConstIter fgIt(fgs.begin());  fgIt != fgs.end();  ++fgIt)
				vms.push_back(::GetVANCModeForGeometry(*fgIt));

			//	For each possible pixel format...
			for (NTV2PixelFormatsConstIter pfIt(pixFmtsToTest.begin());  pfIt != pixFmtsToTest.end();  ++pfIt)
			{	const NTV2PixelFormat pf(*pfIt);

				//	For each VANC mode...
				for (size_t vmNdx(0);  vmNdx < vms.size();  vmNdx++)
				{	const NTV2VANCMode vm(vms.at(vmNdx));
					NTV2FormatDesc fd (st, pf, vm);
					CHECK(fd.IsValid());
					//	Allocate a video buffer for this raster size...
					CHECK(fb.Allocate(fd.GetTotalBytes()));

					//	For each test pattern...
					for (size_t ndx(0);  ndx < tpNames.size();  ndx++)
					{	const string tpName (tpNames.at(ndx));
						const NTV2TestPatternSelect tpID(NTV2TestPatternGen::findTestPatternByName(tpName));
						if (NTV2_IS_12B_PATTERN(tpID)  &&  !NTV2_IS_FBF_12BIT_RGB(pf))
							continue;	//	skip -- this test pattern won't work with this pixel format
						if (!NTV2TestPatternGen::canDrawTestPattern (tpID, fd))
							continue;	//	skip -- it admits it can't do this

						//	Draw the test pattern into the buffer...
						cout << fd << ": '" << tpName << "'" << endl;
						NTV2TestPatternGen gen;
						CHECK(gen.DrawTestPattern (tpName, fd, fb));
					}	//	for each test pattern
				}	//	for each VANC mode
			}	//	for each pixel format
		}	//	for each video standard
	}	//	TEST_CASE("Permutations")
}	//	TEST_SUITE("TestPatternGen")
