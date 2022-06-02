/* ut_ntv2bitfile -- Unit Tests for NTV2 Classic Firmware Bitfiles
 *
 * This test depends upon a file called `ntv2_classic_firmware.json` which must be present
 * in the same directory as the `ut_ntv2bitfile` binary. Each entry in the JSON list contains
 * information about each ntv2 bitfile in active production today. When new bitfiles are committed
 * to SVN the JSON file must be updated with the new expected data.
 */
#define DOCTEST_CONFIG_IMPLEMENT
#define DOCTEST_THREAD_LOCAL
#include "doctest.h"
#include "argparse.h"

#include "test_support.hpp"

#include "ajabase/common/common.h"
#include "ajabase/system/file_io.h"
#include "ajabase/system/system.h"
#include "ajabase/system/systemtime.h"
#include "ajabase/system/debug.h"

#include "ajantv2/includes/ntv2bitfile.h"
#include "ajantv2/includes/ntv2bitfilemanager.h"

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

using AJAStringList = std::vector<std::string>;

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

void check_bitfile_header(const std::string& path, json& fw_json);
#define CHECK_BITFILE_HEADER(__path__, __json__) \
    check_bitfile_header(__path__, __json__)

struct TestOptions {
    const char* firmware_path;  /* path to an NTV2 bitfile or directory of bitfiles */
    bool update_json;           /* update NTV2 firmware JSON expected results from firmware-under-test? */
};
static TestOptions gOptions;
static constexpr auto kFirmwareJSON = "json/ntv2_classic_firmware.json";
static constexpr size_t kNumValidFirmwareDirs = 5;
static constexpr const char* kValidFirmwareDirs[kNumValidFirmwareDirs] = {
    "tprom", "reconfig", "custom", "failsafe", "legacy"
};

enum class BitfileType {
    Tandem,                 /* TPROM Bitfile*/
    Partial,                /* Dynamic Reconfig Partial Bitfile */
    Clear,                  /* Dynamic Reconfig Clear Bitfile */
    Unknown,
};

BitfileType get_bitfile_type(uint32_t flags)
{
    if (flags & NTV2_BITFILE_FLAG_TANDEM)
        return BitfileType::Tandem;
    if (flags & NTV2_BITFILE_FLAG_PARTIAL)
        return BitfileType::Partial;
    if (flags & NTV2_BITFILE_FLAG_CLEAR)
        return BitfileType::Clear;
    return BitfileType::Unknown;
}

std::string get_bitfile_type_string(const BitfileType& type)
{
    if (type == BitfileType::Tandem)
        return "Tandem";
    if (type == BitfileType::Partial)
        return "Partial";
    if (type == BitfileType::Clear)
        return "Clear";
    return "";
}

bool get_bitfile_info(const std::string& path, NTV2BitfileInfo& nfo)
{
    CNTV2BitfileManager mgr;
    if (mgr.AddFile(path)) {
        // dynamic bitfiles...
        const auto& nfo_list = mgr.GetBitfileInfoList();
        if (nfo_list.size() == 1) {
            nfo = nfo_list.at(0);
            return true;
        }
    } else {
        // ...classic bitfiles.
        CNTV2Bitfile bf;
        if (bf.Open(path)) {
            nfo.designName		= bf.GetDesignName();
            nfo.designID		= bf.GetDesignID();
            nfo.designVersion	= bf.GetDesignVersion();
            nfo.bitfileID		= bf.GetBitfileID();
            nfo.bitfileVersion =  bf.GetBitfileVersion();
            return true;
        }
    }
    return false;
}

bool get_bitfile_info_list(const std::string& path, NTV2BitfileInfoList& nfo)
{
    if (AJAFileIO::DoesDirectoryExist(path) == AJA_STATUS_SUCCESS) {
        CNTV2BitfileManager mgr;
        if (mgr.AddDirectory(path)) {
            nfo = mgr.GetBitfileInfoList();
            if (nfo.size() > 0)
                return true;
        }
    }
    return false;
}

bool is_bitfile_path(const std::string& path)
{
    if (AJAFileIO::FileExists(path)) {
        size_t dot_idx = path.rfind('.');
        if (dot_idx > 0 && dot_idx < INT32_MAX) {
            if (path.substr(dot_idx, 4) == ".bit")
                return true;
        }
    }
    return false;
}

bool is_firmware_subdir(const std::string& path)
{
    if (AJAFileIO::DoesDirectoryExist(path) == AJA_STATUS_SUCCESS) {
        std::string path_strip(path);
        path_strip = aja::rstrip(path_strip, std::string(1, AJA_PATHSEP));
        for (uint8_t i = 0; i < kNumValidFirmwareDirs; i++) {
            std::string check_dir = std::string(kValidFirmwareDirs[i]);
            if (path_strip.length() >= check_dir.length()) {
                if (!path_strip.compare(path_strip.length() - check_dir.length(), check_dir.length(), check_dir)) {
                    return true;
                }
            }
        }
    }
    return false;
}


void check_with_msg(const std::string& bitfile, int expected, int found)
{
    std::ostringstream oss;
    oss << "Bitfile: " << bitfile << " Expected: " << expected << " Found: " << found;
    CHECK_MESSAGE(expected == found, oss.str());
}

