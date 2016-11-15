/**
	@file		pnp/ajadevice.h
	@copyright	Copyright (C) 2013-2016 AJA Video Systems, Inc.  All rights reserved.
	@brief		Implements the AJADevice class.
**/

#include "ajadevice.h"
#include <map>
#include <sstream>

using namespace std;


const std::string &		AJAInterfaceName_NTV2	("NTV2");
const std::string &		AJAInterfaceName_NTV4	("NTV4");


AJADeviceIdentifier AJADevice::EncodeDeviceIdentifier (const uint32_t inDeviceID, const uint64_t inSerialNumber)
{
	return "0x" + EncodeDeviceID (inDeviceID) + "/" + EncodeSerialNumber (inSerialNumber);

}	//	EncodeDeviceIdentifier


bool AJADevice::DecodeDeviceIdentifier (const AJADeviceIdentifier & inDeviceID, uint32_t & outDeviceID, uint64_t outSerialNumber)
{
	return DecodeDeviceID (inDeviceID, outDeviceID) && DecodeSerialNumber (inDeviceID, outSerialNumber);

}	//	DecodeDeviceIdentifier


std::string AJADevice::EncodeDeviceID (const uint32_t inDeviceID)
{
	ostringstream	oss;
	oss	<< hex << inDeviceID << dec;
	return oss.str ();

}	//	EncodeDeviceID


std::string AJADevice::EncodeSerialNumber (const uint64_t inSerialNumber)
{
	ostringstream	oss;
	oss	<< hex << inSerialNumber << dec;
	return oss.str ();

}	//	EncodeSerialNumber


bool AJADevice::DecodeDeviceID (const AJADeviceIdentifier & inDeviceID, uint32_t & outDeviceID)
{
	outDeviceID = 0;

	//	Must be of form "0xXXXXXXXX/YYYYYYYYYYYYYYYY" -- exactly 27 chars long
	if (inDeviceID.length () < 27)
		return false;

	const size_t	pos	(inDeviceID.find ("0x"));
	if (pos != 0)
		return false;	//	Missing '0x' in position 0

	const string	deviceIDPart	(inDeviceID.substr (pos + 2, 8));
	if (deviceIDPart.length () != 8)
		return false;	//	Bad or missing deviceIDPart

	//cerr << "DecodeDeviceID '" << inDeviceID << "', deviceID='" << deviceIDPart << "'" << endl;

	istringstream	iss		(deviceIDPart);
	iss >> hex >> outDeviceID >> dec;

	return iss.rdstate () & (istringstream::failbit | istringstream::badbit) ? false : true;

}	//	DecodeDeviceID


bool AJADevice::DecodeSerialNumber (const AJADeviceIdentifier & inDeviceID, uint64_t outSerialNumber)
{
	outSerialNumber = 0;

	//	Must be of form "0xXXXXXXXX/YYYYYYYYYYYYYYYY" -- exactly 27 chars long
	if (inDeviceID.length () < 27)
		return false;

	const size_t	slashPos	(inDeviceID.find ("/"));
	if (slashPos != 10)
		return false;	//	Missing '/' in position 10

	const string	serialNumPart	(inDeviceID.substr (slashPos + 1, 16));
	if (serialNumPart.length () != 16)
		return false;	//	Bad or missing serialNum part

	//cerr << "DecodeSerialNumber '" << inDeviceID << "', sernum='" << serialNumPart << "'" << endl;

	istringstream	iss		(serialNumPart);
	iss >> hex >> outSerialNumber >> dec;

	return iss.rdstate () & (istringstream::failbit | istringstream::badbit) ? false : true;

}	//	DecodeSerialNumber


/**
	@brief	AJADevices know how to stream themselves in a human-readable format.
**/
ostream &	operator << (ostream & inOutStr, const AJADevice & inObj)
{
	return inObj.Print (inOutStr);

}	//	AJADevice ostream operator <<


ostream &	operator << (ostream & inOutStr, const AJADevicePtr & inObj)
{
	return inObj ? inOutStr << *inObj : inOutStr;

}	//	AJADevice ostream operator <<


ostream & AJADevice::Print (ostream & inOutStr) const
{
	return inOutStr	<< GetInterfaceName () << "://" << GetDeviceClassName () << "/" << GetIdentifier ();

}	//	Print


void AJADevice::pvt_Terminated (void)
{
	cerr << "AJADevice instance " << hex << this << dec << " closed" << endl;

}	//	pvt_Terminated


AJADevice::AJADevice (const string & inDeviceClassName)
	:	mDeviceClassName	(inDeviceClassName)
{
}	//	constructor


AJADevice::~AJADevice ()
{
	cerr << "AJADevice instance " << hex << this << dec << " destructed" << endl;
	mInterfaceName.clear ();

}	//	destructor


bool AJADevice::NTV2ReadRegister (const uint64_t inRegisterNumber, uint32_t & outRegisterValue, const uint64_t inRegisterMask) const
{
    (void)inRegisterNumber;
    (void)outRegisterValue;
    (void)inRegisterMask;

	return false;

}	//	NTV2ReadRegister


#define My_NTV4_BUILD_TIME_SIZE		64


typedef struct _My_NTV4MessageHeader
{
	uint64_t	type;
	uint64_t	target;
	uint64_t	status;
	uint64_t	size;
	uint64_t	userContext;
	uint64_t	driverContext;
	uint64_t	flags;

}	My_NTV4MessageHeader;


typedef struct _My_NTV4MessageDeviceInfo
{
	My_NTV4MessageHeader	header;
	uint64_t				deviceInfoFlags;
	uint64_t				deviceType;
	uint64_t				deviceVersion;
	uint64_t				driverVersionMajor;
	uint64_t				driverVersionMinor;
	uint64_t				driverVersionRelease;
	uint64_t				driverVersionBuild;
	char					driverBuildDate [My_NTV4_BUILD_TIME_SIZE];
	char					driverBuildTime [My_NTV4_BUILD_TIME_SIZE];
	uint64_t				driverType;
	uint64_t				driverInterfaceVersion;

} My_NTV4MessageDeviceInfo;


#define My_NTV4_DRIVER_INTERFACE_VERSION	5

#define NTV4_MSG_DRIVER_INIT(_pMsg_, _type_)		memset (_pMsg_, 0, sizeof (*(_pMsg_)));							\
													(_pMsg_)->header.type = (uint64_t) _type_;						\
													(_pMsg_)->header.target = (uint64_t) NTV4_MessageTarget_Driver;	\
													(_pMsg_)->header.status = (uint64_t) AJA_STATUS_UNKNOWN;		\
													(_pMsg_)->header.size = (uint64_t) sizeof (*(_pMsg_));



bool AJADevice::NTV4SendMessage (void * pInMessage) const
{
    (void)pInMessage;

	return false;
}	//	NTV4SendMessage
