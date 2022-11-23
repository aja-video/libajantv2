/* References:
 * https://www.sandordargo.com/blog/2019/04/24/parameterized-testing-with-gtest
 * https://github.com/google/googletest/blob/main/docs/advanced.md#value-parameterized-tests
 */

#include <chrono>
#include <functional>
#include <future>
#include <iomanip>
#include <stdio.h>
#include <stdlib.h>
#include <thread>

#ifdef USE_WFM
#include <libsnmp.h>
#include "snmp_pp/snmp_pp.h"
#include "snmp_pp/auth_priv.h"
using namespace Snmp_pp;
#endif

#include "gtest/gtest.h"

#include "test_support.hpp"

#include "ajabase/common/commandline.h"
#include "ajabase/common/common.h"
#include "ajabase/system/debug.h"
#include "ajabase/system/file_io.h"
#include "ajabase/system/make_unique_shim.h"
#include "ajabase/system/systemtime.h"
#include "ajaconv/ajaconv.h"
#include "ajantv2/includes/ntv2card.h"
#include "ajantv2/utilityfiles/public/ntv2pixelcomponent.h"
#include "ajantv2/includes/ntv2devicefeatures.h"
#include "ajantv2/includes/ntv2devicescanner.h"

#include "ntv2qa/card_config.hpp"
#include "ntv2qa/test_pattern.h"
#include "ntv2qa/time_convert.h"
#include "ntv2qa/routing/preset_router.h"
#include "ntv2qa/video.h"
#include "ntv2qa/vpid.hpp"

#define LOG_QUIET 1
#define AJA_PRINT_REPORT(unit,severity,msg) do {\
    std::ostringstream	oss; oss << msg;\
    std::cout << "(" << AJADebug::SeverityName(severity) << ") " << oss.str() << std::endl;\
    std::ostringstream oss2; oss2 << msg;\
    AJADebug::Report(unit, severity, __FILE__, __LINE__, oss2.str());\
} while (false)
#define AJA_ERR(msg)  AJA_PRINT_REPORT(AJA_DebugUnit_Testing, AJA_DebugSeverity_Error, msg)
#define AJA_WARN(msg) AJA_PRINT_REPORT(AJA_DebugUnit_Testing, AJA_DebugSeverity_Warning, msg)
#define AJA_NOTE(msg) AJA_PRINT_REPORT(AJA_DebugUnit_Testing, AJA_DebugSeverity_Notice, msg)
#define AJA_INFO(msg) AJA_PRINT_REPORT(AJA_DebugUnit_Testing, AJA_DebugSeverity_Info, msg)
#define AJA_DBG(msg)  AJA_PRINT_REPORT(AJA_DebugUnit_Testing, AJA_DebugSeverity_Debug, msg)

namespace {

struct CardTestConfig {
    std::string jsonPath{""};
    std::string testCaseIds{""};
    ULWord srcCardIndex {0};
    ULWord dstCardIndex {0};
    ULWord channelIndexSrc {0};
    ULWord channelIndexDst {0};
    ULWord framestoreIndexSrc {0};
    ULWord framestoreIndexDst {0};
    ULWord frameIndexSrc {0};
    ULWord frameIndexDst {0};
    LWord refSrcIndexDst {-1};
    LWord dstPcbVer {-1};
    bool clearRouting {false};
    bool forceRouting {false};
    bool verbose {false};
    bool savePng {false};
    bool dryRun {false};
    bool runTests {false};
};

struct GenlockBits {
    bool lmh1983 {false};
    bool silabsRenesas {false};
    bool refPresent {false};
};

static CardTestConfig* gTestCfg = nullptr;

class CardManager {
public:
    CardManager(const CardTestConfig &cfg)
        : testCfg_{cfg}, srcCard_{nullptr}, dstCard_{nullptr}
    {
        srcCard_ = new CNTV2Card(cfg.srcCardIndex);
        dstCard_ = new CNTV2Card(cfg.dstCardIndex);
    }
    ~CardManager() {
        delete srcCard_;
        delete dstCard_;
    }
    bool InitializeCards() {
        if (srcCard_->GetSerialNumberString(srcCardSerial_) &&
            dstCard_->GetSerialNumberString(dstCardSerial_)) {
            bool useSingleCard = CardsEqual(srcCard_, dstCard_);
            bool ok = setCardDefaults(srcCard_, testCfg_.clearRouting);
            if (!useSingleCard && ok) {
                ok = setCardDefaults(dstCard_, testCfg_.clearRouting);
            }
            return ok;
        }
        return false;
    }
    static void PrintRoutingString(CNTV2Card *card) {
        std::ostringstream oss;
        CNTV2SignalRouter router;
        card->GetRouting(router);
        router.Print(oss, false);
        std::cout << oss.str() << std::endl;
    }
    static bool CardsEqual(CNTV2Card *src, CNTV2Card *dst) {
        NTV2DeviceID srcId = src->GetDeviceID();
        NTV2DeviceID dstId = dst->GetDeviceID();
        std::string srcSerial;
        std::string dstSerial;
        src->GetSerialNumberString(srcSerial);
        dst->GetSerialNumberString(dstSerial);
        return (srcId != DEVICE_ID_NOTFOUND && dstId != DEVICE_ID_NOTFOUND &&
            srcId == dstId && srcSerial == dstSerial);
    }
    static bool ReadPCBVersion(CNTV2Card *card, ULWord &ver) {
        /* I’ve got the new Kona-5 working with either SiLabs or Renesas using my new bitfile.
         * I’ll add the PCB version at register 63 bits [31:28] = PCB_REV[3:0].
         * Register 63 has CPLD version at [3:0], and it seems like a good place to stick the PCB version.
         * The SiLabs boards will readback version 0x0, 0x1 or 0x2 and the Renesas boards will be version 0xA.
         */
        bool res = false;
        if (card)
            res = card->ReadRegister(kRegCPLDVersion, ver, 0xF0000000, 28);
        return res;
    }
    static bool ReadVpidData(CNTV2Card *card, NTV2Channel channel, qa::VPIDData &vpids)
    {
        bool readOk = false;
        if (card) {
            ULWord vpidARaw = 0;
            ULWord vpidBRaw = 0;
            readOk = card->ReadSDIInVPID(channel, vpidARaw, vpidBRaw);
            vpids.SetA(vpidARaw);
            vpids.SetB(vpidBRaw);
            vpids.Parse();
            CNTV2VPID vpidA(vpidARaw);
            CNTV2VPID vpidB(vpidBRaw);
            std::string vpidAString, vpidBString;
            if (vpidARaw != 0)
                vpidAString = "\n" + vpidA.AsString(true);
            else
                vpidAString = "Not Present";
            if (vpidBRaw != 0)
                vpidBString = "\n" + vpidB.AsString(true);
            else
                vpidBString = "Not Present";
            AJA_INFO("VPID A: " << vpidAString);
            AJA_INFO("VPID B: " << vpidBString);
        }
        return readOk;
    }

