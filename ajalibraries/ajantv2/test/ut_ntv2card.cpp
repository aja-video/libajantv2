#define DOCTEST_CONFIG_IMPLEMENT
#define DOCTEST_THREAD_LOCAL
#include <doctest/doctest.h>
#include <argparse/argparse.h>
#include <rapidcsv/rapidcsv.h>

#include "test_support.hpp"

#include "ajabase/common/common.h"
#include "ajabase/system/debug.h"
#include "ajabase/system/file_io.h"
#include "ajabase/system/make_unique_shim.h"
#include "ajabase/system/systemtime.h"
#include "ajantv2/includes/ntv2card.h"
#include "ajantv2/utilityfiles/public/ntv2pixelcomponent.h"
#include "ajantv2/includes/ntv2devicefeatures.h"
#include "ajantv2/includes/ntv2devicescanner.h"
#include "ntv2qa/binary_pattern_gen.h"
#include "ntv2qa/binary_pattern_cmp.h"
#include "ntv2qa/counting_pattern_cmp.h"
// #include "ntv2qa/component_reader.h"
#include "ntv2qa/routing/preset_router.h"

#include <algorithm>
#include <functional>

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

struct TestCase {
    int id;
    std::string name;
    NTV2VideoFormat vf;
    NTV2PixelFormat pf;
    NTV2VANCMode vanc_mode;
    NTV2ReferenceSource ref_src;
    VPIDStandard standard;
    std::function<bool(const TestCase& tc)> func;
};
#define DOCTEST_PARAMETERIZE(data_container)                                  \
    std::for_each(data_container.begin(), data_container.end(), [&](const TestCase& in) {           \
        DOCTEST_SUBCASE(in.name.c_str()) { CHECK_EQ(in.func(in), true); }   \
    });

class FramestoreSDI {
public:
    FramestoreSDI(ULWord cdx1, ULWord cdx2)
        : _src_card{new CNTV2Card(cdx1)},
          _dst_card{new CNTV2Card(cdx2)},
          _test_json{},
          _clear_routing{true},
          _num_tests{0},
          _num_done{0},
          _num_pass{0},
          _num_fail{0},
          _failed_tests{} {}
    ~FramestoreSDI() {
        LOGINFO("[ framestore_sdi ] Tests: " << _num_done << "/" << _num_tests << " Pass: " << _num_pass << " Fail: " << _num_fail);
        std::ostringstream failed_oss;
        for (const auto& id : _failed_tests) {
            failed_oss << std::to_string(id) << " ";
        }
        std::string failed_id_str = failed_oss.str();
        aja::rstrip(failed_id_str);
        LOGINFO("Failed Test IDs: " << failed_id_str);
        delete _src_card;
        _src_card = nullptr;
        delete _dst_card;
        _dst_card = nullptr;
    }

    uint32_t NumTests() const { return _num_tests; }
    uint32_t NumDone() const { return _num_done; }
    uint32_t NumPass() const { return _num_pass; }
    uint32_t NumFail() const { return _num_fail; }

    AJAStatus Initialize(const std::string& json_path, std::vector<TestCase>& test_cases) {
        AJA_RETURN_STATUS(read_json_file(json_path, _test_json));
        for (auto&& vj : _test_json["vpid_tests"]) {
            auto vpid_db_id = vj["vpid_db_id"].get<int>(); // id of the original VPID test case from the QA Database
            auto vf = (NTV2VideoFormat)vj["vid_fmt_value"].get<int>();
            auto pf = (NTV2PixelFormat)vj["pix_fmt_value"].get<int>();
            auto vpid_standard_str = vj["smpte_id"].get_ref<std::string&>();
            auto vpid_standard = (VPIDStandard)std::stoul(vpid_standard_str, nullptr, 16);

            std::ostringstream oss_log;
            oss_log << "id_" << vpid_db_id << "_standard_0x" << vpid_standard_str << "_vf_" << NTV2VideoFormatToString(vf) << "_pf_" << NTV2FrameBufferFormatToString(pf, true);

            test_cases.push_back(TestCase{
                vpid_db_id,
                oss_log.str(),
                vf, pf,
                NTV2_VANCMODE_OFF,
                NTV2_REFERENCE_FREERUN,
                vpid_standard,
                std::bind(&FramestoreSDI::ExecuteTest, this, std::placeholders::_1)
            });
            _num_tests++;
        }
        return AJA_STATUS_SUCCESS;
    }

