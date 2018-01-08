/**
	@file		ajatypes.h
	@copyright	Copyright (C) 2004-2017 AJA Video Systems, Inc.  Proprietary and Confidential information.
	@brief		Declares the most fundamental data types used by NTV2. Since Windows NT was the first principal
				development platform, many typedefs are Windows-centric.
**/
#ifndef AJATYPES_H
#define AJATYPES_H

#if defined (AJAMac)
	#define	NTV2_USE_STDINT
#endif	//	if not MSWindows
#define NTV2_DEPRECATE				//	If defined, excludes all symbols/APIs first deprecated in SDK 12.4 or earlier
#define NTV2_DEPRECATE_12_5			//	If defined, excludes all symbols/APIs first deprecated in SDK 12.5
#define NTV2_DEPRECATE_12_6			//	If defined, excludes all symbols/APIs first deprecated in SDK 12.6
#define NTV2_DEPRECATE_12_7			//	If defined, excludes all symbols/APIs first deprecated in SDK 12.7
//#define NTV2_DEPRECATE_13_0			//	First deprecated in SDK 13.0
//#define NTV2_DEPRECATE_13_1			//	First deprecated in SDK 13.1
//#define NTV2_DEPRECATE_14_0			//	First deprecated in SDK 14.0
#define NTV2_NUB_CLIENT_SUPPORT		//	If defined, includes nub client support;  otherwise, excludes it

#define	AJA_VIRTUAL		virtual		//	Force use of virtual functions in CNTV2Card, etc.


#if defined (NTV2_USE_STDINT)
	#if defined (MSWindows)
		#define	_WINSOCK_DEPRECATED_NO_WARNINGS		1
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

#if defined (MSWindows)				/////////////// WINDOWS/////////////////////////

    #define	_WINSOCK_DEPRECATED_NO_WARNINGS		1

	#if !defined (NTV2_BUILDING_DRIVER)
		#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
		#endif

		#include <windows.h>
	#endif
	#include <Basetsd.h>
	typedef unsigned char Boolean;
	typedef __int64 LWord64;
	typedef unsigned __int64 ULWord64;
	typedef unsigned __int64 Pointer64;
	typedef LWord 				Fixed_;
	typedef bool				BOOL_;
	typedef UWord				UWord_;

	typedef signed __int8    int8_t;
	typedef signed __int16   int16_t;
	typedef signed __int32   int32_t;
	typedef signed __int64   int64_t;
	typedef unsigned __int8  uint8_t;
	typedef unsigned __int16 uint16_t;
	typedef unsigned __int32 uint32_t;
	typedef unsigned __int64 uint64_t;

	typedef UINT_PTR	AJASocket;

	#define AJATargetBigEndian  0
	#if defined (NTV2_BUILDING_DRIVER)
		#define	NTV2_DEPRECATED
		#define	NTV2_DEPRECATED_12_5
		#define	NTV2_DEPRECATED_12_6
		#define	NTV2_DEPRECATED_13_0
	#else
		#if defined (NTV2_DEPRECATE)
			#define	NTV2_DEPRECATED
		#else
			#define	NTV2_DEPRECATED			__declspec(deprecated)
		#endif

		#if defined (NTV2_DEPRECATE_12_5)
			#define	NTV2_DEPRECATED_12_5
		#else
			#define	NTV2_DEPRECATED_12_5	__declspec(deprecated)
		#endif

		#if defined (NTV2_DEPRECATE_12_6)
			#define	NTV2_DEPRECATED_12_6
		#else
			#define	NTV2_DEPRECATED_12_6	__declspec(deprecated)
		#endif

		#if defined (NTV2_DEPRECATE_12_7)
			#define	NTV2_DEPRECATED_12_7
		#else
			#define	NTV2_DEPRECATED_12_7	__declspec(deprecated)
		#endif

		#if defined (NTV2_DEPRECATE_13_0)
			#define	NTV2_DEPRECATED_13_0
		#else
			#define	NTV2_DEPRECATED_13_0	__declspec(deprecated)
		#endif
	#endif

