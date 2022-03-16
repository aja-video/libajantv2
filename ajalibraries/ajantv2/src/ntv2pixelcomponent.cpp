#include "ajantv2/includes/ntv2pixelcomponent.h"
#include "ajantv2/includes/ntv2endian.h"
#include "ajantv2/includes/ntv2utils.h"

#include <algorithm>
#include <map>

struct PixelElementData {
    int32_t BytesPerElement;
    int32_t ComponentsPerElement;
    int32_t RasterPixelsPerElement;
};

typedef std::map<NTV2PixelFormat, PixelElementData> PixelFormatMetadata;

static const PixelFormatMetadata kPixelFormatMetadata = {
    { NTV2_FBF_10BIT_YCBCR,             {16, 12, 6} },
    { NTV2_FBF_8BIT_YCBCR,              {4, 4, 2} },
    { NTV2_FBF_ARGB,                    {4, 4, 1} },
    { NTV2_FBF_RGBA,                    {4, 4, 1} },
    { NTV2_FBF_10BIT_RGB,               {4, 4, 1} },
    { NTV2_FBF_8BIT_YCBCR_YUY2,         {4, 4, 2} },
    { NTV2_FBF_ABGR,                    {4, 4, 1} },
    { NTV2_FBF_10BIT_DPX,               {4, 3, 1} },
    { NTV2_FBF_10BIT_YCBCR_DPX,         {16, 12, 6} },
    { NTV2_FBF_8BIT_DVCPRO,             {0, 0, 0 } },
    { NTV2_FBF_8BIT_YCBCR_420PL3,       {1, 4, 2} },
    { NTV2_FBF_8BIT_HDV,                {0, 1, 1} },
    { NTV2_FBF_24BIT_RGB,               {3, 3, 1} },
    { NTV2_FBF_24BIT_BGR,               {3, 3, 1} },
    { NTV2_FBF_10BIT_YCBCRA,            {0, 0, 0} },
    { NTV2_FBF_10BIT_DPX_LE,            {4, 3, 1} },
    { NTV2_FBF_48BIT_RGB,               {6, 3, 1} },
    { NTV2_FBF_12BIT_RGB_PACKED,        {6, 3, 1} },
    // { NTV2_FBF_PRORES, {0, 0, 0} },
    { NTV2_FBF_PRORES_DVCPRO,           {0, 0, 0} },
    { NTV2_FBF_PRORES_HDV,              {0, 0, 0} },
    { NTV2_FBF_10BIT_RGB_PACKED,        {0, 0, 0} },
    { NTV2_FBF_10BIT_ARGB,              {0, 0, 0} },
    { NTV2_FBF_16BIT_ARGB,              {0, 0, 0} },
    { NTV2_FBF_8BIT_YCBCR_422PL3,       {1, 4, 2} },
    { NTV2_FBF_10BIT_RAW_RGB,           {0, 0, 0} },
    { NTV2_FBF_10BIT_RAW_YCBCR,         {0, 0, 0} },
    { NTV2_FBF_10BIT_YCBCR_420PL3_LE,   {1, 4, 2} },
    { NTV2_FBF_10BIT_YCBCR_422PL3_LE,   {1, 4, 2} },
    { NTV2_FBF_10BIT_YCBCR_420PL2,      {1, 1, 1} },
    { NTV2_FBF_10BIT_YCBCR_422PL2,      {1, 1, 1} },
    { NTV2_FBF_8BIT_YCBCR_420PL2,       {2, 4, 2} },
    { NTV2_FBF_8BIT_YCBCR_422PL2,       {1, 1, 1} },
};

CNTV2PixelComponentReader::CNTV2PixelComponentReader(const NTV2_POINTER& buffer, const NTV2FormatDesc& desc)
:
_framebuffer{buffer},
_format_desc{desc},
_uint8_lines{},
_uint16_lines{},
_uint32_lines{},
_uint8_components{},
_uint16_components{},
_uint32_components{},
_word_size{0}
{
}

