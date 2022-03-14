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
#include <doctest/doctest.h>

#include <vector>

#include "ajabase/system/thread.h"

#include "ntv2bitfile.h"
#include "ntv2card.h"
#include "ntv2debug.h"
#include "ntv2endian.h"
#include "ntv2signalrouter.h"
#include "ntv2routingexpert.h"
#include "ntv2utils.h"
#include "ntv2vpid.h"

#if 0
template
void filename_marker() {} //this is used to easily just around in a GUI with a symbols list
TEST_SUITE("filename" * doctest::description("functions in streams/common/filename.h")) {

	TEST_CASE("constructor")
	{
	}

} //filename
#endif

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
							NTV2_FORMAT_1080psf_2K_2500}, NTV2_STANDARD_2Kx1080p);

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
		CHECK(std_set.count(NTV2_STANDARD_3840x2160p) > 0);
		CHECK(std_set.count(NTV2_STANDARD_4096x2160p) > 0);
		CHECK(std_set.count(NTV2_STANDARD_3840HFR) > 0);
		CHECK(std_set.count(NTV2_STANDARD_4096HFR) > 0);
		// not supported
		CHECK(std_set.count(NTV2_STANDARD_2K) == 0);
		CHECK(std_set.count(NTV2_STANDARD_2Kx1080i) == 0);
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

	TEST_CASE("NTV2Bitfile")
	{
		static unsigned char sTTapPro[] = { //	.............a.Et_tap_pro;COMPRESS=TRUE;UserID=0XFFFFFFFF;TANDEM=TRUE;Version=2019.1.b..xcku035-fbva676-1LV-i.c..2020/11/04.d..14:58:54.e..'......................................................................".D..........Uf ... ...0. .....0.......0......
			0x00, 0x09, 0x0f, 0xf0, 0x0f, 0xf0, 0x0f, 0xf0, 0x0f, 0xf0, 0x00, 0x00, 0x01, 0x61, 0x00, 0x45, 0x74, 0x5f, 0x74, 0x61, 0x70, 0x5f, 0x70, 0x72, 0x6f, 0x3b, 0x43, 0x4f, 0x4d, 0x50, 0x52, 0x45, 0x53, 0x53, 0x3d, 0x54, 0x52, 0x55, 0x45, 0x3b, 0x55, 0x73, 0x65, 0x72, 0x49, 0x44, 0x3d, 0x30, 0x58, 0x46, 0x46, 0x46, 0x46, 0x46, 0x46, 0x46, 0x46, 0x3b, 0x54, 0x41, 0x4e, 0x44, 0x45, 0x4d,
			0x3d, 0x54, 0x52, 0x55, 0x45, 0x3b, 0x56, 0x65, 0x72, 0x73, 0x69, 0x6f, 0x6e, 0x3d, 0x32, 0x30, 0x31, 0x39, 0x2e, 0x31, 0x00, 0x62, 0x00, 0x16, 0x78, 0x63, 0x6b, 0x75, 0x30, 0x33, 0x35, 0x2d, 0x66, 0x62, 0x76, 0x61, 0x36, 0x37, 0x36, 0x2d, 0x31, 0x4c, 0x56, 0x2d, 0x69, 0x00, 0x63, 0x00, 0x0b, 0x32, 0x30, 0x32, 0x30, 0x2f, 0x31, 0x31, 0x2f, 0x30, 0x34, 0x00, 0x64, 0x00, 0x09, 0x31,
			0x34, 0x3a, 0x35, 0x38, 0x3a, 0x35, 0x34, 0x00, 0x65, 0x00, 0xd2, 0x27, 0xd4, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
			0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0xbb, 0x11, 0x22, 0x00, 0x44, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xaa, 0x99, 0x55, 0x66, 0x20, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x30, 0x02, 0x20, 0x01, 0x00, 0x00, 0x00, 0x00, 0x30, 0x02, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x80, 0x01, 0x00, 0x00, 0x00	};

		const NTV2_POINTER TTapPro(sTTapPro, sizeof(sTTapPro));
		NTV2_POINTER sub(TTapPro.GetHostAddress(0), TTapPro.GetByteCount());
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

			NTV2_POINTER aSectionBad(TTapPro);
			NTV2_POINTER bSectionBad(TTapPro);
			NTV2_POINTER cSectionBad(TTapPro);
			NTV2_POINTER dSectionBad(TTapPro);
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
