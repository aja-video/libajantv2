#pragma once

#include "nlohmann/json.hpp"
#include "ajabase/system/file_io.h"
#include "ajantv2/includes/ntv2enums.h"

using nlohmann::json;

#ifdef USE_PNG
#include <stdlib.h>
#include <png.h>
#include <zlib.h>
#endif

namespace TestSupport {

#ifdef USE_PNG
struct libpng_inmem_write_struct { /* This is from png.c */
  unsigned char *data;  /* destination memory */
  unsigned long size;  /* destination memory size (bytes) */
};

void write_png_mem(png_structp png_write, png_bytep data, png_size_t length) {
    struct libpng_inmem_write_struct *p =
        (struct libpng_inmem_write_struct*)png_get_io_ptr(png_write);
    p->data = (unsigned char*)realloc(p->data, p->size + length);
    if (!p->data)
        return;
    memmove(p->data + p->size, data, length);
    p->size += (unsigned long)length;
}

bool SavePNG(const std::string& path, const uint8_t* data, uint32_t width, uint32_t height, uint32_t bpp) {
    png_byte color_type = PNG_COLOR_TYPE_GRAY;
    uint32_t bits_per_chan = 8;
    if (bpp == 32 || bpp == 64) { /* 16-bit channels */
        color_type = PNG_COLOR_TYPE_RGB_ALPHA;
        bits_per_chan = bpp / 4;
    } else if (bpp == 24 || bpp == 48) { /* 8-bit channels */
        color_type = PNG_COLOR_TYPE_RGB;
        bits_per_chan = bpp / 3;
    } else {
        return false;
    }

    uint32_t stride = width * (bpp / 8);

    AJAFileIO f;
    if (f.Open(path, eAJAWriteOnly|eAJACreateAlways, 0) != AJA_STATUS_SUCCESS) {
        return false;
    }

    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
        NULL, NULL, NULL);
    if (png_ptr == NULL)
        return false;
    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL)
        return false;
    if (setjmp(png_jmpbuf(png_ptr)))
        return false;

    libpng_inmem_write_struct png_mem;
    png_mem.data = (unsigned char *)malloc(4);
    png_mem.size = 4;
    png_init_io(png_ptr, (png_FILE_p)&png_mem);
    png_set_write_fn(png_ptr, &png_mem, write_png_mem, NULL);
    png_set_IHDR(png_ptr, info_ptr, width, height,
                 bits_per_chan, color_type, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
    png_write_info(png_ptr, info_ptr);
    std::vector<uint8_t> row(png_get_rowbytes(png_ptr, info_ptr));
    for (uint32_t y = 0; y < height; y++) {
        memcpy(&row[0], data + stride * y, stride);
        png_write_row(png_ptr, row.data());
    }
    png_write_end(png_ptr, NULL);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    uint32_t bytes_written = f.Write(png_mem.data + 4, png_mem.size);
    free(png_mem.data);
    png_mem.data = NULL;
    return (bytes_written == png_mem.size
        && f.Close() == AJA_STATUS_SUCCESS);
}
#endif

static inline bool ReadJsonFile(const std::string& path, json& j)
{
    AJAFileIO f;
    if (f.Open(path, eAJAReadOnly, 0) == AJA_STATUS_SUCCESS) {
        int64_t create_time, mod_time, file_size;
        if (f.FileInfo(create_time, mod_time, file_size) == AJA_STATUS_SUCCESS) {
            std::string json_str;
            if (f.Read(json_str, (uint32_t)file_size) == file_size) {
                j = json::parse(json_str);
                return true;
            }
        }
    }
    return false;
}

static inline bool WriteJsonFile(const std::string& path, const json& j)
{
    AJAFileIO f;
    return (
        f.Open(path, eAJAWriteOnly, 0) == AJA_STATUS_SUCCESS &&
        f.Write(j.dump()) > 0
    );
}

static inline ULWord MakeVPIDWord(UByte byte1, UByte byte2, UByte byte3, UByte byte4)
{
    return (byte1 << 24 | byte2 << 16 | byte3 << 8 | byte4);
}

static inline bool IsKona5(NTV2DeviceID id)
{
    return (id == DEVICE_ID_KONA5
        || id == DEVICE_ID_KONA5_8KMK
        || id == DEVICE_ID_KONA5_8K
        || id == DEVICE_ID_KONA5_2X4K
        || id == DEVICE_ID_KONA5_3DLUT
        || id == DEVICE_ID_KONA5_OE1
        || id == DEVICE_ID_KONA5_OE2
        || id == DEVICE_ID_KONA5_OE3
        || id == DEVICE_ID_KONA5_OE4
        || id == DEVICE_ID_KONA5_OE5
        || id == DEVICE_ID_KONA5_OE6
        || id == DEVICE_ID_KONA5_OE7
        || id == DEVICE_ID_KONA5_OE8
        || id == DEVICE_ID_KONA5_OE9
        || id == DEVICE_ID_KONA5_OE10
        || id == DEVICE_ID_KONA5_OE11
        || id == DEVICE_ID_KONA5_OE12
        || id == DEVICE_ID_KONA5_8K_MV_TX);
}

static inline bool IsCorvid4412G(NTV2DeviceID id)
{
    return (id == DEVICE_ID_CORVID44_8KMK
        || id == DEVICE_ID_CORVID44_8K
        || id == DEVICE_ID_CORVID44_2X4K
        || id == DEVICE_ID_CORVID44_PLNR);
}

static inline bool HasRenesasGenlock(NTV2DeviceID devId, int8_t pcbId)
{
    return ((IsKona5(devId) || IsCorvid4412G(devId))
        && pcbId == 0xA
    );
}

} // test_support
