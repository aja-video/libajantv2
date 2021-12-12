#define DOCTEST_CONFIG_IMPLEMENT
// need to define this so will work with compilers that don't support thread_local
// ie xcode 6, 7
#define DOCTEST_THREAD_LOCAL

#include <argparse/argparse.h>
#include <doctest/doctest.h>

#include "test_support.h"

#include "ajantv2/includes/ntv2card.h"
#include "ajantv2/includes/ntv2devicefeatures.h"

static int
argparse_help(struct argparse *self, const struct argparse_option *option)
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
static constexpr uint8_t kSDILegalMin = 0x04;
static constexpr uint8_t kSDILegalMax8Bit = 0xfb;
static constexpr uint16_t kSDILegalMax10Bit = 0x3fb;
static constexpr uint32_t kSDILegalMax12Bit = 0xffc;

struct TestOptions {
    uint32_t card_index;
};

static TestOptions* gOpts = NULL;
static CNTV2Card* gCard = NULL;

void ntv2card_general_marker() {}
TEST_SUITE("CNTV2Card General" * doctest::description("CNTV2Card general tests")) {
    TEST_CASE("IsOpen") {
        CHECK(gCard->IsOpen() == true);
    }
    TEST_CASE("GetDeviceID") {
        CHECK(gCard->GetDeviceID() != DEVICE_ID_NOTFOUND);
    }
}

void ntv2card_dma_marker() {}
TEST_SUITE("CNTV2Card DMA" * doctest::description("CNTV2Card DMA tests")) {
    TEST_CASE("DMAWriteAudio/DMAReadAudio") {
        NTV2Channel channel = NTV2_CHANNEL1;
        NTV2AudioSystem audio_sys = NTV2ChannelToAudioSystem(channel);
        uint8_t* buffer = (uint8_t*)malloc(kAudioSize4MiB);
        uint8_t* readback = (uint8_t*)malloc(kAudioSize4MiB);
        qa::RandomPattern<uint32_t>(0, 0, UINT32_MAX, buffer, kAudioSize4MiB, ByteOrder::LittleEndian);
        qa::RandomPattern<uint32_t>(0, 0, UINT32_MAX, readback, kAudioSize4MiB, ByteOrder::LittleEndian);
        CHECK(gCard->SetMode(channel, NTV2_MODE_DISPLAY) == true);
        CHECK(gCard->DMAWriteAudio(audio_sys, reinterpret_cast<const ULWord*>(buffer), 0, kAudioSize4MiB) == true);
        CHECK(gCard->DMAReadAudio(audio_sys, reinterpret_cast<ULWord*>(readback), 0, kAudioSize4MiB) == true);
        free((void*)buffer);
        free((void*)readback);
        buffer = NULL;
        readback = NULL;
    }
    TEST_CASE("DMAWriteFrame/DMAReadFrame One Frame" * doctest::description("DMAWriteFrame/DMAReadFrame frame 0 random data test")) {
        NTV2Channel channel = NTV2_CHANNEL1;
        ULWord frame_num = 0;
        uint8_t* buffer = (uint8_t*)malloc(kFrameSize8MiB);
        uint8_t* readback = (uint8_t*)malloc(kFrameSize8MiB);
        qa::RandomPattern<uint32_t>(0, 0, UINT32_MAX, buffer, kFrameSize8MiB, ByteOrder::LittleEndian);
        qa::RandomPattern<uint32_t>(0, 0, UINT32_MAX, readback, kFrameSize8MiB, ByteOrder::LittleEndian);
        CHECK(gCard->SetMode(channel, NTV2_MODE_DISPLAY) == true);
        CHECK(gCard->SetOutputFrame(channel, frame_num) == true);
        CHECK(gCard->DMAWriteFrame(frame_num, reinterpret_cast<const ULWord*>(buffer), (ULWord)kFrameSize8MiB) == true);
        CHECK(gCard->DMAReadFrame(frame_num, reinterpret_cast<ULWord*>(&readback[0]), (ULWord)kFrameSize8MiB) == true);
        CHECK(memcmp(readback, buffer, kFrameSize8MiB) == 0);
        free((void*)buffer);
        free((void*)readback);
        buffer = NULL;
        readback = NULL;
    }
    TEST_CASE("DMAWriteFrame/DMAReadFrame All Frames" * doctest::description("DMAWriteFrame/DMAReadFrame all frames random data test")) {
        NTV2Channel channel = NTV2_CHANNEL1;
        const ULWord num_frame_buffers = gCard->GetNumFrameBuffers();
        uint8_t* buffer = (uint8_t*)malloc(kFrameSize8MiB);
        uint8_t* readback = (uint8_t*)malloc(kFrameSize8MiB);
        size_t num_writes = 0;
        size_t num_reads = 0;
        size_t num_cmps = 0;
        for (ULWord i = 0; i < num_frame_buffers; i++) {
            gCard->SetMode(channel, NTV2_MODE_DISPLAY);
            gCard->SetOutputFrame(channel, i);
            qa::RandomPattern<uint32_t>(0, 0, UINT32_MAX, buffer, kFrameSize8MiB, ByteOrder::LittleEndian);
            qa::RandomPattern<uint32_t>(0, 0, UINT32_MAX, readback, kFrameSize8MiB, ByteOrder::LittleEndian);
            if (gCard->DMAWriteFrame(i, reinterpret_cast<const ULWord*>(buffer), (ULWord)kFrameSize8MiB))
                num_writes++;
            if (gCard->DMAReadFrame(i, reinterpret_cast<ULWord*>(readback), (ULWord)kFrameSize8MiB))
                num_reads++;
            if (memcmp(readback, buffer, kFrameSize8MiB) == 0)
                num_cmps++;
        }
        free((void*)buffer);
        free((void*)readback);
        buffer = NULL;
        readback = NULL;
        CHECK(num_writes == num_frame_buffers);
        CHECK(num_reads == num_frame_buffers);
        CHECK(num_cmps == num_frame_buffers);
    }
}

