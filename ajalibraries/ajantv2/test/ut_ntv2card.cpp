#define DOCTEST_CONFIG_IMPLEMENT
#define DOCTEST_THREAD_LOCAL
#include <doctest/doctest.h>
#include <argparse/argparse.h>
#include <rapidcsv/rapidcsv.h>

#include "test_support.hpp"

#include "ajabase/system/file_io.h"
#include "ajabase/system/systemtime.h"
#include "ajabase/system/debug.h"
#include "ajantv2/includes/ntv2card.h"
#include "ajantv2/includes/ntv2pixelcomponent.h"
#include "ajantv2/includes/ntv2devicefeatures.h"
#include "ajantv2/includes/ntv2devicescanner.h"
#include "ntv2qa/binary_pattern_gen.h"
#include "ntv2qa/binary_pattern_cmp.h"
#include "ntv2qa/counting_pattern_cmp.h"
// #include "ntv2qa/component_reader.h"
#include "ntv2qa/routing/preset_router.h"

#include <algorithm>

#define LOG_QUIET 1

#define AJA_PRINT_REPORT(unit,severity,msg) do {\
    std::ostringstream	oss; oss << msg;\
    std::cout << "(" << AJADebug::SeverityName(severity) << ") " << oss.str() << std::endl;\
    std::ostringstream oss2; oss2 << msg;\
    AJADebug::Report(unit, severity, __FILE__, __LINE__, oss2.str());\
} while (false)
#define LOGERR(msg)  AJA_PRINT_REPORT(AJA_DebugUnit_Testing, AJA_DebugSeverity_Error, msg)
#define LOGWARN(msg) AJA_PRINT_REPORT(AJA_DebugUnit_Testing, AJA_DebugSeverity_Warning, msg)
#define LOGNOTE(msg) AJA_PRINT_REPORT(AJA_DebugUnit_Testing, AJA_DebugSeverity_Notice, msg)
#define LOGINFO(msg) AJA_PRINT_REPORT(AJA_DebugUnit_Testing, AJA_DebugSeverity_Info, msg)
#define LOGDBG(msg)  AJA_PRINT_REPORT(AJA_DebugUnit_Testing, AJA_DebugSeverity_Debug, msg)

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
static constexpr UByte kSDILegalMin = 0x04;
static constexpr UByte kSDILegalMax8Bit = 0xfb;
static constexpr UWord kSDILegalMax10Bit = 0x3fb;
static constexpr ULWord kSDILegalMax12Bit = 0xffc;
static const char* kVpidJsonFilename = "ntv2_vpid_debug.json";

struct TestOptions {
    ULWord card_a_index;
    ULWord card_b_index;
    ULWord out_channel;
    ULWord inp_channel;
    ULWord out_framestore;
    ULWord inp_framestore;
    ULWord out_frame;
    ULWord inp_frame;
};
static TestOptions* gOptions = NULL;
static CNTV2Card* gCard = NULL;
static CNTV2Card* gCard2 = NULL;
static NTV2EveryFrameTaskMode gTaskMode = NTV2_TASK_MODE_INVALID;

static void SetCardBaseline(CNTV2Card* card, bool clear=false)
{
    if (card) {
        NTV2DeviceID id = card->GetDeviceID();
        if (clear)
            card->ClearRouting();
        for (ULWord i = 0; i < NTV2DeviceGetNumVideoChannels(id); i++) {
            NTV2Channel ch = GetNTV2ChannelForIndex(i);
            NTV2AudioSystem as = NTV2ChannelToAudioSystem(ch);
            card->SetEveryFrameServices(NTV2_OEM_TASKS);
            card->SetReference(NTV2_REFERENCE_FREERUN);
            card->StopAudioInput(as);
            card->StopAudioOutput(as);
            card->AutoCirculateStop(ch);
        }
    }
}

