#define RX(i)  ((i)&0x3FF)
#define INDEX_R(i) (((i)&0x3FF)>>5)
#define FRACTION_R(i) (((i)&0x3FF)&0x1F)

#define GX(i)  ((i>>10)&0x3FF)
#define INDEX_G(i) (((i>>10)&0x3FF)>>5)
#define FRACTION_G(i) (((i>>10)&0x3FF)&0x1F)

#define BX(i)  ((i>>20)&0x3FF)
#define INDEX_B(i) (((i>>20)&0x3FF)>>5)
#define FRACTION_B(i) (((i>>20)&0x3FF)&0x1F)

#define FIX_ONE_33POINT (0x20)

void kernel RGB10_3DLUT( image2d_t input, global const uint* pLut3D, global  uint* output)
{
    int2 coord = {get_global_id(0), get_global_id(1)};

    const sampler_t sampler=CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP | CLK_FILTER_NEAREST;

    float4 outcolor;

    uint4 incolor = read_imageui(input, sampler, coord);

    uint nR = INDEX_R(incolor.x);
    uint fR = FRACTION_R(incolor.x);
    uint nG = INDEX_G(incolor.x);
    uint fG = FRACTION_G(incolor.x);
    uint nB = INDEX_B(incolor.x);
    uint fB = FRACTION_B(incolor.x);

    uint p1,p2,p3,p4;
    p1 = *(pLut3D + (nB*33*33)+(nG*33)+nR);
    p4 = *(pLut3D + ((nB+1)*33*33)+((nG+1)*33)+(nR+1));
    uint f1,f2,f3,f4;
    if ( fG >= fB && fB >= fR )
    {
        // T1
        p2 = *(pLut3D + ((nB)*33*33)+((nG+1)*33)+(nR));
        p3 = *(pLut3D + ((nB+1)*33*33)+((nG+1)*33)+(nR));
        f1 = FIX_ONE_33POINT-fG;
        f2 = fG-fB;
        f3 = fB-fR;
        f4 = fR;

    }
    else if ( fB > fR && fR > fG)
    {
        // T2
        p2 = *(pLut3D + ((nB+1)*33*33)+((nG)*33)+(nR));
        p3 = *(pLut3D + ((nB+1)*33*33)+((nG)*33)+(nR+1));
        f1 = FIX_ONE_33POINT-fB;
        f2 = fB-fR;
        f3 = fR-fG;
        f4 = fG;

    }
    else if ( fB > fG && fG >= fR )
    {
        // T3
        p2 = *(pLut3D + ((nB+1)*33*33)+((nG)*33)+(nR));
        p3 = *(pLut3D + ((nB+1)*33*33)+((nG+1)*33)+(nR));
        f1 = FIX_ONE_33POINT-fB;
        f2 = fB-fG;
        f3 = fG-fR;
        f4 = fR;
    }
    else if ( fR >= fG && fG > fB )
    {
        // T4
        p2 = *(pLut3D + ((nB)*33*33)+((nG)*33)+(nR+1));
        p3 = *(pLut3D + ((nB)*33*33)+((nG+1)*33)+(nR+1));
        f1 = FIX_ONE_33POINT-fR;
        f2 = fR-fG;
        f3 = fG-fB;
        f4 = fB;
    }
    else if ( fG > fR && fR >=fB )
    {
        // T5
        p2 = *(pLut3D + ((nB)*33*33)+((nG+1)*33)+(nR));
        p3 = *(pLut3D + ((nB)*33*33)+((nG+1)*33)+(nR+1));
        f1 = FIX_ONE_33POINT-fG;
        f2 = fG-fR;
        f3 = fR-fB;
        f4 = fB;

    }
    else
    {
        // T6
        p2 = *(pLut3D + ((nB)*33*33)+((nG)*33)+(nR+1));
        p3 = *(pLut3D + ((nB+1)*33*33)+((nG)*33)+(nR+1));
        f1 = FIX_ONE_33POINT-fR;
        f2 = fR-fB;
        f3 = fB-fG;
        f4 = fG;
    }

    uint R,G,B;
    R = (f1*RX(p1) + f2*RX(p2) + f3*RX(p3) + f4*RX(p4))>>5;
    G = (f1*GX(p1) + f2*GX(p2) + f3*GX(p3) + f4*GX(p4))>>5;
    B = (f1*BX(p1) + f2*BX(p2) + f3*BX(p3) + f4*BX(p4))>>5;

    int index = get_global_id(0) + get_global_size(0)*get_global_id(1);

    output[index]  = ((B<<20) + (G<<10) + R);

//    outcolor.z = ((float)R / 1023.0f);
//    outcolor.y = ((float)G / 1023.0f);
//    outcolor.x = ((float)B / 1023.0f);
//    outcolor.w = 1.0;

    // Write output pixel
//    write_imageui(output, coord, (uint4)outcolor);

}