    bool ExecuteTest(const TestCase& tc) {
        const auto& vf = tc.vf;
        const auto& pf = tc.pf;
        const auto& vanc_mode = tc.vanc_mode;
        const auto& ref_src = tc.ref_src;
        const auto& vpid_standard = tc.standard;
        LOGINFO("[ framestore_sdi ] " << tc.name);
        if (!can_do_test_case(vf, pf)) {
            LOGWARN("test not supported!");
            _num_done++;
            _num_fail++;
            _failed_tests.push_back(tc.id);
            return false;
        }

        qa::PresetRouterConfig src_cfg {
            gOptions->out_channel, gOptions->out_framestore,
            NTV2_MODE_DISPLAY, vf, pf,
            ref_src, vanc_mode, vpid_standard, ConnectionKind::SDI
        };
        auto src_router = qa::PresetRouter(src_cfg, _src_card);
        qa::PresetRouterConfig dst_cfg {
            gOptions->inp_channel, gOptions->inp_framestore,
            NTV2_MODE_CAPTURE, vf, pf,
            ref_src, vanc_mode, vpid_standard, ConnectionKind::SDI
        };
        auto dst_router = qa::PresetRouter(dst_cfg, _dst_card);

        SetCardBaseline(_src_card, _clear_routing);
        SetCardBaseline(_dst_card, _clear_routing);

        NTV2FormatDescriptor fd(vf, pf, vanc_mode);
        ULWord frame_size = fd.GetTotalBytes();
        _buffer.clear();
        _buffer.resize(frame_size);
        _readback.clear();
        _readback.resize(frame_size);
        _zeroes.clear();
        _zeroes.resize(frame_size);

        // Zero out framebuffers on cards
        CHECK_EQ(_src_card->DMAWriteFrame(gOptions->out_frame, reinterpret_cast<const ULWord*>(_zeroes.data()), frame_size), true);
        CHECK_EQ(_dst_card->DMAWriteFrame(gOptions->inp_frame, reinterpret_cast<const ULWord*>(_zeroes.data()), frame_size), true);

        CHECK_EQ(src_router.ApplyRouting(_clear_routing, true), AJA_STATUS_SUCCESS);
        CHECK_EQ(dst_router.ApplyRouting(_clear_routing, true), AJA_STATUS_SUCCESS);
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
            qa::CountingPattern8Bit(_buffer, fd.GetRasterHeight(), stride, kSDILegalMin, kSDILegalMax8Bit);
        } else if (NTV2_IS_FBF_10BIT(pf)) {
            if (pf == NTV2_FBF_10BIT_DPX) {
                qa::CountingPattern10BitDPX(_buffer, fd.GetRasterHeight(), stride, kSDILegalMin, kSDILegalMax10Bit, qa::ByteOrder::BigEndian);
            } else if (pf == NTV2_FBF_10BIT_DPX_LE) {
                qa::CountingPattern10BitDPX(_buffer, fd.GetRasterHeight(), stride, kSDILegalMin, kSDILegalMax10Bit, qa::ByteOrder::LittleEndian);
            } else {
                qa::CountingPattern10Bit(_buffer, fd.GetRasterHeight(), stride, kSDILegalMin, kSDILegalMax10Bit, qa::ByteOrder::LittleEndian);
            }
        } else if (NTV2_IS_FBF_12BIT_RGB(pf)) {
            qa::CountingPattern12Bit(_buffer, fd.GetRasterHeight(), stride, kSDILegalMin, kSDILegalMax12Bit, qa::ByteOrder::LittleEndian);
        }

        // qa::RandomPattern<ULWord>(readback, frame_size, 0xAB4D533D, 0, UINT32_MAX, qa::ByteOrder::LittleEndian);

        NTV2Channel out_channel = GetNTV2ChannelForIndex(gOptions->out_channel);
        NTV2Channel inp_channel = GetNTV2ChannelForIndex(gOptions->inp_channel);
        CHECK_EQ(_src_card->SetMode(out_channel, NTV2_MODE_DISPLAY), true);
        CHECK_EQ(_src_card->SetOutputFrame(out_channel, gOptions->out_frame), true);
        CHECK_EQ(_src_card->DMAWriteFrame(gOptions->out_frame, reinterpret_cast<const ULWord*>(_buffer.data()), frame_size), true);
        AJATime::Sleep(1000); // TODO (paulh): Maybe we need to build a histogram of results and check for consistency over multiple readbacks?
        size_t raster_bytes = fd.GetTotalBytes();
        CHECK_EQ(_dst_card->SetMode(inp_channel, NTV2_MODE_CAPTURE, false), true);
        CHECK_EQ(_dst_card->SetInputFrame(inp_channel, gOptions->inp_frame), true);
        CHECK_EQ(_dst_card->DMAReadFrame(gOptions->inp_frame, reinterpret_cast<ULWord*>(&_readback[0]), (ULWord)raster_bytes), true);

        NTV2_POINTER master_bytes(_buffer.data(), raster_bytes);
        NTV2_POINTER compare_bytes(_readback.data(), raster_bytes);
        CNTV2PixelComponentReader master(master_bytes, fd);
        CNTV2PixelComponentReader compare(compare_bytes, fd);
        CHECK_EQ(master.ReadComponentValues(), AJA_STATUS_SUCCESS);
        CHECK_EQ(compare.ReadComponentValues(), AJA_STATUS_SUCCESS);
        bool result = (master == compare);
        if (result == true) {
            _num_pass++;
        } else {
            _num_fail++;
            _failed_tests.push_back(tc.id);
        }
        _num_done++;
        CHECK_EQ(result, true);
        return result;
    }

