/* SPDX-License-Identifier: MIT */
/**
	@file		ajatypes.h
	@brief		Declares the most fundamental data types used by NTV2. Since Windows NT was the first principal
				development platform, many typedefs are Windows-centric.
	@copyright	(C) 2004-2022 AJA Video Systems, Inc.  
**/
#ifndef AJATYPES_H
#define AJATYPES_H

#if defined (AJAMac)
	#define NTV2_USE_STDINT
#endif	//	if not MSWindows

/*/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
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
////////				It's best to activate/include symbols/APIs contiguously from the latest SDK
////////				(starting at the bottom), and continue activating/including to the SDK at which
////////				symbols/APIs should start to be deactivated/excluded.
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////*/

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


/*/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////	COMPILE-TIME FEATURES
////////	These macros control important aspects of SDK behavior.
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////*/
/**************************************************************************************************************
	NTV2_NULL_DEVICE			Controls whether or not the SDK will be able to connect to the NTV2 kernel
								driver via the normal connection method provided by the host operating system.
								Introduced in SDK 12.4.

	Undefined:	(Default) Locally-connected/installed NTV2 devices will be enumerable via
				CNTV2DeviceScanner and accessible via CNTV2Card::Open.

	Defined:	"OpenLocalPhysical" calls will fail, and CNTV2DeviceScanner won't find any locally
				connected/installed devices.
**************************************************************************************************************/
//#define NTV2_NULL_DEVICE	

/**************************************************************************************************************
	NTV2_NUB_CLIENT_SUPPORT		Controls whether or not the SDK will use plugin DLLs/dylibs/so's to connect
								to remote or software NTV2 devices.
								Introduced in SDK 12.4.

	Undefined:	SDK clients cannot access or connect to remote or software devices. This may be useful for
				SDK clients that require greater security by preventing the loading of plugins.

	Defined:	(Default) SDK clients will be able to access and connect to remote or software devices (via
				dynamically-loaded plugins).
**************************************************************************************************************/
#define NTV2_NUB_CLIENT_SUPPORT	

/**************************************************************************************************************
	NTV2_WRITEREG_PROFILING		Controls profiling of WriteRegister calls.
								Introduced in SDK 15.1.

	Undefined:	WriteRegister calls cannot be profiled, and the *RecordRegisterWrites API functions are
				unavailable.

	Defined:	(Default) WriteRegister calls can be profiled, and the API that controls profiling and
				retrieve results is available (e.g. the *RecordRegisterWrites API functions).
**************************************************************************************************************/
#define NTV2_WRITEREG_PROFILING		//	If defined, enables register write profiling


/**************************************************************************************************************
	NTV2_USE_CPLUSPLUS11		Controls use of C++11 language features.
								Introduced in SDK 16.0.

	Undefined:	The 'libajantv2' portion of the SDK will not use C++11 features.

	Defined:	(Default) The 'libajantv2' portion of the SDK will use C++11 features that require
				a C++11 compiler.

	See also:	AJA_USE_CPLUSPLUS11 in 'libajabase/include/types.h'
**************************************************************************************************************/
#if !defined(NTV2_USE_CPLUSPLUS11)
	#define NTV2_USE_CPLUSPLUS11 	
#endif	//	!defined(NTV2_USE_CPLUSPLUS11)


/**************************************************************************************************************
	NTV2_INCLUDE_DEVICE_CAPABILITIES_API	Controls the availability of the new DeviceCapabilities class/API.
											Introduced in SDK 17.0.

	Undefined:	No DeviceCapabilities class/API is declared. SDK clients will have to use the lower-level
				CNTV2DriverInterface::IsSupported and CNTV2DriverInterface::GetNumSupported member functions.

	Defined:	(Default) The DeviceCapabilities class/API is defined. SDK clients will be able to access
				and use this API via the CNTV2Card::features() accessor function.
**************************************************************************************************************/
#define	NTV2_INCLUDE_DEVICE_CAPABILITIES_API


