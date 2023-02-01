/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
//////////////////////////////////////////////////////////////
//
// HEVC Linux Device Driver for AJA HEVC boards.
//
// Boards supported include:
//
// OEM-HEVC
//
// Filename: hevcapi.c
// Purpose:	 Main module file.  Load, unload, fops, ioctls.
//
///////////////////////////////////////////////////////////////


#include "hevccommon.h"
#include "hevcregister.h"
#include "hevcinterrupt.h"
#include "hevccommand.h"
#include "hevcstream.h"

// reset the codec
static Ntv2Status hevcDeviceReset(uint32_t devNum);

// eh parameter changes
static Ntv2Status hevcChangeCbr(HevcDeviceCommand* pCommand, HevcCommandInfo* pCmdInfo);
static Ntv2Status hevcChangeVbr(HevcDeviceCommand* pCommand, HevcCommandInfo* pCmdInfo);
static Ntv2Status hevcChangeResolution(HevcDeviceCommand* pCommand, HevcCommandInfo* pCmdInfo);
static Ntv2Status hevcChangeFrameRate(HevcDeviceCommand* pCommand, HevcCommandInfo* pCmdInfo);

// check register address
static bool hevcIsAccessibleAddress(uint32_t devNum, uint32_t address, uint32_t size);

// transfer raw video
static Ntv2Status hevcVideoTransferRaw(uint32_t devNum, HevcTransferData* pTransfer);

// transfer encoded video
static Ntv2Status hevcVideoTransferEnc(uint32_t devNum, HevcTransferData* pTransfer);

// get the main state
static HevcMainState hevcGetMainState(uint32_t state);

// get the encode mode
static HevcEncodeMode hevcGetEncodeMode(uint32_t mode);

// get the firmware type
static HevcFirmwareType hevcGetFirmwareType(uint32_t type);

// get the vin state
static HevcVinState hevcGetVinState(uint32_t state);

// get the eh state
static HevcEhState hevcGetEhState(uint32_t state);


Ntv2Status hevcDeviceOpen(uint32_t devNum)
{
	HevcModuleParams* pModParams = hevcGetModuleParams();
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);

	if(pDevParams == NULL) return NTV2_STATUS_NO_DEVICE;

	HEVC_MSG_INFO("%s(%d): open hevc device\n", pModParams->pModuleName, devNum);

	HEVC_MSG_INFO("%s(%d): pci id:  vendor %04x   device %04x   subvendor %04x   subdevice %04x\n",
				  pModParams->pModuleName, devNum,
				  pDevParams->pciId.vendor, pDevParams->pciId.device,
				  pDevParams->pciId.subVendor, pDevParams->pciId.subDevice);

	// initialize register access
	hevcRegisterInitialize(devNum);

	if(pDevParams->deviceMode == Hevc_DeviceMode_Codec)
	{
		// initialize interrupt handling
		hevcInterruptInitialize(devNum);

		// initialize command queue
		hevcCommandInitialize(devNum);

		// initialize stream queues
		hevcStreamInitialize(devNum);

		// report version data
		{
			uint32_t revMCPU[FMB_VERSION_SIZE_MCPU_FIRM];
			char revSystem[FMB_VERSION_SIZE_SYSTEM_FIRM + 1];
			char revStdSingle[FMB_VERSION_SIZE_HEVC_ENCODER_FIRM + 1];
			char revStdMultiple[FMB_VERSION_SIZE_HEVC_ENCODER_FIRM + 1];
			char revUserSingle[FMB_VERSION_SIZE_HEVC_ENCODER_FIRM + 1];
			char revUserMultiple[FMB_VERSION_SIZE_HEVC_ENCODER_FIRM + 1];
			int i;

			for(i = 0; i < FMB_VERSION_SIZE_MCPU_FIRM; i++) revMCPU[i] = i;
			for(i = 0; i < FMB_VERSION_SIZE_SYSTEM_FIRM; i++) revSystem[i] = ' ';
			for(i = 0; i < FMB_VERSION_SIZE_HEVC_ENCODER_FIRM; i++) revStdSingle[i] = ' ';
			for(i = 0; i < FMB_VERSION_SIZE_HEVC_ENCODER_FIRM; i++) revStdMultiple[i] = ' ';
			for(i = 0; i < FMB_VERSION_SIZE_HEVC_ENCODER_FIRM; i++) revUserSingle[i] = ' ';
			for(i = 0; i < FMB_VERSION_SIZE_HEVC_ENCODER_FIRM; i++) revUserMultiple[i] = ' ';
			strncpy(revSystem, ">>> register read failed <<<", FMB_VERSION_SIZE_SYSTEM_FIRM-1);
			strncpy(revStdSingle, ">>> register read failed <<<", FMB_VERSION_SIZE_HEVC_ENCODER_FIRM-1);
			strncpy(revStdMultiple, ">>> register read failed <<<", FMB_VERSION_SIZE_HEVC_ENCODER_FIRM-1);
			strncpy(revUserSingle, ">>> register read failed <<<", FMB_VERSION_SIZE_HEVC_ENCODER_FIRM-1);
			strncpy(revUserMultiple, ">>> register read failed <<<", FMB_VERSION_SIZE_HEVC_ENCODER_FIRM-1);

			hevcGetFirmwareVersion(devNum, revMCPU, revSystem,
								   revStdSingle, revStdMultiple,
								   revUserSingle, revUserMultiple);

			HEVC_MSG_INFO("%s(%d): mcpu firmware version:                              %d.%d.%d.%d\n",
						  pModParams->pModuleName, devNum,
						  revMCPU[0], revMCPU[1], revMCPU[2], revMCPU[3]);
			HEVC_MSG_INFO("%s(%d): system firmware version:                            %s\n",
						  pModParams->pModuleName, devNum, revSystem);
			HEVC_MSG_INFO("%s(%d): encoder standard single stream firmware version:    %s\n",
						  pModParams->pModuleName, devNum, revStdSingle);
			HEVC_MSG_INFO("%s(%d): encoder standard multiple stream firmware version:  %s\n",
						  pModParams->pModuleName, devNum, revStdMultiple);
			HEVC_MSG_INFO("%s(%d): encoder User single stream firmware version:        %s\n",
						  pModParams->pModuleName, devNum, revUserSingle);
			HEVC_MSG_INFO("%s(%d): encoder User multiple stream firmware version:      %s\n",
						  pModParams->pModuleName, devNum, revUserMultiple);
		}
	}
	else
	{
		HEVC_MSG_INFO("%s(%d): >>> maintenance mode <<<\n",
					  pModParams->pModuleName, devNum);
	}

	return NTV2_STATUS_SUCCESS;
}

Ntv2Status hevcDeviceClose(uint32_t devNum)
{
	HevcModuleParams* pModParams = hevcGetModuleParams();
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);

	if(pDevParams == NULL) return NTV2_STATUS_NO_DEVICE;

	HEVC_MSG_INFO("%s(%d): close hevc device\n", pModParams->pModuleName, devNum);

	if(pDevParams->deviceMode == Hevc_DeviceMode_Codec)
	{
		hevcStreamRelease(devNum);
		hevcCommandRelease(devNum);
		hevcInterruptRelease(devNum);
	}
	hevcRegisterRelease(devNum);

	return NTV2_STATUS_SUCCESS;
}

