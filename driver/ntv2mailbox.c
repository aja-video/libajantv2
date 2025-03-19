/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2025 AJA Video Systems, Inc.
 */
//==========================================================================
//
//  ntv2mailbox.c
//
//==========================================================================

#include "ntv2mailbox.h"
#include "ntv2commonreg.h"

// debug messages
#define NTV2_DEBUG_INFO					0x00000001
#define NTV2_DEBUG_ERROR				0x00000002
#define NTV2_DEBUG_MAILBOX_SEND 		0x00000004
#define NTV2_DEBUG_MAILBOX_RECV 		0x00000008

#define NTV2_DEBUG_ACTIVE(msg_mask) \
	(((ntv2_debug_mask | ntv2_user_mask) & msg_mask) != 0)

#define NTV2_MSG_PRINT(msg_mask, string, ...) \
	if(NTV2_DEBUG_ACTIVE(msg_mask)) ntv2Message(string, __VA_ARGS__);

#define NTV2_MSG_MAILBOX_INFO(string, ...)			NTV2_MSG_PRINT(NTV2_DEBUG_INFO, string, __VA_ARGS__)
#define NTV2_MSG_MAILBOX_ERROR(string, ...)			NTV2_MSG_PRINT(NTV2_DEBUG_ERROR, string, __VA_ARGS__)
#define NTV2_MSG_MAILBOX_SEND(string, ...)			NTV2_MSG_PRINT(NTV2_DEBUG_MAILBOX_SEND, string, __VA_ARGS__)
#define NTV2_MSG_MAILBOX_RECV(string, ...)	    	NTV2_MSG_PRINT(NTV2_DEBUG_MAILBOX_RECV, string, __VA_ARGS__)

#define FIFO_SIZE 8192 // 8192 bytes (2048 32-bit words) per RX/TX FIFO

#define MB_WRDATA       0x00 // Wr data register
#define MB_RDDATA       0x08 // Rd data register
#define MB_STATUS       0x10 // Status register
#define MB_ERROR        0x14 // Error register
#define MB_SIT          0x18 // send interrupt threshold
#define MB_RIT          0x1C // receive interrupt threshold
#define MB_IS           0x20 // interrupt status register
#define MB_IE           0x24 // interrupt enable register
#define MB_IP           0x28 // interrupt pending register
#define MB_CTRL         0x2C // Control register, write only.

#define MBS_RX_EMPTY    0x01 // FIFO status -- RX EMPTY
#define MBS_TX_FULL     0x02 // FIFO status -- TX FULL
#define MBS_SEND_LT_TA  0x04 // send level <= SIT
#define MBS_RCV_GT_TA   0x08 // receive level > RIT

#define MBE_RX_EMPTY    0x01
#define MBE_TX_FULL     0x02

#define MB_RX_INT       0x02

#define FIFO_TX_CLEAR   0x01
#define FIFO_RX_CLEAR   0x02
#define FIFOCLEAR       0x03


static uint32_t ntv2_debug_mask = NTV2_DEBUG_INFO | NTV2_DEBUG_ERROR | NTV2_DEBUG_MAILBOX_SEND | NTV2_DEBUG_MAILBOX_RECV;
static uint32_t ntv2_user_mask = NTV2_DEBUG_INFO | NTV2_DEBUG_ERROR;

static uint32_t ntv2_magic = 0xE925A61C;

struct ntv2_mailbox *ntv2_mailbox_open(Ntv2SystemContext* sys_con,
									   const char *name, int index)
{
	struct ntv2_mailbox *ntv2_mail = NULL;

	if ((sys_con == NULL) ||
		(name == NULL))
		return NULL;

	ntv2_mail = (struct ntv2_mailbox *)ntv2MemoryAlloc(sizeof(struct ntv2_mailbox));
	if (ntv2_mail == NULL) {
		NTV2_MSG_MAILBOX_ERROR("%s: ntv2_mailbox instance memory allocation failed\n", name);
		return NULL;
	}
	memset(ntv2_mail, 0, sizeof(struct ntv2_mailbox));

	ntv2_mail->index = index;
#if defined(MSWindows)
	sprintf(ntv2_mail->name, "%s%d", name, index);
#else
	snprintf(ntv2_mail->name, NTV2_MAILBOX_STRING_SIZE, "%s%d", name, index);
#endif
	ntv2_mail->system_context = sys_con;

	NTV2_MSG_MAILBOX_INFO("%s: open ntv2_mailbox\n", ntv2_mail->name);

	return ntv2_mail;
}

