
//	These tests and their values are taken from the Tektronix supplied spreadsheet "SMPTE 352M VPID list.xls"
//	and the spreadsheet 4KVPID.xls
//	The tests are indexed by the spreadsheet row

//	Note that some additional tests have been added for formats, i.e. 525i 8 bit, or routings (DualLink1_5) that Tektronix doesn't do
//	Also, some formats that Tektronix does do that we don't have been dropped, such as 720p23.98

#include "testentry.h"
#include "tests.h"

static const ULWord ZeroedVPID (0);

static TestEntry testArray [] =
{

//  Routing            Video Format                        Pixel Format         Link 1 DS1  Link 1 DS2  Link 2 DS1  Link 2 DS2  Link 3 DS1  Link 4 DS1

//	Start of spreadsheet SMPTE 352M VPID list.xls

//	Rows 1 - 2 are column labels
//	Row 3 is the header: SD formats defined in SMPTE 259M, VPID described in 352M §A.1
//	Row 4
{ SingleLink1_5,	NTV2_FORMAT_525_5994,				NTV2_FBF_10BIT_YCBCR,	0x81060001, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 1

//	Row 5
{ SingleLink1_5,	NTV2_FORMAT_625_5000,				NTV2_FBF_10BIT_YCBCR,	0x81050001, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 2

//	Row 6 is the header: HD 720 line formats defined in SMPTE 296M, VPID described in 352M §A.4
//	Rows 7 - 11 are unsupported 720p frame rates
//	Row 12
{ SingleLink1_5,	NTV2_FORMAT_720p_5000,				NTV2_FBF_10BIT_YCBCR,	0x84490001, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 3

//	Row 13
{ SingleLink1_5,	NTV2_FORMAT_720p_5994,				NTV2_FBF_10BIT_YCBCR,	0x844A0001, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 4

//	Row 14
{ SingleLink1_5,	NTV2_FORMAT_720p_6000,				NTV2_FBF_10BIT_YCBCR,	0x844B0001, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 5

//	Row 15 is the header: HD 1080 line formats defined in SMPTE 274M, VPID described in 352M §A.5
//	Row 16
{ SingleLink1_5,	NTV2_FORMAT_1080i_5000,				NTV2_FBF_10BIT_YCBCR,	0x85052001, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 6

//	Row 17
{ SingleLink1_5,	NTV2_FORMAT_1080i_5994,				NTV2_FBF_10BIT_YCBCR,	0x85062001, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 7

//	Row 18
{ SingleLink1_5,	NTV2_FORMAT_1080i_6000,				NTV2_FBF_10BIT_YCBCR,	0x85072001, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 8

//	Row 19
{ SingleLink1_5,	NTV2_FORMAT_1080p_2398,				NTV2_FBF_10BIT_YCBCR,	0x85C22001, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 9

//	Row 20
{ SingleLink1_5,	NTV2_FORMAT_1080p_2400,				NTV2_FBF_10BIT_YCBCR,	0x85C32001, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 10

//	Row 21
{ SingleLink1_5,	NTV2_FORMAT_1080p_2500,				NTV2_FBF_10BIT_YCBCR,	0x85C52001, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 11

//	Row 22
{ SingleLink1_5,	NTV2_FORMAT_1080p_2997,				NTV2_FBF_10BIT_YCBCR,	0x85C62001, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 12

//	Row 23
{ SingleLink1_5,	NTV2_FORMAT_1080p_3000,				NTV2_FBF_10BIT_YCBCR,	0x85C72001, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 13

//	Row 24
{ SingleLink1_5,	NTV2_FORMAT_1080psf_2398,			NTV2_FBF_10BIT_YCBCR,	0x85422001, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 14

//	Row 25
{ SingleLink1_5,	NTV2_FORMAT_1080psf_2400,			NTV2_FBF_10BIT_YCBCR,	0x85432001, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 15

//	Row 26
{ SingleLink1_5,	NTV2_FORMAT_1080psf_2500_2,			NTV2_FBF_10BIT_YCBCR,	0x85452001, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 16

//	Row 27
{ SingleLink1_5,	NTV2_FORMAT_1080psf_2997_2,			NTV2_FBF_10BIT_YCBCR,	0x85462001, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 17

//	Row 28
{ SingleLink1_5,	NTV2_FORMAT_1080psf_3000_2,			NTV2_FBF_10BIT_YCBCR,	0x85472001, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 18

//	Row 29 is the header: 3G Level A mapping structure 1 from SMPTE 425 §3.2.1, VPID described in 425 §4.1.6
//	Row 30
{ SingleLink3_0A,	NTV2_FORMAT_1080p_5000_A,			NTV2_FBF_10BIT_YCBCR,	0x89C98001, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 19

//	Row 31
{ SingleLink3_0A,	NTV2_FORMAT_1080p_5994_A,			NTV2_FBF_10BIT_YCBCR,	0x89CA8001, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 20

//	Row 32
{ SingleLink3_0A,	NTV2_FORMAT_1080p_6000_A,			NTV2_FBF_10BIT_YCBCR,	0x89CB8001, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 21

//	Row 33 is the header: 3G Level A mapping structure 2 (720 lines) from SMPTE 425 §3.2.2, VPID described in 425 §4.1.6
//	Rows 34 - 41 are unsupported pixel format YCbCr 4:4:4
//	Rows 42 - 46 are unsupported 720p frame rates
//	Row 47
{ SingleLink3RGB_A,	NTV2_FORMAT_720p_5000,				NTV2_FBF_10BIT_DPX,		0x88490201, 0x88490201, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 22

//	Row 48
{ SingleLink3RGB_A,	NTV2_FORMAT_720p_5994,				NTV2_FBF_10BIT_DPX,		0x884A0201, 0x884A0201, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 23

//	Row 49
{ SingleLink3RGB_A,	NTV2_FORMAT_720p_6000,				NTV2_FBF_10BIT_DPX,		0x884B0201, 0x884B0201, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 24

//	Rows 50 - 57 are unsupported pixel format YCbCrA 4:4:4:4
//	Rows 58 - 62 are unsupported 720p frame rates
//	Rows 63 - 65 are unsupported pixel format GBRA 4:4:4:4 10 bit

//	Row 66 is the header: 3G Level A mapping structure 2 (1080 lines) from SMPTE 425 §3.2.2, VPID described in 425 §4.1.6
//	Rows 67 - 79 are unsupported pixel format YCbCr 4:4:4
//	Row 80
{ SingleLink3RGB_A,	NTV2_FORMAT_1080i_5000,				NTV2_FBF_10BIT_DPX,		0x89052201, 0x89052201, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 25

//	Row 81
{ SingleLink3RGB_A,	NTV2_FORMAT_1080i_5994,				NTV2_FBF_10BIT_DPX,		0x89062201, 0x89062201, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 26

//	Row 82
{ SingleLink3RGB_A,	NTV2_FORMAT_1080i_6000,				NTV2_FBF_10BIT_DPX,		0x89072201, 0x89072201, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 27

//	Row 83
{ SingleLink3RGB_A,	NTV2_FORMAT_1080p_2398,				NTV2_FBF_10BIT_DPX,		0x89C22201, 0x89C22201, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 28

//	Row 84
{ SingleLink3RGB_A,	NTV2_FORMAT_1080p_2400,				NTV2_FBF_10BIT_DPX,		0x89C32201, 0x89C32201, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 29

//	Row 85
{ SingleLink3RGB_A,	NTV2_FORMAT_1080p_2500,				NTV2_FBF_10BIT_DPX,		0x89C52201, 0x89C52201, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 30

//	Row 86
{ SingleLink3RGB_A,	NTV2_FORMAT_1080p_2997,				NTV2_FBF_10BIT_DPX,		0x89C62201, 0x89C62201, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 31

//	Row 87
{ SingleLink3RGB_A,	NTV2_FORMAT_1080p_3000,				NTV2_FBF_10BIT_DPX,		0x89C72201, 0x89C72201, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 32

//	Row 88
{ SingleLink3RGB_A,	NTV2_FORMAT_1080psf_2398,			NTV2_FBF_10BIT_DPX,		0x89422201, 0x89422201, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 33

//	Row 89
{ SingleLink3RGB_A,	NTV2_FORMAT_1080psf_2400,			NTV2_FBF_10BIT_DPX,		0x89432201, 0x89432201, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 34

//	Row 90
{ SingleLink3RGB_A,	NTV2_FORMAT_1080psf_2500_2,			NTV2_FBF_10BIT_DPX,		0x89452201, 0x89452201, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 35

//	Row 91
{ SingleLink3RGB_A,	NTV2_FORMAT_1080psf_2997_2,			NTV2_FBF_10BIT_DPX,		0x89462201, 0x89462201, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 36

//	Row 92
{ SingleLink3RGB_A,	NTV2_FORMAT_1080psf_3000_2,			NTV2_FBF_10BIT_DPX,		0x89472201, 0x89472201, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 37

//	Rows  93 - 105 are unsupported pixel format YCbCrA 4:4:4:4
//	Rows 106 - 118 are unsupported pixel format RGB_A 4:4:4:4 10 bit

//	Row 119 is the header: 3G Level A mapping structure 3 (1920×080) from SMPTE 425 §3.2.3, VPID described in 425 §4.1.6
//	******** THIS SECTION HAS FRAME RATES IN REVERSE ORDER ********
//	Rows 120 - 127 are unsupported pixel format YCbCr 4:4:4
//	Row 128
{ SingleLink3RGB_A,	NTV2_FORMAT_1080i_6000,				NTV2_FBF_48BIT_RGB,		0x89072202, 0x89072202, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 38

//	Row 129
{ SingleLink3RGB_A,	NTV2_FORMAT_1080i_5994,				NTV2_FBF_48BIT_RGB,		0x89062202, 0x89062202, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 39

//	Row 130
{ SingleLink3RGB_A,	NTV2_FORMAT_1080i_5000,				NTV2_FBF_48BIT_RGB,		0x89052202, 0x89052202, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 40

//	Row 131
{ SingleLink3RGB_A,	NTV2_FORMAT_1080p_3000,				NTV2_FBF_48BIT_RGB,		0x89C72202, 0x89C72202, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 41

//	Row 132
{ SingleLink3RGB_A,	NTV2_FORMAT_1080p_2997,				NTV2_FBF_48BIT_RGB,		0x89C62202, 0x89C62202, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 42

//	Row 133
{ SingleLink3RGB_A,	NTV2_FORMAT_1080p_2500,				NTV2_FBF_48BIT_RGB,		0x89C52202, 0x89C52202, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 43

//	Row 134
{ SingleLink3RGB_A,	NTV2_FORMAT_1080p_2400,				NTV2_FBF_48BIT_RGB,		0x89C32202, 0x89C32202, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 44

//	Row 135
{ SingleLink3RGB_A,	NTV2_FORMAT_1080p_2398,				NTV2_FBF_48BIT_RGB,		0x89C22202, 0x89C22202, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 45

//	Row 136 is the header: 3G Level A mapping structure 3 (2048×080) from SMPTE 425 §3.2.3, VPID described in 425 §4.1.6
//	Row 137
{ SingleLink3RGB_A,	NTV2_FORMAT_1080p_2K_2398,			NTV2_FBF_48BIT_RGB,		0x89C24202, 0x89C24202, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 46

//	Row 138
{ SingleLink3RGB_A,	NTV2_FORMAT_1080p_2K_2400,			NTV2_FBF_48BIT_RGB,		0x89C34202, 0x89C34202, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 47

//	Row 139
{ SingleLink3RGB_A,	NTV2_FORMAT_1080p_2K_2500,			NTV2_FBF_48BIT_RGB,		0x89C54202, 0x89C54202, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 48

//	Row 140
{ SingleLink3RGB_A,	NTV2_FORMAT_1080p_2K_2997,			NTV2_FBF_48BIT_RGB,		0x89C64202, 0x89C64202, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 49

//	Row 141
{ SingleLink3RGB_A,	NTV2_FORMAT_1080p_2K_3000,			NTV2_FBF_48BIT_RGB,		0x89C74202, 0x89C74202, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 50

//	Row 142
{ SingleLink3RGB_A,	NTV2_FORMAT_1080psf_2K_2398,		NTV2_FBF_48BIT_RGB,		0x89424202, 0x89424202, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 51

//	Row 143
{ SingleLink3RGB_A,	NTV2_FORMAT_1080psf_2K_2400,		NTV2_FBF_48BIT_RGB,		0x89434202, 0x89434202, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 52

//	Row 144
{ SingleLink3RGB_A,	NTV2_FORMAT_1080psf_2K_2500,		NTV2_FBF_48BIT_RGB,		0x89454202, 0x89454202, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 53

//	Row 145 has unsupported video format 1080psf_2K_2997
//	Row 146 has unsupported video format 1080psf_2K_3000
//	Rows 147 - 156 are unsupported pixel format XYZ 4:4:4

//	Row 157 is the header: 3G Level A mapping structure 4 from SMPTE 425 §3.2.4, VPID described in 425 §4.1.6
//	Rows 158 - 170 are unsupported pixel format YCbCr 4:2:2 12 bit

//	Row 171 is the header: 3G Level B, Dual link from SMPTE 372M §4.1, VPID described in 372M Annex A and 425 §5.2
//	Row 172
{ SingleLink3_0B,	NTV2_FORMAT_1080p_5000_B,			NTV2_FBF_10BIT_YCBCR,	0x8A492001, 0x8A492041, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 54

//	Row 173
{ SingleLink3_0B,	NTV2_FORMAT_1080p_5994_B,			NTV2_FBF_10BIT_YCBCR,	0x8A4A2001, 0x8A4A2041, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 55

//	Row 174
{ SingleLink3_0B,	NTV2_FORMAT_1080p_6000_B,			NTV2_FBF_10BIT_YCBCR,	0x8A4B2001, 0x8A4B2041, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 56

//	Row 175 is the header: 3G Level B, Dual link from SMPTE 372M §4.2, VPID described in 372M Annex A and 425 §5.2
//	Row 176
{ SingleLink3_0B,	NTV2_FORMAT_1080i_5000,				NTV2_FBF_10BIT_DPX,		0x8A052201, 0x8A052241, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 57

//	Row 177
{ SingleLink3_0B,	NTV2_FORMAT_1080i_5994,				NTV2_FBF_10BIT_DPX,		0x8A062201, 0x8A062241, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 58

//	Row 178
{ SingleLink3_0B,	NTV2_FORMAT_1080i_6000,				NTV2_FBF_10BIT_DPX,		0x8A072201, 0x8A072241, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 59

//	Row 179
{ SingleLink3_0B,	NTV2_FORMAT_1080p_2398,				NTV2_FBF_10BIT_DPX,		0x8AC22201, 0x8AC22241, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 60

//	Row 180
{ SingleLink3_0B,	NTV2_FORMAT_1080p_2400,				NTV2_FBF_10BIT_DPX,		0x8AC32201, 0x8AC32241, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 61

//	Row 181
{ SingleLink3_0B,	NTV2_FORMAT_1080p_2500,				NTV2_FBF_10BIT_DPX,		0x8AC52201, 0x8AC52241, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 62

//	Row 182
{ SingleLink3_0B,	NTV2_FORMAT_1080p_2997,				NTV2_FBF_10BIT_DPX,		0x8AC62201, 0x8AC62241, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 63

//	Row 183
{ SingleLink3_0B,	NTV2_FORMAT_1080p_3000,				NTV2_FBF_10BIT_DPX,		0x8AC72201, 0x8AC72241, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 64

//	Row 184
{ SingleLink3_0B,	NTV2_FORMAT_1080psf_2398,			NTV2_FBF_10BIT_DPX,		0x8A422201, 0x8A422241, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 65

//	Row 185
{ SingleLink3_0B,	NTV2_FORMAT_1080psf_2400,			NTV2_FBF_10BIT_DPX,		0x8A432201, 0x8A432241, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 66

//	Row 186
{ SingleLink3_0B,	NTV2_FORMAT_1080psf_2500_2,			NTV2_FBF_10BIT_DPX,		0x8A452201, 0x8A452241, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 67

//	Row 187
{ SingleLink3_0B,	NTV2_FORMAT_1080psf_2997_2,			NTV2_FBF_10BIT_DPX,		0x8A462201, 0x8A462241, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 68

//	Row 188
{ SingleLink3_0B,	NTV2_FORMAT_1080psf_3000_2,			NTV2_FBF_10BIT_DPX,		0x8A472201, 0x8A472241, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 69

//	Rows 189 - 201 are unsupported pixel format GBRA 4:4:4:4 10 bit

//	Row 202 is the header: 3G Level B, Dual link from SMPTE 372M §4.3, VPID described in 372M Annex A and 425 §5.2
//	Row 203
{ SingleLink3_0B,	NTV2_FORMAT_1080i_5000,				NTV2_FBF_48BIT_RGB,		0x8A052202, 0x8A052242, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 70

//	Row 204
{ SingleLink3_0B,	NTV2_FORMAT_1080i_5994,				NTV2_FBF_48BIT_RGB,		0x8A062202, 0x8A062242, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 71

//	Row 205
{ SingleLink3_0B,	NTV2_FORMAT_1080i_6000,				NTV2_FBF_48BIT_RGB,		0x8A072202, 0x8A072242, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 72

//	Row 206
{ SingleLink3_0B,	NTV2_FORMAT_1080p_2398,				NTV2_FBF_48BIT_RGB,		0x8AC22202, 0x8AC22242, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 73

//	Row 207
{ SingleLink3_0B,	NTV2_FORMAT_1080p_2400,				NTV2_FBF_48BIT_RGB,		0x8AC32202, 0x8AC32242, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 74

//	Row 208
{ SingleLink3_0B,	NTV2_FORMAT_1080p_2500,				NTV2_FBF_48BIT_RGB,		0x8AC52202, 0x8AC52242, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 75

//	Row 209
{ SingleLink3_0B,	NTV2_FORMAT_1080p_2997,				NTV2_FBF_48BIT_RGB,		0x8AC62202, 0x8AC62242, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 76

//	Row 210
{ SingleLink3_0B,	NTV2_FORMAT_1080p_3000,				NTV2_FBF_48BIT_RGB,		0x8AC72202, 0x8AC72242, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 77

//	Row 211
{ SingleLink3_0B,	NTV2_FORMAT_1080psf_2398,			NTV2_FBF_48BIT_RGB,		0x8A422202, 0x8A422242, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 78

//	Row 212
{ SingleLink3_0B,	NTV2_FORMAT_1080psf_2400,			NTV2_FBF_48BIT_RGB,		0x8A432202, 0x8A432242, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 79

//	Row 213
{ SingleLink3_0B,	NTV2_FORMAT_1080psf_2500_2,			NTV2_FBF_48BIT_RGB,		0x8A452202, 0x8A452242, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 80

//	Row 214
{ SingleLink3_0B,	NTV2_FORMAT_1080psf_2997_2,			NTV2_FBF_48BIT_RGB,		0x8A462202, 0x8A462242, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 81

//	Row 215
{ SingleLink3_0B,	NTV2_FORMAT_1080psf_3000_2,			NTV2_FBF_48BIT_RGB,		0x8A472202, 0x8A472242, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 82

//	Row 216 is the header: 3G Level B, Dual link from SMPTE 372M §4.4, VPID described in 372M Annex A and 425 §5.2
//	Rows 217 - 229 are unsupported pixel format YCbCr 4:4:4 10 bit
//	Rows 230 - 242 are unsupported pixel format YCbCr 4:4:4 12 bit
//	Rows 243 - 255 are unsupported pixel format YCbCr 4:4:4:4 10 bit

//	Row 256 is the header: 3G Level B, Dual link from SMPTE 372M §4.5, VPID described in 372M Annex A and 425 §5.2
//	Rows 257 - 269 are unsupported pixel format YCbCr 4:2:2 12 bit
//	Rows 270 - 282 are unsupported pixel format YCbCrA 4:2:2:4 12 bit

//	Row 283 is the header: 3G Level B, 2K dual link from SMPTE 428-9, VPID described in 372M Annex A and 425 §5.2
//	Row 284
{ SingleLink3_0B,	NTV2_FORMAT_1080p_2K_2398,			NTV2_FBF_48BIT_RGB,		0x8AC24202, 0x8AC24242, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 83

//	Row 285
{ SingleLink3_0B,	NTV2_FORMAT_1080p_2K_2400,			NTV2_FBF_48BIT_RGB,		0x8AC34202, 0x8AC34242, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 84

//	Row 286
{ SingleLink3_0B,	NTV2_FORMAT_1080p_2K_2500,			NTV2_FBF_48BIT_RGB,		0x8AC54202, 0x8AC54242, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 85

//	Row 287
{ SingleLink3_0B,	NTV2_FORMAT_1080p_2K_2997,			NTV2_FBF_48BIT_RGB,		0x8AC64202, 0x8AC64242, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 86

//	Row 288
{ SingleLink3_0B,	NTV2_FORMAT_1080p_2K_3000,			NTV2_FBF_48BIT_RGB,		0x8AC74202, 0x8AC74242, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 87

//	Row 289
{ SingleLink3_0B,	NTV2_FORMAT_1080psf_2K_2398,		NTV2_FBF_48BIT_RGB,		0x8A424202, 0x8A424242, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 88

//	Row 290
{ SingleLink3_0B,	NTV2_FORMAT_1080psf_2K_2400,		NTV2_FBF_48BIT_RGB,		0x8A434202, 0x8A434242, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 89

//	Row 291
{ SingleLink3_0B,	NTV2_FORMAT_1080psf_2K_2500,		NTV2_FBF_48BIT_RGB,		0x8A454202, 0x8A454242, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 90

//	Row 292 has unsupported video format 1080psf_2K_2997
//	Row 293 has unsupported video format 1080psf_2K_3000

//	Rows 294 - 303 are unsupported pixel format XYZ 4:4:4 12 bit

//	Row 304 is the header: 3G Level B, 2×20 lines from SMPTE 425 §5, VPID described in 425 §5.2
//	Rows 305 - 309 are unsupported 720p frame rates
//	Row 310
{ DualLink3_0,		NTV2_FORMAT_720p_5000,				NTV2_FBF_10BIT_YCBCR,	0x8B490001, 0x8B490001, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 91

//	Row 311
{ DualLink3_0,		NTV2_FORMAT_720p_5994,				NTV2_FBF_10BIT_YCBCR,	0x8B4A0001, 0x8B4A0001, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 92

//	Row 312
{ DualLink3_0,		NTV2_FORMAT_720p_6000,				NTV2_FBF_10BIT_YCBCR,	0x8B4B0001, 0x8B4B0001, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 93

//	Row 313 is the header: 3G Level B, 2×080 lines from SMPTE 425 §5, VPID described in 425 §5.2
//	Row 314
{ DualLink3_0,		NTV2_FORMAT_1080i_5000,				NTV2_FBF_10BIT_YCBCR,	0x8C052001, 0x8C052001, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 94

//	Row 315
{ DualLink3_0,		NTV2_FORMAT_1080i_5994,				NTV2_FBF_10BIT_YCBCR,	0x8C062001, 0x8C062001, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 95

//	Row 316
{ DualLink3_0,		NTV2_FORMAT_1080i_6000,				NTV2_FBF_10BIT_YCBCR,	0x8C072001, 0x8C072001, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 96

//	Row 317
{ DualLink3_0,		NTV2_FORMAT_1080p_2398,				NTV2_FBF_10BIT_YCBCR,	0x8CC22001, 0x8CC22001, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 97

//	Row 318
{ DualLink3_0,		NTV2_FORMAT_1080p_2400,				NTV2_FBF_10BIT_YCBCR,	0x8CC32001, 0x8CC32001, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 98

//	Row 319
{ DualLink3_0,		NTV2_FORMAT_1080p_2500,				NTV2_FBF_10BIT_YCBCR,	0x8CC52001, 0x8CC52001, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 99

//	Row 320
{ DualLink3_0,		NTV2_FORMAT_1080p_2997,				NTV2_FBF_10BIT_YCBCR,	0x8CC62001, 0x8CC62001, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 100

//	Row 321
{ DualLink3_0,		NTV2_FORMAT_1080p_3000,				NTV2_FBF_10BIT_YCBCR,	0x8CC72001, 0x8CC72001, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 101

//	Row 322
{ DualLink3_0,		NTV2_FORMAT_1080psf_2398,			NTV2_FBF_10BIT_YCBCR,	0x8C422001, 0x8C422001, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 102

//	Row 323
{ DualLink3_0,		NTV2_FORMAT_1080psf_2400,			NTV2_FBF_10BIT_YCBCR,	0x8C432001, 0x8C432001, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 103

//	Row 324
{ DualLink3_0,		NTV2_FORMAT_1080psf_2500_2,			NTV2_FBF_10BIT_YCBCR,	0x8C452001, 0x8C452001, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 104

//	Row 325
{ DualLink3_0,		NTV2_FORMAT_1080psf_2997_2,			NTV2_FBF_10BIT_YCBCR,	0x8C462001, 0x8C462001, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 105

//	Row 326
{ DualLink3_0,		NTV2_FORMAT_1080psf_3000_2,			NTV2_FBF_10BIT_YCBCR,	0x8C472001, 0x8C472001, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 106

//	Row 327 is the header: 3G Level B, 2×83/576 lines from SMPTE 425 §5, VPID described in 425 §5.2
//	Row 328
{ DualLink3_0,		NTV2_FORMAT_625_5000,				NTV2_FBF_10BIT_YCBCR,	0x8D050001, 0x8D050001, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 107

//	Row 329
{ DualLink3_0,		NTV2_FORMAT_525_5994,				NTV2_FBF_10BIT_YCBCR,	0x8D060001, 0x8D060001, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 108

//	Row 330 is the header: Dual link from SMPTE 372M §4.1, VPID described in 372M Annex A
//	Row 331
{ DualLink1_5,		NTV2_FORMAT_1080p_5000_B,			NTV2_FBF_10BIT_YCBCR,	0x87492001, ZeroedVPID, 0x87492041, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 109

//	Row 332
{ DualLink1_5,		NTV2_FORMAT_1080p_5994_B,			NTV2_FBF_10BIT_YCBCR,	0x874A2001, ZeroedVPID, 0x874A2041, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 110

//	Row 333
{ DualLink1_5,		NTV2_FORMAT_1080p_6000_B,			NTV2_FBF_10BIT_YCBCR,	0x874B2001, ZeroedVPID, 0x874B2041, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 111

//	Row 334 is the header: Dual link from SMPTE 372M §4.2, VPID described in 372M Annex A
//	Row 335
{ DualLink1_5,		NTV2_FORMAT_1080i_5000,				NTV2_FBF_10BIT_DPX,		0x87052201, ZeroedVPID, 0x87052241, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 112

//	Row 336
{ DualLink1_5,		NTV2_FORMAT_1080i_5994,				NTV2_FBF_10BIT_DPX,		0x87062201, ZeroedVPID, 0x87062241, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 113

//	Row 337
{ DualLink1_5,		NTV2_FORMAT_1080i_6000,				NTV2_FBF_10BIT_DPX,		0x87072201, ZeroedVPID, 0x87072241, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 114

//	Row 338
{ DualLink1_5,		NTV2_FORMAT_1080p_2398,				NTV2_FBF_10BIT_DPX,		0x87C22201, ZeroedVPID,	0x87C22241, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 115

//	Row 339
{ DualLink1_5,		NTV2_FORMAT_1080p_2400,				NTV2_FBF_10BIT_DPX,		0x87C32201, ZeroedVPID,	0x87C32241, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 116

//	Row 340
{ DualLink1_5,		NTV2_FORMAT_1080p_2500,				NTV2_FBF_10BIT_DPX,		0x87C52201, ZeroedVPID,	0x87C52241, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 117

//	Row 341
{ DualLink1_5,		NTV2_FORMAT_1080p_2997,				NTV2_FBF_10BIT_DPX,		0x87C62201, ZeroedVPID,	0x87C62241, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 118

//	Row 342
{ DualLink1_5,		NTV2_FORMAT_1080p_3000,				NTV2_FBF_10BIT_DPX,		0x87C72201, ZeroedVPID,	0x87C72241, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 119

//	Row 343
{ DualLink1_5,		NTV2_FORMAT_1080psf_2398,			NTV2_FBF_10BIT_DPX,		0x87422201, ZeroedVPID,	0x87422241, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 120

//	Row 344
{ DualLink1_5,		NTV2_FORMAT_1080psf_2400,			NTV2_FBF_10BIT_DPX,		0x87432201, ZeroedVPID,	0x87432241, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 121

//	Row 345
{ DualLink1_5,		NTV2_FORMAT_1080psf_2500_2,			NTV2_FBF_10BIT_DPX,		0x87452201, ZeroedVPID,	0x87452241, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 122

//	Row 346
{ DualLink1_5,		NTV2_FORMAT_1080psf_2997_2,			NTV2_FBF_10BIT_DPX,		0x87462201, ZeroedVPID,	0x87462241, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 123

//	Row 347
{ DualLink1_5,		NTV2_FORMAT_1080psf_3000_2,			NTV2_FBF_10BIT_DPX,		0x87472201, ZeroedVPID,	0x87472241, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 124

//	Rows 348 - 360 are unsupported pixel format GBRA 4:4:4:4 10 bit

//	Row 361 is the header: Dual link from SMPTE 372M §4.3, VPID described in 372M Annex A
//	Row 362
{ DualLink1_5,		NTV2_FORMAT_1080i_5000,				NTV2_FBF_48BIT_RGB,		0x87052202, ZeroedVPID, 0x87052242, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 125

//	Row 363
{ DualLink1_5,		NTV2_FORMAT_1080i_5994,				NTV2_FBF_48BIT_RGB,		0x87062202, ZeroedVPID, 0x87062242, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 126

//	Row 364
{ DualLink1_5,		NTV2_FORMAT_1080i_6000,				NTV2_FBF_48BIT_RGB,		0x87072202, ZeroedVPID, 0x87072242, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 127

//	Row 365
{ DualLink1_5,		NTV2_FORMAT_1080p_2398,				NTV2_FBF_48BIT_RGB,		0x87C22202, ZeroedVPID, 0x87C22242, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 128

//	Row 366
{ DualLink1_5,		NTV2_FORMAT_1080p_2400,				NTV2_FBF_48BIT_RGB,		0x87C32202, ZeroedVPID, 0x87C32242, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 129

//	Row 367
{ DualLink1_5,		NTV2_FORMAT_1080p_2500,				NTV2_FBF_48BIT_RGB,		0x87C52202, ZeroedVPID, 0x87C52242, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 130

//	Row 368
{ DualLink1_5,		NTV2_FORMAT_1080p_2997,				NTV2_FBF_48BIT_RGB,		0x87C62202, ZeroedVPID, 0x87C62242, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 131

//	Row 369
{ DualLink1_5,		NTV2_FORMAT_1080p_3000,				NTV2_FBF_48BIT_RGB,		0x87C72202, ZeroedVPID, 0x87C72242, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 132

//	Row 370
{ DualLink1_5,		NTV2_FORMAT_1080psf_2398,			NTV2_FBF_48BIT_RGB,		0x87422202, ZeroedVPID, 0x87422242, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 133

//	Row 371
{ DualLink1_5,		NTV2_FORMAT_1080psf_2400,			NTV2_FBF_48BIT_RGB,		0x87432202, ZeroedVPID, 0x87432242, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 134

//	Row 372
{ DualLink1_5,		NTV2_FORMAT_1080psf_2500_2,			NTV2_FBF_48BIT_RGB,		0x87452202, ZeroedVPID, 0x87452242, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 135

//	Row 373
{ DualLink1_5,		NTV2_FORMAT_1080psf_2997_2,			NTV2_FBF_48BIT_RGB,		0x87462202, ZeroedVPID, 0x87462242, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 136

//	Row 374
{ DualLink1_5,		NTV2_FORMAT_1080psf_3000_2,			NTV2_FBF_48BIT_RGB,		0x87472202, ZeroedVPID, 0x87472242, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 137

//	Row 375 is the header: Dual link from SMPTE 372M §4.4, VPID described in 372M Annex A
//	Rows 376 - 388 are unsupported pixel format YCbCr 4:4:4 10 bit
//	Rows 389 - 401 are unsupported pixel format YCbCr 4:4:4 12 bit
//	Rows 402 - 414 are unsupported pixel format YCbCrA 4:4:4:4 10 bit

//	Row 415 is the header: Dual link from SMPTE 372M §4.5, VPID described in 372M Annex A
//	Rows 416 - 429 are unsupported pixel format YCbCr 4:2:2 12 bit
//	Rows 430 - 441 are unsupported pixel format YCbCrA 4:2:2 12 bit

//	Row 442 is the header: 2K dual link from SMPTE 428-9, VPID described in 372M Annex A
//	Row 443
{ DualLink1_5,		NTV2_FORMAT_1080p_2K_2398,			NTV2_FBF_48BIT_RGB,		0x87C24202, ZeroedVPID, 0x87C24242, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 138

//	Row 444
{ DualLink1_5,		NTV2_FORMAT_1080p_2K_2400,			NTV2_FBF_48BIT_RGB,		0x87C34202, ZeroedVPID, 0x87C34242, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 139

//	Row 445
{ DualLink1_5,		NTV2_FORMAT_1080p_2K_2500,			NTV2_FBF_48BIT_RGB,		0x87C54202, ZeroedVPID, 0x87C54242, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 140

//	Row 446
{ DualLink1_5,		NTV2_FORMAT_1080p_2K_2997,			NTV2_FBF_48BIT_RGB,		0x87C64202, ZeroedVPID, 0x87C64242, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 141

//	Row 447
{ DualLink1_5,		NTV2_FORMAT_1080p_2K_3000,			NTV2_FBF_48BIT_RGB,		0x87C74202, ZeroedVPID, 0x87C74242, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 142

//	Row 448
{ DualLink1_5,		NTV2_FORMAT_1080psf_2K_2398,		NTV2_FBF_48BIT_RGB,		0x87424202, ZeroedVPID, 0x87424242, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 143

//	Row 449
{ DualLink1_5,		NTV2_FORMAT_1080psf_2K_2400,		NTV2_FBF_48BIT_RGB,		0x87434202, ZeroedVPID, 0x87434242, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 144

//	Row 450
{ DualLink1_5,		NTV2_FORMAT_1080psf_2K_2500,		NTV2_FBF_48BIT_RGB,		0x87454202, ZeroedVPID, 0x87454242, ZeroedVPID, ZeroedVPID, ZeroedVPID},	// 145

//	Row 451 has unsupported video format 1080psf_2K_2997
//	Row 452 has unsupported video format 1080psf_2K_3000

//	Rows 453 - 462 are unsupported pixel format XYZ 4:4:4 12 bit

//	End of spreadsheet SMPTE 352M VPID list.xls

//	Start of spreadsheet 4KVPID.xls

//	Row 1 is the header: 4K Quad HD
//	******** THIS SECTION DIFFERS FROM THE SPREADSHEET ********
//	******** LINK IDENTIFICATION HAS BEEN REMOVED TO COMPLY WITH SMPTE 425-5 ANNEX B ********
//	Row 2 is the header: HD 1080 line formats defined in SMPTE 274M, VPID described in 352M §A.5
//	Rows 3 - 5 are unsupported video format 4x1920x1080i
//	Row 6
{ QuadLink1_5,		NTV2_FORMAT_4x1920x1080p_2398,		NTV2_FBF_10BIT_YCBCR,	0x85C22001, ZeroedVPID, 0x85C22041, ZeroedVPID, 0x85C22081, 0x85C220C1},	// 146

//	Row 7
{ QuadLink1_5,		NTV2_FORMAT_4x1920x1080p_2400,		NTV2_FBF_10BIT_YCBCR,	0x85C32001, ZeroedVPID, 0x85C32041, ZeroedVPID, 0x85C32081, 0x85C320C1},	// 147

//	Row 8
{ QuadLink1_5,		NTV2_FORMAT_4x1920x1080p_2500,		NTV2_FBF_10BIT_YCBCR,	0x85C52001, ZeroedVPID, 0x85C52041, ZeroedVPID, 0x85C52081, 0x85C520C1},	// 148

//	Row 9
{ QuadLink1_5,		NTV2_FORMAT_4x1920x1080p_2997,		NTV2_FBF_10BIT_YCBCR,	0x85C62001, ZeroedVPID, 0x85C62041, ZeroedVPID, 0x85C62081, 0x85C620C1},	// 149

//	Row 10
{ QuadLink1_5,		NTV2_FORMAT_4x1920x1080p_3000,		NTV2_FBF_10BIT_YCBCR,	0x85C72001, ZeroedVPID, 0x85C72041, ZeroedVPID, 0x85C72081, 0x85C720C1},	// 150

//	Row 11
{ QuadLink1_5,		NTV2_FORMAT_4x1920x1080psf_2398,	NTV2_FBF_10BIT_YCBCR,	0x85422001, ZeroedVPID, 0x85422041, ZeroedVPID, 0x85422081, 0x854220C1},	// 151

//	Row 12
{ QuadLink1_5,		NTV2_FORMAT_4x1920x1080psf_2400,	NTV2_FBF_10BIT_YCBCR,	0x85432001, ZeroedVPID, 0x85432041, ZeroedVPID, 0x85432081, 0x854320C1},	// 152

//	Row 13
{ QuadLink1_5,		NTV2_FORMAT_4x1920x1080psf_2500,	NTV2_FBF_10BIT_YCBCR,	0x85452001, ZeroedVPID, 0x85452041, ZeroedVPID, 0x85452081, 0x854520C1},	// 153

//	Row 14 is unsupported video format 4x1920x1080psf_2997
//	Row 15 is unsupported video format 4x1920x1080psf_3000
//	Row 16 is blank

//	Row 17 is the header: 4K Quad 3G
//	Row 18 is the header: 3G Level A mapping structure 1 from SMPTE 425 §3.2.1, VPID described in 425 §4.1.6
//	Row 19
{ QuadLink3_0A,		NTV2_FORMAT_4x1920x1080p_5000,		NTV2_FBF_10BIT_YCBCR,	0x89C98001, ZeroedVPID, 0x89C98041, ZeroedVPID, 0x89C98081, 0x89C980C1},	// 154

//	Row 20
{ QuadLink3_0A,		NTV2_FORMAT_4x1920x1080p_5994,		NTV2_FBF_10BIT_YCBCR,	0x89CA8001, ZeroedVPID, 0x89CA8041, ZeroedVPID, 0x89CA8081, 0x89CA80C1},	// 155

//	Row 21
{ QuadLink3_0A,		NTV2_FORMAT_4x1920x1080p_6000,		NTV2_FBF_10BIT_YCBCR,	0x89CB8001, ZeroedVPID, 0x89CB8041, ZeroedVPID, 0x89CB8081, 0x89CB80C1},	// 156

//	Row 22 is blank
//	Rows 23 - 30 are unsupported pixel format YCbCr 4:4:4 12 bit
//	Rows 31 - 33 are unsupported video format 4x1920x1080i
//	Row 34
{ QuadLink3_0A,		NTV2_FORMAT_4x1920x1080p_3000,		NTV2_FBF_48BIT_RGB,		0x8AC72202, 0x8AC72202, 0x8AC72242, 0x8AC72242, 0x8AC72282, 0x8AC722C2},	// 157

//	Row 35
{ QuadLink3_0A,		NTV2_FORMAT_4x1920x1080p_2997,		NTV2_FBF_48BIT_RGB,		0x8AC62202, 0x8AC62202, 0x8AC62242, 0x8AC62242, 0x8AC62282, 0x8AC622C2},	// 158

//	Row 36
{ QuadLink3_0A,		NTV2_FORMAT_4x1920x1080p_2500,		NTV2_FBF_48BIT_RGB,		0x8AC52202, 0x8AC52202, 0x8AC52242, 0x8AC52242, 0x8AC52282, 0x8AC522C2},	// 159

//	Row 37
{ QuadLink3_0A,		NTV2_FORMAT_4x1920x1080p_2400,		NTV2_FBF_48BIT_RGB,		0x8AC32202, 0x8AC32202, 0x8AC32242, 0x8AC32242, 0x8AC32282, 0x8AC322C2},	// 160

//	Row 38
{ QuadLink3_0A,		NTV2_FORMAT_4x1920x1080p_2398,		NTV2_FBF_48BIT_RGB,		0x8AC22202, 0x8AC22202, 0x8AC22242, 0x8AC22242, 0x8AC22282, 0x8AC222C2},	// 161

//	Row 39 is blank
//	Row 40
{ QuadLink3_0B,		NTV2_FORMAT_4x1920x1080p_5000,		NTV2_FBF_10BIT_YCBCR,	0x8A492001, 0x8A492041, 0x8A492041, 0x8A492041, 0x8A492081, 0x8A4920C1},	// 162

//	Row 41
{ QuadLink3_0B,		NTV2_FORMAT_4x1920x1080p_5994,		NTV2_FBF_10BIT_YCBCR,	0x8A4A2001, 0x8A4A2041, 0x8A4A2041, 0x8A4A2041, 0x8A4A2081, 0x8A4A20C1},	// 163

//	Row 42
{ QuadLink3_0B,		NTV2_FORMAT_4x1920x1080p_6000,		NTV2_FBF_10BIT_YCBCR,	0x8A4B2001, 0x8A4B2041, 0x8A4B2041, 0x8A4B2041, 0x8A4B2081, 0x8A4B20C1},	// 164

//	Row 43 is blank
//	Rows 44 - 46 are unsupported video format 4x1920x1080i
//	Row 47
{ QuadLink3_0B,		NTV2_FORMAT_4x1920x1080p_2398,		NTV2_FBF_10BIT_DPX,		0x8AC22201, 0x8AC22241, 0x8AC22241, 0x8AC22241, 0x8AC22281, 0x8AC222C1},	// 165

//	Row 48
{ QuadLink3_0B,		NTV2_FORMAT_4x1920x1080p_2400,		NTV2_FBF_10BIT_DPX,		0x8AC32201, 0x8AC32241, 0x8AC32241, 0x8AC32241, 0x8AC32281, 0x8AC322C1},	// 166

//	Row 49
{ QuadLink3_0B,		NTV2_FORMAT_4x1920x1080p_2500,		NTV2_FBF_10BIT_DPX,		0x8AC52201, 0x8AC52241, 0x8AC52241, 0x8AC52241, 0x8AC52281, 0x8AC522C1},	// 167

//	Row 50
{ QuadLink3_0B,		NTV2_FORMAT_4x1920x1080p_2997,		NTV2_FBF_10BIT_DPX,		0x8AC62201, 0x8AC62241, 0x8AC62241, 0x8AC62241, 0x8AC62281, 0x8AC622C1},	// 168

//	Row 51
{ QuadLink3_0B,		NTV2_FORMAT_4x1920x1080p_3000,		NTV2_FBF_10BIT_DPX,		0x8AC72201, 0x8AC72241, 0x8AC72241, 0x8AC72241, 0x8AC72281, 0x8AC722C1},	// 169

//	Row 52
{ QuadLink3_0B,		NTV2_FORMAT_4x1920x1080psf_2398,	NTV2_FBF_10BIT_DPX,		0x8A422201, 0x8A422241, 0x8A422241, 0x8A422241, 0x8A422281, 0x8A4222C1},	// 170

//	Row 53
{ QuadLink3_0B,		NTV2_FORMAT_4x1920x1080psf_2400,	NTV2_FBF_10BIT_DPX,		0x8A432201, 0x8A432241, 0x8A432241, 0x8A432241, 0x8A432281, 0x8A4322C1},	// 171

//	Row 54
{ QuadLink3_0B,		NTV2_FORMAT_4x1920x1080psf_2500,	NTV2_FBF_10BIT_DPX,		0x8A452201, 0x8A452241, 0x8A452241, 0x8A452241, 0x8A452281, 0x8A4522C1},	// 172

//	Rows 55 - 56 are unsupported video formats 4x1920x1080psf_2997 and 4x1920x1080psf_3000
//	Rows 57 - 69 are unsupported pixel format GBRA 4:4:4:4 10 bit
//	Row 70 is blank
//	Rows 71 - 73 are unsupported video format 4x1920x1080i
//	Row 74
{ QuadLink3_0B,		NTV2_FORMAT_4x1920x1080p_2398,		NTV2_FBF_48BIT_RGB,		0x8AC22202, 0x8AC22242, 0x8AC22242, 0x8AC22242, 0x8AC22282, 0x8AC222C2},	// 173

//	Row 75
{ QuadLink3_0B,		NTV2_FORMAT_4x1920x1080p_2400,		NTV2_FBF_48BIT_RGB,		0x8AC32202, 0x8AC32242, 0x8AC32242, 0x8AC32242, 0x8AC32282, 0x8AC322C2},	// 174

//	Row 76
{ QuadLink3_0B,		NTV2_FORMAT_4x1920x1080p_2500,		NTV2_FBF_48BIT_RGB,		0x8AC52202, 0x8AC52242, 0x8AC52242, 0x8AC52242, 0x8AC52282, 0x8AC522C2},	// 175

//	Row 77
{ QuadLink3_0B,		NTV2_FORMAT_4x1920x1080p_2997,		NTV2_FBF_48BIT_RGB,		0x8AC62202, 0x8AC62242, 0x8AC62242, 0x8AC62242, 0x8AC62282, 0x8AC622C2},	// 176

//	Row 78
{ QuadLink3_0B,		NTV2_FORMAT_4x1920x1080p_3000,		NTV2_FBF_48BIT_RGB,		0x8AC72202, 0x8AC72242, 0x8AC72242, 0x8AC72242, 0x8AC72282, 0x8AC722C2},	// 177

//	Row 79
{ QuadLink3_0B,		NTV2_FORMAT_4x1920x1080psf_2398,	NTV2_FBF_48BIT_RGB,		0x8A422202, 0x8A422242, 0x8A422242, 0x8A422242, 0x8A422282, 0x8A4222C2},	// 178

//	Row 80
{ QuadLink3_0B,		NTV2_FORMAT_4x1920x1080psf_2400,	NTV2_FBF_48BIT_RGB,		0x8A432202, 0x8A432242, 0x8A432242, 0x8A432242, 0x8A432282, 0x8A4322C2},	// 179

//	Row 81
{ QuadLink3_0B,		NTV2_FORMAT_4x1920x1080psf_2500,	NTV2_FBF_48BIT_RGB,		0x8A452202, 0x8A452242, 0x8A452242, 0x8A452242, 0x8A452282, 0x8A4522C2},	// 180

//	Rows 82 - 83 are unsupported video formats 4x1920x1080psf_2997 and 4x1920x1080psf_3000
//	Row 84 is blank
//	Rows 85  - 97  are unsupported pixel format YCbCr 4:4:4 10 bit
//	Rows 98  - 110 are unsupported pixel format YCbCr 4:4:4 12 bit
//	Rows 111 - 110 are unsupported pixel format YCbCrA 4:4:4:4 10 bit
//	Row 124 is blank
//	Rows 125 - 137 are unsupported pixel format YCbCr 4:4:4 12 bit
//	Rows 138 - 150 are unsupported pixel format YCbCrA 4:4:4:4 12 bit

//	End of spreadsheet 4KVPID.xls

//	End of list marker
{ MaxRouting,		NTV2_FORMAT_UNKNOWN,				NTV2_FBF_INVALID,		ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID, ZeroedVPID}		// End of Array
};

bool FillTestList (TestVector & vector)
{
	for (size_t i = 0; i < (sizeof (testArray) / sizeof (TestEntry)); i++)
	{
		vector.push_back (testArray [i]);
	}

	return true;
}

