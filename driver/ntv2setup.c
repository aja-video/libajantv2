/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
//==========================================================================
//
//  ntv2setup.c
//
//==========================================================================

#include "ntv2setup.h"
#include "ntv2commonreg.h"
#include "ntv2kona.h"

/* debug messages */
#define NTV2_DEBUG_INFO					0x00000001
#define NTV2_DEBUG_ERROR				0x00000002

#define NTV2_DEBUG_ACTIVE(msg_mask) \
	((ntv2_debug_mask & msg_mask) != 0)

#define NTV2_MSG_PRINT(msg_mask, string, ...) \
	if(NTV2_DEBUG_ACTIVE(msg_mask)) ntv2Message(string, __VA_ARGS__);

#define NTV2_MSG_INFO(string, ...)					NTV2_MSG_PRINT(NTV2_DEBUG_INFO, string, __VA_ARGS__)
#define NTV2_MSG_SETUP_INFO(string, ...)			NTV2_MSG_PRINT(NTV2_DEBUG_INFO, string, __VA_ARGS__)

static uint32_t ntv2_debug_mask = NTV2_DEBUG_INFO | NTV2_DEBUG_ERROR;
static const int64_t c_default_timeout = 50000;

static void ntv2_setup_monitor(void* data);
static bool ntv2_set_fan_speed(Ntv2SystemContext* sys_con, NTV2FanSpeed fanSpeed);

struct ntv2_setup *ntv2_setup_open(Ntv2SystemContext* sys_con, const char *name)
{
	struct ntv2_setup *ntv2_setterupper = NULL;

	if ((sys_con == NULL) ||
		(name == NULL))
		return NULL;

	ntv2_setterupper = (struct ntv2_setup *)ntv2MemoryAlloc(sizeof(struct ntv2_setup));
	if (ntv2_setterupper == NULL) {
		NTV2_MSG_SETUP_INFO("%s: ntv2_genlock instance memory allocation failed\n", name);
		return NULL;
	}
	memset(ntv2_setterupper, 0, sizeof(struct ntv2_setup));

#if defined(MSWindows)
	sprintf(ntv2_setterupper->name, "%s", name);
#else
	snprintf(ntv2_setterupper->name, NTV2_SETUP_STRING_SIZE, "%s", name);
#endif
	ntv2_setterupper->system_context = sys_con;

	ntv2SpinLockOpen(&ntv2_setterupper->state_lock, sys_con);
	ntv2ThreadOpen(&ntv2_setterupper->monitor_task, sys_con, "output monitor");
	ntv2EventOpen(&ntv2_setterupper->monitor_event, sys_con);

	NTV2_MSG_SETUP_INFO("%s: open ntv2_setup\n", ntv2_setterupper->name);

	return ntv2_setterupper;
}

void ntv2_setup_close(struct ntv2_setup *ntv2_setterupper)
{
	if (ntv2_setterupper == NULL)
		return;

	NTV2_MSG_SETUP_INFO("%s: close ntv2_setup\n", ntv2_setterupper->name);

	ntv2_setup_disable(ntv2_setterupper);

	ntv2EventClose(&ntv2_setterupper->monitor_event);
	ntv2ThreadClose(&ntv2_setterupper->monitor_task);
	ntv2SpinLockClose(&ntv2_setterupper->state_lock);

	memset(ntv2_setterupper, 0, sizeof(struct ntv2_setup));
	ntv2MemoryFree(ntv2_setterupper, sizeof(struct ntv2_setup));
}

Ntv2Status ntv2_setup_configure(struct ntv2_setup *ntv2_setterupper)
{
	if (ntv2_setterupper == NULL)
		return NTV2_STATUS_BAD_PARAMETER;

	NTV2_MSG_SETUP_INFO("%s: configure output setup\n", ntv2_setterupper->name);

	return NTV2_STATUS_SUCCESS;
}

