/**
	@file		resample.h
	@deprecated	Include 'ntv2resample.h' instead.
	@copyright	(C) 2004-2016 AJA Video Systems, Inc.	Proprietary and confidential information.
**/
#ifndef RESAMPLE_H
	#define RESAMPLE_H

	#if defined (NTV2_DEPRECATE)
		#if defined (MSWindows)
			#pragma message("'resample.h' is deprecated -- include 'ntv2resample.h' instead")
		#else
			#warning "'resample.h' is deprecated -- include 'ntv2resample.h' instead"		
		#endif
	#else
		#include "ntv2resample.h"
	#endif
#endif	//	RESAMPLE_H