    CNTV2Card* GetSrcCard() { return srcCard_; }
    CNTV2Card* GetDstCard() { return dstCard_; }
    std::string GetSrcSerial() const { return srcCardSerial_; }
    std::string GetDstSerial() const { return dstCardSerial_; }
    const CardTestConfig& GetTestConfig() const { return testCfg_; }

private:
    static bool setCardDefaults(CNTV2Card *card, bool clearRouting=false) {
        if (card) {
            NTV2DeviceID id = card->GetDeviceID();
            if (clearRouting) {
                EXPECT_TRUE(card->ClearRouting());
            }
            for (ULWord i = 0; i < NTV2DeviceGetNumVideoChannels(id); i++) {
                NTV2Channel ch = GetNTV2ChannelForIndex(i);
                NTV2AudioSystem audioSys = NTV2ChannelToAudioSystem(ch);
                EXPECT_TRUE(card->SetOutputFrame(ch, ch));
                EXPECT_TRUE(card->SetInputFrame(ch, ch));
                EXPECT_TRUE(card->SetEveryFrameServices(NTV2_OEM_TASKS));
                EXPECT_TRUE(card->SetReference(NTV2_REFERENCE_FREERUN));
                EXPECT_TRUE(card->StopAudioInput(audioSys));
                EXPECT_TRUE(card->StopAudioOutput(audioSys));
                EXPECT_TRUE(card->AutoCirculateStop(ch));
            }
            return true;
        }
        return false;
    }

    CardTestConfig testCfg_;
    CNTV2Card* srcCard_;
    CNTV2Card* dstCard_;
    std::string srcCardSerial_;
    std::string dstCardSerial_;
};

struct CardTestCase {
    int id;
    NTV2VideoFormat vf;
    NTV2PixelFormat pf;
    NTV2VANCMode vancMode;
    ReferenceSelect refSelect;
    VPIDStandard standard;
    uint8_t vpidByte1;
    uint8_t vpidByte2;
    uint8_t vpidByte3;
    uint8_t vpidByte4ADS1;
    uint8_t vpidByte4ADS2;
    uint8_t vpidByte4BDS1;
    uint8_t vpidByte4BDS2;
    uint8_t vpidByte4CDS1;
    uint8_t vpidByte4CDS2;
    uint8_t vpidByte4DDS1;
    uint8_t vpidByte4DDS2;
};

class CardTest : public testing::TestWithParam<CardTestCase> {
public:
    static std::string makeTestCaseName(const CardTestCase &tc) {
        std::string vfStr = NTV2VideoFormatToString(tc.vf);
        std::string pfStr = NTV2FrameBufferFormatToString(tc.pf, true);
        NTV2Standard vidStd = GetNTV2StandardFromVideoFormat(tc.vf);
        std::string defStr;
        if (NTV2_IS_SD_VIDEO_FORMAT(tc.vf)) {
            defStr = "sd";
        } else if (NTV2_IS_2K_VIDEO_FORMAT(tc.vf)) {
            defStr = "2k";
        } else if (NTV2_IS_2K_1080_VIDEO_FORMAT(tc.vf)) {
            defStr = "hd_2k";
        } else if (NTV2_IS_HD_VIDEO_FORMAT(tc.vf)) {
            defStr = "hd";
        } else if (NTV2_IS_4K_VIDEO_FORMAT(tc.vf)) {
            if (NTV2_IS_UHD_STANDARD(vidStd))
                defStr = "uhd";
            else
                defStr = "4k";
        } else if (NTV2_IS_8K_VIDEO_FORMAT(tc.vf)) {
            if (NTV2_IS_UHD2_STANDARD(vidStd))
                defStr = "uhd2";
            else
                defStr = "8k";
        }
        std::ostringstream oss;
        oss << "id_" << tc.id << "_" << defStr
            << "_0x" << std::hex << tc.standard
            << "_" << vfStr << "_" << pfStr;
        std::string s(oss.str());
        return s;
    }
    static std::string makeTestCaseFilename(const CardTestCase &tc) {
        std::string tcName = makeTestCaseName(tc);
        return aja::replace(tcName, ".", "");
    }
    struct PrintToStringParamName {
        template <class ParamType>
        std::string operator()( const testing::TestParamInfo<ParamType>& info ) const
        {
            // NOTE(paulh): gtest only allows alphanumeric characters for test names
            // This is apparently due to gtest converting the names into code under the hood.
            std::string tcName = makeTestCaseName(static_cast<CardTestCase>(info.param));
            return aja::replace(aja::replace(tcName,
                "-", ""), ".", "");
            // return aja::replace(aja::replace(aja::replace(tcName,
            //     "_", ""), "-", ""), ".", "");
        }
    };
    static CardTestCase TestCaseFromJson(const nlohmann::json &j) {
        auto tcId = j["id"].get<int>();
        auto vf = (NTV2VideoFormat)j["vid_fmt_value"].get<int>();
        auto pf = (NTV2PixelFormat)j["pix_fmt_value"].get<int>();
        auto vpidStandardString = j["smpte_id"].get<std::string>();
        auto vpidStandard = (VPIDStandard)std::stoul(
            vpidStandardString, nullptr, 16);
        CardTestCase tc;
        tc.id = tcId;
        tc.vf = vf;
        tc.pf = pf;
        tc.vancMode = NTV2_VANCMODE_OFF;
        tc.refSelect = kVideoIn;
        tc.standard = vpidStandard;
        tc.vpidByte1 = j["byte_1"];
        tc.vpidByte2 = j["byte_2"];
        tc.vpidByte3 = j["byte_3"];
        tc.vpidByte4ADS1 = j["byte_4a_ds1"];
        tc.vpidByte4ADS2 = j["byte_4a_ds2"];
        tc.vpidByte4BDS1 = j["byte_4b_ds1"];
        tc.vpidByte4BDS2 = j["byte_4b_ds2"];
        tc.vpidByte4CDS1 = j["byte_4c_ds1"];
        tc.vpidByte4CDS2 = j["byte_4c_ds2"];
        tc.vpidByte4DDS1 = j["byte_4d_ds1"];
        tc.vpidByte4DDS2 = j["byte_4d_ds2"];
        return tc;
    }

