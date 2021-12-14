#define DOCTEST_CONFIG_IMPLEMENT
// need to define this so will work with compilers that don't support thread_local
// ie xcode 6, 7
#define DOCTEST_THREAD_LOCAL

#include <argparse/argparse.h>
#include <doctest/doctest.h>

#include "auto_router.h"
// #include "test_support.h"
#include "ajabase/system/systemtime.h"
#include "ajabase/system/debug.h"
#include "ajantv2/includes/ntv2card.h"
#include "ajantv2/includes/ntv2devicefeatures.h"
#include "ajantv2/includes/ntv2devicescanner.h"
#include "ntv2qa/binary_pattern_gen.h"
#include "ntv2qa/binary_pattern_cmp.h"

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
    uint32_t out_channel;
    uint32_t inp_channel;
    uint32_t out_framestore;
    uint32_t inp_framestore;
    uint32_t out_frame;
    uint32_t inp_frame;
};
static TestOptions* gOpts = NULL;
static CNTV2Card* gCard = NULL;
static NTV2EveryFrameTaskMode gTaskMode = NTV2_TASK_MODE_INVALID;

static void SetCardBaseline(CNTV2Card* card)
{
    if (card) {
        gCard->ClearRouting();
        for (uint32_t i = 0; i < NTV2DeviceGetNumAudioSystems(gCard->GetDeviceID()); i++) {
            NTV2AudioSystem as = NTV2ChannelToAudioSystem(GetNTV2ChannelForIndex(i));
            gCard->StopAudioInput(as);
            gCard->StopAudioOutput(as);
        }
    }
}

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
        gCard->StopAudioInput(audio_sys);
        gCard->StopAudioOutput(audio_sys);
        std::vector<uint8_t> buffer(kAudioSize4MiB);
        std::vector<uint8_t> readback(kAudioSize4MiB);
        uint32_t seed = 1;
        qa::RandomPattern<uint32_t>(buffer, kAudioSize4MiB, seed, 0, UINT32_MAX, qa::ByteOrder::LittleEndian);
        qa::RandomPattern<uint32_t>(readback, kAudioSize4MiB, seed, 0, UINT32_MAX, qa::ByteOrder::LittleEndian);
        CHECK_EQ(gCard->SetMode(channel, NTV2_MODE_DISPLAY), true);
        CHECK_EQ(gCard->DMAWriteAudio(audio_sys, reinterpret_cast<const ULWord*>(buffer.data()), 0, kAudioSize4MiB), true);
        AJATime::Sleep(150);
        CHECK_EQ(gCard->DMAReadAudio(audio_sys, reinterpret_cast<ULWord*>(&readback[0]), 0, kAudioSize4MiB), true);
        CHECK_EQ(memcmp(readback.data(), buffer.data(), kAudioSize4MiB), 0);
    }
    TEST_CASE("dma_write_read_one_frame" * doctest::description("DMAWriteFrame/DMAReadFrame frame 0 random data test")) {
        NTV2Channel channel = NTV2_CHANNEL1;
        ULWord frame_num = 0;
        std::vector<uint8_t> buffer(kFrameSize8MiB);
        std::vector<uint8_t> readback(kFrameSize8MiB);
        uint32_t seed = 1;
        qa::RandomPattern<uint32_t>(buffer, kFrameSize8MiB, seed, 0, UINT32_MAX, qa::ByteOrder::LittleEndian);
        qa::RandomPattern<uint32_t>(readback, kFrameSize8MiB, seed, 0, UINT32_MAX, qa::ByteOrder::LittleEndian);
        CHECK_EQ(gCard->SetMode(channel, NTV2_MODE_DISPLAY), true);
        CHECK_EQ(gCard->SetOutputFrame(channel, frame_num), true);
        CHECK_EQ(gCard->DMAWriteFrame(frame_num, reinterpret_cast<const ULWord*>(buffer.data()), (ULWord)kFrameSize8MiB), true);
        AJATime::Sleep(150);
        CHECK_EQ(gCard->DMAReadFrame(frame_num, reinterpret_cast<ULWord*>(&readback[0]), (ULWord)kFrameSize8MiB), true);
        CHECK_EQ(memcmp(readback.data(), buffer.data(), kFrameSize8MiB), 0);
    }
    TEST_CASE("dma_write_read_all_frames" * doctest::description("DMAWriteFrame/DMAReadFrame all frames random data test")) {
        SetCardBaseline(gCard);
        NTV2Channel channel = NTV2_CHANNEL1;
        const ULWord num_frame_buffers = gCard->GetNumFrameBuffers();
        std::vector<uint8_t> buffer(kFrameSize8MiB);
        std::vector<uint8_t> readback(kFrameSize8MiB);
        size_t num_writes = 0;
        size_t num_reads = 0;
        size_t num_cmps = 0;
        for (ULWord i = 0; i < num_frame_buffers; i++) {
            gCard->SetMode(channel, NTV2_MODE_DISPLAY);
            gCard->SetOutputFrame(channel, i);
            uint32_t seed = 1;
            qa::RandomPattern<uint32_t>(buffer, kFrameSize8MiB, seed, 0, UINT32_MAX, qa::ByteOrder::LittleEndian);
            qa::RandomPattern<uint32_t>(readback, kFrameSize8MiB, seed, 0, UINT32_MAX, qa::ByteOrder::LittleEndian);
            if (gCard->DMAWriteFrame(i, reinterpret_cast<const ULWord*>(buffer.data()), (ULWord)kFrameSize8MiB))
                num_writes++;
            AJATime::Sleep(150);
            if (gCard->DMAReadFrame(i, reinterpret_cast<ULWord*>(&readback[0]), (ULWord)kFrameSize8MiB))
                num_reads++;
            if (memcmp(readback.data(), buffer.data(), kFrameSize8MiB) == 0)
                num_cmps++;
        }
        CHECK_EQ(num_writes, num_frame_buffers);
        CHECK_EQ(num_reads, num_frame_buffers);
        CHECK_EQ(num_cmps, num_frame_buffers);
    }
}

