/**
	@file		xenaserialcontrol.h
	@deprecated	Include 'ntv2serialcontrol.h' instead.
	@copyright	(C) 2004-2016 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#ifndef _XENASERIALCONTROL_H
	#define _XENASERIALCONTROL_H

	#if defined (NTV2_DEPRECATE)
		#if defined (MSWindows)
			#pragma message("'xenaserialcontrol.h' is deprecated -- include 'ntv2serialcontrol.h' instead")
		#else
			#warning "'xenaserialcontrol.h' is deprecated -- include 'ntv2serialcontrol.h' instead"
		#endif
	#else
		#include "ntv2serialcontrol.h"
	#endif

#endif	//	_XENASERIALCONTROL_H
