/**
 * @file      message.h
 * @brief     NTV42 driver message interface
 * @copyright AJA Video.  All rights reserved.
 * 
 * See LICENSE file in the project root folder for license information
 */

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#define NTV42_MESSAGE_VERSION   1
#define NTV42_MESSAGE_TAG       0x6e657642
#define NTV42_MESSAGE_INIT(MSG, TYPE, FLAGS) \
    do { \
        (MSG)->header.tag = NTV42_MESSAGE_TAG; \
        (MSG)->header.version = NTV42_MESSAGE_VERSION; \
        (MSG)->header.type = (TYPE); \
        (MSG)->header.size = sizeof(MSG); \
        (MSG)->header.flags = (FLAGS);    \
        (MSG)->header.status = 0; \
        (MSG)->trailer.size = sizeof(MSG); \
    } while (0);
#define NTV42_HEADER_VERIFY(MSG) \
    (((MSG)->tag == NTV42_MESSAGE_TAG) &&    \
     ((MSG)->version == NTV42_MESSAGE_VERSION))
#define NTV42_MESSAGE_VERIFY(MSG, TYPE) \
    (((MSG)->header.tag == NTV42_MESSAGE_TAG) &&    \
     ((MSG)->header.version == NTV42_MESSAGE_VERSION) && \
     ((MSG)->header.type == (TYPE)) && \
     ((MSG)->header.size == sizeof(MSG)) &&      \
     ((MSG)->trailer.size == sizeof(MSG)))
    
/** generic message header */
#define NTV42_MESSAGE_FLAG_RW       0x00000001
    
typedef struct ntv42_message_header_t {
    uint32_t        tag;        // ntv42 message tag
    uint32_t        version;    // ntv42 message version
    uint32_t        type;       // message type
    uint32_t        size;       // message size
    uint32_t        flags;      // message action flags
    uint32_t        status;     // message return status
} ntv42_message_header_t;

/** generic message trailer */
typedef struct ntv42_message_trailer_t {
    uint32_t        size;       // message size
} ntv42_message_trailer_t;

#define NTV42_MESSAGE_SUCCESS           0    
#define NTV42_MESSAGE_FAIL              1

/** device info message */
#define NTV42_DEVICE_INFO_TYPE          1
#define NTV42_DEVICE_INFO_INIT(MSG) NTV42_MESSAGE_INIT((MSG), (NTV42_DEVICE_INFO_TYPE), NTV42_MESSAGE_FLAG_RW)
#define NTV42_DEVICE_INFO_VERIFY(MSG) NTV42_MESSAGE_VERIFY((MSG), (NTV42_DEVICE_INFO_TYPE))
    
#define NTV42_DEVICE_INFO_NAME_MAX      16
#define NTV42_DEVICE_INFO_DESC_MAX      32

typedef struct ntv42_message_device_info_t {
    ntv42_message_header_t   header;
    char        name[NTV42_DEVICE_INFO_NAME_MAX];        // Message oriented device name
    char        desc[NTV42_DEVICE_INFO_DESC_MAX];        // A user oriented description of the device
    char        serial[NTV42_DEVICE_INFO_DESC_MAX];      // Serial number or other way to uniquely identify the device
    ntv42_message_trailer_t  trailer;
} ntv42_message_device_info_t;


/** register io message */
#define NTV42_REGIO_TYPE                2
#define NTV42_REGIO_INIT(MSG) NTV42_MESSAGE_INIT((MSG), (NTV42_REGIO_TYPE), NTV42_MESSAGE_FLAG_RW)
#define NTV42_REGIO_VERIFY(MSG) NTV42_MESSAGE_VERIFY((MSG), (NTV42_REGIO_TYPE))

#define NTV42_REGIO_FLAG_READ           0x00000001
#define NTV42_REGIO_FLAG_WRITE          0x00000002
#define NTV42_REGIO_FLAG_32BIT          0x00000100

typedef struct ntv42_message_regio_t {
    ntv42_message_header_t   header;
    uint32_t    reg;            // register byte address
    uint32_t    flags;          // control flags
    uint32_t    mask;           // data mask
    uint32_t    shift;          // data shift
    uint32_t    data;           // data
    ntv42_message_trailer_t  trailer;
} ntv42_message_regio_t;


#ifdef __cplusplus
}
#endif
