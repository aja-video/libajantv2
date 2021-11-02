#pragma once

#include <cstdint>
#include <random>
#include <type_traits>
#include <vector>

#include "ajabase/common/bytestream.h"

enum class ByteOrder {
    LittleEndian = 0,
    BigEndian = 1,
    Network = BigEndian,
};

namespace qa {

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

} // qa
