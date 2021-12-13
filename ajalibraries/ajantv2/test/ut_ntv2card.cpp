#define DOCTEST_CONFIG_IMPLEMENT
// need to define this so will work with compilers that don't support thread_local
// ie xcode 6, 7
#define DOCTEST_THREAD_LOCAL

#include <argparse/argparse.h>
#include <doctest/doctest.h>

#include "auto_router.h"
#include "test_support.h"
#include "ajabase/system/debug.h"
#include "ajantv2/includes/ntv2card.h"
#include "ajantv2/includes/ntv2devicefeatures.h"
#include "ajantv2/includes/ntv2devicescanner.h"
#include <malloc.h>

#define	LOGERR(__x__)	AJA_sREPORT(AJA_DebugUnit_UnitTest, AJA_DebugSeverity_Error,	__FUNCTION__ << ":  " << __x__)
#define	LOGWARN(__x__)	AJA_sREPORT(AJA_DebugUnit_UnitTest, AJA_DebugSeverity_Warning,	__FUNCTION__ << ":  " << __x__)
#define	LOGNOTE(__x__)	AJA_sREPORT(AJA_DebugUnit_UnitTest, AJA_DebugSeverity_Notice,	__FUNCTION__ << ":  " << __x__)
#define	LOGINFO(__x__)	AJA_sREPORT(AJA_DebugUnit_UnitTest, AJA_DebugSeverity_Info,		__FUNCTION__ << ":  " << __x__)
#define	LOGDBG(__x__)	AJA_sREPORT(AJA_DebugUnit_UnitTest, AJA_DebugSeverity_Debug,	__FUNCTION__ << ":  " << __x__)

// The default argparse behavior is to exit after displaying usage text.
// We want to display usage text for both our args, and those of doctest.
static int argparse_help(struct argparse *self, const struct argparse_option *option)
{
    (void)option;
    argparse_usage(self);
    return 0;
}

#define OPT_ARGPARSE_HELP()       OPT_BOOLEAN('h', "help", NULL,        \
                                     "show this help message and exit", \
                                     argparse_help, 0, OPT_NONEG)

static constexpr size_t kAudioSize1MiB = 0xff000;
static constexpr size_t kAudioSize2MiB = kAudioSize1MiB * 2;
static constexpr size_t kAudioSize4MiB = kAudioSize1MiB * 4;
static constexpr size_t kFrameSize8MiB = 0x800000;
static constexpr size_t kFrameSize16MiB = kFrameSize8MiB * 2;
static constexpr size_t kFrameSize32MiB = kFrameSize8MiB * 4;
static constexpr uint8_t kSDILegalMin = 0x04;
static constexpr uint8_t kSDILegalMax8Bit = 0xfb;
static constexpr uint16_t kSDILegalMax10Bit = 0x3fb;
static constexpr uint32_t kSDILegalMax12Bit = 0xffc;

struct TestOptions {
    uint32_t card_a_index;
    uint32_t card_b_index;
};
static TestOptions* gOpts = NULL;
static CNTV2Card* gCard = NULL;
static NTV2EveryFrameTaskMode gTaskMode = NTV2_TASK_MODE_INVALID;

void ntv2card_general_marker() {}
TEST_SUITE("card_general" * doctest::description("CNTV2Card general tests")) {
    TEST_CASE("IsOpen") {
        CHECK(gCard->IsOpen() == true);
    }
    TEST_CASE("GetDeviceID") {
        CHECK(gCard->GetDeviceID() != DEVICE_ID_NOTFOUND);
    }
    TEST_CASE("SetEveryFrameServices") {
        gCard->GetEveryFrameServices(gTaskMode); // cached
        NTV2EveryFrameTaskMode want_task_mode = NTV2_OEM_TASKS;
        CHECK_EQ(gCard->SetEveryFrameServices(want_task_mode), true);
        NTV2EveryFrameTaskMode task_mode = NTV2_TASK_MODE_INVALID;
        gCard->GetEveryFrameServices(task_mode);
        CHECK_EQ(task_mode, want_task_mode);
    }
}