Ntv2Status hevcGetDeviceInfo(uint32_t devNum, HevcMessageInfo* pMessage)
{
	HevcModuleParams* pModParams = hevcGetModuleParams();
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	HevcDeviceInfo* pDevInfo;

	if(pDevParams == NULL) return NTV2_STATUS_NO_DEVICE;
	if(pMessage == NULL) return NTV2_STATUS_BAD_PARAMETER;
	if(pMessage->header.type != Hevc_MessageId_Info) return NTV2_STATUS_BAD_PARAMETER;
	if(pMessage->header.size != sizeof(HevcMessageInfo)) return NTV2_STATUS_BAD_PARAMETER;
	pDevInfo = &pMessage->data;

	// get driver version
	pDevInfo->driverVersion = pModParams->driverVersion;

	// get codec version info
	hevcGetFirmwareVersion(devNum,
						   &pDevInfo->mcpuVersion.major,
						   pDevInfo->systemFirmware,
						   pDevInfo->standardFirmwareSingle,
						   pDevInfo->standardFirmwareMultiple,
						   pDevInfo->userFirmwareSingle,
						   pDevInfo->userFirmwareMultiple);

	pDevInfo->mcpuVersionCheck = ((pDevInfo->mcpuVersion.major == HEVC_MCPU_MAJOR) &&
								  (pDevInfo->mcpuVersion.minor == HEVC_MCPU_MINOR) &&
								  (pDevInfo->mcpuVersion.point == HEVC_MCPU_POINT) &&
								  (pDevInfo->mcpuVersion.build == HEVC_MCPU_BUILD));
	pDevInfo->systemVersionCheck = (strcmp(pDevInfo->systemFirmware, HEVC_SYSTEM_FIRMWARE) == 0);
	pDevInfo->standardSingleCheck = (strcmp(pDevInfo->standardFirmwareSingle, HEVC_ENCODER_FIRMWARE_SINGLE) == 0);
	pDevInfo->standardMultipleCheck = (strcmp(pDevInfo->standardFirmwareMultiple, HEVC_ENCODER_FIRMWARE_MULTIPLE) == 0);

	// get pci info
	pDevInfo->pciId = pDevParams->pciId;

	pDevInfo->pciIdCheck = ((pDevInfo->pciId.vendor == HEVC_PCI_VENDOR) &&
							(pDevInfo->pciId.device == HEVC_PCI_DEVICE) &&
							(pDevInfo->pciId.subVendor == HEVC_PCI_SUBVENDOR) &&
							(pDevInfo->pciId.subDevice == HEVC_PCI_SUBDEVICE));

	// get device mode
	pDevInfo->deviceMode = pDevParams->deviceMode;

	return NTV2_STATUS_SUCCESS;
}

Ntv2Status hevcRegister(uint32_t devNum, HevcMessageRegister* pMessage)
{
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	HevcDeviceRegister* pRegister;
	uint32_t mask;
	uint32_t shift;
	uint32_t value;

	if(pDevParams == NULL) return NTV2_STATUS_NO_DEVICE;
	if(pMessage == NULL) return NTV2_STATUS_BAD_PARAMETER;
	if(pMessage->header.type != Hevc_MessageId_Register) return NTV2_STATUS_BAD_PARAMETER;
	if(pMessage->header.size != sizeof(HevcMessageRegister)) return NTV2_STATUS_BAD_PARAMETER;

	pRegister = &pMessage->data;
	mask = pRegister->mask;
	shift = pRegister->shift;

	if(!hevcIsAccessibleAddress(devNum, pRegister->address, sizeof(uint32_t))) return NTV2_STATUS_FAIL;

	if(pRegister->write)
	{
		if(pRegister->forceBar4)
		{
			hevcRegWriteBar4(devNum, pRegister->address, pRegister->writeValue);
		}
		else if(mask == 0xffffffff)
		{
			// if full mask just write
			hevcRegWrite(devNum, pRegister->address, pRegister->writeValue);
		}
		else
		{
			// do read modify write
			if(shift == 0xffffffff) shift = 0;
			hevcRegRMW(devNum, pRegister->address, mask, 0, pRegister->writeValue << shift);
		}
	}

	if(pRegister->read)
	{
		if(pRegister->forceBar4)
		{
			pRegister->readValue = hevcRegReadBar4(devNum, pRegister->address);
		}
		else
		{
			value = hevcRegRead(devNum, pRegister->address);
			pRegister->readValue = (value & mask) >> shift;
		}
	}

	return NTV2_STATUS_SUCCESS;
}

