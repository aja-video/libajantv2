/**
	@file		pnp/ajadevicescanner.cpp
	@copyright	Copyright (C) 2013-2016 AJA Video Systems, Inc.  All rights reserved.
	@brief		Implementation of the AJADeviceScanner class.
**/
#include "ajadevicescanner.h"
#include "ajabase/system/lock.h"
#include <map>
#include <set>
#include <sstream>


using namespace std;


/**
	@brief	The active device map is a table of known AJADevicePtr instances indexed by AJADeviceIdentifier.
**/
typedef map <AJADeviceIdentifier, AJADevicePtr>		ActiveDeviceMap;
typedef	ActiveDeviceMap::iterator					ActiveDeviceMapIter;
typedef	ActiveDeviceMap::const_iterator				ActiveDeviceMapConstIter;
typedef pair <AJADeviceIdentifier, AJADevicePtr>	DeviceKeyPtrPair;


typedef pair <AJADeviceNotificationCallback, void *>	CallbackUserDataPair;
typedef	set <CallbackUserDataPair>						CallbackSet;
typedef	CallbackSet::iterator							CallbackSetIter;
typedef	CallbackSet::const_iterator						CallbackSetConstIter;

static ActiveDeviceMap								gActiveDevices;												///	My active devices
static AJALock										gActiveDevicesLock			("gActiveDevicesLock");			///	My ActiveDeviceMap's read/write lock
static CallbackSet									gNotificationCallbacks;										///	Notification callback/userdata pairs
static AJALock										gNotificationCallbacksLock	("gNotificationCallbacksLock");	///	My callback pointer's read/write lock


/**
	@brief	An ActiveDeviceMap knows how to stream itself in a human-readable format.
**/
ostream &	operator << (ostream & inOutStr, const ActiveDeviceMap & inObj)
{
	inOutStr << inObj.size () << " entries:" << endl;

	unsigned	ndx	(0);
	for (ActiveDeviceMapConstIter iter (inObj.begin ());  iter != inObj.end ();  ++iter)
		inOutStr << ndx++ << ":  " << iter->first << " ==> " << iter->second << endl;

	return inOutStr;

}	//	ActiveDeviceMap ostream operator <<


AJADevicePtr ActiveDeviceMap_Find (AJADevice * pInDevice)
{
	AJAAutoLock	readLocker (&gActiveDevicesLock);

	//	Scan the active devices, looking for the given underlying AJADevice instance pointer.
	//	Sequential search should be acceptible because...
	//		a)	There usually aren't more than 3 devices on any given host;
	//		b)	Devices aren't constantly being attached/detached.
	ActiveDeviceMapConstIter	iter	(gActiveDevices.begin ());
	while (iter != gActiveDevices.end ())
	{
		if (pInDevice == iter->second.get ())
			return iter->second;
		++iter;
	}	//	while

	return AJADevicePtr ();	//	Not found

}	//	ActiveDeviceMap_Find


bool ActiveDeviceMap_Remove (AJADevice * pInDevice)
{
	AJADevicePtr	removedDevice;
	{
		AJAAutoLock	writeLocker (&gActiveDevicesLock);
		assert (pInDevice && "Device being removed must be valid");
		ActiveDeviceMapIter	iter	(gActiveDevices.begin ());
		while (iter != gActiveDevices.end ())
		{
			if (pInDevice == iter->second.get ())
			{
				removedDevice = iter->second;
				gActiveDevices.erase (iter);
				//cerr << gActiveDevices;
				break;
			}
			++iter;
		}	//	for each table entry
		assert (removedDevice && "Device being removed not found");
	}	//	gActiveDevicesLock writeLocker scope

	if (removedDevice)
	{
		AJAAutoLock	readLocker (&gNotificationCallbacksLock);
		for (CallbackSetConstIter iter (gNotificationCallbacks.begin ());  iter != gNotificationCallbacks.end ();  ++iter)
		{
			AJADeviceNotificationCallback	pNotificationCallback	(iter->first);
			void *							pUserData				(iter->second);
			assert (pNotificationCallback && "NULL function pointer somehow got added to callback list");
			(*pNotificationCallback) (AJA_DeviceRemoved, pUserData, removedDevice);
		}	//	for each installed callback/userdata pair
	}

	return removedDevice;

}	//	ActiveDeviceMap_Remove