void ntv2card_dma_marker() {}
TEST_SUITE("card_dma" * doctest::description("CNTV2Card DMA tests")) {
    TEST_CASE("dma_write_read_audio") {
        NTV2Channel channel = NTV2_CHANNEL1;
        NTV2AudioSystem audio_sys = NTV2ChannelToAudioSystem(channel);
        uint8_t* buffer = (uint8_t*)malloc(kAudioSize4MiB);
        uint8_t* readback = (uint8_t*)malloc(kAudioSize4MiB);
        qa::RandomPattern<uint32_t>(0, 0, UINT32_MAX, buffer, kAudioSize4MiB, qa::ByteOrder::LittleEndian);
        qa::RandomPattern<uint32_t>(0, 0, UINT32_MAX, readback, kAudioSize4MiB, qa::ByteOrder::LittleEndian);
        CHECK(gCard->SetMode(channel, NTV2_MODE_DISPLAY) == true);
        CHECK(gCard->DMAWriteAudio(audio_sys, reinterpret_cast<const ULWord*>(buffer), 0, kAudioSize4MiB) == true);
        CHECK(gCard->DMAReadAudio(audio_sys, reinterpret_cast<ULWord*>(readback), 0, kAudioSize4MiB) == true);
        free((void*)buffer);
        free((void*)readback);
        buffer = NULL;
        readback = NULL;
    }
    TEST_CASE("dma_write_read_one_frame" * doctest::description("DMAWriteFrame/DMAReadFrame frame 0 random data test")) {
        NTV2Channel channel = NTV2_CHANNEL1;
        ULWord frame_num = 0;
        uint8_t* buffer = (uint8_t*)malloc(kFrameSize8MiB);
        uint8_t* readback = (uint8_t*)malloc(kFrameSize8MiB);
        qa::RandomPattern<uint32_t>(0, 0, UINT32_MAX, buffer, kFrameSize8MiB, qa::ByteOrder::LittleEndian);
        qa::RandomPattern<uint32_t>(0, 0, UINT32_MAX, readback, kFrameSize8MiB, qa::ByteOrder::LittleEndian);
        CHECK(gCard->SetMode(channel, NTV2_MODE_DISPLAY) == true);
        CHECK(gCard->SetOutputFrame(channel, frame_num) == true);
        CHECK(gCard->DMAWriteFrame(frame_num, reinterpret_cast<const ULWord*>(buffer), (ULWord)kFrameSize8MiB) == true);
        CHECK(gCard->DMAReadFrame(frame_num, reinterpret_cast<ULWord*>(&readback[0]), (ULWord)kFrameSize8MiB) == true);
        CHECK(memcmp(readback, buffer, kFrameSize8MiB) == 0);
        free((void*)buffer);
        free((void*)readback);
        buffer = NULL;
        readback = NULL;
    }
    TEST_CASE("dma_write_read_all_frames" * doctest::description("DMAWriteFrame/DMAReadFrame all frames random data test")) {
        NTV2Channel channel = NTV2_CHANNEL1;
        const ULWord num_frame_buffers = gCard->GetNumFrameBuffers();
        uint8_t* buffer = (uint8_t*)malloc(kFrameSize8MiB);
        uint8_t* readback = (uint8_t*)malloc(kFrameSize8MiB);
        size_t num_writes = 0;
        size_t num_reads = 0;
        size_t num_cmps = 0;
        for (ULWord i = 0; i < num_frame_buffers; i++) {
            gCard->SetMode(channel, NTV2_MODE_DISPLAY);
            gCard->SetOutputFrame(channel, i);
            qa::RandomPattern<uint32_t>(0, 0, UINT32_MAX, buffer, kFrameSize8MiB, qa::ByteOrder::LittleEndian);
            qa::RandomPattern<uint32_t>(0, 0, UINT32_MAX, readback, kFrameSize8MiB, qa::ByteOrder::LittleEndian);
            if (gCard->DMAWriteFrame(i, reinterpret_cast<const ULWord*>(buffer), (ULWord)kFrameSize8MiB))
                num_writes++;
            if (gCard->DMAReadFrame(i, reinterpret_cast<ULWord*>(readback), (ULWord)kFrameSize8MiB))
                num_reads++;
            if (memcmp(readback, buffer, kFrameSize8MiB) == 0)
                num_cmps++;
        }
        free((void*)buffer);
        free((void*)readback);
        buffer = NULL;
        readback = NULL;
        CHECK(num_writes == num_frame_buffers);
        CHECK(num_reads == num_frame_buffers);
        CHECK(num_cmps == num_frame_buffers);
    }
}

