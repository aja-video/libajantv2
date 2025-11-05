/**
 * @file      device.h
 * @brief     NTV42 driver device
 * @copyright AJA Video.  All rights reserved.
 * 
 * See LICENSE file in the project root folder for license information
 */

#pragma once

#ifdef AJALinux

#include <linux/types.h>

#define NTV42_RETURN_SUCCESS        (0)
#define NTV42_RETURN_FAIL           (-EAGAIN)
#define NTV42_RETURN_NO_DEVICE      (-ENODEV)
#define NTV42_RETURN_BAD_STATE      (-EPERM)
#define NTV42_RETURN_BAD_PARAMETER  (-EINVAL)
#define NTV42_RETURN_NO_MEMORY      (-ENOMEM)
#define NTV42_RETURN_BUSY           (-EBUSY)
#define NTV42_RETURN_IO_ERROR       (-EIO)
#define NTV42_RETURN_TIMEOUT        (-ETIME)
#define NTV42_RETURN_NO_RESOURCES   (-ENOMEM)

#define ntv42_info(string, args...) printk(KERN_ALERT string, ##args)

#endif

#define NTV42_DEVICE_MAX        8
#define NTV42_DEVICE_NAME_MAX   16
#define NTV42_DEVICE_DESC_MAX   32
#define NTV42_DEVICE_BAR_MAX    8

/** Host device state */
typedef enum ntv42device_state_t {
    ntv42device_state_unknown,
    ntv42device_state_init,         // Device initialized
    ntv42device_state_enable,       // Allocate resources
    ntv42device_state_disable,      // Release resources
    ntv42device_state_suspend,      // Suspend device message and interrupt functions
    ntv42device_state_resume,       // Resume device message and interrupt functions
    ntv42device_state_count
} ntv42device_state_t;

typedef struct ntv42device_regio_t {
    uint64_t    address;            // Register address
    uint32_t    mask;               // Field bit mask
    uint32_t    shift;              // Field bit shift
    uint32_t    data;               // Field data
} ntv42device_regio_t;

typedef struct ntv42_bar_t {
    char                    name[NTV42_DEVICE_NAME_MAX];        // Bar name
    void*                   id;                                 // Host id
    uint64_t                address;                            // Bar base address
    uint64_t                size;                               // Bar size
    uint32_t                width;                              // Width for bar access
    int                     (*reg_read)(void* host, void* id, ntv42device_regio_t* regio);
    int                     (*reg_write)(void* host, void* id, ntv42device_regio_t* regio);
} ntv42_bar_t;

typedef struct ntv42_device_config_t {
    char                    name[NTV42_DEVICE_NAME_MAX];        // Message oriented device name
    char                    desc[NTV42_DEVICE_DESC_MAX];        // A user oriented description of the device
    char                    serial[NTV42_DEVICE_DESC_MAX];      // Serial number or other way to uniquely identify the device
} ntv42_device_config_t;

typedef struct ntv42_device_t {
    uint32_t                index;                              // Device index
    void*                   host;                               // Device driver context
    ntv42_device_config_t   config;                             // Device configuration
    ntv42device_state_t     state;                              // Device state
    int                     bar_count;                          // Total number of bars
    ntv42_bar_t             bar[NTV42_DEVICE_BAR_MAX];          // Bar info
} ntv42_device_t;


/** init device manager */
int ntv42device_init(void);

/** create a device */
int ntv42device_create(ntv42_device_t** device, void* host);
int ntv42device_config(ntv42_device_t* device, ntv42_device_config_t* config);
int ntv42device_release(ntv42_device_t* device);

/** add a register bar */
int ntv42device_bar_add(ntv42_device_t* device, ntv42_bar_t* bar);

/** Set device state */
int ntv42device_state(ntv42_device_t* device, ntv42device_state_t state);

/** Device message handler */
int ntv42device_message(ntv42_device_t* device, void* message, uint32_t size);

/** Device event handler */
int ntv42device_event(ntv42_device_t* device, uint32_t index);