/**************************************************************************************************************
	NTV2_ALLOW_OPEN_UNSUPPORTED				Controls whether unsupported devices can be opened.
											Introduced in SDK 17.0.

	Undefined:	(Default) Unsupported devices attached to the host cannot be opened.

	Defined:	Unsupported devices attached to the host can be opened.
**************************************************************************************************************/
//#define	NTV2_ALLOW_OPEN_UNSUPPORTED



/*/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////	HELPER MACROS
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////*/
#define AJA_VIRTUAL		virtual		//	Force use of virtual functions in CNTV2Card, etc.
#define AJA_STATIC		static		//	Do not change this.
#define NTV2_UNUSED(__p__)			(void)__p__

#if defined(__CPLUSPLUS__) || defined(__cplusplus)
	#if defined(AJAMac)
		#if defined(__clang__)
			#ifndef __has_feature
				#define __has_feature(__x__)	0
			#endif
			#if __has_feature(cxx_nullptr)
				#define AJA_CXX11_NULLPTR_AVAILABLE
			#endif
		#endif
	#elif defined(AJALinux)
		#if defined(__clang__)
			#ifndef __has_feature
				#define __has_feature(__x__)	0
			#endif
			#if __has_feature(cxx_nullptr)
				#define AJA_CXX11_NULLPTR_AVAILABLE
			#endif
		#elif defined(__GNUC__)
			#if __cplusplus >= 201103L // C++11 or later
				#define AJA_CXX11_NULLPTR_AVAILABLE
			#endif
		#endif
	#elif defined(MSWindows)
		#if defined(_MSC_VER) && _MSC_VER >= 1700		//	VS2012 or later:
			#define AJA_CXX11_NULLPTR_AVAILABLE
		#endif
	#endif
#endif

#if defined(AJA_CXX11_NULLPTR_AVAILABLE)
	#define AJA_NULL	nullptr
#else
	#define AJA_NULL	NULL
#endif

#if defined(__clang__)
	#ifndef __has_cpp_attribute
		#define __has_cpp_attribute(__x__)	0
	#endif

	#if __has_cpp_attribute(clang::fallthrough)
		#define AJA_FALL_THRU	[[clang::fallthrough]]
	#else
		#define AJA_FALL_THRU
	#endif
#elif defined(__GNUC__)
	#if __GNUC__ >= 5
		#define AJA_FALL_THRU	[[gnu::fallthrough]]
	#else
		#define AJA_FALL_THRU
	#endif
#else
	#define AJA_FALL_THRU
#endif


#if defined (NTV2_USE_STDINT)
	#if defined (MSWindows)
		#define _WINSOCK_DEPRECATED_NO_WARNINGS		1
		#if (_MSC_VER < 1300)
			typedef signed char				int8_t;
			typedef signed short			int16_t;
			typedef signed int				int32_t;
			typedef unsigned char			uint8_t;
			typedef unsigned short			uint16_t;
			typedef unsigned int			uint32_t;
		#else
			#if defined (NTV2_BUILDING_DRIVER)
				typedef signed __int8		int8_t;
				typedef signed __int16		int16_t;
				typedef signed __int32		int32_t;
				typedef unsigned __int8		uint8_t;
				typedef unsigned __int16	uint16_t;
				typedef unsigned __int32	uint32_t;
			#else
				#include <stdint.h>
			#endif
		#endif
		typedef signed __int64		int64_t;
		typedef unsigned __int64	uint64_t;
	#else
		#include <stdint.h>
	#endif
	typedef uint8_t					UByte;
	typedef int8_t					SByte;
	typedef int16_t					Word;
	typedef uint16_t				UWord;
	typedef int32_t					LWord;
	typedef uint32_t				ULWord;
	typedef uint32_t *				PULWord;
	typedef int64_t					LWord64;
	typedef uint64_t				ULWord64;
	typedef uint64_t				Pointer64;
