/**
	@file		export.h
	@copyright	Copyright (C) 2009-2017 AJA Video Systems, Inc.  All rights reserved.
	@brief		Declares system-dependent import/export macros and libraries.
**/

#ifndef AJA_EXPORT_H
#define AJA_EXPORT_H

#if defined(AJA_WINDOWS)
	#if defined(AJA_WINDLL)
		#pragma warning (disable : 4251)
		#if defined(AJA_DLL_BUILD)
			#define AJA_EXPORT __declspec(dllexport)
			#pragma comment (lib, "comctl32.lib")
			#pragma comment (lib, "wsock32.lib")
			#pragma comment (lib, "winmm")
			#pragma comment (lib, "setupapi.lib")
			#pragma comment (lib, "odbc32.lib")
			#pragma comment (lib, "shlwapi.lib")
		#else
			#define AJA_EXPORT __declspec(dllimport)
            #if !defined(AJA_NO_AUTOIMPORT)
                #if defined(AJA_DEBUG)
                    #if defined(_WIN64)
                        #pragma comment (lib, "libajabasedlld")
                    #else
                        #pragma comment (lib, "libajabasedll_32d")
                    #endif
                #else
                    #if defined(_WIN64)
                        #pragma comment (lib, "libajabasedll")
                    #else
                        #pragma comment (lib, "libajabasedll_32")
                    #endif
                #endif
            #endif
		#endif
	#else
		#define AJA_EXPORT
#if !defined(AJA_NO_AUTOIMPORT)
	#if !defined AJA_STUFF_OBJ
			#if defined(AJA_DEBUG)
				#if defined(_WIN64)
                    #pragma comment (lib, "libajabased")
				#else
                    #pragma comment (lib, "libajabase_32d")
				#endif
			#else
				#if defined(_WIN64)
                    #pragma comment (lib, "libajabase")
				#else
                    #pragma comment (lib, "libajabase_32")
				#endif
			#endif
			#pragma comment (lib, "comctl32.lib")
			#pragma comment (lib, "wsock32.lib")
			#pragma comment (lib, "winmm")
			#pragma comment (lib, "setupapi.lib")
			#pragma comment (lib, "odbc32.lib")
		#endif
#endif
	#endif
#else
	#define AJA_EXPORT
#endif

#endif	//	AJA_EXPORT_H
