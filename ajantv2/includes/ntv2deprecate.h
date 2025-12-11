/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2deprecate.h
	@brief		Deprecation control macros.
	@copyright	(C) 2006-2022 AJA Video Systems, Inc.
**/

#ifndef NTV2DEPRECATE_H
#define NTV2DEPRECATE_H

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////	DEPRECATION CONTROL MACROS
////////
////////	These macros control which deprecated symbols and APIs are included or excluded from compilation.
////////
////////	-	To activate/include the symbols/APIs that were deprecated in a particular SDK, comment out
////////		(undefine) the SDK's corresponding macro.
////////
////////	-	To deactivate/exclude the symbols/APIs that were deprecated in a particular SDK, leave the
////////		SDK's corresponding macro defined.
////////
////////	-	NTV2_DEPRECATE was the first deprecation control macro, first introduced just before SDK 12.4.
////////
////////	-	Starting in SDK 12.5, additional version-specific macros were added to delineate newly-deprecated
////////		symbols and APIs for each SDK release. Then in SDK 17.0, all prior deprecated symbols and APIs from
////////		SDKs 14.3 and earlier were removed from the source files, making these macros obsolete:
////////			NTV2_DEPRECATE			NTV2_DEPRECATE_12_5		NTV2_DEPRECATE_12_6		NTV2_DEPRECATE_12_7
////////			NTV2_DEPRECATE_13_0		NTV2_DEPRECATE_13_1
////////			NTV2_DEPRECATE_14_0		NTV2_DEPRECATE_14_1		NTV2_DEPRECATE_14_2		NTV2_DEPRECATE_14_3
////////			NTV2_DEPRECATE_15_0		NTV2_DEPRECATE_15_1		NTV2_DEPRECATE_15_2		NTV2_DEPRECATE_15_3
////////			NTV2_DEPRECATE_15_4		NTV2_DEPRECATE_15_5		NTV2_DEPRECATE_15_6
////////
////////	WARNING:	Do not sparsely mix-and-match across SDK versions.
////////				It's best to deactivate/exclude symbols/APIs contiguously from the oldest SDK
////////				and continue deactivating/excluding to the later SDK at which symbols/APIs should
////////				be retained/included.
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define NTV2_DEPRECATE			//	Do not undefine -- all symbols/APIs first deprecated in SDK 12.4 or earlier have been removed
#define NTV2_DEPRECATE_12_5		//	Do not undefine -- all symbols/APIs first deprecated in SDK 12.5 have been removed
#define NTV2_DEPRECATE_12_6		//	Do not undefine -- all symbols/APIs first deprecated in SDK 12.6 have been removed
#define NTV2_DEPRECATE_12_7		//	Do not undefine -- all symbols/APIs first deprecated in SDK 12.7 have been removed
#define NTV2_DEPRECATE_13_0		//	Do not undefine -- all symbols/APIs first deprecated in SDK 13.0 have been removed
#define NTV2_DEPRECATE_13_1		//	Do not undefine -- all symbols/APIs first deprecated in SDK 13.1 have been removed
#define NTV2_DEPRECATE_14_0		//	Do not undefine -- all symbols/APIs first deprecated in SDK 14.0 have been removed
#define NTV2_DEPRECATE_14_1		//	Do not undefine -- all symbols/APIs first deprecated in SDK 14.1 (never released) have been removed
#define NTV2_DEPRECATE_14_2		//	Do not undefine -- all symbols/APIs first deprecated in SDK 14.2 have been removed
#define NTV2_DEPRECATE_14_3		//	Do not undefine -- all symbols/APIs first deprecated in SDK 14.3 have been removed
#define NTV2_DEPRECATE_15_0		//	Do not undefine -- all symbols/APIs first deprecated in SDK 15.0 have been removed
#define NTV2_DEPRECATE_15_1		//	Do not undefine -- all symbols/APIs first deprecated in SDK 15.1 have been removed
#define NTV2_DEPRECATE_15_2		//	Do not undefine -- all symbols/APIs first deprecated in SDK 15.2 have been removed
#define NTV2_DEPRECATE_15_3		//	Do not undefine -- all symbols/APIs first deprecated in SDK 15.3 (never released) have been removed
#define NTV2_DEPRECATE_15_5		//	Do not undefine -- all symbols/APIs first deprecated in SDK 15.5 have been removed
#define NTV2_DEPRECATE_15_6		//	Do not undefine -- all symbols/APIs first deprecated in SDK 15.6 (never released) have been removed
//#define NTV2_DEPRECATE_16_0		//	If defined, excludes all symbols/APIs first deprecated in SDK 16.0
//#define NTV2_DEPRECATE_16_1		//	If defined, excludes all symbols/APIs first deprecated in SDK 16.1
//#define NTV2_DEPRECATE_16_2		//	If defined, excludes all symbols/APIs first deprecated in SDK 16.2
//#define NTV2_DEPRECATE_16_3		//	If defined, excludes all symbols/APIs first deprecated in SDK 16.3 (never released)
//#define NTV2_DEPRECATE_17_0		//	If defined, excludes all symbols/APIs first deprecated in SDK 17.0
//#define NTV2_DEPRECATE_17_1		//	If defined, excludes all symbols/APIs first deprecated in SDK 17.1
//#define NTV2_DEPRECATE_17_2		//	If defined, excludes all symbols/APIs first deprecated in SDK 17.2 (never released)
//#define NTV2_DEPRECATE_17_5		//	If defined, excludes all symbols/APIs first deprecated in SDK 17.5
//#define NTV2_DEPRECATE_17_6		//	If defined, excludes all symbols/APIs first deprecated in SDK 17.6
//#define NTV2_DEPRECATE_18_0		//	If defined, excludes all symbols/APIs first deprecated in SDK 18.0

#endif // NTV2DEPRECATE_H