bool ActiveDeviceMap_Add (AJADevicePtr inNewDevice)
{
	AJAAutoLock	writeLocker (&gActiveDevicesLock);
	assert (inNewDevice && "New device being added must be valid");
	assert (!ActiveDeviceMap_Find (inNewDevice.get ()).get () && "New device being added already in table!");
	try
	{
		gActiveDevices.insert (DeviceKeyPtrPair (inNewDevice->GetIdentifier (), inNewDevice));
	}
	catch (bad_alloc)
	{
		return false;	//	Insertion failed
	}
	assert (ActiveDeviceMap_Find (inNewDevice.get ()) && "Added device not found!");

	//cerr << gActiveDevices;

	if (inNewDevice)
	{
		AJAAutoLock	readLocker (&gNotificationCallbacksLock);
		for (CallbackSetConstIter iter (gNotificationCallbacks.begin ());  iter != gNotificationCallbacks.end ();  ++iter)
		{
			AJADeviceNotificationCallback	pNotificationCallback	(iter->first);
			void *							pUserData				(iter->second);
			assert (pNotificationCallback && "NULL function pointer somehow got added to callback list");
			(*pNotificationCallback) (AJA_DeviceAdded, pUserData, inNewDevice);
		}	//	for each installed callback/userdata pair
	}

	return true;

}	//	ActiveDeviceMap_Add


class AJADeviceScannerImpl;
typedef AJARefPtr <AJADeviceScannerImpl>	AJADeviceScannerImplPtr;	///	The platform-specific reference-counted "ptr"
static AJADeviceScannerImplPtr				gScannerSingleton;			///	The platform-specific AJADeviceScanner singleton


#if defined (AJA_MAC)
	#include "ajadevicescannermacimpl.hpp"
#elif defined (AJA_WINDOWS)
	#include "ajadevicescannerwinimpl.hpp"
#elif defined (AJA_LINUX)
	#include "ajadevicescannerlinuximpl.hpp"
#endif


std::ostream & operator << (std::ostream & inOutStr, const AJADeviceList & inList)
{
	unsigned	ndx	(0);

	inOutStr << inList.size () << " device(s)" << endl;
	for (AJADeviceListConstIter iter (inList.begin ());  iter != inList.end ();  ++iter)
		inOutStr << ndx++ << ":  " << *iter << endl;

	return inOutStr;

}	//	AJADeviceList operator <<


AJADeviceList AJADeviceScanner::GetAllDevices (void)
{
	AJADeviceList	result;
	if (AJADeviceScannerImpl::Get ())
	{
		AJAAutoLock	readLocker (&gActiveDevicesLock);
		try
		{
			for (ActiveDeviceMapConstIter iter (gActiveDevices.begin ());  iter != gActiveDevices.end ();  ++iter)
				result.push_back (iter->second);
		}
		catch (std::bad_alloc)
		{
			result.clear ();
			cerr << "bad_alloc in AJADeviceScanner::GetAllDevices" << endl;
		}
	}
	return result;

}	//	GetAllDevices


AJADeviceList AJADeviceScanner::GetAllNTV2Devices (void)
{
	AJADeviceList	result;
	if (AJADeviceScannerImpl::Get ())
	{
		AJAAutoLock	readLocker (&gActiveDevicesLock);
		try
		{
			for (ActiveDeviceMapConstIter iter (gActiveDevices.begin ());  iter != gActiveDevices.end ();  ++iter)
				if (iter->second->HasNTV2Interface ())
					result.push_back (iter->second);
		}
		catch (std::bad_alloc)
		{
			result.clear ();
			cerr << "bad_alloc in AJADeviceScanner::GetAllNTV2Devices" << endl;
		}
	}
	return result;

}	//	GetAllNTV2Devices


AJADeviceList AJADeviceScanner::GetAllNTV4Devices (void)
{
	AJADeviceList	result;
	if (AJADeviceScannerImpl::Get ())
	{
		AJAAutoLock	readLocker (&gActiveDevicesLock);
		try
		{
			for (ActiveDeviceMapConstIter iter (gActiveDevices.begin ());  iter != gActiveDevices.end ();  ++iter)
				if (iter->second->HasNTV4Interface ())
					result.push_back (iter->second);
		}
		catch (std::bad_alloc)
		{
			result.clear ();
			cerr << "bad_alloc in AJADeviceScanner::GetAllNTV4Devices" << endl;
		}
	}
	return result;

}	//	GetAllNTV4Devices


AJADeviceList AJADeviceScanner::FindDevices (const uint32_t inDeviceID)
{
	AJADeviceList	result;
	ostringstream	oss;
	oss	<< "0x" << hex << inDeviceID << dec << "/";

	if (AJADeviceScannerImpl::Get ())
	{
		AJAAutoLock		readLocker	(&gActiveDevicesLock);
		const string	deviceIDKey	(oss.str ());
		try
		{
			for (ActiveDeviceMapConstIter iter (gActiveDevices.begin ());  iter != gActiveDevices.end ();  ++iter)
				if (iter->second->GetIdentifier ().find (deviceIDKey) != string::npos)
					result.push_back (iter->second);
		}
		catch (std::bad_alloc)
		{
			result.clear ();
			cerr << "bad_alloc in AJADeviceScanner::GetAllNTV2Devices" << endl;
		}
	}
	return result;

}	//	FindDevices


