/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
#ifndef NTV2_XPT_LOOKUP_H
#define NTV2_XPT_LOOKUP_H

typedef struct
{
	ULWord	registerNumber;
	ULWord	registerMask;
	UByte	registerShift;
} NTV2XptLookupEntry;

#define XPT_SDI_IN_1		(0x0700)
#define XPT_SDI_IN_2		(0x0701)
#define XPT_SDI_IN_3		(0x0702)
#define XPT_SDI_IN_4		(0x0703)
#define XPT_SDI_IN_5		(0x0704)
#define XPT_SDI_IN_6		(0x0705)
#define XPT_SDI_IN_7		(0x0706)
#define XPT_SDI_IN_8		(0x0707)

#define XPT_SDI_IN_1_DS2	(0x0708)
#define XPT_SDI_IN_2_DS2	(0x0709)
#define XPT_SDI_IN_3_DS2	(0x070A)
#define XPT_SDI_IN_4_DS2	(0x070B)
#define XPT_SDI_IN_5_DS2	(0x070C)
#define XPT_SDI_IN_6_DS2 	(0x070D)
#define XPT_SDI_IN_7_DS2	(0x070E)
#define XPT_SDI_IN_8_DS2	(0x070F)

#define XPT_FB_YUV_1		(0x0600)
#define XPT_FB_YUV_2		(0x0601)
#define XPT_FB_YUV_3		(0x0602)
#define XPT_FB_YUV_4		(0x0603)
#define XPT_FB_YUV_5		(0x0604)
#define XPT_FB_YUV_6		(0x0605)
#define XPT_FB_YUV_7		(0x0606)
#define XPT_FB_YUV_8		(0x0607)

#define XPT_FB_RGB_1		(0x0608)
#define XPT_FB_RGB_2		(0x0609)
#define XPT_FB_RGB_3		(0x060A)
#define XPT_FB_RGB_4		(0x060B)
#define XPT_FB_RGB_5		(0x060C)
#define XPT_FB_RGB_6		(0x060D)
#define XPT_FB_RGB_7		(0x060E)
#define XPT_FB_RGB_8		(0x060F)

#define XPT_FB_425_YUV_1	(0x0610)
#define XPT_FB_425_YUV_2	(0x0611)
#define XPT_FB_425_YUV_3	(0x0612)
#define XPT_FB_425_YUV_4	(0x0613)
#define XPT_FB_425_YUV_5	(0x0614)
#define XPT_FB_425_YUV_6	(0x0615)
#define XPT_FB_425_YUV_7	(0x0616)
#define XPT_FB_425_YUV_8	(0x0617)

#define XPT_FB_425_RGB_1	(0x0618)
#define XPT_FB_425_RGB_2	(0x0619)
#define XPT_FB_425_RGB_3	(0x061A)
#define XPT_FB_425_RGB_4	(0x061B)
#define XPT_FB_425_RGB_5	(0x061C)
#define XPT_FB_425_RGB_6	(0x061D)
#define XPT_FB_425_RGB_7	(0x061E)
#define XPT_FB_425_RGB_8	(0x061F)

#define XPT_HDMI_IN			(0x0500)
#define XPT_HDMI_IN_Q2		(0x0501)
#define XPT_HDMI_IN_Q3		(0x0502)
#define XPT_HDMI_IN_Q4		(0x0503)
#define XPT_ANALOG_IN		(0x0400)
#define XPT_4K_DC			(0x0300)

#endif	//	NTV2_XPT_LOOKUP_H

