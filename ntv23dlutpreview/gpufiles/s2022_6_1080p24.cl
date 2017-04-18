void kernel S2022_6_1080p24(global const uchar* input, __write_only image2d_t output)
{
    int2 coord = {get_global_id(0), get_global_id(1)};

    uint rowChars = 6875;

    float y,cb,cr;
    y   = 100.0;
    cb = 512.0;
    cr = 512.0;

    uint outputLine = get_global_id(1);
    uint outputPixel = get_global_id(0);
    uint lineOffset = 42;


    uint startIndex;
    startIndex = 2075 + rowChars*(lineOffset+outputLine);
    uint pixelOffset = 5*(outputPixel/2);
    startIndex = startIndex+pixelOffset;
    if ( outputPixel & 0x1 )
    {
        cr = (float)((((ushort)input[startIndex+2]&0xF)<<6) + (input[startIndex+3] >>2));
        y  = (float)((((ushort)input[startIndex+3]&0x3)<<8) + input[startIndex+4]) ;
        cb = (float)((((ushort)input[startIndex+5])<<2) + (input[startIndex+6] >>6));
    }
    else
    {     
        cb = (float)((((ushort)input[startIndex])<<2) + (input[startIndex+1] >>6));
        y  = (float)((((ushort)input[startIndex+1]&0x3F)<<4) + (input[startIndex+2] >>4));
        cr = (float)((((ushort)input[startIndex+2]&0xF)<<6) + (input[startIndex+3] >>2));
    }

    float4 outcolor;
    outcolor.z = (1.167786 * (y - 64.0) + 2.118591 * (cr - 512.0))/1023.0;
    outcolor.y = (1.167786 * (y - 64.0) - 0.534515 * (cr - 512.0) - 0.213898 * (cb - 512.0))/1023.0;
    outcolor.x = (1.167786 * (y - 64.0)                        + 2.017 * (cb - 512.0))/1023.0;
    outcolor.w = 1.0;

    // Write output pixel
    write_imagef(output, coord, outcolor);

}