    static std::vector<CardTestCase> ReadTestCases() {
        std::vector<CardTestCase> testCases;
        if (gTestCfg->runTests && !gTestCfg->jsonPath.empty()) {
            jsonData_ = new nlohmann::json;
            bool loaded = CardTest::loadJsonData(gTestCfg->jsonPath);
            EXPECT_TRUE(loaded);
            if (!loaded) {
                delete jsonData_;
                jsonData_ = nullptr;
                return testCases;
            }

            AJA_INFO("Successfully loaded test case JSON file: " << gTestCfg->jsonPath);

            // Optional test case filter arg
            std::vector<int> tcFilter;
            if (!gTestCfg->testCaseIds.empty()) {
                AJA_INFO("User specified test case(s) override: " << gTestCfg->testCaseIds);
                AJAStringList tcIdStrings;
                aja::split(gTestCfg->testCaseIds, ',', tcIdStrings);
                for (auto &&id : tcIdStrings) {
                    tcFilter.push_back(std::stoi(id));
                }
            }

            // Load test cases from JSON
            nlohmann::json &jsonData = *jsonData_;
            for (auto &&jd : jsonData["vpid_tests"]) {
                auto tcId = jd["id"].get<int>();
                // optionally filter test cases based on args
                if (!tcFilter.empty()) {
                    bool addTestCase = false;
                    for (const auto &id : tcFilter) {
                        if (tcId == id) {
                            addTestCase = true;
                            break;
                        }
                    }
                    if (!addTestCase) {
                        AJA_INFO("- Skipping Framestore SDI test case ID: " << tcId);
                        continue;
                    }
                }
                testCases.push_back(TestCaseFromJson(jd));
                if (gTestCfg->verbose)
                    AJA_INFO("- Added Framestore SDI test case ID: " << tcId);
            }
        }

        return testCases;
    }

    static void ReadGenlockStatus(CNTV2Card *card, uint32_t timeoutSec, GenlockBits &bits) {
        ULWord regValue = 0;
        uint64_t startTime = AJATime::GetSystemMilliseconds();
        uint64_t endTime = startTime + (timeoutSec * 1000);
	uint64_t regTime = 0;
	uint64_t regFlipTime = 0;
        while (AJATime::GetSystemMilliseconds() < endTime) {
            regValue = 0;
            if (card->ReadRegister(kRegDMAControl, regValue)) {
                bits.lmh1983 = regValue & BIT(29);
                bits.refPresent = regValue & BIT(30);
                bits.silabsRenesas = regValue & BIT(31);
            }
	    if (bits.silabsRenesas == 0 && regTime == 0)
	        regTime = AJATime::GetSystemNanoseconds();
	    if (bits.silabsRenesas == 1 && regFlipTime == 0 && regTime != 0)
		regFlipTime = AJATime::GetSystemNanoseconds();
            if (gTestCfg->verbose) {
                AJA_INFO("[Genlock] " << "LMH1983=" << (int)bits.lmh1983
                    << " SiLabs/Renesas=" << (int)bits.silabsRenesas
                    << " Ref Present=" << (int)bits.refPresent
                    << " " << regValue << " "
                    << std::hex << regValue
                    << " " << std::bitset<32>(regValue));
            }
        }
	AJA_INFO("Silabs/Renesas bit flip time: " << regFlipTime - regTime << "ns (" << nsec_to_msec(regFlipTime - regTime) << "ms)");
    }

protected:
    static bool loadJsonData(const std::string &path) {
        if (!jsonData_) {
            return false;
        }
        if (!AJAFileIO::FileExists(path)) {
            AJA_ERR("Test case json file not found: " << path);
            return false;
        }
        if (!TestSupport::ReadJsonFile(path, *jsonData_)) {
            AJA_ERR("Error reading test cases from json file: " << path);
            return false;
        }
        return true;
    }

    static bool savePng(const std::string &filename, const NTV2FormatDesc &fd,
                        const std::vector<uint8_t> &buffer) {
        if (buffer.size() > 0 && buffer.size() == fd.GetTotalBytes()) {
            AJACONV_Scan convFunc = NULL;
            uint32_t pngBpp = 24;
            NTV2PixelFormat pngFmt = NTV2_FBF_24BIT_BGR;
            if (fd.GetPixelFormat() == NTV2_FBF_8BIT_YCBCR) {
                convFunc = AJACONV_YCbYCr8_709_To_BGR8;
            } else if (fd.GetPixelFormat() == NTV2_FBF_10BIT_YCBCR) {
                convFunc = AJACONV_v210_709_To_BGR8;
            } else if (fd.GetPixelFormat() == NTV2_FBF_10BIT_RGB) {
                convFunc = AJACONV_RGB10_To_BGRA8;
                pngBpp = 32;
                pngFmt = NTV2_FBF_RGBA;
            } else if (fd.GetPixelFormat() == NTV2_FBF_48BIT_RGB) {
                convFunc = AJACONV_RGB48_LE_To_BGR8;
            }
            if (convFunc != NULL) {
                NTV2FormatDesc fdPng(fd.GetVideoFormat(), pngFmt);
                std::vector<uint8_t> pngBuffer;
                pngBuffer.resize(fdPng.GetTotalBytes());
                AJACONV_Frame(convFunc,
                    (void*)pngBuffer.data(), fdPng.GetBytesPerRow(),
                    (const void*)buffer.data(), fd.GetBytesPerRow(),
                    fd.GetRasterWidth(), fd.GetRasterHeight());
                return TestSupport::SavePNG(filename, pngBuffer.data(),
                    fdPng.GetRasterWidth(), fdPng.GetRasterHeight(), pngBpp);
            }
        }
        return false;
    }