Ntv2Status hevcSendCommand(uint32_t devNum, HevcMessageCommand* pMessage)
{
	HevcModuleParams* pModParams = hevcGetModuleParams();
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	HevcDeviceCommand* pCommand;
	HevcCommandInfo cmdInfo;
	HevcCommandTask* pCmdTask;
	Ntv2Status status = 0;
	uint32_t vifStateStop = 0;
	bool saveEncodeMode = false;
	bool setGpioFunction = false;
	bool setGpioDirection = false;
	bool setGpioValue = false;
	bool getGpioValue = false;
	uint32_t streamId;

	if(pDevParams == NULL) return NTV2_STATUS_NO_DEVICE;
	if(pMessage == NULL) return NTV2_STATUS_BAD_PARAMETER;
	if(pMessage->header.type != Hevc_MessageId_Command) return NTV2_STATUS_BAD_PARAMETER;
	if(pMessage->header.size != sizeof(HevcMessageCommand)) return NTV2_STATUS_BAD_PARAMETER;
	if(pDevParams->deviceMode != Hevc_DeviceMode_Codec) return NTV2_STATUS_BAD_PARAMETER;
	pCommand = &pMessage->data;

    memset(&cmdInfo, 0, sizeof(HevcCommandInfo));

	// special processing for reset command
	if(pCommand->command == Hevc_Command_Reset)
	{
		HEVC_MSG_COMMAND_INFO("%s(%d): hevcDeviceCommand()  device reset\n",
							  pModParams->pModuleName, devNum);

		// reset the codec
		status = hevcDeviceReset(devNum);
		if(status == 0)
		{
			pMessage->header.status = HEVC_STATUS_SUCCESS;
		}
		else
		{
			HEVC_MSG_COMMAND_ERROR("%s(%d): hevcDeviceCommand()  *error* reset status %08x\n",
								   pModParams->pModuleName, devNum, status);
		}

		// reset the command and stream queues
		hevcCommandReset(devNum);
		hevcStreamReset(devNum);
		hevcStreamClearAllStats(devNum);

		return status;
	}

	// verify the codec is working
	if(!hevcIsDeviceAlive(devNum)) return NTV2_STATUS_BAD_STATE;

	// fill out command information
	switch(pCommand->command)
	{
	case Hevc_Command_MainState:
		switch(pCommand->mainState)
		{
		case Hevc_MainState_Init:
			HEVC_MSG_COMMAND_INFO("%s(%d): hevcDeviceCommand()  main state = init\n",
								  pModParams->pModuleName, devNum);
			cmdInfo.target = FMB_COMMAND_TARGET_MAIN;
			cmdInfo.id = FMB_COMMAND_ID_STATE_CHANGE;
			cmdInfo.param[0] = FMB_MAIN_STATE_INIT;
			cmdInfo.param[1] = 0;
			break;
		case Hevc_MainState_Encode:
			cmdInfo.target = FMB_COMMAND_TARGET_MAIN;
			cmdInfo.id = FMB_COMMAND_ID_STATE_CHANGE;
			cmdInfo.param[0] = FMB_MAIN_STATE_ENCODE;
			switch(pCommand->encodeMode)
			{
			case Hevc_EncodeMode_Single:
				HEVC_MSG_COMMAND_INFO("%s(%d): hevcDeviceCommand()  main state: encode single\n",
									  pModParams->pModuleName, devNum);
				cmdInfo.param[1] = FMB_ENCODE_MODE_SINGLE_CH;
				break;
			case Hevc_EncodeMode_Multiple:
				HEVC_MSG_COMMAND_INFO("%s(%d): hevcDeviceCommand()  main state: encode multiple\n",
									  pModParams->pModuleName, devNum);
				cmdInfo.param[1] = FMB_ENCODE_MODE_MULTI_CH;
				break;
			default:
				HEVC_MSG_COMMAND_ERROR("%s(%d): hevcDeviceCommand()  main state: *error* bad encode mode %d\n",
									   pModParams->pModuleName, devNum, pCommand->encodeMode);
				status = NTV2_STATUS_BAD_PARAMETER;
				break;
			}
			switch(pCommand->firmwareType)
			{
			case Hevc_FirmwareType_Standard:
				HEVC_MSG_COMMAND_INFO("%s(%d): hevcDeviceCommand()  main state: firmware standard\n",
									  pModParams->pModuleName, devNum);
				cmdInfo.param[2] = FMB_HEVC_ENCODER_FIRM_STD;
				break;
			case Hevc_FirmwareType_User:
				HEVC_MSG_COMMAND_INFO("%s(%d): hevcDeviceCommand()  main state: firmware user\n",
									  pModParams->pModuleName, devNum);
				cmdInfo.param[2] = FMB_HEVC_ENCODER_FIRM_USER;
				break;
			default:
				HEVC_MSG_COMMAND_ERROR("%s(%d): hevcDeviceCommand()  main state: *error* bad fimware type %d\n",
									   pModParams->pModuleName, devNum, pCommand->firmwareType);
				status = NTV2_STATUS_BAD_PARAMETER;
				break;
			}
			// set cached encode mode
			saveEncodeMode = true;
			// set cached vif state
			vifStateStop = 0xffffffff;
			break;
		default:
			HEVC_MSG_COMMAND_ERROR("%s(%d): hevcDeviceCommand()  main state: *error* bad main state %d\n",
								   pModParams->pModuleName, devNum, pCommand->mainState);
			status = NTV2_STATUS_BAD_PARAMETER;
			break;
		}
		break;

	case Hevc_Command_VinState:
		switch(pCommand->vinState)
		{
		case Hevc_VinState_Stop:
			HEVC_MSG_COMMAND_INFO("%s(%d): hevcDeviceCommand()  vin state: stop  stream bits %08x\n",
								  pModParams->pModuleName, devNum, pCommand->streamBits);
			cmdInfo.target = FMB_COMMAND_TARGET_VIN_ID;
			cmdInfo.id = FMB_COMMAND_ID_STATE_CHANGE;
			cmdInfo.param[0] = FMB_VIN_ID_STATE_STOP;
			cmdInfo.param[1] = pCommand->streamBits;
			// set cached vif state
			vifStateStop = pCommand->streamBits;
			break;
		case Hevc_VinState_Start:
			HEVC_MSG_COMMAND_INFO("%s(%d): hevcDeviceCommand()  vin state: start  stream bits %08x\n",
								  pModParams->pModuleName, devNum, pCommand->streamBits);
			cmdInfo.target = FMB_COMMAND_TARGET_VIN_ID;
			cmdInfo.id = FMB_COMMAND_ID_STATE_CHANGE;
			cmdInfo.param[0] = FMB_VIN_ID_STATE_START;
			cmdInfo.param[1] = pCommand->streamBits;
			hevcStreamFrameReset(devNum, pCommand->streamBits);
			hevcStreamClearStats(devNum, FMB_STREAM_TYPE_VEI, pCommand->streamBits);
			break;
		default:
			HEVC_MSG_COMMAND_ERROR("%s(%d): hevcDeviceCommand()  vin state: *error* bad vin state %d\n",
								   pModParams->pModuleName, devNum, pCommand->vinState);
			status = NTV2_STATUS_BAD_PARAMETER;
			break;
		}
		break;

	case Hevc_Command_EhState:
		switch(pCommand->ehState)
		{
		case Hevc_EhState_Stop:
			HEVC_MSG_COMMAND_INFO("%s(%d): hevcDeviceCommand()  eh state: stop  stream bits %08x\n",
								  pModParams->pModuleName, devNum, pCommand->streamBits);
			cmdInfo.target = FMB_COMMAND_TARGET_EH_ID;
			cmdInfo.id = FMB_COMMAND_ID_STATE_CHANGE;
			cmdInfo.param[0] = FMB_EH_ID_STATE_STOP;
			cmdInfo.param[1] = pCommand->streamBits;
			break;
		case Hevc_EhState_Start:
			HEVC_MSG_COMMAND_INFO("%s(%d): hevcDeviceCommand()  eh state: start  stream bits %08x\n",
								  pModParams->pModuleName, devNum, pCommand->streamBits);
			cmdInfo.target = FMB_COMMAND_TARGET_EH_ID;
			cmdInfo.id = FMB_COMMAND_ID_STATE_CHANGE;
			cmdInfo.param[0] = FMB_EH_ID_STATE_START;
			cmdInfo.param[1] = pCommand->streamBits;
			hevcStreamClearStats(devNum, FMB_STREAM_TYPE_SEO, pCommand->streamBits);
			break;
		case Hevc_EhState_ReadyToStop:
			HEVC_MSG_COMMAND_INFO("%s(%d): hevcDeviceCommand()  eh state: ready to stop  stream bits %08x\n",
								  pModParams->pModuleName, devNum, pCommand->streamBits);
			cmdInfo.target = FMB_COMMAND_TARGET_EH_ID;
			cmdInfo.id = FMB_COMMAND_ID_STATE_CHANGE;
			cmdInfo.param[0] = FMB_EH_ID_STATE_READY_TO_STOP;
			cmdInfo.param[1] = pCommand->streamBits;
			break;
		default:
			HEVC_MSG_COMMAND_ERROR("%s(%d): hevcDeviceCommand()  eh state: *error* bad eh state %d\n",
								   pModParams->pModuleName, devNum, pCommand->ehState);
			status = NTV2_STATUS_BAD_PARAMETER;
			break;
		}
		break;

	case Hevc_Command_Gpio:
		switch(pCommand->gpioControl)
		{
		case Hevc_GpioControl_Function:
			cmdInfo.target = FMB_COMMAND_TARGET_GPIO;
			cmdInfo.id = FMB_COMMAND_ID_GPIO_FUNCTION;
			switch(pCommand->gpioFunction)
			{
			case Hevc_GpioFunction_Gpio:
				HEVC_MSG_COMMAND_INFO("%s(%d): hevcDeviceCommand()  gpio(%d) function: gpio\n",
									  pModParams->pModuleName, devNum, pCommand->gpioNumber);
				cmdInfo.param[0] = FMB_GPIO_FUNCTION_GPIO;
				break;
			case Hevc_GpioFunction_Peripheral:
				HEVC_MSG_COMMAND_INFO("%s(%d): hevcDeviceCommand()  gpio(%d) function: peripheral\n",
									  pModParams->pModuleName, devNum, pCommand->gpioNumber);
				cmdInfo.param[0] = FMB_GPIO_FUNCTION_PERIPHERAL;
				break;
			default:
				HEVC_MSG_COMMAND_ERROR("%s(%d): hevcDeviceCommand()  gpio(%d) function: *error* bad gpio function %d\n",
									   pModParams->pModuleName, devNum, pCommand->gpioNumber, pCommand->gpioFunction);
				status = NTV2_STATUS_BAD_PARAMETER;
				break;
			}
			setGpioFunction = true;
			break;
		case Hevc_GpioControl_Direction:
			cmdInfo.target = FMB_COMMAND_TARGET_GPIO;
			cmdInfo.id = FMB_COMMAND_ID_GPIO_DIRECTION;
			switch(pCommand->gpioDirection)
			{
			case Hevc_GpioDirection_Input:
				HEVC_MSG_COMMAND_INFO("%s(%d): hevcDeviceCommand()  gpio(%d) direction: input\n",
									  pModParams->pModuleName, devNum, pCommand->gpioNumber);
				cmdInfo.param[0] = FMB_GPIO_DIRECTION_IN;
				break;
			case Hevc_GpioDirection_Output:
				HEVC_MSG_COMMAND_INFO("%s(%d): hevcDeviceCommand()  gpio(%d) direction: output\n",
									  pModParams->pModuleName, devNum, pCommand->gpioNumber);
				cmdInfo.param[0] = FMB_GPIO_DIRECTION_OUT;
				break;
			default:
				HEVC_MSG_COMMAND_ERROR("%s(%d): hevcDeviceCommand()  gpio(%d) function: *error* bad gpio direction %d\n",
									   pModParams->pModuleName, devNum, pCommand->gpioNumber, pCommand->gpioDirection);
				status = NTV2_STATUS_BAD_PARAMETER;
				break;
			}
			setGpioDirection = true;
			break;
		case Hevc_GpioControl_Set:
			HEVC_MSG_COMMAND_INFO("%s(%d): hevcDeviceCommand()  gpio(%d) set value: %d\n",
								  pModParams->pModuleName, devNum, pCommand->gpioNumber, pCommand->gpioValue);
			cmdInfo.target = FMB_COMMAND_TARGET_GPIO;
			cmdInfo.id = FMB_COMMAND_ID_GPIO_SET_VALUE;
			switch(pCommand->gpioValue)
			{
			case Hevc_GpioValue_Low:
				cmdInfo.param[0] = FMB_GPIO_VALUE_LOW;
				break;
			case Hevc_GpioValue_High:
				cmdInfo.param[0] = FMB_GPIO_VALUE_HIGH;
				break;
			default:
				HEVC_MSG_COMMAND_ERROR("%s(%d): hevcDeviceCommand()  gpio(%d) value: *error* bad gpio value %d\n",
									   pModParams->pModuleName, devNum, pCommand->gpioNumber, pCommand->gpioValue);
				status = NTV2_STATUS_BAD_PARAMETER;
				break;
			}
			setGpioValue = true;
			break;
		case Hevc_GpioControl_Get:
			cmdInfo.target = FMB_COMMAND_TARGET_GPIO;
			cmdInfo.id = FMB_COMMAND_ID_GPIO_GET_VALUE;
			cmdInfo.param[0] = 0;
			getGpioValue = true;
			break;
		default:
			HEVC_MSG_COMMAND_ERROR("%s(%d): hevcDeviceCommand()  *error* bad gpio control %d\n",
								   pModParams->pModuleName, devNum, pCommand->gpioControl);
			status = NTV2_STATUS_BAD_PARAMETER;
		}
		if(pCommand->gpioNumber > FMB_GPIO_PORT_MAX)
		{
			HEVC_MSG_COMMAND_ERROR("%s(%d): hevcDeviceCommand()  gpio(%d) *error* bad gpio port number\n",
								   pModParams->pModuleName, devNum, pCommand->gpioNumber);
			status = NTV2_STATUS_BAD_PARAMETER;
		}
		cmdInfo.param[1] = pCommand->gpioNumber;
		break;
            
    case Hevc_Command_ChangeParam:
        if ((pCommand->paramTarget & Hevc_ParamTarget_All) == 0)
		{
			HEVC_MSG_COMMAND_ERROR("%s(%d): hevcDeviceCommand()  change param: *error* bad param target %08x\n",
								   pModParams->pModuleName, devNum, pCommand->paramTarget);
			status = NTV2_STATUS_BAD_PARAMETER;
		}
		else
		{
			cmdInfo.target = FMB_COMMAND_TARGET_EH_ID;
			cmdInfo.id = FMB_COMMAND_ID_EH_ID_ENCODE_PARAM_CHANGE;
			cmdInfo.param[0] = Hevc_ParamTarget_None;
			FMB_SET_ID_BIT (cmdInfo.param[1], pCommand->paramStreamId);

			if ((pCommand->paramTarget & Hevc_ParamTarget_Cbr) != 0)
			{
				HEVC_MSG_COMMAND_INFO("%s(%d): hevcDeviceCommand()  change param: cbr  rate %08x\n",
									  pModParams->pModuleName, devNum, pCommand->aveBitRate);
				status = hevcChangeCbr(pCommand, &cmdInfo);
			}
			else if ((pCommand->paramTarget & Hevc_ParamTarget_Vbr) != 0)
			{
				HEVC_MSG_COMMAND_INFO("%s(%d): hevcDeviceCommand()  change param: vbr  max %08x, ave %08x, min %08x\n",
									  pModParams->pModuleName, devNum, pCommand->maxBitRate, pCommand->aveBitRate, pCommand->minBitRate);
				status = hevcChangeVbr(pCommand, &cmdInfo);
			}

			if ((pCommand->paramTarget & Hevc_ParamTarget_Resolution) != 0)
			{
				HEVC_MSG_COMMAND_INFO("%s(%d): hevcDeviceCommand()  change param: resolution  width %d  height %d\n",
									  pModParams->pModuleName, devNum, pCommand->hSizeEh, pCommand->vSizeEh);
				status = hevcChangeResolution(pCommand, &cmdInfo);
			}
			if ((pCommand->paramTarget & Hevc_ParamTarget_Frame_Rate) != 0)
			{
				HEVC_MSG_COMMAND_INFO("%s(%d): hevcDeviceCommand()  change param: frame rate  code %d\n",
									  pModParams->pModuleName, devNum, pCommand->frameRateCode);
				status = hevcChangeFrameRate(pCommand, &cmdInfo);
			}
		}
		break;
            
    case Hevc_Command_ChangePicture:
		HEVC_MSG_COMMAND_INFO("%s(%d): hevcDeviceCommand()  change picture:  type %08x\n",
							  pModParams->pModuleName, devNum, pCommand->picType);
		cmdInfo.target = FMB_COMMAND_TARGET_EH_ID;
		cmdInfo.id = FMB_COMMAND_ID_EH_ID_ENCODE_PICTURE_TYPE_CHANGE;
		cmdInfo.param[0] = pCommand->picType;
		FMB_SET_ID_BIT (cmdInfo.param[1], pCommand->picStreamId);
		cmdInfo.param[2] = pCommand->gopEndPicNumber;
		break;

	default:
		HEVC_MSG_COMMAND_ERROR("%s(%d): hevcDeviceCommand()  *error* bad command %d\n",
							   pModParams->pModuleName, devNum, pCommand->command);
		status = NTV2_STATUS_BAD_PARAMETER;
	}

	// if it is a good command then queue it up
	if(status == 0)
	{
		// queue command
		pCmdTask = hevcCommandEnqueue(devNum, &cmdInfo);
		if(pCmdTask == NULL) 
		{
			HEVC_MSG_COMMAND_ERROR("%s(%d): hevcDeviceCommand()  *error* command queue full\n",
								   pModParams->pModuleName, devNum);
			return NTV2_STATUS_BUSY;
		}

		// wait for codec command complete and test result
		if(hevcCommandMsgWait(devNum, pCmdTask) &&
		   (pCmdTask->msgInfo.result[0] == FMB_COMMAND_RESULT_0_OK))
		{
			pMessage->header.status = HEVC_STATUS_SUCCESS;
			// cache encode mode
			if(saveEncodeMode)
			{
				pDevParams->encodeMode = pCommand->encodeMode;
				pDevParams->firmwareType = pCommand->firmwareType;
			}
			// cached vif state
			if(vifStateStop != 0)
			{
				for(streamId = 0; streamId < HEVC_STREAM_MAX; streamId++)
				{
					if((vifStateStop & (0x1U<<streamId)) != 0)
					{
						pDevParams->vifState[streamId] = Hevc_VifState_Stop;
					}
				}
			}
			// cached gpio states
			if(setGpioFunction)
			{
				pDevParams->gpioState[pCommand->gpioNumber].function = pCommand->gpioFunction;
			}
			if(setGpioDirection)
			{
				pDevParams->gpioState[pCommand->gpioNumber].direction = pCommand->gpioDirection;
			}
			if(setGpioValue)
			{
				pDevParams->gpioState[pCommand->gpioNumber].setValue = pCommand->gpioValue;
			}
			if(getGpioValue)
			{
				switch(pCmdTask->msgInfo.result[1])
				{
				case FMB_GPIO_VALUE_LOW:
					pCommand->gpioValue = Hevc_GpioValue_Low;
					HEVC_MSG_COMMAND_INFO("%s(%d): hevcDeviceCommand()  gpio(%d) get value: low\n",
										  pModParams->pModuleName, devNum, pCommand->gpioNumber);
					break;
				case FMB_GPIO_VALUE_HIGH:
					pCommand->gpioValue = Hevc_GpioValue_High;
					HEVC_MSG_COMMAND_INFO("%s(%d): hevcDeviceCommand()  gpio(%d) get value: high\n",
										  pModParams->pModuleName, devNum, pCommand->gpioNumber);
					break;
				default:
					pCommand->gpioValue = Hevc_GpioValue_Unknown;
					HEVC_MSG_COMMAND_ERROR("%s(%d): hevcDeviceCommand()  gpio(%d) get value: *error* bad gpio value %d\n",
										   pModParams->pModuleName, devNum, pCommand->gpioNumber, pCmdTask->msgInfo.result[1]);
					status = NTV2_STATUS_BAD_PARAMETER;
				}
				pDevParams->gpioState[pCommand->gpioNumber].getValue = pCommand->gpioValue;
			}
		}
		else
		{
			status = NTV2_STATUS_IO_ERROR;
			HEVC_MSG_COMMAND_ERROR("%s(%d): hevcDeviceCommand()  *error* command failed status 0x%08x\n",
								   pModParams->pModuleName, devNum, pCmdTask->msgInfo.result[0]);
		}

		// remove from queue
		hevcCommandDequeue(devNum, pCmdTask);
	}
		
	return status;
}

