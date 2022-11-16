/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2version.cpp
	@brief		Defines for the NTV2 SDK git hash, used by `ajantv2/includes/ntv2version.h`.
	See the `ajantv2/includes/ntv2version.h.in` template when building with with CMake.
	@copyright	(C) 2013-2022 AJA Video Systems, Inc.  All rights reserved.
**/

#include "ntv2version.h"

// NOTE: The following should be defined in the build script, i.e. CMake
// The hash should be precisely 40 characters, and the short hash 10 characters.
// Both hashes come from the git rev-parse command.

#if !defined(NTV2_BUILDING_DRIVER)
const std::string& NTV2GitHash()
{
	static std::string gitHash;
#if defined(AJA_GIT_COMMIT_HASH)
	gitHash = AJA_GIT_COMMIT_HASH;	
#endif
	return gitHash;
}

const std::string& NTV2GitHashShort()
{
	static std::string gitHash;
#if defined(AJA_GIT_COMMIT_HASH_SHORT)
	gitHash = AJA_GIT_COMMIT_HASH_SHORT;
#endif
	return gitHash;
}
#endif