#else
	typedef int						LWord;
	typedef unsigned int			ULWord;
	typedef unsigned int *			PULWord;
	typedef short					Word;
	typedef unsigned short			UWord;
	typedef unsigned char			UByte;
	typedef char					SByte;
#endif

// Platform dependent
									//////////////////////////////////////////////////////////////////
#if defined (MSWindows)				////////////////////////	WINDOWS //////////////////////////////
									//////////////////////////////////////////////////////////////////
	#define _WINSOCK_DEPRECATED_NO_WARNINGS		1

	#if !defined (NTV2_BUILDING_DRIVER)
		#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
		#endif

		#include <Windows.h>
	#endif
	#include <Basetsd.h>
	typedef unsigned char Boolean;
	typedef __int64 LWord64;
	typedef unsigned __int64 ULWord64;
	typedef unsigned __int64 Pointer64;
	typedef LWord				Fixed_;
	typedef bool				BOOL_;
	typedef UWord				UWord_;

	typedef signed __int8	 int8_t;
	typedef signed __int16	 int16_t;
	typedef signed __int32	 int32_t;
	typedef signed __int64	 int64_t;
	typedef unsigned __int8	 uint8_t;
	typedef unsigned __int16 uint16_t;
	typedef unsigned __int32 uint32_t;
	typedef unsigned __int64 uint64_t;

	typedef UINT_PTR	AJASocket;

	#define AJATargetBigEndian	0
	#define AJAFUNC		__FUNCTION__
	#define NTV2_CPP_MIN(__x__,__y__)		min((__x__),(__y__))
	#define NTV2_CPP_MAX(__x__,__y__)		max((__x__),(__y__))
	#pragma warning(disable:4996)   //  Sadly MSVC bitches about DECLARING a deprecated function but not about USING one.

									//////////////////////////////////////////////////////////////////
#elif defined (AJAMac)				////////////////////////	MAC		//////////////////////////////
									//////////////////////////////////////////////////////////////////
	#include <stdint.h>
	typedef short					HANDLE;
	typedef void*					PVOID;
	typedef unsigned int			BOOL_;
	typedef ULWord					UWord_;
	typedef int						Fixed_;
	typedef int						AJASocket;

	#define AJATargetBigEndian	0
	#define AJAFUNC		__func__
	#define NTV2_CPP_MIN(__x__,__y__)		std::min((__x__),(__y__))
	#define NTV2_CPP_MAX(__x__,__y__)		std::max((__x__),(__y__))

	#define MAX_PATH 4096

	#define INVALID_HANDLE_VALUE (0)

	#if !defined (NTV2_DEPRECATE)
		typedef struct {
		  int cx;
		  int cy;
		} SIZE;		///< @deprecated	Use NTV2FrameDimensions instead.
	#endif	//	!defined (NTV2_DEPRECATE)

	#define POINTER_32

										//////////////////////////////////////////////////////////////////
