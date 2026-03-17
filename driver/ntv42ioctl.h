/* SPDX-License-Identifier: MIT */
/**
 * @file      ntv42ioctl.h
 * @brief     NTV42 ioctl definitions shared between kernel driver and userspace
 * @copyright (C) 2012-2025 AJA Video Systems, Inc.
 *
 * Included by both kernel driver and userspace. Uses NTV2_DEVICE_TYPE (0xBB)
 * with command numbers 10+ (existing NTV2 commands start at 48).
 *
 * All structs use explicit padding to avoid implicit compiler-inserted gaps
 * between differently-sized fields.
 */

#pragma once

#ifdef __KERNEL__
#include <linux/ioctl.h>
#include <linux/types.h>
#else
#include <sys/ioctl.h>
#include <stdint.h>
#endif

#define NTV42_DEVICE_TYPE  0xBB  /* Same as NTV2_DEVICE_TYPE */

/*============================================================================
 * Phase 1: Version / Register I/O (commands 10-19)
 *==========================================================================*/

/** Version and capability handshake */
typedef struct ntv42_ioctl_version_t {
    uint32_t    version;        /* Output: ioctl interface version (1, 2, ...) */
    uint32_t    capabilities;   /* Output: bitmask of supported phases/features */
} ntv42_ioctl_version_t;

/* Capability bits */
#define NTV42_CAP_REGIO     (1 << 0)  /* Phase 1: register I/O */
#define NTV42_CAP_EVENTS    (1 << 1)  /* Phase 2: events */
#define NTV42_CAP_DMA       (1 << 2)  /* Phase 3: DMA */
#define NTV42_CAP_REGBATCH  (1 << 3)  /* Phase 4: register batch */

/* Current ioctl interface version */
#define NTV42_IOCTL_VERSION 1

/** Bulk register read/write */
typedef struct ntv42_ioctl_regio_t {
    uint32_t    bar;            /* BAR index */
    uint32_t    count;          /* Number of registers */
    uint64_t    offset;         /* Byte offset within BAR */
    uint64_t    data_ptr;       /* Userspace pointer to data buffer (cast to __u64) */
} ntv42_ioctl_regio_t;

/** Device info query */
typedef struct ntv42_ioctl_device_info_t {
    char        name[16];       /* Device name (matches ntv42_device_config_t) */
    char        desc[32];       /* Device description */
    char        serial[32];     /* Serial number */
    uint32_t    bar_count;      /* Number of BARs */
    uint32_t    _pad0;
    struct {
        char        name[16];   /* BAR name */
        uint64_t    address;    /* BAR base address */
        uint64_t    size;       /* BAR size in bytes */
        uint32_t    width;      /* Access width in bytes */
        uint32_t    _pad0;
    } bar[8];
} ntv42_ioctl_device_info_t;

#define IOCTL_NTV42_VERSION      _IOR(NTV42_DEVICE_TYPE,  10, ntv42_ioctl_version_t)
#define IOCTL_NTV42_REG_READ     _IOWR(NTV42_DEVICE_TYPE, 11, ntv42_ioctl_regio_t)
#define IOCTL_NTV42_REG_WRITE    _IOW(NTV42_DEVICE_TYPE,  12, ntv42_ioctl_regio_t)
#define IOCTL_NTV42_DEVICE_INFO  _IOR(NTV42_DEVICE_TYPE,  13, ntv42_ioctl_device_info_t)

/*============================================================================
 * Phase 2: Events (commands 20-29)
 *==========================================================================*/

/** Enable/disable/query event source */
typedef struct ntv42_ioctl_event_control_t {
    uint32_t    type;           /* ntv42 event type (0x0001, 0x0002, etc.) */
    uint32_t    index;          /* Which instance (output 0, input 1, etc.) */
    uint32_t    enable;         /* 1 = enable, 0 = disable */
    uint32_t    _pad0;
    uint64_t    count;          /* Output: current event count */
} ntv42_ioctl_event_control_t;

/** Wait for a single event type */
typedef struct ntv42_ioctl_event_wait_t {
    uint32_t    type;           /* ntv42 event type */
    uint32_t    index;          /* Which instance */
    int32_t     timeout_ms;     /* Timeout: -1 = infinite, 0 = poll, >0 = ms */
    uint32_t    _pad0;
    uint64_t    count;          /* Output: event count after wait */
    uint64_t    timestamp;      /* Output: kernel timestamp (ns) */
} ntv42_ioctl_event_wait_t;

/** Query event status without waiting */
typedef struct ntv42_ioctl_event_status_t {
    uint32_t    type;           /* ntv42 event type */
    uint32_t    index;          /* Which instance */
    uint32_t    enabled;        /* Output: 1 if enabled */
    uint32_t    supported;      /* Output: 1 if supported */
    uint64_t    count;          /* Output: current event count */
} ntv42_ioctl_event_status_t;

