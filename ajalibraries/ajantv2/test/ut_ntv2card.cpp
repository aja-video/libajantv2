/* ut_ntv2card - Application for physical ntv2 card testing.
 *
 * This application can perform a variety of either board-to-board or board-to-self tests.
 *
 * 1. [ Currently Supported Tests ]
 *  - card_dma: dma_write_read_one_frame, dma_write_read_all_frames, dma_write_read_audio
 *  This tests the gamut of basic DMA functionality on NTV2 boards.
 *
 *  - framestore_sdi: framestore_sdi_base test case is driven and parameterized from the JSON format tables.
 *  This test configures a framestore for a particular video/pixel format, outputs to SDI, captures the output
 *  signal into another SDI, and validates the frame data captured matches the output data.
 *
 *  The Framestore SDI tests are driven from the VPID format tables, stored in JSON format in the 'json' sub-directory.
 *
 * 2. [ Planned Future Tests ]
 *  - card_dma: Ancillary data DMA and readback
 *  - framestore_sdi: VPID output and readback validation
 *
 * 3. [ Usage Examples ]
 *  a. Board-to-board: output from one SDI to another on a different board
 *  > ut_ntv2card.exe --card=0 --card_b=1
 *
 *  b. Board-to-self: output from one SDI to another on the same board
 *  > ut_ntv2card.exe --card=0 --card_b=0
 *
 *  c. Specify specific FramestoreSDI test cases from JSON via a list of comma-separated IDs:
 *  > ut_ntv2card.exe --card=0 --card_b=1 --tc_ids=1,2,3,4
 *
 *  d. Run only a specified Doctest "TEST_SUITE":
 *  > ut_ntv2card.exe --card=0 --card_b=1 -ts=framestore_sdi
 *
 *  e. Run only a specified Doctest "TEST_CASE":
 *  > ut_ntv2card.exe --card=0 --card_b=1 -tc=dma_write_read_one_frame
 */

#define DOCTEST_CONFIG_IMPLEMENT
#define DOCTEST_THREAD_LOCAL
#include "doctest.h"
#include "argparse.h"
#include "rapidcsv.h"

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

#include "ntv2qa/card_config.hpp"
#include "ntv2qa/component_reader.h"
#include "ntv2qa/test_pattern.h"
#include "ntv2qa/routing/preset_router.h"
#include "ntv2qa/video.h"

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

static constexpr uint32_t kDefaultAudioChannels = 8;
static constexpr size_t kAudioSize1MiB = 0xff000;
// static constexpr size_t kAudioSize2MiB = kAudioSize1MiB * 2;
static constexpr size_t kAudioSize4MiB = kAudioSize1MiB * 4;
static constexpr size_t kFrameSize8MiB = 0x800000;
// static constexpr size_t kFrameSize16MiB = kFrameSize8MiB * 2;
static constexpr size_t kFrameSize32MiB = kFrameSize8MiB * 4;

struct TestOptions {
    ULWord card_a_index {0};
    ULWord card_b_index {0};
    ULWord out_channel {0};
    ULWord inp_channel {0};
    ULWord out_framestore {0};
    ULWord inp_framestore {0};
    ULWord out_frame {0};
    ULWord inp_frame {0};
    std::string tc_ids{};
};
static TestOptions* gOptions = NULL;
static CNTV2Card* gCard = NULL;
static CNTV2Card* gCard2 = NULL;
static NTV2EveryFrameTaskMode gTaskMode = NTV2_TASK_MODE_INVALID;

static void SetCardDefaults(CNTV2Card* card, bool clear=false)
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
        CHECK_EQ(gCard->DMAWriteFrame(frame_num, (const ULWord*)buffer.data(), (ULWord)kFrameSize8MiB), true);
        AJATime::Sleep(150);
        CHECK_EQ(gCard->DMAReadFrame(frame_num, reinterpret_cast<ULWord*>(&readback[0]), (ULWord)kFrameSize8MiB), true);
        CHECK_EQ(memcmp(readback.data(), buffer.data(), kFrameSize8MiB), 0);
    }
    TEST_CASE("dma_write_read_all_frames" * doctest::description("DMAWriteFrame/DMAReadFrame all frames random data test")) {
        SetCardDefaults(gCard);
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
            if (gCard->DMAWriteFrame(i, (const ULWord*)buffer.data(), (ULWord)kFrameSize8MiB))
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
        CHECK_EQ(gCard->DMAWriteAudio(audio_sys, (const ULWord*)buffer.data(), 0, kAudioSize4MiB), true);
        AJATime::Sleep(150);
        CHECK_EQ(gCard->DMAReadAudio(audio_sys, (ULWord*)&readback[0], 0, kAudioSize4MiB), true);
        CHECK_EQ(memcmp(readback.data(), buffer.data(), kAudioSize4MiB), 0);
    }
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

