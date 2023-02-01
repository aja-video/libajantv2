/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
///////////////////////////////////////////////////////////////
//
// NTV2 Linux v2.6 Device Driver for AJA devices.
//
////////////////////////////////////////////////////////////
//
// Filename: ntv2driverstatus.h
// Purpose:	 Header file for getting versions
// Notes:	 Origin: ntv2status.cpp.  Doesn't have all functions 
// 			 from that file.
//
///////////////////////////////////////////////////////////////

#ifndef NTV2DRIVERSTATUS_HEADER
#define NTV2DRIVERSTATUS_HEADER

void getDeviceVersionString(ULWord deviceNumber, char *deviceVersionString, ULWord strMax);
void getPCIFPGAVersionString(ULWord deviceNumber, char *pcifpgaVersionString, ULWord strMax);
NTV2DeviceID getDeviceID(ULWord deviceNumber);
void getDeviceIDString(ULWord deviceNumber, char *deviceIDString, ULWord strMax);
void getDriverVersionString(char *driverVersionString, ULWord strMax);
void getDeviceSerialNumberString(ULWord deviceNumber, char *deviceIDString, ULWord strMax);

#endif

