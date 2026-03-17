/* SPDX-License-Identifier: MIT */
/**
 * @file      ntv42ioctl.c
 * @brief     NTV42 ioctl handler implementations
 * @copyright (C) 2012-2025 AJA Video Systems, Inc.
 * @note      This file is included in driver builds. It must not contain any c++.
 */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#include "ntv42device.h"
#include "ntv42ioctl.h"

int ntv42_ioctl_version(ntv42_device_t *device, unsigned long arg)
{
    ntv42_ioctl_version_t ver;

    if (device == NULL)
        return NTV42_RETURN_NO_DEVICE;

    ver.version = NTV42_IOCTL_VERSION;
    ver.capabilities = NTV42_CAP_REGIO | NTV42_CAP_EVENTS | NTV42_CAP_DMA | NTV42_CAP_REGBATCH;

    if (copy_to_user((void __user *)arg, &ver, sizeof(ver)))
        return -EFAULT;

    return NTV42_RETURN_SUCCESS;
}

int ntv42_ioctl_reg_read(ntv42_device_t *device, unsigned long arg)
{
    ntv42_ioctl_regio_t param;
    ntv42_bar_t *bar;
    ntv42device_regio_t regio;
    uint32_t *kbuf = NULL;
    uint32_t i;
    int ret;

    if (device == NULL)
        return NTV42_RETURN_NO_DEVICE;

    if (copy_from_user(&param, (void __user *)arg, sizeof(param)))
        return -EFAULT;

    /* Validate BAR index */
    if (param.bar >= (uint32_t)device->bar_count)
        return NTV42_RETURN_BAD_PARAMETER;

    bar = &device->bar[param.bar];

    /* Validate count */
    if (param.count == 0)
        return NTV42_RETURN_BAD_PARAMETER;

    /* Validate offset + count within BAR size */
    if (param.offset + (uint64_t)param.count * bar->width > bar->size)
        return NTV42_RETURN_BAD_PARAMETER;

    /* Validate alignment */
    if (param.offset % bar->width != 0)
        return NTV42_RETURN_BAD_PARAMETER;

    /* Allocate kernel buffer */
    kbuf = kmalloc_array(param.count, bar->width, GFP_KERNEL);
    if (kbuf == NULL)
        return NTV42_RETURN_NO_MEMORY;

    /* Read registers through BAR callbacks */
    for (i = 0; i < param.count; i++) {
        memset(&regio, 0, sizeof(regio));
        regio.address = param.offset + (uint64_t)i * bar->width;
        regio.mask = 0xFFFFFFFF;
        regio.shift = 0;

        ret = bar->reg_read(device->host, bar->id, &regio);
        if (ret != NTV42_RETURN_SUCCESS) {
            kfree(kbuf);
            return ret;
        }
        kbuf[i] = regio.data;
    }

    /* Copy results to userspace */
    if (copy_to_user((void __user *)(uintptr_t)param.data_ptr, kbuf,
                     (size_t)param.count * bar->width)) {
        kfree(kbuf);
        return -EFAULT;
    }

    kfree(kbuf);
    return NTV42_RETURN_SUCCESS;
}

int ntv42_ioctl_reg_write(ntv42_device_t *device, unsigned long arg)
{
    ntv42_ioctl_regio_t param;
    ntv42_bar_t *bar;
    ntv42device_regio_t regio;
    uint32_t *kbuf = NULL;
    uint32_t i;
    int ret;

    if (device == NULL)
        return NTV42_RETURN_NO_DEVICE;

    if (copy_from_user(&param, (void __user *)arg, sizeof(param)))
        return -EFAULT;

    /* Validate BAR index */
    if (param.bar >= (uint32_t)device->bar_count)
        return NTV42_RETURN_BAD_PARAMETER;

    bar = &device->bar[param.bar];

    /* Validate count */
    if (param.count == 0)
        return NTV42_RETURN_BAD_PARAMETER;

    /* Validate offset + count within BAR size */
    if (param.offset + (uint64_t)param.count * bar->width > bar->size)
        return NTV42_RETURN_BAD_PARAMETER;

    /* Validate alignment */
    if (param.offset % bar->width != 0)
        return NTV42_RETURN_BAD_PARAMETER;

    /* Allocate kernel buffer */
    kbuf = kmalloc_array(param.count, bar->width, GFP_KERNEL);
    if (kbuf == NULL)
        return NTV42_RETURN_NO_MEMORY;

    /* Copy data from userspace */
    if (copy_from_user(kbuf, (void __user *)(uintptr_t)param.data_ptr,
                       (size_t)param.count * bar->width)) {
        kfree(kbuf);
        return -EFAULT;
    }

    /* Write registers through BAR callbacks */
    for (i = 0; i < param.count; i++) {
        memset(&regio, 0, sizeof(regio));
        regio.address = param.offset + (uint64_t)i * bar->width;
        regio.mask = 0xFFFFFFFF;
        regio.shift = 0;
        regio.data = kbuf[i];

        ret = bar->reg_write(device->host, bar->id, &regio);
        if (ret != NTV42_RETURN_SUCCESS) {
            kfree(kbuf);
            return ret;
        }
    }

    kfree(kbuf);
    return NTV42_RETURN_SUCCESS;
}

int ntv42_ioctl_device_info(ntv42_device_t *device, unsigned long arg)
{
    ntv42_ioctl_device_info_t info;
    int i;

    if (device == NULL)
        return NTV42_RETURN_NO_DEVICE;

    memset(&info, 0, sizeof(info));

    /* Copy device config */
    memcpy(info.name, device->config.name, sizeof(info.name));
    memcpy(info.desc, device->config.desc, sizeof(info.desc));
    memcpy(info.serial, device->config.serial, sizeof(info.serial));
    info.bar_count = device->bar_count;

    /* Copy BAR info */
    for (i = 0; i < device->bar_count && i < 8; i++) {
        memcpy(info.bar[i].name, device->bar[i].name, sizeof(info.bar[i].name));
        info.bar[i].address = device->bar[i].address;
        info.bar[i].size = device->bar[i].size;
        info.bar[i].width = device->bar[i].width;
    }

    if (copy_to_user((void __user *)arg, &info, sizeof(info)))
        return -EFAULT;

    return NTV42_RETURN_SUCCESS;
}