// NOTE(paulh): Doctest calls SUBCASE recursively per-TEST_CASE,
// in order to parameterize the test cases and results.
// We must ensure that the test case data is only initialized once per test case.
#define DOCTEST_PARAMETERIZE(data_container) \
    std::for_each(data_container.begin(), data_container.end(), [&](const TestCase& in) { \
        DOCTEST_SUBCASE(in.name.c_str()) { CHECK_EQ(in.func(in), true); } \
    });

class FramestoreSDI {
public:
    FramestoreSDI(ULWord cdx1, ULWord cdx2)
        : _src_card{nullptr},
          _dst_card{nullptr},
          _test_json{},
          _clear_routing{true},
          _num_tests{0},
          _num_done{0},
          _num_pass{0},
          _num_fail{0},
          _failed_tests{} {
            _src_card = new CNTV2Card(cdx1);
            if (cdx1 == cdx2)
                _dst_card = _src_card;
            else
                _dst_card = new CNTV2Card(cdx2);
          }
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

    AJAStatus Initialize(const NTV2StringList& json_paths, std::vector<TestCase>& test_cases) {
        if (test_cases.size() == 0) {
            for (const auto& jp : json_paths) {
                LOGINFO("Adding Framestore SDI test cases from JSON: " << jp);
                REQUIRE_EQ(AJAFileIO::FileExists(jp), true);
                if (!read_json_file(jp, _test_json)) {
                    LOGERR("Error reading Framestore SDI test case JSON file!");
                    return AJA_STATUS_FAIL;
                }

                std::vector<int> tc_id_filter;
                if (!gOptions->tc_ids.empty()) {
                    NTV2StringList tc_id_strings;
                    aja::split(gOptions->tc_ids, ',', tc_id_strings);
                    for (auto&& id : tc_id_strings)
                        tc_id_filter.emplace_back(std::stoi(id));
                }

                for (auto&& vj : _test_json["vpid_tests"]) {
                    // id of the original VPID test case from the QA Database
                    auto vpid_db_id = vj["vpid_db_id"].get<int>();

                    // optionally filter test cases based on args
                    if (!tc_id_filter.empty()) {
                        bool run_test = false;
                        for (const auto& id : tc_id_filter) {
                            if (vpid_db_id == id) {
                                run_test = true;
                                break;
                            }
                        }
                        if (!run_test) {
                            LOGINFO("- Skipping Framestore SDI test case ID: " << vpid_db_id);
                            continue;
                        }
                    }

                    auto vf = (NTV2VideoFormat)vj["vid_fmt_value"].get<int>();
                    auto pf = (NTV2PixelFormat)vj["pix_fmt_value"].get<int>();
                    auto vpid_standard_str = vj["smpte_id"].get_ref<std::string&>();
                    auto vpid_standard = (VPIDStandard)std::stoul(
                        vpid_standard_str, nullptr, 16);

                    std::ostringstream oss_log;
                    oss_log << "id_" << vpid_db_id
                        << "_standard_0x" << vpid_standard_str
                        << "_vf_" << NTV2VideoFormatToString(vf)
                        << "_pf_" << NTV2FrameBufferFormatToString(pf, true);

                    test_cases.push_back(TestCase{
                        vpid_db_id,
                        oss_log.str(),
                        vf, pf,
                        NTV2_VANCMODE_OFF,
                        NTV2_REFERENCE_FREERUN,
                        vpid_standard,
                        std::bind(&FramestoreSDI::ExecuteTest,
                            this, std::placeholders::_1)
                    });
                    // LOGINFO("- Added Framestore SDI test case ID: " << vpid_db_id);
                    _num_tests++;
                }
            }
        }
        return AJA_STATUS_SUCCESS;
    }

