/**
	@file		export.h
	@copyright	Copyright (C) 2009-2018 AJA Video Systems, Inc.  All rights reserved.
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
            #pragma comment (lib, "netapi32.lib")
		#else	//	else !defined(AJA_DLL_BUILD)
			#define AJA_EXPORT __declspec(dllimport)
            #if !defined(AJA_NO_AUTOIMPORT)
                #if defined(AJA_DEBUG)
                    #if defined(_WIN64)
                        #pragma comment (lib, "libajantv2dlld")
                    #else
                        #pragma comment (lib, "libajantv2dll_32d")
                    #endif
                #else
                    #if defined(_WIN64)
                        #pragma comment (lib, "libajantv2dll")
                    #else
                        #pragma comment (lib, "libajantv2dll_32")
                    #endif
                #endif
            #endif	//	!defined(AJA_NO_AUTOIMPORT)
		#endif	//	else !defined(AJA_DLL_BUILD)
	#else	//	!defined(AJA_WINDLL)
		#define AJA_EXPORT
		#if !defined(AJA_NO_AUTOIMPORT)
			#if !defined AJA_BASE_OBJ
				#if defined(AJA_DEBUG)
					#if defined(_WIN64)
						#pragma comment (lib, "libajantv2d")
					#else
						#pragma comment (lib, "libajantv2_32d")
					#endif
				#else	//	else !defined(AJA_DEBUG)
					#if defined(_WIN64)
						#pragma comment (lib, "libajantv2")
					#else
						#pragma comment (lib, "libajantv2_32")
					#endif
				#endif	//	else !defined(AJA_DEBUG)
				#pragma comment (lib, "comctl32.lib")
				#pragma comment (lib, "wsock32.lib")
				#pragma comment (lib, "winmm")
				#pragma comment (lib, "setupapi.lib")
				#pragma comment (lib, "odbc32.lib")
				#pragma comment (lib, "shlwapi.lib")
				#pragma comment (lib, "netapi32.lib")
			#endif	//	!defined AJA_BASE_OBJ
		#endif	//	!defined(AJA_NO_AUTOIMPORT)
	#endif	//	else !defined(AJA_WINDLL)
#else	//	else !defined(AJA_WINDOWS)
	#define AJA_EXPORT
#endif	//	else !defined(AJA_WINDOWS)

#endif	//	AJA_EXPORT_H
