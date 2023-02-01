/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
///////////////////////////////////////////////////////////////
//
// HEVC Linux Device Driver for AJA HEVC boards.
//
////////////////////////////////////////////////////////////
//
// Filename: hevcdriver.h
// Purpose:	 Main headerfile for driver.
// Notes:	 PCI Device IDs, memory sizes, fops prototypes
//
///////////////////////////////////////////////////////////////

#ifndef HEVCDRIVER_H
#define HEVCDRIVER_H

#include "ntv2system.h"

#ifdef __cplusplus
extern "C"
{
#endif

#if defined (MSWindows)

#elif defined (AJAMac)

// tbd

#elif defined (AJALinux)

int		hevc_module_init(const char* pModuleName);
int		hevc_probe(struct pci_dev *dev, const struct pci_device_id *id);
void	hevc_module_cleanup(void);
int		hevc_reboot_handler(struct notifier_block *this, unsigned long code, void *x);

#endif

#ifdef __cplusplus
}
#endif

#endif