#elif defined (AJALinux)				////////////////////////	LINUX	//////////////////////////////
										//////////////////////////////////////////////////////////////////
	/* As of kernel 2.6.19, the C type _Bool is typedefed to bool to allow
	 * generic booleans in the kernel.	Unfortunately, we #define bool
	 * here and true and false there, so this fixes it ... until next time
	 * -JAC 3/6/2007 */
	#ifdef __KERNEL__
		#include "linux/version.h"
		#include "linux/kernel.h"
		#if defined (RHEL5) || (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,19))
			#include "linux/types.h"
		#else/* LINUX_VERSION_CODE */
			typedef unsigned char		bool;
		#endif /* LINUX_VERSION_CODE */
	#endif /* __KERNEL__ */

	#if defined (MODULE)
		#define NTV2_BUILDING_DRIVER
		#undef NTV2_USE_CPLUSPLUS11
	#endif

	#if !defined (NTV2_BUILDING_DRIVER)
		#include <stdint.h>
		typedef int64_t				HANDLE;
		typedef uint64_t			ULWord64;
		typedef uint64_t			Pointer64;
		typedef int64_t				LWord64;
		typedef void *				PVOID;
		typedef void *				LPVOID;
		typedef int32_t				Fixed_;
		typedef bool				BOOL_;
		typedef bool				BOOL;
		typedef UWord				UWord_;
		typedef uint32_t			DWORD; /* 32 bits on 32 or 64 bit CPUS */

		typedef int32_t				AJASocket;
		#define NTV2_CPP_MIN(__x__,__y__)		std::min((__x__),(__y__))
		#define NTV2_CPP_MAX(__x__,__y__)		std::max((__x__),(__y__))
	#else
		#if defined (AJAVirtual)
			#include <stdbool.h>
			#include <stdint.h>
		#endif
		typedef long				HANDLE;
		// this is what is is in Windows:
		// typedef void *				HANDLE;
		typedef unsigned long long	ULWord64;
		typedef unsigned long long	Pointer64;
		typedef signed long long	LWord64;
		typedef void *				PVOID;
		typedef void *				LPVOID;
		typedef LWord				Fixed_;
		typedef bool				BOOL_;
		typedef bool				BOOL;
		typedef UWord				UWord_;
		typedef unsigned int		DWORD; /* 32 bits on 32 or 64 bit CPUS */

		typedef int					AJASocket;
	#endif

	#define AJATargetBigEndian	0
	#define AJAFUNC		__func__

	#if !defined (NTV2_BUILDING_DRIVER)
		#if defined (NTV2_DEPRECATE)
			//	The gcc compiler used for Linux NTV2 builds doesn't like __declspec(deprecated)
			#define NTV2_DEPRECATED		//	Disable deprecate warnings (for now)
		#else
			#define NTV2_DEPRECATED
		#endif

		#if defined (NTV2_DEPRECATE_12_5)
			//	The gcc compiler used for Linux NTV2 builds doesn't like __declspec(deprecated)
			#define NTV2_DEPRECATED_12_5		//	Disable deprecate warnings (for now)
		#else
			#define NTV2_DEPRECATED_12_5
		#endif

		#if defined (NTV2_DEPRECATE_12_6)
			//	The gcc compiler used for Linux NTV2 builds doesn't like __declspec(deprecated)
			#define NTV2_DEPRECATED_12_6		//	Disable deprecate warnings (for now)
		#else
			#define NTV2_DEPRECATED_12_6
		#endif

		#if defined (NTV2_DEPRECATE_12_7)
			//	The gcc compiler used for Linux NTV2 builds doesn't like __declspec(deprecated)
			#define NTV2_DEPRECATED_12_7		//	Disable deprecate warnings (for now)
		#else
			#define NTV2_DEPRECATED_12_7
		#endif

		#if defined (NTV2_DEPRECATE_13_0)
			//	The gcc compiler used for Linux NTV2 builds doesn't like __declspec(deprecated)
			#define NTV2_DEPRECATED_13_0		//	Disable deprecate warnings (for now)
		#else
			#define NTV2_DEPRECATED_13_0
		#endif

		#if defined (NTV2_DEPRECATE_13_1)
			//	The gcc compiler used for Linux NTV2 builds doesn't like __declspec(deprecated)
			#define NTV2_DEPRECATED_13_1		//	Disable deprecate warnings (for now)
		#else
			#define NTV2_DEPRECATED_13_1
		#endif

		#if defined (NTV2_DEPRECATE_14_0)
			//	The gcc compiler used for Linux NTV2 builds doesn't like __declspec(deprecated)
			#define NTV2_DEPRECATED_14_0		//	Disable deprecate warnings (for now)
		#else
			#define NTV2_DEPRECATED_14_0
		#endif

		#if defined (NTV2_DEPRECATE_14_1)
			//	The gcc compiler used for Linux NTV2 builds doesn't like __declspec(deprecated)
			#define NTV2_DEPRECATED_14_1		//	Disable deprecate warnings (for now)
		#else
			#define NTV2_DEPRECATED_14_1
		#endif
	#endif

	#if !defined (NTV2_DEPRECATE)
		typedef struct {
		  int cx;
		  int cy;
		} SIZE; ///< @deprecated	Use NTV2FrameDimensions instead.
	#endif	//	!defined (NTV2_DEPRECATE)

	typedef struct {
	  int left;
	  int right;
	  int top;
	  int bottom;
	} RECT;

	#ifndef INVALID_HANDLE_VALUE
		#define INVALID_HANDLE_VALUE (-1L)
	#endif
	#define WINAPI
	#define POINTER_32
	#define MAX_PATH	4096
										//////////////////////////////////////////////////////////////////
