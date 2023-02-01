/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
///////////////////////////////////////////////////////////////
//
// HEVC Linux Device Driver for AJA HEVC devices.
//
////////////////////////////////////////////////////////////
//
// Filename: hevcparams.h
// Purpose:	 HEVC module and device parameters
// Notes:	 
//
///////////////////////////////////////////////////////////////

#ifndef HEVCPARAMS_H
#define HEVCPARAMS_H

#include "hevccommon.h"

#define HEVC_DRIVER_MAJOR		1
#define HEVC_DRIVER_MINOR		1
#define HEVC_DRIVER_POINT		0
#define HEVC_DRIVER_BUILD		0

#define HEVC_MCPU_MAJOR			3
#define HEVC_MCPU_MINOR			0
#define HEVC_MCPU_POINT			0
#define HEVC_MCPU_BUILD			9888

#define HEVC_SYSTEM_FIRMWARE			"010202.01.00.364"
#define HEVC_ENCODER_FIRMWARE_SINGLE	"20160704_094422_dee4284"
#define HEVC_ENCODER_FIRMWARE_MULTIPLE	"20160704_094422_dee4284"

#define HEVC_PCI_VENDOR 		0x10cf
#define HEVC_PCI_DEVICE			0x2049
#define HEVC_PCI_SUBVENDOR 		0xf1d0
#define HEVC_PCI_SUBDEVICE		0xeb15

// driver debug output convenience macros
#define HEVC_MSG_INFO(string, ...) \
	if((hevcGetModuleParams()->debugMask & HEVC_DEBUG_MASK_INFO) != 0) ntv2Message(string, __VA_ARGS__);
#define HEVC_MSG_WARNING(string, ...) \
	if((hevcGetModuleParams()->debugMask & HEVC_DEBUG_MASK_WARNING) != 0) ntv2Message(string, __VA_ARGS__);
#define HEVC_MSG_ERROR(string, ...) \
	if((hevcGetModuleParams()->debugMask & HEVC_DEBUG_MASK_ERROR) != 0) ntv2Message(string , __VA_ARGS__);
#define HEVC_MSG_INT_PRIMARY(string, ...) \
	if((hevcGetModuleParams()->debugMask & HEVC_DEBUG_MASK_INT_PRIMARY) != 0) ntv2Message(string , __VA_ARGS__);
#define HEVC_MSG_INT_COMMAND(string, ...) \
	if((hevcGetModuleParams()->debugMask & HEVC_DEBUG_MASK_INT_COMMAND) != 0) ntv2Message(string , __VA_ARGS__);
#define HEVC_MSG_INT_VEI(string, ...) \
	if((hevcGetModuleParams()->debugMask & HEVC_DEBUG_MASK_INT_VEI) != 0) ntv2Message(string , __VA_ARGS__);
#define HEVC_MSG_INT_SEO(string, ...) \
	if((hevcGetModuleParams()->debugMask & HEVC_DEBUG_MASK_INT_SEO) != 0) ntv2Message(string , __VA_ARGS__);
#define HEVC_MSG_INT_ERROR(string, ...) \
	if((hevcGetModuleParams()->debugMask & HEVC_DEBUG_MASK_INT_ERROR) != 0) ntv2Message(string , __VA_ARGS__);
#define HEVC_MSG_REGISTER_INFO(string, ...) \
	if((hevcGetModuleParams()->debugMask & HEVC_DEBUG_MASK_REGISTER_INFO) != 0) ntv2Message(string , __VA_ARGS__);
#define HEVC_MSG_REGISTER_STATE(string, ...) \
	if((hevcGetModuleParams()->debugMask & HEVC_DEBUG_MASK_REGISTER_STATE) != 0) ntv2Message(string , __VA_ARGS__);
#define HEVC_MSG_REGISTER_ERROR(string, ...) \
	if((hevcGetModuleParams()->debugMask & HEVC_DEBUG_MASK_REGISTER_ERROR) != 0) ntv2Message(string , __VA_ARGS__);
