/* SPDX-License-Identifier: MIT */
/**
 * @file      ntv42ioctl_event.c
 * @brief     NTV42 event ioctl handler implementations
 * @copyright (C) 2012-2025 AJA Video Systems, Inc.
 * @note      This file is included in driver builds. It must not contain any c++.
 */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/ktime.h>

#include "ntv42device.h"
#include "ntv42ioctl.h"

int ntv42_ioctl_event_control(ntv42_device_t *device, unsigned long arg)
{
    ntv42_ioctl_event_control_t param;
    int slot;

    if (device == NULL)
        return NTV42_RETURN_NO_DEVICE;

    if (copy_from_user(&param, (void __user *)arg, sizeof(param)))
        return -EFAULT;

    slot = ntv42device_event_slot(param.type, param.index);
    if (slot < 0)
        return NTV42_RETURN_BAD_PARAMETER;

    device->event_enabled[slot] = (param.enable != 0);
    param.count = device->event_count[slot];

    if (copy_to_user((void __user *)arg, &param, sizeof(param)))
        return -EFAULT;

    return NTV42_RETURN_SUCCESS;
}

int ntv42_ioctl_event_wait(ntv42_device_t *device, unsigned long arg)
{
    ntv42_ioctl_event_wait_t param;
    int slot;
    uint64_t start_count;
    long timeout_jiffies;
    long ret;

    if (device == NULL)
        return NTV42_RETURN_NO_DEVICE;

    if (copy_from_user(&param, (void __user *)arg, sizeof(param)))
        return -EFAULT;

    slot = ntv42device_event_slot(param.type, param.index);
    if (slot < 0)
        return NTV42_RETURN_BAD_PARAMETER;

    start_count = device->event_count[slot];

    /* Determine timeout */
    if (param.timeout_ms == 0) {
        /* Poll mode — check immediately */
        if (device->event_count[slot] == start_count) {
            return NTV42_RETURN_TIMEOUT;
        }
    } else if (param.timeout_ms < 0) {
        /* Infinite wait */
        ret = wait_event_interruptible(device->event_wait,
            device->event_count[slot] != start_count);
        if (ret != 0)
            return -EINTR;
    } else {
        /* Timed wait */
        timeout_jiffies = msecs_to_jiffies(param.timeout_ms);
        ret = wait_event_interruptible_timeout(device->event_wait,
            device->event_count[slot] != start_count,
            timeout_jiffies);
        if (ret == 0)
            return NTV42_RETURN_TIMEOUT;
        if (ret < 0)
            return -EINTR;
    }

    /* Fill output */
    param.count = device->event_count[slot];
    param.timestamp = device->event_timestamp[slot];

    if (copy_to_user((void __user *)arg, &param, sizeof(param)))
        return -EFAULT;

    return NTV42_RETURN_SUCCESS;
}

int ntv42_ioctl_event_status(ntv42_device_t *device, unsigned long arg)
{
    ntv42_ioctl_event_status_t param;
    int slot;

    if (device == NULL)
        return NTV42_RETURN_NO_DEVICE;

    if (copy_from_user(&param, (void __user *)arg, sizeof(param)))
        return -EFAULT;

    slot = ntv42device_event_slot(param.type, param.index);
    param.supported = (slot >= 0) ? 1 : 0;

    if (slot >= 0) {
        param.enabled = device->event_enabled[slot] ? 1 : 0;
        param.count = device->event_count[slot];
    } else {
        param.enabled = 0;
        param.count = 0;
    }

    if (copy_to_user((void __user *)arg, &param, sizeof(param)))
        return -EFAULT;

    return NTV42_RETURN_SUCCESS;
}
