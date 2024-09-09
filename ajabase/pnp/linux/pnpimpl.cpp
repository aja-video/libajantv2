/* SPDX-License-Identifier: MIT */
/**
	@file		pnp/linux/pnpimpl.cpp
	@brief		Implements the AJAPnpImpl class on the Linux platform.
	@copyright	(C) 2011-2022 AJA Video Systems, Inc.  All rights reserved.
**/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <libudev.h>

#include "ajabase/pnp/linux/pnpimpl.h"

static pthread_t child_id = 0;
static bool run = false;

AJAPnpImpl::AJAPnpImpl() : mRefCon(NULL), mCallback(NULL), mDevices(0)
{
}


AJAPnpImpl::~AJAPnpImpl()
{
	Uninstall();
}


AJAStatus 
AJAPnpImpl::Install(AJAPnpCallback callback, void* refCon, uint32_t devices)
{
	mCallback = callback;
	mRefCon   = refCon;
	mDevices  = devices;

	if (!mCallback)
		return AJA_STATUS_NULL;	//	NULL callback

	if (child_id)
		Uninstall();

	//	Linux only handles PCIe devices
	if (mDevices & AJA_Pnp_PciVideoDevices)
	{
		run = true;
		pthread_t child;
		if (pthread_create(&child, NULL, *Worker, (void *)this) < 0)
			return AJA_STATUS_FAIL;	//	Failed to create Worker thread

		(*(mCallback))(AJA_Pnp_DeviceAdded, mRefCon);

		child_id = child;
		return AJA_STATUS_SUCCESS;
	}
	return AJA_STATUS_FAIL;
}
	

AJAStatus 
AJAPnpImpl::Uninstall(void)
{
    if (child_id == 0)
        return AJA_STATUS_SUCCESS;

    run = false;

    struct timespec tt;
    clock_gettime(CLOCK_REALTIME, &tt);
    tt.tv_sec += 1;;

    pthread_timedjoin_np(child_id, NULL, &tt);

    child_id = 0;
	mCallback = NULL;
	mRefCon = NULL;
	mDevices = 0;

	return AJA_STATUS_SUCCESS;
}


void*
AJAPnpImpl::Worker(void* refCon)
{
    AJAPnpImpl* pPnP = (AJAPnpImpl*)refCon;
    struct udev *udev;
	struct udev_device *dev;
   	struct udev_monitor *mon;
	int fd;

	udev = udev_new();
	if (udev == NULL)
    {
		return NULL;
	}

	mon = udev_monitor_new_from_netlink(udev, "udev");
	udev_monitor_filter_add_match_subsystem_devtype(mon, "ajantv2", NULL);
	udev_monitor_enable_receiving(mon);
	fd = udev_monitor_get_fd(mon);

	while (run)
    {
		fd_set fds;
		struct timeval tv;
		int ret;

		FD_ZERO(&fds);
		FD_SET(fd, &fds);
		tv.tv_sec = 0;
		tv.tv_usec = 1000000;

		ret = select(fd+1, &fds, NULL, NULL, &tv);
		if (ret > 0 && FD_ISSET(fd, &fds)) {
			dev = udev_monitor_receive_device(mon);
			if (dev) {
//				printf("I: ACTION=%s\n", udev_device_get_action(dev));
//				printf("I: DEVNAME=%s\n", udev_device_get_sysname(dev));
//				printf("I: DEVPATH=%s\n", udev_device_get_devpath(dev));
                AJAPnpMessage action = AJA_Pnp_DeviceOffline;
                if (strstr(udev_device_get_action(dev), "add") != NULL)
                {
                    action = AJA_Pnp_DeviceAdded;
//                    printf("hotplug add %d\n", count++);
                }
                if (strstr(udev_device_get_action(dev), "remove") != NULL)
                {
                    action = AJA_Pnp_DeviceRemoved;
//                    printf("hotplug remove %d\n", count++);
                }
                if ((action != AJA_Pnp_DeviceOffline) && (pPnP->mCallback != NULL))
                    (*(pPnP->mCallback))(action, pPnP->mRefCon);
                
				udev_device_unref(dev);
			}
		}
	}

	udev_unref(udev);

    return NULL;
}