#define HEVC_MSG_COMMAND_INFO(string, ...) \
	if((hevcGetModuleParams()->debugMask & HEVC_DEBUG_MASK_COMMAND_INFO) != 0) ntv2Message(string , __VA_ARGS__);
#define HEVC_MSG_COMMAND_STATE(string, ...) \
	if((hevcGetModuleParams()->debugMask & HEVC_DEBUG_MASK_COMMAND_STATE) != 0) ntv2Message(string , __VA_ARGS__);
#define HEVC_MSG_COMMAND_ERROR(string, ...) \
	if((hevcGetModuleParams()->debugMask & HEVC_DEBUG_MASK_COMMAND_ERROR) != 0) ntv2Message(string , __VA_ARGS__);
#define HEVC_MSG_STREAM_INFO(string, ...) \
	if((hevcGetModuleParams()->debugMask & HEVC_DEBUG_MASK_STREAM_INFO) != 0) ntv2Message(string , __VA_ARGS__);
#define HEVC_MSG_STREAM_STATE(string, ...) \
	if((hevcGetModuleParams()->debugMask & HEVC_DEBUG_MASK_STREAM_STATE) != 0) ntv2Message(string , __VA_ARGS__);
#define HEVC_MSG_STREAM_COPY(string, ...) \
	if((hevcGetModuleParams()->debugMask & HEVC_DEBUG_MASK_STREAM_COPY) != 0) ntv2Message(string , __VA_ARGS__);
#define HEVC_MSG_STREAM_SEGMENT(string, ...) \
	if((hevcGetModuleParams()->debugMask & HEVC_DEBUG_MASK_STREAM_SEGMENT) != 0) ntv2Message(string , __VA_ARGS__);
#define HEVC_MSG_STREAM_FRAME(string, ...) \
	if((hevcGetModuleParams()->debugMask & HEVC_DEBUG_MASK_STREAM_FRAME) != 0) ntv2Message(string , __VA_ARGS__);
#define HEVC_MSG_STREAM_ERROR(string, ...) \
	if((hevcGetModuleParams()->debugMask & HEVC_DEBUG_MASK_STREAM_ERROR) != 0) ntv2Message(string , __VA_ARGS__);
#define HEVC_MSG_MEMORY_ALLOC(string, ...) \
	if((hevcGetModuleParams()->debugMask & HEVC_DEBUG_MASK_MEMORY_ALLOC) != 0) ntv2Message(string , __VA_ARGS__);
#define HEVC_MSG_MEMORY_ERROR(string, ...) \
	if((hevcGetModuleParams()->debugMask & HEVC_DEBUG_MASK_MEMORY_ERROR) != 0) ntv2Message(string , __VA_ARGS__);
#define HEVC_MSG_DMA_INFO(string, ...) \
	if((hevcGetModuleParams()->debugMask & HEVC_DEBUG_MASK_DMA_INFO) != 0) ntv2Message(string , __VA_ARGS__);
#define HEVC_MSG_DMA_DESCRIPTOR(string, ...) \
	if((hevcGetModuleParams()->debugMask & HEVC_DEBUG_MASK_DMA_DESCRIPTOR) != 0) ntv2Message(string , __VA_ARGS__);
#define HEVC_MSG_DMA_ERROR(string, ...) \
	if((hevcGetModuleParams()->debugMask & HEVC_DEBUG_MASK_DMA_ERROR) != 0) ntv2Message(string , __VA_ARGS__);
#define HEVC_MSG_STATUS_INFO(string, ...) \
	if((hevcGetModuleParams()->debugMask & HEVC_DEBUG_MASK_STATUS_INFO) != 0) ntv2Message(string , __VA_ARGS__);

