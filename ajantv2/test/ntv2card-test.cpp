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
#include "ajantv2/includes/ntv2testpatterngen.h"
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

enum TestCardType {
    kTestCardSource,
    kTestCardDest,
    kTestCardRef,
};

struct CardTestConfig {
    std::string jsonPath{""};
    std::string testCaseIds{""};
    ULWord deviceIndexSrc {0};
    ULWord deviceIndexDst {0};
    LWord deviceIndexRef {-1};
    ULWord channelIndexSrc {0};
    ULWord channelIndexDst {0};
    ULWord framestoreIndexSrc {0};
    ULWord framestoreIndexDst {0};
    ULWord frameIndexSrc {0};
    ULWord frameIndexDst {0};
    LWord refSourceIndexDst {-1};
    LWord pcbVerDst {-1};
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

class CardManager {
public:
    CardManager(const CardTestConfig &cfg)
        : _testCfg{cfg}, _srcCard{nullptr}, _dstCard{nullptr}, _refCard{nullptr}
    {
        _srcCard = new CNTV2Card(cfg.deviceIndexSrc);
        _dstCard = new CNTV2Card(cfg.deviceIndexDst);
        if (cfg.deviceIndexRef >= 0)
            _refCard = new CNTV2Card(static_cast<UWord>(cfg.deviceIndexRef));
    }
    ~CardManager() {
        if (_srcCard) {
            delete _srcCard;
            _srcCard = nullptr;
        }
        if (_dstCard) {
            delete _dstCard;
            _dstCard = nullptr;
        }
        if (_refCard) {
            delete _refCard;
            _refCard = nullptr;
        }
    }
    bool InitializeCards() {
        if (_srcCard->GetSerialNumberString(_srcCardSerial) &&
            _dstCard->GetSerialNumberString(_dstCardSerial)) {
            bool useSingleCard = CardsEqual(_srcCard, _dstCard);
            bool ok = setCardDefaults(_srcCard, _testCfg.clearRouting);
            if (!useSingleCard && ok) {
                ok = setCardDefaults(_dstCard, _testCfg.clearRouting);
            }
            if (_refCard && ok && _refCard->GetSerialNumberString(_refCardSerial)) {
                ok = setCardDefaults(_refCard, _testCfg.clearRouting);
            }
            return ok;
        }
        return false;
    }