    bool ExecuteTest(const TestCase& tc) {
        uint64_t tc_start = AJATime::GetSystemMilliseconds();
        bool use_single_card = _src_card == _dst_card;
        const auto& vf = tc.vf;
        const auto& pf = tc.pf;
        const auto& vanc_mode = tc.vanc_mode;
        const auto& ref_src = tc.ref_src;
        const auto& vpid_standard = tc.standard;
        LOGINFO("[ framestore_sdi ] " << tc.name);
        //TODO(paulh): Add method to PresetRouter to get preset
        // so we can filter device applicability based on num channels, etc.
        if (!can_do_test_case(vf, pf)) {
            LOGWARN("test not supported!");
            _num_done++;
            _num_fail++;
            _failed_tests.push_back(tc.id);
            return false;
        }

        qa::QACardConfig src_cfg;
        src_cfg.base.index = gOptions->card_a_index;
        src_cfg.base.multi_format = false;
        src_cfg.base.channel = GetNTV2ChannelForIndex(gOptions->out_channel);
        src_cfg.base.framestore = GetNTV2ChannelForIndex(gOptions->out_framestore);
        src_cfg.base.mode = NTV2_MODE_DISPLAY;
        src_cfg.video.ntv2_vid_fmt = vf;
        src_cfg.video.ntv2_pix_fmt = pf;
        src_cfg.video.ntv2_vanc_mode = vanc_mode;
        src_cfg.timing.ntv2_ref_src = ref_src;
        src_cfg.video.smpte_standard = vpid_standard;
        src_cfg.base.connection = qa::ConnectionKind::SDI;
        qa::QACardConfig dst_cfg(src_cfg);
        dst_cfg.base.index = gOptions->card_b_index;
        dst_cfg.base.channel = GetNTV2ChannelForIndex(gOptions->inp_channel);
        dst_cfg.base.framestore = GetNTV2ChannelForIndex(gOptions->inp_framestore);
        dst_cfg.base.mode = NTV2_MODE_CAPTURE;
        auto src_router = qa::QAPresetRouter(src_cfg, _src_card);
        auto dst_router = qa::QAPresetRouter(dst_cfg, _dst_card);

        SetCardDefaults(_src_card, _clear_routing);
        SetCardDefaults(_dst_card, _clear_routing);

        const NTV2FormatDesc& fd = src_cfg.video.GetFormatDesc();
        ULWord frame_size = fd.GetTotalBytes();
        _buffer.clear();
        _buffer.resize(frame_size);
        _readback.clear();
        _readback.resize(frame_size);
        _zeroes.clear();
        _zeroes.resize(frame_size);

        // Zero out framebuffers on cards
        CHECK_EQ(_src_card->DMAWriteFrame(gOptions->out_frame, (const ULWord*)_zeroes.data(), frame_size), true);
        CHECK_EQ(_dst_card->DMAWriteFrame(gOptions->inp_frame, (const ULWord*)_zeroes.data(), frame_size), true);

        CHECK_EQ(src_router.ApplyRouting(_clear_routing, true), AJA_STATUS_SUCCESS);
        CHECK_EQ(dst_router.ApplyRouting(_clear_routing && !use_single_card, true), AJA_STATUS_SUCCESS);
        AJATime::Sleep(2000);
        // if (!is_routed) {
        //     is_routed = true;
        // }

        //TODO(paulh): If we use random numbers, we need to massage the values into SDI legal values first
        // or the SDI readback will not compare to the original.
        // qa::RandomPattern<ULWord>(0xDECAFBAD, 0x1337c0d3, 0xdecafbad, buffer, qa::ByteOrder::LittleEndian);
        // aja::AlternatingPattern<ULWord>(buffer, frame_size, { 0xdecafbad, 0x1337c0d3 }, 1, qa::ByteOrder::LittleEndian);
        auto stride = fd.linePitch;
        if (NTV2_IS_FBF_8BIT(pf) || pf == NTV2_FBF_24BIT_RGB || pf == NTV2_FBF_24BIT_BGR) {
            uint8_t sdi_range_min = (uint8_t)(qa::LegalVideoValue<8>::GetSDIRangeMin());
            uint8_t sdi_range_max = (uint8_t)(qa::LegalVideoValue<8>::GetSDIRangeMax());
            qa::CountingPattern8Bit(_buffer, fd.GetRasterHeight(), stride, sdi_range_min, sdi_range_max);
        } else if (NTV2_IS_FBF_10BIT(pf)) {
            uint16_t sdi_range_min = (uint16_t)(qa::LegalVideoValue<10>::GetSDIRangeMin());
            uint16_t sdi_range_max = (uint16_t)(qa::LegalVideoValue<10>::GetSDIRangeMax());
            if (pf == NTV2_FBF_10BIT_DPX) {
                qa::CountingPattern10BitDPX(_buffer,
                    fd.GetRasterHeight(), stride,
                    sdi_range_min, sdi_range_max,
                    qa::ByteOrder::BigEndian);
            } else if (pf == NTV2_FBF_10BIT_DPX_LE) {
                qa::CountingPattern10BitDPX(_buffer,
                    fd.GetRasterHeight(), stride,
                    sdi_range_min, sdi_range_max,
                    qa::ByteOrder::LittleEndian);
            } else {
                qa::CountingPattern10Bit(_buffer,
                    fd.GetRasterHeight(), stride,
                    sdi_range_min, sdi_range_max,
                    qa::ByteOrder::LittleEndian);
            }
        } else if (NTV2_IS_FBF_12BIT_RGB(pf)) {
            uint16_t sdi_range_min = (uint16_t)(qa::LegalVideoValue<12>::GetSDIRangeMin());
            uint16_t sdi_range_max = (uint16_t)(qa::LegalVideoValue<12>::GetSDIRangeMax());
            qa::CountingPatternRGB48(_buffer,
                fd.GetRasterHeight(), fd.GetBytesPerRow(),
                sdi_range_min, sdi_range_max,
                qa::ByteOrder::LittleEndian);
        }

        // qa::RandomPattern<ULWord>(readback, frame_size, 0xAB4D533D, 0, UINT32_MAX, qa::ByteOrder::LittleEndian);

        CHECK_EQ(_src_card->SetMode(src_cfg.base.channel, NTV2_MODE_DISPLAY), true);
        CHECK_EQ(_src_card->SetOutputFrame(src_cfg.base.channel, gOptions->out_frame), true);
        CHECK_EQ(_src_card->DMAWriteFrame(gOptions->out_frame, (const ULWord*)_buffer.data(), frame_size), true);
        AJATime::Sleep(1000); // TODO (paulh): Maybe we need to build a histogram of results and check for consistency over multiple readbacks?
        size_t raster_bytes = fd.GetTotalBytes();
        CHECK_EQ(_dst_card->SetMode(dst_cfg.base.channel, NTV2_MODE_CAPTURE, false), true);
        CHECK_EQ(_dst_card->SetInputFrame(dst_cfg.base.channel, gOptions->inp_frame), true);
        CHECK_EQ(_dst_card->DMAReadFrame(gOptions->inp_frame, (ULWord*)&_readback[0], (ULWord)raster_bytes), true);

        NTV2_POINTER master_bytes(_buffer.data(), raster_bytes);
        NTV2_POINTER compare_bytes(_readback.data(), raster_bytes);
        CNTV2PixelComponentReader master(master_bytes, fd);
        CNTV2PixelComponentReader compare(compare_bytes, fd);
        uint64_t rc_start = AJATime::GetSystemMilliseconds();
        CHECK_EQ(master.ReadComponentValues(), AJA_STATUS_SUCCESS);
        CHECK_EQ(compare.ReadComponentValues(), AJA_STATUS_SUCCESS);
        uint64_t rc_end = AJATime::GetSystemMilliseconds() - rc_start;
        LOGINFO("CNTV2PixelComponentReader::ReadComponents finished in " << rc_end << "ms");
        bool result = (master == compare);
        if (result == true) {
            _num_pass++;
        } else {
            _num_fail++;
            _failed_tests.push_back(tc.id);
        }
        _num_done++;
        CHECK_EQ(result, true);
        uint64_t tc_end = AJATime::GetSystemMilliseconds() - tc_start;
        LOGINFO("FramestoreSDI::ExecuteTest finished in " << tc_end << "ms");
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
static FramestoreSDIPtr fs_sdi = nullptr;

void ntv2card_framestore_sdi_marker() {}
TEST_SUITE("framestore_sdi" * doctest::description("SDI loopback tests")) {
    TEST_CASE("framestore_sdi_base") {
        static std::vector<TestCase> test_cases;
        if (!fs_sdi) {
            CNTV2DeviceScanner scanner;
            const size_t num_devices = scanner.GetNumDevices();
            if (gOptions->card_a_index != gOptions->card_b_index) {
                if (num_devices < 2)
                    LOGERR("SDI board-to-board test requires 2 devices. Is the NTV2 driver loaded?");
                REQUIRE_GE(num_devices, 2);
            } else {
                if (num_devices < 1)
                    LOGERR("SDI loopback tests require at least one device. Is the NTV2 driver loaded?");
                REQUIRE_GE(num_devices, 1);
            }
            fs_sdi = aja::make_unique<FramestoreSDI>(
                gOptions->card_a_index,
                gOptions->card_b_index);
        }
        if (fs_sdi) {
            std::string exe_path, exe_dir;
            REQUIRE_EQ(AJAFileIO::GetExecutablePath(exe_path), AJA_STATUS_SUCCESS);
            REQUIRE_EQ(AJAFileIO::GetDirectoryName(exe_path, exe_dir), AJA_STATUS_SUCCESS);
            const std::string json_base_dir = exe_dir + AJA_PATHSEP + "json" + AJA_PATHSEP;
            NTV2StringList vpid_json_paths;
            vpid_json_paths.push_back(json_base_dir + "sdi_validated.json");            // VALIDATED TESTS
            // ^^^^^ WORKING/TBD vvvvv
            // vpid_json_paths.push_back(json_base_dir + "sdi_sd_hd.json");             // SD/HD TESTS
            // vpid_json_paths.push_back(json_base_dir + "sdi_uhd_4k.json");            // UHD/4K TESTS
            // vpid_json_paths.push_back(json_base_dir + "ntv2_vpid_tests.json");       // ALL TEST CASES
            REQUIRE_EQ(fs_sdi->Initialize(vpid_json_paths, test_cases), AJA_STATUS_SUCCESS);
        }
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
    const char* tc_ids = NULL;
    struct argparse_option options[] = {
        OPT_ARGPARSE_HELP(),
        OPT_GROUP("ut_ntv2card options"),
        OPT_INTEGER('a', "card", &gOptions->card_a_index, "card_a"),
        OPT_INTEGER('b', "card_b", &gOptions->card_b_index, "card_b"),
        OPT_INTEGER('\0', "out_channel", &gOptions->out_channel, "output channel for board-to-board/loopback testing"),
        OPT_INTEGER('\0', "inp_channel", &gOptions->inp_channel, "input channel for board-to-board/loopback testing"),
        OPT_INTEGER('\0', "out_framestore", &gOptions->out_framestore, "output framestore for board-to-board/loopback testing"),
        OPT_INTEGER('\0', "inp_framestore", &gOptions->inp_framestore, "input framestore for board-to-board/loopback testing"),
        OPT_INTEGER('\0', "out_frame", &gOptions->out_frame, "output frame number for board-to-board/loopback testing"),
        OPT_INTEGER('\0', "inp_frame", &gOptions->inp_frame, "input frame number for board-to-board/loopback testing"),
        OPT_STRING('\0', "tc_ids", (void*)&tc_ids, "over-ride one test case id for debugging"),
        OPT_END(),
    };
    struct argparse argparse;
    argparse_init(&argparse, options, usage, ARGPARSE_IGNORE_UNKNOWN_ARGS);
    argparse_describe(&argparse, "\nntv2 card unit tests",
        "\nPerform CNTV2Card tests against physical hardware.");
    argparse_parse(&argparse, argc, (const char**)argv);

    if (tc_ids != NULL)
        gOptions->tc_ids = std::string(tc_ids);

    gCard = new CNTV2Card(gOptions->card_a_index, "");
    if (gOptions->card_a_index == gOptions->card_b_index)
        gCard2 = gCard;
    else
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
