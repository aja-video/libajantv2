#define AJA_ENDIAN_SWAP32(_data_) (((_data_<<24)&0xff000000)|((_data_<<8)&0x00ff0000)|((_data_>>8)&0x0000ff00)|((_data_>>24)&0x000000ff))


void kernel RGB10DPXBE(__read_only image2d_t input, __write_only image2d_t output)
{
    int2 coord = {get_global_id(0), get_global_id(1)};

    const sampler_t sampler=CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP | CLK_FILTER_NEAREST;

    float4 outcolor;

    // Read input pixel
    uint4 incolor = read_imageui(input, sampler, coord);
    uint color = AJA_ENDIAN_SWAP32(incolor.x);

    // Convert to float
    outcolor.x = ((float)(((color)>>2) & 0x3FF)) / 1023.0f;
    outcolor.y = ((float)((((color)>>12)) & 0x3FF)) / 1023.0f;
    outcolor.z = ((float)((((color)>>22)) & 0x3FF)) / 1023.0f;
    outcolor.w = 1.0;

    // Write output pixel
    write_imagef(output, coord, outcolor);

}
