#define RX(i)  ((i)&0x3FF)
#define INDEX_R(i) (((i)&0x3FF)>>5)
#define FRACTION_R(i) (((i)&0x3FF)&0x1F)

#define GX(i)  ((i>>10)&0x3FF)
#define INDEX_G(i) (((i>>10)&0x3FF)>>5)
#define FRACTION_G(i) (((i>>10)&0x3FF)&0x1F)

#define BX(i)  ((i>>20)&0x3FF)
#define INDEX_B(i) (((i>>20)&0x3FF)>>5)
#define FRACTION_B(i) (((i>>20)&0x3FF)&0x1F)

typedef struct {
    ushort r;
    ushort g;
    ushort b;
} RGB16BitCoefs;

void kernel RGB48_3DLUT_16BIT(  global  ushort* input, global RGB16BitCoefs* pLut3D)
{
uint index = get_global_id(0);
ushort ri = input[index*3];
ushort gi = input[index*3+1];
ushort bi = input[index*3+2];

ushort nR = ri>>11;
ushort fR = ri-(nR<<11);
ushort nG = gi>>11;
ushort fG = gi-(nG<<11);
ushort nB = bi>>11;
ushort fB = bi-(nB<<11);

ushort FIX_ONE_33POINT_16BIT=2048;

RGB16BitCoefs p1,p2,p3,p4;
int f1,f2,f3,f4;

p1 = *(pLut3D + (nB*33*33)+(nG*33)+nR);
p4 = *(pLut3D + ((nB+1)*33*33)+((nG+1)*33)+(nR+1));
if ( fG >= fB && fB >= fR )
{
    // T1
    p2 = *(pLut3D + ((nB)*33*33)+((nG+1)*33)+(nR));
    p3 = *(pLut3D + ((nB+1)*33*33)+((nG+1)*33)+(nR));
    f1 = FIX_ONE_33POINT_16BIT-fG;
    f2 = fG-fB;
    f3 = fB-fR;
    f4 = fR;

}
else if ( fB > fR && fR > fG)
{
    // T2
    p2 = *(pLut3D + ((nB+1)*33*33)+((nG)*33)+(nR));
    p3 = *(pLut3D + ((nB+1)*33*33)+((nG)*33)+(nR+1));
    f1 = FIX_ONE_33POINT_16BIT-fB;
    f2 = fB-fR;
    f3 = fR-fG;
    f4 = fG;

}
else if ( fB > fG && fG >= fR )
{
    // T3
    p2 = *(pLut3D + ((nB+1)*33*33)+((nG)*33)+(nR));
    p3 = *(pLut3D + ((nB+1)*33*33)+((nG+1)*33)+(nR));
    f1 = FIX_ONE_33POINT_16BIT-fB;
    f2 = fB-fG;
    f3 = fG-fR;
    f4 = fR;
}
else if ( fR >= fG && fG > fB )
{
    // T4
    p2 = *(pLut3D + ((nB)*33*33)+((nG)*33)+(nR+1));
    p3 = *(pLut3D + ((nB)*33*33)+((nG+1)*33)+(nR+1));
    f1 = FIX_ONE_33POINT_16BIT-fR;
    f2 = fR-fG;
    f3 = fG-fB;
    f4 = fB;
}
else if ( fG > fR && fR >=fB )
{
    // T5
    p2 = *(pLut3D + ((nB)*33*33)+((nG+1)*33)+(nR));
    p3 = *(pLut3D + ((nB)*33*33)+((nG+1)*33)+(nR+1));
    f1 = FIX_ONE_33POINT_16BIT-fG;
    f2 = fG-fR;
    f3 = fR-fB;
    f4 = fB;

}
else
{
    // T6
    p2 = *(pLut3D + ((nB)*33*33)+((nG)*33)+(nR+1));
    p3 = *(pLut3D + ((nB+1)*33*33)+((nG)*33)+(nR+1));
    f1 = FIX_ONE_33POINT_16BIT-fR;
    f2 = fR-fB;
    f3 = fB-fG;
    f4 = fG;
}

input[index*3]     = (f1*(p1.r) + f2*(p2.r) + f3*(p3.r) + f4*(p4.r)+1024)>>11;
input[index*3+1] = (f1*(p1.g) + f2*(p2.g) + f3*(p3.g) + f4*(p4.g)+1024)>>11;
input[index*3+2] = (f1*(p1.b) + f2*(p2.b) + f3*(p3.b) + f4*(p4.b)+1024)>>11;


}