    static bool canDoTest(NTV2DeviceID srcID, NTV2DeviceID dstID,
                   NTV2VideoFormat vf, NTV2PixelFormat pf) {
        if (!NTV2DeviceCanDoVideoFormat(srcID, vf)) {
            AJA_WARN("Video format " << NTV2VideoFormatToString(vf)
                << " not supported by card " << NTV2DeviceIDToString(srcID));
            return false;
        }
        if (!NTV2DeviceCanDoFrameBufferFormat(srcID, pf)) {
            AJA_WARN("Pixel format " << NTV2FrameBufferFormatToString(pf)
                << " not supported by card " << NTV2DeviceIDToString(srcID));
            return false;
        }
        if (!NTV2DeviceCanDoVideoFormat(dstID, vf)) {
            AJA_WARN("Video format " << NTV2VideoFormatToString(vf)
                << " not supported by card " << NTV2DeviceIDToString(dstID));
            return false;
        }
        if (!NTV2DeviceCanDoFrameBufferFormat(dstID, pf)) {
            AJA_WARN("Pixel format " << NTV2FrameBufferFormatToString(pf)
                << " not supported by card " << NTV2DeviceIDToString(dstID));
            return false;
        }
        return true;
    }

    static void logSkipTest(const CardTestCase &tc) {
        std::string vfString = NTV2VideoFormatToString(tc.vf);
        std::string pfString = NTV2FrameBufferFormatToString(tc.pf, true);
        std::stringstream standardStream;
        standardStream << std::setfill ('0') << std::setw(sizeof(uint8_t)*2)
            << std::hex << tc.standard;
        AJA_INFO("Skipping test case ID: " << tc.id << " ("
            << "0x" << standardStream.str() << " " << vfString << " " << pfString << ")");
    }

    static void makeVpidString(const CardTestCase &p, std::ostringstream &oss) {
        ULWord vpid1 = TestSupport::MakeVPIDWord(p.vpidByte1, p.vpidByte2, p.vpidByte3, p.vpidByte4ADS1);
        ULWord vpid2 = 0;
        ULWord vpid3 = 0;
        ULWord vpid4 = 0;
        ULWord vpid5 = 0;
        ULWord vpid6 = 0;
        ULWord vpid7 = 0;
        ULWord vpid8 = 0;
        oss << "VPID A DS1: 0x" << std::hex << vpid1;
        if (p.vpidByte4ADS2) {
            vpid2 = TestSupport::MakeVPIDWord(p.vpidByte1,
                p.vpidByte2, p.vpidByte3, p.vpidByte4ADS2);
            oss << "\nVPID A DS2: 0x" << std::hex << vpid2;
        }
        if (p.vpidByte4BDS1) {
            vpid3 = TestSupport::MakeVPIDWord(p.vpidByte1,
                p.vpidByte2, p.vpidByte3, p.vpidByte4BDS1);
            oss << "\nVPID B DS1: 0x" << std::hex << vpid3;
        }
        if (p.vpidByte4BDS2) {
            vpid4 = TestSupport::MakeVPIDWord(p.vpidByte1,
                p.vpidByte2, p.vpidByte3, p.vpidByte4BDS2);
            oss << "\nVPID B DS2: 0x" << std::hex << vpid4;
        }
        if (p.vpidByte4CDS1) {
            vpid5 = TestSupport::MakeVPIDWord(p.vpidByte1,
                p.vpidByte2, p.vpidByte3, p.vpidByte4CDS1);
            oss << "\nVPID C DS1: 0x" << std::hex << vpid5;
        }
        if (p.vpidByte4CDS2) {
            vpid6 = TestSupport::MakeVPIDWord(p.vpidByte1,
                p.vpidByte2, p.vpidByte3, p.vpidByte4CDS2);
            oss << "\nVPID C DS2: 0x" << std::hex << vpid6;
        }
        if (p.vpidByte4DDS1) {
            vpid7 = TestSupport::MakeVPIDWord(p.vpidByte1,
                p.vpidByte2, p.vpidByte3, p.vpidByte4DDS1);
            oss << "\nVPID D DS1: 0x" << std::hex << vpid7;
        }
        if (p.vpidByte4DDS2) {
            vpid8 = TestSupport::MakeVPIDWord(p.vpidByte1,
                p.vpidByte2, p.vpidByte3, p.vpidByte4DDS2);
            oss << "\nVPID D DS2: 0x" << std::hex << vpid8;
        }
        oss << "\n";
    }

    bool generateTestPattern(const NTV2FormatDesc &fd) {
        auto stride = fd.linePitch;
        NTV2PixelFormat pf = fd.GetPixelFormat();
        if (NTV2_IS_FBF_8BIT(pf) || pf == NTV2_FBF_24BIT_RGB || pf == NTV2_FBF_24BIT_BGR) {
            uint8_t sdiRangeMin = static_cast<uint8_t>(qa::LegalVideoValue<8>::SDIRangeMin());
            uint8_t sdiRangeMax = static_cast<uint8_t>(qa::LegalVideoValue<8>::SDIRangeMax());
            qa::CountingPattern8Bit(patternBuffer_, fd.GetRasterHeight(), stride, sdiRangeMin, sdiRangeMax);
        } else if (NTV2_IS_FBF_10BIT(pf)) {
            uint16_t sdiRangeMin = static_cast<uint16_t>(qa::LegalVideoValue<10>::SDIRangeMin());
            uint16_t sdiRangeMax = static_cast<uint16_t>(qa::LegalVideoValue<10>::SDIRangeMax());
            if (pf == NTV2_FBF_10BIT_DPX) {
                qa::CountingPattern10BitDPX(patternBuffer_,
                    fd.GetRasterHeight(), stride,
                    sdiRangeMin, sdiRangeMax,
                    qa::ByteOrder::BigEndian);
            } else if (pf == NTV2_FBF_10BIT_DPX_LE) {
                qa::CountingPattern10BitDPX(patternBuffer_,
                    fd.GetRasterHeight(), stride,
                    sdiRangeMin, sdiRangeMax,
                    qa::ByteOrder::LittleEndian);
            } else {
                // TODO: Write a true RGB10 counting pattern generator
                qa::CountingPattern10Bit(patternBuffer_,
                    fd.GetRasterHeight(), stride,
                    sdiRangeMin, sdiRangeMax,
                    qa::ByteOrder::LittleEndian);
            }
        } else if (NTV2_IS_FBF_12BIT_RGB(pf)) {
            uint16_t sdiRangeMin = static_cast<uint16_t>(qa::LegalVideoValue<12>::SDIRangeMin());
            uint16_t sdiRangeMax = static_cast<uint16_t>(qa::LegalVideoValue<12>::SDIRangeMax());
            qa::CountingPatternRGB48(patternBuffer_,
                fd.GetRasterHeight(), fd.GetBytesPerRow(),
                sdiRangeMin, sdiRangeMax,
                qa::ByteOrder::LittleEndian);
        } else {
            AJA_ERR("Unsupported test pattern generation format: " << NTV2FrameBufferFormatToString(pf, true));
            return false;
        }
        return true;
    }

