/**
	@file		verticalfilter.h
	@deprecated	Include 'ntv2verticalfilter.h' instead.
	@copyright	(C) 2004-2016 AJA Video Systems, Inc.	Proprietary and confidential information.
**/
#ifndef VERTICALFILTER_H
	#define VERTICALFILTER_H
	#if defined (NTV2_DEPRECATE)
		#if defined (MSWindows)
			#pragma message("'verticalfilter.h' is deprecated -- include 'ntv2verticalfilter.h' instead")
		#else
			#warning "'verticalfilter.h' is deprecated -- include 'ntv2verticalfilter.h' instead"
		#endif
	#else
		#include "ntv2verticalfilter.h"
	#endif
#endif	//	VERTICALFILTER_H