#elif defined (AJA_BAREMETAL)				////////////////////////	Bare Metal		//////////////////////////////
									//////////////////////////////////////////////////////////////////
	#include <stdint.h>
	typedef short					HANDLE;
	typedef void*					PVOID;
	typedef unsigned int			BOOL_;
	typedef ULWord					UWord_;
	typedef int						Fixed_;
	typedef int						AJASocket;

	#define AJATargetBigEndian	0
	#define AJAFUNC		__func__
	#define NTV2_CPP_MIN(__x__,__y__)		std::min((__x__),(__y__))
	#define NTV2_CPP_MAX(__x__,__y__)		std::max((__x__),(__y__))

	#define MAX_PATH 4096

  #undef NTV2_WRITEREG_PROFILING		//	disable register write profiling

	#define INVALID_HANDLE_VALUE (0)

	#if !defined (NTV2_DEPRECATE)
		typedef struct {
		  int cx;
		  int cy;
		} SIZE;		///< @deprecated	Use NTV2FrameDimensions instead.
	#endif	//	!defined (NTV2_DEPRECATE)

	#define POINTER_32

#else									////////////////////////	(OTHER)		//////////////////////////
										//////////////////////////////////////////////////////////////////
	#error "IMPLEMENT OTHER PLATFORM"

#endif	//	end OTHER PLATFORM

#if defined (NTV2_BUILDING_DRIVER)
	//	The AJA NTV2 driver is always built without any deprecated types or functions:
	#if !defined(NTV2_DEPRECATE)
		#define NTV2_DEPRECATE
	#endif
	#if !defined(NTV2_DEPRECATE_12_5)
		#define NTV2_DEPRECATE_12_5
	#endif
	#if !defined(NTV2_DEPRECATE_12_6)
		#define NTV2_DEPRECATE_12_6
	#endif
	#if !defined(NTV2_DEPRECATE_12_7)
		#define NTV2_DEPRECATE_12_7
	#endif
	#if !defined(NTV2_DEPRECATE_13_0)
		#define NTV2_DEPRECATE_13_0
	#endif
	#if !defined(NTV2_DEPRECATE_13_1)
		#define NTV2_DEPRECATE_13_1
	#endif
	#if !defined(NTV2_DEPRECATE_14_0)
		#define NTV2_DEPRECATE_14_0
	#endif
	#if !defined(NTV2_DEPRECATE_14_1)
		#define NTV2_DEPRECATE_14_1		//	(never released)
	#endif
	#if !defined(NTV2_DEPRECATE_14_2)
		#define NTV2_DEPRECATE_14_2
	#endif
	#if !defined(NTV2_DEPRECATE_14_3)
		#define NTV2_DEPRECATE_14_3
	#endif
	#if !defined(NTV2_DEPRECATE_15_0)
		#define NTV2_DEPRECATE_15_0
	#endif
	#if !defined(NTV2_DEPRECATE_15_1)
		#define NTV2_DEPRECATE_15_1
	#endif
	#if !defined(NTV2_DEPRECATE_15_2)
		#define NTV2_DEPRECATE_15_2
	#endif
	#if !defined(NTV2_DEPRECATE_15_3)
		#define NTV2_DEPRECATE_15_3
	#endif
	#if !defined(NTV2_DEPRECATE_15_5)
		#define NTV2_DEPRECATE_15_5
	#endif
	#if !defined(NTV2_DEPRECATE_15_6)
		#define NTV2_DEPRECATE_15_6
	#endif
	#if !defined(NTV2_DEPRECATE_16_0)
		#define NTV2_DEPRECATE_16_0
	#endif
	#if !defined(NTV2_DEPRECATE_16_1)
		#define NTV2_DEPRECATE_16_1
	#endif
	#if !defined(NTV2_DEPRECATE_16_2)
		#define NTV2_DEPRECATE_16_2
	#endif
	#if !defined(NTV2_DEPRECATE_16_3)
		#define NTV2_DEPRECATE_16_3
	#endif
