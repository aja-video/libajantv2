/**
	@file		transcode.h
	@deprecated	Include 'ntv2transcode.h' instead.
	@copyright	(C) 2004-2016 AJA Video Systems, Inc.	Proprietary and confidential information.
**/
#ifndef TRANSCODE_H
	#define TRANSCODE_H

	#if defined (NTV2_DEPRECATE)
		#if defined (MSWindows)
			#pragma message("'transcode.h' is deprecated -- include 'ntv2transcode.h' instead")
		#else
			#warning "'transcode.h' is deprecated -- include 'ntv2transcode.h' instead"
		#endif
	#else
		#include "ntv2transcode.h"
	#endif
#endif	//	TRANSCODE_H
