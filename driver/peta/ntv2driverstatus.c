/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
/////////////////////////////////////////////////////////////////////////////
// NTV2 Linux v2.6 Device Driver for AJA OEM devices.
// ntv2driverstatus.c
// 
/////////////////////////////////////////////////////////////////////////////

/*needed by kernel 2.6.18*/
#ifndef CONFIG_HZ
#include <linux/autoconf.h>
#endif

#if defined(CONFIG_SMP)
#define __SMP__
#endif

#include "ajatypes.h"
#include "ntv2enums.h"
#include "ntv2driver.h"
#include "ntv2publicinterface.h"
#include "ntv2linuxpublicinterface.h"

#include "ntv2driverstatus.h"
#include "registerio.h"

void getDeviceVersionString(ULWord deviceNumber, char *deviceVersionString, ULWord strMax)
{
	char *deviceStr = NULL;
	NTV2DeviceID deviceID;

	if (strMax == 0 || deviceVersionString == NULL)
		return;

	strMax--;

	if (strMax == 0)
		goto full;


	deviceID = getDeviceID(deviceNumber);
	
    switch(deviceID) {
	case DEVICE_ID_CORVID1: deviceStr = "CORVID"; break;	
	case DEVICE_ID_KONALHI: deviceStr = "KONALHI"; break;
	case DEVICE_ID_KONALHEPLUS: deviceStr = "KONALHE+"; break;	
	case DEVICE_ID_IOEXPRESS: deviceStr = "IOEXPRESS"; break;
	case DEVICE_ID_CORVID22: deviceStr = "CORVID22"; break;
	case DEVICE_ID_CORVID3G: deviceStr = "CORVID3G"; break;
	case DEVICE_ID_KONA3G: deviceStr = "KONA3G"; break;
	case DEVICE_ID_KONA3GQUAD: deviceStr = "KONA3GQUAD"; break;
	case DEVICE_ID_CORVID24: deviceStr = "CORVID24"; break;
	case DEVICE_ID_IOXT: deviceStr = "IOXT"; break;
	case DEVICE_ID_KONA4: deviceStr = "KONA4"; break;
	case DEVICE_ID_KONA4UFC: deviceStr = "KONA4UFC"; break;
	case DEVICE_ID_CORVID88: deviceStr = "CORVID88"; break;
	
	case DEVICE_ID_CORVID44:
		//	Hack, until the 446 is supported by device features
		{
            if( ReadRegister(deviceNumber, kVRegPCIDeviceID, NO_MASK, NO_SHIFT) == NTV2_DEVICE_ID_CORVID446 )
				deviceStr = "CORVID446";
			else
				deviceStr = "CORVID44";
		}
		break;
	
	case DEVICE_ID_CORVID44_PLNR: deviceStr = "CORVID44_PLNR"; break;
	case DEVICE_ID_CORVIDHBR: deviceStr = "CORVIDHBR"; break;
    case DEVICE_ID_KONAIP_2022: deviceStr = "KONAIP_2022"; break;
    case DEVICE_ID_KONAIP_2110: deviceStr = "KONAIP_2110"; break;
	case DEVICE_ID_KONAIP_2110_RGB12: deviceStr = "KONAIP_2110_RGB12"; break;
    case DEVICE_ID_KONAIP_4CH_2SFP: deviceStr = "KONAIP_4CH"; break;
    case DEVICE_ID_KONAIP_1RX_1TX_1SFP_J2K: deviceStr = "KONAIP_1RX_1TX_J2K"; break;
	case DEVICE_ID_KONAIP_2TX_1SFP_J2K: deviceStr = "KONAIP_2TX_J2K"; break;
	case DEVICE_ID_KONAIP_1RX_1TX_2110: deviceStr = "KONAIP_1RX_1TX_2110"; break;
    case DEVICE_ID_CORVIDHEVC: deviceStr = "CORVIDHEVC"; break;
	case DEVICE_ID_IO4KPLUS: deviceStr = "IO4KPLUS"; break;
	case DEVICE_ID_IOIP_2022: deviceStr = "IOIP_2022"; break;
    case DEVICE_ID_IOIP_2110: deviceStr = "IOIP_2110"; break;
	case DEVICE_ID_KONA1: deviceStr = "KONA1"; break;
	case DEVICE_ID_KONAHDMI: deviceStr = "KONAHDMI"; break;
    case DEVICE_ID_KONA5: deviceStr = "KONA5"; break;
	case DEVICE_ID_KONA5_8KMK: deviceStr = "KONA5_8KMK"; break;
	case DEVICE_ID_KONA5_8K: deviceStr = "KONA5_8K"; break;
	case DEVICE_ID_KONA5_3DLUT: deviceStr = "KONA5_3DLUT"; break;
	case DEVICE_ID_KONA5_8K_MV_TX: deviceStr = "KONA5_8K_MV_TX"; break;
	case DEVICE_ID_KONA5_2X4K: deviceStr = "KONA5_2X4K"; break;
	case DEVICE_ID_KONA5_OE1:
	case DEVICE_ID_KONA5_OE2:
	case DEVICE_ID_KONA5_OE3:
	case DEVICE_ID_KONA5_OE4:
	case DEVICE_ID_KONA5_OE5:
	case DEVICE_ID_KONA5_OE6:
	case DEVICE_ID_KONA5_OE7:
	case DEVICE_ID_KONA5_OE8:
	case DEVICE_ID_KONA5_OE9:
	case DEVICE_ID_KONA5_OE10:
	case DEVICE_ID_KONA5_OE11:
	case DEVICE_ID_KONA5_OE12: deviceStr = "KONA5_OE"; break;
	case DEVICE_ID_SOJI_3DLUT: deviceStr = "SOJI_3DLUT"; break;
	case DEVICE_ID_SOJI_OE1:
	case DEVICE_ID_SOJI_OE2:
	case DEVICE_ID_SOJI_OE3:
	case DEVICE_ID_SOJI_OE4:
	case DEVICE_ID_SOJI_OE5:
	case DEVICE_ID_SOJI_OE6:
	case DEVICE_ID_SOJI_OE7: deviceStr = "SOJI_OE"; break;
	case DEVICE_ID_SOJI_DIAGS: deviceStr = "SOJI_DIAGS"; break;
	case DEVICE_ID_CORVID44_8KMK: deviceStr = "CORVID44_8KMK"; break;
	case DEVICE_ID_CORVID44_8K: deviceStr = "CORVID44_8K"; break;
	case DEVICE_ID_CORVID44_2X4K: deviceStr = "CORVID44_2X4K"; break;
    case DEVICE_ID_KONAX: deviceStr = "KONAX"; break;
    case DEVICE_ID_KONAXM: deviceStr = "KONAXM"; break;
	case DEVICE_ID_IOX3: deviceStr = "IoX3"; break;
    case DEVICE_ID_TTAP_PRO: deviceStr = "TTAP_PRO"; break;
    case DEVICE_ID_KONAIP_25G: deviceStr = "KonaIP_25G"; break;
	
	default:
		deviceStr = "Unknown Device";
		break;
	}

	snprintf(deviceVersionString, strMax, "%s", deviceStr);

full:
	deviceVersionString[strMax] = '\0';
}