#elif defined (AJAMac)				///////////////MAC OS X//////////////////////////

	#include <stdint.h>
	typedef short					HANDLE;
	typedef void*					PVOID;
	typedef unsigned int			BOOL_;
	typedef ULWord					UWord_;
	typedef int						Fixed_;
	typedef int						AJASocket;

	#define AJATargetBigEndian  TARGET_RT_BIG_ENDIAN

	#define MAX_PATH 4096

	#define INVALID_HANDLE_VALUE (0)

	//	MacDriverInterfaceVersion		Description of Changes
	//			1 .. 7					???
	//				8					Valid since 10.2. Added reference-counted Acquire/Release calls.
	//				9					Valid since NTV2 SDK 12.1
	//
	//	This important number is used in the driver's Info.plist, as well as the NTV2 Mac public interface.
	//	CNTV2Card::Open will fail if the client (SDK) driver interface version number does not match the
	//	driver version number that is reported by the driver.
	//
	#define	AJA_MAC_DRIVER_INTERFACE_VERSION	9

	#if !defined (NTV2_DEPRECATE)
		typedef struct {
		  int cx;
		  int cy;
		} SIZE;		///< @deprecated	Use NTV2FrameDimensions instead.
	#endif	//	!defined (NTV2_DEPRECATE)

	#define POINTER_32
	#if defined (NTV2_BUILDING_DRIVER)
		#define	NTV2_DEPRECATED
		#define	NTV2_DEPRECATED_12_5
		#define	NTV2_DEPRECATED_12_6
		#define	NTV2_DEPRECATED_13_0
	#else
		#if defined (NTV2_DEPRECATE)
			#define	NTV2_DEPRECATED
		#else
			#if !defined (__clang__)
				#define	NTV2_DEPRECATED		//	__declspec unavailable until Xcode 5
			#elif __clang_major__ == 7  &&  __clang_minor__ == 3
				#define	NTV2_DEPRECATED		//	__declspec broken in Xcode 7.3
			#elif __clang_major__ == 8
				#define	NTV2_DEPRECATED		//	__declspec broken in Xcode 8
			#else
				#define	NTV2_DEPRECATED		__declspec(deprecated)
			#endif
		#endif

		#if defined (NTV2_DEPRECATE_12_5)
			#define	NTV2_DEPRECATED_12_5
		#else
			#if !defined (__clang__)
				#define	NTV2_DEPRECATED_12_5	//	__declspec unavailable until Xcode 5
			#elif __clang_major__ == 7  &&  __clang_minor__ == 3
				#define	NTV2_DEPRECATED_12_5	//	__declspec broken in Xcode 7.3
			#elif __clang_major__ == 8
				#define	NTV2_DEPRECATED_12_5	//	__declspec broken in Xcode 8
			#else
				#define	NTV2_DEPRECATED_12_5	__declspec(deprecated)
			#endif
		#endif

		#if defined (NTV2_DEPRECATE_12_6)
			#define	NTV2_DEPRECATED_12_6
		#else
			#if !defined (__clang__)
				#define	NTV2_DEPRECATED_12_6	//	__declspec unavailable until Xcode 5
			#elif __clang_major__ == 7  &&  __clang_minor__ == 3
				#define	NTV2_DEPRECATED_12_6	//	__declspec broken in Xcode 7.3
			#elif __clang_major__ == 8
				#define	NTV2_DEPRECATED_12_6	//	__declspec broken in Xcode 8
			#else
				#define	NTV2_DEPRECATED_12_6	__declspec(deprecated)
			#endif
		#endif

		#if defined (NTV2_DEPRECATE_12_7)
			#define	NTV2_DEPRECATED_12_7
		#else
			#if !defined (__clang__)
				#define	NTV2_DEPRECATED_12_7	//	__declspec unavailable until Xcode 5
			#elif __clang_major__ == 7  &&  __clang_minor__ == 3
				#define	NTV2_DEPRECATED_12_7	//	__declspec broken in Xcode 7.3
			#elif __clang_major__ == 8
				#define	NTV2_DEPRECATED_12_7	//	__declspec broken in Xcode 8
			#else
				#define	NTV2_DEPRECATED_12_7	__declspec(deprecated)
			#endif
		#endif

		#if defined (NTV2_DEPRECATE_13_0)
			#define	NTV2_DEPRECATED_13_0
		#else
			#if !defined (__clang__)
				#define	NTV2_DEPRECATED_13_0	//	__declspec unavailable until Xcode 5
			#elif __clang_major__ == 7  &&  __clang_minor__ == 3
				#define	NTV2_DEPRECATED_13_0	//	__declspec broken in Xcode 7.3
			#elif __clang_major__ == 8
				#define	NTV2_DEPRECATED_13_0	//	__declspec broken in Xcode 8
			#else
				#define	NTV2_DEPRECATED_13_0	__declspec(deprecated)
			#endif
		#endif
	#endif

