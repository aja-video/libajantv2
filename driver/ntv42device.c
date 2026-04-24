/* SPDX-License-Identifier: MIT */
/**
    @file		ntv42device.cpp
    @brief		NTV42 device
    @copyright	(C) 2012-2025 AJA Video Systems, Inc.
    @note		This file is included in driver builds. It must not contain any c++.
**/

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/ktime.h>
#include <linux/sched.h>

#include "ntv42device.h"
#include "ntv42message.h"


static ntv42_device_t ntv42_device[NTV42_DEVICE_MAX];

static const char* ntv42_device_state_string[] = {
    "unknown",
    "initial",
    "enable",
    "disable",
    "suspend",
    "resume"
};

static int ntv42device_do_info(ntv42_device_t* device, ntv42_message_device_info_t* message);
static const char* ntv42device_state_str(ntv42device_state_t state);


int ntv42device_init()
{
    ntv42_info("ntv42: device initialize\n");

    memset(ntv42_device, 0, sizeof(ntv42_device));
    return NTV42_RETURN_SUCCESS;
}

int ntv42device_create(ntv42_device_t** device, void* host)
{
    ntv42_device_t* dev = NULL;
    int i;
    
    if (device == NULL)
        return NTV42_RETURN_BAD_PARAMETER;

    // find empty device slot
    for (i = 0; i < NTV42_DEVICE_MAX; i++) {
        if (ntv42_device[i].state == ntv42device_state_unknown)
            break;
    }
    if (i == NTV42_DEVICE_MAX)
        return NTV42_RETURN_NO_RESOURCES;

    dev = ntv42_device + i;
    dev->index = i;
    dev->host = host;
    dev->state = ntv42device_state_init;
    init_waitqueue_head(&dev->event_wait);

    ntv42_info("ntv42dev%d: create device\n", dev->index);

    *device = dev;
    return NTV42_RETURN_SUCCESS;
}

int ntv42device_config(ntv42_device_t* device, ntv42_device_config_t* config)
{
    if ((device == NULL) || (device->state == ntv42device_state_unknown) || (config == NULL))
        return NTV42_RETURN_BAD_PARAMETER;

    ntv42_info("ntv42dev%d: device config %s\n", device->index, config->name);

    memcpy (&device->config, config, sizeof(ntv42_device_config_t));
    return NTV42_RETURN_SUCCESS;
}

int ntv42device_release(ntv42_device_t* device)
{
    if ((device == NULL) || (device->state == ntv42device_state_unknown))
        return NTV42_RETURN_BAD_PARAMETER;

    ntv42_info("ntv42dev%d: release device\n", device->index);

    memset (device, 0, sizeof(ntv42_device_t));
    return NTV42_RETURN_SUCCESS;
}

int ntv42device_bar_add(ntv42_device_t* device, ntv42_bar_t* bar)
{
    if ((device == NULL) || (device->state == ntv42device_state_unknown) || (bar == NULL))
        return NTV42_RETURN_BAD_PARAMETER;

    if (device->bar_count >= NTV42_DEVICE_BAR_MAX)
        return NTV42_RETURN_NO_RESOURCES;

    memcpy(device->bar + device->bar_count, bar, sizeof(ntv42_bar_t));

    ntv42_info("ntv42dev%d: add bar %s  address %llx  size %lld  width %d\n",
               device->index, bar->name, bar->address, bar->size, bar->width);
    
    device->bar_count++;
    return NTV42_RETURN_SUCCESS;
}

int ntv42device_state(ntv42_device_t* device, ntv42device_state_t state)
{
    if ((device == NULL) || (device->state == ntv42device_state_unknown))
        return NTV42_RETURN_BAD_PARAMETER;

    ntv42_info("ntv42dev%d: set device state %s\n", device->index, ntv42device_state_str(state));

    device->state = state;
    return NTV42_RETURN_SUCCESS;
}

int ntv42device_message(ntv42_device_t* device, void* message, uint32_t size)
{
    if ((device == NULL) || (device->state == ntv42device_state_unknown) ||
        (message == NULL) || (size < sizeof(struct ntv42_message_header_t)))
        return NTV42_RETURN_BAD_PARAMETER;

    if (device->state != ntv42device_state_enable)
        return NTV42_RETURN_BAD_STATE;

    ntv42_message_header_t* hdr = (ntv42_message_header_t*)message;
    if (!NTV42_HEADER_VERIFY(hdr))
        return NTV42_RETURN_BAD_PARAMETER;

    hdr->status = NTV42_MESSAGE_FAIL;
    int ret = NTV42_RETURN_BAD_PARAMETER;
    switch(hdr->type)
    {
    case NTV42_DEVICE_INFO_TYPE:
        ntv42_info("ntv42dev%d: message info received", device->index);
        ntv42_message_device_info_t* msg = (ntv42_message_device_info_t*)message;
        if (NTV42_DEVICE_INFO_VERIFY(msg))
            ret = ntv42device_do_info(device, msg);
        break;
    default:
        break;
    }

    return ret;
}

