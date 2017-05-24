/**
	@file		pnp/mac/pnpimpl.cpp
	@copyright	Copyright (C) 2011-2017 AJA Video Systems, Inc.  All rights reserved.
	@brief		Implements the AJAPnpImpl class on the Mac platform.
**/

#include "ajabase/pnp/mac/pnpimpl.h"

// for now use the ntv2version
#include "devicenotifier.h"


// static
bool sOnline = false;
void PCIDeviceNotifierCallback (unsigned long message, void *refcon);
void FWDeviceNotifierCallback (unsigned long message, void *refcon);



AJAPnpImpl::AJAPnpImpl() : mRefCon(NULL), mCallback(NULL), mDevices(0)
{
	mPciDevices = new KonaNotifier(PCIDeviceNotifierCallback, this);
}


AJAPnpImpl::~AJAPnpImpl()
{
	Uninstall();
	
	delete mPciDevices;
	mPciDevices = NULL;
}


AJAStatus 
AJAPnpImpl::Install(AJAPnpCallback callback, void* refCon, uint32_t devices)
{
	mCallback = callback;
	mRefCon = refCon;
	mDevices = devices;
	
	// pci devices
	if ((mDevices & AJA_Pnp_PciVideoDevices) != 0)
	{
		mPciDevices->Install();
	}
	
	return AJA_STATUS_SUCCESS;
}
	

AJAStatus 
AJAPnpImpl::Uninstall(void)
{
	mCallback = NULL;
	mRefCon = NULL;
	mDevices = 0;
	
	return AJA_STATUS_SUCCESS;
}


AJAPnpCallback 
AJAPnpImpl::GetCallback()
{
	return mCallback;
}


void* 
AJAPnpImpl::GetRefCon()
{
	return mRefCon;
}


uint32_t
AJAPnpImpl::GetPnpDevices()
{
	return mDevices;
}


// static - translate a PCIDeviceNotifierCallback/message to a AJAPnpCallback/message
void PCIDeviceNotifierCallback  (unsigned long message, void *refcon)
{
	//printf ("PCIDeviceNotifierCallback - message = %d\n", message);
	
	AJAPnpImpl* pnpObj = (AJAPnpImpl*) refcon;
	if (pnpObj == NULL)
	{
		printf ("PCIDeviceNotifierCallback - refcon=NULL message=%x\n", (int)message);
		return;
	}
	
	
	AJAPnpCallback callback = pnpObj->GetCallback();
	if (callback == NULL)
	{
		printf ("PCIDeviceNotifierCallback - callback=NULL message=%x\n", (int)message);
		return;
	}
	
	
	switch (message)
	{
		case kIOMessageServiceIsResumed:
			// printf("PCIDeviceNotifierCallback - kIOMessageServiceIsResumed");
			break;
			
		case kIOMessageServiceIsSuspended:
			// printf("PCIDeviceNotifierCallback - kIOMessageServiceIsResumed");
			break;
			
		case kIOMessageServiceIsAttemptingOpen:
			//printf ("PCIDeviceNotifierCallback - kIOMessageServiceIsAttemptingOpen\n");
			break;
			
		case kIOMessageServiceWasClosed:
			//printf ("PCIDeviceNotifierCallback - kIOMessageServiceWasClosed\n");
			break;
			
		case kIOMessageServiceIsTerminated:
			//printf ("PCIDeviceNotifierCallback - kIOMessageServiceIsTerminated\n");
			break;
			
		case kAJADeviceInitialOpen:
			//printf ("PCIDeviceNotifierCallback - deviceCallback - kAJADeviceInitialOpen\n");
			(callback)(AJA_Pnp_DeviceAdded, pnpObj->GetRefCon());
			break;
			
		case kAJADeviceTerminate:
			//printf ("PCIDeviceNotifierCallback - kAJADeviceTerminate\n");
			(callback)(AJA_Pnp_DeviceRemoved, pnpObj->GetRefCon());
			break;
			
		default:
			break;
	}
}


// static - translate a FWDeviceNotifierCallback/message to a AJAPnpCallback/message
void FWDeviceNotifierCallback (unsigned long message, void *refcon)
{
	//printf ("FWDeviceNotifierCallback - message = %d\n", message);
	
	AJAPnpImpl* pnpObj = (AJAPnpImpl*) refcon;
	if (pnpObj == NULL)
	{
		printf ("FWDeviceNotifierCallback - refcon=NULL message=%x\n", (int)message);
		return;
	}
	
	AJAPnpCallback callback = pnpObj->GetCallback();
	if (callback == NULL)
	{
		printf ("FWDeviceNotifierCallback - callback=NULL message=%x\n", (int)message);
		return;
	}
	
	switch (message)
	{
		case kIOMessageServiceIsSuspended:
			//printf ("kIOMessageServiceIsSuspended\n");
			sOnline = false;
			(callback)(AJA_Pnp_DeviceOffline, pnpObj->GetRefCon());
			break;
			
		case kIOMessageServiceIsResumed:
			//printf ("kIOMessageServiceIsResumed\n");
			sOnline = true;
			(callback)(AJA_Pnp_DeviceOnline, pnpObj->GetRefCon());
			break;
			
		case kIOMessageServiceIsTerminated:
			//printf ("kIOMessageServiceIsTerminated\n");
			break;
			
		case kAJADeviceInitialOpen:
			//printf ("kAJADeviceInitialOpen\n");
			(callback)(AJA_Pnp_DeviceAdded, pnpObj->GetRefCon());
			break;
			
		case kIOMessageServiceWasClosed:
			//printf ("kIOMessageServiceWasClosed\n");
			break;
			
		case kIOMessageServiceIsAttemptingOpen:	
			//printf ("kIOMessageServiceIsAttemptingOpen\n");
			break;
			
		default:
			//printf ("deviceCallback - unknown message=%d\n", (int)message);
			break;
	}
}