    void RunSanityCheck() {
        EXPECT_NE(cardMgr_, nullptr);
        if (cardMgr_ != nullptr) {
            auto cfg = cardMgr_->GetTestConfig();
            auto srcCard = cardMgr_->GetSrcCard();
            EXPECT_NE(srcCard, nullptr);
            auto dstCard = cardMgr_->GetDstCard();
            EXPECT_NE(dstCard, nullptr);
            if (srcCard && dstCard) {
                bool useSingleCard = CardManager::CardsEqual(srcCard, dstCard);
                auto srcCardID = srcCard->GetDeviceID();
                auto dstCardID = dstCard->GetDeviceID();
                ULWord srcCardPcbVer = 0;
                ULWord dstCardPcbVer = 0;
                EXPECT_TRUE(CardManager::ReadPCBVersion(srcCard, srcCardPcbVer));
                EXPECT_TRUE(CardManager::ReadPCBVersion(dstCard, dstCardPcbVer));

                double srcCelsius;
                srcCard->GetDieTemperature(srcCelsius);
                double srcFahrenheit = srcCelsius * 9.0 / 5.0 + 32.0;
                if (useSingleCard) {
                    AJA_INFO("NTV2 SANITY CHECK\n"
                        << "Card: " << cfg.srcCardIndex << " "
                        << NTV2DeviceIDToString(srcCardID) << " - "
                        << cardMgr_->GetSrcSerial()
                        << " PCB Rev: 0x" << std::hex
                        << srcCardPcbVer << "\n"
                        << "Temp: " << srcCelsius << " C (" << srcFahrenheit << " F)\n");
                } else {
                    double dstCelsius;
                    dstCard->GetDieTemperature(dstCelsius);
                    double dstFahrenheit = dstCelsius * 9.0 / 5.0 + 32.0;
                    AJA_INFO("NTV2 SANITY CHECK\n"
                        << "Card (src): " << cfg.srcCardIndex << " "
                            << NTV2DeviceIDToString(srcCardID) << " - "
                            << cardMgr_->GetSrcSerial()
                            << " PCB Rev: 0x" << std::hex
                            << srcCardPcbVer << "\n"
                            << "Temp: " << srcCelsius << " C (" << srcFahrenheit << " F)\n"
                        << "Card (dst): " << cfg.dstCardIndex << " "
                            << NTV2DeviceIDToString(dstCardID) << " - "
                            << cardMgr_->GetDstSerial()
                            << " PCB Rev: 0x" << std::hex
                            << dstCardPcbVer << "\n"
                            << "Temp: " << dstCelsius << " C (" << dstFahrenheit << " F)\n");
                }
            }
        }
    }

