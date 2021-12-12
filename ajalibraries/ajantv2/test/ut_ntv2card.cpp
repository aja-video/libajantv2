#define DOCTEST_CONFIG_IMPLEMENT
// need to define this so will work with compilers that don't support thread_local
// ie xcode 6, 7
#define DOCTEST_THREAD_LOCAL

#include <argparse/argparse.h>
#include <doctest/doctest.h>

#include "test_support.h"

#include "ajantv2/includes/ntv2card.h"
#include "ajantv2/includes/ntv2devicefeatures.h"

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

static CNTV2Card* gCardInstance = NULL;

void ntv2card_general_marker() {}
TEST_SUITE("CNTV2Card General" * doctest::description("CNTV2Card general tests")) {
    TEST_CASE("IsOpen") {
        CHECK(gCardInstance->IsOpen() == true);
    }
    TEST_CASE("GetDeviceID") {
        CHECK(gCardInstance->GetDeviceID() != DEVICE_ID_NOTFOUND);
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
        CHECK(gCardInstance->SetMode(channel, NTV2_MODE_DISPLAY) == true);
        CHECK(gCardInstance->DMAWriteAudio(audio_sys, reinterpret_cast<const ULWord*>(buffer), 0, kAudioSize4MiB) == true);
        CHECK(gCardInstance->DMAReadAudio(audio_sys, reinterpret_cast<ULWord*>(readback), 0, kAudioSize4MiB) == true);
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
        CHECK(gCardInstance->SetMode(channel, NTV2_MODE_DISPLAY) == true);
        CHECK(gCardInstance->SetOutputFrame(channel, frame_num) == true);
        CHECK(gCardInstance->DMAWriteFrame(frame_num, reinterpret_cast<const ULWord*>(buffer), (ULWord)kFrameSize8MiB) == true);
        CHECK(gCardInstance->DMAReadFrame(frame_num, reinterpret_cast<ULWord*>(&readback[0]), (ULWord)kFrameSize8MiB) == true);
        CHECK(memcmp(readback, buffer, kFrameSize8MiB) == 0);
        free((void*)buffer);
        free((void*)readback);
        buffer = NULL;
        readback = NULL;
    }
    TEST_CASE("DMAWriteFrame/DMAReadFrame All Frames" * doctest::description("DMAWriteFrame/DMAReadFrame all frames random data test")) {
        NTV2Channel channel = NTV2_CHANNEL1;
        const ULWord num_frame_buffers = gCardInstance->GetNumFrameBuffers();
        uint8_t* buffer = (uint8_t*)malloc(kFrameSize8MiB);
        uint8_t* readback = (uint8_t*)malloc(kFrameSize8MiB);
        size_t num_writes = 0;
        size_t num_reads = 0;
        size_t num_cmps = 0;
        for (ULWord i = 0; i < num_frame_buffers; i++) {
            gCardInstance->SetMode(channel, NTV2_MODE_DISPLAY);
            gCardInstance->SetOutputFrame(channel, i);
            qa::RandomPattern<uint32_t>(0, 0, UINT32_MAX, buffer, kFrameSize8MiB, ByteOrder::LittleEndian);
            qa::RandomPattern<uint32_t>(0, 0, UINT32_MAX, readback, kFrameSize8MiB, ByteOrder::LittleEndian);
            if (gCardInstance->DMAWriteFrame(i, reinterpret_cast<const ULWord*>(buffer), (ULWord)kFrameSize8MiB))
                num_writes++;
            if (gCardInstance->DMAReadFrame(i, reinterpret_cast<ULWord*>(readback), (ULWord)kFrameSize8MiB))
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
        CHECK(gCardInstance->SetMode(NTV2_CHANNEL1, NTV2_MODE_DISPLAY) == true);
        CHECK(gCardInstance->SetVideoFormat(vf, false, false, NTV2_CHANNEL1) == true);
        CHECK(gCardInstance->SetFrameBufferFormat(NTV2_CHANNEL1, pf) == true);
        NTV2FormatDesc fd(vf, pf);
        auto raster_size = fd.GetTotalBytes();
        uint8_t* buffer = (uint8_t*)malloc(kFrameSize8MiB);
        uint8_t* readback = (uint8_t*)malloc(kFrameSize8MiB);
        qa::Generate8Bit(fd.GetRasterHeight(), fd.GetBytesPerRow(), buffer, kSDILegalMin, kSDILegalMax8Bit);
        qa::RandomPattern<uint32_t>(0, 0, UINT32_MAX, readback, fd.GetTotalBytes(), ByteOrder::LittleEndian);
        CHECK(gCardInstance->DMAWriteFrame(0, reinterpret_cast<const ULWord*>(buffer), raster_size) == true);
        CHECK(gCardInstance->DMAReadFrame(0, reinterpret_cast<ULWord*>(readback), raster_size) == true);
        CHECK(memcmp(buffer, readback, raster_size) == 0);
        free((void*)buffer);
        free((void*)readback);
        buffer = NULL;
        readback = NULL;
    }
    TEST_CASE("Framestore UHDp30 YUV-8") {
        auto vf = NTV2_FORMAT_4x1920x1080p_3000;
        auto pf = NTV2_FBF_8BIT_YCBCR;
        CHECK(gCardInstance->SetMode(NTV2_CHANNEL1, NTV2_MODE_DISPLAY) == true);
        CHECK(gCardInstance->SetVideoFormat(vf, false, false, NTV2_CHANNEL1) == true);
        CHECK(gCardInstance->SetFrameBufferFormat(NTV2_CHANNEL1, pf) == true);
        NTV2FormatDesc fd(vf, pf);
        auto raster_size = fd.GetTotalBytes();
        uint8_t* buffer = (uint8_t*)malloc(kFrameSize32MiB);
        uint8_t* readback = (uint8_t*)malloc(kFrameSize32MiB);
        qa::Generate8Bit(fd.GetRasterHeight(), fd.GetBytesPerRow(), buffer, kSDILegalMin, kSDILegalMax8Bit);
        qa::RandomPattern<uint32_t>(0, 0, UINT32_MAX, readback, fd.GetTotalBytes(), ByteOrder::LittleEndian);
        CHECK(gCardInstance->DMAWriteFrame(0, reinterpret_cast<const ULWord*>(buffer), raster_size) == true);
        CHECK(gCardInstance->DMAReadFrame(0, reinterpret_cast<ULWord*>(readback), raster_size) == true);
        CHECK(memcmp(buffer, readback, raster_size) == 0);
        free((void*)buffer);
        free((void*)readback);
        buffer = NULL;
        readback = NULL;
    }
}

int main(int argc, const char** argv) {
    doctest::Context ctx(argc, argv);
    gCardInstance = new CNTV2Card(0, "");
    int res = ctx.run();
    if (gCardInstance) {
        delete gCardInstance;
        gCardInstance = NULL;
    }
    return ctx.shouldExit() ? res : EXIT_SUCCESS;
}
