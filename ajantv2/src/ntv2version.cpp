/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2version.cpp
	@brief		Defines for the NTV2 SDK version helpers.
	See the `ajantv2/includes/ntv2version.h.in` template when building with CMake.
	@copyright	(C) 2013-2022 AJA Video Systems, Inc.  All rights reserved.
**/

#include "ntv2version.h"

#if !defined(NTV2_BUILDING_DRIVER)

	#include <sstream>

	//	NOTE:	AJA_GIT_COMMIT_HASH and AJA_GIT_COMMIT_HASH_SHORT are defined by the CMake build script:
	//			The hash should be precisely 40 characters, and the short hash 10 characters.
	//			Both hashes come from the git rev-parse command.

	const std::string & NTV2GitHash (void)
	{
		//	This is ugly, but it's guaranteed thread-safe:
		static const std::string sLongHash
		#if defined(AJA_GIT_COMMIT_HASH)
										(AJA_GIT_COMMIT_HASH)	
		#endif	//	defined(AJA_GIT_COMMIT_HASH)
																;
		return sLongHash;
	}

	const std::string & NTV2GitHashShort (void)
	{
		//	This is ugly, but it's guaranteed thread-safe, anwith no mutex needed:
		static const std::string sShortHash
		#if defined(AJA_GIT_COMMIT_HASH_SHORT)
										(AJA_GIT_COMMIT_HASH_SHORT)
		#endif	//	defined(AJA_GIT_COMMIT_HASH_SHORT)
																;
		return sShortHash;
	}

	std::string NTV2Version (const bool inDetailed)
	{
		std::ostringstream oss;
		if (inDetailed)
			oss	<< "NTV2 SDK version ";
		else
			oss << "v";
		oss << AJA_NTV2_SDK_VERSION_MAJOR << "." << AJA_NTV2_SDK_VERSION_MINOR << "." << AJA_NTV2_SDK_VERSION_POINT
			<< "." << AJA_NTV2_SDK_BUILD_NUMBER;
		if (inDetailed)
			oss << " built " << AJA_NTV2_SDK_BUILD_DATETIME << " from " << ::NTV2GitHash();
		else
			oss << " (" << ::NTV2GitHashShort() << ")";
		return oss.str();
	}
#endif	//	!defined(NTV2_BUILDING_DRIVER)