    void RunTest() {
        auto p = GetParam(); // The test case data, exposed by sub-classing testing::Test
        auto tc = static_cast<CardTestCase>(p);
        auto srcCard = cardMgr_->GetSrcCard();
        auto dstCard = cardMgr_->GetDstCard();
        bool useSingleCard = CardManager::CardsEqual(srcCard, dstCard);
        auto srcCardID = srcCard->GetDeviceID();
        auto dstCardID = dstCard->GetDeviceID();
        ULWord srcCardPcbVer = 0;
        ULWord dstCardPcbVer = 0;
        EXPECT_TRUE(CardManager::ReadPCBVersion(srcCard, srcCardPcbVer));
        EXPECT_TRUE(CardManager::ReadPCBVersion(dstCard, dstCardPcbVer));
        auto cfg = cardMgr_->GetTestConfig();
        if (cfg.dstPcbVer != -1) {
            std::ostringstream oss;
            oss << std::hex << cfg.dstPcbVer;
            std::string pcbStr = oss.str();
            aja::upper(pcbStr);
            AJA_INFO("Dest Card PCB Override: 0x" << std::hex << pcbStr);
            dstCardPcbVer = gTestCfg->dstPcbVer;
        }
        NTV2Channel srcChannel = GetNTV2ChannelForIndex(cfg.channelIndexSrc);
        NTV2Channel dstChannel = GetNTV2ChannelForIndex(cfg.channelIndexDst);
        NTV2Channel srcFramestore = GetNTV2ChannelForIndex(cfg.framestoreIndexSrc);
        NTV2Channel dstFramestore = GetNTV2ChannelForIndex(cfg.framestoreIndexDst);
        NTV2ReferenceSource srcRefSrc = NTV2_REFERENCE_FREERUN;
        NTV2ReferenceSource dstRefSrc = NTV2_REFERENCE_FREERUN;
        if (gTestCfg->refSrcIndexDst != -1) {
            // Override dst card reference source from args...
            dstRefSrc = NTV2ReferenceSource(gTestCfg->refSrcIndexDst);
        } else {
            // ...otherwise, get dst card reference source from test case.
            if (!useSingleCard) {
                if (p.refSelect == kVideoIn) {
                    dstRefSrc = NTV2InputSourceToReferenceSource(
                        NTV2ChannelToInputSource(dstChannel));
                } else if (p.refSelect == kReferenceIn) {
                    dstRefSrc = NTV2_REFERENCE_EXTERNAL;
                } else if (p.refSelect == kFreeRun) {
                    dstRefSrc = NTV2_REFERENCE_FREERUN;
                }
            }
        }

        std::string vfString = NTV2VideoFormatToString(p.vf);
        std::string pfString = NTV2FrameBufferFormatToString(p.pf, true);
        std::ostringstream standardStream;
        standardStream << std::setfill ('0') << std::setw(sizeof(uint8_t)*2)
            << std::hex << p.standard;
        std::ostringstream vpidStream;
        makeVpidString(p, vpidStream);
        AJA_INFO("[ Framestore SDI Test ]\n\n"
            << "Test case ID: " << p.id << "\n"
            << "Name: " << makeTestCaseName(tc) << "\n"
            << "Standard: 0x" << standardStream.str() << "\n"
            << "Video Format: " << vfString << "\n"
            << "Pixel Format: " << pfString << "\n"
            << "Src Channel: " << NTV2ChannelToString(srcChannel) << "\n"
            << "Dst Channel: " << NTV2ChannelToString(dstChannel) << "\n"
            << "Src Framestore: " << NTV2ChannelToString(srcFramestore) << "\n"
            << "Dst Framestore: " << NTV2ChannelToString(dstFramestore) << "\n"
            << "Src Reference: " << NTV2ReferenceSourceToString(srcRefSrc) << "\n"
            << "Dst Reference: " << NTV2ReferenceSourceToString(dstRefSrc) << "\n"
            << vpidStream.str()
            << std::endl);

        if (!canDoTest(srcCardID, dstCardID, p.vf, p.pf)) {
            logSkipTest(p);
            GTEST_SKIP();
        }
        if (useSingleCard) {
            if (NTV2_IS_QUAD_FRAME_FORMAT(p.vf)) {
                logSkipTest(p);
                GTEST_SKIP();
            }
        }

        qa::NTV2CardConfig srcCardCfg;
        srcCardCfg.base.index = cfg.srcCardIndex;
        srcCardCfg.base.multi_format = true;
        srcCardCfg.base.channel = srcChannel;
        srcCardCfg.base.framestore = srcFramestore;
        srcCardCfg.base.mode = NTV2_MODE_DISPLAY;
        srcCardCfg.video.ntv2_vid_fmt = p.vf;
        srcCardCfg.video.ntv2_pix_fmt = p.pf;
        srcCardCfg.video.ntv2_vanc_mode = p.vancMode;
        srcCardCfg.timing.ref_select = p.refSelect;
        srcCardCfg.video.smpte_standard = p.standard;
        srcCardCfg.base.connection = qa::ConnectionKind::SDI;

        qa::NTV2CardConfig dstCardCfg(srcCardCfg);
        dstCardCfg.base.index = cfg.dstCardIndex;
        dstCardCfg.base.channel = dstChannel;
        dstCardCfg.base.framestore = dstFramestore;
        dstCardCfg.base.mode = NTV2_MODE_CAPTURE;
        srcCard->SetMultiFormatMode(srcCardCfg.base.multi_format);
        dstCard->SetMultiFormatMode(dstCardCfg.base.multi_format);
        srcCard->SetReference(srcRefSrc);
        dstCard->SetReference(dstRefSrc);

        auto srcRouter = qa::QAPresetRouter(srcCardCfg, srcCard);
        auto dstRouter = qa::QAPresetRouter(dstCardCfg, dstCard);

        auto future = std::async(std::launch::async, [dstCard, this]() -> GenlockBits {
            GenlockBits genlockBits;
            genlockBits.lmh1983 = false;
            genlockBits.silabsRenesas = false;
            // NTV2DeviceID devId = dstCard->GetDeviceID();
            // if (NTV2DeviceHasGenlockv3(devId)) {
            if (1) {
                ReadGenlockStatus(dstCard, 5, genlockBits);
            } else {
                std::this_thread::sleep_for(std::chrono::seconds(5)); // wait for signal to lock up
            }
            return genlockBits;
        });

        // Zero out framebuffers on cards.
        // Source card frame is filled with zeroes, while dest card frame is filled with ones.
        // This is so that we don't accidentally read back an identical "zeroed" buffer from both
        // card frames, compare them as the same data, giving an erroenous passing test.
        const NTV2FormatDesc &fd = srcCardCfg.video.GetFormatDesc();
        ULWord frameSize = fd.GetTotalBytes();
        EXPECT_EQ(srcCard->DMAWriteFrame(srcCardCfg.base.channel, (const ULWord*)zeroBuffer_.data(), frameSize), true);
        memset(&zeroBuffer_[0], 1, frameSize);
        EXPECT_EQ(dstCard->DMAWriteFrame(dstCardCfg.base.channel, (const ULWord*)zeroBuffer_.data(), frameSize), true);

        // Apply signal route
        EXPECT_EQ(srcRouter.ApplyRouting(cfg.clearRouting, cfg.forceRouting), AJA_STATUS_SUCCESS);
        EXPECT_EQ(dstRouter.ApplyRouting(cfg.clearRouting && !useSingleCard, cfg.forceRouting), AJA_STATUS_SUCCESS);
        std::cout << "Signal Routing\n" << "[src]" << std::endl;
        CardManager::PrintRoutingString(srcCard);
        std::cout << "[dst]" << std::endl;
        CardManager::PrintRoutingString(dstCard);

        auto genlockBits = future.get(); // block on reading genlock bits
        AJA_INFO("Genlock bits: lmh=" << genlockBits.lmh1983
            << " silabs/renesas=" << genlockBits.silabsRenesas
            << " ref present=" << genlockBits.refPresent);
        EXPECT_TRUE(genlockBits.refPresent) << "Genlock ref not present!";
        if (useSingleCard && dstRefSrc == NTV2_REFERENCE_FREERUN) {
            EXPECT_TRUE(genlockBits.lmh1983);
            EXPECT_TRUE(genlockBits.silabsRenesas);
        } else {
            if (NTV2DeviceHasGenlockv2(dstCardID)) {
		AJA_INFO("Genlock V2");
                EXPECT_FALSE(genlockBits.lmh1983);
                EXPECT_TRUE(genlockBits.silabsRenesas);
            } else if (NTV2DeviceHasGenlockv3(dstCardID)) {
		AJA_INFO("Genlock V3");
                if (TestSupport::HasRenesasGenlock(dstCardID, dstCardPcbVer)) {
                    EXPECT_TRUE(genlockBits.lmh1983);
                    EXPECT_TRUE(genlockBits.silabsRenesas);
                } else {
                    EXPECT_TRUE(genlockBits.lmh1983);
                    if (NTV2_IS_525_FORMAT(p.vf)) {
                        EXPECT_FALSE(genlockBits.silabsRenesas);
                    } else {
                        EXPECT_TRUE(genlockBits.silabsRenesas);
                    }
                }
            }
        }

        // Generate test pattern
        EXPECT_TRUE(generateTestPattern(fd));
        EXPECT_EQ(srcCard->SetMode(srcCardCfg.base.channel, NTV2_MODE_DISPLAY), true);
        // EXPECT_EQ(srcCard->SetOutputFrame(srcCardCfg.base.channel, cfg.frameIndexSrc), true);
        EXPECT_EQ(srcCard->DMAWriteFrame(srcCardCfg.base.channel, (const ULWord*)patternBuffer_.data(), frameSize), true);
        std::this_thread::sleep_for(std::chrono::seconds(1)); // wait for signal to lock up
        size_t rasterBytes = fd.GetTotalBytes();
        EXPECT_EQ(dstCard->SetMode(dstCardCfg.base.channel, NTV2_MODE_CAPTURE, false), true);
        // EXPECT_EQ(dstCard->SetInputFrame(dstCardCfg.base.channel, cfg.frameIndexDst), true);
        EXPECT_EQ(dstCard->DMAReadFrame(dstCardCfg.base.channel, (ULWord*)&readbackBuffer_[0], (ULWord)rasterBytes), true);

        // Dump PNG of test pattern and DMA readback
        if (gTestCfg->savePng) {
            static const std::vector<NTV2PixelFormat> supported = {
                NTV2_FBF_8BIT_YCBCR, NTV2_FBF_10BIT_YCBCR,
                NTV2_FBF_10BIT_RGB, NTV2_FBF_48BIT_RGB };
            bool writePng = false;
            for (const auto& sup : supported) {
                if (fd.GetPixelFormat() == sup) {
                    writePng = true;
                    break;
                }
            }
            if (writePng) {
                std::string tcFilename = makeTestCaseFilename(tc);
                std::string pngPath = tcFilename + "_pattern.png";
                if (savePng(pngPath, fd, patternBuffer_))
                    AJA_INFO("Wrote test pattern PNG: " << pngPath);
                pngPath = tcFilename + "_readback.png";
                if (savePng(pngPath, fd, readbackBuffer_))
                    AJA_INFO("Wrote DMA readback PNG: " << pngPath);
            }
        }

        NTV2_POINTER masterBytes(patternBuffer_.data(), rasterBytes);
        NTV2_POINTER compareBytes(readbackBuffer_.data(), rasterBytes);
        CNTV2PixelComponentReader master(masterBytes, fd);
        CNTV2PixelComponentReader compare(compareBytes, fd);
        uint64_t rcStart = AJATime::GetSystemMilliseconds();
        EXPECT_EQ(master.ReadComponentValues(), AJA_STATUS_SUCCESS);
        EXPECT_EQ(compare.ReadComponentValues(), AJA_STATUS_SUCCESS);
        uint64_t rcEnd = AJATime::GetSystemMilliseconds() - rcStart;
        AJA_INFO("CNTV2PixelComponentReader::ReadComponents finished in " << rcEnd << "ms");
        EXPECT_TRUE(master == compare) << "CNTV2PixelComponentReader compare failed!"
            << "\n\nSource Card: (" << cfg.srcCardIndex << ") - " << NTV2DeviceIDToString(srcCardID) << " - " << cardMgr_->GetSrcSerial() << "\n"
            << "Dest Card: (" << cfg.dstCardIndex << ") - " << NTV2DeviceIDToString(dstCardID) << " - " << cardMgr_->GetDstSerial() << "\n"
            << "Video Format: " << NTV2VideoFormatToString(p.vf) << "\n"
            << "Pixel Format: " << NTV2FrameBufferFormatToString(p.pf, true)
            << std::endl;

        qa::VPIDData vpidData;
        CardManager::ReadVpidData(dstCard, dstChannel, vpidData);
        EXPECT_EQ(tc.standard, vpidData.Standard());
        //TODO: Add helper to qa::PresetRouter to get the number of wires/datastreams from a preset
    }

