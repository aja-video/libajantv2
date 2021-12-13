#pragma once

#include <cstdint>
#include <random>
#include <type_traits>
#include <vector>

#include "ajabase/common/bytestream.h"

namespace qa {

enum class ByteOrder {
    LittleEndian = 0,
    BigEndian = 1,
    Network = BigEndian,
};

template<class T>
inline T clamp(const T& v, const T& lo, const T& hi) {
    return (v < lo) ? lo : (v > hi) ? v : hi;
}

template<class T>
inline T wrap(const T& v, const T& lo, const T& hi) {
    return (v > hi) ? lo : v;
}

template <typename T>
void WriteBytestream(AJAByteStream& bs, T val, const ByteOrder& byte_order=ByteOrder::LittleEndian)
{
    bool little_endian = (byte_order == ByteOrder::LittleEndian);
    if (std::is_same<T, int8_t>::value  || std::is_same<T, uint8_t>::value)
        bs.Write8(static_cast<uint8_t>(val));
    else if (std::is_same<T, int16_t>::value || std::is_same<T, uint16_t>::value) {
        if (little_endian)
            bs.Write16LE(static_cast<uint16_t>(val));
        else
            bs.Write16BE(static_cast<uint16_t>(val));
    }
    else if (std::is_same<T, int32_t>::value || std::is_same<T, uint32_t>::value) {
        if (little_endian)
            bs.Write32LE(static_cast<uint32_t>(val));
        else
            bs.Write32BE(static_cast<uint32_t>(val));
    }
    else if (std::is_same<T, int64_t>::value || std::is_same<T, uint64_t>::value) {
        if (little_endian)
            bs.Write64LE(static_cast<uint64_t>(val));
        else
            bs.Write64BE(static_cast<uint64_t>(val));
    }
}

template <typename T>
void RandomPattern(
    uint32_t seed,
    T min_value,
    T max_value,
    uint8_t* buffer,
    size_t size,
    ByteOrder byte_order=ByteOrder::LittleEndian)
{

    if (!buffer || size == 0)
        return;

    std::size_t num_elems = size / sizeof(T);
    AJAByteStream bs(buffer);

    uint32_t init_seed = !seed ? std::random_device{}() : seed;
    std::default_random_engine engine(init_seed);
    std::uniform_int_distribution<T> int_dist = std::uniform_int_distribution<T>(min_value, max_value);

    for (std::size_t i = 0; i < num_elems; i++) {
        T val = int_dist(engine);
        WriteBytestream<T>(bs, val, byte_order);
    }
}

static void
Generate8Bit(
    uint32_t height,
    uint32_t stride,
    uint8_t* buffer,
    uint8_t min_val,
    uint8_t max_val)
{
    if (!buffer || height == 0 || stride == 0)
        return;

    AJAByteStream bs(buffer);
    uint8_t data = min_val;
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < stride; x++) {
            bs.Write8(data);
            data = wrap(++data, min_val, max_val);
            // bs.Write8(data);
            // data = wrap(++data, min_val, max_val);
            // bs.Write8(data);
            // data = wrap(++data, min_val, max_val);
            // bs.Write8(data);
            // data = wrap(++data, min_val, max_val);
        }
    }
}

// Generates a counting sequence test pattern for rasters formatted with most 10-bit NTV2 Pixel Formats.
static void
Generate10Bit(
    uint32_t height,
    uint32_t stride,
    uint8_t* buffer,
    uint16_t min_val,
    uint16_t max_val,
    ByteOrder byte_order=ByteOrder::LittleEndian)
{
    uint16_t data = min_val;
    AJAByteStream bs(buffer);
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < stride; x++) {
            uint32_t value = 0;
            for (uint32_t cadence = 0; cadence < 3; cadence++) {
                value |= data << (10 * cadence);
                data = wrap(++data, min_val, max_val);
            }
            if (byte_order == ByteOrder::LittleEndian)
                bs.Write32LE(value);
            else
                bs.Write32BE(value);
        }
    }
}

static void
Generate12Bit(
    uint32_t height,
    uint32_t stride,
    uint8_t* buffer,
    uint16_t min_val,
    uint16_t max_val,
    ByteOrder byte_order=ByteOrder::LittleEndian)
{
    uint16_t data = min_val;
    AJAByteStream bs(buffer);
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < stride; x++) {
            uint32_t value = 0;
            for (uint32_t cadence = 0; cadence < 2; cadence++) {
                value |= data << (12 * cadence);
                data = wrap(++data, min_val, max_val);
            }

            if (byte_order == ByteOrder::LittleEndian)
                bs.Write32LE(value);
            else
                bs.Write32BE(value);
        }
    }
}

template <typename T>
struct CompareResult {
    uint64_t offset;
    T masterVal;
    T compareVal;
    T diffVal;
};

//
// TODO: parameter to specify endianness to use when reading, and use AJAByteStream
//
template <typename T>
int CompareTestPatterns(const void* s1, const void* s2, const std::size_t size, std::vector<CompareResult<T>>& res)
{
    int status = 0;

    T* s1Buf = (T*)s1;
    T* s2Buf = (T*)s2;

    if (s1Buf && s2Buf) {
        for (uint64_t pos = 0; pos < (size / sizeof(T)); pos++) {
            T s1Val = s1Buf[pos];
            T s2Val = s2Buf[pos];
            if (s1Val != s2Val) {
                CompareResult<T> cmp = { pos, s1Val, s2Val, static_cast<T>(s1Val^s2Val) };
                res.push_back(cmp);
                status = 1;
            }
        }
    }

    return status;
}

template <typename T>
void AlternatingPattern(
    const std::vector<uint64_t>& values,
    uint32_t repeat_count,
    std::vector<uint8_t>& buffer,
    const ByteOrder& byte_order=ByteOrder::LittleEndian)
{
    if (values.empty() || buffer.empty())
        return;

    AJAByteStream bs(buffer.data());

    const std::size_t num_values = values.size();

    uint32_t num_repeats = repeat_count == 0 ? repeat_count + 1 : repeat_count;
    std::size_t num_elems = (buffer.size() / sizeof(T)) / num_values / num_repeats;

    if (num_values == 1) {
        for(std::size_t elem = 0; elem < num_elems; elem++) {
            WriteBytestream<T>(bs, values[0], byte_order);
        }
    }
    else {
        for(std::size_t elem = 0; elem < num_elems; elem++) {
            for (uint32_t vdx = 0; vdx < num_values; vdx++) {
                for (uint32_t r = 0; r < num_repeats; r++) {
                    WriteBytestream<T>(bs, static_cast<T>(values[vdx]), byte_order);
                }
            }
        }
    }
}

} // qa