#endif


//////////////////////////////////////////////////////////////////////
////////////////////////	NTV2_ASSERT		//////////////////////////
//////////////////////////////////////////////////////////////////////
#if !defined (NTV2_ASSERT)
	#if defined (NTV2_BUILDING_DRIVER)
		//	Kernel space NTV2_ASSERTs
		#if defined (AJA_DEBUG) || defined (_DEBUG)
			#if defined (MSWindows)
				#define NTV2_ASSERT(_expr_)		ASSERT (#_expr_)
			#elif defined (AJAMac)
				#define NTV2_ASSERT(_expr_)		assert (_expr_)
			#elif defined (AJALinux)
				#define NTV2_ASSERT(_expr_)		do {if (#_expr_) break;														\
													printk (KERN_EMERG "### NTV2_ASSERT '%s': %s: line %d: %s\n", \
															__FILE__, __func__, __LINE__, #_expr_); dump_stack(); \
												} while (0)
			#else
				#define NTV2_ASSERT(_expr_)
			#endif
		#else
			#define NTV2_ASSERT(_expr_)
		#endif
	#else
		//	User space NTV2_ASSERTs
		#if defined (AJA_DEBUG) || defined (_DEBUG)
			#include <assert.h>
			#define NTV2_ASSERT(_expr_)		assert (_expr_)
		#else
			#define NTV2_ASSERT(_expr_)		(void) (_expr_)
		#endif
	#endif	//	else !defined (NTV2_BUILDING_DRIVER)
#endif	//	if NTV2_ASSERT undefined


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


#if !defined(BIT)
	#if !defined(AJALinux)
		#define BIT(_x_)	(1u << (_x_))
	#else	//	Linux:
		//	As of kernel 2.6.24, BIT is defined in the kernel source (linux/bitops.h).
		//	By making that definition match the one in the kernel source *exactly*
		//	we supress compiler warnings (thanks Shaun)
		#define BIT(nr) (1UL << (nr))
	#endif	// AJALinux
#endif
#if 1
	#define BIT_0 (1u<<0)
	#define BIT_1 (1u<<1)
	#define BIT_2 (1u<<2)
	#define BIT_3 (1u<<3)
	#define BIT_4 (1u<<4)
	#define BIT_5 (1u<<5)
	#define BIT_6 (1u<<6)
	#define BIT_7 (1u<<7)
	#define BIT_8 (1u<<8)
	#define BIT_9 (1u<<9)
	#define BIT_10 (1u<<10)
	#define BIT_11 (1u<<11)
	#define BIT_12 (1u<<12)
	#define BIT_13 (1u<<13)
	#define BIT_14 (1u<<14)
	#define BIT_15 (1u<<15)
	#define BIT_16 (1u<<16)
	#define BIT_17 (1u<<17)
	#define BIT_18 (1u<<18)
	#define BIT_19 (1u<<19)
	#define BIT_20 (1u<<20)
	#define BIT_21 (1u<<21)
	#define BIT_22 (1u<<22)
	#define BIT_23 (1u<<23)
	#define BIT_24 (1u<<24)
	#define BIT_25 (1u<<25)
	#define BIT_26 (1u<<26)
	#define BIT_27 (1u<<27)
	#define BIT_28 (1u<<28)
	#define BIT_29 (1u<<29)
	#define BIT_30 (1u<<30)
	#define BIT_31 (1u<<31)
