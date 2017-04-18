void kernel RGB16(global const ushort* input, __write_only image2d_t output)
{
int2 coord = {get_global_id(0), get_global_id(1)};

// Read input pixel
int index = 3*get_global_id(0) + get_global_size(0)*get_global_id(1)*3;
ushort r,g,b;
r=input[index+0];
g=input[index+1];
b=input[index+2];


// Convert to float
float4 outcolor;
outcolor.z = (float)r / 65535.0f;
outcolor.y = (float)g / 65535.0f;
outcolor.x = (float)b / 65535.0f;
outcolor.w = (float)1.0f;

// Write output pixel
write_imagef(output, coord, outcolor);

}