void ntv2_mailbox_close(struct ntv2_mailbox *ntv2_mail)
{
	if (ntv2_mail == NULL) 
		return;

	NTV2_MSG_MAILBOX_INFO("%s: close ntv2_mailbox\n", ntv2_mail->name);

	memset(ntv2_mail, 0, sizeof(struct ntv2_mailbox));
	ntv2MemoryFree(ntv2_mail, sizeof(struct ntv2_mailbox));
}

Ntv2Status ntv2_mailbox_configure(struct ntv2_mailbox* ntv2_mail,
                                  uint32_t reg_base)
{
	if (ntv2_mail == NULL)
		return NTV2_STATUS_BAD_PARAMETER;

	NTV2_MSG_MAILBOX_INFO("%s: configure ntv2_mailbox\n", ntv2_mail->name);

    ntv2_mail->reg_base = reg_base;

	return NTV2_STATUS_SUCCESS;
}

Ntv2Status ntv2_mailbox_send(struct ntv2_mailbox* ntv2_mail,
                             uint8_t* buffer, uint32_t size, uint32_t* offset)
{
    uint32_t off = *offset;
    uint32_t value = 0;
    uint32_t ts = 0;

    while (off < size)
    {
        value = ntv2_regnum_read(ntv2_mail->system_context, ntv2_mail->reg_base + MB_STATUS);
        if ((value & MBS_TX_FULL) != 0)
            break;
        ts = size - off; 
        if (ts > 4)
            ts = 4;
        value = 0;
        memcpy(&value, (buffer + off), ts);
        ntv2_regnum_write(ntv2_mail->system_context, ntv2_mail->reg_base + MB_WRDATA, value);
        off += ts;
    }
    value = ntv2_regnum_read(ntv2_mail->system_context, ntv2_mail->reg_base + MB_ERROR);
    if (value != 0)
    {
        NTV2_MSG_MAILBOX_ERROR("%s: send error %08x\n", ntv2_mail->name, value);
        return NTV2_STATUS_FAIL;
    }
    
    *offset = off;
    return NTV2_STATUS_SUCCESS;
}

Ntv2Status ntv2_mailbox_recv(struct ntv2_mailbox* ntv2_mail,
                             uint8_t* buffer, uint32_t size, uint32_t* offset)
{
    uint32_t off = *offset;
    uint32_t value = 0;
    uint32_t ts = 0;

    while (off < size)
    {
        value = ntv2_regnum_read(ntv2_mail->system_context, ntv2_mail->reg_base + MB_STATUS);
        if ((value & MBS_RX_EMPTY) != 0)
            break;
        value = ntv2_regnum_read(ntv2_mail->system_context, ntv2_mail->reg_base + MB_RDDATA);
        ts = size - off; 
        if (ts > 4)
            ts = 4;
        memcpy((buffer + off), &value, ts);
        off += ts;
    }
    value = ntv2_regnum_read(ntv2_mail->system_context, ntv2_mail->reg_base + MB_ERROR);
    if (value != 0)
    {
        NTV2_MSG_MAILBOX_ERROR("%s: recv error %08x\n", ntv2_mail->name, value);
        return NTV2_STATUS_FAIL;
    }
    
    *offset = off;
    return NTV2_STATUS_SUCCESS;
}

