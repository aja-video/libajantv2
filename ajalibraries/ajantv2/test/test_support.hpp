#pragma once

#include <nlohmann/json.hpp>
#include "ajabase/system/file_io.h"

using nlohmann::json;

static inline AJAStatus read_json_file(const std::string& path, json& j)
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
