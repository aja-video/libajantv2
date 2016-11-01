#ifndef _VPID_TEST_H_
#define _VPID_TEST_H_

#include "ntv2publicinterface.h"

typedef enum
{
	SingleLink270,
	SingleLink1_5,
	DualLink1_5,
	SingleLink3_0A,
	SingleLink3_0B,
	SingleLink3RGB_A,
	QuadLink1_5,
	DualLink3_0,
	QuadLink3_0A,
	QuadLink3_0B,
	QuadLink1_5_TSI,
	DualLink3_0_TSI,
	QuadLink3_0A_TSI,
	QuadLink3_0B_TSI,

	MaxRouting
} RoutingControl;


typedef struct
{
	RoutingControl			routing;
	NTV2VideoFormat			videoFormat;
	NTV2FrameBufferFormat	pixelFormat;
	ULWord					vpidLink1_DS1;
	ULWord					vpidLink1_DS2;
	ULWord					vpidLink2_DS1;
	ULWord					vpidLink2_DS2;
	ULWord					vpidLink3_DS1;
	ULWord					vpidLink4_DS1;
} TestEntry;

#endif	//	_VPID_TET_H_

