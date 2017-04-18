
void kernel CbYCr8ToRGB8(__read_only image2d_t input, __write_only image2d_t output)
{
    int2 coord1 = {get_global_id(0), get_global_id(1)};
    int2 coord2 = {get_global_id(0)+1, get_global_id(1)};
    int2 outpos = {get_global_id(0), get_global_id(1)};

    const sampler_t sampler=CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP | CLK_FILTER_NEAREST;

    float4 outcolor;

    // Read input pixel
    uint4 incolor1 = read_imageui(input, sampler, coord1);
    uint4 incolor2 = read_imageui(input, sampler, coord2);

    float cb;
    float y;
    float cr;

    if ( get_global_id(0) & 0x1 )
    {
        cb = (float)incolor2.x;
        y  = (float)incolor1.y;
        cr = (float)incolor1.x;

    }
    else
    {
        cb = (float)incolor1.x;
        y  = (float)incolor1.y;
        cr = (float)incolor2.x;
     }
    outcolor.z = (1.167786 * (y - 16.0) + 2.118591 * (cr - 128.0))/255.0;
    outcolor.y = (1.167786 * (y - 16.0) - 0.534515 * (cr - 128.0) - 0.213898 * (cb - 128.0))/255.0;
    outcolor.x = (1.167786 * (y - 16.0)                        + 2.017 * (cb - 128.0))/255.0;
    outcolor.w = 1.0;

    // Write output pixel
    write_imagef(output, outpos, outcolor);

}