bool CNTV2PixelComponentReader::operator==(const CNTV2PixelComponentReader& rhs) const {
    bool equals = false;

    const NTV2PixelFormat pix_fmt = (NTV2PixelFormat)_format_desc.GetPixelFormat();

    switch (pix_fmt) {
        case NTV2_FBF_8BIT_YCBCR:
        case NTV2_FBF_8BIT_YCBCR_YUY2:
        case NTV2_FBF_ARGB:
        case NTV2_FBF_RGBA:
        case NTV2_FBF_ABGR:
        case NTV2_FBF_24BIT_RGB:
        case NTV2_FBF_24BIT_BGR:
        {
            const auto& components = GetComponentsU16();
            equals = std::equal(components.begin(), components.end(), rhs.GetComponentsU16().begin());
            break;
        }
        case NTV2_FBF_10BIT_YCBCR:
        case NTV2_FBF_10BIT_YCBCR_DPX:
        case NTV2_FBF_48BIT_RGB:
        {
            const auto& components = GetComponentsU16();
            equals = std::equal(components.begin(), components.end(), rhs.GetComponentsU16().begin());
            break;
        }
        case NTV2_FBF_10BIT_RGB:
        {
            const auto& components_a = GetComponentsU16();
            const auto& components_b = rhs.GetComponentsU16();
            if (components_a.size() == components_b.size()) {
                for (std::size_t i = 0; i < components_a.size(); i+=4) {
                    uint16_t r1 = components_a[i];
                    uint16_t g1 = components_a[i+1];
                    uint16_t b1 = components_a[i+2];
                    uint16_t a1 = components_a[i+3];
                    (void)a1;
                    uint16_t r2 = components_b[i];
                    uint16_t g2 = components_b[i+1];
                    uint16_t b2 = components_b[i+2];
                    uint16_t a2 = components_b[i+3];
                    (void)a2;

                    // TODO(paulh): ask about the expected behavior with the alpha bits for RGB-10
                    // equals = (r1 == r2) && (g1 == g2) && (b1 == b2) &&
                    //     (a2 & 0x3 == 0x3);
                    equals = (r1 == r2) && (g1 == g2) && (b1 == b2);

                    if (!equals)
                        break;
                }
            }
            // equals = std::equal(components.begin(), components.end(), rhs.GetComponentsU16().begin());
            break;
        }
        case NTV2_FBF_10BIT_DPX:
        case NTV2_FBF_10BIT_DPX_LE:
        {
            const auto& components = GetComponentsU16();
            equals = std::equal(components.begin(), components.end(), rhs.GetComponentsU16().begin());
            break;
        }
        //TODO
        case NTV2_FBF_12BIT_RGB_PACKED:
        case NTV2_FBF_8BIT_YCBCR_420PL3:	//	I420
        case NTV2_FBF_8BIT_YCBCR_422PL3:	//	I422
        case NTV2_FBF_10BIT_YCBCR_420PL3_LE:
        case NTV2_FBF_10BIT_YCBCR_422PL3_LE:
        case NTV2_FBF_8BIT_YCBCR_420PL2:
        case NTV2_FBF_8BIT_DVCPRO:
        case NTV2_FBF_8BIT_HDV:
        case NTV2_FBF_10BIT_YCBCRA:
        case NTV2_FBF_PRORES_DVCPRO:
        case NTV2_FBF_PRORES_HDV:
        case NTV2_FBF_10BIT_RGB_PACKED:
        case NTV2_FBF_10BIT_ARGB:
        case NTV2_FBF_16BIT_ARGB:
        case NTV2_FBF_10BIT_RAW_RGB:
        case NTV2_FBF_10BIT_RAW_YCBCR:
        case NTV2_FBF_10BIT_YCBCR_420PL2:
        case NTV2_FBF_10BIT_YCBCR_422PL2:
        case NTV2_FBF_8BIT_YCBCR_422PL2:
        case NTV2_FBF_INVALID:
            break;
    }

    return equals;
}