Ntv2Status ntv2_setup_enable(struct ntv2_setup *ntv2_setterupper)
{
	bool success ;

	if (ntv2_setterupper == NULL)
		return NTV2_STATUS_BAD_PARAMETER;

	if (ntv2_setterupper->monitor_enable)
		return NTV2_STATUS_SUCCESS;

	NTV2_MSG_SETUP_INFO("%s: enable output monitor\n", ntv2_setterupper->name);

	ntv2EventClear(&ntv2_setterupper->monitor_event);
	ntv2_setterupper->monitor_enable = true;

	success = ntv2ThreadRun(&ntv2_setterupper->monitor_task, ntv2_setup_monitor, (void*)ntv2_setterupper);
	if (!success) {
		return NTV2_STATUS_FAIL;
	}

	return NTV2_STATUS_SUCCESS;
}

Ntv2Status ntv2_setup_disable(struct ntv2_setup *ntv2_setterupper)
{
	if (ntv2_setterupper == NULL)
		return NTV2_STATUS_BAD_PARAMETER;

	if (!ntv2_setterupper->monitor_enable)
		return NTV2_STATUS_SUCCESS;

	NTV2_MSG_SETUP_INFO("%s: disable output monitor\n", ntv2_setterupper->name);

	ntv2_setterupper->monitor_enable = false;
	ntv2EventSignal(&ntv2_setterupper->monitor_event);

	ntv2ThreadStop(&ntv2_setterupper->monitor_task);

	return NTV2_STATUS_SUCCESS;
}

