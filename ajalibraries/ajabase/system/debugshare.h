/**
	@file		debugshare.h
	@copyright	Copyright (C) 2009-2016 AJA Video Systems, Inc.  All rights reserved.
	@brief		Declares the constants used for sharing debug messages. These structures are used
				to gather debug messages and share them with the applications that report and log.
	@note		This file is shared with drivers written in c.
**/

#ifndef AJA_DEBUGSHARE_H
#define AJA_DEBUGSHARE_H


/**
 *	The list of debug message severity codes.
 *	@ingroup AJAGroupDebug
 *	@{
 */
typedef enum _AJADebugSeverity
{
	AJA_DebugSeverity_Emergency		= 0,	/**< System is unusable */
	AJA_DebugSeverity_Alert			= 1,	/**< Action must be taken immediately */
	AJA_DebugSeverity_Assert		= 2,	/**< Assert conditions */
	AJA_DebugSeverity_Error			= 3,	/**< Error conditions */
	AJA_DebugSeverity_Warning		= 4,	/**< Warning conditions */
	AJA_DebugSeverity_Notice		= 5,	/**< Normal but significant condition */
	AJA_DebugSeverity_Info			= 6,	/**< Informational */
	AJA_DebugSeverity_Debug			= 7,	/**< Debug-level messages */
	AJA_DebugSeverity_Size			= 8		/**< Size of severity enum, must be last */

} AJADebugSeverity;


/**
 *	The list of debug message groups.
 *	@ingroup AJAGroupDebug
 *	@{
 */
typedef enum _AJADebugUnit
{
	AJA_DebugUnit_Unknown					= 0,
	AJA_DebugUnit_Critical					= 1,
	AJA_DebugUnit_DriverState				= 2,
	AJA_DebugUnit_DeviceState				= 3,
	AJA_DebugUnit_FileObjectState			= 4,
	AJA_DebugUnit_Enumeration				= 5,
	AJA_DebugUnit_Device					= 6,
	AJA_DebugUnit_DispatchState				= 7,
	AJA_DebugUnit_DispatchMessage			= 8,
	AJA_DebugUnit_RootState					= 9,
	AJA_DebugUnit_VirtualDeviceState		= 10,
	AJA_DebugUnit_RegisterState				= 11,
	AJA_DebugUnit_RegisterRead				= 12,
	AJA_DebugUnit_RegisterWrite				= 13,
	AJA_DebugUnit_RegisterIOState			= 14,
	AJA_DebugUnit_NTV2DeviceState			= 15,
	AJA_DebugUnit_MacUserClient				= 16,
	AJA_DebugUnit_Unformatted				= 17,
	AJA_DebugUnit_FslpexDmaState			= 18,
	AJA_DebugUnit_FslpexDmaTransfer			= 19,
	AJA_DebugUnit_FslpexDmaError			= 21,
	AJA_DebugUnit_InterruptState			= 22,
	AJA_DebugUnit_InterruptPrimary			= 23,
	AJA_DebugUnit_InterruptDpc				= 24,
	AJA_DebugUnit_InterruptError			= 25,
	AJA_DebugUnit_CorvidUltraDeviceState	= 26,
	AJA_DebugUnit_CorvidUltraDmaState		= 27,
	AJA_DebugUnit_SequencerState			= 28,
	AJA_DebugUnit_FslpexDmaStatistics		= 29,
	AJA_DebugUnit_InterruptIOState			= 30,
	AJA_DebugUnit_InterruptIOStatistics		= 31,
	AJA_DebugUnit_InterruptIOUser			= 32,
	AJA_DebugUnit_DeviceFeaturesState		= 33,
	AJA_DebugUnit_CrusherDeviceState		= 34,
	AJA_DebugUnit_CrusherDmaState       	= 35,
	AJA_DebugUnit_MemoryMapState			= 36,
	AJA_DebugUnit_MemoryMapError			= 37,
	AJA_DebugUnit_ConfigFPGAError			= 38,
	AJA_DebugUnit_DmaIOState				= 39,
	AJA_DebugUnit_DmaIOUser					= 40,
	AJA_DebugUnit_RegSequenceState			= 41,
	AJA_DebugUnit_RegSequenceStatistics		= 42,
	AJA_DebugUnit_RegSequenceUser			= 43,
	AJA_DebugUnit_SequencerStatistics		= 44,
	AJA_DebugUnit_VideoTrackState			= 45,
	AJA_DebugUnit_VideoTrackStatistics		= 46,
	AJA_DebugUnit_DmaState					= 47,
	AJA_DebugUnit_DmaTransfer				= 48,
	AJA_DebugUnit_DmaDescriptor				= 49,
	AJA_DebugUnit_DmaError					= 50,
	AJA_DebugUnit_DmaStatistics				= 51,
	AJA_DebugUnit_AudioTrackState			= 52,
	AJA_DebugUnit_AudioTrackStatistics		= 53,
	AJA_DebugUnit_RegSequenceTask			= 54,
	AJA_DebugUnit_UserGeneric				= 55,
	AJA_DebugUnit_IntopixTrackState			= 56,
	AJA_DebugUnit_IntopixTrackStatistics	= 57,
	AJA_DebugUnit_KiProQuadDeviceState		= 58,
	AJA_DebugUnit_SegmentDmaState			= 59,
	AJA_DebugUnit_SegmentDmaDescriptor		= 60,
	AJA_DebugUnit_EffectTrackState			= 61,
	AJA_DebugUnit_EffectTrackStatistics		= 62,
	AJA_DebugUnit_FrameTrackState			= 63,
	AJA_DebugUnit_FrameTrackStatistics		= 64,
	AJA_DebugUnit_Scaler					= 65,
	AJA_DebugUnit_RegisterTrackState		= 66,
	AJA_DebugUnit_RegisterTrackStatistics	= 67,
	AJA_DebugUnit_RegSequenceDpcTime		= 68,
	AJA_DebugUnit_TimeCodeGenState			= 69,
	AJA_DebugUnit_TimeCodeGenStatistics		= 70,
	AJA_DebugUnit_SpockDeviceState          = 71,
	AJA_DebugUnit_HDMI4KOutState            = 72,
	AJA_DebugUnit_HDMI4KOutI2C	            = 73,
	AJA_DebugUnit_HDMI4KOutEDID	            = 74,
	AJA_DebugUnit_QDeviceState      		= 75,
	AJA_DebugUnit_AncProc_General			= 76,
	AJA_DebugUnit_AncProc_InputThread		= 77,
	AJA_DebugUnit_AncProc_OutputThread		= 78,
	AJA_DebugUnit_AncProc_FrameProc			= 79,

	AJA_DebugUnit_Application				= 80,
    AJA_DebugUnit_StatsGeneric				= 81,

    AJA_DebugUnit_Size						= 82
} AJADebugUnit;