    void SetUp() override {
        AJA_INFO("CardTest::SetUp");
        bool proceed = true;
        // TODO(paulh): Remove this check when gtest is updated to 1.12.0,
        // so that `SetUpTestSuite` fail conditions work properly.
        if (!gTestCfg->jsonPath.empty()) {
            proceed = (jsonData_ != nullptr);
            EXPECT_TRUE(proceed) << "Error loading JSON test case data from: " << gTestCfg->jsonPath;
        }
        if (proceed) {
            EXPECT_TRUE(cardMgr_->InitializeCards());
            auto p = GetParam();
            NTV2FormatDesc fd(p.vf, p.pf);
            ULWord frameSize = fd.GetTotalBytes();
            patternBuffer_.resize(frameSize);
            readbackBuffer_.resize(frameSize);
            zeroBuffer_.resize(frameSize);
        }
    }

    void TearDown() override {
        AJA_INFO("CardTest::TearDown");
    }

    static void SetUpTestSuite() {
        AJA_INFO("CardTest::SetUpTestSuite");
        /* FIXME(paulh): Failures in SetUpTestSuite to do not induce a complete test run failure
        * until gtest 1.12.0, due to a bug that was fixed in the following gtest commit:
        * https://github.com/google/googletest/commit/0570d97fb684452c96327e5e6e5ae02c14dbca29
        *
        * AJA is using gtest 1.10.0 as long as support for GCC 4.8.5 and CentOS7 are required
        * or used by internal CI build systems. */
        EXPECT_TRUE(gTestCfg != nullptr);
        if (gTestCfg) {
            cardMgr_ = new CardManager(*gTestCfg);
        }
    }

    static void TearDownTestSuite() {
        AJA_INFO("CardTest::TearDownTestSuite");
        if (cardMgr_ != nullptr) {
            delete cardMgr_;
            cardMgr_ = nullptr;
        }
        if (jsonData_ != nullptr) {
            delete jsonData_;
            jsonData_ = nullptr;
        }
    }

    std::vector<UByte> patternBuffer_;
    std::vector<UByte> readbackBuffer_;
    std::vector<UByte> zeroBuffer_;

    uint32_t numTests_;
    uint32_t numTestsDone_;
    uint32_t numTestsPass_;
    uint32_t numTestsFail_;

    static CardManager* cardMgr_;
    static nlohmann::json* jsonData_;
};

CardManager* CardTest::cardMgr_ = nullptr;
nlohmann::json* CardTest::jsonData_ = nullptr;

/* --------------------------------- TESTS --------------------------------- */

TEST_P(CardTest, FramestoreSDI) {
  // Inside a test, access the test parameter with the GetParam() method
  // of the TestWithParam<T> class:
    if (!gTestCfg->dryRun) {
        RunSanityCheck();
        RunTest();
    }
}

INSTANTIATE_TEST_SUITE_P(CardTestInstance,
                         CardTest,
                         testing::ValuesIn(CardTest::ReadTestCases()),
                         CardTest::PrintToStringParamName());

}  // namespace

