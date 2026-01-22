/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2link.h
    @brief		Simple C interface for NTV2
    @copyright	(C) 2004-2025 AJA Video Systems, Inc.
**/

#ifndef NTV2LINK_H
#define NTV2LINK_H

#ifdef __cplusplus
extern "C"
{
#endif

    bool ntv2card_open(int index, void** card);
    void ntv2card_close(void* card);

    bool ntv2card_get_display_name(void* card, char* display, int size);
    bool ntv2card_get_description(void* card, char* description, int size);
    bool ntv2card_get_serial_number(void* card, char* serial, int size);
    bool ntv2card_register_read(void* card, unsigned int reg, unsigned int* data);
    bool ntv2card_register_write(void* card, unsigned int reg, unsigned int data);
        
#ifdef __cplusplus
}
#endif

#endif
