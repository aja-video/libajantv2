/**
	@brief		Declared the now-obsolete CXena2Routing class.
	@deprecated	Include "ntv2signalrouter.h" instead.
	@copyright	(C) 2006-2016 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#ifndef XENA2ROUTING_H
	#define XENA2ROUTING_H

	#if defined (NTV2_DEPRECATE)
		#error	"'xena2routing.h' is deprecated -- include 'ntv2signalrouter.h' instead"
	#else
		#include "ntv2signalrouter.h"
	#endif	//	else !defined (NTV2_DEPRECATE)

#endif	//	XENA2ROUTING_H
