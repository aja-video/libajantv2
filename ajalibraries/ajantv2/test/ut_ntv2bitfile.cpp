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

#include "ajabase/system/file_io.h"
#include "ajabase/system/system.h"
#include "ajabase/system/systemtime.h"
#include "ajabase/system/debug.h"

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

json read_json_file(const std::string& path)
{
    AJAFileIO f;
    f.Open(path, eAJAReadOnly, 0);
    int64_t create_time, mod_time, file_size;
    f.FileInfo(create_time, mod_time, file_size);
    std::string json_str;
    f.Read(json_str, (uint32_t)file_size);
    auto fw_json = json::parse(json_str);
    return fw_json;
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

/* ---------- START TESTS SECTION ---------- */
void ntv2bitfile_header_marker() {}
TEST_SUITE("bitfile_header" * doctest::description("NTV2 bitfile header tests")) {
    TEST_CASE("check bitfile headers") {
        // read ntv2_classic_firmware.json
        std::string exe_path;
        std::string exe_dir;
        AJAFileIO::GetExecutablePath(exe_path);
        AJAFileIO::GetDirectoryName(exe_path, exe_dir);
        auto fw_json_path = exe_dir + AJA_PATHSEP + kFirmwareJSON;
        auto fw_json = read_json_file(fw_json_path);

        const std::string& path = gOpts.fw_path;
        AJAFileIO fio;
        bool dir_exists = AJAFileIO::DoesDirectoryExist(path) == AJA_STATUS_SUCCESS;
        bool file_exists = AJAFileIO::FileExists(path);
        CHECK((dir_exists == true || file_exists == true));

        if (dir_exists) {
            // AJAStringList tpromList;
            // AJAFileIO::ReadDirectory(gOpts.fw_path, "*.bit", tpromList);
            // CHECK_GT(tpromList.size(), 0);
            // for (auto& tprom : tpromList) {
            //     LOGINFO(tprom << "\n");
            //     for (auto& fw : fw_json["firmware"]) {
            //         LOGINFO(fw);
            //         std::string fw_filename = fw["filename"];
            //         auto bitfile_id = fw["bitfile_id"];
            //     }
            // }

            NTV2BitfileInfoList bitfile_info_list;
            if (get_bitfile_info_list(path, bitfile_info_list)) {
                for (const auto& bf : bitfile_info_list) {
                    auto bitfile_type = get_bitfile_type(bf.bitfileFlags);
                    auto bitfile_type_str = get_bitfile_type_string(bitfile_type);
                }
            }
        } else if (file_exists) {
            if (fio.Open(path, eAJAReadOnly, 0) != AJA_STATUS_SUCCESS)
                goto cleanup;
cleanup:
            fio.Close();
        }
    }
}

/* ---------- END TESTS SECTION ---------- */

int main(int argc, const char** argv)
{
    AJADebug::Open();
    LOGINFO("Starting NTV2 bitfile unit tests...");

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
        OPT_STRING('p', "path", &gOpts.fw_path, "path to ntv2 classic firmware directory or single bitfile"),
        OPT_END(),
    };
    struct argparse argparse;
    argparse_init(&argparse, options, usage, ARGPARSE_IGNORE_UNKNOWN_ARGS);
    argparse_describe(&argparse, "\nntv2 card unit tests",
        "\nPerform CNTV2Card tests against physical hardware.");
    argparse_parse(&argparse, argc, (const char**)argv);

    int res = ctx.run();

    LOGINFO("Completed NTV2 bitfile unit tests!");
    AJADebug::Close();

    return ctx.shouldExit() ? res : EXIT_SUCCESS;
}