Ntv2Status hevcChangeCbr(HevcDeviceCommand* pCommand, HevcCommandInfo* pCmdInfo)
{
	if ((pCommand == NULL) || (pCmdInfo == NULL))  return NTV2_STATUS_BAD_PARAMETER;

	pCmdInfo->param[0] |= FMB_CHANGE_PARAM_CBR;
	pCmdInfo->param[2] = pCommand->aveBitRate;

	return NTV2_STATUS_SUCCESS;
}

Ntv2Status hevcChangeVbr(HevcDeviceCommand* pCommand, HevcCommandInfo* pCmdInfo)
{
	if ((pCommand == NULL) || (pCmdInfo == NULL))  return NTV2_STATUS_BAD_PARAMETER;

	pCmdInfo->param[0] |= FMB_CHANGE_PARAM_VBR;
	pCmdInfo->param[2] = pCommand->maxBitRate;
	pCmdInfo->param[3] = pCommand->aveBitRate;
	pCmdInfo->param[4] = pCommand->minBitRate;
	if (pCommand->changeSequence == Hevc_ChangeSequence_Enabled)
		pCmdInfo->param[5] = FMB_CHANGE_PARAM_DISCONTINUOUS_SEQUENCE;
	else
		pCmdInfo->param[5] = FMB_CHANGE_PARAM_CONTINUOUS_SEQUENCE;

	return NTV2_STATUS_SUCCESS;
}