static void ntv2_setup_monitor(void* data)
{
	struct ntv2_setup *ntv2_setterupper = (struct ntv2_setup *)data;
	Ntv2SystemContext *systemContext;
	NTV2DeviceID deviceID;
	ULWord thermalSamplingCount = 100;
	ULWord lastTemp = 0;
	bool isSXGA = false;
	ULWord HDMIDirectBit = 0;
	uint32_t i = 0;

	if (ntv2_setterupper == NULL)
		return;

	NTV2_MSG_SETUP_INFO("%s: output monitor task start\n", ntv2_setterupper->name);

	systemContext = ntv2_setterupper->system_context;
	deviceID = (NTV2DeviceID)ntv2ReadRegister(systemContext, kRegBoardID);
	while (!ntv2ThreadShouldStop(&ntv2_setterupper->monitor_task) && ntv2_setterupper->monitor_enable)
	{
		if (ntv2ReadVirtualRegister(systemContext, kVRegEveryFrameTaskFilter) != NTV2_DISABLE_TASKS)
		{
			switch (NTV2DeviceGetNumVideoOutputs(deviceID))
			{
			case 8:
				SetVideoOutputStandard(systemContext, NTV2_CHANNEL8);
				// fallthrough
			case 7:
				SetVideoOutputStandard(systemContext, NTV2_CHANNEL7);
				// fallthrough
			case 6:
				SetVideoOutputStandard(systemContext, NTV2_CHANNEL6);
				// fallthrough
			case 5:
				SetVideoOutputStandard(systemContext, NTV2_CHANNEL5);
				// fallthrough
			case 4:
				SetVideoOutputStandard(systemContext, NTV2_CHANNEL4);
				// fallthrough
			case 3:
				SetVideoOutputStandard(systemContext, NTV2_CHANNEL3);
				// fallthrough
			case 2:
				SetVideoOutputStandard(systemContext, NTV2_CHANNEL2);
				// fallthrough
			default:
				SetVideoOutputStandard(systemContext, NTV2_CHANNEL1);
				break;
			}

			if (NTV2DeviceHasRotaryEncoder(deviceID))
			{
				UpdateAudioMixerGainFromRotaryEncoder(systemContext);
			}

			if (NTV2DeviceCanDoWidget(deviceID, NTV2_WgtSDIMonOut1))
			{
				//Dual link out 5 should follow ch1 for boards that do not have 5 channels
				//Otherwise the duallink setup follows fbf for respective channel
				NTV2FrameBufferFormat fbf = GetFrameBufferFormat(systemContext, NTV2_CHANNEL1);
				SetDualLink5PixelFormat(systemContext, fbf);
				SetVideoOutputStandard(systemContext, NTV2_CHANNEL5);
			}

			if (NTV2GetDACVersion(deviceID) == 2)
			{
				SetLHiAnalogOutputStandard(systemContext);
			}

			if (NTV2DeviceGetNumHDMIVideoOutputs(deviceID) > 0)
			{
				SetHDMIOutputStandard(systemContext);
			}

			for (i = 0; i < NTV2DeviceGetNumVideoChannels(deviceID); i++)
			{
				Set2piCSC(systemContext, (NTV2Channel)i, Get425FrameEnable(systemContext, (NTV2Channel)i));
			}
		}

		if (NTV2DeviceCanThermostat(deviceID) && ntv2ReadVirtualRegister(systemContext, kVRegUseThermostat) == 1)
		{
			ULWord dieTemp = 0x0;
			NTV2FanSpeed fanSpeed = (NTV2FanSpeed)ntv2ReadVirtualRegister(systemContext, kVRegFanSpeed);

			ntv2ReadRegisterMS(systemContext, kRegSysmonVccIntDieTemp, &dieTemp, 0xFFC0, 6);

			if (thermalSamplingCount == 0 || dieTemp > lastTemp)
			{
				if (dieTemp < 0x2ce)//78C
				{
					switch (fanSpeed)
					{
					case NTV2_FanSpeed_Medium:
					case NTV2_FanSpeed_High:
						if (dieTemp < 0x2c4)//75C
						{
							ntv2_set_fan_speed(systemContext, NTV2_FanSpeed_Low);
							thermalSamplingCount = 100;
						}
						break;
					default:
						ntv2_set_fan_speed(systemContext, NTV2_FanSpeed_Low);
						thermalSamplingCount = 100;
						break;
					}
				}
				else if (dieTemp >= 0x2ce && dieTemp < 0x2e6)// between 78C and 92C
				{
					switch (fanSpeed)
					{
					default:
					case NTV2_FanSpeed_Low:
					case NTV2_FanSpeed_Medium:
						ntv2_set_fan_speed(systemContext, NTV2_FanSpeed_Medium);
						thermalSamplingCount = 5000;
						break;
					case NTV2_FanSpeed_High:
						if (dieTemp < 0x2d4)
						{
							ntv2_set_fan_speed(systemContext, NTV2_FanSpeed_Medium);
							thermalSamplingCount = 5000;
						}
						break;
					}
				}
				else if (dieTemp >= 0x2e6)// over 92
				{
					ntv2_set_fan_speed(systemContext, NTV2_FanSpeed_High);
					thermalSamplingCount = 5000;
				}
				lastTemp = dieTemp;
			}
			else
			{
				if (thermalSamplingCount > 0)
					thermalSamplingCount--;
			}
		}
		else if (NTV2DeviceCanThermostat(deviceID) && ntv2ReadVirtualRegister(systemContext, kVRegUseThermostat) == 0)
		{
			ntv2_set_fan_speed(systemContext, (NTV2FanSpeed)ntv2ReadVirtualRegister(systemContext, kVRegFanSpeed));
			thermalSamplingCount = 0;
		}

		if (deviceID == DEVICE_ID_KONALHIDVI)
		{
			bool isLocked = true;
			uint32_t status = ntv2ReadRegister(systemContext, kRegHDMIInputStatus);
			ntv2ReadRegisterMS(systemContext, kRegCh1ControlExtended, &HDMIDirectBit, BIT_1, 1);
			if ((status & (BIT_0 | BIT_1)) != (BIT_0 | BIT_1))
			{
				isLocked = false;
			}
			if (isLocked)
			{
				uint32_t standard = ((status & kRegMaskInputStatusStd) >> kRegShiftInputStatusStd);
				if (standard == 0x5)
				{
					isSXGA = true;
				}
			}

			// Check for SXGA source and set FrameBuffer mode to bypass xpt
			if (isSXGA)
			{
				// Lets check the input source to the FrameBuffer
				// If it is the HDMI input then set the SXGA Mode
				NTV2OutputXptID ch1Input;
				NTV2OutputXptID ch2Input;
				GetXptFrameBuffer1InputSelect(systemContext, &ch1Input);
				GetXptFrameBuffer2InputSelect(systemContext, &ch2Input);

				if (ch1Input == NTV2_XptHDMIIn1)
				{
					if (!(ntv2ReadVirtualRegister(systemContext, kVRegDebug1) & BIT_31))
					{
						if (HDMIDirectBit == 0)
						{
							ntv2WriteRegisterMS(systemContext, kRegCh1ControlExtended, 1, BIT(1), 1);
						}
					}
				}
				else
				{
					if (!(ntv2ReadVirtualRegister(systemContext, kVRegDebug1) & BIT_31))
					{
						if (HDMIDirectBit == 1)
						{
							ntv2WriteRegisterMS(systemContext, kRegCh1ControlExtended, 0, BIT(1), 1);
						}
					}
				}

				if (ch2Input == NTV2_XptHDMIIn1)
				{
					if (!(ntv2ReadVirtualRegister(systemContext, kVRegDebug1) & BIT_31))
					{
						ntv2WriteRegisterMS(systemContext, kRegCh2ControlExtended, 1, BIT(1), 1);
					}
				}
				else
				{
					if (!(ntv2ReadVirtualRegister(systemContext, kVRegDebug1) & BIT_31))
					{
						ntv2WriteRegisterMS(systemContext, kRegCh2ControlExtended, 0, BIT(1), 1);
					}
				}
			}
			else
			{
				if (!(ntv2ReadVirtualRegister(systemContext, kVRegDebug1) & BIT_31))
				{
					ntv2WriteRegisterMS(systemContext, kRegCh1ControlExtended, 0, BIT(1), 1);
					ntv2WriteRegisterMS(systemContext, kRegCh2ControlExtended, 0, BIT(1), 1);
				}
			}
		}

		ntv2EventWaitForSignal(&ntv2_setterupper->monitor_event, c_default_timeout, true);
	}

	NTV2_MSG_SETUP_INFO("%s: output monitor task stop\n", ntv2_setterupper->name);
	ntv2ThreadExit(&ntv2_setterupper->monitor_task);
	return;
}

