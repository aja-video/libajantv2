void kernel RGB10DPXLE(__read_only image2d_t input, __write_only image2d_t output)
{
    int2 coord = {get_global_id(0), get_global_id(1)};

    const sampler_t sampler=CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP | CLK_FILTER_NEAREST;

    float4 outcolor;

    // Read input pixel
    uint4 incolor = read_imageui(input, sampler, coord);

    // Convert to float
    outcolor.x = ((float)(((incolor.x)>>2) & 0x3FF)) / 1023.0f;
    outcolor.y = ((float)((((incolor.x)>>12)) & 0x3FF)) / 1023.0f;
    outcolor.z = ((float)((((incolor.x)>>22)) & 0x3FF)) / 1023.0f;
    outcolor.w = 1.0;

    // Write output pixel
    write_imagef(output, coord, outcolor);

}