void ntv2card_framestore_marker() {}
TEST_SUITE("framestore_formats" * doctest::description("Framestore widget format tests")) {
    TEST_CASE("Framestore 1080p30 YUV-8") {
        auto vf = NTV2_FORMAT_1080p_3000;
        auto pf = NTV2_FBF_8BIT_YCBCR;
        CHECK(gCard->SetMode(NTV2_CHANNEL1, NTV2_MODE_DISPLAY) == true);
        CHECK(gCard->SetVideoFormat(vf, false, false, NTV2_CHANNEL1) == true);
        CHECK(gCard->SetFrameBufferFormat(NTV2_CHANNEL1, pf) == true);
        NTV2FormatDesc fd(vf, pf);
        auto raster_size = fd.GetTotalBytes();
        uint8_t* buffer = (uint8_t*)malloc(kFrameSize8MiB);
        uint8_t* readback = (uint8_t*)malloc(kFrameSize8MiB);
        qa::Generate8Bit(fd.GetRasterHeight(), fd.GetBytesPerRow(), buffer, kSDILegalMin, kSDILegalMax8Bit);
        qa::RandomPattern<uint32_t>(0, 0, UINT32_MAX, readback, fd.GetTotalBytes(), qa::ByteOrder::LittleEndian);
        CHECK(gCard->DMAWriteFrame(0, reinterpret_cast<const ULWord*>(buffer), raster_size) == true);
        CHECK(gCard->DMAReadFrame(0, reinterpret_cast<ULWord*>(readback), raster_size) == true);
        CHECK(memcmp(buffer, readback, raster_size) == 0);
        free((void*)buffer);
        free((void*)readback);
        buffer = NULL;
        readback = NULL;
    }
    TEST_CASE("Framestore UHDp30 YUV-8") {
        auto vf = NTV2_FORMAT_4x1920x1080p_3000;
        auto pf = NTV2_FBF_8BIT_YCBCR;
        CHECK(gCard->SetMode(NTV2_CHANNEL1, NTV2_MODE_DISPLAY) == true);
        CHECK(gCard->SetVideoFormat(vf, false, false, NTV2_CHANNEL1) == true);
        CHECK(gCard->SetFrameBufferFormat(NTV2_CHANNEL1, pf) == true);
        NTV2FormatDesc fd(vf, pf);
        auto raster_size = fd.GetTotalBytes();
        uint8_t* buffer = (uint8_t*)malloc(kFrameSize32MiB);
        uint8_t* readback = (uint8_t*)malloc(kFrameSize32MiB);
        qa::Generate8Bit(fd.GetRasterHeight(), fd.GetBytesPerRow(), buffer, kSDILegalMin, kSDILegalMax8Bit);
        qa::RandomPattern<uint32_t>(0, 0, UINT32_MAX, readback, fd.GetTotalBytes(), qa::ByteOrder::LittleEndian);
        CHECK(gCard->DMAWriteFrame(0, reinterpret_cast<const ULWord*>(buffer), raster_size) == true);
        CHECK(gCard->DMAReadFrame(0, reinterpret_cast<ULWord*>(readback), raster_size) == true);
        CHECK(memcmp(buffer, readback, raster_size) == 0);
        free((void*)buffer);
        free((void*)readback);
        buffer = NULL;
        readback = NULL;
    }
}