Ntv2Status hevcChangeResolution(HevcDeviceCommand* pCommand, HevcCommandInfo* pCmdInfo)
{
	if ((pCommand == NULL) || (pCmdInfo == NULL))  return NTV2_STATUS_BAD_PARAMETER;

	pCmdInfo->param[0] |= FMB_CHANGE_PARAM_RESOLUTION;

	pCmdInfo->param[6] = pCommand->seqEndPicNumber;

	FMB_SET_PARAM_BITS ( pCmdInfo->param[7],  HSIZE_EH,                        pCommand->hSizeEh );
	FMB_SET_PARAM_BITS ( pCmdInfo->param[7],  VSIZE_EH,                        pCommand->vSizeEh );

	FMB_SET_PARAM_BITS ( pCmdInfo->param[8],  CROP_LEFT,                       pCommand->cropLeft );
	FMB_SET_PARAM_BITS ( pCmdInfo->param[8],  CROP_RIGHT,                      pCommand->cropRight );

	FMB_SET_PARAM_BITS ( pCmdInfo->param[9],  CROP_TOP,                        pCommand->cropTop );
	FMB_SET_PARAM_BITS ( pCmdInfo->param[9],  CROP_BOTTOM,                     pCommand->cropBottom );

	FMB_SET_PARAM_BITS ( pCmdInfo->param[10], PAN_SCAN_RECT_LEFT,              pCommand->panScanRectLeft );
	FMB_SET_PARAM_BITS ( pCmdInfo->param[10], PAN_SCAN_RECT_RIGHT,             pCommand->panScanRectRight );

	FMB_SET_PARAM_BITS ( pCmdInfo->param[11], PAN_SCAN_RECT_TOP,               pCommand->panScanRectTop );
	FMB_SET_PARAM_BITS ( pCmdInfo->param[11], PAN_SCAN_RECT_BOTTOM,            pCommand->panScanRectBottom );

	FMB_SET_PARAM_BITS ( pCmdInfo->param[12], VIDEO_SIGNAL_TYPE,               pCommand->videoSignalType );
	FMB_SET_PARAM_BITS ( pCmdInfo->param[12], VIDEO_FORMAT,                    pCommand->videoFormat );
	FMB_SET_PARAM_BITS ( pCmdInfo->param[12], VIDEO_FULL_RANGE_FLAG,           pCommand->videoFullRangeFlag );
	FMB_SET_PARAM_BITS ( pCmdInfo->param[12], COLOUR_DESCRIPTION_PRESENT_FLAG, pCommand->colourDescriptionPresentFlag );
	FMB_SET_PARAM_BITS ( pCmdInfo->param[12], COLOUR_PRIMARIES,                pCommand->colourPrimaries );
	FMB_SET_PARAM_BITS ( pCmdInfo->param[12], TRANSFER_CHARACTERISTICS,        pCommand->transferCharacteristics );

	FMB_SET_PARAM_BITS ( pCmdInfo->param[13], MATRIX_COEFFS,                   pCommand->matrixCoeffs );

	FMB_SET_PARAM_BITS ( pCmdInfo->param[14], ASPECT_RATIO_IDC,                pCommand->aspectRatioIdc );

	FMB_SET_PARAM_BITS ( pCmdInfo->param[15], SAR_WIDTH,                       pCommand->sarWidth );
	FMB_SET_PARAM_BITS ( pCmdInfo->param[15], SAR_HEIGHT,                      pCommand->sarHeight );

	return NTV2_STATUS_SUCCESS;
}

Ntv2Status hevcChangeFrameRate(HevcDeviceCommand* pCommand, HevcCommandInfo* pCmdInfo)
{
	if ((pCommand == NULL) || (pCmdInfo == NULL))  return NTV2_STATUS_BAD_PARAMETER;

	pCmdInfo->param[0] |= FMB_CHANGE_PARAM_FRAME_RATE;
	pCmdInfo->param[6]  = pCommand->seqEndPicNumber;
	pCmdInfo->param[16] = pCommand->frameRateCode;

	return NTV2_STATUS_SUCCESS;
}

Ntv2Status hevcVideoTransfer(uint32_t devNum, HevcMessageTransfer* pMessage)
{
	HevcModuleParams* pModParams = hevcGetModuleParams();
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	HevcTransferData* pTransfer;
	Ntv2Status status = 0;

	if(pDevParams == NULL) return NTV2_STATUS_NO_DEVICE;
	if(!hevcIsDeviceAlive(devNum)) return NTV2_STATUS_BAD_STATE;
	if(pMessage->header.type != Hevc_MessageId_Transfer) return NTV2_STATUS_BAD_PARAMETER;
	if(pMessage->header.size != sizeof(HevcMessageTransfer)) return NTV2_STATUS_BAD_PARAMETER;
	if(pDevParams->deviceMode != Hevc_DeviceMode_Codec) return NTV2_STATUS_BAD_PARAMETER;
	pTransfer = &pMessage->data;

	// verify transfer of a stream we know about
	if((pTransfer->streamType != Hevc_Stream_VideoRaw) && 
	   (pTransfer->streamType != Hevc_Stream_VideoEnc)) 
	{
		HEVC_MSG_STREAM_ERROR("%s(%d): hevcDeviceStream()  *error* bad stream type %d\n",
							  pModParams->pModuleName, devNum, pTransfer->streamType);
		return NTV2_STATUS_BAD_PARAMETER;
	}

	// verify stream id
	if(pTransfer->streamId >= FMB_STREAM_ID_MAX)
	{
		HEVC_MSG_STREAM_ERROR("%s(%d): hevcDeviceStream()  *error* bad stream id %d\n",
							  pModParams->pModuleName, devNum, pTransfer->streamId);
		return NTV2_STATUS_BAD_PARAMETER;
	}

	// choose correct transfer method
	switch(pTransfer->streamType)
	{
	case Hevc_Stream_VideoRaw:
		status = hevcVideoTransferRaw(devNum, pTransfer);
		break;
	case Hevc_Stream_VideoEnc:
		status = hevcVideoTransferEnc(devNum, pTransfer);
		break;
	default:
		status = NTV2_STATUS_BAD_PARAMETER;
	}

	// report back status
	if(status == 0)
	{
		pMessage->header.status = HEVC_STATUS_SUCCESS;
	}
	
	return status;
}