private:
    bool can_do_test_case(NTV2VideoFormat vf, NTV2PixelFormat pf) {
        auto src_card_id = _src_card->GetDeviceID();
        auto dst_card_id = _dst_card->GetDeviceID();
        if (!NTV2DeviceCanDoVideoFormat(src_card_id, vf)) {
            LOGWARN("Skipping test! Video format " << NTV2VideoFormatToString(vf) << " not supported by card " << NTV2DeviceIDToString(src_card_id));
            return false;
        }
        if (!NTV2DeviceCanDoFrameBufferFormat(src_card_id, pf)) {
            LOGWARN("Skipping test! Pixel format " << NTV2FrameBufferFormatToString(pf) << " not supported by card " << NTV2DeviceIDToString(src_card_id));
            return false;
        }
        if (!NTV2DeviceCanDoVideoFormat(dst_card_id, vf)) {
            LOGWARN("Skipping test! Video format " << NTV2VideoFormatToString(vf) << " not supported by card " << NTV2DeviceIDToString(dst_card_id));
            return false;
        }
        if (!NTV2DeviceCanDoFrameBufferFormat(dst_card_id, pf)) {
            LOGWARN("Skipping test! Pixel format " << NTV2FrameBufferFormatToString(pf) << " not supported by card " << NTV2DeviceIDToString(dst_card_id));
            return false;
        }
        return true;
    }

    CNTV2Card* _src_card;
    CNTV2Card* _dst_card;
    json _test_json;
    std::vector<UByte> _buffer;
    std::vector<UByte> _readback;
    std::vector<UByte> _zeroes;
    bool _clear_routing;
    uint32_t _num_tests;
    uint32_t _num_done;
    uint32_t _num_pass;
    uint32_t _num_fail;
    std::vector<int> _failed_tests;
};

using FramestoreSDIPtr = std::unique_ptr<FramestoreSDI>;

void ntv2card_framestore_sdi_marker() {}
TEST_SUITE("framestore_sdi" * doctest::description("SDI loopback tests")) {
    TEST_CASE("framestore_sdi_sd_hd") {
        static FramestoreSDIPtr fs_sdi = nullptr;
        static std::vector<TestCase> test_cases;
        if (fs_sdi == nullptr) {
            CNTV2DeviceScanner scanner;
            const size_t num_devices = scanner.GetNumDevices();
            if (num_devices < 2)
                LOGERR("SDI Loopback tests require two boards connected to the host system!");
            REQUIRE_GE(num_devices, 2);

            std::string exe_path, exe_dir;
            REQUIRE_EQ(AJAFileIO::GetExecutablePath(exe_path), AJA_STATUS_SUCCESS);
            REQUIRE_EQ(AJAFileIO::GetDirectoryName(exe_path, exe_dir), AJA_STATUS_SUCCESS);
            std::string vpid_json_path = exe_dir + AJA_PATHSEP + "json" + AJA_PATHSEP + "sdi_sd_hd.json";
            REQUIRE_EQ(AJAFileIO::FileExists(vpid_json_path), true);

            fs_sdi = aja::make_unique<FramestoreSDI>(gOptions->card_a_index, gOptions->card_b_index);
            REQUIRE_EQ(fs_sdi->Initialize(vpid_json_path, test_cases), AJA_STATUS_SUCCESS);
        }
        // Doctest calls SUBCASE recursively per-TEST_CASE, so we have to ensure the test case data is only established once per test case.
        // This is done in order to get parameterized test results at the end.
        DOCTEST_PARAMETERIZE(test_cases);
    }
    TEST_CASE("framestore_sdi_uhd_4k") {
        static FramestoreSDI* fs_sdi = nullptr;
        static std::vector<TestCase> test_cases;
        if (fs_sdi == nullptr) {
            CNTV2DeviceScanner scanner;
            const size_t num_devices = scanner.GetNumDevices();
            if (num_devices < 2)
                LOGERR("SDI Loopback tests require two boards connected to the host system!");
            REQUIRE_GE(num_devices, 2);

            std::string exe_path, exe_dir;
            REQUIRE_EQ(AJAFileIO::GetExecutablePath(exe_path), AJA_STATUS_SUCCESS);
            REQUIRE_EQ(AJAFileIO::GetDirectoryName(exe_path, exe_dir), AJA_STATUS_SUCCESS);
            std::string vpid_json_path = exe_dir + AJA_PATHSEP + "json" + AJA_PATHSEP + "sdi_uhd_4k.json";
            REQUIRE_EQ(AJAFileIO::FileExists(vpid_json_path), true);

            fs_sdi = new FramestoreSDI(gOptions->card_a_index, gOptions->card_b_index);
            REQUIRE_EQ(fs_sdi->Initialize(vpid_json_path, test_cases), AJA_STATUS_SUCCESS);
        }
        // Doctest calls SUBCASE recursively per-TEST_CASE, so we have to ensure the test case data is only established once per test case.
        // This is done in order to get parameterized test results at the end.
        DOCTEST_PARAMETERIZE(test_cases);
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
