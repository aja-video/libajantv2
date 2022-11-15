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

// #pragma comment(linker, "/include:GitCommitHash")
// #pragma comment(linker, "/include:GitCommitHashShort")

const char* NTV2GitHash()
{
#if !defined(AJA_GIT_COMMIT_HASH)
	static const char GitCommitHash[41] = "";
#else
	static const char GitCommitHash[41] = AJA_GIT_COMMIT_HASH;
#endif
	return &GitCommitHash[0];
}

const char* NTV2GitHashShort()
{
#if !defined(AJA_GIT_COMMIT_HASH_SHORT)
	static const char GitCommitHashShort[11] = "";
#else
	static const char GitCommitHashShort[11] = AJA_GIT_COMMIT_HASH_SHORT;
#endif
    return &GitCommitHashShort[0];
}

// #if defined(WIN64)
// #pragma comment(linker, "/include:GitVersionHash")
// #elif defined(WIN32)
// #pragma comment(linker, "/include:_GitVersionHash")
// #endif
