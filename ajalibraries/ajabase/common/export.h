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
			#if defined(AJA_DEBUG)
				#if defined(_WIN64)
                    #pragma comment (lib, "ajabasedll_64d")
				#else
                    #pragma comment (lib, "ajabasedlld")
				#endif
			#else
				#if defined(_WIN64)
                    #pragma comment (lib, "ajabasedll_64")
				#else
                    #pragma comment (lib, "ajabasedll")
				#endif
			#endif
		#endif
	#else
		#define AJA_EXPORT
#if !defined(AJA_NO_AUTOIMPORT)
	#if !defined AJA_STUFF_OBJ
			#if defined(AJA_DEBUG)
				#if defined(_WIN64)
                    #pragma comment (lib, "ajabase_64d")
				#else
                    #pragma comment (lib, "ajabased")
				#endif
			#else
				#if defined(_WIN64)
                    #pragma comment (lib, "ajabase_64")
				#else
                    #pragma comment (lib, "ajabase")
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