void getPCIFPGAVersionString(ULWord deviceNumber, char *pcifpgaVersionString, ULWord strMax)
{
	ULWord bitfileDate = ReadRegister(deviceNumber, kRegBitfileDate, NO_MASK, NO_SHIFT);
	ULWord bitfileTime = ReadRegister(deviceNumber, kRegBitfileTime, NO_MASK, NO_SHIFT);

	if (strMax == 0 || pcifpgaVersionString == NULL)
		return;

	strMax--;
	
	if (strMax == 0)
		goto full;

	snprintf(pcifpgaVersionString, strMax, "%1x%1x%1x%1x/%1x%1x/%1x%1x %1x%1x:%1x%1x:%1x%1x",
			 (bitfileDate >> 28) & 0xf,
			 (bitfileDate >> 24) & 0xf,
			 (bitfileDate >> 20) & 0xf,
			 (bitfileDate >> 16) & 0xf,
			 (bitfileDate >> 12) & 0xf,
			 (bitfileDate >> 8) & 0xf,
			 (bitfileDate >> 4) & 0xf,
			 (bitfileDate >> 0) & 0xf,
			 (bitfileTime >> 20) & 0xf,
			 (bitfileTime >> 16) & 0xf,
			 (bitfileTime >> 12) & 0xf,
			 (bitfileTime >> 8) & 0xf,
			 (bitfileTime >> 4) & 0xf,
			 (bitfileTime >> 0) & 0xf);
full:
	pcifpgaVersionString[strMax] = '\0';
}

