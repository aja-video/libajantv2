/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2version.h
	@brief		Defines for the NTV2 SDK version number, used by `ajantv2/includes/ntv2enums.h`.
	See the `ajantv2/includes/ntv2version.h.in` template when building with with CMake.
	@copyright	(C) 2013-2022 AJA Video Systems, Inc.  All rights reserved.
**/

#include "ajaexport.h"

//	Automated builds remove this and the following several lines		//	__AUTO_BUILD_REMOVE__
#if !defined (AJA_PATCHED)												//	__AUTO_BUILD_REMOVE__
	#define AJA_DECIMAL_PLACEHOLDER			0							//	__AUTO_BUILD_REMOVE__
	#define AJA_DATETIME_PLACEHOLDER		"00/00/0000 +8:00:00:00"	//	__AUTO_BUILD_REMOVE__
	#define AJA_STRING_PLACEHOLDER			"d"							//	__AUTO_BUILD_REMOVE__
#endif																	//	__AUTO_BUILD_REMOVE__

#define AJA_NTV2_SDK_VERSION_MAJOR		AJA_DECIMAL_PLACEHOLDER			///< @brief The SDK major version number, an unsigned decimal integer.
#define AJA_NTV2_SDK_VERSION_MINOR		AJA_DECIMAL_PLACEHOLDER			///< @brief The SDK minor version number, an unsigned decimal integer.
#define AJA_NTV2_SDK_VERSION_POINT		AJA_DECIMAL_PLACEHOLDER			///< @brief The SDK "point" release version, an unsigned decimal integer.
#define AJA_NTV2_SDK_BUILD_NUMBER		AJA_DECIMAL_PLACEHOLDER			///< @brief The SDK build number, an unsigned decimal integer.
#define AJA_NTV2_SDK_BUILD_DATETIME		AJA_DATETIME_PLACEHOLDER		///< @brief The date and time the SDK was built, in this format: "MM/DD/YYYY +8:hh:mm:ss"
#define AJA_NTV2_SDK_BUILD_TYPE			AJA_STRING_PLACEHOLDER			///< @brief The SDK build type, where "a"=alpha, "b"=beta, "d"=development, ""=release.

#define AJA_NTV2_SDK_VERSION	((AJA_NTV2_SDK_VERSION_MAJOR << 24) | (AJA_NTV2_SDK_VERSION_MINOR << 16) | (AJA_NTV2_SDK_VERSION_POINT << 8) | (AJA_NTV2_SDK_BUILD_NUMBER))
#define AJA_NTV2_SDK_VERSION_AT_LEAST(__a__,__b__)		(AJA_NTV2_SDK_VERSION >= (((__a__) << 24) | ((__b__) << 16)))
#define AJA_NTV2_SDK_VERSION_BEFORE(__a__,__b__)		(AJA_NTV2_SDK_VERSION < (((__a__) << 24) | ((__b__) << 16)))

#if !defined(NTV2_BUILDING_DRIVER)
extern "C" const char GitCommitHash[41];
extern "C" const char GitCommitHashShort[11];
AJAExport const char* NTV2GitHash();
AJAExport const char* NTV2GitHashShort();
#endif
