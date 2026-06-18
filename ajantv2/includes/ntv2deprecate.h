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
//#define NTV2_DEPRECATE_18_1		//	If defined, excludes all symbols/APIs first deprecated in SDK 18.1


//////////////////////////////////////////////////////////////////////////////////
////////////////////////	NTV2_DEPRECATED_ Macros		//////////////////////////
//////////////////////////////////////////////////////////////////////////////////
//	These implement compile-time warnings for use of deprecated variables and functions
#define NTV2_DEPRECATED_INLINE						//	Just a marker/reminder
#define NTV2_DEPRECATED_FIELD						//	Just a marker/reminder
#define NTV2_DEPRECATED_VARIABLE					//	Just a marker/reminder
#define NTV2_DEPRECATED_TYPEDEF						//	Just a marker/reminder
#define NTV2_DEPRECATED_CLASS						//	Just a marker/reminder
#define NTV2_SHOULD_BE_DEPRECATED(__f__)			__f__
#define NTV2_SHOULD_DEPRECATE(__f__)				__f__
#define NTV2_MUST_DEPRECATE(__f__)					__f__
#define NTV2_WILL_BE_DEPRECATED(__f__)				__f__
#if defined(NTV2_BUILDING_DRIVER)
	//	Disable deprecation warnings in driver builds
	#define NTV2_DEPRECATED_f(__f__)				__f__
	#define NTV2_DEPRECATED_v(__v__)				__v__
	#define NTV2_DEPRECATED_vi(__v__, __i__)		__v__  = (__i__)
#elif defined(_MSC_VER) && _MSC_VER >= 1600
	//	Use __declspec(deprecated) for MSVC
	#define NTV2_DEPRECATED_f(__f__)				__declspec(deprecated) __f__
	#define NTV2_DEPRECATED_v(__v__)				__v__
	#define NTV2_DEPRECATED_vi(__v__, __i__)		__v__  = (__i__)
#elif defined(__clang__)
	//	Use __attribute__((deprecated)) for LLVM/Clang
	#define NTV2_DEPRECATED_f(__f__)				__f__  __attribute__((deprecated))
	#define NTV2_DEPRECATED_v(__v__)				__v__
	#define NTV2_DEPRECATED_vi(__v__, __i__)		__v__  = (__i__)
#elif defined(__GNUC__)
	#if __GNUC__ >= 4
		//	Use __attribute__((deprecated)) for GCC 4 or later
		#define NTV2_DEPRECATED_f(__f__)			__f__ __attribute__ ((deprecated))
		#define NTV2_DEPRECATED_v(__v__)			__v__
		#define NTV2_DEPRECATED_vi(__v__, __i__)	__v__  = (__i__)
	#else
		//	Disable deprecation warnings in GCC prior to GCC 4
		#define NTV2_DEPRECATED_f(__f__)			__f__
		#define NTV2_DEPRECATED_v(__v__)			__v__
		#define NTV2_DEPRECATED_vi(__v__, __i__)	__v__  = (__i__)
	#endif
#else
	//	Disable deprecation warnings
	#define NTV2_DEPRECATED_f(__f__)				__f__
	#define NTV2_DEPRECATED_v(__v__)				__v__
	#define NTV2_DEPRECATED_vi(__v__, __i__)		__v__  = (__i__)
#endif

#define NTV2_DEPRECATED_16_0(__f__)					NTV2_DEPRECATED_f(__f__)
#if defined(NTV2_DEPRECATE_16_0)
	#define NTV2_DEPRECATED_16_1(__f__)				NTV2_DEPRECATED_f(__f__)
#else//NTV2_DEPRECATE_16_0
	#define NTV2_DEPRECATED_16_1(__f__)				NTV2_WILL_BE_DEPRECATED(__f__)
#endif//NTV2_DEPRECATE_16_0
#if defined(NTV2_DEPRECATE_16_1)
	#define NTV2_DEPRECATED_16_2(__f__)				NTV2_DEPRECATED_f(__f__)
#else//NTV2_DEPRECATE_16_1
	#define NTV2_DEPRECATED_16_2(__f__)				NTV2_WILL_BE_DEPRECATED(__f__)
#endif//NTV2_DEPRECATE_16_1
#if defined(NTV2_DEPRECATE_16_2)
	#define NTV2_DEPRECATED_16_3(__f__)				NTV2_DEPRECATED_f(__f__)
#else//NTV2_DEPRECATE_16_2
	#define NTV2_DEPRECATED_16_3(__f__)				NTV2_WILL_BE_DEPRECATED(__f__)
#endif//NTV2_DEPRECATE_16_2
#if defined(NTV2_DEPRECATE_16_3)
	#define NTV2_DEPRECATED_17_0(__f__)				NTV2_DEPRECATED_f(__f__)
#else//NTV2_DEPRECATE_16_3
	#define NTV2_DEPRECATED_17_0(__f__)				NTV2_WILL_BE_DEPRECATED(__f__)
#endif//NTV2_DEPRECATE_16_3
#if defined(NTV2_DEPRECATE_17_0)
	#define NTV2_DEPRECATED_17_1(__f__)				NTV2_DEPRECATED_f(__f__)
#else//NTV2_DEPRECATE_17_0
	#define NTV2_DEPRECATED_17_1(__f__)				NTV2_WILL_BE_DEPRECATED(__f__)
#endif//NTV2_DEPRECATE_17_0
#if defined(NTV2_DEPRECATE_17_1)
	#define NTV2_DEPRECATED_17_2(__f__)				NTV2_DEPRECATED_f(__f__)
#else//NTV2_DEPRECATE_17_1
	#define NTV2_DEPRECATED_17_2(__f__)				NTV2_WILL_BE_DEPRECATED(__f__)
#endif//NTV2_DEPRECATE_17_1
#if defined(NTV2_DEPRECATE_17_2)
	#define NTV2_DEPRECATED_17_5(__f__)				NTV2_DEPRECATED_f(__f__)
#else//NTV2_DEPRECATE_17_2
	#define NTV2_DEPRECATED_17_5(__f__)				NTV2_WILL_BE_DEPRECATED(__f__)
#endif//NTV2_DEPRECATE_17_2
#if defined(NTV2_DEPRECATE_17_5)
	#define NTV2_DEPRECATED_17_6(__f__)				NTV2_DEPRECATED_f(__f__)
#else//NTV2_DEPRECATE_17_5
	#define NTV2_DEPRECATED_17_6(__f__)				NTV2_WILL_BE_DEPRECATED(__f__)
#endif//NTV2_DEPRECATE_17_5
#if defined(NTV2_DEPRECATE_17_6)
	#define NTV2_DEPRECATED_18_0(__f__)				NTV2_DEPRECATED_f(__f__)
#else//NTV2_DEPRECATE_17_6
	#define NTV2_DEPRECATED_18_0(__f__)				NTV2_WILL_BE_DEPRECATED(__f__)
#endif//NTV2_DEPRECATE_17_6
#if defined(NTV2_DEPRECATE_18_0)
	#define NTV2_DEPRECATED_18_1(__f__)				NTV2_DEPRECATED_f(__f__)
#else//NTV2_DEPRECATE_18_0
	#define NTV2_DEPRECATED_18_1(__f__)				NTV2_WILL_BE_DEPRECATED(__f__)
#endif//NTV2_DEPRECATE_18_0

#endif // NTV2DEPRECATE_H
