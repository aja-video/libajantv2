void kernel RGBA8(__read_only image2d_t input, __write_only image2d_t output)
{
    int2 coord = {get_global_id(0), get_global_id(1)};

    const sampler_t sampler=CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP | CLK_FILTER_NEAREST;

    float4 outcolor;

    // Read input pixel
    uint4 incolor = read_imageui(input, sampler, coord);

    // Convert to float
    outcolor.x = (float)incolor.w / 255.0f;
    outcolor.y = (float)incolor.z / 255.0f;
    outcolor.z = (float)incolor.y / 255.0f;
    outcolor.w = (float)incolor.x / 255.0f;

    // Write output pixel
    write_imagef(output, coord, outcolor);

}