void ntv2card_framestore_marker() {}
TEST_SUITE("framestore_formats" * doctest::description("Framestore widget format tests")) {
    TEST_CASE("Framestore 1080p30 YUV-8") {
        auto vf = NTV2_FORMAT_1080p_3000;
        auto pf = NTV2_FBF_8BIT_YCBCR;
        CHECK_EQ(gCard->SetMode(NTV2_CHANNEL1, NTV2_MODE_DISPLAY), true);
        CHECK_EQ(gCard->SetVideoFormat(vf, false, false, NTV2_CHANNEL1), true);
        CHECK_EQ(gCard->SetFrameBufferFormat(NTV2_CHANNEL1, pf), true);
        NTV2FormatDesc fd(vf, pf);
        auto raster_size = fd.GetTotalBytes();
        std::vector<uint8_t> buffer(kFrameSize8MiB);
        std::vector<uint8_t> readback(kFrameSize8MiB);
        uint32_t seed = 1;
        qa::CountingPattern8Bit(buffer, fd.GetRasterHeight(), fd.GetBytesPerRow(), kSDILegalMin, kSDILegalMax8Bit);
        qa::RandomPattern<uint32_t>(readback, kFrameSize8MiB, seed, 0, UINT32_MAX, qa::ByteOrder::LittleEndian);
        CHECK_EQ(gCard->DMAWriteFrame(0, reinterpret_cast<const ULWord*>(buffer.data()), raster_size), true);
        AJATime::Sleep(150);
        CHECK_EQ(gCard->DMAReadFrame(0, reinterpret_cast<ULWord*>(&readback[0]), raster_size), true);
        CHECK_EQ(memcmp(buffer.data(), readback.data(), raster_size), 0);
    }
    TEST_CASE("Framestore UHDp30 YUV-8") {
        auto vf = NTV2_FORMAT_4x1920x1080p_3000;
        auto pf = NTV2_FBF_8BIT_YCBCR;
        CHECK_EQ(gCard->SetMode(NTV2_CHANNEL1, NTV2_MODE_DISPLAY), true);
        CHECK_EQ(gCard->SetVideoFormat(vf, false, false, NTV2_CHANNEL1), true);
        CHECK_EQ(gCard->SetFrameBufferFormat(NTV2_CHANNEL1, pf), true);
        NTV2FormatDesc fd(vf, pf);
        auto raster_size = fd.GetTotalBytes();
        std::vector<uint8_t> buffer(kFrameSize32MiB);
        std::vector<uint8_t> readback(kFrameSize32MiB);
        uint32_t seed = 1;
        qa::CountingPattern8Bit(buffer, fd.GetRasterHeight(), fd.GetBytesPerRow(), kSDILegalMin, kSDILegalMax8Bit);
        qa::RandomPattern<uint32_t>(readback, kFrameSize32MiB, seed, 0, UINT32_MAX, qa::ByteOrder::LittleEndian);
        CHECK_EQ(gCard->DMAWriteFrame(0, reinterpret_cast<const ULWord*>(buffer.data()), raster_size), true);
        AJATime::Sleep(150);
        CHECK_EQ(gCard->DMAReadFrame(0, reinterpret_cast<ULWord*>(&readback[0]), raster_size), true);
        CHECK_EQ(memcmp(buffer.data(), readback.data(), raster_size), 0);
    }
}