/**
 * Event slot layout (flat index into event_count/event_enabled arrays):
 *   0-7:   output_vsync[0..7]   (type 0x0001)
 *   8-15:  input_vsync[0..7]    (type 0x0002)
 *   16:    reference[0]         (type 0x0003)
 *   17-24: input_change[0..7]   (type 0x0004)
 *   25-32: audio_in_wrap[0..7]  (type 0x0200)
 *   33-40: audio_out_wrap[0..7] (type 0x0201)
 *   41-48: output_anc[0..7]     (type 0x0005)
 *   49-56: input_anc[0..7]      (type 0x0006)
 * Returns -1 if type/index combination is not mapped.
 */
int ntv42device_event_slot(uint32_t type, uint32_t index)
{
    switch (type) {
    case 0x0001: /* output_vsync */
        return (index < NTV42_EVENT_MAX_OUTPUTS) ? (int)(0 + index) : -1;
    case 0x0002: /* input_vsync */
        return (index < NTV42_EVENT_MAX_INPUTS) ? (int)(8 + index) : -1;
    case 0x0003: /* reference */
        return (index == 0) ? 16 : -1;
    case 0x0004: /* input_change */
        return (index < NTV42_EVENT_MAX_INPUTS) ? (int)(17 + index) : -1;
    case 0x0005: /* output_anc */
        return (index < NTV42_EVENT_MAX_OUTPUTS) ? (int)(41 + index) : -1;
    case 0x0006: /* input_anc */
        return (index < NTV42_EVENT_MAX_INPUTS) ? (int)(49 + index) : -1;
    case 0x0200: /* audio_in_wrap */
        return (index < 8) ? (int)(25 + index) : -1;
    case 0x0201: /* audio_out_wrap */
        return (index < 8) ? (int)(33 + index) : -1;
    default:
        return -1;
    }
}

bool ntv42device_event_supported(ntv42_device_t* device, uint32_t type, uint32_t index)
{
    if (device == NULL)
        return false;
    return ntv42device_event_slot(type, index) >= 0;
}

/* Forward declaration for regbatch ISR hook */
extern void ntv42_regbatch_check(ntv42_device_t *device, uint32_t event_type, uint32_t event_index);

int ntv42device_event(ntv42_device_t* device, uint32_t type, uint32_t index)
{
    int slot;

    if ((device == NULL) || (device->state == ntv42device_state_unknown))
        return NTV42_RETURN_BAD_PARAMETER;

    slot = ntv42device_event_slot(type, index);
    if (slot < 0 || slot >= NTV42_EVENT_SLOT_MAX)
        return NTV42_RETURN_BAD_PARAMETER;

    /* Execute any register batches triggered by this event (Phase 4) */
    ntv42_regbatch_check(device, type, index);

    /* Increment counter and record timestamp */
    device->event_count[slot]++;
    device->event_timestamp[slot] = ktime_get_ns();

    /* Wake anyone waiting for events on this device */
    wake_up(&device->event_wait);

    return NTV42_RETURN_SUCCESS;
}

static int ntv42device_do_info(ntv42_device_t* device, ntv42_message_device_info_t* message)
{
    if ((device == NULL) || (device->state == ntv42device_state_unknown))
        return NTV42_RETURN_BAD_PARAMETER;
    
    snprintf(message->name, sizeof(message->name), "%s", device->config.name);
    snprintf(message->desc, sizeof(message->desc), "%s", device->config.desc);
    snprintf(message->serial, sizeof(message->serial), "%s", device->config.serial);
    message->header.status = NTV42_MESSAGE_SUCCESS;

    ntv42_info("ntv42dev%d: message info name   %s", device->index, device->config.name);
    ntv42_info("ntv42dev%d: message info desc   %s", device->index, device->config.desc);
    ntv42_info("ntv42dev%d: message info serial %s", device->index, device->config.serial);
    
    return NTV42_RETURN_SUCCESS;
}

static const char* ntv42device_state_str(ntv42device_state_t state)
{
    if (state >= sizeof(ntv42_device_state_string))
        return ntv42_device_state_string[ntv42device_state_unknown];

    return ntv42_device_state_string[state];
}