bool CNTV2PixelComponentReader::operator!=(const CNTV2PixelComponentReader& rhs) const {
    return !operator==(rhs);
}

void CNTV2PixelComponentReader::SetBuffer(const NTV2_POINTER& buffer, const NTV2FormatDesc& fd)
{
    if (!_framebuffer.IsNULL())
        _framebuffer.Deallocate();
    _framebuffer.SetFrom(buffer);
    _format_desc.MakeInvalid();
    _format_desc = fd;
}

NTV2_POINTER CNTV2PixelComponentReader::GetBuffer() const
{
    return _framebuffer;
}

NTV2FormatDesc CNTV2PixelComponentReader::GetFormatDesc() const
{
    return _format_desc;
}

AJAStatus CNTV2PixelComponentReader::ReadComponentValues() {
    AJAStatus status = AJA_STATUS_SUCCESS;

    const NTV2PixelFormat pixel_format = _format_desc.GetPixelFormat();

    const uint32_t bytes_per_elem = get_bytes_per_element();
    const uint32_t components_per_elem = get_components_per_element();
    const uint32_t raster_pix_per_elem = get_raster_pixels_per_element(); // first element offset
    const uint32_t elem_per_line = get_elements_per_line();
    const uint32_t fb_size = _framebuffer.GetByteCount();
    const uint32_t total_bytes = _format_desc.GetTotalBytes();
    const uint32_t bytes_per_row = elem_per_line * bytes_per_elem;
    const uint32_t fb_num_lines = fb_size / bytes_per_row;
    
    uint8_t const* planar_bytes[4] = { nullptr, nullptr, nullptr, nullptr };
    uint16_t const* planar_words[4] = { nullptr, nullptr, nullptr, nullptr };
    static const uint32_t y_plane = 0;
    static const uint32_t cb_cr_plane = 1;
    static const uint32_t cb_plane = 1;
    static const uint32_t cr_plane = 2;

    // source file should match size of expected format desc
    if (fb_size < total_bytes)
        return AJA_STATUS_BADBUFFERSIZE;

    const uint32_t raster_height = _format_desc.GetFullRasterHeight();
    const uint32_t raster_width = _format_desc.GetFullRasterDimensions().GetWidth();
    const uint32_t num_horizontal_elems = raster_width / raster_pix_per_elem;

    std::vector<uint32_t> component_values = {};
    std::vector<uint8_t> line_uint8 = {};
    std::vector<uint16_t> line_uint16 = {};
    std::vector<uint32_t> line_uint32 = {};
    for (uint32_t line_index = 0; line_index < raster_height; line_index++) {
        const void* line_ptr = _format_desc.GetRowAddress(_framebuffer.GetHostPointer(), line_index);
        uint8_t const* img_data_uint8 = nullptr;
        uint16_t const* img_data_uint16 = nullptr;
        uint32_t const* img_data_uint32 = nullptr;
        bool unpacked = false;
        const uint32_t line_stride = _format_desc.GetBytesPerRow();
        line_uint8.clear();
        line_uint16.clear();
        line_uint32.clear();
        switch (pixel_format) {
            case NTV2_FBF_10BIT_YCBCR:
            case NTV2_FBF_10BIT_YCBCR_DPX:
            {
                unpacked = UnpackLine_10BitYUVtoUWordSequence(
                    line_ptr,
                    _format_desc,
                    line_uint16
                );
                _uint16_lines.push_back(line_uint16);
                NTV2_ASSERT(unpacked);
                if (!unpacked)
                    status = AJA_STATUS_FAIL;
                break;
            }
            case NTV2_FBF_8BIT_YCBCR:
            case NTV2_FBF_8BIT_YCBCR_YUY2:
            case NTV2_FBF_ARGB:
            case NTV2_FBF_RGBA:
            case NTV2_FBF_ABGR:
            case NTV2_FBF_24BIT_RGB:
            case NTV2_FBF_24BIT_BGR:
            {
                img_data_uint8 = reinterpret_cast<uint8_t const*>(line_ptr);
                line_uint8.assign(img_data_uint8, img_data_uint8 + line_stride);
                _uint8_lines.push_back(line_uint8);
                break;
            }
            case NTV2_FBF_48BIT_RGB:
            case NTV2_FBF_12BIT_RGB_PACKED:
            {
                img_data_uint16 = reinterpret_cast<uint16_t const*>(line_ptr);
                line_uint16.assign(img_data_uint16, img_data_uint16 + (line_stride / 2));
                _uint16_lines.push_back(line_uint16);
                break;
            }
            case NTV2_FBF_10BIT_RGB:
            case NTV2_FBF_10BIT_DPX:
            case NTV2_FBF_10BIT_DPX_LE:
            {
                img_data_uint32 = reinterpret_cast<uint32_t const*>(line_ptr);
                line_uint32.assign(img_data_uint32, img_data_uint32 + (line_stride / 4));
                _uint32_lines.push_back(line_uint32);
                break;
            }
            case NTV2_FBF_8BIT_YCBCR_420PL3: /* I420 */
            case NTV2_FBF_8BIT_YCBCR_422PL3: /* I422 */
            {
                const void* y_plane_line = _format_desc.GetRowAddress(_framebuffer.GetHostPointer(), line_index, y_plane);
                const void* cb_plane_line = _format_desc.GetRowAddress(_framebuffer.GetHostPointer(), line_index, cb_plane);
                const void* cr_plane_line = _format_desc.GetRowAddress(_framebuffer.GetHostPointer(), line_index, cr_plane);
                planar_bytes[0] = planar_bytes[2] = reinterpret_cast<uint8_t const*>(y_plane_line);
                planar_bytes[1] = reinterpret_cast<uint8_t const*>(cb_plane_line);
                planar_bytes[3] = reinterpret_cast<uint8_t const*>(cr_plane_line);
                break;
            }
            case NTV2_FBF_10BIT_YCBCR_420PL3_LE:
            case NTV2_FBF_10BIT_YCBCR_422PL3_LE:
            {
                const void* y_plane_line = _format_desc.GetRowAddress(_framebuffer.GetHostPointer(), line_index, y_plane);
                const void* cb_plane_line = _format_desc.GetRowAddress(_framebuffer.GetHostPointer(), line_index, cb_plane);
                const void* cr_plane_line = _format_desc.GetRowAddress(_framebuffer.GetHostPointer(), line_index, cr_plane);
                planar_words[0] = planar_words[2] = reinterpret_cast<uint16_t const*>(y_plane_line);
                planar_words[1] = reinterpret_cast<uint16_t const*>(cb_plane_line);
                planar_words[3] = reinterpret_cast<uint16_t const*>(cr_plane_line);
                break;
            }
            case NTV2_FBF_8BIT_YCBCR_420PL2:
            {
                break;
            }
            case NTV2_FBF_8BIT_DVCPRO:
            case NTV2_FBF_8BIT_HDV:
            case NTV2_FBF_10BIT_YCBCRA:
            case NTV2_FBF_PRORES_DVCPRO:
            case NTV2_FBF_PRORES_HDV:
            case NTV2_FBF_10BIT_RGB_PACKED:
            case NTV2_FBF_10BIT_ARGB:
            case NTV2_FBF_16BIT_ARGB:
            case NTV2_FBF_10BIT_RAW_RGB:
            case NTV2_FBF_10BIT_RAW_YCBCR:
            case NTV2_FBF_10BIT_YCBCR_420PL2:
            case NTV2_FBF_10BIT_YCBCR_422PL2:
            case NTV2_FBF_8BIT_YCBCR_422PL2:
            case NTV2_FBF_INVALID:
                std::cout << NTV2FrameBufferFormatToString(pixel_format) << " not yet implemented in CNTV2PixelComponentReader!" << std::endl;
                status = AJA_STATUS_FAIL;
                break;
        }

        // Read each pixel
        for (uint32_t elem_index = 0; elem_index < num_horizontal_elems; elem_index++) {
            // Read each pixel component
            for (uint32_t component_index = 0; component_index < components_per_elem; component_index++) {
                switch (pixel_format) {
                    // YUV 10-bit
                    case NTV2_FBF_10BIT_YCBCR:
                    case NTV2_FBF_10BIT_YCBCR_DPX:
                    {
                        const std::size_t uint_index = static_cast<std::size_t>((components_per_elem) * elem_index + component_index);
                        const uint16_t cmp_value = line_uint16[uint_index];
                        _uint16_components.push_back(cmp_value);
                        _word_size = 16;
                        break;
                    }
                    // RGB/YUV 8-bit packed
                    case NTV2_FBF_8BIT_YCBCR:
                    case NTV2_FBF_8BIT_YCBCR_YUY2:
                    case NTV2_FBF_ARGB:
                    case NTV2_FBF_RGBA:
                    case NTV2_FBF_ABGR:
                    {
                        const std::size_t byte_index = static_cast<std::size_t>(
                            (components_per_elem) * elem_index + component_index
                        );
                        const uint16_t component_value = line_uint8[byte_index];
                        _uint16_components.push_back(component_value);
                        _word_size = 16;
                        break;
                    }
                    case NTV2_FBF_10BIT_RGB:
                    {
                        NTV2_ASSERT(component_index < 4);
                        static const uint32_t masks[]  = { 0x3ff, 0x3ff, 0x3ff, 0x003, 0x000 };
                        static const uint32_t shifts[] = {     0,    10,    20,    30,     0 };
                        const uint32_t word = line_uint32[elem_index];
                        const uint16_t component_value = static_cast<uint16_t>(
                            (word >> shifts[component_index]) & masks[component_index]
                        );
                        _uint16_components.push_back(component_value);
                        _word_size = 16;
                        break;
                    }
                    case NTV2_FBF_10BIT_DPX:
                    {
                        NTV2_ASSERT(component_index < 3);

                        static const uint32_t masks[] =  { 0x00000FFC, 0x003FF000, 0xFFC00000, 0 }; // BGR
                        static const uint32_t shifts[] = {          2,         12,         22, 0 };
                        const uint32_t word = NTV2EndianSwap32(line_uint32[elem_index]);
                        const uint16_t component_value = static_cast<uint16_t>(
                            ((word & masks[component_index]) >> shifts[component_index])
                        );
                        _uint16_components.push_back(component_value);
                        _word_size = 16;
                        break;
                    }
                    case NTV2_FBF_10BIT_DPX_LE:
                    {
                        NTV2_ASSERT(component_index < 3);
                        static const uint32_t masks[]  = { 0x00000FFC, 0x003FF000, 0xFFC00000, 0 };
                        static const uint32_t shifts[] = {          2,         12,         22, 0 };
                        const uint32_t word = line_uint32[elem_index];
                        const uint16_t component_value = static_cast<uint16_t>(
                            ((word & masks[component_index]) >> shifts[component_index])
                        );
                        _uint16_components.push_back(component_value);
                        _word_size = 16;
                        break;
                    }
                    case NTV2_FBF_24BIT_RGB:
                    case NTV2_FBF_24BIT_BGR:
                    {
                        NTV2_ASSERT(component_index < 3);
                        // Component 0 is B, 1 is G, 2 is R
                        const std::size_t byte_index = static_cast<std::size_t>(
                            (components_per_elem) * elem_index + component_index
                        );
                        const uint16_t component_value = line_uint8[byte_index];
                        _uint16_components.push_back(component_value);
                        _word_size = 16;
                        break;
                    }
                    case NTV2_FBF_48BIT_RGB:
                    case NTV2_FBF_12BIT_RGB_PACKED:
                    {
                        NTV2_ASSERT(component_index < 3);
                        const std::size_t word_index = static_cast<std::size_t>(
                            (components_per_elem) * elem_index + component_index
                        );
                        const uint16_t component_value = line_uint16[word_index];
                        _uint16_components.push_back(component_value);
                        _word_size = 16;
                        break;
                    }
                    case NTV2_FBF_8BIT_YCBCR_420PL3:
                    case NTV2_FBF_8BIT_YCBCR_422PL3:
                    case NTV2_FBF_10BIT_YCBCR_420PL3_LE:
                    case NTV2_FBF_10BIT_YCBCR_422PL3_LE:
                    case NTV2_FBF_8BIT_YCBCR_420PL2:
                    case NTV2_FBF_8BIT_DVCPRO:
                    case NTV2_FBF_8BIT_HDV:
                    case NTV2_FBF_10BIT_YCBCRA:
                    case NTV2_FBF_PRORES_DVCPRO:
                    case NTV2_FBF_PRORES_HDV:
                    case NTV2_FBF_10BIT_RGB_PACKED:
                    case NTV2_FBF_10BIT_ARGB:
                    case NTV2_FBF_16BIT_ARGB:
                    case NTV2_FBF_10BIT_RAW_RGB:
                    case NTV2_FBF_10BIT_RAW_YCBCR:
                    case NTV2_FBF_10BIT_YCBCR_420PL2:
                    case NTV2_FBF_10BIT_YCBCR_422PL2:
                    case NTV2_FBF_8BIT_YCBCR_422PL2:
                    case NTV2_FBF_INVALID:
                        std::cout << NTV2FrameBufferFormatToString(pixel_format) << " not yet implemented in CNTV2PixelComponentReader!" << std::endl;
                        status = AJA_STATUS_FAIL;
                        break;
                }
            }
        }
    }

    return status;
}