#elif defined (AJALinux)				///////////////LINUX//////////////////////////////

	/* As of kernel 2.6.19, the C type _Bool is typedefed to bool to allow
	 * generic booleans in the kernel.  Unfortunately, we #define bool
	 * here and true and false there, so this fixes it ... until next time
	 * -JAC 3/6/2007 */
	#ifdef __KERNEL__
		#include "linux/version.h"
		#if defined (RHEL5) || (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,19))
			#include "linux/types.h"
		#else/* LINUX_VERSION_CODE */
			typedef unsigned char       bool;
		#endif /* LINUX_VERSION_CODE */
	#endif /* __KERNEL__ */

	typedef long				HANDLE;
	// this is what is is in Windows:
	// typedef void *				HANDLE;
	typedef unsigned long long	ULWord64;
	typedef unsigned long long	Pointer64;
	typedef unsigned long long	__int64;
	typedef signed long long	LWord64;
	typedef void * 				PVOID;
	typedef void * 				LPVOID;
	typedef LWord				Fixed_;
	typedef bool				BOOL_;
	#ifndef FS1
		typedef bool			BOOL;
	#endif
	typedef UWord				UWord_;
	typedef unsigned int        DWORD; /* 32 bits on 32 or 64 bit CPUS */

	typedef int					AJASocket;

	#define AJATargetBigEndian  0
	#if defined (MODULE)
		#define NTV2_BUILDING_DRIVER
		#define	NTV2_DEPRECATED
		#define	NTV2_DEPRECATED_12_5
	#else
		#if defined (NTV2_DEPRECATE)
			//	The gcc compiler used for Linux NTV2 builds doesn't like __declspec(deprecated)
			#define	NTV2_DEPRECATED		//	Disable deprecate warnings (for now)
		#else
			#define	NTV2_DEPRECATED
		#endif

		#if defined (NTV2_DEPRECATE_12_5)
			//	The gcc compiler used for Linux NTV2 builds doesn't like __declspec(deprecated)
			#define	NTV2_DEPRECATED_12_5		//	Disable deprecate warnings (for now)
		#else
			#define	NTV2_DEPRECATED_12_5
		#endif

		#if defined (NTV2_DEPRECATE_12_6)
			//	The gcc compiler used for Linux NTV2 builds doesn't like __declspec(deprecated)
			#define	NTV2_DEPRECATED_12_6		//	Disable deprecate warnings (for now)
		#else
			#define	NTV2_DEPRECATED_12_6
		#endif

		#if defined (NTV2_DEPRECATE_12_7)
			//	The gcc compiler used for Linux NTV2 builds doesn't like __declspec(deprecated)
			#define	NTV2_DEPRECATED_12_7		//	Disable deprecate warnings (for now)
		#else
			#define	NTV2_DEPRECATED_12_7
		#endif

		#if defined (NTV2_DEPRECATE_13_0)
			//	The gcc compiler used for Linux NTV2 builds doesn't like __declspec(deprecated)
			#define	NTV2_DEPRECATED_13_0		//	Disable deprecate warnings (for now)
		#else
			#define	NTV2_DEPRECATED_13_0
		#endif
	#endif

	#if !defined (NTV2_BUILDING_DRIVER)
		#include <stdint.h>
	#endif

	#if !defined (NTV2_DEPRECATE)
		typedef struct {
		  int cx;
		  int cy;
		} SIZE;	///< @deprecated	Use NTV2FrameDimensions instead.
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

#else	//	end AJALinux

	#error "IMPLEMENT OTHER PLATFORM"

#endif	//	end OTHER PLATFORM

