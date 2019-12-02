/**
    @file		main.cpp
	@brief		Unittests for the AJA NTV2 Library (using doctest).
	@copyright	Copyright (c) 2019 AJA Video Systems, Inc. All rights reserved.
**/
// for doctest usage see: https://github.com/onqtam/doctest/blob/1.1.4/doc/markdown/tutorial.md

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
// need to define this so will work with compilers that don't support thread_local
// ie xcode 6, 7
#define DOCTEST_THREAD_LOCAL
#include "doctest.h"

#include <vector>

#include "ntv2card.h"
#include "ntv2debug.h"
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
		CHECK(std::string(NTV2FrameRateString(NTV2_FRAMERATE_1798)) == "NTV2_FRAMERATE_1798");
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
			CHECK(bit_depth == VPIDBitDepth_8);
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
			CHECK(bit_depth == VPIDBitDepth_Reserved3);
		}
	}

	TEST_CASE("CNTV2VPID::SetBitDepth")
	{
		{ // 8-bit
			const ULWord data = 0x8a052201;
			const VPIDBitDepth bd = VPIDBitDepth_8;
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
			const VPIDBitDepth bd = VPIDBitDepth_Reserved3;
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