NTV2DeviceID getDeviceID(ULWord deviceNumber)
{
	return ReadDeviceIDRegister(deviceNumber);
}

void getDeviceIDString(ULWord deviceNumber, char *deviceIDString, ULWord strMax)
{
	ULWord deviceID = (ULWord)getDeviceID(deviceNumber);
	
	if (strMax == 0 || deviceIDString == NULL)
		return;

	strMax--;

	if (strMax == 0)
		goto full;

	snprintf(deviceIDString, strMax, "%X", deviceID);
full:
	deviceIDString[strMax] = '\0';
}

void getDeviceSerialNumberString(ULWord deviceNumber, char *deviceIDString, ULWord strMax)
{
	ULWord lowId = 0;
	ULWord highId = 0;
	char chr[8];
	int i;

	GetDeviceSerialNumberWords(deviceNumber, &lowId, &highId);
	
	if (strMax == 0 || deviceIDString == NULL)
		return;

	strMax--;

	if (strMax == 0)
		goto full;

	chr[0] = (char)(lowId & 0xFF);
	chr[1] = (char)((lowId >> 8) & 0xFF);
	chr[2] = (char)((lowId >> 16) & 0xFF);
   	chr[3] = (char)((lowId >> 24) & 0xFF);
	chr[4] = (char)(highId & 0xFF);
	chr[5] = (char)((highId >> 8) & 0xFF);
	chr[6] = (char)((highId >> 16) & 0xFF);
	chr[7] = (char)((highId >> 24) & 0xFF);

	for(i = 0; i < 8; i++)
		if ((chr[i] < ' ') || (chr[i] > '~'))
			break;

	if (i == 8)
	{
		snprintf(deviceIDString,
				 strMax, "%c%c%c%c%c%c%c%c",
				 chr[0], chr[1], chr[2], chr[3],
				 chr[4], chr[5], chr[6], chr[7]);
	}
	else
	{
		snprintf(deviceIDString,
				 strMax, "not programmed");
	}
full:
	deviceIDString[strMax] = '\0';
}


void getDriverVersionString(char *driverVersionString, ULWord strMax)
{
   	ULWord strIndex = 0;
	ULWord versionInfo = NTV2_LINUX_DRIVER_VERSION;
	UWord	major = NTV2DriverVersionDecode_Major(versionInfo);
	UWord	minor = NTV2DriverVersionDecode_Minor(versionInfo);
	UWord	point = NTV2DriverVersionDecode_Point(versionInfo);
	UWord	build = NTV2DriverVersionDecode_Build(versionInfo);

	if (strMax == 0 || driverVersionString == NULL)
		return;

	if (--strMax == 0)
	{
		driverVersionString[0] = '\0';	//	Full
		return;
	}

	strIndex = snprintf(driverVersionString, strMax, "%d.%d.%d.%d", major, minor, point, build);
	if (strIndex >= strMax)
		driverVersionString[strMax] = '\0';	//	Full
}