void ntv2card_general_marker() {}
TEST_SUITE("card_general" * doctest::description("CNTV2Card general tests")) {
    TEST_CASE("card_is_open") {
        CHECK(gCard->IsOpen() == true);
    }
    TEST_CASE("card_get_device_id") {
        CHECK(gCard->GetDeviceID() != DEVICE_ID_NOTFOUND);
    }
    TEST_CASE("card_set_every_frame_services") {
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
    TEST_CASE("dma_write_read_one_frame" * doctest::description("DMAWriteFrame/DMAReadFrame frame 0 random data test")) {
        NTV2Channel channel = NTV2_CHANNEL1;
        ULWord frame_num = 0;
        std::vector<UByte> buffer(kFrameSize8MiB);
        std::vector<UByte> readback(kFrameSize8MiB);
        ULWord seed = 1;
        qa::RandomPattern<ULWord>(buffer, kFrameSize8MiB, seed, 0, UINT32_MAX, qa::ByteOrder::LittleEndian);
        qa::RandomPattern<ULWord>(readback, kFrameSize8MiB, seed, 0, UINT32_MAX, qa::ByteOrder::LittleEndian);
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
        std::vector<UByte> buffer(kFrameSize8MiB);
        std::vector<UByte> readback(kFrameSize8MiB);
        size_t num_writes = 0;
        size_t num_reads = 0;
        size_t num_cmps = 0;
        for (ULWord i = 0; i < num_frame_buffers; i++) {
            gCard->SetMode(channel, NTV2_MODE_DISPLAY);
            gCard->SetOutputFrame(channel, i);
            ULWord seed = 1;
            qa::RandomPattern<ULWord>(buffer, kFrameSize8MiB, seed, 0, UINT32_MAX, qa::ByteOrder::LittleEndian);
            qa::RandomPattern<ULWord>(readback, kFrameSize8MiB, seed, 0, UINT32_MAX, qa::ByteOrder::LittleEndian);
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
        LOGINFO(num_writes << " DMA Writes | " << num_reads << " DMA Reads | " << num_cmps << " Buffer Compares");
    }
    TEST_CASE("dma_write_read_audio") {
        NTV2Channel channel = NTV2_CHANNEL1;
        NTV2AudioSystem audio_sys = NTV2ChannelToAudioSystem(channel);
        gCard->StopAudioInput(audio_sys);
        gCard->StopAudioOutput(audio_sys);
        std::vector<UByte> buffer(kAudioSize4MiB);
        std::vector<UByte> readback(kAudioSize4MiB);
        ULWord seed = 1;
        qa::RandomPattern<ULWord>(buffer, kAudioSize4MiB, seed, 0, UINT32_MAX, qa::ByteOrder::LittleEndian);
        qa::RandomPattern<ULWord>(readback, kAudioSize4MiB, seed, 0, UINT32_MAX, qa::ByteOrder::LittleEndian);
        CHECK_EQ(gCard->SetMode(channel, NTV2_MODE_DISPLAY), true);
        CHECK_EQ(gCard->DMAWriteAudio(audio_sys, reinterpret_cast<const ULWord*>(buffer.data()), 0, kAudioSize4MiB), true);
        AJATime::Sleep(150);
        CHECK_EQ(gCard->DMAReadAudio(audio_sys, reinterpret_cast<ULWord*>(&readback[0]), 0, kAudioSize4MiB), true);
        CHECK_EQ(memcmp(readback.data(), buffer.data(), kAudioSize4MiB), 0);
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
        std::vector<UByte> buffer(kFrameSize8MiB);
        std::vector<UByte> readback(kFrameSize8MiB);
        ULWord seed = 1;
        qa::CountingPattern8Bit(buffer, fd.GetRasterHeight(), fd.linePitch, kSDILegalMin, kSDILegalMax8Bit);
        qa::RandomPattern<ULWord>(readback, kFrameSize8MiB, seed, 0, UINT32_MAX, qa::ByteOrder::LittleEndian);
        std::vector<UByte> zeroes(kFrameSize8MiB);
        CHECK_EQ(gCard->DMAWriteFrame(0, reinterpret_cast<const ULWord*>(zeroes.data()), (ULWord)kFrameSize8MiB), true);
        CHECK_EQ(gCard->DMAWriteFrame(0, reinterpret_cast<const ULWord*>(buffer.data()), raster_size), true);
        AJATime::Sleep(150);
        CHECK_EQ(gCard->DMAReadFrame(0, reinterpret_cast<ULWord*>(&readback[0]), raster_size), true);
        CHECK_EQ(memcmp(buffer.data(), readback.data(), raster_size), 0);
    }
    // TEST_CASE("Framestore UHDp30 YUV-8") {
    //     auto vf = NTV2_FORMAT_4x1920x1080p_3000;
    //     auto pf = NTV2_FBF_8BIT_YCBCR;
    //     CHECK_EQ(gCard->SetMode(NTV2_CHANNEL1, NTV2_MODE_DISPLAY), true);
    //     CHECK_EQ(gCard->SetVideoFormat(vf, false, false, NTV2_CHANNEL1), true);
    //     CHECK_EQ(gCard->SetFrameBufferFormat(NTV2_CHANNEL1, pf), true);
    //     NTV2FormatDesc fd(vf, pf);
    //     auto raster_size = fd.GetTotalBytes();
    //     std::vector<UByte> buffer(kFrameSize32MiB);
    //     std::vector<UByte> readback(kFrameSize32MiB);
    //     ULWord seed = 1;
    //     qa::CountingPattern8Bit(buffer, fd.GetRasterHeight(), fd.GetBytesPerRow(), kSDILegalMin, kSDILegalMax8Bit);
    //     qa::RandomPattern<ULWord>(readback, kFrameSize32MiB, seed, 0, UINT32_MAX, qa::ByteOrder::LittleEndian);
    //     CHECK_EQ(gCard->DMAWriteFrame(0, reinterpret_cast<const ULWord*>(buffer.data()), raster_size), true);
    //     AJATime::Sleep(150);
    //     CHECK_EQ(gCard->DMAReadFrame(0, reinterpret_cast<ULWord*>(&readback[0]), raster_size), true);
    //     CHECK_EQ(memcmp(buffer.data(), readback.data(), raster_size), 0);
    // }
}

static bool gIsRouted = false;

#define DOCTEST_PARAMETERIZE(name, func) \
        DOCTEST_SUBCASE(name.c_str()) { func(gIsRouted); }

void ntv2card_framestore_sdi_marker() {}
TEST_SUITE("framestore_sdi" * doctest::description("SDI loopback tests")) {
    TEST_CASE("framestore_sdi_json") {
        CNTV2DeviceScanner scanner;
        const size_t num_devices = scanner.GetNumDevices();
        if (num_devices < 2)
            LOGERR("SDI Loopback tests require two boards connected to the host system!");
        REQUIRE_GE(num_devices, 2);

        std::string exe_path, exe_dir;
        AJAFileIO::GetExecutablePath(exe_path);
        AJAFileIO::GetDirectoryName(exe_path, exe_dir);
        std::string vpid_json_path = exe_dir + AJA_PATHSEP + kVpidJsonFilename;
        json vpid_json;
        REQUIRE_EQ(read_json_file(vpid_json_path, vpid_json), AJA_STATUS_SUCCESS);

        std::vector<UByte> buffer(kFrameSize8MiB);
        std::vector<UByte> readback(kFrameSize8MiB);
        std::vector<UByte> zeroes(kFrameSize8MiB);

        for (auto&& vj : vpid_json["vpid_tests"]) {
            auto vpid_db_id = vj["vpid_db_id"].get<int>();
            auto vf = (NTV2VideoFormat)vj["vid_fmt_value"].get<int>();
            auto pf = (NTV2PixelFormat)vj["pix_fmt_value"].get<int>();
            auto vpid_standard_str = vj["smpte_id"].get_ref<std::string&>();
            auto vpid_standard = (VPIDStandard)std::stoul(vpid_standard_str, nullptr, 16);
            NTV2VANCMode vanc_mode = NTV2_VANCMODE_OFF;
            NTV2ReferenceSource ref_src = NTV2_REFERENCE_FREERUN;
            NTV2FormatDescriptor fd(vf, pf, vanc_mode);

            std::ostringstream oss_log;
            oss_log << "framestore_sdi_test_" << vpid_db_id << "_standard_0x" << vpid_standard_str << "_vf_" << NTV2VideoFormatToString(vf) << "_pf_" << NTV2FrameBufferFormatToString(pf, true);
            LOGINFO(oss_log.str());

            CNTV2Card* src_card = gCard;
            CNTV2Card* dst_card = gCard2;
            NTV2DeviceID src_card_id = src_card->GetDeviceID();
            NTV2DeviceID dst_card_id = dst_card->GetDeviceID();
            if (!NTV2DeviceCanDoVideoFormat(src_card_id, vf)) {
                LOGWARN("Skipping test. Video format " << NTV2VideoFormatToString(vf) << " not supported by card " << NTV2DeviceIDToString(src_card_id));
                continue;
            }
            if (!NTV2DeviceCanDoFrameBufferFormat(src_card_id, pf)) {
                LOGWARN("Skipping test. Pixel format " << NTV2FrameBufferFormatToString(pf) << " not supported by card " << NTV2DeviceIDToString(src_card_id));
                continue;
            }
            if (!NTV2DeviceCanDoVideoFormat(dst_card_id, vf)) {
                LOGWARN("Skipping test. Video format " << NTV2VideoFormatToString(vf) << " not supported by card " << NTV2DeviceIDToString(dst_card_id));
                continue;
            }
            if (!NTV2DeviceCanDoFrameBufferFormat(dst_card_id, pf)) {
                LOGWARN("Skipping test. Pixel format " << NTV2FrameBufferFormatToString(pf) << " not supported by card " << NTV2DeviceIDToString(dst_card_id));
                continue;
            }

            auto test_func = [&](bool& is_routed) -> bool {
                qa::PresetRouterConfig cfg_a {
                    gOptions->out_channel, gOptions->out_framestore,
                    NTV2_MODE_DISPLAY, vf, pf,
                    ref_src, vanc_mode, vpid_standard, ConnectionKind::SDI
                };
                auto src_router = qa::PresetRouter(cfg_a, src_card);
                qa::PresetRouterConfig cfg_b {
                    gOptions->inp_channel, gOptions->inp_framestore,
                    NTV2_MODE_CAPTURE, vf, pf,
                    ref_src, vanc_mode, vpid_standard, ConnectionKind::SDI
                };
                auto dst_router = qa::PresetRouter(cfg_b, dst_card);

                SetCardBaseline(src_card, !is_routed);
                SetCardBaseline(dst_card, !is_routed);

                ULWord frame_size = fd.GetTotalBytes();
                buffer.clear();
                buffer.resize(frame_size);
                readback.clear();
                readback.resize(frame_size);
                zeroes.clear();
                zeroes.resize(frame_size);
                // Zero out framebuffers
                CHECK_EQ(src_card->DMAWriteFrame(gOptions->out_frame, reinterpret_cast<const ULWord*>(zeroes.data()), frame_size), true);
                CHECK_EQ(dst_card->DMAWriteFrame(gOptions->inp_frame, reinterpret_cast<const ULWord*>(zeroes.data()), frame_size), true);

                CHECK_EQ(src_router.ApplyRouting(!is_routed, true), AJA_STATUS_SUCCESS);
                CHECK_EQ(dst_router.ApplyRouting(!is_routed, true), AJA_STATUS_SUCCESS);
                AJATime::Sleep(500);
                // if (!is_routed) {
                //     is_routed = true;
                // }

                //TODO(paulh): If we use random numbers, we need to massage the values into SDI legal values first
                // or the SDI readback will not compare to the original.
                // qa::RandomPattern<ULWord>(0xDECAFBAD, 0x1337c0d3, 0xdecafbad, buffer, qa::ByteOrder::LittleEndian);
                // qa::AlternatingPattern<ULWord>(buffer, frame_size, { 0xdecafbad, 0x1337c0d3 }, 1, qa::ByteOrder::LittleEndian);
                auto stride = fd.linePitch;
                if (NTV2_IS_FBF_8BIT(pf) || pf == NTV2_FBF_24BIT_RGB || pf == NTV2_FBF_24BIT_BGR) {
                    qa::CountingPattern8Bit(buffer, fd.GetRasterHeight(), stride, kSDILegalMin, kSDILegalMax8Bit);
                } else if (NTV2_IS_FBF_10BIT(pf)) {
                    if (pf == NTV2_FBF_10BIT_DPX) {
                        qa::CountingPattern10BitDPX(buffer, fd.GetRasterHeight(), stride, kSDILegalMin, kSDILegalMax10Bit, qa::ByteOrder::BigEndian);
                    } else if (pf == NTV2_FBF_10BIT_DPX_LE) {
                        qa::CountingPattern10BitDPX(buffer, fd.GetRasterHeight(), stride, kSDILegalMin, kSDILegalMax10Bit, qa::ByteOrder::LittleEndian);
                    } else {
                        qa::CountingPattern10Bit(buffer, fd.GetRasterHeight(), stride, kSDILegalMin, kSDILegalMax10Bit, qa::ByteOrder::LittleEndian);
                    }
                } else if (NTV2_IS_FBF_12BIT_RGB(pf)) {
                    qa::CountingPattern12Bit(buffer, fd.GetRasterHeight(), stride, kSDILegalMin, kSDILegalMax12Bit, qa::ByteOrder::LittleEndian);
                }

                // qa::RandomPattern<ULWord>(readback, frame_size, 0xAB4D533D, 0, UINT32_MAX, qa::ByteOrder::LittleEndian);

                NTV2Channel out_channel = GetNTV2ChannelForIndex(gOptions->out_channel);
                NTV2Channel inp_channel = GetNTV2ChannelForIndex(gOptions->inp_channel);
                CHECK_EQ(src_card->SetMode(out_channel, NTV2_MODE_DISPLAY), true);
                CHECK_EQ(src_card->SetOutputFrame(out_channel, gOptions->out_frame), true);
                CHECK_EQ(src_card->DMAWriteFrame(gOptions->out_frame, reinterpret_cast<const ULWord*>(buffer.data()), frame_size), true);
                AJATime::Sleep(1000); // TODO (paulh): Maybe we need to build a histogram of results and check for consistency over multiple readbacks?
                size_t raster_bytes = fd.GetTotalBytes();
                CHECK_EQ(dst_card->SetMode(inp_channel, NTV2_MODE_CAPTURE, false), true);
                CHECK_EQ(dst_card->SetInputFrame(inp_channel, gOptions->inp_frame), true);
                CHECK_EQ(dst_card->DMAReadFrame(gOptions->inp_frame, reinterpret_cast<ULWord*>(&readback[0]), (ULWord)raster_bytes), true);

                NTV2_POINTER master_bytes(buffer.data(), raster_bytes);
                NTV2_POINTER compare_bytes(readback.data(), raster_bytes);
                CNTV2PixelComponentReader master(master_bytes, fd);
                CNTV2PixelComponentReader compare(compare_bytes, fd);
                CHECK_EQ(master.ReadComponentValues(), AJA_STATUS_SUCCESS);
                CHECK_EQ(compare.ReadComponentValues(), AJA_STATUS_SUCCESS);
                CHECK(master == compare);
                return true;
            };

            DOCTEST_PARAMETERIZE(oss_log.str(), test_func);
        }
    }
}

int main(int argc, const char** argv) {
    AJADebug::Open();

    gOptions = new TestOptions();
    gOptions->card_a_index = 0;
    gOptions->card_b_index = 1;

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
        OPT_INTEGER('\0', "card", &gOptions->card_a_index, "card_a"),
        OPT_INTEGER('\0', "card_b", &gOptions->card_b_index, "card_b"),
        OPT_INTEGER('\0', "out_channel", &gOptions->out_channel, "output channel for board-to-board/loopback testing"),
        OPT_INTEGER('\0', "inp_channel", &gOptions->inp_channel, "input channel for board-to-board/loopback testing"),
        OPT_INTEGER('\0', "out_framestore", &gOptions->out_framestore, "output framestore for board-to-board/loopback testing"),
        OPT_INTEGER('\0', "inp_framestore", &gOptions->inp_framestore, "input framestore for board-to-board/loopback testing"),
        OPT_INTEGER('\0', "out_frame", &gOptions->out_frame, "output frame number for board-to-board/loopback testing"),
        OPT_INTEGER('\0', "inp_frame", &gOptions->inp_frame, "input frame number for board-to-board/loopback testing"),
        OPT_END(),
    };
    struct argparse argparse;
    argparse_init(&argparse, options, usage, ARGPARSE_IGNORE_UNKNOWN_ARGS);
    argparse_describe(&argparse, "\nntv2 card unit tests",
        "\nPerform CNTV2Card tests against physical hardware.");
    argparse_parse(&argparse, argc, (const char**)argv);

    gCard = new CNTV2Card(gOptions->card_a_index, "");
    gCard2 = new CNTV2Card(gOptions->card_b_index, "");
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
    delete gOptions;

    AJADebug::Close();

    return ctx.shouldExit() ? res : EXIT_SUCCESS;
}
