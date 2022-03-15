/* ut_ntv2bitfile -- Unit Tests for NTV2 Classic Firmware Bitfiles
 *
 * This test depends upon a file called `ntv2_classic_firmware.json` which must be present
 * in the same directory as the `ut_ntv2bitfile` binary. Each entry in the JSON list contains
 * information about each ntv2 bitfile in active production today. When new bitfiles are committed
 * to SVN the JSON file must be updated with the new expected data.
 */
#define DOCTEST_CONFIG_IMPLEMENT
#define DOCTEST_THREAD_LOCAL

#include <argparse/argparse.h>
#include <doctest/doctest.h>
#include <nlohmann/json.hpp>
using nlohmann::json;

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

static constexpr auto kFirmwareJSON = "ntv2_classic_firmware.json";

struct TestOptions {
    const char* fw_path;
};

static TestOptions gOpts;

AJAStatus read_json_file(const std::string& path, json& j)
{
    AJAFileIO f;
    if (f.Open(path, eAJAReadOnly, 0) == AJA_STATUS_SUCCESS) {
        int64_t create_time, mod_time, file_size;
        if (f.FileInfo(create_time, mod_time, file_size) == AJA_STATUS_SUCCESS) {
            std::string json_str;
            if (f.Read(json_str, (uint32_t)file_size) == file_size) {
                j = json::parse(json_str);
                return AJA_STATUS_SUCCESS;
            }
        }
    }
    return AJA_STATUS_FAIL;
}

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
    TEST_CASE("bitfile_path_not_null")
    {
        CHECK_NE(gOpts.fw_path, nullptr);
    }
    TEST_CASE("bitfile_exists")
    {
        bool bitfile_exists = false;
        if (gOpts.fw_path != NULL) {
            const std::string& path = gOpts.fw_path;
            bitfile_exists = AJAFileIO::FileExists(path);
        }
        CHECK_EQ(bitfile_exists, true);
    }
    TEST_CASE("path_is_bitfile") {
        bool is_bitfile = false;
        if (gOpts.fw_path != NULL) {
            const std::string& path = gOpts.fw_path;
            size_t dot_idx = path.rfind('.');
            if (dot_idx > 0 && dot_idx < INT32_MAX) {
                if (path.substr(dot_idx, 4) == ".bit")
                    is_bitfile = true;
            }
        }
        CHECK_EQ(is_bitfile, true);
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

        const std::string& path = gOpts.fw_path;
        bool is_bitfile = false;
        size_t dot_idx = path.rfind('.');
        if (dot_idx > 0 && dot_idx < INT32_MAX) {
            if (path.substr(dot_idx, 4) == ".bit")
                is_bitfile = true;
        }

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
                        CHECK_EQ(int(nfo.bitfileVersion), want_bitfile_ver);
                        CHECK_EQ(int(nfo.bitfileID), want_bitfile_id);
                        CHECK_EQ(int(nfo.designVersion), want_design_ver);
                        CHECK_EQ(int(nfo.designID), want_design_id);
                        found_bitfile_in_json = true;
                        break;
                    }
                }
            }
            CHECK_EQ(found_bitfile_in_json, true);
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
        OPT_STRING('p', "path", &gOpts.fw_path, "path to a single NTV2 Classic Firmware bitfile, or to the `tprom` or `reconfig` directory for a specific AJA card."),
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