static bool hevcIsAccessibleAddress(uint32_t devNum, uint32_t address, uint32_t size)
{
	HevcModuleParams* pModParams = hevcGetModuleParams();
	uint32_t mainState;
	uint32_t endAddr;

	// calculate last address
	endAddr = address + size - 1U;

	mainState = hevcRegRead(devNum, FMB_REG_MAIN_STATE);

	if((mainState == FMB_MAIN_STATE_ENCODE) || (mainState == FMB_MAIN_STATE_ERROR)) return true;

	if(((address >= FMB_REG_ENCODE_WORK_BASE) && (address <= FMB_REG_ENCODE_WORK_END)) ||
	   ((endAddr >= FMB_REG_ENCODE_WORK_BASE) && (endAddr <= FMB_REG_ENCODE_WORK_END)) ||
	   ((address < FMB_REG_ENCODE_WORK_BASE) && (endAddr > FMB_REG_ENCODE_WORK_END))) 
	{
		HEVC_MSG_REGISTER_ERROR("%s(%d): hevcIsAccessibleAddress()  address not accessible 0x%08x  size 0x%08x  main state %d\n",
								pModParams->pModuleName, devNum, address, size, mainState);
		return false;
	}

	return true;
}

static Ntv2Status hevcDeviceReset(uint32_t devNum)
{
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	Ntv2Status status = 0;

	if(pDevParams == NULL) return NTV2_STATUS_NO_DEVICE;

	// restart mcpu
	if(!hevcRestartFirmware(devNum))
	{
		status = NTV2_STATUS_TIMEOUT;
	}

	// reset cached encode mode
	pDevParams->encodeMode = Hevc_EncodeMode_Unknown;
	pDevParams->firmwareType = Hevc_FirmwareType_Unknown;

	// reset cached vif state
	memset(pDevParams->vifState, 0, sizeof(HevcVifState)*HEVC_STREAM_MAX);

	// reset cached gpio state
	memset(pDevParams->gpioState, 0, sizeof(HevcGpioState)*HEVC_GPIO_MAX);

	return status;
}

static Ntv2Status hevcVideoTransferRaw(uint32_t devNum, HevcTransferData* pTransfer)
{
	HevcModuleParams* pModParams = hevcGetModuleParams();
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	HevcStreamInfo strInfo;
	HevcStreamBufferGroup* pBufferGroup;
	HevcStreamTask* pStrTask;
	HevcFrameData frameData;
	uint32_t streamType;
	uint32_t streamId;
	Ntv2Status status = 0;

	if(pDevParams == NULL) return NTV2_STATUS_NO_DEVICE;

	// set stream type and id
	streamType = FMB_STREAM_TYPE_VEI;
	streamId = pTransfer->streamId;

	HEVC_MSG_STREAM_INFO("%s(%d): hevcDeviceTransferRaw()  transfer start  stream id %d  video buffer 0x%llx  size %d  pic buffer 0x%llx  size %d\n",
						 pModParams->pModuleName, devNum, streamId, pTransfer->videoBuffer, (int)pTransfer->videoDataSize,
						 pTransfer->infoBuffer, (int)pTransfer->infoDataSize);

	// allocate a bounce buffer
	pBufferGroup = hevcStreamAssignBuffer(devNum, streamType, streamId);
	if(pBufferGroup == NULL)
	{
		HEVC_MSG_STREAM_ERROR("%s(%d): hevcDeviceTransferRaw()  *error* no bounce buffer available\n",
							  pModParams->pModuleName, devNum);
		hevcStreamOrphan(devNum, streamType);
		return NTV2_STATUS_BUSY;
	}
	pBufferGroup->videoDataSize = 0;
	pBufferGroup->infoDataSize = 0;

	// copy the video data
	hevcStreamCopyVeiBuffer(devNum, pBufferGroup, pTransfer);

	// queue the dma
	strInfo.streamType = streamType;
	strInfo.streamId = streamId;
	strInfo.pBufferGroup = pBufferGroup;
	strInfo.isLastFrame = (pTransfer->flags & HEVC_TRANSFER_FLAG_IS_LAST_FRAME) != 0;

	pStrTask = hevcStreamEnqueue(devNum, &strInfo);
	if(pStrTask == NULL)
	{
		hevcStreamReleaseBuffer(devNum, pBufferGroup);
		hevcStreamOrphan(devNum, streamType);
		return NTV2_STATUS_BUSY;
	}

	// wait for dma complete ack and check result
	if(!hevcStreamAckWait(devNum, pStrTask) ||
	   (pStrTask->ackInfo.result0 != FMB_DMA_RESULT_OK))
	{
		// dma failed
		status = NTV2_STATUS_IO_ERROR;
	}

	
	if((status == 0) && (pTransfer->videoDataSize > 0))
	{
		memset(&frameData, 0, sizeof(frameData));
		frameData.streamId = streamId;
		frameData.encodeTime = ntv2Time100ns();
		hevcStreamFrameReady(devNum, &frameData);
		pTransfer->encodeTime = frameData.encodeTime;
	}

	// if successful record stats
	if(status == 0)
	{
		hevcStreamTransferStats(devNum, streamType, streamId, pBufferGroup->videoDataSize);
	}

	// api is done
	hevcStreamDequeue(devNum, pStrTask);

	// bounce buffer released by stream when dma complete
	if(status != 0)
	{
		// release bounce buffer here if dma fails
		hevcStreamReleaseBuffer(devNum, pBufferGroup);
	}

	// check for orphan tasks
	hevcStreamOrphan(devNum, streamType);

	return status;
}

static Ntv2Status hevcVideoTransferEnc(uint32_t devNum, HevcTransferData* pTransfer)
{
	HevcModuleParams* pModParams = hevcGetModuleParams();
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	HevcStreamInfo strInfo;
	HevcStreamBufferGroup* pBufferGroup;
	HevcStreamTask* pStrTask;
	HevcFrameData frameData;
	uint32_t streamType;
	uint32_t streamId;
	Ntv2Status status = 0;

	if(pDevParams == NULL) return NTV2_STATUS_NO_DEVICE;

	streamType = FMB_STREAM_TYPE_SEO;
	streamId = pTransfer->streamId;

	// no data so far
	pTransfer->videoDataSize = 0;
	pTransfer->infoDataSize = 0;

	// allocate a bounce buffer
	pBufferGroup = hevcStreamAssignBuffer(devNum, streamType, streamId);
	if(pBufferGroup == NULL)
	{
		HEVC_MSG_STREAM_ERROR("%s(%d): hevcDeviceTransferEnc()  *error* no bounce buffer available  stream id %d\n",
							  pModParams->pModuleName, devNum, streamId);
		hevcStreamOrphan(devNum, streamType);
		return NTV2_STATUS_BUSY;
	}
	pBufferGroup->videoDataSize = 0;
	pBufferGroup->infoDataSize = 0;

	// queue the dma
	strInfo.streamType = streamType;
	strInfo.streamId = streamId;
	strInfo.pBufferGroup = pBufferGroup;
	strInfo.isLastFrame = false;

	pStrTask = hevcStreamEnqueue(devNum, &strInfo);
	if(pStrTask == NULL)
	{
		hevcStreamReleaseBuffer(devNum, pBufferGroup);
		hevcStreamOrphan(devNum, streamType);
		return NTV2_STATUS_BUSY;
	}

	// wait for dma complete message and check result
	if(!hevcStreamMsgWait(devNum, pStrTask) ||
	   (pStrTask->msgInfo.result0 != FMB_DMA_RESULT_OK))
	{
		// dma failed
		status = NTV2_STATUS_IO_ERROR;
	}

	// return size of video and info data
	if(status == 0)
	{
		hevcStreamTransferStats(devNum, streamType, streamId, pBufferGroup->videoDataSize);
	}
	pTransfer->flags |= pStrTask->strInfo.isLastFrame? HEVC_TRANSFER_FLAG_IS_LAST_FRAME : 0;

	// remove from queue
	hevcStreamDequeue(devNum, pStrTask);

	if(status == 0)
	{
		// copy the video and info data
		hevcStreamCopySeoBuffer(devNum, pBufferGroup, pTransfer);
	}

	if(status == 0)
	{
		memset(&frameData, 0, sizeof(frameData));
		frameData.streamId = streamId;
		hevcStreamFrameDone(devNum, &frameData);
		pTransfer->encodeTime = frameData.encodeTime;
		HEVC_MSG_STREAM_FRAME("%s(%d): hevcDeviceTransferEnc()  stream id %d  serial %d  itc %02x:%08x  ext %08x  type %d\n",
							  pModParams->pModuleName, devNum, streamId,
							  *(uint32_t*)(pBufferGroup->infoBuffer.pBufferAddress + 0x00),
							  *(uint32_t*)(pBufferGroup->infoBuffer.pBufferAddress + 0x24),
							  *(uint32_t*)(pBufferGroup->infoBuffer.pBufferAddress + 0x20),
							  *(uint32_t*)(pBufferGroup->infoBuffer.pBufferAddress + 0x28),
							  *(uint32_t*)(pBufferGroup->infoBuffer.pBufferAddress + 0x30));
	}

	// release buffer
	hevcStreamReleaseBuffer(devNum, pBufferGroup);

	HEVC_MSG_STREAM_INFO("%s(%d): hevcDeviceTransferEnc()  transfer complete  stream id %d  video buffer 0x%llx  size %d  es buffer 0x%llx  size %d\n",
						 pModParams->pModuleName, devNum, streamId, pTransfer->videoBuffer, (int)pTransfer->videoDataSize,
						 pTransfer->infoBuffer, (int)pTransfer->infoDataSize);

	// check for orphan tasks
	hevcStreamOrphan(devNum, streamType);

	return status;
}

