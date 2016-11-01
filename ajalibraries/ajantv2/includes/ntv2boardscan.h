/**
	@file		ntv2boardscan.h
	@deprecated	Include ntv2devicescanner.h instead.
	@copyright	(C) 2004-2016 AJA Video Systems, Inc.	Proprietary and confidential information.
**/
#ifndef NTV2BOARDSCAN_H
	#define NTV2BOARDSCAN_H

	#if defined (NTV2_DEPRECATE)
		#if defined (MSWindows)
			#pragma message("'ntv2boardscan.h' is deprecated -- include 'ntv2devicescanner.h' instead")
		#else
			#warning "'ntv2boardscan.h' is deprecated -- include 'ntv2devicescanner.h' instead"
		#endif
	#else
		#include "ntv2devicescanner.h"
	#endif
#endif	//	NTV2BOARDSCAN_H