void ntv2card_framestore_marker() {}
TEST_SUITE("Framestore Formats" * doctest::description("Framestore widget format tests")) {
    TEST_CASE("Framestore 1080p30 YUV-8") {
        auto vf = NTV2_FORMAT_1080p_3000;
        auto pf = NTV2_FBF_8BIT_YCBCR;
        CHECK(gCard->SetMode(NTV2_CHANNEL1, NTV2_MODE_DISPLAY) == true);
        CHECK(gCard->SetVideoFormat(vf, false, false, NTV2_CHANNEL1) == true);
        CHECK(gCard->SetFrameBufferFormat(NTV2_CHANNEL1, pf) == true);
        NTV2FormatDesc fd(vf, pf);
        auto raster_size = fd.GetTotalBytes();
        uint8_t* buffer = (uint8_t*)malloc(kFrameSize8MiB);
        uint8_t* readback = (uint8_t*)malloc(kFrameSize8MiB);
        qa::Generate8Bit(fd.GetRasterHeight(), fd.GetBytesPerRow(), buffer, kSDILegalMin, kSDILegalMax8Bit);
        qa::RandomPattern<uint32_t>(0, 0, UINT32_MAX, readback, fd.GetTotalBytes(), ByteOrder::LittleEndian);
        CHECK(gCard->DMAWriteFrame(0, reinterpret_cast<const ULWord*>(buffer), raster_size) == true);
        CHECK(gCard->DMAReadFrame(0, reinterpret_cast<ULWord*>(readback), raster_size) == true);
        CHECK(memcmp(buffer, readback, raster_size) == 0);
        free((void*)buffer);
        free((void*)readback);
        buffer = NULL;
        readback = NULL;
    }
    TEST_CASE("Framestore UHDp30 YUV-8") {
        auto vf = NTV2_FORMAT_4x1920x1080p_3000;
        auto pf = NTV2_FBF_8BIT_YCBCR;
        CHECK(gCard->SetMode(NTV2_CHANNEL1, NTV2_MODE_DISPLAY) == true);
        CHECK(gCard->SetVideoFormat(vf, false, false, NTV2_CHANNEL1) == true);
        CHECK(gCard->SetFrameBufferFormat(NTV2_CHANNEL1, pf) == true);
        NTV2FormatDesc fd(vf, pf);
        auto raster_size = fd.GetTotalBytes();
        uint8_t* buffer = (uint8_t*)malloc(kFrameSize32MiB);
        uint8_t* readback = (uint8_t*)malloc(kFrameSize32MiB);
        qa::Generate8Bit(fd.GetRasterHeight(), fd.GetBytesPerRow(), buffer, kSDILegalMin, kSDILegalMax8Bit);
        qa::RandomPattern<uint32_t>(0, 0, UINT32_MAX, readback, fd.GetTotalBytes(), ByteOrder::LittleEndian);
        CHECK(gCard->DMAWriteFrame(0, reinterpret_cast<const ULWord*>(buffer), raster_size) == true);
        CHECK(gCard->DMAReadFrame(0, reinterpret_cast<ULWord*>(readback), raster_size) == true);
        CHECK(memcmp(buffer, readback, raster_size) == 0);
        free((void*)buffer);
        free((void*)readback);
        buffer = NULL;
        readback = NULL;
    }
}

int main(int argc, const char** argv) {
    gOpts = new TestOptions();
    gOpts->card_index = 0;

    // copy argv list
    char** new_argv = (char**)malloc((argc+1) * sizeof *new_argv);
    for(int i = 0; i < argc; ++i)
    {
        size_t length = strlen(argv[i])+1;
        new_argv[i] = (char*)malloc(length);
        memcpy(new_argv[i], argv[i], length);
    }
    new_argv[argc] = NULL;

    // handle global test args
    static const char *const usage[] = {
        "ut_ntv2card [options] [[--] args]",
        "ut_ntv2card [options]",
        NULL,
    };
    struct argparse_option options[] = {
        OPT_ARGPARSE_HELP(),
        OPT_GROUP("ut_ntv2card options"),
        OPT_INTEGER('\0', "card", &gOpts->card_index, "card_index"),
        OPT_END(),
    };
    struct argparse argparse;
    argparse_init(&argparse, options, usage, 0);
    argparse_describe(&argparse, "\nntv2 card unit tests",
        "\nPerform CNTV2Card tests against physical hardware.");
    argparse_parse(&argparse, argc, (const char**)argv);

    // initialize doctest and handle args
    doctest::Context ctx(argc, new_argv);
    gCard = new CNTV2Card(gOpts->card_index, "");
    int res = ctx.run();
    if (gCard) {
        delete gCard;
        gCard = NULL;
    }

    // free memory
    for(int i = 0; i < argc; ++i)
    {
        free(new_argv[i]);
    }
    free(new_argv);

    delete gOpts;

    return ctx.shouldExit() ? res : EXIT_SUCCESS;
}