void check_bitfile_header(const std::string& path, json& fw_json)
{
    AJAStatus status = AJA_STATUS_SUCCESS;
    bool is_bitfile = is_bitfile_path(path);
    bool bitfile_exists = AJAFileIO::FileExists(path);
    std::string bitfile_filename;
    status = AJAFileIO::GetFileName(path, bitfile_filename);
    if (is_bitfile && bitfile_exists && !bitfile_filename.empty()) {
        bool found_bitfile_in_json = false;
        NTV2BitfileInfo nfo;
        if (status == AJA_STATUS_SUCCESS && get_bitfile_info(path, nfo)) {
            for (auto& fw : fw_json["firmware"]) {
                std::string fw_filename = fw["filename"];
                if (fw_filename == bitfile_filename) {
                    auto want_bitfile_ver = std::stoi(fw["bitfile_ver"].get_ref<std::string&>());
                    auto want_bitfile_id = std::stoi(fw["bitfile_id"].get_ref<std::string&>());
                    auto want_design_ver = std::stoi(fw["design_ver"].get_ref<std::string&>());
                    auto want_design_id = std::stoi(fw["design_id"].get_ref<std::string&>());
                    SUBCASE("Bitfile Version") {
                        check_with_msg(fw_filename, want_bitfile_ver, int(nfo.bitfileVersion));
                    }
                    SUBCASE("Bitfile ID") {
                        check_with_msg(fw_filename, want_bitfile_id, int(nfo.bitfileID));
                    }
                    SUBCASE("Design Version") {
                        check_with_msg(fw_filename, want_design_ver, int(nfo.designVersion));
                    }
                    SUBCASE("Design ID") {
                        check_with_msg(fw_filename, want_design_id, int(nfo.designID));
                    }
                    found_bitfile_in_json = true;
                    break;
                }
            }
        }
        SUBCASE("Bitfile in JSON") { CHECK_EQ(found_bitfile_in_json, true); }
    }
}

void ntv2bitfile_header_marker() {}
TEST_SUITE("bitfile_header" * doctest::description("NTV2 bitfile header tests"))
{
    TEST_CASE("have_ntv2_classic_firmware_json")
    {
        std::string exe_path;
        std::string exe_dir;
        CHECK_EQ(AJAFileIO::GetExecutablePath(exe_path), AJA_STATUS_SUCCESS);
        CHECK_EQ(AJAFileIO::GetDirectoryName(exe_path, exe_dir), AJA_STATUS_SUCCESS);
        auto fw_json_path = exe_dir + AJA_PATHSEP + kFirmwareJSON;
        json fw_json;
        CHECK_EQ(read_json_file(fw_json_path, fw_json), AJA_STATUS_SUCCESS);
    }
    TEST_CASE("valid_firmware_path") {
        bool valid_path = false;
        if (gOptions.firmware_path != NULL) {
            std::string path = gOptions.firmware_path;
            if (is_bitfile_path(path) || is_firmware_subdir(path)) {
                valid_path = true;
            }
        }
        CHECK_EQ(valid_path, true);
    }
    TEST_CASE("bitfile_header_up_to_date") {
        // read ntv2_classic_firmware.json, containing expected values for bitfile headers
        std::string exe_path;
        std::string exe_dir;
        AJAFileIO::GetExecutablePath(exe_path);
        AJAFileIO::GetDirectoryName(exe_path, exe_dir);
        auto fw_json_path = exe_dir + AJA_PATHSEP + kFirmwareJSON;
        json fw_json;
        AJAStatus status = read_json_file(fw_json_path, fw_json);
        CHECK_EQ(status, AJA_STATUS_SUCCESS);
        const std::string& path = gOptions.firmware_path;
        if (is_bitfile_path(path)) {
            std::cout << "Bitfile: " << path << std::endl;
            CHECK_BITFILE_HEADER(path, fw_json);
        } else if (is_firmware_subdir(path)) {
            NTV2StringList bitfile_list;
            if (AJAFileIO::ReadDirectory(path, "*.bit", bitfile_list) == AJA_STATUS_SUCCESS) {
                if (bitfile_list.size() > 0) {
                    for (auto &&bf : bitfile_list) {
                        std::cout << "Bitfile: " << bf << std::endl;
                        CHECK_BITFILE_HEADER(bf, fw_json);
                    }
                }
            }
        }
    }
}

int main(int argc, const char** argv)
{
    AJADebug::Open();

    // copy argv
    char** new_argv = (char**)malloc((argc+1) * sizeof *new_argv);
    for(int i = 0; i < argc; ++i)
    {
        size_t length = strlen(argv[i])+1;
        new_argv[i] = (char*)malloc(length);
        memcpy(new_argv[i], argv[i], length);
    }
    new_argv[argc] = NULL;

    doctest::Context ctx(argc, argv);

    // handle global test args
    static const char *const usage[] = {
        "ut_ntv2bitfile [options] [[--] args]",
        "ut_ntv2bitfile [options]",
        NULL,
    };
    struct argparse_option options[] = {
        OPT_ARGPARSE_HELP(),
        OPT_GROUP("ut_ntv2bitfile options"),
        OPT_STRING('p', "path", &gOptions.firmware_path, "path to a single NTV2 Classic Firmware bitfile, or to the `tprom` or `reconfig` directory for a specific AJA card."),
        OPT_BOOLEAN('u', "update_json", reinterpret_cast<bool*>(&gOptions.update_json), "update NTV2 firmware JSON expected results from firmware-under-test?"),
        OPT_END(),
    };
    struct argparse argparse;
    argparse_init(&argparse, options, usage, ARGPARSE_IGNORE_UNKNOWN_ARGS);
    argparse_describe(&argparse, "\nntv2 bitfile unit tests",
        "\nPerform offline tests against NTV2 Classic Firmware bitfiles (not installed to an AJA card).");
    argparse_parse(&argparse, argc, (const char**)argv);

    int res = ctx.run();
    AJADebug::Close();
    return ctx.shouldExit() ? res : EXIT_SUCCESS;
}