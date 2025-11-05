/* SPDX-License-Identifier: MIT */
/**
    @file		ntv42device.cpp
    @brief		NTV42 device
    @copyright	(C) 2012-2025 AJA Video Systems, Inc.
    @note		This file is included in driver builds. It must not contain any c++.
**/

#include <linux/kernel.h>
#include <linux/string.h>

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

int ntv42device_event(ntv42_device_t* device, uint32_t index)
{
    if ((device == NULL) || (device->state == ntv42device_state_unknown))
        return NTV42_RETURN_BAD_PARAMETER;

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