#define IOCTL_NTV42_EVENT_CONTROL   _IOWR(NTV42_DEVICE_TYPE, 20, ntv42_ioctl_event_control_t)
#define IOCTL_NTV42_EVENT_WAIT      _IOWR(NTV42_DEVICE_TYPE, 21, ntv42_ioctl_event_wait_t)
#define IOCTL_NTV42_EVENT_STATUS    _IOWR(NTV42_DEVICE_TYPE, 22, ntv42_ioctl_event_status_t)

/*============================================================================
 * Phase 3: DMA (commands 30-39)
 *==========================================================================*/

/** DMA transfer request (synchronous) */
typedef struct ntv42_ioctl_dma_transfer_t {
    uint32_t    engine;         /* DMA engine index */
    uint32_t    direction;      /* 1=to_device, 2=from_device */
    uint64_t    host_addr;      /* Userspace buffer address */
    uint64_t    device_addr;    /* Device memory byte offset */
    uint64_t    bytes;          /* Transfer size in bytes */
    int32_t     timeout_ms;     /* Timeout: -1 = infinite, >0 = ms */
    uint32_t    flags;          /* Reserved, set to 0 */
    /* Output */
    uint64_t    bytes_xfered;   /* Actual bytes transferred */
    int32_t     status;         /* 0 = success, negative = error */
    uint32_t    _pad0;
} ntv42_ioctl_dma_transfer_t;

/** DMA engine info query */
typedef struct ntv42_ioctl_dma_info_t {
    uint32_t    engine_count;   /* Output: number of DMA engines */
    uint32_t    _pad0;
    struct {
        uint32_t    directions;     /* Supported directions bitmask */
        uint32_t    host_align;     /* Host address alignment */
        uint32_t    device_align;   /* Device address alignment */
        uint32_t    xfer_align;     /* Transfer size alignment */
        uint64_t    max_xfer;       /* Max transfer size */
    } engines[4];
} ntv42_ioctl_dma_info_t;

#define IOCTL_NTV42_DMA_TRANSFER  _IOWR(NTV42_DEVICE_TYPE, 30, ntv42_ioctl_dma_transfer_t)
#define IOCTL_NTV42_DMA_INFO      _IOR(NTV42_DEVICE_TYPE,  31, ntv42_ioctl_dma_info_t)

/*============================================================================
 * Phase 4: Register Sequencing (commands 40-47)
 *==========================================================================*/

/** Batch flags */
#define NTV42_REGBATCH_ONESHOT      (1 << 0)  /* Execute once, then auto-cancel */
#define NTV42_REGBATCH_RECURRING    (1 << 1)  /* Re-arm after each execution */
#define NTV42_REGBATCH_READBACK     (1 << 2)  /* Read registers after write */

/** Maximum entries per batch */
#define NTV42_REGBATCH_MAX_ENTRIES  256

/** Single register write within a batch */
typedef struct ntv42_ioctl_regbatch_entry_t {
    uint32_t    bar;            /* BAR index */
    uint32_t    offset;         /* Byte offset within BAR */
    uint32_t    value;          /* Value to write */
    uint32_t    mask;           /* Bit mask (0xFFFFFFFF for full write) */
} ntv42_ioctl_regbatch_entry_t;

/** Batch submission */
typedef struct ntv42_ioctl_regbatch_submit_t {
    uint32_t    trigger_event;  /* Event type that triggers execution */
    uint32_t    trigger_index;  /* Event index (e.g., output 0) */
    uint32_t    flags;          /* NTV42_REGBATCH_* flags */
    uint32_t    entry_count;    /* Number of entries */
    uint64_t    entries_ptr;    /* Pointer to ntv42_ioctl_regbatch_entry_t array */
    /* Output */
    uint32_t    batch_id;       /* Assigned batch ID */
    uint32_t    _pad0;
} ntv42_ioctl_regbatch_submit_t;

/** Batch cancel */
typedef struct ntv42_ioctl_regbatch_cancel_t {
    uint32_t    batch_id;       /* Batch to cancel (0 = cancel all) */
    uint32_t    _pad0;
} ntv42_ioctl_regbatch_cancel_t;

/** Batch status query */
typedef struct ntv42_ioctl_regbatch_status_t {
    uint32_t    batch_id;       /* Batch to query */
    uint32_t    state;          /* Output: 0=pending, 1=executed, 2=cancelled, 3=error */
    uint32_t    exec_count;     /* Output: number of times executed */
    uint32_t    _pad0;
    uint64_t    last_exec_time; /* Output: timestamp of last execution (ns) */
} ntv42_ioctl_regbatch_status_t;

#define IOCTL_NTV42_REGBATCH_SUBMIT  _IOWR(NTV42_DEVICE_TYPE, 40, ntv42_ioctl_regbatch_submit_t)
#define IOCTL_NTV42_REGBATCH_CANCEL  _IOW(NTV42_DEVICE_TYPE,  41, ntv42_ioctl_regbatch_cancel_t)
#define IOCTL_NTV42_REGBATCH_STATUS  _IOWR(NTV42_DEVICE_TYPE, 42, ntv42_ioctl_regbatch_status_t)
