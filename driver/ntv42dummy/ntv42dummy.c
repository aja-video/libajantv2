/* SPDX-License-Identifier: MIT */
/**
 * @file      ntv42dummy.c
 * @brief     NTV42 dummy driver module for testing
 * @copyright (C) 2012-2025 AJA Video Systems, Inc.
 *
 * Standalone kernel module that creates /dev/ntv42dummyN device nodes
 * with memory-backed registers. Shares ntv42device_* and ntv42ioctl.c
 * handler code with the NTV2 driver.
 *
 * Build:  make -C /lib/modules/$(uname -r)/build M=$(pwd) modules
 * Load:   sudo insmod ntv42dummy.ko [num_devices=N]
 * Remove: sudo rmmod ntv42dummy
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

/* Build flags needed by ntv42device.h */
#ifndef AJALinux
#define AJALinux
#endif
#ifndef AJA_NTV42
#define AJA_NTV42 1
#endif

#include "../ntv42device.h"
#include "../ntv42ioctl.h"

/* Forward declarations for shared ioctl handlers */
extern int ntv42_ioctl_version(ntv42_device_t *device, unsigned long arg);
extern int ntv42_ioctl_reg_read(ntv42_device_t *device, unsigned long arg);
extern int ntv42_ioctl_reg_write(ntv42_device_t *device, unsigned long arg);
extern int ntv42_ioctl_device_info(ntv42_device_t *device, unsigned long arg);
extern int ntv42_ioctl_event_control(ntv42_device_t *device, unsigned long arg);
extern int ntv42_ioctl_event_wait(ntv42_device_t *device, unsigned long arg);
extern int ntv42_ioctl_event_status(ntv42_device_t *device, unsigned long arg);
extern int ntv42_ioctl_dma_transfer(ntv42_device_t *device, unsigned long arg);
extern int ntv42_ioctl_dma_info(ntv42_device_t *device, unsigned long arg);
extern int ntv42_ioctl_regbatch_submit(ntv42_device_t *device, unsigned long arg);
extern int ntv42_ioctl_regbatch_cancel(ntv42_device_t *device, unsigned long arg);
extern int ntv42_ioctl_regbatch_status(ntv42_device_t *device, unsigned long arg);

#define NTV42_DUMMY_MAX_DEVICES  4
#define NTV42_DUMMY_BAR0_REGS    1024
#define NTV42_DUMMY_BAR0_WIDTH   4
#define NTV42_DUMMY_BAR0_SIZE    (NTV42_DUMMY_BAR0_REGS * NTV42_DUMMY_BAR0_WIDTH)
#define NTV42_DUMMY_DMA_BUF_SIZE (1024 * 1024)  /* 1MB DMA bounce buffer */

static int num_devices = 1;
module_param(num_devices, int, 0444);
MODULE_PARM_DESC(num_devices, "Number of dummy devices to create (default 1, max 4)");

struct ntv42_dummy_state {
    ntv42_device_t  *device;
    struct cdev     cdev;
    uint32_t        registers[NTV42_DUMMY_BAR0_REGS];
    struct timer_list vsync_timer;
    bool            vsync_running;
};

static int ntv42dummy_major;
static struct class *ntv42dummy_class;
static struct ntv42_dummy_state dummy_state[NTV42_DUMMY_MAX_DEVICES];
static int ntv42dummy_device_count;

/*============================================================================
 * BAR callbacks (memory-backed registers)
 *==========================================================================*/

static int dummy_reg_read(void *host, void *id, ntv42device_regio_t *regio)
{
    struct ntv42_dummy_state *state = (struct ntv42_dummy_state *)host;
    uint32_t reg_idx;
    (void)id;

    if (state == NULL || regio == NULL)
        return NTV42_RETURN_BAD_PARAMETER;

    reg_idx = (uint32_t)(regio->address / NTV42_DUMMY_BAR0_WIDTH);
    if (reg_idx >= NTV42_DUMMY_BAR0_REGS)
        return NTV42_RETURN_BAD_PARAMETER;

    regio->data = (state->registers[reg_idx] & regio->mask) >> regio->shift;
    return NTV42_RETURN_SUCCESS;
}

static int dummy_reg_write(void *host, void *id, ntv42device_regio_t *regio)
{
    struct ntv42_dummy_state *state = (struct ntv42_dummy_state *)host;
    uint32_t reg_idx;
    (void)id;

    if (state == NULL || regio == NULL)
        return NTV42_RETURN_BAD_PARAMETER;

    reg_idx = (uint32_t)(regio->address / NTV42_DUMMY_BAR0_WIDTH);
    if (reg_idx >= NTV42_DUMMY_BAR0_REGS)
        return NTV42_RETURN_BAD_PARAMETER;

    if (regio->mask == 0xFFFFFFFF) {
        state->registers[reg_idx] = regio->data;
    } else {
        state->registers[reg_idx] = (state->registers[reg_idx] & ~regio->mask) |
                                     ((regio->data << regio->shift) & regio->mask);
    }
    return NTV42_RETURN_SUCCESS;
}