    bool SetCardsReferenceSource(const CardTestCase &tc, NTV2ReferenceSource &srcRefSrc,
        NTV2ReferenceSource &dstRefSrc, NTV2ReferenceSource &refRefSrc) {
        NTV2Channel srcChannel = GetNTV2ChannelForIndex(_testCfg.channelIndexSrc);
        NTV2Channel dstChannel = GetNTV2ChannelForIndex(_testCfg.channelIndexDst);
        NTV2Channel srcFramestore = GetNTV2ChannelForIndex(_testCfg.framestoreIndexSrc);
        NTV2Channel dstFramestore = GetNTV2ChannelForIndex(_testCfg.framestoreIndexDst);
        if (_testCfg.refSourceIndexDst != -1) {
            // Override dst card reference source from args...
            dstRefSrc = NTV2ReferenceSource(_testCfg.refSourceIndexDst);
        } else {
            // ...otherwise, get dst card reference source from test case.
            if (!CardsEqual(_srcCard, _dstCard)) {
                if (tc.refSelect == kVideoIn) {
                    dstRefSrc = NTV2InputSourceToReferenceSource(
                        NTV2ChannelToInputSource(dstChannel));
                } else if (tc.refSelect == kReferenceIn) {
                    srcRefSrc = NTV2_REFERENCE_EXTERNAL;
                    dstRefSrc = NTV2_REFERENCE_EXTERNAL;
                } else if (tc.refSelect == kFreeRun) {
                    srcRefSrc = NTV2_REFERENCE_FREERUN;
                    dstRefSrc = NTV2_REFERENCE_FREERUN;
                }
            }
        }
        if (_refCard) {
            srcRefSrc = NTV2_REFERENCE_EXTERNAL;
            dstRefSrc = NTV2_REFERENCE_EXTERNAL;
        }
        bool setOk = _srcCard->SetReference(srcRefSrc);
        if (setOk)
            setOk = _dstCard->SetReference(dstRefSrc);
        if (_refCard && setOk)
            setOk = _refCard->SetReference(refRefSrc);
        return setOk;
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

    CNTV2Card* GetSrcCard() { return _srcCard; }
    CNTV2Card* GetDstCard() { return _dstCard; }
    CNTV2Card* GetRefCard() { return _refCard; }
    std::string GetSrcSerial() const { return _srcCardSerial; }
    std::string GetDstSerial() const { return _dstCardSerial; }
    std::string GetRefSerial() const { return _refCardSerial; }
    const CardTestConfig& GetTestConfig() const { return _testCfg; }

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

    CardTestConfig _testCfg;
    CNTV2Card* _srcCard;            // outptu (src) device
    CNTV2Card* _dstCard;            // capture (dst) device
    CNTV2Card* _refCard;            // optional device for generating an external reference signal
    std::string _srcCardSerial;
    std::string _dstCardSerial;
    std::string _refCardSerial;
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
        tc.refSelect = j["ref_select"];
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
            _jsonData = new nlohmann::json;
            bool loaded = CardTest::loadJsonData(gTestCfg->jsonPath);
            EXPECT_TRUE(loaded);
            if (!loaded) {
                delete _jsonData;
                _jsonData = nullptr;
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
            nlohmann::json &jsonData = *_jsonData;
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
        if (!_jsonData) {
            return false;
        }
        if (!AJAFileIO::FileExists(path)) {
            AJA_ERR("Test case json file not found: " << path);
            return false;
        }
        if (!TestSupport::ReadJsonFile(path, *_jsonData)) {
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

    static void makeVpidString(const CardTestCase &tc, std::ostringstream &oss) {
        ULWord vpid1 = TestSupport::MakeVPIDWord(tc.vpidByte1, tc.vpidByte2, tc.vpidByte3, tc.vpidByte4ADS1);
        ULWord vpid2 = 0;
        ULWord vpid3 = 0;
        ULWord vpid4 = 0;
        ULWord vpid5 = 0;
        ULWord vpid6 = 0;
        ULWord vpid7 = 0;
        ULWord vpid8 = 0;
        oss << "VPID A DS1: 0x" << std::hex << vpid1;
        if (tc.vpidByte4ADS2) {
            vpid2 = TestSupport::MakeVPIDWord(tc.vpidByte1,
                tc.vpidByte2, tc.vpidByte3, tc.vpidByte4ADS2);
            oss << "\nVPID A DS2: 0x" << std::hex << vpid2;
        }
        if (tc.vpidByte4BDS1) {
            vpid3 = TestSupport::MakeVPIDWord(tc.vpidByte1,
                tc.vpidByte2, tc.vpidByte3, tc.vpidByte4BDS1);
            oss << "\nVPID B DS1: 0x" << std::hex << vpid3;
        }
        if (tc.vpidByte4BDS2) {
            vpid4 = TestSupport::MakeVPIDWord(tc.vpidByte1,
                tc.vpidByte2, tc.vpidByte3, tc.vpidByte4BDS2);
            oss << "\nVPID B DS2: 0x" << std::hex << vpid4;
        }
        if (tc.vpidByte4CDS1) {
            vpid5 = TestSupport::MakeVPIDWord(tc.vpidByte1,
                tc.vpidByte2, tc.vpidByte3, tc.vpidByte4CDS1);
            oss << "\nVPID C DS1: 0x" << std::hex << vpid5;
        }
        if (tc.vpidByte4CDS2) {
            vpid6 = TestSupport::MakeVPIDWord(tc.vpidByte1,
                tc.vpidByte2, tc.vpidByte3, tc.vpidByte4CDS2);
            oss << "\nVPID C DS2: 0x" << std::hex << vpid6;
        }
        if (tc.vpidByte4DDS1) {
            vpid7 = TestSupport::MakeVPIDWord(tc.vpidByte1,
                tc.vpidByte2, tc.vpidByte3, tc.vpidByte4DDS1);
            oss << "\nVPID D DS1: 0x" << std::hex << vpid7;
        }
        if (tc.vpidByte4DDS2) {
            vpid8 = TestSupport::MakeVPIDWord(tc.vpidByte1,
                tc.vpidByte2, tc.vpidByte3, tc.vpidByte4DDS2);
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
            qa::CountingPattern8Bit(_patternBuffer, fd.GetRasterHeight(), stride, sdiRangeMin, sdiRangeMax);
        } else if (NTV2_IS_FBF_10BIT(pf)) {
            uint16_t sdiRangeMin = static_cast<uint16_t>(qa::LegalVideoValue<10>::SDIRangeMin());
            uint16_t sdiRangeMax = static_cast<uint16_t>(qa::LegalVideoValue<10>::SDIRangeMax());
            if (pf == NTV2_FBF_10BIT_DPX) {
                qa::CountingPattern10BitDPX(_patternBuffer,
                    fd.GetRasterHeight(), stride,
                    sdiRangeMin, sdiRangeMax,
                    qa::ByteOrder::BigEndian);
            } else if (pf == NTV2_FBF_10BIT_DPX_LE) {
                qa::CountingPattern10BitDPX(_patternBuffer,
                    fd.GetRasterHeight(), stride,
                    sdiRangeMin, sdiRangeMax,
                    qa::ByteOrder::LittleEndian);
            } else {
                // TODO: Write a true RGB10 counting pattern generator
                qa::CountingPattern10Bit(_patternBuffer,
                    fd.GetRasterHeight(), stride,
                    sdiRangeMin, sdiRangeMax,
                    qa::ByteOrder::LittleEndian);
            }
        } else if (NTV2_IS_FBF_12BIT_RGB(pf)) {
            uint16_t sdiRangeMin = static_cast<uint16_t>(qa::LegalVideoValue<12>::SDIRangeMin());
            uint16_t sdiRangeMax = static_cast<uint16_t>(qa::LegalVideoValue<12>::SDIRangeMax());
            qa::CountingPatternRGB48(_patternBuffer,
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
        EXPECT_NE(_cardMgr, nullptr);
        if (_cardMgr != nullptr) {
            auto cfg = _cardMgr->GetTestConfig();
            auto srcCard = _cardMgr->GetSrcCard();
            EXPECT_NE(srcCard, nullptr);
            auto dstCard = _cardMgr->GetDstCard();
            EXPECT_NE(dstCard, nullptr);
            auto refCard = _cardMgr->GetRefCard();
            if (srcCard && dstCard) {
                bool useSingleCard = CardManager::CardsEqual(srcCard, dstCard);
                auto srcCardID = srcCard->GetDeviceID();
                auto dstCardID = dstCard->GetDeviceID();
                ULWord srcCardPcbVer = 0;
                ULWord dstCardPcbVer = 0;
                EXPECT_TRUE(CardManager::ReadPCBVersion(srcCard, srcCardPcbVer));
                EXPECT_TRUE(CardManager::ReadPCBVersion(dstCard, dstCardPcbVer));

                std::ostringstream oss;
                oss << "NTV2 SANITY CHECK\n";
                double celsius;
                srcCard->GetDieTemperature(celsius);
                double fahrenheit = celsius * 9.0 / 5.0 + 32.0;
                oss << "Card (src): " << cfg.deviceIndexSrc << " "
                    << NTV2DeviceIDToString(srcCardID) << " - "
                    << _cardMgr->GetSrcSerial() << "\n"
                    << "  PCB Rev: 0x" << std::hex
                    << srcCardPcbVer << "\n"
                    << "  Temp: " << celsius << " C (" << fahrenheit << " F)\n";
                if (!useSingleCard) {
                    dstCard->GetDieTemperature(celsius);
                    fahrenheit = celsius * 9.0 / 5.0 + 32.0;
                    oss << "Card (dst): " << cfg.deviceIndexDst << " "
                        << NTV2DeviceIDToString(dstCardID) << " - "
                        << _cardMgr->GetDstSerial() << "\n"
                        << "  PCB Rev: 0x" << std::hex
                        << dstCardPcbVer << "\n"
                        << "  Temp: " << celsius << " C (" << fahrenheit << " F)\n";
                }
                if (refCard) {
                    ULWord refCardPcbVer = 0;
                    EXPECT_TRUE(CardManager::ReadPCBVersion(refCard, refCardPcbVer));
                    refCard->GetDieTemperature(celsius);
                    fahrenheit = celsius * 9.0 / 5.0 + 32.0;
                    oss << "Card (ref): " << cfg.deviceIndexRef << " "
                        << NTV2DeviceIDToString(refCard->GetDeviceID()) << " - "
                        << _cardMgr->GetRefSerial() << "\n"
                        << "  PCB Rev: 0x" << std::hex
                        << refCardPcbVer << "\n"
                        << "  Temp: " << celsius << " C (" << fahrenheit << " F)\n";
                }
                std::string msg = oss.str();
                AJA_INFO(msg);
            }
        }
    }

    void RunTest() {
        auto tc = static_cast<CardTestCase>(GetParam());
        auto srcCard = _cardMgr->GetSrcCard();
        auto dstCard = _cardMgr->GetDstCard();
        auto refCard = _cardMgr->GetRefCard();
        bool useSingleCard = CardManager::CardsEqual(srcCard, dstCard);
        auto srcCardID = srcCard->GetDeviceID();
        auto dstCardID = dstCard->GetDeviceID();
        ULWord srcCardPcbVer = 0;
        ULWord dstCardPcbVer = 0;
        EXPECT_TRUE(CardManager::ReadPCBVersion(srcCard, srcCardPcbVer));
        EXPECT_TRUE(CardManager::ReadPCBVersion(dstCard, dstCardPcbVer));
        auto cfg = _cardMgr->GetTestConfig();
        if (cfg.pcbVerDst != -1) {
            std::ostringstream oss;
            oss << std::hex << cfg.pcbVerDst;
            std::string pcbStr = oss.str();
            aja::upper(pcbStr);
            AJA_INFO("Dest Card PCB Override: 0x" << std::hex << pcbStr);
            dstCardPcbVer = gTestCfg->pcbVerDst;
        }
        NTV2ReferenceSource srcRefSrc = NTV2_REFERENCE_FREERUN;
        NTV2ReferenceSource dstRefSrc = NTV2_REFERENCE_FREERUN;
        NTV2ReferenceSource refRefSrc = NTV2_REFERENCE_FREERUN;
        EXPECT_TRUE(_cardMgr->SetCardsReferenceSource(tc, srcRefSrc, dstRefSrc, refRefSrc));
        NTV2Channel srcChannel = GetNTV2ChannelForIndex(cfg.channelIndexSrc);
        NTV2Channel dstChannel = GetNTV2ChannelForIndex(cfg.channelIndexDst);
        NTV2Channel srcFramestore = GetNTV2ChannelForIndex(cfg.framestoreIndexSrc);
        NTV2Channel dstFramestore = GetNTV2ChannelForIndex(cfg.framestoreIndexDst);
        std::string vfString = NTV2VideoFormatToString(tc.vf);
        std::string pfString = NTV2FrameBufferFormatToString(tc.pf, true);
        std::ostringstream standardStream;
        standardStream << std::setfill ('0') << std::setw(sizeof(uint8_t)*2)
            << std::hex << tc.standard;
        std::ostringstream ossVpid;
        makeVpidString(tc, ossVpid);
        std::ostringstream ossTest;
        ossTest << "[ Framestore SDI Test ]\n\n"
            << "Test case ID: " << tc.id << "\n"
            << "Name: " << makeTestCaseName(tc) << "\n"
            << "Standard: 0x" << standardStream.str() << "\n"
            << "Video Format: " << vfString << "\n"
            << "Pixel Format: " << pfString << "\n"
            << "Src Channel: " << NTV2ChannelToString(srcChannel) << "\n"
            << "Dst Channel: " << NTV2ChannelToString(dstChannel) << "\n"
            << "Src Framestore: " << NTV2ChannelToString(srcFramestore) << "\n"
            << "Dst Framestore: " << NTV2ChannelToString(dstFramestore) << "\n"
            << "Src Reference: " << NTV2ReferenceSourceToString(srcRefSrc) << "\n"
            << "Ref Reference: " << NTV2ReferenceSourceToString(dstRefSrc) << "\n";
        if (refCard) {
            ossTest << "Ref Reference: " << NTV2ReferenceSourceToString(refRefSrc) << "\n";
            auto extRefRateFamily = GetFrameRateFamily(GetNTV2FrameRateFromVideoFormat(tc.vf));
            NTV2VideoFormat extRefVidFmt(tc.vf);
            if (extRefVidFmt != NTV2_FORMAT_525_5994 &&
                extRefVidFmt != NTV2_FORMAT_625_5000 && !NTV2_IS_720P_VIDEO_FORMAT(tc.vf)) {
                if (extRefRateFamily == NTV2_FRAMERATE_6000)
                    extRefVidFmt = NTV2_FORMAT_1080i_6000;
                else if (extRefRateFamily == NTV2_FRAMERATE_5994)
                    extRefVidFmt = NTV2_FORMAT_1080i_5994;
                else if (extRefRateFamily == NTV2_FRAMERATE_2500)
                    extRefVidFmt = NTV2_FORMAT_1080i_5000;
                else if (extRefRateFamily == NTV2_FRAMERATE_2400)
                    extRefVidFmt = NTV2_FORMAT_1080psf_2400;
                else if (extRefRateFamily == NTV2_FRAMERATE_2398)
                    extRefVidFmt = NTV2_FORMAT_1080psf_2398;
            }
            if (extRefVidFmt != NTV2_FORMAT_UNKNOWN) {
                NTV2FormatDesc fd(extRefVidFmt, NTV2_FBF_8BIT_YCBCR);
                NTV2_POINTER blackBuffer(fd.GetTotalBytes());
                if (_patternGen->DrawTestPattern(NTV2_TestPatt_Black, fd, blackBuffer)) {
                    refCard->DMAWriteFrame(NTV2_CHANNEL1, reinterpret_cast<ULWord*>(blackBuffer.GetHostPointer()), blackBuffer.GetByteCount());
                }
                EXPECT_TRUE(refCard->SetVideoFormat(extRefVidFmt));
                EXPECT_TRUE(refCard->SetFrameBufferFormat(NTV2_CHANNEL1, NTV2_FBF_8BIT_YCBCR));
                EXPECT_TRUE(refCard->SetMode(NTV2_CHANNEL1, NTV2_MODE_DISPLAY));
                EXPECT_TRUE(refCard->SetOutputFrame(NTV2_CHANNEL1, 0));
                EXPECT_TRUE(refCard->Connect(NTV2_XptAnalogOutInput, NTV2_XptFrameBuffer1YUV));
                ossTest << "Ref Format: " << NTV2VideoFormatToString(extRefVidFmt) << "\n";
            }
        }
        ossTest << ossVpid.str() << std::endl;
        std::string ossTestMsg(ossTest.str());
        AJA_INFO(ossTestMsg);

        if (!canDoTest(srcCardID, dstCardID, tc.vf, tc.pf)) {
            logSkipTest(tc);
            GTEST_SKIP();
        }
        if (useSingleCard) {
            if (NTV2_IS_QUAD_FRAME_FORMAT(tc.vf)) {
                logSkipTest(tc);
                GTEST_SKIP();
            }
        }

        qa::NTV2CardConfig srcCardCfg;
        srcCardCfg.base.index = cfg.deviceIndexSrc;
        srcCardCfg.base.multi_format = true;
        srcCardCfg.base.channel = srcChannel;
        srcCardCfg.base.framestore = srcFramestore;
        srcCardCfg.base.mode = NTV2_MODE_DISPLAY;
        srcCardCfg.video.ntv2_vid_fmt = tc.vf;
        srcCardCfg.video.ntv2_pix_fmt = tc.pf;
        srcCardCfg.video.ntv2_vanc_mode = tc.vancMode;
        srcCardCfg.timing.ref_select = tc.refSelect;
        srcCardCfg.sdi.smpte_standard = tc.standard;
        srcCardCfg.base.connection = qa::ConnectionKind::SDI;

        qa::NTV2CardConfig dstCardCfg(srcCardCfg);
        dstCardCfg.base.index = cfg.deviceIndexDst;
        dstCardCfg.base.channel = dstChannel;
        dstCardCfg.base.framestore = dstFramestore;
        dstCardCfg.base.mode = NTV2_MODE_CAPTURE;
        srcCard->SetMultiFormatMode(srcCardCfg.base.multi_format);
        dstCard->SetMultiFormatMode(dstCardCfg.base.multi_format);

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
        EXPECT_EQ(srcCard->DMAWriteFrame(srcCardCfg.base.channel, reinterpret_cast<const ULWord*>(_zeroBuffer.data()), frameSize), true);
        memset(&_zeroBuffer[0], 1, frameSize);
        EXPECT_EQ(dstCard->DMAWriteFrame(dstCardCfg.base.channel, reinterpret_cast<const ULWord*>(_zeroBuffer.data()), frameSize), true);

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
                    if (NTV2_IS_525_FORMAT(tc.vf)) {
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
        EXPECT_EQ(srcCard->DMAWriteFrame(srcCardCfg.base.channel, reinterpret_cast<const ULWord*>(_patternBuffer.data()), frameSize), true);
        std::this_thread::sleep_for(std::chrono::seconds(1)); // wait for signal to lock up
        size_t rasterBytes = fd.GetTotalBytes();
        EXPECT_EQ(dstCard->SetMode(dstCardCfg.base.channel, NTV2_MODE_CAPTURE, false), true);
        // EXPECT_EQ(dstCard->SetInputFrame(dstCardCfg.base.channel, cfg.frameIndexDst), true);
        EXPECT_EQ(dstCard->DMAReadFrame(dstCardCfg.base.channel, (ULWord*)&_readbackBuffer[0], (ULWord)rasterBytes), true);

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
                if (savePng(pngPath, fd, _patternBuffer))
                    AJA_INFO("Wrote test pattern PNG: " << pngPath);
                pngPath = tcFilename + "_readback.png";
                if (savePng(pngPath, fd, _readbackBuffer))
                    AJA_INFO("Wrote DMA readback PNG: " << pngPath);
            }
        }

        NTV2_POINTER masterBytes(_patternBuffer.data(), rasterBytes);
        NTV2_POINTER compareBytes(_readbackBuffer.data(), rasterBytes);
        CNTV2PixelComponentReader master(masterBytes, fd);
        CNTV2PixelComponentReader compare(compareBytes, fd);
        uint64_t rcStart = AJATime::GetSystemMilliseconds();
        EXPECT_EQ(master.ReadComponentValues(), AJA_STATUS_SUCCESS);
        EXPECT_EQ(compare.ReadComponentValues(), AJA_STATUS_SUCCESS);
        uint64_t rcEnd = AJATime::GetSystemMilliseconds() - rcStart;
        AJA_INFO("CNTV2PixelComponentReader::ReadComponents finished in " << rcEnd << "ms");
        EXPECT_TRUE(master == compare) << "CNTV2PixelComponentReader compare failed!"
            << "\n\nSource Card: (" << cfg.deviceIndexSrc << ") - " << NTV2DeviceIDToString(srcCardID) << " - " << _cardMgr->GetSrcSerial() << "\n"
            << "Dest Card: (" << cfg.deviceIndexDst << ") - " << NTV2DeviceIDToString(dstCardID) << " - " << _cardMgr->GetDstSerial() << "\n"
            << "Video Format: " << NTV2VideoFormatToString(tc.vf) << "\n"
            << "Pixel Format: " << NTV2FrameBufferFormatToString(tc.pf, true)
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
            proceed = (_jsonData != nullptr);
            EXPECT_TRUE(proceed) << "Error loading JSON test case data from: " << gTestCfg->jsonPath;
        }
        if (proceed) {
            EXPECT_TRUE(_cardMgr->InitializeCards());
            auto tc = static_cast<CardTestCase>(GetParam());
            NTV2FormatDesc fd(tc.vf, tc.pf);
            ULWord frameSize = fd.GetTotalBytes();
            _patternBuffer.resize(frameSize);
            _readbackBuffer.resize(frameSize);
            _zeroBuffer.resize(frameSize);
            _patternGen = new NTV2TestPatternGen();
        }
    }

    void TearDown() override {
        AJA_INFO("CardTest::TearDown");
        if (_patternGen) {
            delete _patternGen;
            _patternGen = nullptr;
        }
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
            _cardMgr = new CardManager(*gTestCfg);
        }
    }

    static void TearDownTestSuite() {
        AJA_INFO("CardTest::TearDownTestSuite");
        if (_cardMgr) {
            delete _cardMgr;
            _cardMgr = nullptr;
        }
        if (_jsonData) {
            delete _jsonData;
            _jsonData = nullptr;
        }
    }

    std::vector<UByte> _patternBuffer;
    std::vector<UByte> _readbackBuffer;
    std::vector<UByte> _zeroBuffer;

    uint32_t _numTests;
    uint32_t _numTestsDone;
    uint32_t _numTestsPass;
    uint32_t _numTestsFail;

    static CardManager* _cardMgr;
    NTV2TestPatternGen* _patternGen;
    static nlohmann::json* _jsonData;
};

CardManager* CardTest::_cardMgr = nullptr;
nlohmann::json* CardTest::_jsonData = nullptr;

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
    parser.AddOption(AJACommandLineOption(AJAStringList{ "d", "device" }, "NTV2 Card index (or indices if more than one arg set)"));
    parser.AddOption(AJACommandLineOption(AJAStringList{ "e", "ref_device" }, "NTV2 Card index for ext. ref source device"));
    parser.AddOption(AJACommandLineOption(AJAStringList{ "pcb_ver_dst" }, "Capture device PCB version override hex value"));
    parser.AddOption(AJACommandLineOption(AJAStringList{ "ch_src" }, "NTV2 Channel Index for Output (src) device"));
    parser.AddOption(AJACommandLineOption(AJAStringList{ "ch_dst" }, "NTV2 Channel Index for Capture (dst) device"));
    parser.AddOption(AJACommandLineOption(AJAStringList{ "fs_src" }, "NTV2 Framestore Index for Output (src) device"));
    parser.AddOption(AJACommandLineOption(AJAStringList{ "fs_dst" }, "NTV2 Framestore Index for Capture (dst) device"));
    parser.AddOption(AJACommandLineOption(AJAStringList{ "frm_src" }, "NTV2 Frame Index for Output (src) device"));
    parser.AddOption(AJACommandLineOption(AJAStringList{ "frm_dst" }, "NTV2 Frame Index for Capture (dst) device"));
    parser.AddOption(AJACommandLineOption(AJAStringList{ "ref_dst" }, "NTV2 Reference Source for Capture (dst) device"));
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

    AJAVariantList cardIndices = parser.Values("device");
    if (cardIndices.size() > 0) {
        gTestCfg->deviceIndexSrc = cardIndices[0].AsUInt32();
        gTestCfg->deviceIndexDst = cardIndices[1].AsUInt32();
    }
    if (parser.IsSet("ref_device"))
        gTestCfg->deviceIndexRef = parser.Value("ref_device").AsInt32();
    if (parser.IsSet("pcb_ver_dst"))
        gTestCfg->pcbVerDst = parser.Value("pcb_ver_dst").AsInt32();
    gTestCfg->channelIndexSrc = parser.Value("ch_src").AsUInt32();
    gTestCfg->channelIndexDst = parser.Value("ch_dst").AsUInt32();
    gTestCfg->framestoreIndexSrc = parser.Value("fs_src").AsUInt32();
    gTestCfg->framestoreIndexDst = parser.Value("fs_dst").AsUInt32();
    gTestCfg->frameIndexSrc = parser.Value("frm_src").AsUInt32();
    gTestCfg->frameIndexDst = parser.Value("frm_dst").AsUInt32();
    if (parser.IsSet("ref_dst"))
        gTestCfg->refSourceIndexDst = parser.Value("ref_dst").AsUInt32();
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
