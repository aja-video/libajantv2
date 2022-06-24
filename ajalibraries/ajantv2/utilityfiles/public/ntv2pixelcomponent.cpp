#include "ntv2pixelcomponent.h"
#include "ajantv2/includes/ntv2endian.h"
#include "ajantv2/includes/ntv2utils.h"

#include <algorithm>
#include <map>

#define PLANE_Y 0
#define PLANE_CB_CR 1
#define PLANE_CB 1
#define PLANE_CR 2

struct PixelElementData {
    LWord BytesPerElement;
    LWord ComponentsPerElement;
    LWord RasterPixelsPerElement;
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
_line_index{0},
_elem_index{0},
_comp_index{0},
_u8_lines{},
_u16_lines{},
_u32_lines{},
_u8_components{},
_u16_components{},
_u32_components{},
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
                for (size_t i = 0; i < components_a.size(); i+=4) {
                    UWord r1 = components_a[i];
                    UWord g1 = components_a[i+1];
                    UWord b1 = components_a[i+2];
                    UWord a1 = components_a[i+3];
                    (void)a1;
                    UWord r2 = components_b[i];
                    UWord g2 = components_b[i+1];
                    UWord b2 = components_b[i+2];
                    UWord a2 = components_b[i+3];
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

//	Component views shows full raster (including VANC)
ULWord CNTV2PixelComponentReader::LineCount() const {
    return static_cast<LWord>(_format_desc.GetFullRasterHeight());
}

ULWord CNTV2PixelComponentReader::ElemsPerLine() const
{
    ULWord raster_pix_per_elem = RasterPixelsPerElement();
    if (raster_pix_per_elem == 0)
        return 0;
    return _format_desc.GetRasterWidth() / raster_pix_per_elem;
}

// Return number of bytes in one line element of this framebuffer
ULWord CNTV2PixelComponentReader::BytesPerElement() const {
    if (!_format_desc.IsValid())
        return 0;
    const NTV2PixelFormat& pix_fmt = _format_desc.GetPixelFormat();
    return kPixelFormatMetadata.at(pix_fmt).BytesPerElement;
}

//	Return number of components (e.g., R/G/B or Y/Cb/Cr) in one line element of this framebuffer
ULWord CNTV2PixelComponentReader::ComponentsPerElement() const {
    if (!_format_desc.IsValid())
        return 0;
    const NTV2PixelFormat& pix_fmt = _format_desc.GetPixelFormat();
    return kPixelFormatMetadata.at(pix_fmt).ComponentsPerElement;
}

//	Return number of raster pixels are in one line element in this framebuffer
ULWord CNTV2PixelComponentReader::RasterPixelsPerElement() const {
    if (!_format_desc.IsValid())
        return 0;
    const NTV2PixelFormat& pix_fmt = _format_desc.GetPixelFormat();
    return kPixelFormatMetadata.at(pix_fmt).RasterPixelsPerElement;
}

ULWord CNTV2PixelComponentReader::ElementsPerLine() const
{
    if (!_format_desc.IsValid())
        return 0;
    // 3-plane planar
    if (_format_desc.GetNumPlanes() == 3)
        //	2 horizontal pixels per element (each element has 4 components: Y|Cb|Y|Cr)
        return static_cast<LWord>(_format_desc.GetRasterWidth()) / RasterPixelsPerElement();
    return _format_desc.GetRasterWidth() / RasterPixelsPerElement();
    // ULWord bytes_per_elem = BytesPerElement();
    // return bytes_per_elem ? static_cast<LWord>(_format_desc.GetBytesPerRow()) / bytes_per_elem : 0;
}


void CNTV2PixelComponentReader::Reset()
{
    _line_index = 0;
    _elem_index = 0;
    _comp_index = 0;
    _u8_lines.clear();
    _u16_lines.clear();
    _u32_lines.clear();
    _u8_components.clear();
    _u16_components.clear();
    _u32_components.clear();
}

void CNTV2PixelComponentReader::ResetLine()
{
    _line_index = 0;
}

void CNTV2PixelComponentReader::ResetElement()
{
    _elem_index = 0;
}

void CNTV2PixelComponentReader::ResetComponent()
{
    _comp_index = 0;
}


bool CNTV2PixelComponentReader::EndOfLines() const
{
    return _line_index >= _format_desc.GetRasterHeight();
}

bool CNTV2PixelComponentReader::EndOfElements() const
{
//	ULWord elemsPerLine = ElemsPerLine();
//	ULWord elementsPerLine = ElementsPerLine();
	return _elem_index >= ElementsPerLine();
}

bool CNTV2PixelComponentReader::EndOfComponents() const
{
//	ULWord compsPerElem = ComponentsPerElement();
	return _comp_index >= ComponentsPerElement();
}

bool CNTV2PixelComponentReader::EndOfStream() const
{
    return (EndOfLines() && EndOfElements() && EndOfComponents());
}

AJAStatus CNTV2PixelComponentReader::ReadLine(const ULWord lineOffset)
{
	if (_line_index < lineOffset)
		_line_index = lineOffset;
	if (EndOfLines())
		return AJA_STATUS_NOSTREAM;

	AJAStatus status = AJA_STATUS_SUCCESS;

	const ULWord fb_size = _framebuffer.GetByteCount();
	const NTV2PixelFormat pixel_format = _format_desc.GetPixelFormat();
//	const ULWord raster_height = _format_desc.GetRasterHeight();
//	const ULWord raster_width = _format_desc.GetRasterWidth();
	const ULWord total_bytes = _format_desc.GetTotalBytes();
	const ULWord line_stride = _format_desc.GetBytesPerRow();
	if (pixel_format == NTV2_FBF_INVALID)
		return AJA_STATUS_UNSUPPORTED;
	// source file should match size of expected format desc
	if (fb_size < total_bytes)
		return AJA_STATUS_BADBUFFERSIZE;

	// const ULWord bytes_per_elem = get_bytes_per_element();
	// const ULWord components_per_elem = ComponentsPerElement();
	// const ULWord raster_pix_per_elem = RasterPixelsPerElement(); // first element offset
	// const ULWord elem_per_line = get_elements_per_line();
	// const ULWord bytes_per_row = elem_per_line * bytes_per_elem;
	// const ULWord fb_num_lines = fb_size / bytes_per_row;
	// const ULWord num_horizontal_elems = raster_width / raster_pix_per_elem;

	std::vector<ULWord> component_values = {};
	U8Line line_u8 = {};
	U16Line line_u16 = {};
	U32Line line_u32 = {};

	const void* line_ptr = _format_desc.GetRowAddress(_framebuffer.GetHostPointer(), _line_index);
	UByte const* img_data_u8 = nullptr;
	UWord const* img_data_u16 = nullptr;
	ULWord const* img_data_u32 = nullptr;
	bool unpacked = false;

	line_u8.clear();
	line_u16.clear();
	line_u32.clear();
	switch (pixel_format) {
		case NTV2_FBF_10BIT_YCBCR:
		case NTV2_FBF_10BIT_YCBCR_DPX:
		{
			unpacked = UnpackLine_10BitYUVtoUWordSequence(
				line_ptr,
				_format_desc,
				line_u16
			);
			_u16_lines.push_back(line_u16);
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
			img_data_u8 = reinterpret_cast<UByte const*>(line_ptr);
			line_u8.assign(img_data_u8, img_data_u8 + line_stride);
			_u8_lines.push_back(line_u8);
			break;
		}
		case NTV2_FBF_48BIT_RGB:
		case NTV2_FBF_12BIT_RGB_PACKED:
		{
			img_data_u16 = reinterpret_cast<UWord const*>(line_ptr);
			line_u16.assign(img_data_u16, img_data_u16 + (line_stride / 2));
			_u16_lines.push_back(line_u16);
			break;
		}
		case NTV2_FBF_10BIT_RGB:
		case NTV2_FBF_10BIT_DPX:
		case NTV2_FBF_10BIT_DPX_LE:
		{
			img_data_u32 = reinterpret_cast<ULWord const*>(line_ptr);
			line_u32.assign(img_data_u32, img_data_u32 + (line_stride / 4));
			_u32_lines.push_back(line_u32);
			break;
		}
		case NTV2_FBF_8BIT_YCBCR_420PL3: /* I420 */
		case NTV2_FBF_8BIT_YCBCR_422PL3: /* I422 */
		{
			const void* y_plane_line = _format_desc.GetRowAddress(_framebuffer.GetHostPointer(),  _line_index, PLANE_Y);
			const void* cb_plane_line = _format_desc.GetRowAddress(_framebuffer.GetHostPointer(), _line_index, PLANE_CB);
			const void* cr_plane_line = _format_desc.GetRowAddress(_framebuffer.GetHostPointer(), _line_index, PLANE_CR);
			_planar_bytes[0] = _planar_bytes[2] = reinterpret_cast<UByte const*>(y_plane_line);
			_planar_bytes[1] = reinterpret_cast<UByte const*>(cb_plane_line);
			_planar_bytes[3] = reinterpret_cast<UByte const*>(cr_plane_line);
			break;
		}
		case NTV2_FBF_10BIT_YCBCR_420PL3_LE:
		case NTV2_FBF_10BIT_YCBCR_422PL3_LE:
		{
			const void* y_plane_line = _format_desc.GetRowAddress(_framebuffer.GetHostPointer(),  _line_index, PLANE_Y);
			const void* cb_plane_line = _format_desc.GetRowAddress(_framebuffer.GetHostPointer(), _line_index, PLANE_CB);
			const void* cr_plane_line = _format_desc.GetRowAddress(_framebuffer.GetHostPointer(), _line_index, PLANE_CR);
			_planar_words[0] = _planar_words[2] = reinterpret_cast<UWord const*>(y_plane_line);
			_planar_words[1] = reinterpret_cast<UWord const*>(cb_plane_line);
			_planar_words[3] = reinterpret_cast<UWord const*>(cr_plane_line);
			break;
		}
		case NTV2_FBF_8BIT_YCBCR_420PL2:
		{
			const void* y_plane_line = _format_desc.GetRowAddress(_framebuffer.GetHostPointer(), _line_index, PLANE_Y);
			const void* cb_cr_line = _format_desc.GetRowAddress(_framebuffer.GetHostPointer(),   _line_index, PLANE_CB_CR);
			_planar_bytes[PLANE_Y] = reinterpret_cast<UByte const*>(y_plane_line);
			_planar_bytes[PLANE_CB_CR] = reinterpret_cast<UByte const*>(cb_cr_line);
			break;
		}
		case NTV2_FBF_10BIT_YCBCR_420PL2:
		case NTV2_FBF_10BIT_YCBCR_422PL2:
		{
			const int c_line_offset = (pixel_format == NTV2_FBF_10BIT_YCBCR_422PL2 ? _line_index/2 : _line_index);
			const void* y_plane_line = _format_desc.GetRowAddress(_framebuffer.GetHostPointer(), _line_index, PLANE_Y);
			const void* cb_cr_line = _format_desc.GetRowAddress(_framebuffer.GetHostPointer(),   c_line_offset, PLANE_CB_CR);
			_planar_bytes[PLANE_Y] = reinterpret_cast<UByte const*>(y_plane_line);
			_planar_bytes[PLANE_CB_CR] = reinterpret_cast<UByte const*>(cb_cr_line);
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
		case NTV2_FBF_8BIT_YCBCR_422PL2:
		case NTV2_FBF_INVALID:
			std::cerr << NTV2FrameBufferFormatToString(pixel_format) << " not yet implemented in CNTV2PixelComponentReader!" << std::endl;
			status = AJA_STATUS_FAIL;
			break;
	}
	return status;
}

AJAStatus CNTV2PixelComponentReader::ReadElement(const ULWord elemOffset)
{
    if (_elem_index < elemOffset)
        _elem_index = elemOffset;
    if (EndOfElements())
        return AJA_STATUS_NOSTREAM;
    AJAStatus status = AJA_STATUS_SUCCESS;
    return status;
}

AJAStatus CNTV2PixelComponentReader::ReadComponent(const ULWord compOffset)
{
	if (_comp_index < compOffset)
		_comp_index = compOffset;
	if (EndOfComponents())
		return AJA_STATUS_NOSTREAM;
	AJAStatus status = AJA_STATUS_SUCCESS;

//	const ULWord fb_size = _framebuffer.GetByteCount();
//	const ULWord raster_height = _format_desc.GetRasterHeight();
//	const ULWord raster_width = _format_desc.GetRasterWidth();
//	const ULWord total_bytes = _format_desc.GetTotalBytes();
//	const ULWord line_stride = _format_desc.GetBytesPerRow();
	const NTV2PixelFormat pixel_format = _format_desc.GetPixelFormat();
	if (pixel_format == NTV2_FBF_INVALID)
		return AJA_STATUS_UNSUPPORTED;

	ULWord components_per_elem = ComponentsPerElement();
	switch (pixel_format) {
		// YUV 10-bit
		case NTV2_FBF_10BIT_YCBCR:
		case NTV2_FBF_10BIT_YCBCR_DPX:
		{
			const size_t uint_index = static_cast<size_t>((components_per_elem) * _elem_index + _comp_index);
			const U16Line& line_u16 = _u16_lines.at(_line_index);
			const UWord cmp_value = line_u16[uint_index];
			_u16_components.push_back(cmp_value);
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
			const size_t byte_index = static_cast<size_t>(
				(components_per_elem) * _elem_index + _comp_index
			);
			const U8Line& line_u8 = _u8_lines.at(_line_index);
			const UWord component_value = line_u8[byte_index];
			_u16_components.push_back(component_value);
			_word_size = 16;
			break;
		}
		case NTV2_FBF_10BIT_RGB:
		{
			NTV2_ASSERT(_comp_index < 4);
			static const ULWord masks[]  = { 0x3ff, 0x3ff, 0x3ff, 0x003, 0x000 };
			static const ULWord shifts[] = {     0,    10,    20,    30,     0 };
			const U32Line& line_u32 = _u32_lines.at(_line_index);
			const ULWord word = line_u32[_elem_index];
			const UWord component_value = static_cast<UWord>(
				(word >> shifts[_comp_index]) & masks[_comp_index]
			);
			_u16_components.push_back(component_value);
			_word_size = 16;
			break;
		}
		case NTV2_FBF_10BIT_DPX:
		{
			NTV2_ASSERT(_comp_index < 3);
			static const ULWord masks[] =  { 0x00000FFC, 0x003FF000, 0xFFC00000, 0 }; // BGR
			static const ULWord shifts[] = {          2,         12,         22, 0 };
			const U32Line& line_u32 = _u32_lines.at(_line_index);
			const ULWord word = NTV2EndianSwap32(line_u32[_elem_index]);
			const UWord component_value = static_cast<UWord>(
				((word & masks[_comp_index]) >> shifts[_comp_index])
			);
			_u16_components.push_back(component_value);
			_word_size = 16;
			break;
		}
		case NTV2_FBF_10BIT_DPX_LE:
		{
			NTV2_ASSERT(_comp_index < 3);
			static const ULWord masks[]  = { 0x00000FFC, 0x003FF000, 0xFFC00000, 0 };
			static const ULWord shifts[] = {          2,         12,         22, 0 };
			const U32Line& line_u32 = _u32_lines.at(_line_index);
			const ULWord word = line_u32[_elem_index];
			const UWord component_value = static_cast<UWord>(
				((word & masks[_comp_index]) >> shifts[_comp_index])
			);
			_u16_components.push_back(component_value);
			_word_size = 16;
			break;
		}
		case NTV2_FBF_24BIT_RGB:
		case NTV2_FBF_24BIT_BGR:
		{
			NTV2_ASSERT(_comp_index < 3);
			// Component 0 is B, 1 is G, 2 is R
			const size_t byte_index = static_cast<size_t>(
				(components_per_elem) * _elem_index + _comp_index
			);
			const U8Line& line_u8 = _u8_lines.at(_line_index);
			const UWord component_value = line_u8[byte_index];
			_u16_components.push_back(component_value);
			_word_size = 16;
			break;
		}
		case NTV2_FBF_48BIT_RGB:
		case NTV2_FBF_12BIT_RGB_PACKED:
		{
			NTV2_ASSERT(_comp_index < 3);
			const size_t word_index = static_cast<size_t>(
				(components_per_elem) * _elem_index + _comp_index
			);
			const U16Line& line_u16 = _u16_lines.at(_line_index);
			const UWord component_value = line_u16[word_index];
			_u16_components.push_back(component_value);
			_word_size = 16;
			break;
		}
		case NTV2_FBF_8BIT_YCBCR_420PL3:
		case NTV2_FBF_8BIT_YCBCR_422PL3:
		{
			ULWord byte_offset = (_comp_index == 0 ? _elem_index * 2 : (_comp_index == 2 ? _elem_index * 2 + 1 : _elem_index / 2));
			UWord component_value = _planar_bytes[_comp_index][byte_offset];
			_u8_components.push_back((UByte)component_value);
			break;
		}
		case NTV2_FBF_10BIT_YCBCR_420PL3_LE:
		case NTV2_FBF_10BIT_YCBCR_422PL3_LE:
		{
			ULWord word_offset = (_comp_index == 0 ? _elem_index : (_comp_index == 2 ? _elem_index + 1 : _elem_index / 4));
			UWord component_value = _planar_words[_comp_index][word_offset] & 0x3FF;
			_u16_components.push_back(component_value);
			break;
		}
		case NTV2_FBF_8BIT_YCBCR_420PL2:
		{
			UWord plane_index = ((_comp_index & 1) != 0) ? PLANE_CB_CR : PLANE_Y;
			ULWord byte_offset = (_comp_index > 1 ? (_elem_index * 2 + 1) : _elem_index * 2);
			UWord component_value = _planar_bytes[plane_index][byte_offset];
			_u8_components.push_back((UByte)component_value);
			break;
		}
		case NTV2_FBF_10BIT_YCBCR_420PL2:
		case NTV2_FBF_10BIT_YCBCR_422PL2:
		{
			static const UWord	msbShifts []	= {   8,    8,    6,    6,    4,    4,    2,    2};
			static const UByte	msbMasks []		= {0x03, 0x03, 0x0F, 0x0F, 0x3F, 0x3F, 0xFF, 0xFF};
			static const UWord	lsbShifts []	= {   0,    0,    2,    2,    4,    4,    6,    6};
			static const UByte	lsbMasks []		= {0xFF, 0xFF, 0xFC, 0xFC, 0xF0, 0xF0, 0xC0, 0xFF};
			static const int	offsets []		= {   0,    0,    1,    1,    2,    2,    3,    3};
			NTV2_ASSERT(compOffset < 8);
			const UWord plane = (compOffset & 1) ? PLANE_CB_CR : PLANE_Y;
			const int byteOffset = offsets[compOffset];
			const UByte msbMask = msbMasks[compOffset];
			const UByte lsbMask = lsbMasks[compOffset];
			const UWord msbShift = msbShifts[compOffset];
			const UWord lsbShift = lsbShifts[compOffset];
			_u16_components.push_back(UWord(UWord(_planar_bytes[plane][byteOffset+1] & msbMask) << msbShift)
										|  UWord(UWord(_planar_bytes[plane][byteOffset] & lsbMask) >> lsbShift));
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
		case NTV2_FBF_8BIT_YCBCR_422PL2:
		case NTV2_FBF_INVALID:
			std::cout << NTV2FrameBufferFormatToString(pixel_format) << " not yet implemented in CNTV2PixelComponentReader!" << std::endl;
			status = AJA_STATUS_FAIL;
			break;
	}
	return status;
}

void CNTV2PixelComponentReader::NextLine()
{
    _line_index++;
}
void CNTV2PixelComponentReader::NextElement()
{
    _elem_index++;
}
void CNTV2PixelComponentReader::NextComponent()
{
    _comp_index++;
}

AJAStatus CNTV2PixelComponentReader::ReadComponents()
{
    // while (!EndOfLines()) {
    //     ReadLine();
    //     ResetElement();
    //     while (!EndOfElements()) {
    //         ReadElement();
    //         ResetComponent();
    //         while (!EndOfComponents()) {
    //             ReadComponent();
    //             // std::cout << "Comp: " << CompIndex() << std::endl;
    //             NextComponent();
    //         }
    //         // std::cout << "Elem: " << ElemIndex() << std::endl;
    //         NextElement();
    //     }
    //     // std::cout << "Line: " << LineIndex() << std::endl;
    //     NextLine();
    // }
    Reset();
    for (ResetLine(); !EndOfLines(); NextLine()) {
        ReadLine();
        for (ResetElement(); !EndOfElements(); NextElement()) {
            if (ReadElement() != AJA_STATUS_SUCCESS)
                return AJA_STATUS_FAIL;
            for (ResetComponent(); !EndOfComponents(); NextComponent()) {
                if (ReadComponent() != AJA_STATUS_SUCCESS)
                    return AJA_STATUS_FAIL;
            }
        }
    }
    return AJA_STATUS_SUCCESS;
}

AJAStatus CNTV2PixelComponentReader::ReadComponentValues() {
    AJAStatus status = AJA_STATUS_SUCCESS;

    const NTV2PixelFormat pixel_format = _format_desc.GetPixelFormat();

    // const ULWord bytes_per_elem = get_bytes_per_element();
    const ULWord components_per_elem = ComponentsPerElement();
    const ULWord raster_pix_per_elem = RasterPixelsPerElement(); // first element offset
    // const ULWord elem_per_line = get_elements_per_line();
    const ULWord fb_size = _framebuffer.GetByteCount();
    const ULWord total_bytes = _format_desc.GetTotalBytes();
    // const ULWord bytes_per_row = elem_per_line * bytes_per_elem;
    // const ULWord fb_num_lines = fb_size / bytes_per_row;

    UByte const* planar_bytes[4] = { nullptr, nullptr, nullptr, nullptr };
    UWord const* planar_words[4] = { nullptr, nullptr, nullptr, nullptr };
    static const ULWord y_plane = 0;
    static const ULWord cb_cr_plane = 1;
    static const ULWord cb_plane = 1;
    static const ULWord cr_plane = 2;

    // source file should match size of expected format desc
    if (fb_size < total_bytes)
        return AJA_STATUS_BADBUFFERSIZE;

    const ULWord raster_height = _format_desc.GetFullRasterHeight();
    const ULWord raster_width = _format_desc.GetFullRasterDimensions().GetWidth();
    const ULWord num_horizontal_elems = raster_width / raster_pix_per_elem;

    std::vector<ULWord> component_values = {};
    std::vector<UByte> line_u8 = {};
    std::vector<UWord> line_u16 = {};
    std::vector<ULWord> line_u32 = {};
    for (ULWord line_index = 0; line_index < raster_height; line_index++) {
        const void* line_ptr = _format_desc.GetRowAddress(_framebuffer.GetHostPointer(), line_index);
        UByte const* img_data_u8 = nullptr;
        UWord const* img_data_u16 = nullptr;
        ULWord const* img_data_u32 = nullptr;
        bool unpacked = false;
        const ULWord line_stride = _format_desc.GetBytesPerRow();
        line_u8.clear();
        line_u16.clear();
        line_u32.clear();
        switch (pixel_format) {
            case NTV2_FBF_10BIT_YCBCR:
            case NTV2_FBF_10BIT_YCBCR_DPX:
            {
                unpacked = UnpackLine_10BitYUVtoUWordSequence(
                    line_ptr,
                    _format_desc,
                    line_u16
                );
                _u16_lines.push_back(line_u16);
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
                img_data_u8 = reinterpret_cast<UByte const*>(line_ptr);
                line_u8.assign(img_data_u8, img_data_u8 + line_stride);
                _u8_lines.push_back(line_u8);
                break;
            }
            case NTV2_FBF_48BIT_RGB:
            case NTV2_FBF_12BIT_RGB_PACKED:
            {
                img_data_u16 = reinterpret_cast<UWord const*>(line_ptr);
                line_u16.assign(img_data_u16, img_data_u16 + (line_stride / 2));
                _u16_lines.push_back(line_u16);
                break;
            }
            case NTV2_FBF_10BIT_RGB:
            case NTV2_FBF_10BIT_DPX:
            case NTV2_FBF_10BIT_DPX_LE:
            {
                img_data_u32 = reinterpret_cast<ULWord const*>(line_ptr);
                line_u32.assign(img_data_u32, img_data_u32 + (line_stride / 4));
                _u32_lines.push_back(line_u32);
                break;
            }
            case NTV2_FBF_8BIT_YCBCR_420PL3: /* I420 */
            case NTV2_FBF_8BIT_YCBCR_422PL3: /* I422 */
            {
                const void* y_plane_line = _format_desc.GetRowAddress(_framebuffer.GetHostPointer(), line_index, y_plane);
                const void* cb_plane_line = _format_desc.GetRowAddress(_framebuffer.GetHostPointer(), line_index, cb_plane);
                const void* cr_plane_line = _format_desc.GetRowAddress(_framebuffer.GetHostPointer(), line_index, cr_plane);
                planar_bytes[0] = planar_bytes[2] = reinterpret_cast<UByte const*>(y_plane_line);
                planar_bytes[1] = reinterpret_cast<UByte const*>(cb_plane_line);
                planar_bytes[3] = reinterpret_cast<UByte const*>(cr_plane_line);
                break;
            }
            case NTV2_FBF_10BIT_YCBCR_420PL3_LE:
            case NTV2_FBF_10BIT_YCBCR_422PL3_LE:
            {
                const void* y_plane_line = _format_desc.GetRowAddress(_framebuffer.GetHostPointer(), line_index, y_plane);
                const void* cb_plane_line = _format_desc.GetRowAddress(_framebuffer.GetHostPointer(), line_index, cb_plane);
                const void* cr_plane_line = _format_desc.GetRowAddress(_framebuffer.GetHostPointer(), line_index, cr_plane);
                planar_words[0] = planar_words[2] = reinterpret_cast<UWord const*>(y_plane_line);
                planar_words[1] = reinterpret_cast<UWord const*>(cb_plane_line);
                planar_words[3] = reinterpret_cast<UWord const*>(cr_plane_line);
                break;
            }
            case NTV2_FBF_8BIT_YCBCR_420PL2:
            {
                const void* y_plane_line = _format_desc.GetRowAddress(_framebuffer.GetHostPointer(), line_index, y_plane);
                const void* cb_cr_line = _format_desc.GetRowAddress(_framebuffer.GetHostPointer(), line_index, cb_cr_plane);
                planar_bytes[y_plane] = reinterpret_cast<UByte const*>(y_plane_line);
                planar_bytes[cb_cr_plane] = reinterpret_cast<UByte const*>(cb_cr_line);
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
        for (ULWord elem_offset = 0; elem_offset < num_horizontal_elems; elem_offset++) {
            // Read each pixel component
            for (ULWord component_index = 0; component_index < components_per_elem; component_index++) {
                switch (pixel_format) {
                    // YUV 10-bit
                    case NTV2_FBF_10BIT_YCBCR:
                    case NTV2_FBF_10BIT_YCBCR_DPX:
                    {
                        const size_t uint_index = static_cast<size_t>((components_per_elem) * elem_offset + component_index);
                        const UWord cmp_value = line_u16[uint_index];
                        _u16_components.push_back(cmp_value);
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
                        const size_t byte_index = static_cast<size_t>(
                            (components_per_elem) * elem_offset + component_index
                        );
                        const UWord component_value = line_u8[byte_index];
                        _u16_components.push_back(component_value);
                        _word_size = 16;
                        break;
                    }
                    case NTV2_FBF_10BIT_RGB:
                    {
                        NTV2_ASSERT(component_index < 4);
                        static const ULWord masks[]  = { 0x3ff, 0x3ff, 0x3ff, 0x003, 0x000 };
                        static const ULWord shifts[] = {     0,    10,    20,    30,     0 };
                        const ULWord word = line_u32[elem_offset];
                        const UWord component_value = static_cast<UWord>(
                            (word >> shifts[component_index]) & masks[component_index]
                        );
                        _u16_components.push_back(component_value);
                        _word_size = 16;
                        break;
                    }
                    case NTV2_FBF_10BIT_DPX:
                    {
                        NTV2_ASSERT(component_index < 3);

                        static const ULWord masks[] =  { 0x00000FFC, 0x003FF000, 0xFFC00000, 0 }; // BGR
                        static const ULWord shifts[] = {          2,         12,         22, 0 };
                        const ULWord word = NTV2EndianSwap32(line_u32[elem_offset]);
                        const UWord component_value = static_cast<UWord>(
                            ((word & masks[component_index]) >> shifts[component_index])
                        );
                        _u16_components.push_back(component_value);
                        _word_size = 16;
                        break;
                    }
                    case NTV2_FBF_10BIT_DPX_LE:
                    {
                        NTV2_ASSERT(component_index < 3);
                        static const ULWord masks[]  = { 0x00000FFC, 0x003FF000, 0xFFC00000, 0 };
                        static const ULWord shifts[] = {          2,         12,         22, 0 };
                        const ULWord word = line_u32[elem_offset];
                        const UWord component_value = static_cast<UWord>(
                            ((word & masks[component_index]) >> shifts[component_index])
                        );
                        _u16_components.push_back(component_value);
                        _word_size = 16;
                        break;
                    }
                    case NTV2_FBF_24BIT_RGB:
                    case NTV2_FBF_24BIT_BGR:
                    {
                        NTV2_ASSERT(component_index < 3);
                        // Component 0 is B, 1 is G, 2 is R
                        const size_t byte_index = static_cast<size_t>(
                            (components_per_elem) * elem_offset + component_index
                        );
                        const UWord component_value = line_u8[byte_index];
                        _u16_components.push_back(component_value);
                        _word_size = 16;
                        break;
                    }
                    case NTV2_FBF_48BIT_RGB:
                    case NTV2_FBF_12BIT_RGB_PACKED:
                    {
                        NTV2_ASSERT(component_index < 3);
                        const size_t word_index = static_cast<size_t>(
                            (components_per_elem) * elem_offset + component_index
                        );
                        const UWord component_value = line_u16[word_index];
                        _u16_components.push_back(component_value);
                        _word_size = 16;
                        break;
                    }
                    case NTV2_FBF_8BIT_YCBCR_420PL3:
                    case NTV2_FBF_8BIT_YCBCR_422PL3:
                    {
                        ULWord byte_offset = (component_index == 0 ? elem_offset * 2 : (component_index == 2 ? elem_offset * 2 + 1 : elem_offset / 2));
                        UWord component_value = planar_bytes[component_index][byte_offset];
                        _u8_components.push_back((UByte)component_value);
                        break;
                    }
                    case NTV2_FBF_10BIT_YCBCR_420PL3_LE:
                    case NTV2_FBF_10BIT_YCBCR_422PL3_LE:
                    {
                        ULWord word_offset = (component_index == 0 ? elem_offset : (component_index == 2 ? elem_offset + 1 : elem_offset / 4));
                        UWord component_value = planar_words[component_index][word_offset] & 0x3FF;
                        _u16_components.push_back(component_value);
                        break;
                    }
                    case NTV2_FBF_8BIT_YCBCR_420PL2:
                    {
                        UWord plane_index = ((component_index & 1) != 0) ? cb_cr_plane : y_plane;
                        ULWord byte_offset = (component_index > 1 ? (elem_offset * 2 + 1) : elem_offset * 2);
                        UWord component_value = planar_bytes[plane_index][byte_offset];
                        _u8_components.push_back((UByte)component_value);
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
            }
        }
    }

    return status;
}
