/**
	@file		export.h
	@copyright	Copyright (C) 2009-2019 AJA Video Systems, Inc.  All rights reserved.
	@brief		Declares system-dependent import/export macros and libraries.
**/

#ifndef AJA_EXPORT_H
#define AJA_EXPORT_H

#ifdef MSWindows
	#ifndef AJASTATIC
		#if defined(AJADLL) || defined(AJA_WINDLL)
			#pragma warning (disable : 4251)
			#if defined(AJADLL_BUILD) || defined(AJA_DLL_BUILD)
				#define AJAExport	__declspec(dllexport)	//	ajantv2 way
				#define AJA_EXPORT	__declspec(dllexport)	//	ajabase way
			#else
				#define AJAExport	__declspec(dllimport)	//	ajantv2 way
				#define AJA_EXPORT	__declspec(dllimport)	//	ajabase way
			#endif
		#else
			#define AJAExport	//	ajantv2 way
			#define	AJA_EXPORT	//	ajabase way
			#ifndef AJA_NO_AUTOIMPORT
			#endif
		#endif
	#else
		#define AJAExport	//	ajantv2 way
		#define	AJA_EXPORT	//	ajabase way
	#endif
#else
	#define AJAExport	//	ajantv2 way
	#define	AJA_EXPORT	//	ajabase way
#endif

#endif	//	AJA_EXPORT_H