bool ntv2_set_fan_speed(Ntv2SystemContext* sys_con, NTV2FanSpeed fanSpeed)
{
	if (!NTV2DeviceCanThermostat((NTV2DeviceID)ntv2ReadRegister(sys_con, kRegBoardID)))
		return false;
	switch (fanSpeed)
	{
	default:
	case NTV2_FanSpeed_Low:
		ntv2WriteRegisterMS(sys_con, kRegSysmonConfig2, 0x0, kRegFanHiMask, kRegFanHiShift);
		ntv2WriteRegisterMS(sys_con, kRegSysmonConfig2, 0x0, kRegThermalMask, kRegThermalShift);
		ntv2WriteVirtualRegister(sys_con, kVRegFanSpeed, NTV2_FanSpeed_Low);
		break;
	case NTV2_FanSpeed_Medium:
		ntv2WriteRegisterMS(sys_con, kRegSysmonConfig2, 0x0, kRegFanHiMask, kRegFanHiShift);
		ntv2WriteRegisterMS(sys_con, kRegSysmonConfig2, 0x5, kRegThermalMask, kRegThermalShift);
		ntv2WriteVirtualRegister(sys_con, kVRegFanSpeed, NTV2_FanSpeed_Medium);
		break;
	case NTV2_FanSpeed_High:
		ntv2WriteRegisterMS(sys_con, kRegSysmonConfig2, 0x1, kRegFanHiMask, kRegFanHiShift);
		ntv2WriteRegisterMS(sys_con, kRegSysmonConfig2, 0x0, kRegThermalMask, kRegThermalShift);
		ntv2WriteVirtualRegister(sys_con, kVRegFanSpeed, NTV2_FanSpeed_High);
		break;
	}
	return true;
}
