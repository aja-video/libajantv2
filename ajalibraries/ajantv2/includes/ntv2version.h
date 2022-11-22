/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2version.h.in
	@brief		CMake template for generating ajantv2/includes/ntv2version.h, which populates the SDK version number.
    These variables are set in cmake/AJAVersionConfig.cmake in the base of this repo.
	@copyright	(C) 2013-2022 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef _NTV2VERSION_H_
#define _NTV2VERSION_H_

#include "ajaexport.h"

#include <string>

#define AJA_NTV2_SDK_VERSION_MAJOR		17		///< @brief The SDK major version number, an unsigned decimal integer.
#define AJA_NTV2_SDK_VERSION_MINOR		0		///< @brief The SDK minor version number, an unsigned decimal integer.
#define AJA_NTV2_SDK_VERSION_POINT		0		///< @brief The SDK "point" release version, an unsigned decimal integer.
#define AJA_NTV2_SDK_BUILD_NUMBER		1			///< @brief The SDK build number, an unsigned decimal integer.
#define AJA_NTV2_SDK_BUILD_DATETIME		"11/21/2022 +8:21:14:37"		///< @brief The date and time the SDK was built, in this format: "MM/DD/YYYY +8:hh:mm:ss"
#define AJA_NTV2_SDK_BUILD_TYPE			"d"			///< @brief The SDK build type, where "a"=alpha, "b"=beta, "d"=development, ""=release.

#define AJA_NTV2_SDK_VERSION	((AJA_NTV2_SDK_VERSION_MAJOR << 24) | (AJA_NTV2_SDK_VERSION_MINOR << 16) | (AJA_NTV2_SDK_VERSION_POINT << 8) | (AJA_NTV2_SDK_BUILD_NUMBER))
#define AJA_NTV2_SDK_VERSION_AT_LEAST(__a__,__b__)		(AJA_NTV2_SDK_VERSION >= (((__a__) << 24) | ((__b__) << 16)))
#define AJA_NTV2_SDK_VERSION_BEFORE(__a__,__b__)		(AJA_NTV2_SDK_VERSION < (((__a__) << 24) | ((__b__) << 16)))

#if !defined(NTV2_BUILDING_DRIVER)
	#include <string>
	AJAExport std::string NTV2Version (const bool inDetailed = false);	///< @returns a string containing SDK version information
	AJAExport const std::string & NTV2GitHash (void);		///< @returns the 40-character ID of the last commit for this SDK build
	AJAExport const std::string & NTV2GitHashShort (void);	///< @returns the 10-character ID of the last commit for this SDK build
#endif

#endif	//	_NTV2VERSION_H_
