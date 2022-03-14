#define DOCTEST_CONFIG_IMPLEMENT
// need to define this so will work with compilers that don't support thread_local
// ie xcode 6, 7
#define DOCTEST_THREAD_LOCAL

#include <argparse/argparse.h>
#include <doctest/doctest.h>
#include <nlohmann/json.hpp>

#include "ajabase/system/file_io.h"
#include "ajabase/system/system.h"
#include "ajabase/system/systemtime.h"
#include "ajabase/system/debug.h"

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

static constexpr auto kFirmwareJSON = "ntv2_classic_firmware.json";

using nlohmann::json;

json read_json_file(const std::string& path)
{ 
    AJAFileIO f;
    f.Open(path, eAJAReadOnly, 0);
    int64_t create_time, mod_time, file_size;
    f.FileInfo(create_time, mod_time, file_size);
    std::string json_str;
    f.Read(json_str, file_size);
    auto fw_json = json::parse(json_str);
    return fw_json;
}

int main(int argc, const char** argv)
{
    AJADebug::Open();
    LOGINFO("Starting NTV2 bitfile unit tests...");

    doctest::Context ctx(argc, argv);
    int res = ctx.run();

    std::string exe_path, exe_dir;
    AJAFileIO::GetExecutablePath(exe_path);
    AJAFileIO::GetDirectoryName(exe_path, exe_dir);
    auto fw_json_path = exe_dir + AJA_PATHSEP + kFirmwareJSON;   
    auto fw_json = read_json_file(fw_json_path);
    for (auto& fw : fw_json["firmware"]) {
        LOGINFO(fw);
    }
    // LOGINFO(json_str);

    LOGINFO("Completed NTV2 bitfile unit tests!");
    AJADebug::Close();

    return ctx.shouldExit() ? res : EXIT_SUCCESS;
}