//	Component views shows full raster (including VANC)
int32_t CNTV2PixelComponentReader::get_line_count() const {
    return static_cast<int32_t>(_format_desc.GetFullRasterHeight());
}

// Return number of bytes in one line element of this framebuffer
int32_t CNTV2PixelComponentReader::get_bytes_per_element() const {
    if (!_format_desc.IsValid())
        return 0;

    const NTV2PixelFormat& pix_fmt = _format_desc.GetPixelFormat();

    return kPixelFormatMetadata.at(pix_fmt).BytesPerElement;
}

//	Return number of components (e.g., R/G/B or Y/Cb/Cr) in one line element of this framebuffer
int32_t CNTV2PixelComponentReader::get_components_per_element() const {
    if (!_format_desc.IsValid())
        return 0;

    const NTV2PixelFormat& pix_fmt = _format_desc.GetPixelFormat();

    return kPixelFormatMetadata.at(pix_fmt).ComponentsPerElement;
}

//	Return number of raster pixels are in one line element in this framebuffer
int32_t CNTV2PixelComponentReader::get_raster_pixels_per_element() const {
    if (!_format_desc.IsValid())
        return 0;

    const NTV2PixelFormat& pix_fmt = _format_desc.GetPixelFormat();

    return kPixelFormatMetadata.at(pix_fmt).RasterPixelsPerElement;
}

int32_t CNTV2PixelComponentReader::get_elements_per_line() const
{
    if (!_format_desc.IsValid())
        return 0;

    // 3-plane planar
    if (_format_desc.GetNumPlanes() == 3)
        //	2 horizontal pixels per element (each element has 4 components: Y|Cb|Y|Cr)
        return static_cast<int32_t>(_format_desc.GetRasterWidth()) / get_raster_pixels_per_element();

    return get_bytes_per_element() ? static_cast<int32_t>(_format_desc.GetBytesPerRow()) / get_bytes_per_element() : 0;
}
