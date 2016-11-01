/**
	@file		ntv2nubaccess.h
	@brief		Declares functions to connect/open/close/send/receive data via the NTV2 "nub".
	@copyright	(C) 2006-2016 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#ifndef NTV2NUBACCESS_H
#define NTV2NUBACCESS_H

#if defined(AJALinux ) || defined(AJAMac)
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#endif

#include "ajaexport.h"

#ifdef MSWindows
#include <WinSock.h>
#endif

#include "ntv2nubtypes.h"

AJAExport int
NTV2ConnectToNub(	const char *hostname, 
					AJASocket *sockfd);

#define NTV2_REMOTE_ACCESS_SUCCESS						  	 0
#define NTV2_REMOTE_ACCESS_NOT_CONNECTED  				 	-1
#define NTV2_REMOTE_ACCESS_OUT_OF_MEMORY				 	-2
#define NTV2_REMOTE_ACCESS_SEND_ERR						 	-3
#define NTV2_REMOTE_ACCESS_CONNECTION_CLOSED 			 	-4
#define NTV2_REMOTE_ACCESS_RECV_ERR						 	-5
#define NTV2_REMOTE_ACCESS_TIMEDOUT				  		 	-6
#define NTV2_REMOTE_ACCESS_NO_CARD						 	-7
#define NTV2_REMOTE_ACCESS_NOT_OPEN_RESP				 	-8
#define NTV2_REMOTE_ACCESS_NON_NUB_PKT					 	-9
#define NTV2_REMOTE_ACCESS_NOT_READ_REGISTER_RESP			-10
#define NTV2_REMOTE_ACCESS_NOT_WRITE_REGISTER_RESP			-11
#define NTV2_REMOTE_ACCESS_NOT_AUTOCIRC_RESP				-12
#define NTV2_REMOTE_ACCESS_NOT_WAIT_FOR_INTERRUPT_RESP		-13
#define NTV2_REMOTE_ACCESS_WAIT_FOR_INTERRUPT_FAILED		-14
#define NTV2_REMOTE_AUTOCIRC_FAILED							-15
#define NTV2_REMOTE_ACCESS_DRIVER_GET_BITFILE_INFO_FAILED	-16
#define NTV2_REMOTE_ACCESS_NOT_DRIVER_GET_BITFILE_INFO		-17
#define NTV2_REMOTE_ACCESS_NOT_DOWNLOAD_TEST_PATTERN		-18
#define NTV2_REMOTE_ACCESS_DOWNLOAD_TEST_PATTERN_FAILED		-19
#define NTV2_REMOTE_ACCESS_READ_REG_MULTI_FAILED			-20
#define NTV2_REMOTE_ACCESS_NOT_READ_REG_MULTI				-21
#define NTV2_REMOTE_ACCESS_GET_DRIVER_VERSION_FAILED		-22
#define NTV2_REMOTE_ACCESS_NOT_GET_DRIVER_VERSION_RESP		-23
#define NTV2_REMOTE_ACCESS_READ_REG_FAILED					-24
#define NTV2_REMOTE_ACCESS_DRIVER_GET_BUILD_INFO_FAILED		-25
#define NTV2_REMOTE_ACCESS_NOT_DRIVER_GET_BUILD_INFO		-26


AJAExport int 
NTV2OpenRemoteCard(	AJASocket sockfd, 
					UWord inDeviceIndex, 
					UWord boardType, 
					LWord *handle,
					NTV2NubProtocolVersion *nubProtocolVersion);

AJAExport int
NTV2ReadRegisterRemote(	AJASocket sockfd,
						LWord  handle,
						NTV2NubProtocolVersion nubProtocolVersion,
						ULWord registerNumber,
						ULWord *registerValue,
						ULWord registerMask,
						ULWord registerShift);

AJAExport int
NTV2WriteRegisterRemote(AJASocket sockfd,
						LWord  handle,
						NTV2NubProtocolVersion nubProtocolVersion,
						ULWord registerNumber,
						ULWord registerValue,
						ULWord registerMask,
						ULWord registerShift);

AJAExport int 
NTV2AutoCirculateRemote(AJASocket sockfd,
						LWord handle,
						NTV2NubProtocolVersion nubProtocolVersion,
						AUTOCIRCULATE_DATA &autoCircData);

AJAExport int
NTV2WaitForInterruptRemote(AJASocket sockfd,
						LWord  handle,
						NTV2NubProtocolVersion nubProtocolVersion,
						INTERRUPT_ENUMS eInterrupt,
						ULWord timeOutMs);

AJAExport int
NTV2DriverGetBitFileInformationRemote(	AJASocket sockfd,
										LWord  handle,
										NTV2NubProtocolVersion nubProtocolVersion,
										BITFILE_INFO_STRUCT &bitFileInfo,
										NTV2BitFileType bitFileType);

AJAExport int
NTV2DriverGetBuildInformationRemote(	AJASocket sockfd,
										LWord  handle,
										NTV2NubProtocolVersion nubProtocolVersion,
										BUILD_INFO_STRUCT &buildInfo);


AJAExport int
NTV2DownloadTestPatternRemote(AJASocket sockfd,	
								LWord handle,
								NTV2NubProtocolVersion nubProtocolVersion,
								NTV2Channel channel,
								NTV2FrameBufferFormat testPatternFrameBufferFormat,
								UWord signalMask,
								bool testPatternDMAEnable,
								ULWord testPatternNumber);

AJAExport int 
NTV2ReadRegisterMultiRemote(AJASocket sockfd,
							LWord remoteHandle,
							NTV2NubProtocolVersion nubProtocolVersion,
							ULWord numRegs,
							ULWord *whichRegisterFailed,
							NTV2ReadWriteRegisterSingle aRegs[]);

AJAExport int 
NTV2GetDriverVersionRemote(AJASocket sockfd,
							LWord remoteHandle,
							NTV2NubProtocolVersion nubProtocolVersion,
							ULWord *driverVersion);
#endif	//	NTV2NUBACCESS_H
