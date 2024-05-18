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
#include <time.h>

#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <linux/types.h>
#include <linux/netlink.h>

#include "ajabase/pnp/linux/pnpimpl.h"

static pthread_t child_id = 0;
static bool run = false;

static long long millitime()
{
    struct timespec tt;
    clock_gettime(CLOCK_REALTIME, &tt);
    return tt.tv_sec*1000LL + lround(tt.tv_nsec/1e6);
}

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
    pthread_t child;

    printf("install\n");
    
	mCallback = callback;
	mRefCon = refCon;
	mDevices = devices;

    if (child_id != 0)
        Uninstall();
	
	if (mCallback)
		(*(mCallback))(AJA_Pnp_DeviceAdded, mRefCon);

    run = true;
    if(pthread_create(&child, NULL, *Worker, (void *)this) < 0)
        return AJA_STATUS_FAIL;

    child_id = child;

	return AJA_STATUS_SUCCESS;
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


AJAPnpCallback 
AJAPnpImpl::GetCallback()
{
	return mCallback;
}


void* 
AJAPnpImpl::GetRefCon()
{
	return mRefCon;
}


uint32_t
AJAPnpImpl::GetPnpDevices()
{
	return mDevices;
}

void*
AJAPnpImpl::Worker(void* refCon)
{
    AJAPnpImpl* pPnP = (AJAPnpImpl*)refCon;
	struct sockaddr_nl nls;
	struct pollfd pfd;
	char buf[4096];
    int count = 0;

    printf("worker start\n");

    if (pPnP == NULL)
        return NULL;

	// Open hotplug event netlink socket
	memset(&nls,0,sizeof(struct sockaddr_nl));
	nls.nl_family = AF_NETLINK;
	nls.nl_pid = getpid();
	nls.nl_groups = -1;

	pfd.events = POLLIN;
	pfd.fd = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
	if (pfd.fd == -1)
		return NULL;

	// Listen to netlink socket
	if (bind(pfd.fd, (struct sockaddr *)&nls, sizeof(struct sockaddr_nl)))
		return NULL;;

	while (run)
    {
        poll(&pfd, 1, 100);
        
        int i, len = recv(pfd.fd, buf, sizeof(buf), MSG_DONTWAIT);
//        printf("length %d\n", len);
        if (len <= 0)
        {
            sleep(0.1);
            continue;
        }

        i = 0;
        AJAPnpMessage action = AJA_Pnp_DeviceOffline;
        while (i < len)
        {
//            printf("%s\n", buf + i);
            if (strstr(buf + i, "ACTION=add") != NULL)
                action = AJA_Pnp_DeviceAdded;
            if (strstr(buf + i, "ACTION=remove") != NULL)
                action = AJA_Pnp_DeviceRemoved;

            if ((action != AJA_Pnp_DeviceOffline) && (strstr(buf + i, "DEVNAME=ajantv2"/*"PCI_ID=F1D0"*/) != NULL))
            {
                if (action == AJA_Pnp_DeviceAdded)
                    printf("hotplug add %d\n", count++);
                else
                    printf("hotplug remove %d\n", count++);
                if (pPnP->mCallback)
                    (*(pPnP->mCallback))(action, pPnP->mRefCon);
            }
            i += strlen(buf + i) + 1;
        }
    }

    printf("worker stop %d\n", errno);

    return NULL;
}




