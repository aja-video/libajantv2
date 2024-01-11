/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2023 AJA Video Systems, Inc.
 */
///////////////////////////////////////////////////////////////
//
// Defines based on build environment (distro, kernel, etc)
//
////////////////////////////////////////////////////////////
//
// Filename: buildenv.h
// Purpose:	 Common internal defines based on build environment
// Notes:	 
//
///////////////////////////////////////////////////////////////

#ifndef BUILDENV_H
#define BUILDENV_H

#include <linux/version.h>

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6,5,0))
	#define KERNEL_6_5_0_GET_USER_PAGES
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6,4,0))
	#define KERNEL_6_4_0_CLASS_CREATE
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6,3,0))
	#define KERNEL_6_3_0_VM_FLAGS
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6,2,0))
	#define KERNEL_6_2_0_DEV_UEVENT
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6,1,0))
	#define KERNEL_6_1_0_SET_TERMIOS
#endif

#if DISTRO_IS_RHEL_LIKE == 1
	#if DISTRO_MAJ_VERSION == 8
		// RHEL 8 special cases

		// This change was back-ported from kernel 6.2.0 for kernel 4.18.0-504
        #if ((LINUX_VERSION_CODE >= KERNEL_VERSION(4,18,0)) && DISTRO_KERNEL_PKG_MAJ >= 504)
			#define KERNEL_6_2_0_DEV_UEVENT
        #endif
	#endif

	#if DISTRO_MAJ_VERSION >= 9
		// RHEL 9 special cases

		// This change was back-ported from kernel 6.1.0 for kernel 5.14.0-275
		#if ((LINUX_VERSION_CODE >= KERNEL_VERSION(5,14,0)) && DISTRO_KERNEL_PKG_MAJ >= 275)
			#define KERNEL_6_1_0_SET_TERMIOS
		#endif

		// This change was back-ported from kernel 6.2.0 for kernel 5.14.0-362
		#if ((LINUX_VERSION_CODE >= KERNEL_VERSION(5,14,0)) && DISTRO_KERNEL_PKG_MAJ >= 362)
			#define KERNEL_6_2_0_DEV_UEVENT
		#endif

		// This change was back-ported from kernel 6.4.0 for kernel 5.14.0-387
		#if ((LINUX_VERSION_CODE >= KERNEL_VERSION(5,14,0)) && DISTRO_KERNEL_PKG_MAJ >= 387)
			#define KERNEL_6_4_0_CLASS_CREATE
		#endif
	#endif
#endif

#endif
