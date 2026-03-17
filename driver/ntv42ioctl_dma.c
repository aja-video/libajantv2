/* SPDX-License-Identifier: MIT */
/**
 * @file      ntv42ioctl_dma.c
 * @brief     NTV42 DMA ioctl handler implementations
 * @copyright (C) 2012-2025 AJA Video Systems, Inc.
 * @note      This file is included in driver builds. It must not contain any c++.
 *
 * The DMA transfer handler uses the ntv42_device_t's DMA callbacks (set up by
 * the host driver). For the ntv42dummy module, these are bounce-buffer based.
 * For the NTV2 driver, they wrap the existing DMA infrastructure.
 */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#include "ntv42device.h"
#include "ntv42ioctl.h"

int ntv42_ioctl_dma_transfer(ntv42_device_t *device, unsigned long arg)
{
    ntv42_ioctl_dma_transfer_t param;
    void *kbuf = NULL;

    if (device == NULL)
        return NTV42_RETURN_NO_DEVICE;

    if (copy_from_user(&param, (void __user *)arg, sizeof(param)))
        return -EFAULT;

    /* Validate */
    if (param.bytes == 0 || param.bytes > (64 * 1024 * 1024))
        return NTV42_RETURN_BAD_PARAMETER;

    if (param.direction != 1 && param.direction != 2)
        return NTV42_RETURN_BAD_PARAMETER;

    /* Use device DMA callback if available */
    if (device->dma_transfer != NULL) {
        int ret = device->dma_transfer(device->host, param.direction,
                                        (void __user *)(uintptr_t)param.host_addr,
                                        param.device_addr, param.bytes,
                                        &param.bytes_xfered);
        param.status = ret;

        if (copy_to_user((void __user *)arg, &param, sizeof(param)))
            return -EFAULT;

        return (ret == 0) ? NTV42_RETURN_SUCCESS : ret;
    }

    /* Fallback: bounce-buffer DMA using device memory array */
    /* This is used by the dummy module */
    if (device->dma_buf == NULL || device->dma_buf_size == 0) {
        param.status = NTV42_RETURN_BAD_STATE;
        param.bytes_xfered = 0;
        if (copy_to_user((void __user *)arg, &param, sizeof(param)))
            return -EFAULT;
        return NTV42_RETURN_BAD_STATE;
    }

    /* Bounds check device address */
    if (param.device_addr + param.bytes > device->dma_buf_size) {
        param.status = NTV42_RETURN_BAD_PARAMETER;
        param.bytes_xfered = 0;
        if (copy_to_user((void __user *)arg, &param, sizeof(param)))
            return -EFAULT;
        return NTV42_RETURN_BAD_PARAMETER;
    }

    if (param.direction == 1) {
        /* To device: copy from user to device buffer */
        if (copy_from_user(device->dma_buf + param.device_addr,
                           (void __user *)(uintptr_t)param.host_addr,
                           param.bytes)) {
            param.status = -EFAULT;
            param.bytes_xfered = 0;
            copy_to_user((void __user *)arg, &param, sizeof(param));
            return -EFAULT;
        }
    } else {
        /* From device: copy from device buffer to user */
        if (copy_to_user((void __user *)(uintptr_t)param.host_addr,
                         device->dma_buf + param.device_addr,
                         param.bytes)) {
            param.status = -EFAULT;
            param.bytes_xfered = 0;
            copy_to_user((void __user *)arg, &param, sizeof(param));
            return -EFAULT;
        }
    }

    param.bytes_xfered = param.bytes;
    param.status = 0;

    if (copy_to_user((void __user *)arg, &param, sizeof(param)))
        return -EFAULT;

    return NTV42_RETURN_SUCCESS;
}

int ntv42_ioctl_dma_info(ntv42_device_t *device, unsigned long arg)
{
    ntv42_ioctl_dma_info_t info;

    if (device == NULL)
        return NTV42_RETURN_NO_DEVICE;

    memset(&info, 0, sizeof(info));

    /* Report one DMA engine if device has DMA buffer */
    if (device->dma_buf != NULL && device->dma_buf_size > 0) {
        info.engine_count = 1;
        info.engines[0].directions = 0x03; /* both directions */
        info.engines[0].host_align = 1;
        info.engines[0].device_align = 1;
        info.engines[0].xfer_align = 1;
        info.engines[0].max_xfer = device->dma_buf_size;
    } else if (device->dma_transfer != NULL) {
        info.engine_count = 1;
        info.engines[0].directions = 0x03;
        info.engines[0].host_align = 4;
        info.engines[0].device_align = 4;
        info.engines[0].xfer_align = 4;
        info.engines[0].max_xfer = 64 * 1024 * 1024;
    }

    if (copy_to_user((void __user *)arg, &info, sizeof(info)))
        return -EFAULT;

    return NTV42_RETURN_SUCCESS;
}