void ntv2card_sdi_loopback_marker() {}
TEST_SUITE("sdi_loopback" * doctest::description("SDI loopback tests")) {
    TEST_CASE("NTSC") {
        CNTV2DeviceScanner scanner;
        const size_t num_devices = scanner.GetNumDevices();
        if (num_devices < 2)
            LOGERR("SDI Loopback tests require two boards connected to the host system!");
        CHECK_GE(num_devices, 2);
        auto vf = NTV2_FORMAT_525_5994;
        auto pf = NTV2_FBF_8BIT_YCBCR;
        uint32_t chan_index = 0;
        uint32_t fs_index = 0;
        uint32_t frame_num = 0;
        auto src_router = AutoRouter(gOpts->card_a_index, chan_index, fs_index,
            NTV2_MODE_DISPLAY, vf, pf,
            NTV2_REFERENCE_FREERUN, NTV2_VANCMODE_OFF, VPIDStandard_483_576, kConnectionKindSDI);
        src_router.ApplyRouting(true, true);
        auto dst_router = AutoRouter(gOpts->card_b_index, chan_index, fs_index,
            NTV2_MODE_CAPTURE, vf, pf,
            NTV2_REFERENCE_FREERUN, NTV2_VANCMODE_OFF, VPIDStandard_483_576, kConnectionKindSDI);
        dst_router.ApplyRouting(true, true);
        auto src_card = src_router.GetCard();
        auto dst_card = dst_router.GetCard();

        NTV2Channel channel = GetNTV2ChannelForIndex(chan_index);
        CHECK(src_card->SetMode(channel, NTV2_MODE_DISPLAY) == true);
        CHECK(src_card->SetOutputFrame(channel, frame_num) == true);
        CHECK(src_card->DMAWriteFrame(frame_num, reinterpret_cast<const ULWord*>(buffer), (ULWord)kFrameSize8MiB) == true);
        CHECK(src_card->DMAReadFrame(frame_num, reinterpret_cast<ULWord*>(&readback[0]), (ULWord)kFrameSize8MiB) == true);
    }
    TEST_CASE("PAL") {
        CNTV2DeviceScanner scanner;
        const size_t num_devices = scanner.GetNumDevices();
        if (num_devices < 2)
            LOGERR("SDI Loopback tests require two boards connected to the host system!");
        CHECK_GE(num_devices, 2);
        auto vf = NTV2_FORMAT_625_5000;
        auto pf = NTV2_FBF_8BIT_YCBCR;
        uint32_t chan_index = 0;
        uint32_t fs_index = 0;
        auto src_router = AutoRouter(gOpts->card_a_index, chan_index, fs_index,
            NTV2_MODE_DISPLAY, vf, pf,
            NTV2_REFERENCE_FREERUN, NTV2_VANCMODE_OFF, VPIDStandard_483_576, kConnectionKindSDI);
        src_router.ApplyRouting(true, true);
        auto dst_router = AutoRouter(gOpts->card_b_index, chan_index, fs_index,
            NTV2_MODE_CAPTURE, vf, pf,
            NTV2_REFERENCE_FREERUN, NTV2_VANCMODE_OFF, VPIDStandard_483_576, kConnectionKindSDI);
        dst_router.ApplyRouting(true, true);
    }
}

int main(int argc, const char** argv) {
    AJADebug::Open();

    LOGINFO("Starting CNTV2Card unit tests...");

    gOpts = new TestOptions();
    gOpts->card_a_index = 0;
    gOpts->card_b_index = 1;

    // copy argv
    char** new_argv = (char**)malloc((argc+1) * sizeof *new_argv);
    for(int i = 0; i < argc; ++i)
    {
        size_t length = strlen(argv[i])+1;
        new_argv[i] = (char*)malloc(length);
        memcpy(new_argv[i], argv[i], length);
    }
    new_argv[argc] = NULL;

    // initialize doctest and handle args
    doctest::Context ctx(argc, argv);

    // handle global test args
    static const char *const usage[] = {
        "ut_ntv2card [options] [[--] args]",
        "ut_ntv2card [options]",
        NULL,
    };
    struct argparse_option options[] = {
        OPT_ARGPARSE_HELP(),
        OPT_GROUP("ut_ntv2card options"),
        OPT_INTEGER('\0', "card", &gOpts->card_a_index, "card_a"),
        OPT_END(),
    };
    struct argparse argparse;
    argparse_init(&argparse, options, usage, ARGPARSE_IGNORE_UNKNOWN_ARGS);
    argparse_describe(&argparse, "\nntv2 card unit tests",
        "\nPerform CNTV2Card tests against physical hardware.");
    argparse_parse(&argparse, argc, (const char**)argv);

    gCard = new CNTV2Card(gOpts->card_a_index, "");
    int res = ctx.run();
    if (gCard) {
        gCard->SetEveryFrameServices(gTaskMode);
        delete gCard;
        gCard = NULL;
    }

    // free argv copy
    for(int i = 0; i < argc; ++i)
        free(new_argv[i]);
    free(new_argv);
    delete gOpts;

    LOGINFO("Completed CNTV2Card Unit Tests!");
    AJADebug::Close();

    return ctx.shouldExit() ? res : EXIT_SUCCESS;
}