int main(int argc, char** argv)
{
    AJACommandLineParser parser;
    parser.AddOption(AJACommandLineOption(AJAStringList{ "board", "card", "d", "device" }, "NTV2 Card index (or indices if more than one arg set)"));
    parser.AddOption(AJACommandLineOption(AJAStringList{ "pcb_ver_dst" }, "PCB Version B override value"));
    parser.AddOption(AJACommandLineOption(AJAStringList{ "ch_src" }, "NTV2 Channel Index A"));
    parser.AddOption(AJACommandLineOption(AJAStringList{ "ch_dst" }, "NTV2 Channel Index B"));
    parser.AddOption(AJACommandLineOption(AJAStringList{ "fs_src" }, "NTV2 Framestore Index A"));
    parser.AddOption(AJACommandLineOption(AJAStringList{ "fs_dst" }, "NTV2 Framestore Index B"));
    parser.AddOption(AJACommandLineOption(AJAStringList{ "frm_src" }, "NTV2 Frame Index A"));
    parser.AddOption(AJACommandLineOption(AJAStringList{ "frm_dst" }, "NTV2 Frame Index B"));
    parser.AddOption(AJACommandLineOption(AJAStringList{ "ref_dst" }, "NTV2 Reference Source B"));
    parser.AddOption(AJACommandLineOption(AJAStringList{ "tc_ids" }, "Specify test case IDs to run (comma-separated list)"));
    parser.AddOption(AJACommandLineOption(AJAStringList{ "clear" }, "Clear signal routing on card(s)?"));
    parser.AddOption(AJACommandLineOption(AJAStringList{ "force" }, "Force signal routing on card(s)?"));
    parser.AddOption(AJACommandLineOption(AJAStringList{ "j", "json" }, "Path to JSON file containing test case data"));
    parser.AddOption(AJACommandLineOption(AJAStringList{ "r", "run" }, "Run Tests"));
    parser.AddOption(AJACommandLineOption(AJAStringList{ "dryrun" }, "Perform a dry-run of test case generation only"));
    parser.AddOption(AJACommandLineOption(AJAStringList{ "v", "verbose" }, "Enable verbose logging"));
    parser.AddOption(AJACommandLineOption(AJAStringList{ "w", "wfm" }, "WFM8300 IP Address (for SNMP communication)"));
    parser.AddOption(AJACommandLineOption(AJAStringList{ "p", "png" }, "Save PNG of generated test pattern and DMA readback buffer?"));
    parser.AddHelpOption();
    parser.ParseArgs(argc, &argv[0]);
    // parser.Dump();

#ifdef USE_WFM
    if (parser.IsSet("wfm")) {
        Snmp::socket_startup();
        const std::string &wfmAddress = parser.ValueString("wfm");
        UdpAddress address(wfmAddress.c_str());
        int snmpStatus;
        snmp_version version=version1;                  // default is v1
        int retries=1;                                  // default retries is 1
        int timeout=100;                                // default is 1 second
        u_short port=161;                               // default snmp port is 161
        OctetStr community("public");                   // community name
        Snmp snmp(snmpStatus, 0, false);
        if (snmpStatus != SNMP_CLASS_SUCCESS) {
            std::cout << "(" << wfmAddress
                << ") Error establishing SNMP connection with WFM: "
                << snmp.error_msg(snmpStatus) << std::endl;
            return 1;
        }
        CTarget ctarget(address);
        ctarget.set_version(version);         // set the SNMP version SNMPV1 or V2
        ctarget.set_retry(retries);           // set the number of auto retries
        ctarget.set_timeout(timeout);         // set timeout
        ctarget.set_readcommunity(community); // set the read community name
        SnmpTarget *target = &ctarget;
        Pdu pdu;
        Oid oid("1.3.6.1.4.1.128.5.2.10.2.38");
        Vb vb;
        vb.set_oid(oid);
        pdu += vb;
        snmpStatus = snmp.get(pdu, *target);
        if (snmpStatus == SNMP_CLASS_SUCCESS) {
            for (int i = 0; i < pdu.get_vb_count(); i++) {
                pdu.get_vb(vb, i);
                std::cout << vb.get_printable_value() << std::endl;
            }
        }
    }
#endif

    bool printHelp = parser.IsSet("help");
    bool runTests = parser.IsSet("run");
    if (printHelp) {
        std::cout << parser.GetHelpText();
    }
    if (!runTests) {
        std::cout << "\nNOTE: Run `ntv2card_test` with the `--run` argument to execute tests." << std::endl;
        return 0;
    }

    AJADebug::Open();

    if (!AJACONV_Initialize())
        return 1;

    gTestCfg = new CardTestConfig;
    if (!gTestCfg) {
        AJA_ERR("FATAL: Error instantiating CardTestConfig!");
        return 1;
    }

    CNTV2DeviceScanner scanner;
    if (scanner.GetNumDevices() == 0) {
        AJA_ERR("FATAL: No ntv2 devices found! Is the driver loaded?");
        return 1;
    }

    AJAVariantList cardIndices = parser.Values("card");
    if (cardIndices.size() > 0) {
        gTestCfg->srcCardIndex = cardIndices[0].AsUInt32();
        gTestCfg->dstCardIndex = cardIndices[1].AsUInt32();
    }
    if (parser.IsSet("pcb_ver_dst"))
        gTestCfg->dstPcbVer = parser.Value("pcb_ver_dst").AsInt32();
    gTestCfg->channelIndexSrc = parser.Value("ch_src").AsUInt32();
    gTestCfg->channelIndexDst = parser.Value("ch_dst").AsUInt32();
    gTestCfg->framestoreIndexSrc = parser.Value("fs_src").AsUInt32();
    gTestCfg->framestoreIndexDst = parser.Value("fs_dst").AsUInt32();
    gTestCfg->frameIndexSrc = parser.Value("frm_src").AsUInt32();
    gTestCfg->frameIndexDst = parser.Value("frm_dst").AsUInt32();
    if (parser.IsSet("ref_dst"))
        gTestCfg->refSrcIndexDst = parser.Value("ref_dst").AsUInt32();
    gTestCfg->testCaseIds = parser.Value("tc_ids").AsString();
    gTestCfg->jsonPath = parser.Value("json").AsString();
    gTestCfg->clearRouting = parser.IsSet("clear");
    gTestCfg->forceRouting = true;
    gTestCfg->runTests = runTests && !printHelp;
    gTestCfg->savePng = parser.IsSet("png");
    gTestCfg->dryRun = parser.IsSet("dryrun");
    gTestCfg->verbose = parser.IsSet("verbose");
    if (gTestCfg->dryRun)
        AJA_INFO("DRY RUN (test case generation only)");

    ::testing::InitGoogleTest(&argc, argv);
    int result = RUN_ALL_TESTS();

    delete gTestCfg;
    gTestCfg = nullptr;

    AJADebug::Close();

    return result;
}