#endif	//	1

#if 0
// Check at compile time if all the defined types are the correct size
// must support C++11 for this to work
static_assert(sizeof(bool) == 1,	  "bool: size is not correct");
static_assert(sizeof(int8_t) == 1,	  "int8_t: size is not correct");
static_assert(sizeof(int16_t) == 2,	  "int16_t: size is not correct");
static_assert(sizeof(int32_t) == 4,	  "int32_t: size is not correct");
static_assert(sizeof(int64_t) == 8,	  "int64_t: size is not correct");
static_assert(sizeof(uint8_t) == 1,	  "uint8_t: size is not correct");
static_assert(sizeof(uint16_t) == 2,  "uint16_t: size is not correct");
static_assert(sizeof(uint32_t) == 4,  "uint32_t: size is not correct");
static_assert(sizeof(uint64_t) == 8,  "uint64_t: size is not correct");

static_assert(sizeof(LWord) == 4,	  "LWord: size is not correct");
static_assert(sizeof(ULWord) == 4,	  "ULWord: size is not correct");
static_assert(sizeof(PULWord) == 8,	  "PULWord: size is not correct");
static_assert(sizeof(Word) == 2,	  "Word: size is not correct");
static_assert(sizeof(UWord) == 2,	  "UWord: size is not correct");
static_assert(sizeof(UByte) == 1,	  "UByte: size is not correct");
static_assert(sizeof(SByte) == 1,	  "SByte: size is not correct");

static_assert(sizeof(ULWord64) == 8,  "ULWord64: size is not correct");
static_assert(sizeof(Pointer64) == 8, "Pointer64: size is not correct");
static_assert(sizeof(LWord64) == 8,	  "LWord64: size is not correct");
static_assert(sizeof(PVOID) == 8,	  "PVOID: size is not correct");
static_assert(sizeof(Fixed_) == 4,	  "Fixed_: size is not correct");

// ideally these whould be the same across the platforms but historically they have not been
#if defined(MSWindows)
static_assert(sizeof(HANDLE) == 8,	  "HANDLE: size is not correct");
static_assert(sizeof(BOOL) == 4,	  "BOOL: size is not correct");
static_assert(sizeof(BOOL_) == 1,	  "BOOL_: size is not correct");
static_assert(sizeof(AJASocket) == 8, "AJASocket: size is not correct");
static_assert(sizeof(UWord_) == 2,	  "UWord_: size is not correct");
static_assert(sizeof(LPVOID) == 8,	  "LPVOID: size is not correct");
static_assert(sizeof(DWORD) == 4,	  "DWORD: size is not correct");
#elif defined(AJAMac)
static_assert(sizeof(HANDLE) == 2,	  "HANDLE: size is not correct");
//static_assert(sizeof(BOOL) == 1,		"BOOL: size is not correct");
static_assert(sizeof(BOOL_) == 4,	  "BOOL_: size is not correct");
static_assert(sizeof(AJASocket) == 4, "AJASocket: size is not correct");
static_assert(sizeof(UWord_) == 4,	  "UWord_: size is not correct");
//static_assert(sizeof(LPVOID) == 8,	"LPVOID: size is not correct");
//static_assert(sizeof(DWORD) == 4,		"DWORD: size is not correct");
#elif defined(AJALinux)
static_assert(sizeof(HANDLE) == 8,	  "HANDLE: size is not correct");
static_assert(sizeof(BOOL) == 1,	  "BOOL: size is not correct");
static_assert(sizeof(BOOL_) == 1,	  "BOOL_: size is not correct");
static_assert(sizeof(AJASocket) == 4, "AJASocket: size is not correct");
static_assert(sizeof(UWord_) == 2,	  "UWord_: size is not correct");
static_assert(sizeof(LPVOID) == 8,	  "LPVOID: size is not correct");
static_assert(sizeof(DWORD) == 4,	  "DWORD: size is not correct");
#endif

#endif

#endif	//	AJATYPES_H