Ntv2Status hevcGetStatus(uint32_t devNum, HevcMessageStatus* pMessage)
{
	HevcModuleParams* pModParams = hevcGetModuleParams();
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	HevcDeviceStatus* pStatus;
	uint32_t is;

	if(pDevParams == NULL) return NTV2_STATUS_NO_DEVICE;
	if(pMessage == NULL) return NTV2_STATUS_BAD_PARAMETER;
	if(pMessage->header.type != Hevc_MessageId_Status) return NTV2_STATUS_BAD_PARAMETER;
	if(pMessage->header.size != sizeof(HevcMessageStatus)) return NTV2_STATUS_BAD_PARAMETER;
	if(pDevParams->deviceMode != Hevc_DeviceMode_Codec) return NTV2_STATUS_BAD_PARAMETER;
	pStatus = &pMessage->data;

	// get main state
	pStatus->mainState = hevcGetMainState(hevcRegRead(devNum, FMB_REG_MAIN_STATE));
	pStatus->encodeMode = hevcGetEncodeMode(pDevParams->encodeMode);
	pStatus->firmwareType = hevcGetFirmwareType(pDevParams->firmwareType);

	// get stream state
	for(is = 0; is < HEVC_STREAM_MAX; is++)
	{
		pStatus->vifState[is] = pDevParams->vifState[is];
		pStatus->vinState[is] = hevcGetVinState(hevcRegRead(devNum, FMB_REG_VIN_STATE(is)));
		pStatus->ehState[is] = hevcGetEhState(hevcRegRead(devNum, FMB_REG_EH_STATE(is)));
	}

	// get gpio state
	for(is = 0; is < HEVC_GPIO_MAX; is++)
	{
		pStatus->gpioState[is] = pDevParams->gpioState[is];
	}

	// get command and stream queue counts
	pStatus->commandCount = pDevParams->cmdQueueCount;
	pStatus->rawTransferCount = pDevParams->strQueueCount[FMB_STREAM_TYPE_VEI];
	pStatus->encTransferCount = pDevParams->strQueueCount[FMB_STREAM_TYPE_SEO];

	// get command and stream queue levels
	pStatus->commandQueueLevel = pDevParams->cmdQueueLevel;
	pStatus->rawTransferQueueLevel = pDevParams->strQueueLevel[FMB_STREAM_TYPE_VEI];
	pStatus->encTransferQueueLevel = pDevParams->strQueueLevel[FMB_STREAM_TYPE_SEO];

	HEVC_MSG_STATUS_INFO("%s(%d): hevcDeviceStatus()  main state %d  count  cmd %lld  raw %lld  enc %lld\n",
						 pModParams->pModuleName, devNum,
						 pStatus->mainState, pStatus->commandCount, pStatus->rawTransferCount, pStatus->encTransferCount);

	return NTV2_STATUS_SUCCESS;
}

Ntv2Status hevcDebugInfo(uint32_t devNum, HevcMessageDebug* pMessage)
{
	HevcModuleParams* pModParams = hevcGetModuleParams();
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	HevcDeviceDebug* pDebug;
	uint32_t is;

	if(pDevParams == NULL) return NTV2_STATUS_NO_DEVICE;
	if(pMessage == NULL) return NTV2_STATUS_BAD_PARAMETER;
	if(pMessage->header.type != Hevc_MessageId_Debug) return NTV2_STATUS_BAD_PARAMETER;
	if(pMessage->header.size != sizeof(HevcMessageDebug)) return NTV2_STATUS_BAD_PARAMETER;
	if(pDevParams->deviceMode != Hevc_DeviceMode_Codec) return NTV2_STATUS_BAD_PARAMETER;
	pDebug = &pMessage->data;

	// get main state
	pDebug->deviceStatus.mainState = hevcGetMainState(hevcRegRead(devNum, FMB_REG_MAIN_STATE));
	pDebug->deviceStatus.encodeMode = hevcGetEncodeMode(pDevParams->encodeMode);
	pDebug->deviceStatus.firmwareType = hevcGetFirmwareType(pDevParams->firmwareType);

	// get stream state
	for(is = 0; is < HEVC_STREAM_MAX; is++)
	{
		pDebug->deviceStatus.vinState[is] = hevcGetVinState(hevcRegRead(devNum, FMB_REG_VIN_STATE(is)));
		pDebug->deviceStatus.ehState[is] = hevcGetEhState(hevcRegRead(devNum, FMB_REG_EH_STATE(is)));
	}

	// get command and stream queue counts
	pDebug->deviceStatus.commandCount = pDevParams->cmdQueueCount;
	pDebug->deviceStatus.rawTransferCount = pDevParams->strQueueCount[FMB_STREAM_TYPE_VEI];
	pDebug->deviceStatus.encTransferCount = pDevParams->strQueueCount[FMB_STREAM_TYPE_SEO];

	// get command and stream queue levels
	pDebug->deviceStatus.commandQueueLevel = pDevParams->cmdQueueLevel;
	pDebug->deviceStatus.rawTransferQueueLevel = pDevParams->strQueueLevel[FMB_STREAM_TYPE_VEI];
	pDebug->deviceStatus.encTransferQueueLevel = pDevParams->strQueueLevel[FMB_STREAM_TYPE_SEO];

	// get stream statistics
	for(is = 0; is < HEVC_STREAM_MAX; is++)
	{
		hevcStreamGetStats(devNum, FMB_STREAM_TYPE_VEI, is, &pDebug->rawStats[is]);
		hevcStreamGetStats(devNum, FMB_STREAM_TYPE_SEO, is, &pDebug->encStats[is]);
		pDebug->queueLevel[is] = hevcStreamGetFrameQueueLevel(devNum, is);

		hevcStreamClearStats(devNum, FMB_STREAM_TYPE_VEI, pDebug->clearRawStatsBits);
		hevcStreamClearStats(devNum, FMB_STREAM_TYPE_SEO, pDebug->clearEncStatsBits);
	}

	// get command queue statistics
	pDebug->cmdContCount = hevcRegRead(devNum, FMB_REG_COMMAND_CONTINUITY_CNT);
	pDebug->cmdAckContCount = hevcRegRead(devNum, FMB_REG_COMMAND_ACK_CONTINUITY_CNT);
	pDebug->cmdMsgContCount = hevcRegRead(devNum, FMB_REG_COMMAND_RESULT_CONTINUITY_CNT);

	// get raw video queue statistics
	pDebug->rawContCount = hevcRegRead(devNum, FMB_REG_DMA_VEI_CONTINUITY_CNT);
	pDebug->rawAckContCount = hevcRegRead(devNum, FMB_REG_DMA_VEI_ACK_CONTINUITY_CNT);
	pDebug->rawMsgContCount = hevcRegRead(devNum, FMB_REG_DMA_VEI_COMPLETE_CONTINUITY_CNT);

	// get encoded video queue statistics
	pDebug->encContCount = hevcRegRead(devNum, FMB_REG_DMA_SEO_CONTINUITY_CNT);
	pDebug->encAckContCount = hevcRegRead(devNum, FMB_REG_DMA_SEO_ACK_CONTINUITY_CNT);
	pDebug->encMsgContCount = hevcRegRead(devNum, FMB_REG_DMA_SEO_COMPLETE_CONTINUITY_CNT);

	HEVC_MSG_STATUS_INFO("%s(%d): hevcDeviceDebug()  main state %d  count  cmd %d  vei %d  seo %d\n",
						 pModParams->pModuleName, devNum,
						 pDebug->deviceStatus.mainState, pDebug->cmdContCount, pDebug->rawContCount, pDebug->encContCount);

	return NTV2_STATUS_SUCCESS;
}