/**
	@defgroup	AJAUnitDestination	AJA_DEBUG_DESTINATION
	Bit definitions that specify the destination of a debug message.

	@ingroup AJAGroupDebug
	Use logical OR for multiple destinations.
**/
///@{
#define AJA_DEBUG_DESTINATION_NONE		0			/**< Unknown destination, used as default */
#define AJA_DEBUG_DESTINATION_DEBUG		0x00000001	/**< Send message to the debug window */
#define AJA_DEBUG_DESTINATION_CONSOLE	0x00000002	/**< Send message to the console */
#define AJA_DEBUG_DESTINATION_LOG		0x00000004	/**< Send message to a log file */
#define AJA_DEBUG_DESTINATION_DRIVER	0x00000008	/**< Send message directly to driver output (driver messages only) */
///@}

/**
	@defgroup	AJAGroupVarious	AJA_DEBUG
	Various parameters that define the characteristics of the shared debug memory space.
	@ingroup	AJAGroupDebug
**/
///@{
#define AJA_DEBUG_VERSION				100					/**< Version of the debug system */
#define AJA_DEBUG_UNIT_ARRAY_SIZE		65536				/**< Number of unit destinations */
#define AJA_DEBUG_SEVERITY_ARRAY_SIZE	64					/**< Number of severity destinations */
#define AJA_DEBUG_MESSAGE_MAX_SIZE		512					/**< Maximum size of a message */
#define AJA_DEBUG_MESSAGE_RING_SIZE		4096				/**< Size of the message ring */
#define AJA_DEBUG_FILE_NAME_MAX_SIZE	256					/**< Maximum size of a file name */
#define AJA_DEBUG_SHARE_NAME			"AJADebugShare"		/**< Name of the shared memory for the debug messages */
#define AJA_DEBUG_TICK_RATE				1000000				/**< Resolution of debug time in ticks/second */
#define AJA_DEBUG_STATE_FILE_VERSION	500					/**< Version number of the state file format */
///@}

/**
	Structure representing the debug message.
	@ingroup	AJAGroupDebug
**/
typedef struct _AJADebugMessage
{
	int64_t		time;										/**< Time this message was generated (microseconds) */
	int32_t		sequenceNumber;								/**< Sequence number of this message */
	int32_t		groupIndex;									/**< Group that generated this message */
	uint32_t	destinationMask;							/**< Destination of the message */
	int32_t		severity;									/**< Severity of the message */
	int32_t		lineNumber;									/**< Source file line number that generated this message */
	uint32_t	reserved;									/**< Reserved */
	char		fileName[AJA_DEBUG_FILE_NAME_MAX_SIZE];		/**< Source file name that generated this message */
	char		messageText[AJA_DEBUG_MESSAGE_MAX_SIZE];	/**< Text generated for this message */
} AJADebugMessage;

/**
	Structure representing the shared debug groups and messages.
	@ingroup	AJAGroupDebug
**/
typedef struct _AJADebugShare
{
	uint32_t			version;										/**< Version of the debug system */
	int32_t volatile	writeIndex;										/**< Write index for the message ring */
	uint32_t			severityArray[AJA_DEBUG_SEVERITY_ARRAY_SIZE];	/**< Array of message destinations by severity */
	uint32_t			unitArray[AJA_DEBUG_UNIT_ARRAY_SIZE];			/**< Array of message destinations by unit */
	AJADebugMessage		messageRing[AJA_DEBUG_MESSAGE_RING_SIZE];		/**< Message ring holding current message data */
} AJADebugShare;

#endif	//	AJA_DEBUGSHARE_H
