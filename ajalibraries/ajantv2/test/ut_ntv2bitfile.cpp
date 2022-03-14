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

static constexpr auto kFirmwareJSON = L"ntv2_classic_firmware.json";

struct TestOptions {
    const char* fw_path;
};

static TestOptions gOpts;

json read_json_file(const std::wstring& path)
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

    std::wstring exe_path;
    std::wstring exe_dir;
    AJAFileIO::GetExecutablePath(exe_path);
    AJAFileIO::GetDirectoryName(exe_path, exe_dir);
    auto fw_json_path = exe_dir + AJA_PATHSEP_WIDE + kFirmwareJSON;
    auto fw_json = read_json_file(fw_json_path);
    for (auto& fw : fw_json["firmware"]) {
        LOGINFO(fw);
    }
    // LOGINFO(json_str);

    LOGINFO("Completed NTV2 bitfile unit tests!");
    AJADebug::Close();

    return ctx.shouldExit() ? res : EXIT_SUCCESS;
}