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
    using U8Line = std::vector<UByte>;
    using U16Line = std::vector<UWord>;
    using U32Line = std::vector<ULWord>;

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
    ULWord LineCount() const;
    ULWord ElemsPerLine() const;
    ULWord BytesPerElement() const;
    ULWord ComponentsPerElement() const;
    ULWord RasterPixelsPerElement() const;
    ULWord ElementsPerLine() const;

    void Reset();
    void ResetLine();
    void ResetElement();
    void ResetComponent();
    bool EndOfLines() const;
    bool EndOfElements() const;
    bool EndOfComponents() const;
    bool EndOfStream() const;
    AJAStatus ReadLine(const ULWord lineOffset = 0);
    AJAStatus ReadElement(const ULWord elemOffset = 0);
    AJAStatus ReadComponent(const ULWord compOffset = 0);
    void NextLine();
    void NextElement();
    void NextComponent();
    ULWord LineIndex() const { return (ULWord)_line_index; }
    ULWord ElemIndex() const { return (ULWord)_elem_index; }
    ULWord CompIndex() const { return (ULWord)_comp_index; }
    AJAStatus ReadComponentValues();
    AJAStatus ReadComponents();

    std::vector<UByte>& GetComponentsU8() const {
        return _u8_components;
    }

    std::vector<UWord>& GetComponentsU16() const {
        return _u16_components;
    }

    std::vector<ULWord>& GetComponentsU32() const {
        return _u32_components;
    }

    size_t WordSize() const {
        return _word_size;
    }

private:
    mutable NTV2_POINTER _framebuffer;
    mutable NTV2FormatDesc _format_desc;

    ULWord _line_index;
    ULWord _elem_index;
    ULWord _comp_index;

    std::vector<U8Line> _u8_lines;
    std::vector<U16Line> _u16_lines;
    std::vector<U32Line> _u32_lines;

    mutable std::vector<UByte> _u8_components;
    mutable std::vector<UWord> _u16_components;
    mutable std::vector<ULWord> _u32_components;

    UByte const* _planar_bytes[4];
    UWord const* _planar_words[4];
    size_t _word_size;
};

#endif
