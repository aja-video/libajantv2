/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2componentreader.h
	@brief		Declares the NTV2PixelComponent structure
	@copyright	(C) 2005-2022 AJA Video Systems, Inc.
**/

#ifndef NTV2PIXELCOMPONENT_H
#define NTV2PIXELCOMPONENT_H

#include "ajabase/common/types.h"

#include "ajantv2/includes/ntv2enums.h"
#include "ajantv2/includes/ntv2formatdescriptor.h"
#include "ajantv2/includes/ntv2publicinterface.h"

#include <cstdint>
#include <vector>

class CNTV2PixelComponentReader {
public:
    explicit CNTV2PixelComponentReader(
        const NTV2_POINTER& buffer,
        const NTV2FormatDesc& fd);

    ~CNTV2PixelComponentReader() = default;
    bool operator==(const CNTV2PixelComponentReader& rhs) const;
    bool operator!=(const CNTV2PixelComponentReader& rhs) const;

    void SetBuffer(
        const NTV2_POINTER& buffer,
        const NTV2FormatDesc& fd);
    
    NTV2_POINTER GetBuffer() const;
    NTV2FormatDesc GetFormatDesc() const;

    AJAStatus ReadComponentValues();
    
    std::vector<uint8_t>& GetComponentsU8() const {
        return _uint8_components;
    }

    std::vector<uint16_t>& GetComponentsU16() const {
        return _uint16_components;
    }

    std::vector<uint32_t>& GetComponentsU32() const {
        return _uint32_components;
    }

    std::size_t WordSize() const {
        return _word_size;
    }

private:
    int32_t get_line_count() const;
    int32_t get_bytes_per_element() const;
    int32_t get_components_per_element() const;
    int32_t get_raster_pixels_per_element() const;
    int32_t get_elements_per_line() const;

private:
    mutable NTV2_POINTER _framebuffer;
    mutable NTV2FormatDesc _format_desc;
    
    std::vector<std::vector<uint8_t>> _uint8_lines;
    std::vector<std::vector<uint16_t>> _uint16_lines;
    std::vector<std::vector<uint32_t>> _uint32_lines;
    
    mutable std::vector<uint8_t> _uint8_components;
    mutable std::vector<uint16_t> _uint16_components;
    mutable std::vector<uint32_t> _uint32_components;

    std::size_t _word_size;
};

#endif