#define HEVC_DEVICE_MAX 				8		// max devices
#define HEVC_COMMAND_TASK_MAX			16		// command task queue size
#define HEVC_STREAM_TASK_MAX			32		// stream task queue size
#define HEVC_STREAM_DESCRIPTOR_MAX		512		// max stream dma desctiptors
#define HEVC_STREAM_BUFFER_MAX			2		// max bounce buffers for each stream
#define HEVC_STREAM_VEI_BUFFERS			2		// number of vei bounce buffers for each stream
#define HEVC_STREAM_SEO_BUFFERS			1		// number of seo bounce buffers for each stream
#define HEVC_FRAME_DATA_MAX				128		// frame data queue size

// driver module parameters
typedef struct hevc_module_params
{
	const char*			pModuleName;			// module name
	HevcVersion			driverVersion;			// module version
	uint32_t			debugMask;				// debug output mask bits
} HevcModuleParams;

// codec command information
typedef struct hevc_cmd_info 
{
	uint32_t target;							// command component target
	uint32_t id;								// command type
	uint32_t param[FMB_COMMAND_PARAM_MAX];		// command dependent parameters
} HevcCommandInfo;

// codec command acknowledge information
typedef struct hevc_cmd_ack_info 
{
	uint32_t 	cntCount;						// command continuity count		
	uint32_t 	target;							// command component target
	uint32_t 	id;								// command type
	uint32_t 	result0;						// command dependent result
} HevcCommandAckInfo;

// codec command message information
typedef struct hevc_cmd_msg_info 
{
	uint32_t 	cntCount;						// command continuity count
	uint32_t 	target;							// command component target
	uint32_t 	id;								// command type
	uint32_t 	result[FMB_REG_COMMAND_RESULT_PARAM_MAX];	// command dependent results
} HevcCommandMsgInfo;

// command queue task entry
typedef struct hevc_cmd_task
{
	uint32_t			taskNum;				// task number
	Ntv2Event 			ackEvent;				// ack wait event
	Ntv2Event 			msgEvent;				// msg wait event

	bool 				cmdReady;				// command ready to send to codec
	bool 				cmdSent;				// command has been sent to codec				
	bool 				ackReceived;			// command has been acknowledged
	bool 				msgReceived;			// command message has been received
	uint32_t 			cntCount;				// command continuity count

	HevcCommandInfo		cmdInfo;				// task command information
	HevcCommandAckInfo	ackInfo;				// task acknowledge information
	HevcCommandMsgInfo	msgInfo;				// task message information
} HevcCommandTask;

// dma stream buffer (bounce)
typedef struct hevc_stream_buffer
{
	Ntv2DmaMemory		dmaMemory;				// dma memory object
	uint8_t*			pBufferAddress;			// buffer virtual address
	Ntv2DmaAddress		dmaAddress;				// buffer logical address
	uint32_t			bufferSize;				// buffer size
} HevcStreamBuffer;

// dma stream buffer group
typedef struct hevc_stream_buffer_group
{
	uint32_t 				streamType;			// stream type
	uint32_t 				streamId;			// buffer stream id
	uint32_t				bufNum;				// buffer number
	uint32_t				videoDataSize;		// size of video data
	uint32_t				videoBufferSize;	// size of buffer
	HevcStreamBuffer		videoBuffers[HEVC_STREAM_DESCRIPTOR_MAX];	// array of video data buffers
	uint32_t				infoDataSize;		// size of stream info data
	uint32_t				infoBufferSize;		// size of stream info buffer
	HevcStreamBuffer		infoBuffer;			// the info data buffer
	bool					active;				// buffer is in use
} HevcStreamBufferGroup;

// codec stream info
typedef struct hevc_stream_info 
{
	uint32_t 				streamType;			// stream type
	uint32_t 				streamId;			// stream id
	HevcStreamBufferGroup*	pBufferGroup;		// stream buffer
	bool					isLastFrame;		// this is last frame
} HevcStreamInfo;

// codec stream acknowledge
typedef struct hevc_stream_ack_info 
{
	uint32_t 	cntCount;						// stream continuity count						
	uint32_t 	streamType;						// stream type
	uint32_t 	streamId;						// stream id
	uint32_t 	result0;						// stream result
} HevcStreamAckInfo;

