/* SPDX-License-Identifier: MIT */
/**
    @file		ntv2link.cpp
    @brief		Simple C interface to NTV2
    @copyright	(C) 2012-2025 AJA Video Systems, Inc.
    @note		This file is included in driver builds. It must not contain any c++.
**/

#include "ntv2link.h"
#include "ntv2card.h"

extern "C" bool ntv2card_open(int index, void** card)
{
    CNTV2Card* pCard = new CNTV2Card(index);
    if (!pCard->IsOpen())
    {
        delete pCard;
        return false;
    }

    *card = (void*)pCard;
    return true;
}

extern "C" void ntv2card_close(void* card)
{
    CNTV2Card* pCard = (CNTV2Card*)card;
    if (pCard != NULL)
    {
        delete pCard;
    }
}

bool ntv2card_get_display_name(void* card, char* display, int size)
{
    if ((card == NULL) || (display == NULL) || (size == 0))
        return false;
    CNTV2Card* pCard = (CNTV2Card*)card;
    std::string str = pCard->GetModelName();
    strncpy(display, str.c_str(), size);
    return true;
}

bool ntv2card_get_description(void* card, char* description, int size)
{
    if ((card == NULL) || (description == NULL) || (size == 0))
        return false;
    CNTV2Card* pCard = (CNTV2Card*)card;
    std::string str = pCard->GetDisplayName();
    strncpy(description, str.c_str(), size);
    return true;
}

bool ntv2card_get_serial_number(void* card, char* serial, int size)
{
    if ((card == NULL) || (serial == NULL) || (size == 0))
        return false;
    CNTV2Card* pCard = (CNTV2Card*)card;
    std::string str;
    if (!pCard->GetSerialNumberString(str))
        str = "Unknown";
    strncpy(serial, str.c_str(), size);
    return true;
}

bool ntv2card_register_read(void* card, unsigned int reg, unsigned int* data)
{
    if ((card == NULL) || (data == NULL))
        return false;
    CNTV2Card* pCard = (CNTV2Card*)card;
    return pCard->ReadRegister(reg, *data);
}

bool ntv2card_register_write(void* card, unsigned int reg, unsigned int data)
{
    if (card == NULL)
        return false;
    CNTV2Card* pCard = (CNTV2Card*)card;
    return pCard->WriteRegister(reg, data);
}