uint32_t hevcGetNumDevices(void)
{
	return hevcGetMaxDevices();
}

Ntv2SystemContext* hevcGetSystemContext(uint32_t devNum)
{
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);

	if(pDevParams == NULL) return NULL;

	return &pDevParams->systemContext;
}

bool hevcIsCodecMode(uint32_t devNum)
{
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);

	if(pDevParams == NULL) return false;
	if(pDevParams->deviceMode != Hevc_DeviceMode_Codec) return false;

	return true;
}

Ntv2Status hevcSetGpio(uint32_t devNum, uint32_t gpioNum, bool output, bool high)
{
	HevcModuleParams* pModParams = hevcGetModuleParams();
	HevcDeviceParams* pDevParams = hevcGetDeviceParams(devNum);
	HevcMessageCommand hevcCommand;
	HevcDeviceCommand* pCommand;
	HevcGpioDirection direction = output? Hevc_GpioDirection_Output : Hevc_GpioDirection_Input;
	HevcGpioValue value = high? Hevc_GpioValue_High : Hevc_GpioValue_Low;
	Ntv2Status status;

	if(pDevParams == NULL) return false;
	if(gpioNum >= HEVC_GPIO_MAX) return false;

	hevcCommand.header.type = Hevc_MessageId_Command;
	hevcCommand.header.size = sizeof(HevcMessageCommand);
	pCommand = &hevcCommand.data;
	
    // initialize gpio function to gpio
	if(pDevParams->gpioState[gpioNum].function != Hevc_GpioFunction_Gpio)
	{
		memset(pCommand, 0, sizeof(HevcDeviceCommand));
		pCommand->command = Hevc_Command_Gpio;
		pCommand->gpioControl = Hevc_GpioControl_Function;
		pCommand->gpioNumber = gpioNum;
		pCommand->gpioFunction = Hevc_GpioFunction_Gpio;
		status = hevcSendCommand(devNum, &hevcCommand);
		if (status != NTV2_STATUS_SUCCESS)
		{
			HEVC_MSG_ERROR("%s(%d): hevcSetGpio()  gio set function failed %08x\n", pModParams->pModuleName, devNum, status);
			return NTV2_STATUS_FAIL;
		}
	}

	// set gpio direction
	if(pDevParams->gpioState[gpioNum].direction != direction)
	{
		memset(pCommand, 0, sizeof(HevcDeviceCommand));
		pCommand->command = Hevc_Command_Gpio;
		pCommand->gpioControl = Hevc_GpioControl_Direction;
		pCommand->gpioNumber = gpioNum;
		pCommand->gpioDirection = direction;
		status = hevcSendCommand(devNum, &hevcCommand);
		if (status != NTV2_STATUS_SUCCESS)
		{
			HEVC_MSG_ERROR("%s(%d): hevcSetGpio()  gio set direction failed %08x\n", pModParams->pModuleName, devNum, status);
			return NTV2_STATUS_FAIL;
		}
	}

	// set gpio value
	if(pDevParams->gpioState[gpioNum].setValue != value)
	{
		memset(pCommand, 0, sizeof(HevcDeviceCommand));
		pCommand->command = Hevc_Command_Gpio;
		pCommand->gpioControl = Hevc_GpioControl_Set;
		pCommand->gpioNumber = gpioNum;
		pCommand->gpioValue = value;
		status = hevcSendCommand(devNum, &hevcCommand);
		if (status != NTV2_STATUS_SUCCESS)
		{
			HEVC_MSG_ERROR("%s(%d): hevcSetGpio()  gio set value failed %08x\n", pModParams->pModuleName, devNum, status);
			return NTV2_STATUS_FAIL;
		}
	}

	return NTV2_STATUS_SUCCESS;
}

static HevcMainState hevcGetMainState(uint32_t state)
{
	HevcMainState mainState = Hevc_MainState_Unknown;

	// interpret hardware main state
	switch(state)
	{
	case FMB_MAIN_STATE_BOOT:
		mainState = Hevc_MainState_Boot;
		break;
	case FMB_MAIN_STATE_INIT:
		mainState = Hevc_MainState_Init;
		break;
	case FMB_MAIN_STATE_ENCODE:
		mainState = Hevc_MainState_Encode;
		break;
	case FMB_MAIN_STATE_ERROR:
		mainState = Hevc_MainState_Error;
		break;
	default:
		break;
	}

	return mainState;
}

static HevcEncodeMode hevcGetEncodeMode(uint32_t mode)
{
	HevcEncodeMode encodeMode = Hevc_EncodeMode_Unknown;

	// interpret hardware encode mode
	switch(mode)
	{
	case FMB_ENCODE_MODE_SINGLE_CH:
		encodeMode = Hevc_EncodeMode_Single;
		break;
	case FMB_ENCODE_MODE_MULTI_CH:
		encodeMode = Hevc_EncodeMode_Multiple;
		break;
	default:
		break;
	}

	return encodeMode;
}

static HevcFirmwareType hevcGetFirmwareType(uint32_t type)
{
	HevcFirmwareType firmwareType = Hevc_FirmwareType_Unknown;

	// interpret hardware firmware type
	switch(type)
	{
	case FMB_HEVC_ENCODER_FIRM_STD:
	    firmwareType = Hevc_FirmwareType_Standard;
		break;
	case FMB_HEVC_ENCODER_FIRM_USER:
		firmwareType = Hevc_FirmwareType_User;
		break;
	default:
		break;
	}

	return firmwareType;
}

static HevcVinState hevcGetVinState(uint32_t state)
{
	HevcVinState vinState = Hevc_VinState_Unknown;

	// interpret vin state
	switch(state)
	{
	case FMB_VIN_ID_STATE_STOP:
		vinState = Hevc_VinState_Stop;
		break;
	case FMB_VIN_ID_STATE_START:
		vinState = Hevc_VinState_Start;
		break;
	default:
		break;
	}

	return vinState;
}

static HevcEhState hevcGetEhState(uint32_t state)
{
	HevcEhState ehState = Hevc_EhState_Unknown;

	// interpret eh state
	switch(state)
	{
	case FMB_EH_ID_STATE_STOP:
		ehState = Hevc_EhState_Stop;
		break;
	case FMB_EH_ID_STATE_START:
		ehState = Hevc_EhState_Start;
		break;
	case FMB_EH_ID_STATE_READY_TO_STOP:
		ehState = Hevc_EhState_ReadyToStop;
		break;
	default:
		break;
	}

	return ehState;
}