#if !defined (NTV2_ASSERT)
	#if defined (NTV2_BUILDING_DRIVER)
		//	Kernel space NTV2_ASSERTs
		#if defined (AJA_DEBUG) || defined (_DEBUG)
			#if defined (MSWindows)
				#define	NTV2_ASSERT(_expr_)		ASSERT (#_expr_)
			#elif defined (AJAMac)
				#define	NTV2_ASSERT(_expr_)		assert (_expr_)
			#elif defined (AJALinux)
				#define NTV2_ASSERT(_expr_)		do {if (#_expr_) break;														\
													printk (KERN_EMERG "### NTV2_ASSERT '%s': %s: line %d: %s\n",			\
															__FILE__, __func__, __LINE__, #_expr_); dump_stack(); BUG();	\
												} while (0)
			#else
				#define	NTV2_ASSERT(_expr_)
			#endif
		#else
			#define	NTV2_ASSERT(_expr_)
		#endif
	#else
		//	User space NTV2_ASSERTs
		#if defined (AJA_DEBUG) || defined (_DEBUG)
			#include <assert.h>
			#define	NTV2_ASSERT(_expr_)		assert (_expr_)
		#else
			#define	NTV2_ASSERT(_expr_)		(void) (_expr_)
		#endif
	#endif	//	else !defined (NTV2_BUILDING_DRIVER)
#endif	//	if NTV2_ASSERT undefined


/**
	@brief	Describes the horizontal and vertical size dimensions of a raster, bitmap, frame or image.
**/
typedef struct NTV2FrameDimensions
{
	#if !defined (NTV2_BUILDING_DRIVER)
		//	Member Functions

		/**
			@brief		My constructor.
			@param[in]	inWidth		Optionally specifies my initial width dimension, in pixels. Defaults to zero.
			@param[in]	inHeight	Optionally specifies my initial height dimension, in lines. Defaults to zero.
		**/
		inline NTV2FrameDimensions (const ULWord inWidth = 0, const ULWord inHeight = 0)	{Set (inWidth, inHeight);}
		inline ULWord					GetWidth (void) const		{return mWidth;}	///< @return	My width, in pixels.
		inline ULWord					GetHeight (void) const		{return mHeight;}	///< @return	My height, in lines/rows.
		inline ULWord					Width (void) const			{return mWidth;}	///< @return	My width, in pixels.
		inline ULWord					Height (void) const			{return mHeight;}	///< @return	My height, in lines/rows.
		inline bool						IsValid (void) const		{return Width() && Height();}	///< @return	True if both my width and height are non-zero.

		/**
			@brief		Sets my width dimension.
			@param[in]	inValue		Specifies the new width dimension, in pixels.
			@return		A non-constant reference to me.
		**/
		inline NTV2FrameDimensions &	SetWidth (const ULWord inValue)						{mWidth = inValue; return *this;}

		/**
			@brief		Sets my height dimension.
			@param[in]	inValue		Specifies the new height dimension, in lines.
			@return		A non-constant reference to me.
		**/
		inline NTV2FrameDimensions &	SetHeight (const ULWord inValue)					{mHeight = inValue; return *this;}

		/**
			@brief		Sets my dimension values.
			@param[in]	inWidth		Specifies the new width dimension, in pixels.
			@param[in]	inHeight	Specifies the new height dimension, in lines.
			@return		A non-constant reference to me.
		**/
		inline NTV2FrameDimensions &	Set (const ULWord inWidth, const ULWord inHeight)	{return SetWidth (inWidth).SetHeight (inHeight);}

		/**
			@brief		Sets both my width and height to zero (an invalid state).
			@return		A non-constant reference to me.
		**/
		inline NTV2FrameDimensions &	Reset (void)										{return Set (0, 0);}

		private:	//	Private member data only if not building driver
	#endif	//	!defined (NTV2_BUILDING_DRIVER)
	//	Member Variables
	ULWord	mWidth;		///< @brief	The horizontal dimension, in pixels.
	ULWord	mHeight;	///< @brief	The vertical dimension, in lines.
} NTV2FrameDimensions;


// As of kernel 2.6.24, BIT is defined in the kernel source
// (linux/bitops.h).  By making that definition match the one in the
// kernel source *exactly* we supress compiler warnings (thanks Shaun)

#ifndef BIT
	#ifndef AJALinux
		#define BIT(x)		(1u << (x))
	#else
		#define BIT(nr)		(1UL << (nr))
	#endif	// AJALinux
#endif
// or the stupid way.
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

#endif	//	AJATYPES_H