typedef struct hevc_stream_msg_info 
{
	uint32_t 	cntCount;						// stream continuity count
	uint32_t 	streamType;						// stream type
	uint32_t 	streamId;						// stream id
	uint32_t	videoDataSize;					// video data transfer size
	uint32_t	infoDataSize;					// info data transfer size
	uint32_t 	result0;						// stream result
	uint32_t	lastFrame;						// this is last frame
} HevcStreamMsgInfo;

// stream queue task entry
typedef struct hevc_stream_task
{
	uint32_t			taskNum;				// task number
	Ntv2Event		 	ackEvent;				// ack wait event
	Ntv2Event 			msgEvent;				// msg wait event

	bool 				apiReady;				// api ready to send to codec				
	bool 				dmaSent;				// dma sent to codec
	bool 				ackReceived;			// dma has been acknowledged
	bool 				msgReceived;			// dma is complete
	bool				apiDone;				// api is complete
	uint32_t 			cntCount;				// dma continuity count

	HevcStreamInfo		strInfo;				// dma stream information
	HevcStreamAckInfo	ackInfo;				// dma acknowledge information
	HevcStreamMsgInfo	msgInfo;				// dma message information

	int64_t				seqNum;					// sequence number
	int64_t				enqueueTime;			// stream enqueue time
	int64_t				sendTime;				// dma send time
	int64_t				ackTime;				// dma acknowledge time
	int64_t				msgTime;				// dma complete time
	int64_t				dequeueTime;			// stream dequeue time
} HevcStreamTask;

// codec message information
typedef struct hevc_codec_msg_info 
{
	uint32_t 	target;							// codec component target
	uint32_t 	id;								// codec message identifier
	uint32_t 	params[FMB_REG_MESSAGE_RESULT_PARAM_MAX];
} HevcCodecMsgInfo;

typedef struct hevc_frame_data
{
	uint32_t 	streamId;						// stream id
	uint32_t	syncCount;						// vsync count
	uint32_t	itcValueLow;					// internal time clock (90 kHz)
	uint32_t	itcValueHigh;
	uint32_t	itcExtension;					// internal time extension (27 MHz)
	int64_t		encodeTime;						// host system clock (10 MHz)
} HevcFrameData;