/*============================================================================
 * Chardev file operations
 *==========================================================================*/

static int ntv42dummy_open(struct inode *inode, struct file *file)
{
    int minor = iminor(inode);
    if (minor < 0 || minor >= ntv42dummy_device_count)
        return -ENODEV;
    file->private_data = &dummy_state[minor];
    return 0;
}

static int ntv42dummy_release(struct inode *inode, struct file *file)
{
    (void)inode;
    file->private_data = NULL;
    return 0;
}

static long ntv42dummy_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct ntv42_dummy_state *state = file->private_data;

    if (state == NULL || state->device == NULL)
        return -ENODEV;

    if (_IOC_TYPE(cmd) != NTV42_DEVICE_TYPE)
        return -ENOTTY;

    switch (cmd) {
    case IOCTL_NTV42_VERSION:
        return ntv42_ioctl_version(state->device, arg);
    case IOCTL_NTV42_REG_READ:
        return ntv42_ioctl_reg_read(state->device, arg);
    case IOCTL_NTV42_REG_WRITE:
        return ntv42_ioctl_reg_write(state->device, arg);
    case IOCTL_NTV42_DEVICE_INFO:
        return ntv42_ioctl_device_info(state->device, arg);
    case IOCTL_NTV42_EVENT_CONTROL:
        return ntv42_ioctl_event_control(state->device, arg);
    case IOCTL_NTV42_EVENT_WAIT:
        return ntv42_ioctl_event_wait(state->device, arg);
    case IOCTL_NTV42_EVENT_STATUS:
        return ntv42_ioctl_event_status(state->device, arg);
    case IOCTL_NTV42_DMA_TRANSFER:
        return ntv42_ioctl_dma_transfer(state->device, arg);
    case IOCTL_NTV42_DMA_INFO:
        return ntv42_ioctl_dma_info(state->device, arg);
    case IOCTL_NTV42_REGBATCH_SUBMIT:
        return ntv42_ioctl_regbatch_submit(state->device, arg);
    case IOCTL_NTV42_REGBATCH_CANCEL:
        return ntv42_ioctl_regbatch_cancel(state->device, arg);
    case IOCTL_NTV42_REGBATCH_STATUS:
        return ntv42_ioctl_regbatch_status(state->device, arg);
    default:
        return -ENOTTY;
    }
}

static const struct file_operations ntv42dummy_fops = {
    .owner          = THIS_MODULE,
    .open           = ntv42dummy_open,
    .release        = ntv42dummy_release,
    .unlocked_ioctl = ntv42dummy_ioctl,
};

/*============================================================================
 * Vsync timer simulation (~60Hz)
 *==========================================================================*/

#define NTV42_DUMMY_VSYNC_INTERVAL_MS  16  /* ~60Hz */

static void ntv42dummy_vsync_timer_fn(struct timer_list *t)
{
    struct ntv42_dummy_state *state = from_timer(state, t, vsync_timer);

    if (state->device == NULL || !state->vsync_running)
        return;

    /* Generate output vsync event for output 0 */
    ntv42device_event(state->device, 0x0001, 0);

    /* Generate input vsync event for input 0 */
    ntv42device_event(state->device, 0x0002, 0);

    /* Re-arm timer */
    mod_timer(&state->vsync_timer,
              jiffies + msecs_to_jiffies(NTV42_DUMMY_VSYNC_INTERVAL_MS));
}

/*============================================================================
 * Module init/exit
 *==========================================================================*/