void ntv2card_sdi_loopback_marker() {}
TEST_SUITE("sdi_loopback" * doctest::description("SDI loopback tests")) {
    TEST_CASE("SDI Loopback 1-wire") {
        CNTV2DeviceScanner scanner;
        const size_t num_devices = scanner.GetNumDevices();
        if (num_devices < 2)
            LOGERR("SDI Loopback tests require two boards connected to the host system!");
        CHECK_GE(num_devices, 2);

        NTV2VideoFormat vf = NTV2_FORMAT_1080i_5994;
        NTV2PixelFormat pf = NTV2_FBF_8BIT_YCBCR;
        VPIDStandard vpid_standard = VPIDStandard_1080;
        NTV2VANCMode vanc_mode = NTV2_VANCMODE_OFF;
        NTV2ReferenceSource ref_src = NTV2_REFERENCE_FREERUN;
        NTV2FormatDescriptor fd(vf, pf, vanc_mode);
        auto src_router = AutoRouter(gOpts->card_a_index, gOpts->out_channel, gOpts->out_framestore,
            NTV2_MODE_DISPLAY, vf, pf,
            ref_src, vanc_mode, vpid_standard, kConnectionKindSDI);
        src_router.ApplyRouting(true, true);
        auto dst_router = AutoRouter(gOpts->card_b_index, gOpts->inp_channel, gOpts->inp_framestore,
            NTV2_MODE_CAPTURE, vf, pf,
            ref_src, vanc_mode, vpid_standard, kConnectionKindSDI);
        dst_router.ApplyRouting(true, true);

        std::vector<uint8_t> buffer(kFrameSize8MiB);
        std::vector<uint8_t> readback(kFrameSize8MiB);

        //TODO(paulh): If we use random numbers, we need to massage the values into SDI legal values first
        // or the SDI readback will not compare to the original.
        // qa::RandomPattern<uint32_t>(0xDECAFBAD, 0x1337c0d3, 0xdecafbad, buffer, qa::ByteOrder::LittleEndian);
        qa::AlternatingPattern<uint32_t>(buffer, kFrameSize8MiB, { 0xdecafbad, 0x1337c0d3 }, 1, qa::ByteOrder::LittleEndian);
        qa::RandomPattern<uint32_t>(readback, kFrameSize8MiB, 0xAB4D533D, 0, UINT32_MAX, qa::ByteOrder::LittleEndian);

        auto src_card = src_router.GetCard();
        auto dst_card = dst_router.GetCard();
        NTV2Channel out_channel = GetNTV2ChannelForIndex(gOpts->out_channel);
        NTV2Channel inp_channel = GetNTV2ChannelForIndex(gOpts->inp_channel);
        CHECK_EQ(src_card->SetMode(out_channel, NTV2_MODE_DISPLAY), true);
        CHECK_EQ(src_card->SetOutputFrame(out_channel, gOpts->out_frame), true);
        CHECK_EQ(src_card->DMAWriteFrame(gOpts->out_frame, reinterpret_cast<const ULWord*>(buffer.data()), (ULWord)kFrameSize8MiB), true);
        NTV2_POINTER f1_anc, f2_anc;
        src_card->DMAWriteAnc(gOpts->out_frame, f1_anc, f2_anc, out_channel);
        AJATime::Sleep(350);
        CHECK_EQ(dst_card->SetMode(inp_channel, NTV2_MODE_CAPTURE, false), true);
        CHECK_EQ(dst_card->SetInputFrame(inp_channel, gOpts->inp_frame), true);
        CHECK_EQ(dst_card->DMAReadFrame(gOpts->inp_frame, reinterpret_cast<ULWord*>(&readback[0]), (ULWord)kFrameSize8MiB), true);

        size_t raster_bytes = fd.GetTotalBytes();
        std::vector<qa::CompareResult<uint8_t>> cmp_results;
        CHECK_EQ(qa::CompareTestPatterns<uint8_t>(readback.data(), buffer.data(), raster_bytes, cmp_results), 0);
        CHECK_EQ(memcmp(readback.data(), buffer.data(), raster_bytes), 0);
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
        OPT_INTEGER('\0', "card_b", &gOpts->card_b_index, "card_b"),
        OPT_INTEGER('\0', "out_channel", &gOpts->out_channel, "output channel for board-to-board/loopback testing"),
        OPT_INTEGER('\0', "inp_channel", &gOpts->inp_channel, "input channel for board-to-board/loopback testing"),
        OPT_INTEGER('\0', "out_framestore", &gOpts->out_framestore, "output framestore for board-to-board/loopback testing"),
        OPT_INTEGER('\0', "inp_framestore", &gOpts->inp_framestore, "input framestore for board-to-board/loopback testing"),
        OPT_INTEGER('\0', "out_frame", &gOpts->out_frame, "output frame number for board-to-board/loopback testing"),
        OPT_INTEGER('\0', "inp_frame", &gOpts->inp_frame, "input frame number for board-to-board/loopback testing"),
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