// driver device parameters
typedef struct hevc_device_params
{
	const char* 			pDeviceName;		// device name
	uint32_t 				devNum;				// device number
	Ntv2SystemContext		systemContext;		// os device specific data
	HevcPciId				pciId;				// pci id
	uint32_t				ntv2DevNum;			// associated ntv2 device number
	Ntv2Register			bar0Base;			// memory bar 0 base address
	Ntv2Register			bar2Base;			// memory bar 2 base address
	Ntv2Register			bar4Base;			// memory bar 4 base address
	Ntv2Register			bar5Base;			// memory bar 5 base address
	uint32_t				bar0Size;			// memory bar 0 size (bytes)
	uint32_t				bar2Size;			// memory bar 2 size (bytes)
	uint32_t				bar4Size;			// memory bar 4 size (bytes)
	uint32_t				bar5Size;			// memory bar 5 size (bytes)

	HevcDeviceMode			deviceMode;			// hardware device mode
	HevcEncodeMode			encodeMode;			// cached codec encode mode
	HevcFirmwareType		firmwareType;		// cached codec firmware type
	HevcVifState			vifState[HEVC_STREAM_MAX];	// cached video interface state
	HevcGpioState			gpioState[HEVC_GPIO_MAX];	// cached gpio state

	// dpc for each irq
	Ntv2Dpc 				irqDpc[FMB_FACT_IRQ_MAX];
	Ntv2Register			intRegStatus;		// interrupt status register
	Ntv2Register			intRegClear;		// interrupt clear register

	// the register control
	Ntv2InterruptLock		regAccessLock;		// register access spinlock
	// spinlock for each continuity counter (command and stream codec transactions)
	Ntv2SpinLock			regContinuityLock[FMB_PCI_CONT_TYPE_MAX];

	// the command task queue
	HevcCommandTask			cmdQueue[HEVC_COMMAND_TASK_MAX];
	Ntv2Dpc 				cmdQueueDpc;		// command task send to codec dpc
	Ntv2SpinLock			cmdQueueLock;		// command queue data lock
	uint32_t				cmdQueueReady;		// next command ready queue index
	uint32_t				cmdQueueSend;		// next command send queue index
	int64_t					cmdQueueCount;		// total number of commands queued
	uint32_t				cmdQueueLevel;		// number of commands in the queue

	// the streams transfer queue
	HevcStreamTask			strQueue[FMB_STREAM_TYPE_MAX][HEVC_STREAM_TASK_MAX];
	// stream task send to codec dpc
	Ntv2Dpc				 	strQueueDpc[FMB_STREAM_TYPE_MAX];
	// stream queue data lock
	Ntv2SpinLock			strQueueLock[FMB_STREAM_TYPE_MAX];
	// next transfer ready queue index
	uint32_t				strQueueReady[FMB_STREAM_TYPE_MAX];
	// next transfer send queue index
	uint32_t				strQueueSend[FMB_STREAM_TYPE_MAX];
	// next transfer send queue stream id
	uint32_t				strQueueId[FMB_STREAM_TYPE_MAX];
	// total number of transfers queued
	int64_t					strQueueCount[FMB_STREAM_TYPE_MAX];
	// number of transfers in the queue
	uint32_t				strQueueLevel[FMB_STREAM_TYPE_MAX];

	// stream transfer statistics
	HevcStreamStatistics	strStats[FMB_STREAM_TYPE_MAX][FMB_STREAM_ID_MAX];
	// stream transfer statistics data lock
	Ntv2SpinLock			strStatLock[FMB_STREAM_TYPE_MAX][FMB_STREAM_ID_MAX];
	// last stream transfer time stamp
	int64_t					strStatTransferLastTime[FMB_STREAM_TYPE_MAX][FMB_STREAM_ID_MAX];

	// stream buffer pool
	HevcStreamBufferGroup	strBuffers[FMB_STREAM_TYPE_MAX][FMB_STREAM_ID_MAX][HEVC_STREAM_BUFFER_MAX];
	// number of stream buffers
	uint32_t				strNumBuffers[FMB_STREAM_TYPE_MAX];
	// stream buffer pool data lock
	Ntv2SpinLock			strBufferLock[FMB_STREAM_TYPE_MAX][FMB_STREAM_ID_MAX];
	// stream buffer wait event
	Ntv2Event				strBufferEvent[FMB_STREAM_TYPE_MAX][FMB_STREAM_ID_MAX];

	// frame data queue
	HevcFrameData			strFrameQueue[FMB_STREAM_ID_MAX][HEVC_FRAME_DATA_MAX];
	// frame data queue data lock
	Ntv2SpinLock			strFrameLock[FMB_STREAM_ID_MAX];
	// next frame data ready queue index
	uint32_t				strFrameReady[FMB_STREAM_ID_MAX];
	// next frame data done queue index
	uint32_t				strFrameDone[FMB_STREAM_ID_MAX];
	// number of frames in the queue
	uint32_t				strFrameQueueLevel[FMB_STREAM_ID_MAX];
} HevcDeviceParams;

#ifdef __cplusplus
extern "C"
{
#endif

// initialize and release parameter structures
void				hevcParamsInitialize(const char* pModuleName);
void				hevcParamsRelease(void);


// get maximum number of codecs supported
uint32_t			hevcGetMaxDevices(void);

// allocate a new device
uint32_t			hevcNewDevice(void);
void				hevcFreeDevice(uint32_t devNum);

// get the (singleton) module parameters
HevcModuleParams*	hevcGetModuleParams(void);

// get the device parameters
HevcDeviceParams*	hevcGetDeviceParams(uint32_t devNum);

#ifdef __cplusplus
}
#endif

#endif