static int __init ntv42dummy_init(void)
{
    int i, ret;
    dev_t dev;

    if (num_devices < 1)
        num_devices = 1;
    if (num_devices > NTV42_DUMMY_MAX_DEVICES)
        num_devices = NTV42_DUMMY_MAX_DEVICES;

    printk(KERN_INFO "ntv42dummy: initializing %d dummy device(s)\n", num_devices);

    ntv42device_init();

    /* Register chardev region */
    ret = alloc_chrdev_region(&dev, 0, num_devices, "ntv42dummy");
    if (ret < 0) {
        printk(KERN_ERR "ntv42dummy: failed to allocate chrdev region\n");
        return ret;
    }
    ntv42dummy_major = MAJOR(dev);

    ntv42dummy_class = class_create("ntv42dummy");
    if (IS_ERR(ntv42dummy_class)) {
        unregister_chrdev_region(MKDEV(ntv42dummy_major, 0), num_devices);
        return PTR_ERR(ntv42dummy_class);
    }

    /* Create each dummy device */
    for (i = 0; i < num_devices; i++) {
        ntv42_device_config_t config;
        ntv42_bar_t bar0;
        struct ntv42_dummy_state *state = &dummy_state[i];

        memset(state, 0, sizeof(*state));

        /* Create ntv42 device */
        ret = ntv42device_create(&state->device, state);
        if (ret != NTV42_RETURN_SUCCESS) {
            printk(KERN_ERR "ntv42dummy: failed to create device %d\n", i);
            goto err_cleanup;
        }

        /* Configure */
        memset(&config, 0, sizeof(config));
        snprintf(config.name, sizeof(config.name), "dummy");
        snprintf(config.desc, sizeof(config.desc), "ntv42dummy dev %d", i);
        snprintf(config.serial, sizeof(config.serial), "DUMMY%04d", i);
        ntv42device_config(state->device, &config);

        /* Add BAR0 */
        memset(&bar0, 0, sizeof(bar0));
        snprintf(bar0.name, sizeof(bar0.name), "bar0");
        bar0.address = 0;
        bar0.size = NTV42_DUMMY_BAR0_SIZE;
        bar0.width = NTV42_DUMMY_BAR0_WIDTH;
        bar0.reg_read = dummy_reg_read;
        bar0.reg_write = dummy_reg_write;
        ntv42device_bar_add(state->device, &bar0);

        /* Allocate DMA bounce buffer */
        state->device->dma_buf = kzalloc(NTV42_DUMMY_DMA_BUF_SIZE, GFP_KERNEL);
        if (state->device->dma_buf != NULL)
            state->device->dma_buf_size = NTV42_DUMMY_DMA_BUF_SIZE;

        ntv42device_state(state->device, ntv42device_state_enable);

        /* Create chardev */
        dev = MKDEV(ntv42dummy_major, i);
        cdev_init(&state->cdev, &ntv42dummy_fops);
        state->cdev.owner = THIS_MODULE;
        ret = cdev_add(&state->cdev, dev, 1);
        if (ret < 0) {
            printk(KERN_ERR "ntv42dummy: failed to add cdev %d\n", i);
            goto err_cleanup;
        }

        if (IS_ERR(device_create(ntv42dummy_class, NULL, dev, NULL, "ntv42dummy%d", i))) {
            printk(KERN_ERR "ntv42dummy: failed to create device node %d\n", i);
            cdev_del(&state->cdev);
            goto err_cleanup;
        }

        /* Start vsync simulation timer */
        timer_setup(&state->vsync_timer, ntv42dummy_vsync_timer_fn, 0);
        state->vsync_running = true;
        mod_timer(&state->vsync_timer,
                  jiffies + msecs_to_jiffies(NTV42_DUMMY_VSYNC_INTERVAL_MS));

        ntv42dummy_device_count++;
        printk(KERN_INFO "ntv42dummy: created /dev/ntv42dummy%d\n", i);
    }

    return 0;

err_cleanup:
    for (i = i - 1; i >= 0; i--) {
        dummy_state[i].vsync_running = false;
        del_timer_sync(&dummy_state[i].vsync_timer);
        device_destroy(ntv42dummy_class, MKDEV(ntv42dummy_major, i));
        cdev_del(&dummy_state[i].cdev);
        ntv42device_state(dummy_state[i].device, ntv42device_state_disable);
        ntv42device_release(dummy_state[i].device);
        ntv42dummy_device_count--;
    }
    class_destroy(ntv42dummy_class);
    unregister_chrdev_region(MKDEV(ntv42dummy_major, 0), num_devices);
    return ret;
}

static void __exit ntv42dummy_exit(void)
{
    int i;

    for (i = 0; i < ntv42dummy_device_count; i++) {
        dummy_state[i].vsync_running = false;
        del_timer_sync(&dummy_state[i].vsync_timer);
        device_destroy(ntv42dummy_class, MKDEV(ntv42dummy_major, i));
        cdev_del(&dummy_state[i].cdev);
        kfree(dummy_state[i].device->dma_buf);
        dummy_state[i].device->dma_buf = NULL;
        ntv42device_state(dummy_state[i].device, ntv42device_state_disable);
        ntv42device_release(dummy_state[i].device);
    }

    class_destroy(ntv42dummy_class);
    unregister_chrdev_region(MKDEV(ntv42dummy_major, 0), num_devices);

    printk(KERN_INFO "ntv42dummy: unloaded\n");
}

module_init(ntv42dummy_init);
module_exit(ntv42dummy_exit);

MODULE_LICENSE("MIT");
MODULE_AUTHOR("AJA Video Systems, Inc.");
MODULE_DESCRIPTION("NTV42 dummy driver for testing");