AJADevicePtr AJADeviceScanner::FindDevice (const uint64_t inSerialNumber)
{
	AJADevicePtr	result;

	if (AJADeviceScannerImpl::Get ())
	{
		AJAAutoLock		readLocker		(&gActiveDevicesLock);
		const string	searchKey ("/" + AJADevice::EncodeSerialNumber (inSerialNumber));

		for (ActiveDeviceMapConstIter iter (gActiveDevices.begin ());  iter != gActiveDevices.end ();  ++iter)
			if (iter->second->GetIdentifier ().find (searchKey) != string::npos)
			{
				result = iter->second;
				break;		//	Found it!
			}
	}
	return result;

}	//	FindDevice


AJADevicePtr AJADeviceScanner::FindDevice (const AJADeviceIdentifier inDeviceIdentifier)
{
	AJADevicePtr	result;

	if (AJADeviceScannerImpl::Get ())
	{
		AJAAutoLock					readLocker	(&gActiveDevicesLock);
		ActiveDeviceMapConstIter	iter		(gActiveDevices.find (inDeviceIdentifier));

		if (iter != gActiveDevices.end ())
			result = iter->second;	//	Found it!
	}
	return result;

}	//	FindDevice


bool AJADeviceScanner::HasInstalledNotificationCallback (AJADeviceNotificationCallback pInCallback, void * pInUserData)
{
	CallbackUserDataPair	key			(pInCallback, pInUserData);
	AJAAutoLock				readLocker	(&gNotificationCallbacksLock);
	CallbackSetConstIter	iter		(gNotificationCallbacks.find (key));

	return iter != gNotificationCallbacks.end ();

}	//	HasInstalledNotificationCallback


bool AJADeviceScanner::AddNotificationCallback (AJADeviceNotificationCallback pInCallback, void * pInUserData, const bool inPlayAttached)
{
	if (pInCallback)
	{
		CallbackUserDataPair	key			(pInCallback, pInUserData);
		AJAAutoLock				writeLocker	(&gNotificationCallbacksLock);
		CallbackSetConstIter	iter		(gNotificationCallbacks.find (key));

		if (iter != gNotificationCallbacks.end ())
			return false;		//	Already exists -- fail!

		try
		{
			gNotificationCallbacks.insert (key);
		}
		catch (std::bad_alloc)
		{
			gNotificationCallbacks.clear ();
			cerr << "bad_alloc in AJADeviceScanner::AddNotificationCallback" << endl;
		}
	}	//	writeLocker scope
	else
		return false;	//	NULL callback function pointer

	//	Ensure device scanner singleton exists...
	AJADeviceScannerImpl::Get ();

	if (inPlayAttached)
	{
		AJAAutoLock	readLocker (&gActiveDevicesLock);

		for (ActiveDeviceMapConstIter deviceIter (gActiveDevices.begin ());  deviceIter != gActiveDevices.end ();  ++deviceIter)
		{
			AJAAutoLock	readLocker (&gNotificationCallbacksLock);
			for (CallbackSetConstIter cbIter (gNotificationCallbacks.begin ());  cbIter != gNotificationCallbacks.end ();  ++cbIter)
			{
				AJADeviceNotificationCallback	pNotificationCallback	(cbIter->first);
				void *							pUserData				(cbIter->second);
				assert (pNotificationCallback && "NULL function pointer somehow got added to callback list");
				(*pNotificationCallback) (AJA_DeviceAdded, pUserData, deviceIter->second);
			}	//	for each installed callback/userdata pair
		}	//	for each attached device
	}	//	if inPlayAttached

	return true;

}	//	AddNotificationCallback


bool AJADeviceScanner::RemoveNotificationCallback (AJADeviceNotificationCallback pInCallback, void * pInUserData)
{
	CallbackUserDataPair	key			(pInCallback, pInUserData);
	AJAAutoLock				writeLocker	(&gNotificationCallbacksLock);
	CallbackSetIter			iter		(gNotificationCallbacks.find (key));

	if (iter == gNotificationCallbacks.end ())
		return false;		//	Not found -- fail!

	gNotificationCallbacks.erase (key);

	return true;

}	//	RemoveNotificationCallback


/**
	@brief	I sit around waiting for main() to exit(), whereupon I destroy all static globals in this module.
**/
class DeviceScannerDestroyer
{
	public:
		explicit inline DeviceScannerDestroyer ()		{cerr << "DeviceScannerDestroyer is armed" << endl;}
		virtual inline ~DeviceScannerDestroyer ()
		{
			cerr << "DeviceScannerDestroyer triggered" << endl;
			gNotificationCallbacks.clear ();
			gScannerSingleton = NULL;
			AJAAutoLock	writeLocker (&gActiveDevicesLock);
			gActiveDevices.clear ();
		}	//	destructor

	private:
		//	Do not copy!
		inline								DeviceScannerDestroyer (const DeviceScannerDestroyer & inObj)		{if (&inObj != this) assert (false);}
		inline DeviceScannerDestroyer &		operator = (const DeviceScannerDestroyer & inRHS)					{if (&inRHS != this) assert (false); return *this;}

};	//	ScannerDestroyer

static DeviceScannerDestroyer	gDeviceScannerDestroyer;	///	DeviceScannerDestroyer singleton
