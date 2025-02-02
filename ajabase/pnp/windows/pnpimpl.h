/* SPDX-License-Identifier: MIT */
/**
	@file		pnp/windows/pnpimpl.h
	@brief		Declares the AJAPnpImpl class.
	@copyright	(C) 2011-2022 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef AJA_PNP_IMPL_H
#define AJA_PNP_IMPL_H

#include <Windows.h>
#include "ajabase/pnp/pnp.h"

class AJAPnpImpl
{
public:
							AJAPnpImpl();
	virtual					~AJAPnpImpl();

	AJAStatus				Install (AJAPnpCallback callback, void* refCon, uint32_t devices);
	AJAStatus				Uninstall (void);
	
	inline AJAPnpCallback	GetCallback (void) const	{return mCallback;}
	inline void *			GetRefCon (void) const		{return mRefCon;}
	inline uint32_t			GetPnpDevices (void) const	{return mDevices;}

	void					AddSignaled (void);
	void					RemoveSignaled (void);

private:
	void*				mRefCon;
	AJAPnpCallback		mCallback;
	uint32_t			mDevices;
	HANDLE				mAddEventHandle;
	HANDLE				mAddWaitHandle;
	HANDLE				mRemoveEventHandle;
	HANDLE				mRemoveWaitHandle;
	bool				mbInstalled;
};

#endif	//	AJA_PNP_IMPL_H
