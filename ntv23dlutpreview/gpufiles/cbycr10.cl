void kernel CbYCr10(global const uint* input, __write_only image2d_t output)
{
    int2 coord = {get_global_id(0), get_global_id(1)};

    int cadence =  get_global_id(0)%6;
    int rowInts = (((  get_global_size(0) % 48 == 0 ) ?  get_global_size(0) : ((( get_global_size(0) / 48 ) + 1) * 48)) * 8 / 3)/4;
    if ( ((get_global_size(0)-1) == get_global_id(0)) && ((get_global_size(1)-1) == get_global_id(1)))
        cadence = 4;

    float y,cb,cr;
    cb = 512.0;
    cr = 512.0;
    int index;
    switch ( cadence )
    {
    case 0:
    index = (get_global_id(0)/6)*4 + get_global_id(1)*rowInts;
    y = ((float)((input[index]>>10)&0x3FF));
    cb = ((float)((input[index])&0x3FF));
    cr = ((float)((input[index]>>20)&0x3FF));
    break;
    case 1:
    index = (get_global_id(0)/6)*4 + get_global_id(1)*rowInts+1;
    y = ((float)((input[index])&0x3FF));
    cb = ((float)((input[index]>>10)&0x3FF));
    cr = ((float)((input[index-1]>>20)&0x3FF));
    break;
    case 2:
    index = (get_global_id(0)/6)*4 + get_global_id(1)*rowInts+1;
    y = ((float)((input[index]>>20)&0x3FF));
    cb = ((float)((input[index-1])&0x3FF));
    cr = ((float)((input[index+1])&0x3FF));
    break;
    case 3:
    index = (get_global_id(0)/6)*4 + get_global_id(1)*rowInts+2;
    y = ((float)((input[index]>>10)&0x3FF));
    cb = ((float)((input[index]>>20)&0x3FF));
    cr = ((float)((input[index])&0x3FF));
    break;
    case 4:
    index = (get_global_id(0)/6)*4 + get_global_id(1)*rowInts+3;
    y = ((float)((input[index])&0x3FF));
    cb = ((float)((input[index-1]>>20)&0x3FF));
    cr = ((float)((input[index]>>10)&0x3FF));
    break;
    case 5:
    index = (get_global_id(0)/6)*4 + get_global_id(1)*rowInts+3;
    y = ((float)((input[index]>>20)&0x3FF));
    cb = ((float)((input[index+1])&0x3FF));
    cr = ((float)((input[index]>>10)&0x3FF));
    break;
    }

    float4 outcolor;
    outcolor.z = (1.167786 * (y - 64.0) + 2.118591 * (cr - 512.0))/1023.0;
    outcolor.y = (1.167786 * (y - 64.0) - 0.534515 * (cr - 512.0) - 0.213898 * (cb - 512.0))/1023.0;
    outcolor.x = (1.167786 * (y - 64.0)                        + 2.017 * (cb - 512.0))/1023.0;
    outcolor.w = 1.0;

    // Write output pixel
    write_imagef(output, coord, outcolor);

}
