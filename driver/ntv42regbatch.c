/* SPDX-License-Identifier: MIT */
/**
 * @file      ntv42regbatch.c
 * @brief     NTV42 register batch — ISR-time register sequencing
 * @copyright (C) 2012-2025 AJA Video Systems, Inc.
 * @note      This file is included in driver builds. It must not contain any c++.
 */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/ktime.h>
#include <linux/spinlock.h>

#include "ntv42device.h"
#include "ntv42ioctl.h"

#define NTV42_REGBATCH_MAX_ACTIVE  32

struct ntv42_regbatch_entry {
    uint32_t    bar;
    uint32_t    offset;
    uint32_t    value;
    uint32_t    mask;
};

struct ntv42_regbatch {
    uint32_t                    id;
    uint32_t                    trigger_event;
    uint32_t                    trigger_index;
    uint32_t                    flags;
    int                         entry_count;
    struct ntv42_regbatch_entry *entries;
    uint32_t                    exec_count;
    uint64_t                    last_exec_time;
    int                         state;  /* 0=pending, 1=executed, 2=cancelled */
};

/* Per-device batch storage (simple flat array, no linked list for ISR safety) */
static struct ntv42_regbatch batches[NTV42_DEVICE_MAX][NTV42_REGBATCH_MAX_ACTIVE];
static uint32_t batch_next_id[NTV42_DEVICE_MAX];
static DEFINE_SPINLOCK(batch_lock);

/**
 * Called from ntv42device_event() in ISR context.
 * Executes all pending batches matching the given trigger.
 */
void ntv42_regbatch_check(ntv42_device_t *device, uint32_t event_type, uint32_t event_index)
{
    int dev_idx;
    int i;

    if (device == NULL || device->index >= NTV42_DEVICE_MAX)
        return;

    dev_idx = device->index;

    for (i = 0; i < NTV42_REGBATCH_MAX_ACTIVE; i++) {
        struct ntv42_regbatch *batch = &batches[dev_idx][i];
        int j;

        if (batch->state != 0)  /* not pending */
            continue;
        if (batch->id == 0)  /* empty slot */
            continue;
        if (batch->trigger_event != event_type ||
            batch->trigger_index != event_index)
            continue;

        /* Execute all entries */
        for (j = 0; j < batch->entry_count; j++) {
            struct ntv42_regbatch_entry *e = &batch->entries[j];
            ntv42device_regio_t regio;

            if ((int)e->bar >= device->bar_count)
                continue;

            regio.address = e->offset;
            regio.mask = e->mask;
            regio.shift = 0;
            regio.data = e->value;

            device->bar[e->bar].reg_write(device->host,
                                           device->bar[e->bar].id,
                                           &regio);
        }

        batch->exec_count++;
        batch->last_exec_time = ktime_get_ns();

        if (batch->flags & NTV42_REGBATCH_ONESHOT)
            batch->state = 1; /* executed — no longer pending */
        /* RECURRING: stays pending (state=0) */
    }
}

