/**
	@file		lock.h
	@copyright	Copyright (C) 2009-2017 AJA Video Systems, Inc.  All rights reserved.
	@brief		Declares the AJALock class.
**/

#ifndef AJA_LOCK_H
#define AJA_LOCK_H

#include "ajabase/common/public.h"

// forward declarations
class AJALockImpl;

/**
 *	Class to coordinate access to non reentrant code.
 *	@ingroup AJAGroupSystem
 */
class AJA_EXPORT AJALock
{
public:

	/**
	 *	Constructor obtains a lockable object from the system.
	 *
	 *	Specify a name if the lock is to be shared system wide.
	 *
	 *	@param	pName	Name of a shared lock object.
	 */
	AJALock(const char* pName = NULL);
	virtual ~AJALock();

	/**
	 *	Request the lock.
	 *
	 *	@param[in]						timeout	Request timeout in milliseconds (0xffffffff infinite).
	 *	@return		AJA_STATUS_SUCCESS	Object locked
	 *				AJA_STATUS_TIMEOUT	Lock wait timeout
	 *				AJA_STATUS_OPEN		Lock not initialized
	 *				AJA_STATUS_FAIL		Lock failed
	 */
	virtual AJAStatus Lock(uint32_t timeout = 0xffffffff);

	/**
	 *	Release the lock.
	 *
	 *	@return		AJA_STATUS_SUCCESS	Lock released
	 *				AJA_STATUS_OPEN		Lock not initialized
	 */
	virtual AJAStatus Unlock();

private:

	AJALockImpl* mpImpl;
};

/**
 *	Automatically obtain and release a lock.
 */
class AJA_EXPORT AJAAutoLock
{
public:

	/**
	 *	Constructor waits for lock.
	 *
	 *	@param[in]	pLock	The required lock.
	 */
	AJAAutoLock(AJALock* pLock = NULL);

	/**
	 *	Destructor releases lock.
	 */
	virtual ~AJAAutoLock();

private:

	AJALock* mpLock;
};


#endif	//	AJA_LOCK_H
