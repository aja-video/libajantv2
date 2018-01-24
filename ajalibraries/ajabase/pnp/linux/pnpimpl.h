/**
	@file		pnp/linux/pnpimpl.h
	@copyright	Copyright (C) 2011-2018 AJA Video Systems, Inc.  All rights reserved.
	@brief		Declares the AJAPnpImpl class.
**/

#ifndef AJA_PNP_IMPL_H
#define AJA_PNP_IMPL_H

#include "ajabase/pnp/pnp.h"

class AJAPnpImpl
{
public:
	AJAPnpImpl();
	virtual ~AJAPnpImpl(void);

	AJAStatus Install(AJAPnpCallback callback, void* refCon, uint32_t devices);
	AJAStatus Uninstall(void);
	
	AJAPnpCallback GetCallback();
	void* GetRefCon();
	uint32_t GetPnpDevices();

private:
	
	void*				mRefCon;
	AJAPnpCallback		mCallback;
	uint32_t			mDevices;
};

#endif	//	AJA_PNP_IMPL_H