Ntv2Status ntv2_packet_send(struct ntv2_mailbox *ntv2_mail,
                            uint8_t* buffer, uint32_t size, uint32_t* offset,
                            uint32_t delay, uint32_t timeout)
{
    int64_t start = ntv2TimeCounter();
    int64_t check = 0;
    uint32_t off = *offset;
    Ntv2Status status = NTV2_STATUS_SUCCESS;
    uint32_t head_off = 0;
    bool head_magic = false;
    bool head_size = false;

    while (off < size)
    {
        if (!head_magic)
        {
            // write magic header number
            head_off = 0;
            status = ntv2_mailbox_send(ntv2_mail, (uint8_t*)&ntv2_magic, 4, &head_off);
            if (status != NTV2_STATUS_SUCCESS)
            {
                NTV2_MSG_MAILBOX_ERROR("%s: send magic word failed\n", ntv2_mail->name);
                return status;
            }
            if (head_off >= 4)
            {
                head_magic = true;
            }
        }
        else if (!head_size)
        {
            // write packet size
            head_off = 0;
            status = ntv2_mailbox_send(ntv2_mail, (uint8_t*)&size, 4, &head_off);
            if (status != NTV2_STATUS_SUCCESS)
            {
                NTV2_MSG_MAILBOX_ERROR("%s: send packet size %d failed\n", ntv2_mail->name, (int)size);
                return status;
            }
            if (head_off >= 4)
            {
                head_size = true;
            }
        }
        else
        {
            // write packet
            status = ntv2_mailbox_send(ntv2_mail, buffer, size, &off);
            if (status != NTV2_STATUS_SUCCESS)
            {
                NTV2_MSG_MAILBOX_ERROR("%s: send packet data failed at offset %d\n",
                                       ntv2_mail->name, (int)off);
                *offset = off;
                return status;
            }
            if (off >= size)
                break;
        }

        // check for timeout
        check = (ntv2TimeCounter() - start) * 1000000 / ntv2TimeFrequency();
        if (check > timeout)
        {
            NTV2_MSG_MAILBOX_ERROR("%s: send timeout at offset %d\n",
                                   ntv2_mail->name, (int)off);
            *offset = off;
            return NTV2_STATUS_TIMEOUT;
        }

        // sleep if not done
        ntv2TimeSleep(delay);
    }

    return NTV2_STATUS_SUCCESS;
}

Ntv2Status ntv2_packet_recv(struct ntv2_mailbox *ntv2_mail,
                            uint8_t* buffer, uint32_t size, uint32_t* offset,
                            uint32_t delay, uint32_t timeout)
{
    int64_t start = ntv2TimeCounter();
    int64_t check = 0;
    uint32_t off = *offset;
    Ntv2Status status = NTV2_STATUS_SUCCESS;
    uint32_t head_off = 0;
    bool head_magic = false;
    bool head_size = false;
    uint32_t data_magic = 0;
    uint32_t data_size = 0;

   while (off < size)
   {
       if (!head_magic)
       {
           // read magic header number
           head_off = 0;
           status = ntv2_mailbox_recv(ntv2_mail, (uint8_t*)&data_magic, 4, &head_off);
           if (status != NTV2_STATUS_SUCCESS)
           {
               NTV2_MSG_MAILBOX_ERROR("%s: receive magic word failed\n", ntv2_mail->name);
               return status;
           }
           if (head_off >= 4)
           {
               if (data_magic != ntv2_magic)
               {
                   NTV2_MSG_MAILBOX_ERROR("%s: receive magic word %08x not %08x\n",
                                          ntv2_mail->name, data_magic, ntv2_magic);
                   return NTV2_STATUS_FAIL;
               }
               head_magic = true;
           }
       }
       else if (!head_size)
       {
           // read packet size
           head_off = 0;
           status = ntv2_mailbox_recv(ntv2_mail, (uint8_t*)&data_size, 4, &head_off);
           if (status != NTV2_STATUS_SUCCESS)
           {
               NTV2_MSG_MAILBOX_ERROR("%s: receive packet size failed\n", ntv2_mail->name);
               return status;
           }
           if (head_off >= 4)
           {
               if ((data_size == 0) || (data_size > size))
               {
                   NTV2_MSG_MAILBOX_ERROR("%s: receive bad packet size %d  buffer size %d\n",
                                          ntv2_mail->name, data_size, (int)size);
                   return NTV2_STATUS_FAIL;
               }
               head_size = true;
           }
       }
       else
       {
           // read packet
           status = ntv2_mailbox_recv(ntv2_mail, buffer, data_size, &off);
           if (status != NTV2_STATUS_SUCCESS)
           {
               NTV2_MSG_MAILBOX_ERROR("%s: receive packet data failed at offset %d\n",
                                      ntv2_mail->name, (int)off);
               *offset = off;
               return status;
           }
           if (off >= data_size)
               break;
       }

       // check for timeout
       check = (ntv2TimeCounter() - start) * 1000000 / ntv2TimeFrequency();
       if (check > timeout)
       {
           NTV2_MSG_MAILBOX_ERROR("%s: receive timeout at offset %d\n",
                                  ntv2_mail->name, (int)off);
           *offset = off;
           return NTV2_STATUS_TIMEOUT;
       }

       // sleep if not done
       ntv2TimeSleep(delay);
   }

   return NTV2_STATUS_SUCCESS;
}