int ntv42_ioctl_regbatch_submit(ntv42_device_t *device, unsigned long arg)
{
    ntv42_ioctl_regbatch_submit_t param;
    struct ntv42_regbatch *batch = NULL;
    struct ntv42_regbatch_entry *entries = NULL;
    unsigned long flags;
    int dev_idx, i;

    if (device == NULL)
        return NTV42_RETURN_NO_DEVICE;

    if (copy_from_user(&param, (void __user *)arg, sizeof(param)))
        return -EFAULT;

    if (param.entry_count == 0 || param.entry_count > NTV42_REGBATCH_MAX_ENTRIES)
        return NTV42_RETURN_BAD_PARAMETER;

    dev_idx = device->index;
    if (dev_idx >= NTV42_DEVICE_MAX)
        return NTV42_RETURN_BAD_PARAMETER;

    /* Allocate and copy entries from userspace */
    entries = kmalloc_array(param.entry_count, sizeof(*entries), GFP_KERNEL);
    if (entries == NULL)
        return NTV42_RETURN_NO_MEMORY;

    if (copy_from_user(entries,
                       (void __user *)(uintptr_t)param.entries_ptr,
                       param.entry_count * sizeof(ntv42_ioctl_regbatch_entry_t))) {
        kfree(entries);
        return -EFAULT;
    }

    /* Find empty slot and assign batch */
    spin_lock_irqsave(&batch_lock, flags);

    for (i = 0; i < NTV42_REGBATCH_MAX_ACTIVE; i++) {
        if (batches[dev_idx][i].id == 0) {
            batch = &batches[dev_idx][i];
            break;
        }
    }

    if (batch == NULL) {
        spin_unlock_irqrestore(&batch_lock, flags);
        kfree(entries);
        return NTV42_RETURN_NO_RESOURCES;
    }

    batch_next_id[dev_idx]++;
    if (batch_next_id[dev_idx] == 0) batch_next_id[dev_idx] = 1;

    batch->id = batch_next_id[dev_idx];
    batch->trigger_event = param.trigger_event;
    batch->trigger_index = param.trigger_index;
    batch->flags = param.flags;
    batch->entry_count = param.entry_count;
    batch->entries = entries;
    batch->exec_count = 0;
    batch->last_exec_time = 0;
    batch->state = 0; /* pending */

    param.batch_id = batch->id;

    spin_unlock_irqrestore(&batch_lock, flags);

    if (copy_to_user((void __user *)arg, &param, sizeof(param)))
        return -EFAULT;

    return NTV42_RETURN_SUCCESS;
}

int ntv42_ioctl_regbatch_cancel(ntv42_device_t *device, unsigned long arg)
{
    ntv42_ioctl_regbatch_cancel_t param;
    unsigned long flags;
    int dev_idx, i;

    if (device == NULL)
        return NTV42_RETURN_NO_DEVICE;

    if (copy_from_user(&param, (void __user *)arg, sizeof(param)))
        return -EFAULT;

    dev_idx = device->index;
    if (dev_idx >= NTV42_DEVICE_MAX)
        return NTV42_RETURN_BAD_PARAMETER;

    spin_lock_irqsave(&batch_lock, flags);

    for (i = 0; i < NTV42_REGBATCH_MAX_ACTIVE; i++) {
        struct ntv42_regbatch *batch = &batches[dev_idx][i];
        if (batch->id == 0) continue;

        if (param.batch_id == 0 || batch->id == param.batch_id) {
            kfree(batch->entries);
            memset(batch, 0, sizeof(*batch));
        }
    }

    spin_unlock_irqrestore(&batch_lock, flags);
    return NTV42_RETURN_SUCCESS;
}

int ntv42_ioctl_regbatch_status(ntv42_device_t *device, unsigned long arg)
{
    ntv42_ioctl_regbatch_status_t param;
    unsigned long flags;
    int dev_idx, i;

    if (device == NULL)
        return NTV42_RETURN_NO_DEVICE;

    if (copy_from_user(&param, (void __user *)arg, sizeof(param)))
        return -EFAULT;

    dev_idx = device->index;
    if (dev_idx >= NTV42_DEVICE_MAX)
        return NTV42_RETURN_BAD_PARAMETER;

    param.state = 3; /* error — not found */
    param.exec_count = 0;
    param.last_exec_time = 0;

    spin_lock_irqsave(&batch_lock, flags);

    for (i = 0; i < NTV42_REGBATCH_MAX_ACTIVE; i++) {
        struct ntv42_regbatch *batch = &batches[dev_idx][i];
        if (batch->id == param.batch_id) {
            param.state = batch->state;
            param.exec_count = batch->exec_count;
            param.last_exec_time = batch->last_exec_time;
            break;
        }
    }

    spin_unlock_irqrestore(&batch_lock, flags);

    if (copy_to_user((void __user *)arg, &param, sizeof(param)))
        return -EFAULT;

    return NTV42_RETURN_SUCCESS;
}
